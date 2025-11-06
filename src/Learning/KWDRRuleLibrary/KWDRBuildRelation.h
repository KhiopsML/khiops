// Copyright (c) 2023-2025 Orange. All rights reserved.
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
class KWDRBuildDiffTable;
class KWDRBuildDummyTable;

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
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableAdvancedView
// Creation d'une vue sur une table, avec possibilite d'alimenter des attributs
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
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;
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
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;
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
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;
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
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDiffTable
// Creation d'une table cible avec calcul des differences de valeur entre deux
// records successifs pour un sous-ensemble des attributs sources choisis en entree:
// - chaque attribut natif de la table en sortie ayant meme nom et type qu'un attribut
//   natif ou calcule de la table en entree est recopie tel quel, comme dans BuildTableView
// - la table source en premier operande doit etre triee selon le besoin utilisateur
// - les attributs sources dont on doit calculer la difference sont a specifie en entree
//   a partir du deuxieme operande
// - les variable en sortie memorisant les difference sont specifie dans les operandes en
//   sortie de la regle, en meme nombre et position que les attributs correspondant en entree
// - les differences de variables sont calculee selon les types suivant
//   - Numerical: difference numerique
//   - Time: difference en secondes
//   - Date: difference en jours
//   - Timestamp, TimestampTZ: difference en secondes
//   - Categorical: 1 si valeur differente, 0 sinon
// - la table en sortie contient autant de record que la table en entree
//   - pour le premier record en sortie, les valeurs des differences sont Missing
class KWDRBuildDiffTable : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildDiffTable();
	~KWDRBuildDiffTable();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;

	// Verification qu'une regle est une specialisation d'une regle plus generale
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des attributs cibles en sortie comme des difference des valeurs de attributs specifie dans
	// les operande en entree pour deux objet successifs
	// Si le premier objet est NULL, ces differeces sont missing
	void FillTargetDifferenceAttributes(const KWObject* kwoSourcePreviousObject,
					    const KWObject* kwoSourceCurrentObject, KWObject* kwoTargetObject) const;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDummyTable
// Creation d'une table factice avec autant d'instances que specifie en parametre
// Regle interne, uniquement pour des raison de test de volumetrie
class KWDRBuildDummyTable : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildDummyTable();
	~KWDRBuildDummyTable();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;
};
