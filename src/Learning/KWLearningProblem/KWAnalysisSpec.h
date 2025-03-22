// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWVersion.h"
#include "KWModelingSpec.h"
#include "KWAttributeConstructionSpec.h"
#include "KWRecoderSpec.h"
#include "KWPreprocessingSpec.h"
#include "MHHistogramSpec.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWAnalysisSpec
//    Parameters
class KWAnalysisSpec : public Object
{
public:
	// Constructeur
	KWAnalysisSpec();
	~KWAnalysisSpec();

	// Copie et duplication
	void CopyFrom(const KWAnalysisSpec* aSource);
	KWAnalysisSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Target variable
	const ALString& GetTargetAttributeName() const;
	void SetTargetAttributeName(const ALString& sValue);

	// Main target value
	const ALString& GetMainTargetModality() const;
	void SetMainTargetModality(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Parametrage des predicteurs a apprendre
	KWModelingSpec* GetModelingSpec();

	// Parametrage des recodeurs a apprendre
	KWRecoderSpec* GetRecoderSpec();

	// Parametrage des pretraitements
	KWPreprocessingSpec* GetPreprocessingSpec();

	// Parametrage des histogrammes, qui sont definis dans une librairies a part
	MHHistogramSpec* GetHistogramSpec();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sTargetAttributeName;
	ALString sMainTargetModality;

	// ## Custom implementation

	KWModelingSpec* modelingSpec;
	KWRecoderSpec* recoderSpec;
	KWPreprocessingSpec* preprocessingSpec;
	MHHistogramSpec* histogramSpec;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAnalysisSpec::GetTargetAttributeName() const
{
	return sTargetAttributeName;
}

inline void KWAnalysisSpec::SetTargetAttributeName(const ALString& sValue)
{
	sTargetAttributeName = sValue;
}

inline const ALString& KWAnalysisSpec::GetMainTargetModality() const
{
	return sMainTargetModality;
}

inline void KWAnalysisSpec::SetMainTargetModality(const ALString& sValue)
{
	sMainTargetModality = sValue;
}

// ## Custom inlines

// ##
