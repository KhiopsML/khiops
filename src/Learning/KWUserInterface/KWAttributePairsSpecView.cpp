// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributePairsSpecView.h"

KWAttributePairsSpecView::KWAttributePairsSpecView()
{
	SetIdentifier("KWAttributePairsSpec");
	SetLabel("Variable pairs parameters");
	AddIntField("MaxAttributePairNumber", "Max number of variable pairs", 0);
	AddBooleanField("AllAttributePairs", "All pairs", false);

	// Parametrage des styles;
	GetFieldAt("MaxAttributePairNumber")->SetStyle("Spinner");
	GetFieldAt("AllAttributePairs")->SetStyle("CheckBox");

	// ## Custom constructor

	KWAttributePairsSpecFileView* attributePairsSpecFileView;
	KWAttributePairNameArrayView* specificAttributePairs;
	UIList* attributeNameHelpList;

	// Parametrage des nombre min et max
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))
	    ->SetMaxValue(KWAttributePairsSpec::nLargestMaxAttributePairNumber);

	// Nombre de pair specifiques en read-only, uniquement a l'interface
	AddIntField("SpecificPairsNumber", "Specific pairs number", 0);
	GetFieldAt("SpecificPairsNumber")->SetEditable(false);

	// Ajout d'une fiche pour ls actions d'import/export
	attributePairsSpecFileView = new KWAttributePairsSpecFileView;
	AddCardField("File", "Variable pairs file", attributePairsSpecFileView);

	// Ajout d'une liste des paires d'attributs specifiques
	specificAttributePairs = new KWAttributePairNameArrayView;
	AddListField("SpecificAttributePairs", "Specific variable pairs", specificAttributePairs);
	specificAttributePairs->SetLineNumber(7);
	specificAttributePairs->SetMaxAttributePairNumber(KWAttributePairsSpec::nLargestMaxAttributePairNumber);

	// La liste des paires de variables est affichee avant la variable de comptage
	MoveFieldBefore("SpecificAttributePairs", "SpecificPairsNumber");

	// Creation d'une liste cachee des attributs de la classe en cours pour gerer les liste d'aide
	attributeNameHelpList = new UIList;
	attributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", attributeNameHelpList);
	attributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom des des attributs de la paire
	specificAttributePairs->GetFieldAt("FirstName")->SetStyle("HelpedComboBox");
	specificAttributePairs->GetFieldAt("FirstName")->SetParameters("Attributes:Name");
	specificAttributePairs->GetFieldAt("SecondName")->SetStyle("HelpedComboBox");
	specificAttributePairs->GetFieldAt("SecondName")->SetParameters("Attributes:Name");

	// Personnalisation des actions d'insertion et de supression
	specificAttributePairs->GetActionAt("InsertItemBefore")->SetVisible(false);
	specificAttributePairs->GetActionAt("InsertItemAfter")->SetStyle("Button");
	specificAttributePairs->GetActionAt("RemoveItem")->SetStyle("Button");
	specificAttributePairs->GetActionAt("InsertItemAfter")->SetLabel("Insert pair");
	specificAttributePairs->GetActionAt("RemoveItem")->SetLabel("Remove pair");

	// Info-bulles
	GetFieldAt("MaxAttributePairNumber")
	    ->SetHelpText("Max number of variable pairs to analyze during data preparation."
			  "\n If the number of pairs specified is greater than this maximum value, the pairs are "
			  "chosen first for the specific pairs,"
			  "\n then for the pairs involving the variables with the highest level in the supervised case "
			  "and by alphabetic order otherwise.");
	GetFieldAt("AllAttributePairs")->SetHelpText("Analyze all possible variable pairs.");
	GetFieldAt("SpecificPairsNumber")->SetHelpText("Number of specific variable pairs.");
	specificAttributePairs->GetActionAt("InsertItemAfter")
	    ->SetHelpText("Add a variable pair."
			  "\n A variable pair can be specified with a single variable to indicate"
			  "\n that all pairs involving that variable should be analyzed");
	specificAttributePairs->GetActionAt("RemoveItem")->SetHelpText("Remove a variable pair.");

	// Short cuts
	specificAttributePairs->GetActionAt("InsertItemAfter")->SetShortCut('I');
	specificAttributePairs->GetActionAt("RemoveItem")->SetShortCut('R');

	// ##
}

KWAttributePairsSpecView::~KWAttributePairsSpecView()
{
	// ## Custom destructor

	// ##
}

KWAttributePairsSpec* KWAttributePairsSpecView::GetKWAttributePairsSpec()
{
	require(objValue != NULL);
	return cast(KWAttributePairsSpec*, objValue);
}

void KWAttributePairsSpecView::EventUpdate(Object* object)
{
	KWAttributePairsSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairsSpec*, object);
	editedObject->SetMaxAttributePairNumber(GetIntValueAt("MaxAttributePairNumber"));
	editedObject->SetAllAttributePairs(GetBooleanValueAt("AllAttributePairs"));

	// ## Custom update

	// ##
}

void KWAttributePairsSpecView::EventRefresh(Object* object)
{
	KWAttributePairsSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairsSpec*, object);
	SetIntValueAt("MaxAttributePairNumber", editedObject->GetMaxAttributePairNumber());
	SetBooleanValueAt("AllAttributePairs", editedObject->GetAllAttributePairs());

	// ## Custom refresh

	// Mise a jour du champ contenant la taille de la liste des paires
	SetIntValueAt("SpecificPairsNumber", editedObject->GetSpecificAttributePairs()->GetSize());

	// Rafraichissement des listes d'aide
	RefreshHelpLists();

	// ##
}

const ALString KWAttributePairsSpecView::GetClassLabel() const
{
	return "Variable pairs parameters";
}

// ## Method implementation

void KWAttributePairsSpecView::SetObject(Object* object)
{
	KWAttributePairsSpec* attributePairsSpec;

	require(object != NULL);

	// Appel de la methode ancetre
	UIObjectView::SetObject(object);

	// Acces a l'objet edite
	attributePairsSpec = cast(KWAttributePairsSpec*, object);

	// Parametrage des sous fiches
	cast(KWAttributePairsSpecFileView*, GetFieldAt("File"))->SetObject(attributePairsSpec);
	cast(KWAttributePairNameArrayView*, GetFieldAt("SpecificAttributePairs"))
	    ->SetObjectArray(attributePairsSpec->GetSpecificAttributePairs());
}

void KWAttributePairsSpecView::RefreshHelpLists()
{
	// Rafraichissement de la liste d'aide pour les attributs obligatoires dans une  paire
	classAttributeHelpList.FillAttributeNames(cast(KWAttributePairsSpec*, objValue)->GetClassName(), true, true,
						  false, true, cast(UIList*, GetFieldAt("Attributes")), "Name");
}

// ##
