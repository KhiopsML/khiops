// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabase.h"
#include "KWMTDatabaseMapping.h"
#include "KWDataTableDriver.h"

#include "KWDRReference.h"
#include "KWObjectKey.h"

//////////////////////////////////////////////////////////////////////
// Base multi-table (MTDatabase: Multiple Table Database)
class KWMTDatabase : public KWDatabase
{
public:
	// Constructeur
	KWMTDatabase();
	~KWMTDatabase();

	// Creation dynamique
	KWDatabase* Create() const override;

	// Recopie des attributs de definition
	void CopyFrom(const KWDatabase* kwdSource) override;

	// Comparaison des attributs de definition avec une autre base du meme type
	int Compare(const KWDatabase* kwdSource) const override;

	/////////////////////////////////////////////////////////////////////
	// Parametrage obligatoire pour l'utilisation de la base

	// Nom de la base de donnees
	void SetDatabaseName(const ALString& sValue) override;
	const ALString& GetDatabaseName() const override;

	// Nom de la classe associee
	// La classe correspondante doit exister et etre valide pour toutes
	// les operation de lecture/ecriture (sauf ComputeClass)
	void SetClassName(const ALString& sValue) override;
	const ALString& GetClassName() const override;

	///////////////////////////////////////////////////////////
	// Parametrage du mapping multi-table
	// Acces au mapping multi-table (tableau de KWMTDatabaseMapping)
	// Le premier element du tableau correspond toujours a la table principale de la base de donnees
	// Attention: utiliser ce tableau uniquement pour modifier ses elements
	// (la structure du tableau est a actualiser par la methode UpdateMultiTableMappings)
	ObjectArray* GetMultiTableMappings();

	// Nombre de tables utilises
	int GetTableNumber() const override;

	// Nombre de table principales et referencees
	int GetMainTableNumber() const;
	int GetReferencedTableNumber() const;

	// Acces a un maping par son chemin (nom d'attribut)
	KWMTDatabaseMapping* LookupMultiTableMapping(const ALString& sDataPath) const;

	// Indique si un mapping correspond a une classe referencee
	boolean IsReferencedClassMapping(const KWMTDatabaseMapping* mapping) const;

	// Mise a jour du mapping par analyse de la structure du dictionnaire
	// Creation des mappings inexistant, supression des mapping superflus,
	// on garde les mappings existants coherents
	void UpdateMultiTableMappings();

	// Methode avancee, pour acceder aux objets references
	// Ces objects sont disponible uniquement le temps d'une lecture de la base
	const KWObjectReferenceResolver* GetObjectReferenceSolver() const;

	// Acces au driver de table utilise pour les acces aux fichiers
	KWDataTableDriver* GetDataTableDriver() override;

	// Verification de la validite des specifications
	// (mapping et cle des dictionnaires concernes)
	boolean Check() const override;
	boolean CheckPartially(boolean bWriteOnly) const override;

	// Verification du format de la base
	boolean CheckFormat() const override;

	// Reimplementation de la methode de parametrage du mode d'affichage des messages
	// Propagation au tables mappees
	void SetVerboseMode(boolean bValue) override;
	void SetSilentMode(boolean bValue) override;

	// Memoire utilisee par la database pour son fonctionnement
	longint GetUsedMemory() const override;

	// Memoire necessaire pour ouvrir la base
	longint ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Methode avancee, utilisee uniquement en mode debug avant ouverture en lecture d'une base
	// Verification de la coherence des dictionnaires en mode lecture, pour les attributs calcules de type Object et
	// ObjectArray Ces attribut ne peuvent etre calcules qu'a partir d'attribut natifs devant toujours etre presents
	// (Used), sous peine de referencer des objets potentiellement detruits (parce qu'Unused) lors de la mutation
	// des objets physiques en objet logiques
	boolean CheckObjectConsistency() const;
	boolean CheckObjectRuleConsistency(KWDerivationRule* rule) const;

	// Creation de driver, a l'usage des mappings des tables principales et secondaires
	virtual KWDataTableDriver* CreateDataTableDriver(KWMTDatabaseMapping* mapping) const;

	// Reimplementation des methodes virtuelles de KWDatabase
	boolean BuildDatabaseClass(KWClass* kwcDatabaseClass) override;
	boolean IsTypeInitializationManaged() const override;
	boolean PhysicalOpenForRead() override;
	boolean PhysicalOpenForWrite() override;
	boolean IsPhysicalEnd() const override;
	KWObject* PhysicalRead() override;
	void PhysicalSkip() override;
	void PhysicalWrite(const KWObject* kwoObject) override;
	boolean PhysicalClose() override;
	void PhysicalDeleteDatabase() override;
	longint GetPhysicalEstimatedObjectNumber() override;
	double GetPhysicalReadPercentage() override;
	longint GetPhysicalRecordIndex() const override;
	void CollectPhysicalStatsMessages(ObjectArray* oaPhysicalMessages) override;

	// Lecture apres la fin de la base, pour detecter d'eventuelles erreurs
	// Necessaire par exemple en multi-tables pour detecter les eventuelles erreurs (ligne orphelines, probleme de
	// tri...)
	void PhysicalReadAfterEndOfDatabase();

	// Construction de la classe physique
	// Completion en identifiant les attributs natif Object ou ObjecArray utilises par des regles
	// de derivation et ne devant pas etre detruit suite a leur traitement par la classe physique.
	// Ces attributs natifs spnt geres dans les "UnusedNative...Attribute" de la classe (KWClass),
	// qui, lors de la compilation, prevoit un emplacement memoire systematique pour les stocker
	// et assurer leur memorisation au cas ou ils seraient referencables par des regles de derivation.
	// La KWMTDatabase determine les attribut natifs non utilise a garder, pour piloter efficacement
	// la methode de mutation des object physiques
	void BuildPhysicalClass() override;

	// Calcul du dictionnaire des attributs natifs inutilises a garder
	void ComputeUnusedNativeAttributesToKeep(NumericKeyDictionary* nkdAttributes);
	void ComputeUnusedNativeAttributesToKeepForRule(NumericKeyDictionary* nkdAttributes,
							NumericKeyDictionary* nkdAnalysedRules, KWDerivationRule* rule);

	// Destruction de la classe physique
	// Completion en dereferencant les dictionnaire depuis le mapping et en nettoyant le dictionnaire
	// des attributs natifs Object ou ObjectArray a detruire
	void DeletePhysicalClass() override;

	// Mutation d'un objet physique en objet logique
	// Gestion du referencement des objets
	void MutatePhysicalObject(KWObject* kwoPhysicalObject) const override;

	// Test si un objet est selectionne
	// Gestion du referencement des objets
	boolean IsPhysicalObjectSelected(KWObject* kwoPhysicalObject) override;

	// Creation recursive d'un mapping
	// Le mapping est chaine avec ses mappings composant.
	// Le tableaux mapping exhaustifs est egalement egalement mis a jour
	// Les classes referencees sont memorisees dans un dictionnaire, pour gerer les mappings externes a creer
	// ulterieurement Les mappings crees recursivement sont memorises dans un tableau
	KWMTDatabaseMapping* CreateMapping(ObjectDictionary* odReferenceClasses, ObjectArray* oaRankedReferenceClasses,
					   KWClass* mappedClass, const ALString& sDataPathClassName,
					   const ALString& sDataPathAttributeNames, ObjectArray* oaCreatedMappings);

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion des objets natifs references
	// Les objets natifs references sont les objets (principaux, et non composants)
	// appartenant aux classes racines des tables secondaires.
	// Leurs mappings racines sont definis dans le tableau oaRootRefTableMappings.
	// A l'ouverture de la base, tous les objets racines referenceables sont charges en memoire
	// dans leur representation physique, et stockees dans une base memoire, permettant
	// de rechercher efficacement un objet d'une classe donnee par sa cle.
	// Cette base memoire est passee temporairement en parametre a la regle de derivation
	// predefinie KWDRReference, ce qui permet d'utiliser le mecanisme de base des regles
	// de derivation pour resoudre les references aux object racines des autres classes

	// Lecture et chargement en memoire des tables des objets references
	boolean PhysicalReadAllReferenceObjects();

	// Destruction des objets references
	void PhysicalDeleteAllReferenceObjects();

	// Calcul de la memoire necessaire pour le chargement des objets references
	longint ComputeNecessaryMemoryForReferenceObjects();

	/////////////////////////////////////////////////////////////////////////////////
	// Bibliotheques de services associees aux mappings
	// Ces methodes se comportent comme des methodes de la classe KWMTDatabaseMapping
	// en etant toutes prefixees par DMTM et prenant comme premier argument un mapping.
	// L'implementation est neanmoins dans la classe courante, d'une part parce qu'elle exige
	// parfois des donnees de la classe courante "globales" a l'ensemble des ampping, d'autre
	// part pour faciliter la maintenance du code source en localisant en un meme endroit
	// l'implmentation des bases multi-tables. La classe KWMTDatabaseMapping sert
	// essentiellement a memoriser les donnees, et a parmettre leur edition a l'interface

	// Initialisation et terminaison d'un mapping physique
	// en creant les driver de bases de donnees pour chaque mapping (bRead: true pour Read, false pour Write)
	// Propagation recursive sur les sous-mapping de la composition
	void DMTMPhysicalInitializeMapping(KWMTDatabaseMapping* mapping, KWClass* mappedClass, boolean bRead);
	void DMTMPhysicalTerminateMapping(KWMTDatabaseMapping* mapping);

	// Ouverture et fermeture d'une table de mapping
	// Propagation recursive sur les sous-mapping de la composition
	boolean DMTMPhysicalOpenForRead(KWMTDatabaseMapping* mapping, const KWClass* kwcLogicalClass);
	boolean DMTMPhysicalOpenForWrite(KWMTDatabaseMapping* mapping);
	boolean DMTMPhysicalClose(KWMTDatabaseMapping* mapping);

	// Destruction recursive des fichiers mappes
	void DMTMPhysicalDeleteDatabase(KWMTDatabaseMapping* mapping);

	// Lecture et ecriture dans une table de mapping
	// Propagation recursive sur les sous-mapping de la composition
	// Renvoie NULL (sans message) si interruption utilisateur
	KWObject* DMTMPhysicalRead(KWMTDatabaseMapping* mapping);
	void DMTMPhysicalWrite(KWMTDatabaseMapping* mapping, const KWObject* kwoObject);

	/////////////////////////////////////////////////
	// Attributs de l'implementation

	// Mapping racine pour la table principale
	// Ce mapping doit toujours etre present et contient le parametrage (nom de base, nom de classe) principal
	// L'utilisation d'un mapping pour la racine permet d'unifier les traitement entre tables principales
	// et tables secondaires
	mutable KWMTDatabaseMapping* rootMultiTableMapping;

	// Mapping racine des tables secondaires (reference a un sous-ensemble des mapping du tableau
	// oaMultiTableMappings)
	mutable ObjectArray oaRootRefTableMappings;

	// Mapping multi-tables: tableau exhaustif de tous les mappings (permet une interface d'edition "a plat")
	// Attention, le mapping racine est toujours integree comme premier element du tableau,
	// et ne doit toujours etre synchronise
	mutable ObjectArray oaMultiTableMappings;

	// Nombre d'enregistrements ayant ete sautes (Skip)
	// On ne peut faire des tests d'integrite sur les lignes de tables secondaires non associees
	// a la table principale que si aucun enregistrement principal n'a ete saute
	int nSkippedRecordNumber;

	// Gestionnaire de refence pour les objets references des tables secondaires
	mutable KWObjectReferenceResolver objectReferenceResolver;

	// Driver de gestion des acces a une table
	// Template pour la creation de driver du bon type
	KWDataTableDriver* dataTableDriverCreator;
};
