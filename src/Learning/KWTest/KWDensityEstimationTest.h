// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWContinuous.h"
#include "KWStat.h"

//////////////////////////////////////////////////////////////////////
// Classe KWDensityEstimationTest
// Evaluation de la methode d'evaluation de densite conditionnelle
// d'une variable continue, sur quelques jeux de donnees artificiels
class KWDensityEstimationTest : public Object
{
public:
	// Constructeur
	KWDensityEstimationTest();
	~KWDensityEstimationTest();

	// Methode de test globale
	static void Test();

	// Methode de test complete avec un jeu d'essai
	void TestOneDataset();

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Recherche de la taille de la grille de meilleur cout pour un jeu de donnees
	int SearchBestInstanceGridSize(ContinuousVector* cvXValues, ContinuousVector* cvYValues);

	// Cout d'une grille
	double ComputeInstanceGridCost(ObjectArray* oaInstanceGrid);

	// Initialisation des valeurs d'un jeu de donnees aleatoire
	// Memoire: les vecteurs de X et Y doivent etre alloue par l'appelant
	void InitializeRandomDatasetValues(int nRandomSeed, int nInstanceNumber, ContinuousVector* cvXValues,
					   ContinuousVector* cvYValues);

	// Initialisation des valeurs d'un jeu de donnees en dependance lineaire
	void InitializeLinearDatasetValues(int nRandomSeed, int nInstanceNumber, ContinuousVector* cvXValues,
					   ContinuousVector* cvYValues);

	// Initialisation des valeurs d'un jeu de donnees en dependance lineaire bruite
	void InitializeNoisyLinearDatasetValues(int nRandomSeed, int nInstanceNumber, double dNoiseRate,
						ContinuousVector* cvXValues, ContinuousVector* cvYValues);

	// Initialisation des valeurs d'un jeu de donnees en dependance lineaire avec bruit gaussien
	void InitializeGaussianLinearDatasetValues(int nRandomSeed, int nInstanceNumber, double dNoiseRate,
						   ContinuousVector* cvXValues, ContinuousVector* cvYValues);

	// Creation d'un jeu d'essai en dependance bilineaire
	void InitializeBiLinearDatasetValues(int nRandomSeed, int nInstanceNumber, ContinuousVector* cvXValues,
					     ContinuousVector* cvYValues);

	// Affichage d'un jeu de donnees
	void WriteDataset(ostream& ost, ContinuousVector* cvXValues, ContinuousVector* cvYValues);

	// Calcul des instances tombant dans les cellules d'une grille carree
	// nAxisCellNumber*nAxisCellNumber
	// Le resultat est fourni sous la forme d'un tableau (selon X) de vecteur
	// de comptage (selon Y) des instances des cellules de la grille
	// Memoire: le resultat et son contenu sont a detruire par l'appelant
	ObjectArray* ComputeInstanceGrid(ContinuousVector* cvXValues, ContinuousVector* cvYValues, int nAxisCellNumber,
					 Continuous cMinValue, Continuous cMaxValue);

	// Destruction d'une grille d'instances
	void DeleteInstanceGrid(ObjectArray* oaInstanceGrid);

	// Affichage d'une grille d'instances
	void WriteInstanceGrid(ostream& ost, ObjectArray* oaInstanceGrid);

	// Nombre d'instance d'un vecteur d'instance et d'une grille d'instances
	int ComputeInstanceGridTotalFrequency(ObjectArray* oaInstanceGrid);
	int ComputeXIntervalTotalFrequency(ObjectArray* oaInstanceGrid, int nX);
	int ComputeYIntervalTotalFrequency(ObjectArray* oaInstanceGrid, int nY);
};

//////////////////////////////////////////////////////////////////////
// Classe KWDensityEstimationStudy
// Comparaison entre estimation de densite jointe et
// estimation de densite conditionnelle,
// sur des jeu d'essai equidistribue sur I colonne et J lignes, avec
// une seul cellule par colonne d'effectif non nul
class KWDensityEstimationStudy : public Object
{
public:
	// Constructeur
	KWDensityEstimationStudy();
	~KWDensityEstimationStudy();

	// Methode de test globale
	static void Test();

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////
	// Cas de l'estimation de densite jointe

	// Calcul de la valeur de N pour minimum pour rendre le modele multi-cellule preferable
	int ComputeJointDensityMinFrequency(int nI, int nJ);

	// Difference de cout entre modele multi-cellule et modele mono-cellule
	double JointDensityDeltaCost(int nI, int nJ, int nN);

	//////////////////////////////////////////////////
	// Cas de l'estimation de densite conditionnelle

	// Calcul de la valeur de N pour minimum pour rendre le modele multi-cellule preferable
	int ComputeConditionalDensityMinFrequency(int nI, int nJ);

	// Difference de cout entre modele multi-cellule et modele mono-cellule
	double ConditionalDensityDeltaCost(int nI, int nJ, int nN);
};
