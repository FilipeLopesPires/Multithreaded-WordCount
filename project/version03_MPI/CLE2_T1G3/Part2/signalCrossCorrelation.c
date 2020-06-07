/**
 *  \file signalCrossCorrelation.c (implementation file)
 *
 *  \brief Multi-threaded implementation of the program that computes the
 * circular cross-correlation of the values of two signals.
 *
 *  The program 'signalCrossCorrelation' reads in succession the values of pairs
 * of signals stored in several data files whose names are provided, computes
 * the circular cross-correlation of each pair and appends it to the
 * corresponding file. In this implementation, threads synchronization is based
 * on monitors. Both threads and the monitor are implemented using the pthread
 * library which enables the creation of a monitor of the Lampson / Redell type.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
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

#define BILLION 1000000000.0

#define DEBUGGING false

/**
 *  \brief Main function called when the program is executed.
 *
 *  Main function of the 'signalCrossCorrelation' program responsible for
 * creating an MPI session and managing it to achieve the desired results. The
 * function receives the paths to the text files.
 *
 *  \param argc number of files passed to the program.
 *  \param argv paths to the signal files.
 *
 */
int main(int argc, char** argv) {
    // Prepare MPI

    int rank, size;
    // int completed = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int totalNumWorkers = size - 1;

    // Validate number of workers
    if (totalNumWorkers < 1) {
        printf("The program needs at least one worker!\n");
        exit(1);
    }

    struct timespec t0, t1;  // time variables to calculate execution time
    clock_gettime(CLOCK_REALTIME, &t0);

    if (rank == 0) {  // DISPATCHER

        // Validate program arguments

        int opt;
        bool compareEnabled = false;

        while ((opt = getopt(argc, argv, "c")) != -1) {
            switch (opt) {
                case 'c':
                    compareEnabled = true;
                    break;
            }
        }

        if ((argc - optind) < 1) {
            printf("The program need at least one text file to parse!\n");
            exit(1);
        }

        // bool areFilenamesPresented;
        FILE** files;
        char** filenames;
        int filesSize;
        int currentFileIdx;
        double** globalResults;
        int nElem;
        int currentTau;
        double* currentFile[2];
        int* signalSizes;
        double curSig;

        // Retrieval of filenames

        if (DEBUGGING) {
            printf("Dispatcher: Retrieving\n");
        }
        char* fileNames[argc - optind];
        for (int i = optind; i < argc; i++) {
            fileNames[i - optind] = argv[i];
        }
        int size = argc - optind;
        if (size > 0) {
            currentFileIdx = 0;
            currentTau = -1;
            filesSize = size;
            filenames = fileNames;
            // Allocate memory
            if ((files = malloc(sizeof(FILE*) * (filesSize))) == NULL) {
                perror("Error while allocating memory in presentFilenames.\n");
                exit(EXIT_FAILURE);
            }
            if ((globalResults = malloc(sizeof(double*) * (filesSize))) ==
                NULL) {
                perror("Error while allocating memory in presentFilenames.\n");
                exit(EXIT_FAILURE);
            }
            if ((signalSizes = malloc(sizeof(int) * (filesSize))) == NULL) {
                perror("Error while allocating memory in presentFilenames.\n");
                exit(EXIT_FAILURE);
            }
            // Process files given as input
            for (int i = 0; i < size; i++) {
                files[i] = fopen(filenames[i], "r+");
                if (files[i] == NULL) {
                    printf("Error while opening file!\n");
                    exit(EXIT_FAILURE);
                }
                fread(&nElem, 4, 1, files[i]);
                signalSizes[i] = nElem;
                if ((globalResults[i] = malloc(sizeof(double) * (nElem))) ==
                    NULL) {
                    perror(
                        "Error while allocating memory in presentFilenames.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        printf("Files presented.\n");

        // Process Worker Requests

        int i, j, n;

        int receivedTau, receivedFileId, receivedWorkerId;
        double receivedValue;

        int workingWorkers = 0;

        for (n = 0; n < 2; n++) {
            if ((currentFile[n] = malloc(signalSizes[currentFileIdx] *
                                         sizeof(double))) == NULL) {
                perror(
                    "Error while allocating memory in getting "
                    "signal.\n");
                exit(EXIT_FAILURE);
            }
            for (j = 0; j < signalSizes[currentFileIdx]; j++) {
                fread(&curSig, 8, 1, files[currentFileIdx]);
                currentFile[n][j] = curSig;
            }
        }

        for (i = 1; i <= totalNumWorkers; i++) {  // i == worker id
            currentTau++;
            if (currentTau > signalSizes[currentFileIdx]) {
                currentTau = 0;
                currentFileIdx++;
                if (currentFileIdx < filesSize) {
                    for (n = 0; n < 2; n++) {
                        free(currentFile[n]);
                        if ((currentFile[n] =
                                 malloc(signalSizes[currentFileIdx] *
                                        sizeof(double))) == NULL) {
                            perror(
                                "Error while allocating memory in getting "
                                "signal.\n");
                            exit(EXIT_FAILURE);
                        }
                        for (j = 0; j < signalSizes[currentFileIdx]; j++) {
                            fread(&curSig, 8, 1, files[currentFileIdx]);
                            currentFile[n][j] = curSig;
                        }
                    }
                } else {
                    break;
                }
            }
            workingWorkers++;

            if (DEBUGGING) {
                printf("Dispatcher: Sending\n");
            }
            MPI_Send(&currentFileIdx, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&signalSizes[currentFileIdx], 1, MPI_INT, i, 0,
                     MPI_COMM_WORLD);
            MPI_Send(currentFile[0], signalSizes[currentFileIdx], MPI_DOUBLE, i,
                     0, MPI_COMM_WORLD);
            MPI_Send(currentFile[1], signalSizes[currentFileIdx], MPI_DOUBLE, i,
                     0, MPI_COMM_WORLD);
            MPI_Send(&currentTau, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        while (workingWorkers > 0) {
            if (DEBUGGING) {
                printf("Dispatcher: Receiving & Storing\n");
            }
            // Store partial results
            MPI_Recv(&receivedWorkerId, 1, MPI_INT, MPI_ANY_SOURCE, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&receivedFileId, 1, MPI_INT, receivedWorkerId, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&receivedTau, 1, MPI_INT, receivedWorkerId, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&receivedValue, 1, MPI_DOUBLE, receivedWorkerId, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            globalResults[receivedFileId][receivedTau] = receivedValue;
            workingWorkers--;
            // printf("%f \n",globalResults[results.fileId][results.tau]);

            currentTau++;
            if (currentTau > signalSizes[currentFileIdx]) {
                currentTau = 0;
                currentFileIdx++;
                if (currentFileIdx < filesSize) {
                    for (n = 0; n < 2; n++) {
                        free(currentFile[n]);
                        if ((currentFile[n] =
                                 malloc(signalSizes[currentFileIdx] *
                                        sizeof(double))) == NULL) {
                            perror(
                                "Error while allocating memory in getting "
                                "signal.\n");
                            exit(EXIT_FAILURE);
                        }
                        for (j = 0; j < signalSizes[currentFileIdx]; j++) {
                            fread(&curSig, 8, 1, files[currentFileIdx]);
                            currentFile[n][j] = curSig;
                        }
                    }
                } else {
                    continue;
                }
            }

            if (DEBUGGING) {
                printf("Dispatcher: Sending\n");
            }
            MPI_Send(&currentFileIdx, 1, MPI_INT, receivedWorkerId, 0,
                     MPI_COMM_WORLD);
            MPI_Send(&signalSizes[currentFileIdx], 1, MPI_INT, receivedWorkerId,
                     0, MPI_COMM_WORLD);
            MPI_Send(currentFile[0], signalSizes[currentFileIdx], MPI_DOUBLE,
                     receivedWorkerId, 0, MPI_COMM_WORLD);
            MPI_Send(currentFile[1], signalSizes[currentFileIdx], MPI_DOUBLE,
                     receivedWorkerId, 0, MPI_COMM_WORLD);
            MPI_Send(&currentTau, 1, MPI_INT, receivedWorkerId, 0,
                     MPI_COMM_WORLD);
            workingWorkers++;
        }

        if (DEBUGGING) {
            printf("Dispatcher: Ending\n");
        }
        int endSignal = -1;
        for (int workerId = 1; workerId < totalNumWorkers + 1; workerId++) {
            MPI_Send(&endSignal, 1, MPI_INT, workerId, 0, MPI_COMM_WORLD);
        }

        for (i = 0; i < filesSize; i++) {
            printf("Processing file %s\n", filenames[i]);

            int numErrors = 0;
            if (compareEnabled) {
                printf("Comparing one by one:\n");
                for (j = 0; j < signalSizes[i]; j++) {
                    fread(&curSig, 8, 1, files[i]);
                    if (curSig != globalResults[i][j]) {
                        numErrors++;
                    }
                }
                printf("\nNum. Errors Found: %d\n", numErrors);
            } else {
                for (j = 0; j < signalSizes[i]; j++) {
                    fwrite(&globalResults[i][j], 8, 1, files[i]);
                }
            }
        }

        free(files);
        free(globalResults);
        free(signalSizes);

        clock_gettime(CLOCK_REALTIME, &t1);
        double exec_time =
            (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / BILLION;
        printf("\nElapsed time = %.6f s\n", exec_time);

    } else {  // WORKERS

        int msgLen = 1;
        int newMsgLen;

        double curSum;
        int mod;
        double* values0;
        double* values1;
        int tau;

        int fileId;

        // Receive signal

        while (msgLen > 0) {
            if (DEBUGGING) {
                printf("Worker %d: Receiving\n", rank);
            }
            MPI_Recv(&fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (fileId == -1) {
                if (DEBUGGING) {
                    printf("Worker %d: Ending\n", rank);
                }
                break;
            }

            MPI_Recv(&newMsgLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (newMsgLen != msgLen) {
                if (msgLen != 1) {
                    free(values0);
                    free(values1);
                }
                if ((values0 = malloc(sizeof(double) * newMsgLen)) == NULL) {
                    perror("Error while allocating memory.\n");
                    exit(1);
                }

                if ((values1 = malloc(sizeof(double) * newMsgLen)) == NULL) {
                    perror("Error while allocating memory.\n");
                    exit(1);
                }
            }
            msgLen = newMsgLen;

            MPI_Recv(values0, msgLen, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            MPI_Recv(values1, msgLen, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Recv(&tau, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Process signal

            if (DEBUGGING) {
                printf("Worker %d: Processing\n", rank);
            }
            curSum = 0.0;
            for (int n = 0; n < msgLen; n++) {
                mod = (tau + n) % msgLen;
                curSum += values0[n] * values1[mod];
            }

            MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&tau, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&curSum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
        // printf("Worker, with id %d, has successfully terminated.\n", rank);
    }
    MPI_Finalize();

    exit(EXIT_SUCCESS);
}