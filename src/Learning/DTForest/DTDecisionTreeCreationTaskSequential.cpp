// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeCreationTaskSequential.h"
#include "DTDecisionTree.h"
#include "DTDecisionTreeSpec.h"
#include "DTDecisionTreeGlobalCost.h"
#include "DTDecisionTreeRecursiveCost.h"
#include "DTDecisionBinaryTreeCost.h"
#include "DTStat.h"
#include "DTForestAttributeSelection.h"
#include "DTDecisionTreeSpec.h"
#include "DTGlobalTag.h"
#include "DTDiscretizerMODL.h"
#include "DTGrouperMODL.h"
#include "DTConfig.h"
#include "DTForestAttributeSelection.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTaskSequential

DTDecisionTreeCreationTaskSequential::DTDecisionTreeCreationTaskSequential()
{
	bIsTraining = false;
}

DTDecisionTreeCreationTaskSequential::~DTDecisionTreeCreationTaskSequential() {}

ALString DTDecisionTreeCreationTaskSequential::GetReportPrefix() const
{
	return "tree";
}

void DTDecisionTreeCreationTaskSequential::SetCreatedAttributePrefix(const ALString& sValue)
{
	sCreatedAttributePrefix = sValue;
}

const ALString& DTDecisionTreeCreationTaskSequential::GetCreatedAttributePrefix() const
{
	return sCreatedAttributePrefix;
}

boolean DTDecisionTreeCreationTaskSequential::CreatePreparedAttributes(KWLearningSpec* learningSpec,
								       KWTupleTableLoader* tupleTableLoader,
								       KWDataTableSliceSet* dataTableSliceSet,
								       ObjectDictionary* odInputAttributeStats,
								       ObjectArray* oaOutputAttributeStats)
{
	boolean bOk = true;
	int nAttribute;
	int nslice;
	KWClass* kwcUpdatedClass;
	KWAttribute* createdAttribute;
	ObjectArray oaCreatedAttributes;
	ObjectArray* oaInputAttributeStats;
	IntVector ivPairFirstIndexes;
	IntVector ivPairSecondIndexes;
	DTDecisionTree* dttree;
	DTBaseLoader blOrigine;
	DTDecisionTreeSpec treespec;
	DTDecisionTreeSpec* reportTreeSpec;
	KWLearningSpec* learningSpecTree;
	KWLearningSpec* originalLearningSpec;
	DTAttributeSelectionsSlices* attributeselectionsslices;
	int nBuidTreeNumber;
	int nidnAttribute;
	int nsliceoffset;
	int nmaxLoadableAttributeNumber;
	// int nUsedAttributeNumber;
	int nMaxCreatedAttributeNumberSlice;
	ALString svariablename;
	ALString sTmp;
	ALString sMessage;
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	// StringVector* allAttibuteselection;
	ObjectArray oaObjects;
	ObjectArray oatupletable;
	ObjectArray oaOrigineAttributs;
	KWTupleTable* ttattribut;
	NumericKeyDictionary ndTreeSingleton;
	NUMERIC key;
	KWDataPreparationUnivariateTask dataPreparationUnivariateTask;
	DTAttributeSelection* attributegenerator;
	DTForestAttributeSelection forestattributeselection;
	ObjectArray oaAttributeSelectionsSlices;
	int oldseed;
	int oldseedtree;

	require(CheckInputParameters(learningSpec, tupleTableLoader->GetInputExtraAttributeTupleTable()));
	require(CheckPreparedAttributes(learningSpec, odInputAttributeStats, oaOutputAttributeStats));
	require(odInputAttributeStats != NULL and odInputAttributeStats->GetCount() > 0);
	require(oaOutputAttributeStats != NULL and oaOutputAttributeStats->GetSize() == 0);
	// MASTER : Partie Initialisation avant paralelisation

	if (learningSpec->GetTargetAttributeType() != KWType::None and
	    tupleTableLoader->GetInputExtraAttributeTupleTable()->GetSize() == 1)
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("The target variable contains only one value");
		return true;
	}

	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and
	    learningSpec->GetPreprocessingSpec()->GetTargetGrouped())
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("No tree building : target variable is grouped");
		return true;
	}
#ifdef TREE_BUILD_TIMER

	DTTimerDiscretize.Reset();
	DTTimerDiscretizeGFT.Reset();
	DTTimer_CreatePreparedAttributes.Reset();
	DTTimerTree1.Reset();
	DTTimerTree2.Reset();
	DTTimerTree3.Reset();
	DTTimerTree4.Reset();
	DTTimer_CreateDecisionTree.Reset();
	DTTimer_ComputeTree.Reset();
	DTTimer_BuildAllTree.Reset();
	DTTimer_BuildRootAttributeStats.Reset();
	DTTimer_CreateAttribute.Reset();
	DTTimer_CreatePreparedAttributes.Start();
	DTTimerBasic1.Reset();
	DTTimerBasic2.Reset();
#endif
	// Nettoyage du rapport de creation existant
	creationReport.Clean();

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskName());

	// MASTER : Partie creation des slice de variables pour creer les arbres avant paralelisation
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Identification des paires de variables a traiter

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Creation des attributs et insertion de ces attributs dans la classe

	// Calcul du cout de base de construction d'un nouvel attribut, en plus des attributs initiaux
	// ComputeBaseConstructionCost(learningSpec, learningSpec->GetClass());

	// Setradomseed a 1972 annee de naissance de son createur

	oldseed = GetRandomSeed();
	SetRandomSeed(1972);

	kwcUpdatedClass = learningSpec->GetClass();
	kwcUpdatedClass->LookupAttribute(learningSpec->GetTargetAttributeName())->SetLoaded(false);
	kwcUpdatedClass->Compile();

	originalLearningSpec = learningSpec;

	// Calcul du nombre d'attribut a analyser, hors attribut cible
	// nUsedAttributeNumber = kwcUpdatedClass->GetUsedAttributeNumberForType(KWType::Continuous) +
	//		       kwcUpdatedClass->GetUsedAttributeNumberForType(KWType::Symbol);

	// Test nouvelle selection
	forestattributeselection.Initialization(odInputAttributeStats);
	BluidForestAttributeSelections(forestattributeselection, nMaxCreatedAttributeNumber);
	// std::ofstream ofs("testoutputarbreselection2.txt");
	// forestattributeselection.WriteReport(ofs);
	// ofs.close();

	// Evaluation du nombre max de variable loadables
	nmaxLoadableAttributeNumber = ComputeMaxLoadableAttributeNumber(
	    learningSpec, tupleTableLoader->GetInputExtraAttributeTupleTable(), odInputAttributeStats,
	    nMaxCreatedAttributeNumber, forestattributeselection);

	// nmaxLoadableAttributeNumber = 1;

	// allAttibuteselection = forestattributeselection.GetAllSelectedAttributes();

	if (nmaxLoadableAttributeNumber < forestattributeselection.GetMaxAttributesNumber())
	{
		// cas ou on ne peut pas charger les variables pour construire l arbre avec la selection la plus grande
		// on arrete la creation des arbre
		sMessage = "Cannot build trees with ";
		sMessage += IntToString(forestattributeselection.GetMaxAttributesNumber());
		sMessage += " variables";
		AddWarning(sMessage);
		SetRandomSeed(oldseed);

		kwcUpdatedClass->SetAllAttributesLoaded(true);
		kwcUpdatedClass->Compile();
		// Fin de tache
		TaskProgression::EndTask();
		return true;
	}

	// Calcul des slices de slection
	BuildAttributeSelectionsSlices(forestattributeselection.GetAttributeSelections(), &oaAttributeSelectionsSlices,
				       dataTableSliceSet, nmaxLoadableAttributeNumber); // nmaxLoadableAttributeNumber
	// MASTER : FIN

	// SLAVE : Partie creation des arbres par slices
	// debut de la boucle de creation des arbres par selectionslice

	// initialisation de l'ID du nom de l'attribut
	nidnAttribute = 0;
	nBuidTreeNumber = 0;
	nsliceoffset = 0;

	for (nslice = 0; nslice < oaAttributeSelectionsSlices.GetSize(); nslice++)
	{
		nsliceoffset = nidnAttribute;
		// oaCreatedAttributes.RemoveAll();

		attributeselectionsslices =
		    cast(DTAttributeSelectionsSlices*, oaAttributeSelectionsSlices.GetAt(nslice));

		// Tous les attributs passent en Unlaod, sauf ceux a charger pour construire les arbres
		LoadOnlySelectedAttributes(kwcUpdatedClass, attributeselectionsslices);
		StartTimer(DTTimer_BuildAllTree);

		// Lecture des objets avec le sliceSet
		bOk = not TaskProgression::IsInterruptionRequested();
		if (bOk)
		{
			TaskProgression::DisplayMainLabel(GetTaskName() + ": Read data table");
			bOk = dataTableSliceSet->ReadAllObjectsWithClass(kwcUpdatedClass, &oaObjects);
		}

		// creation d un learningspec dedie pour la creation de l'arbre
		bOk = not TaskProgression::IsInterruptionRequested();
		if (bOk)
		{
			// si la cible est continue (regression), on transforme la regression en classification, en
			// modifiant le tuple loader et le learningSpec, et en effectuant une binarisation sur la
			// variable cible
			if (tupleTableLoader->GetInputExtraAttributeType() == KWType::Continuous)
				learningSpec =
				    InitializeEqualFreqDiscretization(tupleTableLoader, originalLearningSpec);

			blOrigine.Initialize(learningSpec, tupleTableLoader, &oaObjects);

			// creation d'un kearningspec dedier aux arbres
			learningSpecTree = new KWLearningSpec;
			learningSpecTree->CopyFrom(learningSpec);

			// creation pour un splice de selection :
			// - des arbres dttree
			// - des rapport KWAttributeStats
			// - des rapport json
			// - des specification des arbres

			StartTimer(DTTimer_BuildRootAttributeStats);
			oaInputAttributeStats =
			    BuildRootAttributeStats(learningSpecTree, &blOrigine, odInputAttributeStats);
			StopTimer(DTTimer_BuildRootAttributeStats);

			TaskProgression::DisplayMainLabel(GetTaskName() + ": Build trees");
			TaskProgression::DisplayProgression(0);

			nMaxCreatedAttributeNumberSlice =
			    attributeselectionsslices->GetAttributeSelections()->GetSize();

			// Creation d'attributs
			for (nAttribute = 0; nAttribute < nMaxCreatedAttributeNumberSlice; nAttribute++)
			{
				attributegenerator =
				    cast(DTAttributeSelection*,
					 attributeselectionsslices->GetAttributeSelections()->GetAt(nAttribute));
				nBuidTreeNumber++;

				// Setradomseed a 1972 + nAttribute, pour preparer le parallele

				oldseedtree = GetRandomSeed();
				SetRandomSeed(attributegenerator->GetSeed());

				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}

				// initialisation de l'arbre
				StartTimer(DTTimer_CreateDecisionTree);
				dttree = CreateDecisionTree(learningSpecTree, tupleTableLoader, &oaObjects,
							    oaInputAttributeStats, attributegenerator);
				StopTimer(DTTimer_CreateDecisionTree);
				// Calcul de l'arbre
				StartTimer(DTTimer_ComputeTree);
				if (!ComputeTree(learningSpecTree, &blOrigine, dttree))
				{
					bOk = false;
					break;
				}
				StopTimer(DTTimer_ComputeTree);
				// Creation de la spec de l'arbre a partir de l'arbre calcule dtdecisiontree
				treespec.InitFromDecisionTree(dttree);
				// detection des doublons
				key = treespec.ComputeHashValue();
				// filtre les arbre qui
				//  1 sont similaire key exist deja
				//  2 au moins une profondeur de 3 (2 niveau de groupage discretisation)
				if (ndTreeSingleton.Lookup(key) == NULL and dttree->GetTreeDepth() > 2)
				{
					nidnAttribute++;
					// choix du nom de la variables
					svariablename = kwcUpdatedClass->BuildAttributeName(
					    "Tree_" + ALString(IntToString(attributegenerator->GetIndex() + 1)));

					attribute = treespec.BuildAttribute(svariablename);
					ndTreeSingleton.SetAt(key, attribute);
					// TODO MB : Remplacement de la ligne suivante
					// ComputeConstructedVariableCost(attribute);
					SetConstructedAttributeCost(attribute, treespec.GetConstructionCost() +
										   learningSpec->GetSelectionCost());
					oaCreatedAttributes.Add(attribute);

					if (originalLearningSpec->GetTargetAttributeType() == KWType::Symbol)
						oatupletable.Add(
						    BuildTupleTableForClassification(dttree, svariablename));
					else
						oatupletable.Add(BuildTupleTableForRegression(originalLearningSpec,
											      dttree, svariablename));

					// Insertion dans le rapport de creation
					reportTreeSpec = new DTDecisionTreeSpec;
					reportTreeSpec->InitFromDecisionTree(dttree);
					creationReport.AddTree(svariablename, reportTreeSpec);
				}

				// netoyage de l'arbre courant
				delete dttree;
				SetRandomSeed(oldseedtree);
				TaskProgression::DisplayLabel(
				    "Build trees : " + ALString(IntToString(nBuidTreeNumber)) + "/" +
				    ALString(IntToString(nMaxCreatedAttributeNumber)));
				TaskProgression::DisplayProgression(nBuidTreeNumber * 100 / nMaxCreatedAttributeNumber);
			}
			delete learningSpecTree;
			oaInputAttributeStats->DeleteAll();
			delete oaInputAttributeStats;
			oaObjects.DeleteAll();
		}
		StopTimer(DTTimer_BuildAllTree);

		StartTimer(DTTimer_CreateAttribute);
		bOk = not TaskProgression::IsInterruptionRequested();
		if (bOk)
		{
			// Creation d'attributs

			// On commence a passer tous les attributs de la classe en not loaded
			kwcUpdatedClass = originalLearningSpec->GetClass();
			kwcUpdatedClass->SetAllAttributesLoaded(false);
			originalLearningSpec->GetDatabase()->SetClassName(originalLearningSpec->GetClass()->GetName());

			TaskProgression::DisplayProgression(0);
			// Ajout des attributs dans la classe
			for (nAttribute = nsliceoffset; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
				// createdAttribute->SetName(kwcUpdatedClass->BuildAttributeName(createdAttribute->GetName()));
				createdAttribute->CompleteTypeInfo(kwcUpdatedClass);
				kwcUpdatedClass->InsertAttribute(createdAttribute);

				TaskProgression::DisplayLabel(
				    "Build variable : " + ALString(IntToString(nAttribute + 1)) + "/" +
				    ALString(IntToString(oaCreatedAttributes.GetSize())));
				TaskProgression::DisplayProgression((nAttribute + 1) * 100 /
								    oaCreatedAttributes.GetSize());
			}
			// Compilation de la classe
			// kwcUpdatedClass->Write(cout);
			kwcUpdatedClass->Compile();
			TaskProgression::DisplayProgression(0);
			//////
			for (nAttribute = nsliceoffset; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				ttattribut = cast(KWTupleTable*, oatupletable.GetAt(nAttribute));
				//// Creation et initialisation d'un objet de stats pour l'attribut
				attributeStats = new KWAttributeStats;
				attributeStats->SetLearningSpec(originalLearningSpec);
				attributeStats->SetAttributeName(ttattribut->GetAttributeNameAt(0));
				attributeStats->SetAttributeType(KWType::Symbol);

				//// Calcul des statistitique univariee a partir de la table de tuples
				attributeStats->ComputeStats(ttattribut);
				oaOutputAttributeStats->Add(attributeStats);

				creationReport.SetLevelAt(nAttribute, attributeStats->GetLevel());

				TaskProgression::DisplayLabel(
				    "Compute tree stastistic : " + ALString(IntToString(nAttribute + 1)) + "/" +
				    ALString(IntToString(oaCreatedAttributes.GetSize())));
				TaskProgression::DisplayProgression((nAttribute + 1) * 100 /
								    oaCreatedAttributes.GetSize());
			}

			// mettre en false tous les arbres deja calcule
			for (nAttribute = nsliceoffset; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
				createdAttribute->SetUsed(false);
			}
			kwcUpdatedClass->Compile();
		}
	}
	// SLAVES : FIN

	// MASTER CREATION des rapports
	bOk = not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// on netoie les arbre de level null
		// On repasse tous les attributs de la classe en loaded
		// SLAVE : partie de filtrage des arbre de level nulle a mettre dans le slave
		kwcUpdatedClass = originalLearningSpec->GetClass();
		for (nAttribute = oaCreatedAttributes.GetSize() - 1; nAttribute >= 0; nAttribute--)
		{
			attributeStats = cast(KWAttributeStats*, oaOutputAttributeStats->GetAt(nAttribute));
			createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
			if (attributeStats->GetLevel() == 0)
			{
				creationReport.DeleteTree(createdAttribute->GetName());
				kwcUpdatedClass->RemoveAttribute(createdAttribute->GetName());
				oaOutputAttributeStats->RemoveAt(nAttribute);
				delete attributeStats;
				oaCreatedAttributes.RemoveAt(nAttribute);
				delete createdAttribute;
			}
			else
			{
				createdAttribute->SetUsed(true);
			}
		}
		// SLAVE  : FIN

		kwcUpdatedClass->SetAllAttributesLoaded(true);
		kwcUpdatedClass->Compile();

		// Parametrage des classStats pour specialiser le contenu des rapports
		creationReport.SetClassStats(GetClassStats());

		oatupletable.DeleteAll();

		// message et warning final
		if (oaCreatedAttributes.GetSize() > 0)
		{
			if (oaCreatedAttributes.GetSize() < nMaxCreatedAttributeNumber)
				AddSimpleMessage(sTmp + "Computation of " + IntToString(oaCreatedAttributes.GetSize()) +
						 " distinct decision trees (out of " +
						 IntToString(nMaxCreatedAttributeNumber) + " constructed)");
			else
				AddSimpleMessage(sTmp + "Computation of " + IntToString(oaCreatedAttributes.GetSize()) +
						 " distinct decision trees");
		}
		else
			AddWarning(sTmp + "No informative tree built among the " +
				   IntToString(nMaxCreatedAttributeNumber) + " planned");
	}

	/////////////////////////////////////////////////////////////////////////////
	// Finalisation

	oaAttributeSelectionsSlices.DeleteAll();

	if (originalLearningSpec->GetTargetAttributeType() == KWType::Continuous)
	{
		// nettoyer les donnes fictives ayant servi a transformer temporairement une regression en
		// classification
		if (tupleTableLoader->GetInputExtraAttributeSymbolValues() != NULL)
		{
			delete learningSpec;
			delete tupleTableLoader->GetInputExtraAttributeSymbolValues();
			KWClassDomain::GetCurrentDomain()->DeleteClass(originalLearningSpec->GetTargetAttributeName() +
								       "_categorical");
		}
	}

	// Nettoyage si echec
	if (not bOk)
	{
		assert(oaOutputAttributeStats->GetSize() == 0);
		creationReport.Clean();
		// Suppression des attributs de la classe
		for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
		{
			createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
			kwcUpdatedClass->DeleteAttribute(createdAttribute->GetName());
		}
		// oaCreatedAttributes.SetSize(0);
		oaCreatedAttributes.DeleteAll();
		oaOutputAttributeStats->DeleteAll();
		oatupletable.DeleteAll();
		// On recompile la classe
		kwcUpdatedClass->Compile();
	}

	// Message si erreur
	if (not bOk)
	{
		if (TaskProgression::IsInterruptionRequested())
			AddWarning("Interrupted by user");
		else
			AddError("Interrupted because of errors");
	}

	// Fin de tache
	TaskProgression::EndTask();

	ensure(CheckPreparedAttributes(originalLearningSpec, odInputAttributeStats, oaOutputAttributeStats));
	ensure(bOk or oaCreatedAttributes.GetSize() == 0);
	ensure(oaCreatedAttributes.GetSize() == oaOutputAttributeStats->GetSize());
	// on remet la seed a sa valeur avant l'appel de la fonction
	SetRandomSeed(oldseed);

// Affichage des performances pour la contruction des arbres
#ifdef TREE_BUILD_TIMER
	StopTimer(DTTimer_CreatePreparedAttributes);
	StopTimer(DTTimer_CreateAttribute);
	AddSimpleMessage(sTmp + " NB TREE : \t" + IntToString(nMaxCreatedAttributeNumber));
	AddSimpleMessage(sTmp + " NB VAR : \t" + IntToString(learningSpec->GetInitialAttributeNumber()));
	AddSimpleMessage(sTmp + " CreatePreparedAttributes time: \t" +
			 DoubleToString(DTTimer_CreatePreparedAttributes.GetElapsedTime()));
	AddSimpleMessage(sTmp + " Buildtree process time: \t" + DoubleToString(DTTimer_BuildAllTree.GetElapsedTime()));
	AddSimpleMessage(sTmp + " CreateDecisionTree time: \t" +
			 DoubleToString(DTTimer_CreateDecisionTree.GetElapsedTime()));
	AddSimpleMessage(sTmp + " ComputeTree time: \t" + DoubleToString(DTTimer_ComputeTree.GetElapsedTime()));
	AddSimpleMessage(sTmp + " prepa attribut de CreatePreparedAttributes time: \t" +
			 DoubleToString(DTTimer_CreateAttribute.GetElapsedTime()));
	AddSimpleMessage(sTmp + " DiscretizeGFT time: \t" +
			 DoubleToString(DTTimerDiscretizeGFT.GetElapsedTime())); // a ajouter dasn la V10 + "\t" +
	// IntToString(DTTimerDiscretizeGFT.GetLoop()));
	AddSimpleMessage(sTmp + " Discretize time: \t" +
			 DoubleToString(DTTimerDiscretize.GetElapsedTime())); // a ajouter dasn la V10 + "\t" +
	// IntToString(DTTimerDiscretize.GetLoop()));
	AddSimpleMessage(sTmp + " BuildRootAttributeStats time: \t" +
			 DoubleToString(DTTimer_BuildRootAttributeStats.GetElapsedTime()));
	AddSimpleMessage(sTmp + " SetUpInternalNode : \t" + DoubleToString(DTTimerTree1.GetElapsedTime()));
	AddSimpleMessage(sTmp + " SetUp DTBaseLoaderSplitter : \t" + DoubleToString(DTTimerTree2.GetElapsedTime()));
	AddSimpleMessage(sTmp + " SetUp ComputeAttributesStat : \t" + DoubleToString(DTTimerTree3.GetElapsedTime()));
	AddSimpleMessage(sTmp + " SetUp SelectPossibleSplits : \t" + DoubleToString(DTTimerTree4.GetElapsedTime()));
	AddSimpleMessage(sTmp + " LoadUnivariate dans BasicCollectPreparationStats time : \t" +
			 DoubleToString(DTTimerBasic1.GetElapsedTime()));
	AddSimpleMessage(sTmp + " ComputeStats dans BasicCollectPreparationStats time : \t" +
			 DoubleToString(DTTimerBasic2.GetElapsedTime()));
#endif

	return bOk;
}

KWLearningReport* DTDecisionTreeCreationTaskSequential::GetCreationReport()
{
	return &creationReport;
}

void DTDecisionTreeCreationTaskSequential::CopyAttributeCreationSpecFrom(
    const KDDataPreparationAttributeCreationTask* attributeCreationTask)
{
	// Appel de la methode ancetre
	KDDataPreparationAttributeCreationTask::CopyAttributeCreationSpecFrom(attributeCreationTask);

	// Recopie des attributs specifiques
	sCreatedAttributePrefix =
	    cast(DTDecisionTreeCreationTaskSequential*, attributeCreationTask)->GetCreatedAttributePrefix();
	GetForestParameter()->CopyFrom(
	    cast(DTDecisionTreeCreationTaskSequential*, attributeCreationTask)->GetForestParameter());
}

const ALString DTDecisionTreeCreationTaskSequential::GetTaskName() const
{
	return "Decision Tree variable creation";
}

PLParallelTask* DTDecisionTreeCreationTaskSequential::Create() const
{
	return new DTDecisionTreeCreationTaskSequential;
}

boolean DTDecisionTreeCreationTaskSequential::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	return false;
}

boolean DTDecisionTreeCreationTaskSequential::MasterAggregateResults()
{
	return false;
}

boolean DTDecisionTreeCreationTaskSequential::SlaveProcess()
{
	return false;
}

DTDecisionTree* DTDecisionTreeCreationTaskSequential::CreateDecisionTree(KWLearningSpec* learningSpec,
									 KWTupleTableLoader* tupleTableLoader,
									 ObjectArray* oaObjects,
									 ObjectArray* oaInputAttributeStats,
									 DTAttributeSelection* attgenerator)
{
	learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName(
	    randomForestParameter.GetDecisionTreeParameter()->GetDiscretizationMethod());
	learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName(
	    randomForestParameter.GetDecisionTreeParameter()->GetGroupingMethod());
	//= new ObjectArray;
	boolean bcomputestat = false;
	// arbre de creation
	DTDecisionTree* currentTree = new DTDecisionTree;

	// SetorigineBaseLoader
	DTBaseLoader* blOrigine = new DTBaseLoader;
	blOrigine->Initialize(learningSpec, tupleTableLoader, oaObjects);
	currentTree->SetOrigineBaseLoader(blOrigine);

	// selection de variable

	currentTree->SetLearningSpec(learningSpec);

	if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::SCHRODER_LABEL)
	{
		currentTree->SetCostClass(new DTDecisionTreeGlobalCost);
		currentTree->GetCostClass()->SetDTCriterionMode(1);
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::CATALAN_LABEL)
	{
		currentTree->SetCostClass(new DTDecisionTreeGlobalCost);
		currentTree->GetCostClass()->SetDTCriterionMode(0);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);
		bcomputestat = true;
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_NEW_BIN_LABEL)
	{
		currentTree->SetCostClass(new DTDecisionBinaryTreeCost);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("DTMODL");
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName("DTMODL");
		bcomputestat = true;
	}
	else
		currentTree->SetCostClass(new DTDecisionTreeRecursiveCost);

	if (randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber() > 1)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
		bcomputestat = true;
	}

	if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_LABEL)
		currentTree->GetCostClass()->SetDTCriterionMode(0);
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_RISSANEN_LABEL)
		currentTree->GetCostClass()->SetDTCriterionMode(1);
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_N2_LABEL)
		currentTree->GetCostClass()->SetDTCriterionMode(2);
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_BIN_LABEL)
	{
		currentTree->GetCostClass()->SetDTCriterionMode(3);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("DTMODL");
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName("DTMODL");
		bcomputestat = true;
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_TRI_LABEL)
	{
		currentTree->GetCostClass()->SetDTCriterionMode(4);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(3);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(3);
		bcomputestat = true;
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_RISSANEN_V1_LABEL)
	{
		currentTree->GetCostClass()->SetDTCriterionMode(5);
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_RISSANEN_V2_LABEL)
	{
		currentTree->GetCostClass()->SetDTCriterionMode(6);
	}

	// if (randomForestParameter.GetHeuristicCreation() == DTForestParameter::Heuristic_UDEPTH_LABEL or
	// randomForestParameter.GetHeuristicCreation() == DTForestParameter::Heuristic_USD_LABEL)
	//{
	//	DTForestParameter * rfparamnew = new DTForestParameter;
	//	rfparamnew->CopyFrom(randomForestParameter);
	//	currentTree->SetParameters(cast(DTPredictorParameter*, randomForestParameter));
	// }
	// else
	//{
	//	currentTree->SetParameters(cast(DTPredictorParameter*, randomForestParameter));
	// }

	currentTree->SetParameters(cast(DTDecisionTreeParameter*, randomForestParameter.GetDecisionTreeParameter()));

	if (randomForestParameter.GetDrawingType() == DTDecisionTree::DrawingType::NoReplacement)
		currentTree->SetDrawingType(DTDecisionTree::DrawingType::NoReplacement);
	if (randomForestParameter.GetDrawingType() == DTDecisionTree::DrawingType::UseOutOfBag)
		currentTree->SetDrawingType(DTDecisionTree::DrawingType::UseOutOfBag);
	if (randomForestParameter.GetDrawingType() == DTDecisionTree::DrawingType::WithReplacementAdaBoost)
		currentTree->SetDrawingType(DTDecisionTree::DrawingType::WithReplacementAdaBoost);

	// SelectTreeAttributes(currentTree, attgenerator, learningSpec->GetDatabase()->GetEstimatedObjectNumber());
	// initialisation de l'arbre

	currentTree->SetSelectedAttributes(attgenerator->GetAttributeSelection());
	currentTree->SetUsableAttributesNumber(attgenerator->GetUsableAttributesNumber());

	currentTree->GetRootNode()->SetNodeAttributeStats(oaInputAttributeStats);
	currentTree->GetRootNode()->SetLearningSpec(learningSpec);

	return currentTree;
}

/////////////////////////////////////////////////////////////////////////////////

ObjectArray* DTDecisionTreeCreationTaskSequential::BuildRootAttributeStats(KWLearningSpec* learningSpec,
									   DTBaseLoader* blOrigine,
									   ObjectDictionary* odInputAttributeStats)
{
	require(odInputAttributeStats != NULL);
	ObjectArray oaInputAttribute;
	learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName(
	    randomForestParameter.GetDecisionTreeParameter()->GetDiscretizationMethod());
	learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName(
	    randomForestParameter.GetDecisionTreeParameter()->GetGroupingMethod());
	//= new ObjectArray;
	boolean bcomputestat = false;
	// arbre de creation
	ObjectArray* oaOutputAttributeStats = new ObjectArray;

	// selection de variable

	if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::CATALAN_LABEL)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);

		bcomputestat = true;
	}
	else

	    if (randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber() > 1)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
		bcomputestat = true;
	}

	if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_BIN_LABEL ||
	    randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_NEW_BIN_LABEL)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("DTMODL");
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName("DTMODL");
		bcomputestat = true;
	}

	if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_TRI_LABEL)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(3);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(3);
		bcomputestat = true;
	}

	if (bcomputestat == true)
	{
		KWDataPreparationUnivariateTask dataPreparationUnivariateTask;
		dataPreparationUnivariateTask.CollectInputAttributes(learningSpec, learningSpec->GetClass(),
								     &oaInputAttribute);

		blOrigine->GetTupleLoader()->SetInputDatabaseObjects(blOrigine->GetDatabaseObjects());
		dataPreparationUnivariateTask.BasicCollectPreparationStats(
		    learningSpec, blOrigine->GetTupleLoader(), &oaInputAttribute, true, oaOutputAttributeStats);
	}
	else
	{
		odInputAttributeStats->ExportObjectArray(oaOutputAttributeStats);
	}

	return oaOutputAttributeStats;
}
////////////////////////////////////////////////////////////////////////////////

boolean DTDecisionTreeCreationTaskSequential::BluidForestAttributeSelections(
    DTForestAttributeSelection& forestAttributeSelection, int nmaxCreatedAttributeNumber)
{
	// calcul du nbre d'attributs potentiellement utilisables dans l'arbre (on retire 1 pour l'attribut cible)
	int bOk;

	bOk = false;

	if (randomForestParameter.GetTreesVariablesSelection() == DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL)
	{
		forestAttributeSelection.BuildForestSelections(nmaxCreatedAttributeNumber,
							       randomForestParameter.GetVariableNumberMin());

		bOk = true;
	}
	else
		forestAttributeSelection.BuildForestUniformSelections(
		    nmaxCreatedAttributeNumber, randomForestParameter.GetTreesVariablesSelection(),
		    randomForestParameter.GetAttributePercentage());
	// DTAttributeSelection attributeselection(attgenerator->GetAttributeStats());
	//  Heuristic_USELECT_LABEL Heuristic_UDEPTH_LABEL Heuristic_USD_LABEL Heuristic_USD_LABEL

	return true;
}

////////////////////////////////////////////////////////////////////////////////

// void DTDecisionTreeCreationTaskSequential::SelectTreeAttributes(DTDecisionTree* tree, DTAttributeSelection*
// attgenerator, longint nObject)
//{
//	assert(attgenerator != NULL); // NB. l'objet DTAttributeSelection recu en parametre a ete construit a partir du
// KWClassStats 'general'. 	assert(attgenerator->GetSize() > 0);
//
//	// calcul du nbre d'attributs potentiellement utilisables dans l'arbre (on retire 1 pour l'attribut cible)
//	int nOrigineAttributesNumber = attgenerator->GetSize();
//	int nAttributesNumber;
//	int ival;
//	ObjectArray* oalistAttributes = attgenerator->GetAttributeSelection();
//	//nAttributesNumber = attgenerator->get
//	//initialisation de l'arbre
//	//			tree->SetSelectedAttributes(oalistAttributes);
//	tree->SetUsableAttributesNumber(nAttributesNumber);
// }

boolean DTDecisionTreeCreationTaskSequential::ComputeTree(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine,
							  DTDecisionTree* tree)
{
	boolean bOk = true;
	// Initialisation de l'arbre courant
	// randomForest->CreateCurrentTree(GetParameters(), baseLoader);
	// Calcul du nombre d'attribut a analyser, hors attribut cible
	int nUsedAttributeNumber = learningSpec->GetClass()->GetUsedAttributeNumberForType(KWType::Continuous) +
				   learningSpec->GetClass()->GetUsedAttributeNumberForType(KWType::Symbol);

	// DTDecisionTree * tree = randomForest->GetCurrentTree();
	tree->SetLearningSpec(learningSpec);

	tree->InitializeRootNode(NULL);

	tree->SetOrigineBaseLoader(blOrigine);

	tree->GetCostClass()->SetClassValueNumber(
	    tree->GetRootNode()->GetTargetDescriptiveStats()->GetTargetDescriptiveStats()->GetValueNumber());
	tree->GetCostClass()->SetTotalAttributeNumber(nUsedAttributeNumber);

	// Initialisation du cout de l'arbre egal a sa racine
	tree->InitializeCostValue(tree->GetCostClass()->ComputeTotalTreeCost(tree));

	// calcul de l'arbre
	bOk = tree->ComputeStats();

	// calcul du cout des noeuds
	// tree->GetCostClass()->ComputeNodeCost(tree->GetRootNode(), tree);

	tree->GetCostClass()->ComputeTreeConstructionCost(tree);

	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

KWAttributeStats* DTDecisionTreeCreationTaskSequential::BuildAttributeStats(KWLearningSpec* learningSpec,
									    DTDecisionTree* dttree,
									    const ALString& svariablename) const
{
	KWAttributeStats* attributeStats;
	KWTupleTable univariateTupleTable;
	KWTuple* inputTuple;
	SymbolVector svValues1;
	SymbolVector svValues2;
	ObjectArray* oaleaves;
	NumericKeyDictionary* ndmodalities;
	int i, nEvent;

	// Initialisation de la structure de la table de tuples
	univariateTupleTable.AddAttribute(svariablename, KWType::Symbol);
	univariateTupleTable.AddAttribute(dttree->GetTargetAttributeName(), KWType::Symbol);

	// Preparation de la table au mode edition
	univariateTupleTable.DeleteAll();
	univariateTupleTable.SetUpdateMode(true);

	oaleaves = new ObjectArray;
	dttree->GetLeaves()->ExportObjectArray(oaleaves);
	DTDecisionTreeNode* dtnode;
	inputTuple = univariateTupleTable.GetInputTuple();

	// remplissage de la table de tuple
	for (int l = 0; l < oaleaves->GetSize(); l++)
	{
		dtnode = cast(DTDecisionTreeNode*, oaleaves->GetAt(l));
		ndmodalities = dtnode->GetTargetModalitiesCountTrain();
		// Alimentation de la table de tuples
		for (i = 0; i < dttree->GetReferenceTargetModalities()->GetSize(); i++)
		{
			Object* o = ndmodalities->Lookup(&dttree->GetReferenceTargetModalities()->GetAt(i));

			if (o != NULL)
			{
				DTDecisionTree::TargetModalityCount* tmcEvent =
				    cast(DTDecisionTree::TargetModalityCount*, o);
				nEvent = tmcEvent->iCount;

				inputTuple->SetSymbolAt(0, Symbol(dtnode->GetNodeIdentifier()));
				inputTuple->SetSymbolAt(1, dttree->GetReferenceTargetModalities()->GetAt(i));
				inputTuple->SetFrequency(nEvent);
				univariateTupleTable.UpdateWithInputTuple();
			}
		}
	}

	// Creation et initialisation d'un objet de stats pour l'attribut
	attributeStats = new KWAttributeStats;
	attributeStats->SetLearningSpec(learningSpec);
	attributeStats->SetAttributeName(svariablename);
	attributeStats->SetAttributeType(KWType::Symbol);

	// Calcul des statistitique univariee a partir de la table de tuples
	attributeStats->ComputeStats(&univariateTupleTable);

	univariateTupleTable.CleanAll();

	delete oaleaves;

	return attributeStats;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

KWTupleTable*
DTDecisionTreeCreationTaskSequential::BuildTupleTableForClassification(DTDecisionTree* dttree,
								       const ALString& svariablename) const
{
	// KWAttributeStats * attributeStats;
	KWTupleTable* univariateTupleTable;
	KWTuple* inputTuple;
	SymbolVector svValues1;
	Symbol sValue;
	SymbolVector svValues2;
	ObjectArray* oaleaves;
	NumericKeyDictionary* ndmodalities;
	int i, nEvent;

	univariateTupleTable = new KWTupleTable;

	// Initialisation de la structure de la table de tuples
	univariateTupleTable->AddAttribute(svariablename, KWType::Symbol);
	univariateTupleTable->AddAttribute(dttree->GetTargetAttributeName(), KWType::Symbol);

	// Preparation de la table au mode edition
	univariateTupleTable->DeleteAll();
	univariateTupleTable->SetUpdateMode(true);

	oaleaves = new ObjectArray;
	dttree->GetLeaves()->ExportObjectArray(oaleaves);
	DTDecisionTreeNode* dtnode;

	oaleaves->SetCompareFunction(DTDecisionTreeNodeCompare);
	oaleaves->Sort();

	// remplissage de la table de tuple
	for (int l = 0; l < oaleaves->GetSize(); l++)
	{
		dtnode = cast(DTDecisionTreeNode*, oaleaves->GetAt(l));
		ndmodalities = dtnode->GetTargetModalitiesCountTrain();
		// Alimentation de la table de tuples
		for (i = 0; i < dttree->GetReferenceTargetModalities()->GetSize(); i++)
		{
			sValue = dttree->GetReferenceTargetModalities()->GetAt(i);
			Object* o = ndmodalities->Lookup(sValue.GetNumericKey());

			if (o != NULL)
			{
				DTDecisionTree::TargetModalityCount* tmcEvent =
				    cast(DTDecisionTree::TargetModalityCount*, o);
				nEvent = tmcEvent->iCount;
				// cout << "noeud : " << dtnode->GetNodeIdentifier() << " , class : " <<
				// dttree->GetReferenceTargetModalities()->GetAt(i) << " , Freq : " << nEvent << endl;
				inputTuple = univariateTupleTable->GetInputTuple();
				inputTuple->SetSymbolAt(0, Symbol(dtnode->GetNodeIdentifier()));
				inputTuple->SetSymbolAt(1, dttree->GetReferenceTargetModalities()->GetAt(i));

				inputTuple->SetFrequency(nEvent);
				univariateTupleTable->UpdateWithInputTuple();
			}
		}
	}

	//// Creation et initialisation d'un objet de stats pour l'attribut
	// attributeStats = new KWAttributeStats;
	// attributeStats->SetLearningSpec(learningSpec);
	// attributeStats->SetAttributeName(svariablename);
	// attributeStats->SetAttributeType(KWType::Symbol);

	//// Calcul des statistitique univariee a partir de la table de tuples
	// attributeStats->ComputeStats(&univariateTupleTable);
	univariateTupleTable->SetUpdateMode(false);

	delete oaleaves;

	return univariateTupleTable;

	// delete oaleaves;

	// return attributeStats;
}
KWTupleTable* DTDecisionTreeCreationTaskSequential::BuildTupleTableForRegression(const KWLearningSpec* learningSpec,
										 DTDecisionTree* dttree,
										 const ALString& svariablename) const
{
	// KWAttributeStats * attributeStats;
	KWTupleTable* tupleTable;
	KWTuple* inputTuple;

	ContinuousVector* cvtarget =
	    cast(ContinuousVector*,
		 dttree->GetOrigineBaseLoader()->GetTupleLoader()->GetInputExtraAttributeContinuousValues());
	const ObjectArray* oaDatabaseObjects =
	    dttree->GetOrigineBaseLoader()->GetTupleLoader()->GetInputDatabaseObjects();

	tupleTable = new KWTupleTable;

	// Initialisation de la structure de la table de tuples
	tupleTable->AddAttribute(svariablename, KWType::Symbol); // ajout de l'attribut Tree_xxx
	tupleTable->AddAttribute(learningSpec->GetTargetAttributeName(), KWType::Continuous);

	// Preparation de la table au mode edition
	tupleTable->DeleteAll();
	tupleTable->SetUpdateMode(true);

	for (int i = 0; i < oaDatabaseObjects->GetSize(); i++)
	{
		KWObject* kwo = cast(KWObject*, oaDatabaseObjects->GetAt(i));
		Object* o = dttree->GetDatabaseObjects()->Lookup(kwo);
		assert(o != NULL);
		const Symbol sNodeId = cast(DTDecisionTreeDatabaseObject*, o)->GetNodeIdentifier();

		inputTuple = tupleTable->GetInputTuple();
		inputTuple->SetSymbolAt(0, sNodeId);
		inputTuple->SetContinuousAt(1, cvtarget->GetAt(i));
		tupleTable->UpdateWithInputTuple();
	}

	tupleTable->SetUpdateMode(false);

	return tupleTable;
}

int DTDecisionTreeCreationTaskSequential::ComputeMaxLoadableAttributeNumber(
    const KWLearningSpec* learningSpec, const KWTupleTable* targetTupleTable, ObjectDictionary* odInputAttributeStats,
    int nDecisionTreeMaxNumber, DTForestAttributeSelection& forestAttributeSelection) const
{
	boolean bDisplayMemoryStats = false;
	RMTaskResourceRequirement resourceRequirement;
	RMTaskResourceGrant grantedResources;
	KWClass* kwcClass;
	longint lMeanStatMemory;
	longint lAvailableMemory;
	longint lResultMemory;
	longint lDBMemoryForTree;
	longint lNecessaryMemory;
	longint lMeanTreeSpecMemory;
	longint lTreeResultMemory;
	longint lTreeWorkingMemory;
	longint lOneAttributeValueMemory;
	int nDatabaseObjectNumber;
	int nNodeVariableNumber;
	int nLeaveVariableNumber;
	// int nDepthMean;
	int nMaxAttributesSelectionNumber;
	int nUsedAttributeNumber;
	longint lMaxAttributeNumber;
	ALString sMessage;

	require(CheckInputParameters(learningSpec, targetTupleTable));

	// explication du CALCUL
	// NBVARMAX nombre de varible MAX loadable
	// NBTREE : nombre d arbre
	//
	// MeanStatMemory = memoire moyen des stats
	// ResultMemory = memoire prise par l'ensemble des arbres
	// DBMemoryForTree = memoire des bases dasn la structure d'arbre
	// MaxAttributeNumber = AvailableMemory - MeanStatMemory - ResultMemory) / (MeanStatMemory + DBMemoryForTree)
	//
	// AvailableMemory >= WorkingMemory+DataBaseMemory+ResultMemory+FieldMemory
	//

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

	// calcul du nombre totale de variable selectionnees
	nMaxAttributesSelectionNumber =
	    forestAttributeSelection.GetMaxAttributesNumber(); // on met toujours au moins un attribut dans l'arbre

	// Calcul du nombre de noeud interne + feuille moyen par arbre
	// nDepthMean = (int)log2(double(nDatabaseObjectNumber / 64.));
	nNodeVariableNumber = nDatabaseObjectNumber / 64;
	nLeaveVariableNumber = nNodeVariableNumber;

	// nDepthMean = nDepthMean < 1 ? 1 : nDepthMean;
	//////////////////////////////////////////////////////////////////////////
	// Estimation des ressources necessaires pour la preparation des donnees

	// Calcul de la memoire de travail et de la memoire de stockage par resultat

	lMeanStatMemory = ComputeMedianStatMemory(odInputAttributeStats);
	lMeanTreeSpecMemory = ComputeMeanTreeSpecMemory(odInputAttributeStats) * nLeaveVariableNumber;

	lTreeResultMemory = lMeanTreeSpecMemory + lMeanStatMemory;

	// calcul de nDBMemoryForTree
	lDBMemoryForTree = 2 * nDatabaseObjectNumber * sizeof(KWObject);

	// Memoire de travail pour gerer un arbre
	// Objet a vide, plus un pointeur dans tous les noeuds de l'arbre, le tout par enregistrement
	lTreeWorkingMemory = sizeof(KWObject) * nDatabaseObjectNumber;
	// lTreeWorkingMemory += (sizeof(KWObject*) * nNodeVariableNumber) * nDatabaseObjectNumber;
	lTreeWorkingMemory += (sizeof(KWObject*) * nMaxAttributesSelectionNumber) * nDatabaseObjectNumber;

	// Memoire pour stocker un valeur d'attribut, pour tous les objets
	// Attention, il faudra prendre en compte le cas sparse de facon plus fine
	lOneAttributeValueMemory = sizeof(KWValue) * nDatabaseObjectNumber;
	lOneAttributeValueMemory *= 2;

	// Memoire pour le stockage d'un attribut de classe
	// lClassAttributeMemory = ComputeNecessaryClassAttributeMemory();

	// Memoire pour charger la specification d'un attribut de dictionnaire
	// lDatabaseAttributeMemory = lClassAttributeMemory;

	// NOTE MB -> NV
	// Regarder les evolutions de KWDataPreparationTask::ComputeMaxLoadableAttributeNumber
	// pour prendre en compte le dimensionnement des blocs

	// ResultMemory = nDecisionTreeMaxNumber*nLeaveVariableNumber*ComputeMeanTreeSpecMemory(odInputAttributeStats);
	lResultMemory = nDecisionTreeMaxNumber * lTreeResultMemory;

	// lAvailableMemory
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	///////////////////////////////////////////////////////////////////////////////////////////
	// Calcul du nombre max d'attributs

	// lMaxAttributeNumber = ((lAvailableMemory - lMeanStatMemory - lResultMemory) / (lMeanStatMemory +
	// lDBMemoryForTree));

	// Nouveau calcul a activer potentiellement apres validation par l'expert forestier
	lMaxAttributeNumber = ((lAvailableMemory - lMeanStatMemory - lResultMemory - lTreeWorkingMemory) /
			       (lMeanStatMemory + lOneAttributeValueMemory));

	// ressoure memoire minimum pour avoir nNodeAttributesNumber attributs
	// lNecessaryMemory = nMaxAttributesSelectionNumber * (lMeanStatMemory + lDBMemoryForTree) + lMeanStatMemory +
	// lResultMemory;
	lNecessaryMemory = nMaxAttributesSelectionNumber * (lMeanStatMemory + lOneAttributeValueMemory) +
			   lMeanStatMemory + lResultMemory + lTreeWorkingMemory;

	// Affichage des stats
	if (bDisplayMemoryStats)
	{
		cout << "DTDecisionTreeCreationTaskSequential::ComputeMaxLoadableAttributeNumber" << endl;
		cout << "lMeanStatMemory = \t" << lMeanStatMemory << endl;
		cout << "nDBMemoryForTree = \t" << lDBMemoryForTree << endl;
		cout << "nMaxAttributesSelectionNumber = \t" << nMaxAttributesSelectionNumber << endl;
		cout << "nMaxAttributeNumber = \t" << lMaxAttributeNumber << endl;
		cout << "ResultMemory = \t" << lResultMemory << endl;
		cout << "nDatabaseObjectNumber = \t" << nDatabaseObjectNumber << endl;
		cout << "nLeaveVariableNumber = \t" << nLeaveVariableNumber << endl;
		cout << "lMeanTreeSpecMemory = \t" << lMeanTreeSpecMemory << endl;
		cout << "lAvailableMemory = \t" << lAvailableMemory << endl;
		cout << "ComputeMeanTreeSpecMemory = \t" << ComputeMeanTreeSpecMemory(odInputAttributeStats) << endl;
		cout << "lNecessaryMemory = \t" << lNecessaryMemory << endl;
		cout << "lOneAttributeValueMemory = \t" << lOneAttributeValueMemory << endl;
		cout << "nNodeVariableNumber = \t" << nNodeVariableNumber << endl;
		cout << "lTreeWorkingMemory = \t" << lTreeWorkingMemory << endl;
	}

	// Message d'erreur si pas assez de memoire
	if (lMaxAttributeNumber < forestAttributeSelection.GetMaxAttributesNumber())
	{
		sMessage = "Not enough memory to build trees";
		AddWarning(sMessage + RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		return 0;
	}

	require(lMaxAttributeNumber > 0 && lMaxAttributeNumber <= INT32_MAX);

	return (int)lMaxAttributeNumber;
}

longint DTDecisionTreeCreationTaskSequential::ComputeMeanStatMemory(ObjectDictionary* odInputAttributeStats) const
{
	longint lWorkingMemorySize = 0;
	longint nbvar;
	ALString sIdentifier;
	POSITION position;
	Object* object;
	KWAttributeStats* attributeStats;

	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);
	nbvar = 0;

	position = odInputAttributeStats->GetStartPosition();
	while (position != NULL)
	{
		odInputAttributeStats->GetNextAssoc(position, sIdentifier, object);
		attributeStats = cast(KWAttributeStats*, object);
		nbvar++;
		lWorkingMemorySize += attributeStats->GetUsedMemory();
	}
	require(nbvar > 0);

	if (nbvar == 0)
		return 0;
	return lWorkingMemorySize / nbvar;
}

longint DTDecisionTreeCreationTaskSequential::ComputeMedianStatMemory(ObjectDictionary* odInputAttributeStats) const
{
	longint lWorkingMemorySize = 0;
	longint nbvar;
	ALString sIdentifier;
	POSITION position;
	Object* object;
	KWAttributeStats* attributeStats;
	LongintVector cvValues;
	longint cMedian;

	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);

	nbvar = 0;
	position = odInputAttributeStats->GetStartPosition();
	while (position != NULL)
	{
		odInputAttributeStats->GetNextAssoc(position, sIdentifier, object);
		attributeStats = cast(KWAttributeStats*, object);
		if (attributeStats->GetLevel() > 0)
			cvValues.Add(attributeStats->GetUsedMemory());
		nbvar++;
		lWorkingMemorySize += attributeStats->GetUsedMemory();
	}

	// Calcul de la valeur mediane
	if (cvValues.GetSize() == 0)
		cMedian = lWorkingMemorySize / nbvar;
	else if (cvValues.GetSize() < 10)
		cMedian = lWorkingMemorySize / nbvar;
	// Cas ou il y a au moins deux valeurs
	else
	{
		// Tri des valeurs
		cvValues.Sort();

		// Calcul de la valeur mediane, selon la parite de la taille du tableau de valeurs
		if (cvValues.GetSize() % 2 == 0)
			cMedian =
			    (cvValues.GetAt(cvValues.GetSize() / 2 - 1) + cvValues.GetAt(cvValues.GetSize() / 2)) / 2;
		else
			cMedian = cvValues.GetAt(cvValues.GetSize() / 2);
	}

	return cMedian;
}

longint DTDecisionTreeCreationTaskSequential::ComputeMeanTreeSpecMemory(ObjectDictionary* odInputAttributeStats) const
{
	longint lResultMemorySize = 0;
	longint nbvar;
	ALString sIdentifier;
	POSITION position;
	Object* object;
	KWAttributeStats* attributeStats;
	KWDGSAttributePartition* attributePartition;

	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);
	nbvar = 0;
	position = odInputAttributeStats->GetStartPosition();
	while (position != NULL)
	{
		odInputAttributeStats->GetNextAssoc(position, sIdentifier, object);
		attributeStats = cast(KWAttributeStats*, object);
		attributePartition =
		    cast(KWDGSAttributePartition*, attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0));
		nbvar++;
		lResultMemorySize += attributePartition->GetUsedMemory();
	}
	return lResultMemorySize / nbvar;
}

void DTDecisionTreeCreationTaskSequential::UnloadNonInformativeAttributes(KWClass* kwclass,
									  ObjectDictionary* odInputAttributeStats)
{
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	ALString sIdentifier;
	POSITION position;
	Object* object;
	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);

	position = odInputAttributeStats->GetStartPosition();
	while (position != NULL)
	{
		odInputAttributeStats->GetNextAssoc(position, sIdentifier, object);
		attributeStats = cast(KWAttributeStats*, object);
		if (attributeStats->GetSortValue() == 0.0)
		{
			attribute = kwclass->LookupAttribute(attributeStats->GetAttributeName());
			attribute->SetLoaded(false);
		}
	}

	kwclass->Compile();
}

void DTDecisionTreeCreationTaskSequential::UnloadLessInformativeAttribute(KWClass* kwclass,
									  ObjectDictionary* odInputAttributeStats,
									  int nloadattribut)
{
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	int nattribute;
	ObjectArray oasorstat;
	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);
	require(nloadattribut >= 0);

	if (nloadattribut < odInputAttributeStats->GetCount())
	{
		odInputAttributeStats->ExportObjectArray(&oasorstat);

		// trie pour garantir les memes resultas en parallele

		oasorstat.SetCompareFunction(DTDecisionTreeNodeCompare);
		oasorstat.Sort();

		for (nattribute = nloadattribut; nattribute < oasorstat.GetSize(); nattribute++)
		{
			attributeStats = cast(KWAttributeStats*, oasorstat.GetAt(nattribute));

			attribute = kwclass->LookupAttribute(attributeStats->GetAttributeName());
			attribute->SetLoaded(false);
		}

		kwclass->Compile();
	}
}

void DTDecisionTreeCreationTaskSequential::LoadOnlySelectedAttributes(
    KWClass* kwcClass, DTAttributeSelectionsSlices* attributeselectionsSlices)
{
	KWAttribute* attribute;
	int nAttribute;
	ObjectArray* oaSortedAttribute;
	DTTreeAttribute* taAttribute;

	require(kwcClass != NULL);
	require(attributeselectionsSlices != NULL);
	require(attributeselectionsSlices->GetTreeAttributes()->GetSize() > 0);

	// On pase prealablement tous les attribut en Unload, ce qui est necessaire
	kwcClass->SetAllAttributesLoaded(false);

	oaSortedAttribute = attributeselectionsSlices->GetTreeAttributes();

	// Parametrage des attributs a charger

	for (nAttribute = 0; nAttribute < oaSortedAttribute->GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaSortedAttribute->GetAt(nAttribute));

		attribute = kwcClass->LookupAttribute(taAttribute->GetName());
		attribute->SetLoaded(true);
	}

	// Recompilation de la classe
	kwcClass->Compile();
}

void DTDecisionTreeCreationTaskSequential::LoadOnlyMostInformativeAttributes(KWClass* kwcClass,
									     ObjectDictionary* odInputAttributeStats,
									     int nMaxAttributeNumber)
{
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	int nAttribute;
	ObjectArray oaSortedAttributeStats;
	int nLoadedAttibuteNumber;

	require(kwcClass != NULL);
	require(odInputAttributeStats != NULL);
	require(odInputAttributeStats->GetCount() > 0);
	require(nMaxAttributeNumber >= 0);

	// On pase prealablement tous les attribut en Unload, ce qui est necessaire
	kwcClass->SetAllAttributesLoaded(false);

	// Export des attributs dans un tableau
	odInputAttributeStats->ExportObjectArray(&oaSortedAttributeStats);

	// Tri des attribut dans le cas ou il est necessaire de filtrer
	if (nMaxAttributeNumber < odInputAttributeStats->GetCount())
	{
		oaSortedAttributeStats.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedAttributeStats.Sort();
	}

	// Parametrage des attributs a charger
	nLoadedAttibuteNumber = min(nMaxAttributeNumber, oaSortedAttributeStats.GetSize());
	for (nAttribute = 0; nAttribute < nLoadedAttibuteNumber; nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaSortedAttributeStats.GetAt(nAttribute));

		attribute = kwcClass->LookupAttribute(attributeStats->GetAttributeName());
		attribute->SetLoaded(true);
	}

	// Recompilation de la classe
	kwcClass->Compile();
}

void DTDecisionTreeCreationTaskSequential::BuildAttributeSelectionsSlices(const ObjectArray* oaAttributeSelections,
									  ObjectArray* oaAttributeSelectionsSlices,
									  KWDataTableSliceSet* dataTableSliceSet,
									  int nMaxloadVariables) const
{
	int i;
	ObjectDictionary odSliceAttributes;
	DTAttributeSelection* attributeSelection;
	DTAttributeSelectionsSlices* attributeSelectionSlices;

	require(oaAttributeSelections != NULL);
	require(oaAttributeSelectionsSlices != NULL);
	require(oaAttributeSelectionsSlices->GetSize() == 0);

	// Alimentation d'un dictionnaire, dont les cles sont les noms des attributs et les objet sont les tranches
	// (KWDataTableSlice) les contenant
	dataTableSliceSet->FillSliceAttributes(&odSliceAttributes);

	attributeSelectionSlices = NULL;
	// Creation d'un objet KWAttributePairsSlices par paire d'attribut
	for (i = 0; i < oaAttributeSelections->GetSize(); i++)
	{
		attributeSelection = cast(DTAttributeSelection*, oaAttributeSelections->GetAt(i));
		require(attributeSelection->GetSize() <= nMaxloadVariables);

		// Creation de l'objet
		if (attributeSelectionSlices == NULL)
		{
			attributeSelectionSlices = new DTAttributeSelectionsSlices;
			oaAttributeSelectionsSlices->Add(attributeSelectionSlices);
		}
		if (attributeSelectionSlices->UnionAttributesCount(attributeSelection) <= nMaxloadVariables)
		{
			attributeSelectionSlices->AddAttributeSelection(attributeSelection, &odSliceAttributes);
		}
		else
		{
			attributeSelectionSlices = new DTAttributeSelectionsSlices;
			oaAttributeSelectionsSlices->Add(attributeSelectionSlices);
			if (attributeSelection->GetSize() <= nMaxloadVariables)
				attributeSelectionSlices->AddAttributeSelection(attributeSelection, &odSliceAttributes);
		}

		assert(attributeSelectionSlices->Check());
	}
}

KWLearningSpec*
DTDecisionTreeCreationTaskSequential::InitializeEqualFreqDiscretization(KWTupleTableLoader* tupleTableLoader,
									const KWLearningSpec* learningSpec)
{
	// transformer une regression en classification
	// on transforme la cible continue en cible categorielle

	KWTupleTable* targetTupleTableClassification = NULL;
	boolean bOk = true;
	DTBaseLoader bl;
	SymbolVector* svTargetValues = NULL;
	KWLearningSpec* newLearningSpec = NULL;
	KWClass* newClass = NULL;
	KWAttribute* newTarget = NULL;

	assert(tupleTableLoader->GetInputExtraAttributeType() == KWType::Continuous);

	if (randomForestParameter.GetDiscretizationTargetMethod() == DTForestParameter::DISCRETIZATION_EQUAL_FREQUENCY)
		svTargetValues = EqualFreqDiscretizeContinuousTarget(
		    tupleTableLoader, randomForestParameter.GetMaxIntervalsNumberForTarget());
	else
	{
		AddWarning("Discretization target method no yet implemented in sequential mode, assuming Equal Freq "
			   "discretization with " +
			   ALString(IntToString(randomForestParameter.GetMaxIntervalsNumberForTarget())) +
			   " intervals");
		svTargetValues = EqualFreqDiscretizeContinuousTarget(
		    tupleTableLoader, randomForestParameter.GetMaxIntervalsNumberForTarget());
	}

	assert(svTargetValues != NULL);

	newLearningSpec = learningSpec->Clone();
	newClass = learningSpec->GetClass()->Clone();
	newClass->SetName(newClass->GetName() + "_classification");
	newTarget = new KWAttribute;

	newTarget->SetName(learningSpec->GetTargetAttributeName() + "_categorical");
	newTarget->SetType(KWType::Symbol);
	newClass->InsertAttribute(newTarget);
	bOk = KWClassDomain::GetCurrentDomain()->InsertClass(newClass);
	assert(bOk);
	newClass->Compile();
	KWClassDomain::GetCurrentDomain()->Compile();
	assert(newClass->Check());

	newLearningSpec->SetClass(newClass);
	newLearningSpec->SetTargetAttributeName(newTarget->GetName());
	newLearningSpec->GetDatabase()->SetClassName(newClass->GetName());

	// modifier le tupleTableLoader
	tupleTableLoader->SetInputExtraAttributeName(newTarget->GetName());
	tupleTableLoader->SetInputExtraAttributeSymbolValues(svTargetValues);
	tupleTableLoader->SetInputExtraAttributeType(KWType::Symbol);

	tupleTableLoader->SetInputClass(newClass);
	tupleTableLoader->SetCheckDatabaseObjectClass(false);
	targetTupleTableClassification = cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable());
	bl.LoadTupleTableFromSymbolValues(newLearningSpec->GetClass(), newLearningSpec->GetTargetAttributeName(),
					  svTargetValues, targetTupleTableClassification);

	// calcul des stats cibles
	newLearningSpec->ComputeTargetStats(targetTupleTableClassification);

	bOk = bOk and newLearningSpec->Check();
	bOk = bOk and tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() <= 1;
	bOk = bOk and (tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() == 0 or
		       newLearningSpec->GetTargetAttributeName() != "");
	bOk = bOk and (newLearningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNameAt(0) ==
			   newLearningSpec->GetTargetAttributeName());
	bOk = bOk and (newLearningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeTypeAt(0) ==
			   newLearningSpec->GetTargetAttributeType());
	assert(bOk);

	return newLearningSpec;
}

SymbolVector*
DTDecisionTreeCreationTaskSequential::EqualFreqDiscretizeContinuousTarget(KWTupleTableLoader* tupleTableLoader,
									  const int nQuantileNumber) const
{
	SymbolVector* svTargetValues = NULL;
	ContinuousVector* cvTargetValues = NULL;
	KWQuantileIntervalBuilder quantileBuilder;
	boolean bDisplayValues = false;

	require(nQuantileNumber >= 1);

	// Initialisation
	cvTargetValues = tupleTableLoader->GetInputExtraAttributeContinuousValues()->Clone();
	require(cvTargetValues != NULL);
	cvTargetValues->Sort();
	quantileBuilder.InitializeValues(cvTargetValues);
	delete cvTargetValues;

	// Calcul des quantiles
	quantileBuilder.ComputeQuantiles(nQuantileNumber);

	svTargetValues = new SymbolVector;

	for (int i = 0; i < tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize(); i++)
	{
		// pour chaque valeur de target continue, determiner a quel intervale elle appartient
		Continuous cValue = tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetAt(i);

		for (int nInterval = 0; nInterval < quantileBuilder.GetIntervalNumber(); nInterval++)
		{
			if (cValue >= quantileBuilder.GetIntervalLowerBoundAt(nInterval) and
			    cValue < quantileBuilder.GetIntervalUpperBoundAt(nInterval))
			{
				ALString s = "I" + ALString(IntToString(nInterval));
				svTargetValues->Add(s.GetBuffer(s.GetLength()));
				break;
			}
		}
	}

	assert(svTargetValues != NULL);

	if (bDisplayValues)
	{
		for (int i = 0; i < svTargetValues->GetSize(); i++)
		{
			cout << svTargetValues->GetAt(i) << endl;
			if (i > 40)
			{
				cout << "..." << endl;
				break;
			}
		}
	}
	return svTargetValues;
}
