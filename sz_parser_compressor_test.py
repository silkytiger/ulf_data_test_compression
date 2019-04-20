import h5py
import numpy as np
import sys
import ctypes
import re

# setup variables
TOL=1E-5
ITER=1
COMPRESSOR="SZ-1.4.11.0"
MODE=1 #Abs=0,REL=1
compressor_path="./compressors/"+COMPRESSOR+"/sz_cycle.dll"
compressor_dll = ctypes.cdll.LoadLibrary(compressor_path)
cycle = compressor_dll.cycle

datafile="./compressors/"+COMPRESSOR+"/"+COMPRESSOR+"/example/testdata/testdouble_8_8_128.txt"
data = np.fromfile(datafile, dtype=np.float64, sep="\r\n")

if (True):
	print "SZ"

	sz_config = "./compressors/"+COMPRESSOR+"/sz.config"
	SZ_Init = compressor_dll.SZ_Init
	SZ_Finalize = compressor_dll.SZ_Finalize

	nparr=data
	n = nparr.shape
	nx=n[0]
	ny=0
	nz=0

	rArr = np.zeros(nx)
	p = 0
	for i in xrange(nx):
		rArr[p] = nparr[i]
		p += 1

	iArr = rArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

	nArr = np.zeros(nx)
	oArr = nArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

	print "Shape of array          " + str(nx) + "          " + str(ny) + "           " + str(nz)
	SZ_Init(ctypes.c_char_p(sz_config))
	cycle(iArr,nx,ny,nz,ctypes.c_double(TOL), ITER, oArr, MODE)
	SZ_Finalize()
	print "=============================\n"
	print nArr
	grid1= np.copy(nparr)
	p = 0
	for i in xrange(nx):
		grid1[i] = nArr[p]
		p += 1

	print np.max(np.abs(nparr-grid1))
	grid1.tofile("test_result.txt",sep="\r\n")
	print grid1.shape

