// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MHHistogramSpec.h"

MHHistogramSpec::MHHistogramSpec()
{
	nDeltaCentralBinExponent = 0;
	bTruncationManagementHeuristic = false;
	bSingularityRemovalHeuristic = false;
	dEpsilonBinWidth = 0;
	bOptimalAlgorithm = false;
	bExportInternalLogFiles = false;
	nFileFormat = 0;

	// ## Custom constructor

	// Valeurs par defaut
	sHistogramCriterion = "G-Enum-fp";
	nDeltaCentralBinExponent = -1;
	bTruncationManagementHeuristic = true;
	bSingularityRemovalHeuristic = true;
	sExportResultHistograms = "None";
	nFileFormat = 1;

	// Valeurs par defaut des indicateurs de gestion des logs
	bTruncationMode = false;
	bDeltaValues = false;

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
	bTruncationManagementHeuristic = aSource->bTruncationManagementHeuristic;
	bSingularityRemovalHeuristic = aSource->bSingularityRemovalHeuristic;
	dEpsilonBinWidth = aSource->dEpsilonBinWidth;
	bOptimalAlgorithm = aSource->bOptimalAlgorithm;
	sExportResultHistograms = aSource->sExportResultHistograms;
	bExportInternalLogFiles = aSource->bExportInternalLogFiles;
	nFileFormat = aSource->nFileFormat;
	sResultFilesDirectory = aSource->sResultFilesDirectory;
	sResultFilesPrefix = aSource->sResultFilesPrefix;

	// ## Custom copyfrom

	bTruncationMode = aSource->bTruncationMode;
	bDeltaValues = aSource->bDeltaValues;

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
	ost << "Truncation management heuristic\t" << BooleanToString(GetTruncationManagementHeuristic()) << "\n";
	ost << "Singularity removal heuristic\t" << BooleanToString(GetSingularityRemovalHeuristic()) << "\n";
	ost << "Epsilon bin width\t" << GetEpsilonBinWidth() << "\n";
	ost << "Optimal algorithm\t" << BooleanToString(GetOptimalAlgorithm()) << "\n";
	ost << "Export result histograms\t" << GetExportResultHistograms() << "\n";
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

boolean MHHistogramSpec::Check() const
{
	boolean bOk = true;

	// Verification de base, en principe forcement correctes
	bOk = bOk and (GetHistogramCriterion() == "G-Enum-fp");
	bOk = bOk and dEpsilonBinWidth >= 0;
	assert(bOk);
	return bOk;
}

const ALString MHHistogramSpec::GetHistogramFileName(const ALString& sSuffix) const
{
	ALString sLogFileName;
	sLogFileName = FileService::BuildFilePathName(GetResultFilesDirectory(),
						      GetResultFilesPrefix() + ".histogram" + sSuffix + ".log");
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

const ALString MHHistogramSpec::GetInternalLogBaseFileName() const
{
	ALString sInternalLogBaseFileName;

	sInternalLogBaseFileName = FileService::BuildFilePathName(GetResultFilesDirectory(), GetResultFilesPrefix());
	sInternalLogBaseFileName += ".MODLH";
	if (GetTruncationMode())
		sInternalLogBaseFileName += ".TMH";
	if (GetDeltaValues())
		sInternalLogBaseFileName += ".delta";
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
		     << MHContinuousLimits::ComputeNumberDistinctValues(cMin, cMax) << "\n";
	}
}

// ##
