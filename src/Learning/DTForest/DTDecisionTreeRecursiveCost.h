// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "DTDecisionTreeCost.h"

class DTDecisionTree;
///////////////////////////////////////////////////////////////
/// Definit les methodes permettant de calculer le cout global d'un arbre recursif
/// Ce cout est decompose en :
/// - un cout par defaut qui correspond au choix du nombre de variables utilisees
/// dans l'arbre
/// - un cout par noeud interne
/// - un cout par feuille
/// Pour l'instant le cout total est obtenu par parcours en largeur de l'arbre
class DTDecisionTreeRecursiveCost : public DTDecisionTreeCost // Object
{
public:
	/// Constructeur
	DTDecisionTreeRecursiveCost();
	/// Destructeur
	~DTDecisionTreeRecursiveCost();

	///// Acces au nombre total d'attributs
	// int GetTotalAttributeNumber() const;
	// void SetTotalAttributeNumber(int nNumber);

	///// Acces au nombre de valeurs de la classe cible
	// int GetClassValueNumber() const;
	// void SetClassValueNumber(int nNumber);

	/// Cout total de l'arbre : parcours de l'arbre en largeur
	///   cout fixe
	/// + cout du choix du nombre de variables utilisees
	///	+ somme des couts des noeuds internes
	/// + somme des couts des feuilles
	double ComputeTotalTreeCost(DTDecisionTree* tree) override;

	/// Cout du choix du nombre de variables utilisees dans l'arbre
	double ComputeAttributeChoiceCost(int nUsedAttributeNumber, int nInternalNodeNumber);

	/// Cout d'un noeud interne
	double ComputeInternalNodeCost(DTDecisionTreeNode* node);

	/// Cout de construction de l'arbre : a reimplementer
	virtual double ComputeTreeConstructionCost(DTDecisionTree*) override;

	/// Cout d'un noeud feuille
	double ComputeLeafCost(DTDecisionTreeNode* node);

	/// Cout d'un arbre de cout dPreviousCost pour la coupure selectionnee
	void ComputeHypotheticalAugmentedTreeCost(DTDecisionTree* tree, double dPreviousCost,
						  DTDecisionTreeNodeSplit* nodeSplit) override;

	/// Cout d'un arbre de cout dPreviousCost pour lequel on transformererait le noeud interne
	/// d'indice nInternalNodeIndex en une feuille
	/// en elaguant de l'arbre tous ses fils qui doivent tous etre des feuilles
	double ComputeHypotheticalPrunedTreeCost(DTDecisionTree* tree, double dPreviousCost,
						 ALString sInternalNodeKey) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	///// Nombre de classes cible
	// int nClassValueNumber;

	///// Nombre total de predicteurs
	// int nTotalAttributeNumber;

	/// Booleen de prise en cout du cout feuille ou non
	boolean bLeafCost;
};
