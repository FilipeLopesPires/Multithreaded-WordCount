#include <errno.h>
#include <limits.h>
#include <math.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/** \brief maximum size possible for a word. */
#define MAXSIZE 50

/** \brief maximum size (number of bytes) possible for a character. */
#define MAXCHARSIZE 6

/** \brief memory space (number of bytes) available for words under processing.
 */
#define BUFFERSIZE 1000

/** \brief auxiliary variable for internal calculations. */
#define BILLION 1000000000.0

/** \brief array containing all the characters defined as word delimiters. */
char delimiters[25][MAXCHARSIZE] = {
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

// Declare useful variables

struct timespec t0, t1;  // time variables to calculate execution time

int** wordSizeResults;
int*** vowelCountResults;
int* numberWordsResults;
int* maximumSizeWordResults;

char tmpWord[MAXSIZE] = "";
int currentFileIdx = 0;
char symbol;
char completeSymbol[MAXCHARSIZE];  // buffer for complex character
                                   // construction
int ones;
bool incrementFileIdx;

int filesSize;
FILE** files;
char** filenames;

int totalNumWorkers = 0;

int maxWordSize = 0;
int maxVowelCount = 0;
int fileId = -1;

bool getTextChunk(char* textChunk, int* fileId) {
    bool stillExistsText;
    strcat(textChunk, tmpWord);
    strcpy(tmpWord, "");
    if (currentFileIdx < filesSize) {
        while (strlen(textChunk) < BUFFERSIZE) {
            symbol = getc(files[currentFileIdx]);
            // Verify if the current file has ended
            if (symbol == EOF) {
                if (strlen(tmpWord) + strlen(textChunk) < BUFFERSIZE) {
                    strcat(textChunk, tmpWord);
                    strcpy(tmpWord, "");
                }
                incrementFileIdx = true;
                break;
            }
            strcpy(completeSymbol, "");
            ones = 0;

            // Verify the number of 1s in the most significant bits
            for (int i = sizeof(symbol) * CHAR_BIT - 1; i >= 0; --i) {
                int bit = (symbol >> i) & 1;
                if (bit == 0) {
                    break;
                }
                ones++;
            }

            // Build the complete character (if it consists of more than
            // 1byte)
            strncat(completeSymbol, &symbol, 1);
            for (int i = 1; i < ones; i++) {
                symbol = getc(files[currentFileIdx]);
                strncat(completeSymbol, &symbol, 1);
            }

            // Check if character is a delimiter
            bool leaveLoop = false;
            for (int i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]);
                 i++) {
                if (strcmp(completeSymbol, delimiters[i]) == 0) {
                    if (strlen(tmpWord) + strlen(textChunk) < BUFFERSIZE) {
                        strcat(textChunk, tmpWord);
                        strcpy(tmpWord, "");
                    } else {
                        leaveLoop = true;
                    }
                    break;
                }
            }

            strcat(tmpWord, completeSymbol);
            if (leaveLoop) {
                break;
            }
        }
    }
    *fileId = currentFileIdx;
    if (strlen(textChunk) <= 0) {
        stillExistsText = false;
    } else {
        stillExistsText = true;
    }

    if (incrementFileIdx) {
        currentFileIdx++;
        incrementFileIdx = false;
    }
    return stillExistsText;
}

void initVariables(int argc, char** argv) {
    filesSize = argc - 1;
    // Allocate memory
    if ((files = malloc(sizeof(FILE*) * (filesSize))) == NULL) {
        perror("Error while allocating memory.\n");
        exit(1);
    }

    if ((filenames = malloc(sizeof(char*) * (filesSize))) == NULL) {
        perror("Error while allocating memory.\n");
        exit(1);
    }

    wordSizeResults = malloc(sizeof(int*) * (filesSize));
    vowelCountResults = malloc(sizeof(int*) * (filesSize));
    maximumSizeWordResults = malloc(sizeof(int) * (filesSize));
    numberWordsResults = malloc(sizeof(int) * (filesSize));
    if (wordSizeResults == NULL || vowelCountResults == NULL ||
        maximumSizeWordResults == NULL || numberWordsResults == NULL) {
        perror("Error while allocating memory.\n");
        exit(1);
    }

    // Process files given as input
    for (int i = 0; i < filesSize; i++) {
        maximumSizeWordResults[i] = 0;
        numberWordsResults[i] = 0;
        wordSizeResults[i] = malloc(sizeof(int) * (MAXSIZE));
        vowelCountResults[i] = malloc(sizeof(int*) * (MAXSIZE));
        if (wordSizeResults[i] == NULL || vowelCountResults[i] == NULL) {
            perror("Error while allocating memory.\n");
            exit(1);
        }
        for (int j = 0; j < MAXSIZE; j++) {
            wordSizeResults[i][j] = 0;
            if ((vowelCountResults[i][j] = malloc(sizeof(int) * (MAXSIZE))) ==
                NULL) {
                perror("Error while allocating memory in.\n");
                exit(1);
            }
            for (int l = 0; l < MAXSIZE; l++) {
                vowelCountResults[i][j][l] = 0;
            }
        }
    }

    for (int i = 1; i < argc; i++) {
        filenames[i - 1] = argv[i];
        files[i - 1] = fopen(argv[i], "r");
        if (files[i - 1] == NULL) {
            // end of file error
            printf("Error while opening file!\n");
            exit(1);
        }
    }
    printf("Files presented.\n");
}

void printResults() {
    for (int k = 0; k < filesSize; k++) {
        printf("File name: %s\n", filenames[k]);
        printf("Total number of words: %d\n", numberWordsResults[k]);
        printf("Word length\n");

        printf("   ");
        for (int i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6d", i);
        }
        printf("\n   ");
        for (int i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6d", wordSizeResults[k][i]);
        }
        printf("\n   ");
        for (int i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6.2f", ((float)wordSizeResults[k][i] * 100.0) /
                                (float)numberWordsResults[k]);
        }
        for (int i = 0; i < maximumSizeWordResults[k] + 1; i++) {
            printf("\n%2d ", i);
            for (int j = 1; j < i; j++) {
                printf("%6s", " ");
            }
            if (i == 0) {
                for (int j = 1; j < maximumSizeWordResults[k] + 1; j++) {
                    if (wordSizeResults[k][j] > 0) {
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) /
                                            (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            } else {
                for (int j = i; j < maximumSizeWordResults[k] + 1; j++) {
                    if (wordSizeResults[k][j] > 0) {
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) /
                                            (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            }
        }
        printf("\n\n");
    }
}

/**
 *  \brief Main function called when the program is executed.
 *
 *  Main function of the 'wordCount' program responsible for creating an MPI
 * session and managing it to achieve the desired results. The function receives
 * the paths to the text files.
 *
 *  \param argc number of files passed to the program.
 *  \param argv paths to the signal files.
 *
 */
int main(int argc, char** argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    totalNumWorkers = size - 1;

    // Validate number of workers
    if(totalNumWorkers<1) {
        printf("The program needs at least one worker!\n");
        exit(1);
    }

    if (rank == 0) {
        srandom((unsigned int)getpid());
        // Validate number of arguments passed to the program
        if (argc <= 1) {
            printf("The program needs at least one text file to parse!\n");
            exit(1);
        }

        initVariables(argc, argv);

        // t0 = ((double)clock()) / CLOCKS_PER_SEC;
        clock_gettime(CLOCK_REALTIME, &t0);

        // process files
        char textChunk[BUFFERSIZE] = "";
        int fileId = -1;
        bool continueProcess = true;

        while (continueProcess) {
            int workingWorkers = 0;
            for (int workerId = 1; workerId <= totalNumWorkers; workerId++) {
                continueProcess = getTextChunk(textChunk, &fileId);

                if (continueProcess) {
                    workingWorkers++;
                    MPI_Send(&fileId, 1, MPI_INT, workerId, 0, MPI_COMM_WORLD);
                    int chunkSize = strlen(textChunk);
                    MPI_Send(&chunkSize, 1, MPI_INT, workerId, 0,
                             MPI_COMM_WORLD);
                    MPI_Send(textChunk, chunkSize, MPI_CHAR, workerId, 0,
                             MPI_COMM_WORLD);
                }

                strcpy(textChunk, "");
                fileId = -1;
            }
            for (int workerId = 1; workerId <= workingWorkers; workerId++) {
                int* wordSizes;
                MPI_Recv(&fileId, 1, MPI_INT, workerId, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                MPI_Recv(&maxWordSize, 1, MPI_INT, workerId, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                MPI_Recv(&maxVowelCount, 1, MPI_INT, workerId, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if ((wordSizes = malloc(sizeof(int) * maxWordSize)) == NULL) {
                    perror("Error while allocating memory.\n");
                    exit(1);
                }
                MPI_Recv(wordSizes, maxWordSize, MPI_INT, workerId, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int* vowelCounts;
                if ((vowelCounts = malloc(sizeof(int) * maxWordSize)) == NULL) {
                    perror("Error while allocating memory.\n");
                    exit(1);
                }
                for (int i = 0; i < maxWordSize; i++) {
                    wordSizeResults[fileId][i] += wordSizes[i];
                    numberWordsResults[fileId] += wordSizes[i];
                    if (i > maximumSizeWordResults[fileId] &&
                        wordSizes[i] > 0) {
                        maximumSizeWordResults[fileId] = i;
                    }
                }
                for (int i = 0; i < maxVowelCount; i++) {
                    MPI_Recv(vowelCounts, maxWordSize, MPI_INT, workerId, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int j = 0; j < maxWordSize; j++) {
                        vowelCountResults[fileId][i][j] += vowelCounts[j];
                    }
                }

                free(wordSizes);
                free(vowelCounts);
            }
        }
        int endSignal = -1;
        for (int workerId = 1; workerId <= totalNumWorkers; workerId++) {
            MPI_Send(&endSignal, 1, MPI_INT, workerId, 0, MPI_COMM_WORLD);
        }

        printResults();

        clock_gettime(CLOCK_REALTIME, &t1);
        double exec_time =
            (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / BILLION;
        printf("\nElapsed time = %.6f s\n", exec_time);

    } else {
        int chunkSize = 0;
        char* textChunk;
        char completeSymbol[MAXCHARSIZE];
        bool increment;
        int control;
        int numVowels = 0;
        int stringSize = 0;

        int* wordSize = malloc(sizeof(int) * (MAXSIZE));
        int** vowelCount = malloc(sizeof(int*) * (MAXSIZE));
        if (wordSize == NULL || vowelCount == NULL) {
            perror("Error while allocating memory.\n");
            exit(1);
        }

        // Process files given as input
        for (int j = 0; j < MAXSIZE; j++) {
            wordSize[j] = 0;
            if ((vowelCount[j] = malloc(sizeof(int) * (MAXSIZE))) == NULL) {
                perror("Error while allocating memory in.\n");
                exit(1);
            }
            for (int l = 0; l < MAXSIZE; l++) {
                vowelCount[j][l] = 0;
            }
        }

        while (chunkSize != -1) {
            MPI_Recv(&fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (fileId == -1) {
                break;
            }
            MPI_Recv(&chunkSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            if ((textChunk = malloc(chunkSize * sizeof(char))) == NULL) {
                perror("Error while allocating memory.\n");
                exit(1);
            }

            MPI_Recv(textChunk, chunkSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            // process text chunk
            for (int h = 0; h < strlen(textChunk); h++) {
                symbol = textChunk[h];
                strcpy(completeSymbol, "");
                increment = true;
                ones = 0;

                // Verify the number of 1s in the most significant bits

                for (int i = sizeof(symbol) * CHAR_BIT - 1; i >= 0; --i) {
                    int bit = (symbol >> i) & 1;
                    if (bit == 0) {
                        break;
                    }
                    ones++;
                }

                // Build the complete character (if it consists of more than 1
                // byte)

                strncat(completeSymbol, &symbol, 1);
                for (int i = 1; i < ones; i++) {
                    h++;
                    symbol = textChunk[h];
                    strncat(completeSymbol, &symbol, 1);
                }

                // Check if character is a delimiter

                for (int i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]);
                     i++) {
                    if (strcmp(completeSymbol, delimiters[i]) == 0) {
                        control = 1;
                        if (stringSize > 0) {
                            wordSize[stringSize]++;
                            vowelCount[numVowels][stringSize]++;
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

                for (int i = 0; i < sizeof(mergers) / sizeof(mergers[0]); i++) {
                    if (strcmp(completeSymbol, mergers[i]) == 0) {
                        increment = false;
                        break;
                    }
                }
                if (increment) {
                    stringSize++;
                    if (stringSize > maxWordSize) {
                        maxWordSize = stringSize;
                    }
                }

                // Increment number of vowels (if applicable)

                for (int i = 0; i < sizeof(vowels) / sizeof(vowels[0]); i++) {
                    if (strcmp(completeSymbol, vowels[i]) == 0) {
                        numVowels++;
                        if (numVowels > maxVowelCount) {
                            maxVowelCount = numVowels;
                        }
                        break;
                    }
                }
            }

            // Consider last word of file

            if (stringSize > 0) {
                wordSize[stringSize]++;
                vowelCount[numVowels][stringSize]++;
                if (stringSize > maxWordSize) {
                    maxWordSize = stringSize;
                }
                numVowels = 0;
                stringSize = 0;
            }

            maxWordSize = maxWordSize + 1;
            maxVowelCount = maxVowelCount + 1;

            free(textChunk);

            // Save chunk processing results
            MPI_Send(&fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&maxWordSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&maxVowelCount, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            // send wordSizes
            MPI_Send(wordSize, maxWordSize, MPI_INT, 0, 0, MPI_COMM_WORLD);

            // send vowelCounts
            for (int i = 0; i < maxVowelCount; i++) {
                MPI_Send(vowelCount[i], maxWordSize, MPI_INT, 0, 0,
                         MPI_COMM_WORLD);
            }

            // Reset thread variables

            for (int i = 0; i < MAXSIZE; i++) {
                wordSize[i] = 0;
            }
            for (int i = 0; i < MAXSIZE; i++) {
                for (int j = 0; j < MAXSIZE; j++) {
                    vowelCount[i][j] = 0;
                }
            }
            maxWordSize = 0;
            maxVowelCount = 0;
        }
        // printf("Worker, with id %d, has successfully terminated.\n", rank);
    }

    MPI_Finalize();

    exit(EXIT_SUCCESS);

    // return 0;
}
