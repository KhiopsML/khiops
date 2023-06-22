// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONFile.h"

JSONFile::JSONFile()
{
	nCurrentListLevel = 0;
	bCamelCaseKeys = false;
}

JSONFile::~JSONFile() {}

void JSONFile::SetFileName(const ALString& sValue)
{
	require(not IsOpened());
	sFileName = sValue;
}

const ALString& JSONFile::GetFileName() const
{
	return sFileName;
}

boolean JSONFile::OpenForWrite()
{
	boolean bOk = true;

	require(not IsOpened());
	require(GetFileName() != "");
	require(ivLevelElementNumber.GetSize() == 0);
	require(nCurrentListLevel == 0);

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Write report Begin");

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);
	if (bOk)
		// Ouverture du fichier
		bOk = FileService::OpenOutputFile(sLocalFileName, fstJSON);

	// Debut de l'objet global
	if (bOk)
	{
		fstJSON << "{";
		Indent();
	}

	return IsOpened();
}

boolean JSONFile::IsOpened()
{
	return fstJSON.is_open();
}

boolean JSONFile::Close()
{
	boolean bOk;

	require(IsOpened());

	// Fin de l'objet global
	Unindent();
	fstJSON << "\n}\n";

	// Fermeture du fichier
	bOk = FileService::CloseOutputFile(sLocalFileName, fstJSON);

	// Copie vers HDFS si necessaire
	PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Write report End");
	ensure(ivLevelElementNumber.GetSize() == 0);
	ensure(nCurrentListLevel == 0);
	return bOk;
}

void JSONFile::WriteString(const ALString& sValue)
{
	require(IsOpened());
	WriteIndent();
	WriteStringValue(sValue);
}

void JSONFile::WriteInt(int nValue)
{
	require(IsOpened());
	WriteIndent();
	WriteIntValue(nValue);
}

void JSONFile::WriteLongint(longint lValue)
{
	require(IsOpened());
	WriteIndent();
	WriteLongintValue(lValue);
}

void JSONFile::WriteDouble(double dValue)
{
	require(IsOpened());
	WriteIndent();
	WriteDoubleValue(dValue);
}

void JSONFile::WriteContinuous(Continuous cValue)
{
	require(IsOpened());
	WriteIndent();
	WriteContinuousValue(cValue);
}

void JSONFile::WriteBoolean(boolean bValue)
{
	require(IsOpened());
	WriteIndent();
	WriteBooleanValue(bValue);
}

void JSONFile::WriteNull()
{
	require(IsOpened());
	WriteIndent();
	WriteNullValue();
}

void JSONFile::WriteKeyString(const ALString& sKey, const ALString& sValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteStringValue(sValue);
}

void JSONFile::WriteKeyInt(const ALString& sKey, int nValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteIntValue(nValue);
}

void JSONFile::WriteKeyLongint(const ALString& sKey, longint lValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteLongintValue(lValue);
}

void JSONFile::WriteKeyDouble(const ALString& sKey, double dValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteDoubleValue(dValue);
}

void JSONFile::WriteKeyContinuous(const ALString& sKey, Continuous cValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteContinuousValue(cValue);
}

void JSONFile::WriteKeyBoolean(const ALString& sKey, boolean bValue)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteBooleanValue(bValue);
}

void JSONFile::WriteKeyNull(const ALString& sKey)
{
	require(IsOpened());
	WriteKey(sKey);
	WriteNullValue();
}

void JSONFile::BeginKeyObject(const ALString& sKey)
{
	require(IsOpened());
	WriteKey(sKey);
	fstJSON << "{";
	Indent();
}

void JSONFile::BeginObject()
{
	require(IsOpened());
	WriteIndent();
	fstJSON << "{";
	Indent();
}

void JSONFile::EndObject()
{
	require(IsOpened());
	EndBlock('}', false);
}

void JSONFile::BeginKeyArray(const ALString& sKey)
{
	require(IsOpened());
	WriteKey(sKey);
	fstJSON << "[";
	Indent();
}

void JSONFile::BeginArray()
{
	require(IsOpened());
	WriteIndent();
	fstJSON << "[";
	Indent();
}

void JSONFile::EndArray()
{
	require(IsOpened());
	EndBlock(']', false);
}

void JSONFile::BeginKeyList(const ALString& sKey)
{
	require(IsOpened());
	WriteKey(sKey);
	fstJSON << "[";
	nCurrentListLevel++;
	Indent();
}

void JSONFile::BeginList()
{
	require(IsOpened());
	WriteIndent();
	fstJSON << "[";
	nCurrentListLevel++;
	Indent();
}

void JSONFile::EndList()
{
	require(IsOpened());
	EndBlock(']', true);
}

void JSONFile::SetCamelCaseKeys(boolean bValue)
{
	bCamelCaseKeys = bValue;
}

boolean JSONFile::GetCamelCaseKeys() const
{
	return bCamelCaseKeys;
}

const ALString JSONFile::ToCamelCase(const ALString& sKey)
{
	ALString sConvertedKey;
	int i;
	char c;
	boolean bBeginWord;

	// Boucle de conversion au format camel case
	bBeginWord = false;
	for (i = 0; i < sKey.GetLength(); i++)
	{
		c = sKey.GetAt(i);

		// On saute les blancs
		if (isblank(c))
		{
			// Le premier caractere non blanc suivant sera un debut de mot
			bBeginWord = true;
			continue;
		}

		// On ecrit le caractere en majuscule si c'est un debut de mot et que ce n'est pas le premier caractere
		if (sConvertedKey.GetLength() > 0 and bBeginWord)
		{
			sConvertedKey += (char)toupper(c);
			bBeginWord = false;
		}
		// Sinon, on ecrit le caractere en minuscules
		else
			sConvertedKey += (char)tolower(c);
	}
	return sConvertedKey;
}

const ALString JSONFile::GetClassLabel() const
{
	return "JSON file";
}

const ALString JSONFile::GetObjectLabel() const
{
	return sFileName;
}

void JSONFile::Test()
{
	JSONFile fJSON;
	ALString sFileName;
	int i;
	int j;
	ALString sUnicodeChars;
	int nCode;

	// Test des encodages unicode des caracteres ascii etendus
	for (i = 0; i < 256; i++)
	{
		Windows1252ToUnicode(i, sUnicodeChars);
		nCode = UnicodeToWindows1252(sUnicodeChars);
		cout << i << "\t";
		if (0x20 <= i and i <= 0x7F)
			cout << (char)i << "\t";
		else
			cout << "\t";
		cout << sUnicodeChars << "\t";
		cout << nCode << "\t";
		cout << (i == nCode) << endl;
		assert(i == nCode);
	}

	// Ouverture
	sFileName = FileService::CreateTmpFile("Test.json", NULL);
	fJSON.SetFileName(sFileName);
	fJSON.OpenForWrite();

	// Ecriture
	if (fJSON.IsOpened())
	{
		fJSON.BeginKeyObject("menu");
		fJSON.WriteKeyString("id", "file");
		fJSON.WriteKeyString("value", "File");
		fJSON.BeginKeyObject("popup");

		// Objet vide
		fJSON.BeginKeyObject("empty");
		fJSON.EndObject();

		// Objet singleton
		fJSON.BeginKeyObject("singleton");
		fJSON.WriteKeyInt("one", 1);
		fJSON.EndObject();

		// Tableau d'objets
		fJSON.BeginKeyArray("menuitem");
		fJSON.BeginObject();
		fJSON.WriteKeyString("value", "New");
		fJSON.WriteKeyString("onclick", "CreateNewDoc()");
		fJSON.EndObject();
		fJSON.BeginObject();
		fJSON.WriteKeyString("value", "Open");
		fJSON.WriteKeyString("onclick", "OpenDoc()");
		fJSON.EndObject();
		fJSON.BeginObject();
		fJSON.WriteKeyString("value", "Close");
		fJSON.WriteKeyString("onclick", "CloseDoc()");
		fJSON.EndObject();
		fJSON.EndArray();
		fJSON.EndObject();
		fJSON.EndObject();

		// Tableau de chaines
		fJSON.BeginKeyArray("colors");
		fJSON.WriteString("red");
		fJSON.WriteString("green");
		fJSON.WriteString("yellow");
		fJSON.WriteString("blue");
		fJSON.EndArray();

		// Tableau heterogene
		fJSON.BeginKeyArray("heteregeneous");
		fJSON.WriteString("hello");
		fJSON.WriteContinuous(1);
		fJSON.WriteNull();
		fJSON.WriteBoolean(true);
		fJSON.WriteBoolean(false);
		fJSON.EndArray();

		// Tableau au format csv, avec des lignes de valeurs
		fJSON.BeginKeyArray("simpleCSV");
		for (i = 0; i < 3; i++)
		{
			fJSON.BeginList();
			for (j = 0; j < 4; j++)
				fJSON.WriteInt(j + 1);
			fJSON.EndList();
		}
		fJSON.EndArray();

		// Tableau au format csv, avec des lignes de valeurs, en deux parties
		fJSON.BeginKeyArray("complexCSV");
		for (i = 0; i < 3; i++)
		{
			fJSON.BeginList();
			fJSON.BeginList();
			for (j = 0; j < 2; j++)
				fJSON.WriteString(IntToString(j + 1));
			fJSON.EndList();
			fJSON.BeginList();
			for (j = 0; j < 4; j++)
				fJSON.WriteInt(j + 1);
			fJSON.EndList();
			fJSON.EndList();
		}
		fJSON.EndArray();
	}

	// Fermeture
	fJSON.Close();
}

void JSONFile::WriteKey(const ALString& sKey)
{
	require(IsOpened());
	WriteIndent();
	if (bCamelCaseKeys)
		WriteStringValue(ToCamelCase(sKey));
	else
		WriteStringValue(sKey);
	fstJSON << ": ";
}

void JSONFile::WriteStringValue(const ALString& sValue)
{
	int i;
	unsigned char c;
	const char* cHexMap = "0123456789ABCDEF";
	ALString sUnicodeChars;
	int nUTF8CharNumber;

	require(IsOpened());

	// Chaine entre double-quotes
	fstJSON << '"';
	i = 0;
	while (i < sValue.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharNumber = GetValidUTF8CharNumberAt(sValue, i);

		// Cas avec 0 ou un caractere valide
		if (nUTF8CharNumber <= 1)
		{
			c = (unsigned char)sValue.GetAt(i);
			i++;

			// Gestion des caracteres speciaux
			switch (c)
			{
			case '"':
				fstJSON << "\\\"";
				break;
			case '\\':
				fstJSON << "\\\\";
				break;
			case '/':
				fstJSON << "\\/";
				break;
			case '\b':
				fstJSON << "\\b";
				break;
			case '\f':
				fstJSON << "\\f";
				break;
			case '\n':
				fstJSON << "\\n";
				break;
			case '\r':
				fstJSON << "\\r";
				break;
			case '\t':
				fstJSON << "\\t";
				break;
			default:
				// Caracteres de controle ansi
				if (c < 0x20)
					fstJSON << "\\u00" << cHexMap[c / 16] << cHexMap[c % 16];
				// Caracteres de ascii etendu
				else if (c >= 0x80)
				{
					Windows1252ToUnicode(c, sUnicodeChars);
					fstJSON << "\\u" << sUnicodeChars;
				}
				// Caractere standard
				else
					fstJSON << c;
				break;
			}
		}
		// Cas d'un catactere UTF8 multi-byte
		else if (nUTF8CharNumber == 2)
		{
			fstJSON << (unsigned char)sValue.GetAt(i);
			fstJSON << (unsigned char)sValue.GetAt(i + 1);
			i += 2;
		}
		else if (nUTF8CharNumber == 3)
		{
			fstJSON << (unsigned char)sValue.GetAt(i);
			fstJSON << (unsigned char)sValue.GetAt(i + 1);
			fstJSON << (unsigned char)sValue.GetAt(i + 2);
			i += 3;
		}
		else if (nUTF8CharNumber == 4)
		{
			fstJSON << (unsigned char)sValue.GetAt(i);
			fstJSON << (unsigned char)sValue.GetAt(i + 1);
			fstJSON << (unsigned char)sValue.GetAt(i + 2);
			fstJSON << (unsigned char)sValue.GetAt(i + 3);
			i += 4;
		}
	}
	fstJSON << '"';
}

void JSONFile::WriteIntValue(int nValue)
{
	require(IsOpened());
	fstJSON << nValue;
}

void JSONFile::WriteLongintValue(longint lValue)
{
	require(IsOpened());
	fstJSON << lValue;
}

void JSONFile::WriteDoubleValue(double dValue)
{
	require(IsOpened());
	fstJSON << dValue;
}

void JSONFile::WriteContinuousValue(Continuous cValue)
{
	require(IsOpened());
	if (cValue == KWContinuous::GetMissingValue())
		WriteNullValue();
	else
		fstJSON << KWContinuous::ContinuousToString(cValue);
}

void JSONFile::WriteBooleanValue(boolean bValue)
{
	require(IsOpened());
	if (bValue)
		fstJSON << "true";
	else
		fstJSON << "false";
}

void JSONFile::WriteNullValue()
{
	require(IsOpened());
	fstJSON << "null";
}

void JSONFile::WriteIndent()
{
	int i;
	require(IsOpened());

	// Ajout d'un separateur si necessaire
	if (ivLevelElementNumber.GetAt(ivLevelElementNumber.GetSize() - 1) > 0)
		fstJSON << ",";

	// Mise en forme sauf si dans une liste
	if (nCurrentListLevel == 0)
	{
		// Nouvelle ligne
		fstJSON << "\n";

		// Ecriture du niveau d'indentation
		for (i = 0; i < ivLevelElementNumber.GetSize(); i++)
			fstJSON << '\t';
	}

	// Incrementation du nombre d'elements au niveau courant
	ivLevelElementNumber.UpgradeAt(ivLevelElementNumber.GetSize() - 1, 1);
}

void JSONFile::EndBlock(char cBlockChar, boolean bInList)
{
	boolean bIsEmptyBlock;
	int i;

	require(IsOpened());
	require(cBlockChar == '}' or cBlockChar == ']');

	bIsEmptyBlock = (ivLevelElementNumber.GetAt(ivLevelElementNumber.GetSize() - 1) == 0);
	Unindent();
	if (not bIsEmptyBlock)
	{
		// Mise en forme sauf si dans une liste
		if (nCurrentListLevel == 0)
		{
			// Nouvelle ligne
			fstJSON << "\n";

			// Ecriture du niveau d'indentation
			for (i = 0; i < ivLevelElementNumber.GetSize(); i++)
				fstJSON << '\t';
		}
	}
	if (bInList)
		nCurrentListLevel--;
	fstJSON << cBlockChar;
}

void JSONFile::Indent()
{
	require(IsOpened());

	// Ajout d'un niveau
	ivLevelElementNumber.Add(0);
}

void JSONFile::Unindent()
{
	require(IsOpened());
	require(ivLevelElementNumber.GetSize() > 0);

	// Suppression d'un niveau
	ivLevelElementNumber.SetSize(ivLevelElementNumber.GetSize() - 1);
}

int JSONFile::GetValidUTF8CharNumberAt(const ALString& sValue, int nStart)
{
	int nCharNumber;
	int c;
	int nLength;

	require(0 <= nStart and nStart < sValue.GetLength());

	// Initialisations
	nCharNumber = 0;
	nLength = sValue.GetLength();
	c = (unsigned char)sValue.GetAt(nStart);

	// Cas d'un caractere ascii 0bbbbbbb
	if (0x00 <= c and c <= 0x7f)
		nCharNumber = 1;
	// Debut d'un caractere UTF8 sur deux octets 110bbbbb
	else if ((c & 0xE0) == 0xC0)
	{
		if (nStart + 1 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80)
			nCharNumber = 2;
		else
			nCharNumber = 0;
	}
	// Debut d'un caractere UTF8 sur trois octets 1110bbbb
	else if ((c & 0xF0) == 0xE0)
	{
		if (nStart + 2 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80)
			nCharNumber = 3;
		else
			nCharNumber = 0;
	}
	// Debut d'un caractere UTF8 sur trois octets 11110bbb
	else if ((c & 0xF8) == 0xF0)
	{
		if (nStart + 3 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 3) & 0xC0) == 0x80)
			nCharNumber = 3;
		else
			nCharNumber = 0;
	}
	return nCharNumber;
}

void JSONFile::Windows1252ToUnicode(int nCode, ALString& sUnicodeChars)
{
	require(0 <= nCode and nCode <= 255);

	// Initialisation si necessaire de la table de conversion
	InitializeWindows1252UnicodeSequences();
	sUnicodeChars = svWindows1252UnicodeSequences.GetAt(nCode);
}

int JSONFile::UnicodeToWindows1252(const ALString& sUnicodeChars)
{
	int nCode;
	int i;

	require(sUnicodeChars.GetLength() == 1 or sUnicodeChars.GetLength() == 4);

	// Initialisation si necessaire de la table de conversion
	InitializeWindows1252UnicodeSequences();

	// Cas de un seul caractere
	nCode = -1;
	if (sUnicodeChars.GetLength() == 1)
	{
		nCode = (int)sUnicodeChars.GetAt(0);
	}
	// Cas ou les deux premiers caracteres sont encodes avec "00"
	else if (sUnicodeChars.GetAt(0) == '0' and sUnicodeChars.GetAt(1) == '0')
	{
		// Decodage du premier caractere suivant
		if (sUnicodeChars.GetAt(2) >= 'A')
			nCode = 10 + sUnicodeChars.GetAt(2) - 'A';
		else
			nCode = sUnicodeChars.GetAt(2) - '0';
		// Le caractere est soit un caractere special, soit ascii etendu si on est audessus de 0xA0
		if (nCode < 2 or nCode >= 10)
		{
			nCode *= 16;
			if (sUnicodeChars.GetAt(3) >= 'A')
				nCode += 10 + sUnicodeChars.GetAt(3) - 'A';
			else
				nCode += sUnicodeChars.GetAt(3) - '0';
		}
		else
			nCode = -1;
	}

	// Sinon, on recherche dans la table des caracteres speciaux
	if (nCode == -1)
	{
		for (i = 0x80; i <= 0x9F; i++)
		{
			if (sUnicodeChars == svWindows1252UnicodeSequences.GetAt(i))
			{
				nCode = i;
				break;
			}
		}
	}
	ensure(nCode == -1 or (0 <= nCode and nCode <= 255));
	ensure(nCode == -1 or svWindows1252UnicodeSequences.GetAt(nCode) == sUnicodeChars);
	return nCode;
}

void JSONFile::InitializeWindows1252UnicodeSequences()
{
	boolean bWindows1252Encoding = true;

	if (svWindows1252UnicodeSequences.GetSize() == 0)
	{
		const char* cHexMap = "0123456789ABCDEF";
		int i;
		ALString sUnicodePrefix = "00";
		ALString sEmpty;

		// Encodage des caracteres de controle ANSI, avec 4 caracteres hexa
		for (i = 0; i < 0x20; i++)
			svWindows1252UnicodeSequences.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage tel quel des caracteres ANSI, avec un seul caractere
		for (i = 0x20; i < 0x80; i++)
			svWindows1252UnicodeSequences.Add(sEmpty + (char)i);

		// Encodage des caracteres de controle ascii etendu windows-1252, avec 4 caracteres hexa
		assert(svWindows1252UnicodeSequences.GetSize() == 128);
		if (bWindows1252Encoding)
		{
			svWindows1252UnicodeSequences.Add("20AC"); // Euro Sign
			svWindows1252UnicodeSequences.Add("0081"); // UNASSIGNED
			svWindows1252UnicodeSequences.Add("201A"); // Single Low-9 Quotation Mark
			svWindows1252UnicodeSequences.Add("0192"); // Latin Small Letter F With Hook
			svWindows1252UnicodeSequences.Add("201E"); // Double Low-9 Quotation Mark
			svWindows1252UnicodeSequences.Add("2026"); // Horizontal Ellipsis
			svWindows1252UnicodeSequences.Add("2020"); // Dagger
			svWindows1252UnicodeSequences.Add("2021"); // Double Dagger
			svWindows1252UnicodeSequences.Add("02C6"); // Modifier Letter Circumflex Accent
			svWindows1252UnicodeSequences.Add("2030"); // Per Mille Sign
			svWindows1252UnicodeSequences.Add("0160"); // Latin Capital Letter S With Caron
			svWindows1252UnicodeSequences.Add("2039"); // Single Left-Pointing Angle Quotation Mark
			svWindows1252UnicodeSequences.Add("0152"); // Latin Capital Ligature OE
			svWindows1252UnicodeSequences.Add("008D"); // UNASSIGNED
			svWindows1252UnicodeSequences.Add("017D"); // Latin Capital Letter Z With Caron
			svWindows1252UnicodeSequences.Add("008F"); // UNASSIGNED
			svWindows1252UnicodeSequences.Add("0090"); // UNASSIGNED
			svWindows1252UnicodeSequences.Add("2018"); // Left Single Quotation Mark
			svWindows1252UnicodeSequences.Add("2019"); // Right Single Quotation Mark
			svWindows1252UnicodeSequences.Add("201C"); // Left Double Quotation Mark
			svWindows1252UnicodeSequences.Add("201D"); // Right Double Quotation Mark
			svWindows1252UnicodeSequences.Add("2022"); // Bullet
			svWindows1252UnicodeSequences.Add("2013"); // En Dash
			svWindows1252UnicodeSequences.Add("2014"); // Em Dash
			svWindows1252UnicodeSequences.Add("02DC"); // Small Tilde
			svWindows1252UnicodeSequences.Add("2122"); // Trade Mark Sign
			svWindows1252UnicodeSequences.Add("0161"); // Latin Small Letter S With Caron
			svWindows1252UnicodeSequences.Add("203A"); // Single Right-Pointing Angle Quotation Mark
			svWindows1252UnicodeSequences.Add("0153"); // Latin Small Ligature OE
			svWindows1252UnicodeSequences.Add("009D"); // UNASSIGNED
			svWindows1252UnicodeSequences.Add("017E"); // Latin Small Letter Z With Caron
			svWindows1252UnicodeSequences.Add("0178"); // Latin Capital Letter Y With Diaeresis
		}
		// Encodage ISO-8859-1
		else
		{
			for (i = 128; i < 160; i++)
				svWindows1252UnicodeSequences.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		}
		assert(svWindows1252UnicodeSequences.GetSize() == 160);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, avec 4 caracteres hexa
		for (i = 160; i < 256; i++)
			svWindows1252UnicodeSequences.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		assert(svWindows1252UnicodeSequences.GetSize() == 256);
	}
}

StringVector JSONFile::svWindows1252UnicodeSequences;
