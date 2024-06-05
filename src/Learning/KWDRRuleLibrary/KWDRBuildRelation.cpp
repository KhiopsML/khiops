// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRBuildRelation.h"

void KWDRRegisterBuildRelationRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRProtoBuildTableView);
	KWDerivationRule::RegisterDerivationRule(new KWDRProtoBuildTableAdvancedView);
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableView

KWDRProtoBuildTableView::KWDRProtoBuildTableView()
{
	SetName("ProtoBuildTableView");
	SetLabel("Table view");
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
}

KWDRProtoBuildTableView::~KWDRProtoBuildTableView() {}

KWDerivationRule* KWDRProtoBuildTableView::Create() const
{
	return new KWDRProtoBuildTableView;
}

ObjectArray* KWDRProtoBuildTableView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoContainedObject;
	KWObject* kwoCopiedContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Copie du tableau en entree
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
			kwoCopiedContainedObject = NewTargetObject((longint)nObject + 1);
			CopyObjectCommonNativeAttributes(kwoContainedObject, kwoCopiedContainedObject);
			oaResult.SetAt(nObject, kwoCopiedContainedObject);
		}
	}
	return &oaResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableAdvancedView

KWDRProtoBuildTableAdvancedView::KWDRProtoBuildTableAdvancedView()
{
	SetName("ProtoBuildTableAdvancedView");
	SetLabel("Table advanced view");
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Symbol);
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Symbol);
}

KWDRProtoBuildTableAdvancedView::~KWDRProtoBuildTableAdvancedView() {}

KWDerivationRule* KWDRProtoBuildTableAdvancedView::Create() const
{
	return new KWDRProtoBuildTableAdvancedView;
}

ObjectArray* KWDRProtoBuildTableAdvancedView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoContainedObject;
	KWObject* kwoCopiedContainedObject;
	int nObject;
	int i;
	KWDerivationRuleOperand* operand;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Copie du tableau en entree
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Copie de l'objet source selon une alimentation de type vue
			kwoCopiedContainedObject = NewTargetObject((longint)nObject + 1);
			CopyObjectCommonNativeAttributes(kwoContainedObject, kwoCopiedContainedObject);
			oaResult.SetAt(nObject, kwoCopiedContainedObject);

			// Modification des champ supplementaire selon une alimentation de type calcul
			assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
			for (i = 0; i < ivComputeModeTargetAttributeTypes.GetSize(); i++)
			{
				operand = GetOperandAt(i + 1);
				kwoCopiedContainedObject->SetSymbolValueAt(
				    livComputeModeTargetAttributeLoadIndexes.GetAt(i),
				    operand->GetSymbolValue(kwoContainedObject));
			}
		}
	}
	return &oaResult;
}
