// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe Sample
//    Sample
class Sample : public Object
{
public:
	// Constructeur
	Sample();
	~Sample();

	// Copie et duplication
	void CopyFrom(const Sample* aSource);
	Sample* Clone() const;

	///////////////////////////////////////////////////////////
	// Attributs de base

	// Key first field
	const ALString& GetFirst() const;
	void SetFirst(const ALString& sValue);

	// Key second field
	int GetSecond() const;
	void SetSecond(int nValue);

	// String
	const ALString& GetString() const;
	void SetString(const ALString& sValue);

	// Integer
	int GetInteger() const;
	void SetInteger(int nValue);

	///////////////////////////////////////////////////////////
	// Attributs de tests

	// Transient 1
	int GetTransient1() const;
	void SetTransient1(int nValue);

	// Transient 2
	int GetTransient2() const;
	void SetTransient2(int nValue);

	// Double
	double GetDouble() const;
	void SetDouble(double dValue);

	// Char
	char GetChar() const;
	void SetChar(char cValue);

	// Boolean
	boolean GetBoolean() const;
	void SetBoolean(boolean bValue);

	// Transient 3
	int GetTransient3() const;
	void SetTransient3(int nValue);

	// Derived 1
	int GetDerived1() const;
	void SetDerived1(int nValue);

	// Derived 2
	const ALString& GetDerived2() const;
	void SetDerived2(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sFirst;
	int nSecond;
	ALString sString;
	int nInteger;
	int nTransient1;
	int nTransient2;
	double dDouble;
	char cChar;
	boolean bBoolean;
	int nTransient3;
	int nDerived1;
	ALString sDerived2;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& Sample::GetFirst() const
{
	return sFirst;
}

inline void Sample::SetFirst(const ALString& sValue)
{
	sFirst = sValue;
}

inline int Sample::GetSecond() const
{
	return nSecond;
}

inline void Sample::SetSecond(int nValue)
{
	nSecond = nValue;
}

inline const ALString& Sample::GetString() const
{
	return sString;
}

inline void Sample::SetString(const ALString& sValue)
{
	sString = sValue;
}

inline int Sample::GetInteger() const
{
	return nInteger;
}

inline void Sample::SetInteger(int nValue)
{
	nInteger = nValue;
}

inline int Sample::GetTransient1() const
{
	return nTransient1;
}

inline void Sample::SetTransient1(int nValue)
{
	nTransient1 = nValue;
}

inline int Sample::GetTransient2() const
{
	return nTransient2;
}

inline void Sample::SetTransient2(int nValue)
{
	nTransient2 = nValue;
}

inline double Sample::GetDouble() const
{
	return dDouble;
}

inline void Sample::SetDouble(double dValue)
{
	dDouble = dValue;
}

inline char Sample::GetChar() const
{
	return cChar;
}

inline void Sample::SetChar(char cValue)
{
	cChar = cValue;
}

inline boolean Sample::GetBoolean() const
{
	return bBoolean;
}

inline void Sample::SetBoolean(boolean bValue)
{
	bBoolean = bValue;
}

inline int Sample::GetTransient3() const
{
	return nTransient3;
}

inline void Sample::SetTransient3(int nValue)
{
	nTransient3 = nValue;
}

inline int Sample::GetDerived1() const
{
	return nDerived1;
}

inline void Sample::SetDerived1(int nValue)
{
	nDerived1 = nValue;
}

inline const ALString& Sample::GetDerived2() const
{
	return sDerived2;
}

inline void Sample::SetDerived2(const ALString& sValue)
{
	sDerived2 = sValue;
}

// ## Custom inlines

// ##
