// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWType.h"

///////////////////////////////////////////////////////////
// KWType

const ALString KWType::ToString(int nType)
{
	switch (nType)
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
	case SymbolValueBlock:
		return "CategoricalBlock";
	case ContinuousValueBlock:
		return "NumericalBlock";
	case ObjectArrayValueBlock:
		return "TableBlock";
	case None:
		return "None";
	case Unknown:
		return "Unknown";
	default:
		return "BadType";
	}
}

int KWType::ToType(const ALString& sType)
{
	if (sType == "Categorical")
		return Symbol;
	else if (sType == "Numerical")
		return Continuous;
	else if (sType == "Date")
		return Date;
	else if (sType == "Time")
		return Time;
	else if (sType == "Timestamp")
		return Timestamp;
	else if (sType == "Entity")
		return Object;
	else if (sType == "Table")
		return ObjectArray;
	else if (sType == "Structure")
		return Structure;
	else if (sType == "CategoricalBlock")
		return SymbolValueBlock;
	else if (sType == "NumericalBlock")
		return ContinuousValueBlock;
	else if (sType == "TableBlock")
		return ObjectArrayValueBlock;
	else if (sType == "None")
		return None;
	else
		return Unknown;
}

boolean KWType::IsPredictorType(int nType)
{
	return (nType == Continuous or nType == Symbol or nType == None);
}

const ALString KWType::GetPredictorLabel(int nType)
{
	switch (nType)
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

int KWType::ToPredictorType(const ALString& sPredictorType)
{
	if (sPredictorType == "Classifier")
		return Symbol;
	else if (sPredictorType == "Regressor")
		return Continuous;
	else if (sPredictorType == "Clusterer")
		return None;
	else
		return Unknown;
}

///////////////////////////////////////////////////////////
// KWValue

void KWValue::Test()
{
	int nType;
	KWValue value;
	KWValue values[KWType::Unknown];
	ObjectArray oaTest;

	// Verification de la taille de l'union
	cout << "Taille de l'union (differ in 32 and 64 bits)\n";
	cout << "\tKWValue     \t" << sizeof(KWValue) << "\n";
	cout << "\tNumerical  \t" << sizeof(Continuous) << "\n";
	cout << "\tCategorical      \t" << sizeof(Symbol) << "\n";
	cout << "\tDate      \t" << sizeof(Date) << "\n";
	cout << "\tTime      \t" << sizeof(Time) << "\n";
	cout << "\tTimestamp      \t" << sizeof(Timestamp) << "\n";
	cout << "\tEntity   \t" << sizeof(KWObject*) << "\n";
	cout << "\tTable\t" << sizeof(ObjectArray*) << "\n";
	cout << "\tStructure   \t" << sizeof(Object*) << "\n";
	cout << "\tCategoricalBlock  \t" << sizeof(KWSymbolValueBlock*) << "\n";
	cout << "\tNumericalBlock  \t" << sizeof(KWContinuousValueBlock*) << "\n";
	cout << "\tTableBlock  \t" << sizeof(KWObjectArrayValueBlock*) << "\n";

	// Transfert des valeurs avec un tableau
	cout << "\n\nDefault values in a table (pointer values differ from Windows to Linux)\n";
	for (nType = 0; nType < KWType::Unknown; nType++)
	{
		value.Init();
		values[nType] = value;
		cout << "\t" << KWType::ToString(nType) << "\t";
		if (nType == KWType::Symbol)
			cout << values[nType].GetSymbol() << endl;
		else if (nType == KWType::Continuous)
			cout << values[nType].GetContinuous() << endl;
		else if (nType == KWType::Date)
			cout << values[nType].GetDate() << endl;
		else if (nType == KWType::Time)
			cout << values[nType].GetTime() << endl;
		else if (nType == KWType::Timestamp)
			cout << values[nType].GetTimestamp() << endl;
		else if (nType == KWType::Object)
			cout << values[nType].GetObject() << endl;
		else if (nType == KWType::ObjectArray)
			cout << values[nType].GetObjectArray() << endl;
		else if (nType == KWType::Structure)
			cout << values[nType].GetStructure() << endl;
		else if (nType == KWType::SymbolValueBlock)
			cout << values[nType].GetSymbolValueBlock() << endl;
		else if (nType == KWType::ContinuousValueBlock)
			cout << values[nType].GetContinuousValueBlock() << endl;
		else if (nType == KWType::ObjectArrayValueBlock)
			cout << values[nType].GetObjectArrayValueBlock() << endl;
	}

	// Test avec accesseurs types
	cout << "\n\nTest with typed initialisation\n";
	//
	cout << "\tNumerical\t";
	value.SetContinuous(1);
	cout << value.GetContinuous() << endl;
	//
	cout << "\tCategorical\t";
	value.Init();
	value.SetSymbol(Symbol("S1"));
	cout << value.GetSymbol() << endl;
	value.SetSymbol("");
	//
	cout << "\tDate\t";
	value.Init();
	value.date.Init(2000, 1, 1);
	cout << value.GetDate() << endl;
	//
	cout << "\tTime\t";
	value.Init();
	value.time.Init(12, 30, 45);
	cout << value.GetTime() << endl;
	//
	cout << "\tDate\t";
	value.Init();
	value.timestamp.Init(2000, 1, 1, 12, 30, 45);
	cout << value.GetTimestamp() << endl;
}