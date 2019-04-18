#!/bin/bash

mkdir results

for i in {1..10}
do
	mkdir ./results/1E-$i
	mkdir ./results/1E-$i/zfp-0.4.1
	mkdir ./results/1E-$i/zfp-0.5.1
	mkdir ./results/1E-$i/zfp-0.5.4
	
	mkdir ./results/1E-$i/SZ-1.3
	mkdir ./results/1E-$i/SZ-1.4.9-beta
	mkdir ./results/1E-$i/SZ-1.4.11
	mkdir ./results/1E-$i/SZ-2.0.2.1
	mkdir ./results/1E-$i/ABS_SZ-2.0.2.1
done

