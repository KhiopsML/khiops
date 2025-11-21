// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTree.h"
#include "DTBaseLoader.h"
#include "DTGlobalTag.h"
#include "DTConfig.h"

const ALString GetDisplayString(const double d);
const ALString GetDisplayString(const int);
const ALString GetDisplayString(const ALString&);

boolean bVerbatim = false;

DTDecisionTree::DTDecisionTree()
{
	DTDecisionTreeNode* rootNode;

	nLastNodeIdentifier = 1;
	dtPredictorParameter = NULL;
	nUsableAttributesNumber = 1;
	SelectedAttributes = NULL;
	origineBaseLoader = NULL;

	// Creation du noeud racine
	rootNode = new DTDecisionTreeNode;

	// Initialisation du niveau de la racine a 1
	rootNode->SetDepth(1);

	// Initialisation de l'identifiant de la racine a 1

	rootNode->SetNodeIdentifier("L0");

	// Initialisation de la profondeur de l'arbre
	nTreeDepth = 1;

	// Initialisation du nombre de noeuds de profondeur 1
	ivNodeNumberByDepth.Add(1);
	// La classe de cout est a specifier lors de l'instanciation d'un classifieur
	treeCost = NULL;
	dCost = -1;
	dConstructionCost = -1;
	dRootCost = -1;
	dEvaluation = -1;
	dOptimalCost = -1;
	dBestDecreasedCost = -1;
	dPreviousCost = -1;
	dTrainingAccuracy = -1;
	dOutOfBagTrainingAccuracy = -1;
	nObjectsNumber = 0;
	nDownStepNumber = 0;
	nOutOfBagObjectsNumber = 0;

	oaSelectedAttributes = NULL;
	nkdDatabaseObjects = NULL;
	svReferenceTargetModalities = new SymbolVector;

	rootNodeTrainBaseLoader = NULL;
	rootNodeOutOfBagBaseLoader = NULL;
	drawingType = DrawingType::NoReplacement;

	odCurrentInternalNodes = new ObjectDictionary;
	odOptimalInternalNodes = new ObjectDictionary;
	odOptimalLeaveNodes = new ObjectDictionary;
	odCurrentLeaveNodes = new ObjectDictionary;
	odCurrentLeaveNodes->SetAt(rootNode->GetNodeIdentifier(),
				   rootNode); // Ajout de la racine dans le dictionnaire des feuilles courantes
	odOptimalLeaveNodes->SetAt(rootNode->GetNodeIdentifier(),
				   rootNode); // Ajout de la racine dans le dictionnaire des feuilles optimal
	odTreeNodes.SetAt(rootNode->GetNodeIdentifier(),
			  rootNode); // Ajout de la racine dans le dictionnaire des feuilles
}

DTDecisionTree::~DTDecisionTree()
{
	POSITION position;
	Object* object;
	ALString sVariableName;

	odUsedVariables.DeleteAll();

	position = odDatabaseSplittersTrain.GetStartPosition();
	// Parcours du dictionnnaire des partitionnements
	while (position != NULL)
	{
		odDatabaseSplittersTrain.GetNextAssoc(position, sVariableName, object);
		cast(DTBaseLoaderSplitter*, object)->CleanDaughterBaseloader();
	}
	odDatabaseSplittersTrain.DeleteAll();

	position = odDatabaseSplittersOutOfBag.GetStartPosition();
	// Parcours du dictionnnaire des partitionnements
	while (position != NULL)
	{
		odDatabaseSplittersOutOfBag.GetNextAssoc(position, sVariableName, object);
		cast(DTBaseLoaderSplitter*, object)->CleanDaughterBaseloader();
	}
	odDatabaseSplittersOutOfBag.DeleteAll();

	if (treeCost != NULL)
		delete treeCost;

	if (oaSelectedAttributes != NULL)
		delete oaSelectedAttributes;

	if (nkdDatabaseObjects != NULL)
	{
		nkdDatabaseObjects->DeleteAll();
		delete nkdDatabaseObjects;
	}

	if (rootNodeTrainBaseLoader != NULL)
	{
		// NV9 rootNodeTrainDatabase->RemoveAll();
		delete rootNodeTrainBaseLoader;
	}

	if (rootNodeOutOfBagBaseLoader != NULL)
	{
		// NV9 rootNodeOutOfBagDatabase->RemoveAll();
		delete rootNodeOutOfBagBaseLoader;
	}

	if (svReferenceTargetModalities != NULL)
		delete svReferenceTargetModalities;

	// odCurrentInternalNodes->DeleteAll();
	delete odCurrentInternalNodes;
	// odCurrentLeaveNodes->DeleteAll();
	delete odCurrentLeaveNodes;

	delete odOptimalInternalNodes;
	delete odOptimalLeaveNodes;

	odTreeNodes.DeleteAll();
}

boolean DTDecisionTree::ComputeStats()
{
	require(GetCostClass() != NULL);
	dEvaluation = 0;
	boolean bCurrentSilentMode;

	// enregistrement du mode silence
	bCurrentSilentMode = Global::GetSilentMode();
	if (not dtPredictorParameter->GetVerboseMode())
	{
		Global::SetSilentMode(true); // eviter les warnings sur valeur unique de modalite cible
	}

	assert(Check());

	// construction de l'arbre complet (etape descendante)
	if (not Build())
	{
		Global::SetSilentMode(bCurrentSilentMode);
		return false;
	}

	if (dtPredictorParameter->GetPruningMode() == DTGlobalTag::POST_PRUNING_LABEL)
		Prune(); // elaguage des feuilles (etape montante)

	if (dtPredictorParameter->GetPruningMode() != DTGlobalTag::PRE_PRUNING_LABEL)
		ReplaceCurrentTreeWithOptimalTree();

	dCost = dOptimalCost;
	dEvaluation = 1 - dCost / dRootCost;

	if (odCurrentLeaveNodes->GetCount() == 0)
	{
		AddWarning("No remaining leaves for this tree");
	}

	assert(Check());

	bIsStatsComputed = true;

	// retour au mode silence itnitiale
	Global::SetSilentMode(bCurrentSilentMode);

	return true;
}

boolean DTDecisionTree::Build()
{
	require(GetCostClass() != NULL);

	boolean bContinue = true;
	boolean bpostmode = true;
	const boolean bIsRegression =
	    (GetOrigineBaseLoader()->GetTupleLoader()->GetInputExtraAttributeContinuousValues() != NULL ? true : false);

	// Debut de suivi des taches
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Tree building (down step)");

	if (dtPredictorParameter->GetVerboseMode())
	{
		AddSimpleMessage("verbose =" + ALString(IntToString(dtPredictorParameter->GetVerboseMode())));
		AddSimpleMessage("Nodes variables selection : " + dtPredictorParameter->GetNodeVariablesSelection());
		AddSimpleMessage("Attributes split selection : " + dtPredictorParameter->GetAttributesSplitSelection());
		AddSimpleMessage("Pruning mode : " + dtPredictorParameter->GetPruningMode());
		AddSimpleMessage("Tree criterion : " + dtPredictorParameter->GetTreeCost());
		AddSimpleMessage("Max Children : " +
				 ALString(IntToString(dtPredictorParameter->GetMaxChildrenNumber())));
		AddSimpleMessage("Max depth : " + ALString(IntToString(dtPredictorParameter->GetMaxDepth())));
		AddSimpleMessage("Max number of internal nodes : " +
				 ALString(IntToString(dtPredictorParameter->GetMaxInternalNodesNumber())));
		AddSimpleMessage("Max number of leaves (before the last split) : " +
				 ALString(IntToString(dtPredictorParameter->GetMaxLeavesNumber())));
		AddSimpleMessage("Min number of instances per leave : " +
				 ALString(IntToString(dtPredictorParameter->GetMinInstancesPerLeaveNumber())));
		AddSimpleMessage("");
		AddSimpleMessage("Down step :");
		AddSimpleMessage("-------------------------------------------------------------------------------------"
				 "----------------------------------------------");
		AddSimpleMessage(" Step\tLeaf split\tPrevious $\tOptimal $\tBest decr. $\tAccuracy\tOOB accur.\tNb "
				 "attributes used ");
	}

	dPreviousCost = dRootCost;
	dOptimalCost = dRootCost;
	nDownStepNumber = 0;

	// Calcul de l'arbre optimal
	while (bContinue)
	{
		TaskProgression::DisplayLabel("Tree Learning : " + ALString(IntToString(nDownStepNumber)) +
					      " Internal Nodes");
		nDownStepNumber++;

#ifdef TRACE_DEBUG
		cout << endl
		     << endl
		     << "============ ETAPE " << nDownStepNumber << "  (cout etape precedente : " << dPreviousCost
		     << ") ===============" << endl
		     << endl;
#endif
		StartTimer(DTTimerTree4);
		NumericKeyDictionary* possibleSplits =
		    SelectPossibleSplits(); // selection des coupures possibles et calcul de leurs couts associes
		StopTimer(DTTimerTree4);
		DTDecisionTreeNodeSplit* bestSplit = NULL;

		if (dtPredictorParameter->GetAttributesSplitSelection() == DTGlobalTag::RANDOM_UNIFORM_LABEL)
			bestSplit = ChooseSplitRandomUniform(possibleSplits); // choix aleatoire uniforme d'une coupure
		else if (dtPredictorParameter->GetAttributesSplitSelection() == DTGlobalTag::RANDOM_LEVEL_LABEL)
		{
			bestSplit = ChooseSplitRandomLevel(
			    possibleSplits, bpostmode); // choix aleatoire d'une coupure, en fonction du level
		}
		else
		{
			const double maxCost =
			    (dtPredictorParameter->GetPruningMode() == DTGlobalTag::PRE_PRUNING_LABEL ? dOptimalCost
												      : 0);
			bestSplit = ChooseSplitBestTreeCost(
			    possibleSplits, maxCost); // par defaut : choix de la coupure de meilleur cout
		}

		if (bestSplit == NULL)
		{
			if (bpostmode)
				bpostmode = false;
			else
				bContinue = false; // plus de coupures possibles
		}
		else
		{
			// creation d'un nouveau noeud interne, et memorisation eventuelle de l'arbre, s'il a ameliore
			// le cout (en post-pruning)

			dBestDecreasedCost = bestSplit->GetTreeCost();
			// NB. le dBestDecreasedCost est le meilleur (ou moins mauvais) cout de l'etape en cours, et
			// n'ameliore pas forcement le cout optimal (post-pruning ou no-pruning uniquement)

			// Determination du nombre de parties initiales avant partitionnement
			const int nPartNumber =
			    (bestSplit->GetAttributeStats()->GetAttributeType() == KWType::Continuous
				 ? bestSplit->GetSplittableNode()->GetObjectNumber()
				 : bestSplit->GetAttributeStats()->GetDescriptiveStats()->GetValueNumber());

			// creation d'un nouveau noeud interne et des feuilles associees, a partir de la coupure
			// selectionnee un ComputeStats sera effectue sur les noeuds feuilles de ce noeud interne
			StartTimer(DTTimerTree1);
			bContinue = SetUpInternalNode(bestSplit->GetTreeCost(),
						      bestSplit->GetSplittableNode()->GetNodeIdentifier(),
						      bestSplit->GetAttributeStats(), nPartNumber, bIsRegression);
			StopTimer(DTTimerTree1);

			if (bContinue == false)
			{
				possibleSplits->DeleteAll();
				delete possibleSplits;
				TaskProgression::DisplayProgression(100);
				TaskProgression::EndTask();
				return false;
			}

			if (bestSplit->GetTreeCost() < dOptimalCost)
			{
				dOptimalCost = bestSplit->GetTreeCost();

				// en post-pruning, memoriser l'arbre uniquement s'il minimise le cout optimal rencontre
				// precedemment
				if (dtPredictorParameter->GetPruningMode() == DTGlobalTag::POST_PRUNING_LABEL)
				{
					MemorizeCurrentTreeInOptimalTree();
					if (bVerbatim)
					{
						cout << "optimal cost pre pruning " << dOptimalCost << " : leave "
						     << odOptimalLeaveNodes->GetCount() << endl;
						cout << "NODE ID :"
						     << bestSplit->GetSplittableNode()->GetNodeIdentifier() << endl;
					}
				}
			}

			dTrainingAccuracy = ComputeTrainingAccuracy();
			if (drawingType == DrawingType::UseOutOfBag)
				dOutOfBagTrainingAccuracy = ComputeOutOfBagTrainingAccuracy();

			if (dtPredictorParameter->GetVerboseMode())
				AddSimpleMessage(GetDisplayString(nDownStepNumber) +
						 ALString(bestSplit->GetSplittableNode()->GetNodeIdentifier()) + "\t" +
						 (bestSplit->GetSplittableNode()->GetSplitAttributeStats() == NULL
						      ? "\t\t"
						      : GetDisplayString(bestSplit->GetSplittableNode()
									     ->GetSplitAttributeStats()
									     ->GetAttributeName())) +
						 GetDisplayString(dPreviousCost) + GetDisplayString(dOptimalCost) +
						 GetDisplayString(dBestDecreasedCost) +
						 GetDisplayString(dTrainingAccuracy) +
						 GetDisplayString(dOutOfBagTrainingAccuracy) +
						 GetDisplayString(odUsedVariables.GetCount()));

			dPreviousCost =
			    bestSplit
				->GetTreeCost(); // Le cout de cette etape devient le cout precedent de l'etape suivante

			bContinue = CanAddNodes(); // criteres d'arret sur la structure de l'arbre : nombre max de
						   // noeuds internes et nombre max de feuilles
		}

		possibleSplits->DeleteAll();
		delete possibleSplits;

		TaskProgression::DisplayProgression(
		    (int)(100 * tanh((double)nDownStepNumber / (log((double)GetClass()->GetUsedAttributeNumber()) *
								log((double)GetRootNode()->GetObjectNumber())))));
	}

	if (dtPredictorParameter->GetPruningMode() == DTGlobalTag::NO_PRUNING_LABEL)
	{
		// en no-pruning, on garde l'arbre maximal (qui n'est pas forcement le meilleur)
		MemorizeCurrentTreeInOptimalTree();
	}

	if (dtPredictorParameter->GetVerboseMode())
		AddSimpleMessage("");

	TaskProgression::DisplayProgression(100);
	TaskProgression::EndTask();
	return true;
}

void DTDecisionTree::Prune()
{
	// elaguage des feuilles, lorsque l'elaguage ameliore le cout de l'arbre

	// TaskProgression::BeginTask();

	// TaskProgression::DisplayMainLabel(" Pruning step");

	if (dtPredictorParameter->GetVerboseMode())
	{
		AddSimpleMessage("");
		AddSimpleMessage("Up step :");
		AddSimpleMessage("-------------------------------------------------------------------------------------"
				 "-------------------------------------------------------------------------");
		AddSimpleMessage(
		    " Step\tPruned leaf\tDecr. cost\tOptimal cost\tAccuracy\tOOB accur.\tNb attributes used ");
	}

	boolean bContinue = true;
	int nUpStepNumber = 0;

	while (bContinue)
	{
		TaskProgression::DisplayLabel(
		    "Tree Learning (Up Step): " + ALString(IntToString(nDownStepNumber - nUpStepNumber)) +
		    " Internal Nodes");

		nUpStepNumber++;

		require(GetInternalNodes() != NULL);

		// retrouver le noeud interne dont l'elaguage serait le plus benefique, ou le moins penalisant
		// (i.e : l'elagage de ce noeud n'ameliorerait pas forcement le cout optimal de l'arbre)
		DTDecisionTreeNode* bestPrunableInternalNode = FindBestPrunableInternalNode();

		bContinue = (bestPrunableInternalNode == NULL ? false : true);

		if (bContinue)
		{
			// elaguage reel du noeud interne selectionne, et transformation en feuille
			// NB. on ne memorisera l'arbre elague, que si cet elaguage ameliore le cout optimal
			CheckCurrentTree();
			PruneInternalNode(bestPrunableInternalNode);
			if (bVerbatim)
			{
				cout << "****** NEW STEP : " << nUpStepNumber << endl;
				cout << "curent cost post pruning " << dBestDecreasedCost << " : leave "
				     << odCurrentLeaveNodes->GetCount() << endl;
			}
			CheckCurrentTree();
			// Mise a jour du cout de l'arbre apres elaguage de ce noeud interne
			dCost = dOptimalCost;
			dEvaluation = 1 - dCost / dRootCost;
			dTrainingAccuracy = ComputeTrainingAccuracy();
			dOutOfBagTrainingAccuracy = ComputeOutOfBagTrainingAccuracy();

			// Memorisation de l'arbre s'il minimise le cout optimal rencontre jusque la
			if (dBestDecreasedCost <= dOptimalCost)
			{
				dOptimalCost = dBestDecreasedCost;
				MemorizeCurrentTreeInOptimalTree();
				// cout << "optimal cost post pruning " << dOptimalCost << " : leave " <<
				// odOptimalLeaveNodes->GetCount() << endl;
			}

			// Le cout de cette etape devient le cout precedent de l'etape suivante
			dPreviousCost = dBestDecreasedCost;

			dTrainingAccuracy = ComputeTrainingAccuracy();
			dOutOfBagTrainingAccuracy = ComputeOutOfBagTrainingAccuracy();

			if (bContinue and dtPredictorParameter->GetVerboseMode())
				AddSimpleMessage(
				    GetDisplayString(nUpStepNumber) + bestPrunableInternalNode->GetNodeIdentifier() +
				    "\t" + GetDisplayString(dBestDecreasedCost) + GetDisplayString(dOptimalCost) +
				    GetDisplayString(dTrainingAccuracy) + GetDisplayString(dOutOfBagTrainingAccuracy) +
				    GetDisplayString(odUsedVariables.GetCount()));

			// TaskProgression::DisplayProgression(99 * nUpStepNumber / nDownStepNumber);
		}
	}

	if (dtPredictorParameter->GetVerboseMode())
		AddSimpleMessage("");

	assert(Check());

	// TaskProgression::EndTask();
}

DTDecisionTreeNode* DTDecisionTree::FindBestPrunableInternalNode()
{
	ALString sNodeKey;
	Object* object;

	DTDecisionTreeNode* bestPrunableNode = NULL;

	POSITION position = odCurrentInternalNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sNodeKey, object);
		DTDecisionTreeNode* selectedNode = cast(DTDecisionTreeNode*, object);

		// Cas d'un noeud elaguable (tous ses fils sont des feuilles)
		if (selectedNode->IsPrunable())
		{
			// Calcul du cout de l'arbre, ce noeud etait elague
			double prunedCost =
			    GetCostClass()->ComputeHypotheticalPrunedTreeCost(this, dPreviousCost, sNodeKey);

			if (bestPrunableNode == NULL)
			{
				dBestDecreasedCost = prunedCost;
				bestPrunableNode = selectedNode;
			}
			if (prunedCost < dBestDecreasedCost)
			{
				dBestDecreasedCost = prunedCost;
				bestPrunableNode = selectedNode;
			}
		}
	}
	if (bVerbatim)
	{
		if (bestPrunableNode == NULL) // NVDELL
			cout << "node ID : NULL " << endl;
		else
			cout << "node ID : " << bestPrunableNode->GetNodeIdentifier() << endl;
	}
	return bestPrunableNode;
}

NumericKeyDictionary* DTDecisionTree::SelectPossibleSplits()
{
	ALString sLeafKey;
	Object* object;
	boolean bOk;
	NumericKeyDictionary* possibleSplits = new NumericKeyDictionary;

	assert(odCurrentLeaveNodes->GetCount() > 0);

	POSITION position = odCurrentLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		// Extraction du noeud feuille courant
		odCurrentLeaveNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* selectedNode = cast(DTDecisionTreeNode*, object);

		// Cas d'un noeud avec un nombre suffisant d'individus, et n'ayant pas atteint la profondeur max de
		// l'arbre
		if (selectedNode->CanBecomeInternal())
		{
			// Parcours des variables
			for (int nAttributeIndex = 0;
			     nAttributeIndex < selectedNode->GetNodeAttributeStats()->GetSize(); nAttributeIndex++)
			{
				KWAttributeStats* attributeStats = cast(
				    KWAttributeStats*, selectedNode->GetNodeAttributeStats()->GetAt(nAttributeIndex));

				bOk = false;
				for (int i = 0; i < oaSelectedAttributes->GetSize(); i++)
				{
					KWAttribute* att = cast(KWAttribute*, oaSelectedAttributes->GetAt(i));
					if (att->GetName() == attributeStats->GetAttributeName())
						bOk = true;
				}
				if (bOk == false)
					continue;

				if (attributeStats->GetPreparedDataGridStats() == NULL or
				    attributeStats->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
					continue; // a partir de learningEnv v8, les attributs a level nul ne sont plus
						  // prepares. Le seul attribut prepare correspond ici a l'attribut
						  // cible

				// On considere la variable seulement si la taille de la partition est > 1 (pas d'arbre
				// filaire)
				if (attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber() > 1)
				{
					DTDecisionTreeNodeSplit* nodeSplit = new DTDecisionTreeNodeSplit;
					nodeSplit->SetSplittableNode(selectedNode);
					nodeSplit->SetAttributeStats(attributeStats);
					// calcul du cout total de l'arbre augmente des feuilles issue du
					// partitionnement courant
					GetCostClass()->ComputeHypotheticalAugmentedTreeCost(this, dPreviousCost,
											     nodeSplit);
					possibleSplits->SetAt(attributeStats, nodeSplit);
				}
			}
		}
	}

	return possibleSplits;
}

DTDecisionTreeNodeSplit* DTDecisionTree::ChooseSplitBestTreeCost(NumericKeyDictionary* possibleSplits,
								 const double maxCost)
{
	// on cherche une coupure dont le cout est le plus petit possible parmi les couts des coupures de l'etape en
	// cours, ET qui soit inferieur au cout maxi passe en parametre NB. en "pre pruning", le cout maxi est celui de
	// l'etape precedente, sinon ce cout est egal a zero (c'est a dire pas de maxi) cela signifie qu'en post-pruning
	// ou no-pruning, la meilleure coupure choisie n'ameliorera pas forcement le cout optimal de l'arbre

	assert(dtPredictorParameter->GetAttributesSplitSelection() == DTGlobalTag::BEST_TREE_COST_LABEL);

	double bestCost = maxCost;
	DTDecisionTreeNodeSplit* bestSplit = NULL;
	bool bFirstSplit = true;

#ifdef TRACE_DEBUG
	cout << endl << endl << "splits possibles : " << endl;
#endif

	NUMERIC key;
	Object* object;
	POSITION position = possibleSplits->GetStartPosition();

	while (position != NULL)
	{
		possibleSplits->GetNextAssoc(position, key, object);

		DTDecisionTreeNodeSplit* nodeSplit = cast(DTDecisionTreeNodeSplit*, object);

#ifdef TRACE_DEBUG
		nodeSplit->Write(cout);
#endif

		if ((maxCost > 0 and nodeSplit->GetTreeCost() < bestCost) or
		    (maxCost == 0 and (bFirstSplit or nodeSplit->GetTreeCost() < bestCost)))
		{
			bestCost = nodeSplit->GetTreeCost();
			bestSplit = nodeSplit;
		}

		bFirstSplit = false;
	}

#ifdef TRACE_DEBUG
	if (bestSplit)
	{
		cout << endl << "split choisi en best tree cost : " << endl;
		bestSplit->Write(cout);
	}
#endif

	return bestSplit;
}

DTDecisionTreeNodeSplit* DTDecisionTree::ChooseSplitRandomUniform(NumericKeyDictionary* possibleSplits)
{
	// choix aleatoire d'une coupure, parmi une selection de coupures
	// (NB. selon si on est en pre-pruning ou non, les coupures selectionnees n'ameliorent pas forcement le cout
	// optimal observe jusqu'ici)

	assert(dtPredictorParameter->GetAttributesSplitSelection() == DTGlobalTag::RANDOM_UNIFORM_LABEL);

	ObjectArray* oaSplitsForStep;

	if (dtPredictorParameter->GetPruningMode() == DTGlobalTag::PRE_PRUNING_LABEL)
		oaSplitsForStep = GetImprovingCostSplits(possibleSplits, true);
	else
	{
		oaSplitsForStep = GetImprovingCostSplits(possibleSplits, false);
	}

	DTDecisionTreeNodeSplit* nodeSplit = NULL;

	if (oaSplitsForStep->GetSize() > 0)
	{
		// choisir aleatoirement parmi les coupures retenues
		int iRandomChoice = RandomInt(oaSplitsForStep->GetSize() - 1);
		nodeSplit = cast(DTDecisionTreeNodeSplit*, oaSplitsForStep->GetAt(iRandomChoice));
	}

	delete oaSplitsForStep;

#ifdef TRACE_DEBUG
	if (nodeSplit)
	{
		cout << endl << "split choisi en random uniform : " << endl;
		nodeSplit->Write(cout);
	}
#endif

	return nodeSplit;
}

DTDecisionTreeNodeSplit* DTDecisionTree::ChooseSplitRandomLevel(NumericKeyDictionary* possibleSplits, boolean bpostmode)
{
	// choix aleatoire d'une coupure, parmi une selection de coupures, en fonction du level
	// (NB. selon si on est en pre-pruning ou non, les coupures selectionnees n'ameliorent pas forcement le cout
	// optimal observe jusqu'ici)

	assert(dtPredictorParameter->GetAttributesSplitSelection() == DTGlobalTag::RANDOM_LEVEL_LABEL);

	ObjectArray* oaSplitsForStep;

	if (dtPredictorParameter->GetPruningMode() == DTGlobalTag::PRE_PRUNING_LABEL || bpostmode)
		oaSplitsForStep = GetImprovingCostSplits(possibleSplits, true);
	else
	{
		oaSplitsForStep = GetImprovingCostSplits(possibleSplits, false);
	}

	DTDecisionTreeNodeSplit* nodeSplitRandomLevel = NULL;

	if (oaSplitsForStep->GetSize() == 0)
	{
		delete oaSplitsForStep;
		return nodeSplitRandomLevel;
	}

	// trie des attributs avant selection pour guarantir l'ordre en parallele

	oaSplitsForStep->SetCompareFunction(DTSplitCompareSortValue);
	oaSplitsForStep->Sort();

	DoubleVector vLevels;
	ObjectArray oaListAttributes;

	// repertorier tous les levels et les index de chargement des attributs des coupures selectionnees
	for (int i = 0; i < oaSplitsForStep->GetSize(); i++)
	{
		DTDecisionTreeNodeSplit* nodeSplit = cast(DTDecisionTreeNodeSplit*, oaSplitsForStep->GetAt(i));
		KWAttributeStats* stats = nodeSplit->GetAttributeStats();
		KWAttribute* attribute = stats->GetClass()->LookupAttribute(stats->GetAttributeName());

		if (attribute->GetUsed() and attribute->GetLoaded() and stats->GetLevel() > 0)
		{
			vLevels.Add(stats->GetLevel());
			oaListAttributes.Add(attribute);
		}
	}

	if (vLevels.GetSize() == 0)
	{
		delete oaSplitsForStep;
		return nodeSplitRandomLevel;
	}

	ObjectArray* oalistatt = DTAttributeSelection::SortObjectArrayFromContinuous(
	    1, vLevels, oaListAttributes); // choisir aleatoirement 1 attribut

	if (oalistatt != NULL)
	{
		assert(oalistatt->GetSize() == 1);

		// chercher la coupure correspondant a l'attribut choisi
		for (int i = 0; i < oaSplitsForStep->GetSize(); i++)
		{
			DTDecisionTreeNodeSplit* nodeSplit = cast(DTDecisionTreeNodeSplit*, oaSplitsForStep->GetAt(i));
			KWAttributeStats* stats = nodeSplit->GetAttributeStats();
			KWAttribute* attribute = stats->GetClass()->LookupAttribute(stats->GetAttributeName());

			if (attribute == oalistatt->GetAt(0))
				nodeSplitRandomLevel = nodeSplit;
		}

		assert(nodeSplitRandomLevel != NULL);

		delete oalistatt;
	}

	delete oaSplitsForStep;

#ifdef TRACE_DEBUG
	if (nodeSplitRandomLevel)
	{
		cout << endl << "split choisi en random level : " << endl;
		nodeSplitRandomLevel->Write(cout);
	}
#endif

	return nodeSplitRandomLevel;
}

ObjectArray* DTDecisionTree::GetImprovingCostSplits(NumericKeyDictionary* possibleSplits, boolean btest)
{
	// selectionner les coupures d'une etape donnee  qui ameliorent le cout optimal obtenu jusqu'ici

	ObjectArray* oaBestSplits = new ObjectArray;

	NUMERIC key;
	Object* object;
	POSITION position = possibleSplits->GetStartPosition();
	double dEpsilon = 1e-6;

	while (position != NULL)
	{
		possibleSplits->GetNextAssoc(position, key, object);
		DTDecisionTreeNodeSplit* nodeSplit = cast(DTDecisionTreeNodeSplit*, object);

		if (btest)
		{
			if ((dOptimalCost - nodeSplit->GetTreeCost()) > dEpsilon)
				oaBestSplits->Add(nodeSplit);
		}
		else
			oaBestSplits->Add(nodeSplit);
	}

	oaBestSplits->SetCompareFunction(DTSplitCompareSortValue);
	oaBestSplits->Sort();

	return oaBestSplits;
}

boolean DTDecisionTree::CanAddNodes()
{
	// Critere d'arret sur la structure de l'arbre :
	// - nombre max de noeuds internes
	// - nombre max de feuilles

	// NB : le critere de profondeur max de l'arbre s'applique au niveau de chaque noeud : si un noeud a atteint la
	// profondeur max, ca n'empeche pas de creer encore des noeuds a partir des autres noeuds (qui ont une
	// profondeur moindre)

	if (odCurrentInternalNodes != NULL and
	    ((dtPredictorParameter->GetMaxInternalNodesNumber() > 0 &&
	      odCurrentInternalNodes->GetCount() >= dtPredictorParameter->GetMaxInternalNodesNumber()) or
	     (dtPredictorParameter->GetMaxLeavesNumber() > 0 &&
	      odCurrentLeaveNodes->GetCount() >= dtPredictorParameter->GetMaxLeavesNumber())))
		return false;
	else
		return true;
}

DTDecisionTreeNode* DTDecisionTree::GetRootNode() const
{
	// Cas d'un arbre avec noeud interne : la racine est le 1er d'entre eux
	if (odCurrentInternalNodes != NULL && odCurrentInternalNodes->GetCount() > 0)
	{
		return cast(DTDecisionTreeNode*, odCurrentInternalNodes->Lookup("L0"));
	}
	// Cas d'un arbre sans noeud interne
	else
	{
		// On verifie qu'il n'y a qu'un noeud feuille
		require(odCurrentLeaveNodes->GetCount() == 1);

		return cast(DTDecisionTreeNode*, odCurrentLeaveNodes->Lookup("L0"));
	}
}

boolean DTDecisionTree::SetUpInternalNode(const double dNewCost, const Symbol sFatherNodeKey,
					  KWAttributeStats* fatherNodeBestAttributeStats, const int nValueNumber,
					  const boolean bIsRegression)
{
	DTBaseLoaderSplitter* databaseSplitterTrain;
	DTBaseLoaderSplitter* databaseSplitterOutOfBag;
	DTDecisionTreeNode* fatherNode;
	DTDecisionTreeNode* sonNode;
	DTBaseLoader* blsonNode;
	// KWAttribute* attribute;
	KWTupleTable targetTupleTable;
	ALString sAttributeName;
	int nSonIndex;
	int nSonNumber;
	boolean bOk = true;
	require(fatherNodeBestAttributeStats != NULL);

	// Extraction du futur noeud pere (qui, pour le moment, est une feuille)
	fatherNode = cast(DTDecisionTreeNode*, odCurrentLeaveNodes->Lookup(sFatherNodeKey));

	if (not fatherNode->CanBecomeInternal())
		return bOk;

	// NV9
	// fatherNodeBestAttributeStats->GetTargetDescriptiveStats()->SetLearningSpec(fatherNode->GetNodeLearningSpec());
	require(fatherNodeBestAttributeStats->GetTargetDescriptiveStats()->GetLearningSpec() != NULL);

	fatherNode->SetSplitAttributeStats(fatherNodeBestAttributeStats);
	fatherNode->SetSplitVariableValueNumber(nValueNumber);
	StartTimer(DTTimerTree2);
	// Calcul du partitionnement de la KWDatabase selon cette assertion
	databaseSplitterTrain = new DTBaseLoaderSplitter;
	databaseSplitterTrain->SetOrigineBaseLoader(fatherNode->GetTrainBaseLoader());
	databaseSplitterTrain->CreateDaughterBaseloaderFromSplitAttribute(fatherNodeBestAttributeStats, learningSpec);

	databaseSplitterOutOfBag = NULL;

	if (drawingType == DrawingType::UseOutOfBag)
	{
		databaseSplitterOutOfBag = new DTBaseLoaderSplitter;
		databaseSplitterOutOfBag->SetOrigineBaseLoader(fatherNode->GetOutOfBagBaseLoader());
		databaseSplitterOutOfBag->CreateDaughterBaseloaderFromSplitAttribute(fatherNodeBestAttributeStats,
										     learningSpec);
	}
	StopTimer(DTTimerTree2);
	if (dtPredictorParameter->GetMinInstancesPerLeaveNumber() > 0)
	{
		// critere d'arret sur le nombre minimal d'objets par feuille.
		// S'il existe au moins un noeud devant etre cree, qui ne comporte pas suffisamment d'elements, alors on
		// ne transforme pas cette feuille en noeud interne

		for (int i = 0;
		     i < fatherNodeBestAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();
		     i++)
		{
			DTBaseLoader* db = databaseSplitterTrain->GetDaughterBaseloaderAt(i);

			if (db->GetDatabaseObjects()->GetSize() < dtPredictorParameter->GetMinInstancesPerLeaveNumber())
			{
				// restitution de l'etat initial
				delete databaseSplitterTrain;
				delete databaseSplitterOutOfBag;
				fatherNode->SetSplitAttributeStats(NULL);
				fatherNode->SetSplitVariableValueNumber(0);
				fatherNode->SetCanBecomeInternal(
				    false); // on laissera dorenavant toujours ce noeud en tant que noeud feuille, sans
					    // creer de nouveaux noeuds fils a partir de lui
				return bOk;
			}
		}
	}

	if (drawingType == DrawingType::UseOutOfBag)
	{
		// verification qu'il existe bien des objets dans la base de Train ET dans la base Out of bag. Si ce
		// n'est pas le cas, alors on ne transforme pas cette feuille en noeud interne

		for (int i = 0;
		     i < fatherNodeBestAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();
		     i++)
		{
			DTBaseLoader* dbTrain = databaseSplitterTrain->GetDaughterBaseloaderAt(i);
			DTBaseLoader* dbOutOfBag = databaseSplitterOutOfBag->GetDaughterBaseloaderAt(i);

			if (dbTrain->GetDatabaseObjects()->GetSize() == 0 or
			    dbOutOfBag->GetDatabaseObjects()->GetSize() == 0)
			{
				// restitution de l'etat initial
				delete databaseSplitterTrain;
				delete databaseSplitterOutOfBag;
				fatherNode->SetSplitAttributeStats(NULL);
				fatherNode->SetSplitVariableValueNumber(0);
				fatherNode->SetCanBecomeInternal(false);
				return bOk;
			}
		}
	}

	// Suppression du noeud pere du dictionnaire des feuilles de l'arbre courant (mais pas de l'arbre optimal, s'il
	// y figure)
	odCurrentLeaveNodes->RemoveKey(sFatherNodeKey);

	// Ajout du noeud pere dans le dictionnaire des noeuds internes
	odCurrentInternalNodes->SetAt(sFatherNodeKey, fatherNode);

	// Passage du statut de noeud feuille a noeud interne
	fatherNode->SetLeaf(false);

	// Passage du statut de non elaguable a elaguable pour ce noeud
	fatherNode->SetPruningStatus(true);

	// Cas d'un noeud autre que le noeud racine
	if (fatherNode->GetFatherNode() != NULL)
	{
		// Passage du statut d'elaguable a non elaguable pour le pere de ce noeud
		fatherNode->GetFatherNode()->SetPruningStatus(false);
	}

	// Creation d'un objet qui contiendra le nom de l'attribut
	// implique dans l'assertion

	// Enregistrement du nom de l'attribut qui a ete utilise pour effectuer le partage du noeud pere
	RegisterAttributeNameAndOccurenceNumber(
	    fatherNodeBestAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetAttributeName());

	// s'assurer que la clef n'existe pas deja, sinon fuite memoire
	DTBaseLoaderSplitter* ds =
	    cast(DTBaseLoaderSplitter*, odDatabaseSplittersTrain.Lookup(fatherNode->GetNodeIdentifier()));
	if (ds != NULL)
	{
		ds->CleanDaughterBaseloader();
		delete ds;
	}

	ds = cast(DTBaseLoaderSplitter*, odDatabaseSplittersOutOfBag.Lookup(fatherNode->GetNodeIdentifier()));
	if (ds != NULL)
	{
		ds->CleanDaughterBaseloader();
		delete ds;
	}

	odDatabaseSplittersTrain.SetAt(fatherNode->GetNodeIdentifier(), databaseSplitterTrain);
	if (drawingType == DrawingType::UseOutOfBag)
		odDatabaseSplittersOutOfBag.SetAt(fatherNode->GetNodeIdentifier(), databaseSplitterOutOfBag);

	// Extraction du nombre de noeuds fils et creation du tableau associe
	nSonNumber = fatherNodeBestAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

	// Creation et initialisation des noeuds fils
	for (nSonIndex = 0; nSonIndex < nSonNumber; nSonIndex++)
	{
		// Creation du noeud (enregistrement automatique dans l'arbre)
		sonNode = new DTDecisionTreeNode;

		// Enregistrement du noeud en tant que fils de son pere
		fatherNode->AddSonNode(sonNode);

		sonNode->SetFatherNode(fatherNode);
		sonNode->SetDepth(fatherNode->GetDepth() + 1);
		ALString idprefix("L");
		sonNode->SetNodeIdentifier((Symbol)(idprefix + IntToString(nLastNodeIdentifier++)));
		odTreeNodes.SetAt(sonNode->GetNodeIdentifier(), sonNode);
		sonNode->SetLeaf(true);

		// Enregistrement du noeud dans le dictionnaire des noeuds feuille et mise a jour du niveau de l'arbre
		AddLeaveNodeAndUpdateTreeDepth(sonNode);

		if (dtPredictorParameter->GetMaxDepth() > 0 &&
		    sonNode->GetDepth() >= dtPredictorParameter->GetMaxDepth())
			sonNode->SetCanBecomeInternal(
			    false); // ce noeud a atteint la profondeur max autorisee pour un arbre, il ne pourra donc
				    // jamais devenir un noeud interne

		// Attribution du learningSpec identique au learningSpec de travail de son pere
		// NV9 sonNode->SetLearningSpecCopy(fatherNode->GetNodeLearningSpec());
		// NV9			sonNode->SetLearningSpec(fatherNode->GetNodeLearningSpec());
		// NV9 sonNode->GetNodeClassStats()->SetLearningSpec(sonNode->GetNodeLearningSpec());
		// NV9
		// sonNode->GetNodeLearningSpec()->GetTargetDescriptiveStats()->SetLearningSpec(sonNode->GetNodeLearningSpec());

		// Affectation de la base partitionnee adequate
		// NV9
		// sonNode->GetNodeLearningSpec()->SetDatabase(databaseSplitter->GetTrainDaughterDatabaseAt(nSonIndex));

		// memorisation de la base Train
		blsonNode = databaseSplitterTrain->GetDaughterBaseloaderAt(nSonIndex);
		sonNode->SetTrainBaseLoader(blsonNode);

		// memorisation de la base OOB
		if (drawingType == DrawingType::UseOutOfBag)
		{
			sonNode->SetOutOfBagBaseLoader(databaseSplitterOutOfBag->GetDaughterBaseloaderAt(nSonIndex));
		}

		// Memorisation du vecteur des valeurs cibles de reference
		sonNode->SetReferenceTargetModalities(fatherNode->GetReferenceTargetModalities());

		// Choix des attributs potentiels pour le partitionnement de ce noeud

		if (dtPredictorParameter->GetNodeVariablesSelection() == DTGlobalTag::RANDOM_LEVEL_LABEL or
		    dtPredictorParameter->GetNodeVariablesSelection() == DTGlobalTag::RANDOM_UNIFORM_LABEL)
		{
			//  Selectionner aleatoirement les variables qui seront utilisees par ce nouveau noeud feuille,
			//  soit de facon uniforme, soit en fonction du level
			// ObjectArray	* oalistAttributes = dtPredictorParameter->GetNodeVariablesSelection() ==
			// DTGlobalTag::RANDOM_LEVEL_LABEL ?
			//	rootNodeDatabaseLoader.GetAttributesLoadIndexesFromLevels(nUsableAttributesNumber) :
			//	rootNodeDatabaseLoader.GetAttributesShuffledLoadIndexes(nUsableAttributesNumber, true);

			sonNode->SetSelectedAttributes(oaSelectedAttributes);

			// cout << endl << endl;
			// sonNode->Write(cout);

			// if (oalistAttributes != NULL)
			//	delete oalistAttributes;
		}
		else
			// sinon, on reprend les attributs selectionnes au depart, et valables pour l'arbre entier, quel
			// que soit le noeud
			sonNode->SetSelectedAttributes(oaSelectedAttributes);

		// initilisation de
		sonNode->SetLearningSpec(learningSpec);
		sonNode->SetNodeLearningSpec(learningSpec);
		// sonNode->GetTrainBaseLoader()->GetTupleLoader()->SetInputClass(learningSpec);
		sonNode->GetTrainBaseLoader()->GetTupleLoader()->SetInputDatabaseObjects(
		    sonNode->GetTrainBaseLoader()->GetDatabaseObjects());
		// attribute = kwclass->LookupAttribute(attributeStats->GetAttributeName());
		// attribute->SetLoaded(false);
		// attribute->SetLoaded(true);
		sonNode->GetNodeLearningSpec()->GetClass()->Compile();
		// sonNode->GetTrainBaseLoader()->GetTupleLoader()->LoadUnivariate(learningSpec->GetTargetAttributeName(),
		// &targetTupleTable);
		sonNode->GetNodeLearningSpec()->SetCheckTargetAttribute(false);
		sonNode->GetNodeLearningSpec()->ComputeTargetStats(
		    sonNode->GetTrainBaseLoader()->GetTupleLoader()->GetInputExtraAttributeTupleTable());
		// attribute->SetLoaded(false);
		sonNode->GetNodeLearningSpec()->GetClass()->Compile();
		targetTupleTable.CleanAll();

		// blsonNode->GetDatabaseObjects()
		// blsonNode->GetTupleLoader()
		// sonNode->GetNodeLearningSpec(learningSpec)->ComputeTargetStats(const KWTupleTable* tupleTable)
		// sonNode->GetNodeLearningSpec()->SetDatabase(databaseSplitter->GetTrainDaughterDatabaseAt(nSonIndex));
		//  Calcul des statistiques univariees MODL
		StartTimer(DTTimerTree3);
		if (TaskProgression::IsInterruptionRequested() or not sonNode->ComputeAttributesStat())
		{
			bOk = false;
			return bOk;
		}
		StopTimer(DTTimerTree3);
		// Initialisation du nombre d'individus
		sonNode->SetObjectNumber(sonNode->GetTrainBaseLoader()->GetDatabaseObjects()->GetSize());
		if (drawingType == DrawingType::UseOutOfBag)
			sonNode->SetOutOfBagObjectNumber(
			    sonNode->GetOutOfBagBaseLoader()->GetDatabaseObjects()->GetSize());

		// Initialisation de l'effectif de la classe majoritaire
		int nMajoritarySize = 0;

		NumericKeyDictionary* targetModalitiesCountTrain =
		    ComputeTargetModalitiesCount(sonNode->GetTrainBaseLoader());
		sonNode->SetTargetModalitiesCountTrain(targetModalitiesCountTrain);

		if (drawingType == DrawingType::UseOutOfBag)
		{
			NumericKeyDictionary* targetModalitiesCountOutOfBag =
			    ComputeTargetModalitiesCount(sonNode->GetOutOfBagBaseLoader());
			sonNode->SetTargetModalitiesCountOutOfBag(targetModalitiesCountOutOfBag);
		}
		POSITION position = targetModalitiesCountTrain->GetStartPosition();
		NUMERIC key;
		Object* obj;

		while (position != NULL)
		{
			targetModalitiesCountTrain->GetNextAssoc(position, key, obj);

			DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);

			if (tmc->iCount > nMajoritarySize)
				nMajoritarySize = tmc->iCount;
		}

		// Initialisation de la purete du noeud
		sonNode->SetPurity((Continuous)nMajoritarySize / sonNode->GetObjectNumber());

		// calcul du level de la regle de la feuille
		// GetCostClass()->ComputeNodeCost(sonNode, this);

		// En cas de regression ayant ete transformee "artificiellement" en classification : mise a jour du
		// dictionnaire des instances de base, associant chaque KWObject a l'id de noeud
		if (bIsRegression)
			UpdateDatabaseObjectsNodeIds(sonNode);
	}

	// Mise a jour du cout de l'arbre ainsi augmente
	dCost = dNewCost;
	dEvaluation = 1 - dCost / dRootCost;

	dTrainingAccuracy = ComputeTrainingAccuracy();
	if (drawingType == DrawingType::UseOutOfBag)
		dOutOfBagTrainingAccuracy = ComputeOutOfBagTrainingAccuracy();

	assert(CheckInternalNodes() and CheckLeavesNodes());

	return true;
}

NumericKeyDictionary* DTDecisionTree::ComputeTargetModalitiesCount(DTBaseLoader* bldata)
{
	assert(bldata != NULL);
	assert(bldata->GetTupleLoader() != NULL);
	assert(bldata->GetTupleLoader()->GetInputExtraAttributeTupleTable() != NULL);

	// assert(targetAttributeLoadIndex.IsValid());//MB
	KWTupleTable* targettupletable =
	    cast(KWTupleTable*, bldata->GetTupleLoader()->GetInputExtraAttributeTupleTable());
	KWTuple* tuple;

	NumericKeyDictionary* targetModalitiesCount = new NumericKeyDictionary;

	for (int i = 0; i < targettupletable->GetSize(); i++)
	{
		tuple = cast(KWTuple*, targettupletable->GetAt(i));
		// svReferenceTargetModalities->Add(tuple->GetSymbolAt(0));

		const Symbol sInstanceModality = tuple->GetSymbolAt(0);

		// for (int j = 0; j < svReferenceTargetModalities->GetSize(); j++){
		//	const Symbol sModalityValue = svReferenceTargetModalities->GetAt(j);

		DTDecisionTree::TargetModalityCount* modalityCount = new DTDecisionTree::TargetModalityCount;
		modalityCount->sModality = sInstanceModality;
		modalityCount->iCount = tuple->GetFrequency();

		targetModalitiesCount->SetAt(sInstanceModality.GetNumericKey(), modalityCount);
	}
	return targetModalitiesCount;
}

void DTDecisionTree::PruneInternalNode(DTDecisionTreeNode* node)
{
	ALString sTmp;
	ALString sAttributeName;
	int nOccurrenceNumber;
	boolean bToPrune;

	require(odCurrentInternalNodes != NULL);

	assert(Check());

	assert(node != NULL);
	assert(node->GetSplitAttributeStats() != NULL);
	assert(node->IsLeaf() == false);

	// Passage du statut de noeud interne a celui de noeud terminal
	node->SetLeaf(true);

	// Passage de statut elagable a non elagable
	node->SetPruningStatus(false);

	// Cas d'un noeud autre que le noeud racine
	if (node->GetFatherNode() != NULL)
	{
		// Etude du statut d'elagabilite du noeud pere
		bToPrune = true;
		// Parcours des fils du pere
		for (int nSonIndex = 0; nSonIndex < node->GetFatherNode()->GetSons()->GetSize(); nSonIndex++)
		{
			// Cas d'un des fils qui n'est pas une feuille
			if (not cast(DTDecisionTreeNode*, node->GetFatherNode()->GetSons()->GetAt(nSonIndex))->IsLeaf())
				bToPrune = false;
		}
		// Affectation du statut (eventuellement nouveau)
		node->GetFatherNode()->SetPruningStatus(bToPrune);
	}

	// On decremente le nombre d'occurence de la variable de partitionnement dans l'arbre
	// Extraction du nom de la variable de partitionnement de ce noeud
	sAttributeName = node->GetSplitAttributeStats()->GetAttributeName();

	// Extraction du nombre d'occurence de cette variable dans l'arbre
	IntObject* ioOccurences = cast(IntObject*, GetUsedVariablesDictionary()->Lookup(sAttributeName));
	nOccurrenceNumber = ioOccurences->GetInt();

	assert(nOccurrenceNumber > 0);

	// Cas d'une occurence unique
	if (nOccurrenceNumber == 1)
	{
		delete ioOccurences;
		GetUsedVariablesDictionary()->RemoveKey(sAttributeName);
	}
	else
	{
		ioOccurences->SetInt(nOccurrenceNumber - 1);
	}

	// Elagage des noeuds fils
	for (int nSonIndex = 0; nSonIndex < node->GetSons()->GetSize(); nSonIndex++)
	{
		DTDecisionTreeNode* sonNode = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(nSonIndex));

		// Tout fils a elaguer doit etre une feuille
		if (not sonNode->IsLeaf())
		{
			AddError(sTmp + "PruneInternalNode : son node " + ALString(sonNode->GetNodeIdentifier()) +
				 " should be a leaf. ");
			// continue;
		}

		// Mise a jour du nombre de noeuds de profondeur egale a celle du noeud
		ivNodeNumberByDepth.UpgradeAt(sonNode->GetDepth() - 1, -1);

		// Cas ou plus aucun noeud n'est de la profondeur de l'arbre
		if (ivNodeNumberByDepth.GetAt(sonNode->GetDepth() - 1) == 0)
		{
			// Decrementation de la profondeur de l'arbre
			nTreeDepth--;
			ivNodeNumberByDepth.SetSize(nTreeDepth);
		}

		// Cas ou la feuille n'est pas dans l'arbre optimal
		if (odOptimalInternalNodes->Lookup(sonNode->GetNodeIdentifier()) == NULL &&
		    odOptimalLeaveNodes->Lookup(sonNode->GetNodeIdentifier()) == NULL)
		{
			// Dereferencement complet dans tous les dictionnaires, et destruction memoire de la feuille
			DeleteNode(sonNode);
		}
		else
		{
			// Suppression de la reference a la feuille, dans le dictionnaire des feuilles de l'arbre
			// courant (mais la reference a la feuille peut continuer a figurer parmi les feuilles de
			// l'arbre optimal)
			odCurrentLeaveNodes->RemoveKey(sonNode->GetNodeIdentifier());
		}
	}

	// suppression de la reference du noeud interne, dans le dictionnaire des noeuds internes de l'arbre courant
	// (mais il peut continuer a figurer parmi les noeuds internes de l'arbre optimal)
	odCurrentInternalNodes->RemoveKey(node->GetNodeIdentifier());

	// Ajout du noeud dans le dictionnaire des feuilles
	odCurrentLeaveNodes->SetAt(node->GetNodeIdentifier(), node);
	node->SetLeaf(true);
}

ObjectDictionary* DTDecisionTree::GetLeaves() const
{
	return odCurrentLeaveNodes;
}

ObjectDictionary* DTDecisionTree::GetInternalNodes() const
{
	return odCurrentInternalNodes;
}

ObjectDictionary* DTDecisionTree::GetUsedVariablesDictionary() const
{
	return (ObjectDictionary*)&odUsedVariables;
}

double DTDecisionTree::GetEvaluation() const
{
	require(dEvaluation != -1);

	return dEvaluation;
}

int DTDecisionTree::GetTreeDepth() const
{
	return nTreeDepth;
}

IntVector* DTDecisionTree::GetNodeNumberByDepth() const
{
	return (IntVector*)&ivNodeNumberByDepth;
}

double DTDecisionTree::GetTreeCost() const
{
	return dCost;
}
double DTDecisionTree::GetConstructionCost() const
{
	return dConstructionCost;
}
double DTDecisionTree::GetRootCost() const
{
	return dRootCost;
}

DTDecisionTreeParameter* DTDecisionTree::GetParameters() const
{
	return dtPredictorParameter;
}

void DTDecisionTree::SetParameters(DTDecisionTreeParameter* dtParam)
{
	dtPredictorParameter = dtParam;
}

SymbolVector* DTDecisionTree::GetReferenceTargetModalities() const
{
	return svReferenceTargetModalities;
}

DTDecisionTreeCost* DTDecisionTree::GetCostClass() const
{
	return treeCost;
}

void DTDecisionTree::SetCostClass(DTDecisionTreeCost* cost)
{
	treeCost = cost;
}
void DTDecisionTree::SetConstructionCost(double d)
{
	dConstructionCost = d;
}

ObjectArray* DTDecisionTree::GetSelectedAttributes() const
{
	require(CheckSelectedAttributes());
	return oaSelectedAttributes;
}

void DTDecisionTree::SetSelectedAttributes(ObjectArray* iv)
{
	// si le parametre est a NULL, alors on utilise tous les attributs initialement retenus (i.e, ceux qui sont
	// marques Loaded dans le dico) sinon, on n'utilise que les attributs dont les index de chargement figurent dans
	// le parametre recu

	if (oaSelectedAttributes != NULL)
	{
		// suppression des anciennes valeurs
		delete oaSelectedAttributes;
		oaSelectedAttributes = NULL;
	}

	if (iv == NULL)
		return;

	else
	{
		oaSelectedAttributes = iv;
	}
	require(CheckSelectedAttributes());
}

void DTDecisionTree::InitializeCostValue(double dValue)
{
	dRootCost = dValue;
}

double DTDecisionTree::GetSonsNumberMean() const
{
	POSITION position;
	Object* object;
	ALString sKey;
	double dMean;

	assert(odCurrentInternalNodes != NULL);

	dMean = 0.0;

	// Parcours des feuilles
	position = odCurrentInternalNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		dMean += node->GetSons()->GetSize();
	}

	dMean /= double(odCurrentInternalNodes->GetCount());

	return dMean;
}

double DTDecisionTree::ComputeOutOfBagTrainingAccuracy()
{
	POSITION position;
	Object* object;
	ALString sKey;
	double dAccuracyFromPurity;
	int objectsNumber;

	// Initialisation
	objectsNumber = 0;
	dAccuracyFromPurity = 0;

	// Parcours des noeuds feuilles
	position = odCurrentLeaveNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sKey, object);

		// Extraction du noeud interne courant
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		int nNodeSize = node->GetOutOfBagObjectNumber();

		// Mise a jour du nombre total d'individus
		objectsNumber += nNodeSize;

		// Calcul du nombre de bien classes (effectif de la classe cible majoritaire)
		// a partir de la purete
		dAccuracyFromPurity += node->GetPurity() * nNodeSize;
	}

	if (objectsNumber == 0)
		return 0;

	// Normalisation
	// require(GetRootNode()->GetOutOfBagObjectNumber() == objectsNumber);
	dAccuracyFromPurity /= (double)objectsNumber;

	return dAccuracyFromPurity;
}

double DTDecisionTree::ComputeTrainingAccuracy()
{
	POSITION position;
	Object* object;
	ALString sKey;
	DTDecisionTreeNode* node;
	double dAccuracyFromPurity;
	int objectsNumber;
	int nNodeSize;

	// Initialisation
	objectsNumber = 0;
	dAccuracyFromPurity = 0;

	// Parcours des noeuds feuilles
	position = odCurrentLeaveNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sKey, object);

		// Extraction du noeud interne courant
		node = cast(DTDecisionTreeNode*, object);

		nNodeSize = node->GetObjectNumber();

		// Mise a jour du nombre total d'individus
		objectsNumber += nNodeSize;

		// Calcul du nombre de bien classes (effectif de la classe cible majoritaire)
		// a partir de la purete
		dAccuracyFromPurity += node->GetPurity() * nNodeSize;
	}
	// Normalisation

	if (objectsNumber == 0)
		dAccuracyFromPurity = 0;
	else
		dAccuracyFromPurity /= (double)objectsNumber;

	return dAccuracyFromPurity;
}

double DTDecisionTree::GetTrainingAccuracy() const
{
	return dTrainingAccuracy;
}

double DTDecisionTree::GetOutOfBagTrainingAccuracy() const
{
	return dOutOfBagTrainingAccuracy;
}

double DTDecisionTree::ComputeTreeEvaluation()
{
	// BEGIN_CH
	// Check de l'arbre pour que tout soit calculable
	// END_CH
	return (GetCostClass()->ComputeTotalTreeCost(this));
}

void DTDecisionTree::RegisterAttributeNameAndOccurenceNumber(const ALString& sAttributeName)
{
	Object* obj = odUsedVariables.Lookup(sAttributeName);

	// Cas ou le nom de la variable est absent
	if (obj == NULL)
	{
		// Creation d'une nouvelle entree dans le dictionnaire
		IntObject* io = new IntObject;
		io->SetInt(1);
		odUsedVariables.SetAt(sAttributeName, io);
	}
	else
	{
		// incrementer le nombre d'occurrences
		IntObject* io = cast(IntObject*, obj);
		io->SetInt(io->GetInt() + 1);
	}
}

void DTDecisionTree::AddLeaveNodeAndUpdateTreeDepth(DTDecisionTreeNode* newLeaveNode)
{
	ALString sTmp;
	require(newLeaveNode != NULL);
	require(newLeaveNode->GetDepth() != -1);
	require(newLeaveNode->GetNodeIdentifier() != -1);
	require(newLeaveNode->IsLeaf());

	if (odCurrentLeaveNodes->Lookup(newLeaveNode->GetNodeIdentifier()) != NULL)
		AddError(sTmp + "Failed to add a new leaf : identifier '" + newLeaveNode->GetNodeIdentifier() +
			 "' already exist as a leaf");

	if (odCurrentInternalNodes->Lookup(newLeaveNode->GetNodeIdentifier()) != NULL)
		AddError(sTmp + "Failed to add a new leaf : identifier '" + newLeaveNode->GetNodeIdentifier() +
			 "' already exist as an internal node");

	odCurrentLeaveNodes->SetAt(newLeaveNode->GetNodeIdentifier(), newLeaveNode);

	// Cas d'un noeud qui n'augmente pas la profondeur de l'arbre
	if (newLeaveNode->GetDepth() <= nTreeDepth)
	{
		// Incrementation du nombre de noeuds de profondeur egale a celle  de ce noeud
		ivNodeNumberByDepth.UpgradeAt(newLeaveNode->GetDepth() - 1, 1);
	}

	// Sinon
	else
	{
		nTreeDepth = newLeaveNode->GetDepth();
		ivNodeNumberByDepth.Add(1);
	}
}

void DTDecisionTree::InitializeRootNode(const NumericKeyDictionary* randomForestDatabaseObjects)
{
	Continuous cPurityValue;
	int nMajoritarySize;
	POSITION position;
	NUMERIC key;
	Object* obj;

	assert(svReferenceTargetModalities != NULL);
	assert(svReferenceTargetModalities->GetSize() == 0);

	// effectuer un tirage avec ou sans remise
	BuildDatabaseObjectsDictionary(randomForestDatabaseObjects);

	assert(rootNodeTrainBaseLoader == NULL);
	assert(rootNodeOutOfBagBaseLoader == NULL);

	// NV V9
	// NV V9 GetRootNode()->SetLearningSpecCopy(GetLearningSpec());
	GetRootNode()->SetNodeSelectedAttributes(GetSelectedAttributes());

	if (drawingType == DrawingType::UseOutOfBag)
	{
		rootNodeTrainBaseLoader = new DTBaseLoader;
		rootNodeOutOfBagBaseLoader = new DTBaseLoader;
		origineBaseLoader->BuildTrainOutOfBagBaseLoader(rootNodeTrainBaseLoader, rootNodeOutOfBagBaseLoader);
	}
	else
	{
		rootNodeTrainBaseLoader = origineBaseLoader;
		rootNodeOutOfBagBaseLoader = NULL;
	}
	// a partir de ce tirage, initialisation des 2 bases du noeud racine (base train et, le cas echeant, base out of
	// bag)

	// NV V9
	// rootNodeTrainDatabase->SetDatabaseName(GetRootNode()->GetNodeLearningSpec()->GetDatabase()->GetDatabaseName());
	// NV V9
	// rootNodeTrainDatabase->SetClassName(GetRootNode()->GetNodeLearningSpec()->GetDatabase()->GetClassName());
	GetRootNode()->SetTrainBaseLoader(rootNodeTrainBaseLoader);

	// NV V9
	// rootNodeOutOfBagDatabase->SetDatabaseName(GetRootNode()->GetNodeLearningSpec()->GetDatabase()->GetDatabaseName()
	// + "_OOB"); NV V9
	// rootNodeOutOfBagDatabase->SetClassName(GetRootNode()->GetNodeLearningSpec()->GetDatabase()->GetClassName());
	GetRootNode()->SetOutOfBagBaseLoader(rootNodeOutOfBagBaseLoader);

	// origineBaseLoader->Write(cout);

	GetRootNode()->SetObjectNumber(rootNodeTrainBaseLoader->GetDatabaseObjects()->GetSize());

	if (drawingType == DrawingType::UseOutOfBag)
	{
		GetRootNode()->SetOutOfBagObjectNumber(rootNodeOutOfBagBaseLoader->GetDatabaseObjects()->GetSize());
	}
	else
	{
		GetRootNode()->SetOutOfBagObjectNumber(0);
	}
	// Initialisation du learningSpec de la racine
	// NV V9 GetRootNode()->SetLearningSpecCopy(GetLearningSpec());
	// NV V9 GetRootNode()->GetNodeClassStats()->SetLearningSpec(GetRootNode()->GetNodeLearningSpec());

	// NV V9 GetRootNode()->GetNodeLearningSpec()->SetDatabase(GetRootNode()->GetTrainDatabase());

	// Calcul des statistiques univariees MODL pour la racine
	// NV V9 if (!GetRootNode()->GetNodeClassStats()->IsStatsComputed())
	{
		// NV V9 	GetRootNode()->GetNodeClassStats()->SetSelectedAttributes(oaSelectedAttributes);
		// NV V9 	GetRootNode()->GetNodeClassStats()->ComputeStats();
		// GetRootNode()->SetLearningSpecCopy(GetRootNode()->GetClassStats()->GetLearningSpec());
	}

	// NV V9
	// GetRootNode()->GetNodeLearningSpec()->GetTargetDescriptiveStats()->SetLearningSpec(GetRootNode()->GetLearningSpec());

	// Memorisation de l'attribut de reference des valeurs cibles
	// KWDGSAttributeSymbolValues * kwdgTargetAttribute = NULL; //NV V9  cast(KWDGSAttributeSymbolValues*,
	// GetRootNode()->GetNodeClassStats()->GetTargetValueStats()->GetAttributeAt(0));
	KWTupleTable* targettupletable =
	    cast(KWTupleTable*, origineBaseLoader->GetTupleLoader()->GetInputExtraAttributeTupleTable());
	KWTuple* tuple;

	for (int i = 0; i < targettupletable->GetSize(); i++)
	{
		tuple = cast(KWTuple*, targettupletable->GetAt(i));
		svReferenceTargetModalities->Add(tuple->GetSymbolAt(0));
	}
	GetRootNode()->SetReferenceTargetModalities(svReferenceTargetModalities);

	// mise a jour des modalites, en fonction de celles qui ont ete repertoriees lors du computeStats sur  la
	// database de train
	UpdateDatabaseObjectsTargetModalities();

	// assert(GetRootNode()->GetClass()->LookupAttribute(GetRootNode()->GetLearningSpec()->GetTargetAttributeName())
	// != NULL);

	NumericKeyDictionary* targetModalitiesCountTrain =
	    ComputeTargetModalitiesCount(GetRootNode()->GetTrainBaseLoader());
	GetRootNode()->SetTargetModalitiesCountTrain(targetModalitiesCountTrain);
	if (drawingType == DrawingType::UseOutOfBag)
	{
		NumericKeyDictionary* targetModalitiesCountOutOfBag =
		    ComputeTargetModalitiesCount(GetRootNode()->GetOutOfBagBaseLoader());
		GetRootNode()->SetTargetModalitiesCountOutOfBag(targetModalitiesCountOutOfBag);
	}

	// Extraction de la repartition de la cible
	// NV V9 dataGridStats = GetRootNode()->GetNodeClassStats()->GetTargetValueStats();

	// Initialisation de l'effectif de la classe majoritaire
	nMajoritarySize = 0;

	// Parcours des modalites cible
	position = targetModalitiesCountTrain->GetStartPosition();
	while (position != NULL)
	{
		targetModalitiesCountTrain->GetNextAssoc(position, key, obj);
		DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);

		// Mise a jour de l'effectif max
		if (tmc->iCount > nMajoritarySize)
			nMajoritarySize = tmc->iCount;
	}

	// Initialisation de la purete du noeud racine
	cPurityValue = (Continuous)nMajoritarySize / (Continuous)GetRootNode()->GetObjectNumber();
	GetRootNode()->SetPurity(cPurityValue);
}

DTDecisionTree* DTDecisionTree::Clone()
{
	DTDecisionTree* copyTree;

	copyTree = new DTDecisionTree;
	copyTree->CopyFrom(this);

	return copyTree;
}

void DTDecisionTree::CopyFrom(const DTDecisionTree* sourceTree)
{
	require(sourceTree != NULL);

	dCost = sourceTree->dCost;
	dEvaluation = sourceTree->dEvaluation;
	dRootCost = sourceTree->dRootCost;
	dConstructionCost = sourceTree->dConstructionCost;
	dtPredictorParameter = sourceTree->dtPredictorParameter;
	dTrainingAccuracy = sourceTree->dTrainingAccuracy;
	dOutOfBagTrainingAccuracy = sourceTree->dOutOfBagTrainingAccuracy;
	ivNodeNumberByDepth.CopyFrom(&sourceTree->ivNodeNumberByDepth);
	nTreeDepth = sourceTree->nTreeDepth;
	nOutOfBagObjectsNumber = sourceTree->nOutOfBagObjectsNumber;
	nObjectsNumber = sourceTree->nObjectsNumber;
	nUsableAttributesNumber = sourceTree->nUsableAttributesNumber;
}

void DTDecisionTree::WriteReportDetailedInternalNode(ostream& ost, const DTDecisionTreeNode* ndRoot) const
{
	ost << "\n\nDetailed internal nodes statistics\n";

	int icurent = 0;
	int icurentdeth = 0;
	const DTDecisionTreeNode* ndCurrent;
	ObjectList olPoplist;
	ObjectArray oaPopList;

	ndCurrent = ndRoot;

	// Parcour en largeur de l'arbre
	while (ndCurrent != NULL)
	{
		// tests
		// if (ndCurrent->IsLeaf()){
		//	ost << "------------------------------------------------------------------\n" ;
		//		ost << "Leave node\t" << ndCurrent->GetNodeIdentifier() << endl ;
		//		ost  << "Depth\t" << ndCurrent->GetDepth () << endl;
		//		ndCurrent->WriteReport(ost);
		//		ost << "\n";

		//	ndCurrent->WriteReport(ost);
		//}

		// Test pour garentir qu'il n'y a pas d'erreur dans les neouds
		if (!ndCurrent->IsLeaf() && ndCurrent->GetDepth() <= icurentdeth + 1)
		{
			icurentdeth = ndCurrent->GetDepth();
			ost << "------------------------------------------------------------------\n";
			ost << "Internal node\t" << ndCurrent->GetNodeIdentifier() << endl;
			ost << "Depth\t" << ndCurrent->GetDepth() << endl;
			ndCurrent->WriteReport(ost);
			ost << "\n";

			// Ajout des fils du neoud en cour dans la liste des noeud a traiter ( voire algo parcour
			// largeur , avec list fifo )
			if (!ndCurrent->IsLeaf())
				for (int i = 0; i < ndCurrent->GetSons()->GetSize(); i++)
				{
					DTDecisionTreeNode* node =
					    cast(DTDecisionTreeNode*, ndCurrent->GetSons()->GetAt(i));
					if (node->GetDepth() == icurentdeth + 1)
						oaPopList.Add(ndCurrent->GetSons()->GetAt(i));
				}
		}

		// Prochain neoud a traiter
		if (icurent < oaPopList.GetSize())
		{
			ndCurrent = cast(DTDecisionTreeNode*, oaPopList.GetAt(icurent));
			icurent++;
		}
		else
			ndCurrent = NULL;
	}
}

void DTDecisionTree::WriteReportLeaveStatistics(ostream& ost, const DTDecisionTreeNode* ndRoot) const
{
	ost << "\n\nLeave statistics\n";

	int icurent = 0;
	int icurentdeth = 0;
	const DTDecisionTreeNode* ndCurrent;
	ObjectList olPoplist;
	ObjectArray oaPopList;

	ndCurrent = ndRoot;

	// Ligne d'entete
	ndCurrent->WriteHeaderLineReport(ost, this, true);

	// Parcour en largeur de l'arbre
	while (ndCurrent != NULL)
	{
		// Test pour garentir qu'il n'y a pas d'erreur dans les noeuds
		if (ndCurrent->GetDepth() <= icurentdeth + 1)
		{
			icurentdeth = ndCurrent->GetDepth();

			// Ajout des fils du neoud en cour dans la liste des noeud a traiter ( voire algo parcour
			// largeur , avec list fifo )
			if (ndCurrent->IsLeaf())
				// Ligne de de stats
				ndCurrent->WriteLineReport(ost, this);
			else
			{
				for (int i = 0; i < ndCurrent->GetSons()->GetSize(); i++)
				{
					DTDecisionTreeNode* node =
					    cast(DTDecisionTreeNode*, ndCurrent->GetSons()->GetAt(i));

					if (node->GetDepth() == icurentdeth + 1)
					{
						oaPopList.Add(ndCurrent->GetSons()->GetAt(i));
					}
				}
			}
		}

		// Prochain noeud a traiter
		if (icurent < oaPopList.GetSize())
		{
			ndCurrent = cast(DTDecisionTreeNode*, oaPopList.GetAt(icurent));
		}
		else
			ndCurrent = NULL;

		icurent++;
	}
}

void DTDecisionTree::WriteReportInternalNodesStatistics(ostream& ost, const DTDecisionTreeNode* ndRoot) const
{
	ost << "\n\nInternal Nodes statistics\n";

	int icurent = 0;
	int icurentdeth = 0;
	const DTDecisionTreeNode* ndCurrent;
	ObjectList olPoplist;
	ObjectArray oaPopList;

	ndCurrent = ndRoot;

	// Ligne d'entete
	ndCurrent->WriteHeaderLineReport(ost, this, false);

	// Parcour en largeur de l'arbre
	while (ndCurrent != NULL)
	{
		// Test pour garentir qu'il n'y a pas d'erreur dans les neouds
		if (ndCurrent->GetDepth() <= icurentdeth + 1)
		{
			icurentdeth = ndCurrent->GetDepth();

			// Ajout des fils du neoud en cour dans la liste des noeud a traiter ( voire algo parcour
			// largeur , avec list fifo )

			if (!ndCurrent->IsLeaf())
			{
				for (int i = 0; i < ndCurrent->GetSons()->GetSize(); i++)
				{
					DTDecisionTreeNode* node =
					    cast(DTDecisionTreeNode*, ndCurrent->GetSons()->GetAt(i));

					if (node->GetDepth() == icurentdeth + 1)
					{
						oaPopList.Add(ndCurrent->GetSons()->GetAt(i));
					}
				}
				// Ligne de de stats
				ndCurrent->WriteLineReport(ost, this);
			}
		}

		// Prochain neoud a traiter
		if (icurent < oaPopList.GetSize())
		{
			ndCurrent = cast(DTDecisionTreeNode*, oaPopList.GetAt(icurent));
		}
		else
			ndCurrent = NULL;

		icurent++;
	}
}

boolean DTDecisionTree::Check() const
{
	boolean bOk = true;
	// CheckInternalNodes();

	// if (not CheckLeavesNodes())
	//	bOk = false;

	// if (not CheckOptimalInternalNodes())
	//	bOk = false;

	// if (not CheckOptimalLeavesNodes())
	//	bOk = false;

	if (not CheckCurrentTree())
		bOk = false;

	if (not CheckOptimalTree())
		bOk = false;

	return (bOk);
}

boolean DTDecisionTree::CheckInternalNodes() const
{
	ALString sTmp;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odCurrentInternalNodes != NULL);
	require(odOptimalInternalNodes != NULL);

	boolean bOk = true;

	Object* object;
	ALString sLeafKey;

	POSITION position = odCurrentInternalNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (odCurrentLeaveNodes->Lookup(node->GetNodeIdentifier()) != NULL)
		{
			bOk = false;
			AddError(sTmp + "Internal node " + node->GetNodeIdentifier() +
				 " should not be also referenced as a leaf node");
		}

		if (node->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "Internal node " + node->GetNodeIdentifier() +
				 " has leave node characteristics");
		}

		for (int i = 0; i < node->GetSons()->GetSize(); i++)
		{
			DTDecisionTreeNode* son = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(i));

			if (odCurrentInternalNodes->Lookup(son->GetNodeIdentifier()) == NULL and
			    odCurrentLeaveNodes->Lookup(son->GetNodeIdentifier()) == NULL)
			{
				bOk = false;
				AddError(sTmp + "Internal node " + node->GetNodeIdentifier() + " : son node " +
					 son->GetNodeIdentifier() + " (" + (son->IsLeaf() ? "leaf" : "internal node") +
					 ") is not referenced in the tree nodes dictionaries");
			}
		}
	}

	if (not bOk)
	{
		AddSimpleMessage("");
		AddError(sTmp + "Current tree internal nodes are not valid");
		AddSimpleMessage("");
	}

	return bOk;
}

boolean DTDecisionTree::CheckOptimalInternalNodes() const
{
	ALString sTmp;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odOptimalInternalNodes != NULL);
	boolean bOk = true;

	if (odOptimalInternalNodes->GetCount() > odTreeNodes.GetCount())
	{
		bOk = false;
		AddError(sTmp + "The tree has more optimal internal nodes, than nodes");
	}

	Object* object;
	ALString sLeafKey;

	POSITION position = odOptimalInternalNodes->GetStartPosition();

	while (position != NULL)
	{
		odOptimalInternalNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (odOptimalLeaveNodes->Lookup(node->GetNodeIdentifier()) != NULL)
		{
			bOk = false;
			AddError(sTmp + "Optimal internal node " + node->GetNodeIdentifier() +
				 " should not be also referenced as an optimal leaf node");
		}

		// if (node->IsLeaf())
		//{
		//	bOk = false;
		//	AddError(sTmp + "Optimal internal node " + node->GetNodeIdentifier() + " has leave node
		// characteristics");
		// }

		for (int i = 0; i < node->GetSons()->GetSize(); i++)
		{
			DTDecisionTreeNode* son = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(i));

			if (odOptimalInternalNodes->Lookup(son->GetNodeIdentifier()) == NULL and
			    odOptimalLeaveNodes->Lookup(son->GetNodeIdentifier()) == NULL)
			{
				bOk = false;
				AddError(sTmp + "Optimal internal node " + node->GetNodeIdentifier() + " : son node " +
					 son->GetNodeIdentifier() + " (" + (son->IsLeaf() ? "leaf" : "internal node") +
					 ") is not referenced in the optimal tree nodes dictionaries");
			}
		}
	}
	if (not bOk)
	{
		AddSimpleMessage("");
		AddError(sTmp + "Optimal tree internal nodes are not valid");
		AddSimpleMessage("");
	}

	return bOk;
}

boolean DTDecisionTree::CheckLeavesNodes() const
{
	ALString sTmp;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odCurrentInternalNodes != NULL);
	require(odOptimalInternalNodes != NULL);

	boolean bOk = true;

	Object* object;
	ALString sLeafKey;

	if (odCurrentLeaveNodes->GetCount() == 0)
	{
		bOk = false;
		AddError(sTmp + "The tree has no leave nodes");
	}

	POSITION position = odCurrentLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (odCurrentInternalNodes->Lookup(node->GetNodeIdentifier()) != NULL)
		{
			bOk = false;
			AddError(sTmp + "Leave node " + node->GetNodeIdentifier() +
				 " should not be also referenced as an internal node");
		}

		if (not node->CheckNode())
		{
			bOk = false;
			AddError(sTmp + "Leave node " + node->GetNodeIdentifier() + " don't check");
		}

		if (not node->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "Leave node " + node->GetNodeIdentifier() +
				 " has internal node characteristics");
		}
	}
	if (not bOk)
	{
		AddSimpleMessage("");
		AddError(sTmp + "Current tree leaves nodes are not valid");
		AddSimpleMessage("");
	}

	return bOk;
}

boolean DTDecisionTree::CheckSelectedAttributes() const
{
	if (oaSelectedAttributes == NULL)
	{
		return true;
	}
	else
	{
		for (int i = 0; i < oaSelectedAttributes->GetSize(); i++)
		{
			KWAttribute* att = cast(KWAttribute*, oaSelectedAttributes->GetAt(i));
			if (att == NULL)
				return false;
			if (att->Check() == false)
				return false;
		}
	}
	return true;
}

boolean DTDecisionTree::CheckOptimalLeavesNodes() const
{
	ALString sTmp;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odCurrentInternalNodes != NULL);
	require(odOptimalInternalNodes != NULL);

	boolean bOk = true;

	if (odOptimalLeaveNodes->GetCount() > odTreeNodes.GetCount())
	{
		bOk = false;
		AddError(sTmp + "The tree has more optimal leaves nodes, than leaves nodes");
	}

	Object* object;
	ALString sLeafKey;

	POSITION position = odOptimalLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		odOptimalLeaveNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (odOptimalInternalNodes->Lookup(node->GetNodeIdentifier()) != NULL)
		{
			bOk = false;
			AddError(sTmp + "Optimal leave node " + node->GetNodeIdentifier() +
				 " should not be also referenced as an optimal internal node");
		}
	}

	if (not bOk)
	{
		AddSimpleMessage("");
		AddError(sTmp + "Optimal tree leaves nodes are not valid");
		AddSimpleMessage("");
	}

	return bOk;
}

boolean DTDecisionTree::CheckCurrentTree() const
{
	ALString sTmp;
	DTDecisionTreeNode* fatherNode;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odCurrentInternalNodes != NULL);
	require(odOptimalInternalNodes != NULL);

	boolean bOk = true;

	Object* object;
	ALString sLeafKey;

	if (odCurrentLeaveNodes->GetCount() == 0)
	{
		bOk = false;
		AddError(sTmp + "The tree has no leave nodes");
	}

	POSITION position = odCurrentLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
		fatherNode = node->GetFatherNode();

		if (fatherNode != NULL && odCurrentInternalNodes->Lookup(fatherNode->GetNodeIdentifier()) == NULL)
		{
			bOk = false;
			AddError(sTmp + "father Node of Leave node " + node->GetNodeIdentifier() +
				 " should not be an internal node");
		}

		if (not node->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "Leave node " + node->GetNodeIdentifier() + " is not leaf");
		}

		if (fatherNode != NULL && fatherNode->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "father of Leave node " + fatherNode->GetNodeIdentifier() +
				 " has internal node characteristics");
		}
	}

	position = odCurrentInternalNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
		fatherNode = node->GetFatherNode();

		if (fatherNode != NULL && odCurrentInternalNodes->Lookup(fatherNode->GetNodeIdentifier()) == NULL)
		{
			bOk = false;
			AddError(sTmp + "father Node of internal node " + node->GetNodeIdentifier() +
				 " should not be an internal node");
		}

		if (node->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "Internal node " + node->GetNodeIdentifier() + " is a leaf");
		}

		if (fatherNode != NULL && fatherNode->IsLeaf())
		{
			bOk = false;
			AddError(sTmp + "father of Internal node " + fatherNode->GetNodeIdentifier() +
				 " has leaf node characteristics");
		}
	}

	return bOk;
}

boolean DTDecisionTree::CheckOptimalTree() const
{
	ALString sTmp;
	DTDecisionTreeNode* fatherNode;
	require(odCurrentLeaveNodes != NULL);
	require(odOptimalLeaveNodes != NULL);
	require(odCurrentInternalNodes != NULL);
	require(odOptimalInternalNodes != NULL);

	boolean bOk = true;

	Object* object;
	ALString sLeafKey;

	// if (odOptimalLeaveNodes->GetCount() == 0)
	//{
	//	bOk = false;
	//	AddError(sTmp + "The tree has no leave nodes");
	// }

	POSITION position = odOptimalLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		odOptimalLeaveNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
		fatherNode = node->GetFatherNode();

		if (fatherNode != NULL && odOptimalInternalNodes->Lookup(fatherNode->GetNodeIdentifier()) == NULL)
		{
			bOk = false;
			AddError(sTmp + "father Node of Leave node " + node->GetNodeIdentifier() +
				 " should not be an internal node");
		}
	}

	position = odOptimalInternalNodes->GetStartPosition();

	while (position != NULL)
	{
		odOptimalInternalNodes->GetNextAssoc(position, sLeafKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
		fatherNode = node->GetFatherNode();

		if (fatherNode != NULL && odOptimalInternalNodes->Lookup(fatherNode->GetNodeIdentifier()) == NULL)
		{
			bOk = false;
			AddError(sTmp + "father Node of internal node " + node->GetNodeIdentifier() +
				 " should not be an internal node");
		}
	}

	return bOk;
}

void DTDecisionTree::Write(ostream& ost)
{
	// affichage des noeuds de l'arbre, en les triant par type (interne/feuille) et par profondeur a l'interieur de
	// chaque type

	ost << endl << endl;
	ost << "Number of leaves "
	    << "\t" << odCurrentLeaveNodes->GetCount() << "\n";
	ost << "Number of internal nodes "
	    << "\t" << odCurrentInternalNodes->GetCount() << "\n";
	ost << "Tree Depth "
	    << "\t" << nTreeDepth << "\n";

	ost << endl << "Internal nodes :" << endl;
	WriteNodes(ost, odCurrentInternalNodes);

	ost << endl << "Leaves nodes :" << endl;
	WriteNodes(ost, odCurrentLeaveNodes);
}
void DTDecisionTree::WriteNodes(ostream& ost, const ObjectDictionary* nodes)
{
	int i;
	int j;

	// export des noeuds dans un tableau, afin de les trier par profondeur

	ObjectArray* sortedNodes = new ObjectArray;
	nodes->ExportObjectArray(sortedNodes);

	sortedNodes->SetCompareFunction(DTDecisionTreeNodesDepthCompare);
	sortedNodes->Sort();

	for (i = 0; i < sortedNodes->GetSize(); i++)
	{
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, sortedNodes->GetAt(i));
		ost << *node << endl;

		if (node->GetSons()->GetSize() > 0)
		{
			for (j = 0; j < node->GetSons()->GetSize(); j++)
			{
				DTDecisionTreeNode* son = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(j));
				if (odCurrentInternalNodes->Lookup(son->GetNodeIdentifier()) == NULL and
				    odCurrentLeaveNodes->Lookup(son->GetNodeIdentifier()) == NULL)
					ost << endl
					    << endl
					    << "warning : son node " << son->GetNodeIdentifier() << " ("
					    << (son->IsLeaf() ? "leaf" : "internal node")
					    << ") is not referenced in nodes dictionaries" << endl
					    << endl;
			}
		}
	}
	delete sortedNodes;
}

void DTDecisionTree::WriteReport(ostream& ost) const
{
	ObjectArray oaReport;
	KWDGSAttributeSymbolValues* valueStats;
	int i;

	// require(IsStatsComputed());
	// require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());

	// Titre
	ost << "Descriptive statistics"
	    << "\n";
	ost << "\n\n";

	// Description du probleme d'apprentissage
	ost << "Problem description"
	    << "\n";
	ost << "\n";
	ost << "Database"
	    << "\t" << TSV::Export(GetLearningSpec()->GetClass()->GetName()) << "\n";

	// Nombres d'attributs
	ost << "Variables"
	    << "\n";

	for (int nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsData(nType))
		{
			int nAttributeNumber = GetLearningSpec()->GetClass()->GetUsedAttributeNumberForType(nType);
			if (nAttributeNumber > 0)
				ost << "\t" << KWType::ToString(nType) << "\t" << nAttributeNumber << "\n";
		}
	}

	ost << "\t"
	    << "Total"
	    << "\t" << GetLearningSpec()->GetClass()->GetUsedAttributeNumber() << "\n";
	ost << "\n";

	// Base d'objets
	// ost << "\nInstances" << "\t" << GetLearningSpec()->GetDatabase()->GetObjects()->GetSize() << "\n";
	ost << "\nInstances"
	    << "\t" << GetRootNode()->GetObjectNumber() << "\n";

	// Parametrage eventuel de l'apprentissage supervise
	if (GetTargetAttributeName() != "")
	{
		// Attribut cible
		ost << "\nTarget variable"
		    << "\t" << TSV::Export(GetTargetAttributeName()) << "\n";

		// Valeur cible principale
		if (GetMainTargetModality() != Symbol())
		{
			ost << "Main target value"
			    << "\t" << TSV::Export(GetMainTargetModality().GetValue()) << "\n";
		}

		// Statistiques descriptives
		if (GetTargetAttributeType() == KWType::Continuous or GetTargetAttributeType() == KWType::Symbol)
			GetTargetDescriptiveStats()->WriteReport(ost);

		// Detail des valeurs
		if (GetTargetAttributeType() == KWType::Symbol)
		{
			valueStats = cast(KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
			// Valeurs
			ost << "\nTarget values"
			    << "\n";
			ost << "\tValue\tFrequency\tCoverage\n";
			// for (i = 0; i < GetTargetAssertion()->GetModalityNumber(); i++)
			for (i = 0; i < valueStats->GetPartNumber(); i++)
			{
				// Affichage
				ost << "\t" << TSV::Export(valueStats->GetValueAt(i).GetValue());
				ost << "\t" << GetTargetValueStats()->GetUnivariateCellFrequencyAt(i);
				ost << "\t"
				    << GetTargetValueStats()->GetUnivariateCellFrequencyAt(i) * 1.0 /
					   GetInstanceNumber();
				ost << "\n";
			}
		}
		ost << "\n";
	}

	// Type de tache d'apprentissage effectue
	// ost << "\nLearning task";

	ost << "Number of variables used in the tree"
	    << "\t" << odUsedVariables.GetCount() << "\n";

	// Caracteristiques generales de l'arbre
	ost << "Level of the optimal tree "
	    << "\t" << GetEvaluation() << "\n";
	ost << "Number of leaves "
	    << "\t" << odCurrentLeaveNodes->GetCount() << "\n";
	ost << "Number of internal nodes "
	    << "\t" << odCurrentInternalNodes->GetCount() << "\n";
	ost << "Depth of the tree "
	    << "\t" << nTreeDepth << "\n";
	ost << "Training accuracy of the optimal tree "
	    << "\t" << GetTrainingAccuracy() << "\n"
	    << endl;

	if (drawingType == DrawingType::UseOutOfBag)
		ost << "Out-of-bag training accuracy of the optimal tree "
		    << "\t" << GetOutOfBagTrainingAccuracy() << "\n"
		    << endl;

	ost << "Variable name\tFrequency" << endl;
	POSITION position;
	Object* obj;
	ALString sKey;
	ALString sLine;

	position = GetUsedVariablesDictionary()->GetStartPosition();
	// Parcours des variables utilisees dans l'arbre
	while (position != NULL)
	{
		GetUsedVariablesDictionary()->GetNextAssoc(position, sKey, obj);
		IntObject* ioOccurences = cast(IntObject*, obj);
		sLine = sLine + sKey + "\t" + KWContinuous::ContinuousToString(ioOccurences->GetInt()) + "\n";
	}

	ost << sLine;
	// fin test

	// Contraintes utilisees lors de la recherche de l'arbre optimal
	dtPredictorParameter->Write(ost);

	// JS : Fonction de remplacement de l'affichage

	ost << "==============================================\n";

	WriteReportLeaveStatistics(ost, GetRootNode());
	WriteReportInternalNodesStatistics(ost, GetRootNode());
	WriteReportDetailedInternalNode(ost, GetRootNode());
	// WriteDatabaseObjects(ost);
}

void DTDecisionTree::WriteDatabaseObjects(ostream& ost, int maxObjects) const
{
	// affichage du contenu du dico des instances

	ost << endl << endl;

	ObjectArray oaInstances;
	nkdDatabaseObjects->ExportObjectArray(&oaInstances);

	oaInstances.SetCompareFunction(DTCompareInstancesOnIds);
	oaInstances.Sort();

	bool firstTime = true;

	for (int i = 0; i < oaInstances.GetSize(); i++)
	{
		DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, oaInstances.GetAt(i));

		if (firstTime)
		{
			dbo->WriteHeaderLine(ost);
			firstTime = false;
		}

		dbo->Write(ost);

		if (maxObjects > 0 and i >= maxObjects)
			break;
	}

	ost << endl << endl;
}

void DTDecisionTree::WriteDTArrayLineReport(ostream& ost, const ALString& sTitle, const ObjectArray* oaLearningReports,
					    DTDecisionTree* tree) const
{
	ObjectArray oaSortedReports;
	int i;
	DTDecisionTreeNode* learningReport;

	require(oaLearningReports != NULL);

	// Affichage si tableau non vide
	if (oaLearningReports->GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.CopyFrom(oaLearningReports);
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Parcours des rapports
		if (sTitle != "")
			ost << "\n\n" << sTitle << "\n\n";
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			learningReport = cast(DTDecisionTreeNode*, oaSortedReports.GetAt(i));

			// Ligne d'entete
			if (i == 0)
				learningReport->WriteHeaderLineReport(ost, this, true);

			// Ligne de de stats
			if (learningReport->IsLineReported())
				learningReport->WriteLineReport(ost, this);
		}
	}
}

void DTDecisionTree::UpdateDatabaseObjectsTargetModalities()
{
	assert(nkdDatabaseObjects != NULL);
	assert(nkdDatabaseObjects->GetCount() > 0);
	assert(svReferenceTargetModalities->GetSize() > 0);

	const ALString sTargetName = GetLearningSpec()->GetTargetAttributeName();

	// POSITION position = nkdDatabaseObjects->GetStartPosition();
	// NUMERIC key;
	Object* obj;
	ObjectArray* oaTreeDatabaseObjects = cast(ObjectArray*, origineBaseLoader->GetDatabaseObjects());
	SymbolVector* svtarget =
	    cast(SymbolVector*, origineBaseLoader->GetTupleLoader()->GetInputExtraAttributeSymbolValues());
	// oaTreeDatabaseObjects represente l'ensemble des KWObject de reference de cet arbre, parmi lequel un eventuel
	// tirage avec remise sera effectue, et qui servira a creer le dico des instances de cet arbre. Cet ensemble de
	// reference peut concerner, soit l'ensemble des objets de la base, soit une selection de cette base a partir
	// d'une valeur d'echantillon (cas du calcul d'uplift) NB. il n'y a pas de duplications d'instances, dans cet
	// ensemble de reference

	for (int i = 0; i < oaTreeDatabaseObjects->GetSize(); i++)
	{
		KWObject* kwo = cast(KWObject*, oaTreeDatabaseObjects->GetAt(i));

		Symbol sTargetValue = svtarget->GetAt(i);
		obj = nkdDatabaseObjects->Lookup(kwo);
		DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, obj);

		for (int j = 0; j < svReferenceTargetModalities->GetSize(); j++)
		{
			if (sTargetValue == svReferenceTargetModalities->GetAt(j))
			{
				dbo->SetTargetModalityIndex(j);
				break;
			}
		}
	}
}

void DTDecisionTree::UpdateDatabaseObjectsNodeIds(DTDecisionTreeNode* node)
{
	assert(nkdDatabaseObjects != NULL);
	assert(nkdDatabaseObjects->GetCount() > 0);

	Object* obj;
	ObjectArray* oaDatabaseObjects = cast(ObjectArray*, node->GetTrainBaseLoader()->GetDatabaseObjects());

	for (int i = 0; i < oaDatabaseObjects->GetSize(); i++)
	{
		KWObject* kwo = cast(KWObject*, oaDatabaseObjects->GetAt(i));

		obj = nkdDatabaseObjects->Lookup(kwo);
		assert(obj != NULL);
		DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, obj);
		dbo->SetNodeIdentifier(node->GetNodeIdentifier());
	}
}

void DTDecisionTree::BuildDatabaseObjectsDictionary(const NumericKeyDictionary* randomForestDatabaseObjects)
{
	assert(origineBaseLoader != NULL);
	assert(origineBaseLoader->GetDatabaseObjects() != NULL);
	assert(origineBaseLoader->GetDatabaseObjects()->GetSize() > 0);
	assert(nkdDatabaseObjects == NULL);

	const ObjectArray* oaTreeDatabaseObjects = origineBaseLoader->GetDatabaseObjects();
	// oaTreeDatabaseObjects represente l'ensemble des KWObject de reference de cet arbre, parmi lequel un eventuel
	// tirage avec remise sera effectue, et qui servira a creer le dico des instances de cet arbre. Cet ensemble de
	// reference peut concerner, soit l'ensemble des objets de la base, soit une selection de cette base a partir
	// d'une valeur d'echantillon (cas du calcul d'uplift) NB. il n'y a pas de duplications d'instances, dans cet
	// ensemble de reference

	nkdDatabaseObjects = new NumericKeyDictionary;

	// pour chaque KWObject de l'ensemble de reference, creer l'entree correspondante dans le dictionnaire des
	// instances de cet arbre. Chaque entree contiendra des infos sur l'instance concernee, notamment le nombre de
	// fois qu'elle a ete tiree dans le cadre d'un tirage avec remise....). On associe chaque instance avec un id
	// unique.

	for (int i = 0; i < oaTreeDatabaseObjects->GetSize(); i++)
	{
		KWObject* kwo = cast(KWObject*, oaTreeDatabaseObjects->GetAt(i));

		int instanceId = -1;

		if (randomForestDatabaseObjects != NULL)
			// s'il s'agit d'un arbre appartenant a un random forest, il faut recuperer l'id de cette
			// instance a partir de l'ensemble de reference du RF recu en parametre.
			// --> garantir qu'une meme instance, presente dans les arbres d'un RF, aura toujours le meme id
			// quel que soit l'arbre considere
			instanceId = GetDatabaseInstanceId(randomForestDatabaseObjects, kwo);

		DTDecisionTreeDatabaseObject* dbo;

		if (instanceId != -1)
			dbo = new DTDecisionTreeDatabaseObject(instanceId);
		else
			dbo = new DTDecisionTreeDatabaseObject(i);

		if (drawingType == DrawingType::NoReplacement)
			dbo->IncrementFrequency(); // si aucun tirage avec remise, la frequence de chaque instance
						   // vaudra 1

		nkdDatabaseObjects->SetAt(kwo, dbo);
	}

	if (drawingType == DrawingType::UseOutOfBag)
	{
		// effectuer un tirage aleatoire avec remise (ne peut etre demande que dans le cadre d'un RF)

		const int instancesNumber = oaTreeDatabaseObjects->GetSize();

		assert(randomForestDatabaseObjects != NULL);
		assert(randomForestDatabaseObjects->GetCount() == instancesNumber);

		for (int i = 0; i < oaTreeDatabaseObjects->GetSize(); i++)
		{
			const int newInstanceId = RandomInt(instancesNumber - 1);
			KWObject* kwo = cast(KWObject*, oaTreeDatabaseObjects->GetAt(newInstanceId));

			Object* obj = nkdDatabaseObjects->Lookup(kwo);
			assert(obj != NULL);

			DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, obj);
			dbo->IncrementFrequency();
		}
	}

	if (drawingType == DrawingType::WithReplacementAdaBoost)
	{
		// effectuer un tirage boost (pondere) avec remise. Ne peut etre demande que dans le cadre d'un RF

		assert(randomForestDatabaseObjects != NULL);
		assert(randomForestDatabaseObjects->GetCount() == oaTreeDatabaseObjects->GetSize());

		ObjectArray* kwoObjectsList = new ObjectArray;

		// calcul du vecteur des poids cumules. Le parametre kwoObjectsList va etre rempli avec les KWObject,
		// dans l'ordre correspondant au cumul calcule
		ContinuousVector* cumulatedWeights =
		    ComputeAdaBoostCumulatedWeights(randomForestDatabaseObjects, kwoObjectsList);

		// effectuer le tirage aleatoire avec remise, en prenant en compte les poids
		IntVector* indexes = SelectRandomWeightedIndexes(cumulatedWeights, oaTreeDatabaseObjects->GetSize());

		for (int i = 0; i < indexes->GetSize(); i++)
		{
			KWObject* kwo = cast(KWObject*, kwoObjectsList->GetAt(indexes->GetAt(i)));

			Object* objTree = nkdDatabaseObjects->Lookup(kwo);
			assert(objTree != NULL);
			DTDecisionTreeDatabaseObject* dboTree = cast(DTDecisionTreeDatabaseObject*, objTree);

			Object* objRF = randomForestDatabaseObjects->Lookup(kwo);
			assert(objRF != NULL);
			DTDecisionTreeDatabaseObject* dboRF = cast(DTDecisionTreeDatabaseObject*, objRF);

			assert(dboTree->GetId() == dboRF->GetId());

			dboTree->IncrementFrequency();

			// les 2 initialisations ci dessous ne servent qu'a titre informatif, lorsqu'on affiche des
			// traces. Elles ne servent pas a l'algo, puisque ce dernier utilise les objets du random forest
			// (et non celui des arbres) lors de l'utilisation de ces variables
			dboTree->SetBoostingTreeWeight(dboRF->GetBoostingTreeWeight());
			dboTree->SetTargetCorrectlyPredicted(dboRF->IsTargetCorrectlyPredicted());
		}

		delete cumulatedWeights;
		delete kwoObjectsList;
		delete indexes;

		// cout << endl << endl << "instances de l'arbre booste, apres tirage avec remise : " << endl;
		// WriteDatabaseObjects(cout);
	}

	// initialiser les nombres d'objets in et out of bag (le cas echeant)

	POSITION position = nkdDatabaseObjects->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		nkdDatabaseObjects->GetNextAssoc(position, key, obj);
		DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, obj);

		if (dbo->GetFrequency() > 0)
			nObjectsNumber += dbo->GetFrequency();
		else
			nOutOfBagObjectsNumber++;
	}

	ensure(nObjectsNumber == oaTreeDatabaseObjects->GetSize());
}

IntVector* DTDecisionTree::SelectRandomWeightedIndexes(const ContinuousVector* cumulatedWeights, const int nbDraws)
{
	assert(cumulatedWeights != NULL);
	assert(cumulatedWeights->GetSize() > 0);

	ContinuousVector* randomValues = new ContinuousVector;

	// effectuer 'nbDraws' tirages de valeurs aleatoires comprises entre 0 et la valeur max cumulee
	const Continuous maxCumulatedValue = cumulatedWeights->GetAt(cumulatedWeights->GetSize() - 1);

	for (int i = 0; i < nbDraws; i++)
		randomValues->Add(maxCumulatedValue * RandomDouble());

	// trier les valeurs aleatoires par ordre croissant
	randomValues->Sort();

	// parcourir les valeurs aleatoires venant d'etre tirees, et renseigner le tableau des index

	int lastCumulatedWeightIndex = 0;

	IntVector* result = new IntVector;

	for (int idxRandomValue = 0; idxRandomValue < randomValues->GetSize(); idxRandomValue++)
	{
		if (randomValues->GetAt(idxRandomValue) <= cumulatedWeights->GetAt(lastCumulatedWeightIndex))
			result->Add(lastCumulatedWeightIndex);
		else
		{
			// tant que la valeur aleatoire est > a la valeur cumulee, passer a la valeur cumulee suivante
			while (randomValues->GetAt(idxRandomValue) > cumulatedWeights->GetAt(lastCumulatedWeightIndex))
				lastCumulatedWeightIndex++;

			result->Add(lastCumulatedWeightIndex);
		}
	}

	assert(randomValues->GetSize() == result->GetSize());

	delete randomValues;

	return result;
}

ContinuousVector*
DTDecisionTree::ComputeAdaBoostCumulatedWeights(const NumericKeyDictionary* randomForestDatabaseObjects,
						ObjectArray* kwoObjectsList) const
{
	assert(kwoObjectsList != NULL);
	assert(kwoObjectsList->GetSize() == 0);

	ContinuousVector* result = new ContinuousVector;

	POSITION position = randomForestDatabaseObjects->GetStartPosition();
	NUMERIC key;
	Object* obj;
	int i = 0;

	while (position != NULL)
	{
		randomForestDatabaseObjects->GetNextAssoc(position, key, obj);
		DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, obj);

		assert(dbo->GetBoostingTreeWeight() > 0);

		if (i == 0)
			result->Add(dbo->GetBoostingTreeWeight());
		else
			result->Add(result->GetAt(i - 1) + dbo->GetBoostingTreeWeight());

		// garder la memoire de l'ordre des KWObjects qui correspond aux valeurs cumulees
		kwoObjectsList->Add(cast(KWObject*, (Object*)key.ToPointer()));

		i++;
	}

	return result;
}

void DTDecisionTree::MemorizeCurrentTreeInOptimalTree()
{
	DTDecisionTreeNode* node;
	Object* object;
	POSITION position;
	ALString sKey;

	// memorisation des noeuds internes dans l'arbre optimal

	odOptimalInternalNodes->RemoveAll();

	position = odCurrentInternalNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sKey, object);
		node = cast(DTDecisionTreeNode*, object);
		odOptimalInternalNodes->SetAt(sKey, node);
	}

	// memorisation des noeuds feuilles dans l'arbre optimal

	odOptimalLeaveNodes->RemoveAll();

	position = odCurrentLeaveNodes->GetStartPosition();

	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sKey, object);
		node = cast(DTDecisionTreeNode*, object);

		odOptimalLeaveNodes->SetAt(sKey, node);
	}

	if (not CheckInternalNodes() or not CheckLeavesNodes())
	{
		cout << "apres MemorizeCurrentTreeInOptimalTree, arbre courant non valide" << endl;
	}
	if (not CheckOptimalInternalNodes() or not CheckOptimalLeavesNodes())
	{
		cout << "apres MemorizeCurrentTreeInOptimalTree, arbre optimal non valide" << endl;
	}
}

void DTDecisionTree::ReplaceCurrentTreeWithOptimalTree()
{
	POSITION position;
	Object* object;
	DTDecisionTreeNode* node;
	ALString sKey;

	assert(Check());

	if (odOptimalLeaveNodes->GetCount() == 0)
		return; // cela signifie que l'arbre optimal n'a jamais ete memorise, donc l'arbre courant represente
			// deja l'arbre optimal

	if (not CheckInternalNodes() or not CheckLeavesNodes())
	{
		cout << "avant ReplaceCurrentTreeWithOptimalTree, arbre courant non valide" << endl;
	}

	// remplacer la liste des noeuds de l'arbre courant, par la liste des noeuds de l'arbre optimal
	// attention, la gestion memoire impose que les feuilles soient remplacees avant les noeuds internes
	ReplaceCurrentLeavesWithOptimalTree();
	ReplaceCurrentInternalNodesWithOptimalTree();

	if (not CheckInternalNodes() or not CheckLeavesNodes())
	{
		cout << "apres ReplaceCurrentTreeWithOptimalTree, arbre courant non valide" << endl;
	}

	if (not CheckOptimalInternalNodes() or not CheckOptimalLeavesNodes())
	{
		cout << "apres ReplaceCurrentTreeWithOptimalTree, arbre optimal non valide" << endl;
	}

	assert(odCurrentLeaveNodes->GetCount() > 0);

	odUsedVariables.DeleteAll();
	odUsedVariables.RemoveAll();

	// enregistrement des variables des noeuds internes
	position = odCurrentInternalNodes->GetStartPosition();
	nTreeDepth = 1;
	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sKey, object);
		node = cast(DTDecisionTreeNode*, object);

		// mise a jour de la profondeur de l'arbre courant
		nTreeDepth = (nTreeDepth < node->GetDepth()) ? node->GetDepth() : nTreeDepth;

		if (node->GetSplitAttributeStats() == NULL)
		{
			cout << endl << "node->GetSplitAttributeStats() == NULL" << endl;
		}
		else
			RegisterAttributeNameAndOccurenceNumber(node->GetSplitAttributeStats()->GetAttributeName());
	}
}

void DTDecisionTree::ReplaceCurrentLeavesWithOptimalTree()
{
	Object* object;
	ALString sKey;

	POSITION position = odCurrentLeaveNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentLeaveNodes->GetNextAssoc(position, sKey, object);

		if (odOptimalLeaveNodes->Lookup(sKey) == NULL and odOptimalInternalNodes->Lookup(sKey) == NULL and
		    odCurrentInternalNodes->Lookup(sKey) == NULL)
		{
			// gestion memoire : supprimer physiquement tous les noeuds feuilles de l'arbre courant, et qui
			// ne sont pas references ailleurs
			DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
			DeleteNode(node);
		}
	}

	// entre le moment ou les feuilles de l'arbre optimal ont ete memorisees, et le moment ou ces feuilles
	// remplacent les feuilles de l'arbre courant, des noeuds ont pu changer de statut (i.e, passer de noeud feuille
	// a noeud interne). Il faut donc transformer les eventuels noeuds internes de la structure odOptimalLeaveNodes,
	// en noeuds feuilles
	position = odOptimalLeaveNodes->GetStartPosition();
	while (position != NULL)
	{
		odOptimalLeaveNodes->GetNextAssoc(position, sKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (not node->IsLeaf())
		{
			node->SetLeaf(true);
			node->SetSplitAttributeStats(NULL);
			node->SetSplitVariableValueNumber(0);
			node->GetSons()->RemoveAll();
		}
	}

	// remplacer la liste courante des noeuds feuilles par la liste optimale des noeuds feuille, et reinitialiser
	// cette derniere
	odCurrentLeaveNodes->CopyFrom(odOptimalLeaveNodes);
	odOptimalLeaveNodes->RemoveAll();
}

void DTDecisionTree::ReplaceCurrentInternalNodesWithOptimalTree()
{
	Object* object;
	ALString sKey;

	POSITION position = odCurrentInternalNodes->GetStartPosition();
	while (position != NULL)
	{
		odCurrentInternalNodes->GetNextAssoc(position, sKey, object);

		if (odOptimalLeaveNodes->Lookup(sKey) == NULL and odOptimalInternalNodes->Lookup(sKey) == NULL and
		    odCurrentLeaveNodes->Lookup(sKey) == NULL)
		{
			// gestion memoire : supprimer physiquement tous les noeuds internes de l'arbre courant, et qui
			// ne sont pas references ailleurs
			DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);
			DeleteNode(node);
		}
	}

	// entre le moment ou les noeuds internes de l'arbre optimal ont ete memorisees, et le moment ou ces noeuds
	// remplacent les noeuds internes de l'arbre courant, des noeuds ont pu changer de statut (i.e, passer de noeud
	// interne a noeud feuille). Il faut donc transformer les eventuels noeuds feuilles de la structure
	// odOptimalInternalNodes, en noeuds internes
	position = odOptimalInternalNodes->GetStartPosition();
	while (position != NULL)
	{
		odOptimalInternalNodes->GetNextAssoc(position, sKey, object);
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, object);

		if (node->IsLeaf())
		{
			node->SetLeaf(false);
			// NB. en prevision, les noeuds fils de ce noeud n'avaient pas ete supprimes, lorsque le noeud
			// avait ete transforme en feuille lors de l'etape d'elaguage
		}
	}

	// remplacer la liste courante des noeuds internes par la liste optimale des noeuds internes, et reinitialiser
	// cette derniere
	odCurrentInternalNodes->CopyFrom(odOptimalInternalNodes);
	odOptimalInternalNodes->RemoveAll();
}

int DTDecisionTree::GetDatabaseInstanceId(const NumericKeyDictionary* randomForestDatabaseObjects, KWObject* kwo) const
{
	assert(randomForestDatabaseObjects != NULL);
	assert(kwo != NULL);

	Object* o = randomForestDatabaseObjects->Lookup(kwo);
	assert(o != NULL);

	DTDecisionTreeDatabaseObject* dbo = cast(DTDecisionTreeDatabaseObject*, o);
	assert(dbo->GetId() >= 0);

	return dbo->GetId();
}

const ALString& DTDecisionTree::GetRootVariableName() const
{
	return RootVariableName;
}

void DTDecisionTree::SetRootVariableName(const ALString& s)
{
	RootVariableName = s;
}

void DTDecisionTree::SetDrawingType(DrawingType d)
{
	drawingType = d;
}

void DTDecisionTree::SetUsableAttributesNumber(const int i)
{
	nUsableAttributesNumber = i;
}

void DTDecisionTree::DeleteNode(DTDecisionTreeNode* node)
{
	boolean bFound = false;
	ALString key = node->GetNodeIdentifier().GetValue();
	;

	if (odCurrentInternalNodes->RemoveKey(key))
		bFound = true;

	if (odOptimalInternalNodes->RemoveKey(key))
		bFound = true;

	if (odCurrentLeaveNodes->RemoveKey(key))
		bFound = true;

	if (odOptimalLeaveNodes->RemoveKey(key))
		bFound = true;

	if (not bFound)
		AddWarning("deleting unreferenced node " + key);

	// delete node; NVDELL
}

DTDecisionTreeNodeSplit::DTDecisionTreeNodeSplit()
{
	attributeStats = NULL;
	splittableNode = NULL;
	dTreeCost = 0;
}

DTDecisionTreeNodeSplit::~DTDecisionTreeNodeSplit() {}

void DTDecisionTreeNodeSplit::Write(ostream& os)
{
	if (splittableNode)
		os << "\tnoeud : " << splittableNode->GetNodeIdentifier();

	if (attributeStats)
		os << ", attribut : " << attributeStats->GetAttributeName();

	if (dTreeCost > 0)
		os << ", cout : " << dTreeCost;

	os << endl;
}

void DTDecisionTree::TargetModalityCount::CopyFrom(const DTDecisionTree::TargetModalityCount* source)
{
	require(source != NULL);

	sModality = source->sModality;
	iCount = source->iCount;
}
DTDecisionTree::TargetModalityCount* DTDecisionTree::TargetModalityCount::Clone() const
{
	DTDecisionTree::TargetModalityCount* copyModalityCount;

	copyModalityCount = new DTDecisionTree::TargetModalityCount;
	copyModalityCount->CopyFrom(this);

	return copyModalityCount;
}

DTDecisionTree::TargetModalityCount::TargetModalityCount()
{
	sModality = "";
	iCount = 0;
}

// fonctions de comparaison pour tri de tableau

int DTCompareInstancesOnIds(const void* elem1, const void* elem2)
{
	DTDecisionTreeDatabaseObject* dbo1 = (DTDecisionTreeDatabaseObject*)*(Object**)elem1;
	DTDecisionTreeDatabaseObject* dbo2 = (DTDecisionTreeDatabaseObject*)*(Object**)elem2;

	return (dbo2->GetId() > dbo1->GetId() ? -1 : 1);
}

int DTTargetCountCompare(const void* elem1, const void* elem2)
{
	DTDecisionTree::TargetModalityCount* i1 = (DTDecisionTree::TargetModalityCount*)*(Object**)elem1;
	DTDecisionTree::TargetModalityCount* i2 = (DTDecisionTree::TargetModalityCount*)*(Object**)elem2;

	if (i1->iCount < i2->iCount)
		return 1;
	else if (i1->iCount > i2->iCount)
		return -1;
	else
		return 0;
}

int DTDecisionTreeNodesDepthCompare(const void* elem1, const void* elem2)
{
	DTDecisionTreeNode* instance1 = cast(DTDecisionTreeNode*, *(Object**)elem1);
	DTDecisionTreeNode* instance2 = cast(DTDecisionTreeNode*, *(Object**)elem2);
	return (instance1->GetDepth() < instance2->GetDepth() ? -1 : 1);
}

const ALString GetDisplayString(const double d)
{
	ALString s(DoubleToString(d));

	return s + "\t";
}

const ALString GetDisplayString(const int d)
{
	ALString s(IntToString(d));

	return s + "\t";
}

const ALString GetDisplayString(const ALString& s)
{
	return s + (s.GetLength() < 13 ? "\t\t" : "\t");
}
