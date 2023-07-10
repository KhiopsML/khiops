// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDDataPreparationAttributeCreationTaskView.h"

KDDataPreparationAttributeCreationTaskView::KDDataPreparationAttributeCreationTaskView()
{
	sName = "Variable creation";
}

KDDataPreparationAttributeCreationTaskView::~KDDataPreparationAttributeCreationTaskView() {}

KDDataPreparationAttributeCreationTaskView* KDDataPreparationAttributeCreationTaskView::Create() const
{
	return new KDDataPreparationAttributeCreationTaskView;
}

const ALString& KDDataPreparationAttributeCreationTaskView::GetName() const
{
	return sName;
}

const ALString KDDataPreparationAttributeCreationTaskView::GetClassLabel() const
{
	return GetName();
}

void KDDataPreparationAttributeCreationTaskView::EventUpdate(Object* object) {}

void KDDataPreparationAttributeCreationTaskView::EventRefresh(Object* object) {}

void KDDataPreparationAttributeCreationTaskView::SetObject(Object* object)
{
	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KDDataPreparationAttributeCreationTask* KDDataPreparationAttributeCreationTaskView::GetAttributeCreationTask()
{
	return cast(KDDataPreparationAttributeCreationTask*, objValue);
}

///////////////////////////////////////////////////////////////////////////

void KDDataPreparationAttributeCreationTaskView::SetGlobalCreationTaskView(
    KDDataPreparationAttributeCreationTaskView* attributeCreationTaskView)
{
	if (globalAttributeCreationTaskView != NULL)
		delete globalAttributeCreationTaskView;
	globalAttributeCreationTaskView = attributeCreationTaskView;
}

KDDataPreparationAttributeCreationTaskView* KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView()
{
	return globalAttributeCreationTaskView;
}

KDDataPreparationAttributeCreationTaskView*
    KDDataPreparationAttributeCreationTaskView::globalAttributeCreationTaskView = NULL;

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDPBivariateCrossProductsCreationTaskView

KDDPBivariateCrossProductsCreationTaskView::KDDPBivariateCrossProductsCreationTaskView()
{
	sName = "Bivariate cross-product variable creation";
	SetIdentifier("KDDPBivariateCrossProductsCreationTask");
	SetLabel(sName);
	AddStringField("CreatedAttributePrefix", "Create variable prefix", "");
}

KDDPBivariateCrossProductsCreationTaskView::~KDDPBivariateCrossProductsCreationTaskView() {}

void KDDPBivariateCrossProductsCreationTaskView::EventUpdate(Object* object)
{
	KDDPBivariateCrossProductsCreationTask* editedObject;

	require(object != NULL);

	editedObject = cast(KDDPBivariateCrossProductsCreationTask*, object);
	editedObject->SetCreatedAttributePrefix(GetStringValueAt("CreatedAttributePrefix"));
}

void KDDPBivariateCrossProductsCreationTaskView::EventRefresh(Object* object)
{
	KDDPBivariateCrossProductsCreationTask* editedObject;

	require(object != NULL);

	editedObject = cast(KDDPBivariateCrossProductsCreationTask*, object);
	SetStringValueAt("CreatedAttributePrefix", editedObject->GetCreatedAttributePrefix());
}
