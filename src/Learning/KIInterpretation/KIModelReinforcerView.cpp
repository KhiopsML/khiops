// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelReinforcerView.h"

KIModelReinforcerView::KIModelReinforcerView()
{
	SetIdentifier("KIModelReinforcer");
	SetLabel("Reinforce model");
	AddStringField("ReinforcedTargetValue", "Target value to reinforce", "");

	// Parametrage des styles;
	GetFieldAt("ReinforcedTargetValue")->SetStyle("HelpedComboBox");

	// ## Custom constructor

	// Variables
	KIPredictorAttributeArrayView* predictorAttributeArrayView;
	UIList* targetValueHelpList;

	// Ajout d'une liste des attributs du predicteur
	predictorAttributeArrayView = new KIPredictorAttributeArrayView;
	predictorAttributeArrayView->SetEditable(false);
	AddListField("LeverAttributes", "Lever variables", predictorAttributeArrayView);

	// Le champ Used est editable
	predictorAttributeArrayView->GetFieldAt("Used")->SetEditable(true);

	// Creation d'une liste cachee des attributs de la classe en cours pour gerer les liste d'aide
	targetValueHelpList = new UIList;
	targetValueHelpList->AddStringField("Value", "Value", "");
	AddListField("TargetValues", "Target values", targetValueHelpList);
	targetValueHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour la valeur cible a renforcer
	GetFieldAt("ReinforcedTargetValue")->SetParameters("TargetValues:Value");

	// Ajout de l'action de construction d'un dictionnaire d'interpretation
	AddAction("BuildReinforcementClass", "Build interpretation dictionary...",
		  (ActionMethod)(&KIModelReinforcerView::BuildReinforcementClass));
	GetActionAt("BuildReinforcementClass")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ReinforcedTargetValue")
	    ->SetHelpText("Target value for which one try to increase the probability of occurrence.");
	GetActionAt("BuildReinforcementClass")
	    ->SetHelpText("Build a reinforcement dictionary that computes the reinforcement variables for the "
			  "specified target value");

	// ##
}

KIModelReinforcerView::~KIModelReinforcerView()
{
	// ## Custom destructor

	// ##
}

KIModelReinforcer* KIModelReinforcerView::GetKIModelReinforcer()
{
	require(objValue != NULL);
	return cast(KIModelReinforcer*, objValue);
}

void KIModelReinforcerView::EventUpdate(Object* object)
{
	KIModelReinforcer* editedObject;

	require(object != NULL);

	KIModelServiceView::EventUpdate(object);
	editedObject = cast(KIModelReinforcer*, object);
	editedObject->SetReinforcedTargetValue(GetStringValueAt("ReinforcedTargetValue"));

	// ## Custom update

	// ##
}

void KIModelReinforcerView::EventRefresh(Object* object)
{
	KIModelReinforcer* editedObject;

	require(object != NULL);

	KIModelServiceView::EventRefresh(object);
	editedObject = cast(KIModelReinforcer*, object);
	SetStringValueAt("ReinforcedTargetValue", editedObject->GetReinforcedTargetValue());

	// ## Custom refresh

	// Variables locales
	int nValue;
	boolean bValueFound;
	ALString sNewTargetValue;

	// Rafraichissement de la liste d'aide sur les valeur cibles
	RefreshHelpLists();

	// Choix de la premiere valeur cible si la valeur cible actuelle n'est pas dans la liste
	if (editedObject->GetClassBuilder()->IsPredictorImported())
	{
		// Recherche de la valeur
		bValueFound = false;
		for (nValue = 0; nValue < editedObject->GetClassBuilder()->GetTargetValues()->GetSize(); nValue++)
		{
			if (editedObject->GetReinforcedTargetValue() ==
			    editedObject->GetClassBuilder()->GetTargetValues()->GetAt(nValue))
			{
				bValueFound = true;
				break;
			}
		}

		// Changement de valeur si valeur incorrecte
		if (not bValueFound)
			sNewTargetValue = editedObject->GetClassBuilder()->GetTargetValues()->GetAt(0);
		// Sinon, on garde la meme
		else
			sNewTargetValue = editedObject->GetReinforcedTargetValue();
	}
	else
		sNewTargetValue = "";

	// Mise a jour de la valeur cible a renforcer
	editedObject->SetReinforcedTargetValue(sNewTargetValue);
	SetStringValueAt("ReinforcedTargetValue", editedObject->GetReinforcedTargetValue());

	// ##
}

const ALString KIModelReinforcerView::GetClassLabel() const
{
	return "Reinforce model";
}

// ## Method implementation

void KIModelReinforcerView::BuildReinforcementClass()
{
	AddSimpleMessage("Not yet implemented");
}

void KIModelReinforcerView::SetObject(Object* object)
{
	KIModelReinforcer* editedObject;

	require(object != NULL);

	// Recherche de la classe editee
	editedObject = cast(KIModelReinforcer*, object);

	// Parametrage des variables du predicteur
	cast(KIPredictorAttributeArrayView*, GetFieldAt("LeverAttributes"))
	    ->SetObjectArray(editedObject->GeLeverAttributes());

	// Appel de la methode ancetre
	UIObjectView::SetObject(object);
}

void KIModelReinforcerView::RefreshPredictorSpec(KIModelService* modelService)
{
	// Appele de la methode ancetre
	KIModelServiceView::RefreshPredictorSpec(modelService);

	// Raffraichissement des variables leviers
	cast(KIModelReinforcer*, modelService)->UpdateLeverAttributes();
}

void KIModelReinforcerView::RefreshHelpLists()
{
	UIList* targetValueHelpList;
	int nValue;

	// Acces a la liste d'aide
	targetValueHelpList = cast(UIList*, GetFieldAt("TargetValues"));

	// On commence par la vider
	targetValueHelpList->RemoveAllItems();

	// Ajout d'autant de ligne que de valeurs cible
	if (GetKIModelReinforcer()->GetClassBuilder()->IsPredictorImported())
	{
		for (nValue = 0; nValue < GetKIModelReinforcer()->GetClassBuilder()->GetTargetValues()->GetSize();
		     nValue++)
		{
			targetValueHelpList->AddItem();
			targetValueHelpList->SetStringValueAt(
			    "Value",
			    GetKIModelReinforcer()->GetClassBuilder()->GetTargetValues()->GetAt(nValue).GetValue());
		}
	}
}

// ##
