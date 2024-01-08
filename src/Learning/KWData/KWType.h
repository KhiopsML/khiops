// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// Predeclaration des classes dont on a besoin pour compiler
// On a seulement besoin de savoir que l'on gere des pointeurs sur les objets de ces classes
// Il ne faut pas inclure les headers correspondant, qui posent des probleme de reference cyclique
class KWObject;
class KWContinuousValueBlock;
class KWSymbolValueBlock;
class KWObjectArrayValueBlock;

#include "Object.h"
#include "ALString.h"
#include "KWSymbol.h"
#include "KWContinuous.h"
#include "KWDate.h"
#include "KWTime.h"
#include "KWTimestamp.h"
#include "KWTimestampTZ.h"

// Desactivation generale de warnings pour le Visual C++
// Ce header etant inclu partout, les pragma va annuler les warning globalement
#ifdef _MSC_VER
#pragma warning(disable : 4100) // C4100: unreferenced formal parameter
#pragma warning(disable : 4702) // C4702: unreachable code (mal gere par Visual C++)
#pragma warning(disable : 4511) // C4511: le constructeur de copie n'a pas pu etre genere
#pragma warning(disable : 4512) // C4512: l'operateur d'assignation n'a pas pu etre genere
#endif                          // _MSC_VER

///////////////////////////////////////////////////////////
// Definition des types d'attributs d'une KWClass
class KWType : public SystemObject
{
public:
	// Liste des types disponibles
	enum
	{
		Symbol,                // Type symbol (valeur de type Symbol)
		Continuous,            // Type continu (valeur de type Continous (cf. KWContinuous))
		Date,                  // Type date (valeur de type Date (cf. KWDate))
		Time,                  // Type time (valeur de type Time (cf. KWTime))
		Timestamp,             // Type timestamp (valeur de type Timestamp (cf. KWTimestamp))
		TimestampTZ,           // Type timestampTZ (valeur de type TimestampTZ (cf. KWTimestampTZ))
		Text,                  // Type texte (valeur de type Symbol)
		TextList,              // Type liste de textes (valeur de type SymbolVector)
		Object,                // Type object (objet de type KWObject, gere par une KWClass)
		ObjectArray,           // Type tableau d'objects (tableau d'objets de type KWObject)
		Structure,             // Type algorithmique (objet de d'une classe specialisee, heritant de Object)
		SymbolValueBlock,      // Type bloc de valeurs Symbol
		ContinuousValueBlock,  // Type bloc de valeurs Continuous
		ObjectArrayValueBlock, // Type bloc de valeurs ObjectArray
		None,                  // Type absent deliberement, pour le non supervise
		Unknown                // Type inconnu (non valide)
	};

	// Verification de validite d'un type
	static boolean Check(int nType);

	// Conversion d'un type vers une chaine
	static const ALString ToString(int nType);

	// Conversion d'une chaine vers un type
	static int ToType(const ALString& sType);

	// Verification si un type est simple (Continuous ou Symbol)
	static boolean IsSimple(int nType);

	// Verification si un type est complexe (Date, Time, Timestamp, TimestampTZ)
	static boolean IsComplex(int nType);

	// Verification si un type est stocke (Simple, Complex ou Text)
	static boolean IsStored(int nType);

	// Verification si un type est a base de texte (Text ou TextList)
	static boolean IsTextBased(int nType);

	// Verification si un type correspond a une relation (Object ou ObjectArray)
	static boolean IsRelation(int nType);

	// Verification si un type correspond a une relation (Object ou ObjectArray)
	// ou a un bloc de type relation (ObjectArrayValueBlock)
	static boolean IsGeneralRelation(int nType);

	// Verification si un type est de donneee (Stored ou Relation)
	static boolean IsData(int nType);

	// Verification si un type est une valeur, par opposition a un bloc de valeurs
	static boolean IsValue(int nType);

	// Verification si un type est un bloc de valeurs
	static boolean IsValueBlock(int nType);

	// Type de bloc de valeur associe a un type de base
	static int GetValueBlockType(int nType);

	// Type de base associe a un bloc de valeurs
	static int GetBlockBaseType(int nType);

	// Indique si le type est un type de predicteur: (Continuous, Symbol ou None)
	static boolean IsPredictorType(int nType);

	// Libelle associe au predicteur pour un type de variable cible: classifier, regressor ou clusterer
	static const ALString GetPredictorLabel(int nType);

	// Recherche du type pour un libelle de predicteur
	static int ToPredictorType(const ALString& sPredictorType);
};

/////////////////////////////////////////////////////////
// Valeur d'un attribut d'une instance
// Correspond a l'instanciation d'un type d'attribut
// Attention: classe delicate a utiliser: il s'agit
// d'une union, dediee au stockage efficace des valeurs
// attribut de facon generique.
union KWValue
{
public:
	// Reinitialisation generique
	// Attention, il n'y a pas de constructeur, et cette methode doit etre appelee explicitement l'initialisation
	// comme pour un type simple (int, double...)
	void Init();

	///////////////////////////////////////////////
	// Acces par type de donne

	// Continuous
	void SetContinuous(Continuous cValue);
	Continuous GetContinuous() const;

	// Symbol
	// Attention aux changements de type (sinon: bugs imprevisibles...)
	//   Premiere utilisation en tant que Symbol: appeler Init au prealable,
	//     qui ici correspond a l'initialisation par le Symbol vide
	//   Derniere utilisation en tant que Symbol, avant changement de type:
	//     appeler ensuite ResetSymbol(), pour provoquer un
	//     dereferencement du dernier Symbol utilise (pour la gestion
	//     correcte de son comptage de references)
	void SetSymbol(const Symbol& sValue);
	Symbol& GetSymbol() const;

	// Reinitialisation d'une valeur Symbol
	void ResetSymbol();

	// Date
	void SetDate(Date dtValue);
	Date GetDate() const;

	// Time
	void SetTime(Time tmValue);
	Time GetTime() const;

	// Timestamp
	void SetTimestamp(Timestamp tsValue);
	Timestamp GetTimestamp() const;

	// Timestamp
	void SetTimestampTZ(TimestampTZ tstzValue);
	TimestampTZ GetTimestampTZ() const;

	// Text
	// Les valeurs de type Text sont gerees comme celle de type Symbol
	void SetText(const Symbol& sValue);
	Symbol& GetText() const;

	// Reinitialisation d'une valeur Text
	void ResetText();

	// TextList
	void SetTextList(SymbolVector* svValue);
	SymbolVector* GetTextList() const;

	// Object
	void SetObject(KWObject* kwoValue);
	KWObject* GetObject() const;

	// ObjectArray
	void SetObjectArray(ObjectArray* oaValue);
	ObjectArray* GetObjectArray() const;

	// Structure
	void SetStructure(Object* oValue);
	Object* GetStructure() const;

	// Bloc de valeurs Symbol
	void SetSymbolValueBlock(KWSymbolValueBlock* valueBlock);
	KWSymbolValueBlock* GetSymbolValueBlock() const;

	// Bloc de valeurs Continuous
	void SetContinuousValueBlock(KWContinuousValueBlock* valueBlock);
	KWContinuousValueBlock* GetContinuousValueBlock() const;

	// Bloc de valeurs ObjectArray
	void SetObjectArrayValueBlock(KWObjectArrayValueBlock* valueBlock);
	KWObjectArrayValueBlock* GetObjectArrayValueBlock() const;

	///////////////////////////////////////////////
	// Comparaison de valeurs de type simple

	int CompareContinuous(const KWValue& value) const;
	int CompareSymbol(const KWValue& value) const;
	int CompareSymbolValue(const KWValue& value) const;

	//////////////////////////////////////////////////
	// Valeur interdite
	// Les valeurs interdites peuvent etre initialisee ou testees,
	// et empechent tout acces type aux valeurs.
	// Les valeurs interdites permettent par exemple d'utiliser les valeurs avec "flag"
	// pour indiquer qu'un traitement doit etre effectuer (cf. KWObject)

	// Test des valeurs interdites (impossible) par type
	boolean IsContinuousForbidenValue() const;
	boolean IsSymbolForbidenValue() const;
	boolean IsDateForbidenValue() const;
	boolean IsTimeForbidenValue() const;
	boolean IsTimestampForbidenValue() const;
	boolean IsTimestampTZForbidenValue() const;
	boolean IsTextForbidenValue() const;
	boolean IsTextListForbidenValue() const;
	boolean IsObjectForbidenValue() const;
	boolean IsObjectArrayForbidenValue() const;
	boolean IsStructureForbidenValue() const;
	boolean IsContinuousValueBlockForbidenValue() const;
	boolean IsSymbolValueBlockForbidenValue() const;
	boolean IsObjectArrayValueBlockForbidenValue() const;

	// Initialisation a la valeur interdite
	void SetTypeForbidenValue(int nType);

	// Taille maximum des champs de type Symbol
	static const unsigned int nMaxSymbolFieldSize = 1000;

	// Test des fonctionnalites
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// L'union permet un stockage optimise des quatre type
	// de valeurs
	// Pour les Symbol, on memorise un vrai pointeur KWSymbolDataPtr,
	// et non un Symbol directement, dont la gestion automatique empeche
	// son utilisation dans une union.
	Continuous continuous;
	KWSymbolData* symbol;
	Date date;
	Time time;
	Timestamp timestamp;
	TimestampTZ timestampTZ;
	KWSymbolData* text;
	SymbolVector* textList;
	KWObject* object;
	ObjectArray* objectArray;
	KWSymbolValueBlock* symbolValueBlock;
	KWContinuousValueBlock* continuousValueBlock;
	KWObjectArrayValueBlock* objectArrayValueBlock;
	Object* structure;
};

// Valeurs interdites pour chacun des types
// (ne peuvent etre definies a l'interieur de l'union KWValue pour des raisons de syntaxes C++)
// Ces constantes ne peuvent correspondre a des valeurs reelles et sont utilisees
// essentiellement a des fins de controle
static const Continuous KWValueForbiddenContinuous = KWContinuous::GetForbiddenValue();
static KWSymbolData* const KWValueForbiddenSymbol = (KWSymbolData*)0x0001;
static KWSymbolData* const KWValueForbiddenText = (KWSymbolData*)0x0002;
static SymbolVector* const KWValueForbiddenTextList = (SymbolVector*)0x0003;
static KWObject* const KWValueForbiddenObject = (KWObject*)0x0005;
static ObjectArray* const KWValueForbiddenObjectArray = (ObjectArray*)0x0006;
static Object* const KWValueForbiddenStructure = (Object*)0x0007;
static KWSymbolValueBlock* const KWValueForbiddenSymbolValueBlock = (KWSymbolValueBlock*)0x0009;
static KWContinuousValueBlock* const KWValueForbiddenContinuousValueBlock = (KWContinuousValueBlock*)0x000A;
static KWObjectArrayValueBlock* const KWValueForbiddenObjectArrayValueBlock = (KWObjectArrayValueBlock*)0x000B;

////////////////////////////////////////////////////////
// Methodes en inline

inline boolean KWType::Check(int nType)
{
	return (0 <= nType and nType < None);
}

inline boolean KWType::IsSimple(int nType)
{
	return (nType == Continuous or nType == Symbol);
}

inline boolean KWType::IsComplex(int nType)
{
	return (nType >= Date and nType <= TimestampTZ);
}

inline boolean KWType::IsStored(int nType)
{
	if (GetLearningTextVariableMode())
		return (nType >= 0 and nType <= Text);
	else
		return (nType >= 0 and nType <= TimestampTZ);
}

inline boolean KWType::IsTextBased(int nType)
{
	if (GetLearningTextVariableMode())
		return (nType == Text or nType == TextList);
	else
		return false;
}

inline boolean KWType::IsRelation(int nType)
{
	return (nType == Object or nType == ObjectArray);
}

inline boolean KWType::IsGeneralRelation(int nType)
{
	return (nType == Object or nType == ObjectArray or nType == ObjectArrayValueBlock);
}

inline boolean KWType::IsData(int nType)
{
	return (nType >= 0 and nType <= ObjectArray);
}

inline boolean KWType::IsValue(int nType)
{
	return (nType <= Structure);
}

inline boolean KWType::IsValueBlock(int nType)
{
	return (SymbolValueBlock <= nType and nType <= ObjectArrayValueBlock);
}

inline int KWType::GetValueBlockType(int nType)
{
	if (nType == Continuous)
		return ContinuousValueBlock;
	else if (nType == Symbol)
		return SymbolValueBlock;
	else if (nType == ObjectArray)
		return ObjectArrayValueBlock;
	else
		return Unknown;
}

inline int KWType::GetBlockBaseType(int nType)
{
	if (nType == ContinuousValueBlock)
		return Continuous;
	else if (nType == SymbolValueBlock)
		return Symbol;
	else if (nType == ObjectArrayValueBlock)
		return ObjectArray;
	else
		return Unknown;
}

inline void KWValue::Init()
{
	// L'initialisation avec continuous a 0 permet une initialisation coherente (0 ou NULL)
	// de tous les types, que l'on soit en 32 ou 64 bits
	assert(sizeof(Continuous) == sizeof(KWValue));
	continuous = 0;
}

inline void KWValue::SetContinuous(Continuous cValue)
{
	require(cValue != KWValueForbiddenContinuous);
	continuous = cValue;
}

inline Continuous KWValue::GetContinuous() const
{
	ensure(continuous != KWValueForbiddenContinuous);
	return continuous;
}

inline void KWValue::SetSymbol(const Symbol& sValue)
{
	// Analogue a la methode operator=(const Symbol& sSymbol) de la classe Symbol
	if (sValue.symbolData)
		++sValue.symbolData->lRefCount;
	// Attention: gestion de la valeur interdite
	if (symbol != KWValueForbiddenSymbol and symbol and --symbol->lRefCount == 0)
		Symbol::sdSharedSymbols.RemoveSymbol(symbol);
	symbol = sValue.symbolData;
}

inline Symbol& KWValue::GetSymbol() const
{
	ensure(symbol != KWValueForbiddenSymbol);
	return *(Symbol*)&symbol;
}

inline void KWValue::ResetSymbol()
{
	// Attention: gestion de la valeur interdite
	if (symbol != KWValueForbiddenSymbol and symbol and --symbol->lRefCount == 0)
		Symbol::sdSharedSymbols.RemoveSymbol(symbol);
	symbol = NULL;
}

inline void KWValue::SetDate(Date dtValue)
{
	require(not dtValue.IsForbiddenValue());
	date = dtValue;
}

inline Date KWValue::GetDate() const
{
	ensure(not date.IsForbiddenValue());
	return date;
}

inline void KWValue::SetTime(Time tmValue)
{
	require(not tmValue.IsForbiddenValue());
	time = tmValue;
}

inline Time KWValue::GetTime() const
{
	ensure(not time.IsForbiddenValue());
	return time;
}

inline void KWValue::SetTimestamp(Timestamp tsValue)
{
	require(not tsValue.IsForbiddenValue());
	timestamp = tsValue;
}

inline Timestamp KWValue::GetTimestamp() const
{
	ensure(not timestamp.IsForbiddenValue());
	return timestamp;
}

inline void KWValue::SetTimestampTZ(TimestampTZ tstzValue)
{
	require(not tstzValue.IsForbiddenValue());
	timestampTZ = tstzValue;
}

inline TimestampTZ KWValue::GetTimestampTZ() const
{
	ensure(not timestampTZ.IsForbiddenValue());
	return timestampTZ;
}

inline void KWValue::SetText(const Symbol& sValue)
{
	// Analogue a la methode operator=(const Symbol& sSymbol) de la classe Symbol
	if (sValue.symbolData)
		++sValue.symbolData->lRefCount;
	// Attention: gestion de la valeur interdite
	if (text != KWValueForbiddenText and text and --text->lRefCount == 0)
		Symbol::sdSharedSymbols.RemoveSymbol(text);
	text = sValue.symbolData;
}

inline Symbol& KWValue::GetText() const
{
	ensure(text != KWValueForbiddenText);
	return *(Symbol*)&text;
}

inline void KWValue::ResetText()
{
	// Attention: gestion de la valeur interdite
	if (text != KWValueForbiddenText and text and --text->lRefCount == 0)
		Symbol::sdSharedSymbols.RemoveSymbol(text);
	text = NULL;
}

inline void KWValue::SetTextList(SymbolVector* svValue)
{
	require(svValue != KWValueForbiddenTextList);
	textList = svValue;
}

inline SymbolVector* KWValue::GetTextList() const
{
	ensure(textList != KWValueForbiddenTextList);
	return textList;
}

inline void KWValue::SetObject(KWObject* kwoValue)
{
	require(kwoValue != KWValueForbiddenObject);
	object = kwoValue;
}

inline KWObject* KWValue::GetObject() const
{
	ensure(object != KWValueForbiddenObject);
	return object;
}

inline void KWValue::SetObjectArray(ObjectArray* oaValue)
{
	require(oaValue != KWValueForbiddenObjectArray);
	objectArray = oaValue;
}

inline ObjectArray* KWValue::GetObjectArray() const
{
	ensure(objectArray != KWValueForbiddenObjectArray);
	return objectArray;
}

inline void KWValue::SetStructure(Object* oValue)
{
	require(oValue != KWValueForbiddenStructure);
	structure = oValue;
}

inline Object* KWValue::GetStructure() const
{
	ensure(structure != KWValueForbiddenStructure);
	return structure;
}

inline void KWValue::SetSymbolValueBlock(KWSymbolValueBlock* valueBlock)
{
	require(valueBlock != KWValueForbiddenSymbolValueBlock);
	symbolValueBlock = valueBlock;
}

inline KWSymbolValueBlock* KWValue::GetSymbolValueBlock() const
{
	ensure(symbolValueBlock != KWValueForbiddenSymbolValueBlock);
	return symbolValueBlock;
}

inline void KWValue::SetContinuousValueBlock(KWContinuousValueBlock* valueBlock)
{
	require(valueBlock != KWValueForbiddenContinuousValueBlock);
	continuousValueBlock = valueBlock;
}

inline KWContinuousValueBlock* KWValue::GetContinuousValueBlock() const
{
	ensure(continuousValueBlock != KWValueForbiddenContinuousValueBlock);
	return continuousValueBlock;
}

inline void KWValue::SetObjectArrayValueBlock(KWObjectArrayValueBlock* valueBlock)
{
	require(valueBlock != KWValueForbiddenObjectArrayValueBlock);
	objectArrayValueBlock = valueBlock;
}

inline KWObjectArrayValueBlock* KWValue::GetObjectArrayValueBlock() const
{
	ensure(objectArrayValueBlock != KWValueForbiddenObjectArrayValueBlock);
	return objectArrayValueBlock;
}

inline int KWValue::CompareContinuous(const KWValue& value) const
{
	return KWContinuous::Compare(continuous, value.continuous);
}

inline int KWValue::CompareSymbol(const KWValue& value) const
{
	// On reprend la fonction de comparaison des Symbol
	// La comparaison directe de deux KWValue est plus efficace qu'en passant par de GetSymbol().Compare(...)
	// parce que l'on evite ainsi la gestion des compteur de references des symboles
	if (symbol == value.symbol)
		return 0;
	else if (symbol > value.symbol)
		return 1;
	else
		return -1;
}

inline int KWValue::CompareSymbolValue(const KWValue& value) const
{
	if (symbol == NULL)
	{
		if (value.symbol == NULL)
			return 0;
		else
			return -1;
	}
	else
	{
		if (value.symbol == NULL)
			return 1;
		else
			return strcmp(symbol->GetString(), value.symbol->GetString());
	}
}

inline boolean KWValue::IsContinuousForbidenValue() const
{
	return continuous == KWValueForbiddenContinuous;
}

inline boolean KWValue::IsSymbolForbidenValue() const
{
	return symbol == KWValueForbiddenSymbol;
}

inline boolean KWValue::IsDateForbidenValue() const
{
	return date.IsForbiddenValue();
}

inline boolean KWValue::IsTimeForbidenValue() const
{
	return time.IsForbiddenValue();
}

inline boolean KWValue::IsTimestampForbidenValue() const
{
	return timestamp.IsForbiddenValue();
}

inline boolean KWValue::IsTimestampTZForbidenValue() const
{
	return timestampTZ.IsForbiddenValue();
}

inline boolean KWValue::IsTextForbidenValue() const
{
	return text == KWValueForbiddenText;
}

inline boolean KWValue::IsTextListForbidenValue() const
{
	return textList == KWValueForbiddenTextList;
}

inline boolean KWValue::IsObjectForbidenValue() const
{
	return object == KWValueForbiddenObject;
}

inline boolean KWValue::IsObjectArrayForbidenValue() const
{
	return objectArray == KWValueForbiddenObjectArray;
}

inline boolean KWValue::IsStructureForbidenValue() const
{
	return structure == KWValueForbiddenStructure;
}

inline boolean KWValue::IsContinuousValueBlockForbidenValue() const
{
	return continuousValueBlock == KWValueForbiddenContinuousValueBlock;
}

inline boolean KWValue::IsSymbolValueBlockForbidenValue() const
{
	return symbolValueBlock == KWValueForbiddenSymbolValueBlock;
}

inline boolean KWValue::IsObjectArrayValueBlockForbidenValue() const
{
	return objectArrayValueBlock == KWValueForbiddenObjectArrayValueBlock;
}

inline void KWValue::SetTypeForbidenValue(int nType)
{
	require(KWType::Check(nType));
	switch (nType)
	{
	case KWType::Continuous:
		continuous = KWValueForbiddenContinuous;
		break;
	case KWType::Symbol:
		// Attention a la valeur speciale, qui n'est pas vraiment un Symbol gere
		if (symbol != KWValueForbiddenSymbol)
		{
			ResetSymbol();
			symbol = KWValueForbiddenSymbol;
		}
		break;
	case KWType::Date:
		date.SetForbiddenValue();
		break;
	case KWType::Time:
		time.SetForbiddenValue();
		break;
	case KWType::Timestamp:
		timestamp.SetForbiddenValue();
		break;
	case KWType::TimestampTZ:
		timestampTZ.SetForbiddenValue();
		break;
	case KWType::Text:
		// Attention a la valeur speciale, qui n'est pas vraiment un Symbol gere
		if (text != KWValueForbiddenText)
		{
			ResetText();
			text = KWValueForbiddenText;
		}
		break;
	case KWType::TextList:
		textList = KWValueForbiddenTextList;
		break;
	case KWType::Object:
		object = KWValueForbiddenObject;
		break;
	case KWType::ObjectArray:
		objectArray = KWValueForbiddenObjectArray;
		break;
	case KWType::Structure:
		structure = KWValueForbiddenStructure;
		break;
	case KWType::SymbolValueBlock:
		symbolValueBlock = KWValueForbiddenSymbolValueBlock;
		break;
	case KWType::ContinuousValueBlock:
		continuousValueBlock = KWValueForbiddenContinuousValueBlock;
		break;
	case KWType::ObjectArrayValueBlock:
		objectArrayValueBlock = KWValueForbiddenObjectArrayValueBlock;
		break;
	}
}
