// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWNewType.h"

///////////////////////////////////////////////////////////
// KWNewType

const ALString KWNewType::ToString() const
{
	switch (cType)
	{
	case Symbol:
		return "Categorical";
	case Continuous:
		return "Numerical";
	case Date:
		return "Date";
	case Time:
		return "Time";
	case Timestamp:
		return "Timestamp";
	case Object:
		return "Entity";
	case ObjectArray:
		return "Table";
	case Structure:
		return "Structure";
	case None:
		return "None";
	case Unknown:
		return "Unknown";
	default:
		return "BadType";
	}
}

void KWNewType::FromString(const ALString& sType)
{
	if (sType == "Categorical")
		cType = Symbol;
	else if (sType == "Numerical")
		cType = Continuous;
	else if (sType == "Date")
		cType = Date;
	else if (sType == "Time")
		cType = Time;
	else if (sType == "Timestamp")
		cType = Timestamp;
	else if (sType == "Entity")
		cType = Object;
	else if (sType == "Table")
		cType = ObjectArray;
	else if (sType == "Structure")
		cType = Structure;
	else if (sType == "None")
		cType = None;
	else
		cType = Unknown;
}

boolean KWNewType::IsPredictorType() const
{
	return (cType == Continuous or cType == Symbol or cType == None);
}

const ALString KWNewType::GetPredictorLabel() const
{
	switch (cType)
	{
	case Continuous:
		return "Regressor";
	case Symbol:
		return "Classifier";
	case Object:
		return "";
	case ObjectArray:
		return "";
	case Structure:
		return "";
	case None:
		return "Clusterer";
	case Unknown:
		return "?";
	default:
		return "!";
	}
}

void KWNewType::FromPredictorLabel(const ALString& sPredictorLabel)
{
	if (sPredictorLabel == "Classifier")
		cType = Symbol;
	else if (sPredictorLabel == "Regressor")
		cType = Continuous;
	else if (sPredictorLabel == "Clusterer")
		cType = None;
	else
		cType = Unknown;
}

void KWNewType::Test()
{
	KWNewType type;
	KWNewType type2;
	boolean bCompare;
	int nType;
	char cType;
	Timer timer;
	int nMax = 1000000000;
	int i;

	// Taille
	cout << "sizeof(KWNewType): " << sizeof(KWNewType) << endl;

	// Affectations autorisees
	nType = KWNewType::Symbol;
	cType = KWNewType::Symbol;
	type = KWNewType::Symbol;
	type = 5;
	type2 = type;

	// Affectations interdites
	// type = nType;
	// type = 1000;
	// nType = type;
	// cType = type;

	// Comparaison
	bCompare = type == type2;
	bCompare = type != type2;
	bCompare = type == KWNewType::Symbol;

	// Performance sur des affectations d'entiers
	debug(nMax /= 10);
	cout << "Performance sur des affectations d'entiers: " << endl;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
	{
		if (i % 2 == 0)
			nType = KWNewType::Symbol;
		else
			nType = KWNewType::Continuous;
	}
	timer.Stop();
	type = (char)nType;
	cout << "SYSTIME\t" << type << " " << timer.GetElapsedTime() << endl;

	// Performance sur des affectations de caracteres
	debug(nMax /= 10);
	cout << "Performance sur des affectations de caracteres: " << endl;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
	{
		if (i % 2 == 0)
			cType = KWNewType::Symbol;
		else
			cType = KWNewType::Continuous;
	}
	timer.Stop();
	type = cType;
	cout << "SYSTIME\t" << type << " " << timer.GetElapsedTime() << endl;

	// Performance sur des affectations de types
	cout << "Performance sur des affectations de type: " << flush;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
	{
		if (i % 2 == 0)
			type = KWNewType::Symbol;
		else
			type = KWNewType::Continuous;
	}
	timer.Stop();
	cout << "SYSTIME\t" << type << " " << timer.GetElapsedTime() << endl;

	// Test de tous les types
	cout << "Type\tLabel\tIsPredictor\tPredictorLabel\n";
	for (nType = 0; nType <= Unknown; nType++)
	{
		cType = (char)nType;
		type = cType;
		cout << nType << "\t";
		cout << type << "\t";
		cout << type.IsPredictorType() << "\t";
		cout << type.GetPredictorLabel() << "\n";
	}
}