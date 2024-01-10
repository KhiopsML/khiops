// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedObject.h"
#include "PLShared_SampleObject.h"

PLShared_ObjectList::PLShared_ObjectList(const PLSharedObject* object)
{
	require(object != NULL);
	elementCreator = object;
}

PLShared_ObjectList::~PLShared_ObjectList()
{
	delete elementCreator;
	Clean();
}

void PLShared_ObjectList::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const ObjectList* olToSerialize;
	Object* oToSerialize;
	POSITION position;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(elementCreator != NULL);

	// Serialisation de la taille
	olToSerialize = cast(ObjectList*, o);
	serializer->PutInt(olToSerialize->GetCount());

	// Serialisation de chaque element
	position = olToSerialize->GetHeadPosition();
	while (position != NULL)
	{
		oToSerialize = olToSerialize->GetNext(position);
		if (oToSerialize == NULL)
			serializer->PutNullToken(true);
		else
		{
			serializer->PutNullToken(false);
			elementCreator->SerializeObject(serializer, oToSerialize);
		}
	}
}

void PLShared_ObjectList::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	ObjectList* olToBuild;
	Object* oCreatedObject;
	int nbValues;
	int i;
	boolean bIsNull;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(elementCreator != NULL);
	require(o != NULL);

	// Acces a la liste contenant les objet a deserialiser
	olToBuild = cast(ObjectList*, o);
	require(olToBuild->GetCount() == 0);

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
		olToBuild->AddTail(oCreatedObject);
	}
}

void PLShared_ObjectList::Test()
{
	PLShared_ObjectList shared_olIn(new PLShared_SampleObject);
	PLShared_ObjectList shared_olOut(new PLShared_SampleObject);
	PLSerializer serializer;

	// Initialisation
	shared_olOut.bIsDeclared = true;
	shared_olOut.GetObjectList()->AddTail(new SampleObject(0, "premier objet"));
	shared_olOut.GetObjectList()->AddTail(new SampleObject(1, "deuxieme objet"));
	shared_olOut.GetObjectList()->AddTail(new SampleObject(2, "troisieme objet"));
	shared_olOut.GetObjectList()->AddTail(new SampleObject(3, "quatrieme objet"));
	shared_olOut.GetObjectList()->AddTail(new SampleObject(4, "cinquieme objet"));

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_olOut.Serialize(&serializer);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_olIn.bIsDeclared = true;
	shared_olIn.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << shared_olOut << endl;
	cout << "Serialized value " << shared_olIn << endl;
	cout << endl << "Serialized form " << serializer << endl;
}
