// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour la gestion des vecteurs de valeur

// Vecteurs de valeurs
class KWDRSymbolHashMap;
class KWDRContinuousHashMap;

// Acces a une valeur d'un vecteur par son index
class KWDRContinuousValueAtKey;
class KWDRSymbolValueAtKey;

#include "KWDerivationRule.h"
#include "KWDRVector.h"
#include "KWSortableIndex.h"

// Enregistrement de ces regles
void KWDRRegisterHashMapRules();

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolHashMap
// Regle de derivation de type Structure(HashMapC), memorisant
// l'association entre des cle (dans un regle VectorC
// en premier operande) et des valeurs symbol (dans une regle
// VectorC en second operande)
class KWDRSymbolHashMap : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolHashMap();
	~KWDRSymbolHashMap();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Recherche d'une valeur associe a une cle
	Symbol LookupValue(const Symbol& sKey);

	// Verification de la validite: les cles doivent etre toutes differentes
	boolean CheckCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Optimisation de la regle
	void Optimize();

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Dictionnaire de KWSortableSymbol pour memoriser l'association entre les cle et les index des valeurs
	NumericKeyDictionary nkdKeyIndexes;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousHashMap
// Regle de derivation de type Structure(HashMap), memorisant
// l'association entre des cle (dans un regle VectorC
// en premier operande) et des valeurs continuous (dans une regle
// DoubleVector en second operande)
class KWDRContinuousHashMap : public KWDerivationRule
{
public:
	// Constructeur
	KWDRContinuousHashMap();
	~KWDRContinuousHashMap();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Recherche d'une valeur associe a une cle
	Continuous LookupValue(const Symbol& sKey);

	// Verification de la validite: les cles doivent etre toutes differentes
	boolean CheckCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Optimisation de la regle
	void Optimize();

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Dictionnaire de KWSortableContinuous pour memoriser l'association entre les cle et les index des valeurs
	NumericKeyDictionary nkdKeyIndexes;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueAtKey
// Regle de derivation basee en un dictionnaire de valeur et une cle,
// renvoyant la valeur correspondant a la cle
// (valeur vide si index erronne)
// Premier operande: regle KWDRSymbolHashMap
// Deuxieme operande: cle
class KWDRSymbolValueAtKey : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolValueAtKey();
	~KWDRSymbolValueAtKey();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueAtKey
// Regle de derivation basee en un dictionnaire de valeur et une cle,
// renvoyant la valeur correspondant a la cle
// (valeur manquante si index erronne)
// Premier operande: regle KWDRContinuousHashMap
// Deuxieme operande: cle
class KWDRContinuousValueAtKey : public KWDerivationRule
{
public:
	// Constructeur
	KWDRContinuousValueAtKey();
	~KWDRContinuousValueAtKey();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};
