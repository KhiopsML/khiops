// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeSpec.h"
#include "KWDRLogical.h"

//////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeSpec

DTDecisionTreeSpec::DTDecisionTreeSpec()
{
	nsRootNode = NULL;
	dLevel = 0;
	dConstructionCost = -1;
	nDepth = 0;
	nLeavesNumber = 0;
	nInternalNodesNumber = 0;
	dTreeLevel = -1;
	nVariablesNumber = 0;
}

DTDecisionTreeSpec::~DTDecisionTreeSpec()
{
	Clean();
}

void DTDecisionTreeSpec::Clean()
{
	nsRootNode = NULL;

	// destruction du tableau des noeuds (y compris le noeud root)
	oaTreeNodes.DeleteAll();
}

DTDecisionTreeNodeSpec* DTDecisionTreeSpec::AddNodeSpec(const DTDecisionTreeNode* nNode,
							DTDecisionTreeNodeSpec* nsFather)
{
	require(nNode != NULL);
	// require(nsRootNode != NULL && nsFather != NULL); // test qu'il est qu'un seul root node
	DTDecisionTreeNodeSpec* nstemp = NULL;
	DTDecisionTreeNodeSpec* nstempson;

	// renseigner le comptage des modalites cibles pour ce noeud, sous forme d'un ObjectArray des modalites triees
	ObjectArray* oaModalitiesCount = new ObjectArray;
	if (nNode->GetTargetModalitiesCountTrain() != NULL)
	{
		POSITION position = nNode->GetTargetModalitiesCountTrain()->GetStartPosition();
		NUMERIC key;
		Object* obj;
		while (position != NULL)
		{
			nNode->GetTargetModalitiesCountTrain()->GetNextAssoc(position, key, obj);
			DTDecisionTree::TargetModalityCount* tmc = cast(DTDecisionTree::TargetModalityCount*, obj);
			oaModalitiesCount->Add(tmc->Clone());
		}
		oaModalitiesCount->SetCompareFunction(DTDecisionTreeNodesModalitiesCountCompare);
		oaModalitiesCount->Sort();
	}

	// ajout d'un noeud interne de l'arbre
	if (nNode != NULL && !nNode->IsLeaf())
	{
		// creation et ajout du noeud dans oaTreeNode
		nstemp = new DTDecisionTreeNodeSpec;
		nstemp->SetFatherNode(nsFather);
		nstemp->InitFromAttributeStats(nNode->GetSplitAttributeStats());
		nstemp->SetTargetModalitiesCountTrain(oaModalitiesCount);

		oaTreeNodes.Add(nstemp);
		nstemp->bIsLeaf = false;
		nstemp->SetNodeIdentifier(nNode->GetNodeIdentifier().GetValue());
		// ajout d'un noeud interne de l'arbre
		if (nsFather == NULL)
		{
			nstemp->ndepth = 0;
		}
		else
		{
			nstemp->ndepth = nsFather->ndepth + 1;
		}

		// appel recurssif a la creation des noeud fils
		for (int i = 0; i < nNode->GetSons()->GetSize(); i++)
		{
			nstempson = AddNodeSpec(cast(DTDecisionTreeNode*, nNode->GetSons()->GetAt(i)), nstemp);
			nstemp->oaChildNode.Add(nstempson);
		}
	}

	/////////////////////////////////////////////////////////////
	// ajout d'un noeud feuille de l'arbre
	if (nNode != NULL && nNode->IsLeaf())
	{
		// creation et ajout du noued dans oaTreeNode
		nstemp = new DTDecisionTreeNodeSpec;
		nstemp->SetFatherNode(nsFather);
		// nstemp->InitFromAttributeStats(nNode->GetSplitAttributeStats());
		nstemp->bIsLeaf = true;
		nstemp->SetNodeIdentifier(nNode->GetNodeIdentifier().GetValue());
		nstemp->SetTargetModalitiesCountTrain(oaModalitiesCount);
		oaTreeNodes.Add(nstemp);
		// ajout d'un noeud interne de l'arbre
		if (nsFather == NULL)
		{
			// specification du noeud root
			nsRootNode = nstemp;
		}
	}

	return nstemp;
}

longint DTDecisionTreeSpec::ComputeHashValue() const
{
	if (nsRootNode == NULL)
		return -1;

	return nsRootNode->ComputeHashValue();
}

bool DTDecisionTreeSpec::InitFromDecisionTree(DTDecisionTree* dtTree)
{
	require(dtTree != NULL);
	Clean();
	dConstructionCost = dtTree->GetConstructionCost();
	nsRootNode = AddNodeSpec(dtTree->GetRootNode(), NULL);
	dTreeLevel = dtTree->GetEvaluation();
	dConstructionCost = dtTree->GetCostClass()->ComputeTreeConstructionCost(dtTree);
	SetVariablesNumber(dtTree->GetUsedVariablesDictionary()->GetCount());
	SetLeavesNumber(dtTree->GetLeaves()->GetCount());
	SetDepth(dtTree->GetTreeDepth());
	nInternalNodesNumber = dtTree->GetInternalNodes()->GetCount();
	return true;
}

KWAttribute* DTDecisionTreeSpec::BuildAttribute(const ALString& svariablename)
{
	const ALString sTreeDataKey = "Tree";
	KWAttribute* attribute = new KWAttribute;
	// creatation de l attribut et de la regle
	KWDerivationRule* switchRule = CreateSwitchRuleC(nsRootNode);

	// TODO MB: la ligne suivante n'est plus valide, et remplacee par le parametrage direct des couts de l'attribut
	// switchRule->SetCost(dConstructionCost + log(1.0 + nNativeVariableNumber));

	attribute->SetName(svariablename);
	attribute->GetMetaData()->SetNoValueAt(sTreeDataKey);

	// Initialisation
	attribute->SetType(KWType::Symbol);
	attribute->SetDerivationRule(switchRule);
	return attribute;
}

KWDerivationRule* DTDecisionTreeSpec::CreateSwitchRuleC(const DTDecisionTreeNodeSpec* node)
{
	// methode destinee a etre appelee recursivement. Pour chaque noeud interne, on cree une regle de derivation
	// Switch, et on y ajoute ses operandes, si necessaire en creant un nouveau switch (dans le cas des noeuds fils
	// qui sont egalement internes)

	assert(node != NULL);

	// Cas d'un arbre reduit a sa racine (modele nul de reference)
	if (node->IsLeaf())
	{
		// Creation d'une regle somme a un seul terme
		KWDRCopySymbol* nodeRule = new KWDRCopySymbol;
		// NV V9 nodeRule->SetClassName(trainedClassifier->GetPredictorClass()->GetName());

		nodeRule->DeleteAllOperands();

		KWDerivationRuleOperand* operand = new KWDerivationRuleOperand;
		nodeRule->AddOperand(operand);

		// Type de l'operande : Continuous et constante
		operand->SetType(KWType::Symbol);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);

		operand->SetSymbolConstant(Symbol(node->GetNodeIdentifier()));

		return nodeRule;
	}

	// Autres cas
	else
	{
		assert(node->apAttributePartitionSpec != NULL);
		assert(node->apAttributePartitionSpec->GetAttributeType() == KWType::Continuous or
		       node->apAttributePartitionSpec->GetAttributeType() == KWType::Symbol);

		KWDerivationRule* switchRule = new KWDRSymbolSwitch;
		// NV V9 switchRule->SetClassName(trainedClassifier->GetPredictorClass()->GetName());
		switchRule->DeleteAllOperands();

		// creation de l'IntervalIndex ou du GroupIndex, selon le type de l'attribut de partition
		KWDerivationRuleOperand* switchOperand1 = new KWDerivationRuleOperand;
		switchOperand1->SetOrigin(KWDerivationRuleOperand::OriginRule);

		if (node->apAttributePartitionSpec->GetAttributeType() == KWType::Continuous)
		{
			KWDerivationRule* intervalIndexRule = CreateIntervalIndexRule(node);
			switchOperand1->SetDerivationRule(intervalIndexRule);
		}
		else
		{
			KWDerivationRule* groupIndexRule = CreateGroupIndexRule(node);
			switchOperand1->SetDerivationRule(groupIndexRule);
		}

		switchRule->AddOperand(switchOperand1);

		KWDerivationRuleOperand* switchOperand2 = new KWDerivationRuleOperand;
		switchOperand2->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		switchOperand2->SetType(KWType::Symbol);
		if (node->GetFatherNode() != NULL)
			switchOperand2->SetSymbolConstant(
			    Symbol(node->GetFatherNode()
				       ->GetNodeIdentifier())); // valeur par defaut : id de noeud du noeud parent
		else
			switchOperand2->SetSymbolConstant((const char*)nsRootNode->GetNodeIdentifier());
		switchRule->AddOperand(switchOperand2);

		// Parcours des noeuds fils
		for (int iNodeIndex = 0; iNodeIndex < node->oaChildNode.GetSize(); iNodeIndex++)
		{
			// Extraction du noeud fils courant
			const DTDecisionTreeNodeSpec* sonNode =
			    cast(const DTDecisionTreeNodeSpec*, node->oaChildNode.GetAt(iNodeIndex));

			// Cas d'un noeud fils feuille
			if (sonNode->IsLeaf())
			{
				KWDerivationRuleOperand* operand = new KWDerivationRuleOperand;
				operand->SetType(KWType::Symbol);
				operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
				operand->SetSymbolConstant(Symbol(sonNode->GetNodeIdentifier()));
				switchRule->AddOperand(operand);
			}

			// Cas d'un noeud fils interne
			else
			{
				KWDerivationRuleOperand* operand = new KWDerivationRuleOperand;
				operand->SetType(KWType::Symbol);
				KWDerivationRule* sonRule = CreateSwitchRuleC(sonNode);
				operand->SetDerivationRule(sonRule);
				operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
				switchRule->AddOperand(operand);
			}
		}

		return switchRule;
	}
}

KWDerivationRule* DTDecisionTreeSpec::CreateIntervalIndexRule(const DTDecisionTreeNodeSpec* node)
{
	// Creation nouvel attribut associe au partitionnement du noeud courant
	// KWDataPreparationAttribute nodePreparationAttribute;

	// NV V9 nodePreparationAttribute.InitFromDataPreparationStats(trainedClassifier->GetPredictorClass(),
	// node->GetSplitAttributeStats());

	if (node->apAttributePartitionSpec == NULL)
		cout << "node->GetSplitAttributeStats() == NULL" << endl;

	// NV V9 const KWDGSAttributePartition* partition =
	// nodePreparationAttribute.GetPreparedStats()->GetPreparedDataGridStats()->SearchAttribute(nodePreparationAttribute.GetNativeAttribute()->GetName());
	assert(node->apAttributePartitionSpec != NULL);

	// creation de l'operande IntervalBounds, par exemple IntervalBounds(5.55),SepalLength)
	KWDerivationRule* intervalBoundsRule = new KWDRIntervalBounds;
	intervalBoundsRule->DeleteAllOperands();

	KWDGSAttributeDiscretization* attributeDiscretization =
	    cast(KWDGSAttributeDiscretization*, node->apAttributePartitionSpec);

	for (int i = 0; i < attributeDiscretization->GetIntervalBoundNumber(); i++)
	{
		KWDerivationRuleOperand* intervalBoundsOperand = new KWDerivationRuleOperand;
		intervalBoundsOperand->SetType(KWType::Continuous);
		intervalBoundsOperand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		intervalBoundsOperand->SetContinuousConstant(attributeDiscretization->GetIntervalBoundAt(i));
		intervalBoundsRule->AddOperand(intervalBoundsOperand);
	}

	// creation de l'operande IntervalIndex
	KWDerivationRule* intervalIndexRule = new KWDRIntervalIndex;
	intervalIndexRule->DeleteAllOperands();

	KWDerivationRuleOperand* intervalIndexOperand1 = new KWDerivationRuleOperand;
	intervalIndexOperand1->SetType(KWType::Structure);
	intervalIndexOperand1->SetOrigin(KWDerivationRuleOperand::OriginRule);
	intervalIndexOperand1->SetDerivationRule(intervalBoundsRule);
	intervalIndexRule->AddOperand(intervalIndexOperand1);

	KWDerivationRuleOperand* intervalIndexOperand2 = new KWDerivationRuleOperand;
	intervalIndexOperand2->SetType(KWType::Continuous);
	intervalIndexOperand2->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	intervalIndexOperand2->SetAttributeName(attributeDiscretization->GetAttributeName());
	intervalIndexRule->AddOperand(intervalIndexOperand2);

	return intervalIndexRule;
}

KWDerivationRule* DTDecisionTreeSpec::CreateGroupIndexRule(const DTDecisionTreeNodeSpec* node)
{
	// creation d'une regle de type  Switch(GroupIndex(
	//												ValueGroups(
	// ValueGroup(“val1”,”val2”),
	// ValueGroup(“val3”)
	//															),
	//												var_categorielle_de_test),
	//												1,2,1)
	//

	// Creation nouvel attribut associe au partitionnement du noeud courant
	// NV V9 KWDataPreparationAttribute nodePreparationAttribute;
	// NV V9 nodePreparationAttribute.InitFromDataPreparationStats(trainedClassifier->GetPredictorClass(),
	// node->GetSplitAttributeStats());

	// NV V9 const KWDGSAttributePartition* partition =
	// nodePreparationAttribute.GetPreparedStats()->GetPreparedDataGridStats()->SearchAttribute(nodePreparationAttribute.GetNativeAttribute()->GetName());
	assert(node->apAttributePartitionSpec != NULL);

	KWDRValueGroups* valueGroupsRule = new KWDRValueGroups;
	valueGroupsRule->DeleteAllOperands();

	for (int nGroup = 0; nGroup < node->apAttributePartitionSpec->GetPartNumber(); nGroup++)
	{
		// Creation d'un operande pour le groupe
		KWDerivationRuleOperand* valueGroupOperand = new KWDerivationRuleOperand;
		valueGroupOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		valueGroupOperand->SetType(KWType::Structure);

		// Creation d'un nouveau groupe
		KWDRValueGroup* valueGroupRule = new KWDRValueGroup;
		valueGroupOperand->SetDerivationRule(valueGroupRule);
		valueGroupOperand->SetStructureName(valueGroupRule->GetStructureName());
		valueGroupOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		valueGroupOperand->SetType(KWType::Structure);

		// Ajout des valeurs du groupe
		valueGroupRule->DeleteAllOperands();

		KWDGSAttributeGrouping* valuesPartition = cast(KWDGSAttributeGrouping*, node->apAttributePartitionSpec);

		const int groupValueNumber = valuesPartition->GetGroupValueNumberAt(nGroup);

		valueGroupRule->SetValueNumber(groupValueNumber);
		for (int nValue = 0; nValue < groupValueNumber; nValue++)
		{
			valueGroupRule->SetValueAt(
			    nValue,
			    valuesPartition->GetValueAt(valuesPartition->GetGroupFirstValueIndexAt(nGroup) + nValue));
		}

		valueGroupsRule->AddOperand(valueGroupOperand);
	}

	KWDerivationRuleOperand* groupIndexOperand1 = new KWDerivationRuleOperand;
	groupIndexOperand1->SetOrigin(KWDerivationRuleOperand::OriginRule);
	groupIndexOperand1->SetType(KWType::Structure);
	groupIndexOperand1->SetDerivationRule(valueGroupsRule);
	groupIndexOperand1->SetStructureName(valueGroupsRule->GetStructureName());

	KWDerivationRuleOperand* groupIndexOperand2 = new KWDerivationRuleOperand;
	groupIndexOperand2->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	groupIndexOperand2->SetType(KWType::Symbol);
	groupIndexOperand2->SetAttributeName(node->GetVariableName());

	KWDRGroupIndex* groupIndexRule = new KWDRGroupIndex;
	groupIndexRule->DeleteAllOperands();
	groupIndexRule->AddOperand(groupIndexOperand1);
	groupIndexRule->AddOperand(groupIndexOperand2);

	return groupIndexRule;
}

void DTDecisionTreeSpec::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const
{
	fJSON->WriteKeyString("name", sTreeVariableName);
	fJSON->WriteKeyInt("variableNumber", nVariablesNumber);
	fJSON->WriteKeyInt("depth", nDepth);
}

const ALString& DTDecisionTreeSpec::GetTreeVariableName() const
{
	return sTreeVariableName;
}

void DTDecisionTreeSpec::SetTreeVariableName(const ALString s)
{
	sTreeVariableName = s;
}

const ALString& DTDecisionTreeSpec::GetRank() const
{
	return sRank;
}

void DTDecisionTreeSpec::SetRank(const ALString s)
{
	sRank = s;
}

void DTDecisionTreeSpec::SetLevel(const double d)
{
	dLevel = d;
}
double DTDecisionTreeSpec::GetLevel() const
{
	return dLevel;
}

void DTDecisionTreeSpec::SetVariablesNumber(const int i)
{
	nVariablesNumber = i;
}
int DTDecisionTreeSpec::GetVariablesNumber() const
{
	return nVariablesNumber;
}

void DTDecisionTreeSpec::SetInternalNodesNumber(const int i)
{
	nInternalNodesNumber = i;
}
int DTDecisionTreeSpec::GetInternalNodesNumber() const
{
	return nInternalNodesNumber;
}

void DTDecisionTreeSpec::SetLeavesNumber(const int i)
{
	nLeavesNumber = i;
}
int DTDecisionTreeSpec::GetLeavesNumber() const
{
	return nLeavesNumber;
}
int DTDecisionTreeSpec::GetDepth() const
{
	return nDepth;
}
void DTDecisionTreeSpec::SetDepth(const int i)
{
	nDepth = i;
}

double DTDecisionTreeSpec::GetConstructionCost() const
{
	return dConstructionCost;
}

//////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeNodeSpec

DTDecisionTreeNodeSpec::DTDecisionTreeNodeSpec()
{
	// adresse du noeud père
	nsFatherNode = NULL;

	// type de la variable continue ou categoriel
	nVariableType = KWType::Unknown;
	// specification de la partition
	apAttributePartitionSpec = NULL;

	bIsLeaf = false;
	ndepth = 0;
	oaTargetModalitiesCountTrain = NULL;
	cJSONMaxValue = 0;
	cJSONMinValue = 0;
}

DTDecisionTreeNodeSpec::~DTDecisionTreeNodeSpec()
{
	// netoyage de apAttributePartitionSpec
	if (apAttributePartitionSpec != NULL)
		delete apAttributePartitionSpec;

	if (oaTargetModalitiesCountTrain != NULL)
	{
		oaTargetModalitiesCountTrain->DeleteAll();
		delete oaTargetModalitiesCountTrain;
	}
}

bool DTDecisionTreeNodeSpec::InitFromAttributeStats(KWAttributeStats* splitAttributeStats)
{
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	require(splitAttributeStats != NULL);
	require(splitAttributeStats->GetAttributeType() == KWType::Continuous or
		splitAttributeStats->GetAttributeType() == KWType::Symbol);
	require(splitAttributeStats->GetPreparedDataGridStats() != NULL);
	const KWDGSAttributePartition* apPartitionSpec;

	if (splitAttributeStats != NULL)
	{
		if (splitAttributeStats->GetAttributeType() == KWType::Continuous or
		    splitAttributeStats->GetAttributeType() == KWType::Symbol)
		{
			nVariableType = splitAttributeStats->GetAttributeType();
			sVariableName = splitAttributeStats->GetAttributeName();
			if (nVariableType == KWType::Continuous)
			{
				// initialisation de apAttributePartitionSpec dans le cas continue
				apAttributePartitionSpec = new KWDGSAttributeDiscretization;
				apPartitionSpec = splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0);
				require(apPartitionSpec != NULL);
				cast(KWDGSAttributeDiscretization*, apAttributePartitionSpec)
				    ->CopyFrom(apPartitionSpec);
				// calcul du min et du max des bornes
				descriptiveContinuousStats =
				    cast(KWDescriptiveContinuousStats*, splitAttributeStats->GetDescriptiveStats());
				cJSONMinValue = descriptiveContinuousStats->GetMin();
				cJSONMaxValue = descriptiveContinuousStats->GetMax();
			}
			else
			{
				// initialisation de apAttributePartitionSpec dasn le cascategoriel

				apAttributePartitionSpec = new KWDGSAttributeGrouping;
				apPartitionSpec = splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0);
				require(apPartitionSpec != NULL);
				cast(KWDGSAttributeGrouping*, apAttributePartitionSpec)->CopyFrom(apPartitionSpec);
			}
			return true;
		}
	}
	return false;
}

void DTDecisionTreeNodeSpec::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	fJSON->WriteKeyString("nodeId", GetNodeIdentifier());
	// fJSON->WriteKeyBoolean("isLeaf", bIsLeaf);
}

const ALString& DTDecisionTreeNodeSpec::GetVariableName() const
{
	return sVariableName;
}

void DTDecisionTreeNodeSpec::SetVariableName(const ALString& sidtemp)
{
	sVariableName = sidtemp;
}
ObjectArray* DTDecisionTreeNodeSpec::GetTargetModalitiesCountTrain() const
{
	return oaTargetModalitiesCountTrain;
}

void DTDecisionTreeNodeSpec::SetTargetModalitiesCountTrain(ObjectArray* n)
{
	oaTargetModalitiesCountTrain = n;
}
///////////////////////////////////////////////  HASH VALUE ///////////////////////////////////////

longint DTDecisionTreeNodeSpec::ComputeHashValue() const
{
	longint lHash = 0;

	// Cas d'un arbre reduit a sa racine (modele nul de reference)
	if (IsLeaf())
	{
		return lHash;
	}

	lHash = HashValue(sVariableName);
	lHash += nVariableType;
	lHash += bIsLeaf;

	if (apAttributePartitionSpec != NULL)
	{
		assert(apAttributePartitionSpec != NULL);
		assert(apAttributePartitionSpec->GetAttributeType() == KWType::Continuous or
		       apAttributePartitionSpec->GetAttributeType() == KWType::Symbol);

		if (apAttributePartitionSpec->GetAttributeType() == KWType::Continuous)
		{
			lHash += ComputeHashOfIntervalIndexRule();
		}
		else
		{
			lHash += ComputeHashOfGroupIndexRule();
		}

		// Parcours des noeuds fils
		for (int iNodeIndex = 0; iNodeIndex < oaChildNode.GetSize(); iNodeIndex++)
		{
			// Extraction du noeud fils courant
			const DTDecisionTreeNodeSpec* sonNode =
			    cast(const DTDecisionTreeNodeSpec*, oaChildNode.GetAt(iNodeIndex));
			lHash += sonNode->ComputeHashValue();
		}
	}

	return lHash;
}

longint DTDecisionTreeNodeSpec::ComputeHashOfIntervalIndexRule() const
{
	// Creation nouvel attribut associe au partitionnement du noeud courant
	// KWDataPreparationAttribute nodePreparationAttribute;
	longint lHash = 0;
	longint ldep = 0;

	// NV V9 nodePreparationAttribute.InitFromDataPreparationStats(trainedClassifier->GetPredictorClass(),
	// node->GetSplitAttributeStats());

	if (apAttributePartitionSpec == NULL)
		cout << "node->GetSplitAttributeStats() == NULL" << endl;

	// NV V9 const KWDGSAttributePartition* partition =
	// nodePreparationAttribute.GetPreparedStats()->GetPreparedDataGridStats()->SearchAttribute(nodePreparationAttribute.GetNativeAttribute()->GetName());
	assert(apAttributePartitionSpec != NULL);

	KWDGSAttributeDiscretization* attributeDiscretization =
	    cast(KWDGSAttributeDiscretization*, apAttributePartitionSpec);

	lHash += attributeDiscretization->GetIntervalBoundNumber() +
		 (attributeDiscretization->GetIntervalBoundNumber() << ndepth);
	for (int i = 0; i < attributeDiscretization->GetIntervalBoundNumber(); i++)
	{
		union
		{
			double d;
			char u[sizeof(double)];
		} tmp;
		tmp.d = attributeDiscretization->GetIntervalBoundAt(i);
		for (i = 0; i < sizeof(lHash); i++)
		{
			ldep = tmp.u[i % sizeof(double)];
			lHash += ldep << i;
			lHash += ldep << (i + ndepth);
		}
	}

	return lHash;
}

longint DTDecisionTreeNodeSpec::ComputeHashOfGroupIndexRule() const
{
	// creation d'une regle de type  Switch(GroupIndex(
	//												ValueGroups(
	// ValueGroup(“val1”,”val2”),
	// ValueGroup(“val3”)
	//															),
	//												var_categorielle_de_test),
	//												1,2,1)
	//

	// Creation nouvel attribut associe au partitionnement du noeud courant
	// NV V9 KWDataPreparationAttribute nodePreparationAttribute;
	// NV V9 nodePreparationAttribute.InitFromDataPreparationStats(trainedClassifier->GetPredictorClass(),
	// node->GetSplitAttributeStats());

	// NV V9 const KWDGSAttributePartition* partition =
	// nodePreparationAttribute.GetPreparedStats()->GetPreparedDataGridStats()->SearchAttribute(nodePreparationAttribute.GetNativeAttribute()->GetName());
	assert(apAttributePartitionSpec != NULL);
	longint lHash = 0;

	for (int nGroup = 0; nGroup < apAttributePartitionSpec->GetPartNumber(); nGroup++)
	{
		// Creation d'un operande pour le groupe

		KWDGSAttributeGrouping* valuesPartition = cast(KWDGSAttributeGrouping*, apAttributePartitionSpec);

		const int groupValueNumber = valuesPartition->GetGroupValueNumberAt(nGroup);
		lHash += groupValueNumber + (groupValueNumber << ndepth);

		for (int nValue = 0; nValue < groupValueNumber; nValue++)
		{
			lHash += HashValue((const char*)(valuesPartition->GetValueAt(
				     valuesPartition->GetGroupFirstValueIndexAt(nGroup) + nValue))) +
				 (HashValue((const char*)(valuesPartition->GetValueAt(
				      valuesPartition->GetGroupFirstValueIndexAt(nGroup) + nValue)))
				  << ndepth);
		}
	}

	return lHash;
}

int DTDecisionTreeNodesModalitiesCountCompare(const void* elem1, const void* elem2)
{
	DTDecisionTree::TargetModalityCount* instance1 = cast(DTDecisionTree::TargetModalityCount*, *(Object**)elem1);
	DTDecisionTree::TargetModalityCount* instance2 = cast(DTDecisionTree::TargetModalityCount*, *(Object**)elem2);
	ALString s1(instance1->sModality);
	ALString s2(instance2->sModality);
	return (s1 < s2 ? -1 : 1);
}
