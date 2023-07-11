@echo off

echo. > genere.log

rem La partie vue est geree dans la librairie KWUserInterface
call ..\..\genere -noview KWTrainParameters "Train parameters" KWTrainParameters.dd >> genere.log
call ..\..\genere -outputdir ..\KWUserInterface -nomodel -noarrayview KWTrainParameters "Train parameters" KWTrainParameters.dd >> genere.log

rem La partie vue est geree dans la librairie KWUserInterface
call ..\..\genere -noview KWSelectionParameters "Selection parameters" KWSelectionParameters.dd >> genere.log
call ..\..\genere -outputdir ..\KWUserInterface -nomodel -noarrayview KWSelectionParameters "Selection parameters" KWSelectionParameters.dd >> genere.log
