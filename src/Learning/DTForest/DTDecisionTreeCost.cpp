// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeCost.h"

DTDecisionTreeCost::DTDecisionTreeCost()
{
	nTotalAttributeNumber = -1;
	nClassValueNumber = -1;
	nDTCriterion = 0;
}

DTDecisionTreeCost::~DTDecisionTreeCost()
{
	CleanRootStat();
}

void DTDecisionTreeCost::CleanRootStat()
{
	POSITION position;
	Object* oElement;
	ALString sKey;
	IntVector* ivRootStat;

	if (odRootStat.GetCount() > 0)
	{
		position = odRootStat.GetStartPosition();
		while (position != NULL)
		{
			odRootStat.GetNextAssoc(position, sKey, oElement);
			ivRootStat = cast(IntVector*, oElement);
			delete ivRootStat;
		}

		odRootStat.RemoveAll();
	}
}

int DTDecisionTreeCost::GetTotalAttributeNumber() const
{
	return nTotalAttributeNumber;
}

void DTDecisionTreeCost::SetTotalAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nTotalAttributeNumber = nValue;
}

int DTDecisionTreeCost::GetClassValueNumber() const
{
	return nClassValueNumber;
}

void DTDecisionTreeCost::SetClassValueNumber(int nValue)
{
	require(nValue >= 0);
	nClassValueNumber = nValue;
}

double DTDecisionTreeCost::ComputeTreeConstructionCost(DTDecisionTree* tree)
{
	return tree->GetConstructionCost();
}

double DTDecisionTreeCost::ComputeTotalTreeCost(DTDecisionTree* tree)
{
	return 0;
}

double DTDecisionTreeCost::ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
							     ALString sInternalNodeKey)
{
	return 0;
}

double DTDecisionTreeCost::ComputeNodeCost(DTDecisionTreeNode* node, DTDecisionTree* tree)
{
	ObjectDictionary odAttributesToNodes;
	DTDecisionTreeNode* fatherNode;
	DTDecisionTreeNode* sonNode;
	DTDecisionTreeNode* rootNode;
	ALString sAttributeName;
	ALString sDerivationRuleName;
	IntVector* ivRootStat;
	IntVector nodenbtargetvalues;
	KWAttributeStats* attributeStats;
	double dCost;
	int nPartNumber;
	int nEvent;
	int nRootEvent;
	int nUsedAttributeNumber;
	int nIntervalEventNumber;
	int nRootEventNumber;
	int i;

	assert(node != NULL);
	assert(tree != NULL);

	require(nClassValueNumber > 0);
	require(node->GetNodeAttributeStats()->GetSize() > 0);
	// require(RootNode->GetClassStats()->IsStatsComputed());

	// Initialisation du cout
	dCost = 0.0;

	// Extraction du noeud racine
	rootNode = tree->GetRootNode();

	// Cas ou le noeud passe en parametre est le noeud racine
	if (node == tree->GetRootNode())
	{
		// Cas ou les statistiques du noeud racine n'ont pas encore ete memorisees
		if (odRootStat.GetCount() == 0)
		{
			for (i = 0; i < node->GetNodeAttributeStats()->GetSize(); i++)
			{
				attributeStats = cast(KWAttributeStats*, node->GetNodeAttributeStats()->GetAt(i));
				assert(attributeStats != NULL);

				if (attributeStats->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
				{
					// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares.
					// Le seul attribut prepare correspond ici a l'attribut cible
					cout << "nb var = " << node->GetNodeAttributeStats()->GetSize() << endl;
					cout << "var = " << attributeStats->GetAttributeName() << endl;
					cout << "var0 = "
					     << attributeStats->GetPreparedDataGridStats()
						    ->GetAttributeAt(0)
						    ->GetAttributeName()
					     << endl;
					cout << "treedepth = " << tree->GetTreeDepth() << endl;
					cout << "treeIN = " << tree->GetInternalNodes()->GetCount() << endl;
					cout << "treeleaves = " << tree->GetLeaves()->GetCount() << endl;
					// NVDELL AddWarning("DTDecisionTreeCost::ComputeNodeCost() : found only one
					// prepared variable (" +
					// attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetAttributeName()
					// + ")");
					continue;
				}
				ivRootStat = new IntVector;
				ivRootStat->SetSize(1);
				ivRootStat->SetAt(
				    0, attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber());
				odRootStat.SetAt(attributeStats->GetAttributeName(), ivRootStat);
			}
		}
	}

	// Cas d'un noeud autre que le noeud racine
	else
	{
		// Extraction du noeud pere et du noeud fils courant
		sonNode = node;
		fatherNode = sonNode->GetFatherNode();
		assert(fatherNode != NULL);

		////////////////

		// Parcours des ancetres jusqu'au noeud racine
		while (fatherNode != NULL)
		{
			// Extraction du nom de l'attribut associe au noeud pere
			sAttributeName = fatherNode->GetSplitAttributeStats()->GetAttributeName();

			// Cas ou cet attribut n'est pas encore enregistre
			if (odAttributesToNodes.Lookup(sAttributeName) == NULL)
			{
				// Enregistrement du noeud et de son attribut de partitionnement associe
				odAttributesToNodes.SetAt(sAttributeName, fatherNode);

				// Mise a jour du cout

				// Cas d'un attribut de type Continuous
				if (fatherNode->GetSplitAttributeStats()->GetAttributeType() == KWType::Continuous)
				{
					if (fatherNode->GetSplitAttributeStats()
						->GetPreparedDataGridStats()
						->GetAttributeNumber() == 1)
					{
						// a partir de learningEnv v8, les attributs a level nul ne sont plus
						// prepares. Le seul attribut prepare correspond ici a l'attribut cible
						AddWarning("ComputeNodeCost 2 : "
							   "GetPreparedDataGridStats()->GetAttributeNumber() == 1");
						continue;
					}

					nPartNumber = fatherNode->GetSplitAttributeStats()
							  ->GetPreparedDataGridStats()
							  ->GetAttributeAt(0)
							  ->GetPartNumber();

					// Ajout des termes dans le cout
					// Rq Carine : plantage ici dans le cas d'un arbre ternaire et de l'appel
					// pour le second fils : le troisieme n'est pas encore cree a ce stade
					// et fatherNode->GetSons()->GetAt(nPartNumber - 1) n'existe pas

					// possibilite correction : creeer methode qui renvoie l'index lie a un
					// DTDecisionTreeNode

					if (fatherNode->GetSonNodeIndex(sonNode) == 0 ||
					    fatherNode->GetSonNodeIndex(sonNode) == nPartNumber - 1)
					{
						dCost += log((double)(rootNode->GetObjectNumber() + 1.)) + log(2.0);
					}
					else
					{
						dCost += log((double)(rootNode->GetObjectNumber() + 2.)) +
							 log(rootNode->GetObjectNumber() + 1.);
					}

					// Nettoyage du vecteur de bornes
					// delete cvBounds;
				}

				// Cas d'un attribut de type Symbol
				else
				{
					// Extraction (par recopie, attention gestion memeoire) du tableau de groupes
					// oaGroups =
					// cast(KWDRGrouper*,rootNode->GetAssertion()->GetDerivationRule())->GetGroups();

					// Extraction du nombre d'intervalles de la regle de discretisation Is
					// nPartNumber = oaGroups->GetSize();
					attributeStats = NULL; // NV9 cast(KWAttributeStats*,
					    // node->GetNodeAttributeStats()->Lookup(->LookupAttributeStats(sAttributeName));

					// cout << "sAttributeName = " << sAttributeName << endl;

					// assert(attributeStats != NULL);

					ivRootStat = cast(IntVector*, odRootStat.Lookup(sAttributeName));

					if (ivRootStat == NULL)
					{
						// AddWarning("attribut " + sAttributeName + " non trouve dans les stats
						// racine");
						continue;
					}
					dCost += KWStat::LnBell(ivRootStat->GetAt(0), 2);

					// oaGroups->DeleteAll();
					// delete oaGroups;
				}
			}
			sonNode = fatherNode;
			fatherNode = sonNode->GetFatherNode();
		}
	}
	// Est ce bien cela que l'on veut?
	nUsedAttributeNumber = odAttributesToNodes.GetCount();

	dCost += log((double)nTotalAttributeNumber + 1);

	// Cout associe au choix des variables

	// Cas ou il y au moins un noeud interne
	// CH cas ou le noeud dont on calcule le cout n'est pas le noeud racine
	if (node->GetDepth() > 1)
	{
		dCost += KWStat::LnFactorial(nTotalAttributeNumber + nUsedAttributeNumber - 1);
		dCost -= KWStat::LnFactorial(nUsedAttributeNumber - 1);
		dCost -= KWStat::LnFactorial(nTotalAttributeNumber);
	}
	// cout commun pour le noeud

	nIntervalEventNumber = 0;
	nRootEventNumber = 0;

	POSITION position = rootNode->GetTargetModalitiesCountTrain()->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		rootNode->GetTargetModalitiesCountTrain()->GetNextAssoc(position, key, obj);
		DTDecisionTree::TargetModalityCount* tmcRoot = cast(DTDecisionTree::TargetModalityCount*, obj);

		nRootEvent = tmcRoot->iCount;
		nEvent = 0;

		Object* o = node->GetTargetModalitiesCountTrain()->Lookup(tmcRoot->sModality.GetNumericKey());

		if (o != NULL)
		{
			DTDecisionTree::TargetModalityCount* tmcEvent = cast(DTDecisionTree::TargetModalityCount*, o);
			nEvent = tmcEvent->iCount;
		}

		dCost -= KWStat::LnFactorial(nEvent);
		dCost -= KWStat::LnFactorial(nRootEvent - nEvent);
		nIntervalEventNumber += nEvent;
		nRootEventNumber += nRootEvent;
	}

	dCost += KWStat::LnFactorial(nIntervalEventNumber + rootNode->GetTargetModalitiesCountTrain()->GetCount() - 1);
	dCost += KWStat::LnFactorial(nRootEventNumber - nIntervalEventNumber +
				     rootNode->GetTargetModalitiesCountTrain()->GetCount() - 1);
	dCost -= 2.0 * KWStat::LnFactorial(rootNode->GetTargetModalitiesCountTrain()->GetCount() - 1);

	node->SetCostValue(dCost);

	return dCost;
}
