// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeSpecArrayView.h"

KWAttributeSpecArrayView::KWAttributeSpecArrayView()
{
	SetIdentifier("Array.KWAttributeSpec");
	SetLabel("Variables");
	AddBooleanField("Used", "Used", false);
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddBooleanField("Derived", "Derived", false);
	AddStringField("MetaData", "Meta-data", "");
	AddStringField("Label", "Label", "");

	// Card and help prameters
	SetItemView(new KWAttributeSpecView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");
	GetFieldAt("Type")->SetStyle("EditableComboBox");
	GetFieldAt("Derived")->SetStyle("CheckBox");

	// ## Custom constructor

	// Liste des types d'attributs
	ALString sTypeValues;
	int nType;

	// Complements de parametrage pour les types
	for (nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsStored(nType))
		{
			if (sTypeValues != "")
				sTypeValues += "\n";
			sTypeValues += KWType::ToString(nType);
		}
	}
	GetFieldAt("Type")->SetParameters(sTypeValues);

	// On indique que le champ de parametrage des types declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir controler la validite des types
	cast(UIElement*, GetFieldAt("Type"))->SetTriggerRefresh(true);

	// Champ a utiliser pour les selections
	SetKeyFieldId("Name");

	// La classe editee est initialiement vide
	kwcEditedClass = NULL;

	// Calcul du libelle des types stockes disponibles
	for (nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsStored(nType))
		{
			if (sStoredTypes != "")
				sStoredTypes += ", ";
			sStoredTypes += KWType::ToString(nType);
		}
	}

	// ##
}

KWAttributeSpecArrayView::~KWAttributeSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWAttributeSpecArrayView::EventNew()
{
	return new KWAttributeSpec;
}

void KWAttributeSpecArrayView::EventUpdate(Object* object)
{
	KWAttributeSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeSpec*, object);
	editedObject->SetUsed(GetBooleanValueAt("Used"));
	editedObject->SetType(GetStringValueAt("Type"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetDerived(GetBooleanValueAt("Derived"));
	editedObject->SetMetaData(GetStringValueAt("MetaData"));
	editedObject->SetLabel(GetStringValueAt("Label"));

	// ## Custom update

	// Variables de travail
	KWAttribute* attribute;
	ALString sCompleteType;
	int nKey;
	boolean bIsKeyAttribute;
	boolean bTypeError;
	int nPreviousType;
	int nClass;
	ALString sTmp;

	assert(kwcEditedClass != NULL);

	// Recherche de l'attribut de meme nom
	bTypeError = false;
	attribute = kwcEditedClass->LookupAttribute(editedObject->GetName());
	if (attribute != NULL)
	{
		// Calcul du type complet de l'attribut
		sCompleteType = attribute->GetTypeLabel();

		// Pour le champ attribut, des controles sont effectues pour determiner
		// si le changement de type est possible
		// En cas de probleme, un warning est emis, et le type correct est restitue
		// afin d'empecher de nouvelles emission de warning
		if (editedObject->GetType() != sCompleteType)
		{
			// On determine si l'attribut fait partie de la cle
			bIsKeyAttribute = false;
			for (nKey = 0; nKey < kwcEditedClass->GetKeyAttributeNumber(); nKey++)
			{
				if (kwcEditedClass->GetKeyAttributeNameAt(nKey) == attribute->GetName())
				{
					bIsKeyAttribute = true;
					break;
				}
			}

			// Erreur si type initial de l'attribut non stocke
			if (not KWType::IsStored(attribute->GetType()))
			{
				attribute->AddError("Cannot change type of variable from " + sCompleteType + " to " +
						    editedObject->GetType() +
						    " (accepted initial types are: " + sStoredTypes + ")");
				bTypeError = true;
			}
			// Erreur si type final de l'attribut non stocke
			else if (not KWType::IsStored(KWType::ToType(editedObject->GetType())))
			{
				attribute->AddError("Cannot change type of variable from " + sCompleteType + " to " +
						    editedObject->GetType() +
						    " (accepted final types are: " + sStoredTypes + ")");
				bTypeError = true;
			}
			// Erreur si attribut utilise dans la cle
			else if (bIsKeyAttribute)
			{
				attribute->AddError("Cannot change type of key variable that must be of type " +
						    KWType::ToString(KWType::Symbol));
				bTypeError = true;
			}
			// Erreur si attribut de type complexe, avec specification de format
			else if (KWType::IsComplex(attribute->GetType()) and
				 attribute->GetConstMetaData()->GetStringValueAt(
				     attribute->GetFormatMetaDataKey(attribute->GetType())) != "")
			{
				attribute->AddError("Cannot change type of variable, converted with format \"" +
						    attribute->GetConstMetaData()->GetStringValueAt(
							attribute->GetFormatMetaDataKey(attribute->GetType())) +
						    "\"");
				bTypeError = true;
			}
			// Erreur si attribut dans un bloc
			else if (attribute->IsInBlock())
			{
				attribute->AddError("Cannot change type of variable used in a sparse variable block (" +
						    attribute->GetAttributeBlock()->GetName() + ")");
				bTypeError = true;
			}
			// Erreur si attribut resultat d'une regle de calcul
			else if (attribute->GetDerivationRule() != NULL)
			{
				attribute->AddError("Cannot change type of variable, computed using derivation rule " +
						    attribute->GetDerivationRule()->GetName());
				bTypeError = true;
			}
			// Changement de type sinon, en verifiant que cela n'entraine pas une incoherence
			// par utilisation de l'attribut directement ou indirectement en operande d'une regle
			else
			{
				// Memorisation prealable du type valide
				nPreviousType = attribute->GetType();
				assert(KWType::IsStored(nPreviousType));

				// Changement de type et verification de la coherence sur tout le domaine
				attribute->SetType(KWType::ToType(editedObject->GetType()));

				// On invalide la fraicheur de toutes les classes du domaine, pour forcer les controles
				for (nClass = 0; nClass < kwcEditedClass->GetDomain()->GetClassNumber(); nClass++)
					kwcEditedClass->GetDomain()->GetClassAt(nClass)->UpdateFreshness();

				// Verification de tout le domaine
				bTypeError = not kwcEditedClass->GetDomain()->Check();
				if (bTypeError)
				{
					attribute->AddError(
					    sTmp +
					    "Because the variable is used as an operand in derivation rules with the "
					    "expected type " +
					    KWType::ToString(nPreviousType) + ", the type cannot be changed to " +
					    editedObject->GetType());
				}

				// On restitue le type iniial en cas d'erreur
				if (bTypeError)
					attribute->SetType(nPreviousType);
			}

			// Annulation si necessaire du changement de type dans l'interface
			if (bTypeError)
			{
				editedObject->SetType(sCompleteType);
				SetStringValueAt("Type", editedObject->GetType());
				attribute->AddMessage(
				    "The dictionary file must be edited directly for complex modifications.");
			}
		}
	}

	// ##
}

void KWAttributeSpecArrayView::EventRefresh(Object* object)
{
	KWAttributeSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeSpec*, object);
	SetBooleanValueAt("Used", editedObject->GetUsed());
	SetStringValueAt("Type", editedObject->GetType());
	SetStringValueAt("Name", editedObject->GetName());
	SetBooleanValueAt("Derived", editedObject->GetDerived());
	SetStringValueAt("MetaData", editedObject->GetMetaData());
	SetStringValueAt("Label", editedObject->GetLabel());

	// ## Custom refresh

	assert(kwcEditedClass != NULL);

	// ##
}

const ALString KWAttributeSpecArrayView::GetClassLabel() const
{
	return "Variables";
}

// ## Method implementation

void KWAttributeSpecArrayView::SetEditedClass(KWClass* kwcClass)
{
	const int nDefaultLabelLength = 0;
	const int nMaxDisplayLabelLength = 50;
	int nMaxLabelLength;
	KWAttribute* attribute;

	// Memorisation de la classe editee
	kwcEditedClass = kwcClass;

	// Recherche de la longueur du plus grand libelle
	nMaxLabelLength = nDefaultLabelLength;
	attribute = kwcClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		nMaxLabelLength = max(nMaxLabelLength, attribute->GetLabel().GetLength());
		kwcClass->GetNextAttribute(attribute);
	}

	// Ajout d'un taille supplementaire au tableau edite en presence de libelles longs
	SetLastColumnExtraWidth(min(nMaxDisplayLabelLength, nMaxLabelLength - nDefaultLabelLength));
}

// ##
