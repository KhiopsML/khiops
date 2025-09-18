// Copyright (c) 2023-2025 Orange. All rights reserved.
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

	// Acces au mapping pincipal
	KWMTDatabaseMapping* GetMainMapping() const;

	// Nombre de tables utilises
	int GetTableNumber() const override;

	// Nombre de table principales et referencees
	int GetMainTableNumber() const;
	int GetReferencedTableNumber() const;

	// Acces a un mapping par son chemin
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

	// Nombre d'erreur d'encodage detectees durant la lecture des tables externes,
	// sous-partie du nombre total d'erreur d'encodage
	// Methode disponible egalement apres la fermeture de la base
	longint GetExternalTablesEncodingErrorNumber() const;

	// Modification du nombre d'erreurs d'encodage
	// Methode avancee, utilisable par exemple lors des taches exploitant une base pour memoriser
	// cette information dans le cas ou la base est traitee par l'ensemble des esclaves
	void SetExternalTablesEncodingErrorNumber(longint lValue) const;

	// Affichage de warnings dedies au mapping multi-table
	// Ces warnings ne sont pas affiches lors du Check, pour eviter d'entrainer une
	// nuisance pour l'utilisateur par des affichage repetes
	// C'est a l'applicatif de decider quand appeler explicitement cette methode
	void DisplayMultiTableMappingWarnings() const;

	// Reimplementation de la methode de parametrage du mode d'affichage des messages
	// Propagation au tables mappees
	void SetVerboseMode(boolean bValue) override;
	void SetSilentMode(boolean bValue) override;

	// Memoire utilisee par la database pour son fonctionnement
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Memoire necessaire pour ouvrir la base, sans prendre en compte les tables externes
	// Cf. ComputeExactNecessaryMemoryForReferenceObjects
	longint ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory) override;

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
	longint GetEncodingErrorNumber() const override;
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
	double GetPhysicalReadPercentage() const override;
	longint GetPhysicalRecordIndex() const override;
	void CollectPhysicalStatsMessages(ObjectArray* oaPhysicalMessages) override;

	// Lecture apres la fin de la base, pour detecter d'eventuelles erreurs
	// Necessaire par exemple en multi-tables pour detecter les eventuelles erreurs
	// (ligne orphelines, probleme de tri...)
	void PhysicalReadAfterEndOfDatabase();

	// Destruction de la classe physique
	// Completion en dereferencant les dictionnaire depuis le mapping et en nettoyant le dictionnaire
	// des attributs natifs Object ou ObjectArray a detruire
	void DeletePhysicalClass() override;

	// Mutation d'un objet physique en objet logique
	// Gestion du referencement des objets des classes externes
	void MutatePhysicalObject(KWObject* kwoPhysicalObject) const override;

	// Test si un objet est selectionne
	// Gestion du referencement des objets des classes externes
	boolean IsPhysicalObjectSelected(KWObject* kwoPhysicalObject) override;

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion des objets natifs references
	// Les objets natifs references sont les objets (principaux, et non composants)
	// appartenant aux classes racines des tables secondaires.
	// Leurs mappings racines sont definis dans le tableau oaRootReferenceTableMappings.
	// A l'ouverture de la base, tous les objets racines referenceables sont charges en memoire
	// dans leur representation physique, et stockees dans une base memoire, permettant
	// de rechercher efficacement un objet d'une classe donnee par sa cle.
	// Cette base memoire est passee temporairement en parametre a la regle de derivation
	// predefinie KWDRReference, ce qui permet d'utiliser le mecanisme de base des regles
	// de derivation pour resoudre les references aux object racines des autres classes

	// Lecture et chargement en memoire des tables des objets references
	// On passe en entree la memoire max utilisable, et on recupere en sortie la memoire
	// effectivement utilisee pour le chargement de tous les objet references et leur traitement.
	// On renvoie egalement en sortie le nombre total d'objets lus ou crees dans l'ensemble des tables externes.
	// Attention, en entree comme en sortie, il s'agit de la memoire logique, et non physique (cf. RMResourceManager)
	// On renvoie false avec un message d'erreur en cas de depassement memoire ou de tout autre erreur
	boolean PhysicalReadAllReferenceObjects(longint lMaxAvailableMemory, longint& lNecessaryMemory,
						longint& lTotalExternalObjectNumber);

	// Destruction des objets references
	void PhysicalDeleteAllReferenceObjects();

	// Calcul de la memoire necessaire pour le chargement des objets references,
	// en chargeant l'ensemble des table externe temporairement
	// On renvoie egalement en sortie le nombre total d'objets lus ou crees dans l'ensemble des tables externes.
	boolean ComputeExactNecessaryMemoryForReferenceObjects(longint& lNecessaryMemory,
							       longint& lTotalExternalObjectNumber);

	// Calcul de la memoire necessaire pour le chargement des objets references
	// Calcul heuristique sans acces aux donnees, donnant uniquement un ordre de grandeur
	longint ComputeEstimatedNecessaryMemoryForReferenceObjects();

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

	// Comptage des erreurs d'encodage courante pour une table ouverte en lecture,
	// avec propagation recursive aux sous-mappings
	longint DMTMPhysicalComputeEncodingErrorNumber(KWMTDatabaseMapping* mapping) const;

	// Destruction recursive des fichiers mappes
	void DMTMPhysicalDeleteDatabase(KWMTDatabaseMapping* mapping);

	// Lecture et ecriture dans une table de mapping
	// Propagation recursive sur les sous-mapping de la composition
	// Renvoie NULL (sans message) si interruption utilisateur
	KWObject* DMTMPhysicalRead(KWMTDatabaseMapping* mapping);
	void DMTMPhysicalWrite(KWMTDatabaseMapping* mapping, const KWObject* kwoObject);

	/////////////////////////////////////////////////
	// Attributs de l'implementation

	// Nombre total d'erreurs d'encodage lies aux tables externes, detectees impliquant des double quotes manquants
	mutable longint lExternalTablesEncodingErrorNumber;

	// Gestionnaire des mappings de la base
	mutable KWMTDatabaseMappingManager mappingManager;

	// Fraicheur des specifications et de leur verification, pour bufferiser la methode Check
	// En theorie, la fraicheur ne capture pas de facon exhaustive toutes les modifications possibles
	// de la classe et pourrait empecher a tort une verification necessaire.
	// En pratique, la methode UpdateMultiTableMappings est appelee systematiquement a chaque changement
	// important, et la bufferisation du Check est sans risque.
	// Elle est tres utile, notamment en mode debug, ou les Check de databases multi-tables sont complexes,
	// et sont potentiellement appeles des dizaines de milliers de fois.
	int nFreshness;
	mutable int nCheckReadFreshness;
	mutable int nCheckWriteFreshness;
	mutable boolean bCheckRead;
	mutable boolean bCheckWrite;

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

///////////////////////////
// Methodes en inline

inline KWMTDatabaseMapping* KWMTDatabase::GetMainMapping() const
{
	assert(mappingManager.IsMainMappingInitialized());
	return mappingManager.GetMainMapping();
}
