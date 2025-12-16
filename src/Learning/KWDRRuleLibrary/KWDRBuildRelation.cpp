// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildDiffTable);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildList);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildDummyTable);
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

ObjectArray* KWDRBuildTableView::ComputeObjectArrayResult(const KWObject* kwoObject,
							  const KWLoadIndex liAttributeLoadIndex) const
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
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);

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

ObjectArray* KWDRBuildTableAdvancedView::ComputeObjectArrayResult(const KWObject* kwoObject,
								  const KWLoadIndex liAttributeLoadIndex) const
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
		// Evaluation des operandes secondaires de scope principal
		EvaluateMainScopeSecondaryOperands(kwoObject);

		// Creation et alimentation des objets cibles
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoSourceContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Alimentation de type vue
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			FillViewModeTargetAttributes(kwoSourceContainedObject, kwoTargetContainedObject);
			oaResult.SetAt(nObject, kwoTargetContainedObject);

			// Alimentation de type calcul pour les operandes en entree correspondant
			assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
			FillComputeModeTargetAttributesForVariableOperandNumber(kwoSourceContainedObject,
										kwoTargetContainedObject);
		}

		// Nettoyage des operandes secondaires de scope principal
		CleanMainScopeSecondaryOperands();
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

KWObject* KWDRBuildEntityView::ComputeObjectResult(const KWObject* kwoObject,
						   const KWLoadIndex liAttributeLoadIndex) const
{
	KWObject* kwoSourceObject;
	KWObject* kwoTargetObject;

	require(IsCompiled());

	// Copie de l'object en entree
	kwoSourceObject = GetFirstOperand()->GetObjectValue(kwoObject);
	kwoTargetObject = NULL;
	if (kwoSourceObject != NULL)
	{
		kwoTargetObject = NewTargetObject(kwoObject, liAttributeLoadIndex);

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

KWObject* KWDRBuildEntityAdvancedView::ComputeObjectResult(const KWObject* kwoObject,
							   const KWLoadIndex liAttributeLoadIndex) const
{
	KWObject* kwoSourceObject;
	KWObject* kwoTargetObject;

	require(IsCompiled());

	// Copie de l'object en entree
	kwoSourceObject = GetFirstOperand()->GetObjectValue(kwoObject);
	kwoTargetObject = NULL;
	if (kwoSourceObject != NULL)
	{
		kwoTargetObject = NewTargetObject(kwoObject, liAttributeLoadIndex);

		// Evaluation des operandes secondaires de scope principal
		EvaluateMainScopeSecondaryOperands(kwoObject);

		// Alimentation de type vue
		FillViewModeTargetAttributes(kwoSourceObject, kwoTargetObject);

		// Alimentation de type calcul pour les operandes en entree correspondant
		assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
		FillComputeModeTargetAttributesForVariableOperandNumber(kwoSourceObject, kwoTargetObject);

		// Nettoyage des operandes secondaires de scope principal
		CleanMainScopeSecondaryOperands();
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

KWObject* KWDRBuildEntity::ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const
{
	KWObject* kwoTargetObject;

	require(IsCompiled());
	assert(GetOperandNumber() == GetOutputOperandNumber());
	assert(GetOperandNumber() == ivComputeModeTargetAttributeTypes.GetSize());

	// Creation de l'objet en sortie
	kwoTargetObject = NewTargetObject(kwoObject, liAttributeLoadIndex);

	// Alimentation de type calcul pour les operandes en entree correspondant
	FillComputeModeTargetAttributesForVariableOperandNumber(kwoObject, kwoTargetObject);
	return kwoTargetObject;
}

boolean KWDRBuildEntity::IsViewModeActivated() const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDiffTable

KWDRBuildDiffTable::KWDRBuildDiffTable()
{
	SetName("BuildDiffTable");
	SetLabel("Build difference table");

	// Variables en entree: une table de base, et un nombre variable d'operande pour specifier
	// les attributs dont il faut calcvuler la difference
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Unknown);
	SetVariableOperandNumber(true);

	// Variables en sortie: nombre variable d'operandes pour alimenter la table en sortie
	// avec les differences de valeur des attributs specifies en entree
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Continuous);
	SetVariableOutputOperandNumber(true);
}

KWDRBuildDiffTable::~KWDRBuildDiffTable() {}

KWDerivationRule* KWDRBuildDiffTable::Create() const
{
	return new KWDRBuildDiffTable;
}

ObjectArray* KWDRBuildDiffTable::ComputeObjectArrayResult(const KWObject* kwoObject,
							  const KWLoadIndex liAttributeLoadIndex) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoSourceContainedPreviousObject;
	KWObject* kwoSourceContainedCurrentObject;
	KWObject* kwoTargetContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Evaluation des operandes secondaires de scope principal
		EvaluateMainScopeSecondaryOperands(kwoObject);

		// Creation et alimentation des objets cibles
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		kwoSourceContainedPreviousObject = NULL;
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoSourceContainedCurrentObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Alimentation de type vue
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			FillViewModeTargetAttributes(kwoSourceContainedCurrentObject, kwoTargetContainedObject);
			oaResult.SetAt(nObject, kwoTargetContainedObject);

			// Alimentation de type calcul pour les operandes en entree correspondant
			assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
			FillTargetDifferenceAttributes(kwoSourceContainedPreviousObject,
						       kwoSourceContainedCurrentObject, kwoTargetContainedObject);

			// Memorisation du prochain objet precedent
			kwoSourceContainedPreviousObject = kwoSourceContainedCurrentObject;
		}

		// Nettoyage des operandes secondaires de scope principal
		CleanMainScopeSecondaryOperands();
	}
	return &oaResult;
}

boolean KWDRBuildDiffTable::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk;
	KWDerivationRuleOperand* operand;
	KWDerivationRuleOperand* familyVariableOperand;
	int i;
	ALString sTmp;

	require(not ruleFamily->GetReference());

	// Appel de la methode ancetre
	bOk = KWDRRelationCreationRule::CheckOperandsFamily(ruleFamily);

	// Specialisation dans le cas d'un nombre variable d'operandes en entree
	if (bOk and ruleFamily->GetVariableOperandNumber())
	{
		familyVariableOperand = ruleFamily->GetOperandAt(ruleFamily->GetOperandNumber() - 1);
		assert(familyVariableOperand->GetType() == KWType::Unknown);

		// Verification des operandes en nombres variables
		for (i = ruleFamily->GetOperandNumber() - 1; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Le type doit permet un calcul de difference
			if (not KWType::IsSimple(operand->GetType()) and not KWType::IsComplex(operand->GetType()))
			{
				AddError(sTmp + "Operand " + IntToString(i + 1) + " with wrong " +
					 KWType::ToString(operand->GetType()) +
					 " type (must be a raw data type for difference calculation)");
				bOk = false;
			}
		}
	}
	return bOk;
}

void KWDRBuildDiffTable::FillTargetDifferenceAttributes(const KWObject* kwoSourcePreviousObject,
							const KWObject* kwoSourceCurrentObject,
							KWObject* kwoTargetObject) const
{
	int nAttribute;
	int nType;
	int nStartInputOperandIndex;
	int nInputOperandIndex;
	KWDerivationRuleOperand* operand;
	KWLoadIndex liTarget;

	require(IsCompiled());
	require(kwoSourceCurrentObject != NULL);
	require(GetOperandNumber() >= GetOutputOperandNumber());
	require(GetOutputOperandNumber() == ivComputeModeTargetAttributeTypes.GetSize());
	require(GetVariableOperandNumber());
	require(GetVariableOutputOperandNumber());

	// Recherche de l'index du premier operande en entree correspondant
	// aux valeurs servant a alimenter les attributs en sortie
	// En effet, les operande en sortie sont alimentes par les derniers operandes en entree
	nStartInputOperandIndex = GetOperandNumber() - GetOutputOperandNumber();

	// Alimentation des attributs de l'objet cible avec les valeurs provenant des operandes en sortie
	for (nAttribute = 0; nAttribute < ivComputeModeTargetAttributeTypes.GetSize(); nAttribute++)
	{
		// Recherche de l'operande source correspondant a l'operande cible
		nInputOperandIndex = nStartInputOperandIndex + nAttribute;
		operand = GetOperandAt(nInputOperandIndex);

		// On ignore l'operande en entree s'il a la valeur speciale None
		if (operand->GetNoneValue())
			continue;

		// Index de chargement cible
		liTarget = livComputeModeTargetAttributeLoadIndexes.GetAt(nAttribute);
		assert(liTarget.IsValid());
		assert(kwoTargetObject->GetClass()
			   ->LookupAttribute(GetOutputOperandAt(nAttribute)->GetAttributeName())
			   ->GetLoaded());

		// La valeur est missing dans le cas ou il n'y pas d'objet precedent
		if (kwoSourcePreviousObject == NULL)
			kwoTargetObject->SetContinuousValueAt(liTarget, KWContinuous::GetMissingValue());
		// Calcul de la difference de valeur selon le type
		else
		{
			nType = operand->GetType();
			assert(ivComputeModeTargetAttributeTypes.GetAt(nAttribute) == KWType::Continuous);
			switch (nType)
			{
			case KWType::Symbol:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetSymbolValue(kwoSourceCurrentObject) !=
						  operand->GetSymbolValue(kwoSourcePreviousObject));
				break;
			case KWType::Continuous:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetContinuousValue(kwoSourceCurrentObject) -
						  operand->GetContinuousValue(kwoSourcePreviousObject));
				break;
			case KWType::Date:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetDateValue(kwoSourceCurrentObject)
						  .Diff(operand->GetDateValue(kwoSourcePreviousObject)));
				break;
			case KWType::Time:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetTimeValue(kwoSourceCurrentObject)
						  .Diff(operand->GetTimeValue(kwoSourcePreviousObject)));
				break;
			case KWType::Timestamp:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetTimestampValue(kwoSourceCurrentObject)
						  .Diff(operand->GetTimestampValue(kwoSourcePreviousObject)));
				break;
			case KWType::TimestampTZ:
				kwoTargetObject->SetContinuousValueAt(
				    liTarget, operand->GetTimestampTZValue(kwoSourceCurrentObject)
						  .Diff(operand->GetTimestampTZValue(kwoSourcePreviousObject)));
				break;
			default:
				assert(false);
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildList

KWDRBuildList::KWDRBuildList()
{
	SetName("BuildList");
	SetLabel("Build list from table");
	SetType(KWType::ObjectArray);

	// Variables en entree: une table
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);

	// Variables en sortie: nombre variable d'operandes pour alimenter la table en sortie
	// avec les differences de valeur des attributs specifies en entree
	SetOutputOperandNumber(3);
	GetOutputOperandAt(0)->SetType(KWType::Object);
	GetOutputOperandAt(1)->SetType(KWType::Object);
	GetOutputOperandAt(2)->SetType(KWType::Object);
}

KWDRBuildList::~KWDRBuildList() {}

KWDerivationRule* KWDRBuildList::Create() const
{
	return new KWDRBuildList;
}

ObjectArray* KWDRBuildList::ComputeObjectArrayResult(const KWObject* kwoObject,
						     const KWLoadIndex liAttributeLoadIndex) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoSourceContainedCurrentObject;
	KWObject* kwoTargetContainedObject;
	KWObject* kwoPrevTargetContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Creation et alimentation des objets cibles
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoSourceContainedCurrentObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Creation d'un noeud de la liste
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			oaResult.SetAt(nObject, kwoTargetContainedObject);

			// Memorisation de l'attribut Data du noeud courant
			if (livComputeModeTargetAttributeLoadIndexes.GetAt(0).IsValid())
				kwoTargetContainedObject->SetObjectValueAt(
				    livComputeModeTargetAttributeLoadIndexes.GetAt(0), kwoSourceContainedCurrentObject);

			// Chainage avec le noeud precedent
			// Memorisation de l'attribut Prev du noeud courant
			assert(not livComputeModeTargetAttributeLoadIndexes.GetAt(1).IsValid() or
			       kwoTargetContainedObject->GetObjectValueAt(
				   livComputeModeTargetAttributeLoadIndexes.GetAt(1)) == NULL);
			assert(not livComputeModeTargetAttributeLoadIndexes.GetAt(2).IsValid() or
			       kwoTargetContainedObject->GetObjectValueAt(
				   livComputeModeTargetAttributeLoadIndexes.GetAt(2)) == NULL);
			if (nObject > 0)
			{
				// Acces au noeud prededent
				kwoPrevTargetContainedObject = cast(KWObject*, oaResult.GetAt(nObject - 1));

				// Chainage du precedent vers le courant
				if (livComputeModeTargetAttributeLoadIndexes.GetAt(2).IsValid())
					kwoPrevTargetContainedObject->SetObjectValueAt(
					    livComputeModeTargetAttributeLoadIndexes.GetAt(2),
					    kwoTargetContainedObject);

				// Chainage du courant vers le precedent
				if (livComputeModeTargetAttributeLoadIndexes.GetAt(1).IsValid())
					kwoTargetContainedObject->SetObjectValueAt(
					    livComputeModeTargetAttributeLoadIndexes.GetAt(1),
					    kwoPrevTargetContainedObject);
			}
		}
	}
	return &oaResult;
}

boolean KWDRBuildList::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nIndex;

	// Appel de la methode ancetre
	bOk = KWDRRelationCreationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation
	if (bOk)
	{
		assert(GetOperandNumber() == 1);
		assert(GetOutputOperandNumber() == 3);

		// Verification de chaque operande en sortie
		for (nIndex = 0; nIndex < GetOutputOperandNumber(); nIndex++)
			bOk = CheckOutputOperandCompletenessAt(kwcOwnerClass, nIndex) and bOk;
	}
	return bOk;
}

boolean KWDRBuildList::CheckOutputOperandCompletenessAt(const KWClass* kwcOwnerClass, int nIndex) const
{
	boolean bOk;
	ALString sExpectedObjectClassName;

	require(kwcOwnerClass != NULL);
	require(0 <= nIndex and nIndex < GetOutputOperandNumber());

	// Le premier operande en sortie (Data) doit etre du meme type que de la table en entree du premier operande
	assert(GetOutputOperandAt(nIndex)->GetType() == KWType::Object);
	if (nIndex == 0)
		sExpectedObjectClassName = GetOperandAt(0)->GetObjectClassName();
	// Les operandes 2 et 3 (Prev et Next) doivent etre du type de la table en sortie
	else
		sExpectedObjectClassName = GetObjectClassName();

	// Verification de la compatibilite du type
	bOk = GetOutputOperandAt(nIndex)->GetObjectClassName() == sExpectedObjectClassName;
	if (not bOk)
	{
		AddError("In the " + GetObjectClassName() + " output dictionary, the " +
			 KWType::ToString(KWType::Object) + "(" + GetOutputOperandAt(nIndex)->GetObjectClassName() +
			 ") " + GetOutputOperandAt(nIndex)->GetDataItemName() + " variable related to output operand " +
			 IntToString(nIndex + 1) + " should be of type " + KWType::ToString(KWType::Object) + "(" +
			 sExpectedObjectClassName + ")");
	}
	return bOk;
}

boolean KWDRBuildList::IsViewModeActivated() const
{
	return false;
}

void KWDRBuildList::CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const
{
	int nInputOperand;

	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Tous les operandes son ici obligatoires
	for (nInputOperand = 0; nInputOperand < GetOperandNumber(); nInputOperand++)
		ivUsedInputOperands->SetAt(nInputOperand, 1);
}

void KWDRBuildList::CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const
{
	require(0 <= nOutputOperand and nOutputOperand < GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Il n'y a ici pas d'operandes en entree associes aux operandes en sortie
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDummyTable

KWDRBuildDummyTable::KWDRBuildDummyTable()
{
	SetName("BuildDummyTable");
	SetLabel("Build dummy table");
	SetType(KWType::ObjectArray);

	// Un operande, pour specifier le nombre d'instance a cree
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);

	// Variables en entree: une pour le nombre d'instance a cree,
	// et un nombre variable d'operande pour alimenter les variables en sortie
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Unknown);
	SetVariableOperandNumber(true);

	// Variables en sortie: nombre variable d'operandes pour alimenter la table en sortie
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Unknown);
	SetVariableOutputOperandNumber(true);
}

KWDRBuildDummyTable::~KWDRBuildDummyTable() {}

KWDerivationRule* KWDRBuildDummyTable::Create() const
{
	return new KWDRBuildDummyTable;
}

ObjectArray* KWDRBuildDummyTable::ComputeObjectArrayResult(const KWObject* kwoObject,
							   const KWLoadIndex liAttributeLoadIndex) const
{
	const int nMaxTableSize = 1000000;
	Continuous cTableSize;
	int nTableSize;
	KWObject* kwoTargetContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Taille de la table a cree
	cTableSize = GetFirstOperand()->GetContinuousValue(kwoObject);
	nTableSize = 0;
	if (cTableSize != KWContinuous::GetMissingValue())
	{
		if (cTableSize < 0)
			nTableSize = 0;
		else if (cTableSize > nMaxTableSize)
			nTableSize = nMaxTableSize;
		else
			nTableSize = int(floor(cTableSize + 0.5));
	}

	// Creation des instances
	oaResult.SetSize(nTableSize);
	for (nObject = 0; nObject < nTableSize; nObject++)
	{
		kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);

		// Alimentation de type calcul pour les operandes en entree correspondant
		FillComputeModeTargetAttributesForVariableOperandNumber(kwoObject, kwoTargetContainedObject);
		oaResult.SetAt(nObject, kwoTargetContainedObject);
	}
	return &oaResult;
}

boolean KWDRBuildDummyTable::IsViewModeActivated() const
{
	return false;
}
