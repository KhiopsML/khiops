// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizerTest.h"

///////////////////////////////////////////////////////////////////////
// Classe KWContinuousSampleGenerator

KWContinuousSampleGenerator::KWContinuousSampleGenerator()
{
	// On initialise les attributs de classe dans le constructeur
	// (l'initialisation directe des variables statiques pose un probleme,
	// a cause de l'ordre non maitrise de l'initialisation des variables
	// statiques entre les classes)
	sClassPlus = Symbol("+");
	sClassMinus = Symbol("-");
}

KWContinuousSampleGenerator::~KWContinuousSampleGenerator() {}

KWClass* KWContinuousSampleGenerator::GetContinuousSampleClass()
{
	const ALString sClassName = "NumericalSample";
	KWClass* kwcClass;
	KWAttribute* attribute;

	// Recherche de la classe de test
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);

	// Creation et enregistrement de la classe si non trouvee
	if (kwcClass == NULL)
	{
		// Initialisation de la classe
		kwcClass = new KWClass;
		kwcClass->SetName(sClassName);
		kwcClass->SetLabel("Test des methodes de discretisation");

		// Initialisation des attributs
		attribute = new KWAttribute;
		attribute->SetName("X");
		attribute->SetType(KWType::Continuous);
		kwcClass->InsertAttribute(attribute);
		attribute = new KWAttribute;
		attribute->SetName("Class");
		attribute->SetType(KWType::Symbol);
		kwcClass->InsertAttribute(attribute);

		// Enregistrement et compilation
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
		kwcClass->Compile();
	}

	// Verifications
	check(kwcClass);
	assert(kwcClass->Check());
	assert(kwcClass->LookupAttribute("X") != NULL);
	assert(kwcClass->LookupAttribute("Class") != NULL);

	// Acces aux index de chargement des attributs sources et cible
	liSourceAttributeLoadIndex = kwcClass->LookupAttribute("X")->GetLoadIndex();
	liTargetAttributeLoadIndex = kwcClass->LookupAttribute("Class")->GetLoadIndex();

	return kwcClass;
}

KWLoadIndex KWContinuousSampleGenerator::GetSourceAttributeLoadIndex() const
{
	assert(liSourceAttributeLoadIndex ==
	       KWClassDomain::GetCurrentDomain()->LookupClass("NumericalSample")->LookupAttribute("X")->GetLoadIndex());
	return liSourceAttributeLoadIndex;
}

KWLoadIndex KWContinuousSampleGenerator::GetTargetAttributeLoadIndex() const
{
	assert(liTargetAttributeLoadIndex == KWClassDomain::GetCurrentDomain()
						 ->LookupClass("NumericalSample")
						 ->LookupAttribute("Class")
						 ->GetLoadIndex());
	return liTargetAttributeLoadIndex;
}

boolean KWContinuousSampleGenerator::CheckObject(KWObject* kwoObject)
{
	boolean bOk = true;

	if (kwoObject == NULL)
	{
		bOk = false;
		AddError("Entity is NULL");
	}
	else if (kwoObject->GetClass() != GetContinuousSampleClass())
	{
		bOk = false;
		kwoObject->AddError("Wrong dictionary");
	}
	else
	{
		bOk = true;
		assert(kwoObject->GetClass()->IsCompiled());
		assert(kwoObject->GetClass()->LookupAttribute("X")->GetLoadIndex() == liSourceAttributeLoadIndex);
		assert(kwoObject->GetClass()->LookupAttribute("Class")->GetLoadIndex() == liTargetAttributeLoadIndex);
	}

	return bOk;
}

double KWContinuousSampleGenerator::ComputeDiscretizationLearningError(KWAttributeStats* kwasDiscretization)
{
	KWDataGridStats* dataGridStats;
	const KWDGSAttributePartition* sourcePartition;
	const KWDGSAttributePartition* targetPartition;
	double dError;
	int nInterval;
	int nCorrectInstanceNumber;

	require(kwasDiscretization != NULL);
	require(kwasDiscretization->GetAttributeType() == KWType::Continuous);
	require(kwasDiscretization->GetPreparedDataGridStats() != NULL);

	// Acces a la grille de preparation de la discretisation
	dataGridStats = kwasDiscretization->GetPreparedDataGridStats();
	assert(dataGridStats->GetAttributeNumber() == 2);
	sourcePartition = dataGridStats->GetAttributeAt(0);
	targetPartition = dataGridStats->GetAttributeAt(1);

	// Cas particulier: une seule classe cible (0% d'erreur en apprentissage)
	if (targetPartition->GetPartNumber() == 1)
		return 0;

	// Comptage des instances correctement classifiees par le predicteur majoritaire
	nCorrectInstanceNumber = 0;
	for (nInterval = 0; nInterval < sourcePartition->GetPartNumber(); nInterval++)
	{
		// Recherche de la classe majoritaire
		if (dataGridStats->GetBivariateCellFrequencyAt(nInterval, 0) >=
		    dataGridStats->GetBivariateCellFrequencyAt(nInterval, 1))
			nCorrectInstanceNumber += dataGridStats->GetBivariateCellFrequencyAt(nInterval, 0);
		else
			nCorrectInstanceNumber += dataGridStats->GetBivariateCellFrequencyAt(nInterval, 1);
	}

	// Calcul du taux d'erreur
	dError = 1 - (nCorrectInstanceNumber * 1.0 / dataGridStats->ComputeGridFrequency());
	return dError;
}

double KWContinuousSampleGenerator::ComputeDiscretizationTestError(KWAttributeStats* kwasDiscretization)
{
	int nClassPlusIndex;
	int nClassMinusIndex;
	boolean bIsClassPlusMajoritary;
	double dTestError;
	int i;
	KWDataGridStats* dataGridStats;
	const KWDGSAttributeDiscretization* attributeDiscretization;
	const KWDGSAttributeSymbolValues* targetPartition;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(kwasDiscretization != NULL);
	require(kwasDiscretization->GetAttributeType() == KWType::Continuous);
	require(kwasDiscretization->GetPreparedDataGridStats() != NULL);

	// Acces a la discretisation de l'attribut
	dataGridStats = kwasDiscretization->GetPreparedDataGridStats();
	assert(dataGridStats->GetAttributeNumber() == 2);
	attributeDiscretization = cast(const KWDGSAttributeDiscretization*, dataGridStats->GetAttributeAt(0));
	targetPartition = cast(const KWDGSAttributeSymbolValues*, dataGridStats->GetAttributeAt(1));

	// Cas particulier: une seule classe cible
	if (kwasDiscretization->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber() == 1)
	{
		assert(attributeDiscretization->GetPartNumber() == 1);

		// Calcul de la classe majoritaire
		nClassPlusIndex = targetPartition->ComputeSymbolPartIndex(sClassPlus);
		bIsClassPlusMajoritary = nClassPlusIndex == 0;

		// Calcul du taux d'erreur sur le seul intervalle
		dTestError = ComputeIntervalTestError(KWContinuous::GetMissingValue(), KWContinuous::GetMaxValue(),
						      bIsClassPlusMajoritary);
	}
	// Cas general
	else
	{
		assert(targetPartition->GetPartNumber() == 2);

		// Recherche de l'index de la classe plus
		nClassPlusIndex = targetPartition->ComputeSymbolPartIndex(sClassPlus);
		nClassMinusIndex = 1 - nClassPlusIndex;
		assert(targetPartition->ComputeSymbolPartIndex(sClassMinus) == nClassMinusIndex);

		// Calcul de l'erreur cumulee sur les intervalles de la discretisation
		dTestError = 0;
		for (i = 0; i < attributeDiscretization->GetPartNumber(); i++)
		{
			// Calcul des bornes de l'intervalle
			cLowerBound = KWContinuous::GetMissingValue();
			if (i > 0)
				cLowerBound = attributeDiscretization->GetIntervalBoundAt(i - 1);
			cUpperBound = KWContinuous::GetMaxValue();
			if (i < attributeDiscretization->GetIntervalBoundNumber())
				cUpperBound = attributeDiscretization->GetIntervalBoundAt(i);

			// Ajout de l'erreur sur l'intervalle
			dTestError += ComputeIntervalTestError(
			    cLowerBound, cUpperBound,
			    dataGridStats->GetBivariateCellFrequencyAt(i, nClassPlusIndex) >=
				dataGridStats->GetBivariateCellFrequencyAt(i, nClassMinusIndex));
		}
	}

	return dTestError;
}

double KWContinuousSampleGenerator::ComputeDiscretizationDistance(KWAttributeStats* kwasDiscretization)
{
	int nClassPlusIndex;
	int nClassMinusIndex;
	double dClassPlusProb;
	double dDistance;
	int i;
	KWDataGridStats* dataGridStats;
	const KWDGSAttributeDiscretization* attributeDiscretization;
	const KWDGSAttributeSymbolValues* targetPartition;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(kwasDiscretization != NULL);
	require(kwasDiscretization->GetAttributeType() == KWType::Continuous);
	require(kwasDiscretization->GetPreparedDataGridStats() != NULL);

	// Acces a la discretisation de l'attribute
	dataGridStats = kwasDiscretization->GetPreparedDataGridStats();
	assert(dataGridStats->GetAttributeNumber() == 2);
	attributeDiscretization = cast(const KWDGSAttributeDiscretization*, dataGridStats->GetAttributeAt(0));
	targetPartition = cast(const KWDGSAttributeSymbolValues*, dataGridStats->GetAttributeAt(1));

	// Cas particulier: une seule classe cibble
	if (kwasDiscretization->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber() == 1)
	{
		assert(attributeDiscretization->GetPartNumber() == 1);

		// Calcul de la classe majoritaire
		nClassPlusIndex = targetPartition->ComputeSymbolPartIndex(sClassPlus);
		if (nClassPlusIndex == 0)
			dClassPlusProb = 1;
		else
			dClassPlusProb = 0;

		// Calcul du taux d'erreur sur le seul intervalle
		dDistance = ComputeIntervalDistance(KWContinuous::GetMissingValue(), KWContinuous::GetMaxValue(),
						    dClassPlusProb);
	}
	// Cas general
	else
	{
		assert(kwasDiscretization->GetTargetDescriptiveStats()->GetValueNumber() == 2);

		// Recherche de l'index de la classe plus
		nClassPlusIndex = targetPartition->ComputeSymbolPartIndex(sClassPlus);
		nClassMinusIndex = 1 - nClassPlusIndex;
		assert(targetPartition->ComputeSymbolPartIndex(sClassMinus) == nClassMinusIndex);

		// Calcul de l'erreur cumulee sur les intervalles de la discretisation
		dDistance = 0;
		for (i = 0; i < attributeDiscretization->GetPartNumber(); i++)
		{
			// Calcul des bornes de l'intervalle
			cLowerBound = KWContinuous::GetMissingValue();
			if (i > 0)
				cLowerBound = attributeDiscretization->GetIntervalBoundAt(i - 1);
			cUpperBound = KWContinuous::GetMaxValue();
			if (i < attributeDiscretization->GetIntervalBoundNumber())
				cUpperBound = attributeDiscretization->GetIntervalBoundAt(i);

			// Ajout de l'erreur sur l'intervalle
			dDistance += ComputeIntervalDistance(
			    cLowerBound, cUpperBound,
			    dataGridStats->GetBivariateCellFrequencyAt(i, nClassPlusIndex) * 1.0 /
				(dataGridStats->GetBivariateCellFrequencyAt(i, nClassMinusIndex) +
				 dataGridStats->GetBivariateCellFrequencyAt(i, nClassPlusIndex)));
		}
	}

	return dDistance;
}

double KWContinuousSampleGenerator::GetRandomDouble()
{
	return RandomDouble();
}

///////////////////////////////////////////////////////////////////////
// Classe KWCSGRandom

KWCSGRandom::KWCSGRandom() {}

KWCSGRandom::~KWCSGRandom() {}

const ALString KWCSGRandom::GetName()
{
	return "Random";
}

void KWCSGRandom::GenerateObjectValues(KWObject* kwoObject)
{
	double dRandom;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	dRandom = GetRandomDouble();
	kwoObject->SetContinuousValueAt(liSourceAttributeLoadIndex, (Continuous)dRandom);

	// La classe est generes aleatoirement
	dRandom = GetRandomDouble();
	if (dRandom >= 0.5)
		kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassPlus);
	else
		kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassMinus);
}

double KWCSGRandom::ComputeIntervalTestError(double dLowerBound, double dUpperBound, boolean bIsClassPlusMajoritary)
{
	double dTrueLowerBound;
	double dTrueUpperBound;

	require(dUpperBound >= dLowerBound);

	// Calcul des bornes reelles
	dTrueLowerBound = dLowerBound;
	if (dTrueLowerBound < 0)
		dTrueLowerBound = 0;
	dTrueUpperBound = dUpperBound;
	if (dTrueUpperBound > 1)
		dTrueUpperBound = 1;

	// La classe Plus est equidistribuee au hazard sur tout l'intervalle: l'erreur en test
	// est toujours 50%
	return (dTrueUpperBound - dTrueLowerBound) * 0.5;
}

double KWCSGRandom::ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb)
{
	double dTrueLowerBound;
	double dTrueUpperBound;

	require(dUpperBound >= dLowerBound);
	require(0 <= dClassPlusProb and dClassPlusProb <= 1);

	// Calcul des bornes reelles
	dTrueLowerBound = dLowerBound;
	if (dTrueLowerBound < 0)
		dTrueLowerBound = 0;
	dTrueUpperBound = dUpperBound;
	if (dTrueUpperBound > 1)
		dTrueUpperBound = 1;

	// La classe Plus est equidistribuee au hazard sur tout l'intervalle
	return (dTrueUpperBound - dTrueLowerBound) * fabs(dClassPlusProb - 0.5);
}

///////////////////////////////////////////////////////////////////////
// Classe KWCSGSinusSign

KWCSGSinusSign::KWCSGSinusSign()
{
	nSinusoidFrequency = 1;
	dNoiseProb = 0;
}

KWCSGSinusSign::~KWCSGSinusSign() {}

void KWCSGSinusSign::SetSinusoidFrequency(int nValue)
{
	require(nValue >= 1);
	nSinusoidFrequency = nValue;
}

int KWCSGSinusSign::GetSinusoidFrequency() const
{
	return nSinusoidFrequency;
}

void KWCSGSinusSign::SetNoiseProb(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dNoiseProb = dValue;
}

double KWCSGSinusSign::GetNoiseProb() const
{
	return dNoiseProb;
}

const ALString KWCSGSinusSign::GetName()
{
	ALString sName;

	sName = sName + "SinusSign(" + IntToString(nSinusoidFrequency * 2) + ".Pi.X";
	if (dNoiseProb > 0)
		sName = sName + ", " + DoubleToString(dNoiseProb);
	sName += ")";

	return sName;
}

void KWCSGSinusSign::GenerateObjectValues(KWObject* kwoObject)
{
	const double dPI = 3.1415926535;
	double dX;
	double dRandom;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	dX = GetRandomDouble();
	kwoObject->SetContinuousValueAt(liSourceAttributeLoadIndex, (Continuous)dX);

	// La classe est generes aleatoirement selon le niveau de bruit
	dRandom = GetRandomDouble();
	if (dRandom <= dNoiseProb)
	{
		// Generation aleatoire de la classe
		dRandom = GetRandomDouble();
		if (dRandom <= 0.5)
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassPlus);
		else
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassMinus);
	}
	// Ici, la classe est calculee par Signe(Sinus(2.k.pi.X))
	else
	{
		if (sin(2 * nSinusoidFrequency * dPI * dX) >= 0)
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassPlus);
		else
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassMinus);
	}
}

double KWCSGSinusSign::ComputeIntervalTestError(double dLowerBound, double dUpperBound, boolean bIsClassPlusMajoritary)
{
	boolean bDebug = false;
	double dError;
	double dTrueLowerBound;
	double dTrueUpperBound;
	int nSinusoidIndex;
	double dSinusoidLowerBound;
	double dSinusoidMiddleBound;
	double dSinusoidUpperBound;
	double dIntervalLowerBound;
	double dIntervalUpperBound;

	require(dUpperBound >= dLowerBound);

	// Calcul des bornes reelles
	dTrueLowerBound = GetIntervalProjection(0, 1, dLowerBound);
	dTrueUpperBound = GetIntervalProjection(0, 1, dUpperBound);

	// Calcul de l'index de la sinusoide precedant l'intervalle
	nSinusoidIndex = (int)floor(dTrueLowerBound * nSinusoidFrequency);

	// Calcul de l'erreur en parcourant les sinusoides sur l'intervalles
	dError = 0;
	dSinusoidLowerBound = nSinusoidIndex * 1.0 / nSinusoidFrequency;
	assert(dSinusoidLowerBound <= dTrueLowerBound);
	while (dSinusoidLowerBound < dTrueUpperBound)
	{
		// Calcul des bornes de la sinusoide
		dSinusoidMiddleBound = dSinusoidLowerBound + 0.5 / nSinusoidFrequency;
		dSinusoidUpperBound = dSinusoidLowerBound + 1.0 / nSinusoidFrequency;

		// Mesure de l'erreur sur la premiere arche (prediction de "+")
		dIntervalLowerBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidLowerBound);
		dIntervalUpperBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidMiddleBound);
		if (bIsClassPlusMajoritary)
			dError += (dIntervalUpperBound - dIntervalLowerBound) * dNoiseProb / 2;
		else
			dError += (dIntervalUpperBound - dIntervalLowerBound) * (1 - dNoiseProb / 2);
		if (bDebug)
			cout << "\t" << bIsClassPlusMajoritary << "\t" << dIntervalLowerBound << "\t"
			     << dIntervalUpperBound << "\t" << dError << endl;

		// Mesure de l'erreur sur la seconde arche (prediction de "-")
		dIntervalLowerBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidMiddleBound);
		dIntervalUpperBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidUpperBound);
		if (not bIsClassPlusMajoritary)
			dError += (dIntervalUpperBound - dIntervalLowerBound) * dNoiseProb / 2;
		else
			dError += (dIntervalUpperBound - dIntervalLowerBound) * (1 - dNoiseProb / 2);
		if (bDebug)
			cout << "\t" << bIsClassPlusMajoritary << "\t" << dIntervalLowerBound << "\t"
			     << dIntervalUpperBound << "\t" << dError << endl;

		// Passage a la sinuoside suivante
		nSinusoidIndex++;
		dSinusoidLowerBound = nSinusoidIndex * 1.0 / nSinusoidFrequency;
	}
	if (bDebug)
		cout << dError << "\t" << dLowerBound << "\t" << dUpperBound << endl;
	return dError;
}

double KWCSGSinusSign::ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb)
{
	boolean bDebug = false;
	double dDistance;
	double dTrueLowerBound;
	double dTrueUpperBound;
	int nSinusoidIndex;
	double dSinusoidLowerBound;
	double dSinusoidMiddleBound;
	double dSinusoidUpperBound;
	double dIntervalLowerBound;
	double dIntervalUpperBound;

	require(dUpperBound >= dLowerBound);

	// Calcul des bornes reelles
	dTrueLowerBound = GetIntervalProjection(0, 1, dLowerBound);
	dTrueUpperBound = GetIntervalProjection(0, 1, dUpperBound);

	// Calcul de l'index de la sinusoide precedant l'intervalle
	nSinusoidIndex = (int)floor(dTrueLowerBound * nSinusoidFrequency);

	// Calcul de la distance en parcourant les sinusoides sur l'intervalles
	dDistance = 0;
	dSinusoidLowerBound = nSinusoidIndex * 1.0 / nSinusoidFrequency;
	assert(dSinusoidLowerBound <= dTrueLowerBound);
	while (dSinusoidLowerBound < dTrueUpperBound)
	{
		// Calcul des bornes de la sinusoide
		dSinusoidMiddleBound = dSinusoidLowerBound + 0.5 / nSinusoidFrequency;
		dSinusoidUpperBound = dSinusoidLowerBound + 1.0 / nSinusoidFrequency;

		// Mesure de la distance sur la premiere arche (prediction de "+")
		dIntervalLowerBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidLowerBound);
		dIntervalUpperBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidMiddleBound);
		dDistance += (dIntervalUpperBound - dIntervalLowerBound) * fabs(dClassPlusProb - (1 - dNoiseProb / 2));
		if (bDebug)
			cout << "\t" << dClassPlusProb << "\t" << dIntervalLowerBound << "\t" << dIntervalUpperBound
			     << "\t" << dDistance << endl;

		// Mesure de la distance sur la seconde arche (prediction de "-")
		dIntervalLowerBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidMiddleBound);
		dIntervalUpperBound = GetIntervalProjection(dTrueLowerBound, dTrueUpperBound, dSinusoidUpperBound);
		dDistance += (dIntervalUpperBound - dIntervalLowerBound) * fabs(dClassPlusProb - dNoiseProb / 2);
		if (bDebug)
			cout << "\t" << dClassPlusProb << "\t" << dIntervalLowerBound << "\t" << dIntervalUpperBound
			     << "\t" << dDistance << endl;

		// Passage a la sinuoside suivante
		nSinusoidIndex++;
		dSinusoidLowerBound = nSinusoidIndex * 1.0 / nSinusoidFrequency;
	}
	if (bDebug)
		cout << dDistance << "\t" << dLowerBound << "\t" << dUpperBound << endl;
	return dDistance;
}

double KWCSGSinusSign::GetIntervalProjection(double dLowerBound, double dUpperBound, double dValue)
{
	require(dLowerBound <= dUpperBound);
	if (dValue < dLowerBound)
		return dLowerBound;
	else if (dValue > dUpperBound)
		return dUpperBound;
	else
		return dValue;
}

///////////////////////////////////////////////////////////////////////
// Classe KWCSGLinearProb

KWCSGLinearProb::KWCSGLinearProb() {}

KWCSGLinearProb::~KWCSGLinearProb() {}

const ALString KWCSGLinearProb::GetName()
{
	return "LinearProb";
}

void KWCSGLinearProb::GenerateObjectValues(KWObject* kwoObject)
{
	double dX;
	double dRandom;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	dX = GetRandomDouble();
	kwoObject->SetContinuousValueAt(liSourceAttributeLoadIndex, (Continuous)dX);

	// La classe est generes aleatoirement
	dRandom = GetRandomDouble();
	if (dRandom <= dX)
		kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassPlus);
	else
		kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, sClassMinus);
}

double KWCSGLinearProb::ComputeIntervalTestError(double dLowerBound, double dUpperBound, boolean bIsClassPlusMajoritary)
{
	double dTrueLowerBound;
	double dTrueUpperBound;

	require(dUpperBound >= dLowerBound);

	// Calcul des bornes reelles
	dTrueLowerBound = dLowerBound;
	if (dTrueLowerBound < 0)
		dTrueLowerBound = 0;
	dTrueUpperBound = dUpperBound;
	if (dTrueUpperBound > 1)
		dTrueUpperBound = 1;

	// La prediction optimale est de predire "-" sur [0; 0.5], et "+" sur [0.5; 1]
	if (dUpperBound <= 0.5)
	{
		if (bIsClassPlusMajoritary)
			return dTrueUpperBound - dTrueLowerBound;
		else
			return 0;
	}
	else if (dTrueLowerBound >= 0.5)
	{
		if (bIsClassPlusMajoritary)
			return 0;
		else
			return dTrueUpperBound - dTrueLowerBound;
	}
	else
	{
		assert(dTrueLowerBound < 0.5 and 0.5 < dTrueUpperBound);
		if (bIsClassPlusMajoritary)
			return 0.5 - dTrueLowerBound;
		else
			return dTrueUpperBound - 0.5;
	}
}

double KWCSGLinearProb::ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb)
{
	double dTrueLowerBound;
	double dTrueUpperBound;
	double dDistance;

	require(dUpperBound >= dLowerBound);
	require(0 <= dClassPlusProb and dClassPlusProb <= 1);

	// Calcul des bornes reelles
	dTrueLowerBound = dLowerBound;
	if (dTrueLowerBound < 0)
		dTrueLowerBound = 0;
	dTrueUpperBound = dUpperBound;
	if (dTrueUpperBound > 1)
		dTrueUpperBound = 1;

	// On doit calculer l'integrale de la difference (en valeur absolue) entre la
	// probabilite estimee (constante) et la probabilite reelle (lineaire)
	// Cas ou la probabilite estimee (prob=dClassPlusProb) intersecte avec la
	// probabilite reelle (prob=X) sur l'intervalle
	if (dTrueLowerBound < dClassPlusProb and dClassPlusProb < dTrueUpperBound)
	{
		dDistance = (dTrueUpperBound - dClassPlusProb) * (dTrueUpperBound - dClassPlusProb) / 2 +
			    (dClassPlusProb - dTrueLowerBound) * (dClassPlusProb - dTrueLowerBound) / 2;
	}
	// Cas ou la probabilite estimee (prob=dClassPlusProb) est au dessus de la
	// probabilite reelle (prob=X) sur l'intervalle
	else if (dClassPlusProb >= dTrueUpperBound)
	{
		dDistance = (dClassPlusProb - dTrueUpperBound) * (dTrueUpperBound - dTrueLowerBound) +
			    (dTrueUpperBound - dTrueLowerBound) * (dTrueUpperBound - dTrueLowerBound) / 2;
	}
	// Cas ou la probabilite estimee (prob=dClassPlusProb) est en dessous de la
	// probabilite reelle (prob=X) sur l'intervalle
	else
	{
		assert(dClassPlusProb <= dTrueLowerBound);
		dDistance = (dTrueLowerBound - dClassPlusProb) * (dTrueUpperBound - dTrueLowerBound) +
			    (dTrueUpperBound - dTrueLowerBound) * (dTrueUpperBound - dTrueLowerBound) / 2;
	}
	return dDistance;
}

///////////////////////////////////////////////////////////////////////
// Classe KWContinuousSampleDiscretizerTest

KWContinuousSampleDiscretizerTest::KWContinuousSampleDiscretizerTest()
{
	// Initialisation des attributs d'instances
	discretizerSpec = NULL;
	nSampleSize = 0;
	nSampleNumber = 0;
	nFreshness = 0;
	nStatsFreshness = 0;
	nDiscretizerStatsFreshness = 0;
}

KWContinuousSampleDiscretizerTest::~KWContinuousSampleDiscretizerTest() {}

void KWContinuousSampleDiscretizerTest::SetGenerator(KWContinuousSampleGenerator* kwcsgSampleGenerator)
{
	sampleGenerator = kwcsgSampleGenerator;
}

KWContinuousSampleGenerator* KWContinuousSampleDiscretizerTest::GetGenerator()
{
	return sampleGenerator;
}

void KWContinuousSampleDiscretizerTest::SetDiscretizer(KWDiscretizerSpec* kwdsDiscretizerSpec)
{
	discretizerSpec = kwdsDiscretizerSpec;
	nFreshness++;
}

KWDiscretizerSpec* KWContinuousSampleDiscretizerTest::GetDiscretizer()
{
	return discretizerSpec;
}

void KWContinuousSampleDiscretizerTest::SetSampleSize(int nValue)
{
	require(nValue >= 0);

	nSampleSize = nValue;
	nFreshness++;
}

int KWContinuousSampleDiscretizerTest::GetSampleSize()
{
	return nSampleSize;
}

void KWContinuousSampleDiscretizerTest::SetSampleNumber(int nValue)
{
	require(nValue >= 0);

	nSampleNumber = nValue;
	nFreshness++;
}

int KWContinuousSampleDiscretizerTest::GetSampleNumber()
{
	return nSampleNumber;
}

boolean KWContinuousSampleDiscretizerTest::Check() const
{
	return sampleGenerator != NULL and discretizerSpec != NULL and discretizerSpec->Check();
}

boolean KWContinuousSampleDiscretizerTest::ComputeStats()
{
	boolean bExportSampleFiles = false;
	boolean bExportStatFiles = false;
	boolean bDisplayNonNulDiscretizations = false;
	KWSTDatabaseTextFile sampleDatabase;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable tupleTable;
	KWLearningSpec sampleLearningSpec;
	KWAttributeStats continuousStats;
	KWDataGridStats* dataGridStats;
	KWClass* kwcSampleTestClass;
	int i;
	KWObject* kwoObject;
	int nRun;
	int nPartNumber;

	require(Check());

	// Preparation des vecteurs de resultats
	dvNoInformativeAttributeNumbers.SetSize(nSampleNumber);
	dvDiscretizationSizes.SetSize(nSampleNumber);
	dvLearningErrors.SetSize(nSampleNumber);
	dvTestErrors.SetSize(nSampleNumber);
	dvDistances.SetSize(nSampleNumber);

	// Remise a 0 de tous les resultats
	dvNoInformativeAttributeNumbers.Initialize();
	dvDiscretizationSizes.Initialize();
	dvLearningErrors.Initialize();
	dvTestErrors.Initialize();
	dvDistances.Initialize();

	// Acces a la classe de test
	kwcSampleTestClass = sampleGenerator->GetContinuousSampleClass();

	// Initialisation de la base et des specifications d'apprentissage
	sampleDatabase.SetClassName(kwcSampleTestClass->GetName());
	sampleDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + ".txt");
	sampleLearningSpec.SetClass(kwcSampleTestClass);
	sampleLearningSpec.SetDatabase(&sampleDatabase);
	sampleLearningSpec.SetTargetAttributeName("Class");
	sampleLearningSpec.GetPreprocessingSpec()->GetDiscretizerSpec()->CopyFrom(discretizerSpec);
	continuousStats.SetLearningSpec(&sampleLearningSpec);
	continuousStats.SetAttributeName("X");
	continuousStats.SetAttributeType(KWType::Continuous);

	// Initialisation des objets de la base de donnees
	for (i = 0; i < nSampleSize; i++)
	{
		kwoObject = new KWObject(kwcSampleTestClass, i + 1);
		sampleDatabase.GetObjects()->Add(kwoObject);
	}

	// Repetition de l'experimentation
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		// On fixe la graine aleatoire
		SetRandomSeed(nRun);

		// Generation des valeurs des exemples
		sampleDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + IntToString(nRun + 1) + ".txt");
		GenerateSample(&sampleDatabase);

		// Export de la base
		if (bExportSampleFiles)
			sampleDatabase.WriteAll(&sampleDatabase);
		tupleTableLoader.SetInputClass(kwcSampleTestClass);
		tupleTableLoader.SetInputDatabaseObjects(sampleDatabase.GetObjects());

		// Calcul des stats sur l'attribut cible
		tupleTableLoader.LoadUnivariate(sampleLearningSpec.GetTargetAttributeName(), &tupleTable);
		sampleLearningSpec.ComputeTargetStats(&tupleTable);

		// Discretization
		tupleTableLoader.LoadBivariate(continuousStats.GetAttributeName(),
					       sampleLearningSpec.GetTargetAttributeName(), &tupleTable);
		continuousStats.ComputeStats(&tupleTable);

		// Acces a la grille de preparation
		dataGridStats = continuousStats.GetPreparedDataGridStats();

		// Calcul du nombre de parties
		nPartNumber = 1;
		if (dataGridStats != NULL and dataGridStats->GetSourceAttributeNumber() > 0)
			nPartNumber = dataGridStats->GetAttributeAt(0)->GetPartNumber();

		// Affichage des discretisations
		if (bDisplayNonNulDiscretizations and nPartNumber > 1)
		{
			int j;
			cout << "Example\t" << nSampleSize << "\t" << nPartNumber << "\t:";
			for (i = 0; i < dataGridStats->GetAttributeAt(0)->GetPartNumber(); i++)
			{
				for (j = 0; j < dataGridStats->GetAttributeAt(1)->GetPartNumber(); j++)
				{
					if (j > 0)
						cout << ".";
					cout << dataGridStats->GetBivariateCellFrequencyAt(i, j);
				}
				cout << ":";
			}
			cout << endl;
		}

		// Export du fichier de stats resultats
		if (bExportStatFiles)
			continuousStats.WriteReportFile(kwcSampleTestClass->GetName() + IntToString(nRun + 1) + ".xls");

		// Memorisation des resultats
		dvNoInformativeAttributeNumbers.SetAt(nRun, nPartNumber <= 1);
		dvDiscretizationSizes.SetAt(nRun, nPartNumber);
		dvLearningErrors.SetAt(nRun, sampleGenerator->ComputeDiscretizationLearningError(&continuousStats));
		dvTestErrors.SetAt(nRun, sampleGenerator->ComputeDiscretizationTestError(&continuousStats));
		dvDistances.SetAt(nRun, sampleGenerator->ComputeDiscretizationDistance(&continuousStats));

		// (sans cette instruction forcant une entree-sortie, les simulations
		// tres longues s'interrompent mysterieusement
		if (nRun % 1000 == 0)
			cout << " " << flush;
	}
	cout << endl;

	// Memorisation des informations de fraicheur
	nStatsFreshness = nFreshness;
	nDiscretizerStatsFreshness = discretizerSpec->GetFreshness();
	ensure(IsStatsComputed());
	return IsStatsComputed();
}

boolean KWContinuousSampleDiscretizerTest::IsStatsComputed() const
{
	return nStatsFreshness == nFreshness and discretizerSpec != NULL and
	       nDiscretizerStatsFreshness == discretizerSpec->GetFreshness();
}

DoubleVector* KWContinuousSampleDiscretizerTest::GetNoInformativeAttributeNumbers()
{
	require(IsStatsComputed());
	return &dvNoInformativeAttributeNumbers;
}

DoubleVector* KWContinuousSampleDiscretizerTest::GetDiscretizationSizes()
{
	require(IsStatsComputed());
	return &dvDiscretizationSizes;
}

DoubleVector* KWContinuousSampleDiscretizerTest::GetLearningErrors()
{
	require(IsStatsComputed());
	return &dvLearningErrors;
}

DoubleVector* KWContinuousSampleDiscretizerTest::GetTestErrors()
{
	require(IsStatsComputed());
	return &dvTestErrors;
}

DoubleVector* KWContinuousSampleDiscretizerTest::GetDistances()
{
	require(IsStatsComputed());
	return &dvDistances;
}

void KWContinuousSampleDiscretizerTest::WriteReportFile(const ALString& sFileName)
{
	fstream ost;
	boolean bOk;

	require(IsStatsComputed());

	bOk = FileService::OpenOutputFile(sFileName, ost);
	if (bOk)
	{
		WriteReport(ost);
		FileService::CloseOutputFile(sFileName, ost);
	}
}

void KWContinuousSampleDiscretizerTest::WriteReport(ostream& ost)
{
	int nRun;

	require(IsStatsComputed());

	// Specifications des tests
	ost << "Generateur\t" << sampleGenerator->GetName() << "\n";
	ost << "Discretiseur\t" << discretizerSpec->GetObjectLabel() << "\n";
	ost << "Taille des echantillons\t" << nSampleSize << "\n";
	ost << "Nombre d'echantillons\t" << nSampleNumber << "\n";
	ost << "\n";

	// Statistiques par echantillon
	ost << "Run\tNo inf. att.\tInterval Number\tLearning Error\tTest Error\tDistance\n";
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		ost << nRun + 1 << "\t" << dvNoInformativeAttributeNumbers.GetAt(nRun) << "\t"
		    << dvDiscretizationSizes.GetAt(nRun) << "\t" << dvLearningErrors.GetAt(nRun) << "\t"
		    << dvTestErrors.GetAt(nRun) << "\t" << dvDistances.GetAt(nRun) << "\n";
	}
	ost << endl;
}

void KWContinuousSampleDiscretizerTest::WriteHeaderLineReport(ostream& ost)
{
	// Specifications des tests
	ost << "Generator\t";
	ost << "Discretizer\t";
	ost << "Sample size\t";
	ost << "Sample number\t";

	// Moyenne des statistiques calculees
	ost << "No inf. att.\t";
	ost << "Interval number\t";
	ost << "Learning Error\t";
	ost << "Test error\t";
	ost << "Distance\t";

	// Ecart type des statistiques calculees
	ost << "No Std. Dev.\t";
	ost << "IN Std. Dev.\t";
	ost << "LE Std. Dev.\t";
	ost << "TE Std. Dev.\t";
	ost << "D Std. Dev.";
}

void KWContinuousSampleDiscretizerTest::WriteLineReport(ostream& ost)
{
	// Specifications des tests
	ost << sampleGenerator->GetName() << "\t";
	ost << discretizerSpec->GetObjectLabel() << "\t";
	ost << nSampleSize << "\t";
	ost << nSampleNumber << "\t";

	// Moyennes des statistiques calculees
	ost << KWStat::Mean(&dvNoInformativeAttributeNumbers) << "\t";
	ost << KWStat::Mean(&dvDiscretizationSizes) << "\t";
	ost << KWStat::Mean(&dvLearningErrors) << "\t";
	ost << KWStat::Mean(&dvTestErrors) << "\t";
	ost << KWStat::Mean(&dvDistances) << "\t";

	// Ecart type des statistiques calculees
	ost << KWStat::StandardDeviation(&dvNoInformativeAttributeNumbers) << "\t";
	ost << KWStat::StandardDeviation(&dvDiscretizationSizes) << "\t";
	ost << KWStat::StandardDeviation(&dvLearningErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvTestErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvDistances);
}

void KWContinuousSampleDiscretizerTest::GenerateSample(KWDatabase* database)
{
	KWObject* kwoObject;
	int i;

	require(database != NULL);
	require(database->GetClassName() == sampleGenerator->GetContinuousSampleClass()->GetName());

	// Generation de l'echantillon
	for (i = 0; i < database->GetObjects()->GetSize(); i++)
	{
		// Recherche de l'objet a modifier
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(i));

		// Generation aleatoire des valeurs d'une instance
		sampleGenerator->GenerateObjectValues(kwoObject);
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerTest

KWDiscretizerTest::KWDiscretizerTest() {}

KWDiscretizerTest::~KWDiscretizerTest() {}

void KWDiscretizerTest::TestHeadPureInterval()
{
	const int nMinSampleSize = 8;
	const int nMaxSampleSize = 1073741824;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	KWFrequencyTable table;
	KWFrequencyTable* targetTable;
	int nIntervalNumber;
	int nSampleSize;
	int nIntervalSize;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Libelles des discretizers
	cout << "Size";
	for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
	{
		discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
		cout << "\t" << discretizerSpec->GetObjectLabel();
	}
	cout << endl;

	// Parcours des differentes tailles d'echantillons
	nSampleSize = nMinSampleSize;
	while (1 <= nSampleSize and nSampleSize <= nMaxSampleSize)
	{
		// Affichage de la taille
		cout << nSampleSize;

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Recherche de la taille d'intervalle min conduisant a une discretisation en plus de un
			// intervalle
			for (nIntervalSize = 1; nIntervalSize < nSampleSize / 2; nIntervalSize++)
			{
				// Initialisation d'une table de contingence avec intervalle pur
				InitializeHeadPureIntervalTable(&table, nSampleSize, nSampleSize / 2, nIntervalSize);

				// Discretization de cette table
				discretizerSpec->GetDiscretizer(KWType::Symbol)->Discretize(&table, targetTable);
				nIntervalNumber = targetTable->GetFrequencyVectorNumber();
				delete targetTable;

				// Si plus de un intervalle, arret
				if (nIntervalNumber > 1)
					break;
			}

			// Affichage du resultat
			cout << "\t" << nIntervalSize;
		}
		cout << endl;

		// Doublement de la taille d'echantillon
		nSampleSize *= 2;
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestUnbalancedHeadPureInterval()
{
	const int nSampleSize = 10000;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	KWFrequencyTable table;
	KWFrequencyTable* targetTable;
	int nIntervalNumber;
	int nFirstClassFrequency;
	int nIntervalSize;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Libelles des discretizers
	cout << "Size\tFirst class";
	for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
	{
		discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
		cout << "\t" << discretizerSpec->GetObjectLabel();
	}
	cout << endl;

	// Parcours des differentes tailles d'echantillons
	nFirstClassFrequency = 1;
	while (nFirstClassFrequency <= nSampleSize / 2)
	{
		// Affichage de la taille
		cout << nSampleSize;

		// Affichage de l'effectif de la premiere classe
		cout << "\t" << nFirstClassFrequency;

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Recherche de la taille d'intervalle min conduisant a une discretisation en plus de un
			// intervalle
			for (nIntervalSize = 1; nIntervalSize <= nFirstClassFrequency; nIntervalSize++)
			{
				// Initialisation d'une table de contingence avec intervalle pur
				InitializeHeadPureIntervalTable(&table, nSampleSize, nFirstClassFrequency,
								nIntervalSize);

				// Discretization de cette table
				discretizerSpec->GetDiscretizer(KWType::Symbol)->Discretize(&table, targetTable);
				nIntervalNumber = targetTable->GetFrequencyVectorNumber();
				delete targetTable;

				// Si plus de un intervalle, arret
				if (nIntervalNumber > 1)
					break;
			}

			// Affichage du resultat
			cout << "\t";
			if (nIntervalSize <= nFirstClassFrequency)
				cout << nIntervalSize;
		}
		cout << endl;

		// Modification de l'effectif de la premiere classe
		nFirstClassFrequency += 1;
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestCenterPureInterval()
{
	const int nMinSampleSize = 8;
	const int nMaxSampleSize = 1073741824;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	KWFrequencyTable table;
	KWFrequencyTable* targetTable;
	int nIntervalNumber;
	int nSampleSize;
	int nIntervalSize;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Libelles des discretizers
	cout << "Size";
	for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
	{
		discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
		cout << "\t" << discretizerSpec->GetObjectLabel();
	}
	cout << endl;

	// Parcours des differentes tailles d'echantillons
	nSampleSize = nMinSampleSize;
	while (1 <= nSampleSize and nSampleSize <= nMaxSampleSize)
	{
		// Affichage de la taille
		cout << nSampleSize;

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Recherche de la taille d'intervalle min conduisant a une discretisation en plus de un
			// intervalle
			for (nIntervalSize = 1; nIntervalSize < nSampleSize / 2; nIntervalSize++)
			{
				// Initialisation d'une table de contingence avec intervalle pur
				InitializeCenterPureIntervalTable(&table, nSampleSize, nSampleSize / 2, nIntervalSize);

				// Discretization de cette table
				discretizerSpec->GetDiscretizer(KWType::Symbol)->Discretize(&table, targetTable);
				nIntervalNumber = targetTable->GetFrequencyVectorNumber();
				delete targetTable;

				// Si plus de deux intervalle, arret
				if (nIntervalNumber > 2)
					break;
			}

			// Affichage du resultat
			cout << "\t" << nIntervalSize;
		}
		cout << endl;

		// Doublement de la taille d'echantillon
		nSampleSize *= 2;
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestUnbalancedCenterPureInterval()
{
	const int nSampleSize = 10000;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	KWFrequencyTable table;
	KWFrequencyTable* targetTable;
	int nIntervalNumber;
	int nFirstClassFrequency;
	int nIntervalSize;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Libelles des discretizers
	cout << "Size\tFirst class";
	for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
	{
		discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
		cout << "\t" << discretizerSpec->GetObjectLabel();
	}
	cout << endl;

	// Parcours des differentes tailles d'echantillons
	nFirstClassFrequency = 1;
	while (nFirstClassFrequency <= nSampleSize / 2)
	{
		// Affichage de la taille
		cout << nSampleSize;

		// Affichage de l'effectif de la premiere classe
		cout << "\t" << nFirstClassFrequency;

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Recherche de la taille d'intervalle min conduisant a une discretisation en plus de un
			// intervalle
			for (nIntervalSize = 1; nIntervalSize <= nFirstClassFrequency; nIntervalSize++)
			{
				// Initialisation d'une table de contingence avec intervalle pur
				InitializeCenterPureIntervalTable(&table, nSampleSize, nFirstClassFrequency,
								  nIntervalSize);

				// Discretization de cette table
				discretizerSpec->GetDiscretizer(KWType::Symbol)->Discretize(&table, targetTable);
				nIntervalNumber = targetTable->GetFrequencyVectorNumber();
				delete targetTable;

				// Si plus de deux intervalle, arret
				if (nIntervalNumber > 2)
					break;
			}

			// Affichage du resultat
			cout << "\t";
			if (nIntervalSize <= nFirstClassFrequency)
				cout << nIntervalSize;
		}
		cout << endl;

		// Modification de l'effectif de la premiere classe
		nFirstClassFrequency += 1;
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestCenterTwoPureIntervals()
{
	const int nMinSampleSize = 8;
	const int nMaxSampleSize = 1073741824;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	KWFrequencyTable table;
	KWFrequencyTable* targetTable;
	int nIntervalNumber;
	int nSampleSize;
	int nIntervalSize;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Libelles des discretizers
	cout << "Size";
	for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
	{
		discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
		cout << "\t" << discretizerSpec->GetObjectLabel();
	}
	cout << endl;

	// Parcours des differentes tailles d'echantillons
	nSampleSize = nMinSampleSize;
	while (1 <= nSampleSize and nSampleSize <= nMaxSampleSize)
	{
		// Affichage de la taille
		cout << nSampleSize;

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Recherche de la taille d'intervalle min conduisant a une discretisation en plus de un
			// intervalle
			for (nIntervalSize = 1; nIntervalSize < nSampleSize / 4; nIntervalSize++)
			{
				// Initialisation d'une table de contingence avec intervalle pur
				InitializeCenterTwoPureIntervalsTable(&table, nSampleSize, nSampleSize / 2,
								      nIntervalSize);

				// Discretization de cette table
				discretizerSpec->GetDiscretizer(KWType::Symbol)->Discretize(&table, targetTable);
				nIntervalNumber = targetTable->GetFrequencyVectorNumber();
				delete targetTable;

				// Si plus de un intervalle, arret
				if (nIntervalNumber > 3)
					break;
			}

			// Affichage du resultat
			cout << "\t" << nIntervalSize;
		}
		cout << endl;

		// Doublement de la taille d'echantillon
		nSampleSize *= 2;
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestRandomSamples()
{
	KWCSGRandom sampleGenerator;
	KWContinuousSampleDiscretizerTest sampleDiscretizerTest;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	IntVector ivSampleSizes;
	int i;
	int nBaseSampleSize = 100;
	int nSampleSize;
	int nSampleNumber = 1000000;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Initialisation des tres petites tailles d'echantillon
	ivSampleSizes.Add(1);
	ivSampleSizes.Add(2);
	ivSampleSizes.Add(4);
	ivSampleSizes.Add(5);
	ivSampleSizes.Add(6);
	ivSampleSizes.Add(7);
	ivSampleSizes.Add(8);
	ivSampleSizes.Add(9);
	ivSampleSizes.Add(10);
	ivSampleSizes.Add(12);
	ivSampleSizes.Add(13);
	ivSampleSizes.Add(14);
	ivSampleSizes.Add(15);
	ivSampleSizes.Add(16);
	ivSampleSizes.Add(20);
	ivSampleSizes.Add(30);
	ivSampleSizes.Add(40);
	ivSampleSizes.Add(50);
	ivSampleSizes.Add(60);
	ivSampleSizes.Add(70);
	ivSampleSizes.Add(80);
	ivSampleSizes.Add(90);

	// Initialisation des tailles d'echantillon
	for (i = 1; i <= 20; i++)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(25 * nBaseSampleSize);
	for (i = 30; i <= 100; i += 10)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(150 * nBaseSampleSize);
	for (i = 200; i <= 500; i += 100)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(750 * nBaseSampleSize);
	ivSampleSizes.Add(1000 * nBaseSampleSize);

	// Parcours des tailles d'echantillon
	for (i = 0; i < ivSampleSizes.GetSize(); i++)
	{
		nSampleSize = ivSampleSizes.GetAt(i);

		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Evaluation de la discretisation
			sampleDiscretizerTest.SetGenerator(&sampleGenerator);
			sampleDiscretizerTest.SetDiscretizer(discretizerSpec);
			sampleDiscretizerTest.SetSampleSize(nSampleSize);
			sampleDiscretizerTest.SetSampleNumber(nSampleNumber);
			// if (nSampleSize <= 100*nBaseSampleSize)
			//	sampleDiscretizerTest.SetSampleNumber(10*nSampleNumber);
			sampleDiscretizerTest.ComputeStats();

			// Impression d'un rapport sous forme de tableau
			if (nDiscretizer == 0 and i == 0)
			{
				sampleDiscretizerTest.WriteHeaderLineReport(cout);
				cout << endl;
			}
			sampleDiscretizerTest.WriteLineReport(cout);
			cout << endl;
		}
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::InitializeBinaryIntegerTable(KWFrequencyTable* table, int nInteger, int nRepresentationSize)
{
	int j;
	int i;
	int nCurrentFrequency;
	IntVector ivTargetValues;
	IntVector ivTargetFrequencies;
	IntVector ivTargetDistinctValues;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	// Vecteur cible : i code sur n bits
	// cout << "Codage binaire de i = " << nInteger << endl;
	// Acces aux valeurs une a une
	nCurrentFrequency = 0;
	ivTargetValues.SetSize(0);
	ivTargetDistinctValues.SetSize(0);
	ivTargetFrequencies.SetSize(0);
	for (j = nRepresentationSize - 1; j >= 0; j--)
	{
		// printf("%d", (i >> j ) & 1);
		// printf("\n");
		ivTargetValues.Add((nInteger >> j) & 1);

		// Cas de la 1ere valeur
		if (j == nRepresentationSize - 1)
		{
			nCurrentFrequency++;
		}
		// Les autres
		else
		{
			// Si changement de valeurs
			if (ivTargetValues.GetAt(ivTargetValues.GetSize() - 1) !=
			    ivTargetValues.GetAt(ivTargetValues.GetSize() - 2))
			{
				// Stockage de la valeur cible precedente et de son effectif
				ivTargetFrequencies.Add(nCurrentFrequency);
				ivTargetDistinctValues.Add(ivTargetValues.GetAt(ivTargetValues.GetSize() - 2));
				// Reinitialisation
				nCurrentFrequency = 1;
			}
			else
			{
				nCurrentFrequency++;
			}
		}
		// La derniere
		if (j == 0)
		{
			ivTargetFrequencies.Add(nCurrentFrequency);
			ivTargetDistinctValues.Add(ivTargetValues.GetAt(ivTargetValues.GetSize() - 1));
		}
	}
	// cout << "Target values " << ivTargetValues << endl;
	// cout << "Target distinct values " << ivTargetDistinctValues << endl;
	// cout << "Target frequencies " << ivTargetFrequencies << endl;
	//  Initialisation de la table de contingence
	table->SetFrequencyVectorNumber(ivTargetFrequencies.GetSize());

	// Initialisation de la taille des vecteurs de la table
	for (i = 0; i < ivTargetFrequencies.GetSize(); i++)
	{
		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(i));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(2);
	}

	// Nbre d'intervalles nbre de modalites cible + effectif total = n
	// Boucle sur les intervalles
	for (j = 0; j < table->GetFrequencyVectorNumber(); j++)
	{
		cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(j))
		    ->GetFrequencyVector()
		    ->SetAt(ivTargetDistinctValues.GetAt(j), ivTargetFrequencies.GetAt(j));
	}
	assert(table->GetTotalFrequency() == nRepresentationSize);
}

// CH V9
// void KWDiscretizerTest::TestNewPriorThreshold()
//{
//	boolean dDisplayResults = false;
//	KWFrequencyTable kwftTable;
//	KWFrequencyTable* targetTable;
//	KWFrequencyTable nullContingencyTable;
//	KWFrequencyTable* nullPreparedTable;
//	KWDiscretizerMODL discretizerMODL;
//	ContinuousVector cvInformativeConfigurationsOldPrior;
//	ContinuousVector cvInformativeConfigurationsNewPrior;
//	ContinuousVector cvInformativeConfigurationsNewPriorMultinomial;
//	ContinuousVector cvInformativeConfigurationsNewPriorGranularite;
//	ContinuousVector cvInformativeConfigurationsNewPriorGranulariteMultinomial;
//	IntVector ivMaxIntervalNumberOldPrior;
//	IntVector ivMaxIntervalNumberNewPrior;
//	IntVector ivMaxIntervalNumberNewPriorMultinomial;
//	IntVector ivMaxIntervalNumberNewPriorGranularite;
//	IntVector ivMaxIntervalNumberNewPriorGranulariteMultinomial;
//	IntVector ivIntervalNumberOldPrior; // nbre de config avec tant d'intervalles
//	IntVector ivIntervalNumberNewPrior;
//	IntVector ivIntervalNumberNewPriorMultinomial;
//	IntVector ivIntervalNumberNewPriorGranularite;
//	IntVector ivIntervalNumberNewPriorGranulariteMultinomial;
//	int nSampleSize;
//	int nSampleSizeMin = 26;
//	int nSampleSizeMax = 30;
//	int nConfigurationNumber;
//	int i;
//	int j;
//	int nValueNumber;
//	int nIntervalNumber;
//	double dNullCost;
//	double dBestCost;
//	double dDataCost;
//	double dAveragedBestCostOldPrior;
//	double dAveragedBestCostNewPrior;
//	double dAveragedBestCostNewPriorMultinomial;
//	double dAveragedBestCostNewPriorGranularite;
//	double dAveragedBestCostNewPriorGranulariteMultinomial;
//	double dAveragedNullCostOldPrior;
//	double dAveragedNullCostNewPrior;
//	double dAveragedNullCostNewPriorMultinomial;
//	double dAveragedNullCostNewPriorGranularite;
//	double dAveragedNullCostNewPriorGranulariteMultinomial;
//	double dAveragedLevelOldPrior;
//	double dAveragedLevelNewPrior;
//	double dAveragedLevelNewPriorMultinomial;
//	double dAveragedLevelNewPriorGranularite;
//	double dAveragedLevelNewPriorGranulariteMultinomial;
//	double dAveragedGranularityNewPrior;
//	double dAveragedGranularityMultinomialNewPrior;
//	ContinuousVector cvDataCostOldPrior;
//	ContinuousVector cvDataCostNewPrior;
//	ContinuousVector cvDataCostNewPriorMultinomial;
//	ContinuousVector cvDataCostNewPriorGranularite;
//	ContinuousVector cvDataCostNewPriorGranulariteMultinomial;
//	ContinuousVector cvBestCostOldPrior;
//	ContinuousVector cvBestCostNewPrior;
//	ContinuousVector cvBestCostNewPriorMultinomial;
//	ContinuousVector cvBestCostNewPriorGranularite;
//	ContinuousVector cvBestCostNewPriorGranulariteMultinomial;
//	ContinuousVector cvNullCostOldPrior;
//	ContinuousVector cvNullCostNewPrior;
//	ContinuousVector cvNullCostNewPriorMultinomial;
//	ContinuousVector cvNullCostNewPriorGranularite;
//	ContinuousVector cvNullCostNewPriorGranulariteMultinomial;
//	ContinuousVector cvLevelOldPrior;
//	ContinuousVector cvLevelNewPrior;
//	ContinuousVector cvLevelNewPriorMultinomial;
//	ContinuousVector cvLevelNewPriorGranularite;
//	ContinuousVector cvLevelNewPriorGranulariteMultinomial;
//	ContinuousVector cvInformativeDataCostOldPrior;
//	ContinuousVector cvInformativeDataCostNewPrior;
//	ContinuousVector cvInformativeDataCostNewPriorMultinomial;
//	ContinuousVector cvInformativeDataCostNewPriorGranularite;
//	ContinuousVector cvInformativeDataCostNewPriorGranulariteMultinomial;
//	ContinuousVector cvAverageGranularities;
//	ContinuousVector cvAverageGranularitiesMultinomial;
//	ObjectArray oaIntervalNumberDistributionOldPrior;
//	ObjectArray oaIntervalNumberDistributionNewPrior;
//	ObjectArray oaIntervalNumberDistributionNewPriorMultinomial;
//	ObjectArray oaIntervalNumberDistributionNewPriorGranularite;
//	ObjectArray oaIntervalNumberDistributionNewPriorGranulariteMultinomial;
//	IntVector* ivDistribution;
//	KWDenseFrequencyVector* kwdfvFrequencyVector;
//	IntVector* ivFrequencyVector;
//
//	// Initialisation des discretizer
//	//InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);
//	cvInformativeConfigurationsOldPrior.SetSize(nSampleSizeMax);
//	cvInformativeConfigurationsNewPrior.SetSize(nSampleSizeMax);
//	cvInformativeConfigurationsNewPriorMultinomial.SetSize(nSampleSizeMax);
//	cvInformativeConfigurationsNewPriorGranularite.SetSize(nSampleSizeMax);
//	cvInformativeConfigurationsNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	ivMaxIntervalNumberOldPrior.SetSize(nSampleSizeMax);
//	ivMaxIntervalNumberNewPrior.SetSize(nSampleSizeMax);
//	ivMaxIntervalNumberNewPriorMultinomial.SetSize(nSampleSizeMax);
//	ivMaxIntervalNumberNewPriorGranularite.SetSize(nSampleSizeMax);
//	ivMaxIntervalNumberNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	cvDataCostOldPrior.SetSize(nSampleSizeMax);
//	cvDataCostNewPrior.SetSize(nSampleSizeMax);
//	cvDataCostNewPriorMultinomial.SetSize(nSampleSizeMax);
//	cvDataCostNewPriorGranularite.SetSize(nSampleSizeMax);
//	cvDataCostNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	cvInformativeDataCostOldPrior.SetSize(nSampleSizeMax);
//	cvInformativeDataCostNewPrior.SetSize(nSampleSizeMax);
//	cvInformativeDataCostNewPriorMultinomial.SetSize(nSampleSizeMax);
//	cvInformativeDataCostNewPriorGranularite.SetSize(nSampleSizeMax);
//	cvInformativeDataCostNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	cvBestCostOldPrior.SetSize(nSampleSizeMax);
//	cvBestCostNewPrior.SetSize(nSampleSizeMax);
//	cvBestCostNewPriorMultinomial.SetSize(nSampleSizeMax);
//	cvBestCostNewPriorGranularite.SetSize(nSampleSizeMax);
//	cvBestCostNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	cvLevelOldPrior.SetSize(nSampleSizeMax);
//	cvLevelNewPrior.SetSize(nSampleSizeMax);
//	cvLevelNewPriorMultinomial.SetSize(nSampleSizeMax);
//	cvLevelNewPriorGranularite.SetSize(nSampleSizeMax);
//	cvLevelNewPriorGranulariteMultinomial.SetSize(nSampleSizeMax);
//	cvAverageGranularities.SetSize(nSampleSizeMax);
//	cvAverageGranularitiesMultinomial.SetSize(nSampleSizeMax);
//
//	cout <<
//"Taille\tOldPrior-NbInformativeConfiguration\tNewPrior-NbInformativeConfiguration\tNewPriorMultinomial-NbInformativeConfiguration\tNewPriorGranu-NbInformativeConfiguration\tNewPriorGranuMulti-NbInformativeConfiguration"
//<<
//		"\tOldPrior-MaxI\tNewPrior-MaxI\tNewPriorMultinomial-MaxI\tNewPriorGranu-MaxI\tNewPriorGranuMulti-MaxI\tOldPrior-BestCost\tNewPrior-BestCost\tNewPriorMultinomial-BestCost\tNewPriorGranu-BestCost\tNewPriorGranuMulti-BestCost\tOldPriorLevel\tNewPriorLevel\tNewPriorMultinomialLevel\tNewPriorGranuLevel\tNewPriorGranuMultiLevel\tOldPrior-InformativeDataCost\tNewPrior-InformativeDataCost\tNewPriorMultinomial-InformativeDataCost\tNewPriorGranu-InformativeDataCost\tNewPriorGranuMulti-InformativeDataCost\tNewPriorGranu-AveragedGranularity\tNewPriorGranuMulti-AveragedGranularity"
//<< endl;
//
//	// Parcours des differentes tailles de jeux de donnes
//	for (nSampleSize = nSampleSizeMin; nSampleSize <= nSampleSizeMax; nSampleSize++)
//	{
//		dAveragedBestCostOldPrior = 0;
//		dAveragedBestCostNewPrior = 0;
//		dAveragedBestCostNewPriorMultinomial = 0;
//		dAveragedBestCostNewPriorGranularite = 0;
//		dAveragedBestCostNewPriorGranulariteMultinomial = 0;
//		dAveragedNullCostOldPrior = 0;
//		dAveragedNullCostNewPrior = 0;
//		dAveragedNullCostNewPriorMultinomial = 0;
//		dAveragedNullCostNewPriorGranularite = 0;
//		dAveragedNullCostNewPriorGranulariteMultinomial = 0;
//		dAveragedLevelOldPrior = 0;
//		dAveragedLevelNewPrior = 0;
//		dAveragedLevelNewPriorMultinomial = 0;
//		dAveragedLevelNewPriorGranularite = 0;
//		dAveragedLevelNewPriorGranulariteMultinomial = 0;
//		dAveragedGranularityNewPrior = 0;
//		dAveragedGranularityMultinomialNewPrior = 0;
//		ivIntervalNumberOldPrior.SetSize(0);
//		ivIntervalNumberNewPrior.SetSize(0);
//		ivIntervalNumberNewPriorMultinomial.SetSize(0);
//		ivIntervalNumberNewPriorGranularite.SetSize(0);
//		ivIntervalNumberNewPriorGranulariteMultinomial.SetSize(0);
//		//  Boucle parcours des differentes configurations des donnees
//		//cout << " Parcours exhaustif des jeux de donnees de taille n = " << nSampleSize << "\n";
//		nConfigurationNumber = (int)pow(2.0, nSampleSize);
//		for (i = 0; i < nConfigurationNumber; i++)
//		{
//			// Avec anciens priors
//			SetOldPriorMode(true);
//			SetPriorV9MultinomialForIntervalChoice(false);
//
//			InitializeBinaryIntegerTable(&kwftTable, i, nSampleSize);
//
//			discretizerMODL.Discretize(&kwftTable, targetTable);
//			dBestCost = discretizerMODL.ComputeDiscretizationCost(targetTable);
//			dAveragedBestCostOldPrior += dBestCost;
//
//			cvDataCostOldPrior.SetAt(nSampleSize - 1, cvDataCostOldPrior.GetAt(nSampleSize - 1) +
// discretizerMODL.ComputeDiscretizationDataCost(targetTable)); 			nIntervalNumber =
// targetTable->GetFrequencyVectorNumber(); 			if (nIntervalNumber >
// ivIntervalNumberOldPrior.GetSize())
//			{
//				ivIntervalNumberOldPrior.SetSize(nIntervalNumber);
//				ivIntervalNumberOldPrior.SetAt(nIntervalNumber - 1, 1);
//			}
//			else
//				ivIntervalNumberOldPrior.SetAt(nIntervalNumber - 1,
// ivIntervalNumberOldPrior.GetAt(nIntervalNumber - 1) + 1);
//
//			if (nIntervalNumber > ivMaxIntervalNumberOldPrior.GetAt(nSampleSize - 1))
//			{
//				ivMaxIntervalNumberOldPrior.SetAt(nSampleSize - 1, nIntervalNumber);
//			}
//
//			// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
//(modele Null) 			nValueNumber = 2; 			nullContingencyTable.Initialize(1);
//
//			// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
//			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*,
// nullContingencyTable.GetFrequencyVectorAt(0)); 			ivFrequencyVector =
// kwdfvFrequencyVector->GetFrequencyVector();
//
//			// Calcul des totaux d'effectif par modalite cible
//			kwftTable.ComputeTargetFrequencies(ivFrequencyVector);
//
//			// Utilisation du discretiseur specifie dans les pretraitements
//			discretizerMODL.Discretize(&nullContingencyTable, nullPreparedTable);
//
//			// Memorisation des couts MODL
//			dNullCost = discretizerMODL.ComputeDiscretizationConstructionCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationPreparationCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationDataCost(&nullContingencyTable);
//
//			dAveragedNullCostOldPrior += dNullCost;
//			dAveragedLevelOldPrior += (1 - dBestCost / dNullCost);
//
//			//cout << "Table initiale" << endl;
//			//cout << kwctTable;
//			//	cout << "Table resultat" << endl;
//			//cout << *targetTable;
//			//cout << "Data cost = " << dDataCost << endl;
//			//cout << "Best model cost = " << dBestCost << endl;
//			//cout << "Old prior - Null cost " << dNullCost;
//
//			// Test coherence
//			if (dDisplayResults)
//			{
//				if (dBestCost > dNullCost and nIntervalNumber > 1)
//					cout << "Anciens prior - nSampleSize =\t" << nSampleSize << "\t
// nConfigurationNumber = \t " << i
//					<< "\tProbleme partition informative de cout superieur au modele nul"
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl; 				if
// (dBestCost
// <
// dNullCost and nIntervalNumber == 1) 					cout << "Anciens prior - nSampleSize =\t" <<
// nSampleSize << "\t nConfigurationNumber =  \t " <<
// i
//					<< "\tProbleme partition non informative de cout inferieur au modele nul"
//					<< "\tnullContingencyTable" << nullContingencyTable
//					<< "\tTable a discretiser" << kwftTable
//					<< "\tdBestCost-dNullCost\t" << dBestCost - dNullCost
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl;
//			}
//
//			// Memorisation du nombre de configurations ou l'on detecte de l'information
//			//if (dNullCost > dBestCost)
//			if (nIntervalNumber > 1)
//			{
//				cvInformativeConfigurationsOldPrior.SetAt(nSampleSize - 1,
// cvInformativeConfigurationsOldPrior.GetAt(nSampleSize - 1) + 1);
// cvInformativeDataCostOldPrior.SetAt(nSampleSize - 1, cvInformativeDataCostOldPrior.GetAt(nSampleSize - 1) +
// discretizerMODL.ComputeDiscretizationDataCost(targetTable));
//			}
//			// Nettoyage
//			delete nullPreparedTable;
//			delete targetTable;
//
//			// Prior refonte 1 (sans granularite)
//			SetPriorV9MultinomialForIntervalChoice(false);
//			SetOldPriorMode(false);
//			SetPriorV9GranularityMode(false);
//			InitializeBinaryIntegerTable(&kwftTable, i, nSampleSize);
//			discretizerMODL.Discretize(&kwftTable, targetTable);
//			dBestCost = discretizerMODL.ComputeDiscretizationCost(targetTable);
//			dAveragedBestCostNewPrior += dBestCost;
//			cvDataCostNewPrior.SetAt(nSampleSize - 1, cvDataCostNewPrior.GetAt(nSampleSize - 1) +
// discretizerMODL.ComputeDiscretizationDataCost(targetTable)); 			nIntervalNumber =
// targetTable->GetFrequencyVectorNumber(); 			if (nIntervalNumber >
// ivIntervalNumberNewPrior.GetSize())
//			{
//				ivIntervalNumberNewPrior.SetSize(nIntervalNumber);
//				ivIntervalNumberNewPrior.SetAt(nIntervalNumber - 1, 1);
//			}
//			else
//				ivIntervalNumberNewPrior.SetAt(nIntervalNumber - 1,
// ivIntervalNumberNewPrior.GetAt(nIntervalNumber - 1) + 1); 			if (nIntervalNumber >
// ivMaxIntervalNumberNewPrior.GetAt(nSampleSize - 1))
//			{
//				ivMaxIntervalNumberNewPrior.SetAt(nSampleSize - 1, nIntervalNumber);
//			}
//
//			// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
//(modele Null) 			nValueNumber = 2; 			nullContingencyTable.Initialize(1);
//
//			// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
//			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*,
// nullContingencyTable.GetFrequencyVectorAt(0)); 			ivFrequencyVector =
// kwdfvFrequencyVector->GetFrequencyVector();
//
//			// Calcul des totaux d'effectif par modalite cible
//			kwftTable.ComputeTargetFrequencies(ivFrequencyVector);
//
//			// Utilisation du discretiseur specifie dans les pretraitements
//			discretizerMODL.Discretize(&nullContingencyTable, nullPreparedTable);
//
//			// Memorisation des couts MODL
//			dNullCost = discretizerMODL.ComputeDiscretizationConstructionCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationPreparationCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationDataCost(&nullContingencyTable);
//
//			dAveragedNullCostNewPrior += dNullCost;
//			dAveragedLevelNewPrior += (1 - dBestCost / dNullCost);
//			//cout << "Table initiale" << endl;
//			//cout << kwctTable;
//			//	cout << "Table resultat" << endl;
//			//cout << *targetTable;
//			//cout << "Data cost = " << dDataCost << endl;
//			//cout << "Best model cost = " << dBestCost << endl;
//			//cout << " New Prior - Null cost " << dNullCost << endl;
//
//			// Test coherence
//			if (dDisplayResults)
//			{
//				if (dBestCost > dNullCost and nIntervalNumber > 1)
//					cout << "Refonte 1 - nSampleSize =\t" << nSampleSize << "\t nConfigurationNumber
//= \t
//"
//<< i
//					<< "\tProbleme partition informative de cout superieur au modele nul"
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl; 				if
// (dBestCost
// < dNullCost and nIntervalNumber == 1) 					cout << "Refonte 1 - nSampleSize =\t" <<
// nSampleSize << "\t nConfigurationNumber =  \t " << i
//					<< "\tProbleme partition non informative de cout inferieur au modele nul"
//					<< "\tnullContingencyTable" << nullContingencyTable
//					<< "\tTable a discretiser" << kwftTable
//					<< "\tdBestCost-dNullCost\t" << dBestCost - dNullCost
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl;
//			}
//
//			// Memorisation du nombre de configurations ou l'on detecte de l'information
//			//if (dNullCost > dBestCost)
//			if (nIntervalNumber > 1)
//			{
//				cvInformativeConfigurationsNewPrior.SetAt(nSampleSize - 1,
// cvInformativeConfigurationsNewPrior.GetAt(nSampleSize - 1) + 1);
// cvInformativeDataCostNewPrior.SetAt(nSampleSize - 1, cvInformativeDataCostNewPrior.GetAt(nSampleSize - 1) +
// discretizerMODL.ComputeDiscretizationDataCost(targetTable));
//			}
//			// Nettoyage
//			delete nullPreparedTable;
//			delete targetTable;
//
//			// Prior refonte 1 (sans granularite avec choix intervalles multinomial)
//			SetPriorV9MultinomialForIntervalChoice(true);
//			SetOldPriorMode(false);
//			SetPriorV9GranularityMode(false);
//			InitializeBinaryIntegerTable(&kwftTable, i, nSampleSize);
//			discretizerMODL.Discretize(&kwftTable, targetTable);
//			dBestCost = discretizerMODL.ComputeDiscretizationCost(targetTable);
//			dAveragedBestCostNewPriorMultinomial += dBestCost;
//			cvDataCostNewPriorMultinomial.SetAt(nSampleSize - 1,
// cvDataCostNewPriorMultinomial.GetAt(nSampleSize
//- 1) + discretizerMODL.ComputeDiscretizationDataCost(targetTable)); 			nIntervalNumber =
// targetTable->GetFrequencyVectorNumber(); 			if (nIntervalNumber >
// ivIntervalNumberNewPriorMultinomial.GetSize())
//			{
//				ivIntervalNumberNewPriorMultinomial.SetSize(nIntervalNumber);
//				ivIntervalNumberNewPriorMultinomial.SetAt(nIntervalNumber - 1, 1);
//			}
//			else
//				ivIntervalNumberNewPriorMultinomial.SetAt(nIntervalNumber - 1,
// ivIntervalNumberNewPriorMultinomial.GetAt(nIntervalNumber - 1) + 1); 			if (nIntervalNumber >
// ivMaxIntervalNumberNewPriorMultinomial.GetAt(nSampleSize - 1))
//			{
//				ivMaxIntervalNumberNewPriorMultinomial.SetAt(nSampleSize - 1, nIntervalNumber);
//			}
//
//			// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
//(modele Null) 			nValueNumber = 2; 			nullContingencyTable.Initialize(1);
//
//			// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
//			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*,
// nullContingencyTable.GetFrequencyVectorAt(0)); 			ivFrequencyVector =
// kwdfvFrequencyVector->GetFrequencyVector();
//
//			// Calcul des totaux d'effectif par modalite cible
//			kwftTable.ComputeTargetFrequencies(ivFrequencyVector);
//
//			// Utilisation du discretiseur specifie dans les pretraitements
//			discretizerMODL.Discretize(&nullContingencyTable, nullPreparedTable);
//
//			// Memorisation des couts MODL
//			dNullCost = discretizerMODL.ComputeDiscretizationConstructionCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationPreparationCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationDataCost(&nullContingencyTable);
//
//			dAveragedNullCostNewPriorMultinomial += dNullCost;
//			dAveragedLevelNewPriorMultinomial += (1 - dBestCost / dNullCost);
//			//cout << "Table initiale" << endl;
//			//cout << kwctTable;
//			//	cout << "Table resultat" << endl;
//			//cout << *targetTable;
//			//cout << "Data cost = " << dDataCost << endl;
//			//cout << "Best model cost = " << dBestCost << endl;
//			//cout << " New Prior - Null cost " << dNullCost << endl;
//
//			// Test coherence
//			if (dDisplayResults)
//			{
//				if (dBestCost > dNullCost and nIntervalNumber > 1)
//					cout << "Refonte 1 Multi - nSampleSize =\t" << nSampleSize << "\t
// nConfigurationNumber = \t " << i
//					<< "\tProbleme partition informative de cout superieur au modele nul"
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl; 				if
// (dBestCost
// <
// dNullCost and nIntervalNumber == 1) 					cout << "Refonte 1 Multi - nSampleSize =\t" <<
// nSampleSize << "\t nConfigurationNumber =  \t "
//<< i
//					<< "\tProbleme partition non informative de cout inferieur au modele nul"
//					<< "\tnullContingencyTable" << nullContingencyTable
//					<< "\tTable a discretiser" << kwftTable
//					<< "\tdBestCost-dNullCost\t" << dBestCost - dNullCost
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl;
//			}
//
//			// Memorisation du nombre de configurations ou l'on detecte de l'information
//			//if (dNullCost > dBestCost)
//			if (nIntervalNumber > 1)
//			{
//				cvInformativeConfigurationsNewPriorMultinomial.SetAt(nSampleSize - 1,
// cvInformativeConfigurationsNewPriorMultinomial.GetAt(nSampleSize - 1) + 1);
//				cvInformativeDataCostNewPriorMultinomial.SetAt(nSampleSize - 1,
// cvInformativeDataCostNewPriorMultinomial.GetAt(nSampleSize - 1) +
// discretizerMODL.ComputeDiscretizationDataCost(targetTable));
//			}
//			// Nettoyage
//			delete nullPreparedTable;
//			delete targetTable;
//
//			// Prior refonte avec granularite
//			SetOldPriorMode(false);
//			SetPriorV9MultinomialForIntervalChoice(false);
//			SetPriorV9GranularityMode(true);
//			InitializeBinaryIntegerTable(&kwftTable, i, nSampleSize);
//			discretizerMODL.Discretize(&kwftTable, targetTable);
//
//			// Parametrage apres discretisation
//			// Effectue desormais dans InitializeWorkingData avec targetTable
//			/*int nGranularity = targetTable->GetGranularity();
//			int nPartileNumber;
//			int nValueNumber = kwctTable.GetTableFrequency();
//			nPartileNumber = pow(2, nGranularity);
//			if (nPartileNumber > nValueNumber)
//			nPartileNumber = nValueNumber;
//			discretizerMODL.GetGranularity()->SetPartileNumber(nPartileNumber);
//			discretizerMODL.GetGranularity()->SetCurrentGranularity(nGranularity);*/
//
//			dBestCost = discretizerMODL.ComputeDiscretizationCost(targetTable);
//			dAveragedBestCostNewPriorGranularite += dBestCost;
//			dAveragedGranularityNewPrior += targetTable->GetGranularity() * 1.0;
//			dDataCost = discretizerMODL.ComputeDiscretizationDataCost(targetTable);
//			cvDataCostNewPriorGranularite.SetAt(nSampleSize - 1,
// cvDataCostNewPriorGranularite.GetAt(nSampleSize
//- 1) + dDataCost); 			nIntervalNumber = targetTable->GetFrequencyVectorNumber(); if (nIntervalNumber >
// ivIntervalNumberNewPriorGranularite.GetSize())
//			{
//				ivIntervalNumberNewPriorGranularite.SetSize(nIntervalNumber);
//				ivIntervalNumberNewPriorGranularite.SetAt(nIntervalNumber - 1, 1);
//			}
//			else
//				ivIntervalNumberNewPriorGranularite.SetAt(nIntervalNumber - 1,
// ivIntervalNumberNewPriorGranularite.GetAt(nIntervalNumber - 1) + 1); 			if (nIntervalNumber >
// ivMaxIntervalNumberNewPriorGranularite.GetAt(nSampleSize - 1))
//			{
//				ivMaxIntervalNumberNewPriorGranularite.SetAt(nSampleSize - 1, nIntervalNumber);
//			}
//
//			// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
//(modele Null) 			nValueNumber = 2; 			nullContingencyTable.Initialize(1);
//
//			// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
//			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*,
// nullContingencyTable.GetFrequencyVectorAt(0)); 			ivFrequencyVector =
// kwdfvFrequencyVector->GetFrequencyVector();
//
//			// Calcul des totaux d'effectif par modalite cible
//			kwftTable.ComputeTargetFrequencies(ivFrequencyVector);
//
//			// Utilisation du discretiseur specifie dans les pretraitements
//			discretizerMODL.Discretize(&nullContingencyTable, nullPreparedTable);
//
//			// Memorisation des couts MODL
//			dNullCost = discretizerMODL.ComputeDiscretizationConstructionCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationPreparationCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationDataCost(&nullContingencyTable);
//
//			dAveragedNullCostNewPriorGranularite += dNullCost;
//			dAveragedLevelNewPriorGranularite += (1 - dBestCost / dNullCost);
//			//cout << "Table initiale" << endl;
//			//cout << kwctTable;
//			//	cout << "Table resultat" << endl;
//			//cout << *targetTable;
//			//cout << "Data cost = " << dDataCost << endl;
//			//cout << "Best model cost = " << dBestCost << endl;
//			//cout << " New Prior Granu - Null cost " << dNullCost << endl;
//
//			// Test coherence
//			if (dDisplayResults)
//			{
//				if (dBestCost > dNullCost and nIntervalNumber > 1)
//					cout << "Granu - nSampleSize =\t" << nSampleSize << "\t nConfigurationNumber =
//\t
//"
//<<
// i
//					<< "\tProbleme partition informative de cout superieur au modele nul"
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl; 				if
// (dBestCost
// < dNullCost and nIntervalNumber == 1) 					cout << "Granu - nSampleSize =\t" <<
// nSampleSize
// << "\t nConfigurationNumber =  \t " << i
//					<< "\tProbleme partition non informative de cout inferieur au modele nul"
//					<< "\tnullContingencyTable" << nullContingencyTable
//					<< "\tTable a discretiser" << kwftTable
//					<< "\tdBestCost-dNullCost\t" << dBestCost - dNullCost
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl;
//			}
//
//			// Memorisation du nombre de configurations ou l'on detecte de l'information
//			if (nIntervalNumber > 1)
//				//if (dNullCost > dBestCost)
//			{
//				cvInformativeConfigurationsNewPriorGranularite.SetAt(nSampleSize - 1,
// cvInformativeConfigurationsNewPriorGranularite.GetAt(nSampleSize - 1) + 1);
//				cvInformativeDataCostNewPriorGranularite.SetAt(nSampleSize - 1,
// cvInformativeDataCostNewPriorGranularite.GetAt(nSampleSize - 1) + dDataCost);
//			}
//
//			// Nettoyage
//			delete nullPreparedTable;
//			delete targetTable;
//
//			// Prior refonte avec granularite et modelisation multinomial du choix des intervalles
//			SetOldPriorMode(false);
//			SetPriorV9MultinomialForIntervalChoice(true);
//			SetPriorV9GranularityMode(true);
//			InitializeBinaryIntegerTable(&kwftTable, i, nSampleSize);
//			discretizerMODL.Discretize(&kwftTable, targetTable);
//
//			// Parametrage apres discretisation
//			// CH V9 Effectue desormais dans InitialiseWorkingData
//			/*nGranularity = discretizerMODL.GetGranularity()->GetBestGranularity();
//			nValueNumber = kwctTable.GetTableFrequency();
//			nPartileNumber = pow(2, nGranularity);
//			if (nPartileNumber > nValueNumber)
//			nPartileNumber = nValueNumber;
//			discretizerMODL.GetGranularity()->SetPartileNumber(nPartileNumber);
//			discretizerMODL.GetGranularity()->SetCurrentGranularity(nGranularity);*/
//
//			dBestCost = discretizerMODL.ComputeDiscretizationCost(targetTable);
//			dAveragedBestCostNewPriorGranulariteMultinomial += dBestCost;
//			dAveragedGranularityMultinomialNewPrior += targetTable->GetGranularity() * 1.0;
//			dDataCost = discretizerMODL.ComputeDiscretizationDataCost(targetTable);
//			cvDataCostNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1,
// cvDataCostNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1) + dDataCost); 			nIntervalNumber
// = targetTable->GetFrequencyVectorNumber(); 			if (nIntervalNumber >
// ivIntervalNumberNewPriorGranulariteMultinomial.GetSize())
//			{
//				ivIntervalNumberNewPriorGranulariteMultinomial.SetSize(nIntervalNumber);
//				ivIntervalNumberNewPriorGranulariteMultinomial.SetAt(nIntervalNumber - 1, 1);
//			}
//			else
//				ivIntervalNumberNewPriorGranulariteMultinomial.SetAt(nIntervalNumber - 1,
// ivIntervalNumberNewPriorGranulariteMultinomial.GetAt(nIntervalNumber - 1) + 1); 			if
// (nIntervalNumber > ivMaxIntervalNumberNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1))
//			{
//				ivMaxIntervalNumberNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1,
// nIntervalNumber);
//			}
//
//			// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
//(modele Null) 			nValueNumber = 2; 			nullContingencyTable.Initialize(1);
//
//			// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
//			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*,
// nullContingencyTable.GetFrequencyVectorAt(0)); 			ivFrequencyVector =
// kwdfvFrequencyVector->GetFrequencyVector();
//
//			// Calcul des totaux d'effectif par modalite cible
//			kwftTable.ComputeTargetFrequencies(ivFrequencyVector);
//
//			// Utilisation du discretiseur specifie dans les pretraitements
//			discretizerMODL.Discretize(&nullContingencyTable, nullPreparedTable);
//
//			// Memorisation des couts MODL
//			dNullCost = discretizerMODL.ComputeDiscretizationConstructionCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationPreparationCost(&nullContingencyTable)
//				+ discretizerMODL.ComputeDiscretizationDataCost(&nullContingencyTable);
//
//			dAveragedNullCostNewPriorGranulariteMultinomial += dNullCost;
//			dAveragedLevelNewPriorGranulariteMultinomial += (1 - dBestCost / dNullCost);
//			//cout << "Table initiale" << endl;
//			//cout << kwctTable;
//			//	cout << "Table resultat" << endl;
//			//cout << *targetTable;
//			//cout << "Data cost = " << dDataCost << endl;
//			//cout << "Best model cost = " << dBestCost << endl;
//			//cout << " New Prior Granu - Null cost " << dNullCost << endl;
//
//			// Test coherence
//			if (dDisplayResults)
//			{
//				if (dBestCost > dNullCost and nIntervalNumber > 1)
//					cout << "Granu Multi - nSampleSize =\t" << nSampleSize << "\t
// nConfigurationNumber = \t " << i
//					<< "\tProbleme partition informative de cout superieur au modele nul"
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl; 				if
// (dBestCost
// < dNullCost and nIntervalNumber == 1) 					cout << "Granu Multi - nSampleSize =\t"
// << nSampleSize << "\t nConfigurationNumber =  \t " << i
//					<< "\tProbleme partition non informative de cout inferieur au modele nul"
//					<< "\tnullContingencyTable" << nullContingencyTable
//					<< "\tTable a discretiser" << kwftTable
//					<< "\tdBestCost-dNullCost\t" << dBestCost - dNullCost
//					<< "\tdBestCost = \t" << DoubleToString(dBestCost) << "\tdNullCost = \t" <<
// DoubleToString(dNullCost) << "\tnIntervalNumber" << nIntervalNumber << endl;
//			}
//
//			// Memorisation du nombre de configurations ou l'on detecte de l'information
//			//if (dNullCost > dBestCost)
//			if (nIntervalNumber > 1)
//			{
//				cvInformativeConfigurationsNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1,
// cvInformativeConfigurationsNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1) + 1);
//				cvInformativeDataCostNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1,
// cvInformativeDataCostNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1) + dDataCost);
//			}
//			// Nettoyage
//			delete nullPreparedTable;
//			delete targetTable;
//		}
//		oaIntervalNumberDistributionOldPrior.Add(ivIntervalNumberOldPrior.Clone());//.Clone()
//		oaIntervalNumberDistributionNewPrior.Add(ivIntervalNumberNewPrior.Clone());//.Clone()
//		oaIntervalNumberDistributionNewPriorMultinomial.Add(ivIntervalNumberNewPriorMultinomial.Clone());//.Clone()
//		oaIntervalNumberDistributionNewPriorGranularite.Add(ivIntervalNumberNewPriorGranularite.Clone());//.Clone()
//		oaIntervalNumberDistributionNewPriorGranulariteMultinomial.Add(ivIntervalNumberNewPriorGranulariteMultinomial.Clone());//.Clone()
//
//		cvBestCostOldPrior.SetAt(nSampleSize - 1, dAveragedBestCostOldPrior / nConfigurationNumber);
//		cvBestCostNewPrior.SetAt(nSampleSize - 1, dAveragedBestCostNewPrior / nConfigurationNumber);
//		cvBestCostNewPriorMultinomial.SetAt(nSampleSize - 1, dAveragedBestCostNewPriorMultinomial /
// nConfigurationNumber); 		cvBestCostNewPriorGranularite.SetAt(nSampleSize - 1,
// dAveragedBestCostNewPriorGranularite / nConfigurationNumber);
// cvBestCostNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1, dAveragedBestCostNewPriorGranulariteMultinomial /
// nConfigurationNumber); 		cvLevelOldPrior.SetAt(nSampleSize - 1, dAveragedLevelOldPrior /
// nConfigurationNumber); 		cvLevelNewPrior.SetAt(nSampleSize - 1, dAveragedLevelNewPrior /
// nConfigurationNumber); 		cvLevelNewPriorMultinomial.SetAt(nSampleSize - 1,
// dAveragedLevelNewPriorMultinomial
// /
// nConfigurationNumber); 		cvLevelNewPriorGranularite.SetAt(nSampleSize - 1,
// dAveragedLevelNewPriorGranularite / nConfigurationNumber);
// cvLevelNewPriorGranulariteMultinomial.SetAt(nSampleSize - 1, dAveragedLevelNewPriorGranulariteMultinomial /
// nConfigurationNumber); cvAverageGranularities.SetAt(nSampleSize - 1, dAveragedGranularityNewPrior /
// nConfigurationNumber); cvAverageGranularitiesMultinomial.SetAt(nSampleSize - 1,
// dAveragedGranularityMultinomialNewPrior / nConfigurationNumber);
//		//cout << "Taille des jeux de donnees : " << nSampleSize << " Nombre de configurations informatives OLD
// prior :" << ivInformativeConfigurationsOldPrior.GetAt(nSampleSize - 1) << " Nombre de configurations informatives NEW
// prior :" << ivInformativeConfigurationsNewPrior.GetAt(nSampleSize - 1) << endl;
//		//cout << "Vraisemblance : " << nSampleSize << " OLD Prior : " << cvDataCostOldPrior.GetAt(nSampleSize -
// 1) << " NEW Prior : " << cvDataCostNewPrior.GetAt(nSampleSize - 1) << endl;
//
//		cout << nSampleSize << "\t" << cvInformativeConfigurationsOldPrior.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeConfigurationsNewPrior.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeConfigurationsNewPriorMultinomial.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeConfigurationsNewPriorGranularite.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeConfigurationsNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1)
//			<< "\t" << ivMaxIntervalNumberOldPrior.GetAt(nSampleSize - 1) << "\t" <<
// ivMaxIntervalNumberNewPrior.GetAt(nSampleSize - 1) << "\t" <<
// ivMaxIntervalNumberNewPriorMultinomial.GetAt(nSampleSize
//- 1) << "\t" << ivMaxIntervalNumberNewPriorGranularite.GetAt(nSampleSize - 1) << "\t" <<
// ivMaxIntervalNumberNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1)
//			<< "\t" << cvBestCostOldPrior.GetAt(nSampleSize - 1) << "\t" <<
// cvBestCostNewPrior.GetAt(nSampleSize
//- 1) << "\t" << cvBestCostNewPriorMultinomial.GetAt(nSampleSize - 1) << "\t" <<
// cvBestCostNewPriorGranularite.GetAt(nSampleSize - 1) << "\t" <<
// cvBestCostNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1)
//			<< "\t" << cvLevelOldPrior.GetAt(nSampleSize - 1) << "\t" << cvLevelNewPrior.GetAt(nSampleSize -
// 1)
//<< "\t" << cvLevelNewPriorMultinomial.GetAt(nSampleSize - 1) << "\t" << cvLevelNewPriorGranularite.GetAt(nSampleSize -
// 1) << "\t" << cvLevelNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1)
//			<< "\t" << cvInformativeDataCostOldPrior.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeDataCostNewPrior.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeDataCostNewPriorMultinomial.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeDataCostNewPriorGranularite.GetAt(nSampleSize - 1) << "\t" <<
// cvInformativeDataCostNewPriorGranulariteMultinomial.GetAt(nSampleSize - 1)
//			<< "\t" << cvAverageGranularities.GetAt(nSampleSize - 1) << "\t" <<
// cvAverageGranularitiesMultinomial.GetAt(nSampleSize - 1)
//			<< endl;
//	}
//
//	cout <<
//"Taille\tOldPrior-NbInformativeConfiguration\tNewPrior-NbInformativeConfiguration\tNewPriorMultinomial-NbInformativeConfiguration\tNewPriorGranu-NbInformativeConfiguration\tNewPriorGranuMulti-NbInformativeConfiguration"
//<<
//		"\tOldPrior-MaxI\tNewPrior-MaxI\tNewPriorMultinomial-MaxI\tNewPriorGranu-MaxI\tNewPriorGranuMulti-MaxI\tOldPrior-BestCost\tNewPrior-BestCost\tNewPriorMultinomial-BestCost\tNewPriorGranu-BestCost\tNewPriorGranuMulti-BestCost\tOldPriorLevel\tNewPriorLevel\tNewPriorMultinomialLevel\tNewPriorGranuLevel\tNewPriorGranuMultiLevel\tOldPrior-InformativeDataCost\tNewPrior-InformativeDataCost\tNewPriorMultinomial-InformativeDataCost\tNewPriorGranu-InformativeDataCost\tNewPriorGranuMulti-InformativeDataCost\tNewPriorGranu-AveragedGranularity\tNewPriorGranuMulti-AveragedGranularity"
//<< endl;
//
//	for (j = 0; j < cvInformativeConfigurationsNewPrior.GetSize(); j++)
//	{
//		cout << j + 1 << "\t" << cvInformativeConfigurationsOldPrior.GetAt(j) << "\t" <<
// cvInformativeConfigurationsNewPrior.GetAt(j) << "\t" << cvInformativeConfigurationsNewPriorMultinomial.GetAt(j) <<
//"\t" << cvInformativeConfigurationsNewPriorGranularite.GetAt(j) << "\t" <<
// cvInformativeConfigurationsNewPriorGranulariteMultinomial.GetAt(j)
//			<< "\t" << ivMaxIntervalNumberOldPrior.GetAt(j) << "\t" << ivMaxIntervalNumberNewPrior.GetAt(j)
//<<
//"\t" << ivMaxIntervalNumberNewPriorMultinomial.GetAt(j) << "\t" << ivMaxIntervalNumberNewPriorGranularite.GetAt(j) <<
//"\t" << ivMaxIntervalNumberNewPriorGranulariteMultinomial.GetAt(j)
//			<< "\t" << cvBestCostOldPrior.GetAt(j) << "\t" << cvBestCostNewPrior.GetAt(j) << "\t" <<
// cvBestCostNewPriorMultinomial.GetAt(j) << "\t" << cvBestCostNewPriorGranularite.GetAt(j) << "\t" <<
// cvBestCostNewPriorGranulariteMultinomial.GetAt(j)
//			<< "\t" << cvLevelOldPrior.GetAt(j) << "\t" << cvLevelNewPrior.GetAt(j) << "\t" <<
// cvLevelNewPriorMultinomial.GetAt(j) << "\t" << cvLevelNewPriorGranularite.GetAt(j) << "\t" <<
// cvLevelNewPriorGranulariteMultinomial.GetAt(j)
//			<< "\t" << cvInformativeDataCostOldPrior.GetAt(j) << "\t" <<
// cvInformativeDataCostNewPrior.GetAt(j)
//<< "\t" << cvInformativeDataCostNewPriorMultinomial.GetAt(j) << "\t" <<
// cvInformativeDataCostNewPriorGranularite.GetAt(j) << "\t" <<
// cvInformativeDataCostNewPriorGranulariteMultinomial.GetAt(j)
//			<< "\t" << cvAverageGranularities.GetAt(j) << "\t" << cvAverageGranularitiesMultinomial.GetAt(j)
//			<< endl;
//	}
//	cout << " Distribution du nombre d'intervalles " << endl;
//	for (j = 0; j < oaIntervalNumberDistributionOldPrior.GetSize(); j++)
//	{
//		ivDistribution = cast(IntVector*, oaIntervalNumberDistributionOldPrior.GetAt(j));
//		nConfigurationNumber = (long int)pow(2.0, j + 2);
//		cout << j + 2 << "\tOld";
//		for (i = 0; i < ivDistribution->GetSize(); i++)
//		{
//			cout << "\t" << ivDistribution->GetAt(i) *1.0 / nConfigurationNumber;
//		}
//		cout << endl;
//		delete ivDistribution;
//	}
//	for (j = 0; j < oaIntervalNumberDistributionNewPrior.GetSize(); j++)
//	{
//		ivDistribution = cast(IntVector*, oaIntervalNumberDistributionNewPrior.GetAt(j));
//		nConfigurationNumber = (long int)pow(2.0, j + 2);
//		cout << j + 2 << "\tNew";
//		for (i = 0; i < ivDistribution->GetSize(); i++)
//		{
//			cout << "\t" << ivDistribution->GetAt(i) * 1.0 / nConfigurationNumber;
//		}
//		cout << endl;
//		delete ivDistribution;
//	}
//	for (j = 0; j < oaIntervalNumberDistributionNewPriorMultinomial.GetSize(); j++)
//	{
//		ivDistribution = cast(IntVector*, oaIntervalNumberDistributionNewPriorMultinomial.GetAt(j));
//		nConfigurationNumber = (long int)pow(2.0, j + 2);
//		cout << j + 2 << "\tNewMultinomial";
//		for (i = 0; i < ivDistribution->GetSize(); i++)
//		{
//			cout << "\t" << ivDistribution->GetAt(i) * 1.0 / nConfigurationNumber;
//		}
//		cout << endl;
//		delete ivDistribution;
//	}
//	for (j = 0; j < oaIntervalNumberDistributionNewPriorGranularite.GetSize(); j++)
//	{
//		ivDistribution = cast(IntVector*, oaIntervalNumberDistributionNewPriorGranularite.GetAt(j));
//		nConfigurationNumber = (long int)pow(2.0, j + 2);
//		cout << j + 2 << "\tNewGranu";
//		for (i = 0; i < ivDistribution->GetSize(); i++)
//		{
//			cout << "\t" << ivDistribution->GetAt(i) * 1.0 / nConfigurationNumber;
//		}
//		cout << endl;
//		delete ivDistribution;
//	}
//	for (j = 0; j < oaIntervalNumberDistributionNewPriorGranulariteMultinomial.GetSize(); j++)
//	{
//		ivDistribution = cast(IntVector*, oaIntervalNumberDistributionNewPriorGranulariteMultinomial.GetAt(j));
//		nConfigurationNumber = (long int)pow(2.0, j + 2);
//		cout << j + 2 << "\tNewGranuMulti";
//		for (i = 0; i < ivDistribution->GetSize(); i++)
//		{
//			cout << "\t" << ivDistribution->GetAt(i) * 1.0 / nConfigurationNumber;
//		}
//		cout << endl;
//		delete ivDistribution;
//	}
//}
//
// void KWDiscretizerTest::TestRandomSamplesForPriorV9()
//{
//	KWCSGRandom sampleGenerator;
//	KWContinuousSampleDiscretizerTest sampleDiscretizerTest;
//	ObjectArray oaDiscretizerSpecs;
//	KWDiscretizerSpec* discretizerSpec;
//	int nDiscretizer;
//	IntVector ivSampleSizes;
//	int i;
//	int nBaseSampleSize = 100;
//	int nSampleSize;
//	int nSampleNumber = 10000;//1000000;
//	ALString sPrefixFileName = "RandomSamples";
//	ALString sSuffixName1 = "AvantRefonte.xls";
//	ALString sSuffixName2 = "Refonte1-SansGranularite.xls";
//	ALString sSuffixName3 = "Refonte2-AvecGranularite.xls";
//	ALString sFileName;
//	boolean bOk;
//	fstream fst1;
//	fstream fst2;
//	fstream fst3;
//
//	// Initialisation des discretizer
//	// 1 seul discretiseur : MODL
//	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);
//
//	// Initialisation des tres petites tailles d'echantillon
//	ivSampleSizes.Add(1);
//	ivSampleSizes.Add(2);
//	ivSampleSizes.Add(4);
//	ivSampleSizes.Add(5);
//	ivSampleSizes.Add(6);
//	ivSampleSizes.Add(7);
//	ivSampleSizes.Add(8);
//	ivSampleSizes.Add(9);
//	ivSampleSizes.Add(10);
//	ivSampleSizes.Add(12);
//	ivSampleSizes.Add(13);
//	ivSampleSizes.Add(14);
//	ivSampleSizes.Add(15);
//	ivSampleSizes.Add(16);
//	ivSampleSizes.Add(20);
//	ivSampleSizes.Add(30);
//	ivSampleSizes.Add(40);
//	ivSampleSizes.Add(50);
//	ivSampleSizes.Add(60);
//	ivSampleSizes.Add(70);
//	ivSampleSizes.Add(80);
//	ivSampleSizes.Add(90);
//
//	// Initialisation des tailles d'echantillon
//	for (i = 1; i <= 20; i++)
//		ivSampleSizes.Add(i*nBaseSampleSize);
//	ivSampleSizes.Add(25 * nBaseSampleSize);
//	for (i = 30; i <= 100; i += 10)
//		ivSampleSizes.Add(i*nBaseSampleSize);
//	ivSampleSizes.Add(150 * nBaseSampleSize);
//	for (i = 200; i <= 500; i += 100)
//		ivSampleSizes.Add(i*nBaseSampleSize);
//	ivSampleSizes.Add(750 * nBaseSampleSize);
//	ivSampleSizes.Add(1000 * nBaseSampleSize);
//
//	// pour tester une seule taille en Debug
//	//ivSampleSizes.SetSize(0);
//	//ivSampleSizes.Add(100);
//
//	// Ouverture des fichiers pour les trois modes de discretiseur
//	sFileName = sPrefixFileName + "-SampleNumber-" + KWContinuous::ContinuousToString(nSampleNumber) + "-" +
// sSuffixName1; 	bOk = FileService::OpenOutputFile(sFileName, fst1);
//
//	sFileName = sPrefixFileName + "-SampleNumber-" + KWContinuous::ContinuousToString(nSampleNumber) + "-" +
// sSuffixName2; 	bOk = FileService::OpenOutputFile(sFileName, fst2);
//
//	sFileName = sPrefixFileName + "-SampleNumber-" + KWContinuous::ContinuousToString(nSampleNumber) + "-" +
// sSuffixName3; 	bOk = FileService::OpenOutputFile(sFileName, fst3);
//
//	// Parcours des tailles d'echantillon
//	for (i = 0; i < ivSampleSizes.GetSize(); i++)
//	{
//		nSampleSize = ivSampleSizes.GetAt(i);
//
//		// Parcours des discretizers
//		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
//		{
//			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));
//
//			// Parametrage de la discretisation
//			sampleDiscretizerTest.SetGenerator(&sampleGenerator);
//			sampleDiscretizerTest.SetDiscretizer(discretizerSpec);
//			sampleDiscretizerTest.SetSampleSize(nSampleSize);
//			sampleDiscretizerTest.SetSampleNumber(nSampleNumber);
//			//if (nSampleSize <= 100*nBaseSampleSize)
//			//	sampleDiscretizerTest.SetSampleNumber(10*nSampleNumber);
//
//			// Avec anciens priors
//			SetOldPriorMode(true);
//			SetPriorV9GranularityMode(false);
//
//			// Evaluation de la discretisation
//			sampleDiscretizerTest.ComputeStats();
//
//			// Impression d'un rapport sous forme de tableau
//			if (nDiscretizer == 0 and i == 0)
//			{
//				sampleDiscretizerTest.WriteHeaderLineReport(fst1);
//				fst1 << endl;
//			}
//			sampleDiscretizerTest.WriteLineReport(fst1);
//			fst1 << endl;
//
//			// Prior refonte 1 (sans granularite)
//			SetOldPriorMode(false);
//			SetPriorV9GranularityMode(false);
//
//			// Evaluation de la discretisation
//			sampleDiscretizerTest.ComputeStats();
//
//			// Impression d'un rapport sous forme de tableau
//			if (nDiscretizer == 0 and i == 0)
//			{
//				sampleDiscretizerTest.WriteHeaderLineReport(fst2);
//				fst2 << endl;
//			}
//			sampleDiscretizerTest.WriteLineReport(fst2);
//			fst2 << endl;
//
//			// Prior refonte 2 (avec granularite)
//			SetOldPriorMode(false);
//			SetPriorV9GranularityMode(true);
//
//			// Evaluation de la discretisation
//			sampleDiscretizerTest.ComputeStats();
//
//			// Impression d'un rapport sous forme de tableau
//			if (nDiscretizer == 0 and i == 0)
//			{
//				sampleDiscretizerTest.WriteHeaderLineReport(fst3);
//				fst3 << endl;
//			}
//			sampleDiscretizerTest.WriteLineReport(fst3);
//			fst3 << endl;
//		}
//	}
//
//	// Fermeture fichiers
//	fst1.close();
//	fst2.close();
//	fst3.close();
//	// Nettoyage
//	oaDiscretizerSpecs.DeleteAll();
//}

void KWDiscretizerTest::TestSinusSignSamples()
{
	KWCSGSinusSign sampleGenerator;
	KWContinuousSampleDiscretizerTest sampleDiscretizerTest;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	int nSinusoidFrequency = 50;
	DoubleVector dvNoiseProbs;
	IntVector ivSampleSizes;
	int nSampleNumber = 100; // 100000;
	int i;
	int j;

	// Parametrage du generateur
	sampleGenerator.SetSinusoidFrequency(nSinusoidFrequency);
	sampleGenerator.SetNoiseProb(0);

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Initialisation des taux de bruits
	// for (i = 0; i <= 10; i++)
	//	dvNoiseProbs.Add(i*1.0/10);
	dvNoiseProbs.Add(0);

	// Initialisation des tres petites tailles d'echantillon
	ivSampleSizes.Add(1);
	ivSampleSizes.Add(2);
	ivSampleSizes.Add(4);
	ivSampleSizes.Add(5);
	ivSampleSizes.Add(6);
	ivSampleSizes.Add(7);
	ivSampleSizes.Add(8);
	ivSampleSizes.Add(9);
	ivSampleSizes.Add(10);
	ivSampleSizes.Add(12);
	ivSampleSizes.Add(15);
	ivSampleSizes.Add(20);
	ivSampleSizes.Add(30);
	ivSampleSizes.Add(40);
	ivSampleSizes.Add(50);
	ivSampleSizes.Add(60);
	ivSampleSizes.Add(70);
	ivSampleSizes.Add(80);
	ivSampleSizes.Add(90);

	// Initialisation des tailles d'echantillon
	for (i = 1; i <= 20; i++)
		ivSampleSizes.Add(i * 2 * nSinusoidFrequency);
	ivSampleSizes.Add(25 * 2 * nSinusoidFrequency);
	for (i = 30; i <= 100; i += 10)
		ivSampleSizes.Add(i * 2 * nSinusoidFrequency);
	ivSampleSizes.Add(150 * 2 * nSinusoidFrequency);
	for (i = 200; i <= 500; i += 100)
		ivSampleSizes.Add(i * 2 * nSinusoidFrequency);
	ivSampleSizes.Add(750 * 2 * nSinusoidFrequency);
	ivSampleSizes.Add(1000 * 2 * nSinusoidFrequency);

	// Etude sur la transition de reconnaissance du motif
	ivSampleSizes.SetSize(0);
	for (i = 600; i <= 900; i += 10)
		ivSampleSizes.Add(i);

	// Parcours des taux de bruit
	for (i = 0; i < dvNoiseProbs.GetSize(); i++)
	{
		// Parcours des tailles d'echantillon
		for (j = 0; j < ivSampleSizes.GetSize(); j++)
		{
			// Parcours des discretizers
			for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
			{
				discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

				// Evaluation de la discretisation
				sampleGenerator.SetNoiseProb(dvNoiseProbs.GetAt(i));
				sampleDiscretizerTest.SetGenerator(&sampleGenerator);
				sampleDiscretizerTest.SetDiscretizer(discretizerSpec);
				sampleDiscretizerTest.SetSampleSize(ivSampleSizes.GetAt(j));
				sampleDiscretizerTest.SetSampleNumber(nSampleNumber);
				sampleDiscretizerTest.ComputeStats();

				// Impression d'un rapport sous forme de tableau
				if (nDiscretizer == 0 and i == 0 and j == 0)
				{
					sampleDiscretizerTest.WriteHeaderLineReport(cout);
					cout << endl;
				}
				sampleDiscretizerTest.WriteLineReport(cout);
				cout << endl;
			}
		}
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestLinearProbSamples()
{
	KWCSGLinearProb sampleGenerator;
	KWContinuousSampleDiscretizerTest sampleDiscretizerTest;
	ObjectArray oaDiscretizerSpecs;
	KWDiscretizerSpec* discretizerSpec;
	int nDiscretizer;
	IntVector ivSampleSizes;
	int nSampleNumber = 1000;
	int nBaseSampleSize = 100;
	int i;

	// Initialisation des discretizer
	InitializeDiscretizerSpecArray(&oaDiscretizerSpecs);

	// Initialisation des tailles d'echantillon
	for (i = 1; i <= 20; i++)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(25 * nBaseSampleSize);
	for (i = 30; i <= 100; i += 10)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(150 * nBaseSampleSize);
	for (i = 200; i <= 500; i += 100)
		ivSampleSizes.Add(i * nBaseSampleSize);
	ivSampleSizes.Add(750 * nBaseSampleSize);
	ivSampleSizes.Add(1000 * nBaseSampleSize);

	// Parcours des tailles d'echantillon
	for (i = 0; i < ivSampleSizes.GetSize(); i++)
	{
		// Parcours des discretizers
		for (nDiscretizer = 0; nDiscretizer < oaDiscretizerSpecs.GetSize(); nDiscretizer++)
		{
			discretizerSpec = cast(KWDiscretizerSpec*, oaDiscretizerSpecs.GetAt(nDiscretizer));

			// Evaluation de la discretisation
			sampleDiscretizerTest.SetGenerator(&sampleGenerator);
			sampleDiscretizerTest.SetDiscretizer(discretizerSpec);
			sampleDiscretizerTest.SetSampleSize(ivSampleSizes.GetAt(i));
			sampleDiscretizerTest.SetSampleNumber(nSampleNumber);
			sampleDiscretizerTest.ComputeStats();

			// Impression d'un rapport sous forme de tableau
			if (nDiscretizer == 0 and i == 0)
			{
				sampleDiscretizerTest.WriteHeaderLineReport(cout);
				cout << endl;
			}
			sampleDiscretizerTest.WriteLineReport(cout);
			cout << endl;
		}
	}

	// Nettoyage
	oaDiscretizerSpecs.DeleteAll();
}

void KWDiscretizerTest::TestAll()
{
	/*
	cout << "UnbalancedHeadPureInterval\n";
	TestUnbalancedHeadPureInterval();
	cout << "UnbalancedCenterPureInterval\n";
	TestUnbalancedCenterPureInterval();
	cout << endl;
	*/
	cout << "HeadPureInterval\n";
	TestHeadPureInterval();
	cout << "CenterPureInterval\n";
	TestCenterPureInterval();
	// cout << "CenterTwoPureIntervals\n";
	// TestCenterTwoPureIntervals();
	cout << endl;
}

void KWDiscretizerTest::InitializeHeadPureIntervalTable(KWFrequencyTable* table, int nSampleSize,
							int nFirstClassFrequency, int nIntervalSize)
{
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int nSource;

	require(table != NULL);
	require(nSampleSize >= 0);
	require(0 <= nFirstClassFrequency and nFirstClassFrequency <= nSampleSize);
	require(0 <= nIntervalSize and nIntervalSize <= nFirstClassFrequency);

	// Initialisation de la table
	table->SetFrequencyVectorNumber(2);

	// Parametrage de la taille des vecteurs de la table d'effectifs
	for (nSource = 0; nSource < 2; nSource++)
	{
		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(nSource));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(2);
	}

	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))->GetFrequencyVector()->SetAt(0, nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))->GetFrequencyVector()->SetAt(1, 0);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))
	    ->GetFrequencyVector()
	    ->SetAt(0, nFirstClassFrequency - nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))
	    ->GetFrequencyVector()
	    ->SetAt(1, nSampleSize - 2 * nIntervalSize - nFirstClassFrequency);

	assert(table->GetTotalFrequency() == nSampleSize);
}

void KWDiscretizerTest::InitializeCenterPureIntervalTable(KWFrequencyTable* table, int nSampleSize,
							  int nFirstClassFrequency, int nIntervalSize)
{
	int nBeginIntervalSize;
	int nEndIntervalSize;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int nFrequencyZeroZero;
	int nFrequencyTwoZero;
	int nSource;

	require(table != NULL);
	require(nSampleSize >= 0);
	require(0 <= nFirstClassFrequency and nFirstClassFrequency <= nSampleSize);
	require(0 <= nIntervalSize and nIntervalSize <= nFirstClassFrequency);

	// Calcul des tailles des intervalles de debut et fin
	nBeginIntervalSize = (nSampleSize - nIntervalSize) / 2;
	nEndIntervalSize = (nSampleSize - nIntervalSize) - nBeginIntervalSize;

	// Initialisation de la table
	table->SetFrequencyVectorNumber(3);

	// Parametrage de la taille des vecteurs de la table d'effectifs
	for (nSource = 0; nSource < 3; nSource++)
	{
		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(nSource));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(2);
	}

	nFrequencyZeroZero = (nFirstClassFrequency - nIntervalSize) / 2;
	nFrequencyTwoZero = nFirstClassFrequency - nFrequencyZeroZero - nIntervalSize;

	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))
	    ->GetFrequencyVector()
	    ->SetAt(0, nFrequencyZeroZero);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))
	    ->GetFrequencyVector()
	    ->SetAt(1, nBeginIntervalSize - nFrequencyZeroZero);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))->GetFrequencyVector()->SetAt(0, nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))->GetFrequencyVector()->SetAt(1, 0);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(2))
	    ->GetFrequencyVector()
	    ->SetAt(0, nFirstClassFrequency - nFrequencyZeroZero - nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(2))
	    ->GetFrequencyVector()
	    ->SetAt(1, nEndIntervalSize - nFrequencyTwoZero);

	assert(table->GetTotalFrequency() == nSampleSize);
}

void KWDiscretizerTest::InitializeCenterTwoPureIntervalsTable(KWFrequencyTable* table, int nSampleSize,
							      int nFirstClassFrequency, int nIntervalSize)
{
	int nBeginIntervalSize;
	int nEndIntervalSize;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int nFrequencyZeroZero;
	int nFrequencyThreeZero;
	int nSource;

	require(table != NULL);
	require(nSampleSize >= 0);
	require(0 <= nFirstClassFrequency and nFirstClassFrequency <= nSampleSize);
	require(0 <= nIntervalSize and nIntervalSize <= nFirstClassFrequency);
	require(nIntervalSize <= nSampleSize - nFirstClassFrequency);

	// Calcul des tailles des intervalles de debut et fin
	nBeginIntervalSize = nSampleSize / 2 - nIntervalSize;
	nEndIntervalSize = (nSampleSize - 2 * nIntervalSize) - nBeginIntervalSize;

	// Initialisation de la table
	table->SetFrequencyVectorNumber(4);

	// Parametrage de la taille des vecteurs de la table d'effectifs
	for (nSource = 0; nSource < 4; nSource++)
	{
		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(nSource));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(2);
	}

	nFrequencyZeroZero = (nFirstClassFrequency - nIntervalSize) / 2;
	nFrequencyThreeZero = nFirstClassFrequency - nFrequencyZeroZero - nIntervalSize;

	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))
	    ->GetFrequencyVector()
	    ->SetAt(0, nFrequencyZeroZero);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(0))
	    ->GetFrequencyVector()
	    ->SetAt(1, nBeginIntervalSize - nFrequencyZeroZero);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))->GetFrequencyVector()->SetAt(0, nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(1))->GetFrequencyVector()->SetAt(1, 0);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(2))->GetFrequencyVector()->SetAt(0, 0);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(2))->GetFrequencyVector()->SetAt(1, nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(3))
	    ->GetFrequencyVector()
	    ->SetAt(0, nFirstClassFrequency - nFrequencyZeroZero - nIntervalSize);
	cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(3))
	    ->GetFrequencyVector()
	    ->SetAt(1, nEndIntervalSize - nFrequencyThreeZero);

	assert(table->GetTotalFrequency() == nSampleSize);
}

void KWDiscretizerTest::InitializeDiscretizerSpecArray(ObjectArray* oaDiscretizerSpecs)
{
	KWDiscretizerSpec* discretizerSpec;

	require(oaDiscretizerSpecs != NULL);
	require(oaDiscretizerSpecs->GetSize() == 0);

	// Ajout de la methode MODL
	discretizerSpec = new KWDiscretizerSpec;
	discretizerSpec->SetSupervisedMethodName("MODL");
	discretizerSpec->SetParam(0);
	oaDiscretizerSpecs->Add(discretizerSpec);

	/*
	  // Ajout de la methode MODL
	  discretizerSpec = new KWDiscretizerSpec;
	  discretizerSpec->SetSupervisedMethodName("MODL");
	  discretizerSpec->SetParam(0);
	  oaDiscretizerSpecs->Add(discretizerSpec);
	  */

	/*
	  // Ajout de la methode MODLEqualFrequency
	  discretizerSpec = new KWDiscretizerSpec;
	  discretizerSpec->SetSupervisedMethodName("MODLEqualFrequency");
	  discretizerSpec->SetParam(0);
	  oaDiscretizerSpecs->Add(discretizerSpec);

	  // Ajout de la methode MODLEqualWidth
	  discretizerSpec = new KWDiscretizerSpec;
	  discretizerSpec->SetSupervisedMethodName("MODLEqualWidth");
	  discretizerSpec->SetParam(0);
	  oaDiscretizerSpecs->Add(discretizerSpec);

	  // Ajout de la methode MDLPM
	  discretizerSpec = new KWDiscretizerSpec;
	  discretizerSpec->SetSupervisedMethodName("MDLPM");
	  discretizerSpec->SetParam(0);
	  oaDiscretizerSpecs->Add(discretizerSpec);
	  */
}
// Fin a supprimer
