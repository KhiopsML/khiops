Libraire Norm - sample2
-----------------------

Exemple minimal de projet utilisant les composant d'interface utilisateur et le generateur de code.
	Ouverture d'une fenetre avec deux champs de saisie, et une action utilisateur.

R�pertoire sample 2:
	Fichiers de parametres de Visual C++ NET
		sample2.sln, sample2.vcproj
	Sources C++ de lancement de l'application
		main.h, main.cpp
	Fichiers de sp�cifications pour le g�n�rateur de code
		PRWorker.dd
	Commande de lancement du g�n�rateur de code
		genereall.cmd
	Fichiers g�n�r�s (classe de travail et classe d'interface graphique)
		PRWorker.h, PRWorker.cpp
		PRWorkerView.h, PRWorkerView.cpp

Les fichiers sources g�n�r�s peuvent �tre modifi�s dans les sections pr�vues � cet effet.
 Il peuvent alors �tre reg�n�r�s suite � modification des sp�cifications (fichier .dd), en pr�servant 
 le code utilisateur sp�cifique.
 

R�f�rences documentaires pour approfondir:
	- le generateur de code
	- l'interface utilisateur
	- la gestion de la memoire
