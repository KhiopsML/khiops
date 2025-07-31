// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKeyPositionFinderTask.h"
#include "KWKeyExtractor.h"
#include "TaskProgression.h"

KWKeyPositionFinderTask::KWKeyPositionFinderTask()
{
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';
	nForcedBufferSize = 0;
	lFilePos = 0;
	lInputFileSize = 0;
	lInputKeysUsedMemory = 0;
	lOutputKeysUsedMemory = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	nReadBufferSize = 0;

	// Variables partagees
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	shared_oaInputKeys = new PLShared_ObjectArray(new PLShared_Key);
	DeclareSharedParameter(shared_oaInputKeys);
	DeclareSharedParameter(&shared_sInputFileName);
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_cFieldSeparator);

	// On munit le tableau des cle en entree d'une fonction de comparaison
	shared_oaInputKeys->GetObjectArray()->SetCompareFunction(KWKeyCompare);

	// Variables en entree et sortie des esclaves
	output_oaKeyPositionSubset = new PLShared_ObjectArray(new PLShared_KeyPosition);
	DeclareTaskOutput(output_oaKeyPositionSubset);
	DeclareTaskOutput(&output_SlaveFirstKeyPosition);
	DeclareTaskOutput(&output_SlaveLastKeyPosition);
	DeclareTaskOutput(&output_lLineNumber);
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);
}

KWKeyPositionFinderTask::~KWKeyPositionFinderTask()
{
	assert(oaAllKeyPositionSubsets.GetSize() == 0);
	assert(oaResultKeyPositions.GetSize() == 0);
	delete shared_oaInputKeys;
	delete output_oaKeyPositionSubset;
}

void KWKeyPositionFinderTask::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

const ALString& KWKeyPositionFinderTask::GetFileName() const
{
	return sFileName;
}

void KWKeyPositionFinderTask::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean KWKeyPositionFinderTask::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

void KWKeyPositionFinderTask::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

char KWKeyPositionFinderTask::GetFieldSeparator() const
{
	return cFieldSeparator;
}

IntVector* KWKeyPositionFinderTask::GetKeyFieldIndexes()
{
	return shared_ivKeyFieldIndexes.GetIntVector();
}

const IntVector* KWKeyPositionFinderTask::GetConstKeyFieldIndexes() const
{
	return shared_ivKeyFieldIndexes.GetConstIntVector();
}

boolean KWKeyPositionFinderTask::FindKeyPositions(const ObjectArray* oaInputKeys, ObjectArray* oaFoundKeyPositions)
{
	boolean bOk = true;
	boolean bDisplay = false;

	require(oaInputKeys != NULL);
	require(CheckKeys(oaInputKeys));
	require(oaFoundKeyPositions != NULL);
	require(sFileName != ""); // Le fichier d'entree doit etre renseigne
	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(oaInputKeys->GetSize() == 0 or
		(cast(KWKey*, oaInputKeys->GetAt(0))->GetSize() == shared_ivKeyFieldIndexes.GetSize()));
	require(shared_oaInputKeys->GetObjectArray()->GetSize() == 0);
	require(oaResultKeyPositions.GetSize() == 0);

	// Execution de la tache, en recopiant temporairement les cle en entree
	shared_oaInputKeys->GetObjectArray()->CopyFrom(oaInputKeys);
	bOk = Run();
	shared_oaInputKeys->GetObjectArray()->RemoveAll();

	// Collecte des resultats
	assert(oaAllKeyPositionSubsets.GetSize() == 0);
	if (bOk and not IsTaskInterruptedByUser())
	{
		oaFoundKeyPositions->CopyFrom(&oaResultKeyPositions);
		oaResultKeyPositions.RemoveAll();
	}
	// Nettoyage sinon
	else
	{
		oaFoundKeyPositions->RemoveAll();

		// Destruction des cles et leur position
		oaResultKeyPositions.DeleteAll();
		bOk = false;
	}

	// Affichage
	if (bDisplay)
	{
		cout << GetClassLabel() << "\t" << bOk << "\n";
		cout << "Input keys\n";
		WriteKeys(oaInputKeys, cout);
		cout << "Found key positions\n";
		WriteKeys(oaFoundKeyPositions, cout);
		cout << endl;
	}

	ensure(not bOk or CheckKeyPositionsConsistency(oaInputKeys, oaFoundKeyPositions));
	return bOk;
}

boolean KWKeyPositionFinderTask::CheckKeys(const ObjectArray* oaKeys) const
{
	boolean bOk = true;
	KWKey* previousKey;
	KWKey* key;
	int i;

	require(oaKeys != NULL);

	for (i = 1; i < oaKeys->GetSize(); i++)
	{
		previousKey = cast(KWKey*, oaKeys->GetAt(i - 1));
		key = cast(KWKey*, oaKeys->GetAt(i));
		bOk = bOk and previousKey->Compare(key) < 0;
	}
	return bOk;
}

boolean KWKeyPositionFinderTask::CheckKeyPositions(const ObjectArray* oaKeyPositions) const
{
	boolean bOk = true;
	KWKeyPosition* previousKeyPosition;
	KWKeyPosition* keyPosition;
	int i;

	require(oaKeyPositions != NULL);

	for (i = 1; i < oaKeyPositions->GetSize(); i++)
	{
		previousKeyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i - 1));
		keyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i));
		bOk = bOk and previousKeyPosition->Compare(keyPosition) < 0;
		bOk = bOk and previousKeyPosition->GetLineIndex() <= keyPosition->GetLineIndex();
		bOk = bOk and previousKeyPosition->GetLinePosition() <= keyPosition->GetLinePosition();
		assert(bOk);
	}
	return bOk;
}

boolean KWKeyPositionFinderTask::CheckKeyPositionsConsistency(const ObjectArray* oaKeys,
							      const ObjectArray* oaKeyPositions) const
{
	boolean bOk = true;
	KWKey* key;
	KWKeyPosition* keyPosition;
	int i;

	require(oaKeys != NULL);
	require(oaKeyPositions != NULL);

	// Test globaux
	bOk = bOk and oaKeys->GetSize() == oaKeyPositions->GetSize();
	bOk = bOk and CheckKeys(oaKeys);
	bOk = bOk and CheckKeyPositions(oaKeyPositions);

	// Verification de la coherence des cle entre les deux tableaux
	if (bOk)
	{
		for (i = 0; i < oaKeys->GetSize(); i++)
		{
			key = cast(KWKey*, oaKeys->GetAt(i));
			keyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i));
			bOk = bOk and keyPosition->GetKey()->Compare(key) == 0;
		}
	}
	return bOk;
}

void KWKeyPositionFinderTask::WriteKeys(const ObjectArray* oaKeys, ostream& ost) const
{
	int i;

	require(oaKeys != NULL);

	ost << "Keys\t" << oaKeys->GetSize() << "\n";
	for (i = 0; i < oaKeys->GetSize(); i++)
		ost << "\t" << i << "\t" << *oaKeys->GetAt(i) << endl;
}

void KWKeyPositionFinderTask::WriteKeyPositions(const ObjectArray* oaKeyPositions, ostream& ost) const
{
	int i;

	require(oaKeyPositions != NULL);

	ost << "Key positions\t" << oaKeyPositions->GetSize() << "\n";
	for (i = 0; i < oaKeyPositions->GetSize(); i++)
		ost << "\t" << i << "\t" << *oaKeyPositions->GetAt(i) << endl;
}

const ALString KWKeyPositionFinderTask::GetObjectLabel() const
{
	return sFileName;
}

void KWKeyPositionFinderTask::Test()
{
	TestWithArtificialMainAndSecondaryTables(100000, 1, 1, 1000000, 10, 1, 0);
	TestWithArtificialMainAndSecondaryTables(10000, 1, 1, 100000, 10, 1, 0);
	TestWithArtificialMainAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0.01, 0);
	TestWithArtificialMainAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0.01, 1000);
	TestWithArtificialMainAndSecondaryTables(100000, 10, 0.01, 1000000, 1, 0.01, 1000);
	TestWithArtificialMainAndSecondaryTables(100000, 1, 0, 1000000, 10, 0.01, 1000);
	TestWithArtificialMainAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0, 1000);
	TestWithArtificialMainAndSecondaryTables(1, 1, 1, 1000000, 10, 0.01, 1000);
}

boolean KWKeyPositionFinderTask::TestWithArtificialMainAndSecondaryTables(
    int nMainLineNumber, int nMainLineNumberPerKey, double dMainSamplingRate, int nSecondaryLineNumber,
    int nSecondaryLineNumberPerKey, double dSecondarySamplingRate, int nBufferSize)
{
	boolean bOk = true;
	boolean bCreateDatasets = true;
	KWArtificialDataset mainArtificialDataset;
	KWArtificialDataset secondaryArtificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaInputKeyPositions;
	ObjectArray oaInputKeys;
	ObjectArray oaFoundKeyPositions;
	KWKeyPositionFinderTask keyPositionFinder;

	require(nMainLineNumber >= 0);
	require(nMainLineNumberPerKey >= 0);
	require(dMainSamplingRate >= 0);
	require(nSecondaryLineNumber >= 0);
	require(nSecondaryLineNumberPerKey >= 0);
	require(dSecondarySamplingRate >= 0);
	require(nBufferSize >= 0);

	// Gestion des taches
	TaskProgression::SetTitle("Test " + keyPositionFinder.GetTaskLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Affichage d'un entete de test
	cout << endl;
	cout << "===============================================================================" << endl;
	cout << keyPositionFinder.GetTaskLabel() << ":"
	     << " Main(" << nMainLineNumber << ", " << nMainLineNumberPerKey << ", " << dMainSamplingRate << ")"
	     << " Secondary(" << nSecondaryLineNumber << ", " << nSecondaryLineNumberPerKey << ", "
	     << dSecondarySamplingRate << ")"
	     << " Buffer(" << nBufferSize << ")" << endl;
	cout << "===============================================================================" << endl;

	// Creation d'un fichier principal avec des champs cle
	mainArtificialDataset.SpecifySortedDataset();
	mainArtificialDataset.SetLineNumber(nMainLineNumber);
	mainArtificialDataset.SetMaxLineNumberPerKey(nMainLineNumberPerKey);
	mainArtificialDataset.SetSamplingRate(dMainSamplingRate);
	mainArtificialDataset.SetFileName(mainArtificialDataset.BuildFileName());
	if (bCreateDatasets)
		mainArtificialDataset.CreateDataset();
	mainArtificialDataset.DisplayFirstLines(15);

	// Evaluation de la taille des cles du fichier principal
	lMeanKeySize = 0;
	lLineNumber = 0;
	bOk = bOk and
	      KWKeySizeEvaluatorTask::TestWithArtificialDataset(&mainArtificialDataset, lMeanKeySize, lLineNumber);

	// Extraction des cle du fichier principal
	bOk = bOk and KWKeyPositionSampleExtractorTask::TestWithArtificialDataset(
			  &mainArtificialDataset, 0.25, lMeanKeySize, lLineNumber, &oaInputKeyPositions);

	// Creation d'un fichier secondaire avec des champs cle
	secondaryArtificialDataset.SpecifySortedDataset();
	secondaryArtificialDataset.SetLineNumber(nSecondaryLineNumber);
	secondaryArtificialDataset.SetMaxLineNumberPerKey(nSecondaryLineNumberPerKey);
	secondaryArtificialDataset.SetSamplingRate(dSecondarySamplingRate);
	secondaryArtificialDataset.SetFileName(secondaryArtificialDataset.BuildFileName());
	if (bCreateDatasets)
		secondaryArtificialDataset.CreateDataset();
	secondaryArtificialDataset.DisplayFirstLines(15);

	// Collecte des cle du fichier secondaire
	KWKeyPosition::CollectKeys(&oaInputKeyPositions, &oaInputKeys);
	bOk = bOk and KWKeyPositionFinderTask::TestWithArtificialDataset(&secondaryArtificialDataset, &oaInputKeys,
									 &oaFoundKeyPositions, nBufferSize);

	// Nettoyage
	oaInputKeyPositions.DeleteAll();
	oaFoundKeyPositions.DeleteAll();

	// Destruction des fichiers
	if (bCreateDatasets)
	{
		mainArtificialDataset.DeleteDataset();
		secondaryArtificialDataset.DeleteDataset();
	}

	// Gestion des taches
	TaskProgression::Stop();
	return bOk;
}

boolean KWKeyPositionFinderTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
							   const ObjectArray* oaInputKeys,
							   ObjectArray* oaFoundKeyPositions, int nBufferSize)
{
	boolean bOk = true;
	KWKeyPositionFinderTask keyPositionFinder;
	int i;

	require(artificialDataset != NULL);
	require(nBufferSize >= 0);

	// Extraction des cles
	keyPositionFinder.SetFileName(artificialDataset->GetFileName());
	keyPositionFinder.SetHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
	keyPositionFinder.SetFieldSeparator(artificialDataset->GetFieldSeparator());
	keyPositionFinder.GetKeyFieldIndexes()->CopyFrom(artificialDataset->GetConstKeyFieldIndexes());
	keyPositionFinder.nForcedBufferSize = nBufferSize;
	bOk = bOk and keyPositionFinder.FindKeyPositions(oaInputKeys, oaFoundKeyPositions);
	cout << "Found key positions: " << oaFoundKeyPositions->GetSize() << " (" << bOk << ")" << endl;
	for (i = 0; i < oaFoundKeyPositions->GetSize(); i++)
	{
		cout << "\t" << *oaFoundKeyPositions->GetAt(i) << endl;
		if (i > 10)
			break;
	}
	return bOk;
}

const ALString KWKeyPositionFinderTask::GetTaskName() const
{
	return "Key position finder";
}

PLParallelTask* KWKeyPositionFinderTask::Create() const
{
	return new KWKeyPositionFinderTask;
}

boolean KWKeyPositionFinderTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	int i;
	KWKey* key;
	int nPreferredSize;

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

	// Memoire necessaire pour stocker les cles en entree
	lInputKeysUsedMemory = 0;
	for (i = 0; i < shared_oaInputKeys->GetObjectArray()->GetSize(); i++)
	{
		key = cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(i));
		lInputKeysUsedMemory += key->GetUsedMemory() + sizeof(Object*);
	}

	// Exigences sur les variables partagees
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->Set(lInputKeysUsedMemory *
									    2); // On double pour etre tranquille

	// Memoire necessaire pour stocker les positions de cle en sortie
	lOutputKeysUsedMemory =
	    shared_oaInputKeys->GetObjectArray()->GetSize() * (sizeof(KWKeyPosition) + sizeof(Object*));

	// Memoire pour memoriser l'echantillon de cle global
	// On ne prevoit de memoire additionnelle pour les permieres et dernieres cles par esclave, qui sont detruites
	// apres verification
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(
	    lOutputKeysUsedMemory * 2); // Taille du sample (*2 pour etre tranquille)

	// Memoire par esclave: les cles en entree, plus le buffer, plus les cle par buffer traite, au prorata de la
	// taille du buffer
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(
	    lInputKeysUsedMemory + nReadSizeMin + (2 * lInputKeysUsedMemory * nReadSizeMin) / (lInputFileSize + 1));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(
	    lInputKeysUsedMemory + nReadSizeMax + (2 * lInputKeysUsedMemory * nReadSizeMax) / (lInputFileSize + 1));

	// Nombre max de slaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / (nReadSizeMin + 1)));
	return bOk;
}

boolean KWKeyPositionFinderTask::MasterInitialize()
{
	boolean bOk = true;
	boolean bTrace = false;
	longint lSlaveGrantedMemory;
	longint lReadBufferSize;
	int nPreferredBufferSize;
	ALString sTmp;

	require(oaAllKeyPositionSubsets.GetSize() == 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(oaAllSlaveFirstKeyPositions.GetSize() == 0);
	require(oaAllSlaveLastKeyPositions.GetSize() == 0);

	// Affichage des cle en entree
	if (bTrace)
	{
		cout << GetTaskName() << "\tInput keys" << endl;
		WriteKeys(shared_oaInputKeys->GetObjectArray(), cout);
	}

	// Parametrage du fichier en lecture
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

	// Test si cle en entree
	if (bOk and shared_oaInputKeys->GetObjectArray()->GetSize() == 0)
	{
		AddError("No input keys: no position to extract");
		bOk = false;
	}

	// Calcul de la taille du buffer en fonction de la memoire disponible pour les esclaves, selon la formule
	// suivante similaire aux exigences min et max
	//   lSlaveGrantedMemory = lInputKeysUsedMemory + nReadBufferSize + (2 * lInputKeysUsedMemory* nReadBufferSize)
	//   / (lInputFileSize + 1)
	// Soit
	//   lSlaveGrantedMemory - lInputKeysUsedMemory = nReadBufferSize * (1 + (2 * lInputKeysUsedMemory) /
	//   (lInputFileSize + 1) nReadBufferSize = (lSlaveGrantedMemory - lInputKeysUsedMemory) / (1 + (2 *
	//   lInputKeysUsedMemory) / (lInputFileSize + 1)
	lSlaveGrantedMemory = GetSlaveResourceGrant()->GetMemory();
	lReadBufferSize = longint((lSlaveGrantedMemory - lInputKeysUsedMemory) /
				  (1 + (2.0 * lInputKeysUsedMemory) / (lInputFileSize + 1)));

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

boolean KWKeyPositionFinderTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Est-ce qu'il y a encore du travail ?
	if (lFilePos >= lInputFileSize)
		bIsTaskFinished = true;
	else
	{
		// On agrandit la taille du tableau des resultats des esclaves
		oaAllKeyPositionSubsets.Add(NULL);
		lvLineCountPerTaskIndex.Add(0);

		// Idem pour les tableaux des premieres et dernieres cles
		oaAllSlaveFirstKeyPositions.Add(NULL);
		oaAllSlaveLastKeyPositions.Add(NULL);

		// Parametrage de la taille du buffer
		input_nBufferSize = ComputeStairBufferSize(nReadSizeMin, nReadBufferSize,
							   PLRemoteFileService::GetPreferredBufferSize(sFileName),
							   lFilePos, lInputFileSize);

		// On peut imposer la taille du buffer pour raison de tests
		if (nForcedBufferSize > 0)
		{
			input_nBufferSize = nForcedBufferSize;
			if (lFilePos + input_nBufferSize > lInputFileSize)
				input_nBufferSize = int(lInputFileSize - input_nBufferSize);
		}

		// Parametrage de la position de lecture
		input_lFilePos = lFilePos;
		lFilePos += input_nBufferSize;

		// Calcul de la progression
		dTaskPercent = input_nBufferSize * 1.0 / lInputFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;
	}
	return true;
}

boolean KWKeyPositionFinderTask::MasterAggregateResults()
{
	boolean bOk = true;
	boolean bTrace = false;
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* keyPosition1;
	KWKeyPosition* keyPosition2;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	// Test si interruption
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Integration des cles de l'esclave
	if (bOk)
	{
		// Affichage
		if (bTrace)
		{
			cout << "KWKeyPositionFinderTask::MasterAggregateResults\t" << GetTaskIndex() << endl;
			cout << "Lines\t" << output_lLineNumber << endl;
			WriteKeyPositions(output_oaKeyPositionSubset->GetObjectArray(), cout);
			cout << endl;
		}

		// Memorisation du nombre de ligne
		lvLineCountPerTaskIndex.SetAt(GetTaskIndex(), output_lLineNumber);

		// Memorisation des cles et de leur positions
		oaKeyPositionSample = output_oaKeyPositionSubset->GetObjectArray();
		output_oaKeyPositionSubset->RemoveObject();

		// Memorisation dans le tableau global, selon le rang de traitement de l'esclave
		assert(oaAllKeyPositionSubsets.GetAt(GetTaskIndex()) == NULL);
		oaAllKeyPositionSubsets.SetAt(GetTaskIndex(), oaKeyPositionSample);

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Gestion de la premiere et derniere cle trouvee par esclave, et controle de validite des que possible
		// Tous les controles de coherence sur l'ordre des cles entre deux esclaves successifs sont effectues
		// ici

		// Memorisation dans le tableau global de la premiere cle de l'esclave
		assert(oaAllSlaveFirstKeyPositions.GetAt(GetTaskIndex()) == NULL);
		oaAllSlaveFirstKeyPositions.SetAt(GetTaskIndex(), output_SlaveFirstKeyPosition.GetKeyPosition());
		output_SlaveFirstKeyPosition.RemoveObject();

		// Memorisation dans le tableau global de la derniere cle de l'esclave
		assert(oaAllSlaveLastKeyPositions.GetAt(GetTaskIndex()) == NULL);
		oaAllSlaveLastKeyPositions.SetAt(GetTaskIndex(), output_SlaveLastKeyPosition.GetKeyPosition());
		output_SlaveLastKeyPosition.RemoveObject();

		// Test de coherence entre la premiere cle retournee et la derniere cle de l'esclave precedent, s'il est
		// deja memorise
		if (bOk and GetTaskIndex() > 0 and oaAllSlaveLastKeyPositions.GetAt(GetTaskIndex() - 1) != NULL)
		{
			// Acces au cles a comparer
			keyPosition1 = cast(KWKeyPosition*, oaAllSlaveLastKeyPositions.GetAt(GetTaskIndex() - 1));
			keyPosition2 = cast(KWKeyPosition*, oaAllSlaveFirstKeyPositions.GetAt(GetTaskIndex()));

			// Comparaison s'il elle ont ete extraites
			if (keyPosition1->GetLineIndex() != 0 and keyPosition2->GetLineIndex() != 0)
			{
				if (keyPosition1->Compare(keyPosition2) > 0)
				{
					// Creation de libelles utilisateurs distincts pour les deux cles
					keyPosition2->GetKey()->BuildDistinctObjectLabels(
					    keyPosition1->GetKey(), sObjectLabel, sOtherObjectLabel);
					// On renonce a preciser le numero de ligne, car il faudrait cumuler les nombre
					// de lignes lues pour les esclaves precedents, et tous n'ont peut-etre pas
					// termine
					AddError(sTmp + "Unsorted record with key " + sObjectLabel +
						 " inferior to key " + sOtherObjectLabel +
						 " of a record found previously");
					bOk = false;
				}
			}

			// Ce controle etant effectue, on peut supprimer les cles correspondantes pour gagner de la
			// memoire
			delete keyPosition1;
			delete keyPosition2;
			oaAllSlaveLastKeyPositions.SetAt(GetTaskIndex() - 1, NULL);
			oaAllSlaveFirstKeyPositions.SetAt(GetTaskIndex(), NULL);
		}

		// Test de coherence entre la premiere cle retournee et la derniere cle de l'esclave precedent, s'il est
		// deja memorise
		if (bOk and oaAllSlaveFirstKeyPositions.GetSize() > GetTaskIndex() + 1 and
		    oaAllSlaveFirstKeyPositions.GetAt(GetTaskIndex() + 1) != NULL)
		{
			// Acces au cles a comparer
			keyPosition1 = cast(KWKeyPosition*, oaAllSlaveLastKeyPositions.GetAt(GetTaskIndex()));
			keyPosition2 = cast(KWKeyPosition*, oaAllSlaveFirstKeyPositions.GetAt(GetTaskIndex() + 1));

			// Comparaison si elles ont ete extraites
			if (keyPosition1->GetLineIndex() != 0 and keyPosition2->GetLineIndex() != 0)
			{
				if (keyPosition1->Compare(keyPosition2) > 0)
				{
					// Creation de libelles utilisateurs distincts pour les deux cles
					keyPosition2->GetKey()->BuildDistinctObjectLabels(
					    keyPosition1->GetKey(), sObjectLabel, sOtherObjectLabel);
					// On renonce a preciser le numero de ligne, car il faudrait cumuler les
					// nombre de lignes lues pour les esclaves precedents, et tous n'ont peut-etre
					// pas termine
					AddError(sTmp + "Unsorted record with key " + sObjectLabel +
						 " inferior to key " + sOtherObjectLabel +
						 " of a record found previously");
					bOk = false;
				}
			}

			// Ce controle etant effectue, on peut supprimer les cles correspondantes pour gagner de la
			// memoire
			delete keyPosition1;
			delete keyPosition2;
			oaAllSlaveLastKeyPositions.SetAt(GetTaskIndex(), NULL);
			oaAllSlaveFirstKeyPositions.SetAt(GetTaskIndex() + 1, NULL);
		}
	}
	return bOk;
}

boolean KWKeyPositionFinderTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	boolean bTrace = false;
	int i;
	ObjectArray* oaKeyPositionSubset;
	int j;
	KWKey* key;
	KWKeyPosition* keyPosition;
	KWKeyPosition* newKeyPosition;
	KWKeyPosition* lastPreviousKeyPosition;
	KWKeyPosition* firstNextKeyPosition;
	ObjectArray oaMasterResultKeyPositions;
	int nCompareKey;
	int nDuplicateNumber;
	longint lLineCountToAdd;
	longint lTotalLineNumber;
	ALString sTmp;

	// Test si arret utilisateur
	bOk = bProcessEndedCorrectly;
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// On consolide les numeros de lignes : on ajoute aux numeros de lignes locaux,
	// le nombre de lignes lues lors des SlaveProcess precedents
	lTotalLineNumber = 0;
	if (bOk)
	{
		lLineCountToAdd = 0;
		for (i = 0; i < oaAllKeyPositionSubsets.GetSize(); i++)
		{
			if (i > 0)
			{
				oaKeyPositionSubset = cast(ObjectArray*, oaAllKeyPositionSubsets.GetAt(i));
				for (j = 0; j < oaKeyPositionSubset->GetSize(); j++)
				{
					keyPosition = cast(KWKeyPosition*, oaKeyPositionSubset->GetAt(j));
					keyPosition->SetLineIndex(keyPosition->GetLineIndex() + lLineCountToAdd);
				}
			}
			lLineCountToAdd += lvLineCountPerTaskIndex.GetAt(i);
		}

		// Memorisation du nombre total de lignes du fichier
		lTotalLineNumber = lLineCountToAdd;
	}
	lvLineCountPerTaskIndex.SetSize(0);

	// Concatenation des tableaux de cle envoyes par les esclaves
	if (bOk)
	{
		// Creation du tableau resultat global par concatenation des tableaux des esclaves
		assert(oaResultKeyPositions.GetSize() == 0);
		for (i = 0; i < oaAllKeyPositionSubsets.GetSize(); i++)
		{
			oaKeyPositionSubset = cast(ObjectArray*, oaAllKeyPositionSubsets.GetAt(i));
			check(oaKeyPositionSubset);

			// Chaque esclave repere les positions des cle d'entree localement a sa partie de fichier
			// Potentiellement, la derniere position identifiee par un esclave correspond a la
			// meme cle d'entree que la premiere position identifiee par l'esclave suivant
			// Il faut de-dupliquer cette position, en ne gardant que la premiere
			// Attention, il peut y avoir plusieurs cle successives egales dans les cle en entres (doublons)
			// Dans ce cas, il y a autant de positions de cle en sortie que de doublons dans chaque esclave,
			// et il faut supprimer tous les doublons entre les resultats de deux esclaves consecutifs
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
				if (oaKeyPositionSubset->GetSize() > 0)
					firstNextKeyPosition = cast(KWKeyPosition*, oaKeyPositionSubset->GetAt(0));

				// Test d'egalite des cle
				if (lastPreviousKeyPosition != NULL and firstNextKeyPosition != NULL)
				{
					nCompareKey = lastPreviousKeyPosition->Compare(firstNextKeyPosition);

					// Erreur si cles non ordonnees
					if (nCompareKey > 0)
					{
						// On connait la position du probleme, la cle recherchee correspondant,
						// mais pas la cle trouvee (non memorisee), d'ou un message
						// entierement explicite
						AddError(sTmp + "Unsorted record " +
							 LongintToString(firstNextKeyPosition->GetLineIndex()) +
							 " with key " +
							 firstNextKeyPosition->GetKey()->GetObjectLabel() +
							 " inferior to a key found previously, beyond record " +
							 LongintToString(lastPreviousKeyPosition->GetLineIndex()));
						bOk = false;
						break;
					}
					// Supression des cles en double si cle egales
					else if (nCompareKey == 0)
					{
						// Calcul du nombre de doublons dans le tableau suivant
						nDuplicateNumber = 1;
						for (j = 1; j < oaKeyPositionSubset->GetSize(); j++)
						{
							keyPosition =
							    cast(KWKeyPosition*, oaKeyPositionSubset->GetAt(j));

							// Doublon si cle egale a la premiere cle du tableau
							if (firstNextKeyPosition->Compare(keyPosition) == 0)
								nDuplicateNumber++;
							else
								break;
						}

						// Affichage
						if (bTrace)
							cout << GetClassLabel() << "\tMF\t=>remove duplicate key ("
							     << nDuplicateNumber << ") " << *firstNextKeyPosition
							     << endl;

						// Boucle de suppression des positions de cle en double
						for (j = 0; j < nDuplicateNumber; j++)
						{
							// Recherche de la paire (lastPreviousKeyPosition,
							// firstNextKeyPosition) a dedoublonner
							lastPreviousKeyPosition = cast(
							    KWKeyPosition*,
							    oaResultKeyPositions.GetAt(oaResultKeyPositions.GetSize() -
										       nDuplicateNumber + j));
							firstNextKeyPosition =
							    cast(KWKeyPosition*, oaKeyPositionSubset->GetAt(j));

							// Remplacement de prochaine cle qui sera inseree
							oaKeyPositionSubset->SetAt(j, lastPreviousKeyPosition);

							// Destruction de la cle remplacee
							assert(lastPreviousKeyPosition->GetKey()->Compare(
								   firstNextKeyPosition->GetKey()) == 0);
							delete firstNextKeyPosition;
						}

						// Retaillage du tableau global, pour dereferencer le cles doublonnees
						oaResultKeyPositions.SetSize(oaResultKeyPositions.GetSize() -
									     nDuplicateNumber);
					}
				}
			}

			// Insertion des cles dans le tableau global des resultats
			oaResultKeyPositions.InsertObjectArrayAt(oaResultKeyPositions.GetSize(), oaKeyPositionSubset);
			oaKeyPositionSubset->SetSize(0);

			// Affichage
			if (bTrace)
			{
				cout << GetClassLabel() << " MF (rank " << i
				     << ": size=" << oaKeyPositionSubset->GetSize() << " -> "
				     << oaResultKeyPositions.GetSize() << ")" << endl;
				for (j = 0; j < oaKeyPositionSubset->GetSize(); j++)
				{
					keyPosition = cast(KWKeyPosition*, oaKeyPositionSubset->GetAt(j));
					if (j <= 2 or j >= oaKeyPositionSubset->GetSize() - 2)
						cout << "\t" << *keyPosition << endl;
					else if (j == 3)
						cout << "\t..." << endl;
				}
			}
		}

		// On detruit le tableau uniquement si ok (sinon, nettoyage complet en fin de methode)
		if (bOk)
			oaAllKeyPositionSubsets.DeleteAll();
		assert(not bOk or oaResultKeyPositions.GetSize() <= shared_oaInputKeys->GetObjectArray()->GetSize());

		// Creation des positions intermediaires manquantes
		// Il est en effet possible que certaines cles en entree, a cheval entre deux chunks de table secondaire
		// consecutifs et sans cle de table secondaire, soit absente des resultats.
		// Il faut donc pour ces position manquantes inserer les positions resultats suivantes les plus proches
		if (bOk and oaResultKeyPositions.GetSize() < shared_oaInputKeys->GetObjectArray()->GetSize())
		{
			// Affichage
			if (bTrace)
				cout << GetClassLabel()
				     << " Input keys: " << shared_oaInputKeys->GetObjectArray()->GetSize()
				     << "\tFound positions: " << oaResultKeyPositions.GetSize() << endl;

			// Recopie prealable des resultats collectes dans le maitre
			oaMasterResultKeyPositions.CopyFrom(&oaResultKeyPositions);

			// Retaillage du tableau des resultats final
			oaResultKeyPositions.SetSize(shared_oaInputKeys->GetObjectArray()->GetSize());

			// Parcours synchronise des deux tableaux pour creer les position manquantes
			j = 0;
			for (i = 0; i < shared_oaInputKeys->GetObjectArray()->GetSize(); i++)
			{
				key = cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(i));

				// Creation d'une nouvelle position de type fin de fichier si necessaire
				// c'est a dire si il manque des cle alors que toutes les cle collectee par le master on
				// ete traitees
				if (j == oaMasterResultKeyPositions.GetSize())
				{
					// Position fin de fichier
					newKeyPosition = new KWKeyPosition;
					newKeyPosition->GetKey()->CopyFrom(key);
					newKeyPosition->SetLineIndex(lTotalLineNumber);
					newKeyPosition->SetLinePosition(lInputFileSize);
					oaResultKeyPositions.SetAt(i, newKeyPosition);

					// Affichage
					if (bTrace)
						cout << GetClassLabel() << " MF create end of file position " << i
						     << "\t" << *newKeyPosition << endl;
				}
				// Sinon, on regarde la position courante
				else
				{
					keyPosition = cast(KWKeyPosition*, oaMasterResultKeyPositions.GetAt(j));

					// On garde la position collectee si elle a la meme cle
					if (key->Compare(keyPosition->GetKey()) == 0)
					{
						oaResultKeyPositions.SetAt(i, keyPosition);
						j++;
					}
					// Sinon, on utilise la position suivante
					else
					{
						assert(key->Compare(keyPosition->GetKey()) < 0);

						// Duplication de la position avant memorisation avec la cle en entree
						newKeyPosition = keyPosition->Clone();
						newKeyPosition->GetKey()->CopyFrom(key);
						oaResultKeyPositions.SetAt(i, newKeyPosition);

						// Affichage
						if (bTrace)
							cout << GetClassLabel() << " MF create new position " << i
							     << "\t" << *newKeyPosition << endl;
					}
				}
			}
			assert(oaResultKeyPositions.GetSize() == shared_oaInputKeys->GetObjectArray()->GetSize());
		}
	}

	// Nettoyage si necessaire
	if (not bOk)
	{
		for (i = 0; i < oaAllKeyPositionSubsets.GetSize(); i++)
		{
			oaKeyPositionSubset = cast(ObjectArray*, oaAllKeyPositionSubsets.GetAt(i));

			// Destruction des cles et leur position deja collectees
			if (oaKeyPositionSubset != NULL)
				oaKeyPositionSubset->DeleteAll();
		}
		oaResultKeyPositions.DeleteAll();
	}
	oaAllKeyPositionSubsets.DeleteAll();
	oaAllSlaveFirstKeyPositions.DeleteAll();
	oaAllSlaveLastKeyPositions.DeleteAll();
	lFilePos = 0;
	lInputFileSize = 0;
	assert(not bOk or oaResultKeyPositions.GetSize() == shared_oaInputKeys->GetObjectArray()->GetSize());
	assert(bOk or oaResultKeyPositions.GetSize() == 0);
	return bOk;
}

boolean KWKeyPositionFinderTask::SlaveInitialize()
{
	boolean bOk = true;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sFileName = shared_sInputFileName.GetValue();

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Parametrage du fichier d'entree a analyser pour la tache
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);

	// Ouverture du fichier en entree
	bOk = inputFile.Open();
	return bOk;
}

boolean KWKeyPositionFinderTask::SlaveProcess()
{
	boolean bOk = true;
	boolean bTrace = false;
	KWKey keyStore1;
	KWKey keyStore2;
	KWKey* previousKey;
	KWKey* key;
	int nCompareKey;
	KWKeyPosition* recordKeyPosition;
	KWKey* inputKey;
	int nInputKeyIndex;
	longint lLinePosition;
	longint lDisplayFreshness;
	double dProgression;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;
	boolean bIsLineOK;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	require(inputFile.IsOpened());
	require(output_lLineNumber == (longint)0);

	// Fraicheur d'affichage pour la gestion de la barre de progression
	lDisplayFreshness = 0;

	// Specification de la portion du fichier a traiter
	// On la termine sur la derniere ligne commencant dans le chunk, donc le '\n' se trouve potentiellement
	// sur le debut de chunk suivant, un octet au dela de la fin
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + input_nBufferSize + 1, inputFile.GetFileSize());

	// Affichage
	if (bTrace)
		cout << GetClassLabel() << "\t" << GetTaskIndex() << "\tBegin\t"
		     << FileService::GetFileName(inputFile.GetFileName()) << "\t" << lBeginPos << "\t" << lMaxEndPos
		     << endl;

	// Parametrage de la taille du buffer
	inputFile.SetBufferSize(input_nBufferSize);

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	// On ne compte pas la ligne sautee en debut de chunk, car elle est traitee en fin du chunk precedent
	nCumulatedLineNumber = 0;
	if (lBeginPos > 0)
	{
		bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
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

	// Initialisation des variables de travail
	keyExtractor.SetBufferedFile(&inputFile);
	nInputKeyIndex = -1;
	nCompareKey = -1;
	previousKey = NULL;
	key = &keyStore1;

	// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
	// On reitere tant que l'on a pas atteint la derniere position pour lire toutes les ligne, y compris la derniere
	lLinePosition = -1;
	while (bOk and lBeginPos < lMaxEndPos)
	{
		bOk = inputFile.FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);
		if (not bOk)
			break;

		// Cas d'un ligne trop longue: on se deplace a la ligne suivante ou a la fin de la portion a traite
		if (bLineTooLong)
		{
			lBeginPos = inputFile.GetPositionInFile();

			// Ajout d'un ligne si elle termine dans la portion traitee
			if (lBeginPos < lMaxEndPos)
				nCumulatedLineNumber++;
		}
		// Cas general
		else
		{
			// Saut du Header
			if (shared_bHeaderLineUsed and inputFile.IsFirstPositionInFile())
			{
				inputFile.SkipLine(bLineTooLong);
				if (bLineTooLong)
				{
					AddLocalError("Header line, " + InputBufferedFile::GetLineTooLongErrorLabel(),
						      nCumulatedLineNumber + inputFile.GetCurrentLineIndex());
					bOk = false;
				}
			}

			// Parcours du buffer d'entree
			while (bOk and not inputFile.IsBufferEnd())
			{
				lLinePosition = inputFile.GetPositionInFile();

				// Gestion de la progresssion
				lDisplayFreshness++;
				if (TaskProgression::IsRefreshNecessary(lDisplayFreshness))
				{
					// Calcul de la progression par rapport a la proportion de la portion du fichier
					// traitee parce que l'on ne sait pas le nombre total de ligne que l'on va
					// traiter
					dProgression = (inputFile.GetPositionInFile() - input_lFilePos) * 1.0 /
						       (lMaxEndPos - input_lFilePos);
					TaskProgression::DisplayProgression((int)floor(dProgression * 100));
					if (TaskProgression::IsInterruptionRequested())
					{
						bOk = false;
						break;
					}
				}

				// Extraction de la cle
				bIsLineOK = keyExtractor.ParseNextKey(key, NULL);

				// Memorisation de la premiere cle de l'esclave, que la la ligne soit valide ou non
				if (output_SlaveFirstKeyPosition.GetKeyPosition()->GetLineIndex() == 0)
				{
					output_SlaveFirstKeyPosition.GetKeyPosition()->GetKey()->CopyFrom(key);
					output_SlaveFirstKeyPosition.GetKeyPosition()->SetLineIndex(
					    (longint)nCumulatedLineNumber + inputFile.GetCurrentLineIndex() - 1);
					output_SlaveFirstKeyPosition.GetKeyPosition()->SetLinePosition(lLinePosition);

					// Affichage
					if (bTrace)
						cout << GetClassLabel() << "\t" << GetTaskIndex() << "\t"
						     << nInputKeyIndex << "\tFirst\t"
						     << *output_SlaveFirstKeyPosition.GetKeyPosition() << endl;
				}

				// Comparaison avec la cle precedente
				if (previousKey != NULL)
					nCompareKey = previousKey->Compare(key);

				// Erreur si cle non ordonnees
				if (nCompareKey > 0)
				{
					key->BuildDistinctObjectLabels(previousKey, sObjectLabel, sOtherObjectLabel);
					AddLocalError("Unsorted record with key " + sObjectLabel + " inferior to key " +
							  sOtherObjectLabel + " of previous record",
						      nCumulatedLineNumber + inputFile.GetCurrentLineIndex() - 1);
					bOk = false;
					break;
				}

				// Test si changement de cle
				if (nCompareKey < 0)
				{
					// Recherche de l'index d'une cle en entree, strictement plus petite que la cle
					// en parametre La premiere fois ou si jamais trouvee: par une recherche
					// dichotomique
					if (nInputKeyIndex == -1)
					{
						// Recherche dichotomique d'une cle en entree plus petite ou egale
						nInputKeyIndex =
						    shared_oaInputKeys->GetObjectArray()->FindPrecedingSortIndex(key);

						// Si on a trouve la cle, on affine pour rechercher une cle strictement
						// plus petite
						if (nInputKeyIndex >= 0)
						{
							inputKey =
							    cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(
									     nInputKeyIndex));

							// Si le cle d'entree correspond a la cle recherchee, c'est la
							// premiere cle en cas de doublons. On prend alors la cle
							// d'entre precedente, strictement inferieure
							if (inputKey->Compare(key) == 0)
								nInputKeyIndex--;

							// Si on cela correspond a une cle, on recherche la premiere
							// d'entre-elles en cas de doublons
							if (nInputKeyIndex >= 0)
							{
								inputKey = cast(
								    KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(
										nInputKeyIndex));
								assert(inputKey->Compare(key) < 0);

								// Recherche de la premiere cle uniquement si presence
								// d'au moins un doublons
								if (nInputKeyIndex > 0 and
								    inputKey->Compare(cast(
									KWKey*,
									shared_oaInputKeys->GetObjectArray()->GetAt(
									    nInputKeyIndex - 1))) == 0)
									nInputKeyIndex =
									    shared_oaInputKeys->GetObjectArray()
										->FindPrecedingSortIndex(inputKey);
							}
						}
						assert(nInputKeyIndex == -1 or
						       cast(KWKey*,
							    shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex))
							       ->Compare(key) < 0);
					}

					// On traite toutes les cle d'entree strictement plus petites que la position
					// courante
					while (nInputKeyIndex >= 0 and
					       nInputKeyIndex < shared_oaInputKeys->GetObjectArray()->GetSize())
					{
						inputKey =
						    cast(KWKey*,
							 shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex));

						// On memorise la position si elle est strictement superieure a la cle
						// d'entree
						if (inputKey->Compare(key) < 0)
						{
							// Creation d'une cle et de sa position
							recordKeyPosition = new KWKeyPosition;

							// Memorisation de la cle d'entree correspondante
							recordKeyPosition->GetKey()->CopyFrom(inputKey);

							// Memorisation de l'index de la ligne precedant la cle
							// secondaire, qui sera traitee avec la ligne de la cle d'entree
							// correspondante On utilise le numero de ligne du buffer, qui
							// commence a zero et correspond a la ligne sur laquelle la cle
							// vient d'etre parsee
							recordKeyPosition->SetLineIndex(
							    (longint)nCumulatedLineNumber +
							    inputFile.GetCurrentLineIndex() - 1);

							// Memorisation du debut de ligne suivant (avant parsing de la
							// cle secondaire)
							recordKeyPosition->SetLinePosition(lLinePosition);

							// Memorisation de la cle
							output_oaKeyPositionSubset->GetObjectArray()->Add(
							    recordKeyPosition);

							// Affichage
							if (bTrace)
								cout << GetClassLabel() << "\t" << GetTaskIndex()
								     << "\t" << nInputKeyIndex << "\t" << *key << "\t"
								     << *recordKeyPosition << endl;

							// On passe a la cle d'entree suivante
							nInputKeyIndex++;
						}
						// Sinon, on sort, et l'index de cle d'entree sera la prochaine cle a
						// depasser
						else
							break;
					}

					// Arret si on a depasse la derniere cle d'entree
					if (nInputKeyIndex >= shared_oaInputKeys->GetObjectArray()->GetSize())
						break;
				}

				// Memorisation de la cle precedente
				previousKey = key;

				// Utilisation de l'autre "cle de stockage" pour la cle courante
				// (optimisation de la memoire pour eviter les recopies entre cle et cle precedente)
				if (key == &keyStore1)
					key = &keyStore2;
				else
					key = &keyStore1;
			}

			// Memorisation de la derniere cle de l'esclave, si on a extrait la premiere cle
			// Cette mis a jour a a refaire pour chaque fin de buffer traitee
			if (bOk and output_SlaveFirstKeyPosition.GetKeyPosition()->GetLineIndex() != 0)
			{
				assert(lLinePosition >= 0);
				output_SlaveLastKeyPosition.GetKeyPosition()->GetKey()->CopyFrom(key);
				output_SlaveLastKeyPosition.GetKeyPosition()->SetLineIndex(
				    (longint)nCumulatedLineNumber + inputFile.GetCurrentLineIndex() - 1);
				output_SlaveLastKeyPosition.GetKeyPosition()->SetLinePosition(lLinePosition);

				// Affichage
				if (bTrace)
					cout << GetClassLabel() << "\t" << GetTaskIndex() << "\t" << nInputKeyIndex
					     << "\tLast\t" << *output_SlaveLastKeyPosition.GetKeyPosition() << endl;
			}

			// On se deplace de la taille du buffer analyse
			lBeginPos += inputFile.GetCurrentBufferSize();
			nCumulatedLineNumber += inputFile.GetBufferLineNumber();
		}
	}

	// Envoi du nombre de lignes au master, en fin de SlaveProcess
	if (bOk)
		SetLocalLineNumber(nCumulatedLineNumber);

	// Collecte du nombre de lignes traitees
	if (bOk)
		output_lLineNumber = nCumulatedLineNumber;

	// Nettoyage si KO
	if (not bOk)
		output_oaKeyPositionSubset->GetObjectArray()->DeleteAll();
	return bOk;
}

boolean KWKeyPositionFinderTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage
	keyExtractor.Clean();

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
}
