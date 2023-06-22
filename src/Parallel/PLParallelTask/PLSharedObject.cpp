// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedObject.h"

/////////////////////////////////////////////
// Implementation de la classe PLSharedObject

PLSharedObject::PLSharedObject()
{
	oValue = NULL;
	bIsInit = false;
}

PLSharedObject::~PLSharedObject()
{
	Clean();
}

const ALString PLSharedObject::GetClassLabel() const
{
	if (oValue == NULL)
		return "";
	else
		return oValue->GetClassLabel();
}

const ALString PLSharedObject::GetObjectLabel() const
{
	if (oValue == NULL)
		return "";
	else
		return oValue->GetObjectLabel();
}

void PLSharedObject::RemoveObject()
{
	oValue = NULL;
}

void PLSharedObject::AddNull(PLSerializer* serializer, const Object* o) const
{
	serializer->PutNullToken(o == NULL);
}

boolean PLSharedObject::GetNull(PLSerializer* serializer) const
{
	return serializer->GetNullToken();
}

void PLSharedObject::SerializeValue(PLSerializer* serializer) const
{
	// Creation potentielle de l'objet si celui-ci n'a pas ete initialise
	if (not bIsInit)
		GetObject();

	AddNull(serializer, oValue);

	// Serialisation
	if (oValue != NULL)
		SerializeObject(serializer, GetObject());
}

void PLSharedObject::DeserializeValue(PLSerializer* serializer)
{
	if (GetNull(serializer))
	{
		oValue = NULL;
		bIsInit = true;
	}
	else
	{
		SetObject(Create());
		DeserializeObject(serializer, oValue);
	}
}