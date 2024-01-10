// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "QueryServices.h"

//////////////////////////////////////////////
// Implementation de la classe QueryServices

QueryServices::QueryServices()
{
	fCompareFunction = NULL;
	fGetterFunction = NULL;
	bIsInit = false;
}

QueryServices::~QueryServices() {}

void QueryServices::Init(CompareFunction fCompare, GetterFunction fGetter, const ALString& sClassNameValue,
			 const ALString& sAttributeNameValue)
{
	require(not IsInit());
	require(fCompare != NULL);
	require(fGetter != NULL);
	require(sClassNameValue != "");
	require(sAttributeNameValue != "");

	// Initialisation des attributs
	fCompareFunction = fCompare;
	fGetterFunction = fGetter;
	sClassName = sClassNameValue;
	sAttributeName = sAttributeNameValue;
	bIsInit = true;

	ensure(IsInit());
}

void QueryServices::LoadObjectArray(const ObjectArray* oaObjects)
{
	require(oaObjects != NULL);
	require(IsInit());

	// Initialisation du tableau
	oaSorted.CopyFrom(oaObjects);
	assert(oaSorted.NoNulls());

	// Tri du tableau
	oaSorted.SetCompareFunction(fCompareFunction);
	oaSorted.Sort();
}

void QueryServices::LoadObjectList(const ObjectList* olObjects)
{
	require(olObjects != NULL);
	require(IsInit());

	// Initialisation du tableau
	olObjects->ExportObjectArray(&oaSorted);
	assert(oaSorted.NoNulls());

	// Tri du tableau
	oaSorted.SetCompareFunction(fCompareFunction);
	oaSorted.Sort();
}

void QueryServices::LoadObjectDictionary(const ObjectDictionary* odObjects)
{
	require(odObjects != NULL);
	require(IsInit());

	// Initialisation du tableau
	odObjects->ExportObjectArray(&oaSorted);
	assert(oaSorted.NoNulls());

	// Tri du tableau
	oaSorted.SetCompareFunction(fCompareFunction);
	oaSorted.Sort();
}

void QueryServices::LoadNumericKeyDictionary(const NumericKeyDictionary* nkdObjects)
{
	require(nkdObjects != NULL);
	require(IsInit());

	// Initialisation du tableau
	nkdObjects->ExportObjectArray(&oaSorted);
	assert(oaSorted.NoNulls());

	// Tri du tableau
	oaSorted.SetCompareFunction(fCompareFunction);
	oaSorted.Sort();
}

boolean QueryServices::IsInit() const
{
	return bIsInit;
}

CompareFunction QueryServices::GetCompareFunction() const
{
	return fCompareFunction;
}

GetterFunction QueryServices::GetGetterFunction() const
{
	return fGetterFunction;
}

const ALString& QueryServices::GetClassName() const
{
	return sClassName;
}

const ALString& QueryServices::GetAttributeName() const
{
	return sAttributeName;
}

ObjectArray* QueryServices::QueryAllObjects() const
{
	require(IsInit());

	return oaSorted.Clone();
}

int QueryServices::GetSize() const
{
	require(IsInit());

	return oaSorted.GetSize();
}

Object* QueryServices::GetAt(int nIndex) const
{
	require(IsInit());
	require(0 <= nIndex and nIndex < GetSize());

	return oaSorted.GetAt(nIndex);
}

ObjectArray* QueryServices::QueryObjectsMatching(const Object* aKey) const
{
	int nIndex;
	int nFirst;
	int nLast;
	int i;
	ObjectArray* oaResult;

	require(IsInit());

	// Recherche binaire de l'index d'un element du tableau
	nIndex = GetMatchingObjectIndex(aKey);

	// Si pas d'elememt trouve: on renvoi un tableau vide
	if (nIndex == -1)
	{
		oaResult = new ObjectArray;
		return oaResult;
	}

	// Sinon, on cherche l'index de depart et l'adresse de fin
	nFirst = GetMatchingObjectFirstIndex(aKey, nIndex);
	nLast = GetMatchingObjectLastIndex(aKey, nIndex);

	// Copie dans le tableau retourne des elements trouves
	oaResult = new ObjectArray;
	oaResult->SetSize(nLast - nFirst + 1);
	for (i = nFirst; i <= nLast; i++)
		oaResult->SetAt(i - nFirst, oaSorted.GetAt(i));
	return oaResult;
}

int QueryServices::ObjectsMatchingNumber(const Object* aKey) const
{
	int nIndex;
	int nFirst;
	int nLast;

	require(IsInit());

	// Recherche binaire de l'index d'un element du tableau
	nIndex = GetMatchingObjectIndex(aKey);

	// Si pas d'elememt trouve: on renvoi 0
	if (nIndex == -1)
		return 0;

	// Sinon, on cherche l'index de depart et l'adresse de fin
	nFirst = GetMatchingObjectFirstIndex(aKey, nIndex);
	nLast = GetMatchingObjectLastIndex(aKey, nIndex);

	// On renvoie le nombre d'elements trouves
	return nLast - nFirst + 1;
}

int QueryServices::ValuesNumber() const
{
	int nValuesNumber = 0;
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;

	require(IsInit());

	// Parcours du tableau trie, pour rechercher les valeurs
	for (nCurrent = 0; nCurrent < oaSorted.GetSize(); nCurrent++)
	{
		objectCurrent = oaSorted.GetAt(nCurrent);
		if (objectReference == NULL or fCompareFunction((void*)&objectCurrent, (void*)&objectReference))
		{
			nValuesNumber++;
			objectReference = objectCurrent;
		}
	}
	return nValuesNumber;
}

Object* QueryServices::Lookup(const Object* aKey) const
{
	require(IsInit());

	return oaSorted.Lookup(aKey);
}

boolean QueryServices::CheckDoubles() const
{
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;
	int nNbElements = 0;

	require(IsInit());

	// Parcours du tableau trie, pour rechercher les doubles
	for (nCurrent = 0; nCurrent < oaSorted.GetSize(); nCurrent++)
	{
		objectCurrent = oaSorted.GetAt(nCurrent);
		if (objectReference == NULL or fCompareFunction((void*)&objectCurrent, (void*)&objectReference))
		{
			if (nNbElements > 1)
				return false;
			objectReference = objectCurrent;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}
		if (nCurrent == oaSorted.GetSize() - 1)
			if (nNbElements > 1)
				return false;
	}
	return true;
}

ObjectArray* QueryServices::QueryStatistics() const
{
	Object* objectCurrent;
	Object* objectReference = NULL;
	int nCurrent;
	int nNbElements = 0;
	ObjectArray* oaStats;
	StatObject* stat;

	require(IsInit());

	// Parcours du tableau trie, pour imprimer les statistiques
	oaStats = new ObjectArray;
	for (nCurrent = 0; nCurrent < oaSorted.GetSize(); nCurrent++)
	{
		objectCurrent = oaSorted.GetAt(nCurrent);
		if (objectReference == NULL or fCompareFunction((void*)&objectCurrent, (void*)&objectReference))
		{
			if (nNbElements > 0)
			{
				stat = new StatObject;
				oaStats->Add(stat);
				stat->SetAttributeValue(fGetterFunction(objectReference));
				stat->SetItemNumber(nNbElements);
			}
			objectReference = objectCurrent;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}
		if (nCurrent == oaSorted.GetSize() - 1)
		{
			stat = new StatObject;
			oaStats->Add(stat);
			stat->SetAttributeValue(fGetterFunction(objectReference));
			stat->SetItemNumber(nNbElements);
		}
	}
	return oaStats;
}

void QueryServices::WriteStatistics(ostream& ost) const
{
	ObjectArray* oaStats;
	int nCurrent;

	require(IsInit());

	// Impression de l'en-tete
	ost << "Statistics on " << sClassName << ": by " << sAttributeName << "\t"
	    << "\n";
	ost << sAttributeName << "\t" << sClassName << " item number"
	    << "\n";

	// Recherche et impression du tableau de stats
	oaStats = QueryStatistics();
	for (nCurrent = 0; nCurrent < oaStats->GetSize(); nCurrent++)
		ost << *oaStats->GetAt(nCurrent);

	// Liberations
	oaStats->DeleteAll();
	delete oaStats;
}

///////////////////////////////////////////////////////

int QueryServices::GetMatchingObjectIndex(const Object* aKey) const
{
	int nInf;
	int nSup;
	int nIndex;
	int nDiff;
	Object* objectCurrent;

	require(IsInit());
	require(aKey != NULL);

	// Recherche binaire de l'index d'un element du tableau
	nInf = 0;
	nSup = oaSorted.GetSize() - 1;
	nDiff = -1;
	do
	{
		nIndex = (nInf + nSup) / 2;
		objectCurrent = oaSorted.GetAt(nIndex);
		nDiff = fCompareFunction((void*)&objectCurrent, (void*)&aKey);
		if (nDiff == 0)
			break;
		if (nDiff < 0)
			nInf = nIndex + 1;
		else
			nSup = nIndex - 1;
	} while (nInf <= nSup);

	// Si pas d'elememt trouve: on renvoi -1
	if (nDiff != 0)
		return -1;
	else
		return nIndex;
}

int QueryServices::GetMatchingObjectFirstIndex(const Object* aKey, int nStartingIndex) const
{
	int nInf;
	int nDiff;
	Object* objectCurrent;
	int i;

	require(IsInit());
	require(aKey != NULL);
	require(0 <= nStartingIndex and nStartingIndex < oaSorted.GetSize());

	// On cherche le premier index
	nInf = nStartingIndex;
	for (i = nStartingIndex; i >= 0; i--)
	{
		objectCurrent = oaSorted.GetAt(i);
		nDiff = fCompareFunction((void*)&objectCurrent, (void*)&aKey);
		if (nDiff == 0)
			nInf = i;
		else
			break;
	}
	return nInf;
}

int QueryServices::GetMatchingObjectLastIndex(const Object* aKey, int nStartingIndex) const
{
	int nSup;
	int nDiff;
	Object* objectCurrent;
	int i;

	require(IsInit());
	require(aKey != NULL);
	require(0 <= nStartingIndex and nStartingIndex < oaSorted.GetSize());

	// On cherche le dernier index
	nSup = nStartingIndex;
	for (i = nStartingIndex; i < oaSorted.GetSize(); i++)
	{
		objectCurrent = oaSorted.GetAt(i);
		nDiff = fCompareFunction((void*)&objectCurrent, (void*)&aKey);
		if (nDiff == 0)
			nSup = i;
		else
			break;
	}
	return nSup;
}

const ALString QueryServices::GetClassLabel() const
{
	return "Query services";
}

//////////////////////////////////////////////////////

void QueryServices::Test(ostream& ost)
{
	ObjectArray oaTest;
	SampleObject so1(10, "a");
	SampleObject so2(3, "a");
	SampleObject so3(3, "a");
	SampleObject so4(5, "a");
	SampleObject so5(1, "a");
	SampleObject soTest;
	QueryServices qsTest;
	int i;
	ObjectArray* oaResult;
	SampleObject* soResult;

	// Initialisation du tableau de test, et des services de query
	oaTest.Add(&so1);
	oaTest.Add(&so2);
	oaTest.Add(&so3);
	oaTest.Add(&so4);
	oaTest.Add(&so5);
	qsTest.Init(SampleObjectCompare, SampleObjectGetInt, "SampleObject", "Int");
	qsTest.LoadObjectArray(&oaTest);

	// Test des fonctionnalites
	ost << "GetterFunction()" << so2 << endl;
	ost << "  " << SampleObjectGetInt(&so2) << endl;

	ost << "QueryAllObjects()" << endl;
	oaResult = qsTest.QueryAllObjects();
	for (i = 0; i < oaResult->GetSize(); i++)
		ost << "  " << *cast(SampleObject*, oaResult->GetAt(i)) << "\n";
	delete oaResult;

	ost << "QueryObjectsMatching()" << soTest << endl;
	oaResult = qsTest.QueryObjectsMatching(&soTest);
	for (i = 0; i < oaResult->GetSize(); i++)
		ost << "  " << *cast(SampleObject*, oaResult->GetAt(i)) << "\n";
	delete oaResult;

	ost << "QueryObjectsMatching()" << so2 << endl;
	oaResult = qsTest.QueryObjectsMatching(&so2);
	for (i = 0; i < oaResult->GetSize(); i++)
		ost << "  " << *cast(SampleObject*, oaResult->GetAt(i)) << "\n";
	delete oaResult;

	ost << "QueryObjectsMatching()" << so1 << endl;
	oaResult = qsTest.QueryObjectsMatching(&so1);
	for (i = 0; i < oaResult->GetSize(); i++)
		ost << "  " << *cast(SampleObject*, oaResult->GetAt(i)) << "\n";
	delete oaResult;

	ost << "Lookup()" << soTest << endl;
	soResult = cast(SampleObject*, qsTest.Lookup(&soTest));
	if (soResult == NULL)
		ost << "  KO"
		    << "\n";
	else
		ost << "  OK"
		    << "\n";

	ost << "Lookup()" << so2 << endl;
	soResult = cast(SampleObject*, qsTest.Lookup(&so2));
	if (soResult == NULL)
		ost << "  KO"
		    << "\n";
	else
		ost << "  OK"
		    << "\n";

	ost << "Lookup()" << so1 << endl;
	soResult = cast(SampleObject*, qsTest.Lookup(&so1));
	if (soResult == NULL)
		ost << "  KO"
		    << "\n";
	else
		ost << "  OK"
		    << "\n";

	ost << "CheckDoubles()" << endl;
	if (qsTest.CheckDoubles())
		ost << "  OK"
		    << "\n";
	else
		ost << "  KO"
		    << "\n";

	ost << "WriteStatistics()" << endl;
	qsTest.WriteStatistics(ost);

	ost << "Total Number: " << qsTest.GetSize() << endl;

	ost << "Values Number: " << qsTest.ValuesNumber() << endl;

	ost << "Number matching " << soTest << ": " << qsTest.ObjectsMatchingNumber(&soTest) << endl;
	ost << "Number matching " << so2 << ": " << qsTest.ObjectsMatchingNumber(&so2) << endl;
	ost << "Number matching " << so1 << ": " << qsTest.ObjectsMatchingNumber(&so1) << endl;
}

const ALString SampleObjectGetInt(const Object* object)
{
	return IntToString(cast(SampleObject*, object)->GetInt());
}

//////////////////////////////////////////////////
// Implementation de la classe StatObject

const ALString StatObject::GetClassLabel() const
{
	return "Stat object";
}
