// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeCreationTaskSequentialView.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTaskSequentialView

DTDecisionTreeCreationTaskSequentialView::DTDecisionTreeCreationTaskSequentialView()
{
	sName = "Decision Tree variable creation";

	SetIdentifier("DTDecisionTreeCreationTaskSequential");
	SetLabel(sName);
	AddStringField("CreatedAttributePrefix", "Create variable prefix", "");

	AddCardField("TreeSpecification", "Tree Specification", new DTDecisionTreeParameterView);
	AddCardField("ForestSpecification", "Forest Specification", new DTForestParameterView);
	// Passage en ergonomie onglet
	SetStyle("TabbedPanes");
}

DTDecisionTreeCreationTaskSequentialView::~DTDecisionTreeCreationTaskSequentialView() {}

void DTDecisionTreeCreationTaskSequentialView::EventUpdate(Object* object)
{
	require(object != NULL);

	DTDecisionTreeCreationTaskSequential* editedObject;
	editedObject = cast(DTDecisionTreeCreationTaskSequential*, object);
	editedObject->SetCreatedAttributePrefix(GetStringValueAt("CreatedAttributePrefix"));
}

void DTDecisionTreeCreationTaskSequentialView::EventRefresh(Object* object)
{
	require(object != NULL);

	DTDecisionTreeCreationTaskSequential* editedObject;
	editedObject = cast(DTDecisionTreeCreationTaskSequential*, object);

	SetStringValueAt("CreatedAttributePrefix", editedObject->GetCreatedAttributePrefix());
}

void DTDecisionTreeCreationTaskSequentialView::SetObject(Object* object)
{
	require(object != NULL);

	// Acces a l'objet edite
	DTDecisionTreeCreationTaskSequential* dttask;
	dttask = cast(DTDecisionTreeCreationTaskSequential*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(DTDecisionTreeParameterView*, GetFieldAt("TreeSpecification"))
	    ->SetObject(dttask->GetForestParameter()->GetDecisionTreeParameter());
	cast(DTForestParameterView*, GetFieldAt("ForestSpecification"))->SetObject(dttask->GetForestParameter());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
