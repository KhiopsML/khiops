// Copyright (c) 2024 Orange. All rights reserved.
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
	KWDerivationRuleOperand* operand;
	int i;
	ALString sOperandOrigin;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsDefinition();

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
					 " that must be used with an origin Variable (origin " + sOperandOrigin +
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
			if (bOk and not KWType::IsData(operand->GetType()) and operand->GetType() != KWType::Unknown)
			{
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1) +
					 " that cannot be specified with type " + KWType::ToString(operand->GetType()));
				bOk = false;
			}

			// Seul le dernier operande, en cas de nombre variables d'operandes, a le droit d'etre de type Unknown
			if (bOk and (i < oaOutputOperands.GetSize() - 1 or not GetVariableOutputOperandNumber()) and
			    operand->GetType() == KWType::Unknown)
			{
				AddError(sTmp + "Incorrect output operand " + IntToString(i + 1) +
					 " that cannot be specified with type " + KWType::ToString(operand->GetType()));
				bOk = false;
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
	int i;
	ALString sTmp;

	require(not ruleFamily->GetReference());

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsFamily(ruleFamily);

	// Specialisation pour les operandes en sortie
	if (bOk)
	{
		// Verification des operandes en sortie
		for (i = 0; i < oaOutputOperands.GetSize(); i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(i));

			// Cas du nombre variable d'operandes utilise effectivement
			if (i >= ruleFamily->GetOutputOperandNumber())
			{
				assert(ruleFamily->GetVariableOutputOperandNumber());
				if (not operand->CheckFamily(
					ruleFamily->GetOperandAt(ruleFamily->GetOutputOperandNumber() - 1)))
				{
					AddError(sTmp + "Output operand " + IntToString(i + 1) +
						 " inconsistent with that of the registered rule");
					bOk = false;
				}
			}
			// Cas du nombre constant d'operandes (ou nombre variable sous-utilise)
			else
			{
				if (not operand->CheckFamily(ruleFamily->GetOutputOperandAt(i)))
				{
					AddError(sTmp + "Output operand " + IntToString(i + 1) +
						 " inconsistent with that of the registered rule");
					bOk = false;
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
	ObjectDictionary odOutputAttributeNames;
	KWDerivationRuleOperand* operand;
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

		// Gestion de l'alimentation de type calcul via les operandes en sortie
		if (bOk and GetOutputOperandNumber() > 0)
		{
			// Collecte des nom des attribut en sortie en verifiant leur unicite
			for (i = 0; i < GetOutputOperandNumber(); i++)
			{
				operand = GetOutputOperandAt(i);
				assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
				assert(KWType::IsData(operand->GetType()));

				// Verification de l'operande en sortie
				if (not operand->CheckCompleteness(kwcTargetClass))
				{
					AddError(sTmp + "Incomplete output operand " + IntToString(i + 1) +
						 " for output dictionary " + kwcTargetClass->GetName());
					bOk = false;
				}

				// Verification de l'unicite des noms d'attributs des operandes en sortie
				if (odOutputAttributeNames.Lookup(operand->GetAttributeName()) == NULL)
					odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);
				else
				{
					AddError(sTmp + "Output operand " + IntToString(i + 1) +
						 " with variable name already used in output dictionary " +
						 kwcTargetClass->GetName());
					bOk = false;
					break;
				}
			}
		}

		// Validation des attributs natifs de la classe cibles
		if (bOk)
		{
			// Recherche de la classe source dans le cas d'une alimentttion de type vue
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
					assert(KWType::IsData(targetAttribute->GetType()));

					// On verifie attribut natifs non deja alimente par les operandes en sortie
					if (odOutputAttributeNames.Lookup(targetAttribute->GetName()) == NULL)
					{
						// Recherche d'un attribut natif source de meme nom dans le cas d'une alimentation de type vue
						sourceAttribute = NULL;
						if (not IsViewModeActivated())
						{
							AddError("In output dictionary " + kwcTargetClass->GetName() +
								 ", variable " + targetAttribute->GetName() +
								 " is not set by any output operand");
							bOk = false;
						}
						// Cas d'une alimenattion de type vue
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
								AddError("In output dictionary " +
									 kwcTargetClass->GetName() + ", variable " +
									 targetAttribute->GetName() +
									 " must exist in input dictionary " +
									 kwcSourceClass->GetName() +
									 " of first rule operand");
								bOk = false;
							}
							// Erreur si le type trouve est incorrect
							else if (sourceAttribute->GetType() !=
								 targetAttribute->GetType())
							{
								AddError("In output dictionary " +
									 kwcTargetClass->GetName() + ", " +
									 KWType::ToString(targetAttribute->GetType()) +
									 " variable " + targetAttribute->GetName() +
									 " is found with a different type (" +
									 KWType::ToString(sourceAttribute->GetType()) +
									 ") in input dictionary " +
									 kwcSourceClass->GetName() +
									 " of first rule operand");
								bOk = false;
							}

							// Erreur si le type de relation est incorrect
							if (bOk and KWType::IsRelation(sourceAttribute->GetType()) and
							    sourceAttribute->GetClass()->GetName() !=
								targetAttribute->GetClass()->GetName())
							{
								AddError("In output dictionary " +
									 kwcTargetClass->GetName() + ", " +
									 KWType::ToString(targetAttribute->GetType()) +
									 "(" + targetAttribute->GetClass()->GetName() +
									 ") variable " + targetAttribute->GetName() +
									 " is found with a different type (" +
									 KWType::ToString(sourceAttribute->GetType()) +
									 "(" + sourceAttribute->GetClass()->GetName() +
									 ")) in input dictionary " +
									 kwcSourceClass->GetName() +
									 " of first rule operand");
								bOk = false;
							}

							// Erreur dans le cas d'un bloc si bloc different ou si VarKey different
							if (bOk and (sourceAttribute->IsInBlock() or
								     targetAttribute->IsInBlock()))
							{
								//DDD Pour l'instant, on interdit les attributs cibles dans un bloc
								if (targetAttribute->IsInBlock())
								{
									AddError(
									    "In output dictionary " +
									    kwcTargetClass->GetName() + ", variable " +
									    targetAttribute->GetName() + " in block " +
									    targetAttribute->GetAttributeBlock()
										->GetName() +
									    " not allowed");
									bOk = false;
								}
								//DDD EN COURS
								// Erreur si nom de bloc ifferent

								// Erreur si type de VarKey different

								// Erreur si VarKey different
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
	boolean bOk;

	// Appel de la methode ancetre
	// Pas de specialisation necessaire actuellement, puisque les operandes en sortie n'impliques pas de regles
	bOk = KWDerivationRule::ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);
	return bOk;
}

void KWDRRelationCreationRule::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;
	ObjectDictionary odOutputAttributeNames;
	KWClass* kwcSourceClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	int nAttribute;
	KWDerivationRuleOperand* operand;
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
			assert(KWType::IsData(operand->GetType()));

			// Memorisation du nom de l'attribut en sortie
			odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);

			// Recherche de l'attribut cible
			targetAttribute = kwcCompiledTargetClass->LookupAttribute(operand->GetAttributeName());
			assert(targetAttribute != NULL);
			assert(KWType::IsData(targetAttribute->GetType()));
			assert(targetAttribute->GetLoaded());

			// Memorisation des infos de chargement
			livComputeModeTargetAttributeLoadIndexes.Add(targetAttribute->GetLoadIndex());
			ivComputeModeTargetAttributeTypes.Add(targetAttribute->GetType());

			// Trace par attribut gere par une alimentation de type vue
			if (bTrace)
			{
				cout << "\t" << i + 1;
				cout << "\t" << targetAttribute->GetName();
				cout << "\t" << KWType::ToString(targetAttribute->GetType());
				cout << "\t" << targetAttribute->GetLoadIndex();
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
				assert(KWType::IsStored(targetAttribute->GetType()) or
				       KWType::IsRelation(targetAttribute->GetType()));

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
	}
}

void KWDRRelationCreationRule::BuildAllUsedOperands(NumericKeyDictionary* nkdAllUsedOperands) const
{
	// Appel de la methode ancetre
	// Pas de specialisation necessaire actuellement, puisque les operandes en sortie n'impliques pas de regles
	KWDerivationRule::BuildAllUsedOperands(nkdAllUsedOperands);
}

void KWDRRelationCreationRule::BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
						      NumericKeyDictionary* nkdAllUsedAttributes) const
{
	ObjectDictionary odOutputAttributeNames;
	KWClass* kwcSourceClass;
	KWClass* kwcTargetClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	KWDerivationRule* sourceAttributeRule;
	KWDerivationRuleOperand* operand;
	int i;

	require(IsCompiled());
	require(derivedAttribute != NULL);
	require(KWType::IsRelation(derivedAttribute->GetType()));
	require(derivedAttribute->GetClass()->GetName() == GetObjectClassName());

	// Appel de la methode ancetre
	KWDerivationRule::BuildAllUsedAttributes(derivedAttribute, nkdAllUsedAttributes);

	// Recherche des attribut cible utilises dans le cas d'une alimentation de type calcul
	if (GetOutputOperandNumber() > 0)
	{
		// Collecte des nom des attribut en sortie en verifiant leur unicite
		for (i = 0; i < GetOutputOperandNumber(); i++)
		{
			operand = GetOutputOperandAt(i);
			assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
			assert(KWType::IsData(operand->GetType()));

			// Memorisation du nom de l'attribut en sortie
			odOutputAttributeNames.SetAt(operand->GetAttributeName(), operand);
		}
	}
	// Recherche des attribut cible utilises dans le cas d'une alimentation de type vue
	if (IsViewModeActivated())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche des classes source et cible
		kwcSourceClass = derivedAttribute->GetParentClass()->GetDomain()->LookupClass(
		    GetFirstOperand()->GetObjectClassName());
		kwcTargetClass = derivedAttribute->GetClass();
		assert(kwcSourceClass != NULL);
		assert(kwcTargetClass != NULL);

		// Parcours des attributs natifs de la classe cible
		targetAttribute = kwcTargetClass->GetHeadAttribute();
		while (targetAttribute != NULL)
		{
			// On ne traite que les attributs natifs utilises non deja prise en compte
			// par une alimentation de type calcul
			//DDD if (targetAttribute->GetUsed() and targetAttribute->GetDerivationRule() == NULL)
			if (targetAttribute->GetDerivationRule() == NULL and
			    odOutputAttributeNames.Lookup(targetAttribute->GetName()) == NULL)
			{
				assert(KWType::IsData(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());
				assert(sourceAttribute != NULL);
				assert(sourceAttribute->GetType() == targetAttribute->GetType());

				// Analyse de l'attribut si necessaire
				if (nkdAllUsedAttributes->Lookup(sourceAttribute) == NULL)
				{
					// Memorisation de l'attribut dans le dictionnaire
					nkdAllUsedAttributes->SetAt(sourceAttribute, sourceAttribute);

					// Acces a la regle d'attribut ou de bloc
					sourceAttributeRule = sourceAttribute->GetAnyDerivationRule();
					if (sourceAttributeRule != NULL)
						sourceAttributeRule->BuildAllUsedAttributes(sourceAttribute,
											    nkdAllUsedAttributes);
				}
			}

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(targetAttribute);
		}
	}
}

void KWDRRelationCreationRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	KWDerivationRuleOperand* operand;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::CopyFrom(kwdrSource);

	// Gestion si necerssaire des operandes en sortie
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

boolean KWDRRelationCreationRule::IsViewModeActivated() const
{
	return true;
}

void KWDRRelationCreationRule::CopyObjectCommonNativeAttributes(const KWObject* kwoSourceObject,
								KWObject* kwoTargetObject) const
{
	ObjectArray* oaSourceObjectArray;
	ObjectArray* oaTargetObjectArray;
	int nAttribute;
	int nType;
	KWLoadIndex liSource;
	KWLoadIndex liTarget;

	require(IsCompiled());
	require(kwoSourceObject != NULL);
	require(kwoTargetObject != NULL);

	// Recopie des valeurs des attributs commun
	for (nAttribute = 0; nAttribute < ivViewModeTargetAttributeTypes.GetSize(); nAttribute++)
	{
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
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRelationCreationRule

KWDRTableCreationRule::KWDRTableCreationRule()
{
	SetType(KWType::ObjectArray);
}

KWDRTableCreationRule::~KWDRTableCreationRule() {}
