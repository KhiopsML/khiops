// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

///////////////////////////////////////////////////////////////////////////
// Regles de derivation portant sur les comparaisons d'attributs
// Continous ou Symbol.
// Ces regles renvoient un booleen 0 ou 1 code sous forme d'une valeur Continuous

class KWDREQ;
class KWDRNEQ;
class KWDRG;
class KWDRGE;
class KWDRL;
class KWDRLE;

class KWDRSymbolEQ;
class KWDRSymbolNEQ;
class KWDRSymbolG;
class KWDRSymbolGE;
class KWDRSymbolL;
class KWDRSymbolLE;

#include "KWDerivationRule.h"

// Enregistrement de ces regles
void KWDRRegisterCompareRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDREQ
// Egalite de deux attributs Continuous
class KWDREQ : public KWDerivationRule
{
public:
	// Constructeur
	KWDREQ();
	~KWDREQ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRNEQ
// Inegalite de deux attributs Continuous
class KWDRNEQ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRNEQ();
	~KWDRNEQ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRG
// Comparaison > de deux attributs Continuous
class KWDRG : public KWDerivationRule
{
public:
	// Constructeur
	KWDRG();
	~KWDRG();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGE
// Comparaison >= de deux attributs Continuous
class KWDRGE : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGE();
	~KWDRGE();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRL
// Comparaison < de deux attributs Continuous
class KWDRL : public KWDerivationRule
{
public:
	// Constructeur
	KWDRL();
	~KWDRL();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRLE
// Comparaison <= de deux attributs Continuous
class KWDRLE : public KWDerivationRule
{
public:
	// Constructeur
	KWDRLE();
	~KWDRLE();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolEQ
// Egalite de deux attributs Symbol
class KWDRSymbolEQ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolEQ();
	~KWDRSymbolEQ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolNEQ
// Inegalite de deux attributs Symbol
class KWDRSymbolNEQ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolNEQ();
	~KWDRSymbolNEQ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolG
// Comparaison > de deux attributs Symbol
class KWDRSymbolG : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolG();
	~KWDRSymbolG();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolGE
// Comparaison >= de deux attributs Symbol
class KWDRSymbolGE : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolGE();
	~KWDRSymbolGE();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolL
// Comparaison < de deux attributs Symbol
class KWDRSymbolL : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolL();
	~KWDRSymbolL();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolLE
// Comparaison <= de deux attributs Symbol
class KWDRSymbolLE : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolLE();
	~KWDRSymbolLE();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};
