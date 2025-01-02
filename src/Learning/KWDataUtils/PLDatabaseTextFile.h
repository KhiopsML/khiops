// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class PLDatabaseTextFile;
class PLShared_DatabaseTextFile;

#include "PLSTDatabaseTextFile.h"
#include "PLMTDatabaseTextFile.h"

///////////////////////////////////////////////////////////////////
// Classe PLDatabaseTextFile
//  Classe dediee aux applications paralleles manipulant des bases
//	Encapsulation des classes PLSTDatabaseTextFile et PLMTDatabaseTextFile
//  pour regrouper les services commun et pouvoir les utiliser sans se soucier
//  de la version mono-table ou multi-tables
class PLDatabaseTextFile : public Object
{
public:
	// Conctructeur
	PLDatabaseTextFile();
	~PLDatabaseTextFile();

	//////////////////////////////////////////////////////////////////////////////////////
	// Initialisation prealable, indispensable a l'utilisation des fonctionnalites

	// Initialisation en mono-table ou multi-tables
	void Initialize(boolean bIsMultiTableTechnology);

	// Initialisation en mono-table ou multi-tables selon le type de la database en parametre
	// et selon son contenu (une base multi-table reduite a une seule table sans cle est mono-table)
	void InitializeFrom(const KWDatabase* database);

	// Indique si l'objet est initialise
	boolean IsInitialized() const;

	// Remise dans l'etat non initialise
	void Reset();

	// Indique si l'on est en technologie multi-tables, avec plusieurs table ou une seule table avec des champs cle
	boolean IsMultiTableTechnology() const;

	// Acces a la version speciale parallelisation, mono-table ou multi-tables selon le type d'initialisation
	PLSTDatabaseTextFile* GetSTDatabase();
	PLMTDatabaseTextFile* GetMTDatabase();

	// Acces generique pour beneficier des services standard de KWDatabase
	KWDatabase* GetDatabase();

	// Acces en version const
	const PLSTDatabaseTextFile* GetSTDatabase() const;
	const PLMTDatabaseTextFile* GetMTDatabase() const;
	const KWDatabase* GetDatabase() const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Services dedies au dimensionnement pour la gestion des taches paralleles

	// Calcul d'informations necessaires a l'ouverture de la base en lecture ou ecriture
	//
	// Cette methode peut etre appelee plusieurs fois, notament si une meme base est exploitee avec des
	// des variantes de dictionnaire. Les resultats de cette methode sont bufferises dans la mesure du possible,
	// notamment en ce qui concerne la taille des fichiers, tous pris en compte des le premier appel,
	// ainsi que les lignes d'entete. Dans le cas multi-table, les operations sont effectuees sur les sous-tables
	// non encore prises en compte dans les appels precedents.
	// Cela permet de minimiser les operations d'entrees-sorties sur les fichiers.
	// Les elements de dimensionnement sont eux re-effectues systematiquement, car ils dependent des dictionnaires.
	boolean ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
				       PLDatabaseTextFile* outputDatabaseTextFile);
	boolean IsOpenInformationComputed() const;

	// Nettoyage des informations d'ouverture, sauf la partie bufferisee sur les tailles des fichiers et leurs
	// entete
	void CleanOpenInformation();

	// Acces a la taille cumulee de tous les fichiers en lecture, y compris les tables externes (0 sinon)
	longint GetTotalFileSize() const;

	// Acces a la taille cumulee des fichiers effectivement utilises en lecture, hors tables externes (0 sinon)
	longint GetTotalUsedFileSize() const;

	// Taille des fichiers de chaque table, index 0 uniquement en mono-table
	longint GetFileSizeAt(int nTableIndex) const;

	// Taille de buffer preferee pour l'ensemble de la base
	int GetDatabasePreferredBuferSize() const;

	// Memoire minimum necessaire pour ouvrir la base sans tenir compte des buffers
	longint GetEmptyOpenNecessaryMemory() const;

	// Memoire minimum et maximum necessaire pour ouvrir la base, en tenant compte des buffers
	longint GetMinOpenNecessaryMemory() const;
	longint GetMaxOpenNecessaryMemory() const;

	// Memoire necessaire pour stocker le fichier d'une base en sortie a partir d'une base en lecture (0 sinon)
	longint GetOutputNecessaryDiskSpace() const;

	// Nombre maximum de taches elementaires qui devront etre traitees par les esclaves
	int GetMaxSlaveProcessNumber() const;

	// Calcul de la memoire par buffer pour une memoire allouee pour l'ouverture et un nombre de process
	int ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory, int nProcessNumber) const;

	// Taille du buffer du driver : a manipuler avec precaution
	void SetBufferSize(int nSize);
	int GetBufferSize() const;

	// Affichage des messages de bilan d'un traitement de base
	//   . le libelle de la base permet de qualifier le type d'utilisation de la base ("Input database", "Output
	//   database", ...) . le libelle des records permet de qualifier le type d'utilisation des records ("Read
	//   records", "Written records", ...) . le nombre de records principal correspond au nombre d'enregistrements
	//   physiques dans la base . le nombre d'objets principal, optionnel (-1 si ignore), correspond au nombre
	//   d'objets selectionnes en lecture . les nombres de records secondaires, optionnels (NULL si ignores),
	//   correspondent au nombre d'enregistrements physiques dans la base
	//     (par base secondaire: -1 si base non ouverte)
	void DisplayAllTableMessages(const ALString& sDatabaseLabel, const ALString& sRecordLabel,
				     longint lMainRecordNumber, longint lMainObjectNumber,
				     LongintVector* lvSecondaryRecordNumbers);

	// Destruction de la base
	void PhysicalDeleteDatabase();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Initialisation en mono-table ou multi-tables selon le type de la database en parametre
	void DefaultInitializeFrom(const KWDatabase* database);

	// Initialisation forcee en mono-table, quelque soit le type de de la database en parametre
	void STInitializeFrom(const KWDatabase* database);

	// Variables de stockage de la base
	// Au plus une version, mono ou multi-table est non null
	PLSTDatabaseTextFile* plstDatabaseTextFile;
	PLMTDatabaseTextFile* plmtDatabaseTextFile;
};

///////////////////////////////////////////////////
// Classe PLShared_DatabaseTextFile
// Serialisation de la classe KWDatabaseTextFile
class PLShared_DatabaseTextFile : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DatabaseTextFile();
	~PLShared_DatabaseTextFile();

	// Acces a la base partagee
	void SetPLDatabase(PLDatabaseTextFile* plDatabase);
	PLDatabaseTextFile* GetPLDatabase();

	// Acces en tant que base standard
	KWDatabase* GetDatabase();

	// Acces a la version mono-table ou multi-tables selon son type
	PLSTDatabaseTextFile* GetSTDatabase();
	PLMTDatabaseTextFile* GetMTDatabase();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
