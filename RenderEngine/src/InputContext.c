#include "InputContext.h"
#include "DynamicArray.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define RGBA_TO_F(r, g, b, a) ((float[4]){(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f})

// expects capital letters
#define CHAR_TO_HEX_NUMBER(c) (((char)(c) - 65) < 0 ? ((char)(c) - 48) : (10 + (char)(c) - 65))

// string must be at least 6 characters long (alpha should be a float between 0 and 255)
// expects capital letters
#define HEX_COLOR_TO_F(hexStr, alpha)                                       \
    RGBA_TO_F(                                                              \
        CHAR_TO_HEX_NUMBER(hexStr[1]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[0]), \
        CHAR_TO_HEX_NUMBER(hexStr[3]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[2]), \
        CHAR_TO_HEX_NUMBER(hexStr[5]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[4]), \
        alpha)

static inline void set_float4_with_hex_color(float array[4], char* hexStr, char alpha) {
    array[0] = (CHAR_TO_HEX_NUMBER(hexStr[1]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[0])) / 255.0f;
    array[1] = (CHAR_TO_HEX_NUMBER(hexStr[3]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[2])) / 255.0f;
    array[2] = (CHAR_TO_HEX_NUMBER(hexStr[5]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[4])) / 255.0f;
    array[3] = (float)(alpha) / 255.0f;
}

static inline void text_render_info_init(TextRenderInfo* info, InputContext* inputContext) {
    DYNAMIC_ARRAY_INIT(info->sequence);
    info->text = TTF_CreateText(inputContext->app_context->textEngine, inputContext->font, "", 0);
    if (info->text == NULL) {
        SDL_Log("Failed to create text object for TextRenderInfo: %s", SDL_GetError());
    }
    info->textWidth = 0;
    info->textHeight = 0;
}

static inline void text_render_info_free(TextRenderInfo* info) {
    TTF_DestroyText(info->text);
    for (size_t i = 0; i < info->sequence.len; i++) {
        GPUBuffer_unref(info->sequence.arr[i].vertexBuffer);
        GPUBuffer_unref(info->sequence.arr[i].indexBuffer);
    }
    DYNAMIC_ARRAY_FREE(info->sequence);
}

static inline void text_render_info_array_init(TextRenderInfoArray* infoArray, InputContext* inputContext) {
    TextRenderInfo info;
    text_render_info_init(&info, inputContext);
    DYNAMIC_ARRAY_INIT((*infoArray));
    DYNAMIC_ARRAY_APPEND((*infoArray), info);
}

static inline void text_render_info_array_free(TextRenderInfoArray* infoArray) {
    for (int i = 0; i < infoArray->len; i++) {
        text_render_info_free(&infoArray->arr[i]);
    }
    DYNAMIC_ARRAY_FREE((*infoArray));
}

static inline void text_line_init(TextLine* textLine, InputContext* inputContext) {
    DYNAMIC_ARRAY_INIT((*textLine));
    text_render_info_array_init(&textLine->renderInfo, inputContext);
    textLine->wrapState = TEXT_CURSOR_WRAP_STATE_UP;
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
static inline StringSliceArray get_line_text_wrap_slice_array(InputContext* inputContext, int lineIndex) {
    StringSliceArray sliceArray;
    DYNAMIC_ARRAY_INIT(sliceArray);

    StringSlice totalSlice = (StringSlice){.string = inputContext->buffer.arr[lineIndex].arr, .len = inputContext->buffer.arr[lineIndex].len};

    int layoutWidth = inputContext->layout.width;

    int lineWidth, lineHeight;
    if (totalSlice.len > 0) {
        TTF_GetStringSize(inputContext->font, totalSlice.string, totalSlice.len, &lineWidth, &lineHeight);
    } else {
        lineWidth = 0;
    }

    if (lineWidth <= layoutWidth) {
        DYNAMIC_ARRAY_APPEND(sliceArray, totalSlice);
        return sliceArray;
    }

    char* line_copy = totalSlice.string;
    size_t psLen = totalSlice.len;

    int currByteOffset = 0;
    int prevWordOffset = 0;
    int prevSliceWidth = 0;
    while (psLen > 0) {
        size_t psLenCopy = psLen;
        Uint32 code = SDL_StepUTF8((const char**)&line_copy, &psLen);
        int code_width = psLenCopy - psLen;

        // Need to check for psLen == 0 first because the last character could be a space,
        // using else if would mean psLen == 0 would never be reached in that case
        if (psLen == 0) {
            int nextWordStart = currByteOffset + code_width;

            int testWidth, testHeight;
            TTF_GetStringSize(
                inputContext->font, totalSlice.string,
                nextWordStart, &testWidth, &testHeight);

            if (testWidth > layoutWidth) {
                if (prevSliceWidth == 0) {
                    // In the case when there are no spaces to use to split up the string,
                    // the string will just have to be split up by finding the character cutoff
                    // for where it goes out of bounds

                    int substringWidth, substringHeight;
                    TTF_GetStringSize(inputContext->font, totalSlice.string, totalSlice.len, &substringWidth, &substringHeight);
                    int prevSubstringWidth = substringWidth;

                    int estimatedNumCodepoints = substringWidth / TTF_GetFontHeight(inputContext->font);
                    int byteOffsetTracker = get_byte_offset_from_codepoints(totalSlice.string, totalSlice.len, estimatedNumCodepoints);
                    int prevByteOffsetTracker = byteOffsetTracker;
                    const char* substringTracker = &totalSlice.string[byteOffsetTracker];

                    for (;;) {
                        prevSubstringWidth = substringWidth;
                        prevByteOffsetTracker = byteOffsetTracker;
                        if (substringWidth < layoutWidth) {
                            size_t psLen = totalSlice.len - byteOffsetTracker;
                            size_t psLenCopy = psLen;
                            const char* stringCopy = totalSlice.string;
                            Uint32 code = SDL_StepUTF8(&stringCopy, &psLen);
                            Uint32 code_width = (Uint32)(psLenCopy - psLen);
                            byteOffsetTracker += code_width;
                            TTF_GetStringSize(inputContext->font, totalSlice.string, byteOffsetTracker, &substringWidth, &substringHeight);
                        } else if (substringWidth > layoutWidth) {
                            const char* substringTrackerCopy = substringTracker;
                            Uint32 code = SDL_StepBackUTF8((const char*)totalSlice.string, &substringTracker);
                            Uint32 code_width = (Uint32)(substringTrackerCopy - substringTracker);
                            byteOffsetTracker -= code_width;
                            TTF_GetStringSize(inputContext->font, totalSlice.string, byteOffsetTracker, &substringWidth, &substringHeight);
                        } else {
                            break;
                        }

                        if (prevSubstringWidth <= layoutWidth && substringWidth > layoutWidth) {
                            byteOffsetTracker = prevByteOffsetTracker;
                            break;
                        }
                    }

                    StringSlice currSlice = (StringSlice){.string = totalSlice.string, .len = byteOffsetTracker};
                    DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
                    totalSlice = (StringSlice){
                        .string = &totalSlice.string[byteOffsetTracker],
                        .len = totalSlice.len - byteOffsetTracker,
                    };

                    int totalSliceWidth, totalSliceHeight;
                    if (totalSlice.len > 0) {
                        // Since this is the end of the string, have to check to make the the second half
                        // of the split string actually fits in the bounds
                        TTF_GetStringSize(inputContext->font, totalSlice.string, totalSlice.len, &totalSliceWidth, &totalSliceHeight);
                        if (totalSliceWidth > layoutWidth) {
                            prevWordOffset = 0;
                            prevSliceWidth = 0;
                            currByteOffset = 0;
                            psLen = totalSlice.len;
                            line_copy = totalSlice.string;
                            continue;
                        } else {
                            DYNAMIC_ARRAY_APPEND(sliceArray, totalSlice);
                        }
                    }
                } else if (prevSliceWidth <= layoutWidth) {
                    StringSlice currSlice = (StringSlice){.string = totalSlice.string, .len = prevWordOffset};
                    DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
                    currSlice = (StringSlice){.string = &totalSlice.string[prevWordOffset], .len = totalSlice.len - prevWordOffset};

                    // Since this is the end of the string, must check to make sure the second half
                    // of the split string actually fits in the bounds
                    int currSliceWidth, currSliceHeight;
                    if (currSlice.len > 0) {
                        TTF_GetStringSize(inputContext->font, currSlice.string, currSlice.len, &currSliceWidth, &currSliceHeight);
                        if (currSliceWidth > layoutWidth) {
                            totalSlice = currSlice;
                            prevWordOffset = 0;
                            prevSliceWidth = 0;
                            currByteOffset = 0;
                            psLen = totalSlice.len;
                            line_copy = totalSlice.string;
                            continue;
                        } else {
                            DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
                        }
                    }
                }
            } else {
                StringSlice currSlice = (StringSlice){.string = totalSlice.string, .len = totalSlice.len};
                DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
            }
            break;
        }

        if (code == ' ') {
            int nextWordStart = currByteOffset + code_width;

            int testWidth, testHeight;
            TTF_GetStringSize(
                inputContext->font, totalSlice.string,
                nextWordStart, &testWidth, &testHeight);

            if (testWidth > layoutWidth) {
                if (prevSliceWidth == 0) {
                    // In the case when there are no spaces to use to split up the string,
                    // the string will just have to be split up by finding the character cutoff
                    // for where it goes out of bounds

                    int substringWidth, substringHeight;
                    TTF_GetStringSize(inputContext->font, totalSlice.string, totalSlice.len, &substringWidth, &substringHeight);
                    int prevSubstringWidth = substringWidth;

                    int estimatedNumCodepoints = substringWidth / TTF_GetFontHeight(inputContext->font);
                    int byteOffsetTracker = get_byte_offset_from_codepoints(totalSlice.string, totalSlice.len, estimatedNumCodepoints);
                    int prevByteOffsetTracker = byteOffsetTracker;
                    const char* substringTracker = &totalSlice.string[byteOffsetTracker];
                    for (;;) {
                        prevSubstringWidth = substringWidth;
                        prevByteOffsetTracker = byteOffsetTracker;
                        if (substringWidth < layoutWidth) {
                            size_t psLen = totalSlice.len - byteOffsetTracker;
                            size_t psLenCopy = psLen;
                            const char* stringCopy = totalSlice.string;
                            Uint32 code = SDL_StepUTF8(&stringCopy, &psLen);
                            Uint32 code_width = (Uint32)(psLenCopy - psLen);
                            byteOffsetTracker += code_width;
                            TTF_GetStringSize(inputContext->font, totalSlice.string, byteOffsetTracker, &substringWidth, &substringHeight);
                        } else if (substringWidth > layoutWidth) {
                            const char* substringTrackerCopy = substringTracker;
                            Uint32 code = SDL_StepBackUTF8((const char*)totalSlice.string, &substringTracker);
                            Uint32 code_width = (Uint32)(substringTrackerCopy - substringTracker);
                            byteOffsetTracker -= code_width;
                            TTF_GetStringSize(inputContext->font, totalSlice.string, byteOffsetTracker, &substringWidth, &substringHeight);
                        } else {
                            break;
                        }

                        if (prevSubstringWidth <= layoutWidth && substringWidth > layoutWidth) {
                            byteOffsetTracker = prevByteOffsetTracker;
                            break;
                        }
                    }

                    StringSlice currSlice = (StringSlice){.string = totalSlice.string, .len = byteOffsetTracker};
                    DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
                    totalSlice = (StringSlice){
                        .string = &totalSlice.string[byteOffsetTracker],
                        .len = totalSlice.len - byteOffsetTracker,
                    };
                    prevWordOffset = 0;
                    prevSliceWidth = 0;
                    currByteOffset = 0;
                    psLen = totalSlice.len;
                    line_copy = totalSlice.string;
                    // Have to continue to prevent currByteOffset from be updated at the end of the loop
                    continue;
                } else if (prevSliceWidth <= layoutWidth) {
                    StringSlice currSlice = (StringSlice){.string = totalSlice.string, .len = prevWordOffset};
                    DYNAMIC_ARRAY_APPEND(sliceArray, currSlice);
                    totalSlice = (StringSlice){
                        .string = &totalSlice.string[prevWordOffset],
                        .len = totalSlice.len - prevWordOffset,
                    };
                    prevWordOffset = 0;
                    prevSliceWidth = 0;
                    currByteOffset = 0;
                    psLen = totalSlice.len;
                    line_copy = totalSlice.string;
                    // Have to continue to prevent currByteOffset from be updated at the end of the loop
                    continue;
                }
            } else {
                prevSliceWidth = testWidth;
                prevWordOffset = nextWordStart;
            }
        }

        currByteOffset += code_width;
    }

    return sliceArray;
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

    set_float4_with_hex_color(inputContext->highlightColor, "90D5FF", (char)(0.4f * 255.0f));
    inputContext->mouseLeftDown = false;

    inputContext->highlightStartIndex = 0;
    inputContext->highlightStartLineIndex = 0;

    inputContext->cursorIndex = 0;
    inputContext->cursorLine = 0;

    inputContext->scrollHorizontalOffset = 0;
    inputContext->scrollVerticalOffset = 0;

    inputContext->lineRenderIndexStart = 0;
    inputContext->lineRenderIndexEnd = 0;

    inputContext->tabWidth = 5;

    inputContext->app_context = appContext;
    inputContext->layout = layout;

    // This will set all the flags to false initially
    inputContext->flags = (uint32_t)(0);

    inputContext->line_spacing = TTF_GetFontLineSkip(font);
    inputContext->font = font;

    DYNAMIC_ARRAY_INIT(inputContext->modifiedLines);

    DYNAMIC_ARRAY_INIT(inputContext->buffer);

    TextLine firstLine;
    text_line_init(&firstLine, inputContext);
    DYNAMIC_ARRAY_APPEND(inputContext->buffer, firstLine);

    return 0;
}

int InputContextDestroy(InputContext* inputContext) {
    for (int i = 0; i < inputContext->buffer.len; i++) {
        text_line_destroy(&inputContext->buffer.arr[i]);
    }
    DYNAMIC_ARRAY_FREE(inputContext->buffer);
    DYNAMIC_ARRAY_FREE(inputContext->modifiedLines);
    return 0;
}

int InputContextClear(InputContext* inputContext) {
    for (int i = 0; i < inputContext->buffer.len; i++) {
        text_line_destroy(&inputContext->buffer.arr[i]);
    }
    DYNAMIC_ARRAY_FREE(inputContext->buffer);
    DYNAMIC_ARRAY_FREE(inputContext->modifiedLines);

    DYNAMIC_ARRAY_INIT(inputContext->buffer);
    DYNAMIC_ARRAY_INIT(inputContext->modifiedLines);

    DYNAMIC_ARRAY_INIT(inputContext->modifiedLines);

    TextLine firstLine;
    text_line_init(&firstLine, inputContext);
    DYNAMIC_ARRAY_APPEND(inputContext->buffer, firstLine);
    inputContext->cursorLine = 0;
    inputContext->cursorIndex = 0;

    inputContext->cursorX = 0;
    inputContext->cursorY = 0;

    inputContext->scrollHorizontalOffset = 0;
    inputContext->scrollVerticalOffset = 0;

    inputContext->lineRenderIndexStart = 0;
    inputContext->lineRenderIndexEnd = 0;

    inputContext->highlightStartIndex = 0;
    inputContext->highlightStartLineIndex = 0;

    inputContext->flags = (uint32_t)(0);

    return 0;
}

static inline int InputContextModifiedLinesAppend(InputContext* inputContext, int index) {
    // Don't want to double count indices
    for (int i = 0; i < inputContext->modifiedLines.len; i++) {
        if (inputContext->modifiedLines.arr[i] == index) {
            return 0;
        }
    }
    DYNAMIC_ARRAY_APPEND(inputContext->modifiedLines, index);
    return 0;
}

static inline int InputContextBufferInsert(InputContext* inputContext, char* text, size_t textLen) {
    DYNAMIC_ARRAY_INSERT_ARRAY(inputContext->buffer.arr[inputContext->cursorLine], ((TextLine){.arr = text, .len = textLen, .capacity = 0}), inputContext->cursorIndex);
    inputContext->cursorIndex += textLen;
    return 0;
}

static inline void InputContextCalculateBufferPosFromMousePos(InputContext* inputContext, int* cursorLine, int* cursorIndex) {
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    int logicalWindowSizeX, logicalWindowSizeY;
    SDL_GetWindowSize(inputContext->app_context->window, &logicalWindowSizeX, &logicalWindowSizeY);

    int pixelWindowSizeX, pixelWindowSizeY;
    SDL_GetWindowSizeInPixels(inputContext->app_context->window, &pixelWindowSizeX, &pixelWindowSizeY);

    float mouseScaleX = (float)pixelWindowSizeX / (float)logicalWindowSizeX;
    float mouseScaleY = (float)pixelWindowSizeY / (float)logicalWindowSizeY;

    int translatedX = (int)(mouseX * mouseScaleX) - inputContext->layout.x + inputContext->scrollHorizontalOffset;
    int translatedY = (int)(mouseY * mouseScaleY) - inputContext->layout.y + inputContext->scrollVerticalOffset;

    if (translatedX < 0) {
        translatedX = 0;
    }

    if (translatedY < 0) {
        translatedY = 0;
    }

    int currLineIndex = (translatedY / inputContext->line_spacing);

    if (currLineIndex >= inputContext->buffer.len) {
        currLineIndex = inputContext->buffer.len - 1;
    }

    char* currString = inputContext->buffer.arr[currLineIndex].arr;
    int currStringLen = inputContext->buffer.arr[currLineIndex].len;

    int totalWidth, totalHeight;
    TTF_GetStringSize(inputContext->font, currString, currStringLen, &totalWidth, &totalHeight);
    if (translatedX > totalWidth) {
        *cursorLine = currLineIndex;
        *cursorIndex = currStringLen;
        inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        return;
    }

    int estimatedNumCodepoints = (int)SDL_lroundf((float)translatedX / (float)TTF_GetFontHeight(inputContext->font));
    int byteOffsetTracker = get_byte_offset_from_codepoints(currString, currStringLen, estimatedNumCodepoints);
    int prevByteOffsetTracker = byteOffsetTracker;
    const char* substringTracker = &currString[byteOffsetTracker];

    if (byteOffsetTracker == 0) {
        *cursorLine = currLineIndex;
        *cursorIndex = byteOffsetTracker;
        inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        return;
    }

    int lineWidth, lineHeight;
    TTF_GetStringSize(inputContext->font, currString, byteOffsetTracker, &lineWidth, &lineHeight);
    int prevLineWidth = lineWidth;

    for (;;) {
        prevLineWidth = lineWidth;
        prevByteOffsetTracker = byteOffsetTracker;
        if (lineWidth < translatedX) {
            size_t psLen = currStringLen - byteOffsetTracker;
            size_t psLenCopy = psLen;
            const char* stringCopy = currString;
            Uint32 code = SDL_StepUTF8(&stringCopy, &psLen);
            Uint32 code_width = (Uint32)(psLenCopy - psLen);
            byteOffsetTracker += code_width;
            TTF_GetStringSize(inputContext->font, currString, byteOffsetTracker, &lineWidth, &lineHeight);
        } else if (lineWidth > translatedX) {
            const char* substringTrackerCopy = substringTracker;
            Uint32 code = SDL_StepBackUTF8((const char*)currString, &substringTracker);
            Uint32 code_width = (Uint32)(substringTrackerCopy - substringTracker);
            byteOffsetTracker -= code_width;
            if (byteOffsetTracker <= 0) {
                break;
            }
            TTF_GetStringSize(inputContext->font, currString, byteOffsetTracker, &lineWidth, &lineHeight);
        } else {
            break;
        }

        if (prevLineWidth <= translatedX && lineWidth > translatedX) {
            byteOffsetTracker = prevByteOffsetTracker;
            break;
        }
    }

    *cursorLine = currLineIndex;
    *cursorIndex = byteOffsetTracker;
}

int InputContextProcessEvent(InputContext* inputContext, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_TEXT_INPUT: {
        InputContextBufferInsert(inputContext, (char*)event->text.text, SDL_strlen(event->text.text));
        InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
        inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
    } break;

    case SDL_EVENT_KEY_DOWN: {
        const bool* key_states = SDL_GetKeyboardState(NULL);

        switch (event->key.key) {
        case SDLK_BACKSPACE: {
            if (inputContext->cursorIndex == 0 && inputContext->cursorLine > 0) {
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine - 1].len;

                if (inputContext->buffer.arr[inputContext->cursorLine].len > 0) {
                    DYNAMIC_ARRAY_INSERT_ARRAY(
                        inputContext->buffer.arr[inputContext->cursorLine - 1],
                        inputContext->buffer.arr[inputContext->cursorLine],
                        inputContext->buffer.arr[inputContext->cursorLine - 1].len);
                }

                text_line_destroy(&inputContext->buffer.arr[inputContext->cursorLine]);
                DYNAMIC_ARRAY_REMOVE(inputContext->buffer, inputContext->cursorLine);

                // Make sure to update any indices that come after inputContext->cursorLine
                // to be one less since they all got shifted left in the array after the removal of the line
                for (int i = 0; i < inputContext->modifiedLines.len; i++) {
                    if (inputContext->modifiedLines.arr[i] > inputContext->cursorLine) {
                        if (inputContext->modifiedLines.arr[i] > 0) {
                            inputContext->modifiedLines.arr[i]--;
                        }
                    } else if (inputContext->modifiedLines.arr[i] == inputContext->cursorLine) {
                        DYNAMIC_ARRAY_REMOVE(inputContext->modifiedLines, i);
                        i--;
                    }
                }
                InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine - 1);

                inputContext->cursorLine--;
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

                InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
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

                InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
            }

            // Have to make sure to update all the indices that come after and at the insertion index
            for (int i = 0; i < inputContext->modifiedLines.len; i++) {
                if (inputContext->modifiedLines.arr[i] >= (inputContext->cursorLine + 1)) {
                    inputContext->modifiedLines.arr[i]++;
                }
            }
            DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

            inputContext->cursorLine++;
            inputContext->cursorIndex = 0;
            InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
            inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
        } break;

        case SDLK_LEFT: {
            const char* indexStr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            const char* indexStrCopy = indexStr;
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);

            if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
                if (
                    inputContext->cursorIndex == inputContext->buffer.arr[inputContext->cursorLine].criticalIndex &&
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState == TEXT_CURSOR_WRAP_STATE_DOWN) {
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState = TEXT_CURSOR_WRAP_STATE_UP;
                    inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
                    break;
                } else {
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState = TEXT_CURSOR_WRAP_STATE_DOWN;
                }
            }

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

            if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
                if (
                    inputContext->cursorIndex == inputContext->buffer.arr[inputContext->cursorLine].criticalIndex &&
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState == TEXT_CURSOR_WRAP_STATE_UP) {
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState = TEXT_CURSOR_WRAP_STATE_DOWN;
                    inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
                    break;
                } else {
                    inputContext->buffer.arr[inputContext->cursorLine].wrapState = TEXT_CURSOR_WRAP_STATE_UP;
                }
            }

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

        case SDLK_TAB: {
            for (int i = 0; i < inputContext->tabWidth; i++) {
                InputContextBufferInsert(inputContext, " ", 1);
            }
            InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
            inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
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
                        InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);

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

                        // Have to make sure to update all the indices that come after and at the insertion index
                        for (int i = 0; i < inputContext->modifiedLines.len; i++) {
                            if (inputContext->modifiedLines.arr[i] >= (inputContext->cursorLine + 1)) {
                                inputContext->modifiedLines.arr[i]++;
                            }
                        }
                        DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

                        inputContext->cursorLine++;
                        inputContext->cursorIndex = 0;
                        InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);

                        startByteOffset = currByteOffset + code_width;
                    } else if (psLen == 0) {
                        InputContextBufferInsert(inputContext, &clipboardText[startByteOffset], clipboardTextLen - startByteOffset);
                        InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
                        break;
                    }

                    currByteOffset += code_width;
                    psLenCopy = psLen;
                    code = SDL_StepUTF8(&clipboardCopy, &psLen);
                }

                SDL_free(clipboardText);
                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            }
        } break;

        case SDLK_C: {
            if (key_states[SDL_SCANCODE_LCTRL]) {
                struct {
                    char* arr;
                    size_t len;
                    size_t capacity;
                } charArray;
                DYNAMIC_ARRAY_INIT(charArray);

                int startLineIndex = 0;
                int endLineIndex = 0;
                int startIndex = 0;
                int endIndex = 0;
                if (inputContext->cursorLine >= inputContext->highlightStartLineIndex) {
                    endLineIndex = inputContext->cursorLine;
                    endIndex = inputContext->cursorIndex;
                    startLineIndex = inputContext->highlightStartLineIndex;
                    startIndex = inputContext->highlightStartIndex;
                } else {
                    endLineIndex = inputContext->highlightStartLineIndex;
                    endIndex = inputContext->highlightStartIndex;
                    startLineIndex = inputContext->cursorLine;
                    startIndex = inputContext->cursorIndex;
                }

                if (startLineIndex == endLineIndex) {
                    if (startIndex != endIndex) {
                        if (startIndex > endIndex) {
                            int temp = endIndex;
                            endIndex = startIndex;
                            startIndex = temp;
                        }

                        struct {
                            char* arr;
                            size_t len;
                            size_t capacity;
                        } tempArr = {
                            .arr = &inputContext->buffer.arr[startLineIndex].arr[startIndex],
                            .len = endIndex - startIndex,
                            .capacity = 0,
                        };

                        DYNAMIC_ARRAY_INSERT_ARRAY(charArray, tempArr, charArray.len);
                    }
                } else {
                    for (int i = startLineIndex; i <= endLineIndex; i++) {
                        struct {
                            char* arr;
                            size_t len;
                            size_t capacity;
                        } tempArr;

                        if (i == startLineIndex) {
                            tempArr.arr = &inputContext->buffer.arr[i].arr[startIndex];
                            tempArr.len = inputContext->buffer.arr[i].len - startIndex;
                            tempArr.capacity = -1;
                        } else if (i == endLineIndex) {
                            tempArr.arr = inputContext->buffer.arr[i].arr;
                            tempArr.len = endIndex;
                            tempArr.capacity = -1;
                        } else {
                            if (inputContext->buffer.arr[i].len > 0) {
                                tempArr.arr = inputContext->buffer.arr[i].arr;
                                tempArr.len = inputContext->buffer.arr[i].len;
                                tempArr.capacity = -1;
                            }
                        }

                        if (tempArr.len > 0) {
                            DYNAMIC_ARRAY_INSERT_ARRAY(charArray, tempArr, charArray.len);
                        }
                        if (i < endLineIndex) {
                            DYNAMIC_ARRAY_APPEND(charArray, '\n');
                        }
                    }
                }

                DYNAMIC_ARRAY_APPEND(charArray, '\0');

                SDL_SetClipboardText(charArray.arr);

                DYNAMIC_ARRAY_FREE(charArray);
            }
        } break;
        }
    } break;

    case SDL_EVENT_MOUSE_WHEEL: {
        if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_SCROLLABLE) {
            int screenWidth, screenHeight;
            SDL_GetWindowSizeInPixels(inputContext->app_context->window, &screenWidth, &screenHeight);
            float scrollFactor = (float)screenWidth / 8.0f;

            InputContextSetHorizontalScrollOffset(inputContext, inputContext->scrollHorizontalOffset + event->wheel.x * scrollFactor);
            InputContextSetVerticalScrollOffset(inputContext, inputContext->scrollVerticalOffset - event->wheel.y * scrollFactor);
        }
    } break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (event->button.button == SDL_BUTTON_LEFT) {
            inputContext->mouseLeftDown = true;
            InputContextCalculateBufferPosFromMousePos(inputContext, &inputContext->cursorLine, &inputContext->cursorIndex);

            inputContext->highlightStartIndex = inputContext->cursorIndex;
            inputContext->highlightStartLineIndex = inputContext->cursorLine;

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        }
    } break;

    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (event->button.button == SDL_BUTTON_LEFT) {
            inputContext->mouseLeftDown = false;
        }
    } break;

    case SDL_EVENT_MOUSE_MOTION: {
        if (inputContext->mouseLeftDown) {
            InputContextCalculateBufferPosFromMousePos(inputContext, &inputContext->cursorLine, &inputContext->cursorIndex);
            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        }
    } break;
    }

    return 0;
}

static inline void UpdateRenderIndexBounds(InputContext* inputContext) {
    int offsetY = 0;
    inputContext->lineRenderOffsetY = 0;
    inputContext->lineRenderIndexStart = 0;
    inputContext->lineRenderIndexEnd = 0;
    for (int i = 0; i < inputContext->buffer.len; i++) {
        inputContext->lineRenderIndexEnd = i + 1;
        if (offsetY - inputContext->scrollVerticalOffset < -inputContext->line_spacing) {
            inputContext->lineRenderIndexStart = i + 1;
            inputContext->lineRenderOffsetY += inputContext->line_spacing * inputContext->buffer.arr[i].renderInfo.len;
        } else if ((offsetY + inputContext->line_spacing) - inputContext->scrollVerticalOffset > inputContext->layout.height) {
            break;
        }
        offsetY += inputContext->buffer.arr[i].renderInfo.len * inputContext->line_spacing;
    }
}

static inline void InputContextUpdateLine(InputContext* inputContext, int index, bool fontWasChanged) {
    StringSliceArray lineSlices = (StringSliceArray){
        .arr = (StringSlice[]){{
            .string = inputContext->buffer.arr[index].arr,
            .len = inputContext->buffer.arr[index].len,
        }},
        .len = 1,
        .capacity = 0,
    };

    if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
        lineSlices = get_line_text_wrap_slice_array(inputContext, index);

        while (inputContext->buffer.arr[index].renderInfo.len != lineSlices.len) {
            if (inputContext->buffer.arr[index].renderInfo.len < lineSlices.len) {
                TextRenderInfo currRenderInfo;
                text_render_info_init(&currRenderInfo, inputContext);
                DYNAMIC_ARRAY_APPEND(inputContext->buffer.arr[index].renderInfo, currRenderInfo);
            } else if (inputContext->buffer.arr[index].renderInfo.len > lineSlices.len) {
                TextRenderInfoArray* infoArray = &inputContext->buffer.arr[index].renderInfo;
                text_render_info_free(&infoArray->arr[infoArray->len - 1]);
                DYNAMIC_ARRAY_POP((*infoArray));
            }
        }
    }

    for (int j = 0; j < inputContext->buffer.arr[index].renderInfo.len; j++) {
        TextRenderInfo* currInfo = &inputContext->buffer.arr[index].renderInfo.arr[j];

        if (fontWasChanged == true) {
            TTF_SetTextFont(currInfo->text, inputContext->font);
        }

        // Internally, the strings strings are not null terminated for the lines, which means passing
        // a length of 0 to TTF_SetTextString will cause invalid reads since TTF_SetTextString assumes null
        // terminated strings when the length parameter is 0
        if (lineSlices.arr[j].len == 0) {
            TTF_SetTextString(currInfo->text, "", 0);
        } else {
            TTF_SetTextString(currInfo->text, lineSlices.arr[j].string, lineSlices.arr[j].len);
        }
        TTF_GetTextSize(currInfo->text, &currInfo->textWidth, &currInfo->textHeight);
        InputContextGenerateGPUBuffersForLine(inputContext->app_context, currInfo);
    }

    if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
        DYNAMIC_ARRAY_FREE(lineSlices);
    }

    for (int j = 0; j < inputContext->buffer.arr[index].renderInfo.len; j++) {
        inputContext->maxHeight += inputContext->line_spacing;
        int textWidth, textHeight;
        TTF_GetStringSize(inputContext->font, inputContext->buffer.arr[index].renderInfo.arr[j].text->text, 0, &textWidth, &textHeight);
        if (textWidth > inputContext->maxLineWidth) {
            inputContext->maxLineWidth = textWidth;
        }
    }
}

int InputContextRefresh(InputContext* inputContext) {
    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED ||
        ((inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED) &&
         inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP)) {

        bool fontWasChanged = inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED;
        bool textWrapAffectedByLayout = ((inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED) &&
                                         inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP);

        if (fontWasChanged || textWrapAffectedByLayout) {
            inputContext->maxLineWidth = 0;
            inputContext->maxHeight = 0;

            for (int i = 0; i < inputContext->buffer.len; i++) {
                InputContextUpdateLine(inputContext, i, fontWasChanged);
            }
        } else {
            for (int i = 0; i < inputContext->modifiedLines.len; i++) {
                InputContextUpdateLine(inputContext, inputContext->modifiedLines.arr[i], false);
            }
        }

        inputContext->scrollMaxHorizontalOffset = inputContext->maxLineWidth - inputContext->layout.width;
        if (inputContext->scrollMaxHorizontalOffset < 0) {
            inputContext->scrollMaxHorizontalOffset = 0;
        }

        inputContext->scrollMaxVerticalOffset = inputContext->maxHeight - inputContext->layout.height;
        if (inputContext->scrollMaxVerticalOffset < 0) {
            inputContext->scrollMaxVerticalOffset = 0;
        }
    }

    // Update cursor
    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_CURSOR_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_FONT_WAS_CHANGED ||
        ((inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED) &&
         inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP)) {
        int tempInt;

        // If statement checks for 0 because the line strings are not null terminated and most of the TTF functions treat
        // a length of 0 as meaning the string is null terminated
        if (inputContext->cursorIndex == 0) {
            inputContext->cursorX = 0;
            inputContext->cursorY = 0;
            for (int i = 0; i < inputContext->cursorLine; i++) {
                inputContext->cursorY += inputContext->buffer.arr[i].renderInfo.len * inputContext->line_spacing;
            }
        } else {
            if (inputContext->layoutMode == INPUT_CONTEXT_LAYOUT_MODE_WRAP) {
                StringSliceArray lineSlices = get_line_text_wrap_slice_array(inputContext, inputContext->cursorLine);

                inputContext->cursorY = 0;
                for (int i = 0; i < inputContext->cursorLine; i++) {
                    inputContext->cursorY += inputContext->buffer.arr[i].renderInfo.len * inputContext->line_spacing;
                }

                int indexTracker = inputContext->cursorIndex;
                int runningOffset = 0;
                for (size_t i = 0; i < lineSlices.len; i++) {
                    if (indexTracker <= lineSlices.arr[i].len) {
                        if (i < lineSlices.len - 1) {
                            inputContext->buffer.arr[inputContext->cursorLine].criticalIndex = runningOffset + lineSlices.arr[i].len;
                        } else {
                            inputContext->buffer.arr[inputContext->cursorLine].criticalIndex = -1;
                        }

                        if (inputContext->cursorIndex == inputContext->buffer.arr[inputContext->cursorLine].criticalIndex) {
                            switch (inputContext->buffer.arr[inputContext->cursorLine].wrapState) {
                            case TEXT_CURSOR_WRAP_STATE_UP: {
                                TTF_GetStringSize(inputContext->font, lineSlices.arr[i].string, lineSlices.arr[i].len, &inputContext->cursorX, &tempInt);
                            } break;

                            case TEXT_CURSOR_WRAP_STATE_DOWN: {
                                inputContext->cursorX = 0;
                                inputContext->cursorY += inputContext->line_spacing;
                            } break;
                            };
                        } else {
                            TTF_GetStringSize(inputContext->font, lineSlices.arr[i].string, indexTracker, &inputContext->cursorX, &tempInt);
                        }
                        break;
                    } else {
                        inputContext->cursorY += inputContext->line_spacing;
                        indexTracker -= lineSlices.arr[i].len;
                        runningOffset += lineSlices.arr[i].len;
                    }
                }

                DYNAMIC_ARRAY_FREE(lineSlices);
            } else {
                inputContext->cursorY = inputContext->cursorLine * inputContext->line_spacing;
                TTF_GetStringSize(inputContext->font, inputContext->buffer.arr[inputContext->cursorLine].arr, inputContext->cursorIndex, &inputContext->cursorX, &tempInt);
            }
        }

        if (inputContext->cursorY - inputContext->scrollVerticalOffset < 0) {
            InputContextSetVerticalScrollOffset(inputContext, inputContext->cursorY);
        } else if (inputContext->cursorY - inputContext->scrollVerticalOffset > inputContext->layout.height - inputContext->line_spacing) {
            InputContextSetVerticalScrollOffset(inputContext, inputContext->scrollVerticalOffset + inputContext->line_spacing);
            if (inputContext->cursorY - inputContext->scrollVerticalOffset > inputContext->layout.height - inputContext->line_spacing) {
                InputContextSetVerticalScrollOffset(inputContext, (inputContext->cursorY / inputContext->layout.height) * inputContext->layout.height);
            }
        }

        if (inputContext->cursorX - inputContext->scrollHorizontalOffset < 0) {
            const char* string_copy = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            size_t psLen = inputContext->buffer.arr[inputContext->cursorLine].len - inputContext->cursorIndex;
            Uint32 code = SDL_StepUTF8(&string_copy, &psLen);
            char buff[5] = {0};
            SDL_UCS4ToUTF8(code, buff);
            int codeWidth, codeHeight;
            TTF_GetStringSize(inputContext->font, buff, 0, &codeWidth, &codeHeight);
            InputContextSetHorizontalScrollOffset(inputContext, inputContext->scrollHorizontalOffset - codeWidth);

            if (inputContext->cursorX - inputContext->scrollHorizontalOffset < 0) {
                InputContextSetHorizontalScrollOffset(inputContext, (inputContext->cursorX / inputContext->layout.width) * inputContext->layout.width);
            }
        } else if (inputContext->cursorX - inputContext->scrollHorizontalOffset > inputContext->layout.width) {
            const char* string_copy = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            Uint32 code = SDL_StepBackUTF8((const char*)inputContext->buffer.arr[inputContext->cursorLine].arr, &string_copy);
            char buff[5] = {0};
            SDL_UCS4ToUTF8(code, buff);
            int codeWidth, codeHeight;
            TTF_GetStringSize(inputContext->font, buff, 0, &codeWidth, &codeHeight);
            InputContextSetHorizontalScrollOffset(inputContext, inputContext->scrollHorizontalOffset + codeWidth);

            if (inputContext->cursorX - inputContext->scrollHorizontalOffset > inputContext->layout.width) {
                InputContextSetHorizontalScrollOffset(inputContext, (inputContext->cursorX / inputContext->layout.width) * inputContext->layout.width);
            }
        }

        if (inputContext->mouseLeftDown == false) {
            inputContext->highlightStartLineIndex = inputContext->cursorLine;
            inputContext->highlightStartIndex = inputContext->cursorIndex;
        }
    }

    if (
        inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_CURSOR_WAS_MODIFIED ||
        inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED) {
        UpdateRenderIndexBounds(inputContext);
    }

    // This will reset all the flags to false
    inputContext->flags = (uint32_t)(0);
    DYNAMIC_ARRAY_CLEAR(inputContext->modifiedLines);

    return 0;
}

void InputContextSetHorizontalScrollOffset(InputContext* inputContext, int h_scroll_amount) {
    inputContext->scrollHorizontalOffset = SDL_clamp(h_scroll_amount, 0, inputContext->scrollMaxHorizontalOffset);
    UpdateRenderIndexBounds(inputContext);
}

void InputContextSetVerticalScrollOffset(InputContext* inputContext, int v_scroll_amount) {
    inputContext->scrollVerticalOffset = SDL_clamp(v_scroll_amount, 0, inputContext->scrollMaxVerticalOffset);
    UpdateRenderIndexBounds(inputContext);
}

int InputContextOpenFile(InputContext* inputContext, const char* file_path) {
    InputContextClear(inputContext);

    size_t file_buffer_len = 0;
    char* file_buffer = SDL_LoadFile(file_path, &file_buffer_len);
    if (file_buffer == NULL) {
        SDL_Log("Failed to load file: %s", SDL_GetError());
        return -1;
    }

    const char* file_buffer_copy = (const char*)file_buffer;

    size_t psLen = file_buffer_len;
    size_t psLenCopy = psLen;
    Uint32 code = SDL_StepUTF8(&file_buffer_copy, &psLen);

    int startByteOffset = 0;
    int currByteOffset = 0;
    while (code != SDL_INVALID_UNICODE_CODEPOINT) {
        int code_width = psLenCopy - psLen;

        if (code == '\n') {
            InputContextBufferInsert(inputContext, &file_buffer[startByteOffset], currByteOffset - startByteOffset);
            InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);

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

            // Have to make sure to update all the indices that come after and at the insertion index
            for (int i = 0; i < inputContext->modifiedLines.len; i++) {
                if (inputContext->modifiedLines.arr[i] >= (inputContext->cursorLine + 1)) {
                    inputContext->modifiedLines.arr[i]++;
                }
            }
            DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

            inputContext->cursorLine++;
            inputContext->cursorIndex = 0;
            InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);

            startByteOffset = currByteOffset + code_width;
        } else if (psLen == 0) {
            InputContextBufferInsert(inputContext, &file_buffer[startByteOffset], file_buffer_len - startByteOffset);
            InputContextModifiedLinesAppend(inputContext, inputContext->cursorLine);
            break;
        }

        currByteOffset += code_width;
        psLenCopy = psLen;
        code = SDL_StepUTF8(&file_buffer_copy, &psLen);
    }

    inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;

    SDL_free(file_buffer);
    return 0;
}