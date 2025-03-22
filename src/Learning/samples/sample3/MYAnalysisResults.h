// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"
#include "KWAnalysisResults.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe MYAnalysisResults
//    Analysis results
class MYAnalysisResults : public KWAnalysisResults
{
public:
	// Constructeur
	MYAnalysisResults();
	~MYAnalysisResults();

	// Copie et duplication
	void CopyFrom(const MYAnalysisResults* aSource);
	MYAnalysisResults* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Compute basic secondary stats
	boolean GetComputeBasicSecondaryStats() const;
	void SetComputeBasicSecondaryStats(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bComputeBasicSecondaryStats;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean MYAnalysisResults::GetComputeBasicSecondaryStats() const
{
	return bComputeBasicSecondaryStats;
}

inline void MYAnalysisResults::SetComputeBasicSecondaryStats(boolean bValue)
{
	bComputeBasicSecondaryStats = bValue;
}

// ## Custom inlines

// ##
