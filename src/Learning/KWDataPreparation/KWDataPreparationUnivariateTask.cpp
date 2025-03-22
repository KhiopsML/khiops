// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPreparationUnivariateTask.h"

KWDataPreparationUnivariateTask::KWDataPreparationUnivariateTask()
{
	// Variables partagees
	DeclareSharedParameter(&shared_nLargestSliceAttributeNumber);
	DeclareSharedParameter(&shared_lLargestSliceUsedMemory);
	DeclareSharedParameter(&shared_lLargestSliceMaxBlockWorkingMemory);
	DeclareSharedParameter(&shared_lLargestSliceDatabaseAllValuesMemory);

	// Variables en entree et sortie des esclaves
	DeclareTaskInput(&input_DataTableSlice);
	DeclareTaskOutput(&output_sSliceClassName);
	output_oaSliceAttributeStats = new PLShared_ObjectArray(new PLShared_AttributeStats);
	DeclareTaskOutput(output_oaSliceAttributeStats);
	output_oaDataTableSubSlices = new PLShared_ObjectArray(new PLShared_DataTableSlice);
	DeclareTaskOutput(output_oaDataTableSubSlices);

	// Initialisation des variables du maitre
	nMasterTotalAttributeNumber = 0;
	dMasterCumulatedPercentage = 0;
	slMasterSortedSlices = NULL;
	odMasterSlices = NULL;
	odMasterDatabaseAttributeStats = NULL;
}

KWDataPreparationUnivariateTask::~KWDataPreparationUnivariateTask()
{
	delete output_oaSliceAttributeStats;
	delete output_oaDataTableSubSlices;
}

boolean KWDataPreparationUnivariateTask::CollectPreparationStats(KWLearningSpec* learningSpec,
								 KWTupleTableLoader* tupleTableLoader,
								 KWDataTableSliceSet* dataTableSliceSet,
								 ObjectDictionary* odOutputAttributeStats)
{
	boolean bOk;

	require(learningSpec != NULL);
	require(tupleTableLoader != NULL);
	require(dataTableSliceSet != NULL);
	require(dataTableSliceSet->Check());
	require(odOutputAttributeStats != NULL);
	require(odOutputAttributeStats->GetCount() == 0);

	// Parametrage de la collecte des resultats par le maitre
	nMasterTotalAttributeNumber = dataTableSliceSet->GetTotalAttributeNumber();
	dMasterCumulatedPercentage = 0;
	odMasterDatabaseAttributeStats = odOutputAttributeStats;

	// Lancement de la tache de preparation
	bOk = RunDataPreparationTask(learningSpec, tupleTableLoader, dataTableSliceSet);
	if (not bOk)
		odOutputAttributeStats->DeleteAll();

	// Nettoyage des variables du maitre
	nMasterTotalAttributeNumber = 0;
	dMasterCumulatedPercentage = 0;
	odMasterDatabaseAttributeStats = NULL;
	ensure(dataTableSliceSet->Check());
	return bOk;
}

boolean KWDataPreparationUnivariateTask::BasicCollectPreparationStats(KWLearningSpec* learningSpec,
								      KWTupleTableLoader* tupleTableLoader,
								      ObjectArray* oaInputAttributes,
								      boolean bDisplayTaskProgression,
								      ObjectArray* oaOutputAttributeStats) const
{
	boolean bOk = true;
	int nAttributeNumber;
	int nAttribute;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ObjectArray oaBlockAttributes;
	ObjectDictionary odBlockAttributes;
	ObjectDictionary odBlockTupleTables;
	KWTupleTable univariateTupleTable;
	KWTupleTable* tupleTable;
	int i;
	KWAttributeStats* attributeStats;
	ALString sTmp;

	require(learningSpec != NULL);
	require(tupleTableLoader != NULL);
	require(tupleTableLoader->GetInputClass() == learningSpec->GetClass());
	require(tupleTableLoader->GetInputClass() != NULL);
	require(tupleTableLoader->GetInputClass()->IsCompiled());
	require(tupleTableLoader->GetInputDatabaseObjects() != NULL);
	require(learningSpec->GetTargetAttributeName() == tupleTableLoader->GetInputExtraAttributeName());
	require(learningSpec->GetInstanceNumber() == tupleTableLoader->GetInputDatabaseObjects()->GetSize());
	require(learningSpec->GetTargetAttributeType() == KWType::None or
		learningSpec->GetTargetDescriptiveStats()->GetInstanceNumber() ==
		    tupleTableLoader->GetInputDatabaseObjects()->GetSize());
	require(learningSpec->GetTargetAttributeType() != KWType::Symbol or
		tupleTableLoader->GetInputDatabaseObjects()->GetSize() ==
		    tupleTableLoader->GetInputExtraAttributeSymbolValues()->GetSize());
	require(learningSpec->GetTargetAttributeType() != KWType::Continuous or
		tupleTableLoader->GetInputDatabaseObjects()->GetSize() ==
		    tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize());
	require(oaInputAttributes != NULL);
	require(AreAttributesSortedByBlock(oaInputAttributes));
	require(oaOutputAttributeStats != NULL);
	require(oaOutputAttributeStats->GetSize() == 0);

	// Calcul des statistiques
	bOk = not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// Pretraitement des attributs de la tranche
		if (bDisplayTaskProgression and TaskProgression::IsInTask())
			TaskProgression::DisplayMainLabel("Discretizations and value groupings");
		nAttributeNumber = oaInputAttributes->GetSize();
		nAttribute = 0;
		while (nAttribute < nAttributeNumber)
		{
			// Acces a l'attribut correspondant aux stats
			attribute = cast(KWAttribute*, oaInputAttributes->GetAt(nAttribute));
			assert(attribute->GetLoaded());
			assert(KWType::IsSimple(attribute->GetType()));
			assert(attribute->GetName() != learningSpec->GetTargetAttributeName());

			// Cas ou l'attribut n'est pas dans un bloc
			if (not attribute->IsInBlock())
			{
				// Gestion de la progression
				if (bDisplayTaskProgression and TaskProgression::IsInTask())
				{
					TaskProgression::DisplayLabel(sTmp + attribute->GetName());
					TaskProgression::DisplayProgression(
					    (int)(100 * (nAttribute + 1.0) / nAttributeNumber));
				}

				// Arret si erreur ou interruption
				if (TaskProgression::IsInterruptionRequested())
					bOk = false;
				if (not bOk)
					break;

				// Chargement de la table de tuple pour l'attribut
				tupleTableLoader->LoadUnivariate(attribute->GetName(), &univariateTupleTable);

				// Creation et initialisation d'un objet de stats pour l'attribut
				attributeStats = new KWAttributeStats;
				attributeStats->SetLearningSpec(learningSpec);
				attributeStats->SetAttributeName(attribute->GetName());
				attributeStats->SetAttributeType(attribute->GetType());
				oaOutputAttributeStats->Add(attributeStats);

				// Calcul des statistitique univariee a partir de la table de tuples
				bOk = attributeStats->ComputeStats(&univariateTupleTable);
				univariateTupleTable.CleanAll();

				// On se prepare a l'attribut suivant
				nAttribute++;
			}
			// Cas ou l'attribut est dans un block
			else
			{
				attributeBlock = attribute->GetAttributeBlock();

				// Collecte de tous les attributs consecutifs du meme bloc
				assert(oaBlockAttributes.GetSize() == 0);
				while (attribute->GetAttributeBlock() == attributeBlock)
				{
					oaBlockAttributes.Add(attribute);
					odBlockAttributes.SetAt(attribute->GetName(), attribute);
					nAttribute++;

					// Recherche de l'attribut suivant
					if (nAttribute < nAttributeNumber)
						attribute = cast(KWAttribute*, oaInputAttributes->GetAt(nAttribute));
					else
						break;
				}
				assert(attribute->GetAttributeBlock() != attributeBlock or
				       nAttribute == nAttributeNumber);

				// Chargement de la table de tuple pour tous les attributs du bloc
				tupleTableLoader->BlockLoadUnivariateInitialize(
				    attributeBlock->GetName(), &odBlockAttributes, &odBlockTupleTables);

				// Parcours des attributs du bloc
				for (i = 0; i < oaBlockAttributes.GetSize(); i++)
				{
					// Acces a l'attribut correspondant aux stats
					attribute = cast(KWAttribute*, oaBlockAttributes.GetAt(i));
					assert(attribute->GetLoaded());
					assert(KWType::IsSimple(attribute->GetType()));
					assert(attribute->GetName() != learningSpec->GetTargetAttributeName());

					// Gestion de la progression
					if (bDisplayTaskProgression and TaskProgression::IsInTask())
					{
						TaskProgression::DisplayLabel(sTmp + attribute->GetName());
						TaskProgression::DisplayProgression(
						    ((nAttribute - oaBlockAttributes.GetSize() + i + 1) * 100) /
						    nAttributeNumber);
					}

					// Arret si erreur ou interruption
					if (TaskProgression::IsInterruptionRequested())
						bOk = false;
					if (not bOk)
					{
						odBlockTupleTables.DeleteAll();
						break;
					}

					// Acces la table de tuple courante
					tupleTable =
					    cast(KWTupleTable*, odBlockTupleTables.Lookup(attribute->GetName()));
					assert(tupleTable != NULL);
					assert(tupleTable->GetAttributeNameAt(0) == attribute->GetName());

					// Finalisation de l'alimentation de la table, en prenant en compte les valeurs
					// manquantes
					tupleTableLoader->BlockLoadUnivariateFinalize(attributeBlock->GetName(),
										      tupleTable);

					// Creation et initialisation d'un objet de stats pour l'attribut
					attributeStats = new KWAttributeStats;
					attributeStats->SetLearningSpec(learningSpec);
					attributeStats->SetAttributeName(attribute->GetName());
					attributeStats->SetAttributeType(attribute->GetType());
					oaOutputAttributeStats->Add(attributeStats);

					// Calcul des statistitique univariee a partir de la table de tuples
					bOk = attributeStats->ComputeStats(tupleTable);

					// On detruit la table de tuple et on la supprime du dictionnaire
					// qui pourrait etre detruit exhaustivement en cas d'interruption utilisateur
					odBlockTupleTables.RemoveKey(attribute->GetName());
					delete tupleTable;
				}
				assert(odBlockTupleTables.GetCount() == 0);

				// Nettoyage
				oaBlockAttributes.RemoveAll();
				odBlockAttributes.RemoveAll();
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}

	// Nettoyage des sorties si erreur ou interruption apres traitement de la derniere variable
	if (not bOk)
		oaOutputAttributeStats->DeleteAll();
	ensure(bOk or oaOutputAttributeStats->GetSize() == 0);
	ensure(not bOk or oaOutputAttributeStats->GetSize() == oaInputAttributes->GetSize());
	return bOk;
}

void KWDataPreparationUnivariateTask::CollectInputAttributes(KWLearningSpec* learningSpec, KWClass* kwcClass,
							     ObjectArray* oaCollectedAttributes) const
{
	int i;
	KWAttribute* attribute;

	require(learningSpec != NULL);
	require(kwcClass != NULL);
	require(kwcClass->IsCompiled());
	require(oaCollectedAttributes != NULL);
	require(oaCollectedAttributes->GetSize() == 0);

	// Collecte des attribut charges a analyser
	for (i = 0; i < kwcClass->GetLoadedAttributeNumber(); i++)
	{
		attribute = learningSpec->GetClass()->GetLoadedAttributeAt(i);

		// On regarde uniquement les attribut concernes
		if (KWType::IsSimple(attribute->GetType()) and
		    attribute->GetName() != learningSpec->GetTargetAttributeName())
			oaCollectedAttributes->Add(attribute);
	}
	ensure(AreAttributesSortedByBlock(oaCollectedAttributes));
}

const ALString KWDataPreparationUnivariateTask::GetTaskName() const
{
	return "Univariate data preparation";
}

PLParallelTask* KWDataPreparationUnivariateTask::Create() const
{
	return new KWDataPreparationUnivariateTask;
}

boolean KWDataPreparationUnivariateTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	KWLearningSpec* learningSpec;
	const KWTupleTable* targetTupleTable;
	int nDatabaseObjectNumber;
	KWDataTableSlice* slice;
	int nSlice;
	int nTotalAttributeNumber;
	longint lSliceMaxBlockWorkingMemory;
	longint lSliceDatabaseAllValuesMemory;
	longint lTotalSliceMaxBlockWorkingMemory;
	longint lTotalSliceDatabaseAllValuesMemory;
	longint lMaxSliceMaxBlockWorkingMemory;
	longint lMaxSliceDatabaseAllValuesMemory;
	longint lMeanSliceMaxBlockWorkingMemory;
	longint lMeanSliceDatabaseAllValuesMemory;
	longint lNecessaryWorkingMemory;
	longint lNecessaryTargetAttributMemory;
	longint lNecessaryUnivariateStatsMemory;

	require(CheckInputParameters(shared_learningSpec.GetLearningSpec(), shared_TargetTupleTable.GetTupleTable()));

	//////////////////////////////////////////////////////////////////////////
	// Estimation des ressources necessaires pour la preparation des donnees

	// Calcul de la memoire de travail et de la memoire de stockage par resultat
	learningSpec = shared_learningSpec.GetLearningSpec();
	targetTupleTable = shared_TargetTupleTable.GetTupleTable();
	lNecessaryWorkingMemory = ComputeNecessaryWorkingMemory(learningSpec, targetTupleTable, false);
	lNecessaryTargetAttributMemory = ComputeNecessaryTargetAttributeMemory(learningSpec, targetTupleTable);
	lNecessaryUnivariateStatsMemory = ComputeNecessaryUnivariateStatsMemory(learningSpec, targetTupleTable);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Nombre total d'attributs a traiter
	nTotalAttributeNumber = masterDataTableSliceSet->GetTotalAttributeNumber();

	// Statistiques sur la plus grande tranche
	// Les statistiques collectees concernent par tranche
	//   . le nombre d'attributs, sparse ou non: pour le dimensionnement des resultats de preparation
	//   . la memoire utilise par la tranche: principalement, celle pour la definition de la kwClass
	//   . la memoire de travail pour la gestion des blocs: pour charger simultanement toutes les tables de tuples
	//   du plus gros bloc de la tranche . la memoire de travail pour le stockage de toutes les valeurs de la
	//   tranche
	// Attention, ces statistiques sont basees sur des estimations heuristiques (notamment pour le taux de
	// remplissage des blocs sparse), les memes estimations qui ont ete utilisees pour le dimensionnement du
	// sliceSet. Cela permet de lancer la taches, meme si les taux de remplissage effectifs sont plus grands que les
	// estimations. Au cas ou ces estimations serait sous-evaluees, se sera a la charge de chaque esclave d'adapter
	// ses algorithmes aux ressources disponibles. Pour cela, l'esclave dispose des elements de dimensionnement et
	// des estimations de nombre de tuples ou de valeurs a la base de ces estimations. Attention: dans les cas
	// extremes, on peut avoir obtenu assez de ressources pour lancer la tache de construction du slice set, base
	// sur une estimation heuristique du nombre moyen de valeur par tranche, mais ne pas pouvoir exploiter ce slice
	// set une fois fabrique, en raison des nombres de valeurs obtenus par tranche du slice set instancie
	shared_nLargestSliceAttributeNumber = 0;
	shared_lLargestSliceUsedMemory = 0;
	shared_lLargestSliceMaxBlockWorkingMemory = 0;
	shared_lLargestSliceDatabaseAllValuesMemory = 0;
	lTotalSliceMaxBlockWorkingMemory = 0;
	lTotalSliceDatabaseAllValuesMemory = 0;
	lMaxSliceMaxBlockWorkingMemory = 0;
	lMaxSliceDatabaseAllValuesMemory = 0;
	for (nSlice = 0; nSlice < masterDataTableSliceSet->GetSliceNumber(); nSlice++)
	{
		slice = masterDataTableSliceSet->GetSliceAt(nSlice);

		// Evaluation de la memoire necessaire pour le traitement de la tranche
		// On ne prend pas en compte les valeurs des variables denses Symbol, etant donne qu'une petite marge a
		// deja ete reservee
		lSliceMaxBlockWorkingMemory =
		    ComputeEstimatedMaxBlockWorkingMemory(learningSpec, targetTupleTable, learningSpec->GetClass(),
							  slice->GetClass(), slice->GetClass()->GetAttributeNumber());
		lSliceDatabaseAllValuesMemory = ComputeEstimatedDatabaseAllValuesMemory(
		    learningSpec, targetTupleTable, learningSpec->GetClass(), slice->GetClass());

		// Mise a jour des statistiques
		shared_nLargestSliceAttributeNumber =
		    max((int)shared_nLargestSliceAttributeNumber, slice->GetClass()->GetAttributeNumber());
		shared_lLargestSliceUsedMemory = max((longint)shared_lLargestSliceUsedMemory, slice->GetUsedMemory());
		lTotalSliceMaxBlockWorkingMemory += lSliceMaxBlockWorkingMemory;
		lTotalSliceDatabaseAllValuesMemory += lSliceDatabaseAllValuesMemory;
		lMaxSliceMaxBlockWorkingMemory = max(lMaxSliceMaxBlockWorkingMemory, lSliceMaxBlockWorkingMemory);
		lMaxSliceDatabaseAllValuesMemory = max(lMaxSliceDatabaseAllValuesMemory, lSliceDatabaseAllValuesMemory);
	}

	// En ce qui concerne les evaluation de memoire pour les valeurs, on cherche a eviter  que le dimensionnement
	// initial a l'origine du slicer soit invalide par le resultat affine dans les tranches On choisi donc un
	// dimensionnement min de la tache base sur au moins la moitie de la moyenne moyenne des estimation par tranche
	lMeanSliceMaxBlockWorkingMemory =
	    lTotalSliceMaxBlockWorkingMemory / max(1, masterDataTableSliceSet->GetSliceNumber());
	lMeanSliceDatabaseAllValuesMemory =
	    lTotalSliceDatabaseAllValuesMemory / max(1, masterDataTableSliceSet->GetSliceNumber());
	if (nTotalAttributeNumber > 2 * masterDataTableSliceSet->GetSliceNumber())
	{
		shared_lLargestSliceMaxBlockWorkingMemory = lMeanSliceMaxBlockWorkingMemory / 2;
		shared_lLargestSliceDatabaseAllValuesMemory = lMeanSliceDatabaseAllValuesMemory / 2;
	}
	else
	{
		shared_lLargestSliceMaxBlockWorkingMemory = lMeanSliceMaxBlockWorkingMemory;
		shared_lLargestSliceDatabaseAllValuesMemory = lMeanSliceDatabaseAllValuesMemory;
	}

	// Pour le maitre: stockage de tous les resultats d'analyse
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMin(lNecessaryUnivariateStatsMemory *
									       nTotalAttributeNumber);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(
	    2 * GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMin());

	// Pour l'esclave:
	//   . memoire de definition de la tranche (kwClass)
	//   . memoire de travail globale (kwObjects, preparation des donnees...)
	//   . stockage des valeurs des objets lu depuis une tranche de base
	//   . memoire de travail pour creer toutes les table de tuples du plus gros bloc
	//   . stockage des resultats d'analyse des attributs de la	 tranche
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(
	    shared_lLargestSliceUsedMemory + lNecessaryWorkingMemory + shared_lLargestSliceDatabaseAllValuesMemory +
	    shared_lLargestSliceMaxBlockWorkingMemory +
	    lNecessaryUnivariateStatsMemory * shared_nLargestSliceAttributeNumber);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(
	    2 * (GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->GetMin() +
		 lMaxSliceMaxBlockWorkingMemory + lMaxSliceDatabaseAllValuesMemory));

	// En partage: stockage des variables partagees
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->Set(lNecessaryTargetAttributMemory);

	// Nombre max de processus
	GetResourceRequirements()->SetMaxSlaveProcessNumber(masterDataTableSliceSet->GetSliceNumber());

	// Affichage des stats
	if (bDisplayMemoryStats)
	{
		cout << "KWDataPreparationUnivariateTask::ComputeResourceRequirements" << endl;
		cout << "\tObject number\t" << nDatabaseObjectNumber << endl;
		cout << "\tAttribute number\t" << nTotalAttributeNumber << endl;
		if (targetTupleTable->GetAttributeNumber() > 0)
			cout << "\tTarget attribute type\t" << KWType::ToString(targetTupleTable->GetAttributeTypeAt(0))
			     << endl;
		cout << "\tTarget value number\t" << targetTupleTable->GetSize() << endl;
		cout << "\tNecessary target attribute memory\t"
		     << LongintToHumanReadableString(lNecessaryTargetAttributMemory) << endl;
		cout << "\tNecessary working memory\t" << LongintToHumanReadableString(lNecessaryWorkingMemory) << endl;
		cout << "\tNecessary univariate stats memory\t"
		     << LongintToHumanReadableString(lNecessaryUnivariateStatsMemory) << endl;
		cout << "\tSliceSet slice number\t" << masterDataTableSliceSet->GetSliceNumber() << endl;
		cout << "\tSliceSet chunk number\t" << masterDataTableSliceSet->GetChunkNumber() << endl;
		cout << "\tLargest slice attribute number\t" << shared_nLargestSliceAttributeNumber << endl;
		cout << "\tLargest slice used memory\t" << LongintToHumanReadableString(shared_lLargestSliceUsedMemory)
		     << endl;
		cout << "\tLargest slice all values memory\t"
		     << LongintToHumanReadableString(shared_lLargestSliceDatabaseAllValuesMemory) << endl;
		cout << "\tLargest slice max block working memory\t"
		     << LongintToHumanReadableString(shared_lLargestSliceMaxBlockWorkingMemory) << endl;
		cout << "\tMean slice all values memory\t"
		     << LongintToHumanReadableString(lMeanSliceDatabaseAllValuesMemory) << endl;
		cout << "\tMean slice max block working memory\t"
		     << LongintToHumanReadableString(lMeanSliceMaxBlockWorkingMemory) << endl;
		cout << "\tMax slice all values memory\t"
		     << LongintToHumanReadableString(lMaxSliceDatabaseAllValuesMemory) << endl;
		cout << "\tMax slice max block working memory\t"
		     << LongintToHumanReadableString(lMaxSliceMaxBlockWorkingMemory) << endl;
		cout << endl;
	}
	return bOk;
}

boolean KWDataPreparationUnivariateTask::MasterInitialize()
{
	boolean bOk;
	int nSlice;
	KWDataTableSlice* slice;

	// Appel de la methode ancetre
	bOk = KWDataPreparationTask::MasterInitialize();

	// Initialisation des variables de travail
	slMasterSortedSlices = new SortedList(KWDataTableSliceCompareLexicographicSortCriterion);
	odMasterSlices = new ObjectDictionary;
	if (bOk)
	{
		// Initialisation da la liste des tranches triees selon leur critere de tri lexicographique
		// et memorisation dans un dictionnaire
		for (nSlice = 0; nSlice < masterDataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			slice = masterDataTableSliceSet->GetSliceAt(nSlice);

			// On insert dans la liste triee par complexite croissante
			InitializeSliceLexicographicSortCriterion(slice);
			slMasterSortedSlices->Add(slice);

			// On memorise le dictionnaire de toutes les tranches ou sous-tranches en ecours
			odMasterSlices->SetAt(slice->GetClass()->GetName(), slice);
		}
	}

	return bOk;
}

boolean KWDataPreparationUnivariateTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk = true;
	boolean bDisplay = false;
	KWDataTableSlice* slice;
	const double dHeuristicRatio = 0.8;
	int nRemainingAttributes;
	double dRemainingAttributePercent;
	double dEstimatedCorrection;

	// Recherche de la prochaine tranche a traiter
	dTaskPercent = 0;
	bIsTaskFinished = (nMasterTotalAttributeNumber == odMasterDatabaseAttributeStats->GetCount());
	slice = NULL;
	if (not bIsTaskFinished)
	{
		// Cas ou il reste des tranches a traiter
		if (slMasterSortedSlices->GetCount() > 0)
		{
			// Recherche de la prochaine tranche a gerer, la derniere selon l'ordre des tranche par
			// complexite croissante
			slice = cast(KWDataTableSlice*, slMasterSortedSlices->RemoveTail());
			assert(slice != NULL);
			assert(odMasterSlices->Lookup(slice->GetClass()->GetName()) == slice);

			// On verifie que le premier attribut ne la tranche n'a pas deja ete traite
			assert(odMasterDatabaseAttributeStats->Lookup(
				   slice->GetClass()->GetLoadedAttributeAt(0)->GetName()) == NULL);

			// La gestion du pourcentage est non triviale, car quand on demande la preparation d'une
			// proportion des attributs, le pourcentage correspondant peut etre consomme si les attributs
			// sont effectivement prepares, mais peut etre nul si l'esclave a du redecoupe la trache a
			// traite en sous-tranche en raison du manque de ressources Une heuristique d'estimation du
			// pourcentage est donc exploitee, en partant du pourcentage d'attributs restant traiyte
			nRemainingAttributes = nMasterTotalAttributeNumber - odMasterDatabaseAttributeStats->GetCount();

			// Normalisation par le pourcentage restant non utilise
			dRemainingAttributePercent =
			    slice->GetClass()->GetAttributeNumber() / (double)nRemainingAttributes;

			// Correction par un facteur heuristique, pour tenir compte du fait que le SlaveProcess peut
			// redecouper la tranche sans effectuer la preparation
			dEstimatedCorrection = dHeuristicRatio + (1 - dHeuristicRatio) * dRemainingAttributePercent;

			// Pourcentage estime resultant
			dTaskPercent =
			    dRemainingAttributePercent * (1 - dMasterCumulatedPercentage) * dEstimatedCorrection;
			dMasterCumulatedPercentage += dTaskPercent;

			// On met une copie de la specification de la tranche
			// (la variable en entree a une duree temporaire et il faut garder
			// la specification initiale de l'ensemble des tranches)
			input_DataTableSlice.SetDataTableSlice(slice->Clone());
		}
		// S'il ne reste plus de tranche a traiter, on on indique a l'esclave de ne rien faire
		// et de se mettre en sommeil. Il sera peut-etre utile a nouveau si les dernier SlaveProcess
		// n'ont pas pu traiter les attributs et ont du redecouper la tranche en sous-tranche, creant ainsi
		// du travail a nouveau
		else
		{
			dTaskPercent = 0;
			SetSlaveAtRest();
		}
	}

	// Affichage des caracteristiques de la tranche a traiter
	if (bDisplay)
	{
		// Entete
		if (GetTaskIndex() == 0 and slice != NULL)
		{
			cout << "TaskIndex\t";
			slice->DisplayLexicographicSortCriterionHeaderLineReport(
			    cout, GetSliceLexicographicSortCriterionLabels());
		}

		// Ligne courante
		cout << GetTaskIndex() << "\t";
		if (slice != NULL)
			slice->DisplayLexicographicSortCriterionLineReport(cout);
		else
			cout << "\n";
	}
	return bOk;
}

boolean KWDataPreparationUnivariateTask::MasterAggregateResults()
{
	boolean bOk = true;
	boolean bDisplay = false;
	int nAttribute;
	KWAttributeStats* attributeStats;
	int nSubSlice;
	KWDataTableSlice* slice;
	KWDataTableSlice* subSlice;

	require(output_oaSliceAttributeStats->GetObjectArray()->GetSize() == 0 or
		output_oaDataTableSubSlices->GetObjectArray()->GetSize() == 0);
	require(output_sSliceClassName.GetValue() != "");
	require(output_oaSliceAttributeStats->GetObjectArray()->GetSize() > 0 or
		output_oaDataTableSubSlices->GetObjectArray()->GetSize() > 0);

	// Affichage
	if (bDisplay)
	{
		cout << "KWDataPreparationUnivariateTask::MasterAggregateResults \t" << GetTaskIndex() << "\t";
		cout << "Output slice\t" << output_sSliceClassName << "\t";
		cout << "Attribute stats\t" << output_oaSliceAttributeStats->GetObjectArray()->GetSize() << "\t";
		cout << "Sub slices\t" << output_oaDataTableSubSlices->GetObjectArray()->GetSize() << "\t";
		cout << "Resting slaves\t" << GetRestingSlaveNumber() << endl;
	}

	// Cas ou la preparation a ete effectuee par l'esclave
	if (output_oaSliceAttributeStats->GetObjectArray()->GetSize() > 0)
	{
		// Transfer des preparations d'attribut de l'esclave vers le dictionnaire global du maitre
		for (nAttribute = 0; nAttribute < output_oaSliceAttributeStats->GetObjectArray()->GetSize();
		     nAttribute++)
		{
			attributeStats =
			    cast(KWAttributeStats*, output_oaSliceAttributeStats->GetObjectArray()->GetAt(nAttribute));

			// On reparametre le learningSpec par celui du maitre
			attributeStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());

			// Memorisation dans le dictionnaire global
			odMasterDatabaseAttributeStats->SetAt(attributeStats->GetAttributeName(), attributeStats);
		}
		output_oaSliceAttributeStats->GetObjectArray()->RemoveAll();
	}
	// Cas ou l'esclave a redecoupe la tranche en sous tranches
	else if (output_oaDataTableSubSlices->GetObjectArray()->GetSize() > 0)
	{
		assert(output_oaDataTableSubSlices->GetObjectArray()->GetSize() > 1);

		// Recherche de la tranche traitee en entree
		slice = cast(KWDataTableSlice*, odMasterSlices->Lookup(output_sSliceClassName.GetValue()));
		assert(slice != NULL);
		assert(slice->GetClass()->GetName() == output_sSliceClassName.GetValue());

		// Destruction de la tranche dans le dictionnaire
		odMasterSlices->RemoveKey(output_sSliceClassName.GetValue());
		delete slice;

		// On prend en compte les nouvelles tranches a traiter dans les variable de travail
		for (nSubSlice = 0; nSubSlice < output_oaDataTableSubSlices->GetObjectArray()->GetSize(); nSubSlice++)
		{
			subSlice =
			    cast(KWDataTableSlice*, output_oaDataTableSubSlices->GetObjectArray()->GetAt(nSubSlice));

			// Insertion dans le dictionnaire des tranches, en remplacement de la tranche decoupee
			odMasterSlices->SetAt(subSlice->GetClass()->GetName(), subSlice);

			// Insertion dans la liste des tranches triees par critere lexicographique
			InitializeSliceLexicographicSortCriterion(subSlice);
			slMasterSortedSlices->Add(subSlice);
		}
		output_oaDataTableSubSlices->GetObjectArray()->RemoveAll();

		// Il faut reveiller les eventuels esclaves en sommeil, car il reste des tranches a traiter
		SetAllSlavesAtWork();
	}
	return bOk;
}

boolean KWDataPreparationUnivariateTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	int nSlice;
	KWDataTableSlice* slice;

	// Nettoyage des resultats en cas d'erreur
	if (not bProcessEndedCorrectly or TaskProgression::IsInterruptionRequested())
		odMasterDatabaseAttributeStats->DeleteAll();
	assert(not bProcessEndedCorrectly or odMasterDatabaseAttributeStats->GetCount() <=
						 masterClass->GetUsedAttributeNumberForType(KWType::Symbol) +
						     masterClass->GetUsedAttributeNumberForType(KWType::Continuous) -
						     shared_TargetTupleTable.GetTupleTable()->GetAttributeNumber());

	// On remplace si necessaire les tranches initiales par les tranches ou sous-tranches creees en cours de
	// traitement
	if (masterDataTableSliceSet->GetSlices()->GetSize() < odMasterSlices->GetCount())
	{
		// Remplacement par les tranches du dictionnaire
		odMasterSlices->ExportObjectArray(masterDataTableSliceSet->GetSlices());

		// On retrie les tranches du domaine selon leur index lexicographique
		masterDataTableSliceSet->GetSlices()->SetCompareFunction(KWDataTableSliceCompareLexicographicIndex);
		masterDataTableSliceSet->GetSlices()->Sort();

		// Si le sliceset a change, il faut reindexer les classes de ses tranches, et verifier l'unicite de leur
		// nom dans le domaine de classe (car les tranche rajoutee l'on ete dans les esclaves, sans connaissance
		// du sliceset global Finalisation de la specification des classes de chaque tranche
		for (nSlice = 0; nSlice < masterDataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			slice = masterDataTableSliceSet->GetSliceAt(nSlice);

			// Finalisation de la classe
			if (KWClassDomain::GetCurrentDomain()->LookupClass(slice->GetClass()->GetName()) != NULL)
				slice->GetClass()->SetName(
				    KWClassDomain::GetCurrentDomain()->BuildClassName(slice->GetClass()->GetName()));
			slice->GetClass()->IndexClass();

			// On l'insere dans le domaine pour reserver le nom de la classe autant que possible
			KWClassDomain::GetCurrentDomain()->InsertClass(slice->GetClass());
		}

		// Supression des classes du domaine
		for (nSlice = 0; nSlice < masterDataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			slice = masterDataTableSliceSet->GetSliceAt(nSlice);

			// Supression de la classe de son domaine
			KWClassDomain::GetCurrentDomain()->RemoveClass(slice->GetClass()->GetName());
		}
	}

	// Nettoyage des variables de travail
	delete slMasterSortedSlices;
	delete odMasterSlices;

	// Appel de la methode ancetre
	bOk = KWDataPreparationTask::MasterFinalize(bProcessEndedCorrectly);
	return bOk;
}

boolean KWDataPreparationUnivariateTask::SlaveProcess()
{
	boolean bOk = true;
	boolean bForceManySubSlices = false;
	boolean bLocallyPrepareSubSlices = false;
	boolean bDisplayMemoryStats = GetPreparationTraceMode();
	int nDatabaseObjectNumber;
	KWDataTableSlice* slice;
	KWSTDatabaseTextFile sliceDatabase;
	longint lNecessaryUnivariateStatsMemory;
	longint lSliceUsedMemory;
	longint lSliceMaxBlockWorkingMemory;
	longint lSliceDatabaseAllValuesMemory;
	longint lSliceWorkingMemory;
	longint lAvailableWorkingMemory;
	longint lSplitAvailableMemory;
	boolean bSplitSliceRequired;
	int n;
	KWDataTableSlice* subSlice;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Preparation du contexte

	// Acces a la tranche a analyser
	slice = input_DataTableSlice.GetDataTableSlice();

	// Memorisation de la tranche en entree
	output_sSliceClassName.SetValue(slice->GetClass()->GetName());
	assert(output_sSliceClassName.GetValue() != "");

	// Preparation de classe de la tranche a analyser
	KWClassDomain::GetCurrentDomain()->InsertClass(slice->GetClass());
	assert(slice->GetClass()->Check());
	slice->GetClass()->Compile();

	// Initialisation d'une database associee a la tranche
	// (uniquement pour parametrer correctement le learningSpec)
	sliceDatabase.SetClassName(slice->GetClass()->GetName());
	sliceDatabase.SetDatabaseName(slice->GetDataFileNames()->GetAt(0));

	// On indique de ne pas verifier l'attribut cible, celui-ci etant absent de la classe dans l'esclave
	shared_learningSpec.GetLearningSpec()->SetCheckTargetAttribute(false);

	// Parametrage du learningSpec local a l'esclave
	shared_learningSpec.FinalizeSpecification(slice->GetClass(), &sliceDatabase);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = shared_learningSpec.GetLearningSpec()->GetInstanceNumber();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dimensionnement des ressources localement a l'esclave
	// En effet, compte tenu des estimations heuristique du taux de sparsite des blocs, on peut ici
	// etre amene a faire plusieurs passe de lecture pour le chgargement des donnees de la tranche,
	// et plusieurs passe de traitement pour l'analyse des blocs de grande taille comportant
	// de grandes nombres de valeurs

	// Memoire necessaire par resultat de preparation univariee
	lNecessaryUnivariateStatsMemory = ComputeNecessaryUnivariateStatsMemory(
	    shared_learningSpec.GetLearningSpec(), shared_TargetTupleTable.GetTupleTable());

	// Evaluation de la memoire necessaire pour le traitement de la tranche
	lSliceUsedMemory = slice->GetUsedMemory();
	lSliceMaxBlockWorkingMemory = ComputeBlockWorkingMemory(
	    slice->GetMaxBlockAttributeNumber(), slice->GetMaxAttributeBlockValueNumber(), nDatabaseObjectNumber);
	lSliceDatabaseAllValuesMemory = ComputeDatabaseAllValuesMemory(slice, nDatabaseObjectNumber);
	lSliceWorkingMemory = lSliceMaxBlockWorkingMemory + lSliceDatabaseAllValuesMemory;

	// Memoire disponible pour le chargement des valeurs de la tranche et les traitements des blocs en se basant sur
	// la memoire minimum demandee
	lAvailableWorkingMemory =
	    shared_lLargestSliceUsedMemory - lSliceUsedMemory +
	    +(shared_nLargestSliceAttributeNumber - (longint)slice->GetClass()->GetLoadedAttributeNumber()) *
		lNecessaryUnivariateStatsMemory +
	    shared_lLargestSliceMaxBlockWorkingMemory + shared_lLargestSliceDatabaseAllValuesMemory;

	// On rajoute la memoire supplementaire disponible
	lAvailableWorkingMemory +=
	    GetSlaveResourceGrant()->GetMemory() - GetSlaveResourceRequirement()->GetMemory()->GetMin();

	// Methode de debug pour forcer la creation de nombreuses sous-tranches
	if (bForceManySubSlices)
	{
		// On evite de forcer plus d'une fois, ce que l'on test en recherchant le mot "Split" dans les noms de
		// fichier
		if (KWDataTableSlice::GetSliceFilePrefix(slice->GetDataFileNames()->GetAt(0))
			.Find(KWDataTableSlice::GetSubSlicePrefix()) == -1)
		{
			lAvailableWorkingMemory = shared_learningSpec.GetLearningSpec()->GetInstanceNumber() *
						  sizeof(KWValue) *
						  int(ceil(sqrt(slice->GetClass()->GetLoadedDenseAttributeNumber())));
			if (slice->GetClass()->GetLoadedAttributeBlockNumber() > 0)
			{
				lAvailableWorkingMemory +=
				    shared_learningSpec.GetLearningSpec()->GetInstanceNumber() *
				    (GetNecessaryMemoryPerEmptyValueBlock() + GetNecessaryMemoryPerSparseValue());
				lAvailableWorkingMemory += ComputeBlockWorkingMemory(
				    slice->GetMaxBlockAttributeNumber(), slice->GetMaxAttributeBlockValueNumber() / 2,
				    nDatabaseObjectNumber);
			}
		}
	}

	// On determine s'il fait redecouper la tranche
	bSplitSliceRequired =
	    (lAvailableWorkingMemory < lSliceWorkingMemory and slice->GetClass()->GetLoadedAttributeNumber() > 1);

	// Affichage des stats memoire
	if (bDisplayMemoryStats)
	{
		cout << GetTaskLabel() << " Slave\t" << GetTaskIndex() << endl;
		cout << "Slice\t" << slice->GetClass()->GetName() << endl;
		cout << "\tAvailable working memory\t" << LongintToHumanReadableString(lAvailableWorkingMemory) << endl;
		cout << "\tSlave requirement min memory\t"
		     << LongintToHumanReadableString(GetSlaveResourceRequirement()->GetMemory()->GetMin()) << endl;
		cout << "\tSlave grant memory\t" << LongintToHumanReadableString(GetSlaveResourceGrant()->GetMemory())
		     << endl;
		cout << "\tSlice attribute number\t" << slice->GetClass()->GetAttributeNumber() << endl;
		cout << "\tSlice total file size\t" << LongintToReadableString(slice->GetTotalDataFileSize()) << endl;
		cout << "\tSlice total dense symbol attribute disk size\t"
		     << LongintToReadableString(slice->GetTotalDenseSymbolAttributeDiskSize()) << endl;
		cout << "\tSlice max value number per block\t"
		     << LongintToReadableString(slice->GetMaxAttributeBlockValueNumber()) << endl;
		cout << "\tSlice total value number in blocs\t"
		     << LongintToReadableString(slice->GetTotalAttributeBlockValueNumber()) << endl;
		cout << "\tSlice used memory\t" << LongintToHumanReadableString(lSliceUsedMemory) << endl;
		cout << "\tSlice max block working memory\t"
		     << LongintToHumanReadableString(lSliceMaxBlockWorkingMemory) << endl;
		cout << "\tSlice all values memory\t" << LongintToHumanReadableString(lSliceDatabaseAllValuesMemory)
		     << endl;
		cout << "\tSlice working memory\t" << LongintToHumanReadableString(lSliceWorkingMemory) << endl;
		cout << "\tLargest slice attribute number\t" << shared_nLargestSliceAttributeNumber << endl;
		cout << "\tLargest slice used memory\t" << LongintToHumanReadableString(shared_lLargestSliceUsedMemory)
		     << endl;
		cout << "\tLargest slice max block working memory\t"
		     << LongintToHumanReadableString(shared_lLargestSliceMaxBlockWorkingMemory) << endl;
		cout << "\tLargest slice all values memory\t"
		     << LongintToHumanReadableString(shared_lLargestSliceDatabaseAllValuesMemory) << endl;
		cout << "\tSplit slice required\t" << BooleanToString(bSplitSliceRequired) << endl;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Traitement des donnees de la tranche

	// Redecoupage de la tranche, qui ne peut etre traitee avec les ressources memoire disponibles
	if (bSplitSliceRequired)
	{
		// Determination de la memoire utilisable pour le split, en suposant que l'ensemble des sous-tranches
		// resultantes utilisera environ la meme memoire que la tranche initiale
		lSplitAvailableMemory =
		    max(lAvailableWorkingMemory - lSliceUsedMemory, (longint)BufferedFile::nDefaultBufferSize);

		// Decoupage de la tranche en sous-tranches
		bOk = SplitSlice(slice, shared_learningSpec.GetLearningSpec()->GetInstanceNumber(),
				 lAvailableWorkingMemory, lSplitAvailableMemory,
				 output_oaDataTableSubSlices->GetObjectArray());
		assert(bOk or output_oaDataTableSubSlices->GetObjectArray()->GetSize() == 0);

		// Exploitation des sous-tranches
		if (bOk)
		{
			assert(output_oaDataTableSubSlices->GetObjectArray()->GetSize() > 0);

			// Preparation des sous-tranches localement, pour une option qui n'est plus envisagee, mais qui
			// permet de faire des tests
			if (bLocallyPrepareSubSlices)
			{
				bOk = SlaveSliceArrayCollectPreparationStats(
				    slice, output_oaDataTableSubSlices->GetObjectArray(),
				    output_oaSliceAttributeStats->GetObjectArray());

				// Destruction des sous-tranches qui ont ete traitees
				for (n = 0; n < output_oaDataTableSubSlices->GetObjectArray()->GetSize(); n++)
				{
					subSlice = cast(KWDataTableSlice*,
							output_oaDataTableSubSlices->GetObjectArray()->GetAt(n));
					subSlice->DeleteSliceFiles();
				}
				output_oaDataTableSubSlices->GetObjectArray()->DeleteAll();
			}
			// Destruction des fichiers de la tranche principale, qui sera rempacee par les sous-tranches
			else
			{
				// On effectue ce nettoyage dans l'esclave plutot que dans le maitre, pour alleger le
				// travail de ce dernier
				slice->DeleteSliceFiles();
			}
		}
	}
	// Preparation des donnees de la tranche
	else
		bOk = SlaveSliceCollectPreparationStats(slice, output_oaSliceAttributeStats->GetObjectArray());

	// Nettoyage si erreur
	if (not bOk)
	{
		output_oaSliceAttributeStats->GetObjectArray()->DeleteAll();

		// Si une tranche a ete decoupee en sous-tranche avec des erreurs, le nettoyage est forcement effectue
		assert(output_oaDataTableSubSlices->GetObjectArray()->GetSize() == 0);
	}

	// Nettoyage
	KWClassDomain::GetCurrentDomain()->RemoveClass(slice->GetClass()->GetName());
	slice->GetClass()->SetAllAttributesLoaded(true);
	return bOk;
}

void KWDataPreparationUnivariateTask::InitializeSliceLexicographicSortCriterion(KWDataTableSlice* slice) const
{
	require(slice != NULL);
	require(slice->GetLexicographicSortCriterion()->GetSize() == 0);
	require(masterDataTableSliceSet != NULL);

	// Premier critere: nombre total de valeurs, dense plus sparse
	slice->GetLexicographicSortCriterion()->Add(double((longint)slice->GetClass()->GetLoadedDenseAttributeNumber() *
							       masterDataTableSliceSet->GetTotalInstanceNumber() +
							   slice->GetTotalAttributeBlockValueNumber()));

	// Deuxieme critere: nombre d'attributs
	slice->GetLexicographicSortCriterion()->Add(double(slice->GetClass()->GetLoadedAttributeNumber()));

	// Troisieme critere: taille sur disque
	slice->GetLexicographicSortCriterion()->Add(double(slice->GetTotalDataFileSize()));
}

const ALString KWDataPreparationUnivariateTask::GetSliceLexicographicSortCriterionLabels() const
{
	return "Values\tVariables\tFile size";
}

boolean KWDataPreparationUnivariateTask::SlaveSliceCollectPreparationStats(KWDataTableSlice* slice,
									   ObjectArray* oaOutputAttributeStats)
{
	boolean bOk = true;
	ObjectArray oaInputAttributes;

	require(IsInSlaveMethod());
	require(shared_learningSpec.GetLearningSpec()->Check());
	require(slice != NULL);
	require(oaOutputAttributeStats != NULL);
	require(oaOutputAttributeStats->GetSize() == 0);

	// Lecture des objets de la tranche
	bOk = slice->ReadAll();

	// Verification minimum de coherence
	if (bOk and shared_learningSpec.GetLearningSpec()->GetInstanceNumber() != slice->GetObjects()->GetSize())
	{
		bOk = false;
		AddError(
		    "Data table slice corrupted " + FileService::GetURIUserLabel(slice->GetDataFileNames()->GetAt(0)) +
		    "\n\t" + IntToString(slice->GetObjects()->GetSize()) + " read records for " +
		    IntToString(shared_learningSpec.GetLearningSpec()->GetInstanceNumber()) + " expected instances");
	}

	// Preparation des donnees sur les attributs de la classe et les objets de la tranche
	if (bOk)
	{
		// Collecte des attributs a analyser
		CollectInputAttributes(shared_learningSpec.GetLearningSpec(), slice->GetClass(), &oaInputAttributes);

		// Parametrage du chargeur de tuple
		slaveTupleTableLoader.SetInputClass(slice->GetClass());
		slaveTupleTableLoader.SetInputDatabaseObjects(slice->GetObjects());

		// Preparation des attributs  analyser
		bOk = BasicCollectPreparationStats(shared_learningSpec.GetLearningSpec(), &slaveTupleTableLoader,
						   &oaInputAttributes, true,
						   output_oaSliceAttributeStats->GetObjectArray());
	}

	// Destruction des objets
	slice->DeleteAll();
	return bOk;
}

boolean KWDataPreparationUnivariateTask::SlaveSliceArrayCollectPreparationStats(KWDataTableSlice* slice,
										ObjectArray* oaSubSlices,
										ObjectArray* oaOutputAttributeStats)
{
	boolean bOk = true;
	ObjectArray oaInputAttributes;
	KWDataTableSlice* subSlice;
	ObjectArray oaSubSliceAttributeStats;
	int n;

	require(IsInSlaveMethod());
	require(shared_learningSpec.GetLearningSpec()->Check());
	require(slice != NULL);
	require(oaSubSlices != NULL);
	require(oaOutputAttributeStats != NULL);
	require(oaOutputAttributeStats->GetSize() == 0);

	// Preparation par sous-tranche
	for (n = 0; n < oaSubSlices->GetSize(); n++)
	{
		subSlice = cast(KWDataTableSlice*, oaSubSlices->GetAt(n));

		// Compilation de la classe de la sous-tranche
		KWClassDomain::GetCurrentDomain()->InsertClass(subSlice->GetClass());
		assert(subSlice->GetClass()->Check());
		subSlice->GetClass()->Compile();

		// Pararemetrage temporaire du learningSpec
		shared_learningSpec.GetLearningSpec()->GetDatabase()->SetClassName(subSlice->GetClass()->GetName());
		shared_learningSpec.GetLearningSpec()->GetDatabase()->SetDatabaseName(
		    subSlice->GetDataFileNames()->GetAt(0));
		shared_learningSpec.FinalizeSpecification(subSlice->GetClass(),
							  shared_learningSpec.GetLearningSpec()->GetDatabase());

		// Lecture des objets de la tranche
		bOk = subSlice->ReadAll();

		// Verification minimum de coherence
		if (bOk and
		    shared_learningSpec.GetLearningSpec()->GetInstanceNumber() != subSlice->GetObjects()->GetSize())
		{
			bOk = false;
			AddError("Data table slice corrupted " +
				 FileService::GetURIUserLabel(subSlice->GetDataFileNames()->GetAt(0)) + "\n\t" +
				 IntToString(subSlice->GetObjects()->GetSize()) + " read records for " +
				 IntToString(shared_learningSpec.GetLearningSpec()->GetInstanceNumber()) +
				 " expected instances");
		}

		// Preparation des donnees sur les attributs de la classe et les objets de la tranche
		if (bOk)
		{
			// Collecte des attributs a analyser
			CollectInputAttributes(shared_learningSpec.GetLearningSpec(), subSlice->GetClass(),
					       &oaInputAttributes);

			// Parametrage du chargeur de tuple
			slaveTupleTableLoader.SetInputClass(subSlice->GetClass());
			slaveTupleTableLoader.SetInputDatabaseObjects(subSlice->GetObjects());

			// Preparation des attributs  analyser
			bOk =
			    BasicCollectPreparationStats(shared_learningSpec.GetLearningSpec(), &slaveTupleTableLoader,
							 &oaInputAttributes, true, &oaSubSliceAttributeStats);
			oaOutputAttributeStats->InsertObjectArrayAt(
			    output_oaSliceAttributeStats->GetObjectArray()->GetSize(), &oaSubSliceAttributeStats);

			// Nettoyage pour la prochaine etape
			oaInputAttributes.SetSize(0);
			oaSubSliceAttributeStats.SetSize(0);
		}

		// Destruction des objets
		subSlice->DeleteAll();

		// Nettoyage
		KWClassDomain::GetCurrentDomain()->RemoveClass(subSlice->GetClass()->GetName());

		// Arret si erreur
		if (not bOk)
			break;
	}

	// On restitue le parametrage initial du learningSpec
	shared_learningSpec.GetLearningSpec()->GetDatabase()->SetClassName(slice->GetClass()->GetName());
	shared_learningSpec.GetLearningSpec()->GetDatabase()->SetDatabaseName(slice->GetDataFileNames()->GetAt(0));
	shared_learningSpec.FinalizeSpecification(slice->GetClass(),
						  shared_learningSpec.GetLearningSpec()->GetDatabase());
	return bOk;
}

boolean KWDataPreparationUnivariateTask::SplitSlice(KWDataTableSlice* slice, int nObjectNumber,
						    longint lAvailableWorkingMemory, longint lSplitAvailableMemory,
						    ObjectArray* oaSubSlices)
{
	boolean bOk = true;
	boolean bDisplay = false;
	boolean bCloseOk;
	KWDatabaseSlicerOutputBufferedFile allSliceOutputBuffer;
	longint lAllSliceOutputBufferSize;
	int nAllSliceOutputBufferSize;
	KWDataTableSliceSet dataTableSliceSet;
	IntVector ivUsedAttributePartIndexes;
	KWDataTableSlice* subSlice;
	KWAttribute* attribute;
	int nSubSliceNumber;
	int nSubSlice;
	int nChunk;
	LongintVector lvSliceAttributeBlockValueNumbers;
	LongintVector lvSliceDenseSymbolAttributeDiskSizes;
	int nDenseSymbolAttributeNumber;
	int nSliceBlockIndex;
	int nSubSliceBlockIndex;
	int nSliceDenseSymbolAttributeIndex;
	int nSubSliceDenseAttributeIndex;
	KWObject* kwoObject;
	longint lRecordNumber;
	ALString sSliceBaseName;
	ALString sSliceFileName;
	longint lSliceMaxBlockWorkingMemory;
	longint lSliceDatabaseAllValuesMemory;

	require(slice != NULL);
	require(slice->GetClass()->GetLoadedAttributeNumber() > 0);
	require(nObjectNumber >= 0);
	require(lAvailableWorkingMemory >= 0);
	require(lSplitAvailableMemory >= 0);
	require(oaSubSlices != NULL);
	require(oaSubSlices->GetSize() == 0);

	// Partitionnement des attributs de la tranche
	nSubSliceNumber =
	    ComputeSlicePartition(slice, nObjectNumber, lAvailableWorkingMemory, &ivUsedAttributePartIndexes);

	// Specification d'un sliceset  sur la base de cette partition
	dataTableSliceSet.BuildSpecificationFromClassPartition(slice->GetClass(), "", nObjectNumber, nSubSliceNumber,
							       slice->GetLexicographicIndex(),
							       &ivUsedAttributePartIndexes);
	assert(dataTableSliceSet.GetSliceNumber() == nSubSliceNumber);

	// Calcul des informations de chargement des attributs de chaque slice pour la classe a partitionner du sliceset
	dataTableSliceSet.ComputeSlicesLoadIndexes(slice->GetClass());

	// Calcul des tailles de buffer permises par ces memoire allouees
	lAllSliceOutputBufferSize = min(
	    longint(slice->GetClass()->GetAttributeNumber() * sizeof(KWValue) * nObjectNumber), lSplitAvailableMemory);
	lAllSliceOutputBufferSize = min(lGB, lAllSliceOutputBufferSize);
	nAllSliceOutputBufferSize = (int)lAllSliceOutputBufferSize;
	nAllSliceOutputBufferSize = max(BufferedFile::nDefaultBufferSize / 8, nAllSliceOutputBufferSize);

	// Creation et initialisation du buffer en sortie pour l'ensemble des sous-tranches
	allSliceOutputBuffer.SetDataTableSliceSet(&dataTableSliceSet);
	allSliceOutputBuffer.SetBufferSize(nAllSliceOutputBufferSize);

	// Initialisation du vecteur de noms de fichiers en sortie
	allSliceOutputBuffer.GetSliceFileNames()->SetSize(nSubSliceNumber);

	// Initialisation du vecteur de collecte des nombres de valeurs par bloc
	lvSliceAttributeBlockValueNumbers.SetSize(dataTableSliceSet.GetTotalAttributeBlockNumber());

	// Calcul du nombres d'attributs denses Symbol du sliceSet
	assert(slice->GetClass()->GetUsedAttributeNumber() == slice->GetClass()->GetLoadedAttributeNumber());
	nDenseSymbolAttributeNumber = slice->GetClass()->GetUsedDenseAttributeNumberForType(KWType::Symbol);

	// Initialisation du vecteur de collecte des taille fichier par attribut dense Symbol
	lvSliceDenseSymbolAttributeDiskSizes.SetSize(nDenseSymbolAttributeNumber);

	// Ouverture de la tranche en lecture
	if (bOk)
		bOk = slice->OpenForRead();

	// Decoupage de la tranche en sous-tranches
	if (bOk)
	{
		TaskProgression::DisplayMainLabel("Split slice into sub-slices");

		// Parcours des fichiers de chunk de la slice a decouper
		for (nChunk = 0; nChunk < slice->GetDataFileNames()->GetSize(); nChunk++)
		{
			// Creation des fichiers de tranche
			for (nSubSlice = 0; nSubSlice < allSliceOutputBuffer.GetSliceFileNames()->GetSize();
			     nSubSlice++)
			{
				subSlice = dataTableSliceSet.GetSliceAt(nSubSlice);

				// Creation d'un nom de chunk temporaire propre a l'esclave
				sSliceBaseName = KWDataTableSlice::BuildSubSliceFileName(
				    subSlice->GetLexicographicIndex(), nChunk,
				    KWDataTableSlice::GetSliceFileBaseName(slice->GetDataFileNames()->GetAt(nChunk)));
				sSliceFileName = FileService::CreateUniqueTmpFile(sSliceBaseName, this);
				bOk = sSliceFileName != "";

				// Memorisation du fichier
				if (bOk)
				{
					allSliceOutputBuffer.GetSliceFileNames()->SetAt(nSubSlice, sSliceFileName);
					SlaveRegisterUniqueTmpFile(sSliceFileName);
				}
				else
					break;
			}

			// Ouverture du buffer en sortie pour l'ensemble des tranches
			if (bOk)
				bOk = allSliceOutputBuffer.Open();

			// Parcours des fichiers de la tranche pour les dispatcher dans les sous-tranches
			if (bOk)
			{
				// On traite les objets de la tranche tant qu'on reste dans le meme chunk
				lRecordNumber = 0;
				while (not slice->IsEnd() and slice->GetChunkIndex() == nChunk)
				{
					kwoObject = slice->Read();
					lRecordNumber++;

					// Ajout de l'objet au tableau
					if (kwoObject != NULL)
					{
						// Ecriture de l'objet pour le dispatcher dans les fichiers de tranche
						bOk = allSliceOutputBuffer.WriteObject(
						    kwoObject, &lvSliceAttributeBlockValueNumbers,
						    &lvSliceDenseSymbolAttributeDiskSizes);

						// Destruction de l'objet traite
						delete kwoObject;

						// Test si erreur
						if (allSliceOutputBuffer.IsError())
						{
							bOk = false;
							break;
						}
					}
					// Arret sinon
					else
					{
						bOk = false;
						break;
					}

					// Suivi de la tache
					if (TaskProgression::IsRefreshNecessary())
						TaskProgression::DisplayProgression(
						    (int)(100 * slice->GetReadPercentage()));
				}

				// Test si interruption sans qu'il y ait d'erreur
				if (not bOk or slice->IsError() or TaskProgression::IsInterruptionRequested())
				{
					bOk = false;

					// Warning ou erreur selon le cas
					if (TaskProgression::IsInterruptionRequested())
						slice->AddWarning("Read data table slice interrupted by user");
					else if (allSliceOutputBuffer.IsError())
						slice->AddError("Error in writing database sub-slice files");
					else
						slice->AddError("Read data table slice interrupted because of errors");
				}
			}

			// Fermeture des fichiers en sortie, meme si erreur
			if (allSliceOutputBuffer.IsOpened())
			{
				bCloseOk = allSliceOutputBuffer.Close();
				bOk = bOk and bCloseOk;
				if (not bCloseOk)
					slice->AddError("Error in closing database sub-slice files");
			}

			// Recopie des caracteristiques des fichiers dans les sous-slices
			// On le fait meme en cas d'erreur pour pouvoir les detruire en cas d'erreur
			nSliceBlockIndex = 0;
			for (nSubSlice = 0; nSubSlice < dataTableSliceSet.GetSliceNumber(); nSubSlice++)
			{
				subSlice = dataTableSliceSet.GetSliceAt(nSubSlice);

				// Recopie du nom deu fichier et de sa taille
				if (allSliceOutputBuffer.GetSliceFileNames()->GetAt(nSubSlice) != "")
				{
					subSlice->GetDataFileNames()->Add(
					    allSliceOutputBuffer.GetSliceFileNames()->GetAt(nSubSlice));
					subSlice->GetDataFileSizes()->Add(FileService::GetFileSize(
					    allSliceOutputBuffer.GetSliceFileNames()->GetAt(nSubSlice)));
				}
			}
			allSliceOutputBuffer.GetSliceFileNames()->Initialize();

			// Arret si erreurs
			if (not bOk)
				break;
		}
	}

	// Fermeture de la tranche si elle est ouverte, meme si erreur
	if (slice->IsOpenedForRead())
		bOk = slice->Close() and bOk;

	// Si Ok, collecte des resultats
	if (bOk)
	{
		// Finalisation des sous-tranches
		// Attention, le vecteur des attributs dense symbol de la tranche ne concerne que que les attribut
		// dense, alors que dans chaque sous-tranche, on a un veceteur d'attribut dense, Symbol ou Continuous,
		// et seuls les index des attributs type Symbol sont concernes
		nSliceBlockIndex = 0;
		nSliceDenseSymbolAttributeIndex = 0;
		for (nSubSlice = 0; nSubSlice < dataTableSliceSet.GetSliceNumber(); nSubSlice++)
		{
			subSlice = dataTableSliceSet.GetSliceAt(nSubSlice);

			// On remplace les noms de fichier de tranches par des URI
			for (nChunk = 0; nChunk < subSlice->GetDataFileNames()->GetSize(); nChunk++)
				subSlice->GetDataFileNames()->SetAt(
				    nChunk, FileService::BuildLocalURI(subSlice->GetDataFileNames()->GetAt(nChunk)));

			// Parcours des blocs de chaque tranche pour collecter les nombres de valeurs par bloc
			subSlice->GetAttributeBlockValueNumbers()->SetSize(
			    subSlice->GetClass()->GetLoadedAttributeBlockNumber());
			for (nSubSliceBlockIndex = 0;
			     nSubSliceBlockIndex < subSlice->GetClass()->GetLoadedAttributeBlockNumber();
			     nSubSliceBlockIndex++)
			{
				// Memorisation du nombre de valeurs collecte pour le bloc courant
				subSlice->GetAttributeBlockValueNumbers()->SetAt(
				    nSubSliceBlockIndex, lvSliceAttributeBlockValueNumbers.GetAt(nSliceBlockIndex));
				nSliceBlockIndex++;
			}

			// Parcours des attributs desnse de chaque tranche pour collecter les tailles sur disque par
			// attribut dense Symbol
			subSlice->GetDenseSymbolAttributeDiskSizes()->SetSize(
			    subSlice->GetClass()->GetLoadedDenseAttributeNumber());
			for (nSubSliceDenseAttributeIndex = 0;
			     nSubSliceDenseAttributeIndex < subSlice->GetClass()->GetLoadedDenseAttributeNumber();
			     nSubSliceDenseAttributeIndex++)
			{
				attribute =
				    subSlice->GetClass()->GetLoadedDenseAttributeAt(nSubSliceDenseAttributeIndex);

				// Memorisation du nombre de la taille collectees collecte dans le cas des attributs
				// denses
				assert(attribute->GetType() == KWType::Symbol or
				       subSlice->GetDenseSymbolAttributeDiskSizes()->GetAt(
					   nSubSliceDenseAttributeIndex) == 0);
				if (attribute->GetType() == KWType::Symbol)
				{
					subSlice->GetDenseSymbolAttributeDiskSizes()->SetAt(
					    nSubSliceDenseAttributeIndex, lvSliceDenseSymbolAttributeDiskSizes.GetAt(
									      nSliceDenseSymbolAttributeIndex));
					nSliceDenseSymbolAttributeIndex++;
				}
			}
			assert(subSlice->Check());
		}
		assert(nSliceBlockIndex == lvSliceAttributeBlockValueNumbers.GetSize());
		assert(nSliceDenseSymbolAttributeIndex == lvSliceDenseSymbolAttributeDiskSizes.GetSize());

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Verifications de coherence entre la tranche et l'ensemble des sous-tranches correspondantes
		// Pour la taille des fichiers, il peut y avoir le cas ou des attributs symbol issu de regles de
		// derivation ont ete ecrits avec des blancs en debut ou fin dans la tranche originelle, ont ete relus
		// en les trimant, et recrit en prenant moins de place dans les sous-tranches. Pour les nombres de
		// valeurs des blocs, il peut de meme y avoir plus de valeurs dans la tranche originelle si certaines
		// valeurs, constituees de blancs uniquement, ne sont pas reecrites dans les sous-tranches
		//
		// Attention, le choix d'ecrire les champs categoriels sans les trimer est un choix de conception a ne
		// pas "corriger":
		//  . avantages:
		//    . possibilite d'utiliser les regles de derivation comme outil de data management, pour recoder
		//    vers n'importe quel format (avec ou sans blancs) . efficacite:
		//      . seul le parsing des fichiers doit etre trime (pas le resultat des regles de derivation,
		//      pontentiellement utilisees recursivement) . il n'est pas necessaire de trimer tout resultat de
		//      calcul aboutissant a une valeur categorielle (ou bloc de valeurs categorielles)
		//  . inconvenients:
		//    . difference de comportement selon qu'une donnee soit calculee ou stockee
		//      . mais ce cas est suppose tres rare, voire pathologique
		//      . necessite de traiter certains effets de bord, comme l'utilisation des valeurs pour fabriquer
		//      des noms de variable
		// Cf. jeu de test LearningTest\TestKhiops\SmallInstability\BugConstructionWithTrailingBlank

		// S'il ny a pas d'attributs Symbol, la taille cumulee des fichiers de slices finale doit etre au moins
		// egale a celle du fichier de slice initial, plus une fin de ligne - un separateur par ligne (objet) de
		// chaque fichier additionnel
		assert(slice->GetTotalDataFileSize() + (dataTableSliceSet.GetSliceNumber() - 1) * nObjectNumber *
							   (FileService::GetEOL().GetLength() - 1) <=
			   dataTableSliceSet.GetTotalDataFileSize() or
		       slice->GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) > 0);

		// Verification sur les valeur dans les blocs d'attributs
		assert(slice->GetTotalAttributeBlockValueNumber() ==
			   dataTableSliceSet.GetTotalAttributeBlockValueNumber() or
		       (slice->GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) > 0 and
			slice->GetTotalAttributeBlockValueNumber() >=
			    dataTableSliceSet.GetTotalAttributeBlockValueNumber()));

		// Verifications sur le nombre total d'attributs, dense et dans les blocs
		assert(slice->GetClass()->GetAttributeNumber() == dataTableSliceSet.GetTotalAttributeNumber());
		assert(slice->GetClass()->GetLoadedDenseAttributeNumber() ==
		       dataTableSliceSet.GetTotalDenseAttributeNumber());
		assert(slice->GetClass()->GetLoadedAttributeBlockNumber() <=
		       dataTableSliceSet.GetTotalAttributeBlockNumber());

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Transfert du contenu du sliceset de travail vers le tableau de slice resultat
		oaSubSlices->CopyFrom(dataTableSliceSet.GetSlices());
		dataTableSliceSet.GetSlices()->RemoveAll();
	}
	// Sinon, on detruit tous les fichiers crees
	else
	{
		// On ne fait pas appel a la methode DeleteAllFiles du sliceset, pour profiter de ce que les fichier
		// sont en local
		for (nSubSlice = 0; nSubSlice < dataTableSliceSet.GetSliceNumber(); nSubSlice++)
		{
			subSlice = dataTableSliceSet.GetSliceAt(nSubSlice);
			for (nChunk = 0; nChunk < subSlice->GetDataFileNames()->GetSize(); nChunk++)
			{
				assert(FileService::GetURIScheme(subSlice->GetDataFileNames()->GetAt(nChunk)) == "");
				FileService::RemoveFile(subSlice->GetDataFileNames()->GetAt(nChunk));
			}
		}

		// Destruction directe des objets tranches pour ne pas relancer une destruction des fichiers
		dataTableSliceSet.GetSlices()->DeleteAll();
	}

	// Nettoyage des informations de chargement des attributs de chaque slice pour la classe a partitionner du
	// sliceSet
	dataTableSliceSet.CleanSlicesLoadIndexes();

	// Affichage des resultats
	if (bDisplay)
	{
		// Statistiques globales sur la tranche initiale
		cout << "SplitSlice\t" << nObjectNumber << "\t" << lAvailableWorkingMemory << "\t"
		     << lSplitAvailableMemory << "\n";
		lSliceMaxBlockWorkingMemory = ComputeBlockWorkingMemory(
		    slice->GetMaxBlockAttributeNumber(), slice->GetMaxAttributeBlockValueNumber(), nObjectNumber);
		lSliceDatabaseAllValuesMemory = ComputeDatabaseAllValuesMemory(slice, nObjectNumber);
		cout << "\tName\tVars\tCat vars\tMem\tFile size\tDense cat disk size\tAll values mem\tMax block work "
			"mem\tOverall mem\n";
		cout << "\t" << slice->GetClass()->GetName() << "\t" << slice->GetClass()->GetAttributeNumber() << "\t"
		     << slice->GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) << "\t"
		     << slice->GetUsedMemory() << "\t" << slice->GetTotalDataFileSize() << "\t"
		     << slice->GetTotalDenseSymbolAttributeDiskSize() << "\t" << lSliceDatabaseAllValuesMemory << "\t"
		     << lSliceMaxBlockWorkingMemory << "\t"
		     << lSliceDatabaseAllValuesMemory + lSliceMaxBlockWorkingMemory << "\n";

		// Affichage de statistiques par sous-tranches
		nSliceBlockIndex = 0;
		nSliceDenseSymbolAttributeIndex = 0;
		for (nSubSlice = 0; nSubSlice < oaSubSlices->GetSize(); nSubSlice++)
		{
			subSlice = cast(KWDataTableSlice*, oaSubSlices->GetAt(nSubSlice));
			lSliceMaxBlockWorkingMemory =
			    ComputeBlockWorkingMemory(subSlice->GetMaxBlockAttributeNumber(),
						      subSlice->GetMaxAttributeBlockValueNumber(), nObjectNumber);
			lSliceDatabaseAllValuesMemory = ComputeDatabaseAllValuesMemory(subSlice, nObjectNumber);
			cout << "\t" << subSlice->GetClass()->GetName() << "\t"
			     << subSlice->GetClass()->GetAttributeNumber() << "\t"
			     << subSlice->GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) << "\t"
			     << subSlice->GetUsedMemory() << "\t" << subSlice->GetTotalDataFileSize() << "\t"
			     << subSlice->GetTotalDenseSymbolAttributeDiskSize() << "\t"
			     << lSliceDatabaseAllValuesMemory << "\t" << lSliceMaxBlockWorkingMemory << "\t"
			     << lSliceDatabaseAllValuesMemory + lSliceMaxBlockWorkingMemory << "\n";
		}
		cout << "\n";
	}
	return bOk;
}

int KWDataPreparationUnivariateTask::ComputeSlicePartition(KWDataTableSlice* slice, int nObjectNumber,
							   longint lAvailableWorkingMemory,
							   IntVector* ivUsedAttributePartIndexes)
{
	boolean bDisplay = false;
	longint lSliceDatabaseAllValuesNecessaryMemory;
	int nPartIndex;
	KWAttribute* attribute;
	int nAttributeIndex;
	int nDenseAttributeIndex;
	int nBlockIndex;
	longint lSliceMaxBlockWorkingMemory;
	longint lSliceDatabaseAllValuesMemory;
	KWAttributeBlock* lastDataAttributeBlock;
	KWDataItemMemoryFootprint* dataItemMemoryFootPrint;
	ObjectArray oaDataItemMemoryFootPrints;
	longint lEstimatedAttributeSymbolValuesMemory;
	longint lMeanNeccesaryMemoryPerAttribute;
	int nMaxAttributeNumber;
	int nAttributeSubsetNumber;
	int n;
	int i;

	require(slice != NULL);
	require(slice->GetClass()->GetUsedAttributeNumber() == slice->GetClass()->GetLoadedAttributeNumber());
	require(slice->GetClass()->GetLoadedAttributeNumber() > 0);
	require(nObjectNumber >= 0);
	require(lAvailableWorkingMemory >= 0);
	require(ivUsedAttributePartIndexes != NULL);

	// Initialisation des resultats
	ivUsedAttributePartIndexes->SetSize(slice->GetClass()->GetAttributeNumber());
	for (i = 0; i < ivUsedAttributePartIndexes->GetSize(); i++)
		ivUsedAttributePartIndexes->SetAt(i, -1);

	// Creation d'un tableau de statistiques memoires par dataItems utilise
	nAttributeIndex = 0;
	nDenseAttributeIndex = 0;
	nBlockIndex = 0;
	lastDataAttributeBlock = NULL;
	dataItemMemoryFootPrint = NULL;
	for (nAttributeIndex = 0; nAttributeIndex < slice->GetClass()->GetUsedAttributeNumber(); nAttributeIndex++)
	{
		attribute = slice->GetClass()->GetUsedAttributeAt(nAttributeIndex);

		// Cas d'un attribut dense
		if (not attribute->IsInBlock())
		{
			assert(lAvailableWorkingMemory >= GetNecessaryMemoryPerDenseValue() * nObjectNumber);

			// Creation et initialisation
			dataItemMemoryFootPrint = new KWDataItemMemoryFootprint;
			oaDataItemMemoryFootPrints.Add(dataItemMemoryFootPrint);
			dataItemMemoryFootPrint->SetDataItem(attribute);
			dataItemMemoryFootPrint->GetUsedAttributeIndexes()->Add(nAttributeIndex);

			// Calcul de la memoire additionnele utilise en cas d'un attribut Symbol
			lEstimatedAttributeSymbolValuesMemory = 0;
			if (attribute->GetType() == KWType::Symbol)
				lEstimatedAttributeSymbolValuesMemory = ComputeEstimatedAttributeSymbolValuesMemory(
				    slice->GetDenseSymbolAttributeDiskSizes()->GetAt(nDenseAttributeIndex),
				    nObjectNumber);

			// Mise a jour des statistiques memoire, en prenant en compte l'overead dans le cas d'un
			// attribut Symbol
			dataItemMemoryFootPrint->SetDataItemNecessaryMemory(
			    GetNecessaryMemoryPerDenseValue() * nObjectNumber + lEstimatedAttributeSymbolValuesMemory);

			// Incrementation de l'index des attributs denses
			nDenseAttributeIndex++;
		}
		// Cas d'un attribut sparse
		else
		{
			// Ajout uniquement si le dataItem n'a pas deja ete traite
			if (dataItemMemoryFootPrint == NULL or
			    dataItemMemoryFootPrint->GetDataItem() != attribute->GetAttributeBlock())
			{
				assert(lAvailableWorkingMemory >=
				       (GetNecessaryMemoryPerEmptyValueBlock() + GetNecessaryMemoryPerSparseValue()) *
					   nObjectNumber);

				// Creation et initialisation
				dataItemMemoryFootPrint = new KWDataItemMemoryFootprint;
				oaDataItemMemoryFootPrints.Add(dataItemMemoryFootPrint);
				dataItemMemoryFootPrint->SetDataItem(attribute->GetAttributeBlock());

				// Mise a jour des statistiques memoire pour le stockage des valeurs
				dataItemMemoryFootPrint->SetDataItemNecessaryMemory(
				    GetNecessaryMemoryPerEmptyValueBlock() * nObjectNumber);
				dataItemMemoryFootPrint->SetAllSparseValuesNecessaryMemory(
				    GetNecessaryMemoryPerSparseValue() *
				    slice->GetAttributeBlockValueNumbers()->GetAt(nBlockIndex));

				// Prise en compte egalement de la memoire de travail necessaire pour le traitement du
				// bloc
				dataItemMemoryFootPrint->SetBlockWorkingMemory(ComputeBlockWorkingMemory(
				    attribute->GetAttributeBlock()->GetLoadedAttributeNumber(),
				    slice->GetAttributeBlockValueNumbers()->GetAt(nBlockIndex), nObjectNumber));
			}

			// Ajout d'un attribut prise en compte dans le bloc
			assert(dataItemMemoryFootPrint->GetDataItem() == attribute->GetAttributeBlock());
			dataItemMemoryFootPrint->GetUsedAttributeIndexes()->Add(nAttributeIndex);

			// Incrementation de l'index de bloc
			if (lastDataAttributeBlock == NULL or lastDataAttributeBlock != attribute->GetAttributeBlock())
			{
				nBlockIndex++;
				lastDataAttributeBlock = attribute->GetAttributeBlock();
			}
		}
	}

	// Tri par empreinte memoire croissante
	oaDataItemMemoryFootPrints.SetCompareFunction(KWDataItemMemoryFootprintCompare);
	oaDataItemMemoryFootPrints.Sort();

	// Parcours des data items pour parametrer la partie de leurs attributs, selon les contraintes memoire
	// On note ici que les dataItem sont traites du plus petit au plus gros, si bien que les premiers traites
	// pourront en majorite etre integree dans une tranche, et que les derniers seront decoupes en autant
	// de tranches que necessaire
	// Cet algorithme est loin d'etre optimal, mais on souhaite pas ici minimiser le nombre de tranches:
	// on veut uniquement decouper assez les tranches pour respecter les contraintes memoires
	nPartIndex = 0;
	lSliceMaxBlockWorkingMemory = 0;
	lSliceDatabaseAllValuesMemory = 0;
	for (n = 0; n < oaDataItemMemoryFootPrints.GetSize(); n++)
	{
		dataItemMemoryFootPrint = cast(KWDataItemMemoryFootprint*, oaDataItemMemoryFootPrints.GetAt(n));

		// On ajoute les dataItem en entier s'il peuvent tenir dans une seule tranche
		// On traite egalement le cas d'un dataItem reduit a un seul attribut, que l'on ne pourra pas decouper
		// de toute facon
		if (dataItemMemoryFootPrint->GetDatabaseAllValuesMemory() +
			    dataItemMemoryFootPrint->GetBlockWorkingMemory() <=
			lAvailableWorkingMemory or
		    dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize() == 1)
		{
			// Creation d'une nouvelle tranche si necessaire
			if (lSliceDatabaseAllValuesMemory > 0 and
			    lSliceDatabaseAllValuesMemory + dataItemMemoryFootPrint->GetDatabaseAllValuesMemory() +
				    max(lSliceMaxBlockWorkingMemory, dataItemMemoryFootPrint->GetBlockWorkingMemory()) >
				lAvailableWorkingMemory)
			{
				nPartIndex++;
				lSliceMaxBlockWorkingMemory = 0;
				lSliceDatabaseAllValuesMemory = 0;
			}

			// Cumul de la memoire utilisee
			lSliceDatabaseAllValuesMemory += dataItemMemoryFootPrint->GetDatabaseAllValuesMemory();
			lSliceMaxBlockWorkingMemory =
			    max(lSliceMaxBlockWorkingMemory, dataItemMemoryFootPrint->GetBlockWorkingMemory());

			// Prise en compte de tous les attributs du data item
			for (i = 0; i < dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize(); i++)
			{
				// Memorisation de la partie
				nAttributeIndex = dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetAt(i);
				ivUsedAttributePartIndexes->SetAt(nAttributeIndex, nPartIndex);
			}
		}
		// Sinon, on decoupe le data item pour le traiter en plusieurs passes
		else
		{
			// Creation d'une nouvelle tranche si necessaire
			if (lSliceDatabaseAllValuesMemory > 0)
			{
				nPartIndex++;
				lSliceMaxBlockWorkingMemory = 0;
				lSliceDatabaseAllValuesMemory = 0;
			}

			// Determination du nombre max d'attributs pouvant tenir en memoire
			lMeanNeccesaryMemoryPerAttribute =
			    (longint)ceil(dataItemMemoryFootPrint->GetAllSparseValuesNecessaryMemory() /
					  (double)dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize());
			lMeanNeccesaryMemoryPerAttribute +=
			    (longint)ceil(dataItemMemoryFootPrint->GetBlockWorkingMemory() /
					  (double)dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize());
			nMaxAttributeNumber =
			    int((lAvailableWorkingMemory - dataItemMemoryFootPrint->GetDataItemNecessaryMemory()) /
				lMeanNeccesaryMemoryPerAttribute);
			nMaxAttributeNumber = max(nMaxAttributeNumber, 1);

			// Repartition des attributs en autant de tranches
			nAttributeSubsetNumber =
			    (int)ceil(dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize() /
				      (double)nMaxAttributeNumber);
			assert(nAttributeSubsetNumber >= 2);
			for (i = 0; i < dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize(); i++)
			{
				// Memorisation de la partie en repartissant dans autant de tranches de lecture que
				// necessaire Les attributs sont repartis de facon contigue dans les tranches
				nAttributeIndex = dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetAt(i);
				ivUsedAttributePartIndexes->SetAt(
				    nAttributeIndex,
				    nPartIndex + (i * nAttributeSubsetNumber) /
						     dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize());
			}

			// Calcul du dernier index utilise
			nPartIndex += nAttributeSubsetNumber - 1;

			// On force la creation d'un prochaine tranche pour la prochaine fois
			lSliceDatabaseAllValuesMemory = lAvailableWorkingMemory + 1;
		}
	}

	// Affichage
	if (bDisplay)
	{
		// En tete
		cout << "ComputeSlicePartition\t" << slice->GetObjectLabel() << "\n";
		cout << "Available working memory\t" << lAvailableWorkingMemory << "\n";
		cout << "Part number\t" << nPartIndex + 1 << "\n";

		// Parcours des data items
		cout << "Data items\n";
		lSliceDatabaseAllValuesNecessaryMemory = 0;
		for (n = 0; n < oaDataItemMemoryFootPrints.GetSize(); n++)
		{
			dataItemMemoryFootPrint = cast(KWDataItemMemoryFootprint*, oaDataItemMemoryFootPrints.GetAt(n));

			// Affichage des statistiques memoire du data item
			cout << "\t" << *dataItemMemoryFootPrint << "\n";
			lSliceDatabaseAllValuesNecessaryMemory += dataItemMemoryFootPrint->GetDatabaseAllValuesMemory();

			// Affichages des attributs du data item et de leur index de tranche
			for (i = 0; i < dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetSize(); i++)
			{
				nAttributeIndex = dataItemMemoryFootPrint->GetUsedAttributeIndexes()->GetAt(i);
				cout << "\t" << ivUsedAttributePartIndexes->GetAt(nAttributeIndex) << "\t"
				     << slice->GetClass()->GetUsedAttributeAt(nAttributeIndex)->GetName() << "\n";
			}
		}
		cout << "Total\t" << LongintToHumanReadableString(lSliceDatabaseAllValuesNecessaryMemory) << endl;
	}

	// Nettoyage
	oaDataItemMemoryFootPrints.DeleteAll();

	// On retourne le nombre de parties, soit le dernier index plus un
	return nPartIndex + 1;
}

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDataItemMemoryFootprint

KWDataItemMemoryFootprint::KWDataItemMemoryFootprint()
{
	dataItem = NULL;
	lDataItemNecessaryMemory = 0;
	lAllSparseValuesNecessaryMemory = 0;
	lBlockWorkingMemory = 0;
}

KWDataItemMemoryFootprint::~KWDataItemMemoryFootprint() {}

void KWDataItemMemoryFootprint::SetDataItem(KWDataItem* dataItemValue)
{
	ivUsedAttributeIndexes.SetSize(0);
	dataItem = dataItemValue;
}

KWDataItem* KWDataItemMemoryFootprint::GetDataItem() const
{
	return dataItem;
}

IntVector* KWDataItemMemoryFootprint::GetUsedAttributeIndexes()
{
	return &ivUsedAttributeIndexes;
}

void KWDataItemMemoryFootprint::SetDataItemNecessaryMemory(longint lValue)
{
	require(lValue >= 0);
	lDataItemNecessaryMemory = lValue;
}

longint KWDataItemMemoryFootprint::GetDataItemNecessaryMemory() const
{
	return lDataItemNecessaryMemory;
}

void KWDataItemMemoryFootprint::SetAllSparseValuesNecessaryMemory(longint lValue)
{
	require(lValue >= 0);
	lAllSparseValuesNecessaryMemory = lValue;
}

longint KWDataItemMemoryFootprint::GetAllSparseValuesNecessaryMemory() const
{
	return lAllSparseValuesNecessaryMemory;
}

longint KWDataItemMemoryFootprint::GetDatabaseAllValuesMemory() const
{
	return lDataItemNecessaryMemory + lAllSparseValuesNecessaryMemory;
}

void KWDataItemMemoryFootprint::SetBlockWorkingMemory(longint lValue)
{
	require(lValue >= 0);
	lBlockWorkingMemory = lValue;
}

longint KWDataItemMemoryFootprint::GetBlockWorkingMemory() const
{
	return lBlockWorkingMemory;
}

void KWDataItemMemoryFootprint::Write(ostream& ost) const
{
	if (dataItem == NULL)
		ost << "\t";
	else
		ost << dataItem->GetName() << "\t";
	ost << ivUsedAttributeIndexes.GetSize() << "\t";
	ost << lDataItemNecessaryMemory << "\t";
	ost << lAllSparseValuesNecessaryMemory;
}

int KWDataItemMemoryFootprintCompare(const void* elem1, const void* elem2)
{
	KWDataItemMemoryFootprint* dataItemMemoryFootprint1;
	KWDataItemMemoryFootprint* dataItemMemoryFootprint2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	dataItemMemoryFootprint1 = cast(KWDataItemMemoryFootprint*, *(Object**)elem1);
	dataItemMemoryFootprint2 = cast(KWDataItemMemoryFootprint*, *(Object**)elem2);

	// Comparaison sur l'empreinte memoire totale
	nDiff = CompareLongint(dataItemMemoryFootprint1->GetDatabaseAllValuesMemory(),
			       dataItemMemoryFootprint2->GetDatabaseAllValuesMemory());

	// En cas d'egalite, comparaison sur le nom
	if (nDiff == 0)
		nDiff = dataItemMemoryFootprint1->GetDataItem()->GetName().Compare(
		    dataItemMemoryFootprint2->GetDataItem()->GetName());
	return nDiff;
}
