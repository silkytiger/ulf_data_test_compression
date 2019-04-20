/**
 *  @file io.h
 *  @author Sheng Di
 *  @date April, 2015
 *  @brief Header file for the whole io interface.
 *  (C) 2015 by Mathematics and Computer Science (MCS), Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _IO_H
#define _IO_H

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else
#define PATH_SEPARATOR ':'
#endif

#ifdef __cplusplus
extern "C" {
#endif

float** create2DArray_float(size_t m, size_t n);
void free2DArray_float(float** data, size_t m);
float*** create3DArray_float(size_t p, size_t m, size_t n);
void free3DArray_float(float*** data, size_t p, size_t m);
double** create2DArray_double(size_t m, size_t n);
void free2DArray_double(double** data, size_t m);
double*** create3DArray_double(size_t p, size_t m, size_t n);
void free3DArray_double(double*** data, size_t p, size_t m);
size_t checkFileSize(char *srcFilePath, int *status);

unsigned char *readByteData(char *srcFilePath, size_t *byteLength, int *status);
double *readDoubleData(char *srcFilePath, size_t *nbEle, int *status);
int8_t *readInt8Data(char *srcFilePath, size_t *nbEle, int *status);
int16_t *readInt16Data(char *srcFilePath, size_t *nbEle, int *status);
uint16_t *readUInt16Data(char *srcFilePath, size_t *nbEle, int *status);
int32_t *readInt32Data(char *srcFilePath, size_t *nbEle, int *status);
uint32_t *readUInt32Data(char *srcFilePath, size_t *nbEle, int *status);
int64_t *readInt64Data(char *srcFilePath, size_t *nbEle, int *status);
uint64_t *readUInt64Data(char *srcFilePath, size_t *nbEle, int *status);
float *readFloatData(char *srcFilePath, size_t *nbEle, int *status);
unsigned short* readShortData(char *srcFilePath, size_t *dataLength, int *status);

double *readDoubleData_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
int8_t *readInt8Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
int16_t *readInt16Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
uint16_t *readUInt16Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
int32_t *readInt32Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
uint32_t *readUInt32Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
int64_t *readInt64Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
uint64_t *readUInt64Data_systemEndian(char *srcFilePath, size_t *nbEle, int *status);
float *readFloatData_systemEndian(char *srcFilePath, size_t *nbEle, int *status);

void writeByteData(unsigned char *bytes, size_t byteLength, char *tgtFilePath, int *status);
void writeDoubleData(double *data, size_t nbEle, char *tgtFilePath, int *status);
void writeFloatData(float *data, size_t nbEle, char *tgtFilePath, int *status);
void writeData(void *data, int dataType, size_t nbEle, char *tgtFilePath, int *status);
void writeFloatData_inBytes(float *data, size_t nbEle, char* tgtFilePath, int *status);
void writeDoubleData_inBytes(double *data, size_t nbEle, char* tgtFilePath, int *status);
void writeShortData_inBytes(short *states, size_t stateLength, char *tgtFilePath, int *status);
void writeUShortData_inBytes(unsigned short *states, size_t stateLength, char *tgtFilePath, int *status);
void writeIntData_inBytes(int *states, size_t stateLength, char *tgtFilePath, int *status);
void writeUIntData_inBytes(unsigned int *states, size_t stateLength, char *tgtFilePath, int *status);
void writeLongData_inBytes(int64_t *states, size_t stateLength, char *tgtFilePath, int *status);
void writeULongData_inBytes(uint64_t *states, size_t stateLength, char *tgtFilePath, int *status);

void checkfilesizec_(char *srcFilePath, size_t *len, int *filesize);
void readbytefile_(char *srcFilePath, size_t *len, unsigned char *bytes, int *byteLength);
void readdoublefile_(char *srcFilePath, size_t *len, double *data, int *nbEle);
void readfloatfile_(char *srcFilePath, size_t *len, float *data, int *nbEle);
void writebytefile_(unsigned char *bytes, int *byteLength, char *tgtFilePath, size_t *len);
void writedoublefile_(double *data, size_t *nbEle, char *tgtFilePath, size_t *len);
void writefloatfile_(float *data, size_t *nbEle, char *tgtFilePath, size_t *len);

void writeStrings(int nbStr, char *str[], char *tgtFilePath, int *status);

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _IO_H  ----- */
