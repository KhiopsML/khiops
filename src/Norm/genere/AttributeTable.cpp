// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "AttributeTable.h"

AttributeTable::AttributeTable() : ManagedObjectTable(Attribute::GetManagedObjectClass())
{
	nRankServicesFreshness = 0;
	Attribute::SetFieldSeparator(';');
}

AttributeTable::~AttributeTable() {}

QueryServices* AttributeTable::RankServices()
{
	if (not qsRankServices.IsInit())
		qsRankServices.Init(AttributeCompareRank, AttributeGetRank, "Attribute", "Rank");
	if (nRankServicesFreshness < nUpdateNumber)
	{
		qsRankServices.LoadObjectDictionary(&dicKeyIndex);
		nRankServicesFreshness = nUpdateNumber;
	}
	return &qsRankServices;
}

boolean AttributeTable::Check() const
{
	boolean bOk = true;
	Attribute* att;

	// Controles de tous les attributs de la tables
	for (int i = 0; i < cast(AttributeTable*, this)->GetSize(); i++)
	{
		att = cast(Attribute*, cast(AttributeTable*, this)->GetAt(i));
		bOk = att->Check() and bOk;
	}

	return bOk;
}

boolean AttributeTable::NoStats()
{
	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetStats() == true)
			return false;
	}
	return true;
}

boolean AttributeTable::NoKeyFields()
{
	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetKeyField() == true)
			return false;
	}
	return true;
}

int AttributeTable::GetKeyFieldsNumber()
{
	int nResult = 0;

	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetKeyField() == true)
			nResult++;
	}
	return nResult;
}

boolean AttributeTable::IsKeyBuilt()
{
	Attribute* att;
	int nResult = 0;
	boolean bBuildNeed = false;

	for (int i = 0; i < GetSize(); i++)
	{
		att = cast(Attribute*, GetAt(i));
		if (att->IsField() and att->GetKeyField() == true)
		{
			nResult++;
			if (att->GetType() != "ALString")
				bBuildNeed = true;
		}
	}
	return nResult > 1 or bBuildNeed == true;
}

int AttributeTable::GetPermanentFieldsNumber()
{
	int nResult = 0;

	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetPermanent() == true)
			nResult++;
	}
	return nResult;
}

int AttributeTable::GetVisibleFieldsNumber()
{
	int nResult = 0;

	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetVisible() == true)
			nResult++;
	}
	return nResult;
}

int AttributeTable::GetStyleFieldsNumber()
{
	int nResult = 0;

	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetStyle() != "" and
		    cast(Attribute*, GetAt(i))->GetVisible() == true)
			nResult++;
	}
	return nResult;
}