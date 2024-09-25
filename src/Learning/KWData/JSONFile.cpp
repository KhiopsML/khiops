// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JSONFile.h"

JSONFile::JSONFile()
{
	nCurrentListLevel = 0;
	bCamelCaseKeys = false;

	// Initialisation des stats sur les caracteres encodes
	InitializeEncodingStats();
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
	return InternalClose(true);
}

boolean JSONFile::CloseWithoutEncodingStats()
{
	return InternalClose(false);
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
	ALString sTmp;

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

boolean JSONFile::InternalClose(boolean bExploitEncodingStats)
{
	boolean bOk;

	require(IsOpened());

	// Exploitation des stats d'encodage: memorisation dans le fichier json et message utilisateur
	if (bExploitEncodingStats)
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
void JSONFile::WriteKey(const ALString& sKey)
{
	require(IsOpened());
	require(sKey != "");
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
	CToJsonString(sValue, sStringBuffer);
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

			// Mise a jour des stats d'encodage pour ce caractere s'il correspond a un caractere windows-1252
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
					sByteString = HexCharStringToByteString(svWindows1252Utf8HexEncoding.GetAt(i));
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

boolean JSONFile::bVerboseMode = true;
