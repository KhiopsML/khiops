// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class JSONValue;
class JSONObject;
class JSONArray;
class JSONString;
class JSONNumber;
class JSONBoolean;
class JSONNull;
class JSONMember;

#include "Object.h"
#include "ALString.h"
#include "Ermgt.h"
#include "FileService.h"
#include "PLRemoteFileService.h"
#include "TextService.h"
#include "JSONYac.hpp"

//////////////////////////////////////////////////////////////////////////////
// Classe JSONValue
// Classe ancetre des classe de gestion d'une valeur json
class JSONValue : public Object
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
	JSONObject* GetObjectValue() const;
	JSONArray* GetArrayValue() const;
	JSONString* GetStringValue() const;
	JSONNumber* GetNumberValue() const;
	JSONBoolean* GetBooleanValue() const;
	JSONNull* GetNullValue() const;

	/////////////////////////////////////////////////////////////
	// Lecture/ecriture depuis un objet courant

	// Lecture depuis une chaine de caractere C dont on connait la taille
	// On emet aucun mesage d'erreur: on les memorise dans le vecteur passe en parametre
	boolean ReadString(const char* sValue, int nValueLength, StringVector* svParsingErrorMessages);

	// Lecture d'un fichier, avec emission de message d'erreur
	boolean ReadFile(const ALString& sFileName);

	// Ecriture dans un fichier
	boolean WriteFile(const ALString& sFileName) const;

	/////////////////////////////////////////////////////////////
	// Lecture en creant un objet selon le type de valeur json
	// On renvoie un objet d'une sous-classe de JSONValue,
	// ou NULL en cas d'erreur

	// Lecture globale depuis une chaine de caractere C dont on connait la taille
	// On emet aucun mesage d'erreur: on les memorise dans le vecteur passe en parametre
	static JSONValue* GlobalReadString(const char* sValue, int nValueLength, StringVector* svParsingErrorMessages);

	// Lecture globale d'un fichier, avec emission de message d'erreur
	static JSONValue* GlobalReadFile(const ALString& sFileName);

	/////////////////////////////////////////////////////////////
	// Divers

	// Conversion du type en chaine de caracteres
	virtual const ALString TypeToString() const = 0;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Affichage de facon compacte en une seule ligne
	void WriteCompact(ostream& ost) const;

	// Ecriture avec indentation et sauts de lignes si le mode pretty print est true
	virtual void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const = 0;

	// Valeur sous forme compact affichable
	const ALString BuildCompactJsonValue() const;

	// Valeur sous forme compact affichable, et tronquee si trop longue
	const ALString BuildDisplayedJsonValue() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reinitialisation
	virtual void Reset() = 0;

	// Transfert de valeur, avec reinitialisation de l'objet source
	virtual void TransferFrom(JSONValue* sourceValue) = 0;

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
// Classe JSONObject
// Valeur json de type Object
class JSONObject : public JSONValue
{
public:
	// Constructeur
	JSONObject();
	~JSONObject();

	// Type de valeur
	int GetType() const override;

	// Ajout d'un membre a l'objet
	void AddMember(JSONMember* member);

	// Supression et destruction du contenu de l'objet
	void RemoveAll();
	void DeleteAll();

	// Nombre de membres de l'objet
	int GetMemberNumber() const;

	// Acces aux membres par index
	JSONMember* GetMemberAt(int nIndex) const;

	// Acces aux membres par cle
	JSONMember* LookupMember(const ALString& sKey) const;

	// Ecriture avec indentation et sauts de lignes si le mode pretty print est true
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const override;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	// Methodes de test
	static void TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName);
	static void Test();

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;

	// Tableau et dictionnaire des membres de l'objet
	ObjectArray oaMembers;
	ObjectDictionary odMembers;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONArray
// Valeur json de type Array
class JSONArray : public JSONValue
{
public:
	// Constructeur
	JSONArray();
	~JSONArray();

	// Type de valeur
	int GetType() const override;

	// Ajout d'une valeur au tableau
	void AddValue(JSONValue* value);

	// Supression et destruction du contenu du tableau
	void RemoveAll();
	void DeleteAll();

	// Nombre de valeurs du tableau
	int GetValueNumber() const;

	// Acces aux valeurs par index
	JSONValue* GetValueAt(int nIndex) const;

	// Ecriture avec indentation et sauts de lignes si le mode pretty print est true
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;

	// Tableau des valeurs
	ObjectArray oaValues;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONString
// Valeur json de type String
class JSONString : public JSONValue
{
public:
	// Constructeur
	JSONString();
	~JSONString();

	// Type de valeur
	int GetType() const override;

	// Valeur de la chaine
	// La valeur est au format C, et non json
	void SetString(const ALString& sValue);
	const ALString& GetString() const;

	// Affichage avec options de pretty print
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;

	// Valeur
	ALString sStringValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONNumber
// Valeur json de type Number
class JSONNumber : public JSONValue
{
public:
	// Constructeur
	JSONNumber();
	~JSONNumber();

	// Type de valeur
	int GetType() const override;

	// Valeur du nombre
	void SetNumber(double dValue);
	double GetNumber() const;

	// Affichage avec options de pretty print
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;

	// Valeur
	double dNumberValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONBoolean
// Valeur json de type Boolean
class JSONBoolean : public JSONValue
{
public:
	// Constructeur
	JSONBoolean();
	~JSONBoolean();

	// Type de valeur
	int GetType() const override;

	// Valeur du booleen
	void SetBoolean(boolean bValue);
	boolean GetBoolean() const;

	// Affichage avec options de pretty print
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;

	// Valeur
	boolean bBooleanValue;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONNull
// Valeur json de type Null
class JSONNull : public JSONValue
{
public:
	// Constructeur
	JSONNull();
	~JSONNull();

	// Type de valeur
	int GetType() const override;

	// Affichage avec options de pretty print
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Conversion du type en chaine de caracteres
	const ALString TypeToString() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void Reset() override;
	void TransferFrom(JSONValue* sourceValue) override;
};

//////////////////////////////////////////////////////////////////////////////
// Classe JSONMember
// Valeur json de type Member
class JSONMember : public Object
{
public:
	// Constructeur
	JSONMember();
	~JSONMember();

	// Cle associe a la valeur
	// La cle est au format C, et non json
	void SetKey(const ALString& sValue);
	const ALString& GetKey() const;

	// Valeur
	void SetValue(JSONValue* value);
	JSONValue* GetValue() const;

	// Type de la valeur
	int GetValueType() const;

	// Acces type a la valeur
	JSONObject* GetObjectValue() const;
	JSONArray* GetArrayValue() const;
	JSONString* GetStringValue() const;
	JSONNumber* GetNumberValue() const;
	JSONBoolean* GetBooleanValue() const;
	JSONNull* GetNullValue() const;

	// Affichage avec options de pretty print
	void Write(ostream& ost) const;

	// Affichage de facon compacte en une seule ligne
	void WriteCompact(ostream& ost) const;

	// Ecriture avec indentation et sauts de lignes si le mode pretty print est true
	void WriteIndent(ostream& ost, int nIndentLevel, boolean bPrettyPrint) const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sKey;
	JSONValue* jsonValue;
};
