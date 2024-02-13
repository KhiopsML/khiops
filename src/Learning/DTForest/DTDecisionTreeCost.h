// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "DTDecisionTree.h"
#include "DTDecisionTreeNode.h"

class DTDecisionTree;
class DTDecisionTreeNodeSplit;

///////////////////////////////////////////////////////////////
/// Classe abstraite generique de cout d'un arbre
/// La methode du calcul du cout total de l'arbre doit etre reimplementee
/// ainsi que la methode de mise a jour du cout lors de l'ajout de fils
/// a un noeud
class DTDecisionTreeCost : public Object
{
public:
	/// Constructeur
	DTDecisionTreeCost();
	/// Destructeur
	~DTDecisionTreeCost();

	/// Acces au nombre total d'attributs
	int GetTotalAttributeNumber() const;
	void SetTotalAttributeNumber(int nNumber);

	/// Acces au nombre de valeurs de la classe cible
	int GetClassValueNumber() const;
	void SetClassValueNumber(int nNumber);

	int GetDTCriterionMode() const
	{
		return nDTCriterion;
	};
	void SetDTCriterionMode(int nNumber)
	{
		nDTCriterion = nNumber;
	};

	/// Cout des feuilles et noeud interne
	double ComputeNodeCost(DTDecisionTreeNode*, DTDecisionTree*);

	/// Cout de construction de l'arbre : a reimplementer
	virtual double ComputeTreeConstructionCost(DTDecisionTree*);

	/// Cout total de l'arbre : a reimplementer dans les classes derivees
	virtual double ComputeTotalTreeCost(DTDecisionTree*);

	/// Cout d'un arbre de cout dPreviousCost pour lequel on transformerait
	/// la feuille  d'indice nLeafIndex en un noeud interne
	/// selon l'assertion et la table de contingence passees en parametre
	/// A reimplementer
	virtual void ComputeHypotheticalAugmentedTreeCost(DTDecisionTree*, double dPreviousCost,
							  DTDecisionTreeNodeSplit*) = 0;

	/// Cout d'un arbre de cout dPreviousCost pour lequel on transformererait le noeud interne
	/// de cle sInternalNodeKey en une feuille
	/// en elaguant de l'arbre tous ses fils qui doivent tous etre des feuilles
	virtual double ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
							 ALString sInternalNodeKey);

	void CleanRootStat();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	/// Nombre de classes cible
	int nClassValueNumber;

	/// Nombre total de predicteurs
	int nTotalAttributeNumber;

	/// Statistiques sur le noeud racine
	ObjectDictionary odRootStat;

	/// Identifiant du critere de l'arbre pour le calcul du cout de sa structure
	/// 0
	/// 1
	/// 2
	/// 3
	/// 4
	/// 5
	/// 6
	int nDTCriterion;
};
