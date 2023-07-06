// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWClassSpecView.h"

KWClassSpecView::KWClassSpecView()
{
	SetIdentifier("KWClassSpec");
	SetLabel("Dictionary");
	AddStringField("ClassName", "Name", "");
	AddBooleanField("Root", "Root", false);
	AddStringField("Key", "Key", "");
	AddIntField("AttributeNumber", "Variables", 0);
	AddIntField("SymbolAttributeNumber", "Categorical", 0);
	AddIntField("ContinuousAttributeNumber", "Numerical", 0);
	AddIntField("DerivedAttributeNumber", "Derived", 0);

	// ## Custom constructor

	int nField;

	// Seul le nom du dictionnaire reste visible
	for (nField = 0; nField < GetFieldNumber(); nField++)
		GetFieldAtIndex(nField)->SetVisible(false);
	GetFieldAt("ClassName")->SetVisible(true);

	// Ajout d'une liste des attributs
	attributeSpecArrayView = new KWAttributeSpecArrayView;
	AddListField("AttributeSpecs", "Variables", attributeSpecArrayView);
	attributeSpecArrayView->SetObjectArray(&oaAttributeSpecs);

	// On passe en mode non editable
	SetEditable(false);

	// Ajout des actions
	AddAction("SelectAll", "Select all", (ActionMethod)(&KWClassSpecView::SelectAll));
	AddAction("UnselectAll", "Unselect all", (ActionMethod)(&KWClassSpecView::UnselectAll));
	GetActionAt("SelectAll")->SetStyle("Button");
	GetActionAt("UnselectAll")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Name of dictionary.");
	GetFieldAt("Root")->SetHelpText("Indicate whether the dictionary relates to a Root entity,"
					"\n identified by a unique key.");
	GetFieldAt("Key")->SetHelpText("Key variables of the dictionary.");
	GetFieldAt("AttributeNumber")->SetHelpText("Total number of currently selected variables.");
	GetFieldAt("SymbolAttributeNumber")->SetHelpText("Number of categorical variables.");
	GetFieldAt("ContinuousAttributeNumber")->SetHelpText("Number of numerical variables.");
	GetFieldAt("DerivedAttributeNumber")->SetHelpText("Number of derived variables.");
	GetActionAt("SelectAll")->SetHelpText("Select all variables.");
	GetActionAt("UnselectAll")->SetHelpText("Unselect all variables.");

	// Short cuts
	GetActionAt("SelectAll")->SetShortCut('S');
	GetActionAt("UnselectAll")->SetShortCut('U');

	// ##
}

KWClassSpecView::~KWClassSpecView()
{
	// ## Custom destructor

	// Destruction des specifications d'attributs (variables de travail)
	oaAttributeSpecs.DeleteAll();

	// ##
}

KWClassSpec* KWClassSpecView::GetKWClassSpec()
{
	require(objValue != NULL);
	return cast(KWClassSpec*, objValue);
}

void KWClassSpecView::EventUpdate(Object* object)
{
	KWClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassSpec*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetRoot(GetBooleanValueAt("Root"));
	editedObject->SetKey(GetStringValueAt("Key"));
	editedObject->SetAttributeNumber(GetIntValueAt("AttributeNumber"));
	editedObject->SetSymbolAttributeNumber(GetIntValueAt("SymbolAttributeNumber"));
	editedObject->SetContinuousAttributeNumber(GetIntValueAt("ContinuousAttributeNumber"));
	editedObject->SetDerivedAttributeNumber(GetIntValueAt("DerivedAttributeNumber"));

	// ## Custom update

	// ##
}

void KWClassSpecView::EventRefresh(Object* object)
{
	KWClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetBooleanValueAt("Root", editedObject->GetRoot());
	SetStringValueAt("Key", editedObject->GetKey());
	SetIntValueAt("AttributeNumber", editedObject->GetAttributeNumber());
	SetIntValueAt("SymbolAttributeNumber", editedObject->GetSymbolAttributeNumber());
	SetIntValueAt("ContinuousAttributeNumber", editedObject->GetContinuousAttributeNumber());
	SetIntValueAt("DerivedAttributeNumber", editedObject->GetDerivedAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KWClassSpecView::GetClassLabel() const
{
	return "Dictionary";
}

// ## Method implementation

void KWClassSpecView::SelectAll()
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	int nAttribute;
	KWAttributeSpec* attributeSpec;

	// Acces a l'objet edite
	editedObject = cast(KWClassSpec*, GetObject());

	// Recherche de la classe editee
	kwcClass = NULL;
	if (editedObject != NULL)
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Modification de la selection des attributs
	if (kwcClass != NULL)
		kwcClass->SetAllAttributesUsed(true);

	// Idem pour les specs d'attributs
	for (nAttribute = 0; nAttribute < oaAttributeSpecs.GetSize(); nAttribute++)
	{
		attributeSpec = cast(KWAttributeSpec*, oaAttributeSpecs.GetAt(nAttribute));
		attributeSpec->SetUsed(true);
	}
}

void KWClassSpecView::UnselectAll()
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	int nAttribute;
	KWAttributeSpec* attributeSpec;

	// Acces a l'objet edite
	editedObject = cast(KWClassSpec*, GetObject());

	// Recherche de la classe editee
	kwcClass = NULL;
	if (editedObject != NULL)
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Modification de la selection des attributs
	if (kwcClass != NULL)
		kwcClass->SetAllAttributesUsed(false);

	// Idem pour les specs d'attributs
	for (nAttribute = 0; nAttribute < oaAttributeSpecs.GetSize(); nAttribute++)
	{
		attributeSpec = cast(KWAttributeSpec*, oaAttributeSpecs.GetAt(nAttribute));
		attributeSpec->SetUsed(false);
	}
}

KWAttributeSpecArrayView* KWClassSpecView::GetAttributeSpecArrayView()
{
	return attributeSpecArrayView;
}

void KWClassSpecView::SetObject(Object* object)
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWAttributeSpec* attributeSpec;
	ALString sCompleteType;
	ALString sMetaData;

	require(object != NULL);

	// Recherche de la classe editee
	editedObject = cast(KWClassSpec*, object);
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Mise a jour des specifications des attributs a partir de la classe
	if (kwcClass != NULL)
	{
		oaAttributeSpecs.DeleteAll();
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Creation de la specification
			attributeSpec = new KWAttributeSpec;
			oaAttributeSpecs.Add(attributeSpec);

			// Calcul du type complet de l'attribut
			sCompleteType = KWType::ToString(attribute->GetType());
			if (KWType::IsRelation(attribute->GetType()) and attribute->GetClass() != NULL)
				sCompleteType = sCompleteType + "(" + attribute->GetClass()->GetName() + ")";
			else if (attribute->GetType() == KWType::Structure)
				sCompleteType = sCompleteType + "(" + attribute->GetStructureName() + ")";

			// Acces aux meta-data
			sMetaData = attribute->GetMetaData()->WriteString();

			// Initialisation a partir de l'attribut
			attributeSpec->SetUsed(attribute->GetUsed());
			attributeSpec->SetType(sCompleteType);
			attributeSpec->SetName(attribute->GetName());
			attributeSpec->SetDerived(attribute->GetAnyDerivationRule() != NULL);
			attributeSpec->SetMetaData(sMetaData);
			attributeSpec->SetLabel(attribute->GetLabel());

			// Attribut suivant
			kwcClass->GetNextAttribute(attribute);
		}
	}

	// Parametrage du nombre de lignes visibles
	if (kwcClass != NULL)
		GetAttributeSpecArrayView()->SetLineNumber(min(25, oaAttributeSpecs.GetSize()));
	else
		GetAttributeSpecArrayView()->SetLineNumber(0);

	// Parametrage de la classe editee
	GetAttributeSpecArrayView()->SetEditedClass(kwcClass);

	// Appel de la methode ancetre
	UIObjectView::SetObject(object);
}

Object* KWClassSpecView::GetObject()
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	int nAttribute;
	KWAttribute* attribute;
	KWAttributeSpec* attributeSpec;

	// Appel de la methode ancetre
	editedObject = cast(KWClassSpec*, UIObjectView::GetObject());

	// Recherche de la classe editee
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Mise a jour des attributs de la classe a partir des specifications
	if (kwcClass != NULL)
	{
		for (nAttribute = 0; nAttribute < oaAttributeSpecs.GetSize(); nAttribute++)
		{
			attributeSpec = cast(KWAttributeSpec*, oaAttributeSpecs.GetAt(nAttribute));

			// Recherche de l'attribut de meme nom
			attribute = kwcClass->LookupAttribute(attributeSpec->GetName());
			if (attribute != NULL)
			{
				// Recopie des specs dans l'attribut (en evitant si possible
				// de modifier la fraicheur)
				if (attribute->GetUsed() != attributeSpec->GetUsed())
					attribute->SetUsed(attributeSpec->GetUsed());

				// Le champs Loaded est base sur Used
				if (attribute->GetLoaded() != attribute->GetUsed())
					attribute->SetLoaded(attribute->GetUsed());
			}
		}

		// Compilation de la classe
		if (kwcClass->Check())
			kwcClass->GetDomain()->Compile();
	}

	return editedObject;
}

// ##