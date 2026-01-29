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
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildCompositeTable);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildEntityFromJson);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildList);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildGraph);
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
			if (kwoTargetContainedObject != NULL)
			{
				FillViewModeTargetAttributes(kwoSourceContainedObject, kwoTargetContainedObject);
				oaResult.SetAt(nObject, kwoTargetContainedObject);
			}
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

			// Creation et alimentation
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			if (kwoTargetContainedObject != NULL)
			{
				// Alimentation de type vue
				FillViewModeTargetAttributes(kwoSourceContainedObject, kwoTargetContainedObject);
				oaResult.SetAt(nObject, kwoTargetContainedObject);

				// Alimentation de type calcul pour les operandes en entree correspondant
				assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
				FillComputeModeTargetAttributesForVariableOperandNumber(kwoSourceContainedObject,
											kwoTargetContainedObject);
			}
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
		if (kwoTargetObject != NULL)
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
		// Creation et alimentation
		kwoTargetObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
		if (kwoTargetObject != NULL)
		{
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
	if (kwoTargetObject != NULL)
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

			// Creation et alimentation
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			if (kwoTargetContainedObject != NULL)
			{
				// Alimentation de type vue
				FillViewModeTargetAttributes(kwoSourceContainedCurrentObject, kwoTargetContainedObject);
				oaResult.SetAt(nObject, kwoTargetContainedObject);

				// Alimentation de type calcul pour les operandes en entree correspondant
				assert(GetOperandNumber() - 1 == ivComputeModeTargetAttributeTypes.GetSize());
				FillTargetDifferenceAttributes(kwoSourceContainedPreviousObject,
							       kwoSourceContainedCurrentObject,
							       kwoTargetContainedObject);

				// Memorisation du prochain objet precedent
				kwoSourceContainedPreviousObject = kwoSourceContainedCurrentObject;
			}
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
	require(GetOperandNumber() >= GetOutputOperandNumber());
	require(GetOutputOperandNumber() == ivComputeModeTargetAttributeTypes.GetSize());
	require(GetVariableOperandNumber());
	require(GetVariableOutputOperandNumber());

	// Retour si objet cible NULL
	if (kwoTargetObject == NULL)
		return;

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
		if (kwoSourcePreviousObject == NULL or kwoSourceCurrentObject == NULL)
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
// Classe KWDRBuildCompositeTable

KWDRBuildCompositeTable::KWDRBuildCompositeTable()
{
	SetName("BuildCompositeTable");
	SetLabel("Build composite table");
	SetType(KWType::ObjectArray);

	// Variables en entree: une table
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	SetVariableOperandNumber(true);

	// Variables en sortie: autant de variables qu'en entree
	SetOutputOperandNumber(1);
	GetOutputOperandAt(0)->SetType(KWType::Object);
	SetVariableOutputOperandNumber(true);
}

KWDRBuildCompositeTable::~KWDRBuildCompositeTable() {}

KWDerivationRule* KWDRBuildCompositeTable::Create() const
{
	return new KWDRBuildCompositeTable;
}

ObjectArray* KWDRBuildCompositeTable::ComputeObjectArrayResult(const KWObject* kwoObject,
							       const KWLoadIndex liAttributeLoadIndex) const
{
	int nOperand;
	ObjectArray oaAllObjectArrayOperand;
	ObjectArray* oaObjectArrayOperand;
	int nMaxTableSize;
	int nObject;
	KWObject* kwoCompositeObject;
	KWObject* kwoInputObject;

	require(IsCompiled());
	require(GetOperandNumber() == GetOutputOperandNumber());

	// Initialisation du resultat
	oaResult.SetSize(0);

	// Collecte des tables en entree, et calcul de la taille max;
	nMaxTableSize = 0;
	oaAllObjectArrayOperand.SetSize(GetOperandNumber());
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		oaObjectArrayOperand = GetOperandAt(nOperand)->GetObjectArrayValue(kwoObject);
		oaAllObjectArrayOperand.SetAt(nOperand, oaObjectArrayOperand);
		if (oaObjectArrayOperand != NULL)
			nMaxTableSize = max(nMaxTableSize, oaObjectArrayOperand->GetSize());
	}

	// Dimensionnement de la table composite
	oaResult.SetSize(nMaxTableSize);

	// Creation des objets composites
	for (nObject = 0; nObject < nMaxTableSize; nObject++)
	{
		// Creation et alimentation de l'objet
		kwoCompositeObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
		if (kwoCompositeObject != NULL)
			oaResult.SetAt(nObject, kwoCompositeObject);
	}

	// Alimentation des entites des objet composites
	for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
	{
		// On n'alimente que les attribut en sortie utilises
		if (livComputeModeTargetAttributeLoadIndexes.GetAt(nOperand).IsValid())
		{
			oaObjectArrayOperand = cast(ObjectArray*, oaAllObjectArrayOperand.GetAt(nOperand));
			if (oaObjectArrayOperand != NULL)
			{
				// Alimentation selon la taille de la table en entree
				for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
				{
					kwoCompositeObject = cast(KWObject*, oaResult.GetAt(nObject));
					if (kwoCompositeObject != NULL)
					{
						kwoInputObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
						kwoCompositeObject->SetObjectValueAt(
						    livComputeModeTargetAttributeLoadIndexes.GetAt(nOperand),
						    kwoInputObject);
					}
				}
			}
		}
	}
	return &oaResult;
}

boolean KWDRBuildCompositeTable::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nOperand;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDRRelationCreationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation
	if (bOk)
	{
		// Le nombre d'operandes en sortie doit etre egale a celui en entree
		if (GetOutputOperandNumber() != GetOperandNumber())
		{
			AddError(sTmp + "Number of output operands (" + IntToString(GetOutputOperandNumber()) +
				 ") should be the same as the number of input operands (" +
				 IntToString(GetOperandNumber()) + ")");
			bOk = false;
		}

		// Verification du type Object chaque operande en sortie
		if (bOk)
		{
			for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
			{
				bOk = CheckOutputOperandExpectedObjectType(
					  kwcOwnerClass, nOperand, GetOperandAt(nOperand)->GetObjectClassName()) and
				      bOk;
			}
		}
	}
	return bOk;
}

boolean KWDRBuildCompositeTable::IsViewModeActivated() const
{
	return false;
}

void KWDRBuildCompositeTable::CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const
{
	int nOperand;

	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Tous les operandes en entree sont obligatoires
	// Il faut en effet avoir toutes les tables en entree pour determiner la taille de la table en sortie
	for (nOperand = 0; nOperand < ivUsedInputOperands->GetSize(); nOperand++)
		ivUsedInputOperands->SetAt(nOperand, 1);
}

void KWDRBuildCompositeTable::CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const
{
	require(0 <= nOutputOperand and nOutputOperand < GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());
	require(GetOutputOperandNumber() == GetOperandNumber());

	// Chaque table en entree est necessaire si l'entite en sortie correspondante est utilisee
	ivUsedInputOperands->SetAt(nOutputOperand, 1);
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityFromJson

KWDRBuildEntityFromJson::KWDRBuildEntityFromJson()
{
	SetName("BuildEntityFromJson");
	SetLabel("Build entity from json value");
	SetType(KWType::Object);

	// Variables en entree: le champ json
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);

	// Initialisation des warnings
	ivWarningNumberPerType.SetSize(WarningTypeNumber);
	nMaxSavedWarningNumberPerType = 0;
}

KWDRBuildEntityFromJson::~KWDRBuildEntityFromJson() {}

KWDerivationRule* KWDRBuildEntityFromJson::Create() const
{
	return new KWDRBuildEntityFromJson;
}

KWObject* KWDRBuildEntityFromJson::ComputeObjectResult(const KWObject* kwoObject,
						       const KWLoadIndex liAttributeLoadIndex) const
{
	KWObject* kwoTargetObject;
	boolean bOk;
	Symbol sJsonValue;
	JSONObject jsonObject;
	StringVector svParsingErrorMessages;
	KWAttribute* attribute;
	ALString sMessage;
	ALString sSyntheticMessage;
	int n;

	require(IsCompiled());
	require(svWarnings.GetSize() == 0);

	// Par defaut, aucun objet n'est cree
	kwoTargetObject = NULL;

	// On va chercher la valeur json
	sJsonValue = GetFirstOperand()->GetTextValue(kwoObject);

	// Creation d'un objet si valeur non vide
	if (sJsonValue != Symbol())
	{
		// Lecture de l'objet json
		bOk = jsonObject.ReadString(sJsonValue, sJsonValue.GetLength(), &svParsingErrorMessages);

		// Creation de l'objet en sortie si pas de probleme de parsing json
		if (bOk)
		{
			// Calcul d'un nombre max de warnings a sauvegarder par type
			// On cherche a limiter le nombre total de warnings affiches par instances,
			// tout en l'augmentant selon le parametrage utilisateur
			nMaxSavedWarningNumberPerType =
			    1 + (int)ceil(sqrt(Global::GetMaxErrorFlowNumber()) / WarningTypeNumber);

			// Creation et alimentation l'objet en sortie
			assert(oaJsonPathMembers.GetSize() == 0);
			kwoTargetObject = CreateObjectFromJsonObject(kwoObject, liAttributeLoadIndex,
								     kwcCompiledTargetClass, &jsonObject);

			// Emission des warnings s'il y en a
			if (svWarnings.GetSize() > 0)
			{
				// Affichage des warnings memorise
				attribute = kwoObject->GetClass()->GetAttributeAtLoadIndex(liAttributeLoadIndex);
				for (n = 0; n < svWarnings.GetSize(); n++)
				{
					sMessage = "Calculation of the variable " + attribute->GetName() +
						   " using the rule " + GetName() + " : " + svWarnings.GetAt(n);
					kwoObject->AddWarning(sMessage);
				}

				// Affichage d'un message synthetique
				sSyntheticMessage = BuildSyntheticWarningMessage();
				if (sSyntheticMessage != "")
				{
					sMessage = "Calculation of the variable " + attribute->GetName() +
						   " using the rule " + GetName() + " : " + sSyntheticMessage;
					kwoObject->AddWarning(sMessage);
				}

				// Reinitialisation
				ResetWarnings();
			}
		}
		// Message d'erreur sinon
		else if (svParsingErrorMessages.GetSize() > 0)
		{
			attribute = kwoObject->GetClass()->GetAttributeAtLoadIndex(liAttributeLoadIndex);
			assert(attribute != NULL);

			// Au plus un seul message
			sMessage = "Calculation of the variable " + attribute->GetName() + " using the rule " +
				   GetName() + " failed during json parsing : " + svParsingErrorMessages.GetAt(0);
			kwoObject->AddWarning(sMessage);
		}
	}
	ensure(svWarnings.GetSize() == 0);
	return kwoTargetObject;
}

boolean KWDRBuildEntityFromJson::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWClass* kwcMainTargetClass;
	KWClass* kwcTargetClass;
	KWAttribute* targetAttribute;
	ObjectDictionary odTargetClasses;
	ObjectArray oaTargetClasses;
	int nClass;
	ALString sTmp;

	// Appel de la methode de KWDerivationRule, et non de la classe ancetre
	// On ne veut en effet pas des controles sur l'obligatoire d'avoir chaque attribut cible alimente
	// par un mecanisme de type vue ou calcul
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation
	if (bOk)
	{
		// Recherche de la classe cible
		kwcMainTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
		assert(kwcMainTargetClass != NULL);

		// La classe cible ne doit pas etre Root
		if (kwcMainTargetClass->GetRoot())
		{
			AddError("Invalid output dictionary " + kwcMainTargetClass->GetName() +
				 " that cannot be a root dictionary, dedicated to managing external tables");
			bOk = false;
		}

		// Il ne doit pas y avoir de cycle dans le schema de la classe cible
		if (not kwcMainTargetClass->CheckNativeAcyclicity())
		{
			// Message d'erreur lie a la regle, en complement de l'erreur detectee pour les cycles dans le dictionnaire
			AddError("Invalid output dictionary " + kwcMainTargetClass->GetName() +
				 ", as it contains a composition cycle.");
			bOk = false;
		}

		// Test de l'absence de blocs de variables natives dans l'ensemble des classes cibles
		if (bOk)
		{
			// On initialise le tableau de toutes les classes cibles avec la classe cible principale
			oaTargetClasses.Add(kwcMainTargetClass);
			odTargetClasses.SetAt(kwcMainTargetClass->GetName(), kwcMainTargetClass);

			// Parcours des classes cibles
			for (nClass = 0; nClass < oaTargetClasses.GetSize(); nClass++)
			{
				kwcTargetClass = cast(KWClass*, oaTargetClasses.GetAt(nClass));

				// Parcours des attributs cibles natifs
				targetAttribute = kwcTargetClass->GetHeadAttribute();
				while (targetAttribute != NULL)
				{
					// On ne traite que les attributs natifs
					if (targetAttribute->GetAnyDerivationRule() == NULL)
					{
						// La variable ne doiut pas etre dans un bloc
						if (targetAttribute->IsInBlock())
						{
							AddError(sTmp + "In output dictionary " +
								 kwcTargetClass->GetName() + " variable " +
								 targetAttribute->GetName() + " (type " +
								 KWType::ToString(targetAttribute->GetType()) +
								 ") must not be in a sparse variable block (" +
								 targetAttribute->GetAttributeBlock()->GetName() + ")");
							bOk = false;
						};

						// Ajout si necessaire d'une nouvelle classe cible
						if (KWType::IsRelation(targetAttribute->GetType()))
						{
							if (targetAttribute->GetClass() != NULL and
							    odTargetClasses.Lookup(
								targetAttribute->GetClass()->GetName()) == NULL)
							{
								oaTargetClasses.Add(targetAttribute->GetClass());
								odTargetClasses.SetAt(
								    targetAttribute->GetClass()->GetName(),
								    targetAttribute->GetClass());
							}
						}
					}

					// Attribut suivant
					kwcTargetClass->GetNextAttribute(targetAttribute);
				}
			}
		}
	}
	return bOk;
}

void KWDRBuildEntityFromJson::Compile(KWClass* kwcOwnerClass)
{
	KWClass* kwcMainTargetClass;
	KWClass* kwcTargetClass;
	KWAttribute* targetAttribute;
	ObjectDictionary odTargetClasses;
	ObjectArray oaTargetClasses;
	int nClass;
	int nNativeAttributeNumber;
	KWAttribute* nativeTargetAttribute;

	// Appel de la methode ancetre
	KWDRRelationCreationRule::Compile(kwcOwnerClass);

	// Nettoyage prealable
	odCompiledSingleNativeAttributePerSingletonClass.RemoveAll();

	// Recherche de la classe cible
	kwcMainTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
	assert(kwcMainTargetClass != NULL);

	// On initialise le tableau de toutes les classes cibles avec la classe cible principale
	oaTargetClasses.Add(kwcMainTargetClass);
	odTargetClasses.SetAt(kwcMainTargetClass->GetName(), kwcMainTargetClass);

	// Parcours des classes cibles
	for (nClass = 0; nClass < oaTargetClasses.GetSize(); nClass++)
	{
		kwcTargetClass = cast(KWClass*, oaTargetClasses.GetAt(nClass));

		// Parcours des attributs cibles natifs
		nativeTargetAttribute = NULL;
		nNativeAttributeNumber = 0;
		targetAttribute = kwcTargetClass->GetHeadAttribute();
		while (targetAttribute != NULL)
		{
			// On ne traite que les attributs natifs hors blocs sparse
			if (targetAttribute->GetAnyDerivationRule() == NULL and not targetAttribute->IsInBlock())
			{
				nNativeAttributeNumber++;
				nativeTargetAttribute = targetAttribute;

				// Ajout si necessaire d'une nouvelle classe cible
				if (KWType::IsRelation(targetAttribute->GetType()))
				{
					if (targetAttribute->GetClass() != NULL and
					    odTargetClasses.Lookup(targetAttribute->GetClass()->GetName()) == NULL)
					{
						oaTargetClasses.Add(targetAttribute->GetClass());
						odTargetClasses.SetAt(targetAttribute->GetClass()->GetName(),
								      targetAttribute->GetClass());
					}
				}
			}

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(targetAttribute);
		}

		// Memorisation de la classe dans le cas d'un seul attribut natif
		if (nNativeAttributeNumber == 1)
			odCompiledSingleNativeAttributePerSingletonClass.SetAt(kwcTargetClass->GetName(),
									       nativeTargetAttribute);
	}
}

longint KWDRBuildEntityFromJson::GetUsedMemory() const
{
	longint lUsedMemory;
	KWDerivationRuleOperand* operand;

	// Appel de la methode ancetre
	lUsedMemory = KWDRRelationCreationRule::GetUsedMemory();

	// Specialisation de la methode
	lUsedMemory += sizeof(KWDRBuildEntityFromJson) - sizeof(KWDRRelationCreationRule);
	lUsedMemory += odCompiledSingleNativeAttributePerSingletonClass.GetUsedMemory() - sizeof(ObjectDictionary);
	lUsedMemory += svWarnings.GetUsedMemory() - sizeof(StringVector);
	lUsedMemory += ivWarningNumberPerType.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += oaJsonPathMembers.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += ivJsonPathArrayIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

boolean KWDRBuildEntityFromJson::IsViewModeActivated() const
{
	return false;
}

KWObject* KWDRBuildEntityFromJson::CreateObjectFromJsonObject(const KWObject* kwoOwnerObject,
							      const KWLoadIndex liAttributeLoadIndex,
							      const KWClass* kwcCreationClass,
							      const JSONObject* jsonObject) const
{
	boolean bTrace = false;
	KWObject* kwoTargetObject;
	KWObject* kwoTargetSubObject;
	KWAttribute* attribute;
	KWAttribute* singleNativeAttribute;
	ObjectArray* oaSubObjects;
	JSONMember* jsonMember;
	JSONValue* jsonValue;
	int nMember;
	int n;

	require(kwoOwnerObject != NULL);
	require(liAttributeLoadIndex.IsValid());
	require(kwcCreationClass != NULL);
	require(jsonObject != NULL);

	// Creation de l'objet en sortie
	// Il n'est pas de type vue, car il doit detruire son contenu natif
	kwoTargetObject = NewObject(kwoOwnerObject, liAttributeLoadIndex, kwcCreationClass, false);

	// Initialisation des valeurs numeriques a Missing
	// Ce n'est pas la peine pour les autres types de valeur
	if (kwcCreationClass->GetUsedAttributeNumberForType(KWType::Continuous) > 0)
	{
		for (n = 0; n < kwcCreationClass->GetLoadedAttributeNumber(); n++)
		{
			attribute = kwcCreationClass->GetLoadedAttributeAt(n);
			if (attribute->GetType() == KWType::Continuous and attribute->GetAnyDerivationRule() == NULL)
				kwoTargetObject->SetContinuousValueAt(attribute->GetLoadIndex(),
								      KWContinuous::GetMissingValue());
		}
	}

	// Alimentation de l'objet
	if (kwoTargetObject != NULL)
	{
		// Parcours du contenu de l'objet
		for (nMember = 0; nMember < jsonObject->GetMemberNumber(); nMember++)
		{
			jsonMember = jsonObject->GetMemberAt(nMember);

			// Recherche de l'attribut correspondant a la cle
			attribute = kwcCreationClass->LookupAttribute(jsonMember->GetKey());

			// Warning si attribut non trouve
			if (attribute == NULL)
				AddMissingAttributeWarning(jsonMember, kwcCreationClass);
			// Warning si attribut derive
			else if (attribute->GetAnyDerivationRule() != NULL)
				AddDerivedAttributeWarning(jsonMember, jsonMember->GetValue(), attribute);
			// Alimentation si attribut natif existant
			else
			{
				// Verifie lors du CheckCompleteness
				assert(not attribute->IsInBlock());

				// Alimentation si attribut charge en memoire
				if (attribute->GetLoadIndex().IsValid())
				{
					// Cas particulier d'un attribut de type Text
					if (attribute->GetType() == KWType::Text)
					{
						// Valeur si type json string
						if (jsonMember->GetValueType() == JSONValue::StringValue)
							kwoTargetObject->SetTextValueAt(
							    attribute->GetLoadIndex(),
							    (Symbol)jsonMember->GetStringValue()->GetString());
						// Valeur json brute sinon
						else
							kwoTargetObject->SetTextValueAt(
							    attribute->GetLoadIndex(),
							    (Symbol)jsonMember->GetValue()->BuildCompactJsonValue());
					}
					// Sinon, alimentation selon le type, si le type destination est compatible
					else
					{
						switch (jsonMember->GetValueType())
						{
						case JSONValue::StringValue:
							FillSymbolAttributeFromJsonString(attribute, jsonMember,
											  jsonMember->GetStringValue(),
											  kwoTargetObject);
							break;
						case JSONValue::NumberValue:
							if (attribute->GetType() == KWType::Continuous)
								kwoTargetObject->SetContinuousValueAt(
								    attribute->GetLoadIndex(),
								    jsonMember->GetNumberValue()->GetNumber());
							else if (attribute->GetType() == KWType::Symbol)
								kwoTargetObject->SetSymbolValueAt(
								    attribute->GetLoadIndex(),
								    KWContinuous::ContinuousToString(
									jsonMember->GetNumberValue()->GetNumber()));
							// Warning si type incompatible
							else
								AddInconsistentAttributeTypeWarning(
								    jsonMember, jsonMember->GetValue(), attribute);
							break;
						case JSONValue::BooleanValue:
							if (attribute->GetType() == KWType::Symbol)
							{
								if (jsonMember->GetBooleanValue()->GetBoolean())
									kwoTargetObject->SetSymbolValueAt(
									    attribute->GetLoadIndex(), Symbol("true"));
								else
									kwoTargetObject->SetSymbolValueAt(
									    attribute->GetLoadIndex(), Symbol("false"));
							}
							// Warning si type incompatible
							else
								AddInconsistentAttributeTypeWarning(
								    jsonMember, jsonMember->GetValue(), attribute);
							break;
						case JSONValue::NullValue:
							if (attribute->GetType() == KWType::Symbol)
								kwoTargetObject->SetSymbolValueAt(
								    attribute->GetLoadIndex(), Symbol("null"));
							// Warning si type incompatible
							else
								AddInconsistentAttributeTypeWarning(
								    jsonMember, jsonMember->GetValue(), attribute);
							break;
						case JSONValue::ObjectValue:
							if (attribute->GetType() == KWType::Object)
							{
								// Creation et alimentation l'objet en sortie
								PushJsonPath(jsonMember, 0);
								kwoTargetSubObject = CreateObjectFromJsonObject(
								    kwoTargetObject, attribute->GetLoadIndex(),
								    attribute->GetClass(),
								    jsonMember->GetObjectValue());
								PopJsonPath();

								// On le memorise meme s'il est NULL: ce n'est pas un probleme
								kwoTargetObject->SetObjectValueAt(
								    attribute->GetLoadIndex(), kwoTargetSubObject);
							}
							// Warning si type incompatible
							else
								AddInconsistentAttributeTypeWarning(
								    jsonMember, jsonMember->GetValue(), attribute);
							break;
						case JSONValue::ArrayValue:
							if (attribute->GetType() == KWType::ObjectArray)
							{
								// Creation et memorisation d'un tableau de sous-objets
								oaSubObjects = new ObjectArray;
								kwoTargetObject->SetObjectArrayValueAt(
								    attribute->GetLoadIndex(), oaSubObjects);

								// Recherche de l'unique attribut natif de la classe, pour les classes concernees
								singleNativeAttribute = cast(
								    KWAttribute*,
								    odCompiledSingleNativeAttributePerSingletonClass
									.Lookup(attribute->GetClass()->GetName()));

								// Parcours de valeurs de l'objet json pour creer les sous-objets
								for (n = 0;
								     n < jsonMember->GetArrayValue()->GetValueNumber();
								     n++)
								{
									jsonValue =
									    jsonMember->GetArrayValue()->GetValueAt(n);

									// Ajout dans le path json
									PushJsonPath(jsonMember, n);

									// Cas standard de valeur de type objet
									if (jsonValue->GetType() ==
									    JSONValue::ObjectValue)
									{
										// Creation d'un KWObject a partir d'un objet json
										kwoTargetSubObject =
										    CreateObjectFromJsonObject(
											kwoTargetObject,
											attribute->GetLoadIndex(),
											attribute->GetClass(),
											jsonValue->GetObjectValue());

										// On ne le memorise que s'il n'est pas NULL, car les tableaux
										// ne peuvent pas avoir de valeur NULL
										// On n'interrompt pas l'alimenattion, pour continuer a collecter
										// des stats sur les probleme memoire potentiels
										if (kwoTargetSubObject != NULL)
											oaSubObjects->Add(
											    kwoTargetSubObject);
									}
									// Autre cas
									else
									{
										if (singleNativeAttribute == NULL)
											AddMissingSingleAttributeWarning(
											    jsonMember,
											    kwcCreationClass);
										// Cas particulier d'une classe ayant un seul attribut natif, pouvant potentiellemen
										// acceuillir des valeur json litterales (number, string...)
										else if (KWType::IsStored(
											     singleNativeAttribute
												 ->GetType()))
										{
											// Tentative de creation d'un KWObject a partir d'une valeur json
											kwoTargetSubObject =
											    CreateObjectFromJsonValue(
												kwoTargetObject,
												attribute
												    ->GetLoadIndex(),
												attribute->GetClass(),
												singleNativeAttribute,
												jsonMember, jsonValue);

											// On ne le memorise que s'il n'est pas NULL
											if (kwoTargetSubObject != NULL)
												oaSubObjects->Add(
												    kwoTargetSubObject);
										}
										// Type incompatible sinon
										else
											AddInconsistentAttributeTypeWarning(
											    NULL, jsonValue,
											    singleNativeAttribute);
									}

									// Supression du path json
									PopJsonPath();
								}
							}
							// Warning si type incompatible
							else
								AddInconsistentAttributeTypeWarning(
								    jsonMember, jsonMember->GetValue(), attribute);
							break;
						default:
							assert(false);
							break;
						}
					}
				}
			}
		}

		// Trace
		if (bTrace)
		{
			cout << kwoOwnerObject->GetObjectLabel() << "\t" << kwoTargetObject->GetObjectLabel() << "\t"
			     << BuildCurrentJsonPath() << endl;
			kwoTargetObject->PrettyWrite(cout, "  ");
		}
	}
	return kwoTargetObject;
}

KWObject* KWDRBuildEntityFromJson::CreateObjectFromJsonValue(
    const KWObject* kwoOwnerObject, const KWLoadIndex liAttributeLoadIndex, const KWClass* kwcCreationClass,
    const KWAttribute* singleNativeAttribute, const JSONMember* jsonOwnerArray, const JSONValue* jsonValue) const
{
	boolean bTrace = false;
	boolean bInconsistentType;
	KWObject* kwoTargetObject;

	require(kwoOwnerObject != NULL);
	require(liAttributeLoadIndex.IsValid());
	require(kwcCreationClass != NULL);
	require(singleNativeAttribute != NULL);
	require(odCompiledSingleNativeAttributePerSingletonClass.Lookup(kwcCreationClass->GetName()) ==
		singleNativeAttribute);
	require(KWType::IsStored(singleNativeAttribute->GetType()));
	require(not singleNativeAttribute->IsInBlock());
	require(singleNativeAttribute->GetAnyDerivationRule() == NULL);
	require(jsonOwnerArray != NULL);
	require(jsonOwnerArray->GetValueType() == JSONValue::ArrayValue);
	require(jsonValue != NULL);

	// Creation de l'objet en sortie
	kwoTargetObject = NewObject(kwoOwnerObject, liAttributeLoadIndex, kwcCreationClass, false);

	// Creation et alimentation de l'objet si les types sont compatibles
	if (kwoTargetObject != NULL)
	{
		bInconsistentType = false;
		if (singleNativeAttribute->GetLoadIndex().IsValid())
		{
			// Cas particulier d'un attribut de type Text
			if (singleNativeAttribute->GetType() == KWType::Text)
			{
				// Valeur si type json string
				if (jsonValue->GetType() == JSONValue::StringValue)
					kwoTargetObject->SetTextValueAt(
					    singleNativeAttribute->GetLoadIndex(),
					    (Symbol)jsonValue->GetStringValue()->GetString());
				// Valeur json brute si,on
				else
					kwoTargetObject->SetTextValueAt(singleNativeAttribute->GetLoadIndex(),
									(Symbol)jsonValue->BuildCompactJsonValue());
			}
			// Alimentation selon le type, si le type destination est compatible
			else
			{
				switch (jsonValue->GetType())
				{
				case JSONValue::StringValue:
					FillSymbolAttributeFromJsonString(singleNativeAttribute, jsonOwnerArray,
									  jsonValue->GetStringValue(), kwoTargetObject);
					break;
				case JSONValue::NumberValue:
					if (singleNativeAttribute->GetType() == KWType::Continuous)
						kwoTargetObject->SetContinuousValueAt(
						    singleNativeAttribute->GetLoadIndex(),
						    jsonValue->GetNumberValue()->GetNumber());
					else if (singleNativeAttribute->GetType() == KWType::Symbol)
						kwoTargetObject->SetSymbolValueAt(
						    singleNativeAttribute->GetLoadIndex(),
						    KWContinuous::ContinuousToString(
							jsonValue->GetNumberValue()->GetNumber()));
					// Warning si type incompatible
					else
					{
						bInconsistentType = true;
						AddInconsistentAttributeTypeWarning(NULL, jsonValue,
										    singleNativeAttribute);
					}
					break;
				case JSONValue::BooleanValue:
					if (singleNativeAttribute->GetType() == KWType::Symbol)
					{
						if (jsonValue->GetBooleanValue()->GetBoolean())
							kwoTargetObject->SetSymbolValueAt(
							    singleNativeAttribute->GetLoadIndex(), Symbol("true"));
						else
							kwoTargetObject->SetSymbolValueAt(
							    singleNativeAttribute->GetLoadIndex(), Symbol("false"));
					}
					else
					{
						bInconsistentType = true;
						AddInconsistentAttributeTypeWarning(NULL, jsonValue,
										    singleNativeAttribute);
					}
					break;
				case JSONValue::NullValue:
					if (singleNativeAttribute->GetType() == KWType::Symbol)
						kwoTargetObject->SetSymbolValueAt(singleNativeAttribute->GetLoadIndex(),
										  Symbol("null"));
					else
					{
						bInconsistentType = true;
						AddInconsistentAttributeTypeWarning(NULL, jsonValue,
										    singleNativeAttribute);
					}
					break;
				default:
					bInconsistentType = true;
					AddInconsistentAttributeTypeWarning(NULL, jsonValue, singleNativeAttribute);
					break;
				}
			}

			// Si l'initialisation s'est bien passee, la valeur est forcement non manquante dans le cas numerique
			assert(not bInconsistentType or singleNativeAttribute->GetType() != KWType::Continuous or
			       kwoTargetObject->GetContinuousValueAt(singleNativeAttribute->GetLoadIndex()) !=
				   KWContinuous::GetMissingValue());

			// Destruction de l'objet si type incompatible
			if (bInconsistentType)
			{
				delete kwoTargetObject;
				kwoTargetObject = NULL;
			}
		}
	}

	// Trace
	if (bTrace and kwoTargetObject != NULL)
	{
		cout << kwoOwnerObject->GetObjectLabel() << "\t" << kwoTargetObject->GetObjectLabel() << "\t"
		     << BuildCurrentJsonPath() << endl;
		kwoTargetObject->PrettyWrite(cout, "  ");
	}
	return kwoTargetObject;
}

void KWDRBuildEntityFromJson::FillSymbolAttributeFromJsonString(const KWAttribute* attribute,
								const JSONMember* jsonOwnerMember,
								const JSONString* jsonString,
								KWObject* kwoTargetObject) const
{
	const JSONMember* jsonWarningOwnerMember;
	Symbol sValue;
	Date dtValue;
	Time tmValue;
	Timestamp tsValue;
	TimestampTZ tstzValue;
	int nValueLength;

	require(attribute != NULL);
	require(jsonString != NULL);
	require(jsonOwnerMember->GetValue() == jsonString or jsonOwnerMember->GetValueType() == JSONValue::ArrayValue);
	require(kwoTargetObject != NULL);
	require(kwoTargetObject->GetClass()->LookupAttribute(attribute->GetName()) == attribute);

	// Parametre pour les warnings dans le cas standard
	if (jsonOwnerMember->GetValue() == jsonString)
		jsonWarningOwnerMember = jsonOwnerMember;
	// Et dans le cas d'une valeur dans un tableau
	else
		jsonWarningOwnerMember = NULL;

	// On laisse la valeur par defaut si la taille est 0
	nValueLength = jsonString->GetString().GetLength();
	if (nValueLength > 0)
	{
		// Cas attribut Symbol
		switch (attribute->GetType())
		{
		case KWType::Symbol:
			// Troncature si necessaire des champs trop longs
			if (nValueLength > KWValue::nMaxSymbolFieldSize)
			{
				sValue = (Symbol)jsonString->GetString().Left(KWValue::nMaxSymbolFieldSize);
				AddOverlengthySymbolValueWarning(jsonWarningOwnerMember, jsonString, attribute);
			}
			else
				sValue = (Symbol)jsonString->GetString();
			kwoTargetObject->SetSymbolValueAt(attribute->GetLoadIndex(), sValue);
			break;

		case KWType::Date:
			dtValue = attribute->GetDateFormat()->StringToDate(jsonString->GetString());
			kwoTargetObject->SetDateValueAt(attribute->GetLoadIndex(), dtValue);
			if (not dtValue.Check())
				AddInvalidTemporalValueWarning(jsonWarningOwnerMember, jsonString, attribute);
			break;

		case KWType::Time:
			tmValue = attribute->GetTimeFormat()->StringToTime(jsonString->GetString());
			kwoTargetObject->SetTimeValueAt(attribute->GetLoadIndex(), tmValue);
			if (not tmValue.Check())
				AddInvalidTemporalValueWarning(jsonWarningOwnerMember, jsonString, attribute);
			break;

		case KWType::Timestamp:
			tsValue = attribute->GetTimestampFormat()->StringToTimestamp(jsonString->GetString());
			kwoTargetObject->SetTimestampValueAt(attribute->GetLoadIndex(), tsValue);
			if (not tsValue.Check())
				AddInvalidTemporalValueWarning(jsonWarningOwnerMember, jsonString, attribute);
			break;

		case KWType::TimestampTZ:
			tstzValue = attribute->GetTimestampTZFormat()->StringToTimestampTZ(jsonString->GetString());
			kwoTargetObject->SetTimestampTZValueAt(attribute->GetLoadIndex(), tstzValue);
			if (not tstzValue.Check())
				AddInvalidTemporalValueWarning(jsonWarningOwnerMember, jsonString, attribute);
			break;

		case KWType::Text:
			// Les champs trop longs sont tronques directement par le inputBuffer
			kwoTargetObject->SetTextValueAt(attribute->GetLoadIndex(), (Symbol)jsonString->GetString());
			break;

		default:
			AddInconsistentAttributeTypeWarning(jsonWarningOwnerMember, jsonString, attribute);
			break;
		}
	}
}

void KWDRBuildEntityFromJson::AddMissingAttributeWarning(const JSONMember* jsonMember, const KWClass* targetClass) const
{
	ALString sMessage;
	ALString sJsonPath;

	require(jsonMember != NULL);
	require(targetClass != NULL);

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(MissingAttributeWarning, 1);
	if (ivWarningNumberPerType.GetAt(MissingAttributeWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path en tenant compte du membre en cours
		PushJsonPath(jsonMember, 0);
		sJsonPath = BuildCurrentJsonPath();
		PopJsonPath();

		// Message
		sMessage = "variable " + jsonMember->GetKey() + " not found in output dictionary " +
			   targetClass->GetName() + " for value at json path " + sJsonPath;
		svWarnings.Add(sMessage);
	}
}

void KWDRBuildEntityFromJson::AddMissingSingleAttributeWarning(const JSONMember* jsonMember,
							       const KWClass* targetClass) const
{
	ALString sMessage;
	ALString sJsonPath;

	require(jsonMember != NULL);
	require(targetClass != NULL);
	require(odCompiledSingleNativeAttributePerSingletonClass.Lookup(targetClass->GetName()) == NULL);

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(MissingSingleAttributeWarning, 1);
	if (ivWarningNumberPerType.GetAt(MissingSingleAttributeWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path directement, puisque l'on est deja dans une valeur de tableau
		sJsonPath = BuildCurrentJsonPath();

		// Message
		sMessage = "output dictionary " + targetClass->GetName() +
			   " should contain one single native variable for the array value at json path " + sJsonPath;
		svWarnings.Add(sMessage);
	}
}

void KWDRBuildEntityFromJson::AddDerivedAttributeWarning(const JSONMember* jsonMember, const JSONValue* jsonValue,
							 const KWAttribute* targetAttribute) const
{
	ALString sMessage;
	ALString sJsonPath;
	ALString sTmp;

	require(jsonMember == NULL or jsonMember->GetValue() == jsonValue);
	require(targetAttribute != NULL);
	require(jsonMember == NULL or jsonMember->GetKey() == targetAttribute->GetName());

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(DerivedAttributeWarning, 1);
	if (ivWarningNumberPerType.GetAt(DerivedAttributeWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path en tenant compte du membre en cours
		PushJsonPath(jsonMember, 0);
		sJsonPath = BuildCurrentJsonPath();
		PopJsonPath();

		// Message, en precisant que le dictionnaire en sortie est singleton dans le cas d'une valeur de tableau
		sMessage = sTmp + KWType::ToString(targetAttribute->GetType()) + " variable " +
			   targetAttribute->GetName() + " in output ";
		if (jsonMember == NULL)
			sMessage += KWType::ToString(KWType::ObjectArray) + " ";
		sMessage += "dictionary " + targetAttribute->GetParentClass()->GetName() +
			    " is derived and cannot be initialized with json " + jsonValue->TypeToString() +
			    " value (" + jsonValue->BuildDisplayedJsonValue() + ") at json path " + sJsonPath;
		if (jsonMember == NULL)
			sMessage += ", the json array value is ignored";
		svWarnings.Add(sMessage);
	}
}

void KWDRBuildEntityFromJson::AddInconsistentAttributeTypeWarning(const JSONMember* jsonMember,
								  const JSONValue* jsonValue,
								  const KWAttribute* targetAttribute) const
{
	ALString sMessage;
	ALString sJsonPath;
	ALString sTmp;

	require(jsonMember == NULL or jsonMember->GetValue() == jsonValue);
	require(targetAttribute != NULL);
	require(jsonMember == NULL or jsonMember->GetKey() == targetAttribute->GetName());

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(InconsistentAttributeTypeWarning, 1);
	if (ivWarningNumberPerType.GetAt(InconsistentAttributeTypeWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path en tenant compte du membre en cours
		if (jsonMember != NULL)
			PushJsonPath(jsonMember, 0);
		sJsonPath = BuildCurrentJsonPath();
		if (jsonMember != NULL)
			PopJsonPath();

		// Message, en precisant que le dictionnaire en sortie est singleton dans le cas d'une valeur de tableau
		sMessage = sTmp + KWType::ToString(targetAttribute->GetType()) + " variable " +
			   targetAttribute->GetName() + " in output ";
		if (jsonMember == NULL)
			sMessage += KWType::ToString(KWType::ObjectArray) + " ";
		sMessage += "dictionary " + targetAttribute->GetParentClass()->GetName() +
			    " of type incompatible with json " + jsonValue->TypeToString() + " value (" +
			    jsonValue->BuildDisplayedJsonValue() + ") at json path " + sJsonPath;
		if (jsonMember == NULL)
			sMessage += ", the json array value is ignored";
		svWarnings.Add(sMessage);
	}
}

void KWDRBuildEntityFromJson::AddInvalidTemporalValueWarning(const JSONMember* jsonMember, const JSONValue* jsonValue,
							     const KWAttribute* targetAttribute) const
{
	ALString sMessage;
	ALString sJsonPath;
	ALString sTemporalFormatInfo;
	ALString sTmp;

	require(jsonMember == NULL or jsonMember->GetValue() == jsonValue);
	require(targetAttribute != NULL);
	require(KWType::IsComplex(targetAttribute->GetType()));
	require(jsonMember == NULL or jsonMember->GetKey() == targetAttribute->GetName());
	require(jsonValue->GetType() == JSONValue::StringValue);

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(InvalidTemporalValueWarning, 1);
	if (ivWarningNumberPerType.GetAt(InvalidTemporalValueWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path en tenant compte du membre en cours
		if (jsonMember != NULL)
			PushJsonPath(jsonMember, 0);
		sJsonPath = BuildCurrentJsonPath();
		if (jsonMember != NULL)
			PopJsonPath();

		// Calcul des informations sur le format
		sTemporalFormatInfo = targetAttribute->GetFormatMetaDataKey(targetAttribute->GetType());
		sTemporalFormatInfo += '=';
		sTemporalFormatInfo += '"';
		if (targetAttribute->GetType() == KWType::Time)
			sTemporalFormatInfo += targetAttribute->GetTimeFormat()->GetFormatString();
		else if (targetAttribute->GetType() == KWType::Date)
			sTemporalFormatInfo += targetAttribute->GetDateFormat()->GetFormatString();
		else if (targetAttribute->GetType() == KWType::Timestamp)
			sTemporalFormatInfo += targetAttribute->GetTimestampFormat()->GetFormatString();
		else if (targetAttribute->GetType() == KWType::TimestampTZ)
			sTemporalFormatInfo += targetAttribute->GetTimestampTZFormat()->GetFormatString();
		sTemporalFormatInfo += '"';

		// Message, en precisant que le dictionnaire en sortie est singleton dans le cas d'une valeur de tableau
		sMessage = sTmp + KWType::ToString(targetAttribute->GetType()) + " variable " +
			   targetAttribute->GetName() + " (" + sTemporalFormatInfo + ") in output ";
		if (jsonMember == NULL)
			sMessage += KWType::ToString(KWType::ObjectArray) + " ";
		sMessage += "dictionary " + targetAttribute->GetParentClass()->GetName() +
			    " with missing value due to invalid temporal format of json " + jsonValue->TypeToString() +
			    " value (" + jsonValue->BuildDisplayedJsonValue() + ") at json path " + sJsonPath;
		svWarnings.Add(sMessage);
	}
}

void KWDRBuildEntityFromJson::AddOverlengthySymbolValueWarning(const JSONMember* jsonMember, const JSONValue* jsonValue,
							       const KWAttribute* targetAttribute) const
{
	ALString sMessage;
	ALString sJsonPath;
	ALString sTmp;

	require(jsonMember == NULL or jsonMember->GetValue() == jsonValue);
	require(targetAttribute != NULL);
	require(jsonMember == NULL or jsonMember->GetKey() == targetAttribute->GetName());

	// Memorisation uniquement des premiers warnings
	ivWarningNumberPerType.UpgradeAt(OverlengthySymbolValueWarning, 1);
	if (ivWarningNumberPerType.GetAt(OverlengthySymbolValueWarning) <= nMaxSavedWarningNumberPerType)
	{
		// Calcul du json path en tenant compte du membre en cours
		if (jsonMember != NULL)
			PushJsonPath(jsonMember, 0);
		sJsonPath = BuildCurrentJsonPath();
		if (jsonMember != NULL)
			PopJsonPath();

		// Message, en precisant que le dictionnaire en sortie est singleton dans le cas d'une valeur de tableau
		sMessage = sTmp + KWType::ToString(targetAttribute->GetType()) + " variable " +
			   targetAttribute->GetName() + " in output ";
		if (jsonMember == NULL)
			sMessage += KWType::ToString(KWType::ObjectArray) + " ";
		sMessage += "dictionary " + targetAttribute->GetParentClass()->GetName() + " with value truncated to " +
			    IntToString(KWValue::nMaxSymbolFieldSize) + " characters to due to overlengthy json " +
			    jsonValue->TypeToString() +
			    " value (length=" + IntToString(jsonValue->GetStringValue()->GetString().GetLength()) +
			    ", value=" + jsonValue->BuildDisplayedJsonValue() + ") at json path " + sJsonPath;
		svWarnings.Add(sMessage);
	}
}

const ALString KWDRBuildEntityFromJson::BuildSyntheticWarningMessage() const
{
	ALString sMessage;
	int nWarningNumber;
	boolean bSyntheticWarningNecessary;
	int nTotalWarnings;
	int nTotalWarningTypes;
	int nDisplayedTypeNumber;
	int nWarningType;
	ALString sTmp;

	// Statistiqtiques sur les types de warnings
	bSyntheticWarningNecessary = false;
	nTotalWarnings = 0;
	nTotalWarningTypes = 0;
	for (nWarningType = 0; nWarningType < WarningTypeNumber; nWarningType++)
	{
		nWarningNumber = ivWarningNumberPerType.GetAt(nWarningType);
		bSyntheticWarningNecessary =
		    bSyntheticWarningNecessary or nWarningNumber > nMaxSavedWarningNumberPerType;
		nTotalWarnings += nWarningNumber;
		if (nWarningNumber > 0)
			nTotalWarningTypes++;
	}

	// Construction d'un message synthetique si necessaire
	if (bSyntheticWarningNecessary)
	{
		// Nombre total de warnings
		sMessage += sTmp + "overall " + IntToString(nTotalWarnings) + " warnings";

		// Decomposition par type de warning uniquement si necessaire
		if (nTotalWarningTypes > 1)
			sMessage += ", including ";

		// Detail par type de warning
		nDisplayedTypeNumber = 0;
		for (nWarningType = 0; nWarningType < WarningTypeNumber; nWarningType++)
		{
			nWarningNumber = ivWarningNumberPerType.GetAt(nWarningType);
			if (nWarningNumber > 0)
			{
				// Separateur si necessaire
				if (nDisplayedTypeNumber > 0)
					sMessage += ", ";

				// On indique le nombre de warnings specifique que s'il y a plusieurs types
				if (nTotalWarningTypes > 1)
					sMessage += IntToString(nWarningNumber);

				// Message specifique par type de warning
				sMessage += " for ";
				switch (nWarningType)
				{
				case MissingAttributeWarning:
					sMessage += "missing target variable";
					break;
				case MissingSingleAttributeWarning:
					sMessage += "target dictionary without one single native variable";
					break;
				case DerivedAttributeWarning:
					sMessage += "derived target variable";
					break;
				case InconsistentAttributeTypeWarning:
					sMessage += "incompatible type";
					break;
				case InvalidTemporalValueWarning:
					sMessage += "invalid temporal format";
					break;
				case OverlengthySymbolValueWarning:
					sMessage += "overlengthy string value";
					break;
				default:
					assert(false);
					break;
				}
				nDisplayedTypeNumber++;
			}
		}
	}
	return sMessage;
}

void KWDRBuildEntityFromJson::ResetWarnings() const
{
	require(ivWarningNumberPerType.GetSize() == WarningTypeNumber);
	ivWarningNumberPerType.Initialize();
	svWarnings.SetSize(0);
}

void KWDRBuildEntityFromJson::PushJsonPath(const JSONMember* jsonMember, int nIndex) const
{
	require(oaJsonPathMembers.GetSize() == ivJsonPathArrayIndexes.GetSize());
	require(jsonMember != NULL);
	require(nIndex >= 0);
	require(nIndex == 0 or (jsonMember->GetValueType() == JSONValue::ArrayValue and
				nIndex < jsonMember->GetArrayValue()->GetValueNumber()));

	oaJsonPathMembers.Add(cast(Object*, jsonMember));
	ivJsonPathArrayIndexes.Add(nIndex);
}

void KWDRBuildEntityFromJson::PopJsonPath() const
{
	require(oaJsonPathMembers.GetSize() == ivJsonPathArrayIndexes.GetSize());
	require(oaJsonPathMembers.GetSize() > 0);

	oaJsonPathMembers.SetSize(oaJsonPathMembers.GetSize() - 1);
	ivJsonPathArrayIndexes.SetSize(ivJsonPathArrayIndexes.GetSize() - 1);
}

const ALString KWDRBuildEntityFromJson::BuildCurrentJsonPath() const
{
	const int nMaxJsonPathLength = 120;
	boolean bIsOverlengthy;
	ALString sJsonPath;
	const JSONMember* jsonMember;
	int n;

	require(oaJsonPathMembers.GetSize() == ivJsonPathArrayIndexes.GetSize());

	// Construction du json path par analyse de ses composants
	bIsOverlengthy = false;
	sJsonPath = '$';
	for (n = 0; n < oaJsonPathMembers.GetSize(); n++)
	{
		// On ne met pas le chemin intermediaire si le path est trop long
		if (not bIsOverlengthy and sJsonPath.GetLength() > nMaxJsonPathLength and
		    n < oaJsonPathMembers.GetSize() - 1)
		{
			sJsonPath += "...";
			bIsOverlengthy = true;
		}

		// Ajout d'un element dans le path, si le path n'est pas trop long
		// On met la fin du path quoi qu'il arrive
		if (not bIsOverlengthy or n == oaJsonPathMembers.GetSize() - 1)
		{
			jsonMember = cast(const JSONMember*, oaJsonPathMembers.GetAt(n));
			sJsonPath += '.';
			sJsonPath += FormatJsonKey(jsonMember->GetKey());
			if (jsonMember->GetValueType() == JSONValue::ArrayValue)
			{
				sJsonPath += '[';
				sJsonPath += IntToString(ivJsonPathArrayIndexes.GetAt(n));
				sJsonPath += ']';
			}
		}
	}
	return sJsonPath;
}

const ALString KWDRBuildEntityFromJson::FormatJsonKey(const ALString& sJsonKey) const
{
	const int nMaxKeyLength = 30;
	ALString sFormattedKey;
	boolean bIsAscii;
	char c;
	int n;

	require(sJsonKey.GetLength() > 0);

	// Test si la cle est alpha-numerique
	bIsAscii = true;
	for (n = 0; n < sJsonKey.GetLength(); n++)
	{
		c = sJsonKey.GetAt(n);
		if (not isalnum(c) and c != '_')
		{
			bIsAscii = false;
			break;
		}
	}

	// On renvoie la cle telle quelle si elle est ascii et pas trop longue
	if (bIsAscii and sJsonKey.GetLength() <= nMaxKeyLength)
		return sJsonKey;
	// Sinon, on la transforme en la mettant entre double-quotes
	else
	{
		if (sJsonKey.GetLength() <= nMaxKeyLength)
			TextService::CToJsonString(sJsonKey, sFormattedKey);
		else
			TextService::CToJsonString(sJsonKey.Left(nMaxKeyLength) + "...", sFormattedKey);
		sFormattedKey = '"' + sFormattedKey + '"';
		return sFormattedKey;
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

			// Creation et alimentation d'un noeud de la liste
			kwoTargetContainedObject = NewTargetObject(kwoObject, liAttributeLoadIndex);
			if (kwoTargetContainedObject != NULL)
			{
				oaResult.SetAt(nObject, kwoTargetContainedObject);

				// Memorisation de l'attribut Data du noeud courant
				if (livComputeModeTargetAttributeLoadIndexes.GetAt(0).IsValid())
					kwoTargetContainedObject->SetObjectValueAt(
					    livComputeModeTargetAttributeLoadIndexes.GetAt(0),
					    kwoSourceContainedCurrentObject);

				// Chainage avec le noeud precedent
				// Memorisation de l'attribut Prev du noeud courant
				assert(not livComputeModeTargetAttributeLoadIndexes.GetAt(1).IsValid() or
				       kwoTargetContainedObject == NULL or
				       kwoTargetContainedObject->GetObjectValueAt(
					   livComputeModeTargetAttributeLoadIndexes.GetAt(1)) == NULL);
				assert(not livComputeModeTargetAttributeLoadIndexes.GetAt(2).IsValid() or
				       kwoTargetContainedObject == NULL or
				       kwoTargetContainedObject->GetObjectValueAt(
					   livComputeModeTargetAttributeLoadIndexes.GetAt(2)) == NULL);
				if (nObject > 0)
				{
					// Acces au noeud prededent
					kwoPrevTargetContainedObject = cast(KWObject*, oaResult.GetAt(nObject - 1));

					// Chainage si possible
					if (kwoPrevTargetContainedObject != NULL)
					{
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
		}
	}
	return &oaResult;
}

boolean KWDRBuildList::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDRRelationCreationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation
	if (bOk)
	{
		assert(GetOperandNumber() == 1);
		assert(GetOutputOperandNumber() == 3);

		// Verification du type Object chaque operande en sortie
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 0, GetOperandAt(0)->GetObjectClassName()) and
		      bOk;
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 1, GetObjectClassName()) and bOk;
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 2, GetObjectClassName()) and bOk;
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

	// Tous les operandes sont ici obligatoires
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
// Classe KWDRBuildGraph

KWDRBuildGraph::KWDRBuildGraph()
{
	SetName("BuildGraph");
	SetLabel("Build graph from node and edges tables");
	SetType(KWType::Object);

	// Variables en entree, a scope multiple pour aller chercher les identifiants des neoeuds
	SetOperandNumber(5);
	SetMultipleScope(true);

	// Table de noeuds en entree, et leur identifiant
	GetOperandAt(0)->SetType(KWType::ObjectArray);
	GetOperandAt(1)->SetType(KWType::Symbol);

	// Table des arcs en entree, et les identifiants des noeuds extremites des arcs
	GetOperandAt(2)->SetType(KWType::ObjectArray);
	GetOperandAt(3)->SetType(KWType::Symbol);
	GetOperandAt(4)->SetType(KWType::Symbol);

	// Variables en sortie
	SetOutputOperandNumber(7);
	SetMultipleOutputScope(true);

	// Table des noeuds du graphe, avec les donnees d'entree par noeud et la liste des arcs adjacents
	GetOutputOperandAt(0)->SetType(KWType::ObjectArray);
	GetOutputOperandAt(1)->SetType(KWType::Object);
	GetOutputOperandAt(2)->SetType(KWType::ObjectArray);

	// Table des arcs du graphe, avec les donnees d'entree par arc, les les deux noeuds extremites
	GetOutputOperandAt(3)->SetType(KWType::ObjectArray);
	GetOutputOperandAt(4)->SetType(KWType::Object);
	GetOutputOperandAt(5)->SetType(KWType::Object);
	GetOutputOperandAt(6)->SetType(KWType::Object);
}

KWDRBuildGraph::~KWDRBuildGraph() {}

KWDerivationRule* KWDRBuildGraph::Create() const
{
	return new KWDRBuildGraph;
}

KWObject* KWDRBuildGraph::ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const
{
	const KWClass* kwcTargetNodeClass;
	const KWClass* kwcTargetEdgeClass;
	KWLoadIndex liTargetNodes;
	KWLoadIndex liTargetNodeData;
	KWLoadIndex liTargetNodeAdjacentEdges;
	KWLoadIndex liTargetEdges;
	KWLoadIndex liTargetEdgeData;
	KWLoadIndex liTargetEdgeNode1;
	KWLoadIndex liTargetEdgeNode2;
	ObjectArray* oaSourceNodesObjects;
	ObjectArray* oaSourceEdgesObjects;
	KWObject* kwoSourceNodeObject;
	KWObject* kwoSourceEdgeObject;
	KWObject* kwoTargetGraph;
	KWObject* kwoTargetNodeObject;
	KWObject* kwoTargetEdgeObject;
	KWObject* kwoTargetNodeObject1;
	KWObject* kwoTargetNodeObject2;
	ObjectArray* oaTargetNodesObjects;
	ObjectArray* oaTargetEdgesObjects;
	ObjectArray* oaTargetNodeAdjacentEdgesObjects;
	Symbol sNodeId;
	Symbol sNodeId1;
	Symbol sNodeId2;
	int nNode;
	int nEdge;
	NumericKeyDictionary nkdTargetNodes;
	SymbolVector sNodeIds;
	int nDuplicateNodeNumber;
	int nIncompleteEdgeNumber;
	ALString sSampleDuplicateNodeId;
	ALString sSampleIncompleteEdsgeId;
	KWAttribute* attribute;
	ALString sMessage;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche des noeuds et des arcs
	oaSourceNodesObjects = GetOperandAt(0)->GetObjectArrayValue(kwoObject);
	oaSourceEdgesObjects = GetOperandAt(2)->GetObjectArrayValue(kwoObject);

	// Creation du graphe en sortie, responsable de la destruction de ses noeuds et arcs natifs
	kwoTargetGraph = NewTargetOwnerObject(kwoObject, liAttributeLoadIndex);

	// Alimentation du graphe
	nDuplicateNodeNumber = 0;
	nIncompleteEdgeNumber = 0;
	if (kwoTargetGraph != NULL)
	{
		// Recherche de la classe de creation des noeuds et des arcs
		kwcTargetNodeClass =
		    kwcCompiledTargetClass->GetDomain()->LookupClass(GetOutputOperandAt(0)->GetObjectClassName());
		kwcTargetEdgeClass =
		    kwcCompiledTargetClass->GetDomain()->LookupClass(GetOutputOperandAt(3)->GetObjectClassName());

		// Recherche des index des attributs cibles
		liTargetNodes = livComputeModeTargetAttributeLoadIndexes.GetAt(0);
		liTargetNodeData = livComputeModeTargetAttributeLoadIndexes.GetAt(1);
		liTargetNodeAdjacentEdges = livComputeModeTargetAttributeLoadIndexes.GetAt(2);
		liTargetEdges = livComputeModeTargetAttributeLoadIndexes.GetAt(3);
		liTargetEdgeData = livComputeModeTargetAttributeLoadIndexes.GetAt(4);
		liTargetEdgeNode1 = livComputeModeTargetAttributeLoadIndexes.GetAt(5);
		liTargetEdgeNode2 = livComputeModeTargetAttributeLoadIndexes.GetAt(6);

		// Creation des noeuds
		oaTargetNodesObjects = NULL;
		if (liTargetNodes.IsValid())
		{
			if (oaSourceNodesObjects != NULL and oaSourceNodesObjects->GetSize() > 0)
			{
				// Creation et memorisation du tableau de noeuds
				oaTargetNodesObjects = new ObjectArray;
				kwoTargetGraph->SetObjectArrayValueAt(liTargetNodes, oaTargetNodesObjects);

				// Creation des noeuds
				for (nNode = 0; nNode < oaSourceNodesObjects->GetSize(); nNode++)
				{
					kwoSourceNodeObject = cast(KWObject*, oaSourceNodesObjects->GetAt(nNode));

					// Ajout dans le tableau si noeud non existant
					sNodeId = GetOperandAt(1)->GetSymbolValue(kwoSourceNodeObject);
					if (nkdTargetNodes.Lookup(sNodeId.GetNumericKey()) == NULL)
					{
						// Creation du noeud
						kwoTargetNodeObject =
						    NewObject(kwoTargetGraph, liTargetNodes, kwcTargetNodeClass, true);

						// Initialisation du noeud
						if (kwoTargetNodeObject != NULL)
						{
							// Memorisation du noeud
							oaTargetNodesObjects->Add(kwoTargetNodeObject);
							nkdTargetNodes.SetAt(sNodeId.GetNumericKey(),
									     kwoTargetNodeObject);

							// On memorise l'identifiant du noeud pour garantir que le meme Symbol
							// (et son NumericKey) est utilise pour deux noeuds dde meme identifiant,
							// meme s'il sont calcules de facon temporaire
							sNodeIds.Add(sNodeId);

							// Memorisation des donnees du noeud
							if (liTargetNodeData.IsValid())
								kwoTargetNodeObject->SetObjectValueAt(
								    liTargetNodeData, kwoSourceNodeObject);

							// Creation du tableau des arcs adjacents
							if (liTargetNodeAdjacentEdges.IsValid())
								kwoTargetNodeObject->SetObjectArrayValueAt(
								    liTargetNodeAdjacentEdges, new ObjectArray);
						}
					}
					// Erreur si noeud existant
					else
					{
						nDuplicateNodeNumber++;
						if (sSampleDuplicateNodeId == "")
						{
							sSampleDuplicateNodeId =
							    "Node(" + BuildLabelFromId(sNodeId.GetValue()) + ")";
						}
					}
				}
			}
		}

		// Creation des arcs
		if (liTargetEdges.IsValid())
		{
			if (oaSourceEdgesObjects != NULL and oaSourceEdgesObjects->GetSize() > 0)
			{
				// Creation et memorisation du tableau d'arcs
				oaTargetEdgesObjects = new ObjectArray;
				kwoTargetGraph->SetObjectArrayValueAt(liTargetEdges, oaTargetEdgesObjects);

				// Creation des arcs
				for (nEdge = 0; nEdge < oaSourceEdgesObjects->GetSize(); nEdge++)
				{
					kwoSourceEdgeObject = cast(KWObject*, oaSourceEdgesObjects->GetAt(nEdge));

					// Ajout dans le tableau si noeuds extremites existants
					sNodeId1 = GetOperandAt(3)->GetSymbolValue(kwoSourceEdgeObject);
					sNodeId2 = GetOperandAt(4)->GetSymbolValue(kwoSourceEdgeObject);
					kwoTargetNodeObject1 =
					    cast(KWObject*, nkdTargetNodes.Lookup(sNodeId1.GetNumericKey()));
					kwoTargetNodeObject2 =
					    cast(KWObject*, nkdTargetNodes.Lookup(sNodeId2.GetNumericKey()));
					if (kwoTargetNodeObject1 != NULL and kwoTargetNodeObject2 != NULL)
					{
						// Creation et memorisation de l'arc
						kwoTargetEdgeObject =
						    NewObject(kwoTargetGraph, liTargetEdges, kwcTargetEdgeClass, true);

						// Initialisation de l'arc
						if (kwoTargetEdgeObject != NULL)
						{
							// Memorisation de l'arc
							oaTargetEdgesObjects->Add(kwoTargetEdgeObject);

							// Memorisation des donnees de l'arc
							if (liTargetEdgeData.IsValid())
								kwoTargetEdgeObject->SetObjectValueAt(
								    liTargetEdgeData, kwoSourceEdgeObject);

							// Memorisation de ses noeuds extremites
							if (liTargetEdgeNode1.IsValid())
								kwoTargetEdgeObject->SetObjectValueAt(
								    liTargetEdgeNode1, kwoTargetNodeObject1);
							if (liTargetEdgeNode2.IsValid())
								kwoTargetEdgeObject->SetObjectValueAt(
								    liTargetEdgeNode2, kwoTargetNodeObject2);

							// Ajout dans la liste d'adjacents des noeuds extremites
							if (liTargetNodeAdjacentEdges.IsValid())
							{
								// Pour le noeuds source
								oaTargetNodeAdjacentEdgesObjects =
								    kwoTargetNodeObject1->GetObjectArrayValueAt(
									liTargetNodeAdjacentEdges);
								oaTargetNodeAdjacentEdgesObjects->Add(
								    kwoTargetEdgeObject);

								// Pour le noeuds cible, s'il est different
								// On n'enregistre l'arc deux fois en cas de self-loop
								if (kwoTargetNodeObject2 != kwoTargetNodeObject1)
								{
									oaTargetNodeAdjacentEdgesObjects =
									    kwoTargetNodeObject2->GetObjectArrayValueAt(
										liTargetNodeAdjacentEdges);
									oaTargetNodeAdjacentEdgesObjects->Add(
									    kwoTargetEdgeObject);
								}
							}
						}
					}
					// Erreur si noeud manquant pour une des deux extremites
					else
					{
						nIncompleteEdgeNumber++;
						if (sSampleIncompleteEdsgeId == "")
						{
							sSampleIncompleteEdsgeId =
							    "Edge(" + BuildLabelFromId(sNodeId1.GetValue()) + ", " +
							    BuildLabelFromId(sNodeId2.GetValue()) + ")";
						}
					}
				}
			}
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();

	// Message d'erreur
	if (nDuplicateNodeNumber > 0 or nIncompleteEdgeNumber > 0)
	{
		// Pas de message en cas de probleme memoire
		if (not kwoObject->IsMemoryLimitReached())
		{
			attribute = kwoObject->GetClass()->GetAttributeAtLoadIndex(liAttributeLoadIndex);
			assert(attribute != NULL);

			// Message synthetique
			sMessage = "Calculation of the variable ";
			sMessage += attribute->GetName();
			sMessage += " using the rule ";
			sMessage += GetName();
			sMessage += " encountered inconsistencies, including ";
			if (nDuplicateNodeNumber > 0)
			{
				sMessage += IntToString(nDuplicateNodeNumber);
				sMessage += " duplicate node";
				if (nDuplicateNodeNumber > 1)
					sMessage += "s";
				sMessage += " (e.g., ";
				sMessage += sSampleDuplicateNodeId;
				sMessage += ")";
			}
			if (nDuplicateNodeNumber > 0 and nIncompleteEdgeNumber > 0)
				sMessage += " and ";
			if (nIncompleteEdgeNumber > 0)
			{
				sMessage += IntToString(nIncompleteEdgeNumber);
				sMessage += " incomplete edge";
				if (nIncompleteEdgeNumber > 1)
					sMessage += "s";
				sMessage += " with missing extremity nodes (e.g., ";
				sMessage += sSampleIncompleteEdsgeId;
				sMessage += ")";
			}
			kwoObject->AddWarning(sMessage);
		}
	}

	// On retourne le graphe cree
	return kwoTargetGraph;
}

boolean KWDRBuildGraph::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	ALString sNodeType;
	ALString sEdgeType;

	// Appel de la methode ancetre
	bOk = KWDRRelationCreationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// On verifie que les type des noeuds et des arcs en sortie sont differents, meme en cas d'erreur,
	// pour avoir un diganostique intelligible minimaliste
	if (GetOutputOperandNumber() == 7 and GetOutputOperandAt(0)->GetType() == KWType::ObjectArray and
	    GetOutputOperandAt(3)->GetType() == KWType::ObjectArray)
	{
		// Type des noeuds et des arcs
		sNodeType = GetOutputOperandAt(0)->GetObjectClassName();
		sEdgeType = GetOutputOperandAt(3)->GetObjectClassName();

		// Les types des noeuds et des arcs en sortie doivent etre differents
		if (sNodeType == sEdgeType)
		{
			AddError("In the " + GetObjectClassName() + " output dictionary, the " +
				 KWType::ToString(KWType::Object) + "(" + sNodeType + ") " +
				 GetOutputOperandAt(0)->GetDataItemName() +
				 " node variable related to the first output operand and the " +
				 KWType::ToString(KWType::Object) + "(" + sEdgeType +
				 ") edge variable related to the third output operand should be of different types");
			bOk = false;
		}
	}

	// Specialisation
	if (bOk)
	{
		assert(GetOperandNumber() == 5);
		assert(GetOutputOperandNumber() == 7);

		// Verification des type Data des operandes en sortie
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 1, GetOperandAt(0)->GetObjectClassName()) and
		      bOk;
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 4, GetOperandAt(2)->GetObjectClassName()) and
		      bOk;

		// Verification de la coherence des liens entre noeuds et arcs
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 2, sEdgeType) and bOk;
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 5, sNodeType) and bOk;
		bOk = CheckOutputOperandExpectedObjectType(kwcOwnerClass, 6, sNodeType) and bOk;
	}
	return bOk;
}

boolean KWDRBuildGraph::IsNewScopeOperand(int nOperandIndex) const
{
	require(0 <= nOperandIndex and nOperandIndex < GetOperandNumber());
	return nOperandIndex == 0 or nOperandIndex == 2;
}

boolean KWDRBuildGraph::IsSecondaryScopeOperand(int nOperandIndex) const
{
	require(0 <= nOperandIndex and nOperandIndex < GetOperandNumber());
	return nOperandIndex == 1 or nOperandIndex > 2;
}

boolean KWDRBuildGraph::IsNewOutputScopeOperand(int nOutputOperandIndex) const
{
	require(0 <= nOutputOperandIndex and nOutputOperandIndex < GetOutputOperandNumber());
	return nOutputOperandIndex == 0 or nOutputOperandIndex == 3;
}

boolean KWDRBuildGraph::IsSecondaryOutputScopeOperand(int nOutputOperandIndex) const
{
	require(0 <= nOutputOperandIndex and nOutputOperandIndex < GetOutputOperandNumber());
	return nOutputOperandIndex == 1 or nOutputOperandIndex == 2 or nOutputOperandIndex > 3;
}

const ALString KWDRBuildGraph::BuildLabelFromId(const ALString& sId) const
{
	const int nLabelMaxLength = 20;
	ALString sLabel;

	if (sId.GetLength() <= nLabelMaxLength)
		sLabel = sId;
	else
		sLabel = sId.Left(nLabelMaxLength) + "...";
	return sLabel;
}

boolean KWDRBuildGraph::CheckOperandCompletenessAt(const KWClass* kwcOwnerClass, int nIndex) const
{
	boolean bOk = true;
	ALString sExpectedObjectClassName;
	KWDerivationRuleOperand* operand;
	const KWClass* kwcScopeClass;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());

	// Acces a l'operande
	operand = GetOperandAt(nIndex);

	// Le premier et le troisieme operande (neoeuds et arcs) sont verifies de facon standard
	if (nIndex == 0 or nIndex == 2)
	{
		if (not operand->CheckCompleteness(kwcOwnerClass))
		{
			AddError(sTmp + "Incomplete operand " + IntToString(nIndex + 1));
			bOk = false;
		}
	}
	// Le deuxieme operande (identifiant) est a verifier au niveau du scope des noeuds
	else if (nIndex == 1)
	{
		// Recherche du dictionnaire des noeuds du premier operande
		kwcScopeClass = kwcOwnerClass->GetDomain()->LookupClass(GetOperandAt(0)->GetObjectClassName());

		// Verification de l'operande si possible
		if (kwcScopeClass != NULL)
		{
			if (not operand->CheckCompleteness(kwcOwnerClass))
			{
				AddError(sTmp + "Incomplete operand " + IntToString(nIndex + 1) +
					 " for identifier in node dictionary " + kwcScopeClass->GetName());
				bOk = false;
			}
		}
	}
	// Les deux derniers operandes (identfiant des extremites des arcs) sont a verifier au niveau du scope des arcs
	else
	{
		// Recherche du dictionnaire des noeuds du premier operande
		kwcScopeClass = kwcOwnerClass->GetDomain()->LookupClass(GetOperandAt(0)->GetObjectClassName());

		// Verification de l'operande si possible
		if (kwcScopeClass != NULL)
		{
			if (not operand->CheckCompleteness(kwcOwnerClass))
			{
				AddError(sTmp + "Incomplete operand " + IntToString(nIndex + 1) +
					 " for identifier in edge dictionary " + kwcScopeClass->GetName());
				bOk = false;
			}
		}
	}
	return bOk;
}

boolean KWDRBuildGraph::IsViewModeActivated() const
{
	return false;
}

void KWDRBuildGraph::FinalizeCollectUsedOutputOperands(IntVector* ivUsedOutputOperands) const
{
	require(ivUsedOutputOperands != NULL);
	require(ivUsedOutputOperands->GetSize() == GetOutputOperandNumber());

	// Les arcs doivent etre calcules si on utilise les arcs adjacents des noeuds, ou les donnees des arcs
	if (ivUsedOutputOperands->GetAt(2) == 1 or ivUsedOutputOperands->GetAt(4) == 1)
		ivUsedOutputOperands->SetAt(3, 1);

	// Les noeuds doivent etre calcules les arcs le sont, ou si on utilie les noeuds extremites des arcs, ou les donnees des noeuds
	if (ivUsedOutputOperands->GetAt(3) == 1 or ivUsedOutputOperands->GetAt(5) == 1 or
	    ivUsedOutputOperands->GetAt(6) == 1 or ivUsedOutputOperands->GetAt(1) == 1)
		ivUsedOutputOperands->SetAt(0, 1);
}

void KWDRBuildGraph::CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const
{
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Aucun operande en entree n'est a priori obbligatoire
}

void KWDRBuildGraph::CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const
{
	require(0 <= nOutputOperand and nOutputOperand < GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Cas de noeuds utilises en sortie, directement, pour verifier l'existence des extremites des arcs, ou pour les extremites des arcs
	if (nOutputOperand == 0 or nOutputOperand == 1 or nOutputOperand == 3 or nOutputOperand == 5 or
	    nOutputOperand == 6)
	{
		ivUsedInputOperands->SetAt(0, 1);
		ivUsedInputOperands->SetAt(1, 1);
	}

	// Cas des arcs utilises en sortie, directement, ou pour les arcs adjacents des noeuds
	if (nOutputOperand == 3 or nOutputOperand == 4 or nOutputOperand == 2)
	{
		ivUsedInputOperands->SetAt(2, 1);
		ivUsedInputOperands->SetAt(3, 1);
		ivUsedInputOperands->SetAt(4, 1);
	}
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
		if (kwoTargetContainedObject != NULL)
		{
			FillComputeModeTargetAttributesForVariableOperandNumber(kwoObject, kwoTargetContainedObject);
			oaResult.SetAt(nObject, kwoTargetContainedObject);
		}
	}
	return &oaResult;
}

boolean KWDRBuildDummyTable::IsViewModeActivated() const
{
	return false;
}
