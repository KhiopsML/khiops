// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2017-08-09 17:11:48
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "MDKhiopsModelingSpecView.h"

MDKhiopsModelingSpecView::MDKhiopsModelingSpecView()
{
	SetIdentifier("MDKhiopsModelingSpec");
	SetLabel("Modeling parameters");
	AddBooleanField("SelectiveNaiveBayesPredictor", "Selective Naive Bayes predictor", false);
	AddBooleanField("MAPNaiveBayesPredictor", "MAP Naive Bayes predictor", false);
	AddBooleanField("NaiveBayesPredictor", "Naive Bayes predictor", false);
	AddBooleanField("DataGridPredictor", "Data Grid predictor", false);

	// Parametrage des styles;
	GetFieldAt("SelectiveNaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("MAPNaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("NaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("DataGridPredictor")->SetStyle("CheckBox");

	// ## Custom constructor

	// Deplacement des nouveaux champs vers le haut, avant ceux de la classe ancetre
	ALString sFirstFieldId = GetFieldAtIndex(0)->GetIdentifier();
	MoveFieldBefore("SelectiveNaiveBayesPredictor", sFirstFieldId);
#ifdef DEPRECATED_V10
	// Supprimer MAPNaiveBayesPredictor de MDKhiopsModelingSpec.dd et regenerer la classe
	MoveFieldBefore("MAPNaiveBayesPredictor", sFirstFieldId);
#endif // DEPRECATED_V10
	MoveFieldBefore("NaiveBayesPredictor", sFirstFieldId);
	MoveFieldBefore("DataGridPredictor", sFirstFieldId);

#ifdef DEPRECATED_V10
	{
		// Supprimer MAPNaiveBayesPredictor de MDKhiopsModelingSpec.dd et regenerer la classe
		// Declaration des actions DEPRECATED
		AddAction("InspectSelectiveNaiveBayesParameters", "Selective Naive Bayes expert parameters",
			  (ActionMethod)(&MDKhiopsModelingSpecView::DEPRECATEDInspectSelectiveNaiveBayesParameters));
		GetActionAt("InspectSelectiveNaiveBayesParameters")->SetStyle("Button");
	}
#endif // DEPRECATED_V10

	// Declaration des actions
	AddAction("InspectDataGridParameters", "Data Grid expert parameters",
		  (ActionMethod)(&MDKhiopsModelingSpecView::InspectDataGridParameters));
	GetActionAt("InspectDataGridParameters")->SetStyle("Button");

	// Fonctionnalites avancees, disponible uniquement en mode expert
	GetFieldAt("NaiveBayesPredictor")->SetVisible(GetLearningExpertMode());
	GetFieldAt("DataGridPredictor")->SetVisible(GetLearningExpertMode());
	GetActionAt("InspectDataGridParameters")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("SelectiveNaiveBayesPredictor")
	    ->SetHelpText(
		"Build a Selective Naive Bayes predictor."
		"\n The Selective Naive Bayes predictor performs a variable selection using a greedy heuristic."
		"\n The selection consists in successive Forward and Backward passes, and is repeated several times"
		"\n in order to optimize variable weights.");
	GetFieldAt("NaiveBayesPredictor")->SetHelpText("Build a Naive Bayes predictor.");
	GetFieldAt("DataGridPredictor")
	    ->SetHelpText("Build a Data Grid predictor."
			  "\n (expert mode only)");
	GetActionAt("InspectDataGridParameters")
	    ->SetHelpText("Inspect advanced parameters for the Data Grid predictor.");

	// Short cuts
	GetActionAt("InspectDataGridParameters")->SetShortCut('G');

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite
		// ascendante des scenarios
		GetActionAt("InspectSelectiveNaiveBayesParameters")
		    ->SetHelpText(
			"Inspect advanced parameters for the Selective Naive Bayes predictor."
			"\n These parameters are user constraints that allow to control the variable selection process."
			"\n Their use might decrease the performance, compared to the default mode (without user "
			"constraints).");
		//
		GetActionAt("InspectSelectiveNaiveBayesParameters")->SetShortCut('X');
		//
		GetFieldAt("MAPNaiveBayesPredictor")->SetVisible(false);
		GetActionAt("InspectSelectiveNaiveBayesParameters")->SetVisible(false);
	}
#endif // DEPRECATED_V10

	// ##
}

MDKhiopsModelingSpecView::~MDKhiopsModelingSpecView()
{
	// ## Custom destructor

	// ##
}

MDKhiopsModelingSpec* MDKhiopsModelingSpecView::GetMDKhiopsModelingSpec()
{
	require(objValue != NULL);
	return cast(MDKhiopsModelingSpec*, objValue);
}

void MDKhiopsModelingSpecView::EventUpdate(Object* object)
{
	MDKhiopsModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventUpdate(object);
	editedObject = cast(MDKhiopsModelingSpec*, object);
	editedObject->SetSelectiveNaiveBayesPredictor(GetBooleanValueAt("SelectiveNaiveBayesPredictor"));
	editedObject->SetMAPNaiveBayesPredictor(GetBooleanValueAt("MAPNaiveBayesPredictor"));
	editedObject->SetNaiveBayesPredictor(GetBooleanValueAt("NaiveBayesPredictor"));
	editedObject->SetDataGridPredictor(GetBooleanValueAt("DataGridPredictor"));

	// ## Custom update

	// ##
}

void MDKhiopsModelingSpecView::EventRefresh(Object* object)
{
	MDKhiopsModelingSpec* editedObject;

	require(object != NULL);

	KWModelingSpecView::EventRefresh(object);
	editedObject = cast(MDKhiopsModelingSpec*, object);
	SetBooleanValueAt("SelectiveNaiveBayesPredictor", editedObject->GetSelectiveNaiveBayesPredictor());
	SetBooleanValueAt("MAPNaiveBayesPredictor", editedObject->GetMAPNaiveBayesPredictor());
	SetBooleanValueAt("NaiveBayesPredictor", editedObject->GetNaiveBayesPredictor());
	SetBooleanValueAt("DataGridPredictor", editedObject->GetDataGridPredictor());

	// ## Custom refresh

	// ##
}

const ALString MDKhiopsModelingSpecView::GetClassLabel() const
{
	return "Modeling parameters";
}

// ## Method implementation

#ifdef DEPRECATED_V10
void MDKhiopsModelingSpecView::DEPRECATEDInspectSelectiveNaiveBayesParameters()
{
	static boolean bWarningEmited = false;
	const SNBPredictorSelectiveNaiveBayesView refPredictorSelectiveNaiveBayesView;
	KWPredictorView* predictorSelectiveNaiveBayesView;
	KWModelingSpec* modelingSpec;

	// Warning utilisateur
	if (not bWarningEmited)
	{
		AddWarning(
		    "Button 'Selective Naive Bayes expert parameters' is deprecated in this pane since Khiops V10 :"
		    "\n   use button from pane 'Parameters/Predictors/Advanced predictor parameters' instead");
		bWarningEmited = true;
	}

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Creation de la fiche specialise pour le predicteur
	predictorSelectiveNaiveBayesView =
	    KWPredictorView::ClonePredictorView(refPredictorSelectiveNaiveBayesView.GetName());

	// Ouverture de la sous-fiche
	predictorSelectiveNaiveBayesView->SetObject(modelingSpec->GetPredictorSelectiveNaiveBayes());
	predictorSelectiveNaiveBayesView->Open();

	// Nettoyage
	delete predictorSelectiveNaiveBayesView;
}
#endif // DEPRECATED_V10

void MDKhiopsModelingSpecView::InspectDataGridParameters()
{
	KWPredictorDataGridView predictorDataGridView;
	KWModelingSpec* modelingSpec;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	predictorDataGridView.SetObject(modelingSpec->GetPredictorDataGrid());
	predictorDataGridView.Open();
}

// ##
