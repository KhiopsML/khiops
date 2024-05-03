// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de creation de Table

// Prototype de regle pour la mise au point de la gestion des regle creant des Table
class KWDRProtoBuildTableView;

#include "KWDerivationRule.h"

// Enregistrement de ces regles
void KWDRRegisterTableCreationRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExist
// Indicateur d'existence presence d'un attribut Object
class KWDRProtoBuildTableView : public KWDerivationRule
{
public:
	// Constructeur
	KWDRProtoBuildTableView();
	~KWDRProtoBuildTableView();

	// Creation
	KWDerivationRule* Create() const override;

	// On indique que la regle cree de nouveau objets
	boolean GetReference() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Copie des champs commun d'un object
	void CopyObjectCommonNativeFields(const KWObject* kwoSourceObject, KWObject* kwoTargetObject) const;

	// Object Array en retour de la regle
	mutable ObjectArray oaResult;
};
