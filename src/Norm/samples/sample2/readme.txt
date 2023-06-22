Libraire Norm - sample2
-----------------------

Exemple minimal de projet utilisant les composant d'interface utilisateur et le generateur de code.
	Ouverture d'une fenetre avec deux champs de saisie, et une action utilisateur.

Répertoire sample 2:
	Fichiers de parametres de Visual C++ NET
		sample2.sln, sample2.vcproj
	Sources C++ de lancement de l'application
		main.h, main.cpp
	Fichiers de spécifications pour le générateur de code
		PRWorker.dd
	Commande de lancement du générateur de code
		genereall.cmd
	Fichiers générés (classe de travail et classe d'interface graphique)
		PRWorker.h, PRWorker.cpp
		PRWorkerView.h, PRWorkerView.cpp

Les fichiers sources générés peuvent être modifiés dans les sections prévues à cet effet.
 Il peuvent alors être regénérés suite à modification des spécifications (fichier .dd), en préservant 
 le code utilisateur spécifique.
 

Références documentaires pour approfondir:
	- le generateur de code
	- l'interface utilisateur
	- la gestion de la memoire
