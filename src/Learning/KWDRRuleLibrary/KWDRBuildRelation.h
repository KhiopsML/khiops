// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de creation de Table

// Prototype de regle pour la mise au point de la gestion des regle creant des Table
class KWDRProtoBuildTableView;
class KWDRProtoBuildTableAdvancedView;

#include "KWDerivationRule.h"
#include "KWRelationCreationRule.h"

// Enregistrement de ces regles
void KWDRRegisterBuildRelationRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableView
// Creation d'une vue sur une table
// Prototype
class KWDRProtoBuildTableView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRProtoBuildTableView();
	~KWDRProtoBuildTableView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableAdvancedView
// Creation d'une vue sur une table
// Prototype
class KWDRProtoBuildTableAdvancedView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRProtoBuildTableAdvancedView();
	~KWDRProtoBuildTableAdvancedView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
};
