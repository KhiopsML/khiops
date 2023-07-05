// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseIndexer.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseChunkBuilder
// Service dediee au traitement paralles d'une base de donnees mono-table ou multi-tables
// Permet d'avoir acces aux services suivant:
//   . taille par fichier
//   . indexation des champs de l'entete du fichier pour le parsing
//   . découpage de la base en chunks de lignes contigues, synchornises
//     par plage de cle dans le cas multi-table
// Cela permet alors de piloter les taches parallele ayant une base en entree,
// pour lire les enregistrements des tables principales et secondaires de facon
// synchronisee selon des plages de cle.
// Ce service est essentiellement sous-traite a une classe KWDatabaseIndexer qui effectue
// ses calculs de facon bufferisee pour pourvoir etre utilisee par l'ensemble des taches
// traitant une meme base de donnees: BasicStats, DataPreparation, Modeling, Evaluation...
// Les lectures de base sont reduites au minimum, au plus deux fois par fichier
// avec une lecture de la premiere ligne pour parser les champs de la libe d'entete,
// et un scan des fichiers pour en deduire les chunks
// L'indexation est faite au plus une seule fois de facon fine, et des chunks sont calcules
// sur la base de cette indexation selon des contraintes dependant de chaque tache
class KWDatabaseChunkBuilder : public Object
{
public:
	// Constructeur
	KWDatabaseChunkBuilder();
	~KWDatabaseChunkBuilder();

	////////////////////////////////////////////////////////////////////
	// Parametrage, entrainant la reinitalisation des chunks potentiels

	// Parametrage d'un indexeur de base, prerequis indispensable
	// Permet de bufferiser les services necessitant des acces a la base
	// Memoire: appartient a l'appelant
	void SetDatabaseIndexer(KWDatabaseIndexer* indexer);
	KWDatabaseIndexer* GetDatabaseIndexer() const;

	// Initialisation de la base multi-table a indexer
	// Parametre en fait le DatabaseIndexer et permet d'avoir acces a ses services,
	// comme les informations liees a l'ouverture de la base
	void InitializeFromDatabase(const KWDatabase* database);

	// Indique si la base est initialisee
	boolean IsInitialized() const;

	//////////////////////////////////////////////////////////////////
	// Calcul des chunks

	// Calcul des chunks pour le parcours parallele d'une base de donnes, pour un nombre max d'esclave donne
	// et une memoire par esclave reservee a la gestion de la base source
	// Le dernier parametre, optionnel, n'est pris en compte que s'il est different de 0. Il permet
	// de parametrer un taille max de fichier a traiter par processus
	boolean BuildChunks(int nSlaveNumber, longint lSlaveGrantedMemoryForSourceDatabase,
			    longint lForcedMaxTotalFileSizePerProcess);

	// Indique si la tache s'est executee correctement
	boolean IsComputed() const;

	// Nettoyage des chunks
	void CleanChunks();

	///////////////////////////////////////////////////////////////////////////////
	// Acces au chunks construits
	// Pour chaque chunk, on a acces aux informations suivantes pour chaque table
	// dans le cas d'une base en entree
	//  . debut de position du chunk par table
	//  . fin de position du chunk par table
	//  . index du premier record du chunk par table
	//  . derniere cle racine du chunk
	// Memoire: les objet retournes appartiennent a l'appelant

	// Nombre total de chunk
	int GetChunkNumber() const;

	// Derniere cle racine du chunk precedent
	// Renvoie une cle vide pour le premier chunk
	// Renvoie une cle vide systematiquement dans le cas particulier d'une base reduite a une seule table sans cle
	void GetChunkLastRootKeyAt(int nChunkIndex, KWKey* lastRootKey) const;

	// Vecteur des index du dernier record du chunk precedent, par table
	// Renvoie un index 0 par table pour le premier chunk
	void GetChunkPreviousRecordIndexesAt(int nChunkIndex, LongintVector* lvChunkPreviousRecordIndexes) const;

	// Vecteur des positions du debut du chunk par table
	// Renvoie 0 pour le premier chunk
	// Renvoie -1 si le resultat n'est pas disponible
	void GetChunkBeginPositionsAt(int nChunkIndex, LongintVector* lvChunkBeginPositions) const;

	// Vecteur des positions du fin du chunk par table
	// Renvoie 0 pour le premier chunk
	// Renvoie -1 si le resultat n'est pas disponible
	void GetChunkEndPositionsAt(int nChunkIndex, LongintVector* lvChunkEndPositions) const;

	// Methodes simplifiees dans le cas mono-table
	longint GetChunkPreviousRecordIndexAt(int nChunkIndex) const;
	longint GetChunkBeginPositionAt(int nChunkIndex) const;
	longint GetChunkEndPositionAt(int nChunkIndex) const;

	///////////////////////////////////////////////////////////////////////////////
	// Services standard

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des informations de decoupage des tables en troncon
	// Pour eviter que toutes les premieres taches demandent un acces disque simultanement, les premiers chunks sont
	// decoupes avec des tailles legerement differentes.
	// Pour favoriser la fin des taches simultanement, les derniers chunks sont decoupes plus petits.
	void ComputeAllChunksInformations(int nSlaveNumber, longint lMaxFileSizePerProcess);

	// Indexeur de base de donnees multi-tables
	KWDatabaseIndexer* databaseIndexer;

	// Vecteur des tables utilisees, avec 1 si utilise et 1 sinon
	IntVector ivUsedTableFlags;

	// Vecteur d'index des debuts de chunks par rapport aux micro-chunks calcule par le DatanbaIndexer
	IntVector ivChunkBeginIndexes;
};