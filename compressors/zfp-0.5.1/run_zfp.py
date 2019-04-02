import sys
import glob
import os
import time


def setupConfigFile(chkptNum):
	outfile = open(INP_OUT, "w")
	for l in open(INP_IN, "r").readlines():
		if "NSTEPMAX" in l:
			split = l.split("=")
			split[-1] = "= " + str(chkptNum * iter + iter) + "\n"
			outfile.write(" ".join(split))
		else:
			outfile.write(l.strip() + "\n")


def getInputFile(i, iter):
	tstep = str((i-1)*iter + iter)
	print "Final time ", tstep 
	if tstep == 0:
		return "initial.q"
	for file in glob.glob("*.q"):
		if tstep in file:
			return file
	for file in glob.glob("*.restart"):
		if tstep in file:
			return file
	return "initial.q"

BASE_DIR = "/zfs/fthpc/"
ZFP = "python /home/ptriant/Research/zfp/zfp-python/cycle_zfp.py "
ZFP_DLL = "/home/ptriant/Research/zfp/zfp-python/zfp-0.4.1/examples/simple.dll"
ERR_TOL = 1e-5
INP_IN = "master.inp"
INP_OUT = "plascomcm.inp"

iter = 2000
if len(sys.argv) > 1:
	iter = int(sys.argv[1])
nChkpt = 1
if len(sys.argv) > 2:
	nChkpt = int(sys.argv[2])
if len(sys.argv) > 3:
	ERR_TOL = float(sys.argv[3])
print "Num iter till checkpoint", iter
print "Num checkpoints", nChkpt
print "Error Tolerance", ERR_TOL

#modify inp file
setupConfigFile(0)

# run sim
cmd_prefix = "mpirun -n 4 ./plascomcm "
input_initial = "startup.q"
input = "initial.q"

os.system("cp " + input_initial + " " + input)
cmd_suffix = " "
for i in xrange(nChkpt+1):
	if i != 0:
		#input = getInputFile(i, iter)  
		cmd = ZFP +input + " compress.q "+ str(ERR_TOL) + " 1 " + ZFP_DLL
		print cmd
		print "cp compress.q " + input
		os.system(cmd)
		os.system("chmod g+w compress.q")
		os.system("chmod o+w compress.q")
		os.system("cp compress.q " + input)
		'''
		else:
			print "Launch: ", cmd_prefix + input_initial + cmd_suffix
			#os.system(cmd_prefix + input_initial + cmd_suffix)
			time.sleep(2)
			continue
		'''
	print "Launch: ", cmd_prefix + input + cmd_suffix
	os.system(cmd_prefix + input + cmd_suffix)
	time.sleep(2)
	input = getInputFile(i+1, iter) 
	print input, "\n-----------------------------------\n\n"
