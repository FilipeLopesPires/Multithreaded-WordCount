#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAXSIZE 50  // maximum size possible for a word
#define MAXCHARSIZE \
    6  // maximum size (number of bytes) possible for a character

int main(int argc, char **argv) {
    // arrays containing all the characters defined as either word delimiters or
    // word mergers
    char delimeters[25][MAXCHARSIZE] = {
        " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
        "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};
    char mergers[5][MAXCHARSIZE] = {"‘", "’", "´", "`", "'"};

    // array with all the possible vowels
    char vowels[50][MAXCHARSIZE] = {
        "a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "á", "à", "ã",
        "â", "ä", "é", "è", "ẽ", "ê", "ë", "Á", "À", "Ã", "Â", "Ä", "É",
        "È", "Ẽ", "Ê", "Ë", "ó", "ò", "õ", "ô", "ö", "Ó", "Ò", "Õ", "Ô",
        "Ö", "í", "ì", "Í", "Ì", "ú", "ù", "ü", "Ú", "Ù", "Ü"};

    // Declare useful variables
    FILE *file;
    int numVowels;                    // number of vowels in current word
    int stringSize;                   // number of characters in current word
    char stringBuffer[MAXSIZE] = "";  // buffer containing the current word
    int i, j;                         // aux variables for local loops
    int ones;     // aux variable to count the number of ones in the most
                  // significant bits of the current character
    char nchar;   // aux variable to construct the complete symbol (used for
                  // those who use more than 1 byte)
    int control;  // aux variable to determine whether the program should update
                  // 'numVowels' and 'stringSize' or not
    bool increment;  // aux variable to determine whether the program should
                     // update 'stringSize' and 'stringBuffer' or not

    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    for (int fileIndex = 1; fileIndex < argc; fileIndex++) {
        file = fopen(argv[fileIndex], "r");
        if (file == NULL) {
            // end of file error
            printf("Error while opening file!");
            exit(1);
        } else {
            // Declare and initialize variables dedicated to the current file
            int largestWordSize = 0;  // size of the largest word found
            int totalNumberOfWords =
                0;        // total number of words of the current file
            char symbol;  // current character being processed
            char completeSymbol[MAXCHARSIZE];  // buffer for complex character
                                               // construction
            int wordCount[MAXSIZE];  // array containing the number of words
                                     // found whose size is equal to the
                                     // respective index
            int vowelsCount[MAXSIZE]
                           [MAXSIZE];  // 2D array containing the number of
                                       // words found whose number of vowels and
                                       // word size are equal to x and y
            float vowelsAvgCount[MAXSIZE];  // array containing the average
                                            // number of vowels found in a word
                                            // whose size is equal to the
                                            // respective index
            for (i = 0; i < MAXSIZE; i++) {
                wordCount[i] = 0;
                for (j = 0; j < MAXSIZE; j++) {
                    vowelsCount[i][j] = 0;
                }
            }
            numVowels = 0;
            stringSize = 0;
            strcpy(stringBuffer, "");
            control = 0;

            while ((symbol = getc(file)) != EOF) {
                strcpy(completeSymbol, "");
                increment = true;
                ones = 0;

                // Verify the number of 1s in the most significant bits
                for (i = sizeof(symbol) * CHAR_BIT - 1; i >= 0; --i) {
                    int bit = (symbol >> i) & 1;
                    if (bit == 0) {
                        break;
                    }
                    ones++;
                }

                // Build the complete character (if it consists of more than 1
                // byte)
                strncat(completeSymbol, &symbol, 1);
                for (i = 1; i < ones; i++) {
                    nchar = getc(file);
                    strncat(completeSymbol, &nchar, 1);
                }

                // 0  1    2    3    4    5    6    7   8   9   10  11 12 13 14
                // 15 16 17 0  1220 1695 1717 1287 1442 1076 626 434 204 115 70
                // 20 3  3  1  0  1
                //                                  -1  +1                -1 +1

                // Check if character is a delimiter
                for (i = 0; i < sizeof(delimeters) / sizeof(delimeters[0]);
                     i++) {
                    if (strcmp(completeSymbol, delimeters[i]) == 0) {
                        control = 1;
                        if (strlen(stringBuffer) > 0) {
                            wordCount[stringSize]++;
                            vowelsCount[numVowels][stringSize]++;
                            if (stringSize > largestWordSize) {
                                largestWordSize = stringSize;
                            }
                            totalNumberOfWords++;
                            if (stringSize == 14) {
                                // printf("%s: %d /%d | ", stringBuffer,
                                // numVowels, stringSize);
                            }
                            // printf("%s: %dvowels/%dsize\n", stringBuffer,
                            // numVowels, stringSize);
                            strcpy(stringBuffer, "");
                            numVowels = 0;
                            stringSize = 0;
                        }
                        break;
                    }
                }

                // Skip remaining steps if current character is a delimiter
                if (control == 1) {
                    control = 0;
                    continue;
                }

                // Increment word size and add character to current word (if
                // applicable)
                for (i = 0; i < sizeof(mergers) / sizeof(mergers[0]); i++) {
                    if (strcmp(completeSymbol, mergers[i]) == 0) {
                        increment = false;
                        break;
                    }
                }
                if (increment) {
                    stringSize++;
                    strcat(stringBuffer, completeSymbol);
                }

                // Increment number of vowels (if applicable)
                for (i = 0; i < sizeof(vowels) / sizeof(vowels[0]); i++) {
                    if (strcmp(completeSymbol, vowels[i]) == 0) {
                        numVowels++;
                        break;
                    }
                }
            }

            // Consider last word of file
            if (strlen(stringBuffer) > 0) {
                control = 1;
                wordCount[stringSize]++;
                vowelsCount[stringSize][numVowels] += numVowels;
                if (stringSize > largestWordSize) {
                    largestWordSize = stringSize;
                }
                totalNumberOfWords++;
                // printf("%s: %dvowels/%dsize\n", stringBuffer, numVowels,
                // stringSize);
                strcpy(stringBuffer, "");
                numVowels = 0;
                stringSize = 0;
            }

            // Print information table
            printf("File name: %s\n", argv[fileIndex]);
            printf("Total number of words: %d\n", totalNumberOfWords);
            printf("Word length\n");

            printf("   ");
            for (i = 1; i < largestWordSize + 1; i++) {
                printf("%6d", i);
            }
            printf("\n   ");
            for (i = 1; i < largestWordSize + 1; i++) {
                printf("%6d", wordCount[i]);
            }
            printf("\n   ");
            for (i = 1; i < largestWordSize + 1; i++) {
                printf("%6.2f", ((float)wordCount[i] * 100.0) /
                                    (float)totalNumberOfWords);
            }
            for (i = 0; i < largestWordSize + 1; i++) {
                printf("\n%2d ", i);
                for (int j = 1; j < i; j++) {
                    printf("%6s", " ");
                }
                if (i == 0) {
                    for (int j = 1; j < largestWordSize + 1; j++) {
                        if (wordCount[j] > 0) {
                            printf("%6.1f", (vowelsCount[i][j] * 100.0) /
                                                (float)wordCount[j]);
                        } else {
                            printf("%6.1f", 0.0);
                        }
                    }
                } else {
                    for (int j = i; j < largestWordSize + 1; j++) {
                        if (wordCount[j] > 0) {
                            printf("%6.1f", (vowelsCount[i][j] * 100.0) /
                                                (float)wordCount[j]);
                        } else {
                            printf("%6.1f", 0.0);
                        }
                    }
                }
            }
            printf("\n\n");

            // for (i=1; i<largestWordSize+1; i++) {
            //     printf("%d->%d\n", i, wordCount[i]);
            // }
            // printf("\n");
        }
        fclose(file);
    }

    return (0);
}