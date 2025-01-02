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
	KWDataItem* referenceDataItem;
	KWAttribute* referenceAttribute;
	KWAttributeBlock* referenceAttributeBlock;
	ALString sCompleteType;
	ALString sOriginClass;
	boolean bTypeError;

	assert(kwcEditedClass != NULL);

	// Recherche de l'attribut de meme nom
	bTypeError = false;
	attribute = kwcEditedClass->LookupAttribute(editedObject->GetName());
	if (attribute != NULL)
	{
		// Calcul du type complet de l'attribut
		sCompleteType = KWType::ToString(attribute->GetType());
		if (KWType::IsRelation(attribute->GetType()) and attribute->GetClass() != NULL)
			sCompleteType = sCompleteType + "(" + attribute->GetClass()->GetName() + ")";
		else if (attribute->GetType() == KWType::Structure)
			sCompleteType = sCompleteType + "(" + attribute->GetStructureName() + ")";

		// Pour le champ attribut, des controles sont effectues pour determiner
		// si le changement de type est possible
		// En cas de probleme, un warning est emis, et le type correct est restitue
		// afin d'empecher de nouvelles emission de warning
		if (editedObject->GetType() != sCompleteType)
		{
			// Erreur si type initial de l'attribut non stocke
			if (not KWType::IsStored(attribute->GetType()))
			{
				attribute->AddError("Cannot change type of variable from " + sCompleteType + " to " +
						    editedObject->GetType() +
						    " (accepted initial types are: " + sStoredTypes + ")");
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
			// Erreur si attribut utilise dans une regle de calcul, directement ou via un bloc
			else if ((not attribute->IsInBlock() and
				  nkdEditedClassUsedAttributeReferences.Lookup((NUMERIC)attribute) != NULL) or
				 (attribute->IsInBlock() and nkdEditedClassUsedAttributeReferences.Lookup(
								 (NUMERIC)attribute->GetAttributeBlock()) != NULL))
			{
				if (not attribute->IsInBlock())
					referenceDataItem =
					    cast(KWDataItem*,
						 nkdEditedClassUsedAttributeReferences.Lookup((NUMERIC)attribute));
				else
					referenceDataItem =
					    cast(KWDataItem*, nkdEditedClassUsedAttributeReferences.Lookup(
								  (NUMERIC)attribute->GetAttributeBlock()));
				check(referenceDataItem);

				// Cas d'un attribut referencant
				if (referenceDataItem->IsAttribute())
				{
					referenceAttribute = cast(KWAttribute*, referenceDataItem);
					assert(referenceAttribute->GetAnyDerivationRule() != NULL);

					// On precise la classe d'origine si necessaire
					if (referenceAttribute->GetParentClass() != attribute->GetParentClass())
						sOriginClass =
						    " in dictionary " + referenceAttribute->GetParentClass()->GetName();
					attribute->AddError("Cannot change type of variable, used in derivation rule " +
							    referenceAttribute->GetAnyDerivationRule()->GetName() +
							    " by variable " + referenceAttribute->GetName() +
							    sOriginClass);
				}
				// Cas d'un bloc d'attributs referencant
				else
				{
					referenceAttributeBlock = cast(KWAttributeBlock*, referenceDataItem);
					assert(referenceAttributeBlock->GetDerivationRule() != NULL);

					// On precise la classe d'origine si necessaire
					if (referenceAttributeBlock->GetParentClass() != attribute->GetParentClass())
						sOriginClass = " in dictionary " +
							       referenceAttributeBlock->GetParentClass()->GetName();
					attribute->AddError(
					    "Cannot change type of variable, used in derivation rule " +
					    referenceAttributeBlock->GetDerivationRule()->GetName() +
					    " by sparse variable block " + referenceAttributeBlock->GetName() +
					    " (from " + referenceAttributeBlock->GetFirstAttribute()->GetName() +
					    " to " + referenceAttributeBlock->GetLastAttribute()->GetName() + ")" +
					    sOriginClass);
				}
				bTypeError = true;
			}
			// Erreur si attribut utilise dans la cle
			else if (nkdEditedClassKeyAttributes.Lookup((NUMERIC)attribute) != NULL)
			{
				attribute->AddError("Cannot change type of key variable that must be of type " +
						    KWType::ToString(KWType::Symbol));
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
			// Changement de type sinon
			else
				attribute->SetType(KWType::ToType(editedObject->GetType()));

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

	// Mise a jour des ses caracteristiques
	nkdEditedClassKeyAttributes.RemoveAll();
	nkdEditedClassUsedAttributeReferences.RemoveAll();
	if (kwcEditedClass != NULL)
	{
		BuildKeyAttributes(kwcEditedClass, &nkdEditedClassKeyAttributes);
		BuildClassUsedAttributeReferences(kwcEditedClass, &nkdEditedClassUsedAttributeReferences);
	}

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

void KWAttributeSpecArrayView::BuildKeyAttributes(KWClass* kwcClass, NumericKeyDictionary* nkdKeyAttributes) const
{
	int nKey;

	require(kwcClass != NULL);
	require(nkdKeyAttributes != NULL);
	require(nkdKeyAttributes->GetCount() == 0);

	for (nKey = 0; nKey < kwcClass->GetKeyAttributeNumber(); nKey++)
		nkdKeyAttributes->SetAt((NUMERIC)kwcClass->GetKeyAttributeAt(nKey), kwcClass->GetKeyAttributeAt(nKey));
}

void KWAttributeSpecArrayView::BuildClassUsedAttributeReferences(KWClass* kwcClass,
								 NumericKeyDictionary* nkdUsedAttributeReferences) const
{
	int nClass;
	KWClass* currentClass;
	KWAttribute* referenceAttribute;
	KWDerivationRule* analysedRule;

	require(kwcClass != NULL);
	require(nkdUsedAttributeReferences != NULL);
	require(nkdUsedAttributeReferences->GetCount() == 0);

	// Parcours de toutes les classes du domaines, qui pontentiellement peut referencer un attribut de la classe
	// dans le contexte multi-tables
	for (nClass = 0; nClass < kwcClass->GetDomain()->GetClassNumber(); nClass++)
	{
		currentClass = kwcClass->GetDomain()->GetClassAt(nClass);

		// Parcours des attributs de la classe pour analyser leur eventuelle regle de derivation
		// Attention, la classe n'est pas forcement compilee
		referenceAttribute = currentClass->GetHeadAttribute();
		while (referenceAttribute != NULL)
		{
			// Analyse de la regle portee par l'attribut
			if (not referenceAttribute->IsInBlock())
			{
				analysedRule = referenceAttribute->GetDerivationRule();
				if (analysedRule != NULL)
					BuildRuleUsedAttributeReferences(analysedRule, referenceAttribute,
									 nkdUsedAttributeReferences);
			}
			// Ou par le bloc d'attribut
			else
			{
				analysedRule = referenceAttribute->GetBlockDerivationRule();
				if (analysedRule != NULL)
				{
					// Analyse de la regle uniquement pour le premier attribut du bloc (et on
					// memorisera le bloc)
					if (referenceAttribute->IsFirstInBlock())
						BuildRuleUsedAttributeReferences(
						    analysedRule, referenceAttribute->GetAttributeBlock(),
						    nkdUsedAttributeReferences);
				}
			}

			// Attribut suivant
			currentClass->GetNextAttribute(referenceAttribute);
		}
	}
}

void KWAttributeSpecArrayView::BuildRuleUsedAttributeReferences(KWDerivationRule* rule, KWDataItem* referenceDataItem,
								NumericKeyDictionary* nkdUsedAttributeReferences) const
{
	KWAttribute* usedAttribute;
	KWAttributeBlock* usedAttributeBlock;
	KWDerivationRuleOperand* operand;
	KWDerivationRule* analysedRule;
	int i;

	require(rule != NULL);
	require(referenceDataItem != NULL);
	require(nkdUsedAttributeReferences != NULL);

	// Analyse des operandes de la regle
	for (i = 0; i < rule->GetOperandNumber(); i++)
	{
		operand = rule->GetOperandAt(i);

		// Cas d'un attribut utilise
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
		{
			// On traite le cas d'un attribut
			if (KWType::IsValue(operand->GetType()))
			{
				usedAttribute = operand->GetOriginAttribute();
				if (usedAttribute != NULL)
					nkdUsedAttributeReferences->SetAt((NUMERIC)usedAttribute, referenceDataItem);
			}
			// Sinon le cas d'un bloc d'attribut
			else
			{
				usedAttributeBlock = operand->GetOriginAttributeBlock();
				if (usedAttributeBlock != NULL)
					nkdUsedAttributeReferences->SetAt((NUMERIC)usedAttributeBlock,
									  referenceDataItem);
			}
		}
		// Cas d'une sous regle
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule)
		{
			analysedRule = operand->GetDerivationRule();
			if (analysedRule != NULL)
				BuildRuleUsedAttributeReferences(analysedRule, referenceDataItem,
								 nkdUsedAttributeReferences);
		}
	}
}

// ##
