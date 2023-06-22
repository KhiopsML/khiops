Libraire Norm - sample3
-----------------------

Exemple de projet utilisant les composant d'interface utilisateur, le generateur de code.
	Fenetre principale avec deux onglets (une liste et une fiche), et une autre fenetre accessible par menu
	Ecriture d'un rapport (avec tri d'un tableau de donnees).

R�pertoire sample 3:
	Fichiers de parametres de Visual C++ NET
		sample3.sln, sample3.vcproj
	Sources C++ de lancement de l'application
		main.h, main.cpp
	Fichiers de sp�cifications pour le g�n�rateur de code
		PRWorker.dd, PRChild.dd, PRAddress.dd
	Commande de lancement du g�n�rateur de code
		genereall.cmd
	Fichiers g�n�r�s
	  Classes de travail
		PRWorker.h, PRWorker.cpp
		PRChild.h, PRChild.cpp
		PRAddress.h, PRAddress.cpp
	  Classes d'interface utilisateur (fiches et listes)
		PRWorkerView.h, PRWorkerView.cpp
		PRChildView.h, PRChildView.cpp
		PRChildArrayView.h, PRChildArrayView.cpp
		PRAddressView.h, PRAddressView.cpp

Le code g�n�r� est modifi� essentiellement pour assembler 
 d'une part les objets de travail:
  	un employe (PRWorker) est d�crit au moyen
  		- d'un tableau (ObjectArray) d'enfants (PRChild)
  		- d'une adresse personnelle (KWAddress)
  		- d'une adresse professionnelle (KWAddress)
 d'autre part l'interface utilisateur permettant d'interagir avec ces donn�es:
 	une fiche d'employe (PRWorkerView) est constituee
 		- d'une sous-liste d'enfant (PRChildArrayView) dans un onglet
 		- d'une fiche adresse perdonnelle (PRAddressView) dans un onglet
 	et comporte deux item de menu:
 		- edition de l'adresse professionnelle
 		- ecriture d'un rapport

Le moteur d'interface graphique g�re automatiquement les interactions entre les objets de travail et
 l'interface utilisateur.
 

R�f�rences documentaires pour approfondir:
	- le generateur de code
	- l'interface utilisateur
	- la gestion de la memoire
	- les classes container (ObjectArray, ObjectList, ObjectDictionary...)
	- la classe FileService
