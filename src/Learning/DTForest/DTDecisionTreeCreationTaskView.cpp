// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeCreationTaskView.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTaskView

DTDecisionTreeCreationTaskView::DTDecisionTreeCreationTaskView()
{
	sName = "Decision Tree variable creation";

	SetIdentifier("DTDecisionTreeCreationTask");
	SetLabel(sName);
	AddStringField("CreatedAttributePrefix", "Create variable prefix", "");

	AddCardField("TreeSpecification", "Tree Specification", new DTDecisionTreeParameterView);
	AddCardField("ForestSpecification", "Forest Specification", new DTForestParameterView);
	// Passage en ergonomie onglet
	SetStyle("TabbedPanes");
}

DTDecisionTreeCreationTaskView::~DTDecisionTreeCreationTaskView() {}

void DTDecisionTreeCreationTaskView::EventUpdate(Object* object)
{
	require(object != NULL);

	DTDecisionTreeCreationTask* editedObject;
	editedObject = cast(DTDecisionTreeCreationTask*, object);

	editedObject->SetCreatedAttributePrefix(GetStringValueAt("CreatedAttributePrefix"));
}

void DTDecisionTreeCreationTaskView::EventRefresh(Object* object)
{
	require(object != NULL);

	DTDecisionTreeCreationTask* editedObject;
	editedObject = cast(DTDecisionTreeCreationTask*, object);

	SetStringValueAt("CreatedAttributePrefix", editedObject->GetCreatedAttributePrefix());
}

void DTDecisionTreeCreationTaskView::SetObject(Object* object)
{
	require(object != NULL);

	// Acces a l'objet edite

	DTDecisionTreeCreationTask* dttask;
	dttask = cast(DTDecisionTreeCreationTask*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(DTDecisionTreeParameterView*, GetFieldAt("TreeSpecification"))
	    ->SetObject(dttask->GetForestParameter()->GetDecisionTreeParameter());
	cast(DTForestParameterView*, GetFieldAt("ForestSpecification"))->SetObject(dttask->GetForestParameter());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}