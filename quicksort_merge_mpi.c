#include "mpi.h"
#include<stdio.h>
#include <stdlib.h>
#include "math.h"
#include <stdbool.h>
#define SIZE 100000



// returns the pivot index
int partition(int *number,int first,int last){
    int i,j, pivot, temp;
    if(first<last){
        pivot=last;
        i=first;
        j=last;
        while(i<j){
            while(number[i]<=number[pivot]&&i<last)
                i++;
            while(number[j]>number[pivot])
                j--;
            if(i<j){
                //swap
                temp=number[i];
                number[i]=number[j];
                number[j]=temp;
            }
        }
        //swap
        temp=number[pivot];
        number[pivot]=number[j];
        number[j]=temp;

        return j;
    }
}

void quicksort(int *number,int first,int last){
    if(first<last){
        int pivot_index = partition(number, first, last);
        quicksort(number,first,pivot_index-1);
        quicksort(number,pivot_index+1,last);
    }
}


void merge(int *first,int *second, int *result,int first_size,int second_size){
	int i=0;
	int j=0;
	int k=0;
	
	
	
	while(i<first_size && j<second_size){
			if(first[i]<second[j]){
			result[k]=first[i];
			k++;
			i++;
		}
		else{
			result[k]=second[j];
			k++;
			j++;
		}
		if(i==first_size){//if the first array has been sorted
			while(j<second_size){
				result[k]=second[j];
				k++;
				j++;
			}	
		}
		else if(j==second_size){
			while(i<first_size){//if the second array has been sorted
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
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if(rank==0){
    	// --- ARRAY GENERATION ---
    	printf("Creating Random List of 100 elements\n");
    	int j = 0;
    	for (j = 0; j < SIZE; ++j) {
        	unsorted_array[j] =(int) rand() % 1000;
			//printf("%d ",unsorted_array[j]);
    	}
        printf("Created\n");
	}
	
	int iter_count = size; //num of processors to be run
	sub_array_size=(int)SIZE/iter_count;
	
	if(rank==0){//rank 0 has to split the array and send each subarray to the respective machine
		double start;
		start=MPI_Wtime();
		int i =0;
		if(iter_count>1){
			double start1;
			start1=MPI_Wtime();
			for(i=0;i<iter_count-1;i++){//=======================================SENDING DATA=======================================================
				int j;
				MPI_Send(&unsorted_array[(i+1)*sub_array_size],sub_array_size,MPI_INT,i+1,0,MPI_COMM_WORLD);//sends the sub array 

			}//=======================================SENDING DATA END=======================================================
			double end1;
			end1=MPI_Wtime();
			printf("execution time send : %1.2f sec\n",end1-start1);
			
			int i =0;//*************************************Calculating First Sub Array ********************************
			int *sub_array = (int *)malloc(sub_array_size*sizeof(int));
			for(i=0;i<sub_array_size;i++){
				sub_array[i]=unsorted_array[i];//passing the first sub array since rank 0 always calculates the first sub array
			}
			quicksort(sub_array,0,sub_array_size-1);//sorting the first array
			//*************************************Calculating First Sub Array End********************************
			
			
			for (i=0;i<iter_count;i++){
				if(i>0){
					double start2;
					start2=MPI_Wtime();
					int temp_sub_array[sub_array_size];
					MPI_Recv(temp_sub_array,sub_array_size,MPI_INT,i,666,MPI_COMM_WORLD,&status);//receive each subarray t
					int j;
					int temp_result[i*sub_array_size];
					for(j=0;j<i*sub_array_size;j++){
						temp_result[j]=result[j];
					}
					int temp_result_size = sub_array_size*i;

					merge(temp_sub_array,temp_result,result,sub_array_size,temp_result_size);
					double end2;
					end2=MPI_Wtime();
					printf("execution time recv merge : %1.2f sec\n",end2-start2);
					
				}//endif i>0
				else{//first iteration we just pass the sorted elements to the result array
					int j;	
					for(j=0;j<sub_array_size;j++){	
						result[j]=sub_array[j];
					}		
					free(sub_array);
				}
			}
		}	
	
		else{//if it runs on only one copmuter
			quicksort(unsorted_array,0,SIZE-1);
			for(i=0;i<SIZE;i++){
				result[i]=unsorted_array[i];
			}
		}
		double finish;
		finish=MPI_Wtime();
		printf("End Result: \n");
		/*for(i=0;i<SIZE;i++){//prints the end result
			//printf("%d ",result[i]);
		}//endfor
		*/
		printf("execution time measured : %1.2f sec\n",finish-start);
	}//end_if_rank==0

	else{// the rest have to sort the data and send it back. need to fix it so rank0 sorts too

		sub_array_size=(int)SIZE/iter_count;
		int *sub_array = (int *)malloc(sub_array_size*sizeof(int));
		MPI_Recv(sub_array,sub_array_size,MPI_INT,0,0,MPI_COMM_WORLD,&status);	
		quicksort(sub_array,0,sub_array_size-1);
		int i=0;
		MPI_Send(sub_array,sub_array_size,MPI_INT,0,666,MPI_COMM_WORLD);//sends the data back to rank 0	
		free(sub_array);
	}

	if(rank==0){
        // --- VALIDATION CHECK ---
        printf("Checking.. \n");
        bool error = false;
        int i=0;
        for(i=0;i<SIZE-1;i++) {
            //printf("%d ",result[i]);
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
	MPI_Finalize();
}
