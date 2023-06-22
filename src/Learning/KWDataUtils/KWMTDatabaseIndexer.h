// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWMTDatabaseTextFile.h"
#include "PLMTDatabaseTextFile.h"
#include "KWKeyPositionSampleExtractorTask.h"
#include "KWKeyPositionFinderTask.h"
#include "KWFileIndexerTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWMTDatabaseIndexer
// Service d'indexation de l'ensemble des tables d'une base de donnees multi-tables
// Permet ensuite de piloter les taches parallele ayant une base multi-table en entree,
// pour lire les enregisretements des tables principales et secondaires de facon
// synchronisee selon des plages de cle
class KWMTDatabaseIndexer : public Object
{
public:
	// Constructeur
	KWMTDatabaseIndexer();
	~KWMTDatabaseIndexer();

	// Indexation d'une base de donnes multi-tables pour une base multi-table dont les informations
	// liees a l'ouverture ont ete calculee, pour un nombre max d'esclave donne et une memoire par esclave
	// reservee a la gestion de la base source
	// Le dernier parametre, optionnel, n'est pris en compte que s'il est different de 0. Il permet
	// de parametrer un taille max de fichier a traiter par processus
	boolean ComputeIndexation(const PLMTDatabaseTextFile* sourceMTDatabase, int nSlaveNumber,
				  longint lSlaveGrantedMemoryForSourceDatabase,
				  longint lForcedMaxTotalFileSizePerProcess);

	// Indique si la tache a ete interrompue par l'utilisateur
	boolean IsInterruptedByUser() const;

	// Indique si la tache s'est executee correctement
	boolean IsComputed() const;

	// Nettoyage de l'indexation
	void CleanIndexation();

	///////////////////////////////////////////////////////////////////////////////
	// Service disponible apres indexation
	// Pour chaque chunk, on a acces aux information suivante pour chaque table
	// de la base multi-table en entree
	//  . debut de position du chunk par table
	//  . fin de position du chunk par table
	//  . index du premier record du chunk par table
	//  . derniere cle racine du chunk

	// Nombre total de chunk
	int GetChunkNumber() const;

	// Vecteur des debuts de position du chunk par table
	const LongintVector* GetChunkBeginPositionsAt(int nChunkIndex) const;

	// Vecteur des fin de position du chunk par table
	const LongintVector* GetChunkEndPositionsAt(int nChunkIndex) const;

	// Vecteur des index de premier record du chunk par table
	const LongintVector* GetChunkBeginRecordIndexesAt(int nChunkIndex) const;

	// Derniere cle racine du chunk
	const KWKey* GetChunkLastRootKeyAt(int nChunkIndex) const;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul du plan d'indexation des tables du schema, a partir de la variable partagee de la base en entree
	// On calcule d'abord les index de champ de chaque table en entree, ce qui permet de savoir quelle table doit
	// etre lue. On collecte un echantillon de cles de la table principale, ainsi que leur position pour chaque
	// table On determine alors un decoupage des tables pour les traitement par esclave En sortie, si OK, on a donc
	// initialise les tableaux oaAllChunksBeginPos, oaAllChunksBeginRecordIndex, et oaAllChunksLastRootKeys

	// Pilotage du calcul du plan d'indexation global
	// Erreur possible si pas assez de ressource par exemple
	boolean ComputeAllDataTableIndexation(int nSlaveNumber, longint lSlaveGrantedMemoryForSourceDatabase,
					      longint lForcedMaxTotalFileSizePerProcess);

	// Extraction d'un echantillon de cles et de leur position pour la table principale en lecture
	// dans le cas particulier d'une table princpale sans cle
	// On obtient en sortie un tableau de KWKeyPosition
	boolean ExtractMonoTableRootSampleKeyPositions(ObjectArray* oaRootKeyPositions);

	// Initialisation d'un indexeur de champ cle pour un mapping en entree
	// On n'indexe que les champs cles secondaire correspondant a ceux de la table principale,
	// Les champs cles secondaire sont potentiellement en positions differentes, de nom different
	// et en nombre supperieur a ceux de la table racine. On n'indexe que ceux correspond
	// a la table racine (en meme nombre et dans el meme ordre)
	boolean InitializeKeyFieldIndexer(const KWMTDatabaseMapping* readMapping, KWKeyFieldsIndexer* keyFieldsIndexer);

	// Evaluation de la taille des cles et du nombre de lignes de la table racine
	boolean EvaluateKeySize(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint& lRootMeanKeySize,
				longint& lRootLineNumber);

	// Extraction d'un echantillon de cles et de leur position pour la table principale en lecture
	// On obtient en sortie un tableau de KWKeyPosition
	boolean ExtractRootSampleKeyPositions(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint lRootMeanKeySize,
					      longint lRootLineNumber, double dSamplingRate,
					      ObjectArray* oaRootKeyPositions);

	// Extraction des positions des cles pour chaque table secondaire en lecture
	// On obtient en sortie un tableau de KWKeyPosition pour chaque index oaAllTableFoundKeyPositions correspondant
	// a un mapping non NULL dans le tableau en entree
	// L'attribut Key des objets KWKeyPosition en sortie est nettoyee pour gagner en espace memoire
	boolean ExtractSecondaryKeyPositions(const ObjectArray* oaRootKeys, const ObjectArray* oaUsedReadMappings,
					     ObjectArray* oaAllTableFoundKeyPositions);

	// Calcul des informations de decoupage des tables en troncon
	// Pour eviter que toutes les premieres taches demandent un acces disque simultanement, les premiers chunks sont
	// decoupes avec des tailles legerement differentes.
	// Pour favoriser la fin des taches simultanement, les derniers chunks sont decoupes plus petits.
	boolean ComputeAllChunksInformations(const ObjectArray* oaRootKeys,
					     const ObjectArray* oaAllTableFoundKeyPositions,
					     const ObjectArray* oaUsedReadMappings,
					     const LongintVector* lvDataTableSizes, int nSlaveNumber,
					     longint lMaxFileSizePerProcess);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tableau des vecteurs de position successifs pour les fichiers de la base multi-table en entree
	// Chaque vecteur de position est indexe comme les mapping de la base multi-table (les mapping des
	// tables externes, non utilises, sont ignores, avec des positions restant a 0 dans tous les vecteurs)
	//  - le premier vecteur contient les positions de depart (des 0 partout)
	//  - les vecteur intermediaires contiennent des position coherentes entre-elles
	//    (enregistrement des memes individus racines)
	//  - le dernier vecteur contient les position finales non comprises (les tailles de fichier)
	// Chaque esclave devra traiter les fichiers selon un troncon coherent en termes de cle:
	// les enregistrements secondaires ont tous une cle strictement superieure a la cle racine du troncon precedent,
	// et inferieure ou agele a la derniere cle du racine du troncon.
	// Chaque troncon est defini (par table effcetivement utilisee en lecture) par:
	//  . une position de debut (comprise) et de fin (non comprise)
	//  . l'index de la derniere ligne du troncon precedent
	//  . la derniere cle racine du troncon precedent
	// En cas de doublon dans la table racine, la derniere cle racine du troncon precedent permet de les detecter.
	// En cas d'enregistrement secondaire orphelin (sans renregistrement racine associe), ils sont detectes
	// localement a chaque troncon, car de cle inferieure aux cles racines du troncon (sauf potentiellement
	// pour le dernier troncon: ils seront ignores s'ils sont de cle superieure a la derniere cle racine du
	// fichier). Un troncon peut etre volumineux et traite via plusieurs buffers consecutifs par table (cf
	// PLDataTableDriverTextFile)

	// Tableau des vecteurs positions de debuts (LongintVector) par table
	// La taille du tableau est le nombre de troncons plus un (les positions de fin)
	ObjectArray oaAllChunksBeginPos;

	// Tableau des vecteurs d'index de record (LongintVector) par table
	ObjectArray oaAllChunksBeginRecordIndex;

	// Tableau des cles racine predente chaque troncon
	ObjectArray oaAllChunksLastRootKeys;

	// Index courant pour acceder aux vecteurs de position et d'index de record
	int nChunkCurrentIndex;

	// Flag d'interruption utilisateur (plusieurs sous-taches peuvent avoir ete interrompues)
	// Gestion pour personnaliser les messages d'erreurs dans le cas multi-tables
	boolean bIsIndexationInterruptedByUser;

	// On passe par une base multi-table parallele, ayant des services avances
	const PLMTDatabaseTextFile* sourceDatabase;
};
