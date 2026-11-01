#include "InputContext.h"
#include "DynamicArray.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

static inline void text_render_info_array_init(TextRenderInfoArray* infoArray, InputContext* inputContext) {
    TextRenderInfo info;
    info.text = TTF_CreateText(inputContext->app_context->textEngine, inputContext->font, "", 0);
    if (info.text == NULL) {
        SDL_Log("Failed to create text object for TextRenderInfo: %s", SDL_GetError());
    }
    DYNAMIC_ARRAY_INIT(info.sequence);

    DYNAMIC_ARRAY_INIT((*infoArray));
    DYNAMIC_ARRAY_APPEND((*infoArray), info);
}

static inline void text_render_info_array_free(TextRenderInfoArray* infoArray) {
    for (int i = 0; i < infoArray->len; i++) {
        TextRenderInfo currInfo = infoArray->arr[i];
        TTF_DestroyText(currInfo.text);
        for (int j = 0; j < currInfo.sequence.len; j++) {
            GPUBuffer_unref(currInfo.sequence.arr[j].vertexBuffer);
            GPUBuffer_unref(currInfo.sequence.arr[j].indexBuffer);
        }
        DYNAMIC_ARRAY_FREE(currInfo.sequence);
    }
    DYNAMIC_ARRAY_FREE((*infoArray));
}

typedef struct IntArray {
    int* arr;
    size_t len;
    size_t capacity;
} IntArray;

static inline void text_line_init(TextLine* textLine, InputContext* inputContext) {
    DYNAMIC_ARRAY_INIT((*textLine));
    text_render_info_array_init(&textLine->renderInfo, inputContext);
    textLine->lineWasUpdated = false;
}

static inline void text_line_destroy(TextLine* textLine) {
    DYNAMIC_ARRAY_FREE((*textLine));
    text_render_info_array_free(&textLine->renderInfo);
}

static inline int get_num_codepoints(char* string, int len_bytes) {
    const char* string_copy = (const char*)string;
    size_t psLen = len_bytes;

    int tracker = 0;
    int numCodePoints = 0;
    while (tracker < len_bytes) {
        size_t psLenCopy = psLen;
        Uint32 code = SDL_StepUTF8(&string_copy, &psLen);
        int code_width = psLenCopy - psLen;
        tracker += code_width;
        numCodePoints++;
    }
    return numCodePoints;
}

static inline int get_byte_offset_from_codepoints(char* string, int string_len_bytes, int num_codepoints) {
    const char* string_copy = (const char*)string;
    size_t psLen = string_len_bytes;

    int numBytes = 0;
    int codePointCount = 0;
    while (codePointCount < num_codepoints && numBytes < string_len_bytes) {
        size_t psLenCopy = psLen;
        Uint32 code = SDL_StepUTF8(&string_copy, &psLen);
        int code_width = psLenCopy - psLen;
        numBytes += code_width;
        codePointCount++;
    }
    return numBytes;
}

typedef struct TextVertex {
    SDL_FPoint point;
    SDL_FPoint tex;
} TextVertex;

static inline int InputContextGenerateGPUBuffersForLine(AppContext* app_context, TextRenderInfo* renderInfo) {
    for (size_t i = 0; i < renderInfo->sequence.len; i++) {
        GPUBuffer_unref(renderInfo->sequence.arr[i].vertexBuffer);
        GPUBuffer_unref(renderInfo->sequence.arr[i].indexBuffer);
    }
    DYNAMIC_ARRAY_CLEAR(renderInfo->sequence);

    TTF_GPUAtlasDrawSequence* sequence = TTF_GetGPUTextDrawData(renderInfo->text);
    if (sequence == NULL) {
        // NULL sequence means there is nothing to render
        return 0;
    }

    for (; sequence != NULL; sequence = sequence->next) {
        TextRenderInfoSequence textSequence;
        switch (sequence->image_type) {
        case TTF_IMAGE_ALPHA: {
            textSequence.imageType = TEXT_RENDER_INFO_IMAGE_TYPE_ALPHA_ONLY;
        } break;

        case TTF_IMAGE_COLOR: {
            textSequence.imageType = TEXT_RENDER_INFO_IMAGE_TYPE_COLOR;
        } break;

        default: {
            textSequence.imageType = TEXT_RENDER_INFO_IMAGE_TYPE_ALPHA_ONLY;
        }
        }
        textSequence.atlas = sequence->atlas_texture;
        textSequence.numIndices = sequence->num_indices;
        textSequence.vertexBuffer = GPUBuffer_create(app_context->device, &(SDL_GPUBufferCreateInfo){
                                                                              .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                                                              .size = sequence->num_vertices * sizeof(TextVertex),
                                                                              .props = 0,
                                                                          });
        textSequence.indexBuffer = GPUBuffer_create(app_context->device, &(SDL_GPUBufferCreateInfo){
                                                                             .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                                                             .size = sequence->num_indices * sizeof(int),
                                                                             .props = 0,
                                                                         });

        TransferContextData vertexData = GPUTransferContextAcquireStagingBuffer(&app_context->transferContext, sequence->num_vertices * sizeof(TextVertex));
        TransferContextData indexData = GPUTransferContextAcquireStagingBuffer(&app_context->transferContext, sequence->num_indices * sizeof(int));

        TextVertex* vertices = vertexData.buffer;
        int* indices = indexData.buffer;

        for (int i = 0; i < sequence->num_vertices; i++) {
            vertices[i] = (TextVertex){.point = sequence->xy[i], .tex = sequence->uv[i]};
        }
        SDL_memcpy(indices, sequence->indices, sequence->num_indices * sizeof(int));

        GPUTransferContextUpload(&app_context->transferContext, textSequence.vertexBuffer, 0, vertexData);
        GPUTransferContextUpload(&app_context->transferContext, textSequence.indexBuffer, 0, indexData);

        DYNAMIC_ARRAY_APPEND(renderInfo->sequence, textSequence);
    }

    return 0;
}

typedef struct StringSlice {
    char* string;
    size_t len;
} StringSlice;

typedef struct StringSliceArray {
    StringSlice* arr;
    size_t len;
    size_t capacity;
} StringSliceArray;

// Takes in the given line index and breaks it down into however many are needed to fit within the bounds.
// Specifically, this wraps by splitting up words unless there is only one word on the line, in which case
// it will wrap by characters. The caller must free the array using DYNAMIC_ARRAY_FREE
static inline void update_line_text_wrap(InputContext* inputContext, int lineIndex) {
    DYNAMIC_ARRAY_FREE(inputContext->buffer.arr[lineIndex].renderInfo);

    char* lineString = inputContext->buffer.arr[lineIndex].arr;
    int lineStringLen = inputContext->buffer.arr[lineIndex].len;

    int layoutWidth = inputContext->layout.width;

    int lineWidth, lineHeight;
    TTF_GetStringSize(inputContext->font, lineString, lineStringLen, &lineWidth, &lineHeight);

    if (lineWidth <= layoutWidth) {
        return;
    }

    const char* line_copy = (const char*)lineString;
    size_t psLen = lineStringLen;
    int startByteOffset = 0;
    int currByteOffset = 0;
    while (psLen > 0) {
        size_t psLenCopy = psLen;
        Uint32 code = SDL_StepUTF8(&line_copy, &psLen);
        int code_width = psLenCopy - psLen;
        currByteOffset += code_width;

        if (code == ' ') {
            int testWidth, testHeight;
            TTF_GetStringSize(inputContext->font, &lineString[startByteOffset], currByteOffset - startByteOffset, &testWidth, &testHeight);
            startByteOffset = currByteOffset;
        }
    }
}

int InputContextCreate(InputContext* inputContext, AppContext* appContext, TTF_Font* font, float textColor[4], float cursorColor[4], InputContextLayout layout) {
    inputContext->cursorHeight = TTF_GetFontHeight(font);
    inputContext->cursorX = 0;
    inputContext->cursorY = 0;

    inputContext->textColor[0] = textColor[0];
    inputContext->textColor[1] = textColor[1];
    inputContext->textColor[2] = textColor[2];
    inputContext->textColor[3] = textColor[3];

    inputContext->cursorColor[0] = cursorColor[0];
    inputContext->cursorColor[1] = cursorColor[1];
    inputContext->cursorColor[2] = cursorColor[2];
    inputContext->cursorColor[3] = cursorColor[3];

    inputContext->app_context = appContext;
    inputContext->layout = layout;

    // This will set all the flags to false initially
    inputContext->flags = (uint32_t)(0);

    inputContext->line_spacing = TTF_GetFontLineSkip(font);
    inputContext->font = font;

    DYNAMIC_ARRAY_INIT(inputContext->buffer);

    TextLine firstLine;
    text_line_init(&firstLine, inputContext);
    DYNAMIC_ARRAY_APPEND(inputContext->buffer, firstLine);

    return 0;
}

int InputContextDestroy(InputContext* inputContext) {
    for (int i = 0; i < inputContext->buffer.len; i++) {
        text_render_info_array_free(&inputContext->buffer.arr[i].renderInfo);
        DYNAMIC_ARRAY_FREE(inputContext->buffer.arr[i]);
    }
    DYNAMIC_ARRAY_FREE(inputContext->buffer);
    return 0;
}

static inline int InputContextBufferInsert(InputContext* inputContext, char* text, size_t textLen) {
    DYNAMIC_ARRAY_INSERT_ARRAY(inputContext->buffer.arr[inputContext->cursorLine], ((TextLine){.arr = text, .len = textLen, .capacity = 0}), inputContext->cursorIndex);
    inputContext->cursorIndex += textLen;
    return 0;
}

int InputContextProcessEvent(InputContext* inputContext, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_TEXT_INPUT: {
        InputContextBufferInsert(inputContext, (char*)event->text.text, SDL_strlen(event->text.text));
        inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;
        inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
    } break;

    case SDL_EVENT_KEY_DOWN: {
        const bool* key_states = SDL_GetKeyboardState(NULL);

        switch (event->key.key) {
        case SDLK_BACKSPACE: {
            if (inputContext->cursorIndex == 0 && inputContext->cursorLine > 0) {
                inputContext->buffer.arr[inputContext->cursorLine - 1].lineWasUpdated = true;
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine - 1].len;

                if (inputContext->buffer.arr[inputContext->cursorLine].len > 0) {
                    DYNAMIC_ARRAY_INSERT_ARRAY(
                        inputContext->buffer.arr[inputContext->cursorLine - 1],
                        inputContext->buffer.arr[inputContext->cursorLine],
                        inputContext->buffer.arr[inputContext->cursorLine - 1].len);
                }

                text_line_destroy(&inputContext->buffer.arr[inputContext->cursorLine]);
                DYNAMIC_ARRAY_REMOVE(inputContext->buffer, inputContext->cursorLine);

                inputContext->cursorLine--;
                if (inputContext->cursorLine < 0) {
                    inputContext->cursorLine = 0;
                }

                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            } else if (inputContext->cursorIndex > 0) {
                const char* indexStr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
                const char* indexStrCopy = indexStr;
                Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
                Uint32 code_width = (Uint32)(indexStrCopy - indexStr);

                DYNAMIC_ARRAY_REMOVE_SELECTION(inputContext->buffer.arr[inputContext->cursorLine], inputContext->cursorIndex - code_width, code_width);
                inputContext->cursorIndex -= code_width;
                if (inputContext->cursorIndex < 0) {
                    inputContext->cursorIndex = 0;
                }

                inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;
                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            }
        } break;

        case SDLK_RETURN: {
            TextLine newLine;
            text_line_init(&newLine, inputContext);
            if (inputContext->cursorIndex < inputContext->buffer.arr[inputContext->cursorLine].len) {
                DYNAMIC_ARRAY_INSERT_ARRAY(
                    newLine,
                    ((TextLine){
                        .arr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex],
                        .len = inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex,
                        .capacity = 0,
                    }),
                    0);
                DYNAMIC_ARRAY_REMOVE_SELECTION(
                    inputContext->buffer.arr[inputContext->cursorLine],
                    inputContext->cursorIndex,
                    inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex);
            }

            DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

            inputContext->cursorLine++;
            inputContext->cursorIndex = 0;
            inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;
            inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
        } break;

        case SDLK_LEFT: {
            const char* indexStr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            const char* indexStrCopy = indexStr;
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
            inputContext->cursorIndex -= (Uint32)(indexStrCopy - indexStr);

            if ((Uint32)(indexStrCopy - indexStr) == 0) {
                inputContext->cursorLine--;
                if (inputContext->cursorLine < 0) {
                    inputContext->cursorLine = 0;
                    inputContext->cursorIndex = 0;
                } else {
                    inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
                }
            }
            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_RIGHT: {
            const char* start = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            size_t psLen = inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex;
            size_t psLenCopy = psLen;
            Uint32 code = SDL_StepUTF8(&start, &psLen);

            inputContext->cursorIndex += psLenCopy - psLen;
            if (psLenCopy - psLen == 0) {
                inputContext->cursorLine++;
                if (inputContext->cursorLine >= inputContext->buffer.len) {
                    inputContext->cursorLine = inputContext->buffer.len - 1;
                    inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
                } else {
                    inputContext->cursorIndex = 0;
                }
            }
            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_UP: {
            inputContext->cursorLine--;
            if (inputContext->cursorLine < 0) {
                inputContext->cursorIndex = 0;
                inputContext->cursorLine = 0;
            } else {
                int prevLineCodePoints = get_num_codepoints(inputContext->buffer.arr[inputContext->cursorLine + 1].arr, inputContext->cursorIndex);
                int currLineCodePoints = get_num_codepoints(inputContext->buffer.arr[inputContext->cursorLine].arr, inputContext->buffer.arr[inputContext->cursorLine].len);
                if (prevLineCodePoints >= currLineCodePoints) {
                    inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
                } else {
                    inputContext->cursorIndex = get_byte_offset_from_codepoints(
                        inputContext->buffer.arr[inputContext->cursorLine].arr,
                        inputContext->buffer.arr[inputContext->cursorLine].len,
                        prevLineCodePoints);
                }
            }

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_DOWN: {
            inputContext->cursorLine++;
            if (inputContext->cursorLine >= inputContext->buffer.len) {
                inputContext->cursorLine = inputContext->buffer.len - 1;
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
            } else if (inputContext->cursorLine > 0) {
                int prevLineCodePoints = get_num_codepoints(inputContext->buffer.arr[inputContext->cursorLine - 1].arr, inputContext->cursorIndex);
                int currLineCodePoints = get_num_codepoints(inputContext->buffer.arr[inputContext->cursorLine].arr, inputContext->buffer.arr[inputContext->cursorLine].len);
                if (prevLineCodePoints >= currLineCodePoints) {
                    inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
                } else {
                    inputContext->cursorIndex = get_byte_offset_from_codepoints(
                        inputContext->buffer.arr[inputContext->cursorLine].arr,
                        inputContext->buffer.arr[inputContext->cursorLine].len,
                        prevLineCodePoints);
                }
            }

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        // Process ctrl + v for pasting
        case SDLK_V: {
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                char* clipboardText = SDL_GetClipboardText();
                const char* clipboardCopy = clipboardText;

                size_t clipboardTextLen = SDL_strlen(clipboardText);
                size_t psLen = clipboardTextLen;
                size_t psLenCopy = psLen;
                Uint32 code = SDL_StepUTF8(&clipboardCopy, &psLen);

                int startByteOffset = 0;
                int currByteOffset = 0;
                while (code != SDL_INVALID_UNICODE_CODEPOINT) {
                    int code_width = psLenCopy - psLen;

                    if (code == '\n') {
                        InputContextBufferInsert(inputContext, &clipboardText[startByteOffset], currByteOffset - startByteOffset);
                        inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;

                        TextLine newLine;
                        text_line_init(&newLine, inputContext);
                        if (inputContext->cursorIndex < inputContext->buffer.arr[inputContext->cursorLine].len) {
                            DYNAMIC_ARRAY_INSERT_ARRAY(
                                newLine,
                                ((TextLine){
                                    .arr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex],
                                    .len = inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex,
                                    .capacity = 0,
                                }),
                                0);
                            DYNAMIC_ARRAY_REMOVE_SELECTION(
                                inputContext->buffer.arr[inputContext->cursorLine],
                                inputContext->cursorIndex,
                                inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex);
                        }

                        DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

                        inputContext->cursorLine++;
                        inputContext->cursorIndex = 0;
                        inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;

                        startByteOffset = currByteOffset + code_width;
                    } else if (psLen == 0) {
                        InputContextBufferInsert(inputContext, &clipboardText[startByteOffset], clipboardTextLen - startByteOffset);
                        inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;
                        break;
                    }

                    currByteOffset += code_width;
                    psLenCopy = psLen;
                    code = SDL_StepUTF8(&clipboardCopy, &psLen);
                }

                SDL_free(clipboardText);
                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            }
        }
        }
    } break;
    }

    return 0;
}

int InputContextRefresh(InputContext* inputContext) {
    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED) {
        bool fontWasChanged = inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED;
        if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
            // handle this later
        } else {
            for (int i = 0; i < inputContext->buffer.len; i++) {
                // If the font was changed, every line needs to be updated to the new font (same for when the base color changes)
                if (inputContext->buffer.arr[i].lineWasUpdated = true || fontWasChanged == true) {
                    TextRenderInfo* currInfo = &inputContext->buffer.arr[i].renderInfo.arr[0];

                    if (fontWasChanged == true) {
                        TTF_SetTextFont(currInfo->text, inputContext->font);
                    }

                    // Internally, the strings strings are not null terminated for the lines, which means passing
                    // a length of 0 to TTF_SetTextString will cause invalid reads since TTF_SetTextString assumes null
                    // terminated strings when the length parameter is 0
                    if (inputContext->buffer.arr[i].len == 0) {
                        TTF_SetTextString(currInfo->text, "", 0);
                    } else {
                        TTF_SetTextString(currInfo->text, inputContext->buffer.arr[i].arr, inputContext->buffer.arr[i].len);
                    }
                    TTF_GetTextSize(currInfo->text, &currInfo->textWidth, &currInfo->textHeight);
                    InputContextGenerateGPUBuffersForLine(inputContext->app_context, currInfo);

                    inputContext->buffer.arr[i].lineWasUpdated = false;
                }
            }
        }
    }

    // Update cursor
    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_CURSOR_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED) {
        inputContext->cursorY = inputContext->cursorLine * inputContext->line_spacing;
        int tempInt;
        // If statement checks for 0 because the line strings are not null terminated and most of the TTF functions treat
        // a length of 0 as meaning the string is null terminated
        if (inputContext->cursorIndex == 0) {
            inputContext->cursorX = 0;
        } else {
            TTF_GetStringSize(inputContext->font, inputContext->buffer.arr[inputContext->cursorLine].arr, inputContext->cursorIndex, &inputContext->cursorX, &tempInt);
        }
    }

    // This will reset all the flags to false
    inputContext->flags = (uint32_t)(0);

    return 0;
}
