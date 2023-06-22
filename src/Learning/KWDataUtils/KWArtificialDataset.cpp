// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWArtificialDataset.h"

KWArtificialDataset::KWArtificialDataset()
{
	bHeaderLineUsed = true;
	cFieldSeparator = ';';
	nLineNumber = 1000;
	nFieldNumber = 10;
	ivKeyFieldIndexes.Add(0);
	bAscendingSort = true;
	nMaxLineNumberPerKey = 1;
	dSamplingRate = 1;
	SetFileName(BuildFileName());
}

KWArtificialDataset::~KWArtificialDataset() {}

void KWArtificialDataset::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

const ALString& KWArtificialDataset::GetFileName() const
{
	return sFileName;
}

void KWArtificialDataset::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean KWArtificialDataset::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

void KWArtificialDataset::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

char KWArtificialDataset::GetFieldSeparator() const
{
	return cFieldSeparator;
}

void KWArtificialDataset::SetLineNumber(int nValue)
{
	require(nValue >= 0);
	nLineNumber = nValue;
}

int KWArtificialDataset::GetLineNumber() const
{
	return nLineNumber;
}

void KWArtificialDataset::SetFieldNumber(int nValue)
{
	require(nValue >= 0);
	nFieldNumber = nValue;
}

int KWArtificialDataset::GetFieldNumber() const
{
	return nFieldNumber;
}

IntVector* KWArtificialDataset::GetKeyFieldIndexes()
{
	return &ivKeyFieldIndexes;
}

const IntVector* KWArtificialDataset::GetConstKeyFieldIndexes() const
{
	return &ivKeyFieldIndexes;
}

void KWArtificialDataset::SetAscendingSort(boolean bValue)
{
	bAscendingSort = bValue;
}

boolean KWArtificialDataset::GetAscendingSort() const
{
	return bAscendingSort;
}

void KWArtificialDataset::SetMaxLineNumberPerKey(int nValue)
{
	require(nValue >= 1);
	nMaxLineNumberPerKey = nValue;
}

int KWArtificialDataset::GetMaxLineNumberPerKey() const
{
	return nMaxLineNumberPerKey;
}

void KWArtificialDataset::SetSamplingRate(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dSamplingRate = dValue;
}

double KWArtificialDataset::GetSamplingRate() const
{
	return dSamplingRate;
}

KWArtificialDataset* KWArtificialDataset::Clone() const
{
	KWArtificialDataset* cloneDataset;

	cloneDataset = new KWArtificialDataset;
	cloneDataset->CopyFrom(this);
	return cloneDataset;
}

void KWArtificialDataset::CopyFrom(const KWArtificialDataset* sourceDataset)
{
	require(sourceDataset != NULL);

	bHeaderLineUsed = sourceDataset->bHeaderLineUsed;
	cFieldSeparator = sourceDataset->cFieldSeparator;
	nLineNumber = sourceDataset->nLineNumber;
	nFieldNumber = sourceDataset->nFieldNumber;
	ivKeyFieldIndexes.CopyFrom(&(sourceDataset->ivKeyFieldIndexes));
	bAscendingSort = sourceDataset->bAscendingSort;
	nMaxLineNumberPerKey = sourceDataset->nMaxLineNumberPerKey;
	dSamplingRate = sourceDataset->dSamplingRate;
	sFileName = sourceDataset->sFileName;
}

const ALString KWArtificialDataset::BuildIdentifier() const
{
	ALString sIdentifier;
	ALString sFilePathName;
	ALString sTmp;

	// Calcul d'un identifiant de test a partir de ses parametres
	sIdentifier = sTmp + "Test_" + IntToString(nLineNumber) + "_" + IntToString(nFieldNumber) + "_" +
		      BooleanToString(bHeaderLineUsed);
	if (ivKeyFieldIndexes.GetSize() > 0)
	{
		sIdentifier += "_K";
		if (not bAscendingSort)
			sIdentifier += "d";
		sIdentifier += IntToString(ivKeyFieldIndexes.GetSize());
		if (nMaxLineNumberPerKey > 1)
			sIdentifier += sTmp + "(" + IntToString(nMaxLineNumberPerKey) + ")";
		if (dSamplingRate < 1)
			sIdentifier += sTmp + "_" + IntToString(int(100 * dSamplingRate)) + "%";
	}
	return sIdentifier;
}

const ALString KWArtificialDataset::BuildFileName() const
{
	ALString sIdentifier;
	ALString sFilePathName;

	// Calcul d'un identifiant de test a partir de ses parametres
	sIdentifier = BuildIdentifier();

	// Creation du nom fichier d'entree
	sFilePathName = FileService::BuildFilePathName(FileService::GetTmpDir(), sIdentifier + ".txt");
	return sFilePathName;
}

void KWArtificialDataset::SpecifySortDataset()
{
	SetLineNumber(100000);
	ivKeyFieldIndexes.SetSize(0);
	ivKeyFieldIndexes.Add(2);
	ivKeyFieldIndexes.Add(0);
	ivKeyFieldIndexes.Add(5);
	SetAscendingSort(false);
	SetFileName(BuildFileName());
}

void KWArtificialDataset::SpecifySortedDataset()
{
	SetLineNumber(100000);
	ivKeyFieldIndexes.SetSize(0);
	ivKeyFieldIndexes.Add(2);
	ivKeyFieldIndexes.Add(0);
	ivKeyFieldIndexes.Add(5);
	SetFileName(BuildFileName());
}

void KWArtificialDataset::CreateDataset() const
{
	OutputBufferedFile outputFile;
	int nField;
	int nLine;
	int nKey;
	int nRootBaseKey;
	int nBaseKey;
	StringVector svFieldValues;
	int nKeyValue;
	ALString sKeyValue;
	boolean bOk;
	ALString sTmp;

	require(sFileName != "");
	require(ivKeyFieldIndexes.GetSize() <= nFieldNumber);
	require(ivKeyFieldIndexes.GetSize() < 9);

	// Suivi de la tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Create artificial dataset");

	// Verification des champs de la cle
	for (nKey = 0; nKey < ivKeyFieldIndexes.GetSize(); nKey++)
	{
		assert(0 <= ivKeyFieldIndexes.GetAt(nKey));
		assert(ivKeyFieldIndexes.GetAt(nKey) < nFieldNumber);
	}

	// Creation du fichier d'entree
	outputFile.SetFileName(sFileName);
	bOk = outputFile.Open();
	if (bOk)
	{
		// Creation de la ligne d'entete
		svFieldValues.SetSize(nFieldNumber);
		if (bHeaderLineUsed)
		{
			// Creation des noms des champs
			for (nField = 0; nField < nFieldNumber; nField++)
				svFieldValues.SetAt(nField, sTmp + "Field" + IntToString(nField + 1));

			// Si necessaire, remplacement par des noms de cle
			for (nKey = 0; nKey < ivKeyFieldIndexes.GetSize(); nKey++)
				svFieldValues.SetAt(ivKeyFieldIndexes.GetAt(nKey),
						    sTmp + "Key" + IntToString(nKey + 1));

			// Ecriture de l'entete
			for (nField = 0; nField < nFieldNumber; nField++)
			{
				if (nField > 0)
					outputFile.Write(cFieldSeparator);
				outputFile.Write(svFieldValues.GetAt(nField));
			}
			outputFile.WriteEOL();
		}

		// On calcule la valeur de base de la cle, de facon a ce que les cles de poids forts evoluent de facon
		// monotone
		nRootBaseKey = 1;
		nBaseKey = nRootBaseKey;
		for (nKey = 0; nKey < ivKeyFieldIndexes.GetSize(); nKey++)
			nBaseKey *= 10;
		while (nMaxLineNumberPerKey * nBaseKey / 10 < nLineNumber)
		{
			nBaseKey *= 10;
			nRootBaseKey *= 10;
		}
		assert(nMaxLineNumberPerKey * nRootBaseKey * pow(10.0, 1.0 * ivKeyFieldIndexes.GetSize()) / 10 >=
		       nLineNumber);

		// Creation des lignes
		for (nLine = 0; nLine < nLineNumber; nLine++)
		{
			// On ne cree pas la ligne en cas d'echantillonnage du fichier
			if (dSamplingRate < 1 and IthRandomDouble(nLine) > dSamplingRate)
				continue;

			// Creation des valeur des champs
			for (nField = 0; nField < nFieldNumber; nField++)
				svFieldValues.SetAt(nField, sTmp + "v" + IntToString(nLine + 1) + "_" +
								IntToString(nField + 1));

			// On alimente d'abord les cles de poids faible, par puissance de 10
			nBaseKey = nRootBaseKey;
			for (nKey = ivKeyFieldIndexes.GetSize() - 1; nKey >= 0; nKey--)
			{
				// On cree les cle selon l'ordre souhaite
				// On ne change de cle que tous les nMaxLineNumberPerKey
				if (bAscendingSort)
					nKeyValue = (nLine / nMaxLineNumberPerKey) % nBaseKey;
				else
					nKeyValue = nBaseKey - 1 - ((nLine / nMaxLineNumberPerKey) % nBaseKey);

				// Les cles de poids fort sont incrementees plus lentement que les cle de poids faible
				nKeyValue /= (nBaseKey / nRootBaseKey);
				nKeyValue *= (nBaseKey / nRootBaseKey);

				// Memorisation de la cle (on maintenant en categoriel son ordre numerique)
				sKeyValue = sTmp + IntToString(nBaseKey + nKeyValue);
				sKeyValue.SetAt(0, 'k');
				svFieldValues.SetAt(ivKeyFieldIndexes.GetAt(nKey), sKeyValue);
				nBaseKey *= 10;
			}

			// Ecriture de la ligne
			for (nField = 0; nField < nFieldNumber; nField++)
			{
				if (nField > 0)
					outputFile.Write(cFieldSeparator);
				outputFile.Write(svFieldValues.GetAt(nField));
			}
			outputFile.WriteEOL();

			// Gestion de l'avancement
			if (nLine % 1000 == 0)
			{
				TaskProgression::DisplayProgression(int(nLine * 100.0 / nLineNumber));
				if (TaskProgression::IsInterruptionRequested())
					break;
			}
		}

		// Fermeture du fichier
		outputFile.Close();
	}

	// Suivi de la tache
	TaskProgression::EndTask();
}

void KWArtificialDataset::ExportNativeFieldNames(StringVector* svNativeFieldNames) const
{
	int nField;
	int nKey;
	ALString sTmp;

	require(svNativeFieldNames != NULL);

	// Creation des noms des champs
	svNativeFieldNames->SetSize(nFieldNumber);
	for (nField = 0; nField < nFieldNumber; nField++)
		svNativeFieldNames->SetAt(nField, sTmp + "Field" + IntToString(nField + 1));

	// Si necessaire, remplacement par des noms de cle
	for (nKey = 0; nKey < ivKeyFieldIndexes.GetSize(); nKey++)
		svNativeFieldNames->SetAt(ivKeyFieldIndexes.GetAt(nKey), sTmp + "Key" + IntToString(nKey + 1));
}

void KWArtificialDataset::ExportKeyAttributeNames(StringVector* svKeyAttributedNames) const
{
	int nKey;
	ALString sTmp;

	require(svKeyAttributedNames != NULL);

	// Creation des noms des champs de la cle
	svKeyAttributedNames->SetSize(ivKeyFieldIndexes.GetSize());
	for (nKey = 0; nKey < ivKeyFieldIndexes.GetSize(); nKey++)
		svKeyAttributedNames->SetAt(nKey, sTmp + "Key" + IntToString(nKey + 1));
}

void KWArtificialDataset::DeleteDataset() const
{
	if (sFileName != "")
		FileService::RemoveFile(sFileName);
}

void KWArtificialDataset::DisplayFirstLines(int nDisplayLineNumber) const
{
	DisplayFileFirstLines(sFileName, nDisplayLineNumber);
}

void KWArtificialDataset::DisplayFileFirstLines(const ALString& sFilePathName, int nDisplayLineNumber)
{
	fstream fst;
	int nLine;
	const int nTmpBufferSize = 100000;
	char* sTmpBuffer;
	ALString sLastLine;

	require(nDisplayLineNumber >= 0);

	// Affichage du nom du fichier
	cout << "File " << FileService::GetFileName(sFilePathName);

	// Test si fichier manquant
	if (not FileService::Exist(sFilePathName))
		cout << " : missing file";
	else
		cout << " (size=" << FileService::GetFileSize(sFilePathName) << ")";
	cout << endl;

	// Lecture des premieres lignes du fichier de cle
	if (FileService::Exist(sFilePathName))
	{
		sTmpBuffer = NewCharArray(nTmpBufferSize);
		FileService::OpenInputFile(sFilePathName, fst);
		if (fst.is_open())
		{
			nLine = 0;
			sLastLine = "";
			while (not fst.eof())
			{
				fst.getline(sTmpBuffer, nTmpBufferSize);
				nLine++;

				// Affichage des ligne demandees
				if (nLine <= nDisplayLineNumber)
				{
					// Affichage sauf si fin de fichier et ligne vide
					if (not fst.eof() or sTmpBuffer[0] != '\0')
						cout << "  " << nLine << ": " << sTmpBuffer << endl;
				}

				// Affichage des dernieres ligne
				if (fst.eof() and nLine > nDisplayLineNumber)
				{
					// Avant derniere ligne si derniere ligne vide
					if (sTmpBuffer[0] == '\0')
					{
						if (nLine - 1 > nDisplayLineNumber + 1)
							cout << "  " << nDisplayLineNumber + 1 << ": ..." << endl;
						cout << "  " << nLine - 1 << ": " << sLastLine << endl;
					}
					// Derniere ligne sinon
					else
					{
						if (nLine > nDisplayLineNumber + 1)
							cout << "  " << nDisplayLineNumber + 1 << ": ..." << endl;
						cout << "  " << nLine << ": " << sTmpBuffer << endl;
					}
				}

				// Memorisation de la ligne precedente
				sLastLine = sTmpBuffer;
			}
			FileService::CloseInputFile(sFilePathName, fst);
		}
		DeleteCharArray(sTmpBuffer);
	}
}

boolean KWArtificialDataset::AddLinesInFile(const ALString& sFileName, const ALString& sDestFileName,
					    const ALString& sLineToCopy, IntVector* ivLines)
{
	InputBufferedFile inputFile;
	OutputBufferedFile outputFile;
	int nLineIndex;
	int nLineIndexToInsert;
	int index;
	boolean bOk;
	longint lBeginPos;
	CharVector cvLine;

	require(FileService::Exist(sFileName));
	require(ivLines != NULL);
	require(ivLines->GetSize() != 0);

	// Ouverture en lecture du fichier d'entree
	inputFile.SetFileName(sFileName);
	bOk = inputFile.Open();
	if (not bOk)
	{
		Global::AddError("", "", "unable to open " + sFileName);
		return false;
	}
	// Ouverture en ecriture du fichier de sortie
	outputFile.SetFileName(sDestFileName);
	bOk = outputFile.Open();

	if (bOk)
	{
		ivLines->Sort();

		// Lecture du fichier d'entree ligne par ligne
		nLineIndex = 0;
		index = 0;
		nLineIndexToInsert = ivLines->GetAt(index);
		lBeginPos = 0;
		while (not inputFile.IsFileEnd())
		{
			// Lecture d'un buffer
			inputFile.Fill(lBeginPos);
			lBeginPos += inputFile.GetBufferSize();
			while (not inputFile.IsBufferEnd())
			{
				inputFile.GetNextLine(&cvLine);

				// Insertion si on a numero de ligne recherche
				if (nLineIndexToInsert == nLineIndex)
				{
					outputFile.Write(sLineToCopy);
					index++;

					// Recherche du numero de ligne a inserer suivant
					if (index < ivLines->GetSize())
						nLineIndexToInsert = ivLines->GetAt(index);
					else
						nLineIndexToInsert = -1;
				}
				nLineIndex++;

				// Traitement standard
				outputFile.Write(&cvLine);
			}
		}

		outputFile.Close();
	}
	else
	{
		Global::AddError("", "", "unable to open " + sDestFileName);
	}
	inputFile.Close();
	assert(FileService::Exist(sDestFileName));
	return bOk;
}

void KWArtificialDataset::Test()
{
	KWArtificialDataset artificialDataset;

	artificialDataset.SetFileName(artificialDataset.BuildFileName());
	artificialDataset.CreateDataset();
	artificialDataset.DisplayFirstLines(10);
	artificialDataset.DeleteDataset();
}