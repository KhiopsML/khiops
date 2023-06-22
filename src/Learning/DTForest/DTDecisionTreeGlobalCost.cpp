// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeGlobalCost.h"
#include "DTStat.h"

DTDecisionTreeGlobalCost::DTDecisionTreeGlobalCost() {}

DTDecisionTreeGlobalCost::~DTDecisionTreeGlobalCost() {}

double DTDecisionTreeGlobalCost::ComputeTotalTreeCost(DTDecisionTree* tree)
{
	POSITION position;
	Object* object;
	ALString sIdentifier;
	DTDecisionTreeNode* internalNode;
	DTDecisionTreeNode* leafNode;
	double dTotalCost;
	int nInternalNodesNumber;
	int nUsedAttributeNumber;
	int nLeavesNumber;

	if (nTotalAttributeNumber == 0)
		// pas d'attribut informatif ?
		return 0;

	// Initialisation du cout total au cout fixe
	// (cout constant qui depend uniquement du nombre total de variables en Used)
	dTotalCost = -KWStat::LnFactorial(nTotalAttributeNumber - 1);

	// Extraction du nombre de predicteurs utilises dans l'arbre
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	// Extraction du nombre de noeuds internes
	if (tree->GetInternalNodes() != NULL)
		nInternalNodesNumber = tree->GetInternalNodes()->GetCount();
	else
		nInternalNodesNumber = 0;

	// Extraction du nombre de feuilles
	nLeavesNumber = tree->GetLeaves()->GetCount();

	// Ajout du cout correspondant au choix du nombre de variables
	dTotalCost += ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);
	dTotalCost += ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(), nLeavesNumber);

	// Parcours des noeuds internes
	if (tree->GetInternalNodes() != NULL)
	{
		// Parcours des noeuds internes
		position = tree->GetInternalNodes()->GetStartPosition();
		while (position != NULL)
		{
			tree->GetInternalNodes()->GetNextAssoc(position, sIdentifier, object);

			// Extraction du noeud interne courant
			internalNode = cast(DTDecisionTreeNode*, object);

			// Ajout du cout de ce noeud
			dTotalCost += ComputeInternalNodeCost(internalNode);
		}
	}

	// Parcours des noeuds feuilles
	position = tree->GetLeaves()->GetStartPosition();
	while (position != NULL)
	{
		tree->GetLeaves()->GetNextAssoc(position, sIdentifier, object);

		// Extraction du noeud interne courant
		leafNode = cast(DTDecisionTreeNode*, object);

		// Ajout du cout de ce noeud
		dTotalCost += ComputeLeafCost(leafNode);
	}

	return dTotalCost;
}

double DTDecisionTreeGlobalCost::ComputeTreeConstructionCost(DTDecisionTree* tree)
{
	POSITION position;
	Object* object;
	ALString sIdentifier;
	DTDecisionTreeNode* internalNode;
	double dTotalCost;
	int nInternalNodesNumber;
	int nUsedAttributeNumber;
	int nLeavesNumber;

	if (nTotalAttributeNumber == 0)
		// pas d'attribut informatif ?
		return 0;

	// Initialisation du cout total au cout fixe
	// (cout constant qui depend uniquement du nombre total de variables en Used)
	dTotalCost = -KWStat::LnFactorial(nTotalAttributeNumber - 1);

	// Extraction du nombre de predicteurs utilises dans l'arbre
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	// Extraction du nombre de noeuds internes
	if (tree->GetInternalNodes() != NULL)
		nInternalNodesNumber = tree->GetInternalNodes()->GetCount();
	else
		nInternalNodesNumber = 0;

	// Extraction du nombre de feuilles
	nLeavesNumber = tree->GetLeaves()->GetCount();

	// Ajout du cout correspondant au choix du nombre de variables
	dTotalCost += ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);
	dTotalCost += ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(), nLeavesNumber);

	// Parcours des noeuds internes
	if (tree->GetInternalNodes() != NULL)
	{
		position = tree->GetInternalNodes()->GetStartPosition();
		while (position != NULL)
		{
			tree->GetInternalNodes()->GetNextAssoc(position, sIdentifier, object);

			// Extraction du noeud interne courant
			internalNode = cast(DTDecisionTreeNode*, object);

			// Ajout du cout de ce noeud
			dTotalCost += ComputeInternalNodeCost(internalNode);
		}
	}

	tree->SetConstructionCost(dTotalCost);

	return tree->GetConstructionCost();
}

double DTDecisionTreeGlobalCost::ComputeAttributeChoiceCost(int nUsedAttributeNumber, int nInternalNodeNumber)
{
	double dAttributeChoiceCost;

	require(nTotalAttributeNumber > 0);

	// Cout associe au choix des variables
	dAttributeChoiceCost = KWStat::LnFactorial(nTotalAttributeNumber + nUsedAttributeNumber - 1);

	dAttributeChoiceCost += log(2.0) * KWStat::NaturalNumbersUniversalCodeLength(nUsedAttributeNumber + 1);

	// Cas ou il y au moins un noeud interne
	if (nInternalNodeNumber > 0)
	{
		dAttributeChoiceCost -= KWStat::LnFactorial(nUsedAttributeNumber);
		dAttributeChoiceCost += log((double)nUsedAttributeNumber) * nInternalNodeNumber;
	}

	return dAttributeChoiceCost;
}

double DTDecisionTreeGlobalCost::ComputeStructureCost(int nInstanceNumber, int nLeaveNumber)
{
	double dStructureCost;

	require(nInstanceNumber > 0);
	require(nLeaveNumber > 0);

	// Cout associe au choix des variables
	// dstructureCost = 2*log((double)nInstanceNumber)+1 ;
	// C3 dans l'annexe technique ?
	dStructureCost = log(2.0) * KWStat::NaturalNumbersUniversalCodeLength(nLeaveNumber);

	// Cas ou il y au moins un noeud interne
	if (nLeaveNumber > 0)
	{
		if (this->nDTCriterion == 0)
		{
			dStructureCost += DTStat::LnCatalan(nLeaveNumber - 1);
		}
		else if (this->nDTCriterion == 1)
		{
			dStructureCost += DTStat::LnSchroder(nLeaveNumber - 1);
		}
	}

	return dStructureCost;
}

double DTDecisionTreeGlobalCost::ComputeInternalNodeCost(DTDecisionTreeNode* node)
{
	double dInternalCost;
	int nPartNumber;
	ALString sAttributeName;
	ALString sDerivationRuleName;
	// ContinuousVector* cvBounds;
	// ObjectArray* oaGroups;

	require(!node->IsLeaf());
	require(node->GetSplitAttributeStats() != NULL);
	require(node->GetSplitVariableValueNumber() > 0);

	// Termes independants de la nature de l'attribut
	dInternalCost = 0.;

	// Extraction du nom de l'attribut associe au noeud
	sAttributeName = node->GetSplitAttributeStats()->GetAttributeName();

	if (node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible
		AddWarning("ComputeInternalNodeCost : GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return dInternalCost;
	}

	// Extraction du nombre de parties du partitionnement
	nPartNumber = node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

	// Cas d'un attribut de type Continuous
	// if(sDerivationRuleName == "Discretization")
	if (node->GetSplitAttributeStats()->GetAttributeType())
	{
		// Extraction (par recopie, attention gestion memoire) de l'intervalle de bornes
		// cvBounds = cast(KWDRDiscretizer*,node->GetAssertion()->GetDerivationRule())->GetIntervalBounds();

		// Extraction du nombre d'intervalles de la regle de discretisation Is
		// nPartNumber = cvBounds->GetSize()+1;

		// Ajout des termes dans le cout
		// if (nDTCriterion==0)
		// dInternalCost += log((double)node->GetSplitVariableValueNumber());
		// if (nDTCriterion==1)
		// dInternalCost += 2*log((double)nPartNumber-1)+1;
		dInternalCost += KWStat::LnFactorial(node->GetSplitVariableValueNumber() + nPartNumber - 1);
		dInternalCost -= KWStat::LnFactorial(nPartNumber - 1);
		dInternalCost -= KWStat::LnFactorial(node->GetSplitVariableValueNumber());

		// Nettoyage du vecteur de bornes
		// delete cvBounds;
	}

	// Cas d'un attribut de type Symbol
	else
	{
		// Extraction (par recopie, attention gestion memeoire) du tableau de groupes
		// oaGroups = cast(KWDRGrouper*,node->GetAssertion()->GetDerivationRule())->GetGroups();

		// Extraction du nombre d'intervalles de la regle de discretisation Is
		// nPartNumber = oaGroups->GetSize();

		// Ajout des termes dans le cout
		// if (nDTCriterion==1)
		// dInternalCost += 2*log((double)nPartNumber-1)+1;
		// if (nDTCriterion==0)
		dInternalCost += log((double)node->GetSplitVariableValueNumber());
		dInternalCost += KWStat::LnBell(node->GetSplitVariableValueNumber(), nPartNumber);

		// oaGroups->DeleteAll();
		// delete oaGroups;
	}

	return (dInternalCost);
}

double DTDecisionTreeGlobalCost::ComputeLeafCost(DTDecisionTreeNode* node)
{
	double dLeafCost;
	int nEvent;
	int nIntervalEventNumber;

	require(node != NULL);
	require(node->GetTargetModalitiesCountTrain() != NULL);
	require(nClassValueNumber > 0);

	// Cout de codage des instances du noeud et de la loi multinomiale du noeud
	dLeafCost = 0;
	nIntervalEventNumber = 0;

	POSITION position = node->GetTargetModalitiesCountTrain()->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		node->GetTargetModalitiesCountTrain()->GetNextAssoc(position, key, obj);

		DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);
		nEvent = tmc->iCount;
		dLeafCost -= KWStat::LnFactorial(nEvent);
		nIntervalEventNumber += nEvent;
	}
	dLeafCost += KWStat::LnFactorial(nIntervalEventNumber + nClassValueNumber - 1);
	dLeafCost -= KWStat::LnFactorial(nClassValueNumber - 1);

	return dLeafCost;
}

double DTDecisionTreeGlobalCost::ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
								   ALString sInternalNodeKey)
{
	// KWAssertion* nodeAssertion;
	DTDecisionTreeNode* node;
	DTDecisionTreeNode* sourceNode;
	DTDecisionTreeNode* sonNode;
	ALString sAttributeName;
	double dCost;
	int nSonIndex;
	int nUsedAttributeNumber;
	int nInternalNodeNumber;

	require(tree != NULL);
	require(tree->GetInternalNodes() != NULL);

	// Initialisation du cout au cout de l'arbre precedent
	dCost = dPreviousCost;

	// Extraction du noeud dont on va elaguer les feuilles
	sourceNode = cast(DTDecisionTreeNode*, tree->GetInternalNodes()->Lookup(sInternalNodeKey));

	// nodeAssertion = sourceNode->GetAssertion();

	// Extraction du nom de la variable de partitionnement de ce noeud
	sAttributeName = sourceNode->GetSplitAttributeStats()->GetAttributeName();

	// Creation d'un noeud receptacle du noeud hypothetique elague
	node = new DTDecisionTreeNode;
	node->CopyWithPruningSons(sourceNode);

	// Extraction du nombre de predicteurs utilises dans l'arbre non elague
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	// Extraction du nombre de noeuds internes de l'arbre non elague
	nInternalNodeNumber = tree->GetInternalNodes()->GetCount();

	// Cas ou l'assertion du noeud elague porte sur une variable non utilisee ailleurs dans l'arbre
	const IntObject* ioOccurences = cast(IntObject*, tree->GetUsedVariablesDictionary()->Lookup(sAttributeName));
	if (ioOccurences->GetInt() == 1)
	{
		// Soustraction du cout de choix des variables pour l'arbre non elague
		dCost -= ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodeNumber);

		// Ajout du cout de choix des variables pour l'arbre elague
		dCost += ComputeAttributeChoiceCost(nUsedAttributeNumber - 1, nInternalNodeNumber - 1);
	}
	else
	{
		// Ajout de la difference de cout de choix des variables pour l'arbre elague
		dCost -= log((double)nUsedAttributeNumber);
	}

	// Variation du cout de structure
	dCost -= ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(), tree->GetLeaves()->GetCount());
	dCost += ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(),
				      tree->GetLeaves()->GetCount() - sourceNode->GetSons()->GetSize() + 1);

	// Soustraction du cout de ce noeud en tant que noeud interne
	dCost -= ComputeInternalNodeCost(sourceNode);

	// Parcours des feuilles elaguees
	for (nSonIndex = 0; nSonIndex < sourceNode->GetSons()->GetSize(); nSonIndex++)
	{
		// Extraction de la feuille
		sonNode = cast(DTDecisionTreeNode*, sourceNode->GetSons()->GetAt(nSonIndex));

		// Soustraction du cout de la feuille courante
		dCost -= ComputeLeafCost(sonNode);
	}

	// Ajout du cout du noeud en tant que feuille
	dCost += ComputeLeafCost(node);

	// Nettoyage du noeud hypothetique
	delete node;

	return dCost;
}

void DTDecisionTreeGlobalCost::ComputeHypotheticalAugmentedTreeCost(DTDecisionTree* tree, double dPreviousCost,
								    DTDecisionTreeNodeSplit* nodeSplit)
{
	DTDecisionTreeNode* node;
	DTDecisionTreeNode* sourceNode;
	DTDecisionTreeNode* sonNode;
	ALString sAttributeName;
	ALString sDerivationRuleName;
	double dCost;
	int nSonIndex;
	int nUsedAttributeNumber;
	int nVariableValueNumber;
	int nInternalNodeNumber;
	int nPartNumber;

	require(tree != NULL);
	require(nodeSplit != NULL);
	require(nodeSplit->GetAttributeStats() != NULL);
	require(nodeSplit->GetSplittableNode() != NULL);

	// Initialisation du cout au cout de l'arbre precedent
	dCost = dPreviousCost;

	// Extraction du noeud que l'on va cloner pour l'enrichir de feuilles
	sourceNode =
	    cast(DTDecisionTreeNode*, tree->GetLeaves()->Lookup(nodeSplit->GetSplittableNode()->GetNodeIdentifier()));

	// Extraction du nom de la variable de partitionnement
	sAttributeName = nodeSplit->GetAttributeStats()->GetAttributeName();

	// Extraction du nombre de valeurs de cette variable
	// (nombre d'instances dans le cas d'une variable Continuous
	// et nombre de modalites distinctes dans le cas d'une variable Symbol)

	// Cas d'un attribut de type Continuous
	if (nodeSplit->GetAttributeStats()->GetAttributeType() == KWType::Continuous)
	{
		// Nombre total d'individus pour la base du noeud
		// CH nVariableValueNumber = sourceNode->GetClassStats()->GetDatabaseObjectNumber();
		nVariableValueNumber = sourceNode->GetObjectNumber();
	}
	// Cas d'un attribut de type Symbol
	else
	{
		// Nombre de valeurs distinctes pour l'attribut
		nVariableValueNumber =
		    0; // NV9  cast(KWAttributeStats*,
		       // sourceNode->GetNodeClassStats()->LookupAttributeStats(sAttributeName))->GetDescriptiveStats()->GetValueNumber();
	}
	assert(nVariableValueNumber > 0);

	// Clone de la feuille hypothetiquement transformee en noeud interne
	// Le noeud pere est le meme que celui de sourceNode
	node = sourceNode->Clone();

	// Ajout des feuilles au noeud clone
	node->SetUpAttributeAndHypotheticalSons(nodeSplit->GetAttributeStats(), nVariableValueNumber);

	// Extraction du nombre de predicteurs utilises dans l'arbre non augmente
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	// Extraction du nombre de noeuds internes de l'arbre non augmente
	if (tree->GetInternalNodes() != NULL)
		nInternalNodeNumber = tree->GetInternalNodes()->GetCount();
	else
		nInternalNodeNumber = 0;

	assert(node->GetSplitAttributeStats() != NULL);

	// Cas ou l'assertion porte sur une variable non encore utilisee dans l'arbre
	if (tree->GetUsedVariablesDictionary()->Lookup(node->GetSplitAttributeStats()->GetAttributeName()) == NULL)
	{
		// Soustraction du cout de choix des variables pour l'arbre non augmente
		dCost -= ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodeNumber);

		// Ajout du cout de choix des variables pour l'arbre augmente
		dCost += ComputeAttributeChoiceCost(nUsedAttributeNumber + 1, nInternalNodeNumber + 1);
	}
	else
	{
		// Ajout de la difference de cout de choix des variables pour l'arbre augmente
		dCost += log((double)nUsedAttributeNumber);
	}

	dCost -= ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(), tree->GetLeaves()->GetCount());

	if (nodeSplit->GetAttributeStats()->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible NVDELL AddWarning("ComputeHypotheticalAugmentedTreeCost :
		// GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return;
	}

	// Extraction du nombre de parties
	nPartNumber = nodeSplit->GetAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

	dCost += ComputeStructureCost(tree->GetRootNode()->GetObjectNumber(),
				      tree->GetLeaves()->GetCount() + nPartNumber - 1);

	// Ajout du cout du nouveau noeud interne
	dCost += ComputeInternalNodeCost(node);

	// Soustraction du cout de ce noeud en tant que feuille
	dCost -= ComputeLeafCost(node);

	// Parcours des feuilles
	for (nSonIndex = 0; nSonIndex < node->GetSons()->GetSize(); nSonIndex++)
	{
		// Extraction de la feuille
		sonNode = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(nSonIndex));

		// Ajout du cout de la feuille courant
		dCost += ComputeLeafCost(sonNode);
	}

	// Nettoyage des fils du noeud hypothetique
	for (nSonIndex = 0; nSonIndex < node->GetSons()->GetSize(); nSonIndex++)
	{
		// Destruction de la feuille
		delete (cast(DTDecisionTreeNode*, node->GetSons()->GetAt(nSonIndex)));
	}
	node->GetSons()->RemoveAll();

	// Nettoyage du noeud hypothetique
	delete node;

	nodeSplit->SetTreeCost(dCost);
}