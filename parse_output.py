import sys
import os
import numpy as np



tolRange = 10
numVars = 4


def parse_output(compressorName, tols, mode=-1):

	modeDir = "" if mode == -1 else str(mode) + "/"

	CompressionValues = np.ones((tolRange, numVars))

	for tolIdx, tol_str in enumerate(tols):

		inputFileName = "results/" + compressorName + "/" + modeDir + tol_str + "/100.txt"
		
		if os.path.isfile(inputFileName) == False or os.path.getsize(inputFileName)==0:
			print 'skipping: ' + inputFileName
			continue

	
		var = 0

		for line in open(inputFileName).readlines():
     
			if "Compression Factor =" in line:
				#print str(float(line.split("=")[1]))
				CompressionValues[tolIdx][var] = float(line.split("=")[1])
 				var += 1

	
	outputFileName = "results/"+ compressorName + "/" + modeDir + "output"
	np.save(outputFileName, CompressionValues)


tols=['1E-10','1E-9','1E-8','1E-7','1E-6','1E-5','1E-4','1E-3','1E-2','1E-1']
compressorName=sys.argv[1] #"SZ-2.0.2.1"
mode = sys.argv[2] if len(sys.argv) > 2 else - 1

parse_output(compressorName,tols,mode=mode)



