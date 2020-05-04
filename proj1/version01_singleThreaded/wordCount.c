/**
 *  \file wordCount.c (implementation file)
 *
 *  \brief Program that computes the occurring frequency of word lengths and the number of vowels in each word for texts given as input.
 *
 *  The program 'wordCount' reads in succession several text files text#.txt and prints a listing of the occurring frequency of word lengths and the number of vowels in each word for each of the supplied texts.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/** \brief maximum size possible for a word. */
#define MAXSIZE 50

/** \brief maximum size (number of bytes) possible for a character. */
#define MAXCHARSIZE 6

/** 
 *  \brief Main function called when the program is executed.
 * 
 *  Main function of the 'wordCount' program containing all of its logic.
 *  The function receives the paths to the text files.
 * 
 *  \param argc number of files passed to the program.
 *  \param argv paths to the text files.
 * 
 */
int main(int argc, char **argv) {

    // Declare useful variables

    /** \brief array containing all the characters defined as word delimiters. */
    char delimeters[25][MAXCHARSIZE] = {
        " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
        "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

    /** \brief array containing all the characters defined as word mergers. */
    char mergers[7][MAXCHARSIZE] = {"‘", "’", "´", "`", "'", "ü", "Ü"};

    /** \brief array containing all the the possible vowels. */
    char vowels[48][MAXCHARSIZE] = {
        "a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "á", "à",
        "ã", "â", "ä", "é", "è", "ẽ", "ê", "ë", "Á", "À", "Ã", "Â",
        "Ä", "É", "È", "Ẽ", "Ê", "Ë", "ó", "ò", "õ", "ô", "ö", "Ó",
        "Ò", "Õ", "Ô", "Ö", "í", "ì", "Í", "Ì", "ú", "ù", "Ú", "Ù"};

    /** \brief pointer to the file currently under processing. */
    FILE *file;

    /** \brief number of vowels in current word. */
    int numVowels;

    /** \brief number of characters in current word. */
    int stringSize;

    /** \brief buffer containing the current word. */
    char stringBuffer[MAXSIZE] = "";
    
    /** \brief auxiliar variables for local loops. */
    int i, j;
    
    /** \brief auxiliar variable to count the number of ones in the most significant bits of the current character. */
    int ones;
    
    /** \brief auxiliar variable to construct the complete symbol (used for those who use more than 1 byte). */
    char nchar;
    
    /** \brief auxiliar variable to determine whether the program should update 'numVowels' and 'stringSize' or not. */
    int control;
    
    /** \brief auxiliar variable to determine whether the program should update 'stringSize' and 'stringBuffer' or not. */
    bool increment;

    // Validate program arguments

    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Process all files passed as arguments and compute the occurring frequency of word lengths and the number of vowels in each word

    for (int fileIndex = 1; fileIndex < argc; fileIndex++) {
        file = fopen(argv[fileIndex], "r");
        if (file == NULL) {
            // end of file error
            printf("Error while opening file!");
            exit(1);
        } else {

            // Declare and initialize variables dedicated to the current file

            /** \brief size of the largest word found. */
            int largestWordSize = 0;

            /** \brief total number of words of the current file. */
            int totalNumberOfWords = 0;

            /** \brief current character being processed. */
            char symbol;

            /** \brief buffer for complex character construction. */
            char completeSymbol[MAXCHARSIZE];

            /** \brief array containing the number of words found whose size is equal to the respective index. */
            int wordCount[MAXSIZE];

            /** \brief 2D array containing the number of words found whose number of vowels and word size are equal to x and y. */
            int vowelsCount[MAXSIZE][MAXSIZE];

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

                // Build the complete character (if it consists of more than 1 byte)
                strncat(completeSymbol, &symbol, 1);
                for (i = 1; i < ones; i++) {
                    nchar = getc(file);
                    strncat(completeSymbol, &nchar, 1);
                }

                // Check if character is a delimiter
                for (i = 0; i < sizeof(delimeters) / sizeof(delimeters[0]); i++) {
                    if (strcmp(completeSymbol, delimeters[i]) == 0) {
                        control = 1;
                        if (strlen(stringBuffer) > 0) {
                            wordCount[stringSize]++;
                            vowelsCount[numVowels][stringSize]++;
                            if (stringSize > largestWordSize) {
                                largestWordSize = stringSize;
                            }
                            totalNumberOfWords++;
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
                // vowelsCount[stringSize][numVowels] += numVowels;
                vowelsCount[numVowels][stringSize]++;
                if (stringSize > largestWordSize) {
                    largestWordSize = stringSize;
                }
                totalNumberOfWords++;
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
                printf("%6.2f", ((float)wordCount[i] * 100.0) / (float)totalNumberOfWords);
            }
            for (i = 0; i < largestWordSize + 1; i++) {
                printf("\n%2d ", i);
                for (int j = 1; j < i; j++) {
                    printf("%6s", " ");
                }
                if (i == 0) {
                    for (int j = 1; j < largestWordSize + 1; j++) {
                        if (wordCount[j] > 0) {
                            printf("%6.1f", (vowelsCount[i][j] * 100.0) / (float)wordCount[j]);
                        } else {
                            printf("%6.1f", 0.0);
                        }
                    }
                } else {
                    for (int j = i; j < largestWordSize + 1; j++) {
                        if (wordCount[j] > 0) {
                            printf("%6.1f", (vowelsCount[i][j] * 100.0) / (float)wordCount[j]);
                        } else {
                            printf("%6.1f", 0.0);
                        }
                    }
                }
            }
            printf("\n\n");
        }
        fclose(file);
    }

    return (0);
}