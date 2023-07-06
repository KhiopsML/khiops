// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPreparationTask.h"

KWDataPreparationTask::KWDataPreparationTask()
{
	// Declaration des variables partagees
	DeclareSharedParameter(&shared_learningSpec);
	DeclareSharedParameter(&shared_svTargetValues);
	DeclareSharedParameter(&shared_cvTargetValues);
	DeclareSharedParameter(&shared_TargetTupleTable);

	// Initialisation des variables du maitre
	masterDataTableSliceSet = NULL;
	masterClass = NULL;
	masterDatabase = NULL;
}

KWDataPreparationTask::~KWDataPreparationTask() {}

int KWDataPreparationTask::ComputeMaxLoadableAttributeNumber(const KWLearningSpec* learningSpec,
							     const KWTupleTable* targetTupleTable,
							     int nAttributePairNumber) const
{
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	const int nMaxProcessBySlave = 2;
	RMTaskResourceRequirement resourceRequirement;
	RMTaskResourceGrant grantedResources;
	KWClass* kwcClass;
	longint lNecessaryWorkingMemory;
	longint lNecessaryMaxBlockWorkingMemory;
	longint lNecessaryUnivariateStatsMemory;
	longint lNecessaryBivariateStatsMemory;
	longint lNecessaryTargetAttributMemory;
	longint lClassAttributeMemory;
	longint lNecessaryDatabaseAllValuesMemory;
	longint lDatabaseAttributeAverageMemory;
	longint lDatabaseAttributeOverallAverageMemory;
	int nDatabaseObjectNumber;
	int nSlaveNumber;
	longint lSlaveMemory;
	int nMinSlaveProcessNumber;
	int nUsedAttributeNumber;
	int nMinAttributeNumber;
	longint lInitialMaxAttributeNumber;
	longint lMaxAttributeNumber;
	int nMaxAttributeNumber;
	ALString sMessage;

	require(CheckInputParameters(learningSpec, targetTupleTable));
	require(nAttributePairNumber >= 0);

	// Recherche de la classe associee a la base
	kwcClass = learningSpec->GetClass();
	check(kwcClass);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Calcul du nombre d'attribut a analyser, hors attribut cible
	nUsedAttributeNumber = kwcClass->GetUsedAttributeNumberForType(KWType::Continuous) +
			       kwcClass->GetUsedAttributeNumberForType(KWType::Symbol);
	if (learningSpec->GetTargetAttributeName() != "")
		nUsedAttributeNumber--;

	// Le nombre de paires d'attribut ne doit pas depasser le max possible
	assert(nAttributePairNumber <= (longint)nUsedAttributeNumber * nUsedAttributeNumber / 2);

	//////////////////////////////////////////////////////////////////////////
	// Estimation des ressources necessaires pour la preparation des donnees
	// On base se baser sur un calcul heuristique dans le cas des attributs sparse
	// C'est lors du dimensionnement effectif que l'on tiendra compte des attributs sparse

	// Calcul de la memoire de travail et de la memoire de stockage par resultat
	lNecessaryWorkingMemory =
	    ComputeNecessaryWorkingMemory(learningSpec, targetTupleTable, (nAttributePairNumber > 0));
	lNecessaryTargetAttributMemory = ComputeNecessaryTargetAttributeMemory(learningSpec, targetTupleTable);
	lNecessaryUnivariateStatsMemory = ComputeNecessaryUnivariateStatsMemory(learningSpec, targetTupleTable);
	lNecessaryBivariateStatsMemory = ComputeNecessaryBivariateStatsMemory(learningSpec, targetTupleTable);

	// Memoire pour le stockage d'un attribut de dictionnaire
	lClassAttributeMemory = ComputeNecessaryClassAttributeMemory();

	// Estimation la place memoire totale necessaire pour le stockage de toutes les valeurs de tous les attributs
	lNecessaryDatabaseAllValuesMemory =
	    ComputeEstimatedDatabaseAllValuesMemory(learningSpec, targetTupleTable, kwcClass, kwcClass);

	// Memoire moyenne necessaire par attribut: estimation heuristique pour un premier dimensionnement de base
	lDatabaseAttributeAverageMemory = lClassAttributeMemory + lNecessaryUnivariateStatsMemory;
	lDatabaseAttributeAverageMemory += lNecessaryDatabaseAllValuesMemory / max(1, nUsedAttributeNumber);

	// Pour minimiser le nombre de tranches, on impose un nombre minimum de variables par tranches
	// Pour les petits nombres de variables utilise, on peut faire des tranches petites, mais pour les
	// grands nombre de variables, il faut minimiser le nombre de tranches
	// On utilise la fonction suivante, qui augmente plus lentement que sqrt(n)
	//  sqrt(n /(1+ln(n))
	// 	n	Att
	// 	1	1
	// 	10	2
	// 	100	4
	// 	1000	11
	// 	10000	31
	// 	100000	89
	nMinAttributeNumber = (int)ceil(sqrt(nUsedAttributeNumber / (1 + log(nUsedAttributeNumber + 1))));

	// Pour le maitre: stockage des dictionnaires de specification des tranches de la KWDataTableSliceSet
	// et de tous les resultats d'analyse
	resourceRequirement.GetMasterRequirement()->GetMemory()->SetMin(
	    lClassAttributeMemory * nUsedAttributeNumber + lNecessaryUnivariateStatsMemory * nUsedAttributeNumber +
	    lNecessaryBivariateStatsMemory * nAttributePairNumber);
	resourceRequirement.GetMasterRequirement()->GetMemory()->SetMax(
	    2 * resourceRequirement.GetMasterRequirement()->GetMemory()->GetMin());

	// Evaluation de la memoire necessaire de travail pour tenir compte des eventuels blocs a charger
	lNecessaryMaxBlockWorkingMemory = 0;
	if (kwcClass->GetLoadedAttributeBlockNumber() >= 1)
		lNecessaryMaxBlockWorkingMemory = ComputeEstimatedMaxBlockWorkingMemory(
		    learningSpec, targetTupleTable, kwcClass, kwcClass, nMinAttributeNumber);

	// Pour l'esclave: memoire de travail plus stockage des resultats des analyses
	// pour un nombre minimum d'attributs
	resourceRequirement.GetSlaveRequirement()->GetMemory()->SetMin(
	    lNecessaryWorkingMemory + lNecessaryMaxBlockWorkingMemory +
	    lDatabaseAttributeAverageMemory * nMinAttributeNumber);
	resourceRequirement.GetSlaveRequirement()->GetMemory()->SetMax(
	    lNecessaryWorkingMemory + lNecessaryMaxBlockWorkingMemory +
	    lDatabaseAttributeAverageMemory * nUsedAttributeNumber);

	// Memoire partagee, en prenant en compte la classes et les stats a transferer pour le minimum d'attributs
	resourceRequirement.GetSharedRequirement()->GetMemory()->Set(
	    lNecessaryTargetAttributMemory + lClassAttributeMemory * nMinAttributeNumber +
	    lNecessaryUnivariateStatsMemory * nMinAttributeNumber);

	// Nombre max de processus
	resourceRequirement.SetMaxSlaveProcessNumber(1 + nUsedAttributeNumber / max(1, nMinAttributeNumber));

	///////////////////////////////////////////////////////////////////////////////////////////
	// Calcul du nombre max d'attributs par esclave

	// Estimation de la memoire globale necessaire par attribut, en tenant compte de l'eventuelle memoire de travail
	// par bloc
	lDatabaseAttributeOverallAverageMemory = lDatabaseAttributeAverageMemory;
	lDatabaseAttributeOverallAverageMemory += lNecessaryMaxBlockWorkingMemory / max(1, nMinAttributeNumber);

	// Calcul des ressources allouees, pour pouvoir estimer le nombre d'esclaves
	lInitialMaxAttributeNumber = 0;
	nSlaveNumber = 0;
	lSlaveMemory = 0;
	RMParallelResourceManager::ComputeGrantedResources(&resourceRequirement, &grantedResources);
	if (not grantedResources.IsEmpty())
	{
		// Recherche du nombre d'esclave possible
		nSlaveNumber = grantedResources.GetSlaveNumber();

		// Calcul de la memoire disponible pour les attributs supplementaires
		lSlaveMemory = grantedResources.GetSlaveMemory() -
			       resourceRequirement.GetSlaveRequirement()->GetMemory()->GetMin();

		// Calcul du nombre max d'attributs gerable par esclave
		lInitialMaxAttributeNumber =
		    nMinAttributeNumber + lSlaveMemory / lDatabaseAttributeOverallAverageMemory;
		assert(lInitialMaxAttributeNumber >= 1 or nMinAttributeNumber == 0);
	}

	// Ajustement du nombre max d'attribut par esclave
	nMinSlaveProcessNumber = 0;
	nMaxAttributeNumber = 0;
	lMaxAttributeNumber = lInitialMaxAttributeNumber;
	if (lInitialMaxAttributeNumber > 0)
	{
		// On diminue le nombre d'attributs max dans le cas des paires d'attributs,
		// pour lequel il faudra potentiellement charger deux paquet d'attributs simultanement
		if (nAttributePairNumber > 0)
			lMaxAttributeNumber = (1 + lMaxAttributeNumber) / 2;
		if (lMaxAttributeNumber >= nUsedAttributeNumber)
			lMaxAttributeNumber = max(1, nUsedAttributeNumber);

		// On diminue potentiellement le nombre d'attributs par esclave pour pouvoir utiliser au mieux les
		// esclaves en les faisant travailler plusieurs fois chacun, de facon a minimer l'attente du dernier
		// esclave
		if (lMaxAttributeNumber * nMaxProcessBySlave * nSlaveNumber > nUsedAttributeNumber)
			lMaxAttributeNumber = 1 + nUsedAttributeNumber / (nSlaveNumber * nMaxProcessBySlave);

		// On ajuste le nombre d'attribut pour qu'il soit si possible equilibre par process esclave
		nMaxAttributeNumber = (int)lMaxAttributeNumber;
		nMinSlaveProcessNumber = max(1, (int)ceil(nUsedAttributeNumber / (nMaxAttributeNumber + 1.0)));
		nMaxAttributeNumber = 1 + (nUsedAttributeNumber / nMinSlaveProcessNumber);
	}

	// Affichage des stats
	if (bDisplayMemoryStats)
	{
		cout << "KWDataPreparationTask::ComputeMaxLoadableAttributeNumber" << endl;
		cout << "\tObject number\t" << nDatabaseObjectNumber << endl;
		cout << "\tAttribute number\t" << nUsedAttributeNumber << endl;
		if (targetTupleTable->GetAttributeNumber() > 0)
			cout << "\tTarget attribute type\t" << KWType::ToString(targetTupleTable->GetAttributeTypeAt(0))
			     << endl;
		cout << "\tTarget value number\t" << targetTupleTable->GetSize() << endl;
		cout << "\tNecessary target attribute memory\t"
		     << LongintToHumanReadableString(lNecessaryTargetAttributMemory) << endl;
		cout << "\tNecessary working memory\t" << LongintToHumanReadableString(lNecessaryWorkingMemory) << endl;
		cout << "\tNecessary database all values memory\t"
		     << LongintToHumanReadableString(lNecessaryDatabaseAllValuesMemory) << endl;
		cout << "\tNecessary max block working memory\t"
		     << LongintToHumanReadableString(lNecessaryMaxBlockWorkingMemory) << endl;
		cout << "\tNecessary univariate stats memory\t"
		     << LongintToHumanReadableString(lNecessaryUnivariateStatsMemory) << endl;
		cout << "\tNecessary bivariate stats memory\t"
		     << LongintToHumanReadableString(lNecessaryBivariateStatsMemory) << endl;
		cout << "\tDictionary attribute memory\t" << LongintToHumanReadableString(lClassAttributeMemory)
		     << endl;
		cout << "\tDatabase dense attribute memory\t"
		     << LongintToHumanReadableString(nDatabaseObjectNumber * sizeof(KWValue)) << endl;
		cout << "\tDatabase attribute average memory\t"
		     << LongintToHumanReadableString(lDatabaseAttributeAverageMemory) << endl;
		cout << "\tDatabase attribute overall average memory\t"
		     << LongintToHumanReadableString(lDatabaseAttributeOverallAverageMemory) << endl;
		cout << "\tSlaveNumber\t" << nSlaveNumber << endl;
		cout << "\tSlaveMemory\t" << lSlaveMemory << endl;
		cout << "\tMinSlaveProcessNumber\t" << nMinSlaveProcessNumber << endl;
		cout << "\tMin attribute number\t" << nMinAttributeNumber << endl;
		cout << "\tMax attribute number (initial)\t" << lInitialMaxAttributeNumber << endl;
		cout << "\tMax attribute number\t" << nMaxAttributeNumber << endl;
		cout << "Requirement\n" << resourceRequirement << endl;
		cout << "Grant\n" << grantedResources << endl;
		cout << endl;
	}

	// Message d'erreur si pas assez de memoire
	if (nMaxAttributeNumber == 0 and nUsedAttributeNumber > 0)
	{
		sMessage = "Not enough memory for data preparation with ";
		sMessage += LongintToReadableString(nDatabaseObjectNumber);
		sMessage += " records (load data base, univariate ";
		if (nAttributePairNumber > 0)
			sMessage += "and bivariate ";
		sMessage += "data processing)";
		AddError(sMessage + ",\n\t" + grantedResources.GetMissingResourceMessage());
	}
	return nMaxAttributeNumber;
}

boolean KWDataPreparationTask::CheckInputParameters(const KWLearningSpec* learningSpec,
						    const KWTupleTable* targetTupleTable) const
{
	boolean bOk = true;

	require(learningSpec != NULL);
	require(targetTupleTable != NULL);

	bOk = bOk and learningSpec->Check();
	bOk = bOk and targetTupleTable->GetAttributeNumber() <= 1;
	bOk = bOk and (targetTupleTable->GetAttributeNumber() == 0 or learningSpec->GetTargetAttributeName() != "");
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       targetTupleTable->GetAttributeNameAt(0) == learningSpec->GetTargetAttributeName());
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       targetTupleTable->GetAttributeTypeAt(0) == learningSpec->GetTargetAttributeType());
	return bOk;
}

void KWDataPreparationTask::SortAttributesByBlock(ObjectArray* oaAttributes) const
{
	require(oaAttributes != NULL);
	oaAttributes->SetCompareFunction(KWAttributeCompareBlockName);
	oaAttributes->Sort();
	ensure(AreAttributesSortedByBlock(oaAttributes));
}

boolean KWDataPreparationTask::AreAttributesSortedByBlock(const ObjectArray* oaAttributes) const
{
	boolean bOk = true;
	int i;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* currentAttributeBlock;
	ObjectDictionary odAttributeBlocks;

	require(oaAttributes != NULL);

	// Parcours des attributs pour verifier l'utilisation des bloc de facon contigues
	currentAttributeBlock = NULL;
	for (i = 0; i < oaAttributes->GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaAttributes->GetAt(i));
		attributeBlock = attribute->GetAttributeBlock();

		// Test si changement de block
		if (attributeBlock != currentAttributeBlock)
		{
			// Si on est dans un bloc
			if (attributeBlock != NULL)
			{
				// Il ne doit jamais avoir ete vu
				if (odAttributeBlocks.Lookup(attributeBlock->GetName()) != NULL)
				{
					AddError("Variable " + attribute->GetName() + " is not contiguous in block " +
						 attributeBlock->GetName());
					bOk = false;
					break;
				}
				// Si ok, on memorise le block
				else
					odAttributeBlocks.SetAt(attributeBlock->GetName(), attributeBlock);
			}

			// Memorisation du nouveau block courant
			currentAttributeBlock = attributeBlock;
		}
	}
	return bOk;
}

longint KWDataPreparationTask::ComputeNecessaryUnivariateStatsMemory(const KWLearningSpec* learningSpec,
								     const KWTupleTable* targetTupleTable) const
{
	longint lAttributeStatSize;
	int nTargetModalityNumber;
	int nMeanPartNumber;
	int nMeanValueNumber;
	longint lSymbolSize;
	longint lClassAttributeMemory;

	require(CheckInputParameters(learningSpec, targetTupleTable));

	// Taille occupe par un Symbol (vide)
	lSymbolSize = Symbol::GetUsedMemoryPerSymbol();

	// Nombre moyen estime de valeurs et de parties
	nMeanPartNumber = 1 + int(log(1 + targetTupleTable->GetTotalFrequency()) / log(2.0));
	nMeanValueNumber = nMeanPartNumber;

	// Nombre de valeurs cibles
	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and not learningSpec->IsTargetGrouped())
	{
		// Prise en compte du nombre de valeurs cibles
		nTargetModalityNumber = targetTupleTable->GetSize();
	}
	// Dans le cas general, on se ramene a un nombre de partie (discretisation, groupement des valeurs cible, non
	// supervise)
	else
		nTargetModalityNumber = nMeanPartNumber;

	// Prise en compte de la place occupee par un attribut dans un dictionnaire
	lClassAttributeMemory = ComputeNecessaryClassAttributeMemory();
	lAttributeStatSize = lClassAttributeMemory;

	// Taille occupee par un attribut
	// L'estimation est tres approximative, et est consideree comme raisonnable que ce soit dans le cas supervise
	// ou non supervise, meme avec des histogrammes
	lAttributeStatSize +=
	    nMeanValueNumber * lSymbolSize + sizeof(KWAttributeStats) + sizeof(KWDescriptiveContinuousStats) +
	    sizeof(KWDataGridStats) + 2 * (sizeof(KWDGSAttributeGrouping) + KWClass::GetNameMaxLength()) +
	    (nMeanPartNumber + nMeanValueNumber + nTargetModalityNumber) * (sizeof(KWValue) + sizeof(int)) +
	    nMeanPartNumber * nTargetModalityNumber * sizeof(int);
	return lAttributeStatSize;
}

longint KWDataPreparationTask::ComputeNecessaryBivariateStatsMemory(const KWLearningSpec* learningSpec,
								    const KWTupleTable* targetTupleTable) const
{
	int nTargetModalityNumber;
	const longint lAttributeBaseStatSize = lKB;
	int nMeanPartNumber;
	int nMeanValueNumber;
	longint lSymbolSize;
	longint lAttributePairStatSize;

	require(CheckInputParameters(learningSpec, targetTupleTable));

	// Taille occupe par un Symbol (vide)
	lSymbolSize = Symbol::GetUsedMemoryPerSymbol();

	// Nombre moyen estime de valeurs et de parties
	nMeanPartNumber = 1 + int(log(1 + targetTupleTable->GetTotalFrequency()) / log(2.0));
	nMeanValueNumber = nMeanPartNumber;

	// Nombre de valeurs cibles
	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and not learningSpec->IsTargetGrouped())
	{
		// Prise en compte du nombre de valeurs cibles
		nTargetModalityNumber = targetTupleTable->GetSize();
	}
	// Dans le cas general, on se ramene a un nombre de parties (discretisation, groupement des valeurs cibles, non
	// supervise)
	else
		nTargetModalityNumber = nMeanPartNumber;

	// Taille occupee par une paire d'attribut
	lAttributePairStatSize =
	    lAttributeBaseStatSize + nMeanValueNumber * lSymbolSize + sizeof(KWDataGridStats) +
	    3 * (sizeof(KWDGSAttributeGrouping) + KWClass::GetNameMaxLength()) +
	    (2 * nMeanPartNumber + 2 * nMeanValueNumber + nTargetModalityNumber) * (sizeof(KWValue) + sizeof(int)) +
	    nMeanPartNumber * nMeanPartNumber * nTargetModalityNumber * sizeof(int);
	return lAttributePairStatSize;
}

longint KWDataPreparationTask::ComputeNecessaryTargetAttributeMemory(const KWLearningSpec* learningSpec,
								     const KWTupleTable* targetTupleTable) const
{
	longint lTargetAttributeMemory;
	const KWTuple* tuple;
	int nValue;

	require(CheckInputParameters(learningSpec, targetTupleTable));
	require(targetTupleTable->GetAttributeNumber() <= 1);

	// On se base sur les informations portees par l'attribut supplementaire du chargeur de table de tuples
	lTargetAttributeMemory = targetTupleTable->GetUsedMemory();
	if (targetTupleTable->GetAttributeNumber() != 0)
	{
		// Prise en compte des Symbol eux-memes dans le cas categoriel
		if (targetTupleTable->GetAttributeTypeAt(0) == KWType::Symbol)
		{
			assert(targetTupleTable->GetAttributeNumber() == 1);
			for (nValue = 0; nValue < targetTupleTable->GetSize(); nValue++)
			{
				tuple = targetTupleTable->GetAt(nValue);
				lTargetAttributeMemory += tuple->GetSymbolAt(0).GetUsedMemory();
			}
		}
	}

	// Prise en compte egalement des vecteurs de valeurs, dans le cas suppervise
	if (targetTupleTable->GetAttributeNumber() != 0)
		lTargetAttributeMemory += targetTupleTable->GetTotalFrequency() * sizeof(KWValue);
	return lTargetAttributeMemory;
}

longint KWDataPreparationTask::ComputeNecessaryWorkingMemory(const KWLearningSpec* learningSpec,
							     const KWTupleTable* targetTupleTable,
							     boolean bComputeAttributePairs) const
{
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	KWClass* kwcClass;
	KWClass dummyClass;
	KWDatabase dummyDatabase;
	int nDatabaseObjectNumber;
	boolean bDataGridRequired;
	longint lWorkingMemorySize;
	longint lClassAttributeMemorySize;
	int nTargetModalityNumber;
	longint lEmptyObjectSize;
	longint lDatabaseSize;
	longint lFrequencyTableSize;
	int nTupleAttributeNumber;
	longint lTupleTableSize;
	longint lMODLLineSize;
	int nSourceValueNumber;
	int nTargetValueNumber;
	longint lInitialDatagridSize;
	longint lInitialUnivariateDatagridSize;
	longint lWorkingDatagridSize;
	longint lDataGridOptimizationSize;
	longint lDataGridPostOptimizationSize;

	require(CheckInputParameters(learningSpec, targetTupleTable));

	// Initialisations
	lWorkingMemorySize = 0;
	lClassAttributeMemorySize = 0;
	nTargetModalityNumber = 0;
	lFrequencyTableSize = 0;
	lMODLLineSize = 0;
	nSourceValueNumber = 0;
	nTargetValueNumber = 0;
	lInitialDatagridSize = 0;
	lInitialUnivariateDatagridSize = 0;
	lWorkingDatagridSize = 0;
	lDataGridOptimizationSize = 0;
	lDataGridPostOptimizationSize = 0;

	///////////////////////////////////////////////////////////////
	// Calcul de la taille memoire  de travail minimum necessaire
	//    taille des objets charges en memoire a vide
	//    taille de la memoire de travail necessaire pour traiter les attributs

	// Utilisation a minima d'un buffer de base de donnee pour la lecture
	lWorkingMemorySize += BufferedFile::nDefaultBufferSize;

	// Nombre d'objets
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Recherche de la classe associee a la base
	kwcClass = learningSpec->GetClass();
	check(kwcClass);

	// Calcul de la taille a vide d'un objet (avec au minimum un champs, en prenant en compte le cas sparse)
	lEmptyObjectSize = sizeof(KWObject) + sizeof(KWObject*) + sizeof(KWValue*) + GetNecessaryMemoryPerDenseValue();

	// Prise en compte d'un dictionnaire et d'une base minimale
	lClassAttributeMemorySize = ComputeNecessaryClassAttributeMemory();
	lWorkingMemorySize += dummyClass.GetUsedMemory();
	lWorkingMemorySize += lClassAttributeMemorySize;
	lWorkingMemorySize += dummyDatabase.GetUsedMemory();
	lWorkingMemorySize += 2 * KWClass::GetNameMaxLength();

	// Taille de la base chargee en memoire avec deux champs dont un categoriel
	// On a ainsi une petite marge pour un dimensionnement minimal
	lDatabaseSize = nDatabaseObjectNumber * lEmptyObjectSize;
	lWorkingMemorySize += lDatabaseSize;

	// On prend en compte la taille necessaire au stockage d'une table de tuples
	// On ignore la memoire necessaire a la construction de la table de tuple
	// (KWTupleTable::ComputeNecessaryBuildingMemory(nDatabaseObjectNumber)), cette
	// memoire etant temporaire, et inferieure a ce qui sera necessaire par la suite
	// pour le pretraitement univarie ou bivarie des variables
	nTupleAttributeNumber = 1;
	if (learningSpec->GetTargetAttributeType() != KWType::None)
		nTupleAttributeNumber++;
	if (bComputeAttributePairs)
	{
		if (learningSpec->GetTargetAttributeType() == KWType::Symbol and not learningSpec->IsTargetGrouped())
			nTupleAttributeNumber++;
		else if (learningSpec->GetTargetAttributeType() == KWType::None)
			nTupleAttributeNumber++;
	}
	lTupleTableSize = KWTupleTable::ComputeNecessaryMemory(nDatabaseObjectNumber, nTupleAttributeNumber);
	lWorkingMemorySize += lTupleTableSize;

	// On determine si une grille de travail est necessaire
	bDataGridRequired = false;
	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and learningSpec->IsTargetGrouped())
		bDataGridRequired = true;
	if (learningSpec->GetTargetAttributeType() == KWType::Continuous)
		bDataGridRequired = true;
	if (bComputeAttributePairs)
		bDataGridRequired = true;

	// Memoire de travail dans le cas supervise non groupe
	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and not learningSpec->IsTargetGrouped())
	{
		// Prise en compte du nombre de valeurs cibles
		nTargetModalityNumber = targetTupleTable->GetSize();

		// Donnees de travail pour pretraitement supervise
		// Taille de la table d'effectifs
		lFrequencyTableSize =
		    sizeof(KWFrequencyTable) +
		    nDatabaseObjectNumber *
			(sizeof(KWFrequencyVector) + nTargetModalityNumber * sizeof(int) + sizeof(KWFrequencyVector*));
		// Taille d'un vecteur dans la liste d'intervalles
		// On ignore les structures pour la post-optimisation, qui sera basee sur un nombre d'intervalles
		// beaucoup plus petit que le nombre d'objets de la base Dans la cas categoriel, avec ou sans poubelle,
		// le groupe fourre-tout contient a minima toutes les valeurs singletons: il y aura au plus deux fois
		// moins de groupes a consideres que d'instances dans la base On ne prevoie pas de memoire
		// supplementaire pour ce cas, qui consommera moins de memoire que dans le pire des cas numerique
		lMODLLineSize = sizeof(KWMODLLine) + sizeof(KWFrequencyVector) + nTargetModalityNumber * sizeof(int);
		lWorkingMemorySize += lFrequencyTableSize + nDatabaseObjectNumber * lMODLLineSize;
	}

	// Memoire de travail avec grille bivariee
	if (bDataGridRequired)
	{
		// Initialisation des nombres de valeurs des dimension potentielles de la grille
		// Au pire, il y a autant de valeur que d'instances
		// Dans le cas supervise univarie, on peut se limiter au nombvre de valeurs cible effectives
		nSourceValueNumber = nDatabaseObjectNumber;
		nTargetValueNumber = nDatabaseObjectNumber;
		if (not bComputeAttributePairs)
		{
			assert(learningSpec->GetTargetAttributeType() == KWType::Continuous or
			       (learningSpec->GetTargetAttributeType() == KWType::Symbol and
				learningSpec->IsTargetGrouped()));
			assert(learningSpec->GetTargetDescriptiveStats() != NULL);
			nTargetValueNumber = learningSpec->GetTargetDescriptiveStats()->GetValueNumber();
		}

		// Un grille bivariee maximale complete initiale (cf. KWAttributeSubsetStats.CreateDatagrid)
		lInitialDatagridSize =
		    nDatabaseObjectNumber * sizeof(KWDGMCell) +
		    (nSourceValueNumber + nTargetValueNumber) *
			(sizeof(KWDGMPart) + max(sizeof(KWDGInterval), sizeof(KWDGValueSet) + sizeof(KWDGValue)));
		lWorkingMemorySize += lInitialDatagridSize;

		// Plus une grille univariee pour la post-optimisation (cf.
		// KWDataGridPostOptimizer::BuildUnivariateInitialDataGrid)
		lInitialUnivariateDatagridSize = (int)ceil(sqrt(nDatabaseObjectNumber * 1.0)) *
						 (sizeof(KWDGMCell) + sizeof(KWDGMPart) +
						  max(sizeof(KWDGInterval), sizeof(KWDGValueSet) + sizeof(KWDGValue)));
		lWorkingMemorySize += lInitialUnivariateDatagridSize;

		// Plus deux grilles reduites de travail (cf. VNSOptimizer)
		lWorkingDatagridSize =
		    nDatabaseObjectNumber * (sizeof(KWDGMCell) + 2 * sizeof(KWDGValue)) +
		    (int)ceil(sqrt(nDatabaseObjectNumber * 1.0) +
			      min(nTargetValueNumber * 1.0, sqrt(nDatabaseObjectNumber * 1.0))) *
			(sizeof(KWDGMPart) + max(sizeof(KWDGInterval), sizeof(KWDGValueSet) + sizeof(KWDGValue)));
		lWorkingMemorySize += 2 * lWorkingDatagridSize;

		// Plus donnees de travail pour le merge des parties (cf. DataGridMerger)
		lDataGridOptimizationSize = nDatabaseObjectNumber * (sizeof(KWDGMPartMerge) + 2 * sizeof(int));
		lWorkingMemorySize += lDataGridOptimizationSize;

		// Plus cout de post-optimisation univarie, variable par variable
		// Dans le cas numeriquie: limitation du nombre de partie a O(sqrt(N)log(N))
		//   En supervise: on limite l'exploration (cf. DataGridPostOptimizer::OptimizeDataGrid)
		//   En non supervise: on limite la precision initiale (cf.
		//   KWAttributeSubsetStats::CreateAttributeIntervals)
		// Dans le cas categoriel: limitation a ~N/2 (en raison du fourre-tout, qui contient laes valeurs
		// singleton) Donc en tout, au pire: N/2
		lDataGridPostOptimizationSize = (nDatabaseObjectNumber / 2) *
						(sizeof(KWMODLLineDeepOptimization) + 2 * sizeof(int) +
						 sizeof(KWDGPOPartFrequencyVector) + sizeof(KWDGPOCellFrequencyVector));
		lWorkingMemorySize += lDataGridPostOptimizationSize;
	}

	// Affichage des estimations memoire
	if (bDisplayMemoryStats)
	{
		cout << "ComputeNecessaryWorkingMemory" << endl;
		cout << "\tObject number\t" << nDatabaseObjectNumber << endl;
		cout << "\tTarget modality number\t" << nTargetModalityNumber << endl;
		cout << "\tTarget value number\t" << nTargetValueNumber << endl;
		cout << "\tEmpty object size\t" << lEmptyObjectSize << endl;
		cout << "\tContingency table size\t" << LongintToHumanReadableString(lFrequencyTableSize) << endl;
		cout << "\tMinimum loaded database size\t" << LongintToHumanReadableString(lDatabaseSize) << endl;
		cout << "\tTuple table size\t" << LongintToHumanReadableString(lTupleTableSize) << endl;
		cout << "\tFrequency table size\t" << LongintToHumanReadableString(lFrequencyTableSize) << endl;
		cout << "\tMODL line size\t" << lMODLLineSize << endl;
		cout << "\tAll MODL line size\t" << LongintToHumanReadableString(nDatabaseObjectNumber * lMODLLineSize)
		     << endl;
		cout << "\tInitial data grid size\t" << LongintToHumanReadableString(lInitialDatagridSize) << endl;
		cout << "\tInitial univariate data grid size\t"
		     << LongintToHumanReadableString(lInitialUnivariateDatagridSize) << endl;
		cout << "\tWorking data grid size\t" << LongintToHumanReadableString(lWorkingDatagridSize) << endl;
		cout << "\tData grid optimization size\t" << LongintToHumanReadableString(lDataGridOptimizationSize)
		     << endl;
		cout << "\tData grid post-optimization size\t"
		     << LongintToHumanReadableString(lDataGridPostOptimizationSize) << endl;
		cout << "\tRemaining available memory size\t"
		     << LongintToHumanReadableString(RMResourceManager::GetRemainingAvailableMemory()) << endl;
		cout << "\tWorking memory size\t" << LongintToHumanReadableString(lWorkingMemorySize) << endl << endl;
	}
	return lWorkingMemorySize;
}

longint KWDataPreparationTask::ComputeNecessaryClassAttributeMemory() const
{
	longint lClassMemory;
	longint lAttributeMemory;
	KWClass dummyClass;
	KWAttribute* dummyAttribute;

	// Prise en compte d'un dictionnaire avec un seul attribut
	dummyAttribute = new KWAttribute;
	dummyAttribute->SetType(KWType::Symbol);
	dummyAttribute->SetName("Dummy");
	dummyClass.InsertAttribute(dummyAttribute);

	// On rajoute une meta-data comme si l'attribut etait dans un bloc
	dummyAttribute->GetMetaData()->SetDoubleValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(), 1);

	// On supprimer l'attribut pour supprimer la memoire de la classe a vide
	dummyClass.IndexClass();
	lClassMemory = dummyClass.GetUsedMemory();
	dummyClass.DeleteAllAttributes();
	dummyClass.IndexClass();
	lAttributeMemory = lClassMemory - dummyClass.GetUsedMemory();

	// Prise en compte d'un nom d'attribut de taille max
	lAttributeMemory += KWClass::GetNameMaxLength();
	return lAttributeMemory;
}

longint KWDataPreparationTask::ComputeNecessaryClassAttributeBlockMemory() const
{
	longint lClassMemory;
	longint lAttributeBlockMemory;
	KWClass dummyClass;
	KWAttribute* dummyAttribute;
	KWAttributeBlock* dummyAttributeBlock;

	// Prise en compte d'un dictionnaire avec un seul attribut
	dummyAttribute = new KWAttribute;
	dummyAttribute->SetType(KWType::Symbol);
	dummyAttribute->SetName("Dummy");
	dummyClass.InsertAttribute(dummyAttribute);

	// On l'insere dans un bloc
	dummyAttributeBlock = dummyClass.CreateAttributeBlock("DummyBlock", dummyAttribute, dummyAttribute);
	dummyAttributeBlock->SetVarKeyType(KWType::Continuous);
	dummyAttribute->GetMetaData()->SetDoubleValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(), 1);

	// Indexation de la classe
	dummyClass.IndexClass();

	// On supprimer le bloc pour supprimer la memoire de la classe a vide
	lClassMemory = dummyClass.GetUsedMemory();
	dummyClass.DeleteAttributeBlock(dummyAttributeBlock);
	dummyClass.IndexClass();
	lAttributeBlockMemory = lClassMemory - dummyClass.GetUsedMemory();

	// Prise en compte d'un nom d'attribut de taille max
	lAttributeBlockMemory += KWClass::GetNameMaxLength();
	return lAttributeBlockMemory;
}

longint KWDataPreparationTask::ComputeEstimatedMaxValueNumberPerBlock(const KWLearningSpec* learningSpec,
								      const KWTupleTable* targetTupleTable,
								      const KWClass* kwcCompleteClass,
								      const KWClass* kwcPartialClass,
								      int nMaxAttributeNumberPerBlock) const
{
	longint lEstimatedMaxValueNumberPerBlock;
	int nDatabaseObjectNumber;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* completeAttributeBlock;
	double dMeanValueNumberPerAttribute;
	int nMeanValueNumberPerAttribute;
	longint lEstimatedValueNumberPerBlock;
	int i;

	require(learningSpec != NULL);
	require(targetTupleTable != NULL);
	require(kwcCompleteClass != NULL);
	require(kwcPartialClass != NULL);
	require(nMaxAttributeNumberPerBlock >= 0);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Parcours de tous les blocs et attribut pour estimer la place memoire totale necessaire pour le
	// stockage de toutes les valeurs de tous les attributs
	// Utilisation d'une heuristique pour les attributs de bloc
	lEstimatedMaxValueNumberPerBlock = 0;
	for (i = 0; i < kwcPartialClass->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcPartialClass->GetLoadedAttributeBlockAt(i);
		if (KWType::IsSimple(attributeBlock->GetType()))
		{
			assert(attributeBlock->GetLoadedAttributeNumber() > 0);

			// Acces a la version complete du bloc
			completeAttributeBlock = kwcCompleteClass->LookupAttributeBlock(attributeBlock->GetName());
			assert(completeAttributeBlock != NULL);
			assert(completeAttributeBlock->GetType() == attributeBlock->GetType());
			assert(completeAttributeBlock->GetLoadedAttributeNumber() >=
			       attributeBlock->GetLoadedAttributeNumber());

			// Estimation du nombre moyen de valeurs par attribut du bloc, en se basant sur le bloc complet
			// pour exploiter son estimation heuristique de nombre de valeurs par bloc
			dMeanValueNumberPerAttribute = KWAttributeBlock::GetEstimatedMeanValueNumber(
			    completeAttributeBlock->GetLoadedAttributeNumber());
			dMeanValueNumberPerAttribute *= nDatabaseObjectNumber;
			dMeanValueNumberPerAttribute /= completeAttributeBlock->GetLoadedAttributeNumber();
			nMeanValueNumberPerAttribute = (int)ceil(dMeanValueNumberPerAttribute);

			// Nombre de tuples simultanement en memoire pour charger toutes les tables de tuples du bloc,
			// dans la limite du nombre max d'attributs a prendre en compte
			lEstimatedValueNumberPerBlock = nMeanValueNumberPerAttribute;
			lEstimatedValueNumberPerBlock *=
			    min(nMaxAttributeNumberPerBlock, attributeBlock->GetLoadedAttributeNumber());

			// On prend le max sur les bloc rencontres
			lEstimatedMaxValueNumberPerBlock =
			    max(lEstimatedMaxValueNumberPerBlock, lEstimatedValueNumberPerBlock);
		}
	}
	return lEstimatedMaxValueNumberPerBlock;
}

longint KWDataPreparationTask::ComputeEstimatedTotalValueNumberInBlocks(const KWLearningSpec* learningSpec,
									const KWTupleTable* targetTupleTable,
									const KWClass* kwcCompleteClass,
									const KWClass* kwcPartialClass) const
{
	longint lEstimatedTotalValueNumberInBlocks;
	int nDatabaseObjectNumber;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* completeAttributeBlock;
	double dMeanValueNumberPerAttribute;
	int nMeanValueNumberPerAttribute;
	longint lValueNumberInMemory;
	int i;

	require(learningSpec != NULL);
	require(targetTupleTable != NULL);
	require(kwcCompleteClass != NULL);
	require(kwcPartialClass != NULL);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Parcours de tous les blocs pour estimer le nombre total de valeurs des blocs
	// Utilisation d'une heuristique pour les attributs de bloc
	lEstimatedTotalValueNumberInBlocks = 0;
	for (i = 0; i < kwcPartialClass->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcPartialClass->GetLoadedAttributeBlockAt(i);
		if (KWType::IsSimple(attributeBlock->GetType()))
		{
			// Acces a la version complete du bloc
			completeAttributeBlock = kwcCompleteClass->LookupAttributeBlock(attributeBlock->GetName());
			assert(completeAttributeBlock != NULL);
			assert(completeAttributeBlock->GetType() == attributeBlock->GetType());
			assert(completeAttributeBlock->GetLoadedAttributeNumber() >=
			       attributeBlock->GetLoadedAttributeNumber());

			// Estimation du nombre moyen de valeurs par attribut du bloc, en se basant sur le bloc complet
			// pour exploiter son estimation heuristique de nombre de valeurs par bloc
			dMeanValueNumberPerAttribute = KWAttributeBlock::GetEstimatedMeanValueNumber(
			    completeAttributeBlock->GetLoadedAttributeNumber());
			dMeanValueNumberPerAttribute *= nDatabaseObjectNumber;
			dMeanValueNumberPerAttribute /= completeAttributeBlock->GetLoadedAttributeNumber();
			nMeanValueNumberPerAttribute = (int)ceil(dMeanValueNumberPerAttribute);

			// Nombre valeur total du bloc a charger en memoire
			lValueNumberInMemory = nMeanValueNumberPerAttribute;
			lValueNumberInMemory *= attributeBlock->GetLoadedAttributeNumber();

			// Memoire pour charger l'ensemble des valeurs du bloc, selon cette estimation heuristique
			lEstimatedTotalValueNumberInBlocks += lValueNumberInMemory;
		}
	}
	return lEstimatedTotalValueNumberInBlocks;
}

longint KWDataPreparationTask::ComputeBlockWorkingMemory(int nBlockAttributeNumber, longint lBlockValueNumber,
							 int nObjectNumber) const
{
	longint lEstimatedBlockWorkingMemory;
	int nMeanValueNumberPerAttribute;
	longint lAttributeBuildingMemorySize;

	require(nBlockAttributeNumber >= 0);
	require(lBlockValueNumber >= 0);
	require(nObjectNumber >= 0);

	// Nombre moyen de valeur par attribut du plus grand bloc
	// Attention: le nombre max d'attribut d'un bloc et le nombre max de valeurs par bloc pourrait
	// etre associe a des bloc de taille differente, mais ce n'est pas important pour le dimensionnement
	nMeanValueNumberPerAttribute = 0;
	if (nBlockAttributeNumber > 0)
		nMeanValueNumberPerAttribute = (int)ceil(lBlockValueNumber * 1.0 / nBlockAttributeNumber);

	// Memoire pour charger l'ensemble des valeurs de tous les attributs du bloc via des tuples
	// On prend ici en compte que le cout de construction pour les valeurs (via une SortedList)
	lAttributeBuildingMemorySize = KWTupleTable::ComputeNecessaryMemory(0, 2) +
				       KWTupleTable::ComputeNecessaryBuildingMemory(nMeanValueNumberPerAttribute);

	// Evaluation de la memoire pour gerer toutes les tables de tuples a vide pour les attributs du plus gros bloc
	lEstimatedBlockWorkingMemory = nBlockAttributeNumber * lAttributeBuildingMemorySize;

	return lEstimatedBlockWorkingMemory;
}

longint KWDataPreparationTask::ComputeDatabaseAllValuesMemory(KWDataTableSlice* slice, int nObjectNumber) const
{
	longint lComputeDatabaseAllValuesMemory;
	KWAttribute* attribute;
	int nDenseAttributeIndex;

	require(slice != NULL);
	require(nObjectNumber >= 0);

	// Calcul la place memoire totale minimum necessaire pour le stockage de toutes les valeurs de tous les
	// attributs charges d'une classe On se base ici sur aucun attributs Symbol, car ils sont pris en compte
	// precisement ensuite
	lComputeDatabaseAllValuesMemory = ComputeDatabaseMinimumAllValuesMemory(
	    0, slice->GetClass()->GetLoadedDenseAttributeNumber(), slice->GetClass()->GetLoadedAttributeBlockNumber(),
	    slice->GetTotalAttributeBlockValueNumber(), nObjectNumber);

	// Parcours des attributs denses Symbol pour estimer la place necessaire a leur gestion
	nDenseAttributeIndex = 0;
	attribute = slice->GetClass()->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas d'un attribut dense
		if (not attribute->IsInBlock())
		{
			if (attribute->GetUsed())
			{
				// On ne prend en compte que les attributs charges en memoire
				if (attribute->GetUsed() and attribute->GetType() == KWType::Symbol)
					lComputeDatabaseAllValuesMemory += ComputeEstimatedAttributeSymbolValuesMemory(
					    slice->GetDenseSymbolAttributeDiskSizes()->GetAt(nDenseAttributeIndex),
					    nObjectNumber);
				nDenseAttributeIndex++;
			}
		}
		// cas d'un bloc
		else
		{
			// On va directement en fin de bloc
			attribute = attribute->GetAttributeBlock()->GetLastAttribute();
		}

		// Attribute suivant
		slice->GetClass()->GetNextAttribute(attribute);
	}
	return lComputeDatabaseAllValuesMemory;
}

longint KWDataPreparationTask::ComputeDatabaseMinimumAllValuesMemory(int nDenseSymbolAttributeNumber,
								     int nDenseContinuousAttributeNumber,
								     int nAttributeBlocNumber,
								     longint lTotalBlockValueNumber,
								     int nObjectNumber) const
{
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	longint lDatabaseAllValuesMemory;

	require(nDenseSymbolAttributeNumber >= 0);
	require(nDenseContinuousAttributeNumber >= 0);
	require(nAttributeBlocNumber >= 0);
	require(lTotalBlockValueNumber >= 0);
	require(nObjectNumber >= 0);

	// Memoire pour les valeurs des attributs dense
	lDatabaseAllValuesMemory = (nDenseSymbolAttributeNumber + nDenseContinuousAttributeNumber) *
				   GetNecessaryMemoryPerDenseValue() * nObjectNumber;

	// Prise uen compte d'un overhead en cas d'attributs d'ense Symbol
	if (nDenseSymbolAttributeNumber > 0)
		lDatabaseAllValuesMemory += ComputeEstimatedAttributeSymbolValuesMemory(
		    nObjectNumber * GetExpectedMeanSymbolValueLength(), nObjectNumber);

	// Memoire pour charger les pointeurs sur les blocs ainsi que les blocs a vides
	lDatabaseAllValuesMemory += nAttributeBlocNumber * GetNecessaryMemoryPerEmptyValueBlock() * nObjectNumber;

	// Memoire necessaire pour stocker  les valeurs denses
	lDatabaseAllValuesMemory += lTotalBlockValueNumber * GetNecessaryMemoryPerSparseValue();

	// Affichage des details de dimensionnement
	if (bDisplayMemoryStats)
	{
		cout << "\t  ComputeDatabaseMinimumAllValuesMemory" << endl;
		cout << "\t\tDense symbol attribute number\t" << nDenseSymbolAttributeNumber << endl;
		cout << "\t\tDense continuous attribute number\t" << nDenseContinuousAttributeNumber << endl;
		cout << "\t\tAttribute block number\t" << nAttributeBlocNumber << endl;
		cout << "\t\tTotal block value number\t" << lTotalBlockValueNumber << endl;
		cout << "\t\tObject number\t" << nObjectNumber << endl;
		cout << "\t\tNecessaryMemoryPerDenseValue\t" << GetNecessaryMemoryPerDenseValue() << endl;
		cout << "\t\tExpectedMeanSymbolValueLength\t" << GetExpectedMeanSymbolValueLength() << endl;
		cout << "\t\tNecessaryMemoryPerEmptyValueBlock\t" << GetNecessaryMemoryPerEmptyValueBlock() << endl;
		cout << "\t\tNecessaryMemoryPerSparseValue\t" << GetNecessaryMemoryPerSparseValue() << endl;
		cout << "\t\tDatabase all values memory\t" << LongintToHumanReadableString(lDatabaseAllValuesMemory)
		     << endl;
	}
	return lDatabaseAllValuesMemory;
}

longint KWDataPreparationTask::ComputeEstimatedAttributeSymbolValuesMemory(longint lSymbolAttributeDiskSize,
									   int nObjectNumber) const
{
	longint lEstimatedAttributeSymbolValuesMemory;
	int nMeanSymbolValueLength;
	int nEstimatedDistinctValueNumber;

	require(lSymbolAttributeDiskSize >= 0);
	require(nObjectNumber >= 0);

	// Longueur moyenne des valeurs
	nMeanSymbolValueLength = int(lSymbolAttributeDiskSize / max(1, nObjectNumber));

	// Estimation heuristique du nombre de valeur differentes
	if (nMeanSymbolValueLength >= GetExpectedMeanSymbolValueLength())
		// On s'attend a autant de valeurs que d'instances nsi la taille moyenne des valeurs est grande
		nEstimatedDistinctValueNumber = nObjectNumber;
	else
	{
		// On s'attend a de petits nombres de valeurs distinctes si la taille moyenne des valeurs est petite
		assert(nMeanSymbolValueLength <= log(INT_MAX) / log(2));
		nEstimatedDistinctValueNumber = min((int)pow(2, nMeanSymbolValueLength + 1), nObjectNumber);
	}

	// Heuristique pour estimer la la taille necessaire en memoire
	lEstimatedAttributeSymbolValuesMemory = lSymbolAttributeDiskSize;
	lEstimatedAttributeSymbolValuesMemory += nEstimatedDistinctValueNumber * Symbol::GetUsedMemoryPerSymbol();
	return lEstimatedAttributeSymbolValuesMemory;
}

longint KWDataPreparationTask::GetNecessaryMemoryPerDenseValue() const
{
	return sizeof(KWValue);
}

longint KWDataPreparationTask::GetNecessaryMemoryPerEmptyValueBlock() const
{
	return (sizeof(KWValue) + sizeof(void*) + max(sizeof(KWSymbolValueBlock), sizeof(KWContinuousValueBlock)));
}

longint KWDataPreparationTask::GetNecessaryMemoryPerSparseValue() const
{
	return sizeof(KWValueIndexPair);
}

longint KWDataPreparationTask::GetExpectedMeanSymbolValueLength() const
{
	// On se base arbitrairement su une longueur moyenne egale a celle d'un champ
	return sizeof(KWValue);
}

longint KWDataPreparationTask::ComputeEstimatedMaxBlockWorkingMemory(const KWLearningSpec* learningSpec,
								     const KWTupleTable* targetTupleTable,
								     const KWClass* kwcCompleteClass,
								     const KWClass* kwcPartialClass,
								     int nMaxAttributeNumberPerBlock) const
{
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	longint lEstimatedMaxBlockWorkingMemory;
	int nMaxBlockAttributeNumber;
	longint lEstimatedMaxValueNumberPerBlock;
	int nDatabaseObjectNumber;
	KWAttributeBlock* attributeBlock;
	int i;

	require(learningSpec != NULL);
	require(targetTupleTable != NULL);
	require(kwcCompleteClass != NULL);
	require(kwcPartialClass != NULL);
	require(nMaxAttributeNumberPerBlock >= 0);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Estimation heuristique des nombres max de valeur par bloc  d'une sous-partie d'une classe a partitionner
	lEstimatedMaxValueNumberPerBlock = ComputeEstimatedMaxValueNumberPerBlock(
	    learningSpec, targetTupleTable, kwcCompleteClass, kwcPartialClass, nMaxAttributeNumberPerBlock);

	// Parcours de tous les blocs pour identifier celui de plus grande taille
	nMaxBlockAttributeNumber = 0;
	for (i = 0; i < kwcPartialClass->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcPartialClass->GetLoadedAttributeBlockAt(i);
		assert(KWType::IsSimple(attributeBlock->GetType()));
		assert(attributeBlock->GetLoadedAttributeNumber() > 0);
		nMaxBlockAttributeNumber = max(nMaxBlockAttributeNumber, attributeBlock->GetLoadedAttributeNumber());
	}

	// Correction avec la contrainte sur le nombre max d'attribut par bloc a prendre en compte
	nMaxBlockAttributeNumber = min(nMaxBlockAttributeNumber, nMaxAttributeNumberPerBlock);

	// Calcul de la memoire de travail necessaire pour traiter le bloc
	lEstimatedMaxBlockWorkingMemory = ComputeBlockWorkingMemory(
	    nMaxBlockAttributeNumber, lEstimatedMaxValueNumberPerBlock, nDatabaseObjectNumber);

	// Affichage des details de dimensionnement
	if (bDisplayMemoryStats)
	{
		cout << "\t  ComputeEstimatedMaxBlockWorkingMemory" << endl;
		cout << "\t\tMax attribute number per block\t" << nMaxAttributeNumberPerBlock << endl;

		cout << "\t\tObjectNumber\t" << nDatabaseObjectNumber << endl;
		cout << "\t\tEstimated max value number per block\t" << lEstimatedMaxValueNumberPerBlock << endl;
		cout << "\t\tMax block attribute number\t" << nMaxBlockAttributeNumber << endl;
		cout << "\t\tEstimated max block working memory\t"
		     << LongintToHumanReadableString(lEstimatedMaxBlockWorkingMemory) << endl;
	}
	return lEstimatedMaxBlockWorkingMemory;
}

longint KWDataPreparationTask::ComputeEstimatedDatabaseAllValuesMemory(const KWLearningSpec* learningSpec,
								       const KWTupleTable* targetTupleTable,
								       const KWClass* kwcCompleteClass,
								       const KWClass* kwcPartialClass) const
{
	longint lEstimatedDatabaseAllValuesMemory;
	longint lEstimatedTotalValueNumberInBlocks;
	int nDatabaseObjectNumber;
	int nDenseSymbolAttributeNumber;
	int nDenseContinuousAttributeNumber;
	KWAttribute* attribute;
	int nAttribute;

	require(learningSpec != NULL);
	require(targetTupleTable != NULL);
	require(kwcCompleteClass != NULL);
	require(kwcPartialClass != NULL);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Estimation heuristiques du nombre total de valeur une sous-partie d'une classe a partitionner
	lEstimatedTotalValueNumberInBlocks =
	    ComputeEstimatedTotalValueNumberInBlocks(learningSpec, targetTupleTable, kwcCompleteClass, kwcPartialClass);

	// Calcul du nombre d'attribut denses
	nDenseSymbolAttributeNumber = 0;
	nDenseContinuousAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < kwcPartialClass->GetLoadedDenseAttributeNumber(); nAttribute++)
	{
		attribute = kwcPartialClass->GetLoadedDenseAttributeAt(nAttribute);
		if (attribute->GetName() != learningSpec->GetTargetAttributeName())
		{
			if (attribute->GetType() == KWType::Symbol)
				nDenseSymbolAttributeNumber++;
			else if (attribute->GetType() == KWType::Continuous)
				nDenseContinuousAttributeNumber++;
		}
	}

	// Calcul de la memoire de travail necessaire pour stocker toutes les valeurs
	lEstimatedDatabaseAllValuesMemory =
	    ComputeDatabaseMinimumAllValuesMemory(nDenseSymbolAttributeNumber, nDenseContinuousAttributeNumber,
						  kwcPartialClass->GetLoadedAttributeBlockNumber(),
						  lEstimatedTotalValueNumberInBlocks, nDatabaseObjectNumber);
	return lEstimatedDatabaseAllValuesMemory;
}

boolean KWDataPreparationTask::RunDataPreparationTask(KWLearningSpec* learningSpec,
						      KWTupleTableLoader* tupleTableLoader,
						      KWDataTableSliceSet* dataTableSliceSet)
{
	boolean bOk;
	boolean bDisplayUserMessage = false;
	ALString sTmp;

	require(tupleTableLoader != NULL);
	require(CheckInputParameters(learningSpec, tupleTableLoader->GetInputExtraAttributeTupleTable()));
	require(dataTableSliceSet != NULL);
	require(dataTableSliceSet != NULL);
	require(dataTableSliceSet->GetClassName() == learningSpec->GetClass()->GetName());
	require(dataTableSliceSet->GetTargetAttributeName() == learningSpec->GetTargetAttributeName());

	// Parametrage des variables partagees
	shared_learningSpec.SetLearningSpec(learningSpec);
	shared_svTargetValues.SetSymbolVector(
	    cast(SymbolVector*, tupleTableLoader->GetInputExtraAttributeSymbolValues()));
	shared_cvTargetValues.SetContinuousVector(
	    cast(ContinuousVector*, tupleTableLoader->GetInputExtraAttributeContinuousValues()));
	shared_TargetTupleTable.SetTupleTable(
	    cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable()));

	// Parametrage des variables du maitre
	masterDataTableSliceSet = dataTableSliceSet;

	// Lancement de la tache
	bOk = Run();

	// Message d'erreur si necessaire
	if (bDisplayUserMessage)
	{
		if (IsJobDone() and IsTaskInterruptedByUser())
			AddWarning("Interrupted by user");
		else if (not IsJobDone() or not IsJobSuccessful())
			AddError("Interrupted because of errors");

		// Temps d'execution de la tache
		if (IsJobDone() and IsJobSuccessful() and not IsTaskInterruptedByUser())
			AddSimpleMessage(GetTaskLabel() + " time: " + SecondsToString(GetJobElapsedTime()));
	}

	// Dereferencement des variables partagees
	shared_learningSpec.RemoveObject();
	shared_svTargetValues.RemoveObject();
	shared_cvTargetValues.RemoveObject();
	shared_TargetTupleTable.RemoveObject();

	// Nettoyage des variables du maitre
	masterDataTableSliceSet = NULL;
	return bOk;
}

boolean KWDataPreparationTask::MasterInitialize()
{
	boolean bOk = true;

	require(masterClass == NULL);
	require(masterDatabase == NULL);

	// Memorisation de la classe et de la base des learningSpec partagees
	// En effet, ces parametres de learningSpec sont temporairement modifies
	// dans les esclaves, et doivent etre restitues apres la fin de la tache
	masterClass = shared_learningSpec.GetLearningSpec()->GetClass();
	masterDatabase = shared_learningSpec.GetLearningSpec()->GetDatabase();

	return bOk;
}

boolean KWDataPreparationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage des resultats en cas d'erreur
	bOk = bProcessEndedCorrectly;

	// Restitution de la classe et de la base des learningSpec partagees
	shared_learningSpec.FinalizeSpecification(masterClass, masterDatabase);
	masterClass = NULL;
	masterDatabase = NULL;
	return bOk;
}

boolean KWDataPreparationTask::SlaveInitialize()
{
	boolean bOk = true;

	// Initialisation de la partie attribut cible du chargeur de tuples de l'esclave
	slaveTupleTableLoader.SetInputExtraAttributeName(
	    shared_learningSpec.GetLearningSpec()->GetTargetAttributeName());
	slaveTupleTableLoader.SetInputExtraAttributeType(
	    shared_learningSpec.GetLearningSpec()->GetTargetAttributeType());
	slaveTupleTableLoader.SetInputExtraAttributeTupleTable(shared_TargetTupleTable.GetTupleTable());
	if (slaveTupleTableLoader.GetInputExtraAttributeType() == KWType::Continuous)
	{
		assert(shared_cvTargetValues.GetSize() ==
		       slaveTupleTableLoader.GetInputExtraAttributeTupleTable()->GetTotalFrequency());
		slaveTupleTableLoader.SetInputExtraAttributeContinuousValues(
		    shared_cvTargetValues.GetConstContinuousVector());
	}
	else if (slaveTupleTableLoader.GetInputExtraAttributeType() == KWType::Symbol)
	{
		assert(shared_svTargetValues.GetSize() ==
		       slaveTupleTableLoader.GetInputExtraAttributeTupleTable()->GetTotalFrequency());
		slaveTupleTableLoader.SetInputExtraAttributeSymbolValues(shared_svTargetValues.GetConstSymbolVector());
	}
	return bOk;
}

boolean KWDataPreparationTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage du chargeur de tuples
	slaveTupleTableLoader.RemoveAllInputs();
	return bOk;
}
