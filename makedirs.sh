#!/bin/bash


#format: ./results/compressor/mode/tolerance

mkdir results

for compressor in SZ-1.3 SZ-1.4.9-beta SZ-1.4.11.0 SZ-2.0.2.1
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

#zfp does not have mode
for compressor in zfp-0.4.1 zfp-0.5.1 zfp-0.5.4
do
	mkdir ./results/$compressor

  for exp in {1..10}
  do
  	mkdir ./results/$compressor/1E-$exp
	done
done


