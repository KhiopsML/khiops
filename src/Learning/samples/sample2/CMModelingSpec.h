// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CMMajorityClassifier.h"
#include "KWModelingSpec.h"

////////////////////////////////////////////////////////////
/// Classe CMModelingSpec :
class CMModelingSpec : public KWModelingSpec
{
public:
	/// Constructeur
	CMModelingSpec();
	~CMModelingSpec();

	/// Duplication
	CMModelingSpec* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	/// Classifier majoritaire
	boolean GetTrainMajorityClassifier() const;
	void SetTrainMajorityClassifier(boolean bValue);

	////////////////////////////////////////////////////////
	// Divers

	/// Ecriture
	void Write(ostream& ost) const override;

	/// Libelles utilisateur
	const ALString GetClassLabel() const;
	const ALString GetObjectLabel() const;

	/// Parametrage d'un predicteur majoritaire
	CMMajorityClassifier* GetMajorityClassifier();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	/// Apprentissage ou non du classifieur majoritaire
	boolean bTrainMajorityClassifier;

	/// Parametrage du classifieur
	CMMajorityClassifier majorityClassifier;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean CMModelingSpec::GetTrainMajorityClassifier() const
{
	return bTrainMajorityClassifier;
}

inline void CMModelingSpec::SetTrainMajorityClassifier(boolean bValue)
{
	bTrainMajorityClassifier = bValue;
}
