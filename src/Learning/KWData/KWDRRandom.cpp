// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRRandom.h"

void KWDRRegisterRandomRule()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRRandom);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRandom::KWDRRandom()
{
	SetName("Random");
	SetLabel("Random number betwen 0 and 1");
	SetType(KWType::Continuous);
	SetOperandNumber(0);
	lSeed = 0;
	lLeap = 0;
}

KWDRRandom::~KWDRRandom() {}

KWDerivationRule* KWDRRandom::Create() const
{
	return new KWDRRandom;
}

Continuous KWDRRandom::ComputeContinuousResult(const KWObject* kwoObject) const
{
	double dResult;
	longint lIndex;

	require(kwoObject != NULL);

	// Calcul du ieme nombre aleatoire a partir de la graine et par sauts selon l'index de l'objet
	lIndex = lSeed + lLeap * kwoObject->GetCreationIndex();
	if (lIndex < 0)
		lIndex += LLONG_MAX;
	dResult = IthRandomDouble(lIndex);
	return (Continuous)dResult;
}

void KWDRRandom::InitializeRandomParameters(const ALString& sCompiledClassName, const ALString& sAttributeName,
					    int nRuleRankInAttribute)
{
	ALString sTmp;
	ALString sSeedEncoding;
	ALString sLeapEncoding;

	require(sCompiledClassName != "");
	require(sAttributeName != "");
	require(nRuleRankInAttribute > 0);

	// Initialisation de la graine du generateur aleatoire par hashage de chaines de caractere
	// dependant du nom de la classe et du rand d'utilisation de la regle localement a la classe
	sSeedEncoding = sTmp + "Seed" + IntToString(sCompiledClassName.GetLength()) + sCompiledClassName +
			IntToString(sAttributeName.GetLength()) + sAttributeName + IntToString(nRuleRankInAttribute) +
			"Seed";
	lSeed = HashValue("MSB" + sSeedEncoding + "MSB");
	lSeed *= INT_MAX;
	sSeedEncoding.MakeReverse();
	lSeed += HashValue("LSB" + sSeedEncoding + "LSB");

	// Initialisation du saut du generateur aleatoire
	sLeapEncoding = sTmp + "Leap" + IntToString(sAttributeName.GetLength()) + sAttributeName +
			IntToString(sCompiledClassName.GetLength()) + sCompiledClassName +
			IntToString(nRuleRankInAttribute) + "Leap";
	lLeap = HashValue("MSB" + sLeapEncoding + "MSB");
	lLeap *= INT_MAX;
	sLeapEncoding.MakeReverse();
	lLeap += HashValue("LSB" + sLeapEncoding + "LSB");
	if (lLeap == 0)
		lLeap = 1;
}
