// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeBlock.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KWAttributeBlock

KWAttributeBlock::KWAttributeBlock()
{
	firstAttribute = NULL;
	lastAttribute = NULL;
	kwdrRule = NULL;
	cBlockDefaultValue = 0;
	bUsed = false;
	bLoaded = false;
	nAttributeNumber = 0;
	loadedAttributesIndexedKeyBlock = NULL;
	ivLoadedAttributeMutationIndexes = NULL;
}

KWAttributeBlock::~KWAttributeBlock()
{
	if (kwdrRule != NULL)
		delete kwdrRule;
	if (loadedAttributesIndexedKeyBlock != NULL)
		delete loadedAttributesIndexedKeyBlock;
	if (ivLoadedAttributeMutationIndexes != NULL)
		delete ivLoadedAttributeMutationIndexes;
}

const ALString& KWAttributeBlock::GetAttributeKeyMetaDataKey()
{
	static const ALString sAttributeKeyMetaDataKey = "VarKey";
	return sAttributeKeyMetaDataKey;
}

const ALString& KWAttributeBlock::GetDefaultValueMetaDataKey(int nType)
{
	static const ALString sDefaultValueSymbolMetaDataKey = "DefaultCategoricalValue";
	static const ALString sDefaultValueContinuousMetaDataKey = "DefaultNumericalValue";

	require(KWType::IsSimple(nType));
	if (nType == KWType::Symbol)
		return sDefaultValueSymbolMetaDataKey;
	else
		return sDefaultValueContinuousMetaDataKey;
}

void KWAttributeBlock::ImportMetaDataFrom(const KWAttributeBlock* sourceAttributeBlock)
{
	require(sourceAttributeBlock != NULL);
	require(GetDerivationRule() == NULL);
	require(GetBlockType() == sourceAttributeBlock->GetBlockType());

	// Import standard des meta-donnees
	GetMetaData()->CopyFrom(sourceAttributeBlock->GetConstMetaData());

	// Rajout si necessaire d'une meta-data pour memoriser la valeur par defaut associee au bloc
	if (sourceAttributeBlock->GetDerivationRule() != NULL)
	{
		if (sourceAttributeBlock->GetType() == KWType::Symbol)
		{
			if (sourceAttributeBlock->GetDerivationRule()->GetValueBlockSymbolDefaultValue() !=
			    KWSymbolValueBlock::GetDefaultDefaultValue())
			{
				GetMetaData()->SetStringValueAt(
				    GetDefaultValueMetaDataKey(sourceAttributeBlock->GetType()),
				    sourceAttributeBlock->GetDerivationRule()
					->GetValueBlockSymbolDefaultValue()
					.GetValue());
			}
		}
		else if (sourceAttributeBlock->GetType() == KWType::Continuous)
		{
			if (sourceAttributeBlock->GetDerivationRule()->GetValueBlockContinuousDefaultValue() !=
			    KWContinuousValueBlock::GetDefaultDefaultValue())
			{
				assert(
				    sourceAttributeBlock->GetDerivationRule()->GetValueBlockContinuousDefaultValue() !=
				    KWContinuous::GetMissingValue());
				GetMetaData()->SetDoubleValueAt(
				    GetDefaultValueMetaDataKey(sourceAttributeBlock->GetType()),
				    sourceAttributeBlock->GetDerivationRule()->GetValueBlockContinuousDefaultValue());
			}
		}
	}
}

int KWAttributeBlock::ComputeAttributeNumber() const
{
	int nNumber;
	KWAttribute* attribute;

	require(GetParentClass() != NULL);
	require(GetFirstAttribute() != NULL);
	require(GetLastAttribute() != NULL);

	// Comptage par parcours des attributs du bloc
	nNumber = 0;
	attribute = GetFirstAttribute();
	while (attribute != NULL)
	{
		nNumber++;
		// Traitemet sur l'attribut en cours

		// Arret si fin du bloc
		if (attribute == GetLastAttribute())
			break;
		GetParentClass()->GetNextAttribute(attribute);
	}
	return nNumber;
}

boolean KWAttributeBlock::Check() const
{
	boolean bOk = true;
	ALString sDefaultValueMetaDataKey;
	KWClass* parentClass;
	KWDerivationRule* kwdrReference;
	KWAttribute* attribute;
	ObjectDictionary odVariableKeys;
	ALString sAttributeKeyMetaDataKey;
	ALString sAttributeKey;
	double dAttributeKey;
	int nAttributeKey;

	// Nom
	if (not KWClass::CheckName(GetName(), KWClass::AttributeBlock, this))
		bOk = false;

	// Verification du Label
	if (not KWClass::CheckLabel(GetLabel(), KWClass::AttributeBlock, this))
		bOk = false;

	// Verification des commentaires
	if (not KWClass::CheckComments(GetComments(), KWClass::AttributeBlock, this))
		bOk = false;

	// Verification des commentaires internes
	if (not KWClass::CheckComments(GetInternalComments(), KWClass::AttributeBlock, this))
		bOk = false;

	// Tests de base sur la specification du block
	if (bOk and firstAttribute == NULL)
	{
		AddError("Missing first variable in the block");
		bOk = false;
	}
	if (bOk and lastAttribute == NULL)
	{
		AddError("Missing last variable in the block");
		bOk = false;
	}
	if (bOk and firstAttribute->GetParentClass() == NULL)
	{
		AddError("Missing dictionary for first variable in the block");
		bOk = false;
	}
	if (bOk and lastAttribute->GetParentClass() == NULL)
	{
		AddError("Missing dictionary for last variable in the block");
		bOk = false;
	}
	if (bOk and firstAttribute->GetParentClass() != lastAttribute->GetParentClass())
	{
		AddError("Inconsistent dictionary between first and last variable in the block");
		bOk = false;
	}
	assert(bOk or GetParentClass()->LookupAttribute(firstAttribute->GetName()) == firstAttribute);
	assert(bOk or GetParentClass()->LookupAttribute(lastAttribute->GetName()) == lastAttribute);

	// Verification du type du premier attribut
	if (bOk and KWType::GetValueBlockType(firstAttribute->GetType()) == KWType::Unknown)
	{
		AddError("First variable in the block should be of type " + KWType::ToString(KWType::Symbol) + ", " +
			 KWType::ToString(KWType::Continuous) + " or " + KWType::ToString(KWType::ObjectArray));
		bOk = false;
	}

	// Verification des valeurs par default (pas necessaire pour les ObjectArrayValueBlock)
	if (bOk)
	{
		// Verification de la valeur par defaut du ContinuousValueBlock
		sDefaultValueMetaDataKey = GetDefaultValueMetaDataKey(KWType::Continuous);
		if (metaData.IsKeyPresent(sDefaultValueMetaDataKey))
		{
			if (GetType() != KWType::Continuous)
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should not be specified for a numerical sparse variable block");
			}
			else if (not metaData.IsDoubleTypeAt(sDefaultValueMetaDataKey))
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should be of numerical type");
			}
			else if (GetDerivationRule() != NULL)
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should not be specified for a derived sparse variable block");
			}
		}

		// Verification de la valeur par defaut du SymbolValueBlock
		sDefaultValueMetaDataKey = GetDefaultValueMetaDataKey(KWType::Symbol);
		if (metaData.IsKeyPresent(sDefaultValueMetaDataKey))
		{
			if (GetType() != KWType::Symbol)
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should not be specified for a categorical sparse variable block");
			}
			else if (not metaData.IsStringTypeAt(sDefaultValueMetaDataKey))
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should be of categorical type");
			}
			else if (GetDerivationRule() != NULL)
			{
				bOk = false;
				AddError("Meta-data " + sDefaultValueMetaDataKey + " (" +
					 metaData.GetExternalValueAt(sDefaultValueMetaDataKey) +
					 ") should not be specified for a derived sparse variable block");
			}
		}
	}

	// Verification de tous les attributs du bloc
	// Meme type, pas de regle de derivation, et meta-data VarKey tous differents
	if (bOk)
	{
		// Parcours des attributs du bloc
		sAttributeKeyMetaDataKey = GetAttributeKeyMetaDataKey();
		parentClass = GetParentClass();
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			// Erreur si attribut du mauvais type
			if (bOk and attribute->GetType() != firstAttribute->GetType())
			{
				AddError("Type of variable " + attribute->GetName() + " (" +
					 KWType::ToString(attribute->GetType()) + ") should be (" +
					 KWType::ToString(firstAttribute->GetType()) +
					 "), like for the first variable in the block");
				bOk = false;
			}

			// Erreur si type ObjectArray, mais dictionnaire different
			if (bOk and firstAttribute->GetType() == KWType::ObjectArray and
			    attribute->GetClass() != NULL and firstAttribute->GetClass() != NULL)
			{
				if (attribute->GetClass()->GetName() != firstAttribute->GetClass()->GetName())
				{
					AddError("Type of variable " + attribute->GetName() + " (" +
						 KWType::ToString(attribute->GetType()) + "(" +
						 attribute->GetClass()->GetName() + ")" + ") should be (" +
						 KWType::ToString(firstAttribute->GetType()) + "(" +
						 firstAttribute->GetClass()->GetName() + ")" +
						 "), like for the first variable in the block");
					bOk = false;
				}
			}

			// Erreur si regle de derivation attachee a l'attribut
			if (bOk and attribute->GetDerivationRule() != NULL)
			{
				AddError("Variable " + attribute->GetName() +
					 " in the block should not have a derivation rule");
				bOk = false;
			}

			// Erreur si probleme de cle de variable
			if (bOk)
			{
				assert(not attribute->GetConstMetaData()->IsKeyPresent(sAttributeKeyMetaDataKey) or
				       GetVarKeyType() != KWType::None);

				// Test de presence de la meta-donnee
				if (not attribute->GetConstMetaData()->IsKeyPresent(sAttributeKeyMetaDataKey))
				{
					AddError("Meta-data " + sAttributeKeyMetaDataKey +
						 " should be specified for variable " + attribute->GetName() +
						 " in the block");
					bOk = false;
				}
				// Test de la meta-donnee dans le cas de cles categorielles
				else if (GetVarKeyType() == KWType::Symbol)
				{
					// Test du type de la meta-donnee
					if (not attribute->GetConstMetaData()->IsStringTypeAt(sAttributeKeyMetaDataKey))
					{
						AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
							 attribute->GetConstMetaData()->GetExternalValueAt(
							     sAttributeKeyMetaDataKey) +
							 ") should be of string type for variable " +
							 attribute->GetName() + " in the block");
						bOk = false;
					}
					// Test de la validite et l'unicite de la valeur de la meta-donne dans le bloc
					// de variables
					else
					{
						sAttributeKey = attribute->GetConstMetaData()->GetStringValueAt(
						    sAttributeKeyMetaDataKey);
						if (sAttributeKey == "")
						{
							AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
								 attribute->GetConstMetaData()->GetExternalValueAt(
								     sAttributeKeyMetaDataKey) +
								 ") should not be empty for variable " +
								 attribute->GetName() + " in the block");
							bOk = false;
						}
					}
				}
				// Test de la meta-donnee dans le cas de cles categorielles
				else if (GetVarKeyType() == KWType::Continuous)
				{
					// Test du type de la meta-donnee
					if (not attribute->GetConstMetaData()->IsDoubleTypeAt(sAttributeKeyMetaDataKey))
					{
						AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
							 attribute->GetConstMetaData()->GetExternalValueAt(
							     sAttributeKeyMetaDataKey) +
							 ") should be of integer type for variable " +
							 attribute->GetName() + " in the block");
						bOk = false;
					}
					// Test de la validite et l'unicite de la valeur de la meta-donne dans le bloc
					// de variables
					else
					{
						// Conversion en entier
						dAttributeKey = attribute->GetConstMetaData()->GetDoubleValueAt(
						    sAttributeKeyMetaDataKey);
						bOk = KWContinuous::ContinuousToInt(dAttributeKey, nAttributeKey);

						// On memorise egalement sous forme chaine de caracteres, pour les test
						// d'unicite et pour les messages utilisant la valeur de cle entiere
						// (sinon, on peut avoir des message de type "Meta(Data VaraKey
						// (1.0e+09) ..."
						sAttributeKey = IntToString(nAttributeKey);

						// Test de la validite du format
						if (not bOk)
						{
							AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
								 attribute->GetConstMetaData()->GetExternalValueAt(
								     sAttributeKeyMetaDataKey) +
								 ") should be an integer for variable " +
								 attribute->GetName() + " in the block");
							assert(bOk == false);
						}
						// Test de valeur min
						else if (nAttributeKey < KWIndexedNKeyBlock::GetMinKey())
						{
							AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
								 sAttributeKey + ") should be greater or equal than " +
								 IntToString(KWIndexedNKeyBlock::GetMinKey()) +
								 " for variable " + attribute->GetName() +
								 " in the block");
							bOk = false;
						}
						// Test de valeur max
						else if (nAttributeKey > KWIndexedNKeyBlock::GetMaxKey())
						{
							AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
								 sAttributeKey + ") should be less or equal than " +
								 IntToString(KWIndexedNKeyBlock::GetMaxKey()) +
								 " for variable " + attribute->GetName() +
								 " in the block");
							bOk = false;
						}
					}
				}

				// Test de l'unicite de la valeur de la meta-donne dans le bloc de variables
				if (bOk)
				{
					if (odVariableKeys.Lookup(sAttributeKey) != NULL)
					{
						AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
							 attribute->GetConstMetaData()->GetExternalValueAt(
							     sAttributeKeyMetaDataKey) +
							 ") related to variable " + attribute->GetName() +
							 " in the block already used by another variable (" +
							 cast(KWAttribute*, odVariableKeys.Lookup(sAttributeKey))
							     ->GetName() +
							 ")");
						bOk = false;
					}
					else
						odVariableKeys.SetAt(sAttributeKey, attribute);
				}
			}

			// Arret si erreur
			if (not bOk)
				break;

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;

			// Passage a l'attribut suivant
			parentClass->GetNextAttribute(attribute);
		}

		// Erreur si l'attribut vaut NULL, signifiant que la derniere variable du bloc n'a pas ete trouvee
		if (attribute == NULL)
		{
			AddError("Last variable not found in the block");
			bOk = false;
		}
	}

	// Un bloc de type ObjectArray doit necessairement etre derive
	if (bOk and firstAttribute->GetType() == KWType::ObjectArray and kwdrRule == NULL)
	{
		AddError("Missing derivation rule for a sparse variable block containing variables of type " +
			 KWType::ToString(GetType()));
		bOk = false;
	}

	// Verification de la regle de derivation
	if (bOk and kwdrRule != NULL)
	{
		// Verification de l'existence de la regle de derivation
		parentClass = GetParentClass();
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
				AddError(kwdrRule->GetClassLabel() + " incorrectly specified");
				bOk = false;
			}
		}

		// Verification de la regle de derivation elle-meme
		if (bOk and parentClass != NULL and not kwdrRule->CheckCompleteness(parentClass))
		{
			AddError(kwdrRule->GetClassLabel() + " incorrectly specified");
			bOk = false;
		}

		// Verification du type de la regle
		if (bOk and not KWType::IsValueBlock(kwdrRule->GetType()))
		{
			AddError("Type of rule " + kwdrRule->GetName() + " should a sparse value block");
			bOk = false;
		}

		// Verification de la coherence avec le type de l'attribut cible de la regle
		if (bOk and kwdrRule->GetType() != GetBlockType())
		{
			AddError("Rule " + kwdrRule->GetName() + " with type " + KWType::ToString(kwdrRule->GetType()) +
				 " inconsistent with that of the sparse variable block (" +
				 KWType::ToString(GetBlockType()) + ")");
			bOk = false;
		}

		// Cas d'un type ObjectArray
		if (bOk and GetBlockType() == KWType::ObjectArrayValueBlock)
		{
			if (firstAttribute->GetClass() != NULL and
			    firstAttribute->GetClass()->GetName() != kwdrRule->GetObjectClassName())
			{
				AddError("Rule " + kwdrRule->GetName() + " with type " +
					 KWType::ToString(kwdrRule->GetType()) + "(" + kwdrRule->GetObjectClassName() +
					 ")" + " inconsistent with that of the sparse variable block (" +
					 KWType::ToString(GetBlockType()) + "(" +
					 firstAttribute->GetClass()->GetName() + "))");
				bOk = false;
			}
		}

		// Verification de la coherence entre de type de cles des variable du bloc et celui de la regle
		if (bOk and kwdrRule->GetVarKeyType() != GetVarKeyType())
		{
			AddError("Rule " + kwdrRule->GetName() + " with var key type " +
				 KWType::ToString(kwdrRule->GetVarKeyType()) +
				 " inconsistent with that of the sparse variable block (" +
				 KWType::ToString(GetVarKeyType()) + ")");
			bOk = false;
		}

		// Verification que les attributs d'un bloc sont tous presents via leur VarKey
		// dans les blocs en operandes de la regle
		if (bOk and not kwdrRule->CheckBlockAttributes(GetParentClass(), this))
		{
			// Pas de message d'erreur, cas deja emis par la methode de verification appelee
			bOk = false;
		}
	}
	return bOk;
}

void KWAttributeBlock::SetVarKeyType(int nValue)
{
	require(loadedAttributesIndexedKeyBlock == NULL);
	require(nValue == KWType::Continuous or nValue == KWType::Symbol);
	if (nValue == KWType::Continuous)
		loadedAttributesIndexedKeyBlock = new KWIndexedNKeyBlock;
	else
		loadedAttributesIndexedKeyBlock = new KWIndexedCKeyBlock;
}

KWIndexedKeyBlock* KWAttributeBlock::BuildAttributesIndexedKeyBlock(NumericKeyDictionary* nkdBlockAttributes) const
{
	KWIndexedKeyBlock* attributesIndexedKeyBlock;
	KWIndexedCKeyBlock* attributesIndexedCKeyBlock;
	KWIndexedNKeyBlock* attributesIndexedNKeyBlock;
	KWClass* parentClass;
	KWAttribute* attribute;
	SymbolVector svVarKeys;
	IntVector ivVarKeys;
	Symbol sVarKey;
	int nVarKey;
	int i;

	require(GetVarKeyType() == KWType::Continuous or GetVarKeyType() == KWType::Symbol);
	require(nkdBlockAttributes != NULL);
	require(nkdBlockAttributes->GetCount() == 0);

	// Cas de cles categorielles
	attributesIndexedKeyBlock = NULL;
	if (GetVarKeyType() == KWType::Symbol)
	{
		// Creation du bloc de cle
		attributesIndexedCKeyBlock = new KWIndexedCKeyBlock;
		attributesIndexedKeyBlock = attributesIndexedCKeyBlock;

		// Parcours des attribut du bloc pour les enregistrer dans le dictionnaire par VarKey
		// et collecter les cles dans un vecteur
		parentClass = GetParentClass();
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			sVarKey = GetSymbolVarKey(attribute);
			svVarKeys.Add(sVarKey);
			nkdBlockAttributes->SetAt(sVarKey.GetNumericKey(), attribute);

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;

			// Passage a l'attribut suivant
			parentClass->GetNextAttribute(attribute);
		}

		// Tri des cles par valeur
		svVarKeys.SortValues();

		// Memorisation des cles dans le block
		for (i = 0; i < svVarKeys.GetSize(); i++)
		{
			sVarKey = svVarKeys.GetAt(i);

			// Memorisation uniquement des cles valides
			if (not sVarKey.IsEmpty() and
			    (attributesIndexedCKeyBlock->GetKeyNumber() == 0 or
			     attributesIndexedCKeyBlock->GetKeyAt(attributesIndexedCKeyBlock->GetKeyNumber() - 1)
				 .CompareValue(sVarKey)))
				attributesIndexedCKeyBlock->AddKey(sVarKey);
		}
	}
	// Cas de cles numeriques
	else
	{
		// Creation du bloc de cle
		attributesIndexedNKeyBlock = new KWIndexedNKeyBlock;
		attributesIndexedKeyBlock = attributesIndexedNKeyBlock;

		// Parcours des attribut du bloc pour les enregistrer dans le dictionnaire par VarKey
		// et collecter les cles dans un vecteur
		parentClass = GetParentClass();
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			nVarKey = GetContinuousVarKey(attribute);
			ivVarKeys.Add(nVarKey);
			nkdBlockAttributes->SetAt(nVarKey, attribute);

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;

			// Passage a l'attribut suivant
			parentClass->GetNextAttribute(attribute);
		}

		// Tri des cles
		ivVarKeys.Sort();

		// Memorisation des cles dans le block
		for (i = 0; i < ivVarKeys.GetSize(); i++)
		{
			nVarKey = ivVarKeys.GetAt(i);

			// Memorisation uniquement des cles valides
			if (nVarKey >= 1 and (attributesIndexedNKeyBlock->GetKeyNumber() == 0 or
					      attributesIndexedNKeyBlock->GetKeyAt(
						  attributesIndexedNKeyBlock->GetKeyNumber() - 1) < nVarKey))
				attributesIndexedNKeyBlock->AddKey(nVarKey);
		}
	}
	check(attributesIndexedKeyBlock);

	// Indexcation des cles
	attributesIndexedKeyBlock->IndexKeys();
	ensure(nkdBlockAttributes->GetCount() <= attributesIndexedKeyBlock->GetKeyNumber());
	return attributesIndexedKeyBlock;
}

void KWAttributeBlock::SortAttributesByVarKey()
{
	KWClass* parentClass;
	KWAttribute* attribute;
	NumericKeyDictionary nkdBlockAttributesByVarKeys;
	Symbol sVarKey;
	int nVarKey;
	SymbolVector svVarKeys;
	IntVector ivVarKeys;
	int i;

	// Cas de cles categorielles
	if (GetVarKeyType() == KWType::Symbol)
	{
		// Parcours des attribut du bloc pour les enregistrer dans le dictionnaire par VarKey
		// et collecter les cles dans un vecteur
		parentClass = GetParentClass();
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			sVarKey = GetSymbolVarKey(attribute);
			assert(nkdBlockAttributesByVarKeys.Lookup(sVarKey.GetNumericKey()) == NULL);
			nkdBlockAttributesByVarKeys.SetAt(sVarKey.GetNumericKey(), attribute);
			svVarKeys.Add(sVarKey);

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;

			// Passage a l'attribut suivant
			parentClass->GetNextAttribute(attribute);
		}

		// Tri des cles
		svVarKeys.SortValues();

		// Parcours des cles dans l'ordre pour reordonnancer les attributs
		for (i = 0; i < svVarKeys.GetSize(); i++)
		{
			attribute =
			    cast(KWAttribute*, nkdBlockAttributesByVarKeys.Lookup(svVarKeys.GetAt(i).GetNumericKey()));
			parentClass->MoveAttributeToBlockTail(attribute);
		}
	}
	// Cas de cles numeriques
	else
	{
		// Parcours des attribut du bloc pour les enregistrer dans le dictionnaire par VarKey
		// et collecter les cles dans un vecteur
		parentClass = GetParentClass();
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			nVarKey = GetContinuousVarKey(attribute);
			assert(nkdBlockAttributesByVarKeys.Lookup(nVarKey) == NULL);
			nkdBlockAttributesByVarKeys.SetAt(nVarKey, attribute);
			ivVarKeys.Add(nVarKey);

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;

			// Passage a l'attribut suivant
			parentClass->GetNextAttribute(attribute);
		}

		// Tri des cles
		ivVarKeys.Sort();

		// Parcours des cles dans l'ordre pour reordonnancer les attributs
		for (i = 0; i < ivVarKeys.GetSize(); i++)
		{
			attribute = cast(KWAttribute*, nkdBlockAttributesByVarKeys.Lookup(ivVarKeys.GetAt(i)));
			parentClass->MoveAttributeToBlockTail(attribute);
		}
	}
}

void KWAttributeBlock::Compile()
{
	// Indexation des cles du bloc
	assert(loadedAttributesIndexedKeyBlock != NULL);
	loadedAttributesIndexedKeyBlock->IndexKeys();

	// Compilation de la regle du bloc
	if (kwdrRule != NULL and not kwdrRule->IsCompiled())
		kwdrRule->Compile(GetParentClass());

	// Calcul des valeur par defaut
	BuildAdvancedTypeSpecification();
}

boolean KWAttributeBlock::ContainsCycle(NumericKeyDictionary* nkdGreyAttributes,
					NumericKeyDictionary* nkdBlackAttributes) const
{
	boolean bContainsCycle = false;

	require(nkdGreyAttributes != NULL);
	require(nkdBlackAttributes != NULL);
	require(Check());
	require(GetParentClass()->IsCompiled());

	// Analyse de l'eventuelle regle de derivation attachee a l'attribut
	// Sinon, l'attribut est un noeud terminal du graphe, et n'a pas besoin d'etre analyse
	if (GetDerivationRule() != NULL)
	{
		// Marquage de l'attribut en Grey
		nkdGreyAttributes->SetAt(this, (Object*)this);

		// Analyse de la regle de derivation
		bContainsCycle = GetDerivationRule()->ContainsCycle(nkdGreyAttributes, nkdBlackAttributes);

		// Marquage du bloc d'attributs en Black
		nkdGreyAttributes->RemoveKey(this);
		nkdBlackAttributes->SetAt(this, (Object*)this);
	}
	return bContainsCycle;
}

longint KWAttributeBlock::GetUsedMemory() const
{
	longint lUsedMemory;

	// On prend en compte la memoire du nom et du libelle, meme s'ils sont potentiellement partages
	lUsedMemory = sizeof(KWAttributeBlock);
	lUsedMemory += usName.GetUsedMemory();
	lUsedMemory += usLabel.GetUsedMemory();
	lUsedMemory += svComments.GetUsedMemory();
	lUsedMemory += svInternalComments.GetUsedMemory();
	lUsedMemory += metaData.GetUsedMemory() - sizeof(KWMetaData);

	// Prise en compte de la regle de derivation
	if (kwdrRule != NULL)
		lUsedMemory += kwdrRule->GetUsedMemory();

	// Prise en compte des donnees additionnelles de gestion du bloc
	if (loadedAttributesIndexedKeyBlock != NULL)
		lUsedMemory += loadedAttributesIndexedKeyBlock->GetUsedMemory();
	if (ivLoadedAttributeMutationIndexes != NULL)
		lUsedMemory += ivLoadedAttributeMutationIndexes->GetUsedMemory();
	lUsedMemory += nkdAttributesByVarKeys.GetUsedMemory() - sizeof(NumericKeyDictionary);
	lUsedMemory += oaLoadedAttributes.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += ivLoadedAttributeIndexesBySparseIndex.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

longint KWAttributeBlock::ComputeHashValue() const
{
	longint lHash;

	lHash = HashValue("Block");
	lHash = LongintUpdateHashValue(lHash, HashValue(GetName()));
	if (firstAttribute != NULL)
		lHash = LongintUpdateHashValue(lHash, HashValue(firstAttribute->GetName()));
	if (lastAttribute != NULL)
		lHash = LongintUpdateHashValue(lHash, HashValue(lastAttribute->GetName()));
	if (kwdrRule != NULL)
		lHash = LongintUpdateHashValue(lHash, kwdrRule->ComputeHashValue());
	return lHash;
}

void KWAttributeBlock::Write(ostream& ost) const
{
	KWClass* parentClass;
	KWAttribute* attribute;
	int i;

	// Commentaires precedant le debut du bloc
	for (i = 0; i < GetComments()->GetSize(); i++)
		ost << "\t// " << GetComments()->GetAt(i) << "\n";
	ost << "\t{\n";

	// Attributs du bloc
	parentClass = GetParentClass();
	attribute = firstAttribute;
	while (attribute != NULL)
	{
		// Ecriture de l'attribut
		ost << *attribute << "\n";

		// Arret si derniere variable du bloc trouvee
		if (attribute == lastAttribute)
			break;

		// Passage a l'attribut suivant
		parentClass->GetNextAttribute(attribute);
	}

	// Commentaires internes precedents la fin du bloc
	for (i = 0; i < GetInternalComments()->GetSize(); i++)
		ost << "\t// " << GetInternalComments()->GetAt(i) << "\n";
	ost << "\t}";

	// Nom du bloc
	ost << "\t" << KWClass::GetExternalName(GetName());
	ost << "\t";

	// Regle de derivation
	if (GetDerivationRule() != NULL)
	{
		// Dans le cas de la regle predefinie de Reference, on n'utilise pas le signe '='
		if (GetDerivationRule()->GetName() != KWDerivationRule::GetReferenceRuleName())
			ost << " = ";
		GetDerivationRule()->WriteUsedRule(ost);
	}

	// Fin de declaration
	ost << "\t;";

	// Meta-donnees
	if (GetConstMetaData()->GetKeyNumber() > 0)
	{
		ost << ' ';
		GetConstMetaData()->Write(ost);
	}
	ost << "\t";

	// Libelle
	if (GetLabel() != "")
		ost << "// " << GetLabel();
}

void KWAttributeBlock::WriteJSONFields(JSONFile* fJSON) const
{
	KWClass* parentClass;
	KWAttribute* attribute;
	ALString sOutputString;
	int i;

	// Nom
	fJSON->WriteKeyString("blockName", GetName());

	// Libelle
	if (GetLabel() != "")
		fJSON->WriteKeyString("label", GetLabel());

	// Commentaires, ecrits uniquement si presents, comme pour les autres champs facultatifs,
	// ce qui facilite la compatibilite ascendante, et diminue la volumetrie
	if (GetComments()->GetSize() > 0)
	{
		fJSON->BeginKeyArray("comments");
		for (i = 0; i < GetComments()->GetSize(); i++)
			fJSON->WriteString(GetComments()->GetAt(i));
		fJSON->EndArray();
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

	// Attributes du bloc
	fJSON->BeginKeyArray("variables");
	parentClass = GetParentClass();
	attribute = firstAttribute;
	while (attribute != NULL)
	{
		// Ecriture de l'attribut
		attribute->WriteJSONReport(fJSON);

		// Arret si derniere variable du bloc trouvee
		if (attribute == lastAttribute)
			break;

		// Passage a l'attribut suivant
		parentClass->GetNextAttribute(attribute);
	}
	fJSON->EndArray();

	// Commentaires internes, uniquement si presents, comme pour les commentaires
	if (GetInternalComments()->GetSize() > 0)
	{
		fJSON->BeginKeyArray("internalComments");
		for (i = 0; i < GetInternalComments()->GetSize(); i++)
			fJSON->WriteString(GetInternalComments()->GetAt(i));
		fJSON->EndArray();
	}
}

const ALString KWAttributeBlock::GetClassLabel() const
{
	return "Sparse variable block";
}

const ALString KWAttributeBlock::GetObjectLabel() const
{
	return GetName();
}

void KWAttributeBlock::BuildAdvancedTypeSpecification()
{
	ALString sFormatMetaDataKey;
	ALString sFormat;
	ALString sDefaultValueMetaDataKey;

	require(Check());

	// Construction de la valeur par defaut pour les SymbolValueBlock
	if (GetBlockType() == KWType::SymbolValueBlock)
	{
		// Dans le cas d'un bloc calcule, on se base sur la regle de derivation
		if (GetDerivationRule() != NULL)
			sBlockDefaultValue = GetDerivationRule()->GetValueBlockSymbolDefaultValue();
		// Sinon, on se base sur les meta-donnees
		else
		{
			sDefaultValueMetaDataKey = GetDefaultValueMetaDataKey(KWType::Symbol);
			if (metaData.IsKeyPresent(sDefaultValueMetaDataKey))
			{
				assert(metaData.IsStringTypeAt(sDefaultValueMetaDataKey));
				sBlockDefaultValue = Symbol(metaData.GetStringValueAt(sDefaultValueMetaDataKey));
			}
			else
				sBlockDefaultValue = KWSymbolValueBlock::GetDefaultDefaultValue();
		}
	}
	// Construction de la valeur par defaut pour les ContinuousValueBlock
	else if (GetBlockType() == KWType::ContinuousValueBlock)
	{
		// Dans le cas d'un bloc calcule, on se base sur la regle de derivation
		if (GetDerivationRule() != NULL)
			cBlockDefaultValue = GetDerivationRule()->GetValueBlockContinuousDefaultValue();
		// Sinon, on se base sur les meta-donnees
		else
		{
			sDefaultValueMetaDataKey = GetDefaultValueMetaDataKey(KWType::Continuous);
			if (metaData.IsKeyPresent(sDefaultValueMetaDataKey))
			{
				assert(metaData.IsDoubleTypeAt(sDefaultValueMetaDataKey));
				cBlockDefaultValue = metaData.GetDoubleValueAt(sDefaultValueMetaDataKey);
			}
			else
				cBlockDefaultValue = KWContinuousValueBlock::GetDefaultDefaultValue();
		}
	}
}

void KWAttributeBlock::ComputeLoadedAttributeMutationIndexes(const KWAttributeBlock* targetAttributeBlock) const
{
	int nAttribute;
	KWAttribute* attribute;
	KWAttribute* targetAttribute;
	KWClass* kwcTargetClass;

	require(targetAttributeBlock != NULL);
	require(targetAttributeBlock->GetName() == GetName());
	require(GetParentClass()->IsCompiled());
	require(targetAttributeBlock->GetParentClass()->IsCompiled());
	require(GetParentClass()->GetName() == targetAttributeBlock->GetParentClass()->GetName());
	require(GetLoadedAttributeNumber() <= targetAttributeBlock->GetAttributeNumber());

	// Nettoyage prealable
	if (ivLoadedAttributeMutationIndexes != NULL)
		delete ivLoadedAttributeMutationIndexes;
	ivLoadedAttributeMutationIndexes = NULL;

	// Creation du vecteur des index de mutation
	ivLoadedAttributeMutationIndexes = new IntVector;
	ivLoadedAttributeMutationIndexes->SetSize(GetLoadedAttributeNumber());

	// Calcul des index de mutation
	kwcTargetClass = targetAttributeBlock->GetParentClass();
	for (nAttribute = 0; nAttribute < GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetLoadedAttributeAt(nAttribute);
		targetAttribute = kwcTargetClass->LookupAttribute(attribute->GetName());
		assert(targetAttribute != NULL);

		// Correspondance entre les index sparse des attributs
		assert(0 <= attribute->GetLoadIndex().GetSparseIndex() and
		       attribute->GetLoadIndex().GetSparseIndex() < GetLoadedAttributeNumber());
		ivLoadedAttributeMutationIndexes->SetAt(attribute->GetLoadIndex().GetSparseIndex(),
							targetAttribute->GetLoadIndex().GetSparseIndex());
	}
}
