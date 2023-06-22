// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDSelectionOperandAnalyser;
class KDClassSelectionStats;
class KDClassSelectionOperandStats;
class KDClassContinuousSelectionOperandStats;
class KDClassSymbolSelectionOperandStats;
class KDSelectionPart;
class KDSelectionInterval;
class KDSelectionValue;

#include "KWClass.h"
#include "KDConstructionRule.h"
#include "KDConstructedRule.h"
#include "KWDRAll.h"
#include "KWLearningReport.h"
#include "KWMTDatabase.h"
#include "KWQuantileBuilder.h"
#include "KDClassBuilder.h"
#include "KDClassCompliantRules.h"
#include "KDDomainKnowledge.h"
#include "KDSelectionOperandDataSampler.h"
#include "KDSelectionOperandSamplingTask.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandAnalyser
//
// Collecte des operandes de selection d'un ensemble de regles de construction de type TableSelection
// pour passer d'une description "conceptuelle" des parties selectionnees a une description physique
// de ces parties, basee sur une lecture de la base.
//
// Pour chaque classe (KWClass), on specifie dans un objet KDClassSelectionStats:
//    . un ensemble d'operandes de selection (KDClassSelectionOperandStats), qui serviront a
//      collecter des statistiques sur des dimensions de partition (KDConstructedPartitionDimension)
//    . les partitions (KDConstructedPartition) decrites sur ces dimensions, decrivant
//      comment partitonnner une table secondaire, et avec quelle granularite par dimension
//      . par partition, les parties (KDConstructedPart) effectivement utilisees
// La description conceptuelle est portee par la classe KDConstructedPart, et permet
// d'associer chaque operande de selection a une dimension de partition (cf. KDConstructedPartition)
// et chaque partie a une granularite et un index de partile.
// Il s'agit ici d'identifier les operandes (attributs ou regles), puis de lire la base
// pour collecter dans un echantillon les statistiques par valeurs permettant de constituer
// les partitions effectives en partiles.
//
// Memoire:
// La classe principale KDSelectionOperandAnalyser contient l'ensemble des objets KDClassSelectionStats,
// La classe KDClassSelectionStats contient:
//   . les operandes de selection (KDClassSelectionOperandStats)
//   . les partitions decrites sur ces dimensions (KDConstructedPartition), toutes decrites sur les
//     dimensions (KDConstructedPartitionDimension) memorisee dans les operandes de selection
// La classe KDClassSelectionOperandStats contient:
//   . la dimension de partition (KDConstructedPartitionDimension) correspondant a l'operande de selection
// La classe KDConstructedPartition reference:
//   . ses dimensions de partition (KDConstructedPartitionDimension), qui peuvent etre
//     ainsi etre references par plusieurs partitioopn univariees ou multivariees
// et contient:
//   . l'ensemble des parties (KDConstructedPart) effectivement utilisees
// Il est a noter que la classe KDDomainKnowledge, qui construite l'ensemble des
// regle construite (KDConstructedRule), contient un tableau de ces regles avant de les transformer
// en regles de derivation (KWDerivationRule). Les regles construites ont une structure recursive,
// composee de trois types d'operandes:
//   . attribute de classe (KWAttribute), reference
//   . autre regle (KDConstructedRule), contenue
//   . partie d'une partition (KDConstructedPart), referencee, appartenant a l'ensemble
//     de la structure KDSelectionOperandAnalyser
class KDSelectionOperandAnalyser : public KWLearningReport
{
public:
	// Constructeur
	KDSelectionOperandAnalyser();
	~KDSelectionOperandAnalyser();

	// Parametrage du domaine de connaissance
	// Attention, parametrage obligatoire (initialement a NULL)
	// Memoire: appartient a l'appelant
	void SetDomainKnowledge(KDDomainKnowledge* domainKnowledgeParam);
	KDDomainKnowledge* GetDomainKnowledge() const;

	// Acces au domaine de construction, depuis le domaine de connaissance
	KDConstructionDomain* GetConstructionDomain() const;

	///////////////////////////////////////////////////////////////////////
	// Specification des analyses a effectuer, par classe et
	// par operande	de selection

	// Ajout d'un objet de stats pour une classe, avec ses regles de construction applicables
	KDClassSelectionStats* AddClassSelectionStats(const KDClassCompliantRules* classCompliantRulesParam);

	///////////////////////////////////////////////////////////////////////
	// Calcul des statistiques par operande de selection

	// Declenchement de tous les calculs
	// Les regles en entrees sont utilisees pour evaluer les nombres d'utilisation par operandes de selection
	boolean ComputeStats(const ObjectArray* oaAllConstructedRules);

	// Nettoyage des resultats d'analyse
	void CleanStats();

	// Nettoyage des specifications et des resultats d'analyse
	void CleanAll();

	///////////////////////////////////////////////////////////////////////
	// Acces au resultats d'analyse
	// Memoire: les resultats appartiennent a l'appele

	// Acces aux resultats pour toutes les classes
	const ObjectArray* GetClassSelectionStats() const;

	// Acces direct par classe
	// Renvoie NULL si non trouve
	KDClassSelectionStats* LookupClassSelectionStats(const ALString& sClassName) const;

	///////////////////////////////////////////////////////////////////////
	// Gestion des comptes d'utilisation par operande de selection de chaque partition

	// Remise a zero pour toutes les classes de selection des nombre d'utilisations pour
	// les partitions, les dimensions de partition et les parties
	void ResetAllUseCounts();

	// Affichage de l'ensemble des operandes de selection et de leur statistiques d'utilisation
	void DisplayUsedSelectionOperands(ostream& ost) const;

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul des exigence en memoire pour le calcul

	// Calcul du nombre max d'objet analysables pour limiter l'empreinte memoire des
	// algorithmes et des resultats d'analyse
	// Le probleme se pose notament dans le cas de table secondaire de tres gros volumes.
	// Le nombre max d'objet analyses sert a parametrer un algorithme de type reservoir sampling
	// pour obtenir un echantillon representatif par classe d'objet danalyse
	// Renvoie true s'il y a assez de memoire (par exemple s'il n'y a aucun operande de selection)
	boolean ComputeMaxAnalysedObjectNumber();

	//////////////////////////////////////////////////////////////////////////////////
	// Exploitation de l'ensemble des operandes de selection extraits pour analyser la
	// base et pretraiter les partitions possibles par operande de selection

	// Analyse de la base en pour le pretraitement des partitions de valeurs
	// Renvoie true si OK (false si par exemple tache interrompue)
	boolean ExtractSelectionOperandPartitions();

	// Tache de lecture de la base pour collecter un echantillon de valeurs par variable secondaire de selection
	// On renvoie le nombre d'objets lus dans la base
	boolean TaskCollectSelectionOperandSamples(int& nReadObjectNumber);

	// Construction de specifications d'echantillonnage des donnees par operande a partit des specification
	// d'analyse en entrees Seules les classes et les operandes effectivement utilisees, ayant un attribut de
	// selection, sont prises en compte Memoire: l'objet construit appartient a l'appelant
	KDSelectionOperandDataSampler* BuildSelectionOperandDataSamplerSpec() const;

	// Prise en compte des donnees collectees par un echantilonneur de donnees
	// Les donnees sources sont transferees depuis l'echantillonneur, puis nettoyees de celui-ci
	void CollectClassSelectionData(KDSelectionOperandDataSampler* sourceSelectionOperandDataSampler);

	// Lecture de la base pour collecter un echantillon de valeurs par variable secondaire de selection
	// On renvoie le nombre d'objets lus dans la base
	boolean CollectSelectionOperandSamples(int& nReadObjectNumber);

	// Construction d'un domain de lecture de la base oriente operandes de selection,
	// c'est a dire ayant tout en Unused, sauf l'acces aux classes de selection et
	// avec des variables creee par operande de selection
	// Memoire: le domaine et son contenu appartiennt a l'appelant
	KWClassDomain* BuildSelectionDomain();

	// Analyse d'objet pour enregistrer les objets des classes de selection
	// Le dictionnaire permet de gerer les objet comportant des cycles, possible via
	// des utilisation d'objet references se referencant entre eux ou via des regles de derivation
	// On assure ainsi l'unicite de la visite de chaque sous objet
	void ExtractSelectionObjects(const KWObject* kwoObject, NumericKeyDictionary* nkdAllSubObjects);

	// Extraction des valeurs de selection d'un objet
	void ExtractSelectionObjectValues(KDClassSelectionStats* classSelectionStats, const KWObject* kwoObject);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des objets references
	// Les objet references sont memorises dans un dictionnaire global une fois pour toute
	// apres ouverture de la base
	// On verifie ensuite qu'ils ne sont analyses qu'une seule fois globalement, alors
	// que la verification est locale a chaque arborescence d'objet pour les objets
	// de la classe a analyser

	// Enregistrement de tous les objets references globaux
	void RegisterAllReferencedObjects();

	// Enregistrement recursif d'un objet et de sa composition
	void RegisterReferencedObject(KWObject* kwoObject);

	// Domaine de connaissance
	KDDomainKnowledge* domainKnowlege;

	// Donnees de travail
	ObjectArray oaClassSelectionStats;
	ObjectDictionary odClassSelectionStats;
	int nMaxAnalysedObjectNumber;

	// Dictionnaire de l'ensemble des objets en references dans la base
	NumericKeyDictionary nkdAllReferencedObjects;

	// Dictionnaire de l'ensemble des objets en references deja analyses
	// Permet de verifier que les objet references ne sont analyses qu'une seule fois
	NumericKeyDictionary nkdAllAnalyzedReferencedObjects;

	// Effectif minimum par partile
	static const int nMinPartileFrequency = 4;

	// Parametrage de l'implementation en parallel
	boolean bPTIsParallel;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionStats
// Statistiques sur operandes de selection par classe
class KDClassSelectionStats : public Object
{
public:
	// Constructeur
	KDClassSelectionStats();
	~KDClassSelectionStats();

	// Parametrage de la classe, avec ses regles de construction applicables
	void SetClassCompliantRules(const KDClassCompliantRules* classCompliantRulesParam);
	const KDClassCompliantRules* GetClassCompliantRules() const;

	// Nom de la classe
	const ALString GetClassName() const;

	////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des operandes de selection
	// Ces operandes de selection appartiennent toutes a l'appele, et contiennent chacune une dimension
	// de partition; ces dimensions de partition seront mutualisees par l'ensemble des partitions

	// Acces aux operandes de selection
	const ObjectArray* GetClassSelectionOperandStats() const;

	// Recherche des stats d'un operande de selection d'une classe selon un attribut
	// Renvoie NULL si non trouve
	KDClassSelectionOperandStats*
	SearchAttributeClassSelectionOperandStats(const KWAttribute* dimensionAttribute) const;

	// Recherche des stats d'un operande de selection d'une classe selon une regle
	// La regle n'est pas forcement egale, mais doit avoir les meme caracteristiques
	// ou meme etre associe a un attribut derive avec une regle de derivation correspondant a la regle construite
	// Renvoie NULL si non trouve
	KDClassSelectionOperandStats*
	SearchRuleClassSelectionOperandStats(const KDConstructedRule* dimensionRule) const;

	// Recherche si des stats d'un operande de selection d'une classe selon une regle ne sont pas deja
	// presente dans une operande de selection selon un attribut, derive selon la regle en entree
	// Dans ce cas, on ne doit pas rajouter l'operande de type regle, redondant avec l'operande de type attribut
	// correspondant
	KDClassSelectionOperandStats*
	SearchAttributeClassSelectionOperandStatsFromRule(const KDConstructedRule* dimensionRule) const;

	// Ajout d'un resultat d'analyse pour une dimension de partition de type attribut ou regle
	// Les dimensions de type attribut doivent etre crees avant celles de type regle, ce qui permet
	// de verifier qu'une regle ne correspont pas a un attribut derive correspondant a la regle de derivation
	// L'operande de selection est cree ainsi que la dimension de partition correspondant, qui lui appartient
	KDClassSelectionOperandStats* AddAttributeClassSelectionOperandStats(const KWAttribute* dimensionAttribute);
	KDClassSelectionOperandStats* AddRuleClassSelectionOperandStats(const KDConstructedRule* dimensionRule);

	////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des operandes de selection
	// Ces partitions appartiennent toutes a l'appele

	// Acces aux partitions
	const ObjectArray* GetPartitions() const;

	// Acces a une partition par une partition identique servant de cle
	// (pas forcement egale, mais ayant exactement les memes dimensions et granularites)
	KDConstructedPartition* LookupPartition(const KDConstructedPartition* partition);

	// Ajout d'une partition basee sur les operandes de selection
	// La partition ne doit pas deja exister
	// En cas de deux partition de different que par leur DataPath, l'attribut Unique des
	// partition est gere correctement
	// Memoire: la partition appartient a l'appele
	void AddPartition(KDConstructedPartition* partition);

	// Calcul de la granularite max de tous les operandes de selection utilises en dimension d'une partition
	int ComputeMaxOperandGranularity() const;

	//////////////////////////////////////////////////////////////////////////////////////..
	// Gestion des resultats d'analyse
	// Apres calcul des stats, chaque operande de selection contient les resultats d'analyse

	// Memorisation du nombre d'objet de la base pour la classe d'objet, a mettre a jour conjointement avec les
	// stats par operande
	void SetDatabaseObjectNumber(int nValue);
	int GetDatabaseObjectNumber() const;

	// Memorisation du nombre d'objet analyses pour la classe d'objet
	void SetAnalysedObjectNumber(int nValue);
	int GetAnalysedObjectNumber() const;

	// Nettoyage des resultats d'analyse des operandes de stats
	void CleanStats();

	// Nettoyage des specifications (operandes de selection et partitions) et des resultats d'analyse
	void CleanAll();

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDSelectionOperandAnalyser;

	// Classe, avec ses regles de construction applicables
	const KDClassCompliantRules* classCompliantRules;

	// Operandes de selection (KDClassSelectionOperandStats)
	ObjectArray oaClassSelectionOperandStats;
	SortedList slClassSelectionOperandStats;

	// Partition differentes bases sur les operandes de selection (KDConstructedPartition)
	ObjectArray oaConstructedPartitions;
	SortedList slConstructedPartitions;

	// Nombre d'objets de la base
	int nDatabaseObjectNumber;
	int nAnalysedObjectNumber;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionOperandStats
// Statistiques sur une operande de selection
class KDClassSelectionOperandStats : public Object
{
public:
	// Constructeur
	KDClassSelectionOperandStats();
	~KDClassSelectionOperandStats();

	// Parametrage de la dimension de partition: attribut ou regle
	// Memoire: l'objet en parametre appartient a l'appelant et sera detruit avec celui-ci
	//   Lors d'un remplacement, le precedent doit etre prealablement supprime si necessaire
	void SetPartitionDimension(const KDConstructedPartitionDimension* dimension);
	const KDConstructedPartitionDimension* GetPartitionDimension() const;

	// Origine de l'operande
	int GetOperandOrigin() const;

	// Type de l'operande
	int GetOperandType() const;

	// Nettoyage des resultats d'analyse
	void CleanStats();

	///////////////////////////////////////////////////////////////////////
	// Acces au resultats d'analyse
	// Memoire: les resultats appartiennent a l'appele

	// Nombre de granularite disponible
	int GetGranularityNumber() const;

	// Acces aux granularites disponibles: en general: 2, 4, 8, ...
	int GetGranularityAt(int nGranularityIndex) const;
	int GetGranularityExponentAt(int nGranularityIndex) const;

	// Acces aux tableaux de partiles par granularite
	// Pour chaque granularite G (reperee par son index), le tableau de partiles ne contient
	// que les partiles utiles (d'index compris entre 0 et G-1).
	// Un partile est inutile s'il peut etre designe a un niveau de granularite plus grossier
	const ObjectArray* GetPartsAt(int nGranularityIndex) const;

	// Rechercher de l'index d'une granularite
	// Renvoie -1 si non trouve
	int SearchGranularityIndex(int nGranularity) const;

	// Recherche d'une partie selon son index (KDSelectionPart::GetIndex()) dans un tableau de partie
	// Renvoie NULL si non trouve
	KDSelectionPart* SearchPart(const ObjectArray* oaParts, int nPartIndex) const;

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Test si une granularite est valide (puissance de 2)
	static boolean CheckGranularity(int nGranularity);

	// Acces a l'exposant (puissance de 2) d'une granularite
	static int GetGranularityExponent(int nGranularity);

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDSelectionOperandAnalyser;
	friend class KDClassBuilder;
	friend class KDConstructedPartitionDimension;

	/////////////////////////////////////////////////////////////////////////
	// Methode de travail pour l'analyse des operandes de selection

	// Parametrage de l'attribute de selection permettant de calculer les valeur de l'operande de selection
	// L'attribut de selection est soit directement l'attribut de la dimension de partition, soit un attribut
	// permettant de calculer la regle de la dimension de partition
	// Memoire: gere par l'appelant
	void SetSelectionAttribute(KWAttribute* attribute);
	KWAttribute* GetSelectionAttribute() const;

	// Verification de la validite de l'attribut de selection et de sa coherence avec la dimension de partition
	// Attention: methode "chere" a calculer, car reposant sur la creation d'une regle de derivation
	boolean CheckSelectionAttribute() const;

	// Calcul des granularites et partiles accessible par analyse des donnees
	// Le calcul est effectue pour toutes les granularite en 2^k, a concurrence
	// d'un nombre max de partiles et d'un effectif minimum par partile
	virtual void AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency);

	// Nettoyage des valeurs de selection
	virtual void CleanInputData();

	// Dimension de partition associee a l'operande de selection
	const KDConstructedPartitionDimension* partitionDimension;

	// Attribute de selection permettant de calculer les statistiques des valeurs de l'operande de selection
	KWAttribute* selectionAttribute;

	// Vecteur des granularite, et tableau de partiles par granularite
	IntVector ivSelectionGranularities;
	ObjectArray oaGranularityPartArrays;
};

// Comparaison de deux statistique par operandes
int KDSparseClassSelectionOperandStatsCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassContinuousSelectionOperandStats
// Statistiques sur une operande de selection Continuous
class KDClassContinuousSelectionOperandStats : public KDClassSelectionOperandStats
{
public:
	// Constructeur
	KDClassContinuousSelectionOperandStats();
	~KDClassContinuousSelectionOperandStats();

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDSelectionOperandAnalyser;

	////////////////////////////////////////////////////////////////
	// Analyse des valeurs pour creer les partiles

	// Donnees a analyser: a initialiser avant analyse
	ContinuousVector* GetInputData();

	// Calcul des granularites et partiles accessibles par analyse du vecteur de Continuous
	void AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency) override;

	// Nettoyage des valeurs de selection
	void CleanInputData() override;

	ContinuousVector cvInputData;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSymbolSelectionOperandStats
// Statistiques sur une operande de selection Symbol
class KDClassSymbolSelectionOperandStats : public KDClassSelectionOperandStats
{
public:
	// Constructeur
	KDClassSymbolSelectionOperandStats();
	~KDClassSymbolSelectionOperandStats();

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDSelectionOperandAnalyser;

	////////////////////////////////////////////////////////////////
	// Analyse des valeurs pour creer les partiles

	// Donnees a analyser: a initialiser avant analyse
	SymbolVector* GetInputData();

	// Calcul des granularites et partiles accessible par analyse du vecteur de Symbol
	void AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency) override;

	// Nettoyage des valeurs de selection
	void CleanInputData() override;

	SymbolVector svInputData;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionPart
// Partie de selection
// Chaque partie est caracterise principalement par son index et ses valeurs
class KDSelectionPart : public Object
{
public:
	// Constructeur
	KDSelectionPart();
	~KDSelectionPart();

	// Index du partile
	void SetIndex(int nValue);
	int GetIndex() const;

	///////////////////////////////
	///// Implementation
protected:
	int nIndex;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionInterval
// Intervalle de valeurs
class KDSelectionInterval : public KDSelectionPart
{
public:
	// Constructeur
	KDSelectionInterval();
	~KDSelectionInterval();

	// Borne inf de l'intervalle
	void SetLowerBound(Continuous cValue);
	Continuous GetLowerBound() const;

	// Borne sup de l'intervalle
	void SetUpperBound(Continuous cValue);
	Continuous GetUpperBound() const;

	///////////////////////////////
	// Services divers

	// Copie et duplication
	void CopyFrom(const KDSelectionInterval* aSource);
	KDSelectionInterval* Clone() const;

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KDSelectionOperandAnalyser;

	// Bornes de l'intervalle
	Continuous cLowerBound;
	Continuous cUpperBound;
};

// Comparaison de deux intervalles
int KDSparseSelectionIntervalCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionValue
// Valeur symbolique
// Une valeur de selection vient en exemplaire unique, si son effectif est
// suffisant pour constituer un partile a elle toute seule.
// Dans le cas contraire, on garde la premiere valeur du partile, a titre documentaire,
// et on definit le partile par la liste des valeur hors du partile
class KDSelectionValue : public KDSelectionPart
{
public:
	// Constructeur
	KDSelectionValue();
	~KDSelectionValue();

	// Valeur principale
	void SetValue(const Symbol& sValue);
	Symbol& GetValue() const;

	// Valeurs hors du partile
	// Permet de definir une partie "poubelle", complementaire des autres valeurs
	SymbolVector* GetOutsideValues();

	// Test si partile poubelle
	boolean IsGarbagePart() const;

	///////////////////////////////
	// Services divers

	// Copie et duplication
	void CopyFrom(const KDSelectionValue* aSource);
	KDSelectionValue* Clone() const;

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	mutable Symbol sSymbolValue;
	SymbolVector svOutsideValues;
};

// Comparaison de deux valeurs
int KDSparseSelectionValueCompare(const void* elem1, const void* elem2);
