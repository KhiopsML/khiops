// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataPreparationClass.h"
#include "DTAttributeSelection.h"
#include "DTGlobalTag.h"
#include "DTConfig.h"

class DTAttributeSelectionsSlices;
class PLShared_AttributeSelectionsSlices;

/////////////////////////////////////////////////////////////////////
// Classe DTForestAttributeSelection
class DTForestAttributeSelection : public Object
{
public:
	// Constructeur
	DTForestAttributeSelection();
	~DTForestAttributeSelection();

	// entree : tableau de KWAttributeStats
	// Ensemble des attributs pour faire les selections

	virtual void Initialization(const ObjectDictionary* odInputAttributeStats);

	void BuildForestSelections(int nMaxSelectionNumber, int nvariableNumberMin);

	// selection de variable des arbres selon leur type et le pct de varible a selectionner
	//  type 0 : Uniforme et racine
	//  type 1 : Uniforme et DTGlobalTag::RF_OPTIMIZATION_RANDOM_VARIABLES
	//  type 2 : level et racine
	//  type 3 : node tous

	void BuildForestUniformSelections(int nmaxselectionnumber, const ALString& sSelectionType, double dPct);

	int GetMaxAttributesNumber();

	// virtual ObjectArray* NextSelection();

	const ALString& GetDrawingType() const;
	void SetDrawingType(const ALString& sdrawingtype);
	// Test l'informativite des variables, et set unload, et unused les attributs non informatif de la BDD
	// void UnSelectNonInformativeAttributes();

	// Charge la bdd en memoire , avec le maximum d'attributs possible
	// int	ComputeMaxLoadableAttributesNumber (const PredictorType &);
	int GetUsableAttributesNumber(int npos);
	// Retourne un vecteur de taille nloadedAttribute, contenant les attributs ordonnes de maniere aleatoire
	// ObjectArray* CreateAttributesShuffled(const int nloadedAttributes, bool renew);

	// tirage des variables selon leur valeur de level, en fonction des levels du classStats de l'instance
	// ObjectArray* GetAttributesFromLevels(const int maxAttributesNumber);

	// ObjectArray* GetAttributesShuffled(const int nMaxAttributesNumber, bool renew);

	// envovie la liste des varariables selectionnees dans toutes les listes
	StringVector* GetAllSelectedAttributes();
	int GetSelectionNumber();
	StringVector* GetSelectedAttributesAt(int npos);
	ObjectArray* GetAttributeSelections();

	void WriteReport(ostream& ost);

	///////////////////////////////////////////////////////
	// Implementation
protected:
	void Clean();
	void CleanAll();

	// tirage des variables selon leur valeur de level, en fonction des levels passes en parametre
	static ObjectArray* GetAttributesFromLevels(const int nMaxAttributesNumber, DoubleVector& vLevels,
						    ObjectArray& oaListAttributes);

	int nMaxSelectionNumber;
	int nOriginalAttributesNumber;
	ALString sDrawingType;

	// Tableaux des nom de variabe et level
	StringVector svAttributeNames;
	ObjectArray oaOriginalAttributesUsed;
	ObjectDictionary odOriginalAttributesUsed;

	// selections generees
	IntVector ivSelectionAttributeNumber;
	IntVector ivSelectionAttributeNumberInf;
	IntVector ivSelectionAttributeNumberNull;
	IntVector ivSeedselection;
	ObjectArray oaSelectionAttributes; // tableau de pointeurs sur objets DTAttributeSelection
};

////////////////////////////////////////////////////////////////////
// Classe DTAttributeSelectionsSlices
// Specification d'un ensemble de selections d'attributs, de leurs attributs, et des
// slices de KWDataTableSliceSet les contenant
// Memoire: le contenu des specifications appartient a l'appelant
class DTAttributeSelectionsSlices : public Object
{
public:
	// Constructeur
	DTAttributeSelectionsSlices();
	~DTAttributeSelectionsSlices();

	////////////////////////////////////////////////
	// Parametrage du contenu

	// Parametrage des selections d'attributs (tableau de pointeurs sur objets DTAttributeSelection)
	ObjectArray* GetAttributeSelections();

	// Parametrage des attributs des selections (tableau de pointeurs sur DTTreeAttribute)
	ObjectArray* GetTreeAttributes();

	// Parametrage de slices contenant les attributs (KWDataTableSlice)
	NumericKeyDictionary* GetSlices();

	//////////////////////////////////////////////////////////////
	// Methodes avancees

	// Ajout du contenu d'un autre ensemble de selections
	void AddAttributeSelectionsSlices(const DTAttributeSelectionsSlices* otherAttributePairsSlices);
	void AddAttributeSelection(const DTAttributeSelection* otherAttributeselection,
				   const ObjectDictionary* odSliceAttributes);
	int UnionAttributesCount(const DTAttributeSelection* otherAttributeselection);

	// Comparaison selon les slices utilisees
	int CompareSlices(const DTAttributeSelectionsSlices* otherAttributePairsSlices) const;

	////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilisable depuis d'autres classes pour personnaliser des
	// criteres de tri lexicographique

	// Vecteur de critere pour un tri lexocographique
	// La gestion de la taille et des valeur de ce vecteur est entierement a la cahrge de l'appelant
	DoubleVector* GetLexicographicSortCriterion();

	// Methode de comparaison de deux tranches selon leur critere lexicographique
	int CompareLexicographicSortCriterion(const DTAttributeSelectionsSlices* otherAttributePairsSlices) const;

	// Affichage des valeurs de tri d'une tranche sous forme d'une ligne
	void DisplayLexicographicSortCriterionHeaderLineReport(ostream& ost, const ALString& sSortCriterion) const;
	void DisplayLexicographicSortCriterionLineReport(ostream& ost) const;

	//////////////////////////////////////////////////////////////
	// Methodes standard

	// Verification de l'integrite
	// Les selections, attribut et slices doivent etre toutes distinctes
	// Les attributs doivent etre dans l'ordre de leurs nom
	// Les tranches doivent etre dans l'ordre de leur nom de classe
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	friend PLShared_AttributeSelectionsSlices;

	// Methode utilitaire de fusion du contenu de deux tableaux, suppose tries de la meme facon, en
	// produisant un tableau resultat trie avec elimination des doublons
	// On suppose que chaque tableau initial est trie, sans doublons
	void MergeArrayContent(const ObjectArray* oaFirst, const ObjectArray* oaSecond, CompareFunction fCompare,
			       ObjectArray* oaMergedResult) const;
	void AddNumericDictionaryContent(NumericKeyDictionary* oaFirst, const NumericKeyDictionary* oaSecond);

	// Attributs de la classe

	// tableau de pointeurs DTAttributeSelection*
	ObjectArray oaAttributeSelections;

	ObjectArray oaTreeAttributes;

	// ObjectArray oaSlices;
	NumericKeyDictionary nkdSlices;

	// Critere de tri lexicographique
	DoubleVector dvLexicographicSortCriterion;
};

// Methode de comparaison sur les tranches (cf DTAttributeSelectionsSlices::CompareSlices)
int DTAttributeSelectionsSlicesCompareSlices(const void* elem1, const void* elem2);

// Methode de comparaison basee sur le critere de tri lexicographique
int DTAttributeSelectionsSlicesCompareLexicographicSortCriterion(const void* elem1, const void* elem2);
