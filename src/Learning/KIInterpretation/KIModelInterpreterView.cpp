// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelInterpreterView.h"

KIModelInterpreterView::KIModelInterpreterView()
{
	SetIdentifier("KIModelInterpreter");
	SetLabel("Interpret model");
	AddStringField("ShapleyValueRanking", "Shapley values ranking", "");
	AddIntField("ContributionAttributeNumber", "Number of contribution variables", 0);

	// Parametrage des styles;
	GetFieldAt("ShapleyValueRanking")->SetStyle("ComboBox");
	GetFieldAt("ContributionAttributeNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Types de ranking pour les valeurs de Shapley
	GetFieldAt("ShapleyValueRanking")->SetParameters("Global\nIndividual");

	// Parametrage des nombres min et max
	cast(UIIntElement*, GetFieldAt("ContributionAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("ContributionAttributeNumber"))
	    ->SetMaxValue(KIModelInterpreter::GetMaxContributionAttributeNumber());

	// Ajout de l'action de construction d'un dictionnaire d'interpretation
	AddAction("BuildInterpretationClass", "Build interpretation dictionary...",
		  (ActionMethod)(&KIModelInterpreterView::BuildInterpretationClass));
	GetActionAt("BuildInterpretationClass")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ShapleyValueRanking")
	    ->SetHelpText("Ranking of the Shapley values produced by the interpretation model"
			  "\n- Global: predictor variables are ranked by decreasing global importance"
			  "\n- Individual: predictor variables are ranked by decreasing individual Shapley value");
	GetFieldAt("ContributionAttributeNumber")
	    ->SetHelpText("Number of predictor variables exploited the interpretation model");
	GetActionAt("BuildInterpretationClass")
	    ->SetHelpText(
		"Build an interpretation dictionary that computes the Shapley values"
		"\n"
		"\n The interpretation model produces the following variables according to the ranking of the "
		"Shapley values:"
		"\n - Global: the value of each contribution variable is output, as well as the Shapley value for "
		"\n each target value and predictor variable, based on their global importance"
		"\n - Individual: three variables are output for each target value and ranked individual "
		"\n importance: name, part and Shapley value of the predictor variable");

	// ##
}

KIModelInterpreterView::~KIModelInterpreterView()
{
	// ## Custom destructor

	// ##
}

KIModelInterpreter* KIModelInterpreterView::GetKIModelInterpreter()
{
	require(objValue != NULL);
	return cast(KIModelInterpreter*, objValue);
}

void KIModelInterpreterView::EventUpdate(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventUpdate(object);
	editedObject = cast(KIModelInterpreter*, object);
	editedObject->SetShapleyValueRanking(GetStringValueAt("ShapleyValueRanking"));
	editedObject->SetContributionAttributeNumber(GetIntValueAt("ContributionAttributeNumber"));

	// ## Custom update

	// ##
}

void KIModelInterpreterView::EventRefresh(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventRefresh(object);
	editedObject = cast(KIModelInterpreter*, object);
	SetStringValueAt("ShapleyValueRanking", editedObject->GetShapleyValueRanking());
	SetIntValueAt("ContributionAttributeNumber", editedObject->GetContributionAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KIModelInterpreterView::GetClassLabel() const
{
	return "Interpret model";
}

// ## Method implementation

void KIModelInterpreterView::BuildInterpretationClass()
{
	boolean bOk;
	ALString sPredictorClassName;
	UIFileChooserCard registerCard;
	ALString sInterpretationClassFileName;
	KWResultFilePathBuilder resultFilePathBuilder;
	KWClass* interpreterClass;
	int nOutputAttributeNumber;
	ALString sTmp;

	// Test de la validite des specifications
	bOk = GetKIModelInterpreter()->Check();

	// Construction de l'interpreteur
	if (bOk)
	{
		// Creation d'un nom de fichier de dictionnaire par defaut
		sInterpretationClassFileName = ChooseDictionaryFileName("Interpretation");

		// Verification du nom du fichier de dictionnaire
		if (sInterpretationClassFileName != "")
		{
			// Construction du dictionnaire
			interpreterClass = GetKIModelInterpreter()->GetClassBuilder()->BuildInterpretationClass(
			    GetKIModelInterpreter());

			// Nombre de variables utilisee en sortie
			interpreterClass->IndexClass();
			nOutputAttributeNumber = interpreterClass->GetUsedAttributeNumber();

			// Message utilisateur
			AddSimpleMessage("Write interpretation dictionary " + interpreterClass->GetName() + " (" +
					 IntToString(nOutputAttributeNumber) + " output variables) to file " +
					 sInterpretationClassFileName);

			// Warning si trop de variables en sortie
			if (nOutputAttributeNumber >
			    GetKIModelInterpreter()->GetMaxOutputAttributeNumberWithoutWarning())
				interpreterClass->AddWarning(sTmp +
							     "Interpretation dictionary with many output variables");

			// Eciture du dictionnaire
			interpreterClass->GetDomain()->WriteFile(sInterpretationClassFileName);

			// Nettoyage
			delete interpreterClass->GetDomain();
		}
	}

	// Ligne de separation dans le log si une erreur affiche, ou action effectuee
	if (not bOk or sInterpretationClassFileName != "")
		AddSimpleMessage("");
}

// ##
