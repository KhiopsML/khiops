// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKeyExtractor.h"

///////////////////////////////////
// Implementation de KWKeyExtractor

KWKeyExtractor::KWKeyExtractor()
{
	nLastStartPos = 0;
	iBuffer = NULL;
	oaKeyFieldsOrderedByFile.SetCompareFunction(KWFieldIndexCompare);
}

KWKeyExtractor::~KWKeyExtractor()
{
	oaKeyFieldsOrderedByKey.DeleteAll();
}

void KWKeyExtractor::SetBufferedFile(InputBufferedFile* bufferedFile)
{
	require(bufferedFile != NULL);
	iBuffer = bufferedFile;
}

InputBufferedFile* KWKeyExtractor::GetBufferedFile() const
{
	return iBuffer;
}

void KWKeyExtractor::SetKeyFieldIndexes(const IntVector* ivIndexes)
{
	int nField;
	KWFieldIndex* fieldIndex;

	ivKeyFieldIndexes.CopyFrom(ivIndexes);

	//////////////////////////////////////////////////////////////////////////////
	// Calcul d'une indexation des champs de la cle selon l'odre dans le fichier

	// Nettoyage initial
	oaKeyFieldsOrderedByKey.DeleteAll();

	// Creation de champs cles
	oaKeyFieldsOrderedByKey.SetSize(ivKeyFieldIndexes.GetSize());
	for (nField = 0; nField < oaKeyFieldsOrderedByKey.GetSize(); nField++)
	{
		fieldIndex = new KWFieldIndex;
		fieldIndex->SetIndex(ivKeyFieldIndexes.GetAt(nField));
		oaKeyFieldsOrderedByKey.SetAt(nField, fieldIndex);
	}

	// Recopie puis tri
	oaKeyFieldsOrderedByFile.CopyFrom(&oaKeyFieldsOrderedByKey);
	oaKeyFieldsOrderedByFile.Sort();
}

const IntVector* KWKeyExtractor::GetConstKeyFieldIndexes() const
{
	return &ivKeyFieldIndexes;
}

void KWKeyExtractor::Clean()
{
	nLastStartPos = 0;
	iBuffer = NULL;
	oaKeyFieldsOrderedByKey.DeleteAll();
	oaKeyFieldsOrderedByFile.SetSize(0);
}

void KWKeyExtractor::ParseNextKey(PLParallelTask* taskErrorSender)
{
	int i;
	int j;
	int nKeyIndex;
	ALString sSubKey;
	char* sField;
	int nErrNo;
	boolean bEol;
	KWFieldIndex* fieldIndex;
	int nLineFieldNumber;
	ALString sDisplayedField;
	ALString sTmp;

	require(iBuffer != NULL);
	require(iBuffer->IsOpened());

	// Initialisations
	j = 0;
	bEol = false;
	nLastStartPos = iBuffer->GetPositionInBuffer();

	// Parcours d'une ligne pour extraire la clef
	// les indices sont donnes dans le bon ordre par oaKeyFieldsOrderedByFile
	nLineFieldNumber = 0;
	for (i = 0; i < oaKeyFieldsOrderedByFile.GetSize(); i++)
	{
		// Index du champ dans le fichier
		fieldIndex = cast(KWFieldIndex*, oaKeyFieldsOrderedByFile.GetAt(i));
		nKeyIndex = fieldIndex->GetIndex();

		// Saut des separateurs jusqu'au bon champ
		while (j < nKeyIndex and not bEol)
		{
			bEol = iBuffer->SkipField();
			nLineFieldNumber++;
			j++;
		}

		// Extraction du champ (par defaut vide si on a deja lu tous les champs de la ligne)
		sField = NULL;
		if (not bEol)
		{
			bEol = iBuffer->GetNextField(sField, nErrNo);
			nLineFieldNumber++;
			j++;
			if (nErrNo != InputBufferedFile::FieldNoError and taskErrorSender != NULL)
			{
				// On tronque le champ s'il est trop long
				sDisplayedField = sField;
				if (sDisplayedField.GetLength() > 20)
					sDisplayedField = sDisplayedField.Left(20) + "...";

				// Affichage du warning
				taskErrorSender->AddLocalWarning("key field <" + sDisplayedField +
								     "> : " + iBuffer->GetFieldErrorLabel(nErrNo),
								 1 + iBuffer->GetCurrentLineNumber());
			}
		}
		// Warning s'il manque des champ pour alimenter la cle
		else
		{
			if (taskErrorSender != NULL)
			{
				// Affichage du warning
				taskErrorSender->AddLocalWarning(sTmp + "key field " + IntToString(i + 1) +
								     " empty because line contains only " +
								     IntToString(nLineFieldNumber) + " fields",
								 1 + iBuffer->GetCurrentLineNumber());
			}
		}

		// Affectation du champ
		if (sField != NULL)
			fieldIndex->SetField(sField);
		else
			fieldIndex->SetField("");
	}

	// On va jusqu'au bout de la ligne
	if (not bEol)
		iBuffer->SkipLine();
}

void KWKeyExtractor::ExtractKey(KWKey* key) const
{
	int i;
	KWFieldIndex* fieldIndex;

	require(key != NULL);

	// Parcours de oaKeyFieldsOrderedByKey pour avoir les champs dans l'ordre de la clef
	key->SetSize(oaKeyFieldsOrderedByKey.GetSize());
	for (i = 0; i < oaKeyFieldsOrderedByKey.GetSize(); i++)
	{
		fieldIndex = cast(KWFieldIndex*, oaKeyFieldsOrderedByKey.GetAt(i));
		key->SetAt(i, fieldIndex->GetField());
	}
}

void KWKeyExtractor::ExtractLine(int& nBeginPos, int& nEndPos) const
{
	check(iBuffer);
	require(iBuffer->IsOpened());

	nBeginPos = nLastStartPos;
	nEndPos = iBuffer->GetPositionInBuffer();

	ensure(nBeginPos < nEndPos);
}

const ALString KWKeyExtractor::GetClassLabel() const
{
	return "Key extractor";
}

const ALString KWKeyExtractor::GetObjectLabel() const
{
	if (iBuffer == NULL)
		return "";
	else
		return iBuffer->GetFileName();
}

////////////////////////////////////////////////////////////
// Implementation de la classe KWKeyFieldsIndexer

KWKeyFieldsIndexer::KWKeyFieldsIndexer() {}

KWKeyFieldsIndexer::~KWKeyFieldsIndexer() {}

StringVector* KWKeyFieldsIndexer::GetKeyAttributeNames()
{
	return &svKeyAttributeNames;
}

StringVector* KWKeyFieldsIndexer::GetNativeFieldNames()
{
	return &svNativeFieldNames;
}

boolean KWKeyFieldsIndexer::ComputeKeyFieldIndexes(boolean bHeaderLineUsed, const StringVector* svHeaderLineFieldNames)
{
	boolean bOk = true;
	const StringVector* svSeachedFieldNames;
	ALString sKeyAttributeName;
	ALString sField;
	int i;
	int nKeyIndex;
	ALString sTmp;

	require(svKeyAttributeNames.GetSize() > 0);
	require(svNativeFieldNames.GetSize() >= svKeyAttributeNames.GetSize());
	require(svHeaderLineFieldNames != NULL);

	// Si pas de ligne d'entete, le nombre d'attributs natif doit etre egal au nombre d'attribut natifs
	if (bOk and not bHeaderLineUsed and svHeaderLineFieldNames->GetSize() != svNativeFieldNames.GetSize())
	{
		bOk = false;
		AddError(sTmp + "Mismatch between the " + IntToString(svNativeFieldNames.GetSize()) +
			 " expected native variables and the " + IntToString(svHeaderLineFieldNames->GetSize()) +
			 " fields found in the first line of the database file");
	}

	// Calcul des index des champs de la cle
	ivKeyFieldIndexes.SetSize(0);
	if (bOk)
	{
		// Recherche des champs de la cle soit parmi les attribut natifs, soit dans la ligne d'entete
		if (bHeaderLineUsed)
		{
			svSeachedFieldNames = svHeaderLineFieldNames;
		}
		else
		{
			svSeachedFieldNames = &svNativeFieldNames;
		}

		// Parcours des champs de la cle pour recherche leur index
		for (i = 0; i < svKeyAttributeNames.GetSize(); i++)
		{
			sKeyAttributeName = svKeyAttributeNames.GetAt(i);

			// Parcours des champs du fichier pour trouver l'emplacement de la cle
			for (nKeyIndex = 0; nKeyIndex < svSeachedFieldNames->GetSize(); nKeyIndex++)
			{
				sField = svSeachedFieldNames->GetAt(nKeyIndex);
				if (sField == sKeyAttributeName)
				{
					ivKeyFieldIndexes.Add(nKeyIndex);
					break;
				}
			}

			// Erreur si champ de la cle non trouve (index non rajoute)
			if (ivKeyFieldIndexes.GetSize() != i + 1)
			{
				if (bHeaderLineUsed)
					AddError(sTmp + "Key field " + sKeyAttributeName + " not found in header line");
				else
					AddError(sTmp + "Key field " + sKeyAttributeName +
						 " not found among native variables");
				bOk = false;
				break;
			}
		}
	}

	// Nettoyage si necessaire
	if (ivKeyFieldIndexes.GetSize() != svKeyAttributeNames.GetSize())
		ivKeyFieldIndexes.SetSize(0);
	return bOk;
}

const IntVector* KWKeyFieldsIndexer::GetConstKeyFieldIndexes() const
{
	return &ivKeyFieldIndexes;
}

void KWKeyFieldsIndexer::Write(ostream& ost) const
{
	int i;
	ost << "Native Attributes : ";
	for (i = 0; i < svNativeFieldNames.GetSize(); i++)
		ost << svNativeFieldNames.GetAt(i) << ";";
	ost << endl << "Keys Attributes : ";
	for (i = 0; i < svKeyAttributeNames.GetSize(); i++)
		ost << svKeyAttributeNames.GetAt(i) << ";";
}

const ALString KWKeyFieldsIndexer::GetClassLabel() const
{
	return "Key field indexer";
}

const ALString KWKeyFieldsIndexer::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////
// Implementation de KWFieldIndex

KWFieldIndex::KWFieldIndex()
{
	nIndex = -1;
}

KWFieldIndex::~KWFieldIndex() {}

int KWFieldIndexCompare(const void* elem1, const void* elem2)
{
	KWFieldIndex* fi1;
	KWFieldIndex* fi2;

	// Acces aux objets
	fi1 = cast(KWFieldIndex*, *(Object**)elem1);
	fi2 = cast(KWFieldIndex*, *(Object**)elem2);

	if (fi1->GetIndex() > fi2->GetIndex())
		return 1;
	if (fi1->GetIndex() < fi2->GetIndex())
		return -1;
	return 0;
}