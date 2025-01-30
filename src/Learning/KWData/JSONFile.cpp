// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONFile.h"

JSONFile::JSONFile()
{
	nCurrentListLevel = 0;
	bCamelCaseKeys = false;

	// Initialisation des stats sur les caracteres encodes
	InitializeEncodingStats();

	// Initialisation des structure d'encodage
	InitializeEncodingStructures();
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

	// Initialisation des stats sur les caracteres encodes
	InitializeEncodingStats();

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

	// Exploitation des stats d'encodage: memorisation dans le fichier json et message utilisateur
	ExploitEncodingStats();

	// Fin de l'objet global
	Unindent();
	fstJSON << "\n}\n";

	// Fermeture du fichier
	bOk = FileService::CloseOutputFile(sLocalFileName, fstJSON);

	// Copie vers HDFS si necessaire
	PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);

	// Nettoyage du buffer de travail
	sStringBuffer = "";

	// Re-initialisation des stats sur les caracteres encodes
	InitializeEncodingStats();

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

void JSONFile::SetVerboseMode(boolean bValue)
{
	bVerboseMode = bValue;
}

boolean JSONFile::GetVerboseMode()
{
	return bVerboseMode;
}

void JSONFile::CStringToJsonString(const ALString& sCString, ALString& sJsonString)
{
	boolean bTrace = false;
	int nMaxValidUTF8CharLength;
	int i;
	unsigned char c;
	const char* cHexMap = "0123456789ABCDEF";
	ALString sUnicodeChars;
	int nUTF8CharLength;

	// Retaillage de la chaine json
	sJsonString.GetBufferSetLength(0);

	// Encodage de la chaine au format json
	i = 0;
	nMaxValidUTF8CharLength = false;
	while (i < sCString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = GetValidUTF8CharLengthAt(sCString, i);
		nMaxValidUTF8CharLength = max(nMaxValidUTF8CharLength, nUTF8CharLength);

		// Cas avec 0 ou un caractere valide
		if (nUTF8CharLength <= 1)
		{
			c = (unsigned char)sCString.GetAt(i);
			i++;

			// Gestion des caracteres speciaux
			switch (c)
			{
			case '"':
				sJsonString += "\\\"";
				break;
			case '\\':
				sJsonString += "\\\\";
				break;
			case '/':
				sJsonString += "\\/";
				break;
			case '\b':
				sJsonString += "\\b";
				break;
			case '\f':
				sJsonString += "\\f";
				break;
			case '\n':
				sJsonString += "\\n";
				break;
			case '\r':
				sJsonString += "\\r";
				break;
			case '\t':
				sJsonString += "\\t";
				break;
			default:
				// Caracteres de controle ansi
				if (c < 0x20)
				{
					sJsonString += "\\u00";
					sJsonString += cHexMap[c / 16];
					sJsonString += cHexMap[c % 16];
				}
				// Caracteres de ascii etendu
				else if (c >= 0x80)
				{
					Windows1252ToUnicodeHex(c, sUnicodeChars);
					sJsonString += "\\u";
					sJsonString += sUnicodeChars;
				}
				// Caractere standard
				else
					sJsonString += c;
				break;
			}
		}
		// Cas d'un catactere UTF8 multi-byte
		else if (nUTF8CharLength == 2)
		{
			sJsonString += (unsigned char)sCString.GetAt(i);
			sJsonString += (unsigned char)sCString.GetAt(i + 1);
			i += 2;
		}
		else if (nUTF8CharLength == 3)
		{
			sJsonString += (unsigned char)sCString.GetAt(i);
			sJsonString += (unsigned char)sCString.GetAt(i + 1);
			sJsonString += (unsigned char)sCString.GetAt(i + 2);
			i += 3;
		}
		else if (nUTF8CharLength == 4)
		{
			sJsonString += (unsigned char)sCString.GetAt(i);
			sJsonString += (unsigned char)sCString.GetAt(i + 1);
			sJsonString += (unsigned char)sCString.GetAt(i + 2);
			sJsonString += (unsigned char)sCString.GetAt(i + 3);
			i += 4;
		}
	}

	// Trace
	if (bTrace)
	{
		if (nMaxValidUTF8CharLength > 1 or sCString.GetLength() != sJsonString.GetLength())
			cout << sCString.GetLength() << "\t" << sJsonString.GetLength() << "\t" << sCString << "\t"
			     << sJsonString << "\tutf8 max length:" << nMaxValidUTF8CharLength << endl;
	}
}

void JSONFile::CStringToCAnsiString(const ALString& sCString, ALString& sCAnsiString)
{
	int i;
	int j;
	unsigned char c;
	int nUTF8CharLength;
	int nUtf8Code;
	int nAnsiCodeFromUtf8;

	// Retaillage de la chaine cible
	sCAnsiString.GetBufferSetLength(0);

	// Encodage de la chaine au format json
	i = 0;
	while (i < sCString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = GetValidUTF8CharLengthAt(sCString, i);

		// Cas avec 0 ou un caractere valide, ascii ou ansi
		if (nUTF8CharLength <= 1)
		{
			c = (unsigned char)sCString.GetAt(i);
			i++;
			sCAnsiString += c;
		}
		// Cas d'un caractere UTF8 multi-byte trop long pour la plage windows-1252
		else if (nUTF8CharLength == 4)
		{
			// On memorise les bytes du carectere utf8 tels quels
			for (j = 0; j < nUTF8CharLength; j++)
			{
				c = (unsigned char)sCString.GetAt(i);
				i++;
				sCAnsiString += c;
			}
		}
		// Cas d'un caractere UTF8 multi-byte pouvant etre encode sur la plage windows-1252
		else
		{
			assert(2 <= nUTF8CharLength and nUTF8CharLength <= nWindows1252EncodingMaxByteNumber);

			// Calcul du code utf8 et deplacement dans la chaine
			nUtf8Code = 0;
			for (j = 0; j < nUTF8CharLength; j++)
			{
				c = (unsigned char)sCString.GetAt(i);
				i++;
				nUtf8Code *= 256;
				nUtf8Code += c;
			}

			// Recherche de l'index du caractere dans la plage windows-1252
			nAnsiCodeFromUtf8 = Windows1252Utf8CodeToWindows1252(nUtf8Code);

			// Encodage ansi si necessaire
			if (nAnsiCodeFromUtf8 != -1)
			{
				assert(128 <= nAnsiCodeFromUtf8 and nAnsiCodeFromUtf8 < 256);
				sCAnsiString += char(nAnsiCodeFromUtf8);
			}
			// Sinon, on remet les bytes de l'encodage utf8 tels quels
			else
			{
				for (j = i - nUTF8CharLength; j < i; j++)
				{
					c = (unsigned char)sCString.GetAt(j);
					sCAnsiString += c;
				}
			}
		}
	}
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
	ALString sUtf8Chars;
	int nAnsiCode;
	int nUtf8Code;
	int nAnsiCodeFromUtf8;
	ALString sTmp;

	// Test des encodages unicode des caracteres ascii etendus
	cout << "Index\tChar\tUnicode\tUtf8\tAnsi code\tUtf8 code\tAnsi code from utf8\tValid\n";
	for (i = 0; i < 256; i++)
	{
		Windows1252ToUnicodeHex(i, sUnicodeChars);
		Windows1252ToUtf8Hex(i, sUtf8Chars);
		nAnsiCode = UnicodeHexToWindows1252(sUnicodeChars);
		nUtf8Code = GetHexStringCode(sUtf8Chars);
		nAnsiCodeFromUtf8 = Windows1252Utf8CodeToWindows1252(nUtf8Code);
		cout << i << "\t";
		if (0x20 <= i and i <= 0x7F and i != 34)
			cout << (char)i << "\t";
		else
			cout << "\t";
		cout << sUnicodeChars << "\t";
		cout << sUtf8Chars << "\t";
		cout << nAnsiCode << "\t";
		cout << nUtf8Code << "\t";
		cout << nAnsiCodeFromUtf8 << "\t";
		cout << ((i == nAnsiCode) and (i == nAnsiCodeFromUtf8)) << endl;
		assert((i == nAnsiCode) and (i == nAnsiCodeFromUtf8));
	}

	// Fichier decrivant l'encodage ansi utilise par khiops
	sFileName = FileService::BuildFilePathName(FileService::GetSystemTmpDir(), "khiops_ansi_encoding.json");
	cout << "Khiops ansi encoding file\t" << sFileName << endl;
	fJSON.SetFileName(sFileName);
	fJSON.OpenForWrite();
	if (fJSON.IsOpened())
	{
		fJSON.BeginKeyArray("khiops_ansi_encoding_chars");
		for (i = 128; i < 256; i++)
			fJSON.WriteString(sTmp + (char)i);
		fJSON.EndArray();
	}
	fJSON.Close();

	// Ouverture d'un fichier de test json
	sFileName = FileService::BuildFilePathName(FileService::GetSystemTmpDir(), "Test.json");
	cout << "JSON test file\t" << sFileName << endl;
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
	require(IsOpened());

	// Encodage de la chaine C au format json
	CStringToJsonString(sValue, sStringBuffer);
	fstJSON << '"';
	fstJSON << sStringBuffer;
	fstJSON << '"';

	// Mise a jour des stats d'encodage
	UpdateEncodingStats(sValue);
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

void JSONFile::InitializeEncodingStats()
{
	lvWindows1252AnsiCharNumbers.SetSize(256);
	lvWindows1252AnsiCharNumbers.Initialize();
	lvWindows1252Utf8CharNumbers.SetSize(256);
	lvWindows1252Utf8CharNumbers.Initialize();
	lAsciiCharNumber = 0;
	lAnsiCharNumber = 0;
	lUtf8CharNumber = 0;
}

void JSONFile::UpdateEncodingStats(const ALString& sCString)
{
	int i;
	int j;
	unsigned char c;
	int nUTF8CharLength;
	int nUtf8Code;
	int nAnsiCodeFromUtf8;

	// Encodage de la chaine au format json
	i = 0;
	while (i < sCString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = GetValidUTF8CharLengthAt(sCString, i);

		// Cas avec 0 ou un caractere valide
		if (nUTF8CharLength <= 1)
		{
			c = (unsigned char)sCString.GetAt(i);
			i++;

			// Caracteres de ascii etendu
			if (c >= 0x80)
			{
				lAnsiCharNumber++;

				// Mise a jour des stats d'encodage pour ce caractere
				lvWindows1252AnsiCharNumbers.UpgradeAt(c, 1);
			}
			// Caractere standard
			else
				lAsciiCharNumber++;
		}
		// Cas d'un caractere UTF8 multi-byte trop long pour la plage windows-1252
		else if (nUTF8CharLength == 4)
		{
			lUtf8CharNumber++;
			i += nUTF8CharLength;
		}
		// Cas d'un caractere UTF8 multi-byte pouvant etre encode sur la plage windows-1252
		else
		{
			assert(2 <= nUTF8CharLength and nUTF8CharLength <= nWindows1252EncodingMaxByteNumber);

			lUtf8CharNumber++;

			// Calcul du code utf8 et deplacement dans la chaine
			nUtf8Code = 0;
			for (j = 0; j < nUTF8CharLength; j++)
			{
				c = (unsigned char)sCString.GetAt(i);
				i++;
				nUtf8Code *= 256;
				nUtf8Code += c;
			}

			// Mise a jour des stats d'encodage pour ce caractere s'il correspond a un caractere
			// windows-1252
			nAnsiCodeFromUtf8 = Windows1252Utf8CodeToWindows1252(nUtf8Code);
			if (nAnsiCodeFromUtf8 != -1)
			{
				assert(128 <= nAnsiCodeFromUtf8 and nAnsiCodeFromUtf8 < 256);
				lvWindows1252Utf8CharNumbers.UpgradeAt(nAnsiCodeFromUtf8, 1);
			}
		}
	}
}

void JSONFile::ExploitEncodingStats()
{
	boolean bDisplay = false;
	int nCollisionNumber;
	int nDistinctWindows1252AnsiCharNumber;
	int nDistinctWindows1252Utf8CharNumber;
	int i;
	ALString sByteString;
	ALString sTmp;

	require(IsOpened());

	// Calcul du nombre de collisions
	nCollisionNumber = 0;
	nDistinctWindows1252AnsiCharNumber = 0;
	nDistinctWindows1252Utf8CharNumber = 0;
	for (i = 128; i < 256; i++)
	{
		if (lvWindows1252AnsiCharNumbers.GetAt(i) > 0 and lvWindows1252Utf8CharNumbers.GetAt(i) > 0)
			nCollisionNumber++;
		if (lvWindows1252AnsiCharNumbers.GetAt(i) > 0)
			nDistinctWindows1252AnsiCharNumber++;
		if (lvWindows1252Utf8CharNumbers.GetAt(i) > 0)
			nDistinctWindows1252Utf8CharNumber++;
	}

	// Mise a jour du json
	// Cas ascii
	if (lAnsiCharNumber == 0 and lUtf8CharNumber == 0)
	{
		WriteKeyString("khiops_encoding", "ascii");
	}
	// Cas utf8
	else if (lAnsiCharNumber == 0)
	{
		WriteKeyString("khiops_encoding", "utf8");
	}
	// Cas ansi
	else if (lUtf8CharNumber == 0)
		WriteKeyString("khiops_encoding", "ansi");
	// Cas mixed, avec des caracteres ansi et utf8, mais pas de caractere utf8 encodant un caractere ansi
	else if (nDistinctWindows1252Utf8CharNumber == 0)
	{
		assert(nDistinctWindows1252AnsiCharNumber > 0);
		WriteKeyString("khiops_encoding", "mixed_ansi_utf8");
	}
	// Cas avec a la fois des caracteres ansi et des caracteres utf8 encodant des caracteres ansi
	else
	{
		WriteKeyString("khiops_encoding", "colliding_ansi_utf8");

		// Encodage de la liste des caracteres ansi
		BeginKeyArray("ansi_chars");
		for (i = 128; i < 256; i++)
		{
			if (lvWindows1252AnsiCharNumbers.GetAt(i) > 0)
			{
				WriteString(sTmp + (char)i);
			}
		}
		EndArray();

		// Encodage de la liste des caracteres en collision, a la fois sous forme ansi et utf8
		if (nCollisionNumber > 0)
		{
			BeginKeyArray("colliding_utf8_chars");
			for (i = 128; i < 256; i++)
			{
				if (lvWindows1252AnsiCharNumbers.GetAt(i) > 0 and
				    lvWindows1252Utf8CharNumbers.GetAt(i) > 0)
				{
					HexCharStringToByteString(svWindows1252Utf8HexEncoding.GetAt(i), sByteString);
					WriteString(sByteString);
				}
			}
			EndArray();
		}

		// Warning si dans le cas colliding
		if (bVerboseMode)
			AddWarning(sTmp +
				   "Ansi characters had to be recoded in utf8 format using the Windows-1252/ISO-8859-1 "
				   "encoding," +
				   " with collisions with existing utf8 chars. " +
				   "This may limit the use of json files: see the Khiops guide for more information.");
	}

	// Affichage des stats d'encodage
	if (bDisplay)
	{
		cout << "Encoding stats\t" << GetFileName() << "\n";
		cout << "acsii\t" << lAsciiCharNumber << "\n";
		cout << "ansi\t" << lAnsiCharNumber << "\n";
		cout << "utf8\t" << lUtf8CharNumber << "\n";
		cout << "Distinct windows-1252 ansi chars\t" << nDistinctWindows1252AnsiCharNumber << "\n";
		cout << "Distinct windows-1252 utf8 chars\t" << nDistinctWindows1252Utf8CharNumber << "\n";
		cout << "Collisions\t" << nCollisionNumber << "\n";
		if (nDistinctWindows1252AnsiCharNumber + nDistinctWindows1252Utf8CharNumber > 0)
		{
			cout << "windows-1252 encoding\n";
			cout << "Index\tAnsi\tUtf8\tAnsi nb\tUtf8 nb\tCollision\n";
			for (i = 128; i < 256; i++)
			{
				if (lvWindows1252AnsiCharNumbers.GetAt(i) > 0 or
				    lvWindows1252Utf8CharNumbers.GetAt(i) > 0)
				{
					cout << i << "\t";
					cout << svWindows1252UnicodeHexEncoding.GetAt(i) << "\t";
					cout << svWindows1252Utf8HexEncoding.GetAt(i) << "\t";
					cout << lvWindows1252AnsiCharNumbers.GetAt(i) << "\t";
					cout << lvWindows1252Utf8CharNumbers.GetAt(i) << "\t";
					cout << (lvWindows1252AnsiCharNumbers.GetAt(i) > 0 and
						 lvWindows1252Utf8CharNumbers.GetAt(i) > 0)
					     << "\n";
				}
			}
		}
	}
}

boolean JSONFile::IsHexChar(char c)
{
	return ('0' <= c and c <= '9') or ('A' <= c and c <= 'F');
}

int JSONFile::GetHexCharCode(char c)
{
	require(IsHexChar(c));
	if (c >= 'A')
		return c - 'A' + 10;
	else
		return c - '0';
}

void JSONFile::HexCharStringToByteString(const ALString& sHexCharString, ALString& sByteString)
{
	int i;
	int nChar;

	require(sHexCharString.GetLength() % 2 == 0);

	// Recodage des caracteres hexa par paires en bytes
	sByteString.GetBufferSetLength(0);
	i = 0;
	while (i < sHexCharString.GetLength())
	{
		nChar = 16 * GetHexCharCode(sHexCharString.GetAt(i));
		i++;
		nChar += GetHexCharCode(sHexCharString.GetAt(i));
		i++;
		sByteString += (char)nChar;
	}
	ensure(sByteString.GetLength() == sHexCharString.GetLength() / 2);
}

int JSONFile::GetHexStringCode(const ALString& sHexString)
{
	int i;
	int nCode;
	require(sHexString.GetLength() == 2 or sHexString.GetLength() == 4 or sHexString.GetLength() == 6);

	nCode = 0;
	for (i = 0; i < sHexString.GetLength(); i++)
	{
		nCode *= 16;
		nCode += GetHexCharCode(sHexString.GetAt(i));
	}
	ensure(0 <= nCode and nCode < pow(256, nWindows1252EncodingMaxByteNumber));
	return nCode;
}

void JSONFile::Windows1252ToUtf8Hex(int nAnsiCode, ALString& sUtf8HexChars)
{
	require(0 <= nAnsiCode and nAnsiCode <= 255);
	require(AreEncodingStructuresInitialized());

	sUtf8HexChars = svWindows1252Utf8HexEncoding.GetAt(nAnsiCode);
	ensure(sUtf8HexChars.GetLength() <= nWindows1252EncodingMaxByteNumber);
}

int JSONFile::Windows1252Utf8CodeToWindows1252(int nWindows1252Utf8Code)
{
	const int nLowerC2Code = 0xC2A0;
	const int nUpperC2Code = 0xC2BF;
	const int nLowerC3Code = 0xC380;
	const int nUpperC3Code = 0xC3BF;
	int nIndex;

	require(nWindows1252Utf8Code >= 0);
	require(nWindows1252Utf8Code < pow(256, nWindows1252EncodingMaxByteNumber));
	require(AreEncodingStructuresInitialized());

	// Verification
	assert(nLowerC2Code == GetHexStringCode("C2A0"));
	assert(nUpperC2Code == GetHexStringCode("C2BF"));
	assert(nLowerC3Code == GetHexStringCode("C380"));
	assert(nUpperC3Code == GetHexStringCode("C3BF"));

	// Recherche dans la plage ascii
	if (nWindows1252Utf8Code < 0x80)
		nIndex = nWindows1252Utf8Code;
	// Recherche dans la plage C3
	else if (nLowerC3Code <= nWindows1252Utf8Code and nWindows1252Utf8Code <= nUpperC3Code)
		nIndex = nWindows1252Utf8Code - nLowerC3Code + 0xC0;
	// Recherche dans la plage C2
	else if (nLowerC2Code <= nWindows1252Utf8Code and nWindows1252Utf8Code <= nUpperC2Code)
		nIndex = nWindows1252Utf8Code - nLowerC2Code + 0xA0;
	// Recherche dans la plage des caracteres de controles
	else
	{
		// Recherche en utilisant la structure de decodage, qui permet une recherche dichotomique
		nIndex = ivsWindows1252ControlCharUtf8CodeSorter.LookupInitialIndex(nWindows1252Utf8Code);

		// Si trouve, on rajoute l'index ansi de base
		if (nIndex != -1)
			nIndex += 0x80;
	}
	ensure(nIndex == -1 or GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(nIndex)) == nWindows1252Utf8Code);
	return nIndex;
}

void JSONFile::InitializeEncodingStructures()
{
	if (not AreEncodingStructuresInitialized())
	{
		InitializeWindows1252UnicodeHexEncoding();
		InitializeWindows1252Utf8HexEncoding();
		InitializeWindows1252Utf8DecodingStructures();
		ensure(CheckEncodingStructures());
	}
	ensure(AreEncodingStructuresInitialized());
}

boolean JSONFile::AreEncodingStructuresInitialized()
{
	return svWindows1252UnicodeHexEncoding.GetSize() > 0;
}

boolean JSONFile::CheckEncodingStructures()
{
	boolean bOk = true;
	int i;
	int nUtf8Code;
	int nAnsiCode;

	// Verification des tailles des structures
	bOk = bOk and svWindows1252UnicodeHexEncoding.GetSize() == 256;
	bOk = bOk and svWindows1252Utf8HexEncoding.GetSize() == 256;
	bOk = bOk and ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 32;
	assert(bOk);

	// Verification de l'encodage unicode
	if (bOk)
	{
		for (i = 0; i < svWindows1252UnicodeHexEncoding.GetSize(); i++)
		{
			bOk = bOk and UnicodeHexToWindows1252(svWindows1252UnicodeHexEncoding.GetAt(i)) == i;
			assert(bOk);
		}
	}

	// Verification de l'encodage utf8
	if (bOk)
	{
		for (i = 0; i < svWindows1252Utf8HexEncoding.GetSize(); i++)
		{
			nUtf8Code = GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(i));
			nAnsiCode = Windows1252Utf8CodeToWindows1252(nUtf8Code);
			bOk = bOk and nAnsiCode == i;
			assert(bOk);
		}
	}
	return bOk;
}

int JSONFile::GetValidUTF8CharLengthAt(const ALString& sValue, int nStart)
{
	int nUtf8CharLength;
	int c;
	int nLength;

	require(0 <= nStart and nStart < sValue.GetLength());

	// Initialisations
	nUtf8CharLength = 0;
	nLength = sValue.GetLength();
	c = (unsigned char)sValue.GetAt(nStart);

	// Cas d'un caractere ascii 0bbbbbbb
	if (0x00 <= c and c <= 0x7f)
		nUtf8CharLength = 1;
	// Debut d'un caractere UTF8 sur deux octets 110bbbbb
	else if ((c & 0xE0) == 0xC0)
	{
		if (nStart + 1 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80)
			nUtf8CharLength = 2;
		else
			nUtf8CharLength = 0;
	}
	// Debut d'un caractere UTF8 sur trois octets 1110bbbb
	else if ((c & 0xF0) == 0xE0)
	{
		if (nStart + 2 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80)
			nUtf8CharLength = 3;
		else
			nUtf8CharLength = 0;
	}
	// Debut d'un caractere UTF8 sur quatre octets 11110bbb
	else if ((c & 0xF8) == 0xF0)
	{
		if (nStart + 3 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 3) & 0xC0) == 0x80)
			nUtf8CharLength = 4;
		else
			nUtf8CharLength = 0;
	}
	return nUtf8CharLength;
}

void JSONFile::Windows1252ToUnicodeHex(int nAnsiCode, ALString& sUnicodeHexChars)
{
	require(0 <= nAnsiCode and nAnsiCode <= 255);
	require(AreEncodingStructuresInitialized());

	sUnicodeHexChars = svWindows1252UnicodeHexEncoding.GetAt(nAnsiCode);
}

int JSONFile::UnicodeHexToWindows1252(const ALString& sUnicodeHexChars)
{
	int nCode;
	int i;

	require(sUnicodeHexChars.GetLength() == 2 or sUnicodeHexChars.GetLength() == 4);
	require(AreEncodingStructuresInitialized());

	// Cas ou les deux premiers caracteres sont encodes avec "00"
	nCode = -1;
	if (sUnicodeHexChars.GetAt(0) == '0' and sUnicodeHexChars.GetAt(1) == '0')
	{
		// Decodage du premier caractere suivant
		nCode = GetHexCharCode(sUnicodeHexChars.GetAt(2));

		// Decodage du second caractere suivant
		nCode *= 16;
		nCode += GetHexCharCode(sUnicodeHexChars.GetAt(3));
	}

	// Sinon, on recherche dans la table des caracteres speciaux
	if (nCode == -1)
	{
		for (i = 0x80; i <= 0x9F; i++)
		{
			if (sUnicodeHexChars == svWindows1252UnicodeHexEncoding.GetAt(i))
			{
				nCode = i;
				break;
			}
		}
	}
	ensure(nCode == -1 or (0 <= nCode and nCode <= 255));
	ensure(nCode == -1 or svWindows1252UnicodeHexEncoding.GetAt(nCode) == sUnicodeHexChars);
	return nCode;
}

void JSONFile::InitializeWindows1252UnicodeHexEncoding()
{
	boolean bWindows1252HexEncoding = true;

	if (svWindows1252UnicodeHexEncoding.GetSize() == 0)
	{
		const char* cHexMap = "0123456789ABCDEF";
		int i;
		ALString sUnicodePrefix = "00";
		ALString sEmpty;

		// Encodage tel quel des caracteres ascii, avec 4 caracteres hexa
		for (i = 0x00; i < 0x80; i++)
			svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres de controle ascii etendu windows-1252, avec 4 caracteres hexa
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 128);
		if (bWindows1252HexEncoding)
		{
			svWindows1252UnicodeHexEncoding.Add("20AC"); // Euro Sign
			svWindows1252UnicodeHexEncoding.Add("0081"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("201A"); // Single Low-9 Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0192"); // Latin Small Letter F With Hook
			svWindows1252UnicodeHexEncoding.Add("201E"); // Double Low-9 Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2026"); // Horizontal Ellipsis
			svWindows1252UnicodeHexEncoding.Add("2020"); // Dagger
			svWindows1252UnicodeHexEncoding.Add("2021"); // Double Dagger
			svWindows1252UnicodeHexEncoding.Add("02C6"); // Modifier Letter Circumflex Accent
			svWindows1252UnicodeHexEncoding.Add("2030"); // Per Mille Sign
			svWindows1252UnicodeHexEncoding.Add("0160"); // Latin Capital Letter S With Caron
			svWindows1252UnicodeHexEncoding.Add("2039"); // Single Left-Pointing Angle Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0152"); // Latin Capital Ligature OE
			svWindows1252UnicodeHexEncoding.Add("008D"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("017D"); // Latin Capital Letter Z With Caron
			svWindows1252UnicodeHexEncoding.Add("008F"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("0090"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("2018"); // Left Single Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2019"); // Right Single Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("201C"); // Left Double Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("201D"); // Right Double Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2022"); // Bullet
			svWindows1252UnicodeHexEncoding.Add("2013"); // En Dash
			svWindows1252UnicodeHexEncoding.Add("2014"); // Em Dash
			svWindows1252UnicodeHexEncoding.Add("02DC"); // Small Tilde
			svWindows1252UnicodeHexEncoding.Add("2122"); // Trade Mark Sign
			svWindows1252UnicodeHexEncoding.Add("0161"); // Latin Small Letter S With Caron
			svWindows1252UnicodeHexEncoding.Add("203A"); // Single Right-Pointing Angle Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0153"); // Latin Small Ligature OE
			svWindows1252UnicodeHexEncoding.Add("009D"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("017E"); // Latin Small Letter Z With Caron
			svWindows1252UnicodeHexEncoding.Add("0178"); // Latin Capital Letter Y With Diaeresis
		}
		// Encodage ISO-8859-1
		else
		{
			for (i = 128; i < 160; i++)
				svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		}
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 160);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, avec 4 caracteres hexa
		for (i = 160; i < 256; i++)
			svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 256);
	}
}

void JSONFile::InitializeWindows1252Utf8HexEncoding()
{
	boolean bWindows1252HexEncoding = true;

	if (svWindows1252Utf8HexEncoding.GetSize() == 0)
	{
		const char* cHexMap = "0123456789ABCDEF";
		int i;
		ALString sUtf8PrefixC2 = "C2";
		ALString sUtf8PrefixC3 = "C3";
		ALString sEmpty;

		// Encodage tel quel des caracteres ANSI, avec un seul byte, deux caracteres hexa
		for (i = 0; i < 0x80; i++)
			svWindows1252Utf8HexEncoding.Add(sEmpty + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres de controle ascii etendu windows-1252, avec 4 caracteres hexa
		assert(svWindows1252Utf8HexEncoding.GetSize() == 128);
		if (bWindows1252HexEncoding)
		{
			svWindows1252Utf8HexEncoding.Add("E282AC"); // Euro Sign
			svWindows1252Utf8HexEncoding.Add("C281");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("E2809A"); // Single Low-9 Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C692");   // Latin Small Letter F With Hook
			svWindows1252Utf8HexEncoding.Add("E2809E"); // Double Low-9 Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E280A6"); // Horizontal Ellipsis
			svWindows1252Utf8HexEncoding.Add("E280A0"); // Dagger
			svWindows1252Utf8HexEncoding.Add("E280A1"); // Double Dagger
			svWindows1252Utf8HexEncoding.Add("CB86");   // Modifier Letter Circumflex Accent
			svWindows1252Utf8HexEncoding.Add("E280B0"); // Per Mille Sign
			svWindows1252Utf8HexEncoding.Add("C5A0");   // Latin Capital Letter S With Caron
			svWindows1252Utf8HexEncoding.Add("E280B9"); // Single Left-Pointing Angle Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C592");   // Latin Capital Ligature OE
			svWindows1252Utf8HexEncoding.Add("C28D");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C5BD");   // Latin Capital Letter Z With Caron
			svWindows1252Utf8HexEncoding.Add("C28F");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C290");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("E28098"); // Left Single Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E28099"); // Right Single Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E2809C"); // Left Double Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E2809D"); // Right Double Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E280A2"); // Bullet
			svWindows1252Utf8HexEncoding.Add("E28093"); // En Dash
			svWindows1252Utf8HexEncoding.Add("E28094"); // Em Dash
			svWindows1252Utf8HexEncoding.Add("CB9C");   // Small Tilde
			svWindows1252Utf8HexEncoding.Add("E284A2"); // Trade Mark Sign
			svWindows1252Utf8HexEncoding.Add("C5A1");   // Latin Small Letter S With Caron
			svWindows1252Utf8HexEncoding.Add("E280BA"); // Single Right-Pointing Angle Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C593");   // Latin Small Ligature OE
			svWindows1252Utf8HexEncoding.Add("C29D");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C5BE");   // Latin Small Letter Z With Caron
			svWindows1252Utf8HexEncoding.Add("C5B8");   // Latin Capital Letter Y With Diaeresis
		}
		// Encodage ISO-8859-1
		else
		{
			for (i = 128; i < 160; i++)
				svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC2 + cHexMap[i / 16] + cHexMap[i % 16]);
		}
		assert(svWindows1252Utf8HexEncoding.GetSize() == 160);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la fin de la plage C2A0 a C2BF
		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la fin de la plage C2A0 a C2BF
		for (i = 160; i < 192; i++)
			svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC2 + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la plage C380 a C2BF
		for (i = 192; i < 256; i++)
			svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC3 + cHexMap[(i - 64) / 16] + cHexMap[i % 16]);
		assert(svWindows1252Utf8HexEncoding.GetSize() == 256);
	}
}

void JSONFile::InitializeWindows1252Utf8DecodingStructures()
{
	int i;
	int nCode;
	IntVector ivInputCodes;

	require(svWindows1252Utf8HexEncoding.GetSize() == 256);

	if (ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 0)
	{
		// Parcours des caracteres de controles de l'encodage Windows-1252 pour initialiser les codes
		// correspondant
		for (i = 0x80; i < 0xA0; i++)
		{
			nCode = GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(i));
			ivInputCodes.Add(nCode);
		}

		// Tri de ces codes pour initialiser la structure de decodage
		ivsWindows1252ControlCharUtf8CodeSorter.SortVector(&ivInputCodes);
	}
	ensure(ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 32);
}

boolean JSONFile::bVerboseMode = true;
StringVector JSONFile::svWindows1252UnicodeHexEncoding;
StringVector JSONFile::svWindows1252Utf8HexEncoding;
KWIntVectorSorter JSONFile::ivsWindows1252ControlCharUtf8CodeSorter;
JSONFile JSONFile::jsonFileGlobalInitializer;
