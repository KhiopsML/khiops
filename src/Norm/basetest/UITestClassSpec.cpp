// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Thu Jul 15 10:55:39 2004
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestClassSpec.h"

UITestClassSpec::UITestClassSpec()
{
	nAttributeNumber = 0;
	nSymbolAttributeNumber = 0;
	nContinuousAttributeNumber = 0;
	nDerivedAttributeNumber = 0;

	// ## Custom constructor

	// Parametrage du constructeur, pour tester la prise en compte des valeurs
	// par defaut lors de la creation de fiches
	sClassName = "Sample";
	nAttributeNumber = 2;
	nSymbolAttributeNumber = 1;
	nContinuousAttributeNumber = 1;

	// ##
}

UITestClassSpec::~UITestClassSpec()
{
	// ## Custom destructor

	// ##
}

UITestClassSpec* UITestClassSpec::Clone() const
{
	UITestClassSpec* aClone;

	aClone = new UITestClassSpec;

	aClone->sClassName = sClassName;
	aClone->nAttributeNumber = nAttributeNumber;
	aClone->nSymbolAttributeNumber = nSymbolAttributeNumber;
	aClone->nContinuousAttributeNumber = nContinuousAttributeNumber;
	aClone->nDerivedAttributeNumber = nDerivedAttributeNumber;

	// ## Custom clone

	// ##
	return aClone;
}

void UITestClassSpec::Write(ostream& ost) const
{
	ost << "Nom\t" << GetClassName() << "\n";
	ost << "Attributs\t" << GetAttributeNumber() << "\n";
	ost << "Modaux\t" << GetSymbolAttributeNumber() << "\n";
	ost << "Continus\t" << GetContinuousAttributeNumber() << "\n";
	ost << "Calcules\t" << GetDerivedAttributeNumber() << "\n";
}

const ALString UITestClassSpec::GetClassLabel() const
{
	return "Test UI (class spec)";
}

// ## Method implementation

const ALString UITestClassSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
