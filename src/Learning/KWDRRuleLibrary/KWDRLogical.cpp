// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRLogical.h"

void KWDRRegisterLogicalRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRAnd);
	KWDerivationRule::RegisterDerivationRule(new KWDROr);
	KWDerivationRule::RegisterDerivationRule(new KWDRNot);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolIf);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousIf);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolSwitch);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousSwitch);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAnd::KWDRAnd()
{
	SetName("And");
	SetLabel("And logical operator");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRAnd::~KWDRAnd() {}

KWDerivationRule* KWDRAnd::Create() const
{
	return new KWDRAnd;
}

Continuous KWDRAnd::ComputeContinuousResult(const KWObject* kwoObject) const
{
	int i;

	require(IsCompiled());

	// Calcul de la conjonction
	for (i = 0; i < GetOperandNumber(); i++)
	{
		if (GetOperandAt(i)->GetContinuousValue(kwoObject) == (Continuous)0.0)
			return (Continuous)0.0;
	}
	return (Continuous)1.0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDROr::KWDROr()
{
	SetName("Or");
	SetLabel("Or logical operator");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDROr::~KWDROr() {}

KWDerivationRule* KWDROr::Create() const
{
	return new KWDROr;
}

Continuous KWDROr::ComputeContinuousResult(const KWObject* kwoObject) const
{
	int i;

	require(IsCompiled());

	// Calcul de la conjonction
	for (i = 0; i < GetOperandNumber(); i++)
	{
		if (GetOperandAt(i)->GetContinuousValue(kwoObject) != (Continuous)0)
			return (Continuous)1.0;
	}
	return (Continuous)0.0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRNot::KWDRNot()
{
	SetName("Not");
	SetLabel("Not logical operator");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRNot::~KWDRNot() {}

KWDerivationRule* KWDRNot::Create() const
{
	return new KWDRNot;
}

Continuous KWDRNot::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return not GetFirstOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolIf::KWDRSymbolIf()
{
	SetName("IfC");
	SetLabel("Ternary operator returning second operand (true) or third operand (false) according to the condition "
		 "in first operand");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KWDRSymbolIf::~KWDRSymbolIf() {}

KWDerivationRule* KWDRSymbolIf::Create() const
{
	return new KWDRSymbolIf;
}

Symbol KWDRSymbolIf::ComputeSymbolResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return (GetOperandAt(0)->GetContinuousValue(kwoObject) ? GetOperandAt(1)->GetSymbolValue(kwoObject)
							       : GetOperandAt(2)->GetSymbolValue(kwoObject));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRContinuousIf::KWDRContinuousIf()
{
	SetName("If");
	SetLabel("Ternary operator returning second operand (true) or third operand (false) according to the condition "
		 "in first operand");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRContinuousIf::~KWDRContinuousIf() {}

KWDerivationRule* KWDRContinuousIf::Create() const
{
	return new KWDRContinuousIf;
}

Continuous KWDRContinuousIf::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return (GetOperandAt(0)->GetContinuousValue(kwoObject) ? GetOperandAt(1)->GetContinuousValue(kwoObject)
							       : GetOperandAt(2)->GetContinuousValue(kwoObject));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSymbolSwitch::KWDRSymbolSwitch()
{
	SetName("SwitchC");
	SetLabel("Switch operator returning a categorical value according to the index in first operand");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous); // index
	GetSecondOperand()->SetType(
	    KWType::Symbol); // default value if index out of boundaries, or no boundaries specified
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KWDRSymbolSwitch::~KWDRSymbolSwitch() {}

KWDerivationRule* KWDRSymbolSwitch::Create() const
{
	return new KWDRSymbolSwitch;
}

Symbol KWDRSymbolSwitch::ComputeSymbolResult(const KWObject* kwoObject) const
{
	int nIndex;

	require(IsCompiled());

	// Calcul de l'index
	nIndex = (int)floor(GetOperandAt(0)->GetContinuousValue(kwoObject) + 0.5);

	// Valeur si index valide
	if (1 <= nIndex and nIndex <= GetOperandNumber() - 2)
		return GetOperandAt(nIndex + 1)->GetSymbolValue(kwoObject);
	// Value par defaut sinon
	else
		return GetSecondOperand()->GetSymbolValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRContinuousSwitch::KWDRContinuousSwitch()
{
	SetName("Switch");
	SetLabel("Switch operator returning a numerical value according to the index in first operand");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous); // index
	GetSecondOperand()->SetType(
	    KWType::Continuous); // default value if index out of boundaries, or no boundaries specified
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRContinuousSwitch::~KWDRContinuousSwitch() {}

KWDerivationRule* KWDRContinuousSwitch::Create() const
{
	return new KWDRContinuousSwitch;
}

Continuous KWDRContinuousSwitch::ComputeContinuousResult(const KWObject* kwoObject) const
{
	int nIndex;

	require(IsCompiled());

	// Calcul de l'index
	nIndex = (int)floor(GetOperandAt(0)->GetContinuousValue(kwoObject) + 0.5);

	// Valeur si index valide
	if (1 <= nIndex and nIndex <= GetOperandNumber() - 2)
		return GetOperandAt(nIndex + 1)->GetContinuousValue(kwoObject);
	// Value par defaut sinon
	else
		return GetSecondOperand()->GetContinuousValue(kwoObject);
}