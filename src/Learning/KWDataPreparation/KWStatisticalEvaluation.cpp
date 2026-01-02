// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWStatisticalEvaluation.h"

KWStatisticalEvaluation::KWStatisticalEvaluation()
{
	nExperimentNumber = 0;
	nRunNumber = 0;
	bMaximization = true;
	dSignificanceLevel = 0.05;
}

KWStatisticalEvaluation::~KWStatisticalEvaluation()
{
	oaExperimentValueVectors.DeleteAll();
}

void KWStatisticalEvaluation::SetCriterionName(const ALString& sValue)
{
	sCriterionName = sValue;
}

const ALString& KWStatisticalEvaluation::GetCriterionName() const
{
	return sCriterionName;
}

void KWStatisticalEvaluation::SetMethodName(const ALString& sValue)
{
	sMethodName = sValue;
}

const ALString& KWStatisticalEvaluation::GetMethodName() const
{
	return sMethodName;
}

void KWStatisticalEvaluation::SetExperimentName(const ALString& sValue)
{
	sExperimentName = sValue;
}

const ALString& KWStatisticalEvaluation::GetExperimentName() const
{
	return sExperimentName;
}

void KWStatisticalEvaluation::SetMaximization(boolean bValue)
{
	bMaximization = bValue;
}

boolean KWStatisticalEvaluation::GetMaximization() const
{
	return bMaximization;
}

void KWStatisticalEvaluation::SetExperimentNumber(int nValue)
{
	int nExperiment;
	DoubleVector* dvValues;

	require(0 <= nValue);

	// Nettoyage prealable
	oaExperimentValueVectors.DeleteAll();
	svExperimentLabels.SetSize(0);

	// Memorisation du nombre d'experience
	nExperimentNumber = nValue;

	// Initialisation des libelles
	svExperimentLabels.SetSize(nExperimentNumber);

	// Initialisation des valeurs
	if (nExperimentNumber > 0 and nRunNumber > 0)
	{
		oaExperimentValueVectors.SetSize(nExperimentNumber);

		// Creation des vecteurs de valeur
		for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
		{
			dvValues = new DoubleVector;
			dvValues->SetSize(nRunNumber);
			oaExperimentValueVectors.SetAt(nExperiment, dvValues);
		}
	}
}

int KWStatisticalEvaluation::GetExperimentNumber() const
{
	return nExperimentNumber;
}

void KWStatisticalEvaluation::SetRunNumber(int nValue)
{
	int nExperiment;
	DoubleVector* dvValues;

	require(0 <= nValue);

	// Nettoyage prealable
	oaExperimentValueVectors.DeleteAll();

	// Memorisation du nombre de repetitions
	nRunNumber = nValue;

	// Initialisation des valeurs
	if (nExperimentNumber > 0 and nRunNumber > 0)
	{
		oaExperimentValueVectors.SetSize(nExperimentNumber);

		// Creation des vecteurs de valeur
		for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
		{
			dvValues = new DoubleVector;
			dvValues->SetSize(nRunNumber);
			oaExperimentValueVectors.SetAt(nExperiment, dvValues);
		}
	}
}

int KWStatisticalEvaluation::GetRunNumber() const
{
	return nRunNumber;
}

void KWStatisticalEvaluation::ResetAllResults()
{
	nExperimentNumber = 0;
	nRunNumber = 0;
	oaExperimentValueVectors.DeleteAll();
	svExperimentLabels.SetSize(0);
}

void KWStatisticalEvaluation::InitializeAllResults()
{
	int nExperiment;
	DoubleVector* dvValues;

	// Reinitialisation des valeurs (s'il en existe)
	if (nExperimentNumber > 0 and nRunNumber > 0)
	{
		for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
		{
			dvValues = cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment));
			dvValues->Initialize();
		}
	}
}

void KWStatisticalEvaluation::SetResultAt(int nExperiment, int nRun, double dValue)
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(0 <= nRun and nRun < nRunNumber);
	cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment))->SetAt(nRun, dValue);
}

double KWStatisticalEvaluation::GetResultAt(int nExperiment, int nRun) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(0 <= nRun and nRun < nRunNumber);
	return cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment))->GetAt(nRun);
}

void KWStatisticalEvaluation::SetExperimentLabelAt(int nExperiment, const ALString& sLabel)
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	svExperimentLabels.SetAt(nExperiment, sLabel);
}

const ALString& KWStatisticalEvaluation::GetExperimentLabelAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	return svExperimentLabels.GetAt(nExperiment);
}

void KWStatisticalEvaluation::InitializeAllLabels()
{
	svExperimentLabels.SetSize(0);
	svExperimentLabels.SetSize(nExperimentNumber);
}

double KWStatisticalEvaluation::GetMeanAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(GetRunNumber() > 0);

	return KWStat::Mean(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment)));
}

double KWStatisticalEvaluation::GetGeometricMeanAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(GetRunNumber() > 0);

	if (KWStat::Min(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment))) >= 0)
		return 0;
	else
		return KWStat::GeometricMean(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment)));
}

double KWStatisticalEvaluation::GetStandardDeviationAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(GetRunNumber() > 0);

	return KWStat::StandardDeviation(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment)));
}

double KWStatisticalEvaluation::GetMinAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(GetRunNumber() > 0);

	return KWStat::Min(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment)));
}

double KWStatisticalEvaluation::GetMaxAt(int nExperiment) const
{
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(GetRunNumber() > 0);

	return KWStat::Max(cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment)));
}

double KWStatisticalEvaluation::GetMean() const
{
	DoubleVector dvAllValues;

	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 0);

	CopyAllResults(&dvAllValues);
	return KWStat::Mean(&dvAllValues);
}

double KWStatisticalEvaluation::GetGeometricMean() const
{
	DoubleVector dvAllValues;

	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 0);

	CopyAllResults(&dvAllValues);
	if (KWStat::Min(&dvAllValues) <= 0)
		return 0;
	else
		return KWStat::GeometricMean(&dvAllValues);
}

double KWStatisticalEvaluation::GetStandardDeviation() const
{
	DoubleVector dvAllValues;

	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 0);

	CopyAllResults(&dvAllValues);
	return KWStat::StandardDeviation(&dvAllValues);
}

double KWStatisticalEvaluation::GetMin() const
{
	DoubleVector dvAllValues;

	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 0);

	CopyAllResults(&dvAllValues);
	return KWStat::Min(&dvAllValues);
}

double KWStatisticalEvaluation::GetMax() const
{
	DoubleVector dvAllValues;

	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 0);

	CopyAllResults(&dvAllValues);
	return KWStat::Max(&dvAllValues);
}

int KWStatisticalEvaluation::GetSignificantDifferenceAt(const KWStatisticalEvaluation* alternativeMethodResult,
							int nExperiment) const
{
	DoubleVector* dvValues;
	DoubleVector* dvAlternativeValues;
	double dTValueThreshold;
	double dTValue;
	int nSign;

	require(alternativeMethodResult != NULL);
	require(IsComparable(alternativeMethodResult));
	require(0 <= nExperiment and nExperiment < nExperimentNumber);
	require(nRunNumber > 1);

	// Acces aux vecteurs de valeurs
	dvValues = cast(DoubleVector*, oaExperimentValueVectors.GetAt(nExperiment));
	dvAlternativeValues = cast(DoubleVector*, alternativeMethodResult->oaExperimentValueVectors.GetAt(nExperiment));

	// Calcul de la valeur test, et du seuil de significativite
	dTValue = KWStat::TValue(dvValues, dvAlternativeValues);
	dTValueThreshold = KWStat::InvStudent(dSignificanceLevel, nRunNumber - 1);

	// Resultat du test de diffrence significative, en tenant compte du sens d'optimisation
	if (bMaximization)
		nSign = 1;
	else
		nSign = -1;
	if (dTValue >= dTValueThreshold)
		return nSign;
	else if (dTValue <= -dTValueThreshold)
		return -nSign;
	else
		return 0;
}

int KWStatisticalEvaluation::GetSignificantWinNumber(const KWStatisticalEvaluation* alternativeMethodResult) const
{
	int nWinNumber;
	int nExperiment;

	require(alternativeMethodResult != NULL);
	require(IsComparable(alternativeMethodResult));
	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 1);

	// Comptage du nombre de victoires
	nWinNumber = 0;
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		if (GetSignificantDifferenceAt(alternativeMethodResult, nExperiment) == 1)
			nWinNumber++;
	}
	return nWinNumber;
}

int KWStatisticalEvaluation::GetSignificantLossNumber(const KWStatisticalEvaluation* alternativeMethodResult) const
{
	int nLossNumber;
	int nExperiment;

	require(alternativeMethodResult != NULL);
	require(IsComparable(alternativeMethodResult));
	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 1);

	// Comptage du nombre de victoires
	nLossNumber = 0;
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		if (GetSignificantDifferenceAt(alternativeMethodResult, nExperiment) == -1)
			nLossNumber++;
	}
	return nLossNumber;
}

int KWStatisticalEvaluation::GetSignificantTieNumber(const KWStatisticalEvaluation* alternativeMethodResult) const
{
	int nTieNumber;
	int nExperiment;

	require(alternativeMethodResult != NULL);
	require(IsComparable(alternativeMethodResult));
	require(GetExperimentNumber() > 0);
	require(GetRunNumber() > 1);

	// Comptage du nombre de victoires
	nTieNumber = 0;
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		if (GetSignificantDifferenceAt(alternativeMethodResult, nExperiment) == 0)
			nTieNumber++;
	}
	return nTieNumber;
}

void KWStatisticalEvaluation::SetSignificanceLevel(double dValue)
{
	require(0 <= dValue and dValue <= 1);

	dSignificanceLevel = dValue;
}

double KWStatisticalEvaluation::GetSignificanceLevel() const
{
	return dSignificanceLevel;
}

int KWStatisticalEvaluation::GetMethodRankAt(const ObjectArray* oaEvaluatedMethodResults, int nMethodIndex,
					     int nExperiment)
{
	int nMethod;
	KWStatisticalEvaluation* alternativeMethodResult;
	DoubleVector dvMethodMeans;
	double dReferenceMean;
	int i;
	int nRank;

	require(oaEvaluatedMethodResults != NULL);
	require(oaEvaluatedMethodResults->GetSize() >= 1);
	require(0 <= nMethodIndex and nMethodIndex < oaEvaluatedMethodResults->GetSize());
	require(0 <= nExperiment and
		nExperiment <
		    cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0))->GetExperimentNumber());

	// Collecte des moyennes par methode
	dvMethodMeans.SetSize(oaEvaluatedMethodResults->GetSize());
	for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
	{
		alternativeMethodResult = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
		dvMethodMeans.SetAt(nMethod, alternativeMethodResult->GetMeanAt(nExperiment));
	}

	// Recherche de la valeur de reference
	dReferenceMean = dvMethodMeans.GetAt(nMethodIndex);

	// Tri des valeurs
	dvMethodMeans.Sort();

	// Recherche du rang de la valeur de reference, en tenant compte du sens de l'optimisation
	if (cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0))->GetMaximization())
	{
		nRank = 1;
		for (i = dvMethodMeans.GetSize() - 1; i >= 0; i--)
		{
			if (dvMethodMeans.GetAt(i) == dReferenceMean)
				return nRank;
			nRank++;
		}
		assert(false);
	}
	else
	{
		nRank = 1;
		for (i = 0; i < dvMethodMeans.GetSize(); i++)
		{
			if (dvMethodMeans.GetAt(i) == dReferenceMean)
				return nRank;
			nRank++;
		}
		assert(false);
	}
	return 0;
}

double KWStatisticalEvaluation::GetMethodMeanRank(const ObjectArray* oaEvaluatedMethodResults, int nMethodIndex)
{
	DoubleVector dvRanks;
	int nExperiment;
	KWStatisticalEvaluation* firstMethodResult;

	require(oaEvaluatedMethodResults != NULL);
	require(oaEvaluatedMethodResults->GetSize() >= 1);
	require(0 <= nMethodIndex and nMethodIndex < oaEvaluatedMethodResults->GetSize());

	// Collecte des rangs de la methode
	firstMethodResult = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0));
	dvRanks.SetSize(firstMethodResult->GetExperimentNumber());
	for (nExperiment = 0; nExperiment < firstMethodResult->GetExperimentNumber(); nExperiment++)
	{
		dvRanks.SetAt(nExperiment, GetMethodRankAt(oaEvaluatedMethodResults, nMethodIndex, nExperiment));
	}

	return KWStat::Mean(&dvRanks);
}

ALString KWStatisticalEvaluation::GetStatisticalMeasuresLabel(int nStatisticalMeasures)
{
	ALString sLabel;

	require(0 <= nStatisticalMeasures and nStatisticalMeasures < 128);

	// Concatenation des libelles des mesures effectivement utilisees
	if (nStatisticalMeasures & Mean)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "Mean";
	}
	if (nStatisticalMeasures & GeometricMean)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "GeometricMean";
	}
	if (nStatisticalMeasures & StandardDeviation)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "StandardDeviation";
	}
	if (nStatisticalMeasures & Min)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "Min";
	}
	if (nStatisticalMeasures & Max)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "Max";
	}
	if (nStatisticalMeasures & SignificantTests)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "SignificantTests";
	}
	if (nStatisticalMeasures & Rank)
	{
		if (sLabel != "")
			sLabel += "\t";
		sLabel += "Rank";
	}
	return sLabel;
}

void KWStatisticalEvaluation::WriteReport(ostream& ost) const
{
	WriteSpecificReport(ost, StandardSyntheticMeasures);
}

void KWStatisticalEvaluation::WriteSpecificReport(ostream& ost, int nStatisticalMeasures) const
{
	require(0 <= nStatisticalMeasures and nStatisticalMeasures < 32);

	// Libelles en tete
	ost << GetCriterionName() << "\t";
	ost << GetMethodName() << "\n";

	// Affichage des mesures statistiques demandees
	if (nStatisticalMeasures & Mean)
		ost << GetStatisticalMeasuresLabel(Mean) << "\t" << GetMean() << "\n";
	if (nStatisticalMeasures & GeometricMean)
		ost << GetStatisticalMeasuresLabel(GeometricMean) << "\t" << GetGeometricMean() << "\n";
	if (nStatisticalMeasures & StandardDeviation)
		ost << GetStatisticalMeasuresLabel(StandardDeviation) << "\t" << GetStandardDeviation() << "\n";
	if (nStatisticalMeasures & Min)
		ost << GetStatisticalMeasuresLabel(Min) << "\t" << GetMin() << "\n";
	if (nStatisticalMeasures & Max)
		ost << GetStatisticalMeasuresLabel(Max) << "\t" << GetMax() << "\n";
}

void KWStatisticalEvaluation::WriteComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults)
{
	WriteComparativeSpecificReport(ost, oaEvaluatedMethodResults, StandardSyntheticComparativeMeasures);
}

void KWStatisticalEvaluation::WriteComparativeSpecificReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults,
							     int nStatisticalMeasures)
{
	KWStatisticalEvaluation* firstEvaluation;
	KWStatisticalEvaluation* evaluation;
	int nMethod;

	require(oaEvaluatedMethodResults != NULL);
	require(0 <= nStatisticalMeasures and nStatisticalMeasures < 128);

	// On ne fait rien si aucune evaluation n'est disponible
	if (oaEvaluatedMethodResults->GetSize() == 0)
		return;

	// Acces a la premiere evaluation
	firstEvaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0));

	// Titre pour le critere
	ost << firstEvaluation->GetCriterionName();

	// Libelles en tete pour la premiere ligne
	for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
	{
		evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));

		// Nom de la methode
		ost << "\t" << evaluation->GetMethodName();
	}
	ost << "\n";

	// Affichage par methode pour la moyenne
	if (nStatisticalMeasures & Mean)
	{
		ost << GetStatisticalMeasuresLabel(Mean);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
			ost << "\t" << evaluation->GetMean();
		}
		ost << "\n";
	}

	// Affichage par methode pour la moyenne geometrique
	if (nStatisticalMeasures & GeometricMean)
	{
		ost << GetStatisticalMeasuresLabel(GeometricMean);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
			ost << "\t" << evaluation->GetGeometricMean();
		}
		ost << "\n";
	}

	// Affichage par methode pour l'ecart type
	if (nStatisticalMeasures & StandardDeviation)
	{
		ost << GetStatisticalMeasuresLabel(StandardDeviation);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
			ost << "\t" << evaluation->GetStandardDeviation();
		}
		ost << "\n";
	}

	// Affichage par methode pour le min
	if (nStatisticalMeasures & Min)
	{
		ost << GetStatisticalMeasuresLabel(Min);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
			ost << "\t" << evaluation->GetMin();
		}
		ost << "\n";
	}

	// Affichage par methode pour le max
	if (nStatisticalMeasures & Max)
	{
		ost << GetStatisticalMeasuresLabel(Max);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
			ost << "\t" << evaluation->GetMax();
		}
		ost << "\n";
	}

	// Affichage par methode pour les test de differences significative
	if (nStatisticalMeasures & SignificantTests and firstEvaluation->GetRunNumber() > 1)
	{
		ost << GetStatisticalMeasuresLabel(SignificantTests);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));

			// On met un blanc apres le TAB pour empercher Excel d'interpreter
			// la valeur comme une date
			ost << "\t " << firstEvaluation->GetSignificantWinNumber(evaluation) << "/"
			    << firstEvaluation->GetSignificantTieNumber(evaluation) << "/"
			    << firstEvaluation->GetSignificantLossNumber(evaluation);
		}
		ost << "\n";
	}

	// Affichage par methode pour le rang
	if (nStatisticalMeasures & Rank)
	{
		ost << GetStatisticalMeasuresLabel(Rank);
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			ost << "\t" << GetMethodMeanRank(oaEvaluatedMethodResults, nMethod);
		}
		ost << "\n";
	}
}

void KWStatisticalEvaluation::WriteExperimentReport(ostream& ost) const
{
	WriteExperimentSpecificReport(ost, StandardMeasures);
}

void KWStatisticalEvaluation::WriteExperimentSpecificReport(ostream& ost, int nStatisticalMeasures) const
{
	int nExperiment;

	require(0 <= nStatisticalMeasures and nStatisticalMeasures < 32);

	// Libelles en tete
	ost << GetCriterionName() << "\n";
	ost << GetExperimentName() << "\t";
	ost << GetMethodName() << "\n";

	// Recherche de la place (en TAB) prise par le libelles des experimentations
	cout << GetEmptyLabel(GetExperimentName()) << "\t";

	// Libelles des mesures statistiques
	ost << GetStatisticalMeasuresLabel(nStatisticalMeasures);
	ost << "\n";

	// Affichage des resultats par experience
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		// Libelle de l'experience
		ost << GetExperimentLabelAt(nExperiment);

		// Affichage des mesures statistiques demandees
		if (nStatisticalMeasures & Mean)
			ost << "\t" << GetMeanAt(nExperiment);
		if (nStatisticalMeasures & GeometricMean)
			ost << "\t" << GetGeometricMeanAt(nExperiment);
		if (nStatisticalMeasures & StandardDeviation)
			ost << "\t" << GetStandardDeviationAt(nExperiment);
		if (nStatisticalMeasures & Min)
			ost << "\t" << GetMinAt(nExperiment);
		if (nStatisticalMeasures & Max)
			ost << "\t" << GetMaxAt(nExperiment);
		ost << "\n";
	}
}

void KWStatisticalEvaluation::WriteExperimentComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults)
{
	WriteExperimentComparativeSpecificReport(ost, oaEvaluatedMethodResults, StandardComparativeMeasures);
}

void KWStatisticalEvaluation::WriteExperimentComparativeSpecificReport(ostream& ost,
								       ObjectArray* oaEvaluatedMethodResults,
								       int nStatisticalMeasures)
{
	KWStatisticalEvaluation* firstEvaluation;
	KWStatisticalEvaluation* evaluation;
	int nMethod;
	int nExperiment;

	require(oaEvaluatedMethodResults != NULL);
	require(0 <= nStatisticalMeasures and nStatisticalMeasures < 128);

	// On ne fait rien si aucune evaluation n'est disponible
	if (oaEvaluatedMethodResults->GetSize() == 0)
		return;

	// Acces a la premiere evaluation
	firstEvaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0));

	// Titre pour le critere
	ost << firstEvaluation->GetCriterionName() << "\n";

	// Libelles en tete pour la premiere ligne
	ost << firstEvaluation->GetExperimentName();
	for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
	{
		evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));

		// Nom de la methode
		ost << "\t" << evaluation->GetMethodName();

		// Place reservee (en TAB) pour les libelles des mesures statistiques
		ost << GetEmptyLabel(GetStatisticalMeasuresLabel(nStatisticalMeasures));
	}
	ost << "\n";

	// Libelles en tete pour la seconde ligne (pour les mesures statistiques)
	ost << GetEmptyLabel(firstEvaluation->GetExperimentName());
	for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
	{
		ost << "\t" << GetStatisticalMeasuresLabel(nStatisticalMeasures);
	}
	ost << "\n";

	// Affichage des valeurs
	for (nExperiment = 0; nExperiment < firstEvaluation->GetExperimentNumber(); nExperiment++)
	{
		// Libelle de l'experience
		ost << firstEvaluation->GetExperimentLabelAt(nExperiment);

		// Resultat par methode
		for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
		{
			evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));

			// Affichage des mesures statistiques demandees
			if (nStatisticalMeasures & Mean)
				ost << "\t" << evaluation->GetMeanAt(nExperiment);
			if (nStatisticalMeasures & GeometricMean)
				ost << "\t" << evaluation->GetGeometricMeanAt(nExperiment);
			if (nStatisticalMeasures & StandardDeviation)
				ost << "\t" << evaluation->GetStandardDeviationAt(nExperiment);
			if (nStatisticalMeasures & Min)
				ost << "\t" << evaluation->GetMinAt(nExperiment);
			if (nStatisticalMeasures & Max)
				ost << "\t" << evaluation->GetMaxAt(nExperiment);
			if (nStatisticalMeasures & SignificantTests and firstEvaluation->GetRunNumber() > 1)
				ost << "\t" << firstEvaluation->GetSignificantDifferenceAt(evaluation, nExperiment);
			if (nStatisticalMeasures & Rank)
				ost << "\t"
				    << evaluation->GetMethodRankAt(oaEvaluatedMethodResults, nMethod, nExperiment);
		}
		ost << "\n";
	}
}

void KWStatisticalEvaluation::WriteValueReport(ostream& ost) const
{
	int nExperiment;
	int nRun;

	// Libelles en tete
	ost << GetCriterionName() << "\n";
	ost << GetExperimentName() << "\t"
	    << "Run"
	    << "\t";
	ost << GetMethodName() << "\n";

	// Affichage des valeurs
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
			ost << GetExperimentLabelAt(nExperiment) << "\t";
			ost << nRun + 1 << "\t";
			ost << GetResultAt(nExperiment, nRun) << "\n";
		}
	}
}

void KWStatisticalEvaluation::WriteValueComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults)
{
	KWStatisticalEvaluation* firstEvaluation;
	KWStatisticalEvaluation* evaluation;
	int nMethod;
	int nExperiment;
	int nRun;

	require(oaEvaluatedMethodResults != NULL);

	// On ne fait rien si aucune evaluation n'est disponible
	if (oaEvaluatedMethodResults->GetSize() == 0)
		return;

	// Acces a la premiere evaluation
	firstEvaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(0));

	// Libelles en tete
	ost << firstEvaluation->GetCriterionName() << "\n";
	ost << firstEvaluation->GetExperimentName() << "\t"
	    << "Run";
	for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
	{
		evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
		ost << "\t" << evaluation->GetMethodName();
	}
	ost << "\n";

	// Affichage des valeurs
	for (nExperiment = 0; nExperiment < firstEvaluation->GetExperimentNumber(); nExperiment++)
	{
		for (nRun = 0; nRun < firstEvaluation->GetRunNumber(); nRun++)
		{
			ost << firstEvaluation->GetExperimentLabelAt(nExperiment) << "\t";
			ost << nRun + 1;

			// Resultat par methode
			for (nMethod = 0; nMethod < oaEvaluatedMethodResults->GetSize(); nMethod++)
			{
				evaluation = cast(KWStatisticalEvaluation*, oaEvaluatedMethodResults->GetAt(nMethod));
				ost << "\t" << evaluation->GetResultAt(nExperiment, nRun);
			}
			ost << "\n";
		}
	}
}

KWStatisticalEvaluation* KWStatisticalEvaluation::Clone() const
{
	KWStatisticalEvaluation* kwseClone;

	kwseClone = new KWStatisticalEvaluation;
	kwseClone->CopyFrom(this);
	return kwseClone;
}

void KWStatisticalEvaluation::CopyFrom(const KWStatisticalEvaluation* kwseSource)
{
	int nExperiment;
	int nRun;

	require(kwseSource != NULL);

	// Recopies des parametres de base
	SetCriterionName(kwseSource->GetCriterionName());
	SetMethodName(kwseSource->GetMethodName());
	SetExperimentName(kwseSource->GetExperimentName());
	SetMaximization(kwseSource->GetMaximization());
	SetExperimentNumber(kwseSource->GetExperimentNumber());
	SetRunNumber(kwseSource->GetRunNumber());
	SetSignificanceLevel(kwseSource->GetSignificanceLevel());

	// Recopie des valeurs
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		for (nRun = 0; nRun < nRunNumber; nRun++)
			SetResultAt(nExperiment, nRun, kwseSource->GetResultAt(nExperiment, nRun));
	}

	// Recopie des libelles
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
		SetExperimentLabelAt(nExperiment, kwseSource->GetExperimentLabelAt(nExperiment));
}

boolean KWStatisticalEvaluation::IsComparable(const KWStatisticalEvaluation* kwseSource) const
{
	boolean bOk = true;

	require(kwseSource != NULL);

	// Comparaison des parametres devant etre semblables pour permettre une comparaison
	// (on ne verifie pas la compatibilite des libelles d'experience)
	if (kwseSource->GetCriterionName() != GetCriterionName())
		bOk = false;
	if (kwseSource->GetExperimentName() != GetExperimentName())
		bOk = false;
	if (kwseSource->GetMaximization() != GetMaximization())
		bOk = false;
	if (kwseSource->GetExperimentNumber() != GetExperimentNumber())
		bOk = false;
	if (kwseSource->GetRunNumber() != GetRunNumber())
		bOk = false;
	if (kwseSource->GetSignificanceLevel() != GetSignificanceLevel())
		bOk = false;

	return bOk;
}

const ALString KWStatisticalEvaluation::GetClassLabel() const
{
	return "Evaluation";
}

const ALString KWStatisticalEvaluation::GetObjectLabel() const
{
	return GetCriterionName() + "(" + GetMethodName() + ")";
}

void KWStatisticalEvaluation::CopyAllResults(DoubleVector* dvAllValues) const
{
	int nExperiment;
	int nRun;

	require(dvAllValues != NULL);

	// Recopie de toutes les valeurs dans un vecteur a plat
	dvAllValues->SetSize(nExperimentNumber * nRunNumber);
	for (nExperiment = 0; nExperiment < nExperimentNumber; nExperiment++)
	{
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
			dvAllValues->SetAt(nExperiment * nRunNumber + nRun, GetResultAt(nExperiment, nRun));
		}
	}
}

ALString KWStatisticalEvaluation::GetEmptyLabel(const ALString& sLabel)
{
	ALString sEmptyLabel;
	int i;

	// Recherche des TAB du libelle
	for (i = 0; i < sLabel.GetLength(); i++)
	{
		if (sLabel.GetAt(i) == '\t')
			sEmptyLabel += '\t';
	}
	return sEmptyLabel;
}

void KWStatisticalEvaluation::Test()
{
	KWStatisticalEvaluation kwseTest1;
	KWStatisticalEvaluation kwseTest2;
	KWStatisticalEvaluation kwseTest3;
	ObjectArray oaAllEvaluations;
	int nExperiment;
	int nRun;
	ALString sTmp;

	// Initialisation de l'evaluation pour la methode de base
	kwseTest1.SetCriterionName("Criterion1");
	kwseTest1.SetMethodName("Method1");
	kwseTest1.SetExperimentName("Experience");
	kwseTest1.SetExperimentNumber(10);
	kwseTest1.SetRunNumber(10);

	// Initialisation de l'evaluation pour les methodes alternatives
	kwseTest2.CopyFrom(&kwseTest1);
	kwseTest2.SetMethodName("Method2");
	kwseTest3.CopyFrom(&kwseTest1);
	kwseTest3.SetMethodName("Method3");

	// Rangement des evaluation dans un tableau
	oaAllEvaluations.Add(&kwseTest1);
	oaAllEvaluations.Add(&kwseTest2);
	oaAllEvaluations.Add(&kwseTest3);

	// Initialisation des libelles des experiences
	for (nExperiment = 0; nExperiment < kwseTest1.GetExperimentNumber(); nExperiment++)
		kwseTest1.SetExperimentLabelAt(nExperiment, sTmp + "Exp" + IntToString(nExperiment + 1));

	// Initialisation des valeurs
	for (nExperiment = 0; nExperiment < kwseTest1.GetExperimentNumber(); nExperiment++)
	{
		for (nRun = 0; nRun < kwseTest1.GetRunNumber(); nRun++)
		{
			kwseTest1.SetResultAt(nExperiment, nRun, nExperiment + RandomDouble());
			kwseTest2.SetResultAt(nExperiment, nRun, nExperiment + RandomDouble());
			kwseTest3.SetResultAt(nExperiment, nRun, nExperiment + sqrt(RandomDouble()));
		}
	}

	// Affichage des resultats comparatifs pour l'ensemble des evaluations
	cout << "Resultats comparatifs synthetiques\n";
	WriteComparativeReport(cout, &oaAllEvaluations);
	cout << endl;
	cout << "Resultats comparatifs par experience\n";
	WriteExperimentComparativeReport(cout, &oaAllEvaluations);
	cout << endl;
	cout << "Resultats comparatifs detailles\n";
	WriteValueComparativeReport(cout, &oaAllEvaluations);
	cout << endl;

	// Affichage des resultats pour la premiere evaluation
	cout << "Resultats synthetiques\n";
	kwseTest1.WriteReport(cout);
	cout << endl;
	cout << "Resultats par experience\n";
	kwseTest1.WriteExperimentReport(cout);
	cout << endl;
	cout << "Resultats detailles\n";
	kwseTest1.WriteValueReport(cout);
	cout << endl;
}
