set path=%KNI_HOME%/bin;%path%
python python\KNIRecodeMTFiles.py -d data/ModelingSpliceJunction.kdic SNB_SpliceJunction ^
     -i data/SpliceJunction.txt 1 -s DNA data/SpliceJunctionDNA.txt 1 -o R_SpliceJunction_python.txt