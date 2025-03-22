// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWContinuous;
class ContinuousObject;
class ContinuousVector;
class PLShared_Continuous;
class PLShared_ContinuousVector;

#include "Standard.h"
#include "Object.h"
#include "MemVector.h"
#include "Timer.h"
#include "PLSharedVariable.h"
#include "KWContinuous.h"
#include "PLSharedObject.h"

//////////////////////////////////////////////////////////
// Definition du type Continuous et des services attaches
// sous forme de methodes de classe

// Declaration du type continu (prefixe: c)
typedef double Continuous;

//////////////////////////////////////
// Classe KWContinuous
// Services de classe de gestion du type Continuous
class KWContinuous : public Object
{
public:
	// Conversion vers une chaine de caractere
	// Missing value est converti en chaine vide
	// Attention: toute sortie doit passer par cette methode (en particulier
	// avec les stream, ne pas utiliser directement la valeur Continuous)
	static const char* const ContinuousToString(Continuous cValue);

	// Conversion depuis une chaine de caractere
	// En cas de probleme, la conversion se fait au mieux
	//     chaine vide ou probleme de conversion: MissingValue
	//     underflow : 0
	//     overflow -inf: MinValue
	//     overflow +inf: MaxValue
	static Continuous StringToContinuous(const char* const sValue);

	// Conversion depuis un double
	// Resolution des problemes de limites numeriques (valeur min, max, nombre
	// de chiffres significatifs) pour etre compatibles avec les Continuous
	// Important pour normaliser la representation interne et empecher de discerner
	// des Continuous indiscernables
	static Continuous DoubleToContinuous(double dValue);

	// Conversion vers un int
	// Renvoie true si la conversion est un succes
	static boolean ContinuousToInt(Continuous cValue, int& nValue);

	// Test si une valeur est entiere
	static boolean IsInt(Continuous cValue);

	// Test si une chaine contient un Continuous (avec tolerance sur les underflow et overflow)
	static boolean IsStringContinuous(const char* const sValue);

	// Diagnostique sur le type d'erreur de conversion d'un Continuous
	enum
	{
		NoError,               // Conversion OK
		ErrorUnderflow,        // Underflow (nombre trop proche de zero)
		ErrorNegativeOverflow, // Overflow -inf
		ErrorPositiveOverflow, // Overflow +inf
		ErrorBadString,        // La chaine n'a pu etre convertie
		ErrorBadEndString
	}; // La fin de la chaine n'a pu etre convertie
	static int StringToContinuousError(const char* const sValue, Continuous& cValue);

	// Test de validite du numero d'erreur
	static boolean CheckError(int nValue);

	// Libelle d'erreur (en anglais) associe a un diagnostic de conversion
	static const ALString ErrorLabel(int nError);

	// Methode utilitaire pour tranformer le separateur decimal ',' en '.'
	static void PreprocessString(char* sValue);

	// Valeurs Min et Max
	// Seules les valeurs entre Min et Max sont des valeurs standards
	static Continuous GetMinValue();
	static Continuous GetMaxValue();

	// Plus petite valeur strictement positive
	static Continuous GetEpsilonValue();

	// Valeur manquante
	// La valeur manquante est representee par -INF, et utilisable pour des comparaisons
	static Continuous GetMissingValue();

	// Valeur interdite (en fait: + INF)
	static Continuous GetForbiddenValue();

	// Valeur moyenne entre deux valeurs
	// En cas de limite de precision numerique, la moyenne peut coincider avec une des deux valeurs,
	// auquel cas on rend la valeur superieure, ou inferieure
	static Continuous GetLowerMeanValue(Continuous cValue1, Continuous cValue2);
	static Continuous GetUpperMeanValue(Continuous cValue1, Continuous cValue2);

	// Valeur moyenne entre deux valeurs lisible pour un humain
	// en minimisant le nombre de chiffre significatifs de facon a rester a
	// moins de 10% de la valeur moyenne exacte
	static Continuous GetHumanReadableLowerMeanValue(Continuous cValue1, Continuous cValue2);
	static Continuous GetHumanReadableUpperMeanValue(Continuous cValue1, Continuous cValue2);

	// Nombre de chiffres significatifs
	static int GetDigitNumber();

	// Acquisition par shell d'une valeur de Continuous
	static Continuous Acquire(const char* const sLabel, Continuous cDefaultValue);

	// Comparaison
	static int Compare(Continuous cValue1, Continuous cValue2);

	// Comparaison de deux valeurs de type indicateur, en principe entre 0 et 1
	// Permet d'avoir une resultat de comparaison robuste, selon la precision des Continuous
	static int CompareIndicatorValue(double dValue1, double dValue2);

	// Test des fonctionnalites
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Recherche de de la valeur la plus simple (avec le moins de chiffres significatifs)
	// comprise entre deux valeurs en entree
	static Continuous ComputeHumanReadableContinuous(double dLowerValue, double dUpperValue);

	// Implementation "standard des methodes de conversion, a titre de comparaison
	static const char* const StandardContinuousToString(Continuous cValue);
	static Continuous StandardStringToContinuous(const char* const sValue);
	static int StandardStringToContinuousError(const char* const sValue, Continuous& cValue);

	// Calcul de l'exposant d'un nombre structement positif
	static int ComputeExponent(Continuous cValue);

	// Tests de conversion dans les deux sens
	static void Conversion(const char* const sValue);
	static void TestConversion();

	// Conversion d'un double en chaine de carcteres, avec precsion maximale
	static const char* const MaxPrecisionDoubleToString(double dValue); // PORTAGE

	// Tests des conversion de Continuous vers chaine de caracteres
	static void CompareStringToContinuous(Continuous cValue, boolean bShow);
	static void TestPerformanceStringToContinuous(int nMaxLowerBaseValue, double dMaxUpperBaseValue,
						      int nMaxExponent, boolean bRefConversion, boolean bNewConversion);
	static void TestStringToContinuous();

	// Tests des conversion de Continuous vers chaine de caracteres
	static void CompareContinuousToString(Continuous cValue, boolean bShow);
	static void TestPerformanceContinuousToString(int nMaxLowerBaseValue, double dMaxUpperBaseValue,
						      int nMaxExponent, boolean bRefConversion, boolean bNewConversion);
	static void TestContinuousToString();
};

/////////////////////////////////////////////////
// Classe ContinuousObject
// Permet d'utiliser des containers de Continuous
class ContinuousObject : public Object
{
public:
	// Destructeur
	ContinuousObject();
	~ContinuousObject();

	// Valeur de l'objet
	void SetContinuous(Continuous cValue);
	Continuous GetContinuous() const;

	// Duplication
	ContinuousObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	Continuous cContinuous;
};

// Comparaison de deux ContinuousObject
int ContinuousObjectCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////
// Classe ContinuousVector
// Vecteur de Continuous, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class ContinuousVector : public Object
{
public:
	// Constructeur
	ContinuousVector();
	~ContinuousVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, Continuous cValue);
	Continuous GetAt(int nIndex) const;

	// Modification par adition d'une valeur a un element
	void UpgradeAt(int nIndex, Continuous cDeltaValue);

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(Continuous cValue);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const ContinuousVector* cvSource);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Duplication
	ContinuousVector* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(Continuous);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		Continuous* pValues;
		Continuous** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
};

///////////////////////////////////////////////////////
// Classe PLShared_Continuous
// Serialisation du type Continuous
class PLShared_Continuous : public PLSharedVariable
{
public:
	PLShared_Continuous();
	~PLShared_Continuous();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateur d'affectation entre continuous et continuous partages
	const PLShared_Continuous& operator=(const PLShared_Continuous& cValue);
	const PLShared_Continuous& operator=(Continuous cValue);
	operator Continuous() const;

	// Operateurs de calcul
	PLShared_Continuous& operator++();
	void operator++(int);
	PLShared_Continuous& operator--();
	void operator--(int);
	PLShared_Continuous& operator+=(Continuous);
	PLShared_Continuous& operator-=(Continuous);
	PLShared_Continuous& operator*=(Continuous);
	PLShared_Continuous& operator/=(Continuous);

protected:
	Continuous cContinuousValue;

	// Acces a la valeur
	void SetValue(Continuous cValue);
	Continuous GetValue() const;

	// Comparaison de valeur
	int Compare(Continuous dOtherValue) const;

	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator==(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator==(Continuous v1, const PLShared_Continuous& v2);
	friend boolean operator!=(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator!=(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator!=(Continuous v1, const PLShared_Continuous& v2);
	friend boolean operator<(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator<(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator<(Continuous v1, const PLShared_Continuous& v2);
	friend boolean operator>(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator>(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator>(Continuous v1, const PLShared_Continuous& v2);
	friend boolean operator<=(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator<=(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator<=(Continuous v1, const PLShared_Continuous& v2);
	friend boolean operator>=(const PLShared_Continuous& v1, const PLShared_Continuous& v2);
	friend boolean operator>=(const PLShared_Continuous& v1, Continuous v2);
	friend boolean operator>=(Continuous v1, const PLShared_Continuous& v2);
};

///////////////////////////////////////////////////////
// Classe PLShared_ContinuousVector
// Serialisation des vecteurs de Continuous
class PLShared_ContinuousVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ContinuousVector();
	~PLShared_ContinuousVector();

	// Acces au vecteur (appartient a l'appele, jamais NULL)
	void SetContinuousVector(ContinuousVector* cv);
	ContinuousVector* GetContinuousVector();
	const ContinuousVector* GetConstContinuousVector() const;

	// Acces aux elements du vecteur
	void SetAt(int nIndex, Continuous cValue);
	Continuous GetAt(int nIndex) const;
	void Add(Continuous cValue);

	// Taille du vecteur
	int GetSize() const;

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

// Methodes en inline

inline Continuous KWContinuous::StringToContinuous(const char* const sValue)
{
	Continuous cValue;
	StringToContinuousError(sValue, cValue);
	return cValue;
}

inline boolean KWContinuous::CheckError(int nValue)
{
	return (NoError <= nValue and nValue <= ErrorBadEndString);
}

inline boolean KWContinuous::ContinuousToInt(Continuous cValue, int& nValue)
{
	// Recherche de l'entier le plus proche
	nValue = int(floor(cValue + 0.5));

	// Ok si presque egale, avec tolerance par rapport au nombre max de digits GetDigitNumber
	if (fabs(cValue - nValue) < 1e-8)
		return true;
	else
	{
		nValue = 0;
		return false;
	}
}

inline boolean KWContinuous::IsInt(Continuous cValue)
{
	int nValue;
	return ContinuousToInt(cValue, nValue);
}

inline boolean KWContinuous::IsStringContinuous(const char* const sValue)
{
	Continuous cValue;
	return StringToContinuousError(sValue, cValue) <= ErrorPositiveOverflow;
}

inline void KWContinuous::PreprocessString(char* sValue)
{
	char* sContinuous;
	require(sValue != NULL);
	sContinuous = sValue;
	while (sContinuous[0] != '\0')
	{
		if (sContinuous[0] == ',')
			sContinuous[0] = '.';
		sContinuous++;
	}
}

inline Continuous KWContinuous::GetMinValue()
{
	assert(-DBL_MAX <= -1e100);
	return -1e100;
}

inline Continuous KWContinuous::GetMaxValue()
{
	assert(DBL_MAX >= 1e100);
	return 1e100;
}

inline Continuous KWContinuous::GetEpsilonValue()
{
	assert(DBL_MIN <= 1e-100);
	return 1e-100;
}

inline Continuous KWContinuous::GetMissingValue()
{
	return (Continuous)-HUGE_VAL;
}

inline Continuous KWContinuous::GetForbiddenValue()
{
	return (Continuous)HUGE_VAL;
}

inline Continuous KWContinuous::GetLowerMeanValue(Continuous cValue1, Continuous cValue2)
{
	Continuous cMeanValue;

	// Si l'une des valeurs est manquante, on renvoie la valeur manquante
	if (cValue1 == GetMissingValue() or cValue2 == GetMissingValue())
		return GetMissingValue();
	// Sinon, calcul de moyenne classique
	else
	{
		cMeanValue = (cValue1 + cValue2) / 2;

		// On convertit explicitement en Continuous pour s'assurer de la
		// coherence entre representation interne et representation externe
		// Cf. StringToContinuous()
		cMeanValue = DoubleToContinuous(cMeanValue);

		// Effet de bord quand la moyenne est egale a l'une des valeurs
		if (cMeanValue == cValue1 or cMeanValue == cValue2)
		{
			if (cValue1 <= cValue2)
				return cValue1;
			else
				return cValue2;
		}
		else
			return cMeanValue;
	}
}

inline Continuous KWContinuous::GetUpperMeanValue(Continuous cValue1, Continuous cValue2)
{
	Continuous cMeanValue;

	// Si les deux valeurs sont manquantes, on renvoie la valeur manquante
	if (cValue1 == GetMissingValue() and cValue2 == GetMissingValue())
		return GetMissingValue();
	// Si l'une des valeurs est manquante, on renvoie la valeur minimale
	else if (cValue1 == GetMissingValue() or cValue2 == GetMissingValue())
		return GetMinValue();
	// Sinon, calcul de moyenne classique
	else
	{
		cMeanValue = (cValue1 + cValue2) / 2;

		// On convertit explicitement en Continuous pour s'assurer de la
		// coherence entre representation interne et representation externe
		// Cf. StringToContinuous()
		cMeanValue = DoubleToContinuous(cMeanValue);

		// Effet de bord quand la moyenne est egale a l'une des valeurs
		if (cMeanValue == cValue1 or cMeanValue == cValue2)
		{
			if (cValue1 >= cValue2)
				return cValue1;
			else
				return cValue2;
		}
		else
			return cMeanValue;
	}
}

inline int KWContinuous::GetDigitNumber()
{
	assert(DBL_DIG >= 10);
	return 10;
}

inline int KWContinuous::Compare(Continuous cValue1, Continuous cValue2)
{
	return (cValue1 == cValue2 ? 0 : (cValue1 > cValue2 ? 1 : -1));
}

inline int KWContinuous::CompareIndicatorValue(double dValue1, double dValue2)
{
	// On ajoute 1 pour avoir une precision de mantisse limitee de facon absolue par rapprt au 0
	return Compare(DoubleToContinuous(1 + dValue1), DoubleToContinuous(1 + dValue2));
}

// Classe ContinuousObject

inline ContinuousObject::ContinuousObject()
{
	cContinuous = 0;
}
inline ContinuousObject::~ContinuousObject() {}

inline void ContinuousObject::SetContinuous(Continuous cValue)
{
	cContinuous = cValue;
}

inline Continuous ContinuousObject::GetContinuous() const
{
	return cContinuous;
}

inline void ContinuousObject::Write(ostream& ost) const
{
	ost << cContinuous;
}

// Classe ContinuousVector

inline ContinuousVector::ContinuousVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int ContinuousVector::GetSize() const
{
	return nSize;
}

inline void ContinuousVector::SetAt(int nIndex, Continuous cValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = cValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = cValue;
}

inline Continuous ContinuousVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void ContinuousVector::UpgradeAt(int nIndex, Continuous cDeltaValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] += cDeltaValue;
	else
		pData.pValueBlocks[nIndex / nBlockSize][nIndex % nBlockSize] += cDeltaValue;
}

inline void ContinuousVector::Add(Continuous cValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, cValue);
}

// Classe PLShared_Continuous

inline void PLShared_Continuous::SetValue(Continuous cValue)
{
	require(bIsWritable);
	cContinuousValue = cValue;
}
inline Continuous PLShared_Continuous::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return cContinuousValue;
}

inline void PLShared_Continuous::Clean()
{
	cContinuousValue = 0;
}

inline const PLShared_Continuous& PLShared_Continuous::operator=(const PLShared_Continuous& shared_ContinuousValue)
{
	SetValue(shared_ContinuousValue.GetValue());
	return *this;
}

inline const PLShared_Continuous& PLShared_Continuous::operator=(Continuous l)
{
	SetValue(l);
	return *this;
}

inline PLShared_Continuous::operator Continuous() const
{
	return GetValue();
}

inline PLShared_Continuous& PLShared_Continuous::operator++()
{
	SetValue(GetValue() + 1);
	return (*this);
}

inline void PLShared_Continuous::operator++(int)
{
	require(bIsWritable);
	++(*this);
}

inline PLShared_Continuous& PLShared_Continuous::operator--()
{
	SetValue(GetValue() - 1);
	return (*this);
}

inline void PLShared_Continuous::operator--(int)
{
	require(bIsWritable);
	--(*this);
}

inline PLShared_Continuous& PLShared_Continuous::operator+=(Continuous lvalue)
{
	SetValue(GetValue() + lvalue);
	return *this;
}

inline PLShared_Continuous& PLShared_Continuous::operator-=(Continuous lvalue)
{
	SetValue(GetValue() - lvalue);
	return *this;
}

inline PLShared_Continuous& PLShared_Continuous::operator*=(Continuous lvalue)
{
	SetValue(GetValue() * lvalue);
	return *this;
}

inline PLShared_Continuous& PLShared_Continuous::operator/=(Continuous lvalue)
{
	SetValue(GetValue() / lvalue);
	return *this;
}

inline int PLShared_Continuous::Compare(Continuous lOtherValue) const
{
	return KWContinuous::Compare(GetValue(), lOtherValue);
}

inline boolean operator==(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare == 0;
}

inline boolean operator==(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare == 0;
}

inline boolean operator==(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare == 0;
}

inline boolean operator!=(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare != 0;
}

inline boolean operator!=(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare != 0;
}

inline boolean operator!=(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare != 0;
}

inline boolean operator<(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare < 0;
}

inline boolean operator<(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare < 0;
}

inline boolean operator<(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare < 0;
}

inline boolean operator>(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare > 0;
}

inline boolean operator>(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare > 0;
}

inline boolean operator>(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare > 0;
}

inline boolean operator<=(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare <= 0;
}

inline boolean operator<=(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare <= 0;
}

inline boolean operator<=(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare <= 0;
}

inline boolean operator>=(const PLShared_Continuous& v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2.GetValue());
	return nCompare >= 0;
}

inline boolean operator>=(const PLShared_Continuous& v1, Continuous v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1.GetValue(), v2);
	return nCompare >= 0;
}

inline boolean operator>=(Continuous v1, const PLShared_Continuous& v2)
{
	int nCompare;
	nCompare = KWContinuous::Compare(v1, v2.GetValue());
	return nCompare >= 0;
}

// Classe PLShared_ContinuousVector

inline const ContinuousVector* PLShared_ContinuousVector::GetConstContinuousVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(ContinuousVector*, GetObject());
}

inline ContinuousVector* PLShared_ContinuousVector::GetContinuousVector()
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(ContinuousVector*, GetObject());
}

inline void PLShared_ContinuousVector::SetContinuousVector(ContinuousVector* cv)
{
	require(bIsWritable);
	SetObject(cv);
}

inline Continuous PLShared_ContinuousVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstContinuousVector()->GetAt(nIndex);
}

inline void PLShared_ContinuousVector::SetAt(int nIndex, Continuous dValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetContinuousVector()->SetAt(nIndex, dValue);
}

inline void PLShared_ContinuousVector::Add(Continuous dValue)
{
	require(bIsWritable);
	require(bIsDeclared);

	GetContinuousVector()->Add(dValue);
}

inline int PLShared_ContinuousVector::GetSize() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstContinuousVector()->GetSize();
}

inline Object* PLShared_ContinuousVector::Create() const
{
	return new ContinuousVector;
}
