// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_SampleObject.h"

PLShared_SampleObject::PLShared_SampleObject() {}

PLShared_SampleObject::~PLShared_SampleObject() {}

void PLShared_SampleObject::Test()
{
	SampleObject* soIn;
	SampleObject* soOut;
	PLShared_SampleObject* shared_soIn;
	PLShared_SampleObject* shared_soOut;
	PLSerializer serializer;
	PLShared_SampleObject shared_so;

	soIn = new SampleObject;
	soIn->SetInt(10);
	soIn->SetString("Sample Object Test");
	cout << "Serialization of : " << endl;
	cout << *soIn << endl;

	// Serialization
	shared_soIn = new PLShared_SampleObject;
	shared_soIn->SetSampleObject(soIn);
	serializer.OpenForWrite(NULL);
	shared_soIn->Serialize(&serializer);
	delete shared_soIn;
	serializer.Close();
	cout << "serializer : " << serializer << endl;

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_soOut = new PLShared_SampleObject;
	shared_soOut->Deserialize(&serializer);
	serializer.Close();
	soOut = shared_soOut->GetSampleObject();

	cout << "Get after deserialization : " << endl;
	cout << *soOut << endl;
	delete shared_soOut;

	soIn = new SampleObject;
	soIn->SetInt(10);
	soIn->SetString("Sample Object Test");
	cout << "Serialization of : " << endl;
	cout << *soIn << endl;

	serializer.OpenForWrite(NULL);
	shared_so.SerializeObject(&serializer, soIn);
	serializer.Close();
	delete soIn;
	cout << serializer << endl;
	serializer.OpenForRead(NULL);
	soOut = new SampleObject;
	shared_so.DeserializeObject(&serializer, soOut);
	serializer.Close();

	cout << "Get after deserialization : " << endl;
	cout << *soOut << endl;
	delete soOut;
}

void PLShared_SampleObject::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const SampleObject* so = cast(SampleObject*, o);

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Serialisation de la chaine de caracteres
	serializer->PutString(so->GetString());

	// Serialisation de l'entier
	serializer->PutInt(so->GetInt());
}

void PLShared_SampleObject::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	SampleObject* so = cast(SampleObject*, o);

	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Deserialisation de la chaine de caracteres
	so->SetString(serializer->GetString());

	// Deserialisation de l'entier
	so->SetInt(serializer->GetInt());
}
