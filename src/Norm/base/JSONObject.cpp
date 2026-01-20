// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONObject.h"

/////////////////////////////////////////////
// Classe JSONValue

JSONObject* JSONValue::GetObjectValue() const
{
	require(GetType() == JSONValue::ObjectValue);
	return cast(JSONObject*, this);
}

JSONArray* JSONValue::GetArrayValue() const
{
	require(GetType() == JSONValue::ArrayValue);
	return cast(JSONArray*, this);
}

JSONString* JSONValue::GetStringValue() const
{
	require(GetType() == JSONValue::StringValue);
	return cast(JSONString*, this);
}

JSONNumber* JSONValue::GetNumberValue() const
{
	require(GetType() == JSONValue::NumberValue);
	return cast(JSONNumber*, this);
}

JSONBoolean* JSONValue::GetBooleanValue() const
{
	require(GetType() == JSONValue::BooleanValue);
	return cast(JSONBoolean*, this);
}

JSONNull* JSONValue::GetNullValue() const
{
	require(GetType() == JSONValue::NullValue);
	return cast(JSONNull*, this);
}

void JSONValue::Write(ostream& ost) const
{
	WriteIndent(ost, 0, true);
}

void JSONValue::WriteCompact(ostream& ost) const
{
	WriteIndent(ost, 0, false);
}

const ALString JSONValue::BuildCompactJsonValue() const
{
	ALString sJsonValue;
	ostringstream osString;

	// Ecriture basique de la valeur json de facon compacte
	osString.str("");
	WriteCompact(osString);
	sJsonValue = osString.str().c_str();
	return sJsonValue;
}

const ALString JSONValue::BuildDisplayedJsonValue() const
{
	const int nMaxLength = 35;
	ALString sJsonValue;

	// Troncature si longueur depassee
	sJsonValue = BuildCompactJsonValue();
	if (sJsonValue.GetLength() > nMaxLength)
		return sJsonValue.Left(nMaxLength) + "...";
	else
		return sJsonValue;
}

const ALString JSONValue::GetClassLabel() const
{
	return "json " + TypeToString();
}

/////////////////////////////////////////////
// Classe JSONObject

JSONObject::JSONObject() {}

JSONObject::~JSONObject()
{
	oaMembers.DeleteAll();
}

int JSONObject::GetType() const
{
	return ObjectValue;
}

void JSONObject::AddMember(JSONMember* member)
{
	require(member != NULL);
	require(LookupMember(member->GetKey()) == NULL);

	oaMembers.Add(member);
	odMembers.SetAt(member->GetKey(), member);
}

void JSONObject::RemoveAll()
{
	oaMembers.RemoveAll();
	odMembers.RemoveAll();
}

void JSONObject::DeleteAll()
{
	oaMembers.DeleteAll();
	odMembers.RemoveAll();
}

int JSONObject::GetMemberNumber() const
{
	return oaMembers.GetSize();
}

JSONMember* JSONObject::GetMemberAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetMemberNumber());
	return cast(JSONMember*, oaMembers.GetAt(nIndex));
}

JSONMember* JSONObject::LookupMember(const ALString& sKey) const
{
	assert(odMembers.Lookup(sKey) == NULL or cast(JSONMember*, odMembers.Lookup(sKey))->GetKey() == sKey);
	return cast(JSONMember*, odMembers.Lookup(sKey));
}

boolean JSONObject::WriteFile(const ALString& sFileName) const
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

void JSONObject::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ALString sIndent;
	int i;

	// Parametrage de l'indentation si necessaire
	if (bPrettyPrint)
		sIndent = ALString('\t', nIndentLevel);

	// Cas particulier d'un objet vide
	if (GetMemberNumber() == 0)
		ost << "{}";
	// Cas general
	else
	{
		// Par d'indentation au depart, pour le cas ou on fait partie d'un membre d'objet ave sa cle
		ost << '{';
		if (bPrettyPrint)
			ost << '\n';

		// Chaque membre est indente au niveau superieur
		for (i = 0; i < GetMemberNumber(); i++)
		{
			GetMemberAt(i)->WriteIndent(ost, nIndentLevel + 1, bPrettyPrint);
			if (i < GetMemberNumber() - 1)
				ost << ',';
			if (bPrettyPrint)
				ost << '\n';
		}

		// Indentation a la fin
		if (bPrettyPrint)
			ost << sIndent;
		ost << '}';
	}
}

const ALString JSONObject::JSONObject::TypeToString() const
{
	return "object";
}

void JSONObject::TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName)
{
	JSONObject testJSONObject;

	require(sReadFileName != "");
	require(sWriteFileName != "");

	testJSONObject.ReadFile(sReadFileName);
	testJSONObject.WriteFile(sWriteFileName);
}

void JSONObject::Test()
{
	TestReadWrite("C:\\temp\\Datasets\\Iris\\AnalysisResults.khj",
		      "C:\\temp\\Datasets\\Iris\\O_AnalysisResults.khj");
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONArray

JSONArray::JSONArray() {}

JSONArray::~JSONArray()
{
	oaValues.DeleteAll();
}

int JSONArray::GetType() const
{
	return ArrayValue;
}

void JSONArray::AddValue(JSONValue* value)
{
	require(value != NULL);

	oaValues.Add(value);
}

void JSONArray::RemoveAll()
{
	oaValues.RemoveAll();
}

void JSONArray::DeleteAll()
{
	oaValues.DeleteAll();
}

int JSONArray::GetValueNumber() const
{
	return oaValues.GetSize();
}

JSONValue* JSONArray::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetValueNumber());
	return cast(JSONValue*, oaValues.GetAt(nIndex));
}

void JSONArray::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ALString sIndent;
	JSONValue* jsonValue;
	int i;

	// Parametrage de l'indentation si necessaire
	if (bPrettyPrint)
		sIndent = ALString('\t', nIndentLevel);

	// Cas particulier d'un objet vide
	if (GetValueNumber() == 0)
		ost << "[]";
	// Cas general
	else
	{
		// Par d'indentation au depart, pour le cas ou on fait partie d'un membre d'objet avec sa cle
		ost << '[';
		if (bPrettyPrint)
			ost << '\n';

		// Chaque membre est indente au niveau superieur
		for (i = 0; i < GetValueNumber(); i++)
		{
			jsonValue = GetValueAt(i);

			// Indentation prealable
			if (bPrettyPrint)
				ost << sIndent << "\t";

			// Ecriture de la valeur en indente dans les cas des type objet ou tableau
			if (jsonValue->GetType() == ObjectValue)
				cast(JSONObject*, jsonValue)->WriteIndent(ost, nIndentLevel + 1, bPrettyPrint);
			else if (jsonValue->GetType() == ArrayValue)
				cast(JSONArray*, jsonValue)->WriteIndent(ost, nIndentLevel + 1, bPrettyPrint);
			// Sinon, ecriture directe
			else
				jsonValue->Write(ost);
			if (i < GetValueNumber() - 1)
				ost << ',';
			if (bPrettyPrint)
				ost << '\n';
		}

		// Indentation a la fin
		if (bPrettyPrint)
			ost << sIndent;
		ost << ']';
	}
}

const ALString JSONArray::TypeToString() const
{
	return "array";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONString

JSONString::JSONString() {}

JSONString::~JSONString() {}

int JSONString::GetType() const
{
	return StringValue;
}

void JSONString::SetString(const ALString& sValue)
{
	sStringValue = sValue;
}

const ALString& JSONString::GetString() const
{
	return sStringValue;
}

void JSONString::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ALString sJSONStringValue;

	// Encodage de la chaine C au format json
	TextService::CToJsonString(sStringValue, sJSONStringValue);
	ost << '"';
	ost << sJSONStringValue;
	ost << '"';
}

const ALString JSONString::TypeToString() const
{
	return "string";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONNumber

JSONNumber::JSONNumber()
{
	dNumberValue = 0;
}

JSONNumber::~JSONNumber() {}

int JSONNumber::GetType() const
{
	return NumberValue;
}

void JSONNumber::SetNumber(double dValue)
{
	dNumberValue = dValue;
}

double JSONNumber::GetNumber() const
{
	return dNumberValue;
}

void JSONNumber::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ost << std::setprecision(10) << dNumberValue;
}

const ALString JSONNumber::TypeToString() const
{
	return "number";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONBoolean

JSONBoolean::JSONBoolean()
{
	bBooleanValue = false;
}

JSONBoolean::~JSONBoolean() {}

int JSONBoolean::GetType() const
{
	return BooleanValue;
}

void JSONBoolean::SetBoolean(boolean bValue)
{
	bBooleanValue = bValue;
}

boolean JSONBoolean::GetBoolean() const
{
	return bBooleanValue;
}

void JSONBoolean::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	if (bBooleanValue)
		ost << "true";
	else
		ost << "false";
}

const ALString JSONBoolean::TypeToString() const
{
	return "boolean";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONNull

JSONNull::JSONNull() {}

JSONNull::~JSONNull() {}

int JSONNull::GetType() const
{
	return NullValue;
}

void JSONNull::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ost << "null";
}

const ALString JSONNull::TypeToString() const
{
	return "null";
}

//////////////////////////////////////////////////////////////////////////////
// Classe JSONMember

JSONMember::JSONMember()
{
	jsonValue = NULL;
}

JSONMember::~JSONMember()
{
	if (jsonValue != NULL)
		delete jsonValue;
}

void JSONMember::SetKey(const ALString& sValue)
{
	sKey = sValue;
}

const ALString& JSONMember::GetKey() const
{
	return sKey;
}

void JSONMember::SetValue(JSONValue* value)
{
	require(jsonValue == NULL);
	require(value != NULL);
	jsonValue = value;
}

JSONValue* JSONMember::GetValue() const
{
	return jsonValue;
}

int JSONMember::GetValueType() const
{
	if (jsonValue == NULL)
		return JSONValue::None;
	else
		return jsonValue->GetType();
}

JSONObject* JSONMember::GetObjectValue() const
{
	require(GetValueType() == JSONValue::ObjectValue);
	return cast(JSONObject*, GetValue());
}

JSONArray* JSONMember::GetArrayValue() const
{
	require(GetValueType() == JSONValue::ArrayValue);
	return cast(JSONArray*, GetValue());
}

JSONString* JSONMember::GetStringValue() const
{
	require(GetValueType() == JSONValue::StringValue);
	return cast(JSONString*, GetValue());
}

JSONNumber* JSONMember::GetNumberValue() const
{
	require(GetValueType() == JSONValue::NumberValue);
	return cast(JSONNumber*, GetValue());
}

JSONBoolean* JSONMember::GetBooleanValue() const
{
	require(GetValueType() == JSONValue::BooleanValue);
	return cast(JSONBoolean*, GetValue());
}

JSONNull* JSONMember::GetNullValue() const
{
	require(GetValueType() == JSONValue::NullValue);
	return cast(JSONNull*, GetValue());
}

void JSONMember::Write(ostream& ost) const
{
	WriteIndent(ost, 0, true);
}

void JSONMember::WriteCompact(ostream& ost) const
{
	WriteIndent(ost, 0, false);
}

void JSONMember::WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const
{
	ALString sIndent;
	ALString sJsonKey;

	// Parametrage de l'indentation si necessaire
	if (bPrettyPrint)
		sIndent = ALString('\t', nIndentLevel);

	// Encodage de la chaine C au format json
	TextService::CToJsonString(sKey, sJsonKey);
	if (bPrettyPrint)
		ost << sIndent;
	ost << '"';
	ost << sJsonKey;
	ost << "\":";
	if (bPrettyPrint)
		ost << ' ';

	// Ecriture de la valeur en indente dans les cas des type objet ou tableau
	GetValue()->WriteIndent(ost, nIndentLevel, bPrettyPrint);
}

const ALString JSONMember::GetClassLabel() const
{
	return "member";
}

const ALString JSONMember::GetObjectLabel() const
{
	return sKey;
}
