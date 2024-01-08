// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedObject.h"
#include "PLShared_SampleObject.h"

PLShared_ObjectDictionary::PLShared_ObjectDictionary(const PLSharedObject* object)
{
	require(object != NULL);
	elementCreator = object;
}

PLShared_ObjectDictionary::~PLShared_ObjectDictionary()
{
	delete elementCreator;
	Clean();
}

void PLShared_ObjectDictionary::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const ObjectDictionary* odToSerialize;
	Object* oToSerialize;
	POSITION position;
	ALString sKeyToSerialize;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(elementCreator != NULL);

	// Serialisation de la taille
	odToSerialize = cast(ObjectDictionary*, o);
	serializer->PutInt(odToSerialize->GetCount());

	// Serialisation de chaque element
	position = odToSerialize->GetStartPosition();
	while (position != NULL)
	{
		odToSerialize->GetNextAssoc(position, sKeyToSerialize, oToSerialize);

		// Serialisation de la clef
		serializer->PutString(sKeyToSerialize);

		// Serialisation de la valeur
		if (oToSerialize == NULL)
			serializer->PutNullToken(true);
		else
		{
			serializer->PutNullToken(false);
			elementCreator->SerializeObject(serializer, oToSerialize);
		}
	}
}

void PLShared_ObjectDictionary::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	ObjectDictionary* odToBuild;
	Object* oCreatedObject;
	int nNbValues;
	int i;
	boolean bIsNull;
	ALString sKey;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(elementCreator != NULL);
	require(o != NULL);

	// Acces au dictionnaire contenant les objets a deserialiser
	odToBuild = cast(ObjectDictionary*, o);
	assert(odToBuild->GetCount() == 0);

	// Deserialisation du nombre de valeurs
	nNbValues = serializer->GetInt();

	// Deserialisation de chaque valeur
	for (i = 0; i < nNbValues; i++)
	{
		// Deserialisation de la clef
		sKey = serializer->GetString();

		// Deserialisation de la valeur
		bIsNull = serializer->GetNullToken();
		if (bIsNull)
			oCreatedObject = NULL;
		else
		{
			oCreatedObject = elementCreator->Create();
			elementCreator->DeserializeObject(serializer, oCreatedObject);
		}
		odToBuild->SetAt(sKey, oCreatedObject);
	}
}

void PLShared_ObjectDictionary::Test()
{
	PLShared_ObjectDictionary shared_odIn(new PLShared_SampleObject);
	PLShared_ObjectDictionary shared_odOut(new PLShared_SampleObject);
	PLSerializer serializer;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_odOut.bIsDeclared = true;
	shared_odIn.bIsDeclared = true;

	// Initialisation
	shared_odOut.GetObjectDictionary()->SetAt("first key", new SampleObject(0, "premier objet"));
	shared_odOut.GetObjectDictionary()->SetAt("second key", new SampleObject(1, "deuxieme objet"));
	shared_odOut.GetObjectDictionary()->SetAt("third key", new SampleObject(2, "troisieme objet"));
	shared_odOut.GetObjectDictionary()->SetAt("fourth key", new SampleObject(3, "quatrieme objet"));
	shared_odOut.GetObjectDictionary()->SetAt("fifth key", NULL);
	shared_odOut.GetObjectDictionary()->SetAt("sixth key", new SampleObject(5, "sixieme objet"));

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_odOut.Serialize(&serializer);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_odIn.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << shared_odOut << endl;
	cout << "Serialized value " << shared_odIn << endl;
	cout << endl << "Serialized form " << serializer << endl;
}
