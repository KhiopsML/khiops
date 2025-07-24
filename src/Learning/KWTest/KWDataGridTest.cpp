// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridTest.h"

///////////////////////////////////////////////////////////////////////
// Classe KWSampleGenerator

KWSampleGenerator::KWSampleGenerator()
{
	nModalityNumber = 0;
	dNoiseRate = 0;
	preparedDataGridStats = NULL;
	probabilityTable = NULL;
	cellIndexRule = NULL;
	dGlobalPlusProb = 0;
	dGlobalMinusProb = 0;
	nMinusIndex = -1;
	nPlusIndex = -1;
}

KWSampleGenerator::~KWSampleGenerator()
{
	if (preparedDataGridStats == NULL)
		delete preparedDataGridStats;
	if (probabilityTable == NULL)
		delete probabilityTable;
	if (cellIndexRule == NULL)
		delete cellIndexRule;
}

ALString KWSampleGenerator::GetFullName() const
{
	ALString sFullName;
	int nAttribute;
	ALString sTmp;

	sFullName = GetName();
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		assert(KWType::IsSimple(ivAttributeTypes.GetAt(nAttribute)));
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Continuous)
			sFullName += "C";
		else if (ivAttributeTypes.GetAt(nAttribute) == KWType::Symbol)
			sFullName += "S";
	}
	if (GetModalityNumber() > 1)
		sFullName += IntToString(GetModalityNumber());
	if (GetNoiseRate() > 0)
		sFullName += sTmp + "(" + IntToString(int(100 * GetNoiseRate())) + ")";
	return sFullName;
}

const IntVector* KWSampleGenerator::GetAttributeTypes() const
{
	return &ivAttributeTypes;
}

void KWSampleGenerator::SetModalityNumber(int nValue)
{
	require(nValue > 0);

	nModalityNumber = nValue;
}

int KWSampleGenerator::GetModalityNumber() const
{
	return nModalityNumber;
}

void KWSampleGenerator::SetNoiseRate(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dNoiseRate = dValue;
}

double KWSampleGenerator::GetNoiseRate() const
{
	return dNoiseRate;
}

void KWSampleGenerator::NoisifyObjectClass(KWObject* kwoObject)
{
	require(kwoObject != NULL);

	if (dNoiseRate > 0 and GetRandomDouble() <= dNoiseRate)
	{
		if (GetRandomDouble() <= 0.5)
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, GetPlusClass());
		else
			kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, GetMinusClass());
	}
}

void KWSampleGenerator::GenerateObjectValuesAndNoisyClass(KWObject* kwoObject)
{
	GenerateObjectValues(kwoObject);
	if (dNoiseRate > 0)
		NoisifyObjectClass(kwoObject);
}

KWClass* KWSampleGenerator::GetSampleClass() const
{
	const ALString sClassPrefix = "Sample";
	ALString sClassName;
	KWClass* kwcClass;
	KWAttribute* attribute;
	int nAttribute;
	ALString sAttributeName = "X";

	// Recherche de la classe de test
	sClassName = sClassPrefix + GetFullName();
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);

	// Creation et enregistrement de la classe si non trouvee
	if (kwcClass == NULL)
	{
		// Initialisation de la classe
		kwcClass = new KWClass;
		kwcClass->SetName(sClassName);
		kwcClass->SetLabel("Test des methodes d'optimisation de grilles de donnees");

		// Initialisation des attributs
		for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		{
			attribute = new KWAttribute;
			attribute->SetName(sAttributeName + IntToString(nAttribute + 1));
			assert(KWType::IsSimple(ivAttributeTypes.GetAt(nAttribute)));
			attribute->SetType(ivAttributeTypes.GetAt(nAttribute));
			kwcClass->InsertAttribute(attribute);
		}
		attribute = new KWAttribute;
		attribute->SetName("Class");
		attribute->SetType(KWType::Symbol);
		kwcClass->InsertAttribute(attribute);

		// Enregistrement et compilation
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
		kwcClass->Compile();
	}

	// Verifications
	check(kwcClass);
	assert(kwcClass->Check());
	assert(kwcClass->LookupAttribute("Class") != NULL);

	// Calcul des index de chargement
	livAttributeLoadIndexes.SetSize(ivAttributeTypes.GetSize());
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		attribute = kwcClass->GetLoadedAttributeAt(nAttribute);
		assert(attribute->GetName() == sAttributeName + IntToString(nAttribute + 1));
		livAttributeLoadIndexes.SetAt(nAttribute, attribute->GetLoadIndex());
	}
	attribute = kwcClass->GetLoadedAttributeAt(ivAttributeTypes.GetSize());
	assert(attribute->GetName() == "Class");
	liTargetAttributeLoadIndex = attribute->GetLoadIndex();

	return kwcClass;
}

boolean KWSampleGenerator::CheckObject(KWObject* kwoObject) const
{
	boolean bOk = true;

	if (kwoObject == NULL)
	{
		bOk = false;
		AddError("Entity is NULL");
	}
	else if (kwoObject->GetClass() != GetSampleClass())
	{
		bOk = false;
		kwoObject->AddError("Wrong dictionary");
	}
	else
	{
		bOk = true;
		assert(kwoObject->GetClass()->IsCompiled());
		assert(kwoObject->GetClass()->LookupAttribute("Class")->GetLoadIndex() == liTargetAttributeLoadIndex);
		assert(kwoObject->GetClass()->GetAttributeNumber() == ivAttributeTypes.GetSize() + 1);
	}

	return bOk;
}

boolean KWSampleGenerator::CheckAttributeSubsetStats(KWAttributeSubsetStats* attributeSubsetStats) const
{
	boolean bOk = true;

	if (attributeSubsetStats == NULL)
	{
		bOk = false;
		AddError("Attribute subset stats is NULL");
	}
	else if (attributeSubsetStats->GetClass() != GetSampleClass())
	{
		bOk = false;
		attributeSubsetStats->AddError("Bad class");
	}
	else
	{
		bOk = attributeSubsetStats->Check();
		bOk = bOk and attributeSubsetStats->CheckSpecifications();
		bOk = bOk and attributeSubsetStats->IsStatsComputed();
	}
	return bOk;
}

void KWSampleGenerator::InitFromAttributeSubsetStats(KWAttributeSubsetStats* attributeSubsetStats)
{
	boolean bTrace = false;
	KWDRDataGrid* dataGridRule;
	const KWDGSAttributePartition* attributePartition;
	KWDataGridStats* targetValueStats;
	const KWDGSAttributeSymbolValues* targetValues;
	KWDerivationRuleOperand* operand;
	int nAttribute;

	require(CheckAttributeSubsetStats(attributeSubsetStats));
	require(attributeSubsetStats->IsStatsComputed());

	// Nettoyage prealable
	if (preparedDataGridStats == NULL)
		delete preparedDataGridStats;
	if (probabilityTable == NULL)
		delete probabilityTable;
	if (cellIndexRule == NULL)
		delete cellIndexRule;
	preparedDataGridStats = NULL;
	probabilityTable = NULL;
	cellIndexRule = NULL;
	dGlobalPlusProb = 0;
	dGlobalMinusProb = 0;
	nMinusIndex = 0;
	nPlusIndex = 0;

	// Initialisation de la grille de preparation
	preparedDataGridStats = attributeSubsetStats->GetPreparedDataGridStats()->Clone();

	// Initialisation de la table de probabilites conditionnelles
	probabilityTable = new KWProbabilityTable;
	probabilityTable->ImportDataGridStats(preparedDataGridStats, true, false);

	// Recherche de l'index de chaque classe
	targetValueStats = attributeSubsetStats->GetTargetValueStats();
	targetValues = cast(const KWDGSAttributeSymbolValues*, targetValueStats->GetAttributeAt(0));
	assert(targetValues->GetValueNumber() <= 2);
	nPlusIndex = targetValues->ComputeSymbolPartIndex(GetPlusClass());
	nMinusIndex = targetValues->ComputeSymbolPartIndex(GetMinusClass());

	// Recherche des probabiltes cibles globales
	if (nMinusIndex >= 0)
		dGlobalMinusProb = targetValueStats->GetUnivariateCellFrequencyAt(nMinusIndex);
	dGlobalMinusProb /= targetValueStats->ComputeGridFrequency();
	if (nPlusIndex >= 0)
		dGlobalPlusProb = targetValueStats->GetUnivariateCellFrequencyAt(nPlusIndex);
	dGlobalPlusProb /= targetValueStats->ComputeGridFrequency();

	// Creation de la regle d'indexation dans la grille
	cellIndexRule = new KWDRCellIndex;

	// Initialisation du premier operande de la regle avec l'attribut de preparation grille
	dataGridRule = new KWDRDataGrid;
	dataGridRule->ImportDataGridStats(preparedDataGridStats, false);
	cellIndexRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	cellIndexRule->GetFirstOperand()->SetDerivationRule(dataGridRule);

	// Ajout des attribut de la grille comme operande
	cellIndexRule->DeleteAllVariableOperands();
	for (nAttribute = 0; nAttribute < preparedDataGridStats->GetPredictionAttributeNumber(); nAttribute++)
	{
		attributePartition = preparedDataGridStats->GetAttributeAt(nAttribute);

		// Ajout d'une operande pour lier l'attribut source
		operand = new KWDerivationRuleOperand;
		cellIndexRule->AddOperand(operand);
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetType(attributePartition->GetAttributeType());
		operand->SetAttributeName(attributePartition->GetAttributeName());
	}
	cellIndexRule->CompleteTypeInfo(GetSampleClass());

	// Compilation de la regle
	cellIndexRule->Compile(GetSampleClass());
	assert(cellIndexRule->IsCompiled());

	// Affichage des resultats
	if (bTrace)
	{
		cout << "Index(-)"
		     << "\t" << nMinusIndex << endl;
		cout << "Index(+)"
		     << "\t" << nPlusIndex << endl;
		cout << *targetValueStats << endl;
		cout << *preparedDataGridStats << endl;
		cout << *probabilityTable << endl;
	}
}

boolean KWSampleGenerator::IsAttributeSubsetStatsInitialized() const
{
	return cellIndexRule != NULL and cellIndexRule->IsCompiled();
}

double KWSampleGenerator::ComputeObjectError(KWObject* kwoObject)
{
	double dError;
	Symbol sPredictedClass;
	Symbol sTrueClass;

	// Acces aux valeurs predites et reelles
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	sPredictedClass = ComputePredictedClass(kwoObject);

	// Calcul de l'erreur
	if (sPredictedClass == sTrueClass)
		dError = 0;
	else
		dError = 1;
	return dError;
}

double KWSampleGenerator::ComputeObjectDKL(KWObject* kwoObject)
{
	boolean bDisplay = false;
	double dDKL;
	double dProbPlus;
	double dProbMinus;
	double dTrueProbPlus;
	double dTrueProbMinus;
	Symbol sTrueClass;

	// Calcul des probabilites vraies et predites par classe
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	dProbPlus = ComputePredictedProb(kwoObject, GetPlusClass());
	dProbMinus = ComputePredictedProb(kwoObject, GetMinusClass());
	dTrueProbPlus = ComputeNoisyTrueProb(kwoObject, GetPlusClass());
	dTrueProbMinus = ComputeNoisyTrueProb(kwoObject, GetMinusClass());

	// Verifications
	assert(0 <= dProbPlus and dProbPlus <= 1);
	assert(0 <= dProbMinus and dProbMinus <= 1);
	assert(fabs(dProbPlus + dProbMinus - 1) < 1e-5);
	assert(0 <= dTrueProbPlus and dTrueProbPlus <= 1);
	assert(0 <= dTrueProbMinus and dTrueProbMinus <= 1);
	assert(fabs(dTrueProbPlus + dTrueProbMinus - 1) < 1e-5);

	// Calcul du critere
	dDKL = 0;
	if (dTrueProbPlus > 0)
		dDKL += dTrueProbPlus * log(dTrueProbPlus / dProbPlus);
	if (dTrueProbMinus > 0)
		dDKL += dTrueProbMinus * log(dTrueProbMinus / dProbMinus);

	// Affichage
	if (bDisplay)
	{
		// Attributs de l'objets
		if (ivAttributeTypes.GetAt(0) == KWType::Continuous)
			cout << kwoObject->ComputeContinuousValueAt(livAttributeLoadIndexes.GetAt(0)) << "\t";
		else
			cout << kwoObject->ComputeSymbolValueAt(livAttributeLoadIndexes.GetAt(0)) << "\t";
		if (ivAttributeTypes.GetAt(1) == KWType::Continuous)
			cout << kwoObject->ComputeContinuousValueAt(livAttributeLoadIndexes.GetAt(1)) << "\t";
		else
			cout << kwoObject->ComputeSymbolValueAt(livAttributeLoadIndexes.GetAt(1)) << "\t";
		cout << sTrueClass << "\t";

		// Probabilites predites
		cout << dProbMinus << "\t" << dProbPlus << "\t";

		// Probabilites vraies
		cout << dTrueProbMinus << "\t" << dTrueProbPlus << "\t";

		// DKL
		cout << dDKL << "\n";
	}

	return dDKL;
}

double KWSampleGenerator::ComputeObjectMSE(KWObject* kwoObject)
{
	double dMSE;
	double dProbPlus;
	double dProbMinus;
	double dTrueProbPlus;
	double dTrueProbMinus;
	Symbol sTrueClass;

	// Calcul des probabilites vraies et predites par classe
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	dProbPlus = ComputePredictedProb(kwoObject, GetPlusClass());
	dProbMinus = ComputePredictedProb(kwoObject, GetMinusClass());
	dTrueProbPlus = ComputeNoisyTrueProb(kwoObject, GetPlusClass());
	dTrueProbMinus = ComputeNoisyTrueProb(kwoObject, GetMinusClass());

	// Verifications
	assert(0 <= dProbPlus and dProbPlus <= 1);
	assert(0 <= dProbMinus and dProbMinus <= 1);
	assert(fabs(dProbPlus + dProbMinus - 1) < 1e-5);
	assert(0 <= dTrueProbPlus and dTrueProbPlus <= 1);
	assert(0 <= dTrueProbMinus and dTrueProbMinus <= 1);
	assert(fabs(dTrueProbPlus + dTrueProbMinus - 1) < 1e-5);

	// Calcul du critere
	if (sTrueClass == GetPlusClass())
		dMSE = pow(dTrueProbPlus - dProbPlus, 2);
	else
		dMSE = pow(dTrueProbMinus - dProbMinus, 2);
	return dMSE;
}

double KWSampleGenerator::ComputeDatabaseError(KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectError(kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

double KWSampleGenerator::ComputeDatabaseDKL(KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectDKL(kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

double KWSampleGenerator::ComputeDatabaseMSE(KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectMSE(kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

Symbol KWSampleGenerator::ComputePredictedClass(KWObject* kwoObject)
{
	int nCellIndex;

	require(IsAttributeSubsetStatsInitialized());
	require(CheckObject(kwoObject));

	// Recherche de l'index de la cellule de grille
	nCellIndex = (int)floor(cellIndexRule->ComputeContinuousResult(kwoObject) + 0.5);

	// Effet de bord avec une seule classe (peut arriver pour des tres petites bases)
	if (nMinusIndex < 0 or nPlusIndex < 0)
	{
		if (nMinusIndex >= 0)
			return GetMinusClass();
		else
			return GetPlusClass();
	}
	// Cas standard avec deux classes
	else
	{
		assert(nPlusIndex >= 0 and nMinusIndex >= 0);

		// Si index valide: recherche de la classe majoritaire dans la cellule
		if (nCellIndex != -1)
		{
			if (probabilityTable->GetTargetConditionalProbAt(nCellIndex, nPlusIndex) >=
			    probabilityTable->GetTargetConditionalProbAt(nCellIndex, nMinusIndex))
				return GetPlusClass();
			else
				return GetMinusClass();
		}
		// Sinon: recherche de la classe majoritaire dans la table globalement
		{
			if (dGlobalPlusProb >= dGlobalMinusProb)
				return GetPlusClass();
			else
				return GetMinusClass();
		}
	}
}

double KWSampleGenerator::ComputePredictedProb(KWObject* kwoObject, const Symbol& sClass)
{
	int nCellIndex;
	double dPlusProb;
	double dMinusProb;
	double dProb;

	require(IsAttributeSubsetStatsInitialized());
	require(CheckObject(kwoObject));

	// Recherche de l'index de la cellule de grille
	nCellIndex = (int)floor(cellIndexRule->ComputeContinuousResult(kwoObject) + 0.5);

	// Effet de bord avec une seule classe (peut arriver pour des tres petites bases)
	if (nMinusIndex < 0 or nPlusIndex < 0)
	{
		if (nPlusIndex >= 0)
		{
			dPlusProb = probabilityTable->GetTargetConditionalProbAt(nCellIndex, nPlusIndex);
			dMinusProb = 0;
		}
		else
		{
			dMinusProb = probabilityTable->GetTargetConditionalProbAt(nCellIndex, nMinusIndex);
			dPlusProb = 0;
		}
	}
	// Cas standard avec deux classes
	else
	{
		assert(nPlusIndex >= 0 and nMinusIndex >= 0);

		// Si index valide: calcul des frequences par classe dans la cellule
		if (nCellIndex != -1)
		{
			dPlusProb = probabilityTable->GetTargetConditionalProbAt(nCellIndex, nPlusIndex);
			dMinusProb = probabilityTable->GetTargetConditionalProbAt(nCellIndex, nMinusIndex);
		}
		// Sinon: calcul des frequences par classe dans la table globalement
		else
		{
			dPlusProb = dGlobalPlusProb;
			dMinusProb = dGlobalMinusProb;
		}
	}

	// Porbabilite conditionnelle demandee
	if (sClass == GetMinusClass())
		dProb = dMinusProb;
	else
		dProb = dPlusProb;
	return dProb;
}

Symbol KWSampleGenerator::ComputeNoisyTrueMajorityClass(KWObject* kwoObject)
{
	double dTruePlusProb;

	require(CheckObject(kwoObject));

	// Calcul de la probabilite conditionnelle de la classe Plus
	dTruePlusProb = ComputeNoisyTrueProb(kwoObject, GetPlusClass());
	assert(fabs(dTruePlusProb + ComputeNoisyTrueProb(kwoObject, GetMinusClass()) - 1) < 1e-5);

	// Retour
	if (dTruePlusProb >= 0.5)
		return GetPlusClass();
	else
		return GetMinusClass();
}

double KWSampleGenerator::ComputeNoisyTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	double dTrueProb;

	require(CheckObject(kwoObject));
	require(sClass == GetPlusClass() or sClass == GetMinusClass());

	// Calcul de la probabilite vraie de la classe, sans bruit
	dTrueProb = ComputeTrueProb(kwoObject, sClass);

	// Prise en compte du bruit
	if (GetNoiseRate() > 0)
		dTrueProb = (1 - dNoiseRate) * dTrueProb + dNoiseRate * 0.5;

	ensure(0 <= dTrueProb and dTrueProb <= 1);
	return dTrueProb;
}

double KWSampleGenerator::GetRandomDouble()
{
	return RandomDouble();
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGRandomContinuous

KWSGRandomContinuous::KWSGRandomContinuous()
{
	int nAttribute;

	ivAttributeTypes.SetSize(10);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Continuous);
}

KWSGRandomContinuous::~KWSGRandomContinuous() {}

const ALString KWSGRandomContinuous::GetName() const
{
	return "Random";
}

void KWSGRandomContinuous::GenerateObjectValues(KWObject* kwoObject)
{
	double dRandom;
	int nAttribute;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		dRandom = GetRandomDouble();
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Continuous)
			kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute), (Continuous)dRandom);
	}

	// La classe est generes aleatoirement
	dRandom = GetRandomDouble();
	if (dRandom >= 0.5)
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());
	else
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());
}

double KWSGRandomContinuous::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	return 0.5;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGRandomSymbol

KWSGRandomSymbol::KWSGRandomSymbol()
{
	int nAttribute;

	ivAttributeTypes.SetSize(2);
	SetModalityNumber(1);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Symbol);
}

KWSGRandomSymbol::~KWSGRandomSymbol() {}

const ALString KWSGRandomSymbol::GetName() const
{
	return "Random";
}

void KWSGRandomSymbol::GenerateObjectValues(KWObject* kwoObject)
{
	double dRandom;
	int nAttribute;
	int nModality;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		nModality = RandomInt(GetModalityNumber() - 1);
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Symbol)
			kwoObject->SetSymbolValueAt(livAttributeLoadIndexes.GetAt(nAttribute),
						    GetModalityAt(nModality));
	}

	// La classe est generes aleatoirement
	dRandom = GetRandomDouble();
	if (dRandom >= 0.5)
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());
	else
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());
}

double KWSGRandomSymbol::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	return 0.5;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGChessBoardSymbol

KWSGChessBoardSymbol::KWSGChessBoardSymbol()
{
	int nAttribute;

	ivAttributeTypes.SetSize(2);
	SetModalityNumber(1);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Symbol);
}

KWSGChessBoardSymbol::~KWSGChessBoardSymbol() {}

const ALString KWSGChessBoardSymbol::GetName() const
{
	return "ChessBoard";
}

void KWSGChessBoardSymbol::GenerateObjectValues(KWObject* kwoObject)
{
	const ALString sModalityPrefix = "v";
	int nAttribute;
	int nModality;
	int nTotalModality;

	require(CheckObject(kwoObject));

	// La valeur aleatoire fournit la valeur numerique
	nTotalModality = 0;
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		nModality = RandomInt(GetModalityNumber() - 1);
		nTotalModality += nModality;
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Symbol)
			kwoObject->SetSymbolValueAt(livAttributeLoadIndexes.GetAt(nAttribute),
						    GetModalityAt(nModality));
	}

	// La classe est genere en fonction de la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());
	else
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());
}

double KWSGChessBoardSymbol::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	int nAttribute;
	ALString sModality;
	int nModality;
	int nTotalModality;

	// Recherche des index de chaque modalite, et calcul de la somme de ces index
	nTotalModality = 0;
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		sModality = kwoObject->GetSymbolValueAt(livAttributeLoadIndexes.GetAt(nAttribute));
		assert(sModality.GetAt(0) == 'v');
		nModality = StringToInt(sModality.Right(sModality.GetLength() - 1));
		nTotalModality += nModality;
	}

	// La vraie classe depend la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
	{
		if (sClass == GetPlusClass())
			return 1;
		else
			return 0;
	}
	else
	{
		if (sClass == GetPlusClass())
			return 0;
		else
			return 1;
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGChessBoardContinuous

KWSGChessBoardContinuous::KWSGChessBoardContinuous()
{
	int nAttribute;

	ivAttributeTypes.SetSize(2);
	SetModalityNumber(1);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Continuous);
}

KWSGChessBoardContinuous::~KWSGChessBoardContinuous() {}

const ALString KWSGChessBoardContinuous::GetName() const
{
	return "ChessBoard";
}

void KWSGChessBoardContinuous::GenerateObjectValues(KWObject* kwoObject)
{
	int nAttribute;
	Continuous cRandom;
	int nModality;
	int nTotalModality;

	require(CheckObject(kwoObject));
	require(GetModalityNumber() >= 1);

	// La valeur aleatoire fournit la valeur numerique
	nTotalModality = 0;
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		cRandom = (Continuous)RandomDouble();
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		nTotalModality += nModality;
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Continuous)
			kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute), cRandom);
	}

	// La classe est genere en fonction de la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());
	else
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());
}

double KWSGChessBoardContinuous::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	int nAttribute;
	Continuous cRandom;
	int nModality;
	int nTotalModality;

	// Recherche des index de chaque modalite, et calcul de la somme de ces index
	nTotalModality = 0;
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		cRandom = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute));
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		assert(0 <= nModality and nModality < nModalityNumber);
		nTotalModality += nModality;
	}

	// La vraie classe depend la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
	{
		if (sClass == GetPlusClass())
			return 1;
		else
			return 0;
	}
	else
	{
		if (sClass == GetPlusClass())
			return 0;
		else
			return 1;
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGMultivariateXORContinuous

KWSGMultivariateXORContinuous::KWSGMultivariateXORContinuous()
{
	SetInputVariableNumber(2); // 10);
	SetXORVariableNumber(2);   // 10);
	SetModalityNumber(2);

	// Determination au hasard des variables du XOR
	RandomizeXORVariables();
}

KWSGMultivariateXORContinuous::~KWSGMultivariateXORContinuous() {}

void KWSGMultivariateXORContinuous::RandomizeXORVariables()
{
	int nIndex;

	// Determination au hasard des variables du XOR
	ivXORVariableIndexes.SetSize(ivAttributeTypes.GetSize());
	for (nIndex = 0; nIndex < ivAttributeTypes.GetSize(); nIndex++)
		ivXORVariableIndexes.SetAt(nIndex, nIndex);
	ivXORVariableIndexes.Shuffle();
	ivXORVariableIndexes.SetSize(GetXORVariableNumber());
	ivXORVariableIndexes.Sort();
}

void KWSGMultivariateXORContinuous::SetInputVariableNumber(int nValue)
{
	int nAttribute;

	require(nValue >= 0);

	nInputVariableNumber = nValue;
	ivAttributeTypes.SetSize(nInputVariableNumber);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Continuous);
}

int KWSGMultivariateXORContinuous::GetInputVariableNumber() const
{
	return nInputVariableNumber;
}

void KWSGMultivariateXORContinuous::SetXORVariableNumber(int nValue)
{
	require(nValue >= 0);
	nXORVariableNumber = nValue;
}

int KWSGMultivariateXORContinuous::GetXORVariableNumber() const
{
	return nXORVariableNumber;
}

const ALString KWSGMultivariateXORContinuous::GetName() const
{
	return "XOR";
}

ALString KWSGMultivariateXORContinuous::GetFullName() const
{
	ALString sFullName;

	sFullName = KWSampleGenerator::GetFullName() + "(" + IntToString(GetXORVariableNumber()) + ")";
	return sFullName;
}

void KWSGMultivariateXORContinuous::GenerateObjectValues(KWObject* kwoObject)
{
	int nAttribute;
	int nI;
	Continuous cRandom;
	int nModality;
	int nTotalModality;

	require(CheckObject(kwoObject));
	require(GetModalityNumber() >= 1);
	require(GetInputVariableNumber() > 0);
	require(GetXORVariableNumber() <= GetInputVariableNumber());

	// La valeur aleatoire fournit la valeur numerique
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		cRandom = (Continuous)RandomDouble();
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		if (ivAttributeTypes.GetAt(nAttribute) == KWType::Continuous)
			kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute), cRandom);
	}

	// On comptabilise les variables qui participent au XOR
	nTotalModality = 0;
	for (nI = 0; nI < ivXORVariableIndexes.GetSize(); nI++)
	{
		nAttribute = ivXORVariableIndexes.GetAt(nI);
		cRandom = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute));
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		nTotalModality += nModality;
	}

	// La classe est genere en fonction de la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());
	else
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());
}

double KWSGMultivariateXORContinuous::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	int nAttribute;
	int nI;
	Continuous cRandom;
	int nModality;
	int nTotalModality;

	// Recherche des index de chaque modalite, et calcul de la somme de ces index
	nTotalModality = 0;
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
	{
		cRandom = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute));
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		assert(0 <= nModality and nModality < nModalityNumber);
	}

	// On comptabilise les variables qui participent au XOR
	nTotalModality = 0;
	for (nI = 0; nI < ivXORVariableIndexes.GetSize(); nI++)
	{
		nAttribute = ivXORVariableIndexes.GetAt(nI);
		cRandom = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(nAttribute));
		nModality = (int)floor(cRandom * GetModalityNumber());
		if (nModality == nModalityNumber)
			nModality--;
		nTotalModality += nModality;
	}

	// La vraie classe depend la parite de la somme des coordonnees des modalites
	if (nTotalModality % 2 == 0)
	{
		if (sClass == GetPlusClass())
			return 1;
		else
			return 0;
	}
	else
	{
		if (sClass == GetPlusClass())
			return 0;
		else
			return 1;
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWSGGaussianMixture

KWSGGaussianMixture::KWSGGaussianMixture()
{
	int nAttribute;

	// Parametrage des attributs
	ivAttributeTypes.SetSize(2);
	SetModalityNumber(1);
	for (nAttribute = 0; nAttribute < ivAttributeTypes.GetSize(); nAttribute++)
		ivAttributeTypes.SetAt(nAttribute, KWType::Continuous);

	// Parametrage par defaut des Gaussiennes
	dMinusClassXMean = -1;
	dMinusClassYMean = -1;
	dMinusClassXStandardDeviation = 2;
	dMinusClassYStandardDeviation = 2;
	dPlusClassXMean = 1;
	dPlusClassYMean = 1;
	dPlusClassXStandardDeviation = 2;
	dPlusClassYStandardDeviation = 2;
}

KWSGGaussianMixture::~KWSGGaussianMixture() {}

const ALString KWSGGaussianMixture::GetName() const
{
	return "GaussianMixture";
}

void KWSGGaussianMixture::GenerateObjectValues(KWObject* kwoObject)
{
	double dProbClass;
	double dProbX;
	double dProbY;
	Continuous cX;
	Continuous cY;

	require(CheckObject(kwoObject));
	require(ivAttributeTypes.GetSize() == 2);
	require(ivAttributeTypes.GetAt(0) == KWType::Continuous);
	require(ivAttributeTypes.GetAt(1) == KWType::Continuous);
	require(GetNoiseRate() == 0);

	// Generation aleatoires independantes
	dProbClass = GetRandomDouble();
	dProbX = GetRandomDouble();
	dProbY = GetRandomDouble();

	// La classe est generes aleatoirement
	if (dProbClass >= 0.5)
	{
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetPlusClass());

		// Generation d'un point de la Gaussienne de la classe Plus
		cX = (Continuous)KWStat::InvNormal(dProbX, dPlusClassXMean, dPlusClassXStandardDeviation);
		cY = (Continuous)KWStat::InvNormal(dProbY, dPlusClassYMean, dPlusClassYStandardDeviation);
		kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(0), cX);
		kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(1), cY);
	}
	else
	{
		kwoObject->SetSymbolValueAt(GetClassLoadIndex(), GetMinusClass());

		// Generation d'un point de la Gaussienne de la classe Minus
		cX = (Continuous)KWStat::InvNormal(dProbX, dMinusClassXMean, dMinusClassXStandardDeviation);
		cY = (Continuous)KWStat::InvNormal(dProbY, dMinusClassYMean, dMinusClassYStandardDeviation);
		kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(0), cX);
		kwoObject->SetContinuousValueAt(livAttributeLoadIndexes.GetAt(1), cY);
	}
}

void KWSGGaussianMixture::SetMinusClassXMean(double dValue)
{
	dMinusClassXMean = dValue;
}

double KWSGGaussianMixture::GetMinusClassXMean() const
{
	return dMinusClassXMean;
}

void KWSGGaussianMixture::SetMinusClassYMean(double dValue)
{
	dMinusClassYMean = dValue;
}

double KWSGGaussianMixture::GetMinusClassYMean() const
{
	return dMinusClassYMean;
}

void KWSGGaussianMixture::SetMinusClassXStandardDeviation(double dValue)
{
	require(dValue > 0);
	dMinusClassXStandardDeviation = dValue;
}

double KWSGGaussianMixture::GetMinusClassXStandardDeviation() const
{
	return dMinusClassXStandardDeviation;
}

void KWSGGaussianMixture::SetMinusClassYStandardDeviation(double dValue)
{
	require(dValue > 0);
	dMinusClassYStandardDeviation = dValue;
}

double KWSGGaussianMixture::GetMinusClassYStandardDeviation() const
{
	return dMinusClassYStandardDeviation;
}

void KWSGGaussianMixture::SetPlusClassXMean(double dValue)
{
	dPlusClassXMean = dValue;
}

double KWSGGaussianMixture::GetPlusClassXMean() const
{
	return dPlusClassXMean;
}

void KWSGGaussianMixture::SetPlusClassYMean(double dValue)
{
	dPlusClassYMean = dValue;
}

double KWSGGaussianMixture::GetPlusClassYMean() const
{
	return dPlusClassYMean;
}

void KWSGGaussianMixture::SetPlusClassXStandardDeviation(double dValue)
{
	require(dValue > 0);
	dPlusClassXStandardDeviation = dValue;
}

double KWSGGaussianMixture::GetPlusClassXStandardDeviation() const
{
	return dPlusClassXStandardDeviation;
}

void KWSGGaussianMixture::SetPlusClassYStandardDeviation(double dValue)
{
	require(dValue > 0);
	dPlusClassYStandardDeviation = dValue;
}

double KWSGGaussianMixture::GetPlusClassYStandardDeviation() const
{
	return dPlusClassYStandardDeviation;
}

double KWSGGaussianMixture::ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass)
{
	double dLaplaceEpsilon;
	double cX;
	double cY;
	double dPlusClassProb;
	double dMinusClassProb;

	require(CheckObject(kwoObject));
	require(ivAttributeTypes.GetSize() == 2);
	require(ivAttributeTypes.GetAt(0) == KWType::Continuous);
	require(ivAttributeTypes.GetAt(1) == KWType::Continuous);

	// Recherche des valeurs de l'objet
	cX = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(0));
	cY = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(1));

	// Recherche des probabilites de chacune des classes (sans le coefficient constant 1/Pi)
	dPlusClassProb = (1.0 / (dPlusClassXStandardDeviation * dPlusClassYStandardDeviation)) *
			 exp(-0.5 * pow((cX - dPlusClassXMean) / dPlusClassXStandardDeviation, 2)) *
			 exp(-0.5 * pow((cY - dPlusClassYMean) / dPlusClassYStandardDeviation, 2));
	dMinusClassProb = (1.0 / (dMinusClassXStandardDeviation * dMinusClassYStandardDeviation)) *
			  exp(-0.5 * pow((cX - dMinusClassXMean) / dMinusClassXStandardDeviation, 2)) *
			  exp(-0.5 * pow((cY - dMinusClassYMean) / dMinusClassYStandardDeviation, 2));

	// Gestion des valeurs proches de 0
	dLaplaceEpsilon = 1e-5;
	dPlusClassProb += dLaplaceEpsilon;
	dMinusClassProb += dLaplaceEpsilon;

	// La vraie classe depend la parite de la somme des coordonnees des modalites
	if (sClass == GetPlusClass())
		return dPlusClassProb / (dMinusClassProb + dPlusClassProb);
	else
		return dMinusClassProb / (dMinusClassProb + dPlusClassProb);
}

void KWSGGaussianMixture::ComputeGaussianParameters(KWDatabase* trainDatabase)
{
	DoubleVector dvPlusClassX;
	DoubleVector dvPlusClassY;
	DoubleVector dvMinusClassX;
	DoubleVector dvMinusClassY;
	int nObject;
	KWObject* kwoObject;
	double cX;
	double cY;
	Symbol sClass;

	require(trainDatabase != NULL);

	// Alimentation des vecteurs de coordonnees par classe
	for (nObject = 0; nObject < trainDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, trainDatabase->GetObjects()->GetAt(nObject));

		// Recherche des valeurs de l'objet
		cX = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(0));
		cY = kwoObject->GetContinuousValueAt(livAttributeLoadIndexes.GetAt(1));
		sClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);

		// Recherche des coordonnees selon la classe
		if (sClass == GetPlusClass())
		{
			dvPlusClassX.Add(cX);
			dvPlusClassY.Add(cY);
		}
		else
		{
			dvMinusClassX.Add(cX);
			dvMinusClassY.Add(cY);
		}
	}

	// Alimentation des stats par defaut
	dMinusClassXMean = 0;
	dMinusClassYMean = 0;
	dMinusClassXStandardDeviation = 1;
	dMinusClassYStandardDeviation = 1;
	dPlusClassXMean = 0;
	dPlusClassYMean = 0;
	dPlusClassXStandardDeviation = 1;
	dPlusClassYStandardDeviation = 1;

	// Alimentation des stats
	if (dvMinusClassX.GetSize() > 0)
	{
		dMinusClassXMean = KWStat::Mean(&dvMinusClassX);
		dMinusClassYMean = KWStat::Mean(&dvMinusClassY);
	}
	if (dvMinusClassX.GetSize() > 1)
	{
		dMinusClassXStandardDeviation = KWStat::StandardDeviation(&dvMinusClassX);
		dMinusClassYStandardDeviation = KWStat::StandardDeviation(&dvMinusClassY);
	}
	if (dvPlusClassX.GetSize() > 0)
	{
		dPlusClassXMean = KWStat::Mean(&dvPlusClassX);
		dPlusClassYMean = KWStat::Mean(&dvPlusClassY);
	}
	if (dvPlusClassX.GetSize() > 1)
	{
		dPlusClassXStandardDeviation = KWStat::StandardDeviation(&dvPlusClassX);
		dPlusClassYStandardDeviation = KWStat::StandardDeviation(&dvPlusClassY);
	}
}

double KWSGGaussianMixture::ComputeObjectGMError(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject)
{
	double dError;
	Symbol sPredictedClass;
	Symbol sTrueClass;

	// Acces aux valeurs predites et reelles
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	if (gaussianMixture->ComputeTrueProb(kwoObject, GetMinusClass()) >=
	    gaussianMixture->ComputeTrueProb(kwoObject, GetPlusClass()))
		sPredictedClass = GetMinusClass();
	else
		sPredictedClass = GetPlusClass();

	// Calcul de l'erreur
	if (sPredictedClass == sTrueClass)
		dError = 0;
	else
		dError = 1;
	return dError;
}

double KWSGGaussianMixture::ComputeObjectGMDKL(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject)
{
	boolean bDisplay = false;
	double dDKL;
	double dProbPlus;
	double dProbMinus;
	double dTrueProbPlus;
	double dTrueProbMinus;
	Symbol sTrueClass;

	// Calcul des probabilites vraies et predites par classe
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	dProbPlus = gaussianMixture->ComputeTrueProb(kwoObject, GetPlusClass());
	dProbMinus = gaussianMixture->ComputeTrueProb(kwoObject, GetMinusClass());
	dTrueProbPlus = ComputeNoisyTrueProb(kwoObject, GetPlusClass());
	dTrueProbMinus = ComputeNoisyTrueProb(kwoObject, GetMinusClass());

	// Verifications
	assert(0 <= dProbPlus and dProbPlus <= 1);
	assert(0 <= dProbMinus and dProbMinus <= 1);
	assert(fabs(dProbPlus + dProbMinus - 1) < 1e-5);
	assert(0 <= dTrueProbPlus and dTrueProbPlus <= 1);
	assert(0 <= dTrueProbMinus and dTrueProbMinus <= 1);
	assert(fabs(dTrueProbPlus + dTrueProbMinus - 1) < 1e-5);

	// Calcul du critere
	dDKL = 0;
	if (dTrueProbPlus > 0)
		dDKL += dTrueProbPlus * log(dTrueProbPlus / dProbPlus);
	if (dTrueProbMinus > 0)
		dDKL += dTrueProbMinus * log(dTrueProbMinus / dProbMinus);

	// Affichage
	if (bDisplay)
	{
		// Attributs de l'objets
		if (ivAttributeTypes.GetAt(0) == KWType::Continuous)
			cout << kwoObject->ComputeContinuousValueAt(livAttributeLoadIndexes.GetAt(0)) << "\t";
		else
			cout << kwoObject->ComputeSymbolValueAt(livAttributeLoadIndexes.GetAt(0)) << "\t";
		if (ivAttributeTypes.GetAt(1) == KWType::Continuous)
			cout << kwoObject->ComputeContinuousValueAt(livAttributeLoadIndexes.GetAt(1)) << "\t";
		else
			cout << kwoObject->ComputeSymbolValueAt(livAttributeLoadIndexes.GetAt(1)) << "\t";
		cout << sTrueClass << "\t";

		// Probabilites predites
		cout << dProbMinus << "\t" << dProbPlus << "\t";

		// Probabilites vraies
		cout << dTrueProbMinus << "\t" << dTrueProbPlus << "\t";

		// DKL
		cout << dDKL << "\n";
	}

	return dDKL;
}

double KWSGGaussianMixture::ComputeObjectGMMSE(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject)
{
	double dMSE;
	double dProbPlus;
	double dProbMinus;
	double dTrueProbPlus;
	double dTrueProbMinus;
	Symbol sTrueClass;

	// Calcul des probabilites vraies et predites par classe
	sTrueClass = kwoObject->GetSymbolValueAt(liTargetAttributeLoadIndex);
	dProbPlus = gaussianMixture->ComputeTrueProb(kwoObject, GetPlusClass());
	dProbMinus = gaussianMixture->ComputeTrueProb(kwoObject, GetMinusClass());
	dTrueProbPlus = ComputeNoisyTrueProb(kwoObject, GetPlusClass());
	dTrueProbMinus = ComputeNoisyTrueProb(kwoObject, GetMinusClass());

	// Verifications
	assert(0 <= dProbPlus and dProbPlus <= 1);
	assert(0 <= dProbMinus and dProbMinus <= 1);
	assert(fabs(dProbPlus + dProbMinus - 1) < 1e-5);
	assert(0 <= dTrueProbPlus and dTrueProbPlus <= 1);
	assert(0 <= dTrueProbMinus and dTrueProbMinus <= 1);
	assert(fabs(dTrueProbPlus + dTrueProbMinus - 1) < 1e-5);

	// Calcul du critere
	if (sTrueClass == GetPlusClass())
		dMSE = pow(dTrueProbPlus - dProbPlus, 2);
	else
		dMSE = pow(dTrueProbMinus - dProbMinus, 2);
	return dMSE;
}

double KWSGGaussianMixture::ComputeDatabaseGMError(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectGMError(gaussianMixture, kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

double KWSGGaussianMixture::ComputeDatabaseGMDKL(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectGMDKL(gaussianMixture, kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

double KWSGGaussianMixture::ComputeDatabaseGMMSE(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase)
{
	double dValue;
	int nObject;
	KWObject* kwoObject;

	require(testDatabase != NULL);

	// Calcul de la moyenne sur la base
	dValue = 0;
	for (nObject = 0; nObject < testDatabase->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, testDatabase->GetObjects()->GetAt(nObject));
		dValue += ComputeObjectGMMSE(gaussianMixture, kwoObject);
	}
	if (testDatabase->GetObjects()->GetSize() > 0)
		dValue /= testDatabase->GetObjects()->GetSize();
	return dValue;
}

const ALString KWSGGaussianMixture::GetClassLabel() const
{
	return "GaussianMixture";
}

const ALString KWSGGaussianMixture::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = sLabel + "Minus " + "(" + DoubleToString(dMinusClassXMean) + " " + DoubleToString(dMinusClassYMean) +
		 ")" + " (" + DoubleToString(dMinusClassXStandardDeviation) + " " +
		 DoubleToString(dMinusClassYStandardDeviation) + ")" + " ";
	sLabel = sLabel + "Plus " + "(" + DoubleToString(dPlusClassXMean) + " " + DoubleToString(dPlusClassYMean) +
		 ")" + " (" + DoubleToString(dPlusClassXStandardDeviation) + " " +
		 DoubleToString(dPlusClassYStandardDeviation) + ")";
	return sLabel;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSampleDataGridTest

KWSampleDataGridTest::KWSampleDataGridTest()
{
	// Initialisation des attributs d'instances
	dataGridOptimizationParameters = NULL;
	nSampleSize = 0;
	nSampleNumber = 0;
	nTestDatabaseSize = 0;
	nFreshness = 0;
	nStatsFreshness = 0;
	nOptimizationParametersFreshness = 0;
	nSampleExportNumber = -1;
	sampleGenerator = NULL;
}

KWSampleDataGridTest::~KWSampleDataGridTest()
{
	oaUnivariatePartitionSizes.DeleteAll();
}

void KWSampleDataGridTest::SetGenerator(KWSampleGenerator* kwcsgSampleGenerator)
{
	sampleGenerator = kwcsgSampleGenerator;
}

KWSampleGenerator* KWSampleDataGridTest::GetGenerator()
{
	return sampleGenerator;
}

void KWSampleDataGridTest::SetDataGridOptimizerParameters(KWDataGridOptimizerParameters* optimizationParameters)
{
	dataGridOptimizationParameters = optimizationParameters;
	nFreshness++;
}

KWDataGridOptimizerParameters* KWSampleDataGridTest::GetDataGridOptimizerParameters()
{
	return dataGridOptimizationParameters;
}

void KWSampleDataGridTest::SetSampleSize(int nValue)
{
	require(nValue >= 0);

	nSampleSize = nValue;
	nFreshness++;
}

int KWSampleDataGridTest::GetSampleSize() const
{
	return nSampleSize;
}

void KWSampleDataGridTest::SetSampleNumber(int nValue)
{
	require(nValue >= 0);

	nSampleNumber = nValue;
	nFreshness++;
}

int KWSampleDataGridTest::GetSampleNumber() const
{
	return nSampleNumber;
}

void KWSampleDataGridTest::SetTestDatabaseSize(int nValue)
{
	require(nValue >= 0);

	nTestDatabaseSize = nValue;
	nFreshness++;
}

int KWSampleDataGridTest::GetTestDatabaseSize() const
{
	return nTestDatabaseSize;
}

void KWSampleDataGridTest::SetSampleExportNumber(int nValue)
{
	nSampleExportNumber = nValue;
}

int KWSampleDataGridTest::GetSampleExportNumber() const
{
	return nSampleExportNumber;
}

boolean KWSampleDataGridTest::Check() const
{
	return sampleGenerator != NULL and dataGridOptimizationParameters != NULL and
	       dataGridOptimizationParameters->Check();
}

boolean KWSampleDataGridTest::ComputeStats()
{
	boolean bExportSampleFiles = false;
	boolean bExportStatFiles = false;
	KWSTDatabaseTextFile sampleTrainDatabase;
	KWSTDatabaseTextFile sampleTestDatabase;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable tupleTable;
	StringVector svAttributeNames;
	KWLearningSpec sampleLearningSpec;
	KWAttributeSubsetStats attributeSubsetStats;
	KWSGGaussianMixture gaussianMixtureTrained;
	KWSGGaussianMixture* gaussianMixtureGenerator;
	KWClass* kwcSampleTestClass;
	int i;
	KWObject* kwoObject;
	int nRun;
	int nAttribute;
	const KWDGSAttributePartition* dgsAttribute;
	const ALString sAttributeName = "X";
	DoubleVector* dvUnivariatePartitionSizes;
	KWDataGridClassificationCosts classificationCosts;
	double dCost;
	clock_t tStartClock;
	clock_t tStopClock;
	double dTime;

	require(Check());

	// Preparation des vecteurs de resultats
	dvErrors.SetSize(0);
	dvDKLs.SetSize(0);
	dvMSEs.SetSize(0);
	dvCosts.SetSize(0);
	dvTimes.SetSize(0);
	dvCellNumbers.SetSize(0);
	dvNoInformativeAttributeNumbers.SetSize(0);
	dvInformativeAttributeNumbers.SetSize(0);
	dvGMErrors.SetSize(0);
	dvGMDKLs.SetSize(0);
	dvGMMSEs.SetSize(0);
	dvErrors.SetSize(nSampleNumber);
	dvDKLs.SetSize(nSampleNumber);
	dvMSEs.SetSize(nSampleNumber);
	dvCosts.SetSize(nSampleNumber);
	dvTimes.SetSize(nSampleNumber);
	dvCellNumbers.SetSize(nSampleNumber);
	dvNoInformativeAttributeNumbers.SetSize(nSampleNumber);
	dvInformativeAttributeNumbers.SetSize(nSampleNumber);
	dvGMErrors.SetSize(nSampleNumber);
	dvGMDKLs.SetSize(nSampleNumber);
	dvGMMSEs.SetSize(nSampleNumber);
	oaUnivariatePartitionSizes.DeleteAll();
	oaUnivariatePartitionSizes.SetSize(GetGenerator()->GetAttributeTypes()->GetSize());
	for (nAttribute = 0; nAttribute < GetGenerator()->GetAttributeTypes()->GetSize(); nAttribute++)
	{
		dvUnivariatePartitionSizes = new DoubleVector;
		dvUnivariatePartitionSizes->SetSize(nSampleNumber);
		oaUnivariatePartitionSizes.SetAt(nAttribute, dvUnivariatePartitionSizes);
	}

	// Test si le generateur est un generateur de melange de Gaussienne
	// Code peu propre, mais moyen rapide de collecter des statistiques specifiquement pour ce generateur
	gaussianMixtureGenerator = NULL;
	if (sampleGenerator->GetName() == "GaussianMixture")
		gaussianMixtureGenerator = cast(KWSGGaussianMixture*, sampleGenerator);

	// Acces a la classe de test
	kwcSampleTestClass = sampleGenerator->GetSampleClass();

	// Initialisation de la base et des specifications d'apprentissage
	sampleTrainDatabase.SetClassName(kwcSampleTestClass->GetName());
	sampleTrainDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + ".txt");
	sampleLearningSpec.SetClass(kwcSampleTestClass);
	sampleLearningSpec.SetDatabase(&sampleTrainDatabase);
	sampleLearningSpec.SetTargetAttributeName("Class");
	sampleLearningSpec.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->CopyFrom(
	    dataGridOptimizationParameters);

	// Initialisation de la base de test
	sampleTestDatabase.SetClassName(kwcSampleTestClass->GetName());
	sampleTestDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + "_Test.txt");

	// Alimentation de la base de test (la meme quelque soit la base d'apprentissage)
	for (i = 0; i < nTestDatabaseSize; i++)
	{
		kwoObject = new KWObject(kwcSampleTestClass, i + 1);
		sampleTestDatabase.GetObjects()->Add(kwoObject);
	}
	GenerateSample(&sampleTestDatabase);

	// Parametrage de l'ensemble d'attributs a analyser
	attributeSubsetStats.SetLearningSpec(&sampleLearningSpec);
	attributeSubsetStats.SetAttributeNumber(GetGenerator()->GetAttributeTypes()->GetSize());
	for (nAttribute = 0; nAttribute < GetGenerator()->GetAttributeTypes()->GetSize(); nAttribute++)
	{
		attributeSubsetStats.SetAttributeNameAt(nAttribute, sAttributeName + IntToString(nAttribute + 1));
	}

	// Creation des objets de la base d'apprentissage
	for (i = 0; i < nSampleSize; i++)
	{
		kwoObject = new KWObject(kwcSampleTestClass, i + 1);
		sampleTrainDatabase.GetObjects()->Add(kwoObject);
	}

	// Repetition de l'experimentation
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		// On fixe la graine aleatoire
		SetRandomSeed(nRun);

		// Dans le cas du XOR, randomisation des variables du XOR
		if (sampleGenerator->GetName() == "XOR")
		{
			int nLastRandomSeed = GetRandomSeed();
			SetRandomSeed(nRun);
			cast(KWSGMultivariateXORContinuous*, sampleGenerator)->RandomizeXORVariables();
			SetRandomSeed(nLastRandomSeed);
		}

		// Generation des valeurs des exemples
		sampleTrainDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + "_" +
						    GetGenerator()->GetFullName() + "_" + IntToString(nSampleSize) +
						    "_" + IntToString(nRun + 1) + ".txt");
		GenerateSample(&sampleTrainDatabase);

		// Export de la base
		if (bExportSampleFiles or nRun == nSampleExportNumber)
			sampleTrainDatabase.WriteAll(&sampleTrainDatabase);
		tupleTableLoader.SetInputClass(kwcSampleTestClass);
		tupleTableLoader.SetInputDatabaseObjects(sampleTrainDatabase.GetObjects());

		// Calcul des stats sur l'attribut cible
		tupleTableLoader.LoadUnivariate(sampleLearningSpec.GetTargetAttributeName(), &tupleTable);
		sampleLearningSpec.ComputeTargetStats(&tupleTable);

		// Chargement de la table de tuple en multivarie
		svAttributeNames.SetSize(attributeSubsetStats.GetAttributeNumber() + 1);
		for (nAttribute = 0; nAttribute < attributeSubsetStats.GetAttributeNumber(); nAttribute++)
			svAttributeNames.SetAt(nAttribute, attributeSubsetStats.GetAttributeNameAt(nAttribute));
		svAttributeNames.SetAt(svAttributeNames.GetSize() - 1, sampleLearningSpec.GetTargetAttributeName());
		tupleTableLoader.LoadMultivariate(&svAttributeNames, &tupleTable);

		// Calcul de la grille de donnees
		tStartClock = clock();
		attributeSubsetStats.ComputeStats(&tupleTable);
		tStopClock = clock();
		dTime = (tStopClock - tStartClock) * 1.0 / CLOCKS_PER_SEC;
		dCost = attributeSubsetStats.GetSortValue();

		// Apprentissage des parametre d'un melange de Gaussiennes
		if (gaussianMixtureGenerator != NULL)
			gaussianMixtureTrained.ComputeGaussianParameters(&sampleTrainDatabase);

		// Initialisation du generateur avec les statistiques multivariee pour permettre
		// les calcul de performances
		sampleGenerator->InitFromAttributeSubsetStats(&attributeSubsetStats);

		// Export du fichier de stats resultats
		if (bExportStatFiles or nRun == nSampleExportNumber)
			attributeSubsetStats.WriteReportFile(
			    kwcSampleTestClass->GetName() + "_" + GetGenerator()->GetFullName() + "_" +
			    IntToString(nSampleSize) + "_" + IntToString(nRun + 1) + ".xls");

		// Memorisation des resultats
		dvErrors.SetAt(nRun, sampleGenerator->ComputeDatabaseError(&sampleTestDatabase));
		dvDKLs.SetAt(nRun, sampleGenerator->ComputeDatabaseDKL(&sampleTestDatabase));
		dvMSEs.SetAt(nRun, sampleGenerator->ComputeDatabaseMSE(&sampleTestDatabase));
		dvCosts.SetAt(nRun, dCost);
		dvTimes.SetAt(nRun, dTime);
		dvCellNumbers.SetAt(nRun, attributeSubsetStats.GetPreparedDataGridStats()->ComputeCellNumber());
		dvNoInformativeAttributeNumbers.SetAt(
		    nRun, attributeSubsetStats.GetPreparedDataGridStats()->ComputeInformativeAttributeNumber() == 0);
		dvInformativeAttributeNumbers.SetAt(
		    nRun, attributeSubsetStats.GetPreparedDataGridStats()->ComputeInformativeAttributeNumber());
		if (gaussianMixtureGenerator != NULL)
		{
			dvGMErrors.SetAt(nRun, gaussianMixtureGenerator->ComputeDatabaseGMError(&gaussianMixtureTrained,
												&sampleTestDatabase));
			dvGMDKLs.SetAt(nRun, gaussianMixtureGenerator->ComputeDatabaseGMDKL(&gaussianMixtureTrained,
											    &sampleTestDatabase));
			dvGMMSEs.SetAt(nRun, gaussianMixtureGenerator->ComputeDatabaseGMMSE(&gaussianMixtureTrained,
											    &sampleTestDatabase));
		}
		for (nAttribute = 0; nAttribute < GetGenerator()->GetAttributeTypes()->GetSize(); nAttribute++)
		{
			dvUnivariatePartitionSizes = cast(DoubleVector*, oaUnivariatePartitionSizes.GetAt(nAttribute));
			dgsAttribute = attributeSubsetStats.GetPreparedDataGridStats()->SearchAttribute(
			    sAttributeName + IntToString(nAttribute + 1));
			if (dgsAttribute == NULL)
				dvUnivariatePartitionSizes->SetAt(nRun, 1);
			else
				dvUnivariatePartitionSizes->SetAt(nRun, dgsAttribute->GetPartNumber());
		}

		// (sans cette instruction forcant une entree-sortie, les simulations
		// tres longues s'interrompent mysterieusement
		if (nRun % 1000 == 0 and nSampleNumber > 1000)
			cout << " " << flush;
	}

	// Memorisation des informations de fraicheur
	nStatsFreshness = nFreshness;
	nOptimizationParametersFreshness = dataGridOptimizationParameters->GetFreshness();
	ensure(IsStatsComputed());
	return IsStatsComputed();
}

boolean KWSampleDataGridTest::IsStatsComputed() const
{
	return nStatsFreshness == nFreshness and dataGridOptimizationParameters != NULL and
	       nOptimizationParametersFreshness == dataGridOptimizationParameters->GetFreshness();
}

DoubleVector* KWSampleDataGridTest::GetErrors()
{
	require(IsStatsComputed());
	return &dvErrors;
}

DoubleVector* KWSampleDataGridTest::GetDKLs()
{
	require(IsStatsComputed());
	return &dvDKLs;
}

DoubleVector* KWSampleDataGridTest::GetMSEs()
{
	require(IsStatsComputed());
	return &dvMSEs;
}

DoubleVector* KWSampleDataGridTest::GetCosts()
{
	require(IsStatsComputed());
	return &dvCosts;
}

DoubleVector* KWSampleDataGridTest::GetTimes()
{
	require(IsStatsComputed());
	return &dvTimes;
}

DoubleVector* KWSampleDataGridTest::GetCellNumbers()
{
	require(IsStatsComputed());
	return &dvCellNumbers;
}

DoubleVector* KWSampleDataGridTest::GetNoInformativeAttributeNumbers()
{
	require(IsStatsComputed());
	return &dvNoInformativeAttributeNumbers;
}

DoubleVector* KWSampleDataGridTest::GetInformativeAttributeNumbers()
{
	require(IsStatsComputed());
	return &dvInformativeAttributeNumbers;
}

DoubleVector* KWSampleDataGridTest::GetGMErrors()
{
	require(IsStatsComputed());
	return &dvGMErrors;
}

DoubleVector* KWSampleDataGridTest::GetGMDKLs()
{
	require(IsStatsComputed());
	return &dvGMDKLs;
}

DoubleVector* KWSampleDataGridTest::GetGMMSEs()
{
	require(IsStatsComputed());
	return &dvGMMSEs;
}

DoubleVector* KWSampleDataGridTest::GetPartNumbersAt(int nAttribute)
{
	require(IsStatsComputed());
	require(0 <= nAttribute and nAttribute < GetGenerator()->GetAttributeTypes()->GetSize());
	return cast(DoubleVector*, oaUnivariatePartitionSizes.GetAt(nAttribute));
}

void KWSampleDataGridTest::WriteReportFile(const ALString& sFileName) const
{
	fstream ost;
	boolean bOk;

	require(IsStatsComputed());

	bOk = FileService::OpenOutputFile(sFileName, ost);
	if (bOk)
	{
		WriteReport(ost);
		FileService::CloseOutputFile(sFileName, ost);
	}
}

void KWSampleDataGridTest::WriteReport(ostream& ost) const
{
	int nRun;
	int nAttribute;
	const ALString sAttributeName = "X";
	DoubleVector* dvUnivariatePartitionSizes;

	require(IsStatsComputed());

	// Specifications des tests
	ost << "Generateur\t" << sampleGenerator->GetFullName() << "\n";
	ost << "Optimisation\t" << dataGridOptimizationParameters->GetObjectLabel() << "\n";
	ost << "Taille des echantillons\t" << nSampleSize << "\n";
	ost << "Nombre d'echantillons\t" << nSampleNumber << "\n";
	ost << "\n";

	// Statistiques par echantillon
	ost << "Run\tInterval Number\tError\tDKL\tMSE\tCost\tTime\tCells\tNo inf. att.\tInf. att.";
	for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
		ost << "\t" << sAttributeName << nAttribute + 1;
	ost << "\n";
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		ost << nRun + 1 << "\t" << dvErrors.GetAt(nRun) << "\t" << dvDKLs.GetAt(nRun) << "\t"
		    << dvMSEs.GetAt(nRun) << "\t" << dvCosts.GetAt(nRun) << "\t" << dvTimes.GetAt(nRun) << "\t"
		    << dvCellNumbers.GetAt(nRun) << "\t" << dvNoInformativeAttributeNumbers.GetAt(nRun) << "\t"
		    << dvInformativeAttributeNumbers.GetAt(nRun) << "\t" << dvGMErrors.GetAt(nRun) << "\t"
		    << dvGMDKLs.GetAt(nRun) << "\t" << dvGMMSEs.GetAt(nRun);
		for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
		{
			dvUnivariatePartitionSizes = cast(DoubleVector*, oaUnivariatePartitionSizes.GetAt(nAttribute));
			ost << "\t" << dvUnivariatePartitionSizes->GetAt(nRun);
		}
		ost << "\n";
	}
	ost << endl;
}

void KWSampleDataGridTest::WriteHeaderLineReport(ostream& ost) const
{
	int nAttribute;
	const ALString sAttributeName = "X";

	// Specifications des tests
	ost << "Generator\t";
	ost << "Parameters\t";
	ost << "Sample size\t";
	ost << "Sample number\t";

	// Moyenne des statistiques calculees
	ost << "Error\t";
	ost << "DKL\t";
	ost << "MSE\t";
	ost << "Cost\t";
	ost << "Time\t";
	ost << "Cells\t";
	ost << "No inf. att.\t";
	ost << "Inf. att.\t";
	ost << "GMError\t";
	ost << "GMDKL\t";
	ost << "GMMSE";
	for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
		ost << "\t" << sAttributeName << nAttribute + 1;
	ost << "\t";

	// Ecart type des statistiques calculees
	ost << "Error SD\t";
	ost << "DKL SD\t";
	ost << "MSE SD\t";
	ost << "Cost SD\t";
	ost << "Time SD\t";
	ost << "Cells SD\t";
	ost << "No inf. att. SD\t";
	ost << "Inf. att. SD\t";
	ost << "GMError SD\t";
	ost << "GMDKL SD\t";
	ost << "GMMSE SD";
	for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
		ost << "\t" << sAttributeName << nAttribute + 1 << " SD";
	ost << flush;
}

void KWSampleDataGridTest::WriteLineReport(ostream& ost) const
{
	int nAttribute;
	DoubleVector* dvUnivariatePartitionSizes;

	// Specifications des tests
	ost << sampleGenerator->GetFullName() << "\t";
	ost << dataGridOptimizationParameters->GetObjectLabel() << "\t";
	ost << nSampleSize << "\t";
	ost << nSampleNumber << "\t";

	// Moyennes des statistiques calculees
	ost << KWStat::Mean(&dvErrors) << "\t";
	ost << KWStat::Mean(&dvDKLs) << "\t";
	ost << KWStat::Mean(&dvMSEs) << "\t";
	ost << KWStat::Mean(&dvCosts) << "\t";
	ost << KWStat::Mean(&dvTimes) << "\t";
	ost << KWStat::Mean(&dvCellNumbers) << "\t";
	ost << KWStat::Mean(&dvNoInformativeAttributeNumbers) << "\t";
	ost << KWStat::Mean(&dvInformativeAttributeNumbers) << "\t";
	ost << KWStat::Mean(&dvGMErrors) << "\t";
	ost << KWStat::Mean(&dvGMDKLs) << "\t";
	ost << KWStat::Mean(&dvGMMSEs);
	for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
	{
		dvUnivariatePartitionSizes = cast(DoubleVector*, oaUnivariatePartitionSizes.GetAt(nAttribute));
		ost << "\t" << KWStat::Mean(dvUnivariatePartitionSizes);
	}
	ost << "\t";

	// Ecart type des statistiques calculees
	ost << KWStat::StandardDeviation(&dvErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvDKLs) << "\t";
	ost << KWStat::StandardDeviation(&dvMSEs) << "\t";
	ost << KWStat::StandardDeviation(&dvCosts) << "\t";
	ost << KWStat::StandardDeviation(&dvTimes) << "\t";
	ost << KWStat::StandardDeviation(&dvCellNumbers) << "\t";
	ost << KWStat::StandardDeviation(&dvNoInformativeAttributeNumbers) << "\t";
	ost << KWStat::StandardDeviation(&dvInformativeAttributeNumbers) << "\t";
	ost << KWStat::StandardDeviation(&dvGMErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvGMDKLs) << "\t";
	ost << KWStat::StandardDeviation(&dvGMMSEs);
	for (nAttribute = 0; nAttribute < sampleGenerator->GetAttributeTypes()->GetSize(); nAttribute++)
	{
		dvUnivariatePartitionSizes = cast(DoubleVector*, oaUnivariatePartitionSizes.GetAt(nAttribute));
		ost << "\t" << KWStat::StandardDeviation(dvUnivariatePartitionSizes);
	}
	ost << flush;
}

void KWSampleDataGridTest::GenerateSample(KWDatabase* database)
{
	KWObject* kwoObject;
	int i;

	require(database != NULL);
	require(database->GetClassName() == sampleGenerator->GetSampleClass()->GetName());

	// Generation de l'echantillon
	for (i = 0; i < database->GetObjects()->GetSize(); i++)
	{
		// Recherche de l'objet a modifier
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(i));

		// Generation aleatoire des valeurs d'une instance, avec bruitage de la classe
		sampleGenerator->GenerateObjectValuesAndNoisyClass(kwoObject);
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWDataGridTest

KWDataGridTest::KWDataGridTest()
{
	int nSize;

	InitializeOptimizationParametersArray();

	nSize = 4;
	while (nSize < 200000)
	{
		ivSampleSizes.Add(nSize);
		ivSampleSizes.Add(nSize + nSize / 2);
		nSize *= 2;
	}
	// Initialisation des tailles d'echantillon
	/*
	int nBaseSampleSize = 100;
	int i;
	ivSampleSizes.Add(10);
	ivSampleSizes.Add(12);
	ivSampleSizes.Add(13);
	ivSampleSizes.Add(14);
	ivSampleSizes.Add(15);
	ivSampleSizes.Add(16);
	ivSampleSizes.Add(20);
	ivSampleSizes.Add(30);
	ivSampleSizes.Add(40);
	ivSampleSizes.Add(50);
	ivSampleSizes.Add(60);
	ivSampleSizes.Add(70);
	ivSampleSizes.Add(80);
	ivSampleSizes.Add(90);
	for (i = 1; i <= 20; i++)
	ivSampleSizes.Add(i*nBaseSampleSize);
	ivSampleSizes.Add(25*nBaseSampleSize);
	for (i = 10; i <= 20; i++)
	ivSampleSizes.Add(i*nBaseSampleSize);
	ivSampleSizes.Add(25*nBaseSampleSize);
	for (i = 30; i <= 100; i += 10)
	ivSampleSizes.Add(i*nBaseSampleSize);
	ivSampleSizes.Add(150*nBaseSampleSize);
	for (i = 200; i <= 500; i += 100)
	ivSampleSizes.Add(i*nBaseSampleSize);
	ivSampleSizes.Add(750*nBaseSampleSize);
	ivSampleSizes.Add(1000*nBaseSampleSize);
	*/
	/*
	nSampleSize = 1;
	while (nSampleSize < 16)
	{
	ivSampleSizes.Add(nSampleSize);
	nSampleSize++;
	}
	nSampleSize = 16;
	while (nSampleSize < 100000000)
	{
	ivSampleSizes.Add(nSampleSize);
	nSampleSize *= 2;
	}
	*/
}

KWDataGridTest::~KWDataGridTest()
{
	oaOptimizationParameters.DeleteAll();
}

void KWDataGridTest::TestSamples(const ALString& sSampleGeneratorName, int nUnivariatePartNumber, double dNoiseRate,
				 int nMinSampleSize, int nMaxSampleSize, int nSampleNumber, int nTestDatabaseSize,
				 int nSampleExportNumber)
{
	KWSampleDataGridTest sampleDataGridTest;
	KWSampleGenerator* sampleGenerator;
	KWDataGridOptimizerParameters* optimizationParameters;
	int nParam;
	int i;
	int nSampleSize;
	fstream ost;
	ALString sFileName;
	boolean bHeader;

	require(sSampleGeneratorName == "RandomContinuous" or sSampleGeneratorName == "RandomSymbol" or
		sSampleGeneratorName == "ChessBoardContinuous" or sSampleGeneratorName == "ChessBoardSymbol" or
		sSampleGeneratorName == "MultivariateXORContinuous" or sSampleGeneratorName == "GaussianMixture");
	require(nUnivariatePartNumber >= 1);
	require(0 <= dNoiseRate and dNoiseRate <= 1);
	require(1 <= nMinSampleSize and nMinSampleSize <= nMaxSampleSize);
	require(nSampleNumber >= 1);
	require(nTestDatabaseSize >= 0);

	// Recherche du generateur
	sampleGenerator = LookupSampleGenerator(sSampleGeneratorName);
	check(sampleGenerator);

	// Parametrage du generateur
	sampleGenerator->SetModalityNumber(nUnivariatePartNumber);
	sampleGenerator->SetNoiseRate(dNoiseRate);

	// Ouverture du fichier de rapport
	cout << "Test with sample generator " << sampleGenerator->GetFullName() << endl;
	cout << "  " << nMinSampleSize << " <= SampleSize <= " << nMaxSampleSize << endl;
	cout << "  " << nSampleNumber << " samples and test database size = " << nTestDatabaseSize << endl;
	sFileName = "Report" + sampleGenerator->GetFullName() + ".xls";
	FileService::OpenOutputFile(sFileName, ost);

	// Parcours des tailles d'echantillon
	bHeader = false;
	for (i = 0; i < ivSampleSizes.GetSize(); i++)
	{
		nSampleSize = ivSampleSizes.GetAt(i);

		// Test de validite de la taille d'echantillon
		if (nMinSampleSize <= nSampleSize and nSampleSize <= nMaxSampleSize)
		{
			cout << "\t" << nSampleSize << "\t" << flush;

			// Parcours des parametres d'optimisation
			for (nParam = 0; nParam < oaOptimizationParameters.GetSize(); nParam++)
			{
				optimizationParameters =
				    cast(KWDataGridOptimizerParameters*, oaOptimizationParameters.GetAt(nParam));

				// Evaluation de la grille de donnees
				sampleDataGridTest.SetGenerator(sampleGenerator);
				sampleDataGridTest.SetDataGridOptimizerParameters(optimizationParameters);
				sampleDataGridTest.SetSampleSize(nSampleSize);
				sampleDataGridTest.SetSampleNumber(nSampleNumber);
				sampleDataGridTest.SetTestDatabaseSize(nTestDatabaseSize);
				sampleDataGridTest.SetSampleExportNumber(nSampleExportNumber);
				sampleDataGridTest.ComputeStats();

				// Impression d'un rapport sous forme de tableau
				if (not bHeader)
				{
					sampleDataGridTest.WriteHeaderLineReport(ost);
					ost << "\n";
					bHeader = true;
				}
				sampleDataGridTest.WriteLineReport(ost);
				ost << "\n";
				if (oaOptimizationParameters.GetSize() > 1)
					cout << ":" << flush;
			}
			cout << endl;
		}
	}
	cout << endl;

	// Nettoyage
	delete sampleGenerator;

	// Fermeture du fichier de rapport
	if (ost.is_open())
		FileService::CloseOutputFile(sFileName, ost);
}

void KWDataGridTest::DataGridTest(int argc, char** argv)
{
	KWDataGridTest dataGridTest;
	ALString sSampleGeneratorName;
	int nUnivariatePartNumber;
	double dNoiseRate;
	int nMinSampleSize;
	int nMaxSampleSize;
	int nSampleNumber;
	int nTestDatabaseSize;
	int nSampleExportNumber;
	boolean bOk;
	ALString sTmp;

	// Debug
	// dataGridTest.TestSamples("GaussianMixture",	1, 0, 50, 100, 1, 100, 0);
	// dataGridTest.TestSamples("ChessBoardSymbol",	2, 0, 2, 2, 1, 0, 0);
	// dataGridTest.TestSamples("ChessBoardSymbol", 512, 0, 4096, 4096, 1, 0, 0);
	// return;

	// Aide si mauvais nombre de parametres
	if (argc != 9)
	{
		cout << "DataGridTest\n"
		     << "\tSampleGeneratorName:\n"
		     << "\t  RandomContinuous, RandomSymbol, MultivariateXORContinuous\n"
		     << "\t  ChessBoardContinuous, ChessBoardSymbol, GaussianMixture\n"
		     << "\tUnivariatePartNumber: number of parts per variable\n"
		     << "\tNoiseRate: percentage of noise\n"
		     << "\tMinSampleSize: min size of train sample\n"
		     << "\tMaxSampleSize: max size of train sample\n"
		     << "\tSampleNumber: number of train samples\n"
		     << "\tTestDatabaseSize: size of test database\n"
		     << "\tSampleExportNumber: index of one sample to export (-1:none)" << endl;
	}
	else
	// Lancement des tests sinon
	{
		// Recuperation des parametres
		sSampleGeneratorName = argv[1];
		nUnivariatePartNumber = StringToInt(argv[2]);
		dNoiseRate = KWContinuous::StringToContinuous(argv[3]);
		nMinSampleSize = StringToInt(argv[4]);
		nMaxSampleSize = StringToInt(argv[5]);
		nSampleNumber = StringToInt(argv[6]);
		nTestDatabaseSize = StringToInt(argv[7]);
		nSampleExportNumber = StringToInt(argv[8]);

		// Test de validite des parametres
		bOk = true;
		if (not(sSampleGeneratorName == "RandomContinuous" or sSampleGeneratorName == "RandomSymbol" or
			sSampleGeneratorName == "ChessBoardContinuous" or sSampleGeneratorName == "ChessBoardSymbol" or
			sSampleGeneratorName == "MultivariateXORContinuous" or
			sSampleGeneratorName == "GaussianMixture"))
		{
			bOk = false;
			cout << "Wrong sample generator name (" << sSampleGeneratorName << ")" << endl;
		}
		if (not(nUnivariatePartNumber >= 1))
		{
			bOk = false;
			cout << "Invalid number of parts per variable (" << nUnivariatePartNumber << ")" << endl;
		}
		if (not(0 <= dNoiseRate and dNoiseRate <= 1))
		{
			bOk = false;
			cout << "Invalid noise rate (" << dNoiseRate << ")" << endl;
		}
		if (not(1 <= nMinSampleSize and nMinSampleSize <= nMaxSampleSize))
		{
			bOk = false;
			cout << "Invalid rand of sample sizes (" << nMinSampleSize << " to " << nMaxSampleSize << ")"
			     << endl;
		}
		if (not(nSampleNumber >= 1))
		{
			bOk = false;
			cout << "Invalid sample number (" << nSampleNumber << ")" << endl;
		}
		if (not(nTestDatabaseSize >= 0))
		{
			bOk = false;
			cout << "Invalid test database size (" << nTestDatabaseSize << ")" << endl;
		}

		// Lancement
		if (bOk)
		{
			dataGridTest.TestSamples(sSampleGeneratorName, nUnivariatePartNumber, dNoiseRate,
						 nMinSampleSize, nMaxSampleSize, nSampleNumber, nTestDatabaseSize,
						 nSampleExportNumber);
		}
	}
}

void KWDataGridTest::TestNonAsymptotic()
{
	const int nMaxUnivariateIntervalNumber = 10;
	const int nMaxMultivariateVariableNumber = 1000;
	int nN;
	int nK;
	int nNInterval;
	int nI;
	double dM0Cost;
	double dMICost;
	int nKs;
	double dG;
	double dN;
	int nNCell;
	double dMGCost;

	// Comparaison des couts de modeles nul de discretisation et de discretisation en I intervalles
	// pour un pattern de type crenaux
	nK = 1;
	cout << "Seuil de detection d'une discretisation informative complexe avec " << nK << "variables" << endl;
	cout << "I\tNInterval\tN\tMICost\tM0Cost" << endl;
	nI = 2;
	while (nI < nMaxUnivariateIntervalNumber)
	{
		// Recherche de l'effectif minimum par intervalle pour obtenir une discretisation informative
		for (nNInterval = 1; nNInterval < 1000000; nNInterval++)
		{
			nN = nNInterval * nI;

			// Calcul du cout du modele nul
			dM0Cost = log(nK + 1.0) + log(nN + 1.0) + KWStat::LnFactorial(nN) -
				  KWStat::LnFactorial(nN / 2) - KWStat::LnFactorial(nN - nN / 2);

			// Calcul du cout du modele en I intervalles
			dMICost = log(nK + 1.0) + log(nK * 1.0) + log(nN * 1.0) + KWStat::LnFactorial(nN + nI - 1) -
				  KWStat::LnFactorial(nN) - KWStat::LnFactorial(nI - 1) + nI * log(nNInterval + 1.0);

			// Test de discretisation informative
			if (dMICost < dM0Cost)
			{
				cout << nI << "\t" << nNInterval << "\t" << nN << "\t" << dMICost << "\t" << dM0Cost
				     << endl;
				break;
			}
		}

		// Passage au nombre d'intervalles suivant
		if (nI < 1000)
			nI++;
		else
			nI *= 2;
	}

	// Comparaison des couts de modeles nul de grille multivariee de Ks variables
	// pour un pattern de type XOR de dimension Ks
	nK = nMaxMultivariateVariableNumber;
	cout << "Seuil de detection d'une grille informative complexe avec " << nK << "variables" << endl;
	cout << "Ks\tNCell\tN\tMGCost\tM0Cost" << endl;
	nKs = 1;
	while (nKs < nMaxMultivariateVariableNumber)
	{
		// Recherche de l'effectif minimum par intervalle pour obtenir une discretisation informative
		for (nNCell = 1; nNCell < 1000000; nNCell++)
		{
			dG = pow(2.0, nKs);
			dN = nNCell * dG;

			// Calcul du cout du modele nul
			if (dN < 1e6)
			{
				nN = (int)dN;
				dM0Cost = log(nK + 1.0) + log(nN + 1.0) + KWStat::LnFactorial(nN) -
					  KWStat::LnFactorial(nN / 2) - KWStat::LnFactorial(nN - nN / 2);
			}
			else
				dM0Cost = log(nK + 1.0) + log(dN + 1.0) + dN * log(2.0);

			// Calcul du cout du modele en I intervalles
			dMGCost = log(nK + 1.0) + KWStat::LnFactorial(nK + nKs - 1) - KWStat::LnFactorial(nK) -
				  KWStat::LnFactorial(nKs - 1) + nKs * (log(dN) + log(dN + 1.0)) +
				  dG * log(nNCell + 1.0);

			// Test de discretisation informative
			if (dMGCost < dM0Cost)
			{
				cout << nKs << "\t" << nNCell << "\t" << dN << "\t" << dMGCost << "\t" << dM0Cost
				     << endl;
				break;
			}
		}

		// Passage au nombre de variables suivant
		if (nKs < 1000)
			nKs++;
		else
			nKs *= 2;
	}
}

void KWDataGridTest::Test()
{
	KWDataGridTest dataGridTest;
	// int nMinSampleSize = 16;
	// int nMaxSampleSize = 1000;
	int nSampleNumber = 10;
	int nTestDatabaseSize = 0;
	double dNoiseRate = 0.1;
	// int nMinPartNumber = 2;
	// int nMaxPartNumber = 100;
	// int nPartNumber;

	dataGridTest.TestSamples("ChessBoardSymbol", 256, dNoiseRate, 1536, 1536, nSampleNumber, nTestDatabaseSize, -1);
	/*
	dataGridTest.TestSamples("ChessBoardContinuous",
	16, dNoiseRate, 768, 768, nSampleNumber, nTestDatabaseSize, -1);
	dataGridTest.TestSamples("ChessBoardSymbol",
	32, dNoiseRate, 128, 128, nSampleNumber, nTestDatabaseSize, -1);
	dataGridTest.TestSamples("ChessBoardSymbol",
	256, dNoiseRate, 1536, 1536, nSampleNumber, nTestDatabaseSize, -1);
	*/
	/*
	nPartNumber = nMinPartNumber;
	while (nPartNumber < nMaxPartNumber/2)
	{
	dataGridTest.TestSamples("ChessBoardContinuous",
	nPartNumber, dNoiseRate, nMinSampleSize, nMaxSampleSize, nSampleNumber, nTestDatabaseSize, -1);
	nPartNumber *= 2;
	}

	nPartNumber = nMinPartNumber;
	while (nPartNumber < nMaxPartNumber/2)
	{
	dataGridTest.TestSamples("ChessBoardSymbol",
	nPartNumber, dNoiseRate, nMinSampleSize, nMaxSampleSize, nSampleNumber, nTestDatabaseSize, -1);
	nPartNumber *= 2;
	}
	*/

	// dataGridTest.TestSamples("GaussianMixture",	1, 0, 50, 100, 1, 100, 0);
	// dataGridTest.TestSamples("ChessBoardSymbol",	2, 0, 2, 2, 1, 0, 0);

	// dataGridTest.TestSamples("ChessBoardSymbol",
	//	16, 0.4, 1, 10000, 100, 1000, -1);
	//	dataGridTest.TestSamples("MultivariateXORContinuous",
	//		2, 0, 64, 200000, 100, 1000, -1);
	//	dataGridTest.TestSamples("MultivariateXORContinuous",
	//		2, 0, 150, 600, 100, 0, -1);
}

void KWDataGridTest::InitializeOptimizationParametersArray()
{
	boolean bOptimizationStudy = false;
	KWDataGridOptimizerParameters* optimizationParameters;
	int nLevel;

	require(oaOptimizationParameters.GetSize() == 0);

	// Ajout du parametrage par defaut
	// for (nLevel = 1; nLevel <= 3; nLevel++)
	for (nLevel = 1; nLevel <= 20; nLevel++)
	{
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetOptimizationLevel(nLevel);
		oaOptimizationParameters.Add(optimizationParameters);
	}

	// Ajout de nouveaux parametrages
	if (bOptimizationStudy)
	{
		// Ajout du parametrage par defaut
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetUnivariateInitialization(true);
		optimizationParameters->SetPreOptimize(true);
		optimizationParameters->SetOptimize(true);
		optimizationParameters->SetPostOptimize(true);
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);

		// Ajout du parametrage par defaut sans pre-opt ni post-opt
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetUnivariateInitialization(false);
		optimizationParameters->SetPreOptimize(false);
		optimizationParameters->SetOptimize(true);
		optimizationParameters->SetPostOptimize(false);
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);

		// Ajout du parametrage par defaut sans pre-opt
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetUnivariateInitialization(false);
		optimizationParameters->SetPreOptimize(false);
		optimizationParameters->SetOptimize(true);
		optimizationParameters->SetPostOptimize(true);
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);

		// Ajout du parametrage par defaut sans post-opt
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetUnivariateInitialization(false);
		optimizationParameters->SetPreOptimize(true);
		optimizationParameters->SetOptimize(true);
		optimizationParameters->SetPostOptimize(false);
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);

		// Ajout du parametrage par defaut sans rien
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetUnivariateInitialization(false);
		optimizationParameters->SetPreOptimize(false);
		optimizationParameters->SetOptimize(false);
		optimizationParameters->SetPostOptimize(false);
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);

		/*
		// Ajout du parametrage par defaut avec level a 5
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetOptimizationLevel(5);
		oaOptimizationParameters.Add(optimizationParameters);

		// Optimization par defaut avec level a 1
		optimizationParameters = new KWDataGridOptimizerParameters;
		optimizationParameters->SetOptimizationLevel(1);
		oaOptimizationParameters.Add(optimizationParameters);
		*/
	}
}

KWSampleGenerator* KWDataGridTest::LookupSampleGenerator(const ALString& sName)
{
	require(sName != "");
	if (sName == "RandomContinuous")
		return new KWSGRandomContinuous;
	else if (sName == "RandomSymbol")
		return new KWSGRandomSymbol;
	else if (sName == "ChessBoardContinuous")
		return new KWSGChessBoardContinuous;
	else if (sName == "ChessBoardSymbol")
		return new KWSGChessBoardSymbol;
	else if (sName == "MultivariateXORContinuous")
		return new KWSGMultivariateXORContinuous;
	else if (sName == "GaussianMixture")
		return new KWSGGaussianMixture;
	else
		return NULL;
}
