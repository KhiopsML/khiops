@echo off

call ..\..\..\genere -noarrayview PRWorker "Employe" PRWorker.dd > genere.log
call ..\..\..\genere              PRChild "Enfant" PRChild.dd >> genere.log
call ..\..\..\genere -noarrayview PRAddress "Adresse" PRAddress.dd >> genere.log
