// Copyright (c) 2023 Orange. All rights reserved.
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

	require(oaKeyPositions != NULL);
	require(sFileName != ""); // Le fichier d'entree doit etre renseigne
	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(lKeyUsedMemory > 0);
	require(lFileLineNumber > 0);

	// Execution de la tache
	bOk = Run();

	// Collecte des resultats
	if (bOk and not TaskProgression::IsInterruptionRequested())
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
	return bOk;
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
	int nBufferLineNumber;
	longint lKeyPositionUsedMemory;
	int nBufferSize;

	// Memoire necessaire pour stocker une cle et sa position
	lKeyPositionUsedMemory = lKeyUsedMemory + sizeof(KWKeyPosition) - sizeof(KWKey) + sizeof(Object*);

	// Memoire pour memoriser l'echantillon de cle global
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(
	    (longint)ceil(shared_dSamplingRate * lFileLineNumber * lKeyPositionUsedMemory *
			  2)); // Taille du sample (*2 pour etre tranquille)

	// Estimation du nombre de clefs dans un buffer
	lInputFileSize = PLRemoteFileService::GetFileSize(sFileName);
	nBufferSize = BufferedFile::nDefaultBufferSize;
	if (nBufferSize > lInputFileSize)
	{
		assert(lInputFileSize < INT_MAX);
		nBufferSize = (int)(lInputFileSize + 1);
	}
	nBufferLineNumber = (int)ceil(lFileLineNumber * 1.0 * nBufferSize / (lInputFileSize + 1));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(
	    nBufferSize + nBufferLineNumber * lKeyPositionUsedMemory * 2);

	// Nombre max de slaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / nBufferSize));
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::MasterInitialize()
{
	boolean bOk = true;

	require(oaAllKeyPositionSamples.GetSize() == 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(0.0 < shared_dSamplingRate and shared_dSamplingRate <= 1.0);

	// Initialisation des variables partagees
	shared_sInputFileName.SetValue(sFileName);
	shared_bHeaderLineUsed = bHeaderLineUsed;
	shared_cFieldSeparator.SetValue(cFieldSeparator);

	lFilePos = 0;

	// Test si fichier d'entree present
	if (not PLRemoteFileService::Exist(sFileName))
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
		input_nBufferSize =
		    ComputeStairBufferSize(lMB, BufferedFile::nDefaultBufferSize, lFilePos, lInputFileSize);

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
	int i, j;
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* lastPreviousKeyPosition;
	KWKeyPosition* firstNextKeyPosition;
	KWKeyPosition* recordKeyPosition;
	int nCompareKey;
	longint lLineCountToAdd;
	ALString sTmp;

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
	// Chaque sample est deja trie, et les sample sont range dans l'ordre des rang des esclaves, donc dans l'odre du
	// fichier
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

					// Erreur si cles non ordonnees
					if (nCompareKey > 0)
					{
						AddError(sTmp + "Record " +
							 LongintToString(firstNextKeyPosition->GetLineIndex()) +
							 " : key " + firstNextKeyPosition->GetKey()->GetObjectLabel() +
							 " inferior to preceding key " +
							 lastPreviousKeyPosition->GetKey()->GetObjectLabel());
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
	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sFileName = shared_sInputFileName.GetValue();

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());
	return true;
}

boolean KWKeyPositionSampleExtractorTask::SlaveProcess()
{
	boolean bOk = true;
	InputBufferedFile inputFile;
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* previousRecordKeyPosition;
	KWKeyPosition* recordKeyPosition;
	int nLineIndex;
	PeriodicTest periodicTestInterruption;
	double dProgression;
	ALString sTmp;
	boolean bIsOpen;

	// Initialisation du buffer de lecture
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);
	inputFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());
	inputFile.SetBufferSize(input_nBufferSize);

	// Index des lignes traitees localement
	nLineIndex = 0;

	// Ouverture et remplissage du buffer de lecture
	bIsOpen = inputFile.Open();
	if (bIsOpen)
	{
		bOk = inputFile.Fill(input_lFilePos);
	}
	else
		bOk = false;
	if (bOk)
	{
		SetLocalLineNumber(inputFile.GetBufferLineNumber());
		output_lReadLineCount = inputFile.GetBufferLineNumber();

		// Index des lignes traitees localement
		nLineIndex = 0;

		// Saut du Header
		if (input_lFilePos == (longint)0 and shared_bHeaderLineUsed)
		{
			inputFile.SkipLine();
			nLineIndex++;
		}
	}
	// Parcours du buffer d'entree
	oaKeyPositionSample = output_oaKeyPositionSample->GetObjectArray();
	previousRecordKeyPosition = NULL;
	keyExtractor.SetBufferedFile(&inputFile);
	while (bOk and not inputFile.IsBufferEnd())
	{
		nLineIndex++;

		// Gestion de la progesssion
		if (periodicTestInterruption.IsTestAllowed(nLineIndex))
		{
			dProgression = inputFile.GetCurrentLineNumber() * 1.0 / inputFile.GetBufferLineNumber();
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
			// Extraction de la cle
			keyExtractor.ParseNextKey(NULL);
			recordKeyPosition = new KWKeyPosition;
			keyExtractor.ExtractKey(recordKeyPosition->GetKey());

			// Memorisation du numero de ligne pour la cle
			recordKeyPosition->SetLineIndex(nLineIndex);

			// Memorisation de la position pour le debut de ligne suivant
			recordKeyPosition->SetLinePosition(inputFile.GetPositionInFile());

			// Memorisation de la cle
			oaKeyPositionSample->Add(recordKeyPosition);

			// Test si les cle etaient ordonnees
			if (previousRecordKeyPosition != NULL and
			    previousRecordKeyPosition->Compare(recordKeyPosition) > 0)
			{
				AddLocalError("key " + recordKeyPosition->GetObjectLabel() +
						  " inferior to preceding key " +
						  previousRecordKeyPosition->GetObjectLabel(),
					      nLineIndex);
				bOk = false;
				break;
			}
			previousRecordKeyPosition = recordKeyPosition;
		}
		// Saut de ligne sinon
		else
			inputFile.SkipLine();
	}

	// Nettoyage si KO
	if (not bOk)
		oaKeyPositionSample->DeleteAll();
	if (bIsOpen)
		inputFile.Close();
	return bOk;
}

boolean KWKeyPositionSampleExtractorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	// Nettoyage
	keyExtractor.Clean();

	return true;
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