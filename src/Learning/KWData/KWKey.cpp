// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	const int nFieldMaxLength = 10;
	const int nTotalMaxLength = 40;
	ALString sLabel;
	ALString sFieldLabel;
	int i;

	sLabel = "[";
	for (i = 0; i < svFields.GetSize(); i++)
	{
		// Libelle lie a un champ de la cle, en le tronquant s'il est trop long
		if (svFields.GetAt(i).GetLength() <= nFieldMaxLength)
			sFieldLabel = svFields.GetAt(i);
		else
			sFieldLabel = svFields.GetAt(i).Left(nFieldMaxLength) + "...";

		// Ajout du libelle du champ de cle
		sLabel += sFieldLabel;
		if (i < svFields.GetSize() - 1)
			sLabel += ", ";

		// Arret si taille globale superieure au seuil fixe
		if (sLabel.GetLength() >= nTotalMaxLength)
		{
			sLabel += "...";
			break;
		}
	}
	sLabel += "]";
	return sLabel;
}

void KWKey::BuildDistinctObjectLabels(const KWKey* otherKey, ALString& sObjectLabel, ALString& sOtherObjectLabel)
{
	const int nFieldMaxLength = 10;
	const int nPreviousDistinctFieldMaxNumber = 2;
	ALString sFieldLabel;
	ALString sOtherFieldLabel;
	ALString sField;
	ALString sOtherField;
	int nFirstDistinctField;
	int nFirstDistinctChar;
	int i;

	assert(svFields.GetSize() == otherKey->GetSize());

	// Cas de libelles utilisateurs deja distincts
	if (GetObjectLabel() != otherKey->GetObjectLabel())
	{
		sObjectLabel = GetObjectLabel();
		sOtherObjectLabel = otherKey->GetObjectLabel();
	}
	// Sinon construction de libelles distincts
	else
	{
		// Recherche du 1er champ qui differe
		nFirstDistinctField = 0;
		while (nFirstDistinctField < svFields.GetSize() and
		       svFields.GetAt(nFirstDistinctField) == otherKey->GetAt(nFirstDistinctField))
			nFirstDistinctField++;

		// Recherche du 1er caractere qui differe pour ce champ
		nFirstDistinctChar = 0;
		sField = svFields.GetAt(nFirstDistinctField);
		sOtherField = otherKey->GetAt(nFirstDistinctField);
		while (nFirstDistinctChar < sField.GetLength() and nFirstDistinctChar < sOtherField.GetLength() and
		       sField.GetAt(nFirstDistinctChar) == sOtherField.GetAt(nFirstDistinctChar))
			nFirstDistinctChar++;

		// Ecriture des libelles distincts des deux cles
		sObjectLabel = "[";
		sOtherObjectLabel = "[";
		i = 0;
		// Affichage des champs precedant le champ qui permet de distinguer les cles
		while (i < nFirstDistinctField)
		{
			// Initialisation des deux champs courants
			sField = svFields.GetAt(i);
			sOtherField = otherKey->GetAt(i);

			// Ajout du separateur a partir du second champ
			if (i > 0)
			{
				sObjectLabel += ",";
				sOtherObjectLabel += ",";
			}

			// Libelle explicite des premiers champs de chaque cle, en le tronquant s'il est trop long
			if (i < nPreviousDistinctFieldMaxNumber)
			{
				if (sField.GetLength() <= nFieldMaxLength)
					sFieldLabel = sField;
				else
					sFieldLabel = sField.Left(nFieldMaxLength) + "...";
				if (sOtherField.GetLength() <= nFieldMaxLength)
					sOtherFieldLabel = sOtherField;
				else
					sOtherFieldLabel = sOtherField.Left(nFieldMaxLength) + "...";
			}
			else
			{
				sFieldLabel = "...";
				sOtherFieldLabel = "...";
			}
			sObjectLabel += sFieldLabel;
			sOtherObjectLabel += sOtherFieldLabel;
			i++;
		}
		// Libelles distincts du 1er champ qui differe entre les deux cles
		// Initialisation des deux champs courants
		sField = svFields.GetAt(nFirstDistinctField);
		sOtherField = otherKey->GetAt(nFirstDistinctField);
		// Ajout du separateur avant ce champ
		if (i > 0)
		{
			sObjectLabel += ",";
			sOtherObjectLabel += ",";
		}
		// Debut du libelle dans le cas ou le caractere qui les distingue est superieure a la taille max affichee par portion de libelle
		if (nFieldMaxLength < nFirstDistinctChar)
		{
			sFieldLabel = sField.Left(nFieldMaxLength);
			sOtherFieldLabel = sOtherField.Left(nFieldMaxLength);
			if (nFieldMaxLength + 1 < nFirstDistinctChar)
			{
				sFieldLabel += "...";
				sOtherFieldLabel += "...";
			}
		}
		else
		{
			sFieldLabel = sField.Left(nFirstDistinctChar);
			sOtherFieldLabel = sOtherField.Left(nFirstDistinctChar);
		}

		// Suite du libelle debutant par le caractere qui les distingue
		if (sField.GetLength() - nFirstDistinctChar > nFieldMaxLength)
			sFieldLabel += sField.Mid(nFirstDistinctChar, nFieldMaxLength) + "...";
		else if (nFirstDistinctChar < sField.GetLength())
			sFieldLabel += sField.Right(sField.GetLength() - nFirstDistinctChar);
		else if (nFirstDistinctChar == sField.GetLength())
			sFieldLabel += sField.Right(1);

		if (sOtherField.GetLength() - nFirstDistinctChar > nFieldMaxLength)
			sOtherFieldLabel += sOtherField.Mid(nFirstDistinctChar, nFieldMaxLength) + "...";
		else if (nFirstDistinctChar < sOtherField.GetLength())
			sOtherFieldLabel += sOtherField.Right(sOtherField.GetLength() - nFirstDistinctChar);
		else if (nFirstDistinctChar == sOtherField.GetLength())
			sOtherFieldLabel += sOtherField.Right(1);
		sObjectLabel += sFieldLabel;
		sOtherObjectLabel += sOtherFieldLabel;

		// Libelles minimaux pour les champs suivants le 1er champ qui distingue les cles
		i = nFirstDistinctField + 1;
		while (i < svFields.GetSize())
		{
			sObjectLabel += ",...";
			sOtherObjectLabel += ",...";
			i++;
		}

		// Cloture des libelles
		sObjectLabel += "]";
		sOtherObjectLabel += "]";
	}
}

void KWKey::Test()
{

	KWKey key1;
	KWKey key2;
	ALString sLabel;
	ALString sOtherLabel;

	// Test 1 : 2 champs deja distincts avec libelle utilisateur par defaut
	key1.SetSize(2);
	key1.SetAt(0, "aaaa");
	key1.SetAt(1, "bbbbb");

	key2.SetSize(2);
	key2.SetAt(0, "aaaaA");
	key2.SetAt(1, "bbbbbB");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 1\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << "]" << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "," << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 2 : Deux champs dont le 1er est distinct
	key1.SetSize(2);
	key1.SetAt(0, "aaaaaaaaaaaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbbbbbbbbbbb");

	key2.SetSize(2);
	key2.SetAt(0, "aaaaaaaaaaaaaaaaaaaaA");
	key2.SetAt(1, "bbbbbbbbbbbbbbbbbbbbB");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 2\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << "]" << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 3 : Deux champs dont le 2nd champ est distinct
	key1.SetSize(2);
	key1.SetAt(0, "aaaaaaaaaaaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbbbbbbbbbbb");

	key2.SetSize(2);
	key2.SetAt(0, "aaaaaaaaaaaaaaaaaaaa");
	key2.SetAt(1, "bbbbbbbbbbbbbbbbbbbbB");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 3\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << "]" << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 4 : Trois champs dont 2nd champ distinct
	key1.SetSize(3);
	key1.SetAt(0, "aaaaaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbbbbb");
	key1.SetAt(2, "cccccccccccccc");

	key2.SetSize(3);
	key2.SetAt(0, "aaaaaaaaaaaaaa");
	key2.SetAt(1, "bbbbbbbbbbbbbb");
	key2.SetAt(2, "ccccccccccccccC");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 4\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << ", " << key1.GetAt(2) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << ", " << key2.GetAt(2) << "]" << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 5 : Trois champs dont le 3eme distinct
	key2.SetSize(3);
	key2.SetAt(0, "aaaaaaaaaaaaaa");
	key2.SetAt(1, "bbbbbbbbbbbbbb");
	key2.SetAt(2, "cccccccccccccc");

	key1.SetSize(3);
	key1.SetAt(0, "aaaaaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbbbbb");
	key1.SetAt(2, "ccccccccccccccC");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 5\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << ", " << key1.GetAt(2) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << ", " << key2.GetAt(2) << "]" << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 6 : 4 champs dont le 3eme distinct
	key1.SetSize(4);
	key1.SetAt(0, "aaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbb");
	key1.SetAt(2, "ccccccccccc");
	key1.SetAt(3, "ddddddddddd");

	key2.SetSize(4);
	key2.SetAt(0, "aaaaaaaaaaa");
	key2.SetAt(1, "bbbbbbbbbbb");
	key2.SetAt(2, "ccccccccccC");
	key2.SetAt(3, "ddddddddddd");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 6\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << ", " << key1.GetAt(2) << ", "
	     << key1.GetAt(3) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << ", " << key2.GetAt(2) << ", " << key2.GetAt(3) << "]"
	     << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;

	// Test 7 : 4 champs dont le 4eme distinct
	key1.SetSize(4);
	key1.SetAt(0, "aaaaaaaaaaa");
	key1.SetAt(1, "bbbbbbbbbbb");
	key1.SetAt(2, "ccccccccccc");
	key1.SetAt(3, "ddddddddddd");

	key2.SetSize(4);
	key2.SetAt(0, "aaaaaaaaaaa");
	key2.SetAt(1, "bbbbbbbbbbb");
	key2.SetAt(2, "ccccccccccc");
	key2.SetAt(3, "ddddddddddD");

	key1.BuildDistinctObjectLabels(&key2, sLabel, sOtherLabel);
	cout << "Test 7\t" << endl;
	cout << "Champs d'origine\t[" << key1.GetAt(0) << ", " << key1.GetAt(1) << ", " << key1.GetAt(2) << ", "
	     << key1.GetAt(3) << "],"
	     << "[" << key2.GetAt(0) << ", " << key2.GetAt(1) << ", " << key2.GetAt(2) << ", " << key2.GetAt(3) << "]"
	     << endl;
	cout << "Libelles par defaut\t" << key1.GetObjectLabel() << "\t;" << key2.GetObjectLabel() << endl;
	cout << "Libelles distincts\t" << sLabel << "\t;" << sOtherLabel << endl;
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
