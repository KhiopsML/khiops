// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTClassBuilderView.h"

KWMTClassBuilderView::KWMTClassBuilderView()
{
	// Parametrage general
	SetIdentifier("KWMTClassBuilderView");
	SetLabel("Multi-table dictionary builder");

	// Libelle de l'action OK, de construction de dictionnaire
	GetActionAt("OK")->SetLabel("Build multi-table dictionary");

	// Nom du nouveau dictionnaire
	AddStringField("MultiTableClassName", "Multi-table dictionary name", "");
	cast(UIStringElement*, GetFieldAt("MultiTableClassName"))->SetMaxLength(KWClass::GetNameMaxLength());

	// Nom de la variable de type table
	AddStringField("TableVariableName", "Table variable name", "");
	cast(UIStringElement*, GetFieldAt("TableVariableName"))->SetMaxLength(KWClass::GetNameMaxLength());

	// Nom du dictionaire secondaire
	AddStringField("SecondaryClassName", "Secondary dictionary name", "");
	GetFieldAt("SecondaryClassName")->SetEditable(false);

	// Creation d'une liste cachee des nom de classes en cours
	classNameList = new UIList;
	classNameList->AddStringField("Name", "Name", "");
	AddListField("Classes", "Existing dictionaries", classNameList);
	GetFieldAt("Classes")->SetEditable(false);

	// Info-bulles
	GetFieldAt("MultiTableClassName")->SetHelpText("Name of the multi-table dictionary to build.");
	GetFieldAt("TableVariableName")->SetHelpText("Name of of the Table variable in the multi-table dictionary.");
	GetFieldAt("SecondaryClassName")
	    ->SetHelpText("Name of secondary dictionary used as a Table in the multi-table dictionary.");
	classNameList->GetFieldAt("Name")->SetHelpText("Name of dictionary.");
	GetActionAt("OK")->SetHelpText("Build a root dictionary with a Table variable based on"
				       "\n the input dictionary, then save the dictionary file.");

	// Short cuts
	GetActionAt("OK")->SetShortCut('B');
}

KWMTClassBuilderView::~KWMTClassBuilderView() {}

void KWMTClassBuilderView::SetSecondaryDictionaryName(const ALString& sValue)
{
	sSecondaryDictionaryName = sValue;
}

const ALString& KWMTClassBuilderView::GetSecondaryDictionaryName() const
{
	return sSecondaryDictionaryName;
}

void KWMTClassBuilderView::InitDefaultParameters()
{
	ALString sDefaultMultiTableClassName;
	int nClass;
	KWClass* kwcClass;

	// Initialisation des parametres si presence d'un dictionnaire secondaire
	if (sSecondaryDictionaryName != "")
	{
		// Nom par defaut du dictionnaire multi-classes a construire
		sDefaultMultiTableClassName = "Root" + sSecondaryDictionaryName;
		sDefaultMultiTableClassName =
		    KWClassDomain::GetCurrentDomain()->BuildClassName(sDefaultMultiTableClassName);

		// Parametrage de la construction du dictionnaire
		SetStringValueAt("MultiTableClassName", sDefaultMultiTableClassName);
		SetStringValueAt("TableVariableName", "Records");
		SetStringValueAt("SecondaryClassName", sSecondaryDictionaryName);
	}

	// Reactualisation des specs de classes
	classNameList->RemoveAllItems();
	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);

		// Ajout du nom de la classe
		classNameList->InsertItemAt(classNameList->GetItemNumber());
		classNameList->SetStringValueAt("Name", kwcClass->GetName());
	}
}

boolean KWMTClassBuilderView::BuildMultiTableClass()
{
	boolean bOk = true;
	ALString sClassName;
	KWClass* kwcSecondaryClass;
	ALString sMultiTableClassName;
	ALString sTableVariableName;
	KWClass* kwcMultiTableClass;
	KWAttribute* initialAttribute;
	KWAttribute* attribute;
	int nKey;
	ObjectDictionary odKeys;

	// Verification de la specification du dictionnaire secondaire
	if (sSecondaryDictionaryName == "")
	{
		AddError("Missing secondary dictionary name");
		bOk = false;
	}

	// Verification de la specification du dictionnaire secondaire
	kwcSecondaryClass = KWClassDomain::GetCurrentDomain()->LookupClass(sSecondaryDictionaryName);
	if (bOk and kwcSecondaryClass == NULL)
	{
		AddError("Secondary dictionary " + sSecondaryDictionaryName + " does not exist");
		bOk = false;
	}

	// Erreur s'il s'agit d'un dictionnaire racine
	if (bOk and kwcSecondaryClass->GetRoot())
	{
		AddError("Secondary dictionary " + sSecondaryDictionaryName + " should not be a Root dictionary");
		bOk = false;
	}

	// Erreur si le nom du dictionnaire a construire n'est pas specifie
	sMultiTableClassName = GetStringValueAt("MultiTableClassName");
	if (bOk and sMultiTableClassName == "")
	{
		AddError("Missing multi-table dictionary name");
		bOk = false;
	}

	// Erreur si le nom du dictionnaire a construire n'est pas specifie
	if (bOk and KWClassDomain::GetCurrentDomain()->LookupClass(sMultiTableClassName) != NULL)
	{
		AddError("Dictionary " + sMultiTableClassName + " already exists");
		bOk = false;
	}

	// Erreur si le nom de la variable Table n'est pas specifie
	sTableVariableName = GetStringValueAt("TableVariableName");
	if (bOk and sTableVariableName == "")
	{
		AddError("Missing table variable name");
		bOk = false;
	}

	// Erreur si le nom de la variable Table est utilise comme nom de cle
	if (bOk)
	{
		check(kwcSecondaryClass);
		for (nKey = 0; nKey < kwcSecondaryClass->GetKeyAttributeNumber(); nKey++)
		{
			if (kwcSecondaryClass->GetKeyAttributeNameAt(nKey) == sTableVariableName)
			{
				AddError("Table variable " + sTableVariableName +
					 " is already used as a key attribute");
				bOk = false;
				break;
			}
		}
	}

	// Construction du dictionnaire
	kwcMultiTableClass = NULL;
	if (bOk)
	{
		assert(KWClassDomain::GetCurrentDomain()->LookupClass(sMultiTableClassName) == NULL);

		// Creation du dictionnaire multi-tables
		kwcMultiTableClass = new KWClass;
		kwcMultiTableClass->SetName(sMultiTableClassName);

		// Parametrage des champs cle et memorisation dans un dictionnaire
		kwcMultiTableClass->SetKeyAttributeNumber(kwcSecondaryClass->GetKeyAttributeNumber());
		for (nKey = 0; nKey < kwcSecondaryClass->GetKeyAttributeNumber(); nKey++)
		{
			kwcMultiTableClass->SetKeyAttributeNameAt(nKey, kwcSecondaryClass->GetKeyAttributeNameAt(nKey));
			odKeys.SetAt(kwcSecondaryClass->GetKeyAttributeNameAt(nKey), kwcMultiTableClass);
		}

		// Creation des attributs cles dans le meme ordre que dans le dictionnaire secondaire
		initialAttribute = kwcSecondaryClass->GetHeadAttribute();
		while (initialAttribute != NULL)
		{
			// Ajout si attribut cle
			if (odKeys.Lookup(initialAttribute->GetName()))
			{
				attribute = new KWAttribute;
				attribute->SetName(initialAttribute->GetName());
				attribute->SetType(initialAttribute->GetType());
				kwcMultiTableClass->InsertAttribute(attribute);
			}

			// Attribut suivant
			kwcSecondaryClass->GetNextAttribute(initialAttribute);
		}

		// Ajout d'un attribut Table
		attribute = new KWAttribute;
		attribute->SetName(kwcMultiTableClass->BuildAttributeName(sTableVariableName));
		attribute->SetType(KWType::ObjectArray);
		attribute->SetClass(kwcSecondaryClass);
		kwcMultiTableClass->InsertAttribute(attribute);

		// Insertion dans le domaine
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcMultiTableClass);
		assert(kwcMultiTableClass->Check());
		kwcMultiTableClass->Compile();
	}

	// Prise en compte du nouveau dictionnaire
	if (bOk)
	{
		assert(kwcMultiTableClass != NULL);

		// Ajout du nom de la classe dans la liste
		classNameList->InsertItemAt(classNameList->GetItemNumber());
		classNameList->SetStringValueAt("Name", kwcMultiTableClass->GetName());

		// Message utilisateur
		AddSimpleMessage("Create multi-table dictionary " + kwcMultiTableClass->GetName());
	}
	return bOk;
}

const ALString KWMTClassBuilderView::GetClassLabel() const
{
	return "Multi-table dictionary builder";
}

const ALString KWMTClassBuilderView::GetObjectLabel() const
{
	return "";
}