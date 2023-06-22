// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorEvaluatorView.h"

KWPredictorEvaluatorView::KWPredictorEvaluatorView()
{
	KWDatabaseView* evaluationDatabaseView;
	KWEvaluatedPredictorSpecArrayView* evaluatedPredictorsSpecArrayView;

	SetIdentifier("KWPredictorEvaluator");
	SetLabel("Evaluate model");
	AddStringField("EvaluationFileName", "Evaluation report", "");
	AddBooleanField("ExportJSON", "Export JSON", true);
	AddStringField("MainTargetModality", "Main target value", "");

	// Parametrage des styles des champs
	GetFieldAt("EvaluationFileName")->SetStyle("FileChooser");

	// Fiche pour la base d'evaluation, creee de facon generique
	evaluationDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	AddCardField("EvaluationDatabase", "Evaluation database", evaluationDatabaseView);
	evaluationDatabaseView->GetFieldAt("ClassName")->SetEditable(false);
	evaluationDatabaseView->GetFieldAt("ClassName")->SetLabel("Initial dictionary");

	// Specialisation du parametrage des listes d'aide de la base
	evaluationDatabaseView->SetHelpListViewPath("EvaluationDatabase");

	// Liste des specification des predicteurs a evaluer
	evaluatedPredictorsSpecArrayView = new KWEvaluatedPredictorSpecArrayView;
	AddListField("EvaluatedPredictors", "Evaluated predictors", evaluatedPredictorsSpecArrayView);
	evaluatedPredictorsSpecArrayView->SetEditable(false);
	evaluatedPredictorsSpecArrayView->GetFieldAt("Evaluated")->SetEditable(true);

	// Ajout de l'action d'evaluation
	AddAction("EvaluatePredictors", "Evaluate model",
		  (ActionMethod)(&KWPredictorEvaluatorView::EvaluatePredictors));
	GetActionAt("EvaluatePredictors")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("EvaluationFileName")->SetHelpText("Name of the evaluation report file.");
	GetFieldAt("ExportJSON")
	    ->SetHelpText("Export the evaluation report under a JSON format."
			  "\n The exported JSON file has the same name as the evaluation report file, with a ." +
			  KWPredictorEvaluator::GetJSONReportSuffix() +
			  " extension."
			  "\n The JSON file is useful to inspect the evaluation results from any external tool.");
	GetFieldAt("MainTargetModality")
	    ->SetHelpText("Value of the target variable in case of classification,"
			  "\n for the lift curves in the evaluation reports.");
	GetActionAt("EvaluatePredictors")
	    ->SetHelpText(
		"Evaluate predictors on a database."
		"\n This action applies the evaluated predictors on the evaluation database and writes an evaluation "
		"report."
		"\n Whereas the 'Train model' action trains predictors and evaluates them immediately on the train and "
		"test databases,"
		"\n the 'Evaluate model' action allows a differed evaluation of previously trained predictors.");

	// Short cuts
	GetActionAt("EvaluatePredictors")->SetShortCut('E');
}

KWPredictorEvaluatorView::~KWPredictorEvaluatorView() {}

void KWPredictorEvaluatorView::EventUpdate(Object* object)
{
	KWPredictorEvaluator* predictorEvaluator;

	require(object != NULL);

	predictorEvaluator = cast(KWPredictorEvaluator*, object);
	predictorEvaluator->SetEvaluationFileName(GetStringValueAt("EvaluationFileName"));
	predictorEvaluator->SetExportJSON(GetBooleanValueAt("ExportJSON"));
	predictorEvaluator->SetMainTargetModality(GetStringValueAt("MainTargetModality"));
}

void KWPredictorEvaluatorView::EventRefresh(Object* object)
{
	KWPredictorEvaluator* predictorEvaluator;

	require(object != NULL);

	predictorEvaluator = cast(KWPredictorEvaluator*, object);
	SetStringValueAt("EvaluationFileName", predictorEvaluator->GetEvaluationFileName());
	SetBooleanValueAt("ExportJSON", predictorEvaluator->GetExportJSON());
	SetStringValueAt("MainTargetModality", predictorEvaluator->GetMainTargetModality());
}

void KWPredictorEvaluatorView::Open(KWPredictorEvaluator* predictorEvaluator)
{
	KWDatabaseView* evaluationDatabaseView;
	boolean bIsMultiTableTechnology;
	KWClassDomain* kwcdInitialClassesDomain;
	KWClassDomain* kwcdCurrentDomain;
	KWClass* kwcClass;
	const int nMaxTotalShownLineNumber = 12;
	int nMaxInputNativeRelationAttributeNumber;
	int nEvaluatedPredictorSpecNumber;
	int i;

	require(predictorEvaluator != NULL);

	// Acces aux vues sur les bases source et cible
	evaluationDatabaseView = cast(KWDatabaseView*, GetFieldAt("EvaluationDatabase"));

	// On determine si on est dans le cas multi-table par recherche d'une sous-vue sur les fichiers
	bIsMultiTableTechnology = evaluationDatabaseView->GetFieldIndex("DatabaseFiles") != -1;

	// Acces au domaines de classes courant et initiaux
	kwcdCurrentDomain = KWClassDomain::GetCurrentDomain();
	kwcdInitialClassesDomain = predictorEvaluator->GetInitialClassesDomain();

	// On positionne le domaine des classes initiales comme domaine courant
	// Cela permet ainsi le parametrage de la base d'evaluation par les
	// classes initiales des predicteurs
	if (kwcdInitialClassesDomain != NULL)
		KWClassDomain::SetCurrentDomain(kwcdInitialClassesDomain);

	// Collecte de stats sur la taille des mapping des dictionnaires
	nMaxInputNativeRelationAttributeNumber = 0;
	for (i = 0; i < KWClassDomain::GetCurrentDomain()->GetClassNumber(); i++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(i);

		// Mise a jour des nombre max de mappings
		if (bIsMultiTableTechnology)
		{
			nMaxInputNativeRelationAttributeNumber =
			    max(nMaxInputNativeRelationAttributeNumber,
				kwcClass->ComputeOverallNativeRelationAttributeNumber(true));
		}
	}

	// Nombre de predicteur evalues
	nEvaluatedPredictorSpecNumber = predictorEvaluator->GetEvaluatedPredictorSpecs()->GetSize();

	// Cas ou on depasse le max de ce qui est affichable
	if (nMaxInputNativeRelationAttributeNumber + nEvaluatedPredictorSpecNumber > nMaxTotalShownLineNumber)
	{
		nEvaluatedPredictorSpecNumber = min(nEvaluatedPredictorSpecNumber, nMaxTotalShownLineNumber / 2);
		nMaxInputNativeRelationAttributeNumber = nMaxTotalShownLineNumber - nEvaluatedPredictorSpecNumber;
	}

	// Parametrage des tailles des listes de fichier des mapping en entree dans le cas multi-table
	if (bIsMultiTableTechnology)
		cast(UIObjectArrayView*, evaluationDatabaseView->GetFieldAt("DatabaseFiles"))
		    ->SetLineNumber(1 + nMaxInputNativeRelationAttributeNumber);

	// Parametrage des liste de predicteurs evalues
	cast(UIList*, GetFieldAt("EvaluatedPredictors"))->SetLineNumber(max(1, nEvaluatedPredictorSpecNumber));

	// Parametrage de l'objet, une fois que les dommaine de classe a bascule vers le domaine initial
	// Ainsi, comme la base d'evaluation est parametree avec les classes initiales, la mise
	// a jour des ses mapping conserve ses specifications
	SetObject(predictorEvaluator);

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();

	// Restitution du dimaine courant
	if (kwcdInitialClassesDomain != NULL)
		KWClassDomain::SetCurrentDomain(kwcdCurrentDomain);
}

void KWPredictorEvaluatorView::EvaluatePredictors()
{
	KWPredictorEvaluator* predictorEvaluator;
	ObjectArray oaEvaluatedPredictors;

	// Execution controlee par licence
	if (not LMLicenseManager::RequestLicenseKey())
		return;

	// Recherche de l'objet edite
	predictorEvaluator = cast(KWPredictorEvaluator*, GetObject());
	check(predictorEvaluator);

	// Evaluation
	predictorEvaluator->EvaluatePredictorSpecs();
	AddSimpleMessage("");
}

void KWPredictorEvaluatorView::SetObject(Object* object)
{
	KWPredictorEvaluator* predictorEvaluator;

	require(object != NULL);

	// Acces a l'objet edite
	predictorEvaluator = cast(KWPredictorEvaluator*, object);

	// Parametrage des sous fenetres
	cast(KWDatabaseView*, GetFieldAt("EvaluationDatabase"))->SetObject(predictorEvaluator->GetEvaluationDatabase());
	cast(KWEvaluatedPredictorSpecArrayView*, GetFieldAt("EvaluatedPredictors"))
	    ->SetObjectArray(predictorEvaluator->GetEvaluatedPredictorSpecs());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

const ALString KWPredictorEvaluatorView::GetClassLabel() const
{
	return "Evaluate model";
}
