// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDEntity.h"

///////////////////////////////////////////////////////////////////////////////////////
// Classe KDEntity

KDEntity::KDEntity() {}

KDEntity::~KDEntity() {}

void KDEntity::SetName(const ALString sValue)
{
	require(sValue == "" or KWClass::CheckName(sValue, NULL));
	sName = sValue;
}

const ALString& KDEntity::GetName() const
{
	return sName;
}

void KDEntity::SetPartName(const ALString sValue)
{
	require(sValue == "" or KWClass::CheckName(sValue, NULL));
	sPartName = sValue;
}

const ALString& KDEntity::GetPartName() const
{
	return sPartName;
}

boolean KDEntity::Matches(const ALString& sEntityName, const ALString& sEntityPartName) const
{
	boolean bOk = true;

	require(sEntityName != "" and sEntityPartName != "");

	if (bOk and sName != "" and sName != sEntityName)
		bOk = false;
	if (bOk and sPartName != "" and sPartName != sEntityPartName)
		bOk = false;
	return bOk;
}

const ALString KDEntity::GetId() const
{
	ALString sId;

	if (sPartName == "")
		sId = "*";
	else
		sId = KWClass::GetExternalName(sName);
	sId += ".";
	if (sPartName == "")
		sId += "*";
	else
		sId += KWClass::GetExternalName(sPartName);
	return sId;
}

boolean KDEntity::Check() const
{
	boolean bOk = true;

	if (sName != "" and KWClass::CheckName(sName, this))
		bOk = false;
	if (sPartName != "" and KWClass::CheckName(sPartName, this))
		bOk = false;
	return bOk;
}

void KDEntity::Write(ostream& ost) const
{
	ost << GetId();
}

const ALString KDEntity::GetClassLabel() const
{
	return "Entity";
}

///////////////////////////////////////////////////////////////////////////////////////
// Classe KDEntitySet

KDEntitySet::KDEntitySet()
{
	bExcludeMode = false;
}

KDEntitySet::~KDEntitySet()
{
	oaEntities.DeleteAll();
}

void KDEntitySet::SetExcludeMode(boolean bValue)
{
	bExcludeMode = bValue;
}

boolean KDEntitySet::GetExcludeMode() const
{
	return bExcludeMode;
}

ObjectArray* KDEntitySet::GetEntities()
{
	return &oaEntities;
}

boolean KDEntitySet::Matches(const ALString& sEntityName, const ALString& sEntityPartName) const
{
	boolean bOk = true;
	int nMatchNumber;
	int i;
	KDEntity* entity;

	require(sEntityName != "" and sEntityPartName != "");

	// Calcul du nombre de correspondances
	nMatchNumber = 0;
	for (i = 0; i < oaEntities.GetSize(); i++)
	{
		entity = cast(KDEntity*, oaEntities.GetAt(i));

		// Test si correspondance
		if (entity->Matches(sEntityName, sEntityPartName))
			nMatchNumber++;
	}

	// On teste s'il y a au moins une correspondance en mode normal, s'il y en aucune en mode exclusion
	if (bExcludeMode)
		bOk = (nMatchNumber == 0);
	else
		bOk = (nMatchNumber > 0);
	return bOk;
}

boolean KDEntitySet::Check() const
{
	boolean bOk = true;
	int i;
	KDEntity* entity;
	ALString sEntityId;
	ObjectDictionary odEntities;

	// On met toutes les entite dans un dictionnaire pour tester leur unicite
	for (i = 0; i < oaEntities.GetSize(); i++)
	{
		entity = cast(KDEntity*, oaEntities.GetAt(i));
		sEntityId = entity->GetId();

		// Test unitaire
		if (not entity->Check())
		{
			AddError("Entity " + sEntityId + " not valid");
			bOk = false;
			break;
		}

		// Test si doublon
		if (odEntities.Lookup(sEntityId) != NULL)
		{
			AddError("Entity " + sEntityId + " is used twice");
			bOk = false;
			break;
		}
		else
			odEntities.SetAt(sEntityId, entity);
	}

	// Il doit y avoir au moins une entite
	if (bOk and oaEntities.GetSize() == 0)
	{
		AddError("Empty entity set");
		bOk = false;
	}
	return bOk;
}

void KDEntitySet::Write(ostream& ost) const
{
	int i;
	KDEntity* entity;

	ost << "{";
	if (GetExcludeMode())
		ost << "^ ";
	for (i = 0; i < oaEntities.GetSize(); i++)
	{
		entity = cast(KDEntity*, oaEntities.GetAt(i));

		if (i > 0)
			ost << ", ";
		ost << entity->GetId();
	}
	ost << "}";
}

const ALString KDEntitySet::GetClassLabel() const
{
	return "Entity set";
}
