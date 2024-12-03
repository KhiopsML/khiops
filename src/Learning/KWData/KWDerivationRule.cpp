// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDerivationRule.h"

////////////////////////////////////////////////////////////////////////
// KWDerivationRule

KWDerivationRule::KWDerivationRule()
{
	cType = KWType::Unknown;
	bVariableOperandNumber = false;
	bMultipleScope = false;
	oaMainScopeSecondaryOperands = NULL;
	kwcClass = NULL;
	nFreshness = 0;
	nClassFreshness = 0;
	nCompileFreshness = 0;
}

KWDerivationRule::~KWDerivationRule()
{
	oaOperands.DeleteAll();
	if (oaMainScopeSecondaryOperands != NULL)
		delete oaMainScopeSecondaryOperands;
}

void KWDerivationRule::RenameClass(const KWClass* refClass, const ALString& sNewClassName)
{
	int i;
	KWDerivationRuleOperand* operand;

	require(refClass != NULL);

	if (sNewClassName != refClass->GetName())
	{
		// Classe de rattachement de la regle
		if (GetClassName() == refClass->GetName())
			SetClassName(sNewClassName);

		// Type objet de la regle
		if (KWType::IsGeneralRelation(GetType()) and GetObjectClassName() == refClass->GetName())
			SetObjectClassName(sNewClassName);

		// Parcours des operandes
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Type objet de l'operande
			if (KWType::IsGeneralRelation(operand->GetType()) and
			    operand->GetObjectClassName() == refClass->GetName())
				operand->SetObjectClassName(sNewClassName);

			// Propagation aux sous-regles
			if (operand->GetDerivationRule() != NULL)
				operand->GetDerivationRule()->RenameClass(refClass, sNewClassName);
		}
	}
}

int KWDerivationRule::GetVarKeyType() const
{
	return KWType::None;
}

void KWDerivationRule::SetOperandNumber(int nValue)
{
	int i;

	require(nValue >= 0);

	// Supression des operandes surnumeraires
	if (nValue < oaOperands.GetSize())
	{
		for (i = nValue; i < oaOperands.GetSize(); i++)
			delete oaOperands.GetAt(i);
		oaOperands.SetSize(nValue);
	}
	// Ajout des operandes en plus
	else if (nValue > oaOperands.GetSize())
	{
		for (i = oaOperands.GetSize(); i < nValue; i++)
			oaOperands.Add(new KWDerivationRuleOperand);
	}
	nFreshness++;
}

void KWDerivationRule::AddOperand(KWDerivationRuleOperand* operand)
{
	require(operand != NULL);

	oaOperands.Add(operand);
	nFreshness++;
}

void KWDerivationRule::DeleteAllOperands()
{
	oaOperands.DeleteAll();
	nFreshness++;
}

void KWDerivationRule::RemoveAllOperands()
{
	oaOperands.RemoveAll();
	nFreshness++;
}

void KWDerivationRule::DeleteAllVariableOperands()
{
	KWDerivationRule* kwdrReference;
	int nOperand;

	require(KWDerivationRule::LookupDerivationRule(GetName()) != NULL);

	// Recherche de la regle de derivation de reference
	kwdrReference = KWDerivationRule::LookupDerivationRule(GetName());

	// Supression des derniers operandes si la regle est a nombre variables d'operandes
	if (kwdrReference->GetVariableOperandNumber())
	{
		// Supression des derniers operandes
		assert(kwdrReference->GetOperandNumber() >= 1);
		for (nOperand = kwdrReference->GetOperandNumber() - 1; nOperand < GetOperandNumber(); nOperand++)
		{
			delete oaOperands.GetAt(nOperand);
			oaOperands.SetAt(nOperand, NULL);
		}
		oaOperands.SetSize(kwdrReference->GetOperandNumber() - 1);

		// Mise a jour de la fraicheur
		nFreshness++;
	}
}

void KWDerivationRule::BuildAllUsedOperands(NumericKeyDictionary* nkdAllUsedOperands) const
{
	int nOperand;
	KWDerivationRuleOperand* operand;
	KWAttribute* originAttribute;
	KWAttributeBlock* originAttributeBlock;
	KWDerivationRule* originRule;

	require(IsCompiled());
	require(nkdAllUsedOperands != NULL);

	// Alimentation par parcours des operandes de la regle
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Ajout de l'operande si non deja traite
		if (nkdAllUsedOperands->Lookup(operand) == NULL)
		{
			nkdAllUsedOperands->SetAt(operand, operand);

			// Recherche recursive si presence effective d'une regle
			if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule and
			    operand->GetDerivationRule() != NULL)
				operand->GetDerivationRule()->BuildAllUsedOperands(nkdAllUsedOperands);
			// Recherche recursive si presence effective d'un attribut calcule avec une regle
			else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
			{
				// Cas d'un attribut
				if (KWType::IsValue(operand->GetType()))
				{
					originAttribute = operand->GetOriginAttribute();
					if (originAttribute != NULL)
					{
						// Acces a la regle d'attribut ou de bloc
						originRule = originAttribute->GetAnyDerivationRule();
						if (originRule != NULL)
							originRule->BuildAllUsedOperands(nkdAllUsedOperands);
					}
				}
				// Cas d'un bloc d'attributs
				else
				{
					originAttributeBlock = operand->GetOriginAttributeBlock();
					if (originAttributeBlock != NULL)
					{
						// Acces a la regle d'attribut ou de bloc
						originRule = originAttributeBlock->GetDerivationRule();
						if (originRule != NULL)
							originRule->BuildAllUsedOperands(nkdAllUsedOperands);
					}
				}
			}
		}
	}
}

void KWDerivationRule::BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
					      NumericKeyDictionary* nkdAllUsedAttributes) const
{
	int nOperand;
	KWDerivationRuleOperand* operand;
	KWAttribute* originAttribute;
	KWAttributeBlock* originAttributeBlock;
	KWDerivationRule* originRule;
	KWAttributeBlock* derivedAttributeBlock;
	Symbol sVarKey;
	int nVarKey;

	require(IsCompiled());
	require(derivedAttribute != NULL);
	require(nkdAllUsedAttributes != NULL);

	// Alimentation par parcours des operandes de la regle
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Recherche recursive si presence effective d'une regle
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule and
		    operand->GetDerivationRule() != NULL)
		{
			assert(not KWType::IsValueBlock(operand->GetDerivationRule()->GetType()));
			operand->GetDerivationRule()->BuildAllUsedAttributes(derivedAttribute, nkdAllUsedAttributes);
		}
		// Recherche recursive si presence effective d'un attribut calcule avec une regle
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
		{
			// Cas d'un attribut
			if (KWType::IsValue(operand->GetType()))
			{
				originAttribute = operand->GetOriginAttribute();
				if (originAttribute != NULL)
				{
					// Analyse de l'attribut si necessaire
					if (nkdAllUsedAttributes->Lookup(originAttribute) == NULL)
					{
						// Memorisation de l'attribut dans le dictionnaire
						nkdAllUsedAttributes->SetAt(originAttribute, originAttribute);

						// Acces a la regle d'attribut ou de bloc
						originRule = originAttribute->GetAnyDerivationRule();
						if (originRule != NULL)
							originRule->BuildAllUsedAttributes(originAttribute,
											   nkdAllUsedAttributes);
					}
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				assert(KWType::IsValueBlock(operand->GetType()));
				originAttributeBlock = operand->GetOriginAttributeBlock();
				if (originAttributeBlock != NULL)
				{
					// Bloc eventuel de l'attribut derive
					derivedAttributeBlock = derivedAttribute->GetAttributeBlock();

					// Cas de l'attribut derive dans un bloc
					if (derivedAttributeBlock != NULL)
					{
						// Recherche de l'attribut utilise dans le bloc si son VarKey coincide
						// avec celui de l'attribut derive
						assert(derivedAttributeBlock->GetVarKeyType() ==
						       originAttributeBlock->GetVarKeyType());
						originAttribute = NULL;
						if (derivedAttributeBlock->GetVarKeyType() == KWType::Symbol)
						{
							sVarKey =
							    derivedAttributeBlock->GetSymbolVarKey(derivedAttribute);
							originAttribute =
							    originAttributeBlock->LookupAttributeBySymbolVarKey(
								sVarKey);
						}
						else
						{
							nVarKey = derivedAttributeBlock->GetContinuousVarKey(
							    derivedAttribute);
							originAttribute =
							    originAttributeBlock->LookupAttributeByContinuousVarKey(
								nVarKey);
						}

						// Identification de l'attribut utilise dans le bloc en operande
						if (originAttribute != NULL and
						    nkdAllUsedAttributes->Lookup(originAttribute) == NULL)
						{
							// Memorisation de l'attribut dans le dictionnaire
							nkdAllUsedAttributes->SetAt(originAttribute, originAttribute);

							// Acces a la regle d'attribut ou de bloc
							if (originAttributeBlock->GetDerivationRule() != NULL)
								originAttributeBlock->GetDerivationRule()
								    ->BuildAllUsedAttributes(originAttribute,
											     nkdAllUsedAttributes);
						}
					}
					// Sinon, propagation a l'eventuelle regle du bloc avec l'attribut derive
					else
					{
						originRule = originAttributeBlock->GetDerivationRule();
						if (originRule != NULL)
							originRule->BuildAllUsedAttributes(derivedAttribute,
											   nkdAllUsedAttributes);
					}
				}
			}
		}
	}
}

boolean KWDerivationRule::IsSecondaryScopeOperand(int nOperandIndex) const
{
	require(0 <= nOperandIndex and nOperandIndex < GetOperandNumber());
	return GetMultipleScope() and nOperandIndex > 0;
}

int KWDerivationRule::GetOutputOperandNumber() const
{
	return 0;
}

boolean KWDerivationRule::GetVariableOutputOperandNumber() const
{
	return false;
}

KWDerivationRuleOperand* KWDerivationRule::GetOutputOperandAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetOutputOperandNumber());
	assert(false);
	return NULL;
}

boolean KWDerivationRule::CheckDefinition() const
{
	// On test d'abord les operandes, qui peuvent etre a l'origine d'erreur sur la regle elle-meme
	return CheckOperandsDefinition() and CheckRuleDefinition();
}

boolean KWDerivationRule::CheckRuleDefinition() const
{
	boolean bResult = true;
	boolean bIsGenericRule;
	ALString sTmp;

	// Nom de la regle
	if (not KWClass::CheckName(GetName(), KWClass::Rule, this))
	{
		AddError("Incorrect name");
		bResult = false;
	}

	// Libelle
	if (not KWClass::CheckLabel(GetLabel(), KWClass::Rule, this))
	{
		AddError("Incorrect label");
		bResult = false;
	}

	// Nom de classe si renseigne
	if (GetClassName() != "" and not KWClass::CheckName(GetName(), KWClass::Class, this))
	{
		AddError("Incorrect dictionary name");
		bResult = false;
	}

	// Type
	if (not KWType::Check(GetType()))
	{
		AddError("Incorrect type");
		bResult = false;
	}

	// Nom de la classe pour un type Object ou ObjectArray si renseigne
	if (KWType::IsGeneralRelation(GetType()) and GetObjectClassName() != "" and
	    not KWClass::CheckName(GetObjectClassName(), KWClass::Class, this))
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
		else if (not KWClass::CheckName(GetStructureName(), KWClass::Structure, this))
		{
			AddError("Incorrect structure name for Structure type");
			bResult = false;
		}
	}

	// Au moins un operande pour les regles a nombre variable d'operandes
	// uniquement pour la regle "generique"
	bIsGenericRule = LookupDerivationRule(GetName()) == this;
	if (bIsGenericRule)
	{
		if (GetVariableOperandNumber() and oaOperands.GetSize() == 0)
		{
			AddError("The definition of a registered derivation rule with a variable number of operands "
				 "must contain at least one operand");
			bResult = false;
		}
	}

	return bResult;
}

boolean KWDerivationRule::CheckOperandsDefinition() const
{
	boolean bResult = true;
	boolean bIsGenericRule;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	// Recherche s'il s'agit de la regle generique enregistree
	bIsGenericRule = LookupDerivationRule(GetName()) == this;

	// Verification des operandes
	for (i = 0; i < oaOperands.GetSize(); i++)
	{
		operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));
		if (not operand->CheckDefinition())
		{
			AddError(sTmp + "Incorrect operand " + IntToString(i + 1));
			bResult = false;
		}

		// Pour la regle generique, seul le dernier operande, en cas de nombre variables d'operandes,
		// a le droit d'etre de type Unknown
		if (bResult and bIsGenericRule)
		{
			if ((i < oaOperands.GetSize() - 1 or not GetVariableOperandNumber()) and
			    operand->GetType() == KWType::Unknown)
			{
				AddError("In the definition of a registered derivation rule with a variable number of "
					 "operands, first operands must have their type specified");
				bResult = false;
			}
		}
	}

	// Cas des regle avec scope multiple
	if (bResult and GetMultipleScope())
	{
		// Il doit y avoir au moins un operande
		if (bResult and GetOperandNumber() == 0)
		{
			bResult = false;
			AddError("At least one operand is mandatory");
		}

		// Verification du premier operande
		if (bResult)
			bResult = CheckFirstMultiScopeOperand();
	}
	return bResult;
}

boolean KWDerivationRule::CheckFamily(const KWDerivationRule* ruleFamily) const
{
	// On test d'abord les operandes, qui peuvent etre a l'origine d'erreur sur la regle elle-meme
	return CheckOperandsFamily(ruleFamily) and CheckRuleFamily(ruleFamily);
}

boolean KWDerivationRule::CheckRuleFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bResult = true;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Test de definition de la regle: arret si incorrect
	if (not CheckDefinition())
		return false;

	// Nom de la regle
	if (GetName() != ruleFamily->GetName())
	{
		AddError("Name of derivation rule inconsistent with that of the registered rule (" +
			 ruleFamily->GetName() + ")");
		bResult = false;
	}

	// Nom de classe si renseigne
	if (ruleFamily->GetClassName() != "" and GetClassName() != ruleFamily->GetClassName())
	{
		AddError("Dictionary " + GetClassName() + " inconsistent with that of the registered rule (" +
			 ruleFamily->GetClassName() + ")");
		bResult = false;
	}

	// Type
	if (GetType() != ruleFamily->GetType())
	{
		AddError("Type " + KWType::ToString(GetType()) + " inconsistent with that of the registered rule (" +
			 KWType::ToString(ruleFamily->GetType()) + ")");
		bResult = false;
	}

	// Nom de la classe pour un type Object ou ObjectArray si renseigne
	if (KWType::IsGeneralRelation(ruleFamily->GetType()) and ruleFamily->GetObjectClassName() != "" and
	    GetObjectClassName() != ruleFamily->GetObjectClassName())
	{
		AddError("Dictionary name for " + KWType::ToString(GetType()) + " type " + GetObjectClassName() +
			 " inconsistent with that of the registered rule (" + ruleFamily->GetObjectClassName() + ")");
		bResult = false;
	}

	// Nom de structure pour un type Structure
	if (ruleFamily->GetType() == KWType::Structure and ruleFamily->GetStructureName() != "" and
	    GetStructureName() != ruleFamily->GetStructureName())
	{
		AddError("Structure name for Structure type " + GetStructureName() +
			 " inconsistent with that of the registered rule (" + ruleFamily->GetStructureName() + ")");
		bResult = false;
	}

	// Verification du nombre d'operandes
	// (exactement, ou pas dans le cas des nombres variables d'operandes)
	if ((ruleFamily->GetVariableOperandNumber() and GetOperandNumber() < ruleFamily->GetOperandNumber() - 1) or
	    (not ruleFamily->GetVariableOperandNumber() and GetOperandNumber() != ruleFamily->GetOperandNumber()))
	{
		AddError(sTmp + "Number of operands (" + IntToString(GetOperandNumber()) +
			 ") inconsistent with that of the registered rule (" +
			 IntToString(ruleFamily->GetOperandNumber()) + ")");
		bResult = false;
	}

	return bResult;
}

boolean KWDerivationRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bResult = true;
	KWDerivationRuleOperand* operand;
	KWDerivationRuleOperand* familyVariableOperand;
	int i;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification des operandes
	for (i = 0; i < oaOperands.GetSize(); i++)
	{
		operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

		// Cas des derniers operandes pour un nombre variable d'operandes
		if (ruleFamily->GetVariableOperandNumber() and i >= ruleFamily->GetOperandNumber() - 1)
		{
			familyVariableOperand = ruleFamily->GetOperandAt(ruleFamily->GetOperandNumber() - 1);

			// Controle sauf dans le cas de type Unknown pour la famille
			// Dans ce cas, le controle est a effectuer dans la classe
			if (familyVariableOperand->GetType() != KWType::Unknown and
			    not operand->CheckFamily(familyVariableOperand))
			{
				AddError(sTmp + "Operand " + IntToString(i + 1) +
					 " inconsistent with that of the registered rule");
				bResult = false;
			}
		}
		// Cas des premiers operandes
		else
		{
			if (not operand->CheckFamily(ruleFamily->GetOperandAt(i)))
			{
				AddError(sTmp + "Operand " + IntToString(i + 1) +
					 " inconsistent with that of the registered rule");
				bResult = false;
			}
		}
	}

	return bResult;
}

boolean KWDerivationRule::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	// On test d'abord les operandes, qui peuvent etre a l'origine d'erreur sur la regle elle-meme
	return CheckOperandsCompleteness(kwcOwnerClass) and CheckRuleCompletness(kwcOwnerClass);
}

boolean KWDerivationRule::CheckRuleCompletness(const KWClass* kwcOwnerClass) const
{
	boolean bResult = true;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Test de definition de la regle: arret si incorrect
	if (not CheckDefinition())
		return false;

	// Nom de la classe pour un type Object
	if (KWType::IsGeneralRelation(GetType()))
	{
		// Classe renseignee
		if (GetObjectClassName() == "")
		{
			AddError("Missing dictionary name for type " + KWType::ToString(GetType()));
			bResult = false;
		}
		// Class existante dans le domaine
		else if (kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName()) == NULL)
		{
			AddError("Unknown dictionary name (" + GetObjectClassName() + ") for type " +
				 KWType::ToString(GetType()));
			bResult = false;
		}
	}

	// Nom de la classe
	if (GetClassName() == "")
	{
		AddError("Missing dictionary name");
		bResult = false;
	}
	// Coherence avec la classe de verification
	else if (GetClassName() != kwcOwnerClass->GetName())
	{
		AddError("Dictionary name (" + GetClassName() + ") inconsistent with dictionary " +
			 kwcOwnerClass->GetName());
		bResult = false;
	}

	/// Verification des cle pour les blocs de variables
	// Verification technique, utile pour la mise au pouint des regles de type bloc
	// Cette verification n'est effectuee qu'a la fin pour ne pas perturber si possible les
	// message d'erreur utilisateur
	if (bResult)
	{
		// Type de cle pour les variables
		if (KWType::IsValueBlock(GetType()))
		{
			if (GetVarKeyType() == KWType::None)
			{
				AddError("Var key type must be specified for block type");
				bResult = false;
			}
			else if (GetVarKeyType() != KWType::Symbol and GetVarKeyType() != KWType::Continuous)
			{
				AddError("Var key type must be categorical or integer for block type");
				bResult = false;
			}
		}
		else
		{
			if (GetVarKeyType() != KWType::None)
			{
				AddError("Var key type must not be specified for non block type");
				bResult = false;
			}
		}
	}
	return bResult;
}

boolean KWDerivationRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bResult = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;
	const KWClass* secondaryScopeClass;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Verification des operandes dans le cas standard
	if (not GetMultipleScope())
	{
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Verification de l'operande
			if (not operand->CheckCompleteness(kwcOwnerClass))
			{
				AddError(sTmp + "Incomplete operand " + IntToString(i + 1));
				bResult = false;
			}
		}
	}
	// Verification des operandes dans le cas a scope multiple
	else
	{
		scopeClass = kwcOwnerClass;
		secondaryScopeClass = NULL;
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Verification de l'operande
			if (not operand->CheckCompleteness(scopeClass))
			{
				AddError(sTmp + "Incomplete operand " + IntToString(i + 1));
				bResult = false;
			}

			// Cas du premier operande pour rechercher la classe de scope scondaire
			if (i == 0)
			{
				secondaryScopeClass = LookupSecondaryScopeClass(kwcOwnerClass);

				// Arret si on a pas trouve la classe secondaire
				if (secondaryScopeClass == NULL)
					break;
			}

			// Recherche de la classe de scope pour le prochain operande
			if (i < oaOperands.GetSize() - 1 and IsSecondaryScopeOperand(i + 1))
			{
				// Empilage du scope si on passe vers le scope secondaire
				if (scopeClass == kwcOwnerClass)
					PushScope(kwcOwnerClass, kwcOwnerClass, this);
				scopeClass = secondaryScopeClass;
			}
			else
				scopeClass = kwcOwnerClass;

			// Depilage du scope si necessaire, si on repasse vers le scope principal au prochain operande
			if (IsSecondaryScopeOperand(i) and
			    (i == oaOperands.GetSize() - 1 or not IsSecondaryScopeOperand(i + 1)))
				PopScope(kwcOwnerClass);
		}
	}
	return bResult;
}

boolean KWDerivationRule::CheckBlockAttributes(const KWClass* kwcOwnerClass,
					       const KWAttributeBlock* attributeBlock) const
{
	boolean bResult = true;
	int i;
	ALString sTmp;
	const KWClass* secondaryScopeClass;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute()->GetParentClass()->GetDomain() == kwcOwnerClass->GetDomain());
	require(attributeBlock->GetDerivationRule() == this);

	// Verification des operandes dans le cas standard
	if (not GetMultipleScope())
	{
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			// Verification de l'operande
			bResult = bResult and CheckBlockAttributesAt(kwcOwnerClass, attributeBlock, i);
		}
	}
	// Verification des operandes dans le cas a scope multiple
	else
	{
		scopeClass = kwcOwnerClass;
		secondaryScopeClass = NULL;
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			// Verification de l'operande
			bResult = bResult and CheckBlockAttributesAt(scopeClass, attributeBlock, i);

			// Cas du premier operande pour rechercher la classe de scope scondaire
			if (i == 0)
			{
				secondaryScopeClass = LookupSecondaryScopeClass(kwcOwnerClass);

				// Arret si on a pas trouve la classe secondaire
				if (secondaryScopeClass == NULL)
					break;
			}

			// Recherche de la classe de scope pour le prochain operande
			if (i < oaOperands.GetSize() - 1 and IsSecondaryScopeOperand(i + 1))
			{
				// Empilage du scope si on passe vers le scope secondaire
				if (scopeClass == kwcOwnerClass)
					PushScope(kwcOwnerClass, kwcOwnerClass, this);
				scopeClass = secondaryScopeClass;
			}
			else
				scopeClass = kwcOwnerClass;

			// Depilage du scope si necessaire, si on repasse vers le scope principal au prochain operande
			if (IsSecondaryScopeOperand(i) and
			    (i == oaOperands.GetSize() - 1 or not IsSecondaryScopeOperand(i + 1)))
				PopScope(kwcOwnerClass);
		}
	}
	return bResult;
}

boolean KWDerivationRule::CheckBlockAttributesAt(const KWClass* kwcOwnerClass, const KWAttributeBlock* attributeBlock,
						 int nOperandIndex) const
{
	boolean bResult = true;
	KWDerivationRuleOperand* operand;
	KWAttribute* checkedAttribute;
	KWAttribute* originAttribute;
	KWAttributeBlock* originAttributeBlock;
	NumericKeyDictionary nkdOriginAttributesByVarKeys;
	Symbol sVarKey;
	int nVarKey;
	ALString sExternalVarKey;
	ALString sScopeMessage;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute()->GetParentClass()->GetDomain() == kwcOwnerClass->GetDomain());
	require(0 <= nOperandIndex and nOperandIndex < GetOperandNumber());

	// Recherche de l'operande
	operand = GetOperandAt(nOperandIndex);

	// Verification de l'operande s'il est de type bloc
	if (KWType::IsValueBlock(operand->GetType()))
	{
		// On ne peut avoir de regle
		assert(operand->GetOrigin() != KWDerivationRuleOperand::OriginRule);

		// Recherche du bloc de l'operande
		originAttributeBlock = kwcOwnerClass->LookupAttributeBlock(operand->GetAttributeBlockName());
		assert(attributeBlock->GetVarKeyType() == originAttributeBlock->GetVarKeyType());
		if (originAttributeBlock != NULL)
		{
			// Indexation des attributs du bloc d'origine
			// Ce bloc n'etant pas encore compile, on ne peut pas utiliser ses methodes de recherche
			// d'attribut par VarKey Parcours des attributs du bloc a verifier
			originAttribute = originAttributeBlock->GetFirstAttribute();
			while (originAttribute != NULL)
			{
				// Recherche de l'attribut utilise dans le bloc si son VarKey coincide
				// avec celui de l'attribut derive
				if (originAttributeBlock->GetVarKeyType() == KWType::Symbol)
				{
					sVarKey = originAttributeBlock->GetSymbolVarKey(originAttribute);
					nkdOriginAttributesByVarKeys.SetAt(sVarKey.GetNumericKey(), originAttribute);
				}
				else
				{
					nVarKey = originAttributeBlock->GetContinuousVarKey(originAttribute);
					nkdOriginAttributesByVarKeys.SetAt(nVarKey, originAttribute);
				}

				// Arret si derniere variable du bloc trouvee
				if (originAttribute == originAttributeBlock->GetLastAttribute())
					break;
				// Sinon, attribut suivant
				else
					originAttribute->GetParentClass()->GetNextAttribute(originAttribute);
			}

			// Parcours des attributs du bloc a verifier
			checkedAttribute = attributeBlock->GetFirstAttribute();
			while (checkedAttribute != NULL)
			{
				// Recherche de l'attribut utilise dans le bloc si son VarKey coincide
				// avec celui de l'attribut derive
				originAttribute = NULL;
				if (attributeBlock->GetVarKeyType() == KWType::Symbol)
				{
					sVarKey = attributeBlock->GetSymbolVarKey(checkedAttribute);
					originAttribute = cast(
					    KWAttribute*, nkdOriginAttributesByVarKeys.Lookup(sVarKey.GetNumericKey()));
				}
				else
				{
					nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
					originAttribute =
					    cast(KWAttribute*, nkdOriginAttributesByVarKeys.Lookup(nVarKey));
				}

				// Erreur si attribut non trouve
				if (originAttribute == NULL)
				{
					// Preparation des informations sur la VarKey et la classe de scope
					sExternalVarKey = attributeBlock->GetStringVarKey(checkedAttribute);
					if (kwcOwnerClass != attributeBlock->GetParentClass())
						sScopeMessage = sTmp + " in dictionary " + kwcOwnerClass->GetName();

					// Messages d'erreur
					attributeBlock->AddError(sTmp + "Variable " + checkedAttribute->GetName() +
								 +" not found with its VarKey=" + sExternalVarKey +
								 " in input sparse variable block " +
								 originAttributeBlock->GetName() + " (operand " +
								 IntToString(nOperandIndex + 1) + " of rule " +
								 GetName() + ")" + sScopeMessage);
					bResult = false;
					break;
				}

				// Arret si derniere variable du bloc trouvee
				if (checkedAttribute == attributeBlock->GetLastAttribute())
					break;
				// Sinon, attribut suivant
				else
					checkedAttribute->GetParentClass()->GetNextAttribute(checkedAttribute);
			}
		}
	}
	return bResult;
}

boolean KWDerivationRule::CheckReferencedDerivationRuleAt(int nOperandIndex, const KWClass* kwcOwnerClass,
							  const ALString& sRuleName) const
{
	boolean bOk = true;
	KWDerivationRule* referencedRule;
	ALString sTmp;
	ALString sOperandLabel;

	require(kwcOwnerClass != NULL);
	require(0 <= nOperandIndex and nOperandIndex < GetOperandNumber());

	// Calcul d'un libelle pour l'operande
	if (nOperandIndex == 0)
		sOperandLabel = "first operand";
	else if (nOperandIndex == GetOperandNumber() - 1)
		sOperandLabel = "last operand";
	else
		sOperandLabel = sTmp + "operand " + IntToString(nOperandIndex + 1);

	// Test d'existence de la regle
	referencedRule = GetOperandAt(nOperandIndex)->GetReferencedDerivationRule(kwcOwnerClass);
	if (referencedRule == NULL)
	{
		bOk = false;
		AddError(sTmp + "Missing referenced rule for " + sOperandLabel);
	}
	// Test du type de la regle
	else if (sRuleName != "" and referencedRule->GetName() != sRuleName)
	{
		bOk = false;
		AddError(sTmp + "Expected " + sRuleName + " rule for " + sOperandLabel + " (not " +
			 referencedRule->GetName() + ")");
	}
	return bOk;
}

boolean KWDerivationRule::ContainsCycle(NumericKeyDictionary* nkdGreyAttributes,
					NumericKeyDictionary* nkdBlackAttributes) const
{
	boolean bContainsCycle = false;
	KWDerivationRuleOperand* operand;
	int nOperand;
	KWAttribute* calledAttribute;
	KWAttributeBlock* calledAttributeBlock;

	require(nkdGreyAttributes != NULL);
	require(nkdBlackAttributes != NULL);
	require(Check());
	require(IsCompiled());

	// Parcours des operandes de la regle
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Recherche recursive si presence effective d'une regle
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule and
		    operand->GetDerivationRule() != NULL)
			bContainsCycle =
			    operand->GetDerivationRule()->ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);
		// Recherche recursive si presence effective d'un attribut calcule avec une regle
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
		{
			// Cas d'un attribut
			if (KWType::IsValue(operand->GetType()))
			{
				calledAttribute = operand->GetOriginAttribute();
				calledAttributeBlock = calledAttribute->GetAttributeBlock();

				// Cas ou l'attribut appele n'est pas dans un bloc
				if (calledAttributeBlock == NULL)
				{
					// L'attribut est marque en Grey: presence d'une cycle
					if (nkdGreyAttributes->Lookup(calledAttribute) != NULL)
					{
						calledAttribute->GetParentClass()->AddError(
						    "Existing derivation cycle caused by the recursive use of "
						    "variable " +
						    calledAttribute->GetName() + " in " + GetName() + " rule");
						bContainsCycle = true;
					}
					// Attribut non marque: il faut continuer l'analyse
					else if (nkdBlackAttributes->Lookup(calledAttribute) == NULL)
						bContainsCycle = calledAttribute->ContainsCycle(nkdGreyAttributes,
												nkdBlackAttributes);
				}
				// Cas ou l'attribut appele est dans un bloc
				else
				{
					// Le bloc est marque en Grey: presence d'un cycle
					if (nkdGreyAttributes->Lookup(calledAttributeBlock) != NULL)
					{
						calledAttribute->GetParentClass()->AddError(
						    "Existing derivation cycle caused by the recursive use of "
						    "variable " +
						    calledAttribute->GetName() + " in " + GetName() + " rule");
						bContainsCycle = true;
					}
					// Bloc non marque: il faut continuer l'analyse
					else if (nkdBlackAttributes->Lookup(calledAttributeBlock) == NULL)
						bContainsCycle = calledAttributeBlock->ContainsCycle(
						    nkdGreyAttributes, nkdBlackAttributes);
				}
			}
			// Cas d'un bloc d'attribut
			else
			{
				calledAttributeBlock = operand->GetOriginAttributeBlock();
				if (calledAttributeBlock != NULL)
				{
					// Le bloc est marque en Grey: presence d'un cycle
					if (nkdGreyAttributes->Lookup(calledAttributeBlock) != NULL)
					{
						calledAttributeBlock->GetParentClass()->AddError(
						    "Existing derivation cycle caused by the recursive use of sparse "
						    "variable block " +
						    calledAttributeBlock->GetName() + " in " + GetName() + " rule");
						bContainsCycle = true;
					}
					// Bloc non marque: il faut continuer l'analyse
					else if (nkdBlackAttributes->Lookup(calledAttributeBlock) == NULL)
						bContainsCycle = calledAttributeBlock->ContainsCycle(
						    nkdGreyAttributes, nkdBlackAttributes);
				}
			}
		}

		// Arret si cycle detecte
		if (bContainsCycle)
			break;
	}
	return bContainsCycle;
}

void KWDerivationRule::CompleteTypeInfo(const KWClass* kwcOwnerClass)
{
	NumericKeyDictionary nkdCompletedAttributes;
	InternalCompleteTypeInfo(kwcOwnerClass, &nkdCompletedAttributes);
}

void KWDerivationRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						NumericKeyDictionary* nkdCompletedAttributes)
{
	KWDerivationRuleOperand* operand;
	int i;
	const KWClass* secondaryScopeClass;
	const KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Nom de la classe
	if (GetClassName() == "")
		SetClassName(kwcOwnerClass->GetName());
	kwcClass = kwcOwnerClass;
	nClassFreshness = kwcClass->GetFreshness();

	// Completion des operandes dans le cas standard
	if (not GetMultipleScope())
	{
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Completion de l'operande
			operand->InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);
		}
	}
	// Completion des operandes dans le cas a scope multiple
	else
	{
		scopeClass = kwcOwnerClass;
		secondaryScopeClass = NULL;
		for (i = 0; i < oaOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			// Completion de l'operande
			operand->InternalCompleteTypeInfo(scopeClass, nkdCompletedAttributes);

			// Cas du premier operande pour rechercher la classe de scope scondaire
			if (i == 0)
			{
				secondaryScopeClass = LookupSecondaryScopeClass(kwcOwnerClass);

				// Arret si on a pas trouve la classe secondaire
				if (secondaryScopeClass == NULL)
					break;
			}

			// Recherche de la classe de scope pour le prochain operande
			if (i < oaOperands.GetSize() - 1 and IsSecondaryScopeOperand(i + 1))
			{
				// Empilage du scope si on passe vers le scope secondaire
				if (scopeClass == kwcOwnerClass)
					PushScope(kwcOwnerClass, kwcOwnerClass, this);
				scopeClass = secondaryScopeClass;
			}
			else
				scopeClass = kwcOwnerClass;

			// Depilage du scope si necessaire, si on repasse vers le scope principal au prochain operande
			if (IsSecondaryScopeOperand(i) and
			    (i == oaOperands.GetSize() - 1 or not IsSecondaryScopeOperand(i + 1)))
				PopScope(kwcOwnerClass);
		}
	}
}

void KWDerivationRule::Compile(KWClass* kwcOwnerClass)
{
	KWDerivationRuleOperand* operand;
	int i;
	KWClass* secondaryScopeClass;
	KWClass* scopeClass;

	require(kwcOwnerClass != NULL);
	require(CheckCompleteness(kwcOwnerClass));
	require(kwcOwnerClass->IsIndexed());
	require(kwcOwnerClass->GetName() == GetClassName());

	// Compilation uniquement si necessaire
	if (kwcOwnerClass != kwcClass or not IsCompiled())
	{
		// Enregistrement de la fraicheur de compilation
		nCompileFreshness = nFreshness;

		// Gestion de la classe
		kwcClass = kwcOwnerClass;
		nClassFreshness = kwcClass->GetFreshness();

		// Nettoyage prealable des operandes secondaires ayant acces aux scope principale de la regle
		if (oaMainScopeSecondaryOperands != NULL)
			oaMainScopeSecondaryOperands->SetSize(0);
		else
			oaMainScopeSecondaryOperands = new ObjectArray;

		// Compilation des operandes dans le cas standard
		if (not GetMultipleScope())
		{
			for (i = 0; i < oaOperands.GetSize(); i++)
			{
				operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

				// Compilation de l'operande
				operand->Compile(kwcOwnerClass);
			}
		}
		// Compilation des operandes dans le cas a scope multiple
		else
		{
			scopeClass = kwcOwnerClass;
			secondaryScopeClass = NULL;
			for (i = 0; i < oaOperands.GetSize(); i++)
			{
				operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

				// Compilation de l'operande
				operand->Compile(scopeClass);

				// Cas du premier operande pour rechercher la classe de scope scondaire
				if (i == 0)
				{
					secondaryScopeClass = LookupSecondaryScopeClass(kwcOwnerClass);

					// Arret si on a pas trouve la classe secondaire
					if (secondaryScopeClass == NULL)
						break;
				}

				// Recherche de la classe de scope pour le prochain operande
				if (i < oaOperands.GetSize() - 1 and IsSecondaryScopeOperand(i + 1))
				{
					// Empilage du scope si on passe vers le scope secondaire
					if (scopeClass == kwcOwnerClass)
						PushScope(kwcOwnerClass, kwcOwnerClass, this);
					scopeClass = secondaryScopeClass;
				}
				else
					scopeClass = kwcOwnerClass;

				// Depilage du scope si necessaire, si on repasse vers le scope principal au prochain
				// operande
				if (IsSecondaryScopeOperand(i) and
				    (i == oaOperands.GetSize() - 1 or not IsSecondaryScopeOperand(i + 1)))
					PopScope(kwcOwnerClass);
			}
		}

		// Destruction du tableau operandes secondaires ayant acces aux scope principale de la regle
		// dans le cas ou le tableau est vide
		assert(oaMainScopeSecondaryOperands != NULL);
		if (oaMainScopeSecondaryOperands->GetSize() == 0)
		{
			delete oaMainScopeSecondaryOperands;
			oaMainScopeSecondaryOperands = NULL;
		}
	}
	ensure(IsCompiled());
}

Continuous KWDerivationRule::ComputeContinuousResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est Continuous
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return 0;
}

Symbol KWDerivationRule::ComputeSymbolResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est Symbol
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return Symbol();
}

Date KWDerivationRule::ComputeDateResult(const KWObject* kwoObject) const
{
	Date dtValue;

	// Doit etre reimplemente si le type est Date
	kwoObject = NULL; // Pour eviter le warning
	dtValue.Reset();
	assert(false);
	return dtValue;
}

Time KWDerivationRule::ComputeTimeResult(const KWObject* kwoObject) const
{
	Time tmValue;

	// Doit etre reimplemente si le type est Time
	kwoObject = NULL; // Pour eviter le warning
	tmValue.Reset();
	assert(false);
	return tmValue;
}

Timestamp KWDerivationRule::ComputeTimestampResult(const KWObject* kwoObject) const
{
	Timestamp tsValue;

	// Doit etre reimplemente si le type est Timestamp
	kwoObject = NULL; // Pour eviter le warning
	tsValue.Reset();
	assert(false);
	return tsValue;
}

TimestampTZ KWDerivationRule::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	TimestampTZ tstzValue;

	// Doit etre reimplemente si le type est Timestamp
	kwoObject = NULL; // Pour eviter le warning
	tstzValue.Reset();
	assert(false);
	return tstzValue;
}

Symbol KWDerivationRule::ComputeTextResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est Text
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return Symbol();
}

SymbolVector* KWDerivationRule::ComputeTextListResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est TextList
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return NULL;
}

KWObject* KWDerivationRule::ComputeObjectResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est Object
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return NULL;
}

ObjectArray* KWDerivationRule::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est ObjectArray
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return NULL;
}

Object* KWDerivationRule::ComputeStructureResult(const KWObject* kwoObject) const
{
	// Doit etre reimplemente si le type est Structure
	kwoObject = NULL; // Pour eviter le warning
	assert(false);
	return NULL;
}

KWContinuousValueBlock*
KWDerivationRule::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						    const KWIndexedKeyBlock* indexedKeyBlock) const
{
	// Doit etre reimplemente si le type est ContinuousValueBlock
	kwoObject = NULL; // Pour eviter le warning
	indexedKeyBlock = NULL;
	assert(false);
	return NULL;
}

KWSymbolValueBlock* KWDerivationRule::ComputeSymbolValueBlockResult(const KWObject* kwoObject,
								    const KWIndexedKeyBlock* indexedKeyBlock) const
{
	// Doit etre reimplemente si le type est SymbolValueBlock
	kwoObject = NULL; // Pour eviter le warning
	indexedKeyBlock = NULL;
	assert(false);
	return NULL;
}

KWObjectArrayValueBlock*
KWDerivationRule::ComputeObjectArrayValueBlockResult(const KWObject* kwoObject,
						     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	// Doit etre reimplemente si le type est ObjectArrayValueBlock
	kwoObject = NULL; // Pour eviter le warning
	indexedKeyBlock = NULL;
	assert(false);
	return NULL;
}

Continuous KWDerivationRule::GetValueBlockContinuousDefaultValue() const
{
	// Doit etre reimplemente si le type est ContinuousValueBlock
	assert(false);
	return KWContinuousValueBlock::GetDefaultDefaultValue();
}

Symbol& KWDerivationRule::GetValueBlockSymbolDefaultValue() const
{
	// Doit etre reimplemente si le type est SymbolValueBlock
	assert(false);
	return KWSymbolValueBlock::GetDefaultDefaultValue();
}

KWDerivationRule* KWDerivationRule::Clone() const
{
	KWDerivationRule* kwdrClone;
	kwdrClone = Create();
	kwdrClone->CopyFrom(this);
	return kwdrClone;
}

KWDerivationRule* KWDerivationRule::Create() const
{
	assert(false);
	return NULL;
}

void KWDerivationRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	KWDerivationRuleOperand* operand;
	int i;

	require(kwdrSource != NULL);

	// Recopie des attributs de base
	usName = kwdrSource->usName;
	usLabel = kwdrSource->usLabel;
	usClassName = kwdrSource->usClassName;
	cType = kwdrSource->cType;
	usSupplementTypeName = kwdrSource->usSupplementTypeName;
	bVariableOperandNumber = kwdrSource->bVariableOperandNumber;
	bMultipleScope = kwdrSource->bMultipleScope;
	if (oaMainScopeSecondaryOperands != NULL)
	{
		delete oaMainScopeSecondaryOperands;
		oaMainScopeSecondaryOperands = NULL;
	}

	// Pas de memorisation des resultats de compilation
	// La nouvelle version est a recompiler
	kwcClass = NULL;
	nFreshness = kwdrSource->nFreshness;
	nClassFreshness = 0;
	nCompileFreshness = 0;

	// Duplication des operandes
	DeleteAllOperands();
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		AddOperand(operand->Clone());
	}
}

boolean KWDerivationRule::Check() const
{
	return CheckDefinition();
}

int KWDerivationRule::FullCompare(const KWDerivationRule* rule) const
{
	const KWClass* secondaryScopeClass;
	const KWClass* scopeClass;
	int nOperand;
	KWDerivationRuleOperand* operand1;
	KWDerivationRuleOperand* operand2;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	KWAttributeBlock* attributeBlock1;
	KWAttributeBlock* attributeBlock2;
	KWDerivationRule* operandRule1;
	KWDerivationRule* operandRule2;
	int nDiff;

	require(rule != NULL);
	require(rule->kwcClass != NULL);
	require(rule->CheckCompleteness(rule->kwcClass));
	require(kwcClass != NULL);
	require(CheckCompleteness(kwcClass));

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison sur le nombre d'operandes
	if (nDiff == 0)
		nDiff = GetOperandNumber() - rule->GetOperandNumber();

	// En cas d'egalite, comparaison sur chaque operande
	if (nDiff == 0)
	{
		// Classe du scope courant
		scopeClass = kwcClass;
		secondaryScopeClass = NULL;

		// Comparaison des noms des premiers attributs
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			operand1 = GetOperandAt(nOperand);
			operand2 = rule->GetOperandAt(nOperand);

			// Comparaison sur le type de l'operande
			if (nDiff == 0)
				nDiff = operand1->GetType() - operand2->GetType();

			// Comparaison sur le sous-type si necessaire
			if (nDiff == 0)
			{
				if (KWType::IsGeneralRelation(operand1->GetType()))
					nDiff = operand1->GetObjectClassName().Compare(operand2->GetObjectClassName());
				else if (operand1->GetType() == KWType::Structure)
					nDiff = operand1->GetStructureName().Compare(operand2->GetStructureName());
			}

			// Comparaison sur l'origine des operande si necessaire
			if (nDiff == 0)
			{
				// Comparaison sur l'origine de l'operande dans le cas d'une constante dans les deux cas
				if (operand1->GetOrigin() == KWDerivationRuleOperand::OriginConstant and
				    operand2->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
				{
					assert(KWType::IsSimple(operand1->GetType()));
					if (operand1->GetType() == KWType::Continuous)
						nDiff = KWContinuous::Compare(operand1->GetContinuousConstant(),
									      operand2->GetContinuousConstant());
					else
						nDiff = operand1->GetSymbolConstant().CompareValue(
						    operand2->GetSymbolConstant());
				}
				// Comparaison si un des operandes a pour origine une constante, et pas l'autre
				else if (operand1->GetOrigin() == KWDerivationRuleOperand::OriginConstant or
					 operand2->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
				{
					assert(operand1->GetOrigin() != operand2->GetOrigin());
					nDiff = operand1->GetOrigin() - operand2->GetOrigin();
				}
				//////////////////////////////////////////////////////////////////
				// Comparaison dans le cas d'attributs, calcules ou non
				else if (KWType::IsValue(operand1->GetType()))
				{
					// Acces aux regles de chaque sous-regle, si necessaire en passant par
					// l'attribut utilise
					attribute1 = NULL;
					if (operand1->GetOrigin() == KWDerivationRuleOperand::OriginRule)
						operandRule1 = operand1->GetDerivationRule();
					else
					{
						attribute1 = operand1->GetOriginAttribute();
						operandRule1 = attribute1->GetDerivationRule();
					}
					attribute2 = NULL;
					if (operand2->GetOrigin() == KWDerivationRuleOperand::OriginRule)
						operandRule2 = operand2->GetDerivationRule();
					else
					{
						attribute2 = operand2->GetOriginAttribute();
						operandRule2 = attribute2->GetDerivationRule();
					}

					// Cas ou les deux regles sont nulles
					if (operandRule1 == NULL and operandRule2 == NULL)
					{
						assert(attribute1 != NULL and attribute2 != NULL);

						// Acces aux bloc des attributs
						attributeBlock1 = attribute1->GetAttributeBlock();
						attributeBlock2 = attribute2->GetAttributeBlock();

						// Cas ou les deux attributs ne sont pas dans un bloc
						if (attributeBlock1 == NULL and attributeBlock2 == NULL)
							// On compare sur le nom des attributs
							nDiff = attribute1->GetName().Compare(attribute2->GetName());
						// Cas ou un des attribut est dans un bloc
						else if (attributeBlock1 == NULL)
							nDiff = -1;
						else if (attributeBlock2 == NULL)
							nDiff = 1;
						// Cas ou les deux attributs sont dans un bloc
						else
						{
							// On compare le type des VarKey des blocs
							nDiff = attributeBlock1->GetVarKeyType() -
								attributeBlock2->GetVarKeyType();

							// On continue la comparaison si on a le meme type de cle
							if (nDiff == 0)
							{
								// Comparaison selon le type de VarKey
								if (attributeBlock1->GetVarKeyType() ==
								    KWType::Continuous)
									nDiff = attributeBlock1->GetContinuousVarKey(
										    attribute1) -
										attributeBlock2->GetContinuousVarKey(
										    attribute2);
								else
									nDiff =
									    attributeBlock1->GetSymbolVarKey(attribute1)
										.CompareValue(
										    attributeBlock2->GetSymbolVarKey(
											attribute2));
							}

							// Comparaison supplementaire si necessaires, selon que les bloc
							// soient calcules ou non
							if (nDiff == 0)
							{
								// Cas ou les deux blocs ne sont pas calcules
								if (attributeBlock1->GetDerivationRule() == NULL and
								    attributeBlock2->GetDerivationRule() == NULL)
									// On compare sur le nom des blocs
									nDiff = attributeBlock1->GetName().Compare(
									    attributeBlock2->GetName());
								// Cas ou un des bloc est calcule
								else if (attributeBlock1->GetDerivationRule() == NULL)
									nDiff = -1;
								else if (attributeBlock2->GetDerivationRule() == NULL)
									nDiff = 1;
								// Cas ou les deux bloc sont calcules
								else
									nDiff = attributeBlock1->GetDerivationRule()
										    ->FullCompare(
											attributeBlock2
											    ->GetDerivationRule());
							}
						}
					}
					// Cas ou une des regles est nulle
					else if (operandRule1 == NULL)
						nDiff = -1;
					else if (operandRule2 == NULL)
						nDiff = 1;
					// Si les deux regles sont non nulle, on propage la comparaison
					else
						nDiff = operandRule1->FullCompare(operandRule2);
				}
				//////////////////////////////////////////////////////////////////
				// Comparaison dans le cas de blocs d'attributs, calcules ou non
				else
				{
					assert(KWType::IsValueBlock(operand1->GetType()));

					// Acces aux regles de chaque sous-regle, si necessaire en passant par
					// l'attribut utilise
					attributeBlock1 = NULL;
					if (operand1->GetOrigin() == KWDerivationRuleOperand::OriginRule)
						operandRule1 = operand1->GetDerivationRule();
					else
					{
						attributeBlock1 = operand1->GetOriginAttributeBlock();
						operandRule1 = attributeBlock1->GetDerivationRule();
					}
					attributeBlock2 = NULL;
					if (operand2->GetOrigin() == KWDerivationRuleOperand::OriginRule)
						operandRule2 = operand2->GetDerivationRule();
					else
					{
						attributeBlock2 = operand2->GetOriginAttributeBlock();
						operandRule2 = attributeBlock2->GetDerivationRule();
					}

					// Cas ou les deux regles sont nulles
					if (operandRule1 == NULL and operandRule2 == NULL)
					{
						assert(attributeBlock1 != NULL and attributeBlock2 != NULL);

						// On compare sur le nom des blocs
						nDiff = attributeBlock1->GetName().Compare(attributeBlock2->GetName());
					}
					// Cas ou une des regles est nulle
					else if (operandRule1 == NULL)
						nDiff = -1;
					else if (operandRule2 == NULL)
						nDiff = 1;
					// Si les deux regles sont non nulles, on propage la comparaison
					// On ne compare que la regle produisant les blocs, pas les attribut de chaque
					// blocs (pourtant disponibles dans le cas d'un bloc d'origine Attribute, avec
					// une regle)
					else
						nDiff = operandRule1->FullCompare(operandRule2);
				}
			}

			// Gestion du cas avec scope multiple
			if (GetMultipleScope())
			{
				// Cas du premier operande pour rechercher la classe de scope scondaire
				if (nOperand == 0)
				{
					secondaryScopeClass = LookupSecondaryScopeClass(kwcClass);

					// Arret si on a pas trouve la classe secondaire
					if (secondaryScopeClass == NULL)
						break;
				}

				// Recherche de la classe de scope pour le prochain operande
				if (nOperand < oaOperands.GetSize() - 1 and IsSecondaryScopeOperand(nOperand + 1))
				{
					// Empilage du scope si on passe vers le scope secondaire
					if (scopeClass == kwcClass)
						PushScope(kwcClass, kwcClass, this);
					scopeClass = secondaryScopeClass;
				}
				else
					scopeClass = kwcClass;

				// Depilage du scope si necessaire, si on repasse vers le scope principal au prochain
				// operande
				if (IsSecondaryScopeOperand(nOperand) and
				    (nOperand == oaOperands.GetSize() - 1 or not IsSecondaryScopeOperand(nOperand + 1)))
					PopScope(kwcClass);
			}
		}
	}
	return nDiff;
}

longint KWDerivationRule::GetUsedMemory() const
{
	longint lUsedMemory;
	longint lOperandUsedMemory;
	int i;
	KWDerivationRuleOperand* operand;

	// Prise en compte de la regle elle-meme
	// On ne prend pas en compte la memoire des UniqueString, car il elle est deja comptee par ailleurs
	lUsedMemory = sizeof(KWDerivationRule);

	// Prise en compte des operandes
	lUsedMemory += oaOperands.GetUsedMemory();
	for (i = 0; i < oaOperands.GetSize(); i++)
	{
		operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));
		lOperandUsedMemory = operand->GetUsedMemory();
		lUsedMemory += lOperandUsedMemory;
	}
	return lUsedMemory;
}

longint KWDerivationRule::ComputeHashValue() const
{
	longint lHash;
	int nOperand;
	KWDerivationRuleOperand* operand;

	// Attributs de base de la regle de derivation
	lHash = HashValue(GetName());
	lHash = LongintUpdateHashValue(lHash, HashValue(KWType::ToString(cType)));
	lHash = LongintUpdateHashValue(lHash, HashValue(GetSupplementTypeName()));

	// Prise en compte des operandes
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);
		lHash = LongintUpdateHashValue(lHash, operand->ComputeHashValue());
	}
	return lHash;
}

void KWDerivationRule::Write(ostream& ost) const
{
	KWDerivationRuleOperand* operand;
	int i;

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Libelle
	if (GetLabel() != "")
		ost << "  //" << GetLabel();
	ost << "\n";

	// Operandes
	for (i = 0; i < GetOperandNumber(); i++)
	{
		operand = GetOperandAt(i);
		ost << "\t";
		operand->Write(ost);
		ost << "\n";
	}

	// Code retour
	ost << "\t-> ";
	ost << KWType::ToString(GetType());
	if (KWType::IsGeneralRelation(GetType()))
	{
		if (GetObjectClassName() == "")
			ost << "()";
		else
			ost << "(" << KWClass::GetExternalName(GetObjectClassName()) << ")";
	}
	else if (GetType() == KWType::Structure)
	{
		if (GetStructureName() == "")
			ost << "()";
		else
			ost << "(" << KWClass::GetExternalName(GetStructureName()) << ")";
	}
	ost << "\n";
}

void KWDerivationRule::WriteUsedRule(ostream& ost) const
{
	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes entre parentheses
	ost << "(";
	WriteUsedRuleOperands(ost);
	ost << ")";
}

void KWDerivationRule::WriteUsedRuleOperands(ostream& ost) const
{
	KWDerivationRuleOperand* operand;
	int i;

	// Operandes
	for (i = 0; i < GetOperandNumber(); i++)
	{
		operand = GetOperandAt(i);
		if (i > 0)
			ost << ", ";
		operand->WriteUsedOperand(ost);
	}
}

void KWDerivationRule::PushScope(const KWClass* kwcOwnerClass, const KWClass* kwcScopeClass,
				 const KWDerivationRule* rule) const
{
	KWClassDomain* ownerDomain;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(kwcScopeClass != NULL);
	require(rule != NULL);

	// Empilement du scope
	ownerDomain = kwcOwnerClass->GetDomain();
	ownerDomain->GetScopeClasses()->Add(cast(Object*, kwcScopeClass));
	ownerDomain->GetScopeRules()->Add(cast(Object*, rule));
}

const KWClass* KWDerivationRule::PopScope(const KWClass* kwcOwnerClass) const
{
	KWClassDomain* ownerDomain;
	const KWClass* currentClass;
	int nScopeDepth;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(GetScopeDepth(kwcOwnerClass) > 0);

	// Depilement du scope
	ownerDomain = kwcOwnerClass->GetDomain();
	nScopeDepth = ownerDomain->GetScopeClasses()->GetSize();
	currentClass = cast(const KWClass*, ownerDomain->GetScopeClasses()->GetAt(nScopeDepth - 1));
	ownerDomain->GetScopeClasses()->SetSize(nScopeDepth - 1);
	ownerDomain->GetScopeRules()->SetSize(nScopeDepth - 1);
	return currentClass;
}

int KWDerivationRule::GetScopeDepth(const KWClass* kwcOwnerClass) const
{
	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	assert(kwcOwnerClass->GetDomain()->GetScopeClasses()->GetSize() ==
	       kwcOwnerClass->GetDomain()->GetScopeRules()->GetSize());
	return kwcOwnerClass->GetDomain()->GetScopeClasses()->GetSize();
}

KWClass* KWDerivationRule::GetClassAtScope(const KWClass* kwcOwnerClass, int nIndex) const
{
	KWClassDomain* ownerDomain;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(1 <= nIndex and nIndex <= GetScopeDepth(kwcOwnerClass));

	ownerDomain = kwcOwnerClass->GetDomain();
	return cast(KWClass*,
		    ownerDomain->GetScopeClasses()->GetAt(ownerDomain->GetScopeClasses()->GetSize() - nIndex));
}

KWDerivationRule* KWDerivationRule::GetRuleAtScope(const KWClass* kwcOwnerClass, int nIndex) const
{
	KWClassDomain* ownerDomain;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(1 <= nIndex and nIndex <= GetScopeDepth(kwcOwnerClass));

	ownerDomain = kwcOwnerClass->GetDomain();
	return cast(KWDerivationRule*,
		    ownerDomain->GetScopeRules()->GetAt(ownerDomain->GetScopeRules()->GetSize() - nIndex));
}

void KWDerivationRule::WriteScope(const KWClass* kwcOwnerClass, ostream& ost) const
{
	int nIndex;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	ost << "Scope ";
	for (nIndex = 0; nIndex < GetScopeDepth(kwcOwnerClass); nIndex++)
	{
		ost << " (" << GetClassAtScope(kwcOwnerClass, nIndex)->GetName() << ", "
		    << GetRuleAtScope(kwcOwnerClass, nIndex)->GetName() << ")";
	}
	ost << "\n";
}

boolean KWDerivationRule::CheckFirstMultiScopeOperand() const
{
	boolean bResult = true;

	// Le premier operande doit etre de type Relation
	if (bResult and not KWType::IsGeneralRelation(GetFirstOperand()->GetType()))
	{
		bResult = false;
		AddError("First operand must be of type Entity or Table");
	}
	return bResult;
}

KWClass* KWDerivationRule::LookupSecondaryScopeClass(const KWClass* kwcOwnerClass) const
{
	KWClass* secondaryScopeClass;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Recherche seulement si necessaire
	secondaryScopeClass = NULL;
	if (GetMultipleScope() and GetOperandNumber() > 0)
	{
		// Recherche si le premier operande est du bon type
		if (KWType::IsGeneralRelation(GetFirstOperand()->GetType()) and
		    GetFirstOperand()->GetObjectClassName() != "")
			secondaryScopeClass =
			    kwcOwnerClass->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
	}
	return secondaryScopeClass;
}

void KWDerivationRule::WriteObject(const KWObject* kwoObject, ostream& ost) const
{
	KWObjectKey key;

	// Affichage du nom de la regle, et des caracteristique de l'objet: Class, identifiant, et adresse memoire
	key.InitializeFromObject(kwoObject);
	ost << GetName() << "\t" << kwoObject->GetClass()->GetName() << "\t" << key.GetObjectLabel() << "\t"
	    << kwoObject << "\n";
}

void KWDerivationRule::WriteUsedObject(const KWObject* kwoSubObject, ostream& ost) const
{
	KWObjectKey key;

	// Affichage du sous-objet: Class, identifiant, et adresse memoire
	if (kwoSubObject != NULL)
	{
		key.InitializeFromObject(kwoSubObject);
		ost << "\t" << kwoSubObject->GetClass()->GetName() << "\t" << key.GetObjectLabel() << "\t"
		    << kwoSubObject << "\n";
	}
	else
		ost << "\t\t\t" << kwoSubObject << "\n";
}

void KWDerivationRule::WriteUsedObjectArray(const ObjectArray* oaSubObjectArray, ostream& ost) const
{
	const KWObject* kwoSubObject;
	int i;
	KWObjectKey key;

	// Affichage des sous-objets du containers
	ost << "\tTable\t" << oaSubObjectArray << endl;
	if (oaSubObjectArray != NULL)
	{
		for (i = 0; i < oaSubObjectArray->GetSize(); i++)
		{
			kwoSubObject = cast(const KWObject*, oaSubObjectArray->GetAt(i));
			ost << "\t" << i + 1;
			if (kwoSubObject != NULL)
				WriteUsedObject(kwoSubObject, ost);
			else
				ost << "\n";
		}
	}
}

void KWDerivationRule::WriteToString(ALString& sOutputString) const
{
	stringstream sstream;

	require(sOutputString.GetLength() == 0);
	Write(sstream);
	sOutputString = sstream.str().c_str();
}

void KWDerivationRule::WriteUsedRuleToString(ALString& sOutputString) const
{
	stringstream sstream;

	require(sOutputString.GetLength() == 0);
	WriteUsedRule(sstream);
	sOutputString = sstream.str().c_str();
}

const ALString KWDerivationRule::GetClassLabel() const
{
	return "Derivation rule";
}

const ALString KWDerivationRule::GetObjectLabel() const
{
	return GetName();
}

void KWDerivationRule::Test()
{
	KWDRCopySymbol refCopySymbolRule;
	KWDerivationRule* refRule;
	KWDerivationRule* rule;
	KWClass* testClass;
	KWObject* kwoObject;
	ObjectArray oaAvailableRules;
	KWDerivationRule* availableRule;
	KWAttribute* attribute;
	int i;

	// Recherche d'un Clone de la regle
	rule = CloneDerivationRule(refCopySymbolRule.GetName());

	// Sortie immediate si la regle n'existe pas
	if (rule == NULL)
	{
		Global::AddError("Derivation rule", "", "Missing rule '" + refCopySymbolRule.GetName() + "'");
		return;
	}

	// Affichage et tests
	cout << *rule << endl;
	rule->WriteUsedRule(cout);
	cout << endl;
	if (rule->CheckDefinition())
	{
		// Creation d'une classe
		testClass = KWClass::CreateClass("TestClass", 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, NULL);
		testClass->IndexClass();
		KWClassDomain::GetCurrentDomain()->InsertClass(testClass);
		KWClassDomain::GetCurrentDomain()->Compile();

		// Completion de la regle pour la classe
		rule->SetClassName(testClass->GetName());
		rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		rule->GetFirstOperand()->SetAttributeName(testClass->GetHeadAttribute()->GetName());

		// Verification de la regle
		refRule = LookupDerivationRule(refCopySymbolRule.GetName());
		rule->CheckFamily(refRule);
		rule->CheckCompleteness(testClass);

		// Compilation
		rule->Compile(testClass);
		cout << "\nInstanciation of rule:\n";
		cout << *rule << endl;
		rule->WriteUsedRule(cout);
		cout << endl;

		// Creation d'une instance
		cout << "\nCreate instance\n";
		kwoObject = KWObject::CreateObject(testClass, 1);
		cout << *kwoObject << endl;

		// Recherche des regles de derivation applicables
		cout << "\nSearch available derivation rules\n";
		attribute = testClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Creation si possible d'une regle
			if (attribute->GetType() == KWType::Symbol)
			{
				availableRule = refRule->Clone();
				availableRule->SetClassName(testClass->GetName());
				availableRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				availableRule->GetFirstOperand()->SetAttributeName(attribute->GetName());
				oaAvailableRules.Add(availableRule);
			}

			// Attribut suivant
			testClass->GetNextAttribute(attribute);
		}
		cout << *refRule << endl;
		for (i = 0; i < oaAvailableRules.GetSize(); i++)
		{
			availableRule = cast(KWDerivationRule*, oaAvailableRules.GetAt(i));
			cout << "\t";
			availableRule->WriteUsedRule(cout);
			availableRule->Compile(testClass);
			cout << "  -> " << availableRule->ComputeSymbolResult(kwoObject) << endl;
		}
		delete kwoObject;

		// Insertion dans la classe
		cout << "\nInsertion in dictionary\n";
		for (i = 0; i < oaAvailableRules.GetSize(); i++)
		{
			availableRule = cast(KWDerivationRule*, oaAvailableRules.GetAt(i));
			attribute = new KWAttribute;
			attribute->SetType(availableRule->GetType());
			attribute->SetDerivationRule(availableRule);
			attribute->SetName(testClass->BuildAttributeName(availableRule->GetName()));
			testClass->InsertAttribute(attribute);
		}
		testClass->Compile();
		cout << *testClass << endl;
		kwoObject = KWObject::CreateObject(testClass, 2);
		cout << *kwoObject << endl;
		delete kwoObject;

		// Renommage de la classe
		cout << "\nRename:\n";
		rule->RenameClass(testClass, "NewTestClass");
		cout << *rule << endl;

		// Destruction de toutes les classes enregistrees
		KWClassDomain::DeleteAllDomains();
	}

	// Liberation
	delete rule;
}

void KWDerivationRule::RegisterDerivationRule(KWDerivationRule* kwdrRule)
{
	require(kwdrRule != NULL);
	require(kwdrRule->GetName() != "");
	require(KWClass::CheckName(kwdrRule->GetName(), KWClass::Rule, kwdrRule));
	require(odDerivationRules == NULL or odDerivationRules->Lookup(kwdrRule->GetName()) == NULL);
	require(kwdrRule->CheckDefinition());

	// Creation si necessaire du dictionnaire de regles
	if (odDerivationRules == NULL)
		odDerivationRules = new ObjectDictionary;

	// Memorisation de la regle
	odDerivationRules->SetAt(kwdrRule->GetName(), kwdrRule);
}

KWDerivationRule* KWDerivationRule::LookupDerivationRule(const ALString& sName)
{
	require(sName != "");

	// Creation si necessaire du dictionnaire de regles
	if (odDerivationRules == NULL)
		odDerivationRules = new ObjectDictionary;

	return cast(KWDerivationRule*, odDerivationRules->Lookup(sName));
}

KWDerivationRule* KWDerivationRule::CloneDerivationRule(const ALString& sName)
{
	KWDerivationRule* referenceRule;
	require(sName != "");

	// Creation si necessaire du dictionnaire de regles
	if (odDerivationRules == NULL)
		odDerivationRules = new ObjectDictionary;

	// Recherche d'une regle de meme nom
	referenceRule = cast(KWDerivationRule*, odDerivationRules->Lookup(sName));

	// Retour de son Clone si possible
	if (referenceRule == NULL)
		return NULL;
	else
		return referenceRule->Clone();
}

void KWDerivationRule::ExportAllDerivationRules(ObjectArray* oaRules)
{
	require(oaRules != NULL);

	// Creation si necessaire du dictionnaire de regles
	if (odDerivationRules == NULL)
		odDerivationRules = new ObjectDictionary;

	odDerivationRules->ExportObjectArray(oaRules);
}

void KWDerivationRule::DeleteAllDerivationRules()
{
	if (odDerivationRules != NULL)
	{
		odDerivationRules->DeleteAll();
		delete odDerivationRules;
		odDerivationRules = NULL;
	}
	ensure(odDerivationRules == NULL);
}

ObjectDictionary* KWDerivationRule::odDerivationRules = NULL;

boolean KWDerivationRule::IsStructureRule() const
{
	return false;
}

int KWDerivationRuleFullCompare(const void* elem1, const void* elem2)
{
	KWDerivationRule* rule1;
	KWDerivationRule* rule2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	rule1 = cast(KWDerivationRule*, *(Object**)elem1);
	rule2 = cast(KWDerivationRule*, *(Object**)elem2);
	require(rule1->IsCompiled());
	require(rule2->IsCompiled());

	// Difference
	nDiff = rule1->FullCompare(rule2);
	return nDiff;
}
