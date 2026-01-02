// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONTokenizer.h"

// Initialisation des variables globales
ALString JSONTokenizer::sErrorFamily;
ALString JSONTokenizer::sFileName;
ALString JSONTokenizer::sLocalFileName;
FILE* JSONTokenizer::fJSON = NULL;
int JSONTokenizer::nLastToken = 0;
JSONSTYPE JSONTokenizer::jsonLastTokenValue = {0};

boolean JSONTokenizer::OpenForRead(const ALString& sErrorFamilyName, const ALString& sInputFileName)
{
	boolean bOk;

	require(not IsOpened());
	require(sInputFileName != "");

	// Copie depuis HDFS si necessaire
	bOk = PLRemoteFileService::BuildInputWorkingFile(sInputFileName, sLocalFileName);

	// Ouverture du fichier
	if (bOk)
		bOk = FileService::OpenInputBinaryFile(sLocalFileName, fJSON);

	// Parametrage du parser genere par flex
	if (bOk)
	{
		sErrorFamily = sErrorFamilyName;
		sFileName = sInputFileName;
		nLastToken = -1;
		jsonLastTokenValue.dValue = 0;
		JSONObject::SetLineno(1);
		JSONObject::Restart(fJSON);
	}
	return IsOpened();
}

boolean JSONTokenizer::IsOpened()
{
	return fJSON != NULL;
}

const ALString& JSONTokenizer::GetErrorFamilyName()
{
	return sErrorFamily;
}

const ALString& JSONTokenizer::GetFileName()
{
	return sFileName;
}

boolean JSONTokenizer::Close()
{
	boolean bOk;

	require(IsOpened());

	// Nettoyage du lexer
	JSONObject::LexDestroy();

	// Fermeture du fichier
	bOk = FileService::CloseInputBinaryFile(sFileName, fJSON);

	// Si le fichier est sur HDFS, on supprime la copie locale
	PLRemoteFileService::CleanInputWorkingFile(sFileName, sLocalFileName);

	// Reinitialisation des variables globales
	sErrorFamily = "";
	sFileName = "";
	fJSON = NULL;
	if (nLastToken == String or nLastToken == Error)
		delete jsonLastTokenValue.sValue;
	jsonLastTokenValue.dValue = 0;
	nLastToken = -1;
	return bOk;
}

boolean JSONTokenizer::CheckToken(int nToken)
{
	if (nToken == '[' or nToken == ']' or nToken == '{' or nToken == '}' or nToken == ',' or nToken == ':' or
	    nToken == 0 or nToken == String or nToken == Number or nToken == Boolean or nToken == Null or
	    nToken == StringError or nToken == Error)
		return true;
	else
		return false;
}

ALString JSONTokenizer::GetTokenLabel(int nToken)
{
	ALString sLabel;

	require(CheckToken(nToken));

	if (nToken == String)
		sLabel = "string";
	else if (nToken == Number)
		sLabel = "number";
	else if (nToken == Boolean)
		sLabel = "boolean";
	else if (nToken == Null)
		sLabel = "null";
	else if (nToken == StringError)
		sLabel = "string error";
	else if (nToken == Error)
		sLabel = "Error";
	else if (nToken == 0)
		sLabel = "EndOfFile";
	else if (nToken < 256)
	{
		sLabel = "' '";
		sLabel.SetAt(1, (char)nToken);
	}
	else
		sLabel = "unknown";
	return sLabel;
}

int JSONTokenizer::ReadNextToken()
{
	ALString sValueCopy;

	require(IsOpened());

	// Nettoyage eventuel de la chaine de caractere associee au dernier token
	if (nLastToken == String or nLastToken == Error)
		delete jsonLastTokenValue.sValue;

	// Lecture du token
	nLastToken = JSONObject::Lex(&jsonLastTokenValue);

	// On accepte avec un warning les erreur d'encodage de chaine de caracteres
	if (nLastToken == StringError)
	{
		nLastToken = String;
		AddParseWarning("Read token " + GetTokenLabel(nLastToken) + GetLastTokenValue() +
				" with non-utf8 encoding");
	}
	return nLastToken;
}

int JSONTokenizer::GetLastToken()
{
	require(IsOpened());
	return nLastToken;
}

int JSONTokenizer::GetCurrentLineIndex()
{
	require(IsOpened());
	return JSONObject::GetLineno();
}

const ALString& JSONTokenizer::GetTokenStringValue()
{
	require(IsOpened());
	require(nLastToken == String or nLastToken == Error);
	return *jsonLastTokenValue.sValue;
}

double JSONTokenizer::GetTokenNumberValue()
{
	require(IsOpened());
	require(nLastToken == Number);
	return jsonLastTokenValue.dValue;
}

boolean JSONTokenizer::GetTokenBooleanValue()
{
	require(IsOpened());
	require(nLastToken == Boolean);
	return jsonLastTokenValue.bValue;
}

void JSONTokenizer::TestReadJsonFile(int argc, char** argv)
{
	boolean bOk = true;
	int nToken;
	char cToken;
	ALString sJsonString;

	// Erreur si pas de nom de fichier
	if (argc != 2)
	{
		cout << "ReadJsonFile <FileName>" << endl;
	}
	// Sinon, analyse du fichier
	else
	{
		// Ouverture du fichier
		bOk = OpenForRead("Test", argv[1]);

		// On continue si fichier ouvert correctement
		if (bOk)
		{
			nToken = 1;
			while (nToken != 0)
			{
				nToken = ReadNextToken();
				if (nToken == String)
				{
					TextService::CToJsonString(GetTokenStringValue(), sJsonString);
					cout << " \"" << sJsonString << "\"";
				}
				else if (nToken == Number)
				{
					// Utilisation d'une precision de 10 decimales pour le test
					cout << " " << std::setprecision(10) << GetTokenNumberValue();
				}
				else if (nToken == Boolean)
				{
					cout << " " << BooleanToString(GetTokenBooleanValue());
				}
				else if (nToken == Error)
				{
					cout << "<ERROR line " << GetCurrentLineIndex() << ": " << GetTokenStringValue()
					     << " > " << endl;
				}
				else
				{
					cToken = (char)nToken;
					cout << cToken;
					if (cToken == ',' or cToken == '}' or cToken == ']')
						cout << "\n";
				}
			}

			// Fermeture du fichier
			Close();
		}
	}
}

boolean JSONTokenizer::ReadExpectedToken(int nExpectedToken)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture du token
	ReadNextToken();

	// Verification du token
	if (nLastToken != nExpectedToken)
	{
		AddParseError("Read token " + GetTokenLabel(nLastToken) + GetLastTokenValue() +
			      " instead of expected " + GetTokenLabel(nExpectedToken));
		bOk = false;
	}
	return bOk;
}

boolean JSONTokenizer::ReadStringValue(ALString& sValue)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la valeur
	bOk = bOk and ReadExpectedToken(String);
	if (bOk)
		sValue = GetTokenStringValue();
	else
		sValue = "";
	return bOk;
}

boolean JSONTokenizer::ReadNumberValue(double& dValue)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la valeur
	bOk = bOk and ReadExpectedToken(Number);
	if (bOk)
		dValue = GetTokenNumberValue();
	else
		dValue = 0;
	return bOk;
}

boolean JSONTokenizer::ReadBooleanValue(boolean& bValue)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la valeur
	bOk = bOk and ReadExpectedToken(Boolean);
	if (bOk)
		bValue = GetTokenBooleanValue();
	else
		bValue = false;
	return bOk;
}

boolean JSONTokenizer::ReadNullValue()
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la valeur
	bOk = bOk and ReadExpectedToken(Null);
	return bOk;
}

boolean JSONTokenizer::ReadDoubleValue(boolean bIsPositive, double& dValue)
{
	boolean bOk;
	ALString sTmp;

	// Lecture puis controle eventuel de positivite
	bOk = ReadNumberValue(dValue);
	if (bOk and bIsPositive and dValue < 0)
	{
		AddParseError(sTmp + "Invalid value (" + DoubleToString(dValue) + ")");
		bOk = false;
	}
	return bOk;
}

boolean JSONTokenizer::ReadIntValue(boolean bIsPositive, int& nValue)
{
	boolean bOk;
	double dValue;
	ALString sTmp;

	// Lecture puis conversion en entier
	bOk = ReadDoubleValue(bIsPositive, dValue);
	if (bOk)
		bOk = DoubleToInt(dValue, nValue);
	else
		nValue = 0;
	if (not bOk)
	{
		AddParseError(sTmp + "Invalid integer value (" + DoubleToString(dValue) + ")");
		bOk = false;
	}
	return bOk;
}

boolean JSONTokenizer::ReadKey(const ALString& sKey)
{
	boolean bOk = true;
	ALString sTmp;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture du token cle
	bOk = bOk and ReadExpectedToken(String);
	if (bOk)
	{
		if (sKey != GetTokenStringValue())
		{
			AddParseError(sTmp + "Read key \"" + GetTokenStringValue() + "\" instead of expected \"" +
				      sKey + "\"");
			bOk = false;
		}
	}

	// Lecture du ':' apres la cle
	bOk = bOk and ReadExpectedToken(':');
	return bOk;
}

boolean JSONTokenizer::ReadKeyStringValue(const ALString& sKey, ALString& sValue, boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture de la valeur
	bOk = bOk and ReadStringValue(sValue);

	// Lecture de l'indicateur de fin
	bOk = bOk and ReadObjectNext(bIsEnd);
	return bOk;
}

boolean JSONTokenizer::ReadKeyNumberValue(const ALString& sKey, double& dValue, boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture de la valeur
	bOk = bOk and ReadNumberValue(dValue);

	// Lecture de l'indicateur de fin
	bOk = bOk and ReadObjectNext(bIsEnd);
	return bOk;
}

boolean JSONTokenizer::ReadKeyBooleanValue(const ALString& sKey, boolean& bValue, boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture de la valeur
	bOk = bOk and ReadBooleanValue(bValue);

	// Lecture de l'indicateur de fin
	bOk = bOk and ReadObjectNext(bIsEnd);
	return bOk;
}

boolean JSONTokenizer::ReadKeyNullValue(const ALString& sKey, boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture de la valeur
	bOk = bOk and ReadNullValue();

	// Lecture de l'indicateur de fin
	bOk = bOk and ReadObjectNext(bIsEnd);
	return bOk;
}

boolean JSONTokenizer::ReadKeyDoubleValue(const ALString& sKey, boolean bIsPositive, double& dValue, boolean& bIsEnd)
{
	boolean bOk;

	// Lecture puis controle eventuel de positivite
	bOk = ReadKeyNumberValue(sKey, dValue, bIsEnd);
	if (bOk and bIsPositive and dValue < 0)
	{
		AddParseError("Invalid " + sKey + " (" + DoubleToString(dValue) + ")");
		bOk = false;
	}
	return bOk;
}

boolean JSONTokenizer::ReadKeyIntValue(const ALString& sKey, boolean bIsPositive, int& nValue, boolean& bIsEnd)
{
	boolean bOk;
	double dValue;

	// Lecture puis conversion en entier
	bOk = ReadKeyDoubleValue(sKey, bIsPositive, dValue, bIsEnd);
	if (bOk)
		bOk = DoubleToInt(dValue, nValue);
	else
		nValue = 0;
	if (not bOk)
	{
		AddParseError("Invalid " + sKey + " (" + DoubleToString(dValue) + ")");
		bOk = false;
	}
	return bOk;
}

boolean JSONTokenizer::ReadKeyObject(const ALString& sKey)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture du debut de l'objet
	bOk = bOk and ReadExpectedToken('{');
	return bOk;
}

boolean JSONTokenizer::ReadKeyArray(const ALString& sKey)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture de la cle
	bOk = bOk and ReadKey(sKey);

	// Lecture du debut du tableau
	bOk = bOk and ReadExpectedToken('[');
	return bOk;
}

boolean JSONTokenizer::ReadObjectNext(boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture du prochain token
	ReadNextToken();

	// Verification du token
	if (nLastToken != ',' and nLastToken != '}')
	{
		AddParseError("Read token " + GetTokenLabel(nLastToken) + GetLastTokenValue() +
			      " instead of expected " + GetTokenLabel(',') + " or " + GetTokenLabel('}'));
		bOk = false;
	}
	bIsEnd = (nLastToken != ',');
	return bOk;
}

boolean JSONTokenizer::ReadArrayNext(boolean& bIsEnd)
{
	boolean bOk = true;

	require(IsOpened());
	require(nLastToken != 0);

	// Lecture du prochain token
	ReadNextToken();

	// Verification du token
	if (nLastToken != ',' and nLastToken != ']')
	{
		AddParseError("Read token " + GetTokenLabel(nLastToken) + GetLastTokenValue() +
			      " instead of expected " + GetTokenLabel(',') + " or " + GetTokenLabel(']'));
		bOk = false;
	}
	bIsEnd = (nLastToken != ',');
	return bOk;
}

boolean JSONTokenizer::CheckObjectEnd(const ALString& sKey, boolean bIsEnd)
{
	boolean bOk = true;

	if (not bIsEnd)
	{
		AddParseError("Object " + sKey + " should be ended");
		bOk = false;
	}
	return bOk;
}

const ALString JSONTokenizer::GetLastTokenValue()
{
	ALString sValue;

	require(IsOpened());

	// Acces a la derniere valeur
	if (nLastToken == String or nLastToken == Error)
		sValue = sValue + "\"" + GetTokenStringValue() + "\"";
	else if (nLastToken == Number)
		sValue = DoubleToString(GetTokenNumberValue());
	else if (nLastToken == Boolean)
	{
		if (GetTokenBooleanValue())
			sValue = "true";
		else
			sValue = "false";
	}

	// Mise en tre parenthses si necessaire
	if (sValue != "")
		sValue = "(" + sValue + ")";
	return sValue;
}

boolean JSONTokenizer::DoubleToInt(double dValue, int& nValue)
{
	// Recherche de l'entier le plus proche
	nValue = int(floor(dValue + 0.5));

	// Ok si presque egale, avec tolerance
	if (fabs(dValue - nValue) < 1e-6)
		return true;
	else
	{
		nValue = 0;
		return false;
	}
}

void JSONTokenizer::AddParseWarning(const ALString& sLabel)
{
	Global::AddWarning(sErrorFamily, sFileName + " line " + IntToString(GetCurrentLineIndex()), sLabel);
}

void JSONTokenizer::AddParseError(const ALString& sLabel)
{
	Global::AddError(sErrorFamily, sFileName + " line " + IntToString(GetCurrentLineIndex()), sLabel);
}
