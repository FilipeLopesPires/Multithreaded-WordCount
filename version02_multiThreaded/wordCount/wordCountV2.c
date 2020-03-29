/**
 *  \file wordCount.c (implementation file)
 *
 *  \brief Program that reads in succession several text files and prints a
 *  listing of the occurring frequency of word lengths and the number of vowels
 *  in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 *  enables the creation of a monitor of the Lampson / Redell type.
 *
 *  Generator thread of the intervening entities.
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

#include <pthread.h>
//#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "controlInfo.h"
#include "textProcV2.h"
#include "wordCount.h"

/** \brief worker life cycle routine */
static void *worker(void *id);

/** \brief worker threads return status array */
int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
int statusMain;

/** \brief array containing all the characters defined as word delimiters */
char delimeters[25][MAXCHARSIZE] = {
    " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
    "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

/** \brief array containing all the characters defined as word mergers */
char mergers[7][MAXCHARSIZE] = {"‘", "’", "´", "`", "'", "ü", "Ü"};

/** \brief array containing all the possible vowels */
char vowels[48][MAXCHARSIZE] = {
    "a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "á", "à",
    "ã", "â", "ä", "é", "è", "ẽ", "ê", "ë", "Á", "À", "Ã", "Â",
    "Ä", "É", "È", "Ẽ", "Ê", "Ë", "ó", "ò", "õ", "ô", "ö", "Ó",
    "Ò", "Õ", "Ô", "Ö", "í", "ì", "Í", "Ì", "ú", "ù", "Ú", "Ù"};

int main(int argc, char **argv) {
    // Validate number of arguments passed to the program
    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables
    pthread_t workerThreadID[NUMWORKERS];  // workers internal thread id array
    unsigned int
        workerID[NUMWORKERS];  // workers application defined thread id array
    int *status_p;             // pointer to execution status
    int i;                     // aux variable for local loops
    double t0, t1;             // time limits

    // Initialization of thread IDs
    for (i = 0; i < NUMWORKERS; i++) {
        workerID[i] = i;
    }
    srandom((unsigned int)getpid());
    t0 = ((double)clock()) / CLOCKS_PER_SEC;

    // Retrieval of filenames
    char *files[argc - 1];
    for (i = 1; i < argc; i++) {
        files[i - 1] = argv[i];
    }

    presentFilenames(argc - 1, files);

    // Generation of worker threads
    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_create(&workerThreadID[i], NULL, worker, &workerID[i]) !=
            0) {
            perror("Error on creating thread worker.\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("Threads created.\n");

    // Report post task completion (by workers)
    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_join(workerThreadID[i], (void *)&status_p) != 0) {
            perror("Error on waiting for thread worker.\n");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *status_p);
    }
    printResults();
    destroy();

    // Execution time calculation
    t1 = ((double)clock()) / CLOCKS_PER_SEC;
    printf("\nElapsed time = %.6f s\n", t1 - t0);

    exit(EXIT_SUCCESS);
}

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */

static void *worker(void *par) {
    // Instantiate thread variables
    int id;  // worker ID
    // char stringBuffer[MAXSIZE];             // buffer containing the current
    // word
    int stringSize;  // number of characters in current word
    int numVowels;   // number of vowels in current word
    char symbol;     // current char under processing from the current chunk
    char completeSymbol[MAXCHARSIZE];  // buffer for complex character
                                       // construction
    // int wordCount[MAXSIZE];  // array containing the number of words found
    // whose
    //                          // size is equal to the respective index
    // // int vowelsCount[MAXSIZE][MAXSIZE];      // 2D array containing the
    // number
    // // of words found whose number of vowels and word size are equal to x and
    // y int **vowelsCount;  // 2D array containing the number of words found
    // whose number of vowels and word size are equal to x and y
    // int localMaxWordSize;    // largest word found in the text chunk
    // int localMaxVocalCount;  // largest number of vowels found in a word from
    // the text chunk
    int ones;     // aux variable to count the number of ones in the most
                  // significant bits of the current character
    char nchar;   // aux variable to construct the complete symbol (used for
                  // those who use more than 1 byte)
    int control;  // aux variable to determine whether the program should update
                  // 'numVowels' and 'stringSize' or not
    bool increment;  // aux variable to determine whether the program should
                     // update 'stringSize' and 'stringBuffer' or not
    int h, i, j;     // aux variables for local loops

    // Initialize thread variables
    id = *((int *)par);
    struct controlInfo controlInfo;
    for (i = 0; i < MAXSIZE; i++) {
        controlInfo.wordSize[i] = 0;
    }

    for (i = 0; i < MAXSIZE; i++) {
        for (j = 0; j < MAXSIZE; j++) {
            controlInfo.vowelCount[i][j] = 0;
        }
    }
    controlInfo.maxWordSize = 0;
    controlInfo.maxVowelCount = 0;
    numVowels = 0;
    stringSize = 0;
    // strcpy(stringBuffer, "");
    control = 0;

    // Process text chunk
    char textChunk[BUFFERSIZE] = "";

    while (getTextChunk(id, textChunk, controlInfo)) {
        for (h = 0; h < strlen(textChunk); h++) {
            symbol = textChunk[h];
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
                h++;
                nchar = textChunk[h];
                strncat(completeSymbol, &nchar, 1);
            }

            // Check if character is a delimiter
            for (i = 0; i < sizeof(delimeters) / sizeof(delimeters[0]); i++) {
                if (strcmp(completeSymbol, delimeters[i]) == 0) {
                    control = 1;
                    if (stringSize > 0) {
                        controlInfo.wordSize[stringSize]++;
                        controlInfo.vowelCount[numVowels][stringSize]++;
                        // totalNumberOfWords++;
                        // strcpy(stringBuffer, "");
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
                if (stringSize > controlInfo.maxWordSize) {
                    controlInfo.maxWordSize = stringSize;
                }
                // strcat(stringBuffer, completeSymbol);
            }

            // Increment number of vowels (if applicable)
            for (i = 0; i < sizeof(vowels) / sizeof(vowels[0]); i++) {
                if (strcmp(completeSymbol, vowels[i]) == 0) {
                    numVowels++;
                    if (numVowels > controlInfo.maxVowelCount) {
                        controlInfo.maxVowelCount = numVowels;
                    }
                    break;
                }
            }
        }
        // Consider last word of file
        if (stringSize > 0) {
            controlInfo.wordSize[stringSize]++;
            controlInfo.vowelCount[numVowels][stringSize]++;
            if (stringSize > controlInfo.maxWordSize) {
                controlInfo.maxWordSize = stringSize;
            }
            // totalNumberOfWords++;
            // strcpy(stringBuffer, "");
            numVowels = 0;
            stringSize = 0;
        }

        controlInfo.maxWordSize = controlInfo.maxWordSize + 1;
        controlInfo.maxVowelCount = controlInfo.maxVowelCount + 1;

        // Save chunk processing results
        savePartialResults(id, controlInfo);

        // Reset thread variables
        for (i = 0; i < MAXSIZE; i++) {
            controlInfo.wordSize[i] = 0;
        }
        for (i = 0; i < MAXSIZE; i++) {
            for (j = 0; j < MAXSIZE; j++) {
                controlInfo.vowelCount[i][j] = 0;
            }
        }
        controlInfo.maxWordSize = 0;
        controlInfo.maxVowelCount = 0;
        strcpy(textChunk, "");
    }

    // Free used memory
    // for (i = 0; i < MAXSIZE; i++) {
    //     free(vowelsCount[i]);
    // }
    // free(vowelsCount);

    statusWorker[id] = EXIT_SUCCESS;
    pthread_exit(&statusWorker[id]);
}