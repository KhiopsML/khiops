// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

const ALString UIObject::GetClassLabel() const
{
	return "UI Object";
}

const ALString UIObject::GetObjectLabel() const
{
	return GetLabel();
}

void UIObject::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString& UIObject::GetLabel() const
{
	return sLabel;
}

const ALString UIObject::GetUnformattedLabel() const
{
	ALString sUnformattedLabel;
	boolean bInTag;
	int i;
	char c;

	// On rend le libelle tel quel s'il ne demarre pas pas la balise
	if (sLabel.Find("<html> ") == -1)
		sUnformattedLabel = sLabel;
	// Sinon, on filtre toutes les balises de type html, en fait de facon heuristique les sous-chaines
	// comprise entre '<' et '>'
	else
	{
		bInTag = false;
		i = 0;
		for (i = 0; i < sLabel.GetLength(); i++)
		{
			c = sLabel[i];

			// On ignore les caracteres dans les tags
			if (bInTag)
			{
				if (c == '>')
					bInTag = false;
			}
			// On prend en compte les caracteres hors tags
			else
			{
				if (c == '<')
					bInTag = true;
				else
				{
					// On dedouble les blancs potentiels autour des tags
					if (sUnformattedLabel == "" or
					    sUnformattedLabel[sUnformattedLabel.GetLength() - 1] != ' ' or c != ' ')
						sUnformattedLabel += c;
				}
			}
		}
	}
	return sUnformattedLabel;
}

void UIObject::SetVisible(boolean bValue)
{
	bVisible = bValue;
}

void UIObject::SetHelpText(const ALString& sValue)
{
	sHelpText = sValue;
}

const ALString& UIObject::GetHelpText() const
{
	return sHelpText;
}

boolean UIObject::GetVisible() const
{
	return bVisible;
}

void UIObject::SetStyle(const ALString& sValue)
{
	sStyle = sValue;
}

const ALString& UIObject::GetStyle() const
{
	return sStyle;
}

const ALString& UIObject::GetParameters() const
{
	return sParameters;
}

void UIObject::SetParameters(const ALString& sValue)
{
	sParameters = sValue;
}

const char& UIObject::GetShortCut() const
{
	return cShortCut;
}

void UIObject::SetShortCut(const char& cValue)
{
	cShortCut = cValue;
}

void UIObject::SetIconImage(const ALString& sIconImage)
{
	sIconImageJarPath = sIconImage;
}

ALString UIObject::GetIconImage()
{
	return sIconImageJarPath;
}

UIObject::UIObject()
{
	nInstanceNumber++;
	bVisible = true;
	cShortCut = ' ';
	uioParent = NULL;

	// Initialisation des managers de message
	InitializeMessageManagers();
}

UIObject::~UIObject()
{
	nInstanceNumber--;

	// Le code suivant est buggue (plante sur le DestroyJavaVM)
	// Liberation si necessaire de l'environnement Java et JNI
	// if (nInstanceNumber == 0)
	// {
	//     if (jvmGlobal != NULL)
	//     {
	//         assert(envGlobal != NULL);
	//         jvmGlobal->DestroyJavaVM();
	//         jvmGlobal = NULL;
	//         envGlobal = NULL;
	//     }
	// }
}

boolean UIObject::IsIdentifier(const ALString& sValue) const
{
	boolean bOk;
	int i;
	char c;

	// La valeur doit etre non vide
	bOk = sValue.GetLength() > 0;

	// Lettre acceptees: lettres, chiffres, '_'
	for (i = 0; i < sValue.GetLength(); i++)
	{
		c = sValue.GetAt(i);
		if (not isalnum(c) and c != '_')
		{
			bOk = false;
			AddError("Le nom ne doit comporter que des lettres ou des chiffres");
			break;
		}
	}

	// Ne commence par un chiffre
	if (isdigit(sValue.GetAt(0)))
	{
		bOk = false;
		AddError("Le nom ne doit pas commence par un chiffre");
	}
	return bOk;
}

boolean UIObject::IsJavaClassAvailable(const ALString& sClassName)
{
	JNIEnv* env;
	jclass cls;

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java
	cls = env->FindClass(sClassName);

	if (cls == 0)
	{
		// On supprime l'exception courante pour pouvoir
		// continuer a travailler par la suite
		env->ExceptionClear();
		return false;
	}
	else
	{
		// Liberation de la classe
		env->DeleteLocalRef(cls);

		return true;
	}
}

void UIObject::SetIdentifier(const ALString& sValue)
{
	require(sIdentifier == "" or GetParent() == NULL);

	sIdentifier = sValue;
}

const ALString& UIObject::GetIdentifier() const
{
	return sIdentifier;
}

void UIObject::SetParent(UIObject* value)
{
	uioParent = value;
}

UIObject* UIObject::GetParent() const
{
	return uioParent;
}

ALString UIObject::GetIdentifierPath() const
{
	if (GetParent() == NULL)
		return "";
	else if (GetParent()->GetParent() == NULL)
		return GetIdentifier();
	else
		return GetParent()->GetIdentifierPath() + "." + GetIdentifier();
}

ALString UIObject::GetGlobalIdentifier() const
{
	if (GetParent() == NULL)
		return GetIdentifier();
	else
		return GetParent()->GetGlobalIdentifier() + "." + GetIdentifier();
}

ALString UIObject::GetHtmlText(const ALString sText) const
{
	const ALString sBeginHtml = "<html>";
	const ALString sEndHtml = "</html>";
	const ALString sHtmlNewLine = "<br>";
	ALString sHtmlText;
	int i;

	// Transformation d'un texte en html pour avoir des info-bulle multi-lignes
	// Cf. http://www.jguru.com/faq/view.jsp?EID=10653
	// Si le texte original contient deja une balise html, on le laisse tel quel
	if (sText.GetLength() >= sBeginHtml.GetLength() and sText.Left(sBeginHtml.GetLength()) == sBeginHtml)
		sHtmlText = sText;
	// Sinon, transformation s'il contient des '\n'
	else if (sText.Find('\n') >= 0)
	{
		// Remplacement des '\n' pour mise au format html
		sHtmlText = sBeginHtml;
		for (i = 0; i < sText.GetLength(); i++)
		{
			if (sText.GetAt(i) == '\n')
				sHtmlText += sHtmlNewLine;
			else
				sHtmlText += sText.GetAt(i);
		}
		sHtmlText += sEndHtml;
	}
	// Sinon, on prend le texte tel quel
	else
		sHtmlText = sText;
	return sHtmlText;
}

const ALString& UIObject::TextualReadInput()
{
	int i;
	char sCharBuffer[1 + BUFFER_LENGTH];
	static ALString sResult;

	// Lecture depuis le shell
	StandardGetInputString(sCharBuffer, stdin);

	// Suppression du '\n'
	for (i = 0; sCharBuffer[i] != '\0'; i++)
	{
		if (sCharBuffer[i] == '\n')
			sCharBuffer[i] = '\0';
	}

	// Suppression des blancs a la fin
	for (--i; (i >= 0) and (sCharBuffer[i] == ' '); i--)
		sCharBuffer[i] = '\0';

	// Retour d'une String
	sResult = sCharBuffer;
	return sResult;
}

int UIObject::GetUIMode()
{
	return nUIMode;
}

void UIObject::TextualDisplayOutput(const ALString& sValue)
{
	cout << sValue << flush;
}

void UIObject::TextualDisplayTitle(const ALString& sValue)
{
	TextualDisplayOutput(sValue + "\n");
}

boolean UIObject::SetUIMode(int nValue)
{
	JNIEnv* env;
	boolean bOk = true;
	require(nValue == Textual or nValue == Graphic);

	// On parametre la gestion des taches comme interruptible par l'utilisateur uniquement en mode Graphic
	// Cela permet d'optimiser les temps de traitement en mode batch en evitant l'overhead
	// de gestion des interruptions utilisateurs
	TaskProgression::SetInterruptible(nValue == Graphic);

	// En mode Graphic, on verifie qu'on peut instancier une JVM correctement
	// Sinon on force le mode Textual et on renvoie false
	if (nValue == Graphic)
	{
		env = GraphicGetJNIEnv();
		if (env != NULL)
			nUIMode = Graphic;
		else
		{
			nUIMode = Textual;
			bOk = false;
		}
	}
	else
		nUIMode = Textual;

	// Parametrage des gestionnaires de message
	InitializeMessageManagers();
	return bOk;
}

void UIObject::DisplayMessage(const ALString& sMessage)
{
	// Ajout d'un message, selon le mode de presentation
	if (GetUIMode() == Graphic)
	{
		static const ALString sFatalErrorPrefix = "fatal error :";
		JNIEnv* env;
		jclass cls;
		jmethodID mid;
		jstring message;
		UICard fatalErrorCard;

		// Acces a l'environnement
		env = GraphicGetJNIEnv();
		if (env == NULL)
			return;

		// Recherche de la classe GUIObject
		cls = GraphicGetGUIObjectID();
		if (cls == 0)
			return;

		// Recherche de la methode displayMessage
		mid = GraphicGetStaticMethodID(cls, "GUIObject", "displayMessage", "(Ljava/lang/String;)V");
		if (mid == 0)
			return;

		// Appel de la methode
		message = ToJstring(env, sMessage);
		env->CallStaticVoidMethod(cls, mid, message);
		env->DeleteLocalRef(message);

		// Liberation de la classe
		env->DeleteLocalRef(cls);

		// En cas de message de type erreur fatale, ouverture d'une boite de fermeture
		// pour permettre a l'utilisateur de lire le message avant la fermeture de l'application
		if (sMessage.GetLength() > sFatalErrorPrefix.GetLength() and
		    sMessage.Left(sFatalErrorPrefix.GetLength()) == sFatalErrorPrefix)
		{
			fatalErrorCard.SetIdentifier("FatalError");
			fatalErrorCard.SetLabel("Fatal error");
			fatalErrorCard.AddStringField("Message", "A fatal error has occured, application will close",
						      sMessage);
			fatalErrorCard.GetFieldAt("Message")->SetEditable(false);
			fatalErrorCard.GetActionAt("Exit")->SetLabel("Quit");
			fatalErrorCard.Open();
		}
	}
	else
	{
		cout << sMessage << endl;
	}
}

const ALString UIObject::GetUIEngineVersion()
{
	return "Swing Version 6.0";
}

//////////////////////////////////////////////////////////////////////////////
// Implementation des methodes natives de la classe Java GUIUnit

// Fonction retournant le pointeur sur l'objet C++ UIUnit associe a l'objet Java GUIUnit
UIUnit* GetUIUnitFromGUIUnit(JNIEnv* env, jobject guiUnit)
{
	UIUnit* uiUnit;
	jclass cls;
	jmethodID mid;

	require(guiUnit != NULL);

	// Recherche de la classe GUIUnit
	cls = UIObject::GraphicGetGUIUnitID();

	// Recherche de la methode getUIObjectHandle
	mid = UIObject::GraphicGetMethodID(cls, "GUIUnit", "getUIObjectHandle", "()J");
	uiUnit = (UIUnit*)env->CallLongMethod(guiUnit, mid);
	check(uiUnit);
	env->DeleteLocalRef(cls);

	return uiUnit;
}

/*
 * Class:     GUIUnit
 * Method:    setBooleanValueAt
 * Signature: (Ljava/lang/String;Z)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_setBooleanValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId,
							   jboolean value)
{
	ALString sFieldId;
	UIUnit* uiUnit;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	uiUnit->SetBooleanValueAt(sFieldId, value);
}

/*
 * Class:     GUIUnit
 * Method:    getBooleanValueAt
 * Signature: (Ljava/lang/String;)Z
 */
jboolean JNICALL Java_normGUI_engine_GUIUnit_getBooleanValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	jboolean value;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	value = (jboolean)uiUnit->GetBooleanValueAt(sFieldId);

	return value;
}

/*
 * Class:     GUIUnit
 * Method:    setCharValueAt
 * Signature: (Ljava/lang/String;C)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_setCharValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId, jchar value)
{
	ALString sFieldId;
	UIUnit* uiUnit;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	uiUnit->SetCharValueAt(sFieldId, (char)value);
}

/*
 * Class:     GUIUnit
 * Method:    getCharValueAt
 * Signature: (Ljava/lang/String;)C
 */
jchar JNICALL Java_normGUI_engine_GUIUnit_getCharValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	jchar value;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	value = uiUnit->GetCharValueAt(sFieldId);

	return value;
}

/*
 * Class:     GUIUnit
 * Method:    setIntValueAt
 * Signature: (Ljava/lang/String;I)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_setIntValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId, jint value)
{
	ALString sFieldId;
	UIUnit* uiUnit;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	uiUnit->SetIntValueAt(sFieldId, value);
}

/*
 * Class:     GUIUnit
 * Method:    getIntValueAt
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_normGUI_engine_GUIUnit_getIntValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	jint value;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	value = uiUnit->GetIntValueAt(sFieldId);

	return value;
}

/*
 * Class:     GUIUnit
 * Method:    setDoubleValueAt
 * Signature: (Ljava/lang/String;D)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_setDoubleValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId, jdouble value)
{
	ALString sFieldId;
	UIUnit* uiUnit;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	uiUnit->SetDoubleValueAt(sFieldId, value);
}

/*
 * Class:     GUIUnit
 * Method:    getDoubleValueAt
 * Signature: (Ljava/lang/String;)D
 */
jdouble JNICALL Java_normGUI_engine_GUIUnit_getDoubleValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	jdouble value;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	value = uiUnit->GetDoubleValueAt(sFieldId);

	return value;
}

/*
 * Class:     GUIUnit
 * Method:    setStringValueAt
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_setStringValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId, jstring value)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	ALString sValue;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	sValue = UIObject::FromJstring(env, value);
	uiUnit->SetStringValueAt(sFieldId, sValue);
}

/*
 * Class:     GUIUnit
 * Method:    getStringValueAt
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
jstring JNICALL Java_normGUI_engine_GUIUnit_getStringValueAt(JNIEnv* env, jobject guiUnit, jstring fieldId)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	ALString sValue;
	jstring value;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	sValue = uiUnit->GetStringValueAt(sFieldId);

	value = UIObject::ToJstring(env, sValue);
	return value;
}

/*
 * Class:     GUIUnit
 * Method:    executeUserActionAt
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_executeUserActionAt(JNIEnv* env, jobject guiUnit, jstring actionId)
{
	ALString sActionId;
	UIUnit* uiUnit;

	env->MonitorEnter(guiUnit);
	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sActionId = UIObject::FromJstring(env, actionId);
	uiUnit->ExecuteUserActionAt(sActionId);
	env->MonitorExit(guiUnit);
}

/*
 * Class:     GUIUnit
 * Method:    writeOutputUnitFieldCommand
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitFieldCommand(JNIEnv* env, jobject guiUnit, jstring fieldId,
								     jstring value)
{
	ALString sFieldId;
	UIUnit* uiUnit;
	ALString sValue;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sFieldId = UIObject::FromJstring(env, fieldId);
	sValue = UIObject::FromJstring(env, value);
	uiUnit->WriteOutputUnitFieldCommand(sFieldId, sValue);
}

/*
 * Class:     GUIUnit
 * Method:    writeOutputUnitActionCommand
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitActionCommand(JNIEnv* env, jobject guiUnit, jstring actionId)
{
	ALString sActionId;
	UIUnit* uiUnit;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sActionId = UIObject::FromJstring(env, actionId);
	uiUnit->WriteOutputUnitActionCommand(sActionId);
}

/*
 * Class:     GUIUnit
 * Method:    writeOutputUnitListIndexCommand
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitListIndexCommand(JNIEnv* env, jobject guiUnit, jstring value)
{
	UIUnit* uiUnit;
	ALString sValue;

	uiUnit = GetUIUnitFromGUIUnit(env, guiUnit);
	check(uiUnit);

	sValue = UIObject::FromJstring(env, value);
	uiUnit->WriteOutputUnitListIndexCommand(sValue);
}

// Tableau des methodes de GUIUnit
// Le cast des parametres permettent d'eviter les warning sur certains compilateurs
JNINativeMethod jniGUIUnitMethodArray[] = {
    {(char*)"setBooleanValueAt", (char*)"(Ljava/lang/String;Z)V", (void*)Java_normGUI_engine_GUIUnit_setBooleanValueAt},
    {(char*)"getBooleanValueAt", (char*)"(Ljava/lang/String;)Z", (void*)Java_normGUI_engine_GUIUnit_getBooleanValueAt},
    {(char*)"setCharValueAt", (char*)"(Ljava/lang/String;C)V", (void*)Java_normGUI_engine_GUIUnit_setCharValueAt},
    {(char*)"getCharValueAt", (char*)"(Ljava/lang/String;)C", (void*)Java_normGUI_engine_GUIUnit_getCharValueAt},
    {(char*)"setIntValueAt", (char*)"(Ljava/lang/String;I)V", (void*)Java_normGUI_engine_GUIUnit_setIntValueAt},
    {(char*)"getIntValueAt", (char*)"(Ljava/lang/String;)I", (void*)Java_normGUI_engine_GUIUnit_getIntValueAt},
    {(char*)"setDoubleValueAt", (char*)"(Ljava/lang/String;D)V", (void*)Java_normGUI_engine_GUIUnit_setDoubleValueAt},
    {(char*)"getDoubleValueAt", (char*)"(Ljava/lang/String;)D", (void*)Java_normGUI_engine_GUIUnit_getDoubleValueAt},
    {(char*)"setStringValueAt", (char*)"(Ljava/lang/String;Ljava/lang/String;)V",
     (void*)Java_normGUI_engine_GUIUnit_setStringValueAt},
    {(char*)"getStringValueAt", (char*)"(Ljava/lang/String;)Ljava/lang/String;",
     (void*)Java_normGUI_engine_GUIUnit_getStringValueAt},
    {(char*)"executeUserActionAt", (char*)"(Ljava/lang/String;)V",
     (void*)Java_normGUI_engine_GUIUnit_executeUserActionAt},
    {(char*)"writeOutputUnitFieldCommand", (char*)"(Ljava/lang/String;Ljava/lang/String;)V",
     (void*)Java_normGUI_engine_GUIUnit_writeOutputUnitFieldCommand},
    {(char*)"writeOutputUnitListIndexCommand", (char*)"(Ljava/lang/String;)V",
     (void*)Java_normGUI_engine_GUIUnit_writeOutputUnitListIndexCommand},
    {(char*)"writeOutputUnitActionCommand", (char*)"(Ljava/lang/String;)V",
     (void*)Java_normGUI_engine_GUIUnit_writeOutputUnitActionCommand}};

//////////////////////////////////////////////////////////////////////////////
// Implementation des methodes natives de la classe Java GUIList

// Fonction retournant le pointeur sur l'objet C++ UIList associe a l'objet Java GUIList
UIList* GetUIListFromGUIList(JNIEnv* env, jobject guiList)
{
	UIList* uiList;
	jclass cls;
	jmethodID mid;

	require(guiList != NULL);

	// Recherche de la classe GUIList
	cls = UIObject::GraphicGetGUIListID();

	// Recherche de la methode getUIObjectHandle
	mid = UIObject::GraphicGetMethodID(cls, "GUIList", "getUIObjectHandle", "()J");
	uiList = (UIList*)env->CallLongMethod(guiList, mid);
	check(uiList);
	env->DeleteLocalRef(cls);

	return uiList;
}

/*
 * Class:     GUIList
 * Method:    getItemNumber
 * Signature: ()I
 */
jint JNICALL Java_normGUI_engine_GUIList_getItemNumber(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	return uiList->GetItemNumber();
}

/*
 * Class:     GUIList
 * Method:    addItem
 * Signature: ()V
 */
void JNICALL Java_normGUI_engine_GUIList_addItem(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->AddItem();
}

/*
 * Class:     GUIList
 * Method:    insertItemAt
 * Signature: (I)V
 */
void JNICALL Java_normGUI_engine_GUIList_insertItemAt(JNIEnv* env, jobject guiList, jint itemIndex)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->InsertItemAt(itemIndex);
}

/*
 * Class:     GUIList
 * Method:    removeItemAt
 * Signature: (I)V
 */
void JNICALL Java_normGUI_engine_GUIList_removeItemAt(JNIEnv* env, jobject guiList, jint itemIndex)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->RemoveItemAt(itemIndex);
}

/*
 * Class:     GUIList
 * Method:    removeAllItems
 * Signature: ()V
 */
void JNICALL Java_normGUI_engine_GUIList_removeAllItems(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->RemoveAllItems();
}

/*
 * Class:     GUIList
 * Method:    setCurrentItemIndex
 * Signature: (I)V
 */
void JNICALL Java_normGUI_engine_GUIList_setCurrentItemIndex(JNIEnv* env, jobject guiList, jint currentItemIndex)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->SetCurrentItemIndex(currentItemIndex);
}

/*
 * Class:     GUIList
 * Method:    getCurrentItemIndex
 * Signature: ()I
 */
jint JNICALL Java_normGUI_engine_GUIList_getCurrentItemIndex(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	return uiList->GetCurrentItemIndex();
}

/*
 * Class:     GUIList
 * Method:    setSelectedItemIndex
 * Signature: (I)V
 */
void JNICALL Java_normGUI_engine_GUIList_setSelectedItemIndex(JNIEnv* env, jobject guiList, jint selectedItemIndex)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	uiList->SetSelectedItemIndex(selectedItemIndex);
}

/*
 * Class:     GUIList
 * Method:    getSelectedItemIndex
 * Signature: ()I
 */
jint JNICALL Java_normGUI_engine_GUIList_getSelectedItemIndex(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	return uiList->GetSelectedItemIndex();
}

/*
 * Class:     GUIList
 * Method:    getFreshness
 * Signature: ()I
 */
jint JNICALL Java_normGUI_engine_GUIList_getFreshness(JNIEnv* env, jobject guiList)
{
	UIList* uiList;

	uiList = GetUIListFromGUIList(env, guiList);
	check(uiList);
	return uiList->GetFreshness();
}

// Tableau des methodes de GUIList
// Le cast des parametres permettent d'eviter les warning sur certains compilateurs
JNINativeMethod jniGUIListMethodArray[] = {
    {(char*)"getItemNumber", (char*)"()I", (void*)Java_normGUI_engine_GUIList_getItemNumber},
    {(char*)"addItem", (char*)"()V", (void*)Java_normGUI_engine_GUIList_addItem},
    {(char*)"insertItemAt", (char*)"(I)V", (void*)Java_normGUI_engine_GUIList_insertItemAt},
    {(char*)"removeItemAt", (char*)"(I)V", (void*)Java_normGUI_engine_GUIList_removeItemAt},
    {(char*)"removeAllItems", (char*)"()V", (void*)Java_normGUI_engine_GUIList_removeAllItems},
    {(char*)"setCurrentItemIndex", (char*)"(I)V", (void*)Java_normGUI_engine_GUIList_setCurrentItemIndex},
    {(char*)"getCurrentItemIndex", (char*)"()I", (void*)Java_normGUI_engine_GUIList_getCurrentItemIndex},
    {(char*)"setSelectedItemIndex", (char*)"(I)V", (void*)Java_normGUI_engine_GUIList_setSelectedItemIndex},
    {(char*)"getSelectedItemIndex", (char*)"()I", (void*)Java_normGUI_engine_GUIList_getSelectedItemIndex},
    {(char*)"getFreshness", (char*)"()I", (void*)Java_normGUI_engine_GUIList_getFreshness},
};

//////////////////////////////////////////////////////////////////////////////

longint UIObject::GetUserInterfaceMemoryReserve()
{
	const longint lJNIBaseMemory = 64 * 1024 * 1024;
	const longint lJNIHeapMemory = 192 * 1024 * 1024;

	// Pas de memoire specifique si java n'a pas ete charge (hors IHM et HDFS)
	if (not bIsJVMLoaded)
		return 0;
	// Memoire pour Java: cf. options -Xms et -Xmx
	else
		return lJNIBaseMemory + lJNIHeapMemory;
}

JNIEnv* UIObject::GetJNIEnv()
{
	static PtrGetCreateJavaVM ptrGetCreateJavaVM = NULL;
	static PtrCreateJavaVM ptrCreateJavaVM = NULL;
	JavaVM* vmBuf[1];
	JavaVM* jvm;
	JNIEnv* env;
	jint res;
	jint nVMnumber;
	JavaVMAttachArgs vm_aargs;
	boolean bIsError = false;
	ALString sTmp;
	char sErrorMessage[SYSTEM_MESSAGE_LENGTH + 1];

	////////////////////////////////////////////////////////////////////////////////////////////
	// Cette methode est appelee de nombreuses fois
	// Le code est organise pour que la partie initialisation (chargement de dll, de la jvm...)
	// effectuee une seule fois soit isolee dans un bloc pour que ses variables de travail
	// ne soit declaree et initialisee qu'une seule fois

	// Initialisation des variables locales
	env = NULL;
	jvm = NULL;
	nVMnumber = 0;

	// Chargement de la lib dynamique de jni
	if (jvmHandle == NULL)
	{
#ifdef _WIN32
		static const ALString sJvmLibraryName = "jvm.dll";
		static const ALString sJvmLibraryPathName =
		    FileService::GetFilePath(sJvmLibraryName, FileService::GetPathDirectoryList());
#elif defined __linux__
		static ALString sJvmLibraryName = "libjvm.so";
		static ALString sJvmLibraryPathName = sJvmLibraryName;
#elif defined __APPLE__
		static ALString sJvmLibraryName = "libjvm.dylib";
		static ALString sJvmLibraryPathName = sJvmLibraryName;
#endif
		// Chargement de la librairie
		jvmHandle = LoadSharedLibrary(sJvmLibraryPathName, sErrorMessage);

		// Si pas de lib dynamique, on retourne une erreur utilisateur
		if (jvmHandle == NULL)
		{
#ifdef _WIN32
			Global::AddError("Java", "",
					 sTmp + "Unable to load " + sJvmLibraryName + " (" + sErrorMessage +
					     "), install Java JRE or check the PATH variable");
#else
			Global::AddError("Java", "",
					 sTmp + "Unable to load " + sJvmLibraryName + " (" + sErrorMessage +
					     "), install java JRE or check the LD_LIBRARY_PATH variable");
#endif
			return NULL;
		}
		// Sinon, on va cherche les fonction dont on a besoin dans cette librairie
		else
		{
			// Recherche de fonction JNI_GetCreatedJavaVMs
			if (not bIsError)
			{
				assert(ptrGetCreateJavaVM == NULL);
				ptrGetCreateJavaVM =
				    (PtrGetCreateJavaVM)GetSharedLibraryFunction(jvmHandle, "JNI_GetCreatedJavaVMs");
				if (ptrGetCreateJavaVM == NULL)
				{
					bIsError = true;
					Global::AddError("Java", "",
							 "Can't create Java VM (JNI_GetCreatedJavaVMs not found)");
				}
			}

			// Recherche de fonction JNI_CreateJavaVM
			if (not bIsError)
			{
				assert(ptrCreateJavaVM == NULL);
				ptrCreateJavaVM =
				    (PtrCreateJavaVM)GetSharedLibraryFunction(jvmHandle, "JNI_CreateJavaVM");
				if (ptrCreateJavaVM == NULL)
				{
					Global::AddError("Java", "",
							 "Can't create Java VM (JNI_CreateJavaVM not found)");
				}
			}

			// On decharge la lib dynamique en cas de probleme
			if (bIsError)
			{
				FreeSharedLibrary(jvmHandle);
				jvmHandle = NULL;
				ptrGetCreateJavaVM = NULL;
				ptrCreateJavaVM = NULL;
				return NULL;
			}
		}
	}
	assert(jvmHandle != NULL);
	assert(ptrGetCreateJavaVM != NULL);
	assert(ptrCreateJavaVM != NULL);

	// Comptage du nombre de VM existantes
	res = ptrGetCreateJavaVM(&(vmBuf[0]), 1, &nVMnumber);
	if (res < 0)
	{
		bIsError = true;
		Global::AddError("Java", "",
				 sTmp + "Can't get created Java VM (return code : " + IntToString(res) + ")");

		// On nettoie tout avant de quiter
		FreeSharedLibrary(jvmHandle);
		jvmHandle = NULL;
		ptrGetCreateJavaVM = NULL;
		ptrCreateJavaVM = NULL;
		return NULL;
	}

	// Il y a deja une vm, on s'attache a celle-ci
	if (nVMnumber != 0)
	{
		jvm = vmBuf[0];
		res = jvm->AttachCurrentThread((void**)&env, &vm_aargs);

		if (res != 0)
		{
			DisplayJavaException(env);
			Global::AddFatalError(
			    "Java", "",
			    sTmp + "Call to AttachCurrentThread failed (return code : " + IntToString(res) + ")");
		}
		else
		{
			bIsJVMLoaded = true;
		}

		assert(env != NULL);
		assert(jvm != NULL);
	}
	else
	// Il n'y a pas de VM, on la cree
	{
		// A titre exceptionnel, toutes les variables qui ne servent qu'a l'initialisation de java sont declaree
		// dans ce bloc
		JavaVMInitArgs vm_iargs;
		int nOptionNumber;
		const int nMaxOptionNumber = 20;
		JavaVMOption options[nMaxOptionNumber];
		// Doc sur options: http://docs.oracle.com/cd/E13150_01/jrockit_jvm/jrockit/jrdocs/refman/optionX.html
		// Doc sur options avancees (-XX): http://www.oracle.com/technetwork/java/javase/clopts-139448.html
		// Problemes potentiels avec certains economiseurs d'ecran ou cartes graphiques, resolue par certains
		// option graphiques avancees: http://docs.oracle.com/javase/1.5.0/docs/guide/2d/flags.html#win
		// Options testees, mais finalement non activee (poesent des problemes de rafraichissement
		//  -Dsun.java2d.noddraw=true, -Dsun.java2d.d3d=false, -Dsun.java2d.ddoffscreen=false
		static const ALString sJavaClassPathPrefix = "-Djava.class.path=";
		static const ALString sOptinMinMemory = "-Xms128M";
		static const ALString sOptinMaxMemory = "-Xmx128M";
		static const ALString sDefaultStackSize = "-Xss512k";
		static const ALString sErrorFilePrefix = "-XX:ErrorFile=";
		static const ALString sHeapDumpPathPrefix = "-XX:HeapDumpPath=";
		static const ALString sShowMessageBoxOnError = "-XX:+ShowMessageBoxOnError";
		static const ALString sLogFileSuffix = "JavaCrash.log";
		ALString sClassPath;
		ALString sLogFileName;
		ALString sJavaClassPath;
		ALString sErrorFile;
		ALString sHeapDumpPath;

		assert(env == NULL);
		assert(jvm == NULL);

		// Preparation de l'option de classpath
		sClassPath = p_getenv("CLASSPATH");
		sJavaClassPath = sJavaClassPathPrefix + sClassPath;

		// Redirection des log sur un fichier, base si possible sur le fichier de log des erreurs
		sLogFileName = Global::GetErrorLogFileName();
		sLogFileName = FileService::BuildFilePathName(
		    FileService::GetPathName(sLogFileName),
		    FileService::BuildFileName(FileService::GetFilePrefix(sLogFileName), sLogFileSuffix));
		sErrorFile = sErrorFilePrefix + sLogFileName;

		// Redirection des dumps sur le fichier null
		sHeapDumpPath = sHeapDumpPathPrefix + FileService::GetSystemNulFileName();

		// Parametrage de la machine virtuelle Java
		// On impose une taille minimum et maximum a la heap, pour forcer l'allocation d'une taille raisonnable
		// en perturbant au minimum de le plan d'allocation de l'allocateur C++ Cf. methode GetUsedMemory
		nOptionNumber = 0;
		options[nOptionNumber].optionString = sJavaClassPath;
		nOptionNumber++;
		options[nOptionNumber].optionString = sOptinMinMemory;
		nOptionNumber++;
		options[nOptionNumber].optionString = sOptinMaxMemory;
		nOptionNumber++;
		options[nOptionNumber].optionString = sDefaultStackSize;
		nOptionNumber++;
		options[nOptionNumber].optionString = sErrorFile;
		nOptionNumber++;
		options[nOptionNumber].optionString = sHeapDumpPath;
		nOptionNumber++;
		// Option pour montrer une boite de message en cas d'erreur
		options[nOptionNumber].optionString = sShowMessageBoxOnError;
		nOptionNumber++;
		// Options pour un mode debug du runtime java
		// options[nOptionNumber].optionString = (char*)"-verbose:jni"; // print JNI-related messages
		// nOptionNumber++;
		// options[nOptionNumber].optionString = (char*)"-Xcheck:jni";
		// nOptionNumber++;
		// options[nOptionNumber].optionString = (char*)"-verbose:class";
		// nOptionNumber++;
		assert(nOptionNumber <= nMaxOptionNumber);
		vm_iargs.version = JNI_VERSION_1_6;
		vm_iargs.options = options;
		vm_iargs.nOptions = nOptionNumber;
		vm_iargs.ignoreUnrecognized = JNI_FALSE;

		// Lancement de la JVM
		res = -1;
		res = ptrCreateJavaVM(&jvm, (void**)&env, &vm_iargs);
		if (res < 0)
		{
			env = NULL;
			bIsJVMLoaded = false;
			Global::AddError("Java", "",
					 sTmp + "Can't create Java VM (return code : " + IntToString(res) + ")");
		}
		else
		{
			bIsJVMLoaded = true;
		}
	}
	return env;
}

void UIObject::FreeJNIEnv()
{
	if (jvmHandle != NULL)
	{
		assert(jvmHandle != NULL);
		FreeSharedLibrary(jvmHandle);
		jvmHandle = NULL;
		bIsJVMLoaded = false;
	}
}

JNIEnv* UIObject::GraphicGetJNIEnv()
{
	JNIEnv* env;
	static ALString sLastIconImageJarPath;
	ALString sCurrentLocale;
	jint res;
	jclass cls;
	ALString sTmp;
	static boolean bGraphicIsLoaded = false;

	cls = 0;

#ifdef __APPLE__
	Global::AddError("Java", "GUI", "Java GUI is not available on MacOS");
	env = NULL;
#else
	env = GetJNIEnv();
#endif

	if (env != NULL and not bGraphicIsLoaded)
	{
		bGraphicIsLoaded = true;

		// Memorisation du locale courant (modifie par Java)
		sCurrentLocale = p_setlocale(LC_ALL, NULL);

		// Recherche de la classe JFrame pour savoir si on a acces au GUI
		cls = env->FindClass("javax/swing/JFrame");
		if (cls == 0)
		{
			DisplayJavaException(env);
			Global::AddFatalError(
			    "Java", "GUI",
			    "No GUI available under current environment, try using the tool in batch mode (-b)");
		}
		else
			env->DeleteLocalRef(cls);

		// Recherche de la classe GUIObject pour verifier la presence de la librairie norm.jar

		cls = env->FindClass("normGUI/engine/GUIObject");
		if (cls == 0)
		{
			DisplayJavaException(env);
#ifdef _WIN32
			Global::AddFatalError("Java", "Library",
					      "Please check that Java version is at least Java 7 and check access to "
					      "the norm.jar library in the Java classpath");
#else
			ALString sDisplay;
			sDisplay = p_getenv("DISPLAY");
			if (sDisplay.IsEmpty())
				Global::AddFatalError("Java", "GUI",
						      "No GUI (X server) available under current environment, try "
						      "using the tool in batch mode (-b)");
			else
				Global::AddFatalError("Java", "Library",
						      "Please check that Java version is at least Java 7, that the X "
						      "server is available under current environment and check access "
						      "to the norm.jar library in the Java classpath");
#endif
		}
		else
			env->DeleteLocalRef(cls);

		// Verification de la compatibilite des moteurs d'interface
		if (not CheckUserInterfaceEngineCompatibility(env))
			env = NULL;

		// Enregistrement des methodes natives de GUIUnit
		if (env != NULL)
		{
			cls = GraphicGetGUIUnitID();
			res = env->RegisterNatives(cls, jniGUIUnitMethodArray,
						   sizeof(jniGUIUnitMethodArray) / sizeof(JNINativeMethod));
			if (res != 0)
			{
				DisplayJavaException(env);
				Global::AddFatalError(
				    "Java", "GUIUnit",
				    sTmp + "Can't register native methods (return code : " + IntToString(res) + ")");
			}
			if (cls != 0)
				env->DeleteLocalRef(cls);
		}
		// Enregistrement des methodes natives de GUIList
		if (env != NULL)
		{
			res = 0;
			cls = GraphicGetGUIListID();
			res = env->RegisterNatives(cls, jniGUIListMethodArray,
						   sizeof(jniGUIListMethodArray) / sizeof(JNINativeMethod));
			if (res != 0)
			{
				DisplayJavaException(env);
				Global::AddFatalError(
				    "Java", "GUIList",
				    sTmp + "Can't register native methods (return code : " + IntToString(res) + ")");
			}
			if (cls != 0)
				env->DeleteLocalRef(cls);
		}

		// Restitution du locale courant
		// fonctionne si on met l'instruction suivante ici plutot que dans la methode GraphicGetClassID
		// C'est peut etre moins propre (on est en locale "" depuis CheckUserInterfaceEngineCompatibility)
		// mais on appelle moins souvent setlocale ...
		p_SetApplicationLocale();

		if (env != NULL)
		{
			// Reparametrage eventuel de l'icone
			if (sLastIconImageJarPath != sIconImageJarPath)
			{
				jmethodID mid;
				jstring iconImageJarPath;

				// Recherche de la classe GUIObject
				// On ne passe pas par GraphicGetGUIObjectID (qui fait appel a GraphicGetJNIEnv)
				cls = env->FindClass("normGUI/engine/GUIObject");
				if (cls == 0)
				{
					DisplayJavaException(env);
					Global::AddFatalError("Java", "GUIObject", "Unable to find class");
				}

				// Recherche de la methode setFrameIconPath
				// On ne passe pas par GraphicGetStaticMethodID (qui fait appel a GraphicGetJNIEnv)
				mid = env->GetStaticMethodID(cls, "setFrameIconPath", "(Ljava/lang/String;)V");
				if (mid == 0)
				{
					DisplayJavaException(env);
					Global::AddFatalError("Java class", "GUIObject",
							      "Unable to find method setFrameIconPath");
				}

				// Appel de la methode
				iconImageJarPath = ToJstring(env, sIconImageJarPath);
				env->CallStaticVoidMethod(cls, mid, iconImageJarPath);
				env->DeleteLocalRef(iconImageJarPath);

				// Liberation de la classe
				env->DeleteLocalRef(cls);

				// Memorisation de l'icone
				sLastIconImageJarPath = sIconImageJarPath;
			}
		}
	}
	return env;
}

boolean UIObject::CheckUserInterfaceEngineCompatibility(JNIEnv* env)
{
	jclass cls;
	jmethodID mid;
	ALString sGUIEngineVersion;
	ALString sJavaGUIEngineVersion;
	jstring javaGUIEngineVersion;

	// Recherche de la classe GUIObject
	cls = GraphicGetGUIObjectID();

	// Recherche de la methode getGUIEngineVersion
	mid = GraphicGetStaticMethodID(cls, "GUIObject", "getGUIEngineVersion", "()Ljava/lang/String;");

	// Appel de la methode
	javaGUIEngineVersion = (jstring)env->CallStaticObjectMethod(cls, mid);
	sGUIEngineVersion = FromJstring(env, javaGUIEngineVersion);
	sJavaGUIEngineVersion = sGUIEngineVersion;
	env->DeleteLocalRef(cls);

	// Comparaison des versions
	if (sJavaGUIEngineVersion != GetUIEngineVersion())
	{
		Global::AddFatalError("Java", "User Interface engine",
				      "Check norm.jar lib: java version " + sJavaGUIEngineVersion +
					  " not compatible with C++ version " + GetUIEngineVersion());
		return false;
	}
	return true;
}

void UIObject::DisplayJavaException(JNIEnv* env)
{
	require(env != NULL);

	// Affichage d'une exception (dans la fenetre de shell)
	if (env->ExceptionOccurred())
		env->ExceptionDescribe();
	else
		cout << "No exception was thrown." << endl;

	// On passe env a NULL
	env = NULL;
}

jclass UIObject::GraphicGetGUIActionID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUIAction");
	return cls;
}

jclass UIObject::GraphicGetGUICardID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUICard");
	return cls;
}

jclass UIObject::GraphicGetGUIListID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUIList");
	return cls;
}

jclass UIObject::GraphicGetGUIObjectID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUIObject");
	return cls;
}

jclass UIObject::GraphicGetGUIUnitID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUIUnit");
	return cls;
}

jclass UIObject::GraphicGetGUIElementID()
{
	jclass cls;
	cls = GraphicGetClassID("normGUI/engine/GUIElement");
	return cls;
}

jclass UIObject::GraphicGetClassID(const char* sClassName)
{
	JNIEnv* env;
	jclass cls;

	require(sClassName != NULL);

	env = GraphicGetJNIEnv();
	cls = env->FindClass(sClassName);
	if (cls == 0)
	{
		DisplayJavaException(env);
		Global::AddFatalError("Java class", sClassName, "Unable to find class");
	}
	return cls;
}

jmethodID UIObject::GraphicGetMethodID(jclass cls, const char* sClassName, const char* sMethodName,
				       const char* sMethodSignature)
{
	JNIEnv* env;
	jmethodID mid;
	ALString sTmp;

	require(cls != NULL);
	require(sClassName != NULL);
	require(sMethodName != NULL);
	require(sMethodSignature != NULL);

	env = GraphicGetJNIEnv();
	mid = env->GetMethodID(cls, sMethodName, sMethodSignature);
	if (mid == 0)
	{
		DisplayJavaException(env);
		Global::AddFatalError("Java class", sClassName, sTmp + "Unable to find method " + sMethodName);
	}
	return mid;
}

jmethodID UIObject::GraphicGetStaticMethodID(jclass cls, const char* sClassName, const char* sMethodName,
					     const char* sMethodSignature)
{
	JNIEnv* env;
	jmethodID mid;
	ALString sTmp;

	require(cls != NULL);
	require(sClassName != NULL);
	require(sMethodName != NULL);
	require(sMethodSignature != NULL);

	env = GraphicGetJNIEnv();
	mid = env->GetStaticMethodID(cls, sMethodName, sMethodSignature);
	if (mid == 0)
	{
		DisplayJavaException(env);
		Global::AddFatalError("Java class", sClassName, sTmp + "Unable to find method " + sMethodName);
	}
	return mid;
}

const ALString UIObject::FromJstring(JNIEnv* envValue, const jstring value)
{
	const char cDEL = (char)127;
	ALString sResult;
	ALString sResultUTF8;
	ALString sResultANSI;
	boolean bIsUTF8;

	require(envValue != NULL);

	// Decodage au format UTF8 et ANSI
	sResultUTF8 = NativeFromJstring(envValue, value, true);
	sResultANSI = NativeFromJstring(envValue, value, false);

	// On teste si la chaine C++ obtenue contient le caractere DEL a la fin,
	// signifiant qu'elle avait ete codee en ANSI
	// et dans ce cas, on refait le decodage en ANSI
	// Attention: solution heuristique pour etre compatible UTF8 au maximum, et sinon ANSI
	bIsUTF8 = sResultUTF8.GetLength() == 0 or sResultUTF8[sResultUTF8.GetLength() - 1] != cDEL;

	// S'il n'y avait pas d'encodage avec le caractere DEL, on fait un deuxieme test heuristique
	// base sur la longueur de la chaine decodee
	// On active cette solution heuristique uniquement pour Windows

#ifdef _WIN32
	if (bIsUTF8)
	{
		if (sResultUTF8.GetLength() > sResultANSI.GetLength())
			bIsUTF8 = false;
	}
#endif

	// Choix du decodage
	if (bIsUTF8)
		sResult = sResultUTF8;
	else
	{
		sResult = sResultANSI;

		// Suppression du caractere DEL de fin, si present dans la version UTF8
		if (sResultUTF8.GetLength() > 0 and sResultUTF8[sResultUTF8.GetLength() - 1] == cDEL)
			sResult = sResult.GetBufferSetLength(sResult.GetLength() - 1);
	}
	return sResult;
}

const jstring UIObject::ToJstring(JNIEnv* envValue, const ALString& sValue)
{
	const char cDEL = (char)127;
	jstring jstringValue;
	boolean bIsUTF8;

	require(envValue != NULL);

	// Test si la chaine est en UTF8
	bIsUTF8 = IsUTF8(envValue, sValue);

	// Enodage UTF8 si compatible
	if (bIsUTF8)
		jstringValue = NativeToJstring(envValue, sValue, true);
	// Sinon, encodage ascii, en rajoutant un DEL a la fin pour memoriser le type d'encodage
	// Attention: solution heuristique pour etre compatible UTF8 au maximum, et sinon ANSI
	else
	{
		// Ajout du caractere DEL de fin, sauf si present
		if (sValue.GetLength() > 0 and sValue[sValue.GetLength() - 1] == cDEL)
			jstringValue = NativeToJstring(envValue, sValue, false);
		else
			jstringValue = NativeToJstring(envValue, sValue + cDEL, false);
	}
	return jstringValue;
}

const ALString UIObject::NativeFromJstring(JNIEnv* envValue, const jstring value, boolean bIsUTF8)
{
	ALString sCurrentLocale;
	ALString sResult;
	jclass class_java_lang_String;
	jstring codeString;
	jmethodID mid_getBytes;
	jbyteArray byte_array;
	jsize len;
	jbyte* p_bytes;

	// Memorisation du locale courant (modifie par Java: cf GraphicGetJNIEnv)
	sCurrentLocale = p_setlocale(LC_ALL, NULL);

	// On utilise directment l'API JNI (sans passer par GraphicGetClassID ou GraphicGetMethodID)
	// pour eviter les recursion avec GraphicGetJNIEnv
	class_java_lang_String = envValue->FindClass("java/lang/String");
	mid_getBytes = envValue->GetMethodID(class_java_lang_String, "getBytes", "(Ljava/lang/String;)[B");

	// Choix du format d'encodage
	if (bIsUTF8)
		codeString = envValue->NewStringUTF("UTF-8");
	// Codge page ANSI latin (specifique pour Windows)
	else
#ifdef _WIN32
		codeString = envValue->NewStringUTF("Windows-1252");
#else
		codeString = envValue->NewStringUTF("ISO-8859-1");
#endif

	// Transcodage direct de tableaux de bytes, pour maitriser les transcodages
	byte_array = (jbyteArray)envValue->CallObjectMethod(value, mid_getBytes, codeString);
	len = envValue->GetArrayLength(byte_array);
	p_bytes = envValue->GetByteArrayElements(byte_array, JNI_FALSE);
	if (len > 0)
	{
		// On passe par un ALString directement, pour ne pas avoir de probleme de desallocation
		sResult.GetBufferSetLength(len);
		memcpy(sResult.GetBuffer(len), p_bytes, len);
	}
	envValue->ReleaseByteArrayElements(byte_array, p_bytes, 0);

	// Restitution du locale courant
	p_setlocale(LC_ALL, sCurrentLocale);
	return sResult;
}

const jstring UIObject::NativeToJstring(JNIEnv* envValue, const ALString& sValue, boolean bIsUTF8)
{
	ALString sCurrentLocale;
	jstring jstringValue;
	jclass class_java_lang_String;
	jmethodID mid_string_ctor;
	jbyteArray byte_array;
	jstring codeString;
	const char* sValueChars;

	require(envValue != NULL);

	// Memorisation du locale courant (modifie par Java: cf GraphicGetJNIEnv)
	sCurrentLocale = p_setlocale(LC_ALL, NULL);

	// On utilise directment l'API JNI (sans passer par GraphicGetClassID ou GraphicGetMethodID)
	// pour eviter les recursion avec GraphicGetJNIEnv
	class_java_lang_String = envValue->FindClass("Ljava/lang/String;");
	mid_string_ctor = envValue->GetMethodID(class_java_lang_String, "<init>", "([BLjava/lang/String;)V");

	// Choix du format d'encodage
	if (bIsUTF8)
		codeString = envValue->NewStringUTF("UTF-8");
	// Codge page ANSI latin (specifique pour Windows)
	else
#ifdef _WIN32
		codeString = envValue->NewStringUTF("Windows-1252");
#else
		codeString = envValue->NewStringUTF("ISO-8859-1");
#endif

	// Transcodage direct de tableaux de bytes, pour maitriser les transcodages
	byte_array = envValue->NewByteArray(sValue.GetLength());
	sValueChars = (const char*)sValue;
	envValue->SetByteArrayRegion(byte_array, 0, sValue.GetLength(), (jbyte*)sValueChars);
	jstringValue = (jstring)envValue->NewObject(class_java_lang_String, mid_string_ctor, byte_array, codeString);

	// Restitution du locale courant
	p_setlocale(LC_ALL, sCurrentLocale);
	return jstringValue;
}

boolean UIObject::IsUTF8(JNIEnv* envValue, const ALString& sValue)
{
	boolean bIsUTF8;
	jstring jstringValue;
	ALString sJavaValue;

	require(envValue != NULL);

	// On essaie d'encoder en UTF8 vers Java
	jstringValue = NativeToJstring(envValue, sValue, true);

	// On redecode au meme format vers C++
	sJavaValue = NativeFromJstring(envValue, jstringValue, true);

	// Si l'aller-retour s'est bien passe, les chaines avant et apres le cycle encodage-decodage
	// doivent etre identiques
	bIsUTF8 = (sValue == sJavaValue);
	return bIsUTF8;
}

CommandLine* UIObject::GetCommandLineOptions()
{
	return &commandLineOptions;
}

//////////////////////////////////////////////////
// Gestion des fichiers d'entree et de sortie

boolean UIObject::CheckCommandLineOptions(const ObjectArray& oaOptions)
{
	boolean bOk = true;
	int nCurrentUIMode;
	CommandLineOption* option;
	FileSpec* fsInputFile;
	FileSpec* fsInputJsonFile;
	FileSpec* fsProgressionFile;
	FileSpec* fsOutputFile;
	FileSpec* fsOutputNoReplayFile;
	FileSpec* fsErrorFile;
	FileSpec* fsFile;
	FileSpec* fsFileToCompare;
	ObjectArray oaSpecFiles;
	ObjectDictionary odUsedOptions;
	int i;
	int nRef;
	ALString sTmp;

	// Gestion des erreurs en mode textuel
	nCurrentUIMode = GetUIMode();
	if (nCurrentUIMode == Graphic)
		SetUIMode(Textual);

	// Acces aux fichiers passes en parametre
	fsInputFile = NULL;
	fsProgressionFile = NULL;
	fsOutputFile = NULL;
	fsErrorFile = NULL;
	for (i = 0; i < oaOptions.GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptions.GetAt(i));
		odUsedOptions.SetAt(sTmp + option->GetFlag(), option);
		switch (option->GetFlag())
		{
		case 'i':
			assert(option->GetParameters()->GetSize() == 1);
			fsInputFile = new FileSpec;
			fsInputFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsInputFile->SetLabel("input commands file");
			oaSpecFiles.Add(fsInputFile);
			break;
		case 'j':
			assert(option->GetParameters()->GetSize() == 1);
			fsInputJsonFile = new FileSpec;
			fsInputJsonFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsInputJsonFile->SetLabel("input parameters json file");
			oaSpecFiles.Add(fsInputJsonFile);
			break;
		case 'p':
			assert(option->GetParameters()->GetSize() == 1);
			fsProgressionFile = new FileSpec;
			fsProgressionFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsProgressionFile->SetLabel("progression messages file");
			oaSpecFiles.Add(fsProgressionFile);
			break;
		case 'o':
			assert(option->GetParameters()->GetSize() == 1);
			fsOutputFile = new FileSpec;
			fsOutputFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsOutputFile->SetLabel("output commands file");
			oaSpecFiles.Add(fsOutputFile);
			break;
		case 'O':
			assert(option->GetParameters()->GetSize() == 1);
			fsOutputNoReplayFile = new FileSpec;
			fsOutputNoReplayFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsOutputNoReplayFile->SetLabel("output commands file without replay");
			oaSpecFiles.Add(fsOutputNoReplayFile);
			break;
		case 'e':
			assert(option->GetParameters()->GetSize() == 1);
			fsErrorFile = new FileSpec;
			fsErrorFile->SetFilePathName(option->GetParameters()->GetAt(0));
			fsErrorFile->SetLabel("logs file");
			oaSpecFiles.Add(fsErrorFile);
			break;
		default:
			break;
		}
	}

	// Test que le fichier d'entree n'est pas une sortie
#ifdef __linux_or_apple__
	if (fsInputFile != NULL and
	    (fsInputFile->GetFilePathName() == "/dev/stdout" or fsInputFile->GetFilePathName() == "/dev/stderr"))
	{
		bOk = false;
		Global::AddError("Command line parameters", "",
				 "wrong input command file " + fsInputFile->GetFilePathName());
	}
#endif

	// Test de conflit entre les noms de fichiers : les fichiers doivent tous etre differents
	for (i = 0; i < oaSpecFiles.GetSize(); i++)
	{
		fsFile = cast(FileSpec*, oaSpecFiles.GetAt(i));

		// Parcours des autres fichiers
		for (nRef = i + 1; nRef < oaSpecFiles.GetSize(); nRef++)
		{
			fsFileToCompare = cast(FileSpec*, oaSpecFiles.GetAt(nRef));

// Test de difference
#ifdef _WIN32
			bOk = bOk and fsFileToCompare->CheckReferenceFileSpec(fsFile);
#else
			bOk = bOk and (fsFileToCompare->CheckReferenceFileSpec(fsFile) or
				       (fsFileToCompare->GetFilePathName() == "/dev/stdout" and
					fsFile->GetFilePathName() == "/dev/stdout") or
				       (fsFileToCompare->GetFilePathName() == "/dev/stderr" and
					fsFile->GetFilePathName() == "/dev/stderr"));
#endif // _WIN32
			if (not bOk)
				break;
		}
		if (not bOk)
			break;
	}
	oaSpecFiles.DeleteAll();

	// Le mode batch ne peut etre utilise qu'avec un fichier de commande en entree
	if (odUsedOptions.Lookup("b") != NULL and odUsedOptions.Lookup("i") == NULL)
	{
		Global::AddError("Command line parameters", "", "-b flag must be used with -i");
		bOk = false;
	}

	// Le parametrage des search/replace ne peut etre utilise qu'avec un fichier de commande en entree
	if (odUsedOptions.Lookup("r") != NULL and odUsedOptions.Lookup("i") == NULL)
	{
		Global::AddError("Command line parameters", "", "-r flag must be used with -i");
		bOk = false;
	}

	// Le parametrage par un fichier json en entree ne peut etre utilise qu'avec un fichier de commande en entree
	if (odUsedOptions.Lookup("j") != NULL and odUsedOptions.Lookup("i") == NULL)
	{
		Global::AddError("Command line parameters", "", "-j flag must be used with -i");
		bOk = false;
	}

	// Les option -j et -r sont exclusives
	if (odUsedOptions.Lookup("j") != NULL and odUsedOptions.Lookup("r") != NULL)
	{
		Global::AddError("Command line parameters", "", "-j and -r flags are exclusive");
		bOk = false;
	}

	// Les option -o et -O sont exclusives
	if (odUsedOptions.Lookup("o") != NULL and odUsedOptions.Lookup("O") != NULL)
	{
		Global::AddError("Command line parameters", "", "-o and -O flags are exclusive");
		bOk = false;
	}

	// Retour au mode initial
	if (nCurrentUIMode == Graphic)
		SetUIMode(Graphic);
	ensure(not bOk or commandFile.Check());
	return bOk;
}

void UIObject::ParseMainParameters(int argc, char** argv)
{
	static boolean bIsUserExitHandlerSet = false;
	boolean bOk = true;
	CommandLineOption* oInputScenario;
	CommandLineOption* oInputParameters;
	CommandLineOption* oError;
	CommandLineOption* oOutputScenario;
	CommandLineOption* oOutputScenarioNoReplay;
	CommandLineOption* oReplace;
	CommandLineOption* oBatchMode;
	CommandLineOption* oTask;
	ALString sMessage;
	ALString sToolName;
	ALString sGlobalHelp;

	// Aide globale
	sToolName = commandLineOptions.GetCommandName();
	if (sToolName == "")
		sToolName = "tool";
	sGlobalHelp = "";
	sGlobalHelp += "Examples:\n";
	sGlobalHelp += "   " + sToolName + " -e log.txt\n";
	sGlobalHelp += "   " + sToolName + " -o scenario.txt\n";
	sGlobalHelp += "   " + sToolName + " -i scenario.txt -r less:more -r 70:90\n";
	sGlobalHelp += "\n";
	sGlobalHelp += "In the first example all the logs are stored in the file log.txt\n";
	sGlobalHelp +=
	    "In the second example, " + sToolName + " records all user interactions in the file scenario.txt\n";
	sGlobalHelp += "In the last example, " + sToolName + " replays all user interactions stored in the\n";
	sGlobalHelp += "file scenario.txt after having replaced 'less' by 'more' and '70' by '90'\n";

	// Option sur le fichier d'erreur
	oError = new CommandLineOption;
	oError->SetFlag('e');
	oError->AddDescriptionLine("store logs in the file");
	oError->SetParameterRequired(true);
	oError->SetParameterDescription(CommandLineOption::sParameterFile);
	oError->SetMethod(ErrorCommand);
	oError->SetPriority(1);
	commandLineOptions.AddOption(oError);

	// Option sur le mode batch
	oBatchMode = new CommandLineOption;
	oBatchMode->SetFlag('b');
	oBatchMode->AddDescriptionLine("batch mode, with no GUI");
	oBatchMode->SetParameterRequired(false);
	oBatchMode->SetMethod(BatchCommand);
	oBatchMode->SetPriority(0);
	commandLineOptions.AddOption(oBatchMode);

	// Option sur le fichier de commandes en entree
	oInputScenario = new CommandLineOption;
	oInputScenario->SetFlag('i');
	oInputScenario->AddDescriptionLine("replay commands stored in the file");
	oInputScenario->SetParameterRequired(true);
	oInputScenario->SetParameterDescription(CommandLineOption::sParameterFile);
	oInputScenario->SetMethod(InputCommand);
	commandLineOptions.AddOption(oInputScenario);

	// Option sur le fichier de parametres en entree
	oInputParameters = new CommandLineOption;
	oInputParameters->SetFlag('j');
	oInputParameters->AddDescriptionLine("json file used to set replay parameters");
	oInputParameters->SetParameterRequired(true);
	oInputParameters->SetParameterDescription(CommandLineOption::sParameterFile);
	oInputParameters->SetMethod(JsonCommand);
	commandLineOptions.AddOption(oInputParameters);

	// Option sur le fichier de commandes en en sortie
	oOutputScenario = new CommandLineOption;
	oOutputScenario->SetFlag('o');
	oOutputScenario->AddDescriptionLine("record commands in the file");
	oOutputScenario->SetParameterRequired(true);
	oOutputScenario->SetParameterDescription(CommandLineOption::sParameterFile);
	oOutputScenario->SetMethod(OutputCommand);
	commandLineOptions.AddOption(oOutputScenario);

	// Option sur le fichier de commandes en en sortie
	oOutputScenarioNoReplay = new CommandLineOption;
	oOutputScenarioNoReplay->SetFlag('O');
	oOutputScenarioNoReplay->AddDescriptionLine("same as -o option, but without replay");
	oOutputScenarioNoReplay->SetParameterRequired(true);
	oOutputScenarioNoReplay->SetParameterDescription(CommandLineOption::sParameterFile);
	oOutputScenarioNoReplay->SetMethod(OutputCommand);
	commandLineOptions.AddOption(oOutputScenarioNoReplay);

	// Option sur le parametrage des serach/replace
	oReplace = new CommandLineOption;
	oReplace->SetFlag('r');
	oReplace->AddDescriptionLine("search and replace in the command file");
	oReplace->SetParameterRequired(true);
	oReplace->SetParameterDescription(CommandLineOption::sParameterString + ":" +
					  CommandLineOption::sParameterString);
	oReplace->SetMethod(ReplaceCommand);
	oReplace->SetRepetitionAllowed(true);
	commandLineOptions.AddOption(oReplace);

	// Option sur le fichier de progression
	oTask = new CommandLineOption;
	oTask->SetFlag('p');
	oTask->AddDescriptionLine("store last progression messages");
	oTask->SetParameterRequired(true);
	oTask->SetParameterDescription(CommandLineOption::sParameterFile);
	oTask->SetMethod(TaskProgressionCommand);
	commandLineOptions.AddOption(oTask);
	commandLineOptions.SetGlobalHelp(sGlobalHelp);
	commandLineOptions.SetGlobalCheckMethod(CheckCommandLineOptions);

	// Nettoyage initial
	CleanCommandLineManagement();

	// Nettoyage et notament fermeture des fichiers en cas d'erreur fatale
	if (not bIsUserExitHandlerSet)
	{
		AddUserExitHandler(ExitHandlerCleanCommandLineManagement);
		bIsUserExitHandlerSet = true;
	}

	// Parsing de la ligne de commande existante
	// pour detecter les options
	bVerboseCommandReplay = false;
	bBatchMode = false;

	// Analyse des option de la ligne de commande
	commandLineOptions.ParseMainParameters(argc, argv);

	// Initialisation des managers de message
	InitializeMessageManagers();

	// Ouverture du fichier de commande en entree
	if (bOk and commandFile.GetInputCommandFileName() != "")
		bOk = commandFile.OpenInputCommandFile();

	// Ouverture du fichier de commande en sortie
	if (bOk and commandFile.GetOutputCommandFileName() != "")
	{
		bOk = commandFile.OpenOutputCommandFile();

		// En mode Graphic on renvoie systematiquement true : on permet de lancer l'outil meme si on ne peut
		// pas ecrire dans le fichier de log (les options -e et -o sont passees par defaut a l'outil dans les scripts de
		// lancement bash ou cmd) En mode Textual on est plus strict (utilisation via python, java ou sur cluster)
		if (not IsBatchMode())
			bOk = true;
	}

	// Si il y a eu un pbm lors de l'excution d'une methode, on sort en erreur
	if (not bOk)
		GlobalExit();

	// Ecriture d'un en-tete dans le fichier des commandes
	WriteOutputCommand("", "", CurrentTimestamp());
	WriteOutputCommand("", "", commandLineOptions.GetCommandName());
	if (commandFile.GetPrintOutputInConsole())
	{
		sMessage = "Output command file\n";
		sMessage += "//\n";
		sMessage += "//This file contains recorded commands, that can be replayed.\n";
		sMessage += "//Commands are based on user interactions:\n";
		sMessage += "//\tfield update\n";
		sMessage += "//\tlist item selection\n";
		sMessage += "//\tmenu action\n";
		sMessage += "//Every command can be commented, using //.\n";
		sMessage += "//For example, commenting the last Exit command will allow other\n";
		sMessage += "//user interactions, after the commands have been replayed.\n";
		sMessage += "//\n";
		sMessage += "//";
	}
	WriteOutputCommand("", "", sMessage);

	// Choix de l'interface
	if (IsBatchMode())
		SetUIMode(UIObject::Textual);
	else
	{
		bOk = SetUIMode(UIObject::Graphic);
		if (not bOk and not GetTextualInteractiveModeAllowed())
		{
			Global::AddError("", "",
					 "Unable to load GUI, use -b and -i flags to launch khiops in batch mode "
					 "(khiops -h for help)");
			GlobalExit();
		}
	}
}

boolean UIObject::IsBatchMode()
{
	return bBatchMode;
}

void UIObject::SetFastExitMode(boolean bValue)
{
	bFastExitMode = bValue;
}

boolean UIObject::GetFastExitMode()
{
	return bFastExitMode;
}

void UIObject::SetTextualInteractiveModeAllowed(boolean bValue)
{
	bTextualInteractiveModeAllowed = bValue;
}

boolean UIObject::GetTextualInteractiveModeAllowed()
{
	return bTextualInteractiveModeAllowed;
}

boolean UIObject::InputCommand(const ALString& sFileName)
{
	commandFile.SetInputCommandFileName(sFileName);
	return true;
}

boolean UIObject::OutputCommand(const ALString& sFileName)
{
	commandFile.SetOutputCommandFileName(sFileName);
	return true;
}

boolean UIObject::JsonCommand(const ALString& sFileName)
{
	commandFile.SetInputParameterFileName(sFileName);
	return true;
}

boolean UIObject::ReplaceCommand(const ALString& sSearchReplacePattern)
{
	int nEqualPosition;
	ALString sSearchValue;
	ALString sReplaceValue;
	boolean bOk;

	bOk = true;
	// On ne traite que les pattern ayant une search value non vide
	nEqualPosition = sSearchReplacePattern.Find(':');
	if (nEqualPosition > 0)
	{
		sSearchValue = sSearchReplacePattern.Left(nEqualPosition);
		sReplaceValue = sSearchReplacePattern.Right(sSearchReplacePattern.GetLength() - nEqualPosition - 1);
		commandFile.AddInputSearchReplaceValues(sSearchValue, sReplaceValue);
	}
	else
	{
		bOk = false;
		Global::AddError("flag", "-r",
				 "Missing separator ':' between search and replace values in '" +
				     sSearchReplacePattern + "'");
	}

	return bOk;
}

boolean UIObject::BatchCommand(const ALString& sParameter)
{
	assert(sParameter == "");

	bBatchMode = true;
	return true;
}

boolean UIObject::ErrorCommand(const ALString& sErrorLog)
{
	boolean bOk;
	sErrorLogFileName = sErrorLog;
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sErrorLogFileName, sLocalErrorLogFileName);
	if (bOk)
	{
		Global::SetErrorLogFileName(sLocalErrorLogFileName);
	}

	// En mode Graphic on renvoie systematiquement true : on permet de lancer l'outil meme si on ne peut
	// pas ecrire dans le fichier de log (les options -e et -o sont passees par defaut a l'outil dans les scripts de
	// lancement bash ou cmd) En mode Textual on est plus strict (utilisation via python, java ou sur cluster)
	return not IsBatchMode() or bOk;
}

boolean UIObject::TaskProgressionCommand(const ALString& sTaskFile)
{
	// TODO est-ce qu'on peut mettre le fichier sur HDFS
	TaskProgression::SetTaskProgressionLogFileName(sTaskFile);
	return true;
}

void UIObject::CleanCommandLineManagement()
{
	// Par defaut, on rehoue les commande
	bNoReplayMode = false;

	// Fermeture des fichiers de commande en entree et sortie
	commandFile.CloseCommandFiles();

	// Reinitialisation du parametrage des fichiers de commande
	commandFile.Reset();

	// Fermeture du fichier d'erreur
	if (Global::GetErrorLogFileName() != "")
	{
		// Fermeture du fichier de log (la reinitialisation entraine la fermeture du fichier)
		Global::SetErrorLogFileName("");

		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sErrorLogFileName, sLocalErrorLogFileName);
	}
}

void UIObject::ExitHandlerCleanCommandLineManagement(int nExitCode)
{
	CleanCommandLineManagement();
}

boolean UIObject::ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue)
{
	return commandFile.ReadInputCommand(svIdentifierPath, sValue);
}

void UIObject::WriteOutputCommand(const ALString& sIdentifierPath, const ALString& sValue, const ALString& sLabel)
{
	commandFile.WriteOutputCommand(sIdentifierPath, sValue, sLabel);
}

void UIObject::WriteOutputFieldCommand(const ALString& sValue) const
{
	WriteOutputCommand(GetIdentifierPath(), sValue, GetUnformattedLabel());
}

void UIObject::WriteOutputActionCommand() const
{
	WriteOutputCommand(GetIdentifierPath(), "", GetUnformattedLabel());
}

void UIObject::InitializeMessageManagers()
{
	// Parametrage en mode graphique
	if (GetUIMode() == Graphic)
	{
		// Parametrage de l'affichage des messages d'erreur
		if (Error::GetDisplayErrorFunction() == NULL or
		    Error::GetDisplayErrorFunction() == Error::GetDefaultDisplayErrorFunction())
			Error::SetDisplayErrorFunction(UIObjectDisplayErrorFunction);

		// Parametrage du gestionnaire de suivi de progression
		if (TaskProgression::GetManager() == NULL and not TaskProgression::IsStarted())
			TaskProgression::SetManager(UITaskProgression::GetManager());
	}
	// Parametrage en mode textuel
	else
	{
		assert(GetUIMode() == Textual);

		// Affichage dans la fenetre de shell, ou pas d'affichage si un fichier de log est specifie
		if (Global::GetErrorLogFileName() == "")
		{
			if (bBatchMode)
				Error::SetDisplayErrorFunction(Error::GetDefaultDisplayErrorFunction());
		}
		else
			Error::SetDisplayErrorFunction(NULL);

		// Aucun affichage d'avancement des taches
		if (not TaskProgression::IsStarted())
			TaskProgression::SetManager(NULL);
	}
}

void UIObjectDisplayErrorFunction(const Error* e)
{
	ALString sDisplayMessage;
	sDisplayMessage = Error::BuildDisplayMessage(e);
	UIObject::DisplayMessage(sDisplayMessage);
}

//////////////////////////////
// Variables de classe

int UIObject::nUIMode = Textual;
int UIObject::nInstanceNumber = 0;
boolean UIObject::bVerboseCommandReplay = false;
boolean UIObject::bBatchMode = false;
boolean UIObject::bFastExitMode = true;
boolean UIObject::bTextualInteractiveModeAllowed = false;
ObjectDictionary UIObject::odListIndexCommands;
ALString UIObject::sIconImageJarPath;
void* UIObject::jvmHandle = NULL;
boolean UIObject::bIsJVMLoaded = false;
ALString UIObject::sLocalErrorLogFileName;
ALString UIObject::sErrorLogFileName;
ALString UIObject::sTaskProgressionLogFileName;
CommandLine UIObject::commandLineOptions;
boolean UIObject::bNoReplayMode = false;
CommandFile UIObject::commandFile;
