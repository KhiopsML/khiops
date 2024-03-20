// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedVector.h"

/////////////////////////////////////////////
// Implementation de la classe PLShared_StringVector

PLShared_StringVector::PLShared_StringVector() {}

PLShared_StringVector::~PLShared_StringVector() {}

void PLShared_StringVector::Test()
{
	PLShared_StringVector shared_svIn;
	PLShared_StringVector shared_svOut;
	PLShared_StringVector shared_sv;
	PLSerializer serializer;
	StringVector svToSerialize;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_svIn.bIsDeclared = true;
	shared_svOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	shared_svIn.Add("Un");
	shared_svIn.Add("test");
	shared_svIn.Add("de");
	shared_svIn.Add("shared");
	shared_svIn.Add("string");
	shared_svIn.Add("array");
	shared_svIn.Add("Une chaine de caracteres pour tester la serialisation.\nUne autre avec un retour a la ligne, "
			"quelques signes de \"ponctuations\" et des '\t' tabulations.");

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_svIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_svOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value:\n" << shared_svIn << endl;
	cout << "Serialized value:\n" << shared_svOut << endl;
	;
	cout << "Serialized form:\n" << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_svOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object:\n" << shared_svOut << endl;

	////////////////////////////////////////////////////////////////////////////////
	// Utilisation des methodes SerializeObject et DeserializeObject

	svToSerialize.Add("un");
	svToSerialize.Add("2");
	svToSerialize.Add("trois \t quatre");
	cout << endl;
	cout << "initial value : " << svToSerialize << endl;

	// Serialisation
	serializer.Initialize();
	serializer.OpenForWrite(NULL);
	shared_sv.SerializeObject(&serializer, &svToSerialize);
	serializer.Close();
	svToSerialize.SetSize(0);

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_sv.DeserializeObject(&serializer, &svToSerialize);
	serializer.Close();

	cout << "Deserialized value : " << svToSerialize << endl;
}

void PLShared_StringVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	serializer->PutStringVector(cast(const StringVector*, o));
}

void PLShared_StringVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	serializer->GetStringVector(cast(StringVector*, o));
}

Object* PLShared_StringVector::Create() const
{
	return new StringVector;
}

StringVector* PLShared_StringVector::GetStringVector()
{
	require(bIsWritable);
	require(bIsDeclared);

	return cast(StringVector*, GetObject());
}

const StringVector* PLShared_StringVector::GetConstStringVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(StringVector*, GetObject());
}

const ALString& PLShared_StringVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstStringVector()->GetAt(nIndex);
}

void PLShared_StringVector::SetAt(int nIndex, const ALString& sValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetStringVector()->SetAt(nIndex, sValue);
}

void PLShared_StringVector::Add(const ALString& nValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetStringVector()->Add(nValue);
}

int PLShared_StringVector::GetSize() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstStringVector()->GetSize();
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_IntVector

PLShared_IntVector::PLShared_IntVector() {}

PLShared_IntVector::~PLShared_IntVector() {}

void PLShared_IntVector::Test()
{
	PLShared_IntVector shared_valueIn;
	PLShared_IntVector shared_valueOut;
	PLSerializer serializer;
	IntVector ivInitialValue;
	int i;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	for (i = 0; i < 5; i++)
		ivInitialValue.Add(i + 1);
	shared_valueIn.GetIntVector()->CopyFrom(&ivInitialValue);
	for (i = 0; i < 5; i++)
		shared_valueIn.Add(101 + i);

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value:\n" << shared_valueIn << endl;
	cout << "Serialized value:\n" << shared_valueOut << endl;
	;
	cout << "Serialized form:\n" << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object:\n" << shared_valueOut << endl;
}

void PLShared_IntVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	serializer->PutIntVector(cast(IntVector*, o));
}

void PLShared_IntVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	serializer->GetIntVector(cast(IntVector*, o));
}

Object* PLShared_IntVector::Create() const
{
	return new IntVector;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_LongintVector

PLShared_LongintVector::PLShared_LongintVector() {}

PLShared_LongintVector::~PLShared_LongintVector() {}

void PLShared_LongintVector::Test()
{
	PLShared_LongintVector shared_valueIn;
	PLShared_LongintVector shared_valueOut;
	PLSerializer serializer;
	LongintVector ivInitialValue;
	int i;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	for (i = 0; i < 5; i++)
		ivInitialValue.Add((longint)i + 1);
	shared_valueIn.GetLongintVector()->CopyFrom(&ivInitialValue);
	for (i = 0; i < 5; i++)
		shared_valueIn.Add(101 + (longint)i);

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value:\n" << shared_valueIn << endl;
	cout << "Serialized value:\n" << shared_valueOut << endl;
	;
	cout << "Serialized form:\n" << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object:\n" << shared_valueOut << endl;
}

void PLShared_LongintVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	serializer->PutLongintVector(cast(LongintVector*, o));
}

void PLShared_LongintVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	serializer->GetLongintVector(cast(LongintVector*, o));
}

Object* PLShared_LongintVector::Create() const
{
	return new LongintVector;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_DoubleVector

PLShared_DoubleVector::PLShared_DoubleVector(void) {}

PLShared_DoubleVector::~PLShared_DoubleVector(void) {}

void PLShared_DoubleVector::Test()
{
	PLShared_DoubleVector shared_valueIn;
	PLShared_DoubleVector shared_valueOut;
	PLSerializer serializer;
	DoubleVector dvInitialValue;
	int i;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	for (i = 0; i < 5; i++)
		dvInitialValue.Add(i + 1.5);
	shared_valueIn.GetDoubleVector()->CopyFrom(&dvInitialValue);
	for (i = 0; i < 5; i++)
		shared_valueIn.Add(101.5 + i);

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value:\n" << shared_valueOut << endl;
	cout << "Serialized value:\n" << shared_valueOut << endl;
	;
	cout << "Serialized form:\n" << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object:\n" << shared_valueOut << endl;
}

void PLShared_DoubleVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	serializer->PutDoubleVector(cast(DoubleVector*, o));
}

void PLShared_DoubleVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	serializer->GetDoubleVector(cast(DoubleVector*, o));
}

Object* PLShared_DoubleVector::Create() const
{
	return new DoubleVector;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_CharVector

PLShared_CharVector::PLShared_CharVector(void) {}

PLShared_CharVector::~PLShared_CharVector(void) {}

void PLShared_CharVector::Test()
{
	PLShared_CharVector shared_valueIn;
	PLShared_CharVector shared_valueOut;
	PLSerializer serializer;
	CharVector cvInitialValue;
	int i;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	for (i = 0; i < 5; i++)
		cvInitialValue.Add(char('a' + i));
	shared_valueIn.GetCharVector()->CopyFrom(&cvInitialValue);
	shared_valueIn.Add('\0');
	for (i = 0; i < 5; i++)
		shared_valueIn.Add(char('A' + i));

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value:\n" << shared_valueIn << endl;
	cout << "Serialized value:\n" << shared_valueOut << endl;
	;
	cout << "Serialized form:\n" << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object:\n" << shared_valueOut << endl;
}

void PLShared_CharVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	serializer->PutCharVector(cast(CharVector*, o));
}

void PLShared_CharVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	serializer->GetCharVector(cast(CharVector*, o));
}

Object* PLShared_CharVector::Create() const
{
	return new CharVector;
}
