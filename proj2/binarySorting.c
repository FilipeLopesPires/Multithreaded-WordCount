#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void arrange(int n, char *x[]) {
    char *temp; // Pointer declaration
    int i,str;
    for(str = 0; str < n-1; ++str)
    {
        for(i = str+1; i < n; ++i)
        {
            if(strcmp(x[str],x[i]) > 0) //comparing the strings
            {
                temp = x[str]; // compared string being stored in temp
                x[str] = x[i];
                x[i] = temp;
            }
        }
    }
    return;
}

int main(int argc, char** argv) {
    int rank, size;
    char *message, *recMsg;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    message = malloc(sizeof(char) * 20);
    recMsg = malloc(sizeof(char) * 20);
    sprintf(message, "%s %d!", "I am here", rank);

    if (rank == 0) {
        printf("%d transmitted message: %s \n", rank, message);
        MPI_Send(message, strlen(message), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(recMsg, 100, MPI_CHAR, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d received message: %s \n", rank, recMsg);
    } else {
        MPI_Recv(recMsg, 100, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d received message: %s \n", rank, recMsg);

        char **words;
        char *curWord;
        curWord = malloc(sizeof(char) * 20);
        char *p;
        int nWords = 0;
        int j = 0;
        for (p = recMsg; *p != '\0'; p++) {
            if(*p != ",") {
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

        arrange(nWords,words);

        for (int i=0; i<nWords; i++){
            message = strcat(message,words[i]);
        }

        printf("%d transmitted message: %s \n", rank, message);
        MPI_Send(message, strlen(message), MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
    }
    MPI_Finalize();

    return 0;
}
