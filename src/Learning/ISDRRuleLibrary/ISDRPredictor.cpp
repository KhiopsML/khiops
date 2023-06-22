// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ISDRPredictor.h"

#include "KWDRNBPredictor.h"
#include "KWDRDataGrid.h"

int ISCompareContributionImportanceValue(const void* elem1, const void* elem2);
int ISCompareReinforcementNewScore(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////////////
// Classe ISPartitionedAttributeProbas

ISPartitionedAttributeProbas::ISPartitionedAttributeProbas()
{

	iAttributeIndex = -1;
	iModalityIndex = -1;
	cReinforcementNewScore = 0;
	cReinforcementClassHasChanged = 0;
	cContributionImportanceValue = 0;
}

ISPartitionedAttributeProbas::~ISPartitionedAttributeProbas() {}

///////////////////////////////////////////////////////////////////////////////
// Classe ISTargetValueProbas

ISTargetValueProbas::ISTargetValueProbas()
{

	cProbaApriori = 0;
	oaProbasAposteriori = new ObjectArray;
}

ISTargetValueProbas::~ISTargetValueProbas()
{

	oaProbasAposteriori->DeleteAll();
	delete oaProbasAposteriori;
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRClassifierInterpretation

ISDRClassifierInterpretation::ISDRClassifierInterpretation()
{
	oaInstanceProbabilities = NULL;
}

ISDRClassifierInterpretation::~ISDRClassifierInterpretation()
{
	Clean();
}

void ISDRClassifierInterpretation::Clean()
{

	oaModelProbabilities.DeleteAll();
	oaModelProbabilities.SetSize(0);

	oaPartitionedPredictiveAttributeNames.DeleteAll();
	oaPartitionedPredictiveAttributeNames.SetSize(0);

	oaNativePredictiveAttributeNames.DeleteAll();
	oaNativePredictiveAttributeNames.SetSize(0);

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

void ISDRClassifierInterpretation::Compile(KWClass* kwcOwnerClass)
{

	ISTargetValueProbas* targetValueProbas;
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;
	int nTargetValueNumber;
	int nFirstOperandIndex = 0;
	KWDerivationRuleOperand* operand;
	KWDRSNBClassifier referenceSNBRule;
	KWDRNBClassifier referenceNBRule;
	boolean bIsSNB = false;
	boolean bIsNB = false;
	ALString sAttributeName;
	Continuous cTargetFrequency;
	Continuous cTargetTotalFrequency;

	KWDerivationRule::Compile(kwcOwnerClass);

	Clean();

	KWDRNBClassifier* classifier =
	    cast(KWDRNBClassifier*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

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

	if (not bIsNB and not bIsSNB)
	{
		AddError("This classifier is not yet implemented.");
		return;
	}

	// Parcours des operandes du classifieur pour identifier les noms des attributs explicatifs et des attributs
	// natifs associes La derniere operande n'est pas parcouru car reserve a l'attribut des valeurs cibles
	for (int nOperandIndex = nFirstOperandIndex; nOperandIndex < classifier->GetOperandNumber() - 1;
	     nOperandIndex++)
	{
		operand = classifier->GetOperandAt(nOperandIndex);

		// Extraction du nom de la variable explicative
		sAttributeName = operand->GetDerivationRule()->GetFirstOperand()->GetAttributeName();

		// Creation d'un StringObject pour memoriser ce nom de variable
		StringObject* soAttributeName = new StringObject;
		soAttributeName->SetString(sAttributeName);

		// Memorisation du nom de la variable pour la synchronisation
		// avec le tableau oaPartitionIntervals
		oaPartitionedPredictiveAttributeNames.Add(soAttributeName);

		// Extraction du nom de la variable native
		sAttributeName = operand->GetDerivationRule()->GetSecondOperand()->GetAttributeName();

		// Creation d'un StringObject pour memoriser ce nom de variable
		soAttributeName = new StringObject;
		soAttributeName->SetString(sAttributeName);

		// Memorisation du nom de la variable pour la synchronisation
		// avec le tableau oaPartitionIntervals
		oaNativePredictiveAttributeNames.Add(soAttributeName);

		// Cas d'un NB
		if (bIsNB)
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
	for (int nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		svTargetValues.SetAt(nTarget, targetSymbolValueSet->GetValueAt(nTarget));

	// Creation et initialisation du tableau des probabilites (sans remplissage lors de ce parcours)
	for (int nClassIndex = 0; nClassIndex < svTargetValues.GetSize(); nClassIndex++)
	{
		// Creation du tableau associe a la classe de l'attribut
		targetValueProbas = new ISTargetValueProbas;

		targetValueProbas->sTargetValue = svTargetValues.GetAt(nClassIndex);

		// Parcours des variables explicatives
		for (int nAttributeIndex = 0; nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize();
		     nAttributeIndex++)
		{
			// Creation du vecteur qui accueillera :
			// les logs des probas a posteriori de la classe pour chaque partie de la variable explicative
			ContinuousVector* cvPosteriorProba = new ContinuousVector;
			targetValueProbas->oaProbasAposteriori->Add(cvPosteriorProba);

			// Extraction de l'attribut explicatif courant
			KWAttribute* attribute = kwcClass->LookupAttribute(
			    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex))
				->GetString());

			// Initialisation de la taille du vecteur de proba de l'attribut
			cvPosteriorProba->SetSize(
			    cast(KWDRDataGrid*, attribute->GetDerivationRule())->GetAttributePartNumberAt(0));
		}

		// Memorisation du lien entre le nom de la classe cible et l'index de la ligne correspondante
		// dans le tableau des probas
		StringObject* soClassIndex = new StringObject;
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
		cTargetTotalFrequency = 0;
		for (int nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			cTargetFrequency = (Continuous)classifier->GetDataGridSetTargetFrequencyAt(nClassIndex);
			assert(cTargetFrequency > 0);
			cTargetTotalFrequency += cTargetFrequency;
		}
		assert(cTargetTotalFrequency > 0);

		// Calcul des logarithme de probabilites des valeurs cibles
		for (int nClassIndex = 0; nClassIndex < classifier->GetDataGridSetTargetPartNumber(); nClassIndex++)
		{
			// Extraction du tableau des probas de cette valeur cible
			targetValueProbas = cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

			// Initialisation avec le prior
			assert(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) > 0);
			targetValueProbas->cProbaApriori =
			    log(classifier->GetDataGridSetTargetFrequencyAt(nClassIndex) / cTargetTotalFrequency);

			// Ajout des probabilites conditionnelles par grille
			for (int nDataGridIndex = 0; nDataGridIndex < classifier->GetDataGridStatsNumber();
			     nDataGridIndex++)
			{
				// Recherche de la grille
				const KWDRDataGridStats* dataGridStats = classifier->GetDataGridStatsAt(nDataGridIndex);

				// Recherche de l'index de la partie cible de la grille
				int nTargetIndex = classifier->GetDataGridSetTargetIndexAt(nDataGridIndex, nClassIndex);

				// Parcours de toutes les parties sources
				for (int nSourceIndex = 0; nSourceIndex < dataGridStats->GetDataGridSourceCellNumber();
				     nSourceIndex++)
				{
					// Extraction de la log proba
					Continuous cTargetLogProb =
					    dataGridStats->GetDataGridSourceConditionalLogProbAt(nSourceIndex,
												 nTargetIndex);

					ContinuousVector* cv =
					    cast(ContinuousVector*,
						 targetValueProbas->oaProbasAposteriori->GetAt(nDataGridIndex));
					cv->SetAt(nSourceIndex, cTargetLogProb);
				}
			}
		}
	}
}

const ALString ISDRClassifierInterpretation::LEVER_ATTRIBUTE_META_TAG = "LeverAttribute";
const ALString ISDRClassifierInterpretation::INTERPRETATION_ATTRIBUTE_META_TAG = "ClassifierInterpretationAttribute";
const ALString ISDRClassifierInterpretation::NO_VALUE_LABEL = "No value";

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRClassifierContribution

ISDRClassifierContribution::ISDRClassifierContribution()
{
	SetName("ScoreContribution");
	SetLabel("Classifier score contribution");
	SetType(KWType::Structure);
	SetStructureName("ScoreContribution");
	SetOperandNumber(5);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier"); // classifieur
	GetSecondOperand()->SetType(KWType::Symbol);       // classe predite (attribut de prediction)
	GetOperandAt(2)->SetType(KWType::Symbol);          // classe cible pour le calcul de l'importance
	GetOperandAt(3)->SetType(KWType::Symbol);          // methode de calcul de l'importance de la variable
	GetOperandAt(4)->SetType(KWType::Symbol); // indique si on trie les probas ou non (modifiable en mode expert
						  // uniquement, et par defaut, on trie)

	oaInstanceProbabilities = NULL;
	contributionComputingMethod = NormalizedOddsRatio;
	bSortInstanceProbas = false;
}

ISDRClassifierContribution::~ISDRClassifierContribution() {}

KWDerivationRule* ISDRClassifierContribution::Create() const
{
	return new ISDRClassifierContribution;
}

Object* ISDRClassifierContribution::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// evaluer l'operande classifieur
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Calcul des donnees d'interpretation
	ComputeContribution(kwoObject);

	return (Object*)this;
}

Continuous ISDRClassifierContribution::GetContributionValueAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->cContributionImportanceValue;
}

Symbol ISDRClassifierContribution::GetContributionNameAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	StringObject* soVariableName =
	    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(attributeProbas->iAttributeIndex));
	ALString as = soVariableName->GetString();
	return as.GetBuffer(as.GetLength());
}

Symbol ISDRClassifierContribution::GetContributionPartitionAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));

	Symbol attributeName = GetContributionNameAt(rank);

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	KWAttribute* attribute = kwcClass->LookupAttribute(attributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient l'individu pour cette variable
	KWDataGridStats* dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);
	ostringstream oss;
	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbas->iModalityIndex);
	delete dataGridStats;

	return oss.str().c_str();
}

Symbol ISDRClassifierContribution::GetContributionClass() const
{

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() > 0);

	return sContributionClass;
}

void ISDRClassifierContribution::ComputeContribution(const KWObject* kwoObject) const
{
	assert(IsCompiled());

	ISPartitionedAttributeProbas* partitionedAttributeProbas;
	Continuous cImportanceValue = -1;
	StringObject* sClassIndex;
	int nAttributeIndex;
	int nClassIndex;
	int nDatabaseSize;
	int nClassNumber;
	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	IntVector ivModalityIndexes;

	ivModalityIndexes.SetSize(oaPartitionedPredictiveAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable predictive, l'index de la partie (valeur variable partionnee) pour
	// l'individu courant
	for (nAttributeIndex = 0; nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		KWAttribute* predictiveAttribute = kwcClass->LookupAttribute(
		    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex))->GetString());

		// Extraction de l'index de l'attribut natif
		KWLoadIndex nNativeIndex =
		    kwcClass
			->LookupAttribute(
			    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetString())
			->GetLoadIndex();

		int nModalityIndex = -1;

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de
		// groupage
		const ALString sRuleLabel =
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
		KWAttribute* predictedClassAttribute =
		    kwcClass->LookupAttribute(GetSecondOperand()->GetAttributeName());
		sContributionClass = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
	}
	else
	{
		if (sContributionClass == CLASS_OF_HIGHEST_GAIN_LABEL)
		{
			// determiner quelle est la classe de plus grand gain

			Continuous cInitialScore;
			Continuous cGain;
			Continuous cMaxGain = 0;
			Continuous cPriorProba;

			// Calcul du vecteur de scores de l'individu
			ContinuousVector* cvScoreVector = ComputeScoreVectorLj(&ivModalityIndexes);

			for (int nIndex = 0; nIndex < oaModelProbabilities.GetSize(); nIndex++)
			{

				ISTargetValueProbas* targetValueProbas =
				    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nIndex));

				cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nIndex);

				cPriorProba = exp(targetValueProbas->cProbaApriori);

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
		oaInstanceProbabilities->SetCompareFunction(ISCompareContributionImportanceValue);
	}
	oaInstanceProbabilities->SetSize(oaPartitionedPredictiveAttributeNames.GetSize());

	// Extraction de l'index de la classe cible dans le tableau des probas
	sClassIndex = cast(StringObject*, odClassNamesIndexes.Lookup(sContributionClass));
	assert(sClassIndex != NULL);
	nClassIndex = (int)KWContinuous::StringToContinuous(sClassIndex->GetString());

	nClassNumber = oaModelProbabilities.GetSize();
	nDatabaseSize = 10000;

	// Parcours des variables contribuant au classifieur
	for (nAttributeIndex = 0; nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		partitionedAttributeProbas = new ISPartitionedAttributeProbas;

		partitionedAttributeProbas->iAttributeIndex = nAttributeIndex;
		partitionedAttributeProbas->iModalityIndex = ivModalityIndexes.GetAt(nAttributeIndex);

		// Calcul de l'indicateur d'importance

		if (contributionComputingMethod == NormalizedOddsRatio)
			cImportanceValue = ComputeNormalizedOddsRatio(nAttributeIndex, nClassIndex, &ivModalityIndexes,
								      nDatabaseSize, nClassNumber);
		else if (contributionComputingMethod == ImportanceValue)
			cImportanceValue = ComputeImportanceValue(nAttributeIndex, nClassIndex,
								  ivModalityIndexes.GetAt(nAttributeIndex));
		else if (contributionComputingMethod == WeightOfEvidence)
			cImportanceValue = ComputeWeightOfEvidence(nAttributeIndex, nClassIndex, &ivModalityIndexes,
								   nDatabaseSize, nClassNumber);
		else if (contributionComputingMethod == InformationDifference)
			cImportanceValue = ComputeInformationDifference(
			    nAttributeIndex, nClassIndex, &ivModalityIndexes, nDatabaseSize, nClassNumber);
		else if (contributionComputingMethod == DifferenceProbabilities)
			cImportanceValue =
			    ComputeDifferenceProbabilities(nAttributeIndex, nClassIndex, &ivModalityIndexes);
		else if (contributionComputingMethod == ModalityProbability)
			cImportanceValue = ComputeModalityProbability(nAttributeIndex, nClassIndex,
								      ivModalityIndexes.GetAt(nAttributeIndex));
		else if (contributionComputingMethod == BayesDistance)
			cImportanceValue = ComputeBayesDistance(nAttributeIndex, nClassIndex,
								ivModalityIndexes.GetAt(nAttributeIndex));
		else if (contributionComputingMethod == Kullback)
			cImportanceValue = ComputeKullback(nAttributeIndex, nClassIndex, &ivModalityIndexes,
							   nDatabaseSize, nClassNumber);
		else if (contributionComputingMethod == LogModalityProbability)
			cImportanceValue = ComputeLogModalityProbability(nAttributeIndex, nClassIndex,
									 ivModalityIndexes.GetAt(nAttributeIndex));
		else if (contributionComputingMethod == LogImportanceValue)
			cImportanceValue = ComputeLogImportanceValue(nAttributeIndex, nClassIndex,
								     ivModalityIndexes.GetAt(nAttributeIndex));
		else if (contributionComputingMethod == BayesDistanceWithoutPrior)
			cImportanceValue = ComputeBayesDistanceWithoutPrior(nAttributeIndex, nClassIndex,
									    ivModalityIndexes.GetAt(nAttributeIndex));

		// Memorisation de la proba (valeur d'importance)
		assert(cImportanceValue != -1);
		partitionedAttributeProbas->cContributionImportanceValue = cImportanceValue;

		// Insertion du vecteur dans le tableau
		oaInstanceProbabilities->SetAt(nAttributeIndex, partitionedAttributeProbas);
	}

	// si demande, tri selon la proba a posteriori
	if (bSortInstanceProbas)
		oaInstanceProbabilities->Sort();
}

Continuous ISDRClassifierContribution::ComputeImportanceValue(int nAttributeIndex, int nTargetClassIndex,
							      int nModalityIndex) const
{
	Continuous cLogPosteriorProba;
	Continuous cImportanceValue;

	// Extraction de la log proba conditionnelle a la classe du pourquoi
	cLogPosteriorProba = ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex);

	// Calcul de l'indicateur d'importance
	cImportanceValue =
	    exp(cLogPosteriorProba) -
	    exp(ComputeMaxLogPosteriorProbaWithoutWhyClassValue(nTargetClassIndex, nAttributeIndex, nModalityIndex));

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeWeightOfEvidence(int nAttributeIndex, int nTargetClassIndex,
							       IntVector* ivModalityIndexes, int nDatabaseSize,
							       int nTargetValuesNumber) const
{
	ContinuousVector* cvScoreVector;
	Continuous cInitialScore;
	Continuous cScoreWithoutOneVariable;
	Continuous cImportanceValue;
	Continuous cRatio;
	Continuous cInitialScoreCorrected;
	Continuous cScoreWithoutOneVariableCorrected;
	Continuous cImportanceValueCorrected;
	Continuous cRatioCorrected;

	// Calcul du score initial p(C|X)
	cvScoreVector = ComputeScoreVectorLj(ivModalityIndexes);
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul du score sans prendre en compte la variable explicative p(C|X\X_i)
	cvScoreVector = ComputeScoreVectorLjWithoutOneVariable(ivModalityIndexes, nAttributeIndex);
	cScoreWithoutOneVariable = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// AjoutVincent 10/2009 - Correction de Laplace pour que P>0 et P<1
	// Prise en compte d'un epsilon de Laplace
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0)
	cInitialScoreCorrected =
	    (cInitialScore + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));
	cScoreWithoutOneVariableCorrected =
	    (cScoreWithoutOneVariable + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));

	// Vincent2009 - j'ai retire la correction et ajoute celle au-dessus
	// Correction de Laplace pour eviter les divisions par zero
	// cInitialScore = (cInitialScore * nDatabaseSize + 1) / (nDatabaseSize + nTargetValuesNumber);
	// cScoreWithoutOneVariable = (cScoreWithoutOneVariable * nDatabaseSize + 1) / (nDatabaseSize +
	// nTargetValuesNumber);

	cRatio = (cInitialScore * (1 - cScoreWithoutOneVariable)) / ((1 - cInitialScore) * cScoreWithoutOneVariable);

	cRatioCorrected = (cInitialScoreCorrected * (1 - cScoreWithoutOneVariableCorrected)) /
			  ((1 - cInitialScoreCorrected) * cScoreWithoutOneVariableCorrected);
	// DDD
	// cout  "ratio " << cRatio << " son log " << log(cRatio) <<  " 1/log(2)*" <<  1/log(2.0)*log(cRatio) << endl;
	// Calcul de l'indicateur Weight of Evidence
	// cImportanceValue = 1/log(2.0) * (log(cInitialScore / (1 - cInitialScore)) - log(cScoreWithoutOneVariable / (1
	// - cScoreWithoutOneVariable))); cImportanceValue = 1/log(2.0) * (log(p / (1-p)) - log(q / (1-q)))

	// cImportanceValue = 1/log(2.0) * (log(cInitialScore) - log(1 - cInitialScore) - log(cScoreWithoutOneVariable)
	// + log(1 - cScoreWithoutOneVariable)); cImportanceValue = 1/log(2.0) * (log(p) - log(1-p) -log(q) + log(1-q))

	// cImportanceValueCorrected = 1/log(2.0) * (log(cInitialScoreCorrected) - log(1 - cInitialScoreCorrected) -
	// log(cScoreWithoutOneVariableCorrected) + log(1 - cScoreWithoutOneVariableCorrected));
	cImportanceValue = 1 / log(2.0) * log(cRatio);
	cImportanceValueCorrected = 1 / log(2.0) * log(cRatioCorrected);

	// cout << "Attribute " << nAttributeIndex << " Class " << nTargetClassIndex << endl;
	// cout << " P1= " << cInitialScore  << " P1C= " << cInitialScoreCorrected << endl;
	// cout << " P2= " << cScoreWithoutOneVariable << " P2C= " << cScoreWithoutOneVariableCorrected << endl;
	// cout << "cImportanceValue " << cImportanceValue << endl;
	// cout << "cImportanceValueCorrected " << cImportanceValueCorrected << endl;

	return cImportanceValueCorrected;
}

Continuous ISDRClassifierContribution::ComputeInformationDifference(int nAttributeIndex, int nTargetClassIndex,
								    IntVector* ivModalityIndexes, int nDatabaseSize,
								    int nTargetValuesNumber) const
{
	ContinuousVector* cvScoreVector;
	Continuous cInitialScore;
	Continuous cScoreWithoutOneVariable;
	Continuous cImportanceValue;
	Continuous cInitialScoreCorrected;
	Continuous cScoreWithoutOneVariableCorrected;
	Continuous cImportanceValueCorrected;

	// Calcul du score initial p(C|X)
	cvScoreVector = ComputeScoreVectorLj(ivModalityIndexes);
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul du score sans prendre en compte la variable explicative p(C|X\X_i)
	cvScoreVector = ComputeScoreVectorLjWithoutOneVariable(ivModalityIndexes, nAttributeIndex);
	cScoreWithoutOneVariable = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// AjoutVincent 10/2009 - Correction de Laplace pour que P>0 et P<1
	// Prise en compte d'un epsilon de Laplace
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0)
	cInitialScoreCorrected =
	    (cInitialScore + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));
	cScoreWithoutOneVariableCorrected =
	    (cScoreWithoutOneVariable + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));

	// Vincent2009 - j'ai retir� la correction et ajout� celle au-dessus
	// Correction de Laplace pour eviter les divisions par zero
	// cInitialScore = (cInitialScore * nDatabaseSize + 1) / (nDatabaseSize + nTargetValuesNumber);
	// cScoreWithoutOneVariable = (cScoreWithoutOneVariable * nDatabaseSize + 1) / (nDatabaseSize +
	// nTargetValuesNumber);

	// Calcul de l'indicateur Weight of Evidence
	cImportanceValue = 1 / log(2.0) * (log(cInitialScore) - log(cScoreWithoutOneVariable));
	cImportanceValueCorrected =
	    1 / log(2.0) * (log(cInitialScoreCorrected) - log(cScoreWithoutOneVariableCorrected));

	return cImportanceValueCorrected;
}

Continuous ISDRClassifierContribution::ComputeDifferenceProbabilities(
    int nAttributeIndex, int nTargetClassIndex,
    IntVector* ivModalityIndexes) const //, int nDatabaseSize, int nTargetValuesNumber)
{
	ContinuousVector* cvScoreVector;
	Continuous cInitialScore;
	Continuous cScoreWithoutOneVariable;
	Continuous cImportanceValue;

	// Calcul du score initial p(C|X)
	cvScoreVector = ComputeScoreVectorLj(ivModalityIndexes);
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul du score sans prendre en compte la variable explicative p(C|X\X_i)
	cvScoreVector = ComputeScoreVectorLjWithoutOneVariable(ivModalityIndexes, nAttributeIndex);
	cScoreWithoutOneVariable = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul de l'indicateur Weight of Evidence
	cImportanceValue = cInitialScore - cScoreWithoutOneVariable;

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeLogImportanceValue(int nAttributeIndex, int nTargetClassIndex,
								 int nModalityIndex) const
{
	Continuous cLogPosteriorProba;
	Continuous cImportanceValue;

	// Extraction de la log proba conditionnelle a la classe du pourquoi
	cLogPosteriorProba = ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex);

	// Calcul de l'indicateur d'importance
	cImportanceValue = cLogPosteriorProba - ComputeMaxLogPosteriorProbaWithoutWhyClassValue(
						    nTargetClassIndex, nAttributeIndex, nModalityIndex);

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeModalityProbability(int nAttributeIndex, int nTargetClassIndex,
								  int nModalityIndex) const
{
	Continuous cImportanceValue;

	cImportanceValue = exp(ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex));

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeLogModalityProbability(int nAttributeIndex, int nTargetClassIndex,
								     int nModalityIndex) const
{
	Continuous cImportanceValue;

	cImportanceValue = ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex);

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeBayesDistance(int nAttributeIndex, int nTargetClassIndex,
							    int nModalityIndex) const
{
	Continuous cImportanceValue;

	cImportanceValue = ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex) *
			   cvVariableWeights.GetAt(nAttributeIndex) * exp(ExtractLogPriorProba(nTargetClassIndex));

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeBayesDistanceWithoutPrior(int nAttributeIndex, int nTargetClassIndex,
									int nModalityIndex) const
{
	Continuous cImportanceValue;

	cImportanceValue = ExtractLogPosteriorProba(nTargetClassIndex, nAttributeIndex, nModalityIndex) *
			   cvVariableWeights.GetAt(nAttributeIndex);

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeKullback(int nAttributeIndex, int nTargetClassIndex,
						       IntVector* ivModalityIndexes, int nDatabaseSize,
						       int nTargetValuesNumber) const
{
	ContinuousVector* cvScoreVector;
	Continuous cInitialScore;
	Continuous cScoreWithoutOneVariable;
	Continuous cImportanceValue;
	Continuous cInitialScoreCorrected;
	Continuous cScoreWithoutOneVariableCorrected;

	// Calcul du score initial p(C|X)
	cvScoreVector = ComputeScoreVectorLj(ivModalityIndexes);
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul du score sans prendre en compte la variable explicative p(C|X\X_i)
	cvScoreVector = ComputeScoreVectorLjWithoutOneVariable(ivModalityIndexes, nAttributeIndex);
	cScoreWithoutOneVariable = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// AjoutVincent 10/2009 - Correction de Laplace pour que P>0 et P<1
	// Prise en compte d'un epsilon de Laplace
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0)
	cInitialScoreCorrected =
	    (cInitialScore + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));
	cScoreWithoutOneVariableCorrected =
	    (cScoreWithoutOneVariable + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));

	cImportanceValue = cInitialScoreCorrected * log(cInitialScoreCorrected / cScoreWithoutOneVariableCorrected);

	return cImportanceValue;
}

Continuous ISDRClassifierContribution::ComputeMaxLogPosteriorProbaWithoutWhyClassValue(int nWhyTargetValueNumber,
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

void ISDRClassifierContribution::Compile(KWClass* kwcOwnerClass)
{

	ISDRClassifierInterpretation::Compile(kwcOwnerClass); // code classe ancetre

	// initialiser des booleens indiquant la methode de calcul de contribution a utiliser (perfs)

	const Symbol sWhyMethod = GetOperandAt(3)->GetSymbolConstant();

	if (sWhyMethod == NORMALIZED_ODDS_RATIO_LABEL)
		contributionComputingMethod = NormalizedOddsRatio;
	else if (sWhyMethod == MIN_PROBA_DIFF_LABEL)
		contributionComputingMethod = ImportanceValue;
	else if (sWhyMethod == WEIGHT_EVIDENCE_LABEL)
		contributionComputingMethod = WeightOfEvidence;
	else if (sWhyMethod == INFO_DIFF_LABEL)
		contributionComputingMethod = InformationDifference;
	else if (sWhyMethod == DIFF_PROBA_LABEL)
		contributionComputingMethod = DifferenceProbabilities;
	else if (sWhyMethod == MODALITY_PROBA_LABEL)
		contributionComputingMethod = ModalityProbability;
	else if (sWhyMethod == BAYES_DISTANCE_LABEL)
		contributionComputingMethod = BayesDistance;
	else if (sWhyMethod == KULLBACK_LABEL)
		contributionComputingMethod = Kullback;
	else if (sWhyMethod == LOG_MODALITY_PROBA_LABEL)
		contributionComputingMethod = LogModalityProbability;
	else if (sWhyMethod == LOG_MIN_PROBA_DIFF_LABEL)
		contributionComputingMethod = LogImportanceValue;
	else if (sWhyMethod == BAYES_DISTANCE_WITHOUT_PRIOR_LABEL)
		contributionComputingMethod = BayesDistanceWithoutPrior;

	bSortInstanceProbas = (strcmp(GetOperandAt(4)->GetSymbolConstant().GetValue(), "sorted") == 0 ? true : false);

	sContributionClass = "";
}

/// Calcul du Normalized Odds Ratio (NOR)
Continuous ISDRClassifierContribution::ComputeNormalizedOddsRatio(int nAttributeIndex, int nTargetClassIndex,
								  IntVector* ivModalityIndexes, int nDatabaseSize,
								  int nTargetValuesNumber) const
{

	ContinuousVector* cvScoreVector;
	Continuous cInitialScore;
	Continuous cScoreWithoutOneVariable;
	Continuous cInitialScoreCorrected;
	Continuous cScoreWithoutOneVariableCorrected;
	Continuous cImportanceValueCorrected;
	Continuous cRatioCorrected;

	// Calcul du score initial p(C|X)
	cvScoreVector = ComputeScoreVectorLj(ivModalityIndexes);
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Calcul du score sans prendre en compte la variable explicative p(C|X\X_i)
	cvScoreVector = ComputeScoreVectorLjWithoutOneVariable(ivModalityIndexes, nAttributeIndex);
	cScoreWithoutOneVariable = ComputeScoreFromScoreVector(cvScoreVector, nTargetClassIndex);
	delete cvScoreVector;

	// Commentaires sur ce code voir fonction "Weight of Evidence"
	// on calcule le odd ratio entre P(C|X) et (P(C|X) priv� de la variable
	// puis on normalise pour avoir des valeurs entre -1 et +1
	// cela corresponds au fait de faire passer le weight of evidence dans une sigmoide
	cInitialScoreCorrected =
	    (cInitialScore + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));
	cScoreWithoutOneVariableCorrected =
	    (cScoreWithoutOneVariable + (0.5 / (nTargetValuesNumber * nDatabaseSize))) / (1.0 + (0.5 / nDatabaseSize));
	cRatioCorrected = (cInitialScoreCorrected * (1 - cScoreWithoutOneVariableCorrected)) /
			  ((1 - cInitialScoreCorrected) * cScoreWithoutOneVariableCorrected);
	cImportanceValueCorrected = (cRatioCorrected - 1) / (cRatioCorrected + 1);

	if (cImportanceValueCorrected < -1 or cImportanceValueCorrected > 1)
		AddWarning(" Normalized Odds Ratio value is " + ALString(DoubleToString(cImportanceValueCorrected)) +
			   ". Should be >= -1 and <= 1");

	return cImportanceValueCorrected;
}

ContinuousVector* ISDRClassifierContribution::ComputeScoreVectorLj(IntVector* ivModalityIndexes) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		ISTargetValueProbas* targetValueProbas =
		    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->cProbaApriori);

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

ContinuousVector* ISDRClassifierContribution::ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
										     int nVariableIndex) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		ISTargetValueProbas* targetValueProbas =
		    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->cProbaApriori);

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

Continuous ISDRClassifierContribution::ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector,
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
// Classe ISDRContributionValueAt

ISDRContributionValueAt::ISDRContributionValueAt()
{
	SetName("ContributionValueAt");
	SetLabel("Variable importance value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreContribution"); // resultats de la contribution au score, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

ISDRContributionValueAt::~ISDRContributionValueAt() {}

KWDerivationRule* ISDRContributionValueAt::Create() const
{
	return new ISDRContributionValueAt;
}

Continuous ISDRContributionValueAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	ISDRClassifierContribution* scoreInterpretation =
	    cast(ISDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionValueAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRContributionNameAt

ISDRContributionNameAt::ISDRContributionNameAt()
{
	SetName("ContributionNameAt");
	SetLabel("Variable importance name");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreContribution"); // resultats de la contribution au score, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

ISDRContributionNameAt::~ISDRContributionNameAt() {}

KWDerivationRule* ISDRContributionNameAt::Create() const
{
	return new ISDRContributionNameAt;
}

Symbol ISDRContributionNameAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ISDRClassifierContribution* scoreInterpretation =
	    cast(ISDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionNameAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRContributionClass

ISDRContributionClass::ISDRContributionClass()
{
	SetName("ContributionClass");
	SetLabel("Contribution class");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreContribution"); // resultats de la contribution au score, pour une classe cible donnee
}

ISDRContributionClass::~ISDRContributionClass() {}

KWDerivationRule* ISDRContributionClass::Create() const
{
	return new ISDRContributionClass;
}

Symbol ISDRContributionClass::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ISDRClassifierContribution* scoreInterpretation =
	    cast(ISDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionClass();
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRContributionPartitionAt

ISDRContributionPartitionAt::ISDRContributionPartitionAt()
{
	SetName("ContributionPartitionAt");
	SetLabel("Variable importance partition");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreContribution"); // resultats de la contribution au score, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

ISDRContributionPartitionAt::~ISDRContributionPartitionAt() {}

KWDerivationRule* ISDRContributionPartitionAt::Create() const
{
	return new ISDRContributionPartitionAt;
}

Symbol ISDRContributionPartitionAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ISDRClassifierContribution* scoreInterpretation =
	    cast(ISDRClassifierContribution*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreInterpretation->GetContributionPartitionAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
							       1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRContributionPriorClass

ISDRContributionPriorClass::ISDRContributionPriorClass()
{
	SetName("ContributionPriorClass");
	SetLabel("Prior for class contribution");
	SetType(KWType::Continuous);
	SetOperandNumber(4);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGrid");
	GetSecondOperand()->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Symbol); // classe predite (attribut de prediction)
	GetOperandAt(3)->SetType(
	    KWType::Symbol); // classe de la contribution (peut etre la classe de meilleur gain pour cet individu)
	nTotalFrequency = 0;
}

ISDRContributionPriorClass::~ISDRContributionPriorClass() {}

KWDerivationRule* ISDRContributionPriorClass::Create() const
{
	return new ISDRContributionPriorClass;
}

void ISDRContributionPriorClass::Compile(KWClass* kwcOwnerClass)
{
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;

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
	for (int nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		svTargetValues.SetAt(nTarget, targetSymbolValueSet->GetValueAt(nTarget));

	nTotalFrequency = 0;

	// Memorisation des effectifs par partie cible
	ivDataGridSetTargetFrequencies.SetSize(nTargetValueNumber);
	for (int nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		ivDataGridSetTargetFrequencies.SetAt(nTarget, targetDataGrid->GetCellFrequencyAt(nTarget));
		nTotalFrequency += targetDataGrid->GetCellFrequencyAt(nTarget);
	}
}

Continuous ISDRContributionPriorClass::ComputeContinuousResult(const KWObject* kwoObject) const
{

	if (nTotalFrequency == 0)
		return 0;

	Symbol starget = GetSecondOperand()->GetSymbolValue(kwoObject);

	if (starget == PREDICTED_CLASS_LABEL)
	{
		KWAttribute* predictedClassAttribute = kwcClass->LookupAttribute(GetOperandAt(2)->GetAttributeName());
		starget = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
	}
	else
	{
		if (starget == CLASS_OF_HIGHEST_GAIN_LABEL)
		{
			KWAttribute* contributiobnClassAttribute =
			    kwcClass->LookupAttribute(GetOperandAt(3)->GetAttributeName());
			starget = contributiobnClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);
		}
	}

	for (int nIndex = 0; nIndex < svTargetValues.GetSize(); nIndex++)
	{
		if (starget == svTargetValues.GetAt(nIndex))
		{
			return (double)ivDataGridSetTargetFrequencies.GetAt(nIndex) / (double)nTotalFrequency;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRClassifierReinforcement

ISDRClassifierReinforcement::ISDRClassifierReinforcement()
{
	SetName("ScoreReinforcement");
	SetLabel("Classifier score contribution");
	SetType(KWType::Structure);
	SetStructureName("ScoreReinforcement");
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier"); // classifieur
	GetSecondOperand()->SetType(KWType::Symbol); // classe predite pour l'individu (via l'attribut de prediction)
	GetOperandAt(2)->SetType(KWType::Symbol);    // classe cible pour le calcul du renforcement
	cInitialScore = 0.0;
	nTargetValuesNumberInNBScore = 0;
	oaInstanceProbabilities = NULL;
}

ISDRClassifierReinforcement::~ISDRClassifierReinforcement() {}

KWDerivationRule* ISDRClassifierReinforcement::Create() const
{
	return new ISDRClassifierReinforcement;
}

Object* ISDRClassifierReinforcement::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// evaluer l'operande classifieur
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Calcul des donnees d'interpretation
	ComputeReinforcement(kwoObject);

	return (Object*)this;
}

Continuous ISDRClassifierReinforcement::GetReinforcementInitialScore() const
{

	assert(oaInstanceProbabilities != NULL);

	if (oaInstanceProbabilities->GetSize() == 0)
		return 0; // pas de reenforcement pour cet individu

	return cInitialScore;
}

Continuous ISDRClassifierReinforcement::GetReinforcementFinalScoreAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);

	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0; // pas de reenforcement pour cet individu et ce rang

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->cReinforcementNewScore;
}

Continuous ISDRClassifierReinforcement::GetReinforcementClassChangeTagAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);

	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return 0; // pas de reenforcement pour cet individu et ce rang

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	return attributeProbas->cReinforcementClassHasChanged;
}

Symbol ISDRClassifierReinforcement::GetReinforcementNameAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);

	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return ""; // pas de reenforcement pour cet individu et ce rang

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));
	StringObject* soVariableName =
	    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(attributeProbas->iAttributeIndex));
	ALString as = soVariableName->GetString();
	return as.GetBuffer(as.GetLength());
}

Symbol ISDRClassifierReinforcement::GetReinforcementPartitionAt(int rank) const
{

	assert(oaInstanceProbabilities != NULL);

	if (oaInstanceProbabilities->GetSize() < rank + 1)
		return ""; // pas de reenforcement pour cet individu et ce rang

	ISPartitionedAttributeProbas* attributeProbas =
	    cast(ISPartitionedAttributeProbas*, oaInstanceProbabilities->GetAt(rank));

	Symbol attributeName = GetReinforcementNameAt(rank);

	// Extraction de l'attribut du dictionnaire associe a ce nom de variable
	KWAttribute* attribute = kwcClass->LookupAttribute(attributeName.GetValue());

	// Ecriture de la partie (intervalle ou groupe) a laquelle appartient
	// l'individu pour cette variable
	KWDataGridStats* dataGridStats = new KWDataGridStats;
	cast(KWDRDataGrid*, attribute->GetDerivationRule())->ExportDataGridStats(dataGridStats);
	ostringstream oss;
	dataGridStats->GetAttributeAt(0)->WritePartAt(oss, attributeProbas->iModalityIndex);
	delete dataGridStats;

	return oss.str().c_str();
}

void ISDRClassifierReinforcement::ComputeReinforcement(const KWObject* kwoObject) const
{
	assert(IsCompiled());

	KWDRIntervalBounds discretizationRuleRef;
	KWDRValueGroups groupingRuleRef;
	IntVector ivModalityIndexes;

	ivModalityIndexes.SetSize(oaPartitionedPredictiveAttributeNames.GetSize());

	// Parcours des variables contribuant au predicteur
	// On va memoriser, pour chaque variable predictive, l'index de la partie (valeur variable partionnee) pour
	// l'individu courant
	for (int nAttributeIndex = 0; nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize();
	     nAttributeIndex++)
	{
		// Extraction de la variable partitionnee
		KWAttribute* predictiveAttribute = kwcClass->LookupAttribute(
		    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex))->GetString());

		// Extraction de l'index de l'attribut natif
		KWLoadIndex nNativeIndex =
		    kwcClass
			->LookupAttribute(
			    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetString())
			->GetLoadIndex();

		int nModalityIndex = -1;

		// Extraction du label de la regle permettant de savoir s'il s'agit d'une regle de discretisation ou de
		// groupage
		const ALString sRuleLabel =
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

	Symbol sReinforcementClass = GetOperandAt(2)->GetSymbolConstant();

	KWAttribute* predictedClassAttribute = kwcClass->LookupAttribute(GetSecondOperand()->GetAttributeName());
	Symbol sPredictedClass = predictedClassAttribute->GetDerivationRule()->ComputeSymbolResult(kwoObject);

	// Calcul du vecteur de scores de l'individu
	ContinuousVector* cvScoreVector = ComputeScoreVectorLj(&ivModalityIndexes);

	if (oaInstanceProbabilities != NULL)
		oaInstanceProbabilities->DeleteAll();
	else
	{
		oaInstanceProbabilities = new ObjectArray;
		oaInstanceProbabilities->SetCompareFunction(ISCompareReinforcementNewScore);
	}

	// Calcul du score associe pour la classe de reenforcement
	cInitialScore = ComputeScoreFromScoreVector(cvScoreVector, nTargetValuesNumberInNBScore);

	// Calcul du tableau des valeurs d'amelioration de score
	ComputeReinforcementProbas(&ivModalityIndexes, sPredictedClass, cvScoreVector, nTargetValuesNumberInNBScore);

	delete cvScoreVector;
}

void ISDRClassifierReinforcement::ComputeReinforcementProbas(IntVector* ivModalityIndexes, Symbol sPredictedClass,
							     ContinuousVector* cvBestScore, int nHowNumber) const
{
	KWAttribute* attribute;
	KWAttribute* partitionedAttribute;
	ISPartitionedAttributeProbas* partitionedAttributeProbas;
	ContinuousVector* cvScore;
	Continuous cBestScore;
	Continuous cScore;
	Continuous cMaxScore;
	Continuous cNewPredictedClassIsHowReferenceClass = -1;
	ALString sNativeVariableName;
	ALString sPartitionedVariableName;
	int nBestModalityIndex = -1;
	int nModalityNumber;

	assert(oaInstanceProbabilities != NULL);
	assert(oaInstanceProbabilities->GetSize() == 0);

	ISTargetValueProbas* targetValueProbas = cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nHowNumber));

	// Parcours des variables natives contribuant au NB a interpreter
	for (int nAttributeIndex = 0; nAttributeIndex < oaNativePredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		// Extraction du nom de la variable native
		sNativeVariableName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetString();

		// Extraction de l'attribut natif du dictionnaire
		attribute = kwcClass->LookupAttribute(sNativeVariableName);

		// Cas ou cette variable est une variable levier selectionnee
		if (attribute->GetConstMetaData()->GetStringValueAt(LEVER_ATTRIBUTE_META_TAG) == "true")
		{
			// Extraction du nom de la variable explicative
			sPartitionedVariableName =
			    cast(StringObject*, oaPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex))
				->GetString();

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
			for (int nModalityIndex = 0; nModalityIndex < nModalityNumber; nModalityIndex++)
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

						// Calcul d'un tag indiquant le changement de classe eventuel
						// qu'impliquerait le renforcement Cas ou la classe predite pour cet
						// individu est deja la classe de reference

						if (sPredictedClass == targetValueProbas->sTargetValue)
							cNewPredictedClassIsHowReferenceClass = 0;
						else
						{
							// Calcul du max des nouveaux scores pour les autres classes que
							// la classe de reference Initialisation du max des scores
							// conditionnellement aux autres classes
							cMaxScore = 0;

							// Parcours des classes
							for (int nClassIndex = 0;
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
							// Alors le renforcement permet le changement de classe pour la
							// classe de reference
							if (cScore > cMaxScore)
								cNewPredictedClassIsHowReferenceClass = 1;

							// Sinon : pas de changement de classe ou vers une autre classe
							// que la classe de reference
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
				partitionedAttributeProbas = new ISPartitionedAttributeProbas;

				partitionedAttributeProbas->iAttributeIndex = nAttributeIndex;

				assert(nBestModalityIndex != -1);
				partitionedAttributeProbas->iModalityIndex = nBestModalityIndex;

				partitionedAttributeProbas->cReinforcementNewScore = cBestScore;

				// Memorisation du tag indiquant si la nouvelle classe predite est la classe de
				// reference du pourquoi
				partitionedAttributeProbas->cReinforcementClassHasChanged =
				    cNewPredictedClassIsHowReferenceClass;

				oaInstanceProbabilities->Add(partitionedAttributeProbas);
			}
		}
	}
	// Tri selon le score
	oaInstanceProbabilities->Sort();
}

ContinuousVector* ISDRClassifierReinforcement::ComputeScoreVectorVariation(ContinuousVector* cvPreviousScoreVector,
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
		cvNewScoreVector->Add(
		    cvPreviousScoreVector->GetAt(nClassIndex) -
		    cvVariableWeights.GetAt(nAttributeIndex) *
			(ExtractLogPosteriorProba(nClassIndex, nAttributeIndex, nPreviousModalityIndex) -
			 ExtractLogPosteriorProba(nClassIndex, nAttributeIndex, nNewModalityIndex)));

		// De la forme
		// -> Add(P � W *((OLD) � (NEW)))
	}

	return cvNewScoreVector;
}

void ISDRClassifierReinforcement::Compile(KWClass* kwcOwnerClass)
{

	ISDRClassifierInterpretation::Compile(kwcOwnerClass); // code classe ancetre

	cInitialScore = 0;

	nTargetValuesNumberInNBScore = -1;

	Symbol sReinforcementClass = GetOperandAt(2)->GetSymbolConstant();

	// Parcours des valeurs cible
	for (int nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{

		ISTargetValueProbas* targetValueProbas =
		    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

		if (targetValueProbas->sTargetValue == sReinforcementClass)
		{
			// Memorisation de l'index du vecteur de probas pour cette classe
			nTargetValuesNumberInNBScore = nClassIndex;
			break;
		}
	}

	assert(nTargetValuesNumberInNBScore != -1);
}

ContinuousVector* ISDRClassifierReinforcement::ComputeScoreVectorLj(IntVector* ivModalityIndexes) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		ISTargetValueProbas* targetValueProbas =
		    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->cProbaApriori);

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

ContinuousVector* ISDRClassifierReinforcement::ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
										      int nVariableIndex) const
{
	ContinuousVector* cvScore;
	int nClassIndex;
	int nAttributeIndex;

	require(oaModelProbabilities.GetSize() > 0);
	require(ivModalityIndexes != NULL);

	cvScore = new ContinuousVector;
	cvScore->SetSize(oaModelProbabilities.GetSize());

	// Parcours des classes
	for (nClassIndex = 0; nClassIndex < oaModelProbabilities.GetSize(); nClassIndex++)
	{
		// Extraction du vecteur de probabilites pour la classe en cours
		ISTargetValueProbas* targetValueProbas =
		    cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));
		assert(targetValueProbas->oaProbasAposteriori->GetSize() == ivModalityIndexes->GetSize());

		// Initialisation du score au log de la proba a priori
		cvScore->SetAt(nClassIndex, targetValueProbas->cProbaApriori);

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

Continuous ISDRClassifierReinforcement::ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector,
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
// Classe ISDRReinforcementInitialScore

ISDRReinforcementInitialScore::ISDRReinforcementInitialScore()
{
	SetName("ReinforcementInitialScore");
	SetLabel("Reinforcement initial score");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreReinforcement"); // resultats du renforcement, pour une classe cible donnee
}

ISDRReinforcementInitialScore::~ISDRReinforcementInitialScore() {}

KWDerivationRule* ISDRReinforcementInitialScore::Create() const
{
	return new ISDRReinforcementInitialScore;
}

Continuous ISDRReinforcementInitialScore::ComputeContinuousResult(const KWObject* kwoObject) const
{
	ISDRClassifierReinforcement* scoreReinforcement =
	    cast(ISDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementInitialScore();
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRReinforcementFinalScoreAt

ISDRReinforcementFinalScoreAt::ISDRReinforcementFinalScoreAt()
{
	SetName("ReinforcementFinalScoreAt");
	SetLabel("Reinforcement final score");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(
	    "ScoreReinforcement"); // resultats du renforcement, pour une classe cible donnee
	GetSecondOperand()->SetType(
	    KWType::Continuous); // rang d'importance de la variable pour laquelle ont veut obtenir les resultats
}

ISDRReinforcementFinalScoreAt::~ISDRReinforcementFinalScoreAt() {}

KWDerivationRule* ISDRReinforcementFinalScoreAt::Create() const
{
	return new ISDRReinforcementFinalScoreAt;
}

Continuous ISDRReinforcementFinalScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	ISDRClassifierReinforcement* scoreReinforcement =
	    cast(ISDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementFinalScoreAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
								1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRReinforcementNameAt

ISDRReinforcementNameAt::ISDRReinforcementNameAt()
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

ISDRReinforcementNameAt::~ISDRReinforcementNameAt() {}

KWDerivationRule* ISDRReinforcementNameAt::Create() const
{
	return new ISDRReinforcementNameAt;
}

Symbol ISDRReinforcementNameAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ISDRClassifierReinforcement* scoreReinforcement =
	    cast(ISDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementNameAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRReinforcementPartitionAt

ISDRReinforcementPartitionAt::ISDRReinforcementPartitionAt()
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

ISDRReinforcementPartitionAt::~ISDRReinforcementPartitionAt() {}

KWDerivationRule* ISDRReinforcementPartitionAt::Create() const
{
	return new ISDRReinforcementPartitionAt;
}

Symbol ISDRReinforcementPartitionAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ISDRClassifierReinforcement* scoreReinforcement =
	    cast(ISDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementPartitionAt((int)GetSecondOperand()->GetContinuousValue(kwoObject) -
							       1);
}

///////////////////////////////////////////////////////////////////////////////
// Classe ISDRReinforcementClassChangeTagAt

ISDRReinforcementClassChangeTagAt::ISDRReinforcementClassChangeTagAt()
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

ISDRReinforcementClassChangeTagAt::~ISDRReinforcementClassChangeTagAt() {}

KWDerivationRule* ISDRReinforcementClassChangeTagAt::Create() const
{
	return new ISDRReinforcementClassChangeTagAt;
}

Continuous ISDRReinforcementClassChangeTagAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	ISDRClassifierReinforcement* scoreReinforcement =
	    cast(ISDRClassifierReinforcement*, GetFirstOperand()->GetStructureValue(kwoObject));
	return scoreReinforcement->GetReinforcementClassChangeTagAt(
	    (int)GetSecondOperand()->GetContinuousValue(kwoObject) - 1);
}