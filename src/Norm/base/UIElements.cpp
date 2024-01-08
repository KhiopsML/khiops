// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

//////////////////////////////////////////////////////////////////////////

const ALString UIElement::GetClassLabel() const
{
	return "UI Element";
}

boolean UIElement::GetTriggerRefresh() const
{
	return bTriggerRefresh;
}

void UIElement::SetTriggerRefresh(boolean bValue)
{
	bTriggerRefresh = bValue;
}

UIElement::UIElement()
{
	bTriggerRefresh = false;
}

boolean UIElement::IsElement() const
{
	return true;
}

boolean UIElement::IsListable() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////

UIStringElement::UIStringElement()
{
	nMinLength = 0;
	nMaxLength = INT_MAX;
}

UIStringElement::~UIStringElement() {}

boolean UIStringElement::Check() const
{
	boolean bOk = true;
	int nPos;
	ALString sLines;
	ALString sColumns;
	int nLines;
	int nColumns;
	ALString sTmp;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "TextField" and GetStyle() != "ComboBox" and
	    GetStyle() != "EditableComboBox" and GetStyle() != "HelpedComboBox" and GetStyle() != "FileChooser" and
	    GetStyle() != "DirectoryChooser" and GetStyle() != "Password" and GetStyle() != "RadioButton" and
	    GetStyle() != "ImageComboBox" and GetStyle() != "TextArea" and GetStyle() != "FormattedLabel" and
	    GetStyle() != "SelectableLabel" and GetStyle() != "UriLabel")
		AddWarning("Le style " + GetStyle() + " est inconnu");

	// Verification des longueurs
	if (GetMinLength() < 0)
	{
		AddError(sTmp + "Longueur minimum (" + IntToString(GetMinLength()) + ") incorrecte");
		bOk = false;
	}
	if (GetMinLength() > GetMaxLength())
	{
		AddError(sTmp + "La longueur maximum (" + IntToString(GetMaxLength()) +
			 ") est inferieure a la longueur minimum (" + IntToString(GetMinLength()) + ")");
		bOk = false;
	}

	// Verification de la valeur par defaut
	if (not CheckValue(GetDefaultValue()))
	{
		AddError(sTmp + "La valeur par defaut (" + GetDefaultValue() + ") est incorrecte");
		bOk = false;
	}

	// Verification du parametrage
	if (bOk and GetStyle() == "TextArea" and GetParameters() != "")
	{
		// Tentative de parsing des deux valeurs Lines et columns
		nPos = GetParameters().Find("\n");
		if (nPos == -1)
			bOk = false;
		if (bOk)
		{
			sLines = GetParameters().Left(nPos);
			sColumns = GetParameters().Right(GetParameters().GetLength() - nPos - 1);
			bOk = sColumns.Find('\n') == -1;
		}
		if (bOk)
		{
			nLines = StringToInt(sLines);
			nColumns = StringToInt(sColumns);
			bOk = bOk and sLines == IntToString(nLines);
			bOk = bOk and sColumns == IntToString(nColumns);
			bOk = bOk and (0 <= nLines and nLines <= 100);
			bOk = bOk and (0 <= nColumns and nColumns <= 500);
		}
		if (not bOk)
			AddError(sTmp + "Parametrage incorrect <" + GetParameters() + ">");
	}
	return bOk;
}

boolean UIStringElement::CheckValue(const ALString& sValue) const
{
	return GetMinLength() <= sValue.GetLength() and sValue.GetLength() <= GetMaxLength();
}

const ALString UIStringElement::GetClassLabel() const
{
	return "UI string Element";
}

const ALString UIStringElement::GetActualStyle() const
{
	if (GetStyle() != "")
		return GetStyle();
	else
		return "TextField";
}

void UIStringElement::SetDefaultValue(const ALString& sValue)
{
	sDefaultValue = sValue;
}

const ALString& UIStringElement::GetDefaultValue() const
{
	return sDefaultValue;
}

void UIStringElement::SetMinLength(int nValue)
{
	nMinLength = nValue;
}

int UIStringElement::GetMinLength() const
{
	return nMinLength;
}

void UIStringElement::SetMaxLength(int nValue)
{
	nMaxLength = nValue;
}

int UIStringElement::GetMaxLength() const
{
	return nMaxLength;
}

int UIStringElement::GetDataType() const
{
	return String;
}

Object* UIStringElement::ValueContainerCreate()
{
	return new StringVector;
}

void UIStringElement::ValueContainerAddItem(Object* container)
{
	StringVector* values;

	require(container != NULL);

	values = cast(StringVector*, container);
	values->Add(sDefaultValue);
}

void UIStringElement::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	StringVector* values;
	int i;

	// Agrandissement du container
	values = cast(StringVector*, container);
	assert(nIndex <= values->GetSize());
	values->SetSize(values->GetSize() + 1);

	// Recopie des valeurs
	for (i = values->GetSize() - 1; i > nIndex; i--)
		values->SetAt(i, values->GetAt(i - 1));
	values->SetAt(nIndex, sDefaultValue);
}

void UIStringElement::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	StringVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(StringVector*, container);
	assert(nIndex < values->GetSize());

	// Recopie des valeurs
	for (i = nIndex; i < values->GetSize() - 1; i++)
		values->SetAt(i, values->GetAt(i + 1));

	// Retrecissement du container
	values->SetSize(values->GetSize() - 1);
}

void UIStringElement::TextualCardDisplay(Object* container, int nIndex)
{
	StringVector* values;
	ALString sValue;
	boolean bOk;
	ALString sBuffer;
	ALString sBuildLabel;
	ALString sErrorLabel;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(StringVector*, container);
	assert(nIndex < values->GetSize());
	sValue = values->GetAt(nIndex);

	// Preparation du libelle
	sBuildLabel = GetLabel();
	if (sValue != "")
		sBuildLabel = sBuildLabel + " [" + sValue + "]";
	sBuildLabel += ":";

	// Libelle et valeur uniquement si non editable
	if (GetEditable() == false)
		TextualDisplayOutput(sBuildLabel + "\n");
	else
	{
		// Attente de la reponse
		bOk = false;
		sErrorLabel = "\t(length >= ";
		sErrorLabel += IntToString(nMinLength);
		sErrorLabel += ")\n";
		while (not bOk)
		{
			TextualDisplayOutput(sBuildLabel);
			sBuffer = TextualReadInput();
			if (sBuffer.GetLength() == 0 and sValue.GetLength() >= nMinLength)
				bOk = true;
			else if (sBuffer.GetLength() < nMinLength)
				TextualDisplayOutput(sErrorLabel);
			else
			{
				// Diminution eventuelle de la taille
				if (sBuffer.GetLength() > nMaxLength)
					sValue = sBuffer.Left(nMaxLength);
				else
					sValue = sBuffer;
				values->SetAt(nIndex, sValue);
				WriteOutputFieldCommand(GetFieldStringValue(values, nIndex));

				// Declenchement d'un refresh selon le parametree du champ
				if (GetTriggerRefresh())
				{
					UIUnit* uiUnit;
					uiUnit = cast(UIUnit*, GetParent());
					uiUnit->ExecuteUserActionAt("Refresh");
				}
				bOk = true;
			}
		}
	}
}

void UIStringElement::TextualListDisplay(Object* container, int nIndex)
{
	StringVector* values;
	ALString sValue;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(StringVector*, container);
	assert(nIndex < values->GetSize());
	sValue = values->GetAt(nIndex);

	// Affichage de la valeur uniquement
	TextualDisplayOutput(sValue);
}

void UIStringElement::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jstring defaultValue;
	jint minLength;
	jint maxLength;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUIUnit
	cls = GraphicGetGUIUnitID();

	// Recherche de la methode addRangedStringField
	mid = GraphicGetMethodID(cls, "GUIUnit", "addRangedStringField",
				 "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	defaultValue = ToJstring(env, sDefaultValue);
	minLength = nMinLength;
	maxLength = nMaxLength;
	style = ToJstring(env, GetActualStyle());

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, defaultValue, minLength, maxLength, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(defaultValue);
	env->DeleteLocalRef(style);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

const char* const UIStringElement::GetFieldStringValue(Object* container, int nIndex)
{
	StringVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(StringVector*, container);
	assert(nIndex < values->GetSize());
	return values->GetAt(nIndex);
}

void UIStringElement::SetFieldStringValue(Object* container, int nIndex, const char* const sValue)
{
	StringVector* values;
	ALString sNewValue;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(StringVector*, container);
	assert(nIndex < values->GetSize());

	// Retaillage eventuel si trop long
	sNewValue = sValue;
	if (sNewValue.GetLength() > nMaxLength)
		sNewValue = sNewValue.Left(nMaxLength);
	values->SetAt(nIndex, sNewValue);
}

UIElement* UIStringElement::CloneAsElement() const
{
	UIStringElement* clone;

	clone = new UIStringElement;
	clone->SetLabel(GetLabel());
	clone->SetIdentifier(GetIdentifier());
	clone->SetHelpText(GetHelpText());
	clone->SetStyle(GetStyle());
	clone->SetVisible(GetVisible());
	clone->SetDefaultValue(GetDefaultValue());
	clone->SetMinLength(GetMinLength());
	clone->SetMaxLength(GetMaxLength());
	clone->SetParameters(GetParameters());
	clone->SetTriggerRefresh(GetTriggerRefresh());
	clone->SetShortCut(GetShortCut());

	return clone;
}

//////////////////////////////////////////////////////////////////////////

UIBooleanElement::UIBooleanElement()
{
	bDefaultValue = true;
}

UIBooleanElement::~UIBooleanElement() {}

boolean UIBooleanElement::Check() const
{
	boolean bOk = true;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "TextField" and GetStyle() != "CheckBox" and GetStyle() != "ComboBox" and
	    GetStyle() != "RadioButton")
		AddWarning("Le style " + GetStyle() + " est inconnu");

	return bOk;
}

const ALString UIBooleanElement::GetClassLabel() const
{
	return "UI boolean Element";
}

const ALString UIBooleanElement::GetActualStyle() const
{
	if (GetStyle() != "")
		return GetStyle();
	else
		return "CheckBox";
}

void UIBooleanElement::SetDefaultValue(boolean bValue)
{
	bDefaultValue = bValue;
}

boolean UIBooleanElement::GetDefaultValue() const
{
	return bDefaultValue;
}

int UIBooleanElement::GetDataType() const
{
	return Boolean;
}

Object* UIBooleanElement::ValueContainerCreate()
{
	return new IntVector;
}

void UIBooleanElement::ValueContainerAddItem(Object* container)
{
	IntVector* values;

	require(container != NULL);

	values = cast(IntVector*, container);
	values->SetSize(values->GetSize() + 1);
	values->SetAt(values->GetSize() - 1, bDefaultValue);
}

void UIBooleanElement::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	// Agrandissement du container
	values = cast(IntVector*, container);
	assert(nIndex <= values->GetSize());
	values->SetSize(values->GetSize() + 1);

	// Recopie des valeurs
	for (i = values->GetSize() - 1; i > nIndex; i--)
		values->SetAt(i, values->GetAt(i - 1));
	values->SetAt(nIndex, bDefaultValue);
}

void UIBooleanElement::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Recopie des valeurs
	for (i = nIndex; i < values->GetSize() - 1; i++)
		values->SetAt(i, values->GetAt(i + 1));

	// Retrecissement du container
	values->SetSize(values->GetSize() - 1);
}

void UIBooleanElement::TextualCardDisplay(Object* container, int nIndex)
{
	IntVector* values;
	char cYes;
	boolean bOk;
	ALString sBuffer;
	ALString sBuildLabel;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Preparation du libelle
	sBuildLabel = GetLabel() + "(y/n)";
	if (values->GetAt(nIndex) == false)
		sBuildLabel = sBuildLabel + " [n]:";

	// Libelle et valeur uniquement si non editable
	if (GetEditable() == false)
		TextualDisplayOutput(sBuildLabel + "\n");
	else
	{
		// Attente de la reponse
		bOk = false;
		while (not bOk)
		{
			TextualDisplayOutput(sBuildLabel);
			sBuffer = TextualReadInput();
			if (sBuffer.GetLength() == 0)
				bOk = true;
			else
			{
				sscanf(sBuffer, "%c", &cYes);
				if (cYes == 'y' or cYes == 'n')
				{
					values->SetAt(nIndex, cYes == 'y');
					WriteOutputFieldCommand(GetFieldStringValue(values, nIndex));

					// Declenchement d'un refresh selon le parametree du champ
					if (GetTriggerRefresh())
					{
						UIUnit* uiUnit;
						uiUnit = cast(UIUnit*, GetParent());
						uiUnit->ExecuteUserActionAt("Refresh");
					}
					bOk = true;
				}
			}
		}
	}
}

void UIBooleanElement::TextualListDisplay(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Affichage de la valeur uniquement
	if (values->GetAt(nIndex) == false)
		TextualDisplayOutput("n");
	else
		TextualDisplayOutput("y");
}

void UIBooleanElement::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jboolean defaultValue;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUIUnit
	cls = GraphicGetGUIUnitID();

	// Recherche de la methode addBooleanField
	mid = GraphicGetMethodID(cls, "GUIUnit", "addBooleanField",
				 "(Ljava/lang/String;Ljava/lang/String;ZLjava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	defaultValue = (jboolean)bDefaultValue;
	style = ToJstring(env, GetActualStyle());

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, defaultValue, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

const char* const UIBooleanElement::GetFieldStringValue(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	return BooleanToString((boolean)values->GetAt(nIndex));
}

void UIBooleanElement::SetFieldStringValue(Object* container, int nIndex, const char* const sValue)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	if (not strcmp(sValue, "true"))
		values->SetAt(nIndex, true);
	else
		values->SetAt(nIndex, false);
}

UIElement* UIBooleanElement::CloneAsElement() const
{
	UIBooleanElement* clone;

	clone = new UIBooleanElement;
	clone->SetLabel(GetLabel());
	clone->SetIdentifier(GetIdentifier());
	clone->SetHelpText(GetHelpText());
	clone->SetStyle(GetStyle());
	clone->SetVisible(GetVisible());
	clone->SetDefaultValue(GetDefaultValue());
	clone->SetParameters(GetParameters());
	clone->SetTriggerRefresh(GetTriggerRefresh());
	clone->SetShortCut(GetShortCut());

	return clone;
}

//////////////////////////////////////////////////////////////////////////

UICharElement::UICharElement()
{
	cDefaultValue = ' ';
}

UICharElement::~UICharElement() {}

boolean UICharElement::Check() const
{
	boolean bOk = true;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "TextField" and GetStyle() != "ComboBox" and
	    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton")
		AddWarning("Le style " + GetStyle() + " est inconnu");

	return bOk;
}

const ALString UICharElement::GetClassLabel() const
{
	return "UI char Element";
}

const ALString UICharElement::GetActualStyle() const
{
	if (GetStyle() != "")
		return GetStyle();
	else
		return "TextField";
}

void UICharElement::SetDefaultValue(char cValue)
{
	cDefaultValue = cValue;
}

char UICharElement::GetDefaultValue() const
{
	return cDefaultValue;
}

int UICharElement::GetDataType() const
{
	return Char;
}

Object* UICharElement::ValueContainerCreate()
{
	return new IntVector;
}

void UICharElement::ValueContainerAddItem(Object* container)
{
	IntVector* values;

	require(container != NULL);

	values = cast(IntVector*, container);
	values->SetSize(values->GetSize() + 1);
	values->SetAt(values->GetSize() - 1, cDefaultValue);
}

void UICharElement::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	// Agrandissement du container
	values = cast(IntVector*, container);
	assert(nIndex <= values->GetSize());
	values->SetSize(values->GetSize() + 1);

	// Recopie des valeurs
	for (i = values->GetSize() - 1; i > nIndex; i--)
		values->SetAt(i, values->GetAt(i - 1));
	values->SetAt(nIndex, cDefaultValue);
}

void UICharElement::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Recopie des valeurs
	for (i = nIndex; i < values->GetSize() - 1; i++)
		values->SetAt(i, values->GetAt(i + 1));

	// Retrecissement du container
	values->SetSize(values->GetSize() - 1);
}

void UICharElement::TextualCardDisplay(Object* container, int nIndex)
{
	IntVector* values;
	ALString sBuffer;
	ALString sBuildLabel;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Preparation du libelle
	sBuildLabel = GetLabel();
	if (values->GetAt(nIndex) != '\0')
		sBuildLabel = sBuildLabel + " [" + CharToString((char)values->GetAt(nIndex)) + "]:";

	// Libelle et valeur uniquement si non editable
	if (GetEditable() == false)
		TextualDisplayOutput(sBuildLabel + "\n");
	else
	{
		// Attente de la reponse
		TextualDisplayOutput(sBuildLabel);
		sBuffer = TextualReadInput();
		if (sBuffer.GetLength() != 0)
		{
			values->SetAt(nIndex, sBuffer.GetAt(0));
			WriteOutputFieldCommand(GetFieldStringValue(values, nIndex));

			// Declenchement d'un refresh selon le parametree du champ
			if (GetTriggerRefresh())
			{
				UIUnit* uiUnit;
				uiUnit = cast(UIUnit*, GetParent());
				uiUnit->ExecuteUserActionAt("Refresh");
			}
		}
	}
}

void UICharElement::TextualListDisplay(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Affichage de la valeur uniquement
	TextualDisplayOutput(CharToString((char)values->GetAt(nIndex)));
}

void UICharElement::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jchar defaultValue;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUIUnit
	cls = GraphicGetGUIUnitID();

	// Recherche de la methode addCharField
	mid = GraphicGetMethodID(cls, "GUIUnit", "addCharField",
				 "(Ljava/lang/String;Ljava/lang/String;CLjava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	defaultValue = cDefaultValue;
	style = ToJstring(env, GetActualStyle());

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, defaultValue, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

const char* const UICharElement::GetFieldStringValue(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	return CharToString((char)values->GetAt(nIndex));
}

void UICharElement::SetFieldStringValue(Object* container, int nIndex, const char* const sValue)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	if (strlen(sValue) > 0)
		values->SetAt(nIndex, sValue[0]);
	else
		values->SetAt(nIndex, ' ');
}

UIElement* UICharElement::CloneAsElement() const
{
	UICharElement* clone;

	clone = new UICharElement;
	clone->SetLabel(GetLabel());
	clone->SetIdentifier(GetIdentifier());
	clone->SetHelpText(GetHelpText());
	clone->SetStyle(GetStyle());
	clone->SetVisible(GetVisible());
	clone->SetDefaultValue(GetDefaultValue());
	clone->SetParameters(GetParameters());
	clone->SetTriggerRefresh(GetTriggerRefresh());
	clone->SetShortCut(GetShortCut());

	return clone;
}

//////////////////////////////////////////////////////////////////////////

UIIntElement::UIIntElement()
{
	nDefaultValue = 0;
	nMinValue = -INT_MAX;
	nMaxValue = INT_MAX;
}

UIIntElement::~UIIntElement() {}

boolean UIIntElement::Check() const
{
	boolean bOk = true;
	ALString sTmp;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "TextField" and GetStyle() != "ComboBox" and
	    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton" and GetStyle() != "Spinner" and
	    GetStyle() != "Slider")
		AddWarning("Le style " + GetStyle() + " est inconnu");

	// Verification des valeurs min et max
	if (GetMinValue() > GetMaxValue())
	{
		AddError(sTmp + "La valeur maximum (" + IntToString(GetMaxValue()) +
			 ") est inferieure a la valeur minimum (" + IntToString(GetMinValue()) + ")");
		bOk = false;
	}

	// Verification de la valeur par defaut
	if (not CheckValue(GetDefaultValue()))
	{
		AddError(sTmp + "La valeur par defaut (" + IntToString(GetDefaultValue()) + ") est incorrecte");
		bOk = false;
	}

	return bOk;
}

boolean UIIntElement::CheckValue(int nValue) const
{
	return GetMinValue() <= nValue and nValue <= GetMaxValue();
}

const ALString UIIntElement::GetClassLabel() const
{
	return "UI int Element";
}

const ALString UIIntElement::GetActualStyle() const
{
	if (GetStyle() != "")
		return GetStyle();
	else
		return "TextField";
}

void UIIntElement::SetDefaultValue(int nValue)
{
	nDefaultValue = nValue;
}

int UIIntElement::GetDefaultValue() const
{
	return nDefaultValue;
}

void UIIntElement::SetMinValue(int nValue)
{
	nMinValue = nValue;
}

int UIIntElement::GetMinValue() const
{
	return nMinValue;
}

void UIIntElement::SetMaxValue(int nValue)
{
	nMaxValue = nValue;
}

int UIIntElement::GetMaxValue() const
{
	return nMaxValue;
}

int UIIntElement::GetDataType() const
{
	return Int;
}

Object* UIIntElement::ValueContainerCreate()
{
	return new IntVector;
}

void UIIntElement::ValueContainerAddItem(Object* container)
{
	IntVector* values;

	require(container != NULL);

	values = cast(IntVector*, container);
	values->SetSize(values->GetSize() + 1);
	values->SetAt(values->GetSize() - 1, nDefaultValue);
}

void UIIntElement::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	// Agrandissement du container
	values = cast(IntVector*, container);
	assert(nIndex <= values->GetSize());
	values->SetSize(values->GetSize() + 1);

	// Recopie des valeurs
	for (i = values->GetSize() - 1; i > nIndex; i--)
		values->SetAt(i, values->GetAt(i - 1));
	values->SetAt(nIndex, nDefaultValue);
}

void UIIntElement::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	IntVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Recopie des valeurs
	for (i = nIndex; i < values->GetSize() - 1; i++)
		values->SetAt(i, values->GetAt(i + 1));

	// Retrecissement du container
	values->SetSize(values->GetSize() - 1);
}

void UIIntElement::TextualCardDisplay(Object* container, int nIndex)
{
	IntVector* values;
	int nInput;
	boolean bOk;
	ALString sBuffer;
	ALString sBuildLabel;
	ALString sMin;
	ALString sMax;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Preparation du libelle
	sBuildLabel = GetLabel();
	if (nMinValue == -INT_MAX)
		sMin = "-INF";
	else
		sMin = IntToString(nMinValue);
	if (nMaxValue == INT_MAX)
		sMax = "INF";
	else
		sMax = IntToString(nMaxValue);
	if (nMinValue != -INT_MAX or nMaxValue != INT_MAX)
		sBuildLabel += " (" + sMin + " to " + sMax + ")";
	if (values->GetAt(nIndex) != nMinValue)
		sBuildLabel = sBuildLabel + " [" + IntToString(values->GetAt(nIndex)) + "]";
	sBuildLabel += ":";

	// Libelle et valeur uniquement si non editable
	if (GetEditable() == false)
		TextualDisplayOutput(sBuildLabel + "\n");
	else
	{
		// Attente de la reponse
		bOk = false;
		while (not bOk)
		{
			TextualDisplayOutput(sBuildLabel);
			sBuffer = TextualReadInput();
			if (sBuffer.GetLength() == 0)
				bOk = true;
			else
			{
				nInput = atoi(sBuffer);
				if (nMinValue <= nInput and nInput <= nMaxValue)
				{
					values->SetAt(nIndex, nInput);
					WriteOutputFieldCommand(GetFieldStringValue(values, nIndex));

					// Declenchement d'un refresh selon le parametree du champ
					if (GetTriggerRefresh())
					{
						UIUnit* uiUnit;
						uiUnit = cast(UIUnit*, GetParent());
						uiUnit->ExecuteUserActionAt("Refresh");
					}
					bOk = true;
				}
			}
		}
	}
}

void UIIntElement::TextualListDisplay(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	// Affichage de la valeur uniquement
	TextualDisplayOutput(IntToString(values->GetAt(nIndex)));
}

void UIIntElement::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jint defaultValue;
	jint minValue;
	jint maxValue;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUIUnit
	cls = GraphicGetGUIUnitID();

	// Recherche de la methode addRangedIntField
	mid = GraphicGetMethodID(cls, "GUIUnit", "addRangedIntField",
				 "(Ljava/lang/String;Ljava/lang/String;IIILjava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	defaultValue = nDefaultValue;
	minValue = nMinValue;
	maxValue = nMaxValue;
	style = ToJstring(env, GetActualStyle());

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, defaultValue, minValue, maxValue, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

const char* const UIIntElement::GetFieldStringValue(Object* container, int nIndex)
{
	IntVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	return IntToString(values->GetAt(nIndex));
}

void UIIntElement::SetFieldStringValue(Object* container, int nIndex, const char* const sValue)
{
	IntVector* values;
	int nValue;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(IntVector*, container);
	assert(nIndex < values->GetSize());

	nValue = atoi(sValue);
	if (nValue < nMinValue)
		values->SetAt(nIndex, nMinValue);
	else if (nValue > nMaxValue)
		values->SetAt(nIndex, nMaxValue);
	else
		values->SetAt(nIndex, nValue);
}

UIElement* UIIntElement::CloneAsElement() const
{
	UIIntElement* clone;

	clone = new UIIntElement;
	clone->SetLabel(GetLabel());
	clone->SetIdentifier(GetIdentifier());
	clone->SetHelpText(GetHelpText());
	clone->SetStyle(GetStyle());
	clone->SetVisible(GetVisible());
	clone->SetDefaultValue(GetDefaultValue());
	clone->SetMinValue(GetMinValue());
	clone->SetMaxValue(GetMaxValue());
	clone->SetParameters(GetParameters());
	clone->SetTriggerRefresh(GetTriggerRefresh());
	clone->SetShortCut(GetShortCut());

	return clone;
}

//////////////////////////////////////////////////////////////////////////

UIDoubleElement::UIDoubleElement()
{
	dDefaultValue = 0;
	dMinValue = -DBL_MAX;
	dMaxValue = DBL_MAX;
}

UIDoubleElement::~UIDoubleElement() {}

boolean UIDoubleElement::Check() const
{
	boolean bOk = true;
	ALString sTmp;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "TextField" and GetStyle() != "ComboBox" and
	    GetStyle() != "EditableComboBox" and GetStyle() != "RadioButton" and GetStyle() != "Spinner")
		AddWarning("Le style " + GetStyle() + " est inconnu");

	// Verification des valeurs min et max
	if (GetMinValue() > GetMaxValue())
	{
		AddError(sTmp + "La valeur maximum (" + DoubleToString(GetMaxValue()) +
			 ") est inferieure a la valeur minimum (" + DoubleToString(GetMinValue()) + ")");
		bOk = false;
	}

	// Verification de la valeur par defaut
	if (not CheckValue(GetDefaultValue()))
	{
		AddError(sTmp + "La valeur par defaut (" + DoubleToString(GetDefaultValue()) + ") est incorrecte");
		bOk = false;
	}

	// Verification du parametre dans le cas spinner
	if (bOk and GetStyle() == "Spinner")
	{
		if (GetParameters() != "")
		{
			if (GetParameters().GetLength() > 1)
			{
				AddError(
				    sTmp +
				    "Le parametre pour un style Spinner doit etre specifie avec un seul caractere (" +
				    GetParameters() + ")");
				bOk = false;
			}
			else if (not isdigit(GetParameters().GetAt(0)))
			{
				AddError(sTmp + "Le parametre pour un style Spinner doit etre un chiffre (" +
					 GetParameters() + ")");
				bOk = false;
			}
		}
	}
	return bOk;
}

boolean UIDoubleElement::CheckValue(double dValue) const
{
	return GetMinValue() <= dValue and dValue <= GetMaxValue();
}

const ALString UIDoubleElement::GetClassLabel() const
{
	return "UI double Element";
}

const ALString UIDoubleElement::GetActualStyle() const
{
	if (GetStyle() != "")
		return GetStyle();
	else
		return "TextField";
}

void UIDoubleElement::SetDefaultValue(double dValue)
{
	dDefaultValue = dValue;
}

double UIDoubleElement::GetDefaultValue() const
{
	return dDefaultValue;
}

void UIDoubleElement::SetMinValue(double dValue)
{
	dMinValue = dValue;
}

double UIDoubleElement::GetMinValue() const
{
	return dMinValue;
}

void UIDoubleElement::SetMaxValue(double dValue)
{
	dMaxValue = dValue;
}

double UIDoubleElement::GetMaxValue() const
{
	return dMaxValue;
}

int UIDoubleElement::GetDataType() const
{
	return Double;
}

Object* UIDoubleElement::ValueContainerCreate()
{
	return new DoubleVector;
}

void UIDoubleElement::ValueContainerAddItem(Object* container)
{
	DoubleVector* values;

	require(container != NULL);

	values = cast(DoubleVector*, container);
	values->SetSize(values->GetSize() + 1);
	values->SetAt(values->GetSize() - 1, dDefaultValue);
}

void UIDoubleElement::ValueContainerInsertItemAt(Object* container, int nIndex)
{
	DoubleVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	// Agrandissement du container
	values = cast(DoubleVector*, container);
	assert(nIndex <= values->GetSize());
	values->SetSize(values->GetSize() + 1);

	// Recopie des valeurs
	for (i = values->GetSize() - 1; i > nIndex; i--)
		values->SetAt(i, values->GetAt(i - 1));
	values->SetAt(nIndex, dDefaultValue);
}

void UIDoubleElement::ValueContainerRemoveItemAt(Object* container, int nIndex)
{
	DoubleVector* values;
	int i;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(DoubleVector*, container);
	assert(nIndex < values->GetSize());

	// Recopie des valeurs
	for (i = nIndex; i < values->GetSize() - 1; i++)
		values->SetAt(i, values->GetAt(i + 1));

	// Retrecissement du container
	values->SetSize(values->GetSize() - 1);
}

void UIDoubleElement::TextualCardDisplay(Object* container, int nIndex)
{
	DoubleVector* values;
	double dInput;
	boolean bOk;
	ALString sBuffer;
	ALString sBuildLabel;
	ALString sMin;
	ALString sMax;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(DoubleVector*, container);
	assert(nIndex < values->GetSize());

	// Preparation du libelle
	sBuildLabel = GetLabel();
	if (dMinValue == -DBL_MAX)
		sMin = "-INF";
	else
		sMin = DoubleToString(dMinValue);
	if (dMaxValue == DBL_MAX)
		sMax = "INF";
	else
		sMax = DoubleToString(dMaxValue);
	if (dMinValue != -DBL_MAX or dMaxValue != DBL_MAX)
		sBuildLabel += " (" + sMin + " to " + sMax + ")";
	if (values->GetAt(nIndex) != dMinValue)
		sBuildLabel = sBuildLabel + " [" + DoubleToString(values->GetAt(nIndex)) + "]";
	sBuildLabel += ":";

	// Libelle et valeur uniquement si non editable
	if (GetEditable() == false)
		TextualDisplayOutput(sBuildLabel + "\n");
	else
	{
		// Attente de la reponse
		bOk = false;
		while (not bOk)
		{
			TextualDisplayOutput(sBuildLabel);
			sBuffer = TextualReadInput();
			if (sBuffer.GetLength() == 0)
				bOk = true;
			else
			{
				dInput = atof(sBuffer);
				if (dMinValue <= dInput and dInput <= dMaxValue)
				{
					values->SetAt(nIndex, dInput);
					WriteOutputFieldCommand(GetFieldStringValue(values, nIndex));

					// Declenchement d'un refresh selon le parametree du champ
					if (GetTriggerRefresh())
					{
						UIUnit* uiUnit;
						uiUnit = cast(UIUnit*, GetParent());
						uiUnit->ExecuteUserActionAt("Refresh");
					}
					bOk = true;
				}
			}
		}
	}
}

void UIDoubleElement::TextualListDisplay(Object* container, int nIndex)
{
	DoubleVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(DoubleVector*, container);
	assert(nIndex < values->GetSize());

	// Affichage de la valeur uniquement
	TextualDisplayOutput(DoubleToString(values->GetAt(nIndex)));
}

void UIDoubleElement::GraphicAddField(jobject guiUnit)
{
	JNIEnv* env;
	jclass cls;
	jmethodID mid;
	jstring fieldId;
	jstring label;
	jdouble defaultValue;
	jdouble minValue;
	jdouble maxValue;
	jstring style;

	require(guiUnit != NULL);

	env = GraphicGetJNIEnv();

	// Recherche de la classe Java GUIUnit
	cls = GraphicGetGUIUnitID();

	// Recherche de la methode addRangedDoubleField
	mid = GraphicGetMethodID(cls, "GUIUnit", "addRangedDoubleField",
				 "(Ljava/lang/String;Ljava/lang/String;DDDLjava/lang/String;)V");

	// Preparation des arguments
	fieldId = ToJstring(env, sIdentifier);
	label = ToJstring(env, GetLabel());
	defaultValue = dDefaultValue;
	minValue = dMinValue;
	maxValue = dMaxValue;
	style = ToJstring(env, GetActualStyle());

	// Appel de la methode
	env->CallVoidMethod(guiUnit, mid, fieldId, label, defaultValue, minValue, maxValue, style);

	// On avertit Java que les chaines sont liberables
	env->DeleteLocalRef(fieldId);
	env->DeleteLocalRef(label);
	env->DeleteLocalRef(style);

	// Liberation de la classe
	env->DeleteLocalRef(cls);
}

const char* const UIDoubleElement::GetFieldStringValue(Object* container, int nIndex)
{
	DoubleVector* values;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(DoubleVector*, container);
	assert(nIndex < values->GetSize());

	return DoubleToString(values->GetAt(nIndex));
}

void UIDoubleElement::SetFieldStringValue(Object* container, int nIndex, const char* const sValue)
{
	DoubleVector* values;
	double dValue;

	require(container != NULL);
	require(0 <= nIndex);

	values = cast(DoubleVector*, container);
	assert(nIndex < values->GetSize());

	dValue = atof(sValue);
	if (dValue < dMinValue)
		values->SetAt(nIndex, dMinValue);
	else if (dValue > dMaxValue)
		values->SetAt(nIndex, dMaxValue);
	else
		values->SetAt(nIndex, dValue);
}

UIElement* UIDoubleElement::CloneAsElement() const
{
	UIDoubleElement* clone;

	clone = new UIDoubleElement;
	clone->SetLabel(GetLabel());
	clone->SetIdentifier(GetIdentifier());
	clone->SetHelpText(GetHelpText());
	clone->SetStyle(GetStyle());
	clone->SetVisible(GetVisible());
	clone->SetDefaultValue(GetDefaultValue());
	clone->SetMinValue(GetMinValue());
	clone->SetMaxValue(GetMaxValue());
	clone->SetParameters(GetParameters());
	clone->SetTriggerRefresh(GetTriggerRefresh());
	clone->SetShortCut(GetShortCut());

	return clone;
}

//////////////////////////////////////////////////////////////////////////

UIAction::UIAction()
{
	actionMethod = NULL;
}

UIAction::~UIAction() {}

const ALString UIAction::GetClassLabel() const
{
	return "UI Action";
}

void UIAction::SetActionMethod(ActionMethod method)
{
	actionMethod = method;
}

ActionMethod UIAction::GetActionMethod() const
{
	return actionMethod;
}

void UIAction::SetAccelKey(const ALString& sValue)
{
	sAccelKey = sValue;
}

const ALString& UIAction::GetAccelKey() const
{
	return sAccelKey;
}

boolean UIAction::Check() const
{
	boolean bOk = true;
	ALString sTmp;

	// Verification du style
	if (GetStyle() != "" and GetStyle() != "Button" and GetStyle() != "SmallButton")
	{
		bOk = false;
		AddError("Le style " + GetStyle() + " est inconnu");
	}

	// Verification des parametres
	if (GetIdentifier() == "Exit" and GetParameters() != "")
	{
		bOk = false;
		AddError("Le parametre (" + GetParameters() + ") ne doit pas etre modifie pour l'action systeme Exit");
	}
	return bOk;
}
