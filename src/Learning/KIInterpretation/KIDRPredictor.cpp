// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRPredictor.h"

int KICompareContributionImportanceValue(const void* elem1, const void* elem2);
int KICompareReinforcementNewScore(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////////////
// Classe KIPartitionedAttributeProbas
KIPartitionedAttributeProbas::KIPartitionedAttributeProbas()
{
	nAttributeIndex = -1;
	nModalityIndex = -1;
	dReinforcementNewScore = 0;
	dReinforcementClassHasChanged = 0;
	dContributionImportanceValue = 0;
}

KIPartitionedAttributeProbas::~KIPartitionedAttributeProbas() {}

///////////////////////////////////////////////////////////////////////////////
// Classe KITargetValueProbas
KITargetValueProbas::KITargetValueProbas()
{
	dProbaApriori = 0;
	oaProbasAposteriori = new ObjectArray;
}

KITargetValueProbas::~KITargetValueProbas()
{
	oaProbasAposteriori->DeleteAll();
	delete oaProbasAposteriori;
}

void KITargetValueProbas::Write(ostream& ost) const
{
	int npart;
	int nval;

	ost << "KITargetValueProbas pour target " << sTargetValue << " : ";
	ost << "\tproba a priori : " << dProbaApriori << endl;
	ost << "\tproba a posteriori : " << endl;

	// un ContinuousVector * par variable explicative. Chaque ContinuousVector contient les logs des probas a
	// 	posteriori de la classe, pour chaque partie de la variable explicative

	for (npart = 0; npart < oaProbasAposteriori->GetSize(); npart++)
	{
		ContinuousVector* cv = cast(ContinuousVector*, oaProbasAposteriori->GetAt(npart));
		ost << "\tVar " << npart << " :\t";
		for (nval = 0; nval < cv->GetSize(); nval++)
			ost << exp(cv->GetAt(nval)) << ", ";
		ost << endl;
	}
}

longint KITargetValueProbas::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KITargetValueProbas);
	if (oaProbasAposteriori != NULL)
		lUsedMemory += oaProbasAposteriori->GetOverallUsedMemory();

	return lUsedMemory;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierInterpretation
KIDRClassifierInterpretation::KIDRClassifierInterpretation()
{
	oaInstanceProbabilities = NULL;
	dTotalFrequency = 0;
}

KIDRClassifierInterpretation::~KIDRClassifierInterpretation()
{
	Clean();
}

void KIDRClassifierInterpretation::Clean()
{
	oaModelProbabilities.DeleteAll();
	oaModelProbabilities.SetSize(0);

	svPartitionedPredictiveAttributeNames.SetSize(0);

	svNativePredictiveAttributeNames.SetSize(0);

	odClassNamesIndexes.DeleteAll();
	odClassNamesIndexes.RemoveAll();

	cvVariableWeights.SetSize(0);
	svTargetValues.SetSize(0);

	if (oaInstanceProbabilities != NULL)
	{
		oaInstanceProbabilities->DeleteAll();
		delete oaInstanceProbabilities;
		oaInstanceProbabilities = NULL;
	}
}

void KIDRClassifierInterpretation::Compile(KWClass* kwcOwnerClass)
{
	KITargetValueProbas* targetValueProbas;
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;
	int nTargetValueNumber;
	int nFirstOperandIndex;
	KWDRSNBClassifier referenceSNBRule;
	KWDRNBClassifier referenceNBRule;
	boolean bIsSNB = false;
	boolean bIsNB = false;
	ALString sAttributeName;
	int nTargetFrequency;
	const KWDRDataGridStats refDataGridStatsRule;
	const KWDRDataGridStatsBlock refDataGridStatsBlockRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KWDRDataGridBlock* dataGridBlockRule;
	KWAttributeBlock* attributeBlock;
	KWAttribute* attribute;
	int nDataGridStatsOrBlock;
	int nIndex;
	int nDataGrid;
	int nTarget;
	int nBlockValue;
	int nClassIndex;
	int nAttributeIndex;
	int nSourceIndex;
	int nTargetIndex;
	KWDRNBClassifier* classifier;
	Continuous cTargetLogProb;
	ContinuousVector* cv;
	ContinuousVector* cvPosteriorProba;
	StringObject* soClassIndex;
	KWDerivationRule::Compile(kwcOwnerClass);
	const KWDRDataGridStats* dataGridStats;
	nFirstOperandIndex = 0;
	Clean();

	classifier = cast(KWDRNBClassifier*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

	if (classifier->GetName() == referenceSNBRule.GetName())
	{
		bIsSNB = true;
		nFirstOperandIndex = 1;
		// recuperer les poids du SNB
		cvVariableWeights.CopyFrom(
		    cast(KWDRContinuousVector*, classifier->GetFirstOperand()->GetDerivationRule())->GetValues());
	}
	else
	{
		if (classifier->GetName() == referenceNBRule.GetName())
			bIsNB = true;
	}

	// Parcours des operandes du classifieur pour identifier les noms des attributs explicatifs et des attributs natifs associes
	// La derniere operande n'est pas parcouru car reserve a l'attribut des valeurs cibles
	classifier->ExportAttributeNames(&svPartitionedPredictiveAttributeNames, &svNativePredictiveAttributeNames);

	// Cas d'un NB on met tous les poid a 1.0
	if (bIsNB)
	{
		for (nIndex = 0; nIndex < svPartitionedPredictiveAttributeNames.GetSize(); nIndex++)
			cvVariableWeights.Add(1.0);
	}

	// Recherche du dernier operande du classifieur: distribution des valeurs cibles
	targetDataGrid = cast(
	    const KWDRDataGrid*,
	    classifier->GetOperandAt(classifier->GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetDataGrid->GetAttributeNumber() == 1);
	assert(targetDataGrid->GetAttributeTypeAt(0) == KWType::Symbol);
	nTargetValueNumber = targetDataGrid->GetTotalCellNumber();

	// Initialisation du vecteur des valeurs cibles
	targetSymbolValueSet = cast(const KWDRSymbolValueSet*,
				    targetDataGrid->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetSymbolValueSet->GetValueNumber() == nTargetValueNumber);
	svTargetValues.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		svTargetValues.SetAt(nTarget, targetSymbolValueSet->GetValueAt(nTarget));

	// Creation et initialisation du tableau des probabilites (sans remplissage lors de ce parcours)
	for (nClassIndex = 0; nClassIndex < svTargetValues.GetSize(); nClassIndex++)
	{
		// Creation du tableau associe a la classe de l'attribut
		targetValueProbas = new KITargetValueProbas;

		targetValueProbas->sTargetValue = svTargetValues.GetAt(nClassIndex);

		// Parcours des variables explicatives
		for (nAttributeIndex = 0; nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize();
		     nAttributeIndex++)
		{
			// Creation du vecteur qui accueillera :
			// les logs des probas a posteriori de la classe pour chaque partie de la variable explicative
			cvPosteriorProba = new ContinuousVector;
			targetValueProbas->oaProbasAposteriori->Add(cvPosteriorProba);

			// Extraction de l'attribut explicatif courant
			attribute =
			    kwcClass->LookupAttribute(svPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex));

			// Initialisation de la taille du vecteur de proba de l'attribut
			cvPosteriorProba->SetSize(
			    cast(KWDRDataGrid*, attribute->GetDerivationRule())->GetAttributePartNumberAt(0));
		}

		// Memorisation du lien entre le nom de la classe cible et l'index de la ligne correspondante
		// dans le tableau des probas
		soClassIndex = new StringObject;
		soClassIndex->SetString(IntToString(oaModelProbabilities.GetSize()));

		assert(odClassNamesIndexes.Lookup(targetValueProbas->sTargetValue) == NULL);
		odClassNamesIndexes.SetAt(targetValueProbas->sTargetValue, soClassIndex);

		// Ajout du vecteur associe a la classe cible
		oaModelProbabilities.Add(targetValueProbas);
	}

	if (bIsNB or bIsSNB)
	{
		// Extraction des probas a partir de la RDD l'attribut NB predicteur
		// Calcul de l'effectif global
		dTotalFrequency = 0;
		ivTargetFrequencies.SetSize(classifier->GetDataGridSetTargetPartNumber());
		for (nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			nTargetFrequency = classifier->GetDataGridSetTargetFrequencyAt(nClassIndex);
			assert(nTargetFrequency > 0);
			ivTargetFrequencies.SetAt(nClassIndex, nTargetFrequency);
			dTotalFrequency += (Continuous)nTargetFrequency;
		}
		assert(dTotalFrequency > 0);

		// Calcul des logarithme de probabilites des valeurs cibles
		for (nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			// Extraction du tableau des probas de cette valeur cible
			targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

			// Initialisation avec le prior
			assert(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) > 0);
			targetValueProbas->dProbaApriori =
			    log(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) / dTotalFrequency);

			nDataGrid = 0;

			// Ajout des probabilites conditionnelles par grille
			for (nDataGridStatsOrBlock = 0;
			     nDataGridStatsOrBlock < classifier->GetDataGridStatsOrBlockNumber();
			     nDataGridStatsOrBlock++)
			{
				if (classifier->IsDataGridStatsAt(nDataGridStatsOrBlock))
				{
					// Recherche de la grille
					dataGridStats = classifier->GetDataGridStatsAt(nDataGridStatsOrBlock);

					// Recherche de l'index de la partie cible de la grille
					nTargetIndex = classifier->GetDataGridSetTargetCellIndexAt(
					    nDataGridStatsOrBlock, nClassIndex);

					// Parcours de toutes les parties sources
					for (nSourceIndex = 0;
					     nSourceIndex < dataGridStats->GetDataGridSourceCellNumber();
					     nSourceIndex++)
					{
						// Extraction de la log proba
						cTargetLogProb = dataGridStats->GetDataGridSourceConditionalLogProbAt(
						    nSourceIndex, nTargetIndex);

						cv = cast(ContinuousVector*,
							  targetValueProbas->oaProbasAposteriori->GetAt(nDataGrid));
						cv->SetAt(nSourceIndex, cTargetLogProb);
					}

					// Mise-a-jour du compteur de grilles
					nDataGrid++;
				}
				else
				{

					dataGridStatsBlockRule =
					    classifier->GetDataGridStatsBlockAt(nDataGridStatsOrBlock);
					dataGridBlockRule =
					    cast(KWDRDataGridBlock*, dataGridStatsBlockRule->GetFirstOperand()
									 ->GetOriginAttribute()
									 ->GetDerivationRule());
					attributeBlock =
					    dataGridStatsBlockRule->GetSecondOperand()->GetOriginAttributeBlock();

					for (nBlockValue = 0;
					     nBlockValue < dataGridStatsBlockRule->GetDataGridStatsNumber();
					     nBlockValue++)
					{
						dataGridStats =
						    dataGridStatsBlockRule->GetDataGridStatsAtBlockIndex(nBlockValue);
						// Acces aux indexes de la source et la cible
						// La source doit etre ajuste a zero par des raisons techiques de l'implementation de la regle DataGridBlock

						// Recherche de l'index de la partie cible de la grille
						nTargetIndex = classifier->GetDataGridSetTargetCellIndexAt(
						    nDataGridStatsOrBlock, nClassIndex);

						// Mise a jour du terme de proba, en prenant en compte le poids de la grille

						for (nSourceIndex = 0;
						     nSourceIndex < dataGridStats->GetDataGridSourceCellNumber();
						     nSourceIndex++)
						{
							// Extraction de la log proba
							cTargetLogProb =
							    dataGridStats->GetDataGridSourceConditionalLogProbAt(
								nSourceIndex, nTargetIndex);

							cv = cast(
							    ContinuousVector*,
							    targetValueProbas->oaProbasAposteriori->GetAt(nDataGrid));
							cv->SetAt(nSourceIndex, cTargetLogProb);
						}
						// Mise-a-jour du compteur de grilles
						nDataGrid++;
					}
				}
			}
			ensure(nDataGrid == targetValueProbas->oaProbasAposteriori->GetSize());
		}
	}
}

longint KIDRClassifierInterpretation::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KIDRClassifierInterpretation);
	lUsedMemory += svNativePredictiveAttributeNames.GetUsedMemory();
	lUsedMemory += odClassNamesIndexes.GetOverallUsedMemory();
	lUsedMemory += oaModelProbabilities.GetOverallUsedMemory();
	lUsedMemory += svPartitionedPredictiveAttributeNames.GetUsedMemory();
	lUsedMemory += cvVariableWeights.GetUsedMemory();
	lUsedMemory += svTargetValues.GetUsedMemory();
	lUsedMemory += ivTargetFrequencies.GetUsedMemory();

	return lUsedMemory;
}

const ALString KIDRClassifierInterpretation::LEVER_ATTRIBUTE_META_TAG = "LeverVariable";
const ALString KIDRClassifierInterpretation::INTERPRETATION_ATTRIBUTE_META_TAG = "ClassifierInterpretationVariable";
const ALString KIDRClassifierInterpretation::NO_VALUE_LABEL = "";
const ALString KIDRClassifierInterpretation::SHAPLEY_LABEL = "Shapley";
const ALString KIDRClassifierInterpretation::PREDICTED_CLASS_LABEL = "Predicted class";
const ALString KIDRClassifierInterpretation::CLASS_OF_HIGHEST_GAIN_LABEL = "Class of highest gain";
const ALString KIDRClassifierInterpretation::ALL_CLASSES_LABEL = "All classes";

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierContribution
KIDRClassifierContribution::KIDRClassifierContribution()
{
	SetName("ScoreContribution");
	SetLabel("Classifier score contribution");
	SetType(KWType::Structure);
	SetStructureName("ScoreContribution");
	SetOperandNumber(5);
	GetFirstOperand()->SetType(KWType::Structure);
	// classifieur
	GetFirstOperand()->SetStructureName("Classifier");
	// classe predite (attribut de prediction)
	GetSecondOperand()->SetType(KWType::Symbol);
	// classe cible pour le calcul de l'importance
	GetOperandAt(2)->SetType(KWType::Symbol);
	// methode de calcul de l'importance de la variable
	GetOperandAt(3)->SetType(KWType::Symbol);
	// indique si on trie les probas ou non (modifiable en mode expert
	// uniquement, et par defaut, on trie)
	GetOperandAt(4)->SetType(KWType::Symbol);

	oaInstanceProbabilities = NULL;
	bSortInstanceProbas = false;
}

KIDRClassifierContribution::~KIDRClassifierContribution()
{
	//clean oaShapleyTables
	if (oaShapleyTables.GetSize() > 0)
	{
		oaShapleyTables.DeleteAll();
		oaShapleyTables.SetSize(0);
	}
}

KWDerivationRule* KIDRClassifierContribution::Create() const
{
	return new KIDRClassifierContribution;
}

Object* KIDRClassifierContribution::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// evaluer l'operande classifieur
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Calcul des donnees d'interpretation
	ComputeContribution(kwoObject);

	return (Object*)this;
}

Continuous KIDRClassifierContribution::GetContributionValueAt(int rank) const
{
	KIPartitionedAttributeProbas* attributeProbas;
	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->dContributionImportanceValue;
}

Symbol KIDRClassifierContribution::GetContributionNameAt(int rank) const
{
	KIPartitionedAttributeProbas* attributeProbas;
	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	;
	ALString as = svPartitionedPredictiveAttributeNames.GetAt(attributeProbas->nAttributeIndex);
	return as.GetBuffer(as.GetLength());
}

Symbol KIDRClassifierContribution::GetContributionPartitionAt(int rank) const
{
	KIPartitionedAttributeProbas* attributeProbas;
	Symbol attributeName;
	KWAttribute* attribute;
	KWDataGridStats* dataGridStats;
	ostringstream oss;

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));

	attributeName = GetContributionNameAt(rank);

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	attribute = kwcClass->LookupAttribute(attributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient l'individu pour cette variable
	dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);

	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbas->nModalityIndex);
	delete dataGridStats;

	return oss.str().c_str();
}

Symbol KIDRClassifierContribution::GetContributionClass() const
{
	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	return sContributionClass;
}

longint KIDRClassifierContribution::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;

	lUsedMemory = KIDRClassifierInterpretation::GetUsedMemory();
	lUsedMemory += sizeof(KIDRClassifierContribution) - sizeof(KIDRClassifierContribution);
	for (i = 0; i < oaShapleyTables.GetSize(); i++)
		lUsedMemory += cast(KIShapleyTable*, oaShapleyTables.GetAt(i))->GetUsedMemory();
	return lUsedMemory;
}

void KIDRClassifierContribution::ComputeContribution(const KWObject* kwoObject) const
{
	assert(IsCompiled());

	KIPartitionedAttributeProbas* partitionedAttributeProbas;
	Continuous cImportanceValue = -1;
	StringObject* sClassIndex;
	int nAttributeIndex;
	int nClassIndex;
	int nDatabaseSize;
	int nClassNumber;
	int nIndex;
	KITargetValueProbas* targetValueProbas;
	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	IntVector ivModalityIndexes;
	Continuous cInitialScore;
	Continuous cGain;
	Continuous cMaxGain;
	Continuous cPriorProba;
	ContinuousVector* cvScoreVector;
	KWAttribute* predictiveAttribute;
	KWAttribute* predictedClassAttribute;
	KWLoadIndex nNativeIndex;
	ALString sRuleLabel;

	ivModalityIndexes.SetSize(svPartitionedPredictiveAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable predictive, l'index de la partie (valeur variable partionnee) pour
	// l'individu courant
	for (nAttributeIndex = 0; nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		predictiveAttribute =
		    kwcClass->LookupAttribute(svPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex));

		// Extraction de l'index de l'attribut natif
		nNativeIndex =
		    kwcClass->LookupAttribute(svNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetLoadIndex();

		int nModalityIndex = -1;

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de groupage
		sRuleLabel =
		    predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule()->GetLabel();

		// Cas d'une discretisation
		if (sRuleLabel == discretizationRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*,
				 predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetContinuousPartIndex(kwoObject->GetContinuousValueAt(nNativeIndex));
		// Cas d'un groupage
		else if (sRuleLabel == groupingRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*,
				 predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetSymbolPartIndex(kwoObject->GetSymbolValueAt(nNativeIndex));

		assert(nModalityIndex != -1);

		// Memorisation de la valeur de la modalite pour la variable et l'individu
		ivModalityIndexes.SetAt(nAttributeIndex, nModalityIndex);
	}

	sContributionClass = GetOperandAt(2)->GetSymbolConstant();

	if (sContributionClass == PREDICTED_CLASS_LABEL)
	{
		predictedClassAttribute = kwcClass->LookupAttribute(GetSecondOperand()->GetAttributeName());
		sContributionClass = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
	}
	else
	{
		if (sContributionClass == CLASS_OF_HIGHEST_GAIN_LABEL)
		{
			// determiner quelle est la classe de plus grand gain

			cMaxGain = 0;

			// Calcul du vecteur de scores de l'individu
			cvScoreVector = ComputeScoreVectorLj(&ivModalityIndexes);

			for (nIndex = 0; nIndex < oaModelProbabilities.GetSize(); nIndex++)
			{
				targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nIndex));

				cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nIndex);

				cPriorProba = exp(targetValueProbas->dProbaApriori);

				// Calcul du gain : ratio entre le score et la proba a priori
				cGain = cInitialScore / cPriorProba;

				// Cas ou ce gain est le gain maximal rencontre : memorisation de la classe cible
				if (cGain > cMaxGain)
					sContributionClass = targetValueProbas->sTargetValue;
			}

			delete cvScoreVector;
		}
	}

	if (oaInstanceProbabilities != NULL)
		oaInstanceProbabilities->DeleteAll();
	else
	{
		oaInstanceProbabilities = new ObjectArray;
		oaInstanceProbabilities->SetCompareFunction(KICompareContributionImportanceValue);
	}
	oaInstanceProbabilities->SetSize(svPartitionedPredictiveAttributeNames.GetSize());

	// Extraction de l'index de la classe cible dans le tableau des probas
	sClassIndex = cast(StringObject*, odClassNamesIndexes.Lookup(sContributionClass));
	assert(sClassIndex != NULL);
	nClassIndex = (int)KWContinuous::StringToContinuous(sClassIndex->GetString());

	nClassNumber = oaModelProbabilities.GetSize();
	nDatabaseSize = 10000;

	// Parcours des variables contribuant au classifieur
	for (nAttributeIndex = 0; nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		partitionedAttributeProbas = new KIPartitionedAttributeProbas;

		partitionedAttributeProbas->nAttributeIndex = nAttributeIndex;
		partitionedAttributeProbas->nModalityIndex = ivModalityIndexes.GetAt(nAttributeIndex);

		// Calcul de l'indicateur d'importance

		cImportanceValue =
		    ComputeShapley(nAttributeIndex, nClassIndex, ivModalityIndexes.GetAt(nAttributeIndex));

		// Memorisation de la proba (valeur d'importance)
		assert(cImportanceValue != -1);
		partitionedAttributeProbas->dContributionImportanceValue = cImportanceValue;

		// Insertion du vecteur dans le tableau
		oaInstanceProbabilities->SetAt(nAttributeIndex, partitionedAttributeProbas);
	}

	// si demande, tri selon la proba a posteriori
	if (bSortInstanceProbas)
		oaInstanceProbabilities->Sort();
}

Continuous KIDRClassifierContribution::ComputeModalityProbability(int nAttributeIndex, int nTargetClassIndex,
								  int nModalityIndex) const
{
	Continuous cImportanceValue;

	cImportanceValue = exp(ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex));

	return cImportanceValue;
}

void KIDRClassifierContribution::InitializeShapleyTables()
{
	// nModalityIndex indique dans quel intervalle (ou groupe) de l'attribut designe par nAttributeIndex, cet individu appartient
	// nTargetClassIndex est la classe cible pour le calcul de l'importance

	Continuous cTerm1Numerator;
	Continuous cTerm1Denominator;
	Continuous cTerm1;
	Continuous cTerm2Numerator;
	Continuous cTerm2Denominator;
	Continuous cTerm2;
	Continuous cProbaModality;
	ALString sTarget;
	KIShapleyTable* stShapleyTable;
	KITargetValueProbas* targetValueProbas;
	ContinuousVector* cvVectorProbas;
	Continuous variableWeight;
	int nAttributeIndex;
	int nTargetClassIndex;
	int nModalityIndex;
	int nClassIndex;
	int nModality;
	int nTarget;
	int nVariableNumber = svPartitionedPredictiveAttributeNames.GetSize();
	int nClassNumber = svTargetValues.GetSize();
	boolean bLocalTrace = false;
	ContinuousVector cvShapeleyExpectedValueByTarget;
	Continuous cProbaForTarget;
	Continuous cImportanceValue;

	//clean oaShapleyTables
	if (oaShapleyTables.GetSize() > 0)
	{
		oaShapleyTables.DeleteAll();
		oaShapleyTables.SetSize(0);
	}
	//set size of  oaShapleyTables
	oaShapleyTables.SetSize(nVariableNumber);

	cvShapeleyExpectedValueByTarget.SetSize(nVariableNumber * nClassNumber);

	for (nAttributeIndex = 0; nAttributeIndex < nVariableNumber; nAttributeIndex++)
	{
		for (nTargetClassIndex = 0; nTargetClassIndex < nClassNumber; nTargetClassIndex++)
		{
			// 2eme terme : on somme sur tous les intervalles possibles de l'attribut
			cTerm2 = 0;

			// Extraction du tableau des probas pour la classe cible courante
			targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nTargetClassIndex));

			// Extraction du vecteur de probas pour l'attribut predictif
			cvVectorProbas =
			    cast(ContinuousVector*, targetValueProbas->oaProbasAposteriori->GetAt(nAttributeIndex));

			for (nModality = 0; nModality < cvVectorProbas->GetSize(); nModality++)
			{
				// P(In) : proba de tomber dans l'intervalle In, qque soit la classe C
				// P(In) = P(In|C1) * P(C1) + P(In|C2) * P(C2) + P(In|C3) * P(C3) + ....
				cProbaModality = 0;
				for (nTarget = 0; nTarget < ivTargetFrequencies.GetSize(); nTarget++)
				{
					cProbaForTarget = ivTargetFrequencies.GetAt(nTarget) / dTotalFrequency; // P(Cn)
					cProbaModality +=
					    (ComputeModalityProbability(nAttributeIndex, nTarget, nModality) *
					     cProbaForTarget); // P(In|Cn) * P(Cn)
				}

				// calcul de P(Xm=Xi | Y1) --> Y1 represente la valeur de reference
				cTerm2Numerator =
				    ComputeModalityProbability(nAttributeIndex, nTargetClassIndex, nModality);

				// calcul de P(Xm=Xi | Y0)  --> Y0 represente toutes les classes sauf la valeur de reference
				cTerm2Denominator = ComputeModalityProbabilityWithoutTargetClass(
				    nAttributeIndex, nTargetClassIndex, nModality);
				cTerm2 += (cProbaModality * log(cTerm2Numerator / cTerm2Denominator));

				if (bLocalTrace)
				{
					cout << "cProbaModality\t" << cProbaModality << endl;
					cout << "cTerm2Numerator\t" << cTerm2Numerator << endl;
					cout << "cTerm2Denominator\t" << cTerm2Denominator << endl;
				}
			}

			cvShapeleyExpectedValueByTarget.SetAt(nAttributeIndex * nClassNumber + nTargetClassIndex,
							      cTerm2);
		}
	}

	// Parcours des variables explicatives
	for (nAttributeIndex = 0; nAttributeIndex < nVariableNumber; nAttributeIndex++)
	{
		stShapleyTable = new KIShapleyTable;
		// Extraction du tableau des probas pour la classe cible courante
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(0));

		variableWeight = cvVariableWeights.GetAt(nAttributeIndex);

		// Extraction du vecteur de probas pour l'attribut predictif
		cvVectorProbas =
		    cast(ContinuousVector*, targetValueProbas->oaProbasAposteriori->GetAt(nAttributeIndex));
		stShapleyTable->Initialize(cvVectorProbas->GetSize(), nClassNumber);
		oaShapleyTables.SetAt(nAttributeIndex, stShapleyTable);

		for (nClassIndex = 0; nClassIndex < nClassNumber; nClassIndex++)
		{

			// Extraction du tableau des probas pour la classe cible courante
			targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

			// Extraction du vecteur de probas pour l'attribut predictif
			cvVectorProbas =
			    cast(ContinuousVector*, targetValueProbas->oaProbasAposteriori->GetAt(nAttributeIndex));

			for (nModalityIndex = 0; nModalityIndex < cvVectorProbas->GetSize(); nModalityIndex++)
			{
				//const Continuous variableWeight = cvVariableWeights.GetAt(nAttributeIndex);

				// calcul de P(Xm=Xi | Y1) --> Y1 represente la valeur de reference
				cTerm1Numerator =
				    ComputeModalityProbability(nAttributeIndex, nClassIndex, nModalityIndex);

				cTerm1Denominator = ComputeModalityProbabilityWithoutTargetClass(
				    nAttributeIndex, nClassIndex, nModalityIndex);
				cTerm2 = cvShapeleyExpectedValueByTarget.GetAt(
				    nAttributeIndex * ivTargetFrequencies.GetSize() + nClassIndex);
				cTerm1 = log(cTerm1Numerator / cTerm1Denominator);
				cImportanceValue = variableWeight * (cTerm1 - cTerm2);
				// affectation au tableau des valeur de Shapley
				stShapleyTable->SetShapleyValueAt(nModalityIndex, nClassIndex, cImportanceValue);
			}
		}
	}
}

Continuous KIDRClassifierContribution::ComputeShapley(const int nAttributeIndex, const int nTargetClassIndex,
						      const int nModalityIndex) const
{
	// nModalityIndex indique dans quel intervalle (ou groupe) de l'attribut designe par nAttributeIndex, cet individu appartient
	// nTargetClassIndex est la classe cible pour le calcul de l'importance

	KIShapleyTable* stShapleyTable;

	stShapleyTable = cast(KIShapleyTable*, oaShapleyTables.GetAt(nAttributeIndex));

	return stShapleyTable->GetShapleyValueAt(nModalityIndex, nTargetClassIndex);
}

Continuous KIDRClassifierContribution::ComputeModalityProbabilityWithoutTargetClass(int nAttributeIndex,
										    int nTargetClassIndex,
										    int nModalityIndex) const
{
	// sert au calcul de P(Xm=Xi | Y0)  --> Y0 represente toutes les classes sauf la valeur de reference

	Continuous cTargetProbas = 0;
	int iTargetFrequencies = 0;
	int nClassIndex;

	assert(ivTargetFrequencies.GetSize() > 0);
	assert(oaModelProbabilities.GetSize() > 0);

	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		if (nClassIndex != nTargetClassIndex)
		{
			cTargetProbas += (ComputeModalityProbability(nAttributeIndex, nClassIndex, nModalityIndex) *
					  ivTargetFrequencies.GetAt(nClassIndex));
			iTargetFrequencies += ivTargetFrequencies.GetAt(nClassIndex);
		}
	}
	return cTargetProbas / iTargetFrequencies;
}

Continuous KIDRClassifierInterpretation::ExtractLogPosteriorProba(int nClassIndex, int nAttributeIndex,
								  int nModalityIndex) const
{
	ContinuousVector* cvVector;

	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	KITargetValueProbas* targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

	// Extraction du vecteur de probas pour l'attribut predictif
	cvVector = cast(ContinuousVector*, targetValueProbas->oaProbasAposteriori->GetAt(nAttributeIndex));

	// On retourne la proba associee a la modalite pour cet attribut predictif
	require(nModalityIndex < cvVector->GetSize());
	return cvVector->GetAt(nModalityIndex);
}

boolean KIDRClassifierInterpretation::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRNBClassifier referenceNBRule;
	KWDRSNBClassifier referenceSNBRule;
	KWDRClassifier* classifierRule;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation pour verifier le type de classifier
	if (bOk)
	{
		classifierRule = cast(KWDRClassifier*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		if (classifierRule->GetName() != referenceNBRule.GetName() and
		    classifierRule->GetName() != referenceSNBRule.GetName())
		{
			bOk = false;
			AddError("Classifier " + classifierRule->GetName() + " in first operand must be " +
				 referenceNBRule.GetName() + " or " + referenceSNBRule.GetName());
		}
	}
	return bOk;
}

Continuous KIDRClassifierContribution::ComputeMaxLogPosteriorProbaWithoutWhyClassValue(int nWhyTargetValueNumber,
										       int nAttributeIndex,
										       int nModalityIndex) const
{
	Continuous cLogProbaMax;
	int nClassIndex;

	cLogProbaMax = -100;

	// Parcours des valeurs cible
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Cas d'une valeur differente de la valeur de reference
		if (nClassIndex != nWhyTargetValueNumber)
		{
			// Mise a jour du max
			cLogProbaMax =
			    max(cLogProbaMax, ExtractLogPosteriorProba(nClassIndex, nAttributeIndex, nModalityIndex));
		}
	}

	return cLogProbaMax;
}

void KIDRClassifierContribution::Compile(KWClass* kwcOwnerClass)
{
	KIDRClassifierInterpretation::Compile(kwcOwnerClass); // code classe ancetre

	// voir si on doit trier les variables de contribution et reinforcement
	bSortInstanceProbas = (strcmp(GetOperandAt(4)->GetSymbolConstant().GetValue(), "sorted") == 0 ? true : false);

	sContributionClass = "";

	InitializeShapleyTables();
}

ContinuousVector* KIDRClassifierContribution::ComputeScoreVectorLj(IntVector* ivModalityIndexes) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;
	KITargetValueProbas* targetValueProbas;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->dProbaApriori);

		// Parcours des variables
		for (nAttributeIndex = 0; nAttributeIndex < ivModalityIndexes->GetSize(); nAttributeIndex++)
		{
			cvScore->SetAt(nClassIndex,
				       cvScore->GetAt(nClassIndex) +
					   (cvVariableWeights.GetAt(nAttributeIndex) *
					    ExtractLogPosteriorProba(nClassIndex, nAttributeIndex,
								     ivModalityIndexes->GetAt(nAttributeIndex))));
		}
	}

	return cvScore;
}

ContinuousVector* KIDRClassifierContribution::ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
										     int nVariableIndex) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;
	KITargetValueProbas* targetValueProbas;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->dProbaApriori);

		// Parcours des variables
		for (nAttributeIndex = 0; nAttributeIndex < ivModalityIndexes->GetSize(); nAttributeIndex++)
		{
			// Cas ou il ne s'agit pas de la variable a exclure
			if (nAttributeIndex != nVariableIndex)
				cvScore->SetAt(nClassIndex, cvScore->GetAt(nClassIndex) +
								cvVariableWeights.GetAt(nAttributeIndex) *
								    ExtractLogPosteriorProba(
									nClassIndex, nAttributeIndex,
									ivModalityIndexes->GetAt(nAttributeIndex)));
		}
	}

	return cvScore;
}

Continuous KIDRClassifierContribution::ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector,
								   int nReferenceClassIndex) const
{
	Continuous cScore;
	int nClassIndex;

	cScore = 0;

	// Parcours des classes cible
	for (nClassIndex = 0; nClassIndex < cvScoreVector->GetSize(); nClassIndex++)
	{
		// Calcul de la somme
		cScore += exp(cvScoreVector->GetAt(nClassIndex) - cvScoreVector->GetAt(nReferenceClassIndex));
	}
	// Calcul de l'inverse de la somme
	cScore = 1.0 / cScore;

	return cScore;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
KIDRContributionValueAt::KIDRContributionValueAt()
{
	SetName("ContributionValueAt");
	SetLabel("Variable importance value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats de la contribution au score, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreContribution");
	// rang d'importance de la variable pour laquelle ont veut obtenir les resultats
	GetSecondOperand()->SetType(KWType::Continuous);
}

KIDRContributionValueAt::~KIDRContributionValueAt() {}

KWDerivationRule* KIDRContributionValueAt::Create() const
{
	return new KIDRContributionValueAt;
}

Continuous KIDRContributionValueAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierContribution* scoreInterpretation =
	    cast(KIDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionValueAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRContributionNameAt

KIDRContributionNameAt::KIDRContributionNameAt()
{
	SetName("ContributionNameAt");
	SetLabel("Variable importance name");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats de la contribution au score, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreContribution");
	// rang d'importance de la variable pour laquelle ont veut obtenir les resultats
	GetSecondOperand()->SetType(KWType::Continuous);
}
KIDRContributionNameAt::~KIDRContributionNameAt() {}

KWDerivationRule* KIDRContributionNameAt::Create() const
{
	return new KIDRContributionNameAt;
}

Symbol KIDRContributionNameAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierContribution* scoreInterpretation =
	    cast(KIDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionNameAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRContributionClass
KIDRContributionClass::KIDRContributionClass()
{
	SetName("ContributionClass");
	SetLabel("Contribution class");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats de la contribution au score, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreContribution");
}

KIDRContributionClass::~KIDRContributionClass() {}

KWDerivationRule* KIDRContributionClass::Create() const
{
	return new KIDRContributionClass;
}

Symbol KIDRContributionClass::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierContribution* scoreInterpretation;

	scoreInterpretation = cast(KIDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionClass();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRContributionPartitionAt
KIDRContributionPartitionAt::KIDRContributionPartitionAt()
{
	SetName("ContributionPartitionAt");
	SetLabel("Variable importance partition");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats de la contribution au score, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreContribution");
	// rang d'importance de la variable pour laquelle ont veut obtenir les resultats
	GetSecondOperand()->SetType(KWType::Continuous);
}

KIDRContributionPartitionAt::~KIDRContributionPartitionAt() {}

KWDerivationRule* KIDRContributionPartitionAt::Create() const
{
	return new KIDRContributionPartitionAt;
}

Symbol KIDRContributionPartitionAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierContribution* scoreInterpretation;
	scoreInterpretation = cast(KIDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionPartitionAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
							       1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRContributionPriorClass
KIDRContributionPriorClass::KIDRContributionPriorClass()
{
	SetName("ContributionPriorClass");
	SetLabel("Prior for class contribution");
	SetType(KWType::Continuous);
	SetOperandNumber(4);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGrid");
	GetSecondOperand()->SetType(KWType::Symbol);
	// classe predite (attribut de prediction)
	GetOperandAt(2)->SetType(KWType::Symbol);
	// classe de la contribution (peut etre la classe de meilleur gain pour cet individu)
	GetOperandAt(3)->SetType(KWType::Symbol);
	nTotalFrequency = 0;
}

KIDRContributionPriorClass::~KIDRContributionPriorClass() {}

KWDerivationRule* KIDRContributionPriorClass::Create() const
{
	return new KIDRContributionPriorClass;
}

void KIDRContributionPriorClass::Compile(KWClass* kwcOwnerClass)
{
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;
	int nTarget;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	targetDataGrid = cast(const KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetDataGrid->GetAttributeNumber() == 1);
	assert(targetDataGrid->GetAttributeTypeAt(0) == KWType::Symbol);
	int nTargetValueNumber = targetDataGrid->GetTotalCellNumber();

	// Initialisation du vecteur des valeurs cibles
	targetSymbolValueSet = cast(const KWDRSymbolValueSet*,
				    targetDataGrid->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetSymbolValueSet->GetValueNumber() == nTargetValueNumber);
	svTargetValues.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		svTargetValues.SetAt(nTarget, targetSymbolValueSet->GetValueAt(nTarget));

	nTotalFrequency = 0;

	// Memorisation des effectifs par partie cible
	ivDataGridSetTargetFrequencies.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		ivDataGridSetTargetFrequencies.SetAt(nTarget, targetDataGrid->GetCellFrequencyAt(nTarget));
		nTotalFrequency += targetDataGrid->GetCellFrequencyAt(nTarget);
	}
}

Continuous KIDRContributionPriorClass::ComputeContinuousResult(const KWObject* kwoObject) const
{

	int nIndex;
	Symbol starget;
	KWAttribute* predictedClassAttribute;
	KWAttribute* contributiobnClassAttribute;

	if (nTotalFrequency == 0)
		return 0;

	starget = GetSecondOperand()->GetSymbolValue(kwoObject);

	if (starget == PREDICTED_CLASS_LABEL)
	{
		predictedClassAttribute = kwcClass->LookupAttribute(GetOperandAt(2)->GetAttributeName());
		starget = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
	}
	else
	{
		if (starget == CLASS_OF_HIGHEST_GAIN_LABEL)
		{
			contributiobnClassAttribute = kwcClass->LookupAttribute(GetOperandAt(3)->GetAttributeName());
			starget = contributiobnClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
		}
	}

	for (nIndex = 0; nIndex < svTargetValues.GetSize(); nIndex++)
	{
		if (starget == svTargetValues.GetAt(nIndex))
		{
			return (double)ivDataGridSetTargetFrequencies.GetAt(nIndex) / (double)nTotalFrequency;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierReinforcement
KIDRClassifierReinforcement::KIDRClassifierReinforcement()
{
	SetName("ScoreReinforcement");
	SetLabel("Classifier score contribution");
	SetType(KWType::Structure);
	SetStructureName("ScoreReinforcement");
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	// classifieur
	GetFirstOperand()->SetStructureName("Classifier");
	// classe predite pour l'individu (via l'attribut de prediction)
	GetSecondOperand()->SetType(KWType::Symbol);
	// classe cible pour le calcul du renforcement
	GetOperandAt(2)->SetType(KWType::Symbol);
	cInitialScore = 0.0;
	nTargetValuesNumberInNBScore = 0;
	oaInstanceProbabilities = NULL;
}

KIDRClassifierReinforcement::~KIDRClassifierReinforcement() {}

KWDerivationRule* KIDRClassifierReinforcement::Create() const
{
	return new KIDRClassifierReinforcement;
}

Object* KIDRClassifierReinforcement::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// evaluer l'operande classifieur
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Calcul des donnees d'interpretation
	ComputeReinforcement(kwoObject);

	return (Object*)this;
}

Continuous KIDRClassifierReinforcement::GetReinforcementInitialScore() const
{
	assert(oaInstanceProbabilities != NULL);
	// pas de reenforcement pour cet individu
	if (oaInstanceProbabilities->GetSize() == 0)
		return 0;

	return cInitialScore;
}

Continuous KIDRClassifierReinforcement::GetReinforcementFinalScoreAt(int rank) const
{
	assert(oaInstanceProbabilities != NULL);
	// pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0;

	KIPartitionedAttributeProbas* attributeProbas =
	    cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->dReinforcementNewScore;
}

Continuous KIDRClassifierReinforcement::GetReinforcementClassChangeTagAt(int rank) const
{
	assert(oaInstanceProbabilities != NULL);
	// pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0;

	KIPartitionedAttributeProbas* attributeProbas =
	    cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->dReinforcementClassHasChanged;
}

Symbol KIDRClassifierReinforcement::GetReinforcementNameAt(int rank) const
{
	assert(oaInstanceProbabilities != NULL);
	// pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return "";

	KIPartitionedAttributeProbas* attributeProbas =
	    cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	ALString as = svPartitionedPredictiveAttributeNames.GetAt(attributeProbas->nAttributeIndex);
	return as.GetBuffer(as.GetLength());
}

Symbol KIDRClassifierReinforcement::GetReinforcementPartitionAt(int rank) const
{
	Symbol attributeName;
	KWAttribute* attribute;
	KWDataGridStats* dataGridStats;
	ostringstream oss;

	assert(oaInstanceProbabilities != NULL);

	// pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return "";

	KIPartitionedAttributeProbas* attributeProbas =
	    cast(KIPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));

	attributeName = GetReinforcementNameAt(rank);

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	attribute = kwcClass->LookupAttribute(attributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient
	// l'individu pour cette variable
	dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);

	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbas->nModalityIndex);
	delete dataGridStats;

	return oss.str().c_str();
}

void KIDRClassifierReinforcement::ComputeReinforcement(const KWObject* kwoObject) const
{

	int nAttributeIndex;
	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	IntVector ivModalityIndexes;
	KWAttribute* predictiveAttribute;
	KWLoadIndex nNativeIndex;
	ALString sRuleLabel;
	Symbol sReinforcementClass;
	KWAttribute* predictedClassAttribute;
	Symbol sPredictedClass;
	ContinuousVector* cvScoreVector;

	assert(IsCompiled());
	ivModalityIndexes.SetSize(svPartitionedPredictiveAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable predictive, l'index de la partie (valeur variable partionnee) pour l'individu courant
	for (nAttributeIndex = 0; nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		predictiveAttribute =
		    kwcClass->LookupAttribute(svPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex));

		// Extraction de l'index de l'attribut natif
		nNativeIndex =
		    kwcClass->LookupAttribute(svNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetLoadIndex();

		int nModalityIndex = -1;

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de groupage
		sRuleLabel =
		    predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule()->GetLabel();

		// Cas d'une discretisation
		if (sRuleLabel == discretizationRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*,
				 predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetContinuousPartIndex(kwoObject->GetContinuousValueAt(nNativeIndex));
		// Cas d'un groupage
		else if (sRuleLabel == groupingRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*,
				 predictiveAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetSymbolPartIndex(kwoObject->GetSymbolValueAt(nNativeIndex));

		assert(nModalityIndex != -1);

		// Memorisation de la valeur de la modalite pour la variable et l'individu
		ivModalityIndexes.SetAt(nAttributeIndex, nModalityIndex);
	}

	sReinforcementClass = GetOperandAt(2)->GetSymbolConstant();

	predictedClassAttribute = kwcClass->LookupAttribute(GetSecondOperand()->GetAttributeName());
	sPredictedClass = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);

	// Calcul du vecteur de scores de l'individu
	cvScoreVector = ComputeScoreVectorLj(&ivModalityIndexes);

	if (oaInstanceProbabilities != NULL)
		oaInstanceProbabilities->DeleteAll();
	else
	{
		oaInstanceProbabilities = new ObjectArray;
		oaInstanceProbabilities->SetCompareFunction(KICompareReinforcementNewScore);
	}

	// Calcul du score associe pour la classe de reenforcement
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetValuesNumberInNBScore);

	// Calcul du tableau des valeurs d'amelioration de score
	ComputeReinforcementProbas(&ivModalityIndexes, sPredictedClass, cvScoreVector, nTargetValuesNumberInNBScore);

	delete cvScoreVector;
}

void KIDRClassifierReinforcement::ComputeReinforcementProbas(IntVector* ivModalityIndexes, Symbol sPredictedClass,
							     ContinuousVector* cvBestScore, int nHowNumber) const
{
	KWAttribute* attribute;
	KWAttribute* partitionedAttribute;
	KIPartitionedAttributeProbas* partitionedAttributeProbas;
	ContinuousVector* cvScore;
	Continuous cBestScore;
	Continuous cScore;
	Continuous cMaxScore;
	Continuous cNewPredictedClassIsHowReferenceClass = -1;
	ALString sNativeVariableName;
	ALString sPartitionedVariableName;
	int nAttributeIndex;
	int nBestModalityIndex = -1;
	int nModalityNumber;
	int nModalityIndex;
	int nClassIndex;
	KITargetValueProbas* targetValueProbas;

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() == 0);

	targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nHowNumber));

	// Parcours des variables natives contribuant au NB a interpreter
	for (nAttributeIndex = 0; nAttributeIndex < svNativePredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction du nom de la variable native
		sNativeVariableName = svNativePredictiveAttributeNames.GetAt(nAttributeIndex);

		// Extraction de l'attribut natif du dictionnaire
		attribute = kwcClass->LookupAttribute(sNativeVariableName);

		// Cas ou cette variable est une variable levier selectionnee
		if (attribute->GetConstMetaData()->GetStringValueAt(LEVER_ATTRIBUTE_META_TAG) == "true")
		{
			// Extraction du nom de la variable explicative
			sPartitionedVariableName = svPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex);

			// Extraction de l'attribut explicatif
			partitionedAttribute = kwcClass->LookupAttribute(sPartitionedVariableName);

			// Calcul du nombre de modalites de l'attribut
			nModalityNumber =
			    cast(KWDRUnivariatePartition*,
				 partitionedAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetPartNumber();

			// Initialisation du score a ameliorer
			cBestScore = cInitialScore;

			// Parcours des modalites
			for (nModalityIndex = 0; nModalityIndex < nModalityNumber; nModalityIndex++)
			{
				if (ivModalityIndexes->GetAt(nAttributeIndex) != nModalityIndex)
				{
					// Calcul du nouveau vecteur de scores
					cvScore = ComputeScoreVectorVariation(cvBestScore, nAttributeIndex,
									      ivModalityIndexes->GetAt(nAttributeIndex),
									      nModalityIndex);
					cScore = ComputeScoreFromScoreVector(cvScore, nHowNumber);

					// Cas de l'amelioration du score
					if (cScore > cBestScore)
					{
						// Memorisation de l'index de la modalite et du score
						nBestModalityIndex = nModalityIndex;
						cBestScore = cScore;

						// Calcul d'un tag indiquant le changement de classe eventuel qu'impliquerait le renforcement
						// Cas ou la classe predite pour cet individu est deja la classe de reference

						if (sPredictedClass == targetValueProbas->sTargetValue)
							cNewPredictedClassIsHowReferenceClass = 0;
						else
						{
							// Calcul du max des nouveaux scores pour les autres classes que la classe de reference
							// Initialisation du max des scores conditionnellement aux autres classes
							cMaxScore = 0;

							// Parcours des classes
							for (nClassIndex = 0;
							     nClassIndex < oaModelProbabilities.GetSize();
							     nClassIndex++)
							{
								// Cas d'une classe autre que la classe de reference
								if (nClassIndex != nHowNumber)
								{
									// Mise a jour du max
									cMaxScore =
									    max(cMaxScore, ComputeScoreFromScoreVector(
											       cvScore, nClassIndex));
								}
							}

							// Cas ou l'argmax des nouveaux score est la classe de reference
							// Alors le renforcement permet le changement de classe pour la classe de reference
							if (KWContinuous::CompareIndicatorValue(cScore, cMaxScore) > 0)
								cNewPredictedClassIsHowReferenceClass = 1;

							// Sinon : pas de changement de classe ou vers une autre classe que la classe de reference
							else
								cNewPredictedClassIsHowReferenceClass = -1;
						}
					}
					// Nettoyage du vecteur de scores
					delete cvScore;
				}
			}
			// Cas d'une amelioration pour une des modalites de la variable
			if (cBestScore > cInitialScore)
			{
				partitionedAttributeProbas = new KIPartitionedAttributeProbas;

				partitionedAttributeProbas->nAttributeIndex = nAttributeIndex;

				assert(nBestModalityIndex != -1);
				partitionedAttributeProbas->nModalityIndex = nBestModalityIndex;

				partitionedAttributeProbas->dReinforcementNewScore = cBestScore;

				// Memorisation du tag indiquant si la nouvelle classe predite est la classe de reference du pourquoi
				partitionedAttributeProbas->dReinforcementClassHasChanged =
				    cNewPredictedClassIsHowReferenceClass;

				oaInstanceProbabilities->Add(partitionedAttributeProbas);
			}
		}
	}
	// Tri selon le score
	oaInstanceProbabilities->Sort();
}

ContinuousVector* KIDRClassifierReinforcement::ComputeScoreVectorVariation(ContinuousVector* cvPreviousScoreVector,
									   int nAttributeIndex,
									   int nPreviousModalityIndex,
									   int nNewModalityIndex) const
{
	ContinuousVector* cvNewScoreVector;
	int nClassIndex;

	cvNewScoreVector = new ContinuousVector;

	// Parcours des classes cible
	for (nClassIndex = 0; nClassIndex < cvPreviousScoreVector->GetSize(); nClassIndex++)
	{
		// Mise a jour de la somme des log probas
		// De la forme
		// -> Add(P - W * ((OLD) - (NEW)))
		cvNewScoreVector->Add(
		    cvPreviousScoreVector->GetAt(nClassIndex) -
		    cvVariableWeights.GetAt(nAttributeIndex) *
			(ExtractLogPosteriorProba(nClassIndex, nAttributeIndex, nPreviousModalityIndex) -
			 ExtractLogPosteriorProba(nClassIndex, nAttributeIndex, nNewModalityIndex)));
	}

	return cvNewScoreVector;
}

void KIDRClassifierReinforcement::Compile(KWClass* kwcOwnerClass)
{
	KIDRClassifierInterpretation::Compile(kwcOwnerClass); // code classe ancetre
	int nClassIndex;
	Symbol sReinforcementClass;
	KITargetValueProbas* targetValueProbas;

	cInitialScore = 0;

	nTargetValuesNumberInNBScore = -1;

	sReinforcementClass = GetOperandAt(2)->GetSymbolConstant();

	// Parcours des valeurs cible
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

		if (targetValueProbas->sTargetValue == sReinforcementClass)
		{
			// Memorisation de l'index du vecteur de probas pour cette classe
			nTargetValuesNumberInNBScore = nClassIndex;
			break;
		}
	}

	assert(nTargetValuesNumberInNBScore != -1);
}

ContinuousVector* KIDRClassifierReinforcement::ComputeScoreVectorLj(IntVector* ivModalityIndexes) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;
	KITargetValueProbas* targetValueProbas;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->dProbaApriori);

		// Parcours des variables
		for (nAttributeIndex = 0; nAttributeIndex < ivModalityIndexes->GetSize(); nAttributeIndex++)
		{
			cvScore->SetAt(nClassIndex,
				       cvScore->GetAt(nClassIndex) +
					   (cvVariableWeights.GetAt(nAttributeIndex) *
					    ExtractLogPosteriorProba(nClassIndex, nAttributeIndex,
								     ivModalityIndexes->GetAt(nAttributeIndex))));
		}
	}

	return cvScore;
}

ContinuousVector* KIDRClassifierReinforcement::ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
										      int nVariableIndex) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;
	KITargetValueProbas* targetValueProbas;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->dProbaApriori);

		// Parcours des variables
		for (nAttributeIndex = 0; nAttributeIndex < ivModalityIndexes->GetSize(); nAttributeIndex++)
		{
			// Cas ou il ne s'agit pas de la variable a exclure
			if (nAttributeIndex != nVariableIndex)
				cvScore->SetAt(nClassIndex, cvScore->GetAt(nClassIndex) +
								cvVariableWeights.GetAt(nAttributeIndex) *
								    ExtractLogPosteriorProba(
									nClassIndex, nAttributeIndex,
									ivModalityIndexes->GetAt(nAttributeIndex)));
		}
	}

	return cvScore;
}

Continuous KIDRClassifierReinforcement::ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector,
								    int nReferenceClassIndex) const
{
	Continuous cScore;
	int nClassIndex;

	cScore = 0;

	// Parcours des classes cible
	for (nClassIndex = 0; nClassIndex < cvScoreVector->GetSize(); nClassIndex++)
	{
		// Calcul de la somme
		cScore += exp(cvScoreVector->GetAt(nClassIndex) - cvScoreVector->GetAt(nReferenceClassIndex));
	}
	// Calcul de l'inverse de la somme
	cScore = 1.0 / cScore;

	return cScore;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
KIDRReinforcementInitialScore::KIDRReinforcementInitialScore()
{
	SetName("ReinforcementInitialScore");
	SetLabel("Reinforcement initial score");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats du renforcement, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreReinforcement");
}

KIDRReinforcementInitialScore::~KIDRReinforcementInitialScore() {}

KWDerivationRule* KIDRReinforcementInitialScore::Create() const
{
	return new KIDRReinforcementInitialScore;
}

Continuous KIDRReinforcementInitialScore::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcement* scoreReinforcement =
	    cast(KIDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementInitialScore();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementFinalScoreAt
KIDRReinforcementFinalScoreAt::KIDRReinforcementFinalScoreAt()
{
	SetName("ReinforcementFinalScoreAt");
	SetLabel("Reinforcement final score");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats du renforcement, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreReinforcement");
	// rang d'importance de la variable pour laquelle ont veut obtenir les resultats
	GetSecondOperand()->SetType(KWType::Continuous);
}

KIDRReinforcementFinalScoreAt::~KIDRReinforcementFinalScoreAt() {}

KWDerivationRule* KIDRReinforcementFinalScoreAt::Create() const
{
	return new KIDRReinforcementFinalScoreAt;
}

Continuous KIDRReinforcementFinalScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcement* scoreReinforcement =
	    cast(KIDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementFinalScoreAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
								1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementNameAt

KIDRReinforcementNameAt::KIDRReinforcementNameAt()
{
	SetName("ReinforcementNameAt");
	SetLabel("Variable reinforcement name");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreReinforcement"); // resultats du renforcement, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

KIDRReinforcementNameAt::~KIDRReinforcementNameAt() {}

KWDerivationRule* KIDRReinforcementNameAt::Create() const
{
	return new KIDRReinforcementNameAt;
}

Symbol KIDRReinforcementNameAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcement* scoreReinforcement =
	    cast(KIDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementNameAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementPartitionAt

KIDRReinforcementPartitionAt::KIDRReinforcementPartitionAt()
{
	SetName("ReinforcementPartitionAt");
	SetLabel("Variable reinforcement partition");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreReinforcement"); // resultats du renforcement, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

KIDRReinforcementPartitionAt::~KIDRReinforcementPartitionAt() {}

KWDerivationRule* KIDRReinforcementPartitionAt::Create() const
{
	return new KIDRReinforcementPartitionAt;
}

Symbol KIDRReinforcementPartitionAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcement* scoreReinforcement =
	    cast(KIDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementPartitionAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
							       1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementClassChangeTagAt

KIDRReinforcementClassChangeTagAt::KIDRReinforcementClassChangeTagAt()
{
	SetName("ReinforcementClassChangeTagAt");
	SetLabel("Variable reinforcement class change tag");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreReinforcement"); // resultats du renforcement, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

KIDRReinforcementClassChangeTagAt::~KIDRReinforcementClassChangeTagAt() {}

KWDerivationRule* KIDRReinforcementClassChangeTagAt::Create() const
{
	return new KIDRReinforcementClassChangeTagAt;
}

Continuous KIDRReinforcementClassChangeTagAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcement* scoreReinforcement =
	    cast(KIDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementClassChangeTagAt(
	    (int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}
