// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDerivationRule.h"

//////////////////////////////////////////////////////////////////////////
// KWDerivationRuleOperand

KWDerivationRuleOperand::KWDerivationRuleOperand()
{
	cType = KWType::Unknown;
	cScopeLevel = 0;
	cOrigin = OriginAny;
	cCompiledOrigin = CompiledOriginConstant;
	kwvConstant.Init();
	rule = NULL;
	kwcClass = NULL;
	debug(nFreshness = 0;) debug(nClassFreshness = 0;) debug(nCompileFreshness = 0;)
}

KWDerivationRuleOperand::~KWDerivationRuleOperand()
{
	// On force le dereferencement d'une eventuelle constante Symbol
	if (cType == KWType::Symbol)
		kwvConstant.ResetSymbol();
	if (rule != NULL)
		delete rule;
}

int KWDerivationRuleOperand::GetVarKeyType() const
{
	int nVarKeyType;
	KWAttributeBlock* attributeBlock;

	// Valeur par defaut
	nVarKeyType = KWType::None;

	// Recherche dans le cas d'un bloc
	if (KWType::IsValueBlock(GetType()))
	{
		if (GetOrigin() == OriginAttribute)
		{
			if (kwcClass != NULL)
			{
				attributeBlock = kwcClass->LookupAttributeBlock(GetAttributeBlockName());
				if (attributeBlock != NULL)
					nVarKeyType = attributeBlock->GetVarKeyType();
			}
		}
		else if (GetOrigin() == OriginRule)
		{
			if (GetDerivationRule() != NULL)
				nVarKeyType = GetDerivationRule()->GetVarKeyType();
		}
	}
	return nVarKeyType;
}

ALString KWDerivationRuleOperand::OriginToString(int nOrigin, int nType)
{
	require(OriginConstant <= nOrigin and nOrigin <= OriginAny);
	require(0 <= nType and nType < KWType::Unknown);
	if (nOrigin == OriginConstant)
		return "Constant";
	else if (nOrigin == OriginAttribute)
	{
		if (KWType::IsValueBlock(nType))
			return "Sparse variable block";
		else
			return "Variable";
	}
	else if (nOrigin == OriginRule)
		return "Rule";
	else if (nOrigin == OriginAny)
		return "Any";
	else
		return "Unknown";
}

const ALString KWDerivationRuleOperand::GetExternalStringConstant() const
{
	require(KWType::IsSimple(GetType()));

	// Si litteral Symbol
	if (cType == KWType::Symbol)
		return KWClass::GetExternalSymbolConstant(GetSymbolConstant());
	// Si litteral Continuous
	else
	{
		assert(cType == KWType::Continuous);
		return KWClass::GetExternalContinuousConstant(GetContinuousConstant());
	}
}

boolean KWDerivationRuleOperand::CheckDefinition() const
{
	boolean bResult = true;

	// Type
	if (GetType() != KWType::Unknown and not KWType::Check(GetType()))
	{
		AddError("Incorrect type");
		bResult = false;
	}

	// Nom de la classe pour un type Object ou ObjectArray si renseigne
	if (KWType::IsGeneralRelation(GetType()) and GetObjectClassName() != "" and
	    not KWClass::CheckName(GetObjectClassName(), this))
	{
		AddError("Incorrect dictionary name for " + KWType::ToString(GetType()) + " type");
		bResult = false;
	}

	// Nom de structure pour un type Structure
	if (GetType() == KWType::Structure)
	{
		if (GetStructureName() == "")
		{
			AddError("Missing structure name for Structure type");
			bResult = false;
		}
		else if (not KWClass::CheckName(GetStructureName(), this))
		{
			AddError("Incorrect structure name for Structure type");
			bResult = false;
		}
	}

	// Origine
	assert(GetOrigin() == OriginConstant or GetOrigin() == OriginAttribute or GetOrigin() == OriginRule or
	       GetOrigin() == OriginAny);

	// Verification de coherence entre type d'origine et parametre de l'origine
	if (GetOrigin() == OriginAny and GetDataItemName() != "")
		AddWarning(GetDataItemLabel() + " name specified (" + GetDataItemName() +
			   ") for an operand with origin Any");
	if (GetOrigin() == OriginAny and GetDerivationRule() != NULL)
		AddWarning("Rule specified (" + GetDerivationRule()->GetName() + ") for an operand with origin Any");
	if (GetOrigin() == OriginConstant and GetDataItemName() != "")
		AddWarning(GetDataItemLabel() + " name specified (" + GetDataItemName() +
			   ") for an operand with origin Constant");
	if (GetOrigin() == OriginConstant and GetDerivationRule() != NULL)
		AddWarning("Rule specified (" + GetDerivationRule()->GetName() +
			   ") for an operand with origin Constant");
	if (GetOrigin() == OriginAttribute and GetDerivationRule() != NULL)
		AddWarning("Rule specified (" + GetDerivationRule()->GetName() +
			   ") for an operand with origin Attribute");
	if (GetOrigin() == OriginRule and GetDataItemName() != "")
		AddWarning(GetDataItemLabel() + " name specified (" + GetDataItemName() +
			   ") for an operand with origin Rule");

	// Verification eventuelle du type de constante
	if (GetOrigin() == OriginConstant and not KWType::IsSimple(GetType()))
	{
		AddError("Incorrect variable type for a constant value");
		bResult = false;
	}

	// Verification de la compatibilite entre niveau de scope et origine de l'operande
	if (GetOrigin() == OriginConstant and GetScopeLevel() > 0)
	{
		AddError("Constant value cannot be used with scope prefix (" + ALString('.', GetScopeLevel()) + ")");
		bResult = false;
	}

	// Verification eventuelle de l'attribut
	if (GetOrigin() == OriginAttribute and GetDataItemName() != "" and
	    not KWClass::CheckName(GetDataItemName(), this))
	{
		AddError(GetDataItemLabel() + " name is not correct in the operand");
		bResult = false;
	}

	// Verification eventuelle de la regle
	if (GetOrigin() == OriginRule and GetDerivationRule() != NULL and not GetDerivationRule()->CheckDefinition())
	{
		AddError("Incorrect rule used in the operand");
		bResult = false;
	}

	return bResult;
}

boolean KWDerivationRuleOperand::CheckFamily(const KWDerivationRuleOperand* operandFamily) const
{
	boolean bResult = true;
	KWDerivationRule* kwdrReference;
	ALString sTmp;

	require(operandFamily != NULL);
	require(operandFamily->CheckDefinition());
	require(CheckDefinition());

	// Type
	if (GetType() != operandFamily->GetType())
	{
		AddError("Type " + KWType::ToString(GetType()) + " inconsistent with that of the operand (" +
			 KWType::ToString(operandFamily->GetType()) + ")");
		bResult = false;
	}

	// Nom de la classe pour un type Object ou ObjectArray si renseigne
	if (bResult and KWType::IsGeneralRelation(GetType()) and operandFamily->GetObjectClassName() != "" and
	    GetObjectClassName() != operandFamily->GetObjectClassName())
	{
		AddError("Dictionary name for " + KWType::ToString(GetType()) + " type " + GetObjectClassName() +
			 " inconsistent with that of the operand (" + operandFamily->GetObjectClassName() + ")");
		bResult = false;
	}

	// Nom de structure pour un type Structure
	if (bResult and GetType() == KWType::Structure and operandFamily->GetStructureName() != "" and
	    GetStructureName() != operandFamily->GetStructureName())
	{
		AddError("Structure name for Structure type " + GetStructureName() +
			 " inconsistent with that of the operand (" + operandFamily->GetStructureName() + ")");
		bResult = false;
	}

	// Origine
	if (operandFamily->GetOrigin() != OriginAny and GetOrigin() != operandFamily->GetOrigin())
	{
		AddError("Origin (" + OriginToString(GetOrigin(), GetType()) +
			 ") inconsistent with that of the operand (" +
			 OriginToString(operandFamily->GetOrigin(), operandFamily->GetType()) + ")");
		bResult = false;
	}

	// Origine attribut
	if (operandFamily->GetOrigin() == OriginAttribute and operandFamily->GetDataItemName() != "" and
	    GetDataItemName() != operandFamily->GetDataItemName())
	{
		AddError(GetDataItemLabel() + " name for the origin of the value (" + GetDataItemName() +
			 ") inconsistent with that of the operand (" + operandFamily->GetDataItemName() + ")");
		bResult = false;
	}

	// Origine regle
	if (operandFamily->GetOrigin() == OriginRule and operandFamily->GetDerivationRule() != NULL)
	{
		if (GetDerivationRule() == NULL)
		{
			AddError("Missing rule for the operand");
			bResult = false;
		}
		else if (not GetDerivationRule()->CheckFamily(operandFamily->GetDerivationRule()))
		{
			AddError(sTmp + "Rule inconsistent with that of the operand");
			bResult = false;
		}
	}

	// Verification de la famille de la regle si l'operande se base sur une regle
	if (bResult and GetDerivationRule() != NULL)
	{
		// Verification de l'existence de la regle de derivation
		kwdrReference = KWDerivationRule::LookupDerivationRule(GetDerivationRule()->GetName());
		if (kwdrReference == NULL)
		{
			AddError(GetDerivationRule()->GetClassLabel() + " " + GetDerivationRule()->GetName() +
				 " does not exist");
			bResult = false;
		}

		// Verification de la coherence avec la regle de derivation enregistree
		if (bResult and kwdrReference != NULL)
		{
			assert(kwdrReference->GetName() == GetDerivationRule()->GetName());

			if (not GetDerivationRule()->CheckFamily(kwdrReference))
			{
				AddError(GetDerivationRule()->GetClassLabel() + " " + GetDerivationRule()->GetName() +
					 " incorrectly specified");
				bResult = false;
			}
		}
	}
	return bResult;
}

boolean KWDerivationRuleOperand::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bResult = true;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	const KWClass* scopeClass;
	ALString sScopeMessage;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Test de definition de la regle: arret si incorrect
	if (not CheckDefinition())
		return false;

	// La regle predefinie de resolution des references aux objets ne peut etre utilisee
	// comme operande d'une regle de derivation
	if (GetOrigin() == OriginRule and GetDerivationRule() != NULL and
	    GetDerivationRule()->GetName() == KWDerivationRule::GetReferenceRuleName())
	{
		AddError("Reference not allowed in an operand of a rule");
		return false;
	}

	// On ne peut peut pas acceder a un bloc via une regle, qui ne peut parametrer
	// l'ensemble des variables a utiliser, avec leur varkeys
	if (GetOrigin() == OriginRule and GetDerivationRule() != NULL and KWType::IsValueBlock(GetType()))
	{
		AddError("Derivation rule returning a sparse value block not allowed in an operand of a rule");
		return false;
	}
	if (GetOrigin() != OriginAttribute and KWType::IsValueBlock(GetType()))
	{
		AddError("A sparse value block operand of a rule can only be a sparse variable block (not a rule)");
		return false;
	}

	// L'origine doit etre specifiee
	if (GetOrigin() == OriginAny)
	{
		AddError(" Missing origin for the operand");
		bResult = false;
	}

	// Recherche de la classe du bon scope
	scopeClass = kwcOwnerClass;
	if (GetScopeLevel() > 0)
	{
		// Test si la profondeur du scope est suffisante
		if (GetScopeLevel() > GetScopeDepth(kwcOwnerClass))
		{
			AddError("Scope prefix (" + ALString('.', GetScopeLevel()) +
				 ") goes beyond the depth of the calling rules");
			bResult = false;
		}
		// Acces a la classe du bin scope sinon
		else
		{
			scopeClass = GetClassAtScope(kwcOwnerClass, GetScopeLevel());
		}
	}

	// Verification eventuelle de l'attribut si la classe de scope est correcte
	// On effectue cette verification meme en cas d'erreur, pour avoir un diagnostique potentiellement plus precis
	if (bResult and GetOrigin() == OriginAttribute)
	{
		assert((GetScopeLevel() == 0 and scopeClass == kwcOwnerClass) or
		       scopeClass == GetClassAtScope(kwcOwnerClass, GetScopeLevel()));

		// Attribut renseigne
		if (GetDataItemName() == "")
		{
			AddError(GetDataItemLabel() + " name is missing in the operand");
			bResult = false;
		}
		else
		{
			// Creation d'un message lie au scope
			if (GetScopeLevel() == 0)
				sScopeMessage = " (in dictionary " + scopeClass->GetName() + ")";
			else
				sScopeMessage = " (from dictionary " + scopeClass->GetName() + " at scope level " +
						ALString('.', GetScopeLevel()) + ")";

			// Cas d'un attribut
			if (KWType::IsValue(GetType()))
			{
				// Attribut existant dans la classe
				attribute = scopeClass->LookupAttribute(GetAttributeName());
				if (attribute == NULL)
				{
					AddError("Variable " + GetAttributeName() + " not found" + sScopeMessage);
					bResult = false;
				}
				// Coherence avec le type de l'operande
				else if (attribute->GetType() != GetType())
				{
					AddError("Variable " + GetAttributeName() + sScopeMessage + " with type " +
						 KWType::ToString(attribute->GetType()) +
						 " inconsistent with that of the operand (" +
						 KWType::ToString(GetType()) + ")");
					bResult = false;
				}
				// Coherence avec le type objet de l'operande (hors bloc dans ce cas)
				else if (KWType::IsRelation(GetType()) and attribute->GetClass() != NULL and
					 attribute->GetClass()->GetName() != GetObjectClassName())
				{
					AddError("Variable " + GetAttributeName() + sScopeMessage + " with type " +
						 KWType::ToString(GetType()) + "(" + attribute->GetClass()->GetName() +
						 ")" + " inconsistent with that of the operand (" +
						 KWType::ToString(GetType()) + "(" + GetObjectClassName() + "))");
					bResult = false;
				}
				// Coherence avec le nom de structure de l'operande
				else if (GetType() == KWType::Structure and
					 attribute->GetStructureName() != GetStructureName())
				{
					AddError("Variable " + GetAttributeName() + sScopeMessage +
						 " with Structure type " + attribute->GetStructureName() +
						 " inconsistent with that of the operand (" + GetStructureName() + ")");
					bResult = false;
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				// Bloc d'attribut existant dans la classe
				attributeBlock = scopeClass->LookupAttributeBlock(GetAttributeBlockName());
				if (attributeBlock == NULL)
				{
					AddError("Sparse variable block " + GetAttributeBlockName() + " not found" +
						 sScopeMessage);
					bResult = false;
				}
				// Coherence avec le type de l'operande
				else if (attributeBlock->GetBlockType() != GetType())
				{
					AddError("Sparse variable block " + GetAttributeBlockName() + sScopeMessage +
						 " with type " + KWType::ToString(attributeBlock->GetType()) +
						 " inconsistent with that of the operand (" +
						 KWType::ToString(GetType()) + ")");
					bResult = false;
				}
				// Coherence avec le type objet de l'operande
				else if (GetType() == KWType::ObjectArrayValueBlock and
					 attributeBlock->GetFirstAttribute()->GetClass() != NULL and
					 attributeBlock->GetFirstAttribute()->GetClass()->GetName() !=
					     GetObjectClassName())
				{
					AddError("Sparse variable block " + GetAttributeBlockName() + sScopeMessage +
						 " with type " + KWType::ToString(GetType()) + "(" +
						 attributeBlock->GetFirstAttribute()->GetClass()->GetName() + ")" +
						 " inconsistent with that of the operand (" +
						 KWType::ToString(GetType()) + "(" + GetObjectClassName() + "))");
					bResult = false;
				}
			}
		}
	}

	// Verification eventuelle de la regle
	if (bResult and GetOrigin() == OriginRule)
	{
		assert((GetScopeLevel() == 0 and scopeClass == kwcOwnerClass) or
		       scopeClass == GetClassAtScope(kwcOwnerClass, GetScopeLevel()));

		// Regle renseignee
		if (GetDerivationRule() == NULL)
		{
			AddError("Missing rule for the operand");
			bResult = false;
		}
		// Regle correcte
		else if (not GetDerivationRule()->CheckCompleteness(scopeClass))
		{
			AddError("Incomplete rule for the operand");
			bResult = false;
		}
		// Type retour de la regle coherent avec celui de l'operande
		else if (GetDerivationRule()->GetType() != GetType())
		{
			AddError("Rule " + GetDerivationRule()->GetName() + " with type " +
				 KWType::ToString(GetDerivationRule()->GetType()) +
				 " inconsistent with that of the operand (" + KWType::ToString(GetType()) + ")");
			bResult = false;
		}
		// Coherence avec le type objet de l'operande
		else if (KWType::IsGeneralRelation(GetType()) and
			 GetDerivationRule()->GetObjectClassName() != GetObjectClassName())
		{
			AddError("Rule " + GetDerivationRule()->GetName() + " with type " +
				 KWType::ToString(GetType()) + "(" + GetDerivationRule()->GetObjectClassName() + ")" +
				 " inconsistent with that of the operand (" + KWType::ToString(GetType()) + "(" +
				 GetObjectClassName() + "))");
			bResult = false;
		}
		// Coherence avec le nom de structure de l'operande
		else if (GetType() == KWType::Structure and
			 GetDerivationRule()->GetStructureName() != GetStructureName())
		{
			AddError("Rule " + GetDerivationRule()->GetName() + " with Structure type " +
				 GetDerivationRule()->GetStructureName() + " inconsistent with that of the operand (" +
				 GetStructureName() + ")");
			bResult = false;
		}
	}

	// Nom de la classe pour un type Object ou ObjectArray si renseigne
	// On effectue cette verification en dernier, car l'absence de renseignement
	// de la classe peut etre du a un probleme diagnostique precedemment
	// (et ayant entraine un probleme dans le CompleteTypeInfo)
	if (bResult and KWType::IsGeneralRelation(GetType()))
	{
		// Classe renseignee
		if (GetObjectClassName() == "")
		{
			AddError("Missing dictionary name for type " + KWType::ToString(GetType()));
			bResult = false;
		}
		// Classe existante dans le domaine
		else if (kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName()) == NULL)
		{
			AddError("Unknown dictionary name (" + GetObjectClassName() + ") for type " +
				 KWType::ToString(GetType()));
			bResult = false;
		}
	}

	return bResult;
}

void KWDerivationRuleOperand::CompleteTypeInfo(const KWClass* kwcOwnerClass)
{
	NumericKeyDictionary nkdCompletedAttributes;
	InternalCompleteTypeInfo(kwcOwnerClass, &nkdCompletedAttributes);
}

void KWDerivationRuleOperand::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						       NumericKeyDictionary* nkdCompletedAttributes)
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Acces a la classe du bon scope
	scopeClass = kwcOwnerClass;
	if (GetScopeLevel() > 0)
	{
		// Recherche dans la pile des scopes
		if (GetScopeLevel() <= GetScopeDepth(kwcOwnerClass))
			scopeClass = GetClassAtScope(kwcOwnerClass, GetScopeLevel());
		// Arret si non trouve
		else
			return;
	}

	// Memorisation de la classe
	kwcClass = kwcOwnerClass;
	debug(nClassFreshness = kwcClass->GetFreshness());

	// Informations de type si attribut
	if (GetOrigin() == OriginAttribute)
	{
		// Cas ou il s'agit dun attribut
		attribute = scopeClass->LookupAttribute(GetDataItemName());
		if (attribute != NULL)
		{
			// Pas de completion des infos de l'attribut: la propagation se fait recursivement dans les
			// regles sans impact sur les attributs ou la classe, ce qui est necessaire pour completer des
			// regles exterieures a la classe
			// On evite ainsi l'instruction: attribute->InternalCompleteTypeInfo(kwcOwnerClass,
			// nkdCompletedAttributes);

			// Recuperation des informations de l'attribut
			SetType(attribute->GetType());
			if (KWType::IsRelation(attribute->GetType()) and GetObjectClassName() == "" and
			    attribute->GetClass() != NULL)
				SetObjectClassName(attribute->GetClass()->GetName());
			if (attribute->GetType() == KWType::Structure)
				SetStructureName(attribute->GetStructureName());
		}
		// Sinon, on envisage le cas d'un bloc
		else
		{
			attributeBlock = scopeClass->LookupAttributeBlock(GetDataItemName());
			if (attributeBlock != NULL)
			{
				// Recuperation des informations du bloc d'attributs
				SetType(attributeBlock->GetBlockType());
				if (attributeBlock->GetBlockType() == KWType::ObjectArrayValueBlock and
				    GetObjectClassName() == "" and attributeBlock->GetClass() != NULL)
					SetObjectClassName(attributeBlock->GetClass()->GetName());
			}
		}
	}

	// Informations de type si regle
	if (GetOrigin() == OriginRule and GetDerivationRule() != NULL)
	{
		// Completion des infos de la regle
		rule->InternalCompleteTypeInfo(scopeClass, nkdCompletedAttributes);

		// Recuperation des informations de la regle
		SetType(rule->GetType());
		if (KWType::IsGeneralRelation(rule->GetType()))
			SetObjectClassName(rule->GetObjectClassName());
		if (rule->GetType() == KWType::Structure)
			SetStructureName(rule->GetStructureName());
	}
}

void KWDerivationRuleOperand::ComputeUpperScopeValue(const KWObject* kwoObject)
{
	require(GetScopeLevel() > 0);
	require(GetOrigin() == OriginAttribute or GetOrigin() == OriginRule);
	require(kwvConstant.GetContinuous() == 0);
	debug(require(IsCompiled()));

	// Calcul de la valeur de l'operande selon son type
	switch (GetType())
	{
	case KWType::Continuous:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetContinuous(kwoObject->ComputeContinuousValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetContinuous(GetDerivationRule()->ComputeContinuousResult(kwoObject));
		break;
	case KWType::Symbol:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetSymbol(kwoObject->ComputeSymbolValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetSymbol(GetDerivationRule()->ComputeSymbolResult(kwoObject));
		break;
	case KWType::Date:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetDate(kwoObject->ComputeDateValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetDate(GetDerivationRule()->ComputeDateResult(kwoObject));
		break;
	case KWType::Time:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetTime(kwoObject->ComputeTimeValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetTime(GetDerivationRule()->ComputeTimeResult(kwoObject));
		break;
	case KWType::Timestamp:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetTimestamp(kwoObject->ComputeTimestampValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetTimestamp(GetDerivationRule()->ComputeTimestampResult(kwoObject));
		break;
	case KWType::TimestampTZ:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetTimestampTZ(kwoObject->ComputeTimestampTZValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetTimestampTZ(GetDerivationRule()->ComputeTimestampTZResult(kwoObject));
		break;
	case KWType::Text:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetText(kwoObject->ComputeTextValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetText(GetDerivationRule()->ComputeTextResult(kwoObject));
		break;
	case KWType::TextList:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetTextList(kwoObject->ComputeTextListValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetTextList(GetDerivationRule()->ComputeTextListResult(kwoObject));
		break;
	case KWType::Object:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetObject(kwoObject->ComputeObjectValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetObject(GetDerivationRule()->ComputeObjectResult(kwoObject));
		break;
	case KWType::ObjectArray:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetObjectArray(kwoObject->ComputeObjectArrayValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetObjectArray(GetDerivationRule()->ComputeObjectArrayResult(kwoObject));
		break;
	case KWType::Structure:
		if (GetOrigin() == OriginAttribute)
			kwvConstant.SetStructure(kwoObject->ComputeStructureValueAt(liDataItemLoadIndex));
		else
			kwvConstant.SetStructure(GetDerivationRule()->ComputeStructureResult(kwoObject));
		break;
	case KWType::ContinuousValueBlock:
		assert(GetOrigin() == OriginAttribute);
		kwvConstant.SetContinuousValueBlock(kwoObject->ComputeContinuousValueBlockAt(liDataItemLoadIndex));
		break;
	case KWType::SymbolValueBlock:
		assert(GetOrigin() == OriginAttribute);
		kwvConstant.SetSymbolValueBlock(kwoObject->ComputeSymbolValueBlockAt(liDataItemLoadIndex));
		break;
	case KWType::ObjectArrayValueBlock:
		assert(GetOrigin() == OriginAttribute);
		kwvConstant.SetObjectArrayValueBlock(kwoObject->ComputeObjectArrayValueBlockAt(liDataItemLoadIndex));
		break;
	}
}

void KWDerivationRuleOperand::InitUpperScopeValue()
{
	require(GetScopeLevel() > 0);
	require(GetOrigin() == OriginAttribute or GetOrigin() == OriginRule);
	debug(require(IsCompiled()));

	// Dans le cas Symbol, il faut reinitialiser explicitement avec le bon type pour la gestion
	// des compteurs de reference
	if (GetType() == KWType::Symbol)
		kwvConstant.ResetSymbol();
	// Dans le cas particulier des blocs, il n'est pas necessaire de nettoyer la memoire
	// puisque le bloc ne peut venir d'uine regle
	assert(not KWType::IsValueBlock(GetType()) or GetOrigin() != OriginRule);

	// On reinitialise dans tous les cas
	kwvConstant.Init();
}

KWDerivationRule* KWDerivationRuleOperand::GetReferencedDerivationRule(const KWClass* kwcOwnerClass) const
{
	KWDerivationRule* referencedRule;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);

	// Acces directe a une regle
	referencedRule = NULL;
	if (GetOrigin() == OriginRule)
	{
		referencedRule = GetDerivationRule();
	}
	// Acces indirect potentiel a une regle via un attribute
	else if (GetOrigin() == OriginAttribute)
	{
		// Acces a la classe du bon scope
		scopeClass = kwcOwnerClass;
		if (GetScopeLevel() > 0)
		{
			// Recherche dans la pile des scopes
			if (GetScopeLevel() <= GetScopeDepth(kwcOwnerClass))
				scopeClass = GetClassAtScope(kwcOwnerClass, GetScopeLevel());
			// Arret si non trouve
			else
				return NULL;
		}

		// Recherche de l'attribut dans la classe du bon scope
		if (KWType::IsValue(GetType()))
		{
			attribute = scopeClass->LookupAttribute(GetAttributeName());
			if (attribute != NULL)
				referencedRule = attribute->GetDerivationRule();
		}
		// Ou du bloc d'attribut
		else
		{
			attributeBlock = scopeClass->LookupAttributeBlock(GetAttributeBlockName());
			if (attributeBlock != NULL)
				referencedRule = attributeBlock->GetDerivationRule();
		}
	}
	return referencedRule;
}

void KWDerivationRuleOperand::Compile(KWClass* kwcOwnerClass)
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWClass* scopeClass;
	KWDerivationRule* scopeRule;

	require(kwcOwnerClass != NULL);
	require(CheckCompleteness(kwcOwnerClass));

	///////////////////////////////////////////////////////
	// Compilation systematique
	// La fraicheur est gere en permanence pour les regles
	// Pour les operandes, elle ne sert qu'a controler l'acces
	// a la valeur si l'operande est compilee (controle en mode debug)

	// Enregistrement de la fraicheur de compilation
	debug(nCompileFreshness = nFreshness);

	// Acces a la classe du bon scope
	scopeClass = kwcOwnerClass;
	if (GetScopeLevel() > 0)
	{
		scopeClass = GetClassAtScope(kwcOwnerClass, GetScopeLevel());

		// Enregistrement de l'operande dans la regle du bon scope
		scopeRule = GetRuleAtScope(kwcOwnerClass, GetScopeLevel());
		scopeRule->AddMainScopeSecondaryOperands(kwcOwnerClass, this);
	}

	// Compilation de l'origine
	if (GetScopeLevel() > 0 or GetOrigin() == OriginConstant)
		cCompiledOrigin = CompiledOriginConstant;
	else if (cOrigin == OriginAttribute)
		cCompiledOrigin = CompiledOriginAttribute;
	else
		cCompiledOrigin = CompiledOriginRule;

	// Gestion de la classe
	kwcClass = scopeClass;
	debug(nClassFreshness = kwcClass->GetFreshness());

	// Calcul de l'index de l'attribut
	liDataItemLoadIndex.Reset();
	if (GetOrigin() == OriginAttribute)
	{
		// Cas d'un attribut
		if (KWType::IsValue(GetType()))
		{
			attribute = scopeClass->LookupAttribute(GetAttributeName());
			check(attribute);
			liDataItemLoadIndex = attribute->GetLoadIndex();

			// Compilation d'eventuelle regle portee par l'attribut
			if (attribute->GetDerivationRule() != NULL)
			{
				check(attribute->GetDerivationRule());
				attribute->GetDerivationRule()->Compile(scopeClass);
			}
		}
		// Cas d'un bloc d'attributs
		else
		{
			attributeBlock = scopeClass->LookupAttributeBlock(GetAttributeBlockName());
			check(attributeBlock);
			liDataItemLoadIndex = attributeBlock->GetLoadIndex();

			// Compilation d'eventuelle regle portee par l'attribut
			if (attributeBlock->GetDerivationRule() != NULL)
			{
				check(attributeBlock->GetDerivationRule());
				attributeBlock->GetDerivationRule()->Compile(scopeClass);
			}
		}
	}

	// Compilation de la regle de l'operande de la regle
	if (GetOrigin() == OriginRule)
	{
		check(GetDerivationRule());
		GetDerivationRule()->Compile(scopeClass);
	}
	debug(ensure(IsCompiled()));
}

void KWDerivationRuleOperand::RenameAttribute(const KWClass* kwcOwnerClass, KWAttribute* refAttribute,
					      const ALString& sNewAttributeName)
{
	KWClass* refClass;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(refAttribute != NULL);
	require(refAttribute->GetParentClass() != NULL);
	require(refAttribute->GetParentClass()->LookupAttribute(refAttribute->GetName()) == refAttribute);

	// Acces a la classe du scope
	refClass = refAttribute->GetParentClass();
	scopeClass = kwcOwnerClass;
	if (GetScopeLevel() > 0 and GetScopeLevel() <= GetScopeDepth(kwcOwnerClass))
		scopeClass = GetClassAtScope(kwcOwnerClass, GetScopeLevel());

	// Renommage si necessaire
	if (GetAttributeName() == refAttribute->GetName())
	{
		if (refClass == scopeClass)
			SetAttributeName(sNewAttributeName);
	}

	// Propagation aux sous-regles
	if (GetDerivationRule() != NULL and scopeClass != NULL)
		GetDerivationRule()->RenameAttribute(scopeClass, refAttribute, sNewAttributeName);
}

const ALString KWDerivationRuleOperand::ComputeOperandName() const
{
	ALString sOperandName;

	// Prefixage en fonction de niveau de scope
	sOperandName = ALString('.', GetScopeLevel());

	// On complete selon la nature de l'operande
	if (cOrigin == OriginAttribute)
		sOperandName += GetDataItemName();
	else if (cOrigin == OriginConstant)
		sOperandName += GetStringConstant();
	else if (GetDerivationRule() != NULL)
		sOperandName += GetDerivationRule()->ComputeAttributeName();
	return sOperandName;
}

KWDerivationRuleOperand* KWDerivationRuleOperand::Clone() const
{
	KWDerivationRuleOperand* kwdroClone;

	kwdroClone = new KWDerivationRuleOperand;
	kwdroClone->cType = cType;
	kwdroClone->usSupplementTypeName = usSupplementTypeName;
	kwdroClone->cScopeLevel = cScopeLevel;
	kwdroClone->cOrigin = cOrigin;
	kwdroClone->cCompiledOrigin = cCompiledOrigin;

	// Attention a la gestion automatique des Symbol
	switch (cType)
	{
	case KWType::Continuous:
		kwdroClone->kwvConstant.SetContinuous(kwvConstant.GetContinuous());
		break;
	case KWType::Symbol:
		kwdroClone->kwvConstant.SetSymbol(kwvConstant.GetSymbol());
		break;
	case KWType::Object:
		kwdroClone->kwvConstant.SetObject(kwvConstant.GetObject());
		break;
	case KWType::ObjectArray:
		kwdroClone->kwvConstant.SetObjectArray(kwvConstant.GetObjectArray());
		break;
	case KWType::Structure:
		kwdroClone->kwvConstant.SetStructure(kwvConstant.GetStructure());
		break;
	default:
		kwdroClone->kwvConstant = kwvConstant;
		break;
	}
	kwdroClone->usDataItemName = usDataItemName;
	if (rule != NULL)
		kwdroClone->rule = rule->Clone();

	// Pas de memorisation des resultats de compilation
	// La nouvelle version est a recompiler
	kwdroClone->kwcClass = NULL;
	kwdroClone->liDataItemLoadIndex.Reset();
	debug(kwdroClone->nFreshness = nFreshness;) debug(kwdroClone->nClassFreshness = 0;)
	    debug(kwdroClone->nCompileFreshness = 0;) return kwdroClone;
}

boolean KWDerivationRuleOperand::Check() const
{
	return CheckDefinition();
}

longint KWDerivationRuleOperand::GetUsedMemory() const
{
	longint lUsedMemory;

	// Prise en compte de l'operande lui-meme
	// On ne prend pas en compte la memoire des UniqueString, car il elle est deja comptee par ailleurs
	lUsedMemory = sizeof(KWDerivationRuleOperand);

	// Prise en compte de l'eventuelle regle de derivation
	if (rule != NULL)
		lUsedMemory += rule->GetUsedMemory();
	return lUsedMemory;
}

longint KWDerivationRuleOperand::ComputeHashValue() const
{
	longint lHash;

	// Prise en compte de l'operande lui-meme
	lHash = HashValue(KWType::ToString(cType));
	lHash = LongintUpdateHashValue(lHash, HashValue(GetSupplementTypeName()));
	lHash = LongintUpdateHashValue(lHash, cScopeLevel * 97);

	// Prise en compte de l'origine de la valeur
	lHash = LongintUpdateHashValue(lHash, cOrigin * 17);
	if (cOrigin == OriginConstant)
		lHash = LongintUpdateHashValue(lHash, HashValue(GetStringConstant()));
	else if (cOrigin == OriginAttribute)
		lHash = LongintUpdateHashValue(lHash, HashValue(GetDataItemName()));
	else if (cOrigin == OriginRule and rule != NULL)
		lHash = LongintUpdateHashValue(lHash, rule->ComputeHashValue());
	return lHash;
}

void KWDerivationRuleOperand::Write(ostream& ost) const
{
	int i;

	// Niveau de scope
	for (i = 0; i < GetScopeLevel(); i++)
		ost << '.';

	// Type
	ost << KWType::ToString(GetType());

	// Classe pour un type objet
	if (KWType::IsGeneralRelation(GetType()))
	{
		if (GetObjectClassName() == "")
			ost << "()";
		else
			ost << "(" << KWClass::GetExternalName(GetObjectClassName()) << ")";
	}
	// Nom de structure pour un type Structure
	else if (GetType() == KWType::Structure)
	{
		if (GetStructureName() == "")
			ost << "()";
		else
			ost << "(" << KWClass::GetExternalName(GetStructureName()) << ")";
	}
	ost << " ";

	// Origine
	ost << "(" << OriginToString(GetOrigin(), GetType()) << ") ";
	if (cOrigin == OriginAny)
		ost << "???";
	else if (cOrigin == OriginAttribute)
		ost << KWClass::GetExternalName(GetDataItemName());
	else if (cOrigin == OriginConstant)
		ost << GetExternalStringConstant();
	else if (GetDerivationRule() != NULL)
		GetDerivationRule()->Write(ost);
	else
		ost << "UnknownRule()";
}

void KWDerivationRuleOperand::WriteUsedOperand(ostream& ost) const
{
	int i;

	// Niveau de scope
	for (i = 0; i < GetScopeLevel(); i++)
		ost << '.';

	// On complete selon la nature de l'operande
	if (cOrigin == OriginAny)
		ost << "???";
	else if (cOrigin == OriginAttribute)
		ost << KWClass::GetExternalName(GetDataItemName());
	else if (cOrigin == OriginConstant)
		ost << GetExternalStringConstant();
	else if (GetDerivationRule() != NULL)
		GetDerivationRule()->WriteUsedRule(ost);
	else
		ost << "UnknownRule()";
}

const ALString KWDerivationRuleOperand::GetClassLabel() const
{
	return "Operand";
}

const ALString KWDerivationRuleOperand::GetObjectLabel() const
{
	ALString sLabel;
	int i;

	// Type
	sLabel = KWType::ToString(GetType());

	// Classe pour un type objet
	if (KWType::IsGeneralRelation(GetType()))
	{
		if (GetObjectClassName() == "")
			sLabel += "()";
		else
			sLabel += "(" + KWClass::GetExternalName(GetObjectClassName()) + ")";
	}
	// Nom de structure pour un type Structure
	else if (GetType() == KWType::Structure)
	{
		if (GetStructureName() == "")
			sLabel += "()";
		else
			sLabel += "(" + KWClass::GetExternalName(GetStructureName()) + ")";
	}

	// Niveau de scope
	sLabel += " ";
	for (i = 0; i < GetScopeLevel(); i++)
		sLabel += '.';

	// Valeur de l'operande selon son origine
	if (GetOrigin() == OriginConstant and KWType::IsSimple(GetType()))
		sLabel += GetExternalStringConstant();
	else if (GetOrigin() == OriginAttribute and GetDataItemName() != "")
		sLabel += GetDataItemName();
	else if (GetOrigin() == OriginRule and GetDerivationRule() != NULL)
		sLabel += GetDerivationRule()->GetName() + "()";
	return sLabel;
}

int KWDerivationRuleOperand::GetScopeDepth(const KWClass* kwcOwnerClass) const
{
	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	assert(kwcOwnerClass->GetDomain()->GetScopeClasses()->GetSize() ==
	       kwcOwnerClass->GetDomain()->GetScopeRules()->GetSize());
	return kwcOwnerClass->GetDomain()->GetScopeClasses()->GetSize();
}

KWClass* KWDerivationRuleOperand::GetClassAtScope(const KWClass* kwcOwnerClass, int nIndex) const
{
	KWClassDomain* ownerDomain;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(1 <= nIndex and nIndex <= GetScopeDepth(kwcOwnerClass));

	ownerDomain = kwcOwnerClass->GetDomain();
	return cast(KWClass*,
		    ownerDomain->GetScopeClasses()->GetAt(ownerDomain->GetScopeClasses()->GetSize() - nIndex));
}

KWDerivationRule* KWDerivationRuleOperand::GetRuleAtScope(const KWClass* kwcOwnerClass, int nIndex) const
{
	KWClassDomain* ownerDomain;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(1 <= nIndex and nIndex <= GetScopeDepth(kwcOwnerClass));

	ownerDomain = kwcOwnerClass->GetDomain();
	return cast(KWDerivationRule*,
		    ownerDomain->GetScopeRules()->GetAt(ownerDomain->GetScopeRules()->GetSize() - nIndex));
}