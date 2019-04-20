/**
 *  @file sz_double_pwr.c
 *  @author Sheng Di
 *  @date Aug, 2016
 *  @brief SZ_Init, Compression and Decompression functions
 * This file contains the compression/decompression functions related to point-wise relative errors
 *  (C) 2016 by Mathematics and Computer Science (MCS), Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "sz.h"
#include "CompressElement.h"
#include "DynamicByteArray.h"
#include "DynamicIntArray.h"
#include "TightDataPointStorageD.h"
#include "zlib.h"
#include "rw.h"

void compute_segment_precisions_double_1D(double *oriData, size_t dataLength, double* pwrErrBound, unsigned char* pwrErrBoundBytes, double globalPrecision)
{
	size_t i = 0, j = 0, k = 0;
	double realPrecision = oriData[0]!=0?fabs(pw_relBoundRatio*oriData[0]):pw_relBoundRatio; 
	double approxPrecision;
	unsigned char realPrecBytes[8];
	double curPrecision;
	double curValue;
	float sum = 0;
	for(i=0;i<dataLength;i++)
	{
		curValue = oriData[i];
		if(i%segment_size==0&&i>0)
		{
			//get two first bytes of the realPrecision
			if(pwr_type==SZ_PWR_AVG_TYPE)
			{
				realPrecision = sum/segment_size;
				sum = 0;			
			}
			if(errorBoundMode==ABS_AND_PW_REL||errorBoundMode==REL_AND_PW_REL)
				realPrecision = realPrecision<globalPrecision?realPrecision:globalPrecision; 
			else if(errorBoundMode==ABS_OR_PW_REL||errorBoundMode==REL_OR_PW_REL)
				realPrecision = realPrecision<globalPrecision?globalPrecision:realPrecision;
				
			doubleToBytes(realPrecBytes, realPrecision);
			memset(&realPrecBytes[2], 0, 6);
			approxPrecision = bytesToDouble(realPrecBytes);
			//put the realPrecision in float* pwrErBound
			pwrErrBound[j++] = approxPrecision;
			//put the two bytes in pwrErrBoundBytes
			pwrErrBoundBytes[k++] = realPrecBytes[0];
			pwrErrBoundBytes[k++] = realPrecBytes[1];
		}
		
		if(curValue!=0)
		{
			curPrecision = fabs(pw_relBoundRatio*curValue);
			
			switch(pwr_type)
			{
			case SZ_PWR_MIN_TYPE: 
				if(realPrecision>curPrecision)
					realPrecision = curPrecision;	
				break;
			case SZ_PWR_AVG_TYPE:
				sum += curPrecision;
				break;
			case SZ_PWR_MAX_TYPE:
				if(realPrecision<curPrecision)
					realPrecision = curPrecision;					
				break;
			}
		}
	}
	if(pwr_type==SZ_PWR_AVG_TYPE)
	{
		int size = dataLength%segment_size==0?segment_size:dataLength%segment_size;
		realPrecision = sum/size;		
	}	
	if(errorBoundMode==ABS_AND_PW_REL||errorBoundMode==REL_AND_PW_REL)
		realPrecision = realPrecision<globalPrecision?realPrecision:globalPrecision; 
	else if(errorBoundMode==ABS_OR_PW_REL||errorBoundMode==REL_OR_PW_REL)
		realPrecision = realPrecision<globalPrecision?globalPrecision:realPrecision;	
	doubleToBytes(realPrecBytes, realPrecision);
	memset(&realPrecBytes[2], 0, 6);
	approxPrecision = bytesToDouble(realPrecBytes);
	//put the realPrecision in float* pwrErBound
	pwrErrBound[j++] = approxPrecision;
	//put the two bytes in pwrErrBoundBytes
	pwrErrBoundBytes[k++] = realPrecBytes[0];
	pwrErrBoundBytes[k++] = realPrecBytes[1];
}

unsigned int optimize_intervals_double_1D_pwr(double *oriData, size_t dataLength, double* pwrErrBound)
{	
	size_t i = 0, j = 0;
	double realPrecision = pwrErrBound[j++];	
	unsigned long radiusIndex;
	double pred_value = 0, pred_err;
	int *intervals = (int*)malloc(maxRangeRadius*sizeof(int));
	memset(intervals, 0, maxRangeRadius*sizeof(int));
	int totalSampleSize = dataLength/sampleDistance;
	for(i=2;i<dataLength;i++)
	{
		if(i%segment_size==0)
			realPrecision = pwrErrBound[j++];
		if(i%sampleDistance==0)
		{
			pred_value = 2*oriData[i-1] - oriData[i-2];
			//pred_value = oriData[i-1];
			pred_err = fabs(pred_value - oriData[i]);
			radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
			if(radiusIndex>=maxRangeRadius)
				radiusIndex = maxRangeRadius - 1;			
			intervals[radiusIndex]++;
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*predThreshold;
	int sum = 0;
	for(i=0;i<maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=maxRangeRadius)
		i = maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);
	
	if(powerOf2<32)
		powerOf2 = 32;
	
	free(intervals);
	//printf("accIntervals=%d, powerOf2=%d\n", accIntervals, powerOf2);
	return powerOf2;
}

void compute_segment_precisions_double_2D(double *oriData, double* pwrErrBound, 
size_t r1, size_t r2, size_t R2, size_t edgeSize, unsigned char* pwrErrBoundBytes, double Min, double Max, double globalPrecision)
{
	size_t i = 0, j = 0, k = 0, p = 0, index = 0, J; //I=-1,J=-1 if they are needed
	double realPrecision; 
	double approxPrecision;
	unsigned char realPrecBytes[8];
	double curValue, curAbsValue;
	double* statAbsValues = (double*)malloc(R2*sizeof(double));	
	
	double max = fabs(Min)<fabs(Max)?fabs(Max):fabs(Min); //get the max abs value.
	double min = fabs(Min)<fabs(Max)?fabs(Min):fabs(Max);
	for(i=0;i<R2;i++)
	{
		if(pwr_type == SZ_PWR_MIN_TYPE)
			statAbsValues[i] = max;
		else if(pwr_type == SZ_PWR_MAX_TYPE)
			statAbsValues[i] = min;
		else
			statAbsValues[i] = 0; //for SZ_PWR_AVG_TYPE
	}
	for(i=0;i<r1;i++)
	{
		for(j=0;j<r2;j++)
		{
			index = i*r2+j;
			curValue = oriData[index];				
			if(((i%edgeSize==edgeSize-1 || i==r1-1) &&j%edgeSize==0&&j>0) || (i%edgeSize==0&&j==0&&i>0))
			{
				if(pwr_type==SZ_PWR_AVG_TYPE)
				{
					int a = edgeSize, b = edgeSize;
					if(j==0)
					{
						if(r2%edgeSize==0) 
							b = edgeSize;
						else
							b = r2%edgeSize;
					}
					if(i==r1-1)
					{
						if(r1%edgeSize==0)
							a = edgeSize;
						else
							a = r1%edgeSize;
					}
					realPrecision = pw_relBoundRatio*statAbsValues[J]/(a*b);
				}
				else
					realPrecision = pw_relBoundRatio*statAbsValues[J];

				if(errorBoundMode==ABS_AND_PW_REL||errorBoundMode==REL_AND_PW_REL)
					realPrecision = realPrecision<globalPrecision?realPrecision:globalPrecision; 
				else if(errorBoundMode==ABS_OR_PW_REL||errorBoundMode==REL_OR_PW_REL)
					realPrecision = realPrecision<globalPrecision?globalPrecision:realPrecision;
					
				doubleToBytes(realPrecBytes, realPrecision);
				memset(&realPrecBytes[2], 0, 6);
				approxPrecision = bytesToDouble(realPrecBytes);
				//put the realPrecision in double* pwrErBound
				pwrErrBound[p++] = approxPrecision;
				//put the two bytes in pwrErrBoundBytes
				pwrErrBoundBytes[k++] = realPrecBytes[0];
				pwrErrBoundBytes[k++] = realPrecBytes[1];	
				if(pwr_type == SZ_PWR_MIN_TYPE)
					statAbsValues[J] = max;
				else if(pwr_type == SZ_PWR_MAX_TYPE)
					statAbsValues[J] = min;
				else
					statAbsValues[J] = 0; //for SZ_PWR_AVG_TYPE		
			}	
			if(j==0)
				J = 0;
			else if(j%edgeSize==0)
				J++;			
			if(curValue!=0)
			{
				curAbsValue = fabs(curValue);
				
				switch(pwr_type)
				{
				case SZ_PWR_MIN_TYPE: 
					if(statAbsValues[J]>curAbsValue)
						statAbsValues[J] = curAbsValue;	
					break;
				case SZ_PWR_AVG_TYPE:
					statAbsValues[J] += curAbsValue;
					break;
				case SZ_PWR_MAX_TYPE:
					if(statAbsValues[J]<curAbsValue)
						statAbsValues[J] = curAbsValue;					
					break;
				}
			}
		}
	}
		
	if(pwr_type==SZ_PWR_AVG_TYPE)
	{
		int a = edgeSize, b = edgeSize;
		if(r2%edgeSize==0) 
			b = edgeSize;
		else
			b = r2%edgeSize;
		if(r1%edgeSize==0)
			a = edgeSize;
		else
			a = r1%edgeSize;
		realPrecision = pw_relBoundRatio*statAbsValues[J]/(a*b);
	}
	else
		realPrecision = pw_relBoundRatio*statAbsValues[J];		

	if(errorBoundMode==ABS_AND_PW_REL||errorBoundMode==REL_AND_PW_REL)
		realPrecision = realPrecision<globalPrecision?realPrecision:globalPrecision; 
	else if(errorBoundMode==ABS_OR_PW_REL||errorBoundMode==REL_OR_PW_REL)
		realPrecision = realPrecision<globalPrecision?globalPrecision:realPrecision;
		
	doubleToBytes(realPrecBytes, realPrecision);
	realPrecBytes[2] = realPrecBytes[3] = 0;
	approxPrecision = bytesToDouble(realPrecBytes);
	//put the realPrecision in double* pwrErBound
	pwrErrBound[p++] = approxPrecision;
	//put the two bytes in pwrErrBoundBytes
	pwrErrBoundBytes[k++] = realPrecBytes[0];
	pwrErrBoundBytes[k++] = realPrecBytes[1];	
	
	free(statAbsValues);
}

unsigned int optimize_intervals_double_2D_pwr(double *oriData, size_t r1, size_t r2, size_t R2, size_t edgeSize, double* pwrErrBound)
{	
	size_t i = 0,j = 0, index, I=0, J=0;
	double realPrecision = pwrErrBound[0];	
	unsigned long radiusIndex;
	double pred_value = 0, pred_err;
	int *intervals = (int*)malloc(maxRangeRadius*sizeof(int));
	memset(intervals, 0, maxRangeRadius*sizeof(int));
	size_t totalSampleSize = (r1-1)*(r2-1)/sampleDistance;
	size_t ir2;
	for(i=1;i<r1;i++)
	{
		ir2 = i*r2;
		if(i%edgeSize==0)
		{	
			I++;
			J = 0;
		}
		for(j=1;j<r2;j++)
		{
			index = ir2+j;
			if(j%edgeSize==0)
				J++;
				
			if((i+j)%sampleDistance==0)
			{
				realPrecision = pwrErrBound[I*R2+J];
				pred_value = oriData[index-1] + oriData[index-r2] - oriData[index-r2-1];
				pred_err = fabs(pred_value - oriData[index]);
				radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
				if(radiusIndex>=maxRangeRadius)
					radiusIndex = maxRangeRadius - 1;
				intervals[radiusIndex]++;
			}			
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*predThreshold;
	int sum = 0;
	for(i=0;i<maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=maxRangeRadius)
		i = maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	//printf("maxRangeRadius = %d, accIntervals=%d, powerOf2=%d\n", maxRangeRadius, accIntervals, powerOf2);
	return powerOf2;
}

void compute_segment_precisions_double_3D(double *oriData, double* pwrErrBound, 
size_t r1, size_t r2, size_t r3, size_t R2, size_t R3, size_t edgeSize, unsigned char* pwrErrBoundBytes, double Min, double Max, double globalPrecision)
{
	size_t i = 0, j = 0, k = 0, p = 0, q = 0, index = 0, J = 0, K = 0; //I=-1,J=-1 if they are needed
	size_t r23 = r2*r3, ir, jr;
	double realPrecision; 
	double approxPrecision;
	unsigned char realPrecBytes[8];
	double curValue, curAbsValue;
	
	double** minAbsValues = create2DArray_double(R2, R3);
	double max = fabs(Min)<fabs(Max)?fabs(Max):fabs(Min); //get the max abs value.	
	for(i=0;i<R2;i++)
		for(j=0;j<R3;j++)
			minAbsValues[i][j] = max;			
			
	for(i=0;i<r1;i++)
	{
		ir = i*r23;		
		if(i%edgeSize==0&&i>0)
		{
			realPrecision = pw_relBoundRatio*minAbsValues[J][K];
			doubleToBytes(realPrecBytes, realPrecision);
			memset(&realPrecBytes[2], 0, 6);
			approxPrecision = bytesToDouble(realPrecBytes);
			//put the realPrecision in float* pwrErBound
			pwrErrBound[p++] = approxPrecision;
			//put the two bytes in pwrErrBoundBytes
			//printf("q=%d, i=%d, j=%d, k=%d\n",q,i,j,k);
			pwrErrBoundBytes[q++] = realPrecBytes[0];
			pwrErrBoundBytes[q++] = realPrecBytes[1];
			minAbsValues[J][K] = max;			
		}		
		for(j=0;j<r2;j++)
		{
			jr = j*r3;
			if((i%edgeSize==edgeSize-1 || i == r1-1)&&j%edgeSize==0&&j>0)
			{
				realPrecision = pw_relBoundRatio*minAbsValues[J][K];
				doubleToBytes(realPrecBytes, realPrecision);
				memset(&realPrecBytes[2], 0, 6);
				approxPrecision = bytesToDouble(realPrecBytes);
				//put the realPrecision in float* pwrErBound
				pwrErrBound[p++] = approxPrecision;
				//put the two bytes in pwrErrBoundBytes
				//printf("q=%d, i=%d, j=%d, k=%d\n",q,i,j,k);
				pwrErrBoundBytes[q++] = realPrecBytes[0];
				pwrErrBoundBytes[q++] = realPrecBytes[1];
				minAbsValues[J][K] = max;				
			}
			
			if(j==0)
				J = 0;
			else if(j%edgeSize==0)
				J++;					
			
			for(k=0;k<r3;k++)
			{
				index = ir+jr+k;				
				curValue = oriData[index];				
				if((i%edgeSize==edgeSize-1 || i == r1-1)&&(j%edgeSize==edgeSize-1||j==r2-1)&&k%edgeSize==0&&k>0)
				{
					realPrecision = pw_relBoundRatio*minAbsValues[J][K];
					doubleToBytes(realPrecBytes, realPrecision);
					memset(&realPrecBytes[2], 0, 6);
					approxPrecision = bytesToDouble(realPrecBytes);
					//put the realPrecision in float* pwrErBound
					pwrErrBound[p++] = approxPrecision;
					//put the two bytes in pwrErrBoundBytes
					//printf("q=%d, i=%d, j=%d, k=%d\n",q,i,j,k);
					pwrErrBoundBytes[q++] = realPrecBytes[0];
					pwrErrBoundBytes[q++] = realPrecBytes[1];
					minAbsValues[J][K] = max;
				}	

				if(k==0)
					K = 0;
				else if(k%edgeSize==0)
					K++;
					
				if(curValue!=0)
				{
					curAbsValue = fabs(curValue);
					if(minAbsValues[J][K]>curAbsValue)
						minAbsValues[J][K] = curAbsValue;
				}
			}			
		}
	}	
	
	realPrecision = pw_relBoundRatio*minAbsValues[J][K];
	doubleToBytes(realPrecBytes, realPrecision);
	memset(&realPrecBytes[2], 0, 6);
	approxPrecision = bytesToDouble(realPrecBytes);
	//put the realPrecision in float* pwrErBound
	pwrErrBound[p++] = approxPrecision;
	//put the two bytes in pwrErrBoundBytes
	pwrErrBoundBytes[q++] = realPrecBytes[0];
	pwrErrBoundBytes[q++] = realPrecBytes[1];
	
	free2DArray_double(minAbsValues, R2);
}

unsigned int optimize_intervals_double_3D_pwr(double *oriData, size_t r1, size_t r2, size_t r3, size_t R2, size_t R3, size_t edgeSize, double* pwrErrBound)
{	
	size_t i,j,k, ir,jr,index, I = 0,J=0,K=0;
	double realPrecision = pwrErrBound[0];		
	unsigned long radiusIndex;
	size_t r23=r2*r3;
	size_t R23 = R2*R3;
	double pred_value = 0, pred_err;
	int *intervals = (int*)malloc(maxRangeRadius*sizeof(int));
	memset(intervals, 0, maxRangeRadius*sizeof(int));
	size_t totalSampleSize = (r1-1)*(r2-1)*(r3-1)/sampleDistance;
	for(i=1;i<r1;i++)
	{
		ir = i*r23;
		if(i%edgeSize==0)
		{	
			I++;
			J = 0;
		}
		for(j=1;j<r2;j++)
		{
			jr = j*r3;
			if(j%edgeSize==0)
			{	
				J++;
				K = 0;
			}			
			for(k=1;k<r3;k++)
			{
				index = ir+jr+k;
				if(k%edgeSize==0)
					K++;		
				if((i+j+k)%sampleDistance==0)
				{
					realPrecision = pwrErrBound[I*R23+J*R2+K];					
					pred_value = oriData[index-1] + oriData[index-r3] + oriData[index-r23] 
					- oriData[index-1-r23] - oriData[index-r3-1] - oriData[index-r3-r23] + oriData[index-r3-r23-1];
					pred_err = fabs(pred_value - oriData[index]);
					radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
					if(radiusIndex>=maxRangeRadius)
						radiusIndex = maxRangeRadius - 1;
					intervals[radiusIndex]++;
				}
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*predThreshold;
	int sum = 0;
	for(i=0;i<maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=maxRangeRadius)
		i = maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;
	
	free(intervals);
	//printf("accIntervals=%d, powerOf2=%d\n", accIntervals, powerOf2);
	return powerOf2;
}

void SZ_compress_args_double_NoCkRngeNoGzip_1D_pwr(unsigned char** newByteData, double *oriData, double globalPrecision, 
size_t dataLength, size_t *outSize, double min, double max)
{
	SZ_Reset();	
	size_t pwrLength = dataLength%segment_size==0?dataLength/segment_size:dataLength/segment_size+1;
	double* pwrErrBound = (double*)malloc(sizeof(double)*pwrLength);
	size_t pwrErrBoundBytes_size = sizeof(unsigned char)*pwrLength*2;
	unsigned char* pwrErrBoundBytes = (unsigned char*)malloc(pwrErrBoundBytes_size);
	
	compute_segment_precisions_double_1D(oriData, dataLength, pwrErrBound, pwrErrBoundBytes, globalPrecision);

	unsigned int quantization_intervals;
	if(optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_double_1D_pwr(oriData, dataLength, pwrErrBound);	
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = intvCapacity;
	//clearHuffmanMem();
	size_t i = 0, j = 0;
	int reqLength;
	double realPrecision = pwrErrBound[j++];	
	double medianValue = 0;
	double radius = fabs(max)<fabs(min)?fabs(min):fabs(max);
	short radExpo = getExponent_double(radius);
	
	computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);	

	int* type = (int*) malloc(dataLength*sizeof(int));
	//type[dataLength]=0;
		
	double* spaceFillingValue = oriData; //
	
	DynamicByteArray *resiBitLengthArray;
	new_DBA(&resiBitLengthArray, DynArrayInitLen);
	
	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);
	
	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);
	
	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);
	
	type[0] = 0;
	
	unsigned char preDataBytes[4] = {0};
	intToBytes_bigEndian(preDataBytes, 0);
	
	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;
	double last3CmprsData[3] = {0};

	DoubleValueCompressElement *vce = (DoubleValueCompressElement*)malloc(sizeof(DoubleValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));
						
	//add the first data	
	addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
	compressSingleDoubleValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_double(last3CmprsData, vce->data);
	//printf("%.30G\n",last3CmprsData[0]);	
		
	//add the second data
	type[1] = 0;
	addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);			
	compressSingleDoubleValue(vce, spaceFillingValue[1], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_double(last3CmprsData, vce->data);
	//printf("%.30G\n",last3CmprsData[0]);	
	
	int state;
	double lcf, qcf;	
	double checkRadius;
	double curData;
	double pred;
	double predAbsErr;
	double min_pred, minErr, minIndex;
	int a = 0;
	checkRadius = (intvCapacity-1)*realPrecision;
	double interval = 2*realPrecision;
	int updateReqLength = 0; //a marker: 1 means already updated
	
	for(i=2;i<dataLength;i++)
	{
		curData = spaceFillingValue[i];
		if(i%segment_size==0)
		{
			realPrecision = pwrErrBound[j++];
			checkRadius = (intvCapacity-1)*realPrecision;
			interval = 2*realPrecision;
			updateReqLength = 0;
		}
		pred = 2*last3CmprsData[0] - last3CmprsData[1];
		//pred = last3CmprsData[0];
		predAbsErr = fabs(curData - pred);	
		if(predAbsErr<checkRadius)
		{
			state = (predAbsErr/realPrecision+1)/2;
			if(curData>=pred)
			{
				type[i] = intvRadius+state;
				pred = pred + state*interval;
			}
			else //curData<pred
			{
				type[i] = intvRadius-state;
				pred = pred - state*interval;
			}
			listAdd_double(last3CmprsData, pred);			
			continue;
		}
		
		//unpredictable data processing		
		if(updateReqLength==0)
		{
			computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);				
			reqBytesLength = reqLength/8;
			resiBitsLength = reqLength%8;
			updateReqLength = 1;		
		}
		
		type[i] = 0;
		addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
		
		compressSingleDoubleValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);

		listAdd_double(last3CmprsData, vce->data);	
	}//end of for
		
//	char* expSegmentsInBytes;
//	int expSegmentsInBytes_size = convertESCToBytes(esc, &expSegmentsInBytes);
	int exactDataNum = exactLeadNumArray->size;
	
	TightDataPointStorageD* tdps;
			
	new_TightDataPointStorageD(&tdps, dataLength, exactDataNum, 
			type, exactMidByteArray->array, exactMidByteArray->size,  
			exactLeadNumArray->array,  
			resiBitArray->array, resiBitArray->size, 
			resiBitLengthArray->array, resiBitLengthArray->size, 
			realPrecision, medianValue, (char)reqLength, quantization_intervals, pwrErrBoundBytes, pwrErrBoundBytes_size, radExpo);

//sdi:Debug
/*	int sum =0;
	for(i=0;i<dataLength;i++)
		if(type[i]==0) sum++;
	printf("opt_quantizations=%d, exactDataNum=%d, sum=%d\n",quantization_intervals, exactDataNum, sum);
*/
//	writeUShortData(type, dataLength, "compressStateBytes.sb");
//	unsigned short type_[dataLength];
//	SZ_Reset();
//	decode_withTree(tdps->typeArray, tdps->typeArray_size, type_);	
//	printf("tdps->typeArray_size=%d\n", tdps->typeArray_size);
		
	//free memory
	free_DBA(resiBitLengthArray);
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
		
	convertTDPStoFlatBytes_double(tdps, newByteData, outSize);
	
	int doubleSize=sizeof(double);
	if(*outSize>dataLength*doubleSize)
	{
		size_t k = 0, i;
		tdps->isLossless = 1;
		size_t totalByteLength = 3 + 4 + 1 + doubleSize*dataLength;
		*newByteData = (unsigned char*)malloc(totalByteLength);
		
		unsigned char dsLengthBytes[4];
		intToBytes_bigEndian(dsLengthBytes, dataLength);//4
		for (i = 0; i < 3; i++)//3
			(*newByteData)[k++] = versionNumber[i];
		
		if(SZ_SIZE_TYPE==4)
		{
			(*newByteData)[k++] = 16;	//=00010000	
		}
		else 
		{
			(*newByteData)[k++] = 80;
		}
		for (i = 0; i < SZ_SIZE_TYPE; i++)//4 or 8
			(*newByteData)[k++] = dsLengthBytes[i];

		
		if(sysEndianType==BIG_ENDIAN_SYSTEM)
			memcpy((*newByteData)+4+SZ_SIZE_TYPE, oriData, dataLength*doubleSize);
		else
		{
			unsigned char* p = (*newByteData)+4+SZ_SIZE_TYPE;
			for(i=0;i<dataLength;i++,p+=doubleSize)
				doubleToBytes(p, oriData[i]);
		}
		*outSize = totalByteLength;
	}
	
	free(pwrErrBound);
	
	free(vce);
	free(lce);
	free_TightDataPointStorageD(tdps);
	free(exactMidByteArray);
}


/**
 * 
 * Note: @r1 is high dimension
 * 		 @r2 is low dimension 
 * */
void SZ_compress_args_double_NoCkRngeNoGzip_2D_pwr(unsigned char** newByteData, double *oriData, double globalPrecision, size_t r1, size_t r2,
size_t *outSize, double min, double max)
{
	SZ_Reset();	
	size_t dataLength=r1*r2;
	int blockEdgeSize = computeBlockEdgeSize_2D(segment_size);
	size_t R1 = 1+(r1-1)/blockEdgeSize;
	size_t R2 = 1+(r2-1)/blockEdgeSize;
	double* pwrErrBound = (double*)malloc(sizeof(double)*R1*R2);
	size_t pwrErrBoundBytes_size = sizeof(unsigned char)*R1*R2*2;
	unsigned char* pwrErrBoundBytes = (unsigned char*)malloc(pwrErrBoundBytes_size);
	
	compute_segment_precisions_double_2D(oriData, pwrErrBound, r1, r2, R2, blockEdgeSize, pwrErrBoundBytes, min, max, globalPrecision);
	unsigned int quantization_intervals;
	if(optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_double_2D_pwr(oriData, r1, r2, R2, blockEdgeSize, pwrErrBound);
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = intvCapacity;	
	//clearHuffmanMem();	
	//printf("quantization_intervals=%d\n",quantization_intervals);
	
	size_t i=0,j=0,I=0,J=0; 
	int reqLength;
	double realPrecision = pwrErrBound[I*R2+J];	
	double pred1D, pred2D;
	double diff = 0.0;
	double itvNum = 0;
	double *P0, *P1;
	
	P0 = (double*)malloc(r2*sizeof(double));
	memset(P0, 0, r2*sizeof(double));
	P1 = (double*)malloc(r2*sizeof(double));
	memset(P1, 0, r2*sizeof(double));
		
	double medianValue = 0;
	double radius = fabs(max)<fabs(min)?fabs(min):fabs(max);
	short radExpo = getExponent_double(radius);
	int updateReqLength = 1;
	
	computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));
	//type[dataLength]=0;
		
	double* spaceFillingValue = oriData; //
	
	DynamicByteArray *resiBitLengthArray;
	new_DBA(&resiBitLengthArray, DynArrayInitLen);
	
	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);
	
	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);
	
	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);
	
	type[0] = 0;
	
	unsigned char preDataBytes[8];
	longToBytes_bigEndian(preDataBytes, 0);
	
	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	DoubleValueCompressElement *vce = (DoubleValueCompressElement*)malloc(sizeof(DoubleValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));
			
	/* Process Row-0 data 0*/
	type[0] = 0;
	addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
	compressSingleDoubleValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,8);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[0] = vce->data;

	/* Process Row-0 data 1*/
	pred1D = P1[0];
	diff = spaceFillingValue[1] - pred1D;

	itvNum =  fabs(diff)/realPrecision + 1;

	if (itvNum < intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + intvRadius;
		P1[1] = pred1D + 2 * (type[1] - intvRadius) * realPrecision;
	}
	else
	{
		type[1] = 0;

		addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
		compressSingleDoubleValue(vce, spaceFillingValue[1], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,8);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[1] = vce->data;
	}

    /* Process Row-0 data 2 --> data r2-1 */
	for (j = 2; j < r2; j++)
	{
		if(j%blockEdgeSize==0)
		{
			J++;
			realPrecision = pwrErrBound[I*R2+J];
			updateReqLength = 0;
		}

		pred1D = 2*P1[j-1] - P1[j-2];
		diff = spaceFillingValue[j] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + intvRadius;
			P1[j] = pred1D + 2 * (type[j] - intvRadius) * realPrecision;
		}
		else
		{
			if(updateReqLength==0)
			{
				computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
				reqBytesLength = reqLength/8;
				resiBitsLength = reqLength%8;
				updateReqLength = 1;
			}

			type[j] = 0;

			addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
			compressSingleDoubleValue(vce, spaceFillingValue[j], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,8);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[j] = vce->data;
		}
	}

	/* Process Row-1 --> Row-r1-1 */
	size_t index;
	for (i = 1; i < r1; i++)
	{	
		/* Process row-i data 0 */
		index = i*r2;
		J = 0;
		if(i%blockEdgeSize==0)
			I++;
		realPrecision = pwrErrBound[I*R2+J]; //J==0
		updateReqLength = 0;
		
		pred1D = P1[0];
		diff = spaceFillingValue[index] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + intvRadius;
			P0[0] = pred1D + 2 * (type[index] - intvRadius) * realPrecision;
		}
		else
		{
			if(updateReqLength==0)
			{
				computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
				reqBytesLength = reqLength/8;
				resiBitsLength = reqLength%8;
				updateReqLength = 1;
			}
			
			type[index] = 0;

			addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
			compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,8);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[0] = vce->data;
		}
									
		/* Process row-i data 1 --> r2-1*/
		for (j = 1; j < r2; j++)
		{
			index = i*r2+j;
			if(j%blockEdgeSize==0)
			{
				J++;
				realPrecision = pwrErrBound[I*R2+J];
				updateReqLength = 0;
			}
			pred2D = P0[j-1] + P1[j] - P1[j-1];

			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + intvRadius;
				P0[j] = pred2D + 2 * (type[index] - intvRadius) * realPrecision;
			}
			else
			{
				if(updateReqLength==0)
				{
					computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
					reqBytesLength = reqLength/8;
					resiBitsLength = reqLength%8;
					updateReqLength = 1;
				}

				type[index] = 0;

				addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
				compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,8);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[j] = vce->data;
			}
		}

		double *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}
		
	if(r2!=1)	
		free(P0);
	free(P1);
	int exactDataNum = exactLeadNumArray->size;
	
	TightDataPointStorageD* tdps;
			
	new_TightDataPointStorageD(&tdps, dataLength, exactDataNum, 
			type, exactMidByteArray->array, exactMidByteArray->size,  
			exactLeadNumArray->array,  
			resiBitArray->array, resiBitArray->size, 
			resiBitLengthArray->array, resiBitLengthArray->size, 
			realPrecision, medianValue, (char)reqLength, quantization_intervals, pwrErrBoundBytes, pwrErrBoundBytes_size, radExpo);

	//free memory
	free_DBA(resiBitLengthArray);
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
		
	convertTDPStoFlatBytes_double(tdps, newByteData, outSize);

	free(pwrErrBound);
	
	free(vce);
	free(lce);
	free_TightDataPointStorageD(tdps);	
	free(exactMidByteArray);
}

void SZ_compress_args_double_NoCkRngeNoGzip_3D_pwr(unsigned char** newByteData, double *oriData, double globalPrecision, 
size_t r1, size_t r2, size_t r3, size_t *outSize, double min, double max)
{
	SZ_Reset();	
	size_t dataLength=r1*r2*r3;
	
	int blockEdgeSize = computeBlockEdgeSize_3D(segment_size);
	size_t R1 = 1+(r1-1)/blockEdgeSize;
	size_t R2 = 1+(r2-1)/blockEdgeSize;
	size_t R3 = 1+(r3-1)/blockEdgeSize;
	double* pwrErrBound = (double*)malloc(sizeof(double)*R1*R2*R3);
	size_t pwrErrBoundBytes_size = sizeof(unsigned char)*R1*R2*R3*2;
	unsigned char* pwrErrBoundBytes = (unsigned char*)malloc(pwrErrBoundBytes_size);	
	
	compute_segment_precisions_double_3D(oriData, pwrErrBound, r1, r2, r3, R2, R3, blockEdgeSize, pwrErrBoundBytes, min, max, globalPrecision);	
	unsigned int quantization_intervals;
	if(optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_double_3D_pwr(oriData, r1, r2, r3, R2, R3, blockEdgeSize, pwrErrBound);
		updateQuantizationInfo(quantization_intervals);
	}	
	else
		quantization_intervals = intvCapacity;
	//clearHuffmanMem();
	size_t i=0,j=0,k=0, I = 0, J = 0, K = 0;
	int reqLength;
	double realPrecision = pwrErrBound[0];		
	double pred1D, pred2D, pred3D;
	double diff = 0.0;
	double itvNum = 0;
	double *P0, *P1;

	size_t r23 = r2*r3;
	size_t R23 = R2*R3;
	P0 = (double*)malloc(r23*sizeof(double));
	P1 = (double*)malloc(r23*sizeof(double));
	double radius = fabs(max)<fabs(min)?fabs(min):fabs(max);
	double medianValue = 0;
	short radExpo = getExponent_double(radius);
	int updateReqLength = 0;
	computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);	

	int* type = (int*) malloc(dataLength*sizeof(int));
	//type[dataLength]=0;

	double* spaceFillingValue = oriData; //
	
	DynamicByteArray *resiBitLengthArray;
	new_DBA(&resiBitLengthArray, DynArrayInitLen);

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	type[0] = 0;

	unsigned char preDataBytes[8];
	longToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	DoubleValueCompressElement *vce = (DoubleValueCompressElement*)malloc(sizeof(DoubleValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));


	///////////////////////////	Process layer-0 ///////////////////////////
	/* Process Row-0 data 0*/
	type[0] = 0;
	addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
	compressSingleDoubleValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,8);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[0] = vce->data;

	/* Process Row-0 data 1*/
	pred1D = P1[0];
	diff = spaceFillingValue[1] - pred1D;

	itvNum = fabs(diff)/realPrecision + 1;

	if (itvNum < intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + intvRadius;
		P1[1] = pred1D + 2 * (type[1] - intvRadius) * realPrecision;
	}
	else
	{
		if(updateReqLength==0)
		{
			computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
			reqBytesLength = reqLength/8;
			resiBitsLength = reqLength%8;
			updateReqLength = 1;
		}		
		
		type[1] = 0;

		addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
		compressSingleDoubleValue(vce, spaceFillingValue[1], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,8);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[1] = vce->data;
	}

    /* Process Row-0 data 2 --> data r3-1 */
	for (j = 2; j < r3; j++)
	{
		if(j%blockEdgeSize==0)
		{
			J++;
			realPrecision = pwrErrBound[J];
			updateReqLength = 0;
		}		
		pred1D = 2*P1[j-1] - P1[j-2];
		diff = spaceFillingValue[j] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + intvRadius;
			P1[j] = pred1D + 2 * (type[j] - intvRadius) * realPrecision;
		}
		else
		{
			if(updateReqLength==0)
			{
				computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
				reqBytesLength = reqLength/8;
				resiBitsLength = reqLength%8;
				updateReqLength = 1;
			}			

			type[j] = 0;

			addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
			compressSingleDoubleValue(vce, spaceFillingValue[j], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,8);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[j] = vce->data;
		}
	}

	/* Process Row-1 --> Row-r2-1 */
	size_t index;
	K = 0;
	for (i = 1; i < r2; i++)
	{
		/* Process row-i data 0 */
		index = i*r3;	

		J = 0;
		if(i%blockEdgeSize==0)
			I++;
		realPrecision = pwrErrBound[I*R3+J]; //J==0
		updateReqLength = 0;

		pred1D = P1[index-r3];
		diff = spaceFillingValue[index] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + intvRadius;
			P1[index] = pred1D + 2 * (type[index] - intvRadius) * realPrecision;
		}
		else
		{
			if(updateReqLength==0)
			{
				computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
				reqBytesLength = reqLength/8;
				resiBitsLength = reqLength%8;
				updateReqLength = 1;
			}		
						
			type[index] = 0;

			addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
			compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,8);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index] = vce->data;
		}

		/* Process row-i data 1 --> data r3-1*/
		for (j = 1; j < r3; j++) //note that this j refers to fastest dimension (lowest order)
		{
			index = i*r3+j;		
			if(j%blockEdgeSize==0)
			{
				J++;
				realPrecision = pwrErrBound[I*R3+J];
				updateReqLength = 0;
			}			
		
			pred2D = P1[index-1] + P1[index-r3] - P1[index-r3-1];

			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + intvRadius;
				P1[index] = pred2D + 2 * (type[index] - intvRadius) * realPrecision;
			}
			else
			{
				if(updateReqLength==0)
				{
					computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
					reqBytesLength = reqLength/8;
					resiBitsLength = reqLength%8;
					updateReqLength = 1;
				}						
				
				type[index] = 0;

				addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
				compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,8);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index] = vce->data;
			}
		}
	}


	///////////////////////////	Process layer-1 --> layer-r1-1 ///////////////////////////

	for (k = 1; k < r1; k++)
	{
		/* Process Row-0 data 0*/
		index = k*r23;			
		I = 0;
		J = 0;
		if(k%blockEdgeSize==0)
			K++;
		realPrecision = pwrErrBound[K*R23]; //J==0
		updateReqLength = 0;
		
		pred1D = P1[0];
		diff = spaceFillingValue[index] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + intvRadius;
			P0[0] = pred1D + 2 * (type[index] - intvRadius) * realPrecision;
		}
		else
		{
			if(updateReqLength==0)
			{
				computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
				reqBytesLength = reqLength/8;
				resiBitsLength = reqLength%8;
				updateReqLength = 1;
			}					
			
			type[index] = 0;

			addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
			compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,8);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[0] = vce->data;
		}


	    /* Process Row-0 data 1 --> data r3-1 */
		for (j = 1; j < r3; j++)
		{
			index = k*r23+j;	

			if(j%blockEdgeSize==0)
			{
				J++;
				realPrecision = pwrErrBound[K*R23+J];
				updateReqLength = 0;			
			}					
			pred2D = P0[j-1] + P1[j] - P1[j-1];
			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + intvRadius;
				P0[j] = pred2D + 2 * (type[index] - intvRadius) * realPrecision;
			}
			else
			{
				if(updateReqLength==0)
				{
					computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
					reqBytesLength = reqLength/8;
					resiBitsLength = reqLength%8;
					updateReqLength = 1;
				}						
				
				type[index] = 0;

				addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
				compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,8);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[j] = vce->data;
			}
		}

	    /* Process Row-1 --> Row-r2-1 */
		size_t index2D;
		for (i = 1; i < r2; i++)
		{
			/* Process Row-i data 0 */
			index = k*r23 + i*r3;

			J = 0;
			if(i%blockEdgeSize==0)
				I++;
			realPrecision = pwrErrBound[K*R23+I*R3+J]; //J==0
			updateReqLength = 0;			
			
			index2D = i*r3;		
			pred2D = P0[index2D-r3] + P1[index2D] - P1[index2D-r3];
			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + intvRadius;
				P0[index2D] = pred2D + 2 * (type[index] - intvRadius) * realPrecision;
			}
			else
			{
				if(updateReqLength==0)
				{
					computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
					reqBytesLength = reqLength/8;
					resiBitsLength = reqLength%8;
					updateReqLength = 1;
				}						
				
				type[index] = 0;

				addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
				compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,8);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}

			/* Process Row-i data 1 --> data r3-1 */
			for (j = 1; j < r3; j++)
			{
				index = k*r23 + i*r3 + j;
				
				if(j%blockEdgeSize==0)
				{
					J++;
					realPrecision = pwrErrBound[K*R23+I*R3+J];
					updateReqLength = 0;			
				}							
				index2D = i*r3 + j;
				pred3D = P0[index2D-1] + P0[index2D-r3]+ P1[index2D] - P0[index2D-r3-1] - P1[index2D-r3] - P1[index2D-1] + P1[index2D-r3-1];
				diff = spaceFillingValue[index] - pred3D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + intvRadius;
					P0[index2D] = pred3D + 2 * (type[index] - intvRadius) * realPrecision;
				}
				else
				{
					if(updateReqLength==0)
					{
						computeReqLength_double(realPrecision, radExpo, &reqLength, &medianValue);
						reqBytesLength = reqLength/8;
						resiBitsLength = reqLength%8;
						updateReqLength = 1;
					}							
					
					type[index] = 0;

					addDBA_Data(resiBitLengthArray, (unsigned char)resiBitsLength);
					compressSingleDoubleValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Double(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,8);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}
			}
		}

		double *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}
	if(r23!=1)
		free(P0);
	free(P1);
	int exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageD* tdps;

	new_TightDataPointStorageD(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitLengthArray->array, resiBitLengthArray->size, 
			realPrecision, medianValue, (char)reqLength, quantization_intervals, pwrErrBoundBytes, pwrErrBoundBytes_size, radExpo);

	//free memory
	free_DBA(resiBitLengthArray);
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);

	convertTDPStoFlatBytes_double(tdps, newByteData, outSize);

	free(pwrErrBound);

	free(vce);
	free(lce);
	free_TightDataPointStorageD(tdps);
	free(exactMidByteArray);
}
