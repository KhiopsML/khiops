// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRDataGridBlock.h"

void KWDRRegisterDataGridBlockRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGridBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRCellIndexBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGridStatsBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGridStatsBlockTest);
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridBlock

KWDRDataGridBlock::KWDRDataGridBlock()
{
	SetName("DataGridBlock");
	SetLabel("Data grid block");
	SetType(KWType::Structure);
	SetStructureName("DataGridBlock");
	SetOperandNumber(2);
	SetVariableOperandNumber(true);

	// Le premier operande contient une regles de type Structure, pour des VarKey soit numeriques, soit
	// categorielles
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetFirstOperand()->SetStructureName("ValueSet");

	// Les autres operandes sont des DataGrid
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("DataGrid");

	// Parametre de la compilation optimisee
	nOptimizationFreshness = 0;
	nDataGridAttributeNumber = 0;
	nDataGridVarKeyType = KWType::Unknown;
}

KWDRDataGridBlock::~KWDRDataGridBlock() {}

int KWDRDataGridBlock::GetUncheckedVarKeyNumber() const
{
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRSymbolValueSet symbolValueSetRule;

	// Erreur si pas d'operande
	if (GetOperandNumber() == 0)
		return -1;
	// Analyse de la regle si derivation du premier operande si presente
	else if (GetFirstOperand()->GetType() == KWType::Structure and
		 GetFirstOperand()->GetDerivationRule() != NULL and
		 GetFirstOperand()->GetStructureName() == GetFirstOperand()->GetDerivationRule()->GetName())
	{
		if (GetFirstOperand()->GetStructureName() == continuousValueSetRule.GetName())
			return cast(KWDRContinuousValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueNumber();
		else if (GetFirstOperand()->GetStructureName() == symbolValueSetRule.GetName())
			return cast(KWDRSymbolValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueNumber();
		else
			return -1;
	}
	// Erreur sinon
	else
		return -1;
}

int KWDRDataGridBlock::GetUncheckedDataGridVarKeyType() const
{
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRSymbolValueSet symbolValueSetRule;

	// Erreur si pas d'operande
	if (GetOperandNumber() == 0)
		return KWType::Unknown;
	// Analyse de la regle si derivation du premier operande si presente
	else if (GetFirstOperand()->GetType() == KWType::Structure and
		 GetFirstOperand()->GetDerivationRule() != NULL and
		 GetFirstOperand()->GetStructureName() == GetFirstOperand()->GetDerivationRule()->GetName())
	{
		if (GetFirstOperand()->GetStructureName() == continuousValueSetRule.GetName())
			return KWType::Continuous;
		else if (GetFirstOperand()->GetStructureName() == symbolValueSetRule.GetName())
			return KWType::Symbol;
		else
			return KWType::Unknown;
	}
	// Erreur sinon
	else
		return KWType::Unknown;
}

KWDerivationRule* KWDRDataGridBlock::Create() const
{
	return new KWDRDataGridBlock;
}

void KWDRDataGridBlock::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Optimisation
		Optimize(kwcOwnerClass);
	}
}

Object* KWDRDataGridBlock::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// On retourne l'objet directement, dont le contenu ne depend pas des objets de la base
	return (Object*)this;
}

boolean KWDRDataGridBlock::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDRSymbolValueSet symbolValueSetRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRDataGridBlock datagridBlockSpecifiedFamily;
	int nUncheckedVarKeyType;
	KWDRSymbolValueSet* symbolVarKeyRule;
	KWDRContinuousValueSet* continuousValueKeyRule;
	int nVarKeyNumber;
	int nIndex;
	Continuous cVarKey;
	int nVarKey;
	Symbol sVarKey;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(GetName() == ruleFamily->GetName());

	// On commence par verifier que le type du premier operande
	nUncheckedVarKeyType = GetUncheckedDataGridVarKeyType();
	if (nUncheckedVarKeyType == KWType::Unknown)
	{
		bOk = false;
		AddError("First operand should be of type " + continuousValueSetRule.GetName() + " or " +
			 symbolValueSetRule.GetName());
		AddError("The number of operands should be at least 2");
	}

	// On utilise une regle dont on specifie le type du premier operande pour finaliser la verification
	if (bOk)
	{
		// Specification du type du premier operande
		if (nUncheckedVarKeyType == KWType::Continuous)
			datagridBlockSpecifiedFamily.GetFirstOperand()->SetStructureName(
			    continuousValueSetRule.GetName());
		else
			datagridBlockSpecifiedFamily.GetFirstOperand()->SetStructureName(symbolValueSetRule.GetName());

		// On verifie alors la regle avec cette famille correctement specifiee par appel;de la la methode
		// ancetre
		bOk = KWDerivationRule::CheckOperandsFamily(&datagridBlockSpecifiedFamily);
	}

	// On verifie le nombre de DataGrid
	// Les grilles elles memes-sont verifiees dans CheckOperandsCompleteness, car on a besoin de la kwcOwnerClass
	// pour y acceder
	symbolVarKeyRule = NULL;
	continuousValueKeyRule = NULL;
	nVarKeyNumber = 0;
	if (bOk)
	{
		// Acces au vecteur de VarKey
		if (nUncheckedVarKeyType == KWType::Continuous)
		{
			continuousValueKeyRule = cast(KWDRContinuousValueSet*, GetFirstOperand()->GetDerivationRule());
			nVarKeyNumber = continuousValueKeyRule->GetValueNumber();
		}
		else
		{
			assert(nUncheckedVarKeyType == KWType::Symbol);
			symbolVarKeyRule = cast(KWDRSymbolValueSet*, GetFirstOperand()->GetDerivationRule());
			nVarKeyNumber = symbolVarKeyRule->GetValueNumber();
		}

		// Le nombre de VarKey doit etre au moins 1
		if (nVarKeyNumber == 0)
		{
			bOk = false;
			AddError(sTmp + "The number of VarKeys in the first operand should be at least 1");
		}
		// Le nombre de VarKey doit etre egal au nombre de grilles
		else if (nVarKeyNumber != GetOperandNumber() - 1)
		{
			bOk = false;
			AddError(sTmp + "The number of VarKeys (" + IntToString(nVarKeyNumber) +
				 ") in the first operand should be the same as the number of data grid operands (" +
				 IntToString(GetOperandNumber() - 1) + ")");
		}
	}

	// On verifie que les VarKey numeriques sont valides
	if (bOk and nUncheckedVarKeyType == KWType::Continuous)
	{
		for (nIndex = 0; nIndex < nVarKeyNumber; nIndex++)
		{
			cVarKey = continuousValueKeyRule->GetValueAt(nIndex);
			bOk = KWContinuous::ContinuousToInt(cVarKey, nVarKey);

			// La VarKey doit etre entiere
			if (not bOk)
			{
				assert(bOk == false);
				AddError(sTmp + "VarKey " + KWContinuous::ContinuousToString(cVarKey) +
					 " in the first operand should be an integer");
			}
			// Test de valeur min
			else if (nVarKey < KWIndexedNKeyBlock::GetMinKey())
			{
				AddError(sTmp + "VarKey " + KWContinuous::ContinuousToString(cVarKey) +
					 " in the first operand should be greater or equal than " +
					 IntToString(KWIndexedNKeyBlock::GetMinKey()));
				bOk = false;
			}
			// Test de valeur max
			else if (nVarKey > KWIndexedNKeyBlock::GetMaxKey())
			{
				AddError(sTmp + "VarKey " + KWContinuous::ContinuousToString(cVarKey) +
					 " in the first operand should be be less or equal than " +
					 IntToString(KWIndexedNKeyBlock::GetMaxKey()));
				bOk = false;
			}
			// Test sur l'ordre des VarKey (potentiellement deja detecte dans la regle ValueSet
			else if (nIndex > 0 and cVarKey <= continuousValueKeyRule->GetValueAt(nIndex - 1))
			{
				AddError(
				    sTmp + "VarKey " + KWContinuous::ContinuousToString(cVarKey) +
				    " in the first operand should be greater that its predecessor " +
				    KWContinuous::ContinuousToString(continuousValueKeyRule->GetValueAt(nIndex - 1)));
				bOk = false;
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}

	// On verifie que les VarKey categorielles sont valides
	if (bOk and nUncheckedVarKeyType == KWType::Symbol)
	{
		for (nIndex = 0; nIndex < nVarKeyNumber; nIndex++)
		{
			sVarKey = symbolVarKeyRule->GetValueAt(nIndex);

			// La VarKey doit etre non vide
			if (sVarKey.IsEmpty())
			{
				bOk = false;
				AddError(sTmp + "VarKey in the first operand should not be empty");
			}
			// Test sur l'ordre des VarKey
			else if (nIndex > 0 and sVarKey.CompareValue(symbolVarKeyRule->GetValueAt(nIndex - 1)) <= 0)
			{
				AddError(sTmp + "VarKey \"" + sVarKey.GetValue() +
					 "\" in the first operand should be greater that its predecessor \"" +
					 symbolVarKeyRule->GetValueAt(nIndex - 1).GetValue() + "\"");
				bOk = false;
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean KWDRDataGridBlock::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWDRSymbolValueSet symbolValueSetRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRDataGridBlock datagridBlockSpecifiedFamily;
	KWDRDataGrid* referencedDataGrid;
	int nReferenceFirstType;
	int nReferenceSecondType;
	int nFirstType;
	int nSecondType;
	int nIndex;
	int nDataGridNumber;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(GetOperandNumber() > 0);

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// On verifie que chaque grille est bivariee (univarie supervise), avec les meme types pour toutes les grilles
	if (bOk)
	{
		nReferenceFirstType = KWType::Unknown;
		nReferenceSecondType = KWType::Unknown;
		nDataGridNumber = GetOperandNumber() - 1;
		for (nIndex = 0; nIndex < nDataGridNumber; nIndex++)
		{
			referencedDataGrid =
			    cast(KWDRDataGrid*, GetOperandAt(nIndex + 1)->GetReferencedDerivationRule(kwcOwnerClass));

			// La grille doit avoir une seul attribut
			if (bOk and referencedDataGrid->GetUncheckedAttributeNumber() != 2)
			{
				bOk = false;
				AddError(sTmp + "The data grid in the operand " + IntToString(nIndex + 2) +
					 " should be bivariate");
			}

			// Memorisation du type de la premiere grille rencontree
			if (bOk and nIndex == 0)
			{
				nReferenceFirstType = referencedDataGrid->GetUncheckedAttributeTypeAt(0);
				nReferenceSecondType = referencedDataGrid->GetUncheckedAttributeTypeAt(1);
			}

			// Les deux attributs de cette grille doit etre du meme type pour toutes les grilles
			if (bOk)
			{
				nFirstType = referencedDataGrid->GetUncheckedAttributeTypeAt(0);
				if (nFirstType != nReferenceFirstType)
				{
					bOk = false;
					AddError(sTmp + "The type " + KWType::ToString(nFirstType) +
						 " of the first dimension of the data grid in the operand " +
						 IntToString(nIndex + 2) + " should be " +
						 KWType::ToString(nReferenceFirstType) +
						 " as in the first data grid operand");
				}
				nSecondType = referencedDataGrid->GetUncheckedAttributeTypeAt(1);
				if (nSecondType != nReferenceSecondType)
				{
					bOk = false;
					AddError(sTmp + "The type " + KWType::ToString(nSecondType) +
						 " of the second dimension of the data grid in the operand " +
						 IntToString(nIndex + 2) + " should be " +
						 KWType::ToString(nReferenceSecondType) +
						 " as in first data grid operand");
				}
			}
			if (not bOk)
				break;
		}
	}
	return bOk;
}

longint KWDRDataGridBlock::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridBlock) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

void KWDRDataGridBlock::Optimize(KWClass* kwcOwnerClass)
{
	require(IsCompiled());

	// Memorisation du nombre de grille et du type de VarKey
	nDataGridAttributeNumber = GetUncheckedDataGridNumber();
	nDataGridVarKeyType = GetUncheckedDataGridVarKeyType();
	ensure(nDataGridAttributeNumber > 0);
	ensure(nDataGridVarKeyType == KWType::Symbol or nDataGridVarKeyType == KWType::Continuous);
}

boolean KWDRDataGridBlock::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridBlockRule

boolean KWDRDataGridBlockRule::CheckOperandsDefinition() const
{
	boolean bOk = true;

	// On ne verifie que le premier operande, le second n'ayant pas de type defini a ce stade
	if (GetOperandNumber() > 0 and not GetFirstOperand()->CheckDefinition())
	{
		AddError("Incorrect operand 1");
		bOk = false;
	}

	return bOk;
}

boolean KWDRDataGridBlockRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(GetName() == ruleFamily->GetName());
	require(ruleFamily->GetOperandNumber() == 2);
	require(GetOperandNumber() == 2);

	// Verification du premier operande
	// La verification du second operande ne sera effectuee que dans CheckOperandsCompleteness
	// quand on aura les informations sur le type de variable gere par le DataGridBlock
	if (not GetFirstOperand()->CheckFamily(ruleFamily->GetFirstOperand()))
	{
		AddError(sTmp + "Operand 1 inconsistent with that of the registered rule");
		bOk = false;
	}
	return bOk;
}

boolean KWDRDataGridBlockRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWDRDataGridBlock* referencedDataGridBlock;
	KWDRDataGrid* referencedDataGrid;
	int nDataGridBlockAttributeType;
	KWDerivationRuleOperand secondFamilyOperand;
	KWAttributeBlock* originAttributeBlock;
	const KWIndexedKeyBlock* sourceIndexedKeyBlock;
	const KWIndexedNKeyBlock* sourceIndexedNKeyBlock;
	const KWIndexedCKeyBlock* sourceIndexedCKeyBlock;
	int nSourceIndex;
	int nIndex;
	Symbol sKey;
	int nKey;

	ALString sTmp;

	require(kwcOwnerClass != NULL);

	// Acces au premier operande de type DataGridBlock, correct puisque l'on est passe par CheckDefinition
	referencedDataGridBlock =
	    cast(KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	check(referencedDataGridBlock);
	assert(referencedDataGridBlock->GetUncheckedDataGridNumber() > 0);

	// Le type de VarKey du DataGridBlock doit etre valide a ce stade
	assert(nDataGridVarKeyType == referencedDataGridBlock->GetUncheckedDataGridVarKeyType());
	assert(KWType::IsSimple(nDataGridVarKeyType));

	// Recherche de la premiere grille du DataGridBlock
	referencedDataGrid =
	    cast(KWDRDataGrid*, referencedDataGridBlock->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	check(referencedDataGrid);
	assert(referencedDataGrid->GetUncheckedAttributeNumber() == 2);

	// Type du premier attribut de la grille
	nDataGridBlockAttributeType = referencedDataGrid->GetUncheckedAttributeTypeAt(0);
	assert(KWType::IsSimple(nDataGridBlockAttributeType));

	// Verification du deuxieme operande, pilote par les information du premier operande
	secondFamilyOperand.SetType(KWType::GetValueBlockType(nDataGridBlockAttributeType));
	if (not GetSecondOperand()->CheckFamily(&secondFamilyOperand))
	{
		AddError(sTmp + "Operand 2 inconsistent with that of the registered rule");
		bOk = false;
	}

	// Appel de la methode ancetre
	if (bOk)
		bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que les cles des grilles sont presentes dans les cles du bloc en deuxieme operande
	if (bOk)
	{
		// Recherche du bloc d'attribut source dans la classe
		// Il doit etre present puisque le CheckOperandsCompleteness est ok
		originAttributeBlock = kwcOwnerClass->LookupAttributeBlock(GetOperandAt(1)->GetAttributeBlockName());
		check(originAttributeBlock);

		// Calcul du bloc de cle source
		// On ne peut utiliser GetLoadedAttributesIndexedKeyBlock, car la classe n'est pas encore indexee a ce
		// stade, et qu'il faut faire la verification par rapport a toutes les VarKey sources
		sourceIndexedKeyBlock = originAttributeBlock->BuildAttributesIndexedKeyBlock();

		// Verification de la coherence de type des VarKeys entre le DataGridBlock et le block de valeurs
		// soiurce en deuxieme operande
		if (referencedDataGridBlock->GetUncheckedDataGridVarKeyType() != sourceIndexedKeyBlock->GetVarKeyType())
		{
			AddError(sTmp + "VarKey type (" +
				 KWType::ToString(referencedDataGridBlock->GetUncheckedDataGridVarKeyType()) +
				 ") in data grid block first operand should be " +
				 KWType::ToString(sourceIndexedKeyBlock->GetVarKeyType()) +
				 " as in value block second operand ");
			bOk = false;
		}

		// Verification des cles
		if (bOk)
		{
			// Cas des blocs avec des VarKey categoriels
			if (sourceIndexedKeyBlock->GetVarKeyType() == KWType::Symbol)
			{
				// On prend les bons types de blocs de cles
				sourceIndexedCKeyBlock = cast(const KWIndexedCKeyBlock*, sourceIndexedKeyBlock);

				// On test la presence des VarKeys du DataGridBlock dans le bloc source
				for (nIndex = 0; nIndex < referencedDataGridBlock->GetUncheckedVarKeyNumber(); nIndex++)
				{
					sKey = referencedDataGridBlock->GetUncheckedSymbolVarKeyAt(nIndex);

					// On ne verifie que les cles valides
					// Les cles invalides seront diagnostiquees par ailleurs
					if (not sKey.IsEmpty())
					{
						// Recherche de l'index dans le bloc source
						nSourceIndex = sourceIndexedCKeyBlock->GetKeyIndex(sKey);

						// Erreur si cle absente
						if (nSourceIndex == -1)
						{
							AddError(sTmp + "VarKey " + sKey.GetValue() +
								 " in data grid block first operand is missing in "
								 "value block second operand");
							bOk = false;
							break;
						}
					}
				}
			}
			// Cas des blocs avec des VarKey numeriques
			else
			{
				// On prend les bons types de blocs de cles
				sourceIndexedNKeyBlock = cast(const KWIndexedNKeyBlock*, sourceIndexedKeyBlock);

				// On test la presence des VarKeys du DataGridBlock dans le bloc source
				for (nIndex = 0; nIndex < referencedDataGridBlock->GetUncheckedVarKeyNumber(); nIndex++)
				{
					nKey = referencedDataGridBlock->GetUncheckedContinuousVarKeyAt(nIndex);

					// On ne verifie que les cles valides
					// Les cles invalides seront diagnostiquees par ailleurs
					if (nKey != -1)
					{
						// Recherche de l'index dans le bloc source
						nSourceIndex = sourceIndexedNKeyBlock->GetKeyIndex(nKey);

						// Erreur si cle absente
						if (nSourceIndex == -1)
						{
							AddError(sTmp + "VarKey " + IntToString(nKey) +
								 " in data grid block first operand is missing in "
								 "value block second operand");
							bOk = false;
							break;
						}
					}
				}
			}
		}

		// Nettoyage
		delete sourceIndexedKeyBlock;
	}
	return bOk;
}

boolean KWDRDataGridBlockRule::CheckPredictorCompletness(int nPredictorType, const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWDRDataGridBlock* referencedDataGridBlock;
	KWDRDataGrid* referencedDataGrid;
	int nTargetAttributeType;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(CheckCompleteness(kwcOwnerClass));

	// Acces au premier operande de type DataGridBlock, correct puisque l'on est passe par CheckCompleteness
	referencedDataGridBlock =
	    cast(KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	check(referencedDataGridBlock);
	assert(referencedDataGridBlock->GetUncheckedDataGridNumber() > 0);

	// Acces a la premiere grille
	referencedDataGrid =
	    cast(KWDRDataGrid*, referencedDataGridBlock->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));

	// Recherche du type du dernier attribut de la grille
	nTargetAttributeType = referencedDataGrid->GetUncheckedAttributeTypeAt(1);

	// Test de consistence de ce type
	if (nTargetAttributeType != nPredictorType)
	{
		if (nPredictorType == KWType::Symbol)
			AddError(sTmp + "Wrong type of the last variable of data grids for classification");
		else
			AddError(sTmp + "Wrong type of the last variable of data grids for regression");
		bOk = false;
	}
	return bOk;
}

void KWDRDataGridBlockRule::BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
						   NumericKeyDictionary* nkdAllUsedAttributes) const
{
	boolean bTrace = false;
	const KWDRDataGridBlock* firstOperandDataGridBlock;
	KWAttributeBlock* secondOperandAttributeBlock;
	NumericKeyDictionary nkdInitialAllUsedAttributes;
	ObjectArray oaAllUsedAttributes;
	int nAttribute;
	KWAttribute* attribute;
	Symbol sVarKey;
	int nVarKey;

	require(IsCompiled());

	// Memorisation des attributs utilise initiaux
	if (bTrace)
		nkdInitialAllUsedAttributes.CopyFrom(nkdAllUsedAttributes);

	// Appel de la methode ancetre
	KWDerivationRule::BuildAllUsedAttributes(derivedAttribute, nkdAllUsedAttributes);

	// Recherche du bloc de grille en premier operande
	firstOperandDataGridBlock =
	    cast(const KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(GetOwnerClass()));
	check(firstOperandDataGridBlock);

	// Recherche du bloc en deuxieme operande
	secondOperandAttributeBlock = GetSecondOperand()->GetOriginAttributeBlock();
	assert(firstOperandDataGridBlock->GetDataGridVarKeyType() == secondOperandAttributeBlock->GetVarKeyType());

	// Parcours des VarKey du bloc de grilles pour chercher les attributs utilises dans le bloc du deuxieme operande
	for (nAttribute = 0; nAttribute < firstOperandDataGridBlock->GetDataGridNumber(); nAttribute++)
	{
		// Recherche de l'attribut du bloc correspondant a la grille, c'est a dire de meme VarKey
		// Il doit etre present puisque l'on a passe le CheckOperandsCompleteness de la regle
		// Cas des des VarKey categoriels
		if (GetDataGridVarKeyType() == KWType::Symbol)
		{
			sVarKey = firstOperandDataGridBlock->GetSymbolVarKeyAt(nAttribute);
			attribute = secondOperandAttributeBlock->LookupAttributeBySymbolVarKey(sVarKey);
			check(attribute);
		}
		// Cas des des VarKey numeriques
		else
		{
			nVarKey = firstOperandDataGridBlock->GetContinuousVarKeyAt(nAttribute);
			attribute = secondOperandAttributeBlock->LookupAttributeByContinuousVarKey(nVarKey);
			check(attribute);
		}

		// Memorisation de l'attribut dans le dictionnaire
		nkdAllUsedAttributes->SetAt((NUMERIC)attribute, cast(Object*, attribute));

		// Propagation a la regle du bloc en operande, car celui-ci peut lui meme etre le resultat d'un calcul
		// Notamment s'il se base sur un autre bloc en operande, il faut propager a l'attribut de ce bloc de
		// meme VarKey
		if (secondOperandAttributeBlock->GetDerivationRule() != NULL)
			secondOperandAttributeBlock->GetDerivationRule()->BuildAllUsedAttributes(attribute,
												 nkdAllUsedAttributes);
	}

	// Affichage des nouveau attributs utilises
	if (bTrace)
	{
		// Expport des nouveau attributs utilises
		nkdAllUsedAttributes->ExportObjectArray(&oaAllUsedAttributes);
		oaAllUsedAttributes.SetCompareFunction(KWAttributeCompareName);
		oaAllUsedAttributes.Sort();

		// Affichage des nouveaux attributs non presents parmi les attributs initiaux
		cout << "KWDRDataGridStatsBlock::BuildAllUsedAttributes\t" << nkdInitialAllUsedAttributes.GetCount()
		     << "\t" << nkdAllUsedAttributes->GetCount() - nkdInitialAllUsedAttributes.GetCount() << "\n";
		for (nAttribute = 0; nAttribute < oaAllUsedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaAllUsedAttributes.GetAt(nAttribute));
			if (nkdInitialAllUsedAttributes.Lookup((NUMERIC)attribute) == NULL)
				cout << "\t" << attribute->GetName() << "\n";
		}
	}
}

void KWDRDataGridBlockRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	require(kwdrSource != NULL);

	// Specialisation de la methode ancetre pour recopier le VarKeyType
	KWDerivationRule::CopyFrom(kwdrSource);

	// Reinitialisation de variables issues de l'optimisation de la regle
	nDataGridVarKeyType = cast(KWDRDataGridBlockRule*, kwdrSource)->nDataGridVarKeyType;
	usedDataGridBlock = NULL;
	oaAllRecodingDataGrids.SetSize(0);
	ivUsedRecodingDataGridIndexes.SetSize(0);
	nOptimizationFreshness = 0;
}

void KWDRDataGridBlockRule::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Optimisation
		Optimize(kwcOwnerClass);
	}
}

longint KWDRDataGridBlockRule::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridBlockRule) - sizeof(KWDerivationRule);
	lUsedMemory += oaOperands.GetUsedMemory() - sizeof(ObjectArray);
	return lUsedMemory;
}

KWDRDataGridBlockRule::KWDRDataGridBlockRule()
{
	SetName("DataGridBlockRule");
	SetLabel("DataBlockGrid rule");

	// Le type en retour est a definir dans une sous-classe
	// Le premier operande est un DataGridBlock
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGridBlock");

	// Le deuxieme operande doit etre un bloc numerique ou categoriel, selon le type des grille du premier operandes
	// On ne le sait pas sans la connaissance du premier operande
	GetSecondOperand()->SetType(KWType::Unknown);

	// Type de VarKey du DataGridBlock en premier operande
	nDataGridVarKeyType = KWType::Unknown;

	// Regle DataGridBlock en premier operande
	usedDataGridBlock = NULL;

	// Gestion de la compilation optimisee
	nOptimizationFreshness = 0;
}

void KWDRDataGridBlockRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						     NumericKeyDictionary* nkdCompletedAttributes)
{
	KWDRDataGridBlock* referencedDataGridBlock;

	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Recherche du type de VarKey a partir du deuxieme operande, en principe de type bloc de valeurs
	// Proparagation des caracteristiques du bloc
	nDataGridVarKeyType = KWType::Unknown;
	if (GetOperandNumber() > 0)
	{
		assert(GetFirstOperand()->CheckDefinition());

		// Acces au premier operande de type DataGridBlock, correct puisque l'on est passe par CheckDefinition
		referencedDataGridBlock =
		    cast(KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		check(referencedDataGridBlock);

		// Memorisation du type de VarKey du DataGridBlock
		nDataGridVarKeyType = referencedDataGridBlock->GetUncheckedDataGridVarKeyType();
	}
}

void KWDRDataGridBlockRule::Optimize(KWClass* kwcOwnerClass)
{
	boolean bDisplay = false;
	KWAttributeBlock* originAttributeBlock;
	const KWIndexedKeyBlock* sourceIndexedKeyBlock;
	const KWIndexedNKeyBlock* sourceIndexedNKeyBlock;
	const KWIndexedCKeyBlock* sourceIndexedCKeyBlock;
	int nSourceIndex;
	int nIndex;
	Symbol sVarKey;
	int nVarKey;
	NumericKeyDictionary nkdVarKeyDataGridIndexes;
	IntObject* ioDataGridIndex;

	require(kwcOwnerClass->IsCompiled());
	require(IsCompiled());

	// Memorisation du premier operande de type DataGridBlock
	usedDataGridBlock = cast(KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(GetOwnerClass()));
	check(usedDataGridBlock);
	assert(usedDataGridBlock->IsCompiled());

	// Recherche du bloc d'attribut source
	originAttributeBlock = GetOperandAt(1)->GetOriginAttributeBlock();
	check(originAttributeBlock);

	// Recherche du bloc de cle source
	sourceIndexedKeyBlock = originAttributeBlock->GetLoadedAttributesIndexedKeyBlock();
	assert(sourceIndexedKeyBlock->GetVarKeyType() == usedDataGridBlock->GetDataGridVarKeyType());

	// Initialisation du tableau de grilles
	oaAllRecodingDataGrids.SetSize(usedDataGridBlock->GetDataGridNumber());
	for (nIndex = 0; nIndex < usedDataGridBlock->GetDataGridNumber(); nIndex++)
		oaAllRecodingDataGrids.SetAt(nIndex, usedDataGridBlock->GetDataGridAt(nIndex));

	// Initialisation du vecteur des grilles utilisees
	ivUsedRecodingDataGridIndexes.SetSize(0);
	ivUsedRecodingDataGridIndexes.SetSize(sourceIndexedKeyBlock->GetKeyNumber());

	// Cas des blocs avec des VarKey categoriels
	if (sourceIndexedKeyBlock->GetVarKeyType() == KWType::Symbol)
	{
		// On prend les bons types de blocs de cles
		sourceIndexedCKeyBlock = cast(const KWIndexedCKeyBlock*, sourceIndexedKeyBlock);

		// Memorisation de la grille associee a chaque VarKey dans un dictionnaire
		for (nIndex = 0; nIndex < usedDataGridBlock->GetDataGridNumber(); nIndex++)
		{
			sVarKey = usedDataGridBlock->GetSymbolVarKeyAt(nIndex);
			ioDataGridIndex = new IntObject;
			ioDataGridIndex->SetInt(nIndex);
			nkdVarKeyDataGridIndexes.SetAt(sVarKey.GetNumericKey(), ioDataGridIndex);
		}

		// Parcours des cles du bloc source
		for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
		{
			sVarKey = sourceIndexedCKeyBlock->GetKeyAt(nSourceIndex);

			// Memorisation de l'index de la grille correspondante, -1 si non trouvee
			ioDataGridIndex = cast(IntObject*, nkdVarKeyDataGridIndexes.Lookup(sVarKey.GetNumericKey()));
			if (ioDataGridIndex == NULL)
				ivUsedRecodingDataGridIndexes.SetAt(nSourceIndex, -1);
			else
				ivUsedRecodingDataGridIndexes.SetAt(nSourceIndex, ioDataGridIndex->GetInt());
		}
	}
	// Cas des blocs avec des VarKey numeriques
	else
	{
		// On prend les bons types de blocs de cles
		sourceIndexedNKeyBlock = cast(const KWIndexedNKeyBlock*, sourceIndexedKeyBlock);

		// Memorisation de l'index associe a chaque VarKey
		for (nIndex = 0; nIndex < usedDataGridBlock->GetDataGridNumber(); nIndex++)
		{
			nVarKey = usedDataGridBlock->GetContinuousVarKeyAt(nIndex);
			ioDataGridIndex = new IntObject;
			ioDataGridIndex->SetInt(nIndex);
			nkdVarKeyDataGridIndexes.SetAt((NUMERIC)(longint)nVarKey, ioDataGridIndex);
		}

		// Parcours des cles du bloc source
		for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
		{
			nVarKey = sourceIndexedNKeyBlock->GetKeyAt(nSourceIndex);

			// Memorisation de l'index de la grille correspondante, -1 si non trouvee
			ioDataGridIndex = cast(IntObject*, nkdVarKeyDataGridIndexes.Lookup((NUMERIC)(longint)nVarKey));
			if (ioDataGridIndex == NULL)
				ivUsedRecodingDataGridIndexes.SetAt(nSourceIndex, -1);
			else
				ivUsedRecodingDataGridIndexes.SetAt(nSourceIndex, ioDataGridIndex->GetInt());
		}
	}
	assert(ComputeUsedIndexNumber(&ivUsedRecodingDataGridIndexes) <= oaAllRecodingDataGrids.GetSize());

	// Nettoyage des index de grilles utilisee pour associer les VarKeys aux index des grilles dans le DataGridBlock
	nkdVarKeyDataGridIndexes.DeleteAll();

	// Affichage
	if (bDisplay)
	{
		// Affichage
		cout << "Optimize " << GetName() << endl;
		cout << "\tDomain: " << GetOwnerClass()->GetDomain()->GetName() << " " << GetOwnerClass()->GetDomain()
		     << endl;
		cout << "\tDictionary: " << GetOwnerClass()->GetName() << " " << GetOwnerClass() << " ("
		     << GetOwnerClass()->GetFreshness() << ", " << GetOwnerClass()->GetFreshness() << ")" << endl;
		cout << "\tSource block: " << *sourceIndexedKeyBlock;
		cout << "\tRecoding data grids: " << oaAllRecodingDataGrids.GetSize() << endl;
		cout << "\tUsed recoding data grids: " << ComputeUsedIndexNumber(&ivUsedRecodingDataGridIndexes)
		     << endl;
	}
}

boolean KWDRDataGridBlockRule::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

KWContinuousValueBlock* KWDRDataGridBlockRule::BuildRecodedBlock(const KWValueBlock* sourceValueBlock,
								 int nSourceBlockType,
								 IntVector* ivTargetRecodedValueIndexes) const
{
	KWContinuousValueBlock* valueBlock;
	const KWContinuousValueBlock* sourceContinuousValueBlock;
	const KWSymbolValueBlock* sourceSymbolValueBlock;
	int nNewBlockSize;
	int i;
	int nRecodedValueIndex;
	int nUsedDataGridIndex;
	const KWDRDataGrid* datagridRule;
	int nRecodedValue;

	require(IsCompiled());
	require(sourceValueBlock != NULL);
	require(nSourceBlockType == KWType::ContinuousValueBlock or nSourceBlockType == KWType::SymbolValueBlock);
	require(ivTargetRecodedValueIndexes->GetSize() == ivUsedRecodingDataGridIndexes.GetSize());
	require(sourceValueBlock != NULL);
	require(sourceValueBlock->GetValueNumber() == 0 or
		sourceValueBlock->GetAttributeSparseIndexAt(sourceValueBlock->GetValueNumber() - 1) <
		    ivTargetRecodedValueIndexes->GetSize());

	// Calcul de la taille du nouveau bloc en comptant le nombre de valeurs a transferer
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		assert(i == 0 or sourceValueBlock->GetAttributeSparseIndexAt(i) >
				     sourceValueBlock->GetAttributeSparseIndexAt(i - 1));
		if (ivTargetRecodedValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i)) >= 0)
			nNewBlockSize++;
	}

	// Creation du nouveau bloc
	valueBlock = KWContinuousValueBlock::NewValueBlock(nNewBlockSize);

	// Cast system du bloc source selon son type
	// On ne peut ici utiliser l'operateur cast de Norm, car les ValueBloc sont des SystemObject, et non des Object
	sourceContinuousValueBlock = NULL;
	sourceSymbolValueBlock = NULL;
	if (nSourceBlockType == KWType::ContinuousValueBlock)
		sourceContinuousValueBlock = (const KWContinuousValueBlock*)sourceValueBlock;
	else
		sourceSymbolValueBlock = (const KWSymbolValueBlock*)sourceValueBlock;

	// Recopie de son contenu apres recodage
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		// Index sparse de la valeur recodee
		nRecodedValueIndex = ivTargetRecodedValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i));

		// Memorisation de la paire (index, valeur) si valide
		if (nRecodedValueIndex >= 0)
		{
			// Grille de recodage
			nUsedDataGridIndex =
			    ivUsedRecodingDataGridIndexes.GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i));
			datagridRule = cast(const KWDRDataGrid*, oaAllRecodingDataGrids.GetAt(nUsedDataGridIndex));
			check(datagridRule);

			// Calcul de la valeur recodee selon le type de block
			if (nSourceBlockType == KWType::ContinuousValueBlock)
				nRecodedValue =
				    ComputeContinuousCellIndex(datagridRule, sourceContinuousValueBlock->GetValueAt(i));
			else
				nRecodedValue =
				    ComputeSymbolCellIndex(datagridRule, sourceSymbolValueBlock->GetValueAt(i));

			// Memorisation dans le bloc des valeurs recodees
			valueBlock->SetAttributeSparseIndexAt(nNewBlockSize, nRecodedValueIndex);
			valueBlock->SetValueAt(nNewBlockSize, nRecodedValue);
			assert(nNewBlockSize == 0 or valueBlock->GetAttributeSparseIndexAt(nNewBlockSize) >
							 valueBlock->GetAttributeSparseIndexAt(nNewBlockSize - 1));
			nNewBlockSize++;
		}
	}
	return valueBlock;
}

int KWDRDataGridBlockRule::ComputeContinuousCellIndex(const KWDRDataGrid* dataGridRule, Continuous cValue) const
{
	int nPartIndex;

	require(dataGridRule != NULL);
	require(dataGridRule->IsCompiled());
	require(dataGridRule->GetAttributeNumber() == 2);
	require(dataGridRule->GetAttributeTypeAt(0) == KWType::Continuous);

	nPartIndex = dataGridRule->GetContinuousAttributePartIndexAt(0, cValue) + 1;
	return nPartIndex;
}

int KWDRDataGridBlockRule::ComputeSymbolCellIndex(const KWDRDataGrid* dataGridRule, Symbol sValue) const
{
	int nPartIndex;

	require(dataGridRule != NULL);
	require(dataGridRule->IsCompiled());
	require(dataGridRule->GetAttributeNumber() == 2);
	require(dataGridRule->GetAttributeTypeAt(0) == KWType::Symbol);

	nPartIndex = dataGridRule->GetSymbolAttributePartIndexAt(0, sValue) + 1;
	return nPartIndex;
}

int KWDRDataGridBlockRule::ComputeUsedIndexNumber(IntVector* ivIndexes) const
{
	int nUsedIndexNumber;
	int i;

	require(ivIndexes != NULL);

	nUsedIndexNumber = 0;
	for (i = 0; i < ivIndexes->GetSize(); i++)
	{
		assert(ivIndexes->GetAt(i) >= -1);
		if (ivIndexes->GetAt(i) != -1)
			nUsedIndexNumber++;
	}
	return nUsedIndexNumber;
}

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndexBlock

KWDRCellIndexBlock::KWDRCellIndexBlock()
{
	SetName("CellIndexBlock");
	SetLabel("Cell index block");
	SetType(KWType::ContinuousValueBlock);
	nDynamicCompileFreshness = 0;
}

KWDRCellIndexBlock::~KWDRCellIndexBlock() {}

KWDerivationRule* KWDRCellIndexBlock::Create() const
{
	return new KWDRCellIndexBlock;
}

int KWDRCellIndexBlock::GetVarKeyType() const
{
	// On renvoie le type de VarKey du DataGridBlock en premier operande
	return GetDataGridVarKeyType();
}

Continuous KWDRCellIndexBlock::GetValueBlockContinuousDefaultValue() const
{
	// L'index de recodage est -1 pour les valeurs manquantes, comme dans la regle CellIndexWithMissing
	return -1;
}

KWContinuousValueBlock*
KWDRCellIndexBlock::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						      const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbResult;

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Calcul du bloc recode selon le type de variable a recoder
	if (GetSecondOperand()->GetType() == KWType::ContinuousValueBlock)
		cvbResult = BuildRecodedBlock(GetSecondOperand()->GetContinuousValueBlock(kwoObject),
					      KWType::ContinuousValueBlock, &ivRecodedValueIndexes);
	else
		cvbResult = BuildRecodedBlock(GetSecondOperand()->GetSymbolValueBlock(kwoObject),
					      KWType::SymbolValueBlock, &ivRecodedValueIndexes);
	return cvbResult;
}

boolean KWDRCellIndexBlock::CheckBlockAttributesAt(const KWClass* kwcOwnerClass, const KWAttributeBlock* attributeBlock,
						   int nOperandIndex) const
{
	boolean bOk;
	boolean bContinueCheck;
	KWDRDataGridBlock* referencedDataGridBlock;
	KWAttribute* checkedAttribute;
	int nIndex;
	Symbol sVarKey;
	int nVarKey;
	NumericKeyDictionary nkdDataGridVarKeys;
	boolean bVarKeyFound;
	ALString sExternalVarKey;

	// Appel de la methode ancetre
	bOk = KWDRDataGridBlockRule::CheckBlockAttributesAt(kwcOwnerClass, attributeBlock, nOperandIndex);

	// Specialisation pour le premier operande de type grille
	// A noter que tout n'est pas encore necessairement valdie a ce moment. Les verifications se font alors
	// en mode "unchecked", en ne cherchant pas a verifier ce qui est verifier par ailleurs
	if (bOk and nOperandIndex == 0)
	{
		bContinueCheck = true;

		// Acces au premier operande de type DataGridBlock, correct puisque l'on est passe par CheckDefinition
		referencedDataGridBlock = NULL;
		if (bContinueCheck)
		{
			referencedDataGridBlock =
			    cast(KWDRDataGridBlock*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

			// Verification de la validite pour continuer ou non la verification
			bContinueCheck = referencedDataGridBlock != NULL;
			bContinueCheck = bContinueCheck and referencedDataGridBlock->GetUncheckedDataGridNumber() > 0;
			bContinueCheck =
			    bContinueCheck and
			    GetDataGridVarKeyType() == referencedDataGridBlock->GetUncheckedDataGridVarKeyType();
			bContinueCheck = bContinueCheck and KWType::IsSimple(GetDataGridVarKeyType());
			bContinueCheck = bContinueCheck and attributeBlock->GetVarKeyType() == GetDataGridVarKeyType();
		}

		// Enregistrement des VarKeys des data grids du DataGridBlock, qu'ils soient valides ou non
		if (bContinueCheck)
		{
			// Cas des VarKey categoriels
			if (attributeBlock->GetVarKeyType() == KWType::Symbol)
			{
				for (nIndex = 0; nIndex < referencedDataGridBlock->GetUncheckedVarKeyNumber(); nIndex++)
				{
					sVarKey = referencedDataGridBlock->GetUncheckedSymbolVarKeyAt(nIndex);
					nkdDataGridVarKeys.SetAt(sVarKey.GetNumericKey(), &nkdDataGridVarKeys);
				}
			}
			// Cas des VarKey numeriques
			else
			{
				for (nIndex = 0; nIndex < referencedDataGridBlock->GetUncheckedVarKeyNumber(); nIndex++)
				{
					nVarKey = referencedDataGridBlock->GetUncheckedContinuousVarKeyAt(nIndex);
					nkdDataGridVarKeys.SetAt((NUMERIC)(longint)nVarKey, &nkdDataGridVarKeys);
				}
			}
		}

		// Verification que le VarKey de chaque attribut du bloc de recodage est trouve parmi les VarKey des
		// DataGrid de recodage
		if (bContinueCheck)
		{
			// Parcours des attributs du bloc a verifier
			checkedAttribute = attributeBlock->GetFirstAttribute();
			while (checkedAttribute != NULL)
			{
				// Recherche de l'attribut utilise dans le bloc si son VarKey coincide
				// avec celui d'une grille du DataGridBlock
				bVarKeyFound = false;
				if (attributeBlock->GetVarKeyType() == KWType::Symbol)
				{
					sVarKey = attributeBlock->GetSymbolVarKey(checkedAttribute);
					bVarKeyFound = nkdDataGridVarKeys.Lookup(sVarKey.GetNumericKey()) != NULL;
				}
				else
				{
					nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
					bVarKeyFound = nkdDataGridVarKeys.Lookup((NUMERIC)(longint)nVarKey) != NULL;
				}

				// Erreur si la VarKey de grille n'est pas trouvee
				if (not bVarKeyFound)
				{
					// Preparation des informations sur la VarKey et la classe de scope
					sExternalVarKey = attributeBlock->GetStringVarKey(checkedAttribute);

					// Messages d'erreur
					attributeBlock->AddError("Variable " + checkedAttribute->GetName() +
								 +" not found with its VarKey=" + sExternalVarKey +
								 " in data grid block first operand of rule " +
								 GetName());
					bOk = false;
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
	return bOk;
}

longint KWDRCellIndexBlock::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDRDataGridBlockRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRCellIndexBlock) - sizeof(KWDRDataGridBlockRule);
	lUsedMemory += ivRecodedValueIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

void KWDRCellIndexBlock::DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const
{
	boolean bDisplay = false;
	KWAttributeBlock* originAttributeBlock;
	const KWIndexedKeyBlock* sourceIndexedKeyBlock;
	const KWIndexedNKeyBlock* indexedNKeyBlock;
	const KWIndexedNKeyBlock* sourceIndexedNKeyBlock;
	const KWIndexedCKeyBlock* indexedCKeyBlock;
	const KWIndexedCKeyBlock* sourceIndexedCKeyBlock;
	int nSourceIndex;
	int nIndex;
	Symbol sVarKey;
	int nVarKey;

	require(indexedKeyBlock != NULL);
	require(IsCompiled());
	require(KWType::IsValueBlock(GetType()));
	require(GetOperandAt(1)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Recherche du bloc d'attribut source
		originAttributeBlock = GetOperandAt(1)->GetOriginAttributeBlock();

		// Recherche du bloc de cle source
		sourceIndexedKeyBlock = originAttributeBlock->GetLoadedAttributesIndexedKeyBlock();
		assert(sourceIndexedKeyBlock->GetVarKeyType() == indexedKeyBlock->GetVarKeyType());

		// Cas des blocs avec des VarKey categoriels
		if (indexedKeyBlock->GetVarKeyType() == KWType::Symbol)
		{
			// On prend les bons types de blocs de cles
			indexedCKeyBlock = cast(const KWIndexedCKeyBlock*, indexedKeyBlock);
			sourceIndexedCKeyBlock = cast(const KWIndexedCKeyBlock*, sourceIndexedKeyBlock);

			// Parcours des cles du bloc source
			ivRecodedValueIndexes.SetSize(sourceIndexedKeyBlock->GetKeyNumber());
			for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
			{
				sVarKey = sourceIndexedCKeyBlock->GetKeyAt(nSourceIndex);

				// Recherche de l'index dans le bloc cible
				nIndex = indexedCKeyBlock->GetKeyIndex(sVarKey);

				// Memorisation du nouvel index
				ivRecodedValueIndexes.SetAt(nSourceIndex, nIndex);
			}
		}
		// Cas des blocs avec des VarKey numeriques
		else
		{
			// On prend les bons types de blocs de cles
			indexedNKeyBlock = cast(const KWIndexedNKeyBlock*, indexedKeyBlock);
			sourceIndexedNKeyBlock = cast(const KWIndexedNKeyBlock*, sourceIndexedKeyBlock);

			// Parcours des cles du bloc source
			ivRecodedValueIndexes.SetSize(sourceIndexedKeyBlock->GetKeyNumber());
			for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
			{
				nVarKey = sourceIndexedNKeyBlock->GetKeyAt(nSourceIndex);

				// Recherche de l'index dans le bloc cible
				nIndex = indexedNKeyBlock->GetKeyIndex(nVarKey);

				// Memorisation du nouvel index
				ivRecodedValueIndexes.SetAt(nSourceIndex, nIndex);
			}
		}

		// Affichage
		if (bDisplay)
		{
			cout << GetClassName() << " " << GetName() << endl;
			cout << "\tSource block: " << *sourceIndexedKeyBlock;
			cout << "\tTarget block: " << *indexedKeyBlock;
			cout << "\tRecoded value indexes: " << ivRecodedValueIndexes << endl;
			cout << "\tRecoding data grid indexes: " << ivUsedRecodingDataGridIndexes << endl;
		}

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStatsBlock

KWDRDataGridStatsBlock::KWDRDataGridStatsBlock()
{
	SetName("DataGridStatsBlock");
	SetLabel("Data grid stats block");
	SetType(KWType::Structure);
	SetStructureName("DataGridStatsBlock");
	resultCellIndexBlock = NULL;
}

KWDRDataGridStatsBlock::~KWDRDataGridStatsBlock()
{
	if (resultCellIndexBlock != NULL)
		delete resultCellIndexBlock;
	CleanAllDataGridStatsRules();
}

KWDerivationRule* KWDRDataGridStatsBlock::Create() const
{
	return new KWDRDataGridStatsBlock;
}

Object* KWDRDataGridStatsBlock::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());

	// Nettoyage prelable du precedent resultat
	if (resultCellIndexBlock != NULL)
		delete resultCellIndexBlock;

	// Calcul du bloc recode selon le type de variable a recoder, en indiquant qu'il faut recoder les valeurs
	// correspondant a toutes les grilles
	assert(ComputeUsedIndexNumber(&ivUsedRecodingDataGridIndexes) == oaAllRecodingDataGrids.GetSize());
	if (GetSecondOperand()->GetType() == KWType::ContinuousValueBlock)
		resultCellIndexBlock = BuildRecodedBlock(GetSecondOperand()->GetContinuousValueBlock(kwoObject),
							 KWType::ContinuousValueBlock, &ivUsedRecodingDataGridIndexes);
	else
		resultCellIndexBlock = BuildRecodedBlock(GetSecondOperand()->GetSymbolValueBlock(kwoObject),
							 KWType::SymbolValueBlock, &ivUsedRecodingDataGridIndexes);

	// On retourne la structure elle meme, pour disposer de ses services
	return (Object*)this;
}

longint KWDRDataGridStatsBlock::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDRDataGridBlockRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridStatsBlock) - sizeof(KWDRDataGridBlockRule);
	lUsedMemory += oaAllDataGridStatsRules.GetOverallUsedMemory() - sizeof(ObjectArray);
	return lUsedMemory;
}

void KWDRDataGridStatsBlock::Optimize(KWClass* kwcOwnerClass)
{
	int i;
	KWDRDataGridStats* dataGridStats;
	int nSourceValueType;

	// Appel de la methode ancetre
	KWDRDataGridBlockRule::Optimize(kwcOwnerClass);

	// Recherche du type de variable source d'apres le type de bloc de valeur en second operande
	nSourceValueType = KWType::GetBlockBaseType(GetSecondOperand()->GetType());

	// Creation d'une regle DataGridStats par grille du DataGridBlock
	CleanAllDataGridStatsRules();
	oaAllDataGridStatsRules.SetSize(GetDataGridBlock()->GetDataGridNumber());
	for (i = 0; i < oaAllDataGridStatsRules.GetSize(); i++)
	{
		// Creation et memorisation d'une regle DataGridsStats
		dataGridStats = new KWDRDataGridStats;
		oaAllDataGridStatsRules.SetAt(i, dataGridStats);

		// Parametregae de la grille en premier operande
		dataGridStats->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		dataGridStats->GetFirstOperand()->SetDerivationRule(GetDataGridBlock()->GetDataGridAt(i));

		// Parametrage du type de deuxieme operande
		// Ce deuxieme operande ne sera pas utilise en pratique, mais on l'initialise correctement
		// pour pouvoir compiler la regle
		dataGridStats->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		dataGridStats->GetSecondOperand()->SetType(nSourceValueType);

		// Fianlsaition de la specification de la regle
		dataGridStats->CompleteTypeInfo(kwcOwnerClass);

		// Compilation de la regle
		assert(dataGridStats->CheckCompleteness(kwcOwnerClass));
		dataGridStats->Compile(kwcOwnerClass);
	}
}

void KWDRDataGridStatsBlock::CleanAllDataGridStatsRules()
{
	int i;
	KWDRDataGridStats* dataGridStats;

	// On dereference prealablement les regles de type DataGrid en premier operande des DataGridStats
	// pour quelques ne soit pas detruites une deuxieme fois par ces regles
	for (i = 0; i < oaAllDataGridStatsRules.GetSize(); i++)
	{
		dataGridStats = cast(KWDRDataGridStats*, oaAllDataGridStatsRules.GetAt(i));
		dataGridStats->GetFirstOperand()->SetDerivationRule(NULL);
	}

	// On peyut maintenant detruire ces regles
	oaAllDataGridStatsRules.DeleteAll();
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStatsBlockTest

KWDRDataGridStatsBlockTest::KWDRDataGridStatsBlockTest()
{
	SetName("DataGridStatsBlockTest");
	SetLabel("Data grid stats block test");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGridStatsBlock");
}

KWDRDataGridStatsBlockTest::~KWDRDataGridStatsBlockTest() {}

KWDerivationRule* KWDRDataGridStatsBlockTest::Create() const
{
	return new KWDRDataGridStatsBlockTest;
}

Symbol KWDRDataGridStatsBlockTest::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRDataGridStatsBlock* referenceDataGridStatsBlock;
	const KWDRDataGridStats* dataGridStats;
	ALString sDetailedResult;
	int nValueIndex;
	int nCellIndex;
	int nTarget;

	require(IsCompiled());

	// On evalue les operandes
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Recherche du bloc en premier operande
	referenceDataGridStatsBlock =
	    cast(KWDRDataGridStatsBlock*, GetFirstOperand()->GetReferencedDerivationRule(GetOwnerClass()));
	check(referenceDataGridStatsBlock);

	// Exploitation de ce resultats pour fabriquer un resultat en sortie
	for (nValueIndex = 0; nValueIndex < referenceDataGridStatsBlock->GetCellIndexBlockSize(); nValueIndex++)
	{
		if (nValueIndex > 0)
			sDetailedResult += " ";

		// VarKey de la grille recodee
		if (referenceDataGridStatsBlock->GetDataGridVarKeyType() == KWType::Continuous)
			sDetailedResult += IntToString(referenceDataGridStatsBlock->GetContinuousVarKeyAt(nValueIndex));
		else
			sDetailedResult += referenceDataGridStatsBlock->GetSymbolVarKeyAt(nValueIndex).GetValue();
		sDetailedResult += ":";

		// Index de recodage dans la grille
		nCellIndex = referenceDataGridStatsBlock->GetCellIndexAt(nValueIndex);
		sDetailedResult += IntToString(nCellIndex);

		// Vecteur des probabilites conditionnelles
		dataGridStats = referenceDataGridStatsBlock->GetDataGridStatsAt(nValueIndex);
		sDetailedResult += "(";
		for (nTarget = 0; nTarget < dataGridStats->GetDataGridTargetCellNumber(); nTarget++)
		{
			if (nTarget > 0)
				sDetailedResult += ",";

			// Attention, on a besoin ici d'un index interne demarant a 0 (et non a 1 comme pour les index
			// externes)
			sDetailedResult += KWContinuous::ContinuousToString(
			    dataGridStats->GetDataGridSourceConditionalLogProbAt(nCellIndex - 1, nTarget));
		}
		sDetailedResult += ")";
	}
	return Symbol(sDetailedResult);
}