// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Object.h"

ObjectList::ObjectList()
{
	require(nBlockSize > 0);

	nCount = 0;
	pNodeHead = NULL;
	pNodeTail = NULL;
	pNodeFree = NULL;
	pBlocks = NULL;
}

void ObjectList::RemoveAll()
{
	nCount = 0;
	pNodeHead = NULL;
	pNodeTail = NULL;
	pNodeFree = NULL;
	CPlex::FreeDataChain(pBlocks);
	pBlocks = NULL;
}

void ObjectList::DeleteAll()
{
	POSITION position;
	Object* object;

	// Premiere passe de destruction du contenu de la liste
	position = GetHeadPosition();
	while (position != NULL)
	{
		object = GetNext(position);
		if (object != NULL)
			delete object;
	}

	// Nettoyage de la liste elle-meme
	RemoveAll();
}

ObjectList::~ObjectList()
{
	RemoveAll();
	assert(nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Gestion des noeuds d'une liste
//
// Les noeuds d'une liste (ListNode) sont stockes dans des blocs CPlex et chaines ensembles.
// Les blocs libres sont maintenus dans une liste simplement chainee utilisant pNext
// avec pNodeFree en tete de liste.
// Les bloc sont eux meme gere dans une liste doublement chainee en utilisant pPrev et pNext
// pour les liens, et pNodeHead and pNodeTail pour la tete et la queue de liste
//
// Les blocs CPlex ne sont jamais liberes, sauf la liste est detruite par RemoveAll(),
// si bien que le nombre de blocs peut croitre, en fonction des utuilisation passees de la liste

ListNode* ObjectList::NewNode(ListNode* pPrev, ListNode* pNext)
{
	if (pNodeFree == NULL)
	{
		// Ajout d'un nouveau bloc
		CPlex* pNewBlock = CPlex::Create(pBlocks, nBlockSize, sizeof(ListNode));

		// Chainage dans la liste des blocs libres
		ListNode* pNode = (ListNode*)pNewBlock->data();

		// Liberation en ordre inverse pour faciliter le debugage
		pNode += nBlockSize - 1;
		for (int i = nBlockSize - 1; i >= 0; i--, pNode--)
		{
			pNode->pNext = pNodeFree;
			pNodeFree = pNode;
		}
	}
	assert(pNodeFree != NULL);

	ListNode* pNode = pNodeFree;
	pNodeFree = pNodeFree->pNext;
	pNode->pPrev = pPrev;
	pNode->pNext = pNext;
	pNode->data = NULL;
	nCount++;
	assert(nCount > 0);

	return pNode;
}

POSITION ObjectList::AddHead(Object* newElement)
{
	ListNode* pNewNode = NewNode(NULL, pNodeHead);
	pNewNode->data = newElement;
	if (pNodeHead != NULL)
		pNodeHead->pPrev = pNewNode;
	else
		pNodeTail = pNewNode;
	pNodeHead = pNewNode;
	return (POSITION)pNewNode;
}

POSITION ObjectList::AddTail(Object* newElement)
{
	ListNode* pNewNode = NewNode(pNodeTail, NULL);
	pNewNode->data = newElement;
	if (pNodeTail != NULL)
		pNodeTail->pNext = pNewNode;
	else
		pNodeHead = pNewNode;
	pNodeTail = pNewNode;
	return (POSITION)pNewNode;
}

void ObjectList::AddHead(ObjectList* pNewList)
{
	require(pNewList != NULL);

	POSITION pos = pNewList->GetTailPosition();
	while (pos != NULL)
		AddHead(pNewList->GetPrev(pos));
}

void ObjectList::AddTail(ObjectList* pNewList)
{
	require(pNewList != NULL);

	POSITION pos = pNewList->GetHeadPosition();
	while (pos != NULL)
		AddTail(pNewList->GetNext(pos));
}

Object* ObjectList::RemoveHead()
{
	require(not IsEmpty());
	require(pNodeHead != NULL);

	ListNode* pOldNode = pNodeHead;
	Object* returnValue = pOldNode->data;

	pNodeHead = pOldNode->pNext;
	if (pNodeHead != NULL)
		pNodeHead->pPrev = NULL;
	else
		pNodeTail = NULL;
	FreeNode(pOldNode);
	return returnValue;
}

Object* ObjectList::RemoveTail()
{
	require(not IsEmpty());
	require(pNodeTail != NULL);

	ListNode* pOldNode = pNodeTail;
	Object* returnValue = pOldNode->data;

	pNodeTail = pOldNode->pPrev;
	if (pNodeTail != NULL)
		pNodeTail->pNext = NULL;
	else
		pNodeHead = NULL;
	FreeNode(pOldNode);
	return returnValue;
}

POSITION ObjectList::InsertBefore(POSITION position, Object* newElement)
{
	// Insertion avant rien: tete de liste
	if (position == NULL)
		return AddHead(newElement);

	// Insertion avant la position
	ListNode* pOldNode = (ListNode*)position;
	ListNode* pNewNode = NewNode(pOldNode->pPrev, pOldNode);
	pNewNode->data = newElement;

	if (pOldNode->pPrev != NULL)
	{
		pOldNode->pPrev->pNext = pNewNode;
	}
	else
	{
		assert(pOldNode == pNodeHead);
		pNodeHead = pNewNode;
	}
	pOldNode->pPrev = pNewNode;
	return (POSITION)pNewNode;
}

POSITION ObjectList::InsertAfter(POSITION position, Object* newElement)
{
	// Insertion apres rien: queue de liste
	if (position == NULL)
		return AddTail(newElement);

	// Insertion apres la position
	ListNode* pOldNode = (ListNode*)position;
	ListNode* pNewNode = NewNode(pOldNode, pOldNode->pNext);
	pNewNode->data = newElement;

	if (pOldNode->pNext != NULL)
	{
		pOldNode->pNext->pPrev = pNewNode;
	}
	else
	{
		assert(pOldNode == pNodeTail);
		pNodeTail = pNewNode;
	}
	pOldNode->pNext = pNewNode;
	return (POSITION)pNewNode;
}

void ObjectList::RemoveAt(POSITION position)
{
	ListNode* pOldNode = (ListNode*)position;
	check(pOldNode);

	// Supression de pOldNode de la liste
	if (pOldNode == pNodeHead)
	{
		pNodeHead = pOldNode->pNext;
	}
	else
	{
		pOldNode->pPrev->pNext = pOldNode->pNext;
	}
	if (pOldNode == pNodeTail)
	{
		pNodeTail = pOldNode->pPrev;
	}
	else
	{
		pOldNode->pNext->pPrev = pOldNode->pPrev;
	}
	FreeNode(pOldNode);
}

POSITION ObjectList::FindIndex(int nIndex) const
{
	require(nIndex >= 0);

	if (nIndex >= nCount)
		return NULL;

	ListNode* pNode = pNodeHead;
	while (nIndex--)
	{
		pNode = pNode->pNext;
	}
	return (POSITION)pNode;
}

POSITION ObjectList::Find(Object* searchValue, POSITION startAfter) const
{
	ListNode* pNode = (ListNode*)startAfter;
	// On part de la tete
	if (pNode == NULL)
	{
		pNode = pNodeHead;
	}
	// Ou d'une position specifiee
	else
	{
		pNode = pNode->pNext;
	}

	for (; pNode != NULL; pNode = pNode->pNext)
		if (pNode->data == searchValue)
			return (POSITION)pNode;
	return NULL;
}

void ObjectList::CopyFrom(const ObjectList* olSource)
{
	POSITION position;

	require(olSource != NULL);

	// Cas particulier ou source egale cible
	if (olSource == this)
		return;

	// Nettoyage
	RemoveAll();

	// Recopie
	position = olSource->GetHeadPosition();
	while (position != NULL)
		AddTail(olSource->GetNext(position));
}

ObjectList* ObjectList::Clone() const
{
	ObjectList* olClone;
	POSITION position;

	olClone = new ObjectList;
	position = GetHeadPosition();
	while (position != NULL)
		olClone->AddTail(GetNext(position));
	return olClone;
}

void ObjectList::ExportObjectArray(ObjectArray* oaResult) const
{
	POSITION position;
	int nIndex;

	require(oaResult != NULL);

	oaResult->SetSize(GetCount());
	position = GetHeadPosition();
	nIndex = 0;
	while (position != NULL)
	{
		oaResult->SetAt(nIndex, GetNext(position));
		nIndex++;
	}
	ensure(oaResult->GetSize() == GetCount());
}

void ObjectList::Write(ostream& ost) const
{
	POSITION position;
	Object* object;
	const int nMax = 10;
	int n;

	// Parcours des elements de la liste pour alimenter le tableau
	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	position = GetHeadPosition();
	n = 0;
	while (position != NULL)
	{
		n++;
		object = GetNext(position);
		if (object == NULL)
			ost << "\tnull\n";
		else
			ost << "\t" << *object << "\n";
		if (n > nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint ObjectList::GetUsedMemory() const
{
	return sizeof(ObjectList) + nCount * sizeof(ListNode) + (nCount * sizeof(CPlex)) / nBlockSize;
}

longint ObjectList::GetOverallUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	Object* oValue;

	lUsedMemory = GetUsedMemory();
	position = GetHeadPosition();
	while (position != NULL)
	{
		oValue = GetNext(position);
		if (oValue != NULL)
			lUsedMemory += oValue->GetUsedMemory();
	}
	return lUsedMemory;
}

longint ObjectList::GetUsedMemoryPerElement() const
{
	return sizeof(ListNode);
}

const ALString ObjectList::GetClassLabel() const
{
	return "List";
}

void ObjectList::Test()
{
	ObjectList olTest;
	SampleObject soTest;
	SampleObject* soResult;
	int i;
	POSITION position;
	int nNbInsert;
	int nNbRead;
	int nStartClock;
	int nStopClock;

	// Test d'insertions
	nNbInsert = AcquireRangedInt("Nombre d'insertion", 0, 10000000, 1000);
	nStartClock = clock();
	for (i = 0; i < nNbInsert; i++)
		olTest.AddTail(&soTest);
	nStopClock = clock();
	cout << "SYS TIME\tObjectList insertion\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
	cout << "SYS MEMORY\tUsed memory\t" << olTest.GetCount() << "\t" << olTest.GetUsedMemory() << "\t"
	     << olTest.GetOverallUsedMemory() << endl;

	// Affichage
	cout << "\n" << olTest << "\n";

	// Test de parcours de liste
	nNbRead = AcquireRangedInt("Nombre de parcours de la liste", 0, 100000, 1000);
	nStartClock = clock();
	for (i = 0; i < nNbRead; i++)
	{
		soResult = NULL;
		position = olTest.GetHeadPosition();
		while (position != NULL)
			soResult = cast(SampleObject*, olTest.GetNext(position));
		assert(soResult == &soTest);
	}
	nStopClock = clock();
	cout << "SYS TIME\tObjectList access\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
}
