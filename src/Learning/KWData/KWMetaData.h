// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWMetaData;
class KWKeyValuePair;
class PLShared_MetaData;

#include "Object.h"
#include "ALString.h"
#include "KWType.h"
#include "JSONFile.h"
#include "KWCDUniqueString.h"
#include "PLSharedObject.h"

/////////////////////////////////////////////////////////////////////
// Classe KWMetaData
// Gestion d'un ensemble de paires (cle, valeur), selon trois variante
//    . valeur numerique
//    . valeur chaine de caracteres
//    . pas de valeur (seul la presence de la cle est pertinente)
// Chaque cle existe en un seul exemplaire est est associee a une valeur numerique,
// chaine de caracteres, pas de valeur, selon son dernier parametrage
class KWMetaData : public Object
{
public:
	// Constructeur (les cles sont supprimees lors de la destruction)
	KWMetaData();
	~KWMetaData();

	// Parametrage de la valeur associee a une cle (ce qui entraine le changement de son type)
	// Le dernier parametrage ecrase le precedent
	void SetStringValueAt(const ALString& sKey, const ALString& sValue);
	void SetDoubleValueAt(const ALString& sKey, double dValue);
	void SetNoValueAt(const ALString& sKey);

	// Supression d'une cle et de sa valeur (marche meme si la cle est absente)
	void RemoveKey(const ALString& sKey);

	// Supression de toutes les cles
	void RemoveAllKeys();

	// Test de presence d'une cle
	boolean IsKeyPresent(const ALString& sKey) const;

	// Acces au type de la valeur associee a une cle
	// Si la cle n'existe pas, les methodes renvoient false
	boolean IsStringTypeAt(const ALString& sKey) const;
	boolean IsDoubleTypeAt(const ALString& sKey) const;
	boolean IsMissingTypeAt(const ALString& sKey) const;

	// Valeur associee a une cle
	// Renvoie les valeurs par defaut (0 ou "") en cas d'absence de la cle ou de mauvais type
	const ALString GetStringValueAt(const ALString& sKey) const;
	double GetDoubleValueAt(const ALString& sKey) const;

	// Valeur associee a une cle, au format externe
	// (entre doubles-quotes dans le cas du type String)
	const ALString GetExternalValueAt(const ALString& sKey) const;

	// Enumeration des cles presentes, par ordres alphabetique des cle
	int GetKeyNumber() const;
	const ALString& GetKeyAt(int nIndex) const;

	// Duplication
	KWMetaData* Clone() const;

	// Recopie
	void CopyFrom(const KWMetaData* sourceKeyValuePairs);

	// Memoire utilisee par la classe
	longint GetUsedMemory() const override;

	// Cle de hashage des meta-donnees
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier, sous la forme d'une liste de
	//   <key> dans le cas sans valeur
	//   <key=value> dans le cas numerique
	//   <key="value"> dans le cas categoriel
	void Write(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) const;

	// Test des fonctionnalites
	static void Test();

	//////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Acces a une paire (cle, valeur), avec creation et memorisation au passage en cas d'absence de la cle
	KWKeyValuePair* GetKeyValuePairAt(const ALString& sKey);

	// Recherche de l'index d'une cle dans le tableau (-1 si absent)
	int SearchKeyIndex(const ALString& sKey) const;

	// Tableau de KWKeyValuePair
	// On passe par un pointeur pour optimiser la memoire en cas de tableau vide
	// Optimisation non negligeable si l'on teint compte du nombre potentiel d'usages
	ObjectArray* oaKeyValuePairs;
	friend class PLShared_MetaData;
};

/////////////////////////////////////////////////////////////////////
// Classe KWKeyValuePair
// Gestion d'une paire (cle, valeur)
//  cle: identifiant
//  valeur: numerique, chaine de caracteres, ou pas de valeur
class KWKeyValuePair : public Object
{
public:
	// Constructeur
	KWKeyValuePair();
	~KWKeyValuePair();

	// Cle
	void SetKey(const ALString& sValue);
	const ALString& GetKey() const;

	// Verification de la validite d'une cle
	// Il s'agit d'un identifiant compose de lettres, chiffres ou '_', le premier caractere n'etant pas un chiffre
	static boolean CheckKey(const ALString& sValue);

	// Acces au type de la valeur
	// Par defaut, le type est manquant (pas de valeur)
	// Ensuite, le type est deduit de la derniere modification de valeur
	boolean IsDoubleType() const;
	boolean IsStringType() const;
	boolean IsMissingType() const;

	// Parametrage d'une valeur chaine de caractere (ce qui entraine un type chaine de caracteres)
	void SetStringValue(const ALString& sValue);

	// Acces a la valeur chaine de caractere (renvoie "" si le type est Double ou manquant)
	const ALString GetStringValue() const;

	// Parametrage d'une valeur numerique (ce qui entraine un type numerique)
	void SetDoubleValue(double dValue);

	// Acces a la valeur numerique (renvoie 0 si le type est String ou manquant)
	double GetDoubleValue() const;

	// Parametrage d'une absence de valeur (ce qui entraine un type manquant)
	void SetNoValue();

	// Duplication
	KWKeyValuePair* Clone() const;

	// Recopie
	void CopyFrom(const KWKeyValuePair* sourceKeyValuePair);

	// Cle de hashage
	longint ComputeHashValue() const;

	// Memoire utilisee par la classe
	longint GetUsedMemory() const override;

	// Affichage, ecriture dans un fichier, sous la forme
	//   <key> dans le cas sans valeur
	//   <key=value> dans le cas numerique
	//   <key="value"> dans le cas categoriel (avec doublement des eventuel caracteres double quotes internes)
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Liste des types
	enum
	{
		MissingType,
		StringType,
		DoubleType
	};

	// Cle de la meta donnee
	KWCDUniqueString usKey;

	// Valeur et type de valeur
	// On utilise une KWValue, parce qu'elle permet de stocker la valeur de facon generique et compacte
	// Et qu'elle permet de garder en un exemplaire une valeur de Symbol pour les type String, ce qui
	// permet sans risque une utilisation de la valeur dans un NumericKeyDictionary
	KWValue kwvValue;
	int nType;
	friend class PLShared_MetaData;
};

///////////////////////////////////////////////////
// Classe PLShared_MetaData
// Serialisation de la classe KWDataTableSliceSet
class PLShared_MetaData : public PLSharedObject
{
public:
	// Constructeur
	PLShared_MetaData();
	~PLShared_MetaData();

	// Acces a l'objet serialise
	void SetMetaData(KWMetaData* metaData);
	KWMetaData* GetMetaData();

	// Reimplementation des methodes virtuelles, avec transfer des specifications de la base ainsi que des index
	// d'attribut par mapping
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
