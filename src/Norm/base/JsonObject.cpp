// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JsonObject.h"

/////////////////////////////////////////////
// Classe JsonObject

JsonObject::JsonObject() {}

JsonObject::~JsonObject()
{
	oaMembers.DeleteAll();
}

int JsonObject::GetType()
{
	return ObjectValue;
}

void JsonObject::AddMember(JsonMember* member)
{
	require(member != NULL);
	require(LookupMember(member->GetKey()) == NULL);

	oaMembers.Add(member);
	odMembers.SetAt(member->GetKey(), member);
}

void JsonObject::RemoveAll()
{
	oaMembers.RemoveAll();
	odMembers.RemoveAll();
}

void JsonObject::DeleteAll()
{
	oaMembers.DeleteAll();
	odMembers.RemoveAll();
}

int JsonObject::GetMemberNumber() const
{
	return oaMembers.GetSize();
}

JsonMember* JsonObject::GetMemberAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetMemberNumber());
	return cast(JsonMember*, oaMembers.GetAt(nIndex));
}

JsonMember* JsonObject::LookupMember(const ALString& sKey) const
{
	assert(odMembers.Lookup(sKey) == NULL or cast(JsonMember*, odMembers.Lookup(sKey))->GetKey() == sKey);
	return cast(JsonMember*, odMembers.Lookup(sKey));
}

boolean JsonObject::WriteFile(const ALString& sFileName) const
{
	boolean bOk = true;
	ALString sLocalFileName;
	fstream fstJson;

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);

	// Ouverture du fichier
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, fstJson);

	// Ecriture
	if (bOk)
		Write(fstJson);

	// Fermeture du fichier
	if (bOk)
		bOk = FileService::CloseOutputFile(sLocalFileName, fstJson);

	// Copie vers HDFS si necessaire
	PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);
	return bOk;
}

void JsonObject::Write(ostream& ost) const
{
	return WriteIndent(ost, 0);
}

void JsonObject::WriteIndent(ostream& ost, int nIndentLevel) const
{
	ALString sIndent('\t', nIndentLevel);
	int i;

	// Cas particulier d'un objet vide
	if (GetMemberNumber() == 0)
		ost << "{}";
	// Cas general
	else
	{
		// Par d'indentation au depart, pour le cas ou on fait partie d'un membre d'objet ave sa cle
		ost << '{' << '\n';

		// Chaque membre est indente au niveau superieur
		for (i = 0; i < GetMemberNumber(); i++)
		{
			GetMemberAt(i)->WriteIndent(ost, nIndentLevel + 1);
			if (i < GetMemberNumber() - 1)
				ost << ',';
			ost << '\n';
		}

		// Indentation a la fin
		ost << sIndent << '}';
	}
}

const ALString JsonObject::JsonObject::GetClassLabel() const
{
	return "json object";
}

const ALString JsonObject::GetObjectLabel() const
{
	return "";
}

void JsonObject::TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName)
{
	JsonObject testJsonObject;

	require(sReadFileName != "");
	require(sWriteFileName != "");

	testJsonObject.ReadFile(sReadFileName);
	testJsonObject.WriteFile(sWriteFileName);
}

void JsonObject::Test()
{
	TestReadWrite("C:\\temp\\Datasets\\Iris\\AnalysisResults.khj",
		      "C:\\temp\\Datasets\\Iris\\O_AnalysisResults.khj");
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonArray

JsonArray::JsonArray() {}

JsonArray::~JsonArray()
{
	oaValues.DeleteAll();
}

int JsonArray::GetType()
{
	return ArrayValue;
}

void JsonArray::AddValue(JsonValue* value)
{
	require(value != NULL);

	oaValues.Add(value);
}

void JsonArray::RemoveAll()
{
	oaValues.RemoveAll();
}

void JsonArray::DeleteAll()
{
	oaValues.DeleteAll();
}

int JsonArray::GetValueNumber() const
{
	return oaValues.GetSize();
}

JsonValue* JsonArray::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetValueNumber());
	return cast(JsonValue*, oaValues.GetAt(nIndex));
}

void JsonArray::Write(ostream& ost) const
{
	return WriteIndent(ost, 0);
}

void JsonArray::WriteIndent(ostream& ost, int nIndentLevel) const
{
	ALString sIndent('\t', nIndentLevel);
	JsonValue* jsonValue;
	int i;

	// Cas particulier d'un objet vide
	if (GetValueNumber() == 0)
		ost << "[]";
	// Cas general
	else
	{
		// Par d'indentation au depart, pour le cas ou on fait partie d'un membre d'objet avec sa cle
		ost << '[' << '\n';

		// Chaque membre est indente au niveau superieur
		for (i = 0; i < GetValueNumber(); i++)
		{
			jsonValue = GetValueAt(i);

			// Indentation prealable
			ost << sIndent << "\t";

			// Ecriture de la valeur en indente dans les cas des type objet ou tableau
			if (jsonValue->GetType() == ObjectValue)
				cast(JsonObject*, jsonValue)->WriteIndent(ost, nIndentLevel + 1);
			else if (jsonValue->GetType() == ArrayValue)
				cast(JsonArray*, jsonValue)->WriteIndent(ost, nIndentLevel + 1);
			// Sinon, ecriture directe
			else
				jsonValue->Write(ost);
			if (i < GetValueNumber() - 1)
				ost << ',';
			ost << '\n';
		}

		// Indentation a la fin
		ost << sIndent << ']';
	}
}

const ALString JsonArray::GetClassLabel() const
{
	return "json array";
}

const ALString JsonArray::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonString

JsonString::JsonString() {}

JsonString::~JsonString() {}

int JsonString::GetType()
{
	return StringValue;
}

void JsonString::SetString(const ALString& sValue)
{
	sStringValue = sValue;
}

const ALString& JsonString::GetString() const
{
	return sStringValue;
}

void JsonString::Write(ostream& ost) const
{
	ALString sJsonStringValue;

	// Encodage de la chaine C au format json
	TextService::CToJsonString(sStringValue, sJsonStringValue);
	ost << '"';
	ost << sJsonStringValue;
	ost << '"';
}

const ALString JsonString::GetClassLabel() const
{
	return "json string";
}

const ALString JsonString::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonNumber

JsonNumber::JsonNumber()
{
	dNumberValue = 0;
}

JsonNumber::~JsonNumber() {}

int JsonNumber::GetType()
{
	return NumberValue;
}

void JsonNumber::SetNumber(double dValue)
{
	dNumberValue = dValue;
}

double JsonNumber::GetNumber() const
{
	return dNumberValue;
}

void JsonNumber::Write(ostream& ost) const
{
	ost << std::setprecision(10) << dNumberValue;
}

const ALString JsonNumber::GetClassLabel() const
{
	return "json number";
}

const ALString JsonNumber::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonBoolean

JsonBoolean::JsonBoolean()
{
	bBooleanValue = false;
}

JsonBoolean::~JsonBoolean() {}

int JsonBoolean::GetType()
{
	return BooleanValue;
}

void JsonBoolean::SetBoolean(boolean bValue)
{
	bBooleanValue = bValue;
}

boolean JsonBoolean::GetBoolean() const
{
	return bBooleanValue;
}

void JsonBoolean::Write(ostream& ost) const
{
	if (bBooleanValue)
		ost << "true";
	else
		ost << "false";
}

const ALString JsonBoolean::GetClassLabel() const
{
	return "json boolean";
}

const ALString JsonBoolean::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonNull

JsonNull::JsonNull() {}

JsonNull::~JsonNull() {}

int JsonNull::GetType()
{
	return NullValue;
}

void JsonNull::Write(ostream& ost) const
{
	ost << "null";
}

const ALString JsonNull::GetClassLabel() const
{
	return "json null";
}

const ALString JsonNull::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JsonMember

JsonMember::JsonMember()
{
	jsonValue = NULL;
}

JsonMember::~JsonMember()
{
	if (jsonValue != NULL)
		delete jsonValue;
}

void JsonMember::SetKey(const ALString& sValue)
{
	sKey = sValue;
}

const ALString& JsonMember::GetKey()
{
	return sKey;
}

void JsonMember::SetValue(JsonValue* value)
{
	require(jsonValue == NULL);
	require(value != NULL);
	jsonValue = value;
}

JsonValue* JsonMember::GetValue() const
{
	return jsonValue;
}

int JsonMember::GetValueType() const
{
	if (jsonValue == NULL)
		return JsonValue::None;
	else
		return jsonValue->GetType();
}

JsonObject* JsonMember::GetObjectValue() const
{
	require(GetValueType() == JsonValue::ObjectValue);
	return cast(JsonObject*, GetValue());
}

JsonArray* JsonMember::GetArrayValue() const
{
	require(GetValueType() == JsonValue::ArrayValue);
	return cast(JsonArray*, GetValue());
}

JsonString* JsonMember::GetStringValue() const
{
	require(GetValueType() == JsonValue::StringValue);
	return cast(JsonString*, GetValue());
}

JsonNumber* JsonMember::GetNumberValue() const
{
	require(GetValueType() == JsonValue::NumberValue);
	return cast(JsonNumber*, GetValue());
}

JsonBoolean* JsonMember::GetBooleanValue() const
{
	require(GetValueType() == JsonValue::BooleanValue);
	return cast(JsonBoolean*, GetValue());
}

JsonNull* JsonMember::GetNullValue() const
{
	require(GetValueType() == JsonValue::NullValue);
	return cast(JsonNull*, GetValue());
}

void JsonMember::Write(ostream& ost) const
{
	return WriteIndent(ost, 0);
}

void JsonMember::WriteIndent(ostream& ost, int nIndentLevel) const
{
	ALString sIndent('\t', nIndentLevel);
	ALString sJsonKey;

	// Encodage de la chaine C au format json
	TextService::CToJsonString(sKey, sJsonKey);
	ost << sIndent;
	ost << '"';
	ost << sJsonKey;
	ost << "\": ";

	// Ecriture de la valeur en indente dans les cas des type objet ou tableau
	if (GetValueType() == JsonValue::ObjectValue)
		GetObjectValue()->WriteIndent(ost, nIndentLevel);
	else if (GetValueType() == JsonValue::ArrayValue)
		GetArrayValue()->WriteIndent(ost, nIndentLevel);
	// Sinon, ecriture directe
	else
		GetValue()->Write(ost);
}

const ALString JsonMember::GetClassLabel() const
{
	return "json member";
}

const ALString JsonMember::GetObjectLabel() const
{
	return sKey;
}
