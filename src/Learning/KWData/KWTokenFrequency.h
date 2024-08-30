// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTokenFrequency;
class PLShared_TokenFrequency;

#include "Object.h"
#include "ALString.h"
#include "TextService.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////////////////
// Token avec son effectif
class KWTokenFrequency : public Object
{
public:
	// Constructeur
	KWTokenFrequency();
	~KWTokenFrequency();

	// (Re)initialisation
	void Initialize();

	// Token
	void SetToken(const ALString& sValue);
	const ALString& GetToken() const;

	// Effectif
	void SetFrequency(longint lValue);
	longint GetFrequency() const;

	// Incrementation de l'effectif
	void IncrementFrequency();

	// Comparaison sur le token croissant, puis sur l'effectif decroissant
	int CompareToken(const KWTokenFrequency* otherTokenFrequency) const;

	// Comparaison sur l'effectif decroissant, puis la longueur de token croissante, puis le token croissant
	int CompareFrequency(const KWTokenFrequency* otherTokenFrequency) const;

	// Comparaison sur la longueur croissante, puis sur l'effectif decroissant, puis sur le token croissant
	int CompareLengthFrequency(const KWTokenFrequency* otherTokenFrequency) const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ALString sToken;
	longint lFrequency;
};

// Comparaison de deux objets KWTokenFrequency sur le token croissant, puis sur l'effectif decroissant
int KWTokenFrequencyCompareToken(const void* elem1, const void* elem2);

// Comparaison de deux objets KWTokenFrequency sur l'effectif decroissant, puis la longueur croissante, puis sur le
// token croissant
int KWTokenFrequencyCompareFrequency(const void* elem1, const void* elem2);

// Comparaison de deux objets KWTokenFrequency sur la longueur de token croissant, puis l'effectif decroissant, puis sur
// le token croissant
int KWTokenFrequencyCompareLengthFrequency(const void* elem1, const void* elem2);

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_TokenFrequency
// Serialisation de KWTokenFrequency
class PLShared_TokenFrequency : public PLSharedObject
{
public:
	// Constructeur
	PLShared_TokenFrequency();
	~PLShared_TokenFrequency();

	// Acces au TokenFrequency
	void SetTokenFrequency(KWTokenFrequency* tfObject);
	KWTokenFrequency* GetTokenFrequency();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////
// Methodes en inline

inline KWTokenFrequency::KWTokenFrequency()
{
	lFrequency = 0;
}

inline KWTokenFrequency::~KWTokenFrequency() {}

inline void KWTokenFrequency::Initialize()
{
	sToken = "";
	lFrequency = 0;
}

inline void KWTokenFrequency::SetToken(const ALString& sValue)
{
	sToken = sValue;
}

inline const ALString& KWTokenFrequency::GetToken() const
{
	return sToken;
}

inline void KWTokenFrequency::SetFrequency(longint lValue)
{
	require(lValue >= 0);
	lFrequency = lValue;
}

inline longint KWTokenFrequency::GetFrequency() const
{
	return lFrequency;
}

inline void KWTokenFrequency::IncrementFrequency()
{
	lFrequency++;
}

inline int KWTokenFrequency::CompareToken(const KWTokenFrequency* otherTokenFrequency) const
{
	int nCompare;
	require(otherTokenFrequency != NULL);
	nCompare = GetToken().Compare(otherTokenFrequency->GetToken());
	if (nCompare == 0)
		nCompare = -CompareLongint(GetFrequency(), otherTokenFrequency->GetFrequency());
	return nCompare;
}

inline int KWTokenFrequency::CompareFrequency(const KWTokenFrequency* otherTokenFrequency) const
{
	int nCompare;
	require(otherTokenFrequency != NULL);
	nCompare = -CompareLongint(GetFrequency(), otherTokenFrequency->GetFrequency());
	if (nCompare == 0)
		nCompare = GetToken().GetLength() - otherTokenFrequency->GetToken().GetLength();
	if (nCompare == 0)
		nCompare = GetToken().Compare(otherTokenFrequency->GetToken());
	return nCompare;
}

inline int KWTokenFrequency::CompareLengthFrequency(const KWTokenFrequency* otherTokenFrequency) const
{
	int nCompare;
	require(otherTokenFrequency != NULL);
	nCompare = GetToken().GetLength() - otherTokenFrequency->GetToken().GetLength();
	if (nCompare == 0)
		nCompare = -CompareLongint(GetFrequency(), otherTokenFrequency->GetFrequency());
	if (nCompare == 0)
		nCompare = GetToken().Compare(otherTokenFrequency->GetToken());
	return nCompare;
}
