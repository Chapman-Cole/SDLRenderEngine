#include "InputContext.h"
#include "DynamicArray.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

static inline void text_render_info_init(TextRenderInfo* info, InputContext* inputContext) {
    info->text = TTF_CreateText(inputContext->app_context->textEngine, inputContext->font, "", 0);
    if (info->text == NULL) {
        SDL_Log("Failed to create text object for TextRenderInfo: %s", SDL_GetError());
        info->text = NULL;
    }
    TTF_SetTextColorFloat(info->text, inputContext->textColor[0], inputContext->textColor[1], inputContext->textColor[2], inputContext->textColor[3]);
    DYNAMIC_ARRAY_INIT(info->sequence);
}

static inline void text_render_info_free(TextRenderInfo* info) {
    TTF_DestroyText(info->text);
    DYNAMIC_ARRAY_FREE(info->sequence);
}

typedef struct IntArray {
    int* arr;
    size_t len;
    size_t capacity;
} IntArray;

static inline int get_utf8_width(Uint32 code) {
    if (code <= 127) {
        return 1;
    } else if (code <= 2047) {
        return 2;
    } else if (code <= 65535) {
        return 3;
    } else {
        return 4;
    }
}

static inline void text_line_init(TextLine* textLine, InputContext* inputContext) {
    DYNAMIC_ARRAY_INIT((*textLine));
    text_render_info_init(&textLine->renderInfo, inputContext);
    textLine->lineWasUpdated = false;
}

static inline void text_line_destroy(TextLine* textLine) {
    DYNAMIC_ARRAY_FREE((*textLine));
    text_render_info_free(&textLine->renderInfo);
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
    DYNAMIC_ARRAY_INIT(firstLine);
    // Must initialize after all basic variables in inputContext are set
    text_render_info_init(&firstLine.renderInfo, inputContext);
    DYNAMIC_ARRAY_APPEND(inputContext->buffer, firstLine);

    return 0;
}

int InputContextDestroy(InputContext* inputContext) {
    for (int i = 0; i < inputContext->buffer.len; i++) {
        DYNAMIC_ARRAY_FREE(inputContext->buffer.arr[i]);
        text_render_info_free(&inputContext->buffer.arr[i].renderInfo);
    }
    DYNAMIC_ARRAY_FREE(inputContext->buffer);
    return 0;
}

static inline int InputContextBufferInsert(InputContext* inputContext, char* text, size_t textLen) {
    DYNAMIC_ARRAY_INSERT_ARRAY(inputContext->buffer.arr[inputContext->cursorLine], ((TextLine){.arr = text, .len = textLen, .capacity = 0}), inputContext->cursorIndex);
    inputContext->cursorIndex += textLen;
    return 0;
}

// Generates the GPUText object while accounting for text wrapping
/*
static inline int InputContextGenTextWrap(InputContext* inputContext) {
    IntArray indices;
    DYNAMIC_ARRAY_INIT(indices);

    CharArray tempBuffer;
    DYNAMIC_ARRAY_INIT(tempBuffer);

    int tracker = 0;
    int prevStartIndex = 0;
    char* bufferAdvancedPtr = inputContext->buffer;
    while (tracker < inputContext->bufferLen) {
        Uint32 code = SDL_StepUTF8((const char**)&bufferAdvancedPtr, NULL);
        int code_width = get_utf8_width(code);

        if (code == ' ') {
            DYNAMIC_ARRAY_APPEND(indices, tracker);
        } else if (code == '\n') {
            // This solves the very specific case of when adding one more character
            // would push that character onto the next line, and that character happens to
            // be the newline character (pressing enter). Essentially, it needs to be handled the same
            // way as the space, which is looking at the string not including the space or in this case newline
            // (the loop will initially look at the string including the newline, and when tracker is updated
            // according to the indices array, it will get the proper length not including the newline character)
            DYNAMIC_ARRAY_APPEND(indices, tracker);
        }

        if (code == '\n' || tracker + code_width >= inputContext->bufferLen) {
            bool loopRan = false;
            int textSegmentLength = (tracker + code_width) - prevStartIndex;

            int textWidth, textHeight;
            // TTF_GetStringSize is very bizzare in that the height will only ever max out at the height of one line,
            // while the width will continuously grow. This means it works fine for single lines, but it has other quirks
            // like newline characters adding to width, which luckily had an easy solution (see the explanation above in else if (code == '\n'))
            // I have absolutely no idea why it works this way when TTF_GetTextSize works as expected, but it is somewhat annoying
            TTF_GetStringSize(inputContext->font, inputContext->buffer + prevStartIndex, textSegmentLength, &textWidth, &textHeight);
            while (indices.len > 0 && (float)textWidth > inputContext->layout.width) {
                loopRan = true;
                tracker = indices.arr[indices.len - 1];
                textSegmentLength = tracker - prevStartIndex;
                TTF_GetStringSize(inputContext->font, inputContext->buffer + prevStartIndex, textSegmentLength, &textWidth, &textHeight);
                DYNAMIC_ARRAY_POP(indices);
            }

            DYNAMIC_ARRAY_INSERT_ARRAY(tempBuffer, ((CharArray){.arr = inputContext->buffer + prevStartIndex, .len = textSegmentLength, .capacity = 0}), tempBuffer.len);

            if (loopRan == true) {
                // +1 to account for the tracker pointing to the space before the starting character
                bufferAdvancedPtr = &inputContext->buffer[tracker + 1];

                // no offset is needed for the cursorIndex even though a newline is being appended
                // because a space is effectively being traded for a newline (this means
                // the cursor index will also work in the newly created buffer)
                DYNAMIC_ARRAY_INSERT(tempBuffer, tempBuffer.len, '\n');

                // +1 to account for the fact that the tracker points to a space instead of the
                // start of the line
                prevStartIndex = tracker + 1;
            } else {
                // + code_width so that the current character (almost always a newline character) is skipped over since
                // it's already included when copying the line into the temp buffer
                prevStartIndex = tracker + code_width;
            }

            DYNAMIC_ARRAY_FREE(indices);
        }

        tracker += code_width;
    }

    // If inputContext buffer is empty, then the loop will never, so must handle this special case
    if (tempBuffer.arr == NULL) {
        GPUTextUpdate(&inputContext->gpuText, "", 0);
    } else {
        GPUTextUpdate(&inputContext->gpuText, tempBuffer.arr, tempBuffer.len);
    }

    DYNAMIC_ARRAY_FREE(indices);
    DYNAMIC_ARRAY_FREE(tempBuffer);
    return 0;
}
    */

static inline int get_num_codepoints(char* string, int len_bytes) {
    const char* string_copy = (const char*)string;
    int tracker = 0;
    int numCodePoints = 0;
    while (tracker < len_bytes) {
        Uint32 code = SDL_StepUTF8(&string_copy, NULL);
        int code_width = get_utf8_width(code);
        tracker += code_width;
        numCodePoints++;
    }
    return numCodePoints;
}

static inline int get_byte_offset_from_codepoints(char* string, int num_codepoints) {
    const char* string_copy = (const char*)string;
    int numBytes = 0;
    int codePointCount = 0;
    while (codePointCount < num_codepoints) {
        Uint32 code = SDL_StepUTF8(&string_copy, NULL);
        int code_width = get_utf8_width(code);
        numBytes += code_width;
        codePointCount++;
    }
    return numBytes;
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
                Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
                Uint32 code_width = get_utf8_width(code);

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
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
            inputContext->cursorIndex -= get_utf8_width(code);
            if (inputContext->cursorIndex < 0) {
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
            Uint32 code = SDL_StepUTF8(&start, NULL);
            inputContext->cursorIndex += get_utf8_width(code);
            if (inputContext->cursorIndex > inputContext->buffer.arr[inputContext->cursorLine].len) {
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
                    inputContext->cursorIndex = get_byte_offset_from_codepoints(inputContext->buffer.arr[inputContext->cursorLine].arr, prevLineCodePoints);
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
                    inputContext->cursorIndex = get_byte_offset_from_codepoints(inputContext->buffer.arr[inputContext->cursorLine].arr, prevLineCodePoints);
                }
            }

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        // Process ctrl + v for pasting
        case SDLK_V: {
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                char* clipboardText = SDL_GetClipboardText();

                const char* clipboardCopy = clipboardText;
                Uint32 code = SDL_StepUTF8(&clipboardCopy, NULL);
                int startByteOffset = 0;
                int currByteOffset = 0;
                while (code != SDL_INVALID_UNICODE_CODEPOINT) {
                    int code_width = get_utf8_width(code);

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
                    } else if (code == '\0') {
                        InputContextBufferInsert(inputContext, &clipboardText[startByteOffset], currByteOffset - startByteOffset);
                        inputContext->buffer.arr[inputContext->cursorLine].lineWasUpdated = true;
                        break;
                    }

                    currByteOffset += code_width;
                    code = SDL_StepUTF8(&clipboardCopy, NULL);
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

int InputContextRefresh(InputContext* inputContext) {
    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED) {
        bool fontWasChanged = inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED;
        for (int i = 0; i < inputContext->buffer.len; i++) {
            // If the font was changed, every line needs to be updated to the new font (same for when the base color changes)
            if (inputContext->buffer.arr[i].lineWasUpdated = true || fontWasChanged == true) {
                TextRenderInfo* currInfo = &inputContext->buffer.arr[i].renderInfo;

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
