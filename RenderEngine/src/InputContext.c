#include "InputContext.h"
#include "DynamicArray.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

static inline void text_render_info_init(TextRenderInfo* info, SDL_GPUDevice* device, TTF_TextEngine* textEngine, TTF_Font* font) {
    info->text = TTF_CreateText(textEngine, font, "", 0);
    if (info->text == NULL) {
        SDL_Log("Failed to create text object for TextRenderInfo: %s", SDL_GetError());
        info->text = NULL;
    }
    info->vertexBuffer = NULL;
    info->indexBuffer = NULL;
    DYNAMIC_ARRAY_INIT(info->sequence);
}

static inline void text_render_info_free(TextRenderInfo* info) {
    TTF_DestroyText(info->text);
    GPUBuffer_unref(info->vertexBuffer);
    GPUBuffer_unref(info->indexBuffer);
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

static inline int InputContextGenerateBuffersForLine(SDL_GPUDevice* device, TextRenderInfo* renderInfo) {
    DYNAMIC_ARRAY_CLEAR(renderInfo->sequence);
    GPUBuffer_unref(renderInfo->vertexBuffer);
}

static inline int InputContextGenerateBuffers(InputContext* inputContext) {
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

    inputContext->layout = layout;

    inputContext->flags &= ~INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
    inputContext->flags &= ~INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
    inputContext->flags &= ~INPUT_CONTEXT_LAYOUT_WAS_MODIFIED;

    inputContext->font = font;

    DYNAMIC_ARRAY_INIT(inputContext->buffer);

    TextLine firstLine;
    DYNAMIC_ARRAY_INIT(firstLine);
    DYNAMIC_ARRAY_APPEND(inputContext->buffer, firstLine);

    DYNAMIC_ARRAY_INIT(inputContext->renderInfo);

    TextRenderInfo renderInfoLine1;
    text_render_info_init(&renderInfoLine1, inputContext->app_context->device, inputContext->app_context->textEngine, font);
    DYNAMIC_ARRAY_APPEND(inputContext->renderInfo, renderInfoLine1);

    return 0;
}

int InputContextDestroy(InputContext* inputContext) {
}

static inline int InputContextBufferInsert(InputContext* inputContext, char* text) {
    int textLen = SDL_strlen(text);
    DYNAMIC_ARRAY_INSERT_ARRAY(inputContext->buffer.arr[inputContext->cursorLine], ((TextLine){.arr = text, .len = textLen, .capacity = 0}), inputContext->cursorIndex);
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

int InputContextProcessEvent(InputContext* inputContext, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_TEXT_INPUT: {
        InputContextBufferInsert(inputContext, (char*)event->text.text);
        inputContext->cursorIndex += SDL_strlen(event->text.text);
        inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
    } break;

    case SDL_EVENT_KEY_DOWN: {
        const bool* key_states = SDL_GetKeyboardState(NULL);

        switch (event->key.key) {
        case SDLK_BACKSPACE: {
            if (inputContext->cursorIndex == 0 && inputContext->cursorLine > 0) {
                DYNAMIC_ARRAY_INSERT_ARRAY(
                    inputContext->buffer.arr[inputContext->cursorLine - 1],
                    inputContext->buffer.arr[inputContext->cursorLine],
                    inputContext->buffer.arr[inputContext->cursorLine - 1].len);

                DYNAMIC_ARRAY_REMOVE(inputContext->buffer, inputContext->cursorLine);

                inputContext->cursorLine--;
                if (inputContext->cursorLine < 0) {
                    inputContext->cursorLine = 0;
                }
                if (inputContext->cursorIndex > inputContext->buffer.arr[inputContext->cursorLine].len) {
                    inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
                }

                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            } else if (inputContext->cursorIndex > 0) {
                const char* indexStr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
                Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
                Uint32 code_width = get_utf8_width(code);

                DYNAMIC_ARRAY_REMOVE_SELECTION(inputContext->buffer.arr[inputContext->cursorLine], inputContext->cursorIndex, code_width);
                inputContext->cursorIndex -= code_width;
                if (inputContext->cursorIndex < 0) {
                    inputContext->cursorIndex = 0;
                }

                inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
            }
        } break;

        case SDLK_RETURN: {
            TextLine newLine;
            DYNAMIC_ARRAY_INIT(newLine);
            DYNAMIC_ARRAY_INSERT(inputContext->buffer, inputContext->cursorLine + 1, newLine);

            TextRenderInfo newLineInfo;
            text_render_info_init(&newLineInfo, inputContext->app_context->device, inputContext->app_context->textEngine, inputContext->font);
            DYNAMIC_ARRAY_APPEND(inputContext->renderInfo, newLineInfo);

            inputContext->cursorLine++;
            inputContext->flags |= INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
        } break;

        case SDLK_LEFT: {
            const char* indexStr = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer.arr[inputContext->cursorLine].arr, &indexStr);
            inputContext->cursorIndex -= get_utf8_width(code);
            if (inputContext->cursorIndex < 0) {
                inputContext->cursorIndex = 0;
                inputContext->cursorLine--;
                if (inputContext->cursorLine < 0) {
                    inputContext->cursorLine = 0;
                }
            }
            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_RIGHT: {
            const char* start = &inputContext->buffer.arr[inputContext->cursorLine].arr[inputContext->cursorIndex];
            Uint32 code = SDL_StepUTF8(&start, NULL);
            inputContext->cursorIndex += get_utf8_width(code);
            if (inputContext->cursorIndex > inputContext->buffer.arr[inputContext->cursorLine].len) {
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
            }
            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_UP: {
            inputContext->cursorLine--;
            if (inputContext->cursorLine < 0) {
                inputContext->cursorLine = 0;
            }

            if (inputContext->cursorIndex > inputContext->buffer.arr[inputContext->cursorLine].len) {
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
            }

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        case SDLK_DOWN: {
            inputContext->cursorLine++;
            if (inputContext->cursorLine >= inputContext->buffer.len) {
                inputContext->cursorLine = inputContext->buffer.len - 1;
            }

            if (inputContext->cursorLine == inputContext->buffer.len - 1) {
                inputContext->cursorIndex = inputContext->buffer.arr[inputContext->cursorLine].len;
            }

            inputContext->flags |= INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
        } break;

        // Process ctrl + v for pasting
        case SDLK_V: {
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                /* Multi line pastes will be challenging, so handle them later for now
                char* clipboardText = SDL_GetClipboardText();
                InputContextBufferInsert(inputContext, clipboardText);
                SDL_free(clipboardText);
                bufferModified = true;
                */
            }
        }
        }
    } break;
    }

    return 0;
}

static inline void InputContextGenerateGPUBuffers(InputContext* inputContext) {
}

int InputContextRefresh(InputContext* inputContext) {
    if (inputContext->flags & INPUT_CONTEXT_BUFFER_WAS_MODIFIED || inputContext->flags & INPUT_CONTEXT_LAYOUT_WAS_MODIFIED) {
        // InputContextGenTextWrap(inputContext);
    }

    // Update cursor
    /*
    if (inputContext->bufferWasModified == true || inputContext->cursorWasModified) {
        // The text could be a NULL pointer if it is set with an empty string, and get_num_newlines
        // and prev_line_start_index can't handle NULL pointers
        if (inputContext->gpuText.text->text == NULL) {
            inputContext->cursorY = 0;
            inputContext->cursorX = 0;
        } else {
            int currLineStartIndex = prev_line_start_index(inputContext->gpuText.text->text, inputContext->cursorIndex);
            const char* currLineStart = &inputContext->gpuText.text->text[currLineStartIndex];

            // Can't pass in 0 to TTF_GetStringSize because the function uses that as an indication
            // that it should treat it as a null terminated string, meaning it will screw up the x
            // length because it will fill it with the x length of the whole string
            if (inputContext->cursorIndex - currLineStartIndex <= 0) {
                inputContext->cursorX = 0;
            } else {
                TTF_GetStringSize(
                    inputContext->font,
                    currLineStart,
                    inputContext->cursorIndex - currLineStartIndex,
                    &inputContext->cursorX,
                    &inputContext->cursorY);
            }

            inputContext->cursorY = get_num_newlines(inputContext->gpuText.text->text, inputContext->cursorIndex) * inputContext->cursorHeight;
        }
    }
        */

    inputContext->flags &= ~INPUT_CONTEXT_BUFFER_WAS_MODIFIED;
    inputContext->flags &= ~INPUT_CONTEXT_CURSOR_WAS_MODIFIED;
    inputContext->flags &= ~INPUT_CONTEXT_LAYOUT_WAS_MODIFIED;

    return 0;
}
