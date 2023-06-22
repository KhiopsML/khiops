// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRCompare.h"

void KWDRRegisterCompareRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDREQ);
	KWDerivationRule::RegisterDerivationRule(new KWDRNEQ);
	KWDerivationRule::RegisterDerivationRule(new KWDRG);
	KWDerivationRule::RegisterDerivationRule(new KWDRGE);
	KWDerivationRule::RegisterDerivationRule(new KWDRL);
	KWDerivationRule::RegisterDerivationRule(new KWDRLE);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolEQ);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolNEQ);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolG);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolGE);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolL);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolLE);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDREQ::KWDREQ()
{
	SetName("EQ");
	SetLabel("Equality test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDREQ::~KWDREQ() {}

KWDerivationRule* KWDREQ::Create() const
{
	return new KWDREQ;
}

Continuous KWDREQ::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) == GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRNEQ::KWDRNEQ()
{
	SetName("NEQ");
	SetLabel("Inequality test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRNEQ::~KWDRNEQ() {}

KWDerivationRule* KWDRNEQ::Create() const
{
	return new KWDRNEQ;
}

Continuous KWDRNEQ::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) != GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRG::KWDRG()
{
	SetName("G");
	SetLabel("Greater than test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRG::~KWDRG() {}

KWDerivationRule* KWDRG::Create() const
{
	return new KWDRG;
}

Continuous KWDRG::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) > GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRGE::KWDRGE()
{
	SetName("GE");
	SetLabel("Greater than or equal test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRGE::~KWDRGE() {}

KWDerivationRule* KWDRGE::Create() const
{
	return new KWDRGE;
}

Continuous KWDRGE::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) >= GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRL::KWDRL()
{
	SetName("L");
	SetLabel("Less than test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRL::~KWDRL() {}

KWDerivationRule* KWDRL::Create() const
{
	return new KWDRL;
}

Continuous KWDRL::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) < GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRLE::KWDRLE()
{
	SetName("LE");
	SetLabel("Less than or equal test between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRLE::~KWDRLE() {}

KWDerivationRule* KWDRLE::Create() const
{
	return new KWDRLE;
}

Continuous KWDRLE::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject) <= GetSecondOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolEQ::KWDRSymbolEQ()
{
	SetName("EQc");
	SetLabel("Equality test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolEQ::~KWDRSymbolEQ() {}

KWDerivationRule* KWDRSymbolEQ::Create() const
{
	return new KWDRSymbolEQ;
}

Continuous KWDRSymbolEQ::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	// La comparaison entre Symbol est plus rapide, et identique a la comparaison
	// des valeurs chaines de caracteres
	return (Continuous)(GetFirstOperand()->GetSymbolValue(kwoObject) ==
			    GetSecondOperand()->GetSymbolValue(kwoObject));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolNEQ::KWDRSymbolNEQ()
{
	SetName("NEQc");
	SetLabel("Inequality test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolNEQ::~KWDRSymbolNEQ() {}

KWDerivationRule* KWDRSymbolNEQ::Create() const
{
	return new KWDRSymbolNEQ;
}

Continuous KWDRSymbolNEQ::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	// La comparaison entre Symbol est plus rapide, et identique a la comparaison
	// des valeurs chaines de caracteres
	return (Continuous)(GetFirstOperand()->GetSymbolValue(kwoObject) !=
			    GetSecondOperand()->GetSymbolValue(kwoObject));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolG::KWDRSymbolG()
{
	SetName("Gc");
	SetLabel("Greater than test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolG::~KWDRSymbolG() {}

KWDerivationRule* KWDRSymbolG::Create() const
{
	return new KWDRSymbolG;
}

Continuous KWDRSymbolG::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject).CompareValue(
		   GetSecondOperand()->GetSymbolValue(kwoObject)) > 0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolGE::KWDRSymbolGE()
{
	SetName("GEc");
	SetLabel("Greater than or equal test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolGE::~KWDRSymbolGE() {}

KWDerivationRule* KWDRSymbolGE::Create() const
{
	return new KWDRSymbolGE;
}

Continuous KWDRSymbolGE::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject).CompareValue(
		   GetSecondOperand()->GetSymbolValue(kwoObject)) >= 0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolL::KWDRSymbolL()
{
	SetName("Lc");
	SetLabel("Less than test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolL::~KWDRSymbolL() {}

KWDerivationRule* KWDRSymbolL::Create() const
{
	return new KWDRSymbolL;
}

Continuous KWDRSymbolL::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject).CompareValue(
		   GetSecondOperand()->GetSymbolValue(kwoObject)) < 0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolLE::KWDRSymbolLE()
{
	SetName("LEc");
	SetLabel("Less than or equal test between two categorical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolLE::~KWDRSymbolLE() {}

KWDerivationRule* KWDRSymbolLE::Create() const
{
	return new KWDRSymbolLE;
}

Continuous KWDRSymbolLE::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject).CompareValue(
		   GetSecondOperand()->GetSymbolValue(kwoObject)) <= 0;
}