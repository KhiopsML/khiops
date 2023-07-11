// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "PEPiTask.h"

// ##

////////////////////////////////////////////////////////////
// Classe PEPi
//    Pi parallel computation
class PEPi : public Object
{
public:
	// Constructeur
	PEPi();
	~PEPi();

	// Copie et duplication
	void CopyFrom(const PEPi* aSource);
	PEPi* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Number of iterations
	int GetIterationNumber() const;
	void SetIterationNumber(int nValue);

	// Pi approximation
	const ALString& GetPi() const;
	void SetPi(const ALString& sValue);

	// Computation time
	double GetElapsedTime() const;
	void SetElapsedTime(double dValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations
	//  Action de calcul

	void ComputePi();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	int nIterationNumber;
	ALString sPi;
	double dElapsedTime;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline int PEPi::GetIterationNumber() const
{
	return nIterationNumber;
}

inline void PEPi::SetIterationNumber(int nValue)
{
	nIterationNumber = nValue;
}

inline const ALString& PEPi::GetPi() const
{
	return sPi;
}

inline void PEPi::SetPi(const ALString& sValue)
{
	sPi = sValue;
}

inline double PEPi::GetElapsedTime() const
{
	return dElapsedTime;
}

inline void PEPi::SetElapsedTime(double dValue)
{
	dElapsedTime = dValue;
}

// ## Custom inlines

// ##
