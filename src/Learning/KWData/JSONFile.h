// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "KWContinuous.h"
#include "FileService.h"
#include "MemoryStatsManager.h"
#include "PLRemoteFileService.h"

//////////////////////////////////////////
// Fichier JSON
class JSONFile : public Object
{
public:
	// Constructeur
	JSONFile();
	~JSONFile();

	// Nom du fichier
	void SetFileName(const ALString& sValue);
	const ALString& GetFileName() const;

	// Ouverture du fichier (et debut du l'objet global)
	boolean OpenForWrite();
	boolean IsOpened();

	// fermeture du fichier (precede de la fin de l'objet global et d'un flush)
	boolean Close();

	////////////////////////////////////////////////////
	// Ecriture des structures JSON
	// Selon les specifications definies https://www.json.org/json-fr.html
	// Gestion automatiques des separateurs , dans les objets et les tableaux
	// Gestion automatique de l'indentation, selon https://codebeautify.org/jsonviewer

	// Ecriture d'une valeur, dans un tableau
	void WriteString(const ALString& sValue);
	void WriteInt(int nValue);
	void WriteLongint(longint lValue);
	void WriteDouble(double dValue);
	void WriteContinuous(Continuous cValue);
	void WriteBoolean(boolean bValue);
	void WriteNull();

	// Ecriture d'une cle et de la valeur associee, dans un objet
	void WriteKeyString(const ALString& sKey, const ALString& sValue);
	void WriteKeyInt(const ALString& sKey, int nValue);
	void WriteKeyLongint(const ALString& sKey, longint lValue);
	void WriteKeyDouble(const ALString& sKey, double dValue);
	void WriteKeyContinuous(const ALString& sKey, Continuous cValue);
	void WriteKeyBoolean(const ALString& sKey, boolean bValue);
	void WriteKeyNull(const ALString& sKey);

	// Gestion d'un objet, ensemble de paires <key: value> entre {}
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	void BeginKeyObject(const ALString& sKey);
	void BeginObject();
	void EndObject();

	// Gestion d'un tableau, ensemble d'elements (valeur, objet, tableau) entre []
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	void BeginKeyArray(const ALString& sKey);
	void BeginArray();
	void EndArray();

	// Gestion d'une liste: en fait comme un tableau, mais sur une meme ligne
	// (sans les retours a la ligne et indentations: cf. http://www.convertcsv.com/csv-to-json.htm)
	void BeginKeyList(const ALString& sKey);
	void BeginList();
	void EndList();

	////////////////////////////////////////////////////
	// Methodes avancees

	// Passage de toutes les cles au format camel case (defaut: false)
	void SetCamelCaseKeys(boolean bValue);
	boolean GetCamelCaseKeys() const;

	// Conversion d'un identifiant au format camel case
	// Les blancs sont supprimes, les mots commencent par une minuscule, sauf le premier
	static const ALString ToCamelCase(const ALString& sKey);

	// Nombre de caracteres UTF8 valides a partir d'une position donnees
	// Retourne 1 a 4 dans le cas d'un caractere valides, 0 sinon pour un caractere ANSI non encodable directement
	static int GetValidUTF8CharNumberAt(const ALString& sValue, int nStart);

	// Conversion d'un caractere asci windows-1252 vers un caractere unicode pour les caractere ascii etendus non
	// ansi Les caracteres speciaux ansi 0x20 a 0x7F sont encodes tels quel avec un caracteres Tous les autres
	// caracteres sont encode sont 4 caracteres hexas en unicode
	static void Windows1252ToUnicode(int nCode, ALString& sUnicodeChars);

	// Tentative de conversion inverse
	// Renvoie un code entre 0 et 255 si ok, -1 sinon
	static int UnicodeToWindows1252(const ALString& sUnicodeChars);

	////////////////////////////////////////////////////
	// Divers

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Ecriture d'une cle (suivi de ": ")
	void WriteKey(const ALString& sKey);

	// Ecriture d'une valeur elementaire
	void WriteStringValue(const ALString& sValue);
	void WriteIntValue(int nValue);
	void WriteLongintValue(longint lValue);
	void WriteDoubleValue(double dValue);
	void WriteContinuousValue(Continuous cValue);
	void WriteBooleanValue(boolean bValue);
	void WriteNullValue();

	// Ecriture de l'indentation courante, et du separateur d'element si necessaire
	void WriteIndent();

	// Ecriture de la fin d'un bloc (objet, tableau ou liste)
	void EndBlock(char cBlockChar, boolean bInList);

	// Gestion du niveau d'indentation
	void Indent();
	void Unindent();

	// Initialisation de la table de transcodage entre les caracteres asci windows-1252 et les caracteres unicode
	// Les caracteres speciaux 0x00 a 0x1F sont encodes en 0x00HH (4 caractere hexa)
	// Les caracteres ansi 0x20 a 0x7F sont encodes tels quels avec un seul caractere (par de sequence unicode)
	// Les caracteres speciaux 0x80 a 0x9F sont encodes de facon speciale pour avoir le meme
	// caractere imprimable qu'avec l'encodage windows-1252 (4 caractere hexa).
	// Cf. https://www.i18nqa.com/debug/table-iso8859-1-vs-windows-1252.html
	// Les caracteres latin etendus speciaux 0xA0 a 0xEF sont encodes en 0x00HH (4 caractere hexa)
	static void InitializeWindows1252UnicodeSequences();

	// Nom du fichier
	ALString sFileName;      // URI specifiee par l'utilisateur (hdfs ou locale)
	ALString sLocalFileName; // Fichier local (=sFileName si fichier standard, recopie vers HDFS sinon)

	// Fichier en cours
	fstream fstJSON;

	// Niveau de list courant
	int nCurrentListLevel;

	// Nombre d'elements dans la structure (objet au tableau) par niveau
	IntVector ivLevelElementNumber;

	// Parametrage des cles en mode camel case
	boolean bCamelCaseKeys;

	// Table de transcodage entre les caracteres asci windows-1252 et les caracteres unicode
	// pour les caracteres de controle entre 128 et 159
	static StringVector svWindows1252UnicodeSequences;
};