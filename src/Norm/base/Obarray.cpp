// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Object.h"
#include "Ermgt.h"
#include "SortedList.h"

/////////////////////////////////////////////
// Implementation de la classe Object

void Object::Write(ostream& ost) const
{
	ost << "[" << (void*)this << "]";
}

const ALString Object::WriteString() const
{
	ALString sResult;
	ostringstream osString;
	osString.str("");
	Write(osString);
	sResult = osString.str().c_str();
	return sResult;
}

boolean Object::Check() const
{
	return true;
}

longint Object::GetUsedMemory() const
{
	// Assertion pour forcer la reimplementation de la methode si elle est utilisee
	assert(false);
	return sizeof(Object);
}

const ALString Object::GetClassLabel() const
{
	return "Object";
}

const ALString Object::GetObjectLabel() const
{
	ALString sTmp;
	return sTmp;
}

void Object::AddSimpleMessage(const ALString& sLabel) const
{
	Global::AddSimpleMessage(sLabel);
}

void Object::AddMessage(const ALString& sLabel) const
{
	Global::AddMessage(GetClassLabel(), GetObjectLabel(), sLabel);
}

void Object::AddWarning(const ALString& sLabel) const
{
	Global::AddWarning(GetClassLabel(), GetObjectLabel(), sLabel);
}

void Object::AddError(const ALString& sLabel) const
{
	Global::AddError(GetClassLabel(), GetObjectLabel(), sLabel);
}

void Object::AddFatalError(const ALString& sLabel) const
{
	Global::AddFatalError(GetClassLabel(), GetObjectLabel(), sLabel);
}

const ALString Object::GetTranslatedClassLabel() const
{
	return GetClassLabel();
}

int ObjectCompare(const void* elem1, const void* elem2)
{
	Object* object1;
	Object* object2;

	object1 = *(Object**)elem1;
	object2 = *(Object**)elem2;

	// Comparaison de deux pointeurs: attention une simple diference ne rend pas necessairement un int (en 64 bits)
	if (object1 == object2)
		return 0;
	else if (object1 > object2)
		return 1;
	else
		return -1;
}

/////////////////////////////////////////////
// Implementation de la classe SampleObject

SampleObject::SampleObject(int nValue, ALString sValue)
{
	nInt = nValue;
	sString = sValue;
}

SampleObject* SampleObject::Clone() const
{
	SampleObject* soClone;

	soClone = new SampleObject;
	soClone->SetInt(GetInt());
	soClone->SetString(GetString());
	return soClone;
}

void SampleObject::Write(ostream& ost) const
{
	ost << " [" << GetInt() << "," << GetString() << "]";
}

longint SampleObject::GetUsedMemory() const
{
	return sizeof(SampleObject) + sString.GetAllocLength();
}

const ALString SampleObject::GetClassLabel() const
{
	return "Sample object";
}

int SampleObjectCompare(const void* elem1, const void* elem2)
{
	return (cast(SampleObject*, *(Object**)elem1)->GetInt() - cast(SampleObject*, *(Object**)elem2)->GetInt());
}

/////////////////////////////////////////////
// Implementation de la classe StringObject

StringObject* StringObject::Clone() const
{
	StringObject* soClone;

	soClone = new StringObject;
	soClone->SetString(GetString());
	return soClone;
}

longint StringObject::GetUsedMemory() const
{
	return sizeof(StringObject) + sString.GetAllocLength();
}

const ALString StringObject::GetClassLabel() const
{
	return "String object";
}

int StringObjectCompare(const void* elem1, const void* elem2)
{
	return cast(StringObject*, *(Object**)elem1)
	    ->GetString()
	    .Compare(cast(StringObject*, *(Object**)elem2)->GetString());
}

/////////////////////////////////////////////
// Implementation de la classe DoubleObject

DoubleObject* DoubleObject::Clone() const
{
	DoubleObject* ioClone;

	ioClone = new DoubleObject;
	ioClone->SetDouble(GetDouble());
	return ioClone;
}

longint DoubleObject::GetUsedMemory() const
{
	return sizeof(DoubleObject);
}

const ALString DoubleObject::GetClassLabel() const
{
	return "Double object";
}

int DoubleObjectCompare(const void* elem1, const void* elem2)
{
	return CompareDouble(cast(DoubleObject*, *(Object**)elem1)->GetDouble(),
			     cast(DoubleObject*, *(Object**)elem2)->GetDouble());
}

/////////////////////////////////////////////
// Implementation de la classe IntObject

IntObject* IntObject::Clone() const
{
	IntObject* ioClone;

	ioClone = new IntObject;
	ioClone->SetInt(GetInt());
	return ioClone;
}

longint IntObject::GetUsedMemory() const
{
	return sizeof(IntObject);
}

const ALString IntObject::GetClassLabel() const
{
	return "Int object";
}

int IntObjectCompare(const void* elem1, const void* elem2)
{
	return (cast(IntObject*, *(Object**)elem1)->GetInt() - cast(IntObject*, *(Object**)elem2)->GetInt());
}

/////////////////////////////////////////////
// Implementation de la classe LongintObject

LongintObject* LongintObject::Clone() const
{
	LongintObject* ioClone;

	ioClone = new LongintObject;
	ioClone->SetLongint(GetLongint());
	return ioClone;
}

longint LongintObject::GetUsedMemory() const
{
	return sizeof(LongintObject);
}

const ALString LongintObject::GetClassLabel() const
{
	return "Longint object";
}

int LongintObjectCompare(const void* elem1, const void* elem2)
{
	return CompareLongint(cast(LongintObject*, *(Object**)elem1)->GetLongint(),
			      cast(LongintObject*, *(Object**)elem2)->GetLongint());
}

//////////////////////////////////////////////////////////////////////////////
// Classe ObjectArray

ObjectArray::~ObjectArray()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void ObjectArray::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void ObjectArray::DeleteAll()
{
	int nI;
	for (nI = 0; nI < GetSize(); nI++)
	{
		if (GetAt(nI) != NULL)
			delete GetAt(nI);
	}
	RemoveAll();
}

void ObjectArray::InsertAt(int nIndex, Object* oElement)
{
	int nI;

	require(0 <= nIndex and nIndex <= GetSize());

	if (nIndex == GetSize())
		Add(oElement);
	else
	{
		// Retaillage du tableau
		SetSize(GetSize() + 1);

		// Decallage des elements vers la fin
		for (nI = GetSize() - 1; nI >= nIndex + 1; nI--)
			SetAt(nI, GetAt(nI - 1));

		// Insertion du nouvel element
		SetAt(nIndex, oElement);
	}
}

void ObjectArray::RemoveAt(int nIndex)
{
	int nI;

	require(0 <= nIndex and nIndex < GetSize());

	// Decallage des elements vers le debut
	for (nI = nIndex + 1; nI < GetSize(); nI++)
		SetAt(nI - 1, GetAt(nI));

	// Retaillage du tableau
	SetSize(GetSize() - 1);
}

void ObjectArray::InsertObjectArrayAt(int nStartIndex, ObjectArray* oaArray)
{
	int nInsertedSize;
	int nI;

	require(0 <= nStartIndex and nStartIndex <= GetSize());
	require(oaArray != NULL);
	require(oaArray != this);

	// Retaillage du tableau
	nInsertedSize = oaArray->GetSize();
	SetSize(GetSize() + nInsertedSize);

	// Decallage des elements du premier tableau vers la fin
	for (nI = GetSize() - 1; nI >= nStartIndex + nInsertedSize; nI--)
		SetAt(nI, GetAt(nI - nInsertedSize));

	// Recopie du tableau insere
	for (nI = 0; nI < oaArray->GetSize(); nI++)
		SetAt(nStartIndex + nI, oaArray->GetAt(nI));
}

void ObjectArray::SetCompareFunction(CompareFunction fCompare)
{
	fCompareFunction = fCompare;
}

CompareFunction ObjectArray::GetCompareFunction() const
{
	return fCompareFunction;
}

boolean ObjectArray::IsSortable() const
{
	return (fCompareFunction != NULL);
}

void ObjectArray::Sort()
{
	require(IsSortable());
	require(NoNulls());

	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, fCompareFunction);
}

Object* ObjectArray::Lookup(const Object* searchedKey) const
{
	int nFoundIndex;
	Object* key;

	require(IsSortable());
	require(searchedKey != NULL);

	// Recherche de l'index precedent
	nFoundIndex = FindPrecedingSortIndex(searchedKey);

	// On renvoie l'objet s'il correspond a un objet dont la cle est egale
	if (nFoundIndex >= 0)
	{
		key = GetAt(nFoundIndex);
		if (fCompareFunction(&key, &searchedKey) == 0)
			return key;
	}

	// Sinon, on renvoie NULL
	return NULL;
}

int ObjectArray::FindSortIndex(const Object* searchedKey) const
{
	int nFoundIndex;
	const Object* key;

	require(IsSortable());
	require(searchedKey != NULL);

	// Recherche de l'index precedent
	nFoundIndex = FindPrecedingSortIndex(searchedKey);

	// On renvoie l'index s'il correspond a un objet dont la cle est egale
	if (nFoundIndex >= 0)
	{
		key = GetAt(nFoundIndex);
		if (fCompareFunction(&key, &searchedKey) == 0)
			return nFoundIndex;
	}

	// Sinon, on renvoie -1
	return -1;
}

int ObjectArray::FindPrecedingSortIndex(const Object* searchedKey) const
{
	int nLowerIndex;
	int nUpperIndex;
	int nIndex;
	int nCompare;
	const Object* key;
	debug(Object * oFirstKey);
	Object* oLowerKey;
	Object* oUpperKey;

	require(IsSortable());
	require(searchedKey != NULL);

	// On elimine le cas d'un tableau vide (pour ne pas traiter les effet de bord ensuite)
	if (GetSize() == 0)
		return -1;

	// Recherche sequentielle s'il y a peu d'elements
	if (GetSize() <= 10)
	{
		// Recherche sequentielle
		for (nIndex = 0; nIndex < GetSize(); nIndex++)
		{
			key = GetAt(nIndex);
			check(key);
			debug(if (nIndex > 0) oLowerKey = GetAt(nIndex - 1));
			debug(assert(nIndex == 0 or fCompareFunction(&key, &oLowerKey) >= 0));
			nCompare = fCompareFunction(&key, &searchedKey);

			// On a atteint la cle
			if (nCompare == 0)
				break;
			// On a depasse la cle
			else if (nCompare > 0)
			{
				nIndex--;
				break;
			}
		}

		// Cas ou l'on a jamais depasse la cle
		if (nIndex == GetSize())
			nIndex--;
	}
	// Recherche dichotomique sinon
	else
	{
		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = GetSize() - 1;

		// Recherche dichotomique de la cle
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			debug(oLowerKey = GetAt(nLowerIndex));
			debug(oUpperKey = GetAt(nUpperIndex));
			debug(assert(fCompareFunction(&oLowerKey, &oUpperKey) <= 0));

			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			key = GetAt(nIndex);
			check(key);
			nCompare = fCompareFunction(&key, &searchedKey);
			if (nCompare >= 0)
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain element teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		oLowerKey = GetAt(nLowerIndex);
		nCompare = fCompareFunction(&oLowerKey, &searchedKey);
		if (nCompare < 0)
		{
			oUpperKey = GetAt(nUpperIndex);
			nCompare = fCompareFunction(&searchedKey, &oUpperKey);
			if (nCompare >= 0)
				nIndex = nUpperIndex;
			else
				nIndex = nLowerIndex;
		}
		else if (nCompare == 0)
			nIndex = nLowerIndex;
		else
			nIndex = -1;
	}

	// Verification de la solution
	debug(if (nIndex >= 0) key = GetAt(nIndex));
	debug(oFirstKey = GetAt(0));
	debug(if (nIndex > 0) oLowerKey = GetAt(nIndex - 1));
	debug(if (nIndex >= 0 and nIndex < GetSize() - 1) oUpperKey = GetAt(nIndex + 1));
	debug(assert(GetSize() > 0 or nIndex == -1));
	// La cle trouvee doit etre plus petite que la clee cherchee
	debug(assert(GetSize() == 0 or nIndex == -1 or fCompareFunction(&key, &searchedKey) <= 0));
	// La cle cherchee doit etre plus petite que la cle suivant la cle trouvee
	debug(assert(GetSize() == 0 or nIndex == -1 or nIndex == GetSize() - 1 or
		     fCompareFunction(&searchedKey, &oUpperKey) <= 0));
	// La cle precedant la cle trouvee doit etre plus petite que la cle cherchee
	debug(assert(GetSize() == 0 or nIndex == -1 or nIndex == 0 or fCompareFunction(&oLowerKey, &searchedKey) < 0));
	// Si pas de cle trouvee, la cle cherche doit etre plus petite que la permiere cle
	debug(
	    assert(GetSize() == 0 or nIndex >= 0 or (nIndex == -1 and fCompareFunction(&searchedKey, &oFirstKey) < 0)));
	// La cle precedant la cle trouvee doit etre strictement plus petite que la cle cherchee si la cle trouvee est
	// egale a la cle cherchee
	debug(assert(GetSize() == 0 or nIndex == -1 or nIndex == 0 or fCompareFunction(&key, &searchedKey) == 0 or
		     fCompareFunction(&oLowerKey, &searchedKey) < 0));
	return nIndex;
}

void ObjectArray::SystemSort()
{
	require(NoNulls());

	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, ObjectCompare);
}

void ObjectArray::Shuffle()
{
	int i;
	int iSwap;
	Object* swappedObject;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		swappedObject = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, swappedObject);
	}
}

boolean ObjectArray::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void ObjectArray::CopyFrom(const ObjectArray* oaSource)
{
	require(oaSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, oaSource->pData.hugeVector,
			    oaSource->nSize, oaSource->nAllocSize);
}

void ObjectArray::ConcatFrom(const ObjectArray* oaSource1, const ObjectArray* oaSource2)
{
	int nI;
	int nIndex;

	require(oaSource1 != NULL);
	require(oaSource2 != NULL);
	require(oaSource1 != this);
	require(oaSource2 != this);

	// Retaillage
	SetSize(oaSource1->GetSize() + oaSource2->GetSize());

	// Recopie du premier tableau
	nIndex = 0;
	for (nI = 0; nI < oaSource1->GetSize(); nI++)
	{
		SetAt(nIndex, oaSource1->GetAt(nI));
		nIndex++;
	}

	// Recopie du second tableau
	for (nI = 0; nI < oaSource2->GetSize(); nI++)
	{
		SetAt(nIndex, oaSource2->GetAt(nI));
		nIndex++;
	}
}

ObjectArray* ObjectArray::Clone() const
{
	ObjectArray* oaClone;

	oaClone = new ObjectArray;

	// Recopie
	oaClone->CopyFrom(this);
	return oaClone;
}

void ObjectArray::ExportObjectList(ObjectList* olResult) const
{
	int i;

	require(olResult != NULL);

	olResult->RemoveAll();
	for (i = 0; i < GetSize(); i++)
		olResult->AddTail(GetAt(i));

	ensure(olResult->GetCount() == GetSize());
}

void ObjectArray::ExportSortedList(SortedList* slResult) const
{
	int i;

	require(IsSortable());
	require(slResult != NULL);
	require(slResult->GetCompareFunction() == GetCompareFunction());

	// Parcours des objets pour constituer la liste
	slResult->RemoveAll();
	for (i = 0; i < GetSize(); i++)
		slResult->Add(GetAt(i));

	ensure(slResult->GetCount() == GetSize());
}

void ObjectArray::Union(ObjectArray* oaFirst, ObjectArray* oaSecond)
{
	ObjectArray* oaTmp;
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;
	int nNbElements = 0;

	require(oaFirst != NULL);
	require(oaFirst->NoNulls());
	require(oaSecond != NULL);
	require(oaSecond->NoNulls());

	// Initialisation
	oaTmp = GetConcatenatedArray(oaFirst, oaSecond);
	oaTmp->SystemSort();

	// Parcours du tableau trie sur les pointeurs, pour regrouper par paquet
	SetSize(0);
	for (nCurrent = 0; nCurrent < oaTmp->GetSize(); nCurrent++)
	{
		objectCurrent = oaTmp->GetAt(nCurrent);
		if (objectReference == NULL or ObjectCompare((void*)&objectCurrent, (void*)&objectReference))
		{
			if (nNbElements > 0)
				Add(objectReference);
			objectReference = objectCurrent;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}
		if (nCurrent == oaTmp->GetSize() - 1)
			Add(objectReference);
	}

	// Terminaison
	delete oaTmp;
}

void ObjectArray::Intersection(ObjectArray* oaFirst, ObjectArray* oaSecond)
{
	ObjectArray* oaTmp;
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;
	int nNbElements = 0;

	require(oaFirst != NULL);
	require(oaFirst->NoNulls());
	require(oaSecond != NULL);
	require(oaSecond->NoNulls());

	// Initialisation
	oaTmp = GetConcatenatedArray(oaFirst, oaSecond);
	oaTmp->SystemSort();

	// Parcours du tableau trie sur les pointeurs, pour regrouper par paquet
	SetSize(0);
	for (nCurrent = 0; nCurrent < oaTmp->GetSize(); nCurrent++)
	{
		objectCurrent = oaTmp->GetAt(nCurrent);
		if (objectReference == NULL or ObjectCompare((void*)&objectCurrent, (void*)&objectReference))
		{
			if (nNbElements > 1)
				Add(objectReference);
			objectReference = objectCurrent;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}
		if (nCurrent == oaTmp->GetSize() - 1)
			if (nNbElements > 1)
				Add(objectReference);
	}

	// Terminaison
	delete oaTmp;
}

void ObjectArray::Difference(ObjectArray* oaFirst, ObjectArray* oaSecond)
{
	ObjectArray* oaTmp;
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;
	int nNbElements = 0;

	require(oaFirst != NULL);
	require(oaFirst->NoNulls());
	require(oaSecond != NULL);
	require(oaSecond->NoNulls());

	// Initialisation
	oaTmp = GetConcatenatedArray(oaFirst, oaSecond);
	oaTmp->SystemSort();

	// Parcours du tableau trie sur les pointeurs, pour regrouper par paquet
	SetSize(0);
	for (nCurrent = 0; nCurrent < oaTmp->GetSize(); nCurrent++)
	{
		objectCurrent = oaTmp->GetAt(nCurrent);
		if (objectReference == NULL or ObjectCompare((void*)&objectCurrent, (void*)&objectReference))
		{
			if (nNbElements == 1)
				Add(objectReference);
			objectReference = objectCurrent;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}
		if (nCurrent == oaTmp->GetSize() - 1)
			if (nNbElements == 1)
				Add(objectReference);
	}

	// Terminaison
	delete oaTmp;
}

boolean ObjectArray::NoNulls() const
{
	int i;
	for (i = 0; i < GetSize(); i++)
	{
		if (GetAt(i) == NULL)
			return false;
	}
	return true;
}

boolean ObjectArray::OnlyNulls() const
{
	int i;
	for (i = 0; i < GetSize(); i++)
	{
		if (GetAt(i) != NULL)
			return false;
	}
	return true;
}

void ObjectArray::Write(ostream& ost) const
{
	Object* object;
	const int nMax = 10;
	int n;

	// Parcours des elements de la liste pour alimenter le tableau
	ost << GetClassLabel() << " [" << GetSize() << "]\n";
	for (n = 0; n < GetSize(); n++)
	{
		object = GetAt(n);
		ost << "\t" << n << ":";
		if (object == NULL)
			ost << "\tnull\n";
		else
			ost << "\t" << *object << "\n";
		if (n >= nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint ObjectArray::GetUsedMemory() const
{
	return sizeof(ObjectArray) + nAllocSize * sizeof(void*);
}

longint ObjectArray::GetOverallUsedMemory() const
{
	longint lUsedMemory;
	int i;
	Object* oValue;

	lUsedMemory = GetUsedMemory();
	for (i = 0; i < GetSize(); i++)
	{
		oValue = GetAt(i);
		if (oValue != NULL)
			lUsedMemory += oValue->GetUsedMemory();
	}
	return lUsedMemory;
}

longint ObjectArray::GetUsedMemoryPerElement() const
{
	return sizeof(void*);
}

const ALString ObjectArray::GetClassLabel() const
{
	return "Array";
}

ObjectArray* ObjectArray::GetConcatenatedArray(ObjectArray* oaFirst, ObjectArray* oaSecond)
{
	ObjectArray* oaResult;
	int nI;

	require(oaFirst != NULL);
	require(oaSecond != NULL);

	oaResult = new ObjectArray;
	oaResult->SetSize(oaFirst->GetSize() + oaSecond->GetSize());
	for (nI = 0; nI < oaFirst->GetSize(); nI++)
		oaResult->SetAt(nI, oaFirst->GetAt(nI));
	for (nI = 0; nI < oaSecond->GetSize(); nI++)
		oaResult->SetAt(oaFirst->GetSize() + nI, oaSecond->GetAt(nI));

	return oaResult;
}

/////////////////////////////////////////////////////////////////

void ObjectArray::Test()
{
	ObjectArray oaTest;
	ObjectArray* oaClone;
	SampleObject soElement;
	SampleObject* soSorted;
	int nSize;
	int nNb;
	int nIndex;
	int nI, nJ;
	StringObject soTest;
	int nStartClock;
	int nStopClock;
	boolean bOk;

	// Test des StringObjects
	SetRandomSeed(1);
	soTest.SetString("Hello, StringObject");
	cout << soTest << endl << endl;

	// Test de changement de taille
	cout << "Test de changement de taille\n";
	oaTest.SetSize(0);
	oaTest.SetSize(10);
	oaTest.SetSize(100);
	oaTest.SetSize(10);
	oaTest.SetSize(10);
	oaTest.SetSize(0);

	// Test de changement de taille dynamique
	cout << "Test de changement de taille dynamique\n";
	nSize = AcquireRangedInt("Nombre d'objects a inserer", 1, 1000000, 1000);
	nNb = AcquireRangedInt("Nombre d'iterations)", 1, 100000, 1000);
	nStartClock = clock();
	for (nI = 0; nI < nNb; nI++)
	{
		oaTest.RemoveAll();
		for (nJ = 0; nJ < nSize; nJ++)
			oaTest.Add(&soElement);
	}
	nStopClock = clock();
	cout << "SYS TIME\tObjectArray change size\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
	{
		// Test de conversion en liste
		ObjectList olTest;
		oaTest.ExportObjectList(&olTest);
		assert(oaTest.GetSize() == olTest.GetCount());
	}

	// Test d'acces aux valeurs
	cout << "\nTest d'acces aux valeurs\n";
	nSize = AcquireRangedInt("Nombre d'objects", 1, 1000, 100);
	oaTest.SetSize(nSize);
	for (nI = 0; nI < nSize; nI++)
	{
		oaTest.SetAt(nI, &soElement);
	}
	cout << "\n" << oaTest << "\n";

	// Test de performance
	cout << "\nTest de performance\n";
	nNb = AcquireRangedInt("Nombre d'iterations (lecture et ecriture)", 1, 100000, 1000);
	nStartClock = clock();
	for (nI = 0; nI < nNb; nI++)
	{
		for (nJ = 0; nJ < nSize; nJ++)
		{
			oaTest.SetAt(nJ, &soElement);
			oaTest.GetAt(nJ);
		}
	}
	nStopClock = clock();
	cout << "SYS TIME\tObjectArray access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
	cout << "SYS MEMORY\tUsed memory\t" << oaTest.GetSize() << "\t" << oaTest.GetUsedMemory() << "\t"
	     << oaTest.GetOverallUsedMemory() << endl;

	/// Test de tri
	cout << "\nTests de tri\n";

	// Initialisation
	oaTest.RemoveAll();
	for (nI = 0; nI < 10; nI++)
	{
		soSorted = new SampleObject(RandomInt(100), "so");
		oaTest.Add(soSorted);
	}

	// Impression du contenu initial
	cout << "Impression du contenu initial\n";
	for (nI = 0; nI < 10; nI++)
	{
		soSorted = cast(SampleObject*, oaTest.GetAt(nI));
		cout << *soSorted;
	}
	cout << "\n";

	// Impression apres perturbation aleatoire
	// de l'ordre des elements
	cout << "Impression du contenu apres perturbation\n";
	oaTest.Shuffle();
	for (nI = 0; nI < 10; nI++)
	{
		soSorted = cast(SampleObject*, oaTest.GetAt(nI));
		cout << *soSorted;
	}
	cout << "\n";

	// Impression du contenu, apres tri utilisateur
	cout << "Impression du contenu, apres tri utilisateur\n";
	oaTest.SetCompareFunction(SampleObjectCompare);
	oaTest.Sort();
	for (nI = 0; nI < 10; nI++)
	{
		soSorted = cast(SampleObject*, oaTest.GetAt(nI));
		cout << *soSorted;
	}
	cout << "\n";

	// Duplication
	cout << "Duplication\n";
	oaClone = oaTest.Clone();
	for (nI = 0; nI < 10; nI++)
	{
		soSorted = cast(SampleObject*, oaClone->GetAt(nI));
		cout << *soSorted;
	}
	cout << "\n";

	// Concatenation
	cout << "Concatenation\t";
	oaClone->ConcatFrom(&oaTest, &oaTest);
	bOk = true;
	for (nI = 0; nI < 10; nI++)
	{
		bOk = bOk and oaTest.GetAt(nI) == oaClone->GetAt(nI);
		bOk = bOk and oaTest.GetAt(nI) == oaClone->GetAt(oaTest.GetSize() + nI);
	}
	cout << bOk << "\n";

	// Copie
	cout << "Copy\t";
	oaClone->CopyFrom(&oaTest);
	bOk = true;
	for (nI = 0; nI < 10; nI++)
	{
		bOk = bOk and oaTest.GetAt(nI) == oaClone->GetAt(nI);
	}
	delete oaClone;
	cout << bOk << "\n";

	// Test du contenu, apres tri systeme
	cout << "Tri systeme\t";
	oaTest.SystemSort();
	bOk = true;
	for (nI = 1; nI < 10; nI++)
	{
		bOk = bOk and oaTest.GetAt(nI - 1) <= oaTest.GetAt(nI);
	}
	cout << bOk << "\n";

	// Recherche par cle
	cout << "Recherche par cle en iterant de 0 a 100\n";
	oaTest.Sort();
	for (nI = 0; nI < 100; nI++)
	{
		soElement.SetInt(nI);
		soSorted = cast(SampleObject*, oaTest.Lookup(&soElement));
		if (soSorted != NULL)
			cout << *soSorted;
	}
	cout << "\n";

	// Recherche des positions des cle dans une liste contetant des doublons
	oaTest.DeleteAll();
	cout << "Recherche avancee par cle dans un tableau de doublons\n";
	for (nI = 0; nI < 5; nI++)
	{
		// On ajoute deux fois chaque element
		soSorted = new SampleObject(2 * nI + 1, "so");
		oaTest.Add(soSorted);
		soSorted = new SampleObject(2 * nI + 1, "so");
		oaTest.Add(soSorted);
	}
	for (nI = 0; nI < oaTest.GetSize(); nI++)
		cout << " " << *oaTest.GetAt(nI);
	cout << endl;
	cout << "\tKey\tFound\tIndex\tPreceding" << endl;
	for (nI = 0; nI <= 10; nI++)
	{
		soElement.SetInt(nI);

		// Recherche de l'element
		soSorted = cast(SampleObject*, oaTest.Lookup(&soElement));
		cout << "\t" << soElement << "\t";
		if (soSorted != NULL)
			cout << *soSorted;

		// Recjercje de la position et- de la position precedente
		nIndex = oaTest.FindSortIndex(&soElement);
		cout << "\t" << nIndex;
		nIndex = oaTest.FindPrecedingSortIndex(&soElement);
		cout << "\t" << nIndex << endl;
	}

	// Liberation, avec les object contenus
	oaTest.DeleteAll();

	// Tests ensemblistes
	{
		ObjectArray oaFirst;
		ObjectArray oaSecond;
		ObjectArray oaResult;
		SampleObject* soElem;

		// Initialisations
		cout << "\n\nTests ensemblistes\n";
		oaResult.SetCompareFunction(SampleObjectCompare);
		for (nI = 0; nI < 10; nI++)
		{
			soElem = new SampleObject(nI, "so");
			oaFirst.Add(soElem);
		}
		for (nI = 5; nI < 10; nI++)
		{
			oaSecond.Add(oaFirst.GetAt(nI));
		}
		for (nI = 10; nI < 15; nI++)
		{
			soElem = new SampleObject(nI, "so");
			oaSecond.Add(soElem);
		}

		// Impression des contenus

		cout << "Ensemble First\n";
		for (nI = 0; nI < oaFirst.GetSize(); nI++)
		{
			soElem = cast(SampleObject*, oaFirst.GetAt(nI));
			cout << *soElem;
		}
		cout << "\n";

		cout << "Ensemble Second\n";
		for (nI = 0; nI < oaSecond.GetSize(); nI++)
		{
			soElem = cast(SampleObject*, oaSecond.GetAt(nI));
			cout << *soElem;
		}
		cout << "\n";

		cout << "Ensemble Union(First,Second)\n";
		oaResult.Union(&oaFirst, &oaSecond);
		oaResult.Sort();
		for (nI = 0; nI < oaResult.GetSize(); nI++)
		{
			soElem = cast(SampleObject*, oaResult.GetAt(nI));
			cout << *soElem;
		}
		cout << "\n";

		cout << "Ensemble Intersection(First,Second)\n";
		oaResult.Intersection(&oaFirst, &oaSecond);
		oaResult.Sort();
		for (nI = 0; nI < oaResult.GetSize(); nI++)
		{
			soElem = cast(SampleObject*, oaResult.GetAt(nI));
			cout << *soElem;
		}
		cout << "\n";

		cout << "Ensemble Difference(First,Second)\n";
		oaResult.Difference(&oaFirst, &oaSecond);
		oaResult.Sort();
		for (nI = 0; nI < oaResult.GetSize(); nI++)
		{
			soElem = cast(SampleObject*, oaResult.GetAt(nI));
			cout << *soElem;
		}
		cout << "\n";

		// Terminaison
		oaResult.Union(&oaFirst, &oaSecond);
		oaResult.DeleteAll();
	}
}
