// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de creation de Table

class KWDRBuildTableView;
class KWDRBuildTableAdvancedView;
class KWDRBuildEntityView;
class KWDRBuildEntityAdvancedView;
class KWDRBuildEntity;

#include "KWDerivationRule.h"
#include "KWRelationCreationRule.h"

// Enregistrement de ces regles
void KWDRRegisterBuildRelationRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableView
// Creation d'une vue sur une table
// Chaque attribut natif de la table en sortie doit correspondre a un attribut
// natif ou calcule de la table en entree
class KWDRBuildTableView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildTableView();
	~KWDRBuildTableView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableAdvancedView
// Creation d'une vue sur une table, avec possibilite d'alimenter des attribut
// de la table en sortie directement via des des operandes en sortie et
// des valeurs fournies par les operandes en entree corespondants
class KWDRBuildTableAdvancedView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildTableAdvancedView();
	~KWDRBuildTableAdvancedView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityView
// Creation d'une vue sur une Entity
// Chaque attribut natif de l'entite en sortie doit correspondre a un attribut
// natif ou calcule de l'entite en entree
class KWDRBuildEntityView : public KWDRBuildTableView
{
public:
	// Constructeur
	KWDRBuildEntityView();
	~KWDRBuildEntityView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityAdvancedView
// Creation d'une vue sur une entite, avec possibilite d'alimenter des attributs
// de l'entite en sortie directement via des des operandes en sortie et
// des valeurs fournies par les operandes en entree corespondants
class KWDRBuildEntityAdvancedView : public KWDRBuildTableAdvancedView
{
public:
	// Constructeur
	KWDRBuildEntityAdvancedView();
	~KWDRBuildEntityAdvancedView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntity
// Creation d'une entite en alimentant chaque attribut natif de l'entite en sortie
// via un operande en sortie designant l'attribut a alimenter et une valeur issue
// de l'operande correspondante en entree
class KWDRBuildEntity : public KWDRRelationCreationRule
{
public:
	// Constructeur
	KWDRBuildEntity();
	~KWDRBuildEntity();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimenattionde type vue
	boolean IsViewModeActivated() const override;
};
