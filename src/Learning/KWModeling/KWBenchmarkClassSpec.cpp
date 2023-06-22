// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWBenchmarkClassSpec.h"

KWBenchmarkClassSpec::KWBenchmarkClassSpec()
{
	currentClassDomain = NULL;
	temporaryClassDomain = NULL;
}

KWBenchmarkClassSpec::~KWBenchmarkClassSpec() {}

void KWBenchmarkClassSpec::Write(ostream& ost) const
{
	ost << "Dictionary file\t" << GetClassFileName() << "\n";
	ost << "Dictionary\t" << GetClassName() << "\n";
	ost << "Target variable\t" << GetTargetAttributeName() << "\n";
}

const ALString KWBenchmarkClassSpec::GetClassLabel() const
{
	return "Benchmark dictionary";
}

const ALString KWBenchmarkClassSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

void KWBenchmarkClassSpec::ReadClasses()
{
	boolean bTemporaryDomainCreation;

	////////////////////////////////////////////////////////////////
	// Tentative de lecture du dictionnaire associe au benchmark
	// On a besoin qu'il soit en memoire pour les parametrages de la base

	// Mise en place d'un domaine de classe temporaire
	bTemporaryDomainCreation = (temporaryClassDomain == NULL);
	if (bTemporaryDomainCreation)
	{
		currentClassDomain = KWClassDomain::GetCurrentDomain();
		temporaryClassDomain = new KWClassDomain;
		temporaryClassDomain->SetName("Benchmark");
		KWClassDomain::SetCurrentDomain(temporaryClassDomain);
	}

	// Lecture des classes
	Global::SetSilentMode(true);
	if (bTemporaryDomainCreation)
	{
		if (GetClassFileName() != "")
			temporaryClassDomain->ReadFile(GetClassFileName());
	}
	else
	{
		if (GetClassFileName() == "")
			temporaryClassDomain->DeleteAllClasses();
		else if (GetClassFileName() != sLastReadClassFileName)
		{
			temporaryClassDomain->DeleteAllClasses();
			temporaryClassDomain->ReadFile(GetClassFileName());
		}
	}
	Global::SetSilentMode(false);
	sLastReadClassFileName = GetClassFileName();
}

void KWBenchmarkClassSpec::DropClasses()
{
	// Nettoyage des dictionnaires charges en memoire
	// et restitution du domaine courant
	if (temporaryClassDomain != NULL)
	{
		assert(currentClassDomain != NULL);
		assert(KWClassDomain::GetCurrentDomain() == temporaryClassDomain);
		KWClassDomain::SetCurrentDomain(currentClassDomain);
		currentClassDomain = NULL;
		delete temporaryClassDomain;
		temporaryClassDomain = NULL;
	}
	sLastReadClassFileName = "";
}