// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWTupleTableLoader.h"
#include "DTBaseLoaderSplitter.h"
#include "DTBaseLoader.h"

class DTDecisionTree;

// Classe decrivant un noeud quelconque de l'arbre
// Le statut d'un noeud (feuille ou noeud interne) est renseigne par le booleen
// bIsLeaf qui est a vrai s'il s'agit d'une feuille.
// Lors de sa creation, un noeud est considere comme terminal (feuille), il n'a
// donc pas de fils. Il peut :
// - soit etre cree en tant que nouvelle feuille d'un arbre.
// Dans ce cas, il est cree par l'appel de la methode DTDecisionTree::SetUpInternalNode
// - soit etre cree comme feuille hypothetique d'un clone d'un noeud
// de l'arbre. Il est alors cree par l'appel de la methode DTDecisionTreeNode::SetUpAssertionandHypotheticalSons
///
// Son initialisation en tant que noeud feuille consiste a :
///		- indiquer quel est son noeud pere
///		- initialiser son niveau en incrementant celui de son pere
///		- initialiser son KWLearningSpec a partir de son noeud pere
///		  (la KWDatabase du learningSpec est calculee a partir de la KWDatabase
///			et de l'assertion de son noeud pere)
///		- extraire la distribution de la classe cible a partir de la table de contingence
//       de son noeud pere
///		- initialiser sa purete (fraction de la classe cible majoritaire)
// S'il ne s'agit pas d'une feuille hypothetique :
//     - initialiser son identifiant dans l'arbre
//     - initialiser son dtClassStats par appel a ComputeStats()
///
// S'il s'agit du noeud racine :
///		- le noeud pere reste a NULL
///		- le learningSpec est le learningSpec initial du probleme d'apprentissage
///		- la distribution cible est alimentee par l'assertion cible initiale
///		- initialiser sa purete
// Un noeud terminal devient un noeud interne si on lui attribue une assertion
// de partitionnement, une table de contingence ainsi que le nombre de valeurs de
// la variable de partitionnement.
// L'attribution d'une assertion conduit a la creation et a l'initialisation des
// noeuds fils en tant que noeuds terminaux selon le procede decrit ci-dessus
class DTDecisionTreeNode : public KWLearningReport
{
public:
	DTDecisionTreeNode();
	~DTDecisionTreeNode();

	// Acces au noeud pere
	DTDecisionTreeNode* GetFatherNode() const;
	void SetFatherNode(DTDecisionTreeNode* node);

	// Acces aux specifications d'apprentissage
	// Appartient au noeud (il s'agit initialement d'une copie
	// du learningSpec du pere avec ensuite precision sur la
	// base partitionnee)
	KWLearningSpec* GetNodeLearningSpec();
	void SetNodeLearningSpec(KWLearningSpec* learningSpec);

	// Acces la liste des modalites cible
	// Disponible uniquement quand le noeud est interne
	SymbolVector* GetReferenceTargetModalities() const;
	void SetReferenceTargetModalities(SymbolVector*);

	// Teste si le noeud est une feuille
	// true is a leave
	// false is internal node
	boolean IsLeaf() const;

	// Affectation du statut feuille ou noeud interne
	void SetLeaf(boolean bStatus);

	// Teste si le noeud est elaguable
	boolean IsPrunable() const;

	// Affectation du statut elagable ou non
	void SetPruningStatus(boolean bStatus);

	// Acces a la profondeur du noeud (la racine a pour niveau 1)
	// Avant initialisation le niveau est a -1
	int GetDepth() const;
	void SetDepth(int nLevel);

	// Acces a la class du noeud
	// KWClass * GetClass() const;
	// void SetClass(KWClass * kwClass);

	// Acces a l'identifiant du noeud
	const Symbol& GetNodeIdentifier() const;
	void SetNodeIdentifier(const Symbol& s);

	// Acces a la distribution de la classe cible. Dico d'objets DTree::TargetModalityCount (cle = modalite cible,
	// valeur = objet TargetModalityCount)
	NumericKeyDictionary* GetTargetModalitiesCountTrain() const;
	void SetTargetModalitiesCountTrain(NumericKeyDictionary*);

	// Acces a la distribution de la classe cible (out of bag). Dico d'objets DTree::TargetModalityCount (cle =
	// modalite cible, valeur = objet TargetModalityCount)
	NumericKeyDictionary* GetTargetModalitiesCountOutOfBag();
	void SetTargetModalitiesCountOutOfBag(NumericKeyDictionary*);

	// Acces a la purete du noeud
	Continuous GetPurity();
	void SetPurity(Continuous cValue);

	// Acces au nombre d'objets de la sous-base associee au noeud
	int GetObjectNumber() const;
	void SetObjectNumber(int nNumber);

	void SetTrainBaseLoader(const DTBaseLoader*);
	DTBaseLoader* GetTrainBaseLoader() const;

	void SetOutOfBagBaseLoader(const DTBaseLoader*);
	DTBaseLoader* GetOutOfBagBaseLoader() const;

	// Acces au nombre d'objets Out of bag de la sous-base associee au noeud
	int GetOutOfBagObjectNumber() const;
	void SetOutOfBagObjectNumber(int nNumber);

	// Acces aux stats MODL 1D
	ObjectArray* GetNodeAttributeStats();
	void SetNodeAttributeStats(ObjectArray* oaStat);

	// Acces aux caracteristiques de l'attribut de partitionnement du noeud
	KWAttributeStats* GetSplitAttributeStats() const;
	void SetSplitAttributeStats(KWAttributeStats* attributeStats);

	// Acces au cout de la regle du noeud
	double GetCostValue() const;
	void SetCostValue(double dcost);

	// Acces au nombre de valeurs de la variable de partitionnement
	void SetSplitVariableValueNumber(int nValueNumber);
	int GetSplitVariableValueNumber() const;

	// ce noeud peut il devenir interne, compte-tenu des contraintes parametrees via l'IHM ?
	boolean CanBecomeInternal() const;
	void SetCanBecomeInternal(boolean);

	// Acces au tableau des fils
	ObjectArray* GetSons() const;

	// renvoie le noeud racine
	DTDecisionTreeNode* GetRoot() const;

	// Ajout d'un noeud fils
	void AddSonNode(DTDecisionTreeNode* sonNode);

	// index d'un noeud fils
	int GetSonNodeIndex(DTDecisionTreeNode* sonNode);

	// Calcule l'ensemble des feuilles d'un noeud
	ObjectArray* ComputeNodeLeaves();

	// Attribution d'une assertion hypohtetique au noeud
	// et creation des noeuds fils associes
	// Permet l'evaluation du cout des noeuds fils et du noeud
	// devenu hypothetiquement noeud interne
	// Il n'y a pas calcul des stats MODL 1D des noeuds donc pas
	// de partitionnement de la base du noeud pour ses fils
	void SetUpAttributeAndHypotheticalSons(KWAttributeStats* attributeStats, int nValueNumber);

	// Recopie d'un noeud en l'elaguant. Ce noeud ne doit posseder que des fils feuilles
	void CopyWithPruningSons(DTDecisionTreeNode* node);

	// Teste si la regle du noeud est de type continue
	boolean IsContinuous() const;

	// Teste si la regle du noeud est du type sumbolique
	boolean IsSymbol() const;

	// Duplication du noeud
	DTDecisionTreeNode* Clone();

	// Recopie d'un noeud
	void CopyFrom(DTDecisionTreeNode* node);

	/////////////////////////////////////////////////////
	// Rapport sur les resultats
	// Accessible uniquement si statistiques calculees

	// Ecriture d'un rapport sur le noeud
	void WriteReport(ostream& ost);
	boolean IsReported() const;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost, DTDecisionTree* tree, bool forleave);
	void WriteLineReport(ostream& ost, DTDecisionTree* tree);

	// Redefinition des criteres de tri des rapports
	// const ALString GetSortName() const;
	double GetSortValue() const;

	void Write(ostream&) const;

	// Verification de la presence et de la validite des specifications
	boolean CheckNode() const;

	ObjectArray* GetNodeSelectedAttributes();
	void SetNodeSelectedAttributes(ObjectArray* oasel);

	void SetSelectedAttributes(const ObjectArray* liv);

	boolean ComputeAttributesStat();

	////////////////////////////////////////////////////////
	// Implementation
protected:
	// Calcul de la repartition des individus d'un fils par valeur cible
	NumericKeyDictionary* ComputeSonNodeTargetModalitiesCount(int nSonIndex,
								  const NumericKeyDictionary* targetModalitiesCount);

	// Pointeur vers le noeud pere
	// NULL si c'est le noeud racine (Root)
	// N'appartient pas au noeud
	DTDecisionTreeNode* fatherNode;

	// Specifications d'apprentissage pour ce noeud
	KWLearningSpec* nodeLearningSpec;

	// list des variables d'un noeud
	ObjectArray oaInputAttributes;
	ObjectArray oaSelectedAttributes;

	// Valeurs cible de reference
	SymbolVector* svReferenceTargetModalities;

	// le noeud est-il une feuille ? 
	boolean bIsLeaf;

	// le noeud peut-il etre elague ? 
	boolean bCanBePruned;

	// Profondeur du noeud
	int nDepth;

	// Cout du noeud
	double dNodeCost;

	// Identifiant du noeud
	Symbol sIdentifier;

	// Distribution de la variable cible pour ce noeud (database train). Dico d'objets TargetModalitiesCount (cle =
	// modalite cible, valeur = objet TargetModalityCount)
	NumericKeyDictionary* nkdTargetModalitiesCountTrain;

	// Distribution de la variable cible pour ce noeud (database out of bag). Dico d'objets TargetModalitiesCount
	// (cle = modalite cible, valeur = objet TargetModalityCount)
	NumericKeyDictionary* nkdTargetModalitiesCountOutOfBag;

	// Nombre d'individus de la base (eventuellement presents plusieurs fois, si tirage avec remise) se rapportant
	// a ce noeud.
	int nObjectNumber;

	// Nombre d'individus oob de la base se rapportant a ce noeud
	int nOutOfBagObjectNumber;

	DTBaseLoader* baseloaderTrain;
	DTBaseLoader* baseloaderOutOfBag;

	// Purete du noeud
	Continuous cPurity;

	// classe d'analyse de kwobject
	// KWClass* kwcClass;

	// Stats MODL 1D des attributs pour la KWDatabase restreinte
	// au noeud
	ObjectArray* oaAttributeStats;
	boolean bAttributeStatsBelongsToNode;
	// Pointeur sur l'attribut de partitionnement du noeud
	// NULL si le noeud est une feuille
	KWAttributeStats* splitAttributeStats;

	// Nombre de valeurs de la variable de partitionnement
	// Dans le cas d'une variable continue, cette variable est le nombre
	// d'individus de la base du nodeLearningSpec
	// Dans le cas d'une variable symbolique c'est le nombre initial de
	// modalites de la variable
	int nSplitVariableValueNumber;

	// Tableau de pointeurs vers les fils de l'arbre
	// N'appartiennent pas au noeud
	ObjectArray oaSonNodes;

	boolean bCanBecomeInternal;
};

int DTDecisionTreeNodeCompare(const void* elem1, const void* elem2);

inline boolean DTDecisionTreeNode::CanBecomeInternal() const
{
	return bCanBecomeInternal;
}
inline void DTDecisionTreeNode::SetCanBecomeInternal(boolean b)
{
	bCanBecomeInternal = b;
}

inline DTBaseLoader* DTDecisionTreeNode::GetOutOfBagBaseLoader() const
{
	assert(baseloaderOutOfBag != NULL);
	return baseloaderOutOfBag;
}

inline DTBaseLoader* DTDecisionTreeNode::GetTrainBaseLoader() const
{
	assert(baseloaderTrain != NULL);
	return baseloaderTrain;
}

inline void DTDecisionTreeNode::SetNodeSelectedAttributes(ObjectArray* oasel)
{
	// require(CheckSelectedAttributes());
	require(oasel != NULL);

	oaSelectedAttributes.CopyFrom(oasel);
}

inline ObjectArray* DTDecisionTreeNode::GetNodeSelectedAttributes()
{
	// require(CheckSelectedAttributes());
	return &oaSelectedAttributes;
}

inline void DTDecisionTreeNode::SetSelectedAttributes(const ObjectArray* liv)
{
	if (liv == NULL)
		return;

	oaSelectedAttributes.CopyFrom(liv);

	// require(CheckSelectedAttributes());
}

inline KWLearningSpec* DTDecisionTreeNode::GetNodeLearningSpec()
{
	return nodeLearningSpec;
}

inline void DTDecisionTreeNode::SetNodeLearningSpec(KWLearningSpec* lplearningSpec)
{
	require(lplearningSpec != NULL);

	if (nodeLearningSpec != NULL)
	{
		delete nodeLearningSpec;
	}
	nodeLearningSpec = lplearningSpec->Clone();
}

// KWClass * DTDecisionTreeNode::GetClass() const
//{
//	return kwcClass;
// }
//
// void DTDecisionTreeNode::SetClass(KWClass * kwClass)
//{
//	kwcClass = kwClass;
// }
