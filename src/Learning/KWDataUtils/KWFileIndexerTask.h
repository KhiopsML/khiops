// Copyright (c) 2024 Orange. All rights reserved.
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
	//  . une taille de buffer
	//  . un nombre de positions a calculer en moyenne par taille de buffer
	// Sorties:
	//  . un vecteur de debut de positions, commencant a 0 et avec une derniere valeur
	//    corespondant a la taille du fichier
	//  . un vecteur d'index de lignes, commencant a 0 et avec une derniere valeur
	//    corespondant au nombre de lignes du fichier
	// L'ecart moyen entre les positions est d'environ la taille du buffer divisee par le nombre
	// de positions a calculer par buffer.
	// Les positions au debut et en fin de vecteur sont dimensionnees de facon progressive
	// avec un decallage en escalier.
	boolean ComputeIndexation(int nBufferSize, int nPositionNumberPerBuffer, LongintVector* lvFileBeginPositions,
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

	// Variable de travail du maitre pour collecter toutes les vecteurs de resutats renvoyes par les esclaves
	ObjectArray oaMasterFileStartLinePositionVectors;
	ObjectArray oaMasterBufferLineCountVectors;

	// Parcours du fichier par le maitre
	longint lFilePos;

	// Variable partagee pour specifier le fichier a traiter
	PLShared_String shared_sFileName;

	// Variables partagges pour les parametres de la tache
	PLShared_Int shared_nBufferSize;
	PLShared_Int shared_nPositionNumberPerBuffer;

	// Variable en entree de l'esclave pour parcourir le fichier
	PLShared_Longint input_lFilePos;

	// Variable en sortie de l'esclave pour les vecteurs de positions et un nombre de lignes traitees entre
	// positions
	PLShared_LongintVector output_lvFileStartLinePositions;
	PLShared_IntVector output_ivBufferLineCounts;

	// Fichier de travail pour l'esclave
	InputBufferedFile inputFile;
};
