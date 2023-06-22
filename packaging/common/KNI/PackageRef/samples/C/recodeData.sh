#!/bin/bash

./KNIRecodeFile ../data/ModelingIris.kdic SNB_Iris ../data/Iris.txt R_Iris.txt
./KNIRecodeMTFiles -d ../data/ModelingSpliceJunction.kdic SNB_SpliceJunction -i ../data/SpliceJunction.txt 1 -s DNA  ../data/SpliceJunctionDNA.txt 1 -o R_SpliceJunction.txt
