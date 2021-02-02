# Quicksort-Parallel-MPI

This repository contains two implementations of the Quicksort Algorithm using the MPI library.  
  
  - **Recursive Quicksort** : Each cluster divides the array it receives into smaller parts and distributes it to available clusters. If there aren't any other available, then it sorts it sequentially.  
  - **Merge Quicksort** : The master cluster divides the array into equal parts and send one in each available cluster. All clusters sort their part and return it back, where it gets merged into a final big sorted array.  
  
