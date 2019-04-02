import h5py
import numpy as np
import sys
import ctypes
import re

# setup variables
TOL=float(sys.argv[1])
ITER=int(sys.argv[2])
compressor_path=sys.argv[3]
compressor_dll = ctypes.cdll.LoadLibrary(compressor_path)
cycle = compressor_dll.cycle
ZFP=re.search("(ZFP|zfp)",compressor_path) != None


f = h5py.File('results_snapshot_1029840.h5', 'r')
dset = f[u'piece0']

# i do not think that I will be needing these
cell_locations = dset[u'cell_locations']
cell_types = dset[u'cell_types']
cells = dset[u'cells']

# retrieve the data sets that I will need
points = dset[u'points'].value
point_data = dset[u'point_data']
pressure = point_data[u'pressure'].value
velocity = point_data[u'velocity'].value

# separate x, y, and z components of velocity
xVelocity,yVelocity,zVelocity = np.split(velocity,3, axis=1)




if ZFP:
	print "ZFP"
	for i,nparr in enumerate([pressure,xVelocity,yVelocity,zVelocity]):
		print "\n============================="
		print "Compressing CV=           " + str(i)

		n = nparr.shape
		nx=n[0]
		ny=1
		nz=1

		rArr = np.zeros(nx*ny*nz)
		p = 0
		for i in xrange(nx):
			rArr[p] = nparr[i]
			p += 1

		iArr = rArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

		nArr = np.zeros(nx*ny*nz)
		oArr = nArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

		print "Shape of array          " + str(nx) + "          " + str(ny) + "           " + str(nz)
		cycle(iArr,nx,ny,nz,ctypes.c_double(TOL), ITER, oArr)
		print "=============================\n"
		print nArr
		grid1= np.copy(nparr)
		p = 0
		for i in xrange(nx):
			grid1[i] = nArr[p]
			p += 1

		print np.max(np.abs(nparr-grid1))


else:
	print "SZ"

	sz_config = sys.argv[4]
	SZ_Init = compressor_dll.SZ_Init
	SZ_Finalize = compressor_dll.SZ_Finalize

	for i,nparr in enumerate([pressure,xVelocity,yVelocity,zVelocity]):
		print "\n============================="
		print "Compressing CV=           " + str(i)

		n = nparr.shape
		nx=n[0]
		ny=1
		nz=1

		rArr = np.zeros(nx)
		#rArr = np.zeros(nx*ny*nz)
		p = 0
		for i in xrange(nx):
			rArr[p] = nparr[i]
			p += 1

		iArr = rArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

		nArr = np.zeros(nx)
		#nArr = np.zeros(nx*ny*nz)
		oArr = nArr.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

		print "Shape of array          " + str(nx) + "          " + str(ny) + "           " + str(nz)
		SZ_Init(ctypes.c_char_p(sz_config))
		cycle(iArr,nx,ny,nz,ctypes.c_double(TOL), ITER, oArr,1)
		SZ_Finalize()
		print "=============================\n"
		print nArr
		grid1= np.copy(nparr)
		p = 0
		for i in xrange(nx):
			grid1[i] = nArr[p]
			p += 1

		print np.max(np.abs(nparr-grid1))



