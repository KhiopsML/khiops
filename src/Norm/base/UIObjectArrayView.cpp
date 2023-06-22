// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

UIObjectArrayView::UIObjectArrayView()
{
	array = NULL;
	itemView = NULL;
}

UIObjectArrayView::~UIObjectArrayView()
{
	if (itemView != NULL)
		delete itemView;
}

const ALString UIObjectArrayView::GetClassLabel() const
{
	return "UI object array view";
}

void UIObjectArrayView::SetObjectArray(ObjectArray* value)
{
	int nPreviousSelectedItemIndex;
	Object* object;
	int nItem;
	int i;

	array = value;

	// Memorisation de l'item selectionne en cours
	nPreviousSelectedItemIndex = GetSelectedItemIndex();

	// Reinitialisation de l'interface a partir du tableau
	if (array == NULL)
		RemoveAllItems();
	else
	{
		// Retaillage si necessaire du tableau graphique
		for (i = GetItemNumber(); i < array->GetSize(); i++)
			AddItem();
		assert(array->GetSize() <= GetItemNumber());
		for (i = GetItemNumber() - 1; i >= array->GetSize(); i--)
			RemoveItemAt(GetItemNumber() - 1);
		assert(array->GetSize() == GetItemNumber());

		// Mise a jour des elements du tableau
		for (nItem = 0; nItem < array->GetSize(); nItem++)
		{
			object = array->GetAt(nItem);
			if (object != NULL)
			{
				SetCurrentItemIndex(nItem);
				EventRefresh(object);
			}
		}
	}

	// On restitue si possible l'item selectionne
	if (nPreviousSelectedItemIndex < GetItemNumber())
		SetSelectedItemIndex(nPreviousSelectedItemIndex);
	else
		SetSelectedItemIndex(GetItemNumber() - 1);
}

ObjectArray* UIObjectArrayView::GetObjectArray()
{
	int nPreviousSelectedItemIndex;
	Object* object;
	int nItem;
	int i;

	// Memorisation de l'item selectionne en cours
	nPreviousSelectedItemIndex = GetSelectedItemIndex();

	// Acces aux donnees
	if (array != NULL)
	{
		// Retaillage si necessaire du tableau
		for (i = array->GetSize(); i < GetItemNumber(); i++)
			EventInsertItemAt(i);
		assert(GetItemNumber() <= array->GetSize());
		for (i = array->GetSize() - 1; i >= GetItemNumber(); i--)
			EventRemoveItemAt(i);
		assert(GetItemNumber() == array->GetSize());

		// Mise a jour des elements du tableau
		for (nItem = 0; nItem < array->GetSize(); nItem++)
		{
			object = array->GetAt(nItem);
			if (object != NULL)
			{
				SetCurrentItemIndex(nItem);
				EventUpdate(object);
			}
		}
	}

	// On restitue si possible l'item selectionne
	if (nPreviousSelectedItemIndex < GetItemNumber())
		SetSelectedItemIndex(nPreviousSelectedItemIndex);
	else
		SetSelectedItemIndex(GetItemNumber() - 1);

	return array;
}

void UIObjectArrayView::CopyCardHelpTexts()
{
	int nField;
	UIData* uiElement;
	int nAction;
	UIAction* uiAction;
	int nIndex;

	// Copie si presence d'une fiche
	if (GetItemView() != NULL)
	{
		// Copie de l'aide principale
		SetHelpText(GetItemView()->GetHelpText());

		// Copie de l'aide des champs
		for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
		{
			uiElement = cast(UIData*, oaUIDatas.GetAt(nField));

			// Recherche d'un champ equivalent dans la fiche
			nIndex = GetItemView()->GetFieldIndex(uiElement->GetIdentifier());
			if (nIndex != -1)
				uiElement->SetHelpText(
				    GetItemView()->GetFieldAt(uiElement->GetIdentifier())->GetHelpText());
		}

		// Copie de l'aide des actions
		for (nAction = 0; nAction < oaUIActions.GetSize(); nAction++)
		{
			uiAction = cast(UIAction*, oaUIActions.GetAt(nAction));

			// Recherche d'une action equivalente dans la fiche
			nIndex = GetItemView()->GetActionIndex(uiAction->GetIdentifier());
			if (nIndex != -1)
				uiAction->SetHelpText(
				    GetItemView()->GetActionAt(uiAction->GetIdentifier())->GetHelpText());
		}
	}
}

void UIObjectArrayView::SetItemView(UIObjectView* view)
{
	if (itemView != NULL)
		delete itemView;
	itemView = view;
}

UIObjectView* UIObjectArrayView::GetItemView()
{
	return itemView;
}

void UIObjectArrayView::ActionInspectItem()
{
	Object* object;
	int nIndex;
	int nCardFreshness;

	require(-1 <= nSelectedItemIndex and nSelectedItemIndex < nItemNumber);

	nIndex = nSelectedItemIndex;

	// Mise a jour seulement si index d'un item existant
	if (0 <= nIndex and nIndex < nItemNumber)
	{
		// Comportement par defaut si pas de fiche (ou probleme...)
		if (itemView == NULL or array == NULL or array->GetSize() <= nSelectedItemIndex or
		    array->GetAt(nSelectedItemIndex) == NULL)
			UIList::ActionInspectItem();
		else
		{
			object = array->GetAt(nSelectedItemIndex);

			// Ouverture de la fiche d'edition
			itemView->SetObject(object);
			nCardFreshness = itemView->GetFreshness();
			itemView->Open();

			// Mise a jour de l'objet
			itemView->GetObject();

			// Rafraichissement de l'item de la liste
			SetCurrentItemIndex(nIndex);
			EventRefresh(object);
			nFreshness += itemView->GetFreshness() - nCardFreshness;

			// Mise a jour dans les donnees (inutile, mais evenement standard)
			EventUpdate(object);
		}
	}
}

void UIObjectArrayView::EventRemoveItemAt(int nIndex)
{
	Object* object;

	if (array != NULL)
	{
		if (0 <= nIndex and nIndex < array->GetSize())
		{
			object = array->GetAt(nIndex);
			if (object != NULL)
				delete object;
			array->RemoveAt(nIndex);
		}
	}
}

void UIObjectArrayView::EventInsertItemAt(int nIndex)
{
	Object* object;
	int nRefItemIndex;

	if (array != NULL)
	{
		if (0 <= nIndex and nIndex <= array->GetSize())
		{
			object = EventNew();
			array->InsertAt(nIndex, object);

			// On rafraichit l'interface par les valeurs initiales de l'objet
			nRefItemIndex = GetCurrentItemIndex();
			SetCurrentItemIndex(nIndex);
			EventRefresh(object);
			SetCurrentItemIndex(nRefItemIndex);
		}
	}
}

void UIObjectArrayView::EventRefresh()
{
	SetObjectArray(array);
}

void UIObjectArrayView::EventUpdate()
{
	GetObjectArray();
}

////////////////////////////////////////////////////////////////////////
// Classe SampleObjectArrayView

SampleObjectArrayView::SampleObjectArrayView()
{
	SetIdentifier("SampleObject");
	SetLabel("Objet Sample");
	SetHelpText("Texte d'aide des objets Sample");
	AddStringField("String", "Chaine", "");
	AddIntField("Int", "Entier", 0);
	SetHelpText("Cette liste est utilisee pour tester la synchronisation modele-vue");
}

SampleObjectArrayView::~SampleObjectArrayView() {}

Object* SampleObjectArrayView::EventNew()
{
	SampleObject* soObject;

	soObject = new SampleObject;
	return soObject;
}

void SampleObjectArrayView::EventUpdate(Object* object)
{
	SampleObject* editedObject;

	require(object != NULL);

	editedObject = cast(SampleObject*, object);
	editedObject->SetString(GetStringValueAt("String"));
	editedObject->SetInt(GetIntValueAt("Int"));
}

void SampleObjectArrayView::EventRefresh(Object* object)
{
	SampleObject* editedObject;

	require(object != NULL);

	editedObject = cast(SampleObject*, object);
	SetStringValueAt("String", editedObject->GetString());
	SetIntValueAt("Int", editedObject->GetInt());
}