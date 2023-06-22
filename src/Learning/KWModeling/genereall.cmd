@echo off

echo. > genere.log

rem La partie vue est geree dans la librairie KWUserInterface
genere -noview KWTrainParameters "Train parameters" KWTrainParameters.dd >> genere.log
genere -outputdir ..\KWUserInterface -nomodel -noarrayview KWTrainParameters "Train parameters" KWTrainParameters.dd >> genere.log

rem La partie vue est geree dans la librairie KWUserInterface
genere -noview KWSelectionParameters "Selection parameters" KWSelectionParameters.dd >> genere.log
genere -outputdir ..\KWUserInterface -nomodel -noarrayview KWSelectionParameters "Selection parameters" KWSelectionParameters.dd >> genere.log
