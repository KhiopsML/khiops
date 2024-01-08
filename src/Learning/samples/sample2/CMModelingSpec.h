// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Wed Jun 27 17:02:15 2007
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

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

	/// CM classifier
	boolean GetCMClassifier() const;
	void SetCMClassifier(boolean bValue);

	////////////////////////////////////////////////////////
	// Divers

	/// Ecriture
	void Write(ostream& ost) const;

	/// Libelles utilisateur
	const ALString GetClassLabel() const;
	const ALString GetObjectLabel() const;

	// ## Custom declarations

	/// Parametrage d'un predicteur CM
	CMMajorityClassifier* GetClassifieurMajoritaire();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	/// Apprentissage ou non du classifieur majoritaire
	boolean bCMClassifier;

	/// Parametrage du classifieur
	CMMajorityClassifier classifieurMajoritaire;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean CMModelingSpec::GetCMClassifier() const
{
	return bCMClassifier;
}

inline void CMModelingSpec::SetCMClassifier(boolean bValue)
{
	bCMClassifier = bValue;
}
