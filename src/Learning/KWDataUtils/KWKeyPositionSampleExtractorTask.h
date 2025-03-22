// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWSortedKeySample;

#include "PLParallelTask.h"
#include "KWKeyExtractor.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWArtificialDataset.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWKeyPositionSampleExtractorTask
// Extraction d'une liste ordonnee de K cles et de leur position a partir d'un fichier deja trie
//
//	En entree : principalement le fichier, l'index des champs de la cle a extraire,
//              et le taux d'echantillonnage des cles a extraire
//	En sortie : un tableau ordonne de cles et de leur positions
class KWKeyPositionSampleExtractorTask : public PLParallelTask
{
public:
	// Constructeur
	KWKeyPositionSampleExtractorTask();
	~KWKeyPositionSampleExtractorTask();

	/////////////////////////////////////////////////////
	// Specification du fichier d'entree

	// Fichier d'entree
	void SetFileName(const ALString& sFileName);
	const ALString& GetFileName() const;

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	/////////////////////////////////////////////////////
	// Parametres principaux de l'extraction

	// Acces aux index des clefs
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	// Pourcentage de cles a extraire (par defaut: 1)
	void SetSamplingRate(double dValue);
	double GetSamplingRate() const;

	/////////////////////////////////////////////////////
	// Parametres avances

	// Nombre de lignes dans le fichier d'entree (estimation)
	void SetFileLineNumber(longint lLines);
	longint GetFileLineNumber() const;

	// Estimation de la taille en octet necessaire pour stocker une cle
	void SetKeyUsedMemory(longint lValue);
	longint GetKeyUsedMemory() const;

	/////////////////////////////////////////////////////
	// Methode principale

	// Extraction de l'ensemble des cles avec leur position (KWKeyPosition), en garantissant
	// l'unicite des cle extraites par supressions des doublons potentiel de l'echantillon
	// Le resultat est stocke dans le tableau passe en parametre et appartient a l'appelant
	// Il peut y avoir des erreurs detectees si par exemple les cles ne sont pas ordonnees
	// parmi les cles echantillonnees
	// En cas d'erreur, on renvoie un tableau vide
	boolean ExtractSample(ObjectArray* oaKeyPositions);

	///////////////////////////////////////////////////////////////
	// Services divers

	// Affichage d'un tableau de positions de cles
	void WriteKeyPositions(const ObjectArray* oaKeyPositions, ostream& ost) const;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	// Test avec un jeu de donnees artificiel deja cree
	// Alimente en sortie le tableau de cles
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset, double dSamplingRate,
						 longint lMeanKeySize, longint lFileLineNumber,
						 ObjectArray* oaKeyPositions);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Algorithme
	// Chaque esclave traite sa portion du fichier
	// en extrayant un echantillon de cles et en le renvoyant au maitre qui se contente de les archiver
	// dans un tableau de resultats indexe par le rang des esclave.
	// A la fin, les resultats de chaque esclave sont concatenes dans l'ordre initial (fichier deja trie)
	// pour produire le tableau de cle final
	// Chaque esclave ne connait les numero de ligne que localement a sa tache. Le maitre collecte donc les
	// nombre de lignes traitees par tache (dans lvLineCountPerTaskIndex), et au moment de sa finalisation,
	// il peut ainsi recalculer le numero de ligne global associe a chaque cle.

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

	////////////////////////////////////////////////////
	// Variables du Master

	// Specifications en entree de la tache
	ALString sFileName;
	boolean bHeaderLineUsed;
	char cFieldSeparator;
	longint lFileLineNumber;
	longint lKeyUsedMemory;
	longint lInputFileSize;

	// Definition des exigences pour la taille du buffer
	int nReadSizeMin;
	int nReadSizeMax;
	int nReadBufferSize;

	// Table d'echantillons de cle (ObjectArray de KWKeyPosition)
	// Memorisation des resultats d'analyse des esclaves
	ObjectArray oaAllKeyPositionSamples;

	// Memorisation du nombre de lignes lues pour chaque SlaveProcess
	LongintVector lvLineCountPerTaskIndex;

	// Resultats : tableau de clefs en sortie (classe KWKeyPosition)
	ObjectArray oaResultKeyPositions;

	// Position dans le fichier en lecture
	longint lFilePos;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Index des champs de la cle
	PLShared_IntVector shared_ivKeyFieldIndexes;

	// Taux d'echantillonnage en entree
	PLShared_Double shared_dSamplingRate;

	// Attributs du fichier d'entree
	PLShared_String shared_sInputFileName;
	PLShared_Boolean shared_bHeaderLineUsed;
	PLShared_Char shared_cFieldSeparator;

	///////////////////////////////////////////////////////////
	// Parametres en entree et sortie des esclaves

	// Echantillon de cle retourne par l'esclave
	PLShared_ObjectArray* output_oaKeyPositionSample;

	// Nombre de ligne lues dans chaque SlaveProcess
	PLShared_Longint output_lReadLineCount;

	// Taille du buffer en entree
	PLShared_Int input_nBufferSize;

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	////////////////////////////////////////////////////
	// Variables de l'esclave

	// Extracteur de cle
	KWKeyExtractor keyExtractor;

	// Fichier de travail pour l'esclave
	InputBufferedFile inputFile;
};

int KWSortedKeyPositionArrayCompare(const void* elem1, const void* elem2);
