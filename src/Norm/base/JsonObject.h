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
#include "TextService.h"
#include "JsonYac.hpp"

//////////////////////////////////////////////////////////////////////////////
// Classe JsonValue
// Classe ancetre des classe de gestion d'une valeur json
class JsonValue : public Object
{
public:
	// Types de valeur possible
	enum
	{
		ObjectValue,
		ArrayValue,
		StringValue,
		NumberValue,
		BooleanValue,
		NullValue,
		None
	};

	// Type de valeur
	virtual int GetType() const = 0;

	// Acces type a la valeur
	JsonObject* GetObjectValue() const;
	JsonArray* GetArrayValue() const;
	JsonString* GetStringValue() const;
	JsonNumber* GetNumberValue() const;
	JsonBoolean* GetBooleanValue() const;
	JsonNull* GetNullValue() const;

	// Conversion du type en chaine de caracteres
	virtual const ALString TypeToString() const = 0;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonObject
// Valeur json de type Object
class JsonObject : public JsonValue
{
public:
	// Constructeur
	JsonObject();
	~JsonObject();

	// Type de valeur
	int GetType() const override;

	// Ajout d'un membre a l'objet
	void AddMember(JsonMember* member);

	// Supression et destruction du contenu de l'objet
	void RemoveAll();
	void DeleteAll();

	// Nombre de membres de l'objet
	int GetMemberNumber() const;

	// Acces aux membres par index
	JsonMember* GetMemberAt(int nIndex) const;

	// Acces aux membres par cle
	JsonMember* LookupMember(const ALString& sKey) const;

	// Lecture d'un fichier
	boolean ReadFile(const ALString& sFileName);

	// Ecriture dans un fichier
	boolean WriteFile(const ALString& sFileName) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;
	void WriteIndent(ostream& ost, int nIndentLevel) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	// Methodes de test
	static void TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName);
	static void Test();

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tableau et dictionnaire des membres de l'objet
	ObjectArray oaMembers;
	ObjectDictionary odMembers;

	// Tokenizer en classe friend, pour acceder a l'analyse lexicale
	friend class JSONTokenizer;

	// Acces aux fonctions d'analyse lexicale generees par lex
	static void SetLineno(int nValue);
	static int GetLineno();
	static void Restart(FILE* inputFile);
	static int Lex(JSONSTYPE* jsonValue);
	static int LexDestroy();
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonArray
// Valeur json de type Array
class JsonArray : public JsonValue
{
public:
	// Constructeur
	JsonArray();
	~JsonArray();

	// Type de valeur
	int GetType() const override;

	// Ajout d'une valeur au tableau
	void AddValue(JsonValue* value);

	// Supression et destruction du contenu du tableau
	void RemoveAll();
	void DeleteAll();

	// Nombre de valeurs du tableau
	int GetValueNumber() const;

	// Acces aux valeurs par index
	JsonValue* GetValueAt(int nIndex) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;
	void WriteIndent(ostream& ost, int nIndentLevel) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tableau des valeurs
	ObjectArray oaValues;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonString
// Valeur json de type String
class JsonString : public JsonValue
{
public:
	// Constructeur
	JsonString();
	~JsonString();

	// Type de valeur
	int GetType() const override;

	// Valeur de la chaine
	// La valeur est au format C, et non json
	void SetString(const ALString& sValue);
	const ALString& GetString() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sStringValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonNumber
// Valeur json de type Number
class JsonNumber : public JsonValue
{
public:
	// Constructeur
	JsonNumber();
	~JsonNumber();

	// Type de valeur
	int GetType() const override;

	// Valeur du nombre
	void SetNumber(double dValue);
	double GetNumber() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	double dNumberValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonBoolean
// Valeur json de type Boolean
class JsonBoolean : public JsonValue
{
public:
	// Constructeur
	JsonBoolean();
	~JsonBoolean();

	// Type de valeur
	int GetType() const override;

	// Valeur du booleen
	void SetBoolean(boolean bValue);
	boolean GetBoolean() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	boolean bBooleanValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonNull
// Valeur json de type Null
class JsonNull : public JsonValue
{
public:
	// Constructeur
	JsonNull();
	~JsonNull();

	// Type de valeur
	int GetType() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
};

//////////////////////////////////////////////////////////////////////////////
// Classe JsonMember
// Valeur json de type Member
class JsonMember : public Object
{
public:
	// Constructeur
	JsonMember();
	~JsonMember();

	// Cle associe a la valeur
	// La cle est au format C, et non json
	void SetKey(const ALString& sValue);
	const ALString& GetKey();

	// Valeur
	void SetValue(JsonValue* value);
	JsonValue* GetValue() const;

	// Type de la valeur
	int GetValueType() const;

	// Acces type a la valeur
	JsonObject* GetObjectValue() const;
	JsonArray* GetArrayValue() const;
	JsonString* GetStringValue() const;
	JsonNumber* GetNumberValue() const;
	JsonBoolean* GetBooleanValue() const;
	JsonNull* GetNullValue() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;
	void WriteIndent(ostream& ost, int nIndentLevel) const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sKey;
	JsonValue* jsonValue;
};
