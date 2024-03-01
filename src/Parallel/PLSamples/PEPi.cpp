// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PEPi.h"

PEPi::PEPi()
{
	nIterationNumber = 0;
	dElapsedTime = 0;

	// ## Custom constructor

	// Valeurs par defaut
	nIterationNumber = 100000000;

	// ##
}

PEPi::~PEPi()
{
	// ## Custom destructor

	// ##
}

void PEPi::CopyFrom(const PEPi* aSource)
{
	require(aSource != NULL);

	nIterationNumber = aSource->nIterationNumber;
	sPi = aSource->sPi;
	dElapsedTime = aSource->dElapsedTime;

	// ## Custom copyfrom

	// ##
}

PEPi* PEPi::Clone() const
{
	PEPi* aClone;

	aClone = new PEPi;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void PEPi::Write(ostream& ost) const
{
	ost << "Number of iterations\t" << GetIterationNumber() << "\n";
	ost << "Pi approximation\t" << GetPi() << "\n";
	ost << "Computation time\t" << GetElapsedTime() << "\n";
}

const ALString PEPi::GetClassLabel() const
{
	return "Pi parallel computation";
}

// ## Method implementation

const ALString PEPi::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

void PEPi::ComputePi()
{
	PEPiTask piTask;
	char sPiResult[30];
	ALString sTmp;

	// Parametrage et lancement de la tache de calcul de Pi
	piTask.SetTotalIteration(GetIterationNumber());
	piTask.ComputePi();

	// Collecte des resultats, avec le maximum de precision
	snprintf(sPiResult, sizeof(sPiResult), "%.15g", piTask.GetComputedPi());
	SetPi(sPiResult);
	SetElapsedTime(piTask.GetJobElapsedTime());
}

// ##
