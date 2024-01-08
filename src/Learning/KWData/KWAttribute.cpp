// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClass.h"

//////////////////////////////
// KWAttribute

KWAttribute::KWAttribute()
{
	cType = KWType::Unknown;
	attributeClass = NULL;
	kwdrRule = NULL;
	bUsed = true;
	bLoaded = true;
	advancedTypeSpecification.genericSpecification = NULL;
	parentClass = NULL;
	attributeBlock = NULL;
	listPosition = NULL;
	dCost = 0;
}

KWAttribute::~KWAttribute()
{
	if (kwdrRule != NULL)
		delete kwdrRule;
	if (advancedTypeSpecification.genericSpecification != NULL)
		delete advancedTypeSpecification.genericSpecification;
}

const ALString KWAttribute::GetFormatMetaDataKey(int nComplexType)
{
	ALString sFormatKey;
	require(KWType::IsComplex(nComplexType));
	sFormatKey = KWType::ToString(nComplexType) + "Format";
	return sFormatKey;
}

const ALString& KWAttribute::GetCostMetaDataKey()
{
	static const ALString sCostMetaDataKey = "Cost";
	return sCostMetaDataKey;
}

void KWAttribute::SetMetaDataCost(double dValue)
{
	require(dValue >= 0);
	GetMetaData()->SetDoubleValueAt(GetCostMetaDataKey(), dValue);
}

double KWAttribute::GetMetaDataCost() const
{
	return GetConstMetaData()->GetDoubleValueAt(GetCostMetaDataKey());
}

void KWAttribute::CompleteTypeInfo(KWClass* kwcOwnerClass)
{
	NumericKeyDictionary nkdCompletedAttributes;
	InternalCompleteTypeInfo(kwcOwnerClass, &nkdCompletedAttributes);
}

void KWAttribute::InternalCompleteTypeInfo(KWClass* kwcOwnerClass, NumericKeyDictionary* nkdCompletedAttributes)
{
	KWClass* kwcObjectClass;

	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Completion des infos de l'attributs, si cela n'a pas deja ete fait
	if (nkdCompletedAttributes->Lookup(this) == NULL)
	{
		// Memorisation de l'attribut
		nkdCompletedAttributes->SetAt(this, this);

		// Informations de type si premier attribut d'un bloc
		if (IsFirstInBlock() and GetBlockDerivationRule() != NULL)
		{
			// Completion des infos de la regle
			GetBlockDerivationRule()->InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);
		}
		// Informations de type si regle
		else if (GetDerivationRule() != NULL)
		{
			// Completion des infos de la regle
			GetDerivationRule()->InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

			// Recuperation des informations de la regle
			SetType(GetDerivationRule()->GetType());
			if (GetDerivationRule()->GetType() == KWType::Structure)
				SetStructureName(GetDerivationRule()->GetStructureName());

			// Cas d'un type Object
			if (KWType::IsGeneralRelation(GetDerivationRule()->GetType()))
			{
				kwcObjectClass = NULL;
				if (kwcOwnerClass->GetDomain() != NULL)
					kwcObjectClass = kwcOwnerClass->GetDomain()->LookupClass(
					    GetDerivationRule()->GetObjectClassName());
				SetClass(kwcObjectClass);
			}
		}
	}
}

void KWAttribute::BuildAdvancedTypeSpecification()
{
	ALString sFormatMetaDataKey;
	ALString sFormat;
	boolean bOk;

	require(Check());

	// Nettoyage prealable
	if (advancedTypeSpecification.genericSpecification != NULL)
		delete advancedTypeSpecification.genericSpecification;
	advancedTypeSpecification.genericSpecification = NULL;

	// Construction d'un objet de format Date
	if (GetType() == KWType::Date)
	{
		sFormatMetaDataKey = GetFormatMetaDataKey(KWType::Date);
		sFormat = metaData.GetStringValueAt(sFormatMetaDataKey);
		if (sFormat == "")
			sFormat = KWDateFormat::GetDefaultFormatString();
		advancedTypeSpecification.dateFormat = new KWDateFormat;
		bOk = advancedTypeSpecification.dateFormat->SetFormatString(sFormat);
		assert(bOk);
	}
	// Construction d'un objet de format Time
	else if (GetType() == KWType::Time)
	{
		sFormatMetaDataKey = GetFormatMetaDataKey(KWType::Time);
		sFormat = metaData.GetStringValueAt(sFormatMetaDataKey);
		if (sFormat == "")
			sFormat = KWTimeFormat::GetDefaultFormatString();
		advancedTypeSpecification.timeFormat = new KWTimeFormat;
		bOk = advancedTypeSpecification.timeFormat->SetFormatString(sFormat);
		assert(bOk);
	}
	// Construction d'un objet de format Timestamp
	else if (GetType() == KWType::Timestamp)
	{
		sFormatMetaDataKey = GetFormatMetaDataKey(KWType::Timestamp);
		sFormat = metaData.GetStringValueAt(sFormatMetaDataKey);
		if (sFormat == "")
			sFormat = KWTimestampFormat::GetDefaultFormatString();
		advancedTypeSpecification.timestampFormat = new KWTimestampFormat;
		bOk = advancedTypeSpecification.timestampFormat->SetFormatString(sFormat);
		assert(bOk);
	}
	// Construction d'un objet de format TimestampTZ
	else if (GetType() == KWType::TimestampTZ)
	{
		sFormatMetaDataKey = GetFormatMetaDataKey(KWType::TimestampTZ);
		sFormat = metaData.GetStringValueAt(sFormatMetaDataKey);
		if (sFormat == "")
			sFormat = KWTimestampTZFormat::GetDefaultFormatString();
		advancedTypeSpecification.timestampTZFormat = new KWTimestampTZFormat;
		bOk = advancedTypeSpecification.timestampTZFormat->SetFormatString(sFormat);
		assert(bOk);
	}
}

KWAttribute* KWAttribute::Clone() const
{
	KWAttribute* kwaClone;

	kwaClone = new KWAttribute;
	kwaClone->usName = usName;
	kwaClone->metaData.CopyFrom(&metaData);
	kwaClone->usLabel = usLabel;
	kwaClone->cType = cType;
	kwaClone->attributeClass = attributeClass;
	kwaClone->usStructureName = usStructureName;
	if (kwdrRule != NULL)
		kwaClone->kwdrRule = kwdrRule->Clone();
	kwaClone->bUsed = bUsed;
	kwaClone->bLoaded = bLoaded;
	kwaClone->dCost = dCost;

	return kwaClone;
}

boolean KWAttribute::Check() const
{
	boolean bOk = true;
	KWDerivationRule* kwdrReference;
	ALString sFormatMetaDataKey;
	KWDateFormat dateFormat;
	KWTimeFormat timeFormat;
	KWTimestampFormat timestampFormat;
	KWTimestampTZFormat timestampTZFormat;
	ALString sFormat;
	int nComplexType;

	require(parentClass == NULL or parentClass->GetDomain() == NULL or
		parentClass->GetDomain()->LookupClass(parentClass->GetName()) == parentClass);

	// Nom
	if (not KWClass::CheckName(GetName(), this))
		bOk = false;

	// Libelle
	if (not KWClass::CheckLabel(GetLabel(), this))
		bOk = false;

	// Type
	if (not KWType::Check(GetType()))
	{
		bOk = false;
		AddError("Unknown type");
	}

	// Meta-donnee de format: doivent etre utilisee avec le bon type, et etre valide
	// Les autres meta-donnees sont valides par construction pour leur syntaxe generale
	if (bOk)
	{
		// Parcours des types complexes
		for (nComplexType = KWType::Date; nComplexType <= KWType::TimestampTZ; nComplexType++)
		{
			// Verification pour les types complexes
			if (KWType::IsComplex(nComplexType))
			{
				// Verification du format
				sFormatMetaDataKey = GetFormatMetaDataKey(nComplexType);
				if (metaData.IsKeyPresent(sFormatMetaDataKey))
				{
					if (GetType() != nComplexType)
					{
						bOk = false;
						AddError("Meta-data " + sFormatMetaDataKey + " (" +
							 metaData.GetExternalValueAt(sFormatMetaDataKey) +
							 ") should not be specified for type " +
							 KWType::ToString(GetType()));
					}
					else if (not metaData.IsStringTypeAt(sFormatMetaDataKey))
					{
						bOk = false;
						AddError("Meta-data " + sFormatMetaDataKey +
							 " should be specified with a string value");
					}
					else
					{
						sFormat = metaData.GetStringValueAt(sFormatMetaDataKey);
						if (GetType() == KWType::Date)
							bOk = dateFormat.SetFormatString(sFormat);
						else if (GetType() == KWType::Time)
							bOk = timeFormat.SetFormatString(sFormat);
						else if (GetType() == KWType::Timestamp)
							bOk = timestampFormat.SetFormatString(sFormat);
						else if (GetType() == KWType::TimestampTZ)
							bOk = timestampTZFormat.SetFormatString(sFormat);
						if (not bOk)
							AddError("Invalid " + KWType::ToString(GetType()) + " format " +
								 sFormat + " in meta-data " + sFormatMetaDataKey);
					}
				}
			}
		}
	}

	// Classe si type Object
	if (KWType::IsGeneralRelation(GetType()))
	{
		// Une variable native de type relation ne doit pas avoir de back-quote dans son nom, pour qu'il n'y
		// ait pas d'ambiguite dans les DataPath natifs (cf database multi-tables)
		if (kwdrRule == NULL and attributeBlock == NULL and GetName().Find('`') >= 0)
		{
			AddError("Incorrect name for a native variable of type " + KWType::ToString(GetType()) +
				 ": must not contain back-quote");
			bOk = false;
		}

		// Presence de la classe
		if (attributeClass == NULL)
		{
			AddError("No reference dictionary for type " + KWType::ToString(GetType()));
			bOk = false;
		}
		else if (not KWClass::CheckName(attributeClass->GetName(), this))
		{
			AddError("Incorrect name of reference dictionary for type " + KWType::ToString(GetType()));
			bOk = false;
		}

		// Coherence entre status d'objet reference et classe utilisee dans la composition
		if (not GetReference() and kwdrRule == NULL and attributeClass != NULL and attributeClass->GetRoot())
		{
			AddError("Dictionary " + attributeClass->GetName() + " used within type " +
				 KWType::ToString(GetType()) + " should not be a root dictionary");
			bOk = false;
		}

		// La classe utilisee doit avoir une cle
		if (attributeClass != NULL and attributeClass->GetKeyAttributeNumber() == 0)
		{
			AddError("The used dictionary " + attributeClass->GetName() + " should have a key");
			bOk = false;
		}

		// La cle de la classe utilisee doit etre au moins aussi longue que
		// celle de la classe utilisante dans le cas d'un lien de composition
		if (not GetReference() and kwdrRule == NULL and attributeClass != NULL and parentClass != NULL and
		    not attributeClass->GetRoot() and
		    attributeClass->GetKeyAttributeNumber() < parentClass->GetKeyAttributeNumber())
		{
			AddError("In used dictionary (" + attributeClass->GetName() + "), the length of the key (" +
				 IntToString(attributeClass->GetKeyAttributeNumber()) +
				 " variables) should not be inferior to that of the parent dictionary (" +
				 parentClass->GetName() + " with a key of " +
				 IntToString(parentClass->GetKeyAttributeNumber()) + " variables)");
			bOk = false;
		}
	}

	// Nom de structure si type Structure
	if (GetType() == KWType::Structure)
	{
		// Presence du type
		if (GetStructureName() == "")
		{
			AddError("No name for type Structure");
			bOk = false;
		}
		else if (not KWClass::CheckName(GetStructureName(), this))
		{
			AddError("Incorrect name for type Structure");
			bOk = false;
		}
	}

	// Warning si classe sans type Relation
	if (not KWType::IsGeneralRelation(GetType()) and attributeClass != NULL)
	{
		AddWarning("Dictionary (" + attributeClass->GetName() +
			   ") referenced by a variable with non relation type");
	}

	// Warning si nom de structure sans type Structure
	if (GetType() != KWType::Structure and GetStructureName() != "")
	{
		AddWarning("Structure name used with a non Structure type");
	}

	// Warning si type Structure sans regle associee
	if (GetType() == KWType::Structure and kwdrRule == NULL)
	{
		AddWarning("Structure type should be related to a derivation rule");
	}

	// Verification de la regle de derivation
	if (kwdrRule != NULL)
	{
		// Verification de l'existence de la regle de derivation
		kwdrReference = KWDerivationRule::LookupDerivationRule(kwdrRule->GetName());
		if (kwdrReference == NULL)
		{
			AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() + " does not exist");
			bOk = false;
		}

		// Verification de la coherence avec la regle de derivation enregistree
		if (bOk and kwdrReference != NULL)
		{
			assert(kwdrReference->GetName() == kwdrRule->GetName());
			assert(parentClass == NULL or kwdrReference->CheckDefinition());

			if (not kwdrRule->CheckFamily(kwdrReference))
			{
				AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() +
					 " incorrectly specified");
				bOk = false;
			}
		}

		// Verification de la regle de derivation elle-meme
		if (bOk and parentClass != NULL and not kwdrRule->CheckCompleteness(parentClass))
		{
			AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() +
				 " incorrectly specified");
			bOk = false;
		}

		// Verification de la coherence avec le type de l'attribut cible de la regle
		if (bOk and kwdrRule->GetType() != GetType())
		{
			AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() + " with type " +
				 KWType::ToString(kwdrRule->GetType()) + " inconsistent with that of the variable (" +
				 KWType::ToString(GetType()) + ")");
			bOk = false;
		}
		// Verification de la coherence avec le type objet de l'attribut cible de la regle
		else if (bOk and KWType::IsGeneralRelation(GetType()) and GetClass() != NULL and
			 kwdrRule->GetObjectClassName() != GetClass()->GetName())
		{
			AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() + " with type " +
				 KWType::ToString(GetType()) + " (" + kwdrRule->GetObjectClassName() +
				 ") inconsistent with that of the variable (" + GetClass()->GetName() + ")");
			bOk = false;
		}
		// Verification de la coherence avec le nom de structure
		else if (bOk and GetType() == KWType::Structure and GetStructureName() != kwdrRule->GetStructureName())
		{
			AddError(kwdrRule->GetClassLabel() + " " + GetDerivationRule()->GetName() +
				 " with Structure name (" + GetStructureName() +
				 ") inconsistent with that of the variable (" + kwdrRule->GetStructureName() + ")");
			bOk = false;
		}
	}

	// Verification de l'eventuel bloc, uniquement si l'attribut est le dernier du bloc
	if (bOk and attributeBlock != NULL and attributeBlock->GetLastAttribute() == this)
		bOk = attributeBlock->Check();
	return bOk;
}

void KWAttribute::Compile()
{
	require(parentClass != NULL and parentClass->LookupAttribute(GetName()) == this);
	require(Check());

	// Compilation de l'eventuelle regle de derivation
	if (GetDerivationRule() != NULL)
		GetDerivationRule()->Compile(parentClass);

	// Compilation de l'eventuel bloc, uniquement si l'attribut est le premier du bloc
	if (IsFirstInBlock())
		attributeBlock->Compile();

	// Construction des specification avancee pour certains types
	BuildAdvancedTypeSpecification();
}

boolean KWAttribute::ContainsCycle(NumericKeyDictionary* nkdGreyAttributes,
				   NumericKeyDictionary* nkdBlackAttributes) const
{
	boolean bContainsCycle = false;

	require(nkdGreyAttributes != NULL);
	require(nkdBlackAttributes != NULL);
	require(Check());
	require(parentClass->IsCompiled());
	require(not IsInBlock());

	// Marquage de l'attribut en Grey
	nkdGreyAttributes->SetAt(this, (Object*)this);

	// Analyse de l'eventuelle regle de derivation attachee a l'attribut
	if (GetDerivationRule() != NULL)
		bContainsCycle = GetDerivationRule()->ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);

	// Marquage de l'attribut en Black
	nkdGreyAttributes->SetAt(this, NULL);
	nkdBlackAttributes->SetAt(this, (Object*)this);

	return bContainsCycle;
}

longint KWAttribute::GetUsedMemory() const
{
	longint lUsedMemory;

	// Memoire utilise par l'attribut lui-meme
	// On prend en compte la memoire du nom et du libelle, meme s'ils sont potentiellement partages
	// (mais pas memoire de la Structure, qui est largement partagee)
	// On rajoute une fois le nom le nom de l'attribut, qui sert de cle dans sa classe utilisante
	lUsedMemory = sizeof(KWAttribute);
	lUsedMemory += usName.GetUsedMemory();
	lUsedMemory += usName.GetValue().GetUsedMemory();
	lUsedMemory += usLabel.GetUsedMemory();
	lUsedMemory += metaData.GetUsedMemory() - sizeof(KWMetaData);

	// Prise en compte de la regle de derivation
	if (kwdrRule != NULL)
		lUsedMemory += kwdrRule->GetUsedMemory();

	// Prise en compte approximative des specifications avancees liee au type de l'attribut
	if (advancedTypeSpecification.genericSpecification != NULL)
		lUsedMemory += sizeof(advancedTypeSpecification.genericSpecification->GetUsedMemory());
	return lUsedMemory;
}

longint KWAttribute::ComputeHashValue() const
{
	longint lHash;

	// Prise en compte des caracteristiques de l'attribut
	lHash = HashValue(GetName());
	lHash = LongintUpdateHashValue(lHash, HashValue(KWType::ToString(GetType())));
	if (KWType::IsGeneralRelation(GetType()) and attributeClass != NULL)
		lHash = LongintUpdateHashValue(lHash, HashValue(attributeClass->GetName()));
	else if (GetType() == KWType::Structure)
		lHash = LongintUpdateHashValue(lHash, HashValue(GetStructureName()));
	if (bUsed)
		lHash = LongintUpdateHashValue(lHash, HashValue("Used"));
	if (bLoaded)
		lHash = LongintUpdateHashValue(lHash, HashValue("Loaded"));
	if (kwdrRule != NULL)
		lHash = LongintUpdateHashValue(lHash, kwdrRule->ComputeHashValue());
	lHash = LongintUpdateHashValue(lHash, metaData.ComputeHashValue());
	return lHash;
}

void KWAttribute::Write(ostream& ost) const
{
	// Attribute a ne pas utiliser
	if (not GetUsed())
		ost << "Unused";
	ost << "\t";

	// Type
	ost << KWType::ToString(GetType());

	// Classe pour un type objet
	if (KWType::IsGeneralRelation(GetType()))
	{
		if (GetClass() != NULL and GetClass()->GetName() != "")
			ost << "(" << KWClass::GetExternalName(GetClass()->GetName()) << ")";
		else
			ost << "(UnknownClass)";
	}
	// Nom de structure pour un type Structure
	else if (GetType() == KWType::Structure)
	{
		if (GetStructureName() != "")
			ost << "(" << KWClass::GetExternalName(GetStructureName()) << ")";
		else
			ost << "(UnknownStructure)";
	}
	ost << "\t";

	// Nom
	ost << KWClass::GetExternalName(GetName());
	ost << "\t";

	// Regle de derivation
	if (kwdrRule != NULL)
	{
		// Dans le cas de la regle predefinie de Reference, on n'utilise pas le signe '='
		if (kwdrRule->GetName() != KWDerivationRule::GetReferenceRuleName())
			ost << " = ";
		kwdrRule->WriteUsedRule(ost);
	}
	ost << "\t";

	// Fin de declaration
	ost << ";";

	// Meta-donnees
	WriteNotLoadedMetaData(ost);
	if (metaData.GetKeyNumber() > 0)
	{
		ost << ' ';
		metaData.Write(ost);
	}
	ost << "\t";

	// Commentaire
	if (GetLabel() != "")
		ost << "// " << GetLabel();
}

void KWAttribute::WriteJSONFields(JSONFile* fJSON)
{
	ALString sOutputString;

	// Nom
	fJSON->WriteKeyString("name", GetName());

	// Commentaire
	if (GetLabel() != "")
		fJSON->WriteKeyString("label", GetLabel());

	// Attribute a ne pas utiliser
	if (not GetUsed())
		fJSON->WriteKeyBoolean("used", false);

	// Type
	fJSON->WriteKeyString("type", KWType::ToString(GetType()));

	// Classe pour un type objet
	if (KWType::IsGeneralRelation(GetType()))
	{
		if (GetClass() != NULL and GetClass()->GetName() != "")
			fJSON->WriteKeyString("objectType", GetClass()->GetName());
		else
			fJSON->WriteKeyString("objectType", "UnknownClass");
	}
	// Nom de structure pour un type Structure
	else if (GetType() == KWType::Structure)
	{
		if (GetStructureName() != "")
			fJSON->WriteKeyString("structureType", GetStructureName());
		else
			fJSON->WriteKeyString("structureType", "UnknownStructure");
	}

	// Regle de derivation
	if (kwdrRule != NULL)
	{
		kwdrRule->WriteUsedRuleToString(sOutputString);
		fJSON->WriteKeyString("rule", sOutputString);
	}

	// Meta-donnees
	if (metaData.GetKeyNumber() > 0)
		metaData.WriteJSONKeyReport(fJSON, "metaData");
}

void KWAttribute::WriteNotLoadedMetaData(ostream& ost) const
{
	KWMetaData usedNotLoadedMetaData;

	// Memorisation dans une meta-data temporaire de l'information d'utilisation d'un attribut non charge en memoire
	// Permet de transferer cette information "privee", par exemple pour une tache parallele
	if (GetUsed() and not GetLoaded())
	{
		usedNotLoadedMetaData.SetNoValueAt("_NotLoaded");
		ost << ' ';
		usedNotLoadedMetaData.Write(ost);
	}
}

void KWAttribute::ReadNotLoadedMetaData()
{
	if (GetMetaData()->GetKeyNumber() > 0 and GetMetaData()->IsMissingTypeAt("_NotLoaded"))
	{
		SetLoaded(false);
		GetMetaData()->RemoveKey("_NotLoaded");
	}
}

const ALString KWAttribute::GetClassLabel() const
{
	return "Variable";
}

const ALString KWAttribute::GetObjectLabel() const
{
	return GetName();
}

int KWAttributeCompareName(const void* elem1, const void* elem2)
{
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	attribute1 = cast(KWAttribute*, *(Object**)elem1);
	attribute2 = cast(KWAttribute*, *(Object**)elem2);

	// Difference
	nDiff = attribute1->GetName().Compare(attribute2->GetName());
	return nDiff;
}

int KWAttributeCompareBlockName(const void* elem1, const void* elem2)
{
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	attribute1 = cast(KWAttribute*, *(Object**)elem1);
	attribute2 = cast(KWAttribute*, *(Object**)elem2);

	// Difference
	if (attribute1->GetAttributeBlock() == attribute2->GetAttributeBlock())
		nDiff = attribute1->GetName().Compare(attribute2->GetName());
	else if (attribute1->GetAttributeBlock() == NULL)
		nDiff = -1;
	else if (attribute2->GetAttributeBlock() == NULL)
		nDiff = 1;
	else
		nDiff = attribute1->GetAttributeBlock()->GetName().Compare(attribute2->GetAttributeBlock()->GetName());
	return nDiff;
}

int KWAttributeCompareVarKey(const void* elem1, const void* elem2)
{
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	attribute1 = cast(KWAttribute*, *(Object**)elem1);
	attribute2 = cast(KWAttribute*, *(Object**)elem2);
	assert(attribute2->GetAttributeBlock()->GetName() == attribute1->GetAttributeBlock()->GetName());

	// Difference dans le cas d'une VarKey categorielle
	if (attribute1->GetAttributeBlock()->GetVarKeyType() == KWType::Symbol)
		nDiff = attribute1->GetAttributeBlock()
			    ->GetSymbolVarKey(attribute1)
			    .CompareValue(attribute2->GetAttributeBlock()->GetSymbolVarKey(attribute2));
	// Difference dans le cas d'une VarKey categorielle
	else
		nDiff = attribute1->GetAttributeBlock()->GetContinuousVarKey(attribute1) -
			attribute2->GetAttributeBlock()->GetContinuousVarKey(attribute2);
	return nDiff;
}
