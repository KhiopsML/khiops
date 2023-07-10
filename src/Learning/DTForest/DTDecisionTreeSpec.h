// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "KWDataGridStats.h"
#include "DTDecisionTree.h"

class DTDecisionTreeSpec;
class DTDecisionTreeNodeSpec;
class PLShared_DecisionTreeNodeSpec;

int DTDecisionTreeNodesModalitiesCountCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////////
// Specification d'un modele arbre
// DTDecisionTreeSpec:
//  spec minimal de definition  d'un arbre (nom de variable, spec de partitionnement, neouds fils...)
//  service de creation d'une variable dans un dictionnaire

class DTDecisionTreeSpec : public Object
{
public:
	// Constructeur et destructeur
	DTDecisionTreeSpec();
	~DTDecisionTreeSpec();

	// inisialisation de dtTree
	bool InitFromDecisionTree(DTDecisionTree* dtTree);

	// creation de l'attribut a partir de la specification
	KWAttribute* BuildAttribute(const ALString& svariablename);

	/// Acces au cout de construction de l'arbre
	void SetConstructionCost(double);
	double GetConstructionCost() const;

	// Cle de hashage de la regle et des ses operandes
	longint ComputeHashValue() const;

	void SetRank(const ALString);
	const ALString& GetRank() const;

	void SetTreeVariableName(const ALString);
	const ALString& GetTreeVariableName() const;

	void SetLevel(const double);
	double GetLevel() const;

	void SetVariablesNumber(const int);
	int GetVariablesNumber() const;

	void SetInternalNodesNumber(const int);
	int GetInternalNodesNumber() const;

	void SetLeavesNumber(const int);
	int GetLeavesNumber() const;

	void SetDepth(const int);
	int GetDepth() const;

	const ObjectArray& GetTreeNodes() const;

	// ecriture du rapport json
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const;

protected:
	friend class PLShared_DecisionTreeSpec;

	// fonction pour ajouter un nodespec a la structure d'arbre a partir d'un DTDecisionTreeNode
	DTDecisionTreeNodeSpec* AddNodeSpec(const DTDecisionTreeNode* nNode, DTDecisionTreeNodeSpec* nsFather);

	KWDerivationRule* CreateSwitchRuleC(const DTDecisionTreeNodeSpec* node);
	KWDerivationRule* CreateGroupIndexRule(const DTDecisionTreeNodeSpec* node);
	KWDerivationRule* CreateIntervalIndexRule(const DTDecisionTreeNodeSpec* node);

	// clean structure
	void Clean();

	ALString sRank;
	ALString sTreeVariableName;
	double dTreeLevel;
	double dLevel;
	int nVariablesNumber;
	int nInternalNodesNumber;
	int nLeavesNumber;
	int nDepth;
	double dConstructionCost;

	// adresse du noeud racine
	DTDecisionTreeNodeSpec* nsRootNode;
	// tableau de l'ensemble des DTDecisionTreeNodeSpec
	ObjectArray oaTreeNodes;
};

inline const ObjectArray& DTDecisionTreeSpec::GetTreeNodes() const
{
	return oaTreeNodes;
}

////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DecisionTreeSpec
// Serialisation de la classe DTDecisionTreeSpec
class PLShared_DecisionTreeSpec : public PLSharedObject
{
public:
	// Constructor
	PLShared_DecisionTreeSpec();
	~PLShared_DecisionTreeSpec();

	void SetDecisionTreeSpec(DTDecisionTreeSpec*);
	DTDecisionTreeSpec* GetDecisionTreeSpec() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	void AddNode(ObjectArray& oaTreeNodes, DTDecisionTreeNodeSpec*) const;

	// noeud racine
	PLShared_DecisionTreeNodeSpec* shared_nsRootNode;

	// liste d'objets PLShared_DecisionTreeNodeSpec
	// PLShared_ObjectArray* shared_oaTreeNodes;

	Object* Create() const override;
};

///////////////////////////////////////////////////////////////////////////
// Specification d'une coupure d'un noued interne de l'arbre
// DTDecisionTreeNodeSpec:
//  spec minimal de definition  d'une coupure pour un noeud (nom de variable, spec de partitionnement, noeuds fils...)

class DTDecisionTreeNodeSpec : public Object
{
public:
	// Constructeur et destructeur
	DTDecisionTreeNodeSpec();
	~DTDecisionTreeNodeSpec();

	bool InitFromAttributeStats(KWAttributeStats* splitAttributeStats);

	// initlisation du pere
	void SetFatherNode(DTDecisionTreeNodeSpec* nsValue);
	DTDecisionTreeNodeSpec* GetFatherNode() const;

	// initlisation des fils
	// renvoie le nombre de fils du noued courant si noued terminal alors renvoie zero
	int GetChildrenNumber() const;

	// renvoie la valeur min et max des intervalles pour ecrire le JSON
	Continuous GetJSONMin() const;
	Continuous GetJSONMax() const;

	// ajoute un fils au noeud pere.
	void AddNode(DTDecisionTreeNodeSpec* nsValue);

	// acces aux fils
	const ObjectArray& GetChildNodes() const;

	// isLeaf
	boolean IsLeaf() const;

	KWDGSAttributePartition* GetAttributePartitionSpec() const;

	// Get sET ID identification du noeud c'est un int
	const ALString& GetNodeIdentifier() const;
	void SetNodeIdentifier(const ALString& stemps);

	const ALString& GetVariableName() const;
	void SetVariableName(const ALString& stemps);

	/// Acces a la distribution de la classe cible. Tableau d'objets TargetModalityCount, tri par valeur cible
	/// alphabetique croissante
	ObjectArray* GetTargetModalitiesCountTrain() const;
	void SetTargetModalitiesCountTrain(ObjectArray*);

	// Cle de hashage de la regle et des ses operandes
	longint ComputeHashValue() const;

	// ecriture du rapport json
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary);

protected:
	friend class DTDecisionTreeSpec;
	friend PLShared_DecisionTreeNodeSpec;

	longint ComputeHashOfGroupIndexRule() const;
	longint ComputeHashOfIntervalIndexRule() const;

	// adresse du noeud pere
	DTDecisionTreeNodeSpec* nsFatherNode;
	// adresse des noeud fils
	ObjectArray oaChildNode;
	// nom de la variable
	ALString sVariableName;
	// type de la variable continue ou categoriel
	int nVariableType;
	// is leaf
	boolean bIsLeaf;
	// depeth
	int ndepth;
	// ID node
	ALString sIdentifier;
	// specification de la partition
	KWDGSAttributePartition* apAttributePartitionSpec;

	// Vecteur des valeurs min et max par intervalle, pour l'ecriture des discretisation JSON
	Continuous cJSONMinValue;
	Continuous cJSONMaxValue;

	/// Distribution de la variable cible pour ce noeud (database train). Tableau d'objets TargetModalitiesCount,
	/// tri par valeur cible alphabetique croissante
	ObjectArray* oaTargetModalitiesCountTrain;
};

inline KWDGSAttributePartition* DTDecisionTreeNodeSpec::GetAttributePartitionSpec() const
{
	return apAttributePartitionSpec;
}

inline void DTDecisionTreeNodeSpec::SetFatherNode(DTDecisionTreeNodeSpec* nsValue)
{
	nsFatherNode = nsValue;
}

inline DTDecisionTreeNodeSpec* DTDecisionTreeNodeSpec::GetFatherNode() const
{
	return nsFatherNode;
}

// renvoie le nombre de fils du noued courant si noued terminal alors renvoie zero
inline int DTDecisionTreeNodeSpec::GetChildrenNumber() const
{
	return oaChildNode.GetSize();
}

// ajoute un fils au noeud pere.
inline void DTDecisionTreeNodeSpec::AddNode(DTDecisionTreeNodeSpec* nsValue)
{
	oaChildNode.Add(nsValue);
}

inline boolean DTDecisionTreeNodeSpec::IsLeaf() const
{
	return bIsLeaf;
}

inline Continuous DTDecisionTreeNodeSpec::GetJSONMin() const
{
	return cJSONMinValue;
}

inline Continuous DTDecisionTreeNodeSpec::GetJSONMax() const
{
	return cJSONMaxValue;
}

inline const ALString& DTDecisionTreeNodeSpec::GetNodeIdentifier() const
{
	return sIdentifier;
}

inline void DTDecisionTreeNodeSpec::SetNodeIdentifier(const ALString& sidtemp)
{
	sIdentifier = sidtemp;
}

inline const ObjectArray& DTDecisionTreeNodeSpec::GetChildNodes() const
{
	return oaChildNode;
}

inline void DTDecisionTreeSpec::SetConstructionCost(double d)
{
	dConstructionCost = d;
}

////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DecisionTreeNodeSpec
// Serialisation de la classe DTDecisionTreeNodeSpec
class PLShared_DecisionTreeNodeSpec : public PLSharedObject
{
public:
	// Constructor
	PLShared_DecisionTreeNodeSpec();
	~PLShared_DecisionTreeNodeSpec();

	void SetDecisionTreeNodeSpec(DTDecisionTreeNodeSpec*);
	DTDecisionTreeNodeSpec* GetDecisionTreeNodeSpec() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	Object* Create() const override;

	mutable PLShared_ObjectArray* shared_oaChildrenNode;
	PLShared_ObjectArray* shared_oaTargetModalitiesCountTrain;
	PLShared_DGSAttributeDiscretization* shared_dgsAttributeDiscretization;
	PLShared_DGSAttributeGrouping* shared_dgsAttributeGrouping;
};
