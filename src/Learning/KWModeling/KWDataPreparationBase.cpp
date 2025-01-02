// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPreparationBase.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationBase

KWDataPreparationBase::KWDataPreparationBase()
{
	targetIndexAttribute = NULL;
	nTargetColumnIndex = 0;
	nColumnNumber = 0;
	nLineNumber = 0;
	nChunkTraceLevel = 0;
	bIsFillError = false;
}

KWDataPreparationBase::~KWDataPreparationBase()
{
	CleanPreparedData();
}

void KWDataPreparationBase::SetLearningSpec(KWLearningSpec* specification)
{
	KWLearningService::SetLearningSpec(specification);
	dataPreparationClass.SetLearningSpec(specification);
}

void KWDataPreparationBase::SetDataPreparationUsedAttributes(ObjectArray* oaDataPreparationAttributes)
{
	require(oaDataPreparationAttributes != NULL);
	require(not IsPreparedDataComputed());

	oaDataPreparationUsedAttributes.CopyFrom(oaDataPreparationAttributes);
}

void KWDataPreparationBase::ComputePreparedData()
{
	boolean bOk = true;
	KWClassDomain* initialCurrentDomain;
	KWClass* kwcDataPreparationClass;
	KWDatabase* loadDatabase;
	ALString sRefClassName;
	int nMaxMemoryColumnNumber;
	int nChunkNumber;
	int nMemoryChunkNumber;
	int nChunkColumnNumber;
	int nMemoryColumnNumber;
	longint lEstimatedNecessaryFreeSpace;
	IntVector* ivRecodingIndexVector;
	int nChunk;
	KWDataPreparationChunk* dataPreparationChunk;
	int nColumn;
	KWDataPreparationColumn* dataPreparationColumn;
	KWDataPreparationAttribute* dataPreparationAttribute;
	ALString sTmp;

	require(Check());
	require(GetLearningSpec()->IsTargetStatsComputed());

	// Nettoyage des donnees preparees precedentes
	CleanPreparedData();

	////////////////////////////////////////////////////////////////////////////////////
	// On commence par ouvrir la base de preparation en lecture, ce qui permet de tenir
	// de tenir compte de son empreinte memoire qui est difficile a evaluer (dictionnaire
	// intermediaire, buffers de fichier, objets references...)

	// Calcul des specifications des colonnes, necessaire a la preparation de la classe
	// On suppose ici que la mamoire n'est pas un probleme
	for (nColumn = 0; nColumn < oaDataPreparationUsedAttributes.GetSize(); nColumn++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nColumn));

		// Creation/initialisation d'une specification de colonne
		dataPreparationColumn = new KWDataPreparationColumn;
		dataPreparationColumn->SetDataPreparationAttribute(dataPreparationAttribute);

		// Rangement dans le dictionnaire de colonnes
		nkdPreparedDataColumns.SetAt((NUMERIC)dataPreparationAttribute, dataPreparationColumn);
	}

	// Acces a la classe de preparation
	kwcDataPreparationClass = dataPreparationClass.GetDataPreparationClass();

	// Initialisation de la classe de preparation des donnees
	initialCurrentDomain = KWClassDomain::GetCurrentDomain();
	SetupDataPreparationClass();

	// Recherche de la base de donnees
	// On va modifier temporairement sa classe associee, ce qui rendra les
	// specifications d'apprentissage incoherentes, le temps de l'utilisation de
	// la base dans cette methode
	loadDatabase = GetDatabase();
	sRefClassName = loadDatabase->GetClassName();
	loadDatabase->SetClassName(kwcDataPreparationClass->GetName());
	KWClassDomain::SetCurrentDomain(kwcDataPreparationClass->GetDomain());

	// Test d'ouverture de la base
	if (nChunkTraceLevel >= 1)
		cout << "KWDataPreparationBase, before open preparation database, heap memory:\t" << MemGetHeapMemory()
		     << endl;
	loadDatabase->OpenForRead();
	if (nChunkTraceLevel >= 1)
		cout << "KWDataPreparationBase, after open preparation database, heap memory:\t" << MemGetHeapMemory()
		     << endl;

	// Arret et sortie si probleme d'ouverture
	if (not loadDatabase->IsOpenedForRead())
	{
		AddError(sTmp + "Unable to open database " + loadDatabase->GetDatabaseName() + " for data recoding");
		bOk = false;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Calcul de la strategie d'utilisation des ressource memoires

	// Allocation prealable du vecteur des index cibles
	if (bOk)
	{
		ivTargetIndexes.SetLargeSize(GetInstanceNumber());

		// Arret et sortie si probleme d'allocation
		if (ivTargetIndexes.GetSize() < GetInstanceNumber())
		{
			ivTargetIndexes.SetSize(0);
			bOk = false;
			AddError(sTmp + "Unable to create target index vector of size " +
				 IntToString(GetInstanceNumber()) + " as a preparation step for data chunking");
		}
	}

	// On continue si OK
	nMaxMemoryColumnNumber = 0;
	nChunkNumber = 0;
	nMemoryChunkNumber = 0;
	nChunkColumnNumber = 0;
	if (bOk)
	{
		// Calcul du nombre de chunks en fonction de la memoire disponible (par estimation a priori)
		nMaxMemoryColumnNumber = ComputeMaxMemoryColumnNumber();
		ComputeChunkParameters(nMaxMemoryColumnNumber, nChunkNumber, nMemoryChunkNumber, nChunkColumnNumber);
		nMemoryColumnNumber = nMemoryChunkNumber * nChunkColumnNumber;

		// Trace de la strategie de chunking
		if (nChunkTraceLevel >= 1)
		{
			cout << "Initial chunk number\t" << nChunkNumber << endl;
			cout << "Initial memory chunk number\t" << nMemoryChunkNumber << endl;
			cout << "Initial chunk column number\t" << nChunkColumnNumber << endl;
		}

		// Creation des vecteurs d'index de recodage
		// Les vecteurs sont alloues avec une methode gerant les erreurs d'allocation,
		// ce qui permet de s'adapter a la memoire reellement disponible sans risque
		// de "planter" le programme
		oaFreeRecodingIndexVectors.SetSize(nMemoryColumnNumber);
		for (nColumn = 0; nColumn < nMemoryColumnNumber; nColumn++)
		{
			ivRecodingIndexVector = new IntVector;
			ivRecodingIndexVector->SetLargeSize(GetInstanceNumber());

			// Test si l'allocation s'est bien passee
			if (ivRecodingIndexVector->GetSize() == GetInstanceNumber())
				oaFreeRecodingIndexVectors.SetAt(nColumn, ivRecodingIndexVector);
			// Arret d'allocation si echec
			else
			{
				delete ivRecodingIndexVector;
				oaFreeRecodingIndexVectors.SetSize(nColumn);
				break;
			}
		}

		// Trace de la strategie de chunking
		if (nChunkTraceLevel >= 1)
			cout << "Number of recoding index vectors\t" << oaFreeRecodingIndexVectors.GetSize() << endl;

		// Redefinition des parametrage des chunks en cas de probleme d'allocation
		if (oaFreeRecodingIndexVectors.GetSize() == 0)
		{
			// Erreur si au moins une variable etait informative
			if (oaDataPreparationUsedAttributes.GetSize() > 0)
			{
				AddError(sTmp + "Unable to create index vectors of size " +
					 IntToString(GetInstanceNumber()) + " as a preparation step for data chunking");
				bOk = false;
			}
		}
		else if (oaFreeRecodingIndexVectors.GetSize() < nMemoryColumnNumber)
		{
			// Etant "limite" en memoire, on essaie de ne pas tout utiliser
			nMaxMemoryColumnNumber = oaFreeRecodingIndexVectors.GetSize();
			nMaxMemoryColumnNumber = (int)floor(nMaxMemoryColumnNumber * 0.9);
			if (nMaxMemoryColumnNumber < 1)
				nMaxMemoryColumnNumber = 1;

			// Recalcul du nombre de chunks en fonction de la memoire disponible
			ComputeChunkParameters(nMaxMemoryColumnNumber, nChunkNumber, nMemoryChunkNumber,
					       nChunkColumnNumber);
			nMemoryColumnNumber = nMemoryChunkNumber * nChunkColumnNumber;
			assert(nMemoryColumnNumber <= oaFreeRecodingIndexVectors.GetSize());

			// Trace de la strategie de chunking
			if (nChunkTraceLevel >= 1)
			{
				cout << "Memory allocated columns\t" << nMemoryColumnNumber << endl;
				cout << "Final chunk number\t" << nChunkNumber << endl;
				cout << "Final memory chunk number\t" << nMemoryChunkNumber << endl;
				cout << "Final chunk column number\t" << nChunkColumnNumber << endl;
			}

			// Liberation de la memoire allouee en trop
			for (nColumn = nMemoryColumnNumber; nColumn < oaFreeRecodingIndexVectors.GetSize(); nColumn++)
			{
				ivRecodingIndexVector = cast(IntVector*, oaFreeRecodingIndexVectors.GetAt(nColumn));
				delete ivRecodingIndexVector;
			}
			oaFreeRecodingIndexVectors.SetSize(nMemoryColumnNumber);
		}

		// Test s'il y a assez de memoire sur disque
		if (nChunkNumber > 1)
		{
			// Estimation approximative de la memoire necessaire, en ajoutant 4 Ko par chunk
			lEstimatedNecessaryFreeSpace =
			    oaDataPreparationUsedAttributes.GetSize() * GetInstanceNumber() * sizeof(int) +
			    nChunkNumber * KWDataPreparationChunk::nFileBufferMaxSize;

			// Test si memoire suffisante
			if (lEstimatedNecessaryFreeSpace > RMResourceManager::GetTmpDirFreeSpace())
			{
				AddError(sTmp + "Unsufficient disk space on temp dir (" + FileService::GetTmpDir() +
					 "): needs at least " +
					 LongintToHumanReadableString(lEstimatedNecessaryFreeSpace));
				bOk = false;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Preparation effective si tout s'est bien passe

	// On continue s'il y a assez de memoire
	if (bOk)
	{
		// Trace de la strategie de chunking
		if (nChunkTraceLevel >= 1)
			cout << "Creation of data preparation chunks\t" << nChunkNumber << endl;

		// Creation des chunks
		oaDataPreparationChunks.SetSize(nChunkNumber);
		for (nChunk = 0; nChunk < nChunkNumber; nChunk++)
			oaDataPreparationChunks.SetAt(nChunk, new KWDataPreparationChunk);

		// Trace de la strategie de chunking
		if (nChunkTraceLevel >= 1)
			cout << "Creation of data preparation columns\t" << oaDataPreparationUsedAttributes.GetSize()
			     << endl;

		// Memorisation des colonnes dans les chunks
		for (nColumn = 0; nColumn < oaDataPreparationUsedAttributes.GetSize(); nColumn++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nColumn));

			// Recherche dans le dictionnaire de colonnes
			dataPreparationColumn = cast(KWDataPreparationColumn*,
						     nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));
			check(dataPreparationColumn);

			// Recherche du chunk hebergeant la colonne
			nChunk = nColumn / nChunkColumnNumber;
			dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(nChunk));

			// Ajout de la colonne dans ce chunk
			dataPreparationChunk->GetColumns()->Add(dataPreparationColumn);
			dataPreparationColumn->SetChunk(dataPreparationChunk);
			dataPreparationColumn->SetColumnIndex(dataPreparationChunk->GetColumns()->GetSize() - 1);
		}

		// Chargement des donnees preparees
		if (nChunkTraceLevel >= 1)
			cout << "KWDataPreparationBase, before LoadPreparedData, heap memory:\t" << MemGetHeapMemory()
			     << endl;
		bOk = LoadPreparedData(loadDatabase);
		if (nChunkTraceLevel >= 1)
			cout << "KWDataPreparationBase, after LoadPreparedData, heap memory:\t" << MemGetHeapMemory()
			     << endl;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Nettoyage

	// Fermeture de la base
	if (loadDatabase->IsOpenedForRead())
		loadDatabase->Close();

	// Restitution de l'etat initial
	loadDatabase->SetClassName(sRefClassName);

	// Nettoyage de la classe de preparation des donnees
	CleanDataPreparationClass();
	KWClassDomain::SetCurrentDomain(initialCurrentDomain);

	// Nettoyage si echec
	if (not bOk)
		CleanPreparedData();
	if (nChunkTraceLevel >= 1)
		cout << "KWDataPreparationBase::ComputePreparedData: " << bOk << endl;
}

boolean KWDataPreparationBase::IsPreparedDataComputed()
{
	return oaDataPreparationUsedAttributes.GetSize() > 0 and
	       nkdPreparedDataColumns.GetCount() == oaDataPreparationUsedAttributes.GetSize();
}

void KWDataPreparationBase::CleanPreparedData()
{
	// Nettoyage des donnees
	nColumnNumber = 0;
	nLineNumber = 0;
	ivTargetIndexes.SetSize(0);

	// Nettoyage des specifications des colonnes
	nkdPreparedDataColumns.DeleteAll();
	oaDataPreparationChunks.DeleteAll();
	oaMemoryDataPreparationChunks.RemoveAll();
	oaFreeRecodingIndexVectors.DeleteAll();

	// Reinitialisation de l'indicateur d'erreur d'entres-sorties
	bIsFillError = false;
}

void KWDataPreparationBase::ShuffleDataPreparationUsedAttributes()
{
	ObjectArray oaShuffledDataPreparationChunks;
	ObjectArray oaShuffledDataPreparationColumns;
	KWDataPreparationChunk* dataPreparationChunk;
	KWDataPreparationColumn* dataPreparationColumn;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nChunk;
	int nColumn;
	int nAttribute;

	require(Check());
	require(IsPreparedDataComputed());

	// Parcours des chunks dans un ordre aleatoire (sans perturber l'ordre initial)
	nAttribute = 0;
	oaShuffledDataPreparationChunks.CopyFrom(&oaDataPreparationChunks);
	oaShuffledDataPreparationChunks.Shuffle();
	for (nChunk = 0; nChunk < oaShuffledDataPreparationChunks.GetSize(); nChunk++)
	{
		dataPreparationChunk = cast(KWDataPreparationChunk*, oaShuffledDataPreparationChunks.GetAt(nChunk));

		// Parcours des colonne du chunk dans un ordre aleatoire (sans perturber l'ordre initial)
		oaShuffledDataPreparationColumns.CopyFrom(dataPreparationChunk->GetColumns());
		oaShuffledDataPreparationColumns.Shuffle();
		for (nColumn = 0; nColumn < oaShuffledDataPreparationColumns.GetSize(); nColumn++)
		{
			dataPreparationColumn =
			    cast(KWDataPreparationColumn*, oaShuffledDataPreparationColumns.GetAt(nColumn));

			// On range l'attribut de preparation associe dans le container global
			dataPreparationAttribute = dataPreparationColumn->GetDataPreparationAttribute();
			oaDataPreparationUsedAttributes.SetAt(nAttribute, dataPreparationAttribute);
			nAttribute++;
		}
	}
	assert(nAttribute == oaDataPreparationUsedAttributes.GetSize());
	ensure(Check());
}

void KWDataPreparationBase::RestoreDataPreparationUsedAttributes()
{
	KWDataPreparationChunk* dataPreparationChunk;
	KWDataPreparationColumn* dataPreparationColumn;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nChunk;
	int nColumn;
	int nAttribute;

	require(Check());
	require(IsPreparedDataComputed());

	// Parcours des chunks dans un ordre aleatoire (sans perturber l'ordre initial)
	nAttribute = 0;
	for (nChunk = 0; nChunk < oaDataPreparationChunks.GetSize(); nChunk++)
	{
		dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(nChunk));

		// Parcours des colonne du chunk dans un ordre aleatoire (sans perturber l'ordre initial)
		for (nColumn = 0; nColumn < dataPreparationChunk->GetColumns()->GetSize(); nColumn++)
		{
			dataPreparationColumn =
			    cast(KWDataPreparationColumn*, dataPreparationChunk->GetColumns()->GetAt(nColumn));

			// On range l'attribut de preparation associe dans le container global
			dataPreparationAttribute = dataPreparationColumn->GetDataPreparationAttribute();
			oaDataPreparationUsedAttributes.SetAt(nAttribute, dataPreparationAttribute);
			nAttribute++;
		}
	}
	assert(nAttribute == oaDataPreparationUsedAttributes.GetSize());
	ensure(Check());
}

boolean KWDataPreparationBase::FillRecodingIndexesAt(KWDataPreparationAttribute* dataPreparationAttribute,
						     IntVector* ivObjectRecodings)
{
	boolean bOk = true;
	KWDataPreparationColumn* dataPreparationColumn;

	require(Check());
	require(IsPreparedDataComputed());
	require(dataPreparationAttribute != NULL);
	require(nkdPreparedDataColumns.Lookup(dataPreparationAttribute) != NULL);
	require(ivObjectRecodings != NULL);

	// Recherche de l'index de la colonne preparee
	dataPreparationColumn = cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
	check(dataPreparationColumn);

	// Chargement de la colonne en memoire
	bOk = LoadDataPreparationColumn(dataPreparationColumn);
	if (not bOk)
		bIsFillError = true;

	// Recopie des index de recodage
	ivObjectRecodings->CopyFrom(dataPreparationColumn->GetRecodingIndexes());
	return bOk;
}

boolean KWDataPreparationBase::FillTargetConditionalLnProbsAt(KWDataPreparationAttribute* dataPreparationAttribute,
							      int nTargetIndex,
							      ContinuousVector* cvUnivariateTargetConditionalProbs)
{
	boolean bOk = true;
	KWDataPreparationColumn* dataPreparationColumn;
	int nValueIndex;
	int nLine;
	IntVector* ivRecodingIndexes;

	require(Check());
	require(IsPreparedDataComputed());
	require(dataPreparationAttribute != NULL);
	require(nkdPreparedDataColumns.Lookup(dataPreparationAttribute) != NULL);
	require(0 <= nTargetIndex and
		nTargetIndex <
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize());
	require(cvUnivariateTargetConditionalProbs != NULL);

	// Recherche de l'index de la colonne preparee
	dataPreparationColumn = cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
	check(dataPreparationColumn);

	// Chargement de la colonne en memoire
	bOk = LoadDataPreparationColumn(dataPreparationColumn);
	if (not bOk)
		bIsFillError = true;
	ivRecodingIndexes = dataPreparationColumn->GetRecodingIndexes();

	// Parcours de index de recodage pour initialiser les probabilites conditionnelles
	cvUnivariateTargetConditionalProbs->SetSize(nLineNumber);
	for (nLine = 0; nLine < nLineNumber; nLine++)
	{
		nValueIndex = ivRecodingIndexes->GetAt(nLine);
		cvUnivariateTargetConditionalProbs->SetAt(
		    nLine, dataPreparationColumn->GetLnSourceConditionalProb(nValueIndex, nTargetIndex));
	}
	return bOk;
}

Continuous KWDataPreparationBase::FillTargetConditionalLnProbAt(KWDataPreparationAttribute* dataPreparationAttribute,
								int nTargetIndex, int nRecodingIndex)
{
	boolean bOk = true;
	KWDataPreparationColumn* dataPreparationColumn;

	require(Check());
	require(IsPreparedDataComputed());
	require(dataPreparationAttribute != NULL);
	require(nkdPreparedDataColumns.Lookup(dataPreparationAttribute) != NULL);
	require(0 <= nTargetIndex and
		nTargetIndex <
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize());

	// Recherche de l'index de la colonne preparee
	dataPreparationColumn = cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
	check(dataPreparationColumn);

	// Chargement de la colonne en memoire
	bOk = LoadDataPreparationColumn(dataPreparationColumn);
	if (not bOk)
		bIsFillError = true;

	return dataPreparationColumn->GetLnSourceConditionalProb(nRecodingIndex, nTargetIndex);
}

boolean
KWDataPreparationBase::UpgradeTargetConditionalLnProbsAt(KWDataPreparationAttribute* dataPreparationAttribute,
							 int nTargetIndex, Continuous cWeight,
							 ContinuousVector* cvInputMultivariateTargetConditionalProbs,
							 ContinuousVector* cvOutputMultivariateTargetConditionalProbs)
{
	boolean bOk = true;
	KWDataPreparationColumn* dataPreparationColumn;
	int nValueIndex;
	int nLine;
	IntVector* ivRecodingIndexes;

	require(Check());
	require(IsPreparedDataComputed());
	require(dataPreparationAttribute != NULL);
	require(nkdPreparedDataColumns.Lookup(dataPreparationAttribute) != NULL);
	require(0 <= nTargetIndex and
		nTargetIndex <
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize());
	require(cvInputMultivariateTargetConditionalProbs != NULL);
	require(cvInputMultivariateTargetConditionalProbs->GetSize() == nLineNumber);
	require(cvOutputMultivariateTargetConditionalProbs != NULL);
	require(cvOutputMultivariateTargetConditionalProbs->GetSize() == nLineNumber);

	// Recherche de l'index de la colonne preparee
	dataPreparationColumn = cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
	check(dataPreparationColumn);

	// Chargement de la colonne en memoire
	bOk = LoadDataPreparationColumn(dataPreparationColumn);
	if (not bOk)
		bIsFillError = true;
	ivRecodingIndexes = dataPreparationColumn->GetRecodingIndexes();

	// Parcours de index de recodage pour mettre a jour les probabilites conditionnelles multivariees
	for (nLine = 0; nLine < nLineNumber; nLine++)
	{
		nValueIndex = ivRecodingIndexes->GetAt(nLine);
		cvOutputMultivariateTargetConditionalProbs->SetAt(
		    nLine, cvInputMultivariateTargetConditionalProbs->GetAt(nLine) +
			       cWeight * dataPreparationColumn->GetLnSourceConditionalProb(nValueIndex, nTargetIndex));
	}
	return bOk;
}

void KWDataPreparationBase::WriteIndexedPreparedDataFile(const ALString& sFileName)
{
	fstream fstIndexedPreparedDataFile;
	boolean bOk;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationColumn* dataPreparationColumn;
	ObjectArray oaDataPreparationColumns;
	int nValueIndex;
	int nLine;
	debug(int nTestedAttribute = oaDataPreparationUsedAttributes.GetSize() / 2);
	debug(IntVector ivObjectRecodings;);

	require(Check());
	require(IsPreparedDataComputed());

	// Calcul des index d'une colonne en mode debug, pour effectuer des test de coherence
	debug(dataPreparationAttribute =
		  cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nTestedAttribute)));
	debug(FillRecodingIndexesAt(dataPreparationAttribute, &ivObjectRecodings));

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sFileName, fstIndexedPreparedDataFile);

	// Ecriture de son contenu
	if (bOk)
	{
		// Ecriture de l'entete
		oaDataPreparationColumns.SetSize(oaDataPreparationUsedAttributes.GetSize());
		for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes.GetSize(); nAttribute++)
		{
			// Acces au specifications
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nAttribute));

			// Memorisation de la colonne
			dataPreparationColumn =
			    cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
			oaDataPreparationColumns.SetAt(nAttribute, dataPreparationColumn);

			// Ecriture du nom de la variable recodee
			fstIndexedPreparedDataFile << dataPreparationAttribute->GetPreparedAttribute()->GetName()
						   << "Index";
			fstIndexedPreparedDataFile << "\t";
		}
		fstIndexedPreparedDataFile << GetTargetAttributeName() << "Index"
					   << "\n";

		// Ecriture des enregistrements
		for (nLine = 0; nLine < nLineNumber; nLine++)
		{
			// Ecriture des index des variables recodees
			for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes.GetSize(); nAttribute++)
			{
				// Chargement de la colonne en memoire
				dataPreparationColumn =
				    cast(KWDataPreparationColumn*, oaDataPreparationColumns.GetAt(nAttribute));
				LoadDataPreparationColumn(dataPreparationColumn);

				// Ecriture de l'index de la variable recodee
				nValueIndex = dataPreparationColumn->GetRecodingIndexes()->GetAt(nLine);
				fstIndexedPreparedDataFile << nValueIndex;
				fstIndexedPreparedDataFile << "\t";

				// Verifications en mode debug
				debug(assert(nAttribute != nTestedAttribute or
					     ivObjectRecodings.GetAt(nLine) == nValueIndex));
			}

			// Index de la valeur cible
			fstIndexedPreparedDataFile << ivTargetIndexes.GetAt(nLine);
			fstIndexedPreparedDataFile << "\n";
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sFileName, fstIndexedPreparedDataFile);
	}
}

void KWDataPreparationBase::WriteScoredPreparedDataFile(const ALString& sFileName)
{
	fstream fstScoredPreparedDataFile;
	boolean bOk;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationColumn* dataPreparationColumn;
	ObjectArray oaDataPreparationColumns;
	int nValueIndex;
	int nTargetIndex;
	int nLine;
	int nAttributeTargetNumber;
	debug(int nTestedAttribute = oaDataPreparationUsedAttributes.GetSize() / 2);
	debug(int nTestedTargetIndex = 0);
	debug(ContinuousVector cvObjectScores;);

	require(Check());
	require(IsPreparedDataComputed());

	// Calcul des score d'une colonne en mode debug, pour effectuer des test de coherence
	debug(dataPreparationAttribute =
		  cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nTestedAttribute)));
	debug(FillTargetConditionalLnProbsAt(dataPreparationAttribute, nTestedTargetIndex, &cvObjectScores));

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sFileName, fstScoredPreparedDataFile);

	// Ecriture de son contenu
	if (bOk)
	{
		// Ecriture de l'entete
		oaDataPreparationColumns.SetSize(oaDataPreparationUsedAttributes.GetSize());
		for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes.GetSize(); nAttribute++)
		{
			// Acces au specifications
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nAttribute));

			// Memorisation de l'index de colonne
			dataPreparationColumn =
			    cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup(dataPreparationAttribute));
			oaDataPreparationColumns.SetAt(nAttribute, dataPreparationColumn);

			// Ecriture du nom des variables recodees par valeur cible
			nAttributeTargetNumber = dataPreparationAttribute->GetPreparedStats()
						     ->GetPreparedDataGridStats()
						     ->ComputeTargetGridSize();
			for (nTargetIndex = 0; nTargetIndex < nAttributeTargetNumber; nTargetIndex++)
			{
				fstScoredPreparedDataFile << dataPreparationAttribute->GetPreparedAttribute()->GetName()
							  << "_Score" << nTargetIndex;
				fstScoredPreparedDataFile << "\t";
			}
		}
		fstScoredPreparedDataFile << GetTargetAttributeName() << "Index"
					  << "\n";

		// Ecriture des enregistrements
		for (nLine = 0; nLine < nLineNumber; nLine++)
		{
			// Ecriture des index des variables recodees
			for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes.GetSize(); nAttribute++)
			{
				// Acces au specifications
				dataPreparationAttribute = cast(KWDataPreparationAttribute*,
								oaDataPreparationUsedAttributes.GetAt(nAttribute));

				// Chargement de la colonne en memoire
				dataPreparationColumn =
				    cast(KWDataPreparationColumn*, oaDataPreparationColumns.GetAt(nAttribute));
				LoadDataPreparationColumn(dataPreparationColumn);

				// Ecriture des scores par valeur cible de la variable recodee
				nValueIndex = dataPreparationColumn->GetRecodingIndexes()->GetAt(nLine);
				dataPreparationColumn =
				    cast(KWDataPreparationColumn*, oaDataPreparationColumns.GetAt(nAttribute));
				nAttributeTargetNumber = dataPreparationAttribute->GetPreparedStats()
							     ->GetPreparedDataGridStats()
							     ->ComputeTargetGridSize();
				for (nTargetIndex = 0; nTargetIndex < nAttributeTargetNumber; nTargetIndex++)
				{
					fstScoredPreparedDataFile << dataPreparationColumn->GetLnSourceConditionalProb(
					    nValueIndex, nTargetIndex);
					fstScoredPreparedDataFile << "\t";

					// Verifications en mode debug
					debug(assert(nAttribute != nTestedAttribute or
						     nTargetIndex != nTestedTargetIndex or
						     fabs(dataPreparationColumn->GetLnSourceConditionalProb(
							      nValueIndex, nTargetIndex) -
							  cvObjectScores.GetAt(nLine)) < 1e-5));
				}
			}

			// Index de la valeur cible
			fstScoredPreparedDataFile << ivTargetIndexes.GetAt(nLine);
			fstScoredPreparedDataFile << "\n";
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sFileName, fstScoredPreparedDataFile);
	}
}

boolean KWDataPreparationBase::CheckPreparedData() const
{
	boolean bOk;
	ALString sTmp;
	int nColumn;
	int nTotalColumnNumber;
	int nChunk;
	KWDataPreparationChunk* dataPreparationChunk;
	KWDataPreparationColumn* dataPreparationColumn;
	KWDataPreparationAttribute* dataPreparationAttribute;

	// Verification des specifications
	bOk = Check();

	// Verification de la coherence du nombre de colonnes
	if (bOk)
	{
		if (oaDataPreparationUsedAttributes.GetSize() != nkdPreparedDataColumns.GetCount())
		{
			bOk = false;
			AddError(sTmp + "The number of columns (" + IntToString(nkdPreparedDataColumns.GetCount()) +
				 ") is different from the number of prepared variables (" +
				 IntToString(oaDataPreparationUsedAttributes.GetSize()) + ")");
		}
	}

	// Verification de la coherences des donnees prepares avec les specifications
	if (bOk)
	{
		for (nColumn = 0; nColumn < oaDataPreparationUsedAttributes.GetSize(); nColumn++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nColumn));

			// Recherche de la colonne correspondant a l'attribut
			dataPreparationColumn = cast(KWDataPreparationColumn*,
						     nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));

			// Erreur si non tropuve
			if (dataPreparationColumn == NULL)
			{
				bOk = false;
				AddError("Column not found for variable " + dataPreparationAttribute->GetObjectLabel());
			}
			// Tests d'integrite sinon
			else
			{
				assert(dataPreparationColumn->GetDataPreparationAttribute() ==
				       dataPreparationAttribute);
				assert(dataPreparationColumn->GetColumnIndex() == nColumn);
			}
		}
	}

	// Verification de la coherences des colonnes avec les chunks
	if (bOk)
	{
		// Parcours des chunks pour verifier leur integrite
		nTotalColumnNumber = 0;
		for (nChunk = 0; nChunk < oaDataPreparationChunks.GetSize(); nChunk++)
		{
			dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(nChunk));

			// Verification des colonnes du chunk
			for (nColumn = 0; nColumn < dataPreparationChunk->GetColumns()->GetSize(); nColumn++)
			{
				dataPreparationColumn =
				    cast(KWDataPreparationColumn*, dataPreparationChunk->GetColumns()->GetAt(nColumn));
				nTotalColumnNumber++;

				// Verification de la colonne
				check(dataPreparationColumn);
				dataPreparationAttribute = dataPreparationColumn->GetDataPreparationAttribute();
				check(dataPreparationAttribute);
				assert(nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute) ==
				       dataPreparationColumn);

				// Verification de son index
				assert(dataPreparationColumn->GetColumnIndex() == nColumn);
			}
		}

		// Verification de la coherence du nombre de colonnes dans les chunks
		if (oaDataPreparationUsedAttributes.GetSize() != nTotalColumnNumber)
		{
			bOk = false;
			AddError(sTmp + "The number of columns in the chunks (" + IntToString(nTotalColumnNumber) +
				 ") is different from the number of prepared variables (" +
				 IntToString(oaDataPreparationUsedAttributes.GetSize()) + ")");
		}
	}

	return bOk;
}

boolean KWDataPreparationBase::Check() const
{
	boolean bOk = true;
	ObjectArray* oaDataPreparationAttributes;
	NumericKeyDictionary nkdDataPreparationAttributes;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationAttribute* dataPreparationUsedAttribute;

	// Acces aux specifications de preparation par attribut dans une variable locale
	// (pour eviter les warning sur les const)
	oaDataPreparationAttributes =
	    cast(KWDataPreparationClass*, &dataPreparationClass)->GetDataPreparationAttributes();

	// Validation des specifications
	bOk = KWLearningService::Check();

	// Test de validite de la preparation
	if (bOk and GetLearningSpec() != dataPreparationClass.GetLearningSpec())
	{
		bOk = false;
		AddError("Learning specifications inconsistent with those of data preparation");
	}
	if (bOk and not dataPreparationClass.CheckDataPreparation())
	{
		bOk = false;
		AddError("Incorrect data preparation specifications");
	}

	// Test de coherence entre les attributs utilises et les attributs disponibles
	if (bOk)
	{
		// Rangement des attributs de preparation dans un dictionnaire
		for (nAttribute = 0; nAttribute < oaDataPreparationAttributes->GetSize(); nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationAttributes->GetAt(nAttribute));
			nkdDataPreparationAttributes.SetAt((NUMERIC)dataPreparationAttribute, dataPreparationAttribute);
		}

		// Recherche des attributs de preparation utilises parmi les attributs disponibles
		for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes.GetSize(); nAttribute++)
		{
			dataPreparationUsedAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nAttribute));

			// Recherche dans le dictionnaire
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 nkdDataPreparationAttributes.Lookup((NUMERIC)dataPreparationUsedAttribute));

			// Erreur si non trouve
			if (dataPreparationAttribute == NULL)
			{
				bOk = false;
				AddError("Used variable " + dataPreparationUsedAttribute->GetObjectLabel() +
					 " not available");
				break;
			}
		}
	}
	return bOk;
}

void KWDataPreparationBase::Write(ostream& ost) const
{
	KWClass* kwcDataPreparationClass;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int i;

	require(Check());

	// En tete
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Dictionnaire
	kwcDataPreparationClass = cast(KWDataPreparationClass*, &dataPreparationClass)->GetDataPreparationClass();
	ost << *kwcDataPreparationClass << endl;

	// Affichage des details par attributs utilise
	for (i = 0; i < oaDataPreparationUsedAttributes.GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(i));
		ost << *dataPreparationAttribute;
	}
	ost << endl;
}

const ALString KWDataPreparationBase::GetClassLabel() const
{
	return "Prepared database";
}

const ALString KWDataPreparationBase::GetObjectLabel() const
{
	if (GetLearningSpec() == NULL)
		return "";
	else
		return GetLearningSpec()->GetObjectLabel();
}

////////////////////////////////////////////////////////////////////////

boolean KWDataPreparationBase::LoadDataPreparationColumn(KWDataPreparationColumn* dataPreparationColumn)
{
	boolean bOk = true;
	int nChunk;
	KWDataPreparationChunk* dataPreparationChunk;
	int nChunkToUnload;
	double dOldestFreshness;

	require(Check());
	require(IsPreparedDataComputed());
	require(dataPreparationColumn != NULL);

	// Trace
	if (nChunkTraceLevel >= 3)
		cout << "Load\tColumn\t"
		     << dataPreparationColumn->GetDataPreparationAttribute()->GetPreparedAttribute()->GetName() << endl;

	// Chargement en memoire du chunk de la colonne si necessaire
	if (not dataPreparationColumn->GetChunk()->IsLoaded())
	{
		// Test s'il est necessaire de decharger un chunk
		if (oaFreeRecodingIndexVectors.GetSize() < dataPreparationColumn->GetChunk()->GetColumns()->GetSize())
		{
			assert(oaMemoryDataPreparationChunks.GetSize() > 0);

			// Recherche d'un chunk a decharger de la memoire
			nChunkToUnload = -1;
			dOldestFreshness = DBL_MAX;
			for (nChunk = 0; nChunk < oaMemoryDataPreparationChunks.GetSize(); nChunk++)
			{
				dataPreparationChunk =
				    cast(KWDataPreparationChunk*, oaMemoryDataPreparationChunks.GetAt(nChunk));

				// Verifications
				assert(dataPreparationChunk->IsLoaded());
				assert(dataPreparationChunk->GetLoadFreshness() > 0);

				// Test si chunk le plus ancien
				if (dataPreparationChunk->GetLoadFreshness() < dOldestFreshness)
				{
					dOldestFreshness = dataPreparationChunk->GetLoadFreshness();
					nChunkToUnload = nChunk;
				}
			}
			assert(nChunkToUnload >= 0);

			// Dechargement du chunk le plus anciennnement charge
			dataPreparationChunk =
			    cast(KWDataPreparationChunk*, oaMemoryDataPreparationChunks.GetAt(nChunkToUnload));
			dataPreparationChunk->UnloadChunk(&oaFreeRecodingIndexVectors);

			// On supprime le chunk du tableau des chunks en memoire en deplacant le dernier
			// element a la place du chunk decharge
			oaMemoryDataPreparationChunks.SetAt(
			    nChunkToUnload,
			    oaMemoryDataPreparationChunks.GetAt(oaMemoryDataPreparationChunks.GetSize() - 1));
			oaMemoryDataPreparationChunks.SetSize(oaMemoryDataPreparationChunks.GetSize() - 1);

			// Trace du dechargement
			if (nChunkTraceLevel >= 2)
				cout << "Unload\tChunk\t" << dataPreparationChunk->GetChunkFileName() << endl;
		}
		assert(oaFreeRecodingIndexVectors.GetSize() >=
		       dataPreparationColumn->GetChunk()->GetColumns()->GetSize());

		// Chargement en memoire du chunk
		bOk = dataPreparationColumn->GetChunk()->LoadChunk(&oaFreeRecodingIndexVectors);

		// Memorisation du chunk dans le tableau des chunks en memoire
		oaMemoryDataPreparationChunks.Add(dataPreparationColumn->GetChunk());

		// Trace du chargement
		if (nChunkTraceLevel >= 2)
			cout << "Load\tChunk\t" << dataPreparationColumn->GetChunk()->GetChunkFileName() << endl;
	}

	// Verifications
	ensure(dataPreparationColumn->GetChunk()->IsLoaded());
	ensure(dataPreparationColumn->GetRecodingIndexes() != NULL);
	ensure(dataPreparationColumn->GetRecodingIndexes()->GetSize() == nLineNumber);
	return bOk;
}

boolean KWDataPreparationBase::LoadPreparedData(KWDatabase* loadDatabase)
{
	boolean bOk = true;
	boolean bLoadInMemory;
	ALString sChunkFileName;
	int nChunk;
	KWDataPreparationColumn* dataPreparationColumn;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationChunk* dataPreparationChunk;
	NumericKeyDictionary nkdDataPreparationColumnsByAttribute;
	ObjectArray oaChunksByAttribute;
	IntVector ivColumnIndexesByAttribute;
	KWLoadIndexVector livAttributeLoadIndexes;
	KWClass* kwcDataPreparationClass;
	KWAttribute* indexingAttribute;
	KWObject* kwoObject;
	int nRecord;
	int nObject;
	ALString sTmp;
	int nLine;
	int nColumn;
	int nValueIndex;
	int nAttribute;
	PeriodicTest periodicTestDisplay;

	require(ivTargetIndexes.GetSize() == GetInstanceNumber());
	require(loadDatabase != NULL);
	require(loadDatabase->IsOpenedForRead());

	// Acces a la classe de preparation
	kwcDataPreparationClass = dataPreparationClass.GetDataPreparationClass();

	// Initialisation des containers d'acceuil des donnees preparees
	nColumnNumber = oaDataPreparationUsedAttributes.GetSize();
	nLineNumber = GetInstanceNumber();
	assert(GetTargetColumnIndex() == nColumnNumber);
	bLoadInMemory = oaDataPreparationChunks.GetSize() == 1;
	dataPreparationChunk = NULL;

	// Preparation de l'unique chunk dans le cas du chargement en memoire
	if (bLoadInMemory)
	{
		dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(0));
		dataPreparationChunk->SetChunkMemory(&oaFreeRecodingIndexVectors);
	}
	// Sinon, preparation de l'ensemble des chunks
	else
	{
		// Parcours pour creation en ecriture des fichier de chunks
		for (nChunk = 0; nChunk < oaDataPreparationChunks.GetSize(); nChunk++)
		{
			dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(nChunk));

			// Creation d'un fichier en ecriture
			sChunkFileName = BuildChunkFileName(nChunk);
			dataPreparationChunk->SetChunkFileName(sChunkFileName);
			bOk = dataPreparationChunk->OpenOutputChunkFile();
			if (not bOk)
			{
				AddError(sTmp + "Only " + IntToString(nChunk) + "/" +
					 IntToString(oaDataPreparationChunks.GetSize()) +
					 " prepared chunk data files could be created");
				break;
			}
		}

		// Rangement des colonnes dans un dictionnaire indexe par les attributs d'indexation
		// de la classe de preparation
		for (nColumn = 0; nColumn < oaDataPreparationUsedAttributes.GetSize(); nColumn++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(nColumn));

			// Recherche de la colonne correspondant a l'attribut
			dataPreparationColumn = cast(KWDataPreparationColumn*,
						     nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));
			check(dataPreparationColumn);

			// Rangement dans le dictionnaire indexe par les attribut de recodage
			indexingAttribute = dataPreparationColumn->GetIndexingAttribute();
			check(indexingAttribute);
			nkdDataPreparationColumnsByAttribute.SetAt((NUMERIC)indexingAttribute, dataPreparationColumn);
		}

		// Rangement dans un tableau des chunks associes a chaque attribut d'indexation
		oaChunksByAttribute.SetSize(kwcDataPreparationClass->GetLoadedAttributeNumber());
		ivColumnIndexesByAttribute.SetSize(kwcDataPreparationClass->GetLoadedAttributeNumber());
		for (nAttribute = 0; nAttribute < kwcDataPreparationClass->GetLoadedAttributeNumber(); nAttribute++)
		{
			indexingAttribute = kwcDataPreparationClass->GetLoadedAttributeAt(nAttribute);

			// Recherche de la colonne correspondant a l'attribut
			dataPreparationColumn =
			    cast(KWDataPreparationColumn*,
				 nkdDataPreparationColumnsByAttribute.Lookup((NUMERIC)indexingAttribute));

			// Rangement du chunk correspondant ainsi que l'index de la colonne dans le chunk
			if (dataPreparationColumn != NULL)
			{
				oaChunksByAttribute.SetAt(nAttribute, dataPreparationColumn->GetChunk());
				ivColumnIndexesByAttribute.SetAt(nAttribute, dataPreparationColumn->GetColumnIndex());
			}
		}
	}

	// Lecture des donnees enregistrement par enregistrement pour alimenter
	// les scores par attributs
	nRecord = 0;
	nObject = 0;
	if (bOk)
	{
		// Memorisation des index de chargement des attributs
		livAttributeLoadIndexes.SetSize(kwcDataPreparationClass->GetLoadedAttributeNumber());
		for (nAttribute = 0; nAttribute < kwcDataPreparationClass->GetLoadedAttributeNumber(); nAttribute++)
			livAttributeLoadIndexes.SetAt(
			    nAttribute, kwcDataPreparationClass->GetLoadedAttributeAt(nAttribute)->GetLoadIndex());

		// Lecture des donnees
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel("Database recoding: initial analysis of " +
						  loadDatabase->GetDatabaseName());
		Global::ActivateErrorFlowControl();
		nLine = 0;
		nColumn = 0;
		while (bOk and not loadDatabase->IsEnd())
		{
			kwoObject = loadDatabase->Read();
			nRecord++;
			if (kwoObject != NULL)
			{
				nObject++;

				// Parcours des champs de l'objet pour memoriser les index de recodage (et de valeur
				// cible)
				for (nAttribute = 0; nAttribute < kwcDataPreparationClass->GetLoadedAttributeNumber();
				     nAttribute++)
				{
					nValueIndex = (int)floor(
					    kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute)) +
					    0.5);

					// Cas de l'attribut cible
					if (nAttribute == GetTargetColumnIndex())
						ivTargetIndexes.SetAt(nLine, nValueIndex);
					// Cas des attributs recodes
					else
					{
						// Mise a jour en memoire dans le cas d'un seul chunk
						if (bLoadInMemory)
						{
							check(dataPreparationChunk);
							assert(dataPreparationChunk->IsLoaded());

							// Mise a jour en de l'index de recodage en memoire
							dataPreparationChunk->SetRecodingIndexAt(nLine, nAttribute,
												 nValueIndex);
						}
						// Mise a jour dans le fichier de chunk sinon
						else
						{
							// Recherche du chunk correspondant a l'attribut
							dataPreparationChunk =
							    cast(KWDataPreparationChunk*,
								 oaChunksByAttribute.GetAt(nAttribute));
							check(dataPreparationChunk);

							// Recherche de l'index de la colonne dans le chunk
							nColumn = ivColumnIndexesByAttribute.GetAt(nAttribute);

							// Mise a jour en de l'index de recodage dans le fichier du
							// chunk
							dataPreparationChunk->WriteChunkFileRecodingIndexAt(
							    nLine, nColumn, nValueIndex);
						}
					}
				}

				// Arret si erreur d'ecriture dans le chunk
				if (dataPreparationChunk->IsError())
					bOk = false;

				// Instance suivante
				nLine++;

				// Nettoyage
				delete kwoObject;
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}

			// Arret si erreur de lecture dans la base
			if (loadDatabase->IsError())
				bOk = false;

			// Suivi de tache (de temps en temps pour ne pas penaliser le temps de calcul)
			if (periodicTestDisplay.IsTestAllowed(nRecord))
			{
				TaskProgression::DisplayProgression((nObject * 100) / GetInstanceNumber());
			}
		}
		Global::DesactivateErrorFlowControl();
		TaskProgression::EndTask();

		// Fermeture
		bOk = loadDatabase->Close() and bOk;
	}

	// Fermeture des fichiers de chunk
	if (not bLoadInMemory)
	{
		// Parcours pour fermeture des chunks
		Global::ActivateErrorFlowControl();
		for (nChunk = 0; nChunk < oaDataPreparationChunks.GetSize(); nChunk++)
		{
			dataPreparationChunk = cast(KWDataPreparationChunk*, oaDataPreparationChunks.GetAt(nChunk));

			// Fermeture du fichier
			bOk = dataPreparationChunk->CloseChunkFile() and bOk;
		}
		Global::DesactivateErrorFlowControl();
	}

	// Test si interruption demandee
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Test si on a lu le bon nombre d'instances
	if (nObject != GetInstanceNumber() and bOk)
	{
		bOk = false;
		AddError(sTmp + "Number of managed instances (" + IntToString(nObject) +
			 ") inconsistent with the number of initial instances (" + IntToString(GetInstanceNumber()));
	}
	return bOk;
}

const ALString KWDataPreparationBase::BuildChunkFileName(int nChunk)
{
	ALString sChunkBaseName = "DataChunk";
	return FileService::CreateTmpFile(sChunkBaseName + IntToString(nChunk) + ".dat", this);
}

void KWDataPreparationBase::SetupDataPreparationClass()
{
	KWClass* kwcDataPreparationClass;
	int i;
	KWAttribute* indexingAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationColumn* dataPreparedColum;
	KWClassDomain* workingClassDomain;
	KWDRDiff* diffRule;

	require(Check());
	require(targetIndexAttribute == NULL);

	// Acces a la classe de preparation
	kwcDataPreparationClass = dataPreparationClass.GetDataPreparationClass();

	// Ajout d'attributs d'indexation des valeurs pour chaque attribut de recodage
	for (i = 0; i < oaDataPreparationUsedAttributes.GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(i));

		// Recherche des specifications de la colonne correspondante
		dataPreparedColum =
		    cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));
		check(dataPreparedColum);

		// Creation et memorisation d'un attribut d'indexation
		indexingAttribute = dataPreparedColum->GetDataPreparationAttribute()->AddPreparedIndexingAttribute();
		dataPreparedColum->SetIndexingAttribute(indexingAttribute);

		// Modification de la regle d'indexation
		// On soustrait 1 a l'index pour passer d'un index "externe" (entre 1 et N) a un index "interne" (entre
		// 0 et N-1)
		diffRule = new KWDRDiff;
		diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		diffRule->GetFirstOperand()->SetDerivationRule(indexingAttribute->GetDerivationRule()->Clone());
		diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		diffRule->GetSecondOperand()->SetContinuousConstant(1);
		diffRule->CompleteTypeInfo(kwcDataPreparationClass);
		indexingAttribute->SetDerivationRule(diffRule);
	}

	// Ajout de l'attribut de calcul des index de valeurs cibles
	targetIndexAttribute = dataPreparationClass.GetDataPreparationTargetAttribute()->AddPreparedIndexingAttribute();

	// Modification de la regle d'indexation
	// On soustrait 1 a l'index pour passer d'un index "externe" (entre 1 et N) a un index "interne" (entre 0 et
	// N-1)
	diffRule = new KWDRDiff;
	diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	diffRule->GetFirstOperand()->SetDerivationRule(targetIndexAttribute->GetDerivationRule()->Clone());
	diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	diffRule->GetSecondOperand()->SetContinuousConstant(1);
	diffRule->CompleteTypeInfo(kwcDataPreparationClass);
	targetIndexAttribute->SetDerivationRule(diffRule);

	// Passage de tous les attributs en etat Unused (et non Loaded)
	kwcDataPreparationClass->SetAllAttributesUsed(false);

	// Passage en Used et Loaded des attributs d'indexation dans la classe de preparation
	for (i = 0; i < oaDataPreparationUsedAttributes.GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(i));

		// Recherche de l'attribut d'indexation memorise dans la specification de colonne correspondante
		dataPreparedColum =
		    cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));
		check(dataPreparedColum);
		indexingAttribute = dataPreparedColum->GetIndexingAttribute();

		// Passage de l'attribut d'indexation en Used Loaded
		indexingAttribute->SetUsed(true);
		indexingAttribute->SetLoaded(true);
	}

	// Passage de l'attribut d'indexation des valeurs cibles en Used Loaded
	targetIndexAttribute->SetUsed(true);
	targetIndexAttribute->SetLoaded(true);

	// Compilation de la classe de preparation
	workingClassDomain = dataPreparationClass.GetDataPreparationDomain();
	workingClassDomain->Compile();

	// Memorisation des index de colonne des donnes preparees
	assert(kwcDataPreparationClass->GetLoadedAttributeAt(kwcDataPreparationClass->GetLoadedAttributeNumber() - 1) ==
	       targetIndexAttribute);
	nTargetColumnIndex = kwcDataPreparationClass->GetLoadedAttributeNumber() - 1;
	ensure(kwcDataPreparationClass->IsCompiled());
}

void KWDataPreparationBase::CleanDataPreparationClass()
{
	KWClass* kwcDataPreparationClass;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationColumn* dataPreparedColum;
	KWAttribute* indexingAttribute;
	int i;

	require(targetIndexAttribute != NULL);

	// Acces a la classe de preparation
	kwcDataPreparationClass = dataPreparationClass.GetDataPreparationClass();
	require(kwcDataPreparationClass->IsCompiled());

	// Destruction des attributs d'indexation
	for (i = 0; i < oaDataPreparationUsedAttributes.GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes.GetAt(i));

		// Recherche des specifications de la colonne correspondante
		dataPreparedColum =
		    cast(KWDataPreparationColumn*, nkdPreparedDataColumns.Lookup((NUMERIC)dataPreparationAttribute));
		check(dataPreparedColum);

		// Creation et memorisation d'un attribut d'indexation
		indexingAttribute = dataPreparedColum->GetIndexingAttribute();

		// Destruction de l'attribut d'index des modalites recodees
		kwcDataPreparationClass->DeleteAttribute(indexingAttribute->GetName());
		dataPreparedColum->SetIndexingAttribute(NULL);
	}

	// Destruction de l'attribut d'index des modalites cibles
	kwcDataPreparationClass->DeleteAttribute(targetIndexAttribute->GetName());
	targetIndexAttribute = NULL;
	nTargetColumnIndex = 0;

	// Passage de tous les attributs en etat Used Loaded
	kwcDataPreparationClass->SetAllAttributesUsed(true);
	kwcDataPreparationClass->SetAllAttributesLoaded(true);

	ensure(targetIndexAttribute == NULL);
}

void KWDataPreparationBase::ComputeChunkParameters(int nMaxMemoryColumnNumber, int& nChunkNumber,
						   int& nMemoryChunkNumber, int& nChunkColumnNumber)
{
	require(nMaxMemoryColumnNumber > 0);

	// On regarde en priorite si tout peut tenir en memoire
	if (oaDataPreparationUsedAttributes.GetSize() <= nMaxMemoryColumnNumber)
	{
		nChunkColumnNumber = oaDataPreparationUsedAttributes.GetSize();
		nChunkNumber = 1;
		nMemoryChunkNumber = 1;
	}
	// Sinon, on calcul la taille des chunks
	else
	{
		// On se base sur l'utilisation de trois chunks simultanement en memoire
		nChunkColumnNumber = nMaxMemoryColumnNumber / 3;

		// On utilise au minimum une colonne par chunk
		if (nChunkColumnNumber < 1)
			nChunkColumnNumber = 1;

		// Calcul du nombre total de chunks
		nChunkNumber = oaDataPreparationUsedAttributes.GetSize() / nChunkColumnNumber;
		if (nChunkNumber * nChunkColumnNumber < oaDataPreparationUsedAttributes.GetSize())
			nChunkNumber++;
		assert(nChunkNumber * nChunkColumnNumber >= oaDataPreparationUsedAttributes.GetSize());

		// On recalcule le nombre de colonnes par chunk pour les repartir uniformement
		nChunkColumnNumber = oaDataPreparationUsedAttributes.GetSize() / nChunkNumber;
		if (nChunkNumber * nChunkColumnNumber < oaDataPreparationUsedAttributes.GetSize())
			nChunkColumnNumber++;
		assert(nChunkNumber * nChunkColumnNumber >= oaDataPreparationUsedAttributes.GetSize());

		// Calcul du nombre de chunks simultanement en memoire
		nMemoryChunkNumber = nMaxMemoryColumnNumber / nChunkColumnNumber;
		if (nMemoryChunkNumber < 1)
			nMemoryChunkNumber = 1;
		if (nMemoryChunkNumber > nChunkNumber)
			nMemoryChunkNumber = nChunkNumber;
	}

	// Trace des resultats
	if (nChunkTraceLevel >= 1)
	{
		cout << "Used attributes\t" << oaDataPreparationUsedAttributes.GetSize() << endl;
		cout << "Max memory column number\t" << nMaxMemoryColumnNumber << endl;
		cout << "Chunk column number\t" << nChunkColumnNumber << endl;
		cout << "Chunk number\t" << nChunkNumber << endl;
		cout << "Memory chunk number\t" << nMemoryChunkNumber << endl;
	}
}

int KWDataPreparationBase::ComputeMaxMemoryColumnNumber()
{
	longint lMaxMemoryColumnNumber;
	int nMaxMemoryColumnNumber;
	longint lAvailableMemory;
	longint lColumnNecessayMemory;

	// Recherche de la taille necessaire pour une colonne
	lColumnNecessayMemory =
	    sizeof(KWDataPreparationColumn) + sizeof(IntVector) + 2 * sizeof(void*) + GetInstanceNumber() * sizeof(int);

	// On rajoute la taille du buffer ce chargement d'un chunk, un par colonne dans le pire des cas
	lColumnNecessayMemory += KWDataPreparationChunk::nFileBufferMaxSize;

	// Recherche de la memoire disponible
	// La base est deja censee etre ouverte
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Prise en compte des colonnes et des chunks, avec au pire un chunk par colonne
	lAvailableMemory -= oaDataPreparationUsedAttributes.GetSize() *
			    (sizeof(KWDataPreparationColumn) + sizeof(KWDataPreparationChunk) + 2 * sizeof(void*));

	// Il faut tenir compte de la classe cible, toujours presente en memoire
	lAvailableMemory -= lColumnNecessayMemory;

	// Test si assez de memoire
	if (lAvailableMemory < 0)
		lAvailableMemory = 0;

	// On accepte d'en utiliser au plus la moitie
	lAvailableMemory /= 2;

	// Estimation du nombre de colonnes utilisable en memoire
	// Calcul du nombre max de colonnes
	lMaxMemoryColumnNumber = lAvailableMemory / lColumnNecessayMemory;
	if (lMaxMemoryColumnNumber > INT_MAX)
		lMaxMemoryColumnNumber = INT_MAX;
	nMaxMemoryColumnNumber = (int)lMaxMemoryColumnNumber;
	return nMaxMemoryColumnNumber;
}

////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationChunk

double KWDataPreparationChunk::dFreshness = 0;

KWDataPreparationChunk::KWDataPreparationChunk()
{
	fChunkFile = NULL;
	bIsOpenedForWrite = false;
	bIsLoaded = false;
	dLoadFreshness = 0;

	// Creation du buffer de fichier
	nFileBuffer = NewIntArray(nFileBufferMaxSize);
	lFilePosition = 0;
	nFileBufferOffset = 0;
	nFileBufferSize = 0;
	bIsError = false;
}

KWDataPreparationChunk::~KWDataPreparationChunk()
{
	// Destruction du buffer du fichier
	DeleteIntArray(nFileBuffer);

	// Destruction du fichier
	RemoveChunkFile();
}

boolean KWDataPreparationChunk::LoadChunk(ObjectArray* oaFreeRecodingIndexVectors)
{
	boolean bOk;
	longint lChunkReadOffset;
	int nLine;
	int nColumn;
	int nIndex;

	require(not IsLoaded());
	require(GetChunkFileName() != "");
	require(oaFreeRecodingIndexVectors != NULL);
	require(oaFreeRecodingIndexVectors->GetSize() >= GetColumns()->GetSize());

	// Memorisation de la fraicheur de chargement
	dFreshness++;
	dLoadFreshness = dFreshness;

	// Installation de la memoire dans les colonnes
	SetChunkMemory(oaFreeRecodingIndexVectors);

	// Ouverture du fichier
	bOk = OpenInputChunkFile();

	// Parcours du fichier
	if (bOk)
	{
		// Lecture du fichier pour alimenter les index de recodage des colonnes du chunk
		lChunkReadOffset = 0;
		nLine = 0;
		nColumn = 0;
		while (not IsChunkFileEnd())
		{
			assert(nLine == lChunkReadOffset / GetColumns()->GetSize());
			assert(nColumn == lChunkReadOffset % GetColumns()->GetSize());

			// Lecture de la prochaine valeur
			nIndex = ReadChunkFileInt();

			// Mise a jour d'un index de recodage pour une ligne et une colonne donnee
			SetRecodingIndexAt(nLine, nColumn, nIndex);

			// Incrementation de l'index de la ligne et de la colonne
			nColumn++;
			if (nColumn == GetColumns()->GetSize())
			{
				nColumn = 0;
				nLine++;
			}

			// Incrementation de l'offset de lecture du fichier de chunk
			lChunkReadOffset++;
		}

		// Fermeture du fichier
		bOk = CloseChunkFile() and bOk;

		// Test si tout a ete correctement lu
		assert(GetColumns()->GetSize() > 0);
		assert(cast(KWDataPreparationColumn*, GetColumns()->GetAt(0))->GetRecodingIndexes() != NULL);
		if (not bOk or
		    lChunkReadOffset !=
			longint(GetColumns()->GetSize()) *
			    cast(KWDataPreparationColumn*, GetColumns()->GetAt(0))->GetRecodingIndexes()->GetSize())
		{
			AddError("Read error when loading a chunk in memory");
			bOk = false;
		}
	}

	return bOk;
}

void KWDataPreparationChunk::UnloadChunk(ObjectArray* oaFreeRecodingIndexVectors)
{
	require(IsLoaded());
	require(GetChunkFileName() != "");
	require(oaFreeRecodingIndexVectors != NULL);

	// Liberation de la memoire des colonnes
	UnsetChunkMemory(oaFreeRecodingIndexVectors);
}

void KWDataPreparationChunk::SetChunkMemory(ObjectArray* oaFreeRecodingIndexVectors)
{
	int nColumn;
	KWDataPreparationColumn* dataPreparationColumn;
	IntVector* ivRecodingIndexes;

	require(not IsLoaded());
	require(oaFreeRecodingIndexVectors != NULL);
	require(oaFreeRecodingIndexVectors->GetSize() >= GetColumns()->GetSize());

	// Installation de vecteurs de recodage dans les colonnes
	for (nColumn = 0; nColumn < GetColumns()->GetSize(); nColumn++)
	{
		dataPreparationColumn = cast(KWDataPreparationColumn*, GetColumns()->GetAt(nColumn));

		// Verification de l'etat initial
		check(dataPreparationColumn);
		assert(dataPreparationColumn->GetRecodingIndexes() == NULL);

		// On preleve un vecteur de recodage libre
		ivRecodingIndexes =
		    cast(IntVector*, oaFreeRecodingIndexVectors->GetAt(oaFreeRecodingIndexVectors->GetSize() - 1));
		oaFreeRecodingIndexVectors->SetSize(oaFreeRecodingIndexVectors->GetSize() - 1);

		// On l'installe dans la colonne
		dataPreparationColumn->SetRecodingIndexes(ivRecodingIndexes);
	}

	// Mise a jour du flag
	bIsLoaded = true;
}

void KWDataPreparationChunk::UnsetChunkMemory(ObjectArray* oaFreeRecodingIndexVectors)
{
	int nColumn;
	KWDataPreparationColumn* dataPreparationColumn;
	IntVector* ivRecodingIndexes;

	require(IsLoaded());
	require(oaFreeRecodingIndexVectors != NULL);

	// Installation de vecteurs de recodage dans les colonnes
	for (nColumn = 0; nColumn < GetColumns()->GetSize(); nColumn++)
	{
		dataPreparationColumn = cast(KWDataPreparationColumn*, GetColumns()->GetAt(nColumn));

		// Verification de l'etat initial
		check(dataPreparationColumn);
		assert(dataPreparationColumn->GetRecodingIndexes() != NULL);

		// On desinstalle le vecteur de recodage de la colonne
		ivRecodingIndexes = dataPreparationColumn->GetRecodingIndexes();
		dataPreparationColumn->SetRecodingIndexes(NULL);

		// On restitue le vecteur de recodage libre
		oaFreeRecodingIndexVectors->Add(ivRecodingIndexes);
	}

	// Mise a jour du flag
	bIsLoaded = false;
	ensure(oaFreeRecodingIndexVectors->GetSize() >= GetColumns()->GetSize());
}

void KWDataPreparationChunk::SetChunkFileName(const ALString& sFileName)
{
	sChunkFileName = sFileName;
}

const ALString& KWDataPreparationChunk::GetChunkFileName() const
{
	return sChunkFileName;
}

boolean KWDataPreparationChunk::OpenInputChunkFile()
{
	boolean bOk;

	require(fChunkFile == NULL);

	// Initialisation des caracteristiques du fichier
	fChunkFile = NULL;
	bIsOpenedForWrite = false;
	lFilePosition = 0;
	nFileBufferOffset = 0;
	nFileBufferSize = 0;
	bIsError = false;

	// Tentative d'ouverture du fichier en mode binaire
	bOk = FileService::OpenInputBinaryFile(GetChunkFileName(), fChunkFile);
	assert(fChunkFile != NULL or not bOk);

	// Lecture d'un premier buffer
	if (bOk)
		ReadChunkFileBuffer();

	return bOk;
}

boolean KWDataPreparationChunk::OpenOutputChunkFile()
{
	boolean bOk;

	require(fChunkFile == NULL);

	// Initialisation des caracteristiques du fichier
	fChunkFile = NULL;
	bIsOpenedForWrite = false;
	lFilePosition = 0;
	nFileBufferOffset = 0;
	nFileBufferSize = 0;
	bIsError = false;

	// Tentative d'ouverture du fichier en mode binaire
	bOk = FileService::OpenOutputBinaryFile(GetChunkFileName(), fChunkFile);
	assert(fChunkFile != NULL or not bOk);
	if (bOk)
		bIsOpenedForWrite = true;

	return bOk;
}

boolean KWDataPreparationChunk::CloseChunkFile()
{
	boolean bOk = true;

	require(fChunkFile != NULL);

	// Fermeture du fichier
	if (fChunkFile != NULL)
	{
		// On vide le dernier buffer en ecriture si necessaire
		if (bIsOpenedForWrite)
			WriteChunkFileBuffer();
		bOk = not bIsError;

		// Ecriture physique du fichier avant fermeture
		if (bOk and bIsOpenedForWrite)
			bOk = FileService::CloseOutputBinaryFile(GetChunkFileName(), fChunkFile);
		// Fermeture standard
		else
			bOk = FileService::CloseInputBinaryFile(GetChunkFileName(), fChunkFile);
	}

	// Initialisation des caracteristiques du fichier
	fChunkFile = NULL;
	bIsOpenedForWrite = false;
	lFilePosition = 0;
	nFileBufferOffset = 0;
	nFileBufferSize = 0;
	bIsError = false;
	return bOk;
}

void KWDataPreparationChunk::RemoveChunkFile()
{
	if (fChunkFile != NULL)
		CloseChunkFile();
	if (GetChunkFileName() != "")
		FileService::RemoveFile(GetChunkFileName());
}

void KWDataPreparationChunk::ReadChunkFileBuffer()
{
	boolean bOk;

	require(fChunkFile != NULL);

	// On augmente la position dans le fichier pour les caracteres non traites
	lFilePosition += nFileBufferSize - nFileBufferOffset;

	// Lecture d'un buffer
	timerRead.Start();
	nFileBufferSize = (int)fread(nFileBuffer, sizeof(int), nFileBufferMaxSize, fChunkFile);
	bOk = (ferror(fChunkFile) == 0);
	assert(nFileBufferSize == nFileBufferMaxSize or feof(fChunkFile) or not bOk);
	timerRead.Stop();
	nFileBufferOffset = 0;

	// Positionnement du flag d'erreur
	if (not bOk)
	{
		// Message si c'est la premiere erreur
		if (not bIsError)
			AddError("Physical error when reading data from file " +
				 FileService::GetLastSystemIOErrorMessage());
		bIsError = true;
	}
}

void KWDataPreparationChunk::WriteChunkFileBuffer()
{
	boolean bOk;
	int nWrittenBufferSize;

	require(fChunkFile != NULL);
	require(nFileBufferSize <= nFileBufferMaxSize);

	if (nFileBufferSize > 0)
	{
		timerWrite.Start();
		nWrittenBufferSize = (int)fwrite(nFileBuffer, sizeof(int), nFileBufferSize, fChunkFile);
		bOk = (ferror(fChunkFile) == 0);
		assert(nWrittenBufferSize == nFileBufferSize or not bOk);
		timerWrite.Stop();
		nFileBufferSize = 0;

		// Positionnement du flag d'erreur
		if (not bOk)
		{
			// Message si c'est la premiere erreur
			if (not bIsError)
				AddError("Physical error when writing data to file " +
					 FileService::GetLastSystemIOErrorMessage());
			bIsError = true;
		}
	}
}

const ALString KWDataPreparationChunk::GetClassLabel() const
{
	return "Prepared chunk data file";
}

const ALString KWDataPreparationChunk::GetObjectLabel() const
{
	return GetChunkFileName();
}

////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationColumn

KWDataPreparationColumn::KWDataPreparationColumn()
{
	dataPreparationAttribute = NULL;
	indexingAttribute = NULL;
	nColumnIndex = 0;
	ivRecodingIndexes = NULL;
	chunk = NULL;
}

KWDataPreparationColumn::~KWDataPreparationColumn()
{
	if (ivRecodingIndexes != NULL)
		delete ivRecodingIndexes;
}

void KWDataPreparationColumn::SetDataPreparationAttribute(KWDataPreparationAttribute* kwdpaDataPreparationAttribute)
{
	dataPreparationAttribute = kwdpaDataPreparationAttribute;

	// Calcul des probabilites conditionnelles des valeurs recodees
	lnSourceConditionalProbabilityTable.Initialize(0, 0, false, false);
	if (dataPreparationAttribute != NULL)
	{
		lnSourceConditionalProbabilityTable.ImportDataGridStats(
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats(), false, true);
	}
}

boolean KWDataPreparationColumn::Check() const
{
	require(dataPreparationAttribute != NULL);
	require(nColumnIndex >= 0);
	return dataPreparationAttribute->Check();
}

void KWDataPreparationColumn::Write(ostream& ost) const
{
	require(Check());

	ost << nColumnIndex << "\t" << *dataPreparationAttribute;
}
