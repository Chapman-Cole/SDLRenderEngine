#include "InputContext.h"

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

static inline void InputContextUpdateCursor(InputContext* inputContext) {
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

    if (inputContext->bufferWasModified == true) {
        GPUTextUpdate(&inputContext->gpuText, inputContext->buffer);
    }

    // Update cursor
    if (inputContext->bufferWasModified == true || inputContext->cursorWasModified) {
        int currLineStartIndex = prev_line_start_index(inputContext->buffer, inputContext->cursorIndex);
        const char* currLineStart = &inputContext->buffer[currLineStartIndex];

        // Can't pass in 0 to TTF_GetStringSize because the function uses that as an indication
        // that it should treat it as a null terminated string, meaning it will screw up the x
        // length because it will fill it with the x length of the whole string
        if (inputContext->cursorIndex - currLineStartIndex <= 0) {
            inputContext->cursorX = 0;
        } else {
            TTF_GetStringSize(
                inputContext->gpuText.font,
                currLineStart,
                inputContext->cursorIndex - currLineStartIndex,
                &inputContext->cursorX,
                &inputContext->cursorY);
        }

        // +1 and minus sign necessary because the y position has to be a negative offset since it is drawn from the bottom left corner up
        inputContext->cursorY = -(get_num_newlines(inputContext->buffer, inputContext->cursorIndex) + 1) * inputContext->cursorHeight;
    }

    return 0;
}