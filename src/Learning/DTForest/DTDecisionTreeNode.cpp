// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeNode.h"
#include "DTDecisionTree.h"
#include "DTDecisionTreeCreationTask.h"
#include "DTConfig.h"

DTDecisionTreeNode::DTDecisionTreeNode()
{
	bIsLeaf = true;
	bCanBePruned = false;
	fatherNode = NULL;
	nDepth = -1;
	dNodeCost = -1;
	splitAttributeStats = NULL;
	nSplitVariableValueNumber = 0;
	cPurity = -1;
	nObjectNumber = -1;
	nOutOfBagObjectNumber = -1;
	bCanBecomeInternal = true;
	bAttributeStatsBelongsToNode = false;

	nkdTargetModalitiesCountTrain = NULL;
	nkdTargetModalitiesCountOutOfBag = NULL;

	svReferenceTargetModalities = NULL;

	baseloaderOutOfBag = NULL;
	baseloaderTrain = NULL;

	oaAttributeStats = NULL;
	nodeLearningSpec = NULL;
}

DTDecisionTreeNode::~DTDecisionTreeNode()
{
	if (oaAttributeStats != NULL && bAttributeStatsBelongsToNode)
	{
		oaAttributeStats->DeleteAll();
		delete oaAttributeStats;
	}

	if (nkdTargetModalitiesCountTrain != NULL)
	{
		nkdTargetModalitiesCountTrain->DeleteAll();
		delete nkdTargetModalitiesCountTrain;
	}

	if (nkdTargetModalitiesCountOutOfBag != NULL)
	{
		nkdTargetModalitiesCountOutOfBag->DeleteAll();
		delete nkdTargetModalitiesCountOutOfBag;
	}

	if (nodeLearningSpec != NULL)
	{
		delete nodeLearningSpec;
	}
}

double DTDecisionTreeNode::GetSortValue() const
{
	return StringToDouble(sIdentifier);
}

double DTDecisionTreeNode::GetCostValue() const
{
	return dNodeCost;
}

void DTDecisionTreeNode::SetCostValue(double dcost)
{
	dNodeCost = dcost;
}

DTDecisionTreeNode* DTDecisionTreeNode::GetRoot() const
{
	if (fatherNode == NULL)
		return (DTDecisionTreeNode*)this;

	return fatherNode->GetRoot();
}

DTDecisionTreeNode* DTDecisionTreeNode::GetFatherNode() const
{
	return fatherNode;
}

void DTDecisionTreeNode::SetFatherNode(DTDecisionTreeNode* node)
{
	fatherNode = node;
}

int DTDecisionTreeNode::GetDepth() const
{
	return nDepth;
}

void DTDecisionTreeNode::SetDepth(int i)
{
	nDepth = i;
}

const Symbol& DTDecisionTreeNode::GetNodeIdentifier() const
{
	return sIdentifier;
}

void DTDecisionTreeNode::SetNodeIdentifier(const Symbol& s)
{
	sIdentifier = s;
}

ObjectArray* DTDecisionTreeNode::GetSons() const
{
	return (ObjectArray*)&oaSonNodes;
}

int DTDecisionTreeNode::GetSonNodeIndex(DTDecisionTreeNode* sonNode)
{
	assert(sonNode != NULL);

	for (int i = 0; i < oaSonNodes.GetSize(); i++)
	{
		DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, oaSonNodes.GetAt(i));
		assert(node != NULL);
		if (node->GetNodeIdentifier() == sonNode->GetNodeIdentifier())
			return i;
	}

	return -1;
}

void DTDecisionTreeNode::AddSonNode(DTDecisionTreeNode* sonNode)
{
	oaSonNodes.Add(sonNode);
}

ObjectArray* DTDecisionTreeNode::ComputeNodeLeaves()
{
	ObjectArray* oaLeaves;
	DTDecisionTreeNode* sonNode;
	int i;

	oaLeaves = new ObjectArray;
	if (bIsLeaf)
	{
		oaLeaves->Add(this);
		return oaLeaves;
	}

	for (i = 0; i < oaSonNodes.GetSize(); i++)
	{
		sonNode = cast(DTDecisionTreeNode*, oaSonNodes.GetAt(i));
		oaLeaves->ConcatFrom(oaLeaves, sonNode->ComputeNodeLeaves());
	}
	return oaLeaves;
}

KWAttributeStats* DTDecisionTreeNode::GetSplitAttributeStats() const
{
	return splitAttributeStats;
}

void DTDecisionTreeNode::SetSplitAttributeStats(KWAttributeStats* attributeStats)
{
	splitAttributeStats = attributeStats;
}

void DTDecisionTreeNode::SetSplitVariableValueNumber(int nValueNumber)
{
	require(nValueNumber >= 0);
	nSplitVariableValueNumber = nValueNumber;
}

int DTDecisionTreeNode::GetSplitVariableValueNumber() const
{
	return nSplitVariableValueNumber;
}

NumericKeyDictionary* DTDecisionTreeNode::GetTargetModalitiesCountTrain() const
{
	return nkdTargetModalitiesCountTrain;
}

void DTDecisionTreeNode::SetTargetModalitiesCountTrain(NumericKeyDictionary* n)
{
	nkdTargetModalitiesCountTrain = n;
}
NumericKeyDictionary* DTDecisionTreeNode::GetTargetModalitiesCountOutOfBag()
{
	return nkdTargetModalitiesCountOutOfBag;
}

void DTDecisionTreeNode::SetTargetModalitiesCountOutOfBag(NumericKeyDictionary* n)
{
	nkdTargetModalitiesCountOutOfBag = n;
}
Continuous DTDecisionTreeNode::GetPurity()
{
	return cPurity;
}

void DTDecisionTreeNode::SetPurity(Continuous cValue)
{
	cPurity = cValue;
}

int DTDecisionTreeNode::GetObjectNumber() const
{
	return nObjectNumber;
}

void DTDecisionTreeNode::SetObjectNumber(int nNumber)
{
	nObjectNumber = nNumber;
}

int DTDecisionTreeNode::GetOutOfBagObjectNumber() const
{
	return nOutOfBagObjectNumber;
}

void DTDecisionTreeNode::SetOutOfBagObjectNumber(int nNumber)
{
	nOutOfBagObjectNumber = nNumber;
}

ObjectArray* DTDecisionTreeNode::GetNodeAttributeStats()
{
	return oaAttributeStats;
}

void DTDecisionTreeNode::SetNodeAttributeStats(ObjectArray* at)
{
	if (oaAttributeStats != NULL)
	{
		oaAttributeStats->DeleteAll();
		delete oaAttributeStats;
	}
	oaAttributeStats = at;
}

// KWLearningSpec* DTDecisionTreeNode::GetNodeLearningSpec()
//{
//	return &nodeLearningSpec;
// }
//
// void DTDecisionTreeNode::SetLearningSpecCopy(KWLearningSpec* learningSpec)
//{
//	nodeLearningSpec.CopyFrom(learningSpec);
// }

void DTDecisionTreeNode::SetUpAttributeAndHypotheticalSons(KWAttributeStats* attributeStats, int nValueNumber)
{
	DTDecisionTreeNode* node;
	int nSonIndex;
	int nSonNumber;
	int nMajoritarySize;
	int nTotalSize;

	require(attributeStats != NULL);

	// Passage du statut de noeud terminal a noeud interne
	bIsLeaf = false;

	// Attribution de l'assertion de partitionnement
	splitAttributeStats = attributeStats;
	nSplitVariableValueNumber = nValueNumber;

	if (splitAttributeStats->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible
		// NVDELL AddWarning("SetUpAttributeAndHypotheticalSons :
		// GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return;
	}

	// Extraction du nombre de noeuds fils et creation du tableau associe
	nSonNumber = splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();
	oaSonNodes.SetSize(nSonNumber);

	// Extraction du nombre de classes cible
	// nClassNumber = splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(1)->GetPartNumber();

	// Creation et initialisation des noeuds fils
	for (nSonIndex = 0; nSonIndex < nSonNumber; nSonIndex++)
	{
		// Creation du noeud (enregistrement automatique dans l'arbre)
		node = new DTDecisionTreeNode;

		// Enregistrement du noeud dans le tableau des noeuds fils
		oaSonNodes.SetAt(nSonIndex, node);

		node->SetFatherNode(this);

		node->SetDepth(nDepth + 1);

		// Attribution d'une copie du learningSpec identique a celui du noeud pere
		// NV V9 node->SetLearningSpecCopy(GetNodeLearningSpec());

		// Extraction de la distribution de la classe cible du fils a partir de la distribution de son pere
		node->nkdTargetModalitiesCountTrain = node->GetFatherNode()->ComputeSonNodeTargetModalitiesCount(
		    nSonIndex, nkdTargetModalitiesCountTrain);

		node->nkdTargetModalitiesCountOutOfBag = node->GetFatherNode()->ComputeSonNodeTargetModalitiesCount(
		    nSonIndex, nkdTargetModalitiesCountOutOfBag);

		// Initialisation de l'effectif de la classe majoritaire pour calcul
		// de la purete du noeud
		nMajoritarySize = 0;
		nTotalSize = 0;

		// Parcours des classes
		POSITION position = node->nkdTargetModalitiesCountTrain->GetStartPosition();
		NUMERIC key;
		Object* obj;

		while (position != NULL)
		{
			node->nkdTargetModalitiesCountTrain->GetNextAssoc(position, key, obj);

			DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);

			nTotalSize += tmc->iCount;

			// Mise a jour de l'effectif max
			if (tmc->iCount > nMajoritarySize)
				nMajoritarySize = tmc->iCount;
		}
		// Initialisation de la purete du noeud
		// definie comme le ratio nombre d'individus de la classe majoritaire / nbre d'individus du noeud
		node->SetPurity((Continuous)nMajoritarySize / nTotalSize);
	}
}

NumericKeyDictionary*
DTDecisionTreeNode::ComputeSonNodeTargetModalitiesCount(int nSonIndex,
							const NumericKeyDictionary* targetModalitiesCount)
{
	KWDataGridStats* dataGridStats;
	IntVector ivPartIndexes;
	int nSize;
	int nNodeModalityIndex;

	if (targetModalitiesCount == NULL)
		return NULL;

	NumericKeyDictionary* sonTargetModalitiesCount = new NumericKeyDictionary;

	require(splitAttributeStats != NULL);

	// Extraction de la grille des cellules
	dataGridStats = splitAttributeStats->GetPreparedDataGridStats();

	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());

	POSITION position = targetModalitiesCount->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		targetModalitiesCount->GetNextAssoc(position, key, obj);

		DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);

		if (splitAttributeStats->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
		{
			// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut
			// prepare correspond ici a l'attribut cible
			// NVDELL AddWarning("ComputeSonNodeTargetModalitiesCountOutOfBag :
			// GetPreparedDataGridStats()->GetAttributeNumber() == 1");
			continue;
		}
		// Extraction de l'indice de la modalite courante de l'assertion de reference de l'arbre dans
		// l'assertion de partitionnement cible du noeud
		nNodeModalityIndex = cast(KWDGSAttributeSymbolValues*, dataGridStats->GetAttributeAt(1))
					 ->ComputeSymbolPartIndex(tmc->sModality);

		// Attention a integrer
		// Cas ou la modalite courante n'est pas presente dans l'assertion cible
		// du noeud pere car aucun individu de sa sous base ne prend cette modalite
		if (nNodeModalityIndex == -1)
			nSize = 0;
		else
			// Extraction de l'effectif a partir de la table
			nSize = dataGridStats->GetBivariateCellFrequencyAt(nSonIndex, nNodeModalityIndex);

		// Affectation de l'effectif de la modalite cible
		DTDecisionTree::TargetModalityCount* sonTmc = new DTDecisionTree::TargetModalityCount;
		sonTmc->sModality = tmc->sModality;
		sonTmc->iCount = nSize;
		sonTargetModalitiesCount->SetAt(sonTmc->sModality.GetNumericKey(), sonTmc);
	}

	// delete dataGridStats;
	return sonTargetModalitiesCount;
}

boolean DTDecisionTreeNode::IsLeaf() const
{
	return bIsLeaf;
}

boolean DTDecisionTreeNode::IsPrunable() const
{
	return bCanBePruned;
}

void DTDecisionTreeNode::SetLeaf(boolean b)
{
	if (fatherNode == NULL)
		bIsLeaf = b;
	bIsLeaf = b;
}

void DTDecisionTreeNode::SetPruningStatus(boolean bStatus)
{
	bCanBePruned = bStatus;
}

boolean DTDecisionTreeNode::IsContinuous() const
{
	require(splitAttributeStats != NULL);

	if (splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetClassLabel() == "Intervals")
		return true;
	else
		return false;
}

boolean DTDecisionTreeNode::IsSymbol() const
{
	require(splitAttributeStats != NULL);

	if (splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetClassLabel() == "Value groups")
		return true;
	else
		return false;
}

DTDecisionTreeNode* DTDecisionTreeNode::Clone()
{
	DTDecisionTreeNode* node = new DTDecisionTreeNode;
	node->CopyFrom(this);

	return node;
}

void DTDecisionTreeNode::CopyFrom(DTDecisionTreeNode* node)
{
	require(bIsLeaf);
	require(node->splitAttributeStats == NULL);

	// pas de Clone ou CopyFrom pour attributeStats
	// consequence pas de delete a la destruction
	bCanBePruned = node->bCanBePruned;
	bCanBecomeInternal = node->bCanBecomeInternal;
	bIsLeaf = node->bIsLeaf;
	cPurity = node->cPurity;
	dNodeCost = node->dNodeCost;
	// NV V9 GetNodeClassStats()->SetLearningSpec(node->GetNodeLearningSpec());
	fatherNode = node->fatherNode;
	nDepth = node->nDepth;
	// NV V9	nodeLearningSpec.CopyFrom(node->GetNodeLearningSpec());
	nObjectNumber = node->nObjectNumber;
	nOutOfBagObjectNumber = node->nOutOfBagObjectNumber;
	nSplitVariableValueNumber = node->nSplitVariableValueNumber;
	svReferenceTargetModalities = node->GetReferenceTargetModalities();

	// copier les objets DTDecisionTree::TargetModalityCountTrain
	if (nkdTargetModalitiesCountTrain != NULL)
		nkdTargetModalitiesCountTrain->DeleteAll();
	else
		nkdTargetModalitiesCountTrain = new NumericKeyDictionary;

	POSITION position = node->nkdTargetModalitiesCountTrain->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		node->nkdTargetModalitiesCountTrain->GetNextAssoc(position, key, obj);
		DTDecisionTree::TargetModalityCount* source = cast(DTDecisionTree::TargetModalityCount*, obj);
		DTDecisionTree::TargetModalityCount* target = new DTDecisionTree::TargetModalityCount;
		target->CopyFrom(source);
		nkdTargetModalitiesCountTrain->SetAt(target->sModality.GetNumericKey(), target);
	}

	if (node->nkdTargetModalitiesCountOutOfBag == NULL)
		return;

	// copier les objets DTDecisionTree::TargetModalityCountOutOfBag
	if (nkdTargetModalitiesCountOutOfBag != NULL)
		nkdTargetModalitiesCountOutOfBag->DeleteAll();
	else
		nkdTargetModalitiesCountOutOfBag = new NumericKeyDictionary;

	position = node->nkdTargetModalitiesCountOutOfBag->GetStartPosition();

	while (position != NULL)
	{
		node->nkdTargetModalitiesCountOutOfBag->GetNextAssoc(position, key, obj);
		DTDecisionTree::TargetModalityCount* source = cast(DTDecisionTree::TargetModalityCount*, obj);
		DTDecisionTree::TargetModalityCount* target = new DTDecisionTree::TargetModalityCount;
		target->CopyFrom(source);
		nkdTargetModalitiesCountOutOfBag->SetAt(target->sModality.GetNumericKey(), target);
	}
}

void DTDecisionTreeNode::CopyWithPruningSons(DTDecisionTreeNode* node)
{
	DTDecisionTreeNode* sonNode;
	int nSonIndex;

	require(node->splitAttributeStats != NULL);

	// Parcours des noeuds fils du noeud initial que l'on copie en l'elaguant
	for (nSonIndex = 0; nSonIndex < node->GetSons()->GetSize(); nSonIndex++)
	{
		sonNode = cast(DTDecisionTreeNode*, node->GetSons()->GetAt(nSonIndex));

		// Tout fils a elaguer doit etre une feuille
		if (not sonNode->IsLeaf())
		{
			AddError("CopyWithPruningSons : son node should be a leaf");
			return;
		}
	}

	// Passage du statut de noeud interne a celui de noeud terminal
	SetLeaf(true);

	// Passage de statut elagable a non elagable
	SetPruningStatus(false);

	cPurity = node->cPurity;
	dNodeCost = node->dNodeCost;
	bCanBecomeInternal = node->bCanBecomeInternal;
	// NV V9 GetNodeClassStats()->SetLearningSpec(node->GetNodeLearningSpec());
	fatherNode = node->fatherNode;
	nDepth = node->nDepth;
	// NV V9 nodeLearningSpec.CopyFrom(node->GetNodeLearningSpec());
	nObjectNumber = node->nObjectNumber;
	svReferenceTargetModalities = node->GetReferenceTargetModalities();

	// copier les objets TargetModalitiesCountTrain
	if (nkdTargetModalitiesCountTrain != NULL)
		nkdTargetModalitiesCountTrain->DeleteAll();
	else
		nkdTargetModalitiesCountTrain = new NumericKeyDictionary;

	POSITION position = node->nkdTargetModalitiesCountTrain->GetStartPosition();
	NUMERIC key;
	Object* obj;

	while (position != NULL)
	{
		node->nkdTargetModalitiesCountTrain->GetNextAssoc(position, key, obj);
		DTDecisionTree::TargetModalityCount* source = cast(DTDecisionTree::TargetModalityCount*, obj);
		DTDecisionTree::TargetModalityCount* target = new DTDecisionTree::TargetModalityCount;
		target->CopyFrom(source);
		nkdTargetModalitiesCountTrain->SetAt(target->sModality.GetNumericKey(), target);
	}

	// copier les objets TargetModalitiesCountOutOfBag
	// if (nkdTargetModalitiesCountOutOfBag != NULL)
	//	nkdTargetModalitiesCountOutOfBag->DeleteAll();
	// else
	//	nkdTargetModalitiesCountOutOfBag = new NumericKeyDictionary;

	// position = node->nkdTargetModalitiesCountOutOfBag->GetStartPosition();

	// while (position != NULL)
	//{
	//	node->nkdTargetModalitiesCountOutOfBag->GetNextAssoc(position, key, obj);
	//	DTDecisionTree::TargetModalityCount* source = cast(DTDecisionTree::TargetModalityCount*, obj);
	//	DTDecisionTree::TargetModalityCount* target = new DTDecisionTree::TargetModalityCount;
	//	target->CopyFrom(source);
	//	nkdTargetModalitiesCountOutOfBag->SetAt(target->sModality.GetNumericKey(), target);
	// }
}

void DTDecisionTreeNode::WriteReport(ostream& ost) const
{
	if (!bIsLeaf)
		splitAttributeStats->GetPreparedDataGridStats()->Write(ost);

	ost << endl;
}

boolean DTDecisionTreeNode::IsReported() const
{
	return GetSortValue() > 0;
}

void DTDecisionTreeNode::WriteHeaderLineReport(ostream& ost, const DTDecisionTree* tree, bool forleave) const
{
	KWDGSAttributeSymbolValues* attributeSymbolValues;
	int nTarget;

	ost << "Identifier"
	    << "\tLevel"
	    << "\tDepth"
	    << "\tMode"
	    << "\tFrequency"
	    << "\tCoverage"
	    << "\tPurity";

	// Extraction de la partition cible
	attributeSymbolValues = cast(KWDGSAttributeSymbolValues*, tree->GetTargetValueStats()->GetAttributeAt(0));

	// Parcours des modalites cible
	for (nTarget = 0; nTarget < attributeSymbolValues->GetPartNumber(); nTarget++)
	{
		// Affichage de la valeur courante
		ost << "\t" << TSV::Export(attributeSymbolValues->GetValueAt(nTarget).GetValue());
	}

	// Cas d'une feuille
	if (forleave)
	{
		POSITION position;
		Object* obj;
		ALString sVariableName;
		ALString sLine;

		position = tree->GetUsedVariablesDictionary()->GetStartPosition();
		// Parcours des variables utilisees dans l'arbre
		while (position != NULL)
		{
			tree->GetUsedVariablesDictionary()->GetNextAssoc(position, sVariableName, obj);
			ost << "\t" << sVariableName;
		}

		ost << sLine;
	}

	// Cas d'un noeud interne
	if (!forleave)
	{
		// Affichage de la variable de split et des identifiants des fils
		ost << "\tSplit Variable "
		    << "\tSons Identifiers ";
	}
	ost << "\n";
}

void DTDecisionTreeNode::Write(ostream& ost) const
{
	ost << "Node " << GetNodeIdentifier() << " : " << endl;
	ost << "\t" << (bIsLeaf ? "leaf " : "internal node") << endl
	    << "\t" << (bCanBePruned ? "prunable " : "not prunable") << endl
	    << "\t" << (bCanBecomeInternal ? "can become internal " : "cannot become internal") << endl
	    << "\t Depth " << nDepth << endl
	    << "\t Node Cost " << dNodeCost << endl
	    << "\t Object Number " << nObjectNumber << endl
	    << "\t Out Of Bag Object Number " << nOutOfBagObjectNumber << endl
	    << "\t Purity " << cPurity << endl
	    << "\t Split Variable Value Number " << nSplitVariableValueNumber << endl
	    << "\t split Attribute " << (splitAttributeStats == NULL ? "none" : splitAttributeStats->GetAttributeName())
	    << endl
	    << "\t Sons Nodes number : " << oaSonNodes.GetSize();

	if (oaSonNodes.GetSize() > 0)
	{
		ost << " (ids : ";
		for (int i = 0; i < oaSonNodes.GetSize(); i++)
		{
			DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, oaSonNodes.GetAt(i));
			ost << node->GetNodeIdentifier();
			if (i != oaSonNodes.GetSize() - 1)
				ost << ",  ";
			else
				ost << ")";
		}
	}

	ost << endl << "\t Node Cost " << dNodeCost << endl << endl;
}

void DTDecisionTreeNode::WriteLineReport(ostream& ost, const DTDecisionTree* tree) const
{
	KWDGSAttributeSymbolValues* attributeSymbolValues;
	int nIndex;
	int nTarget;

	ost << KWContinuous::ContinuousToString(GetSortValue()) << "\t"
	    << KWContinuous::ContinuousToString(1.0 - GetCostValue() / tree->GetRootNode()->GetCostValue()) << "\t"
	    << IntToString(GetDepth())
	    // NV V9		<< "\t" << cast(KWDescriptiveSymbolStats*,
	    // GetNodeClassStats()->GetTargetDescriptiveStats())->GetMode()
	    << "\t" << IntToString(GetObjectNumber()) << "\t"
	    << KWContinuous::ContinuousToString(GetObjectNumber() * 1.0 /
						(1.0 * tree->GetRootNode()->GetObjectNumber()));
	// NV V9		<< "\t" << KWContinuous::ContinuousToString((Continuous)cast(KWDescriptiveSymbolStats*,
	// GetNodeClassStats()->GetTargetDescriptiveStats())->GetModeFrequency() / (Continuous)GetObjectNumber());

	// Extraction de la partition cible

	attributeSymbolValues = cast(KWDGSAttributeSymbolValues*, tree->GetTargetValueStats()->GetAttributeAt(0));

	for (nTarget = 0; nTarget < attributeSymbolValues->GetPartNumber(); nTarget++)
	{
		if (GetObjectNumber() != 0)
		{
			Object* obj = nkdTargetModalitiesCountTrain->Lookup(
			    attributeSymbolValues->GetValueAt(nTarget).GetNumericKey());

			if (obj != NULL)
			{
				DTDecisionTree::TargetModalityCount* tmc =
				    cast(DTDecisionTree::TargetModalityCount*, obj);
				ost << "\t" << tmc->iCount * 1.0 / (1.0 * GetObjectNumber());
			}
			else
				ost << "\t" << 0;
		}
		else
			ost << "\t" << 0;
	}

	// Cas d'une feuille
	if (IsLeaf())
	{
		DTDecisionTreeNode* currentFatherNode;
		const DTDecisionTreeNode* sonNode;
		POSITION position;
		Object* obj;
		ALString sVariableName;
		ALString sModalityName;
		int nPosition;

		position = tree->GetUsedVariablesDictionary()->GetStartPosition();

		// Parcours des variables utilisees
		while (position != NULL)
		{
			ost << "\t";
			tree->GetUsedVariablesDictionary()->GetNextAssoc(position, sVariableName, obj);

			sonNode = this;
			currentFatherNode = fatherNode;

			// Parcours ascendant de l'arbre
			while (currentFatherNode != NULL)
			{
				// Cas ou la variable courante du dico est la variable de partitionnement du noeud pere
				// courant
				if (sVariableName == currentFatherNode->GetSplitAttributeStats()
							 ->GetPreparedDataGridStats()
							 ->GetAttributeAt(0)
							 ->GetAttributeName())
				{
					nPosition = 0;
					// Recherche de la position du noeud courant dans sa fratrie
					while (nPosition < currentFatherNode->oaSonNodes.GetSize() &&
					       cast(DTDecisionTreeNode*, currentFatherNode->oaSonNodes.GetAt(nPosition))
						       ->GetNodeIdentifier() != sonNode->GetNodeIdentifier())
					{
						nPosition++;
					}
					require(nPosition < currentFatherNode->oaSonNodes.GetSize());

					// Extraction de la partie associe a la variable pour ce noeud
					currentFatherNode->GetSplitAttributeStats()
					    ->GetPreparedDataGridStats()
					    ->GetAttributeAt(0)
					    ->WritePartAt(ost, nPosition);
				}
				// On remonte d'un niveau dans l'arbre
				sonNode = currentFatherNode;
				currentFatherNode = currentFatherNode->GetFatherNode();
			}
		}
	}

	// Cas d'un noeud interne
	if (!IsLeaf())
	{
		// Affichage du nom de la variable de split
		ost << "\t" << TSV::Export(GetSplitAttributeStats()->GetAttributeName());

		ost << "\t";
		// Parcours des fils
		for (nIndex = 0; nIndex < GetSons()->GetSize(); nIndex++)
		{
			// Ecriture de l'identifiant
			DTDecisionTreeNode* node = cast(DTDecisionTreeNode*, GetSons()->GetAt(nIndex));
			ost << node->GetNodeIdentifier() << ",";
		}
	}

	ost << "\n";
}

SymbolVector* DTDecisionTreeNode::GetReferenceTargetModalities() const
{
	return svReferenceTargetModalities;
}

void DTDecisionTreeNode::SetReferenceTargetModalities(SymbolVector* targetValues)
{
	svReferenceTargetModalities = targetValues;
}

void DTDecisionTreeNode::SetOutOfBagBaseLoader(const DTBaseLoader* db)
{
	assert(baseloaderOutOfBag == NULL);
	baseloaderOutOfBag = cast(DTBaseLoader*, db);
}

void DTDecisionTreeNode::SetTrainBaseLoader(const DTBaseLoader* db)
{
	assert(baseloaderTrain == NULL);
	baseloaderTrain = cast(DTBaseLoader*, db);
}

boolean DTDecisionTreeNode::CheckNode() const
{
	boolean bOk = true;

	// if (not nodeLearningSpec.Check() )
	//{
	//	bOk = false;
	//	AddError("Incorrect Node learning specification");
	// }

	return bOk;
}

boolean DTDecisionTreeNode::ComputeAttributesStat()
{
	boolean bOk = true;
	require(oaAttributeStats == NULL);

	if (oaAttributeStats != NULL && bAttributeStatsBelongsToNode)
	{
		oaAttributeStats->DeleteAll();
		delete oaAttributeStats;
	}

	bAttributeStatsBelongsToNode = true;
	oaAttributeStats = new ObjectArray;

	// calcul de la preparation du noeud
	KWDataPreparationUnivariateTask dataPreparationUnivariateTask;

	baseloaderTrain->GetTupleLoader()->SetInputDatabaseObjects(baseloaderTrain->GetDatabaseObjects());
	bOk = dataPreparationUnivariateTask.BasicCollectPreparationStats(
	    nodeLearningSpec, baseloaderTrain->GetTupleLoader(), GetNodeSelectedAttributes(), false, oaAttributeStats);

	return bOk;
}

int DTDecisionTreeNodeCompare(const void* elem1, const void* elem2)
{
	DTDecisionTreeNode* i1 = (DTDecisionTreeNode*)*(Object**)elem1;
	DTDecisionTreeNode* i2 = (DTDecisionTreeNode*)*(Object**)elem2;

	ALString sSortValue1 = (const char*)i1->GetNodeIdentifier();
	ALString sSortValue2 = (const char*)i2->GetNodeIdentifier();

	if (i1->GetInstanceNumber() < i2->GetInstanceNumber())
		return 1;
	else if (i1->GetInstanceNumber() > i2->GetInstanceNumber())
		return -1;
	else
		return sSortValue1.Compare(sSortValue2);
}
