// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class JsonValue;
class JsonObject;
class JsonArray;
class JsonString;
class JsonNumber;
class JsonBoolean;
class JsonNull;
class JsonMember;

#include "Object.h"
#include "ALString.h"
#include "Ermgt.h"
#include "FileService.h"
#include "PLRemoteFileService.h"
#include "JsonYac.hpp"

//////////////////////////////////////////////////////////////////////////////
// Classe JsonValue
// Classe ancetre des classe de gestion d'une valeur json
class JsonValue : public Object
{
public:
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonObject
// Valuer json de type objet
class JsonObject : public JsonValue
{
public:
	// Constructeur
	JsonObject();
	~JsonObject();

	// Lecture d'un fichier
	boolean ReadFile(const ALString& sFileName);

	// Ecriture dans un fichier
	boolean WriteFile(const ALString& sFileName) const;

	// Methodes de test
	static void TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName);
	static void Test();

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class JSONTokenizer;

	// Acces aux fonctions d'analyse lexicale generees par lex
	static void SetLineno(int nValue);
	static int GetLineno();
	static void Restart(FILE* inputFile);
	static int Lex(JSONSTYPE* jsonValue);
	static int LexDestroy();
};
