// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/////////////////////////////////////////
//			Projet KhioLog
//		Copyright Orange 2008
//          Vincent LEMAIRE
//
//  Carine Hue
//  GFI Informatique
/////////////////////////////////////////
#pragma once

#include "KWLearningProblem.h"
#include "CMModelingSpec.h"

////////////////////////////////////////////////////////////////
/// Classe CMLearningProblem :  Gestion de l'apprentissage avec predicteur majoritaire
class CMLearningProblem : public KWLearningProblem
{
public:
	/// Constructeur
	CMLearningProblem();
	~CMLearningProblem();

	// Recherche des predicteurs a utiliser
	// Redefinition de la methode pour rechercher le predicteur majoritaire
	void CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors);
};

////////////////////////////////////////////////////////////////////////////////
/// Classe CMAnalysisSpec : Analysis parameters for Classifieur majoritaire
class CMAnalysisSpec : public KWAnalysisSpec
{
public:
	/// Constructeur
	CMAnalysisSpec();
	~CMAnalysisSpec();
};
