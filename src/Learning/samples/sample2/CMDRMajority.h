// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"
#include "KWDRNBPredictor.h"

//////////////////////////////////////////////////////////////////////
// Classe CMDRMajority
// Cette classe implemente le classifieur majoritaire
// a partir de la regle de derivation classifieur NB
class CMDRMajorityClassifier : public KWDRNBClassifier
{
public:
	// Constructeur
	CMDRMajorityClassifier();
	~CMDRMajorityClassifier();

	// Reimplementation de la methode Create
	KWDerivationRule* Create() const;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass);

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul du vecteur de probabilites conditionnelles
	void ComputeTargetProbs() const;

	// Vecteurs des frequences cibles
	IntVector ivFrequencies;

	// Effectif total
	int nTotalFrequency;
};
