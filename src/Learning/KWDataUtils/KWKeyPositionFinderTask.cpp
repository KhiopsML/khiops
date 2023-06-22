// Copyright (c) 2023 Orange. All rights reserved.
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
	if (bOk and not TaskProgression::IsInterruptionRequested())
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
	return bOk;
}

const ALString KWKeyPositionFinderTask::GetObjectLabel() const
{
	return sFileName;
}

void KWKeyPositionFinderTask::Test()
{
	TestWithArtificialRootAndSecondaryTables(100000, 1, 1, 1000000, 10, 1, 0);
	TestWithArtificialRootAndSecondaryTables(10000, 1, 1, 100000, 10, 1, 0);
	TestWithArtificialRootAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0.01, 0);
	TestWithArtificialRootAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0.01, 1000);
	TestWithArtificialRootAndSecondaryTables(100000, 10, 0.01, 1000000, 1, 0.01, 1000);
	TestWithArtificialRootAndSecondaryTables(100000, 1, 0, 1000000, 10, 0.01, 1000);
	TestWithArtificialRootAndSecondaryTables(100000, 1, 0.01, 1000000, 10, 0, 1000);
	TestWithArtificialRootAndSecondaryTables(1, 1, 1, 1000000, 10, 0.01, 1000);
}

boolean KWKeyPositionFinderTask::TestWithArtificialRootAndSecondaryTables(
    int nRootLineNumber, int nRootLineNumberPerKey, double dRootSamplingRate, int nSecondaryLineNumber,
    int nSecondaryLineNumberPerKey, double dSecondarySamplingRate, int nBufferSize)
{
	boolean bOk = true;
	boolean bCreateDatasets = true;
	KWArtificialDataset rootArtificialDataset;
	KWArtificialDataset secondaryArtificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaInputKeyPositions;
	ObjectArray oaInputKeys;
	ObjectArray oaFoundKeyPositions;
	KWKeyPositionFinderTask keyPositionFinder;

	require(nRootLineNumber >= 0);
	require(nRootLineNumberPerKey >= 0);
	require(dRootSamplingRate >= 0);
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
	     << " Root(" << nRootLineNumber << ", " << nRootLineNumberPerKey << ", " << dRootSamplingRate << ")"
	     << " Secondary(" << nSecondaryLineNumber << ", " << nSecondaryLineNumberPerKey << ", "
	     << dSecondarySamplingRate << ")"
	     << " Buffer(" << nBufferSize << ")" << endl;
	cout << "===============================================================================" << endl;

	// Creation d'un fichier racine avec des champs cle
	rootArtificialDataset.SpecifySortedDataset();
	rootArtificialDataset.SetLineNumber(nRootLineNumber);
	rootArtificialDataset.SetMaxLineNumberPerKey(nRootLineNumberPerKey);
	rootArtificialDataset.SetSamplingRate(dRootSamplingRate);
	rootArtificialDataset.SetFileName(rootArtificialDataset.BuildFileName());
	if (bCreateDatasets)
		rootArtificialDataset.CreateDataset();
	rootArtificialDataset.DisplayFirstLines(15);

	// Evaluation de la taille des cles du fichier principal
	lMeanKeySize = 0;
	lLineNumber = 0;
	bOk = bOk and
	      KWKeySizeEvaluatorTask::TestWithArtificialDataset(&rootArtificialDataset, lMeanKeySize, lLineNumber);

	// Extraction des cle du fichier principal
	bOk = bOk and KWKeyPositionSampleExtractorTask::TestWithArtificialDataset(
			  &rootArtificialDataset, 0.25, lMeanKeySize, lLineNumber, &oaInputKeyPositions);

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
		rootArtificialDataset.DeleteDataset();
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
	longint lInputKeysUsedMemory;
	longint lOutputKeysUsedMemory;
	int i;
	KWKey* key;
	int nBufferSize;

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

	// Estimation du nombre de clefs dans un buffer
	lInputFileSize = PLRemoteFileService::GetFileSize(sFileName);
	nBufferSize = BufferedFile::nDefaultBufferSize;
	if (nBufferSize > lInputFileSize)
	{
		assert(lInputFileSize < INT_MAX);
		nBufferSize = (int)(lInputFileSize + 1);
	}
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(
	    lInputKeysUsedMemory + nBufferSize + 2 * lInputKeysUsedMemory * nBufferSize / (lInputFileSize + 1));

	// Nombre max de slaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / nBufferSize));
	return bOk;
}

boolean KWKeyPositionFinderTask::MasterInitialize()
{
	boolean bOk = true;
	boolean bTrace = false;
	int i;
	KWKey* key;

	require(oaAllKeyPositionSubsets.GetSize() == 0);
	require(oaResultKeyPositions.GetSize() == 0);
	require(oaAllSlaveFirstKeyPositions.GetSize() == 0);
	require(oaAllSlaveLastKeyPositions.GetSize() == 0);

	// Affichage des cle en entree
	if (bTrace)
	{
		cout << "Input keys" << endl;
		for (i = 0; i < shared_oaInputKeys->GetObjectArray()->GetSize(); i++)
		{
			key = cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(i));
			cout << "\t" << i << "\t" << *key << endl;
		}
	}

	// Parametrage du fichier en lecture
	shared_sInputFileName.SetValue(sFileName);
	shared_bHeaderLineUsed = bHeaderLineUsed;
	shared_cFieldSeparator.SetValue(cFieldSeparator);

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

	// Test si cle en entree
	if (bOk and shared_oaInputKeys->GetObjectArray()->GetSize() == 0)
	{
		AddError("No input keys: no position to extract");
		bOk = false;
	}
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

		// Idem pour les tableau des permiere et dernieres cles
		oaAllSlaveFirstKeyPositions.Add(NULL);
		oaAllSlaveLastKeyPositions.Add(NULL);

		// Parametrage de la taille du buffer
		input_nBufferSize =
		    ComputeStairBufferSize(lMB, BufferedFile::nDefaultBufferSize, lFilePos, lInputFileSize);

		// On peut imposer la taille du buffer pour raison de tests
		if (nForcedBufferSize > 0)
			input_nBufferSize = nForcedBufferSize;

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
	ObjectArray* oaKeyPositionSample;
	KWKeyPosition* keyPosition1;
	KWKeyPosition* keyPosition2;
	ALString sTmp;

	// Test si interruption
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Integration des cles de l'esclave
	if (bOk)
	{
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
					// On renonce a preciser le numeroid de ligne, car il faudrait cimuler les
					// nombre de lignes lues pour les esclaves precedents, et tous n'ont peut-etre
					// pas termine
					AddError(sTmp + "Record with key " + keyPosition2->GetKey()->GetObjectLabel() +
						 " inferior to key " + keyPosition1->GetKey()->GetObjectLabel() +
						 " found for preceding record");
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

			// Comparaison s'il elle ont ete extraites
			if (keyPosition1->GetLineIndex() != 0 and keyPosition2->GetLineIndex() != 0)
			{
				if (keyPosition1->Compare(keyPosition2) > 0)
				{
					// On renonce a preciser le numeroid de ligne, car il faudrait cimuler les
					// nombre de lignes lues pour les esclaves precedents, et tous n'ont peut-etre
					// pas termine
					AddError(sTmp + "Record with key " + keyPosition2->GetKey()->GetObjectLabel() +
						 " inferior to key " + keyPosition1->GetKey()->GetObjectLabel() +
						 " found for preceding record");
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
			lLineCountToAdd = lvLineCountPerTaskIndex.GetAt(i);

			// Incrementation du nombre total de lignes
			lTotalLineNumber += lLineCountToAdd;
		}
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
						// mais pas la la cle touvee (non memorisee), d'ou un message pas
						// entierement explicite
						AddError(sTmp + "Record " +
							 LongintToString(firstNextKeyPosition->GetLineIndex()) +
							 " : key " + firstNextKeyPosition->GetKey()->GetObjectLabel() +
							 " inferior to key found previously, beyond record " +
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
	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sFileName = shared_sInputFileName.GetValue();

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());
	return true;
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
	PeriodicTest periodicTestInterruption;
	double dProgression;
	ALString sTmp;
	InputBufferedFile inputFile;

	// Initialisation du buffer de lecture
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);
	inputFile.SetBufferSize(input_nBufferSize);

	// Ouverture et remplissage du buffer
	bOk = inputFile.Open();
	if (bOk)
		bOk = inputFile.Fill(input_lFilePos);

	// Initialisations
	if (bOk)
	{
		// Envoi du nombre de lignes au master
		SetLocalLineNumber(inputFile.GetBufferLineNumber());

		// Collecte du nombre de lignes du buffer
		output_lLineNumber = inputFile.GetBufferLineNumber();

		// Saut du Header
		if (inputFile.GetPositionInFile() == 0 and shared_bHeaderLineUsed)
			inputFile.SkipLine();
	}

	// Initialisation de la cle precedente et de la cle courante
	previousKey = NULL;
	key = &keyStore1;

	// Parcours du buffer d'entree
	nInputKeyIndex = -1;
	nCompareKey = -1;
	lLinePosition = -1;
	keyExtractor.SetBufferedFile(&inputFile);
	while (bOk and not inputFile.IsBufferEnd())
	{
		lLinePosition = inputFile.GetPositionInFile();

		// Gestion de la progresssion
		if (periodicTestInterruption.IsTestAllowed(inputFile.GetCurrentLineNumber()))
		{
			dProgression = inputFile.GetCurrentLineNumber() * 1.0 / inputFile.GetBufferLineNumber();
			TaskProgression::DisplayProgression((int)floor(dProgression * 100));
			if (TaskProgression::IsInterruptionRequested())
			{
				bOk = false;
				break;
			}
		}

		// Extraction de la cle
		keyExtractor.ParseNextKey(NULL);
		keyExtractor.ExtractKey(key);

		// Memorisation de la premiere cle de l'esclave
		if (output_SlaveFirstKeyPosition.GetKeyPosition()->GetLineIndex() == 0)
		{
			output_SlaveFirstKeyPosition.GetKeyPosition()->GetKey()->CopyFrom(key);
			output_SlaveFirstKeyPosition.GetKeyPosition()->SetLineIndex(inputFile.GetCurrentLineNumber());
			output_SlaveFirstKeyPosition.GetKeyPosition()->SetLinePosition(lLinePosition);
		}

		// Comparaison avec la cle precedente
		if (previousKey != NULL)
			nCompareKey = previousKey->Compare(key);

		// Erreur si cle non ordonnees
		if (nCompareKey > 0)
		{
			AddLocalError("key " + key->GetObjectLabel() + " inferior to key " +
					  previousKey->GetObjectLabel() + " of previous record",
				      inputFile.GetCurrentLineNumber());
			bOk = false;
			break;
		}

		// Test si changement de cle
		if (nCompareKey < 0)
		{
			// Recherche de l'index d'une cle en entree, strictement plus petite que la cle en parametre
			// La premiere fois ou si jamais trouvee: par une recherche dichotomique
			if (nInputKeyIndex == -1)
			{
				// Recherche dichotomique d'une cle en entree plus petite ou egale
				nInputKeyIndex = shared_oaInputKeys->GetObjectArray()->FindPrecedingSortIndex(key);

				// Si on a trouve la cle, on affine pour rechercher une cle strictement plus petite
				if (nInputKeyIndex >= 0)
				{
					inputKey =
					    cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex));

					// Si le cle d'entree correspond a la cle recherchee, c'est la premiere cle en
					// cas de doublons. On prend alors la cle d'entre precedente, strictement
					// inferieure
					if (inputKey->Compare(key) == 0)
						nInputKeyIndex--;

					// Si on cela correspond a une cle, on recherche la premiere d'entre-elles en
					// cas de doublons
					if (nInputKeyIndex >= 0)
					{
						inputKey =
						    cast(KWKey*,
							 shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex));
						assert(inputKey->Compare(key) < 0);

						// Recherche de la premiere cle uniquement si presence d'au moins un
						// doublons
						if (nInputKeyIndex > 0 and
						    inputKey->Compare(
							cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(
									 nInputKeyIndex - 1))) == 0)
							nInputKeyIndex = shared_oaInputKeys->GetObjectArray()
									     ->FindPrecedingSortIndex(inputKey);
					}
				}
				assert(nInputKeyIndex == -1 or
				       cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex))
					       ->Compare(key) < 0);
			}

			// On traite toutes les cle d'entree strictement plus petites que la position courante
			while (nInputKeyIndex >= 0 and nInputKeyIndex < shared_oaInputKeys->GetObjectArray()->GetSize())
			{
				inputKey = cast(KWKey*, shared_oaInputKeys->GetObjectArray()->GetAt(nInputKeyIndex));

				// On memorise la position si elle est strictement superieure a la cle d'entree
				if (inputKey->Compare(key) < 0)
				{
					// Creation d'une cle et de sa position
					recordKeyPosition = new KWKeyPosition;

					// Memorisation de la cle d'entree correspondante
					recordKeyPosition->GetKey()->CopyFrom(inputKey);

					// Memorisation de l'index de la ligne predecent la cle secondaire, qui sera
					// traite avec la ligne de la cle d'entre correspondante
					recordKeyPosition->SetLineIndex(inputFile.GetCurrentLineNumber() - 1);

					// Memorisation du debut de ligne suivant (avant parsing de la cle secondaire)
					recordKeyPosition->SetLinePosition(lLinePosition);

					// Memorisation de la cle
					output_oaKeyPositionSubset->GetObjectArray()->Add(recordKeyPosition);

					// Affichage
					if (bTrace)
						cout << GetClassLabel() << "\tSP" << GetTaskIndex() << "\t" << *key
						     << "\t" << *inputKey << "\t" << nInputKeyIndex << endl;

					// On passe a la cle d'entree suivante
					nInputKeyIndex++;
				}
				// Sinon, on sort, et l'index de cle d'entree sera la prochaine cle a depasser
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
	if (bOk and output_SlaveFirstKeyPosition.GetKeyPosition()->GetLineIndex() != 0)
	{
		assert(lLinePosition >= 0);
		output_SlaveLastKeyPosition.GetKeyPosition()->GetKey()->CopyFrom(key);
		output_SlaveLastKeyPosition.GetKeyPosition()->SetLineIndex(inputFile.GetCurrentLineNumber());
		output_SlaveLastKeyPosition.GetKeyPosition()->SetLinePosition(lLinePosition);
	}

	// Nettoyage si KO
	if (not bOk)
		output_oaKeyPositionSubset->GetObjectArray()->DeleteAll();

	if (inputFile.IsOpened())
		inputFile.Close();
	return bOk;
}

boolean KWKeyPositionFinderTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	// Nettoyage
	keyExtractor.Clean();

	return true;
}