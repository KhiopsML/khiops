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
	const boolean bTrace = true;
	ObjectArray oaAttributePairStats;
	KWAttributePairStats* resultPairStats;
	int n;

	require(learningSpec != NULL);
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(initialDataGridSolution != NULL);
	require(initialDataGridSolution->GetCellNumber() == 0);

	// Calcul des paires de variables
	ComputeInternalAttributesBivariateStats(initialDataGrid, &oaAttributePairStats);

	// Trace des paires
	if (bTrace)
	{
		cout << "Internal pair stats\t" << oaAttributePairStats.GetSize() << "\n";
		for (n = 0; n < oaAttributePairStats.GetSize(); n++)
		{
			resultPairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(n));
			//resultPairStats->WriteReport(cout);
		}
	}

	// Nettoyage
	oaAttributePairStats.DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::ComputeInternalAttributesBivariateStats(
    const KWDataGrid* initialDataGrid, ObjectArray* oaAttributePairStats) const
{
	KWLearningSpec bivariateLearningSpec;
	KWAttributePairsSpec bivariatePairSpec;
	KWClassStats bivariateClassStats;
	KWAttributePairStats* resultPairStats;
	ObjectArray oaAttributeStats;
	KWAttributePairName* pairName;
	int n1;
	int n2;

	require(initialDataGrid != NULL);
	require(oaAttributePairStats != NULL);
	require(oaAttributePairStats->GetSize() == 0);

	// Parametrage d'un objet classStats pour  calculer les statistiques bivariees
	bivariateLearningSpec.CopyFrom(GetLearningSpec());
	bivariateLearningSpec.GetPreprocessingSpec()->SetDiscretizerUnsupervisedMethodName("none");
	bivariateLearningSpec.GetPreprocessingSpec()->SetGrouperUnsupervisedMethodName("none");
	bivariatePairSpec.SetClassName(bivariateLearningSpec.GetClass()->GetName());
	bivariateClassStats.SetLearningSpec(&bivariateLearningSpec);

	// Parametrage des paires a analyser
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

	// Recopie des paires obtenues
	oaAttributePairStats->CopyFrom(bivariateClassStats.GetAttributePairStats());

	//DDD
	WriteJSONAnalysisReport(&bivariateClassStats, "c:\\temp\\TestBivariate.khj");

	// Nettoyage
	oaAttributeStats.CopyFrom(bivariateClassStats.GetAttributeStats());
	bivariateClassStats.RemoveAll();
	oaAttributeStats.DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::WriteJSONAnalysisReport(KWClassStats* classStats,
								  const ALString& sREportFileName) const
{
	JSONFile fJSON;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(sREportFileName != "");

	// Ouverture du rapport
	fJSON.SetFileName("c:\\temp\\TestBivariate.khj");
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
