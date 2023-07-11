// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionBinaryTreeCost.h"
#include "DTStat.h"

DTDecisionBinaryTreeCost::DTDecisionBinaryTreeCost() {}

DTDecisionBinaryTreeCost::~DTDecisionBinaryTreeCost() {}

double DTDecisionBinaryTreeCost::ComputeTotalTreeCost(DTDecisionTree* tree)
{
	POSITION position;
	Object* object;
	DTDecisionTreeNode* internalNode;
	DTDecisionTreeNode* leafNode;
	ALString sIdentifier;
	double dCost;
	int nInternalNodesNumber;
	int nUsedAttributeNumber;

	if (nTotalAttributeNumber == 0)
		// pas d'attribut informatif ?
		return 0;

	// Extraction du nombre de predicteurs utilises dans l'arbre
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	// Extraction du nombre de noeuds internes
	if (tree->GetInternalNodes() != NULL)
		nInternalNodesNumber = tree->GetInternalNodes()->GetCount();
	else
		nInternalNodesNumber = 0;

	// Ajout du cout correspondant au choix du nombre de variables
	dCost = ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);

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
			dCost += ComputeInternalNodeCost(tree, internalNode);
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
		dCost += ComputeLeafCost(leafNode);
	}

	return dCost;
}

double DTDecisionBinaryTreeCost::ComputeTreeConstructionCost(DTDecisionTree* tree)
{
	DoubleVector dvCost;
	POSITION position;
	Object* object;
	DTDecisionTreeNode* internalNode;
	ALString sIdentifier;
	double dCost;
	int i;
	int nInternalNodesNumber;
	int nUsedAttributeNumber;

	// Extraction du nombre de predicteurs utilises dans l'arbre
	nUsedAttributeNumber = tree->GetUsedVariablesDictionary()->GetCount();

	if (nUsedAttributeNumber == 0 || nTotalAttributeNumber == 0)
	{ // pas d'attribut informatif ?
		tree->SetConstructionCost(0.0);
		return 0;
	}
	// Extraction du nombre de noeuds internes
	if (tree->GetInternalNodes() != NULL)
		nInternalNodesNumber = tree->GetInternalNodes()->GetCount();
	else
		nInternalNodesNumber = 0;
	// Cout choix entre modele nul et modele informatif
	dCost = log(2.0) * (1.0 + nInternalNodesNumber);
	dCost += log(1.0 * nUsedAttributeNumber) * nInternalNodesNumber;

	// Choix du nombre de variables

	dCost += KWStat::KWStat::NaturalNumbersUniversalCodeLength(nUsedAttributeNumber);

	// choix des variables
	dCost -= KWStat::LnFactorial(nUsedAttributeNumber);

	// Ajout du cout correspondant au choix du nombre de variables
	// dCost += ComputeAttributeChoiceCost(nUsedAttributeNumber, nInternalNodesNumber);

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
			dvCost.Add(ComputeInternalNodeCost(tree, internalNode));
			// dTotalCost += ComputeInternalNodeCost(internalNode);
		}
	}

	dvCost.Sort();

	for (i = 0; i < dvCost.GetSize(); i++)
	{
		dCost += dvCost.GetAt(i);
	}
	tree->SetConstructionCost(dCost);

	return tree->GetConstructionCost();
}

double DTDecisionBinaryTreeCost::ComputeAttributeChoiceCost(int nUsedAttributeNumber, int nInternalNodeNumber)
{
	double dAttributeChoiceCost;

	require(nTotalAttributeNumber > 0);
	require(nUsedAttributeNumber >= 0);
	require(nInternalNodeNumber >= nUsedAttributeNumber);

	if (nUsedAttributeNumber == 0)
		return log(2.0);

	// Cout choix entre modele nul et modele informatif
	dAttributeChoiceCost = log(2.0) * (1.0 + nInternalNodeNumber);
	dAttributeChoiceCost += log(1.0 * nUsedAttributeNumber) * nInternalNodeNumber;

	// Choix du nombre de variables

	dAttributeChoiceCost += KWStat::KWStat::NaturalNumbersUniversalCodeLength(nUsedAttributeNumber);

	// choix des variables
	dAttributeChoiceCost -= KWStat::LnFactorial(nUsedAttributeNumber);

	return dAttributeChoiceCost;
}

double DTDecisionBinaryTreeCost::ComputeInternalNodeCost(DTDecisionTree* tree, DTDecisionTreeNode* node)
{
	double dInternalCost;
	int nPartNumber;
	ALString sAttributeName;
	ALString sDerivationRuleName;
	KWAttribute* attribute;
	int nGranularity;
	int nInstanceNumber;

	require(!node->IsLeaf());
	require(node->GetSplitAttributeStats() != NULL);
	require(node->GetSplitAttributeStats()->GetPreparedDataGridStats() != NULL);
	require(node->GetSplitVariableValueNumber() > 0);

	// Extraction du nom de l'attribut associe au noeud
	sAttributeName = node->GetSplitAttributeStats()->GetAttributeName();

	attribute = tree->GetLearningSpec()->GetClass()->LookupAttribute(sAttributeName);
	// Termes independants de la nature de l'attribut
	dInternalCost = attribute->GetCost();

	if (node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible
		// NVDELL AddWarning("ComputeInternalNodeCost : GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return dInternalCost;
	}

	nInstanceNumber = node->GetSplitAttributeStats()->GetInstanceNumber();
	nGranularity = node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetGranularity();

	dInternalCost +=
	    KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, (int)log2(0.5 * nInstanceNumber));
	// Extraction de l'attribut de partitionnement

	nPartNumber = node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

	// Cas d'un attribut de type Continuous
	// if(sDerivationRuleName == "Discretization")
	require(nPartNumber <= pow(2.0, nGranularity));

	if (node->GetSplitAttributeStats()->GetAttributeType() == KWType::Continuous)
	{
		const KWDGSAttributeDiscretization* splitAttributePartition =
		    cast(KWDGSAttributeDiscretization*,
			 node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0));
		dInternalCost += log(splitAttributePartition->GetGranularizedValueNumber() - 1.0);
	}
	// Cas d'un attribut de type Symbol
	else
	{
		const KWDGSAttributeGrouping* splitAttributePartition =
		    cast(KWDGSAttributeGrouping*,
			 node->GetSplitAttributeStats()->GetPreparedDataGridStats()->GetAttributeAt(0));
		int nVG1 = splitAttributePartition->GetGroupValueNumberAt(0);
		int nVG2 = splitAttributePartition->GetGroupValueNumberAt(1);
		int nVGmin = (nVG2 > nVG1) ? nVG1 : nVG2;
		if (nVGmin > 1 and (nVG1 + nVG2) > 2)
			dInternalCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nVGmin - 1, nVG1 + nVG2 - 2);
		dInternalCost -= KWStat::LnFactorial(nVGmin);
		dInternalCost += 1.0 * nVGmin * log(nVG1 + nVG2);
	}

	return (dInternalCost);
}

double DTDecisionBinaryTreeCost::ComputeLeafCost(DTDecisionTreeNode* node)
{
	double dLeafCost;
	int nEvent;
	int nIntervalEventNumber;

	require(node != NULL);
	require(node->GetTargetModalitiesCountTrain() != NULL);
	require(nClassValueNumber > 0);

	// Cout de codage des instances du noeud et de la loi multinomiale du noeud
	dLeafCost = log(2.0);
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

	return dLeafCost;
}

void DTDecisionBinaryTreeCost::ComputeHypotheticalAugmentedTreeCost(DTDecisionTree* tree, double dPreviousCost,
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
		// Cout choix entre modele nul et modele informatif
		dCost += log(2.0);
		dCost += log(1.0 * nUsedAttributeNumber);
	}

	// Ajout du cout du nouveau noeud interne
	dCost += ComputeInternalNodeCost(tree, node);

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

double DTDecisionBinaryTreeCost::ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
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
		dCost -= log(2.0);
		dCost -= log(1.0 * nUsedAttributeNumber);
	}

	// Soustraction du cout de ce noeud en tant que noeud interne
	dCost -= ComputeInternalNodeCost(tree, sourceNode);

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
