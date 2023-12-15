// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKey.h"

//////////////////////////////////
// Implementation de Key

KWKey::KWKey()
{
	svFields.SetSize(0);
}

KWKey::~KWKey() {}

KWKey* KWKey::Clone() const
{
	KWKey* clone = new KWKey;
	clone->CopyFrom(this);
	return clone;
}

void KWKey::CopyFrom(const KWKey* keySource)
{
	check(keySource);
	svFields.CopyFrom(&keySource->svFields);
}

void KWKey::Write(ostream& ost) const
{
	ost << GetObjectLabel();
}

const ALString KWKey::GetClassLabel() const
{
	return "Key";
}

const ALString KWKey::GetObjectLabel() const
{
	ALString sLabel;
	const int nFieldMaxLength = 20;
	const int nFieldMaxNumber = 10;
	ALString sFieldLabel;
	int i;

	sLabel = "[";
	for (i = 0; i < svFields.GetSize(); i++)
	{
		// Arret si trop de champs cles
		if (i >= nFieldMaxNumber)
		{
			sLabel += "...";
			break;
		}

		// Libelle lie a un champ de la cle, en le tronquant s'il est trop long
		if (svFields.GetAt(i).GetLength() <= nFieldMaxLength)
			sFieldLabel = svFields.GetAt(i);
		else
			sFieldLabel = svFields.GetAt(i).Left(nFieldMaxLength) + "...";

		// Ajout du libelle du champ de cle
		sLabel += sFieldLabel;
		if (i < svFields.GetSize() - 1)
			sLabel += ", ";
	}
	sLabel += "]";
	return sLabel;
}

int KWKeyCompare(const void* elem1, const void* elem2)
{
	KWKey* k1;
	KWKey* k2;

	// Acces aux objets
	k1 = cast(KWKey*, *(Object**)elem1);
	k2 = cast(KWKey*, *(Object**)elem2);

	assert(k1->GetSize() == k2->GetSize());
	return k1->Compare(k2);
}

//////////////////////////////////
// Implementation de PLShared_Key

PLShared_Key::PLShared_Key() {}

PLShared_Key::~PLShared_Key() {}

void PLShared_Key::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWKey* key = cast(KWKey*, o);
	require(serializer->IsOpenForWrite());
	serializer->PutStringVector(&key->svFields);
}

void PLShared_Key::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWKey* key = cast(KWKey*, o);
	require(serializer->IsOpenForRead());
	require(key->svFields.GetSize() == 0);
	serializer->GetStringVector(&key->svFields);
}

//////////////////////////////////
// Implementation de KeyPosition

KWKeyPosition::KWKeyPosition()
{
	lLineIndex = 0;
	lLinePosition = 0;
}

KWKeyPosition::~KWKeyPosition() {}

KWKeyPosition* KWKeyPosition::Clone() const
{
	KWKeyPosition* clone = new KWKeyPosition;
	clone->CopyFrom(this);
	return clone;
}

void KWKeyPosition::CopyFrom(const KWKeyPosition* keySource)
{
	require(keySource != NULL);

	key.CopyFrom(&keySource->key);
	lLineIndex = keySource->lLineIndex;
	lLinePosition = keySource->lLinePosition;
}

void KWKeyPosition::CollectKeys(const ObjectArray* oaKeyPositions, ObjectArray* oaKeys)
{
	int i;
	KWKeyPosition* keyPosition;
	KWKey* key;

	require(oaKeyPositions != NULL);
	require(oaKeys != NULL);
	require(oaKeys->GetSize() == 0);

	for (i = 0; i < oaKeyPositions->GetSize(); i++)
	{
		keyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i));
		key = keyPosition->GetKey();
		check(key);
		oaKeys->Add(key);
	}
	ensure(oaKeys->GetSize() == oaKeyPositions->GetSize());
}

void KWKeyPosition::CollectClonedKeys(const ObjectArray* oaKeyPositions, ObjectArray* oaClonedKeys)
{
	int i;
	KWKeyPosition* keyPosition;
	KWKey* key;

	require(oaKeyPositions != NULL);
	require(oaClonedKeys != NULL);
	require(oaClonedKeys->GetSize() == 0);

	for (i = 0; i < oaKeyPositions->GetSize(); i++)
	{
		keyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i));
		key = keyPosition->GetKey();
		check(key);
		oaClonedKeys->Add(key->Clone());
	}
	ensure(oaClonedKeys->GetSize() == oaKeyPositions->GetSize());
}

void KWKeyPosition::CleanKeys(const ObjectArray* oaKeyPositions)
{
	int i;
	KWKeyPosition* keyPosition;

	require(oaKeyPositions != NULL);

	for (i = 0; i < oaKeyPositions->GetSize(); i++)
	{
		keyPosition = cast(KWKeyPosition*, oaKeyPositions->GetAt(i));
		keyPosition->GetKey()->SetSize(0);
	}
}

void KWKeyPosition::Write(ostream& ost) const
{
	ost << GetObjectLabel();
}

const ALString KWKeyPosition::GetClassLabel() const
{
	return "Key position";
}

const ALString KWKeyPosition::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = key.GetObjectLabel();
	sLabel = sLabel + " at line " + LongintToString(lLineIndex);
	return sLabel;
}

int KWKeyPositionCompare(const void* elem1, const void* elem2)
{
	KWKeyPosition* kp1;
	KWKeyPosition* kp2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux objets
	kp1 = cast(KWKeyPosition*, *(Object**)elem1);
	kp2 = cast(KWKeyPosition*, *(Object**)elem2);

	// Comparaison
	nCompare = kp1->GetKey()->Compare(kp2->GetKey());
	return nCompare;
}

//////////////////////////////////
// Implementation de PLShared_KeyPosition

PLShared_KeyPosition::PLShared_KeyPosition() {}

PLShared_KeyPosition::~PLShared_KeyPosition() {}

void PLShared_KeyPosition::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLShared_Key shared_key;
	KWKeyPosition* keyPosition = cast(KWKeyPosition*, o);

	require(serializer->IsOpenForWrite());

	// On sous-traite la serialisation de la cle
	shared_key.SerializeObject(serializer, keyPosition->GetKey());

	// Serialisation du reste des attributs
	serializer->PutLongint(keyPosition->lLinePosition);
	serializer->PutLongint(keyPosition->lLineIndex);
}

void PLShared_KeyPosition::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLShared_Key shared_key;
	KWKeyPosition* keyPosition = cast(KWKeyPosition*, o);

	require(serializer->IsOpenForRead());

	// On sous-traite la deserialisation de la cle
	shared_key.DeserializeObject(serializer, keyPosition->GetKey());

	// Deserialisation du reste des attributs
	keyPosition->lLinePosition = serializer->GetLongint();
	keyPosition->lLineIndex = serializer->GetLongint();
}
