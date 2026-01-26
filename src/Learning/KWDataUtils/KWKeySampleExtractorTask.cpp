// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKeySampleExtractorTask.h"
#include "PLSerializer.h"
#include "KWKeyExtractor.h"
#include "TaskProgression.h"

KWKeySampleExtractorTask::KWKeySampleExtractorTask()
{
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';
	nSplitKeyNumberMin = 0;
	nSplitKeyNumberMax = 0;
	lCurrentUsedMemory = 0;
	lMaxUsedMemory = 0;
	dCummulatedBufferSize = 0;
	lKeyUsedMemory = 0;
	lFileLineNumber = 0;
	nSampleSize = 0;
	dSamplingRate = 0;
	lInputFileSize = 0;
	lFilePos = 0;

	// Variables partagees
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_sFileName);
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_cFieldSeparator);
	DeclareSharedParameter(&shared_nBufferSize);

	// Variable en entree des esclaves
	DeclareTaskInput(&input_dSamplingRate);
	DeclareTaskInput(&input_lFilePos);

	// Variables resultats des esclaves
	output_oaSampledKeys = new PLShared_ObjectArray(new PLShared_Key);
	DeclareTaskOutput(&output_dSamplingRate);
	DeclareTaskOutput(&output_dBufferSize);
	DeclareTaskOutput(output_oaSampledKeys);
	DeclareTaskOutput(&output_lSampledKeysUsedMemory);
}

KWKeySampleExtractorTask::~KWKeySampleExtractorTask()
{
	delete output_oaSampledKeys;
}

void KWKeySampleExtractorTask::SetFileURI(const ALString& sValue)
{
	sFileURI = sValue;
}

const ALString& KWKeySampleExtractorTask::GetFileURI() const
{
	return sFileURI;
}

void KWKeySampleExtractorTask::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean KWKeySampleExtractorTask::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

void KWKeySampleExtractorTask::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

char KWKeySampleExtractorTask::GetFieldSeparator() const
{
	return cFieldSeparator;
}

void KWKeySampleExtractorTask::SetSplitKeyNumber(int nMinValue, int nMaxValue)
{
	require(nMinValue > 0);
	require(nMaxValue >= nMinValue);
	nSplitKeyNumberMin = nMinValue;
	nSplitKeyNumberMax = nMaxValue;
}

int KWKeySampleExtractorTask::GetSplitKeyNumberMin() const
{
	return nSplitKeyNumberMin;
}

int KWKeySampleExtractorTask::GetSplitKeyNumberMax() const
{
	return nSplitKeyNumberMax;
}

IntVector* KWKeySampleExtractorTask::GetKeyFieldIndexes()
{
	return shared_ivKeyFieldIndexes.GetIntVector();
}

const IntVector* KWKeySampleExtractorTask::GetConstKeyFieldIndexes() const
{
	return shared_ivKeyFieldIndexes.GetConstIntVector();
}

void KWKeySampleExtractorTask::SetSampleSize(int nLines)
{
	require(nLines > 0);
	nSampleSize = nLines;
}

int KWKeySampleExtractorTask::GetSampleSize() const
{
	return nSampleSize;
}

void KWKeySampleExtractorTask::SetFileLineNumber(longint lLines)
{
	require(lLines > 0);
	lFileLineNumber = lLines;
}

longint KWKeySampleExtractorTask::GetFileLineNumber() const
{
	return lFileLineNumber;
}

void KWKeySampleExtractorTask::SetKeyUsedMemory(longint lValue)
{
	require(lValue > 0);
	lKeyUsedMemory = lValue;
}

longint KWKeySampleExtractorTask::GetKeyUsedMemory() const
{
	return lKeyUsedMemory;
}

boolean KWKeySampleExtractorTask::ExtractSample(ObjectArray* oaResult)
{
	boolean bOk = true;

	require(sFileURI != ""); // Le fichier d'entree doit etre renseigne
	require(nSplitKeyNumberMin <= nSplitKeyNumberMax);
	require(nSplitKeyNumberMin > 0); // Le nombre de splits doit etre renseigne
	require(lKeyUsedMemory > 0);
	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(oaSplitKeys.GetSize() == 0);
	require(lFileLineNumber > 0);
	require(nSampleSize > 0);

	// Execution de la tache
	bOk = Run();

	// Collecte des resultats
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		assert(oaSplitKeys.GetSize() <= nSplitKeyNumberMax);
		oaResult->CopyFrom(&oaSplitKeys);
		oaSplitKeys.RemoveAll();
	}
	// Nettoyage sinon
	else
	{
		oaResult->RemoveAll();
		oaSplitKeys.DeleteAll();
		bOk = false;
	}
	return bOk;
}

void KWKeySampleExtractorTask::Test()
{
	boolean bOk = true;
	KWArtificialDataset artificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaKeys;
	KWKeySampleExtractorTask keySampleExtractor;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + keySampleExtractor.GetTaskLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Creation d'un fichier avec des champs cle
	artificialDataset.SpecifySortDataset();
	artificialDataset.CreateDataset();
	artificialDataset.DisplayFirstLines(15);

	// Evaluation de la taille des cles
	lMeanKeySize = 0;
	lLineNumber = 0;
	bOk = bOk and KWKeySizeEvaluatorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, lLineNumber);

	// Extraction des cles
	bOk = bOk and KWKeySampleExtractorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, 10, 20,
									  artificialDataset.GetLineNumber() / 10,
									  lLineNumber, &oaKeys);
	oaKeys.DeleteAll();

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWKeySampleExtractorTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
							    longint lMeanKeySize, int nSplitKeyNumberMin,
							    int nSplitKeyNumberMax, int nSampleSize,
							    longint lFileLineNumber, ObjectArray* oaKeys)
{
	boolean bOk = true;
	KWKeySampleExtractorTask keySampleExtractor;
	int i;

	require(artificialDataset != NULL);

	// Extraction des cles
	if (lMeanKeySize > 0)
	{
		keySampleExtractor.SetFileURI(artificialDataset->GetFileName());
		keySampleExtractor.SetHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
		keySampleExtractor.SetFieldSeparator(artificialDataset->GetFieldSeparator());
		keySampleExtractor.SetSplitKeyNumber(nSplitKeyNumberMin, nSplitKeyNumberMax);
		keySampleExtractor.GetKeyFieldIndexes()->CopyFrom(artificialDataset->GetConstKeyFieldIndexes());
		keySampleExtractor.SetFileLineNumber(lFileLineNumber);
		keySampleExtractor.SetSampleSize(nSampleSize);
		keySampleExtractor.SetKeyUsedMemory(lMeanKeySize);
		bOk = bOk and keySampleExtractor.ExtractSample(oaKeys);
		cout << "Extracted keys: " << oaKeys->GetSize() << endl;
		for (i = 0; i < oaKeys->GetSize(); i++)
		{
			cout << "\t" << *oaKeys->GetAt(i) << endl;
			if (i > 10)
				break;
		}
	}
	return bOk;
}

const ALString KWKeySampleExtractorTask::GetTaskName() const
{
	return "Key sample extractor";
}

PLParallelTask* KWKeySampleExtractorTask::Create() const
{
	return new KWKeySampleExtractorTask;
}

longint KWKeySampleExtractorTask::ComputeGlobalMemory(int nMaxBufferSize) const
{
	int nBufferLineNumber;

	// Estimation du nombre de clefs dans un buffer
	nBufferLineNumber = (int)ceil(lFileLineNumber * (nMaxBufferSize / (1.0 + lInputFileSize)));
	return nMaxBufferSize + nBufferLineNumber * lKeyUsedMemory * 2;
}

int KWKeySampleExtractorTask::ComputeBufferSize(longint lGlobalMemory) const
{
	return (int)floor(lGlobalMemory / (1.0 + 2 * lFileLineNumber * lKeyUsedMemory / (1.0 + lInputFileSize)));
}

boolean KWKeySampleExtractorTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	longint lSlaveMemoryMin;
	longint lReadMemoryMax;
	const longint lMemoryLimit = 128 * lMB;
	const int nPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sFileURI);

	lInputFileSize = PLRemoteFileService::GetFileSize(sFileURI);

	// Taille du sample (*2 pour etre tranquille)
	// On met la meme chose en max : on part du principe que la taille du sample en parametre de la tache est
	// correctement estimee
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(nSampleSize * lKeyUsedMemory * 2);

	// Pour la memoire min, dans les esclaves on veut lire au moins GetPreferredBufferSize
	// On calcule la memoire "globale" necessaire resultante de la lecture d'un buffer de taille
	// GetPreferredBufferSize
	lSlaveMemoryMin = ComputeGlobalMemory(nPreferredSize);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(lSlaveMemoryMin);

	// On se protege d'un depassement de la limite des int qui posera probleme lors du cast
	if (lSlaveMemoryMin > INT_MAX)
		lSlaveMemoryMin = INT_MAX;

	// Pour la memoire max, on ne veut souhaite pas lire plus de 128Mo et on veut un arrondi par un multiple de
	// GetPreferredBufferSize
	lReadMemoryMax = (lMemoryLimit / nPreferredSize) * nPreferredSize;
	lReadMemoryMax = max(lReadMemoryMax, lSlaveMemoryMin);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(ComputeGlobalMemory((int)lReadMemoryMax));
	ensure(GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Check());

	// Nombre max de slaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / nPreferredSize));
	return bOk;
}

boolean KWKeySampleExtractorTask::MasterInitialize()
{
	boolean bOk = true;
	int nBufferSize;
	ALString sTmp;
	const int nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(sFileURI);

	// Parametrage du fichier a analyser
	shared_sFileName.SetValue(sFileURI);
	shared_bHeaderLineUsed = bHeaderLineUsed;
	shared_cFieldSeparator.SetValue(cFieldSeparator);

	// Initialisation des variables de travail
	lCurrentUsedMemory = 0;
	dCummulatedBufferSize = 0;
	assert(oaAllSortedKeySamples.GetSize() == 0);
	assert(oaSplitKeys.GetSize() == 0);

	// Test si fichier d'entree present
	if (not PLRemoteFileService::FileExists(sFileURI))
	{
		AddError("Input file " + FileService::GetURIUserLabel(sFileURI) + " does not exist");
		bOk = false;
	}

	// Test si taille du fichier non nulle
	if (bOk and lInputFileSize == 0)
	{
		AddError("Input file " + FileService::GetURIUserLabel(sFileURI) + " is empty");
		bOk = false;
	}

	// Taille max utilisable par l'echantillon (on enleve 20% pour etre tranquille)
	lMaxUsedMemory = (longint)ceil(GetMasterResourceGrant()->GetMemory() * 0.8);

	// Ratio d'extraction
	dSamplingRate = ((double)nSampleSize) / lFileLineNumber;
	assert(dSamplingRate > 0 and dSamplingRate <= 1);

	if (GetVerbose())
		AddMessage(sTmp + "Available memory per slave " +
			   LongintToHumanReadableString(GetSlaveResourceGrant()->GetMemory()));

	// Calcul de la memoire utilisable pour le buffer a partir de la memoire disponible
	nBufferSize = InputBufferedFile::FitBufferSize(ComputeBufferSize(GetSlaveResourceGrant()->GetMemory()));
	if (GetVerbose())
		AddMessage(sTmp + "Computed max buffer size " + LongintToHumanReadableString(nBufferSize));

	// Chaque esclave doit lire au moins 5 buffers (pour que le travail soit bien reparti entre les esclaves)
	if (lInputFileSize / (GetProcessNumber() * (longint)5) < nBufferSize)
	{
		nBufferSize = InputBufferedFile::FitBufferSize(lInputFileSize / (GetProcessNumber() * (longint)5));
		if (GetVerbose())
			AddMessage(sTmp + "Buffer size reduced to " + LongintToHumanReadableString(nBufferSize));
	}

	// Si le buffer est plus grand que nPrefferedBufferSize, on prend un multiple de nPrefferedBufferSize
	if (nBufferSize > nPreferredBufferSize)
	{
		nBufferSize = (nBufferSize / nPreferredBufferSize) * nPreferredBufferSize;
		if (GetVerbose())
			AddMessage(sTmp + "Buffer size rounded to " + LongintToHumanReadableString(nBufferSize));
	}
	else
	// On prend a minima nPrefferedBufferSize comme taille de buffer
	{
		nBufferSize = nPreferredBufferSize;
		if (GetVerbose())
			AddMessage(sTmp + "Buffer size bounded to " + LongintToHumanReadableString(nBufferSize));
	}

	shared_nBufferSize = nBufferSize;
	lFilePos = 0;
	return bOk;
}

boolean KWKeySampleExtractorTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Est-ce qu'il y a encore du travail ?
	if (lFilePos >= lInputFileSize)
		bIsTaskFinished = true;
	else
	{
		// Position dans le fichier
		input_lFilePos = lFilePos;

		// Ratio d'echantillonnage
		input_dSamplingRate = dSamplingRate;

		// Calcul de la progression
		dTaskPercent = shared_nBufferSize * 1.0 / lInputFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;

		// Position dans le fichier pour la prochaine lecture
		lFilePos += shared_nBufferSize;
	}
	return true;
}

boolean KWKeySampleExtractorTask::MasterAggregateResults()
{
	boolean bOk = true;
	const double dEpsilon = 1e-5;
	int i;
	KWSortedKeySample* aSortedKeySamples;
	double dFileAnalysisPercentage;
	boolean bNeedResampling;
	longint lNewUsedMemory;
	KWSortedKeySample* sampleToReSample;
	double dNewSamplingRatio;
	longint lRemovedKeysUsedMemory;
	ALString sTmp;

	// Test si interruption
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Taille cumulee des buffers traites par les esclaves, pour determiner la proportion du fichier traitee
	dCummulatedBufferSize += output_dBufferSize;
	assert(dCummulatedBufferSize <= lInputFileSize);

	// S'il n'y a pas assez de memoire pour stocker les cles renvoyes par l'esclave,
	// on doit reechantillonner les cles deja stockees par le maitre avec un taux plus faible
	if (bOk)
	{
		lNewUsedMemory = lCurrentUsedMemory + output_lSampledKeysUsedMemory;
		if (lNewUsedMemory > lMaxUsedMemory)
		{
			// Calcul du nouveau taux (avec un peu de marge)
			dFileAnalysisPercentage = dCummulatedBufferSize / lInputFileSize;
			dNewSamplingRatio =
			    dSamplingRate * dFileAnalysisPercentage * 0.9 * lMaxUsedMemory * 1.0 / lNewUsedMemory;
			assert(dNewSamplingRatio < dSamplingRate);

			// Reechantilonnage de chacun de ces tableaux de cles, en reactualisant la taille memoire
			// utilisee
			for (i = 0; i < oaAllSortedKeySamples.GetSize(); i++)
			{
				sampleToReSample = cast(KWSortedKeySample*, oaAllSortedKeySamples.GetAt(i));
				lRemovedKeysUsedMemory =
				    sampleToReSample->Sample(dNewSamplingRatio / dSamplingRate, GetTaskIndex());
				lCurrentUsedMemory -= lRemovedKeysUsedMemory;
			}

			// Memorisation du nouveau taux d'echantillonnage
			dSamplingRate = dNewSamplingRatio;
		}
		assert(0 <= lCurrentUsedMemory);
		assert(lCurrentUsedMemory + output_lSampledKeysUsedMemory * dSamplingRate / output_dSamplingRate <=
		       lMaxUsedMemory);
	}

	// Integration des cles de l'esclave
	if (bOk)
	{
		// Ajout des clefs envoyees par l'esclave
		aSortedKeySamples = new KWSortedKeySample;
		oaAllSortedKeySamples.Add(aSortedKeySamples);
		aSortedKeySamples->ImportKeys(output_oaSampledKeys->GetObjectArray());
		lCurrentUsedMemory += output_lSampledKeysUsedMemory;

		// Il faut re-echantilloner si le ratio d'echantillonage a change dans le maitre
		// par rapport au ratio utilise dans l'esclave
		bNeedResampling = (fabs(output_dSamplingRate - dSamplingRate) > dEpsilon * dSamplingRate);
		if (bNeedResampling)
		{
			lRemovedKeysUsedMemory =
			    aSortedKeySamples->Sample(dSamplingRate / output_dSamplingRate, GetTaskIndex());
			lCurrentUsedMemory -= lRemovedKeysUsedMemory;
		}
	}
	return bOk;
}

boolean KWKeySampleExtractorTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	int i;
	int nLastKeyIndex;
	int nKeyIndex;
	ObjectArray oaAllSortedKeys;
	SortedList* slAllSortedKeySamples;
	KWSortedKeySample* aSortedKeySamples;
	KWKey* aKey;
	int nKeyNumber;
	int nChunkSizeMin;
	int nChunkSizeMax;
	int nStep;

	require(nSplitKeyNumberMin <= nSplitKeyNumberMax);

	// Test si arret utilisateur
	bOk = bProcessEndedCorrectly;
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Tri des clefs envoyees par les esclaves (chaque sample est deja trie : MergeSort)
	if (bOk)
	{
		// Titre de la tache
		TaskProgression::DisplayLabel("Sort key sample");
		nKeyNumber = 0;

		// Creation de la liste triee des tableaux tries de cles
		slAllSortedKeySamples = new SortedList(KWSortedKeySampleCompareHead);
		for (i = 0; i < oaAllSortedKeySamples.GetSize(); i++)
		{
			aSortedKeySamples = cast(KWSortedKeySample*, oaAllSortedKeySamples.GetAt(i));
			nKeyNumber += aSortedKeySamples->GetCount();
			if (not aSortedKeySamples->IsEmpty())
				slAllSortedKeySamples->Add(aSortedKeySamples);
			else
				delete aSortedKeySamples;
		}
		oaAllSortedKeySamples.RemoveAll();

		// On a une liste triee de tableau de cle (par premiere cle de tableau), chacun trie par cle
		while (slAllSortedKeySamples->GetCount() > 0)
		{
			// On retire a chaque iteration le tableau de tete (ayant la plus petite cle),
			aSortedKeySamples = cast(KWSortedKeySample*, slAllSortedKeySamples->RemoveHead());

			// On retire cette premiere cle que l'on range dans l'echantillon de cle resultats (qui sera
			// trie globalement)
			aKey = aSortedKeySamples->PopKey();
			oaAllSortedKeys.Add(aKey);

			// On reinsere le tableau modifie dans la liste triee
			if (not aSortedKeySamples->IsEmpty())
				slAllSortedKeySamples->Add(aSortedKeySamples);
			// On le detruit s'il ne contient plus de cle
			else
				delete aSortedKeySamples;

			// Progression et test si arret utilisateur
			if (oaAllSortedKeys.GetSize() % 100 == 0)
			{
				TaskProgression::DisplayProgression(oaAllSortedKeys.GetSize() * 100 / nKeyNumber);
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}
		}

		// Nettoyage
		slAllSortedKeySamples->DeleteAll();
		delete slAllSortedKeySamples;
	}

	///////////////////////////////////////////////////////////////////////////
	// Choix des bornes
	// Il faut prendre N-1 bornes pour faire N sacs

	if (bOk)
	{
		// Calcul du la taille du chunk
		nChunkSizeMin = oaAllSortedKeys.GetSize() / nSplitKeyNumberMax;
		nChunkSizeMax = oaAllSortedKeys.GetSize() / nSplitKeyNumberMin;

		nKeyIndex = 0;

		// Extraction des bornes
		nLastKeyIndex = -1;

		// Cas particulier ou le nombre de clefs n'est pas suffisant
		// A priori c'est quand l'algo de DeWitt a echoue
		// Dans ce cas on decoupe en tranche egales
		if (oaAllSortedKeys.GetSize() <= nChunkSizeMin)
		{
			for (i = 0; i < nSplitKeyNumberMin; i++)
			{
				// Calcul de l'index de la cle de coupure a extraire
				nKeyIndex =
				    (int)floor((i + 1) * 1.0 * oaAllSortedKeys.GetSize() / (nSplitKeyNumberMin + 1));

				// Extraction si nouvel index (si echantillon assez grand)
				if (nKeyIndex > nLastKeyIndex)
				{
					// On extrait la cle (que l'on remplace par NULL pour ne pas la detuire lors du
					// nettoyage final)
					oaSplitKeys.Add(oaAllSortedKeys.GetAt(nKeyIndex));
					oaAllSortedKeys.SetAt(nKeyIndex, NULL);
					nLastKeyIndex = nKeyIndex;
				}
			}
		}
		else
		{
			// La taille des chunks va etre construite en escalier :
			// Au premier SlaveProcess les esclaves n'auront pas la meme taille de chunk, le premier
			// aura nStep, le deuxieme 2*nStep etc, la taille de la marche est calculee de telle sorte
			// que la derniere marche soit la taille max de chunk

			// Calcul de la taille de la marche
			nStep = (nChunkSizeMax - nChunkSizeMin) / GetProcessNumber();
			int nChunkSize;
			int nRest;

			// Tant qu'il reste plus qu'une taille de chunk
			nRest = 0;
			while (nKeyIndex < oaAllSortedKeys.GetSize() - nChunkSizeMin)
			{
				// Calcul de l'index de la cle de coupure a extraire
				if (nKeyIndex < nChunkSizeMax and nStep > 0)
				{
					// Premiers chunks : marche d'escalier, une marche a nStep de plus que la
					// precedente
					if (nKeyIndex == 0)
					{
						nRest = 0;
					}
					else
					{
						nRest += nStep;
					}
				}
				else if (nKeyIndex > oaAllSortedKeys.GetSize() - 2 * GetProcessNumber() * nChunkSizeMin)
				{
					// Atterissage : on finit avec des petits chunks entre nChunkSizeMin et
					// nChunkSizeMin+nChunkSizeMin/2
					nRest = nChunkSizeMin / 2;
				}
				else
				{
					// Rythme de croisiere : taille entre nChunkSizeMin et nChunkSizeMax
					nRest = nChunkSizeMax - nChunkSizeMin;
				}

				if (nRest > nChunkSizeMax - nChunkSizeMin)
					nRest = nChunkSizeMax - nChunkSizeMin;

				nChunkSize = nChunkSizeMin + RandomInt(nRest);

				if (nChunkSize > oaAllSortedKeys.GetSize() - nKeyIndex)
				{
					nChunkSize = (oaAllSortedKeys.GetSize() - nKeyIndex) / 2;
				}

				nKeyIndex += nChunkSize;

				// Extraction si nouvel index (si echantillon assez grand)
				if (nKeyIndex > nLastKeyIndex)
				{
					// On extrait la cle (que l'on remplace par NULL pour ne pas la detuire lors du
					// nettoyage final)
					oaSplitKeys.Add(oaAllSortedKeys.GetAt(nKeyIndex));
					oaAllSortedKeys.SetAt(nKeyIndex, NULL);
					nLastKeyIndex = nKeyIndex;
				}

				// Test si arret utilisateur
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}
		}
		assert(oaSplitKeys.GetSize() <= nSplitKeyNumberMax);
	}

	// Nettoyage, que la tache se soit terminee correctement ou non
	oaAllSortedKeySamples.DeleteAll();
	oaAllSortedKeys.DeleteAll();
	if (not bOk)
		oaSplitKeys.DeleteAll();
	return bOk;
}

boolean KWKeySampleExtractorTask::SlaveInitialize()
{
	boolean bOk = true;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Parametrage du fichier d'entree a analyser pour la tache
	inputFile.SetFileName(shared_sFileName.GetValue());
	inputFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);

	// Ouverture du fichier en entree
	bOk = inputFile.Open();
	return bOk;
}

boolean KWKeySampleExtractorTask::SlaveProcess()
{
	boolean bOk = true;
	KWKey* recordKey;
	double dProgression;
	int nCount;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	boolean bIsLineOk;

	inputFile.SetBufferSize(shared_nBufferSize);

	// Specification de la portion du fichier a traiter
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + shared_nBufferSize, inputFile.GetFileSize());

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	if (lBeginPos > 0)
	{
		bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
		if (bOk)
		{
			// On se positionne sur le debut de la ligne suivante si elle est trouvee
			if (lNextLinePos != -1)
			{
				lBeginPos = lNextLinePos;
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

	if (bOk and inputFile.GetCurrentBufferSize() > 0)
	{
		// Saut du Header
		// Il n'est pas utile ici d'avoir un warning en cas de ligne trop longue
		if (shared_bHeaderLineUsed and input_lFilePos == (longint)0)
			inputFile.SkipLine(bLineTooLong);

		// Parcours du buffer d'entree
		output_lSampledKeysUsedMemory = 0;
		keyExtractor.SetBufferedFile(&inputFile);
		nCount = 0;
		while (not inputFile.IsBufferEnd())
		{
			// Gestion de la progesssion
			nCount++;
			if (TaskProgression::IsRefreshNecessary(nCount))
			{
				dProgression = inputFile.GetCurrentLineIndex() * 1.0 / inputFile.GetBufferLineNumber();
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}

			// Tirage aleatoire pour decider si on garde chaque record
			if (IthRandomDouble(inputFile.GetPositionInFile()) <= input_dSamplingRate)
			{
				// Extraction de la cle, que la ligne soti valdie ou non
				recordKey = new KWKey;
				bIsLineOk = keyExtractor.ParseNextKey(recordKey, NULL);

				// Memorisation de la cle et de sa memoire utilisee
				output_oaSampledKeys->GetObjectArray()->Add(recordKey);
				output_lSampledKeysUsedMemory += sizeof(KWKey*) + recordKey->GetUsedMemory();
			}
			// Saut de ligne sinon
			else
			{
				inputFile.SkipLine(bLineTooLong);
			}
		}

		// Tri des clefs extraites dans l'esclave
		if (not TaskProgression::IsInterruptionRequested())
		{
			output_oaSampledKeys->GetObjectArray()->SetCompareFunction(KWKeyCompare);
			output_oaSampledKeys->GetObjectArray()->Sort();
		}
		else
			bOk = false;

		// On renvoie au master le ratio qu'on a eu en entree
		// Cela permet a celui ci de savoir si l'esclave avait ete echantillonne avec un taux different
		// du taux dans le maitre (qui peut changer a tout moment)
		output_dSamplingRate = input_dSamplingRate;

		// On envoi la taille du buffer traite dans l'esclave
		output_dBufferSize = inputFile.GetCurrentBufferSize();
	}
	else
	{
		output_dSamplingRate = input_dSamplingRate;
		output_dBufferSize = 0;
	}

	// Nettoyage si KO
	if (not bOk)
		output_oaSampledKeys->GetObjectArray()->DeleteAll();
	return bOk;
}

boolean KWKeySampleExtractorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage
	keyExtractor.Clean();

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
}

//////////////////////////////////
// Implementation de SortedSample

KWSortedKeySample::KWSortedKeySample()
{
	nHeadKeyIndex = 0;
}

KWSortedKeySample::~KWSortedKeySample()
{
	oaKeys.DeleteAll();
}

void KWSortedKeySample::ImportKeys(ObjectArray* oaSourceKeys)
{
	require(oaSourceKeys != NULL);
	require(oaSourceKeys->NoNulls());

	oaKeys.DeleteAll();
	oaKeys.CopyFrom(oaSourceKeys);
	nHeadKeyIndex = 0;
	oaSourceKeys->SetSize(0);
}

longint KWSortedKeySample::Sample(double dSamplingRatio, int ntaskIndex)
{
	KWKey* key;
	longint lRemovedKeysUsedMemory;
	int i;
	int nNewIndex;

	require(0 < dSamplingRatio and dSamplingRatio <= 1);

	// Detruit une partie des cle selon le taux d'echantilonnage
	lRemovedKeysUsedMemory = 0;
	nNewIndex = 0;
	for (i = nHeadKeyIndex; i < oaKeys.GetSize(); i++)
	{
		key = cast(KWKey*, oaKeys.GetAt(i));
		check(key);

		// Destruction si necessaire
		if (IthRandomDouble(ntaskIndex) > dSamplingRatio)
		{
			lRemovedKeysUsedMemory += sizeof(KWKey*) + key->GetUsedMemory();
			delete key;
			oaKeys.SetAt(i, NULL);
		}
		// Deplacement vers une nouvelle position en tete de liste sinon
		else
		{
			oaKeys.SetAt(nNewIndex, key);
			nNewIndex++;
		}
	}

	// Retaillage de la liste, et repositionnement de la nouvelle tete de liste
	oaKeys.SetSize(nNewIndex);
	nHeadKeyIndex = 0;

	// On rend la memoire liberee
	return lRemovedKeysUsedMemory;
}

int KWSortedKeySampleCompareHead(const void* elem1, const void* elem2)
{
	KWSortedKeySample* sk1;
	KWSortedKeySample* sk2;
	KWKey* key1;
	KWKey* key2;
	check(elem1);
	check(elem2);

	// Acces aux objets
	sk1 = cast(KWSortedKeySample*, *(Object**)elem1);
	sk2 = cast(KWSortedKeySample*, *(Object**)elem2);
	assert(sk1->GetCount() > 0);
	assert(sk2->GetCount() > 0);

	// Acces aux cles
	key1 = cast(KWKey*, sk1->GetFirstKey());
	key2 = cast(KWKey*, sk2->GetFirstKey());
	return key1->Compare(key2);
}
