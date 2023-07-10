// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "Timer.h"
#include "FileService.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tour de Hanoi
// Une tour de Hanoi est une pile de disques dont les taille sont decroissantes du bas vers le haut.
// Les tours de Hanoi sont un jeu de reflexion consistant a deplacer des disques de diametres differents
// d'une tour de « depart » a une tour d'« arrivee » en passant par une tour « intermediaire », et ceci
// en un minimum de coups, tout en respectant les regles suivantes:
//    . on ne peut deplacer plus d'un disque a la fois,
//    . on ne peut placer un disque que sur un autre disque plus grand que lui ou sur un emplacement vide.
// Cf. https://en.wikipedia.org/wiki/Tower_of_Hanoi
class HanoiTower : public Object
{
public:
	// Constructeur
	HanoiTower();
	~HanoiTower();

	// Initialisation avec une hauteur maximale et un nombre disque de tailles decroissantes
	void Initialize(int nTowerHeight, int nTowerDiskNumber, const ALString& sTowerName);

	// Nom de la tour
	const ALString& GetName() const;

	// Taille de la tour
	int GetHeight() const;

	// Nombre de disque de la tour
	int GetDiskNumber() const;

	// Taille d'un disque a une hauteur donnees (0 si pas de disque)
	int GetDiskSizeAt(int nDiskHeight) const;

	// Ajout d'un disque d'une taille donnees
	void AddDisk(int nDiskSize);

	// Suprression du disque du dessus, en renvoyant sa taille
	int RemoveDisk();

	// Resolution du probleme des tours de Hanoi, la toru courante etant consideree comme celle de depart
	// Le fichier en argument est optionnel (peut etre vide, pour stocker tous les deplacerment de disque d'une tout
	// a une autre On renvoie le nombre d'etapes utilisees
	longint SolveHanoiProblem(HanoiTower* destinationTower, HanoiTower* intermediateTower,
				  const ALString& sMoveStepsTraceFileName);

	// Verification de l'integrite de la tour
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test, de resolution du probleme pour de tailles de tours de 1 a 30
	// Affichage des temps moyens de resolution par taille
	static void Test();

	////////////////////////////////////////////////////
	// Implementation
protected:
	// Deplacement de la partie haut de la tour vers la tour destination en se servant de la tour intermediaire
	longint MoveSubTower(int nSubTowerDiskNumber, HanoiTower* destinationTower, HanoiTower* intermediateTower);

	// Gestion d'une tour
	ALString sName;
	int nHeight;
	int nDiskNumber;
	IntVector ivDisks;

	// Gestion de la resolution du probleme
	static fstream fstsMoveStepsTraceFileName;
	static boolean bTrace;
};
