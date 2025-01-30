// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MHHistogramSpec.h"

MHHistogramSpec::MHHistogramSpec()
{
	nDeltaCentralBinExponent = 0;
	nMaxHierarchyLevel = 0;
	nMaxIntervalNumber = 0;
	nMaxCoarsenedHistogramNumber = 0;
	bTruncationManagementHeuristic = false;
	bSingularityRemovalHeuristic = false;
	bOutlierManagementHeuristic = false;
	dEpsilonBinWidth = 0;
	nEpsilonBinNumber = 0;
	bOptimalAlgorithm = false;
	bExportInternalLogFiles = false;
	nFileFormat = 0;

	// ## Custom constructor

	// Valeurs par defaut
	sHistogramCriterion = "G-Enum-fp";
	nDeltaCentralBinExponent = -1;
	nEpsilonBinNumber = GetMaxEpsilonBinNumber();
	bTruncationManagementHeuristic = true;
	bSingularityRemovalHeuristic = true;
	bOutlierManagementHeuristic = true;
	nFileFormat = 1;

	// Valeurs par defaut des indicateurs de gestion des logs
	bTruncationMode = false;
	bDeltaValues = false;
	bLogTrValues = false;
	nOutlierSplitIndex = -1;
	bOutlierBoundary = false;

	// ##
}

MHHistogramSpec::~MHHistogramSpec()
{
	// ## Custom destructor

	// ##
}

void MHHistogramSpec::CopyFrom(const MHHistogramSpec* aSource)
{
	require(aSource != NULL);

	sHistogramCriterion = aSource->sHistogramCriterion;
	nDeltaCentralBinExponent = aSource->nDeltaCentralBinExponent;
	nMaxHierarchyLevel = aSource->nMaxHierarchyLevel;
	nMaxIntervalNumber = aSource->nMaxIntervalNumber;
	nMaxCoarsenedHistogramNumber = aSource->nMaxCoarsenedHistogramNumber;
	bTruncationManagementHeuristic = aSource->bTruncationManagementHeuristic;
	bSingularityRemovalHeuristic = aSource->bSingularityRemovalHeuristic;
	bOutlierManagementHeuristic = aSource->bOutlierManagementHeuristic;
	dEpsilonBinWidth = aSource->dEpsilonBinWidth;
	nEpsilonBinNumber = aSource->nEpsilonBinNumber;
	bOptimalAlgorithm = aSource->bOptimalAlgorithm;
	bExportInternalLogFiles = aSource->bExportInternalLogFiles;
	nFileFormat = aSource->nFileFormat;
	sResultFilesDirectory = aSource->sResultFilesDirectory;
	sResultFilesPrefix = aSource->sResultFilesPrefix;

	// ## Custom copyfrom

	bTruncationMode = aSource->bTruncationMode;
	bDeltaValues = aSource->bDeltaValues;
	bLogTrValues = aSource->bLogTrValues;
	nOutlierSplitIndex = aSource->nOutlierSplitIndex;
	bOutlierBoundary = aSource->bOutlierBoundary;

	// ##
}

MHHistogramSpec* MHHistogramSpec::Clone() const
{
	MHHistogramSpec* aClone;

	aClone = new MHHistogramSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void MHHistogramSpec::Write(ostream& ost) const
{
	ost << "Histogram criterion\t" << GetHistogramCriterion() << "\n";
	ost << "Delta central bin exponent\t" << GetDeltaCentralBinExponent() << "\n";
	ost << "Max hierarchy level\t" << GetMaxHierarchyLevel() << "\n";
	ost << "Max interval number\t" << GetMaxIntervalNumber() << "\n";
	ost << "Max coarsened histogram number\t" << GetMaxCoarsenedHistogramNumber() << "\n";
	ost << "Truncation management heuristic\t" << BooleanToString(GetTruncationManagementHeuristic()) << "\n";
	ost << "Singularity removal heuristic\t" << BooleanToString(GetSingularityRemovalHeuristic()) << "\n";
	ost << "Outlier management heuristic\t" << BooleanToString(GetOutlierManagementHeuristic()) << "\n";
	ost << "Epsilon bin width\t" << GetEpsilonBinWidth() << "\n";
	ost << "Epsilon bin number\t" << GetEpsilonBinNumber() << "\n";
	ost << "Optimal algorithm\t" << BooleanToString(GetOptimalAlgorithm()) << "\n";
	ost << "Export internal log files\t" << BooleanToString(GetExportInternalLogFiles()) << "\n";
	ost << "File format\t" << GetFileFormat() << "\n";
	ost << "Result files directory\t" << GetResultFilesDirectory() << "\n";
	ost << "Result files prefix\t" << GetResultFilesPrefix() << "\n";
}

const ALString MHHistogramSpec::GetClassLabel() const
{
	return "Histogram parameters";
}

// ## Method implementation

const ALString MHHistogramSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

const ALString MHHistogramSpec::GetVersion()
{
	return "7.1";
}

boolean MHHistogramSpec::GetGranularizedModel() const
{
	return sHistogramCriterion == "G-Enum" or sHistogramCriterion == "G-Enum-fp";
}

boolean MHHistogramSpec::Check() const
{
	boolean bOk = true;

	// Verification de base, en principe forcement correctes
	bOk = bOk and (GetHistogramCriterion() == "G-Enum-fp" or GetHistogramCriterion() == "G-Enum" or
		       GetHistogramCriterion() == "Enum" or GetHistogramCriterion() == "KM");
	bOk = bOk and nMaxHierarchyLevel >= 0;
	bOk = bOk and nMaxIntervalNumber >= 0;
	bOk = bOk and dEpsilonBinWidth >= 0;
	bOk = bOk and nEpsilonBinNumber >= 0;
	assert(bOk);

	// Verification dans le cas de l'algorithme KM
	if (GetHistogramCriterion() == "Enum" or GetHistogramCriterion() == "KM")
	{
		// Verifications de base
		bOk = bOk and nMaxHierarchyLevel == 0;
		bOk = bOk and nMaxIntervalNumber == 0;
		bOk = bOk and nEpsilonBinNumber == 0;
		bOk = bOk and not bTruncationManagementHeuristic;
		bOk = bOk and not bSingularityRemovalHeuristic;
		bOk = bOk and not bOutlierManagementHeuristic;
		assert(bOk);

		// Verification du parametre de largeur de bin
		if (dEpsilonBinWidth == 0)
		{
			AddError("Epsilon bin width parameter must be strictly positive for criterion " +
				 GetHistogramCriterion());
			bOk = false;
		}
	}
	return bOk;
}

void MHHistogramSpec::FitSpecificationToCriterion()
{
	if (GetHistogramCriterion() == "Enum" or GetHistogramCriterion() == "KM")
	{
		nMaxHierarchyLevel = 0;
		nMaxIntervalNumber = 0;
		nEpsilonBinNumber = 0;
		bTruncationManagementHeuristic = false;
		bSingularityRemovalHeuristic = false;
		bOutlierManagementHeuristic = false;
	}
	else if (GetHistogramCriterion() == "G-Enum-fp" or GetHistogramCriterion() == "G-Enum")
	{
		if (not bTruncationManagementHeuristic)
			dEpsilonBinWidth = 0;
	}
}

int MHHistogramSpec::ComputeMaxPartileNumber(int nTotalFrequency, Continuous cMinValue, Continuous cMaxValue) const
{
	int nTotalBinNumber;
	int nMaxPartileNumber;
	int nMinimumDistinctValueNumberPerBin;
	double dExpectedNumberDistinctValues;

	require(nTotalFrequency >= 0);
	require(cMinValue <= cMaxValue);

	// Par defaut, on prend la precision maximale
	nTotalBinNumber = GetEpsilonBinNumber();
	nMaxPartileNumber = nTotalBinNumber;

	// Calcul du nombre de valeurs distinctes encodable sur l'intervalles de valeurs
	dExpectedNumberDistinctValues = MHContinuousLimits::ComputeNumberDistinctValues(cMinValue, cMaxValue);

	// Estimation du nombre minimal de valeurs distinctes par bin que l'on veut obtenir
	// On prend racine(n log(n)) pour que ce nombre grandisse avec n, avec plus de fiabilite
	// Un essai avec racine(n) montre que l'on est trop a la limite et que cela est parfois insuffisant
	// Sinon, pour tout seuil fixe et n assez grand, tout difference de densite meme legere devient significative
	nMinimumDistinctValueNumberPerBin = int(ceil(sqrt(1.0 + nTotalFrequency * log(1.0 + nTotalFrequency))));

	// Correction si l'on atteint pas le nombre minimal de valeur distincts par bin
	if (dExpectedNumberDistinctValues / nMaxPartileNumber < nMinimumDistinctValueNumberPerBin)
	{
		// Dans ce cas, on limite le nombre max de bin de telle facon a avoir suffisament de possibilites
		// de valeurs numerique distinctes
		nMaxPartileNumber = int(floor(dExpectedNumberDistinctValues / nMinimumDistinctValueNumberPerBin));
		nMaxPartileNumber = min(nMaxPartileNumber, nTotalBinNumber);
		nMaxPartileNumber = max(nMaxPartileNumber, 1);
	}
	ensure(1 <= nMaxPartileNumber and nMaxPartileNumber <= nTotalBinNumber);
	return nMaxPartileNumber;
}

Continuous MHHistogramSpec::ComputeEpsilonBinLength(int nTotalBinNumber, Continuous cMinValue,
						    Continuous cMaxValue) const
{
	Continuous cEpsilonBinLength;

	require(nTotalBinNumber >= 1);
	require(cMinValue <= cMaxValue);

	// On se base sur le calcul (potentiellement corrige) des bornes inf et sup
	cEpsilonBinLength = (ComputeHistogramUpperBound(nTotalBinNumber, cMinValue, cMaxValue) -
			     ComputeHistogramLowerBound(nTotalBinNumber, cMinValue, cMaxValue)) /
			    nTotalBinNumber;
	ensure(cEpsilonBinLength > 0);
	return cEpsilonBinLength;
}

Continuous MHHistogramSpec::ComputeHistogramLowerBound(int nTotalBinNumber, Continuous cMinValue,
						       Continuous cMaxValue) const
{
	Continuous cEpsilonBinLength;
	Continuous cLowerBound;

	require(nTotalBinNumber >= 1);
	require(cMinValue <= cMaxValue);

	// Calcul du epsilon selon la methode interne
	cEpsilonBinLength = InternalComputeEpsilonBinLength(nTotalBinNumber, cMinValue, cMaxValue);

	// Cas il y a au moins de deux bins
	if (nTotalBinNumber >= 2)
		cLowerBound = cMinValue - cEpsilonBinLength / 2;
	// Cas d'un seul bin
	else
		cLowerBound = (cMinValue + cMaxValue) / 2 - cEpsilonBinLength / 2;
	cLowerBound = KWContinuous::DoubleToContinuous(cLowerBound);
	if (cLowerBound == cMinValue)
		cLowerBound = MHContinuousLimits::ComputeClosestLowerBound(cMinValue);
	ensure(cLowerBound <= cMinValue);
	return cLowerBound;
}

Continuous MHHistogramSpec::ComputeHistogramUpperBound(int nTotalBinNumber, Continuous cMinValue,
						       Continuous cMaxValue) const
{
	Continuous cEpsilonBinLength;
	Continuous cUpperBound;

	require(nTotalBinNumber >= 1);
	require(cMinValue <= cMaxValue);

	// Calcul du epsilon selon la methode interne
	cEpsilonBinLength = InternalComputeEpsilonBinLength(nTotalBinNumber, cMinValue, cMaxValue);

	// Cas il y a au moins de deux bins
	if (nTotalBinNumber >= 2)
		cUpperBound = cMaxValue + cEpsilonBinLength / 2;
	// Cas d'un seul bin
	else
		cUpperBound = (cMinValue + cMaxValue) / 2 + cEpsilonBinLength / 2;
	cUpperBound = KWContinuous::DoubleToContinuous(cUpperBound);
	if (cUpperBound == cMaxValue)
		cUpperBound = MHContinuousLimits::ComputeClosestUpperBound(cMaxValue);
	ensure(cUpperBound >= cMaxValue);
	return cUpperBound;
}

int MHHistogramSpec::GetOutlierEpsilonBinNumber() const
{
	int nOutlierEpsilonBinNumber;
	nOutlierEpsilonBinNumber = int(ceil(sqrt(GetEpsilonBinNumber() * 1.0) * log(GetEpsilonBinNumber() * 1.0)));
	return nOutlierEpsilonBinNumber;
}

int MHHistogramSpec::GetOutlierMaxBinFrequency(int nDatasetFrequency) const
{
	int nOutlierMaxBinFrequency;

	require(nDatasetFrequency > 0);

	nOutlierMaxBinFrequency = int(ceil(log(nDatasetFrequency * 1.0)));
	return nOutlierMaxBinFrequency;
}

int MHHistogramSpec::GetMaxEpsilonBinNumber()
{
	return 1000000000;
}

const ALString MHHistogramSpec::GetHistogramFileName(const ALString& sSuffix) const
{
	ALString sLogFileName;
	sLogFileName = FileService::BuildFilePathName(GetResultFilesDirectory(),
						      GetResultFilesPrefix() + "histogram" + sSuffix + ".log");
	return sLogFileName;
}

const ALString MHHistogramSpec::GetInternalTraceFileName() const
{
	ALString sLogFileName;
	sLogFileName =
	    FileService::BuildFilePathName(GetResultFilesDirectory(), GetResultFilesPrefix() + ".MODLH.trace.log");
	return sLogFileName;
}

const ALString MHHistogramSpec::GetInternalHistogramFileName() const
{
	ALString sLogFileName;
	sLogFileName = GetInternalLogBaseFileName() + ".histogram.log";
	return sLogFileName;
}

const ALString MHHistogramSpec::GetInternalOptimizationFileName(const ALString& sOptimizationPrefix) const
{
	ALString sLogFileName;
	sLogFileName = GetInternalLogBaseFileName() + sOptimizationPrefix;
	if (GetTruncationMode())
		sLogFileName += ".TMH";
	if (GetDeltaValues())
		sLogFileName += ".delta";
	sLogFileName += ".optimization.log";
	return sLogFileName;
}

const ALString MHHistogramSpec::GetInternalOutlierLogTrDataFileName() const
{
	ALString sLogFileName;
	sLogFileName = GetInternalLogBaseFileName() + ".data.txt";
	return sLogFileName;
}

const ALString MHHistogramSpec::GetInternalLogBaseFileName() const
{
	ALString sInternalLogBaseFileName;

	sInternalLogBaseFileName = FileService::BuildFilePathName(GetResultFilesDirectory(), GetResultFilesPrefix());
	sInternalLogBaseFileName += ".MODLH";
	if (GetLogTrValues())
		sInternalLogBaseFileName += ".OMH.logTr";
	else if (GetOutlierSplitIndex() != -1)
	{
		sInternalLogBaseFileName += ".OMH.split";
		if (GetOutlierBoundary())
		{
			sInternalLogBaseFileName += IntToString(GetOutlierSplitIndex() - 1);
			sInternalLogBaseFileName += "_";
		}
		sInternalLogBaseFileName += IntToString(GetOutlierSplitIndex());
	}
	if (GetTruncationMode())
		sInternalLogBaseFileName += ".TMH";
	if (GetDeltaValues())
		sInternalLogBaseFileName += ".delta";
	if (GetGranularizedModel())
		sInternalLogBaseFileName += ".gr";
	return sInternalLogBaseFileName;
}

void MHHistogramSpec::SetTruncationMode(boolean bValue)
{
	bTruncationMode = bValue;
}
boolean MHHistogramSpec::GetTruncationMode() const
{
	return bTruncationMode;
}

void MHHistogramSpec::SetDeltaValues(boolean bValue)
{
	bDeltaValues = bValue;
}
boolean MHHistogramSpec::GetDeltaValues() const
{
	return bDeltaValues;
}

void MHHistogramSpec::SetLogTrValues(boolean bValue)
{
	bLogTrValues = bValue;
}
boolean MHHistogramSpec::GetLogTrValues() const
{
	return bLogTrValues;
}

void MHHistogramSpec::SetOutlierSplitIndex(int nValue)
{
	require(nValue >= -1);
	nOutlierSplitIndex = nValue;
}

int MHHistogramSpec::GetOutlierSplitIndex() const
{
	return nOutlierSplitIndex;
}

void MHHistogramSpec::SetOutlierBoundary(boolean bValue)
{
	bOutlierBoundary = bValue;
}

int MHHistogramSpec::GetOutlierBoundary() const
{
	return bOutlierBoundary;
}

void MHHistogramSpec::Test()
{
	MHHistogramSpec histogramSpecTest;
	ContinuousVector cvLowerBounds;
	ContinuousVector cvUpperBounds;
	Continuous cMin;
	Continuous cMax;
	int i;

	// Intervalles autour de 0
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(-KWContinuous::GetEpsilonValue() * (1 + pow(10, -i)));
		cvUpperBounds.Add(KWContinuous::GetEpsilonValue() * (1 + pow(10, -i)));
	}

	// Intervalles autour de 1
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(1 - pow(10, -i));
		cvUpperBounds.Add(1);
	}
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(1);
		cvUpperBounds.Add(1 + pow(10, -i));
	}
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(1);
		cvUpperBounds.Add(1 + pow(10, i));
	}

	// Intervalles autour des bornes max
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(-KWContinuous::GetMaxValue());
		cvUpperBounds.Add(-KWContinuous::GetMaxValue() * (1 - pow(10, -i)));
	}
	for (i = 0; i < 12; i++)
	{
		cvLowerBounds.Add(KWContinuous::GetMaxValue() * (1 - pow(10, -i)));
		cvUpperBounds.Add(KWContinuous::GetMaxValue());
	}

	// Tests
	cout << "min\tmax\tn_d(min, max)\tE\n";
	for (i = 0; i < cvLowerBounds.GetSize(); i++)
	{
		cMin = KWContinuous::DoubleToContinuous(cvLowerBounds.GetAt(i));
		cMax = KWContinuous::DoubleToContinuous(cvUpperBounds.GetAt(i));
		cout << KWContinuous::ContinuousToString(cMin) << "\t" << KWContinuous::ContinuousToString(cMax) << "\t"
		     << MHContinuousLimits::ComputeNumberDistinctValues(cMin, cMax) << "\t"
		     << histogramSpecTest.ComputeMaxPartileNumber(100, cMin, cMax) << "\t"
		     << histogramSpecTest.ComputeMaxPartileNumber(10000, cMin, cMax) << "\t"
		     << histogramSpecTest.ComputeMaxPartileNumber(100000, cMin, cMax) << "\n";
	}
}

Continuous MHHistogramSpec::InternalComputeEpsilonBinLength(int nTotalBinNumber, Continuous cMinValue,
							    Continuous cMaxValue) const
{
	Continuous cEpsilonBinLength;

	require(nTotalBinNumber >= 1);
	require(cMinValue <= cMaxValue);

	// Cas il y a au moins de deux bins avec deux valeurs distinctes
	if (nTotalBinNumber >= 2 and cMinValue < cMaxValue)
	{
		cEpsilonBinLength = (cMaxValue - cMinValue) / (nTotalBinNumber - 1);
	}
	// Cas particulier d'un seul bin: on prend la valeur de 2 epsilon
	else
	{
		// Cas de deux valeurs identiques
		if (cMinValue == cMaxValue)
		{
			// En 0, on se base sur la valeur par defaut
			if (cMinValue == 0)
				cEpsilonBinLength = 2.0 / GetEpsilonBinNumber();
			else
				cEpsilonBinLength = 2 * fabs(cMinValue) / GetEpsilonBinNumber();
		}
		// Cas deux valeurs distinctes
		// Il peut en effet etre necessaire d'avoir un seul bin si les valeurs sont differentes,
		// mais si proches qu'elles sont quasiment indiscernables
		else
		{
			cEpsilonBinLength =
			    cMaxValue - cMinValue + (fabs(cMinValue) + fabs(cMaxValue)) / GetEpsilonBinNumber();
		}
	}
	ensure(cEpsilonBinLength > 0);
	return cEpsilonBinLength;
}

// ##
