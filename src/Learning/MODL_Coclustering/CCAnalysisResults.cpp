// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCAnalysisResults.h"

CCAnalysisResults::CCAnalysisResults()
{
	bExportAsKhc = false;

	// ## Custom constructor

	sCoclusteringFileName = "Coclustering.khcj";
	sClusterFileName = "Clusters.txt";
	sPostProcessedCoclusteringFileName = "SimplifiedCoclustering.khcj";
	sCoclusteringDictionaryFileName = "Coclustering.kdic";

	// Option DEPRECATED, pour l'instant active par defaut
	bExportAsKhc = true;

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

	sCoclusteringFileName = aSource->sCoclusteringFileName;
	sShortDescription = aSource->sShortDescription;
	bExportAsKhc = aSource->bExportAsKhc;
	sInputCoclusteringFileName = aSource->sInputCoclusteringFileName;
	sClusterFileName = aSource->sClusterFileName;
	sPostProcessedCoclusteringFileName = aSource->sPostProcessedCoclusteringFileName;
	sCoclusteringDictionaryFileName = aSource->sCoclusteringDictionaryFileName;

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
	ost << "Coclustering report\t" << GetCoclusteringFileName() << "\n";
	ost << "Short description\t" << GetShortDescription() << "\n";
	ost << "Export as khc\t" << BooleanToString(GetExportAsKhc()) << "\n";
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

void CCAnalysisResults::SetTrainDatabase(const KWDatabase* database)
{
	trainDatabase = database;
}

const KWDatabase* CCAnalysisResults::GetTrainDatabase() const
{
	return trainDatabase;
}

// ##
