// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMMajorityClassifier.h"

CMMajorityClassifier::CMMajorityClassifier() {}

CMMajorityClassifier::~CMMajorityClassifier() {}

boolean CMMajorityClassifier::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol;
}

KWPredictor* CMMajorityClassifier::Create() const
{
	return new CMMajorityClassifier;
}

const ALString CMMajorityClassifier::GetPrefix() const
{
	return "MAJ";
}

const ALString CMMajorityClassifier::GetName() const
{
	return "Majority";
}

boolean CMMajorityClassifier::InternalTrain()
{
	KWDataPreparationClass dataPreparationClass;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetClassStats()->IsStatsComputed());
	require(GetTargetAttributeType() == KWType::Symbol or GetTargetAttributeType() == KWType::Continuous);
	require(IsTraining());

	// Apprentissage si au moins une valeur cible
	if (GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		// Parametrage de la preparation de donnees
		dataPreparationClass.SetLearningSpec(GetLearningSpec());

		// Generation de la classe de preparation des donnees
		dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

		// Construction d'un predicteur bayesien naif a partir de tous les attributs
		InternalTrainMC1(&dataPreparationClass, dataPreparationClass.GetDataPreparationAttributes());
		return true;
	}
	return false;
}

void CMMajorityClassifier::InternalTrainMC1(KWDataPreparationClass* dataPreparationClass,
					    ObjectArray* oaDataPreparationUsedAttributes)
{
	ObjectArray oaFilteredDataPreparationAttributes;
	ContinuousVector cvWorkingWeights;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;

	require(dataPreparationClass != NULL);
	require(dataPreparationClass->CheckDataPreparation());
	require(oaDataPreparationUsedAttributes != NULL);
	require(GetTargetDescriptiveStats()->GetValueNumber() > 0);
	require(GetPredictorReport() != NULL);
	require(GetTrainedPredictor() != NULL);
	require(GetTrainedPredictor()->GetPredictorClass() == NULL);

	// Filtrage des attributs et poids effectivement utilises
	for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes->GetAt(nAttribute));

		// Test si attribut utile
		if (dataPreparationAttribute->GetPreparedStats()
			->GetPreparedDataGridStats()
			->ComputeInformativeAttributeNumber() > 0)
		{
			oaFilteredDataPreparationAttributes.Add(dataPreparationAttribute);
		}
	}

	// Initialisation de la classe du predicteur
	GetTrainedPredictor()->SetPredictorClass(dataPreparationClass->GetDataPreparationClass(),
						 GetTargetAttributeType(), GetName());

	// Construction des attributs de prediction selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
	{
		InternalTrainMC2(GetTrainedClassifier(), dataPreparationClass, &oaFilteredDataPreparationAttributes);
	}

	// Nettoyage de la preparation, la classe de preparation etant maintenant reference par le predicteur
	dataPreparationClass->RemoveDataPreparation();
}

void CMMajorityClassifier::InternalTrainMC2(KWTrainedClassifier* trainedClassifier,
					    KWDataPreparationClass* dataPreparationClass,
					    ObjectArray* oaDataPreparationUsedAttributes)
{
	KWAttribute* classifierAttribute;
	KWAttribute* targetValuesAttribute;

	require(trainedClassifier != NULL);
	require(trainedClassifier->GetPredictorClass() != NULL);
	require(dataPreparationClass != NULL);
	require(oaDataPreparationUsedAttributes != NULL);

	// Memorisation de la reference a l'attribut cible
	trainedClassifier->SetTargetAttribute(
	    trainedClassifier->GetPredictorClass()->LookupAttribute(GetTargetAttributeName()));

	// Recherche de l'attribut memorisant les valeurs cibles
	targetValuesAttribute = dataPreparationClass->GetDataPreparationTargetAttribute()->GetPreparedAttribute();
	trainedClassifier->SetTargetValuesAttribute(targetValuesAttribute);

	// Ajout de l'attribut classifieur
	classifierAttribute = AddClassifierAttribute(trainedClassifier, oaDataPreparationUsedAttributes);

	// Ajout des attributs de prediction pour la classification
	AddClassifierPredictionAttributes(trainedClassifier, classifierAttribute);

	// Completion automatique des informations de la classe (nom de classe par regle...)
	trainedClassifier->GetPredictorClass()->CompleteTypeInfo();
}

KWAttribute* CMMajorityClassifier::AddClassifierAttribute(KWTrainedClassifier* trainedClassifier,
							  ObjectArray* oaDataPreparationUsedAttributes)
{
	boolean bTrace = false;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDerivationRuleOperand* operand;
	KWDRDataGridStats* dgsRule;
	KWDRNBClassifier* classifierRule;
	KWAttribute* classifierAttribute;

	require(trainedClassifier != NULL);
	require(oaDataPreparationUsedAttributes != NULL);

	// Creation d'une regle de classification majoritaire
	classifierRule = new CMDRMajorityClassifier;
	classifierRule->DeleteAllVariableOperands();

	// Ajout d'un attribut de type grille de donnees par attribut prepare
	for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes->GetAt(nAttribute));

		// Affichage des statistiques de preparation
		if (bTrace)
		{
			cout << dataPreparationAttribute->GetObjectLabel() << endl;
			cout << *dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() << endl;
		}

		// Creation d'une regle DataGridStats
		dgsRule = dataPreparationAttribute->CreatePreparedStatsRule();

		// Ajout d'un operande DataGridStats au predicteur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dgsRule);
		operand->SetType(dgsRule->GetType());
		operand->SetStructureName(dgsRule->GetStructureName());
		classifierRule->AddOperand(operand);
	}

	// Ajout d'un operande pour les valeurs cible
	operand = new KWDerivationRuleOperand;
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetAttributeName(trainedClassifier->GetTargetValuesAttribute()->GetName());
	classifierRule->AddOperand(operand);

	// Creation d'un attribut de classification
	classifierAttribute =
	    trainedClassifier->CreatePredictionAttribute(GetPrefix() + GetTargetAttributeName(), classifierRule);
	return classifierAttribute;
}
