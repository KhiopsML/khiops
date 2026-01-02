// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "FileService.h"
#include "PLRemoteFileService.h"
#include "TextService.h"
#include "JSONObject.h"
#include "JSONYac.hpp"

//////////////////////////////////////////////////////
// Parser de fichier JSON pour en extraire les tokens
// Permet une analyse specifique de fichier json token par token,
// efficace en temps en memoire, car ne necessitant pas d'instancier
// la strcuture des objets json
// Attention: les methodes sont toutes statiques
class JSONTokenizer : public TextService
{
public:
	//////////////////////////////////////////////////////////////////////////////
	// Parametrage du fichier a analyser

	// Ouverture du fichier pour analyse
	// Le premier parametre sera utilise si necessaire pour parametrer les messages d'erreur
	static boolean OpenForRead(const ALString& sErrorFamilyName, const ALString& sInputFileName);
	static boolean IsOpened();

	// Famille d'erreur et nom du fichier, si un fichier est ouvert
	static const ALString& GetErrorFamilyName();
	static const ALString& GetFileName();

	// Fermeture du fichier
	static boolean Close();

	//////////////////////////////////////////////////////////////////////////////
	// Lecture des tokens
	// Un token peut etre soit 0, pour la fin du parsing, soit un des types de tokens
	// predefinis, soit un caractere parmi "[]{}:,"

	// Identifiant des tokens
	enum TokenType
	{
		String = STRINGVALUE,
		Number = NUMBERVALUE,
		Boolean = BOOLEANVALUE,
		Null = NULLVALUE,
		StringError = STRINGERROR,
		Error = ERROR,
	};

	// Test si un token est valide
	static boolean CheckToken(int nToken);

	// Renvoie le libelle associe a un token
	static ALString GetTokenLabel(int nToken);

	// Lecture du prochain token
	static int ReadNextToken();

	// Dernier token lu
	static int GetLastToken();

	// Numero de ligne courante
	static int GetCurrentLineIndex();

	// Acces a la valeur associee a un token
	static const ALString& GetTokenStringValue();
	static double GetTokenNumberValue();
	static boolean GetTokenBooleanValue();

	//////////////////////////////////////////////////////////////////////////////
	// Methodes de parsing
	// Ces methodes analyse une sequence courte de tokens attendues, et renvoient
	// false avec message d'erreur au cas ou les tokens ne sont pas ceux attendus

	// Lecture d'un token en precisant le token attendu
	static boolean ReadExpectedToken(int nExpectedToken);

	// Lecture d'une valeur
	static boolean ReadStringValue(ALString& sValue);
	static boolean ReadNumberValue(double& dValue);
	static boolean ReadBooleanValue(boolean& bValue);
	static boolean ReadNullValue();

	// Specialisation dans le cas d'un nombre, en verifiant si necessaire que la valeur est positive ou nul
	static boolean ReadDoubleValue(boolean bIsPositive, double& dValue);
	static boolean ReadIntValue(boolean bIsPositive, int& nValue);

	// Lecture d'un identifiant avec le token ':'
	static boolean ReadKey(const ALString& sKey);

	// Lecture d'une paire cle valeur et d'un indicateur de fin de l'objet en cours
	static boolean ReadKeyStringValue(const ALString& sKey, ALString& sValue, boolean& bIsEnd);
	static boolean ReadKeyNumberValue(const ALString& sKey, double& dValue, boolean& bIsEnd);
	static boolean ReadKeyBooleanValue(const ALString& sKey, boolean& bValue, boolean& bIsEnd);
	static boolean ReadKeyNullValue(const ALString& sKey, boolean& bIsEnd);

	// Specialisation dans le cas d'un nombre, en verifiant si necessaire que la valeur est positive ou nul
	static boolean ReadKeyDoubleValue(const ALString& sKey, boolean bIsPositive, double& dValue, boolean& bIsEnd);
	static boolean ReadKeyIntValue(const ALString& sKey, boolean bIsPositive, int& nValue, boolean& bIsEnd);

	// Lecture d'une paire cle objet ou cle tableau, en ne lisant que le debut de l'objet ou tableau ('{' ou '[')
	static boolean ReadKeyObject(const ALString& sKey);
	static boolean ReadKeyArray(const ALString& sKey);

	// Lecture de l'element suivant d'un objet ou d'un tableau
	// Le parametre bIsEnd indique que l'on est a la fin
	static boolean ReadObjectNext(boolean& bIsEnd);
	static boolean ReadArrayNext(boolean& bIsEnd);

	// Verification que l'on est a la fin d'un objet
	// Emet un message d'erreur si on est pas a la fin
	static boolean CheckObjectEnd(const ALString& sKey, boolean bIsEnd);

	// Emission d'un warning ou d'une erreur de parsing
	static void AddParseWarning(const ALString& sLabel);
	static void AddParseError(const ALString& sLabel);

	//////////////////////////////////////////////////////////////////////////////
	// Methodes standard

	// Lecture d'un fichier JSON depuis la ligne de commande
	static void TestReadJsonFile(int argc, char** argv);

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeur du dernier token entre parenthese si le token est associe a une valeur, vide sinon
	static const ALString GetLastTokenValue();

	// Conversion d'un double en entier
	// Renvoie true si la conversion est un succes
	static boolean DoubleToInt(double dValue, int& nValue);

	// Famille d'erreur a utiliser pour emettre les erreur
	static ALString sErrorFamily;

	// Nom du fichier
	static ALString sFileName;

	// Nom du fichier temporaire local (si sFileName est sur HDFS)
	static ALString sLocalFileName;

	// Fichier en d'analyse
	static FILE* fJSON;

	// Type du dernier token, pour les assertions
	static int nLastToken;

	// Valeur du dernier token
	static JSONSTYPE jsonLastTokenValue;
};
