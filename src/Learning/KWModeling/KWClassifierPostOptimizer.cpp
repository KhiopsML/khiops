// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassifierPostOptimizer.h"

KWClassifierPostOptimizer::KWClassifierPostOptimizer()
{
	oaSortableScores.SetCompareFunction(KWSortableContinuousCompare);
}

KWClassifierPostOptimizer::~KWClassifierPostOptimizer()
{
	CleanWorkingData();
}

boolean KWClassifierPostOptimizer::PostOptimize(KWPredictor* predictor, KWTrainedClassifier* trainedClassifier,
						ContinuousVector* cvBestTargetScoreOffsets)
{
	boolean bOptimized;
	int nTarget;

	require(predictor != NULL);
	require(predictor->GetTargetAttributeType() == KWType::Symbol);
	require(predictor->IsTraining());
	require(trainedClassifier != NULL);
	require(trainedClassifier->Check());
	require(predictor->GetTargetAttributeName() == trainedClassifier->GetTargetAttribute()->GetName());
	require(cvBestTargetScoreOffsets != NULL);

	// Si la post-optimisation est demandee
	bOptimized = false;
	if (predictor->GetTrainParameters()->GetClassifierCriterion() != "None")
	{
		// Les attributs de probabilites conditionnelles (facultatifs) doivent etre presents
		// pour pouvoir proceder a la post-optimisation
		if (trainedClassifier->GetTargetValueNumber() > 1)
		{
			assert(trainedClassifier->GetTargetValueNumber() ==
			       predictor->GetTargetDescriptiveStats()->GetValueNumber());
			ComputeBiasedClassificationOffsets(predictor, trainedClassifier, cvBestTargetScoreOffsets);

			// On determine au moins un des offsets est non nul
			for (nTarget = 0; nTarget < cvBestTargetScoreOffsets->GetSize(); nTarget++)
			{
				if (cvBestTargetScoreOffsets->GetAt(nTarget) != 0)
					bOptimized = true;
			}
		}
	}
	return bOptimized;
}

int KWClassifierPostOptimizer::ComputeClassifierCriterionIndex(KWPredictor* predictor) const
{
	int nClassifierCriterionIndex;

	require(predictor != NULL);
	require(predictor->GetTargetAttributeType() == KWType::Symbol);

	// Recherche du critere de classification
	nClassifierCriterionIndex = -1;
	if (predictor->GetTrainParameters()->GetClassifierCriterion() == "Accuracy")
		nClassifierCriterionIndex = Accuracy;
	else if (predictor->GetTrainParameters()->GetClassifierCriterion() == "BalancedAccuracy")
		nClassifierCriterionIndex = BalancedAccuracy;
	return nClassifierCriterionIndex;
}

void KWClassifierPostOptimizer::ComputeBiasedClassificationOffsets(KWPredictor* predictor,
								   KWTrainedClassifier* trainedClassifier,
								   ContinuousVector* cvBestTargetScoreOffsets)
{
	boolean bTrace = false;
	boolean bOk;
	int nTargetValueNumber;
	int nClassifierCriterionIndex;
	ContinuousVector cvTestTargetScoreOffsets;
	ContinuousVector cvTargetScoreOffsets;
	KWConfusionMatrix kwctConfusionMatrix;
	Continuous cOffset;
	int nTarget;
	int nInstance;
	Continuous cCoefMin;
	Continuous cCoefMax;
	Continuous cCoefMiddle;
	double dCriterionMin;
	double dCriterionMax;
	double dCriterionMiddle;
	double dBestCriterion;
	boolean bGlobalContinue;
	boolean bContinue;
	Continuous cMinOffset;

	require(predictor != NULL);
	require(trainedClassifier != NULL);
	require(predictor->GetTrainParameters()->GetClassifierCriterion() != "None");
	require(trainedClassifier->GetTargetValueNumber() > 1);
	require(cvBestTargetScoreOffsets != NULL);

	// Chagment des donnees de travail
	bOk = LoadWorkingData(predictor, trainedClassifier);

	// Arret si probleme de chargement
	if (not bOk)
		return;

	// Initialisation des donnees de travail
	nTargetValueNumber = trainedClassifier->GetTargetValueNumber();
	oaSortableScores.SetSize(ivTargetValueIndexes.GetSize());
	for (nInstance = 0; nInstance < oaSortableScores.GetSize(); nInstance++)
		oaSortableScores.SetAt(nInstance, new KWSortableContinuous);
	cvBestTargetScoreOffsets->SetSize(nTargetValueNumber);
	cvTestTargetScoreOffsets.SetSize(nTargetValueNumber);
	cvTargetScoreOffsets.SetSize(nTargetValueNumber);
	kwctConfusionMatrix.Initialize(nTargetValueNumber, nTargetValueNumber);

	// Calcul de l'index d'un critere de classification defini depuis les parametres d'apprentissage
	nClassifierCriterionIndex = ComputeClassifierCriterionIndex(predictor);
	assert(nClassifierCriterionIndex != -1);

	// Mode trace: matrice de confusion initiale
	if (bTrace)
	{
		FillConfusionMatrix(cvBestTargetScoreOffsets, &kwctConfusionMatrix);
		dBestCriterion = EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);
		cout << "Initial confusion matrix\t" << dBestCriterion << "\n" << kwctConfusionMatrix << endl;
	}

	// Cas particulier avec deux modalites cibles
	if (nTargetValueNumber == 2)
	{
		// La solution optimale est trouvee en parcourant toutes les instances
		// (les offsets simules sont a 0)
		cvBestTargetScoreOffsets->SetAt(
		    0, ComputeBiasedClassificationOffsetAt(nClassifierCriterionIndex, &cvTargetScoreOffsets, 0));
	}
	// Cas general
	else
	{
		/////////////////////////////////////////////////////////////////////////
		// Recherche de la matrice de confusion optimale en ajustant les scores
		// de lift par des offsets
		// On procede par un algorithme glouton en deux etapes
		//   Tant que amelioration, repeter:
		//     Recherche d'une solution optimale par modalite cible
		//      (la combinaison de ces solutions locales n'est pas forcement
		//       optimale globalement)
		//     Recherche de la meilleure solution globale par interpolation
		//      lineaire entre la meilleure solution courante et la nouvelle
		//      solution globale
		//      (recherche dichotomique par division de l'intervalle de recherche)

		// Cout de la solution initiale, sans offsets
		FillConfusionMatrix(cvBestTargetScoreOffsets, &kwctConfusionMatrix);
		dBestCriterion = EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);

		// Recherche d'une amelioration globale
		bGlobalContinue = true;
		while (bGlobalContinue)
		{
			// On trouve la solution optimale par modalites cible, a partir de la solution
			// de base (initialement offsets a 0)
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				cOffset = ComputeBiasedClassificationOffsetAt(nClassifierCriterionIndex,
									      cvBestTargetScoreOffsets, nTarget);
				cvTargetScoreOffsets.SetAt(nTarget, cOffset);
			}

			// Mode trace: Offset de depart et variation optimale (localement)
			if (bTrace)
				cout << "Base offset\n"
				     << *cvBestTargetScoreOffsets << "Delta offset\n"
				     << cvTargetScoreOffsets << endl;

			// Evaluation de la solution courante
			cCoefMin = 0;
			cvTestTargetScoreOffsets.CopyFrom(cvBestTargetScoreOffsets);
			FillConfusionMatrix(&cvTestTargetScoreOffsets, &kwctConfusionMatrix);
			dCriterionMin = EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);

			// Evaluation de la solution avec offsets complets
			cCoefMax = 1;
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				cvTestTargetScoreOffsets.SetAt(nTarget, cvBestTargetScoreOffsets->GetAt(nTarget) +
									    cvTargetScoreOffsets.GetAt(nTarget));
			}
			FillConfusionMatrix(&cvTestTargetScoreOffsets, &kwctConfusionMatrix);
			dCriterionMax = EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);

			// Mode trace: header pour recherche dichotomique de la solution
			if (bTrace)
				cout << "\tCoefMin\tCoefMiddle\tCoefMax\tCriterionMin\tCriterionMiddle\tCriterionMax"
				     << endl;

			// Amelioration de la solution
			bContinue = true;
			cCoefMiddle = (cCoefMin + cCoefMax) / 2;
			while (bContinue)
			{
				// Evaluation d'une solution intermediaire
				cCoefMiddle = (cCoefMin + cCoefMax) / 2;
				for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
					cvTestTargetScoreOffsets.SetAt(
					    nTarget, cvBestTargetScoreOffsets->GetAt(nTarget) +
							 cCoefMiddle * cvTargetScoreOffsets.GetAt(nTarget));
				FillConfusionMatrix(&cvTestTargetScoreOffsets, &kwctConfusionMatrix);
				dCriterionMiddle =
				    EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);

				// Mode trace: recherche dichotomique de la solution
				if (bTrace)
					cout << "\t" << cCoefMin << "\t" << cCoefMiddle << "\t" << cCoefMax << "\t"
					     << dCriterionMin << "\t" << dCriterionMiddle << "\t" << dCriterionMax
					     << endl;

				// Test d'arret: les trois valeurs sont identiques
				if (dCriterionMin == dCriterionMiddle and dCriterionMiddle == dCriterionMax)
					bContinue = false;

				// On se deplace vers l'intervalle le plus prometteur
				if (dCriterionMax > dCriterionMin)
				{
					cCoefMin = cCoefMiddle;
					dCriterionMin = dCriterionMiddle;
				}
				else
				{
					cCoefMax = cCoefMiddle;
					dCriterionMax = dCriterionMiddle;
				}
			}

			// Test d'amelioration globale
			bGlobalContinue = false;
			if (dCriterionMax > dBestCriterion)
			{
				bGlobalContinue = true;

				// Memorisation de la meilleure solution courante
				dBestCriterion = dCriterionMax;
				for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
					cvBestTargetScoreOffsets->SetAt(
					    nTarget, cvBestTargetScoreOffsets->GetAt(nTarget) +
							 cCoefMiddle * cvTargetScoreOffsets.GetAt(nTarget));

				// Mode trace: meilleure nouvelle solution
				if (bTrace)
					cout << "Best offsets: " << *cvBestTargetScoreOffsets << endl
					     << "New best Criterion\t" << dBestCriterion << endl;
			}
		}
	}

	// Normalisation des offsets de facon a ce qu'ils soient tous positifs
	cMinOffset = KWContinuous::GetMaxValue();
	for (nTarget = 0; nTarget < cvBestTargetScoreOffsets->GetSize(); nTarget++)
	{
		if (cvBestTargetScoreOffsets->GetAt(nTarget) < cMinOffset)
			cMinOffset = cvBestTargetScoreOffsets->GetAt(nTarget);
	}
	for (nTarget = 0; nTarget < cvBestTargetScoreOffsets->GetSize(); nTarget++)
		cvBestTargetScoreOffsets->UpgradeAt(nTarget, -cMinOffset);

	// Mode trace: matrice de confusion finale
	if (bTrace)
	{
		FillConfusionMatrix(cvBestTargetScoreOffsets, &kwctConfusionMatrix);
		dBestCriterion = EvaluateConfusionMatrix(nClassifierCriterionIndex, &kwctConfusionMatrix);
		cout << "Final confusion matrix\t" << dBestCriterion << "\n" << kwctConfusionMatrix << endl;
		cout << "Best offsets: " << *cvBestTargetScoreOffsets << endl;
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
}

Continuous KWClassifierPostOptimizer::ComputeBiasedClassificationOffsetAt(int nClassifierCriterionIndex,
									  const ContinuousVector* cvTargetScoreOffsets,
									  int nTargetIndex)
{
	boolean bTrace = false;
	int nTargetValueNumber;
	int nInstanceNumber;
	int nTarget;
	Continuous cMainTargetProb;
	Continuous cTargetProb;
	Continuous cOtherTargetProb;
	Continuous cBestOffset;
	KWConfusionMatrix confusionMatrix;
	double dBestCriterion;
	double dCriterion;
	int nBestIndex;
	int nInstance;
	int nCorrectInstanceNumber;
	Continuous cScore;
	int nLastCorrectInstanceNumber;
	Continuous cLastScore;

	require(nClassifierCriterionIndex == Accuracy or nClassifierCriterionIndex == BalancedAccuracy);
	require(oaTargetProbVectors.GetSize() > 1);
	require(0 <= nTargetIndex and nTargetIndex < oaTargetProbVectors.GetSize());
	require(cvTargetScoreOffsets != NULL);
	require(cvTargetScoreOffsets->GetSize() == oaTargetProbVectors.GetSize());

	// Initialisation de la structure de tri pour l'ensemble des instances,
	// pour la valeur cible en parametre
	nTargetValueNumber = oaTargetProbVectors.GetSize();
	nInstanceNumber = ivTargetValueIndexes.GetSize();
	cBestOffset = 0;
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		// Proba de la valeur cible d'interet
		cMainTargetProb = cast(ContinuousVector*, oaTargetProbVectors.GetAt(nTargetIndex))->GetAt(nInstance) +
				  cvTargetScoreOffsets->GetAt(nTargetIndex);

		// Calcul de la proba corrigee (avec offset) max pour les autres valeurs cibles
		cOtherTargetProb = KWContinuous::GetMinValue();
		for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		{
			if (nTarget != nTargetIndex)
			{
				cTargetProb =
				    cast(ContinuousVector*, oaTargetProbVectors.GetAt(nTarget))->GetAt(nInstance) +
				    cvTargetScoreOffsets->GetAt(nTarget);
				if (cTargetProb > cOtherTargetProb)
					cOtherTargetProb = cTargetProb;
			}
		}

		// Memorisation du score de lift: delta de la proba d'interet et du max des autres
		// On en prend l'oppose pour avoir un tri par valeur decroissante
		cScore = -(cMainTargetProb - cOtherTargetProb);
		SetScoreAt(nInstance, cScore);

		// Memorisation de l'index de la valeur cible reelle
		SetTargetIndexAt(nInstance, ivTargetValueIndexes.GetAt(nInstance));
	}

	// Tri des instances
	oaSortableScores.Sort();

	// En tete pour les infos de trace
	if (bTrace)
	{
		cout << "Score\tClass\tConfusion matrix\t\t\t\t"
		     << "Criterion\tAccuracy\t"
		     << "BestIndex\tLastCorrectInstanceNumber\tBestCriterion\tBestOffset\n";
	}

	// Recherche de la meilleure matrice de confusion
	cBestOffset = 0;
	confusionMatrix.Initialize(2, 2);
	dBestCriterion = -1;
	nBestIndex = -1;
	nCorrectInstanceNumber = 0;
	cLastScore = 0;
	nLastCorrectInstanceNumber = 0;
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		// Prise en compte des donnees de l'instance courante
		if (GetTargetIndexAt(nInstance) == nTargetIndex)
			nCorrectInstanceNumber++;
		cScore = GetScoreAt(nInstance);

		// Informations de score en mode trace
		if (bTrace)
			cout << cScore << "\t" << GetTargetIndexAt(nInstance);

		// On impacte la matrice de confusion que s'il y a un changement
		// On prend alors la version correspondant a l'instance precedente
		if (nInstance > 0 and cScore != cLastScore)
		{
			// Mise a jour de la matrice de confusion, en se basant sur les
			// donnees recuillies a l'instance precedente
			confusionMatrix.SetFrequencyAt(0, 0, nLastCorrectInstanceNumber);
			confusionMatrix.SetFrequencyAt(0, 1, nInstance - nLastCorrectInstanceNumber);
			confusionMatrix.SetFrequencyAt(
			    1, 0, ivTargetFrequencies.GetAt(nTargetIndex) - confusionMatrix.GetFrequencyAt(0, 0));
			confusionMatrix.SetFrequencyAt(1, 1,
						       nInstanceNumber - ivTargetFrequencies.GetAt(nTargetIndex) -
							   confusionMatrix.GetFrequencyAt(0, 1));

			// Informations sur la matrice de confusion en mode trace
			if (bTrace)
				cout << "\t" << confusionMatrix.GetFrequencyAt(0, 0) << "\t"
				     << confusionMatrix.GetFrequencyAt(0, 1) << "\t"
				     << confusionMatrix.GetFrequencyAt(1, 0) << "\t"
				     << confusionMatrix.GetFrequencyAt(1, 1) << "\t"
				     << EvaluateConfusionMatrix(nClassifierCriterionIndex, &confusionMatrix) << "\t"
				     << (confusionMatrix.GetFrequencyAt(0, 0) + confusionMatrix.GetFrequencyAt(1, 1)) *
					    1.0 / confusionMatrix.GetTableFrequency();

			// Test si amelioration de la valeur du critere
			dCriterion = EvaluateConfusionMatrix(nClassifierCriterionIndex, &confusionMatrix);
			if (dCriterion > dBestCriterion)
			{
				dBestCriterion = dCriterion;
				nBestIndex = nInstance - 1;

				// On prend comme offset la valeur mediane entre deux instances (si possible)
				if (nInstance < nInstanceNumber)
					cBestOffset = (GetScoreAt(nBestIndex) + GetScoreAt(nBestIndex + 1)) / 2;
				else
					cBestOffset = (Continuous)1.01 * GetScoreAt(nBestIndex);

				// Informations sur la nouvelle meilleure solution
				if (bTrace)
					cout << "\t" << nBestIndex << "\t" << nLastCorrectInstanceNumber << "\t"
					     << dBestCriterion << "\t" << cBestOffset;
			}
		}
		if (bTrace)
			cout << endl;

		// Memorisation des donnees de l'instance precedente
		cLastScore = cScore;
		nLastCorrectInstanceNumber = nCorrectInstanceNumber;
	}
	return cBestOffset;
}

void KWClassifierPostOptimizer::FillConfusionMatrix(const ContinuousVector* cvTargetScoreOffsets,
						    KWConfusionMatrix* kwctConfusionMatrix)
{
	int i;
	int j;
	int nTargetValueNumber;
	int nInstanceNumber;
	int nInstance;
	int nTarget;
	Continuous cScore;
	Continuous cMaxScore;
	int nIndexMax;

	require(cvTargetScoreOffsets != NULL);
	require(cvTargetScoreOffsets->GetSize() == oaTargetProbVectors.GetSize());
	require(kwctConfusionMatrix != NULL);
	require(kwctConfusionMatrix->GetSourceValueNumber() == oaTargetProbVectors.GetSize());
	require(kwctConfusionMatrix->GetTargetValueNumber() == oaTargetProbVectors.GetSize());

	// Reinitialisation de la matrice
	for (i = 0; i < kwctConfusionMatrix->GetSourceValueNumber(); i++)
	{
		for (j = 0; j < kwctConfusionMatrix->GetTargetValueNumber(); j++)
			kwctConfusionMatrix->SetFrequencyAt(i, j, 0);
	}

	// On remplit cette matrice de confusion par parcours des instances
	nTargetValueNumber = oaTargetProbVectors.GetSize();
	nInstanceNumber = ivTargetValueIndexes.GetSize();
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		// Recherche du score max
		cMaxScore = -1;
		nIndexMax = -1;
		for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		{
			cScore = cast(ContinuousVector*, oaTargetProbVectors.GetAt(nTarget))->GetAt(nInstance) +
				 cvTargetScoreOffsets->GetAt(nTarget);

			// Memorisation du max
			if (cScore > cMaxScore)
			{
				nIndexMax = nTarget;
				cMaxScore = cScore;
			}
		}
		assert(nIndexMax >= 0);

		// Mise a jour de la matrice de confusion
		kwctConfusionMatrix->UpgradeFrequencyAt(nIndexMax, ivTargetValueIndexes.GetAt(nInstance), 1);
	}
	ensure(kwctConfusionMatrix->GetTableFrequency() == nInstanceNumber);
}

double KWClassifierPostOptimizer::EvaluateConfusionMatrix(int nClassifierCriterionIndex,
							  KWConfusionMatrix* confusionMatrix)
{
	double dEvaluation;
	int nTarget;
	int nTrueTargetNumber;

	require(nClassifierCriterionIndex == Accuracy or nClassifierCriterionIndex == BalancedAccuracy);
	require(confusionMatrix != NULL);
	require(confusionMatrix->GetSourceValueNumber() == confusionMatrix->GetTargetValueNumber());

	// Calcul du taux de bonne prediction
	dEvaluation = 0;
	if (nClassifierCriterionIndex == Accuracy)
	{
		dEvaluation = 0;
		for (nTarget = 0; nTarget < confusionMatrix->GetSourceValueNumber(); nTarget++)
			dEvaluation += confusionMatrix->GetFrequencyAt(nTarget, nTarget);
		if (dEvaluation > 0)
			dEvaluation /= confusionMatrix->GetTableFrequency();
	}
	// Calcul du taux de bonne prediction equilibre
	else if (nClassifierCriterionIndex == BalancedAccuracy)
	{
		dEvaluation = 0;
		nTrueTargetNumber = 0;
		for (nTarget = 0; nTarget < confusionMatrix->GetSourceValueNumber(); nTarget++)
		{
			if (confusionMatrix->GetTargetFrequencyAt(nTarget) > 0)
			{
				nTrueTargetNumber++;
				dEvaluation += confusionMatrix->GetFrequencyAt(nTarget, nTarget) * 1.0 /
					       confusionMatrix->GetTargetFrequencyAt(nTarget);
			}
		}
		if (nTrueTargetNumber > 0)
			dEvaluation /= nTrueTargetNumber;
	}
	assert(dEvaluation >= 0);
	return dEvaluation;
}

boolean KWClassifierPostOptimizer::LoadWorkingData(KWPredictor* predictor, KWTrainedClassifier* trainedClassifier)
{
	boolean bOk;
	KWClassDomain* currentDomain;
	KWClassDomain* evaluationDomain;
	KWDatabase* evaluationDatabase;
	int nTargetValueNumber;
	int nTarget;
	KWAttribute* attribute;
	longint lRecordNumber;
	KWObject* kwoObject;
	Symbol sTargetValue;
	int nTargetIndex;
	KWLoadIndex liTargetAttributeLoadIndex;
	KWLoadIndexVector livProbAttributeLoadIndexes;
	KWLoadIndex liProbAttributeLoadIndex;

	require(predictor->GetTrainParameters()->GetClassifierCriterion() != "None");
	require(trainedClassifier->GetTargetValueNumber() > 1);
	require(ivTargetValueIndexes.GetSize() == 0);
	require(oaTargetProbVectors.GetSize() == 0);

	/////////////////////////////////////////////////////////////////////
	// Preparation de la base d'evaluation

	// Personnalisation du dictionnaire de deploiement pour l'evaluation
	// On passe tout en not Loaded prealablement
	trainedClassifier->GetPredictorClass()->SetAllAttributesUsed(false);

	// On charge l'attribut cible
	attribute = trainedClassifier->GetTargetAttribute();
	check(attribute);
	attribute->SetUsed(true);
	attribute->SetLoaded(true);

	// On charge les attributs de probabilites conditionnelles
	for (nTarget = 0; nTarget < trainedClassifier->GetTargetValueNumber(); nTarget++)
	{
		attribute = trainedClassifier->GetProbAttributeAt(nTarget);
		attribute->SetUsed(true);
		attribute->SetLoaded(true);
	}

	// Utilisation d'un domaine de classe dedie a l'evaluation
	currentDomain = KWClassDomain::GetCurrentDomain();
	evaluationDomain = trainedClassifier->GetPredictorDomain();
	evaluationDomain->SetName("Evaluation");
	KWClassDomain::SetCurrentDomain(evaluationDomain);
	evaluationDomain->Compile();

	// Clonage de la base d'evaluation, pour ne pas interagir avec les spec d'apprentissage en cours
	evaluationDatabase = predictor->GetDatabase()->Clone();
	evaluationDatabase->SetClassName(trainedClassifier->GetPredictorClass()->GetName());

	/////////////////////////////////////////////////////////////////////
	// Chargement de la base pour evaluation des critere specifiques

	// Preparation des donnees temporaires
	nTargetValueNumber = trainedClassifier->GetTargetValueNumber();
	oaTargetProbVectors.SetSize(trainedClassifier->GetTargetValueNumber());
	for (nTarget = 0; nTarget < trainedClassifier->GetTargetValueNumber(); nTarget++)
		oaTargetProbVectors.SetAt(nTarget, new ContinuousVector);
	ivTargetFrequencies.SetSize(nTargetValueNumber);

	// Calcul des index des attributs
	liTargetAttributeLoadIndex = trainedClassifier->GetTargetAttribute()->GetLoadIndex();
	livProbAttributeLoadIndexes.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		livProbAttributeLoadIndexes.SetAt(nTarget,
						  trainedClassifier->GetProbAttributeAt(nTarget)->GetLoadIndex());

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Load database " + evaluationDatabase->GetDatabaseName());

	// Ouverture de la base en lecture
	bOk = evaluationDatabase->OpenForRead();

	// Lecture d'objets dans la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		while (not evaluationDatabase->IsEnd())
		{
			kwoObject = evaluationDatabase->Read();
			lRecordNumber++;
			if (kwoObject != NULL)
			{
				// Memorisation de l'index de la valeur cible
				sTargetValue = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
				nTargetIndex = -1;
				for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
				{
					if (sTargetValue == trainedClassifier->GetTargetValueAt(nTarget))
					{
						nTargetIndex = nTarget;
						break;
					}
				}
				assert(nTargetIndex != -1);
				ivTargetValueIndexes.Add(nTargetIndex);

				// Mise a jour des effectif des valeurs cibles
				ivTargetFrequencies.UpgradeAt(nTargetIndex, 1);

				// Memorisation des probabilites conditionnelles
				for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
				{
					liProbAttributeLoadIndex = livProbAttributeLoadIndexes.GetAt(nTarget);
					cast(ContinuousVector*, oaTargetProbVectors.GetAt(nTarget))
					    ->Add(kwoObject->GetContinuousValueAt(liProbAttributeLoadIndex));
				}

				// Destruction de l'objet
				delete kwoObject;
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}

			// Arret si erreur ou tache interrompue
			if (evaluationDatabase->IsError())
			{
				bOk = false;
				break;
			}

			// Suivi de la tache
			if (TaskProgression::IsRefreshNecessary())
			{
				TaskProgression::DisplayProgression(
				    (int)(100 * evaluationDatabase->GetReadPercentage()));
			}
		}
		Global::DesactivateErrorFlowControl();

		// Fermeture
		bOk = evaluationDatabase->Close() and bOk;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	////////////////////////////////////
	// Nettoyage

	// Destruction de la base temporaire
	delete evaluationDatabase;

	// Restitution de l'etat initial
	KWClassDomain::SetCurrentDomain(currentDomain);
	trainedClassifier->PrepareDeploymentClass(true, false);

	// Nettoyage complet si probleme
	if (not bOk)
		CleanWorkingData();

	ensure(not bOk or oaTargetProbVectors.GetSize() == trainedClassifier->GetTargetValueNumber());
	return bOk;
}

void KWClassifierPostOptimizer::CleanWorkingData()
{
	ivTargetValueIndexes.SetSize(0);
	oaTargetProbVectors.DeleteAll();
	ivTargetFrequencies.SetSize(0);
	oaSortableScores.DeleteAll();
}

void KWClassifierPostOptimizer::DisplayWorkingData() const
{
	int nInstance;
	int nTarget;

	// Libelle d'entete pour les probabilites conditionnelles
	for (nTarget = 0; nTarget < oaTargetProbVectors.GetSize(); nTarget++)
		cout << "Prob" << nTarget + 1 << "\t";
	cout << "Class"
	     << "\n";

	// Ligne par instance
	for (nInstance = 0; nInstance < ivTargetValueIndexes.GetSize(); nInstance++)
	{
		for (nTarget = 0; nTarget < oaTargetProbVectors.GetSize(); nTarget++)
			cout << cast(ContinuousVector*, oaTargetProbVectors.GetAt(nTarget))->GetAt(nInstance) << "\t";
		cout << "c" << ivTargetValueIndexes.GetAt(nInstance) + 1 << "\n";
	}
	cout << endl;

	// Effectifs par valeur cible
	cout << "Class\tFrequency\n";
	for (nTarget = 0; nTarget < ivTargetFrequencies.GetSize(); nTarget++)
		cout << "c" << nTarget + 1 << "\t" << ivTargetFrequencies.GetAt(nTarget) << "\n";
	cout << endl;

	// Instances triees par score
	cout << "Score\tClass\n";
	for (nInstance = 0; nInstance < oaSortableScores.GetSize(); nInstance++)
		cout << GetScoreAt(nInstance) << "\tc" << GetTargetIndexAt(nInstance) + 1 << "\n";
	cout << endl;
}

void KWClassifierPostOptimizer::DisplayOffsets(const ContinuousVector* cvTargetScoreOffsets) const
{
	int nTarget;

	cout << "Class\tOffset\n";
	for (nTarget = 0; nTarget < cvTargetScoreOffsets->GetSize(); nTarget++)
		cout << "c" << nTarget + 1 << "\t" << cvTargetScoreOffsets->GetAt(nTarget) << "\n";
}

/////////////////////////////////////////////////////////////////////////////////////

KWConfusionMatrix::KWConfusionMatrix()
{
	nSourceValueNumberAtt = 0;
	nTargetValueNumberAtt = 0;
	nTableFrequency = 0;
}

KWConfusionMatrix::~KWConfusionMatrix() {}

void KWConfusionMatrix::Initialize(int nSourceValueNumber, int nTargetValueNumber)
{
	require(nSourceValueNumber >= 0);
	require(nTargetValueNumber >= 0);

	// Affectation des tailles
	nSourceValueNumberAtt = nSourceValueNumber;
	nTargetValueNumberAtt = nTargetValueNumber;
	nTableFrequency = 0;

	// Initialisation de la table d'effectifs
	// On force d'abord la taille a zero pour liberer la memoire de
	// la table precedente (potentiellement importante)
	ivTableFrequencies.SetSize(0);
	ivTableFrequencies.SetSize(nSourceValueNumber * nTargetValueNumber);

	// Creation du vecteur de la loi source
	ivSourceFrequencies.SetSize(0);
	ivSourceFrequencies.SetSize(nSourceValueNumber);

	// Creation du vecteur de la loi cible
	ivTargetFrequencies.SetSize(0);
	ivTargetFrequencies.SetSize(nTargetValueNumber);
}

void KWConfusionMatrix::Write(ostream& ost) const
{
	int nSource;
	int nTarget;

	// Libelles des valeurs cibles
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		ost << "\tTE" << nTarget + 1;
	ost << "\tTotal\n";

	// Matrice, avec statistiques par lignes
	for (nSource = 0; nSource < GetSourceValueNumber(); nSource++)
	{
		// Libelle de la valeur source
		ost << "SE" << nSource + 1;
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			ost << "\t" << GetFrequencyAt(nSource, nTarget);
		ost << "\t" << ((KWConfusionMatrix*)this)->GetSourceFrequencyAt(nSource) << "\n";
	}

	// Statistiques par colonnes, et total
	ost << "Total";
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		ost << "\t" << ((KWConfusionMatrix*)this)->GetTargetFrequencyAt(nTarget);
	ost << "\t" << ((KWConfusionMatrix*)this)->GetTableFrequency() << "\n";
}

const ALString KWConfusionMatrix::GetClassLabel() const
{
	return "Confusion matrix";
}

const ALString KWConfusionMatrix::GetObjectLabel() const
{
	ALString sTmp;
	return sTmp + IntToString(GetSourceValueNumber()) + "*" + IntToString(GetTargetValueNumber());
}
