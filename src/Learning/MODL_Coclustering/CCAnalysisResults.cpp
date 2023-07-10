// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCAnalysisResults.h"

CCAnalysisResults::CCAnalysisResults()
{
	bExportJSON = false;

	// ## Custom constructor

	sCoclusteringFileName = "Coclustering.khc";
	sClusterFileName = "Clusters.txt";
	sPostProcessedCoclusteringFileName = "SimplifiedCoclustering.khc";
	sCoclusteringDictionaryFileName = "Coclustering.kdic";

	// Export JSON par defaut
	bExportJSON = true;
	// TODO JSON
	// bExportJSON = false; // Pour passer les tests

	// ##
}

CCAnalysisResults::~CCAnalysisResults()
{
	// ## Custom destructor

	// ##
}

void CCAnalysisResults::CopyFrom(const CCAnalysisResults* aSource)
{
	require(aSource != NULL);

	sResultFilesDirectory = aSource->sResultFilesDirectory;
	sResultFilesPrefix = aSource->sResultFilesPrefix;
	sShortDescription = aSource->sShortDescription;
	sCoclusteringFileName = aSource->sCoclusteringFileName;
	sInputCoclusteringFileName = aSource->sInputCoclusteringFileName;
	sClusterFileName = aSource->sClusterFileName;
	sPostProcessedCoclusteringFileName = aSource->sPostProcessedCoclusteringFileName;
	sCoclusteringDictionaryFileName = aSource->sCoclusteringDictionaryFileName;
	bExportJSON = aSource->bExportJSON;

	// ## Custom copyfrom

	// ##
}

CCAnalysisResults* CCAnalysisResults::Clone() const
{
	CCAnalysisResults* aClone;

	aClone = new CCAnalysisResults;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void CCAnalysisResults::Write(ostream& ost) const
{
	ost << "Result files directory\t" << GetResultFilesDirectory() << "\n";
	ost << "Result files prefix\t" << GetResultFilesPrefix() << "\n";
	ost << "Short description\t" << GetShortDescription() << "\n";
	ost << "Coclustering report\t" << GetCoclusteringFileName() << "\n";
	ost << "Input coclustering report\t" << GetInputCoclusteringFileName() << "\n";
	ost << "Cluster table file\t" << GetClusterFileName() << "\n";
	ost << "Simplified coclustering report\t" << GetPostProcessedCoclusteringFileName() << "\n";
	ost << "Coclustering dictionary file\t" << GetCoclusteringDictionaryFileName() << "\n";
	ost << "Export JSON\t" << BooleanToString(GetExportJSON()) << "\n";
}

const ALString CCAnalysisResults::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

const ALString CCAnalysisResults::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
