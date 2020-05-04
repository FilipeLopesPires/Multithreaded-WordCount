#include <math.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void arrange(int n, char* x[]) {
    char* temp;  // Pointer declaration
    int i, str;
    for (str = 0; str < n - 1; ++str) {
        for (i = str + 1; i < n; ++i) {
            if (strcmp(x[str], x[i]) > 0)  // comparing the strings
            {
                temp = x[str];  // compared string being stored in temp
                x[str] = x[i];
                x[i] = temp;
            }
        }
    }
    return;
}

int main(int argc, char** argv) {
    int rank, size;
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    char** names = NULL;
    int currSize = 0;
    int currentIdx = 0;
    char* recMsg = malloc(sizeof(char));

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        fp = fopen("names.txt", "r");
        if (fp == NULL) exit(EXIT_FAILURE);
        while (getline(&line, &len, fp) != -1) {
            names = (char**)realloc(names, (currSize + 1) * sizeof(char*));
            if (names == NULL) {
                printf("ERROR\n");
                exit(EXIT_FAILURE);
            }
            names[currSize] = malloc(1 * sizeof(char));
            strcpy(names[currSize], strtok(line, "\n"));
            currSize++;
        }
        fclose(fp);
        if (line) free(line);

        int amountPerWorker = ceil((double)currSize / (size - 1));

        char* message = malloc(1 * sizeof(char));
        for (int i = 1; i < size; i++) {
            strcpy(message, "");
            for (int j = currentIdx;
                 j < currentIdx + amountPerWorker && j < currSize; j++) {
                strcat(message, names[j]);
                strcat(message, ",");
            }
            currentIdx += amountPerWorker;
            MPI_Send(message, strlen(message), MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

        size--;
        char** preordered = malloc(size * sizeof(char*));
        for (int i = 0; i < size; i++) {
            preordered[i] = malloc(sizeof(char));
        }

        for (int i = 0; i < size; i++) {
            MPI_Recv(recMsg, 1000, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            strcpy(preordered[i], recMsg);
        }

        while (true) {
            int newSize = ceil((double)size / 2);
            if (newSize == size) {
                break;
            }
            int control = 0;
            int preorderIdx = 0;
            for (int i = 0; i < size; i++) {
                if (control >= newSize) {
                    strcpy(message, "done");
                    MPI_Send(message, strlen(message), MPI_CHAR, i + 1, 0,
                             MPI_COMM_WORLD);
                } else {
                    strcpy(message, preordered[preorderIdx]);
                    preorderIdx++;
                    if (preorderIdx < size) {
                        strcat(message, preordered[preorderIdx]);
                    }
                    MPI_Send(message, strlen(message), MPI_CHAR, i + 1, 0,
                             MPI_COMM_WORLD);
                }
                control++;
            }
            size = newSize;
            free(preordered);
            char** preordered = malloc(size * sizeof(char*));
            for (int i = 0; i < size; i++) {
                preordered[i] = malloc(sizeof(char));
            }
            for (int i = 0; i < size; i++) {
                MPI_Recv(recMsg, 1000, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                strcpy(preordered[i], recMsg);
            }
        }

    } else {
        MPI_Recv(recMsg, 100, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("%d received message: %s \n", rank, recMsg);

        char** words;
        char* curWord;
        curWord = malloc(sizeof(char) * 20);
        char* p;
        int nWords = 0;
        int j = 0;
        for (p = recMsg; *p != '\0'; p++) {
            if (*p != ",") {
                curWord[j] = *p;
                j++;
            } else {
                words[nWords] = curWord;
                nWords++;
                curWord = malloc(sizeof(char) * 20);
                j = 0;
            }
        }
        words[nWords] = curWord;
        nWords++;

        arrange(nWords, words);

        for (int i = 0; i < nWords; i++) {
            message = strcat(message, words[i]);
        }

        printf("%d transmitted message: %s \n", rank, message);
        MPI_Send(message, strlen(message), MPI_CHAR, (rank + 1) % size, 0,
                 MPI_COMM_WORLD);
    }
    MPI_Finalize();

    return 0;
}
