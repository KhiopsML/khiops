// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLSharedObject.h"

class PLShared_StringVector;
class PLShared_IntVector;
class PLShared_LongintVector;
class PLShared_DoubleVector;
class PLShared_CharVector;

//////////////////////////////////////////////////////////
// Classe PLShared_StringVector
// Serialisation de la classe StringVector
class PLShared_StringVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_StringVector();
	~PLShared_StringVector();

	// Acces au vecteur
	StringVector* GetStringVector();
	const StringVector* GetConstStringVector() const;

	// Acces aux valeurs
	const ALString& GetAt(int nIndex) const;
	void SetAt(int nIndex, const ALString& sValue);
	void Add(const ALString& nValue);

	// Taille du vecteur
	int GetSize() const;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer*, const Object*) const override;
	void DeserializeObject(PLSerializer*, Object*) const override;
	Object* Create() const override;
};

//////////////////////////////////////////////////////////
// Classe PLShared_IntVector
// Serialisation de la classe IntVector
class PLShared_IntVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_IntVector();
	~PLShared_IntVector();

	// Acces aux valeurs
	int GetAt(int nIndex) const;
	void SetAt(int nIndex, int nValue);
	void Add(int nValue);

	// Taille du vecteur
	int GetSize();

	// Acces au vecteur
	const IntVector* GetConstIntVector() const;
	IntVector* GetIntVector();

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer*, const Object*) const override;
	void DeserializeObject(PLSerializer*, Object*) const override;
	Object* Create() const override;
};

//////////////////////////////////////////////////////////
// Classe PLShared_LongintVector
// Serialisation de la classe LongIntVector
class PLShared_LongintVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_LongintVector();
	~PLShared_LongintVector();

	// Acces aux valeurs
	longint GetAt(int nIndex) const;
	void SetAt(int nIndex, longint lValue);
	void Add(longint lValue);

	// Taille du vecteur
	int GetSize();

	// Acces au vecteur
	const LongintVector* GetConstLongintVector() const;
	LongintVector* GetLongintVector();

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer*, const Object*) const override;
	void DeserializeObject(PLSerializer*, Object*) const override;
	Object* Create() const override;
};

//////////////////////////////////////////////////////////
// Classe PLShared_DoubleVector
// Serialisation de la classe DoubleVector
class PLShared_DoubleVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DoubleVector(void);
	~PLShared_DoubleVector(void);

	// Acces aux valeurs
	double GetAt(int nIndex) const;
	void SetAt(int nIndex, double dValue);
	void Add(double dValue);

	// Acces au vecteur
	const DoubleVector* GetConstDoubleVector() const;
	DoubleVector* GetDoubleVector();

	// Taille du vecteur
	int GetSize() const;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer*, const Object*) const override;
	void DeserializeObject(PLSerializer*, Object*) const override;
	Object* Create() const override;
};

//////////////////////////////////////////////////////////
// Classe PLShared_CharVector
// Serialisation de la classe CharVector
class PLShared_CharVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_CharVector(void);
	~PLShared_CharVector(void);

	// Acces aux valeurs
	char GetAt(int nIndex) const;
	void SetAt(int nIndex, char cValue);
	void Add(char cValue);

	// Taille du vecteur
	int GetSize() const;

	// Acces au vecteur
	const CharVector* GetConstCharVector() const;
	CharVector* GetCharVector();

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer*, const Object*) const override;
	void DeserializeObject(PLSerializer*, Object*) const override;
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline longint PLShared_LongintVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstLongintVector()->GetAt(nIndex);
}

inline void PLShared_LongintVector::SetAt(int nIndex, longint lValue)
{
	require(bIsWritable);
	GetLongintVector()->SetAt(nIndex, lValue);
}

inline void PLShared_LongintVector::Add(longint lValue)
{
	require(bIsWritable);
	GetLongintVector()->Add(lValue);
}

inline int PLShared_LongintVector::GetSize()
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstLongintVector()->GetSize();
}

inline const LongintVector* PLShared_LongintVector::GetConstLongintVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(LongintVector*, GetObject());
}

inline LongintVector* PLShared_LongintVector::GetLongintVector()
{
	require(bIsWritable);
	require(bIsDeclared);

	return cast(LongintVector*, GetObject());
}

inline double PLShared_DoubleVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstDoubleVector()->GetAt(nIndex);
}

inline void PLShared_DoubleVector::SetAt(int nIndex, double dValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetDoubleVector()->SetAt(nIndex, dValue);
}

inline void PLShared_DoubleVector::Add(double dValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetDoubleVector()->Add(dValue);
}

inline int PLShared_DoubleVector::GetSize() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstDoubleVector()->GetSize();
}

inline const DoubleVector* PLShared_DoubleVector::GetConstDoubleVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(DoubleVector*, GetObject());
}

inline DoubleVector* PLShared_DoubleVector::GetDoubleVector()
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(DoubleVector*, GetObject());
	;
}

inline char PLShared_CharVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstCharVector()->GetAt(nIndex);
}

inline void PLShared_CharVector::SetAt(int nIndex, char cValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetCharVector()->SetAt(nIndex, cValue);
}

inline void PLShared_CharVector::Add(char cValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetCharVector()->Add(cValue);
}

inline int PLShared_CharVector::GetSize() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstCharVector()->GetSize();
}

inline const CharVector* PLShared_CharVector::GetConstCharVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(CharVector*, GetObject());
}

inline CharVector* PLShared_CharVector::GetCharVector()
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(CharVector*, GetObject());
}
