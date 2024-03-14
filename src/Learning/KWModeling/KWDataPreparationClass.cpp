// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPreparationClass.h"
#include "KWLearningReport.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationClass

KWDataPreparationClass::KWDataPreparationClass()
{
	kwcDataPreparationClass = NULL;
	kwcdDataPreparationDomain = NULL;
	dataPreparationTargetAttribute = NULL;
}

KWDataPreparationClass::~KWDataPreparationClass()
{
	DeleteDataPreparation();
}

void KWDataPreparationClass::ComputeDataPreparationFromClassStats(KWClassStats* classStats)
{
	const ALString sLevelMetaDataKey = KWDataPreparationAttribute::GetLevelMetaDataKey();
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDataPreparationStats* preparedStats;
	KWAttribute* nativeAttribute;
	int i;

	require(Check());
	require(classStats != NULL);
	require(classStats->Check());
	require(GetLearningSpec() == classStats->GetLearningSpec());
	require(classStats->IsStatsComputed());
	require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());

	// Nettoyage des specifications de preparation
	DeleteDataPreparation();

	// Duplication de la classe
	kwcdDataPreparationDomain = GetClass()->GetDomain()->CloneFromClass(GetClass());
	kwcDataPreparationClass = kwcdDataPreparationDomain->LookupClass(GetClass()->GetName());

	// Nettoyage des meta-donnees de Level
	kwcDataPreparationClass->RemoveAllAttributesMetaDataKey(sLevelMetaDataKey);

	// Preparation de l'attribut cible
	if (GetTargetAttributeName() != "")
	{
		dataPreparationTargetAttribute = new KWDataPreparationTargetAttribute;
		dataPreparationTargetAttribute->InitFromAttributeValueStats(kwcDataPreparationClass,
									    GetTargetValueStats());
	}

	// Ajout d'attributs derives pour toute stats de preparation disponible (univarie, bivarie...)
	for (i = 0; i < classStats->GetAllPreparedStats()->GetSize(); i++)
	{
		preparedStats = cast(KWDataPreparationStats*, classStats->GetAllPreparedStats()->GetAt(i));

		// Meta-donne de Level sur l'attribut natif, uniquement dans le cas univarie
		if (preparedStats->GetTargetAttributeType() != KWType::None and
		    preparedStats->GetAttributeNumber() == 1)
		{
			// Recherche de l'attribute dans la classe de preparation
			nativeAttribute =
			    kwcDataPreparationClass->LookupAttribute(preparedStats->GetAttributeNameAt(0));

			// Parametrage de l'indication de Level
			nativeAttribute->GetMetaData()->SetDoubleValueAt(sLevelMetaDataKey, preparedStats->GetLevel());
		}

		// Ajout d'un attribut derive s'il existe une grille de donnees
		if (preparedStats->GetPreparedDataGridStats() != NULL)
		{
			// Memorisation des infos de preparation de l'attribut
			dataPreparationAttribute = new KWDataPreparationAttribute;
			dataPreparationAttribute->InitFromDataPreparationStats(kwcDataPreparationClass, preparedStats);
			oaDataPreparationAttributes.Add(dataPreparationAttribute);
			dataPreparationAttribute->SetIndex(oaDataPreparationAttributes.GetSize() - 1);
		}
	}

	ensure(CheckDataPreparation());
}

void KWDataPreparationClass::ComputeDataPreparationFromAttributeSubsetStats(
    KWClassStats* classStats, KWAttributeSubsetStats* attributeSubsetStats)
{
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(Check());
	require(classStats != NULL);
	require(classStats->Check());
	require(GetLearningSpec() == classStats->GetLearningSpec());
	require(classStats->IsStatsComputed());
	require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());
	require(attributeSubsetStats != NULL);

	// Nettoyage des specifications de preparation
	DeleteDataPreparation();

	// Duplication de la classe
	kwcdDataPreparationDomain = GetClass()->GetDomain()->CloneFromClass(GetClass());
	kwcDataPreparationClass = kwcdDataPreparationDomain->LookupClass(GetClass()->GetName());

	// Nettoyage des meta-donnees de Level
	kwcDataPreparationClass->RemoveAllAttributesMetaDataKey(KWDataPreparationAttribute::GetLevelMetaDataKey());

	// Preparation de l'attribut cible
	if (GetTargetAttributeName() != "")
	{
		dataPreparationTargetAttribute = new KWDataPreparationTargetAttribute;
		dataPreparationTargetAttribute->InitFromAttributeValueStats(kwcDataPreparationClass,
									    GetTargetValueStats());
	}

	// Ajout d'un attribut derive s'il existe une grille de donnees informative
	if (attributeSubsetStats->GetPreparedDataGridStats() != NULL and
	    (GetTargetAttributeName() == "" or
	     attributeSubsetStats->GetPreparedDataGridStats()->GetSourceAttributeNumber() > 0))
	{
		// Memorisation des infos de preparation de l'attribut
		dataPreparationAttribute = new KWDataPreparationAttribute;
		dataPreparationAttribute->InitFromDataPreparationStats(kwcDataPreparationClass, attributeSubsetStats);
		oaDataPreparationAttributes.Add(dataPreparationAttribute);
		dataPreparationAttribute->SetIndex(oaDataPreparationAttributes.GetSize() - 1);
	}

	ensure(CheckDataPreparation());
}

void KWDataPreparationClass::RemoveDataPreparation()
{
	kwcDataPreparationClass = NULL;
	kwcdDataPreparationDomain = NULL;
	if (dataPreparationTargetAttribute != NULL)
		delete dataPreparationTargetAttribute;
	dataPreparationTargetAttribute = NULL;
	oaDataPreparationAttributes.DeleteAll();
}

void KWDataPreparationClass::DeleteDataPreparation()
{
	if (kwcdDataPreparationDomain != NULL)
	{
		// La classe de prediction doit etre dans son propre domaine, non enregistre dans l'ensemble des
		// domaines
		assert(KWClassDomain::GetCurrentDomain() != kwcdDataPreparationDomain);
		assert(KWClassDomain::LookupDomain(kwcdDataPreparationDomain->GetName()) != kwcdDataPreparationDomain);

		// Destruction de toutes les classes du domaine, puis du domaine lui meme
		kwcdDataPreparationDomain->DeleteAllClasses();
		delete kwcdDataPreparationDomain;
	}
	RemoveDataPreparation();
}

KWClass* KWDataPreparationClass::GetDataPreparationClass()
{
	// Verification a minima que la preparation a ete effectue
	require(kwcDataPreparationClass != NULL);
	require(kwcdDataPreparationDomain != NULL);
	require(kwcDataPreparationClass->GetDomain() == kwcdDataPreparationDomain);
	return kwcDataPreparationClass;
}

KWClassDomain* KWDataPreparationClass::GetDataPreparationDomain()
{
	// Verification a minima que la preparation a ete effectue
	require(kwcDataPreparationClass != NULL);
	require(kwcdDataPreparationDomain != NULL);
	require(kwcDataPreparationClass->GetDomain() == kwcdDataPreparationDomain);
	return kwcdDataPreparationDomain;
}

KWDataPreparationTargetAttribute* KWDataPreparationClass::GetDataPreparationTargetAttribute()
{
	// Verification a minima que la preparation a ete effectue
	require(kwcDataPreparationClass != NULL);
	return dataPreparationTargetAttribute;
}

ObjectArray* KWDataPreparationClass::GetDataPreparationAttributes()
{
	// Verification a minima que la preparation a ete effectue
	require(kwcDataPreparationClass != NULL);
	return &oaDataPreparationAttributes;
}

KWAttribute* KWDataPreparationClass::AddDataGridBlock(ObjectArray* oaDataGridBlockDataPreparationAttributes,
						      const ALString& sPrefix)
{
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWAttribute* nativeAttribute;
	KWAttributeBlock* firstNativeAttributeBlock;
	KWDRDataGridBlock* dataGridBlock;
	KWDRContinuousValueSet* continuousValueSetRule;
	KWDRSymbolValueSet* symbolValueSetRule;
	KWAttribute* dataGridBlockAttribute;
	int nAttribute;
	KWAttribute* preparedAttribute;
	KWDerivationRuleOperand* preparedAttributeOperand;

	require(oaDataGridBlockDataPreparationAttributes != NULL);
	require(oaDataGridBlockDataPreparationAttributes->GetSize() > 0);

	// Tri des attributs de preparation par VarKey des attributs natifs du bloc
	oaDataGridBlockDataPreparationAttributes->SetCompareFunction(KWDataPreparationAttributeCompareVarKey);
	oaDataGridBlockDataPreparationAttributes->Sort();

	// Obtention des informations du bloc
	dataPreparationAttribute =
	    cast(KWDataPreparationAttribute*, oaDataGridBlockDataPreparationAttributes->GetAt(0));
	firstNativeAttributeBlock = dataPreparationAttribute->GetNativeAttribute()->GetAttributeBlock();
	assert(firstNativeAttributeBlock != NULL);

	// Creation de la regle data grid bloc
	dataGridBlock = new KWDRDataGridBlock;
	dataGridBlock->SetOperandNumber(1);

	// Initialisation de la liste de VarKeys (premier operand du DataGridBlock) en fonction du type des VarKeys du bloc
	continuousValueSetRule = NULL;
	symbolValueSetRule = NULL;
	if (firstNativeAttributeBlock->GetVarKeyType() == KWType::Continuous)
	{
		continuousValueSetRule = new KWDRContinuousValueSet;
		continuousValueSetRule->SetValueNumber(oaDataGridBlockDataPreparationAttributes->GetSize());
		dataGridBlock->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		dataGridBlock->GetFirstOperand()->SetDerivationRule(continuousValueSetRule);
	}
	else
	{
		assert(firstNativeAttributeBlock->GetVarKeyType() == KWType::Symbol);
		symbolValueSetRule = new KWDRSymbolValueSet;
		symbolValueSetRule->SetValueNumber(oaDataGridBlockDataPreparationAttributes->GetSize());
		dataGridBlock->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		dataGridBlock->GetFirstOperand()->SetDerivationRule(symbolValueSetRule);
	}

	// Creation et insertion de l'attribut conteneur de la structure DataGridBlock
	dataGridBlockAttribute = new KWAttribute;
	dataGridBlockAttribute->SetType(KWType::Structure);
	dataGridBlockAttribute->SetDerivationRule(dataGridBlock);
	dataGridBlockAttribute->SetName(
	    kwcDataPreparationClass->BuildAttributeName(sPrefix + firstNativeAttributeBlock->GetName()));
	dataGridBlockAttribute->SetUsed(false);
	kwcDataPreparationClass->InsertAttribute(dataGridBlockAttribute);

	// Completion du DataGridBlock et creation des attributs du CellIndexBlock
	for (nAttribute = 0; nAttribute < oaDataGridBlockDataPreparationAttributes->GetSize(); nAttribute++)
	{
		// Acces aux information de l'attribut courant
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataGridBlockDataPreparationAttributes->GetAt(nAttribute));
		nativeAttribute = dataPreparationAttribute->GetNativeAttribute();
		preparedAttribute = dataPreparationAttribute->GetPreparedAttribute();
		assert(nativeAttribute->GetAttributeBlock() == firstNativeAttributeBlock);

		// Creation et ajout de l'attribut prepare comme operand pour la regle du DataGridBlock
		preparedAttributeOperand = new KWDerivationRuleOperand;
		preparedAttributeOperand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		preparedAttributeOperand->SetAttributeName(preparedAttribute->GetName());
		dataGridBlock->AddOperand(preparedAttributeOperand);

		// Mise-a-jour de la liste de VarKeys dans le DataGridBlock et de la metadata de l'attribut CellIndex
		if (firstNativeAttributeBlock->GetVarKeyType() == KWType::Continuous)
		{
			assert(continuousValueSetRule != NULL);
			continuousValueSetRule->SetValueAt(
			    nAttribute, firstNativeAttributeBlock->GetContinuousVarKey(nativeAttribute));
		}
		else
		{
			assert(symbolValueSetRule != NULL);
			assert(firstNativeAttributeBlock->GetVarKeyType() == KWType::Symbol);
			symbolValueSetRule->SetValueAt(nAttribute,
						       firstNativeAttributeBlock->GetSymbolVarKey(nativeAttribute));
		}
	}
	dataGridBlockAttribute->CompleteTypeInfo(kwcDataPreparationClass);

	ensure(kwcDataPreparationClass->Check());
	return dataGridBlockAttribute;
}

KWAttributeBlock*
KWDataPreparationClass::AddPreparedIndexingAttributeBlock(KWAttribute* dataGridBlockAttribute,
							  ObjectArray* oaDataGridBlockDataPreparationAttributes)
{
	KWDRCellIndexBlock* cellIndexBlockRule;
	KWAttributeBlock* cellIndexBlock;
	KWAttributeBlock* nativeAttributeBlock;
	KWAttribute* nativeAttribute;
	KWAttribute* firstCellIndexAttribute;
	KWAttribute* cellIndexAttribute;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	ALString sLevelMetaDataKey;

	require(oaDataGridBlockDataPreparationAttributes != NULL);
	require(oaDataGridBlockDataPreparationAttributes->GetSize() > 0);

	// Tri des attributs de preparation par VarKey des attributs natifs du bloc
	oaDataGridBlockDataPreparationAttributes->SetCompareFunction(KWDataPreparationAttributeCompareVarKey);
	oaDataGridBlockDataPreparationAttributes->Sort();

	// Obtention des informations du bloc
	dataPreparationAttribute =
	    cast(KWDataPreparationAttribute*, oaDataGridBlockDataPreparationAttributes->GetAt(0));
	nativeAttributeBlock = dataPreparationAttribute->GetNativeAttribute()->GetAttributeBlock();
	assert(nativeAttributeBlock != NULL);
	assert(kwcDataPreparationClass->LookupAttributeBlock(nativeAttributeBlock->GetName()) != NULL);

	// Completion du DataGridBlock et creation des attributs du CellIndexBlock
	firstCellIndexAttribute = NULL;
	cellIndexAttribute = NULL;
	for (nAttribute = 0; nAttribute < oaDataGridBlockDataPreparationAttributes->GetSize(); nAttribute++)
	{
		// Acces aux information de l'attribut courant
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataGridBlockDataPreparationAttributes->GetAt(nAttribute));
		assert(dataPreparationAttribute->GetNativeAttributeNumber() == 1);
		nativeAttribute = dataPreparationAttribute->GetNativeAttribute();
		assert(nativeAttribute->GetAttributeBlock() == nativeAttributeBlock);

		// Creation et ajout de l'attribut numerique pour l'index de attribut prepare
		cellIndexAttribute = new KWAttribute;
		cellIndexAttribute->SetName(kwcDataPreparationClass->BuildAttributeName(
		    "Index" + dataPreparationAttribute->GetPreparedAttribute()->GetName()));
		cellIndexAttribute->SetType(KWType::Continuous);
		sLevelMetaDataKey = dataPreparationAttribute->GetLevelMetaDataKey();
		cellIndexAttribute->GetMetaData()->SetDoubleValueAt(
		    sLevelMetaDataKey, nativeAttribute->GetMetaData()->GetDoubleValueAt(sLevelMetaDataKey));
		kwcDataPreparationClass->InsertAttribute(cellIndexAttribute);

		// Mise-a-jour de la liste de VarKeys dans le DataGridBlock et de la metadata de l'attribut CellIndex
		if (nativeAttributeBlock->GetVarKeyType() == KWType::Continuous)
		{
			cellIndexAttribute->GetMetaData()->SetDoubleValueAt(
			    "VarKey", nativeAttributeBlock->GetContinuousVarKey(nativeAttribute));
		}
		else
		{
			assert(nativeAttributeBlock->GetVarKeyType() == KWType::Symbol);
			cellIndexAttribute->GetMetaData()->SetStringValueAt(
			    "VarKey", nativeAttributeBlock->GetSymbolVarKey(nativeAttribute).GetValue());
		}

		if (nAttribute == 0)
			firstCellIndexAttribute = cellIndexAttribute;
	}

	// Creation de la regle pour le bloc de CellIndex
	cellIndexBlockRule = new KWDRCellIndexBlock;
	cellIndexBlockRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	cellIndexBlockRule->GetFirstOperand()->SetAttributeName(dataGridBlockAttribute->GetName());
	cellIndexBlockRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	cellIndexBlockRule->GetSecondOperand()->SetAttributeBlockName(nativeAttributeBlock->GetName());

	// Creation du bloc de CellIndex dans la classe de preparation
	assert(firstCellIndexAttribute != NULL);
	assert(cellIndexAttribute != NULL);

	cellIndexBlock = kwcDataPreparationClass->CreateAttributeBlock(
	    kwcDataPreparationClass->BuildAttributeBlockName("IndexesTmpPB" + nativeAttributeBlock->GetName()),
	    firstCellIndexAttribute, cellIndexAttribute);
	cellIndexBlock->SetDerivationRule(cellIndexBlockRule);
	cellIndexBlockRule->CompleteTypeInfo(kwcDataPreparationClass);

	return cellIndexBlock;
}

boolean KWDataPreparationClass::CheckDataPreparation() const
{
	boolean bOk = false;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWAttribute* nativeAttribute;
	KWAttribute* preparedAttribute;
	int nAttribute;
	int nNative;

	// Les specifications doivent etre correctes
	bOk = Check();

	// Test de presence
	if (bOk and kwcDataPreparationClass == NULL)
	{
		bOk = false;
		AddError("Data not prepared");
	}

	// Test de validite du domaine de classe
	if (bOk)
	{
		assert(kwcdDataPreparationDomain != NULL);
		assert(kwcDataPreparationClass->GetDomain() == kwcdDataPreparationDomain);
		bOk = kwcdDataPreparationDomain->Check();
	}

	// Test d'integrite de l'attribut de preparation cible
	assert(GetTargetAttributeName() != "" or dataPreparationTargetAttribute == NULL);
	if (bOk and GetTargetAttributeName() != "")
	{
		if (dataPreparationTargetAttribute == NULL)
		{
			bOk = false;
			AddError("Target variable " + GetTargetAttributeName() + " is not prepared");
		}
		else if (not dataPreparationTargetAttribute->Check())
			bOk = false;
		else if (kwcDataPreparationClass->LookupAttribute(GetTargetAttributeName()) == NULL)
		{
			bOk = false;
			AddError("Target variable " + GetTargetAttributeName() +
				 " missing in the preparation dictionary");
		}
		assert(not bOk or
		       dataPreparationTargetAttribute->GetNativeAttribute()->GetName() == GetTargetAttributeName());
	}

	// Test d'integrite des attributs de preparation sources
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < oaDataPreparationAttributes.GetSize(); nAttribute++)
		{
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, oaDataPreparationAttributes.GetAt(nAttribute));
			assert(dataPreparationAttribute->GetIndex() == nAttribute);

			// Erreur si non valide
			if (not dataPreparationAttribute->Check())
				bOk = false;

			// Test d'existence de l'attribut natif dans la classe de preparation
			if (bOk)
			{
				for (nNative = 0; nNative < dataPreparationAttribute->GetNativeAttributeNumber();
				     nNative++)
				{
					nativeAttribute = dataPreparationAttribute->GetNativeAttributeAt(nNative);
					if (kwcDataPreparationClass->LookupAttribute(nativeAttribute->GetName()) ==
					    NULL)
					{
						bOk = false;
						AddError("Native variable " + nativeAttribute->GetName() +
							 " unknown in the preparation dictionary");
					}
				}
			}

			// Test d'existence de l'attribut prepare dans la classe de preparation
			if (bOk)
			{
				preparedAttribute = dataPreparationAttribute->GetPreparedAttribute();
				if (kwcDataPreparationClass->LookupAttribute(preparedAttribute->GetName()) == NULL)
				{
					bOk = false;
					AddError("Prepared variable " + preparedAttribute->GetName() +
						 " unknown in the preparation dictionary");
				}
			}

			// Arret si erreurs
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void KWDataPreparationClass::Write(ostream& ost) const
{
	KWDataPreparationAttribute* dataPreparationAttribute;
	int i;

	require(CheckDataPreparation());

	// En tete
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Dictionnaire
	ost << *kwcDataPreparationClass << endl;

	// Affichage des details par attributs
	for (i = 0; i < oaDataPreparationAttributes.GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationAttributes.GetAt(i));
		ost << *dataPreparationAttribute;
	}
	ost << endl;
}

const ALString KWDataPreparationClass::GetClassLabel() const
{
	return "Data preparation";
}

const ALString KWDataPreparationClass::GetObjectLabel() const
{
	if (GetLearningSpec() == NULL)
		return "";
	else
		return GetLearningSpec()->GetObjectLabel();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationAttribute

KWDataPreparationAttribute::KWDataPreparationAttribute()
{
	nIndex = 0;
	preparedAttribute = NULL;
	preparedStats = NULL;
}

KWDataPreparationAttribute::~KWDataPreparationAttribute() {}

void KWDataPreparationAttribute::SetIndex(int nValue)
{
	require(nValue >= 0);
	nIndex = nValue;
}

int KWDataPreparationAttribute::GetIndex() const
{
	return nIndex;
}

void KWDataPreparationAttribute::InitFromDataPreparationStats(KWClass* kwcClass,
							      KWDataPreparationStats* preparationStats)
{
	const ALString sLevelMetaDataKey = GetLevelMetaDataKey();
	KWDRDataGrid* dgRule;
	KWDataGridStats* preparedDataGridStats;
	int nAttribute;
	KWAttribute* nativeAttribute;
	ALString sDataGridLevel;
	ALString sDataGridVariableNames;

	require(preparedStats == NULL);
	require(preparedAttribute == NULL);
	require(kwcClass != NULL);
	require(preparationStats != NULL);
	require(preparationStats->GetPreparedDataGridStats() != NULL);

	// Memorisation des infos de preparation de l'attribut
	preparedStats = preparationStats;
	preparedDataGridStats = preparationStats->GetPreparedDataGridStats();

	// Memorisation des attributs natifs en entree de la grille
	SetNativeAttributeNumber((preparationStats->GetAttributeNumber()));
	for (nAttribute = 0; nAttribute < preparationStats->GetAttributeNumber(); nAttribute++)
	{
		nativeAttribute = kwcClass->LookupAttribute(preparationStats->GetAttributeNameAt(nAttribute));
		check(nativeAttribute);
		SetNativeAttributeAt(nAttribute, nativeAttribute);
	}

	// Creation d'une regle DataGrid
	dgRule = new KWDRDataGrid;
	dgRule->ImportDataGridStats(preparedDataGridStats, false);

	// Creation d'un attribut DataGridStats
	preparedAttribute = new KWAttribute;
	preparedAttribute->SetName(kwcClass->BuildAttributeName("P" + ComputeNativeAttributeName()));
	preparedAttribute->SetDerivationRule(dgRule);

	// Ajout de l'attribut dans la classe
	preparedAttribute->SetName(kwcClass->BuildAttributeName(preparedAttribute->GetName()));
	kwcClass->InsertAttribute(preparedAttribute);
	preparedAttribute->CompleteTypeInfo(kwcClass);

	// On le met en unused (il s'agit d'une Structure(DataGrid))
	preparedAttribute->SetUsed(false);

	// Creation d'un libelle sur les variables de la grille
	sDataGridVariableNames = preparedDataGridStats->ExportVariableNames();

	// Ajout d'un libelle pour l'attribut de preparation
	preparedAttribute->SetLabel(sDataGridVariableNames);

	// Meta-donne de Level sur l'attribut de preparation, dans le cas supervise
	if (preparedStats->GetTargetAttributeType() != KWType::None)
		preparedAttribute->GetMetaData()->SetDoubleValueAt(sLevelMetaDataKey, preparedStats->GetLevel());
	ensure(Check());
}

const ALString& KWDataPreparationAttribute::GetLevelMetaDataKey()
{
	static const ALString sLevelMetaDataKey = "Level";
	return sLevelMetaDataKey;
}

void KWDataPreparationAttribute::Reset()
{
	nIndex = 0;
	preparedAttribute = NULL;
	preparedStats = NULL;
	oaNativeAttributes.SetSize(0);
}

void KWDataPreparationAttribute::SetNativeAttribute(KWAttribute* kwaNativeAttribute)
{
	require(GetNativeAttributeNumber() >= 1);
	oaNativeAttributes.SetAt(0, kwaNativeAttribute);
}

KWAttribute* KWDataPreparationAttribute::GetNativeAttribute()
{
	require(Check());
	require(GetNativeAttributeNumber() >= 1);
	return cast(KWAttribute*, oaNativeAttributes.GetAt(0));
}

void KWDataPreparationAttribute::SetPreparedAttribute(KWAttribute* kwaPreparedAttribute)
{
	preparedAttribute = kwaPreparedAttribute;
}

boolean KWDataPreparationAttribute::IsNativeAttributeInBlock()
{
	require(GetNativeAttributeNumber() >= 1);
	require(GetNativeAttributeAt(0) != NULL);

	if (GetNativeAttributeNumber() == 1)
		return GetNativeAttributeAt(0)->GetAttributeBlock() != NULL;
	else
		return false;
}

KWAttribute* KWDataPreparationAttribute::GetPreparedAttribute() const
{
	require(Check());
	return preparedAttribute;
}

void KWDataPreparationAttribute::SetPreparedStats(KWDataPreparationStats* kwdpsAttributeStats)
{
	preparedStats = kwdpsAttributeStats;
}

KWDataPreparationStats* KWDataPreparationAttribute::GetPreparedStats() const
{
	require(Check());
	return preparedStats;
}

bool KWDataPreparationAttribute::IsInformativeOnTarget() const
{
	require(GetPreparedStats() != NULL);
	require(GetPreparedStats()->GetTargetAttributeName() != "");
	require(GetPreparedStats()->GetPreparedDataGridStats() != NULL);

	return GetPreparedStats()->GetPreparedDataGridStats() != NULL and
	       GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize() > 1 and
	       GetPreparedStats()->GetSortValue() > 0;
}

void KWDataPreparationAttribute::SetNativeAttribute1(KWAttribute* kwaNativeAttribute)
{
	require(GetNativeAttributeNumber() >= 1);
	oaNativeAttributes.SetAt(0, kwaNativeAttribute);
}

KWAttribute* KWDataPreparationAttribute::GetNativeAttribute1()
{
	require(Check());
	require(GetNativeAttributeNumber() >= 1);
	return cast(KWAttribute*, oaNativeAttributes.GetAt(0));
}

void KWDataPreparationAttribute::SetNativeAttribute2(KWAttribute* kwaNativeAttribute)
{
	require(GetNativeAttributeNumber() >= 2);
	oaNativeAttributes.SetAt(1, kwaNativeAttribute);
}

KWAttribute* KWDataPreparationAttribute::GetNativeAttribute2()
{
	require(Check());
	require(GetNativeAttributeNumber() >= 2);
	return cast(KWAttribute*, oaNativeAttributes.GetAt(1));
}

void KWDataPreparationAttribute::SetNativeAttributeNumber(int nValue)
{
	require(nValue >= 0);
	oaNativeAttributes.SetSize(nValue);
}

int KWDataPreparationAttribute::GetNativeAttributeNumber() const
{
	return oaNativeAttributes.GetSize();
}

void KWDataPreparationAttribute::SetNativeAttributeAt(int nAttribute, KWAttribute* kwaNativeAttribute)
{
	require(0 <= nAttribute and nAttribute < GetNativeAttributeNumber());
	oaNativeAttributes.SetAt(nAttribute, kwaNativeAttribute);
}

KWAttribute* KWDataPreparationAttribute::GetNativeAttributeAt(int nAttribute)
{
	require(Check());
	require(0 <= nAttribute and nAttribute < GetNativeAttributeNumber());
	return cast(KWAttribute*, oaNativeAttributes.GetAt(nAttribute));
}

KWAttribute* KWDataPreparationAttribute::AddPreparedIndexingAttribute()
{
	KWDRCellIndex* cellIndexRule;

	cellIndexRule = new KWDRCellIndex;
	InitDataGridRule(cellIndexRule);
	return AddDataPreparationRuleAttribute(cellIndexRule, "Index");
}

KWAttribute* KWDataPreparationAttribute::AddPreparedIdAttribute()
{
	KWDRCellId* cellIdRule;

	cellIdRule = new KWDRCellId;
	InitDataGridRule(cellIdRule);
	return AddDataPreparationRuleAttribute(cellIdRule, "Id");
}

KWAttribute* KWDataPreparationAttribute::AddPreparedLabelAttribute()
{
	KWDRCellLabel* cellLabelRule;

	cellLabelRule = new KWDRCellLabel;
	InitDataGridRule(cellLabelRule);
	return AddDataPreparationRuleAttribute(cellLabelRule, "Label");
}

void KWDataPreparationAttribute::AddPreparedBinarizationAttributes(ObjectArray* oaAddedAttributes)
{
	KWAttribute* cellIndexAttribute;
	KWAttribute* binaryAttribute;
	KWDREQ* eqRule;
	const ALString sBinaryPrefix = "B";
	int nBinaryAttributeNumber;
	int i;

	// Creation d'un attribut intermediaire (Unused) pour le calcul de l'index
	cellIndexAttribute = AddPreparedIndexingAttribute();
	cellIndexAttribute->SetUsed(false);

	// Parcours des cellules sources la grille en supervise, ou cibles en non supervise
	if (GetPreparedStats()->GetTargetAttributeType() == KWType::None)
	{
		assert(GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize() <= 1);
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize();
	}
	else
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize();
	oaAddedAttributes->SetSize(0);
	for (i = 0; i < nBinaryAttributeNumber; i++)
	{
		// Creation d'une regle de calcul pour l'egalite a un des index
		eqRule = new KWDREQ;
		eqRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		eqRule->GetFirstOperand()->SetAttributeName(cellIndexAttribute->GetName());
		eqRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		eqRule->GetSecondOperand()->SetContinuousConstant((Continuous)(i + 1));

		// Creation de l'attribut de binarisation
		binaryAttribute = AddDataPreparationRuleAttribute(eqRule, sBinaryPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(binaryAttribute);
	}
}

void KWDataPreparationAttribute::AddPreparedSourceConditionalInfoAttributes(ObjectArray* oaAddedAttributes)
{
	KWDRDataGridStats* preparedStatsRule;
	KWAttribute* preparedStatsAttribute;
	KWAttribute* sourceConditionalInfoAttribute;
	KWDRSourceConditionalInfo* sourceConditionalInfoRule;
	const ALString sProbabilityPrefix = "Info";
	int i;

	// Creation d'un attribut intermediaire (Unused) pour le calcul de l'index
	preparedStatsRule = CreatePreparedStatsRule();
	preparedStatsAttribute = AddDataPreparationRuleAttribute(preparedStatsRule, "Stats");
	preparedStatsAttribute->SetUsed(false);

	// Parcours des cellules cibles la grille
	oaAddedAttributes->SetSize(0);
	for (i = 0; i < GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize(); i++)
	{
		// Creation d'une regle de calcul pour la difference a la moyenne
		sourceConditionalInfoRule = new KWDRSourceConditionalInfo;
		sourceConditionalInfoRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		sourceConditionalInfoRule->GetFirstOperand()->SetAttributeName(preparedStatsAttribute->GetName());
		sourceConditionalInfoRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		sourceConditionalInfoRule->GetSecondOperand()->SetContinuousConstant((Continuous)(i + 1));

		// Creation de l'attribut de probabilite conditionnelle
		sourceConditionalInfoAttribute =
		    AddDataPreparationRuleAttribute(sourceConditionalInfoRule, sProbabilityPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(sourceConditionalInfoAttribute);
	}
}

KWDRDataGridStats* KWDataPreparationAttribute::CreatePreparedStatsRule()
{
	KWDRDataGridStats* dataGridStatsRule;

	dataGridStatsRule = new KWDRDataGridStats;
	InitDataGridRule(dataGridStatsRule);
	return dataGridStatsRule;
}

KWAttribute* KWDataPreparationAttribute::AddPreparedCenterReducedAttribute()
{
	KWDRDivide* divideRule;
	KWDRDiff* diffRule;
	KWAttributeStats* attributeStats;
	KWDescriptiveContinuousStats* continuousStats;
	Continuous cStandardDeviation;

	require(GetNativeAttributeNumber() == 1);
	require(GetNativeAttribute()->GetType() == KWType::Continuous);

	// Acces aux statistiques descriptives de l'attribut
	attributeStats = cast(KWAttributeStats*, GetPreparedStats());
	continuousStats = cast(KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());

	// Creation d'une regle de calcul pour la difference a la moyenne
	diffRule = new KWDRDiff;
	diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	diffRule->GetFirstOperand()->SetAttributeName(GetNativeAttribute()->GetName());
	diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	diffRule->GetSecondOperand()->SetContinuousConstant((Continuous)continuousStats->GetMean());

	// Calcul de l'ecart type, en passant 1 si necessaire
	cStandardDeviation = continuousStats->GetStandardDeviation();
	if (cStandardDeviation != KWContinuous::GetMissingValue() and cStandardDeviation <= 0)
		cStandardDeviation = 1;

	// Creation d'une regle de calcul pour diviser par l'ecart type
	divideRule = new KWDRDivide;
	divideRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	divideRule->GetFirstOperand()->SetDerivationRule(diffRule);
	divideRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	divideRule->GetSecondOperand()->SetContinuousConstant(cStandardDeviation);

	// Creation de l'attribut centre reduit
	return AddDataPreparationRuleAttribute(divideRule, "CR");
}

KWAttribute* KWDataPreparationAttribute::AddPreparedNormalizedAttribute()
{
	KWDRDivide* divideRule;
	KWDRDiff* diffRule;
	KWAttributeStats* attributeStats;
	KWDescriptiveContinuousStats* continuousStats;
	Continuous cDelta;

	require(GetNativeAttributeNumber() == 1);
	require(GetNativeAttribute()->GetType() == KWType::Continuous);

	// Acces aux statistiques descriptives de l'attribut
	attributeStats = cast(KWAttributeStats*, GetPreparedStats());
	continuousStats = cast(KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());

	// Creation d'une regle de calcul pour la difference au minimum
	diffRule = new KWDRDiff;
	diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	diffRule->GetFirstOperand()->SetAttributeName(GetNativeAttribute()->GetName());
	diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	diffRule->GetSecondOperand()->SetContinuousConstant((Continuous)continuousStats->GetMin());

	// Calcul de l'amplitude, en passant 1 si necessaire
	if (continuousStats->GetMin() == KWContinuous::GetMissingValue())
		cDelta = KWContinuous::GetMissingValue();
	else
	{
		assert(continuousStats->GetMax() != KWContinuous::GetMissingValue());
		cDelta = continuousStats->GetMax() - continuousStats->GetMin();
		if (cDelta <= 0)
			cDelta = 1;
	}

	// Creation d'une regle de calcul pour diviser par l'amplitude
	divideRule = new KWDRDivide;
	divideRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	divideRule->GetFirstOperand()->SetDerivationRule(diffRule);
	divideRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	divideRule->GetSecondOperand()->SetContinuousConstant(cDelta);

	// Creation de l'attribut normalise
	return AddDataPreparationRuleAttribute(divideRule, "N01");
}

KWAttribute* KWDataPreparationAttribute::AddPreparedRankNormalizedAttribute()
{
	KWDRValueRank* valueRankRule;
	KWAttributeStats* attributeStats;
	KWDataGridStats* preparedDataGridStats;
	KWDataGridStats univariateDataGridStats;
	KWDRDataGrid* dgRule;

	require(GetNativeAttributeNumber() == 1);
	require(GetNativeAttribute()->GetType() == KWType::Continuous);

	// Acces aux statistiques descriptives de l'attribut
	attributeStats = cast(KWAttributeStats*, GetPreparedStats());

	// Acces a la grille de donnees
	preparedDataGridStats = attributeStats->GetPreparedDataGridStats();

	// Acces a la grille de donnees univariee correspondant a l'attribut natif
	assert(preparedDataGridStats->GetAttributeAt(0)->GetAttributeName() == GetNativeAttribute()->GetName());
	preparedDataGridStats->ExportAttributeDataGridStatsAt(0, &univariateDataGridStats);

	// Creation d'une regle DataGrid
	dgRule = new KWDRDataGrid;
	dgRule->ImportDataGridStats(&univariateDataGridStats, false);

	// Creation d'une regle de calcul pour normaliser par le rank
	valueRankRule = new KWDRValueRank;
	valueRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	valueRankRule->GetFirstOperand()->SetDerivationRule(dgRule);
	valueRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankRule->GetSecondOperand()->SetAttributeName(GetNativeAttribute()->GetName());

	// Creation de l'attribut normalise par son rang
	return AddDataPreparationRuleAttribute(valueRankRule, "NR");
}

const ALString KWDataPreparationAttribute::ComputeNativeAttributeName() const
{
	ALString sName;

	// Utilisation du nom de preparation
	sName = preparedStats->GetSortName();
	return sName;
}

boolean KWDataPreparationAttribute::Check() const
{
	return CheckSpecification(&oaNativeAttributes, preparedAttribute, preparedStats);
}

void KWDataPreparationAttribute::Write(ostream& ost) const
{
	require(Check());

	// Nom de l'attribut natif et de son attribut recode
	ost << "Variable\t" << ComputeNativeAttributeName() << "\t" << preparedAttribute->GetName() << "\n";

	// Grille de preparation
	if (preparedStats->GetPreparedDataGridStats() != NULL)
		ost << *(preparedStats->GetPreparedDataGridStats()) << "\n";
}

const ALString KWDataPreparationAttribute::GetClassLabel() const
{
	return "Variable preparation";
}

const ALString KWDataPreparationAttribute::GetObjectLabel() const
{
	return ComputeNativeAttributeName();
}

void KWDataPreparationAttribute::AddPreparedDistanceStudyAttributes(ObjectArray* oaAddedAttributes)
{
	KWAttribute* createdAttribute;

	require(GetDistanceStudyMode());

	// Nettoyage prealable
	oaAddedAttributes->RemoveAll();

	// Cas des variables Continuous
	if (GetNativeAttributeNumber() == 1 and GetNativeAttribute()->GetType() == KWType::Continuous)
	{
		createdAttribute = AddPreparedRankNormalizedAttribute();
		oaAddedAttributes->Add(createdAttribute);
		createdAttribute = AddPreparedRankNormalizedSelfDistanceAttribute();
		oaAddedAttributes->Add(createdAttribute);
	}
	// Cas des variables Symbol
	else if (GetNativeAttributeNumber() == 1 and GetNativeAttribute()->GetType() == KWType::Symbol)
	{
		AddPreparedCategoricalDistanceAttributes2(oaAddedAttributes);
	}
}

KWAttribute* KWDataPreparationAttribute::AddPreparedRankNormalizedSelfDistanceAttribute()
{
	KWDRValueRankSelfDistance* valueRankSelfDistanceRule;
	KWAttributeStats* attributeStats;
	KWDataGridStats* preparedDataGridStats;
	KWDataGridStats univariateDataGridStats;
	KWDRDataGrid* dgRule;

	require(GetDistanceStudyMode());
	require(GetNativeAttributeNumber() == 1);
	require(GetNativeAttribute()->GetType() == KWType::Continuous);

	// Acces aux statistiques descriptives de l'attribut
	attributeStats = cast(KWAttributeStats*, GetPreparedStats());

	// Acces a la grille de donnees
	preparedDataGridStats = attributeStats->GetPreparedDataGridStats();

	// Acces a la grille de donnees univariee correspondant a l'attribut natif
	assert(preparedDataGridStats->GetAttributeAt(0)->GetAttributeName() == GetNativeAttribute()->GetName());
	preparedDataGridStats->ExportAttributeDataGridStatsAt(0, &univariateDataGridStats);

	// Creation d'une regle DataGrid
	dgRule = new KWDRDataGrid;
	dgRule->ImportDataGridStats(&univariateDataGridStats, false);

	// Creation d'une regle de calcul pour normaliser par le rank
	valueRankSelfDistanceRule = new KWDRValueRankSelfDistance;
	valueRankSelfDistanceRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	valueRankSelfDistanceRule->GetFirstOperand()->SetDerivationRule(dgRule);
	valueRankSelfDistanceRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankSelfDistanceRule->GetSecondOperand()->SetAttributeName(GetNativeAttribute()->GetName());

	// Creation de l'attribut normalise par son rang
	return AddDataPreparationRuleAttribute(valueRankSelfDistanceRule, "SD");
}

void KWDataPreparationAttribute::AddPreparedCategoricalDistanceAttributes1(ObjectArray* oaAddedAttributes)
{
	KWAttribute* cellIndexAttribute;
	KWAttribute* binaryAttribute;
	KWDREQ* eqRule;
	const ALString sBinaryPrefix = "B";
	int nBinaryAttributeNumber;
	KWAttribute* distanceAttribute;
	KWAttribute* selfDistanceAttribute;
	KWDRSum* sumRule;
	KWDRProduct* productRule;
	KWDRProduct* productRule0;
	KWDRProduct* productRule1;
	KWDRDiff* diffRule;
	int nTotalFrequency;
	Continuous cValueProb;
	int i;

	require(GetDistanceStudyMode());

	// Creation d'un attribut intermediaire (Unused) pour le calcul de l'index
	cellIndexAttribute = AddPreparedIndexingAttribute();
	cellIndexAttribute->SetUsed(false);

	// Parcours des cellules sources la grille en supervise, ou cibles en non supervise
	if (GetPreparedStats()->GetTargetAttributeType() == KWType::None)
	{
		assert(GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize() <= 1);
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize();
	}
	else
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize();
	oaAddedAttributes->SetSize(0);
	nTotalFrequency = GetPreparedStats()->GetPreparedDataGridStats()->ComputeGridFrequency();
	for (i = 0; i < nBinaryAttributeNumber; i++)
	{
		// Creation d'une regle de calcul pour l'egalite a un des index
		eqRule = new KWDREQ;
		eqRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		eqRule->GetFirstOperand()->SetAttributeName(cellIndexAttribute->GetName());
		eqRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		eqRule->GetSecondOperand()->SetContinuousConstant((Continuous)(i + 1));

		// Creation de l'attribut de binarisation
		binaryAttribute = AddDataPreparationRuleAttribute(eqRule, sBinaryPrefix + IntToString(i + 1));
		binaryAttribute->SetUsed(false);

		// Probabilite de la valeurs
		cValueProb = GetPreparedStats()->GetPreparedDataGridStats()->GetUnivariateCellFrequencyAt(i);
		cValueProb /= nTotalFrequency;

		// Creation d'un attribut de distance
		productRule = new KWDRProduct;
		productRule->AddOperand(productRule->GetFirstOperand()->Clone());
		productRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		productRule->GetFirstOperand()->SetContinuousConstant(cValueProb / 2);
		productRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		productRule->GetSecondOperand()->SetAttributeName(binaryAttribute->GetName());
		distanceAttribute =
		    AddDataPreparationRuleAttribute(productRule, "D_" + sBinaryPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(distanceAttribute);

		// Creation d'un attribut de self-distance
		diffRule = new KWDRDiff;
		diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		diffRule->GetFirstOperand()->SetContinuousConstant(1);
		diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		diffRule->GetSecondOperand()->SetAttributeName(binaryAttribute->GetName());
		productRule0 = new KWDRProduct;
		productRule0->AddOperand(productRule0->GetFirstOperand()->Clone());
		productRule0->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		productRule0->GetFirstOperand()->SetContinuousConstant(cValueProb * ((1 - cValueProb) / 3));
		productRule0->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		productRule0->GetSecondOperand()->SetDerivationRule(diffRule);
		productRule1 = new KWDRProduct;
		productRule1->AddOperand(productRule1->GetFirstOperand()->Clone());
		productRule1->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		productRule1->GetFirstOperand()->SetContinuousConstant(cValueProb * (cValueProb / 3));
		productRule1->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		productRule1->GetSecondOperand()->SetAttributeName(binaryAttribute->GetName());
		sumRule = new KWDRSum;
		sumRule->AddOperand(sumRule->GetFirstOperand()->Clone());
		sumRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		sumRule->GetFirstOperand()->SetDerivationRule(productRule0);
		sumRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		sumRule->GetSecondOperand()->SetDerivationRule(productRule1);
		selfDistanceAttribute =
		    AddDataPreparationRuleAttribute(sumRule, "SD_" + sBinaryPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(selfDistanceAttribute);
	}
}

void KWDataPreparationAttribute::AddPreparedCategoricalDistanceAttributes2(ObjectArray* oaAddedAttributes)
{
	KWAttribute* cellIndexAttribute;
	KWAttribute* binaryAttribute;
	KWDREQ* eqRule;
	const ALString sBinaryPrefix = "B";
	int nBinaryAttributeNumber;
	KWAttribute* probAttribute;
	KWDRCopyContinuous* copyRule;
	int nTotalFrequency;
	Continuous cValueProb;
	int i;

	require(GetDistanceStudyMode());

	// Creation d'un attribut intermediaire (Unused) pour le calcul de l'index
	cellIndexAttribute = AddPreparedIndexingAttribute();
	cellIndexAttribute->SetUsed(false);

	// Parcours des cellules sources la grille en supervise, ou cibles en non supervise
	if (GetPreparedStats()->GetTargetAttributeType() == KWType::None)
	{
		assert(GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize() <= 1);
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize();
	}
	else
		nBinaryAttributeNumber = GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize();
	oaAddedAttributes->SetSize(0);
	nTotalFrequency = GetPreparedStats()->GetPreparedDataGridStats()->ComputeGridFrequency();
	for (i = 0; i < nBinaryAttributeNumber; i++)
	{
		// Creation d'une regle de calcul pour l'egalite a un des index
		eqRule = new KWDREQ;
		eqRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		eqRule->GetFirstOperand()->SetAttributeName(cellIndexAttribute->GetName());
		eqRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		eqRule->GetSecondOperand()->SetContinuousConstant((Continuous)(i + 1));

		// Creation de l'attribut de binarisation
		binaryAttribute = AddDataPreparationRuleAttribute(eqRule, sBinaryPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(binaryAttribute);

		// Probabilite de la valeurs
		cValueProb = GetPreparedStats()->GetPreparedDataGridStats()->GetUnivariateCellFrequencyAt(i);
		cValueProb /= nTotalFrequency;

		// Creation d'un attribut memorisant la probabilite
		copyRule = new KWDRCopyContinuous;
		copyRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		copyRule->GetFirstOperand()->SetContinuousConstant(cValueProb);
		probAttribute = AddDataPreparationRuleAttribute(copyRule, "P_" + sBinaryPrefix + IntToString(i + 1));
		oaAddedAttributes->Add(probAttribute);
	}
}

void KWDataPreparationAttribute::InitDataGridRule(KWDRDataGridRule* dataGridRule)
{
	KWClass* kwcDataPreparationClass;
	KWDataGridStats* preparedDataGridStats;
	const KWDGSAttributePartition* attributePartition;
	KWDerivationRuleOperand* operand;
	int nAttribute;

	require(Check());
	require(dataGridRule != NULL);

	// Acces a la classe de preparation
	kwcDataPreparationClass = preparedAttribute->GetParentClass();

	// Acces a la grille de donnees
	preparedDataGridStats = preparedStats->GetPreparedDataGridStats();

	// Initialisation du premier operande de la regle avec l'attribut de preparation grille
	dataGridRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	dataGridRule->GetFirstOperand()->SetAttributeName(preparedAttribute->GetName());

	// Ajout des attributs de la grille comme operande
	dataGridRule->DeleteAllVariableOperands();
	for (nAttribute = 0; nAttribute < preparedDataGridStats->GetPredictionAttributeNumber(); nAttribute++)
	{
		attributePartition = preparedDataGridStats->GetAttributeAt(nAttribute);

		// Ajout d'une operande pour lier l'attribut source
		if (attributePartition->GetAttributeName() != preparedStats->GetTargetAttributeName())
		{
			operand = new KWDerivationRuleOperand;
			dataGridRule->AddOperand(operand);
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			operand->SetType(attributePartition->GetAttributeType());
			operand->SetAttributeName(attributePartition->GetAttributeName());
		}
	}
	dataGridRule->CompleteTypeInfo(kwcDataPreparationClass);
}

KWAttribute* KWDataPreparationAttribute::AddDataPreparationRuleAttribute(KWDerivationRule* preparationRule,
									 const ALString& sAttributePrefix)
{
	const ALString sLevelMetaDataKey = GetLevelMetaDataKey();
	KWClass* kwcDataPreparationClass;
	KWAttribute* dataGridRuleAttribute;

	require(Check());
	require(preparationRule != NULL);

	// Acces a la classe de preparation
	kwcDataPreparationClass = preparedAttribute->GetParentClass();

	// Ajout de l'attribut exploitant la regle de preparation
	dataGridRuleAttribute = new KWAttribute;
	dataGridRuleAttribute->SetName(sAttributePrefix + preparedAttribute->GetName());
	dataGridRuleAttribute->SetDerivationRule(preparationRule);

	// Completion des informations pour finaliser la specification
	dataGridRuleAttribute->CompleteTypeInfo(kwcDataPreparationClass);

	// Meta-donne de Level sur l'attribut de preparation dans le cas supervise
	if (preparedStats->GetTargetAttributeType() != KWType::None)
		dataGridRuleAttribute->GetMetaData()->SetDoubleValueAt(sLevelMetaDataKey, preparedStats->GetLevel());

	// Ajout dans la classe
	dataGridRuleAttribute->SetName(kwcDataPreparationClass->BuildAttributeName(dataGridRuleAttribute->GetName()));
	kwcDataPreparationClass->InsertAttribute(dataGridRuleAttribute);
	return dataGridRuleAttribute;
}

boolean KWDataPreparationAttribute::CheckSpecification(const ObjectArray* oaCheckedNativeAttributes,
						       KWAttribute* kwaPreparedAttribute,
						       KWDataPreparationStats* kwdpsAttributeStats) const
{
	boolean bOk = true;
	KWAttribute* kwaNativeAttribute;
	const KWDGSAttributePartition* attributePartition;
	KWClass* kwcPreparedClass;
	int nAttribute;

	require(oaCheckedNativeAttributes != NULL);
	require(kwaPreparedAttribute != NULL);
	require(kwdpsAttributeStats != NULL);

	// Quelques verification supplementaires, sans message utilisateur
	// On passe par GetLearningSpec() pour l'acces au target attribute afin
	// d'eviter le Check() du LearningService
	assert(kwdpsAttributeStats->GetLearningSpec()->GetTargetAttributeName() == "" or
	       kwdpsAttributeStats->GetPreparedDataGridStats() != NULL);

	// Acces aux classes des attributs
	kwaNativeAttribute = NULL;
	if (oaCheckedNativeAttributes->GetSize() > 0)
	{
		kwaNativeAttribute = cast(KWAttribute*, oaCheckedNativeAttributes->GetAt(0));
	}
	kwcPreparedClass = kwaPreparedAttribute->GetParentClass();

	// Test d'existence de statistiques descriptives
	bOk = kwdpsAttributeStats->IsStatsComputed();

	// Test d'existence des attributs natifs dans la classe de recodage
	if (kwcPreparedClass != NULL)
	{
		for (nAttribute = 0; nAttribute < oaCheckedNativeAttributes->GetSize(); nAttribute++)
		{
			kwaNativeAttribute = cast(KWAttribute*, oaCheckedNativeAttributes->GetAt(nAttribute));
			if (kwcPreparedClass->LookupAttribute(kwaNativeAttribute->GetName()) == NULL)
			{
				AddError("Native variable " + kwaNativeAttribute->GetName() +
					 " unknown in the prepared dictionary");
				bOk = false;
				break;
			}
		}
	}

	// Test d'existence des attribut de la grille preparee
	if (kwcPreparedClass != NULL and kwdpsAttributeStats->GetPreparedDataGridStats() != NULL)
	{
		for (nAttribute = 0;
		     nAttribute < kwdpsAttributeStats->GetPreparedDataGridStats()->GetPredictionAttributeNumber();
		     nAttribute++)
		{
			attributePartition =
			    kwdpsAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(nAttribute);
			if (kwcPreparedClass->LookupAttribute(attributePartition->GetAttributeName()) == NULL)
			{
				AddError("Prepared variable " + attributePartition->GetAttributeName() +
					 " unknown in the prepared dictionary");
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

int KWDataPreparationAttributeCompareSortValue(const void* elem1, const void* elem2)
{
	KWDataPreparationAttribute* dataPreparationAttribute1;
	KWDataPreparationAttribute* dataPreparationAttribute2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux objets
	dataPreparationAttribute1 = cast(KWDataPreparationAttribute*, *(Object**)elem1);
	dataPreparationAttribute2 = cast(KWDataPreparationAttribute*, *(Object**)elem2);
	assert(dataPreparationAttribute1->Check());
	assert(dataPreparationAttribute2->Check());

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(dataPreparationAttribute1->GetPreparedStats()->GetSortValue(),
							dataPreparationAttribute2->GetPreparedStats()->GetSortValue());

	// Comparaison sur le nom en cas d'egalite du level (sort value)
	if (nCompare == 0)
		nCompare = dataPreparationAttribute1->GetPreparedStats()->CompareName(
		    dataPreparationAttribute2->GetPreparedStats());
	return nCompare;
}

int KWDataPreparationAttributeCompareVarKey(const void* elem1, const void* elem2)
{
	KWDataPreparationAttribute* dataPreparationAttribute1;
	KWDataPreparationAttribute* dataPreparationAttribute2;
	KWAttribute* nativeAttribute1;
	KWAttribute* nativeAttribute2;
	KWAttributeBlock* attributeBlock;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs de preparation
	dataPreparationAttribute1 = cast(KWDataPreparationAttribute*, *(Object**)elem1);
	dataPreparationAttribute2 = cast(KWDataPreparationAttribute*, *(Object**)elem2);

	// Test d'integrite des attributs de preparation
	assert(dataPreparationAttribute1->Check());
	assert(dataPreparationAttribute2->Check());
	assert(dataPreparationAttribute1->GetNativeAttributeNumber() == 1);
	assert(dataPreparationAttribute2->GetNativeAttributeNumber() == 1);

	// Acces aux attributs natifs
	nativeAttribute1 = dataPreparationAttribute1->GetNativeAttribute();
	nativeAttribute2 = dataPreparationAttribute2->GetNativeAttribute();

	// Test d'integrite des attributs natif
	assert(nativeAttribute1->GetAttributeBlock() != NULL);
	assert(nativeAttribute2->GetAttributeBlock() != NULL);
	assert(nativeAttribute1->GetAttributeBlock() == nativeAttribute2->GetAttributeBlock());

	// Acces au bloc common
	attributeBlock = nativeAttribute1->GetAttributeBlock();

	// Difference pour chaque type de VarKey: Symbol ou Continuous
	if (attributeBlock->GetVarKeyType() == KWType::Symbol)
		nDiff = attributeBlock->GetSymbolVarKey(nativeAttribute1)
			    .CompareValue(attributeBlock->GetSymbolVarKey(nativeAttribute2));
	else
		nDiff = KWContinuous::CompareIndicatorValue(attributeBlock->GetContinuousVarKey(nativeAttribute1),
							    attributeBlock->GetContinuousVarKey(nativeAttribute2));
	return nDiff;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationTargetAttribute

KWDataPreparationTargetAttribute::KWDataPreparationTargetAttribute()
{
	preparedAttribute = NULL;
	nativeAttribute = NULL;
	attributeValueStats = NULL;
}

KWDataPreparationTargetAttribute::~KWDataPreparationTargetAttribute() {}

void KWDataPreparationTargetAttribute::InitFromAttributeValueStats(KWClass* kwcClass,
								   KWDataGridStats* kwdgsAttributeValueStats)
{
	KWDRDataGrid* dgRule;

	require(nativeAttribute == NULL);
	require(preparedAttribute == NULL);
	require(attributeValueStats == NULL);
	require(kwcClass != NULL);
	require(kwdgsAttributeValueStats != NULL);
	require(kwdgsAttributeValueStats->GetAttributeNumber() == 1);

	// Memorisation de l'attribut natif
	nativeAttribute = kwcClass->LookupAttribute(kwdgsAttributeValueStats->GetAttributeAt(0)->GetAttributeName());
	check(nativeAttribute);

	// Memorisation des infos de preparation de l'attribut
	attributeValueStats = kwdgsAttributeValueStats;

	// Creation d'une regle DataGrid
	dgRule = new KWDRDataGrid;
	dgRule->ImportDataGridStats(attributeValueStats, false);
	dgRule->CompleteTypeInfo(kwcClass);

	// Creation d'un attribut DataGridStats
	preparedAttribute = new KWAttribute;
	preparedAttribute->SetName(kwcClass->BuildAttributeName("V" + nativeAttribute->GetName()));
	preparedAttribute->SetDerivationRule(dgRule);
	preparedAttribute->SetType(dgRule->GetType());
	preparedAttribute->SetStructureName(dgRule->GetStructureName());

	// Ajout de l'attribut dans la classe
	preparedAttribute->SetName(kwcClass->BuildAttributeName(preparedAttribute->GetName()));
	kwcClass->InsertAttribute(preparedAttribute);

	// On le met en unused (il s'agit d'une Structure(DataGrid))
	preparedAttribute->SetUsed(false);

	ensure(Check());
}

void KWDataPreparationTargetAttribute::Reset()
{
	preparedAttribute = NULL;
	nativeAttribute = NULL;
	attributeValueStats = NULL;
}

void KWDataPreparationTargetAttribute::SetNativeAttribute(KWAttribute* kwaNativeAttribute)
{
	nativeAttribute = kwaNativeAttribute;
}

KWAttribute* KWDataPreparationTargetAttribute::GetNativeAttribute()
{
	require(Check());
	return nativeAttribute;
}

void KWDataPreparationTargetAttribute::SetPreparedAttribute(KWAttribute* kwaPreparedAttribute)
{
	preparedAttribute = kwaPreparedAttribute;
}

KWAttribute* KWDataPreparationTargetAttribute::GetPreparedAttribute()
{
	require(Check());
	return preparedAttribute;
}

void KWDataPreparationTargetAttribute::SetAttributeValueStats(KWDataGridStats* kwdgsAttributeValueStats)
{
	attributeValueStats = kwdgsAttributeValueStats;
}

KWDataGridStats* KWDataPreparationTargetAttribute::GetAttributeValueStats()
{
	require(Check());
	return attributeValueStats;
}

KWAttribute* KWDataPreparationTargetAttribute::AddPreparedIndexingAttribute()
{
	KWClass* kwcDataPreparationClass;
	KWAttribute* indexingAttribute;
	KWDRValueIndex* valueIndexRule;
	KWDerivationRuleOperand* operand;

	require(Check());

	// Acces a la classe de preparation
	kwcDataPreparationClass = preparedAttribute->GetParentClass();

	// Creation d'une regle pour indexer les cellules
	valueIndexRule = new KWDRValueIndex;
	valueIndexRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueIndexRule->GetFirstOperand()->SetAttributeName(preparedAttribute->GetName());

	// Ajout de l'attribut natif comme operande
	valueIndexRule->DeleteAllVariableOperands();
	operand = new KWDerivationRuleOperand;
	valueIndexRule->AddOperand(operand);
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetType(attributeValueStats->GetAttributeAt(0)->GetAttributeType());
	operand->SetAttributeName(attributeValueStats->GetAttributeAt(0)->GetAttributeName());
	valueIndexRule->CompleteTypeInfo(kwcDataPreparationClass);

	// Ajout de l'attribut de calcul des index de valeurs cibles
	indexingAttribute = new KWAttribute;
	indexingAttribute->SetName("Index" + preparedAttribute->GetName());
	indexingAttribute->SetDerivationRule(valueIndexRule);
	indexingAttribute->SetType(indexingAttribute->GetDerivationRule()->GetType());

	// Ajout dans la classe
	indexingAttribute->SetName(kwcDataPreparationClass->BuildAttributeName(indexingAttribute->GetName()));
	kwcDataPreparationClass->InsertAttribute(indexingAttribute);
	return indexingAttribute;
}

boolean KWDataPreparationTargetAttribute::Check() const
{
	return CheckSpecification(nativeAttribute, preparedAttribute, attributeValueStats);
}

void KWDataPreparationTargetAttribute::Write(ostream& ost) const
{
	require(Check());

	// Nom de l'attribut natif et de son attribut recode
	ost << "Target variable\t" << nativeAttribute->GetName() << "\t" << preparedAttribute->GetName() << "\n";

	// Grille de preparation
	if (attributeValueStats != NULL)
		ost << *attributeValueStats << "\n";
}

const ALString KWDataPreparationTargetAttribute::GetClassLabel() const
{
	return "Target variable preparation";
}

const ALString KWDataPreparationTargetAttribute::GetObjectLabel() const
{
	if (nativeAttribute == NULL)
		return "";
	else
		return nativeAttribute->GetName();
}

boolean KWDataPreparationTargetAttribute::CheckSpecification(KWAttribute* kwaNativeAttribute,
							     KWAttribute* kwaPreparedAttribute,
							     KWDataGridStats* kwdgsAttributeValueStats) const
{
	boolean bOk = true;
	KWClass* kwcPreparedClass;

	require(kwaNativeAttribute != NULL);
	require(kwaPreparedAttribute != NULL);
	require(kwdgsAttributeValueStats != NULL);
	require(kwdgsAttributeValueStats->GetAttributeNumber() == 1);

	// Acces aux classes des attributs
	kwcPreparedClass = kwaPreparedAttribute->GetParentClass();

	// Test d'existence de l'attribut natif dans la classe de recodage
	if (kwcPreparedClass != NULL)
	{
		if (kwcPreparedClass->LookupAttribute(kwaNativeAttribute->GetName()) == NULL)
		{
			AddError("Native variable " + kwaNativeAttribute->GetName() +
				 " unknown in the prepared dictionary");
			bOk = false;
		}
	}
	return bOk;
}
