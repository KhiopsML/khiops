@echo off

if exist ..\KWUserInterface\KWTrainParametersView.h copy ..\KWUserInterface\KWTrainParametersView.h
if exist ..\KWUserInterface\KWTrainParametersView.cpp copy ..\KWUserInterface\KWTrainParametersView.cpp
genere -noarrayview KWTrainParameters "Train parameters" KWTrainParameters.dd
copy KWTrainParametersView.h ..\KWUserInterface
copy KWTrainParametersView.cpp ..\KWUserInterface
del KWTrainParametersView.h
del KWTrainParametersView.cpp 

if exist ..\KWUserInterface\KWSelectionParametersView.h copy ..\KWUserInterface\KWSelectionParametersView.h
if exist ..\KWUserInterface\KWSelectionParametersView.cpp copy ..\KWUserInterface\KWSelectionParametersView.cpp
genere -noarrayview KWSelectionParameters "Selection parameters" KWSelectionParameters.dd
copy KWSelectionParametersView.h ..\KWUserInterface
copy KWSelectionParametersView.cpp ..\KWUserInterface
del KWSelectionParametersView.h
del KWSelectionParametersView.cpp 
