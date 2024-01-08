// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSelectiveNaiveBayes.h"

NumericKeyDictionary* KWPredictorSelectiveNaiveBayes::OptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	NumericKeyDictionary* nkdSelectedAttributes = NULL;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// MS_FFWBW: multi-start de gloutons FFWBW
	if (GetSelectionParameters()->GetOptimizationAlgorithm() == "MS_FFWBW")
	{
		nkdSelectedAttributes = MSOptimizeAttributeSelection(dCurrentPredictorCost);
	}
	// OPT: algorithme optimal par exploration exhaustive
	else if (GetSelectionParameters()->GetOptimizationAlgorithm() == "OPT")
	{
		// On se limite a 25 attributs potentiels
		if (predictorSelectionScoreManager->GetDataPreparationBase()
			->GetDataPreparationUsedAttributes()
			->GetSize() > 25)
		{
			AddMessage("No exhaustive search optimization for numbers of variables above 25");
		}
		else
		{
			nkdSelectedAttributes = OPTOptimizeAttributeSelection(dCurrentPredictorCost);
		}
	}
	// FW: Forward
	else if (GetSelectionParameters()->GetOptimizationAlgorithm() == "FW")
	{
		FWOptimizeAttributeSelection(dCurrentPredictorCost);
	}
	// FWBW: Forward Backward
	else if (GetSelectionParameters()->GetOptimizationAlgorithm() == "FWBW")
	{
		FWBWOptimizeAttributeSelection(dCurrentPredictorCost);
	}
	// FFW: Fast Forward
	else if (GetSelectionParameters()->GetOptimizationAlgorithm() == "FFW")
	{
		FFWOptimizeAttributeSelection(dCurrentPredictorCost);
	}
	// FFWBW: Fast Backward Forward
	else if (GetSelectionParameters()->GetOptimizationAlgorithm() == "FFWBW")
	{
		FFWBWOptimizeAttributeSelection(dCurrentPredictorCost);
	}

	// Creation si necessaire du dictionnaire d'attributs selectionnes
	if (nkdSelectedAttributes == NULL)
	{
		assert(GetSelectionParameters()->GetOptimizationAlgorithm() != "OPT" or
		       GetSelectionParameters()->GetOptimizationAlgorithm() != "MS_FFWBW");
		nkdSelectedAttributes = predictorSelectionScoreManager->GetSelectedAttributes()->Clone();
	}
	ensure(GetSelectionParameters()->GetOptimizationAlgorithm() != "OPT" or
	       GetSelectionParameters()->GetOptimizationAlgorithm() != "MS_FFWBW" or
	       CheckSelectionScore(dCurrentPredictorCost));
	return nkdSelectedAttributes;
}

NumericKeyDictionary* KWPredictorSelectiveNaiveBayes::MSOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	NumericKeyDictionary* nkdSelectedAttributes = NULL;
	int nRandomGreedyStepNumber;
	int nAttributeNumber;
	KWDataPreparationBase* dataPreparationBase;
	ALString sTmp;
	int nInitialSeed;
	double dTestedPredictorCost;
	int nStep;

	require(CheckSelectionScore(dCurrentPredictorCost));
	require(GetLearningSpec()->IsTargetStatsComputed());

	// Libelle de tache
	TaskProgression::DisplayLabel("Variable selection");

	// Memorisation du contexte initial
	nInitialSeed = GetRandomSeed();

	// On fixe la graine
	SetRandomSeed(1);

	// Statistiques sur le jeu de donnees
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	nAttributeNumber = dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize();

	// Calcul du nombre d'etapes d'optimisation: log(N)+log(K)
	nRandomGreedyStepNumber = GetSelectionParameters()->GetOptimizationLevel() - 1;
	if (nRandomGreedyStepNumber < 0)
		nRandomGreedyStepNumber =
		    (int)ceil((log(GetInstanceNumber() + 1.0) + log(nAttributeNumber + 1.0)) / log(2.0));

	// Optimisation gloutonne avec le tri initial des attributs
	TaskProgression::DisplayLabel("Variable selection: step 1");
	FFWBWOptimizeAttributeSelection(dCurrentPredictorCost);
	RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::GlobalOptimum, NULL, dCurrentPredictorCost);

	// Memorisation du dictionnaire d'attributs de l'evaluation intermediaire, memorisant un optimum local
	// (qui ne correspond pas necessairement a une amelioration globale)
	nkdSelectedAttributes = predictorSelectionScoreManager->GetSelectedAttributes()->Clone();

	// On recommence le glouton avec des perturbations aleatoires de l'ordre des attributs
	for (nStep = 0; nStep < nRandomGreedyStepNumber; nStep++)
	{
		// Taux d'avancement
		TaskProgression::DisplayLabel(sTmp + "Variable selection: step " + IntToString(nStep + 2));
		TaskProgression::DisplayProgression((int)(100 * (nStep + 1.0) / (nRandomGreedyStepNumber + 1)));
		if (StopTraining(dataPreparationBase))
			break;

		// Reinitialisation de la solution
		predictorSelectionScoreManager->InitializeWorkingData();
		dTestedPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();
		RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::ForcedRemoveAll, NULL, dTestedPredictorCost);

		// Mise a jour du taux d'avancement (apres l'etape des attributs obligatoires)
		TaskProgression::DisplayLabel(sTmp + "Variable selection: step " + IntToString(nStep + 2));
		TaskProgression::DisplayProgression((int)(100 * (nStep + 1.0) / (nRandomGreedyStepNumber + 1)));
		if (StopTraining(dataPreparationBase))
			break;

		// Perturbation de l'ordre des attributs
		predictorSelectionScoreManager->GetDataPreparationBase()->ShuffleDataPreparationUsedAttributes();

		// Optimisation gloutonne avec le nouvel ordre (aleatoire) des attributs
		FFWBWOptimizeAttributeSelection(dTestedPredictorCost);
		RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::LocalOptimum, NULL, dTestedPredictorCost);

		// Test si amelioration globale
		// Memorisation dans le classifieur courant si amelioration globale
		if (dTestedPredictorCost < dCurrentPredictorCost - dEpsilon)
		{
			// Le meilleur cout est memorise
			dCurrentPredictorCost = dTestedPredictorCost;

			// Memorisation egalement de la selection d'attributs
			check(nkdSelectedAttributes);
			delete nkdSelectedAttributes;
			nkdSelectedAttributes = predictorSelectionScoreManager->GetSelectedAttributes()->Clone();

			// Enregistrement du nouvel optimum global
			RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::GlobalOptimum, NULL,
						 dCurrentPredictorCost);
		}
	}

	// Restitution du contexte initial
	SetRandomSeed(nInitialSeed);
	dataPreparationBase->RestoreDataPreparationUsedAttributes();

	// Attention, pas d'ensure(CheckSelectionScore(dCurrentPredictorCost)) a ce niveau, car la procedure
	// d'optimisation FFWBWOptimizeAttributeSelection peut changer completement le contexte de
	// bufferisation des calculs temporaires
	return nkdSelectedAttributes;
}

void KWPredictorSelectiveNaiveBayes::FFWBWOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	boolean bTrace = false;
	boolean bShuffleAttributes = true;
	double dPreviousPredictorCost;
	boolean bContinue;
	const int nMaxStepLevel = 2;
	int nMaxStepNumber;
	int nStep;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// Trace
	if (bTrace)
		cout << "FFWBWOptimizeAttributeSelection\t\t" << dCurrentPredictorCost << endl;

	// Nombre d'etapes d'optimisation fixe a 2
	// Permet de controler la complexite algorithmique
	// L'essentiel de l'optimisation est effectuer dans les premieres passes. Au dela, il
	// est preferable de depenser la CPU en faisant de nouveaux starts
	nMaxStepNumber = nMaxStepLevel;

	// On effectue des passes de supression puis ajout tant qu'il y a amelioration
	bContinue = true;
	nStep = 0;
	while (bContinue and nStep < nMaxStepNumber)
	{
		nStep++;

		// Passe d'ajout
		dPreviousPredictorCost = dCurrentPredictorCost;
		FFWOptimizeAttributeSelection(dCurrentPredictorCost);
		bContinue = bContinue or dCurrentPredictorCost < dPreviousPredictorCost - dEpsilon;
		if (bTrace)
			cout << "\tFFW\t" << dCurrentPredictorCost << "\t" << nStep << "\t" << bContinue << endl;

		// Perturbation de l'ordre des attributs
		if (bShuffleAttributes)
			predictorSelectionScoreManager->GetDataPreparationBase()
			    ->ShuffleDataPreparationUsedAttributes();

		// Passe de suppression
		dPreviousPredictorCost = dCurrentPredictorCost;
		FBWOptimizeAttributeSelection(dCurrentPredictorCost);
		bContinue = dCurrentPredictorCost < dPreviousPredictorCost - dEpsilon;
		if (bTrace)
			cout << "\tFBW\t" << dCurrentPredictorCost << "\t" << nStep << "\t" << bContinue << endl;

		// Perturbation de l'ordre des attributs
		if (bShuffleAttributes)
			predictorSelectionScoreManager->GetDataPreparationBase()
			    ->ShuffleDataPreparationUsedAttributes();
	}

	ensure(CheckSelectionScore(dCurrentPredictorCost));
}

void KWPredictorSelectiveNaiveBayes::FFWOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	ALString sTmp;
	double dNewPredictorCost;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationBase* dataPreparationBase;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Forward selection");

	// Boucle d'ajout d'attributs
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	dataPreparationAttribute = NULL;
	for (nAttribute = 0; nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	     nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

		// Evaluation si attribut non deja utilise
		if (not predictorSelectionScoreManager->IsAttributeSelected(dataPreparationAttribute))
		{
			// Taux d'avancement
			TaskProgression::DisplayProgression(
			    (int)(100 * (nAttribute * 1.0) /
				  dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize()));
			if (StopTraining(dataPreparationBase))
				break;

			// Ajout de l'attribut dans la selection
			predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);

			// Evaluation de la nouvelle selection d'attributs
			dNewPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::Add, dataPreparationAttribute,
						 dNewPredictorCost);

			// Test si amelioration
			if (dNewPredictorCost < dCurrentPredictorCost - dEpsilon)
			{
				dCurrentPredictorCost = dNewPredictorCost;

				// Enregistrement de l'evaluation
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::BestAdd,
							 dataPreparationAttribute, dCurrentPredictorCost);

				// Libelle de tache
				TaskProgression::DisplayLabel(
				    sTmp + IntToString(predictorSelectionScoreManager->GetSelectedAttributeNumber()) +
				    " (Add " + dataPreparationAttribute->GetPreparedAttribute()->GetName() + ")");
			}
			// Sinon, on enleve l'attribut ajoute
			else
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);
		}
	}

	// Fin de tache
	TaskProgression::EndTask();
	ensure(CheckSelectionScore(dCurrentPredictorCost));
}

void KWPredictorSelectiveNaiveBayes::FBWOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	ALString sTmp;
	double dNewPredictorCost;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationBase* dataPreparationBase;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Backward Selection");

	// Boucle de supression d'attributs
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	dataPreparationAttribute = NULL;
	for (nAttribute = dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize() - 1; nAttribute >= 0;
	     nAttribute--)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

		// Evaluation si attribut non obligatoire utilise
		if (predictorSelectionScoreManager->IsAttributeSelected(dataPreparationAttribute))
		{
			// Taux d'avancement
			TaskProgression::DisplayProgression(
			    (int)(100 - 100 * (nAttribute * 1.0) /
					    dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize()));
			if (StopTraining(dataPreparationBase))
				break;

			// Supression de l'attribut dans la selection
			predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);

			// Evaluation de la nouvelle selection d'attributs
			dNewPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::Remove, dataPreparationAttribute,
						 dNewPredictorCost);

			// Test si amelioration
			// En Backward, on retire l'attribut meme si le critere n'est pas ameliore strictement
			if (dNewPredictorCost < dCurrentPredictorCost + dEpsilon)
			{
				dCurrentPredictorCost = dNewPredictorCost;

				// Enregistrement de l'evaluation
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::BestRemove,
							 dataPreparationAttribute, dCurrentPredictorCost);

				// Libelle de tache
				TaskProgression::DisplayLabel(
				    sTmp + IntToString(predictorSelectionScoreManager->GetSelectedAttributeNumber()) +
				    " (Remove " + dataPreparationAttribute->GetPreparedAttribute()->GetName() + ")");
			}
			// Sinon, on ajoute l'attribut enleve
			else
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);
		}
	}

	// Fin de tache
	TaskProgression::EndTask();
	ensure(CheckSelectionScore(dCurrentPredictorCost));
}

void KWPredictorSelectiveNaiveBayes::FWOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	double dNewPredictorCost;
	double dBestNewPredictorCost;
	int nAttribute;
	int nBestAttribute;
	boolean bContinue;
	KWDataPreparationBase* dataPreparationBase;
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// Ajout d'attribut tant qu'il y a amelioration du score
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	bContinue = predictorSelectionScoreManager->GetSelectedAttributeNumber() <
		    dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	while (bContinue)
	{
		// Initialisation des couts
		dBestNewPredictorCost = dCurrentPredictorCost;
		nBestAttribute = -1;

		// Parcours des attributs pour identifier le meilleur ajout
		for (nAttribute = 0; nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
		     nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

			// Evaluation si attribut non deja utilise
			if (not predictorSelectionScoreManager->IsAttributeSelected(dataPreparationAttribute))
			{
				// Ajout de l'attribut dans la selection
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);

				// Evaluation de la nouvelle selection d'attributs
				dNewPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();

				// Test si amelioration
				if (dNewPredictorCost < dBestNewPredictorCost - dEpsilon)
				{
					dBestNewPredictorCost = dNewPredictorCost;
					nBestAttribute = nAttribute;
				}

				// Enregistrement de l'evaluation
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::Add, dataPreparationAttribute,
							 dNewPredictorCost);

				// On enleve l'attribut ajoute pour repasser a la selection en cours
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);
			}
		}

		// Ajout du meilleur attribut au classifieur courant
		if (nBestAttribute != -1)
		{
			// Acces au nouvel attribut
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nBestAttribute));

			// Ajout effectif de l'attribut dans la selection
			predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);

			// La version nouvelle devient la version courante
			assert(fabs(predictorSelectionScoreManager->ComputeSelectionTotalCost() -
				    dBestNewPredictorCost) < dEpsilon);
			dCurrentPredictorCost = dBestNewPredictorCost;

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::BestAdd, dataPreparationAttribute,
						 dBestNewPredictorCost);
		}

		// Mise a jour des conditions d'arret
		bContinue =
		    nBestAttribute != -1 and predictorSelectionScoreManager->GetSelectedAttributeNumber() <
						 dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	}
	ensure(CheckSelectionScore(dCurrentPredictorCost));
}

void KWPredictorSelectiveNaiveBayes::FWBWOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	double dNewPredictorCost;
	double dBestNewPredictorCost;
	int nAttribute;
	int nBestAttribute;
	boolean bContinue;
	KWDataPreparationBase* dataPreparationBase;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nSelectionOperation;
	int nBestSelectionOperation;
	int nStep;
	int nMaxStepNumber;

	require(CheckSelectionScore(dCurrentPredictorCost));

	// Ajout ou supression d'attribut tant qu'il y a amelioration du score
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	bContinue = predictorSelectionScoreManager->GetSelectedAttributeNumber() <
		    dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	nMaxStepNumber = (int)ceil(
	    log(dataPreparationBase->GetInstanceNumber() + 1.0) +
	    log(dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize() + 1.0));
	nStep = 0;
	while (bContinue and nStep < nMaxStepNumber)
	{
		nStep++;

		// Initialisation des couts
		dBestNewPredictorCost = dCurrentPredictorCost;
		nBestAttribute = -1;
		nBestSelectionOperation = 0;

		// Parcours des attributs pour identifier le meilleur ajout ou supression
		for (nAttribute = 0; nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
		     nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

			// Test d'ajout ou suppression d'un attribut
			if (not predictorSelectionScoreManager->IsAttributeSelected(dataPreparationAttribute))
				nSelectionOperation = 1;
			else
				nSelectionOperation = -1;

			// Ajout de l'attribut dans la selection
			if (nSelectionOperation == 1)
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);
			else
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);

			// Evaluation de la nouvelle selection d'attributs
			dNewPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();

			// Test si amelioration
			if (dNewPredictorCost < dBestNewPredictorCost - nSelectionOperation * dEpsilon)
			{
				dBestNewPredictorCost = dNewPredictorCost;
				nBestAttribute = nAttribute;
				nBestSelectionOperation = nSelectionOperation;
			}

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute((nSelectionOperation == 1 ? KWPredictorEvaluatedAttribute::Add
									   : KWPredictorEvaluatedAttribute::Remove),
						 dataPreparationAttribute, dNewPredictorCost);

			// On repasse a la selection en cours
			if (nSelectionOperation == 1)
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);
			else
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);
		}

		// Ajout ou supression du meilleur attribut au classifieur courant
		if (nBestAttribute != -1)
		{
			// Acces au nouvel attribut
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nBestAttribute));

			// Mise a jour des probabilite conditionnelles des classes cible
			if (nBestSelectionOperation == 1)
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);
			else
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);

			// La version nouvelle devient la version courante
			assert(fabs(predictorSelectionScoreManager->ComputeSelectionTotalCost() -
				    dBestNewPredictorCost) < dEpsilon);
			dCurrentPredictorCost = dBestNewPredictorCost;

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute((nBestSelectionOperation == 1
						      ? KWPredictorEvaluatedAttribute::BestAdd
						      : KWPredictorEvaluatedAttribute::BestRemove),
						 dataPreparationAttribute, dBestNewPredictorCost);
		}

		// Mise a jour des conditions d'arret
		bContinue =
		    nBestAttribute != -1 and predictorSelectionScoreManager->GetSelectedAttributeNumber() <
						 dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	}
	ensure(CheckSelectionScore(dCurrentPredictorCost));
}

NumericKeyDictionary* KWPredictorSelectiveNaiveBayes::OPTOptimizeAttributeSelection(double& dCurrentPredictorCost)
{
	NumericKeyDictionary* nkdSelectedAttributes;
	int nMaxSelectionID;
	int nSelectionID;
	int nAttribute;
	int nAttributeID;
	boolean bAttributInPreviousSelection;
	boolean bAttributInSelection;
	boolean bAddAttribute;
	boolean bRemoveAttribute;
	boolean bAllMandatoryAttributesInSelection;
	KWDataPreparationBase* dataPreparationBase;
	KWDataPreparationAttribute* dataPreparationAttribute;
	NumericKeyDictionary nkdMandatorySelectedAttributes;
	double dTestedPredictorCost;

	require(CheckSelectionScore(dCurrentPredictorCost));
	require(
	    predictorSelectionScoreManager->GetDataPreparationBase()->GetDataPreparationUsedAttributes()->GetSize() <=
	    25);

	////////////////////////////////////////////////////////////////////////////////
	// Gestion des selections d'attributs
	// Une selection d'attribut est identifiee par un entier compris entre 0 et
	// 2^AttributeNumber (un booleen par presence d'attribut).
	// En parccourant tous les identifiants possibles de selection, on calcul
	// a chaque fois le nombre de bits ayant changes dans la notation binaire
	// de l'entier. Un bit passant a 1 signifie l'ajout d'une attribut. Un bit
	// passant a 0 signifie la supression d'un attribut. On montre qu'en moyenne,
	// on passe d'un selection a la suivante en modifiant deux bits, dont deux attributs.

	// Calcul de l'indentifiant de selection d'attributs max
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	nMaxSelectionID = (int)pow(2.0, dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize());

	// Initialisation de la solution avec une selection vide
	predictorSelectionScoreManager->InitializeWorkingData();
	dTestedPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();
	RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::ForcedRemoveAll, NULL, dTestedPredictorCost);
	nkdSelectedAttributes = new NumericKeyDictionary;

	// Meilleur cout initial: celui de la selection vide, sauf s'il y a des attributs obligatoires
	dCurrentPredictorCost = dTestedPredictorCost;
	if (nkdMandatorySelectedAttributes.GetCount() > 0)
		dCurrentPredictorCost = DBL_MAX;

	// Parcours de toutes les selections d'attributs
	for (nSelectionID = 1; nSelectionID < nMaxSelectionID; nSelectionID++)
	{
		// Parcours des attributs de la selection pour determiner ceux qui sont a ajouter ou retrancher
		bAllMandatoryAttributesInSelection = true;
		for (nAttribute = 0; nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
		     nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));
			nAttributeID = (int)pow(2.0, nAttribute);

			// Test si attribut present ou absent de la selection courante et precedente
			bAttributInSelection = (nSelectionID & nAttributeID) != 0;
			bAttributInPreviousSelection = ((nSelectionID - 1) & nAttributeID) != 0;

			// On determine s'il faut ajouter ou retrancher l'attribut
			bAddAttribute = bAttributInSelection and not bAttributInPreviousSelection;
			bRemoveAttribute = not bAttributInSelection and bAttributInPreviousSelection;
			assert(not(bAddAttribute and bRemoveAttribute));

			// Mise a jour de l'indicateur de presence des attribut obligatoires dans la selection
			if (not bAttributInSelection and
			    nkdMandatorySelectedAttributes.Lookup(dataPreparationAttribute) != NULL)
				bAllMandatoryAttributesInSelection = false;

			// Mise a jour des probabilite conditionnelles des classes cible par ajout d'un attribut
			if (bAddAttribute)
			{
				predictorSelectionScoreManager->AddAttribute(dataPreparationAttribute);

				// Enregistrement de l'ajout de l'attribut
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::UnevaluatedAdd,
							 dataPreparationAttribute, 0);
			}
			// Mise a jour des probabilite conditionnelles des classes cible par suppression d'un attribut
			else if (bRemoveAttribute)
			{
				predictorSelectionScoreManager->RemoveAttribute(dataPreparationAttribute);

				// Enregistrement de la supression de l'attribut
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::UnevaluatedRemove,
							 dataPreparationAttribute, 0);
			}
		}

		// Evaluation d'une selection d'attributs si elle contient les attribut obligatoires
		if (bAllMandatoryAttributesInSelection)
		{
			dTestedPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();

			// Enregistrement de l'evaluation
			RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::ForcedEvaluation, NULL,
						 dTestedPredictorCost);

			// Test si amelioration globale
			// Memorisation dans le classifieur courant si amelioration globale
			if (dTestedPredictorCost < dCurrentPredictorCost - dEpsilon)
			{
				// Memorisation du cout
				dCurrentPredictorCost = dTestedPredictorCost;

				// Memorisation egalement de la selection d'attributs
				delete nkdSelectedAttributes;
				nkdSelectedAttributes =
				    predictorSelectionScoreManager->GetSelectedAttributes()->Clone();
			}
		}
	}
	return nkdSelectedAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////

boolean KWPredictorSelectiveNaiveBayes::CheckTrainWorkingData()
{
	boolean bOk = true;

	bOk = Check();
	bOk = bOk and GetClassStats() != NULL;
	bOk = bOk and weightManager != NULL;
	bOk = bOk and predictorSelectionScoreManager != NULL;
	bOk = bOk and predictorSelectionScoreManager->IsWorkingDataInitialized();
	ensure(predictorSelectionScoreManager->GetDataPreparationBase() != NULL);
	return bOk;
}

boolean KWPredictorSelectiveNaiveBayes::CheckSelectionScore(double dEvaluationScore)
{
	boolean bOk = true;

	require(CheckTrainWorkingData());
	if (not StopTraining(predictorSelectionScoreManager->GetDataPreparationBase()))
	{
		bOk = bOk and CheckTrainWorkingData();
		bOk = bOk and dEvaluationScore >= 0;
		bOk = bOk and
		      fabs(dEvaluationScore - predictorSelectionScoreManager->ComputeSelectionTotalCost()) < dEpsilon;
	}
	return bOk;
}

boolean KWPredictorSelectiveNaiveBayes::StopTraining(KWDataPreparationBase* dataPreparationBase)
{
	return dataPreparationBase->IsFillError() or TaskProgression::IsInterruptionRequested();
}

void KWPredictorSelectiveNaiveBayes::CollectSelectedAttributes(ContinuousVector* cvAttributeWeights,
							       NumericKeyDictionary* nkdSelectedAttributes)
{
	ObjectArray* oaPreparedAttributes;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWSelectedAttributeReport* predictorSelectedAttribute;

	require(CheckTrainWorkingData());
	require(nkdSelectedAttributes != NULL);

	// Supression des eventuel resultats precedents
	GetPredictorSelectionReport()->GetSelectedAttributes()->DeleteAll();

	// Creation des informations sur les attributs selectionnes
	oaPreparedAttributes = predictorSelectionScoreManager->GetDataPreparationBase()
				   ->GetDataPreparationClass()
				   ->GetDataPreparationAttributes();
	for (nAttribute = 0; nAttribute < oaPreparedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaPreparedAttributes->GetAt(nAttribute));

		// Ajout d'information si l'attribut est selectionne ou s'il a un poids non nul
		if (nkdSelectedAttributes->Lookup(dataPreparationAttribute) != NULL or
		    (cvAttributeWeights != NULL and
		     cvAttributeWeights->GetAt(dataPreparationAttribute->GetIndex()) > 0))
		{
			predictorSelectedAttribute = new KWSelectedAttributeReport;
			GetPredictorSelectionReport()->GetSelectedAttributes()->Add(predictorSelectedAttribute);

			// Initialisation des informations sur cet attribut
			predictorSelectedAttribute->SetPreparedAttributeName(
			    dataPreparationAttribute->GetPreparedAttribute()->GetName());
			predictorSelectedAttribute->SetNativeAttributeName(
			    dataPreparationAttribute->ComputeNativeAttributeName());
			predictorSelectedAttribute->SetUnivariateEvaluation(
			    dataPreparationAttribute->GetPreparedStats()->GetLevel());

			// Poids de l'attribut
			if (cvAttributeWeights != NULL and
			    cvAttributeWeights->GetAt(dataPreparationAttribute->GetIndex()) > 0)
				predictorSelectedAttribute->SetWeight(
				    cvAttributeWeights->GetAt(dataPreparationAttribute->GetIndex()));
		}
	}

	// Tri des attributs selectionnes selon l'importance predictive
	GetPredictorSelectionReport()->GetSelectedAttributes()->SetCompareFunction(KWLearningReportCompareSortValue);
	GetPredictorSelectionReport()->GetSelectedAttributes()->Sort();
}

void KWPredictorSelectiveNaiveBayes::FilterSelectedAttributes(ContinuousVector* cvAttributeWeights,
							      NumericKeyDictionary* nkdSelectedAttributes)
{
	ObjectArray* oaPreparedAttributes;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWSelectedAttributeReport* predictorSelectedAttribute;
	ObjectDictionary odRecodedDataPreparationAttributes;

	require(CheckTrainWorkingData());
	require(nkdSelectedAttributes != NULL);

	// Filtrage uniquement si demande
	if (GetSelectionParameters()->GetMaxSelectedAttributeNumber() > 0)
	{
		// Rangement des attributs de preparation dans un dictionaire indexe par nom de recodage
		oaPreparedAttributes = predictorSelectionScoreManager->GetDataPreparationBase()
					   ->GetDataPreparationClass()
					   ->GetDataPreparationAttributes();
		for (nAttribute = 0; nAttribute < oaPreparedAttributes->GetSize(); nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaPreparedAttributes->GetAt(nAttribute));

			odRecodedDataPreparationAttributes.SetAt(
			    dataPreparationAttribute->GetPreparedAttribute()->GetName(), dataPreparationAttribute);
		}

		// Tri des attributs selectionnes selon l'importance predictive
		GetPredictorSelectionReport()->GetSelectedAttributes()->SetCompareFunction(
		    KWLearningReportCompareSortValue);
		GetPredictorSelectionReport()->GetSelectedAttributes()->Sort();

		// Supression des informations sur les attributs selectionnes surnumeraires
		for (nAttribute = GetSelectionParameters()->GetMaxSelectedAttributeNumber();
		     nAttribute < GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize(); nAttribute++)
		{
			predictorSelectedAttribute =
			    cast(KWSelectedAttributeReport*,
				 GetPredictorSelectionReport()->GetSelectedAttributes()->GetAt(nAttribute));

			// Recherche de l'attribut de preparation correspondant
			dataPreparationAttribute = cast(KWDataPreparationAttribute*,
							odRecodedDataPreparationAttributes.Lookup(
							    predictorSelectedAttribute->GetPreparedAttributeName()));

			// Mise a 0 du poids de cet attribut (si les poids sont geres)
			if (cvAttributeWeights != NULL)
				cvAttributeWeights->SetAt(dataPreparationAttribute->GetIndex(), 0);

			// Supression eventuelle du dictionnaire des attributs selectionnes
			if (nkdSelectedAttributes->Lookup((NUMERIC)dataPreparationAttribute) != NULL)
				nkdSelectedAttributes->RemoveKey((NUMERIC)dataPreparationAttribute);

			// Destruction de l'objet d'information sur l'attribut
			delete predictorSelectedAttribute;
			GetPredictorSelectionReport()->GetSelectedAttributes()->SetAt(nAttribute, NULL);
		}

		// Retaillage eventuel de la liste des attributs selectionnes
		if (GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize() >
		    GetSelectionParameters()->GetMaxSelectedAttributeNumber())
			GetPredictorSelectionReport()->GetSelectedAttributes()->SetSize(
			    GetSelectionParameters()->GetMaxSelectedAttributeNumber());
	}

	// Tri des attributs selectionnes selon l'importance predictive
	GetPredictorSelectionReport()->GetSelectedAttributes()->SetCompareFunction(KWLearningReportCompareSortValue);
	GetPredictorSelectionReport()->GetSelectedAttributes()->Sort();
	ensure(GetSelectionParameters()->GetMaxSelectedAttributeNumber() == 0 or
	       GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize() <=
		   GetSelectionParameters()->GetMaxSelectedAttributeNumber());
	ensure(nkdSelectedAttributes->GetCount() <= GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize());
}

void KWPredictorSelectiveNaiveBayes::RecordEvaluatedAttribute(
    int nEvaluationType, KWDataPreparationAttribute* evaluatedDataPreparationAttribute, double dPredictorCost)
{
	double dPredictorModelCost;
	double dPredictorDataCost;

	require(predictorSelectionScoreManager != NULL);
	require(predictorSelectionScoreManager->IsWorkingDataInitialized());
	require(KWPredictorEvaluatedAttribute::Initial <= nEvaluationType and
		nEvaluationType <= KWPredictorEvaluatedAttribute::Final);
	require(evaluatedDataPreparationAttribute != NULL or
		nEvaluationType == KWPredictorEvaluatedAttribute::Initial or
		nEvaluationType == KWPredictorEvaluatedAttribute::LocalOptimum or
		nEvaluationType == KWPredictorEvaluatedAttribute::ForcedEvaluation or
		nEvaluationType == KWPredictorEvaluatedAttribute::GlobalOptimum or
		nEvaluationType == KWPredictorEvaluatedAttribute::ForcedRemoveAll or
		nEvaluationType == KWPredictorEvaluatedAttribute::Final);
	require(dPredictorCost >= 0);
	require(dPredictorCost == 0 or nEvaluationType != KWPredictorEvaluatedAttribute::UnevaluatedAdd or
		nEvaluationType != KWPredictorEvaluatedAttribute::UnevaluatedRemove);
	require(nEvaluationType == KWPredictorEvaluatedAttribute::UnevaluatedAdd or
		nEvaluationType == KWPredictorEvaluatedAttribute::UnevaluatedRemove or
		nEvaluationType == KWPredictorEvaluatedAttribute::Final or
		fabs(dPredictorCost - predictorSelectionScoreManager->ComputeSelectionTotalCost()) < dEpsilon);

	// Decomposition du nouveau cout
	dPredictorModelCost = predictorSelectionScoreManager->ComputeSelectionModelCost();
	dPredictorDataCost = dPredictorCost - dPredictorModelCost;

	// Couts nuls si pas d'evaluation
	// (ou evaluation finale, dont la selection d'attribut n'est pas celle du predictorSelectionScoreManager)
	if (nEvaluationType == KWPredictorEvaluatedAttribute::UnevaluatedAdd or
	    nEvaluationType == KWPredictorEvaluatedAttribute::UnevaluatedRemove or
	    nEvaluationType == KWPredictorEvaluatedAttribute::Final)
	{
		assert(dPredictorCost == 0 or nEvaluationType == KWPredictorEvaluatedAttribute::Final);
		dPredictorModelCost = 0;
		dPredictorDataCost = 0;
	}

	// Memorisation de l'evaluation
	if (weightManager != NULL)
	{
		weightManager->UpdateWeightEvaluation(
		    nEvaluationType, predictorSelectionScoreManager->GetSelectedAttributes(),
		    evaluatedDataPreparationAttribute, dPredictorModelCost, dPredictorDataCost);
	}
}
