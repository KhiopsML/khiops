// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBAttributeSelectionScorer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBAttributeSelectionScorer
SNBAttributeSelectionScorer::SNBAttributeSelectionScorer()
{
	dSelectionModelAllAttributeCost = 0.0;
	binarySliceSet = NULL;
	dataCostCalculator = NULL;
	bIsDataCostCalculatorCreated = false;
	bIsDataCostCalculatorInitialized = false;
	bIsConstructionCostEnabled = true;
	bIsPreparationCostEnabled = true;

	// Le poids par defaut du prior a 0.25 releve d'une etude empirique
	dPriorWeight = 0.25;
}

SNBAttributeSelectionScorer::~SNBAttributeSelectionScorer()
{
	CleanDataCostCalculator();
}

void SNBAttributeSelectionScorer::SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* dataTableBinarySliceSet)
{
	require(dataTableBinarySliceSet != NULL);
	CleanWorkingData();
	binarySliceSet = dataTableBinarySliceSet;
}

SNBDataTableBinarySliceSet* SNBAttributeSelectionScorer::GetDataTableBinarySliceSet() const
{
	return binarySliceSet;
}

void SNBAttributeSelectionScorer::SetPriorWeight(double dValue)
{
	dPriorWeight = dValue;
}

double SNBAttributeSelectionScorer::GetPriorWeight() const
{
	return dPriorWeight;
}

void SNBAttributeSelectionScorer::SetConstructionCostEnabled(boolean bValue)
{
	bIsConstructionCostEnabled = bValue;
}

boolean SNBAttributeSelectionScorer::IsConstructionCostEnabled() const
{
	return bIsConstructionCostEnabled;
}

void SNBAttributeSelectionScorer::SetPreparationCostEnabled(boolean bValue)
{
	bIsPreparationCostEnabled = bValue;
}

boolean SNBAttributeSelectionScorer::IsPreparationCostEnabled() const
{
	return bIsPreparationCostEnabled;
}

void SNBAttributeSelectionScorer::InitializeWorkingData()
{
	dSelectionModelAllAttributeCost = 0.0;
	if (IsDataCostCalculatorCreated())
		InitializeDataCostCalculator();
}

void SNBAttributeSelectionScorer::CleanWorkingData()
{
	if (IsDataCostCalculatorCreated())
		CleanDataCostCalculator();
}

boolean SNBAttributeSelectionScorer::CreateDataCostCalculator()
{
	require(Check());

	// Nettoyage au prealable
	CleanDataCostCalculator();

	// Creation de l'instance de la calculatrice en fonction de la cible; nettoyage si echec
	if (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped())
		dataCostCalculator = new SNBGeneralizedClassifierSelectionDataCostCalculator;
	else if (GetTargetAttributeType() == KWType::Symbol)
		dataCostCalculator = new SNBClassifierSelectionDataCostCalculator;
	else if (GetTargetAttributeType() == KWType::Continuous)
		dataCostCalculator = new SNBRegressorSelectionDataCostCalculator;
	else
	{
		CleanDataCostCalculator();
		AddError("Target variable type must be either Categorical or Numerical");
	}

	// Finalisation de la creation de la calculatrice; nettoyage si echec
	if (dataCostCalculator != NULL)
	{
		dataCostCalculator->SetLearningSpec(GetLearningSpec());
		dataCostCalculator->SetDataTableBinarySliceSet(GetDataTableBinarySliceSet());
		bIsDataCostCalculatorCreated = dataCostCalculator->Create();
	}
	if (not bIsDataCostCalculatorCreated)
	{
		CleanDataCostCalculator();
		AddError("Unable to create working data for a preparation step for variable selection");
	}

	ensure(Check());
	return bIsDataCostCalculatorCreated;
}

boolean SNBAttributeSelectionScorer::IsDataCostCalculatorCreated() const
{
	return bIsDataCostCalculatorCreated;
}

void SNBAttributeSelectionScorer::InitializeDataCostCalculator()
{
	require(IsDataCostCalculatorCreated());
	require(Check());

	dataCostCalculator->Initialize();
	bIsDataCostCalculatorInitialized = true;

	ensure(IsDataCostCalculatorInitialized());
}

void SNBAttributeSelectionScorer::CleanDataCostCalculator()
{
	if (dataCostCalculator != NULL)
	{
		delete dataCostCalculator;
		dataCostCalculator = NULL;
	}
	bIsDataCostCalculatorCreated = false;
	bIsDataCostCalculatorInitialized = false;
}

double SNBAttributeSelectionScorer::ComputeSelectionScore()
{
	require(IsDataCostCalculatorInitialized());
	return ComputeSelectionModelCost() + ComputeSelectionDataCost();
}

double SNBAttributeSelectionScorer::ComputeSelectionDataCost()
{
	require(IsDataCostCalculatorInitialized());
	return dataCostCalculator->ComputeSelectionDataCost();
}

double SNBAttributeSelectionScorer::ComputeSelectionModelCost() const
{
	return ComputeSelectionModelAttributeWeightCost() + dSelectionModelAllAttributeCost;
}

double
SNBAttributeSelectionScorer::ComputeSelectionModelAttributeCost(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	double dCost;

	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);

	// Cout de selection et de construction (sans cout de modele nul)
	dCost = 0.0;
	if (IsConstructionCostEnabled() and attribute->GetConstructionCost() > 0)
	{
		dCost += attribute->GetConstructionCost();
		dCost -= attribute->GetNullConstructionCost();
	}
	// Si pas de cout de construction, on se rabat sur un cout de selection de variables
	else
		dCost += log(GetDataTableBinarySliceSet()->GetInitialAttributeNumber() * 1.0);

	// Cout de preparation
	if (IsPreparationCostEnabled())
		dCost += attribute->GetPreparationCost();

	// Prise en compte du poids du prior
	dCost *= GetPriorWeight();

	return dCost;
}

boolean SNBAttributeSelectionScorer::IsDataCostCalculatorInitialized() const
{
	return bIsDataCostCalculatorInitialized;
}

boolean SNBAttributeSelectionScorer::Check() const
{
	boolean bOk = true;

	// Validation des specifications
	bOk = KWLearningService::Check();
	bOk = bOk and learningSpec->IsTargetStatsComputed();

	// Verification de la compatibilite avec la base d'apprentissage
	bOk = bOk and binarySliceSet != NULL;

	// Validation de l'etat de la calculatrice de cout de donnees
	if (bIsDataCostCalculatorCreated)
		bOk = dataCostCalculator->Check();

	if (bIsDataCostCalculatorInitialized)
		bOk = bOk and bIsDataCostCalculatorCreated;

	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBHardAttributeSelection
SNBHardAttributeSelection::SNBHardAttributeSelection() {}

SNBHardAttributeSelection::~SNBHardAttributeSelection() {}

int SNBHardAttributeSelection::GetAttributeNumber() const
{
	return nkdAttributeSelectionSet.GetCount();
}

void SNBHardAttributeSelection::AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(not Contains(attribute));
	nkdAttributeSelectionSet.SetAt((NUMERIC)attribute, attribute);
}

void SNBHardAttributeSelection::RemoveAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(Contains(attribute));
	nkdAttributeSelectionSet.RemoveKey((NUMERIC)attribute);
}

void SNBHardAttributeSelection::RemoveAllAttributes()
{
	nkdAttributeSelectionSet.RemoveAll();
}

boolean SNBHardAttributeSelection::Contains(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	return nkdAttributeSelectionSet.Lookup((NUMERIC)attribute) != NULL;
}

SNBHardAttributeSelection* SNBHardAttributeSelection::Clone() const
{
	SNBHardAttributeSelection* attributeSelectionClone;

	attributeSelectionClone = new SNBHardAttributeSelection;
	attributeSelectionClone->nkdAttributeSelectionSet.CopyFrom(&nkdAttributeSelectionSet);

	return attributeSelectionClone;
}

ObjectArray* SNBHardAttributeSelection::CollectAttributes() const
{
	ObjectArray* oaSelectedAttributes;

	oaSelectedAttributes = new ObjectArray;
	nkdAttributeSelectionSet.ExportObjectArray(oaSelectedAttributes);

	return oaSelectedAttributes;
}

void SNBHardAttributeSelection::Write(ostream& ost) const
{
	ObjectArray oaAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	nkdAttributeSelectionSet.ExportObjectArray(&oaAttributes);

	ost << "Attribute selection: ";
	if (oaAttributes.GetSize() == 0)
		ost << "<Empty>\n";
	else
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(0));
		ost << attribute->GetNativeAttributeName();
		for (nAttribute = 1; nAttribute < oaAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nAttribute));
			ost << ", " << attribute->GetNativeAttributeName();
		}
		ost << "\n";
	}
}

longint SNBHardAttributeSelection::ComputeNecessaryMemory(int nAttributes)
{
	NumericKeyDictionary nkdDummy;

	// Formule de l'estimation :
	//   Objets directes de l'instance +
	//   Ensemble des pointeurs de la selection (nkdAttributeSelectionSet)
	return sizeof(SNBHardAttributeSelection) + nAttributes * nkdDummy.GetUsedMemoryPerElement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBHardAttributeSelectionScorer

SNBHardAttributeSelectionScorer::SNBHardAttributeSelectionScorer()
{
	lastModificationAttribute = NULL;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	bWasLastModificationAdd = false;
}

SNBHardAttributeSelectionScorer::~SNBHardAttributeSelectionScorer()
{
	CleanWorkingData();
}

void SNBHardAttributeSelectionScorer::InitializeWorkingData()
{
	require(Check());

	attributeSelection.RemoveAllAttributes();
	lastModificationAttribute = NULL;
	bWasLastModificationAdd = false;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	SNBAttributeSelectionScorer::InitializeWorkingData();
}

void SNBHardAttributeSelectionScorer::CleanWorkingData()
{
	attributeSelection.RemoveAllAttributes();
	lastModificationAttribute = NULL;
	bWasLastModificationAdd = false;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	SNBAttributeSelectionScorer::CleanWorkingData();
}

boolean SNBHardAttributeSelectionScorer::AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	boolean bOk = true;
	require(Check());
	require(attribute != NULL);
	require(not attributeSelection.Contains(attribute));

	// Memorisation des infos de l'ajout pour une eventuelle annulation
	lastModificationAttribute = attribute;
	dLastModificationSelectionModelAllAttributeCost = dSelectionModelAllAttributeCost;
	bWasLastModificationAdd = true;

	// Ajout d'un attribut dans la selection
	attributeSelection.AddAttribute(attribute);

	// Memorisation de l'ajout de son cout unitaire
	dSelectionModelAllAttributeCost += ComputeSelectionModelAttributeCost(attribute);

	// Mise a jour de la calculatrice de cout de donnees
	// La maj de la partition cible doit se faire *avant* celle des vecteurs de scores
	if (IsDataCostCalculatorInitialized())
	{
		dataCostCalculator->UpdateTargetPartitionWithAddedAttribute(attribute);
		bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(attribute, 1.0);
	}

	// Trace de deboggage
	if (IsDataCostCalculatorInitialized() and dataCostCalculator->GetDisplay())
	{
		cout << "Adding\t" << attribute->GetNativeAttributeName() << "\n";
		dataCostCalculator->Write(cout);
		cout << "\n";
	}
	ensure(Check());
	ensure(attributeSelection.Contains(attribute));
	return bOk;
}

boolean SNBHardAttributeSelectionScorer::RemoveAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	boolean bOk = true;
	require(Check());
	require(attribute != NULL);
	require(attributeSelection.Contains(attribute));

	// Memorisation des infos de l'enlevement pour une eventuelle annulation
	lastModificationAttribute = attribute;
	dLastModificationSelectionModelAllAttributeCost = dSelectionModelAllAttributeCost;
	bWasLastModificationAdd = false;

	// Supression de l'attribut de la selection
	attributeSelection.RemoveAttribute(attribute);

	// Memorisation de la supression de son cout unitaire
	dSelectionModelAllAttributeCost -= ComputeSelectionModelAttributeCost(attribute);

	// Mise a jour de la calculatrice de cout de donnees
	// La mise a jour de la partition cible doit se faire *apres* celle des vecteurs de scores
	if (IsDataCostCalculatorInitialized())
	{
		bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(attribute, -1.0);
		dataCostCalculator->UpdateTargetPartitionWithRemovedAttribute(attribute);
	}

	// Trace de deboggage
	if (IsDataCostCalculatorInitialized() and dataCostCalculator->GetDisplay())
	{
		cout << "Removing\t" << attribute->GetNativeAttributeName() << "\n";
		dataCostCalculator->Write(cout);
		cout << "\n";
	}
	ensure(Check());
	ensure(not attributeSelection.Contains(attribute));
	return bOk;
}

boolean SNBHardAttributeSelectionScorer::UndoLastModification()
{
	boolean bOk = true;
	require(Check());
	require(IsUndoAllowed());

	dSelectionModelAllAttributeCost = dLastModificationSelectionModelAllAttributeCost;
	if (bWasLastModificationAdd)
	{
		attributeSelection.RemoveAttribute(lastModificationAttribute);
		if (IsDataCostCalculatorInitialized())
		{
			bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(
					  lastModificationAttribute, -1.0);
			dataCostCalculator->UpdateTargetPartitionWithRemovedAttribute(lastModificationAttribute);
		}
	}
	else
	{
		attributeSelection.AddAttribute(lastModificationAttribute);
		if (IsDataCostCalculatorInitialized())
		{
			dataCostCalculator->UpdateTargetPartitionWithAddedAttribute(lastModificationAttribute);
			bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(
					  lastModificationAttribute, 1.0);
		}
	}
	lastModificationAttribute = NULL;
	dLastModificationSelectionModelAllAttributeCost = 0.0;

	require(Check());
	ensure(not IsUndoAllowed());
	return bOk;
}

boolean SNBHardAttributeSelectionScorer::IsUndoAllowed()
{
	return lastModificationAttribute != NULL;
}

const SNBHardAttributeSelection* SNBHardAttributeSelectionScorer::GetAttributeSelection() const
{
	return &attributeSelection;
}

SNBHardAttributeSelection* SNBHardAttributeSelectionScorer::CollectAttributeSelection() const
{
	return attributeSelection.Clone();
}

boolean SNBHardAttributeSelectionScorer::Check() const
{
	boolean bOk = true;
	ObjectArray* oaAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	// Appel a la methode ancetre
	bOk = SNBAttributeSelectionScorer::Check();

	// Verification de la selection
	bOk = bOk and 0 <= attributeSelection.GetAttributeNumber();
	bOk = bOk and attributeSelection.GetAttributeNumber() <= binarySliceSet->GetAttributeNumber();
	oaAttributes = attributeSelection.CollectAttributes();
	for (nAttribute = 0; nAttribute < attributeSelection.GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes->GetAt(nAttribute));
		bOk = bOk and binarySliceSet->ContainsAttribute(attribute);
	}
	delete oaAttributes;

	return bOk;
}

void SNBHardAttributeSelectionScorer::Write(ostream& ost) const
{
	require(binarySliceSet != NULL);

	ost << "Target partition\n";
	ost << "Instances\t" << binarySliceSet->GetInstanceNumber() << "\n";
	ost << *dataCostCalculator;
}

const ALString SNBHardAttributeSelectionScorer::GetClassLabel() const
{
	return "Selection score";
}

const ALString SNBHardAttributeSelectionScorer::GetObjectLabel() const
{
	if (GetLearningSpec() == NULL)
		return "";
	else if (GetTargetAttributeType() == KWType::Symbol)
		return "Classifier";
	else if (GetTargetAttributeType() == KWType::Continuous)
		return "Regressor";
	else
		return "";
}

double SNBHardAttributeSelectionScorer::ComputeSelectionModelAttributeWeightCost() const
{
	double dModelCost;

	// Si modele nul, prise en compte du cout de preparation du modele nul
	dModelCost = GetNullConstructionCost();
	if (attributeSelection.GetAttributeNumber() == 0 and IsPreparationCostEnabled())
		dModelCost += GetNullPreparationCost();

	// Cas d'un modele informatif
	if (attributeSelection.GetAttributeNumber() > 0)
	{
		// Cout du codage du nombre d'attributs selectionnes
		dModelCost += KWStat::NaturalNumbersUniversalCodeLength(attributeSelection.GetAttributeNumber());

		// Cout du codage du choix du sous-ensemble des attributs
		dModelCost -= KWStat::LnFactorial(attributeSelection.GetAttributeNumber());
	}
	// Prise en compte du poids du prior
	dModelCost *= GetPriorWeight();

	return dModelCost;
}

longint SNBHardAttributeSelectionScorer::ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber,
								int nTargetValueNumber, int nTargetType,
								boolean bIsTargetGrouped,
								boolean bIncludeDataCostCalculator)
{
	longint lDataCostCalculatorMemory;

	require(nTargetType == KWType::Symbol or nTargetType == KWType::Continuous);

	// Estimation de la calculatrice de cout de donnees en fonction learning specs de la cible
	if (bIncludeDataCostCalculator)
	{
		if (nTargetType == KWType::Symbol and bIsTargetGrouped)
			lDataCostCalculatorMemory =
			    SNBGeneralizedClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(
				nInstanceNumber, nAttributeNumber, nTargetValueNumber);
		else if (nTargetType == KWType::Symbol)
			lDataCostCalculatorMemory = SNBClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(
			    nInstanceNumber, nTargetValueNumber);
		else
			lDataCostCalculatorMemory = SNBRegressorSelectionDataCostCalculator::ComputeNecessaryMemory(
			    nInstanceNumber, nTargetValueNumber);
	}
	else
		lDataCostCalculatorMemory = 0l;

	// Formule pour l'estimation:
	//   Objets direts de l'instance +
	//   Selection d'attributs (attributeSelection) +
	//   Calculatrice du cout de donnees de la selection (dataCostCalculator) +
	return sizeof(SNBHardAttributeSelectionScorer) +
	       SNBHardAttributeSelection::ComputeNecessaryMemory(nAttributeNumber) - sizeof(SNBHardAttributeSelection) +
	       lDataCostCalculatorMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBWeightedAttributeSelection
SNBWeightedAttributeSelection::SNBWeightedAttributeSelection()
{
	dWeightSum = 0.0;
}

SNBWeightedAttributeSelection::~SNBWeightedAttributeSelection()
{
	Clean();
}

void SNBWeightedAttributeSelection::Clean()
{
	nkdAttributeWeights.DeleteAll();
	dWeightSum = 0.0;
}

double SNBWeightedAttributeSelection::IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
							      double dDeltaWeight)
{
	DoubleObject* doWeight;
	double dOldWeight;
	double dNewWeight;
	double dEffectiveDeltaWeight;

	require(Check());
	require(attribute != NULL);
	require(dDeltaWeight >= 0.0);

	// Recuperation du poids ancien et calcul du nouveau poids
	// Si l'attribut n'est pas selectionne : Creation de son entree dans le dictionnaire de poids
	if (not Contains(attribute))
	{
		doWeight = new DoubleObject;
		doWeight->SetDouble(dDeltaWeight);
		nkdAttributeWeights.SetAt((NUMERIC)attribute, doWeight);
		dOldWeight = 0.0;
		dNewWeight = dDeltaWeight;
	}
	// Sinon : Mise a jour du poids
	else
	{
		doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup((NUMERIC)attribute));
		dOldWeight = doWeight->GetDouble();
		dNewWeight = dOldWeight + dDeltaWeight;
	}

	// Calcul du poids effectif pour ne pas depasser 1.0
	if (dNewWeight >= 1.0)
	{
		dNewWeight = 1.0;
		dEffectiveDeltaWeight = 1.0 - dOldWeight;
	}
	else
		dEffectiveDeltaWeight = dDeltaWeight;
	doWeight->SetDouble(dNewWeight);

	// Mis a jour de la somme des poids avec le delta effectif
	dWeightSum += dEffectiveDeltaWeight;

	ensure(Check());
	return dEffectiveDeltaWeight;
}

double SNBWeightedAttributeSelection::DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
							      double dDeltaWeight)
{
	DoubleObject* doWeight;
	double dOldWeight;
	double dNewWeight;
	double dEffectiveDeltaWeight;

	require(Check());
	require(attribute != NULL);
	require(Contains(attribute));
	require(dDeltaWeight >= 0.0);

	// Recuperation du poids ancien et calcul du nouveau
	doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup((NUMERIC)attribute));
	dOldWeight = doWeight->GetDouble();
	dNewWeight = dOldWeight - dDeltaWeight;

	// Si le poids est negatif: Elimination de l'attribut de la selection
	if (dNewWeight <= 0.0)
	{
		nkdAttributeWeights.RemoveKey((NUMERIC)attribute);
		delete doWeight;
		doWeight = NULL;
		dEffectiveDeltaWeight = dOldWeight;
	}
	// Sinon : Mise a jour son poids
	else
	{
		doWeight->SetDouble(dNewWeight);
		dEffectiveDeltaWeight = dDeltaWeight;
	}

	// Mise a jour de la somme des poids avec le delta effectif du poids
	dWeightSum -= dEffectiveDeltaWeight;

	ensure(Check());
	return dEffectiveDeltaWeight;
}

boolean SNBWeightedAttributeSelection::Contains(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	return nkdAttributeWeights.Lookup((NUMERIC)attribute) != NULL;
}

double SNBWeightedAttributeSelection::GetAttributeWeightAt(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	DoubleObject* doWeight;
	double dWeight;

	require(attribute != NULL);

	if (Contains(attribute))
	{
		doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup((NUMERIC)attribute));
		dWeight = doWeight->GetDouble();
	}
	else
		dWeight = 0.0;

	return dWeight;
}

int SNBWeightedAttributeSelection::GetAttributeNumber() const
{
	return nkdAttributeWeights.GetCount();
}

double SNBWeightedAttributeSelection::GetWeightSum() const
{
	return dWeightSum;
}

boolean SNBWeightedAttributeSelection::Check() const
{
	boolean bOk = true;
	POSITION position;
	NUMERIC key;
	Object* oValue;
	SNBDataTableBinarySliceSetAttribute* attribute;
	DoubleObject* doWeight;

	// Verification de l'integrite des attributs et ses poids
	position = nkdAttributeWeights.GetStartPosition();
	while (position != NULL)
	{
		nkdAttributeWeights.GetNextAssoc(position, key, oValue);
		attribute = (SNBDataTableBinarySliceSetAttribute*)key;
		doWeight = cast(DoubleObject*, oValue);
		bOk = bOk and attribute != NULL;
		bOk = bOk and doWeight != NULL;
		if (bOk)
			bOk = bOk and (0.0 < doWeight->GetDouble() and doWeight->GetDouble() <= 1.0);
	}
	return bOk;
}

SNBWeightedAttributeSelection* SNBWeightedAttributeSelection::Clone() const
{
	SNBWeightedAttributeSelection* cloneWeightedAttributeSelection;
	POSITION position;
	NUMERIC key;
	Object* oValue;
	SNBDataTableBinarySliceSetAttribute* attribute;
	DoubleObject* doWeight;

	cloneWeightedAttributeSelection = new SNBWeightedAttributeSelection;
	position = nkdAttributeWeights.GetStartPosition();
	while (position != NULL)
	{
		nkdAttributeWeights.GetNextAssoc(position, key, oValue);
		attribute = (SNBDataTableBinarySliceSetAttribute*)key;
		doWeight = cast(DoubleObject*, oValue);
		cloneWeightedAttributeSelection->IncreaseAttributeWeight(attribute, doWeight->GetDouble());
	}
	return cloneWeightedAttributeSelection;
}

ObjectArray* SNBWeightedAttributeSelection::CollectSelectedAttributes() const
{
	ObjectArray* oaSelectedAttributes;
	int nAttribute;
	POSITION position;
	NUMERIC key;
	Object* oValue;
	SNBDataTableBinarySliceSetAttribute* attribute;

	oaSelectedAttributes = new ObjectArray;
	oaSelectedAttributes->SetSize(nkdAttributeWeights.GetCount());

	position = nkdAttributeWeights.GetStartPosition();
	nAttribute = 0;
	while (position != NULL)
	{
		nkdAttributeWeights.GetNextAssoc(position, key, oValue);
		attribute = (SNBDataTableBinarySliceSetAttribute*)key;
		oaSelectedAttributes->SetAt(nAttribute, attribute);
		nAttribute++;
	}
	ensure(nAttribute == nkdAttributeWeights.GetCount());
	return oaSelectedAttributes;
}

void SNBWeightedAttributeSelection::Write(ostream& ost) const
{
	POSITION position;
	NUMERIC key;
	Object* oValue;
	SNBDataTableBinarySliceSetAttribute* attribute;
	ObjectArray oaAttributes;
	int nAttribute;

	ost << "Weighted attribute selection: ";
	if (nkdAttributeWeights.GetCount() == 0)
		ost << "<Empty>\n";
	else
	{
		// Tri par nom les attributs (cles) dans un tableau
		position = nkdAttributeWeights.GetStartPosition();
		nkdAttributeWeights.GetNextAssoc(position, key, oValue);
		oaAttributes.Add((Object*)key);
		while (position != NULL)
		{
			nkdAttributeWeights.GetNextAssoc(position, key, oValue);
			oaAttributes.Add((Object*)key);
		}
		oaAttributes.SetCompareFunction(SNBDataTableBinarySliceSetAttributeCompareNativeAttributeName);
		oaAttributes.Sort();

		// Ecriture des attributs et ses poids
		attribute = (SNBDataTableBinarySliceSetAttribute*)oaAttributes.GetAt(0);
		ost << "(" << attribute->GetNativeAttributeName() << ", " << GetAttributeWeightAt(attribute) << ")";
		for (nAttribute = 1; nAttribute < oaAttributes.GetSize(); nAttribute++)
		{
			attribute = (SNBDataTableBinarySliceSetAttribute*)oaAttributes.GetAt(nAttribute);
			ost << ", (" << attribute->GetNativeAttributeName() << ", " << GetAttributeWeightAt(attribute)
			    << ")";
		}
		ost << "\n";
	}
}

longint SNBWeightedAttributeSelection::ComputeNecessaryMemory(int nAttributeNumber)
{
	NumericKeyDictionary nkdDummy;

	// Formule de l'estimation :
	//   Objets directes de l'instance +
	//   Ensemble des pointeurs de la selection et ses poids (nkdAttributeWeights)
	return sizeof(SNBWeightedAttributeSelection) +
	       nAttributeNumber * (nkdDummy.GetUsedMemoryPerElement() + sizeof(DoubleObject));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWWeightedSelectionScorer

SNBWeightedAttributeSelectionScorer::SNBWeightedAttributeSelectionScorer()
{
	dPenalizationExponent = 0.0;
	lastModificationAttribute = NULL;
	dLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	bWasLastModificationIncrease = false;
}

SNBWeightedAttributeSelectionScorer::~SNBWeightedAttributeSelectionScorer()
{
	weightedAttributeSelection.Clean();
}

void SNBWeightedAttributeSelectionScorer::SetPriorExponent(double dExponent)
{
	dPenalizationExponent = dExponent;
}

double SNBWeightedAttributeSelectionScorer::GetPriorExponent() const
{
	return dPenalizationExponent;
}

void SNBWeightedAttributeSelectionScorer::InitializeWorkingData()
{
	require(Check());

	weightedAttributeSelection.Clean();
	if (IsDataCostCalculatorCreated())
		SNBAttributeSelectionScorer::InitializeDataCostCalculator();
}

void SNBWeightedAttributeSelectionScorer::CleanWorkingData()
{
	weightedAttributeSelection.Clean();
	SNBAttributeSelectionScorer::CleanWorkingData();
}

boolean SNBWeightedAttributeSelectionScorer::IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
								     double dDeltaWeight)
{
	boolean bOk = true;
	double dWeight;
	double dAlpha;
	double dAttributeCost;
	double dEffectiveDeltaWeight;

	require(Check());
	require(attribute != NULL);
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));
	require(dDeltaWeight >= 0);

	// Memorisation du poids courant de l'attribut
	dWeight = weightedAttributeSelection.GetAttributeWeightAt(attribute);

	// Increment du poids de l'attribut dans la selection; on memorise le delta effectif
	dEffectiveDeltaWeight = weightedAttributeSelection.IncreaseAttributeWeight(attribute, dDeltaWeight);

	// Memorisation des infos de la modification pour une eventuelle annulation
	dLastModificationSelectionModelAllAttributeCost = dSelectionModelAllAttributeCost;
	lastModificationAttribute = attribute;
	dLastModificationDeltaWeight = dEffectiveDeltaWeight;
	bWasLastModificationIncrease = true;

	// Mise a jour de la partie prior (penalisation) du cout de selection
	dAlpha = GetPriorExponent();
	dAttributeCost = ComputeSelectionModelAttributeCost(attribute);
	dSelectionModelAllAttributeCost +=
	    (pow(dWeight + dEffectiveDeltaWeight, dAlpha) - pow(dWeight, dAlpha)) * dAttributeCost;

	// Mise a jour des scores de la calculatrice de cout de donnees
	// En cas que l'attribut n'etait pas selectionne : Mise a jour de la partition de la cible *avant* celle des
	// vecteurs de scores
	if (IsDataCostCalculatorInitialized())
	{
		if (dWeight == 0.0)
			dataCostCalculator->UpdateTargetPartitionWithAddedAttribute(attribute);
		bOk = bOk and
		      dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(attribute, dEffectiveDeltaWeight);
	}

	// Trace de deboggage
	if (IsDataCostCalculatorInitialized() and dataCostCalculator->GetDisplay())
	{
		cout << "Increasing by\t" << dDeltaWeight << "\t" << attribute->GetNativeAttributeName() << "\n";
		dataCostCalculator->Write(cout);
		cout << "\n";
	}
	ensure(IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBWeightedAttributeSelectionScorer::DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
								     double dDeltaWeight)
{
	boolean bOk = true;
	double dWeight;
	double dAlpha;
	double dAttributeCost;
	double dEffectiveDeltaWeight;

	require(Check());
	require(attribute != NULL);
	require(weightedAttributeSelection.Contains(attribute));
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));
	require(dDeltaWeight >= 0);

	// Memorisation du poids courant de l'attribut
	dWeight = weightedAttributeSelection.GetAttributeWeightAt(attribute);

	// Decrement du poids de l'attribut dans la selection; on memorise le delta effectif
	dEffectiveDeltaWeight = weightedAttributeSelection.DecreaseAttributeWeight(attribute, dDeltaWeight);

	// Memorisation des infos de la modificiation pour une eventuelle annulation
	dLastModificationSelectionModelAllAttributeCost = dSelectionModelAllAttributeCost;
	lastModificationAttribute = attribute;
	dLastModificationDeltaWeight = dEffectiveDeltaWeight;
	bWasLastModificationIncrease = false;

	// Mise a jour de la partie prior (penalisation) du cout de selection
	dAlpha = GetPriorExponent();
	dAttributeCost = ComputeSelectionModelAttributeCost(attribute);
	dSelectionModelAllAttributeCost +=
	    (pow(dWeight - dEffectiveDeltaWeight, dAlpha) - pow(dWeight, dAlpha)) * dAttributeCost;

	// Mise a jour de la calculatrice de cout de donnees
	// En cas d'elimination de l'attribut de la selection : Mise a jour de la partition cible *apres* celle des
	// vecteurs de scores
	if (IsDataCostCalculatorInitialized())
	{
		bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(attribute,
											      -dEffectiveDeltaWeight);
		if (not weightedAttributeSelection.Contains(attribute))
			dataCostCalculator->UpdateTargetPartitionWithRemovedAttribute(attribute);
	}

	// Trace de deboggage
	if (IsDataCostCalculatorInitialized() and dataCostCalculator->GetDisplay())
	{
		cout << "Decreasing by"
		     << "\t" << dDeltaWeight << "\t" << attribute->GetNativeAttributeName() << "\n";
		dataCostCalculator->Write(cout);
		cout << "\n";
	}

	ensure(IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBWeightedAttributeSelectionScorer::UndoLastModification()
{
	boolean bOk = true;
	require(IsUndoAllowed());
	require(Check());

	dSelectionModelAllAttributeCost = dLastModificationSelectionModelAllAttributeCost;
	if (bWasLastModificationIncrease)
	{
		weightedAttributeSelection.DecreaseAttributeWeight(lastModificationAttribute,
								   dLastModificationDeltaWeight);
		if (IsDataCostCalculatorInitialized())
		{
			bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(
					  lastModificationAttribute, -dLastModificationDeltaWeight);
			if (not weightedAttributeSelection.Contains(lastModificationAttribute))
				dataCostCalculator->UpdateTargetPartitionWithRemovedAttribute(
				    lastModificationAttribute);

			if (dataCostCalculator->GetDisplay())
			{
				cout << "Increase undone\t" << dLastModificationDeltaWeight << "\t"
				     << lastModificationAttribute->GetNativeAttributeName() << "\n";
				dataCostCalculator->Write(cout);
				cout << "\n";
			}
		}
	}
	else
	{
		if (IsDataCostCalculatorInitialized())
		{
			if (not weightedAttributeSelection.Contains(lastModificationAttribute))
				dataCostCalculator->UpdateTargetPartitionWithAddedAttribute(lastModificationAttribute);
			bOk = bOk and dataCostCalculator->UpdateTargetPartScoresWithWeightedAttribute(
					  lastModificationAttribute, dLastModificationDeltaWeight);

			if (dataCostCalculator->GetDisplay())
			{
				cout << "Decrease undone\t" << dLastModificationDeltaWeight << "\t"
				     << lastModificationAttribute->GetNativeAttributeName() << "\n";
				dataCostCalculator->Write(cout);
				cout << "\n";
			}
		}
		weightedAttributeSelection.IncreaseAttributeWeight(lastModificationAttribute,
								   dLastModificationDeltaWeight);
	}

	lastModificationAttribute = NULL;
	dLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	bWasLastModificationIncrease = false;

	ensure(not IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBWeightedAttributeSelectionScorer::IsUndoAllowed()
{
	return lastModificationAttribute != NULL;
}

const SNBWeightedAttributeSelection* SNBWeightedAttributeSelectionScorer::GetAttributeSelection() const
{
	return &weightedAttributeSelection;
}

SNBWeightedAttributeSelection* SNBWeightedAttributeSelectionScorer::CollectAttributeSelection() const
{
	return weightedAttributeSelection.Clone();
}

boolean SNBWeightedAttributeSelectionScorer::Check() const
{
	boolean bOk;
	ObjectArray* oaAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	// Appel a la methode ancetre
	bOk = SNBAttributeSelectionScorer::Check();

	// Verification de la selection
	bOk = bOk and weightedAttributeSelection.Check();
	bOk = bOk and 0 <= weightedAttributeSelection.GetAttributeNumber();
	bOk = bOk and weightedAttributeSelection.GetAttributeNumber() <= binarySliceSet->GetAttributeNumber();
	oaAttributes = weightedAttributeSelection.CollectSelectedAttributes();
	for (nAttribute = 0; nAttribute < weightedAttributeSelection.GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes->GetAt(nAttribute));
		bOk = bOk and binarySliceSet->ContainsAttribute(attribute);
	}
	delete oaAttributes;

	return bOk;
}

longint SNBWeightedAttributeSelectionScorer::ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber,
								    int nTargetValueNumber, int nTargetType,
								    boolean bIsTargetGrouped,
								    boolean bIncludeDataCostCalculator)
{
	longint lDataCostCalculatorMemory;

	require(nTargetType == KWType::Symbol or nTargetType == KWType::Continuous);

	// Estimation de la calculatrice de cout de donnees en fonction learning specs de la cible
	if (bIncludeDataCostCalculator)
	{
		if (nTargetType == KWType::Symbol and bIsTargetGrouped)
		{
			lDataCostCalculatorMemory =
			    SNBGeneralizedClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(
				nInstanceNumber, nAttributeNumber, nTargetValueNumber);
		}
		else if (nTargetType == KWType::Symbol)
		{
			lDataCostCalculatorMemory = SNBClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(
			    nInstanceNumber, nTargetValueNumber);
		}
		else
			lDataCostCalculatorMemory = SNBRegressorSelectionDataCostCalculator::ComputeNecessaryMemory(
			    nInstanceNumber, nTargetValueNumber);
	}
	else
		lDataCostCalculatorMemory = 0ll;

	// Formule pour l'estimation:
	//   Objets direts de l'instance +
	//   Selection d'attributs (weightedAttributeSelection) +
	//   Calculatrice du cout de donnees de la selection (dataCostCalculator) +
	return sizeof(SNBHardAttributeSelectionScorer) +
	       SNBWeightedAttributeSelection::ComputeNecessaryMemory(nAttributeNumber) -
	       sizeof(SNBWeightedAttributeSelection) + lDataCostCalculatorMemory;
}

double SNBWeightedAttributeSelectionScorer::ComputeSelectionModelAttributeWeightCost() const
{
	double dModelCost;

	// Si modele nul, prise en compte du cout de preparation du modele nul
	dModelCost = GetNullConstructionCost();
	if (weightedAttributeSelection.GetAttributeNumber() == 0 and IsPreparationCostEnabled())
		dModelCost += GetNullPreparationCost();

	// Cas d'une selection non vide
	if (weightedAttributeSelection.GetAttributeNumber() > 0)
	{
		// Cout de codage du "nombre" attributs choisis (= somme des poids)
		dModelCost +=
		    KWStat::NaturalNumbersUniversalCodeLength(int(ceil(weightedAttributeSelection.GetWeightSum())));

		// Cout de codage du choix du sous-ensemble des attributs
		dModelCost -= KWStat::LnFactorial(int(ceil(weightedAttributeSelection.GetWeightSum())));
	}
	// Prise en compte du poids du prior
	dModelCost *= GetPriorWeight();

	return dModelCost;
}
