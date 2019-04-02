/* minimal code example showing how to call the zfp (de)compressor */
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sz.h"
#include "rw.h"

double calculateAVG(double array[], int len);
double calculateSTDDEV(double array[], int len);

void print_3d_array(double* array, int nx, int ny, int nz)
{
	int i, j, k;
  for (k = 0; k < nz; k++)
  	for (j = 0; j < ny; j++)
    	for (i = 0; i < nx; i++)
			{
    		fprintf(stdout, "%lf\n", array[i + nx * (j + ny * k)]);
      }
}
void copy_doubles(double* iArray, int nx, int ny, int nz, double* oArray)
{
	int i, j, k;
  for (k = 0; k < nz; k++)
    for (j = 0; j < ny; j++)
      for (i = 0; i < nx; i++)
      {
        oArray[i + nx * (j + ny * k)] = iArray[i + nx * (j + ny * k)];
      }
}



/* compress and decompress iArray, place result into oArray, preserve iArray*/
void cycle(double* iArray, int nx, int ny, int nz, double tolerance, int iterations, double* oArray, int mode)
{

	int i;
  double cRatio;
  double cTimesAVG;
  double cTimesSTDDEV;
  double dTimesAVG;
  double dTimesSTDDEV;
	double cTimes[iterations];
	double dTimes[iterations];

	size_t outSize;
	char* bytes;
	double* data;

	clock_t stime,etime;
	

	int n_y = ny > 0 ? ny : 1;
	int n_z = nz > 0 ? nz : 1;
	size_t inSize = ((double)nx) * ((double)n_y) * ((double)n_z) * sizeof(double);

	/* compress multiple times */
	for (i = 0; i < iterations; ++i)
  {
		stime = clock();
		bytes = SZ_compress_args(SZ_DOUBLE, iArray, &outSize, REL, tolerance, tolerance, tolerance, 0, 0, nz, ny, nx);
		etime = clock();
		cTimes[i] = etime-stime;

		if (i < iterations - 1)
		{
			free(bytes);
		}	
	}
  
	if (bytes == NULL)
	{
  	fprintf(stderr, "compression failed\n");
	}
	
	/* calculate compression ratio */
	cRatio = inSize / outSize;

	/* decompress multiple times */
	for (i = 0; i < iterations; ++i)
 	{
		stime = clock();
 		data = SZ_decompress(SZ_DOUBLE, bytes, outSize, 0, 0, nz, ny, nx);	
		etime = clock();  
		dTimes[i] = etime-stime;		

		if (i < iterations - 1)
		{
			free(data);
		}
	}

	if (data == NULL)
	{
  	fprintf(stderr, "decompression failed\n");
  }		

	/* copy data into output array */
	memcpy(oArray,data,inSize);
  
	/* clean up */
  free(bytes);
	free(data);
	
	/* calculate avg & stddev for cTime and dTime*/
  cTimesAVG = calculateAVG(cTimes, iterations);
  dTimesAVG = calculateAVG(dTimes, iterations);
	cTimesSTDDEV =  calculateSTDDEV(cTimes, iterations);
  dTimesSTDDEV =  calculateSTDDEV(dTimes, iterations);

  /* print stats to user */
  printf("Compression Factor =\t%lf\n",cRatio);
  printf("Time =\t%E\n",cTimesAVG);
  printf("STD_DEV =\t%E\n",cTimesSTDDEV);
  printf("Time =\t%E\n",dTimesAVG);
  printf("STD_DEV =\t%E\n",dTimesSTDDEV);

	//print_3d_array(iArray,nx,ny,nz);
	//fprintf(stdout,"----------\n");
	//print_3d_array(oArray,nx,ny,nz);
}

double calculateAVG(double array[], int len)
{
  int i;
  double avg = 0;

  for (i = 0; i < len; ++i)
  {
    avg += array[i];
  }

  return avg;
}

/* I did not know how to calculate this, I used the formula from:
 *  * https://www.mathsisfun.com/data/standard-deviation-formulas.html
 *   * */
double calculateSTDDEV(double array[], int len)
{
  int i;
  double stddev;
  double sumsq = 0;
  double mean = calculateAVG(array, len);

  for (i = 0; i < len; ++i)
  {
    sumsq += pow(array[i] - mean, 2);
  }

  stddev = sqrt(sumsq / len);

  return stddev;
}


