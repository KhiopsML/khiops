// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClass.h"
#include "KWNewType.h"
#include "KWTupleTable.h"
#include "KWDiscretizerMODL.h"
#include "KDConstructionDomain.h"
#include "KWDataTableDriverTextFile.h"
#include "KWDataGrid.h"
#include "KWLearningReport.h"
#include "KWProbabilityTable.h"
#include "KWSortBuckets.h"
#include "KWAttributeSpec.h"

// Etude sur la taille memoire des classes de Learning
void SizeofStudyTest();

///////////////////////////////////////////////////////////
// Definition des types d'attributs d'une KWClass
class OptType : public SystemObject
{
public:
	OptType();
	OptType operator=(const OptType type);
	OptType operator=(const char cValue);

	// Liste des types disponibles
	enum
	{
		Symbol,                // Type symbol (valeur de type Symbol)
		Continuous,            // Type continu (valeur de type Continous (cf. KWContinuous))
		Date,                  // Type date (valeur de type Date (cf. KWDate))
		Time,                  // Type time (valeur de type Time (cf. KWTime))
		Timestamp,             // Type timestamp (valeur de type Timestamp (cf. KWTimestamp))
		Object,                // Type object (objet de type KWObject, gere par une KWClass)
		ObjectArray,           // Type tableau d'objects (tableau d'objets de type KWObject)
		Structure,             // Type algorithmique (objet de d'une classe specialisee, heritant de Object)
		SymbolValueBlock,      // Bloc sparse de valeurs
		ContinuousValueBlock,  // Bloc sparse de valeurs Continuous
		ObjectArrayValueBlock, // Bloc sparse de valeurs ObjectArray
		None,                  // Type absent deliberement, pour le non supervise
		Unknown                // Type inconnu (non valide)
	};
	char cType;

	// Test
	static void Test();
};

// Redefinition de l'operateur <<, pour les stream
inline ostream& operator<<(ostream& ost, const OptType& type)
{
	ost << KWType::ToString(type.cType);
	return ost;
}

inline OptType::OptType()
{
	cType = Unknown;
}

inline OptType OptType::operator=(const OptType type)
{
	cType = type.cType;
	return *this;
}

inline OptType OptType::operator=(const char cValue)
{
	cType = cValue;
	return *this;
}
