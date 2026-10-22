// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCHierarchicalDataGrid;
class CCHDGAttribute;
class CCHDGPart;
class CCHDGCell;

#include "KWDataGrid.h"
#include "KWDatabase.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe CCHierarchicalDataGrid
// Sous-classe de KWDataGrid permettant de gerer l'organisation des
// partie des attributs sous forme d'une hierarchie, ainsi que des informations
// complementaires sur les composantes de la grille
class CCHierarchicalDataGrid : public KWDataGrid
{
public:
	// Constructeur
	CCHierarchicalDataGrid();
	~CCHierarchicalDataGrid();

	// Description courte du probleme d'apprentissage
	void SetShortDescription(const ALString& sValue);
	const ALString& GetShortDescription() const;

	// Cout du modele nul
	void SetNullCost(double dValue);
	double GetNullCost() const;

	// Cout du modele de coclustering
	void SetCost(double dValue);
	double GetCost() const;

	// Level du coclustering
	double GetLevel() const;

	// Importance absolue du coclustering
	void SetCoclusteringGlobalImportance(double dValue);
	double GetCoclusteringGlobalImportance() const;

	/////////////////////////////////////////////////////////////////////////
	// Informations sur le parametrage du coclustering

	// Nombre initial d'attributs (en entree du coclustering)
	void SetInitialAttributeNumber(int nValue);
	int GetInitialAttributeNumber() const;

	// Nom de l'attribute d'effectif (optionnel)
	void SetFrequencyAttributeName(const ALString& sValue);
	const ALString& GetFrequencyAttributeName() const;

	// Acces aux specification de la base:
	//   nom du dictionnaire
	//   nom de la base
	//   taux d'echantillonage
	//   mode d'echantillonage
	//   variable de selection
	//   valeur de selection
	// Cette base est ici uniquement pour memoriser ses specification, par pour l'utiliser
	KWDatabase* GetDatabaseSpec();
	const KWDatabase* GetConstDatabaseSpec() const;

	// Nom de l'attribute d'identifiant (optionnel)
	void SetIdentifierAttributeName(const ALString& sValue);
	const ALString& GetIdentifierAttributeName() const;

	/////////////////////////////////////////////////////////////////////////
	// Services divers

	// Nettoyage complet
	void DeleteAll() override;

	// Verification de l'integrite pour les infos supplementaires sur la hierarchie
	boolean CheckHierarchy() const;

	// Verification de l'integrite des importances de la grille
	boolean CheckImportances() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	// Calcul de toutes les importances de la grille
	// Principe du calcul des importances
	// L'importance totale du coclustering est la somme des valeurs absolues des contributions
	// a l'information mutuelle de l'ensemble des cellules du coclustering
	// L'importance d'une partie est la somme des importances de ses cellules divisee par l'importance totale du coclustering
	// L 'importance d' une valeur dans une partie est le ratio de son effectif a celui de sa partie
	// Dans le cas individus * variables :
	// - l'importance d'une partie de variable au sein de son cluster de PV est le ratio de l'effectif de cette partie de variable a celui du cluster de PV
	// - l'importance d'une variable interne est la somme des produits des importances des PV par l'importance du cluster auquel la PV appartient
	// - l'importance d'une partie de variable au sein de sa variable est le ratio du produit de l'importance de la PV au sein de son cluster par l'importance du cluster
	// auquel la PV apparient a l'importance de la variable interne
	// Reference : article "Fully Automated Co-clustering of Mixed Data Using a Maximum A Posteriori Approach", Bouchareb et al,2027

	// Pilotage de toutes les methodes detaillees
	void ComputeAllImportances();

	// Calcul de l'importance totale du coclustering et de la contribution a l'information mutuelle de chaque cellule
	void ComputeCoclusteringImportance();

	// Calcul de l'importance des parties des attributs de la grille
	void ComputePartImportances();

	// Cas d'un coclustering instances x variables
	// Calcul :
	// - des importances des parties de variable au sein de leur cluster de PV
	// - des importances des parties de variable au sein de leur variable
	// - des importances des variables internes
	void ComputeInnerAttributeImportances();

	///////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWDGAttribute* NewAttribute() const override;
	KWDGCell* NewCell() const override;

	// Informations synthetique sur la grille de coclustering
	ALString sShortDescription;
	double dNullCost;
	double dCost;
	int nInitialAttributeNumber;
	ALString sFrequencyAttributeName;
	KWDatabase databaseSpec;
	double dCoclusteringGlobalImportance;

	// CH IV Refactoring: le sIdentifierAttributeName est-il toujours utile???
	// CH IV Refactoring: on le conserve pour l'instant dans l'hypothese ou l'on etende le coclustering IV a plusieurs variables hors variable varPart
	ALString sIdentifierAttributeName;
};

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGAttribute
// Attribut d'un HierarchicalDataGrid
class CCHDGAttribute : public KWDGAttribute
{
public:
	// Constructeur
	CCHDGAttribute();
	~CCHDGAttribute();

	// Valeur minimum d'un attribut, uniquement dans le cas continu
	void SetMin(Continuous cValue);
	Continuous GetMin() const;

	// Valeur maximum d'un attribut, uniquement dans le cas continu
	void SetMax(Continuous cValue);
	Continuous GetMax() const;

	// Nombre initial de parties (avant reduction par clustering hierarchique)
	void SetInitialPartNumber(int nValue);
	int GetInitialPartNumber() const;

	// Interest DEPRECATED
	void SetInterest(double dValue);
	double GetInterest() const;

	// Importance de l'attribut parmi l'ensemble des attributs
	void SetImportance(double dValue);
	double GetImportance() const;

	// Description
	void SetDescription(const ALString& sValue);
	const ALString& GetDescription() const;

	////////////////////////////////////////////////////////////////////////////////
	// Gestion de la hierarchie
	// Les chainages sont a construire explicitement au moyen des methodes suivantes
	// et de celles de CCHDGPart (Parent, Child1, Child2, Root...)
	// Leur validite est controlee par les methodes Check(), et leur destruction
	// est assuree par le destructeur (dans la mesure du possible, sans probleme si
	// la hierarchie est correcte)

	// Creation d'une partie non terminale, a utiliser pour construire la hierarchie des parties
	CCHDGPart* NewHierarchyPart();

	// Gestion de la partie racine de la hierarchie des parties
	void SetRootPart(CCHDGPart* part);
	CCHDGPart* GetRootPart();

	// Fusions des parties filles d'une partie (uniquement si celles ci sont des feuilles de la hierarchie)
	// Impacts:
	//  - les anciennes parties filles sont detruites
	//  - la partie fusionne devient a son tour une feuille de la hierarchie, correctement specifiee
	//    pour son contenus (intervalle ou valeurs) et chainee dans la liste des parties de l'attribut
	//  - les cellules concernees sont mises a jour
	//  - la hierarchie est mise a jour
	CCHDGPart* MergePart(CCHDGPart* part);
	boolean IsPartMergeable(CCHDGPart* part) const;

	// Exports de toutes les parties de la hierarchie en partant de la racine
	void ExportHierarchyParts(ObjectArray* oaParts) const;

	// Tri des parties feuilles par rang croissante
	void SortPartsByRank();

	// Controle d'integrite pour la hierarchie
	boolean CheckHierarchy() const;

	///////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWDGPart* NewPart() const override;

	// Min et max
	Continuous cMin;
	Continuous cMax;

	// Informations sur l'attribut
	int nInitialPartNumber;
	double dInterest;
	double dImportance;
	ALString sDescription;

	// Partie racine
	CCHDGPart* rootPart;
};

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGPart
// Partie d'un HierarchicalDataGrid
class CCHDGPart : public KWDGPart
{
public:
	// Constructeur
	CCHDGPart();
	~CCHDGPart();

	// Nom de partie
	void SetPartName(const ALString& sValue);
	const ALString& GetPartName();

	// Typicalite DEPRECATED
	void SetInterest(double dValue);
	double GetInterest() const;

	// Importance de la partie relativement a son attribut
	// Dans le cas d'une partie de variable d'une variable interne a un coclustering individus * variables
	// il s'agit de l'importance de la partie relativement a son cluster de parties de variable
	void SetImportance(double dValue);
	double GetImportance() const;

	// Dans le cas d'une partie de variable d'une variable interne a un coclustering individus * variables
	// il s'agit de l'importance de la partie relativement a sa variable
	void SetImportanceInVariable(double dValue);
	double GetImportanceInVariable() const;

	// Niveau hierarchique
	void SetHierarchicalLevel(double dValue);
	double GetHierarchicalLevel() const;

	// Rang, dans l'affiche infixe de la hierarchie
	void SetRank(int nValue);
	int GetRank() const;

	// Rang hierarchique
	void SetHierarchicalRank(int nValue);
	int GetHierarchicalRank() const;

	///////////////////////////////////////////////////////////
	// Gestion des annotations de Khiphren

	// Indicateur de partie depliee
	void SetExpand(boolean bValue);
	boolean GetExpand() const;

	// Indicateur de partie selectionnee
	void SetSelected(boolean bValue);
	boolean GetSelected() const;

	// Libelle court
	void SetShortDescription(const ALString& sValue);
	const ALString& GetShortDescription() const;

	// Description longue
	void SetDescription(const ALString& sValue);
	const ALString& GetDescription() const;

	// Libelle utilisateur: description courte si presente, nom de la partie sinon
	const ALString& GetUserLabel() const;

	///////////////////////////////////////////////////////////
	// Gestion de la hierarchie

	// Partie pere (aucune pour la racine)
	void SetParentPart(CCHDGPart* part);
	CCHDGPart* GetParentPart();

	// Parties filles
	void SetChildPart1(CCHDGPart* part);
	CCHDGPart* GetChildPart1();
	void SetChildPart2(CCHDGPart* part);
	CCHDGPart* GetChildPart2();

	// Nom de partie parente (vide si pas de pere)
	const ALString& GetParentPartName();

	// Test si partie racine (sans pere)
	boolean IsRoot() const;

	// Test si partie intermediaire (deux filles)
	boolean IsParent() const;

	// Test si partie terminale (sans fille)
	boolean IsLeaf() const;

	// Controle d'integrite pour la hierarchie
	boolean CheckHierarchy() const;

	///////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode indiquant si les donnees sont emulees
	// pour gerer les effectifs des parties de la hierarchie, n'ayant pas directement de cellules
	boolean GetEmulated() const override;

	// Informations sur la partie
	ALString sPartName;
	double dInterest;
	double dImportance;
	double dImportanceInVariable;
	double dHierarchicalLevel;
	int nRank;
	int nHierachicalRank;
	boolean bExpand;
	boolean bSelected;
	ALString sShortDescription;
	ALString sDescription;

	// Hierarchie
	CCHDGPart* parentPart;
	CCHDGPart* childPart1;
	CCHDGPart* childPart2;
};

// Comparaison de deux parties, en mettant les feuilles en tete, puis le rank
int CCHDGPartCompareLeafRank(const void* elem1, const void* elem2);

// Comparaison de deux parties selon leur rang hierarchique, index d'attribut et partie (pointeur)
int CCHDGPartCompareHierarchicalRank(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGCell
// Cellule d'un HierarchicalDataGrid
class CCHDGCell : public KWDGCell
{
public:
	// Constructeur
	CCHDGCell();
	~CCHDGCell();

	// Information mutuelle
	void SetMutualInformation(double dValue);
	double GetMutualInformation() const;

	///////////////////////////////
	///// Implementation
protected:
	// Information mutuelle de la cellule
	double dMutualInformation;
};
