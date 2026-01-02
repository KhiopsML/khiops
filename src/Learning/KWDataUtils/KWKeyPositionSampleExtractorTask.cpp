// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKeyPositionSampleExtractorTask.h"
#include "KWKeyExtractor.h"
#include "TaskProgression.h"

KWKeyPositionSampleExtractorTask::KWKeyPositionSampleExtractorTask()
{
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';
	shared_dSamplingRate = 1;
	lFileLineNumber = 0;
	lKeyUsedMemory = 0;
	lInputFileSize = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	nReadBufferSize = 0;
	lFilePos = 0;

	// Variables partagees
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_dSamplingRate);
	DeclareSharedParameter(&shared_sInputFileName);
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_cFieldSeparator);

	// Variables en entree et sortie des esclaves
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);

	output_oaKeyPositionSample = new PLShared_ObjectArray(new PLShared_KeyPosition);
	DeclareTaskOutput(output_oaKeyPositionSample);
	DeclareTaskOutput(&output_lReadLineCount);
}

KWKeyPositionSampleExtractorTask::~KWKeyPositionSampleExtractorTask()
{
	assert(oaAllKeyPositionSamples.GetSize() == 0);
	assert(oaResultKeyPositions.GetSize() == 0);
	delete output_oaKeyPositionSample;
}

void KWKeyPositionSampleExtractorTask::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

const ALString& KWKeyPositionSampleExtractorTask::GetFileName() const
{
	return sFileName;
}

void KWKeyPositionSampleExtractorTask::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean KWKeyPositionSampleExtractorTask::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

void KWKeyPositionSampleExtractorTask::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

char KWKeyPositionSampleExtractorTask::GetFieldSeparator() const
{
	return cFieldSeparator;
}

IntVector* KWKeyPositionSampleExtractorTask::GetKeyFieldIndexes()
{
	return shared_ivKeyFieldIndexes.GetIntVector();
}

const IntVector* KWKeyPositionSampleExtractorTask::GetConstKeyFieldIndexes() const
{
	return shared_ivKeyFieldIndexes.GetConstIntVector();
}

void KWKeyPositionSampleExtractorTask::SetSamplingRate(double dValue)
{
	require(0 < dValue and dValue <= 1);
	shared_dSamplingRate = dValue;
}

double KWKeyPositionSampleExtractorTask::GetSamplingRate() const
{
	return shared_dSamplingRate;
}

void KWKeyPositionSampleExtractorTask::SetFileLineNumber(longint lLines)
{
	require(lLines > 0);
	lFileLineNumber = lLines;
}

longint KWKeyPositionSampleExtractorTask::GetFileLineNumber() const
{
	return lFileLineNumber;
}

void KWKeyPositionSampleExtractorTask::SetKeyUsedMemory(longint lValue)
{
	require(lValue > 0);
	lKeyUsedMemory = lValue;
}

longint KWKeyPositionSampleExtractorTask::GetKeyUsedMemory() const
{
	return lKeyUsedMemory;
}

boolean KWKeyPositionSampleExtractorTask::ExtractSample(ObjectArray* oaKeyPositions)
{
	boolean bOk = true;
	boolean bDisplay = false;

	require(oaKeyPositions != NULL);
	require(sFileName != ""); // Le fichier d'entree doit etre renseigne
	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(lKeyUsedMemory > 0);
	require(lFileLineNumber > 0);

	// Execution de la tache
	bOk = Run();

	// Collecte des resultats
	if (bOk and not IsTaskInterruptedByUser())
	{
		oaKeyPositions->CopyFrom(&oaResultKeyPositions);
		oaResultKeyPositions.RemoveAll();
	}
	// Nettoyage sinon
	else
	{
		oaKeyPositions->RemoveAll();
		oaResultKeyPositions.DeleteAll();
		bOk = false;
	}

	// Affichage
	if (bDisplay)
	{
		cout << GetClassLabel() << "\t" << bOk << "\n";
		WriteKeyPositions(oaKeyPositions, cout);
		cout << endl;
	}
	return bOk;
}

void KWKeyPositionSampleExtractorTask::WriteKeyPositions(const ObjectArray* oaKeyPositions, ostream& ost) const
{
	int i;

	require(oaKeyPositions != NULL);

	ost << "Key positions\t" << oaKeyPositions->GetSize() << "\n";
	for (i = 0; i < oaKeyPositions->GetSize(); i++)
		ost << "\t" << i << "\t" << *oaKeyPositions->GetAt(i) << endl;
}

const ALString KWKeyPositionSampleExtractorTask::GetObjectLabel() const
{
	return sFileName;
}

void KWKeyPositionSampleExtractorTask::Test()
{
	boolean bOk = true;
	KWArtificialDataset artificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaKeyPositions;
	KWKeyPositionSampleExtractorTask keyPositionSampleExtractor;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + keyPositionSampleExtractor.GetTaskLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Creation d'un fichier avec des champs cle
	artificialDataset.SpecifySortedDataset();
	artificialDataset.CreateDataset();
	artificialDataset.DisplayFirstLines(15);

	// Evaluation de la taille des cles
	lMeanKeySize = 0;
	lLineNumber = 0;
	bOk = bOk and KWKeySizeEvaluatorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, lLineNumber);

	// Extraction des cles
	bOk = bOk and KWKeyPositionSampleExtractorTask::TestWithArtificialDataset(&artificialDataset, 0.5, lMeanKeySize,
										  lLineNumber, &oaKeyPositions);
	oaKeyPositions.DeleteAll();

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWKeyPositionSampleExtractorTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
								    double dSamplingRate, longint lMeanKeySize,
								    longint lFileLineNumber,
								    ObjectArray* oaKeyPositions)
{
	boolean bOk = true;
	KWKeyPositionSampleExtractorTask keyPositionSampleExtractor;
	int i;

	require(artificialDataset != NULL);

	// Extraction des cles
	keyPositionSampleExtractor.SetFileName(artificialDataset->GetFileName());
	keyPositionSampleExtractor.SetHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
	keyPositionSampleExtractor.SetFieldSeparator(artificialDataset->GetFieldSeparator());
	keyPositionSampleExtractor.SetSamplingRate(dSamplingRate);
	keyPositionSampleExtractor.SetKeyUsedMemory(lMeanKeySize);
	keyPositionSampleExtractor.SetFileLineNumber(lFileLineNumber);
	keyPositionSampleExtractor.GetKeyFieldIndexes()->CopyFrom(artificialDataset->GetConstKeyFieldIndexes());
	bOk = bOk and keyPositionSampleExtractor.ExtractSample(oaKeyPositions);
	cout << "Extracted key positions: " << oaKeyPositions->GetSize() << " (" << bOk << ")" << endl;
	for (i = 0; i < oaKeyPositions->GetSize(); i++)
	{
		cout << "\t" << *oaKeyPositions->GetAt(i) << endl;
		if (i > 10)
			break;
	}
	return bOk;
}

const ALString KWKeyPositionSampleExtractorTask::GetTaskName() const
{
	return "Key position sample extractor";
}

PLParallelTask* KWKeyPositionSampleExtractorTask::Create() const
{
	return new KWKeyPositionSampleExtractorTask;
}

boolean KWKeyPositionSampleExtractorTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	longint lKeyPositionUsedMemory;
	int nPreferredSize;
	int nMinBufferLineNumber;
	int nMaxBufferLineNumber;

	// Taille du fichier en entree
	lInputFileSize = PLRemoteFileService::GetFileSize(sFileName);

	// Exigences sur la taille du buffer de lecture
	// On neglige la memoire necessaire pour la gestion du cache, dans ce cas de traitement mono-fichier
	nPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sFileName);
	nReadSizeMin = nPreferredSize;
	nReadSizeMax = (SystemFile::nMaxPreferredBufferSize / nPreferredSize) * nPreferredSize;
	nReadSizeMax = max(nReadSizeMax, nReadSizeMin);
	if (nReadSizeMin > lInputFileSize)
		nReadSizeMin = (int)lInputFileSize;
	if (nReadSizeMax > lInputFileSize)
		nReadSizeMax = (int)lInputFileSize;

	// Memoire necessaire pour stocker une cle et sa position
	lKeyPositionUsedMemory = lKeyUsedMemory + sizeof(KWKeyPosition) - sizeof(KWKey) + sizeof(Object*);

	// Memoire pour memoriser l'echantillon de cle global
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(
	    (longint)ceil(shared_dSamplingRate * lFileLineNumber * lKeyPositionUsedMemory *
			  2)); // Taille du sample (*2 pour etre tranquille)

	// Estimation du nombre de clefs dans un buffer
	nMinBufferLineNumber = (int)ceil(lFileLineNumber * (1.0 * nReadSizeMin) / (lInputFileSize + 1));
	nMaxBufferLineNumber = (int)ceil(lFileLineNumber * (1.0 * nReadSizeMax) / (lInputFileSize + 1));

	// Memoire par esclave: le buffer, plus les cle par buffer traite, au prorata de la taille du buffer
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(
	    nReadSizeMin + (2 * lKeyPositionUsedMemory * nMinBufferLineNumber));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(
	    nReadSizeMax + (2 * lKeyPositionUsedMemory * nMaxBufferLineNumber));

	// Nombre max de slaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / (nReadSizeMin + 1)));
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::MasterInitialize()
{
	boolean bOk = true;
	longint lKeyPositionUsedMemory;
	longint lSlaveGrantedMemory;
	longint lReadBufferSize;
	int nPreferredBufferSize;
	ALString sTmp;

	require(oaAllKeyPositionSamples.GetSize() == 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(0.0 < shared_dSamplingRate and shared_dSamplingRate <= 1.0);

	// Initialisation des variables partagees
	shared_sInputFileName.SetValue(sFileName);
	shared_bHeaderLineUsed = bHeaderLineUsed;
	shared_cFieldSeparator.SetValue(cFieldSeparator);

	// Test si fichier d'entree present
	if (not PLRemoteFileService::FileExists(sFileName))
	{
		AddError("Input file " + FileService::GetURIUserLabel(sFileName) + " does not exist");
		bOk = false;
	}

	// Test si taille du fichier non nulle
	if (bOk and lInputFileSize == 0)
	{
		AddError("Input file " + FileService::GetURIUserLabel(sFileName) + " is empty");
		bOk = false;
	}

	// Memoire necessaire pour stocker une cle et sa position
	lKeyPositionUsedMemory = lKeyUsedMemory + sizeof(KWKeyPosition) - sizeof(KWKey) + sizeof(Object*);

	// Calcul de la taille du buffer en fonction de la memoire disponible pour les esclaves, selon la formule
	// suivante similaire aux exigences min et max
	//   lSlaveGrantedMemory = nReadBufferSize + (2 * lKeyPositionUsedMemory * lFileLineNumber * nReadBufferSize /
	//   (lInputFileSize + 1))
	// Soit
	//   lSlaveGrantedMemory = nReadBufferSize * (1 + (2 * lKeyPositionUsedMemory * lFileLineNumber) /
	//   (lInputFileSize + 1) nReadBufferSize = lSlaveGrantedMemory / (1 + (2 * lKeyPositionUsedMemory *
	//   lFileLineNumber) / (lInputFileSize + 1)
	lSlaveGrantedMemory = GetSlaveResourceGrant()->GetMemory();
	lReadBufferSize = longint(lSlaveGrantedMemory /
				  (1 + (2.0 * lKeyPositionUsedMemory * lFileLineNumber) / (lInputFileSize + 1)));

	// Chaque esclave doit lire au moins 5 buffer (pour que le travail soit bien reparti entre les esclaves)
	if (lInputFileSize / (GetProcessNumber() * (longint)5) < lReadBufferSize)
	{
		lReadBufferSize = lInputFileSize / (GetProcessNumber() * (longint)5);
		if (GetVerbose())
			AddMessage(sTmp + "Read buffer size reduced to " +
				   LongintToHumanReadableString(lReadBufferSize));
	}
	nReadBufferSize = InputBufferedFile::FitBufferSize(lReadBufferSize);

	// Arrondi a un multiple de preferredBufferSize du buffer de lecture
	nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(sFileName);
	if (nReadBufferSize > nPreferredBufferSize)
	{
		nReadBufferSize = (nReadBufferSize / nPreferredBufferSize) * nPreferredBufferSize;
		if (GetVerbose())
			AddMessage(sTmp + "Read buffer size shrunk to preferred size multiple " +
				   LongintToHumanReadableString(nReadBufferSize));
	}

	// Projection sur les exigences min et max
	nReadBufferSize = max(nReadBufferSize, nReadSizeMin);
	nReadBufferSize = min(nReadBufferSize, nReadSizeMax);

	// Initialisation de la position du fichier a traiter
	lFilePos = 0;
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Est-ce qu'il y a encore du travail ?
	if (lFilePos >= lInputFileSize)
		bIsTaskFinished = true;
	else
	{
		// On agrandit la taille du tableau des resultats des esclaves
		oaAllKeyPositionSamples.Add(NULL);
		lvLineCountPerTaskIndex.Add(0);

		// Parametrage de la taille du buffer
		input_nBufferSize = ComputeStairBufferSize(nReadSizeMin, nReadBufferSize,
							   PLRemoteFileService::GetPreferredBufferSize(sFileName),
							   lFilePos, lInputFileSize);

		// Parametrage de l'offset de lecture
		input_lFilePos = lFilePos;
		lFilePos += input_nBufferSize;

		// Calcul de la progression
		dTaskPercent = input_nBufferSize * 1.0 / lInputFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;
	}
	return true;
}

boolean KWKeyPositionSampleExtractorTask::MasterAggregateResults()
{
	boolean bOk = true;
	ObjectArray* oaKeyPositionSample;

	// Test si interruption
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Integration des cles de l'esclave
	if (bOk)
	{
		// Memorisation des clefs envoyees par l'esclave dans un tableau
		oaKeyPositionSample = output_oaKeyPositionSample->GetObjectArray();
		output_oaKeyPositionSample->RemoveObject();

		// Memorisation du nombre de lignes lues par l'esclave
		assert(lvLineCountPerTaskIndex.GetAt(GetTaskIndex()) == 0);
		lvLineCountPerTaskIndex.SetAt(GetTaskIndex(), output_lReadLineCount);

		// Memorisation dans le tableau global, selon le rang de traitement de l'esclave
		assert(oaAllKeyPositionSamples.GetAt(GetTaskIndex()) == NULL);
		oaAllKeyPositionSamples.SetAt(GetTaskIndex(), oaKeyPositionSample);
	}
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* lastPreviousKeyPosition;
	KWKeyPosition* firstNextKeyPosition;
	KWKeyPosition* recordKeyPosition;
	int nCompareKey;
	longint lLineCountToAdd;
	int i;
	int j;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	// Test si arret utilisateur
	bOk = bProcessEndedCorrectly;
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// On consolide les numeros de lignes : on ajoute aux numeros de lignes locaux,
	// le nombre de lignes lues lors des SlaveProcess precedents
	assert(oaAllKeyPositionSamples.GetSize() == lvLineCountPerTaskIndex.GetSize());
	if (bOk)
	{
		lLineCountToAdd = 0;
		for (i = 0; i < oaAllKeyPositionSamples.GetSize(); i++)
		{
			if (i > 0)
			{
				oaKeyPositionSample = cast(ObjectArray*, oaAllKeyPositionSamples.GetAt(i));
				for (j = 0; j < oaKeyPositionSample->GetSize(); j++)
				{
					recordKeyPosition = cast(KWKeyPosition*, oaKeyPositionSample->GetAt(j));
					recordKeyPosition->SetLineIndex(recordKeyPosition->GetLineIndex() +
									lLineCountToAdd);
				}
			}
			lLineCountToAdd += lvLineCountPerTaskIndex.GetAt(i);
		}
	}
	lvLineCountPerTaskIndex.SetSize(0);

	// On fabrique le tableau de cle global renvoye par la tache
	// Chaque sample est deja trie, et les samples sont ranges dans l'ordre des rangs des esclaves, donc dans
	// l'ordre du fichier
	if (bOk)
	{
		// Creation du tableau resultat global par concatenation des tableaux des esclaves
		assert(oaResultKeyPositions.GetSize() == 0);
		for (i = 0; i < oaAllKeyPositionSamples.GetSize(); i++)
		{
			oaKeyPositionSample = cast(ObjectArray*, oaAllKeyPositionSamples.GetAt(i));

			// Chaque esclave repere les positions des cle d'entree localement a sa partie de fichier
			// Potentiellement, la derniere cle identifiee par un esclave ne doit pas etre
			// plus grande que la premiere cle identifiee par l'esclave suivant
			if (i > 0)
			{
				// Recherche de la derniere cle precedente
				lastPreviousKeyPosition = NULL;
				if (oaResultKeyPositions.GetSize() > 0)
					lastPreviousKeyPosition =
					    cast(KWKeyPosition*,
						 oaResultKeyPositions.GetAt(oaResultKeyPositions.GetSize() - 1));

				// Recherche de la premiere cle suivante
				firstNextKeyPosition = NULL;
				if (oaKeyPositionSample->GetSize() > 0)
					firstNextKeyPosition = cast(KWKeyPosition*, oaKeyPositionSample->GetAt(0));

				// Test d'egalite des cle
				if (lastPreviousKeyPosition != NULL and firstNextKeyPosition != NULL)
				{
					nCompareKey = lastPreviousKeyPosition->Compare(firstNextKeyPosition);

					// Supression de la derniere cle du tableau global si egalite, pour ne garder
					// que la cle suivante
					if (nCompareKey == 0)
					{
						oaResultKeyPositions.SetSize(oaResultKeyPositions.GetSize() - 1);
						delete lastPreviousKeyPosition;
					}
					// Erreur si cles non ordonnees
					else if (nCompareKey > 0)
					{
						// Creation de libelles utilisateurs distincts pour les deux cles
						firstNextKeyPosition->GetKey()->BuildDistinctObjectLabels(
						    lastPreviousKeyPosition->GetKey(), sObjectLabel, sOtherObjectLabel);
						AddError(sTmp + "Unsorted record " +
							 LongintToString(firstNextKeyPosition->GetLineIndex()) +
							 " with key " + sObjectLabel + " inferior to preceding key " +
							 sOtherObjectLabel);
						bOk = false;
						break;
					}
				}
			}

			// Insertion du tableau de cles de l'esclave en fin du tableau global
			oaResultKeyPositions.InsertObjectArrayAt(oaResultKeyPositions.GetSize(), oaKeyPositionSample);
			oaKeyPositionSample->SetSize(0);
		}

		// On detruit le tableau uniquement si ok (sinon, nettoyage complet en fin de methode)
		if (bOk)
			oaAllKeyPositionSamples.DeleteAll();
	}

	// Nettoyage si necessaire
	if (not bOk)
	{
		for (i = 0; i < oaAllKeyPositionSamples.GetSize(); i++)
		{
			oaKeyPositionSample = cast(ObjectArray*, oaAllKeyPositionSamples.GetAt(i));
			if (oaKeyPositionSample != NULL)
				oaKeyPositionSample->DeleteAll();
		}
		oaAllKeyPositionSamples.DeleteAll();
	}
	lInputFileSize = 0;

	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::SlaveInitialize()
{
	boolean bOk = true;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sFileName = shared_sInputFileName.GetValue();

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Parametrage du fichier d'entree a analyser pour la tache
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);
	inputFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());

	// Ouverture du fichier en entree
	bOk = inputFile.Open();
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::SlaveProcess()
{
	boolean bOk = true;
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* previousRecordKeyPosition;
	KWKeyPosition* recordKeyPosition;
	longint lDisplayFreshness;
	double dProgression;
	int nCompareKey;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;
	boolean bIsLineOK;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	// Fraicheur d'affichage pour la gestion de la barre de progression
	lDisplayFreshness = 0;

	// Specification de la portion du fichier a traiter
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + input_nBufferSize, inputFile.GetFileSize());

	// Parametrage de la taille du buffer
	inputFile.SetBufferSize(input_nBufferSize);

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	nCumulatedLineNumber = 0;
	if (lBeginPos > 0)
	{
		bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
		if (bOk)
		{
			// On se positionne sur le debut de la ligne suivante si elle est trouvee
			if (lNextLinePos != -1)
			{
				lBeginPos = lNextLinePos;
				nCumulatedLineNumber = 1;
			}
			// Si non trouvee, on se place en fin de chunk
			else
				lBeginPos = lMaxEndPos;
		}
	}
	assert(inputFile.GetCurrentBufferSize() == 0);

	// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
	// On ignorera ainsi le debut de la derniere ligne commencee avant lMaxEndPos
	// Ce n'est pas grave d'ignorer au plus une ligne par buffer, puisque l'on est ici dans une optique
	// d'echantillonnage
	if (bOk and lBeginPos < lMaxEndPos)
		bOk = inputFile.FillInnerLinesUntil(lBeginPos, lMaxEndPos);

	// Initialisation des variables de travail
	keyExtractor.SetBufferedFile(&inputFile);
	oaKeyPositionSample = output_oaKeyPositionSample->GetObjectArray();
	previousRecordKeyPosition = NULL;

	// Analyse des lignes du buffer
	if (bOk and inputFile.GetCurrentBufferSize() > 0)
	{
		assert(inputFile.GetCurrentBufferSize() <= input_nBufferSize);

		// Saut du Header
		// Il n'est pas utile ici d'avoir un warning en cas de ligne trop longue
		if (input_lFilePos == (longint)0 and shared_bHeaderLineUsed)
			inputFile.SkipLine(bLineTooLong);

		// Parcours du buffer d'entree
		while (bOk and not inputFile.IsBufferEnd())
		{
			// Gestion de la progresssion
			lDisplayFreshness++;
			if (TaskProgression::IsRefreshNecessary(lDisplayFreshness))
			{
				// Calcul de la progression par rapport a la proportion de la portion du fichier traitee
				// parce que l'on ne sait pas le nombre total de ligne que l'on va traiter
				dProgression = (inputFile.GetPositionInFile() - input_lFilePos) * 1.0 /
					       (lMaxEndPos - input_lFilePos);
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}

			// Tirage aleatoire pour decider si on garde chaque record
			if (IthRandomDouble(inputFile.GetPositionInFile()) <= shared_dSamplingRate)
			{
				// Extraction de la cle, que la ligne soit valide ou non
				recordKeyPosition = new KWKeyPosition;
				bIsLineOK = keyExtractor.ParseNextKey(recordKeyPosition->GetKey(), NULL);

				// Memorisation du numero de ligne pour la cle
				recordKeyPosition->SetLineIndex((longint)nCumulatedLineNumber +
								inputFile.GetCurrentLineIndex());

				// Memorisation de la position pour le debut de ligne suivant
				recordKeyPosition->SetLinePosition(inputFile.GetPositionInFile());

				// Comparaison de la cle avec la precedente si elle existe
				nCompareKey = -1;
				if (previousRecordKeyPosition != NULL)
					nCompareKey = previousRecordKeyPosition->Compare(recordKeyPosition);

				// Memorisation de la cle si nouvelle cle
				if (nCompareKey < 0)
				{
					oaKeyPositionSample->Add(recordKeyPosition);
					previousRecordKeyPosition = recordKeyPosition;
				}
				// Destruction si egalite
				else if (nCompareKey == 0)
					delete recordKeyPosition;
				// Erreur si cles non ordonnees
				else
				{
					// Creation de libelles utilisateurs distincts pour les deux cles
					recordKeyPosition->GetKey()->BuildDistinctObjectLabels(
					    previousRecordKeyPosition->GetKey(), sObjectLabel, sOtherObjectLabel);
					AddLocalError(
					    "unsorted record with key " + sObjectLabel + " inferior to preceding key " +
						sOtherObjectLabel + " found " +
						LongintToReadableString(recordKeyPosition->GetLineIndex() -
									previousRecordKeyPosition->GetLineIndex()) +
						" lines before",
					    nCumulatedLineNumber + inputFile.GetCurrentLineIndex());
					delete recordKeyPosition;
					bOk = false;
					break;
				}
			}
			// Saut de ligne sinon
			else
				inputFile.SkipLine(bLineTooLong);
		}

		// Prise en compte des lignes du buffer
		nCumulatedLineNumber += inputFile.GetBufferLineNumber();
	}

	// On indique le nombre de lignes traitees, en fin de SlaveProcess
	if (bOk)
	{
		SetLocalLineNumber(nCumulatedLineNumber);
		output_lReadLineCount = nCumulatedLineNumber;
	}

	// Nettoyage si KO
	if (not bOk)
		oaKeyPositionSample->DeleteAll();
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage
	keyExtractor.Clean();

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
}

int KWSortedKeyPositionArrayCompare(const void* elem1, const void* elem2)
{
	ObjectArray* oaSortedKeyPositions1;
	ObjectArray* oaSortedKeyPositions2;
	KWKeyPosition* keyPosition1;
	KWKeyPosition* keyPosition2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux objets
	oaSortedKeyPositions1 = cast(ObjectArray*, *(Object**)elem1);
	oaSortedKeyPositions2 = cast(ObjectArray*, *(Object**)elem2);
	assert(oaSortedKeyPositions1->GetSize() > 0);
	assert(oaSortedKeyPositions2->GetSize() > 0);

	// Acces aux cles
	keyPosition1 = cast(KWKeyPosition*, oaSortedKeyPositions1->GetAt(0));
	keyPosition2 = cast(KWKeyPosition*, oaSortedKeyPositions2->GetAt(0));
	return keyPosition1->Compare(keyPosition2);
}
