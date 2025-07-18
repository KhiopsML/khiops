// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	const KWObjectDataPath* objectDataPath;
	longint lSeed;
	longint lLeap;
	longint lGlobalCreationIndex;
	longint lRandomIndex;
	double dResult;

	require(kwoObject != NULL);

	// Acces au data path de l'objet
	objectDataPath = kwoObject->GetDataPath();

	// Calcul d'index de creation d'index global pour les objets crees par des regles de creation d'instance
	if (objectDataPath->GetCreatedObjects())
	{
		// On calcul un index unique localement a chaque instance principale, en multipliant son index
		// par 2^27, ce qui comme 2^64 = 2^37 * 2^27 permet d'assure un identifiant unique jusqu'a
		// 2^37 ~ 128 milliards d'instances principales lues depuis des fichier, et jusqu'a
		// 2^27 ~ 128 millions d'instances creees par instance principale
		lGlobalCreationIndex = objectDataPath->GetMainCreationIndex();
		lGlobalCreationIndex = lGlobalCreationIndex << 27;

		// On ajoute l'index local a l'objet cree
		lGlobalCreationIndex += kwoObject->GetCreationIndex();
	}
	// Utilisation direct de l'index de creation (numero de ligne) pour les instances lue depuis un fichier
	else
		lGlobalCreationIndex = kwoObject->GetCreationIndex();

	// Parametrage du generateur aleatoire, a partir des parametre globaux du data path (bits de poids fort)
	// et des parametres locaux de la fonction (bits de poid faible)
	lSeed = objectDataPath->GetRandomSeed();
	lSeed = lSeed << 32;
	lSeed += nSeed;
	lLeap = objectDataPath->GetRandomLeap();
	lLeap = lLeap << 32;
	lLeap += nLeap;

	// Calcul du ieme nombre aleatoire a partir de la graine et par sauts selon l'index de l'objet
	lRandomIndex = lSeed + lLeap * lGlobalCreationIndex;
	if (lRandomIndex < 0)
		lRandomIndex += LLONG_MAX;
	dResult = IthRandomDouble(lRandomIndex);

	// Trace
	if (bTrace)
	{
		cout << GetName() << "\t";
		cout << kwoObject->GetClass()->GetName() << "\t";
		cout << kwoObject->GetDataPath()->GetDataPath();
		if (kwoObject->GetDataPath()->GetCreatedObjects())
			cout << "(C)";
		cout << "\t";
		cout << "(" << kwoObject->GetDataPath()->GetMainCreationIndex() << "," << kwoObject->GetCreationIndex()
		     << ")\t";
		cout << lGlobalCreationIndex << "\t";
		cout << "(" << kwoObject->GetDataPath()->GetRandomSeed() << ","
		     << kwoObject->GetDataPath()->GetRandomLeap() << ")\t";
		cout << "(" << nSeed << "," << nLeap << ")\t";
		cout << "(" << lSeed << "," << lLeap << ")\t";
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
