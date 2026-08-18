// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridInitialSolutionSearcherIV.h"

KWDataGridInitialSolutionSearcherIV::KWDataGridInitialSolutionSearcherIV()
{
	learningSpec = NULL;
}

KWDataGridInitialSolutionSearcherIV::~KWDataGridInitialSolutionSearcherIV() {}

void KWDataGridInitialSolutionSearcherIV::SetLearningSpec(KWLearningSpec* specification)
{
	learningSpec = specification;
}

KWLearningSpec* KWDataGridInitialSolutionSearcherIV::GetLearningSpec() const
{
	return learningSpec;
}

void KWDataGridInitialSolutionSearcherIV::SearchInitialSolution(const KWDataGrid* initialDataGrid,
								KWDataGridMerger* initialDataGridSolution) const
{
	const boolean bTrace = false;
	ObjectArray oaAttributePairStats;
	KWAttributePairStats* resultPairStats;
	KWDataGridStats* pairStats;
	const KWDGSAttributePartition* attributePartition;
	int n;
	int nAttribute;

	require(learningSpec != NULL);
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(initialDataGridSolution != NULL);
	require(initialDataGridSolution->GetCellNumber() == 0);

	// Calcul des paires de variables
	ComputeInternalAttributesBivariateStats(initialDataGrid, &oaAttributePairStats);

	// Extraction des partition les plus fine pour chaque attribut, par fusion des partition de
	// chaque attribut impliquee dans une paire
	for (n = 0; n < oaAttributePairStats.GetSize(); n++)
	{
		resultPairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(n));
		pairStats = resultPairStats->GetPreparedDataGridStats();

		// Analyse de chaque attribut des paires non nulles
		if (pairStats != NULL)
		{
			for (nAttribute = 0; nAttribute < pairStats->GetAttributeNumber(); nAttribute++)
			{
				attributePartition = pairStats->GetAttributeAt(nAttribute);
				cout << attributePartition->GetAttributeName() << "\t";
				cout << KWType::ToString(attributePartition->GetAttributeType()) << "\t";
				cout << attributePartition->GetPartNumber() << "\t";
			}
			cout << "\n";
		}
	}

	// Trace des paires
	if (bTrace)
	{
		cout << "Internal pair stats\t" << oaAttributePairStats.GetSize() << "\n";
		for (n = 0; n < oaAttributePairStats.GetSize(); n++)
		{
			resultPairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(n));
			resultPairStats->WriteReport(cout);
		}
	}

	// Nettoyage
	oaAttributePairStats.DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::ComputeInternalAttributesBivariateStats(
    const KWDataGrid* initialDataGrid, ObjectArray* oaAttributePairStats) const
{
	const boolean bTrace = true;
	ALString sBivariateReportPath;
	KWAttributePairStats* resultPairStats;
	ObjectArray oaAttributeStats;
	KWAttributePairName* pairName;
	int n1;
	int n2;

	require(GetLearningSpec() != NULL);
	require(initialDataGrid != NULL);
	require(oaAttributePairStats != NULL);
	require(oaAttributePairStats->GetSize() == 0);

	// Parametrage des variables de travail pour  calculer les statistiques bivariees
	bivariateLearningSpec.CopyFrom(GetLearningSpec());
	bivariateLearningSpec.GetPreprocessingSpec()->SetDiscretizerUnsupervisedMethodName("none");
	bivariateLearningSpec.GetPreprocessingSpec()->SetGrouperUnsupervisedMethodName("none");
	bivariatePairSpec.SetClassName(bivariateLearningSpec.GetClass()->GetName());
	bivariateClassStats.SetLearningSpec(&bivariateLearningSpec);

	// Parametrage des paires a analyser
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();
	for (n1 = 0; n1 < initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber(); n1++)
	{
		for (n2 = n1 + 1; n2 < initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber(); n2++)
		{
			pairName = new KWAttributePairName;
			pairName->SetFirstName(
			    initialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(n1)->GetAttributeName());
			pairName->SetSecondName(
			    initialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(n2)->GetAttributeName());
			bivariatePairSpec.GetSpecificAttributePairs()->Add(pairName);
		}
	}
	bivariateClassStats.SetAttributePairsSpec(&bivariatePairSpec);
	bivariatePairSpec.SetMaxAttributePairNumber(bivariatePairSpec.GetSpecificAttributePairs()->GetSize());

	// Calcul des statistques sur les paires de variables
	bivariateClassStats.ComputeStats();

	// Trace
	if (bTrace)
	{
		sBivariateReportPath =
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "CoclusteringBivariate.khj");
		cout << GetLearningSpec()->GetClass()->GetName()
		     << " coclustering bivariate report: " << sBivariateReportPath << "\n";
		WriteJSONAnalysisReport(&bivariateClassStats, sBivariateReportPath);
	}

	// Recopie des paires obtenues
	oaAttributePairStats->CopyFrom(bivariateClassStats.GetAttributePairStats());

	// Nettoyage
	oaAttributeStats.CopyFrom(bivariateClassStats.GetAttributeStats());
	bivariateClassStats.RemoveAll();
	oaAttributeStats.DeleteAll();
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::WriteJSONAnalysisReport(KWClassStats* classStats,
								  const ALString& sReportFileName) const
{
	JSONFile fJSON;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(sReportFileName != "");

	// Ouverture du rapport
	fJSON.SetFileName(sReportFileName);
	fJSON.OpenForWrite();

	// Ecriture
	if (fJSON.IsOpened())
	{

		// Outil et version
		fJSON.WriteKeyString("tool", GetLearningApplicationName());
		if (GetLearningModuleName() != "")
			fJSON.WriteKeyString("sub_tool", GetLearningModuleName());
		fJSON.WriteKeyString("version", GetLearningVersion());

		// Description courte
		fJSON.WriteKeyString("shortDescription", "Bivariate analysis for initialization a cocluystereing IxV");

		// Rapport de preparation complet
		classStats->SetWriteOptionStatsNativeOrConstructed(true);
		classStats->WriteJSONKeyReport(&fJSON, "preparationReport");
		classStats->SetWriteOptionStatsNativeOrConstructed(false);

		// Rapport de preparation bivarie
		if (classStats->GetAttributePairStats()->GetSize() > 0)
		{
			classStats->SetWriteOptionStats2D(true);
			classStats->WriteJSONKeyReport(&fJSON, "bivariatePreparationReport");
			classStats->SetWriteOptionStats2D(false);
		}

		// Fermeture du fichier
		fJSON.Close();
	}
}
