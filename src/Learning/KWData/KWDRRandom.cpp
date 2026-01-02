// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	nSeed = 0;
	nLeap = 0;
}

KWDRRandom::~KWDRRandom() {}

KWDerivationRule* KWDRRandom::Create() const
{
	return new KWDRRandom;
}

Continuous KWDRRandom::ComputeContinuousResult(const KWObject* kwoObject) const
{
	const boolean bTrace = false;
	const ulongint ulLargePrime = 100000007;
	const KWObjectDataPath* objectDataPath;
	ulongint ulSeed;
	ulongint ulLeap;
	ulongint ulGlobalCreationIndex;
	ulongint ulRandomIndex;
	longint lRandomIndex;
	double dResult;

	require(kwoObject != NULL);

	/////////////////////////////////////////////////////////////////////////////
	// Les calculs intermediaires sont effectues avec des entiers non signes, dont
	// l'arithmetique effectuee modulo 2^64 est garantiee par la norme C++.
	// On evite ainsi les entiers signes, pour lesquels les depassements (overflow)
	// ont un comportement indetermine, potentiellement differents selon le compilateur

	// Acces au data path de l'objet
	objectDataPath = kwoObject->GetObjectDataPath();

	// Calcul d'index de creation d'index global pour les objets crees par des regles de creation d'instance
	assert(kwoObject->GetCreationIndex() >= 0);
	assert(objectDataPath->GetMainCreationIndex() >= 0);
	if (objectDataPath->GetCreatedObjects())
	{
		// On calcul un index unique localement a chaque instance principale, en multipliant son index
		// par un nombre premier superieur a 100 millions, ce qui comme 2^64 = 1.8 x 10^19 permet d'assure
		// un identifiant unique jusqu'a environ 180 milliards d'instances principales lues depuis des fichier, et jusqu'a
		// 2^27 ~ 128 millions d'instances creees par instance principale
		ulGlobalCreationIndex = (ulongint)objectDataPath->GetMainCreationIndex();
		ulGlobalCreationIndex *= ulLargePrime;

		// On ajoute l'index local a l'objet cree
		ulGlobalCreationIndex += (ulongint)kwoObject->GetCreationIndex();
	}
	// Utilisation direct de l'index de creation (numero de ligne) pour les instances lue depuis un fichier
	else
		ulGlobalCreationIndex = (ulongint)kwoObject->GetCreationIndex();

	// Parametrage du generateur aleatoire, a partir des parametre globaux du data path (bits de poids fort)
	// et des parametres locaux de la fonction (bits de poid faible)
	ulSeed = (ulongint)objectDataPath->GetRandomSeed();
	ulSeed = ulSeed << 32;
	ulSeed += (ulongint)(unsigned int)nSeed;
	ulLeap = (ulongint)objectDataPath->GetRandomLeap();
	ulLeap = ulLeap << 32;
	ulLeap += (ulongint)(unsigned int)ulLeap;

	// Calcul du ieme nombre aleatoire a partir de la graine et par sauts selon l'index de l'objet
	ulRandomIndex = ulSeed + ulLeap * ulGlobalCreationIndex;

	// Passage en longint selon le type attendu
	lRandomIndex = (longint)ulRandomIndex;
	if (lRandomIndex < 0)
	{
		// A faire en deux temps, puisque 2^63 = LLONG_MAX+1
		lRandomIndex += LLONG_MAX;
		lRandomIndex++;
	}
	dResult = IthRandomDouble(lRandomIndex);

	// Trace
	if (bTrace)
	{
		cout << GetName() << "\t";
		cout << kwoObject->GetClass()->GetName() << "\t";
		cout << kwoObject->GetObjectDataPath()->GetDataPath();
		if (kwoObject->GetObjectDataPath()->GetCreatedObjects())
			cout << "(C)";
		cout << "\t";
		cout << "(" << kwoObject->GetObjectDataPath()->GetMainCreationIndex() << ","
		     << kwoObject->GetCreationIndex() << ")\t";
		cout << ulGlobalCreationIndex << "\t";
		cout << "(" << (unsigned int)kwoObject->GetObjectDataPath()->GetRandomSeed() << ","
		     << (unsigned int)kwoObject->GetObjectDataPath()->GetRandomLeap() << ")\t";
		cout << "(" << (unsigned int)nSeed << "," << (unsigned int)nLeap << ")\t";
		cout << "(" << ulSeed << "," << ulLeap << ")\t";
		cout << ulRandomIndex << "\t";
		cout << lRandomIndex << "\t";
		cout << dResult << "\n";
	}
	return (Continuous)dResult;
}

void KWDRRandom::InitializeRandomParameters(const ALString& sCompiledClassName, const ALString& sAttributeName,
					    int nRuleRankInAttribute)
{
	ALString sSeedEncoding;
	ALString sLeapEncoding;
	int n;
	ALString sTmp;

	require(sCompiledClassName != "");
	require(sAttributeName != "");
	require(nRuleRankInAttribute > 0);

	// Initialisation de la graine du generateur aleatoire par hashage de chaines de caractere
	// dependant du nom de la classe et du rand d'utilisation de la regle localement a la classe
	sSeedEncoding = sTmp + "Seed" + IntToString(sCompiledClassName.GetLength()) + sCompiledClassName +
			IntToString(sAttributeName.GetLength()) + sAttributeName + IntToString(nRuleRankInAttribute) +
			"Seed";
	nSeed = HashValue(sSeedEncoding);

	// Initialisation du saut du generateur aleatoire
	sLeapEncoding = sTmp + "Leap" + IntToString(sAttributeName.GetLength()) + sAttributeName +
			IntToString(sCompiledClassName.GetLength()) + sCompiledClassName +
			IntToString(nRuleRankInAttribute) + "Leap";
	nLeap = HashValue(sLeapEncoding);

	// On interdit un Leap de 0
	n = 0;
	while (nLeap == 0)
	{
		sLeapEncoding += "_";
		sLeapEncoding += IntToString(n);
		nLeap = HashValue(sLeapEncoding);
		n++;
	}
}
