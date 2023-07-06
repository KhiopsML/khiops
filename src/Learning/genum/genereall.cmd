@echo off

echo. > genere.log

genere -noview -noarrayview -super MHHistogramSpec MHGenumHistogramSpec "Histogram parameters" MHGenumHistogramSpec.dd >> genere.log
