// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouperTest.h"

///////////////////////////////////////////////////////////////////////
// Classe KWSymbolSampleGenerator

KWSymbolSampleGenerator::KWSymbolSampleGenerator()
{
	nSourceModalityNumber = 2;
	nTargetClassNumber = 2;
	bIsGeneratorCompiled = false;
}

KWSymbolSampleGenerator::~KWSymbolSampleGenerator()
{
	oaTargetClassCumulatedConditionalProbs.DeleteAll();
}

void KWSymbolSampleGenerator::GenerateObjectValues(KWObject* kwoObject)
{
	double dRandom;
	int nSource;
	int nTarget;

	require(CheckObject(kwoObject));

	// Compilation si necessaire du generateur
	if (not bIsGeneratorCompiled)
		CompileGenerator();

	// Tirage aleatoire d'une modalite source
	dRandom = GetRandomDouble();
	nSource = GenerateSourceModalityIndex(dRandom);
	kwoObject->SetSymbolValueAt(liSourceAttributeLoadIndex, GetSourceModalityAt(nSource));

	// Tirage aleatoire d'une modalite cible pour cette modalite source
	dRandom = GetRandomDouble();
	nTarget = GenerateTargetClassIndex(nSource, dRandom);
	kwoObject->SetSymbolValueAt(liTargetAttributeLoadIndex, GetTargetClassAt(nTarget));
}

boolean KWSymbolSampleGenerator::CheckObject(KWObject* kwoObject)
{
	boolean bOk = true;

	if (kwoObject == NULL)
	{
		bOk = false;
		AddError("Entity is NULL");
	}
	else if (kwoObject->GetClass() != GetSymbolSampleClass())
	{
		bOk = false;
		kwoObject->AddError("Wrong dictionary");
	}
	else
	{
		bOk = true;
		assert(kwoObject->GetClass()->IsCompiled());
		assert(kwoObject->GetClass()->LookupAttribute("X")->GetLoadIndex() == liSourceAttributeLoadIndex);
		assert(kwoObject->GetClass()->LookupAttribute("Class")->GetLoadIndex() == liTargetAttributeLoadIndex);
	}

	return bOk;
}

KWClass* KWSymbolSampleGenerator::GetSymbolSampleClass()
{
	const ALString sClassName = "CategoricalSample";
	KWClass* kwcClass;
	KWAttribute* attribute;

	// Recherche de la classe de test
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);

	// Creation et enregistrement de la classe si non trouvee
	if (kwcClass == NULL)
	{
		// Initialisation de la classe
		kwcClass = new KWClass;
		kwcClass->SetName(sClassName);
		kwcClass->SetLabel("Test des methodes de groupage");

		// Initialisation des attributs
		attribute = new KWAttribute;
		attribute->SetName("X");
		attribute->SetType(KWType::Symbol);
		kwcClass->InsertAttribute(attribute);
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
	assert(kwcClass->LookupAttribute("X") != NULL);
	assert(kwcClass->LookupAttribute("Class") != NULL);

	// Acces aux index de chargement des attributs sources et cible
	liSourceAttributeLoadIndex = kwcClass->LookupAttribute("X")->GetLoadIndex();
	liTargetAttributeLoadIndex = kwcClass->LookupAttribute("Class")->GetLoadIndex();

	return kwcClass;
}

KWLoadIndex KWSymbolSampleGenerator::GetSourceAttributeLoadIndex() const
{
	assert(
	    liSourceAttributeLoadIndex ==
	    KWClassDomain::GetCurrentDomain()->LookupClass("CategoricalSample")->LookupAttribute("X")->GetLoadIndex());
	return liSourceAttributeLoadIndex;
}

KWLoadIndex KWSymbolSampleGenerator::GetTargetAttributeLoadIndex() const
{
	assert(liTargetAttributeLoadIndex == KWClassDomain::GetCurrentDomain()
						 ->LookupClass("CategoricalSample")
						 ->LookupAttribute("Class")
						 ->GetLoadIndex());
	return liTargetAttributeLoadIndex;
}

double KWSymbolSampleGenerator::ComputeGroupingLearningError(KWAttributeStats* kwasGrouping)
{
	KWDataGridStats* dataGridStats;
	const KWDGSAttributePartition* sourcePartition;
	const KWDGSAttributePartition* targetPartition;
	double dError;
	int nGroup;
	int i;
	int nHighestFrequency;
	int nCorrectInstanceNumber;

	require(IsGeneratorCompiled());
	require(kwasGrouping != NULL);
	require(kwasGrouping->GetAttributeType() == KWType::Symbol);
	require(kwasGrouping->GetPreparedDataGridStats() != NULL);

	// Acces a la grille de preparation du groupage
	dataGridStats = kwasGrouping->GetPreparedDataGridStats();
	assert(dataGridStats->GetAttributeNumber() == 2);
	sourcePartition = dataGridStats->GetAttributeAt(0);
	targetPartition = dataGridStats->GetAttributeAt(1);

	// Comptage des instances correctement classifiees par le predicteur majoritaire
	nCorrectInstanceNumber = 0;
	for (nGroup = 0; nGroup < sourcePartition->GetPartNumber(); nGroup++)
	{
		// Recherche de l'effectif de la classe majoritaire
		nHighestFrequency = -1;
		for (i = 0; i < targetPartition->GetPartNumber(); i++)
		{
			if (dataGridStats->GetBivariateCellFrequencyAt(nGroup, i) > nHighestFrequency)
				nHighestFrequency = dataGridStats->GetBivariateCellFrequencyAt(nGroup, i);
		}

		// Incrementation du nombre d'instances correctement classifiees en
		// apprentissage par le predicteur des classes majoritaires dans chaque groupe
		nCorrectInstanceNumber += nHighestFrequency;
	}

	// Calcul du taux d'erreur
	dError = 1 - (nCorrectInstanceNumber * 1.0 / dataGridStats->ComputeGridFrequency());
	return dError;
}

double KWSymbolSampleGenerator::ComputeGroupingTestError(KWAttributeStats* kwasGrouping)
{
	boolean bTrace = false;
	KWDataGridStats* preparedAttributeStats;
	const KWDGSAttributeGrouping* attributeGrouping;
	const KWDGSAttributePartition* targetAttributePartition;
	int nSource;
	int nGroup;
	int nTarget;
	Symbol sSourceModality;
	ALString sGroupLabel;
	Symbol sTargetClass;
	double dSourceModalityProb;
	double dTargetClassConditionalProb;
	int i;
	int iBest;
	int nHighestFrequency;
	double dTestError;

	require(IsGeneratorCompiled());
	require(kwasGrouping != NULL);
	require(kwasGrouping->GetAttributeType() == KWType::Symbol);
	require(kwasGrouping->GetPreparedDataGridStats() != NULL);

	// Acces au groupage de l'attribut
	preparedAttributeStats = kwasGrouping->GetPreparedDataGridStats();
	attributeGrouping = cast(const KWDGSAttributeGrouping*, preparedAttributeStats->GetAttributeAt(0));

	// Acces a l'attribut cible
	assert(preparedAttributeStats->GetAttributeNumber() == 2);
	targetAttributePartition = preparedAttributeStats->GetAttributeAt(1);

	// Affichage des caracteristiques de groupage
	if (bTrace)
	{
		cout << *kwasGrouping->GetPreparedDataGridStats() << endl;
	}

	// Parcours de toutes les modalites sources pour calculer l'erreur en test
	dTestError = 0;
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		// Renseignements sur la modalite source
		sSourceModality = GetSourceModalityAt(nSource);
		dSourceModalityProb = GetSourceModalityProbAt(nSource);

		// Recherche du groupe de rattachement de la modalite source
		nGroup = attributeGrouping->ComputeSymbolPartIndex(sSourceModality);
		sGroupLabel = "G_";
		sGroupLabel += attributeGrouping->GetValueAt(attributeGrouping->GetGroupFirstValueIndexAt(nGroup));

		// Recherche de l'index de la classe cible majoritaire pour ce groupe,
		// Dans la table de contingence
		iBest = -1;
		nHighestFrequency = -1;
		for (i = 0; i < targetAttributePartition->GetPartNumber(); i++)
		{
			if (preparedAttributeStats->GetBivariateCellFrequencyAt(nGroup, i) > nHighestFrequency)
			{
				nHighestFrequency = preparedAttributeStats->GetBivariateCellFrequencyAt(nGroup, i);
				iBest = i;
			}
		}
		assert(iBest >= 0);

		// Renseignements complet sur cette classe majoritaire
		sTargetClass = cast(KWDGSAttributeSymbolValues*, targetAttributePartition)->GetValueAt(iBest);
		assert(kwasGrouping->GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(sTargetClass) ==
		       iBest);
		nTarget = GetTargetClassIndex(sTargetClass);
		dTargetClassConditionalProb = GetTargetClassConditionalProbAt(nSource, nTarget);

		// Prise en compte de la modalite pour le calcul de l'erreur
		dTestError += dSourceModalityProb * (1 - dTargetClassConditionalProb);

		// Affichage des details de calcul
		if (bTrace)
			cout << sSourceModality << "\t" << dSourceModalityProb << "\t" << sGroupLabel << "\t"
			     << sTargetClass << "\t" << dTargetClassConditionalProb << "\t" << dTestError << endl;
	}

	return dTestError;
}

double KWSymbolSampleGenerator::ComputeGroupingDistance(KWAttributeStats* kwasGrouping)
{
	boolean bTrace = false;
	KWDataGridStats* preparedAttributeStats;
	const KWDGSAttributeGrouping* attributeGrouping;
	const KWDGSAttributePartition* targetAttributePartition;
	int nGroupFrequency;
	int nSource;
	int nGroup;
	int nTarget;
	Symbol sSourceModality;
	ALString sGroupLabel;
	Symbol sTargetClass;
	double dSourceModalityProb;
	double dTargetClassConditionalProb;
	int nTargetTableIndex;
	double dDistance;
	double dLaplaceEpsilon;
	double dLearnedConditionalProb;
	double dKullbackLeiblerTargetDistance;
	double dKullbackLeiblerDistance;

	require(IsGeneratorCompiled());
	require(kwasGrouping != NULL);
	require(kwasGrouping->GetAttributeType() == KWType::Symbol);
	require(kwasGrouping->GetPreparedDataGridStats() != NULL);

	// Acces au groupage de l'attribut
	preparedAttributeStats = kwasGrouping->GetPreparedDataGridStats();
	attributeGrouping = cast(const KWDGSAttributeGrouping*, preparedAttributeStats->GetAttributeAt(0));

	// Acces a l'attribut cible
	assert(preparedAttributeStats->GetAttributeNumber() == 2);
	targetAttributePartition = preparedAttributeStats->GetAttributeAt(1);

	// Affichage des caracteristiques de groupage
	if (bTrace)
	{
		cout << *kwasGrouping->GetPreparedDataGridStats() << endl;
	}

	// Parcours de toutes les modalites sources pour calculer l'erreur en test
	dDistance = 0;
	dLaplaceEpsilon = 1;
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		// Renseignements sur la modalite source
		sSourceModality = GetSourceModalityAt(nSource);
		dSourceModalityProb = GetSourceModalityProbAt(nSource);

		// Recherche du groupe de rattachement de la modalite source
		nGroup = attributeGrouping->ComputeSymbolPartIndex(sSourceModality);
		sGroupLabel = "G_";
		sGroupLabel += attributeGrouping->GetValueAt(attributeGrouping->GetGroupFirstValueIndexAt(nGroup));

		// Calcul de l'effectif du groupe
		nGroupFrequency = 0;
		for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
			nGroupFrequency += preparedAttributeStats->GetBivariateCellFrequencyAt(nGroup, nTarget);

		// Prise en compte des contribution a la distance de KullbackLeibler pour cette
		// modalite source
		dKullbackLeiblerDistance = 0;
		for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
		{
			// Renseignements complet sur la classe cible
			sTargetClass = GetTargetClassAt(nTarget);
			dTargetClassConditionalProb = GetTargetClassConditionalProbAt(nSource, nTarget);

			// Recherche de l'index de la classe cible dans la table de contingence
			nTargetTableIndex = targetAttributePartition->ComputeSymbolPartIndex(sTargetClass);

			// Calcul des probabilites cibles conditionnelles estimees en apprentissage
			// sur la table de contingence (estimateur de Laplace)
			if (nTargetTableIndex >= 0)
				dLearnedConditionalProb =
				    (preparedAttributeStats->GetBivariateCellFrequencyAt(nGroup, nTargetTableIndex) +
				     dLaplaceEpsilon) /
				    (nGroupFrequency + dLaplaceEpsilon * nTargetClassNumber);
			// Cas particulier si classe cible non trouvee en apprentissage
			else
				dLearnedConditionalProb = 1.0 / nTargetClassNumber;

			// Ajout de la contribution a la distance de Kullback-Leibler
			dKullbackLeiblerTargetDistance = 0;
			if (dTargetClassConditionalProb > 0)
				dKullbackLeiblerTargetDistance =
				    dTargetClassConditionalProb *
				    log(dTargetClassConditionalProb / dLearnedConditionalProb);
			dKullbackLeiblerDistance += dKullbackLeiblerTargetDistance;

			// Affichage des details de calcul
			if (bTrace)
				cout << nSource << "\t" << nTarget << "\t" << nTargetTableIndex << "\t"
				     << sSourceModality << "\t" << dSourceModalityProb << "\t" << sGroupLabel << "\t"
				     << sTargetClass << "\t" << dTargetClassConditionalProb << "\t"
				     << dLearnedConditionalProb << "\t" << dKullbackLeiblerTargetDistance << endl;
		}

		// Prise en compte de la modalite pour le calcul de l'erreur
		dDistance += dSourceModalityProb * dKullbackLeiblerDistance;
	}

	return dDistance;
}

void KWSymbolSampleGenerator::Write(ostream& ost) const
{
	int nSource;
	int nTarget;

	require(Check());

	// Compilation si necessaire du generateur
	if (not IsGeneratorCompiled())
		CompileGenerator();

	// Nom du generateur
	cout << GetName() << "\n";

	// Affichage de la ligne d'entete
	ost << "\tProb";
	for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
		ost << "\t" << GetTargetClassAt(nTarget);
	ost << "\n";

	// Affichage des details pour chaque modalite source
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		// Modalite source et sa probabilite
		cout << GetSourceModalityAt(nSource) << "\t" << GetSourceModalityProbAt(nSource);

		// Probabilites conditionnelles des classes cibles
		for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
			cout << "\t" << GetTargetClassConditionalProbAt(nSource, nTarget);
		cout << "\n";
	}
}

boolean KWSymbolSampleGenerator::Check() const
{
	boolean bOk = true;
	const double dEpsilon = 1e-5;
	int nSource;
	int nTarget;
	double dProb;
	double dCumulatedProb;
	ALString sTmp;

	require(nSourceModalityNumber >= 2);
	require(nTargetClassNumber >= 2);

	// Verification de la distribution de probabilites des modalites sources
	dCumulatedProb = 0;
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		// Acces a la probabilite de la modalite
		dProb = GetSourceModalityProbAt(nSource);

		// Verification de la probabilite
		if (dProb < 0 or dProb > 1)
		{
			bOk = false;
			AddError(sTmp + "La probabilite specifiee pour la modalite source " + IntToString(nSource) +
				 " est incorrecte (" + DoubleToString(dProb) + ")");
		}

		// Cumul de la probabilite
		dCumulatedProb += dProb;
	}

	// La probabilite cumulee doit etre egale a 1
	if (fabs(dCumulatedProb - 1) > dEpsilon)
	{
		bOk = false;
		AddError(sTmp + "La probabilite cumulee des modalites sources est incorrecte (" +
			 DoubleToString(dCumulatedProb) + ")");
	}

	// Verification de la distribution de probabilites des classes cibles
	// pour chaque modalite source
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		dCumulatedProb = 0;
		for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
		{
			// Acces a la probabilite de la modalite
			dProb = GetTargetClassConditionalProbAt(nSource, nTarget);

			// Verification de la probabilite
			if (dProb < 0 or dProb > 1)
			{
				bOk = false;
				AddError(sTmp + "La probabilite specifiee pour la classe cible " +
					 IntToString(nTarget) + " conditionnellement a la modalite source " +
					 IntToString(nSource) + " est incorrecte (" + DoubleToString(dProb) + ")");
			}

			// Cumul de la probabilite
			dCumulatedProb += dProb;
		}

		// La probabilite cumulee doit etre egale a 1
		if (fabs(dCumulatedProb - 1) > dEpsilon)
		{
			bOk = false;
			AddError(sTmp + "La probabilite cumulee des modalites cible " + "pour la modalite source " +
				 IntToString(nSource) + " est incorrecte (" + DoubleToString(dCumulatedProb) + ")");
		}
	}

	return bOk;
}

const ALString KWSymbolSampleGenerator::GetClassLabel() const
{
	return "Categorical Sample Generator";
}

const ALString KWSymbolSampleGenerator::GetObjectLabel() const
{
	return GetName() + "(" + IntToString(GetSourceModalityNumber()) + "," + IntToString(GetTargetClassNumber()) +
	       ")";
}

void KWSymbolSampleGenerator::CompileGenerator() const
{
	const ALString sModalityPrefix = "S";
	const ALString sClassPrefix = "Class";
	DoubleVector* dvTargetClassCumulatedConditionalProbs;
	int nSource;
	int nTarget;
	double dCumulatedProb;

	require(Check());

	// Creation des modalites sources
	sSourceModalities.SetSize(nSourceModalityNumber);
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
		sSourceModalities.SetAt(nSource, Symbol(sModalityPrefix + IntToString(nSource + 1)));

	// Creation des classes cibles
	sTargetClasses.SetSize(nTargetClassNumber);
	for (nTarget = 0; nTarget < nTargetClassNumber; nTarget++)
		sTargetClasses.SetAt(nTarget, Symbol(sClassPrefix + IntToString(nTarget + 1)));

	// Calcul des probabilites sources cumulees
	dCumulatedProb = 1;
	dvSourceModalityCumulatedProbs.SetSize(nSourceModalityNumber);
	for (nSource = nSourceModalityNumber - 1; nSource >= 0; nSource--)
	{
		dvSourceModalityCumulatedProbs.SetAt(nSource, dCumulatedProb);
		dCumulatedProb -= GetSourceModalityProbAt(nSource);
	}

	// Calcul des probabilites cibles cumulees pour chaque modalite source
	oaTargetClassCumulatedConditionalProbs.DeleteAll();
	oaTargetClassCumulatedConditionalProbs.SetSize(nSourceModalityNumber);
	for (nSource = 0; nSource < nSourceModalityNumber; nSource++)
	{
		// Creation d'un vecteur de probabilites conditionnelles cible cumule
		dvTargetClassCumulatedConditionalProbs = new DoubleVector;
		oaTargetClassCumulatedConditionalProbs.SetAt(nSource, dvTargetClassCumulatedConditionalProbs);

		// Calcul de probabilites cumulees
		dCumulatedProb = 1;
		dvTargetClassCumulatedConditionalProbs->SetSize(nTargetClassNumber);
		for (nTarget = nTargetClassNumber - 1; nTarget >= 0; nTarget--)
		{
			dvTargetClassCumulatedConditionalProbs->SetAt(nTarget, dCumulatedProb);
			dCumulatedProb -= GetTargetClassConditionalProbAt(nSource, nTarget);
		}
	}

	// Memorisation du flag de compilation
	bIsGeneratorCompiled = true;

	ensure(bIsGeneratorCompiled);
}

int KWSymbolSampleGenerator::GetSourceModalityIndex(const Symbol& sValue)
{
	int nSource;

	require(bIsGeneratorCompiled);

	// Recherche dans le tableau de modalites sources
	for (nSource = 0; nSource < sSourceModalities.GetSize(); nSource++)
	{
		if (sSourceModalities.GetAt(nSource) == sValue)
			return nSource;
	}
	return -1;
}

int KWSymbolSampleGenerator::GetTargetClassIndex(const Symbol& sValue)
{
	int nTarget;

	require(bIsGeneratorCompiled);

	// Recherche dans le tableau de modalites sources
	for (nTarget = 0; nTarget < sTargetClasses.GetSize(); nTarget++)
	{
		if (sTargetClasses.GetAt(nTarget) == sValue)
			return nTarget;
	}
	return -1;
}

int KWSymbolSampleGenerator::GenerateSourceModalityIndex(double dRandom)
{
	int nSource;

	require(bIsGeneratorCompiled);
	require(0 <= dRandom and dRandom <= 1);

	// Parcours du vecteur des probabilites cumulees des modalites sources
	// pour retrouver l'index recherche
	for (nSource = 0; nSource < dvSourceModalityCumulatedProbs.GetSize(); nSource++)
	{
		if (dRandom <= dvSourceModalityCumulatedProbs.GetAt(nSource))
			return nSource;
	}
	assert(false);
	return 0;
}

int KWSymbolSampleGenerator::GenerateTargetClassIndex(int nSourceModalityIndex, double dRandom)
{
	int nTarget;
	DoubleVector* dvTargetClassCumulatedConditionalProbs;

	require(bIsGeneratorCompiled);
	require(0 <= dRandom and dRandom <= 1);

	// Recherche du vecteur des probabilite conditionnelles cuimulees des classe cible
	// pour cette modalite source
	dvTargetClassCumulatedConditionalProbs =
	    cast(DoubleVector*, oaTargetClassCumulatedConditionalProbs.GetAt(nSourceModalityIndex));
	check(dvTargetClassCumulatedConditionalProbs);

	// Recherche de l'index de la classe cible a partir de ce vecteur
	for (nTarget = 0; nTarget < dvTargetClassCumulatedConditionalProbs->GetSize(); nTarget++)
	{
		if (dRandom <= dvTargetClassCumulatedConditionalProbs->GetAt(nTarget))
			return nTarget;
	}
	assert(false);
	return 0;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSSGRandom

KWSSGRandom::KWSSGRandom() {}

KWSSGRandom::~KWSSGRandom() {}

const ALString KWSSGRandom::GetName() const
{
	return "Random";
}

double KWSSGRandom::GetSourceModalityProbAt(int nSourceModalityIndex) const
{
	return 1.0 / nSourceModalityNumber;
}

double KWSSGRandom::GetTargetClassConditionalProbAt(int nSourceModalityIndex, int nTargetClassIndex) const
{
	return 1.0 / nTargetClassNumber;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSSGHiddenGroup

KWSSGHiddenGroup::KWSSGHiddenGroup()
{
	dSourceFrequencyRatio = 1;
	nNoiseRateNumber = 1;
}

KWSSGHiddenGroup::~KWSSGHiddenGroup() {}

void KWSSGHiddenGroup::SetSourceFrequencyRatio(double dValue)
{
	require(dValue >= 1);
	dSourceFrequencyRatio = dValue;
}

double KWSSGHiddenGroup::GetSourceFrequencyRatio() const
{
	return dSourceFrequencyRatio;
}

void KWSSGHiddenGroup::SetNoiseRateNumber(int nValue)
{
	require(nValue >= 1);

	nNoiseRateNumber = nValue;
	bIsGeneratorCompiled = false;
}

int KWSSGHiddenGroup::GetNoiseRateNumber() const
{
	return nNoiseRateNumber;
}

const ALString KWSSGHiddenGroup::GetName() const
{
	return "HiddenGroup";
}

const ALString KWSSGHiddenGroup::GetObjectLabel() const
{
	return GetName() + "(" + IntToString(GetSourceModalityNumber()) + "," + IntToString(GetTargetClassNumber()) +
	       "," + IntToString(GetNoiseRateNumber()) + ")";
}

double KWSSGHiddenGroup::GetSourceModalityProbAt(int nSourceModalityIndex) const
{
	double dBaseFrequency;
	double dGeometricCoef;

	// Cas ou toutes les modalites ont meme frequence
	if (dSourceFrequencyRatio <= 1 + 1e-5)
		return 1.0 / nSourceModalityNumber;
	// Cas ou les modalites suivent une loi geometrique
	else
	{
		// Calcul du coefficient de la loi geometrique a partir du ratio de
		// frequence entre les modalites extremes
		dGeometricCoef = pow(dSourceFrequencyRatio, 1.0 / (nSourceModalityNumber - 1));

		// Calcul de la frequence la plus faible
		dBaseFrequency = (dGeometricCoef - 1) / (dSourceFrequencyRatio * dGeometricCoef - 1);

		// On retourne la frequence de la modalite en fonction de son rang dans
		// la progression geometrique
		return dBaseFrequency * pow(dGeometricCoef, nSourceModalityIndex);
	}
}

double KWSSGHiddenGroup::GetTargetClassConditionalProbAt(int nSourceModalityIndex, int nTargetClassIndex) const
{
	int nNoiseRateIndex;
	double dNoiseRate;

	// Determination du niveau de bruit
	nNoiseRateIndex = (nSourceModalityIndex / nTargetClassNumber) % nNoiseRateNumber;
	dNoiseRate = (1 + nNoiseRateIndex) * 1.0 / (1 + nNoiseRateNumber);

	// Dans le cas correle (avec bruit en partie)
	if (nSourceModalityIndex % nTargetClassNumber == nTargetClassIndex)
		return (1 - dNoiseRate) + dNoiseRate * 1.0 / nTargetClassNumber;
	// Dans le cas bruite
	else
		return dNoiseRate * 1.0 / nTargetClassNumber;
}

///////////////////////////////////////////////////////////////////////
// Classe KWSymbolSampleGrouperTest

KWSymbolSampleGrouperTest::KWSymbolSampleGrouperTest()
{
	// Initialisation des attributs d'instances
	grouperSpec = NULL;
	sampleGenerator = NULL;
	nSampleSize = 0;
	nSampleNumber = 0;
	nFreshness = 0;
	nStatsFreshness = 0;
	nGrouperStatsFreshness = 0;
}

KWSymbolSampleGrouperTest::~KWSymbolSampleGrouperTest() {}

void KWSymbolSampleGrouperTest::SetGenerator(KWSymbolSampleGenerator* kwssgSampleGenerator)
{
	sampleGenerator = kwssgSampleGenerator;
}

KWSymbolSampleGenerator* KWSymbolSampleGrouperTest::GetGenerator()
{
	return sampleGenerator;
}

void KWSymbolSampleGrouperTest::SetGrouper(KWGrouperSpec* kwgsGrouperSpec)
{
	grouperSpec = kwgsGrouperSpec;
	nFreshness++;
}

KWGrouperSpec* KWSymbolSampleGrouperTest::GetGrouper()
{
	return grouperSpec;
}

void KWSymbolSampleGrouperTest::SetSampleSize(int nValue)
{
	require(nValue >= 0);

	nSampleSize = nValue;
	nFreshness++;
}

int KWSymbolSampleGrouperTest::GetSampleSize()
{
	return nSampleSize;
}

void KWSymbolSampleGrouperTest::SetSampleNumber(int nValue)
{
	require(nValue >= 0);

	nSampleNumber = nValue;
	nFreshness++;
}

int KWSymbolSampleGrouperTest::GetSampleNumber()
{
	return nSampleNumber;
}

boolean KWSymbolSampleGrouperTest::Check() const
{
	return sampleGenerator != NULL and grouperSpec != NULL and grouperSpec->Check();
}

boolean KWSymbolSampleGrouperTest::ComputeStats()
{
	boolean bExportSampleFiles = false;
	boolean bExportStatFiles = false;
	KWSTDatabaseTextFile sampleDatabase;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable tupleTable;
	KWLearningSpec sampleLearningSpec;
	KWAttributeStats symbolStats;
	KWClass* kwcSampleTestClass;
	int i;
	KWObject* kwoObject;
	int nRun;
	int nPartNumber;

	require(Check());

	// Preparation des vecteurs de resultats
	dvGroupingSizes.SetSize(nSampleNumber);
	dvLearningErrors.SetSize(nSampleNumber);
	dvTestErrors.SetSize(nSampleNumber);
	dvDistances.SetSize(nSampleNumber);

	// Remise a 0 de tous les resultats
	dvGroupingSizes.Initialize();
	dvLearningErrors.Initialize();
	dvTestErrors.Initialize();
	dvDistances.Initialize();

	// Acces a la classe de test
	kwcSampleTestClass = sampleGenerator->GetSymbolSampleClass();

	// Initialisation de la base et des specifications d'apprentissage
	sampleDatabase.SetClassName(kwcSampleTestClass->GetName());
	sampleDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + ".txt");
	sampleLearningSpec.SetClass(kwcSampleTestClass);
	sampleLearningSpec.SetDatabase(&sampleDatabase);
	sampleLearningSpec.SetTargetAttributeName("Class");
	sampleLearningSpec.GetPreprocessingSpec()->GetGrouperSpec()->CopyFrom(grouperSpec);
	symbolStats.SetLearningSpec(&sampleLearningSpec);
	symbolStats.SetAttributeName("X");
	symbolStats.SetAttributeType(KWType::Symbol);

	// Initialisation des objets de la base de donnees
	for (i = 0; i < nSampleSize; i++)
	{
		kwoObject = new KWObject(kwcSampleTestClass, i + 1);
		sampleDatabase.GetObjects()->Add(kwoObject);
	}

	// Repetition de l'experimentation
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		// On fixe la graine aleatoire
		SetRandomSeed(nRun);

		// Generation des valeurs des exemples
		sampleDatabase.SetDatabaseName(kwcSampleTestClass->GetName() + IntToString(nRun + 1) + ".txt");
		GenerateSample(&sampleDatabase);

		// Export de la base
		if (bExportSampleFiles)
			sampleDatabase.WriteAll(&sampleDatabase);
		tupleTableLoader.SetInputClass(kwcSampleTestClass);
		tupleTableLoader.SetInputDatabaseObjects(sampleDatabase.GetObjects());

		// Calcul des stats sur l'attribut cible
		tupleTableLoader.LoadUnivariate(sampleLearningSpec.GetTargetAttributeName(), &tupleTable);
		sampleLearningSpec.ComputeTargetStats(&tupleTable);

		// Grouping
		tupleTableLoader.LoadBivariate(symbolStats.GetAttributeName(),
					       sampleLearningSpec.GetTargetAttributeName(), &tupleTable);
		symbolStats.ComputeStats(&tupleTable);

		// Export du fichier de stats resultats
		if (bExportStatFiles)
			symbolStats.WriteReportFile(kwcSampleTestClass->GetName() + IntToString(nRun + 1) + ".xls");

		// Calcul du nombre de parties
		nPartNumber = 1;
		if (symbolStats.GetPreparedDataGridStats() != NULL and
		    symbolStats.GetPreparedDataGridStats()->GetSourceAttributeNumber() > 0)
			nPartNumber = symbolStats.GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();

		// Memorisation des resultats
		dvGroupingSizes.SetAt(nRun, nPartNumber);
		dvLearningErrors.SetAt(nRun, sampleGenerator->ComputeGroupingLearningError(&symbolStats));
		dvTestErrors.SetAt(nRun, sampleGenerator->ComputeGroupingTestError(&symbolStats));
		dvDistances.SetAt(nRun, sampleGenerator->ComputeGroupingDistance(&symbolStats));

		// Sans cette instruction forcant une entree-sortie, les simulations
		// tres longues s'interrompent mysterieusement
		if (nSampleNumber > 2000 and nRun % 1000 == 0)
			cout << " " << flush;
	}
	if (nSampleNumber > 2000)
		cout << endl;

	// Memorisation des informations de fraicheur
	nStatsFreshness = nFreshness;
	nGrouperStatsFreshness = grouperSpec->GetFreshness();
	ensure(IsStatsComputed());
	return IsStatsComputed();
}

boolean KWSymbolSampleGrouperTest::IsStatsComputed() const
{
	return nStatsFreshness == nFreshness and grouperSpec != NULL and
	       nGrouperStatsFreshness == grouperSpec->GetFreshness();
}

DoubleVector* KWSymbolSampleGrouperTest::GetGroupingSizes()
{
	require(IsStatsComputed());
	return &dvGroupingSizes;
}

DoubleVector* KWSymbolSampleGrouperTest::GetLearningErrors()
{
	require(IsStatsComputed());
	return &dvLearningErrors;
}

DoubleVector* KWSymbolSampleGrouperTest::GetTestErrors()
{
	require(IsStatsComputed());
	return &dvTestErrors;
}

DoubleVector* KWSymbolSampleGrouperTest::GetDistances()
{
	require(IsStatsComputed());
	return &dvDistances;
}

void KWSymbolSampleGrouperTest::WriteReportFile(const ALString& sFileName) const
{
	fstream ost;
	boolean bOk;

	require(IsStatsComputed());

	bOk = FileService::OpenOutputFile(sFileName, ost);
	if (bOk)
	{
		WriteReport(ost);
		ost.close();
	}
}

void KWSymbolSampleGrouperTest::WriteReport(ostream& ost) const
{
	int nRun;

	require(IsStatsComputed());

	// Specifications des tests
	ost << "Generator\t" << sampleGenerator->GetName() << "\n";
	ost << "Grouper\t" << grouperSpec->GetObjectLabel() << "\n";
	ost << "Sample size\t" << nSampleSize << "\n";
	ost << "Sample number\t" << nSampleNumber << "\n";
	ost << "\n";

	// Statistiques par echantillon
	ost << "Run\tGroup Number\tLearning Error\tTest Error\tDistance\n";
	for (nRun = 0; nRun < nSampleNumber; nRun++)
	{
		ost << nRun + 1 << "\t" << dvGroupingSizes.GetAt(nRun) << "\t" << dvLearningErrors.GetAt(nRun) << "\t"
		    << dvTestErrors.GetAt(nRun) << "\t" << dvDistances.GetAt(nRun) << "\n";
	}
	ost << endl;
}

void KWSymbolSampleGrouperTest::WriteHeaderLineReport(ostream& ost) const
{
	// Specifications des tests
	ost << "Generator\t";
	ost << "Grouper\t";
	ost << "Sample size\t";
	ost << "Sample number\t";

	// Moyenne des statistiques calculees
	ost << "Group number\t";
	ost << "Learn Error\t";
	ost << "Test error\t";
	ost << "Distance\t";

	// Ecart type des statistiques calculees
	ost << "GN Std. Dev.\t";
	ost << "LE Std. Dev.\t";
	ost << "TE Std. Dev.\t";
	ost << "D Std. Dev.";
}

void KWSymbolSampleGrouperTest::WriteLineReport(ostream& ost) const
{
	// Specifications des tests
	ost << sampleGenerator->GetObjectLabel() << "\t";
	ost << grouperSpec->GetObjectLabel() << "\t";
	ost << nSampleSize << "\t";
	ost << nSampleNumber << "\t";

	// Moyennes des statistiques calculees
	ost << KWStat::Mean(&dvGroupingSizes) << "\t";
	ost << KWStat::Mean(&dvLearningErrors) << "\t";
	ost << KWStat::Mean(&dvTestErrors) << "\t";
	ost << KWStat::Mean(&dvDistances) << "\t";

	// Ecart type des statistiques calculees
	ost << KWStat::StandardDeviation(&dvGroupingSizes) << "\t";
	ost << KWStat::StandardDeviation(&dvLearningErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvTestErrors) << "\t";
	ost << KWStat::StandardDeviation(&dvDistances);
}

void KWSymbolSampleGrouperTest::GenerateSample(KWDatabase* database)
{
	KWObject* kwoObject;
	int i;

	require(database != NULL);
	require(database->GetClassName() == sampleGenerator->GetSymbolSampleClass()->GetName());

	// Generation de l'echantillon
	for (i = 0; i < database->GetObjects()->GetSize(); i++)
	{
		// Recherche de l'objet a modifier
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(i));

		// Generation aleatoire des valeurs d'une instance
		sampleGenerator->GenerateObjectValues(kwoObject);
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWGrouperTest

KWGrouperTest::KWGrouperTest() {}

KWGrouperTest::~KWGrouperTest() {}

void KWGrouperTest::TestRandomSamples()
{
	const int nMaxSourceModalityNumber = 2050;
	const int nSampleNumber = 1000;
	KWSSGRandom sampleGenerator;
	KWSymbolSampleGrouperTest sampleGrouperTest;
	ObjectArray oaGrouperSpecs;
	KWGrouperSpec* grouperSpec;
	int nGrouper;
	IntVector ivSourceModalityNumbers;
	IntVector ivTargetClassNumbers;
	IntVector ivSampleSizes;
	int nParam1;
	int nParam2;
	int nParam3;
	int nSampleSize;
	int nSourceModalityNumber;
	int nTargetClassNumber;
	boolean bFirstLine;

	// Initialisation des groupers
	InitializeGrouperSpecArray(&oaGrouperSpecs);

	// Initialisation des tailles d'echantillon
	ivSampleSizes.Add(1000);

	// Initialisation des parametres de generation
	nSourceModalityNumber = 2;
	while (nSourceModalityNumber < nMaxSourceModalityNumber)
	{
		ivSourceModalityNumbers.Add(nSourceModalityNumber);
		nSourceModalityNumber *= 2;
	}
	ivSourceModalityNumbers.Add(1000);
	ivTargetClassNumbers.Add(2);
	// ivTargetClassNumbers.Add(4);

	// Parcours des tailles d'echantillon
	bFirstLine = true;
	for (nParam1 = 0; nParam1 < ivSampleSizes.GetSize(); nParam1++)
	{
		nSampleSize = ivSampleSizes.GetAt(nParam1);

		// Parcours des nombres de classes cibles
		for (nParam2 = 0; nParam2 < ivTargetClassNumbers.GetSize(); nParam2++)
		{
			nTargetClassNumber = ivTargetClassNumbers.GetAt(nParam2);

			// Parcours des nombres de modalites sources
			for (nParam3 = 0; nParam3 < ivSourceModalityNumbers.GetSize(); nParam3++)
			{
				nSourceModalityNumber = ivSourceModalityNumbers.GetAt(nParam3);

				// Experimentation si parametrage interessant
				if (nSourceModalityNumber <= nSampleSize)
				{
					// Parametrage du generateur
					sampleGenerator.SetSourceModalityNumber(nSourceModalityNumber);
					sampleGenerator.SetTargetClassNumber(nTargetClassNumber);

					// Parcours des groupers
					for (nGrouper = 0; nGrouper < oaGrouperSpecs.GetSize(); nGrouper++)
					{
						grouperSpec = cast(KWGrouperSpec*, oaGrouperSpecs.GetAt(nGrouper));

						// Evaluation de la groupage
						sampleGrouperTest.SetGenerator(&sampleGenerator);
						sampleGrouperTest.SetGrouper(grouperSpec);
						sampleGrouperTest.SetSampleSize(nSampleSize);
						sampleGrouperTest.SetSampleNumber(nSampleNumber);
						sampleGrouperTest.ComputeStats();

						// Impression d'un rapport sous forme de tableau
						if (bFirstLine)
						{
							sampleGrouperTest.WriteHeaderLineReport(cout);
							cout << endl;
							bFirstLine = false;
						}
						sampleGrouperTest.WriteLineReport(cout);
						cout << endl;
					}
				}
			}
		}
	}

	// Nettoyage
	oaGrouperSpecs.DeleteAll();
}

void KWGrouperTest::TestHiddenGroupSamples()
{
	const int nMaxSourceModalityNumber = 2050;
	const int nSampleNumber = 10; // 1000;
	KWSSGHiddenGroup sampleGenerator;
	KWSymbolSampleGrouperTest sampleGrouperTest;
	ObjectArray oaGrouperSpecs;
	KWGrouperSpec* grouperSpec;
	int nGrouper;
	IntVector ivSourceModalityNumbers;
	IntVector ivTargetClassNumbers;
	IntVector ivNoiseRateNumbers;
	IntVector ivSampleSizes;
	int nParam1;
	int nParam2;
	int nParam3;
	int nSampleSize;
	int nSourceModalityNumber;
	int nTargetClassNumber;
	int nNoiseRateNumber;
	boolean bFirstLine;

	// Initialisation des groupers
	InitializeGrouperSpecArray(&oaGrouperSpecs);

	// Initialisation des tailles d'echantillon
	ivSampleSizes.Add(100);
	ivSampleSizes.Add(1000);
	// ivSampleSizes.Add(10000);

	// Initialisation des parametres de generation
	// ivTargetClassNumbers.Add(2);
	ivTargetClassNumbers.Add(4);
	ivNoiseRateNumbers.Add(2);
	// ivNoiseRateNumbers.Add(3);

	// Parcours des tailles d'echantillon
	bFirstLine = true;
	for (nParam1 = 0; nParam1 < ivSampleSizes.GetSize(); nParam1++)
	{
		nSampleSize = ivSampleSizes.GetAt(nParam1);

		// Parcours des nombres de classes cibles
		for (nParam2 = 0; nParam2 < ivTargetClassNumbers.GetSize(); nParam2++)
		{
			nTargetClassNumber = ivTargetClassNumbers.GetAt(nParam2);

			// Parcours des nombres de taux de bruits
			for (nParam3 = 0; nParam3 < ivNoiseRateNumbers.GetSize(); nParam3++)
			{
				nNoiseRateNumber = ivNoiseRateNumbers.GetAt(nParam3);

				// Parcours des nombre de modalites sources
				nSourceModalityNumber = nNoiseRateNumber * nTargetClassNumber;
				while (nSourceModalityNumber <= nSampleSize and
				       nSourceModalityNumber <= nMaxSourceModalityNumber)
				{
					// Parametrage du generateur
					sampleGenerator.SetSourceModalityNumber(nSourceModalityNumber);
					sampleGenerator.SetTargetClassNumber(nTargetClassNumber);
					sampleGenerator.SetNoiseRateNumber(nNoiseRateNumber);

					// Parcours des groupers
					for (nGrouper = 0; nGrouper < oaGrouperSpecs.GetSize(); nGrouper++)
					{
						grouperSpec = cast(KWGrouperSpec*, oaGrouperSpecs.GetAt(nGrouper));

						// Evaluation de la groupage
						sampleGrouperTest.SetGenerator(&sampleGenerator);
						sampleGrouperTest.SetGrouper(grouperSpec);
						sampleGrouperTest.SetSampleSize(nSampleSize);
						sampleGrouperTest.SetSampleNumber(nSampleNumber);
						sampleGrouperTest.ComputeStats();

						// Impression d'un rapport sous forme de tableau
						if (bFirstLine)
						{
							sampleGrouperTest.WriteHeaderLineReport(cout);
							cout << endl;
							bFirstLine = false;
						}
						sampleGrouperTest.WriteLineReport(cout);
						cout << endl;
					}

					// Doublement du nombre de modalites sources
					nSourceModalityNumber *= 2;
				}
			}
		}
	}

	// Nettoyage
	oaGrouperSpecs.DeleteAll();
}

void KWGrouperTest::TestUnbalancedHiddenGroupSamples()
{
	double dSourceFrequencyRatio = 100;
	const int nSampleNumber = 1000;
	KWSSGHiddenGroup sampleGenerator;
	KWSymbolSampleGrouperTest sampleGrouperTest;
	ObjectArray oaGrouperSpecs;
	KWGrouperSpec* grouperSpec;
	int nGrouper;
	IntVector ivSourceModalityNumbers;
	IntVector ivTargetClassNumbers;
	IntVector ivNoiseRateNumbers;
	IntVector ivSampleSizes;
	int nParam1;
	int nParam2;
	int nParam3;
	int nParam4;
	int nSampleSize;
	int nSourceModalityNumber;
	int nTargetClassNumber;
	int nNoiseRateNumber;
	boolean bFirstLine;

	// Initialisation des groupers
	InitializeGrouperSpecArray(&oaGrouperSpecs);

	// Initialisation des tailles d'echantillon
	ivSampleSizes.Add(100);
	ivSampleSizes.Add(150);
	ivSampleSizes.Add(200);
	ivSampleSizes.Add(300);
	ivSampleSizes.Add(500);
	ivSampleSizes.Add(750);
	ivSampleSizes.Add(1000);
	ivSampleSizes.Add(1500);
	ivSampleSizes.Add(2000);
	ivSampleSizes.Add(3000);
	ivSampleSizes.Add(5000);
	ivSampleSizes.Add(7500);
	ivSampleSizes.Add(10000);

	// Initialisation des parametres de generation
	ivSourceModalityNumbers.Add(128);
	ivTargetClassNumbers.Add(4);
	ivNoiseRateNumbers.Add(2);

	// Parcours des tailles d'echantillon
	bFirstLine = true;
	for (nParam1 = 0; nParam1 < ivSampleSizes.GetSize(); nParam1++)
	{
		nSampleSize = ivSampleSizes.GetAt(nParam1);

		// Parcours des nombres de classes cibles
		for (nParam2 = 0; nParam2 < ivTargetClassNumbers.GetSize(); nParam2++)
		{
			nTargetClassNumber = ivTargetClassNumbers.GetAt(nParam2);

			// Parcours des nombres de taux de bruits
			for (nParam3 = 0; nParam3 < ivNoiseRateNumbers.GetSize(); nParam3++)
			{
				nNoiseRateNumber = ivNoiseRateNumbers.GetAt(nParam3);

				// Parcours des nombre de modalites sources
				for (nParam4 = 0; nParam4 < ivSourceModalityNumbers.GetSize(); nParam4++)
				{
					nSourceModalityNumber = ivSourceModalityNumbers.GetAt(nParam4);

					// Parametrage du generateur
					sampleGenerator.SetSourceModalityNumber(nSourceModalityNumber);
					sampleGenerator.SetTargetClassNumber(nTargetClassNumber);
					sampleGenerator.SetNoiseRateNumber(nNoiseRateNumber);
					sampleGenerator.SetSourceFrequencyRatio(dSourceFrequencyRatio);

					// Parcours des groupers
					for (nGrouper = 0; nGrouper < oaGrouperSpecs.GetSize(); nGrouper++)
					{
						grouperSpec = cast(KWGrouperSpec*, oaGrouperSpecs.GetAt(nGrouper));

						// Evaluation de la groupage
						sampleGrouperTest.SetGenerator(&sampleGenerator);
						sampleGrouperTest.SetGrouper(grouperSpec);
						sampleGrouperTest.SetSampleSize(nSampleSize);
						sampleGrouperTest.SetSampleNumber(nSampleNumber);
						sampleGrouperTest.ComputeStats();

						// Impression d'un rapport sous forme de tableau
						if (bFirstLine)
						{
							sampleGrouperTest.WriteHeaderLineReport(cout);
							cout << endl;
							bFirstLine = false;
						}
						sampleGrouperTest.WriteLineReport(cout);
						cout << endl;
					}

					// Doublement du nombre de modalites sources
					nSourceModalityNumber *= 2;
				}
			}
		}
	}

	// Nettoyage
	oaGrouperSpecs.DeleteAll();
}

void KWGrouperTest::TestCPUPerformance()
{
	const int nSourceModalityFrequency = 100;
	const int nTestMinNumber = 10;
	int nSampleNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dComputeTime;
	KWSSGHiddenGroup sampleGenerator;
	KWSymbolSampleGrouperTest sampleGrouperTest;
	ObjectArray oaGrouperSpecs;
	KWGrouperSpec* grouperSpec;
	int nGrouper;
	IntVector ivSourceModalityNumbers;
	IntVector ivTargetClassNumbers;
	IntVector ivNoiseRateNumbers;
	IntVector ivSampleSizes;
	int nParam1;
	int nParam2;
	int nParam3;
	int nSampleSize;
	int nSourceModalityNumber;
	int nTargetClassNumber;
	int nNoiseRateNumber;
	boolean bFirstLine;

	// Initialisation des groupers
	InitializeGrouperSpecArray(&oaGrouperSpecs);

	// Initialisation des tailles d'echantillon
	ivSampleSizes.Add(1000);
	ivSampleSizes.Add(1500);
	ivSampleSizes.Add(2000);
	ivSampleSizes.Add(3000);
	ivSampleSizes.Add(5000);
	ivSampleSizes.Add(7500);
	ivSampleSizes.Add(10000);
	ivSampleSizes.Add(15000);
	ivSampleSizes.Add(20000);
	ivSampleSizes.Add(30000);
	ivSampleSizes.Add(50000);
	ivSampleSizes.Add(75000);
	ivSampleSizes.Add(100000);

	// Initialisation des parametres de generation
	ivTargetClassNumbers.Add(4);
	ivNoiseRateNumbers.Add(2);

	// Parcours des tailles d'echantillon
	bFirstLine = true;
	// Parcours des groupers
	for (nGrouper = 0; nGrouper < oaGrouperSpecs.GetSize(); nGrouper++)
	{
		grouperSpec = cast(KWGrouperSpec*, oaGrouperSpecs.GetAt(nGrouper));

		for (nParam1 = 0; nParam1 < ivSampleSizes.GetSize(); nParam1++)
		{
			nSampleSize = ivSampleSizes.GetAt(nParam1);

			// Parcours des nombres de classes cibles
			for (nParam2 = 0; nParam2 < ivTargetClassNumbers.GetSize(); nParam2++)
			{
				nTargetClassNumber = ivTargetClassNumbers.GetAt(nParam2);

				// Parcours des nombres de taux de bruits
				for (nParam3 = 0; nParam3 < ivNoiseRateNumbers.GetSize(); nParam3++)
				{
					nNoiseRateNumber = ivNoiseRateNumbers.GetAt(nParam3);

					// Parcours des nombre de modalites sources
					nSourceModalityNumber = nSampleSize / nSourceModalityFrequency;

					// Parametrage du generateur
					sampleGenerator.SetSourceModalityNumber(nSourceModalityNumber);
					sampleGenerator.SetTargetClassNumber(nTargetClassNumber);
					sampleGenerator.SetNoiseRateNumber(nNoiseRateNumber);

					// Calcul d'un nombre d'essai fonction de la taille de l'echantillon
					nSampleNumber =
					    (nTestMinNumber * ivSampleSizes.GetAt(ivSampleSizes.GetSize() - 1)) /
					    nSampleSize;

					// Evaluation de la groupage
					sampleGrouperTest.SetGenerator(&sampleGenerator);
					sampleGrouperTest.SetGrouper(grouperSpec);
					sampleGrouperTest.SetSampleSize(nSampleSize);
					sampleGrouperTest.SetSampleNumber(nSampleNumber);
					tBegin = clock();
					sampleGrouperTest.ComputeStats();
					tEnd = clock();
					dComputeTime = (tEnd - tBegin) * 1.0 / CLOCKS_PER_SEC;
					dComputeTime /= sampleGrouperTest.GetSampleNumber();

					// Impression d'un rapport sous forme de tableau
					if (bFirstLine)
					{
						cout << "CPU\t";
						sampleGrouperTest.WriteHeaderLineReport(cout);
						cout << endl;
						bFirstLine = false;
					}
					cout << dComputeTime << "\t";
					sampleGrouperTest.WriteLineReport(cout);
					cout << endl;
				}
			}
		}
	}

	// Nettoyage
	oaGrouperSpecs.DeleteAll();
}

void KWGrouperTest::TestAll()
{
	TestRandomSamples();
	TestHiddenGroupSamples();
	// TestUnbalancedHiddenGroupSamples();
	// TestCPUPerformance();
}

void KWGrouperTest::InitializeGrouperSpecArray(ObjectArray* oaGrouperSpecs)
{
	KWGrouperSpec* grouperSpec;

	require(oaGrouperSpecs != NULL);
	require(oaGrouperSpecs->GetSize() == 0);

	// Ajout de la methode MODL
	grouperSpec = new KWGrouperSpec;
	grouperSpec->SetSupervisedMethodName("MODL");
	grouperSpec->SetParam(0);
	grouperSpec->SetMinGroupFrequency(1);
	oaGrouperSpecs->Add(grouperSpec);

	/*
	// Ajout de la methode MODLBasic
	grouperSpec = new KWGrouperSpec;
	grouperSpec->SetSupervisedMethodName("MODLBasic");
	grouperSpec->SetParam(0);
	grouperSpec->SetMinGroupFrequency(1);
	oaGrouperSpecs->Add(grouperSpec);

	// Ajout de la methode BasicGrouping
	grouperSpec = new KWGrouperSpec;
	grouperSpec->SetSupervisedMethodName("BasicGrouping");
	grouperSpec->SetParam(0);
	grouperSpec->SetMaxGroupNumber(1);
	oaGrouperSpecs->Add(grouperSpec);

	// Ajout de la methode MODL
	grouperSpec = new KWGrouperSpec;
	grouperSpec->SetSupervisedMethodName("MODL");
	grouperSpec->SetParam(0);
	grouperSpec->SetMinGroupFrequency(1);
	oaGrouperSpecs->Add(grouperSpec);
	*/
}
