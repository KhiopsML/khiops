// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHStreamBining.h"

MHStreamBining::MHStreamBining()
{
	nMaxBinNumber = 0;
	bFloatingPointGrid = false;
	bSaturatedBins = false;
	workingBin = NULL;

	// Parametrage des listes triees
	slBins.SetCompareFunction(MHBinMergeComparePotentiallyIncludedBin);
	slBinMerges.SetCompareFunction(MHBinMergeCompareBinMerge);
}

MHStreamBining::~MHStreamBining()
{
	assert(slBins.GetCount() == 0);
	assert(slBinMerges.GetCount() == 0);
	assert(workingBin == NULL);
}

void MHStreamBining::SetMaxBinNumber(int nValue)
{
	require(nValue >= 0);
	require(not IsStreamInitialized());
	nMaxBinNumber = nValue;
}

int MHStreamBining::GetMaxBinNumber() const
{
	return nMaxBinNumber;
}

void MHStreamBining::SetFloatingPointGrid(boolean bValue)
{
	require(not IsStreamInitialized());
	bFloatingPointGrid = bValue;
}

boolean MHStreamBining::GetFloatingPointGrid() const
{
	return bFloatingPointGrid;
}

void MHStreamBining::SetSaturatedBins(boolean bValue)
{
	require(not IsStreamInitialized());
	bSaturatedBins = bValue;
}

boolean MHStreamBining::GetSaturatedBins() const
{
	return bSaturatedBins;
}

void MHStreamBining::InitializeStream()
{
	require(not IsStreamInitialized());
	require(GetMaxBinNumber() >= 1);

	// Initialisation du bin global de stats
	globalStatsBin.Reset();
	globalStatsBin.SetLowerValue(KWContinuous::GetMaxValue());
	globalStatsBin.SetUpperValue(KWContinuous::GetMinValue());

	// Initialisation du bin de travail
	workingBin = new MHBinMerge;
}

boolean MHStreamBining::IsStreamInitialized() const
{
	return workingBin != NULL;
}

void MHStreamBining::AddStreamValue(Continuous cValue)
{
	POSITION currentBinPosition;
	POSITION prevBinPosition;
	POSITION nextBinPosition;
	POSITION nextNextBinPosition;
	MHBinMerge* currentBin;
	MHBinMerge* prevBin;
	MHBinMerge* nextBin;
	MHBinMerge* nextNextBin;
	POSITION bestBinMergePosition;
	MHBinMerge* bestBinMerge;
	boolean bNewBin;

	require(IsStreamInitialized());
	require(workingBin != NULL);
	require(globalStatsBin.GetFrequency() % GetMaxBinNumber() != 0 or CheckAllBins());
	require(globalStatsBin.GetFrequency() % GetMaxBinNumber() != 0 or CheckAllBinMerges());

	// Mise a jour des stats globales
	globalStatsBin.SetLowerValue(min(globalStatsBin.GetLowerValue(), cValue));
	globalStatsBin.SetUpperValue(max(globalStatsBin.GetUpperValue(), cValue));
	globalStatsBin.SetFrequency(globalStatsBin.GetFrequency() + 1);

	// Arret si un seul bin
	if (GetMaxBinNumber() == 1)
		return;

	// Parametrage du bin de travail avec la nouvelle valeur
	debug(workingBin->Reset());
	workingBin->SetLowerValue(cValue);
	workingBin->SetUpperValue(cValue);
	workingBin->SetFrequency(1);

	// Recherche dans la liste de la position d'un bin contenant la valeur
	currentBinPosition = slBins.Find(workingBin);

	// Insertion du bin si sa position est non trouvee
	bNewBin = (currentBinPosition == NULL);
	if (bNewBin)
	{
		currentBin = workingBin;
		currentBinPosition = slBins.Add(currentBin);

		// Le bin de travail n'est plus disponible
		workingBin = NULL;
	}
	// Mise a jour de son effectif sinon
	else
	{
		currentBin = cast(MHBinMerge*, slBins.GetAt(currentBinPosition));
		currentBin->SetFrequency(currentBin->GetFrequency() + 1);
	}
	assert(slBins.Find(currentBin) == currentBinPosition);

	// Recherche du bin precedent
	prevBinPosition = currentBinPosition;
	slBins.GetPrev(prevBinPosition);
	if (prevBinPosition == NULL)
		prevBin = NULL;
	else
		prevBin = cast(MHBinMerge*, slBins.GetAt(prevBinPosition));

	// Recherche du bin suivant
	nextBinPosition = currentBinPosition;
	slBins.GetNext(nextBinPosition);
	if (nextBinPosition == NULL)
		nextBin = NULL;
	else
		nextBin = cast(MHBinMerge*, slBins.GetAt(nextBinPosition));

	// Mise a jour des fusions si un nouveau bin a ete cree
	if (bNewBin)
	{
		// Supression de la fusion initiale entre les bins precedent et suivant
		if (prevBin != NULL and nextBin != NULL)
		{
			assert(slBinMerges.GetAt(prevBin->GetMergePosition()) == prevBin);
			slBinMerges.RemoveAt(prevBin->GetMergePosition());
			debug(prevBin->SetMergePosition(NULL));
		}

		// Ajout d'une nouvelle fusion entre le bin precedent et le bin courant
		if (prevBin != NULL)
		{
			prevBin->UpdateMergeCriteria(currentBin, bFloatingPointGrid, bSaturatedBins);
			prevBin->SetMergePosition(slBinMerges.Add(prevBin));
		}

		// Ajout d'une nouvelle fusion entre le bin courant et le bin suivant
		if (nextBin != NULL)
		{
			currentBin->UpdateMergeCriteria(nextBin, bFloatingPointGrid, bSaturatedBins);
			currentBin->SetMergePosition(slBinMerges.Add(currentBin));
		}
	}
	// Mise a jour des fusions si un bin existant a ete mis a jour
	else
	{
		// Dans le cas standard, le critere de vraisemblance de la fusion change necessairement
		// a cause de l'incrementation de l'effectif du bin, et il faut mettre la liste des fusions a jour
		// Dans le cas, seule la granularite est a prendre en compte, et comme elle ne change pas
		// si l'effectif seul du bin a change, ce n'est pas la peine de mettre a jour la liste des fusions
		if (not bSaturatedBins)
		{
			// Mise a jour de la fusion precedente
			if (prevBin != NULL)
			{
				// Suppression prealable de la fusion, le critere de tri ayant change
				assert(slBinMerges.GetAt(prevBin->GetMergePosition()) == prevBin);
				slBinMerges.RemoveAt(prevBin->GetMergePosition());
				debug(prevBin->SetMergePosition(NULL));

				// Re-insertion de la fusion dans la liste
				prevBin->UpdateMergeCriteria(currentBin, bFloatingPointGrid, bSaturatedBins);
				prevBin->SetMergePosition(slBinMerges.Add(prevBin));
			}

			// Mise a jour de la fusion suivante
			if (nextBin != NULL)
			{
				// Suppression prealable de la fusion, le critere de tri ayant change
				assert(slBinMerges.GetAt(currentBin->GetMergePosition()) == currentBin);
				slBinMerges.RemoveAt(currentBin->GetMergePosition());
				debug(currentBin->SetMergePosition(NULL));

				// Re-insertion de la fusion dans la liste
				currentBin->UpdateMergeCriteria(nextBin, bFloatingPointGrid, bSaturatedBins);
				currentBin->SetMergePosition(slBinMerges.Add(currentBin));
			}
		}
	}

	// Cas ou la liste des bins depasse sa capacite
	if (slBins.GetCount() > GetMaxBinNumber())
	{
		assert(slBinMerges.GetCount() > 0);

		// Recherche de la meilleure fusion
		bestBinMergePosition = slBinMerges.GetHeadPosition();
		bestBinMerge = cast(MHBinMerge*, slBinMerges.GetAt(bestBinMergePosition));
		assert(bestBinMerge != NULL);

		// Le bin portant la fusion est le meme
		// (un BinMerge contient les informations sur un bin et la fusion avec le bin suivant)
		currentBin = bestBinMerge;
		currentBinPosition = slBins.Find(currentBin);
		assert(currentBinPosition != NULL);

		// Recherche du bin precedent
		prevBinPosition = currentBinPosition;
		slBins.GetPrev(prevBinPosition);
		if (prevBinPosition == NULL)
			prevBin = NULL;
		else
			prevBin = cast(MHBinMerge*, slBins.GetAt(prevBinPosition));

		// Recherche du bin suivant
		nextBinPosition = currentBinPosition;
		slBins.GetNext(nextBinPosition);
		assert(nextBinPosition != NULL);
		nextBin = cast(MHBinMerge*, slBins.GetAt(nextBinPosition));
		assert(nextBin != NULL);

		// Recherche du bin suivant du suivant
		nextNextBinPosition = nextBinPosition;
		slBins.GetNext(nextNextBinPosition);
		if (nextNextBinPosition == NULL)
			nextNextBin = NULL;
		else
			nextNextBin = cast(MHBinMerge*, slBins.GetAt(nextNextBinPosition));

		// Suppression des trois fusions initiales autour de la fusion a effectuer
		if (prevBin != NULL)
		{
			assert(slBinMerges.GetAt(prevBin->GetMergePosition()) == prevBin);
			slBinMerges.RemoveAt(prevBin->GetMergePosition());
			debug(prevBin->SetMergePosition(NULL));
		}
		assert(currentBin != NULL);
		assert(slBinMerges.GetAt(currentBin->GetMergePosition()) == currentBin);
		slBinMerges.RemoveAt(currentBin->GetMergePosition());
		debug(currentBin->SetMergePosition(NULL));
		if (nextNextBin != NULL)
		{
			assert(slBinMerges.GetAt(nextBin->GetMergePosition()) == nextBin);
			slBinMerges.RemoveAt(nextBin->GetMergePosition());
			debug(nextBin->SetMergePosition(NULL));
		}

		// Supression du bin suivant de la liste des bins
		slBins.RemoveAt(nextBinPosition);

		// Mise a jour du bin fusionne
		assert(currentBin->GetUpperValue() < nextBin->GetLowerValue());
		currentBin->SetUpperValue(nextBin->GetUpperValue());
		currentBin->SetFrequency(currentBin->GetFrequency() + nextBin->GetFrequency());

		// Le bin courant devrait rester en meme position, son critere de tri n'ayant pas d'impact
		assert(slBins.Find(currentBin) == currentBinPosition);

		// Insertion de la nouvelle fusion avant et apres le nouveau bin
		if (prevBin != NULL)
		{
			prevBin->UpdateMergeCriteria(currentBin, bFloatingPointGrid, bSaturatedBins);
			prevBin->SetMergePosition(slBinMerges.Add(prevBin));
		}
		if (nextNextBin != NULL)
		{
			currentBin->UpdateMergeCriteria(nextNextBin, bFloatingPointGrid, bSaturatedBins);
			currentBin->SetMergePosition(slBinMerges.Add(currentBin));
		}

		// On recupere le bin supprime comme bin de travail
		assert(workingBin == NULL);
		workingBin = nextBin;
		assert(workingBin != NULL);
	}

	// Creation si necessaire d'un nouveau bin de travail
	if (workingBin == NULL)
		workingBin = new MHBinMerge;
	ensure(slBins.GetCount() <= GetMaxBinNumber());
	ensure(slBinMerges.GetCount() == slBins.GetCount() - 1);
	ensure(CheckStreamValue(cValue));
}

void MHStreamBining::FinalizeStream()
{
	require(IsStreamInitialized());

	// Re-initialisation du bin global de stats
	globalStatsBin.Reset();

	// Destruction des bins
	slBins.DeleteAll();
	slBinMerges.RemoveAll();

	// Destruction du bin de travail
	delete workingBin;
	workingBin = NULL;
}

Continuous MHStreamBining::GetStreamLowerValue() const
{
	return globalStatsBin.GetLowerValue();
}

Continuous MHStreamBining::GetStreamUpperValue() const
{
	return globalStatsBin.GetUpperValue();
}

int MHStreamBining::GetStreamFrequency() const
{
	return globalStatsBin.GetFrequency();
}

void MHStreamBining::ExportStreamBins(ObjectArray* oaStreamBins) const
{
	if (GetSaturatedBins())
		ExportStreamSaturatedBins(oaStreamBins);
	else
		ExportStreamStandardBins(oaStreamBins);
	ensure(GetBinArrayTotalFrequency(oaStreamBins) == GetStreamFrequency());
}

boolean MHStreamBining::WriteStreamBinsFile(const ALString& sOutputFileName) const
{
	boolean bOk;
	fstream fOutputFile;
	ObjectArray oaStreamBins;
	MHBin* bin;
	int i;

	require(IsStreamInitialized());

	// Creation du repertoire en sortie si necessaire
	if (FileService::GetPathName(sOutputFileName) != "")
		FileService::MakeDirectories(FileService::GetPathName(sOutputFileName));

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sOutputFileName, fOutputFile);
	if (bOk)
	{
		// Exports des bins dans un tableau
		ExportStreamBins(&oaStreamBins);

		// Ecriture dans le fichier
		for (i = 0; i < oaStreamBins.GetSize(); i++)
		{
			bin = cast(MHBin*, oaStreamBins.GetAt(i));
			fOutputFile << *bin << '\n';
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sOutputFileName, fOutputFile);

		// Nettoyage
		oaStreamBins.DeleteAll();
	}
	return bOk;
}

boolean MHStreamBining::WriteStreamBinMergesFile(const ALString& sOutputFileName) const
{
	boolean bOk;
	fstream fOutputFile;
	POSITION position;
	MHBinMerge* binMerge;

	require(IsStreamInitialized());

	// Creation du repertoire en sortie si necessaire
	if (FileService::GetPathName(sOutputFileName) != "")
		FileService::MakeDirectories(FileService::GetPathName(sOutputFileName));

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sOutputFileName, fOutputFile);
	if (bOk)
	{
		// Entete
		globalStatsBin.WriteMergeHeaderLine(fOutputFile);
		fOutputFile << '\t';
		globalStatsBin.WriteHeaderLine(fOutputFile);
		fOutputFile << '\n';

		// Parcours des bins pour les ecrire
		position = slBinMerges.GetHeadPosition();
		while (position != NULL)
		{
			binMerge = cast(MHBinMerge*, slBinMerges.GetNext(position));
			binMerge->WriteMerge(fOutputFile);
			fOutputFile << '\t';
			binMerge->Write(fOutputFile);
			fOutputFile << '\n';
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sOutputFileName, fOutputFile);
	}
	return bOk;
}

boolean MHStreamBining::WriteStreamBinsAndMergesFile(const ALString& sOutputFileName) const
{
	boolean bOk;
	fstream fOutputFile;
	POSITION position;
	MHBinMerge* bin;

	require(IsStreamInitialized());

	// Creation du repertoire en sortie si necessaire
	if (FileService::GetPathName(sOutputFileName) != "")
		FileService::MakeDirectories(FileService::GetPathName(sOutputFileName));

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sOutputFileName, fOutputFile);
	if (bOk)
	{
		// Entete
		globalStatsBin.WriteHeaderLine(fOutputFile);
		fOutputFile << '\t';
		globalStatsBin.WriteMergeHeaderLine(fOutputFile);
		fOutputFile << '\n';

		// Cas particulier d'un seul bin
		if (GetMaxBinNumber() == 1)
		{
			if (globalStatsBin.GetFrequency() > 0)
				fOutputFile << globalStatsBin << '\n';
		}
		// Cas general
		else
		{
			// Parcours des bins pour les ecrire
			position = slBins.GetHeadPosition();
			while (position != NULL)
			{
				// Information sur le binb
				bin = cast(MHBinMerge*, slBins.GetNext(position));
				fOutputFile << *bin << '\t';

				// Informations sur le merge
				if (bin->GetMergePosition() != NULL)
				{
					bin->WriteMerge(fOutputFile);
				}
				fOutputFile << '\n';
			}
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sOutputFileName, fOutputFile);
	}
	return bOk;
}

boolean MHStreamBining::ComputeBins(const ALString& sInputDataFileName, const ALString& sOutputBinFileName)
{
	const int nMaxRecordNumber = INT_MAX;
	boolean bOk = true;
	Continuous cValue;
	InputBufferedFile inputFile;
	longint lFilePos;
	longint lRecordIndex;
	boolean bEndOfLine;
	ALString sTmp;

	require(sInputDataFileName != "");
	require(sOutputBinFileName != "");

	// Initialisation du fichier a lire
	inputFile.SetFileName(sInputDataFileName);
	inputFile.SetHeaderLineUsed(false);
	inputFile.SetBufferSize(InputBufferedFile::nDefaultBufferSize),

	    // Initialisation du stream
	    InitializeStream();

	// Lecture de la base
	lRecordIndex = 0;
	bOk = inputFile.Open();
	if (bOk)
	{
		lFilePos = 0;
		while (bOk and lFilePos < inputFile.GetFileSize())
		{
			// Remplissage du buffer
			bOk = inputFile.FillInnerLines(lFilePos);
			if (not bOk)
				AddInputFileError(&inputFile, lRecordIndex, "Error while reading file");

			// Erreur si ligne trop longue
			if (bOk)
			{
				bOk = inputFile.GetCurrentBufferSize() > 0;
				if (not bOk)
					AddInputFileError(&inputFile, lRecordIndex + 1,
							  InputBufferedFile::GetLineTooLongErrorLabel());
			}

			// Traitement du buffer
			while (bOk and not inputFile.IsBufferEnd())
			{
				// Lecture du champs de la ligne
				lRecordIndex++;

				// Warning si trop de valeurs
				if (lRecordIndex > nMaxRecordNumber)
				{
					AddInputFileWarning(&inputFile, lRecordIndex,
							    sTmp + "total value number too large (" +
								LongintToString(lRecordIndex) + ")");
					break;
				}

				// Lecture d'une valeur
				bOk = ReadValue(&inputFile, lRecordIndex, cValue, bEndOfLine);

				// Erreur si trop de champs
				if (not bEndOfLine)
				{
					AddInputFileError(&inputFile, lRecordIndex, "too many fields in line");
					bOk = false;
				}

				// Prise en compte de la valeur
				if (bOk)
					AddStreamValue(cValue);
			}
			lFilePos += inputFile.GetBufferSize();

			// Arret si trop de valeurs
			if (lRecordIndex > nMaxRecordNumber)
				break;
		}
		inputFile.Close();
	}

	// Ecriture du resultat
	if (bOk)
		WriteStreamBinsFile(sOutputBinFileName);

	// Finalisation du stream
	FinalizeStream();
	return bOk;
}

const ALString MHStreamBining::GetClassLabel() const
{
	return "streambining";
}

const ALString MHStreamBining::GetObjectLabel() const
{
	return "";
}

void MHStreamBining::ExportStreamStandardBins(ObjectArray* oaStreamBins) const
{
	POSITION position;
	MHBin* bin;

	require(IsStreamInitialized());
	require(oaStreamBins != NULL);
	require(oaStreamBins->GetSize() == 0);

	// Cas particulier d'au plus un seul bin
	if (slBins.GetCount() <= 1)
	{
		if (globalStatsBin.GetFrequency() > 0)
			oaStreamBins->Add(globalStatsBin.Clone());
	}
	// Cas general
	else
	{
		// Parcours des bins pour les memoriser
		position = slBins.GetHeadPosition();
		while (position != NULL)
		{
			bin = cast(MHBin*, slBins.GetNext(position));
			oaStreamBins->Add(bin->Clone());
		}
	}
	ensure(GetBinArrayTotalFrequency(oaStreamBins) == GetStreamFrequency());
}

void MHStreamBining::ExportStreamSaturatedBins(ObjectArray* oaStreamBins) const
{
	int nStreamBinsGranularity;
	int nBinMergeGranularity;
	POSITION position;
	MHBinMerge* binMerge;
	MHBinMerge saturatedBin;

	require(IsStreamInitialized());
	require(oaStreamBins != NULL);
	require(oaStreamBins->GetSize() == 0);

	// Cas particulier d'au plus un seul bin
	if (slBins.GetCount() <= 1)
	{
		if (globalStatsBin.GetFrequency() > 0)
			oaStreamBins->Add(globalStatsBin.Clone());
	}
	// Cas general
	else
	{
		// Calcul de la granularite du resume en bins
		nStreamBinsGranularity = ComputeStreamBinsGranularity();

		// Initialisation du bin sature de travail avec le premier bin de la liste
		position = slBins.GetHeadPosition();
		binMerge = cast(MHBinMerge*, slBins.GetNext(position));
		saturatedBin.CopyFrom(binMerge);

		// Parcours des bins restant pour obtenir un resume sature
		assert(position != NULL);
		while (position != NULL)
		{
			binMerge = cast(MHBinMerge*, slBins.GetNext(position));

			// Calcul de lagranularite de la fusion entre le bin sature en cours et le bin courant
			nBinMergeGranularity = binMerge->FindSmallestGridGranularity(
			    saturatedBin.GetLowerValue(), binMerge->GetUpperValue(), GetFloatingPointGrid());

			// Fusion si on ne depasse pas la granularite max
			if (nBinMergeGranularity <= nStreamBinsGranularity)
			{
				saturatedBin.SetUpperValue(binMerge->GetUpperValue());
				saturatedBin.SetFrequency(saturatedBin.GetFrequency() + binMerge->GetFrequency());
			}
			// Sinon, on memorise le bin sature
			else
			{
				oaStreamBins->Add(saturatedBin.Clone());

				// Re-intialisation pour la suite
				saturatedBin.CopyFrom(binMerge);
			}

			// Cas particulier du dernier bin
			if (position == NULL)
				oaStreamBins->Add(saturatedBin.Clone());
		}
	}
	ensure(GetBinArrayTotalFrequency(oaStreamBins) == GetStreamFrequency());
}

int MHStreamBining::ComputeStreamBinsGranularity() const
{
	int nStreamBinsGranularity;
	int nBinGranularity;
	POSITION position;
	MHBinMerge* binMerge;

	require(IsStreamInitialized());

	// Parcours des bins pour les ecrire
	nStreamBinsGranularity = INT_MIN;
	position = slBins.GetHeadPosition();
	while (position != NULL)
	{
		binMerge = cast(MHBinMerge*, slBins.GetNext(position));
		if (not binMerge->IsSingular())
		{
			nBinGranularity = binMerge->FindSmallestGridGranularity(
			    binMerge->GetLowerValue(), binMerge->GetUpperValue(), GetFloatingPointGrid());
			nStreamBinsGranularity = max(nStreamBinsGranularity, nBinGranularity);
		}
	}
	return nStreamBinsGranularity;
}

int MHStreamBining::GetBinArrayTotalFrequency(ObjectArray* oaStreamBins) const
{
	int nTotalFrequency;
	MHBin* bin;
	int i;

	require(oaStreamBins != NULL);

	// Cumul des effectif sur l'ensemble du tableau
	nTotalFrequency = 0;
	for (i = 0; i < oaStreamBins->GetSize(); i++)
	{
		bin = cast(MHBin*, oaStreamBins->GetAt(i));
		nTotalFrequency += bin->GetFrequency();
	}
	return nTotalFrequency;
}

boolean MHStreamBining::CheckStreamValue(Continuous cValue) const
{
	MHBinMerge checkBin;
	POSITION currentBinPosition;
	POSITION prevBinPosition;
	POSITION nextBinPosition;
	MHBinMerge* currentBin;
	MHBinMerge* prevBin;
	MHBinMerge* nextBin;
	POSITION bestBinMergePosition;
	POSITION bestNextBinMergePosition;
	MHBinMerge* bestBinMerge;
	MHBinMerge* bestNextBinMerge;

	require(IsStreamInitialized());
	require(workingBin != NULL);

	// Parametrage du bin a verifier avec la nouvelle valeur
	checkBin.SetLowerValue(cValue);
	checkBin.SetUpperValue(cValue);
	checkBin.SetFrequency(1);

	// Recherche dans la liste de la position d'un bin contenant la valeur
	currentBinPosition = slBins.Find(&checkBin);
	assert(currentBinPosition != NULL);

	// Recherche du bin
	currentBin = cast(MHBinMerge*, slBins.GetAt(currentBinPosition));
	assert(slBins.Find(currentBin) == currentBinPosition);

	// Verification du bin
	assert(currentBin->Check());
	assert(currentBin->GetLowerValue() <= cValue and cValue <= currentBin->GetUpperValue());
	assert(currentBin->GetFrequency() >= 1);

	// Recherche du bin precedent
	prevBinPosition = currentBinPosition;
	slBins.GetPrev(prevBinPosition);
	if (prevBinPosition == NULL)
		prevBin = NULL;
	else
		prevBin = cast(MHBinMerge*, slBins.GetAt(prevBinPosition));

	// Verification du bin precedent
	assert(prevBin != NULL or currentBin == slBins.GetHead());
	if (prevBin != NULL)
	{
		assert(prevBin->Check());
		assert(prevBin->GetUpperValue() < currentBin->GetLowerValue());
		assert(prevBin->GetFrequency() >= 1);
	}

	// Recherche du bin suivant
	nextBinPosition = currentBinPosition;
	slBins.GetNext(nextBinPosition);
	if (nextBinPosition == NULL)
		nextBin = NULL;
	else
		nextBin = cast(MHBinMerge*, slBins.GetAt(nextBinPosition));

	// Verification du bin suivant
	assert(nextBin != NULL or currentBin == slBins.GetTail());
	if (nextBin != NULL)
	{
		assert(nextBin->Check());
		assert(currentBin->GetUpperValue() < nextBin->GetLowerValue());
		assert(nextBin->GetFrequency() >= 1);
	}

	// Verification de la fusion du bin precedent avec le bin courant
	if (prevBin != NULL)
	{
		assert(prevBin == slBinMerges.GetAt(prevBin->GetMergePosition()));
		assert(prevBin->CheckMergeCriteria(currentBin, bFloatingPointGrid, bSaturatedBins));
	}

	// Verification de la fusion du bin courant avec le bin suivant
	if (nextBin != NULL)
	{
		assert(currentBin == slBinMerges.GetAt(currentBin->GetMergePosition()));
		assert(currentBin->CheckMergeCriteria(nextBin, bFloatingPointGrid, bSaturatedBins));
	}

	// Recherche de la meilleure fusion
	bestBinMergePosition = slBinMerges.GetHeadPosition();
	if (bestBinMergePosition == NULL)
		bestBinMerge = NULL;
	else
		bestBinMerge = cast(MHBinMerge*, slBinMerges.GetAt(bestBinMergePosition));

	// Recherche de la meilleure fusion suivante pour verification
	if (bestBinMerge != NULL)
	{
		bestNextBinMergePosition = bestBinMergePosition;
		slBinMerges.GetNext(bestNextBinMergePosition);
		if (bestNextBinMergePosition == NULL)
			bestNextBinMerge = NULL;
		else
			bestNextBinMerge = cast(MHBinMerge*, slBinMerges.GetAt(bestNextBinMergePosition));

		// Verification de la meilleure fusion et de la suivante
		if (bestNextBinMerge != NULL)
		{
			assert(bestBinMerge->CompareBinMerge(bestNextBinMerge) <= 0);
		}
	}
	return true;
}

boolean MHStreamBining::CheckAllBins() const
{
	POSITION position;
	MHBinMerge* prevBin;
	MHBinMerge* bin;

	require(IsStreamInitialized());

	// Parcours des bins pour les verifier
	prevBin = NULL;
	position = slBins.GetHeadPosition();
	while (position != NULL)
	{
		bin = cast(MHBinMerge*, slBins.GetNext(position));

		// Verification
		assert(bin->Check());
		assert(prevBin == NULL or prevBin->GetUpperValue() < bin->GetLowerValue());
		assert(prevBin == NULL or prevBin->CheckMergeCriteria(bin, bFloatingPointGrid, bSaturatedBins));
		assert(prevBin == NULL or prevBin->CompareBin(bin) <= 0);

		// Memorisation du bin precdent pour l'iteration suivante
		prevBin = bin;
	}
	return true;
}

boolean MHStreamBining::CheckAllBinMerges() const
{
	POSITION position;
	MHBinMerge* prevBinMerge;
	MHBinMerge* binMerge;

	require(IsStreamInitialized());

	// Parcours des bins pour les verifier
	prevBinMerge = NULL;
	position = slBinMerges.GetHeadPosition();
	while (position != NULL)
	{
		binMerge = cast(MHBinMerge*, slBinMerges.GetNext(position));

		// Verification
		assert(binMerge->Check());
		assert(prevBinMerge == NULL or prevBinMerge->CompareBinMerge(binMerge) <= 0);

		// Memorisation de la fusion precedente pour l'iteration suivante
		prevBinMerge = binMerge;
	}
	return true;
}

boolean MHStreamBining::ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue,
				  boolean& bEndOfLine)
{
	boolean bOk = true;
	char* sField;
	int nFieldLength;
	int nError;
	boolean bLineTooLong;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);

	// Lecture du champ suivant
	bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nError, bLineTooLong);

	// Test si champ vide
	if (sField[0] == '\0')
	{
		AddInputFileError(inputFile, lRecordIndex, "empty field instead of value");
		bOk = false;
	}

	// Test si erreur de parsing
	if (bOk and nError != InputBufferedFile::FieldNoError)
	{
		AddInputFileError(inputFile, lRecordIndex,
				  "invalid value <" + InputBufferedFile::GetDisplayValue(sField) +
				      "> : " + inputFile->GetFieldErrorLabel(nError));
		bOk = false;
	}

	// Test si ligne trop longue
	if (bOk and bLineTooLong)
	{
		AddInputFileError(inputFile, lRecordIndex, InputBufferedFile::GetLineTooLongErrorLabel());
		bOk = false;
	}

	// Parsing si pas d'erreur
	if (bOk)
	{
		// Transformation de l'eventuel separateur decimal ',' en '.'
		KWContinuous::PreprocessString(sField);

		// Conversion en Continuous
		nError = KWContinuous::StringToContinuousError(sField, cValue);

		// Test de validite du champs
		if (bOk and nError != 0)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  "invalid value <" + InputBufferedFile::GetDisplayValue(sField) + "> (" +
					      KWContinuous::ErrorLabel(nError) + ")");
			bOk = false;
		}
	}
	if (not bOk)
		cValue = KWContinuous::GetMissingValue();
	return bOk;
}

void MHStreamBining::AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddWarning(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddWarning(sLabel);
}

void MHStreamBining::AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddError(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddError(sLabel);
}
