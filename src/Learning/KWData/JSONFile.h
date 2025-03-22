// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class JSONFile;

#include "Object.h"
#include "ALString.h"
#include "KWContinuous.h"
#include "FileService.h"
#include "MemoryStatsManager.h"
#include "PLRemoteFileService.h"
#include "TextService.h"

//////////////////////////////////////////
// Fichier JSON
// Service specifique aux rapports de modelisation, avec gestion du type Continuous
// et de l'encodage des fichier par une balise dediee en fin de fichier JSON
class JSONFile : public TextService
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

	// Fermeture du fichier (precede de la fin de l'objet global et d'un flush)
	//
	// Le fichier json est au format utf8, ce qui pose des problemes potentiels pour
	// les caracteres ansi (128 a 255) sui sont alors recodes en utf8 avec windows1252/iso8859-1.
	// Ces caracteres ansi sont encodes sous leur forme unicode \uXXXX dans le fichier json, ce qui
	// permet de les distinguer des caracteres utf8 latins initiaux
	// L'encodage du fichier est ecrit avant la fermeture au moyen d'un champ "khiops_encoding"
	// pouvant prendre les valeurs suivantes:
	//  . ascii: ascii uniquement
	//  . ansi: ansi, sans utf8
	//  . utf8: utf8, sans ansi
	//  . mixed_ansi_utf8: ansi et utf8, sans utf8 provenant de windows1252/iso8859-1
	//  . colliding_ansi_utf8: ansi et utf8, avec utf8 provenant de windows1252/iso8859-1
	// Seul le dernier cas pose probleme, car une fois lu depuis un json reader, il y a des collisions
	// potentielles entres les caracteres ansi initiaux recodes en utf8 avec windows1252/iso8859-1
	// et les caracteres utf8 initiaux presents dans windows1252/iso8859-1.
	// En cas de collision, les deux champs supplementaires sont ajoutes dans le json pour aide
	// au diagnostic:
	//   . ansi_chars: tableau des caracteres ansi utilises
	//   . colliding_utf8_chars: tableau des caracteres utf8 initiaux utilises dans windows1252/iso8859-1.
	boolean Close();

	// Variante de la fermeture, permetant d'ignorer la gestion de l'encodage
	boolean CloseWithoutEncodingStats();

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
	// Options avancees

	// Passage de toutes les cles au format camel case (defaut: false)
	void SetCamelCaseKeys(boolean bValue);
	boolean GetCamelCaseKeys() const;

	// Conversion d'un identifiant au format camel case
	// Les blancs sont supprimes, les mots commencent par une minuscule, sauf le premier
	static const ALString ToCamelCase(const ALString& sKey);

	// Parametrage global du mode verbeux pour les messages d'erreur ou de warning (defaut: true)
	// Ces messages sont emis notamment pour les probleme potentiels d'encodage
	static void SetVerboseMode(boolean bValue);
	static boolean GetVerboseMode();

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
	// Fermeture du fichier, en choisissant d'ignorer ou non la gestion de l'encodage
	boolean InternalClose(boolean bExploitEncodingStats);

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

	// Initialisation des statistiques d'encodage par type de caracteres
	void InitializeEncodingStats();

	// Mise a jour des stats d'encodage
	void UpdateEncodingStats(const ALString& sCString);

	// Exploitation des stats d'encodage: memorisation dans le fichier json et message utilisateur
	void ExploitEncodingStats();

	///////////////////////////////////////////////////////////////////////////////////
	// Variable d'instance

	// Nom du fichier
	ALString sFileName;      // URI specifiee par l'utilisateur (hdfs ou locale)
	ALString sLocalFileName; // Fichier local (=sFileName si fichier standard, recopie vers HDFS sinon)

	// Fichier en cours
	fstream fstJSON;

	// Niveau de liste courant
	int nCurrentListLevel;

	// Nombre d'elements dans la structure (objet au tableau) par niveau
	IntVector ivLevelElementNumber;

	// Parametrage des cles en mode camel case
	boolean bCamelCaseKeys;

	// Vecteurs de collecte de stats pour les caracteres ansi etendus (128 a 255) encodes soit en unicode,
	// soit directement presents avec leur encodage.
	// Les vecteurs sont de taille 256, mais seul les index 128 a 255 sont collectes
	LongintVector lvWindows1252AnsiCharNumbers;
	LongintVector lvWindows1252Utf8CharNumbers;

	// Stats par caracteres ascii, ansi etendu, et utf8 (hors ascii)
	longint lAsciiCharNumber;
	longint lAnsiCharNumber;
	longint lUtf8CharNumber;

	// Buffer de travail pour les encodages CString vers UTF8, pour eviter de reallouer sans arret les chaines de
	// caracteres
	ALString sStringBuffer;

	// Mode verbeux
	static boolean bVerboseMode;
};
