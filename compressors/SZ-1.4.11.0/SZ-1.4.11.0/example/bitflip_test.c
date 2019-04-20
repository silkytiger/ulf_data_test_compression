/**
 *  @file bitflip_test.c
 *  @author Jon Calhoun
 *  @date July, 2018
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sz.h"
#include "rw.h"

struct timeval startTime;
struct timeval endTime;  /* Start and end times */
struct timeval costStart; /*only used for recording the cost*/
double totalCost = 0;

uint64_t swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

void cost_start()
{
        gettimeofday(&costStart, NULL);
}

void cost_end()
{
        double elapsed;
        struct timeval costEnd;
        gettimeofday(&costEnd, NULL);
        elapsed = ((costEnd.tv_sec*1000000+costEnd.tv_usec)-(costStart.tv_sec*1000000+costStart.tv_usec))/1000000.0;
        totalCost += elapsed;
}

void flipbit(unsigned char* data, int len, long bit)
{
    int numbytes = bit / (8*sizeof(unsigned char));
    int byte_bit = bit % (8*sizeof(unsigned char));
    int mask = 0x1;
    printf("Data arrray is of size %d bytes\n", len);
    printf("%d bit is in byte %d at %d position\n", (int) bit, numbytes, byte_bit);

    if (bit != -1) {
        assert( numbytes >= 0 && numbytes < len && "Bit outside array bounds!\n" );
        printf("Data value 0x%x\n", data[numbytes]);
        printf("mask = 0x%x\n", mask << byte_bit);
        
        data[numbytes] ^= (unsigned char)(mask << byte_bit);
        
        printf("Data value 0x%x\n", data[numbytes]);
    }
}


int main(int argc, char * argv[])
{
    size_t r5=0,r4=0,r3=0,r2=0,r1=0, bit;
    char outDir[640], oriFilePath[640], outputFilePath[640];
    char *cfgFile;
    
    if(argc < 3)
    {
	printf("Test case: testdouble_compress [config_file] [srcFilePath] [dimension sizes...]\n");
	printf("Example: testdouble_compress sz.config testdouble_8_8_128.dat 8 8 128\n");
	exit(0);
    }
   
    cfgFile=argv[1];
    sprintf(oriFilePath, "%s", argv[2]);
    if(argc>=4)
    	r1 = atoi(argv[3]); //8
    if(argc>=5)
    	r2 = atoi(argv[4]); //8
    if(argc>=6)
	r3 = atoi(argv[5]); //128
    if(argc>=7)
    	r4 = atoi(argv[6]);
    if(argc>=8)
	r5 = atoi(argv[7]);
 
    /* map bit to flip as r4  for 3D data */
    bit = r4;
    r4 = r5 = 0;

    int status = 0;
    printf("cfgFile=%s\n", cfgFile); 
    status = SZ_Init(cfgFile);

   /* sz_params sz;
    sz.dataEndianType = LITTLE_ENDIAN_DATA;
    sz.sysEndianType = LITTLE_ENDIAN_DATA;
    sz.sol_ID = SZ;
    sz.layers = 1;
    sz.sampleDistance = 50;
    sz.quantization_intervals = 0;
    sz.max_quant_intervals = 65536;
    sz.predThreshold = 0.98;
    sz.offset = 0;
    sz.szMode = SZ_DEFAULT_COMPRESSION;
    sz.gzipMode = 1;
    sz.errorBoundMode = REL;
    sz.absErrBound = 1E-6;
    sz.relBoundRatio = 1E-5;
    //sz.pw_relBoundRatio = 1E-5;
    //sz.segment_size = 32;
    sz.pwr_type = 0;

    SZ_Init_Params(&sz);*/

    sprintf(outputFilePath, "%s.sz", oriFilePath);
   
    size_t nbEle;
    double *data = readDoubleData(oriFilePath, &nbEle, &status);
    if(status!=SZ_SCES)
    {
	printf("Error: file %s cannot be read!\n", oriFilePath);
	exit(0);
    }
    int i,N;
    printf("READ %d ELMENTS\n", nbEle);
    for (i=0; i < nbEle; i++)
    {
        unsigned long long* ptr = (unsigned long long*)&data[i];
        unsigned long long tmp = swap_uint64(*ptr);
        //printf("%p \t %p \t %g\n", *ptr, tmp, data[i]);
        *ptr = tmp;
        //printf("%p \t %p \t %g\n", *ptr, tmp, data[i]);
    }
  
    for(i=0; i < 10; i++)
        printf("%g\n", data[i]);
 
    size_t outSize;
    i=0, N=25;
    unsigned char *bytes; 
    for (i=0; i < N; i++)
    {
        cost_start(); 
        bytes = SZ_compress(SZ_DOUBLE, data, &outSize, r5, r4, r3, r2, r1);
        //char *bytes = (char *)malloc(nbEle*sizeof(double)); //
        //char* bytes = SZ_compress_args(SZ_DOUBLE, data, &outSize, ABS, 1E-12, 0.000001, r5, r4, r3, r2, r1);
        cost_end();
        if (i == 0)
        {
            flipbit(bytes, outSize, bit);
            //flipbit(bytes, outSize, bit);
            writeByteData(bytes, outSize, outputFilePath, &status);
            
            if(status!=SZ_SCES)
            {
                printf("Error: file %s cannot be written!\n", outputFilePath);
                free(data);
                exit(0);
            }

        }
        if (i +1 == N)
        {   
            data = SZ_decompress(SZ_DOUBLE, bytes, outSize, r5, r4, r3, r2, r1);
        }
        free(bytes);
    }
    for(i=0; i < 10; i++)
        printf("%g\n", data[i]);
    

    printf("factor=%e\n", 1. * computeDataLength(r5, r4, r3, r2, r1) / outSize);
    printf("timecost=%e\n",totalCost/N);

    free(data);
    //free(bytes);
    printf("done\n");
    
    SZ_Finalize();
    
    return 0;
}
