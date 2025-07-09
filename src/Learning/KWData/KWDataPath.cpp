// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPath.h"

KWDataPath::KWDataPath()
{
	bExternalTable = false;
}

KWDataPath::~KWDataPath() {}

void KWDataPath::CopyFrom(const KWDataPath* aSource)
{
	require(aSource != NULL);

	bExternalTable = aSource->bExternalTable;
	sOriginClassName = aSource->sOriginClassName;
	svAttributeNames.CopyFrom(&aSource->svAttributeNames);
	sClassName = aSource->sClassName;
}

KWDataPath* KWDataPath::Create() const
{
	return new KWDataPath;
}

KWDataPath* KWDataPath::Clone() const
{
	KWDataPath* aClone;

	aClone = Create();
	aClone->CopyFrom(this);
	return aClone;
}

int KWDataPath::Compare(const KWDataPath* aSource) const
{
	int nCompare = 0;
	int i;

	// Comparaison terme a terme
	if (nCompare == 0)
		nCompare = GetExternalTable() == aSource->GetExternalTable();
	if (nCompare == 0)
		nCompare = GetOriginClassName().Compare(aSource->GetOriginClassName());
	if (nCompare == 0)
	{
		// Comparaison des attributs du data path
		nCompare = GetDataPathAttributeNumber() == aSource->GetDataPathAttributeNumber();
		if (nCompare == 0)
		{
			for (i = 0; i < GetDataPathAttributeNumber(); i++)
			{
				nCompare = GetDataPathAttributeNameAt(i) == aSource->GetDataPathAttributeNameAt(i);
				if (nCompare != 0)
					break;
			}
		}
	}
	if (nCompare == 0)
		nCompare = GetClassName().Compare(aSource->GetClassName());
	return nCompare;
}

////////////////////////////////////////////////////////////

ALString KWDataPath::GetDataPath() const
{
	// Cas d'un table externe
	if (GetExternalTable())
	{
		if (GetDataPathAttributeNumber() == 0)
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName());
		else
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName()) +
			       GetDataPathSeparator() + GetDataPathAttributeNames();
	}
	// Cas standard
	else
		return GetDataPathAttributeNames();
}

ALString KWDataPath::GetDataPathAttributeNames() const
{
	ALString sDataPathAttributeNames;
	int i;

	// Partie du data path lie au attributs
	for (i = 0; i < GetDataPathAttributeNumber(); i++)
	{
		if (i > 0)
			sDataPathAttributeNames += GetDataPathSeparator();
		sDataPathAttributeNames += GetFormattedName(GetDataPathAttributeNameAt(i));
	}
	return sDataPathAttributeNames;
}

boolean KWDataPath::IsTerminalAttributeUsed() const
{

	boolean bIsTerminalAttributeUsed = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int n;

	require(CheckDataPath());

	// Recherche de la classe de depart
	kwcCurrentClass = KWClassDomain::GetCurrentDomain()->LookupClass(sOriginClassName);
	check(kwcCurrentClass);

	// Parcours des attributs du data path
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
	{
		// Recherche de l'attribut
		currentAttribute = kwcCurrentClass->LookupAttribute(GetDataPathAttributeNameAt(n));
		check(currentAttribute);
		assert(KWType::IsRelation(currentAttribute->GetType()));

		// Arret si l'attribut n'est pas en Used
		if (not currentAttribute->GetUsed())
		{
			bIsTerminalAttributeUsed = false;
			break;
		}

		// Changement de classe courante
		kwcCurrentClass = currentAttribute->GetClass();
	}
	return bIsTerminalAttributeUsed;
}

boolean KWDataPath::CheckDataPath() const
{
	boolean bOk = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int n;
	ALString sTmp;

	// Le DataPathClassName doit etre non vide
	if (sOriginClassName.GetLength() == 0)
	{
		AddError("Empty origin dictionary in data path");
		bOk = false;
	}

	// Recherche de la classe de depart
	kwcCurrentClass = NULL;
	if (bOk)
	{
		kwcCurrentClass = KWClassDomain::GetCurrentDomain()->LookupClass(sOriginClassName);
		if (kwcCurrentClass == NULL)
		{
			AddError("Unknown origin dictionary " + sOriginClassName + " in data path");
			bOk = false;
		}
	}

	// Parcours des attributs du data path
	if (bOk)
	{
		for (n = 0; n < GetDataPathAttributeNumber(); n++)
		{
			// Recherche de l'attribut
			currentAttribute = kwcCurrentClass->LookupAttribute(GetDataPathAttributeNameAt(n));

			// Erreur si attribut inexistant
			if (currentAttribute == NULL)
			{
				AddError(sTmp + "Unknown variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path");
				bOk = false;
			}
			// Erreur si attribut n'est pas de type relation
			else if (not KWType::IsRelation(currentAttribute->GetType()))
			{
				AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path should of relational data type");
				bOk = false;
			}
			// Erreur si attribut calcule
			else if (currentAttribute->GetDerivationRule() != NULL)
			{
				AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path should be native");
				bOk = false;
			}

			// Arret si erreur
			if (not bOk)
				break;

			// Changement de classe courante
			kwcCurrentClass = currentAttribute->GetClass();
		}
	}
	return bOk;
}

ALString KWDataPath::GetFormattedName(const ALString& sValue)
{
	ALString sResult;
	int nLength;
	char c;
	int i;

	// Les valeurs comportant les caracteres speciaux sont entoure de back-quotes,
	// en doublant les eventuels back-quotes internes
	if (sValue.Find(GetDataPathSeparator()) != -1 or sValue.Find(GetDataPathEscapeChar()) != -1)
	{
		sResult = '`';
		nLength = sValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sValue.GetAt(i);
			if (c == '`')
				sResult += '`';
			sResult += c;
		}
		sResult += '`';
		return sResult;
	}
	// Sinon, on garde le nom tel quel
	else
		return sValue;
}

void KWDataPath::Write(ostream& ost) const
{
	int n;

	ost << "Data path\t" << GetDataPath() << "\n";
	ost << "External table\t" << BooleanToString(GetExternalTable()) << "\n";
	ost << "Data path origin dictionary\t" << GetOriginClassName() << "\n";
	ost << "Data path variables\t" << GetDataPathAttributeNumber() << "\n";
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
		ost << "\t" << GetDataPathAttributeNameAt(n) << "\n";
	ost << "Dictionary\t" << GetClassName() << "\n";
}

const ALString KWDataPath::GetClassLabel() const
{
	return "Data path";
}

const ALString KWDataPath::GetObjectLabel() const
{
	return GetDataPath();
}

longint KWDataPath::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDataPath);
	lUsedMemory += sOriginClassName.GetLength();
	lUsedMemory += svAttributeNames.GetUsedMemory();
	lUsedMemory += sClassName.GetLength();
	return lUsedMemory;
}
