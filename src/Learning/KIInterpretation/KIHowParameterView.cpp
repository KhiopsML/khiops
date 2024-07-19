// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIHowParameterView.h"
#include "KILeverVariablesSpecView.h"

KIHowParameterView::KIHowParameterView()
{
	leverVariablesSpecView = new KILeverVariablesSpecView;

	// Parametrage principal de l'interface
	SetIdentifier("KIHowParameter");
	SetLabel("Reinforcement Parameters");

	// Ajout d'un champ qui rappelle le nombre max de variables
	AddIntField("VarMax", "Number of variables in the model", 0);
	// On interdit l'acces au champ
	GetFieldAt("VarMax")->SetEditable(false);

	// Ajout d'un champ de saisie du nom de la modalite cible de reference pour
	// le renforcement de la probabilite d'appartenance
	AddStringField("HowClass", "Value of the class to reinforce", "");

	// Ajout d'un champ de saisie du nombre maximal autorise de variables
	// pour le renforcement de la classe de reference
	AddIntField("HowNumber", "Maximum number of variables written", 0);

	AddCardField("leverVariablesSpecView", "Lever variables", leverVariablesSpecView);

	GetFieldAt("HowNumber")->SetStyle("Spinner");
	GetFieldAt("HowClass")->SetStyle("ComboBox");

	// mettre visible uniquement en ExpertMode
	GetFieldAt("HowNumber")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("VarMax")->SetHelpText("Number of variables used by the predictor.");
	GetFieldAt("HowClass")->SetHelpText("Class value for which one try to increase the probability of occurrence.");
	GetFieldAt("leverVariablesSpecView")
	    ->SetHelpText("List of the variables included in the analysis to try to increase the probability of "
			  "occurrence - by default, all are used.");
	//GetFieldAt("VarMax")->SetHelpText("Number of variables used by the predictor.");
	//GetFieldAt("VarMax")->SetHelpText("Number of variables used by the predictor.");

	// Plage de valeur pour le champ du nombre W de variables du comment
	cast(UIIntElement*, GetFieldAt("VarMax"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("HowNumber"))->SetMinValue(0);
}

KIHowParameterView::~KIHowParameterView() {}

KIHowParameterView* KIHowParameterView::Create() const
{
	return new KIHowParameterView;
}

void KIHowParameterView::SetObject(Object* object)
{
	// La methode stocke l'objet passe en parametre, puis appelle EventRefresh

	UIObjectView::SetObject(object);
	KIInterpretationSpec* interpretationSpec = cast(KIInterpretationSpec*, object);
	leverVariablesSpecView = cast(KILeverVariablesSpecView*, GetCardValueAt("leverVariablesSpecView"));
	leverVariablesSpecView->SetObject(interpretationSpec->GetLeverClassSpec());
}

void KIHowParameterView::EventUpdate(Object* object)
{
	// Mise a jour des specs d'interpretation par les valeurs de l'interface

	KIInterpretationSpec* interpretationSpec;

	require(object != NULL);

	interpretationSpec = cast(KIInterpretationSpec*, object);

	interpretationSpec->SetHowAttributesNumber(GetIntValueAt("HowNumber"));
	interpretationSpec->SetHowClass(GetStringValueAt("HowClass"));
	interpretationSpec->SetMaxAttributesNumber(GetIntValueAt("VarMax"));
}

void KIHowParameterView::EventRefresh(Object* object)
{
	// Mise a jour des valeurs de l'interface par les valeurs des specs d'interpretation

	KIInterpretationSpec* editedObject;
	require(object != NULL);

	editedObject = cast(KIInterpretationSpec*, object);

	SetIntValueAt("HowNumber", editedObject->GetHowAttributesNumber());
	SetStringValueAt("HowClass", editedObject->GetHowClass());
	SetIntValueAt("VarMax", editedObject->GetMaxAttributesNumber());

	// Mise a jour des choix en fonction de l'etat de l'interpreteur

	ALString sTargetValues;
	for (int i = 0; i < editedObject->GetInterpretationDictionary()->GetTargetValues().GetSize(); i++)
	{
		if (i > 0)
			sTargetValues += "\n";
		sTargetValues += ALString(editedObject->GetInterpretationDictionary()->GetTargetValues().GetAt(i));
	}

	GetFieldAt("HowClass")->SetParameters(KIInterpretationDictionary::NO_VALUE_LABEL + "\n" + sTargetValues);
	cast(UIIntElement*, GetFieldAt("HowNumber"))
	    ->SetMaxValue(editedObject->GetInterpretationDictionary()->GetPredictiveAttributeNamesArray()->GetSize());
}
