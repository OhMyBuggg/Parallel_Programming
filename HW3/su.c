#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef W
#define W 20 // Width
#endif
#define TOLEFT 0
#define FROMLEFT 1
#define TORIGHT 1
#define FROMRIGHT 0

#ifdef DEBUG
#include <string.h>
void show(int *temp, int L) {
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < W; j++) {
            printf("temp[%d * W + %d]: %d\n ", i, j, temp[i * W + j]);
        }
    }
    printf("\n");
}
#endif

int SIZE, RANK;
void send_border(int *l_buf, int *r_buf) {
    if (RANK != 0) {
        MPI_Bsend(l_buf, W, MPI_INT, RANK - 1, TOLEFT, MPI_COMM_WORLD);
    }
    if (RANK != SIZE - 1) {
        MPI_Bsend(r_buf, W, MPI_INT, RANK + 1, TORIGHT, MPI_COMM_WORLD);
    }
}

/*
void fetch_border(int *l_buf, int *r_buf) {
    MPI_Status status;
    if (RANK != 0) {
        MPI_Recv(l_buf, W, MPI_INT, RANK - 1, FROMLEFT, MPI_COMM_WORLD, &status);
    }
    if (RANK != SIZE - 1) {
        MPI_Recv(r_buf, W, MPI_INT, RANK + 1, FROMRIGHT, MPI_COMM_WORLD, &status);
    }
}
*/
void fetch_border(int *temp, int left_idx, int right_idx) {
    MPI_Status status;
    if (RANK != 0) {
        MPI_Recv(&temp[left_idx], W, MPI_INT, RANK - 1, FROMLEFT, MPI_COMM_WORLD, &status);
    }
    if (RANK != SIZE - 1) {
        MPI_Recv(&temp[right_idx], W, MPI_INT, RANK + 1, FROMRIGHT, MPI_COMM_WORLD, &status);
    }
}
int main(int argc, char **argv) {
    int startL, endL; //, startW, endW;
    int segment_length;
    //int *_balance = malloc(sizeof(int) * SIZE);
    int _balance;

    // MPI initialization here
    MPI_Init(&argc, &argv);
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    // -----------------------------

    // Normal variables
    int L = atoi(argv[1]); // Length
    segment_length = L / SIZE;
    int iteration = atoi(argv[2]); // Iteration
    srand(atoi(argv[3])); // Seed
    float d = (float)random() / RAND_MAX * 0.2; // Diffusivity
    int *temp = malloc(L * W * sizeof(int)); // Current temperature
    // int *temp = malloc(segment_length * W * sizeof(int)); // Current temperature
    int *next = malloc(L * W * sizeof(int)); // Next time step
    //int *next = malloc(segment_length * W * sizeof(int)); // Next time step
    int *buf_l = malloc(W * sizeof(int)); //
    int *buf_r = malloc(W * sizeof(int)); //

    // -----------------------------

    // Rank-dependent variables
    startL = segment_length * RANK;
    endL = segment_length * (RANK + 1);

    // Bsend buffer initialization
    int buf_size;
    MPI_Pack_size(2 * W, MPI_INT, MPI_COMM_WORLD, &buf_size);
    buf_size += 2 * MPI_BSEND_OVERHEAD;
    int *buf = malloc(buf_size);
    MPI_Buffer_attach(buf, buf_size);

    // -----------------------------
    /*
    for (int i = 0; i < segment_length; i++) {
        for (int j = 0; j < W; j++) {
            temp[i * W + j] = random() >> 3;
        }
    }
    */
    //#ifdef TEST
    // handle random for serial testing
    for (int i = 0; i < startL * W; i++) {
        random();
    }
    //memset(temp, 0, sizeof(int) * L * W);
    //#endif
    for (int i = startL; i < endL; i++) {
        for (int j = 0; j < W; j++) {
            temp[i * W + j] = random() >> 3;
        }
    }

#ifdef DEBUG
    show(temp, L);
#endif

    int count = 0, balance = 0;
    send_border(&temp[startL * W], &temp[(endL - 1) * W]);
    while (iteration--) { // Compute with up, left, right, down points
        //fetch_border(buf_l, buf_r);
        fetch_border(temp, (startL - 1) * W, endL * W);
        //MPI_Barrier(MPI_COMM_WORLD);
        //MPI_Gather(&balance, 1, MPI_INT, _balance, 1 MPI_INT, 0, MPI_COMM_WORLD);

        balance = 1;
        count++;
        for (int i = startL; i < endL; i++) {
            for (int j = 0; j < W; j++) {
                float t = temp[i * W + j] / d;
                t += temp[i * W + j] * -4;
                t += temp[(i - 1 < 0 ? 0 : i - 1) * W + j];
                t += temp[(i + 1 >= L ? i : i + 1) * W + j];
                t += temp[i * W + (j - 1 < 0 ? 0 : j - 1)];
                t += temp[i * W + (j + 1 >= W ? j : j + 1)];
                t *= d;
                next[i * W + j] = t;
                if (next[i * W + j] != temp[i * W + j]) {
                    balance = 0;
                }
            }
        }
        //if (balance) {
        //    break;
        //}
        int *tmp = temp;
        temp = next;
        next = tmp;

        MPI_Reduce(&balance, &_balance, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        if (RANK == 0) {
            balance = (_balance == SIZE) ? 1 : 0; // if all balanced then set balance = 1
        }
        MPI_Bcast(&balance, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (balance == 1) {
            break;
        }
        send_border(&temp[startL * W], &temp[(endL - 1) * W]);
    }

    //MPI_Gather(&temp[startL], segment_length * W, MPI_INT, &temp[endL], segment_length * W, MPI_INT, 0, MPI_COMM_WORLD);

    int *tt;
    if (RANK == 0) {
        tt = malloc(L * W * sizeof(int));
        MPI_Gather(&temp[startL * W], segment_length * W, MPI_INT, tt, segment_length * W, MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        MPI_Gather(&temp[startL * W], segment_length * W, MPI_INT, NULL, segment_length * W, MPI_INT, 0, MPI_COMM_WORLD);
    }

    if (RANK == 0) {
        free(temp);
        temp = tt;
        int min = temp[0];
        for (int i = 0; i < L; i++) {
            for (int j = 0; j < W; j++) {
#ifdef DEBUG
                printf("temp[%d * W + %d]: %d\n ", i, j, temp[i * W + j]);
#endif
                if (temp[i * W + j] < min) {
                    min = temp[i * W + j];
                }
            }
        }
#ifdef DEBUG
        printf("\n");
#endif
        printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, min);
    }
    MPI_Finalize();
    return 0;
}
