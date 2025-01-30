// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"
#include "KWSortBuckets.h"
#include "KWKeyExtractor.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWKeySampleExtractorTask.h"
#include "KWArtificialDataset.h"
#include "PLFileConcatenater.h"
#include "KWChunkSorterTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWSortedChunkBuilderTask
// Decoupage d'un fichier en chunks contenant chacun une portion du fichier
// definie par un intervalle de cles.
// Les intervalles de cles etant ordonnes et adjacents, il suffira ensuite de trier
// chaque chunk et de les concatener pour obtenir une version triee du fichier.
class KWSortedChunkBuilderTask : public PLParallelTask
{
public:
	// Constructeur
	KWSortedChunkBuilderTask();
	~KWSortedChunkBuilderTask();

	/////////////////////////////////////////////////////
	// Specification du fichier d'entree

	// Fichier d'entree
	void SetFileURI(const ALString& sFileURI);
	const ALString& GetFileURI() const;

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetInputFieldSeparator(char cValue);
	char GetInputFieldSeparator() const;

	/////////////////////////////////////////////////////
	// Parametres du tri des chunks

	// Acces aux index des champs de la cle
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	/////////////////////////////////////////////////////
	// Methode principale

	// Decoupage du fichier d'entree en chunks contenant chacun une plage de cles.
	// Le parametre en entree specifie les bornes de tri (deux cles)
	// En sortie chaque bucket contient un ensemble de fichiers (les chunks)
	// Ces fichiers ne sont pas concatenes, les fichiers d'un bucket seront
	// tous charges en memoire lors de la phase de tri (tache KWChunkSorterTask)
	boolean BuildSortedChunks(const KWSortBuckets* buckets);

	// Methode de test
	static void Test();

	// Test avec un jeu de donnees artificiel deja cree
	// Alimente une structure de buckets et remplit les fichiers correspondants
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
						 const ObjectArray* oaKeySample, KWSortBuckets* sortBuckets);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Algorithme
	// Le maitre parcourt le fichier d'entree et chaque esclave traite sa portion du fichier
	// en extrayant analysant les lignes et les distribuant selon leur cle dans le chunk approprie.
	// Les esclaves ecrivent alors chaque chunk pour leur portion de fichier traites.
	// Lors de la finalisation, le maitre concatene les chunks partiels des esclaves.
	// Pour eviter un acces trop frequent au disque, chaque esclave exploite au mieux sa memoire disponible
	// pour bufferiser les acces disque.

	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Ecriture et vidage du bucket
	boolean WriteBucket(KWSortBucket* bucketToWrite);

	////////////////////////////////////////////////////
	// Variables du Master

	// Specifications en entree de la tache
	ALString sFileURI;
	boolean bHeaderLineUsed;
	boolean bLastSlaveProcessDone;
	ObjectDictionary
	    odBucketsFiles; // Dictionnaire qui pour chaque bucketId donne la liste (StringVector) de ses fichiers
	ObjectDictionary odIdBucketsSize_master; // Dictionnaire qui pour chaque bucketId donne sa taille
	longint lInputFileSize;
	longint lFilePos;

	// Definition des exigences
	longint lBucketsSizeMin;
	longint lBucketsSizeMax;
	int nReadSizeMin;
	int nReadSizeMax;
	int nReadBufferSize;

	//////////////////////////////////////////////////////
	// Variables du Slave

	// Extracteur de cle
	KWKeyExtractor keyExtractor;

	// Buckets de l'esclave
	KWSortBuckets slaveBuckets;

	// Memoire utilisee par l'ensemble des buckets
	longint lBucketsUsedMemory;

	// Dictionnaire bucket ID / bucket size
	ObjectDictionary odIdBucketsSize_slave;

	// Fichier en lecture
	InputBufferedFile inputFile;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves
	// tout au long du programme

	// Index des champs de la cle
	PLShared_IntVector shared_ivKeyFieldIndexes;

	// Tableau des buckets contenant les specification des chunks
	PLShared_ObjectArray* shared_oaBuckets;

	// Taille memoire max pour la gestion memoire des buckets de chaque esclave
	PLShared_Longint shared_lMaxSlaveBucketMemory;

	// Attributs du fichier d'entree
	PLShared_String shared_sFileName;
	PLShared_Boolean shared_bHeaderLineUsed;
	PLShared_Char shared_cInputFieldSeparator;
	PLShared_Longint shared_lFileSize;

	///////////////////////////////////////////////////////////
	// Parametres en entree et sortie des esclaves
	PLShared_Boolean input_bLastRound;
	PLShared_StringVector output_svBucketIds;
	PLShared_StringVector output_svBucketFilePath;

	// Taille du buffer en entree
	PLShared_Int input_nBufferSize;

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	// Serialization du dictionnaire bucket ID / Bucket size dans la derniere phase du SlaveProcess
	PLShared_StringVector output_svBucketIds_dictionary;
	PLShared_LongintVector output_ivBucketSize_dictionary;
};
