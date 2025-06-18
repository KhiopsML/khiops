// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"
#include "KWSortBuckets.h"
#include "KWSTDatabaseTextFile.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWKeySampleExtractorTask.h"
#include "KWSortedChunkBuilderTask.h"
#include "KWArtificialDataset.h"
#include "PLFileConcatenater.h"
#include "MemoryBufferedFile.h"

////////////////////////////////////////////////////////////
// Classe KWChunkSorterTask
// Tri parallele :
//	en entree : une liste de fichiers (les buckets, non tries, mais ordonnes par plage de rang)
//	en sortie : un ensemble ordonne de fichiers tries
// Chaque fichier de bucket doit tenir en memoire
// Les esclaves chargent un fichier different lors de chaque SlaveProcess
// Les fichiers en entree sont supprimes a la fin de la tache
// En cas d'erreur les fichiers de sortie (et les fichier d'entree) sont supprimes
//
class KWChunkSorterTask : public PLParallelTask
{
public:
	// Constructeur
	KWChunkSorterTask();
	~KWChunkSorterTask();

	/////////////////////////////////////////////////////////////////////////
	// Specification des donnees d'entree, permettant de parser les buckets

	// Utilisation d'une ligne d'entete: par defaut true
	void SetInputHeaderLineUsed(boolean bValue);
	boolean GetInputHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetInputFieldSeparator(char cValue);
	char GetInputFieldSeparator() const;

	// Index des clefs
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	////////////////////////////////////////////////////////////////////////
	// Specification du fichier trie en sortie

	// Separateur de champs utilise (par defaut: '\t')
	void SetOutputFieldSeparator(char cValue);
	char GetOutputFieldSeparator() const;

	// Resultat du tri : un ensemble de fichiers ordonnes
	const StringVector* GetSortedFiles() const;

	/////////////////////////////////////////////////////
	// Parametres principaux du tri

	// Referencement des buckets a traiter
	// Memoire: appartient a l'appelant
	void SetBuckets(KWSortBuckets* bucketsToSort);
	KWSortBuckets* GetBuckets();

	// Nombre de lignes d'un chunk (necessaire pour evaluer la memoire necessaire)
	void SetLineNumber(longint lLineNumber);
	longint GetLineNumber() const;

	// Taille des clefs (necessaire pour evaluer la memoire necessaire)
	void SetKeySize(longint lKeySize);
	longint GetKeySize() const;

	// Taille des buckets a trier (necessaire pour evaluer la memoire necessaire)
	void SetBucketSize(longint lBucketSize);
	longint GetBucketSize() const;

	/////////////////////////////////////////////////////
	// Methodes principales

	// Methode de tri parallele
	boolean Sort();

	// Nombre de lignes triees
	longint GetSortedLinesNumber();

	///////////////////////////////////////////////////////////////
	// Services divers

	// Methode de test
	static void Test();

	// Test avec un jeu de donnees artificiel deja cree
	// En sortie, un nouveau fichier trie est cree avec suffix _sorted
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
						 KWSortBuckets* bucketsToSort);

	// Calcul les exigences memoires des esclaves
	static longint ComputeSlaveMemoryRequirements(longint lBucketSize, longint lBucketLineNumber, longint lKeySize);

	// Calcul de la taille maximale des chunks qu'on peut construire si les esclaves ont lSlaveMemory de disponible
	static int ComputeMaxChunkSize(longint lKeySize, longint lSlaveMemory, longint lFileLineNumber,
				       longint lFileSize);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

protected:
	// Algorithme
	// Le maitre parcourt les fichiers des buckets, les fait trier par les esclaves,
	// puis concatene le resultat

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

	/////////////////////////////////////////////////////////////////
	// Variables du Master

	// Specification de la tache
	boolean bIsInputHeaderLineUsed;
	KWSortBuckets* buckets;
	longint lLineNumber;
	longint lKeySize;
	longint lBucketSize;

	// Index permettant le pilotage du chunk courant a trier pour la prochaine tache
	int nCurrentBucketIndexToSort;

	// Resultat du tri : un ensemble de fichiers ordonnes
	StringVector svResultFileNames;

	// Resultat en plus du fichier trie
	longint lSortedLinesNumber;

	///////////////////////////////////////////////////////////
	// Parametres partages

	// En entree des taches
	// Pour des raisons d'optimisation, le chunk a trier peut etre constitue
	// d'un ou plusieurs fichiers constitues par une tache precedente
	PLShared_Int input_nBucketIndex;         // Index du bucket a trier
	PLShared_StringVector input_svFileNames; // Liste des noms des fichier constituant le chunk a trier
	PLShared_Boolean input_bSingleton; // Est-ce que le fichier est un singleton (il ne contient qu'une seule clef)

	// En sortie des taches
	PLShared_Int output_nBucketIndex; // Index du bucket trie (recopie d'apres la variable correspondante en entree)
	PLShared_String output_sOutputFileName; // Nom du fichier resultat du tri
	PLShared_Longint output_nLinesSortedNumber;

	// Partages entre maitre et esclave
	PLShared_IntVector shared_ivKeyFieldIndexes; // Index des champs de la cle
	PLShared_Boolean shared_bHeaderLineUsed;
	PLShared_Boolean shared_bOnlyOneBucket;
	PLShared_Longint shared_lBucketSize;
	PLShared_Char shared_cOutputSeparator; // Separateur du fichier de sortie
	PLShared_Char shared_cInputSeparator;  // Separateur du fichier d'entree
	PLShared_Boolean shared_bSameSeparator;

	/////////////////////////////////////////////////////////////////
	// Methodes techniques

	// Saut d'un champ a partir de l'indice i
	// Prend en compte les double-quotes (Utilise dans la methode ReplaceSeparator)
	// On reproduit ici le comportement de InputBufferedFile::SkipField
	static void SkipField(CharVector* cvLineToWrite, char cOriginalSeparator, int& nPos);
};

////////////////////////////////////////////////////////////
// Classe KeyLinePair
// Contient la position de la ligne du fichier, la clef de cette ligne et l'adresse de l'inputBuffer
// Classe utilisee pour le tri
class KWKeyLinePair : public Object
{
public:
	// Constructeur
	KWKeyLinePair();
	~KWKeyLinePair();

	// Acces a le clef
	void SetKey(KWKey*);
	KWKey* GetKey() const;

	// Acces a la ligne par sa position dans un buffer
	void SetLinePosition(int nBeginPos, int nEndPos);
	void GetLinePosition(int& nBeginPos, int& nEndPos) const;

	// Acces au buffer
	void SetInputBuffer(InputBufferedFile* buffer);
	InputBufferedFile* GetInputBuffer();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWKey* theKey;
	int nLineBeginPos;
	int nLineEndPos;
	InputBufferedFile* inputBuffer;

	friend int KWKeyLinePairCompare(const void* elem1, const void* elem2);
};

// Fonction de comparaison de deux KeyLinePair suivant la clef et la position dans le fichier
int KWKeyLinePairCompare(const void* elem1, const void* elem2);

// Methdoes en inline

inline void KWKeyLinePair::SetKey(KWKey* key)
{
	check(key);
	theKey = key;
}

inline KWKey* KWKeyLinePair::GetKey() const
{
	return theKey;
}

inline void KWKeyLinePair::SetLinePosition(int nBeginPos, int nEndPos)
{
	require(nBeginPos < nEndPos);
	nLineBeginPos = nBeginPos;
	nLineEndPos = nEndPos;
}

inline void KWKeyLinePair::GetLinePosition(int& nBeginPos, int& nEndPos) const
{
	nBeginPos = nLineBeginPos;
	nEndPos = nLineEndPos;
}

inline void KWKeyLinePair::SetInputBuffer(InputBufferedFile* buffer)
{
	require(buffer != NULL);
	require(inputBuffer == NULL);
	inputBuffer = buffer;
}

inline InputBufferedFile* KWKeyLinePair::GetInputBuffer()
{
	assert(inputBuffer != NULL);
	return inputBuffer;
}
