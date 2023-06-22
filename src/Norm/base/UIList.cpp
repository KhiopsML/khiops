// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

UIList::UIList()
{
	itemCard = NULL;
	ergonomy = CellEditable;
	nItemNumber = 0;
	nCurrentItemIndex = -1;
	nSelectedItemIndex = -1;
	AddAction("InsertItemBefore", "Insert before", (ActionMethod)(&UIList::ActionInsertItemBefore));
	AddAction("InsertItemAfter", "Insert after", (ActionMethod)(&UIList::ActionInsertItemAfter));
	AddAction("InspectItem", "Inspect", (ActionMethod)(&UIList::ActionInspectItem));
	AddAction("RemoveItem", "Remove", (ActionMethod)(&UIList::ActionRemoveItem));
	// L'ergonomie par defaut ne permet pas l'inspection
	GetActionAt("InspectItem")->SetVisible(false);
}

UIList::~UIList()
{
	if (itemCard != NULL)
		delete itemCard;
}

void UIList::SetErgonomy(int nValue)
{
	require(nValue == CellEditable or nValue == Inspectable);
	ergonomy = nValue;

	if (ergonomy == Inspectable)
	{
		SetEditable(false);
		GetActionAt("InsertItemBefore")->SetVisible(true);
		GetActionAt("InsertItemAfter")->SetVisible(true);
		GetActionAt("InspectItem")->SetVisible(true);
		GetActionAt("RemoveItem")->SetVisible(true);
	}

	else
	{
		SetEditable(true);
		GetActionAt("InsertItemBefore")->SetVisible(true);
		GetActionAt("InsertItemAfter")->SetVisible(true);
		GetActionAt("InspectItem")->SetVisible(false);
		GetActionAt("RemoveItem")->SetVisible(true);
	}
}

int UIList::GetErgonomy() const
{
	return ergonomy;
}

void UIList::SetLineNumber(int nValue)
{
	require(nValue >= 0 and nValue <= 50);
	WriteParameters(nValue, GetLastColumnExtraWidth());
}

int UIList::GetLineNumber() const
{
	int nLineNumber;
	int nLastColumnExtraWidth;
	ReadParameters(nLineNumber, nLastColumnExtraWidth);
	return nLineNumber;
}

void UIList::SetLastColumnExtraWidth(int nValue)
{
	require(nValue >= 0 and nValue <= 50);
	WriteParameters(GetLineNumber(), nValue);
}

int UIList::GetLastColumnExtraWidth() const
{
	int nLineNumber;
	int nLastColumnExtraWidth;
	ReadParameters(nLineNumber, nLastColumnExtraWidth);
	return nLastColumnExtraWidth;
}

const ALString UIList::GetClassLabel() const
{
	return "UI List";
}

void UIList::SetEditable(boolean bValue)
{
	int nField;
	UIData* uiElement;

	// Propagation aux sous-composants
	bEditable = bValue;
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		uiElement->SetEditable(bValue);
	}

	// Ajout sur les actions d'edition par controle de leur visibilite
	GetActionAt("InsertItemBefore")->SetVisible(bValue);
	GetActionAt("InsertItemAfter")->SetVisible(bValue);
	GetActionAt("InspectItem")->SetVisible(bValue);
	GetActionAt("RemoveItem")->SetVisible(bValue);
}

void UIList::SetKeyFieldId(const ALString& sFieldId)
{
	require(sFieldId == "" or GetFieldIndex(sFieldId) != -1);
	require(sFieldId == "" or cast(UIData*, oaUIDatas.GetAt(GetFieldIndex(sFieldId)))->GetDataType() == String);
	sKeyFieldId = sFieldId;
}

const ALString& UIList::GetKeyFieldId() const
{
	ensure(sKeyFieldId == "" or GetFieldIndex(sKeyFieldId) != -1);
	ensure(sKeyFieldId == "" or
	       cast(UIData*, oaUIDatas.GetAt(GetFieldIndex(sKeyFieldId)))->GetDataType() == String);
	return sKeyFieldId;
}

int UIList::GetItemNumber()
{
	return nItemNumber;
}

void UIList::AddItem()
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;

	// Parcours de tous les champs
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		assert(uiElement->IsListable());
		valueContainer = oaUnitValues.GetAt(nField);
		uiElement->ValueContainerAddItem(valueContainer);
	}
	nItemNumber++;

	// Mise a jour des index
	nFreshness++;
	nSelectedItemIndex = nItemNumber - 1;
	nCurrentItemIndex = nItemNumber - 1;
}

void UIList::InsertItemAt(int nIndex)
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;

	require(0 <= nIndex and nIndex <= nItemNumber);

	// Parcours de tous les champs
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		assert(uiElement->IsListable());
		valueContainer = oaUnitValues.GetAt(nField);
		uiElement->ValueContainerInsertItemAt(valueContainer, nIndex);
	}
	nItemNumber++;

	// Mise a jour des index
	nFreshness++;
	nSelectedItemIndex = nIndex;
	nCurrentItemIndex = nIndex;
}

void UIList::RemoveItemAt(int nIndex)
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;

	require(0 <= nIndex and nIndex < nItemNumber);

	// Parcours de tous les champs
	nItemNumber--;
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		assert(uiElement->IsListable());
		valueContainer = oaUnitValues.GetAt(nField);
		uiElement->ValueContainerRemoveItemAt(valueContainer, nIndex);
	}

	// Mise a jour des index
	nFreshness++;
	// S'il n'y a plus d'items ou s'il l'item supprime etait le dernier de la liste
	if (nItemNumber == 0 || nSelectedItemIndex == nItemNumber)
	{
		nSelectedItemIndex--;
		nCurrentItemIndex--;
	}
}

void UIList::RemoveAllItems()
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;
	int nIndex;

	// Parcours de tous les champs
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		assert(uiElement->IsListable());
		valueContainer = oaUnitValues.GetAt(nField);

		// On les detruit du dernier au premier, car cela optimise
		// le retaillage des tableaux sous-jacents
		for (nIndex = nItemNumber - 1; nIndex >= 0; nIndex--)
			uiElement->ValueContainerRemoveItemAt(valueContainer, nIndex);
	}
	nItemNumber = 0;

	// Mise a jour des index
	nFreshness++;
	nSelectedItemIndex = -1;
	nCurrentItemIndex = -1;
}

void UIList::SetCurrentItemIndex(int nIndex)
{
	require(-1 <= nIndex and nIndex < nItemNumber);

	// Garde-fou, utile quand on rejout un scenario compose a la main (donc sujet a erreurs)
	if (nIndex < -1)
		nIndex = -1;
	if (nIndex > nItemNumber)
		nIndex = nItemNumber - 1;

	nCurrentItemIndex = nIndex;
}

int UIList::GetCurrentItemIndex() const
{
	require(0 < nItemNumber);

	return nCurrentItemIndex;
}

void UIList::SetSelectedItemIndex(int nIndex)
{
	require(-1 <= nIndex and nIndex < nItemNumber);

	// Garde-fou, utile quand on rejout un scenario compose a la main (donc sujet a erreurs)
	if (nIndex < -1)
		nIndex = -1;
	if (nIndex > nItemNumber)
		nIndex = nItemNumber;

	// Memorisation de l'index
	nSelectedItemIndex = nIndex;
	nCurrentItemIndex = nIndex;
}

int UIList::GetSelectedItemIndex() const
{
	return nSelectedItemIndex;
}

void UIList::SetSelectedItemKey(const ALString& sKey)
{
	int nKeyFieldIndex;
	StringVector* values;
	ALString sValue;
	int nIndex;
	int nFoundIndex;

	require(GetKeyFieldId() != "");

	// Recherche de l'index du champ cle
	nKeyFieldIndex = GetFieldIndex(GetKeyFieldId());
	assert(nKeyFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nKeyFieldIndex))->GetDataType() == String);

	// Garde-fou, utile quand on rejout un scenario compose a la main (donc sujet a erreurs)
	nFoundIndex = -1;
	if (nKeyFieldIndex >= 0 and cast(UIData*, oaUIDatas.GetAt(nKeyFieldIndex))->GetDataType() == String)
	{
		// Recherche de l'index de la cle en parcourant toutes les valeurs du champs
		values = cast(StringVector*, oaUnitValues.GetAt(nKeyFieldIndex));
		for (nIndex = 0; nIndex < values->GetSize(); nIndex++)
		{
			sValue = values->GetAt(nIndex);
			if (sValue == sKey)
			{
				nFoundIndex = nIndex;
				break;
			}
		}
	}

	// Memorisation de l'index
	SetSelectedItemIndex(nFoundIndex);
}

const ALString UIList::GetSelectedItemKey() const
{
	int nKeyFieldIndex;
	StringVector* values;
	ALString sValue;

	// Recherche de l'index du champ cle
	nKeyFieldIndex = GetFieldIndex(GetKeyFieldId());
	assert(nKeyFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nKeyFieldIndex))->GetDataType() == String);

	// On retourne la valeur du champ selectionne
	if (0 <= nSelectedItemIndex and nSelectedItemIndex < nItemNumber)
	{
		values = cast(StringVector*, oaUnitValues.GetAt(nKeyFieldIndex));
		sValue = values->GetAt(nSelectedItemIndex);
		return sValue;
	}
	else
		return "";
}

void UIList::EventInsertItemAt(int nIndex)
{
	require(0 <= nIndex and nIndex <= nItemNumber);
}

void UIList::EventRemoveItemAt(int nIndex)
{
	require(0 <= nIndex and nIndex < nItemNumber);
}

void UIList::EventUpdateItemAt(int nIndex)
{
	require(0 <= nIndex and nIndex < nItemNumber);
}

void UIList::ActionInsertItemBefore()
{
	int nIndex;

	require(nSelectedItemIndex >= -1 and nSelectedItemIndex < nItemNumber);

	// Si aucun item n'est selectionne, on se base sur le premier possible
	if (nSelectedItemIndex == -1)
		nIndex = 0;
	else
		nIndex = nSelectedItemIndex;

	// Insertion d'un item
	InsertItemAt(nIndex);

	// Insertion dans les donnees
	EventInsertItemAt(nIndex);

	// Mise a jour dans les donnees
	EventUpdateItemAt(nIndex);

	// Si l'action InspectItem est disponible
	if (GetActionAt("InspectItem")->GetVisible())
		ActionInspectItem();
	else
		EventUpdate();
}

void UIList::ActionInsertItemAfter()
{
	int nIndex;

	require(nSelectedItemIndex >= -1 and nSelectedItemIndex < nItemNumber);

	// Si aucun item n'est selectionne, on se base sur le dernier possible
	if (nSelectedItemIndex == -1)
		nIndex = nItemNumber;
	else
		nIndex = nSelectedItemIndex + 1;

	// Insertion d'un item
	InsertItemAt(nIndex);

	// Insertion dans les donnees
	EventInsertItemAt(nIndex);

	// Mise a jour dans les donnees
	EventUpdateItemAt(nIndex);

	// Si l'action InspectItem est disponible
	if (GetActionAt("InspectItem")->GetVisible())
		ActionInspectItem();
	else
		EventUpdate();
}

void UIList::ActionInspectItem()
{
	require(-1 <= nSelectedItemIndex and nSelectedItemIndex < nItemNumber);

	// Mise a jour seulement si index d'un item existant
	if (0 <= nSelectedItemIndex and nSelectedItemIndex < nItemNumber)
	{
		int nIndex;
		int nCardField;
		int nListField;
		ALString sFieldId;
		UIElement* element;
		UIElement* cardElement;
		Object* valueContainer;
		Object* cardValueContainer;
		int nCardFreshness;

		nIndex = nSelectedItemIndex;

		// Si une vue a ete specifiee
		if (itemCard != NULL)
		{
			// Initialisation de cette vue a partir des champs courants
			for (nCardField = 0; nCardField < itemCard->oaUIDatas.GetSize(); nCardField++)
			{
				// Acces aux donnees de la fiche
				cardElement = cast(UIElement*, itemCard->oaUIDatas.GetAt(nCardField));
				cardValueContainer = itemCard->oaUnitValues.GetAt(nCardField);

				// Recherche du champ par l'identifiant
				for (nListField = 0; nListField < oaUIDatas.GetSize(); nListField++)
				{
					element = cast(UIElement*, oaUIDatas.GetAt(nListField));
					// Comparaison sur l'identifiant
					if (element->GetIdentifier() == cardElement->GetIdentifier())
					{
						valueContainer = oaUnitValues.GetAt(nListField);
						// Transfert des donnees
						cardElement->SetFieldStringValue(
						    cardValueContainer, 0,
						    element->GetFieldStringValue(valueContainer, nIndex));
					}
				}
			}

			nCardFreshness = itemCard->GetFreshness();
			itemCard->Open();

			// Mise a jour des donnees logiques a partir de la vue
			for (nCardField = 0; nCardField < itemCard->oaUIDatas.GetSize(); nCardField++)
			{
				// Acces aux donnees de la fiche
				cardElement = cast(UIElement*, itemCard->oaUIDatas.GetAt(nCardField));
				cardValueContainer = itemCard->oaUnitValues.GetAt(nCardField);

				// Acces aux donnees de la liste

				// Recherche du champ par l'identifiant
				for (nListField = 0; nListField < oaUIDatas.GetSize(); nListField++)
				{
					element = cast(UIElement*, oaUIDatas.GetAt(nListField));
					// Comparaison sur l'identifiant
					if (element->GetIdentifier() == cardElement->GetIdentifier())
					{
						valueContainer = oaUnitValues.GetAt(nListField);
						// Transfert des donnees
						element->SetFieldStringValue(
						    valueContainer, nIndex,
						    cardElement->GetFieldStringValue(cardValueContainer, 0));
					}
				}
			}
		}
		// Si aucune vue n'a ete specifiee
		else
		{
			// Creation de la vue par defaut
			CreateItemCard();

			// Initialisation de cette vue a partir des champs courants
			for (nListField = 0; nListField < oaUIDatas.GetSize(); nListField++)
			{
				// Acces aux donnees de la liste
				element = cast(UIElement*, oaUIDatas.GetAt(nListField));
				valueContainer = oaUnitValues.GetAt(nListField);

				// Acces aux donnees de la fiche
				cardElement = cast(UIElement*, itemCard->oaUIDatas.GetAt(nListField));
				cardValueContainer = itemCard->oaUnitValues.GetAt(nListField);

				// Transfert des donnees
				cardElement->SetFieldStringValue(cardValueContainer, 0,
								 element->GetFieldStringValue(valueContainer, nIndex));
			}

			// Ouverture de la fiche
			nCardFreshness = itemCard->GetFreshness();
			itemCard->Open();

			// Mise a jour des valeurs dans l'item de la liste
			for (nListField = 0; nListField < oaUIDatas.GetSize(); nListField++)
			{
				// Acces aux donnees de la liste
				element = cast(UIElement*, oaUIDatas.GetAt(nListField));
				valueContainer = oaUnitValues.GetAt(nListField);

				// Acces aux donnees de la fiche
				cardElement = cast(UIElement*, itemCard->oaUIDatas.GetAt(nListField));
				cardValueContainer = itemCard->oaUnitValues.GetAt(nListField);

				// Transfert des donnees
				element->SetFieldStringValue(valueContainer, nIndex,
							     cardElement->GetFieldStringValue(cardValueContainer, 0));
			}
		}
		nFreshness += itemCard->GetFreshness() - nCardFreshness;

		// Mise a jour dans les donnees
		EventUpdate();
		EventUpdateItemAt(nIndex);
	}
}

void UIList::CreateItemCard()
{
	int nListField;
	UIElement* element;

	itemCard = new UICard();
	itemCard->SetLabel("InspectItem");

	// Creation d'une vue reprenant les champs de la liste
	for (nListField = 0; nListField < oaUIDatas.GetSize(); nListField++)
	{
		element = cast(UIElement*, oaUIDatas.GetAt(nListField));
		itemCard->AddField(element->CloneAsElement());
	}
}

void UIList::ActionRemoveItem()
{
	require(nSelectedItemIndex >= -1 and nSelectedItemIndex < nItemNumber);

	// Si un item est selectionne dans une liste non vide
	if (nItemNumber > 0 and nSelectedItemIndex > -1)
	{
		// Insertion dans les donnees
		EventRemoveItemAt(nSelectedItemIndex);

		// Insertion a l'interface
		RemoveItemAt(nSelectedItemIndex);
	}
}

int UIList::GetDataType() const
{
	return List;
}

void UIList::TextualSelectItemIndex()
{
	ALString sDisplayLabel;
	int nMinIndex;
	int nMaxIndex;
	int nDefaultIndex;
	boolean bOk;
	ALString sBuffer;
	int nIndex;

	// Valeurs extremes
	nMinIndex = -1;
	nMaxIndex = nItemNumber - 1;
	nDefaultIndex = GetSelectedItemIndex();

	// Preparation du libelle
	sDisplayLabel = sDisplayLabel + "List index (" + IntToString(nMinIndex) + " to " + IntToString(nMaxIndex) +
			") [" + IntToString(nDefaultIndex) + "]";

	// Attente de la reponse
	bOk = false;
	nIndex = nDefaultIndex;
	while (not bOk)
	{
		TextualDisplayOutput(sDisplayLabel);
		sBuffer = TextualReadInput();
		if (sBuffer.GetLength() == 0)
			bOk = true;
		else
		{
			nIndex = atoi(sBuffer);
			if (nMinIndex <= nIndex and nIndex <= nMaxIndex)
				bOk = true;
		}
	}

	// Memorisation si changement d'index
	if (nIndex != nDefaultIndex)
	{
		// Memorisation de l'index
		SetSelectedItemIndex(nIndex);

		// Memorisation de la selection, par index ou par cle
		if (GetKeyFieldId() == "")
			WriteOutputCommand("List.Index", IntToString(GetSelectedItemIndex()), "List index");
		else
			WriteOutputCommand("List.Key", GetSelectedItemKey(), "List key");
	}
}

void UIList::TextualCardDisplay(Object* container, int nIndex)
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;
	const ALString sSeparator = ';';
	int nItem;
	boolean bContinue = true;
	const int nMaxItem = 20;
	ObjectArray oaUIVisibleDatas;
	ObjectArray oaUIVisibleUnitValues;
	ALString sTitle;

	check(container);
	require(nIndex >= 0);

	// Determination des champs visibles
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		if (uiElement->GetVisible() == true)
		{
			oaUIVisibleDatas.Add(uiElement);
			oaUIVisibleUnitValues.Add(oaUnitValues.GetAt(nField));
		}
	}

	// Affichage de la liste jusqu'a l'action de sortie
	while (bContinue)
	{
		sTitle = GetLabel();
		sTitle += " (";
		sTitle += IntToString(nItemNumber);
		sTitle += " items)";

		// Affichage du titre
		TextualDisplayTitle(sTitle);

		// Si la liste n'est pas vide
		if (nItemNumber > 0)
		{
			// Demande de l'index de l'item selectionne (et pour le depart de
			// l'affichage)
			TextualSelectItemIndex();

			// Affichage si un item selectionne
			if (nSelectedItemIndex >= 0)
			{
				// Si l'ergonomie est CellEditable, on modifie directement les champs
				if (ergonomy == CellEditable)
				{
					for (nField = 0; nField < oaUIVisibleDatas.GetSize(); nField++)
					{
						uiElement = cast(UIData*, oaUIVisibleDatas.GetAt(nField));
						valueContainer = oaUIVisibleUnitValues.GetAt(nField);
						uiElement->TextualCardDisplay(valueContainer, nSelectedItemIndex);
					}
				}

				// Affichage des libelles des champs
				for (nField = 0; nField < oaUIVisibleDatas.GetSize(); nField++)
				{
					uiElement = cast(UIData*, oaUIVisibleDatas.GetAt(nField));
					TextualDisplayOutput(uiElement->GetLabel());
					if (nField < oaUIVisibleDatas.GetSize() - 1)
						TextualDisplayOutput(sSeparator);
				}
				TextualDisplayOutput("\n");

				// Affichage des champs
				for (nItem = GetSelectedItemIndex();
				     nItem < GetItemNumber() and nItem < GetSelectedItemIndex() + nMaxItem; nItem++)
				{
					for (nField = 0; nField < oaUIVisibleDatas.GetSize(); nField++)
					{
						uiElement = cast(UIData*, oaUIVisibleDatas.GetAt(nField));
						valueContainer = oaUIVisibleUnitValues.GetAt(nField);
						uiElement->TextualListDisplay(valueContainer, nItem);
						if (nField < oaUIVisibleDatas.GetSize() - 1)
							TextualDisplayOutput(sSeparator);
					}
					TextualDisplayOutput("\n");
				}
			}
		}

		// Gestion des actions
		bContinue = TextualDealWithActions();
	}
}

void UIList::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jobject list;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUICard
	cls = GraphicGetGUICardID();

	// Recherche de la methode addListField
	mid = GraphicGetMethodID(cls, "GUICard", "addListField",
				 "(Ljava/lang/String;Ljava/lang/String;LnormGUI/engine/GUIList;Ljava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	style = ToJstring(env, sStyle);
	list = GraphicBuildGUIUnit();

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, list, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Idem pour l'objet liste
	env->DeleteLocalRef(list);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

void UIList::SetItemCard(UICard* view)
{
	if (itemCard != NULL)
		delete itemCard;
	itemCard = view;
}

UICard* UIList::GetItemCard()
{
	if (itemCard == NULL)
		CreateItemCard();
	return itemCard;
}

void UIList::ReadParameters(int& nLineNumber, int& nLastColumnExtraWith) const
{
	ALString sLineNumber;
	ALString sLastColumnExtraWidth;
	int nPos;

	// Initialiusation, valide dans le cas de parametres vide
	nLineNumber = 0;
	nLastColumnExtraWith = 0;

	// Parsing si parametres non vide
	if (GetParameters().GetLength() > 0)
	{
		// Decomposition des parametyres en deux champs
		nPos = GetParameters().Find('\n');
		assert(nPos >= 0);
		sLineNumber = GetParameters().Left(nPos);
		sLastColumnExtraWidth = GetParameters().Right(GetParameters().GetLength() - nPos - 1);

		// Extraction des valeurs des parametres
		if (sLineNumber.GetLength() > 0)
		{
			nLineNumber = StringToInt(sLineNumber);
			assert(IntToString(nLineNumber) == sLineNumber);
		}
		if (sLastColumnExtraWidth.GetLength() > 0)
		{
			nLastColumnExtraWith = StringToInt(sLastColumnExtraWidth);
			assert(IntToString(nLastColumnExtraWith) == sLastColumnExtraWidth);
		}
	}
}

void UIList::WriteParameters(int nLineNumber, int nLastColumnExtraWith)
{
	ALString sNewParameters;

	require(0 <= nLineNumber and nLineNumber <= 50);
	require(0 <= nLastColumnExtraWith and nLastColumnExtraWith <= 50);

	// On fabrique la nouvelle valeur des parametres
	if (nLineNumber > 0)
		sNewParameters += IntToString(nLineNumber);
	sNewParameters += "\n";
	if (nLastColumnExtraWith > 0)
		sNewParameters += IntToString(nLastColumnExtraWith);
	SetParameters(sNewParameters);
}
