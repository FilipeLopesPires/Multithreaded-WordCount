#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        MPI_Recv(recMsg, 100, MPI_CHAR, size - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("%d received message: %s \n", rank, recMsg);
    } else {
        MPI_Recv(recMsg, 100, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("%d received message: %s \n", rank, recMsg);
        printf("%d transmitted message: %s \n", rank, message);
        MPI_Send(message, strlen(message), MPI_CHAR, (rank + 1) % size, 0,
                 MPI_COMM_WORLD);
    }
    MPI_Finalize();

    return 0;
}
