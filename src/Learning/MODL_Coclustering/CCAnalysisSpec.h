// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CCCoclusteringSpec.h"
#include "CCVarPartCoclusteringSpec.h"
#include "KWDataGridOptimizerParameters.h"

////////////////////////////////////////////////////////////
// Classe CCAnalysisSpec
//    Parameters
class CCAnalysisSpec : public Object
{
public:
	// Constructeur
	CCAnalysisSpec();
	~CCAnalysisSpec();

	// Parametrage du type de coclustering
	// A true pour un coclustering de type VarPart, instances * variables
	// A false sinon pour un coclustering de variables (valeur par defaut)
	void SetVarPartCoclustering(boolean bValue);
	boolean GetVarPartCoclustering() const;

	// Parametrage du coclustering
	CCCoclusteringSpec* GetCoclusteringSpec();

	// Parametrage du coclustering VarPart instances x variables
	CCVarPartCoclusteringSpec* GetVarPartCoclusteringSpec();

	// Libelles correspondant aux type de coclustering
	// pour les cas VartPart (coclustering instances x variables) ou standard (coclustering de variables)
	static const ALString GetCoclusteringLabelFromType(boolean bIsVarPartCoclustering);

	// Type correspond a un libelle de coclustering
	// Renvoie true dans la cas VarPart, false sinon
	static boolean GetCoclusteringTypeFromLabel(const ALString& sLabel);

	// Parametres d'optimisation
	KWDataGridOptimizerParameters* GetOptimizationParameters();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bVarPartCoclustering;
	CCCoclusteringSpec coclusteringSpec;
	CCVarPartCoclusteringSpec varPartCoclusteringSpec;
	KWDataGridOptimizerParameters optimizationParameters;
};
