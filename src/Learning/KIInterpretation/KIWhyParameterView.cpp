// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIWhyParameterView.h"

KIWhyParameterView::KIWhyParameterView()
{
	// Parametrage principal de l'interface
	SetIdentifier("ISWhyParameter");

	// Ajout d'un champ qui rappelle le nombre max de variables
	AddIntField("VarMax", "Maximum number of variables", 0);
	// On interdit l'acces au champ
	GetFieldAt("VarMax")->SetEditable(false);

	// Ajout d'un champ de saisie du nombre maximal autorise de variables
	// dont l'importance est ecrite dans le fichier de sortie
	AddIntField("WhyNumber", "Maximum number of variables written", 0);

	// Ajout d'un champ de saisie pour indiquer quelle classe on souhaite interpreter
	AddStringField("WhyClass", "Class for the contribution analysis", "");

	// Choix de la methode de calcul de l'importance d'une variable
	AddStringField("WhyType", "Type of the contribution indicator", "");

	// Choix de trier ou non les resultats par importance
	AddBooleanField("SortWhy", "Sort contribution results", true);
	// Possibilite d'opter pour le mode Expert (sorties reduites)
	AddBooleanField("ExpertMode", "Expert mode", false);

	GetFieldAt("WhyClass")->SetStyle("ComboBox");
	GetFieldAt("WhyType")->SetStyle("ComboBox");

	GetFieldAt("WhyType")->SetParameters(
	    ALString(
		"Normalized odds ratio\nWeight of evidence\nInformation difference\nDifference of probabilities\n") +
	    ALString("Log minimum of variable probabilities difference\nMinimum of variable probabilities "
		     "difference\nModality probability\n") +
	    ALString("Log modality probability\nBayes distance\nBayes distance without prior\nKullback\nShapley"));

	// Parametrage des styles;
	GetFieldAt("WhyNumber")->SetStyle("Spinner");

	// Plage de valeur pour le champ du nombre W de variables du pourquoi? et du comment
	cast(UIIntElement*, GetFieldAt("VarMax"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("WhyNumber"))->SetMinValue(0);

	GetFieldAt("ExpertMode")->SetVisible(GetLearningExpertMode());
	GetFieldAt("WhyType")->SetVisible(GetLearningExpertMode());
}

KIWhyParameterView::~KIWhyParameterView() {}

KIWhyParameterView* KIWhyParameterView::Create() const
{
	return new KIWhyParameterView;
}

void KIWhyParameterView::EventUpdate(Object* object)
{
	/// Mise a jour des specs d'interpretation par les valeurs de l'interface

	KIInterpretationSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KIInterpretationSpec*, object);

	editedObject->SetWhyAttributesNumber(GetIntValueAt("WhyNumber"));
	editedObject->SetMaxAttributesNumber(GetIntValueAt("VarMax"));
	editedObject->SetWhyType(GetStringValueAt("WhyType"));
	editedObject->SetWhyClass(GetStringValueAt("WhyClass"));
	editedObject->SetSortWhyResults(GetBooleanValueAt("SortWhy"));
	editedObject->SetExpertMode(GetBooleanValueAt("ExpertMode"));
}

void KIWhyParameterView::EventRefresh(Object* object)
{
	/// Mise a jour des valeurs de l'interface par les specs d'interpretation

	KIInterpretationSpec* editedObject;
	require(object != NULL);

	editedObject = cast(KIInterpretationSpec*, object);

	// Mise a jour des parametres de l'interface par les valeurs des parametres
	SetIntValueAt("WhyNumber", editedObject->GetWhyAttributesNumber());
	SetIntValueAt("VarMax", editedObject->GetMaxAttributesNumber());

	// Mise a jour des choix en fonction de l'etat de l'interpreteur
	cast(UIIntElement*, GetFieldAt("WhyNumber"))
	    ->SetMaxValue(editedObject->GetInterpretationDictionary()->GetPredictiveAttributeNamesArray()->GetSize());

	SetStringValueAt("WhyType", editedObject->GetWhyType());
	SetStringValueAt("WhyClass", editedObject->GetWhyClass());
	SetBooleanValueAt("SortWhy", editedObject->GetSortWhyResults());
	SetBooleanValueAt("ExpertMode", editedObject->IsExpertMode());

	// Mise a jour de la liste deroulante en fonction de la liste des valeurs cibles figurant dans le dictionnaire
	// d'interpretation
	ALString sTargetValues;
	for (int i = 0; i < editedObject->GetInterpretationDictionary()->GetTargetValues().GetSize(); i++)
		sTargetValues +=
		    ALString(editedObject->GetInterpretationDictionary()->GetTargetValues().GetAt(i)) + "\n";

	GetFieldAt("WhyClass")
	    ->SetParameters(ALString(KIInterpretationSpec::PREDICTED_CLASS_LABEL) + "\n" +
			    ALString(KIInterpretationSpec::CLASS_OF_HIGHEST_GAIN_LABEL) + "\n" + sTargetValues +
			    ALString(KIInterpretationSpec::ALL_CLASSES_LABEL));
}
