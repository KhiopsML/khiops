// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ManagedObject.h"

////////////////////////////////////////////////////////////
// Classe Attribute
//    Attribute
class Attribute : public ManagedObject
{
public:
	// Constructeur
	Attribute();
	~Attribute();

	// Copie et duplication
	void CopyFrom(const Attribute* aSource);
	Attribute* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Rank (stored)
	int GetRank() const;
	void SetRank(int nValue);

	// Name (stored)
	const ALString& GetName() const;
	void SetName(const ALString& sValue);

	// Type (stored)
	const ALString& GetType() const;
	void SetType(const ALString& sValue);

	// Status (stored)
	const ALString& GetStatus() const;
	void SetStatus(const ALString& sValue);

	// Style (stored)
	const ALString& GetStyle() const;
	void SetStyle(const ALString& sValue);

	// Invisible (stored)
	boolean GetInvisible() const;
	void SetInvisible(boolean bValue);

	// Label (stored)
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

	// Champs de la cle
	//    Name
	const ALString& GetKey() const;

	////////////////////////////////////////////////////////
	// Fonctionnalites generiques

	// Acces a un exemplaire d'objet gere
	// Permet d'initialiser facilement un container par
	// new ManagedObjectTable(Attribute::GetManagedObjectClass())
	static Attribute* GetManagedObjectClass();

	// Liste des index des champs
	enum
	{
		Rank,
		Name,
		Type,
		Status,
		Style,
		Invisible,
		Label,
		LastField
	};

	// Nombre de champs
	int GetFieldNumber() const;

	// Acces generique aux champs, par une valeur de
	// type chaine de caracteres
	// (les champs derives sont ignores en ecriture)
	void SetFieldAt(int nFieldIndex, const char* sValue);
	const char* GetFieldAt(int nFieldIndex) const;

	// Nom d'un champ d'apres son index
	const ALString GetFieldNameAt(int nFieldIndex) const;

	// Libelle d'un champ d'apres son index
	const ALString GetFieldLabelAt(int nFieldIndex) const;

	// Nom de stockage d'un champ d'apres son index
	const ALString GetFieldStorageNameAt(int nFieldIndex) const;

	// Parametrage du separateur de champs (par defaut: '\t')
	static void SetFieldSeparator(char cValue);
	static char GetFieldSeparator();

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Consultation du status
	boolean GetDerived() const;

	// Visible
	boolean GetVisible() const;

	// Indique si l'attribut est un champs (ou un libelle)
	boolean IsField() const;
	boolean IsLabel() const;

	// Normalisation de la casse des valeurs des champs
	void NormalizeValues();

	// Methode de controle d'integrite
	boolean Check() const override;

	// Calcul de valeurs par defaut pour les attributs non
	// renseignes
	void ComputeDefaultValues();

	//////////////////////////////////////////
	// Methodes utilitaires de generation

	// Prefixe de variable associe a un type
	const ALString GetPrefix() const;

	// Type de champs d'interface utilisateur associe a un type
	const ALString GetFieldType() const;

	// Valeur par defaut associee a un type
	const ALString GetDefaultValue() const;

	// Initilisation dans un constructeur
	boolean IsConstructorInit() const;

	// Declaration du type dans une methode
	const ALString GetMethodDecl() const;

	// Declaration du type dans un getter d'attribut derive
	const ALString GetDerivedGetterType() const;

	// Convertion d'une variable de type char* vers le type
	const ALString GetCharVarToType(const ALString& sVarName) const;

	// Convertion d'une variable de type sType vers ALString
	const ALString GetTypeVarToString(const ALString& sVarName) const;

	// Convertion d'une variable de type sType vers un stream
	const ALString GetTypeVarToStream(const ALString& sVarName) const;

	// Convertion d'une variable de type sType vers un Field
	const ALString GetTypeVarToField(const ALString& sVarName) const;

	// Convertion d'un Field vers une variable de type sType
	const ALString GetFieldToTypeVar(const ALString& sVarName) const;

	// Comparaison entre 2 variables du type donne
	const ALString GetTypeVarComparison(const ALString& sFirstElem, const ALString& sSecondElem) const;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Implementation des methodes virtuelles de ManagedObject
	ManagedObject* CloneManagedObject() const;
	boolean GetFieldStored(int nFieldIndex) const;
	IntVector* GetStoredFieldIndexes() const;
	CompareFunction GetCompareKeyFunction() const;
	char GetSeparator() const;

	// Exemplaire d'objet gere
	static Attribute managedObjectClass;

	// Attributs de la classe
	static char cFieldSeparator;
	static IntVector ivStoredFieldIndexes;
	int nRank;
	ALString sName;
	ALString sType;
	ALString sStatus;
	ALString sStyle;
	boolean bInvisible;
	ALString sLabel;
};

// Fonction de comparaison sur les champs de la cle
int AttributeCompareKey(const void* first, const void* second);

// Fonctions de comparaison et d'acces aux attributs
int AttributeCompareRank(const void* first, const void* second);
const ALString AttributeGetRank(const Object* object);

////////////////////////////////////////////////////////////
// Implementations inline

inline int Attribute::GetRank() const
{
	return nRank;
}

inline void Attribute::SetRank(int nValue)
{
	nRank = nValue;
}

inline const ALString& Attribute::GetName() const
{
	return sName;
}

inline void Attribute::SetName(const ALString& sValue)
{
	sName = sValue;
}

inline const ALString& Attribute::GetType() const
{
	return sType;
}

inline void Attribute::SetType(const ALString& sValue)
{
	sType = sValue;
}

inline const ALString& Attribute::GetStatus() const
{
	return sStatus;
}

inline void Attribute::SetStatus(const ALString& sValue)
{
	sStatus = sValue;
}

inline const ALString& Attribute::GetStyle() const
{
	return sStyle;
}

inline void Attribute::SetStyle(const ALString& sValue)
{
	sStyle = sValue;
}

inline boolean Attribute::GetInvisible() const
{
	return bInvisible;
}

inline void Attribute::SetInvisible(boolean bValue)
{
	bInvisible = bValue;
}

inline const ALString& Attribute::GetLabel() const
{
	return sLabel;
}

inline void Attribute::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

inline boolean Attribute::GetDerived() const
{
	return sStatus == "Derived";
}

inline boolean Attribute::GetVisible() const
{
	return not bInvisible;
}

inline boolean Attribute::IsField() const
{
	return sType != "";
}

inline boolean Attribute::IsLabel() const
{
	return sType == "";
}
