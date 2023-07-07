// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CCCoclusteringSpec.h"
// CH IV Refactoring: renommer en CCVarPartCoclusteringSpec
#include "CCInstancesVariablesCoclusteringSpec.h"

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
	// Parametrage du coclustering instances variables (generique)
	// CH IV Refactoring: renommer en CCVarPartCoclusteringSpec
	CCInstancesVariablesCoclusteringSpec* GetInstancesVariablesCoclusteringSpec();

	// Parametrage du type de coclustering
	// generique : instances * variable (i.e. variable * variable avec nouveau type de variable de type parties de
	// variable) sinon : variable * variable
	// CH IV Refactoring: renommer en Get|SetVarPartCoclustering
	void SetGenericCoclustering(boolean bValue);
	boolean GetGenericCoclustering() const;

	void SetCoclusteringType(const ALString& sValue);
	const ALString GetCoclusteringType() const;
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
	CCInstancesVariablesCoclusteringSpec instancesVariablesCoclusteringSpec;
	boolean bGenericCoclustering;
	// CH IV End
};
