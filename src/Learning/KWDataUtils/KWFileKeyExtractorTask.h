// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWKeyExtractor.h"
#include "PLParallelTask.h"
#include "KWArtificialDataset.h"
#include "PLFileConcatenater.h"
#include "KWKeySizeEvaluatorTask.h"

////////////////////////////////////////////////////////////
// Classe KWFileKeyExtractorTask
//	 Extraction des cles d'un fichier en parallele
class KWFileKeyExtractorTask : public PLParallelTask
{
public:
	// Constructeur
	KWFileKeyExtractorTask();
	~KWFileKeyExtractorTask();

	///////////////////////////////////////////////////////////////
	// Specification de la cle et des attributs natifs

	// Parametrage des noms des attributs de la cle
	StringVector* GetKeyAttributeNames();

	// Parametrage des noms de tous les champs natifs
	StringVector* GetNativeFieldNames();

	////////////////////////////////////////////////////////////////////
	// Specification du fichier en entree, dont il faut extraire les cles

	// Nom du fichier en entree
	void SetInputFileName(const ALString& sValue);
	const ALString& GetInputFileName() const;

	// Utilisation d'une ligne d'entete en entree: par defaut true
	void SetInputHeaderLineUsed(boolean bValue);
	boolean GetInputHeaderLineUsed() const;

	// Separateur de champs utilise en entree (par defaut: '\t')
	void SetInputFieldSeparator(char cValue);
	char GetInputFieldSeparator() const;

	////////////////////////////////////////////////////////////////////
	// Specification du fichier en sortie, contenant les cle extraites

	// Nom du fichier en sortie
	void SetOutputFileName(const ALString& sValue);
	const ALString& GetOutputFileName() const;

	// Utilisation d'une ligne d'entete en sortie: par defaut true
	void SetOutputHeaderLineUsed(boolean bValue);
	boolean GetOutputHeaderLineUsed() const;

	// Separateur de champs utilise en sortie (par defaut: '\t')
	void SetOutputFieldSeparator(char cValue);
	char GetOutputFieldSeparator() const;

	///////////////////////////////////////////////////////////////
	// Extraction des cles

	// Methode principale : extraction des clefs en parallele
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean ExtractKeys(boolean bDisplayUserMessage);

	// Resultats d'extraction
	longint GetExtractedKeyNumber() const;
	longint GetLineNumber() const;

	// Nombre d'erreurs d'encodage detectees
	longint GetEncodingErrorNumber() const;

	///////////////////////////////////////////////////////////////
	// Services divers

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;

	// Methodes de test
	static void Test();
	static boolean TestArtificialData(int nInputLineNumber, int nInputFieldNumber, boolean bInputKeyFieldsAscending,
					  boolean bInputHeaderLine, boolean bOutputHeaderLine);

	// Test avec un jeu de donnees artificiel deja cree
	// En sortie, chaque fichier de chunk est trie
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
						 boolean bOutputHeaderLine, char cOutputFiedSeparator);

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
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

	// Concatenation de chunks en evitant de dupliquer les lignes de fin de chunk et de debut de chunks suivants
	// (chaque esclave peut avoir une cle en commun avec l'esclave suivant)
	boolean ConcatenateFilesWithoutDuplicateKeys(StringVector* svFileURIs, StringVector* svHeader);

	// Nom des champs de cle et natifs, memorises au moyen KWKeyFieldsIndexer
	KWKeyFieldsIndexer keyFieldsIndexer;

	// Fichier en entree
	ALString sInputFileName;
	boolean bInputHeaderLineUsed;
	char cInputFieldSeparator;

	// Fichier en sortie
	ALString sOutputFileName;
	boolean bOutputHeaderLineUsed;
	char cOutputFieldSeparator;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves
	// tout au long du programme

	// Tableau des index des clefs
	PLShared_IntVector shared_ivKeyFieldIndexes;

	// Attributs du fichier d'entree
	PLShared_String shared_sInputFileName;
	PLShared_Boolean shared_bInputHeaderLineUsed;
	PLShared_Char shared_cInputFieldSeparator;

	// Attributs du fichier de sortie
	PLShared_String shared_sOutputFileName;
	PLShared_Char shared_cOutputFieldSeparator;

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache

	// Taille du buffer en entree
	PLShared_Int input_nBufferSize;

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Maitre a l'issue de chaque tache

	// Nombre de clefs extraites
	PLShared_Longint output_lExtractedKeyNumber;

	// Nom du chunk
	PLShared_String output_sChunkFileName;

	// Nombre de lignes lues
	PLShared_Int output_nReadLineCount;

	// Nombre d'erreurs d'encodage detectees
	PLShared_Longint output_lEncodingErrorNumber;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Estimation de la taille moyenne des clefs
	longint lMeanKeySize;

	// Estimation du nombre de lignes
	longint lEstimatedLineNumber;

	// Pour la gestion de la progression
	longint lInputFileSize;

	// Resultat : nombre de clefs extraites
	longint lExtractedKeyNumber;

	// Resultat : nombre de lignes lues
	longint lReadLineNumber;

	// Liste des chunks
	// URI: fichiers partages entre maitre et esclave
	StringVector svChunkFileNames;

	// Position dans le fichier d'entree
	longint lFilePos;

	// Gestion des exigences
	int nReadSizeMin;
	int nReadSizeMax;
	int nWriteSizeMin;
	int nWriteSizeMax;

	// Nombre d'erreurs d'encodage detectees impliquant des double quotes manquants
	longint lEncodingErrorNumber;

	//////////////////////////////////////////////////////////
	// Variables du slave

	// Parser de cles
	KWKeyExtractor keyExtractor;

	// Fichier de travail pour l'esclave
	InputBufferedFile inputFile;
};
