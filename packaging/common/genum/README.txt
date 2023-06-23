Usage: genum [VALUES] [HISTOGRAM]
 Compute histogram from the values in VALUES file, using the G-Enum method.
 The resulting histogram is output in HISTOGRAM file, with the lower value, upper value,
  length, frequency, probability and density per bin, using the csv format.
        -h      display this help and exit
        -v      display version information and exit
       
Files:
  genum.exe: executable for Windows
  README.txt: this file
  LICENSE.txt: licence file
  gaussian_values.txt: sample input file with values
  gaussian_histogram.csv: sample output file with resulting histogram


Usage example: genum gaussian_values.txt gaussian_histogram.txt


Reference:
 Valentina Zelaya Mendizabala, Marc Boullé and Fabrice Rossi, 
 Fast and fully-automated histograms for large-scale data sets, 
 Computational Statistics & Data Analysis, 2022
 