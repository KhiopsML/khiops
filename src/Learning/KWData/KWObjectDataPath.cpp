// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWObjectDataPath.h"

// Inclusion de ce header dans le source pour eviter une dependance cyclique
#include "KWClass.h"

////////////////////////////////////////////////////////////
// Classe KWObjectDataPath

KWObjectDataPath::KWObjectDataPath()
{
	bCreatedObjects = false;
	lMainCreationIndex = 0;
	lCreationNumber = 0;
	dataPathManager = NULL;
	nCompiledRandomSeed = 0;
	nCompiledRandomLeap = 0;
	nCompileHash = 0;
}

KWObjectDataPath::~KWObjectDataPath() {}

boolean KWObjectDataPath::IsRuleCreationManaged() const
{
	return true;
}

void KWObjectDataPath::ResetCreationNumber(longint lNewMainCreationIndex) const
{
	int n;
	KWObjectDataPath* componentDataPath;

	require(IsCompiled());
	require(lNewMainCreationIndex >= 0);

	// Reinitialisation des index
	lMainCreationIndex = lNewMainCreationIndex;
	lCreationNumber = 0;

	// Propagation aux sous data paths
	for (n = 0; n < oaComponents.GetSize(); n++)
	{
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
		componentDataPath->ResetCreationNumber(lMainCreationIndex);
	}
}

void KWObjectDataPath::CopyFrom(const KWDataPath* aSource)
{
	const KWObjectDataPath* sourceObjectDataPath;

	require(aSource != NULL);

	// Methode ancetre
	KWDataPath::CopyFrom(aSource);

	// Specialisation
	sourceObjectDataPath = cast(const KWObjectDataPath*, aSource);
	bCreatedObjects = sourceObjectDataPath->bCreatedObjects;

	// Nettoyage des attributs de composition ou de gestion
	lCreationNumber = 0;
	oaComponents.SetSize(0);
	dataPathManager = NULL;
	liCompiledTerminalAttributeLoadIndex.Reset();
	nCompiledRandomSeed = 0;
	nCompiledRandomLeap = 0;
	oaCompiledComponentDataPathsByLoadIndex.SetSize(0);
	nCompileHash = 0;
}

KWDataPath* KWObjectDataPath::Create() const
{
	return new KWObjectDataPath;
}

int KWObjectDataPath::Compare(const KWDataPath* aSource) const
{
	int nCompare;

	require(aSource != NULL);

	// Methode ancetre
	nCompare = KWDataPath::Compare(aSource);

	// Specialisation
	if (nCompare == 0)
		nCompare =
		    CompareBoolean(GetCreatedObjects(), cast(const KWObjectDataPath*, aSource)->GetCreatedObjects());
	return nCompare;
}

void KWObjectDataPath::Write(ostream& ost) const
{
	KWDataPath::Write(ost);
	ost << "Created objects\t" << BooleanToString(GetCreatedObjects()) << "\n";
}

void KWObjectDataPath::WriteHeaderLineReport(ostream& ost) const
{
	KWDataPath::WriteHeaderLineReport(ost);
	ost << "\tCreated";
}

void KWObjectDataPath::WriteLineReport(ostream& ost) const
{
	KWDataPath::WriteLineReport(ost);
	ost << "\t";
	if (GetCreatedObjects())
		ost << "*";
}

longint KWObjectDataPath::GetUsedMemory() const
{
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDataPath::GetUsedMemory();
	lUsedMemory += sizeof(KWObjectDataPath) - sizeof(KWDataPath);

	// Specialisation
	lUsedMemory += oaComponents.GetUsedMemory();
	lUsedMemory += oaCompiledComponentDataPathsByLoadIndex.GetUsedMemory();
	return lUsedMemory;
}

void KWObjectDataPath::Compile(const KWClass* mainClass)
{
	KWClass* kwcOriginClass;
	KWClass* currentClass;
	KWAttribute* currentAttribute;
	int n;
	KWObjectDataPath* componentDataPath;
	ALString sSeedEncoding;
	ALString sLeapEncoding;
	ALString sTmp;

	require(mainClass != NULL);
	require(mainClass->IsCompiled());
	require(Check());

	// Arret si deja compile
	if (IsCompiled())
		return;

	// Recherche de la classe origine
	kwcOriginClass = mainClass->GetDomain()->LookupClass(sOriginClassName);
	assert(kwcOriginClass != NULL);
	assert(kwcOriginClass == mainClass or kwcOriginClass->GetName() != mainClass->GetName());

	// Recherche de l'index de chargement du dernier attribut du data path et de sa class extremite
	liCompiledTerminalAttributeLoadIndex.Reset();
	currentClass = kwcOriginClass;
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
	{
		// Recherche de l'attribut
		currentAttribute = currentClass->LookupAttribute(GetDataPathAttributeNameAt(n));
		assert(currentAttribute != NULL);
		assert(currentAttribute->GetLoaded());
		assert(currentAttribute->GetLoadIndex().IsDense());
		assert(KWType::IsRelation(currentAttribute->GetType()));
		assert(currentAttribute->GetDerivationRule() == NULL or
		       not currentAttribute->GetDerivationRule()->GetReference());

		// Memorisation de son index de chargement
		liCompiledTerminalAttributeLoadIndex.lFullIndex = currentAttribute->GetLoadIndex().lFullIndex;

		// Changement de classe courante
		currentClass = currentAttribute->GetClass();
	}

	// Compilation des sous data paths, por acceder a leurs services
	for (n = 0; n < oaComponents.GetSize(); n++)
	{
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
		componentDataPath->Compile(mainClass);
	}

	// Utilisation de la partie dense des index de chargement des attributs pour un acces direct aux sous data path
	// Cet index ne peut depasser le nombre d'attributs charges en memoire de la classe terminale du data path
	oaCompiledComponentDataPathsByLoadIndex.RemoveAll();
	if (oaComponents.GetSize() > 0)
	{
		// Acces au dernier sous data path, pour avoir la valeur max d'index
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(oaComponents.GetSize() - 1));

		// Retaillage du tableau compile
		oaCompiledComponentDataPathsByLoadIndex.SetSize(
		    componentDataPath->GetTerminalLoadIndex().GetDenseIndex() + 1);

		// Memorisation de chaque sous data path selon son index dense
		for (n = 0; n < oaComponents.GetSize(); n++)
		{
			componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
			oaCompiledComponentDataPathsByLoadIndex.SetAt(
			    componentDataPath->GetTerminalLoadIndex().GetDenseIndex(), componentDataPath);
		}
	}

	// Initialisation des parametres de generateur aleatoire par hashage de chaines de caractere
	// dependant du nom de la classe origine et du data path
	sSeedEncoding = sTmp + "DataPathSeed" + kwcOriginClass->GetName() + "///" + GetDataPath() + "DataPathSeed";
	sLeapEncoding = sTmp + "DataPathLeap" + kwcOriginClass->GetName() + "///" + GetDataPath() + "DataPathLeap";
	sLeapEncoding.MakeReverse();
	nCompiledRandomSeed = HashValue(sSeedEncoding);
	nCompiledRandomLeap = HashValue(sLeapEncoding);

	// On interdit un Leap de 0
	n = 0;
	while (nCompiledRandomLeap == 0)
	{
		sLeapEncoding += "_";
		sLeapEncoding += IntToString(n);
		nCompiledRandomLeap = HashValue(sLeapEncoding);
		n++;
	}

	// Calcul de hash de compilation
	nCompileHash = HashValue(GetDataPath());

	ensure(IsCompiled());
}

boolean KWObjectDataPath::IsCompiled() const
{
	return nCompiledRandomLeap != 0 and nCompileHash == HashValue(GetDataPath());
}

////////////////////////////////////////////////////////////
// Classe KWObjectDataPathManager

KWObjectDataPathManager::KWObjectDataPathManager()
{
	databaseMemoryGuard = NULL;

	// Specifialisation des objets data path crees par la classe
	delete dataPathCreator;
	dataPathCreator = new KWObjectDataPath;
}

KWObjectDataPathManager::~KWObjectDataPathManager() {}

void KWObjectDataPathManager::ComputeAllDataPaths(const KWClass* mainClass)
{
	int i;
	KWObjectDataPath* objectDataPath;

	// Methode ancetre
	KWDataPathManager::ComputeAllDataPaths(mainClass);

	// Specialisation par optimisation des data paths
	for (i = 0; i < oaDataPaths.GetSize(); i++)
	{
		objectDataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));

		// Compilation
		objectDataPath->Compile(mainClass);
	}
}

void KWObjectDataPathManager::CopyFrom(const KWDataPathManager* aSource)
{
	int i;
	KWObjectDataPath* objectDataPath;
	KWClass* kwcMainClass;

	// Methode ancetre
	KWDataPathManager::CopyFrom(aSource);

	// Specialisation par optimisation des data paths
	kwcMainClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetMainClassName());
	if (kwcMainClass != NULL)
	{
		for (i = 0; i < oaDataPaths.GetSize(); i++)
		{
			objectDataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));

			// Compilation
			objectDataPath->Compile(kwcMainClass);
		}
	}
	databaseMemoryGuard = cast(KWObjectDataPathManager*, aSource)->databaseMemoryGuard;
}

KWDataPathManager* KWObjectDataPathManager::Create() const
{
	return new KWObjectDataPathManager;
}
