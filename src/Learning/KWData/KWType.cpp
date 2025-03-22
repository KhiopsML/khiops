// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	case TimestampTZ:
		return "TimestampTZ";
	case Text:
		return "Text";
	case TextList:
		return "TextList";
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
	case VarPart:
		return "VarPart";
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
	else if (sType == "TimestampTZ")
		return TimestampTZ;
	else if (sType == "Text")
		return Text;
	else if (sType == "TextList")
		return TextList;
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
	else if (sType == "VarPart")
		return VarPart;
	else if (sType == "None")
		return None;
	else
		return Unknown;
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
	Timestamp tsValue;

	// Verification de la taille de l'union
	cout << "Taille de l'union (differ in 32 and 64 bits)\n";
	cout << "\tKWValue     \t" << sizeof(KWValue) << "\n";
	cout << "\tNumerical  \t" << sizeof(Continuous) << "\n";
	cout << "\tCategorical      \t" << sizeof(Symbol) << "\n";
	cout << "\tDate      \t" << sizeof(Date) << "\n";
	cout << "\tTime      \t" << sizeof(Time) << "\n";
	cout << "\tTimestamp      \t" << sizeof(Timestamp) << "\n";
	cout << "\tTimestampTZ      \t" << sizeof(TimestampTZ) << "\n";
	cout << "\tText      \t" << sizeof(Symbol) << "\n";
	cout << "\tTextList      \t" << sizeof(SymbolVector*) << "\n";
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
		switch (nType)
		{
		case KWType::Symbol:
			cout << values[nType].GetSymbol() << endl;
			break;
		case KWType::Continuous:
			cout << values[nType].GetContinuous() << endl;
			break;
		case KWType::Date:
			cout << values[nType].GetDate() << endl;
			break;
		case KWType::Time:
			cout << values[nType].GetTime() << endl;
			break;
		case KWType::Timestamp:
			cout << values[nType].GetTimestamp() << endl;
			break;
		case KWType::TimestampTZ:
			cout << values[nType].GetTimestampTZ() << endl;
			break;
		case KWType::Text:
			cout << values[nType].GetText() << endl;
			break;
		case KWType::TextList:
			cout << values[nType].GetTextList() << endl;
			break;
		case KWType::Object:
			cout << values[nType].GetObject() << endl;
			break;
		case KWType::ObjectArray:
			cout << values[nType].GetObjectArray() << endl;
			break;
		case KWType::Structure:
			cout << values[nType].GetStructure() << endl;
			break;
		case KWType::SymbolValueBlock:
			cout << values[nType].GetSymbolValueBlock() << endl;
			break;
		case KWType::ContinuousValueBlock:
			cout << values[nType].GetContinuousValueBlock() << endl;
			break;
		case KWType::ObjectArrayValueBlock:
			cout << values[nType].GetObjectArrayValueBlock() << endl;
			break;
		}
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
	cout << "\tTimestamp\t";
	value.Init();
	value.timestamp.Init(2000, 1, 1, 12, 30, 45);
	cout << value.GetTimestamp() << endl;
	//
	cout << "\tTimestampTZ\t";
	value.Init();
	tsValue.Init(2000, 1, 1, 12, 30, 45);
	value.timestampTZ.Init(tsValue, 13 * 60);
	cout << value.GetTimestampTZ() << endl;
	//
	cout << "\tText\t";
	value.Init();
	value.SetText(Symbol("Hello world"));
	cout << value.GetText() << endl;
	value.SetText("");
}
