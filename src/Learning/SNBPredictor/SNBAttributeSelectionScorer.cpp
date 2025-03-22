// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	lastModificationAttribute = NULL;
	dLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	bWasLastModificationIncrease = false;

	// Le poids et exposant du prior (valeurs 0.1 et 0.95 resp.) relevent des etudes empiriques
	dPriorWeight = 0.1;
	dPriorExponent = 0.95;
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
	require(dValue >= 0.0);
	dPriorWeight = dValue;
}

double SNBAttributeSelectionScorer::GetPriorWeight() const
{
	return dPriorWeight;
}

void SNBAttributeSelectionScorer::SetPriorExponent(double dValue)
{
	require(dValue >= 0.0);
	dPriorExponent = dValue;
}

double SNBAttributeSelectionScorer::GetPriorExponent() const
{
	return dPriorExponent;
}

void SNBAttributeSelectionScorer::SetConstructionCostEnabled(boolean bIsEnabled)
{
	bIsConstructionCostEnabled = bIsEnabled;
}

boolean SNBAttributeSelectionScorer::GetConstructionCostEnabled() const
{
	return bIsConstructionCostEnabled;
}

void SNBAttributeSelectionScorer::SetPreparationCostEnabled(boolean bIsEnabled)
{
	bIsPreparationCostEnabled = bIsEnabled;
}

boolean SNBAttributeSelectionScorer::GetPreparationCostEnabled() const
{
	return bIsPreparationCostEnabled;
}

void SNBAttributeSelectionScorer::InitializeWorkingData()
{
	require(Check());

	dSelectionModelAllAttributeCost = 0.0;
	weightedAttributeSelection.Clean();
	if (IsDataCostCalculatorCreated())
		InitializeDataCostCalculator();
}

void SNBAttributeSelectionScorer::CleanWorkingData()
{
	weightedAttributeSelection.Clean();
	if (IsDataCostCalculatorCreated())
		CleanDataCostCalculator();
}

boolean SNBAttributeSelectionScorer::CreateDataCostCalculator()
{
	require(Check());

	// Nettoyage de la calculatrice de cout de donnees
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
	return ComputeSelectionModelCost() + GetSelectionDataCost();
}

double SNBAttributeSelectionScorer::GetSelectionDataCost()
{
	require(IsDataCostCalculatorInitialized());
	return dataCostCalculator->GetSelectionDataCost();
}

boolean SNBAttributeSelectionScorer::IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
							     double dDeltaWeight)
{
	boolean bOk = true;
	double dWeight;
	double dAlpha;
	double dAttributeCost;
	double dEffectiveDeltaWeight;
	boolean bUpdateTargetPartition;

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
	// En cas que l'attribut n'etait pas selectionne : Mise a jour de la partition de la cible *avant* celle des vecteurs de scores
	if (IsDataCostCalculatorInitialized())
	{
		bUpdateTargetPartition = dWeight == 0.0;
		bOk = bOk and dataCostCalculator->IncreaseAttributeWeight(attribute, dEffectiveDeltaWeight,
									  bUpdateTargetPartition);
	}

	ensure(IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBAttributeSelectionScorer::DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
							     double dDeltaWeight)
{
	boolean bOk = true;
	double dWeight;
	double dAlpha;
	double dAttributeCost;
	double dEffectiveDeltaWeight;
	boolean bUpdateTargetPartition;

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
	if (IsDataCostCalculatorInitialized())
	{
		bUpdateTargetPartition = not weightedAttributeSelection.Contains(attribute);
		bOk = bOk and dataCostCalculator->DecreaseAttributeWeight(attribute, dEffectiveDeltaWeight,
									  bUpdateTargetPartition);
	}

	ensure(IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBAttributeSelectionScorer::UndoLastModification()
{
	boolean bOk = true;
	require(IsUndoAllowed());
	require(Check());

	// Remise au dernier etat
	dSelectionModelAllAttributeCost = dLastModificationSelectionModelAllAttributeCost;
	if (bWasLastModificationIncrease)
		weightedAttributeSelection.DecreaseAttributeWeight(lastModificationAttribute,
								   dLastModificationDeltaWeight);
	else
		weightedAttributeSelection.IncreaseAttributeWeight(lastModificationAttribute,
								   dLastModificationDeltaWeight);
	if (IsDataCostCalculatorInitialized())
		bOk = bOk and dataCostCalculator->UndoLastModification();

	// Remise a zero du etat de l'undo
	lastModificationAttribute = NULL;
	dLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionModelAllAttributeCost = 0.0;
	bWasLastModificationIncrease = false;

	ensure(not IsUndoAllowed());
	ensure(Check());
	return bOk;
}

boolean SNBAttributeSelectionScorer::IsUndoAllowed()
{
	return lastModificationAttribute != NULL;
}

const SNBWeightedAttributeSelection* SNBAttributeSelectionScorer::GetAttributeSelection() const
{
	return &weightedAttributeSelection;
}

SNBWeightedAttributeSelection* SNBAttributeSelectionScorer::CollectAttributeSelection() const
{
	return weightedAttributeSelection.Clone();
}

double SNBAttributeSelectionScorer::ComputeSelectionModelCost() const
{
	return ComputeSelectionModelAttributeWeightCost() + dSelectionModelAllAttributeCost;
}

double SNBAttributeSelectionScorer::ComputeSelectionModelAttributeWeightCost() const
{
	double dModelCost;

	// Si modele nul, prise en compte du cout de preparation du modele nul
	dModelCost = GetNullConstructionCost();
	if (weightedAttributeSelection.GetAttributeNumber() == 0 and GetPreparationCostEnabled())
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

double
SNBAttributeSelectionScorer::ComputeSelectionModelAttributeCost(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	double dCost;

	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);

	// Cout de selection et de construction (sans cout de modele nul)
	dCost = 0.0;
	if (GetConstructionCostEnabled() and attribute->GetConstructionCost() > 0)
	{
		dCost += attribute->GetConstructionCost();
		dCost -= attribute->GetNullConstructionCost();
	}
	// Si pas de cout de construction, on se rabat sur un cout de selection de variables
	else
		dCost += log(GetDataTableBinarySliceSet()->GetInitialAttributeNumber() * 1.0);

	// Cout de preparation
	if (GetPreparationCostEnabled())
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
	ObjectArray* oaAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

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

longint SNBAttributeSelectionScorer::ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber,
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
	return sizeof(SNBAttributeSelectionScorer) +
	       SNBWeightedAttributeSelection::ComputeNecessaryMemory(nAttributeNumber) -
	       sizeof(SNBWeightedAttributeSelection) + lDataCostCalculatorMemory;
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
		nkdAttributeWeights.SetAt(attribute, doWeight);
		dOldWeight = 0.0;
		dNewWeight = dDeltaWeight;
	}
	// Sinon : Mise a jour du poids
	else
	{
		doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup(attribute));
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
	doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup(attribute));
	dOldWeight = doWeight->GetDouble();
	dNewWeight = dOldWeight - dDeltaWeight;

	// Si le poids est negatif: Elimination de l'attribut de la selection
	if (dNewWeight <= 0.0)
	{
		nkdAttributeWeights.RemoveKey(attribute);
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
	return nkdAttributeWeights.Lookup(attribute) != NULL;
}

double SNBWeightedAttributeSelection::GetAttributeWeightAt(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	DoubleObject* doWeight;
	double dWeight;

	require(attribute != NULL);

	if (Contains(attribute))
	{
		doWeight = cast(DoubleObject*, nkdAttributeWeights.Lookup(attribute));
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
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, (Object*)key.ToPointer());
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
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, (Object*)key.ToPointer());
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
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, (Object*)key.ToPointer());
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
	SNBDataTableBinarySliceSetAttribute* attribute;
	Object* oValue;
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
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, (Object*)key.ToPointer());
		oaAttributes.Add(attribute);
		while (position != NULL)
		{
			nkdAttributeWeights.GetNextAssoc(position, key, oValue);
			attribute = cast(SNBDataTableBinarySliceSetAttribute*, (Object*)key.ToPointer());
			oaAttributes.Add(attribute);
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
