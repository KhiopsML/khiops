@echo off

echo. > genere.log

call ..\..\genere -noarrayview MHHistogramSpec "Histogram parameters" MHHistogramSpec.dd >> genere.log
