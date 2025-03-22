// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCHierarchicalDataGrid;
class CCHDGAttribute;
class CCHDGPart;

#include "KWDataGrid.h"
#include "KWDatabase.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe CCHierarchicalDataGrid
// Sous-classe de KWDataGrid permettant de gerer la l'organisation des
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

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	///////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWDGAttribute* NewAttribute() const override;

	// Informations synthetique sur la grille de coclustering
	ALString sShortDescription;
	double dNullCost;
	double dCost;
	int nInitialAttributeNumber;
	ALString sFrequencyAttributeName;
	KWDatabase databaseSpec;

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

	// Interest
	void SetInterest(double dValue);
	double GetInterest() const;

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

	// Typicalite
	void SetInterest(double dValue);
	double GetInterest() const;

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
