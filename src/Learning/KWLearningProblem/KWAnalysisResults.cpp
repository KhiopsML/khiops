// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAnalysisResults.h"

KWAnalysisResults::KWAnalysisResults()
{
	bExportAsXls = false;

	// ## Custom constructor

	trainDatabase = NULL;
	sReportFileName = "AnalysisResults.khj";
	sPreparationFileName = "PreparationReport.xls";
	sTextPreparationFileName = "TextPreparationReport.xls";
	sTreePreparationFileName = "TreePreparationReport.xls";
	sPreparation2DFileName = "Preparation2DReport.xls";
	sModelingDictionaryFileName = "model.kdic";
	sModelingFileName = "ModelingReport.xls";
	sTrainEvaluationFileName = "TrainEvaluationReport.xls";
	sTestEvaluationFileName = "TestEvaluationReport.xls";

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

	sReportFileName = aSource->sReportFileName;
	sShortDescription = aSource->sShortDescription;
	bExportAsXls = aSource->bExportAsXls;
	sPreparationFileName = aSource->sPreparationFileName;
	sTextPreparationFileName = aSource->sTextPreparationFileName;
	sTreePreparationFileName = aSource->sTreePreparationFileName;
	sPreparation2DFileName = aSource->sPreparation2DFileName;
	sModelingDictionaryFileName = aSource->sModelingDictionaryFileName;
	sModelingFileName = aSource->sModelingFileName;
	sTrainEvaluationFileName = aSource->sTrainEvaluationFileName;
	sTestEvaluationFileName = aSource->sTestEvaluationFileName;

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
	ost << "Analysis report\t" << GetReportFileName() << "\n";
	ost << "Short description\t" << GetShortDescription() << "\n";
	ost << "Export as xls\t" << BooleanToString(GetExportAsXls()) << "\n";
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

boolean KWAnalysisResults::CheckResultDirectory() const
{
	// Verification du repertoire
	return GetResultFilePathBuilder()->CheckResultDirectory("Train model");
}

const ALString KWAnalysisResults::BuildOutputFilePathName(const ALString& sOutputFileName) const
{
	// Construction du nom du fichier en sortie
	if (sOutputFileName == GetReportFileName())
		return GetResultFilePathBuilder()->BuildResultFilePathName();
	else
	{
		assert(
		    sOutputFileName == GetPreparationFileName() or sOutputFileName == GetTextPreparationFileName() or
		    sOutputFileName == GetTreePreparationFileName() or sOutputFileName == GetPreparation2DFileName() or
		    sOutputFileName == GetModelingDictionaryFileName() or sOutputFileName == GetModelingFileName() or
		    sOutputFileName == GetTrainEvaluationFileName() or sOutputFileName == GetTestEvaluationFileName());
		return GetResultFilePathBuilder()->BuildOtherResultFilePathName(sOutputFileName);
	}
}

void KWAnalysisResults::SetTrainDatabase(const KWDatabase* database)
{
	trainDatabase = database;
}

const KWDatabase* KWAnalysisResults::GetTrainDatabase() const
{
	return trainDatabase;
}

const KWResultFilePathBuilder* KWAnalysisResults::GetResultFilePathBuilder() const
{
	require(GetTrainDatabase() != NULL);

	resultFilePathBuilder.SetInputFilePathName(GetTrainDatabase()->GetDatabaseName());
	resultFilePathBuilder.SetOutputFilePathName(GetReportFileName());
	resultFilePathBuilder.SetFileSuffix("khj");
	return &resultFilePathBuilder;
}

// ##
