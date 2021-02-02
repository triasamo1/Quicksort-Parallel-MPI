#include "mpi.h"
#include<stdio.h>
#include <stdlib.h>
#include "math.h"
#include <stdbool.h>
#define SIZE 1000000

/*
    Divides the array given into two partitions
        - Lower than pivot
        - Higher than pivot
    and returns the Pivot index in the array
*/
int partition(int *arr, int low, int high){
    int pivot = arr[high];
    int i = (low - 1);
    int j,temp;
    for (j=low;j<=high-1;j++){
	if(arr[j] < pivot){
	     i++;
             temp=arr[i];  
             arr[i]=arr[j];
             arr[j]=temp;	
	}
    }
    temp=arr[i+1];  
    arr[i+1]=arr[high];
    arr[high]=temp; 
    return (i+1);
}

/*
    Hoare Partition - Starting pivot is the middle point
    Divides the array given into two partitions
        - Lower than pivot
        - Higher than pivot
    and returns the Pivot index in the array
*/
int hoare_partition(int *arr, int low, int high){
    int middle = floor((low+high)/2);
    int pivot = arr[middle];
    int j,temp;
    // move pivot to the end
    temp=arr[middle];  
    arr[middle]=arr[high];
    arr[high]=temp;

    int i = (low - 1);
    for (j=low;j<=high-1;j++){
        if(arr[j] < pivot){
            i++;
            temp=arr[i];  
            arr[i]=arr[j];
            arr[j]=temp;	
        }
    }
    // move pivot back
    temp=arr[i+1];  
    arr[i+1]=arr[high];
    arr[high]=temp; 

    return (i+1);
}

/*
    Simple sequential Quicksort Algorithm
*/
void quicksort(int *number,int first,int last){
    if(first<last){
        int pivot_index = partition(number, first, last);
        quicksort(number,first,pivot_index-1);
        quicksort(number,pivot_index+1,last);
    }
}

/*
    Functions that handles the sharing of subarrays to the right clusters
*/
int quicksort_recursive(int* arr, int arrSize, int currProcRank, int maxRank, int rankIndex) {
    MPI_Status status;

    // Calculate the rank of the Cluster which I'll send the other half
    int shareProc = currProcRank + pow(2, rankIndex);
    // Move to lower layer in the tree
    rankIndex++;

    // If no Cluster is available, sort sequentially by yourself and return
    if (shareProc > maxRank) {
        MPI_Barrier(MPI_COMM_WORLD);
	    quicksort(arr, 0, arrSize-1 );
        return 0;
    }
    // Divide array in two parts with the pivot in between
    int j = 0;
    int pivotIndex;
    pivotIndex = hoare_partition(arr, j, arrSize-1 );

    // Send partition based on size(always send the smaller part), 
    // Sort the remaining partitions,
    // Receive sorted partition
    if (pivotIndex <= arrSize - pivotIndex) {
        MPI_Send(arr, pivotIndex , MPI_INT, shareProc, pivotIndex, MPI_COMM_WORLD);
	    quicksort_recursive((arr + pivotIndex+1), (arrSize - pivotIndex-1 ), currProcRank, maxRank, rankIndex); 
        MPI_Recv(arr, pivotIndex , MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
    else {
        MPI_Send((arr + pivotIndex+1), arrSize - pivotIndex-1, MPI_INT, shareProc, pivotIndex + 1, MPI_COMM_WORLD);
        quicksort_recursive(arr, (pivotIndex), currProcRank, maxRank, rankIndex);
        MPI_Recv((arr + pivotIndex+1), arrSize - pivotIndex-1, MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
}


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

    // Calculate in which layer of the tree each Cluster belongs
    int rankPower = 0;
    while (pow(2, rankPower) <= rank){
        rankPower++;
    }
    // Wait for all clusters to reach this point 
    MPI_Barrier(MPI_COMM_WORLD);
    double start_timer, finish_timer;
    if (rank == 0) {
	    start_timer = MPI_Wtime();
        // Cluster Zero(Master) starts the Execution and
        // always runs recursively and keeps the left bigger half
        quicksort_recursive(unsorted_array, array_size, rank, size - 1, rankPower);    
    }else{ 
        // All other Clusters wait for their subarray to arrive,
        // they sort it and they send it back.
        MPI_Status status;
        int subarray_size;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // Capturing size of the array to receive
        MPI_Get_count(&status, MPI_INT, &subarray_size);
	    int source_process = status.MPI_SOURCE;     
        int subarray[subarray_size];
        MPI_Recv(subarray, subarray_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        quicksort_recursive(subarray, subarray_size, rank, size - 1, rankPower);
        MPI_Send(subarray, subarray_size, MPI_INT, source_process, 0, MPI_COMM_WORLD);
    };
    
    if(rank==0){
        finish_timer = MPI_Wtime();
	    printf("Total time for %d Clusters : %2.2f sec \n",size, finish_timer-start_timer);

        // --- VALIDATION CHECK ---
        printf("Checking.. \n");
        bool error = false;
        int i=0;
        for(i=0;i<SIZE-1;i++) { 
            if (unsorted_array[i] > unsorted_array[i+1]){
		        error = true;
                printf("error in i=%d \n", i);
            }
        }
        if(error)
            printf("Error..Not sorted correctly\n");
        else
            printf("Correct!\n");        
    }
       
    MPI_Finalize();
    // End of Parallel Execution
    return 0;
}
