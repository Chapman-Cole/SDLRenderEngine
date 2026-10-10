#include "InputContext.h"
#include "DynamicArray.h"
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef struct IntArray {
    int* arr;
    size_t len;
    size_t capacity;
} IntArray;

typedef struct CharArray {
    char* arr;
    size_t len;
    size_t capacity;
} CharArray;

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

static inline int prev_line_start_index(const char* buffer, int cursorIndex) {
    const char* prevNewlineStart = &buffer[cursorIndex];
    int newlineIndex = cursorIndex;

    while (true) {
        Uint32 code = SDL_StepBackUTF8(buffer, &prevNewlineStart);
        newlineIndex -= get_utf8_width(code);

        if (code == '\n') {
            return newlineIndex + 1; // This will get the actual start instead of including the newline
        } else if (code == 0) {
            // code == 0 means it reached the start of the buffer
            return 0;
        }
    }
}

static inline int next_line_start_index(const char* buffer, int cursorIndex) {
    const char* start = buffer + cursorIndex;
    int nextLineStartIndex = cursorIndex;

    while (true) {
        Uint32 code = SDL_StepUTF8(&start, NULL);
        nextLineStartIndex += get_utf8_width(code);

        if (code == '\n') {
            return nextLineStartIndex;
        } else if (code == 0) {
            // code == 0 means it reached the end of the buffer. The minus 1 is needed
            // because otherwise the null terminator is counted as a character
            return nextLineStartIndex - 1;
        }
    }
}

static inline int get_num_newlines(const char* buffer, int cursorIndex) {
    int tracker = 0;
    int numNewLines = 0;
    while (tracker < cursorIndex) {
        Uint32 code = SDL_StepUTF8(&buffer, NULL);
        if (code == '\n') {
            numNewLines++;
        }
        tracker += get_utf8_width(code);
    }

    return numNewLines;
}

static inline int InputContextBufferInsert(InputContext* inputContext, const char* text) {
    int textLen = SDL_strlen(text);

    if (inputContext->bufferLen + textLen >= inputContext->bufferCapacity) {
        inputContext->bufferCapacity += INPUT_CONTEXT_BASE_BUFFER_SIZE;
        void* temp_ptr = SDL_realloc(inputContext->buffer, inputContext->bufferCapacity);
        if (temp_ptr == NULL) {
            SDL_Log("Failed to re-allocate memory for InputContext buffer: %s", SDL_GetError());
            return -1;
        }
        inputContext->buffer = temp_ptr;
    }

    if (inputContext->cursorIndex == inputContext->bufferLen) {
        SDL_memcpy(inputContext->buffer + inputContext->bufferLen, text, textLen);
    } else {
        SDL_memmove(inputContext->buffer + inputContext->cursorIndex + textLen, inputContext->buffer + inputContext->cursorIndex, inputContext->bufferLen - inputContext->cursorIndex);
        SDL_memcpy(inputContext->buffer + inputContext->cursorIndex, text, textLen);
    }

    inputContext->bufferLen += textLen;
    inputContext->cursorIndex += textLen;

    // Keep the input buffer null terminated
    inputContext->buffer[inputContext->bufferLen] = '\0';

    return 0;
}

// Generates the GPUText object while accounting for text wrapping
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

    GPUTextUpdate(&inputContext->gpuText, tempBuffer.arr, tempBuffer.len);

    DYNAMIC_ARRAY_FREE(indices);
    DYNAMIC_ARRAY_FREE(tempBuffer);
    return 0;
}

int InputContextUpdate(InputContext* inputContext, SDL_Event* event) {
    bool bufferModified = false;
    bool cursorModified = false;

    switch (event->type) {
    case SDL_EVENT_TEXT_INPUT: {
        InputContextBufferInsert(inputContext, event->text.text);
        bufferModified = true;
    } break;

    case SDL_EVENT_KEY_DOWN: {
        switch (event->key.key) {
        case SDLK_BACKSPACE: {
            if (inputContext->cursorIndex == 0) {
                return 0;
            }

            const char* indexStr = &inputContext->buffer[inputContext->cursorIndex];
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer, &indexStr);
            int prevLen = get_utf8_width(code);

            if (inputContext->cursorIndex != inputContext->bufferLen) {
                SDL_memmove(inputContext->buffer + inputContext->cursorIndex - prevLen, inputContext->buffer + inputContext->cursorIndex, inputContext->bufferLen - inputContext->cursorIndex);
            }

            inputContext->bufferLen -= prevLen;
            inputContext->cursorIndex -= prevLen;
            inputContext->buffer[inputContext->bufferLen] = '\0';

            // -1 is to account for null terminator
            if (inputContext->bufferLen < inputContext->bufferCapacity - INPUT_CONTEXT_BASE_BUFFER_SIZE - 1) {
                inputContext->bufferCapacity -= INPUT_CONTEXT_BASE_BUFFER_SIZE;
                void* temp_ptr = SDL_realloc(inputContext->buffer, inputContext->bufferCapacity);
                if (temp_ptr == NULL) {
                    SDL_Log("Failed to re-allocate memory for InputContext buffer: %s", SDL_GetError());
                    return -1;
                }
                inputContext->buffer = temp_ptr;
            }

            bufferModified = true;
        } break;

        case SDLK_RETURN: {
            InputContextBufferInsert(inputContext, "\n");
            bufferModified = true;
        } break;

        case SDLK_LEFT: {
            const char* indexStr = &inputContext->buffer[inputContext->cursorIndex];
            Uint32 code = SDL_StepBackUTF8(inputContext->buffer, &indexStr);
            inputContext->cursorIndex -= get_utf8_width(code);
            if (inputContext->cursorIndex < 0) {
                inputContext->cursorIndex = 0;
            }
            cursorModified = true;
        } break;

        case SDLK_RIGHT: {
            const char* start = &inputContext->buffer[inputContext->cursorIndex];
            Uint32 code = SDL_StepUTF8(&start, NULL);
            inputContext->cursorIndex += get_utf8_width(code);
            if (inputContext->cursorIndex > inputContext->bufferLen) {
                inputContext->cursorIndex = inputContext->bufferLen;
            }
            cursorModified = true;
        } break;

        case SDLK_UP: {
            int currLineStartIndex = prev_line_start_index(inputContext->buffer, inputContext->cursorIndex);
            if (currLineStartIndex == 0) {
                inputContext->cursorIndex = 0;
            } else {
                int currLineCharOffset = SDL_utf8strnlen(inputContext->buffer + currLineStartIndex, inputContext->cursorIndex - currLineStartIndex);

                // -1 to avoid including the newline character
                int prevLineStartIndex = prev_line_start_index(inputContext->buffer, currLineStartIndex - 1);
                int prevLineCharLength = SDL_utf8strnlen(inputContext->buffer + prevLineStartIndex, currLineStartIndex - 1 - prevLineStartIndex);

                if (prevLineCharLength <= currLineCharOffset) {
                    inputContext->cursorIndex = currLineStartIndex - 1;
                } else {
                    const char* start = inputContext->buffer + prevLineStartIndex;
                    inputContext->cursorIndex = prevLineStartIndex;
                    for (int i = 0; i < currLineCharOffset; i++) {
                        Uint32 code = SDL_StepUTF8(&start, NULL);
                        inputContext->cursorIndex += get_utf8_width(code);
                    }
                }
            }
            cursorModified = true;
        } break;

        case SDLK_DOWN: {
            int currLineStartIndex = prev_line_start_index(inputContext->buffer, inputContext->cursorIndex);
            int currLineCharOffset = SDL_utf8strnlen(inputContext->buffer + currLineStartIndex, inputContext->cursorIndex - currLineStartIndex);

            int nextLineStartIndex = next_line_start_index(inputContext->buffer, inputContext->cursorIndex);

            if (nextLineStartIndex >= inputContext->bufferLen) {
                inputContext->cursorIndex = inputContext->bufferLen;
            } else {
                int nextNextLineStartIndex = next_line_start_index(inputContext->buffer, nextLineStartIndex);
                int nextLineCharLength = SDL_utf8strnlen(inputContext->buffer + nextLineStartIndex, nextNextLineStartIndex - nextLineStartIndex);

                if (nextLineCharLength <= currLineCharOffset) {
                    if (nextNextLineStartIndex >= inputContext->bufferLen) {
                        inputContext->cursorIndex = inputContext->bufferLen;
                    } else {
                        // -1 is to account for the null terminator
                        inputContext->cursorIndex = nextNextLineStartIndex - 1;
                    }
                } else {
                    // The minus 1 is to account for the fact that SDL_StepUTF8 function
                    // gets the next character, so it is necessary to start with the null
                    // terminator so that the next character represents the start of the line
                    const char* start = inputContext->buffer + nextLineStartIndex - 1;
                    inputContext->cursorIndex = nextLineStartIndex;
                    for (int i = 0; i < currLineCharOffset; i++) {
                        Uint32 code = SDL_StepUTF8(&start, NULL);
                        inputContext->cursorIndex += get_utf8_width(code);
                    }
                }
            }
            cursorModified = true;
        } break;

        case SDLK_LCTRL: {
            if (inputContext->keySequencesLen >= INPUT_CONTEXT_KEY_SEQUENCE_MAX) {
                inputContext->keySequencesLen = 0;
            }

            inputContext->keySequences[inputContext->keySequencesLen] = SDLK_LCTRL;
            inputContext->keySequencesLen++;
        } break;

        case SDLK_V: {
            if (inputContext->keySequencesLen >= INPUT_CONTEXT_KEY_SEQUENCE_MAX) {
                inputContext->keySequencesLen = 0;
            }

            inputContext->keySequences[inputContext->keySequencesLen] = SDLK_V;
            inputContext->keySequencesLen++;
        }
        }
    } break;
    }

    if (bufferModified == true) {
        inputContext->bufferWasModified = true;
    }

    if (bufferModified == true || cursorModified == true) {
        inputContext->cursorWasModified = true;
    }

    return 0;
}

int InputContextEnd(InputContext* inputContext) {
    // Parse the key sequences to look for commands, if any
    bool commandCompleted = false;
    switch (inputContext->keySequences[0]) {
    case SDLK_LCTRL: {
        switch (inputContext->keySequences[1]) {
        case SDLK_V: {
            char* clipboardText = SDL_GetClipboardText();
            InputContextBufferInsert(inputContext, clipboardText);
            SDL_free(clipboardText);
            commandCompleted = true;
        } break;
        }
    } break;
    }

    if (commandCompleted == true) {
        SDL_zeroa(inputContext->keySequences);
        inputContext->keySequencesLen = 0;
        inputContext->bufferWasModified = true;
    }

    // If the layout of the input context changes, the buffer must be re-generated
    if (
        inputContext->layout.x != inputContext->prevLayout.x ||
        inputContext->layout.y != inputContext->prevLayout.y ||
        inputContext->layout.width != inputContext->prevLayout.width ||
        inputContext->layout.height != inputContext->prevLayout.height
    ) {
        inputContext->bufferWasModified = true;
        inputContext->prevLayout = inputContext->layout;
    }

    if (inputContext->bufferWasModified == true) {
        InputContextGenTextWrap(inputContext);
    }

    // Update cursor
    if (inputContext->bufferWasModified == true || inputContext->cursorWasModified) {
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

    return 0;
}
