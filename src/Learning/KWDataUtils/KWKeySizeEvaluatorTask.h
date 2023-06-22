// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"
#include "KWKeyExtractor.h"
#include "KWArtificialDataset.h"

///////////////////////////////////////////////////////////////
// Classe KWKeySizeEvaluatorTask
// Tache qui permet d'estimer la taille moyenne des clefs et le nombre de lignes d'un fichier
// Chaque processus lit un buffer constituant un echantillon de cles,
// la position de depart du buffer est determinee aleatoirement.
class KWKeySizeEvaluatorTask : public PLParallelTask
{
public:
	// Constructeur
	KWKeySizeEvaluatorTask();
	~KWKeySizeEvaluatorTask();

	// Methode principale, les parametres sont :
	//  - un vecteur contenant les index de la clef
	//  - les attributs du fichier (nom, header, separateur)
	// En sortie:
	//  - dMeanKeySize: estimation de la moyenne des tailles des cles (>0)
	//  - dFileLineNumber: estimation du nombre de lignes du fichier
	// la methode renvoie true si tout s'est bien passe
	boolean EvaluateKeySize(const IntVector* ivKeyIndexes, const ALString& sFileName, boolean bIsHeaderLineUsed,
				char cFieldSeparator, longint& lMeanKeySize, longint& lFileLineNumber);

	// Methode de test
	static void Test();

	// Test avec un jeu de donnees artificiel deja cree
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset, longint& lMeanKeySize,
						 longint& lLineNumber);

	///////////////////////////////////////////////////////////////////////////////
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

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves
	// tout au long du programme
	// Elles correspondent principalement aux champs de saisie de l'IHM

	// Fichier d'entree
	PLShared_InputBufferedFile shared_inputFile;

	// Index des clefs
	PLShared_IntVector shared_ivKeyFieldIndexes;

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache

	// Position dans le fichier
	PLShared_Longint input_lBufferStartPosition;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Maitre a l'issue de chaque tache

	PLShared_Int output_nTotalKeySize;
	PLShared_Int output_nLineNumber;
	PLShared_Int output_nBufferSize;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Taille du fichier d'entree
	longint lInputFileSize;

	// Taille de l'ensemble des clefs lues
	longint lTotalKeySize;

	// Nombre de lignes entieres lues
	longint lTotalLineNumber;

	// Taille cummulee des buffers lus
	longint lCummulatedBufferSize;

	// Debut de buffer du fichier a analyser par les esclaves
	LongintVector lvBufferStartPositions;

	//////////////////////////////////////////////////////////
	// Variables du slave

	// Extracteur de cle
	KWKeyExtractor keyExtractor;
};
