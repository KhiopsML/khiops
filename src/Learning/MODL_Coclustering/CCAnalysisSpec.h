// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "CCCoclusteringSpec.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCAnalysisSpec
//    Parameters
class CCAnalysisSpec : public Object
{
public:
	// Constructeur
	CCAnalysisSpec();
	~CCAnalysisSpec();

	// Copie et duplication
	void CopyFrom(const CCAnalysisSpec* aSource);
	CCAnalysisSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Parametrage du coclustering
	CCCoclusteringSpec* GetCoclusteringSpec();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe

	// ## Custom implementation

	CCCoclusteringSpec coclusteringSpec;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

// ## Custom inlines

// ##
