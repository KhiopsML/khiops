// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseMapping.h"

KWMTDatabaseMapping::KWMTDatabaseMapping()
{
	mappedDataTableDriver = NULL;
	nMappedAttributeType = KWType::Unknown;
	kwoLastReadObject = NULL;
}

KWMTDatabaseMapping::~KWMTDatabaseMapping()
{
	assert(mappedDataTableDriver == NULL);
}

void KWMTDatabaseMapping::CopyFrom(const KWMTDatabaseMapping* aSource)
{
	require(aSource != NULL);

	// Pas de recopie des attributs de gestion, qui ne peuvent etre partages entre deux mapping
	// Seules les spec sont recopiees
	sDataPathClassName = aSource->sDataPathClassName;
	sDataPathAttributeNames = aSource->sDataPathAttributeNames;
	sClassName = aSource->sClassName;
	sDataTableName = aSource->sDataTableName;
}

KWMTDatabaseMapping* KWMTDatabaseMapping::Clone() const
{
	KWMTDatabaseMapping* aClone;

	aClone = new KWMTDatabaseMapping;
	aClone->CopyFrom(this);
	return aClone;
}

int KWMTDatabaseMapping::Compare(const KWMTDatabaseMapping* aSource) const
{
	int nCompare = 0;

	// Comparaison terme a terme
	if (nCompare == 0)
		nCompare = GetDataPathClassName().Compare(aSource->GetDataPathClassName());
	if (nCompare == 0)
		nCompare = GetDataPathAttributeNames().Compare(aSource->GetDataPathAttributeNames());
	if (nCompare == 0)
		nCompare = GetClassName().Compare(aSource->GetClassName());
	if (nCompare == 0)
		nCompare = GetDataTableName().Compare(aSource->GetDataTableName());
	return nCompare;
}

void KWMTDatabaseMapping::Write(ostream& ost) const
{
	ost << "Data path\t" << GetDataPath() << "\n";
	ost << "Data root\t" << GetDataPathClassName() << "\n";
	ost << "Path\t" << GetDataPathAttributeNames() << "\n";
	ost << "Dictionary\t" << GetClassName() << "\n";
	ost << "Data table file\t" << GetDataTableName() << "\n";
}

const ALString KWMTDatabaseMapping::GetClassLabel() const
{
	return "Multi-table mapping";
}

////////////////////////////////////////////////////////////

ALString KWMTDatabaseMapping::GetDataPath() const
{
	// Le chemin de donnes se decompose en la classe et les attribut accedant aux sous-objets
	if (GetDataPathAttributeNames() == "")
		return GetDataPathClassName();
	else
		return GetDataPathClassName() + '`' + GetDataPathAttributeNames();
}

const ALString KWMTDatabaseMapping::GetObjectLabel() const
{
	if (GetDataPathAttributeNames() == "")
		return GetDataPathClassName();
	else
		return GetDataPathClassName() + " " + GetDataPathAttributeNames();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Les resultats de parsing du DataPathAttributeNames sont recalcules a chaque fois
// Cela n'est pas couteux en temps de calcul, et cela evite les problemes de synchronisation

int KWMTDatabaseMapping::GetDataPathAttributeNumber() const
{
	int nAttributeNumber;
	int n;

	require(CheckDataPath());

	// Comptage du nombre de separateurs backquote, donc du nombre d'attribut du DataPath
	nAttributeNumber = 0;
	if (sDataPathAttributeNames.GetLength() > 0)
	{
		nAttributeNumber++;
		for (n = 0; n < sDataPathAttributeNames.GetLength(); n++)
		{
			if (sDataPathAttributeNames.GetAt(n) == '`')
				nAttributeNumber++;
		}
	}

	return nAttributeNumber;
}

const ALString KWMTDatabaseMapping::GetDataPathAttributeNameAt(int nIndex) const
{
	ALString sAttributeName;
	int nLastAttributeBegin;
	int nAttributeNumber;
	int n;

	require(CheckDataPath());
	require(0 <= nIndex and nIndex < GetDataPathAttributeNumber());

	// Recherche de l'attribut a extraire
	nLastAttributeBegin = 0;
	nAttributeNumber = 0;
	for (n = 0; n < sDataPathAttributeNames.GetLength(); n++)
	{
		// Cas d'un separateur d'attribut dans le DataPath
		if (sDataPathAttributeNames.GetAt(n) == '`')
		{
			nAttributeNumber++;

			// Extraction du nom si on a trouve le bon attribut
			if (nAttributeNumber == nIndex + 1)
			{
				sAttributeName =
				    sDataPathAttributeNames.Mid(nLastAttributeBegin, n - nLastAttributeBegin);
				break;
			}

			// Memorisation du debut de l'attribut suivant
			nLastAttributeBegin = n + 1;
		}

		// Cas de la fin du DataPath
		if (n == sDataPathAttributeNames.GetLength() - 1)
		{
			nAttributeNumber++;

			// Extraction du nom si on a trouve le bon attribut
			if (nAttributeNumber == nIndex + 1)
				sAttributeName =
				    sDataPathAttributeNames.Mid(nLastAttributeBegin, n + 1 - nLastAttributeBegin);
		}
	}
	return sAttributeName;
}

boolean KWMTDatabaseMapping::IsTerminalAttributeUsed() const
{
	boolean bIsTerminalAttributeUsed = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int n;
	int nLastAttributeBegin;
	boolean bNewAttribute;
	ALString sAttributeName;
	ALString sTmp;

	require(CheckDataPath());

	// Recherche de la classe de depart
	kwcCurrentClass = KWClassDomain::GetCurrentDomain()->LookupClass(sDataPathClassName);
	check(kwcCurrentClass);

	// On verifie que chaque nom d'attribut est non vide
	nLastAttributeBegin = 0;
	bNewAttribute = false;
	for (n = 0; n < sDataPathAttributeNames.GetLength(); n++)
	{
		// Cas d'un separateur d'attribut dans le DataPath
		if (sDataPathAttributeNames.GetAt(n) == '`')
		{
			// Extraction du nom
			sAttributeName = sDataPathAttributeNames.Mid(nLastAttributeBegin, n - nLastAttributeBegin);
			bNewAttribute = true;

			// Memorisation du debut de l'attribut suivant
			nLastAttributeBegin = n + 1;
		}

		// Cas de la fin du DataPath
		if (n == sDataPathAttributeNames.GetLength() - 1)
		{
			// Extraction du nom
			sAttributeName = sDataPathAttributeNames.Mid(nLastAttributeBegin, n + 1 - nLastAttributeBegin);
			bNewAttribute = true;
		}

		// Traitement des attribut trouves dans la DataPath
		if (bNewAttribute)
		{
			bNewAttribute = false;

			// Recherche de l'attribut
			currentAttribute = kwcCurrentClass->LookupAttribute(sAttributeName);
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
	}
	return bIsTerminalAttributeUsed;
}

boolean KWMTDatabaseMapping::CheckDataPath() const
{
	boolean bOk = true;
	int n;
	int nLastAttributeBegin;
	int nAttributeNumber;
	ALString sTmp;

	// Le DataPathClassName doit etre non vide
	if (sDataPathClassName.GetLength() == 0)
	{
		AddError("Empty data root");
		bOk = false;
	}

	// On verifie que chaque nom d'attribut est non vide
	nLastAttributeBegin = 0;
	nAttributeNumber = 0;
	for (n = 0; n < sDataPathAttributeNames.GetLength(); n++)
	{
		// Cas d'un separateur d'attribut dans le DataPath
		if (sDataPathAttributeNames.GetAt(n) == '`')
		{
			nAttributeNumber++;
			if (n == nLastAttributeBegin)
			{
				AddError(sTmp + "Missing name for variable " + IntToString(nAttributeNumber) +
					 " in data path");
				bOk = false;
				break;
			}

			// Memorisation du debut de l'attribut suivant
			nLastAttributeBegin = n + 1;
		}

		// Cas de la fin du DataPath
		if (n == sDataPathAttributeNames.GetLength() - 1)
		{
			nAttributeNumber++;
			if (n <= nLastAttributeBegin)
			{
				AddError(sTmp + "Missing name for variable " + IntToString(nAttributeNumber + 1) +
					 " in data path");
				bOk = false;
			}
		}
	}
	return bOk;
}

longint KWMTDatabaseMapping::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWMTDatabaseMapping);
	lUsedMemory += sDataPathClassName.GetLength();
	lUsedMemory += sDataPathAttributeNames.GetLength();
	lUsedMemory += sClassName.GetLength();
	lUsedMemory += sDataTableName.GetLength();
	lUsedMemory += oaComponentTableMappings.GetUsedMemory();
	if (mappedDataTableDriver != NULL)
		lUsedMemory += mappedDataTableDriver->GetUsedMemory();
	return lUsedMemory;
}

void KWMTDatabaseMapping::CollectFullHierarchyComponentTableMappings(ObjectArray* oaResults)
{
	int i;
	KWMTDatabaseMapping* mapping;

	require(oaResults != NULL);

	// Ajout du mapping
	oaResults->Add(this);

	// Ajout des mapping de la composition
	for (i = 0; i < oaComponentTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaComponentTableMappings.GetAt(i));
		mapping->CollectFullHierarchyComponentTableMappings(oaResults);
	}
}
