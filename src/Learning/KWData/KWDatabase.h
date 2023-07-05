// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDatabase;

#include "KWClass.h"
#include "KWObject.h"
#include "KWTypeAutomaticRecognition.h"
#include "KWDataTableDriver.h"
#include "KWDRStandard.h"
#include "Vector.h"
#include "TaskProgression.h"
#include "Timer.h"
#include "RMResourceManager.h"
#include "KWLoadIndex.h"
#include "KWDatabaseMemoryGuard.h"

///////////////////////////////////////////////////////////
// Gestion d'une base de donnees de KWObject
// Il s'agit d'une classe virtuelle, comportant la
// definition des services generiques de base de donnees.
// ces services doivent etre reimplementes dans une sous-classe.
//
// Le nom de la KWClass associee doit etre precise:
//		- s'il correspond a une KWClass existante, celle-ci
//        sera utilise comme reference pour charger les
//        enregistrements avec le bon format
//      - sinon, une KWClass sera construite avec analyse
//        automatique du type des champs des enregistrements
//
// Si la classe existe de facon prealable, les champs inconnus
// seront ignores, ainsi que les champs calcules (qui sont
// recalcules)
//
// Il est a noter qu'en lecture/ecriture, seuls les attributs
// de type Loaded seront geres.
class KWDatabase : public Object
{
public:
	// Constructeur
	KWDatabase();
	~KWDatabase();

	//////////////////////////////////////////////////////////////////
	// Duplication
	// Seules les specifications sont dupliquees, pas les objets
	// Par contre, si on utilise une base dupliquee pour la lecture,
	// on chargera des objets ayant les memes caracteristiques

	// Duplication (s'appuie sur Create et CopyFrom)
	KWDatabase* Clone() const;

	// Creation pour renvoyer une instance du meme type dynamique
	virtual KWDatabase* Create() const;

	// Recopie des attributs de definition de la la base
	// Peut eventuellement etre redefini si necessaire
	// Prerequis: la base cible doit etre vide
	virtual void CopyFrom(const KWDatabase* kwdSource);

	// Reinitialisation ou recopie uniquement le parametrage d'echantillonnage et de selection
	void InitializeSamplingAndSelection();
	void CopySamplingAndSelectionFrom(const KWDatabase* kwdSource);

	// Comparaison des attributs de definition avec une autre base du meme type
	virtual int Compare(const KWDatabase* kwdSource) const;

	/////////////////////////////////////////////////////////////////////
	// Parametrage obligatoire pour l'utilisation de la base

	// Nom de la base de donnees
	virtual void SetDatabaseName(const ALString& sValue);
	virtual const ALString& GetDatabaseName() const;

	// Nom de la classe associee
	// La classe correspondante doit exister et etre valide pour toutes
	// les operation de lecture/ecriture (sauf ComputeClass)
	virtual void SetClassName(const ALString& sValue);
	virtual const ALString& GetClassName() const;

	/////////////////////////////////////////////////////////////////////
	// Services et information de nommage et de technologie utilisee

	// Ajout d'un prefixe ou ux fichiers utilises pour gerer la base
	virtual void AddPrefixToUsedFiles(const ALString& sPrefix);

	// Ajout d'un d'un suffixe aux fichiers utilises pour gerer la base (ici, fin du nom du fichier, avant le
	// suffxie du fichier)
	virtual void AddSuffixToUsedFiles(const ALString& sSuffix);

	// Ajout d'un path aux fichiers utilises pour gerer la base (qui n'en ont pas deja un)
	virtual void AddPathToUsedFiles(const ALString& sPathName);

	// Nombre de tables utilisee (utile en multi-tables)
	virtual int GetTableNumber() const;

	// Export des specs de fichier utilises pour gerer la base, a fins de controles
	// La variante avec Write concerne les fichiers utilise en ecriture uniquement
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	//    le tableau contient des objets FileSpec appartenant a l'appelant
	virtual void ExportUsedFileSpecs(ObjectArray* oaUsedFileSpecs) const;
	virtual void ExportUsedWriteFileSpecs(ObjectArray* oaUsedFileSpecs) const;

	// Nom d'une technologie de base de donnees ("" par defaut)
	// Methodes a reimplementer dans les technologies reellement instanciables
	virtual ALString GetTechnologyName() const;

	// Indique si l'on est en technologie multi-tables (defaut: false)
	virtual boolean IsMultiTableTechnology() const;

	// Acces au driver de table utilise pour les acces aux fichiers
	// Methode avancee permettant de parametre le driver de facon generique
	// Attention: methode avancee a n'utiliser dans les sous-classes
	// qu'en dehors des acces en lecture ou ecriture
	virtual KWDataTableDriver* GetDataTableDriver();

	///////////////////////////////////////////////////////////////////
	// Lecture/ecriture instance par instance
	// Si le suivi des taches est active, ces methodes alimentent le
	// suivi des taches et gerent l'interruption des taches.

	// Initialisation des caracteristiques de la classe a partir de la base
	// de donnees. Fonctionnalite disponible si la classe est inexistante.
	// Renvoie NULL et des messages d'erreur en cas de probleme. Sinon, la
	// classe est cree et enregistree.
	// Methode interruptible, sans consequence autre qu'une analyse d'un moins
	// grand nombre d'enregistrements
	KWClass* ComputeClass();

	// Ouverture de la base de donnees pour lecture ou ecriture
	// L'etat passe en IsOpened si l'ouverture a reussie
	boolean OpenForRead();
	boolean OpenForWrite();

	// Test etat de la base
	boolean IsOpenedForRead() const;
	boolean IsOpenedForWrite() const;

	// Test de fin de base (cas Read), ou d'interruption utilisateur
	boolean IsEnd() const;

	// Lecture d'une instance (a liberer par l'apelant)
	// Les attributs derives sont calcules.
	// Renvoie NULL dans les cas suivants:
	//   - si instance ignoree selon l'echantilonnage, la selection ou le marquage des instances
	//   - si warning non bloquant
	//   - si interruption utilisateur (test par TaskProgression)
	//   - si fin de base (test par IsEnd())
	//   - si erreur bloquante (test par IsError())
	// En cas de probleme de lecture, les problemes sont diagnostiques par des warning ou error
	// Le suivi des taches (progression, messages d'avancement, message si interruption) est a faire par l'appelant
	KWObject* Read();

	// Saut d'un enregistrement
	void Skip();

	// Test si erreur grave
	// Positionne suite a une erreur grave de lecture ou ecriture, reinitialise uniquement a la fermeture du fichier
	boolean IsError() const;

	// Ecriture d'une instance
	// Peut positionner une erreur grave
	void Write(const KWObject* kwoObject);

	// Fermeture de la base
	// Doit etre appele uniquement si la base est ouverte
	// Renvoie false si erreur grave avant ou pendant la fermeture, true sinon
	boolean Close();

	////////////////////////////////////////////////////////////////////
	// Lecture/ecriture globale pour gestion de tous les objets en memoire

	// Lecture de tous les objets
	// Le resultat de la lecture est accessible par la methode GetObjets
	// Methode avec suivi de tache
	// En cas d'interruption, d'erreur ou de memoire insuffisante, on renvoie
	// false en emettant un warning et on detruits les objets ayant deja ete lus
	boolean ReadAll();

	// Acces aux objets
	// Ce tableau a pour role principal d'accueillir le resultat d'un ReadAll.
	// Il peut aussi etre modifie explicitement.
	// Memoire: le tableau et son contenu sont gere par KWDatabase
	ObjectArray* GetObjects();

	// Supression/destruction de tous les objets charges en memoire
	void RemoveAll();
	void DeleteAll();

	// Ecriture de tous les objets charges en memoire,
	// provenant d'une autre base de donnees
	// Si erreur, destruction de la base
	// Methode avec suivi de tache
	// Si interruption utilisateur, on conserve la partie de la base ecrite
	// Retourne false si erreur ou interruption, true sinon
	boolean WriteAll(KWDatabase* sourceObjects);

	// Nombre exact d'objets presents dans la base de donnees
	// Permet une evaluation du volume memoire d'un chargement global
	virtual longint GetExactObjectNumber();

	// Nombre approximatif d'objets presents dans la base de donnees
	// Permet une evaluation du volume memoire d'un chargement global
	// L'interet de cette methode est sa rapidite, car elle opere sur un
	// echantillon d'objets
	// Renvoie 0 si on ne sait pas
	longint GetEstimatedObjectNumber();

	// Idem en tenant compte de l'echantillonnage (proportion des objets a garder)
	// (on ne tient pas compte du critere de selection)
	longint GetSampleEstimatedObjectNumber();

	// Estimation du pourcentage d'avancement de la lecture d'un fichier
	// Methode a priori rapide, sans effet important sur le temps de lecture
	// Peut ne pas etre reimplementee (par defaut: 0)
	double GetReadPercentage();

	/////////////////////////////////////////////////////////////
	// Gestion de la constitution d'un echantillon en lecture.
	// Une pourcentage des exemples est soit inclu, soit exclu
	// (selon le SamplingMode) lors de la lecture.
	// Les exemples choisis le sont au hazard.
	// Le generateur aleatoire est reinitialise de la meme facon
	// a chaque lecture, ce qui permet par exemple de separer
	// un fichier en deux parties (apprentissage et validation)
	// en refaisant la lecture du fichier en changeant le mode
	// d'echantillonnage

	// Pourcentage des exemples a garder ou ignorer (defaut: 100)
	void SetSampleNumberPercentage(double dValue);
	double GetSampleNumberPercentage() const;

	// Indique si on est en mode exclusion des exemples pour l'echantillonage
	void SetModeExcludeSample(boolean bValue);
	boolean GetModeExcludeSample() const;

	// Mode d'echantillonnage: commme methode precedente, avec parametrage chaine de caractere
	// Utilise pour les libelles et le parametrage des interfaces graphiques
	//  Include sample: (par defaut): on garde les exemples
	//  Exclude sample: on les ignore
	void SetSamplingMode(const ALString& sValue);
	const ALString GetSamplingMode() const;

	// Verification du libelle, sans message d'erreur
	static boolean CheckSamplingMode(const ALString& sValue);

	// Indique si l'echantillonnage est vide,
	// soit a 0% en mode ""Include sample"
	// soit a 100% en mode ""Exclude sample"
	boolean IsEmptySampling() const;

	// Attribut de selection des exemples
	// Par defaut, si rien n'est precise, tous les exemples sont traites.
	// Si un attribut est precise, seuls les exemples pour lesquels la
	// valeur de l'attribut de selection correspond a la valeur de selection
	// des exemples seront pris en compte
	void SetSelectionAttribute(const ALString& sValue);
	const ALString& GetSelectionAttribute() const;

	// Valeur de selection des exemples: utilise uniquement si un attribut
	// de selection des exemples est specifie
	void SetSelectionValue(const ALString& sValue);
	const ALString& GetSelectionValue() const;

	// Verification de la validite d'une valeur de selection (qui doit etre
	// Continuous si l'attribut de selection est Continuous). La validite
	// de l'eventuel attribut de selection est verifiee de facon prealable
	// Emission de Warning si erreur
	boolean CheckSelectionValue(const ALString& sValue) const;

	// Parametrage avance: choix des instances a lire en parametrant les index
	// des instances a garder (marquage true) ou a eviter (marquage false).
	// Ce parametrage s'applique aux instances restantes apres application
	// de l'echantillonnage et de la selection.
	// Les instances d'index depassant la taille du vecteur de parametrages
	// seront lues (par defaut, elles le sont donc toutes)
	// Memoire: le vecteur de parametrage appartient a l'appele
	IntVector* GetMarkedInstances();

	// Parametrage avance: index de record du dernier enregistrement lu, pour le marquer
	longint GetLastReadMarkIndex() const;

	////////////////////////////////////////////////////////////////////////////////
	// Pametrage de la protection memoire des acces en lecture
	// Permet d'eviter de depasser la memoire en cas d'instance volumineuse a lire et traiter
	// Ce parametrage est a effectuer avant ouverture de la base

	// Reinitialisation des parametres la protection memoire
	void ResetMemoryGuard();

	// Nombre d'enregistrements secondaires au dela duquel une alerte est declenchee
	// Cela ne declenche qu'un warning informatif en cas de depassement
	// Parametrage inactif si 0
	void SetMemoryGuardMaxSecondaryRecordNumber(longint lValue);
	longint GetMemoryGuardMaxSecondaryRecordNumber() const;

	// Limite a ne pas depasser de la memoire utilisable dans la heap pour gerer l'ensemble de la lecture et du
	// calcul des attributs derivee Parametrage inactif si 0
	void SetMemoryGuardSingleInstanceMemoryLimit(longint lValue);
	longint GetMemoryGuardSingleInstanceMemoryLimit() const;

	////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalites de base

	// Verification de la validite des specifications
	// (classe et valeur de selection)
	boolean Check() const override;
	virtual boolean CheckPartially(boolean bWriteOnly) const;

	// Verification du format de la base
	// A redefinir dans les sous-classes
	virtual boolean CheckFormat() const;

	// Mode d'affichage des messages lors de l'analyse des fichiers (defaut: true)
	// Les erreurs sont affichees quoi qu'il arrivent.
	// Les warning et messages sont inhibees en mode non verbeux, en complement avec
	// le suivi des taches gere par la classe TaskManager
	virtual void SetVerboseMode(boolean bValue);
	virtual boolean GetVerboseMode() const;

	// Mode silencieux, pour inhiber tout affichage de message, verbeux ou non (defaut: false)
	virtual void SetSilentMode(boolean bValue);
	virtual boolean GetSilentMode() const;

	// Redefinition des methodes de gestion des erreurs pour tenir compte du mode d'affichage
	void AddSimpleMessage(const ALString& sLabel) const override;
	void AddMessage(const ALString& sLabel) const override;
	void AddWarning(const ALString& sLabel) const override;
	void AddError(const ALString& sLabel) const override;

	// Etat d'avancement pour le suivi de taches de lecture ayant collecte le nombre de records (lecture physique)
	// et d'objets Methode avancee
	void DisplayReadTaskProgressionLabel(longint lRecordNumber, longint lObjectNumber);

	// Memoire utilisee par la database pour son fonctionnement,
	// dans son etat actuel, y compris les objets charge en memoire par un ReadAll
	// Memoire de base, buffers de lecture, objets references potentiels...
	longint GetUsedMemory() const override;

	// Estimation de la taille memoire minimum necessaire pour ouvrir la base, en lecture ou en ecriture
	// Methode avancee uniquement (non en prerequis de l'ouverture effective des bases)
	// Prend en compte la taille des dictionnaires, en lecture (classe et classe physique) ou
	// ecriture (classe uniquement).
	// Le bIncludingClassMemory indique s'il faut prendre en compte la classe courante, qui
	// est deja chargee dans dans le domaine courant (mais utile pour estimation des resource d'un
	// esclave en mode parallele)
	// La base ne doit pas etre ouverte en lecture ou ecriture, mais les classes doivent etre disponibles
	// A completer dans les sous-classes
	virtual longint ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory);

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON);

	// Libelles utilisateurs
	// Le libelle de l'objet contient le nom de la base et le numero de record s'il y en a un
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////
	// Methodes de test

	// Creation d'objets dans une classe (cree elle meme si necessaire)
	void TestCreateObjects(int nNumber);

	// Alimentation d'objet par lecture
	void TestRead();

	/////////////////////////////////////////////////
	// Administration des technologies de KWDatabase
	// L'objectif est de pouvoir implementer un service d'apprentissage utilisant une base de donnees
	// independamment de sa technologie
	// Il suffit pour cela de passer par un constructeur virtuel (par CreateDefaultDatabaseTechnology)
	// au lieu de faire une creation "en dur" (par new)

	// Nom de la technologie a utiliser par defaut ("" par defaut)
	static void SetDefaultTechnologyName(const ALString& sTechnologyName);
	static const ALString GetDefaultTechnologyName();

	// Creation d'une base pour la technologie par defaut
	// Prerequis: la technologie correspondant doit etre enregistree
	static KWDatabase* CreateDefaultDatabaseTechnology();

	// Enregistrement dans la base des technologies
	// Il ne doit pas y avoir deux technologies enregistrees avec le meme nom
	// Memoire: les technologies enregistrees sont gerees par l'appele
	static void RegisterDatabaseTechnology(KWDatabase* database);

	// Recherche par nom de technologie
	// Retourne NULL si absent
	static KWDatabase* LookupDatabaseTechnology(const ALString& sTechnologyName);

	// Recherche par nom de technologie et duplication
	// Permet d'obtenir une base de donnees prete a etre instanciee
	// Retourne NULL si absent
	static KWDatabase* CloneDatabaseTechnology(const ALString& sTechnologyName);

	// Export de toutes les technologies enregistrees
	// Memoire: le contenu du tableau en retour appartient a l'appele
	static void ExportAllDatabaseTechnologies(ObjectArray* oaDatabases);

	// Destruction de toutes les technologies enregistrees
	static void DeleteAllDatabaseTechnologies();

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametrage des meta-donnees de format en utilisant des informations de reconnaissance de types
	// Warning eventuels si probleme de coherences entre differents types reconnus
	void AddFormatMetaData(KWAttribute* sourceAttribute,
			       const KWTypeAutomaticRecognition* typeAutomaticRecognition) const;

	////////////////////////////////////////////////////////////////////////////
	// Gestion des attributs Loaded derives, utilisant des attributs non Loaded
	// Une classe physique est introduite pour la lecture dans la base de donnees
	// de tous les attributs necessaires au calcul des attributs Loaded.
	// Tous les attributs de la classe physique sont en Loaded, ce qui fait que la
	// premiere partie des valeurs physique coincide exactment avec les attribut
	// Loaded initiaux. Cette propriete est utilisee pour "muter" les objets
	// physique tres rapidement apres lecture dans la base.
	// Effet de bord: la classe physique peut coincider avec la classe initiale,
	// ce qui simplifie encore la mutation.

	////////////////////////////////////////////////////////////////////////////
	// Gestion de la classe physique

	// Construction de la classe physique
	virtual void BuildPhysicalClass();

	// Destruction de la classe physique
	virtual void DeletePhysicalClass();

	// Mutation d'un objet physique en objet logique
	// Propagation aux sous-objets, destruction des sous-objets inutilises
	virtual void MutatePhysicalObject(KWObject* kwoPhysicalObject) const;

	// Compilation des informations de selection des objets physiques
	void CompilePhysicalSelection();

	// Test si un objet est selectionne (pour base en etat IsOpenedForRead())
	virtual boolean IsPhysicalObjectSelected(KWObject* kwoPhysicalObject);

	////////////////////////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes
	// Les implementation par defaut ne font rien: la base peut s'ouvrir ou se
	// fermer, mais les lecture/ecriture sont sans effet.

	// Initialisation des caracteristiques de la classe a partir de la base
	// de donnees.
	// Seul la structure de la classe est initialisee a partir du schema de la base,
	// qui peut etre ouverte puis fermee pour l'occasion.
	// La classe passee en parametre n'a initialement aucun champ. Ceux-ci
	// doivent etre initialises par la methode.
	// Si cette fonctionnalite n'est pas disponible, la methode n'est pas a
	// reimplementer (par defaut: ne fait rien et renvoie false).
	// Retourne true si la classe a ete construite sans erreurs
	virtual boolean BuildDatabaseClass(KWClass* kwcDatabaseClass);

	// Indique si la construction de base initialise le type des attributs
	// Retourne true si le type des attributs a ete iniatialise d'apres le schema.
	// Retourne false si seul le libelle des attributs a ete initialise (auquel cas,
	// les types rendus doivent etre Symbol, et la methode logique appelante lira
	// quelques instances pour initialiser les types)
	// Par defaut: false
	virtual boolean IsTypeInitializationManaged() const;

	// Ouverture physique de la base de donnees pour lecture
	// Les classe logique et physique sont initialisees prealablement
	// En cas de succes, la base doit etre preparee pour la lecture
	// d'un objet physique. Les eventuelles donnees internes permettant de
	// parametrer l'alimentation d'objets physiques doivent etre initialisees
	// dans cette methode
	// En cas d'echec, c'est la methode appelante OpenForRead qui nettoie
	virtual boolean PhysicalOpenForRead();

	// Ouverture physique de la base de donnees pour ecriture
	// La classe logique est initialisee prealablement
	// En cas de succes, la base doit etre preparee pour l'ecriture
	// d'un objet physique. Les eventuelles donnees internes permettant de
	// parametrer l'ecriture des objets logiques doivent etre initialisees
	// dans cette methode
	// En cas d'echec, c'est la methode appelante OpenForWrite qui nettoie
	virtual boolean PhysicalOpenForWrite();

	// Test de fin de base (cas Read)
	virtual boolean IsPhysicalEnd() const;

	// Lecture d'une instance (en utilisant la classe physique)
	// Renvoie NULL si pas possibilite de produire un objet physique valide,
	// et emet un message d'erreur dans ce cas
	// Renvoie egalement NULL si interruption utilisateur, mais sans message
	virtual KWObject* PhysicalRead();

	// Lecture sans production d'un objet physique, pour sauter un enregistrement
	virtual void PhysicalSkip();

	// Ecriture d'une instance (de la classe initiale)
	virtual void PhysicalWrite(const KWObject* kwoObject);

	// Fermeture de la base
	// Renvoie false si erreur grave avant ou pendant la fermeture, true sinon
	virtual boolean PhysicalClose();

	// Destruction de la base
	// Utile en cas d'erreur d'ecriture, pour tout nettoyer correctement
	virtual void PhysicalDeleteDatabase();

	// Nombre approximatif d'objets presents dans la base de donnees
	// Potentiellement base sur une estimation heuristique, sans lecture du fichier
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual longint GetPhysicalEstimatedObjectNumber();

	// Estimation du pourcentage d'avancement de la lecture d'un fichier
	// Methode a priori rapide, sans effet important sur le temps de lecture
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual double GetPhysicalReadPercentage();

	// Index d'enregistrement physique, pour localiser les erreurs
	virtual longint GetPhysicalRecordIndex() const;

	// Collecte de messages de stats sur les enregistrements (lus ou ecrits) au niveau physique
	// Remis a zero uniquement lors des ouvertures de fichiers, donc disponible a tout moment
	// entre l'ouverture et la fermeture des fichiers
	// Messages non redondants avec le niveau logique
	// Memoire: le tableau, vide initialement, et son contenu (object Error) appartient a l'appelant
	virtual void CollectPhysicalStatsMessages(ObjectArray* oaPhysicalMessages);

	// Affichage des messages physiques
	virtual void DisplayPhysicalMessages(ObjectArray* oaPhysicalMessages);

	// Classe gerant les objets charges
	// Cette classe n'est presente que temporairement, lors d'une phase de
	// lecture ou d'ecriture. Elle est donc initialisee lors d'une ouverture,
	// utilisee dans les lectures/ecritures, puis remise a NULL a la fermeture.
	KWClass* kwcClass;

	// Fraicheur de la classe en cours d'utilisation
	int nClassFreshness;

	// Classe physique
	// Meme remarque que pour kwcClass, mais uniquement en lecture.
	KWClass* kwcPhysicalClass;

	// Dictionnaire des attributs natifs Object ou ObjectArray a garder lors des mutations d'objet
	// Utile dans le cas multi-table
	// Identifie les attributs de kwcClass natif Object ou ObjectArray, non utilises, mais referencables
	// par des regles de derivation. Ils ne doivent pas etre detruits pour que les attributs calcules
	// referencent des objects existants
	NumericKeyDictionary nkdUnusedNativeAttributesToKeep;

	// Objets charges en memoire
	ObjectArray oaAllObjects;

	// Attributs principaux
	ALString sClassName;
	ALString sDatabaseName;
	boolean bOpenedForRead;
	boolean bOpenedForWrite;
	boolean bVerboseMode;
	boolean bSilentMode;
	boolean bIsError;

	// Gestion des echantillons
	boolean bModeExcludeSample;
	double dSampleNumberPercentage;
	ALString sSelectionAttribute;
	ALString sSelectionValue;
	KWLoadIndex liSelectionAttributeLoadIndex;
	int nSelectionAttributeType;
	Symbol sSelectionSymbol;
	Continuous cSelectionContinuous;
	IntVector ivMarkedInstances;

	// Memorisation de l'etat de suivi des taches
	PeriodicTest periodicTestInterruption;
	PeriodicTest periodicTestDisplay;

	// Formats par defaut des types complexes, pour gerer les conversions vers les chaines de caracteres
	KWDateFormat dateDefaultConverter;
	KWTimeFormat timeDefaultConverter;
	KWTimestampFormat timestampDefaultConverter;
	KWTimestampTZFormat timestampTZDefaultConverter;

	// Service de protection memoire pour gerer les enregistrement trop volumineux, notamment dans le cas
	// multi-tables ou le nombre d'enregistrements secondaires peut etre tres important
	mutable KWDatabaseMemoryGuard memoryGuard;

	// Administration des technologies de bases de donnees (objets KWDatabase)
	static ALString* sDefaultTechnologyName;
	static ObjectDictionary* odDatabaseTechnologies;
};

//////////////////////////////////////////////////////////////////////
// Methodes en inline

inline boolean KWDatabase::IsOpenedForRead() const
{
	assert(not bOpenedForRead or not bOpenedForWrite);
	require(not bOpenedForRead or not bOpenedForWrite or
		(kwcClass != NULL and kwcClass->GetFreshness() == nClassFreshness));
	return bOpenedForRead;
}

inline boolean KWDatabase::IsOpenedForWrite() const
{
	assert(not bOpenedForRead or not bOpenedForWrite);
	require(not bOpenedForRead or not bOpenedForWrite or
		(kwcClass != NULL and kwcClass->GetFreshness() == nClassFreshness));
	return bOpenedForWrite;
}

inline boolean KWDatabase::IsEnd() const
{
	require(GetClassName() != "");
	require(kwcClass != NULL);
	require(kwcClass->GetFreshness() == nClassFreshness);
	require(IsOpenedForRead());
	return IsPhysicalEnd();
}

inline boolean KWDatabase::IsError() const
{
	return bIsError;
}

inline void KWDatabase::Write(const KWObject* kwoObject)
{
	require(GetClassName() != "");
	require(kwcClass != NULL);
	require(kwcClass->GetFreshness() == nClassFreshness);
	require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass);
	require(IsOpenedForWrite());
	require(kwcPhysicalClass == NULL);

	// Ecriture physique de l'objet
	PhysicalWrite(kwoObject);
}

inline ObjectArray* KWDatabase::GetObjects()
{
	return &oaAllObjects;
}

inline longint KWDatabase::GetEstimatedObjectNumber()
{
	longint lPhysicalEstimatedObjectNumber;
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	lPhysicalEstimatedObjectNumber = GetPhysicalEstimatedObjectNumber();
	return lPhysicalEstimatedObjectNumber;
}

inline longint KWDatabase::GetSampleEstimatedObjectNumber()
{
	longint lEstimatedObjectNumber;
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	lEstimatedObjectNumber = GetEstimatedObjectNumber();
	return (GetModeExcludeSample() ? (longint)(lEstimatedObjectNumber * (100 - GetSampleNumberPercentage()) / 100)
				       : (longint)(lEstimatedObjectNumber * GetSampleNumberPercentage() / 100));
}

inline double KWDatabase::GetReadPercentage()
{
	require(IsOpenedForRead());
	require(not IsOpenedForWrite());
	return GetPhysicalReadPercentage();
}

inline void KWDatabase::SetSampleNumberPercentage(double dValue)
{
	require(0 <= dValue and dValue <= 100);

	dSampleNumberPercentage = dValue;
}

inline double KWDatabase::GetSampleNumberPercentage() const
{
	return dSampleNumberPercentage;
}

inline void KWDatabase::SetModeExcludeSample(boolean bValue)
{
	bModeExcludeSample = bValue;
}

inline boolean KWDatabase::GetModeExcludeSample() const
{
	return bModeExcludeSample;
}

inline void KWDatabase::SetSamplingMode(const ALString& sValue)
{
	require(CheckSamplingMode(sValue));
	bModeExcludeSample = (sValue == "Exclude sample");
}

inline const ALString KWDatabase::GetSamplingMode() const
{
	if (bModeExcludeSample)
		return "Exclude sample";
	else
		return "Include sample";
}

inline boolean KWDatabase::CheckSamplingMode(const ALString& sValue)
{
	return sValue == "Exclude sample" or sValue == "Include sample";
}

inline boolean KWDatabase::IsEmptySampling() const
{
	return (GetSampleNumberPercentage() == 0 and not GetModeExcludeSample()) or
	       (GetSampleNumberPercentage() == 100 and GetModeExcludeSample());
}

inline void KWDatabase::SetSelectionAttribute(const ALString& sValue)
{
	sSelectionAttribute = sValue;
	liSelectionAttributeLoadIndex.Reset();
	nSelectionAttributeType = KWType::Unknown;
}

inline const ALString& KWDatabase::GetSelectionAttribute() const
{
	return sSelectionAttribute;
}

inline void KWDatabase::SetSelectionValue(const ALString& sValue)
{
	sSelectionValue = sValue;
}

inline const ALString& KWDatabase::GetSelectionValue() const
{
	return sSelectionValue;
}

inline IntVector* KWDatabase::GetMarkedInstances()
{
	return &ivMarkedInstances;
}

inline longint KWDatabase::GetLastReadMarkIndex() const
{
	return GetPhysicalRecordIndex();
}

inline void KWDatabase::SetVerboseMode(boolean bValue)
{
	bVerboseMode = bValue;
}

inline boolean KWDatabase::GetVerboseMode() const
{
	return bVerboseMode;
}

inline void KWDatabase::SetSilentMode(boolean bValue)
{
	bSilentMode = bValue;
}

inline boolean KWDatabase::GetSilentMode() const
{
	return bSilentMode;
}