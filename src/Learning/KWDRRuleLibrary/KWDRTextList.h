// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de traitement des TextList

class KWDRTextListSize;
class KWDRTextListAt;
class KWDRTextList;
class KWDRTextListConcat;
class KWDRGetTextList;
class KWDRTableAllTexts;
class KWDRTableAllTextLists;

#include "KWDerivationRule.h"
#include "KWSymbol.h"

// Enregistrement de ces regles
void KWDRRegisterTextListRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListSize
// Nombre de Text d'une TextList
class KWDRTextListSize : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextListSize();
	~KWDRTextListSize();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListAt
// Acces a un Text d'une TextList par son index
// Redn une Text vide si l'index est invalide
class KWDRTextListAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextListAt();
	~KWDRTextListAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeTextResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextList
// Creation d'une TextList a partir d'une liste d'operandes de type Text
class KWDRTextList : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextList();
	~KWDRTextList();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Vecteur de Text en retour de la regle
	mutable SymbolVector svResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListConcat
// Concatenation d'une liste d'operandes de type TextList
class KWDRTextListConcat : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextListConcat();
	~KWDRTextListConcat();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Vecteur de Text en retour de la regle
	mutable SymbolVector svResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTextListValue
// Valeur d'un attribut TextList d'un sous-objet (TextList vide si absent)
// Le premier operande porte sur l'objet, le second sur l'attribut a renvoyer
class KWDRGetTextListValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetTextListValue();
	~KWDRGetTextListValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Vecteur de Text en retour de la regle
	mutable SymbolVector svResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAllTexts
// Concatenation des Text d'une sous-table
// Le premier operande porte sur la sous-table, le second sur l'attribut Text a collecter
class KWDRTableAllTexts : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableAllTexts();
	~KWDRTableAllTexts();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Vecteur de Text en retour de la regle
	mutable SymbolVector svResult;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAllTextLists
// Concatenation des TextList d'une sous-table
// Le premier operande porte sur la sous-table, le second sur l'attribut TextList a collecter et concatener
class KWDRTableAllTextLists : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTableAllTextLists();
	~KWDRTableAllTextLists();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Vecteur de Text en retour de la regle
	mutable SymbolVector svResult;
};
