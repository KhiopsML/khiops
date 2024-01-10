// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONTokenizer.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_UNUSED_FUNCTION
#include "JsonLex.inc"
DISABLE_WARNING_POP

/* Declaration du lexer utilise */
void jsonerror(char const* fmt);

#define json_STATIC

/* default jsonerror for YACC and LEX */
void jsonerror(char const* fmt)
{
	cout << "Error " << fmt << endl;
}

// Initialisation des variables globales
ALString JSONTokenizer::sErrorFamily;
ALString JSONTokenizer::sFileName;
ALString JSONTokenizer::sLocalFileName;
FILE* JSONTokenizer::fJSON = NULL;
int JSONTokenizer::nLastToken = 0;
boolean JSONTokenizer::bForceAnsi = false;

void JSONTokenizer::SetForceAnsi(boolean bValue)
{
	bForceAnsi = bValue;
}

boolean JSONTokenizer::GetForceAnsi()
{
	return bForceAnsi;
}

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
		jsonlineno = 1;
		jsonrestart(fJSON);
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
	jsonlex_destroy();

	// Fermeture du fichier
	bOk = FileService::CloseInputBinaryFile(sFileName, fJSON);

	// Si le fichier est sur HDFS, on supprime la copie locale
	PLRemoteFileService::CleanInputWorkingFile(sFileName, sLocalFileName);

	// Reinitialisation des variables globales
	sErrorFamily = "";
	sFileName = "";
	fJSON = NULL;
	sJsonTokenString = "";
	cJsonTokenDouble = 0;
	bJsonTokenBoolean = false;
	nLastToken = 0;
	return bOk;
}

boolean JSONTokenizer::CheckToken(int nToken)
{
	if (nToken == '[' or nToken == ']' or nToken == '{' or nToken == '}' or nToken == ',' or nToken == ':' or
	    nToken == 0 or nToken == String or nToken == Number or nToken == Boolean or nToken == Null or
	    nToken == Error)
		return true;
	else
		return false;
}

ALString JSONTokenizer::GetTokenLabel(int nToken)
{
	ALString sLabel;

	require(CheckToken(nToken));

	if (nToken == String)
		sLabel = "String";
	else if (nToken == Number)
		sLabel = "Number";
	else if (nToken == Boolean)
		sLabel = "Boolean";
	else if (nToken == Null)
		sLabel = "Null";
	else if (nToken == Error)
		sLabel = "Error";
	else if (nToken == 0)
		sLabel = "EndOfFile";
	else
	{
		sLabel = "' '";
		sLabel.SetAt(1, (char)nToken);
	}
	return sLabel;
}

int JSONTokenizer::ReadNextToken()
{
	require(IsOpened());

	// Lecture du token
	nLastToken = jsonlex();
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
	return jsonlineno;
}

const ALString& JSONTokenizer::GetTokenStringValue()
{
	require(IsOpened());
	require(nLastToken == String or nLastToken == Error);
	return sJsonTokenString;
}

double JSONTokenizer::GetTokenNumberValue()
{
	require(IsOpened());
	require(nLastToken == Number);
	return cJsonTokenDouble;
}

boolean JSONTokenizer::GetTokenBooleanValue()
{
	require(IsOpened());
	require(nLastToken == Boolean);
	return bJsonTokenBoolean;
}

void JSONTokenizer::TestReadJsonFile(int argc, char** argv)
{
	boolean bOk = true;
	int nToken;
	char cToken;
	ALString sCString;
	ALString sJsonString;
	ALString sTest;
	boolean bTestConversion = false;

	// Test de conversion elementaire
	if (bTestConversion)
	{
		sTest = "aa\\u221Ebb\\u00e9cc\\/\\\\dd\\tee";
		cout << "Json string\t" << sTest << "\t";
		JsonToCString(sTest, sCString);
		cout << sCString << "\t";
	}

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
				nToken = jsonlex();
				if (nToken == String)
				{
					sCString.GetBufferSetLength(0);
					JsonToCString(sJsonTokenString, sCString);
					cout << " \"" << sCString << "\"";
				}
				else if (nToken == Number)
				{
					cout << " " << cJsonTokenDouble;
				}
				else if (nToken == Boolean)
				{
					cout << " " << bJsonTokenBoolean;
				}
				else if (nToken == Error)
				{
					cout << "<ERROR line " << GetCurrentLineIndex() << ": " << sJsonTokenString
					     << " > " << endl;
					break;
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

static void HexStringToCode(const unsigned char* sHexString, unsigned int* nCode)
{
	unsigned int i;
	for (i = 0; i < 4; i++)
	{
		unsigned char c = sHexString[i];
		if (c >= 'A')
			c = (c & ~0x20) - 7;
		c -= '0';
		assert(!(c & 0xF0));
		*nCode = (*nCode << 4) | c;
	}
}

// Recode un entier representant un code UTF32 sous forme d'une sequence de 1 a 4 caracteres au format UTF8
// La chaine doit avoir au moins 5 caracteres pour stocker le resultat
// Retourne le nombre de caracteres utilises
static int Utf32toUtf8String(unsigned int nCode, char* sUtf8Chars)
{
	if (nCode < 0x80)
	{
		sUtf8Chars[0] = (char)nCode;
		sUtf8Chars[1] = 0;
		return 1;
	}
	else if (nCode < 0x0800)
	{
		sUtf8Chars[0] = (char)((nCode >> 6) | 0xC0);
		sUtf8Chars[1] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[2] = 0;
		return 2;
	}
	else if (nCode < 0x10000)
	{
		sUtf8Chars[0] = (char)((nCode >> 12) | 0xE0);
		sUtf8Chars[1] = (char)(((nCode >> 6) & 0x3F) | 0x80);
		sUtf8Chars[2] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[3] = 0;
		return 3;
	}
	else if (nCode < 0x200000)
	{
		sUtf8Chars[0] = (char)((nCode >> 18) | 0xF0);
		sUtf8Chars[1] = (char)(((nCode >> 12) & 0x3F) | 0x80);
		sUtf8Chars[2] = (char)(((nCode >> 6) & 0x3F) | 0x80);
		sUtf8Chars[3] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[4] = 0;
		return 4;
	}
	else
	{
		sUtf8Chars[0] = '?';
		sUtf8Chars[1] = 0;
		return 1;
	}
}

void JSONTokenizer::JsonToCString(const char* sJsonString, ALString& sCString)
{
	const unsigned char* sInputString;
	unsigned int nCode;
	unsigned int nPotentialCode;
	int nBegin;
	int nEnd;
	int nLength;
	char sUtf8Chars[5];
	const char* sCharsToAdd;
	ALString sUnicodeChars;
	int nCharNumber;

	require(sJsonString != NULL);

	// On passe par un format unsigned char pour le parsing
	sInputString = (const unsigned char*)sJsonString;
	nLength = (int)strlen(sJsonString);

	// On repasse la chaine a convertir a vide, sans desallouer la memoire
	sCString.GetBufferSetLength(0);

	// Analyse de la chaine en entree
	nBegin = 0;
	nEnd = 0;
	sCharsToAdd = "?";
	while (nEnd < nLength)
	{
		if (sInputString[nEnd] == '\\')
		{
			AppendSubString(sCString, sJsonString, nBegin, nEnd - nBegin);
			nEnd++;
			assert(nEnd < nLength);
			nCharNumber = 1;
			switch (sInputString[nEnd])
			{
			case 'r':
				sCharsToAdd = "\r";
				break;
			case 'n':
				sCharsToAdd = "\n";
				break;
			case '\\':
				sCharsToAdd = "\\";
				break;
			case '/':
				sCharsToAdd = "/";
				break;
			case '"':
				sCharsToAdd = "\"";
				break;
			case 'f':
				sCharsToAdd = "\f";
				break;
			case 'b':
				sCharsToAdd = "\b";
				break;
			case 't':
				sCharsToAdd = "\t";
				break;
			case 'u':
			{
				nCode = 0;
				nEnd++;
				assert(nEnd < nLength);

				// Extraction des caracteres unicode
				if (sUnicodeChars.GetLength() != 4)
					sUnicodeChars.GetBufferSetLength(4);
				sUnicodeChars.SetAt(0, sInputString[nEnd]);
				sUnicodeChars.SetAt(1, sInputString[nEnd + 1]);
				sUnicodeChars.SetAt(2, sInputString[nEnd + 2]);
				sUnicodeChars.SetAt(3, sInputString[nEnd + 3]);

				// On tente d'abord le decodgage d'un caractere windows-1252 encode avec unicode
				nCode = JSONFile::UnicodeHexToWindows1252(sUnicodeChars);
				if (nCode != -1)
				{
					nEnd += 3;
					nCharNumber = 1;
					sUtf8Chars[0] = (char)nCode;
					sUtf8Chars[1] = '\0';
					sCharsToAdd = sUtf8Chars;
				}
				// Cas general sinon
				else
				{
					HexStringToCode(sInputString + nEnd, &nCode);
					nEnd += 3;
					assert(nEnd < nLength);

					// Verification du code
					if ((nCode & 0xFC00) == 0xD800)
					{
						nEnd++;
						assert(nEnd + 1 < nLength);
						if (sInputString[nEnd] == '\\' && sInputString[nEnd + 1] == 'u')
						{
							nPotentialCode = 0;
							HexStringToCode(sInputString + nEnd + 2, &nPotentialCode);
							nCode = (((nCode & 0x3F) << 10) |
								 ((((nCode >> 6) & 0xF) + 1) << 16) |
								 (nPotentialCode & 0x3FF));
							nEnd += 5;
							assert(nEnd < nLength);
						}
						else
						{
							sCharsToAdd = "?";
							break;
						}
					}

					// Conversion
					nCharNumber = Utf32toUtf8String(nCode, sUtf8Chars);
					sCharsToAdd = sUtf8Chars;

					// Cas particulier du code 0
					if (nCode == 0)
					{
						sCString += sCharsToAdd[0];
						nEnd++;
						nBegin = nEnd;
						continue;
					}
				}
				break;
			}
			default:
				// En principe, impossible avec une chaine json correctement formee
				assert(false);
			}
			if (nCharNumber == 1)
				sCString += sCharsToAdd[0];
			else
				AppendSubString(sCString, sCharsToAdd, 0, nCharNumber);
			nEnd++;
			nBegin = nEnd;
		}
		else
		{
			nEnd++;
		}
	}
	AppendSubString(sCString, sJsonString, nBegin, nEnd - nBegin);
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

boolean JSONTokenizer::ReadContinuousValue(boolean bIsPositive, Continuous& cValue)
{
	boolean bOk;
	double dValue;
	ALString sTmp;

	// Lecture puis controle eventuel de positivite
	bOk = ReadNumberValue(dValue);
	cValue = KWContinuous::DoubleToContinuous(dValue);
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
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
	else
		nValue = 0;
	if (not bOk)
	{
		AddParseError(sTmp + "Invalid integer value (" + KWContinuous::ContinuousToString(dValue) + ")");
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

boolean JSONTokenizer::ReadKeyContinuousValue(const ALString& sKey, boolean bIsPositive, Continuous& cValue,
					      boolean& bIsEnd)
{
	boolean bOk;
	double dValue;

	// Lecture puis controle eventuel de positivite
	bOk = ReadKeyDoubleValue(sKey, bIsPositive, dValue, bIsEnd);
	cValue = KWContinuous::DoubleToContinuous(dValue);
	return bOk;
}

boolean JSONTokenizer::ReadKeyIntValue(const ALString& sKey, boolean bIsPositive, int& nValue, boolean& bIsEnd)
{
	boolean bOk;
	double dValue;

	// Lecture puis conversion en entier
	bOk = ReadKeyDoubleValue(sKey, bIsPositive, dValue, bIsEnd);
	if (bOk)
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
	else
		nValue = 0;
	if (not bOk)
	{
		AddParseError("Invalid " + sKey + " (" + KWContinuous::ContinuousToString(dValue) + ")");
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

void JSONTokenizer::AppendSubString(ALString& sString, const char* sAddedString, int nBegin, int nLength)
{
	int i;
	int nStringLength;

	require(sAddedString != NULL);
	require(nBegin >= 0);
	require(nLength >= 0);
	require(nBegin + nLength <= (int)strlen(sAddedString));

	// Reservation de la place necessaire
	nStringLength = sString.GetLength();
	sString.GetBufferSetLength(nStringLength + nLength);

	// Ajout des caracteres
	for (i = nBegin; i < nBegin + nLength; i++)
	{
		sString.SetAt(nStringLength, sAddedString[i]);
		nStringLength++;
	}
}

void JSONTokenizer::AddParseError(const ALString& sLabel)
{
	Global::AddError(sErrorFamily, sFileName + " line " + IntToString(GetCurrentLineIndex()), sLabel);
}
