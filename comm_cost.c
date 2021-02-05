#include "mpi.h"
#include<stdio.h>
#include <stdlib.h>
#include "math.h"
#include <stdbool.h>
#define SIZE 50000

int main(int argc, char *argv[]) {
    int unsorted_array[SIZE]; 
    int array_size = SIZE;
    int size, rank;
    // Start Parallel Execution
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank==0){
        // --- RANDOM ARRAY GENERATION ---
        printf("Creating Random List of %d elements\n", SIZE);
        int j = 0;
        for (j = 0; j < SIZE; ++j) {
            unsorted_array[j] =(int) rand() % 1000;
        }
        printf("Created\n");
	}
    MPI_Status status;
    int rankPower = 0;
    while (pow(2, rankPower) <= rank){
        rankPower++;
    }
    int shareProc = rank + pow(2, rankPower);

    double start_timer, finish_timer;
    if (rank == 0) {
	start_timer = MPI_Wtime();
        MPI_Send(unsorted_array, array_size , MPI_INT, 1, 1, MPI_COMM_WORLD);
        MPI_Recv(unsorted_array, array_size , MPI_INT, 1, 1, MPI_COMM_WORLD,  &status);
    }else{ 
        // All other Clusters wait for their subarray to arrive,
        // they sort it and they send it back.
        //MPI_Status status; 
        int rec_array_size;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // Capturing size of the array to receive
        MPI_Get_count(&status, MPI_INT, &rec_array_size);
	    //int source_process = status.MPI_SOURCE;     
        int unsorted_array2[rec_array_size];
        MPI_Recv(unsorted_array2, rec_array_size, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Send(unsorted_array2, rec_array_size, MPI_INT, 0, 1, MPI_COMM_WORLD);
    };
    
    if(rank==0){
        finish_timer = MPI_Wtime();
	    printf("Total comm time forward and back : %2.4f sec \n", finish_timer-start_timer);       
    }
       
    MPI_Finalize();
    // End of Parallel Execution
    return 0;
}
