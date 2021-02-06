#include "mpi.h"
#include<stdio.h>
#include <stdlib.h>
#include "math.h"
#include <stdbool.h>
#define SIZE 1000000


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
        int pivot_index = hoare_partition(number, first, last);
        quicksort(number,first,pivot_index-1);
        quicksort(number,pivot_index+1,last);
    }
}

/*
    Function that handles the merging of two sorted subarrays
	and returns one bigger sorted array
*/
void merge(int *first,int *second, int *result,int first_size,int second_size){
	int i=0;
	int j=0;
	int k=0;
	
	while(i<first_size && j<second_size){

		if (first[i]<second[j]) {
			result[k]=first[i];
			k++;
			i++;
		}else{
			result[k]=second[j];
			k++;
			j++;
		}

		if(i == first_size){
			// if the first array has been sorted
			while(j<second_size){
				result[k]=second[j];
				k++;
				j++;
			}	
		} else if (j == second_size){
			// if the second array has been sorted
			while(i < first_size){
				result[k]=first[i];
				i++;
				k++;
			}
		}
	}	
}

int main(int argc, char *argv[]) {
	
    int *unsorted_array = (int *)malloc(SIZE * sizeof(int));
    int *result = (int *)malloc(SIZE * sizeof(int));
    int array_size = SIZE;
    int size, rank;
    int sub_array_size;
        
	MPI_Status status;    
	// Start parallel execution
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
	
	// Number of Clusters to be run
	int iter_count = size; 
	// Determine the size of the subarray each Cluster receives
	sub_array_size=(int)SIZE/iter_count;

	// Cluster 0 (Master) splits the array and sends each subarray to the respective machine
	if( rank == 0 ){
		double start_timer;
		start_timer=MPI_Wtime();
		int i =0;
		if(iter_count > 1){
			// ==============================================SENDING DATA==============================================
			for(i=0;i<iter_count-1;i++){
				int j;
				//send the subarray
				MPI_Send(&unsorted_array[(i+1)*sub_array_size],sub_array_size,MPI_INT,i+1,0,MPI_COMM_WORLD); 
			}
		
			// ========================================CALCULATE FIRST SUBARRAY========================================
			int i =0;
			int *sub_array = (int *)malloc(sub_array_size*sizeof(int));
			for(i=0;i<sub_array_size;i++){
				// Passing the first sub array since rank 0 always calculates the first sub array
				sub_array[i]=unsorted_array[i];
			}
			// Sequentially sorting the first array
			quicksort(sub_array,0,sub_array_size-1);
			
			// =============================================RECEIVING DATA=============================================
			for (i=0;i<iter_count;i++){
				if(i > 0){				
					int temp_sub_array[sub_array_size];
					// Receive each subarray
					MPI_Recv(temp_sub_array,sub_array_size,MPI_INT,i,777,MPI_COMM_WORLD,&status);
					int j;
					int temp_result[i*sub_array_size];
					for(j=0;j<i*sub_array_size;j++){
						temp_result[j]=result[j];
					}
					int temp_result_size = sub_array_size*i;
					// Merge it back into the result array
					merge(temp_sub_array,temp_result,result,sub_array_size,temp_result_size);

				}else{
					// On first iteration we just pass the sorted elements to the result array
					int j;	
					for(j=0;j<sub_array_size;j++){	
						result[j]=sub_array[j];
					}		
					free(sub_array);
				}
			}
		}else{
			// if it runs only in a single Cluster
			quicksort(unsorted_array,0,SIZE-1);
			for(i=0;i<SIZE;i++){
				result[i]=unsorted_array[i];
			}
		}
		double finish_timer;
		finish_timer=MPI_Wtime();
		printf("End Result: \n");
		printf("Cluster Size %d, execution time measured : %2.7f sec \n",size, finish_timer-start_timer);
	}else{
		// All the other Clsuters have to sort the data and send it back
		sub_array_size=(int)SIZE/iter_count;
		int *sub_array = (int *)malloc(sub_array_size*sizeof(int));
		MPI_Recv(sub_array,sub_array_size,MPI_INT,0,0,MPI_COMM_WORLD,&status);	
		quicksort(sub_array,0,sub_array_size-1);
		int i=0;
		MPI_Send(sub_array,sub_array_size,MPI_INT,0,777,MPI_COMM_WORLD);//sends the data back to rank 0	
		free(sub_array);
	}

	if(rank==0){
        // --- VALIDATION CHECK ---
        printf("Checking.. \n");
        bool error = false;
        int i=0;
        for(i=0;i<SIZE-1;i++) {
            if (result[i] > result[i+1]){
                error = true;
        		printf("error in i=%d \n", i);
        	}
        }
        if(error)
            printf("Error..Not sorted correctly\n");
        else
            printf("Correct!\n");
    }
	free(unsorted_array);
	// End of Parallel Execution
	MPI_Finalize();
}
