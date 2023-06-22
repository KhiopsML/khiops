// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Vector.h"
#include "KWSortableIndex.h"
#include "KWDataGridStats.h"

///////////////////////////////////////////////////////////////////////////////////
// Definition generique d'un vecteur d'effectifs
// Il s'agit d'un ensemble de compteurs associes a des classes distinctes
// Il s'agit d'une classe virtuelle, permettant l'implementation d'algorithme generique
// travaillant sur des sous-classes specialisees (vecteur plein ou creux par exemple)
class KWFrequencyVector : public Object
{
public:
	// Constructeur
	KWFrequencyVector();
	~KWFrequencyVector();

	// Createur, renvoyant une instance du meme type
	virtual KWFrequencyVector* Create() const;

	// Taille du vecteur d'effectif (par defaut: 0)
	virtual int GetSize() const;

	// Acces au nombre de modalites
	int GetModalityNumber() const;
	void SetModalityNumber(int nModality);

	// Position dans une liste triee par nombre de modalites
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Copie a partir d'un vecteur source
	virtual void CopyFrom(const KWFrequencyVector* kwfvSource);

	// Duplication (y compris du contenu)
	virtual KWFrequencyVector* Clone() const;

	// Calcul de l'effectif total
	virtual int ComputeTotalFrequency() const;

	// Rapport synthetique destine a rentrer dans une sous partie d'un tableau (par de retour a la ligne)
	virtual void WriteHeaderLineReport(ostream& ost) const;
	virtual void WriteLineReport(ostream& ost) const;

	///////////////////////////////
	///// Implementation
protected:
	int nModalityNumber;
	POSITION position;
};

// Comparaison des nombres de modalites des lignes de contingence
int KWFrequencyVectorModalityNumberCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////////////////
// Vecteur d'effectifs plein
// Similaire a un compteur d'entiers, avec gestion de l'effectif total
class KWDenseFrequencyVector : public KWFrequencyVector
{
public:
	// Constructeur
	KWDenseFrequencyVector();
	~KWDenseFrequencyVector();

	// Createur, renvoyant une instance du meme type
	KWFrequencyVector* Create() const override;

	// Vecteur d'effectif par classe cible
	IntVector* GetFrequencyVector();

	// Taille du vecteur d'effectif
	int GetSize() const override;

	// Copie a partir d'un vecteur source
	void CopyFrom(const KWFrequencyVector* kwfvSource) override;

	// Duplication (y compris du contenu)
	KWFrequencyVector* Clone() const override;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const override;

	// Rapport synthetique destine a rentrer dans une sous partie d'un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Vecteur de comptage des effectifs par classe cible
	IntVector ivFrequencyVector;
};

////////////////////////////////////////////////////////////////////
// Tableau d'effectif
// Il s'agit en fait d'un tableau de vecteurs d'effectifs
// d'une sous-classe de KWFrequencyVector.
// Cette classe est adaptee aux algorithmes de regroupement des lignes
// (discretisation et groupage)
class KWFrequencyTable : public Object
{
public:
	// Constructeur
	KWFrequencyTable();
	~KWFrequencyTable();

	// Initialisation avec la taille du tableau en nombre de vecteurs d'effectifs
	// Le createur de vecteur d'effectif (pour choisir la representation creuse ou pleine) doit etre initialise
	// A l'issue de cette methode, le contenu precedant est detruit et
	// une nouvelle table est initialise avec creation d'autant de vecteurs
	// d'effectifs que specifie
	void Initialize(int nFrequencyVectorNumber);

	// Object createur de vecteur d'effectif
	// Par defaut le createur de vecteur d'effectif produit des vecteurs pleins (KWDenseFrequencyVector)
	// Si necessaire, il peut etre parametre pour produire des vecteurs creux (KWDGPOPartFrequencyVector) notamment
	// lors de l'utilisation par les grilles La table doit dans ce cas etre vide
	const KWFrequencyVector* GetFrequencyVectorCreator() const;
	void SetFrequencyVectorCreator(const KWFrequencyVector* fvCreator);

	// Taille de la table en nombre de vecteurs d'effectifs
	int GetFrequencyVectorNumber() const;

	// Taille des vecteurs d'effectifs de la table
	int GetFrequencyVectorSize() const;

	// Acces aux vecteur d'effectifs
	// Memoire: l'objet retourne est gere par la table
	KWFrequencyVector* GetFrequencyVectorAt(int nIndex) const;

	// Acces a l'effectif total
	// On ne doit appeler cette methode que une fois la table entierement renseignee
	int GetTotalFrequency() const;

	// Calcul de l'effectif d'une sous-partie d'une table (entre FirstIndex compris et LastIndex exclu)
	int ComputePartialFrequency(int nFirstIndex, int nLastIndex) const;

	////////////////////////////////////////////////////////
	// Methodes avancee pour memoriser certaines caracteristique
	// de la creation de la table de contingence, pouvant
	// impacter son evaluation

	// Acces au nombre initial de modalites
	void SetInitialValueNumber(int nValue);
	int GetInitialValueNumber() const;

	// Acces au nombre de modalites apres granularisation
	// Dans le cas numerique, il s'agit de Ng, le nombre theorique = 2 ^ granularite
	// Dans le cas categoriel, il s'agit de Vg, le nombre de partiles obtenu effectivement
	void SetGranularizedValueNumber(int nValue);
	int GetGranularizedValueNumber() const;

	// Calcul du nombre total de modalites
	int ComputeTotalValueNumber() const;

	// Acces a la granularite du tableau (par defaut a 0)
	void SetGranularity(int nModality);
	int GetGranularity() const;

	// Acces au nombre de modalites de la partie poubelle (par defaut a 0)
	void SetGarbageModalityNumber(int nModality);
	int GetGarbageModalityNumber() const;

	// Acces a des valeurs d'entropie (en bit)
	// Uniquement pour les tables de vecteur d'effectifs plein (KWDenseFrequencyVector)
	// L'entropie permet de mesurer la quantite d'information
	// pour coder une variable
	// Entropie = somme( -p(x) log(p(x)), avec p(x) = e(x)/e
	void ComputeTargetFrequencies(IntVector* ivTargetFrequencies);
	double ComputeTargetEntropy(const IntVector* ivTargetFrequencies);
	double ComputeMutualEntropy(const IntVector* ivTargetFrequencies);

	////////////////////////////////////////////////////////
	// Divers

	// Copie a partir d'une table source
	void CopyFrom(const KWFrequencyTable* kwftSource);

	// Duplication (y compris du contenu)
	KWFrequencyTable* Clone() const;

	// Import a partir d'une table source
	// Comme un CopyFrom, sauf que les FrequencyVector de la table source sont transferres tel quels
	// dans la table destination, avant nettoyage de la table source par RemoveAllFrequencyVectors
	void ImportFrom(KWFrequencyTable* kwftSource);

	// Destruction ou suppression de tous les vectors de la tables
	// Toutes les caracteristiques de la table sont reinitialisees (sauf le FrequencyVectorCreator)
	void RemoveAllFrequencyVectors();
	void DeleteAllFrequencyVectors();

	// Verification de l'integrite
	boolean Check() const override;

	// Calcul de la table "modele nul" a un seul vecteur a partir d'une table source
	// Uniquement pour les tables de vecteur d'effectifs plein (KWDenseFrequencyVector)
	void ComputeNullTable(KWFrequencyTable* kwftSource);

	// Import d'un objet KWDataGridStats afin d'en deduire les probabilites conditionnelles
	// La taille de la table est deduite de la taille des grilles sources et cibles
	// (produits cartesiens des attributs sources et cibles)
	// L'indexation de la table provient de l'indexation des cellules source et cible de la grille
	void ImportDataGridStats(const KWDataGridStats* dataGridStats);

	// Filtrage des vecteurs vides
	// Le tableau ivNewIndexes (de taille l'ancienne table) contient la correspondance entre les
	// anciens et les nouveaux index de vecteurs (le nouvel index des anciens vecteurs vide est -1)
	void FilterEmptyFrequencyVectors(IntVector* ivNewIndexes);

	// Tri des lignes par effectif (ascendant ou descendant)
	void SortTableBySourceFrequency(boolean bAscending, IntVector* ivInitialLineIndexes,
					IntVector* ivSortedLineIndexes);

	// Tri des lignes de la table par effectif (ascendant ou descendant),
	// puis par modalite source (ascendant).
	// Le vecteur des modalite sources est trie de facon synchronisee avec la table.
	void SortTableAndModalitiesBySourceFrequency(SymbolVector* svSourceModalities, boolean bAscending,
						     IntVector* ivInitialLineIndexes, IntVector* ivSortedLineIndexes);

	// Tri des lignes d'une table groupee par effectif decroissant
	// En cas d'egalite, tri selon l'effectif de la premiere modalite de la ligne
	// Le vecteur ivGroups est dans l'ordre des modalites initiales, trie par effectif decroissant
	void SortTableBySourceAndFirstModalityFrequency(IntVector* ivGroups);

	// Permutation des lignes de la table
	// En entree, les deux tableaux d'objets SortableValue contiennent les lignes
	// de la table avant et apres la permutation souhaitee
	// En sortie, si les vecteurs correspondants ne sont pas a NULL, on fabrique
	// les index des lignes cibles pour chaque index initial, et des lignes initiales
	// pour chaque index cible
	// La permutation des nombres de modalites par ligne est egalement effectuee
	void PermutateTableLines(const ObjectArray* oaFromLines, const ObjectArray* oaToLines,
				 IntVector* ivInitialLineIndexes, IntVector* ivPermutatedLineIndexes);

	// Methodes de verification de tri
	// Ces methodes sont couteuse en temps de calcul, mais sont interessante
	// dans le cadre d'assertions
	boolean IsTableSortedBySourceFrequency(boolean bAscending) const;

	// Affichage
	// Integre l'affichage de la granularite et du nombre de modalites du groupe poubelle
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////
	// Parametrage avance

	// Nombre minimal de modalites d'un attribut categoriel pour la recherche d'une partition avec groupe poubelle
	// Par defaut a 7
	static int GetMinimumNumberOfModalitiesForGarbage();
	static void SetMinimumNumberOfModalitiesForGarbage(int nValue);

	// Affichage avance avec informations de granularite et de poubelle
	// Par defaut a false
	static boolean GetWriteGranularityAndGarbage();
	static void SetWriteGranularityAndGarbage(boolean bValue);

	///////////////////////////////
	///// Implementation
protected:
	// Calcul de l'effectif total
	int ComputeTotalFrequency() const;

	// Objet createur de vecteur d'effectif
	const KWFrequencyVector* kwfvFrequencyVectorCreator;

	// Tableau des vecteurs d'effectifs
	ObjectArray oaFrequencyVectors;
	// Effectif total
	mutable int nTotalFrequency;
	// Nombre initial de valeurs
	int nInitialValueNumber;
	// Nombre de valeurs apres granularisation
	int nGranularizedValueNumber;
	// Granularite de la partition
	int nGranularity;
	// Nombre de modalites du groupe poubelle
	int nGarbageModalityNumber;

	// Nombre minimum de modalites a partir duquel le modele avec poubelle est envisage (nombre de modalites apres
	// granularisation)
	static int nMinimumNumberOfModalitiesForGarbage;
	// Booleen pour l'affichage avance integrant les caracterisques de granularite et de poubelle
	static boolean bWriteGranularityAndGarbage;
};

////////////////////////////////////////////////////////////////////////
// Classe interne, utilisee uniquement pour KWFrequencyVector
// Objet a trier par effectif de KWFrequencyVector
// En cas d'egalite d'effectif, tri selon le rang de la premiere modalite du vecteur dans le vecteur initial des
// modalites trie par effectif decroissant
class KWSortableFrequencyVector : public KWSortableIndex
{
public:
	// Constructeur
	KWSortableFrequencyVector();
	~KWSortableFrequencyVector();

	// Acces au KWFrequencyVector
	void SetFrequencyVector(KWFrequencyVector* kwfvVector);
	KWFrequencyVector* GetFrequencyVector() const;

	// Acces a l'ordre de la premiere modalite du frequencyVector dans le vecteur initial des modalites triees par
	// effectif decroissant
	void SetFirstModalityFrequencyOrder(int nValue);
	int GetFirstModalityFrequencyOrder() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWFrequencyVector* kwfvVector;
	int nFirstModalityFrequencyOrder;
};

// Comparaison de deux objets KWSortableFrequencyVector
int KWSortableFrequencyVectorCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////
// Methodes en inline

// Classe KWFrequencyVector

inline KWFrequencyVector::KWFrequencyVector()
{
	nModalityNumber = 0;
	position = NULL;
}

inline KWFrequencyVector::~KWFrequencyVector() {}

inline KWFrequencyVector* KWFrequencyVector::Create() const
{
	return new KWFrequencyVector;
}

inline int KWFrequencyVector::GetSize() const
{
	return 0;
}

inline int KWFrequencyVector::GetModalityNumber() const
{
	return nModalityNumber;
}

inline void KWFrequencyVector::SetModalityNumber(int nModality)
{
	assert(nModality >= 0);
	nModalityNumber = nModality;
}

inline void KWFrequencyVector::SetPosition(POSITION pos)
{
	position = pos;
}

inline POSITION KWFrequencyVector::GetPosition() const
{
	return position;
}

// Classe KWDenseFrequencyVector

inline KWDenseFrequencyVector::KWDenseFrequencyVector() {}

inline KWDenseFrequencyVector::~KWDenseFrequencyVector() {}

inline KWFrequencyVector* KWDenseFrequencyVector::Create() const
{
	return new KWDenseFrequencyVector;
}

inline IntVector* KWDenseFrequencyVector::GetFrequencyVector()
{
	return &ivFrequencyVector;
}

inline void KWDenseFrequencyVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const KWDenseFrequencyVector* kwdfvSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwdfvSource = cast(KWDenseFrequencyVector*, kwfvSource);

	// Recopie du vecteur d'effectifs
	ivFrequencyVector.CopyFrom(&(kwdfvSource->ivFrequencyVector));
}

inline KWFrequencyVector* KWDenseFrequencyVector::Clone() const
{
	KWDenseFrequencyVector* kwfvClone;

	kwfvClone = new KWDenseFrequencyVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

inline void KWDenseFrequencyVector::WriteHeaderLineReport(ostream& ost) const
{
	int i;

	// Libelle des valeurs cibles
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		ost << "C" << i + 1 << "\t";
	ost << "V";
}

inline void KWDenseFrequencyVector::WriteLineReport(ostream& ost) const
{
	int i;

	// Frequence des valeurs cibles
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		ost << ivFrequencyVector.GetAt(i) << "\t";
	ost << nModalityNumber;
}

inline const ALString KWDenseFrequencyVector::GetClassLabel() const
{
	return "Dense frequency vector";
}

// Classe KWFrequencyTable

inline const KWFrequencyVector* KWFrequencyTable::GetFrequencyVectorCreator() const
{
	return kwfvFrequencyVectorCreator;
}

inline void KWFrequencyTable::SetFrequencyVectorCreator(const KWFrequencyVector* fvCreator)
{
	require(fvCreator != NULL);
	require(oaFrequencyVectors.GetSize() == 0);

	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = fvCreator;
}

inline int KWFrequencyTable::GetFrequencyVectorNumber() const
{
	return oaFrequencyVectors.GetSize();
}

inline int KWFrequencyTable::GetFrequencyVectorSize() const
{
	if (oaFrequencyVectors.GetSize() > 0)
		return cast(KWFrequencyVector*, oaFrequencyVectors.GetAt(0))->GetSize();
	return 0;
}

inline KWFrequencyVector* KWFrequencyTable::GetFrequencyVectorAt(int nIndex) const
{
	return cast(KWFrequencyVector*, oaFrequencyVectors.GetAt(nIndex));
}

/////

inline KWSortableFrequencyVector::KWSortableFrequencyVector()
{
	kwfvVector = NULL;
	nFirstModalityFrequencyOrder = -1;
}

inline KWSortableFrequencyVector::~KWSortableFrequencyVector() {}

inline void KWSortableFrequencyVector::SetFrequencyVector(KWFrequencyVector* kwVector)
{
	kwfvVector = kwVector;
}

inline KWFrequencyVector* KWSortableFrequencyVector::GetFrequencyVector() const
{
	return kwfvVector;
}

inline void KWSortableFrequencyVector::SetFirstModalityFrequencyOrder(int nValue)
{
	nFirstModalityFrequencyOrder = nValue;
}

inline int KWSortableFrequencyVector::GetFirstModalityFrequencyOrder() const
{
	return nFirstModalityFrequencyOrder;
}
