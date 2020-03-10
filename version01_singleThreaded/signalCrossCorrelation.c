#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char **argv) {

    // Validate program arguments
    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables
    FILE *file;
    int nElem;          // number of elements, given by the first 4 bytes of the processed file
    double *sig[3];     // array containing the values of the signals present in the file
    double curSig;      // auxiliar variable containing the current signal under analysis
    double *results;    // array containing the results of the similarity evaluation

    for (int fileIndex=1; fileIndex<argc; fileIndex++) {
        // Read file
        file = fopen(argv[fileIndex], "r");
        if (file == NULL) {
            // end of file error
            printf("Error while opening file!");
            exit(1);
        } 

        // Get number of elements
        fread(&nElem, 4, 1, file);
        printf("%d\n",nElem);

        // Process remainder of file (actual signals)
        for(int n=0; n<3; n++) {
            if((sig[n]=malloc(nElem*sizeof(double)))==NULL) {
                // malloc error message
                printf("Error while allocating memory for signal!");
                exit(1);
            }
            for(int i=0; i<nElem; i++) {
                //if(reaches EOF) {
                //    // file format error message
                //    ...
                //}
                fread(&curSig, 8, 1, file);
                sig[n][i] = curSig;
            }
        }

        // Allocate memory for results
        if((results=malloc(nElem*sizeof(double)))==NULL) {
            // malloc error message
            printf("Error while allocating memory for results!");
            exit(1);
        }

        // Calculate cross correlation 
        int mod;
        for(int t=0; t<nElem; t++) {
            double curSum = 0.0;
            for(int n=0; n<nElem; n++) {
                mod = (t+n) % nElem;
                curSum += sig[0][n]*sig[1][mod];
            }
            results[t] = curSum;
            if(results[t]==sig[2][t]) {
                printf("1");
            } else {
                printf("0");
            }
        }
        
        printf("\n");
        fclose(file);
    }

}