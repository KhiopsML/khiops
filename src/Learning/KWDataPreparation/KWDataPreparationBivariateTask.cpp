// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPreparationBivariateTask.h"

KWDataPreparationBivariateTask::KWDataPreparationBivariateTask()
{
	// Variables partagees
	shared_odAttributeStats = new PLShared_ObjectDictionary(new PLShared_AttributeStats);
	DeclareSharedParameter(shared_odAttributeStats);
	DeclareSharedParameter(&shared_ivDataTableSliceSetChunkInstanceNumbers);

	// Variables en entree et sortie des esclaves
	DeclareTaskInput(&input_svAttributePairNames1);
	DeclareTaskInput(&input_svAttributePairNames2);
	DeclareTaskInput(&input_svAttributeNames);
	input_oaDataTableSlices = new PLShared_ObjectArray(new PLShared_DataTableSlice);
	DeclareTaskInput(input_oaDataTableSlices);
	output_oaAttributPairStats = new PLShared_ObjectArray(new PLShared_AttributePairStats);
	DeclareTaskOutput(output_oaAttributPairStats);

	// Initialisation des variables du maitre
	masterClassStats = NULL;
	oaMasterInputAttributeStats = NULL;
	oaMasterOutputAttributePairStats = NULL;
}

KWDataPreparationBivariateTask::~KWDataPreparationBivariateTask()
{
	delete shared_odAttributeStats;
	delete input_oaDataTableSlices;
	delete output_oaAttributPairStats;
}

boolean KWDataPreparationBivariateTask::CollectPreparationPairStats(KWLearningSpec* learningSpec,
								    KWClassStats* classStats,
								    KWTupleTableLoader* tupleTableLoader,
								    KWDataTableSliceSet* dataTableSliceSet,
								    ObjectArray* oaAttributePairStats)
{
	boolean bOk;

	require(learningSpec != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(classStats->GetLearningSpec() == learningSpec);
	require(masterClassStats == NULL);
	require(oaMasterInputAttributeStats == NULL);
	require(oaMasterOutputAttributePairStats == NULL);
	require(shared_odAttributeStats->GetObjectDictionary()->GetCount() == 0);

	// Memorisation des statistiques univariee en entree
	masterClassStats = classStats;
	oaMasterInputAttributeStats = classStats->GetAttributeStats();

	// Parametrage de la collecte des resultats par le maitre
	oaMasterOutputAttributePairStats = oaAttributePairStats;

	// Lancement de la tache de preparation
	bOk = RunDataPreparationTask(learningSpec, tupleTableLoader, dataTableSliceSet);
	if (not bOk)
		oaAttributePairStats->DeleteAll();

	// Nettoyage des donnees de pilotage de la tache
	CleanTaskInputs();

	// Nettoyage des variables du maitre
	masterClassStats = NULL;
	oaMasterInputAttributeStats = NULL;
	oaMasterOutputAttributePairStats = NULL;
	return bOk;
}

const ALString KWDataPreparationBivariateTask::GetTaskName() const
{
	return "Bivariate data preparation";
}

PLParallelTask* KWDataPreparationBivariateTask::Create() const
{
	return new KWDataPreparationBivariateTask;
}

boolean KWDataPreparationBivariateTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	boolean bDisplayRequirements = false;
	KWLearningSpec* learningSpec;
	const KWTupleTable* targetTupleTable;
	int nDatabaseObjectNumber;
	KWAttributePairsSlices* attributePairsSlices;
	KWDataTableSlice* slice;
	int nSlice;
	int i;
	longint lLargestSliceUsedMemory;
	int nLargestSliceAttributePairNumber;
	longint lLargestSliceMaxBlockWorkingMemory;
	longint lLargestSliceDatabaseAllValuesMemory;
	longint lSliceMaxBlockWorkingMemory;
	longint lSliceDatabaseAllValuesMemory;
	int nLargestSliceNumber;
	int nRequestedAttributePairNumber;
	longint lNecessaryAllAttributesStatsMemory;
	longint lNecessaryWorkingMemory;
	longint lNecessaryTargetAttributMemory;
	longint lNecessaryBivariateStatsMemory;

	require(shared_odAttributeStats->GetObjectDictionary()->GetCount() == 0);
	require(oaInputAttributePairStats.GetSize() == 0);
	require(CheckInputParameters(shared_learningSpec.GetLearningSpec(), shared_TargetTupleTable.GetTupleTable()));

	// Calcul des donnees de pilotage de la tache
	// Ces calcul sont bases sur des initialisations provenant de la classe ancetre et ne peuvent etre effectues
	// qu'une fois la tache lancee, donc dans cette methode
	ComputeTaskInputs();

	//////////////////////////////////////////////////////////////////////////
	// Estimation des ressources necessaires pour la preparation des donnees

	// Estimation des ressources pour les statistiques univariees partagees, pour tous les attributs impliques dans
	// les paires
	lNecessaryAllAttributesStatsMemory = shared_odAttributeStats->GetObjectDictionary()->GetOverallUsedMemory();

	// Calcul de la memoire de travail et de la memoire de stockage par resultat
	learningSpec = shared_learningSpec.GetLearningSpec();
	targetTupleTable = shared_TargetTupleTable.GetTupleTable();
	lNecessaryWorkingMemory = ComputeNecessaryWorkingMemory(learningSpec, targetTupleTable, true);
	lNecessaryTargetAttributMemory = ComputeNecessaryTargetAttributeMemory(learningSpec, targetTupleTable);
	lNecessaryBivariateStatsMemory = ComputeNecessaryBivariateStatsMemory(learningSpec, targetTupleTable);

	// Nombre d'objets de la base
	nDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Nombre de paires d'attributs a analyser effectivement
	nRequestedAttributePairNumber = oaInputAttributePairStats.GetSize();

	// Statistiques sur la plus grande paire de tranche
	//   . le nombre d'attributs, sparse ou non: pour le dimensionnement des resultats de preparation
	//   . la memoire utilise par la tranche: principalement, celle pour la definition de la kwClass
	//   . la memoire de travail pour la gestion des blocs: pour charger simultanement toutes les tables de tuples
	//   du plus gros bloc de la tranche . la memoire de travail pour le stockage de toutes les valeurs de la
	//   tranche
	// Contrairement au cas univarie, ces statistiques ne sont pas basees sur des estimations heuristiques
	// (notamment pour le taux de remplissage des blocs sparse).
	// Elles sont ici basees sur des estimations fines exploitant les informations collectees dans chaque tranche
	// lors des etapes precedentes (taux de sparsite des blocs, tailles des valeurs denses Symbol)
	lLargestSliceUsedMemory = 0;
	nLargestSliceAttributePairNumber = 0;
	lLargestSliceMaxBlockWorkingMemory = 0;
	lLargestSliceDatabaseAllValuesMemory = 0;
	nLargestSliceNumber = 0;
	for (i = 0; i < oaInputAttributePairsSlices.GetSize(); i++)
	{
		attributePairsSlices = cast(KWAttributePairsSlices*, oaInputAttributePairsSlices.GetAt(i));

		// Evaluation de la memoire necessaire pour le traitement des tranches impliquees
		lSliceMaxBlockWorkingMemory = 0;
		lSliceDatabaseAllValuesMemory = 0;
		for (nSlice = 0; nSlice < attributePairsSlices->GetSlices()->GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, attributePairsSlices->GetSlices()->GetAt(nSlice));
			lSliceMaxBlockWorkingMemory = max(
			    lSliceMaxBlockWorkingMemory,
			    ComputeBlockWorkingMemory(slice->GetMaxBlockAttributeNumber(),
						      slice->GetMaxAttributeBlockValueNumber(), nDatabaseObjectNumber));
			lSliceDatabaseAllValuesMemory += ComputeDatabaseAllValuesMemory(slice, nDatabaseObjectNumber);
		}

		// Mise a jour des statistiques
		lLargestSliceUsedMemory =
		    max(lLargestSliceUsedMemory, attributePairsSlices->GetSlices()->GetOverallUsedMemory());
		lLargestSliceMaxBlockWorkingMemory =
		    max(lLargestSliceMaxBlockWorkingMemory, lSliceMaxBlockWorkingMemory);
		lLargestSliceDatabaseAllValuesMemory =
		    max(lLargestSliceDatabaseAllValuesMemory, lSliceDatabaseAllValuesMemory);
		nLargestSliceAttributePairNumber =
		    max(nLargestSliceAttributePairNumber, attributePairsSlices->GetAttributePairStats()->GetSize());
		nLargestSliceNumber = max(nLargestSliceNumber, attributePairsSlices->GetSlices()->GetSize());
	}
	assert(nLargestSliceNumber <= 2);

	// Pour le maitre: stockage de tous les resultats d'analyse
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMin(lNecessaryBivariateStatsMemory *
									       nRequestedAttributePairNumber);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(
	    2 * GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMin());

	// Pour l'esclave:
	//   . memoire de definition de la tranche
	//   . memoire de travail globale (preparation des donnees...)
	//   . stockage des valeurs des objets lu depuis une tranche de base
	//   . memoire de travail pour creer toutes les table de tuples du plus gros bloc
	//   . stockage des resultats d'analyse bivariee des paires d'attributs de la tranche
	//   . buffers de lecture des slices (une fois les slices lus, la memoire est a nouveau disponible pour les
	//   resultats)
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(
	    lLargestSliceUsedMemory + lNecessaryWorkingMemory + lLargestSliceDatabaseAllValuesMemory +
	    lLargestSliceMaxBlockWorkingMemory +
	    max(lNecessaryBivariateStatsMemory * nLargestSliceAttributePairNumber,
		(longint)nLargestSliceNumber * BufferedFile::nDefaultBufferSize));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(
	    2 * GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->GetMin());

	// En partage:
	//   . stockage des variables partagees
	//   . stockage des resultats d'analyse univariee de tous les attributs impliques, necessaire pour les analyses
	//   univariees
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->Set(lNecessaryTargetAttributMemory +
									    lNecessaryAllAttributesStatsMemory);

	// Nombre max de processus
	GetResourceRequirements()->SetMaxSlaveProcessNumber(oaInputAttributePairsSlices.GetSize());

	// Affichage detaille des demandes de ressource
	if (bDisplayRequirements)
	{
		cout << "KWDataPreparationBivariateTask::ComputeResourceRequirements" << endl;
		cout << "\tAttributsPairStatsNumber\t" << nRequestedAttributePairNumber << endl;
		cout << "\tAttributsStatsNumber\t" << shared_odAttributeStats->GetObjectDictionary()->GetCount()
		     << endl;
		cout << "\tNecessarAllyAttributsStatsMemory\t"
		     << LongintToHumanReadableString(lNecessaryAllAttributesStatsMemory) << endl;
		cout << "\tNecessaryWorkingMemory\t" << LongintToHumanReadableString(lNecessaryWorkingMemory) << endl;
		cout << "\tNecessaryTargetAttributMemory\t"
		     << LongintToHumanReadableString(lNecessaryTargetAttributMemory) << endl;
		cout << "\tNecessaryBivariateStatsMemory\t"
		     << LongintToHumanReadableString(lNecessaryBivariateStatsMemory) << endl;
		cout << "\tLargestSliceAttributePairNumber\t" << nLargestSliceAttributePairNumber << endl;
		cout << "\tLargest slice used memory\t" << LongintToHumanReadableString(lLargestSliceUsedMemory)
		     << endl;
		cout << "\tLargest slice max block working memory\t"
		     << LongintToHumanReadableString(lLargestSliceMaxBlockWorkingMemory) << endl;
		cout << "\tLargest slice all values memory\t"
		     << LongintToHumanReadableString(lLargestSliceDatabaseAllValuesMemory) << endl;
		cout << "\tLargest slice number\t" << IntToString(nLargestSliceNumber) << endl;
	}
	return bOk;
}

boolean KWDataPreparationBivariateTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk = true;
	boolean bDisplay = false;
	KWAttributePairsSlices* attributePairsSlices;
	KWAttributePairStats* attributePairStats;
	KWAttribute* attribute;
	KWDataTableSlice* slice;
	int i;

	// Recherche de la prochaine tranche de paires a analyser
	attributePairsSlices = NULL;
	dTaskPercent = 0;
	bIsTaskFinished = (GetTaskIndex() >= oaInputAttributePairsSlices.GetSize());
	if (not bIsTaskFinished)
	{
		// On prend les pairs de tranche en ordre inverse, pour avoir les plus grosses au debut
		attributePairsSlices =
		    cast(KWAttributePairsSlices*,
			 oaInputAttributePairsSlices.GetAt(oaInputAttributePairsSlices.GetSize() - 1 - GetTaskIndex()));
		dTaskPercent = attributePairsSlices->GetAttributePairStats()->GetSize() * 1.0 /
			       oaInputAttributePairStats.GetSize();

		/////////////////////////////////////////////////////////////////////////
		// Recopie dans les variables partagee en entree de tache des parametres
		// specifiees dans la tranche des paires a analyser

		// Noms des paires de variables
		for (i = 0; i < attributePairsSlices->GetAttributePairStats()->GetSize(); i++)
		{
			attributePairStats =
			    cast(KWAttributePairStats*, attributePairsSlices->GetAttributePairStats()->GetAt(i));
			input_svAttributePairNames1.Add(attributePairStats->GetAttributeName1());
			input_svAttributePairNames2.Add(attributePairStats->GetAttributeName2());
		}

		// Noms des variables impliquees
		for (i = 0; i < attributePairsSlices->GetAttributes()->GetSize(); i++)
		{
			attribute = cast(KWAttribute*, attributePairsSlices->GetAttributes()->GetAt(i));
			input_svAttributeNames.Add(attribute->GetName());
		}

		// Tranches elementaires impliques dans les paires
		// On met une copie de la specification de la tranche
		// (la variable en entree a une duree temporaire et il faut garder
		// la specification initiale de l'ensemble des tranches)
		for (i = 0; i < attributePairsSlices->GetSlices()->GetSize(); i++)
		{
			slice = cast(KWDataTableSlice*, attributePairsSlices->GetSlices()->GetAt(i));
			input_oaDataTableSlices->GetObjectArray()->Add(slice->Clone());
		}
	}

	// Affichage des caracteristiques de l'ensemble des paires a traiter
	if (bDisplay)
	{
		// Entete
		if (GetTaskIndex() == 0 and attributePairsSlices != NULL)
		{
			cout << "TaskIndex\t";
			attributePairsSlices->DisplayLexicographicSortCriterionHeaderLineReport(
			    cout, GetAttributePairsSlicesLexicographicSortCriterionLabels());
		}

		// Ligne courante
		cout << GetTaskIndex() << "\t";
		if (attributePairsSlices != NULL)
			attributePairsSlices->DisplayLexicographicSortCriterionLineReport(cout);
		else
			cout << "\n";
	}
	return bOk;
}

boolean KWDataPreparationBivariateTask::MasterAggregateResults()
{
	boolean bOk = true;
	int nPair;
	KWAttributePairStats* attributePairStats;

	// Transfer des preparations d'attribut de l'esclave vers le dictionnaire global du maitre
	for (nPair = 0; nPair < output_oaAttributPairStats->GetObjectArray()->GetSize(); nPair++)
	{
		attributePairStats =
		    cast(KWAttributePairStats*, output_oaAttributPairStats->GetObjectArray()->GetAt(nPair));

		// On reparametre le learningSpec par celui du maitre
		attributePairStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());

		// Idem pour la classStats
		attributePairStats->SetClassStats(masterClassStats);

		// Memorisation dans le dictionnaire global
		oaMasterOutputAttributePairStats->Add(attributePairStats);
	}
	output_oaAttributPairStats->GetObjectArray()->RemoveAll();
	return bOk;
}

boolean KWDataPreparationBivariateTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	require(masterClassStats != NULL);

	// Nettoyage des resultats en cas d'erreur
	if (not bProcessEndedCorrectly or TaskProgression::IsInterruptionRequested())
		oaMasterOutputAttributePairStats->DeleteAll();
	assert(not bProcessEndedCorrectly or
	       oaMasterOutputAttributePairStats->GetSize() == oaInputAttributePairStats.GetSize());

	// Appel de la methode ancetre
	bOk = KWDataPreparationTask::MasterFinalize(bProcessEndedCorrectly);

	// Finalisation des resultats
	if (bOk)
	{
		// Tri par nom, pour assurer la reproductibilite de l'ordre des resultats
		oaMasterOutputAttributePairStats->SetCompareFunction(KWLearningReportCompareSortName);
		oaMasterOutputAttributePairStats->Sort();
	}
	return bOk;
}

boolean KWDataPreparationBivariateTask::SlaveInitialize()
{
	boolean bOk;
	ObjectArray oaAttributsStats;
	KWAttributeStats* attributeStats;
	int i;

	// Appel de la methode ancetre
	bOk = KWDataPreparationTask::SlaveInitialize();

	// Parametrage des statistiques univariee par leur learningSpec partage, local a l'esclave
	// Dans le cas sequentiel, ce n'est pas un probleme: on ne fait que re-specifier la reference au learningSpec
	if (bOk)
	{
		shared_odAttributeStats->GetObjectDictionary()->ExportObjectArray(&oaAttributsStats);
		for (i = 0; i < oaAttributsStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaAttributsStats.GetAt(i));
			attributeStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());
		}
	}
	return bOk;
}

boolean KWDataPreparationBivariateTask::SlaveProcess()
{
	boolean bOk = true;
	boolean bDisplay = false;
	boolean bExportTupleTables = false;
	KWDataTableSliceSet dataTableSliceSet;
	KWClass* kwcSliceSetClass;
	KWDataTableSlice* slice;
	KWSTDatabaseTextFile sliceDatabase;
	ObjectArray oaAllObjects;
	KWAttribute* attribute;
	KWTupleTable bivariateTupleTable;
	KWClassStats classStats;
	ObjectArray oaUnivariateTupleTables;
	KWTupleTable* tupleTable;
	ObjectArray oaAttributePairs;
	KWAttributePair* firstAttributePair;
	KWAttributePair* attributePair;
	ObjectDictionary odInputAttributes;
	ObjectDictionary odBlockTupleTables;
	KWAttribute* firstBlockAttribute;
	int nFirst;
	int nLast;
	int i;
	KWAttributeStats* attributeStats;
	KWAttributePairStats* attributePairStats;
	ALString sTmp;

	// Affichage des inputs de la tache
	if (bDisplay)
	{
		cout << "SlaveProcess " << GetTaskIndex() << endl;
		cout << "  Variables pairs\t" << input_svAttributePairNames1.GetSize() << endl;
		for (i = 0; i < input_svAttributePairNames1.GetSize(); i++)
		{
			cout << "\t" << input_svAttributePairNames1.GetAt(i) << " x "
			     << input_svAttributePairNames2.GetAt(i) << endl;
		}
		cout << "  Variables\t" << input_svAttributeNames.GetSize() << endl;
		for (i = 0; i < input_svAttributeNames.GetSize(); i++)
		{
			cout << "\t" << input_svAttributeNames.GetAt(i) << endl;
		}
		cout << "  Slices\t" << input_oaDataTableSlices->GetObjectArray()->GetSize() << endl;
		for (i = 0; i < input_oaDataTableSlices->GetObjectArray()->GetSize(); i++)
		{
			slice = cast(KWDataTableSlice*, input_oaDataTableSlices->GetObjectArray()->GetAt(i));
			cout << "\t" << slice->GetObjectLabel() << endl;
		}
	}

	// Specification de l'ensemble des tranche a utiliser
	dataTableSliceSet.GetSlices()->CopyFrom(input_oaDataTableSlices->GetObjectArray());
	for (i = 0; i < dataTableSliceSet.GetSliceNumber(); i++)
		dataTableSliceSet.GetSliceAt(i)->GetClass()->IndexClass();

	// On reparametre les nombres d'instances par chunk
	dataTableSliceSet.GetChunkInstanceNumbers()->CopyFrom(
	    shared_ivDataTableSliceSetChunkInstanceNumbers.GetConstIntVector());
	dataTableSliceSet.UpdateTotalInstanceNumber();
	assert(dataTableSliceSet.Check());

	// Construction d'une classe specifique pour lire les attributs impliques dans les paires
	kwcSliceSetClass = dataTableSliceSet.BuildClassFromAttributeNames(
	    KWClassDomain::GetCurrentDomain()->BuildClassName(sTmp + "SlaveBivariate" +
							      IntToString(GetTaskIndex() + 1)),
	    input_svAttributeNames.GetConstStringVector());

	// Preparation de classe permettant l'analyse des paires
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcSliceSetClass);
	assert(kwcSliceSetClass->Check());
	kwcSliceSetClass->Compile();

	// Initialisation d'une database associee a la tranche
	// (uniquement pour parametrer correctement le learningSpec)
	sliceDatabase.SetClassName(kwcSliceSetClass->GetName());
	sliceDatabase.SetDatabaseName(dataTableSliceSet.GetSliceAt(0)->GetDataFileNames()->GetAt(0));

	// On indique de ne pas verifier l'attribut cible, celui-ci etant absent de la classe dans l'esclave
	shared_learningSpec.GetLearningSpec()->SetCheckTargetAttribute(false);

	// Parametrage du learningSpec local a l'esclave
	shared_learningSpec.FinalizeSpecification(kwcSliceSetClass, &sliceDatabase);

	// Parametrage d'un objet classStats, avec les attributs impliques dans les paires
	classStats.SetLearningSpec(shared_learningSpec.GetLearningSpec());
	for (i = 0; i < kwcSliceSetClass->GetUsedAttributeNumber(); i++)
	{
		attribute = kwcSliceSetClass->GetUsedAttributeAt(i);
		attributeStats = cast(KWAttributeStats*,
				      shared_odAttributeStats->GetObjectDictionary()->Lookup(attribute->GetName()));
		if (attributeStats != NULL)
			classStats.AddAttributeStats(attributeStats);
	}

	// Parametrage des buffers de lecture du slice set
	dataTableSliceSet.SetTotalBufferSize(dataTableSliceSet.GetSliceNumber() * BufferedFile::nDefaultBufferSize);

	// Lecture des objets avec tous les attributs impliques dans le bivarie
	bOk = dataTableSliceSet.ReadAllObjectsWithClass(kwcSliceSetClass, &oaAllObjects);

	// Verification minimum de coherence
	if (bOk and shared_learningSpec.GetLearningSpec()->GetInstanceNumber() != oaAllObjects.GetSize())
	{
		bOk = false;
		AddError(
		    "Data table slices corrupted " + FileService::GetURIUserLabel(sliceDatabase.GetDatabaseName()) +
		    "\n\t" + IntToString(oaAllObjects.GetSize()) + " read records for " +
		    IntToString(shared_learningSpec.GetLearningSpec()->GetInstanceNumber()) + " expected instances");
	}

	// Calcul des statistiques
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		// Parametrage du chargeur de tuple
		slaveTupleTableLoader.SetInputClass(kwcSliceSetClass);
		slaveTupleTableLoader.SetInputDatabaseObjects(&oaAllObjects);

		// Initialisation d'un tableau de KWAttributePair a partir des noms des attributs des paires
		InitializeAttributePairs(shared_learningSpec.GetLearningSpec()->GetClass(),
					 input_svAttributePairNames1.GetConstStringVector(),
					 input_svAttributePairNames2.GetConstStringVector(), &oaAttributePairs);

		// Tri des paires d'attributs, selon leur bloc
		SortAttributePairsByBlocks(&oaAttributePairs);

		// Parametrage de la taille du tableau de resultats
		output_oaAttributPairStats->GetObjectArray()->SetSize(oaAttributePairs.GetSize());

		// Message de debut de tache
		TaskProgression::DisplayMainLabel("Bivariate discretizations and value groupings");

		// Traitement des paires d'attributs par groupe ayant meme premier bloc
		// et meme attribut du second bloc
		nFirst = 0;
		while (nFirst < oaAttributePairs.GetSize())
		{
			// Recherche du prochain groupe de paires d'attributs a traiter
			nLast = SearchNextGroupOfAttributePairs(&oaAttributePairs, nFirst, &odInputAttributes);

			// Recherche de la premiere paire d'attribut du groupe
			firstAttributePair = cast(KWAttributePair*, oaAttributePairs.GetAt(nFirst));

			// Cas d'un groupe d'attributs sans bloc, ou d'un groupe singleton
			// On doit traiter les paires une a une
			if (odInputAttributes.GetCount() == 0 or (nLast - nFirst) == 1)
			{
				assert(firstAttributePair->GetFirstBlockAttribute()->GetAttributeBlock() == NULL or
				       (nLast - nFirst) == 1);

				// Message sur le groupe
				if (bDisplay)
					cout << "Pairs without block\t" << nLast - nFirst << "\t"
					     << firstAttributePair->GetFirstBlockAttribute()->GetName() << "\t"
					     << firstAttributePair->GetSecondBlockAttribute()->GetName() << "\t"
					     << endl;

				// Parcours des paires du groupe
				for (i = nFirst; i < nLast; i++)
				{
					attributePair = cast(KWAttributePair*, oaAttributePairs.GetAt(i));

					// Creation et initialisation d'un objet de stats pour la paire d'attributs
					attributePairStats = new KWAttributePairStats;
					attributePairStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());
					attributePairStats->SetClassStats(&classStats);
					attributePairStats->SetAttributeName1(
					    attributePair->GetAttribute1()->GetName());
					attributePairStats->SetAttributeName2(
					    attributePair->GetAttribute2()->GetName());
					output_oaAttributPairStats->GetObjectArray()->SetAt(attributePair->GetIndex(),
											    attributePairStats);

					// Gestion de la progression
					TaskProgression::DisplayLabel(sTmp + attributePairStats->GetAttributeName1() +
								      " x " + attributePairStats->GetAttributeName2());
					TaskProgression::DisplayProgression(((i + 1) * 100) /
									    oaAttributePairs.GetSize());

					// Arret si erreur ou interruption
					if (TaskProgression::IsInterruptionRequested())
						bOk = false;
					if (not bOk)
						break;

					// Chargement de la table de tuple pour l'attribut
					slaveTupleTableLoader.LoadBivariate(attributePairStats->GetAttributeName1(),
									    attributePairStats->GetAttributeName2(),
									    &bivariateTupleTable);

					// Export de la table de tuple, pour la mise au point
					if (bExportTupleTables)
					{
						bivariateTupleTable.SortByValues();
						bivariateTupleTable.WriteFile(
						    bivariateTupleTable.GetObjectLabel() + "_D" +
						    IntToString(attributePair->GetFirstBlock() != NULL) + ".txt");
						cout << bivariateTupleTable.GetObjectLabel() << endl;
						if (attributePair->GetFirstBlock() == NULL)
							cout << "\tD No block" << endl;
						else
							cout << "\tD Block "
							     << attributePair->GetFirstBlock()->GetName() << endl;
					}

					// Calcul des statistitique univariee a partir de la table de tuples
					bOk = attributePairStats->ComputeStats(&bivariateTupleTable);

					// Nettoyage
					attributePairStats->SetClassStats(NULL);
					bivariateTupleTable.CleanAll();
				}
			}
			// Cas d'un groupe d'attributs dans un bloc
			// On peut traiter toutes les paires sur la base de leur bloc sparse
			else
			{
				assert(firstAttributePair->GetFirstBlockAttribute()->GetAttributeBlock() != NULL);

				// Message sur le groupe
				if (bDisplay)
					cout << "Pairs with block\t" << nLast - nFirst << "\t"
					     << firstAttributePair->GetFirstBlock()->GetName() << "\t"
					     << firstAttributePair->GetSecondBlockAttribute()->GetName() << endl;

				// Calcul des tables de tuples pour toutes les paires du groupe
				// Chargement de la table de tuple pour toutes les paires d'attributs du groupe
				slaveTupleTableLoader.BlockLoadBivariate(
				    firstAttributePair->GetFirstBlock()->GetName(), &odInputAttributes,
				    firstAttributePair->GetSecondBlockAttribute()->GetName(), &odBlockTupleTables);

				// Parcours des attributs du bloc
				for (i = nFirst; i < nLast; i++)
				{
					attributePair = cast(KWAttributePair*, oaAttributePairs.GetAt(i));

					// Creation et initialisation d'un objet de stats pour la paire d'attributs
					attributePairStats = new KWAttributePairStats;
					attributePairStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());
					attributePairStats->SetClassStats(&classStats);
					attributePairStats->SetAttributeName1(
					    attributePair->GetAttribute1()->GetName());
					attributePairStats->SetAttributeName2(
					    attributePair->GetAttribute2()->GetName());
					output_oaAttributPairStats->GetObjectArray()->SetAt(attributePair->GetIndex(),
											    attributePairStats);

					// Gestion de la progression
					TaskProgression::DisplayLabel(sTmp + attributePairStats->GetAttributeName1() +
								      " x " + attributePairStats->GetAttributeName2());
					TaskProgression::DisplayProgression(((i + 1) * 100) /
									    oaAttributePairs.GetSize());

					// Arret si erreur ou interruption
					if (TaskProgression::IsInterruptionRequested())
						bOk = false;
					if (not bOk)
					{
						odBlockTupleTables.DeleteAll();
						break;
					}

					// Acces a l'attribut du premier bloc de la paire
					firstBlockAttribute = attributePair->GetFirstBlockAttribute();
					assert(odInputAttributes.Lookup(firstBlockAttribute->GetName()) != NULL);

					// Acces la table de tuple courante, pour l'attribut se trouvant dans le premier
					// bloc de la paire
					tupleTable = cast(KWTupleTable*,
							  odBlockTupleTables.Lookup(firstBlockAttribute->GetName()));
					assert(tupleTable != NULL);
					assert(tupleTable->GetAttributeNameAt(0) ==
					       attributePair->GetAttribute1()->GetName());
					assert(tupleTable->GetAttributeNameAt(1) ==
					       attributePair->GetAttribute2()->GetName());

					// Export de la table de tuples, pour la mise au point
					if (bExportTupleTables)
					{
						tupleTable->SortByValues();
						tupleTable->WriteFile(
						    tupleTable->GetObjectLabel() + "_S" +
						    IntToString(attributePair->GetFirstBlock() != NULL) + ".txt");
						cout << tupleTable->GetObjectLabel() << endl;
						if (attributePair->GetFirstBlock() == NULL)
							cout << "\tS No block" << endl;
						else
							cout << "\tS Block "
							     << attributePair->GetFirstBlock()->GetName() << endl;
					}

					// Calcul des statistitique univariee a partir de la table de tuples
					bOk = attributePairStats->ComputeStats(tupleTable);

					// On detruit la table de tuple et on la supprime du dictionnaire
					// qui pourrait etre detruit exhaustivement en cas d'interruption utilisateur
					odBlockTupleTables.RemoveKey(firstBlockAttribute->GetName());
					delete tupleTable;
				}
				assert(odBlockTupleTables.GetCount() == 0);
			}

			// Nettoyage
			odInputAttributes.RemoveAll();
			nFirst = nLast;
		}

		// Destruction des paires
		oaAttributePairs.DeleteAll();
	}

	// Destruction des objets
	oaAllObjects.DeleteAll();

	// Nettoyage
	classStats.RemoveAll();
	dataTableSliceSet.GetSlices()->RemoveAll();
	KWClassDomain::GetCurrentDomain()->DeleteClass(kwcSliceSetClass->GetName());

	// Nettoyage des sorties si erreur
	if (not bOk)
		output_oaAttributPairStats->GetObjectArray()->DeleteAll();
	return bOk;
}

void KWDataPreparationBivariateTask::ComputeTaskInputs()
{
	boolean bDisplay = false;
	int n;
	KWAttributePairStats* attributePairStats;
	KWAttributePairsSlices* attributePairsSlices;

	require(IsMasterProcess());
	require(IsRunning());
	require(oaInputAttributePairStats.GetSize() == 0);
	require(shared_odAttributeStats->GetObjectDictionary()->GetCount() == 0);

	// Memorisation des nombre d'instance par chunk du dataTableSliseSet
	shared_ivDataTableSliceSetChunkInstanceNumbers.GetIntVector()->CopyFrom(
	    masterDataTableSliceSet->GetChunkInstanceNumbers());

	// Selection des paires d'attributs a analyser
	GetAttributePairsSpec()->SelectAttributePairStats(masterClassStats->GetAttributeStats(),
							  &oaInputAttributePairStats);

	// Memorisation de toutes les stats univariees d'attributs impliquees dans les parires
	for (n = 0; n < oaInputAttributePairStats.GetSize(); n++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaInputAttributePairStats.GetAt(n));

		// Memorisation pour chaque attribut de la paire
		shared_odAttributeStats->GetObjectDictionary()->SetAt(
		    attributePairStats->GetAttributeName1(),
		    masterClassStats->LookupAttributeStats(attributePairStats->GetAttributeName1()));
		shared_odAttributeStats->GetObjectDictionary()->SetAt(
		    attributePairStats->GetAttributeName2(),
		    masterClassStats->LookupAttributeStats(attributePairStats->GetAttributeName2()));
	}

	// Un ensemble de tranches est cree pour chaque paire d'attributs
	InitializeSingletonAllAttributePairsSlices(&oaInputAttributePairStats, &oaInputAttributePairsSlices);

	// Les ensembles de tranches sont fusionnes, pour contenir chacun toutes les paires d'attributs la concernant
	MergeAttributePairsSlices(&oaInputAttributePairsSlices);

	// Specification du critere de tri lexicographique de chaque pair de tranche
	for (n = 0; n < oaInputAttributePairsSlices.GetSize(); n++)
	{
		attributePairsSlices = cast(KWAttributePairsSlices*, oaInputAttributePairsSlices.GetAt(n));
		InitializeAttributePairsSlicesLexicographicSortCriterion(attributePairsSlices);
	}

	// Tri par nombres de paires d'attributs decroissant
	oaInputAttributePairsSlices.SetCompareFunction(KWAttributePairsSlicesCompareLexicographicSortCriterion);
	oaInputAttributePairsSlices.Sort();

	// Ecriture des ensembles de tranches gerant les paires d'attributs
	if (bDisplay)
		WriteAttributePairsSlices(&oaInputAttributePairsSlices, cout);
}

void KWDataPreparationBivariateTask::CleanTaskInputs()
{
	shared_odAttributeStats->GetObjectDictionary()->RemoveAll();
	oaInputAttributePairStats.DeleteAll();
	oaInputAttributePairsSlices.DeleteAll();
}

void KWDataPreparationBivariateTask::InitializeAttributePairsSlicesLexicographicSortCriterion(
    KWAttributePairsSlices* attributePairsSlices) const
{
	int n;
	KWAttributePairStats* attributePairStats;
	int i;
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	KWDataTableSlice* slice;
	int nValueNumber;
	double dTotalAlgorithmicComplexity;
	double dTotalValueNumber;
	double dTotalFileSize;

	require(attributePairsSlices != NULL);
	require(attributePairsSlices->GetLexicographicSortCriterion()->GetSize() == 0);
	require(masterDataTableSliceSet != NULL);

	// Premier critere: on va evaluer les complexite algorithmique approximative du traitement de chaque paire
	// d'attributs, en exploitant les statistiques descriptives des attributs
	dTotalAlgorithmicComplexity = 0;
	for (n = 0; n < attributePairsSlices->GetAttributePairStats()->GetSize(); n++)
	{
		attributePairStats =
		    cast(KWAttributePairStats*, attributePairsSlices->GetAttributePairStats()->GetAt(n));

		// Acces a chaque attribut de la paire
		for (i = 0; i < attributePairStats->GetAttributeNumber(); i++)
		{
			// Stats de l'attribut correspondant
			attributeStats = cast(KWAttributeStats*, shared_odAttributeStats->GetObjectDictionary()->Lookup(
								     attributePairStats->GetAttributeNameAt(i)));

			// Estimation de la complexite algorithmique
			nValueNumber = attributeStats->GetDescriptiveStats()->GetValueNumber();
			if (attributeStats->GetAttributeType() == KWType::Continuous)
				dTotalAlgorithmicComplexity += nValueNumber * log(1 + nValueNumber);
			else
				dTotalAlgorithmicComplexity +=
				    nValueNumber *
				    min(nValueNumber, (int)sqrt(masterDataTableSliceSet->GetTotalInstanceNumber())) *
				    log(1 + nValueNumber);
		}
	}
	attributePairsSlices->GetLexicographicSortCriterion()->Add(dTotalAlgorithmicComplexity);

	// Deuxieme critere: nombre total de valeurs distinctes des attributs impliques
	dTotalValueNumber = 0;
	for (n = 0; n < attributePairsSlices->GetAttributes()->GetSize(); n++)
	{
		attribute = cast(KWAttribute*, attributePairsSlices->GetAttributes()->GetAt(n));

		// Stats de l'attribut correspondant
		attributeStats = cast(KWAttributeStats*,
				      shared_odAttributeStats->GetObjectDictionary()->Lookup(attribute->GetName()));

		// Cumul du nombre total de valeurs
		nValueNumber = attributeStats->GetDescriptiveStats()->GetValueNumber();
		dTotalValueNumber += nValueNumber;
	}
	attributePairsSlices->GetLexicographicSortCriterion()->Add(dTotalValueNumber);

	// Troisieme critere: taille des fichiers des tranches
	dTotalFileSize = 0;
	for (n = 0; n < attributePairsSlices->GetSlices()->GetSize(); n++)
	{
		slice = cast(KWDataTableSlice*, attributePairsSlices->GetSlices()->GetAt(n));
		dTotalFileSize += slice->GetTotalDataFileSize();
	}
	attributePairsSlices->GetLexicographicSortCriterion()->Add(dTotalFileSize);
}

const ALString KWDataPreparationBivariateTask::GetAttributePairsSlicesLexicographicSortCriterionLabels() const
{
	return "Complexity\tValues\tFile size";
}

const KWAttributePairsSpec* KWDataPreparationBivariateTask::GetAttributePairsSpec() const
{
	require(masterClassStats != NULL);
	return masterClassStats->GetAttributePairsSpec();
}

void KWDataPreparationBivariateTask::InitializeSingletonAllAttributePairsSlices(
    const ObjectArray* oaAttributePairStats, ObjectArray* oaAttributePairsSlices) const
{
	int i;
	ObjectDictionary odSliceAttributes;
	KWAttributePairStats* attributePairStats;
	KWAttributePairsSlices* attributePairsSlices;
	KWDataTableSlice* slice1;
	KWDataTableSlice* slice2;
	KWAttribute* attribute1;
	KWAttribute* attribute2;

	require(oaAttributePairStats != NULL);
	require(oaAttributePairsSlices != NULL);
	require(oaAttributePairsSlices->GetSize() == 0);

	// Alimentation d'un dictionnaire, dont les cles sont les noms des attributs et les objet sont les tranches
	// (KWDataTableSlice) les contenant
	masterDataTableSliceSet->FillSliceAttributes(&odSliceAttributes);

	// Creation d'un objet KWAttributePairsSlices par paire d'attribut
	for (i = 0; i < oaAttributePairStats->GetSize(); i++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaAttributePairStats->GetAt(i));

		// Creation de l'objet
		attributePairsSlices = new KWAttributePairsSlices;
		oaAttributePairsSlices->Add(attributePairsSlices);

		// Recherche des tranches et des attributs correspondant a la paire
		slice1 = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attributePairStats->GetAttributeName1()));
		attribute1 = slice1->GetClass()->LookupAttribute(attributePairStats->GetAttributeName1());
		slice2 = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attributePairStats->GetAttributeName2()));
		attribute2 = slice2->GetClass()->LookupAttribute(attributePairStats->GetAttributeName2());
		assert(attribute1 != attribute2);

		// Mise a jour de l'objet
		attributePairsSlices->GetAttributePairStats()->Add(attributePairStats);
		attributePairsSlices->GetAttributes()->Add(attribute1);
		attributePairsSlices->GetAttributes()->Add(attribute2);

		// Ajout des tranches dans l'ordre
		if (slice1 == slice2)
			attributePairsSlices->GetSlices()->Add(slice1);
		else
		{
			assert(slice1->CompareLexicographicOrder(slice2) != 0);
			if (slice1->CompareLexicographicOrder(slice2) < 0)
			{
				attributePairsSlices->GetSlices()->Add(slice1);
				attributePairsSlices->GetSlices()->Add(slice2);
			}
			else
			{
				attributePairsSlices->GetSlices()->Add(slice2);
				attributePairsSlices->GetSlices()->Add(slice1);
			}
		}
		assert(attributePairsSlices->Check());
	}
}

void KWDataPreparationBivariateTask::MergeAttributePairsSlices(ObjectArray* oaAttributePairsSlices) const
{
	int i;
	int nSize;
	KWAttributePairsSlices* attributePairsSlices;
	KWAttributePairsSlices* currentAttributePairsSlices;

	// Tri selon les tranches associees a chaque paire
	oaAttributePairsSlices->SetCompareFunction(KWAttributePairsSlicesCompareSlices);
	oaAttributePairsSlices->Sort();

	// Parcours des paires du tableau
	nSize = 0;
	currentAttributePairsSlices = NULL;
	for (i = 0; i < oaAttributePairsSlices->GetSize(); i++)
	{
		attributePairsSlices = cast(KWAttributePairsSlices*, oaAttributePairsSlices->GetAt(i));

		// Memorisation de la premiere tranche
		if (currentAttributePairsSlices == NULL)
		{
			nSize++;
			currentAttributePairsSlices = attributePairsSlices;
		}
		// Comparaison avec la tranche courante
		else
		{
			// Fusion si meme tranches
			if (currentAttributePairsSlices->CompareSlices(attributePairsSlices) == 0)
			{
				// Ajout des attributs a la tranche courante
				currentAttributePairsSlices->AddAttributePairs(attributePairsSlices);

				// Destruction de la tranche traitee
				delete attributePairsSlices;
			}
			// Memorisation de la nouvelle tranche courante sinon
			else
			{
				currentAttributePairsSlices = attributePairsSlices;
				oaAttributePairsSlices->SetAt(nSize, currentAttributePairsSlices);
				nSize++;
			}
		}
	}

	// Retaillage du tableau
	oaAttributePairsSlices->SetSize(nSize);
}

void KWDataPreparationBivariateTask::WriteAttributePairsSlices(const ObjectArray* oaAttributePairsSlices,
							       ostream& ost) const
{
	int i;
	KWAttributePairsSlices attributePairsSlicesLabel;
	KWAttributePairsSlices* attributePairsSlices;

	// Creation d'un objet KWAttributePairsSlices par paire d'attribut
	ost << "Array of " << attributePairsSlicesLabel.GetClassLabel() << ": " << oaAttributePairsSlices->GetSize()
	    << "\n";
	for (i = 0; i < oaAttributePairsSlices->GetSize(); i++)
	{
		attributePairsSlices = cast(KWAttributePairsSlices*, oaAttributePairsSlices->GetAt(i));
		ost << *attributePairsSlices << endl;
	}
}

void KWDataPreparationBivariateTask::InitializeAttributePairs(const KWClass* kwcClass,
							      const StringVector* svAttributePairNames1,
							      const StringVector* svAttributePairNames2,
							      ObjectArray* oaOuputAttributePairs) const
{
	int i;
	KWAttributePair* attributePair;

	require(svAttributePairNames1 != NULL);
	require(svAttributePairNames2 != NULL);
	require(svAttributePairNames1->GetSize() == svAttributePairNames2->GetSize());
	require(oaOuputAttributePairs != NULL);
	require(oaOuputAttributePairs->GetSize() == 0);

	// Creation des paires d'attributs dans le tableau
	oaOuputAttributePairs->SetSize(svAttributePairNames1->GetSize());
	for (i = 0; i < svAttributePairNames1->GetSize(); i++)
	{
		attributePair = new KWAttributePair;
		attributePair->SetIndex(i);
		attributePair->SetAttribute1(kwcClass->LookupAttribute(svAttributePairNames1->GetAt(i)));
		attributePair->SetAttribute2(kwcClass->LookupAttribute(svAttributePairNames2->GetAt(i)));
		oaOuputAttributePairs->SetAt(i, attributePair);
	}
}

void KWDataPreparationBivariateTask::SortAttributePairsByBlocks(ObjectArray* oaAttributePairs) const
{
	require(oaAttributePairs != NULL);
	oaAttributePairs->SetCompareFunction(KWAttributePairCompareBlocks);
	oaAttributePairs->Sort();
}

int KWDataPreparationBivariateTask::SearchNextGroupOfAttributePairs(const ObjectArray* oaAttributePairs,
								    int nFirstIndex,
								    ObjectDictionary* odFirstBlockAttributes) const
{
	KWAttributePair* firstAttributePair;
	KWAttributePair* attributePair;
	int nNextIndex;

	require(oaAttributePairs != NULL);
	require(0 <= nFirstIndex and nFirstIndex < oaAttributePairs->GetSize());
	require(odFirstBlockAttributes != NULL);
	require(odFirstBlockAttributes->GetCount() == 0);

	// Acces a la paire au premier index
	firstAttributePair = cast(KWAttributePair*, oaAttributePairs->GetAt(nFirstIndex));
	if (firstAttributePair->GetFirstBlock() != NULL)
		odFirstBlockAttributes->SetAt(firstAttributePair->GetFirstBlockAttribute()->GetName(),
					      firstAttributePair->GetFirstBlockAttribute());

	// Parcours des paires du tableau a partir de l'index de depart, pour trouver un ensemble de paires
	// ayant meme premier bloc et meme second attribut
	nNextIndex = nFirstIndex + 1;
	while (nNextIndex < oaAttributePairs->GetSize())
	{
		// Acces a la paire en cours
		attributePair = cast(KWAttributePair*, oaAttributePairs->GetAt(nNextIndex));
		assert(firstAttributePair->CompareBlocks(attributePair) < 0);

		// Memorisation si les paires sont dans un meme groupe
		if (attributePair->GetFirstBlock() == firstAttributePair->GetFirstBlock() and
		    attributePair->GetSecondBlockAttribute() == firstAttributePair->GetSecondBlockAttribute())
		{
			// Memorisation si on etait dans un bloc
			if (odFirstBlockAttributes->GetCount() > 0)
			{
				assert(odFirstBlockAttributes->Lookup(
					   firstAttributePair->GetFirstBlockAttribute()->GetName()) != NULL);
				assert(odFirstBlockAttributes->Lookup(
					   attributePair->GetFirstBlockAttribute()->GetName()) == NULL);
				odFirstBlockAttributes->SetAt(attributePair->GetFirstBlockAttribute()->GetName(),
							      attributePair->GetFirstBlockAttribute());
			}
		}
		// Arret sinon
		else
			break;

		// Index suivant
		nNextIndex++;
	}

	// On retour le prochain index
	return nNextIndex;
}

////////////////////////////////////////////////////////////////////
// Classe KWAttributePairsSlices

KWAttributePairsSlices::KWAttributePairsSlices()
{
	oaAttributes.SetCompareFunction(KWAttributeCompareName);
	oaSlices.SetCompareFunction(KWDataTableSliceCompareLexicographicIndex);
}

KWAttributePairsSlices::~KWAttributePairsSlices() {}

ObjectArray* KWAttributePairsSlices::GetAttributePairStats()
{
	return &oaAttributePairStats;
}

ObjectArray* KWAttributePairsSlices::GetAttributes()
{
	return &oaAttributes;
}

ObjectArray* KWAttributePairsSlices::GetSlices()
{
	return &oaSlices;
}

void KWAttributePairsSlices::AddAttributePairs(const KWAttributePairsSlices* otherAttributePairsSlices)
{
	int i;
	ObjectArray oaTmpFirstArray;

	require(otherAttributePairsSlices != NULL);
	require(Check());
	require(otherAttributePairsSlices->Check());
	require(this != otherAttributePairsSlices);

	// Ajout des paires d'attributs
	for (i = 0; i < otherAttributePairsSlices->oaAttributePairStats.GetSize(); i++)
		oaAttributePairStats.Add(otherAttributePairsSlices->oaAttributePairStats.GetAt(i));

	// Ajout des attributs en respectant le tri
	oaTmpFirstArray.CopyFrom(&oaAttributes);
	oaAttributes.SetSize(0);
	MergeArrayContent(&oaTmpFirstArray, &otherAttributePairsSlices->oaAttributes, KWAttributeCompareName,
			  &oaAttributes);

	// Ajout des slices en respectant le tri
	oaTmpFirstArray.CopyFrom(&oaSlices);
	oaSlices.SetSize(0);
	MergeArrayContent(&oaTmpFirstArray, &otherAttributePairsSlices->oaSlices,
			  KWDataTableSliceCompareLexicographicIndex, &oaSlices);
	ensure(Check());
}

int KWAttributePairsSlices::CompareSlices(const KWAttributePairsSlices* otherAttributePairsSlices) const
{
	int nSlice;
	int nMaxSliceNumber;
	KWDataTableSlice* slice1;
	KWDataTableSlice* slice2;
	int nCompare;

	require(otherAttributePairsSlices != NULL);

	// Comparaison des tranches
	nCompare = 0;
	nMaxSliceNumber = max(oaSlices.GetSize(), otherAttributePairsSlices->oaSlices.GetSize());
	for (nSlice = 0; nSlice < nMaxSliceNumber; nSlice++)
	{
		// Acces a chaque tranche
		slice1 = NULL;
		slice2 = NULL;
		if (nSlice < oaSlices.GetSize())
			slice1 = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		if (nSlice < otherAttributePairsSlices->oaSlices.GetSize())
			slice2 = cast(KWDataTableSlice*, otherAttributePairsSlices->oaSlices.GetAt(nSlice));
		assert(slice1 != NULL or slice2 != NULL);

		// Comparaison
		if (slice1 == NULL)
			nCompare = -1;
		else if (slice2 == NULL)
			nCompare = 1;
		else
			nCompare = slice1->CompareLexicographicOrder(slice2);
		if (nCompare != 0)
			break;
	}
	return nCompare;
}

DoubleVector* KWAttributePairsSlices::GetLexicographicSortCriterion()
{
	return &dvLexicographicSortCriterion;
}

int KWAttributePairsSlices::CompareLexicographicSortCriterion(
    const KWAttributePairsSlices* otherAttributePairsSlices) const
{
	int nCompare;
	int nMaxSize;
	int i;

	require(otherAttributePairsSlices != NULL);

	// Comparaison des vecteur d'index lexicographique
	nCompare = 0;
	nMaxSize = max(dvLexicographicSortCriterion.GetSize(),
		       otherAttributePairsSlices->dvLexicographicSortCriterion.GetSize());
	for (i = 0; i < nMaxSize; i++)
	{
		if (i >= dvLexicographicSortCriterion.GetSize())
			nCompare = -1;
		else if (i >= otherAttributePairsSlices->dvLexicographicSortCriterion.GetSize())
			nCompare = 1;
		else
			nCompare = CompareDouble(dvLexicographicSortCriterion.GetAt(i),
						 otherAttributePairsSlices->dvLexicographicSortCriterion.GetAt(i));
		if (nCompare != 0)
			break;
	}
	return nCompare;
}

void KWAttributePairsSlices::DisplayLexicographicSortCriterionHeaderLineReport(ostream& ost,
									       const ALString& sSortCriterion) const
{
	ost << GetClassLabel() << "\t" << sSortCriterion << "\n";
}

void KWAttributePairsSlices::DisplayLexicographicSortCriterionLineReport(ostream& ost) const
{
	int i;

	ost << GetObjectLabel() << "\t";
	for (i = 0; i < dvLexicographicSortCriterion.GetSize(); i++)
	{
		ost << "\t";
		ost << dvLexicographicSortCriterion.GetAt(i);
	}
	ost << "\n";
}

boolean KWAttributePairsSlices::Check() const
{
	boolean bOk = true;
	ObjectDictionary odAttributesInPairs;
	ObjectDictionary odAttributes;
	ObjectDictionary odAttributeSlices;
	ObjectDictionary odSlices;
	ObjectDictionary odSlicePerAttributes;
	int nSlice;
	int i;
	KWAttributePairStats* attributePairStats;
	KWAttribute* previousAttribute;
	KWDataTableSlice* previousSlice;
	KWAttribute* attribute;
	KWDataTableSlice* slice;
	ALString sTmp;

	// Il doit y avoir au moins une paire
	if (bOk)
	{
		if (oaAttributePairStats.GetSize() == 0)
		{
			AddError("Empty specification");
			bOk = false;
		}
	}

	// Collecte dans un dictionnaire des tranches et de leurs attributs
	if (bOk)
	{
		previousSlice = NULL;
		for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

			// Erreur si probleme d'ordre
			if (previousSlice != NULL and previousSlice->CompareLexicographicOrder(slice) >= 0)
			{
				AddError("Wrong ordering between " + slice->GetClassLabel() + "s " +
					 previousSlice->GetObjectLabel() + " and " + slice->GetObjectLabel());
				bOk = false;
				break;
			}
			previousSlice = slice;

			// Erreur si tranche deja utilisee
			if (odSlices.Lookup(slice->GetClass()->GetName()) != NULL)
			{
				bOk = false;
				AddError(slice->GetClassLabel() + " " + slice->GetObjectLabel() +
					 " used several times");
				break;
			}
			// Ajout de la tranche sinon
			else
			{
				// Ajout de la tranche
				odSlices.SetAt(slice->GetClass()->GetName(), slice);

				// Collecte des attributs de la tranche
				for (i = 0; i < slice->GetClass()->GetUsedAttributeNumber(); i++)
				{
					attribute = slice->GetClass()->GetUsedAttributeAt(i);
					assert(odSlicePerAttributes.Lookup(attribute->GetName()) == NULL);
					odSlicePerAttributes.SetAt(attribute->GetName(), slice);
				}
			}
		}
	}

	// Collecte dans un dictionnaire des attributs et verification et de leur tranche
	if (bOk)
	{
		previousAttribute = NULL;
		for (i = 0; i < oaAttributes.GetSize(); i++)
		{
			attribute = cast(KWAttribute*, oaAttributes.GetAt(i));

			// Erreur si probleme d'ordre
			if (previousAttribute != NULL and previousAttribute->GetName() > attribute->GetName())
			{
				AddError("Wrong ordering between " + attribute->GetClassLabel() + "s " +
					 previousAttribute->GetName() + " and " + attribute->GetName());
				bOk = false;
				break;
			}
			previousAttribute = attribute;

			// Erreur si attribut deja utilisee
			if (odAttributes.Lookup(attribute->GetName()) != NULL)
			{
				bOk = false;
				AddError(attribute->GetClassLabel() + " " + attribute->GetObjectLabel() +
					 " used several times");
				break;
			}
			// Ajout de l'attribut sinon
			else
			{
				// Ajout de l'attribut
				odAttributes.SetAt(attribute->GetName(), attribute);

				// Erreur si aucune tranche ne le contient
				slice = cast(KWDataTableSlice*, odSlicePerAttributes.Lookup(attribute->GetName()));
				if (slice == NULL)
				{
					bOk = false;
					AddError("No data set slice found for " + attribute->GetClassLabel() + " " +
						 attribute->GetObjectLabel());
					break;
				}
				// Sinon, on memorise la tranche
				else
					odAttributeSlices.SetAt(slice->GetClass()->GetName(), slice);
			}
		}
	}

	// On verifie qu'il n'y a pas tranche inutile
	if (bOk)
	{
		assert(odAttributeSlices.GetCount() <= oaSlices.GetSize());
		if (odAttributeSlices.GetCount() < oaSlices.GetSize())
		{
			bOk = false;
			AddError(sTmp + "More data set slice than necessary to store the variables (" +
				 IntToString(oaSlices.GetSize()) + " instead of " +
				 IntToString(odAttributeSlices.GetCount()) + ")");
		}
	}

	// Collecte dans un dictionnaire des attributs utilises dans les paires
	if (bOk)
	{
		for (i = 0; i < oaAttributePairStats.GetSize(); i++)
		{
			attributePairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(i));

			// Recherche du premier attribut de la paire
			attribute = cast(KWAttribute*, odAttributes.Lookup(attributePairStats->GetAttributeName1()));
			if (attribute == NULL)
			{
				bOk = false;
				AddError("First variable of pair " + attributePairStats->GetObjectLabel() +
					 " not found in variables");
			}
			// Memorisation si ok
			else
				odAttributesInPairs.SetAt(attribute->GetName(), attribute);

			// Recherche du second attribut de la paire
			attribute = cast(KWAttribute*, odAttributes.Lookup(attributePairStats->GetAttributeName2()));
			if (attribute == NULL)
			{
				bOk = false;
				AddError("Second variable of pair " + attributePairStats->GetObjectLabel() +
					 " not found in variables");
			}
			// Memorisation si ok
			else
				odAttributesInPairs.SetAt(attribute->GetName(), attribute);
		}
	}

	// On verifie qu'il n'y a pas d'attribut inutile
	if (bOk)
	{
		assert(odAttributesInPairs.GetCount() <= odAttributes.GetCount());
		if (odAttributesInPairs.GetCount() < odAttributes.GetCount())
		{
			bOk = false;
			AddError(sTmp + "More variables than necessary to store the variables in the pairs (" +
				 IntToString(odAttributes.GetCount()) + " instead of " +
				 IntToString(odAttributesInPairs.GetCount()) + ")");
		}
	}
	return bOk;
}

void KWAttributePairsSlices::Write(ostream& ost) const
{
	int i;
	KWAttributePairStats* attributePairStats;
	KWAttribute* attribute;
	KWDataTableSlice* slice;

	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";

	// Affichages des paires d'attributs
	ost << "  Variable pairs\n";
	for (i = 0; i < oaAttributePairStats.GetSize(); i++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(i));
		ost << "\t" << attributePairStats->GetAttributeName1() << "\t"
		    << attributePairStats->GetAttributeName2() << "\n";
	}

	// Affichages des attributs
	ost << "  Variables\n";
	for (i = 0; i < oaAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaAttributes.GetAt(i));
		ost << "\t" << attribute->GetName() << "\n";
	}

	// Affichages des tranches
	ost << "  Data table slices\n";
	for (i = 0; i < oaSlices.GetSize(); i++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(i));
		ost << "\t" << slice->GetClass()->GetName() << "\n";
	}
}

const ALString KWAttributePairsSlices::GetClassLabel() const
{
	return "Variable pair slices";
}

const ALString KWAttributePairsSlices::GetObjectLabel() const
{
	ALString sLabel;
	sLabel = "(";
	if (oaSlices.GetSize() > 0)
		sLabel += cast(KWDataTableSlice*, oaSlices.GetAt(0))->GetClass()->GetName();
	if (oaSlices.GetSize() > 1)
	{
		sLabel += ";";
		sLabel += cast(KWDataTableSlice*, oaSlices.GetAt(1))->GetClass()->GetName();
	}
	sLabel += ": ";
	sLabel += IntToString(oaAttributePairStats.GetSize());
	sLabel += ", ";
	sLabel += IntToString(oaAttributes.GetSize());
	sLabel += ", ";
	sLabel += IntToString(oaSlices.GetSize());
	sLabel += ")";
	return sLabel;
}

void KWAttributePairsSlices::MergeArrayContent(const ObjectArray* oaFirst, const ObjectArray* oaSecond,
					       CompareFunction fCompare, ObjectArray* oaMergedResult) const
{
	Object* object1;
	Object* object2;
	int nCompare;
	int i;
	int j;

	require(oaFirst != NULL);
	require(oaSecond != NULL);
	require(fCompare != NULL);
	require(oaMergedResult != NULL);
	require(oaMergedResult->GetCompareFunction() != NULL);
	require(oaMergedResult->GetSize() == 0);

	// Cas ou un des tableau est vide
	if (oaFirst->GetSize() == 0)
		oaMergedResult->CopyFrom(oaSecond);
	else if (oaSecond->GetSize() == 0)
		oaMergedResult->CopyFrom(oaFirst);
	// Cas ou aucun tableau n'est vide
	else
	{
		// Acces au premier objet de chaque tableau
		object1 = oaFirst->GetAt(0);
		i = 1;
		object2 = oaSecond->GetAt(0);
		j = 1;

		// Ajout des objets en respectant le tri
		while (object1 != NULL or object2 != NULL)
		{
			// On recopie la fin du premier tableau si on a fini le second
			if (object2 == NULL)
			{
				oaMergedResult->Add(object1);
				while (i < oaFirst->GetSize())
				{
					oaMergedResult->Add(oaFirst->GetAt(i));
					i++;
				}
				break;
			}
			// On recopie la fin du second tableau si on a fini le premier
			else if (object1 == NULL)
			{
				oaMergedResult->Add(object2);
				while (j < oaSecond->GetSize())
				{
					oaMergedResult->Add(oaSecond->GetAt(j));
					j++;
				}
				break;
			}
			// Sinon, on compare les elements
			else
			{
				nCompare = fCompare(&object1, &object2);

				// On rajoute le plus petit element
				if (nCompare <= 0)
					oaMergedResult->Add(object1);
				else
					oaMergedResult->Add(object2);

				// On passe a l'element suivant pour chaque liste, en tenant compte des cas d'egalite
				if (nCompare <= 0)
				{
					object1 = NULL;
					if (i < oaFirst->GetSize())
					{
						object1 = oaFirst->GetAt(i);
						i++;
					}
				}
				if (nCompare >= 0)
				{
					object2 = NULL;
					if (j < oaSecond->GetSize())
					{
						object2 = oaSecond->GetAt(j);
						j++;
					}
				}
			}
		}
	}
}

int KWAttributePairsSlicesCompareSlices(const void* elem1, const void* elem2)
{
	KWAttributePairsSlices* pairs1;
	KWAttributePairsSlices* pairs2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pairs1 = cast(KWAttributePairsSlices*, *(Object**)elem1);
	pairs2 = cast(KWAttributePairsSlices*, *(Object**)elem2);

	// Difference
	nCompare = pairs1->CompareSlices(pairs2);
	return nCompare;
}

int KWAttributePairsSlicesCompareLexicographicSortCriterion(const void* elem1, const void* elem2)
{
	KWAttributePairsSlices* pairs1;
	KWAttributePairsSlices* pairs2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pairs1 = cast(KWAttributePairsSlices*, *(Object**)elem1);
	pairs2 = cast(KWAttributePairsSlices*, *(Object**)elem2);

	// Difference
	nCompare = pairs1->CompareLexicographicSortCriterion(pairs2);
	if (nCompare == 0)
		nCompare = pairs1->CompareSlices(pairs2);
	return nCompare;
}

////////////////////////////////////////////////////////////////////
// Classe KWAttributePair

KWAttributePair::KWAttributePair()
{
	nIndex = 0;
	attribute1 = NULL;
	attribute2 = NULL;
}

KWAttributePair::~KWAttributePair() {}

void KWAttributePair::SetIndex(int nValue)
{
	require(nValue >= 0);
	nIndex = nValue;
}

int KWAttributePair::GetIndex() const
{
	return nIndex;
}

void KWAttributePair::SetAttribute1(KWAttribute* attribute)
{
	attribute1 = attribute;
}

KWAttribute* KWAttributePair::GetAttribute1() const
{
	return attribute1;
}

void KWAttributePair::SetAttribute2(KWAttribute* attribute)
{
	attribute2 = attribute;
}

KWAttribute* KWAttributePair::GetAttribute2() const
{
	return attribute2;
}

const KWAttributeBlock* KWAttributePair::GetFirstBlock() const
{
	require(GetAttribute1() != NULL);
	require(GetAttribute2() != NULL);
	require(GetAttribute1() != GetAttribute2());
	if (attribute1->GetAttributeBlock() == NULL)
		return attribute2->GetAttributeBlock();
	else if (attribute2->GetAttributeBlock() == NULL)
		return attribute1->GetAttributeBlock();
	else if (attribute1->GetAttributeBlock()->GetName() < attribute2->GetAttributeBlock()->GetName())
		return attribute1->GetAttributeBlock();
	else
		return attribute2->GetAttributeBlock();
}

const KWAttributeBlock* KWAttributePair::GetSecondBlock() const
{
	const KWAttributeBlock* firstAttributeBlock;
	firstAttributeBlock = GetFirstBlock();
	if (attribute1->GetAttributeBlock() == firstAttributeBlock)
		return attribute2->GetAttributeBlock();
	else
		return attribute1->GetAttributeBlock();
}

KWAttribute* KWAttributePair::GetFirstBlockAttribute() const
{
	const KWAttributeBlock* firstAttributeBlock;

	firstAttributeBlock = GetFirstBlock();
	if (attribute1->GetAttributeBlock() == firstAttributeBlock)
		return attribute1;
	else
		return attribute2;
}

KWAttribute* KWAttributePair::GetSecondBlockAttribute() const
{
	const KWAttributeBlock* firstAttributeBlock;

	firstAttributeBlock = GetFirstBlock();
	if (attribute1->GetAttributeBlock() == firstAttributeBlock)
		return attribute2;
	else
		return attribute1;
}

int KWAttributePair::CompareBlocks(const KWAttributePair* otherAttributePair)
{
	const KWAttributeBlock* firstAttributeBlock;
	const KWAttributeBlock* secondAttributeBlock;
	const KWAttributeBlock* otherFirstAttributeBlock;
	const KWAttributeBlock* otherSecondAttributeBlock;
	int nCompare;

	require(otherAttributePair != NULL);

	// Acces aux blocs de la paire courante
	firstAttributeBlock = GetFirstBlock();
	secondAttributeBlock = GetSecondBlock();

	// Acces aux blocs de l'autre paire
	otherFirstAttributeBlock = otherAttributePair->GetFirstBlock();
	otherSecondAttributeBlock = otherAttributePair->GetSecondBlock();

	// Comparaison sur le premier bloc de chaque paire
	if (firstAttributeBlock == otherFirstAttributeBlock)
		nCompare = 0;
	else if (firstAttributeBlock == NULL)
		nCompare = 1;
	else if (otherFirstAttributeBlock == NULL)
		nCompare = -1;
	else
	{
		nCompare =
		    -(firstAttributeBlock->GetAttributeNumber() - otherFirstAttributeBlock->GetAttributeNumber());
		if (nCompare == 0)
			nCompare = firstAttributeBlock->GetName().Compare(otherFirstAttributeBlock->GetName());
	}

	// Comparaison sur le second bloc de chaque paire
	if (nCompare == 0)
	{
		if (secondAttributeBlock == otherSecondAttributeBlock)
			nCompare = 0;
		else if (secondAttributeBlock == NULL)
			nCompare = 1;
		else if (otherSecondAttributeBlock == NULL)
			nCompare = -1;
		else
		{
			nCompare = -(secondAttributeBlock->GetAttributeNumber() -
				     otherSecondAttributeBlock->GetAttributeNumber());
			if (nCompare == 0)
				nCompare =
				    secondAttributeBlock->GetName().Compare(otherSecondAttributeBlock->GetName());
		}
	}

	// Comparaison sur l'attribut du deuxieme bloc, qui conditionne l'appatetance a un meme groupe de paire
	if (nCompare == 0)
		nCompare = GetSecondBlockAttribute()->GetName().Compare(
		    otherAttributePair->GetSecondBlockAttribute()->GetName());

	// Comparaison sur l'attribut du premier bloc, pour assurer la reproductibilite
	if (nCompare == 0)
		nCompare = GetFirstBlockAttribute()->GetName().Compare(
		    otherAttributePair->GetFirstBlockAttribute()->GetName());
	return nCompare;
}

int KWAttributePairCompareBlocks(const void* elem1, const void* elem2)
{
	KWAttributePair* pair1;
	KWAttributePair* pair2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pair1 = cast(KWAttributePair*, *(Object**)elem1);
	pair2 = cast(KWAttributePair*, *(Object**)elem2);

	// Difference
	nCompare = pair1->CompareBlocks(pair2);
	return nCompare;
}
