// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Attribute.h"

Attribute::Attribute()
{
	nRank = 0;
	bInvisible = false;
}

Attribute::~Attribute() {}

void Attribute::CopyFrom(const Attribute* aSource)
{
	require(aSource != NULL);

	nRank = aSource->nRank;
	sName = aSource->sName;
	sType = aSource->sType;
	sStatus = aSource->sStatus;
	sStyle = aSource->sStyle;
	bInvisible = aSource->bInvisible;
	sLabel = aSource->sLabel;
}

Attribute* Attribute::Clone() const
{
	Attribute* aClone;

	aClone = new Attribute;
	aClone->CopyFrom(this);
	return aClone;
}

const ALString& Attribute::GetKey() const
{
	return GetName();
}

Attribute Attribute::managedObjectClass;

Attribute* Attribute::GetManagedObjectClass()
{
	return &managedObjectClass;
}

int Attribute::GetFieldNumber() const
{
	return LastField;
}

void Attribute::SetFieldAt(int nFieldIndex, const char* sValue)
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	switch (nFieldIndex)
	{
	case Rank:
		SetRank(StringToInt(sValue));
		break;
	case Name:
		SetName(sValue);
		break;
	case Type:
		SetType(sValue);
		break;
	case Status:
		SetStatus(sValue);
		break;
	case Style:
		SetStyle(sValue);
		break;
	case Invisible:
		SetInvisible(StringToBoolean(sValue));
		break;
	case Label:
		SetLabel(sValue);
		break;
	default:
		break;
	};
}

const char* Attribute::GetFieldAt(int nFieldIndex) const
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	switch (nFieldIndex)
	{
	case Rank:
		return IntToString(GetRank());
	case Name:
		return GetName();
	case Type:
		return GetType();
	case Status:
		return GetStatus();
	case Style:
		return GetStyle();
	case Invisible:
		return BooleanToString(GetInvisible());
	case Label:
		return GetLabel();
	default:
		return "";
	};
}

const ALString Attribute::GetFieldNameAt(int nFieldIndex) const
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	switch (nFieldIndex)
	{
	case Rank:
		return "Rank";
	case Name:
		return "Name";
	case Type:
		return "Type";
	case Status:
		return "Status";
	case Style:
		return "Style";
	case Invisible:
		return "Invisible";
	case Label:
		return "Label";
	default:
		return "";
	};
}

const ALString Attribute::GetFieldLabelAt(int nFieldIndex) const
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	switch (nFieldIndex)
	{
	case Rank:
		return "Rank";
	case Name:
		return "Name";
	case Type:
		return "Type";
	case Status:
		return "Status";
	case Style:
		return "Style";
	case Invisible:
		return "Invisible";
	case Label:
		return "Label";
	default:
		return "";
	};
}

const ALString Attribute::GetFieldStorageNameAt(int nFieldIndex) const
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	switch (nFieldIndex)
	{
	case Rank:
		return "Rank";
	case Name:
		return "Name";
	case Type:
		return "Type";
	case Status:
		return "Status";
	case Style:
		return "Style";
	case Invisible:
		return "Invisible";
	case Label:
		return "Label";
	default:
		return "";
	};
}

void Attribute::Write(ostream& ost) const
{
	ost << "Rank\t" << GetRank() << "\n";
	ost << "Name\t" << GetName() << "\n";
	ost << "Type\t" << GetType() << "\n";
	ost << "Status\t" << GetStatus() << "\n";
	ost << "Style\t" << GetStyle() << "\n";
	ost << "Invisible\t" << BooleanToString(GetInvisible()) << "\n";
	ost << "Label\t" << GetLabel() << "\n";
}

char Attribute::cFieldSeparator = '\t';

void Attribute::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

char Attribute::GetFieldSeparator()
{
	return cFieldSeparator;
}

const ALString Attribute::GetClassLabel() const
{
	return "Attribute";
}

////////////////////////////////////////////////////////////

ManagedObject* Attribute::CloneManagedObject() const
{
	return Clone();
}

IntVector Attribute::ivStoredFieldIndexes;

boolean Attribute::GetFieldStored(int nFieldIndex) const
{
	int i;

	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	// On force l'initialisation des index
	GetStoredFieldIndexes();

	// On recherche dans le tableau des index
	// des attributs stockes
	for (i = 0; i < ivStoredFieldIndexes.GetSize(); i++)
	{
		if (ivStoredFieldIndexes.GetAt(i) == nFieldIndex)
			return true;
	}
	return false;
}

IntVector* Attribute::GetStoredFieldIndexes() const
{
	// Initialisation si necessaire du tableau d'index
	if (ivStoredFieldIndexes.GetSize() == 0)
	{
		ivStoredFieldIndexes.Add(Rank);
		ivStoredFieldIndexes.Add(Name);
		ivStoredFieldIndexes.Add(Type);
		ivStoredFieldIndexes.Add(Status);
		ivStoredFieldIndexes.Add(Style);
		ivStoredFieldIndexes.Add(Invisible);
		ivStoredFieldIndexes.Add(Label);
	}
	return &ivStoredFieldIndexes;
}

CompareFunction Attribute::GetCompareKeyFunction() const
{
	return AttributeCompareKey;
}

////////////////////////////////////////////////////////////

char Attribute::GetSeparator() const
{
	return cFieldSeparator;
}

int AttributeCompareKey(const void* first, const void* second)
{
	Attribute* aFirst;
	Attribute* aSecond;
	int nResult;

	aFirst = cast(Attribute*, *(Object**)first);
	aSecond = cast(Attribute*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	if (nResult != 0)
		return nResult;
	return nResult;
}

int AttributeCompareRank(const void* first, const void* second)
{
	return cast(Attribute*, *(Object**)first)->GetRank() - cast(Attribute*, *(Object**)second)->GetRank();
}

const ALString AttributeGetRank(const Object* object)
{
	return IntToString(cast(Attribute*, object)->GetRank());
}

const ALString Attribute::GetObjectLabel() const
{
	return GetName();
}

void Attribute::NormalizeValues()
{
	// Normalisation de la casse des caracteres du Type
	sType.MakeLower();
	if (GetType() == "alstring")
		SetType("ALString");

	// Normalisation de la casse des caracteres du Status
	sStatus.MakeLower();
	if (GetStatus() == "standard")
		SetStatus("Standard");
	if (GetStatus() == "derived")
		SetStatus("Derived");
}

boolean Attribute::Check() const
{
	boolean bOk = true;
	int i;
	char c;

	// Test du nom
	if (GetName() == "")
	{
		bOk = false;
		AddError("The name must not be empty");
	}

	// Test de la validite lexicale du nom
	if (GetName() != "")
	{
		// Lettre acceptees: lettres, chiffres, '_'
		for (i = 0; i < GetName().GetLength(); i++)
		{
			c = GetName().GetAt(i);
			if (not isalnum(c) and c != '_')
			{
				bOk = false;
				AddError("The name must contain only alphanumeric characteres");
				break;
			}
		}

		// Ne commence par un chiffre
		if (isdigit(GetName().GetAt(0)))
		{
			bOk = false;
			AddError("The name must not start with a digit");
		}
	}

	// Test de la coherence si attribut libelle
	if (IsLabel())
	{
		if (GetStatus() != "")
		{
			bOk = false;
			AddError("The Status field muts be empty in case of a label line");
		}
	}

	// Test du type
	if (IsField())
	{
		if (GetType() != "int" and GetType() != "ALString" and GetType() != "float" and
		    GetType() != "double" and GetType() != "boolean" and GetType() != "char")
		{
			bOk = false;
			AddError("The type must be char, int, float, double, boolean ou ALString");
		}
	}

	// Test du status
	if (GetStatus() != "Standard" and GetStatus() != "" and GetStatus() != "Derived")
	{
		bOk = false;
		AddError("The status must be Standard or Derived");
	}

	// Test du style
	if (IsField() and GetStyle() != "")
	{
		if (GetType() == "int")
		{
			if (GetStyle() != "TextField" and GetStyle() != "ComboBox" and
			    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton" and
			    GetStyle() != "Spinner" and GetStyle() != "Slider")
			{
				bOk = false;
				AddError("The style of an attribute of type " + GetType() + " must be " +
					 "TextField, ComboBox, EditableComboBox, RadioButton, Slider");
			}
		}
		if (GetType() == "ALString")
		{
			if (GetStyle() != "TextField" and GetStyle() != "ComboBox" and
			    GetStyle() != "EditableComboBox" and GetStyle() != "HelpedComboBox" and
			    GetStyle() != "FileChooser" and GetStyle() != "DirectoryChooser" and
			    GetStyle() != "RadioButton" and GetStyle() != "Password" and GetStyle() != "TextArea" and
			    GetStyle() != "FormattedLabel" and GetStyle() != "SelectableLabel" and
			    GetStyle() != "UriLabel")
			{
				bOk = false;
				AddError("The style of an attribute of type " + GetType() + " must be " +
					 "TextField, ComboBox, EditableComboBox, HelpedComboBox, FileChooser, "
					 "DirectoryChooser,"
					 " Password, RadioButton, TextArea, FormattedLabel, SelectableLabel, UriLabel");
			}
		}
		if (GetType() == "float" or GetType() == "double")
		{
			if (GetStyle() != "TextField" and GetStyle() != "ComboBox" and
			    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton" and
			    GetStyle() != "Spinner")
			{
				bOk = false;
				AddError("The style of an attribute of type " + GetType() + " must be " +
					 "TextField, ComboBox, EditableComboBox, RadioButton");
			}
		}
		if (GetType() == "boolean")
		{
			if (GetStyle() != "TextField" and GetStyle() != "CheckBox" and GetStyle() != "ComboBox" and
			    GetStyle() != "RadioButton")
			{
				bOk = false;
				AddError("The style of an attribute of type " + GetType() + " must be " +
					 "TextField, CheckBox, ComboBox, RadioButton");
			}
		}
		if (GetType() == "char")
		{
			if (GetStyle() != "TextField" and GetStyle() != "ComboBox" and
			    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton")
			{
				bOk = false;
				AddError("The style of an attribute of type " + GetType() + " must be " +
					 "TextField, ComboBox, EditableComboBox, RadioButton");
			}
		}
	}

	return bOk;
}

void Attribute::ComputeDefaultValues() {}

const ALString Attribute::GetPrefix() const
{
	if (sType == "int")
		return "n";
	else if (sType == "ALString")
		return "s";
	else if (sType == "float")
		return "f";
	else if (sType == "double")
		return "d";
	else if (sType == "boolean")
		return "b";
	else if (sType == "char")
		return "c";
	else if (sType.GetLength() > 0 and sType[sType.GetLength() - 1] == '*')
		return "p";
	else
		return "a";
}

const ALString Attribute::GetFieldType() const
{
	if (sType == "int")
		return "Int";
	else if (sType == "ALString")
		return "String";
	else if (sType == "float")
		return "Double";
	else if (sType == "double")
		return "Double";
	else if (sType == "boolean")
		return "Boolean";
	else if (sType == "char")
		return "Char";
	else if (sType.GetLength() > 0 and sType[sType.GetLength() - 1] == '*')
		return "p";
	else
		return "a";
}

const ALString Attribute::GetDefaultValue() const
{
	if (sType == "int")
		return "0";
	else if (sType == "ALString")
		return "\"\"";
	else if (sType == "float")
		return "0";
	else if (sType == "double")
		return "0";
	else if (sType == "boolean")
		return "false";
	else if (sType == "char")
		return "' '";
	else if (sType.GetLength() > 0 and sType[sType.GetLength() - 1] == '*')
		return "NULL";
	else
		return "NULL";
}

boolean Attribute::IsConstructorInit() const
{
	if (sType == "int")
		return true;
	else if (sType == "ALString")
		return false;
	else if (sType == "float")
		return true;
	else if (sType == "double")
		return true;
	else if (sType == "boolean")
		return true;
	else if (sType == "char")
		return true;
	else if (sType.GetLength() > 0 and sType[sType.GetLength() - 1] == '*')
		return true;
	else
		return false;
}

const ALString Attribute::GetMethodDecl() const
{
	if (sType == "ALString")
		return "const ALString&";
	else
		return sType;
}

const ALString Attribute::GetDerivedGetterType() const
{
	return sType;
}

const ALString Attribute::GetCharVarToType(const ALString& sVarName) const
{
	if (sType == "int")
		return "StringToInt(" + sVarName + ")";
	else if (sType == "ALString")
		return sVarName;
	else if (sType == "float")
		return "StringToFloat(" + sVarName + ")";
	else if (sType == "double")
		return "StringToDouble(" + sVarName + ")";
	else if (sType == "boolean")
		return "StringToBoolean(" + sVarName + ")";
	else if (sType == "char")
		return "StringToChar(" + sVarName + ")";
	else
		// portage unix ??  au lieu de ??? pour eviter le trigraphe
		return "?? (" + sVarName + ")";
}

const ALString Attribute::GetTypeVarToString(const ALString& sVarName) const
{
	if (sType == "int")
		return "IntToString(" + sVarName + ")";
	else if (sType == "ALString")
		return sVarName;
	else if (sType == "float")
		return "FloatToString(" + sVarName + ")";
	else if (sType == "double")
		return "DoubleToString(" + sVarName + ")";
	else if (sType == "boolean")
		return "BooleanToString(" + sVarName + ")";
	else if (sType == "char")
		return "CharToString(" + sVarName + ")";
	else
		// portage Unix ?? au lieu de ??? pour eviter le trigraphe
		return "?? (" + sVarName + ")";
}

const ALString Attribute::GetTypeVarToStream(const ALString& sVarName) const
{
	if (sType == "boolean")
		return "BooleanToString(" + sVarName + ")";
	else
		return sVarName;
}

const ALString Attribute::GetTypeVarToField(const ALString& sVarName) const
{
	return sVarName;
}

const ALString Attribute::GetFieldToTypeVar(const ALString& sVarName) const
{
	return sVarName;
}

const ALString Attribute::GetTypeVarComparison(const ALString& sFirstElem, const ALString& sSecondElem) const
{
	if (sType == "ALString")
		return sFirstElem + ".Compare( " + sSecondElem + ")";
	else if (sType == "int")
		return sFirstElem + " - " + sSecondElem;
	else if (sType == "float")
		return "(" + sFirstElem + " == " + sSecondElem + " ? 0 : (" + sFirstElem + " < " + sSecondElem +
		       " ? -1 : 1))";
	else if (sType == "double")
		return "(" + sFirstElem + " == " + sSecondElem + " ? 0 : (" + sFirstElem + " < " + sSecondElem +
		       " ? -1 : 1))";
	else if (sType == "boolean")
		return "(" + sFirstElem + " ? 1 : 0) - (" + sSecondElem + " ? 1 : 0)";
	else if (sType == "char")
		return sFirstElem + " - " + sSecondElem;
	else
		// portage unix ?? au lieu de ??? pour eviter le trigraphe
		return sFirstElem + " ?? " + sSecondElem;
}