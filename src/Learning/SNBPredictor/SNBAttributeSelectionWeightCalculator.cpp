// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBAttributeSelectionWeightCalculator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBAttributeSelectionWeightCalculator

SNBAttributeSelectionWeightCalculator::SNBAttributeSelectionWeightCalculator()
{
	binarySliceSet = NULL;
	cvAttributeWeights = NULL;
	nWeightingMethod = None;
	nTraceLevel = 0;
	bTraceSelectedAttributes = false;
}

SNBAttributeSelectionWeightCalculator::~SNBAttributeSelectionWeightCalculator()
{
	olEvaluatedAttributes.DeleteAll();
	if (cvAttributeWeights != NULL)
		delete cvAttributeWeights;
}

void SNBAttributeSelectionWeightCalculator::SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* database)
{
	binarySliceSet = database;
}

SNBDataTableBinarySliceSet* SNBAttributeSelectionWeightCalculator::GetDataTableBinarySliceSet() const
{
	return binarySliceSet;
}

ContinuousVector* SNBAttributeSelectionWeightCalculator::GetAttributeWeights()
{
	return cvAttributeWeights;
}

ContinuousVector* SNBAttributeSelectionWeightCalculator::CollectAttributeWeights()
{
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	ContinuousVector* cvOrderedAttributeWeights;

	require(binarySliceSet != NULL);
	require(binarySliceSet->GetDataPreparationClass() != NULL);

	cvOrderedAttributeWeights = new ContinuousVector;
	cvOrderedAttributeWeights->Initialize();
	cvOrderedAttributeWeights->SetSize(GetDataTableBinarySliceSet()->GetInitialAttributeNumber());
	for (nAttribute = 0; nAttribute < GetDataTableBinarySliceSet()->GetAttributeNumber(); nAttribute++)
	{
		attribute = GetDataTableBinarySliceSet()->GetAttributeAt(nAttribute);
		cvOrderedAttributeWeights->SetAt(attribute->GetDataPreparationClassIndex(),
						 cvAttributeWeights->GetAt(nAttribute));
	}

	return cvOrderedAttributeWeights;
}

void SNBAttributeSelectionWeightCalculator::SetWeightingMethod(int nMethod)
{
	require(nMethod == WeightingMethod::None or nMethod == WeightingMethod::PredictorCompressionRate or
		nMethod == WeightingMethod::PredictorProb);

	nWeightingMethod = nMethod;

	// Creation/destruction du vecteur de poids selon la methode
	if (nWeightingMethod == WeightingMethod::None)
	{
		if (cvAttributeWeights != NULL)
			delete cvAttributeWeights;
		cvAttributeWeights = NULL;
	}
	else
	{
		if (cvAttributeWeights == NULL)
			cvAttributeWeights = new ContinuousVector;
	}
}

int SNBAttributeSelectionWeightCalculator::GetWeightingMethod() const
{
	return nWeightingMethod;
}

const ALString SNBAttributeSelectionWeightCalculator::GetWeightingMethodLabel() const
{
	if (nWeightingMethod == WeightingMethod::PredictorCompressionRate)
		return "PredictorCompressionRate";
	else if (nWeightingMethod == WeightingMethod::PredictorProb)
		return "PredictorProb";
	else
		return "None";
}

void SNBAttributeSelectionWeightCalculator::SetTraceLevel(int nValue)
{
	require(0 <= nValue and nValue <= 3);
	nTraceLevel = nValue;
}

int SNBAttributeSelectionWeightCalculator::GetTraceLevel() const
{
	return nTraceLevel;
}

void SNBAttributeSelectionWeightCalculator::SetTraceSelectedAttributes(boolean bValue)
{
	bTraceSelectedAttributes = bValue;
}

boolean SNBAttributeSelectionWeightCalculator::GetTraceSelectedAttributes() const
{
	return bTraceSelectedAttributes;
}

void SNBAttributeSelectionWeightCalculator::Reset()
{
	require(GetDataTableBinarySliceSet() != NULL);

	// Supression des evaluations enregistres
	olEvaluatedAttributes.DeleteAll();

	// Retaillage eventuel du vecteur de poids
	if (GetWeightingMethod() != WeightingMethod::None)
	{
		check(cvAttributeWeights);
		cvAttributeWeights->SetSize(GetDataTableBinarySliceSet()->GetAttributeNumber());
		cvAttributeWeights->Initialize();
	}
	ensure(Check());
}

void SNBAttributeSelectionWeightCalculator::AddSelectionOptimizationRecord(
    int nAttributeEvaluationType, SNBDataTableBinarySliceSetAttribute* attribute, double dNewPredictorModelCost,
    double dNewPredictorDataCost, const SNBHardAttributeSelection* attributeSelection)
{
	SNBAttributeSelectionOptimizationRecord* attributeEvaluationRecord;

	require(SNBAttributeSelectionOptimizationRecord::Start <= nAttributeEvaluationType);
	require(nAttributeEvaluationType <= SNBAttributeSelectionOptimizationRecord::Final);
	require(attribute != NULL or nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::Start or
		nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::LocalOptimum or
		nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::GlobalOptimum or
		nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::ForcedRemoveAll or
		nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::ForcedEvaluation or
		nAttributeEvaluationType == SNBAttributeSelectionOptimizationRecord::Final);
	require(dNewPredictorModelCost >= 0);
	require(dNewPredictorDataCost >= 0);
	require(Check());

	// Creation/initialisation d'une evaluation d'attribut
	attributeEvaluationRecord = new SNBAttributeSelectionOptimizationRecord;
	attributeEvaluationRecord->SetAttribute(attribute);
	attributeEvaluationRecord->SetType(nAttributeEvaluationType);
	attributeEvaluationRecord->SetModelCost(dNewPredictorModelCost);
	attributeEvaluationRecord->SetDataCost(dNewPredictorDataCost);

	// Trace
	if (GetTraceLevel() > 0)
		TraceSelectionOptimizationRecord(attributeEvaluationRecord, attributeSelection);

	// Memorisation de l'evaluation si le poids est gere
	if (nWeightingMethod != None)
		olEvaluatedAttributes.AddTail(attributeEvaluationRecord);
	// Sinon, destruction
	else
		delete attributeEvaluationRecord;
}

void SNBAttributeSelectionWeightCalculator::TraceSelectionOptimizationRecord(
    SNBAttributeSelectionOptimizationRecord* record, const SNBHardAttributeSelection* attributeSelection)
{
	boolean bTrace;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	int nSelectedAttributeNumber;
	StringVector svSelectedAttributeNames;

	// Entete de la trace
	if (record->GetType() == SNBAttributeSelectionOptimizationRecord::Start)
	{
		cout << "Nb Var.\t";
		record->WriteLabel(cout);
	}

	// Trace niveau 1 : seul les optimaux locaux et globaux sont affiches
	// Trace niveau 2 : seuls les changements importants sont affiches
	// Trace niveau 3 : tout est affiche
	if (GetTraceLevel() == 1)
	{
		bTrace = record->GetType() == SNBAttributeSelectionOptimizationRecord::Start or
			 record->GetType() == SNBAttributeSelectionOptimizationRecord::LocalOptimum or
			 record->GetType() == SNBAttributeSelectionOptimizationRecord::GlobalOptimum or
			 record->GetType() == SNBAttributeSelectionOptimizationRecord::Final;
	}
	else if (GetTraceLevel() == 2)
		bTrace = record->IsAcceptationType();
	else
		bTrace = true;

	// Affichage
	if (bTrace)
	{
		// Suite de la trace
		// TODO
		cout << attributeSelection->GetAttributeNumber() << "\t";
		if (record->GetAttribute() != NULL)
			cout << record->GetAttribute()->GetPreparedAttributeName();
		else
			cout << "";
		cout << "\t" << *record;

		// Affichage de la presence des attributs
		// if (bTraceSelectedAttributes)
		if (false)
		{
			cout << "\t";
			nSelectedAttributeNumber = 0;
			for (nAttribute = 0; nAttribute < GetDataTableBinarySliceSet()->GetAttributeNumber();
			     nAttribute++)
			{
				attribute = GetDataTableBinarySliceSet()->GetAttributeAt(nAttribute);
				if (attributeSelection->Contains(attribute))
				{
					svSelectedAttributeNames.Add(attribute->GetRecodedAttributeName());
					nSelectedAttributeNumber++;
				}
			}

			svSelectedAttributeNames.Sort();
			for (nAttribute = 0; nAttribute < svSelectedAttributeNames.GetSize(); nAttribute++)
				cout << svSelectedAttributeNames.GetAt(nAttribute) << "\t";
		}
		cout << "\n";
	}
}

void SNBAttributeSelectionWeightCalculator::ComputeAttributeWeigths()
{
	int nAttribute;
	SNBAttributeSelectionOptimizationRecord* evaluationRecord;
	IntVector ivSelectedAttributes;
	NumericKeyDictionary nkdSelectedAttributeSet;
	double dInitialEvaluation;
	double dFinalEvaluation;
	Continuous cWeightThreshold;
	Continuous cWeight;
	Continuous cTotalWeight;
	double dCompressionRate;
	POSITION headPosition;
	POSITION secondPosition;
	POSITION currentPosition;

	require(Check());
	require(olEvaluatedAttributes.GetCount() >= 2);

	// Initialisation du vecteur-dictionnaire d'attributs selectiones
	ivSelectedAttributes.SetSize(GetDataTableBinarySliceSet()->GetAttributeNumber());
	ivSelectedAttributes.Initialize();

	// Calcul uniquement si le poids est gere
	if (nWeightingMethod != None)
	{
		// Recherche de l'evaluation initiale
		headPosition = olEvaluatedAttributes.GetHeadPosition();
		evaluationRecord =
		    cast(SNBAttributeSelectionOptimizationRecord*, olEvaluatedAttributes.GetAt(headPosition));
		assert(evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::Start);
		dInitialEvaluation = evaluationRecord->GetTotalCost();

		// Modification de l'evaluation initiale si presence d'attributs obligatoires
		// Dans ce cas, l'evaluation initiale est la selection de tous les attributs obligatoires
		secondPosition = headPosition;
		olEvaluatedAttributes.GetNext(secondPosition);
		if (cast(SNBAttributeSelectionOptimizationRecord*, olEvaluatedAttributes.GetAt(secondPosition))
			->GetType() == SNBAttributeSelectionOptimizationRecord::MandatoryAdd)
		{
			currentPosition = secondPosition;
			while (currentPosition != NULL)
			{
				evaluationRecord = cast(SNBAttributeSelectionOptimizationRecord*,
							olEvaluatedAttributes.GetNext(currentPosition));
				if (evaluationRecord->GetType() ==
				    SNBAttributeSelectionOptimizationRecord::MandatoryAdd)
					dInitialEvaluation = evaluationRecord->GetTotalCost();
				else
					break;
			}
		}

		// Recherche de l'evaluation finale
		evaluationRecord = cast(SNBAttributeSelectionOptimizationRecord*, olEvaluatedAttributes.GetTail());
		assert(evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::Final);
		dFinalEvaluation = evaluationRecord->GetTotalCost();

		// Calcul du seuil de prise en compte des poids des classifieurs et des poids terminaux
		cWeightThreshold = (Continuous)(1.0 / (GetDataTableBinarySliceSet()->GetInstanceNumber() + 1.0));

		// Parcours des evaluations d'attribut pour le calcul des poids
		cTotalWeight = 0;
		currentPosition = headPosition;
		while (currentPosition != NULL)
		{
			evaluationRecord = cast(SNBAttributeSelectionOptimizationRecord*,
						olEvaluatedAttributes.GetNext(currentPosition));

			// Calcul du poids du modele selon la methode
			if (nWeightingMethod == WeightingMethod::PredictorCompressionRate)
			{
				dCompressionRate =
				    (dInitialEvaluation - evaluationRecord->GetTotalCost()) / dInitialEvaluation;
				cWeight = (Continuous)dCompressionRate;

				// On ignore les modeles dont le cout est superieur au cout du modele par defaut
				if (cWeight < 0)
					cWeight = 0;
			}
			else
				cWeight = (Continuous)exp(dFinalEvaluation - evaluationRecord->GetTotalCost());

			// Evaluation par ajout d'attribut
			if (evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::AddAttribute)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpgradeAttributeWeigths(&nkdSelectedAttributeSet, cWeight);

				// Plus celui de l'attribut ajoute
				cvAttributeWeights->UpgradeAt(evaluationRecord->GetAttribute()->GetIndex(), cWeight);
			}
			// Evaluation par supression d'attribut
			else if (evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::Remove)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpgradeAttributeWeigths(&nkdSelectedAttributeSet, cWeight);

				// Sauf celui de l'attribut supprime
				cvAttributeWeights->UpgradeAt(evaluationRecord->GetAttribute()->GetIndex(), -cWeight);
			}
			// Evaluation forcee
			else if (evaluationRecord->GetType() ==
				 SNBAttributeSelectionOptimizationRecord::ForcedEvaluation)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpgradeAttributeWeigths(&nkdSelectedAttributeSet, cWeight);
			}

			// Ajout d'un attribut dans la selection
			if (evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::MandatoryAdd or
			    evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::UnevaluatedAdd or
			    evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::BestAdd)
			{
				nkdSelectedAttributeSet.SetAt(evaluationRecord->GetAttribute(),
							      evaluationRecord->GetAttribute());
			}
			// Suppression d'un attribut de la selection
			else if (evaluationRecord->GetType() ==
				     SNBAttributeSelectionOptimizationRecord::UnevaluatedRemove or
				 evaluationRecord->GetType() == SNBAttributeSelectionOptimizationRecord::BestRemove)
			{
				nkdSelectedAttributeSet.RemoveKey(evaluationRecord->GetAttribute());
			}
			// Suppression de tous les attributs de la selection
			else if (evaluationRecord->GetType() ==
				 SNBAttributeSelectionOptimizationRecord::ForcedRemoveAll)
				nkdSelectedAttributeSet.RemoveAll();
		}

		// Normalisation des poids
		if (cTotalWeight > 0)
		{
			for (nAttribute = 0; nAttribute < cvAttributeWeights->GetSize(); nAttribute++)
			{
				cWeight = cvAttributeWeights->GetAt(nAttribute) / cTotalWeight;
				if (cWeight < cWeightThreshold)
					cvAttributeWeights->SetAt(nAttribute, 0);
				else
					cvAttributeWeights->SetAt(nAttribute, cWeight);
			}
		}
	}
}

boolean SNBAttributeSelectionWeightCalculator::Check() const
{
	boolean bOk = true;

	bOk = bOk and binarySliceSet != NULL;
	bOk = bOk and binarySliceSet->Check();

	if (bOk and nWeightingMethod != None)
		bOk = cvAttributeWeights != NULL and
		      cvAttributeWeights->GetSize() == GetDataTableBinarySliceSet()->GetAttributeNumber();
	return bOk;
}

void SNBAttributeSelectionWeightCalculator::WriteAttributeWeights(ostream& ost) const
{
	int nAttribute;

	require(Check());

	ost << "Variable\tWeight\n";
	for (nAttribute = 0; nAttribute < GetDataTableBinarySliceSet()->GetAttributeNumber(); nAttribute++)
	{
		// Affichage du poids si la methode le prend en compte
		if (nWeightingMethod != None)
			ost << GetDataTableBinarySliceSet()->GetAttributeNameAt(nAttribute) << "\t"
			    << cvAttributeWeights->GetAt(nAttribute) << endl;
		else
			ost << "\t0\n";
	}
	ost << flush;
}

void SNBAttributeSelectionWeightCalculator::UpgradeAttributeWeigths(NumericKeyDictionary* nkdSelectedAttributeSet,
								    Continuous cDeltaWeight)
{
	ObjectArray oaSelectedAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	require(nkdSelectedAttributeSet != NULL);
	require(Check());

	// Parcours des attributs selectionnes pour mettre a jour leur poids
	nkdSelectedAttributeSet->ExportObjectArray(&oaSelectedAttributes);
	for (nAttribute = 0; nAttribute < oaSelectedAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaSelectedAttributes.GetAt(nAttribute));
		cvAttributeWeights->UpgradeAt(attribute->GetIndex(), cDeltaWeight);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBAttributeSelectionOptimizationRecord

SNBAttributeSelectionOptimizationRecord::SNBAttributeSelectionOptimizationRecord()
{
	evaluatedAttribute = NULL;
	nType = Start;
	dModelCost = 0;
	dDataCost = 0;
}

SNBAttributeSelectionOptimizationRecord::~SNBAttributeSelectionOptimizationRecord() {}

SNBDataTableBinarySliceSetAttribute* SNBAttributeSelectionOptimizationRecord::GetAttribute() const
{
	return evaluatedAttribute;
}

void SNBAttributeSelectionOptimizationRecord::SetAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	evaluatedAttribute = attribute;
}

void SNBAttributeSelectionOptimizationRecord::SetType(int nValue)
{
	require(Start <= nValue and nValue <= Final);
	nType = nValue;
}

int SNBAttributeSelectionOptimizationRecord::GetType()
{
	return nType;
}

ALString SNBAttributeSelectionOptimizationRecord::GetTypeLabel() const
{
	if (nType == Start)
		return "Initial";
	else if (nType == AddAttribute)
		return "Add";
	else if (nType == MandatoryAdd)
		return "MandatoryAdd";
	else if (nType == BestAdd)
		return "BestAdd";
	else if (nType == Remove)
		return "Remove";
	else if (nType == BestRemove)
		return "BestRemove";
	else if (nType == LocalOptimum)
		return "LocalOptimum";
	else if (nType == GlobalOptimum)
		return "GlobalOptimum";
	else if (nType == ForcedRemoveAll)
		return "ForcedRemoveAll";
	else if (nType == UnevaluatedAdd)
		return "UnevaluatedAdd";
	else if (nType == UnevaluatedRemove)
		return "UnevaluatedRemove";
	else if (nType == ForcedEvaluation)
		return "ForcedEvaluation";
	else if (nType == Final)
		return "Final";
	else
		return "";
}

boolean SNBAttributeSelectionOptimizationRecord::IsAcceptationType() const
{
	return nType == Start or nType == MandatoryAdd or nType == BestAdd or nType == BestRemove or
	       nType == ForcedEvaluation or nType == LocalOptimum or nType == GlobalOptimum or nType == Final;
}

void SNBAttributeSelectionOptimizationRecord::SetModelCost(double dValue)
{
	dModelCost = dValue;
}

double SNBAttributeSelectionOptimizationRecord::GetModelCost() const
{
	return dModelCost;
}

void SNBAttributeSelectionOptimizationRecord::SetDataCost(double dValue)
{
	dDataCost = dValue;
}

double SNBAttributeSelectionOptimizationRecord::GetDataCost() const
{
	return dDataCost;
}

double SNBAttributeSelectionOptimizationRecord::GetTotalCost() const
{
	return dModelCost + dDataCost;
}

void SNBAttributeSelectionOptimizationRecord::WriteLabel(ostream& ost) const
{
	ost << "Variable\tType\tTotal\tModel\tData\n";
}

void SNBAttributeSelectionOptimizationRecord::Write(ostream& ost) const
{
	ost << GetTypeLabel() << "\t";
	ost << GetTotalCost() << "\t";
	ost << GetModelCost() << "\t";
	ost << GetDataCost();
}