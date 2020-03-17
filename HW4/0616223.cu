/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265

void check_param(void);
void init_line(void);
void update (void);
void printfinal (void);

int nsteps,                 	/* number of time steps */
    tpoints, 	     		/* total points along string */
    rcode;                  	/* generic return code */
float  values[MAXPOINTS+2]; 	/* values at time t */
       //oldval[MAXPOINTS+2], 	/* values at time (t-dt) */
       //newval[MAXPOINTS+2]; 	/* values at time (t+dt) */


/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}

/**********************************************************************
 *     Initialize points on line
 *********************************************************************/
/**********************************************************************
 *      Calculate new values using wave equation
 *********************************************************************/
/**********************************************************************
 *     Update all values along line a specified number of times
 *********************************************************************/
__global__  void update(float *vd, float *od, float *nd, int nsteps, int tpoints){
   int i, j = (blockIdx.x * 1024) + threadIdx.x + 1;
   float x, fac, k, tmp;

   fac = 2.0 * PI;
   k = j-1; 
   tmp = tpoints - 1;

   x = k/tmp;
   vd[j] = sin (fac * x);

   od[j] = vd[j];
   if( j == 1 ){
      printf("Updating all points for all time steps...\n");
   }
   __syncthreads();

   /* Update values for each time step */
   for (i = 1; i<= nsteps; i++) {
      /* Update points along line for this time step */

      /* global endpoints */
      if ((j == 1) || (j  == tpoints))
         nd[j] = 0.0;
      else{
         float dtime, c, dx, tau, sqtau;
         dtime = 0.3;
         c = 1.0;
         dx = 1.0;
         tau = (c * dtime / dx);
         sqtau = tau * tau;
         nd[j] = (2.0 * vd[j]) - od[j] + (sqtau *  (-2.0)*vd[j]);
      }
      __syncthreads();

      /* Update old values with new values */
      od[j] = vd[j];
      vd[j] = nd[j];
      __syncthreads();
   }

}

/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
	sscanf(argv[1],"%d",&tpoints);
	sscanf(argv[2],"%d",&nsteps);
	check_param();
	printf("Initializing points on the line...\n");

   int size = (tpoints+1) * sizeof(float);
   float *vd, *od, *nd;

   cudaMalloc(&vd, size);
   //cudaMemcpy(vd, values, size, cudaMemcpyHostToDevice);
   cudaMalloc(&od, size);
   //cudaMemcpy(od, oldval, size, cudaMemcpyHostToDevice);
   cudaMalloc(&nd, size);

   int threadPerBlock = 1024;
   int numBlocks = (tpoints % threadPerBlock) ? tpoints/threadPerBlock + 1 : tpoints/threadPerBlock;
	update<<<numBlocks, threadPerBlock>>>(vd, od, nd, nsteps, tpoints);
   cudaDeviceSynchronize();
	printf("Printing final results...\n");

   cudaMemcpy(values, vd, size, cudaMemcpyDeviceToHost);
   cudaFree(vd);
   cudaFree(od);
   cudaFree(nd);

	printfinal();
	printf("\nDone.\n\n");
	
	return 0;
}