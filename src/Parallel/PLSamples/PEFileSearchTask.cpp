// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEFileSearchTask.h"

PEFileSearchTask::PEFileSearchTask()
{
	// Parametres du programme
	DeclareSharedParameter(&shared_sSearchedString);
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

longint PEFileSearchTask::SeachString(const ALString& sinputBufferName, const ALString& sSeachedString)
{
	// Parametrage du fichier resultat
	sMasterBufferInputFileName = sinputBufferName;

	// Parametrage de la chaine recherchee
	shared_sSearchedString.SetValue(sSeachedString);

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
	ALString sinputBufferName;
	fstream fst;
	const int nInputLineNumber = 10000;
	ALString sTestId;
	int nLine;
	ALString sTmp;
	PEFileSearchTask fileSearchTask;
	longint lFoundNumber;

	// Calcul d'un identifiant de test a partir de ses parametres
	sTestId = sTmp + "_" + IntToString(nInputLineNumber);

	// Creation du fichier d'entree
	sinputBufferName = FileService::BuildFilePathName(FileService::GetTmpDir(), sFilePrefix + sTestId + ".txt");
	if (bCreateFiles)
	{
		FileService::OpenOutputFile(sinputBufferName, fst);

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
	lFoundNumber = fileSearchTask.SeachString(sinputBufferName, sSearchedString);

	// Affichage des resultats
	cout << "Artificial test:  " << nInputLineNumber << " lines" << endl;
	cout << "\tSearched string: " << sSearchedString << endl;
	cout << "\tFound number: " << lFoundNumber << endl;

	// Destruction des fichiers
	if (bCreateFiles)
	{
		FileService::RemoveFile(sinputBufferName);
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
		    (int)lMB, InputBufferedFile::FitBufferSize(GetSlaveResourceGrant()->GetMemory()), (int)lMB,
		    lFilePos, lInputFileSize);
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
	InputBufferedFile inputBuffer;
	double dProgression;
	ALString sTmp;
	boolean bIsOpen;
	boolean bTrace = false;
	longint lLinePosition;
	PeriodicTest periodicTestInterruption;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;

	// Initialisation des resultats de la tache
	output_nFoundLineNumber = 0;

	// On recupere le master buffer, qui est initialise correctement pour la tache
	inputBuffer.SetFileName(shared_sFileName.GetValue());
	inputBuffer.SetBufferSize(input_nBufferSize);
	bIsOpen = inputBuffer.Open();

	// Specification de la portion du fichier a traiter
	// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve potentiellement
	// sur le debut de chunk suivant, un octet au dela de la fin
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + input_nBufferSize + 1, inputBuffer.GetFileSize());

	// Affichage
	if (bTrace)
		cout << GetClassLabel() << "\t" << GetTaskIndex() << "\tBegin\t"
		     << FileService::GetFileName(inputBuffer.GetFileName()) << "\t" << lBeginPos << "\t" << lMaxEndPos
		     << endl;

	// Parametrage de la taille du buffer
	inputBuffer.SetBufferSize(input_nBufferSize);

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	// On ne compte pas la ligne sautee en debut de chunk, car elle est traitee en fin du chunk precedent
	nCumulatedLineNumber = 0;
	if (lBeginPos > 0)
	{
		bOk = inputBuffer.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
		if (bOk)
		{
			// On se positionne sur le debut de la ligne suivante si elle est trouvee
			if (lNextLinePos != -1)
				lBeginPos = lNextLinePos;
			// Si non trouvee, on se place en fin de chunk
			else
				lBeginPos = lMaxEndPos;
		}
	}

	// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
	// On reitere tant que l'on a pas atteint la derniere position pour lire toutes les ligne, y compris la derniere
	lLinePosition = -1;
	while (bOk and lBeginPos < lMaxEndPos)
	{
		bOk = inputBuffer.FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);
		if (not bOk)
			break;

		// Traitement des lignes trop longues
		while (bOk and bLineTooLong)
		{
			assert(inputBuffer.GetPositionInFile() - lBeginPos > InputBufferedFile::GetMaxLineLength());

			// Warning sur la ligne trop longue
			AddWarning(sTmp + "Line too long (" +
				   LongintToHumanReadableString(inputBuffer.GetPositionInFile() - lBeginPos) + ")");

			// Lecture de la suite a partir de la fin de la ligne trop longue
			lBeginPos = inputBuffer.GetPositionInFile();
			bOk = inputBuffer.FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);

			// Incrementation du numero de ligne pour tenir compte de la ligne sautee
			nCumulatedLineNumber++;
		}

		// Parcours du buffer d'entree
		while (bOk and not inputBuffer.IsBufferEnd())
		{
			lLinePosition = inputBuffer.GetPositionInFile();

			// Gestion de la progresssion
			if (periodicTestInterruption.IsTestAllowed(nCumulatedLineNumber +
								   inputBuffer.GetCurrentLineIndex()))
			{
				// Calcul de la progression par rapport a la proportion de la portion du fichier traitee
				// parce que l'on ne sait pas le nombre total de ligne que l'on va traiter
				dProgression = (inputBuffer.GetPositionInFile() - input_lFilePos) * 1.0 /
					       (lMaxEndPos - input_lFilePos);
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}

			// Lecture de la prochaine ligne
			inputBuffer.GetNextLine(&cvLine, bLineTooLong);

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
				AddLocalMessage(sBuffer, inputBuffer.GetCurrentLineIndex());
			}

			// On se deplace de la taille du buffer analyse
			lBeginPos += inputBuffer.GetCurrentBufferSize();
			nCumulatedLineNumber += inputBuffer.GetBufferLineNumber();
		}
	}
	// Envoi du nombre de lignes au master, en fin de SlaveProcess
	if (bOk)
		SetLocalLineNumber(nCumulatedLineNumber);

	if (bIsOpen)
		inputBuffer.Close();
	return bOk;
}

boolean PEFileSearchTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
