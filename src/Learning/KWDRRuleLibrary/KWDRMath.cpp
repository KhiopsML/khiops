// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRMath.h"

void KWDRRegisterMathRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRFormatContinuous);
	KWDerivationRule::RegisterDerivationRule(new KWDRSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRMinus);
	KWDerivationRule::RegisterDerivationRule(new KWDRDiff);
	KWDerivationRule::RegisterDerivationRule(new KWDRProduct);
	KWDerivationRule::RegisterDerivationRule(new KWDRDivide);
	KWDerivationRule::RegisterDerivationRule(new KWDRIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRRound);
	KWDerivationRule::RegisterDerivationRule(new KWDRFloor);
	KWDerivationRule::RegisterDerivationRule(new KWDRCeil);
	KWDerivationRule::RegisterDerivationRule(new KWDRAbs);
	KWDerivationRule::RegisterDerivationRule(new KWDRSign);
	KWDerivationRule::RegisterDerivationRule(new KWDRMod);
	KWDerivationRule::RegisterDerivationRule(new KWDRLog);
	KWDerivationRule::RegisterDerivationRule(new KWDRExp);
	KWDerivationRule::RegisterDerivationRule(new KWDRPower);
	KWDerivationRule::RegisterDerivationRule(new KWDRSqrt);
	KWDerivationRule::RegisterDerivationRule(new KWDRPi);
	KWDerivationRule::RegisterDerivationRule(new KWDRSin);
	KWDerivationRule::RegisterDerivationRule(new KWDRCos);
	KWDerivationRule::RegisterDerivationRule(new KWDRTan);
	KWDerivationRule::RegisterDerivationRule(new KWDRASin);
	KWDerivationRule::RegisterDerivationRule(new KWDRACos);
	KWDerivationRule::RegisterDerivationRule(new KWDRATan);
	KWDerivationRule::RegisterDerivationRule(new KWDRMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRMin);
	KWDerivationRule::RegisterDerivationRule(new KWDRMax);
	KWDerivationRule::RegisterDerivationRule(new KWDRArgMin);
	KWDerivationRule::RegisterDerivationRule(new KWDRArgMax);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFormatContinuous::KWDRFormatContinuous()
{
	SetName("FormatNumerical");
	SetLabel("String format of numerical value with number of digits before and after separator");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(2)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFormatContinuous::~KWDRFormatContinuous() {}

KWDerivationRule* KWDRFormatContinuous::Create() const
{
	return new KWDRFormatContinuous;
}

Symbol KWDRFormatContinuous::ComputeSymbolResult(const KWObject* kwoObject) const
{
	char sBuffer[200];
	Continuous cValue;
	int nWidth;
	int nPrecision;
	int nLength;

	require(IsCompiled());

	// Recherche de la valeur
	cValue = GetOperandAt(0)->GetContinuousValue(kwoObject);

	// On retourne chaine vide si valeur manquante
	if (cValue == KWContinuous::GetMissingValue())
		return Symbol();
	// Sinon, on formate la valeur
	else
	{
		// Recherche des parametres
		nWidth = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
		if (nWidth < 0)
			nWidth = 0;
		if (nWidth > 50)
			nWidth = 50;
		nPrecision = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) + 0.5);
		if (nPrecision < 0)
			nPrecision = 0;
		if (nPrecision > 50)
			nPrecision = 50;

		// Formattage de la valeur
		nLength = nWidth + 1 + nPrecision;
		if (nPrecision == 0)
			nLength--;
		if (cValue < 0)
			nLength++;
		snprintf(sBuffer, sizeof(sBuffer), "%0*.*f", nLength, nPrecision, (double)cValue);
		return (Symbol)sBuffer;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSum::KWDRSum()
{
	SetName("Sum");
	SetLabel("Sum of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRSum::~KWDRSum() {}

KWDerivationRule* KWDRSum::Create() const
{
	return new KWDRSum;
}

Continuous KWDRSum::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cResult = 0;
	Continuous cValue;
	int i;

	require(IsCompiled());

	// Calcul de la somme
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue == KWContinuous::GetMissingValue())
			return KWContinuous::GetMissingValue();
		cResult += cValue;
	}
	if (cResult == KWContinuous::GetForbiddenValue())
		cResult = KWContinuous::GetMissingValue();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMinus::KWDRMinus()
{
	SetName("Minus");
	SetLabel("Opposite value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRMinus::~KWDRMinus() {}

KWDerivationRule* KWDRMinus::Create() const
{
	return new KWDRMinus;
}

Continuous KWDRMinus::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return -cValue;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDiff::KWDRDiff()
{
	SetName("Diff");
	SetLabel("Difference between two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRDiff::~KWDRDiff() {}

KWDerivationRule* KWDRDiff::Create() const
{
	return new KWDRDiff;
}

Continuous KWDRDiff::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue1;
	Continuous cValue2;
	Continuous cResult;

	require(IsCompiled());

	// Acces aux valeurs
	cValue1 = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue1 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	cValue2 = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cValue2 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul de la difference
	cResult = cValue1 - cValue2;
	if (cResult == KWContinuous::GetForbiddenValue())
		cResult = KWContinuous::GetMissingValue();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRProduct::KWDRProduct()
{
	SetName("Product");
	SetLabel("Product of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRProduct::~KWDRProduct() {}

KWDerivationRule* KWDRProduct::Create() const
{
	return new KWDRProduct;
}

Continuous KWDRProduct::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cResult = 1;
	Continuous cValue;
	int i;

	require(IsCompiled());

	// Calcul de la somme
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue == KWContinuous::GetMissingValue())
			return KWContinuous::GetMissingValue();
		cResult *= cValue;
	}
	if (cResult == KWContinuous::GetForbiddenValue())
		cResult = KWContinuous::GetMissingValue();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDivide::KWDRDivide()
{
	SetName("Divide");
	SetLabel("Ratio of two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRDivide::~KWDRDivide() {}

KWDerivationRule* KWDRDivide::Create() const
{
	return new KWDRDivide;
}

Continuous KWDRDivide::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue1;
	Continuous cValue2;
	Continuous cResult;

	require(IsCompiled());

	// Acces aux valeurs
	cValue1 = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue1 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	cValue2 = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cValue2 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du quotient
	if (cValue2 != 0)
	{
		cResult = cValue1 / cValue2;
		if (cResult == KWContinuous::GetForbiddenValue())
			cResult = KWContinuous::GetMissingValue();
		return cResult;
	}
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRIndex::KWDRIndex()
{
	SetName("Index");
	SetLabel("Counter used for indexing records");
	SetType(KWType::Continuous);
	SetOperandNumber(0);
}

KWDRIndex::~KWDRIndex() {}

KWDerivationRule* KWDRIndex::Create() const
{
	return new KWDRIndex;
}

Continuous KWDRIndex::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(kwoObject != NULL);

	return (Continuous)kwoObject->GetCreationIndex();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRound::KWDRRound()
{
	SetName("Round");
	SetLabel("Closest integer value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRRound::~KWDRRound() {}

KWDerivationRule* KWDRRound::Create() const
{
	return new KWDRRound;
}

Continuous KWDRRound::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	if ((cValue - floor(cValue)) >= 0.5)
		return (Continuous)ceil(cValue);
	else
		return (Continuous)floor(cValue);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFloor::KWDRFloor()
{
	SetName("Floor");
	SetLabel("Largest previous integer value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRFloor::~KWDRFloor() {}

KWDerivationRule* KWDRFloor::Create() const
{
	return new KWDRFloor;
}

Continuous KWDRFloor::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return (Continuous)floor(cValue);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCeil::KWDRCeil()
{
	SetName("Ceil");
	SetLabel("Smallest following integer value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRCeil::~KWDRCeil() {}

KWDerivationRule* KWDRCeil::Create() const
{
	return new KWDRCeil;
}

Continuous KWDRCeil::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return (Continuous)ceil(cValue);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAbs::KWDRAbs()
{
	SetName("Abs");
	SetLabel("Absolute value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRAbs::~KWDRAbs() {}

KWDerivationRule* KWDRAbs::Create() const
{
	return new KWDRAbs;
}

Continuous KWDRAbs::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return cValue >= 0 ? cValue : -cValue;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSign::KWDRSign()
{
	SetName("Sign");
	SetLabel("Sign of a numerical value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRSign::~KWDRSign() {}

KWDerivationRule* KWDRSign::Create() const
{
	return new KWDRSign;
}

Continuous KWDRSign::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return cValue >= 0 ? (Continuous)1 : (Continuous)-1;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMod::KWDRMod()
{
	SetName("Mod");
	SetLabel("Modulo of two numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRMod::~KWDRMod() {}

KWDerivationRule* KWDRMod::Create() const
{
	return new KWDRMod;
}

Continuous KWDRMod::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue1;
	Continuous cValue2;
	Continuous cResult;

	require(IsCompiled());

	// Acces aux valeurs
	cValue1 = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue1 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	cValue2 = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cValue2 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du quotient
	if (cValue2 != 0)
	{
		cResult = cValue1 - cValue2 * floor(cValue1 / cValue2);
		if (cResult == KWContinuous::GetForbiddenValue())
			cResult = KWContinuous::GetMissingValue();
		return cResult;
	}
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRLog::KWDRLog()
{
	SetName("Log");
	SetLabel("Natural logarithm");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRLog::~KWDRLog() {}

KWDerivationRule* KWDRLog::Create() const
{
	return new KWDRLog;
}

Continuous KWDRLog::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	return cValue > 0 ? (Continuous)log(cValue) : KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRExp::KWDRExp()
{
	SetName("Exp");
	SetLabel("Exponential value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRExp::~KWDRExp() {}

KWDerivationRule* KWDRExp::Create() const
{
	return new KWDRExp;
}

Continuous KWDRExp::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	cResult = (Continuous)exp(cValue);
	if (cResult == KWContinuous::GetForbiddenValue())
		cResult = KWContinuous::GetMissingValue();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRPower::KWDRPower()
{
	SetName("Power");
	SetLabel("Power function");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRPower::~KWDRPower() {}

KWDerivationRule* KWDRPower::Create() const
{
	return new KWDRPower;
}

Continuous KWDRPower::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue1;
	Continuous cValue2;
	Continuous cResult;

	require(IsCompiled());

	// Acces aux valeurs
	cValue1 = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue1 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();
	cValue2 = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cValue2 == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)pow(cValue1, cValue2);
	if (cResult == KWContinuous::GetForbiddenValue())
		cResult = KWContinuous::GetMissingValue();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSqrt::KWDRSqrt()
{
	SetName("Sqrt");
	SetLabel("Square root");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRSqrt::~KWDRSqrt() {}

KWDerivationRule* KWDRSqrt::Create() const
{
	return new KWDRSqrt;
}

Continuous KWDRSqrt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue() or cValue < 0)
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)sqrt(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRPi::KWDRPi()
{
	SetName("Pi");
	SetLabel("Pi value");
	SetType(KWType::Continuous);
	SetOperandNumber(0);
}

KWDRPi::~KWDRPi() {}

KWDerivationRule* KWDRPi::Create() const
{
	return new KWDRPi;
}

Continuous KWDRPi::ComputeContinuousResult(const KWObject* kwoObject) const
{
	static const double dPi = 3.14159265358979323846;
	require(IsCompiled());
	return dPi;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSin::KWDRSin()
{
	SetName("Sin");
	SetLabel("Sine");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRSin::~KWDRSin() {}

KWDerivationRule* KWDRSin::Create() const
{
	return new KWDRSin;
}

Continuous KWDRSin::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)sin(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCos::KWDRCos()
{
	SetName("Cos");
	SetLabel("Cosine");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRCos::~KWDRCos() {}

KWDerivationRule* KWDRCos::Create() const
{
	return new KWDRCos;
}

Continuous KWDRCos::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)cos(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTan::KWDRTan()
{
	SetName("Tan");
	SetLabel("Tangent");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRTan::~KWDRTan() {}

KWDerivationRule* KWDRTan::Create() const
{
	return new KWDRTan;
}

Continuous KWDRTan::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)tan(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRASin::KWDRASin()
{
	SetName("ASin");
	SetLabel("Arc sine");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRASin::~KWDRASin() {}

KWDerivationRule* KWDRASin::Create() const
{
	return new KWDRASin;
}

Continuous KWDRASin::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue() or cValue < -1 or cValue > 1)
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)asin(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRACos::KWDRACos()
{
	SetName("ACos");
	SetLabel("Arc cosine");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRACos::~KWDRACos() {}

KWDerivationRule* KWDRACos::Create() const
{
	return new KWDRACos;
}

Continuous KWDRACos::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue() or cValue < -1 or cValue > 1)
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)acos(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRATan::KWDRATan()
{
	SetName("ATan");
	SetLabel("Arc tangent");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRATan::~KWDRATan() {}

KWDerivationRule* KWDRATan::Create() const
{
	return new KWDRATan;
}

Continuous KWDRATan::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cResult;

	require(IsCompiled());

	// Acces a la valeur
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul du resultat
	cResult = (Continuous)atan(cValue);
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMean::KWDRMean()
{
	SetName("Mean");
	SetLabel("Mean of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRMean::~KWDRMean() {}

KWDerivationRule* KWDRMean::Create() const
{
	return new KWDRMean;
}

Continuous KWDRMean::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	double dMean;
	int i;
	int nSize;

	require(IsCompiled());

	// Calcul de la moyenne
	dMean = 0;
	nSize = 0;
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			dMean += cValue;
			nSize++;
		}
	}

	// Retourne Missing si aucune valeur
	if (nSize == 0)
		return KWContinuous::GetMissingValue();
	// Retourne la moyenne sinon
	else
	{
		dMean /= nSize;
		return (Continuous)dMean;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRStandardDeviation::KWDRStandardDeviation()
{
	SetName("StdDev");
	SetLabel("Standard deviation of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRStandardDeviation::~KWDRStandardDeviation() {}

KWDerivationRule* KWDRStandardDeviation::Create() const
{
	return new KWDRStandardDeviation;
}

Continuous KWDRStandardDeviation::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	double dValue;
	double dSum;
	double dSquareSum;
	double dStandardDeviation;
	int nSize;
	int i;

	require(IsCompiled());

	// Calcul des sommes et sommes des carres des valeurs
	dSum = 0;
	dSquareSum = 0;
	nSize = 0;
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			dValue = cValue;

			// Mise a jour des sommes
			dSum += dValue;
			dSquareSum += dValue * dValue;
			nSize++;
		}
	}

	// Retourne Missing si aucune valeur
	if (nSize == 0)
		return KWContinuous::GetMissingValue();
	// Retourne l'ecart type sinon
	else
	{
		dStandardDeviation = sqrt(fabs((dSquareSum - dSum * dSum / nSize) / nSize));
		assert(dStandardDeviation >= 0);
		return (Continuous)dStandardDeviation;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMin::KWDRMin()
{
	SetName("Min");
	SetLabel("Min of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRMin::~KWDRMin() {}

KWDerivationRule* KWDRMin::Create() const
{
	return new KWDRMin;
}

Continuous KWDRMin::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cMinValue;
	int i;
	int nSize;

	require(IsCompiled());

	// Recherche de la plus petite valeur
	cMinValue = KWContinuous::GetMaxValue();
	nSize = 0;
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue < cMinValue)
				cMinValue = cValue;
			nSize++;
		}
	}
	if (nSize == 0)
		return KWContinuous::GetMissingValue();
	else
		return cMinValue;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMax::KWDRMax()
{
	SetName("Max");
	SetLabel("Max of numerical values");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRMax::~KWDRMax() {}

KWDerivationRule* KWDRMax::Create() const
{
	return new KWDRMax;
}

Continuous KWDRMax::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cMaxValue;
	int i;
	int nSize;

	require(IsCompiled());

	// Recherche de la plus grande valeur
	cMaxValue = KWContinuous::GetMinValue();
	nSize = 0;
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue > cMaxValue)
				cMaxValue = cValue;
			nSize++;
		}
	}
	if (nSize == 0)
		return KWContinuous::GetMissingValue();
	else
		return cMaxValue;
}

///////////////////////////////////////////////////////////////
// Classe KWDRArgMin

KWDRArgMin::KWDRArgMin()
{
	SetName("ArgMin");
	SetLabel("Index of the min value in a numerical serie");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRArgMin::~KWDRArgMin() {}

KWDerivationRule* KWDRArgMin::Create() const
{
	return new KWDRArgMin;
}

Continuous KWDRArgMin::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cMinValue;
	int nArgMin;
	int i;

	require(IsCompiled());

	// Recherche de la plus petite valeur
	nArgMin = -1;
	cMinValue = KWContinuous::GetMaxValue();
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue() and cValue < cMinValue)
		{
			cMinValue = cValue;
			nArgMin = i;
		}
	}

	// On retourne l'argmin si au moins une valeur non manquante
	if (nArgMin != -1)
		return (Continuous)(nArgMin + 1);
	// Sinon, on retourne la valeur manquante
	else
		return KWContinuous::GetMissingValue();
}

///////////////////////////////////////////////////////////////
// Classe KWDRArgMax

KWDRArgMax::KWDRArgMax()
{
	SetName("ArgMax");
	SetLabel("Index of the max value in a numerical serie");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRArgMax::~KWDRArgMax() {}

KWDerivationRule* KWDRArgMax::Create() const
{
	return new KWDRArgMax;
}

Continuous KWDRArgMax::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Continuous cMaxValue;
	int nArgMax;
	int i;

	require(IsCompiled());

	// Recherche de la plus grande valeur
	nArgMax = -1;
	cMaxValue = KWContinuous::GetMinValue();
	for (i = 0; i < GetOperandNumber(); i++)
	{
		cValue = GetOperandAt(i)->GetContinuousValue(kwoObject);
		if (cValue != KWContinuous::GetMissingValue() and cValue > cMaxValue)
		{
			cMaxValue = cValue;
			nArgMax = i;
		}
	}

	// On retourne l'argmax si au moins une valeur non manquante
	if (nArgMax != -1)
		return (Continuous)(nArgMax + 1);
	// Sinon, on retourne la valeur manquante
	else
		return KWContinuous::GetMissingValue();
}
