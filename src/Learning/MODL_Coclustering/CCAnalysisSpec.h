// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CCCoclusteringSpec.h"
#include "CCVarPartCoclusteringSpec.h"

////////////////////////////////////////////////////////////
// Classe CCAnalysisSpec
//    Parameters
class CCAnalysisSpec : public Object
{
public:
	// Constructeur
	CCAnalysisSpec();
	~CCAnalysisSpec();

	// Parametrage du coclustering
	CCCoclusteringSpec* GetCoclusteringSpec();

	// CH IV Begin
	// Parametrage du coclustering VarPart instances x variables
	CCVarPartCoclusteringSpec* GetVarPartCoclusteringSpec();

	// Parametrage du type de coclustering
	// A true pour un coclustering de type VarPart, instances * varialbes
	// A false sinon pour un coclustering variable * variable (valeur par defaut)
	void SetVarPartCoclustering(boolean bValue);
	boolean GetVarPartCoclustering() const;

	// Libelles correspondant aux type de coclustering
	// pour les cas VartPart (instances x variables) ou standard (variables x variables)
	static const ALString GetCoclusteringLabelFromType(boolean bIsVarPartCoclustering);

	// Type correspond a un libelle de coclustering
	// Renvoie true dans la cas VarPart, false sinon
	static boolean GetCoclusteringTypeFromLabel(const ALString& sLabel);
	// CH IV End

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	CCCoclusteringSpec coclusteringSpec;
	// CH IV Begin
	CCVarPartCoclusteringSpec varPartCoclusteringSpec;
	boolean bVarPartCoclustering;
	// CH IV End
};
