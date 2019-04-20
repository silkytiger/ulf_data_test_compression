import sys
import numpy as np


def parse_output(sz_version):

	tolRange = 9
	numVars = 4

	CompressionValues = np.zeros((tolRange, numVars))

	for tolIdx in xrange(tolRange):

		var = 0
		inputFileName = "results/1E-" + str(tolIdx + 1) + "/" + sz_version + "/100.txt"

		for line in open(inputFileName).readlines():
     
			if "Compression Factor =" in line:
				#print str(float(line.split("=")[1]))
				CompressionValues[tolIdx][var] = float(line.split("=")[1])
 				var += 1

	outputFileName = "results/" + sz_version + "_output"
	np.save(outputFileName, CompressionValues)


