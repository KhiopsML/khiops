// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

UICard::UICard()
{
	nItemNumber = 1;
}

UICard::~UICard() {}

void UICard::AddCardField(const ALString& sFieldId, const ALString& sFieldLabel, UICard* card)
{
	UICard* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);
	require(card != NULL);

	// Initialisation de l'element d'interface
	uiElement = card;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetParent(this);
	uiElement->SetLabel(sFieldLabel);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UICard::AddListField(const ALString& sFieldId, const ALString& sFieldLabel, UIList* list)
{
	UIList* uiElement;

	require(sFieldId != "");
	require(sFieldId.Find(' ') == -1);
	require(IsKeyUsed(sFieldId) == false);
	require(list != NULL);

	// Initialisation de l'element d'interface
	uiElement = list;
	uiElement->SetIdentifier(sFieldId);
	uiElement->SetParent(this);
	uiElement->SetLabel(sFieldLabel);

	// Ajout dans la definition de l'interface et preparation des valeurs
	AddField(uiElement);

	ensure(IsKeyUsed(sFieldId) == true);
}

void UICard::ReplaceCardField(const ALString& sFieldId, UICard* newCard)
{
	UICard* oldCard;
	int nFieldIndex;

	require(sFieldId != "");
	require(IsKeyUsed(sFieldId) == true);
	require(GetFieldAt(sFieldId)->GetDataType() == UIData::Card);
	require(newCard != NULL);
	require(not newCard->IsListable());

	// Recherche de l'element d'interface et de son libelle
	oldCard = cast(UICard*, GetFieldAt(sFieldId));
	nFieldIndex = GetFieldIndex(sFieldId);

	// Recopie des caracteristiques de l'ancienne version
	newCard->SetIdentifier(oldCard->GetIdentifier());
	newCard->SetParent(oldCard->GetParent());
	newCard->SetLabel(oldCard->GetLabel());
	newCard->SetStyle(oldCard->GetStyle());
	newCard->SetParameters(oldCard->GetParameters());
	newCard->SetShortCut(oldCard->GetShortCut());
	newCard->SetHelpText(oldCard->GetHelpText());

	// Remplacement dans la definition de l'interface
	odUIObjects.SetAt(sFieldId, newCard);
	oaUIDatas.SetAt(nFieldIndex, newCard);
	oaUnitValues.SetAt(nFieldIndex, newCard);

	// Destruction de l'ancienne version
	delete oldCard;

	ensure(IsKeyUsed(sFieldId) == true);
}

void UICard::ReplaceListField(const ALString& sFieldId, UIList* newList)
{
	UIList* oldList;
	int nFieldIndex;

	require(sFieldId != "");
	require(IsKeyUsed(sFieldId) == true);
	require(GetFieldAt(sFieldId)->GetDataType() == UIData::List);
	require(newList != NULL);
	require(not newList->IsListable());

	// Recherche de l'element d'interface et de son libelle
	oldList = cast(UIList*, GetFieldAt(sFieldId));
	nFieldIndex = GetFieldIndex(sFieldId);

	// Recopie des caracteristiques de l'ancienne version
	newList->SetIdentifier(oldList->GetIdentifier());
	newList->SetParent(oldList->GetParent());
	newList->SetLabel(oldList->GetLabel());

	// Remplacement dans la definition de l'interface
	odUIObjects.SetAt(sFieldId, newList);
	oaUIDatas.SetAt(nFieldIndex, newList);
	oaUnitValues.SetAt(nFieldIndex, newList);

	// Destruction de l'ancienne version
	delete oldList;

	ensure(IsKeyUsed(sFieldId) == true);
}

UICard* UICard::GetCardValueAt(const ALString& sFieldId)
{
	int nFieldIndex;
	UICard* values;

	require(nCurrentItemIndex == 0);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == Card);
	values = cast(UICard*, oaUnitValues.GetAt(nFieldIndex));
	return values;
}

UIList* UICard::GetListValueAt(const ALString& sFieldId)
{
	int nFieldIndex;
	UIList* values;

	require(nCurrentItemIndex == 0);

	nFieldIndex = GetFieldIndex(sFieldId);
	assert(nFieldIndex >= 0);
	assert(cast(UIData*, oaUIDatas.GetAt(nFieldIndex))->GetDataType() == List);
	values = cast(UIList*, oaUnitValues.GetAt(nFieldIndex));
	return values;
}

boolean UICard::Check() const
{
	boolean bOk = true;

	// Appel de la methode ancetre
	bOk = UIUnit::Check();

	// Validite des parametres
	if (GetParameters() != "" and GetParameters() != "NoBorder")
		bOk = false;
	return bOk;
}

const ALString UICard::GetClassLabel() const
{
	return "UI Card";
}

int UICard::GetDataType() const
{
	return Card;
}

void UICard::TextualCardDisplay(Object* container, int nIndex)
{
	int nField;
	Object* valueContainer;
	UIData* uiElement;
	boolean bContinue = true;
	ObjectArray oaUIVisibleDatas;
	ObjectArray oaUIVisibleUnitValues;

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

	// Affichage de la fiche jusqu'a l'action de sortie
	while (bContinue)
	{
		// Affichage du titre
		TextualDisplayTitle(GetLabel());

		// Affichage des champs
		for (nField = 0; nField < oaUIVisibleDatas.GetSize(); nField++)
		{
			uiElement = cast(UIData*, oaUIVisibleDatas.GetAt(nField));
			valueContainer = oaUIVisibleUnitValues.GetAt(nField);
			uiElement->TextualCardDisplay(valueContainer, 0);
		}

		// Gestion des actions
		bContinue = TextualDealWithActions();
	}
}

void UICard::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jobject card;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUICard
	cls = GraphicGetGUICardID();

	// Recherche de la methode addCardField
	mid = GraphicGetMethodID(cls, "GUICard", "addCardField",
				 "(Ljava/lang/String;Ljava/lang/String;LnormGUI/engine/GUICard;Ljava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	style = ToJstring(env, sStyle);

	// Construction de la fiche
	card = GraphicBuildGUIUnit();

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, card, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Idem pour l'objet fiche et sa classe
	env->DeleteLocalRef(card);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

////////////////////////////////////////////////////////////////////////
// Classe UIConfirmationCard

UIConfirmationCard::UIConfirmationCard()
{
	// Initialisation
	SetIdentifier("Confirmation");
	SetLabel("Confirmation");
	bConfirmed = false;

	// L'action OK est une action systeme, utilisant un parametre dedie
	// pour piloter la partie Java
	AddAction("OK", "OK", (ActionMethod)(&UIConfirmationCard::OK));
	GetActionAt("OK")->SetStyle("Button");
	GetActionAt("OK")->SetParameters("Exit");

	// L'action systeme Exit prend un nouveau libelle car elle
	// associee au l'action systeme OK
	GetActionAt("Exit")->SetLabel("Cancel");
}

UIConfirmationCard::~UIConfirmationCard() {}

boolean UIConfirmationCard::OpenAndConfirm()
{
	bConfirmed = false;
	Open();
	return bConfirmed;
}

const ALString UIConfirmationCard::GetClassLabel() const
{
	return "UI confirmation dialog box";
}

void UIConfirmationCard::OK()
{
	bConfirmed = true;
}

////////////////////////////////////////////////////////////////////////
// Classe UIQuestionCard

UIQuestionCard::UIQuestionCard()
{
	// Initialisation
	SetIdentifier("QuestionDialog");
	SetLabel("Question");
	SetStyle("QuestionDialog");
	bConfirmed = false;

	// Ajout d'un champ question
	AddStringField("Question", "", "");
	GetFieldAt("Question")->SetStyle("FormattedLabel");

	// Ajout d'un champ type de question
	AddStringField("QuestionType", "", "");
	GetFieldAt("QuestionType")->SetStyle("FormattedLabel");
	GetFieldAt("QuestionType")->SetVisible(false);

	// L'action OK est une action systeme, utilisant un parametre dedie
	// pour piloter la partie Java
	AddAction("OK", "Yes", (ActionMethod)(&UIQuestionCard::OK));
	GetActionAt("OK")->SetStyle("Button");
	GetActionAt("OK")->SetParameters("Exit");

	// L'action systeme Exit prend un nouveau libelle car elle
	// associee au l'action systeme OK
	GetActionAt("Exit")->SetLabel("No");
}

UIQuestionCard::~UIQuestionCard() {}

boolean UIQuestionCard::GetAnswer(const ALString& sDialogBoxTitle, const ALString& sQuestionType,
				  const ALString& sQuestion)
{
	ALString sOutputFilePath;

	require(sQuestionType == "Information" or sQuestionType == "Question" or sQuestionType == "Message" or
		sQuestionType == "Warning" or sQuestionType == "Error");
	require(sQuestion != "");

	// On verifie que la structure principale de la fiche n'a pas ete modifiee
	assert(GetIdentifier() == "QuestionDialog");
	assert(GetStyle() == "QuestionDialog" or GetStyle() == "");
	assert(GetActionNumber() == 3);
	assert(GetActionIndex("OK") != -1);
	assert(GetActionIndex("Exit") != -1);
	assert(GetActionIndex("Refresh") != -1);
	assert(GetFieldNumber() == 2);
	assert(cast(UIStringElement*, GetFieldAtIndex(0))->GetStyle() == "FormattedLabel");
	assert(cast(UIStringElement*, GetFieldAtIndex(1))->GetStyle() == "FormattedLabel");
	assert(not cast(UIStringElement*, GetFieldAtIndex(1))->GetVisible());

	// Parametrage specifique de la boite de dialogue
	SetLabel(sDialogBoxTitle);
	SetStringValueAt("QuestionType", sQuestionType);
	SetStringValueAt("Question", sQuestion);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Gestion de la coherence entre une interface entierement standard avec des UICard,
	// et une interface specifique java par une boite de dialogue JDialog
	// Il faut gerer a la fois le mode interface et les scenarios, en mode batch ou interface.
	//
	// Interface standard avec des UICard: style ""
	// L'interface standard passe par les actions Yes (OK) et No (Exit) explicitement
	// On initialise le booleen bConfirm a false avant l'ouverture de la fiche.
	// Le bConfim ne sera positionne a true que si l'utilisateur clique sur Yes
	// Cela marche en mode interface et en mode scenario
	//
	// Interface specifique avec des GUIDialog dans java: style "QuestionDialog"
	// L'interface specifique ne declenche aucune action.
	// Cote java, quand on sort par No, on met (biduliquement) la valeur
	// du champ "Question" a vide pour signaler la sortie par No, sinon,
	// on la laisse intacte pour signaler la sortie par "Yes"
	// D'autre part, au moment de la sortie, on enregistre soit la command "OK",
	// soit la commande "Exit", ce qui produit des scenarios conformes au mode standard.
	// On teste donc la sortie par true en regardant la valeur du champ "Question"
	// Et quand on rejoue un scenario, on passe soit par "OK", ce qui est ignore,
	// soit par "Exit", et on forec la valeur du champ "Question" a vide pour
	// interpreter la sortie de la boite de dialogue comme un "No"

	// Ouverture
	if (GetStyle() == "QuestionDialog")
	{
		bConfirmed = true;
		Open();
		bConfirmed = GetStringValueAt("Question") != "";
	}
	// Ouverture en mode standard
	else if (GetStyle() == "")
	{
		bConfirmed = false;
		Open();
	}
	return bConfirmed;
}

const ALString UIQuestionCard::GetClassLabel() const
{
	return "UI QuestionDialog dialog box";
}

void UIQuestionCard::EventExit()
{
	bConfirmed = false;
	SetStringValueAt("Question", "");
}

void UIQuestionCard::OK()
{
	bConfirmed = true;
}

////////////////////////////////////////////////////////////////////////
// Classe UIFileChooserCard

UIFileChooserCard::UIFileChooserCard()
{
	// Initialisation
	SetIdentifier("FileChooser");
	SetLabel("File chooser");
	SetStyle("FileChooser");
	bConfirmed = false;

	// Ajout d'un champ file chooser
	AddStringField("FileName", "File", "");
	GetFieldAt("FileName")->SetStyle("FileChooser");

	// L'action OK est une action systeme, utilisant un parametre dedie
	// pour piloter la partie Java
	AddAction("OK", "Open", (ActionMethod)(&UIFileChooserCard::OK));
	GetActionAt("OK")->SetStyle("Button");
	GetActionAt("OK")->SetParameters("Exit");

	// L'action systeme Exit prend un nouveau libelle car elle
	// associee au l'action systeme OK
	GetActionAt("Exit")->SetLabel("Cancel");
}

UIFileChooserCard::~UIFileChooserCard() {}

const ALString UIFileChooserCard::ChooseFile(const ALString& sDialogBoxTitle, const ALString& sApproveActionLabel,
					     const ALString& sChooseType, const ALString& sChooseParameters,
					     const ALString& sPathFieldIdentifier, const ALString& sPathFieldLabel,
					     const ALString& sInputFilePath)
{
	ALString sOutputFilePath;
	UIStringElement* pathField;

	require(IsIdentifier(sPathFieldIdentifier));
	require(sChooseType == "FileChooser" or sChooseType == "DirectoryChooser");

	// On verifie que la structure principale de la fiche n'a pas ete modifiee
	assert(GetIdentifier() == "FileChooser");
	assert(GetStyle() == "FileChooser" or GetStyle() == "");
	assert(GetActionNumber() == 3);
	assert(GetActionIndex("OK") != -1);
	assert(GetActionIndex("Exit") != -1);
	assert(GetActionIndex("Refresh") != -1);
	assert(GetFieldNumber() == 1);
	assert(cast(UIStringElement*, GetFieldAtIndex(0))->GetStyle() == "FileChooser" or
	       cast(UIStringElement*, GetFieldAtIndex(0))->GetStyle() == "DirectoryChooser");

	// Parametrage specifique de la boite de dialogue
	SetLabel(sDialogBoxTitle);
	DeleteFieldAt(GetFieldAtIndex(0)->GetIdentifier());
	AddStringField(sPathFieldIdentifier, sPathFieldLabel, "");
	pathField = cast(UIStringElement*, GetFieldAtIndex(0));
	pathField->SetStyle(sChooseType);
	pathField->SetParameters(sChooseParameters);
	SetStringValueAt(sPathFieldIdentifier, sInputFilePath);
	GetActionAt("OK")->SetLabel(sApproveActionLabel);

	// Ouverture en mode FileChooser
	bConfirmed = false;
	if (GetStyle() == "FileChooser")
	{
		Open();

		// Recherche du champ en sortie
		sOutputFilePath = GetStringValueAt(sPathFieldIdentifier);
	}
	// Ouverture en mode standard
	else if (GetStyle() == "")
	{
		Open();

		// Memorisation du champ si OK
		if (bConfirmed)
			sOutputFilePath = GetStringValueAt(sPathFieldIdentifier);
		// Sinon, on met a vide
		else
			sOutputFilePath = "";
	}
	return sOutputFilePath;
}

void UIFileChooserCard::SetApproveActionHelpText(const ALString& sValue)
{
	GetActionAt("OK")->SetHelpText(sValue);
}

const ALString& UIFileChooserCard::GetApproveActionHelpText()
{
	return GetActionAt("OK")->GetHelpText();
}

const ALString UIFileChooserCard::GetClassLabel() const
{
	return "UI file chooser dialog box";
}

void UIFileChooserCard::OK()
{
	bConfirmed = true;
}
