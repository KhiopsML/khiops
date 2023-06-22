// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWIndexedKeyBlock.h"

//////////////////////////////////////////////////////////////////
// Classe KWIndexedCKeyBlock

KWIndexedCKeyBlock::~KWIndexedCKeyBlock()
{
	nkdKeyIndexes.RemoveAll();
	oaKeyIndexes.DeleteAll();
}

int KWIndexedCKeyBlock::GetVarKeyType() const
{
	return KWType::Symbol;
}

void KWIndexedCKeyBlock::Clean()
{
	nkdKeyIndexes.RemoveAll();
	oaKeyIndexes.DeleteAll();
}

boolean KWIndexedCKeyBlock::IndexKeys()
{
	// Rien a faire dans le cas categoriel
	// Les cles sont deja dans leur dictionnaire
	return true;
}

void KWIndexedCKeyBlock::AddKey(const Symbol& sKey)
{
	KWKeyIndex* keyIndex;

	require(not sKey.IsEmpty());
	require(not IsKeyPresent(sKey));
	require(oaKeyIndexes.GetSize() == 0 or GetKeyAt(oaKeyIndexes.GetSize() - 1).CompareValue(sKey) < 0);

	// Creation d'une nouvelle paire (cle, index)
	keyIndex = new KWKeyIndex;
	keyIndex->SetKey(sKey);
	keyIndex->SetIndex(oaKeyIndexes.GetSize());

	// Ajout dans les containeurs
	oaKeyIndexes.Add(keyIndex);
	nkdKeyIndexes.SetAt(sKey.GetNumericKey(), keyIndex);
}

void KWIndexedCKeyBlock::Write(ostream& ost) const
{
	int nMax = 10;
	int i;

	ost << GetClassLabel() << " [" << GetKeyNumber() << "]:";
	for (i = 0; i < GetKeyNumber(); i++)
	{
		if (i > 0)
			cout << ", ";
		if (i < nMax)
			cout << "(" << i << ", " << GetKeyAt(i) << ")";
		else
		{
			cout << "...";
			break;
		}
	}
	cout << "\n";
}

longint KWIndexedCKeyBlock::GetUsedMemory() const
{
	return sizeof(KWIndexedCKeyBlock) + GetKeyNumber() * sizeof(KWKeyIndex) + oaKeyIndexes.GetUsedMemory() -
	       sizeof(ObjectArray) + nkdKeyIndexes.GetUsedMemory() - sizeof(NumericKeyDictionary);
}

const ALString KWIndexedCKeyBlock::GetClassLabel() const
{
	return "Indexed key block";
}

void KWIndexedCKeyBlock::Test()
{
	KWIndexedCKeyBlock indexedKeyBlock;
	IntVector ivMaxKeyNumbers;
	int nTest;
	int nTotalIter;
	int nMaxMaxKeyNumber = 100000;
	int nMaxKeyNumber;
	int nTimes;
	int nKey;
	int i;
	SymbolVector svTestKeys;
	int nKeyIndex;
	longint lTotal;
	Timer timer;
	longint lEmptySize;
	longint lSize;
	ALString sTmp;

	//////////////////////////////////////////////////////////////////////////
	// Tests de base

	// Initialisation de cles
	svTestKeys.SetSize(0);
	for (nKey = 0; nKey < 100; nKey++)
		svTestKeys.Add(Symbol(sTmp + "Key" + IntToString(nKey + 1)));

	// Initialisation avec cles consecutives
	svTestKeys.SortValues();
	indexedKeyBlock.Clean();
	for (nKey = 0; nKey < 10; nKey++)
		indexedKeyBlock.AddKey(svTestKeys.GetAt(nKey));
	indexedKeyBlock.IndexKeys();

	// Calcul de la valeur totale des index des cles presentes
	lTotal = 0;
	for (nKey = 0; nKey < 100; nKey++)
	{
		nKeyIndex = indexedKeyBlock.GetKeyIndex(svTestKeys.GetAt(nKey));
		if (nKeyIndex != -1)
			lTotal += nKeyIndex;
	}
	cout << "Consectutive keys\n";
	cout << "Sum key indexes\t" << lTotal << endl;
	cout << indexedKeyBlock << endl;

	// Calcul de la valeur totale des index des cles presentes
	lTotal = 0;
	for (nKey = 0; nKey < 100; nKey++)
	{
		nKeyIndex = indexedKeyBlock.GetKeyIndex(svTestKeys.GetAt(nKey));
		if (nKeyIndex != -1)
			lTotal += nKeyIndex;
	}
	cout << "Non consecutive keys\n";
	cout << "Sum key indexes\t" << lTotal << endl;
	cout << indexedKeyBlock << endl;

	// Nombre total d'iteration par test (plus petit en debug)
	nTotalIter = nMaxMaxKeyNumber * 100;
	debug(nTotalIter = nMaxMaxKeyNumber);

	// Initialisation de la famille de test
	nMaxKeyNumber = 1;
	while (nMaxKeyNumber <= nMaxMaxKeyNumber)
	{
		ivMaxKeyNumbers.Add(nMaxKeyNumber);
		nMaxKeyNumber *= 10;
	}

	// Parcours de tous les test
	for (nTest = 0; nTest < ivMaxKeyNumbers.GetSize(); nTest++)
	{
		nMaxKeyNumber = ivMaxKeyNumbers.GetAt(nTest);
		nTimes = nTotalIter / nMaxKeyNumber;

		// Affichage de l'entete des resultats
		if (nTest == 0)
			cout << "Key number\tEmpty size\tKey size\tCreation time\tSearch time\tIndex check\n";

		// Initialisation de cles
		svTestKeys.SetSize(0);
		for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
			svTestKeys.Add(Symbol(sTmp + "Key" + IntToString(nKey + 1)));

		// Statistique preliminaires
		svTestKeys.SortValues();
		indexedKeyBlock.Clean();
		lEmptySize = indexedKeyBlock.GetUsedMemory();
		cout << nMaxKeyNumber << "\t";
		cout << lEmptySize << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		for (i = 0; i < nTimes; i++)
		{
			indexedKeyBlock.Clean();
			for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
				indexedKeyBlock.AddKey(svTestKeys.GetAt(nKey));
			indexedKeyBlock.IndexKeys();
		}
		timer.Stop();
		lSize = indexedKeyBlock.GetUsedMemory();
		cout << int((lSize - lEmptySize) * 10 / nMaxKeyNumber) / 10.0 << "\t";
		cout << (timer.GetElapsedTime() / (nTimes * nMaxKeyNumber)) * 1000000000 << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < nTimes; i++)
		{
			lTotal = 0;
			for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
			{
				nKeyIndex = indexedKeyBlock.GetKeyIndex(svTestKeys.GetAt(nKey));
				lTotal += nKeyIndex;
				assert(indexedKeyBlock.GetKeyAt(nKeyIndex) == svTestKeys.GetAt(nKey));
			}
		}
		timer.Stop();
		cout << (timer.GetElapsedTime() / (nTimes * nMaxKeyNumber)) * 1000000000 << "\t";
		cout << lTotal << endl;
	}
}

//////////////////////////////////////////////////////////////////
// Classe KWIndexedNKeyBlock

int KWIndexedNKeyBlock::GetVarKeyType() const
{
	return KWType::Continuous;
}

void KWIndexedNKeyBlock::Clean()
{
	ivKeys.SetSize(0);
	ivKeyIndexes.SetSize(0);
}

void KWIndexedNKeyBlock::Write(ostream& ost) const
{
	int nMax = 10;
	int i;

	ost << GetClassLabel() << " [" << GetKeyNumber() << "]:";
	for (i = 0; i < GetKeyNumber(); i++)
	{
		if (i > 0)
			cout << ", ";
		if (i < nMax)
			cout << "(" << i << ", " << GetKeyAt(i) << ")";
		else
		{
			cout << "...";
			break;
		}
	}
	cout << "\n";
}

longint KWIndexedNKeyBlock::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWIndexedNKeyBlock) + ivKeys.GetUsedMemory() - sizeof(IntVector) +
		      ivKeyIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

const ALString KWIndexedNKeyBlock::GetClassLabel() const
{
	return "Indexed numerical key block";
}

void KWIndexedNKeyBlock::Test()
{
	cout << "Sorted mode: " << true << endl;
	TestWithMode(true);
	cout << "Sorted mode: " << false << endl;
	TestWithMode(false);
}

boolean KWIndexedNKeyBlock::InternalIndexKeys(boolean bAutomaticMode, boolean bSortedKeyMode)
{
	boolean bOk = true;
	int nLastKey;
	int i;
	int nKey;
	boolean bFinalySortedKeyMode;

	// Recherche de la valeur de la derniere cle
	nLastKey = 1;
	if (ivKeys.GetSize() > 0)
		nLastKey = ivKeys.GetAt(ivKeys.GetSize() - 1);

	// Determination du mode de gestion des cles en fonction de la taille
	// qu'occuperait une gestion des clee base sur un dictionnaire
	if (bAutomaticMode)
		bFinalySortedKeyMode =
		    ComputeDictionaryNecessaryMemory(ivKeys.GetSize()) < longint(nLastKey * sizeof(int));
	else
		bFinalySortedKeyMode = bSortedKeyMode;

	// Nettoyage des resultats
	ivKeyIndexes.SetSize(0);

	// Memorisation des cles presentes si on on utilise le vecteur d'index
	// Attention: on se sert de taille de vecteur des cles presentes comme indicateur du mode
	if (not bFinalySortedKeyMode or ivKeys.GetSize() == 0)
	{
		// Initialisation du vecteur avec des -1 (indique l'absence de cle)
		ivKeyIndexes.SetSize(nLastKey + 1);
		for (i = 0; i < ivKeyIndexes.GetSize(); i++)
			ivKeyIndexes.SetAt(i, -1);

		// Memorisation de l'index de chaque cle
		for (i = 0; i < ivKeys.GetSize(); i++)
		{
			nKey = ivKeys.GetAt(i);

			// memorisation de l'index de la cle
			ivKeyIndexes.SetAt(nKey, i);
		}
	}
	return bOk;
}

int KWIndexedNKeyBlock::GetDichotomicKeyIndex(int nKey) const
{
	int nLeft;
	int nRight;
	int nMiddle;
	int nValue;

	// Initialisation des index de recherche dichotomique
	nLeft = 0;
	nRight = ivKeys.GetSize() - 1;

	// Recherche dichotomique
	while (nLeft < nRight)
	{
		assert(ivKeys.GetAt(nLeft) < ivKeys.GetAt(nRight));
		nMiddle = (nLeft + nRight) / 2;
		nValue = ivKeys.GetAt(nMiddle);
		if (nValue < nKey)
			nLeft = nMiddle + 1;
		else
			nRight = nMiddle;
	}

	// On teste la cle pour l'index final
	if (nRight >= 0 and ivKeys.GetAt(nLeft) == nKey)
		return nLeft;
	else
		return -1;
}

longint KWIndexedNKeyBlock::ComputeDictionaryNecessaryMemory(int nKeyNumber) const
{
	static longint lNumericKeyDictionaryBaseMemory = 0;
	static longint lNumericKeyDictionaryMemoryPerElement = 0;
	NumericKeyDictionary* nkdKeyIndexes;
	KWKeyIndex* keyIndex;
	longint lNecessaryMemory;

	require(nKeyNumber >= 0);

	// Bufferisation des calcul des stats sur la memoire necessaire
	if (lNumericKeyDictionaryBaseMemory == 0)
	{
		// Creation des objets pour acceder a leur utilisation memoire
		nkdKeyIndexes = new NumericKeyDictionary;
		keyIndex = new KWKeyIndex;

		// Memorisation des statistiques memoire
		lNumericKeyDictionaryBaseMemory = nkdKeyIndexes->GetUsedMemory();
		lNumericKeyDictionaryMemoryPerElement =
		    nkdKeyIndexes->GetUsedMemoryPerElement() + sizeof(void*) + keyIndex->GetUsedMemory();

		// Nettoyage
		delete nkdKeyIndexes;
		delete keyIndex;
	}

	// Calcul de la memoire necessaire
	lNecessaryMemory = lNumericKeyDictionaryBaseMemory + nKeyNumber * lNumericKeyDictionaryMemoryPerElement;
	return lNecessaryMemory;
}

void KWIndexedNKeyBlock::TestWithMode(boolean bSortedKeyMode)
{
	KWIndexedNKeyBlock indexedKeyBlock;
	IntVector ivMaxKeyNumbers;
	int nTest;
	int nTotalIter;
	int nMaxMaxKeyNumber = 100000;
	int nMaxKeyNumber;
	int nTimes;
	int nKey;
	int i;
	IntVector ivTestKeys;
	int nKeyIndex;
	longint lTotal;
	Timer timer;
	longint lEmptySize;
	longint lSize;
	ALString sTmp;

	//////////////////////////////////////////////////////////////////////////
	// Tests de base

	// Initialisation avec cles consecutives
	indexedKeyBlock.Clean();
	for (nKey = 0; nKey < 10; nKey++)
		indexedKeyBlock.AddKey(nKey + 1);
	indexedKeyBlock.InternalIndexKeys(false, bSortedKeyMode);

	// Calcul de la valeur totale des index des cles presentes
	lTotal = 0;
	for (nKey = 0; nKey < 100; nKey++)
	{
		nKeyIndex = indexedKeyBlock.GetKeyIndex(nKey + 1);
		if (nKeyIndex != -1)
			lTotal += nKeyIndex;
	}
	cout << "Consectutive keys\n";
	cout << "Sum key indexes\t" << lTotal << endl;
	cout << indexedKeyBlock << endl;

	// Calcul de la valeur totale des index des cles presentes
	lTotal = 0;
	for (nKey = 0; nKey < 100; nKey++)
	{
		nKeyIndex = indexedKeyBlock.GetKeyIndex(nKey + 1);
		if (nKeyIndex != -1)
			lTotal += nKeyIndex;
	}
	cout << "Non consecutive keys\n";
	cout << "Sum key indexes\t" << lTotal << endl;
	cout << indexedKeyBlock << endl;

	//////////////////////////////////////////////////////////////////////////
	// Tests de performance

	// Nombre total d'iteration par test (plus petit en debug)
	nTotalIter = nMaxMaxKeyNumber * 100;
	debug(nTotalIter = nMaxMaxKeyNumber);

	// Initialisation de la famille de test
	nMaxKeyNumber = 1;
	while (nMaxKeyNumber <= nMaxMaxKeyNumber)
	{
		ivMaxKeyNumbers.Add(nMaxKeyNumber);
		nMaxKeyNumber *= 10;
	}

	// Parcours de tous les test
	for (nTest = 0; nTest < ivMaxKeyNumbers.GetSize(); nTest++)
	{
		nMaxKeyNumber = ivMaxKeyNumbers.GetAt(nTest);
		nTimes = nTotalIter / nMaxKeyNumber;

		// Affichage de l'entete des resultats
		if (nTest == 0)
			cout << "Key number\tEmpty size\tKey size\tCreation time\tSearch time\tIndex check\n";

		// Initialisation de cles
		ivTestKeys.SetSize(0);
		for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
			ivTestKeys.Add(nKey + 1);

		// Statistique preliminaires
		indexedKeyBlock.Clean();
		lEmptySize = indexedKeyBlock.GetUsedMemory();
		cout << nMaxKeyNumber << "\t";
		cout << lEmptySize << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		for (i = 0; i < nTimes; i++)
		{
			indexedKeyBlock.Clean();
			for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
				indexedKeyBlock.AddKey(ivTestKeys.GetAt(nKey));
			indexedKeyBlock.InternalIndexKeys(false, bSortedKeyMode);
		}
		timer.Stop();
		lSize = indexedKeyBlock.GetUsedMemory();
		cout << int((lSize - lEmptySize) * 10 / nMaxKeyNumber) / 10.0 << "\t";
		cout << (timer.GetElapsedTime() / (nTimes * nMaxKeyNumber)) * 1000000000 << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < nTimes; i++)
		{
			lTotal = 0;
			for (nKey = 0; nKey < nMaxKeyNumber; nKey++)
			{
				nKeyIndex = indexedKeyBlock.GetKeyIndex(ivTestKeys.GetAt(nKey));
				lTotal += nKeyIndex;
				assert(indexedKeyBlock.GetKeyAt(nKeyIndex) == ivTestKeys.GetAt(nKey));
			}
		}
		timer.Stop();
		cout << (timer.GetElapsedTime() / (nTimes * nMaxKeyNumber)) * 1000000000 << "\t";
		cout << lTotal << endl;
	}
}

////////////////////////////////////////////////////////////////////////
// Classe KWKeyIndex

int KWKeyIndex::GetType() const
{
	return KWType::Unknown;
}

KWKeyIndex* KWKeyIndex::Clone() const
{
	KWKeyIndex* clone;
	clone = new KWKeyIndex;
	clone->sKey = sKey;
	clone->nIndex = nIndex;
	return clone;
}

longint KWKeyIndex::GetUsedMemory() const
{
	return sizeof(KWKeyIndex);
}

void KWKeyIndex::Write(ostream& ost) const
{
	ost << '(' << sKey << ", " << nIndex << ": ";
	WriteValue(ost);
	ost << ')';
}

void KWKeyIndex::WriteValue(ostream& ost) const {}

int KWKeyIndexCompareIndex(const void* elem1, const void* elem2)
{
	// Comparaison sur le critere de tri
	return cast(KWKeyIndex*, *(Object**)elem1)->GetIndex() - cast(KWKeyIndex*, *(Object**)elem2)->GetIndex();
}

int KWKeyIndexCompareKeyValue(const void* elem1, const void* elem2)
{
	// Comparaison sur le critere de tri
	return cast(KWKeyIndex*, *(Object**)elem1)
	    ->GetKey()
	    .CompareValue(cast(KWKeyIndex*, *(Object**)elem2)->GetKey());
}

////////////////////////////////////////////////////////////////////////
// Classe KWContinuousKeyIndex

int KWContinuousKeyIndex::GetType() const
{
	return KWType::Continuous;
}

KWKeyIndex* KWContinuousKeyIndex::Clone() const
{
	KWContinuousKeyIndex* clone;
	clone = new KWContinuousKeyIndex;
	clone->sKey = sKey;
	clone->nIndex = nIndex;
	clone->cStoredValue = cStoredValue;
	return clone;
}

longint KWContinuousKeyIndex::GetUsedMemory() const
{
	return sizeof(KWContinuousKeyIndex);
}

void KWContinuousKeyIndex::WriteValue(ostream& ost) const
{
	ost << KWContinuous::ContinuousToString(cStoredValue);
}

////////////////////////////////////////////////////////////////////////
// Classe KWSymbolKeyIndex

int KWSymbolKeyIndex::GetType() const
{
	return KWType::Symbol;
}

KWKeyIndex* KWSymbolKeyIndex::Clone() const
{
	KWSymbolKeyIndex* clone;
	clone = new KWSymbolKeyIndex;
	clone->sKey = sKey;
	clone->nIndex = nIndex;
	clone->sStoredValue = sStoredValue;
	return clone;
}

longint KWSymbolKeyIndex::GetUsedMemory() const
{
	return sizeof(KWSymbolKeyIndex);
}

void KWSymbolKeyIndex::WriteValue(ostream& ost) const
{
	ost << sStoredValue;
}

////////////////////////////////////////////////////////////////////////
// Classe KWObjectArrayKeyIndex

int KWObjectArrayKeyIndex::GetType() const
{
	return KWType::ObjectArray;
}

KWKeyIndex* KWObjectArrayKeyIndex::Clone() const
{
	KWObjectArrayKeyIndex* clone;
	clone = new KWObjectArrayKeyIndex;
	clone->sKey = sKey;
	clone->nIndex = nIndex;
	clone->oaStoredValue.CopyFrom(&oaStoredValue);
	return clone;
}

longint KWObjectArrayKeyIndex::GetUsedMemory() const
{
	return sizeof(KWSymbolKeyIndex) - sizeof(ObjectArray) + oaStoredValue.GetUsedMemory();
}

void KWObjectArrayKeyIndex::WriteValue(ostream& ost) const
{
	ost << '[' << oaStoredValue.GetSize() << ']';
}