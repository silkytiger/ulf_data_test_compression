#!/bin/bash


#format: ./results/compressor/mode/tolerance

mkdir results

for compressor in SZ-1.3 SZ-1.4.9-beta SZ-1.4.11 SZ-2.0.2.1
do
	mkdir ./results/$compressor

	for mode in 0 1
	do
		mkdir ./results/$compressor/$mode

		for exp in {1..10}
		do
			mkdir ./results/$compressor/$mode/1E-$exp
		done
	done
done


#for i in {1..10}
#do
#	mkdir ./results/1E-$i
#	mkdir ./results/1E-$i/zfp-0.4.1
#	mkdir ./results/1E-$i/zfp-0.5.1
#	mkdir ./results/1E-$i/zfp-0.5.4
	
#	mkdir ./results/1E-$i/SZ-1.3
#	mkdir ./results/1E-$i/SZ-1.4.9-beta
#	mkdir ./results/1E-$i/SZ-1.4.11
#	mkdir ./results/1E-$i/SZ-2.0.2.1
#	mkdir ./results/1E-$i/ABS_SZ-2.0.2.1
#done

