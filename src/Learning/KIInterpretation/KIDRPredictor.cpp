// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRPredictor.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KIPartitionedAttributeProbs
KIPartitionedAttributeProbs::KIPartitionedAttributeProbs()
{
	nAttributeIndex = 0;
	nModalityIndex = 0;
	cReinforcementNewScore = 0;
	nReinforcementClassHasChanged = 0;
	cContributionImportanceValue = 0;
}

KIPartitionedAttributeProbs::~KIPartitionedAttributeProbs() {}

int KIPartitionedAttributeProbsCompareReinforcementNewScore(const void* elem1, const void* elem2)
{
	int nCompare;
	KIPartitionedAttributeProbs* dataAttribute1;
	KIPartitionedAttributeProbs* dataAttribute2;

	// Acces aux objets
	dataAttribute1 = cast(KIPartitionedAttributeProbs*, *(Object**)elem1);
	dataAttribute2 = cast(KIPartitionedAttributeProbs*, *(Object**)elem2);
	assert(dataAttribute1->Check());
	assert(dataAttribute2->Check());

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(dataAttribute1->GetReinforcementNewScore(),
							dataAttribute2->GetReinforcementNewScore());

	// Comparaison sur l'index de l'attribut nom en cas d'egalite du level (sort value)
	if (nCompare == 0)
		nCompare = dataAttribute1->GetAttributeIndex() - dataAttribute2->GetAttributeIndex();
	return nCompare;
}

int KIPartitionedAttributeProbsCompareContributionImportanceValue(const void* elem1, const void* elem2)
{
	int nCompare;
	KIPartitionedAttributeProbs* dataAttribute1;
	KIPartitionedAttributeProbs* dataAttribute2;

	// Acces aux objets
	dataAttribute1 = cast(KIPartitionedAttributeProbs*, *(Object**)elem1);
	dataAttribute2 = cast(KIPartitionedAttributeProbs*, *(Object**)elem2);
	assert(dataAttribute1->Check());
	assert(dataAttribute2->Check());

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(dataAttribute1->GetContributionImportanceValue(),
							dataAttribute2->GetContributionImportanceValue());

	// Comparaison sur l'index de l'attribut nom en cas d'egalite du level (sort value)
	if (nCompare == 0)
		nCompare = dataAttribute1->GetAttributeIndex() - dataAttribute2->GetAttributeIndex();
	return nCompare;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KITargetValueLogProbs
KITargetValueLogProbs::KITargetValueLogProbs()
{
	cTargetLogProb = 0;
}

KITargetValueLogProbs::~KITargetValueLogProbs()
{
	oaAttributeSourceConditionalLogProbs.DeleteAll();
}

void KITargetValueLogProbs::Write(ostream& ost) const
{
	ContinuousVector* cvPartSourceConditionalLogProbs;
	int nAttribute;
	int nPart;

	ost << "KITargetValueLogProbs pour target " << sTargetValue << " : ";
	ost << "\tTarget prob : " << exp(GetTargetLogProb()) << endl;
	ost << "\tTarget conditional prob per variable : " << endl;
	for (nAttribute = 0; nAttribute < oaAttributeSourceConditionalLogProbs.GetSize(); nAttribute++)
	{
		cvPartSourceConditionalLogProbs =
		    cast(ContinuousVector*, oaAttributeSourceConditionalLogProbs.GetAt(nAttribute));
		ost << "\tVariable " << nAttribute << " :\t";
		for (nPart = 0; nPart < cvPartSourceConditionalLogProbs->GetSize(); nPart++)
			ost << exp(cvPartSourceConditionalLogProbs->GetAt(nPart)) << ", ";
		ost << endl;
	}
}

longint KITargetValueLogProbs::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KITargetValueLogProbs);
	lUsedMemory += oaAttributeSourceConditionalLogProbs.GetOverallUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierInterpretation
KIDRClassifierInterpretation::KIDRClassifierInterpretation()
{
	oaInstanceProbabilities = NULL;
	lTotalFrequency = 0;
}

KIDRClassifierInterpretation::~KIDRClassifierInterpretation()
{
	Clean();
}

void KIDRClassifierInterpretation::Clean()
{
	oaModelProbabilities.DeleteAll();
	oaModelProbabilities.SetSize(0);
	svPredictorAttributeNames.SetSize(0);
	svPredictorPartitionedAttributeNames.SetSize(0);
	lnkdClassNamesIndexes.DeleteAll();
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
	const KWDRSNBClassifier referenceSNBRule;
	const KWDRNBClassifier referenceNBRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KITargetValueLogProbs* targetValueLogProbs;
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;
	int nTargetValueNumber;
	int nFirstOperandIndex;
	boolean bIsSNB = false;
	boolean bIsNB = false;
	ALString sAttributeName;
	int nTargetFrequency;
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
	ContinuousVector* cvSourceConditionalLogProbs;
	const KWDRDataGridStats* dataGridStats;
	nFirstOperandIndex = 0;

	// Nettoyage prealable
	Clean();

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

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
	classifier->ExportAttributeNames(kwcOwnerClass, &svPredictorAttributeNames,
					 &svPredictorPartitionedAttributeNames);

	// Cas d'un NB on met tous les poid a 1.0
	if (bIsNB)
	{
		for (nIndex = 0; nIndex < svPredictorAttributeNames.GetSize(); nIndex++)
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
		targetValueLogProbs = new KITargetValueLogProbs;

		targetValueLogProbs->SetTargetValue(svTargetValues.GetAt(nClassIndex));

		// Parcours des variables explicatives
		for (nAttributeIndex = 0; nAttributeIndex < svPredictorPartitionedAttributeNames.GetSize();
		     nAttributeIndex++)
		{
			// Creation du vecteur qui accueillera :
			// les logs des probas a posteriori de la classe pour chaque partie de la variable explicative
			cvSourceConditionalLogProbs = new ContinuousVector;
			targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->Add(cvSourceConditionalLogProbs);

			// Extraction de l'attribut explicatif courant
			attribute =
			    kwcClass->LookupAttribute(svPredictorPartitionedAttributeNames.GetAt(nAttributeIndex));

			// Initialisation de la taille du vecteur de proba de l'attribut
			cvSourceConditionalLogProbs->SetSize(
			    cast(KWDRDataGrid*, attribute->GetDerivationRule())->GetAttributePartNumberAt(0));
		}

		// Memorisation du lien entre le nom de la classe cible et l'index de la ligne correspondante
		// dans le tableau des probas
		assert(lnkdClassNamesIndexes.Lookup(targetValueLogProbs->GetTargetValue().GetNumericKey()) == 0);
		lnkdClassNamesIndexes.SetAt(targetValueLogProbs->GetTargetValue().GetNumericKey(),
					    oaModelProbabilities.GetSize());

		// Ajout du vecteur associe a la classe cible
		oaModelProbabilities.Add(targetValueLogProbs);
	}

	if (bIsNB or bIsSNB)
	{
		// Extraction des probas a partir de la RDD l'attribut NB predicteur
		// Calcul de l'effectif global
		lTotalFrequency = 0;
		ivTargetFrequencies.SetSize(classifier->GetDataGridSetTargetPartNumber());
		for (nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			nTargetFrequency = classifier->GetDataGridSetTargetFrequencyAt(nClassIndex);
			assert(nTargetFrequency > 0);
			ivTargetFrequencies.SetAt(nClassIndex, nTargetFrequency);
			lTotalFrequency += nTargetFrequency;
		}
		assert(lTotalFrequency > 0);

		// Calcul des logarithme de probabilites des valeurs cibles
		for (nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			// Extraction du tableau des probas de cette valeur cible
			targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));

			// Initialisation avec le prior
			assert(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) > 0);
			targetValueLogProbs->SetTargetLogProb(
			    log(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) / (double)lTotalFrequency));

			// Ajout des probabilites conditionnelles par grille
			nDataGrid = 0;
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

						cvSourceConditionalLogProbs = cast(
						    ContinuousVector*,
						    targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetAt(
							nDataGrid));
						cvSourceConditionalLogProbs->SetAt(nSourceIndex, cTargetLogProb);
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

						// Mise a jour du terme de probabilite, en prenant en compte le poids de la grille
						for (nSourceIndex = 0;
						     nSourceIndex < dataGridStats->GetDataGridSourceCellNumber();
						     nSourceIndex++)
						{
							// Extraction de la log proba
							cTargetLogProb =
							    dataGridStats->GetDataGridSourceConditionalLogProbAt(
								nSourceIndex, nTargetIndex);
							cvSourceConditionalLogProbs = cast(
							    ContinuousVector*,
							    targetValueLogProbs->GetAttributeSourceConditionalLogProbs()
								->GetAt(nDataGrid));
							cvSourceConditionalLogProbs->SetAt(nSourceIndex,
											   cTargetLogProb);
						}
						// Mise-a-jour du compteur de grilles
						nDataGrid++;
					}
				}
			}
			ensure(nDataGrid == targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetSize());
		}
	}
}

longint KIDRClassifierInterpretation::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KIDRClassifierInterpretation);
	lUsedMemory += lnkdClassNamesIndexes.GetOverallUsedMemory();
	lUsedMemory += oaModelProbabilities.GetOverallUsedMemory();
	lUsedMemory += svPredictorAttributeNames.GetUsedMemory();
	lUsedMemory += svPredictorPartitionedAttributeNames.GetUsedMemory();
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
	KIPartitionedAttributeProbs* attributeProbas;

	require(oaInstanceProbabilities != NULL);
	require(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->GetContributionImportanceValue();
}

Symbol KIDRClassifierContribution::GetContributionNameAt(int rank) const
{
	KIPartitionedAttributeProbs* attributeProbas;
	Symbol sName;

	require(oaInstanceProbabilities != NULL);
	require(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	sName = svPredictorPartitionedAttributeNames.GetAt(attributeProbas->GetAttributeIndex());
	return sName;
}

Symbol KIDRClassifierContribution::GetContributionPartitionAt(int rank) const
{
	KIPartitionedAttributeProbs* attributeProbas;
	Symbol sAttributeName;
	KWAttribute* attribute;
	KWDataGridStats* dataGridStats;
	ostringstream oss;

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));

	sAttributeName = GetContributionNameAt(rank);

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	attribute = kwcClass->LookupAttribute(sAttributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient l'individu pour cette variable
	dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);

	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbas->GetModalityIndex());
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
	KIPartitionedAttributeProbs* partitionedAttributeProbs;
	Continuous cImportanceValue = -1;
	int nAttributeIndex;
	int nClassIndex;
	int nIndex;
	KITargetValueLogProbs* targetValueLogProbs;
	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	IntVector ivModalityIndexes;
	Continuous cInitialScore;
	Continuous cGain;
	Continuous cMaxGain;
	Continuous cPriorProba;
	ContinuousVector* cvScoreVector;
	KWAttribute* predictorPartitionedAttribute;
	KWAttribute* predictedClassAttribute;
	KWLoadIndex liIndex;
	ALString sRuleLabel;
	int nModalityIndex;

	require(IsCompiled());

	ivModalityIndexes.SetSize(svPredictorPartitionedAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable du predicteur, l'index de la partie (valeur variable partionnee) pour
	// l'individu courant
	for (nAttributeIndex = 0; nAttributeIndex < svPredictorPartitionedAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		predictorPartitionedAttribute =
		    kwcClass->LookupAttribute(svPredictorPartitionedAttributeNames.GetAt(nAttributeIndex));

		// Extraction de l'index de l'attribut natif
		liIndex = kwcClass->LookupAttribute(svPredictorAttributeNames.GetAt(nAttributeIndex))->GetLoadIndex();

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de groupage
		sRuleLabel = predictorPartitionedAttribute->GetDerivationRule()
				 ->GetFirstOperand()
				 ->GetDerivationRule()
				 ->GetLabel();

		// Cas d'une discretisation
		nModalityIndex = -1;
		if (sRuleLabel == discretizationRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*, predictorPartitionedAttribute->GetDerivationRule()
							       ->GetFirstOperand()
							       ->GetDerivationRule())
				->GetContinuousPartIndex(kwoObject->GetContinuousValueAt(liIndex));
		// Cas d'un groupage
		else if (sRuleLabel == groupingRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*, predictorPartitionedAttribute->GetDerivationRule()
							       ->GetFirstOperand()
							       ->GetDerivationRule())
				->GetSymbolPartIndex(kwoObject->GetSymbolValueAt(liIndex));
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
				targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nIndex));

				cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nIndex);

				cPriorProba = exp(targetValueLogProbs->GetTargetLogProb());

				// Calcul du gain : ratio entre le score et la proba a priori
				cGain = cInitialScore / cPriorProba;

				// Cas ou ce gain est le gain maximal rencontre : memorisation de la classe cible
				if (cGain > cMaxGain)
					sContributionClass = targetValueLogProbs->GetTargetValue();
			}

			delete cvScoreVector;
		}
	}

	if (oaInstanceProbabilities != NULL)
		oaInstanceProbabilities->DeleteAll();
	else
	{
		oaInstanceProbabilities = new ObjectArray;
		oaInstanceProbabilities->SetCompareFunction(
		    KIPartitionedAttributeProbsCompareContributionImportanceValue);
	}
	oaInstanceProbabilities->SetSize(svPredictorPartitionedAttributeNames.GetSize());

	// Extraction de l'index de la classe cible dans le tableau des probas
	nClassIndex = (int)lnkdClassNamesIndexes.Lookup(sContributionClass.GetNumericKey());

	// Parcours des variables contribuant au classifieur
	for (nAttributeIndex = 0; nAttributeIndex < svPredictorPartitionedAttributeNames.GetSize(); nAttributeIndex++)
	{
		partitionedAttributeProbs = new KIPartitionedAttributeProbs;
		partitionedAttributeProbs->SetAttributeIndex(nAttributeIndex);
		partitionedAttributeProbs->SetModalityIndex(ivModalityIndexes.GetAt(nAttributeIndex));

		// Calcul de l'indicateur d'importance
		cImportanceValue =
		    ComputeShapley(nAttributeIndex, nClassIndex, ivModalityIndexes.GetAt(nAttributeIndex));

		// Memorisation de la proba (valeur d'importance)
		assert(cImportanceValue != -1);
		partitionedAttributeProbs->SetContributionImportanceValue(cImportanceValue);

		// Insertion du vecteur dans le tableau
		oaInstanceProbabilities->SetAt(nAttributeIndex, partitionedAttributeProbs);
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
	boolean bTrace = false;
	Continuous cTerm1Numerator;
	Continuous cTerm1Denominator;
	Continuous cTerm1;
	Continuous cTerm2Numerator;
	Continuous cTerm2Denominator;
	Continuous cTerm2;
	Continuous cProbaModality;
	ALString sTarget;
	KIShapleyTable* stShapleyTable;
	KITargetValueLogProbs* targetValueLogProbs;
	ContinuousVector* cvSourceConditionalLogProbs;
	Continuous variableWeight;
	int nAttributeIndex;
	int nTargetClassIndex;
	int nModalityIndex;
	int nClassIndex;
	int nModality;
	int nTarget;
	int nVariableNumber = svPredictorPartitionedAttributeNames.GetSize();
	int nClassNumber = svTargetValues.GetSize();
	boolean bLocalTrace = false;
	ContinuousVector cvShapeleyExpectedValueByTarget;
	Continuous cProbaForTarget;
	Continuous cImportanceValue;

	// Nettoyage prealable
	oaShapleyTables.DeleteAll();

	// Initialisation
	oaShapleyTables.SetSize(nVariableNumber);
	cvShapeleyExpectedValueByTarget.SetSize(nVariableNumber * nClassNumber);
	for (nAttributeIndex = 0; nAttributeIndex < nVariableNumber; nAttributeIndex++)
	{
		for (nTargetClassIndex = 0; nTargetClassIndex < nClassNumber; nTargetClassIndex++)
		{
			// 2eme terme : on somme sur tous les intervalles possibles de l'attribut
			cTerm2 = 0;

			// Extraction du tableau des probas pour la classe cible courante
			targetValueLogProbs =
			    cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nTargetClassIndex));

			// Extraction du vecteur de probas pour l'attribut predictif
			cvSourceConditionalLogProbs =
			    cast(ContinuousVector*,
				 targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetAt(nAttributeIndex));

			for (nModality = 0; nModality < cvSourceConditionalLogProbs->GetSize(); nModality++)
			{
				// P(In) : proba de tomber dans l'intervalle In, qque soit la classe C
				// P(In) = P(In|C1) * P(C1) + P(In|C2) * P(C2) + P(In|C3) * P(C3) + ....
				cProbaModality = 0;
				for (nTarget = 0; nTarget < ivTargetFrequencies.GetSize(); nTarget++)
				{
					cProbaForTarget =
					    ivTargetFrequencies.GetAt(nTarget) / (double)lTotalFrequency; // P(Cn)
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
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(0));

		variableWeight = cvVariableWeights.GetAt(nAttributeIndex);

		// Extraction du vecteur de probas pour l'attribut predictif
		cvSourceConditionalLogProbs =
		    cast(ContinuousVector*,
			 targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetAt(nAttributeIndex));
		stShapleyTable->Initialize(cvSourceConditionalLogProbs->GetSize(), nClassNumber);
		oaShapleyTables.SetAt(nAttributeIndex, stShapleyTable);

		for (nClassIndex = 0; nClassIndex < nClassNumber; nClassIndex++)
		{

			// Extraction du tableau des probas pour la classe cible courante
			targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));

			// Extraction du vecteur de probas pour l'attribut predictif
			cvSourceConditionalLogProbs =
			    cast(ContinuousVector*,
				 targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetAt(nAttributeIndex));

			for (nModalityIndex = 0; nModalityIndex < cvSourceConditionalLogProbs->GetSize();
			     nModalityIndex++)
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

	// Affichage des table de Shapley
	if (bTrace)
	{
		cout << "Shapley tables\n";
		for (nAttributeIndex = 0; nAttributeIndex < nVariableNumber; nAttributeIndex++)
		{
			stShapleyTable = cast(KIShapleyTable*, oaShapleyTables.GetAt(nAttributeIndex));

			cout << "Variable " << nAttributeIndex + 1 << "\n";
			cout << *stShapleyTable << "\n";
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
	KITargetValueLogProbs* targetValueLogProbs;
	ContinuousVector* cvSourceConditionalLogProbs;

	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));

	// Extraction du vecteur de probas pour l'attribut predictif
	cvSourceConditionalLogProbs = cast(
	    ContinuousVector*, targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetAt(nAttributeIndex));

	// On retourne la proba associee a la modalite pour cet attribut predictif
	require(nModalityIndex < cvSourceConditionalLogProbs->GetSize());
	return cvSourceConditionalLogProbs->GetAt(nModalityIndex);
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
	KITargetValueLogProbs* targetValueLogProbs;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetSize() ==
		       ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueLogProbs->GetTargetLogProb());

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
	KITargetValueLogProbs* targetValueLogProbs;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetSize() ==
		       ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueLogProbs->GetTargetLogProb());

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
// Classe KIDRContributionValueAtOld
KIDRContributionValueAtOld::KIDRContributionValueAtOld()
{
	SetName("ContributionValueAtOld");
	SetLabel("Variable importance value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	// resultats de la contribution au score, pour une classe cible donnee
	GetFirstOperand()->SetStructureName("ScoreContribution");
	// rang d'importance de la variable pour laquelle ont veut obtenir les resultats
	GetSecondOperand()->SetType(KWType::Continuous);
}

KIDRContributionValueAtOld::~KIDRContributionValueAtOld() {}

KWDerivationRule* KIDRContributionValueAtOld::Create() const
{
	return new KIDRContributionValueAtOld;
}

Continuous KIDRContributionValueAtOld::ComputeContinuousResult(const KWObject* kwoObject) const
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
	require(oaInstanceProbabilities != NULL);

	// Pas de reenforcement pour cet individu
	if (oaInstanceProbabilities->GetSize() == 0)
		return 0;
	return cInitialScore;
}

Continuous KIDRClassifierReinforcement::GetReinforcementFinalScoreAt(int rank) const
{
	KIPartitionedAttributeProbs* attributeProbas;

	require(oaInstanceProbabilities != NULL);

	// Pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0;
	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->GetReinforcementNewScore();
}

Continuous KIDRClassifierReinforcement::GetReinforcementClassChangeTagAt(int rank) const
{
	KIPartitionedAttributeProbs* attributeProbas;

	require(oaInstanceProbabilities != NULL);

	// Pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0;
	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	return (Continuous)attributeProbas->GetReinforcementClassHasChanged();
}

Symbol KIDRClassifierReinforcement::GetReinforcementNameAt(int rank) const
{
	KIPartitionedAttributeProbs* attributeProbas;
	Symbol sName;

	require(oaInstanceProbabilities != NULL);

	// Pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return "";
	attributeProbas = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	sName = svPredictorPartitionedAttributeNames.GetAt(attributeProbas->GetAttributeIndex());
	return sName;
}

Symbol KIDRClassifierReinforcement::GetReinforcementPartAt(int rank) const
{
	Symbol sAttributeName;
	KWAttribute* attribute;
	KWDataGridStats* dataGridStats;
	KIPartitionedAttributeProbs* attributeProbs;
	ostringstream oss;

	require(oaInstanceProbabilities != NULL);

	// Pas de reenforcement pour cet individu et ce rang
	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return "";

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	attributeProbs = cast(KIPartitionedAttributeProbs*, oaInstanceProbabilities->GetAt(rank));
	sAttributeName = GetReinforcementNameAt(rank);
	attribute = kwcClass->LookupAttribute(sAttributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient
	// l'individu pour cette variable
	dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);
	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbs->GetModalityIndex());
	delete dataGridStats;
	return oss.str().c_str();
}

void KIDRClassifierReinforcement::ComputeReinforcement(const KWObject* kwoObject) const
{

	int nAttributeIndex;
	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	KWDerivationRule* partitionRule;
	IntVector ivModalityIndexes;
	KWAttribute* predictorPartitionedAttribute;
	KWLoadIndex liIndex;
	Symbol sReinforcementClass;
	KWAttribute* predictedClassAttribute;
	Symbol sPredictedClass;
	ContinuousVector* cvScoreVector;
	int nModalityIndex;

	require(IsCompiled());

	ivModalityIndexes.SetSize(svPredictorPartitionedAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable du predicteur, l'index de la partie (valeur variable partionnee) pour l'individu courant
	for (nAttributeIndex = 0; nAttributeIndex < svPredictorPartitionedAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		predictorPartitionedAttribute =
		    kwcClass->LookupAttribute(svPredictorPartitionedAttributeNames.GetAt(nAttributeIndex));

		// Extraction de l'index de l'attribut natif
		liIndex = kwcClass->LookupAttribute(svPredictorAttributeNames.GetAt(nAttributeIndex))->GetLoadIndex();

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de groupage
		partitionRule =
		    predictorPartitionedAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule();

		// Cas d'une discretisation
		nModalityIndex = -1;
		if (partitionRule->GetLabel() == discretizationRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*, predictorPartitionedAttribute->GetDerivationRule()
							       ->GetFirstOperand()
							       ->GetDerivationRule())
				->GetContinuousPartIndex(kwoObject->GetContinuousValueAt(liIndex));
		// Cas d'un groupage
		else if (partitionRule->GetLabel() == groupingRuleRef.GetLabel())
			nModalityIndex =
			    cast(KWDRUnivariatePartition*, predictorPartitionedAttribute->GetDerivationRule()
							       ->GetFirstOperand()
							       ->GetDerivationRule())
				->GetSymbolPartIndex(kwoObject->GetSymbolValueAt(liIndex));
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
		oaInstanceProbabilities->SetCompareFunction(KIPartitionedAttributeProbsCompareReinforcementNewScore);
	}

	// Calcul du score associe pour la classe de reenforcement
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetValuesNumberInNBScore);

	// Calcul du tableau des valeurs d'amelioration de score
	ComputeReinforcementProbas(&ivModalityIndexes, sPredictedClass, cvScoreVector, nTargetValuesNumberInNBScore);

	// Nettoyage
	delete cvScoreVector;
}

void KIDRClassifierReinforcement::ComputeReinforcementProbas(IntVector* ivModalityIndexes, Symbol sPredictedClass,
							     ContinuousVector* cvBestScore, int nHowNumber) const
{
	KWAttribute* attribute;
	KWAttribute* partitionedAttribute;
	KIPartitionedAttributeProbs* partitionedAttributeProbas;
	ContinuousVector* cvScore;
	Continuous cBestScore;
	Continuous cScore;
	Continuous cMaxScore;
	int nReinforcementClassHasChanged;
	ALString sPredictorAttributeName;
	ALString sPredictorPartitionedAttributeName;
	int nAttributeIndex;
	int nBestModalityIndex = -1;
	int nModalityNumber;
	int nModalityIndex;
	int nClassIndex;
	KITargetValueLogProbs* targetValueLogProbs;

	require(oaInstanceProbabilities != NULL);
	require(oaInstanceProbabilities->GetSize() == 0);

	targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nHowNumber));

	// Parcours des variables natives contribuant au NB a interpreter
	for (nAttributeIndex = 0; nAttributeIndex < svPredictorAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction du nom de la variable native
		sPredictorAttributeName = svPredictorAttributeNames.GetAt(nAttributeIndex);

		// Extraction de l'attribut natif du dictionnaire
		attribute = kwcClass->LookupAttribute(sPredictorAttributeName);

		// Cas ou cette variable est une variable levier selectionnee
		if (attribute->GetConstMetaData()->IsMissingTypeAt(LEVER_ATTRIBUTE_META_TAG))
		{
			// Extraction du nom de la variable explicative
			sPredictorPartitionedAttributeName =
			    svPredictorPartitionedAttributeNames.GetAt(nAttributeIndex);

			// Extraction de l'attribut explicatif
			partitionedAttribute = kwcClass->LookupAttribute(sPredictorPartitionedAttributeName);

			// Calcul du nombre de modalites de l'attribut
			nModalityNumber =
			    cast(KWDRUnivariatePartition*,
				 partitionedAttribute->GetDerivationRule()->GetFirstOperand()->GetDerivationRule())
				->GetPartNumber();

			// Initialisation du score a ameliorer
			cBestScore = cInitialScore;

			// Parcours des modalites
			nReinforcementClassHasChanged = -1;
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

						if (sPredictedClass == targetValueLogProbs->GetTargetValue())
							nReinforcementClassHasChanged = 0;
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
								nReinforcementClassHasChanged = 1;

							// Sinon : pas de changement de classe ou vers une autre classe que la classe de reference
							else
								nReinforcementClassHasChanged = -1;
						}
					}
					// Nettoyage du vecteur de scores
					delete cvScore;
				}
			}
			assert(nReinforcementClassHasChanged == -1 or nReinforcementClassHasChanged == 0 or
			       nReinforcementClassHasChanged == 1);

			// Cas d'une amelioration pour une des modalites de la variable
			if (cBestScore > cInitialScore)
			{
				partitionedAttributeProbas = new KIPartitionedAttributeProbs;

				partitionedAttributeProbas->SetAttributeIndex(nAttributeIndex);

				assert(nBestModalityIndex != -1);
				partitionedAttributeProbas->SetModalityIndex(nBestModalityIndex);

				partitionedAttributeProbas->SetReinforcementNewScore(cBestScore);

				// Memorisation du tag indiquant si la nouvelle classe predite est la classe de reference du pourquoi
				partitionedAttributeProbas->SetReinforcementClassHasChanged(
				    nReinforcementClassHasChanged);
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
	KITargetValueLogProbs* targetValueLogProbs;

	// Initialisations
	cInitialScore = 0;
	nTargetValuesNumberInNBScore = -1;
	sReinforcementClass = GetOperandAt(2)->GetSymbolConstant();

	// Parcours des valeurs cible
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));

		if (targetValueLogProbs->GetTargetValue() == sReinforcementClass)
		{
			// Memorisation de l'index du vecteur de probas pour cette classe
			nTargetValuesNumberInNBScore = nClassIndex;
			break;
		}
	}
	ensure(nTargetValuesNumberInNBScore != -1);
}

ContinuousVector* KIDRClassifierReinforcement::ComputeScoreVectorLj(IntVector* ivModalityIndexes) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;
	KITargetValueLogProbs* targetValueLogProbs;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetSize() ==
		       ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueLogProbs->GetTargetLogProb());

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
	KITargetValueLogProbs* targetValueLogProbs;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		targetValueLogProbs = cast(KITargetValueLogProbs*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueLogProbs->GetAttributeSourceConditionalLogProbs()->GetSize() ==
		       ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueLogProbs->GetTargetLogProb());

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

/*DDD
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
	return scoreReinforcement->GetReinforcementPartAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
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
*/
