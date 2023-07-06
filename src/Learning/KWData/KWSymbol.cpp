// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSymbol.h"

int* Symbol::operator*() const
{
	assert(false);
	return (int*)NULL;
}

longint Symbol::GetUsedMemory() const
{
	return GetUsedMemoryPerSymbol() + GetLength();
}

Symbol Symbol::BuildNewSymbol(const char* sBaseName)
{
	int nId;
	ALString sNewSymbol;

	require(sBaseName != NULL);

	// Boucle de recherche d'un symbol inexistant
	sNewSymbol = sBaseName;
	nId = 0;
	while (sdSharedSymbols.Lookup(sNewSymbol) != NULL)
	{
		// Construction d'une nouvelle valeur
		nId++;
		sNewSymbol = sBaseName;
		sNewSymbol += IntToString(nId);
	}
	assert(sdSharedSymbols.Lookup(sNewSymbol) == NULL);

	return Symbol(sNewSymbol);
}

int Symbol::GetSymbolNumber()
{
	int nSymbolNumber;
	nSymbolNumber = sdSharedSymbols.GetCount();
	return nSymbolNumber;
}

longint Symbol::GetAllSymbolsUsedMemory()
{
	longint lUsedMemory;
	lUsedMemory = sdSharedSymbols.GetUsedMemory();
	return lUsedMemory;
}

longint Symbol::GetUsedMemoryPerSymbol()
{
	longint lUsedMemory;
	// On compte la taille pour le Symbol, plus un pointeur dans la table de hashage du dictionnairee de symboles
	lUsedMemory = sizeof(KWSymbolDataPtr) + sizeof(KWSymbolData);
	return lUsedMemory;
}

void Symbol::Test()
{
	static Symbol sSpecialStar = Symbol::GetStarValue();
	static Symbol sStar = " * ";
	Symbol sEmpty;
	Symbol sToto = "Toto";
	Symbol sTotoBis = sToto;
	Symbol sTest;
	int i;
	int nIndex;
	const int nIterNumber = 10000000;
	const int nMaxCreationNumber = 2 + nIterNumber / 100;
	SymbolVector symbolVector;
	clock_t tBegin;
	clock_t tEnd;
	int nCompareNumber;
	ALString sTmp;

	// Test du dictionnaire de symboles
	KWSymbolDictionary::Test();

	// Taille des classes concernees
	cout << "SYSTEM\tsizeof(Symbol): " << sizeof(Symbol) << endl;
	cout << "SYSTEM\tsizeof(KWSymbolData): " << sizeof(KWSymbolData) << endl << endl;

	// Valeurs speciale
	cout << "Star: " << sStar << endl;
	cout << "SpecialStar: " << sSpecialStar << endl;
	if (sStar == sSpecialStar)
		cout << "\tSymbols identical" << endl;
	else
		cout << "\tSymbols different" << endl;

	// Creation du tableau de Symbol a creer
	symbolVector.SetSize(nMaxCreationNumber);

	// Test de contenu
	symbolVector.SetAt(1, "titi");
	cout << "sEmpty: " << sEmpty << " (" << sEmpty.GetLength() << ")" << endl;
	cout << "sToto: " << sToto << " (" << sToto.GetLength() << ")" << endl;
	cout << "sTotoBis: " << sTotoBis << " (" << sTotoBis.GetLength() << ")" << endl;
	cout << "createdSymbolArray[0]: " << symbolVector.GetAt(0) << " (" << symbolVector.GetAt(0).GetLength() << ")"
	     << endl;
	cout << "createdSymbolArray[1]: " << symbolVector.GetAt(1) << " (" << symbolVector.GetAt(1).GetLength() << ")"
	     << endl;

	// Performances en creation
	cout << "Performance test: " << nIterNumber / 10 << " creations" << endl;
	tBegin = clock();
	for (i = 0; i < nIterNumber / 10; i++)
	{
		nIndex = RandomInt(nMaxCreationNumber - 1);
		symbolVector.SetAt(nIndex, IntToString(nIndex));
	}
	tEnd = clock();
	cout << "TIME\t" << (double)(tEnd - tBegin) / CLOCKS_PER_SEC << endl;
	cout << "Total Symbol Number: " << sdSharedSymbols.GetCount() << endl;

	// Reaffectation des symbols
	sEmpty = "";
	sToto = "Toto";
	sTotoBis = sTmp + sToto + "Bis";

	// Performances en affectation
	cout << "Performance test: " << 5 * nIterNumber << " affectations" << endl;
	tBegin = clock();
	for (i = 0; i < nIterNumber; i++)
	{
		sTest = sEmpty;
		sTest = sTotoBis;
		sTest = "";
		sTest = sToto;
		sTest = sTotoBis;
	}
	tEnd = clock();
	cout << "TIME\t" << (double)(tEnd - tBegin) / CLOCKS_PER_SEC << endl;

	// Reaffectation des symbols
	sEmpty = "";
	sToto = "Toto";
	sTotoBis = sTmp + sToto + "Bis";
	cout << "Total Symbol Number: " << sdSharedSymbols.GetCount() << endl;

	// Performances en comparaisons
	cout << "Performance test: " << 5 * nIterNumber << " comparaisons" << endl;
	nCompareNumber = 0;
	tBegin = clock();
	for (i = 0; i < nIterNumber; i++)
	{
		if (sTotoBis == sEmpty)
			nCompareNumber++;
		if (sTotoBis == sToto)
			nCompareNumber++;
		if (sTotoBis == sTotoBis)
			nCompareNumber++;
		if (sTotoBis == sStar)
			nCompareNumber++;
		if (sTotoBis == sSpecialStar)
			nCompareNumber++;
	}
	tEnd = clock();
	cout << "TIME\t" << (double)(tEnd - tBegin) / CLOCKS_PER_SEC << endl;
	cout << "Compare OK Number: " << nCompareNumber << endl;
}

// Dictionnaire partage des KWSymbolData
KWSymbolDictionary Symbol::sdSharedSymbols;

/////////////////////////////////////////////
// Implementation de la classe SymbolObject

SymbolObject* SymbolObject::Clone() const
{
	SymbolObject* soClone;

	soClone = new SymbolObject;
	soClone->SetSymbol(GetSymbol());
	return soClone;
}

longint SymbolObject::GetUsedMemory() const
{
	return sizeof(SymbolObject);
}

const ALString SymbolObject::GetClassLabel() const
{
	return "Categorical object";
}

int SymbolObjectCompare(const void* elem1, const void* elem2)
{
	return cast(SymbolObject*, *(Object**)elem1)
	    ->GetSymbol()
	    .Compare(cast(SymbolObject*, *(Object**)elem2)->GetSymbol());
}

/////////////////////////////////////////////
// Implementation de la classe SymbolVector

SymbolVector::~SymbolVector()
{
	// Attention: on force prealablement une initialisation afin de dereferencer les symbols
	Initialize();

	// Destruction du vecteur de grande taille
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void SymbolVector::SetSize(int nValue)
{
	int i;

	require(nValue >= 0);
	require((unsigned)nValue <= INT_MAX / sizeof(Symbol));

	// Attention: on force le deferencement des valeurs Symbol en trop
	// Cas des petites tailles
	if (nAllocSize <= nBlockSize)
	{
		for (i = nValue; i < nSize; i++)
			pData.pValues[i].Reset();
	}
	// Cas des grandes tailles
	else
	{
		for (i = nValue; i < nSize; i++)
			(pData.pValueBlocks[i / nBlockSize])[i % nBlockSize].Reset();
	}

	// Retaillage du vecteur de grande taille
	// Une recopie directe des valeurs ne pose pas de probleme, puisque le compteur de reference
	// des symbol est inchange entre avant et apres le retaillage
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void SymbolVector::Initialize()
{
	int i;

	// Attention: on force explicitement le deferencement des valeurs Symbol en trop
	// Attention: on force le deferencement des valeurs Symbol en trop
	// Cas des petites tailles
	if (nAllocSize <= nBlockSize)
	{
		for (i = 0; i < nSize; i++)
			pData.pValues[i].Reset();
	}
	// Cas des grandes tailles
	else
	{
		for (i = 0; i < nSize; i++)
			(pData.pValueBlocks[i / nBlockSize])[i % nBlockSize].Reset();
	}
}

void SymbolVector::Shuffle()
{
	int i;
	int iSwap;
	Symbol sSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		sSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, sSwappedValue);
	}
}

void SymbolVector::CopyFrom(const SymbolVector* svSource)
{
	int i;

	require(svSource != NULL);

	// Cas particulier ou source egale cible
	if (svSource == this)
		return;

	// Retaillage
	SetSize(svSource->GetSize());

	// Attention: recopie explicite des valeurs sources pour gerer le dereferencement des valeurs Symbol
	if (nSize > 0)
	{
		for (i = 0; i < nSize; i++)
			SetAt(i, svSource->GetAt(i));
	}
}

SymbolVector* SymbolVector::Clone() const
{
	SymbolVector* svClone;

	svClone = new SymbolVector;

	// Recopie
	svClone->CopyFrom(this);
	return svClone;
}

boolean SymbolVector::SetLargeSize(int nValue)
{
	int i;

	// Attention: on force le deferencement des valeurs Symbol en trop
	// Attention: on force le deferencement des valeurs Symbol en trop
	// Cas des petites tailles
	if (nAllocSize <= nBlockSize)
	{
		for (i = nValue; i < nSize; i++)
			pData.pValues[i].Reset();
	}
	// Cas des grandes tailles
	else
	{
		for (i = nValue; i < nSize; i++)
			(pData.pValueBlocks[i / nBlockSize])[i % nBlockSize].Reset();
	}

	// Retaillage du vecteur de grande taille
	// Une recopie directe des valeurs ne pose pas de probleme, puisque le compteur de reference
	// des symbol est inchange entre avant et apres le retaillage
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

int SymbolVectorCompareValue(const void* elem1, const void* elem2)
{
	Symbol* sValue1;
	Symbol* sValue2;

	// Acces aux valeurs
	sValue1 = (Symbol*)elem1;
	sValue2 = (Symbol*)elem2;

	// Comparaison
	return sValue1->CompareValue(*sValue2);
}

void SymbolVector::SortValues()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, SymbolVectorCompareValue);
}

int SymbolVectorCompareKey(const void* elem1, const void* elem2)
{
	Symbol* sValue1;
	Symbol* sValue2;

	// Acces aux valeurs
	sValue1 = (Symbol*)elem1;
	sValue2 = (Symbol*)elem2;

	// Comparaison
	return sValue1->Compare(*sValue2);
}

void SymbolVector::SortKeys()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, SymbolVectorCompareKey);
}

void SymbolVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;
	int i;

	ost << GetClassLabel() + " (size=" << GetSize() << ")\n";
	for (i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t" << GetAt(i).GetValue() << "\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint SymbolVector::GetUsedMemory() const
{
	return sizeof(SymbolVector) + nAllocSize * sizeof(Symbol);
}

const ALString SymbolVector::GetClassLabel() const
{
	return "Symbol vector";
}

void SymbolVector::Test()
{
	ALString sTmp;
	SymbolVector* svTest;
	SymbolVector* svClone;
	int nSize;
	int i;

	// Creation
	nSize = AcquireRangedInt("Size of vector", 0, 100000, 1000);
	svTest = new SymbolVector;
	svTest->SetSize(nSize);
	cout << *svTest << endl;

	// Remplissage avec des symboles numerotes
	cout << "Fill with indexed symbols" << endl;
	for (i = 0; i < svTest->GetSize(); i++)
		svTest->SetAt(i, Symbol(sTmp + "S" + IntToString(i + 1)));
	cout << *svTest << endl;

	// Duplication
	cout << "Duplicate" << endl;
	svClone = svTest->Clone();
	cout << *svClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("New size of vector, keeping the values", 0, 100000, 1000);
	svTest->SetSize(nSize);
	cout << *svTest << endl;

	// Retaillage (une deuxieme fois)
	nSize = AcquireRangedInt("New size of vector, keeping the values", 0, 100000, 1000);
	svTest->SetSize(nSize);
	cout << *svTest << endl;

	// Retaillage avec reinitialisation
	nSize = AcquireRangedInt("New size of vector", 0, 100000, 1000);
	svTest->SetSize(nSize);
	svTest->Initialize();
	cout << *svTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Number of resizings", 0, 1000000, 1000);
	svTest->SetSize(0);
	for (i = 0; i < nSize; i++)
		svTest->Add(Symbol("S0"));
	cout << *svTest << endl;

	// Liberations
	delete svClone;
	delete svTest;
}

int SymbolVectorCompare(const void* elem1, const void* elem2)
{
	SymbolVector* sv1;
	SymbolVector* sv2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux vacteurs la parties
	sv1 = cast(SymbolVector*, *(Object**)elem1);
	sv2 = cast(SymbolVector*, *(Object**)elem2);

	// Comparaison selon la taille
	nDiff = -(sv1->GetSize() - sv2->GetSize());

	// Comparaison sur la premier valeur en cas d'egalite
	if (nDiff == 0 and sv1->GetSize() > 0)
	{
		nDiff = sv1->GetAt(0).CompareValue(sv2->GetAt(0));
	}
	return nDiff;
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

static KWSymbolData KWSymbolDataStarValue;

KWSymbolData* KWSymbolData::InitStarValue()
{
	const char* sStar = " * ";
	const int nStarValueLength = 3;

	KWSymbolDataStarValue.lRefCount = 1;
	KWSymbolDataStarValue.nHashValue = 0;
	KWSymbolDataStarValue.nLength = nStarValueLength;
	KWSymbolDataStarValue.pNext = NULL;
	assert(strlen(sStar) == nStarValueLength);
	assert(nStarValueLength < KWSymbolData::nMinStringSize);
	memcpy(&(KWSymbolDataStarValue.cFirstStringChar), sStar, nStarValueLength);
	KWSymbolDataStarValue.cFirstStringChar[nStarValueLength] = '\0';
	return &KWSymbolDataStarValue;
}

inline KWSymbolData* KWSymbolData::NewSymbolData(const char* sValue, int nLength)
{
	KWSymbolData* pSymbolData;
	int nMemorySize;

	require(sValue != NULL);
	require(nLength == (int)strlen(sValue));

	// Calcul de la taille a allouer (en prevoyant le '\0' de fin de chaine de caracteres)
	if (nLength < KWSymbolData::nMinStringSize)
		nMemorySize = sizeof(KWSymbolData);
	else
		nMemorySize = sizeof(KWSymbolData) + nLength + 1 - KWSymbolData::nMinStringSize;

	// Creation des donnees d'un symbol
	// On alloue le nombre de caracteres necessaire pour stocker la chaine de caracateres
	// en plus des donnees du symbol (le caractere fin de chaine '\0' est deja prevu)
	pSymbolData = (KWSymbolData*)NewMemoryBlock(nMemorySize);
	pSymbolData->lRefCount = 0;
	pSymbolData->nLength = nLength;

	// Recopie de la chaine de caracteres ('\0' en fin de chaine)
	memcpy(&(pSymbolData->cFirstStringChar), sValue, nLength + 1);
	return pSymbolData;
}

inline void KWSymbolData::DeleteSymbolData(KWSymbolData* pSymbolData)
{
	require(pSymbolData != NULL);
	DeleteMemoryBlock(pSymbolData);
}

KWSymbolDictionary::KWSymbolDictionary()
{
	m_nCount = 0;
}

UINT KWSymbolDictionary::HashKey(const char* key) const
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash << 5) + nHash + *key++;
	return nHash;
}

void KWSymbolDictionary::InitHashTable(int nHashSize)
{
	assert(m_nCount == 0);
	assert(nHashSize > 0);

	pvSymbolDatas.SetSize(nHashSize);
}

void KWSymbolDictionary::ReinitHashTable(int nNewHashSize)
{
	boolean bDisplay = false;
	Timer timer;
	int i;
	int nHashPosition;
	KWSymbolDataPtr pSymbolData;
	KWSymbolDataPtr pSymbolDataNext;
	KWSymbolDataPtr pAllSymbolDatas;

	// Affichage du debut de la methode
	if (bDisplay)
	{
		cout << "Symbol ReinitHashTable (" << GetCount() << "," << GetHashTableSize() << ")";
		cout << " -> " << nNewHashSize << ": " << flush;
		timer.Start();
	}

	// Memorisation de tous les symboles dans une liste chainee
	pAllSymbolDatas = NULL;
	for (i = 0; i < pvSymbolDatas.GetSize(); i++)
	{
		// Acces au symbole courant
		pSymbolData = (KWSymbolDataPtr)pvSymbolDatas.GetAt(i);
		if (pSymbolData != NULL)
		{
			// On met a NULL la position correspondante
			pvSymbolDatas.SetAt(i, NULL);

			// On recherche le dernier symbole chaine
			pSymbolDataNext = pSymbolData;
			while (pSymbolDataNext->pNext != NULL)
				pSymbolDataNext = pSymbolDataNext->pNext;

			// On chaine avec la liste de tous les symboles
			pSymbolDataNext->pNext = pAllSymbolDatas;
			pAllSymbolDatas = pSymbolData;
		}
	}

	// Retaillage de la table de hashage
	pvSymbolDatas.SetSize(nNewHashSize);

	// On reinsere tous les symbole dans la nouvelle taille de hashage
	pSymbolData = pAllSymbolDatas;
	while (pSymbolData != NULL)
	{
		// Sauvegarde du suivant
		pSymbolDataNext = pSymbolData->pNext;

		// Reconversion de pSymbolData, vers la nouvelle table de hashage
		nHashPosition = pSymbolData->nHashValue % nNewHashSize;
		pSymbolData->pNext = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nHashPosition);
		pvSymbolDatas.SetAt(nHashPosition, pSymbolData);

		// Passage au suivant
		pSymbolData = pSymbolDataNext;
	}

	// Affichage de la fin de la methode
	if (bDisplay)
	{
		timer.Stop();
		cout << timer.GetElapsedTime() << endl;
	}
}

void KWSymbolDictionary::ShowAllocErrorMessage(KWSymbolDataPtr symbolData, int nMessageIndex)
{
	const int nMaxMessageNumber = 20;

	require(symbolData != NULL);
	require(nMessageIndex >= 0);

	if (nMessageIndex < nMaxMessageNumber)
	{
		if (GetProcessId() != 0)
			fprintf(stdout, "Process %d: ", GetProcessId());
		fprintf(stdout, "Symbol not free (refCount=%lld, %p, %s)\n", symbolData->lRefCount, symbolData,
			symbolData->GetString());
	}
	if (nMessageIndex == nMaxMessageNumber)
		fprintf(stdout, "...\n");
}

void KWSymbolDictionary::RemoveAll()
{
	boolean bShowAllocErrorMessages;
	ALString sUserName;
	int nHashPosition;
	KWSymbolDataPtr pSymbolData;
	KWSymbolDataPtr pSymbolDataNext;
	int nMessageIndex;

	// Recherche des variables d'environnement
	sUserName = p_getenv("USERNAME");
	sUserName.MakeLower();

	// On ne montre les erreurs de non liberation de Symbol qu'en mode expert
	// pour le dictionnaire global des Symbol et en release, pour ne pas
	// avoir de reporting verbeux systematique en debug
	bShowAllocErrorMessages =
	    (sUserName == "miib6422") and GetLearningExpertMode() and this == &(Symbol::sdSharedSymbols);
	debug(bShowAllocErrorMessages = false);

	// Desactivation de cette option y compris en mode release, car cela provoque du reporting verbeux en cas
	// d'erreur fatale A reactiver si necessaire
	bShowAllocErrorMessages = false;

	// Nettoyage des cles de la table de hashage
	nMessageIndex = 0;
	for (nHashPosition = 0; nHashPosition < pvSymbolDatas.GetSize(); nHashPosition++)
	{
		for (pSymbolData = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nHashPosition); pSymbolData != NULL;)
		{
			// Sauvegarde du suivant
			pSymbolDataNext = pSymbolData->pNext;

			// Affichage des informations sur le symbole non libere
			if (bShowAllocErrorMessages)
			{
				ShowAllocErrorMessage(pSymbolData, nMessageIndex);
				nMessageIndex++;
			}

			// Destruction des donnees du symbol
			KWSymbolData::DeleteSymbolData(pSymbolData);
			debug(m_nCount--);
			debug(assert(m_nCount >= 0));

			// Passage au suivant
			pSymbolData = pSymbolDataNext;
		}
	}

	// Affichage synthetique sur l'enseble des symboles non liberes
	if (bShowAllocErrorMessages)
	{
		if (nMessageIndex > 0)
		{
			if (GetProcessId() != 0)
				fprintf(stdout, "Process %d: ", GetProcessId());
			fprintf(stdout, "Number of Symbol not free: %d\n", nMessageIndex);
		}
	}

	// Reinitialisation du tableau de symbole
	pvSymbolDatas.SetSize(0);
	m_nCount = 0;
}

KWSymbolDictionary::~KWSymbolDictionary()
{
	RemoveAll();
	assert(m_nCount == 0);
}

KWSymbolDataPtr KWSymbolDictionary::GetSymbolDataAt(const char* key, UINT& nHash) const
{
	KWSymbolDataPtr pSymbolData;
	int nHashPosition;

	require(key != NULL);

	// Cas particulier d'un dictionnaire vide
	if (pvSymbolDatas.GetSize() == 0)
	{
		assert(m_nCount == 0);

		// Calcul de sa cle par rapport a la premiere taille de dictionnaire
		nHash = HashKey(key);
		return NULL;
	}

	// Calcul de la cle par rapport a la taille courante de la table de hashage
	nHash = HashKey(key);

	// Regarde si la cle existe
	nHashPosition = nHash % GetHashTableSize();
	for (pSymbolData = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nHashPosition); pSymbolData != NULL;
	     pSymbolData = pSymbolData->pNext)
	{
		assert(pSymbolData->GetString() != NULL);

		// Test de la valeur exacte de la string
		if (strcmp(pSymbolData->GetString(), key) == 0)
			return pSymbolData;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////

KWSymbolDataPtr KWSymbolDictionary::Lookup(const char* key) const
{
	UINT nHash;
	KWSymbolDataPtr pSymbolData = GetSymbolDataAt(key, nHash);
	return pSymbolData;
}

KWSymbolDataPtr KWSymbolDictionary::AsSymbol(const char* key, int nLength)
{
	UINT nHash;
	int nHashPosition;
	KWSymbolDataPtr pSymbolData;

	require(key != NULL);
	require(nLength >= 0);

	pSymbolData = GetSymbolDataAt(key, nHash);
	if (pSymbolData == NULL)
	{
		if (pvSymbolDatas.GetSize() == 0)
		{
			assert(m_nCount == 0);
			InitHashTable(DictionaryGetNextTableSize(1));
		}

		// Creation d'un nouveau Symbol
		pSymbolData = KWSymbolData::NewSymbolData(key, nLength);
		pSymbolData->nHashValue = nHash;
		m_nCount++;
		assert(m_nCount > 0);

		// Ajout dans la table de hashage
		nHashPosition = nHash % GetHashTableSize();
		pSymbolData->pNext = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nHashPosition);
		pvSymbolDatas.SetAt(nHashPosition, pSymbolData);

		// Retaillage dynamique
		if (GetCount() > GetHashTableSize() / 2 and GetHashTableSize() < INT_MAX)
			ReinitHashTable(DictionaryGetNextTableSize(2 * min(GetHashTableSize(), INT_MAX / 2)));
	}
	return pSymbolData;
}

void KWSymbolDictionary::RemoveSymbol(KWSymbolDataPtr symbolData)
{
	KWSymbolDataPtr pHeadSymbolData;
	KWSymbolDataPtr pSymbolDataPrev;
	int nHashPosition;

	require(symbolData != NULL);
	require(Lookup(symbolData->GetString()) == symbolData);

	// Recherche du premier element de la table de hashage correspondant a la cle
	nHashPosition = symbolData->nHashValue % GetHashTableSize();
	pHeadSymbolData = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nHashPosition);

	// Cas de la tete de liste
	if (pHeadSymbolData == symbolData)
		pvSymbolDatas.SetAt(nHashPosition, pHeadSymbolData->pNext);
	// Cas hors tete de liste
	else
	{
		// Parcours de la liste pour rechercher l'element
		pSymbolDataPrev = pHeadSymbolData;
		while (pSymbolDataPrev->pNext != symbolData)
			pSymbolDataPrev = pSymbolDataPrev->pNext;

		// Rechainage en derefencant l'element a supprimer
		pSymbolDataPrev->pNext = pSymbolDataPrev->pNext->pNext;
	}

	// Supression de l'element dereference de la table de hashage
	KWSymbolData::DeleteSymbolData(symbolData);
	m_nCount--;
	assert(m_nCount >= 0);

	// Retaillage dynamique pour recuperer la memoire inutilisee
	assert(GetHashTableSize() < INT_MAX / sizeof(void*));
	if (GetCount() > 20 and GetCount() < GetHashTableSize() / 8)
		ReinitHashTable(DictionaryGetNextTableSize(2 * GetCount()));
}

void KWSymbolDictionary::GetNextSymbolData(POSITION& rNextPosition, KWSymbolDataPtr& sSymbol) const
{
	KWSymbolDataPtr pSymbolDataRet;
	int nBucket;
	KWSymbolDataPtr pSymbolDataNext;

	require(pvSymbolDatas.GetSize() != 0);
	require(rNextPosition != NULL);

	pSymbolDataRet = (KWSymbolDataPtr)rNextPosition;
	assert(pSymbolDataRet != NULL);

	if (pSymbolDataRet == (KWSymbolDataPtr)BEFORE_START_POSITION)
	{
		// Recherche du premier Symbol
		for (nBucket = 0; nBucket < pvSymbolDatas.GetSize(); nBucket++)
			if ((pSymbolDataRet = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nBucket)) != NULL)
				break;
		assert(pSymbolDataRet != NULL);
	}

	// Recherche du Symbol suivant
	if ((pSymbolDataNext = pSymbolDataRet->pNext) == NULL)
	{
		// Passage au prochain bucket
		for (nBucket = (pSymbolDataRet->nHashValue % GetHashTableSize()) + 1; nBucket < pvSymbolDatas.GetSize();
		     nBucket++)
			if ((pSymbolDataNext = (KWSymbolDataPtr)pvSymbolDatas.GetAt(nBucket)) != NULL)
				break;
	}
	rNextPosition = (POSITION)pSymbolDataNext;

	// Initialisation du resultat
	sSymbol = pSymbolDataRet;
}

longint KWSymbolDictionary::GetUsedMemory() const
{
	POSITION current;
	KWSymbolDataPtr sElement;
	longint lUsedMemory;

	// Initialisation de la taille memoire utilisee
	lUsedMemory = sizeof(KWSymbolDictionary);
	lUsedMemory += pvSymbolDatas.GetSize() * sizeof(void*);

	// A jout de la memoire pour tous les symbols
	current = GetStartPosition();
	while (current != NULL)
	{
		GetNextSymbolData(current, sElement);
		lUsedMemory += sizeof(KWSymbolDataPtr) + sizeof(KWSymbolData) + sElement->GetLength();
	}
	return lUsedMemory;
}

void KWSymbolDictionary::Write(ostream& ost) const
{
	POSITION current;
	KWSymbolDataPtr sElement;
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	current = GetStartPosition();
	n = 0;
	while (current != NULL)
	{
		n++;
		GetNextSymbolData(current, sElement);
		ost << "\t" << sElement->GetString() << "(" << sElement->lRefCount << ")"
		    << "\n";
		if (n > nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

const ALString KWSymbolDictionary::GetClassLabel() const
{
	return "Symbol dictionary";
}

/////////////////////////////////////////////////////////////////////////////

void KWSymbolDictionary::Test()
{
	KWSymbolDictionary sdTest;
	KWSymbolDictionary sdPerf;
	ALString sRef;
	StringVector svArray;
	ALString sPerf;
	KWSymbolDataPtr sSymbol;
	int nI;
	int nMaxSize;
	int nNbIter;
	clock_t tStartClock;
	clock_t tStopClock;

	// Initialisation de 11 Strings
	sRef = "Symbol";
	for (nI = 0; nI < 11; nI++)
		svArray.Add(sRef + IntToString(nI));

	/////
	cout << "Basic test\n";
	//
	cout << "\tInsertion of 10 KWSymbolDataPtrs\n";
	for (nI = 0; nI < 10; nI++)
	{
		sSymbol = sdTest.AsSymbol(svArray.GetAt(nI), svArray.GetAt(nI).GetLength());
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
			sdPerf.AsSymbol(sPerf, sPerf.GetLength());
		}
		tStopClock = clock();
		cout << "TIME\tIteration " << nI << "\tSize = " << sdPerf.GetCount() << "\t"
		     << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	}

	//////////
	sdTest.RemoveAll();
	cout << "Performance test for the management of KWSymbolDataPtrs\n";

	nMaxSize = AcquireRangedInt("Number of KWSymbolData", 1, 100000, 10000);
	tStartClock = clock();
	for (nI = 0; nI < nMaxSize; nI++)
	{
		sPerf = IntToString(nI);
		sSymbol = sdTest.AsSymbol(sPerf, sPerf.GetLength());
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
		sSymbol = sdTest.AsSymbol("1", 1);
	}
	tStopClock = clock();
	cout << "TIME\t" << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	//
	cout << "\tPour un ALString:\n";
	tStartClock = clock();
	sPerf = IntToString(1);
	for (nI = 0; nI < nNbIter; nI++)
	{
		sSymbol = sdTest.AsSymbol(sPerf, sPerf.GetLength());
	}
	tStopClock = clock();
	cout << "TIME\t" << (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC << "\n";
	// Pour eviter un warning
	assert(sSymbol != NULL);
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Symbol
PLShared_Symbol::PLShared_Symbol() {}

PLShared_Symbol::~PLShared_Symbol() {}

void PLShared_Symbol::Write(ostream& ost) const
{
	ost << sSymbolValue.GetValue();
}

void PLShared_Symbol::Test()
{
	Symbol sValue1;
	PLShared_Symbol shared_symbol;
	Symbol sValue2;
	PLSerializer serializer;

	// Initialisation
	sValue1 = "test";
	shared_symbol = sValue1;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_symbol.Serialize(&serializer);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_symbol.Deserialize(&serializer);
	serializer.Close();
	sValue2 = shared_symbol;

	// Affichage du resultat
	cout << "Value 1 " << sValue1 << endl;
	cout << "Value 2 " << sValue2 << endl;
}

void PLShared_Symbol::SerializeValue(PLSerializer* serializer) const
{
	boolean bIsStarValue;

	// On encode le fait d'etre egal ou non a la StarValue
	bIsStarValue = (sSymbolValue == Symbol::GetStarValue());
	serializer->PutBoolean(bIsStarValue);

	// On encode la valeur du Symbol si necessaire
	if (not bIsStarValue)
		serializer->PutCharArray(sSymbolValue.GetValue());
}

void PLShared_Symbol::DeserializeValue(PLSerializer* serializer)
{
	boolean bIsStarValue;
	char* sValue;

	// On decode le fait d'etre egal ou non a la StarValue
	bIsStarValue = serializer->GetBoolean();

	// Cas la la StarValue
	if (bIsStarValue)
		sSymbolValue = Symbol::GetStarValue();
	// Decodage de la valeur du Symbol sinon
	else
	{
		sValue = serializer->GetCharArray();
		sSymbolValue = sValue;
		DeleteCharArray(sValue);
	}
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_SymbolVector

PLShared_SymbolVector::PLShared_SymbolVector() {}

PLShared_SymbolVector::~PLShared_SymbolVector() {}

void PLShared_SymbolVector::Test()
{
	SymbolVector svVector1;
	SymbolVector svVector2;
	PLShared_SymbolVector shared_symbolVector;
	PLSerializer serializer;

	// Initialisation
	svVector1.Add("test1");
	svVector1.Add("test2");
	svVector1.Add("test3");

	// Serialisation
	shared_symbolVector.SetSymbolVector(svVector1.Clone());
	serializer.OpenForWrite(NULL);
	shared_symbolVector.bIsDeclared = true;
	shared_symbolVector.Serialize(&serializer);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_symbolVector.Deserialize(&serializer);
	serializer.Close();
	svVector2.CopyFrom(shared_symbolVector.GetSymbolVector());

	// Affichage
	cout << "Vector 1" << endl;
	cout << svVector1 << endl << endl;
	cout << "Vector 2" << endl;
	cout << svVector2 << endl;
}

void PLShared_SymbolVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	SymbolVector* cv;
	PLShared_Symbol sharedSymbol;
	Symbol symbol;
	int i;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	cv = cast(SymbolVector*, o);

	// Serialisation du nombre de Symbol
	serializer->PutInt(cv->GetSize());

	// Serialisation de chaque Symbol
	for (i = 0; i < cv->GetSize(); i++)
	{
		symbol = cv->GetAt(i);
		sharedSymbol = symbol;
		sharedSymbol.Serialize(serializer);
	}
}

void PLShared_SymbolVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	SymbolVector* sv;
	PLShared_Symbol sharedSymbol;
	int i;
	int nSize;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	sv = cast(SymbolVector*, o);

	// Deserialisation de la taille
	nSize = serializer->GetInt();

	// DeSerialisation de chaque Symbol
	for (i = 0; i < nSize; i++)
	{
		sharedSymbol.Deserialize(serializer);
		sv->Add(sharedSymbol);
	}
}
