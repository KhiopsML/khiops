// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////
// Types simples serialises :
class PLSharedVariable; // classe virtuelle ancetre
class PLShared_Boolean;
class PLShared_Double;
class PLShared_Int;
class PLShared_Longint;
class PLShared_String;
class PLShared_Char;

#include "Object.h"
#include "PLSerializer.h"

//////////////////////////////////////////////////////////
// Classe  PLSharedVariable
// Classe virtuelle offrant des services de (de)serialisation.
// Les instances des classes filles  de PLSharedVariable sont
// transmisent automatiquement entre le maitre et les esclaves.
// (necessite l'appel l'une des methodes DeclareTaskInput,  DeclareSharedParameter
// ou DeclareTaskOutput de la classe PLParallelTask)
//
// Notes aux developpeurs :
// -Pour garantir une bonne utilisation des variables partagees dans les applications
// paralleles, les acces en lecture et ecriture doivent etre precedes de requires sur bIsWritable et bIsReadable.
// De plus, les acces en lecture doivent etre precedes de require(bIsDeclared).
// -La (de)serialisation est effectuee via la classe PLSerializer
//
class PLSharedVariable : public Object
{
public:
	// Constructeur
	PLSharedVariable();
	~PLSharedVariable();

	// Serialisation / deserialisation de la variable partagee
	// avec controles sur le type etc...
	// appelle les methodes SerializeValue() et DeserializeValue()
	// Le serializer doit etre ouvert en lecture/ecriture
	void Serialize(PLSerializer*) const;
	void Deserialize(PLSerializer*);

	// Nettoyage : liberation de la memoire, utile pour les variable partagee complexes
	// Cette methode est appelee automatiquement
	//	-avant la de-serialisation
	//	-apres l'envoi des variables pour liberer la memoire au plus tot
	//	-a la fin du programme
	virtual void Clean();

	// Test de toutes les variables simples
	// Appel du test de chaque classe
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes virtuelles a reimplementer

	// Serialisation / Deserialization de la valeur de la variable partagee (sans controle par assertion)
	virtual void SerializeValue(PLSerializer*) const = 0;
	virtual void DeserializeValue(PLSerializer*) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes techniques (privees)

	// Read only / read write
	boolean IsReadOnly() const;     // lecture seulement
	boolean IsReadAndWrite() const; // lecture et ecriture

	// Par defaut Read / write
	void SetReadWrite();
	void SetReadOnly();
	void SetNoPermission();

	// Lecture seule ou non
	boolean bIsReadable;
	boolean bIsWritable;

	// A bien ete declare
	boolean bIsDeclared;

	// Classes friend
	friend class PLParallelTask;
	friend class PLMPITaskDriver;
	friend class PLMPIMaster;
	friend class PLMPISlave;
};

//////////////////////////////////////////////////////////
// Classe PLShared_Boolean
// Se manipule comme un boolean, avec les operateurs de comparaison == et !=
class PLShared_Boolean : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_Boolean();
	~PLShared_Boolean();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateur d'affectation entre booleens et boolens partages
	const PLShared_Boolean& operator=(const PLShared_Boolean& shared_booleanValue);
	const PLShared_Boolean& operator=(boolean bValue);
	operator boolean() const;

protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Acces a la valeur
	boolean GetValue() const;
	void SetValue(boolean bValue);

	// Valeur
	boolean bValue;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Boolean& v1, const PLShared_Boolean& v2);
	friend boolean operator==(const PLShared_Boolean& v1, boolean i2);
	friend boolean operator==(boolean i1, const PLShared_Boolean& v2);
	friend boolean operator!=(const PLShared_Boolean& v1, const PLShared_Boolean& v2);
	friend boolean operator!=(const PLShared_Boolean& v1, boolean i2);
	friend boolean operator!=(boolean i1, const PLShared_Boolean& v2);
};

boolean operator==(const PLShared_Boolean& v1, const PLShared_Boolean& v2);
boolean operator==(const PLShared_Boolean& v1, boolean i2);
boolean operator==(boolean i1, const PLShared_Boolean& v2);
boolean operator!=(const PLShared_Boolean& v1, const PLShared_Boolean& v2);
boolean operator!=(const PLShared_Boolean& v1, boolean i2);
boolean operator!=(boolean i1, const PLShared_Boolean& v2);

//////////////////////////////////////////////////////////
// Classe PLShared_Double
// Se manipule comme un double, avec les operateurs arithmetique et de comparaison
class PLShared_Double : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_Double(void);
	~PLShared_Double(void);

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateur d'affectation entre double et double partages
	const PLShared_Double& operator=(const PLShared_Double& shared_doubleValue);
	const PLShared_Double& operator=(double dValue);
	operator double() const;

	// Operateurs de calcul
	PLShared_Double& operator++();
	void operator++(int);
	PLShared_Double& operator--();
	void operator--(int);
	PLShared_Double& operator+=(double);
	PLShared_Double& operator-=(double);
	PLShared_Double& operator*=(double);
	PLShared_Double& operator/=(double);

protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Acces a la valeur
	double GetValue() const;
	void SetValue(double dValue);

	// Comparaison de valeur
	int Compare(double dOtherValue) const;

	// Valeur
	double dValue;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator==(const PLShared_Double& v1, double v2);
	friend boolean operator==(double v1, const PLShared_Double& v2);
	friend boolean operator!=(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator!=(const PLShared_Double& v1, double v2);
	friend boolean operator!=(double v1, const PLShared_Double& v2);
	friend boolean operator<(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator<(const PLShared_Double& v1, double v2);
	friend boolean operator<(double v1, const PLShared_Double& v2);
	friend boolean operator>(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator>(const PLShared_Double& v1, double v2);
	friend boolean operator>(double v1, const PLShared_Double& v2);
	friend boolean operator<=(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator<=(const PLShared_Double& v1, double v2);
	friend boolean operator<=(double v1, const PLShared_Double& v2);
	friend boolean operator>=(const PLShared_Double& v1, const PLShared_Double& v2);
	friend boolean operator>=(const PLShared_Double& v1, double v2);
	friend boolean operator>=(double v1, const PLShared_Double& v2);
};

// Operateurs de comparaison entre entier et entiers partages
boolean operator==(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator==(const PLShared_Double& v1, double v2);
boolean operator==(double v1, const PLShared_Double& v2);
boolean operator!=(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator!=(const PLShared_Double& v1, double v2);
boolean operator!=(double v1, const PLShared_Double& v2);
boolean operator<(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator<(const PLShared_Double& v1, double v2);
boolean operator<(double v1, const PLShared_Double& v2);
boolean operator>(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator>(const PLShared_Double& v1, double v2);
boolean operator>(double v1, const PLShared_Double& v2);
boolean operator<=(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator<=(const PLShared_Double& v1, double v2);
boolean operator<=(double v1, const PLShared_Double& v2);
boolean operator>=(const PLShared_Double& v1, const PLShared_Double& v2);
boolean operator>=(const PLShared_Double& v1, double v2);
boolean operator>=(double v1, const PLShared_Double& v2);

//////////////////////////////////////////////////////////
// Classe PLShared_Int
// Se manipule comme un int, avec les operateurs arithmetique et de comparaison
class PLShared_Int : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_Int();
	~PLShared_Int();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateur d'affectation entre entiers et entiers partages
	const PLShared_Int& operator=(const PLShared_Int& shared_intValue);
	const PLShared_Int& operator=(int nValue);
	operator int() const;

	// Operateurs de calcul
	PLShared_Int& operator++();
	void operator++(int);
	PLShared_Int& operator--();
	void operator--(int);
	PLShared_Int& operator+=(int);
	PLShared_Int& operator-=(int);
	PLShared_Int& operator*=(int);
	PLShared_Int& operator/=(int);

protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Acces a la valeur
	int GetValue() const;
	void SetValue(int nValue);

	// Comparaison de valeur
	int Compare(int nOtherValue) const;

	// Valeur
	int nValue;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator==(const PLShared_Int& s_i1, int i2);
	friend boolean operator==(int i1, const PLShared_Int& s_i2);
	friend boolean operator!=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator!=(const PLShared_Int& s_i1, int i2);
	friend boolean operator!=(int i1, const PLShared_Int& s_i2);
	friend boolean operator<(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator<(const PLShared_Int& s_i1, int i2);
	friend boolean operator<(int i1, const PLShared_Int& s_i2);
	friend boolean operator>(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator>(const PLShared_Int& s_i1, int i2);
	friend boolean operator>(int i1, const PLShared_Int& s_i2);
	friend boolean operator<=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator<=(const PLShared_Int& s_i1, int i2);
	friend boolean operator<=(int i1, const PLShared_Int& s_i2);
	friend boolean operator>=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
	friend boolean operator>=(const PLShared_Int& s_i1, int i2);
	friend boolean operator>=(int i1, const PLShared_Int& s_i2);
};

// Operateurs de comparaison entre entier et entiers partages
boolean operator==(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator==(const PLShared_Int& s_i1, int i2);
boolean operator==(int i1, const PLShared_Int& s_i2);
boolean operator!=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator!=(const PLShared_Int& s_i1, int i2);
boolean operator!=(int i1, const PLShared_Int& s_i2);
boolean operator<(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator<(const PLShared_Int& s_i1, int i2);
boolean operator<(int i1, const PLShared_Int& s_i2);
boolean operator>(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator>(const PLShared_Int& s_i1, int i2);
boolean operator>(int i1, const PLShared_Int& s_i2);
boolean operator<=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator<=(const PLShared_Int& s_i1, int i2);
boolean operator<=(int i1, const PLShared_Int& s_i2);
boolean operator>=(const PLShared_Int& s_i1, const PLShared_Int& s_i2);
boolean operator>=(const PLShared_Int& s_i1, int i2);
boolean operator>=(int i1, const PLShared_Int& s_i2);

//////////////////////////////////////////////////////////
// Classe PLShared_Longint
// Se manipule comme un longint, avec les operateurs arithmetiques et de comparaison
class PLShared_Longint : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_Longint();
	~PLShared_Longint();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateur d'affectation entre entiers longs et entiers longs partages
	const PLShared_Longint& operator=(const PLShared_Longint& shared_lValue);
	const PLShared_Longint& operator=(longint lValue);
	operator longint() const;

	// Operateurs de calcul
	PLShared_Longint& operator++();
	void operator++(int);
	PLShared_Longint& operator--();
	void operator--(int);
	PLShared_Longint& operator+=(longint);
	PLShared_Longint& operator-=(longint);
	PLShared_Longint& operator*=(longint);
	PLShared_Longint& operator/=(longint);

protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Acces a la valeur
	longint GetValue() const;
	void SetValue(longint lValue);

	// Comparaison de valeur
	int Compare(longint lOtherValue) const;

	// Valeur
	longint lValue;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator==(const PLShared_Longint& v1, longint v2);
	friend boolean operator==(longint v1, const PLShared_Longint& v2);
	friend boolean operator!=(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator!=(const PLShared_Longint& v1, longint v2);
	friend boolean operator!=(longint v1, const PLShared_Longint& v2);
	friend boolean operator<(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator<(const PLShared_Longint& v1, longint v2);
	friend boolean operator<(longint v1, const PLShared_Longint& v2);
	friend boolean operator>(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator>(const PLShared_Longint& v1, longint v2);
	friend boolean operator>(longint v1, const PLShared_Longint& v2);
	friend boolean operator<=(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator<=(const PLShared_Longint& v1, longint v2);
	friend boolean operator<=(longint v1, const PLShared_Longint& v2);
	friend boolean operator>=(const PLShared_Longint& v1, const PLShared_Longint& v2);
	friend boolean operator>=(const PLShared_Longint& v1, longint v2);
	friend boolean operator>=(longint v1, const PLShared_Longint& v2);
};

// Operateurs de comparaison entre entier et entiers partages
boolean operator==(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator==(const PLShared_Longint& v1, longint v2);
boolean operator==(longint v1, const PLShared_Longint& v2);
boolean operator!=(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator!=(const PLShared_Longint& v1, longint v2);
boolean operator!=(longint v1, const PLShared_Longint& v2);
boolean operator<(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator<(const PLShared_Longint& v1, longint v2);
boolean operator<(longint v1, const PLShared_Longint& v2);
boolean operator>(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator>(const PLShared_Longint& v1, longint v2);
boolean operator>(longint v1, const PLShared_Longint& v2);
boolean operator<=(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator<=(const PLShared_Longint& v1, longint v2);
boolean operator<=(longint v1, const PLShared_Longint& v2);
boolean operator>=(const PLShared_Longint& v1, const PLShared_Longint& v2);
boolean operator>=(const PLShared_Longint& v1, longint v2);
boolean operator>=(longint v1, const PLShared_Longint& v2);

//////////////////////////////////////////////////////////
// Classe PLShared_String
class PLShared_String : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_String(void);
	~PLShared_String(void);

	// Acces a la valeur
	const ALString& GetValue() const;
	void SetValue(const ALString& sValue);

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Valeur
	ALString sValue;
};

//////////////////////////////////////////////////////////
// Classe PLShared_Char
class PLShared_Char : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_Char(void);
	~PLShared_Char(void);

	// Acces a la valeur
	const char& GetValue() const;
	void SetValue(const char& cValue);

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Valeur
	char cValue;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLSharedVariable::Clean() {}

inline void PLSharedVariable::SetReadWrite()
{
	require(bIsDeclared);
	bIsReadable = true;
	bIsWritable = true;
}

inline void PLSharedVariable::SetReadOnly()
{
	require(bIsDeclared);
	bIsReadable = true;
	bIsWritable = false;
}

inline void PLSharedVariable::SetNoPermission()
{
	require(bIsDeclared);
	bIsReadable = false;
	bIsWritable = false;
}

inline boolean PLSharedVariable::IsReadOnly() const
{
	require(bIsDeclared);
	return bIsReadable and not bIsWritable;
}

inline boolean PLSharedVariable::IsReadAndWrite() const
{
	require(bIsDeclared);
	return bIsReadable and bIsWritable;
}

inline boolean PLShared_Boolean::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return bValue;
}

inline void PLShared_Boolean::SetValue(boolean bvalue)
{
	assert(bIsWritable);
	require(bIsDeclared);
	this->bValue = bvalue;
}

inline PLShared_Boolean::operator boolean() const
{
	return GetValue();
}

inline const PLShared_Boolean& PLShared_Boolean::operator=(const PLShared_Boolean& shared_intValue)
{
	this->SetValue(shared_intValue.GetValue());
	return *this;
}

inline const PLShared_Boolean& PLShared_Boolean::operator=(boolean b)
{
	this->SetValue(b);
	return *this;
}

inline boolean operator==(const PLShared_Boolean& v1, const PLShared_Boolean& v2)
{
	return v1.GetValue() == v2.GetValue();
}

inline boolean operator==(const PLShared_Boolean& v1, boolean v2)
{
	return v1.GetValue() == v2;
}

inline boolean operator==(boolean v1, const PLShared_Boolean& v2)
{
	return v1 == v2.GetValue();
}

inline boolean operator!=(const PLShared_Boolean& v1, const PLShared_Boolean& v2)
{
	return v1.GetValue() != v2.GetValue();
}

inline boolean operator!=(const PLShared_Boolean& v1, boolean v2)
{
	return v1.GetValue() != v2;
}

inline boolean operator!=(boolean v1, const PLShared_Boolean& v2)
{
	return v1 != v2.GetValue();
}

inline double PLShared_Double::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return dValue;
}

inline void PLShared_Double::SetValue(double d)
{
	require(bIsWritable);
	this->dValue = d;
}

inline int PLShared_Double::Compare(double dOtherValue) const
{
	if (GetValue() > dOtherValue)
		return 1;
	else if (GetValue() < dOtherValue)
		return -1;
	else
		return 0;
}

inline PLShared_Double::operator double() const
{
	return GetValue();
}

inline const PLShared_Double& PLShared_Double::operator=(const PLShared_Double& shared_doubleValue)
{
	this->SetValue(shared_doubleValue.GetValue());
	return *this;
}

inline const PLShared_Double& PLShared_Double::operator=(double d)
{
	this->SetValue(d);
	return *this;
}

inline PLShared_Double& PLShared_Double::operator++()
{
	SetValue(GetValue() + 1);
	return (*this);
}

inline void PLShared_Double::operator++(int)
{
	require(bIsWritable);
	require(bIsDeclared);
	++(*this);
}

inline PLShared_Double& PLShared_Double::operator--()
{
	SetValue(GetValue() - 1);
	return (*this);
}

inline void PLShared_Double::operator--(int)
{
	require(bIsWritable);
	require(bIsDeclared);
	--(*this);
}

inline PLShared_Double& PLShared_Double::operator+=(double d)
{
	SetValue(GetValue() + d);
	return *this;
}

inline PLShared_Double& PLShared_Double::operator-=(double d)
{
	SetValue(GetValue() - d);
	return *this;
}

inline PLShared_Double& PLShared_Double::operator*=(double d)
{
	SetValue(GetValue() * d);
	return *this;
}

inline PLShared_Double& PLShared_Double::operator/=(double d)
{
	SetValue(GetValue() / d);
	return *this;
}

inline boolean operator==(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() == v2.GetValue();
}

inline boolean operator==(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() == v2;
}

inline boolean operator==(double v1, const PLShared_Double& v2)
{
	return v1 == v2.GetValue();
}

inline boolean operator!=(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() != v2.GetValue();
}

inline boolean operator!=(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() != v2;
}

inline boolean operator!=(double v1, const PLShared_Double& v2)
{
	return v1 != v2.GetValue();
}

inline boolean operator<(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() < v2.GetValue();
}

inline boolean operator<(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() < v2;
}

inline boolean operator<(double v1, const PLShared_Double& v2)
{
	return v1 < v2.GetValue();
}

inline boolean operator>(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() > v2.GetValue();
}

inline boolean operator>(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() > v2;
}

inline boolean operator>(double v1, const PLShared_Double& v2)
{
	return v1 > v2.GetValue();
}

inline boolean operator<=(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() <= v2.GetValue();
}

inline boolean operator<=(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() <= v2;
}

inline boolean operator<=(double v1, const PLShared_Double& v2)
{
	return v1 <= v2.GetValue();
}

inline boolean operator>=(const PLShared_Double& v1, const PLShared_Double& v2)
{
	return v1.GetValue() >= v2.GetValue();
}

inline boolean operator>=(const PLShared_Double& v1, double v2)
{
	return v1.GetValue() >= v2;
}

inline boolean operator>=(double v1, const PLShared_Double& v2)
{
	return v1 >= v2.GetValue();
}

inline int PLShared_Int::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return nValue;
}

inline void PLShared_Int::SetValue(int nvalue)
{
	require(bIsWritable);
	require(bIsDeclared);
	this->nValue = nvalue;
}

inline const PLShared_Int& PLShared_Int::operator=(const PLShared_Int& shared_intValue)
{
	SetValue(shared_intValue.GetValue());
	return *this;
}

inline const PLShared_Int& PLShared_Int::operator=(int n)
{
	SetValue(n);
	return *this;
}

inline PLShared_Int::operator int() const
{
	return GetValue();
}

inline PLShared_Int& PLShared_Int::operator++()
{
	SetValue(GetValue() + 1);
	return (*this);
}

inline void PLShared_Int::operator++(int)
{
	require(bIsWritable);
	require(bIsDeclared);
	++(*this);
}

inline PLShared_Int& PLShared_Int::operator--()
{
	SetValue(GetValue() - 1);
	return (*this);
}

inline void PLShared_Int::operator--(int)
{
	require(bIsWritable);
	require(bIsDeclared);
	--(*this);
}

inline PLShared_Int& PLShared_Int::operator+=(int n)
{
	SetValue(GetValue() + n);
	return *this;
}

inline PLShared_Int& PLShared_Int::operator-=(int n)
{
	SetValue(GetValue() - n);
	return *this;
}

inline PLShared_Int& PLShared_Int::operator*=(int n)
{
	SetValue(GetValue() * n);
	return *this;
}

inline PLShared_Int& PLShared_Int::operator/=(int n)
{
	SetValue(GetValue() / n);
	return *this;
}

inline int PLShared_Int::Compare(int nOtherValue) const
{
	return GetValue() - nOtherValue;
}

inline longint PLShared_Longint::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return lValue;
}

inline void PLShared_Longint::SetValue(longint l)
{
	require(bIsWritable);
	require(bIsDeclared);
	this->lValue = l;
}

inline const PLShared_Longint& PLShared_Longint::operator=(const PLShared_Longint& shared_longintValue)
{
	SetValue(shared_longintValue.GetValue());
	return *this;
}

inline const PLShared_Longint& PLShared_Longint::operator=(longint l)
{
	SetValue(l);
	return *this;
}

inline PLShared_Longint::operator longint() const
{
	return GetValue();
}

inline PLShared_Longint& PLShared_Longint::operator++()
{
	SetValue(GetValue() + 1);
	return (*this);
}

inline void PLShared_Longint::operator++(int)
{
	require(bIsWritable);
	++(*this);
}

inline PLShared_Longint& PLShared_Longint::operator--()
{
	SetValue(GetValue() - 1);
	return (*this);
}

inline void PLShared_Longint::operator--(int)
{
	require(bIsWritable);
	--(*this);
}

inline PLShared_Longint& PLShared_Longint::operator+=(longint lvalue)
{
	SetValue(GetValue() + lvalue);
	return *this;
}

inline PLShared_Longint& PLShared_Longint::operator-=(longint lvalue)
{
	SetValue(GetValue() - lvalue);
	return *this;
}

inline PLShared_Longint& PLShared_Longint::operator*=(longint lvalue)
{
	SetValue(GetValue() * lvalue);
	return *this;
}

inline PLShared_Longint& PLShared_Longint::operator/=(longint lvalue)
{
	SetValue(GetValue() / lvalue);
	return *this;
}

inline int PLShared_Longint::Compare(longint lOtherValue) const
{
	if (GetValue() > lOtherValue)
		return 1;
	if (GetValue() < lOtherValue)
		return -1;
	return 0;
}

inline boolean operator==(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() == v2.GetValue();
}

inline boolean operator==(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() == v2;
}

inline boolean operator==(longint v1, const PLShared_Longint& v2)
{
	return v1 == v2.GetValue();
}

inline boolean operator!=(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() != v2.GetValue();
}

inline boolean operator!=(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() != v2;
}

inline boolean operator!=(longint v1, const PLShared_Longint& v2)
{
	return v1 != v2.GetValue();
}

inline boolean operator<(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() < v2.GetValue();
}

inline boolean operator<(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() < v2;
}

inline boolean operator<(longint v1, const PLShared_Longint& v2)
{
	return v1 < v2.GetValue();
}

inline boolean operator>(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() > v2.GetValue();
}

inline boolean operator>(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() > v2;
}

inline boolean operator>(longint v1, const PLShared_Longint& v2)
{
	return v1 > v2.GetValue();
}

inline boolean operator<=(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() <= v2.GetValue();
}

inline boolean operator<=(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() <= v2;
}

inline boolean operator<=(longint v1, const PLShared_Longint& v2)
{
	return v1 <= v2.GetValue();
}

inline boolean operator>=(const PLShared_Longint& v1, const PLShared_Longint& v2)
{
	return v1.GetValue() >= v2.GetValue();
}

inline boolean operator>=(const PLShared_Longint& v1, longint v2)
{
	return v1.GetValue() >= v2;
}

inline boolean operator>=(longint v1, const PLShared_Longint& v2)
{
	return v1 >= v2.GetValue();
}

inline const ALString& PLShared_String::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return sValue;
}

inline void PLShared_String::SetValue(const ALString& s)
{
	require(bIsWritable);
	require(bIsDeclared);
	this->sValue = s;
}

inline const char& PLShared_Char::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return cValue;
}

inline void PLShared_Char::SetValue(const char& c)
{
	require(bIsWritable);
	require(bIsDeclared);
	cValue = c;
}

inline boolean operator==(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) == 0;
}

inline boolean operator==(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) == 0;
}

inline boolean operator==(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) == 0;
}

inline boolean operator!=(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) != 0;
}

inline boolean operator!=(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) != 0;
}

inline boolean operator!=(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) != 0;
}

inline boolean operator<(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) < 0;
}

inline boolean operator<(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) < 0;
}

inline boolean operator<(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) > 0;
}

inline boolean operator>(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) > 0;
}

inline boolean operator>(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) > 0;
}

inline boolean operator>(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) < 0;
}

inline boolean operator<=(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) <= 0;
}

inline boolean operator<=(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) <= 0;
}

inline boolean operator<=(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) >= 0;
}

inline boolean operator>=(const PLShared_Int& s_i1, const PLShared_Int& s_i2)
{
	return s_i1.Compare(s_i2) >= 0;
}

inline boolean operator>=(const PLShared_Int& s_i1, int i2)
{
	return s_i1.Compare(i2) >= 0;
}

inline boolean operator>=(int i1, const PLShared_Int& s_i2)
{
	return s_i2.Compare(i1) <= 0;
}

inline void PLShared_Int::Clean()
{
	nValue = 0;
}

inline void PLShared_Double::Clean()
{
	dValue = 0;
}

inline void PLShared_Boolean::Clean()
{
	bValue = true;
}
inline void PLShared_Longint::Clean()
{
	lValue = 0;
}

inline void PLShared_String::Clean()
{
	sValue = "";
}

inline void PLShared_Char::Clean()
{
	cValue = '\0';
}
