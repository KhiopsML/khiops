// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWObject.h"
#include "ALString.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////////////////
// Token avec son effectif
class KDTokenFrequency : public Object
{
public:
	// Constructeur
	KDTokenFrequency();
	~KDTokenFrequency();

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

	// Compte des caracteres par type
	int GetSpaceCharNumber() const;
	int GetPunctuationCharNumber() const;

	// Comparaison sur le token croissant, puis sur l'effectif decroissant
	int CompareToken(const KDTokenFrequency* otherTokenFrequency) const;

	// Comparaison sur sur l'effectif decroissant, puis sur le token croissant
	int CompareFrequency(const KDTokenFrequency* otherTokenFrequency) const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ALString sToken;
	longint lFrequency;
};

// Comparaison de deux objets KDTokenFrequency sur le token croissant, puis sur l'effectif decroissant
int KDTokenFrequencyCompareToken(const void* elem1, const void* elem2);

// Comparaison de deux objets KDTokenFrequency sur l'effectif decroissant, puis sur le token croissant
int KDTokenFrequencyCompareFrequency(const void* elem1, const void* elem2);

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_TokenFrequency
// Serialisation de KDTokenFrequency
class PLShared_TokenFrequency : public PLSharedObject
{
public:
	// Constructeur
	PLShared_TokenFrequency();
	~PLShared_TokenFrequency();

	// Acces au IntObject
	void SetTokenFrequency(KDTokenFrequency* tfObject);
	KDTokenFrequency* GetTokenFrequency();

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

inline KDTokenFrequency::KDTokenFrequency()
{
	lFrequency = 0;
}

inline KDTokenFrequency::~KDTokenFrequency() {}

inline void KDTokenFrequency::Initialize()
{
	sToken = "";
	lFrequency = 0;
}

inline void KDTokenFrequency::SetToken(const ALString& sValue)
{
	sToken = sValue;
}

inline const ALString& KDTokenFrequency::GetToken() const
{
	return sToken;
}

inline void KDTokenFrequency::SetFrequency(longint lValue)
{
	require(lValue >= 0);
	lFrequency = lValue;
}

inline longint KDTokenFrequency::GetFrequency() const
{
	return lFrequency;
}

inline void KDTokenFrequency::IncrementFrequency()
{
	lFrequency++;
}

inline int KDTokenFrequency::CompareToken(const KDTokenFrequency* otherTokenFrequency) const
{
	int nCompare;
	require(otherTokenFrequency != NULL);
	nCompare = GetToken().Compare(otherTokenFrequency->GetToken());
	if (nCompare == 0)
		nCompare = -CompareLongint(GetFrequency(), otherTokenFrequency->GetFrequency());
	return nCompare;
}

inline int KDTokenFrequency::CompareFrequency(const KDTokenFrequency* otherTokenFrequency) const
{
	int nCompare;
	require(otherTokenFrequency != NULL);
	nCompare = -CompareLongint(GetFrequency(), otherTokenFrequency->GetFrequency());
	if (nCompare == 0)
		nCompare = GetToken().Compare(otherTokenFrequency->GetToken());
	return nCompare;
}

inline void PLShared_TokenFrequency::SetTokenFrequency(KDTokenFrequency* tfObject)
{
	SetObject(tfObject);
}

inline KDTokenFrequency* PLShared_TokenFrequency::GetTokenFrequency()
{
	return cast(KDTokenFrequency*, GetObject());
}

inline Object* PLShared_TokenFrequency::Create() const
{
	return new KDTokenFrequency;
}
