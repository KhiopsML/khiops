// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSortAttributeNameArrayView.h"

KWSortAttributeNameArrayView::KWSortAttributeNameArrayView()
{
	ALString sFirstActionId;

	// Ajout de l'action de choix des cle variables de cle par defaut
	AddAction("SelectDefaultKeyAttributes", "Default key variables",
		  (ActionMethod)(&KWSortAttributeNameArrayView::ActionSelectDefaultKeyAttributes));
	GetActionAt("SelectDefaultKeyAttributes")->SetStyle("Button");

	// Deplacement de l'action en tete, avant ceux de la classe ancetre
	sFirstActionId = GetActionAtIndex(0)->GetIdentifier();
	MoveActionBefore("SelectDefaultKeyAttributes", sFirstActionId);

	// Info-bulles
	GetActionAt("SelectDefaultKeyAttributes")
	    ->SetHelpText("Select the default key variables."
			  "\n The sort variables are the key variables retrieved from the sort dictionary.");
}

KWSortAttributeNameArrayView::~KWSortAttributeNameArrayView() {}

void KWSortAttributeNameArrayView::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

const ALString& KWSortAttributeNameArrayView::GetClassName() const
{
	return sClassName;
}

void KWSortAttributeNameArrayView::ActionSelectDefaultKeyAttributes()
{
	KWClass* kwcClass;
	KWAttributeName* attributeName;
	ObjectArray* oaKeyVariableNames;
	int i;

	require(GetObjectArray() != NULL);

	// On vide le tableau en prealable
	oaKeyVariableNames = GetObjectArray();
	oaKeyVariableNames->DeleteAll();

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);

	// Parametrage des champs de la cle
	if (kwcClass != NULL)
	{
		for (i = 0; i < kwcClass->GetKeyAttributeNumber(); i++)
		{
			attributeName = new KWAttributeName;
			attributeName->SetName(kwcClass->GetKeyAttributeNameAt(i));
			oaKeyVariableNames->Add(attributeName);
		}
	}

	// Rafraichissement de l'interface, en positionnant l'index d'insertion sur le dernier element de la liste
	SetObjectArray(oaKeyVariableNames);
	SetSelectedItemIndex(oaKeyVariableNames->GetSize() - 1);
}
