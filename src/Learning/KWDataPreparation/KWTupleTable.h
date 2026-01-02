// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTuple;
class KWTupleTable;
class PLShared_TupleTable;

#include "Object.h"
#include "SortedList.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWType.h"
#include "FileService.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////////////////////////////////
// Tuple: vecteur de valeur d'une base de donnes, numerique ou categoriel,
// avec l'effectif du tuple, c'est a dire le nombre d'enregistrements ayant les
// meme valeurs du tuple
// Il n'y a pas de constructeur/destructeur: on ne peut manipuler les tuples que depuis
// une table de tuples (KWTupleTable)
class KWTuple : public Object
{
public:
	// Gestion d'une valeur Symbol du tuple
	void SetSymbolAt(int nAttributeIndex, const Symbol& sValue);
	Symbol& GetSymbolAt(int nAttributeIndex) const;

	// Gestion d'une valeur Continuous du tuple
	void SetContinuousAt(int nAttributeIndex, Continuous cValue);
	Continuous GetContinuousAt(int nAttributeIndex) const;

	// Effectif du tuple
	void SetFrequency(int nValue);
	int GetFrequency() const;

	///////////////////////////////////////////////////////////////
	// Services divers

	// Comparaison d'une valeur entre deux tuple
	int CompareContinuousAt(const KWTuple* tuple, int nAttributeIndex) const;
	int CompareSymbolAt(const KWTuple* tuple, int nAttributeIndex) const;
	int CompareSymbolValueAt(const KWTuple* tuple, int nAttributeIndex) const;

	// Affichage detaille, a l'aide de la structure du tuple definie par sa table
	void FullWrite(const KWTupleTable* ownerTupleTable, ostream& ost) const;

	// Affichage standard (uniquement l'effectif)
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Constructeur et destructeur en protected
	KWTuple();
	~KWTuple();

	// Table de tuple decrivant la structure du tuple
	// A utiliser uniquement pour les assertions en mode debug
	// Ne devrait jamais etre appele en release
	const KWTupleTable* GetTupleTable() const;

	// Table de tuple decrivant la structure du tuple
	// Uniquement en mode debug
	// Inutile en mode release, pour economiser de la memoire
	debug(const KWTupleTable* tupleTable);

	// Effectif du tuple
	int nFrequency;

	// Stockage des valeurs du tuple (de taille quelconque)
	// On alouera les tuples en une seule fois, pour que les valeurs du tuple soient
	// dans le meme emplacement memoire que le reste de l'objet tuple (Frequency...)
	// Les valeurs seront initialisees par la methode KWTupleTable::NewTuple(),
	// qui est la seule habilitee a creer des tuples
	static const int nMinTupleSize = 1;
	KWValue tupleValues[nMinTupleSize]; // Pour acceder au debut du tableau des valeurs

	// La classe KWTupleTable est friend pour acceder au donnees protegees
	friend class KWTupleTable;

	// Les fonctions de comparaisons sont protegees pour acceder en mode debug
	// a la fonction GetTupleTable, permettant de controler le type des tuples
	friend int KWTupleCompare(const void* elem1, const void* elem2);
	friend int KWTupleCompareC(const void* elem1, const void* elem2);
	friend int KWTupleCompareN(const void* elem1, const void* elem2);
	friend int KWTupleCompareCC(const void* elem1, const void* elem2);
	friend int KWTupleCompareNN(const void* elem1, const void* elem2);
	friend int KWTupleCompareCN(const void* elem1, const void* elem2);
	friend int KWTupleCompareNC(const void* elem1, const void* elem2);
	//
	friend int KWTupleCompareValues(const void* elem1, const void* elem2);
	friend int KWTupleCompareValuesC(const void* elem1, const void* elem2);
	friend int KWTupleCompareValuesCC(const void* elem1, const void* elem2);
	friend int KWTupleCompareValuesCN(const void* elem1, const void* elem2);
	friend int KWTupleCompareValuesNC(const void* elem1, const void* elem2);
	//
	friend int KWTupleCompareDecreasingFrequencies(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesC(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesN(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesCC(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesNN(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesCN(const void* elem1, const void* elem2);
	friend int KWTupleCompareDecreasingFrequenciesNC(const void* elem1, const void* elem2);
	//
	friend int KWTupleCompareCAt(const void* elem1, const void* elem2);
	friend int KWTupleCompareNAt(const void* elem1, const void* elem2);
	//
	friend int KWTupleCompareCCAt(const void* elem1, const void* elem2);
	friend int KWTupleCompareNNAt(const void* elem1, const void* elem2);
	friend int KWTupleCompareCNAt(const void* elem1, const void* elem2);
	friend int KWTupleCompareNCAt(const void* elem1, const void* elem2);
};

///////////////////////////////////////////////////////////////////////////////////
// Table de tuples
// Definition de la structure d'un tuple et gestion d'un ensemble de tuples uniques
// Les tuples d'une table de tuples sont tous distincts, avec des effectifs
// plus grands ou egaux a 1
class KWTupleTable : public Object
{
public:
	// Constructeur
	KWTupleTable();
	~KWTupleTable();

	// Nettoyage complet, du contenu et de la structure
	void CleanAll();

	/////////////////////////////////////////////////////////////////////////////////////
	// Definition de la structure des tuples, uniquement si la table est vide
	// Une table de tuple peut conenir 0 a n attributs, numeriques ou categoriels

	// Ajout d'un attribut de type simple
	void AddAttribute(const ALString& sName, int nType);

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Acces aux caracteristiques des attributs
	const ALString& GetAttributeNameAt(int nAttributeIndex) const;
	int GetAttributeTypeAt(int nAttributeIndex) const;

	// Recherche de l'index d'un attribut (-1 si absent)
	int LookupAttributeIndex(const ALString& sName) const;

	// Supression de tous les attributs
	void DeleteAllAttributes();

	/////////////////////////////////////////////////////////////////////////////////////
	// Acces aux tuples de la table (en mode consultation)

	// Nombre de tuples
	int GetSize() const;

	// Effectif total des tuples de la table
	int GetTotalFrequency() const;

	// Nombre des valeurs manquantes pour les attributs sparse
	// Pertinent dans le cas de table de tuples univariees
	void SetSparseMissingValueNumber(int nValueNumber);
	int GetSparseMissingValueNumber() const;

	// Acces aux tuples
	// Les tuples sont tries par valeurs croissantes
	const KWTuple* GetAt(int nIndex) const;

	// Destruction de tous les tuples
	void DeleteAll();

	/////////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalite de tri (en mode consultation)

	// Tri standard, selon les valeurs
	// Ordre par defaut obtenu en fin du mode edition
	void Sort();

	// Tri selon les libelles des valeurs
	// Identique pour les Continuous, selon la chaine de caractere pour les Symbol
	void SortByValues();

	// Tri par frequence decroissante, puis par libelles des valeurs de facon croissante
	void SortByDecreasingFrequencies();

	/////////////////////////////////////////////////////////////////////////////////////
	// Modification de la tables de tuples (en mode edition)

	// Parametrage du mode edition
	// La fin du mode edition rend les tuple de la table accessible par index
	void SetUpdateMode(boolean bValue);
	boolean GetUpdateMode() const;

	// Acces a un tuple d'entree, disponible editer les valeurs et l'effectif d'un tuple a ajouter dans la table
	// N'est disponible qu'en phase d'edition, en etant initialise (a 0 ou "" selon les valeurs, avec un effectif
	// par defaut de 1) La mise a jour des tuple se fait alors en editant ce tuple, puis en enregistrant sa
	// contribution dans la table Memoire: appartient a l'appele
	KWTuple* GetInputTuple() const;

	// Mise a jour de la table avec les caracteristiques du tuple d'entree
	// On recherche dans la table un tuple de memes valeurs que le tuple en entree
	// (on en cree un nouveau si necessaire), et on met a jour son effectif
	// En sortie, on renvoie le tuple de la table correspondant au tuple en entree
	// Memoire: le tuple en sortie appartient a la table
	const KWTuple* UpdateWithInputTuple();

	///////////////////////////////////////////////////////////////
	// Services divers

	// Construction d'une table de tuples univariee a partir d'une table de tuples comportant l'attribut
	// Memoire: la table de tuple en sortie appartien a l'appele
	void BuildUnivariateTupleTable(const ALString& sAttributeName, KWTupleTable* outputTupleTable) const;

	// Tri selon un attribut d'un tableau de tuples extraits de la table courante
	// Le tableau en sortie contient tous les tuples de la table courante, tries
	// par valeur croissante pour l'attribut specifie (deux tuples successifs peuvent
	// avoir la meme valeur pour cet attribut).
	void SortTuplesByAttribute(const ALString& sSortAttributeName, ObjectArray* oaTuplesToSort) const;

	// Tri selon une paire d'attributs d'un tableau de tuples extraits de la table courante
	void SortTuplesByAttributePair(const ALString& sSortAttributeName1, const ALString& sSortAttributeName2,
				       ObjectArray* oaTuplesToSort) const;

	// Duplication
	KWTupleTable* Clone() const;

	// Copie
	void CopyFrom(const KWTupleTable* sourceTupleTable);

	// Export des tuples vers un tableau
	// Ce service est disponible en mode consultation ou edition
	// Memoire: les tuples appartiennent a l'appele
	void ExportObjectArray(ObjectArray* oaExportedTuples) const;

	// Affichage, ecriture dans un fichier, de facon structuree
	void Write(ostream& ost) const override;

	// Ecriture dans un fichier tabule
	void WriteFile(const ALString& sFileName);

	// Estimation de la memoire utilisee par l'objet
	longint GetUsedMemory() const override;

	// Estimation de la memoire necessaire pour stocker une table de tuple
	static longint ComputeNecessaryMemory(int nTupleNumber, int nAttributeNumber);

	// Estimation de la memoire necessaire specifique au chargement d'une table de tuple
	static longint ComputeNecessaryBuildingMemory(int nTupleNumber);

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Creation/destruction d'un tuple
	KWTuple* NewTuple() const;
	void DeleteTuple(KWTuple* tuple) const;

	// Recherche de la fonction de comparaison pour un type de tri donne
	// Permet d'eviter de retrier si les tri sont compatibles/
	// Par exemple, le tri par valeurs ou par valeurs utilisateurs est le meme
	// s'il n'y a pas d'attributs Symbol
	// On peut egalement beneficier de tris optimises, dedies pour les
	// petits nombres d'attributs
	CompareFunction GetCompareFunction() const;
	CompareFunction GetCompareValuesFunction() const;
	CompareFunction GetCompareDecreasingFrequenciesFunction() const;

	// Parametrage de la table de tuple courante a utiliser pour les comparaisons
	// On a en effet besoin de connaitre la table de tuple en cours pour les fonction de comparaisons
	void SetSortTupleTable(const KWTupleTable* tupleTable) const;
	const KWTupleTable* GetSortTupleTable() const;

	// Definition des attributs
	StringVector svAttributeNames;
	IntVector ivAttributeTypes;

	// Tableau des tuples
	// En mode consultation, tous les tuples sont dans ce tableau
	// En mode edition, ce tableau contient un seul tuple: le tuple d'entree permettant les editions
	ObjectArray oaTuples;

	// Nombre de tuples
	int nSize;

	// Effectif total
	int nTotalFrequency;

	// Nombre des valeurs manquantes en mode sparse
	int nSparseMissingValueNumber;

	// Liste triee pour la gestion du mode edition
	// En mode edition, tous les tuples sont dans cette liste, qui en assure l'unicite
	SortedList* slTuples;

	// Tuple reserve au mode edition
	KWTuple* inputTuple;
	friend class PLShared_TupleTable;
};

///////////////////////////////////////////////////////
// Classe PLShared_TupleTable
// Serialisation des vecteurs de Continuous
class PLShared_TupleTable : public PLSharedObject
{
public:
	// Constructeur
	PLShared_TupleTable();
	~PLShared_TupleTable();

	// Acces au vecteur (appartient a l'appele, jamais NULL)
	void SetTupleTable(KWTupleTable* tupleTable);
	KWTupleTable* GetTupleTable();
	const KWTupleTable* GetConstTupleTable() const;

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////////////
// Methode en inline

#ifdef __MSC__
#pragma warning(disable : 26495) // disable C26495 warning("La variable'% variable% 'n'est pas initialisee...")
#endif                           // __MSC__

inline KWTuple::KWTuple()
{
	// Pas d'initialisation de nFrequency, tupleTable et tupleValues, qui sont censes etre initialises par la
	// methode NewTuple appelante Cela declenche un warning sous Visual C++ 2019, que l'on peut ignorer
}

#ifdef __MSC__
#pragma warning(default : 26495)
#endif // __MSC__

inline KWTuple::~KWTuple() {}

inline void KWTuple::SetSymbolAt(int nAttributeIndex, const Symbol& sValue)
{
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol));

	tupleValues[nAttributeIndex].SetSymbol(sValue);
}

inline Symbol& KWTuple::GetSymbolAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol));

	return tupleValues[nAttributeIndex].GetSymbol();
}

inline void KWTuple::SetContinuousAt(int nAttributeIndex, Continuous cValue)
{
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous));

	tupleValues[nAttributeIndex].SetContinuous(cValue);
}

inline Continuous KWTuple::GetContinuousAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous));

	return tupleValues[nAttributeIndex].GetContinuous();
}

inline void KWTuple::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline int KWTuple::GetFrequency() const
{
	return nFrequency;
}

inline int KWTuple::CompareContinuousAt(const KWTuple* tuple, int nAttributeIndex) const
{
	require(tuple != NULL);
	debug(require(tupleTable == tuple->tupleTable));
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous));
	return tupleValues[nAttributeIndex].CompareContinuous(tuple->tupleValues[nAttributeIndex]);
}

inline int KWTuple::CompareSymbolAt(const KWTuple* tuple, int nAttributeIndex) const
{
	require(tuple != NULL);
	debug(require(tupleTable == tuple->tupleTable));
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol));
	return tupleValues[nAttributeIndex].CompareSymbol(tuple->tupleValues[nAttributeIndex]);
}

inline int KWTuple::CompareSymbolValueAt(const KWTuple* tuple, int nAttributeIndex) const
{
	require(tuple != NULL);
	debug(require(tupleTable == tuple->tupleTable));
	require(0 <= nAttributeIndex);
	debug(require(nAttributeIndex < tupleTable->GetAttributeNumber()));
	debug(require(tupleTable->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol));
	return tupleValues[nAttributeIndex].CompareSymbolValue(tuple->tupleValues[nAttributeIndex]);
}

inline int KWTupleTable::GetAttributeNumber() const
{
	return svAttributeNames.GetSize();
}

inline const ALString& KWTupleTable::GetAttributeNameAt(int nAttributeIndex) const
{
	return svAttributeNames.GetAt(nAttributeIndex);
}

inline int KWTupleTable::GetAttributeTypeAt(int nAttributeIndex) const
{
	return ivAttributeTypes.GetAt(nAttributeIndex);
}

inline int KWTupleTable::GetSize() const
{
	require((not GetUpdateMode() and nSize == oaTuples.GetSize()) or
		(GetUpdateMode() and nSize == slTuples->GetCount()));
	return nSize;
}

inline int KWTupleTable::GetTotalFrequency() const
{
	return nTotalFrequency;
}

inline void KWTupleTable::SetSparseMissingValueNumber(int nValueNumber)
{
	nSparseMissingValueNumber = nValueNumber;
}

inline int KWTupleTable::GetSparseMissingValueNumber() const
{
	return nSparseMissingValueNumber;
}

inline const KWTuple* KWTupleTable::GetAt(int nIndex) const
{
	require(not GetUpdateMode());
	return cast(const KWTuple*, oaTuples.GetAt(nIndex));
}

inline boolean KWTupleTable::GetUpdateMode() const
{
	assert(slTuples == NULL or inputTuple != NULL);
	return (slTuples != NULL);
}

inline KWTuple* KWTupleTable::GetInputTuple() const
{
	require(GetUpdateMode());
	ensure(inputTuple != NULL);
	return inputTuple;
}

// Classe PLShared_TupleTable

inline const KWTupleTable* PLShared_TupleTable::GetConstTupleTable() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(KWTupleTable*, GetObject());
}

inline KWTupleTable* PLShared_TupleTable::GetTupleTable()
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(KWTupleTable*, GetObject());
}

inline void PLShared_TupleTable::SetTupleTable(KWTupleTable* tupleTable)
{
	require(bIsWritable);
	SetObject(tupleTable);
}
