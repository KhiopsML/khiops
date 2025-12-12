// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWRelationCreationRule.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRelationCreationRule

KWDRRelationCreationRule::KWDRRelationCreationRule()
{
	SetType(KWType::ObjectArray);
	bVariableOutputOperandNumber = false;
	kwcCompiledTargetClass = NULL;
}

KWDRRelationCreationRule::~KWDRRelationCreationRule()
{
	oaOutputOperands.DeleteAll();
	oaViewModeCopyBlockRules.DeleteAll();
}

boolean KWDRRelationCreationRule::GetReference() const
{
	require(KWType::IsRelation(GetType()));
	return false;
}

void KWDRRelationCreationRule::SetOutputOperandNumber(int nValue)
{
	KWDerivationRuleOperand* operand;
	int i;

	require(nValue >= 0);

	// Supression des OutputOperandes surnumeraires
	if (nValue < oaOutputOperands.GetSize())
	{
		for (i = nValue; i < oaOutputOperands.GetSize(); i++)
			delete oaOutputOperands.GetAt(i);
		oaOutputOperands.SetSize(nValue);
	}
	// Ajout des OutputOperandes en plus
	else if (nValue > oaOutputOperands.GetSize())
	{
		for (i = oaOutputOperands.GetSize(); i < nValue; i++)
		{
			operand = new KWDerivationRuleOperand;
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			oaOutputOperands.Add(operand);
		}
	}
	nFreshness++;
}

void KWDRRelationCreationRule::AddOutputOperand(KWDerivationRuleOperand* operand)
{
	require(operand != NULL);

	oaOutputOperands.Add(operand);
	nFreshness++;
}

void KWDRRelationCreationRule::DeleteAllOutputOperands()
{
	oaOutputOperands.DeleteAll();
	nFreshness++;
}

void KWDRRelationCreationRule::RemoveAllOutputOperands()
{
	oaOutputOperands.RemoveAll();
	nFreshness++;
}

void KWDRRelationCreationRule::DeleteAllVariableOutputOperands()
{
	KWDerivationRule* kwdrReference;
	int nOutputOperand;

	require(KWDerivationRule::LookupDerivationRule(GetName()) != NULL);

	// Recherche de la regle de derivation de reference
	kwdrReference = KWDerivationRule::LookupDerivationRule(GetName());

	// Supression des derniers OutputOperandes si la regle est a nombre variables d'OutputOperandes
	if (kwdrReference->GetVariableOutputOperandNumber())
	{
		// Supression des derniers OutputOperandes
		assert(kwdrReference->GetOutputOperandNumber() >= 1);
		for (nOutputOperand = kwdrReference->GetOutputOperandNumber() - 1;
		     nOutputOperand < GetOutputOperandNumber(); nOutputOperand++)
		{
			delete oaOutputOperands.GetAt(nOutputOperand);
			oaOutputOperands.SetAt(nOutputOperand, NULL);
		}
		oaOutputOperands.SetSize(kwdrReference->GetOutputOperandNumber() - 1);

		// Mise a jour de la fraicheur
		nFreshness++;
	}
}

void KWDRRelationCreationRule::RenameClass(const KWClass* refClass, const ALString& sNewClassName)
{
	int i;
	KWDerivationRuleOperand* operand;

	require(refClass != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::RenameClass(refClass, sNewClassName);

	// Specialisation pour les operandes en sortie
	if (sNewClassName != refClass->GetName())
	{
		// Parcours des operandes en sortie
		for (i = 0; i < oaOutputOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(i));
			assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);

			// Type objet de l'operande
			if (KWType::IsGeneralRelation(operand->GetType()) and
			    operand->GetObjectClassName() == refClass->GetName())
				operand->SetObjectClassName(sNewClassName);
		}
	}
}

boolean KWDRRelationCreationRule::CheckRuleDefinition() const
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckRuleDefinition();

	// Specialisation pour les operandes en sortie
	if (bOk)
	{
		// Au moins un operande pour les regles a nombre variable d'operandes en sortie
		// uniquement pour la regle "generique"
		if (LookupDerivationRule(GetName()) == this)
		{
			if (GetVariableOutputOperandNumber() and oaOutputOperands.GetSize() == 0)
			{
				AddError("The definition of a registered derivation rule with a variable number of "
					 "output operands must contain at least one operand");
				bOk = false;
			}
		}
	}
	return bOk;
}

boolean KWDRRelationCreationRule::CheckOperandsDefinition() const
{
	boolean bOk;
	boolean bIsGenericRule;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sOperandOrigin;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsDefinition();

	// Recherche s'il s'agit de la regle generique enregistree
	bIsGenericRule = LookupDerivationRule(GetName()) == this;

	// Specialisation pour les alimentations de type vue
	if (bOk)
	{
		if (IsViewModeActivated())
		{
			if (GetOperandNumber() == 0)
			{
				AddError(sTmp + "At least one operand is mandatory");
				bOk = false;
			}
			else if (not KWType::IsRelation(GetFirstOperand()->GetType()))
			{
				AddError(sTmp + "Incorrect first operand whose type (" +
					 KWType::ToString(GetFirstOperand()->GetType()) + " should be " +
					 KWType::ToString(KWType::Object) + " or " +
					 KWType::ToString(KWType::ObjectArray));
				bOk = false;
			}
		}
	}

	// Specialisation pour les operandes en sortie
	if (bOk)
	{
		// Verification des operandes en sortie
		for (i = 0; i < oaOutputOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(i));

			// Controle de base
			if (not operand->CheckDefinition())
			{
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1));
				bOk = false;
			}

			// On ne peut avoir que l'origine attribut aux operandes en sortie
			if (bOk and operand->GetOrigin() != KWDerivationRuleOperand::OriginAttribute)
			{
				sOperandOrigin =
				    KWDerivationRuleOperand::OriginToString(operand->GetOrigin(), operand->GetType());
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1) +
					 " that must be used with an origin variable (origin " + sOperandOrigin +
					 " not allowed)");
				bOk = false;
			}

			// Verification de la compatibilite entre niveau de scope et origine de l'operande
			if (bOk and operand->GetScopeLevel() > 0)
			{
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1) +
					 " that cannot be used with scope prefix (" +
					 ALString('.', operand->GetScopeLevel()) + ")");
				bOk = false;
			}

			// On ne peut avoir que des type de donnees dense pour les operandes en sortie
			if (bOk and not IsValidOutputOperandType(operand->GetType()) and
			    operand->GetType() != KWType::Unknown)
			{
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1) +
					 " that cannot be specified with type " + KWType::ToString(operand->GetType()));
				bOk = false;
			}

			// Pour la regle generique, seul le dernier operande, en cas de nombre variables d'operandes,
			// a le droit d'etre de type Unknown
			if (bOk and bIsGenericRule)
			{
				if ((i < oaOutputOperands.GetSize() - 1 or not GetVariableOutputOperandNumber()) and
				    operand->GetType() == KWType::Unknown)
				{
					AddError(
					    "In the definition of a registered derivation rule with a variable "
					    "number of operands, first output operands must have their type specified");
					bOk = false;
				}
			}
		}
	}
	return bOk;
}

boolean KWDRRelationCreationRule::CheckRuleFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk;
	ALString sTmp;

	require(not ruleFamily->GetReference());

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckRuleFamily(ruleFamily);

	// Specialisation pour les operandes en sortie
	if (bOk)
	{
		// Verification du nombre d'operandes en sortie
		// (exactement, ou pas dans le cas des nombres variables d'operandes)
		if ((ruleFamily->GetVariableOutputOperandNumber() and
		     GetOutputOperandNumber() < ruleFamily->GetOutputOperandNumber() - 1) or
		    (not ruleFamily->GetVariableOutputOperandNumber() and
		     GetOutputOperandNumber() != ruleFamily->GetOutputOperandNumber()))
		{
			AddError(sTmp + "Number of output operands (" + IntToString(GetOutputOperandNumber()) +
				 ") inconsistent with that of the registered rule (" +
				 IntToString(ruleFamily->GetOutputOperandNumber()) + ")");
			bOk = false;
		}
	}
	return bOk;
}

boolean KWDRRelationCreationRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk;
	KWDerivationRuleOperand* operand;
	KWDerivationRuleOperand* outputOperand;
	KWDerivationRuleOperand* familyVariableOperand;
	KWDerivationRuleOperand* familyVariableOutputOperand;
	int nInputVariableOperandNumber;
	int nOutputVariableOperandNumber;
	int i;
	int nInput;
	int nOutput;
	ALString sTmp;

	require(not ruleFamily->GetReference());

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsFamily(ruleFamily);

	// Specialisation dans le cas de nombre variable d'operande a la fois en entree et en sortie
	if (bOk and GetVariableOperandNumber() and GetVariableOutputOperandNumber())
	{
		// Recherche des nombre effectifs d'operande en nombre variable
		nInputVariableOperandNumber = GetOperandNumber() - ruleFamily->GetOperandNumber() + 1;
		nOutputVariableOperandNumber = GetOutputOperandNumber() - ruleFamily->GetOutputOperandNumber() + 1;

		// Erreur si nombre differents
		if (nOutputVariableOperandNumber != nInputVariableOperandNumber)
		{
			// Specialisation des messages d'erreur pour les cas extremes
			if (nOutputVariableOperandNumber == 0)
				AddError(sTmp + "Missing output operands (expected " +
					 IntToString(nInputVariableOperandNumber) +
					 ") corresponding to the input operands");
			else if (nInputVariableOperandNumber == 0)
				AddError(sTmp + "Missing input operands (expected " +
					 IntToString(nOutputVariableOperandNumber) +
					 ") corresponding to the output operands");
			// Message d'erreur standard
			else
				AddError(sTmp + "Mismatch between the number of output and input operands (" +
					 IntToString(nOutputVariableOperandNumber) + " output operands for " +
					 IntToString(nInputVariableOperandNumber) + " corresponding input operands) ");
			bOk = false;
		}
	}

	// Specialisation dans le cas d'un nombre variable d'operandes en entree
	if (bOk and ruleFamily->GetVariableOperandNumber())
	{
		familyVariableOperand = ruleFamily->GetOperandAt(ruleFamily->GetOperandNumber() - 1);

		// Verification si le type dans la famille est Unknown, et qu'il n'est donc pas verifie dans la classe ancetre
		if (familyVariableOperand->GetType() == KWType::Unknown)
		{
			// Verification des operandes en nombres variables
			for (i = ruleFamily->GetOperandNumber() - 1; i < oaOperands.GetSize(); i++)
			{
				operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

				// On ne teste pas le cas d'un operande de type inconnu, qui correspond a une variable
				// n'existant pas, et qui est diagnostique precisement dans CheckOperandsCompleteness
				if (operand->GetType() != KWType::Unknown)
				{
					if (not IsValidOutputOperandType(operand->GetType()))
					{
						AddError(sTmp + "Operand " + IntToString(i + 1) + " with wrong " +
							 KWType::ToString(operand->GetType()) +
							 " type (must be a data or relation type)");
						bOk = false;
					}
				}
			}
		}
	}

	// Specialisation pour les operandes en sortie
	if (bOk)
	{
		// Verification des operandes en sortie
		for (i = 0; i < oaOutputOperands.GetSize(); i++)
		{
			outputOperand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(i));

			// Cas du dernier operande pour un nombre variable d'operandes
			if (ruleFamily->GetVariableOutputOperandNumber() and
			    i >= ruleFamily->GetOutputOperandNumber() - 1)
			{
				familyVariableOutputOperand =
				    ruleFamily->GetOutputOperandAt(ruleFamily->GetOutputOperandNumber() - 1);

				// On ne teste pas le cas d'un operande de type inconnu, qui correspond a une variable
				// n'existant pas, et qui est diagnostique precisement dans CheckOperandsCompleteness
				if (outputOperand->GetType() != KWType::Unknown)
				{
					// Cas d'un operande de type inconnu dans la famille
					if (familyVariableOutputOperand->GetType() == KWType::Unknown)
					{
						// Test d'un type valid en sortie
						if (not IsValidOutputOperandType(outputOperand->GetType()))
						{
							AddError(sTmp + "Output operand " + IntToString(i + 1) +
								 " with wrong " +
								 KWType::ToString(outputOperand->GetType()) +
								 " type (must be a data or relation type)");
							bOk = false;
						}
					}
					// Verification avec le bon type sinon
					else
					{
						if (not outputOperand->CheckFamily(familyVariableOutputOperand))
						{
							AddError(sTmp + "Output operand " + IntToString(i + 1) +
								 " inconsistent with that of the registered rule");
							bOk = false;
						}
					}
				}
			}
			// Cas des premiers operandes
			else
			{
				assert(ruleFamily->GetOutputOperandAt(i)->GetType() != KWType::Unknown);
				if (not outputOperand->CheckFamily(ruleFamily->GetOutputOperandAt(i)))
				{
					AddError(sTmp + "Output operand " + IntToString(i + 1) +
						 " inconsistent with that of the registered rule");
					bOk = false;
				}
			}
		}
	}

	// Verification de la coherence des types dans le cas de nombre variable d'operandes
	// a la fois en entree et en sortie
	if (bOk and GetVariableOperandNumber() and GetVariableOutputOperandNumber())
	{
		// Recherche des nombre effectifs d'operande en nombre variable
		nInputVariableOperandNumber = GetOperandNumber() - ruleFamily->GetOperandNumber() + 1;
		nOutputVariableOperandNumber = GetOutputOperandNumber() - ruleFamily->GetOutputOperandNumber() + 1;
		assert(nInputVariableOperandNumber == nOutputVariableOperandNumber);

		// Recherche des operandes de la regle enregistree
		familyVariableOperand = ruleFamily->GetOperandAt(ruleFamily->GetOperandNumber() - 1);
		familyVariableOutputOperand = ruleFamily->GetOutputOperandAt(ruleFamily->GetOutputOperandNumber() - 1);

		// Verification de la compatibilite de type des operandes en nombres variables
		for (i = 0; i < nInputVariableOperandNumber; i++)
		{
			nInput = ruleFamily->GetOperandNumber() - 1 + i;
			nOutput = ruleFamily->GetOutputOperandNumber() - 1 + i;
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(nInput));
			outputOperand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(nOutput));

			// On ne verifie la compatibilite que si le type est non defini dans la regle enregistree
			if (familyVariableOperand->GetType() == KWType::Unknown and
			    familyVariableOutputOperand->GetType() == KWType::Unknown)
			{
				// On ne teste pas le cas d'operandes de type inconnu, disgnostiques dans CheckOperandsCompleteness
				if (operand->GetType() != KWType::Unknown and
				    outputOperand->GetType() != KWType::Unknown)
				{
					assert(IsValidOutputOperandType(operand->GetType()));
					assert(IsValidOutputOperandType(outputOperand->GetType()));

					// Type
					if (operand->GetType() != outputOperand->GetType())
					{
						AddError("Type " + KWType::ToString(operand->GetType()) +
							 " of input operand " + IntToString(nInput + 1) +
							 " inconsistent with type " +
							 KWType::ToString(outputOperand->GetType()) +
							 " of corresponding output operand " +
							 IntToString(nOutput + 1));
						bOk = false;
					}

					// Nom de la classe pour un type Object ou ObjectArray si renseigne
					if (bOk and KWType::IsRelation(operand->GetType()) and
					    outputOperand->GetObjectClassName() != "" and
					    operand->GetObjectClassName() != outputOperand->GetObjectClassName())
					{
						AddError("Dictionary " + operand->GetObjectClassName() +
							 " used with type " + KWType::ToString(operand->GetType()) +
							 " of input operand " + IntToString(nInput + 1) +
							 " inconsistent with the dictionary " +
							 outputOperand->GetObjectClassName() +
							 " used with corresponding output operand " +
							 IntToString(nOutput + 1));
						bOk = false;
					}
				}
			}
		}
	}
	return bOk;
}

boolean KWDRRelationCreationRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWClass* kwcSourceClass;
	KWClass* kwcTargetClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	KWAttributeBlock* sourceBlock;
	KWAttributeBlock* targetBlock;
	ObjectDictionary odOutputAttributeNames;
	KWDerivationRuleOperand* operand;
	ALString sLabel;
	int i;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Specialisation
	if (bOk)
	{
		// Recherche de la classe cible
		kwcTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
		assert(kwcTargetClass != NULL);

		// La classe cible ne doit pas etre Root
		if (kwcTargetClass->GetRoot())
		{
			AddError(sTmp + "Invalid output dictionary " + kwcTargetClass->GetName() +
				 " that cannot be a root dictionary, dedicated to managing external tables");
			bOk = false;
		}

		// Gestion de l'alimentation de type calcul via les operandes en sortie
		if (bOk and GetOutputOperandNumber() > 0)
		{
			// Collecte des noms des attributs en sortie en verifiant leur unicite
			for (i = 0; i < GetOutputOperandNumber(); i++)
			{
				operand = GetOutputOperandAt(i);
				assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
				assert(IsValidOutputOperandType(operand->GetType()) or
				       operand->GetType() == KWType::Unknown);

				// Message special dans le cas d'un type inconnu, qui correspond a une variable non
				// trouvee dans le dictionnaire en sortie
				if (operand->GetType() == KWType::Unknown)
				{
					assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
					assert(operand->GetDataItemName() != "");
					assert(kwcTargetClass->LookupAttribute(operand->GetDataItemName()) == NULL);

					// On passe par la methode GetDataItemName, car le type n'est pas valide
					AddError(sTmp + "Invalid output operand " + IntToString(i + 1) + ", as the " +
						 operand->GetDataItemName() + " variable is not found in the " +
						 kwcTargetClass->GetName() + " output dictionary");
					bOk = false;
				}
				// Verification de l'operande en sortie dans le cas general
				else if (not operand->CheckCompleteness(kwcTargetClass))
				{
					AddError(sTmp + "Incomplete output operand " + IntToString(i + 1) +
						 " related to the " + kwcTargetClass->GetName() + " output dictionary");
					bOk = false;
				}
				// Verification que l'attribut en sortie n'est pas dans un bloc
				else
				{
					targetAttribute = kwcTargetClass->LookupAttribute(operand->GetDataItemName());
					assert(targetAttribute != NULL);

					if (targetAttribute->IsInBlock())
					{
						AddError(sTmp + "Invalid output operand " + IntToString(i + 1) +
							 ", as the " + operand->GetDataItemName() +
							 " variable should not be in a variable block (" +
							 targetAttribute->GetAttributeBlock()->GetName() + ") in the " +
							 kwcTargetClass->GetName() + " output dictionary");
						bOk = false;
					}
				}

				// Verification de l'unicite des noms d'attributs des operandes en sortie
				if (bOk)
				{
					if (odOutputAttributeNames.Lookup(operand->GetAttributeName()) == NULL)
						odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);
					else
					{
						AddError(sTmp + "Output operand " + IntToString(i + 1) + " with the " +
							 operand->GetAttributeName() +
							 " variable already used in the " + kwcTargetClass->GetName() +
							 " output dictionary");
						bOk = false;
					}
				}

				// Arret en cas d'erreur
				if (not bOk)
					break;
			}
		}

		// Validation des attributs natifs de la classe cible
		if (bOk)
		{
			// Recherche de la classe source dans le cas d'une alimentation de type vue
			kwcSourceClass = NULL;
			if (IsViewModeActivated())
			{
				assert(GetOperandNumber() > 0);
				assert(KWType::IsRelation(GetFirstOperand()->GetType()));
				kwcSourceClass =
				    kwcOwnerClass->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
				assert(kwcSourceClass != NULL);
			}

			// Parcours des attributs natif de la classe cible
			targetAttribute = kwcTargetClass->GetHeadAttribute();
			while (targetAttribute != NULL)
			{
				// On ne traite que les attributs natifs
				if (targetAttribute->GetAnyDerivationRule() == NULL)
				{
					assert(IsValidOutputOperandType(targetAttribute->GetType()));

					// On verifie si l'attribut natif n'est pas deja alimente par les operandes en sortie
					// pour se focaliser ici sur les controle de type vue
					if (odOutputAttributeNames.Lookup(targetAttribute->GetName()) == NULL)
					{
						// Erreur si pas attribut natif source de meme nom
						sourceAttribute = NULL;
						if (not IsViewModeActivated())
						{
							AddError("In the " + kwcTargetClass->GetName() +
								 " output dictionary, the " +
								 targetAttribute->GetName() +
								 " variable is not set by any output operand");
							bOk = false;
						}
						// Controle avance dans le cas d'une alimentation de type vue
						else
						{
							assert(IsViewModeActivated());
							assert(kwcSourceClass != NULL);

							// Recherche de l'attribut source
							sourceAttribute =
							    kwcSourceClass->LookupAttribute(targetAttribute->GetName());

							// Erreur si pas d'attribut correspondant trouve
							if (sourceAttribute == NULL)
							{
								AddViewModeError(kwcSourceClass, kwcTargetClass,
										 targetAttribute, "must exist");
								bOk = false;
							}
							// Erreur si le type trouve est incorrect
							else if (sourceAttribute->GetType() !=
								 targetAttribute->GetType())
							{
								sLabel = "is found with a different type (" +
									 sourceAttribute->GetTypeLabel() + ")";
								AddViewModeError(kwcSourceClass, kwcTargetClass,
										 targetAttribute, sLabel);
								bOk = false;
							}

							// Erreur si le type de relation est incorrect
							if (bOk and KWType::IsRelation(sourceAttribute->GetType()) and
							    sourceAttribute->GetClass()->GetName() !=
								targetAttribute->GetClass()->GetName())
							{
								sLabel = "is found with a different type (" +
									 sourceAttribute->GetTypeLabel() + ")";
								AddViewModeError(kwcSourceClass, kwcTargetClass,
										 targetAttribute, sLabel);
								bOk = false;
							}

							// Erreur dans le cas d'un bloc cible si bloc source incompatible
							if (bOk and targetAttribute->IsInBlock())
							{
								assert(sourceAttribute->GetType() ==
								       targetAttribute->GetType());

								// Acces aux eventuels blocs source et cible
								sourceBlock = sourceAttribute->GetAttributeBlock();
								targetBlock = targetAttribute->GetAttributeBlock();
								assert(targetAttribute != NULL);

								// Un attribut cible peut etre dans un bloc si l'attribut source est dans un bloc
								// Meme nom, meme type, meme VarKey, meme nom de bloc
								// Si un attribut cible est dense, l'attribut surce doit etre dense
								if (not sourceAttribute->IsInBlock())
								{
									sLabel = "is in a block named " +
										 targetAttribute->GetName() +
										 " and should also be in a block";
									AddViewModeError(kwcSourceClass, kwcTargetClass,
											 targetAttribute, sLabel);
									bOk = false;
								}
								// Erreur si nom de bloc different
								else if (targetAttribute->GetAttributeBlock()
									     ->GetName() !=
									 sourceAttribute->GetAttributeBlock()
									     ->GetName())
								{
									sLabel = "is in a block named " +
										 targetBlock->GetName() +
										 " and is found in a block with a "
										 "different name (" +
										 sourceBlock->GetName() + ")";
									AddViewModeError(kwcSourceClass, kwcTargetClass,
											 targetAttribute, sLabel);
									bOk = false;
								}
								// Erreur si type de VarKey different
								else if (targetBlock->GetVarKeyType() !=
									 sourceAttribute->GetAttributeBlock()
									     ->GetVarKeyType())
								{
									sLabel =
									    "is in a block named " +
									    targetBlock->GetName() +
									    " with VarKey type " +
									    KWType::ToString(
										targetBlock->GetVarKeyType()) +
									    " and is found in a block with a different "
									    "VarKey type (" +
									    KWType::ToString(
										sourceBlock->GetVarKeyType()) +
									    ")";
									AddViewModeError(kwcSourceClass, kwcTargetClass,
											 targetAttribute, sLabel);
									bOk = false;
								}
								// Erreur si valeur de VarKey different
								else if (sourceBlock->GetStringVarKey(
									     sourceAttribute) !=
									 targetBlock->GetStringVarKey(targetAttribute))
								{
									sLabel = "is in a block named " +
										 targetBlock->GetName() +
										 " with VarKey value " +
										 targetBlock->GetStringVarKey(
										     targetAttribute) +
										 " and is found with a different "
										 "VarKey value (" +
										 sourceBlock->GetStringVarKey(
										     sourceAttribute) +
										 ")";
									AddViewModeError(kwcSourceClass, kwcTargetClass,
											 targetAttribute, sLabel);
									bOk = false;
								}
							}
						}
					}

					// Arret si erreur
					if (not bOk)
						break;
				}

				// Attribut suivant
				kwcTargetClass->GetNextAttribute(targetAttribute);
			}
		}
	}
	return bOk;
}

boolean KWDRRelationCreationRule::ContainsCycle(NumericKeyDictionary* nkdGreyAttributes,
						NumericKeyDictionary* nkdBlackAttributes) const
{
	boolean bContainsCycle;
	KWClass* kwcTargetClass;
	KWAttribute* attribute;
	KWDerivationRule* rule;

	// Appel de la methode ancetre
	bContainsCycle = KWDerivationRule::ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);

	// Specialisation pour une regle de creation d'instance
	// On propage la detection de cycle au dictionnaire des instances creees en sortie, qui potentiellement
	// pourrait creer des instances en boucle infinie
	// Il n'est pas par contre necessaire de detecter les cycles de calcul sur les attributs sources de vue
	// dans le cas d'une alimentation de type vue, car cette detection est de toute facon effectuee
	// pour chaque dictionnaire
	if (not bContainsCycle)
	{
		// Recherche de la classe cible
		kwcTargetClass = GetOwnerClass()->GetDomain()->LookupClass(GetObjectClassName());

		// Parcours des attributs de la classe, en ne s'interessant qu'aux attributs ayant une regle de creation d'instance
		// Les cycles via les regles de derivation standard sont deja detectes par ailleurs
		attribute = kwcTargetClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Les regles de bloc de type relation sont forcement de type reference
			assert(attribute->GetBlockDerivationRule() == NULL or
			       not KWType::IsGeneralRelation(attribute->GetType()) or
			       attribute->GetBlockDerivationRule()->GetReference());

			// Test si l'attribut est en White (ni Grey, ni Black)
			rule = attribute->GetDerivationRule();
			if (rule != NULL and KWType::IsRelation(rule->GetType()) and not rule->GetReference())

			{
				// L'attribut est marque en Grey: presence d'une cycle
				if (nkdGreyAttributes->Lookup(attribute) != NULL)
				{
					GetOwnerClass()->AddError(
					    "Existing derivation cycle caused by the recursive use of "
					    "variable " +
					    attribute->GetName() + " in target dictionary " +
					    kwcTargetClass->GetName() + " built using " + GetName() + " rule");
					bContainsCycle = true;
				}
				// Attribut non marque: il faut continuer l'analyse
				else if (nkdBlackAttributes->Lookup(attribute) == NULL)
					bContainsCycle =
					    attribute->ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);
			}

			// Arret en cas de cycle
			if (bContainsCycle)
				break;

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(attribute);
		}
	}
	return bContainsCycle;
}

void KWDRRelationCreationRule::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;
	ObjectDictionary odOutputAttributeNames;
	KWClass* kwcSourceClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	int nAttribute;
	KWAttributeBlock* sourceBlock;
	KWAttributeBlock* targetBlock;
	int nBlock;
	KWDerivationRuleOperand* operand;
	KWDerivationRule* valueBlockRule;
	KWLoadIndex liInvalid;
	int i;

	require(kwcOwnerClass != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Reinitialisation des resultats de compilation
	livComputeModeTargetAttributeLoadIndexes.SetSize(0);
	ivComputeModeTargetAttributeTypes.SetSize(0);
	livViewModeSourceAttributeLoadIndexes.SetSize(0);
	livViewModeTargetAttributeLoadIndexes.SetSize(0);
	ivViewModeTargetAttributeTypes.SetSize(0);
	livViewModeSourceBlockLoadIndexes.SetSize(0);
	livViewModeTargetBlockLoadIndexes.SetSize(0);
	ivViewModeTargetBlockTypes.SetSize(0);
	oaViewModeCopyBlockRules.DeleteAll();

	// Recherche et memorisation de la classe cible
	kwcCompiledTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
	assert(kwcCompiledTargetClass != NULL);

	// Trace initiale
	if (bTrace)
	{
		cout << "Compile rule " << GetName() << "\n";
		cout << "  Owner dictionary\t" << kwcOwnerClass->GetName() << "\t"
		     << kwcOwnerClass->GetDomain()->GetName() << "\t" << kwcOwnerClass->GetCompileFreshness() << "\t"
		     << kwcOwnerClass << "\n";
		cout << "  Target dictionary\t" << kwcCompiledTargetClass->GetName() << "\t"
		     << kwcCompiledTargetClass->GetDomain()->GetName() << "\t"
		     << kwcCompiledTargetClass->GetCompileFreshness() << "\t" << kwcCompiledTargetClass << "\n";
	}

	// Compilation dans le cas d'une alimentation de type calcul
	if (GetOutputOperandNumber() > 0)
	{
		// Trace dediee
		if (bTrace)
			cout << "\tOperand index\tVariable\tType\tTarget index\n";

		// Collecte des nom des attribut en sortie en verifiant leur unicite
		for (i = 0; i < GetOutputOperandNumber(); i++)
		{
			operand = GetOutputOperandAt(i);
			assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
			assert(IsValidOutputOperandType(operand->GetType()));

			// Memorisation du nom de l'attribut en sortie
			odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);

			// Recherche de l'attribut cible
			targetAttribute = kwcCompiledTargetClass->LookupAttribute(operand->GetAttributeName());
			assert(targetAttribute != NULL);
			assert(IsValidOutputOperandType(targetAttribute->GetType()));

			// Memorisation des infos de chargement, que l'attribut cible soit Loaded ou non
			ivComputeModeTargetAttributeTypes.Add(targetAttribute->GetType());
			if (targetAttribute->GetUsed())
				livComputeModeTargetAttributeLoadIndexes.Add(targetAttribute->GetLoadIndex());
			// On utilise un index invalide dans le cas d'un attribut non utilise
			// C'est en particulier le cas pour les attributs de type relation internes,
			// gardes en memoire uniquement pour des raisons de liberation de la memoire
			else
				livComputeModeTargetAttributeLoadIndexes.Add(liInvalid);

			// Trace par attribut gere par une alimentation de type vue
			if (bTrace)
			{
				cout << "\t" << i + 1;
				cout << "\t" << (targetAttribute->GetUsed() ? "" : "Unused");
				cout << "\t" << targetAttribute->GetName();
				cout << "\t" << KWType::ToString(targetAttribute->GetType());
				cout << "\t" << targetAttribute->GetLoadIndex();
				cout << "\t" << livComputeModeTargetAttributeLoadIndexes.GetAt(i);
				cout << "\n";
			}
		}
	}

	// Compilation dans le cas d'une alimentation de type vue
	if (IsViewModeActivated())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche de la classe source
		kwcSourceClass = kwcOwnerClass->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
		assert(kwcSourceClass != NULL);

		// Trace dediee
		if (bTrace)
		{
			cout << "  Source dictionary\t" << kwcSourceClass->GetName() << "\t"
			     << kwcSourceClass->GetDomain()->GetName() << "\t" << kwcSourceClass->GetCompileFreshness()
			     << "\t" << kwcSourceClass << "\n";
			cout << "\tVariable\tType\tSource index\tTarget index\n";
		}

		// Parcours des attributs natifs de la classe cible charges en memoire
		for (nAttribute = 0; nAttribute < kwcCompiledTargetClass->GetLoadedDenseAttributeNumber(); nAttribute++)
		{
			targetAttribute = kwcCompiledTargetClass->GetLoadedDenseAttributeAt(nAttribute);

			// On ne traite que les attributs natifs utilises non deja prise en compte
			// par une alimentation de type calcul
			if (targetAttribute->GetDerivationRule() == NULL and
			    odOutputAttributeNames.Lookup(targetAttribute->GetName()) == NULL)
			{
				assert(IsValidOutputOperandType(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());
				assert(sourceAttribute != NULL);
				assert(sourceAttribute->GetType() == targetAttribute->GetType());

				// Memorisation des infos de chargement si l'attribut source est charge
				// Il peut ne pas etre charge dans le dictionnaire "logique", mais il sera
				// de toute facon charge dans le dictionnaire "physique"
				// Cf. gestion des dictionnaire logiques et physiques dans KWDatabase
				if (sourceAttribute->GetLoaded())
				{
					livViewModeSourceAttributeLoadIndexes.Add(sourceAttribute->GetLoadIndex());
					livViewModeTargetAttributeLoadIndexes.Add(targetAttribute->GetLoadIndex());
					ivViewModeTargetAttributeTypes.Add(targetAttribute->GetType());

					// Trace par attribut gere par une alimentation de type vue
					if (bTrace)
					{
						cout << "\t" << sourceAttribute->GetName();
						cout << "\t" << KWType::ToString(sourceAttribute->GetType());
						cout << "\t" << sourceAttribute->GetLoadIndex();
						cout << "\t" << targetAttribute->GetLoadIndex();
						cout << "\n";
					}
				}
			}
		}

		// Parcours des blocs d'attributs natifs de la classe cible charges en memoire
		for (nBlock = 0; nBlock < kwcCompiledTargetClass->GetLoadedAttributeBlockNumber(); nBlock++)
		{
			targetBlock = kwcCompiledTargetClass->GetLoadedAttributeBlockAt(nBlock);

			// On ne traite que les blocs d'attributs natifs utilises
			if (targetBlock->GetDerivationRule() == NULL)
			{
				assert(IsValidOutputOperandType(targetBlock->GetType()));

				// Recherche d'un bloc d'attributs natif source de meme nom
				sourceBlock = kwcSourceClass->LookupAttributeBlock(targetBlock->GetName());
				assert(sourceBlock != NULL);
				assert(sourceBlock->GetType() == targetBlock->GetType());

				// Memorisation des infos de chargement si l'attribut source est charge
				// Il peut ne pas etre charge dans le dictionnaire "logique", mais il sera
				// de toute facon charge dans le dictionnaire "physique"
				// Cf. gestion des dictionnaire logiques et physiques dans KWDatabase
				if (sourceBlock->GetLoaded())
				{
					livViewModeSourceBlockLoadIndexes.Add(sourceBlock->GetLoadIndex());
					livViewModeTargetBlockLoadIndexes.Add(targetBlock->GetLoadIndex());
					ivViewModeTargetBlockTypes.Add(targetBlock->GetType());

					// Creation et memorisation d'une regle de type CopyBlock du bon type
					assert(KWType::IsSimple(sourceBlock->GetType()));
					if (sourceBlock->GetType() == KWType::Continuous)
						valueBlockRule = new KWDRCopyContinuousValueBlock;
					else
						valueBlockRule = new KWDRCopySymbolValueBlock;
					oaViewModeCopyBlockRules.Add(valueBlockRule);

					// Specification et compilation de la regle
					valueBlockRule->GetFirstOperand()->SetOrigin(
					    KWDerivationRuleOperand::OriginAttribute);
					valueBlockRule->GetFirstOperand()->SetAttributeBlockName(
					    sourceBlock->GetName());
					valueBlockRule->CompleteTypeInfo(kwcSourceClass);
					valueBlockRule->Compile(kwcSourceClass);

					// Trace par attribut gere par une alimentation de type vue
					if (bTrace)
					{
						cout << "\t" << sourceBlock->GetName();
						cout << "\t" << KWType::ToString(sourceBlock->GetType());
						cout << "\t" << sourceBlock->GetLoadIndex();
						cout << "\t" << targetBlock->GetLoadIndex();
						cout << "\n";
					}
				}
			}
		}
	}
}

void KWDRRelationCreationRule::BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
						      NumericKeyDictionary* nkdAllUsedAttributes) const
{
	boolean bTrace = false;
	ObjectDictionary odOutputAttributeNames;
	IntVector ivUsedInputOperands;
	IntVector ivUsedOutputOperands;
	KWClass* kwcSourceClass;
	KWClass* kwcTargetClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	KWDerivationRuleOperand* operand;
	int nOperand;
	boolean bIsTargetAttributeUsed;

	require(IsCompiled());
	require(derivedAttribute != NULL);
	require(nkdAllUsedAttributes != NULL);
	require(KWType::IsRelation(derivedAttribute->GetType()));
	require(derivedAttribute->GetClass()->GetName() == GetObjectClassName());

	// Trace de debut
	if (bTrace)
	{
		cout << "Begin KWDRRelationCreationRule::BuildAllUsedAttributes " << GetName() << ", "
		     << derivedAttribute->GetParentClass()->GetName() << " " << derivedAttribute->GetName() << "\n";
		cout << "- target class: " << GetObjectClassName() << "\n";
	}

	// Recherche des attributs cibles utilises dans le cas d'une alimentation de type calcul
	if (GetOutputOperandNumber() > 0)
	{
		// Initialisation des vecteurs des operandes utilises en entree et en sortie
		ivUsedInputOperands.SetSize(GetOperandNumber());
		ivUsedOutputOperands.SetSize(GetOutputOperandNumber());

		// Recherche de la classe cible
		kwcTargetClass = derivedAttribute->GetClass();
		assert(kwcTargetClass != NULL);

		// Trace
		if (bTrace)
			cout << "- output operands: " << GetOutputOperandNumber() << "\n";

		// Collecte des attributs en sortie
		// On optimise les impacts en n'exploitant que les attributs en sortie utilises
		// - on ne collecte pas les attributs cibles dans methode
		// - on se base en fait sur leur utilisation par d'autre regles
		for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
		{
			operand = GetOutputOperandAt(nOperand);
			assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
			assert(KWType::IsData(operand->GetType()));

			// Recherche de l'attribut cible correspond a l'operande en sortie
			targetAttribute = kwcTargetClass->LookupAttribute(operand->GetAttributeName());
			assert(targetAttribute != NULL);

			// Recherche si l'attribut cible est utilise
			bIsTargetAttributeUsed = nkdAllUsedAttributes->Lookup(targetAttribute) != NULL;

			// Memorisation de l'utilisation de l'operande
			if (bIsTargetAttributeUsed)
				ivUsedOutputOperands.SetAt(nOperand, 1);

			// Memorisation du nom de l'attribut en sortie pour les distinguer des attributs de type view
			odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);

			// Trace
			if (bTrace)
				cout << "  - " << nOperand + 1 << ": " << operand->GetAttributeName() << "\t"
				     << BooleanToString(bIsTargetAttributeUsed) << "\n";
		}

		// On en en deduit la sous-partie des operandes en entree a utiliser
		CollectUsedInputOperands(&ivUsedOutputOperands, &ivUsedInputOperands);

		// Trace
		if (bTrace)
		{
			cout << "- used input operands: " << GetOperandNumber() << "\n";
			for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
			{
				cout << "  - " << nOperand + 1 << ": "
				     << BooleanToString(ivUsedInputOperands.GetAt(nOperand) == 1) << "\n";
			}
		}
	}

	// Appel de la methode ancetre s'il n'y a pas d'operandes en sortie
	if (GetOutputOperandNumber() == 0)
		KWDerivationRule::BuildAllUsedAttributes(derivedAttribute, nkdAllUsedAttributes);
	// Sinon, on n'utilise que les operandes necessaire pour les operandes utilises en sortie
	else
	{
		assert(ivUsedInputOperands.GetSize() == GetOperandNumber());
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			if (ivUsedInputOperands.GetAt(nOperand) == 1)
				BuildAllUsedAttributesAtOperand(derivedAttribute, nOperand, nkdAllUsedAttributes);
		}
	}

	// Recherche des attributs cibles utilises dans le cas d'une alimentation de type vue
	if (IsViewModeActivated())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche des classes source et cible
		kwcSourceClass = GetOwnerClass()->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
		kwcTargetClass = derivedAttribute->GetClass();
		assert(kwcSourceClass != NULL);
		assert(kwcTargetClass != NULL);

		// Trace
		if (bTrace)
			cout << "- view class: " << kwcTargetClass->GetName() << "\n";

		// Parcours des attributs natifs de la classe cible
		targetAttribute = kwcTargetClass->GetHeadAttribute();
		while (targetAttribute != NULL)
		{
			// On ne traite que les attributs natifs non deja prise en compte par une alimentation de type calcul
			if (targetAttribute->GetAnyDerivationRule() == NULL and
			    odOutputAttributeNames.Lookup(targetAttribute->GetName()) == NULL)
			{
				assert(IsValidOutputOperandType(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());
				assert(sourceAttribute != NULL);
				assert(sourceAttribute->GetType() == targetAttribute->GetType());

				// Trace
				if (bTrace)
					cout << "  - " << targetAttribute->GetName() << "\t"
					     << BooleanToString(targetAttribute->GetLoaded()) << "\t"
					     << BooleanToString(nkdAllUsedAttributes->Lookup(targetAttribute)) << "\t"
					     << BooleanToString(nkdAllUsedAttributes->Lookup(sourceAttribute)) << "\n";

				// Memorisation de l'attribut cible s'il est utilise directement
				if (targetAttribute->GetLoaded())
				{
					// Memorisation de l'attribut dans le dictionnaire
					if (nkdAllUsedAttributes->Lookup(targetAttribute) == NULL)
						nkdAllUsedAttributes->SetAt(targetAttribute, targetAttribute);
				}

				// Analyse de l'attribut source si necessaire, car l'attribut cible est utilise,
				// directement ou via une autre regle de derivation
				if (nkdAllUsedAttributes->Lookup(targetAttribute) != NULL)
				{
					if (nkdAllUsedAttributes->Lookup(sourceAttribute) == NULL)
					{
						// Memorisation de l'attribut dans le dictionnaire
						nkdAllUsedAttributes->SetAt(sourceAttribute, sourceAttribute);

						// Acces a la regle d'attribut ou de bloc
						if (sourceAttribute->GetAnyDerivationRule() != NULL)
							sourceAttribute->GetAnyDerivationRule()->BuildAllUsedAttributes(
							    sourceAttribute, nkdAllUsedAttributes);
					}
				}
			}

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(targetAttribute);
		}
	}

	// Trace de fin
	if (bTrace)
	{
		cout << "Begin KWDRRelationCreationRule::BuildAllUsedAttributes " << GetName() << ", "
		     << derivedAttribute->GetParentClass()->GetName() << " " << derivedAttribute->GetName() << "\n";
		cout << "- target class: " << GetObjectClassName() << "\n";
		cout << "End KWDRRelationCreationRule::BuildAllUsedAttributes " << GetName() << ", "
		     << derivedAttribute->GetParentClass()->GetName() << " " << derivedAttribute->GetName() << "\n";
	}
}

void KWDRRelationCreationRule::CollectCreationRuleMandatoryInputOperands(
    const KWAttribute* derivedAttribute, const NumericKeyDictionary* nkdAllUsedAttributes,
    IntVector* ivMandatoryInputOperands) const
{
	boolean bTrace = false;
	IntVector ivUsedOutputOperands;
	KWClass* kwcTargetClass;
	KWAttribute* targetAttribute;
	KWDerivationRuleOperand* operand;
	int nOperand;

	require(GetOutputOperandNumber() > 0);
	require(IsCompiled());
	require(derivedAttribute != NULL);
	require(derivedAttribute->GetDerivationRule() == this);
	require(nkdAllUsedAttributes != NULL);
	require(ivMandatoryInputOperands != NULL);

	// Initialisation des vecteurs des operandes utilises en entree et en sortie
	ivMandatoryInputOperands->SetSize(GetOperandNumber());
	ivMandatoryInputOperands->Initialize();
	ivUsedOutputOperands.SetSize(GetOutputOperandNumber());

	// Recherche de la classe cible
	kwcTargetClass = derivedAttribute->GetClass();
	assert(kwcTargetClass != NULL);

	// Collecte des attributs en sortie
	// On optimise les impacts en n'exploitant que les attributs en sortie utilises
	// - on ne collecte pas les attributs cibles dans methode
	// - on se base en fait sur leur utilisation par d'autre regles
	for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
	{
		operand = GetOutputOperandAt(nOperand);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
		assert(KWType::IsData(operand->GetType()));

		// Recherche de l'attribut cible correspond a l'operande en sortie
		targetAttribute = kwcTargetClass->LookupAttribute(operand->GetAttributeName());
		assert(targetAttribute != NULL);

		// Memorisation de l'utilisation de l'operande
		if (nkdAllUsedAttributes->Lookup(targetAttribute) != NULL)
			ivUsedOutputOperands.SetAt(nOperand, 1);
	}

	// On en en deduit la sous-partie des operandes en entree a utiliser
	CollectUsedInputOperands(&ivUsedOutputOperands, ivMandatoryInputOperands);

	// Trace
	if (bTrace)
	{
		cout << "Begin KWDRRelationCreationRule::CollectMandatoryInputOperands " << GetName() << ", "
		     << derivedAttribute->GetParentClass()->GetName() << " " << derivedAttribute->GetName() << "\n";
		cout << "- target class: " << GetObjectClassName() << "\n";
		cout << "- used input operands: " << GetOperandNumber() << "\n";
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			cout << "  - " << nOperand + 1 << ": "
			     << BooleanToString(ivMandatoryInputOperands->GetAt(nOperand) == 1) << "\n";
		}
		cout << "- used output operands: " << GetOutputOperandNumber() << "\n";
		for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
		{
			cout << "  - " << nOperand + 1 << ": "
			     << BooleanToString(ivUsedOutputOperands.GetAt(nOperand) == 1) << "\n";
		}
	}
}

void KWDRRelationCreationRule::CollectCreationRuleAllAttributes(
    const KWAttribute* derivedAttribute, NumericKeyDictionary* nkdAllNonDeletableAttributes) const
{
	KWClass* kwcTargetClass;
	KWAttribute* targetAttribute;
	KWDerivationRuleOperand* operand;
	int nOperand;

	require(GetOutputOperandNumber() > 0);
	require(derivedAttribute != NULL);
	require(derivedAttribute->GetDerivationRule() == this);
	require(nkdAllNonDeletableAttributes != NULL);

	// Il faut exploiter tous les operandes en entree et en sortie, pour garder la coherence des classes,
	// au dela des optimisations avancees qui ont detecte les attributs effectivement utilises a calculer
	// Pour cela, on appel la methode ancetre pour les operandes en entree, pour tous les prendre
	// sans preoccupation d'optimisation
	// En effet, seules les regles de creation d'instances peuvent exploiter des attributs en sortie non utilises,
	// et les operandes en entree inutiles, car servant a alimenter ces attributs en sorties inutiles
	KWDerivationRule::BuildAllUsedAttributes(derivedAttribute, nkdAllNonDeletableAttributes);

	// Memorisation des attributs des operandes en sortie
	for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
	{
		operand = GetOutputOperandAt(nOperand);

		// Recherche de l'attribut cible correspond a l'operande en sortie
		targetAttribute = derivedAttribute->GetClass()->LookupAttribute(operand->GetAttributeName());
		assert(targetAttribute != NULL);
		nkdAllNonDeletableAttributes->SetAt(targetAttribute, targetAttribute);
	}
}

void KWDRRelationCreationRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	KWDerivationRuleOperand* operand;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::CopyFrom(kwdrSource);

	// Gestion si necessaire des operandes en sortie
	DeleteAllOutputOperands();
	bVariableOutputOperandNumber = false;
	if (not kwdrSource->GetReference())
	{
		// Duplication des operandes en sortie
		bVariableOutputOperandNumber = kwdrSource->GetVariableOutputOperandNumber();
		for (i = 0; i < kwdrSource->GetOutputOperandNumber(); i++)
		{
			operand = kwdrSource->GetOutputOperandAt(i);
			AddOutputOperand(operand->Clone());
		}
	}
}

int KWDRRelationCreationRule::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;
	int nOperand;
	KWDerivationRuleOperand* operand1;
	KWDerivationRuleOperand* operand2;

	// Appel de la methode ancetre
	nDiff = KWDerivationRule::FullCompare(rule);

	// En cas d'egalite, comparaison sur les operandes en sortie
	if (nDiff == 0 and KWType::IsRelation(rule->GetType()))
	{
		// Cas ou la regle a comparer est de type Reference, et n'a donc pas d'operandes en sortie
		if (rule->GetReference())
			nDiff = GetOutputOperandNumber();
		// Cas ou la regle a comparer  est de type Reference
		else
		{
			// Comparaison sur le nombre d'operandes en sortie
			nDiff = GetOutputOperandNumber() - rule->GetOutputOperandNumber();

			// En cas d'egalite, comparaison sur chaque operande en sortie
			if (nDiff == 0)
			{
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
							nDiff = operand1->GetObjectClassName().Compare(
							    operand2->GetObjectClassName());
						else if (operand1->GetType() == KWType::Structure)
							nDiff = operand1->GetStructureName().Compare(
							    operand2->GetStructureName());
					}

					// Comparaison sur l'origine de l'operande
					if (nDiff == 0)
						nDiff = operand1->GetOrigin() - operand2->GetOrigin();

					// Comparaison sur le nom de l'attribut
					// On ignore les comparaison complexe sur les autres cas, qui sont inutiles ici
					if (nDiff == 0 and
					    operand1->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
						nDiff =
						    operand1->GetAttributeName().Compare(operand2->GetAttributeName());
				}
			}
		}
	}
	return nDiff;
}

longint KWDRRelationCreationRule::GetUsedMemory() const
{
	longint lUsedMemory;
	longint lOperandUsedMemory;
	int i;
	KWDerivationRuleOperand* operand;

	// Appel de la methode ancetre
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRRelationCreationRule) - sizeof(KWDerivationRule);

	// Prise en compte des operandes
	lUsedMemory += oaOutputOperands.GetUsedMemory();
	for (i = 0; i < oaOutputOperands.GetSize(); i++)
	{
		operand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(i));
		lOperandUsedMemory = operand->GetUsedMemory();
		lUsedMemory += lOperandUsedMemory;
	}

	// Prise en compte des  information compilees
	lUsedMemory += livComputeModeTargetAttributeLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += ivComputeModeTargetAttributeTypes.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += livViewModeSourceAttributeLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += livViewModeTargetAttributeLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += ivViewModeTargetAttributeTypes.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += livViewModeSourceBlockLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += livViewModeTargetBlockLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += ivViewModeTargetBlockTypes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

longint KWDRRelationCreationRule::ComputeHashValue() const
{
	longint lHash;
	int nOperand;
	KWDerivationRuleOperand* operand;

	// Appel de la methode ancetre
	lHash = KWDerivationRule::ComputeHashValue();

	// Prise en compte des operandes
	for (nOperand = 0; nOperand < GetOutputOperandNumber(); nOperand++)
	{
		operand = GetOutputOperandAt(nOperand);
		lHash = LongintUpdateHashValue(lHash, operand->ComputeHashValue());
	}
	return lHash;
}

void KWDRRelationCreationRule::Write(ostream& ost) const
{
	KWDerivationRuleOperand* operand;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::Write(ost);

	// Operandes en sortie
	for (i = 0; i < GetOutputOperandNumber(); i++)
	{
		operand = GetOutputOperandAt(i);
		ost << "\t\t";
		operand->Write(ost);
		ost << "\n";
	}
}

void KWDRRelationCreationRule::WriteUsedRuleOperands(ostream& ost) const
{
	KWDerivationRuleOperand* operand;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::WriteUsedRuleOperands(ost);

	// Operandes en sortie
	if (GetOutputOperandNumber() > 0)
	{
		ost << " : ";
		for (i = 0; i < GetOutputOperandNumber(); i++)
		{
			operand = GetOutputOperandAt(i);
			if (i > 0)
				ost << ", ";
			operand->WriteUsedOperand(ost);
		}
	}
}

void KWDRRelationCreationRule::CollectUsedInputOperands(const IntVector* ivUsedOutputOperands,
							IntVector* ivUsedInputOperands) const
{
	int nOutputOperand;

	require(GetOutputOperandNumber() > 0);
	require(ivUsedOutputOperands != NULL);
	require(ivUsedOutputOperands->GetSize() == GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Collecte des operandes en entree obligatoires
	CollectMandatoryInputOperands(ivUsedInputOperands);

	// Collecte des operandes en entree specifiques par operande en sortie
	for (nOutputOperand = 0; nOutputOperand < GetOutputOperandNumber(); nOutputOperand++)
	{
		if (ivUsedOutputOperands->GetAt(nOutputOperand) == 1)
			CollectSpecificInputOperandsAt(nOutputOperand, ivUsedInputOperands);
	}
}

void KWDRRelationCreationRule::CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const
{
	int nInputOperand;

	require(GetOutputOperandNumber() > 0);
	require(GetOperandNumber() >= GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Par defaut, les operandes en entree du debut de liste sont obligatoires
	for (nInputOperand = 0; nInputOperand < GetOperandNumber() - GetOutputOperandNumber(); nInputOperand++)
		ivUsedInputOperands->SetAt(nInputOperand, 1);
}

void KWDRRelationCreationRule::CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const
{
	int nInputOperand;

	require(GetOutputOperandNumber() > 0);
	require(GetOperandNumber() >= GetOutputOperandNumber());
	require(0 <= nOutputOperand and nOutputOperand < GetOutputOperandNumber());
	require(ivUsedInputOperands != NULL);
	require(ivUsedInputOperands->GetSize() == GetOperandNumber());

	// Par defaut, on utilise l'operande en entree correspondant a l'operande en sortie
	nInputOperand = GetOperandNumber() - GetOutputOperandNumber() + nOutputOperand;
	ivUsedInputOperands->SetAt(nInputOperand, 1);
}

void KWDRRelationCreationRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
							NumericKeyDictionary* nkdCompletedAttributes)
{
	KWClass* kwcTargetClass;
	KWDerivationRuleOperand* operand;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Specialisation pour les operandes en sortie
	if (GetOutputOperandNumber() > 0)
	{
		// Recherche de la classe cible
		kwcTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());

		// Completion des operandes en sortie si classe en sortie trouvee
		if (kwcTargetClass != NULL)
		{
			for (i = 0; i < GetOutputOperandNumber(); i++)
			{
				operand = GetOutputOperandAt(i);

				// Completion de l'operande
				operand->InternalCompleteTypeInfo(kwcTargetClass, nkdCompletedAttributes);
			}
		}
	}
}

KWObject* KWDRRelationCreationRule::NewTargetObject(const KWObject* kwoOwnerObject,
						    const KWLoadIndex liAttributeLoadIndex) const
{
	const boolean bTrace = false;
	KWObject* kwoTargetObject;
	const KWObjectDataPath* objectDataPath;
	KWDatabaseMemoryGuard* memoryGuard;

	require(kwoOwnerObject != NULL);
	require(kwoOwnerObject->GetObjectDataPath() != NULL);
	require(IsCompiled());

	// Recherhe du data path de l'objet a creer
	objectDataPath = kwoOwnerObject->GetObjectDataPath()->GetComponentDataPath(liAttributeLoadIndex);

	// Acces au service de protection memoire
	memoryGuard = objectDataPath->GetMemoryGuard();

	// Prise en compte de la tentative dans le memory guard
	memoryGuard->AddCreatedRecord();

	// Pas de tentative de creation si la limite est deja ateinte
	if (memoryGuard->IsMemoryLimitReached())
		kwoTargetObject = NULL;
	// Sinon, on tente de creer l'objet
	else
	{

		// Creation d'un objet en mode vue, avec un index de creation fourni par le data path
		kwoTargetObject = new KWObject(kwcCompiledTargetClass, objectDataPath->NewCreationIndex());
		kwoTargetObject->SetViewTypeUse(true);
		kwoTargetObject->SetObjectDataPath(objectDataPath);

		// Nettoyage si le memory guard a detecte un depassement de limite memoire
		if (memoryGuard->IsMemoryLimitReached())
		{
			delete kwoTargetObject;
			kwoTargetObject = NULL;
		}
	}

	// Trace
	if (bTrace)
	{
		cout << "NewTargetObject\t" << GetName() << "\t";
		cout << memoryGuard->GetTotalCreatedRecordNumber() << "\t";
		cout << kwoOwnerObject->GetObjectDataPath()->GetDataPath() << "\t"
		     << kwoOwnerObject->GetClass()->GetName() << "\t" << kwoOwnerObject->GetCreationIndex() << "\t";
		if (kwoOwnerObject->GetClass()->GetKeyAttributeNumber() > 0)
			cout << kwoOwnerObject->GetObjectLabel() << "\t";
		if (kwoTargetObject != NULL)
		{
			cout << kwoTargetObject->GetObjectDataPath()->GetDataPath() << "\t"
			     << kwoTargetObject->GetClass()->GetName() << "\t" << kwoTargetObject->GetCreationIndex();
		}
		if (memoryGuard->IsMemoryLimitReached())
			cout << "\tMEMORY LIMIT";
		cout << "\n";
	}
	ensure(kwoTargetObject != NULL or memoryGuard->IsMemoryLimitReached());
	return kwoTargetObject;
}

void KWDRRelationCreationRule::AddViewModeError(const KWClass* kwcSourceClass, const KWClass* kwcTargetClass,
						const KWAttribute* targetAttribute, const ALString& sLabel) const
{
	require(kwcSourceClass != NULL);
	require(kwcTargetClass != NULL);
	require(targetAttribute != NULL);
	require(IsValidOutputOperandType(targetAttribute->GetType()));
	require(kwcTargetClass->LookupAttribute(targetAttribute->GetName()) == targetAttribute);

	// Message d'erreur, avec partie generique
	AddError("In the " + kwcTargetClass->GetName() + " output dictionary, the " + targetAttribute->GetTypeLabel() +
		 " variable " + targetAttribute->GetName() + " " + sLabel + " in the " + kwcSourceClass->GetName() +
		 " input dictionary of the first operand" + " of the rule");
}

boolean KWDRRelationCreationRule::IsViewModeActivated() const
{
	return true;
}

void KWDRRelationCreationRule::FillViewModeTargetAttributes(const KWObject* kwoSourceObject,
							    KWObject* kwoTargetObject) const
{
	ObjectArray* oaSourceObjectArray;
	ObjectArray* oaTargetObjectArray;
	int nAttribute;
	int nBlock;
	int nType;
	KWLoadIndex liSource;
	KWLoadIndex liTarget;
	KWDerivationRule* valueBlockRule;
	KWContinuousValueBlock* targetContinuousValueBlock;
	KWSymbolValueBlock* targetSymbolValueBlock;
	KWAttributeBlock* targetAttributeBlock;

	require(IsCompiled());
	require(kwoSourceObject != NULL);

	// Retour si objet cible NULL
	if (kwoTargetObject == NULL)
		return;

	// Alimentation des attributs de l'objet cible avec les valeurs provenant de l'objet source
	for (nAttribute = 0; nAttribute < ivViewModeTargetAttributeTypes.GetSize(); nAttribute++)
	{
		// Acces aux informations issues de la compilation
		nType = ivViewModeTargetAttributeTypes.GetAt(nAttribute);
		liSource = livViewModeSourceAttributeLoadIndexes.GetAt(nAttribute);
		liTarget = livViewModeTargetAttributeLoadIndexes.GetAt(nAttribute);

		// Recopie de la valeur selon le type
		switch (nType)
		{
		case KWType::Symbol:
			kwoTargetObject->SetSymbolValueAt(liTarget, kwoSourceObject->ComputeSymbolValueAt(liSource));
			break;
		case KWType::Continuous:
			kwoTargetObject->SetContinuousValueAt(liTarget,
							      kwoSourceObject->ComputeContinuousValueAt(liSource));
			break;
		case KWType::Date:
			kwoTargetObject->SetDateValueAt(liTarget, kwoSourceObject->ComputeDateValueAt(liSource));
			break;
		case KWType::Time:
			kwoTargetObject->SetTimeValueAt(liTarget, kwoSourceObject->ComputeTimeValueAt(liSource));
			break;
		case KWType::Timestamp:
			kwoTargetObject->SetTimestampValueAt(liTarget,
							     kwoSourceObject->ComputeTimestampValueAt(liSource));
			break;
		case KWType::TimestampTZ:
			kwoTargetObject->SetTimestampTZValueAt(liTarget,
							       kwoSourceObject->ComputeTimestampTZValueAt(liSource));
			break;
		case KWType::Text:
			kwoTargetObject->SetTextValueAt(liTarget, kwoSourceObject->ComputeTextValueAt(liSource));
			break;
		case KWType::Object:
			kwoTargetObject->SetObjectValueAt(liTarget, kwoSourceObject->ComputeObjectValueAt(liSource));
			break;
		case KWType::ObjectArray:
			oaSourceObjectArray = kwoSourceObject->ComputeObjectArrayValueAt(liSource);
			oaTargetObjectArray = NULL;
			if (oaSourceObjectArray != NULL)
				oaTargetObjectArray = oaSourceObjectArray->Clone();
			kwoTargetObject->SetObjectArrayValueAt(liTarget, oaTargetObjectArray);
			break;
		default:
			assert(false);
			break;
		}
	}

	// Alimentation des blocs d'attributs de l'objet cible avec les valeurs provenant de l'objet source
	for (nBlock = 0; nBlock < ivViewModeTargetBlockTypes.GetSize(); nBlock++)
	{
		// Acces aux informations issues de la compilation
		nType = ivViewModeTargetBlockTypes.GetAt(nBlock);
		liSource = livViewModeSourceBlockLoadIndexes.GetAt(nBlock);
		liTarget = livViewModeTargetBlockLoadIndexes.GetAt(nBlock);
		valueBlockRule = cast(KWDerivationRule*, oaViewModeCopyBlockRules.GetAt(nBlock));

		// Acces au bloc cible
		targetAttributeBlock = kwoTargetObject->GetClass()->GetAttributeBlockAtLoadIndex(liTarget);

		// Recopie de la valeur selon le type
		switch (nType)
		{
		case KWType::Symbol:
			// Calcul et memorisation de la valeur du bloc cible
			targetSymbolValueBlock = valueBlockRule->ComputeSymbolValueBlockResult(
			    kwoSourceObject, targetAttributeBlock->GetLoadedAttributesIndexedKeyBlock());
			kwoTargetObject->SetSymbolValueBlockAt(liTarget, targetSymbolValueBlock);
			break;
		case KWType::Continuous:
			// Calcul et memorisation de la valeur du bloc cible
			targetContinuousValueBlock = valueBlockRule->ComputeContinuousValueBlockResult(
			    kwoSourceObject, targetAttributeBlock->GetLoadedAttributesIndexedKeyBlock());
			kwoTargetObject->SetContinuousValueBlockAt(liTarget, targetContinuousValueBlock);
			break;
		default:
			assert(false);
			break;
		}
	}
}

void KWDRRelationCreationRule::FillComputeModeTargetAttributesForVariableOperandNumber(const KWObject* kwoSourceObject,
										       KWObject* kwoTargetObject) const
{
	ObjectArray* oaSourceObjectArray;
	ObjectArray* oaTargetObjectArray;
	int nAttribute;
	int nType;
	int nStartInputOperandIndex;
	int nInputOperandIndex;
	KWDerivationRuleOperand* operand;
	KWLoadIndex liTarget;

	require(IsCompiled());
	require(kwoSourceObject != NULL);
	require(GetOperandNumber() >= GetOutputOperandNumber());
	require(GetOutputOperandNumber() == ivComputeModeTargetAttributeTypes.GetSize());
	require(GetVariableOperandNumber());
	require(GetVariableOutputOperandNumber());

	// Retour si objet cible NULL
	if (kwoTargetObject == NULL)
		return;

	// Recherche de l'index du premier operande en entree correspondant
	// aux valeurs servant a alimenter les attributs en sortie
	// En effet, les operande en sortie sont alimentes par les derniers operandes en entree
	nStartInputOperandIndex = GetOperandNumber() - GetOutputOperandNumber();

	// Alimentation des attributs de l'objet cible avec les valeurs provenant des operandes en sortie
	for (nAttribute = 0; nAttribute < ivComputeModeTargetAttributeTypes.GetSize(); nAttribute++)
	{
		// Recherche de l'operande source correspondant a l'operande cible
		nInputOperandIndex = nStartInputOperandIndex + nAttribute;
		operand = GetOperandAt(nInputOperandIndex);

		// On ignore l'operande en entree s'il a la valeur speciale None
		if (operand->GetNoneValue())
			continue;

		// Index de chargement cible
		liTarget = livComputeModeTargetAttributeLoadIndexes.GetAt(nAttribute);
		assert(liTarget.IsValid());
		assert(kwoTargetObject->GetClass()
			   ->LookupAttribute(GetOutputOperandAt(nAttribute)->GetAttributeName())
			   ->GetLoaded());

		// Recopie de la valeur selon le type
		nType = ivComputeModeTargetAttributeTypes.GetAt(nAttribute);
		assert(ivComputeModeTargetAttributeTypes.GetAt(nAttribute) == operand->GetType());
		switch (nType)
		{
		case KWType::Symbol:
			kwoTargetObject->SetSymbolValueAt(liTarget, operand->GetSymbolValue(kwoSourceObject));
			break;
		case KWType::Continuous:
			kwoTargetObject->SetContinuousValueAt(liTarget, operand->GetContinuousValue(kwoSourceObject));
			break;
		case KWType::Date:
			kwoTargetObject->SetDateValueAt(liTarget, operand->GetDateValue(kwoSourceObject));
			break;
		case KWType::Time:
			kwoTargetObject->SetTimeValueAt(liTarget, operand->GetTimeValue(kwoSourceObject));
			break;
		case KWType::Timestamp:
			kwoTargetObject->SetTimestampValueAt(liTarget, operand->GetTimestampValue(kwoSourceObject));
			break;
		case KWType::TimestampTZ:
			kwoTargetObject->SetTimestampTZValueAt(liTarget, operand->GetTimestampTZValue(kwoSourceObject));
			break;
		case KWType::Text:
			kwoTargetObject->SetTextValueAt(liTarget, operand->GetTextValue(kwoSourceObject));
			break;
		case KWType::Object:
			kwoTargetObject->SetObjectValueAt(liTarget, operand->GetObjectValue(kwoSourceObject));
			break;
		case KWType::ObjectArray:
			oaSourceObjectArray = operand->GetObjectArrayValue(kwoSourceObject);
			oaTargetObjectArray = NULL;
			if (oaSourceObjectArray != NULL)
				oaTargetObjectArray = oaSourceObjectArray->Clone();
			kwoTargetObject->SetObjectArrayValueAt(liTarget, oaTargetObjectArray);
			break;
		default:
			assert(false);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCreationRule

KWDRTableCreationRule::KWDRTableCreationRule()
{
	SetType(KWType::ObjectArray);
}

KWDRTableCreationRule::~KWDRTableCreationRule() {}
