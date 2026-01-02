// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLFileConcatenater.h"
#include "UserInterface.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PEFileSearchTask
// Classe de test de lecture de fichier en parallele et de l'utilisation de
// l'affichage de messages avec le numero de lignes.
// Cette classe a pour but de rechercher une chaine de caracteres dans un
// fichier, de compter le nombre d'occurrences trouvees, et d'afficher dans
// le log les lignes ou le texte a ete trouve
class PEFileSearchTask : public PLParallelTask
{
public:
	// Constructeur
	PEFileSearchTask();
	~PEFileSearchTask();

	// Lance la recherche de la chaine dans le fichier d'entree
	// Renvoie le nombre d'occurrences trouvees
	longint SeachString(const ALString& sInputFileName, const ALString& sSeachedString);

	// Methode de test
	static void Test();

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

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves
	// tout au long du programme
	// Le fichier en entree est gere par un MasterBuffer

	// Chaine recherchee
	PLShared_String shared_sSearchedString;

	// Nom du fichier d'entree (les esclaves en ont besoin mais il ne change pas,
	// on ne l'envoie qu'une fois)
	PLShared_String shared_sFileName;

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	// Taille du buffer en entree
	PLShared_Int input_nBufferSize;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Maitre a l'issue de chaque tache

	// Nombre de chaines trouvees lors de chaque traitement
	PLShared_Int output_nFoundLineNumber;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Nom du fichier d'entree, utilise pour le master buffer
	ALString sMasterBufferInputFileName;

	// Pour la gestion de la progression
	longint lInputFileSize;

	// Resultat : nombre de clefs extraites
	longint lFoundLineNumber;

	// Position dans le fichier
	longint lFilePos;

	// Taille du buffer de lecture
	const int nBufferSize = 8 * lMB;
};
