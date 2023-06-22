// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeRecursiveCost.h"
#include "DTStat.h"

DTDecisionTreeRecursiveCost::DTDecisionTreeRecursiveCost()
{
	// Par defaut (et donc en mode non expert, on prend en compte
	// le cout feuille)
	bLeafCost = false;
}

DTDecisionTreeRecursiveCost::~DTDecisionTreeRecursiveCost() {}

double DTDecisionTreeRecursiveCost::ComputeTotalTreeCost(DTDecisionTree* tree)
{
	POSITION position;
	Object* object;
	DTDecisionTreeNode* internalNode;
	DTDecisionTreeNode* leafNode;
	ALString sIdentifier;
	double dTotalCost;
	int nInternalNodesNumber;
	int nUsedAttributeNumber;

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

	// Ajout du cout correspondant au choix du nombre de variables
	dTotalCost += ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);

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

double DTDecisionTreeRecursiveCost::ComputeTreeConstructionCost(DTDecisionTree* tree)
{
	POSITION position;
	Object* object;
	DTDecisionTreeNode* internalNode;
	ALString sIdentifier;
	double dTotalCost;
	int nInternalNodesNumber;
	int nUsedAttributeNumber, i;
	DoubleVector dvCost;

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

	// Ajout du cout correspondant au choix du nombre de variables
	dTotalCost += ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);

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
			dvCost.Add(ComputeInternalNodeCost(internalNode));
			// dTotalCost += ComputeInternalNodeCost(internalNode);
		}
	}

	dvCost.Sort();

	for (i = 0; i < dvCost.GetSize(); i++)
	{
		dTotalCost += dvCost.GetAt(i);
	}
	tree->SetConstructionCost(dTotalCost);

	return tree->GetConstructionCost();
}

double DTDecisionTreeRecursiveCost::ComputeAttributeChoiceCost(int nUsedAttributeNumber, int nInternalNodeNumber)
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

double DTDecisionTreeRecursiveCost::ComputeInternalNodeCost(DTDecisionTreeNode* node)
{
	double dInternalCost;
	int nPartNumber;
	ALString sAttributeName;
	ALString sDerivationRuleName;

	require(!node->IsLeaf());
	require(node->GetSplitAttributeStats() != NULL);
	require(node->GetSplitAttributeStats()->GetPreparedDataGridStats() != NULL);
	require(node->GetSplitVariableValueNumber() > 0);

	// Termes independants de la nature de l'attribut
	dInternalCost = 0.;

	// Extraction du nom de l'attribut associe au noeud
	sAttributeName = node->GetSplitAttributeStats()->GetAttributeName();

	if (node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible NVDELL AddWarning("ComputeInternalNodeCost :
		// GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return dInternalCost;
	}

	nPartNumber = node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

	// Ajout des termes dans le cout
	switch (nDTCriterion)
	{
	case 0:
		dInternalCost += log((double)2) + log((double)node->GetSplitVariableValueNumber());
		break;
	case 1:
		dInternalCost += log(2.0) * KWStat::NaturalNumbersUniversalCodeLength(nPartNumber);
		break;
	case 2:
		dInternalCost += 2 * log((double)nPartNumber) + log(1.644934066); // pi^2/6
		break;
	case 3:
		dInternalCost += log(2.);
		break;
	case 4:
		dInternalCost += log(3.);
		break;
	case 5:
		dInternalCost += log(2.0) * DTStat::NumbersCodeLength1(nPartNumber);
		break;
	case 6:
		dInternalCost += log(2.0) * DTStat::NumbersCodeLength2(nPartNumber);
		break;
	default:
		dInternalCost += log((double)2) + log((double)node->GetSplitVariableValueNumber());
		break;
	}

	// Cas d'un attribut de type Continuous
	// if(sDerivationRuleName == "Discretization")
	if (node->GetSplitAttributeStats()->GetAttributeType() == KWType::Continuous)
	{
		dInternalCost += KWStat::LnFactorial(node->GetSplitVariableValueNumber() + nPartNumber - 1);
		dInternalCost -= KWStat::LnFactorial(nPartNumber - 1);
		dInternalCost -= KWStat::LnFactorial(node->GetSplitVariableValueNumber());
	}
	// Cas d'un attribut de type Symbol
	else
	{
		dInternalCost += KWStat::LnBell(node->GetSplitVariableValueNumber(), nPartNumber);
	}

	return (dInternalCost);
}

double DTDecisionTreeRecursiveCost::ComputeLeafCost(DTDecisionTreeNode* node)
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

	// parcours des modalites cibles
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

	switch (nDTCriterion)
	{
	case 0:
		dLeafCost += log(2.0);
		break;
	case 1:
		dLeafCost += log(2.0) * 1.518535;
		break;
	case 2:
		dLeafCost += log(1.644934066);
		break;
	case 3:
		dLeafCost += log(2.0);
		break;
	case 4:
		dLeafCost += log(3.0);
		break;
	case 5:
		dLeafCost += log(2.0);
		break;
	case 6:
		dLeafCost += log(2.0);
		break;
	default:
		dLeafCost += log(2.0);
		break;
	}

	return dLeafCost;
}

void DTDecisionTreeRecursiveCost::ComputeHypotheticalAugmentedTreeCost(DTDecisionTree* tree, double dPreviousCost,
								       DTDecisionTreeNodeSplit* nodeSplit)
{
	DTDecisionTreeNode* node;
	DTDecisionTreeNode* sourceNode;
	DTDecisionTreeNode* sonNode;
	ALString sAttributeName;
	double dCost;
	int nSonIndex;
	int nUsedAttributeNumber;
	int nVariableValueNumber;
	int nInternalNodeNumber;

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

		nVariableValueNumber = sourceNode->GetObjectNumber();
	}

	// Cas d'un attribut de type Symbol
	else
	{
		nVariableValueNumber = nodeSplit->GetAttributeStats()->GetDescriptiveStats()->GetValueNumber();
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

double DTDecisionTreeRecursiveCost::ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
								      ALString sInternalNodeKey)
{
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
	// require((0 <= nInternalNodeIndex) &&(nInternalNodeIndex < tree->GetInternalNodes()->GetCount()));

	// Initialisation du cout au cout de l'arbre precedent
	dCost = dPreviousCost;

	// Extraction du noeud dont on va elaguer les feuilles
	sourceNode = cast(DTDecisionTreeNode*, tree->GetInternalNodes()->Lookup(sInternalNodeKey));

	// Extraction du nom de la variable de partitionnement de ce noeud
	sAttributeName = sourceNode->GetSplitAttributeStats()->GetAttributeName();

	// Rq Carine : modifie car on interdit le Clone d'un noeud interne
	// Clone du noeud hypothetiquement elague
	// Le noeud pere est le meme que celui de sourceNode
	// node = sourceNode->Clone();
	// Elagage des fils du noeud clone
	// node->HypotheticallyPruneAllSons();

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