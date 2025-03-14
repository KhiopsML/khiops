// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KILeverVariablesSpecView.h"

KILeverVariablesSpecView::KILeverVariablesSpecView()
{
	// Parametrage principal de l'interface
	SetLabel("Lever attributes base view");

	GetFieldAt("ClassName")->SetVisible(false);
	GetFieldAt("Root")->SetVisible(false);
	GetFieldAt("Key")->SetVisible(false);
	GetAttributeSpecArrayView()->GetFieldAt("Used")->SetEditable(true);
	GetAttributeSpecArrayView()->GetFieldAt("Derived")->SetVisible(false);
	GetAttributeSpecArrayView()->GetFieldAt("Label")->SetVisible(false);
	GetAttributeSpecArrayView()->GetFieldAt("MetaData")->SetVisible(false);

	// Info-bulles
	//GetFieldAt("ClassName")->SetHelpText("Name of the predictor dictionary");

	GetActionAt("SelectAll")->SetHelpText("Select all the variables to test.");
	GetActionAt("UnselectAll")->SetHelpText("Unselect all the variables to test.");
}

KILeverVariablesSpecView::~KILeverVariablesSpecView() {}

void KILeverVariablesSpecView::EventUpdate(Object* object)
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWAttributeSpec* attributeSpec;
	ALString sMetaData;
	int nAttribute;
	require(object != NULL);

	editedObject = cast(KWClassSpec*, object);

	KWClassSpecView::EventUpdate(object);

	// Recherche de la classe editee
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Mise a jour des metatags des attributs leviers a partir des specifications, si une classe utilisable par l'interpretation a ete chargee par l'utilisateur
	if (kwcClass != NULL)
	{
		// on ne touche pas a la propriete Used de l'attribut, afin de ne pas faire apparaitre inutilement cet attribut dans le transfert, si pas demande

		for (nAttribute = 0; nAttribute < oaAttributeSpecs.GetSize(); nAttribute++)
		{
			attributeSpec = cast(KWAttributeSpec*, oaAttributeSpecs.GetAt(nAttribute));

			// Recherche de l'attribut de meme nom
			attribute = kwcClass->LookupAttribute(attributeSpec->GetName());
			if (attribute != NULL)
			{
				assert(attribute->GetConstMetaData()->IsKeyPresent(
				    KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG));
				attribute->GetMetaData()->SetStringValueAt(
				    KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG,
				    attributeSpec->GetUsed() ? "true" : "false");
			}
		}
	}
}

void KILeverVariablesSpecView::SetObject(Object* object)
{
	KWClassSpec* editedObject;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWAttributeSpec* attributeSpec;
	ALString sMetaData;
	boolean bHasLeverVariables = false;

	require(object != NULL);

	// Recherche de la classe editee
	editedObject = cast(KWClassSpec*, object);
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(editedObject->GetClassName());

	// Mise a jour des specifications des attributs a partir de la classe, si une classe utilisable par l'interpretation a ete chargee par l'utilisateur
	if (kwcClass != NULL)
	{
		oaAttributeSpecs.DeleteAll();
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// on n'affiche que les variables pouvant etre eventuellement utilisees comme leviers
			if (attribute->GetConstMetaData()->IsKeyPresent(
				KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG))
			{
				bHasLeverVariables = true;
				// Creation de la specification
				attributeSpec = new KWAttributeSpec;
				oaAttributeSpecs.Add(attributeSpec);

				// Acces aux meta-data
				sMetaData = attribute->GetMetaData()->WriteString();

				// Initialisation a partir de l'attribut
				attributeSpec->SetUsed(attribute->GetConstMetaData()->GetStringValueAt(
							   KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG) ==
							       "true"
							   ? true
							   : false);
				attributeSpec->SetType(attribute->GetTypeLabel());
				attributeSpec->SetName(attribute->GetName());
				attributeSpec->SetDerived(attribute->GetAnyDerivationRule() != NULL);
				attributeSpec->SetMetaData(sMetaData);
				attributeSpec->SetLabel(attribute->GetLabel());
			}

			// Attribut suivant
			kwcClass->GetNextAttribute(attribute);
		}
		// Parametrage de la classe editee
		if (bHasLeverVariables)
			GetAttributeSpecArrayView()->SetEditedClass(kwcClass);
	}

	// Appel de la methode ancetre
	UIObjectView::SetObject(object);
}

KILeverVariablesSpecView* KILeverVariablesSpecView::Create() const
{
	return new KILeverVariablesSpecView;
}
