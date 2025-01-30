// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLDatabaseTextFile.h"
#include "KWKeyPositionSampleExtractorTask.h"
#include "KWKeyPositionFinderTask.h"
#include "KWFileIndexerTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseIndexer
// Service d'indexation de l'ensemble des tables d'une base de donnees multi-tables
// Permet ensuite de piloter les taches parallele ayant une base multi-table en entree,
// pour lire les enregistrements des tables principales et secondaires de facon
// synchronisee selon des plages de cle
class KWDatabaseIndexer : public Object
{
public:
	// Constructeur
	KWDatabaseIndexer();
	~KWDatabaseIndexer();

	// Initialisation de la base multi-table a indexer
	// Ses informations sont memorisees  dans une version parallele de la base
	// Les informations d'ouverture de la base et d'indexation sont conservees
	// si la base presente la meme structure que la base precedente:
	// meme schema, memes fichiers, memes variables natives et memes cles
	void InitializeFromDatabase(const KWDatabase* database);

	// Indique si la base est initialisee
	boolean IsInitialized() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Information sur la base une fois les informations d'ouverture calculees

	// Indique si l'on est en technologie multi-tables, avec plusieurs table ou une seule table avec des champs cle
	boolean IsMultiTableTechnology() const;

	// Acces a la version speciale parallelisation, mono-table ou multi-tables selon le type d'initialisation
	// Memoire: appartient a l'appele
	const PLDatabaseTextFile* GetPLDatabase() const;
	const PLSTDatabaseTextFile* GetSTDatabase() const;
	const PLMTDatabaseTextFile* GetMTDatabase() const;

	// Nombre de tables
	// On renvoie le nombre total de table de la base multi-table, meme si les tables externes
	// ne sont jamais indexees
	int GetTableNumber() const;

	/////////////////////////////////////////////////////////////////////////////////
	// Parametrage en entree
	// Toute modification d'un des parametres en entree provoque le nettoyage de tous
	// les resultats d'indexation prededents

	// Nombre max d'esclaves a prendre en compte pour l'indexation
	// Ce parametre, optionnel, n'est pris en compte que s'il est different de 0
	// Sinon, on prend en compte le nombre de coeurs disponibles selon les contraintes de ressources
	void SetMaxSlaveNumber(int nValue);
	int GetMaxSlaveNumber() const;

	// Nombre d'esclave utilisable selon les ressources systemes au moment du parametrage de la base
	int GetResourceSlaveNumber() const;

	// Nombre d'esclave utilise pour le calcul de l'indexation
	// Par defaut, on prend celui donne
	int GetUsedSlaveNumber() const;

	// Taille memoire max dediee aux resultats d'indexation (defaut: 64 MB)
	void SetMaxIndexationMemory(longint lValue);
	longint GetMaxIndexationMemory() const;

	// Taille cumulee des fichiers a traiter par process
	// Ce parametre, optionnel, n'est pris en compte que s'il est different de 0,
	// dans la limite de la memoire dediees aux resultats d'indexation
	void SetMaxTotalFileSizePerProcess(longint lValue);
	longint GetMaxTotalFileSizePerProcess() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Indexation de la base de donnes multi-tables selon les parametres specifies

	// Methode d'indexation principale
	// Cette methode peut etre appelee plusieurs fois, notament si une meme base est exploitee avec des
	// des variantes de dictionnaire. Les resultats de cette methode sont bufferises pour les operations
	// sur les sous-tables non encore prises en compte dans les appel precedents.
	// Cela permet de minimiser les operations d'entrees-sorties sur les fichiers.
	// Si la tache est interrompue ou erronee, seuls les derniers resultats sont invalides
	boolean ComputeIndexation();

	// Indique si la tache a ete interrompue par l'utilisateur
	boolean IsInterruptedByUser() const;

	// Indique si la tache s'est executee correctement
	boolean IsIndexationComputed() const;

	// Nettoyage de l'indexation
	void CleanIndexation();

	///////////////////////////////////////////////////////////////////////////////
	// Resultats disponible apres indexation
	// Pour chaque chunk, on a acces aux information suivante pour chaque table
	// de la base multi-table en entree
	//  . debut de position du chunk par table
	//  . fin de position du chunk par table
	//  . index du premier record du chunk par table
	//  . derniere cle racine du chunk
	//
	// Ces resultats sont calcules de facon bufferises au fur et a mesure des calcul d'indexation
	// pour une meme database, uniquement pour la table principale et ses sous-table utilisees.
	// A minima, le nombre de chunks est disponible avec les resultats d'indexation sur la
	// table principale.
	// Les resultats d'indexation sur les tables secondaires peuvent ne pas etre disponibles s'ils n'ont
	// pas ete calcules ou si leur calcul a echoue ou a ete interrompu. Dans ce cas, les index retournes
	// sont a -1. Les resultats retournes sont a 0 pour les tables externes, qui ne sont jamais indexees.

	// Nombre total de chunk
	int GetChunkNumber() const;

	// Derniere cle racine du chunk precedent
	// Renvoie une cle vide pour le premier chunk
	// Renvoie une cle vide systematiquement dans le cas particulier d'une base reduite a une seule table sans cle
	// Memoire: appartient a l'appele
	const KWKey* GetChunkPreviousRootKeyAt(int nChunkIndex) const;

	// Index du dernier record du chunk precedent, par table
	// Renvoie 0 pour le premier chunk
	longint GetChunkPreviousRecordIndexesAt(int nChunkIndex, int nTableIndex) const;

	// Position du debut du chunk par table
	// Renvoie 0 pour le premier chunk
	// Renvoie -1 si le resultat n'est pas disponible
	longint GetChunkBeginPositionsAt(int nChunkIndex, int nTableIndex) const;

	// Position de fin du chunk par table
	// Egale a la position du debut du chunk suivant
	// Renvoie la taille du fichier par table pour le dernier chunk
	longint GetChunkEndPositionsAt(int nChunkIndex, int nTableIndex) const;

	// Verification de l'indexation si elle est calculee
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nombre de tables principales, memorise pour raisons d'optimisation
	int GetMainTableNumber() const;

	// Indication de la presence de cle dans la table principale, memorise pour raisons d'optimisation
	// En mono table, il n'y a pas de cle
	// En multi-table, on peut avoir une seule table avec des cle, et aussi une table principale sans cle
	// avec des tables externes
	boolean HasMainTableKeys() const;

	// Comparaison de la structure de deux bases: meme schema, memes fichiers, memes variables natives et memes cles
	boolean HasDatabaseSameStructure(const KWDatabase* database);

	// Comparaison de la structure de deux classes: memes variables natives et memes cles
	boolean CompareClassStructure(const KWClass* kwcClass1, const KWClass* kwcClass2);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul du plan d'indexation des tables du schema, a partir de la variable partagee de la base en entree
	// On calcule d'abord les index de champ de chaque table en entree, ce qui permet de savoir quelle table doit
	// etre lue. On collecte un echantillon de cles de la table principale, ainsi que leur position pour chaque
	// table On determine alors un decoupage des tables pour les traitement par esclave En sortie, si OK, on a donc
	// initialise les tableaux oaAllChunksBeginPos, oaAllChunksBeginRecordIndex, et oaAllChunksLastRootKeys

	// Pilotage du calcul du plan d'indexation global
	// Erreur possible si pas assez de ressource par exemple
	boolean ComputeAllDataTableIndexation();

	// Indexation basique avec un seul troncon de la table principale
	boolean ComputeRootTableBasicIndexation();

	// Extraction d'un echantillon de cles et de leur position pour la table principale
	// dans le cas particulier d'une table principale sans cle
	boolean ComputeSingleTableIndexation();

	// Extraction d'un echantillon de cles et de leur position pour la table principale
	// dans le cas d'une table princpale avec cle
	boolean ComputeRootTableIndexation();

	// Indexation basique avec un seul troncon des tables secondaires non deja traitees
	boolean ComputeSecondaryTablesBasicIndexation();

	// Indexation des tables secondaires non deja traitees
	boolean ComputeSecondaryTablesIndexation();

	// Calcul de la taille totale de fichier a traiter par process
	longint ComputeTotalFileSizePerProcess() const;

	// Initialisation d'un indexeur de champ cle pour un index de table en entree
	// On n'indexe que les champs cles secondaire correspondant a ceux de la table principale,
	// Les champs cles secondaire sont potentiellement en positions differentes, de nom different
	// et en nombre supperieur a ceux de la table racine. On n'indexe que ceux correspond
	// a la table racine (en meme nombre et dans le meme ordre)
	boolean InitializeKeyFieldIndexer(int nTableIndex, KWKeyFieldsIndexer* keyFieldsIndexer);

	// Evaluation de la taille des cles et du nombre de lignes de la table racine
	boolean EvaluateKeySize(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint& lRootMeanKeySize,
				longint& lRootLineNumber);

	// Extraction d'un echantillon de cles et de leur position pour la table principale en lecture
	boolean ExtractRootSampleKeyPositions(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint lRootMeanKeySize,
					      longint lRootLineNumber, double dSamplingRate,
					      ObjectArray* oaRootKeyPositions);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Parametrage de l'indexation

	// Nombre de tables principales
	int nMainTableNumber;

	// Indique si la table principale contient des cles
	boolean bHasMainTableKeys;

	// On passe par une base multi-table parallele, ayant des services avances
	PLDatabaseTextFile sourcePLDatabase;

	// Parametres d'indexation
	int nResourceSlaveNumber;
	int nMaxSlaveNumber;
	longint lMaxIndexationMemory;
	longint lMaxTotalFileSizePerProcess;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Resultats bruts d'extraction
	// On a des informations uniquement sur par cle extraites de la table principale.
	// Les troncon sont ensuite definis entre deux cles successives, ou entre les debuts de fichier
	// et la premiere cle, ou entre la dernier cle et les fins de fichiers

	// Tableau des cles extraites de la table principale en entree
	ObjectArray oaExtractedKeys;

	// Tableau de vecteur d'index de ligne par cle extraite
	// Pour la table principale, il s'agit de l'index de la ligne contenant la cle
	// Pour chaque table secondaire, il s'agit de l'index de la derniere ligne contenant la cle
	//  ou une cle inferieure ou egale si la cle est absente dans la table secondaire
	// Contient un vecteur NULL pour les tables secondaires non encore indexees
	ObjectArray oaTableRecordIndexVectors;

	// Position du debut de la ligne suivant la cle par table
	// Contient un vecteur NULL pour les tables secondaires non encore indexees
	ObjectArray oaTableNextRecordPositionVectors;

	// Cle vide pour les resultats
	const KWKey emptyKey;

	// Flag d'interruption utilisateur (plusieurs sous-taches peuvent avoir ete interrompues)
	// Gestion pour personnaliser les messages d'erreurs dans le cas multi-tables
	boolean bIsIndexationInterruptedByUser;
};
