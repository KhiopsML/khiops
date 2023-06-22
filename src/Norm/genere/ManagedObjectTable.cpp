// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ManagedObjectTable.h"

ManagedObjectTable::ManagedObjectTable(ManagedObject* theManagedObjectClass)
{
	require(theManagedObjectClass != NULL);
	require(theManagedObjectClass->GetCompareKeyFunction() != NULL);

	managedObjectClass = theManagedObjectClass;
	bUsingHeaderLine = true;
	nUpdateNumber = 0;
	nAllObjectsFreshness = 0;
	oaAllObjects.SetCompareFunction(managedObjectClass->GetCompareKeyFunction());
}

ManagedObjectTable::~ManagedObjectTable() {}

ManagedObject* ManagedObjectTable::GetManagedObjectClass()
{
	return managedObjectClass;
}

///////////////////////////////////////////////////////////

boolean ManagedObjectTable::Load(fstream& fst)
{
	return LoadFields(fst, managedObjectClass->GetStoredFieldIndexes());
}

boolean ManagedObjectTable::Load(const ALString& sFileName)
{
	return LoadFields(sFileName, managedObjectClass->GetStoredFieldIndexes());
}

boolean ManagedObjectTable::LoadFields(fstream& fst, IntVector* ivMandatoryFieldIndexes)
{
	boolean bOk = true;
	boolean bLineOk;
	ManagedObject* current;
	IntVector ivStoredFieldIndexes;
	IntVector* ivFieldIndexes;
	int nLineIndex = 0;
	ALString sTmp;

	require(ivMandatoryFieldIndexes != NULL);

	// Verification de coherence:
	// les champs obligatoires doivent etre stockes
	debug(int i; int nFieldIndex; for (i = 0; i < ivMandatoryFieldIndexes->GetSize(); i++) {
		nFieldIndex = ivMandatoryFieldIndexes->GetAt(i);
		require(managedObjectClass->GetFieldStored(nFieldIndex));
	})

	    // Analyse de la premiere ligne
	    if (bUsingHeaderLine)
	{
		// Lecture de la premiere ligne
		nLineIndex++;
		LoadHeaderLine(fst, &ivStoredFieldIndexes);

		// Verification de presence des champs obligatoires
		bOk = CheckMandatoryFields(&ivStoredFieldIndexes, ivMandatoryFieldIndexes);
		if (not bOk)
			return false;
	}

	// Specification des champs a lire
	if (bUsingHeaderLine)
		ivFieldIndexes = &ivStoredFieldIndexes;
	else
		ivFieldIndexes = ivMandatoryFieldIndexes;

	// Lecture des autres lignes
	Global::ActivateErrorFlowControl();
	while (not fst.eof())
	{
		current = managedObjectClass->CloneManagedObject();
		nLineIndex++;
		bLineOk = current->LoadFields(fst, ivFieldIndexes) != NULL;
		if (bLineOk)
		{
			if (not Insert(current))
			{
				current->AddError("Existing record in table for this key");
				delete current;
				bOk = false;
			}
		}
		else
		{
			if (not fst.eof())
				AddWarning(sTmp + "Line" + " " + IntToString(nLineIndex) + " " + "is empty");
			delete current;
		}
	}
	Global::DesactivateErrorFlowControl();
	return bOk;
}

boolean ManagedObjectTable::LoadFields(const ALString& sFileName, IntVector* ivMandatoryFieldIndexes)
{
	fstream fst;
	boolean bOk;

	require(ivMandatoryFieldIndexes != NULL);

	bOk = FileService::OpenInputFile(sFileName, fst);
	if (bOk)
	{
		bOk = LoadFields(fst, ivMandatoryFieldIndexes);
		fst.close();
	}
	return bOk;
}

void ManagedObjectTable::Unload(ostream& ost)
{
	UnloadFields(ost, managedObjectClass->GetStoredFieldIndexes());
}

void ManagedObjectTable::Unload(const ALString& sFileName)
{
	UnloadFields(sFileName, managedObjectClass->GetStoredFieldIndexes());
}

void ManagedObjectTable::UnloadFields(ostream& ost, IntVector* ivFieldIndexes)
{
	ManagedObject* current;
	int i;

	require(ivFieldIndexes != NULL);

	// Sortie de la ligne de libelles
	if (bUsingHeaderLine)
	{
		UnloadHeaderLine(ost, ivFieldIndexes);
		ost << "\n";
	}

	// Sortie de chaque ligne
	for (i = 0; i < GetSize(); i++)
	{
		current = GetAt(i);
		current->UnloadFields(ost, ivFieldIndexes);
		ost << "\n";
	}
}

void ManagedObjectTable::UnloadFields(const ALString& sFileName, IntVector* ivFieldIndexes)
{
	fstream fst;
	boolean bOk;

	require(ivFieldIndexes != NULL);

	bOk = FileService::OpenOutputFile(sFileName, fst);
	if (bOk)
	{
		UnloadFields(fst, ivFieldIndexes);
		fst.close();
	}
}

///////////////////////////////////////////////////////////

void ManagedObjectTable::SetUsingHeaderLine(boolean bValue)
{
	bUsingHeaderLine = bValue;
}

boolean ManagedObjectTable::GetUsingHeaderLine() const
{
	return bUsingHeaderLine;
}

char* ManagedObjectTable::LoadHeaderLine(fstream& fst, IntVector* ivHeaderLoadedFieldIndexes)
{
	static char sFirstLineRef[10000];
	char sFirstLine[sizeof(sFirstLineRef)];
	char* sStorageName;
	char* sNextStorageName;
	int nStorageNameNumber;
	int i;
	int j;
	int nFieldIndex;

	require(ivHeaderLoadedFieldIndexes != NULL);

	// Lecture d'une ligne
	sFirstLineRef[0] = '\0';
	fst.getline(sFirstLineRef, sizeof(sFirstLineRef));

	// Recopie de la ligne dans un buffer de travail
	// (dont le contenu sera altere)
	p_strcpy(sFirstLine, sFirstLineRef);

	// Parcours des libelles de la premiere ligne
	// pour chercher les index de champs correspondants
	nStorageNameNumber = GetTokenSeparatorCount(sFirstLine, managedObjectClass->GetSeparator()) + 1;
	sStorageName = sFirstLine;
	ivHeaderLoadedFieldIndexes->SetSize(0);
	for (i = 0; i < nStorageNameNumber; i++)
	{
		// Recherche du premier libelle
		sNextStorageName = NextToken(sStorageName, managedObjectClass->GetSeparator());

		// Recherche de l'index du champs correspondant
		nFieldIndex = -1;
		for (j = 0; j < managedObjectClass->GetFieldNumber(); j++)
		{
			if (managedObjectClass->GetFieldStorageNameAt(j) == (const char*)sStorageName)
			{
				nFieldIndex = j;
				break;
			}
		}
		ivHeaderLoadedFieldIndexes->Add(nFieldIndex);

		// Passage au libelle suivant
		sStorageName = sNextStorageName;
	}

	return sFirstLineRef;
}

boolean ManagedObjectTable::CheckMandatoryFields(IntVector* ivFieldIndexes, IntVector* ivMandatoryFieldIndexes)
{
	int i;
	int j;
	boolean bFound;
	boolean bOk;
	ALString sTmp;

	require(ivFieldIndexes != NULL);
	require(ivMandatoryFieldIndexes != NULL);

	// Verification de presence des champs obligatoires
	bOk = true;
	for (i = 0; i < ivMandatoryFieldIndexes->GetSize(); i++)
	{
		// Recherche parmi les champs stockes
		bFound = false;
		for (j = 0; j < ivFieldIndexes->GetSize(); j++)
		{
			if (ivMandatoryFieldIndexes->GetAt(i) == ivFieldIndexes->GetAt(j))
			{
				bFound = true;
				break;
			}
		}

		// Diagnostique d'erreur si probleme
		if (not bFound)
		{
			AddError(sTmp + "Field" + " " +
				 managedObjectClass->GetFieldStorageNameAt(ivMandatoryFieldIndexes->GetAt(i)) +
				 " not found in file header line");
			bOk = false;
		}
	}
	return bOk;
}

void ManagedObjectTable::UnloadHeaderLine(ostream& ost, IntVector* ivFieldIndexes)
{
	int i;
	int nFieldIndex;

	require(ivFieldIndexes != NULL);

	// Sortie de la ligne de libelles
	for (i = 0; i < ivFieldIndexes->GetSize(); i++)
	{
		nFieldIndex = ivFieldIndexes->GetAt(i);
		if (i > 0)
			ost << managedObjectClass->GetSeparator();
		if (nFieldIndex != -1)
			ost << managedObjectClass->GetFieldStorageNameAt(nFieldIndex);
	}
}

///////////////////////////////////////////////////////////

ManagedObject* ManagedObjectTable::Lookup(const ALString& sKey) const
{
	return cast(ManagedObject*, dicKeyIndex.Lookup(sKey));
}

boolean ManagedObjectTable::Insert(ManagedObject* newObject)
{
	require(newObject != NULL);

	if (dicKeyIndex.Lookup(newObject->GetKey()) == NULL)
	{
		dicKeyIndex.SetAt(newObject->GetKey(), newObject);
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

boolean ManagedObjectTable::Remove(const ALString& sKey)
{
	if (dicKeyIndex.RemoveKey(sKey))
	{
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

int ManagedObjectTable::GetSize() const
{
	return cast(ManagedObjectTable*, this)->GetAllObjects()->GetSize();
}

ManagedObject* ManagedObjectTable::GetAt(int i) const
{
	require(0 <= i and i < GetSize());
	return cast(ManagedObject*, cast(ManagedObjectTable*, this)->GetAllObjects()->GetAt(i));
}

void ManagedObjectTable::SetCompareFunction(CompareFunction fCompare)
{
	if (fCompare == NULL)
		oaAllObjects.SetCompareFunction(managedObjectClass->GetCompareKeyFunction());
	else
		oaAllObjects.SetCompareFunction(fCompare);
	oaAllObjects.Sort();
}

CompareFunction ManagedObjectTable::GetCompareFunction()
{
	assert(oaAllObjects.GetCompareFunction() != NULL);
	if (oaAllObjects.GetCompareFunction() == managedObjectClass->GetCompareKeyFunction())
		return NULL;
	else
		return oaAllObjects.GetCompareFunction();
}

void ManagedObjectTable::RemoveAll()
{
	dicKeyIndex.RemoveAll();
	nUpdateNumber++;
}

void ManagedObjectTable::DeleteAll()
{
	dicKeyIndex.DeleteAll();
	nUpdateNumber++;
}

boolean ManagedObjectTable::Check() const
{
	boolean bOk = true;
	int i;
	ManagedObject* managedObject;

	// Parcours des objets de la table
	Global::ActivateErrorFlowControl();
	for (i = 0; i < GetSize(); i++)
	{
		managedObject = cast(ManagedObject*, GetAt(i));
		bOk = managedObject->Check() and bOk;
	}
	Global::DesactivateErrorFlowControl();

	return bOk;
}

void ManagedObjectTable::ExportObjectArray(ObjectArray* oaResult) const
{
	require(oaResult != NULL);

	oaResult->SetSize(GetSize());

	// On passe par les methodes GetAt, qui forcent le tri des objets
	for (int nI = 0; nI < GetSize(); nI++)
		oaResult->SetAt(nI, GetAt(nI));
	ensure(oaResult->GetSize() == dicKeyIndex.GetCount());
}

void ManagedObjectTable::ExportObjectDictionary(ObjectDictionary* odResult) const
{
	require(odResult != NULL);

	odResult->CopyFrom(&dicKeyIndex);
	ensure(odResult->GetCount() == dicKeyIndex.GetCount());
}

void ManagedObjectTable::ExportObjectList(ObjectList* olResult) const
{
	require(olResult != NULL);

	// On passe par les methodes GetAt, qui forcent le tri des objets
	olResult->RemoveAll();
	for (int nI = 0; nI < GetSize(); nI++)
		olResult->AddTail(GetAt(nI));
	ensure(olResult->GetCount() == dicKeyIndex.GetCount());
}

const ALString ManagedObjectTable::GetClassLabel() const
{
	return managedObjectClass->GetClassLabel() + " table";
}

ObjectArray* ManagedObjectTable::GetAllObjects()
{
	if (nAllObjectsFreshness < nUpdateNumber)
	{
		dicKeyIndex.ExportObjectArray(&oaAllObjects);
		oaAllObjects.Sort();
		nAllObjectsFreshness = nUpdateNumber;
	}
	return &oaAllObjects;
}