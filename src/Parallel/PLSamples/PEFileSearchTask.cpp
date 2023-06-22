// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEFileSearchTask.h"

PEFileSearchTask::PEFileSearchTask()
{
	// Parametres du programme
	DeclareSharedParameter(&shared_sSearchedString);
	DeclareSharedParameter(&shared_sResultFileName);
	DeclareSharedParameter(&shared_sFileName);

	// Input des esclaves
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);

	// Resultats envoyes par l'esclave
	DeclareTaskOutput(&output_nFoundLineNumber);

	// Variables du maitre
	lInputFileSize = 0;
	lFoundLineNumber = 0;
}

PEFileSearchTask::~PEFileSearchTask() {}

longint PEFileSearchTask::SeachString(const ALString& sInputFileName, const ALString& sSeachedString,
				      const ALString& sResultFileName)
{
	// Parametrage du fichier resultat
	sMasterBufferInputFileName = sInputFileName;

	// Parametrage de la chaine recherchee
	shared_sSearchedString.SetValue(sSeachedString);

	// Parametrage du fichier resultat
	shared_sResultFileName.SetValue(sResultFileName);

	// Execution de la tache
	Run();

	// On renvoie le resultat calcule par la tache
	return lFoundLineNumber;
}

void PEFileSearchTask::Test()
{
	const ALString sFilePrefix = "TestFile";
	const ALString sSearchedString = "Bonjour monsieur";
	boolean bCreateFiles = true;
	ALString sInputFileName;
	ALString sResultFileName;
	fstream fst;
	const int nInputLineNumber = 10000;
	ALString sTestId;
	int nLine;
	ALString sTmp;
	PEFileSearchTask fileSearchTask;
	longint lFoundNumber;
	const int nTmpBufferSize = 10000;
	char sTmpBuffer[nTmpBufferSize];

	// Calcul d'un identifiant de test a partir de ses parametres
	sTestId = sTmp + "_" + IntToString(nInputLineNumber);

	// Creation du fichier d'entree
	sInputFileName = FileService::BuildFilePathName(FileService::GetTmpDir(), sFilePrefix + sTestId + ".txt");
	sResultFileName =
	    FileService::BuildFilePathName(FileService::GetTmpDir(), sFilePrefix + "Result" + sTestId + ".txt");
	if (bCreateFiles)
	{
		FileService::OpenOutputFile(sInputFileName, fst);

		// Creation des lignes
		for (nLine = 0; nLine < nInputLineNumber; nLine++)
		{
			// Creation de deux champ cle en position 2 et en fin de ligne
			fst << "Line " << nLine + 1 << "\t";
			fst << "abcdefghijklmnopqrstuvwxyz";
			if (nLine % 10 == 0)
				fst << sSearchedString;
			fst << "0123456789";
			fst << "\n";
		}
		fst.close();
	}

	// Appel de la tache de recherche de chaine de caracteres
	lFoundNumber = fileSearchTask.SeachString(sInputFileName, sSearchedString, sResultFileName);

	// Affichage des resultats
	cout << "Artificial test:  " << nInputLineNumber << " lines" << endl;
	cout << "\tSearched string: " << sSearchedString << endl;
	cout << "\tFound number: " << lFoundNumber << endl;

	// Lecture des premieres lignes du fichier de resultat
	FileService::OpenInputFile(sResultFileName, fst);
	if (fst.is_open())
	{
		for (nLine = 0; nLine < 3; nLine++)
		{
			fst.getline(sTmpBuffer, nTmpBufferSize);
			cout << sTmpBuffer << "\n";
		}
		fst.close();
	}

	// Destruction des fichiers
	if (bCreateFiles)
	{
		FileService::RemoveFile(sInputFileName);
		FileService::RemoveFile(sResultFileName);
	}
}

const ALString PEFileSearchTask::GetTaskName() const
{
	return "File search";
}

PLParallelTask* PEFileSearchTask::Create() const
{
	return new PEFileSearchTask;
}

boolean PEFileSearchTask::ComputeResourceRequirements()
{
	// Exigences de la tache
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(nBufferSize); // Lecture
	return true;
}

boolean PEFileSearchTask::MasterInitialize()
{
	// Parametrage du master buffer
	shared_sFileName.SetValue(sMasterBufferInputFileName);

	// Calcul de la taille du fichier d'entree pour estimer la progression
	lInputFileSize = FileService::GetFileSize(sMasterBufferInputFileName);

	// Initialisation des resultats
	lFoundLineNumber = 0;

	lFilePos = 0;
	return true;
}

boolean PEFileSearchTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Est-ce qu'il y a encore du travail ?
	if (lFilePos >= lInputFileSize)
		bIsTaskFinished = true;
	else
	{
		// Calcul de la progression
		dTaskPercent = nBufferSize * 1.0 / lInputFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;

		input_lFilePos = lFilePos;

		// Initialisation de l'input buffer
		// Cette gestion fine des tailles du master buffer permet de mieux repartir
		// la charge de travail entre les esclaves, notamment en debut et fin de fichier
		input_nBufferSize = ComputeStairBufferSize(
		    (int)lMB, InputBufferedFile::FitBufferSize(GetSlaveResourceGrant()->GetMemory()), lFilePos,
		    lInputFileSize);
		lFilePos += input_nBufferSize;
	}
	return true;
}

boolean PEFileSearchTask::MasterAggregateResults()
{
	// Ajout des statistiques traites par l'esclave
	lFoundLineNumber += output_nFoundLineNumber;

	return true;
}

boolean PEFileSearchTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PEFileSearchTask::SlaveInitialize()
{
	return true;
}

boolean PEFileSearchTask::SlaveProcess()
{
	boolean bOk = true;
	CharVector cvLine;
	ALString sBuffer;
	int i;
	InputBufferedFile inputBufferedFile;
	double dProgression;
	ALString sTmp;
	boolean bIsOpen;

	// Initialisation des resultats de la tache
	output_nFoundLineNumber = 0;

	// On recupere le master buffer, qui est initialise correctement pour la tache
	inputBufferedFile.SetFileName(shared_sFileName.GetValue());
	inputBufferedFile.SetBufferSize(input_nBufferSize);
	bIsOpen = inputBufferedFile.Open();
	if (bIsOpen)
	{
		// Lecture du fichier a partir de input_lFilePos
		bOk = inputBufferedFile.Fill(input_lFilePos);

		// Envoi au master du nombre de lignes contenues dans le buffer
		// Le maitre pourra calculer l'index global des lignes a partir de l'index
		// local au buffer C'est necessaire pour l'affichage de messages qui
		// contienent un numero de ligne
		SetLocalLineNumber(inputBufferedFile.GetBufferLineNumber());
	}
	else
		bOk = false;

	// Traitement du buffer maitre courant
	while (bOk and not inputBufferedFile.IsBufferEnd())
	{
		// Lecture de la prochaine ligne
		inputBufferedFile.GetNextLine(&cvLine);

		// On transfert la ligne (CharVector) dans une chaine de caracteres
		// Attention: cela pourrait poser des problemes dans le cas de lignes de
		// tres grandes tailles, bien gerees avec des CharVector, mais pas avec
		// des ALString
		sBuffer = "";
		for (i = 0; i < cvLine.GetSize(); i++)
			sBuffer += cvLine.GetAt(i);

		// Recherche de la sous-chaine
		if (sBuffer.Find(shared_sSearchedString.GetValue()) >= 0)
		{
			output_nFoundLineNumber++;

			// Affichage de la ligne dans le log de Khiops
			// On donne le numero de ligne local au buffer de lecture
			// Le maitre qui affiche le message calculera la numero de ligne dans le
			// fichier Pour que cela fonctionne, il est necessaire d'appeler la
			// methode SetLocalLineNumber
			AddLocalMessage(sBuffer, inputBufferedFile.GetCurrentLineNumber());
		}

		// Suivi de la tache
		bOk = bOk and not inputBufferedFile.IsError();
		if (inputBufferedFile.GetCurrentLineNumber() % 100 == 0)
		{
			dProgression =
			    1.0 * inputBufferedFile.GetCurrentLineNumber() / inputBufferedFile.GetBufferLineNumber();
			TaskProgression::DisplayProgression((int)floor(dProgression * 100));
		}
		if (TaskProgression::IsInterruptionRequested())
			break;
	}

	if (bIsOpen)
		inputBufferedFile.Close();
	return bOk;
}

boolean PEFileSearchTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}