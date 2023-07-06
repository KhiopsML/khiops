// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CCCoclusteringSpec.h"

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

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	CCCoclusteringSpec coclusteringSpec;
};
