// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Object.h"

// Tableau des tailles d'allocations des dictionnaires
static const int nDictionaryPrimeSizes[] = {17,       37,       79,        163,       331,       673,       1361,
					    2729,     5471,     10949,     21911,     43853,     87719,     175447,
					    350899,   701819,   1403641,   2807303,   5614657,   11229331,  22458671,
					    44917381, 89834777, 179669557, 359339171, 718678369, 1437356741};
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
	return INT_MAX;
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

GenericDictionary::GenericDictionary()
{
	assert(nBlockSize > 0);

	nCount = 0;
	pFreeList = NULL;
	pBlocks = NULL;

	// Par defaut: cle chaine de caractere et valeur de type Object*
	bIsStringKey = true;
	bIsObjectValue = true;
}

void GenericDictionary::RemoveAll()
{
	int nHash;
	GDAssoc* pAssoc;

	// Nettoyage des cles de la table de hashage
	if (bIsStringKey)
	{
		for (nHash = 0; nHash < pvGDAssocs.GetSize(); nHash++)
		{
			for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
			{
				assert(pAssoc->key.genericKey != 0);
				DeleteCharArray(pAssoc->key.sKey);
				pAssoc->key.genericKey = 0;
			}
		}
	}

	// Reinitialisation des variables
	pvGDAssocs.SetSize(0);
	nCount = 0;
	pFreeList = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

void GenericDictionary::DeleteAll()
{
	int nHash;
	GDAssoc* pAssoc;
	POSITION current;
	GenericKey genericKey;
	GenericValue genericValue;

	// Destruction des cles et des valeurs de la table de hashage dans le
	if (bIsStringKey)
	{
		for (nHash = 0; nHash < pvGDAssocs.GetSize(); nHash++)
		{
			for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
			{
				DeleteCharArray(pAssoc->key.sKey);
				pAssoc->key.genericKey = 0;
				if (bIsObjectValue)
				{
					if (pAssoc->value.oValue != NULL)
						delete pAssoc->value.oValue;
				}
			}
		}
	}
	// Destuction si necessaire des valeurs dans le cas de cles numeriques
	else if (bIsObjectValue)
	{
		assert(not bIsStringKey);
		current = GenericGetStartPosition();
		while (current != NULL)
		{
			GenericGetNextAssoc(current, genericKey, genericValue);
			if (genericValue.oValue != NULL) // Destruction de l'element contenu
				delete genericValue.oValue;
		}
	}

	// Reinitialisation des variables
	pvGDAssocs.SetSize(0);
	nCount = 0;
	pFreeList = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

int GenericDictionary::RemoveAllNullValues()
{
	int nRemovedKeyNumber = 0;
	int i;
	GDAssoc* pAssocPrev;
	GDAssoc* pAssoc;
	GDAssoc* pAssocNext;

	require(not bIsObjectValue);

	// Parcours de toute la table de hashage
	nRemovedKeyNumber = GetCount();
	for (i = 0; i < pvGDAssocs.GetSize(); i++)
	{
		// Parcours de la liste chainee
		pAssocPrev = NULL;
		for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL;)
		{
			assert(pAssoc == (GDAssoc*)pvGDAssocs.GetAt(i) or pAssocPrev != NULL);
			assert(pAssocPrev == NULL or pAssocPrev->pNext == pAssoc);

			// Sauvegarde du suivant
			pAssocNext = pAssoc->pNext;

			// Supression si la valeur est nulle
			if (pAssoc->value.lValue == 0)
			{
				// Supression de l'element, soit du tableau, soit supprimant son chainage
				if (pAssocPrev == NULL)
				{
					assert(pAssoc == (GDAssoc*)pvGDAssocs.GetAt(i));
					pvGDAssocs.SetAt(i, pAssocNext);
				}
				else
				{
					assert(pAssoc != (GDAssoc*)pvGDAssocs.GetAt(i));
					pAssocPrev->pNext = pAssoc->pNext;
				}
				FreeAssoc(pAssoc);
			}
			// Passage au prev suivant
			else
				pAssocPrev = pAssoc;

			// Passage au suivant
			pAssoc = pAssocNext;
		}
	}

	// Calcul par difference du nombre de cke supprimmees
	nRemovedKeyNumber -= GetCount();

	// Retaillage dynamique
	assert(GetHashTableSize() < INT_MAX / 4);
	if (GetCount() < GetHashTableSize() / 8)
		ReinitHashTable(DictionaryGetNextTableSize(GetCount() * 2));
	return nRemovedKeyNumber;
}

void GenericDictionary::ExportObjectArray(ObjectArray* oaResult) const
{
	POSITION current;
	GenericKey genericKey;
	GenericValue genericValue;
	int nIndex;

	require(bIsObjectValue);
	require(oaResult != NULL);

	// Initialisation du resultat
	oaResult->SetSize(GetCount());

	// Parcours du dictionnaire
	current = GenericGetStartPosition();
	nIndex = 0;
	while (current != NULL)
	{
		GenericGetNextAssoc(current, genericKey, genericValue);
		oaResult->SetAt(nIndex, genericValue.oValue);
		nIndex++;
	}
	ensure(oaResult->GetSize() == GetCount());
}

void GenericDictionary::ExportObjectList(ObjectList* olResult) const
{
	POSITION current;
	GenericKey genericKey;
	GenericValue genericValue;

	require(bIsObjectValue);
	require(olResult != NULL);

	// Initialisation du resultat
	olResult->RemoveAll();

	// Parcours du dictionnaire
	current = GenericGetStartPosition();
	while (current != NULL)
	{
		GenericGetNextAssoc(current, genericKey, genericValue);
		olResult->AddTail(genericValue.oValue);
	}
	ensure(olResult->GetCount() == GetCount());
}

void GenericDictionary::UpgradeAll(longint liDeltaValue)
{
	int i;
	GDAssoc* pAssoc;

	require(not bIsObjectValue);

	// Parcours de toute la table de hashage
	for (i = 0; i < pvGDAssocs.GetSize(); i++)
	{
		// Parcours de la liste chainee
		for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL; pAssoc = pAssoc->pNext)
			pAssoc->value.lValue += liDeltaValue;
	}
}

void GenericDictionary::BoundedUpgradeAll(longint liDeltaValue, longint lLowerBound, longint lUpperBound)
{
	int i;
	GDAssoc* pAssoc;
	longint lNewValue;

	require(not bIsObjectValue);
	require(lLowerBound <= lUpperBound);

	// Parcours de toute la table de hashage
	for (i = 0; i < pvGDAssocs.GetSize(); i++)
	{
		// Parcours de la liste chainee
		for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL; pAssoc = pAssoc->pNext)
		{
			lNewValue = pAssoc->value.lValue + liDeltaValue;
			lNewValue = max(lNewValue, lLowerBound);
			lNewValue = min(lNewValue, lUpperBound);
			pAssoc->value.lValue = lNewValue;
		}
	}
}

longint GenericDictionary::ComputeMinValue() const
{
	longint lMinValue;
	int i;
	GDAssoc* pAssoc;

	require(not bIsObjectValue);

	// Parcours de toute la table de hashage
	if (pvGDAssocs.GetSize() > 0)
	{
		lMinValue = LLONG_MAX;
		for (i = 0; i < pvGDAssocs.GetSize(); i++)
		{
			// Parcours de la liste chainee
			for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL; pAssoc = pAssoc->pNext)
				lMinValue = min(lMinValue, pAssoc->value.lValue);
		}
	}
	else
		lMinValue = 0;
	return lMinValue;
}

longint GenericDictionary::ComputeMaxValue() const
{
	longint lMaxValue;
	int i;
	GDAssoc* pAssoc;

	require(not bIsObjectValue);

	// Parcours de toute la table de hashage
	if (pvGDAssocs.GetSize() > 0)
	{
		lMaxValue = LLONG_MIN;
		for (i = 0; i < pvGDAssocs.GetSize(); i++)
		{
			// Parcours de la liste chainee
			for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL; pAssoc = pAssoc->pNext)
				lMaxValue += max(lMaxValue, pAssoc->value.lValue);
		}
	}
	else
		lMaxValue = 0;
	return lMaxValue;
}

longint GenericDictionary::ComputeTotalValue() const
{
	longint lTotalValue;
	int i;
	GDAssoc* pAssoc;

	require(not bIsObjectValue);

	// Parcours de toute la table de hashage
	lTotalValue = 0;
	for (i = 0; i < pvGDAssocs.GetSize(); i++)
	{
		// Parcours de la liste chainee
		for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL; pAssoc = pAssoc->pNext)
			lTotalValue += pAssoc->value.lValue;
	}
	return lTotalValue;
}

void GenericDictionary::Write(ostream& ost) const
{
	POSITION current;
	GenericKey genericKey;
	GenericValue genericValue;
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	current = GenericGetStartPosition();
	n = 0;
	while (current != NULL)
	{
		n++;
		GenericGetNextAssoc(current, genericKey, genericValue);

		// Affichage de la cle selon son type
		if (bIsStringKey)
			ost << "\t" << genericKey.sKey << ":";
		else
			ost << "\t" << genericKey.genericKey << ":";

		// Affichage de la valeur selon son type
		if (bIsObjectValue)
		{
			if (genericValue.oValue == NULL)
				ost << "\tnull\n";
			else
				ost << "\t" << *genericValue.oValue << "\n";
		}
		else
			ost << "\t" << genericValue.genericValue << "\n";

		// Gestion du nombre limite de valeurs a afficher
		if (n >= nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint GenericDictionary::GetUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	GenericKey genericKey;
	GenericValue genericValue;

	lUsedMemory = sizeof(GenericDictionary) + pvGDAssocs.GetUsedMemory() + nCount * sizeof(GDAssoc) +
		      (nCount * sizeof(CPlex)) / nBlockSize;

	// Prise en compte des tailles des cles
	position = GenericGetStartPosition();
	while (position != NULL)
	{
		GenericGetNextAssoc(position, genericKey, genericValue);
		if (bIsStringKey)
			lUsedMemory += strlen(genericKey.sKey);
	}
	return lUsedMemory;
}

longint GenericDictionary::GetOverallUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	GenericKey genericKey;
	GenericValue genericValue;

	lUsedMemory = sizeof(GenericDictionary) + pvGDAssocs.GetUsedMemory() + nCount * sizeof(GDAssoc) +
		      (nCount * sizeof(CPlex)) / nBlockSize;

	// Prise en compte des tailles des cles et des objets
	position = GenericGetStartPosition();
	while (position != NULL)
	{
		GenericGetNextAssoc(position, genericKey, genericValue);
		if (bIsStringKey)
			lUsedMemory += strlen(genericKey.sKey);
		if (bIsObjectValue)
		{
			if (genericValue.oValue != NULL)
				lUsedMemory += genericValue.oValue->GetUsedMemory();
		}
	}
	return lUsedMemory;
}

longint GenericDictionary::GetUsedMemoryPerElement() const
{
	return sizeof(void*) + sizeof(GDAssoc);
}

/////////////////////////////////////////////////////////////////////////////

GDAssoc* GenericDictionary::GenericGetAssocAt(GenericKey genericKey)
{
	UINT nHash;
	GDAssoc* pAssoc;
	int nLength;

	require(genericKey.genericKey != 0 or not bIsStringKey);

	if ((pAssoc = GetAssocAt(genericKey, nHash)) == NULL)
	{
		if (pvGDAssocs.GetSize() == 0)
		{
			assert(nCount == 0);
			InitHashTable(DictionaryGetNextTableSize(1));
		}

		// Cree une nouvelle Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		if (bIsStringKey)
		{
			if (pAssoc->key.sKey != NULL)
				DeleteCharArray(pAssoc->key.sKey);
			assert(strlen(genericKey.sKey) <= INT_MAX);
			nLength = (int)strlen(genericKey.sKey);
			pAssoc->key.sKey = NewCharArray(nLength + 1);
			pAssoc->key.sKey[nLength] = '\0';
			memcpy(pAssoc->key.sKey, genericKey.sKey, nLength);
		}
		else
			pAssoc->key = genericKey;

		// Ajout dans la table de hashage
		pAssoc->pNext = (GDAssoc*)pvGDAssocs.GetAt(nHash);
		pvGDAssocs.SetAt(nHash, pAssoc);

		// Retaillage dynamique
		if (GetCount() > GetHashTableSize() / 2 and GetHashTableSize() < INT_MAX)
			ReinitHashTable(DictionaryGetNextTableSize(2 * min(GetHashTableSize(), INT_MAX / 2)));
	}
	return pAssoc;
}

boolean GenericDictionary::GenericRemoveKey(GenericKey genericKey)
{
	GDAssoc* pHeadAssoc;
	GDAssoc* pAssocPrev;
	GDAssoc* pAssoc;
	int nHash;
	boolean bKeyFound;

	require(genericKey.genericKey != 0 or not bIsStringKey);

	// Cas d'une table vide: rien a faire
	if (pvGDAssocs.GetSize() == 0)
		return false;

	// Recherche du premier element de la table de hashage correspondant a la cle
	nHash = HashKey(genericKey) % pvGDAssocs.GetSize();
	pHeadAssoc = (GDAssoc*)pvGDAssocs.GetAt(nHash);

	// Parcours de la liste chainee des elements pour supprimer l'element correspondant exactement a la cle
	pAssocPrev = NULL;
	for (pAssoc = pHeadAssoc; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		// Test si cle trouve selon le type de cle
		if (bIsStringKey)
		{
			assert(genericKey.genericKey != 0);
			bKeyFound = strcmp(pAssoc->key.sKey, genericKey.sKey) == 0;
		}
		else
			bKeyFound = pAssoc->key.genericKey == genericKey.genericKey;

		// Supression si on a trouve la cle
		if (bKeyFound)
		{
			assert(pAssoc == pHeadAssoc or pAssocPrev != NULL);
			// Supression de l'element, soit du tableau, soit supprimant son chainage
			if (pAssoc == pHeadAssoc)
				pvGDAssocs.SetAt(nHash, pAssoc->pNext);
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

void GenericDictionary::GenericGetNextAssoc(POSITION& rNextPosition, GenericKey& genericKey,
					    GenericValue& genericValue) const
{
	GDAssoc* pAssocRet;
	int nBucket;
	GDAssoc* pAssocNext;

	require(pvGDAssocs.GetSize() != 0);
	require(rNextPosition != NULL);

	pAssocRet = (GDAssoc*)rNextPosition;
	assert(pAssocRet != NULL);

	if (pAssocRet == (GDAssoc*)BEFORE_START_POSITION)
	{
		// Recherche de la premiere association
		for (nBucket = 0; nBucket < pvGDAssocs.GetSize(); nBucket++)
			if ((pAssocRet = (GDAssoc*)pvGDAssocs.GetAt(nBucket)) != NULL)
				break;
		assert(pAssocRet != NULL);
	}

	// Recherche de l'association suivante
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// Passage au prochain bucket
		for (nBucket = pAssocRet->nHashValue + 1; nBucket < pvGDAssocs.GetSize(); nBucket++)
			if ((pAssocNext = (GDAssoc*)pvGDAssocs.GetAt(nBucket)) != NULL)
				break;
	}

	rNextPosition = (POSITION)pAssocNext;

	// Initialisation des resultats
	genericKey = pAssocRet->key;
	genericValue = pAssocRet->value;
}

void GenericDictionary::GenericCopyFrom(const GenericDictionary* gdSource)
{
	POSITION current;
	GenericKey genericKey;
	GenericValue genericValue;

	require(gdSource != NULL);

	// Cas particulier ou source egale cible
	if (gdSource == this)
		return;

	// Nettoyage
	RemoveAll();

	// Recopie du contenu du dictionnaire source
	current = gdSource->GenericGetStartPosition();
	while (current != NULL)
	{
		gdSource->GenericGetNextAssoc(current, genericKey, genericValue);
		GenericSetAt(genericKey, genericValue);
	}
}

void GenericDictionary::ReinitHashTable(int nNewHashSize)
{
	PointerVector pvNewGDAssocs;
	int i;
	int nHash;
	GDAssoc* pAssoc;
	GDAssoc* pAssocNext;

	// Creation d'une nouvelle table de hashage
	pvNewGDAssocs.SetSize(nNewHashSize);

	// Recalcul des caracteristiques des elements par rapport a la nouvelle taille de la table de hashage
	for (i = 0; i < pvGDAssocs.GetSize(); i++)
	{
		for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(i); pAssoc != NULL;)
		{
			// Sauvegarde du suivant
			pAssocNext = pAssoc->pNext;

			// Reconversion de pAssoc, vers la nouvelle table de hashage
			nHash = HashKey(pAssoc->key) % nNewHashSize;
			pAssoc->nHashValue = nHash;
			pAssoc->pNext = (GDAssoc*)pvNewGDAssocs.GetAt(nHash);
			pvNewGDAssocs.SetAt(nHash, pAssoc);

			// Passage au suivant
			pAssoc = pAssocNext;
		}
	}

	// Echange du contenu entre l'ancienne et la nouvelle table de hashage
	// Le contenu de la nouvelle table de hashage courante est transferer dans la table courante
	// Le contenu de l'ancienne table de hashage, transfere dans la variable locale pvNewGDAssocs, est detruit
	pvGDAssocs.SwapFrom(&pvNewGDAssocs);
}

GDAssoc* GenericDictionary::NewAssoc()
{
	if (pFreeList == NULL)
	{
		// Ajout d'un nouveau bloc
		CPlex* newBlock = CPlex::Create(pBlocks, nBlockSize, sizeof(GDAssoc));
		// Chainage dans la liste des bloc libres
		GDAssoc* pAssoc = (GDAssoc*)newBlock->data();
		// Chainage en ordre inverse pour faciliter le debuggage
		pAssoc += nBlockSize - 1;
		for (int i = nBlockSize - 1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = pFreeList;
			pFreeList = pAssoc;
		}
	}
	assert(pFreeList != NULL);

	GDAssoc* pAssoc = pFreeList;
	pFreeList = pFreeList->pNext;
	nCount++;
	assert(nCount > 0);
	pAssoc->key.genericKey = 0;
	pAssoc->value.genericValue = 0;

	return pAssoc;
}

void GenericDictionary::FreeAssoc(GDAssoc* pAssoc)
{
	if (bIsStringKey)
		DeleteCharArray(pAssoc->key.sKey);
	pAssoc->key.genericKey = 0;
	pAssoc->pNext = pFreeList;
	pFreeList = pAssoc;
	nCount--;
	assert(nCount >= 0);
}

GDAssoc* GenericDictionary::GetAssocAt(GenericKey genericKey, UINT& nHash) const
{
	GDAssoc* pAssoc;

	require(genericKey.genericKey != 0 or not bIsStringKey);

	// Cas particulier d'un dictionnaire vide
	if (pvGDAssocs.GetSize() == 0)
	{
		assert(nCount == 0);

		// Calcul de sa cle par rapport a la premiere taille de dictionnaire
		nHash = HashKey(genericKey) % DictionaryGetNextTableSize(1);
		return NULL;
	}

	// Calcul de la cle par rapport a la taille courante de la table de hashage
	nHash = HashKey(genericKey) % pvGDAssocs.GetSize();

	// Recherche si elle existe
	for (pAssoc = (GDAssoc*)pvGDAssocs.GetAt(nHash); pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (bIsStringKey)
		{
			assert(pAssoc->key.genericKey != 0);
			if (strcmp(pAssoc->key.sKey, genericKey.sKey) == 0)
				return pAssoc;
		}
		else
		{
			if (pAssoc->key.genericKey == genericKey.genericKey)
				return pAssoc;
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ObjectDictionary::CopyFrom(const ObjectDictionary* odSource)
{
	GenericCopyFrom(odSource);
}

ObjectDictionary* ObjectDictionary::Clone() const
{
	ObjectDictionary* odClone;

	odClone = new ObjectDictionary;
	odClone->CopyFrom(this);
	return odClone;
}

const ALString ObjectDictionary::GetClassLabel() const
{
	return "Dictionary";
}

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
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 10000000, 100000);
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
	cout << "SYS TIME\tObjectDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
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
	cout << "SYS TIME\tObjectDictionary remove\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void NumericKeyDictionary::CopyFrom(const NumericKeyDictionary* odSource)
{
	GenericCopyFrom(odSource);
}

NumericKeyDictionary* NumericKeyDictionary::Clone() const
{
	NumericKeyDictionary* odClone;

	odClone = new NumericKeyDictionary;
	odClone->CopyFrom(this);
	return odClone;
}

const ALString NumericKeyDictionary::GetClassLabel() const
{
	return "Numeric key dictionary";
}

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
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 10000000, 1000000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	// Creation d'un grand tableau de cles numeriques toutes differentes
	oaLargeNumericKeyArray.SetSize(nMaxSize);
	for (i = 0; i < oaLargeNumericKeyArray.GetSize(); i++)
		oaLargeNumericKeyArray.SetAt(i, new SampleObject);
	//
	nkdPerf.RemoveAll();
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
	cout << "SYS TIME\tNumericKeyDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << nkdPerf.GetCount() << "\t" << nkdPerf.GetUsedMemory() << "\t"
	     << nkdPerf.GetOverallUsedMemory() << endl;

	/////
	cout << "Test de performance avec des entiers\n";
	nMaxSize = AcquireRangedInt("Nombre maxi d'entiers inseres (Random)", 1, 10000000, 1000000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	//
	nkdPerf.RemoveAll();
	nStartClock = clock();
	for (int nIter = 0; nIter < nNbIter; nIter++)
	{
		// Insertions
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			numPerf = RandomInt(nMaxSize - 1);
			nkdPerf.SetAt(numPerf, &soTest);
		}
		cout << "\tIteration " << nIter << "\tSize = " << nkdPerf.GetCount();

		// Recherches
		nFound = 0;
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			numPerf = nInsert;
			if (nkdPerf.Lookup(numPerf) != NULL)
				nFound++;
		}
		cout << "\tFound = " << nFound << "\n";
	}
	nStopClock = clock();
	cout << "SYS TIME\tNumericKeyDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << nkdPerf.GetCount() << "\t" << nkdPerf.GetUsedMemory() << "\t"
	     << nkdPerf.GetOverallUsedMemory() << endl;

	// Nettoyage
	oaSmallNumericKeyArray.DeleteAll();
	oaLargeNumericKeyArray.DeleteAll();
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void LongintDictionary::CopyFrom(const LongintDictionary* lidSource)
{
	GenericCopyFrom(lidSource);
}

LongintDictionary* LongintDictionary::Clone() const
{
	LongintDictionary* lidClone;

	lidClone = new LongintDictionary;
	lidClone->CopyFrom(this);
	return lidClone;
}

const ALString LongintDictionary::GetClassLabel() const
{
	return "Longint dictionary ";
}

void LongintDictionary::Test()
{
	LongintDictionary ldTest;
	LongintDictionary ldPerf;
	ObjectArray oaConverted;
	ObjectList olConverted;
	ALString sKey;
	ALString sPerf;
	int i;
	int nMaxSize;
	int nNb;
	int nNbIter;
	int nInsert;
	int nFound;
	int nIter;
	int nStartClock;
	int nStopClock;
	longint lTotal;
	POSITION position;
	longint lValue;
	longint lInitialHeapMemory;
	longint lFinalHeapMemory;
	ObjectDictionary odPerf;
	LongintObject* loCount;
	Object* oValue;
	int nRemovedKeyNumber;

	/////
	cout << "Test des fonctionnalites de base\n";
	//
	cout << "\tInsertion de 10 valeurs\n";
	for (i = 0; i < 10; i++)
	{
		ldTest.SetAt(IntToString(i), i);
	}
	cout << ldTest;

	//
	cout << "\tTest d'existence\n";
	for (i = 8; i < 11; i++)
	{
		sKey = IntToString(i);
		cout << "\t\tLookup (" << sKey << "): " << ldTest.Lookup(sKey) << "\n";
	}

	//
	cout << "\tLoop of clean and decrement\n";
	while (ldTest.GetCount() > 0)
	{
		cout << "\t\t(" << ldTest.GetCount() << ", " << ldTest.ComputeTotalValue() << ")";
		nRemovedKeyNumber = ldTest.RemoveAllNullValues();
		cout << "\t[-" << nRemovedKeyNumber << "]\t";
		cout << "\t(" << ldTest.GetCount() << ", " << ldTest.ComputeTotalValue() << ")";
		ldTest.UpgradeAll(-1);
		cout << " ->\t(" << ldTest.GetCount() << ", " << ldTest.ComputeTotalValue() << ")\n";
	}

	//
	cout << "Insertion puis supression d'un element temporaire a detruire\n";
	sKey = "Temporary";
	ldTest.SetAt(sKey, -1);
	cout << "Inserted: " << ldTest.Lookup("Temporary") << endl;
	ldTest.RemoveKey(sKey);
	cout << "Removed: " << ldTest.Lookup("Temporary") << endl;

	/////
	cout << "Test de changement de taille dynamique\n";
	nMaxSize = AcquireRangedInt("Nombre d'elements inseres)", 1, 100000, 1000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 100000, 1000);

	nStartClock = clock();
	for (nNb = 0; nNb < nNbIter; nNb++)
	{
		ldPerf.RemoveAll();
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			sPerf = IntToString(nInsert);
			ldPerf.SetAt(sPerf, nInsert);
		}
	}
	nStopClock = clock();
	cout << "  HashTableSize = " << ldPerf.GetHashTableSize() << "\n";
	cout << "SYS TIME\tLongintDictionary change size\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n";

	/////
	cout << "Test de performance de base\n";
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 10000000, 100000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	ldPerf.RemoveAll();
	nStartClock = clock();
	for (nIter = 0; nIter < nNbIter; nIter++)
	{
		// Insertions
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			sPerf = IntToString(RandomInt(nMaxSize - 1));
			ldPerf.SetAt(sPerf, 1);
		}
		cout << "\tIteration " << nIter << "\tSize = " << ldPerf.GetCount();

		// Recherches
		nFound = 0;
		for (nInsert = 0; nInsert <= nMaxSize; nInsert++)
		{
			sPerf = IntToString(nInsert);
			if (ldPerf.Lookup(sPerf) != 0)
				nFound++;
		}
		cout << "\tFound = " << nFound << "\n";
	}
	nStopClock = clock();
	cout << "SYS TIME\tLongintDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << ldPerf.GetCount() << "\t" << ldPerf.GetUsedMemory() << "\t"
	     << ldPerf.GetOverallUsedMemory() << endl;

	////////////////////////////////////////////////////////////////////////////////
	cout << "\nTest de performance de comptage\n";
	nMaxSize = AcquireRangedInt("Nombre max d'elements", 1, 10000000, 1000000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 100000000, 10000000);

	ldPerf.RemoveAll();
	lInitialHeapMemory = MemGetHeapMemory();
	nStartClock = clock();
	// Insertions
	for (nInsert = 0; nInsert < nNbIter; nInsert++)
	{
		sPerf = IntToString(RandomInt(nMaxSize - 1));
		ldPerf.UpgradeAt(sPerf, 1);
	}
	// Comptage
	lTotal = 0;
	position = ldPerf.GetStartPosition();
	while (position != NULL)
	{
		ldPerf.GetNextAssoc(position, sKey, lValue);
		lTotal += lValue;
	}
	assert(lTotal == nNbIter);
	cout << "\tTotal = " << lTotal << "\n";
	nStopClock = clock();
	lFinalHeapMemory = MemGetHeapMemory();
	cout << "SYS TIME\tLongintDictionary counts\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << ldPerf.GetCount() << "\t" << ldPerf.GetUsedMemory() << "\t"
	     << ldPerf.GetOverallUsedMemory() << "\t" << lFinalHeapMemory - lInitialHeapMemory << endl;

	/////
	cout << "\nTest de performance de comptage via un dictionnaire de LongintObject\n";
	ldPerf.RemoveAll();
	odPerf.RemoveAll();
	lInitialHeapMemory = MemGetHeapMemory();
	nStartClock = clock();
	// Insertions
	for (nInsert = 0; nInsert < nNbIter; nInsert++)
	{
		sPerf = IntToString(RandomInt(nMaxSize - 1));
		loCount = cast(LongintObject*, odPerf.Lookup(sPerf));
		if (loCount == NULL)
		{
			loCount = new LongintObject;
			loCount->SetLongint(1);
			odPerf.SetAt(sPerf, loCount);
		}
		else
			loCount->SetLongint(loCount->GetLongint() + 1);
	}
	// Comptage
	lTotal = 0;
	position = odPerf.GetStartPosition();
	while (position != NULL)
	{
		odPerf.GetNextAssoc(position, sKey, oValue);
		loCount = cast(LongintObject*, oValue);
		lTotal += loCount->GetLongint();
	}
	assert(lTotal == nNbIter);
	cout << "\tTotal = " << lTotal << "\n";
	nStopClock = clock();
	lFinalHeapMemory = MemGetHeapMemory();
	cout << "SYS TIME\tLongintDictionary counts\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << odPerf.GetCount() << "\t" << odPerf.GetUsedMemory() << "\t"
	     << odPerf.GetOverallUsedMemory() << "\t" << lFinalHeapMemory - lInitialHeapMemory << endl;
	odPerf.DeleteAll();
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void LongintNumericKeyDictionary::CopyFrom(const LongintNumericKeyDictionary* linkdSource)
{
	GenericCopyFrom(linkdSource);
}

LongintNumericKeyDictionary* LongintNumericKeyDictionary::Clone() const
{
	LongintNumericKeyDictionary* linkdClone;

	linkdClone = new LongintNumericKeyDictionary;
	linkdClone->CopyFrom(this);
	return linkdClone;
}

const ALString LongintNumericKeyDictionary::GetClassLabel() const
{
	return "Longint numeric key dictionary ";
}

void LongintNumericKeyDictionary::Test()
{
	LongintNumericKeyDictionary lnkdTest;
	LongintNumericKeyDictionary lnkdPerf;
	ObjectArray oaConverted;
	ObjectList olConverted;
	NUMERIC liKey;
	NUMERIC liPerf;
	int i;
	int nMaxSize;
	int nNb;
	int nNbIter;
	int nInsert;
	int nFound;
	int nIter;
	int nStartClock;
	int nStopClock;
	longint lTotal;
	POSITION position;
	longint lValue;
	longint lInitialHeapMemory;
	longint lFinalHeapMemory;
	NumericKeyDictionary nkdPerf;
	LongintObject* loCount;
	Object* oValue;
	int nRemovedKeyNumber;

	/////
	cout << "Test des fonctionnalites de base\n";
	//
	cout << "\tInsertion de 10 valeurs\n";
	for (i = 0; i < 10; i++)
	{
		lnkdTest.SetAt(i, i);
	}
	cout << lnkdTest;

	//
	cout << "\tTest d'existence\n";
	for (i = 8; i < 11; i++)
	{
		liKey = i;
		cout << "\t\tLookup (" << liKey << "): " << lnkdTest.Lookup(liKey) << "\n";
	}

	//
	cout << "\tLoop of clean and decrement\n";
	while (lnkdTest.GetCount() > 0)
	{
		cout << "\t\t(" << lnkdTest.GetCount() << ", " << lnkdTest.ComputeTotalValue() << ")";
		nRemovedKeyNumber = lnkdTest.RemoveAllNullValues();
		cout << "\t[-" << nRemovedKeyNumber << "]\t";
		cout << "\t(" << lnkdTest.GetCount() << ", " << lnkdTest.ComputeTotalValue() << ")";
		lnkdTest.UpgradeAll(-1);
		cout << " ->\t(" << lnkdTest.GetCount() << ", " << lnkdTest.ComputeTotalValue() << ")\n";
	}

	//
	cout << "Insertion puis supression d'un element temporaire a detruire\n";
	liKey = -1;
	lnkdTest.SetAt(liKey, -1);
	cout << "Inserted " << liKey << ": " << lnkdTest.Lookup(liKey) << endl;
	lnkdTest.RemoveKey(liKey);
	cout << "Removed " << liKey << ": " << lnkdTest.Lookup(liKey) << endl;

	/////
	cout << "Test de changement de taille dynamique\n";
	nMaxSize = AcquireRangedInt("Nombre d'elements inseres)", 1, 100000, 1000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 100000, 1000);

	nStartClock = clock();
	for (nNb = 0; nNb < nNbIter; nNb++)
	{
		lnkdPerf.RemoveAll();
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			liPerf = nInsert;
			lnkdPerf.SetAt(liPerf, nInsert);
		}
	}
	nStopClock = clock();
	cout << "  HashTableSize = " << lnkdPerf.GetHashTableSize() << "\n";
	cout << "SYS TIME\tLongintNumericKeyDictionary change size\t"
	     << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";

	/////
	cout << "Test de performance de base\n";
	nMaxSize = AcquireRangedInt("Nombre maxi d'elements inseres (Random)", 1, 10000000, 100000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 1000, 20);

	lnkdPerf.RemoveAll();
	nStartClock = clock();
	for (nIter = 0; nIter < nNbIter; nIter++)
	{
		// Insertions
		for (nInsert = 0; nInsert < nMaxSize; nInsert++)
		{
			liPerf = RandomInt(nMaxSize - 1);
			lnkdPerf.SetAt(liPerf, 1);
		}
		cout << "\tIteration " << nIter << "\tSize = " << lnkdPerf.GetCount();

		// Recherches
		nFound = 0;
		for (nInsert = 0; nInsert <= nMaxSize; nInsert++)
		{
			liPerf = nInsert;
			if (lnkdPerf.Lookup(liPerf) != 0)
				nFound++;
		}
		cout << "\tFound = " << nFound << "\n";
	}
	nStopClock = clock();
	cout << "SYS TIME\tLongintNumericKeyDictionary access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << lnkdPerf.GetCount() << "\t" << lnkdPerf.GetUsedMemory() << "\t"
	     << lnkdPerf.GetOverallUsedMemory() << endl;

	////////////////////////////////////////////////////////////////////////////////
	cout << "\nTest de performance de comptage\n";
	nMaxSize = AcquireRangedInt("Nombre max d'elements", 1, 10000000, 1000000);
	nNbIter = AcquireRangedInt("Nombre d'iterations", 1, 100000000, 10000000);

	lnkdPerf.RemoveAll();
	lInitialHeapMemory = MemGetHeapMemory();
	nStartClock = clock();
	// Insertions
	for (nInsert = 0; nInsert < nNbIter; nInsert++)
	{
		liPerf = RandomInt(nMaxSize - 1);
		lnkdPerf.UpgradeAt(liPerf, 1);
	}
	// Comptage
	lTotal = 0;
	position = lnkdPerf.GetStartPosition();
	while (position != NULL)
	{
		lnkdPerf.GetNextAssoc(position, liPerf, lValue);
		lTotal += lValue;
	}
	assert(lTotal == nNbIter);
	cout << "\tTotal = " << lTotal << "\n";
	nStopClock = clock();
	lFinalHeapMemory = MemGetHeapMemory();
	cout << "SYS TIME\tLongintNumericKeyDictionary counts\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << lnkdPerf.GetCount() << "\t" << lnkdPerf.GetUsedMemory() << "\t"
	     << lnkdPerf.GetOverallUsedMemory() << "\t" << lFinalHeapMemory - lInitialHeapMemory << endl;

	/////
	cout << "\nTest de performance de comptage via un dictionnaire de LongintObject\n";
	lnkdPerf.RemoveAll();
	nkdPerf.RemoveAll();
	lInitialHeapMemory = MemGetHeapMemory();
	nStartClock = clock();
	// Insertions
	for (nInsert = 0; nInsert < nNbIter; nInsert++)
	{
		liPerf = RandomInt(nMaxSize - 1);
		loCount = cast(LongintObject*, nkdPerf.Lookup(liPerf));
		if (loCount == NULL)
		{
			loCount = new LongintObject;
			loCount->SetLongint(1);
			nkdPerf.SetAt(liPerf, loCount);
		}
		else
			loCount->SetLongint(loCount->GetLongint() + 1);
	}
	// Comptage
	lTotal = 0;
	position = nkdPerf.GetStartPosition();
	while (position != NULL)
	{
		nkdPerf.GetNextAssoc(position, liPerf, oValue);
		loCount = cast(LongintObject*, oValue);
		lTotal += loCount->GetLongint();
	}
	assert(lTotal == nNbIter);
	cout << "\tTotal = " << lTotal << "\n";
	nStopClock = clock();
	lFinalHeapMemory = MemGetHeapMemory();
	cout << "SYS TIME\tLongintNumericKeyDictionary counts\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n";
	cout << "SYS MEMORY\tUsed memory\t" << nkdPerf.GetCount() << "\t" << nkdPerf.GetUsedMemory() << "\t"
	     << nkdPerf.GetOverallUsedMemory() << "\t" << lFinalHeapMemory - lInitialHeapMemory << endl;
	nkdPerf.DeleteAll();
}
