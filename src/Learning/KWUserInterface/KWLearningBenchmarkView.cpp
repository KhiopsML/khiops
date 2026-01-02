// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWLearningBenchmarkView.h"

KWLearningBenchmarkView::KWLearningBenchmarkView()
{
	SetIdentifier("KWLearningBenchmark");
	SetLabel("Learning benchmark");
	AddIntField("CrossValidationNumber", "Cross-validation number", 0);
	AddIntField("FoldNumber", "Fold number", 0);
	AddBooleanField("Stratified", "Stratified", false);
	AddDoubleField("SignificanceLevel", "Significance level", 0);
	AddStringField("ReportFileName", "Benchmark report", "");
	AddBooleanField("RunReport", "Detailed run report", false);
	AddBooleanField("ExportBenchmarkDatabases", "Export fold datasets", false);

	// Parametrage des styles;
	GetFieldAt("CrossValidationNumber")->SetStyle("Spinner");
	GetFieldAt("FoldNumber")->SetStyle("Spinner");
	GetFieldAt("Stratified")->SetStyle("CheckBox");
	GetFieldAt("ReportFileName")->SetStyle("FileChooser");
	GetFieldAt("RunReport")->SetStyle("CheckBox");
	GetFieldAt("ExportBenchmarkDatabases")->SetStyle("CheckBox");

	// ## Custom constructor

	KWBenchmarkSpecArrayView* benchmarkSpecArrayView;
	KWPredictorSpecArrayView* predictorSpecArrayView;

	// Plage de valeur pour les parametres de cross-validation
	cast(UIIntElement*, GetFieldAt("CrossValidationNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("CrossValidationNumber"))->SetMaxValue(1000);
	cast(UIIntElement*, GetFieldAt("FoldNumber"))->SetMinValue(2);
	cast(UIIntElement*, GetFieldAt("FoldNumber"))->SetMaxValue(1000);
	cast(UIIntElement*, GetFieldAt("FoldNumber"))->SetDefaultValue(2);

	// Plage de valeur pour le niveau de test
	cast(UIDoubleElement*, GetFieldAt("SignificanceLevel"))->SetMinValue(0);
	cast(UIDoubleElement*, GetFieldAt("SignificanceLevel"))->SetMaxValue(1);
	cast(UIDoubleElement*, GetFieldAt("SignificanceLevel"))->SetDefaultValue(0.05);

	// Ajout d'une liste de benchmark et d'une liste de predicteurs
	benchmarkSpecArrayView = new KWBenchmarkSpecArrayView;
	AddListField("BenchmarkSpecs", "Benchmarks", benchmarkSpecArrayView);
	predictorSpecArrayView = new KWPredictorSpecArrayView;
	AddListField("PredictorSpecs", "Predictors", predictorSpecArrayView);

	// Passage de la liste des benchmarks en ergonomy Inspectable
	benchmarkSpecArrayView->SetErgonomy(UIList::Inspectable);

	// Ajout d'une action pour l'evaluation des predicteurs
	AddAction("Evaluate", "Evaluation of predictors", (ActionMethod)(&KWLearningBenchmarkView::Evaluate));

	// Passage en style onglets
	SetStyle("TabbedPanes");

	// ##
}

KWLearningBenchmarkView::~KWLearningBenchmarkView()
{
	// ## Custom destructor

	// ##
}

KWLearningBenchmark* KWLearningBenchmarkView::GetKWLearningBenchmark()
{
	require(objValue != NULL);
	return cast(KWLearningBenchmark*, objValue);
}

void KWLearningBenchmarkView::EventUpdate(Object* object)
{
	KWLearningBenchmark* editedObject;

	require(object != NULL);

	editedObject = cast(KWLearningBenchmark*, object);
	editedObject->SetCrossValidationNumber(GetIntValueAt("CrossValidationNumber"));
	editedObject->SetFoldNumber(GetIntValueAt("FoldNumber"));
	editedObject->SetStratified(GetBooleanValueAt("Stratified"));
	editedObject->SetSignificanceLevel(GetDoubleValueAt("SignificanceLevel"));
	editedObject->SetReportFileName(GetStringValueAt("ReportFileName"));
	editedObject->SetRunReport(GetBooleanValueAt("RunReport"));
	editedObject->SetExportBenchmarkDatabases(GetBooleanValueAt("ExportBenchmarkDatabases"));

	// ## Custom update

	// ##
}

void KWLearningBenchmarkView::EventRefresh(Object* object)
{
	KWLearningBenchmark* editedObject;

	require(object != NULL);

	editedObject = cast(KWLearningBenchmark*, object);
	SetIntValueAt("CrossValidationNumber", editedObject->GetCrossValidationNumber());
	SetIntValueAt("FoldNumber", editedObject->GetFoldNumber());
	SetBooleanValueAt("Stratified", editedObject->GetStratified());
	SetDoubleValueAt("SignificanceLevel", editedObject->GetSignificanceLevel());
	SetStringValueAt("ReportFileName", editedObject->GetReportFileName());
	SetBooleanValueAt("RunReport", editedObject->GetRunReport());
	SetBooleanValueAt("ExportBenchmarkDatabases", editedObject->GetExportBenchmarkDatabases());

	// ## Custom refresh

	// ##
}

const ALString KWLearningBenchmarkView::GetClassLabel() const
{
	return "Learning benchmark";
}

// ## Method implementation

void KWLearningBenchmarkView::Evaluate()
{
	KWLearningBenchmark* learningBenchmark;
	ALString sReportFileName = "Benchmark.xls";

	// Acces a l'objet
	learningBenchmark = GetLearningBenchmark();

	// Evaluation et ecriture d'un rapport si specifications valides
	if (learningBenchmark->Check())
	{
		// Demarage du suivi de la tache
		TaskProgression::SetTitle("Predictor evaluation");
		TaskProgression::SetDisplayedLevelNumber(3);
		TaskProgression::Start();

		// Evaluation
		learningBenchmark->Evaluate();

		// Ecriture d'un rapport d'evaluation
		sReportFileName = learningBenchmark->GetReportFileName();
		if (sReportFileName != "")
		{
			AddSimpleMessage("Write benchmark evaluation report " + sReportFileName);
			learningBenchmark->WriteReportFile(sReportFileName);
		}

		// Fin du suivi de la tache
		TaskProgression::Stop();
	}
}

void KWLearningBenchmarkView::SetObject(Object* object)
{
	KWLearningBenchmark* learningBenchmark;

	require(object != NULL);

	// Acces a l'objet edite
	learningBenchmark = cast(KWLearningBenchmark*, object);

	// Parametrage des sous-fiches
	cast(KWBenchmarkSpecArrayView*, GetFieldAt("BenchmarkSpecs"))
	    ->SetObjectArray(learningBenchmark->GetBenchmarkSpecs());
	cast(KWPredictorSpecArrayView*, GetFieldAt("PredictorSpecs"))
	    ->SetObjectArray(learningBenchmark->GetPredictorSpecs());

	// Parametrage des predicteurs autorises
	// Attention, le typage doit intervenir avant le filtrage
	cast(KWPredictorSpecArrayView*, GetFieldAt("PredictorSpecs"))
	    ->SetTargetAttributeType(learningBenchmark->GetTargetAttributeType());
	cast(KWPredictorSpecArrayView*, GetFieldAt("PredictorSpecs"))
	    ->SetPredictorFilter(learningBenchmark->GetPredictorFilter());

	// L'option d'echantillonnage stratifie n'est disponible que dans le cas de la classification
	GetFieldAt("Stratified")->SetVisible(learningBenchmark->GetTargetAttributeType() == KWType::Symbol);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWLearningBenchmark* KWLearningBenchmarkView::GetLearningBenchmark()
{
	return cast(KWLearningBenchmark*, objValue);
}

// ##
