// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Object.h"

void ObjectDictionary::FreeAssoc(ODAssoc* pAssoc)
{
	DeleteCharArray(pAssoc->key);
	pAssoc->key = NULL;

	pAssoc->pNext = pFreeList;
	pFreeList = pAssoc;
	nCount--;
	assert(nCount >= 0);
}

// Tableau des tailles d'allocations des dictionnaires
static const int nDictionaryPrimeSizes[] = {17,       37,       79,        163,       331,      673,      1361,
					    2729,     5471,     10949,     21911,     43853,    87719,    175447,
					    350899,   701819,   1403641,   2807303,   5614657,  11229331, 22458671,
					    44917381, 89834777, 179669557, 359339171, 718678369};
static const int nDictionaryPrimeSizeNumber = sizeof(nDictionaryPrimeSizes) / sizeof(int);

// Rend la taille de table superieure ou egale a une taille donnee
int DictionaryGetNextTableSize(int nSize)
{
	int i;
	for (i = 0; i < nDictionaryPrimeSizeNumber; i++)
	{
		if (nDictionaryPrimeSizes[i] >= nSize)
			return nDictionaryPrimeSizes[i];
	}
	return 2 * nSize + 1;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

CPlex* CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
	assert(nMax > 0 and cbElement > 0);
	CPlex* p = (CPlex*)NewMemoryBlock(sizeof(CPlex) + nMax * cbElement);
	p->nMax = nMax;
	p->nCur = 0;
	p->pNext = pHead;
	// Change la tete (ajouts en ordre inverse pour des raisons de simplicite)
	pHead = p;
	return p;
}

void CPlex::FreeDataChain(CPlex* pToDelete)
{
	while (pToDelete != NULL)
	{
		char* bytes = (char*)pToDelete;
		CPlex* pNext = pToDelete->pNext;
		DeleteMemoryBlock(bytes);
		pToDelete = pNext;
	}
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

ObjectDictionary::ObjectDictionary()
{
	assert(nBlockSize > 0);

	nCount = 0;
	pFreeList = NULL;
	pBlocks = NULL;
}

void ObjectDictionary::InitHashTable(int nHashSize)
{
	assert(nCount == 0);
	assert(nHashSize > 0);

	pvODAssocs.SetSize(nHashSize);
}

void ObjectDictionary::ReinitHashTable(int nNewHashSize)
{
	PointerVector pvNewODAssocs;
	int i;
	int nHash;
	ODAssoc* pAssoc;
	ODAssoc* pAssocNext;

	// Creation d'une nouvelle table de hashage
	pvNewODAssocs.SetSize(nNewHashSize);

	// Recalcul des caracteristiques des elements par rapport a la nouvelle taille de la table de hashage
	for (i = 0; i < pvODAssocs.GetSize(); i++)
	{
		for (pAssoc = (ODAssoc*)pvODAssocs.GetAt(i); pAssoc != NULL;)
		{
			// Sauvegarde du suivant
			pAssocNext = pAssoc->pNext;

			// Reconversion de pAssoc, vers la nouvelle table de hashage
			nHash = HashKey(pAssoc->key) % nNewHashSize;
			pAssoc->nHashValue = nHash;
			pAssoc->pNext = (ODAssoc*)pvNewODAssocs.GetAt(nHash);
			pvNewODAssocs.SetAt(nHash, pAssoc);

			// Passage au suivant
			pAssoc = pAssocNext;
		}
	}

	// Echange du contenu entre l'ancienne et la nouvelle table de hashage
	// Le contenu de la nouvelle table de hashage courante est transferer dans la table courante
	// Le contenu de l'ancienne table de hashage, transfere dans la variable locale pvNewODAssocs, est detruit
	pvODAssocs.SwapFrom(&pvNewODAssocs);
}

void ObjectDictionary::RemoveAll()
{
	int nHash;
	ODAssoc* pAssoc;

	// Nettoyage des cles de la table de hashage
	for (nHash = 0; nHash < pvODAssocs.GetSize(); nHash++)
	{
		for (pAssoc = (ODAssoc*)pvODAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
		{
			assert(pAssoc->key != NULL);
			DeleteCharArray(pAssoc->key);
			pAssoc->key = NULL;
		}
	}

	// Reinitialisation des variables
	pvODAssocs.SetSize(0);
	nCount = 0;
	pFreeList = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

void ObjectDictionary::DeleteAll()
{
	int nHash;
	ODAssoc* pAssoc;

	// Destruction des elements de la table de hashage
	for (nHash = 0; nHash < pvODAssocs.GetSize(); nHash++)
	{
		for (pAssoc = (ODAssoc*)pvODAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
		{
			DeleteCharArray(pAssoc->key);
			pAssoc->key = NULL;
			if (pAssoc->value != NULL)
				delete pAssoc->value;
		}
	}

	// Reinitialisation des variables
	pvODAssocs.SetSize(0);
	nCount = 0;
	pFreeList = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

ObjectDictionary::~ObjectDictionary()
{
	RemoveAll();
	assert(nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Gestion des associations
// Comme pour l'implementation des ObjectList, sauf que l'on gere
// ici une liste simplement chainee

UINT ObjectDictionary::HashKey(const char* key) const
{
	return HashValue(key);
}

ODAssoc* ObjectDictionary::GetAssocAt(const char* key, UINT& nHash) const
{
	ODAssoc* pAssoc;

	require(key != NULL);

	// Cas particulier d'un dictionnaire vide
	if (pvODAssocs.GetSize() == 0)
	{
		assert(nCount == 0);

		// Calcul de sa cle par rapport a la premiere taille de dictionnaire
		nHash = HashKey(key) % DictionaryGetNextTableSize(1);
		return NULL;
	}

	// Calcul de la cle par rapport a la taille courante de la table de hashage
	nHash = HashKey(key) % pvODAssocs.GetSize();

	// Recherche si elle existe
	for (pAssoc = (ODAssoc*)pvODAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		assert(pAssoc->key != NULL);
		if (strcmp(pAssoc->key, key) == 0)
			return pAssoc;
	}
	return NULL;
}

ODAssoc* ObjectDictionary::NewAssoc()
{
	if (pFreeList == NULL)
	{
		// Ajout d'un nouveau bloc
		CPlex* newBlock = CPlex::Create(pBlocks, nBlockSize, sizeof(ODAssoc));
		// Chainage dans la liste des bloc libres
		ODAssoc* pAssoc = (ODAssoc*)newBlock->data();
		// Chainage en ordre inverse pour faciliter le debuggage
		pAssoc += nBlockSize - 1;
		for (int i = nBlockSize - 1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = pFreeList;
			pFreeList = pAssoc;
		}
	}
	assert(pFreeList != NULL);

	ODAssoc* pAssoc = pFreeList;
	pFreeList = pFreeList->pNext;
	nCount++;
	assert(nCount > 0);
	pAssoc->key = NULL;
	pAssoc->value = NULL;

	return pAssoc;
}

/////////////////////////////////////////////////////////////////////////////

Object*& ObjectDictionary::operator[](const char* key)
{
	UINT nHash;
	ODAssoc* pAssoc;
	int nLength;

	require(key != NULL);

	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (pvODAssocs.GetSize() == 0)
		{
			assert(nCount == 0);
			InitHashTable(DictionaryGetNextTableSize(1));
		}

		// Cree une nouvelle Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		if (pAssoc->key != NULL)
			DeleteCharArray(pAssoc->key);
		assert(strlen(key) <= INT_MAX);
		nLength = (int)strlen(key);
		pAssoc->key = NewCharArray(nLength + 1);
		pAssoc->key[nLength] = '\0';
		memcpy(pAssoc->key, key, nLength);

		// Ajout dans la table de hashage
		pAssoc->pNext = (ODAssoc*)pvODAssocs.GetAt(nHash);
		pvODAssocs.SetAt(nHash, pAssoc);

		// Retaillage dynamique
		assert(GetHashTableSize() < INT_MAX / 4);
		if (GetCount() > GetHashTableSize())
			ReinitHashTable(DictionaryGetNextTableSize(2 * GetHashTableSize()));
	}
	return pAssoc->value;
}

boolean ObjectDictionary::RemoveKey(const char* key)
{
	ODAssoc* pHeadAssoc;
	ODAssoc* pAssocPrev;
	ODAssoc* pAssoc;
	int nHash;

	require(key != NULL);

	// Cas d'une table vide: rien a faire
	if (pvODAssocs.GetSize() == 0)
		return false;

	// Recherche du premier element de la table de hashage correspondant a la cle
	nHash = HashKey(key) % pvODAssocs.GetSize();
	pHeadAssoc = (ODAssoc*)pvODAssocs.GetAt(nHash);

	// Parcours de la liste chainee des elements pour supprimer l'element correspondant exactement a la cle
	pAssocPrev = NULL;
	for (pAssoc = pHeadAssoc; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		// Supression si on a trouve la cle
		if (strcmp(pAssoc->key, key) == 0)
		{
			assert(pAssoc == pHeadAssoc or pAssocPrev != NULL);
			// Supression de l'element, soit du tableau, soit supprimant son chainage
			if (pAssoc == pHeadAssoc)
				pvODAssocs.SetAt(nHash, pAssoc->pNext); // remove from list
			else
				pAssocPrev->pNext = pAssoc->pNext;
			FreeAssoc(pAssoc);

			// Retaillage dynamique
			assert(GetHashTableSize() < INT_MAX / 4);
			if (GetCount() < GetHashTableSize() / 8)
				ReinitHashTable(DictionaryGetNextTableSize(GetCount() * 2));

			return true;
		}
		pAssocPrev = pAssoc;
	}

	// La cle n'a pas ete trouve
	return false;
}

void ObjectDictionary::GetNextAssoc(POSITION& rNextPosition, ALString& rKey, Object*& rValue) const
{
	ODAssoc* pAssocRet;
	int nBucket;
	ODAssoc* pAssocNext;

	require(pvODAssocs.GetSize() != 0);
	require(rNextPosition != NULL);

	pAssocRet = (ODAssoc*)rNextPosition;
	assert(pAssocRet != NULL);

	if (pAssocRet == (ODAssoc*)BEFORE_START_POSITION)
	{
		// Recherche de la premiere association
		for (nBucket = 0; nBucket < pvODAssocs.GetSize(); nBucket++)
			if ((pAssocRet = (ODAssoc*)pvODAssocs.GetAt(nBucket)) != NULL)
				break;
		assert(pAssocRet != NULL);
	}

	// Recherche de l'association suivante
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// Passage au prochain bucket
		for (nBucket = pAssocRet->nHashValue + 1; nBucket < pvODAssocs.GetSize(); nBucket++)
			if ((pAssocNext = (ODAssoc*)pvODAssocs.GetAt(nBucket)) != NULL)
				break;
	}

	rNextPosition = (POSITION)pAssocNext;

	// Initialisation des resultats
	rKey = pAssocRet->key;
	rValue = pAssocRet->value;
}

void ObjectDictionary::CopyFrom(const ObjectDictionary* odSource)
{
	POSITION current;
	ALString sKey;
	Object* oElement;

	require(odSource != NULL);

	// Cas particulier ou source egale cible
	if (odSource == this)
		return;

	// Nettoyage
	RemoveAll();

	// Recopie du contenu du dictionnaire source
	current = odSource->GetStartPosition();
	while (current != NULL)
	{
		odSource->GetNextAssoc(current, sKey, oElement);
		SetAt(sKey, oElement);
	}
}

ObjectDictionary* ObjectDictionary::Clone(void) const
{
	ObjectDictionary* clone;

	// Innitialisation du resultat
	clone = new ObjectDictionary;

	// Recopie
	clone->CopyFrom(this);
	return clone;
}

void ObjectDictionary::ExportObjectArray(ObjectArray* oaResult) const
{
	POSITION current;
	ALString sKey;
	Object* oElement;
	int nIndex;

	require(oaResult != NULL);

	// Initialisation du resultat
	oaResult->SetSize(GetCount());

	// Parcours du dictionnaire
	current = GetStartPosition();
	nIndex = 0;
	while (current != NULL)
	{
		GetNextAssoc(current, sKey, oElement);
		oaResult->SetAt(nIndex, oElement);
		nIndex++;
	}
	ensure(oaResult->GetSize() == GetCount());
}

void ObjectDictionary::ExportObjectList(ObjectList* olResult) const
{
	POSITION current;
	ALString sKey;
	Object* oElement;

	require(olResult != NULL);

	// Initialisation du resultat
	olResult->RemoveAll();

	// Parcours du dictionnaire
	current = GetStartPosition();
	while (current != NULL)
	{
		GetNextAssoc(current, sKey, oElement);
		olResult->AddTail(oElement);
	}
	ensure(olResult->GetCount() == GetCount());
}

void ObjectDictionary::Write(ostream& ost) const
{
	POSITION current;
	ALString sKey;
	Object* oElement;
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	current = GetStartPosition();
	n = 0;
	while (current != NULL)
	{
		n++;
		GetNextAssoc(current, sKey, oElement);
		ost << "\t" << sKey << ":";
		if (oElement == NULL)
			ost << "\tnull\n";
		else
			ost << "\t" << *oElement << "\n";
		if (n >= nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint ObjectDictionary::GetUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	Object* oValue;
	ALString sKey;

	lUsedMemory = sizeof(ObjectDictionary) + pvODAssocs.GetUsedMemory() + nCount * sizeof(ODAssoc) +
		      (nCount * sizeof(CPlex)) / nBlockSize;

	// Prise en compte des tailles des cles
	position = GetStartPosition();
	while (position != NULL)
	{
		GetNextAssoc(position, sKey, oValue);
		lUsedMemory += sKey.GetAllocLength();
	}
	return lUsedMemory;
}

longint ObjectDictionary::GetOverallUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	Object* oValue;
	ALString sKey;

	lUsedMemory = sizeof(ObjectDictionary) + pvODAssocs.GetUsedMemory() + nCount * sizeof(ODAssoc) +
		      (nCount * sizeof(CPlex)) / nBlockSize;

	// Prise en compte des tailles des cles et des objets
	position = GetStartPosition();
	while (position != NULL)
	{
		GetNextAssoc(position, sKey, oValue);
		lUsedMemory += sKey.GetAllocLength();
		if (oValue != NULL)
			lUsedMemory += oValue->GetUsedMemory();
	}
	return lUsedMemory;
}

longint ObjectDictionary::GetUsedMemoryPerElement() const
{
	return sizeof(void*) + sizeof(ODAssoc);
}

const ALString ObjectDictionary::GetClassLabel() const
{
	return "Dictionary";
}

/////////////////////////////////////////////////////////////////////////////

void ObjectDictionary::Test()
{
	SampleObject soTest;
	SampleObject* soToDelete;
	ObjectDictionary odTest;
	ObjectDictionary odPerf;
	ObjectArray oaConverted;
	ObjectList olConverted;
	SampleObject soArray[11];
	ALString sKey;
	ALString sPerf;
	int i;
	POSITION position;
	int nMaxSize;
	int nNb;
	int nNbIter;
	int nInsert;
	int nFound;
	int nIter;
	int nStartClock;
	int nStopClock;

	// Initialisation de 11 Strings
	soArray[0].SetString("zero");
	soArray[1].SetString("un");
	soArray[2].SetString("deux");
	soArray[3].SetString("trois");
	soArray[4].SetString("quatre");
	soArray[5].SetString("cinq");
	soArray[6].SetString("six");
	soArray[7].SetString("sept");
	soArray[8].SetString("huit");
	soArray[9].SetString("neuf");
	soArray[10].SetString("dix");

	/////
	cout << "Test des fonctionnalites de base\n";
	//
	cout << "\tInsertion de 10 SampleObject\n";
	for (i = 0; i < 10; i++)
	{
		odTest.SetAt(soArray[i].GetString(), &soArray[i]);
	}
	cout << odTest;

	//
	cout << "\tTest d'existence\n";
	for (i = 8; i < 11; i++)
	{
		sKey = soArray[i].GetString();
		cout << "\t\tLookup (" << sKey << "): " << (odTest.Lookup(soArray[i].GetString()) != NULL) << "\n";
	}

	//
	cout << "Insertion puis supression d'un element temporaire a detruire\n";
	soToDelete = new SampleObject;
	soToDelete->SetString("Temporary");
	odTest.SetAt(soToDelete->GetString(), soToDelete);
	cout << "Inserted: " << (odTest.Lookup("Temporary") != NULL) << endl;
	odTest.RemoveKey(soToDelete->GetString());
	cout << "Removed: " << (odTest.Lookup("Temporary") == NULL) << endl;
	delete soToDelete;
	soToDelete = NULL;
	cout << "Removed and deleted: " << (odTest.Lookup("Temporary") == NULL) << endl;

	////
	cout << "Conversion en ObjectArray\n";
	odTest.ExportObjectArray(&oaConverted);
	for (i = 0; i < oaConverted.GetSize(); i++)
	{
		sKey = cast(SampleObject*, oaConverted.GetAt(i))->GetString();
		cout << "\tAt " << i << "; " << sKey << "\n";
	}
	cout << "\n";

	////
	cout << "Conversion en ObjectList\n";
	odTest.ExportObjectList(&olConverted);
	position = olConverted.GetHeadPosition();
	while (position != NULL)
	{
		sKey = cast(SampleObject*, olConverted.GetNext(position))->GetString();
		cout << "\t" << sKey << "\n";
	}
	cout << "\n";

	/////
	cout << "Test de changement de taille dynamique\n";
	nMaxSize = AcquireRangedInt("Nombre d'elements inseres)", 1, 100000, 1000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 100000, 1000);

	nStartClock = clock();
	for (nNb = 0; nNb < nNbIter; nNb++)
	{
		odPerf.RemoveAll();
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			sPerf = IntToString(nInsert);
			odPerf.SetAt(sPerf, &soTest);
		}
	}
	nStopClock = clock();
	cout << "  HashTableSize = " << odPerf.GetHashTableSize() << "\n";
	cout << "SYS TIME\tObjectDictionary change size\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n\n";

	/////
	cout << "Test de performance A\n";
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 1000000, 50000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	nStartClock = clock();
	for (nIter = 0; nIter < nNbIter; nIter++)
	{
		// Insertions
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			sPerf = IntToString(RandomInt(nMaxSize - 1));
			odPerf.SetAt(sPerf, &soTest);
		}
		cout << "\tIteration " << nIter << "\tSize = " << odPerf.GetCount();

		// Recherches
		nFound = 0;
		for (nInsert = 0; nInsert <= nMaxSize; nInsert++)
		{
			sPerf = IntToString(nInsert);
			if (odPerf.Lookup(sPerf) != NULL)
				nFound++;
		}
		cout << "\tFound = " << nFound << "\n";
	}
	nStopClock = clock();
	cout << "SYS TIME\tObjectDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
	cout << "SYS MEMORY\tUsed memory\t" << odPerf.GetCount() << "\t" << odPerf.GetUsedMemory() << "\t"
	     << odPerf.GetOverallUsedMemory() << endl;

	/////
	cout << "Test de performance B\n";
	odPerf.RemoveAll();
	nMaxSize = AcquireRangedInt("Nombre de chaines inserees", 1, 10000000, 10000);
	nStartClock = clock();

	// Insertions
	for (nInsert = 0; nInsert < nMaxSize; nInsert++)
	{
		sPerf = IntToString(nInsert);
		odPerf.SetAt(sPerf, &soTest);
	}
	nStopClock = clock();
	cout << "SYS TIME\tObjectDictionary insertion\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Recherches
	nNbIter = AcquireRangedInt("Nombre de recherches", 1, 100000000, 100000);
	sPerf = IntToString(0);
	nFound = 0;
	nStartClock = clock();
	for (nIter = 0; nIter < nNbIter; nIter++)
	{
		if (odPerf.Lookup(sPerf) != NULL)
			nFound++;
	}
	assert(nFound == nNbIter);
	nStopClock = clock();
	cout << "SYS TIME\tObjectDictionary lookup\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Supressions
	cout << "Nombre de supressions: " << nMaxSize << endl;
	nFound = 0;
	nStartClock = clock();
	for (nInsert = 0; nInsert < nMaxSize; nInsert++)
	{
		sPerf = IntToString(nInsert);
		if (odPerf.RemoveKey(sPerf))
			nFound++;
	}
	assert(nFound == nMaxSize);
	assert(odPerf.GetCount() == 0);
	nStopClock = clock();
	cout << "SYS TIME\tObjectDictionary remove\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

NumericKeyDictionary::NumericKeyDictionary()
{
	assert(nBlockSize > 0);

	nCount = 0;
	pFreeList = NULL;
	pBlocks = NULL;
}

void NumericKeyDictionary::InitHashTable(int nHashSize)
{
	assert(nCount == 0);
	assert(nHashSize > 0);

	pvNKDAssocs.SetSize(nHashSize);
}

void NumericKeyDictionary::ReinitHashTable(int nNewHashSize)
{
	PointerVector pvNewNKDAssocs;
	int i;
	int nHash;
	NKDAssoc* pAssoc;
	NKDAssoc* pAssocNext;

	// Creation d'une nouvelle table de hashage
	pvNewNKDAssocs.SetSize(nNewHashSize);

	// Recalcul des carcateristiques des elements par rapport a la nouvelle taille de la table de hashage
	for (i = 0; i < pvNKDAssocs.GetSize(); i++)
	{
		for (pAssoc = (NKDAssoc*)pvNKDAssocs.GetAt(i); pAssoc != NULL;)
		{
			// Sauvegarde du suivant
			pAssocNext = pAssoc->pNext;

			// Reconversion de pAssoc, vers la nouvelle table de hashage
			nHash = HashKey(pAssoc->key) % nNewHashSize;
			pAssoc->nHashValue = nHash;
			pAssoc->pNext = (NKDAssoc*)pvNewNKDAssocs.GetAt(nHash);
			pvNewNKDAssocs.SetAt(nHash, pAssoc);

			// Passage au suivant
			pAssoc = pAssocNext;
		}
	}

	// Echange du contenu entre l'ancienne et la nouvelle table de hashage
	// Le contenu de la nouvelle table de hashage courante est transferer dans la table courante
	// Le contenu de l'ancienne table de hashage, transfere dans la variable locale pvNewNKDAssocs, est detruit
	pvNKDAssocs.SwapFrom(&pvNewNKDAssocs);
}

void NumericKeyDictionary::RemoveAll()
{
	// Reinitialisation des variables
	pvNKDAssocs.SetSize(0);
	nCount = 0;
	pFreeList = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

void NumericKeyDictionary::DeleteAll(void)
{
	POSITION current;
	NUMERIC key;
	Object* oElement;

	current = GetStartPosition();
	while (current != NULL)
	{
		GetNextAssoc(current, key, oElement);
		if (oElement != NULL) // Destruction de l'element contenu
			delete oElement;
	}
	RemoveAll();
}

NumericKeyDictionary::~NumericKeyDictionary()
{
	RemoveAll();
	assert(nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Gestion des associations
// Comme pour l'implementation des ObjectList, sauf que l'on gere
// ici une liste simplement chainee

NKDAssoc* NumericKeyDictionary::GetAssocAt(NUMERIC key, UINT& nHash) const
{
	NKDAssoc* pAssoc;

	// Cas particulier d'un dictionnaire vide
	if (pvNKDAssocs.GetSize() == 0)
	{
		assert(nCount == 0);

		// Calcul de sa cle par rapport a la premiere taille de dictionnaire
		nHash = HashKey(key) % DictionaryGetNextTableSize(1);
		return NULL;
	}

	// Calcul de la cle par rapport a la taille courante de la table de hashage
	nHash = HashKey(key) % pvNKDAssocs.GetSize();
	// Recherche si elle existe
	for (pAssoc = (NKDAssoc*)pvNKDAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (pAssoc->key == key)
			return pAssoc;
	}
	return NULL;
}

NKDAssoc* NumericKeyDictionary::NewAssoc()
{
	if (pFreeList == NULL)
	{
		// Ajout d'un nouveau bloc
		CPlex* newBlock = CPlex::Create(pBlocks, nBlockSize, sizeof(NKDAssoc));
		// Chainage dans la liste des bloc libres
		NKDAssoc* pAssoc = (NKDAssoc*)newBlock->data();
		// Chainage en ordre inverse pour faciliter le debuggage
		pAssoc += nBlockSize - 1;
		for (int i = nBlockSize - 1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = pFreeList;
			pFreeList = pAssoc;
		}
	}
	assert(pFreeList != NULL);

	NKDAssoc* pAssoc = pFreeList;
	pFreeList = pFreeList->pNext;
	nCount++;
	pAssoc->key = NULL;
	pAssoc->value = NULL;
	assert(nCount > 0);

	return pAssoc;
}

/////////////////////////////////////////////////////////////////////////////

Object*& NumericKeyDictionary::operator[](NUMERIC key)
{
	UINT nHash;
	NKDAssoc* pAssoc;

	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (pvNKDAssocs.GetSize() == 0)
		{
			assert(nCount == 0);
			InitHashTable(DictionaryGetNextTableSize(1));
		}

		// Cree une nouvelle Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		pAssoc->key = key;

		// Ajout dans la table de hashage
		pAssoc->pNext = (NKDAssoc*)pvNKDAssocs.GetAt(nHash);
		pvNKDAssocs.SetAt(nHash, pAssoc);

		// Retaillage dynamique
		assert(GetHashTableSize() < INT_MAX / 4);
		if (GetCount() > GetHashTableSize())
			ReinitHashTable(DictionaryGetNextTableSize(2 * GetHashTableSize()));
	}
	return pAssoc->value;
}

boolean NumericKeyDictionary::RemoveKey(NUMERIC key)
{
	NKDAssoc* pHeadAssoc;
	NKDAssoc* pAssocPrev;
	NKDAssoc* pAssoc;
	int nHash;

	// Cas d'une table vide: rien a faire
	if (pvNKDAssocs.GetSize() == 0)
		return false;

	// Recherche du premier element de la table de hashage correspondant a la cle
	nHash = HashKey(key) % pvNKDAssocs.GetSize();
	pHeadAssoc = (NKDAssoc*)pvNKDAssocs.GetAt(nHash);

	// Parcours de la liste chainee des elements pour supprimer l'element correspondant exactement a la cle
	pAssocPrev = NULL;
	for (pAssoc = pHeadAssoc; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (pAssoc->key == key)
		{
			assert(pAssoc == pHeadAssoc or pAssocPrev != NULL);
			// Supression de l'element, soit du tableau, soit supprimant son chainage
			if (pAssoc == pHeadAssoc)
				pvNKDAssocs.SetAt(nHash, pAssoc->pNext); // remove from list
			else
				pAssocPrev->pNext = pAssoc->pNext;
			FreeAssoc(pAssoc);

			// Retaillage dynamique
			assert(GetHashTableSize() < INT_MAX / 4);
			if (GetCount() < GetHashTableSize() / 8)
				ReinitHashTable(DictionaryGetNextTableSize(GetCount() * 2));

			return true;
		}
		pAssocPrev = pAssoc;
	}

	// La cle n'a pas ete trouve
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Iterating

void NumericKeyDictionary::GetNextAssoc(POSITION& rNextPosition, NUMERIC& rKey, Object*& rValue) const
{
	NKDAssoc* pAssocRet;
	int nBucket;
	NKDAssoc* pAssocNext;

	require(pvNKDAssocs.GetSize() != 0);
	require(rNextPosition != NULL);

	pAssocRet = (NKDAssoc*)rNextPosition;
	assert(pAssocRet != NULL);

	if (pAssocRet == (NKDAssoc*)BEFORE_START_POSITION)
	{
		// Recherche de la premiere association
		for (nBucket = 0; nBucket < pvNKDAssocs.GetSize(); nBucket++)
			if ((pAssocRet = (NKDAssoc*)pvNKDAssocs.GetAt(nBucket)) != NULL)
				break;
		assert(pAssocRet != NULL); // must find something
	}

	// Recherche de l'association suivante
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// Passage au prochain bucket
		for (nBucket = pAssocRet->nHashValue + 1; nBucket < pvNKDAssocs.GetSize(); nBucket++)
			if ((pAssocNext = (NKDAssoc*)pvNKDAssocs.GetAt(nBucket)) != NULL)
				break;
	}

	rNextPosition = (POSITION)pAssocNext;

	// fill in return data
	rKey = pAssocRet->key;
	rValue = pAssocRet->value;
}

void NumericKeyDictionary::CopyFrom(const NumericKeyDictionary* nkdSource)
{
	POSITION current;
	NUMERIC key;
	Object* oElement;

	require(nkdSource != NULL);

	// Cas particulier ou source egale cible
	if (nkdSource == this)
		return;

	// Nettoyage
	RemoveAll();

	// Recopie du contenu du dictionnaire source
	current = nkdSource->GetStartPosition();
	while (current != NULL)
	{
		nkdSource->GetNextAssoc(current, key, oElement);
		SetAt(key, oElement);
	}
}

NumericKeyDictionary* NumericKeyDictionary::Clone(void) const
{
	NumericKeyDictionary* clone;

	// Inbitialisation du resultat
	clone = new NumericKeyDictionary;

	// Recopie
	clone->CopyFrom(this);
	return clone;
}

void NumericKeyDictionary::ExportObjectArray(ObjectArray* oaResult) const
{
	POSITION current;
	NUMERIC key;
	Object* oElement;
	int nIndex;

	require(oaResult != NULL);

	// Initialisation du resultat
	oaResult->SetSize(GetCount());

	// Parcours du dictionnaire
	current = GetStartPosition();
	nIndex = 0;
	while (current != NULL)
	{
		GetNextAssoc(current, key, oElement);
		oaResult->SetAt(nIndex, oElement);
		nIndex++;
	}
	ensure(oaResult->GetSize() == GetCount());
}

void NumericKeyDictionary::ExportObjectList(ObjectList* olResult) const
{
	POSITION current;
	NUMERIC key;
	Object* oElement;

	require(olResult != NULL);

	// Initialisation du resultat
	olResult->RemoveAll();

	// Parcours du dictionnaire
	current = GetStartPosition();
	while (current != NULL)
	{
		GetNextAssoc(current, key, oElement);
		olResult->AddTail(oElement);
	}
	ensure(olResult->GetCount() == GetCount());
}

void NumericKeyDictionary::Write(ostream& ost) const
{
	POSITION current;
	NUMERIC key;
	Object* oElement;
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	current = GetStartPosition();
	n = 0;
	while (current != NULL)
	{
		n++;
		GetNextAssoc(current, key, oElement);
		ost << "\t" << key << ":";
		if (oElement == NULL)
			ost << "\tnull\n";
		else
			ost << "\t" << *oElement << "\n";
		if (n >= nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint NumericKeyDictionary::GetUsedMemory() const
{
	return sizeof(NumericKeyDictionary) + pvNKDAssocs.GetUsedMemory() + nCount * sizeof(NKDAssoc) +
	       (nCount * sizeof(CPlex)) / nBlockSize;
}

longint NumericKeyDictionary::GetOverallUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	Object* oValue;
	NUMERIC key;

	lUsedMemory = GetUsedMemory();
	position = GetStartPosition();
	while (position != NULL)
	{
		GetNextAssoc(position, key, oValue);
		if (oValue != NULL)
			lUsedMemory += oValue->GetUsedMemory();
	}
	return lUsedMemory;
}

longint NumericKeyDictionary::GetUsedMemoryPerElement() const
{
	return sizeof(void*) + sizeof(NKDAssoc);
}

const ALString NumericKeyDictionary::GetClassLabel() const
{
	return "Numeric key dictionary";
}

/////////////////////////////////////////////////////////////////////////////

void NumericKeyDictionary::Test()
{
	SampleObject* soElement;
	SampleObject soTest;
	SampleObject* soToDelete;
	NumericKeyDictionary nkdTest;
	NumericKeyDictionary nkdPerf;
	ObjectArray oaSmallNumericKeyArray;
	ObjectArray oaLargeNumericKeyArray;
	NUMERIC numPerf;
	int i;
	int nMaxSize;
	int nNbIter;
	int nInsert;
	int nFound;
	int nStartClock;
	int nStopClock;

	// Initialisation de 11 NUMERIC
	SetRandomSeed(1);
	oaSmallNumericKeyArray.SetSize(11);
	for (i = 0; i < oaSmallNumericKeyArray.GetSize(); i++)
	{
		soElement = new SampleObject;
		soElement->SetInt(i);
		oaSmallNumericKeyArray.SetAt(i, soElement);
	}

	/////
	cout << "Test des fonctionnalites de base\n";
	//
	cout << "Insertion de 10 NUMERICs\n";
	for (i = 0; i < 10; i++)
	{
		nkdTest.SetAt(oaSmallNumericKeyArray.GetAt(i), &soTest);
	}
	cout << "\tNumeric key dictionary size\t" << nkdTest.GetCount() << "\n";

	//
	cout << "Test d'existence\n";
	for (i = 8; i < 11; i++)
	{
		soElement = cast(SampleObject*, oaSmallNumericKeyArray.GetAt(i));
		cout << "\tLookup (" << *soElement << "): " << (nkdTest.Lookup(soElement) != NULL) << "\n";
	}

	//
	cout << "Insertion puis supression d'un element temporaire a detruire\n";
	soToDelete = new SampleObject;
	soToDelete->SetString("Temporary");
	nkdTest.SetAt(&soTest, soToDelete);
	cout << "Inserted: " << (nkdTest.Lookup(&soTest) != NULL) << endl;
	nkdTest.RemoveKey(&soTest);
	cout << "Removed: " << (nkdTest.Lookup(&soTest) == NULL) << endl;
	delete soToDelete;
	soToDelete = NULL;
	cout << "Removed and deleted: " << (nkdTest.Lookup(&soTest) == NULL) << endl;

	/////
	cout << "Test de performance\n";
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 1000000, 50000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	// Creation d'un grand tableau de cles numeriques toutes differentes
	oaLargeNumericKeyArray.SetSize(nMaxSize);
	for (i = 0; i < oaLargeNumericKeyArray.GetSize(); i++)
		oaLargeNumericKeyArray.SetAt(i, new SampleObject);
	//
	nStartClock = clock();
	for (int nIter = 0; nIter < nNbIter; nIter++)
	{
		// Insertions
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			numPerf = oaLargeNumericKeyArray.GetAt(RandomInt(nMaxSize - 1));
			nkdPerf.SetAt(numPerf, &soTest);
		}
		cout << "\tIteration " << nIter << "\tSize = " << nkdPerf.GetCount();

		// Recherches
		nFound = 0;
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			numPerf = oaLargeNumericKeyArray.GetAt(nInsert);
			if (nkdPerf.Lookup(numPerf) != NULL)
				nFound++;
		}
		cout << "\tFound = " << nFound << "\n";
	}
	nStopClock = clock();
	cout << "SYS TIME\tNumericKeyDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n\n";
	cout << "SYS MEMORY\tUsed memory\t" << nkdPerf.GetCount() << "\t" << nkdPerf.GetUsedMemory() << "\t"
	     << nkdPerf.GetOverallUsedMemory() << endl;

	// Nettoyage
	oaSmallNumericKeyArray.DeleteAll();
	oaLargeNumericKeyArray.DeleteAll();
}