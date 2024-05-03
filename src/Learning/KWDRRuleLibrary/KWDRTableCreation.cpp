// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTableCreation.h"

void KWDRRegisterTableCreationRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRProtoBuildTableView);
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableView

KWDRProtoBuildTableView::KWDRProtoBuildTableView()
{
	SetName("ProtoBuildTableView");
	SetLabel("Table view");
	SetType(KWType::ObjectArray);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
}

KWDRProtoBuildTableView::~KWDRProtoBuildTableView() {}

KWDerivationRule* KWDRProtoBuildTableView::Create() const
{
	return new KWDRProtoBuildTableView;
}

boolean KWDRProtoBuildTableView::GetReference() const
{
	require(KWType::IsGeneralRelation(GetType()));
	return false;
}

ObjectArray* KWDRProtoBuildTableView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoContainedObject;
	KWObject* kwoCopiedContainedObject;
	KWClass* kwcTargetClass;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Recherche de la classe cible
		kwcTargetClass = kwoObject->GetClass()->GetDomain()->LookupClass(GetObjectClassName());

		// Copie du tableau en entree
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
			kwoCopiedContainedObject = new KWObject(kwcTargetClass, 1);
			kwoCopiedContainedObject->SetViewTypeUse(true);
			CopyObjectCommonNativeFields(kwoContainedObject, kwoCopiedContainedObject);
			oaResult.SetAt(nObject, kwoCopiedContainedObject);
		}
	}
	return &oaResult;
}

void KWDRProtoBuildTableView::CopyObjectCommonNativeFields(const KWObject* kwoSourceObject,
							   KWObject* kwoTargetObject) const
{
	const KWClass* kwcSourceClass;
	const KWClass* kwcTargetClass;
	const KWAttribute* sourceAttribute;
	const KWAttribute* targetAttribute;
	ObjectArray* oaSourceObjectArray;
	ObjectArray* oaTargetObjectArray;
	int nAttribute;

	require(kwoSourceObject != NULL);
	require(kwoTargetObject != NULL);

	// Recopie des valeurs des attributs commun
	kwcSourceClass = kwoSourceObject->GetClass();
	kwcTargetClass = kwoTargetObject->GetClass();
	for (nAttribute = 0; nAttribute < kwcSourceClass->GetLoadedDenseAttributeNumber(); nAttribute++)
	{
		sourceAttribute = kwcSourceClass->GetLoadedDenseAttributeAt(nAttribute);

		// Recherche d'un attribut natif cible de meme nom
		targetAttribute = kwcTargetClass->LookupAttribute(sourceAttribute->GetName());

		// Recopie de la valeur
		if (targetAttribute != NULL and targetAttribute->GetAnyDerivationRule() == NULL)
		{
			assert(targetAttribute->GetType() == sourceAttribute->GetType());

			// Recopie de la valeur selon le type
			if (sourceAttribute->GetType() == KWType::Continuous)
				kwoTargetObject->SetContinuousValueAt(
				    targetAttribute->GetLoadIndex(),
				    kwoSourceObject->GetContinuousValueAt(sourceAttribute->GetLoadIndex()));
			else if (sourceAttribute->GetType() == KWType::Symbol)
				kwoTargetObject->SetSymbolValueAt(
				    targetAttribute->GetLoadIndex(),
				    kwoSourceObject->GetSymbolValueAt(sourceAttribute->GetLoadIndex()));
			else if (sourceAttribute->GetType() == KWType::Object)
				kwoTargetObject->SetObjectValueAt(
				    targetAttribute->GetLoadIndex(),
				    kwoSourceObject->GetObjectValueAt(sourceAttribute->GetLoadIndex()));
			else if (sourceAttribute->GetType() == KWType::ObjectArray)
			{
				oaSourceObjectArray =
				    kwoSourceObject->GetObjectArrayValueAt(sourceAttribute->GetLoadIndex());
				oaTargetObjectArray = NULL;
				if (oaSourceObjectArray != NULL)
				{
					oaTargetObjectArray = new ObjectArray;
					oaTargetObjectArray->CopyFrom(oaSourceObjectArray);
				}
				kwoTargetObject->SetObjectArrayValueAt(targetAttribute->GetLoadIndex(),
								       oaTargetObjectArray);
			}
		}
	}
}
