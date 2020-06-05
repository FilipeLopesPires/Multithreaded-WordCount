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
#include <ctype.h>

#include "results.h"
#include "signal.h"

#define BILLION 1000000000.0

/**
 *  \brief Main function called when the program is executed.
 *
 *  Main function of the 'signalCrossCorrelation' program responsible for creating an MPI session and managing it to achieve the desired results. 
 *  The function receives the paths to the text files.
 *
 *  \param argc number of files passed to the program.
 *  \param argv paths to the signal files.
 *
 */
int main(int argc, char **argv) {

    // Prepare MPI

    int rank, numworkers;
    //int completed = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numworkers);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    struct timespec t0, t1;                 // time variables to calculate execution time
    clock_gettime(CLOCK_REALTIME, &t0);

    if (rank == 0) {    // DISPATCHER

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

        //bool areFilenamesPresented;
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
        bool stillExistsText = true;

        // Retrieval of filenames

        //printf("Dispatcher: Retrieving\n");
        char *fileNames[argc - optind];
        for (int i = optind; i < argc; i++) {
            fileNames[i - optind] = argv[i];
        }
        int size = argc - optind;
        if (size > 0) {
            currentFileIdx = 0; currentTau = -1;
            filesSize = size; filenames = fileNames;
            // Allocate memory
            if ((files = malloc(sizeof(FILE*) * (filesSize))) == NULL) {
                perror("Error while allocating memory in presentFilenames.\n"); exit(EXIT_FAILURE);
            } if ((globalResults = malloc(sizeof(double*) * (filesSize))) == NULL) {
                perror("Error while allocating memory in presentFilenames.\n"); exit(EXIT_FAILURE);
            } if ((signalSizes = malloc(sizeof(int) * (filesSize))) == NULL) {
                perror("Error while allocating memory in presentFilenames.\n"); exit(EXIT_FAILURE);
            }
            // Process files given as input
            for (int i = 0; i < size; i++) {
                files[i] = fopen(filenames[i], "r+");
                if (files[i] == NULL) {
                    printf("Error while opening file!\n"); exit(EXIT_FAILURE);
                }
                fread(&nElem, 4, 1, files[i]);
                signalSizes[i] = nElem;
                if ((globalResults[i] = malloc(sizeof(double) * (nElem))) == NULL) {
                    perror("Error while allocating memory in presentFilenames.\n"); exit(EXIT_FAILURE);
                }
            }
        }
        printf("Files presented.\n");

        // Process Worker Requests

        //int reqSignal;
        int signalMsgLen;
        //int resultsMsgLen;
        //char* signalMsg;
        //char* signalMsgCpy;
        //char* resultsMsg;
        int toBeIgnored[numworkers]; for(int i=0; i<numworkers; i++) { toBeIgnored[i] = -1; }

        struct signal signal;
        struct results results; 
        int i, j, n;
        //int control, ces;
        //char* curElemStr; 
        while(stillExistsText) {
            for(i=1; i<numworkers; i++) {
                //printf("Dispatcher: Waiting (%d)\n", i);
                //MPI_Recv(&reqSignal, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //printf("Dispatcher: Preparing\n");
                //struct signal signal;
                //struct results results; 
                results.fileId = 0; results.tau = 0; results.value = 0.0; // these values are never used, they avoid warnings
                if (currentFileIdx < filesSize) {
                    if (currentTau == -1) { 
                        for (n = 0; n < 2; n++) {
                            if ((currentFile[n] = malloc(signalSizes[currentFileIdx] * sizeof(double))) == NULL) {
                                perror("Error while allocating memory in getSignalAndTau.\n"); exit(EXIT_FAILURE);
                            }
                            for (j = 0; j < signalSizes[currentFileIdx]; j++) {
                                fread(&curSig, 8, 1, files[currentFileIdx]);
                                currentFile[n][j] = curSig;
                            }
                        }
                    }
                    results.fileId = currentFileIdx;
                    signal.values[0] = currentFile[0];
                    signal.values[1] = currentFile[1];
                    signal.signalSize = signalSizes[currentFileIdx];
                    currentTau++;
                    results.tau = currentTau;
                    signal.tau = currentTau;
                    if (currentTau > signalSizes[currentFileIdx]) {
                        currentTau = -1;
                        currentFileIdx++;
                    }
                }
                //printf("Dispatcher: Sending\n");
                if (currentFileIdx >= filesSize) {
                    stillExistsText = false;
                    char* emptySignalMsg = "";
                    signalMsgLen = strlen(emptySignalMsg);
                    MPI_Send(&signalMsgLen, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(emptySignalMsg, signalMsgLen, MPI_CHAR, i, 0, MPI_COMM_WORLD); 
                    toBeIgnored[i] = i;
                } else {
                    stillExistsText = true;
                    /*
                    signalMsg = malloc(100 * signal.signalSize * sizeof(char));
                    sprintf(signalMsg, "%d;[[%f", signal.signalSize, signal.values[0][0]);
                    signalMsgCpy = malloc(100 * signal.signalSize * sizeof(char));
                    strcpy(signalMsgCpy, signalMsg);
                    for(j=1; j<signal.signalSize; j++) {
                        sprintf(signalMsg, "%s,%f", signalMsgCpy, signal.values[0][j]);
                        strcpy(signalMsgCpy, signalMsg);
                    }
                    sprintf(signalMsg, "%s],[%f", signalMsgCpy, signal.values[1][0]);
                    strcpy(signalMsgCpy, signalMsg);
                    for(j=1; j<signal.signalSize; j++) {
                        sprintf(signalMsg, "%s,%f", signalMsgCpy, signal.values[1][j]);
                        strcpy(signalMsgCpy, signalMsg);
                    }
                    sprintf(signalMsg, "%s]];%d", signalMsgCpy, signal.tau);
                    signalMsgLen = strlen(signalMsg) + 1;
                    MPI_Send(&signalMsgLen, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(signalMsg, signalMsgLen, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                    */
                    MPI_Send(&signal.signalSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(signal.values[0], signal.signalSize, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
                    MPI_Send(signal.values[1], signal.signalSize, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
                    MPI_Send(&signal.tau, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

                    /*
                    resultsMsg = malloc(100 * sizeof(char));
                    sprintf(resultsMsg, "%d;%d;%f", results.fileId, results.tau, results.value);
                    resultsMsgLen = strlen(resultsMsg) + 1;
                    MPI_Send(&resultsMsgLen, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(resultsMsg, resultsMsgLen, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                    */
                    MPI_Send(&results.fileId, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(&results.tau, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(&results.value, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
                }
            }

            // Store partial results
            for(i=1; i<numworkers; i++) {
                if(toBeIgnored[i]>0) {
                    continue;
                }
                /*
                MPI_Recv(&resultsMsgLen, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                resultsMsg = malloc(resultsMsgLen * sizeof(char));
                MPI_Recv(resultsMsg, resultsMsgLen, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Dispatcher: Storing (%d)\n", i);
                control = 0; curElemStr = malloc(50 * sizeof(char)); ces = 0;
                for(int j=0; j<resultsMsgLen; j++) {
                    if(resultsMsg[j]==';') {
                        if(control==0) {
                            results.fileId = atoi(curElemStr);
                            control = 1;
                        } else {
                            results.tau = atoi(curElemStr);
                            char *ptr = resultsMsg; ptr = ptr+j+1;
                            char *eptr;
                            results.value = strtod(ptr,&eptr);
                        }
                        curElemStr = malloc(50 * sizeof(char));
                        ces = 0;
                    } else {
                        curElemStr[ces] = resultsMsg[j];
                        ces++;
                    }
                }
                */
                MPI_Recv(&results.fileId, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&results.tau, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&results.value, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                globalResults[results.fileId][results.tau] = results.value;
                //printf("%f \n",globalResults[results.fileId][results.tau]);
                if(!stillExistsText) {
                    char* emptySignalMsg = "";
                    signalMsgLen = strlen(emptySignalMsg);
                    MPI_Send(&signalMsgLen, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Send(emptySignalMsg, signalMsgLen, MPI_CHAR, i, 0, MPI_COMM_WORLD); 
                }
            }
        }
        
        for (i=0; i<filesSize; i++) {
            printf("Processing file %s\n", filenames[i]);

            int numErrors = 0;
            if (compareEnabled) {
                printf("Comparing one by one:\n");
                /*
                for (j = 0; j < signalSizes[i]; j++) {
                    fread(&curSig, 8, 1, files[i]);
                    if (curSig == globalResults[i][j]) {
                        printf("1");
                    } else {
                        numErrors++;
                        printf("0");
                        printf(" %d ", j);
                    }
                }
                */
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
        //printf("Dispatcher: Closing\n");

        clock_gettime(CLOCK_REALTIME, &t1);
        double exec_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / BILLION;
        printf("\nElapsed time = %.6f s\n", exec_time);

    } else {    // WORKERS

        //int reqSignal = 1;
        int msgLen;
        //int resultsMsgLen;
        //char* signalMsg;
        //char* resultsMsg;

        struct signal signal;
        struct results results;
        //int i, v, cvs, control, ces;
        ///bool middle;
        //char* signalSizeStr;
        //char* curValStr;
        //char* curElemStr;
        double curSum;
        int mod;

        // Receive signal

        //printf("Worker %d: Requesting\n", rank);
        //MPI_Send(&reqSignal, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        //printf("Worker %d: Waiting\n", rank);
        MPI_Recv(&msgLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        while(msgLen > 1) {
            /*
            signalMsg = malloc(msgLen * sizeof(char));
            MPI_Recv(signalMsg, msgLen, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Convert message to signal struct
            
            signalSizeStr = malloc(10 * sizeof(char));
            for(i=0; i<msgLen; i++) {
                if(signalMsg[i]==';') {
                    break;
                }
                signalSizeStr[i] = signalMsg[i];
            }
            signal.signalSize = atoi(signalSizeStr);
            
            curValStr = malloc(50 * sizeof(char));
            double values0[signal.signalSize]; double values1[signal.signalSize]; 
            middle = false; v = 0; cvs = 0; i = i+3;
            while(signalMsg[i]!=';') {
                if(signalMsg[i]==']') {
                    if(!middle) {
                        i = i+3; v = 0; middle = true;
                        signal.values[0] = values0;
                    } else {
                        i++;
                        signal.values[1] = values1;
                    }
                    continue;
                }
                if(signalMsg[i]==',') {
                    char *eptr;
                    if(!middle) {
                        values0[v] = strtod(curValStr,&eptr);
                    } else {
                        values1[v] = strtod(curValStr,&eptr);
                    }
                    i++; v = v+1; cvs = 0;
                } else {
                    curValStr[cvs] = signalMsg[i];
                    i++; cvs++; 
                }
            }
            char *ptr = signalMsg; ptr = ptr + i + 1;
            signal.tau = atoi(ptr);
            */
            signal.signalSize = msgLen;
            double* values0;
            if ((values0 = malloc(sizeof(double) * signal.signalSize)) == NULL) {
                perror("Error while allocating memory.\n");
                exit(1);
            }
            MPI_Recv(values0, signal.signalSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            double* values1;
            if ((values1 = malloc(sizeof(double) * signal.signalSize)) == NULL) {
                perror("Error while allocating memory.\n");
                exit(1);
            }
            MPI_Recv(values1, signal.signalSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            signal.values[0] = values0;
            signal.values[1] = values1;
            MPI_Recv(&signal.tau, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Convert message to results struct
            /*
            MPI_Recv(&msgLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            signalMsg = malloc(msgLen * sizeof(char));
            MPI_Recv(signalMsg, msgLen, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            control = 0; ces = 0;
            curElemStr = malloc(50 * sizeof(char));
            for(i=0; i<msgLen; i++) {
                if(signalMsg[i]==';') {
                    if(control==0) {
                        results.fileId = atoi(curElemStr);
                        control = 1;
                    } else {
                        results.tau = atoi(curElemStr);
                        char *ptr = signalMsg; ptr = ptr+i+1;
                        char *eptr;
                        results.value = strtod(ptr,&eptr);
                    }
                    curElemStr = malloc(50 * sizeof(char));
                    ces = 0;
                } else {
                    curElemStr[ces] = signalMsg[i];
                    ces++;
                }
            }
            */
            MPI_Recv(&results.fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&results.tau, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&results.value, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Process signal

            //printf("Worker %d: Processing\n", rank);
            curSum = 0.0;
            for (int n = 0; n < signal.signalSize; n++) {
                mod = (results.tau + n) % signal.signalSize;
                curSum += signal.values[0][n] * signal.values[1][mod];
            }
            results.value = curSum;
            //printf("Worker %d: Sending\n", rank);
            /*
            resultsMsg = malloc(100 * sizeof(char));
            sprintf(resultsMsg, "%d;%d;%f", results.fileId, results.tau, results.value);
            resultsMsgLen = strlen(resultsMsg) + 1;
            MPI_Send(&resultsMsgLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(resultsMsg, resultsMsgLen, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            */
            MPI_Send(&results.fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&results.tau, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&results.value, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

            //printf("Worker %d: Requesting\n", rank);
            //reqSignal = 1;
            //MPI_Send(&reqSignal, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            //printf("Worker %d: Waiting\n", rank);
            MPI_Recv(&msgLen, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } 
        //printf("Worker %d: Closing\n", rank);
        printf("Worker, with id %d, has successfully terminated.\n", rank);
    }
    MPI_Finalize();

    exit(EXIT_SUCCESS);

}