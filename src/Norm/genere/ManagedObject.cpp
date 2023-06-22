// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ManagedObject.h"

ManagedObject::ManagedObject() {}

ManagedObject::~ManagedObject() {}

char* ManagedObject::Load(fstream& fst)
{
	return LoadFields(fst, GetStoredFieldIndexes());
}

void ManagedObject::Unload(ostream& ost) const
{
	UnloadFields(ost, GetStoredFieldIndexes());
}

char* ManagedObject::LoadFields(fstream& fst, IntVector* ivFieldIndexes)
{
	static char sBufferRef[10000];
	char sBuffer[sizeof(sBufferRef)];
	char* sToken;
	char* sNextToken;
	int i;
	int nFieldIndex;

	require(ivFieldIndexes != NULL);

	// Lecture d'une ligne
	if (!fst.getline(sBufferRef, sizeof(sBufferRef)))
		return NULL;

	// Test de ligne vide
	if (AreTokensEmpty(sBufferRef, GetSeparator()))
		return NULL;

	// Recopie de la ligne dans un buffer de travail
	// (dont le contenu sera altere)
	p_strcpy(sBuffer, sBufferRef);

	// Parcours des champs a lire
	sToken = sBuffer;
	for (i = 0; i < ivFieldIndexes->GetSize(); i++)
	{
		nFieldIndex = ivFieldIndexes->GetAt(i);
		assert(-1 <= nFieldIndex and nFieldIndex < GetFieldNumber());

		// Lecture du champ
		sNextToken = NextToken(sToken, GetSeparator());
		if (nFieldIndex != -1 and GetFieldStored(nFieldIndex))
			SetFieldAt(nFieldIndex, sToken);
		sToken = sNextToken;
	}
	return sBufferRef;
}

void ManagedObject::UnloadFields(ostream& ost, IntVector* ivFieldIndexes) const
{
	int i;
	int nFieldIndex;

	require(ivFieldIndexes != NULL);

	// Parcours des champs a ecrire
	for (i = 0; i < ivFieldIndexes->GetSize(); i++)
	{
		nFieldIndex = ivFieldIndexes->GetAt(i);
		assert(-1 <= nFieldIndex and nFieldIndex < GetFieldNumber());

		// Ecriture du champ
		if (i > 0)
			ost << GetSeparator();
		if (nFieldIndex != -1)
			ost << GetFieldAt(nFieldIndex);
	}
}

const ALString& ManagedObject::GetKey() const
{
	static ALString sKey;

	sKey = PointerToString(this);
	return sKey;
}

const ALString ManagedObjectGetKey(const Object* object)
{
	return cast(ManagedObject*, object)->GetKey();
}