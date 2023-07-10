// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

///////////////////////////////////////////////////////////////////////////
// Regles de derivation portant sur les operations logiques
// Ces regles prennent en entree des booleen (Continuous code 0 pour false,
// et different de 0 pour true) renvoient un booleen 0 ou 1 code sous forme
// d'une valeur Continuous

class KWDRAnd;
class KWDROr;
class KWDRNot;
class KWDRSymbolIf;
class KWDRContinuousIf;
class KWDRTimeIf;
class KWDRDateIf;
class KWDRTimestampIf;
class KWDRTimestampTZIf;
class KWDRSymbolSwitch;
class KWDRContinuousSwitch;

#include "KWDerivationRule.h"

// Enregistrement de ces regles
void KWDRRegisterLogicalRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAnd
// Conjonction de un, deux ou plusieurs attributs Continuous
class KWDRAnd : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAnd();
	~KWDRAnd();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDROr
// Disjonction de un, deux ou plusieurs attributs Continuous
class KWDROr : public KWDerivationRule
{
public:
	// Constructeur
	KWDROr();
	~KWDROr();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRNot
// Negation d'un Attribut Continuous
class KWDRNot : public KWDerivationRule
{
public:
	// Constructeur
	KWDRNot();
	~KWDRNot();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolIf
// Calcul d'une valeur Symbol en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRSymbolIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolIf();
	~KWDRSymbolIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRContinuousIf
// Calcul d'une valeur Continuous en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRContinuousIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRContinuousIf();
	~KWDRContinuousIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDateIf
// Calcul d'une valeur Date en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRDateIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDateIf();
	~KWDRDateIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTimeIf
// Calcul d'une valeur Time en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRTimeIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTimeIf();
	~KWDRTimeIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTimestampIf
// Calcul d'une valeur Timestamp en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRTimestampIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTimestampIf();
	~KWDRTimestampIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTimestampTZIf
// Calcul d'une valeur TimestampTZ en fonction du resultat d'un test
// Si test positif (premier parametre), le resultat est contenu dans le
// dexuieme parametre, sinon dans le troisieme
class KWDRTimestampTZIf : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTimestampTZIf();
	~KWDRTimestampTZIf();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSymbolSwitch
//   SwitchC(Numerical index, Categorical defaultValue, Categorical ValueIndex1, Categorical valueIndex2 ...)
//
// Calcul d'une valeur Symbol en fonction d'un calcul d'index
//   . le premier operande renvoie un index, compris entre 1 et K
//   . le second operande renvoie la valeur Symbol par defaut, en cas d'index invalide
//   . les operande 3 a K-2 renvoie les operandes correspondant aux index 1 a K
class KWDRSymbolSwitch : public KWDerivationRule
{
public:
	/// Constructeur
	KWDRSymbolSwitch();
	~KWDRSymbolSwitch();

	/// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRContinuousSwitch
//   SwitchC(Numerical index, Numerical defaultValue, Numerical ValueIndex1, Numerical valueIndex2 ...)
//
// Calcul d'une valeur Continuous en fonction d'un calcul d'index
//   . le premier operande renvoie un index, compris entre 1 et K
//   . le second operande renvoie la valeur Continuous par defaut, en cas d'index invalide
//   . les operande 3 a K-2 renvoie les operandes correspondant aux index 1 a K
class KWDRContinuousSwitch : public KWDerivationRule
{
public:
	/// Constructeur
	KWDRContinuousSwitch();
	~KWDRContinuousSwitch();

	/// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};
