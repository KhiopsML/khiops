// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedObject.h"
#include "PLShared_SampleObject.h"

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_ObjectArray

PLShared_ObjectArray::PLShared_ObjectArray(const PLSharedObject* object)
{
	require(object != NULL);
	elementCreator = object;
	compareFunction = NULL;
}

PLShared_ObjectArray::~PLShared_ObjectArray()
{
	delete elementCreator;
	compareFunction = NULL;
	Clean();
}

void PLShared_ObjectArray::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	int i;
	ObjectArray* oaToSerialize;
	Object* oToSerialize;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(elementCreator != NULL);

	// Serialisation de la taille
	oaToSerialize = cast(ObjectArray*, o);

	// Serialization de la presence d'une fonction de comparaison
	serializer->PutInt(oaToSerialize->GetSize());

	// Serialisation de chaque element
	for (i = 0; i < oaToSerialize->GetSize(); i++)
	{
		oToSerialize = oaToSerialize->GetAt(i);
		if (oToSerialize == NULL)
			serializer->PutNullToken(true);
		else
		{
			serializer->PutNullToken(false);
			elementCreator->SerializeObject(serializer, oToSerialize);
		}
	}
}

void PLShared_ObjectArray::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	ObjectArray* oaToBuild;
	Object* oCreatedObject;
	int nbValues;
	int i;
	boolean bIsNull;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(elementCreator != NULL);
	require(o != NULL);

	// Acces au tableau contenant les objet a deserialiser
	oaToBuild = cast(ObjectArray*, o);
	assert(oaToBuild->GetSize() == 0);

	// Deserialisation du nombre de valeurs
	nbValues = serializer->GetInt();

	// Deserialisation de chaque valeur
	for (i = 0; i < nbValues; i++)
	{
		bIsNull = serializer->GetNullToken();
		if (bIsNull)
			oCreatedObject = NULL;
		else
		{
			oCreatedObject = elementCreator->Create();
			elementCreator->DeserializeObject(serializer, oCreatedObject);
		}
		oaToBuild->Add(oCreatedObject);
	}

	// Ajout de la fonction de comparaison
	assert(not oaToBuild->IsSortable());
	if (compareFunction != NULL)
	{
		oaToBuild->SetCompareFunction(compareFunction);
		ensure(oaToBuild->IsSortable());
	}
}

void PLShared_ObjectArray::Test()
{
	PLShared_ObjectArray shared_oaIn(new PLShared_SampleObject);
	PLShared_ObjectArray shared_oaOut(new PLShared_SampleObject);
	PLShared_ObjectArray shared_oa(new PLShared_SampleObject);
	PLSerializer serializer;
	ObjectArray oaToSerialize;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_oaIn.bIsDeclared = true;
	shared_oaOut.bIsDeclared = true;

	// Initialisation
	shared_oaOut.GetObjectArray()->Add(new SampleObject(0, "premier objet"));
	shared_oaOut.GetObjectArray()->Add(new SampleObject(1, "deuxieme objet"));
	shared_oaOut.GetObjectArray()->Add(new SampleObject(2, "troisieme objet"));
	shared_oaOut.GetObjectArray()->Add(new SampleObject(3, "quatrieme objet"));
	shared_oaOut.GetObjectArray()->Add(NULL);
	shared_oaOut.GetObjectArray()->Add(new SampleObject(5, "sixieme objet"));

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_oaOut.Serialize(&serializer);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_oaIn.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << shared_oaOut << endl;
	cout << "Serialized value " << shared_oaIn << endl;
	cout << endl << "Serialized form " << serializer << endl;

	/////////////////////////////////////////////////
	// Utilisation des methodes AddToSerializer et GetFromSerializer

	// Initialisation
	oaToSerialize.Add(new SampleObject(0, "premier objet"));
	oaToSerialize.Add(new SampleObject(1, "deuxieme objet"));
	oaToSerialize.Add(new SampleObject(2, "troisieme objet"));
	oaToSerialize.Add(new SampleObject(3, "quatrieme objet"));
	oaToSerialize.Add(NULL);
	oaToSerialize.Add(new SampleObject(5, "sixieme objet"));

	cout << endl;
	cout << "Initial value " << oaToSerialize << endl;

	// Serialisation
	serializer.Initialize();
	serializer.OpenForWrite(NULL);
	shared_oa.SerializeObject(&serializer, &oaToSerialize);
	serializer.Close();
	oaToSerialize.DeleteAll();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_oa.DeserializeObject(&serializer, &oaToSerialize);
	serializer.Close();

	cout << "Deserialized value " << oaToSerialize << endl;
	oaToSerialize.DeleteAll();
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_StringObject

PLShared_StringObject::PLShared_StringObject() {}

PLShared_StringObject::~PLShared_StringObject() {}

void PLShared_StringObject::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const StringObject* so;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	so = cast(StringObject*, o);
	serializer->PutString(so->GetString());
}

void PLShared_StringObject::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	StringObject* so;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	so = cast(StringObject*, o);
	so->SetString(serializer->GetString());
}
///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_DoubleObject

PLShared_DoubleObject::PLShared_DoubleObject() {}

PLShared_DoubleObject::~PLShared_DoubleObject() {}

void PLShared_DoubleObject::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const DoubleObject* hr;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	hr = cast(DoubleObject*, o);
	serializer->PutDouble(hr->GetDouble());
}

void PLShared_DoubleObject::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	DoubleObject* hr;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	hr = cast(DoubleObject*, o);
	hr->SetDouble(serializer->GetDouble());
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_IntObject

PLShared_IntObject::PLShared_IntObject() {}

PLShared_IntObject::~PLShared_IntObject() {}

void PLShared_IntObject::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const IntObject* io;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	io = cast(IntObject*, o);
	serializer->PutInt(io->GetInt());
}

void PLShared_IntObject::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	IntObject* io;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	io = cast(IntObject*, o);
	io->SetInt(serializer->GetInt());
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_LongintObject

PLShared_LongintObject::PLShared_LongintObject() {}

PLShared_LongintObject::~PLShared_LongintObject() {}

void PLShared_LongintObject::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const LongintObject* lo;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	lo = cast(LongintObject*, o);
	serializer->PutLongint(lo->GetLongint());
}

void PLShared_LongintObject::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	LongintObject* lo;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	lo = cast(LongintObject*, o);
	lo->SetLongint(serializer->GetLongint());
}