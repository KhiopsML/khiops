// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:56
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWClassManagement.h"

KWClassManagement::KWClassManagement()
{
	// ## Custom constructor

	// ##
}

KWClassManagement::~KWClassManagement()
{
	// ## Custom destructor

	// Destruction des specs de classes
	oaClassSpecs.DeleteAll();

	// ##
}

void KWClassManagement::CopyFrom(const KWClassManagement* aSource)
{
	require(aSource != NULL);

	sClassName = aSource->sClassName;
	sClassFileName = aSource->sClassFileName;

	// ## Custom copyfrom

	// ##
}

KWClassManagement* KWClassManagement::Clone() const
{
	KWClassManagement* aClone;

	aClone = new KWClassManagement;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWClassManagement::Write(ostream& ost) const
{
	ost << "Analysis dictionary\t" << GetClassName() << "\n";
	ost << "Dictionary file\t" << GetClassFileName() << "\n";
}

const ALString KWClassManagement::GetClassLabel() const
{
	return "Dictionary management";
}

// ## Method implementation

const ALString KWClassManagement::GetObjectLabel() const
{
	return GetClassFileName();
}

ObjectArray* KWClassManagement::GetClassSpecs()
{
	return &oaClassSpecs;
}

boolean KWClassManagement::ReadClasses()
{
	KWClassDomain* readDomain;
	boolean bOk = true;

	// Creation d'un domain temporaire pour la lecture
	readDomain = new KWClassDomain;
	readDomain->SetName(KWClassDomain::GetCurrentDomain()->GetName());

	// Lecture des classes
	bOk = readDomain->ReadFile(GetClassFileName());

	// Si probleme: annulation de la lecture
	if (bOk and not readDomain->Check())
	{
		AddError("Read cancelled because of errors");
		bOk = false;
	}
	// Sinon: compilation des classes
	else
	{
		readDomain->Compile();
	}

	// Remplacement du domaine courant si pas d'erreur
	if (bOk)
	{
		// Nettoyage
		DropClasses();
		KWClassDomain::DeleteDomain(KWClassDomain::GetCurrentDomain()->GetName());

		// Insertion du nouveau domaine, memorise pour pouvoir etre detruit
		KWClassDomain::InsertDomain(readDomain);
		KWClassDomain::SetCurrentDomain(readDomain);

		// On initialise le choix du dictionnaire si necessaire
		if (GetClassName() == "" or KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()) == NULL)
			SetClassName(SearchDefaultClassName());
	}
	// Sinon, destruction du domaine de lecture
	else
		delete readDomain;
	return bOk;
}

boolean KWClassManagement::WriteClasses()
{
	boolean bOk = true;
	ALString sOutputPathName;

	// Creation si necessaire du repertoire cible
	sOutputPathName = FileService::GetPathName(GetClassFileName());
	if (sOutputPathName != "" and not PLRemoteFileService::Exist(sOutputPathName))
	{
		bOk = PLRemoteFileService::MakeDirectories(sOutputPathName);
		if (not bOk)
			AddError("Unable to create directory (" + sOutputPathName + ") for dictionary");
	}

	// Ecriture si OK
	if (bOk)
		bOk = KWClassDomain::GetCurrentDomain()->WriteFile(GetClassFileName());
	return bOk;
}

boolean KWClassManagement::ExportJSONClasses(const ALString& sJSONFileName)
{
	boolean bOk = true;
	ALString sOutputPathName;

	// Creation si necessaire du repertoire cible
	sOutputPathName = FileService::GetPathName(sJSONFileName);
	if (sOutputPathName != "" and not PLRemoteFileService::Exist(sOutputPathName))
	{
		bOk = FileService::MakeDirectories(sOutputPathName);
		if (not bOk)
			AddError("Unable to create directory (" + sOutputPathName + ") for dictionary JSON file");
	}

	// Ecriture si OK
	if (bOk)
		bOk = KWClassDomain::GetCurrentDomain()->WriteJSONFile(sJSONFileName);
	return bOk;
}

void KWClassManagement::DropClasses()
{
	KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
	oaClassSpecs.DeleteAll();
}

void KWClassManagement::RefreshClassSpecs()
{
	int nClass;
	KWClass* kwcClass;
	KWClassSpec* classSpec;
	int nDerivedAttributeNumber;
	int i;
	KWAttribute* attribute;
	ALString sKey;

	// Synchronisation de la taille du tableau de KWClassSpec avec le nombre de classes du domaines
	// On tente de preserver au maximum les meme objects KWClassSpec pour minimiser les
	// problemes de synchronisation avec Java
	if (oaClassSpecs.GetSize() > KWClassDomain::GetCurrentDomain()->GetClassNumber())
	{
		for (nClass = KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass < oaClassSpecs.GetSize();
		     nClass++)
			delete oaClassSpecs.GetAt(nClass);
		oaClassSpecs.SetSize(KWClassDomain::GetCurrentDomain()->GetClassNumber());
	}
	else if (oaClassSpecs.GetSize() < KWClassDomain::GetCurrentDomain()->GetClassNumber())
	{
		while (oaClassSpecs.GetSize() < KWClassDomain::GetCurrentDomain()->GetClassNumber())
			oaClassSpecs.Add(new KWClassSpec);
	}
	assert(oaClassSpecs.GetSize() == KWClassDomain::GetCurrentDomain()->GetClassNumber());

	// Reactualisation des specs de classes
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);
		assert(kwcClass->GetUsedAttributeNumber() == kwcClass->GetLoadedAttributeNumber());

		// Comptage des attributs derives
		nDerivedAttributeNumber = 0;
		for (i = 0; i < kwcClass->GetUsedAttributeNumber(); i++)
		{
			attribute = kwcClass->GetUsedAttributeAt(i);
			if (attribute->GetAnyDerivationRule() != NULL)
				nDerivedAttributeNumber++;
		}

		// Initialisation et ajout d'une spec de classe
		classSpec = cast(KWClassSpec*, oaClassSpecs.GetAt(nClass));
		classSpec->SetClassName(kwcClass->GetName());
		classSpec->SetRoot(kwcClass->GetRoot());
		classSpec->SetAttributeNumber(kwcClass->GetUsedAttributeNumber());
		classSpec->SetSymbolAttributeNumber(kwcClass->GetUsedAttributeNumberForType(KWType::Symbol));
		classSpec->SetContinuousAttributeNumber(kwcClass->GetUsedAttributeNumberForType(KWType::Continuous));
		classSpec->SetDerivedAttributeNumber(nDerivedAttributeNumber);

		// Parametres de la cle
		sKey = "";
		for (i = 0; i < kwcClass->GetKeyAttributeNumber(); i++)
		{
			if (i > 0)
				sKey += ", ";
			sKey += kwcClass->GetKeyAttributeNameAt(i);
		}
		classSpec->SetKey(sKey);
	}
}

const ALString KWClassManagement::SearchDefaultClassName() const
{
	ALString sDefaultClassName;
	int nClass;
	KWClass* kwcClass;

	// Recherche du premier dictionnaire racine s'il y en a, du premier dictionnaire sinon
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);

		// Memorisation du premier dictionnaire rencontre
		if (nClass == 0)
			sDefaultClassName = kwcClass->GetName();

		// On arrete si on a trouve un dictionnaire racine
		if (kwcClass->GetRoot() == true)
		{
			sDefaultClassName = kwcClass->GetName();
			break;
		}
	}
	return sDefaultClassName;
}

// ##
