#include "MAText.h"

#include "gfx.h"
#include <stdio.h>      // For sprintf. Remove this when no longer needed
#include <stdlib.h>
#include <string.h>

int getCharIndex(MAFont * font, uint8_t asciiID) {
    int count = font->nChars;
    int i;

    for (i=0; i<count; i++) {
        MACharDesc aCharDesc = font->charDescs[i];
        if (aCharDesc.asciiID == asciiID) {
            return i;
        }
    }

    return -1;
}

int getCharIndexOrErrorCharIndex(MAFont * font, uint8_t asciiID) {
    if (asciiID == 0) {
        return -1;
    }

    //Try to get the index of the actual character
    int charIndex = getCharIndex(font, asciiID);

    //If no index was found, try to get the index of the '?' character
    if (charIndex == -1) {
        charIndex = getCharIndex(font, 63);
    }

    //If no index was found, try to get the index of the space character
    if (charIndex == -1) {
        charIndex = getCharIndex(font, 32);
    }

    //If no index was found by this time, -1 will be returned

    //Return whatever index was found
    return charIndex;
}

int MADrawCharacter(gfxScreen_t screen, gfx3dSide_t side, MAFont *font, int charDescIndex, int cursorx, int cursory, int red, int green, int blue) {
    MACharDesc aCharDesc = font->charDescs[charDescIndex];

    //Get the image using the x/y/w/h from the image data
    //Memcpy this to a new image

    int dataOffsetTop = (aCharDesc.x * font->pngW);
    int dataOffsetLeft = ( font->pngW - aCharDesc.y - aCharDesc.h );
    int initialSourceByte = dataOffsetTop + dataOffsetLeft;
    int imageDataSize = (aCharDesc.w * aCharDesc.h)*4;

    u8 destImageData[imageDataSize];
    int destByte = 0;
    int row, col;
    for (row=0; row<aCharDesc.w; row++) {
        for (col=0; col<aCharDesc.h; col++) {
            int rowOffset = (row * font->pngW);
            int colOffset = col;
            int sourceByte = initialSourceByte + rowOffset + colOffset;

            memcpy(&destImageData[destByte], &blue, 1);                     destByte++;
            memcpy(&destImageData[destByte], &green, 1);                    destByte++;
            memcpy(&destImageData[destByte], &red, 1);                      destByte++;
            memcpy(&destImageData[destByte], &font->data[sourceByte], 1);   destByte++;
        }
    }

    //Draw the character
    gfxDrawSpriteAlphaBlend(screen, side, destImageData, aCharDesc.h, aCharDesc.w, cursorx, cursory);

    //Return the width of the character (for moving the cursor along to draw the next character)
    return aCharDesc.w;
}

void truncateTextToFitMaxWidth(char * text, int maxWidth, MAFont * font, bool force) {
    //If a maximum width in pixels was specified
    if (maxWidth > 0) {
        //Get the current width
        int textWidth = MATextWidthInPixels(text, font);

        //If the current width exceeds the maximum width
        if (textWidth > maxWidth) {
            //Start by truncating the original string by four characters - enough to remove one and add '...'
            int numTruncatedChars = 4;

            //Create a new string to contain, at a minimum, the original string truncated by one character and with '...' added
            char * truncatedText = malloc(strlen(text));

            //While the string is too big to fit
            while (textWidth > maxWidth) {
                //Erase the truncated string (all of it - not just the bit that was truncated before)
                memset(&truncatedText[0], 0, strlen(text));

                //Copy the original string, minus the number of characters which need to be truncated, to the truncated string
                size_t copyLen = strlen(text) - numTruncatedChars;
                size_t textLen = strlen(text);

                if (numTruncatedChars >= textLen)
                    copyLen = 0;
                else
                    copyLen = textLen - numTruncatedChars;

                memcpy(truncatedText, text, copyLen);
                truncatedText[copyLen] = '\0';


                //Add '...' to the end
                strcat(truncatedText, "...");

                //Recalculate the width of the truncated string
                textWidth = MATextWidthInPixels(truncatedText, font);

                //Increase the number of characters to truncate in case we need to loop round again and make the string even shorter next time
                numTruncatedChars++;

                //If we have truncated everything, give up and break. The original string will be drawn, but might not fit
                if (numTruncatedChars >= strlen(text)) {
                    break;
                }
            }

            //By this point, we should have a truncated string with '...' on the end which will fit within maxWidth
            //Erase the original (too long) string
            memset(&text[0], 0, strlen(text));

            //Overwrite it with the truncated string
            strcpy(text, truncatedText);

            //Free the memory for the truncated string
            free(truncatedText);
        }

        //If the text will fit but we want to force ellipses onto it
        else if (force) {
            char forced[strlen(text) + 4];
            strcpy(forced, text);
            strcat(forced, "...");
            memset(&text[0], 0, strlen(text));
            strcpy(text, forced);
        }
    }
}

void MADrawText(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue) {

    int offset = 0;

    char out[1024];
    strcpy(out, "");

    while(offset>-1) {
        uint8_t code = (uint8_t)text[offset];

//        if (code >= 128) {
//            int bytesnumber=0;
//            if (code < 224) bytesnumber = 2;
//            else if (code < 240) bytesnumber = 3;
//            else if (code < 248) bytesnumber = 4;
//            uint8_t codetemp = code - 192 - (bytesnumber > 2 ? 32 : 0) - (bytesnumber > 3 ? 16 : 0);
//            int i;
//
//            for (i = 2; i <= bytesnumber; i++) {
//                offset ++;
//                uint8_t code2 = text[offset] - 128;
//                codetemp = codetemp*64 + code2;
//            }
//            code = codetemp;
//        }

        char s[8];
        memset(s, 0, 8);
        sprintf(s, "%u\n", code);
        strcat(out, s);

        offset += 1;
        if (offset >= strlen(text)) offset = -1;


        /*
         We now have the correct character code, corrected for unicode chars.
         Yay!!!!!
         We should now be able to match the character to the code in the struct and draw it. Fingers crossed...
         */

        int charIndex = getCharIndexOrErrorCharIndex(font, code);
        if (charIndex > -1) {
            int xAdvance = MADrawCharacter(screen, side, font, charIndex, cursorx, cursory, red, green, blue);
            cursory += xAdvance;
        }
    }

}

bool stringContainsChar(char * string, char c) {
    return (strchr(string, c) != NULL);
}

bool charIsWordBreak(char c) {
    return stringContainsChar(".,-?! ", c);
}

bool charIsNewline(char c) {
    return stringContainsChar("\n\r", c);
}

int MADrawTextWrap(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue, int maxWidth, int maxLines) {



//    text = "pokémon Ω ";
//
//    MADrawText(screen, side, cursorx, cursory, text, font, red, green, blue);
//    return;

    //If no maximum width was specified, just draw the text
    if (maxWidth < 1) {
        MADrawText(screen, side, cursorx, cursory, text, font, red, green, blue);
        return 1;
    }

    if (maxLines == 1) {
        truncateTextToFitMaxWidth(text, maxWidth, font, false);
        MADrawText(screen, side, cursorx, cursory, text, font, red, green, blue);
        return 1;
    }

    //Store initial cursor position in case we need to go down a line
    int initialy = cursory;

    int lineHeight = font->lineHeight;

    //Start from line 1
    int currentline = 1;

    int maxWords = 128;
    int maxWordLen = 64;

    char words[maxWords][maxWordLen];
    memset(words, 0, maxWords*maxWordLen);

    int numWords = 0;

    int numChars = strlen(text);
    int currentWordStartPosition = 0;
    int i=0;

    /*
     This was originally added for testing the output of the sentence splitting function below.
     It is no longer used or even referred to beyond its declaration here and copying one character to it.
     I have no idea why, but if this is removed the text wrapping goes all fucked.
     I should probably try to figure it out, but I would rather have text wrapping working than
        save 1k of memory, and I don't really have the time to figure it out.
     */
    char out[1024];
    strcpy(out, "|");

    while (i < numChars) {
        char c = text[i];

        //We have reached a newline character
        if (charIsNewline(c)) {
            int currentWordEndPosition = i;
            int currentWordLength = currentWordEndPosition - currentWordStartPosition;
            char currentWord[maxWordLen];
            memset(currentWord, 0, maxWordLen);
            strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);
            strncpy(words[numWords], currentWord, maxWordLen);
            numWords++;
            i++;
            currentWordStartPosition = i;

            char newlineWord[14];
            strcpy(newlineWord, "***NEWLINE***");
            strncpy(words[numWords], newlineWord, maxWordLen);
            numWords++;
        }

        //We have reached the end of a word
        else if (charIsWordBreak(c)) {

//            bool scanToEndOfWord = true;

            //Now scan until the beginning of the next word or the end of the string
            while (true) {
                //Go to the next character
                i++;

                //If we have reached the end of the string, stop scanning
                if (i >= numChars) {
//                    scanToEndOfWord = false;
                    break;
                }

                //If we have not reached the end of the string, get the next character
                char c2 = text[i];

                //If we have reached a non word break character, stop scanning
                if (!charIsWordBreak(c2)) {
//                    scanToEndOfWord = false;
                    break;
                }
            }

            //Get the index of the character and store it as current word end position
            int currentWordEndPosition = i;

            //Calculate the length of the current word
            int currentWordLength = currentWordEndPosition - currentWordStartPosition;

            char currentWord[maxWordLen];
            memset(currentWord, 0, maxWordLen);
            strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);

//            strcat(out, currentWord);
//            strcat(out, "|");

            //Copy the word to the words array
            strncpy(words[numWords], currentWord, maxWordLen);

            //Prepare to start scanning the next word
            numWords++;
            currentWordStartPosition = i;
        }

        //We have not yet reached the end of a word
        else {
            //Go to the next character
            i++;

            //Last word - this is pretty much the same code as above, so needs to be abstracted to a function to save repetition (later!)
            if (i >= numChars) {
                //Get the index of the character and store it as current word end position
                int currentWordEndPosition = i;

                //Calculate the length of the current word
                int currentWordLength = currentWordEndPosition - currentWordStartPosition;

                char currentWord[maxWordLen];
                memset(currentWord, 0, maxWordLen);
                strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);

//                strcat(out, currentWord);
//                strcat(out, "|");

                strncpy(words[numWords], currentWord, maxWordLen);

                numWords++;
            }
        }
    }

    for (i=0; i < numWords; i++) {
        //Copy the current word from the array
        char currentWord[maxWordLen];
        memset(currentWord, 0, maxWordLen);
        strncpy(currentWord, words[i], maxWordLen - 1);
        currentWord[maxWordLen - 1] = '\0';

        if (strcmp(currentWord, "***NEWLINE***") == 0) {
            cursory = initialy;
            cursorx -= lineHeight;
            currentline++;

            continue;
        }

        //Calculate the width of the word about to be drawn
        int currentWordWidth = MATextWidthInPixels(currentWord, font);

        //Work out how much space is left on the current line
        int availableWidthForCurrentWord = maxWidth - (cursory - initialy);

        //If there are more words to draw after this one
        if (i < numWords-1) {
            //Copy the next word from the array
            char nextWord[maxWordLen]; // = new char[sNextWord.length() + 1];
            memset(nextWord, 0, maxWordLen);
            strncpy(nextWord, words[i+1], maxWordLen - 1);
            nextWord[maxWordLen - 1] = '\0';  // Ensure null termination

            //Calculate the width of the next word
            int nextWordWidth = MATextWidthInPixels(nextWord, font);

            //Calculate space available for the next word
            int availableWidthForNextWord = availableWidthForCurrentWord - currentWordWidth;

            //If the next word will fit on this line
            if (nextWordWidth <= availableWidthForNextWord) {
                //Draw the current word and move the cursor along ready to draw the next word on the next cycle of the loop
                MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
                cursory += currentWordWidth;
            }

            //If the next word will not fit on this line
            else {
                //If there is no restriction on the number of lines, or if we have not yet reached the last line
                if (maxLines < 1 || currentline < maxLines) {
                    MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
                    cursory = initialy;
                    cursorx -= lineHeight;
                    currentline++;
                }

                //If there are no more lines left
                else {
                    //Create the minimum truncated version of the next word (its first letter plus ellipses)
                    char minimumTruncatedNextWord[5];
                    memset(minimumTruncatedNextWord, 0, 5);
                    strncpy(minimumTruncatedNextWord, nextWord, 1);
                    strcat(minimumTruncatedNextWord, "...");

                    //Get the width of the minimum truncated next word
                    int minimumTruncatedNextWordWidth = MATextWidthInPixels(minimumTruncatedNextWord, font);

                    //If the minimum truncated next word will fit
                    if (minimumTruncatedNextWordWidth <= availableWidthForNextWord) {
                        //Draw the current word
                        MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
                        cursory += currentWordWidth;

                        //Truncate the next word so it will fit on the line with ellipses
                        truncateTextToFitMaxWidth(nextWord, availableWidthForNextWord, font, false);

                        //Draw the next word
                        MADrawText(screen, side, cursorx, cursory, nextWord, font, red, green, blue);

                        //Break since there's no space for any more words
                        break;
                    }

                    //Even the minimum truncated next word won't fit on this line, and there isn't a line available below for it to go on.
                    else {
                        //Truncate the current word and add ellipses
                        truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, true);

                        //Draw the truncated current word
                        MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);

                        //Break, since there's not space for any more words
                        break;
                    }
                }
            }
        }

        //This is the last word
        else {
            //If the last word fits, just draw it and we're done
            if (currentWordWidth <= availableWidthForCurrentWord) {
                MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
                /*
                 DONE
                 */
            }

            //The last word doesn't fit on the current line
            else {
                //If there is no restriction on the number of lines, or if we still have lines available
                if (maxLines < 1 || currentline < maxLines) {
                    //Move the cursor to the beginning of the next line
                    cursory = initialy;
                    cursorx -= lineHeight;

                    //Truncate the last word if necessary
                    if (availableWidthForCurrentWord > maxWidth) {
                        truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, false);
                    }

                    //Draw the last word
                    MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);

                    /*
                     DONE
                     */
                }

                //There are no more lines available. Truncate the final word and display it
                else {
                    truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, false);
                    MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);

                    /*
                     DONE
                     */
                }
            }
        }
    }

    return currentline;
}

int MATextWidthInPixels(char * text, MAFont * font) {
    int size = 0;

    int i;
    int n = strlen(text);

    for (i=0; i<n; i++) {
        char c = text[i];
        int asciiID = c;
        int charIndex = getCharIndexOrErrorCharIndex(font, asciiID);

        if (charIndex > -1) {
            MACharDesc aCharDesc = font->charDescs[charIndex];

            size += aCharDesc.w;
        }
    }

    return size;
}
