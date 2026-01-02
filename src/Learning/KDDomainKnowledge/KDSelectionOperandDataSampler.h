// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDSelectionOperandDataSampler;
class KDClassSelectionData;
class KDClassSelectionOperandData;
class KDClassSelectionObjectRef;
class PLShared_SelectionOperandDataSampler;
class PLShared_ClassSelectionData;
class PLShared_ClassSelectionOperandData;
class PLShared_ClassSelectionObjectRef;

#include "KWClass.h"
#include "KWMTDatabase.h"
#include "SortedList.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandDataSampler
//
// Classe de service pour la classe KDSelectionOperandAnalyser
//   . KDSelectionOperandAnalyser supervise l'ensemble des l'extraction des operandes de selection
//   . KDSelectionOperandSamplingTask gere l'extraction des donnees paroperande de selection
//   . KDSelectionOperandDataSampler comporte une specification minimale des operandes de selection
//     issue de KDSelectionOperandAnalyser pour memoriser un echantillon de donnees par operande,
//     extraite par la tache KDSelectionOperandSamplingTask
// Cette classe est donc dediee a la memorisation des echantillons de donnees par operande de selection
// pour la tache KDSelectionOperandSamplingTask.
// Les specification des operandes a extraire proviennent du KDSelectionOperandAnalyser en entree
// de la tache. Les donnees collectee par chaque esclave seront agregees par le maitre au moyen de variable
// partagees de type KDSelectionOperandDataSampler, pour alimenter a la fin le KDSelectionOperandAnalyser.
class KDSelectionOperandDataSampler : public Object
{
public:
	// Constructeur
	KDSelectionOperandDataSampler();
	~KDSelectionOperandDataSampler();

	// Nettoyage des donnees collectees
	void CleanData();

	// Nettoyage des specifications et des donnees collectees
	void CleanAll();

	///////////////////////////////////////////////////////////////////////
	// Acces au donnees collectees
	// Memoire: appartient a l'appele

	// Nom de la base de donnees
	const ALString& GetDatabaseName();

	// Acces aux donnees pour toutes les classes
	const ObjectArray* GetClassSelectionData() const;

	// Acces direct par classe
	// Renvoie NULL si non trouve
	KDClassSelectionData* LookupClassSelectionData(const ALString& sClassName) const;

	///////////////////////////////
	// Services divers

	// Taille max des echantillons collectes par classe
	void SetMaxSampleSize(int nValue);
	int GetMaxSampleSize() const;

	// Calcul de la taille max des echantillons a collecter pour assurer une fiabilite de
	// l'estimation des partiles
	int ComputeMaxSampleSize();

	// Nombre total d'operande sur l'ensemble des classes
	int GetTotalSelectionOperandNumber() const;

	// Duplication de la partie specification
	KDSelectionOperandDataSampler* CloneSpec() const;

	// Echange du contenu avec toutes les donnees collectees par classe
	void SwapClassSelectionData(KDSelectionOperandDataSampler* otherSelectionOperandDataSampler);

	// Aggregation des donnees collectees par un autre echantilonneur de donnees
	// Les donnees sources sont transferees depuis l'echantillonneur source, puis nettoyees de celui-ci
	void AggregateClassSelectionData(KDSelectionOperandDataSampler* sourceSelectionOperandDataSampler);

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class PLShared_SelectionOperandDataSampler;
	friend class KDSelectionOperandAnalyser;
	friend class KDSelectionOperandSamplingTask;

	//////////////////////////////////////////////////////////////////////////////////////
	// Collecte des echantillons de donnees par operande de selection
	//
	// Chaque objet est reference par une paire (MainObjectIndex, subObjectIndex) qui permettra de generer un
	// nombre aleatoire de facon reproductible pour la selection des objets a garder dans l'echantillon
	// On utilise des valeurs de MainObjectIndex superieure ou egale a 1 pour les objets principaux,
	// et avec une valeur 0 pour les objets des tables externes
	//
	// Le dictionnaire nkdAllSubObjects permet de gerer les objets comportant des cycles, possible via
	// des utilisations d'objets references se referencant entre eux ou via des regles de derivation
	// On assure ainsi l'unicite de la visite de chaque sous objet
	// Pour les tables externes, c'est l'ensemble exhaustif de tous les objets de toutes les tables externes
	// qui est concerne. Les tables externes etant triees par classe, puis par CreationIndex des objets racines,
	// l'unicite de l'index des objets externes est assure.
	//
	// Lors de l'analyse des objets de la table principale, on pousse l'analyse des objets internes completement
	// jusqu'a avoir decouvert tous les objets des tables externes utilises
	// Lors de l'analyse des objets des tables externes, tous les objets externes seront parcourus exhaustivement,
	// mais seuls ceux identifies lors de la passe d'analyse des objets principaux sont pris en compte.

	// Analyse d'un objet et de sa composition pour enregistrer les sous-objets a garder dans
	// les echantillons par classes de selection
	void ExtractSelectionObjects(const KWObject* kwoObject, longint lMainObjectIndex, longint& lSubObjectIndex,
				     NumericKeyDictionary* nkdAllSubObjects);

	// Analyse d'un objet unique pour enregistrer l'echantillon de sa par classe de selection
	// Renvoie true si l'objet a ete effectivement analyse, et il faut propager l'analyse aux sous-objets
	// Renvoie false etait deja analyse auparavent et enregistre dans le dictionnaire local d'analyse en parametre
	// ou dans le dictionnaire des objets externes analyses
	boolean ExtractSelectionOneObject(const KWObject* kwoObject, longint lMainObjectIndex, longint& lSubObjectIndex,
					  NumericKeyDictionary* nkdAllSubObjects,
					  KDClassSelectionData* classSelectionData);

	// Extraction des valeurs de selection d'un objet
	void ExtractSelectionObjectValues(KDClassSelectionData* classSelectionData, const KWObject* kwoObject,
					  longint lMainObjectIndex, longint lSubObjectIndex);

	// Extraction des valeurs de selection de tous les objets references globaux ayant ete utilises au moins une fois
	void ExtractAllSelectionReferencedObjects(KWMTDatabase* inputDatabase);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des objets references
	// Les objet references sont memorises dans un dictionnaire global une fois pour toute
	// apres ouverture de la base
	// On verifie ensuite qu'ils ne sont analyses qu'une seule fois globalement, alors
	// que la verification est locale a chaque arborescence d'objet pour les objets
	// de la classe a analyser

	// Enregistrement de tous les objets references globaux
	void RegisterAllReferencedObjects(KWMTDatabase* inputDatabase);

	// Enregistrement recursif d'un objet et de sa composition
	void RegisterReferencedObject(KWObject* kwoObject);

	// Nettoyage de tous les objets references globaux
	void CleanAllReferencedObjects();

	//////////////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Parametrage du fichier principal de la base de donnees
	void SetDatabaseName(const ALString& sValue);

	// Parametrage de la taille du fichier principal de la base
	void SetDatabaseFileSize(longint lValue);

	// Mise a jour du dictionnaire des classe a partir du tableau
	void RefreshClassSelectionData();

	// Recherche du prochain nombre premier superieur ou egal a une valeur donnee
	longint GetNextPrimeNumber(longint lMinValue) const;

	// Nombre aleatoire pour un index d'objet principal et un index d'enregistrement secondaire locale au dossier
	double GetObjectRandomDouble(longint lMainIndex, longint lSecondaryIndex) const;

	// Donnees de collecte des echantillons
	ObjectArray oaClassSelectionData;
	ObjectDictionary odClassSelectionData;

	// Dictionnaire de l'ensemble des objets en references dans la base
	// Permet de savoir si on accede a un objet d'une table externe
	NumericKeyDictionary nkdAllReferencedObjects;

	// Dictionnaire de l'ensemble des objets en references deja analyses
	// Permet de verifier que les objet references ne sont analyses qu'une seule fois
	NumericKeyDictionary nkdAllAnalyzedReferencedObjects;

	// Nom du fichier principale de la base de donnees
	ALString sDatabaseName;

	// Taille max des echantillons collectes par classe
	int nMaxSampleSize;

	// Taille du fichier principal de la base
	longint lDatabaseFileSize;

	// Nombre premier majorant de la taille de l'ensemble des fichiers, donc de l'ensemble des objet principaux
	// et des objets racine des tables externes
	// Cela permet de generer des index uniques pour l'ensemble de tous les objet parcourus
	longint lRandomPrimeFactor;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionData
// Statistiques sur operandes de selection par classe
class KDClassSelectionData : public Object
{
public:
	// Constructeur
	KDClassSelectionData();
	~KDClassSelectionData();

	// Nom de la classe
	const ALString GetClassName() const;

	////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des operandes de selection
	// Ces operandes de selection appartiennent toutes a l'appele, et contiennent chacune une dimension
	// de partition; ces dimensions de partition seront mutualisees par l'ensemble des partitions

	// Acces aux operandes de selection
	const ObjectArray* GetClassSelectionOperandData() const;

	// Granularite max de tous les operandes de selection
	int GetMaxOperandGranularity() const;

	//////////////////////////////////////////////////////////////////////////////////////..
	// Gestion des resultats d'analyse
	// Apres calcul des stats, chaque operande de selection contient les resultats d'analyse

	// Parametrage d'un seuil de selection des objets
	void SetObjectSelectionProbThreshold(double dValue);
	double GetObjectSelectionProbThreshold() const;

	// Taille courante de l'echantillon
	int GetSampleSize() const;

	// Proba la plus petite des objets selectionnees dans l'echantillon
	double GetSampleMinSelectionProb() const;

	// Nombre d'objets analyses
	// Ce nombre peut depasser le nombre total des oebjt de la base, dans le cas parallele ou les objet des tables
	// externes peuvent etre analyses par plusieurs esclaves
	void SetAnalysedObjectNumber(longint lValue);
	longint GetAnalyzedObjectNumber() const;

	// Nettoyage des donnees collectees par operande
	void CleanData();

	// Nettoyage des specifications (operandes de selection et partitions) et des resultats d'analyse
	void CleanAll();

	///////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class PLShared_ClassSelectionData;
	friend class KDSelectionOperandDataSampler;
	friend class KDSelectionOperandAnalyser;

	// Acces aux objets selectionnes
	SortedList* GetSelectionObjects();

	// Nom de la classe
	// Permet de serialiser l'objet via des variables partagees
	ALString sClassName;

	// Granularite max de tous les operandes de selection
	int nMaxOperandGranularity;

	// Operandes de selection (KDClassSelectionOperandData)
	ObjectArray oaClassSelectionOperandData;

	// Liste des objets selectionnes, triee par valeur aleatoire decroissante
	SortedList slSelectedObjects;

	// Seuil de proba pour la selection des objets
	double dObjectSelectionProbThreshold;

	// Nombre d'objets analyses
	longint lAnalysedObjectNumber;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionOperandData
// Statistiques sur un operande de selection
class KDClassSelectionOperandData : public Object
{
public:
	// Constructeur
	KDClassSelectionOperandData();
	~KDClassSelectionOperandData();

	// Nom de l'attribut correspondant a l'operande de selection
	const ALString GetSelectionAttributeName() const;

	// Nettoyage des donnees collectees
	void CleanData();

	///////////////////////////////
	// Services divers

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class PLShared_ClassSelectionOperandData;
	friend class PLShared_SelectionOperandDataSampler;
	friend class KDSelectionOperandDataSampler;
	friend class KDSelectionOperandAnalyser;
	friend int KDSparseClassSelectionOperandDataCompare(const void* elem1, const void* elem2);

	/////////////////////////////////////////////////////////////////////////
	// Methode de travail pour l'analyse des operandes de selection

	// Parametrage de l'attribute de selection permettant de calculer les valeur de l'operande de selection
	// L'attribut de selection est soit directement l'attribut de la dimension de partition, soit un attribut
	// permettant de calculer la regle de la dimension de partition
	// Memoire: gere par l'appelant
	void SetSelectionAttribute(KWAttribute* attribute);
	KWAttribute* GetSelectionAttribute() const;

	// Donnees collectees selon le type d'attribut
	ContinuousVector* GetContinuousInputData();
	SymbolVector* GetSymbolInputData();

	// Nom de l'attribut de selection
	// Permet de serialiser l'objet via des variables partagees
	ALString sSelectionAttributeName;

	// Attribute de selection permettant de calculer les statistiques des valeurs de l'operande de selection
	KWAttribute* selectionAttribute;

	// Donnees collectees selon le type d'attribut
	ContinuousVector cvContinuousInputData;
	SymbolVector svSymbolInputData;
};

// Comparaison de deux statistique par operandes
int KDSparseClassSelectionOperandDataCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionObjectRef
// Reference sur un objet selectionne pour une classe de selection donnees
// Une reference d'objet est identifiee par un couple (MainObjectIndex, SubObjectIndex), qui permet
// d'avoir un nombre aleatoire unique de facon reproductible
// On peut alors l'associe a un index de valeur secondaire, utilisable pour referencer les valeurs
// echantilonnees pour toutes les variables de l'objet
class KDClassSelectionObjectRef : public Object
{
public:
	// Constructeur
	KDClassSelectionObjectRef();
	~KDClassSelectionObjectRef();

	// Index d'objet principal
	void SetMainObjectIndex(longint lValue);
	longint GetMainObjectIndex() const;

	// Index de sous-objet
	void SetSubObjectIndex(longint lValue);
	longint GetSubObjectIndex() const;

	// Probabilite de selection deduitee de (MainObjectIndex, SubObjectIndex)
	void SetSelectionProb(double dValue);
	double GetSelectionProb() const;

	// Index de valeur dans tous les vecteur de valeurs echantillonnes pour la classe
	void SetValueIndex(int nValue);
	int GetValueIndex() const;

	///////////////////////////////
	// Services divers

	// Copie et duplication
	void CopyFrom(const KDClassSelectionObjectRef* aSource);
	KDClassSelectionObjectRef* Clone() const;

	// Methode de comparaison
	// La comparaison se fait sur la probabilite de selection par valeur croissante,
	// puis en cas d'egalite sur (MainObjectIndex, SubObjectIndex)
	int Compare(const KDClassSelectionObjectRef* otherObject) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class PLShared_ClassSelectionObjectRef;

	// Attributs de la classe
	longint lMainObjectIndex;
	longint lSubObjectIndex;
	double dSelectionProb;
	int nValueIndex;
};

// Comparaison de deux references d'objets par valeur aleatoire decroissante
int KDClassSelectionObjectRefCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////
// Classe PLShared_SelectionOperandDataSampler
// Serialisation de la classe PLShared_SelectionOperandDataSampler
class PLShared_SelectionOperandDataSampler : public PLSharedObject
{
public:
	// Constructeur
	PLShared_SelectionOperandDataSampler();
	~PLShared_SelectionOperandDataSampler();

	// Acces aux statistiques
	void SetSelectionOperandDataSampler(KDSelectionOperandDataSampler* selectionOperandDataSampler);
	KDSelectionOperandDataSampler* GetSelectionOperandDataSampler();

	// Methode a appeler apres la deserialisation pour finaliser les specifications
	// Permet de connecter les operandes aux attributs de selections des dictionnaires correspondant
	void FinalizeSpecification();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionData
// Serialisation de la classe PLShared_ClassSelectionData
class PLShared_ClassSelectionData : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ClassSelectionData();
	~PLShared_ClassSelectionData();

	// Acces aux statistiques
	void SetClassSelectionData(KDClassSelectionData* classSelectionData);
	KDClassSelectionData* GetClassSelectionData();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionOperandData
// Serialisation de la classe PLShared_ClassSelectionOperandData
class PLShared_ClassSelectionOperandData : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ClassSelectionOperandData();
	~PLShared_ClassSelectionOperandData();

	// Acces aux statistiques
	void SetClassSelectionOperandData(KDClassSelectionOperandData* classSelectionData);
	KDClassSelectionOperandData* GetClassSelectionOperandData();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionObjectRef
// Serialisation de la classe PLShared_ClassSelectionObjectRef
class PLShared_ClassSelectionObjectRef : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ClassSelectionObjectRef();
	~PLShared_ClassSelectionObjectRef();

	// Acces aux statistiques
	void SetClassSelectionObjectRef(KDClassSelectionObjectRef* classSelectionData);
	KDClassSelectionObjectRef* GetClassSelectionObjectRef();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
