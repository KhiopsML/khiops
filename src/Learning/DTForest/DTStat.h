// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWStat.h"
#include "Vector.h"

// #include "KWVersion.h"

//////////////////////////////////////////////////////////////////////////
// Bibliotheque de fonctions statistiques
// Notamment, implementation specifique des fonction Khiops pour
// l'evaluation de la loi du Khi2 et de ses variations (DeltaKhi2) pour
// de tres grandes valeurs de Khi2 et de degres de libertes.
// Reference: Note Technique NT/FTR&D/7339 - Annexe
//   Khiops: discretisation des attributs numeriques pour le Data Mining
class DTStat : public KWStat
{
public:
	/////////////////////////////////////////////////////////////
	// Fonction statistique standard

	// Logarithme du nombre de catalan
	static double LnCatalan(int nValue);

	// Logarithme du petit nombre de schroder
	static double LnSchroder(int nValue);

	// petit nombre de Schroder
	static double NbSchroder(int nValue);

	// Logarithme du nombre de Bell "generalise"
	// Nombre de partition de n elements en k classes (eventuellements vides)
	static double LnBell(int n, int k);

	// derivee du codage RISSANEN 2log(n) + 1
	static double NumbersCodeLength1(int n);

	// derivee du codage RISSANEN log(n)+ 2log(log(n)) + 1
	static double NumbersCodeLength2(int n);

	// renvoie un nombre entier aleatoire selon la distribution de rissanen bornee
	static int RissanenMaxNumber(int nMax);

	////////////////////////////
	// Test
	static void Test();

	///////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////
	// Methodes internes

	// Tableau des valeurs de la fonction logarithme du nombre de Catalan
	static DoubleVector vLnCatalan;

	// Tableau des valeurs de la fonction logarithme du nombre de Schroder
	static DoubleVector vLnSchroder;
	static DoubleVector vSchroder;
	static DoubleVector vProbaRissanen;
};