#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#ifndef W
#define W 20                                    // Width
#endif
int main(int argc, char **argv) {
  int my_rank;
  int all;
  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &all);
  L = L / all;
  
  int *temp = (int *)malloc(L*W*sizeof(int));          // Current temperature
  int *next = (int *)malloc((L+2)*W*sizeof(int));          // Next time step
  int *doing = (int *)malloc((L+2)*W*sizeof(int));
  int *left = (int *)malloc(W*sizeof(int));
  int *right = (int *)malloc(W*sizeof(int));
  int *gather;
  int trash;

  if(my_rank == 0){
    gather = (int *)malloc(all*sizeof(int));
  }

  MPI_Status status;
  for(int i=0; i<my_rank*W*L; i++){
    trash = random();
  }
  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      temp[i*W+j] = random()>>3;
    }
  }
  //printf("%d\n", temp[0]);
  //printf("%d\n", temp[W*(L/4)]);
  //printf("%d\n", temp[W*(L/4)*2]);
  //printf("%d\n", temp[W*(L/4)*3]);

  int count = 0, balance = 0;
  while (iteration--) {     // Compute with up, left, right, down points
    //MPI_Barrier(MPI_COMM_WORLD);
    for(int i=0; i<W; i++){
      left[i] = temp[i];
      right[i] = temp[W*(L-1)+i];
    }
    if(my_rank > 0){
      MPI_Send(left, W, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD);
    }
    if(my_rank < all-1){
      MPI_Send(right, W, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD);
    }
    if(my_rank > 0){
      MPI_Recv(left, W, MPI_INT, my_rank-1, 0, MPI_COMM_WORLD, &status);
    }
    if(my_rank < all-1){
      MPI_Recv(right, W, MPI_INT, my_rank+1, 0, MPI_COMM_WORLD, &status);
    }
    
    for(int i=0; i<W; i++){
      doing[i] = left[i];
    }
    for(int i=0; i<L; i++){
      for(int j=0; j<W; j++){
        doing[W+i*W+j] = temp[i*W+j];
      }
    }
    for(int i=0; i<W; i++){
      doing[W*(L+1)+i] = right[i];
    }
    balance = 1;
    for (int i = 0; i < L+2; i++) {
      for (int j = 0; j < W; j++) {
        float t = doing[i*W+j] / d;
        t += doing[i*W+j] * -4;
        t += doing[(i - 1 <  0 ? 0 : i - 1) * W + j];
        t += doing[(i + 1 >= L+2 ? i : i + 1)*W+j];
        t += doing[i*W+(j - 1 <  0 ? 0 : j - 1)];
        t += doing[i*W+(j + 1 >= W ? j : j + 1)];
        t *= d;
        next[i*W+j] = t ;
      }
    }
    
    for(int i=0; i<L; i++){
      for(int j=0; j<W; j++){
        if(next[W+i*W+j] != temp[i*W+j]){
          balance = 0;
        }
      }
    }
    MPI_Gather(&balance, 1, MPI_INT, gather, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int b = 1;
    if(my_rank == 0){
      count++;
      for(int i=0; i<all; i++){
        if(gather[i] != 1)b = 0;
      }
    }
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&b, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(b) break;
    for(int i=0; i<L; i++){
      for(int j=0; j<W; j++){
        temp[i*W+j] = next[W+i*W+j];
      }
    }
  }
  int min = temp[0];
  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      if (temp[i*W+j] < min) {
        min = temp[i*W+j];
      }
    }
  }
  MPI_Gather(&min, 1, MPI_INT, gather, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if(my_rank == 0){
    L = L*all;
    int min_t = gather[0];
    for(int i=1; i<all; i++){
      if(min_t > gather[i])min_t = gather[i];
    }
    printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, min_t);
  }
  MPI_Finalize();
  return 0;
}