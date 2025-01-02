// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

boolean UIUnit::Check() const
{
	boolean bOk = true;
	int nField;
	UIData* uiData;
	int nAction;
	UIAction* uiAction;

	// Validite des champs
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiData = cast(UIData*, oaUIDatas.GetAt(nField));
		if (not uiData->Check())
			bOk = false;
	}

	// Validite des actions
	for (nAction = 0; nAction < oaUIActions.GetSize(); nAction++)
	{
		uiAction = cast(UIAction*, oaUIActions.GetAt(nAction));
		if (not uiAction->Check())
			bOk = false;
	}

	return bOk;
}

const ALString UIUnit::GetClassLabel() const
{
	return "UI Unit";
}

void UIUnit::Open()
{
	StringVector svIdentifierPath;
	ALString sValue;
	ALString sIdentifierValue;
	UIData* uiData;
	UIUnit* uiUnit;
	UIList* uiList;
	UIAction* uiAction;
	UIElement* uiField;
	Object* values;
	int i;
	int nIndex;
	int nListIndex;
	boolean bActionExit;
	ALString sCommand;

	require(Check());
	require(GetVisible() == true);

	// Flag d'ouverture de la fenetre, permettant de synchroniser
	bIsOpened = true;

	// Enregistrement d'un commentaire d'entree de fenetre pour les commandes
	if (GetLabel() != "")
	{
		WriteOutputCommand("", "", "");
		WriteOutputCommand("", "", "-> " + GetLabel());
	}

	// Rafraichissement de l'interface par les donnees
	PropagateRefresh();

	///////////////////////////////////////////////
	// On rejoue les commandes tant qu'il y en a
	// On envoie un log des commandes rejouees avec les erreurs eventuelles
	// La politique est tres tolerante aux erreurs: on se contente de les signaler
	bActionExit = false;
	while (bActionExit != true and ReadInputCommand(&svIdentifierPath, sValue) == true)
	{
		// Recherche de l'unite d'interface concernee
		assert(svIdentifierPath.GetSize() > 0);
		sCommand = "";
		uiUnit = this;
		for (i = 0; i < svIdentifierPath.GetSize(); i++)
		{
			sIdentifierValue = svIdentifierPath.GetAt(i);

			// Reconstitution de la commande en cours
			if (sCommand != "")
				sCommand += ".";
			sCommand += sIdentifierValue;

			// Recherche d'une action
			nIndex = uiUnit->GetActionIndex(sIdentifierValue);
			if (nIndex != -1)
			{
				// Execution de l'action
				uiAction = cast(UIAction*, uiUnit->oaUIActions.GetAt(nIndex));
				if (uiAction->GetIdentifier() == "Exit" or uiAction->GetParameters() == "Exit")
					bActionExit = true;
				if (bVerboseCommandReplay)
					AddSimpleMessage("Replay user action\t: " + sCommand);
				WriteOutputCommand(sCommand, sValue, uiAction->GetUnformattedLabel());
				uiUnit->ExecuteUserActionAt(uiAction->GetIdentifier());
				break;
			}

			// Recherche d'un champs
			nIndex = uiUnit->GetFieldIndex(sIdentifierValue);
			if (nIndex != -1)
			{
				uiData = cast(UIData*, uiUnit->oaUIDatas.GetAt(nIndex));

				// Affectation de valeur si champs terminal
				if (uiData->IsElement())
				{
					// Modification du champs si index valide
					if (0 <= uiUnit->nCurrentItemIndex and
					    uiUnit->nCurrentItemIndex < uiUnit->nItemNumber)
					{
						uiField = cast(UIElement*, uiData);
						values = uiUnit->oaUnitValues.GetAt(nIndex);
						if (bVerboseCommandReplay)
							AddSimpleMessage("Replay field update\t: " + sCommand + " " +
									 sValue);
						WriteOutputCommand(sCommand, sValue, uiField->GetUnformattedLabel());
						uiField->SetFieldStringValue(values, uiUnit->nCurrentItemIndex, sValue);
						uiUnit->nFreshness++;

						// Declenchement d'un refresh selon le parametrae du champ
						if (uiField->GetTriggerRefresh())
							uiUnit->ExecuteUserActionAt("Refresh");
						break;
					}
					else
					{
						assert(uiUnit->GetDataType() == List);
						// Arret sinon
						Global::AddError("Command file", "",
								 "Replay failure\t: " + sCommand + " " + sValue +
								     " at index " +
								     IntToString(uiUnit->nCurrentItemIndex));
						CloseCommandFiles();
						DeleteAllInputSearchReplaceValues();
						if (bBatchMode)
							Global::AddFatalError("Command file", "", "Batch mode failure");
						break;
					}
				}
				// Changement d'unite d'interface courante
				else
					uiUnit = cast(UIUnit*, uiData);
			}
			else
			{
				// Recherche de changement d'index
				if (uiUnit->GetDataType() == List)
				{
					uiList = cast(UIList*, uiUnit);
					if (i == svIdentifierPath.GetSize() - 2)
					{
						sIdentifierValue = svIdentifierPath.GetAt(i);
						if (sIdentifierValue == "List")
						{
							sIdentifierValue = svIdentifierPath.GetAt(i + 1);
							sCommand += "." + sIdentifierValue;

							// Cas d'une selection par index
							if (sIdentifierValue == "Index")
							{
								// Calcul de l'index
								nListIndex = atoi(sValue);
								if (nListIndex < 0)
									nListIndex = 0;
								else if (nListIndex > uiList->GetItemNumber())
									nListIndex = uiList->GetItemNumber();

								// Changement d'index si OK
								if (bVerboseCommandReplay)
									AddSimpleMessage(
									    "Replay list item selection\t: " +
									    sCommand + " " + sValue);
								WriteOutputCommand(sCommand, sValue,
										   "List item selection");
								uiList->SetSelectedItemIndex(nListIndex);

								// Sortie normale, sauf si execution non conforme
								if (uiList->GetSelectedItemIndex() == atoi(sValue))
									break;
							}

							// Cas d'une selection par cle
							if (sIdentifierValue == "Key")
							{
								// Changement d'index
								if (bVerboseCommandReplay)
									AddSimpleMessage(
									    "Replay list item selection\t: " +
									    sCommand + " " + sValue);
								WriteOutputCommand(sCommand, sValue,
										   "List item selection");
								uiList->SetSelectedItemKey(sValue);

								// Sortie normale, sauf si execution non conforme
								if (uiList->GetSelectedItemKey() == sValue)
									break;
							}
						}
					}
				}

				// Arret sinon
				Global::AddError("Command file", "", "Replay failure\t: " + sCommand + " " + sValue);
				CloseCommandFiles();
				DeleteAllInputSearchReplaceValues();
				if (bBatchMode)
					Global::AddFatalError("Command file", "", "Batch mode failure");
				break;
			}
		}

		// Liberation
		svIdentifierPath.SetSize(0);
	}
	svIdentifierPath.SetSize(0);

	// Ouverture de la fenetre, selon le mode (inutile si action Exit rejouee)
	if (bActionExit == false)
	{
		// Mode graphique
		if (GetUIMode() == Graphic)
		{
			JNIEnv* env;
			jclass cls;
			jmethodID mid;
			jobject guiObject;

			env = GraphicGetJNIEnv();

			// Fabrication de l'objet Java associe
			guiObject = GraphicBuildGUIUnit();

			// Recherche de la classe GUIUnit
			cls = GraphicGetGUIUnitID();

			// Recherche de la methode open
			mid = GraphicGetMethodID(cls, "GUIUnit", "open", "()V");

			// Appel de la methode JAVA open, qui sera executee dans un nouveau thread
			// et provoquera la suite de l'execution de la methode C++ courante
			env->CallVoidMethod(guiObject, mid);

			// On libere l'objet fiche cote Java, et sa classe
			env->DeleteLocalRef(guiObject);
			env->DeleteLocalRef(cls);
		}
		// Mode textuel
		else
		{
			// Passage en interaction dans la fenetre de shell si cela est autorise
			if (GetTextualInteractiveModeAllowed())
				TextualCardDisplay(this, 0);
			else
			// Sinon, en mode textuel, on n'autorise pas d'interactions sans scenario
			{
				if (fInputCommands != NULL)
					AddFatalError("Unexpected end of file in the input commands file");
				else
					AddFatalError("Missing input commands file");
			}
		}
	}

	// Enregistrement d'un commentaire de sortie de fenetre pour les commandes
	if (GetLabel() != "")
	{
		WriteOutputCommand("", "", "<- " + GetLabel());
		WriteOutputCommand("", "", "");
	}
}

boolean UIUnit::IsOpened() const
{
	return bIsOpened;
}

void UIUnit::SetEditable(boolean bValue)
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
}

// on ajoute un parametre style
void UIUnit::AddBooleanField(const ALString& sFieldId, const ALString& sFieldLabel, boolean bDefaultValue)
{
	UIBooleanElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIBooleanElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(bDefaultValue);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddCharField(const ALString& sFieldId, const ALString& sFieldLabel, char cDefaultValue)
{
	UICharElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UICharElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(cDefaultValue);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddIntField(const ALString& sFieldId, const ALString& sFieldLabel, int nDefaultValue)
{
	UIIntElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIIntElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(nDefaultValue);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddRangedIntField(const ALString& sFieldId, const ALString& sFieldLabel, int nDefaultValue, int nMin,
			       int nMax)
{
	UIIntElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIIntElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(nDefaultValue);
	uiElement->SetMinValue(nMin);
	uiElement->SetMaxValue(nMax);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddDoubleField(const ALString& sFieldId, const ALString& sFieldLabel, double dDefaultValue)
{
	UIDoubleElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIDoubleElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(dDefaultValue);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddRangedDoubleField(const ALString& sFieldId, const ALString& sFieldLabel, double dDefaultValue,
				  double dMin, double dMax)
{
	UIDoubleElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIDoubleElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(dDefaultValue);
	uiElement->SetMinValue(dMin);
	uiElement->SetMaxValue(dMax);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddStringField(const ALString& sFieldId, const ALString& sFieldLabel, const ALString& sDefaultValue)
{
	UIStringElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIStringElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(sDefaultValue);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddRangedStringField(const ALString& sFieldId, const ALString& sFieldLabel, const ALString& sDefaultValue,
				  int nMinLength, int nMaxLength)
{
	UIStringElement* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);

	// Creation et initialisation de l'element d'interface
	uiElement = new UIStringElement;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetLabel(sFieldLabel);
	uiElement->SetDefaultValue(sDefaultValue);
	uiElement->SetMinLength(nMinLength);
	uiElement->SetMaxLength(nMaxLength);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UIUnit::AddAction(const ALString& sActionId, const ALString& sFieldLabel, ActionMethod amActionMethod)
{
	UIAction* uiAction;

	require(sActionId != "");
	require(sActionId.Find(' ') == -1);
	require(IsKeyUsed(sActionId) == false);

	// Creation et initialisation de l'element d'interface
	uiAction = new UIAction;
	uiAction->SetIdentifier(sActionId);
	uiAction->SetParent(this);
	uiAction->SetLabel(sFieldLabel);
	uiAction->SetActionMethod(amActionMethod);

	// Ajout dans la definition de l'interface
	odUIObjects.SetAt(sActionId, uiAction);
	oaUIActions.Add(uiAction);

	ensure(IsKeyUsed(sActionId) == true);
}

int UIUnit::GetFieldNumber() const
{
	return oaUIDatas.GetSize();
}

UIData* UIUnit::GetFieldAtIndex(int nFieldIndex)
{
	require(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());

	return cast(UIData*, oaUIDatas.GetAt(nFieldIndex));
}

UIData* UIUnit::GetFieldAt(const ALString& sFieldId)
{
	int nFieldIndex;

	require(IsKeyUsed(sFieldId) == true);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex != -1);
	return cast(UIData*, oaUIDatas.GetAt(nFieldIndex));
}

void UIUnit::DeleteFieldAt(const ALString& sFieldId)
{
	int nFieldIndex;
	UIData* uiData;
	Object* valueContainer;
	int nIndex;

	require(IsKeyUsed(sFieldId) == true);

	// Recherche de l'index existant
	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex != -1);

	// Recherche de la specification et des donnees correspondant au champ
	uiData = cast(UIData*, oaUIDatas.GetAt(nFieldIndex));
	valueContainer = oaUnitValues.GetAt(nFieldIndex);
	check(valueContainer);

	// Destruction des valeurs
	// On les detruit du dernier au premier, car cela optimise
	// le retaillage des tableaux sous-jacents
	// Attention: les valeurs de type UIUnit sont gerees de facon
	// speciales; elles sont contenues dans l'objet UIUNit lui meme,
	// et donc detruites avec lui
	// (on evite ci-dessous de les detruire 2 fois)
	if (uiData->IsListable())
	{
		for (nIndex = nItemNumber - 1; nIndex >= 0; nIndex--)
			uiData->ValueContainerRemoveItemAt(valueContainer, nIndex);
		delete valueContainer;
	}

	// Deplacement des autres champs vers le debut
	for (nIndex = nFieldIndex + 1; nIndex < GetFieldNumber(); nIndex++)
	{
		oaUIDatas.SetAt(nIndex - 1, oaUIDatas.GetAt(nIndex));
		oaUnitValues.SetAt(nIndex - 1, oaUnitValues.GetAt(nIndex));
	}

	// Retaillage des tableaux
	oaUIDatas.SetSize(oaUIDatas.GetSize() - 1);
	oaUnitValues.SetSize(oaUnitValues.GetSize() - 1);

	// Destruction de la definition de l'interface
	odUIObjects.RemoveKey(sFieldId);
	assert(IsKeyUsed(sFieldId) == false);
	delete uiData;
}

void UIUnit::MoveFieldBefore(const ALString& sFieldId, const ALString& sNextFieldId)
{
	int nFieldIndex;
	int nNewIndex;
	UIData* uiData;
	Object* valueContainer;
	int nIndex;

	require(IsKeyUsed(sFieldId) == true);
	require(sNextFieldId == "" or IsKeyUsed(sNextFieldId) == true);

	// Recherche de l'index existant
	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex != -1);

	// Recherche du nouvel index du champ
	nNewIndex = GetFieldNumber() - 1;
	if (sNextFieldId != "")
		nNewIndex = GetFieldIndex(sNextFieldId);
	assert(0 <= nNewIndex and nNewIndex < GetFieldNumber());

	// Recherche de la specification et des donnees correspondant au champ
	uiData = cast(UIData*, oaUIDatas.GetAt(nFieldIndex));
	valueContainer = oaUnitValues.GetAt(nFieldIndex);
	check(valueContainer);

	// Deplacement des autres champs vers le debut si le nouvel index est plus grand
	if (nNewIndex > nFieldIndex)
	{
		for (nIndex = nFieldIndex + 1; nIndex <= nNewIndex; nIndex++)
		{
			oaUIDatas.SetAt(nIndex - 1, oaUIDatas.GetAt(nIndex));
			oaUnitValues.SetAt(nIndex - 1, oaUnitValues.GetAt(nIndex));
		}
	}
	// Deplacement des autres champs vers la fin si le nouvel index est plus petit
	else if (nNewIndex < nFieldIndex)
	{
		for (nIndex = nFieldIndex; nIndex >= nNewIndex + 1; nIndex--)
		{
			oaUIDatas.SetAt(nIndex, oaUIDatas.GetAt(nIndex - 1));
			oaUnitValues.SetAt(nIndex, oaUnitValues.GetAt(nIndex - 1));
		}
	}

	// Memorisation du champ a son nouvel index
	oaUIDatas.SetAt(nNewIndex, uiData);
	oaUnitValues.SetAt(nNewIndex, valueContainer);

	ensure(IsKeyUsed(sFieldId) == true);
}

int UIUnit::GetActionNumber() const
{
	return oaUIActions.GetSize();
}

UIAction* UIUnit::GetActionAtIndex(int nActionIndex)
{
	require(0 <= nActionIndex and nActionIndex < GetActionNumber());

	return cast(UIAction*, oaUIActions.GetAt(nActionIndex));
}

int UIUnit::GetActionIndex(const ALString& sKey) const
{
	int nIndex;
	UIAction* uiAction;

	for (nIndex = 0; nIndex < oaUIActions.GetSize(); nIndex++)
	{
		uiAction = cast(UIAction*, oaUIActions.GetAt(nIndex));
		if (uiAction->GetIdentifier() == sKey)
			return nIndex;
	}
	return -1;
}

UIAction* UIUnit::GetActionAt(const ALString& sActionId)
{
	int nActionIndex;

	nActionIndex = GetActionIndex(sActionId);
	assert(nActionIndex != -1);
	return cast(UIAction*, oaUIActions.GetAt(nActionIndex));
}

void UIUnit::DeleteActionAt(const ALString& sActionId)
{
	int nActionIndex;
	UIAction* uiAction;
	int nIndex;

	require(IsKeyUsed(sActionId) == true);

	// Recherche de l'index existant
	nActionIndex = GetActionIndex(sActionId);
	assert(nActionIndex != -1);

	// Recherche de la specification de l'action
	uiAction = cast(UIAction*, oaUIActions.GetAt(nActionIndex));

	// Destruction de la definiton de l'interface
	odUIObjects.RemoveKey(sActionId);
	delete uiAction;

	// Deplacement des autres actions vers le debut
	for (nIndex = nActionIndex + 1; nIndex < GetActionNumber(); nIndex++)
		oaUIActions.SetAt(nIndex - 1, oaUIActions.GetAt(nIndex));

	// Retaillage du tableau d'actions
	oaUIActions.SetSize(oaUIActions.GetSize() - 1);

	ensure(IsKeyUsed(sActionId) == false);
}

void UIUnit::MoveActionBefore(const ALString& sActionId, const ALString& sNextActionId)
{
	int nActionIndex;
	int nNewIndex;
	UIAction* uiAction;
	int nIndex;

	require(IsKeyUsed(sActionId) == true);
	require(sNextActionId == "" or IsKeyUsed(sNextActionId) == true);

	// Recherche de l'index existant
	nActionIndex = GetActionIndex(sActionId);
	assert(nActionIndex != -1);

	// Recherche du nouvel index de l'action
	nNewIndex = GetActionNumber() - 1;
	if (sNextActionId != "")
		nNewIndex = GetActionIndex(sNextActionId);
	assert(0 <= nNewIndex and nNewIndex < GetActionNumber());

	// Recherche de la specification de l'action
	uiAction = cast(UIAction*, oaUIActions.GetAt(nActionIndex));

	// Deplacement des autres actions vers le debut si le nouvel index est plus grand
	if (nNewIndex > nActionIndex)
	{
		for (nIndex = nActionIndex + 1; nIndex <= nNewIndex; nIndex++)
			oaUIActions.SetAt(nIndex - 1, oaUIActions.GetAt(nIndex));
	}
	// Deplacement des autres actions vers la fin si le nouvel index est plus petit
	else if (nNewIndex < nActionIndex)
	{
		for (nIndex = nActionIndex; nIndex >= nNewIndex + 1; nIndex--)
			oaUIActions.SetAt(nIndex, oaUIActions.GetAt(nIndex - 1));
	}

	// Memorisation de l'action a son nouvel index
	oaUIActions.SetAt(nNewIndex, uiAction);

	ensure(IsKeyUsed(sActionId) == true);
}

void UIUnit::SetBooleanValueAt(const ALString& sFieldId, boolean bValue)
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Boolean);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	if ((boolean)values->GetAt(nCurrentItemIndex) != bValue)
	{
		values->SetAt(nCurrentItemIndex, bValue);
		nFreshness++;
	}
}

boolean UIUnit::GetBooleanValueAt(const ALString& sFieldId) const
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Boolean);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	return (boolean)values->GetAt(nCurrentItemIndex);
}

void UIUnit::SetCharValueAt(const ALString& sFieldId, char cValue)
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Char);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	if ((char)values->GetAt(nCurrentItemIndex) != cValue)
	{
		values->SetAt(nCurrentItemIndex, (int)cValue);
		nFreshness++;
	}
}

char UIUnit::GetCharValueAt(const ALString& sFieldId) const
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Char);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	return (char)values->GetAt(nCurrentItemIndex);
}

void UIUnit::SetIntValueAt(const ALString& sFieldId, int nValue)
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Int);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	if (values->GetAt(nCurrentItemIndex) != nValue)
	{
		values->SetAt(nCurrentItemIndex, nValue);
		nFreshness++;
	}
}

int UIUnit::GetIntValueAt(const ALString& sFieldId) const
{
	int nFieldIndex;
	IntVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Int);
	values = cast(IntVector*, oaUnitValues.GetAt(nFieldIndex));
	return values->GetAt(nCurrentItemIndex);
}

void UIUnit::SetDoubleValueAt(const ALString& sFieldId, double dValue)
{
	int nFieldIndex;
	DoubleVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Double);
	values = cast(DoubleVector*, oaUnitValues.GetAt(nFieldIndex));
	if (values->GetAt(nCurrentItemIndex) != dValue)
	{
		values->SetAt(nCurrentItemIndex, dValue);
		nFreshness++;
	}
}

double UIUnit::GetDoubleValueAt(const ALString& sFieldId) const
{
	int nFieldIndex;
	DoubleVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Double);
	values = cast(DoubleVector*, oaUnitValues.GetAt(nFieldIndex));
	return values->GetAt(nCurrentItemIndex);
}

void UIUnit::SetStringValueAt(const ALString& sFieldId, const ALString& sValue)
{
	int nFieldIndex;
	StringVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == String);
	values = cast(StringVector*, oaUnitValues.GetAt(nFieldIndex));
	if (values->GetAt(nCurrentItemIndex) != sValue)
	{
		values->SetAt(nCurrentItemIndex, sValue);
		nFreshness++;
	}
}

const ALString& UIUnit::GetStringValueAt(const ALString& sFieldId) const
{
	int nFieldIndex;
	StringVector* values;

	require(0 <= nCurrentItemIndex and nCurrentItemIndex < nItemNumber);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == String);
	values = cast(StringVector*, oaUnitValues.GetAt(nFieldIndex));
	return values->GetAt(nCurrentItemIndex);
}

void UIUnit::ExecuteUserActionAt(const ALString& sActionId)
{
	UIAction* uiAction;
	UIObject* parent;
	UIUnit* root;
	bool hasParent = true;

	// Mise a jour des donnees par l'interface
	// On lance la mise a jour depuis l'unite racine
	parent = this;
	while (hasParent)
	{
		if (parent->GetParent() != NULL)
			parent = parent->GetParent();
		else
			break;
	}
	root = cast(UIUnit*, parent);
	root->PropagateUpdate();

	// Recherche de l'action par sa cle
	uiAction = cast(UIAction*, odUIObjects.Lookup(sActionId));
	check(uiAction);

	// Execution
	(this->*uiAction->GetActionMethod())();

	// Rafraichissement de l'interface par les donnees (sauf si Exit)
	// On lance le rafraichissement depuis l'unite racine
	if (sActionId != "Exit" and uiAction->GetParameters() != "Exit")
	{
		parent = this;
		while (hasParent)
		{
			if (parent->GetParent() != NULL)
				parent = parent->GetParent();
			else
				break;
		}
		root = cast(UIUnit*, parent);
		root->PropagateRefresh();
	}
}

boolean UIUnit::GetBooleanValue(const ALString& sFieldLabel, boolean bInitialValue)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddBooleanField("Value", sFieldLabel, bInitialValue);
	card.Open();
	return card.GetBooleanValueAt("Value");
}

char UIUnit::GetCharValue(const ALString& sFieldLabel, char cInitialValue)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddCharField("Value", sFieldLabel, cInitialValue);
	card.Open();
	return card.GetCharValueAt("Value");
}

int UIUnit::GetIntValue(const ALString& sFieldLabel, int nInitialValue)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddIntField("Value", sFieldLabel, nInitialValue);
	card.Open();
	return card.GetIntValueAt("Value");
}

int UIUnit::GetRangedIntValue(const ALString& sFieldLabel, int nInitialValue, int nMin, int nMax)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddRangedIntField("Value", sFieldLabel, nInitialValue, nMin, nMax);
	card.Open();
	return card.GetIntValueAt("Value");
}

double UIUnit::GetDoubleValue(const ALString& sFieldLabel, double dInitialValue)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddDoubleField("Value", sFieldLabel, dInitialValue);
	card.Open();
	return card.GetDoubleValueAt("Value");
}

double UIUnit::GetRangedDoubleValue(const ALString& sFieldLabel, double dInitialValue, double dMin, double dMax)
{
	UICard card;

	require(sFieldLabel != "");

	card.AddRangedDoubleField("Value", sFieldLabel, dInitialValue, dMin, dMax);
	card.Open();
	return card.GetDoubleValueAt("Value");
}

const ALString& UIUnit::GetStringValue(const ALString& sFieldLabel, const ALString& sInitialValue)
{
	static ALString sValue;
	UICard card;

	require(sFieldLabel != "");

	card.AddStringField("Value", sFieldLabel, sInitialValue);
	card.Open();
	sValue = card.GetStringValueAt("Value");
	return sValue;
}

const ALString& UIUnit::GetRangedStringValue(const ALString& sFieldLabel, const ALString& sInitialValue, int nMinLength,
					     int nMaxLength)
{
	static ALString sValue;
	UICard card;

	require(sFieldLabel != "");

	card.AddRangedStringField("Value", sFieldLabel, sInitialValue, nMinLength, nMaxLength);
	card.Open();
	sValue = card.GetStringValueAt("Value");
	return sValue;
}

void UIUnit::EventRefresh() {}

void UIUnit::EventUpdate() {}

int UIUnit::GetFreshness() const
{
	int nGlobalFreshness;
	int nField;
	UIData* uiElement;
	UIUnit* uiUnit;

	// On fait la somme de la fraicheur de tous les composants
	nGlobalFreshness = nFreshness;
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		if (uiElement->GetDataType() == Card or uiElement->GetDataType() == List)
		{
			uiUnit = cast(UIUnit*, uiElement);
			nGlobalFreshness += uiUnit->GetFreshness();
		}
	}

	return nGlobalFreshness;
}

UIUnit::UIUnit()
{
	bIsOpened = false;
	nCurrentItemIndex = 0;
	nItemNumber = 0;
	nFreshness = 1;
	AddAction("Exit", "Close", (ActionMethod)(&UIUnit::ActionExit));
	AddAction("Refresh", "Refresh", (ActionMethod)(&UIUnit::ActionRefresh));
}

UIUnit::~UIUnit()
{
	int nField;
	int nIndex;
	Object* valueContainer;
	UIData* uiElement;

	// Destruction des valeurs
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		valueContainer = oaUnitValues.GetAt(nField);

		// On les detruit du dernier au premier, car cela optimise
		// le retaillage des tableaux sous-jacents
		// Attention: les valeurs de type UIUnit sont gerees de facon
		// speciales; elles sont contenues dans l'objet UIUNit lui meme,
		// et donc detruites avec lui
		// (on evite ci-dessous de les detruire 2 fois)
		if (uiElement->IsListable())
		{
			for (nIndex = nItemNumber - 1; nIndex >= 0; nIndex--)
				uiElement->ValueContainerRemoveItemAt(valueContainer, nIndex);
			delete valueContainer;
		}
	}

	// Destruction des definitons de l'interface
	odUIObjects.DeleteAll();
}

void UIUnit::PropagateRefresh()
{
	int nField;
	UIData* uiElement;
	UIUnit* uiUnit;

	// On propage les EventRefresh de l'unite vers ses sous-unites
	EventRefresh();
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		if (uiElement->GetDataType() == Card or uiElement->GetDataType() == List)
		{
			uiUnit = cast(UIUnit*, uiElement);
			uiUnit->PropagateRefresh();
		}
	}
}

void UIUnit::PropagateUpdate()
{
	int nField;
	UIData* uiElement;
	UIUnit* uiUnit;

	// On propage les EventUpdate de l'unite vers ses sous-unites
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nField));
		if (uiElement->GetDataType() == Card or uiElement->GetDataType() == List)
		{
			uiUnit = cast(UIUnit*, uiElement);
			uiUnit->PropagateUpdate();
		}
	}
	EventUpdate();
}

void UIUnit::ActionExit()
{
	bIsOpened = false;
	EventExit();
}

void UIUnit::EventExit() {}

void UIUnit::ActionRefresh() {}

void UIUnit::AddField(UIData* uiElement)
{
	Object* valueContainer;

	require(uiElement != NULL);
	require(uiElement->GetIdentifier() != "");
	require(IsKeyUsed(uiElement->GetIdentifier()) == false);
	require(GetDataType() == Card or uiElement->IsListable());

	// Ajout dans la definition de l'interface
	uiElement->SetParent(this);
	odUIObjects.SetAt(uiElement->GetIdentifier(), uiElement);
	oaUIDatas.Add(uiElement);

	// Preparation des containers de donnees
	valueContainer = uiElement->ValueContainerCreate();
	check(valueContainer);
	oaUnitValues.Add(valueContainer);
	if (GetDataType() == Card)
	{
		if (uiElement->IsListable())
			uiElement->ValueContainerAddItem(valueContainer);
	}

	ensure(IsKeyUsed(uiElement->GetIdentifier()) == true);
}

boolean UIUnit::IsListable() const
{
	return false;
}

Object* UIUnit::ValueContainerCreate()
{
	// Les unites d'interfaces gerent elles-meme les valeurs associees
	// (contrairement aux elements d'interface)
	return this;
}

void UIUnit::ValueContainerAddItem(Object* container)
{
	check(container);
	// Une unite de peut etre utilise en tant que liste
	assert(false);
}

void UIUnit::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	check(container);
	require(nIndex >= 0);
	// Une unite de peut etre utilise en tant que liste
	assert(false);
}

void UIUnit::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	check(container);
	require(nIndex >= 0);
	// Une unite de peut etre utilise en tant que liste
	assert(false);
}

void UIUnit::TextualCardDisplay(Object* container, int nIndex)
{
	check(container);
	require(nIndex >= 0);
	// La methode doit etre redefinie dans les sous-classes
	assert(false);
}

void UIUnit::TextualListDisplay(Object* container, int nIndex)
{
	check(container);
	require(nIndex >= 0);
	// Une unite de peut etre utilise en tant que liste
	assert(false);
}

boolean UIUnit::TextualDealWithActions()
{
	int nAction;
	UIAction* uiAction;
	ALString sActionLabel;
	const int nActionExit = 0;
	int nActionIndex;
	ObjectArray oaUIVisibleActions;

	require(oaUIActions.GetSize() > 0);

	// Determination des actions visibles
	for (nAction = 0; nAction < oaUIActions.GetSize(); nAction++)
	{
		uiAction = cast(UIAction*, oaUIActions.GetAt(nAction));
		if (uiAction->GetVisible() == true)
			oaUIVisibleActions.Add(uiAction);
	}

	// Pas de visualisation des actions si uniquement action Exit et Refresh
	if (oaUIVisibleActions.GetSize() == 2)
		nActionIndex = nActionExit;
	else
	{
		// Affichage des actions
		TextualDisplayOutput("\t" + GetLabel() + " commands\n");
		for (nAction = 0; nAction < oaUIVisibleActions.GetSize(); nAction++)
		{
			uiAction = cast(UIAction*, oaUIVisibleActions.GetAt(nAction));

			// Preparation du libelle de l'action
			sActionLabel = "\t";
			sActionLabel += IntToString(nAction);
			sActionLabel += ": ";
			sActionLabel += uiAction->GetUnformattedLabel();
			sActionLabel += "\n";
			TextualDisplayOutput(sActionLabel);
		}
		TextualDisplayOutput("\n");

		// Attente d'un code action
		nActionIndex = TextualReadActionIndex();
	}

	// Enregistrement de l'action dans le fichier de commande
	// On filtre les Refresh, et les Exit hors unite d'interface terminale
	uiAction = cast(UIAction*, oaUIVisibleActions.GetAt(nActionIndex));
	if (uiAction->GetIdentifier() != "Refresh" and
	    (uiAction->GetIdentifier() != "Exit" or uiAction->GetParent()->GetParent() == NULL))
		uiAction->WriteOutputActionCommand();

	// Execution de l'action
	TextualDisplayOutput("\n");
	ExecuteUserActionAt(uiAction->GetIdentifier());
	return nActionIndex != nActionExit and uiAction->GetParameters() != "Exit";
}

int UIUnit::TextualReadActionIndex()
{
	int nResult = 0;
	boolean bOk;
	ALString sBuffer;

	require(oaUIActions.GetSize() > 0);

	// Attente de la saisie d'un index d'action correct
	bOk = false;
	while (not bOk)
	{
		TextualDisplayOutput("\tCommand: ");
		sBuffer = TextualReadInput();
		if (sBuffer.GetLength() == 0)
		{
			// Dans le cas d'un retour chariot, on prend la premiere action
			nResult = 0;
			bOk = true;
		}
		else
		{
			nResult = atoi(sBuffer);
			if (0 <= nResult and nResult < oaUIActions.GetSize())
				bOk = true;
		}
	}
	return (nResult);
}

void UIUnit::GraphicAddField(jobject guiUnit)
{
	require(guiUnit != NULL);
	// Une unite ne peut pas etre instanciee
	assert(false);
}

jobject UIUnit::GraphicBuildGUIUnit()
{
	JNIEnv* env;
	jclass clsCard;
	jclass clsElement;
	jclass cls;
	jobject guiCard;
	jobject guiUnit;
	int nField;
	UIData* uiData;
	UIElement* uiElement;
	int nAction;
	UIAction* uiAction;
	jmethodID mid;
	jstring actionId;
	jstring fieldId;
	jstring identifier;
	jstring label;
	jboolean visible;
	jboolean editable;
	jstring helpText;
	jstring style;
	jstring parameters;
	jboolean triggerRefresh;
	jchar shortcut;
	jstring accelKey;
	jlong handle;
	ALString sJavaClassName;
	jobject field;
	jobject action;

	env = GraphicGetJNIEnv();

	// On recupere la classe java GUIElement
	clsElement = GraphicGetGUIElementID();

	// On recupere la classe java GUICard
	clsCard = GraphicGetGUICardID();

	// On instancie un objet GUICard
	guiCard = env->AllocObject(clsCard);

	// Si l'unite est une fiche
	if (GetDataType() == Card)
	{
		// On recupere la methode java qui renvoie la classe de la fiche
		mid =
		    GraphicGetMethodID(clsCard, "GUICard", "getUserCardClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	}
	// Si l'unite est une liste
	else
	{
		// On recupere la methode java qui renvoie la classe de la liste
		mid =
		    GraphicGetMethodID(clsCard, "GUICard", "getUserListClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	}

	// Liberation de la classe GUICard
	env->DeleteLocalRef(clsCard);

	// On recupere le style associe a l'unite
	style = ToJstring(env, GetStyle());

	// On invoque la methode java qui renvoie la classe en fonction du style
	cls = (jclass)env->CallObjectMethod(guiCard, mid, style);
	env->DeleteLocalRef(style);
	sJavaClassName = "GUIUnit";

	// Liberation de l'objet card
	env->DeleteLocalRef(guiCard);

	// Creation de l'objet Java
	guiUnit = env->AllocObject(cls);
	if (guiUnit == 0)
		Global::AddFatalError("Java class", sJavaClassName, "Can't create Java object");

	// Memorisation par l'objet Java du pointeur sur l'objet C++ associe
	mid = GraphicGetMethodID(cls, sJavaClassName, "setUIObjectHandle", "(J)V");
	handle = (jlong)this;
	env->CallVoidMethod(guiUnit, mid, handle);

	// Recherche de la methode setIdentifier
	mid = GraphicGetMethodID(cls, sJavaClassName, "setIdentifier", "(Ljava/lang/String;)V");
	identifier = ToJstring(env, GetIdentifier());
	env->CallVoidMethod(guiUnit, mid, identifier);
	env->DeleteLocalRef(identifier);

	// Recherche de la methode setLabel et positionnement du libelle
	mid = GraphicGetMethodID(cls, sJavaClassName, "setLabel", "(Ljava/lang/String;)V");
	label = ToJstring(env, GetLabel());
	env->CallVoidMethod(guiUnit, mid, label);
	env->DeleteLocalRef(label);

	// Recherche et appel de la methode setVisible
	mid = GraphicGetMethodID(cls, sJavaClassName, "setVisible", "(Z)V");
	visible = (jboolean)GetVisible();
	env->CallVoidMethod(guiUnit, mid, visible);

	// Recherche et appel de la methode setEditable
	mid = GraphicGetMethodID(cls, sJavaClassName, "setEditable", "(Z)V");
	editable = (jboolean)GetEditable();
	env->CallVoidMethod(guiUnit, mid, editable);

	// Recherche et appel de la methode setHelpText
	mid = GraphicGetMethodID(cls, sJavaClassName, "setHelpText", "(Ljava/lang/String;)V");
	helpText = ToJstring(env, GetHtmlText(GetHelpText()));
	env->CallVoidMethod(guiUnit, mid, helpText);
	env->DeleteLocalRef(helpText);

	// Recherche et appel de la methode setStyle
	mid = GraphicGetMethodID(cls, sJavaClassName, "setStyle", "(Ljava/lang/String;)V");
	style = ToJstring(env, GetStyle());
	env->CallVoidMethod(guiUnit, mid, style);
	env->DeleteLocalRef(style);

	// Recherche et appel de la methode setParameters
	mid = GraphicGetMethodID(cls, sJavaClassName, "setParameters", "(Ljava/lang/String;)V");
	parameters = ToJstring(env, GetParameters());
	env->CallVoidMethod(guiUnit, mid, parameters);
	env->DeleteLocalRef(parameters);

	// Recherche et appel de la methode setShortCut
	mid = GraphicGetMethodID(cls, sJavaClassName, "setShortCut", "(C)V");
	shortcut = (jchar)(GetShortCut());
	env->CallVoidMethod(guiUnit, mid, shortcut);

	// Construction de la composition de l'interface
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiData = cast(UIData*, oaUIDatas.GetAt(nField));
		uiData->GraphicAddField(guiUnit);
	}

	// Positionnement des attributs des champs de l'interface
	// (non deja positionnes par les methodes GraphicAddField):
	//    Visible, Editable, HelpText, Shortcut, Parameters, TriggerRefresh
	for (nField = 0; nField < oaUIDatas.GetSize(); nField++)
	{
		uiData = cast(UIData*, oaUIDatas.GetAt(nField));

		// Recherche de l'objet Java representant le champs
		mid = GraphicGetMethodID(cls, sJavaClassName, "getFieldAt",
					 "(Ljava/lang/String;)LnormGUI/engine/GUIData;");
		fieldId = ToJstring(env, uiData->GetIdentifier());
		field = env->CallObjectMethod(guiUnit, mid, fieldId);
		env->DeleteLocalRef(fieldId);

		// Recherche et appel de la methode setVisible
		mid = GraphicGetMethodID(cls, sJavaClassName, "setVisible", "(Z)V");
		visible = (jboolean)uiData->GetVisible();
		env->CallVoidMethod(field, mid, visible);

		// Recherche et appel de la methode setEditable
		mid = GraphicGetMethodID(cls, sJavaClassName, "setEditable", "(Z)V");
		editable = (jboolean)uiData->GetEditable();
		env->CallVoidMethod(field, mid, editable);

		// Recherche et appel de la methode setHelpText
		mid = GraphicGetMethodID(cls, sJavaClassName, "setHelpText", "(Ljava/lang/String;)V");
		helpText = ToJstring(env, GetHtmlText(uiData->GetHelpText()));
		env->CallVoidMethod(field, mid, helpText);
		env->DeleteLocalRef(helpText);

		// Recherche et appel de la methode setShortCut
		mid = GraphicGetMethodID(cls, sJavaClassName, "setShortCut", "(C)V");
		shortcut = (jchar)uiData->GetShortCut();
		env->CallVoidMethod(field, mid, shortcut);

		// Recherche et appel de la methode setParameters
		mid = GraphicGetMethodID(cls, sJavaClassName, "setParameters", "(Ljava/lang/String;)V");
		parameters = ToJstring(env, uiData->GetParameters());
		env->CallVoidMethod(field, mid, parameters);
		env->DeleteLocalRef(parameters);

		// Recherche et appel de la methode setTriggerRefresh, dans les cas des UIElements
		if (uiData->IsElement())
		{
			uiElement = cast(UIElement*, uiData);
			mid = GraphicGetMethodID(clsElement, "GUIElement", "setTriggerRefresh", "(Z)V");
			triggerRefresh = (jboolean)uiElement->GetTriggerRefresh();
			env->CallVoidMethod(field, mid, triggerRefresh);
		}

		// On libere l'objet champ cote Java
		env->DeleteLocalRef(field);
	}

	// Recherche de la methode addAction
	mid = GraphicGetMethodID(cls, sJavaClassName, "addAction", "(Ljava/lang/String;Ljava/lang/String;)V");

	// Construction des actions
	for (nAction = 0; nAction < oaUIActions.GetSize(); nAction++)
	{
		uiAction = cast(UIAction*, oaUIActions.GetAt(nAction));

		// Preparation des arguments
		actionId = ToJstring(env, uiAction->GetIdentifier());
		label = ToJstring(env, uiAction->GetLabel());

		// Appel de la methode
		env->CallVoidMethod(guiUnit, mid, actionId, label);
		env->DeleteLocalRef(actionId);
		env->DeleteLocalRef(label);
	}

	// Positionnement des attributs Visible, HelpText, Style,
	// Parameters, Shortcut et AccelKey des actions
	for (nAction = 0; nAction < oaUIActions.GetSize(); nAction++)
	{
		uiAction = cast(UIAction*, oaUIActions.GetAt(nAction));

		// Recherche de l'objet Java representant l'action
		mid = GraphicGetMethodID(cls, sJavaClassName, "getActionAt",
					 "(Ljava/lang/String;)LnormGUI/engine/GUIAction;");
		actionId = ToJstring(env, uiAction->GetIdentifier());
		action = env->CallObjectMethod(guiUnit, mid, actionId);
		env->DeleteLocalRef(actionId);

		// Recherche et appel de la methode setVisible
		mid = GraphicGetMethodID(cls, sJavaClassName, "setVisible", "(Z)V");
		visible = (jboolean)uiAction->GetVisible();
		env->CallVoidMethod(action, mid, visible);

		// Recherche et appel de la methode setHelpText
		mid = GraphicGetMethodID(cls, sJavaClassName, "setHelpText", "(Ljava/lang/String;)V");
		helpText = ToJstring(env, GetHtmlText(uiAction->GetHelpText()));
		env->CallVoidMethod(action, mid, helpText);
		env->DeleteLocalRef(helpText);

		// Recherche et appel de la methode setStyle
		mid = GraphicGetMethodID(cls, sJavaClassName, "setStyle", "(Ljava/lang/String;)V");
		style = ToJstring(env, uiAction->GetStyle());
		env->CallVoidMethod(action, mid, style);
		env->DeleteLocalRef(style);

		// Recherche et appel de la methode setParameters
		mid = GraphicGetMethodID(cls, sJavaClassName, "setParameters", "(Ljava/lang/String;)V");
		parameters = ToJstring(env, uiAction->GetParameters());
		env->CallVoidMethod(action, mid, parameters);
		env->DeleteLocalRef(parameters);

		// Recherche et appel de la methode setShortCut
		mid = GraphicGetMethodID(cls, sJavaClassName, "setShortCut", "(C)V");
		shortcut = (jchar)uiAction->GetShortCut();
		env->CallVoidMethod(action, mid, shortcut);

		// Recherche et appel de la methode setAccelKey
		mid = GraphicGetMethodID(GraphicGetGUIActionID(), "GUIAction", "setAccelKey", "(Ljava/lang/String;)V");
		accelKey = ToJstring(env, uiAction->GetAccelKey());
		env->CallVoidMethod(action, mid, accelKey);
		env->DeleteLocalRef(accelKey);

		// On livere l'objet action cote Java
		env->DeleteLocalRef(action);
	}

	// Liberation de la classe
	env->DeleteLocalRef(cls);

	return guiUnit;
}

boolean UIUnit::IsKeyUsed(const ALString& sKey) const
{
	return odUIObjects.Lookup(sKey) != NULL;
}

void UIUnit::WriteOutputUnitFieldCommand(const ALString& sFieldId, const ALString& sValue) const
{
	int nFieldIndex;
	UIData* uiField;

	// Memorisation des dernieres commandes de selection d'index (bufferisees auparavant)
	// avant que l'action ait connaissance des indexs courants des listes
	if (not odListIndexCommands.IsEmpty())
	{
		POSITION position;
		ALString sKey;
		Object* oElement;
		StringObject* soIndexValue;

		// Parcours du dictionnaire pour memoriser les commandes dans le fichier scenario
		position = odListIndexCommands.GetStartPosition();
		while (position != NULL)
		{
			odListIndexCommands.GetNextAssoc(position, sKey, oElement);
			soIndexValue = cast(StringObject*, oElement);
			WriteOutputCommand(sKey, soIndexValue->GetString(), "List item selection");
		}

		// On reinitialise le dictionnaire
		odListIndexCommands.DeleteAll();
	}

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex != -1);
	uiField = cast(UIData*, oaUIDatas.GetAt(nFieldIndex));
	uiField->WriteOutputFieldCommand(sValue);
}

void UIUnit::WriteOutputUnitActionCommand(const ALString& sActionId) const
{
	int nActionIndex;
	UIAction* uiAction;

	// Memorisation des dernieres commandes de selection d'index (bufferisees auparavant)
	// avant que l'action ait connaissance des indexs courants des listes
	if (not odListIndexCommands.IsEmpty())
	{
		POSITION position;
		ALString sKey;
		Object* oElement;
		StringObject* soIndexValue;

		// Parcours du dictionnaire pour memoriser les commandes dans le fichier scenario
		position = odListIndexCommands.GetStartPosition();
		while (position != NULL)
		{
			odListIndexCommands.GetNextAssoc(position, sKey, oElement);
			soIndexValue = cast(StringObject*, oElement);
			WriteOutputCommand(sKey, soIndexValue->GetString(), "List item selection");
		}

		// On reinitialise le dictionnaire
		odListIndexCommands.DeleteAll();
	}

	// Memorisation de l'action
	nActionIndex = GetActionIndex(sActionId);
	assert(nActionIndex != -1);
	uiAction = cast(UIAction*, oaUIActions.GetAt(nActionIndex));
	uiAction->WriteOutputActionCommand();
}

void UIUnit::WriteOutputUnitListIndexCommand(const ALString& sValue) const
{
	UIList* uiList;
	StringObject* soIndexValue;
	ALString sListCommandKey;
	ALString sKey;

	require(GetDataType() == UIData::List);

	// Acces a l'objet en tant que liste
	uiList = cast(UIList*, this);

	// Bufferisation des commandes de selection d'index dans les listes
	// On se contente ici de memoriser ces commandes dans un dictionnaire.
	// Cela permet de ne garder que la derniere selection en cours

	// Cas standard ou la selection se fait par index
	if (uiList->GetKeyFieldId() == "")
	{
		// Calcul de la cle de la commande
		if (GetIdentifierPath() == "")
			sListCommandKey = "List.Index";
		else
			sListCommandKey = GetIdentifierPath() + ".List.Index";

		// Memorisation dans le dictionnaire
		soIndexValue = cast(StringObject*, odListIndexCommands.Lookup(sListCommandKey));
		if (soIndexValue != NULL)
			soIndexValue->SetString(sValue);
		else
		{
			soIndexValue = new StringObject;
			soIndexValue->SetString(sValue);
			odListIndexCommands.SetAt(sListCommandKey, soIndexValue);
		}
	}
	// Cas ou la selection se fait par cle
	else
	{
		// Calcul de la cle de la commande
		if (GetIdentifierPath() == "")
			sListCommandKey = "List.Key";
		else
			sListCommandKey = GetIdentifierPath() + ".List.Key";

		// Recherche de la cle correspondant a l'index selectionne
		assert(StringToInt(sValue) == uiList->GetSelectedItemIndex());
		sKey = uiList->GetSelectedItemKey();

		// Memorisation dans le dictionnaire
		soIndexValue = cast(StringObject*, odListIndexCommands.Lookup(sListCommandKey));
		if (soIndexValue != NULL)
			soIndexValue->SetString(sKey);
		else
		{
			soIndexValue = new StringObject;
			soIndexValue->SetString(sKey);
			odListIndexCommands.SetAt(sListCommandKey, soIndexValue);
		}
	}
}
