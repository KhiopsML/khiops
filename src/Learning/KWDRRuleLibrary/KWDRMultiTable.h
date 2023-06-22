// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour le multi-tables
// Dans toutes les regles multi-tables portant sur un attribut Continuous
// (Mean, Median...), les calculs sont effectues sur les valeurs renseignees,
// en ignorant les valeurs manquantes

// Extraction de valeur simple d'un attribut Object
class KWDRExist;
class KWDRGetContinuousValue;
class KWDRGetSymbolValue;
class KWDRGetDateValue;
class KWDRGetTimeValue;
class KWDRGetTimestampValue;

// Extraction d'un bloc sparse d'un attribut Object
class KWDRGetContinuousValueBlock;
class KWDRGetSymbolValueBlock;

// Classe ancetre des regles ayant un premier ou deuxieme operande de type Object ou ObjectArray,
// et rendant un Object ou ObjectArray de meme classe
class KWDRObjectRule;
class KWDRSubObjectRule;
class KWDRObjectArrayRule;
class KWDRObjectSetRule;

// Extraction de valeur Object ou ObjectArray d'un attribut Object
class KWDRGetObjectValue;
class KWDRGetObjectArrayValue;
class KWDRTableAtKey;
class KWDRTableAt;

// Operation ensemblistes sur les ObjectArray
class KWDRTableUnion;
class KWDRTableIntersection;
class KWDRTableDifference;
class KWDRTableSubUnion;
class KWDRTableSubIntersection;
class KWDREntitySet;
class KWDRTableExtraction;
class KWDRTableSelection;
class KWDRTableSelectAt;
class KWDRTableSort;

// Classe ancetre des regles ayant un premier operande de type ObjectArray et
// d'eventuels operandes additionnels sur des attributs secondaires, pour un calcul de stats
class KWDRTableStats;
class KWDRTableStatsContinuous;
class KWDRTableStatsSymbol;

// Calcul de stats pour des regle ayant un premier operande ObjectArray
class KWDRTableCount;
class KWDRTableCountDistinct;
class KWDRTableEntropy;
class KWDRTableMode;
class KWDRTableModeAt;
class KWDRTableMean;
class KWDRTableStandardDeviation;
class KWDRTableMedian;
class KWDRTableMin;
class KWDRTableMax;
class KWDRTableSum;
class KWDRTableTrend;
class KWDRTableConcat;

// Extraction des valeurs d'un ObjectArray sous forme d'un vecteur
class KWDRTableSymbolVector;
class KWDRTableContinuousVector;

// Extraction des valeurs d'un ObjectArray sous forme d'un dictionnaire de valeur
class KWDRTableSymbolHashMap;
class KWDRTableContinuousHashMap;

#include "KWDerivationRule.h"
#include "KWStructureRule.h"
#include "KWDRVector.h"
#include "KWDRHashMap.h"
#include "KWSortableIndex.h"
#include "KWObjectKey.h"

// Enregistrement de ces regles
void KWDRRegisterMultiTableRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExist
// Indicateur d'existence presence d'un attribut Object
class KWDRExist : public KWDerivationRule
{
public:
	// Constructeur
	KWDRExist();
	~KWDRExist();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetContinuousValue
// Valeur d'un attribut Continuous d'un sous-objet (valeur missing si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetContinuousValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetContinuousValue();
	~KWDRGetContinuousValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetSymbolValue
// Valeur d'un attribut Symbol d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetSymbolValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetSymbolValue();
	~KWDRGetSymbolValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetDateValue
// Valeur d'un attribut Date d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetDateValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetDateValue();
	~KWDRGetDateValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimeValue
// Valeur d'un attribut Time d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetTimeValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetTimeValue();
	~KWDRGetTimeValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimestampValue
// Valeur d'un attribut Timestamp d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetTimestampValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetTimestampValue();
	~KWDRGetTimestampValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetContinuousValueBlock
// Acces a un bloc sparse de valeurs Continuous d'un sous-objet (bloc vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetContinuousValueBlock : public KWDRValueBlockRule
{
public:
	// Constructeur
	KWDRGetContinuousValueBlock();
	~KWDRGetContinuousValueBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetSymbolValueBlock d'un sous-objet (bloc vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetSymbolValueBlock : public KWDRValueBlockRule
{
public:
	// Constructeur
	KWDRGetSymbolValueBlock();
	~KWDRGetSymbolValueBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWSymbolValueBlock* ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectRule
// Regle generique de type Object ou ObjectArray, dont la classe d'Object
// retournee est la meme que celle du premier operande de la regle
class KWDRObjectRule : public KWDerivationRule
{
public:
	// Reimplementation de la verification qu'une regle est completement renseignee et compilable
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode ancetre de completion des informations de specification de la regle
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSubObjectRule
// Regle generique de type Object ou ObjectArray, dont la classe d'Object
// retournee est la meme que celle du deuxieme operande de la regle
class KWDRSubObjectRule : public KWDerivationRule
{
public:
	// Reimplementation de la verification qu'une regle est completement renseignee et compilable
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode ancetre de completion des informations de specification de la regle
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectArrayRule
// Specialisation de KWDRObjectRule, ou tous les operandes doivent etre
// de type Object ou ObjectArray avec la meme classe
class KWDRObjectArrayRule : public KWDRObjectRule
{
	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification de l'integrite d'un tableau d'objet: sans NULL ni doublons
	boolean CheckObjectArray(const ObjectArray* oaObjects) const;

	// Object Array en retour de la regle
	mutable ObjectArray oaResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectSetRule
// Specialisation de KWDRObjectRule, ou tous les operandes doivent etre
// de type Object ou ObjectArray avec la meme classe
class KWDRObjectSetRule : public KWDRObjectArrayRule
{
public:
	// Reimplementation de la verification qu'une regle est completement renseignee et compilable
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;
};

// Fonction de comparaison de deux KWSortableObject, selon la cle des KWObjects (SortValue) puis leur index
int KWSortableObjectCompareKeyIndex(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetObjectValue
// Valeur d'un attribut Object d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetObjectValue : public KWDRSubObjectRule
{
public:
	// Constructeur
	KWDRGetObjectValue();
	~KWDRGetObjectValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetObjectArrayValue
// Valeur d'un attribut ObjectArray d'un sous-objet (valeur vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetObjectArrayValue : public KWDRSubObjectRule
{
public:
	// Constructeur
	KWDRGetObjectArrayValue();
	~KWDRGetObjectArrayValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAt
// Rechercher dans un attribut ObjectArray d'un objet selon son index
// Retourne vide si rang invalide
class KWDRTableAt : public KWDRObjectRule
{
public:
	// Constructeur
	KWDRTableAt();
	~KWDRTableAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAtKey
// Rechercher dans un attribut ObjectArray d'un objet identifie par les
// attributs Symbol de sa cle (cle definies selon le dictionnaire de l'ObjectArray)
// Retourne vide si absent
class KWDRTableAtKey : public KWDRObjectRule
{
public:
	// Constructeur
	KWDRTableAtKey();
	~KWDRTableAtKey();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;

	// Reimplementation de la verification qu'une regle est completement renseignee et compilable
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSelectFirst
// Selection du premier element d'un attribut ObjectArray selon un
// deuxieme operande de selection
// Similaire a KWDRTableSelection, en renvoyant uniquement le premier element trouve
class KWDRTableSelectFirst : public KWDRObjectRule
{
public:
	// Constructeur
	KWDRTableSelectFirst();
	~KWDRTableSelectFirst();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableUnion
// Union d'attributs ObjectArray de meme classe d'objet
class KWDRTableUnion : public KWDRObjectSetRule
{
public:
	// Constructeur
	KWDRTableUnion();
	~KWDRTableUnion();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableIntersection
// Intersection d'attributs ObjectArray de meme classe d'objet
class KWDRTableIntersection : public KWDRObjectSetRule
{
public:
	// Constructeur
	KWDRTableIntersection();
	~KWDRTableIntersection();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableDifference
// Difference de deux attribut ObjectArray de meme classe d'objet
class KWDRTableDifference : public KWDRObjectSetRule
{
public:
	// Constructeur
	KWDRTableDifference();
	~KWDRTableDifference();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSubUnion
// Union sur le objets d'une table secondaire (ObjectArray) des sous objets
// d'un attribut ObjectArray de la table secondaire
// Le premier operande porte sur la table, le second sur la sous-table
// Par exemple: TableSubUnion(Services, Logs) pour avoir l'union des logs des
// services d'un client
class KWDRTableSubUnion : public KWDRSubObjectRule
{
public:
	// Constructeur
	KWDRTableSubUnion();
	~KWDRTableSubUnion();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Object Array en retour de la regle
	mutable ObjectArray oaResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSubIntersection
// Intersection sur le objets d'une table secondaire (ObjectArray) des sous objets
// d'un attribut ObjectArray de la table secondaire
// Le premier operande porte sur la table, le second sur la sous-table
// Par exemple: TableSubIntersection(Services, Logs) pour avoir l'Intersection des logs des
// services d'un client
class KWDRTableSubIntersection : public KWDRSubObjectRule
{
public:
	// Constructeur
	KWDRTableSubIntersection();
	~KWDRTableSubIntersection();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Object Array en retour de la regle
	mutable ObjectArray oaResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDREntitySet
// Renvoie un ObjectArray a partir d'un ensemble d'Object
class KWDREntitySet : public KWDRObjectSetRule
{
public:
	// Constructeur
	KWDREntitySet();
	~KWDREntitySet();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableExtraction
// Extraction d'une sous-partie d'un attribut ObjectArray selon un index
// de debut et de fin
// Renvoie tous les elements ayant des index compatibles avec les index
// specifies (tolerant aux index hors taille du tableau)
class KWDRTableExtraction : public KWDRObjectArrayRule
{
public:
	// Constructeur
	KWDRTableExtraction();
	~KWDRTableExtraction();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSelection
// Selection d'un sous ensemble d'un attribut ObjectArray selon un
// deuxieme operande de selection
class KWDRTableSelection : public KWDRObjectArrayRule
{
public:
	// Constructeur
	KWDRTableSelection();
	~KWDRTableSelection();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSort
// Tri d'un attribut ObjectArray selon un ou plusieurs operandes de tri de type simple
class KWDRTableSort : public KWDRObjectArrayRule
{
public:
	// Constructeur
	KWDRTableSort();
	~KWDRTableSort();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const override;

	// Verification des operandes
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStats
// Classe ancetre des regles ayant un premier operande de type ObjectArray,
// et d'eventuelles operandes portant sur des attributs secondaires, pour un calcul de stats
// Operandes:
//   . ObjectArray (au niveau du scope initial)
// Operandes additionnels et type retour a specifier dans les classes filles
class KWDRTableStats : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableStats();
	~KWDRTableStats();

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul d'une valeur a partir d'un vecteur de valeur
	// En entree, on a en complement du vecteur de valeurs:
	//  . RecordNumber: nombre total d'enregistrement de table secondaire, d'ou ont ete extraites les valeurs
	//  . DefaultValue: la valeur par defaut des blocs de la table secondaire, pour les enregistrements absents des
	//  valeurs . Values: vecteur des valeurs extraites des blocs de la table secondaire (ne peut etre NULL, mais
	//  peut etre vide)
	// Les sous-classes prenant exactement deux operandes (une table, et une valeur de type simple)
	// doivent reimplementer une des quatre methodes suivantes, selon le type du code retour et le type
	// du deuximeme operande, qui doit etre celui des valeurs du vecteur
	// Alors que la methode prenant en entree un KWObject collecte les valeurs du deuxieme operande
	// pour tous les enregistrements de la table en premier operande, ces methodes travaillent directement
	// sur un vecteur de valeurs correspondant a toutes les valeurs collectees.
	// Et le resultat doit etre le meme
	// Ces methodes sont utiles pour implementer de facon optimisee les sous-classes de KWDRTableBlock
	// En principe, les methode standard base sur une table de KWObject pourrait prealablement extraire
	// un vecteur de valeur, et appeler la methode traitant un vecteur de valeur. Cela eviterait de
	// dupliquer une partie du code, mais on garde la soltuion actuelle pour des raisons d'optimisation.
	// Par defaut, ces methodes sont implementees avec assert(false), et une seule de ces methodes
	// doit etre reimplementee.
	virtual Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								      const ContinuousVector* cvValues) const;
	virtual Continuous ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
								  const SymbolVector* svValues) const;
	virtual Symbol ComputeSymbolStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const;
	virtual Symbol ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							  const SymbolVector* svValues) const;

	// Declaration  en classe friend pour pouvoir reutiliser le calcul des stats
	// sur chaque valeur d'un bloc sparse
	friend class KWDRTableBlockStatsContinuous;
	friend class KWDRTableBlockStatsSymbol;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStatsContinuous
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type Continuous en sortie
class KWDRTableStatsContinuous : public KWDRTableStats
{
public:
	// Constructeur
	KWDRTableStatsContinuous();
	~KWDRTableStatsContinuous();

	// Calcul de l'attribut derive (renvoie Missing par defaut pour un ObjectArray vide)
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul de la valeur sur un tableau d'objets (provenant du premier operande)
	// Les valeurs permettant de calculer les stats proviennent des autres operandes de la regle
	// La gestion du scope multiple doit etre evalue avant l'appel et nettoye ensuite
	// Le tableau en parametre doit etre non NULL et avoir au moins un element
	// (ce qui est toujours le cas pour les partie d'une partition, mais pas forcement pour un ObjectArray)
	virtual Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const = 0;

	// Valeur a retourner d'un le cas d'un ObjectArray NULL ou vide (par defaut: Missing)
	virtual Continuous GetDefaultContinuousStats() const;

	// Declaration  en classe friend pour pouvoir reutiliser le calcul des stats
	// sur chaque partie d'une partition de table
	friend class KWDRTablePartitionStatsContinuous;
	friend class KWDRTableBlockStatsContinuous;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStatsSymbol
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type Symbol en sortie
class KWDRTableStatsSymbol : public KWDRTableStats
{
public:
	// Constructeur
	KWDRTableStatsSymbol();
	~KWDRTableStatsSymbol();

	// Calcul de l'attribut derive (renvoie "" pour un ObjectArray vide)
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul de la valeur sur un tableau d'objet, en passant en
	// second parametre l'operande permettant d'acceder aux valeurs des objets
	// La gestion du scope multiple doit etre evalue avant l'appel et nettoye ensuite
	// Le tableau en parametre doit etre non NULL et avoir au moins un element
	// (ce qui est toujours le cas pour les partie d'une partition, mais pas forcement pour un ObjectArray)
	virtual Symbol ComputeSymbolStats(const ObjectArray* oaObjects) const = 0;

	// Declaration  en classe friend pour pouvoir reutiliser le calcul des stats
	// sur chaque partie d'une partition de table
	friend class KWDRTablePartitionStatsSymbol;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCount
// Nombre d'elements d'un attribut tableau d'objets
class KWDRTableCount : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableCount();
	~KWDRTableCount();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Methode redefinie ici explicitement (car tres simple)
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous GetDefaultContinuousStats() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCountDistinct
// Nombre de valeurs distincts d'un attribut categoriel issu
// d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer le nombre de valeurs distinctes
class KWDRTableCountDistinct : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableCountDistinct();
	~KWDRTableCountDistinct();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							  const SymbolVector* svValues) const override;
	Continuous GetDefaultContinuousStats() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableEntropy
// Entropie des valeurs d'un attribut categoriel issu
// d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer l'entropie
class KWDRTableEntropy : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableEntropy();
	~KWDRTableEntropy();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							  const SymbolVector* svValues) const override;
	Continuous GetDefaultContinuousStats() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMode
// Valeur la plus freqente d'un attribut categoriel issu
// d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la valeur plus frequente
class KWDRTableMode : public KWDRTableStatsSymbol
{
public:
	// Constructeur
	KWDRTableMode();
	~KWDRTableMode();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Symbol ComputeSymbolStats(const ObjectArray* oaObjects) const override;
	Symbol ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
						  const SymbolVector* svValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableModeAt
// Ieme valeur la plus freqente d'un attribut categoriel issu
// d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la valeur plus frequente,
// le troisieme sur l'index du ieme mode
class KWDRTableModeAt : public KWDRTableStatsSymbol
{
public:
	// Constructeur
	KWDRTableModeAt();
	~KWDRTableModeAt();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Symbol ComputeSymbolStats(const ObjectArray* oaObjects) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMean
// Moyenne des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la moyenne
class KWDRTableMean : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableMean();
	~KWDRTableMean();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStandardDeviation
// Ecart type des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la moyenne
class KWDRTableStandardDeviation : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableStandardDeviation();
	~KWDRTableStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMedian
// Valeur mediane des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la mediane
class KWDRTableMedian : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableMedian();
	~KWDRTableMedian();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMin
// Minimum des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer le minimum
class KWDRTableMin : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableMin();
	~KWDRTableMin();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMax
// Maximum des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer le maximum
class KWDRTableMax : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableMax();
	~KWDRTableMax();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSum
// Somme des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la somme
class KWDRTableSum : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableSum();
	~KWDRTableSum();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCountSum
// Somme des valeurs d'un attribut Continuous d'un attribut tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la somme
class KWDRTableCountSum : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableCountSum();
	~KWDRTableCountSum();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
	Continuous ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const override;
	Continuous GetDefaultContinuousStats() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableTrend
// Pente de la droite de regression d'une valeur numerique (Y) par rapport
// a une valeur de reference (X)
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la tendance et le troisieme sur
// l'attribut de reference
// Les valeurs manquantes (X ou Y) ne sont pas prises en compte
class KWDRTableTrend : public KWDRTableStatsContinuous
{
public:
	// Constructeur
	KWDRTableTrend();
	~KWDRTableTrend();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Continuous ComputeContinuousStats(const ObjectArray* oaObjects) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableConcat
// Concatenation des valeurs d'un attribut Symbol d'un tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut calculer la concatenation
class KWDRTableConcat : public KWDRTableStatsSymbol
{
public:
	// Constructeur
	KWDRTableConcat();
	~KWDRTableConcat();

	// Creation
	KWDerivationRule* Create() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition de la methode de calcul des stats
	Symbol ComputeSymbolStats(const ObjectArray* oaObjects) const override;
	Symbol ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
						  const SymbolVector* svValues) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSymbolVector
// Regle de derivation de type Structure(VectorC), basee sur l'extraction
// de valeurs Symbol d'un attribut d'un tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut extraire les valeurs
class KWDRTableSymbolVector : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableSymbolVector();
	~KWDRTableSymbolVector();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRSymbolVector symbolVector;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableContinuousVector
// Regle de derivation de type Structure(DoubleVector), basee sur l'extraction
// de valeurs Continuous d'un attribut d'un tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut dont il faut extraire les valeurs
class KWDRTableContinuousVector : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableContinuousVector();
	~KWDRTableContinuousVector();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRContinuousVector continuousVector;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSymbolHashMap
// Regle de derivation de type Structure(HashMapC), basee sur l'extraction
// de cle et de valeurs Symbol d'un attribut d'un tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut cle et le troisime sur les valeurs
// Si une cle est presente en plusieurs exemplaire, seul le premier
// exemplaire est pris en compte
class KWDRTableSymbolHashMap : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableSymbolHashMap();
	~KWDRTableSymbolHashMap();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRSymbolHashMap symbolHashMap;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableContinuousHashMap
// Regle de derivation de type Structure(HashMap), basee sur l'extraction
// de cles et de valeurs Continuous d'un attribut d'un tableau d'objets
// Le premier operande porte sur le tableau d'objet, le second sur
// l'attribut cle et le troisime sur les valeurs
// Si une cle est presente en plusieurs exemplaire, seul le premier
// exemplaire est pris en compte
class KWDRTableContinuousHashMap : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableContinuousHashMap();
	~KWDRTableContinuousHashMap();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRContinuousHashMap continuousHashMap;
};
