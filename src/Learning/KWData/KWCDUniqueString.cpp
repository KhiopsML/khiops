// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWCDUniqueString.h"

///////////////////////////////////////////////////////////////////////////
// Classe KWCDUniqueString

ALString KWCDUniqueString::sEmptyString;
KWCDUniqueStringDictionary KWCDUniqueString::sdSharedUniqueStrings;

void KWCDUniqueString::Test()
{
	const int nIterNumber = 10000000;
	KWCDUniqueString sEmpty;
	KWCDUniqueString sToto;
	KWCDUniqueString sTotoBis;
	KWCDUniqueString sTest;
	int i;
	clock_t tBegin;
	clock_t tEnd;
	ALString sTmp;

	// Test du dictionnaire de UniqueString
	KWCDUniqueStringDictionary::Test();

	// Taille des classes concernees
	cout << "SYSTEM\tsizeof(KWCDUniqueString): " << sizeof(KWCDUniqueString) << endl;
	cout << "SYSTEM\tsizeof(KWCDUniqueStringData): " << sizeof(KWCDUniqueStringData) << endl << endl;

	// Test de contenu
	sToto.SetValue("Toto");
	sTotoBis.SetValue("Toto");
	cout << "sEmpty: " << sEmpty << endl;
	cout << "sToto: " << sToto << endl;
	cout << "sTotoBis: " << sTotoBis << endl;

	// Reaffectation des symbols
	sEmpty.SetValue("");
	sToto.SetValue("Toto");
	sTotoBis.SetValue(sTmp + sToto.GetValue() + "Bis");

	// Performances en affectation
	cout << "Performance test: " << 5 * nIterNumber << " affectations" << endl;
	tBegin = clock();
	for (i = 0; i < nIterNumber; i++)
	{
		sTest = sEmpty;
		sTest = sTotoBis;
		sTest.SetValue("");
		sTest = sToto;
		sTest = sTotoBis;
	}
	tEnd = clock();
	cout << "TIME\t" << (double)(tEnd - tBegin) / CLOCKS_PER_SEC << endl;

	// Reaffectation des symbols
	sEmpty.SetValue("");
	sToto.SetValue("Toto");
	sTotoBis.SetValue(sTmp + sToto.GetValue() + "Bis");
	cout << "Total unique string number: " << GetUniqueStringNumber() << endl;
	cout << "Total all unique strings used memory: " << GetAllUniqueStringsUsedMemory() << endl;
}

int KWCDUniqueString::GetUniqueStringNumber()
{
	int nUniqueStringNumber;
	nUniqueStringNumber = sdSharedUniqueStrings.GetCount();
	return nUniqueStringNumber;
}

longint KWCDUniqueString::GetAllUniqueStringsUsedMemory()
{
	longint lUsedMemory;
	lUsedMemory = sdSharedUniqueStrings.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////////////////
// Classe KWCDUniqueStringDictionary

KWCDUniqueStringDictionary::~KWCDUniqueStringDictionary()
{
	RemoveAll();
	assert(m_nCount == 0);
}

KWCDUniqueStringDictionary::KWCDUniqueStringDictionary()
{
	m_nCount = 0;
}

int KWCDUniqueStringDictionary::GetCount() const
{
	return m_nCount;
}

boolean KWCDUniqueStringDictionary::IsEmpty() const
{
	return m_nCount == 0;
}

KWCDUniqueStringDataPtr KWCDUniqueStringDictionary::AsUniqueString(const ALString& sValue)
{
	UINT nHash;
	int nHashPosition;
	KWCDUniqueStringDataPtr pUniqueStringData;

	require(sValue.GetLength() != 0);

	pUniqueStringData = GetUniqueStringDataAt(sValue, nHash);
	if (pUniqueStringData == NULL)
	{
		if (pvUniqueStringDatas.GetSize() == 0)
		{
			assert(m_nCount == 0);
			InitHashTable(DictionaryGetNextTableSize(1));
		}

		// Ajout d'un nouveau UniqueString
		pUniqueStringData = new KWCDUniqueStringData;
		pUniqueStringData->sValue = sValue;
		pUniqueStringData->nHashValue = nHash;
		m_nCount++;
		assert(m_nCount > 0);

		// Ajout dans la table de hashage
		nHashPosition = nHash % GetHashTableSize();
		pUniqueStringData->pNext = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nHashPosition);
		pvUniqueStringDatas.SetAt(nHashPosition, pUniqueStringData);

		// Retaillage dynamique
		assert(GetHashTableSize() < INT_MAX / sizeof(void*));
		if (GetCount() > GetHashTableSize() / 2)
			ReinitHashTable(DictionaryGetNextTableSize(2 * GetHashTableSize()));
	}
	return pUniqueStringData;
}

void KWCDUniqueStringDictionary::RemoveUniqueString(KWCDUniqueStringDataPtr uniqueStringData)
{
	KWCDUniqueStringDataPtr pHeadUniqueStringData;
	KWCDUniqueStringDataPtr pUniqueStringDataPrev;
	int nHashPosition;

	require(uniqueStringData != NULL);
	require(Lookup(uniqueStringData->GetValue()) == uniqueStringData);

	// Recherche du premier element de la table de hashage correspondant a la cle
	nHashPosition = uniqueStringData->nHashValue % GetHashTableSize();
	pHeadUniqueStringData = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nHashPosition);

	// Cas de la tete de liste
	if (pHeadUniqueStringData == uniqueStringData)
		pvUniqueStringDatas.SetAt(nHashPosition, pHeadUniqueStringData->pNext);
	// Cas hors tete de liste
	else
	{
		// Parcours de la liste pour rechercher l'element
		pUniqueStringDataPrev = pHeadUniqueStringData;
		while (pUniqueStringDataPrev->pNext != uniqueStringData)
			pUniqueStringDataPrev = pUniqueStringDataPrev->pNext;

		// Rechainage en derefencant l'element a supprimer
		pUniqueStringDataPrev->pNext = pUniqueStringDataPrev->pNext->pNext;
	}

	// Supression de l'element dereference de la table de hashage
	delete uniqueStringData;
	m_nCount--;
	assert(m_nCount >= 0);

	// Retaillage dynamique pour recuperer la memoire inutilisee
	assert(GetHashTableSize() < INT_MAX / sizeof(void*));
	if (GetCount() > 20 and GetCount() < GetHashTableSize() / 8)
		ReinitHashTable(DictionaryGetNextTableSize(2 * GetCount()));
}

void KWCDUniqueStringDictionary::RemoveAll()
{
	boolean bShowAllocErrorMessages = false;
	ALString sUserName;
	int nHashPosition;
	KWCDUniqueStringDataPtr pUniqueStringData;
	KWCDUniqueStringDataPtr pUniqueStringDataNext;
	int nMessageIndex;

	// Recherche des variables d'environnement
	sUserName = p_getenv("USERNAME");
	sUserName.MakeLower();

	// On ne montre les erreurs de non liberation de UniqueString qu'en mode expert
	// pour le dictionnaire global des UniqueString et en release, pour ne pas
	// avoir de reporting verbeux systematique en debug
	bShowAllocErrorMessages = bShowAllocErrorMessages and this == &(KWCDUniqueString::sdSharedUniqueStrings) and
				  (sUserName == "miib6422") and GetLearningExpertMode();
	debug(bShowAllocErrorMessages = false);

	// Nettoyage des cles de la table de hashage
	nMessageIndex = 0;
	for (nHashPosition = 0; nHashPosition < pvUniqueStringDatas.GetSize(); nHashPosition++)
	{
		for (pUniqueStringData = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nHashPosition);
		     pUniqueStringData != NULL;)
		{
			// Sauvegarde du suivant
			pUniqueStringDataNext = pUniqueStringData->pNext;

			// Affichage des informations sur le UniqueString non libere
			if (bShowAllocErrorMessages)
			{
				ShowAllocErrorMessage(pUniqueStringData, nMessageIndex);
				nMessageIndex++;
			}

			// Destruction des donnees du symbol
			delete pUniqueStringData;
			debug(m_nCount--);
			debug(assert(m_nCount >= 0));

			// Passage au suivant
			pUniqueStringData = pUniqueStringDataNext;
		}
	}

	// Affichage synthetique sur l'enseble des symboles non liberes
	if (bShowAllocErrorMessages)
	{
		if (nMessageIndex > 0)
		{
			if (GetProcessId() != 0)
				fprintf(stdout, "Process %d: ", GetProcessId());
			fprintf(stdout, "Number of UniqueString not free: %d\n", nMessageIndex);
		}
	}

	// Reinitialisation du tableau de symbole
	pvUniqueStringDatas.SetSize(0);
	m_nCount = 0;
}

POSITION KWCDUniqueStringDictionary::GetStartPosition() const
{
	return (m_nCount == 0) ? NULL : BEFORE_START_POSITION;
}

void KWCDUniqueStringDictionary::GetNextUniqueStringData(POSITION& rNextPosition,
							 KWCDUniqueStringDataPtr& uniqueStringData) const
{
	KWCDUniqueStringDataPtr pUniqueStringDataRet;
	int nBucket;
	KWCDUniqueStringDataPtr pUniqueStringDataNext;

	require(pvUniqueStringDatas.GetSize() != 0);
	require(rNextPosition != NULL);

	pUniqueStringDataRet = (KWCDUniqueStringDataPtr)rNextPosition;
	assert(pUniqueStringDataRet != NULL);

	if (pUniqueStringDataRet == (KWCDUniqueStringDataPtr)BEFORE_START_POSITION)
	{
		// Recherche du premier UniqueString
		for (nBucket = 0; nBucket < pvUniqueStringDatas.GetSize(); nBucket++)
			if ((pUniqueStringDataRet = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nBucket)) !=
			    NULL)
				break;
		assert(pUniqueStringDataRet != NULL);
	}

	// // Recherche du UniqueString suivant
	if ((pUniqueStringDataNext = pUniqueStringDataRet->pNext) == NULL)
	{
		// Passage au prochain bucket
		for (nBucket = (pUniqueStringDataRet->nHashValue % GetHashTableSize()) + 1;
		     nBucket < pvUniqueStringDatas.GetSize(); nBucket++)
			if ((pUniqueStringDataNext = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nBucket)) !=
			    NULL)
				break;
	}
	rNextPosition = (POSITION)pUniqueStringDataNext;

	// Initialisation du resultat
	uniqueStringData = pUniqueStringDataRet;
}

longint KWCDUniqueStringDictionary::GetUsedMemory() const
{
	POSITION current;
	KWCDUniqueStringDataPtr sElement;
	longint lUsedMemory;

	// Initialisation de la taille memoire utilisee
	lUsedMemory = sizeof(KWCDUniqueStringDictionary);
	lUsedMemory += pvUniqueStringDatas.GetSize() * sizeof(void*);

	// A jout de la memoire pour tous les symbols
	current = GetStartPosition();
	while (current != NULL)
	{
		GetNextUniqueStringData(current, sElement);
		lUsedMemory += sizeof(KWCDUniqueStringDataPtr) + sizeof(KWCDUniqueStringData) - sizeof(ALString) +
			       sElement->GetValue().GetUsedMemory();
	}
	return lUsedMemory;
}

void KWCDUniqueStringDictionary::Write(ostream& ost) const
{
	POSITION current;
	KWCDUniqueStringDataPtr sElement;
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	current = GetStartPosition();
	n = 0;
	while (current != NULL)
	{
		n++;
		GetNextUniqueStringData(current, sElement);
		ost << "\t" << sElement->GetValue() << "(" << sElement->nRefCount << ")"
		    << "\n";
		if (n > nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

const ALString KWCDUniqueStringDictionary::GetClassLabel() const
{
	return "Unique string dictionary";
}

void KWCDUniqueStringDictionary::Test()
{
	KWCDUniqueStringDictionary sdTest;
	KWCDUniqueStringDictionary sdPerf;
	ALString sRef;
	StringVector svArray;
	ALString sPerf;
	KWCDUniqueStringDataPtr sUniqueString;
	int nI;
	int nMaxSize;
	int nNbIter;
	clock_t tStartClock;
	clock_t tStopClock;

	// Initialisation de 11 Strings
	sRef = "UniqueString";
	for (nI = 0; nI < 11; nI++)
		svArray.Add(sRef + IntToString(nI));

	/////
	cout << "Basic test\n";
	//
	cout << "\tInsertion of 10 KWUniqueStringDataPtrs\n";
	for (nI = 0; nI < 10; nI++)
	{
		sUniqueString = sdTest.AsUniqueString(svArray.GetAt(nI));
	}

	//
	cout << "\tTest of existence\n";
	for (nI = 0; nI < 11; nI++)
	{
		cout << "\t\tLookup (" << (const char*)svArray.GetAt(nI)
		     << "): " << (sdTest.Lookup(svArray.GetAt(nI)) != NULL) << "\n";
	}

	/////
	cout << "Performance test\n";
	nMaxSize = AcquireRangedInt("Max number of inserted strings (Random)", 1, 30000, 1000);
	nNbIter = AcquireRangedInt("Iteration number", 1, 1000, 10);
	tStartClock = clock();
	for (nI = 0; nI < nNbIter; nI++)
	{
		for (int nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			sPerf = IntToString(RandomInt(nMaxSize));
			sdPerf.AsUniqueString(sPerf);
		}
		tStopClock = clock();
		cout << "TIME\tIteration " << nI << "\tSize = " << sdPerf.GetCount() << "\t"
		     << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	}

	//////////
	sdTest.RemoveAll();
	cout << "Performance test for the management of KWUniqueStringDataPtrs\n";

	nMaxSize = AcquireRangedInt("Number of KWUniqueStringData", 1, 100000, 10000);
	tStartClock = clock();
	for (nI = 0; nI < nMaxSize; nI++)
	{
		sPerf = IntToString(nI);
		sUniqueString = sdTest.AsUniqueString(sPerf);
	}
	tStopClock = clock();
	cout << "TIME\t" << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";

	nNbIter = AcquireRangedInt("Number of searchs", 1, 1000000, 100000);
	//
	cout << "\tPour un char*:\n";
	tStartClock = clock();
	sPerf = IntToString(1);
	for (nI = 0; nI < nNbIter; nI++)
	{
		sUniqueString = sdTest.AsUniqueString("1");
	}
	tStopClock = clock();
	cout << "TIME\t" << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	//
	cout << "\tPour un ALString:\n";
	tStartClock = clock();
	sPerf = IntToString(1);
	for (nI = 0; nI < nNbIter; nI++)
	{
		sUniqueString = sdTest.AsUniqueString(sPerf);
	}
	tStopClock = clock();
	cout << "TIME\t" << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	// Pour eviter un warning
	assert(sUniqueString != NULL);
}

void KWCDUniqueStringDictionary::ReinitHashTable(int nNewHashSize)
{
	boolean bDisplay = false;
	Timer timer;
	int i;
	int nHashPosition;
	KWCDUniqueStringDataPtr pUniqueStringData;
	KWCDUniqueStringDataPtr pUniqueStringDataNext;
	KWCDUniqueStringDataPtr pAllUniqueStringDatas;

	// Affichage du debut de la methode
	if (bDisplay)
	{
		cout << "ReinitHashTable (" << GetCount() << "," << GetHashTableSize() << ")";
		cout << " -> " << nNewHashSize << ": " << flush;
		timer.Start();
	}

	// Memorisation de tous les symboles dans une liste chainee
	pAllUniqueStringDatas = NULL;
	for (i = 0; i < pvUniqueStringDatas.GetSize(); i++)
	{
		// Acces au symbole courant
		pUniqueStringData = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(i);
		if (pUniqueStringData != NULL)
		{
			// On met a NULL la position correspondante
			pvUniqueStringDatas.SetAt(i, NULL);

			// On recherche le dernier symbole chaine
			pUniqueStringDataNext = pUniqueStringData;
			while (pUniqueStringDataNext->pNext != NULL)
				pUniqueStringDataNext = pUniqueStringDataNext->pNext;

			// On chaine avec la liste de tous les symboles
			pUniqueStringDataNext->pNext = pAllUniqueStringDatas;
			pAllUniqueStringDatas = pUniqueStringData;
		}
	}

	// Retaillage de la table de hashage
	pvUniqueStringDatas.SetSize(nNewHashSize);

	// On reinsere tous les symbole dans la nouvelle taille de hashage
	pUniqueStringData = pAllUniqueStringDatas;
	while (pUniqueStringData != NULL)
	{
		// Sauvegarde du suivant
		pUniqueStringDataNext = pUniqueStringData->pNext;

		// Reconversion de pUniqueStringData, vers la nouvelle table de hashage
		nHashPosition = pUniqueStringData->nHashValue % nNewHashSize;
		pUniqueStringData->pNext = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nHashPosition);
		pvUniqueStringDatas.SetAt(nHashPosition, pUniqueStringData);

		// Passage au suivant
		pUniqueStringData = pUniqueStringDataNext;
	}

	// Affichage de la fin de la methode
	if (bDisplay)
	{
		timer.Stop();
		cout << timer.GetElapsedTime() << endl;
	}
}

KWCDUniqueStringDataPtr KWCDUniqueStringDictionary::GetUniqueStringDataAt(const ALString& sValue, UINT& nHash) const
{
	KWCDUniqueStringDataPtr pUniqueStringData;
	int nHashPosition;

	require(sValue.GetLength() != 0);

	// Cas particulier d'un dictionnaire vide
	if (pvUniqueStringDatas.GetSize() == 0)
	{
		assert(m_nCount == 0);

		// Calcul de sa cle par rapport a la premiere taille de dictionnaire
		nHash = HashKey(sValue);
		return NULL;
	}

	// Calcul de la cle par rapport a la taille courante de la table de hashage
	nHash = HashKey(sValue);

	// Regarde si la cle existe
	nHashPosition = nHash % GetHashTableSize();
	for (pUniqueStringData = (KWCDUniqueStringDataPtr)pvUniqueStringDatas.GetAt(nHashPosition);
	     pUniqueStringData != NULL; pUniqueStringData = pUniqueStringData->pNext)
	{
		assert(pUniqueStringData->GetValue().GetLength() != 0);

		// Test de la valeur exacte de la string
		if (pUniqueStringData->GetValue() == sValue)
			return pUniqueStringData;
	}
	return NULL;
}

void KWCDUniqueStringDictionary::ShowAllocErrorMessage(KWCDUniqueStringDataPtr uniqueStringData, int nMessageIndex)
{
	const int nMaxMessageNumber = 20;

	require(uniqueStringData != NULL);
	require(nMessageIndex >= 0);

	if (nMessageIndex < nMaxMessageNumber)
	{
		if (GetProcessId() != 0)
			cout << "Process " << GetProcessId() << ": ";
		cout << "UniqueString not free (refCount=" << uniqueStringData->nRefCount << ", " << uniqueStringData
		     << ", " << uniqueStringData->GetValue() << ")\n";
	}
	if (nMessageIndex == nMaxMessageNumber)
		fprintf(stdout, "...\n");
}
