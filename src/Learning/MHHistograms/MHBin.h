// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWContinuous.h"

///////////////////////////////////////////////////////////////////////////////
// Classe MHBin
// Bin elementaire ([lower value, upper value], frequency)
// Classe technique, utile essentiellement pour le tri d'un ensemble de bins
// Sinon, la gestion des ensemble de bins est gerer par des vecteurs valeur
// inerfieur et superieur et d'effectif, pour des raison d'efficacite memoire
class MHBin : public Object
{
public:
	// Constructeur
	MHBin();
	~MHBin();

	// Initialisation
	void Initialize(Continuous cBinLowerValue, Continuous cBinUpperValue, int nBinFrequency);

	// Valeur inf
	void SetLowerValue(Continuous cValue);
	Continuous GetLowerValue() const;

	// Valeur sup
	void SetUpperValue(Continuous cValue);
	Continuous GetUpperValue() const;

	// Indique si le bin est singulier, avec une seule valeur
	boolean IsSingular() const;

	// Effectif
	void SetFrequency(int nValue);
	int GetFrequency() const;

	////////////////////////////////////////////////////////////////////////////////
	// Divers

	// Test d'integrite
	boolean Check() const override;

	// Copie et duplication
	void CopyFrom(const MHBin* aSource);
	MHBin* Clone() const;

	// Ecriture des bins
	void WriteHeaderLine(ostream& ost) const;
	void Write(ostream& ost) const override;

	// Tri d'un tableau de bin par valeurs croissantes
	static void SortBinArray(ObjectArray* oaBins);

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	Continuous cLowerValue;
	Continuous cUpperValue;
	int nFrequency;
};

// Comparaison de deux bins
int MHBinCompare(const void* elem1, const void* elem2);

// Methode en inline

inline MHBin::MHBin()
{
	cLowerValue = 0;
	cUpperValue = 0;
	nFrequency = 0;
}

inline MHBin::~MHBin() {}

inline void MHBin::SetLowerValue(Continuous cValue)
{
	cLowerValue = cValue;
}

inline void MHBin::Initialize(Continuous cBinLowerValue, Continuous cBinUpperValue, int nBinFrequency)
{
	require(cBinLowerValue <= cBinUpperValue);
	require(nBinFrequency >= 0);

	cLowerValue = cBinLowerValue;
	cUpperValue = cBinUpperValue;
	nFrequency = nBinFrequency;
}

inline Continuous MHBin::GetLowerValue() const
{
	return cLowerValue;
}

inline void MHBin::SetUpperValue(Continuous cValue)
{
	cUpperValue = cValue;
}

inline Continuous MHBin::GetUpperValue() const
{
	return cUpperValue;
}

inline boolean MHBin::IsSingular() const
{
	return cLowerValue == cUpperValue;
}

inline void MHBin::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline int MHBin::GetFrequency() const
{
	return nFrequency;
}
