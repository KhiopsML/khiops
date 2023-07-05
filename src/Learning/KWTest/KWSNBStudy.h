// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWStat.h"

/////////////////////////////////////////////////////////////////////////
// Classe KWSNBStudy
// Etude du classifieur Bayesien naif selectif, avec plusieurs algorithmes
// de selection de variables: NB, SNB(MAP), SNB(BMA), SNB(CMA)
// On dispose de deux variables explicatives boolennes X1 et X2, et d'une variable
// a expliquer boolenne Y
// On cherche a estimer P(Y | X1, X2) avec un classifieur Bayesien naif
// L'etude consiste a evaluer toutes les configuration des 8 probabilites
// P000, P001, P010, P011, P100, P101, P110, P111
class KWSNBStudy : public Object
{
public:
	// ructeur
	KWSNBStudy();
	~KWSNBStudy();

	// Methode de test principale
	static void Test();

protected:
	///////////////////////////////////////////////////////
	///// Implementation

	// Etude
	void Study();
	void StudyOneCase(int nCaseIndex);

	// Calcul des probabilites conditionnelles vraies
	void ComputeTrueConditionalProbs();
	void ComputeNullConditionalProbs();
	void ComputeX1ConditionalProbs();
	void ComputeX2ConditionalProbs();
	void ComputeNBConditionalProbs();
	void ComputeMAPConditionalProbs();
	void ComputeBMAConditionalProbs();
	void ComputeCMAConditionalProbs();

	// Calcul de la divergence de Kullback-Leibler sum {Pijk log P(k|ij)/Q(k|ij)}
	double ComputeDKL(DoubleVector* dvEstimatedProbs);

	// Calcul de l'esperance de la log vraisemblance negative
	double ComputeMeanNLPD(DoubleVector* dvEstimatedProbs);

	// Affichage d'un vecteur de probabilites
	void DisplayProbs(const ALString& sLabel, DoubleVector* dvProbs, ostream& ost);

	// Index des probabilites
	enum
	{
		I000,
		I001,
		I010,
		I011,
		I100,
		I101,
		I110,
		I111
	};

	// Nombre de pas de discretisation des probabilites
	int nProbBinNumber;

	// Probabilites
	double dP000;
	double dP001;
	double dP010;
	double dP011;
	double dP100;
	double dP101;
	double dP110;
	double dP111;

	// Vecteurs de probabilites conditionnelles
	DoubleVector dvTrueConditionalProbs;
	DoubleVector dvNullConditionalProbs;
	DoubleVector dvX1ConditionalProbs;
	DoubleVector dvX2ConditionalProbs;
	DoubleVector dvNBConditionalProbs;
	DoubleVector dvMAPConditionalProbs;
	DoubleVector dvBMAConditionalProbs;
	DoubleVector dvCMAConditionalProbs;
};