// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWFileIndexerTask;

#include "PLParallelTask.h"
#include "KWArtificialDataset.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWFileIndexerTask
// Extraction d'une liste ordonnee des positions et d'index de lignes
class KWFileIndexerTask : public PLParallelTask
{
public:
	// Constructeur
	KWFileIndexerTask();
	~KWFileIndexerTask();

	/////////////////////////////////////////////////////
	// Specification du fichier d'entree

	// Fichier d'entree
	void SetFileName(const ALString& sFileName);
	const ALString& GetFileName() const;

	/////////////////////////////////////////////////////
	// Methode principale

	// Extraction de l'ensemble des positions et de nombre de lignes lues du fichier
	// Entrees:
	//  . le fichier specifie en entree
	//  . une taille max de buffer
	// Sorties:
	//  . un vecteur de debut de positions, commencant a 0 et avec une derniere valeur
	//    corespondant a la taille du fichier
	//  . un vecteur d' index de lignes, commencant a 0 et avec une derniere valeur
	//    corespondant au nombre de ligne du fichier
	// Chaque position est a distance d'au plus une taille de buffer de la suivante.
	// Les positions au debut et en fin de vecteur sont dimensionnes de facon progressive
	// avec un decallage en escalier.
	boolean ComputeIndexation(int nMaxBufferSize, LongintVector* lvFileBeginPositions,
				  LongintVector* lvFileBeginRecordIndexes);

	///////////////////////////////////////////////////////////////
	// Services divers

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Algorithme
	// On utilise une taille de buffer egale a la contrainte de taille en entree
	// ce qui permet de ne collecter que une seule position de debut de premier ligne
	// et un seul nombre de ligne par esaclave, ce qui est tres efficace
	// La gestion des tailles en escalier est geree par le maitre

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

	// Specifications en entree de la tache
	ALString sFileName;
	longint lFileSize;

	// Acces aux parametres de la methode d'indexation principale
	LongintVector* lvTaskFileBeginPositions;
	LongintVector* lvTaskFileBeginRecordIndexes;
	int nTaskMaxBufferSize;

	// Parcours du fichier par le maitre
	longint lFilePos;

	// Variable partagee pour specifier le fichier a traiter
	PLShared_String shared_sFileName;

	// Variable en entree de l'esclave pour parcourir le fichier
	PLShared_Longint input_lFilePos;
	PLShared_Int input_nBufferSize;

	// Variable en sortie de l'esclave pour indiquer une position et un nombre de lignes traitees
	PLShared_Longint output_lStartLinePos;
	PLShared_Int output_nLineCount;

	// Fichier de travail pour l'esclave
	InputBufferedFile bufferedFile;

	// Taille minimum de buffer utilisee
	const int nMinBufferSize = BufferedFile::nDefaultBufferSize / 8;
};
