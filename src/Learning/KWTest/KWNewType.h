// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Timer.h"

///////////////////////////////////////////////////////////
// Definition des types d'attributs d'une KWClass
class KWNewType : public SystemObject
{
public:
	KWNewType();
	// Liste des types disponibles
	enum
	{
		Symbol,      // Type symbol (valeur de type Symbol)
		Continuous,  // Type continu (valeur de type Continous (cf. KWContinuous))
		Date,        // Type date (valeur de type Date (cf. KWDate))
		Time,        // Type time (valeur de type Time (cf. KWTime))
		Timestamp,   // Type timestamp (valeur de type Timestamp (cf. KWTimestamp))
		Object,      // Type object (objet de type KWObject, gere par une KWClass)
		ObjectArray, // Type tableau d'objects (tableau d'objets de type KWObject)
		Structure,   // Type algorithmique (objet de d'une classe specialisee, heritant de Object)
		None,        // Type absent deliberement, pour le non supervise
		Unknown      // Type inconnu (non valide)
	};

	// Operateurs d'affectation
	KWNewType& operator=(const char cValue);

	// Operateurs de comparaison
	boolean operator==(const KWNewType value) const;
	boolean operator!=(const KWNewType value) const;
	boolean operator==(const char value) const;
	boolean operator!=(const char value) const;

	// Verification de validite d'un type
	boolean Check() const;

	// Conversion d'un type vers une chaine
	const ALString ToString() const;

	// Conversion d'une chaine vers un type
	void FromString(const ALString& sType);

	// Verification si un type est simple (Continuous ou Symbol)
	boolean IsSimple() const;

	// Verification si un type est complexe (Date, Time ou Timestamp)
	boolean IsComplex() const;

	// Verification si un type est stocke (Simple ou Complex)
	boolean IsStored() const;

	// Verification si un type correspond a une relation (Object ou ObjectArray)
	boolean IsRelation() const;

	// Verification si un type est de donneee (Stored ou Relation)
	boolean IsData() const;

	// Indique si le type est un type de predicteur: (Continuous, Symbol ou None)
	boolean IsPredictorType() const;

	// Libelle associe au predicteur pour un type de variable cible: classifier, regressor ou clusterer
	const ALString GetPredictorLabel() const;

	// Recherche du type pour un libelle de predicteur
	void FromPredictorLabel(const ALString& sPredictorLabel);

	// Test des fonctionnalites
	static void Test();

	/////////////////////////////////
	///// Implementation
protected:
	// Memorisation du type au moyen d'un caractere
	char cType;
};

////////////////////////////////////////////////////////
// Methodes en inline

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const KWNewType& value)
{
	ost << value.ToString();
	return ost;
}

inline KWNewType::KWNewType()
{
	cType = Unknown;
}

inline KWNewType& KWNewType::operator=(const char cValue)
{
	require(0 <= cType and cType <= Unknown);
	cType = cValue;
	return (*this);
}

inline boolean KWNewType::operator==(const KWNewType value) const
{
	return (cType == value.cType);
}

inline boolean KWNewType::operator!=(const KWNewType value) const
{
	return (cType != value.cType);
}

inline boolean KWNewType::operator==(const char cValue) const
{
	return (cType == cValue);
}

inline boolean KWNewType::operator!=(const char cValue) const
{
	return (cType != cValue);
}

inline boolean KWNewType::Check() const
{
	return (0 <= cType and cType < None);
}

inline boolean KWNewType::IsSimple() const
{
	return (cType == Continuous or cType == Symbol);
}

inline boolean KWNewType::IsComplex() const
{
	return (cType >= Date and cType <= Timestamp);
}

inline boolean KWNewType::IsStored() const
{
	return (cType >= 0 and cType <= Timestamp);
}

inline boolean KWNewType::IsRelation() const
{
	return (cType == Object or cType == ObjectArray);
}

inline boolean KWNewType::IsData() const
{
	return (cType >= 0 and cType <= ObjectArray);
}
