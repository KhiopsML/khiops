// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRBuildRelation.h"

void KWDRRegisterBuildRelationRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildTableView);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildTableAdvancedView);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildEntityView);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildEntityAdvancedView);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildEntity);
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableView

KWDRBuildTableView::KWDRBuildTableView()
{
	SetName("BuildTableView");
	SetLabel("Build table view");
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
}

KWDRBuildTableView::~KWDRBuildTableView() {}

KWDerivationRule* KWDRBuildTableView::Create() const
{
	return new KWDRBuildTableView;
}

ObjectArray* KWDRBuildTableView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoSourceContainedObject;
	KWObject* kwoTargetContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoSourceContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
			kwoTargetContainedObject = NewTargetObject((longint)nObject + 1);

			// Alimentation de type vue
			FillViewModeTargetAttributes(kwoSourceContainedObject, kwoTargetContainedObject);
			oaResult.SetAt(nObject, kwoTargetContainedObject);
		}
	}
	return &oaResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableAdvancedView

KWDRBuildTableAdvancedView::KWDRBuildTableAdvancedView()
{
	SetName("BuildTableAdvancedView");
	SetLabel("Build table advanced view");

	// Variables en entree: une table de base, et un nombre variable d'operande pour alimenter les variables en sortie
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Unknown);
	SetVariableOperandNumber(true);

	// Variables en sortie: nombre variable d'operandes pour alimenter la table en sortie
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Unknown);
	SetVariableOutputOperandNumber(true);
}

KWDRBuildTableAdvancedView::~KWDRBuildTableAdvancedView() {}

KWDerivationRule* KWDRBuildTableAdvancedView::Create() const
{
	return new KWDRBuildTableAdvancedView;
}

ObjectArray* KWDRBuildTableAdvancedView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoSourceContainedObject;
	KWObject* kwoTargetContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoSourceContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Alimentation de type vue
			kwoTargetContainedObject = NewTargetObject((longint)nObject + 1);
			FillViewModeTargetAttributes(kwoSourceContainedObject, kwoTargetContainedObject);
			oaResult.SetAt(nObject, kwoTargetContainedObject);

			// Alimentation de type calcul pour les operandes en entree correspondant
			assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
			FillComputeModeTargetAttributesForVariableOperandNumber(kwoSourceContainedObject,
										kwoTargetContainedObject);
		}
	}
	return &oaResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityView

KWDRBuildEntityView::KWDRBuildEntityView()
{
	SetName("BuildEntityView");
	SetLabel("Build entity view");
	SetType(KWType::Object);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Object);
}

KWDRBuildEntityView::~KWDRBuildEntityView() {}

KWDerivationRule* KWDRBuildEntityView::Create() const
{
	return new KWDRBuildEntityView;
}

KWObject* KWDRBuildEntityView::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWObject* kwoSourceObject;
	KWObject* kwoTargetObject;

	require(IsCompiled());

	// Copie de l'object en entree
	kwoSourceObject = GetFirstOperand()->GetObjectValue(kwoObject);
	kwoTargetObject = NULL;
	if (kwoSourceObject != NULL)
	{
		kwoTargetObject = NewTargetObject((longint)1);

		// Alimentation de type vue
		FillViewModeTargetAttributes(kwoSourceObject, kwoTargetObject);
	}
	return kwoTargetObject;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityAdvancedView

KWDRBuildEntityAdvancedView::KWDRBuildEntityAdvancedView()
{
	SetName("BuildEntityAdvancedView");
	SetLabel("Build entity advanced view");
	SetType(KWType::Object);
	GetFirstOperand()->SetType(KWType::Object);
}

KWDRBuildEntityAdvancedView::~KWDRBuildEntityAdvancedView() {}

KWDerivationRule* KWDRBuildEntityAdvancedView::Create() const
{
	return new KWDRBuildEntityAdvancedView;
}

KWObject* KWDRBuildEntityAdvancedView::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWObject* kwoSourceObject;
	KWObject* kwoTargetObject;

	require(IsCompiled());

	// Copie de l'object en entree
	kwoSourceObject = GetFirstOperand()->GetObjectValue(kwoObject);
	kwoTargetObject = NULL;
	if (kwoSourceObject != NULL)
	{
		kwoTargetObject = NewTargetObject((longint)1);

		// Alimentation de type vue
		FillViewModeTargetAttributes(kwoSourceObject, kwoTargetObject);

		// Alimentation de type calcul pour les operandes en entree correspondant
		assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
		FillComputeModeTargetAttributesForVariableOperandNumber(kwoSourceObject, kwoTargetObject);
	}
	return kwoTargetObject;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntity

KWDRBuildEntity::KWDRBuildEntity()
{
	SetName("BuildEntity");
	SetLabel("Build entity");
	SetType(KWType::Object);

	// Variables en entree: nombre variable d'operande pour alimenter les variables en sortie
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Unknown);
	SetVariableOperandNumber(true);

	// Variables en sortie: nombre variable d'operandes pour alimenter la table en sortie
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Unknown);
	SetVariableOutputOperandNumber(true);
}

KWDRBuildEntity::~KWDRBuildEntity() {}

KWDerivationRule* KWDRBuildEntity::Create() const
{
	return new KWDRBuildEntity;
}

KWObject* KWDRBuildEntity::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWObject* kwoTargetObject;

	require(IsCompiled());
	assert(GetOperandNumber() == GetOutputOperandNumber());
	assert(GetOperandNumber() == ivComputeModeTargetAttributeTypes.GetSize());

	// Creation de l'objet en sortie
	kwoTargetObject = NewTargetObject((longint)1);

	// Alimentation de type calcul pour les operandes en entree correspondant
	FillComputeModeTargetAttributesForVariableOperandNumber(kwoObject, kwoTargetObject);
	return kwoTargetObject;
}

boolean KWDRBuildEntity::IsViewModeActivated() const
{
	return false;
}
