@echo off

echo. > genere.log

call ..\..\genere -noview -noarrayview -super MHHistogramSpec MHGenumHistogramSpec "Histogram parameters" MHGenumHistogramSpec.dd >> genere.log
