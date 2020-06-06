/**
 *  \file wordCount.c (implementation file)
 *
 *  \brief Multi-threaded implementation of the program that computes the
 * occurring frequency of word lengths and the number of vowels in each word for
 * texts given as input.
 *
 *  The program 'wordCount' reads in succession several text files text#.txt and
 * prints a listing of the occurring frequency of word lengths and the number of
 * vowels in each word for each of the supplied texts. In this implementation,
 * threads synchronization is based on monitors. Both threads and the monitor
 * are implemented using the pthread library which enables the creation of a
 * monitor of the Lampson / Redell type.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
//#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "controlInfo.h"
#include "textProcV2.h"
#include "wordCount.h"

#define BILLION 1000000000.0

/** \brief worker life cycle routine. */
static void *worker(void *id);

/** \brief worker threads return status array. */
int statusWorker[NUMWORKERS];

/** \brief main thread return status value. */
int statusMain;

/** \brief array containing all the characters defined as word delimiters. */
char delimeters[25][MAXCHARSIZE] = {
    " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
    "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

/** \brief array containing all the characters defined as word mergers. */
char mergers[7][MAXCHARSIZE] = {"‘", "’", "´", "`", "'", "ü", "Ü"};

/** \brief array containing all the possible vowels. */
char vowels[48][MAXCHARSIZE] = {
    "a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "á", "à",
    "ã", "â", "ä", "é", "è", "ẽ", "ê", "ë", "Á", "À", "Ã", "Â",
    "Ä", "É", "È", "Ẽ", "Ê", "Ë", "ó", "ò", "õ", "ô", "ö", "Ó",
    "Ò", "Õ", "Ô", "Ö", "í", "ì", "Í", "Ì", "ú", "ù", "Ú", "Ù"};

/**
 *  \brief Main function called when the program is executed.
 *
 *  Main function of the 'wordCount' program responsible for creating worker
 * threads and managing the monitor for delivering the desired results. The
 * function receives the paths to the text files.
 *
 *  \param argc number of files passed to the program.
 *  \param argv paths to the text files.
 *
 */
int main(int argc, char **argv) {
    // Validate number of arguments passed to the program

    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables

    /** \brief workers internal thread id array. */
    pthread_t workerThreadID[NUMWORKERS];

    /** \brief workers application defined thread id array. */
    unsigned int workerID[NUMWORKERS];

    /** \brief pointer to execution status. */
    int *status_p;

    /** \brief aux variable for local loops. */
    int i;

    /** \brief time limits (for execution time calculation). */
    struct timespec t0, t1;  // double t0, t1;

    // Initialization of thread IDs

    for (i = 0; i < NUMWORKERS; i++) {
        workerID[i] = i;
    }
    srandom((unsigned int)getpid());
    // t0 = ((double)clock()) / CLOCKS_PER_SEC;
    clock_gettime(CLOCK_REALTIME, &t0);

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

    // Execution of time calculation

    // t1 = ((double)clock()) / CLOCKS_PER_SEC;
    // printf("\nElapsed time = %.6f s\n", t1 - t0);

    clock_gettime(CLOCK_REALTIME, &t1);
    double exec_time =
        (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / BILLION;
    printf("\nElapsed time = %.6f s\n", exec_time);

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

    /** \brief worker ID. */
    int id;

    // char stringBuffer[MAXSIZE];             // buffer containing the current
    // word

    /** \brief number of characters in current word. */
    int stringSize;

    /** \brief  number of vowels in current word. */
    int numVowels;

    /** \brief current char under processing from the current chunk. */
    char symbol;

    /** \brief buffer for complex character construction. */
    char completeSymbol[MAXCHARSIZE];

    // int wordCount[MAXSIZE];                  // array containing the number
    // of words found whose size is equal to the respective index
    // // int vowelsCount[MAXSIZE][MAXSIZE];      // 2D array containing the
    // number of words found whose number of vowels and word size are equal to x
    // and y int **vowelsCount;                       // 2D array containing the
    // number of words found whose number of vowels and word size are equal to x
    // and y int localMaxWordSize;                    // largest word found in
    // the text chunk int localMaxVocalCount;                  // largest number
    // of vowels found in a word from the text chunk

    /** \brief auxiliar variable to count the number of ones in the most
     * significant bits of the current character. */
    int ones;

    /** \brief auxiliar variable to construct the complete symbol (used for
     * those who use more than 1 byte). */
    char nchar;

    /** \brief auxiliar variable to determine whether the program should update
     * 'numVowels' and 'stringSize' or not. */
    int control;

    /** \brief auxiliar variable to determine whether the program should update
     * 'stringSize' and 'stringBuffer' or not. */
    bool increment;

    /** \brief auxiliar variables for local loops. */
    int h, i, j;

    /** \brief structure containing control variables for the program
     * (wordSize[], vowelCount[][], maxWordSize, maxVowelCount, fileId). */
    struct controlInfo controlInfo;

    // Initialize thread variables

    id = *((int *)par);
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