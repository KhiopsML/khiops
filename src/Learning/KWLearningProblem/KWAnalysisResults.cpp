// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-06 18:11:58
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWAnalysisResults.h"

KWAnalysisResults::KWAnalysisResults()
{
	// ## Custom constructor

	sPreparationFileName = "PreparationReport.xls";
	sPreparation2DFileName = "Preparation2DReport.xls";
	sModelingDictionaryFileName = "Modeling.kdic";
	sModelingFileName = "ModelingReport.xls";
	sTrainEvaluationFileName = "TrainEvaluationReport.xls";
	sTestEvaluationFileName = "TestEvaluationReport.xls";
	sJSONFileName = "AllReports.khj";

	// ##
}

KWAnalysisResults::~KWAnalysisResults()
{
	// ## Custom destructor

	// ##
}

void KWAnalysisResults::CopyFrom(const KWAnalysisResults* aSource)
{
	require(aSource != NULL);

	sResultFilesDirectory = aSource->sResultFilesDirectory;
	sResultFilesPrefix = aSource->sResultFilesPrefix;
	sShortDescription = aSource->sShortDescription;
	sPreparationFileName = aSource->sPreparationFileName;
	sPreparation2DFileName = aSource->sPreparation2DFileName;
	sModelingDictionaryFileName = aSource->sModelingDictionaryFileName;
	sModelingFileName = aSource->sModelingFileName;
	sTrainEvaluationFileName = aSource->sTrainEvaluationFileName;
	sTestEvaluationFileName = aSource->sTestEvaluationFileName;
	sVisualizationFileName = aSource->sVisualizationFileName;
	sJSONFileName = aSource->sJSONFileName;

	// ## Custom copyfrom

	// ##
}

KWAnalysisResults* KWAnalysisResults::Clone() const
{
	KWAnalysisResults* aClone;

	aClone = new KWAnalysisResults;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWAnalysisResults::Write(ostream& ost) const
{
	ost << "Result files directory\t" << GetResultFilesDirectory() << "\n";
	ost << "Result files prefix\t" << GetResultFilesPrefix() << "\n";
	ost << "Short description\t" << GetShortDescription() << "\n";
	ost << "Preparation report\t" << GetPreparationFileName() << "\n";
	ost << "2D preparation report\t" << GetPreparation2DFileName() << "\n";
	ost << "Modeling dictionary file\t" << GetModelingDictionaryFileName() << "\n";
	ost << "Modeling report\t" << GetModelingFileName() << "\n";
	ost << "Train evaluation report\t" << GetTrainEvaluationFileName() << "\n";
	ost << "Test evaluation report\t" << GetTestEvaluationFileName() << "\n";
	ost << "Visualization report\t" << GetVisualizationFileName() << "\n";
	ost << "JSON report\t" << GetJSONFileName() << "\n";
}

const ALString KWAnalysisResults::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

const ALString KWAnalysisResults::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
