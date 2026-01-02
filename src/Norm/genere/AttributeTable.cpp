// Copyright (c) 2023-2026 Orange. All rights reserved.
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

int AttributeTable::GetVisibleFieldsNumber()
{
	int nResult = 0;

	for (int i = 0; i < GetSize(); i++)
	{
		if (cast(Attribute*, GetAt(i))->IsField() and cast(Attribute*, GetAt(i))->GetVisible())
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
		    cast(Attribute*, GetAt(i))->GetVisible())
			nResult++;
	}
	return nResult;
}
