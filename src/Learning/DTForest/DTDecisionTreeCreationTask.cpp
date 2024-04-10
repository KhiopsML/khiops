// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeCreationTask.h"
#include "DTDecisionTreeCreationTask.h"
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
#include "DTBaseLoader.h"

// #define GENERATE_PYTHON_REPORTING_TRACES // a definir si on veut generer des rapports de tests de performances a
// partir de notre code python, en parsant la sortie de la fenetre de log

int DTTreeSpecsCompareNames(const void* elem1, const void* elem2);
int DTLearningReportCompareSortName(const void* elem1, const void* elem2);

DTDecisionTreeCreationTask::DTDecisionTreeCreationTask()
{
	bIsTraining = false;

	// Variables partagees
	shared_odAttributeStats = new PLShared_ObjectDictionary(new PLShared_AttributeStats);
	DeclareSharedParameter(shared_odAttributeStats);
	DeclareSharedParameter(&shared_ivDataTableSliceSetChunkInstanceNumbers);

	// Variables en entree et sortie des esclaves
	DeclareTaskInput(&input_svAttributeNames);

	input_oaDataTableSlices = new PLShared_ObjectArray(new PLShared_DataTableSlice);
	DeclareTaskInput(input_oaDataTableSlices);

	input_oaAttributeSelections = new PLShared_ObjectArray(new PLShared_AttributeSelection);
	DeclareTaskInput(input_oaAttributeSelections);

	input_forestParameter = new PLShared_ForestParameter;
	DeclareTaskInput(input_forestParameter);

	DeclareTaskInput(&input_cvIntervalValues);
	DeclareTaskInput(&input_ivSplitValues);

	output_oaAttributeStats = new PLShared_ObjectArray(new PLShared_AttributeStats);
	DeclareTaskOutput(output_oaAttributeStats);

	output_oaDecisionTreeSpecs = new PLShared_ObjectArray(new PLShared_DecisionTreeSpec);
	DeclareTaskOutput(output_oaDecisionTreeSpecs);

	// Initialisation des variables du maitre
	oaMasterOutputAttributeStats = NULL;
	odMasterInputAttributeStats = NULL;
	masterTupleTableLoader = NULL;
	nMasterDatabaseObjectNumber = 0;

	oaMasterAllDecisionTreeSpecs = new ObjectArray;
	forestattributeselection = NULL;

	lMasterAllResultsMeanMemory = 0;
	lMasterMeanTreeSpecMemory = 0;
	lMasterMedianStatMemory = 0;
	lMasterTreeResultMeanMemory = 0;
	lMasterOneAttributeValueMemory = 0;
	lMasterTreeWorkingMemory = 0;
	lMasterEmptyObjectSize = 0;
	nMasterForestMaxAttributesSelectionNumber = 0;

	bMasterTraceOn = false; // true si on veut tracer le master dans la sortie standard
}

DTDecisionTreeCreationTask::~DTDecisionTreeCreationTask()
{
	delete shared_odAttributeStats;
	delete input_oaDataTableSlices;
	delete input_oaAttributeSelections;
	delete input_forestParameter;
	delete output_oaAttributeStats;
	delete output_oaDecisionTreeSpecs;
	delete oaMasterAllDecisionTreeSpecs;

	if (forestattributeselection != NULL)
		delete forestattributeselection;
}

boolean DTDecisionTreeCreationTask::CreatePreparedAttributes(KWLearningSpec* learningSpec,
							     KWTupleTableLoader* tupleTableLoader,
							     KWDataTableSliceSet* dataTableSliceSet,
							     ObjectDictionary* odInputAttributeStats,
							     ObjectArray* oaOutputAttributeStats)
{
	boolean bOk = true;
	KWClass* kwcUpdatedClass;
	ALString sMessage;
	int oldseed;
	int nInputInformativeVariable;
	int nAttribute;
	ObjectArray oaAttributeStats;
	KWAttributeStats* attributeStats;
	require(learningSpec != NULL);
	require(oaMasterOutputAttributeStats == NULL);
	require(shared_odAttributeStats->GetObjectDictionary()->GetCount() == 0);
	require(CheckInputParameters(learningSpec, tupleTableLoader->GetInputExtraAttributeTupleTable()));
	require(CheckPreparedAttributes(learningSpec, odInputAttributeStats, oaOutputAttributeStats));
	require(odInputAttributeStats != NULL and odInputAttributeStats->GetCount() > 0);
	require(oaOutputAttributeStats != NULL and oaOutputAttributeStats->GetSize() == 0);
	Timer DTTimer_CreatePreparedAttributes;

	if (bMasterTraceOn)
		cout << "DTDecisionTreeCreationTask::CreatePreparedAttributes" << endl;

	require(nMaxCreatedAttributeNumber > 0);

	if (learningSpec->GetTargetAttributeType() == KWType::None)
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("No tree building : no target variable");
		return true;
	}

	if (GetForestExpertMode() == false and learningSpec->GetTargetAttributeType() != KWType::Symbol)
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("No tree building : target variable is not categorical");
		return true;
	}

	if (learningSpec->GetTargetAttributeType() == KWType::Symbol and
	    learningSpec->GetPreprocessingSpec()->GetTargetGrouped())
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("No tree building : target variable is grouped");
		return true;
	}

	if (tupleTableLoader->GetInputExtraAttributeTupleTable()->GetSize() == 1)
	{
		if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
			AddWarning("No tree building : the target variable contains only one value");
		return true;
	}

	if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
	{
		DTTimer_CreatePreparedAttributes.Start();
	}

	// test le nombre de variable informative
	//  si egal a zero on ne peut pas contruire d'arbre
	odInputAttributeStats->ExportObjectArray(&oaAttributeStats);
	nInputInformativeVariable = 0;
	for (nAttribute = 0; nAttribute < oaAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(nAttribute));
		if (attributeStats->GetLevel() > 0)
			nInputInformativeVariable++;
	}
	if (nInputInformativeVariable == 0)
	{
		AddWarning("No tree building : no informative input variable");
		return true;
	}

	odMasterInputAttributeStats = odInputAttributeStats;
	masterTupleTableLoader = tupleTableLoader;
	masterDataTableSliceSet = dataTableSliceSet;

	// Nettoyage du rapport de creation existant
	masterOutputCreationReport.Clean();

	if (forestattributeselection != NULL)
		delete forestattributeselection;

	forestattributeselection = new DTForestAttributeSelection;

	oldseed = GetRandomSeed();
	SetRandomSeed(1972);

	kwcUpdatedClass = learningSpec->GetClass();
	kwcUpdatedClass->LookupAttribute(learningSpec->GetTargetAttributeName())->SetLoaded(false);
	kwcUpdatedClass->Compile();

	// Parametrage de la collecte des resultats par le maitre
	oaMasterOutputAttributeStats = oaOutputAttributeStats;

	// Lancement de la tache
	bOk = RunDataPreparationTask(learningSpec, tupleTableLoader, dataTableSliceSet);

	if (not bOk)
		oaOutputAttributeStats->DeleteAll();

	// Nettoyage des donnees de pilotage de la tache
	CleanTaskInputs();

	// Nettoyage des variables du maitre
	oaMasterOutputAttributeStats = NULL;

	if (bMasterTraceOn)
	{
		if (oaOutputAttributeStats->GetSize() > 0)
		{
			cout << endl << "\t--> CreatePreparedAttributes results :" << endl;
			for (int i = 0; i < oaOutputAttributeStats->GetSize(); i++)
			{
				KWAttributeStats* stats = cast(KWAttributeStats*, oaOutputAttributeStats->GetAt(i));
				cout << stats->GetAttributeName() << endl;
			}
		}
		else
		{
			cout << endl
			     << "\t--> CreatePreparedAttributes : no trees computed (no informative tree)" << endl;
		}
	}

	// restitution de l'etat initial :

	SetRandomSeed(oldseed);

	kwcUpdatedClass->LookupAttribute(learningSpec->GetTargetAttributeName())->SetLoaded(true);
	kwcUpdatedClass->Compile();

	if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
	{
		DTTimer_CreatePreparedAttributes.Stop();
		AddSimpleMessage(ALString("CreatePreparedAttributes time: \t") +
				 DoubleToString(DTTimer_CreatePreparedAttributes.GetElapsedTime()));
	}

	if (bMasterTraceOn)
		cout << endl << "FIN de DTDecisionTreeCreationTask::CreatePreparedAttributes" << endl;

	return bOk;
}

// *************************************************************************************************************************
//                                         methodes du process maitre *
// *************************************************************************************************************************

boolean DTDecisionTreeCreationTask::MasterInitialize()
{
	boolean bOk = true;
	int nAttributesNumberForOneSlave = 0;
	const RMTaskResourceGrant* grantedResources = 0;

	if (bMasterTraceOn)
		cout << endl << "MasterInitialize" << endl;

	bOk = KWDataPreparationTask::MasterInitialize();

	if (bOk)
	{
		grantedResources = PLParallelTask::GetTaskResourceGrant();

		masterOutputCreationReport.Clean();
		oaMasterAllDecisionTreeSpecs->DeleteAll();

		nAttributesNumberForOneSlave = ComputeOneSlaveMaxLoadableAttributeNumber(
		    grantedResources->GetSlaveNumber(), grantedResources->GetSlaveMemory());

		if (nAttributesNumberForOneSlave < nMasterForestMaxAttributesSelectionNumber)
		{
			AddWarning("No tree built : system resources are not sufficient to run the task");
			return false;
		}

		BuildAttributeSelectionsSlices(forestattributeselection->GetAttributeSelections(),
					       &oaInputAttributeSelectionsSlices, nAttributesNumberForOneSlave,
					       grantedResources->GetSlaveNumber());

		oaInputAttributeSelectionsSlices.SetCompareFunction(DTAttributeSelectionsSlicesCompareSlices);
		oaInputAttributeSelectionsSlices.Sort();
		oaInputAttributeSelectionsSlices.SetCompareFunction(
		    NULL); // raz pour permettre la serialisation et deserialisation

#ifdef GENERATE_PYTHON_REPORTING_TRACES
		ALString sMessage;
		sMessage += "\n\nShared memory = \t" +
			    ALString(LongintToHumanReadableString(grantedResources->GetSharedMemory()));
		sMessage += "\nMaster memory = \t" +
			    ALString(LongintToHumanReadableString(grantedResources->GetMasterMemory()));
		sMessage +=
		    "\nSlave memory = \t" + ALString(LongintToHumanReadableString(grantedResources->GetSlaveMemory()));
		sMessage += "\nForest selected attributes number = \t" +
			    ALString(IntToString(nMasterForestMaxAttributesSelectionNumber));
		sMessage +=
		    "\nMax loadable attributes for 1 slave  = \t" + ALString(IntToString(nAttributesNumberForOneSlave));
		sMessage += "\nSlices number (1 by slave) = \t" +
			    ALString(IntToString(oaInputAttributeSelectionsSlices.GetSize()));
		sMessage += "\nGranted Slaves Number = \t" + ALString(IntToString(grantedResources->GetSlaveNumber()));
		sMessage += "\n\n";
		AddSimpleMessage(sMessage);
#endif
	}

	if (bMasterTraceOn)
		cout << endl << "FIN MasterInitialize" << endl;

	return bOk;
}
boolean DTDecisionTreeCreationTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk = true;
	boolean bLocalTrace = false;
	DTAttributeSelectionsSlices* attributeSelectionsSlices;
	DTTreeAttribute* attribute;
	KWDataTableSlice* slice;
	int i;

	if (bMasterTraceOn)
		cout << endl << "DTDecisionTreeCreationTask::MasterPrepareTaskInput" << endl;

	// Recherche de la prochaine tranche de selections d'attributs a analyser
	attributeSelectionsSlices = NULL;
	dTaskPercent = 0;

	bIsTaskFinished = (GetTaskIndex() >= oaInputAttributeSelectionsSlices.GetSize());

	if (oaMasterOutputAttributeStats->GetSize() >= nMaxCreatedAttributeNumber)
		bIsTaskFinished = true;

	if (not bIsTaskFinished)
	{
		input_forestParameter->SetForestParameter(randomForestParameter.Clone());

		// On prend les groupes de selections  en ordre inverse, pour avoir les plus grosses au debut
		attributeSelectionsSlices = cast(DTAttributeSelectionsSlices*,
						 oaInputAttributeSelectionsSlices.GetAt(
						     oaInputAttributeSelectionsSlices.GetSize() - 1 - GetTaskIndex()));
		dTaskPercent = attributeSelectionsSlices->GetAttributeSelections()->GetSize() * 1.0 /
			       oaInputAttributeSelectionStats.GetSize();

		/////////////////////////////////////////////////////////////////////////
		// Recopie dans les variables partagee en entree de tache des parametres
		// specifiees dans la tranche des selections a analyser

		for (i = 0; i < attributeSelectionsSlices->GetAttributeSelections()->GetSize(); i++)
		{
			DTAttributeSelection* attributeSelection =
			    cast(DTAttributeSelection*, attributeSelectionsSlices->GetAttributeSelections()->GetAt(i));
			input_oaAttributeSelections->GetObjectArray()->Add(attributeSelection->Clone());
		}

		// Noms des variables impliquees
		for (i = 0; i < attributeSelectionsSlices->GetTreeAttributes()->GetSize(); i++)
		{
			attribute = cast(DTTreeAttribute*, attributeSelectionsSlices->GetTreeAttributes()->GetAt(i));
			input_svAttributeNames.Add(attribute->GetName());
		}

		// liste des coupures effectuees sur une cible continue (toutes variables confondues), en amont
		if (shared_learningSpec.GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous and
		    input_cvIntervalValues.GetSize() == 0)
		{
			// reference des coupure MODL et DISCRETIZATION_BINARY_EQUAL_FREQUENCY
			ReferenceTargetIntervalValues(odMasterInputAttributeStats);
		}
		// initialisation pour chaque selection du split pour la regression
		// DISCRETIZATION_BINARY_EQUAL_FREQUENCY pour chaque selection on choisi un nombre alétoire de la
		// coupure allant de 0 à nb_coupure-2
		if (shared_learningSpec.GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous and
		    randomForestParameter.GetDiscretizationTargetMethod() ==
			DTForestParameter::DISCRETIZATION_BINARY_EQUAL_FREQUENCY)
		{
			IntVector ivTemp;
			ivTemp.SetSize(nMaxCreatedAttributeNumber);
			int nMax = input_cvIntervalValues.GetContinuousVector()->GetSize() - 1;

			for (i = 0; i < nMaxCreatedAttributeNumber; i++)
			{
				ivTemp.SetAt(i, RandomInt(nMax - 1));
			}
			input_ivSplitValues.GetIntVector()->CopyFrom(&ivTemp);
		}

		// Tranches elementaires impliques dans les arbres
		// On met une copie de la specification de la tranche
		// (la variable en entree a une duree temporaire et il faut garder
		// la specification initiale de l'ensemble des tranches)
		POSITION position = attributeSelectionsSlices->GetSlices()->GetStartPosition();
		NUMERIC key;
		Object* obj;
		while (position != NULL)
		{
			attributeSelectionsSlices->GetSlices()->GetNextAssoc(position, key, obj);
			slice = cast(KWDataTableSlice*, obj);
			input_oaDataTableSlices->GetObjectArray()->Add(slice->Clone());
		}
	}

	// Affichage des caracteristiques des selections d'atrtributs a traiter
	if (bLocalTrace and bMasterTraceOn)
	{
		cout << GetTaskIndex() << "\t";
		if (attributeSelectionsSlices != NULL)
			attributeSelectionsSlices->Write(cout);
		else
			cout << endl;
	}
	return bOk;
}
boolean DTDecisionTreeCreationTask::MasterAggregateResults()
{
	boolean bOk = true;
	int nTreeSpec;
	DTDecisionTreeSpec* decisionTreeSpec;
	KWAttributeStats* attributeStats;

	if (bMasterTraceOn)
	{
		cout << endl
		     << "******************************************************" << endl
		     << "DTDecisionTreeCreationTask::MasterAggregateResults()" << endl;

		if (output_oaAttributeStats->GetObjectArray()->GetSize() == 0)
			cout << "No attribute stats have been computed" << endl;
	}

	// Transfer des preparations d'attribut de l'esclave vers le dictionnaire global du maitre
	for (int nPair = 0; nPair < output_oaAttributeStats->GetObjectArray()->GetSize(); nPair++)
	{
		attributeStats = cast(KWAttributeStats*, output_oaAttributeStats->GetObjectArray()->GetAt(nPair));

		// On reparametre le learningSpec par celui du maitre
		attributeStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());

		// Memorisation dans le dictionnaire global
		oaMasterOutputAttributeStats->Add(attributeStats);

		if (bMasterTraceOn)
		{
			cout << endl
			     << "Master is aggregating attribute stats " << attributeStats->GetAttributeName()
			     << " with level " << attributeStats->GetLevel() << endl;
			// attributeStats->WriteReport(cout);
		}
	}
	output_oaAttributeStats->GetObjectArray()->RemoveAll();

	if (bMasterTraceOn)
	{
		if (output_oaDecisionTreeSpecs->GetObjectArray()->GetSize() == 0)
			cout << "No tree spec have been computed" << endl;
		else
			cout << endl << "Corresponding tree specs : " << endl;
	}

	// Transfer des arbres de decision calcules par l'esclave, vers le dictionnaire global du maitre
	for (nTreeSpec = 0; nTreeSpec < output_oaDecisionTreeSpecs->GetObjectArray()->GetSize(); nTreeSpec++)
	{
		decisionTreeSpec =
		    cast(DTDecisionTreeSpec*, output_oaDecisionTreeSpecs->GetObjectArray()->GetAt(nTreeSpec));

		// Memorisation dans le dictionnaire global
		oaMasterAllDecisionTreeSpecs->Add(decisionTreeSpec);

		if (bMasterTraceOn)
		{
			cout << endl
			     << decisionTreeSpec->GetTreeVariableName()
			     << ", leaves number = " << decisionTreeSpec->GetLeavesNumber()
			     << ", variables number = " << decisionTreeSpec->GetVariablesNumber()
			     << ", construction cost = " << decisionTreeSpec->GetConstructionCost() << endl;
		}
	}
	output_oaDecisionTreeSpecs->GetObjectArray()->RemoveAll();

	return bOk;
}

boolean DTDecisionTreeCreationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	KWClass* kwcUpdatedClass;
	NumericKeyDictionary ndTreeSingleton;
	NUMERIC key;
	ALString sTmp;

	masterOutputCreationReport.Clean();

	// Appel de la methode ancetre
	bOk = bOk and KWDataPreparationTask::MasterFinalize(bProcessEndedCorrectly);

	// Finalisation des resultats : renseigner le rapport avec les arbres calcules par les esclaves
	if (bOk)
	{
		assert(oaMasterOutputAttributeStats->GetSize() == oaMasterAllDecisionTreeSpecs->GetSize());

		// Tri par nom des KWAttributeStats, pour assurer la reproductibilite de l'ordre des resultats
		oaMasterOutputAttributeStats->SetCompareFunction(DTLearningReportCompareSortName);
		oaMasterOutputAttributeStats->Sort();

		// alimenter une structure avec recherche par cle pour ameliorer les perfs
		ObjectDictionary odMasterOutputAttributeStats;
		for (int i = 0; i < oaMasterOutputAttributeStats->GetSize(); i++)
		{
			KWAttributeStats* attributeStats =
			    cast(KWAttributeStats*, oaMasterOutputAttributeStats->GetAt(i));
			odMasterOutputAttributeStats.SetAt(attributeStats->GetAttributeName(), attributeStats);
		}

		/* parcourir les DTDecisionTreeSpec pour :
			- ajouter les arbres au rapport, afin de permettre ulterieurement l'ecriture du rapport JSON
			- recreer les attributs correspondants aux arbres et les ajouter dans le dictionnaire initial,
		   pour prise en compte dans le modele
		*/
		kwcUpdatedClass = shared_learningSpec.GetLearningSpec()->GetClass();

		// Tri par nom des arbres, pour assurer la reproductibilite de l'ordre des resultats
		oaMasterAllDecisionTreeSpecs->SetCompareFunction(DTTreeSpecsCompareNames);
		oaMasterAllDecisionTreeSpecs->Sort();

		for (int i = 0; i < oaMasterAllDecisionTreeSpecs->GetSize(); i++)
		{
			DTDecisionTreeSpec* treeSpec =
			    cast(DTDecisionTreeSpec*, oaMasterAllDecisionTreeSpecs->GetAt(i));

			KWAttributeStats* attributeStats = cast(
			    KWAttributeStats*, odMasterOutputAttributeStats.Lookup(treeSpec->GetTreeVariableName()));
			assert(attributeStats != NULL);

			key = treeSpec->ComputeHashValue();

			// filtrer les arbres qui sont similaires
			if (ndTreeSingleton.Lookup(key) == NULL)
			{
				masterOutputCreationReport.AddTree(treeSpec->GetTreeVariableName(), treeSpec);
				masterOutputCreationReport.SetLevelAt(masterOutputCreationReport.GetTreeNumber() - 1,
								      attributeStats->GetLevel());
				KWAttribute* attribute = treeSpec->BuildAttribute(treeSpec->GetTreeVariableName());
				attribute->CompleteTypeInfo(kwcUpdatedClass);
				SetConstructedAttributeCost(
				    attribute, treeSpec->GetConstructionCost() +
						   shared_learningSpec.GetLearningSpec()->GetSelectionCost());
				kwcUpdatedClass->InsertAttribute(attribute);
				ndTreeSingleton.SetAt(key, attribute);
			}
			else
			{
				if (bMasterTraceOn)
				{
					KWAttribute* a = cast(KWAttribute*, ndTreeSingleton.Lookup(key));
					cout << "MasterFinalize : Tree " << treeSpec->GetTreeVariableName()
					     << " is similar to tree " << a->GetName() << ", so it will be ignored."
					     << endl;
				}

				delete treeSpec;
				oaMasterAllDecisionTreeSpecs->SetAt(i, NULL);
			}
		}
		kwcUpdatedClass->Compile();

		// supprimer les KWAttributeStats correspondant aux arbres doublons
		for (int i = 0; i < oaMasterOutputAttributeStats->GetSize(); i++)
		{
			KWAttributeStats* attributeStats =
			    cast(KWAttributeStats*, oaMasterOutputAttributeStats->GetAt(i));
			if (kwcUpdatedClass->LookupAttribute(attributeStats->GetAttributeName()) == NULL)
			{
				delete attributeStats;
				oaMasterOutputAttributeStats->RemoveAt(i);
				i = -1; // recommencer la boucle
			}
		}

		// Parametrage des classStats pour specialiser le contenu des rapports
		masterOutputCreationReport.SetClassStats(GetClassStats());

		// message et warning final
		if (oaMasterOutputAttributeStats->GetSize() > 0)
		{
			if (oaMasterOutputAttributeStats->GetSize() < nMaxCreatedAttributeNumber)
				AddSimpleMessage(sTmp + "Computation of " +
						 IntToString(oaMasterOutputAttributeStats->GetSize()) +
						 " distinct decision trees (out of " +
						 IntToString(nMaxCreatedAttributeNumber) + " planned)");
			else
				AddSimpleMessage(sTmp + "Computation of " +
						 IntToString(oaMasterOutputAttributeStats->GetSize()) +
						 " distinct decision trees");
		}
		else
			AddWarning(sTmp + "No informative tree built among the " +
				   IntToString(nMaxCreatedAttributeNumber) + " planned");
	}
	else
	{
		// netoyage si bOk = False exemple interuption dans master initialise ou dans un slave process
		oaMasterAllDecisionTreeSpecs->DeleteAll();
		oaMasterOutputAttributeStats->DeleteAll();
	}
	return bOk;
}

// *************************************************************************************************************************
//                                         methodes du process esclave *
// *************************************************************************************************************************

boolean DTDecisionTreeCreationTask::SlaveInitialize()
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

boolean DTDecisionTreeCreationTask::SlaveProcess()
{
	boolean bLocalTrace = false;
	KWDataTableSlice* slice = NULL;
	KWDataTableSliceSet dataTableSliceSet;
	KWClass* kwcSliceSetClass = NULL;
	KWSTDatabaseTextFile sliceDatabase;
	int i;
	boolean bOk = true;
	int nAttribute;
	KWAttribute* createdAttribute;
	ObjectArray oaCreatedAttributes;
	ObjectArray* oaInputAttributeStats = NULL;
	DTDecisionTree* dttree = NULL;
	DTBaseLoader blOrigine;
	DTDecisionTreeSpec* reportTreeSpec = NULL;
	KWLearningSpec* learningSpecTree = NULL;
	KWLearningSpec* slaveLearningSpec = NULL;
	int nNativeVariableNumber;
	ALString svariablename;
	ALString sTmp;
	ALString sMessage;
	KWAttribute* attribute = NULL;
	KWAttributeStats* attributeStats = NULL;
	ObjectArray oaObjects;
	ObjectArray oatupletable;
	ObjectArray oaOrigineAttributs;
	ObjectArray oaZeroLevelAttributs;
	KWTupleTable* ttattribut = NULL;
	NumericKeyDictionary ndTreeSingleton;
	NUMERIC key;
	DTAttributeSelection* attributegenerator;
	int nBuidTreeNumber;
	int oldseedtree;

	if (randomForestParameter.GetDecisionTreeParameter()->GetVerboseMode())
	{
		Global::ActivateErrorFlowControl();
		Global::SetMaxErrorFlowNumber(10000);
	}

#ifdef GENERATE_PYTHON_REPORTING_TRACES
	const int oldMaxErrorFlowNumber = Global::GetMaxErrorFlowNumber();
	Global::ActivateErrorFlowControl();
	Global::SetMaxErrorFlowNumber(10000);
	Timer DTTimer_ComputeDecisionTree;
	Timer DTTimer_SlaveProcess;
#endif

	// Affichage des inputs de la tache
	if (bLocalTrace)
	{
		cout << "Task index (SlaveProcess) : " << GetTaskIndex() << endl;
		cout << "  Variables\t" << input_svAttributeNames.GetConstStringVector()->GetSize() << endl;
		for (i = 0; i < input_svAttributeNames.GetConstStringVector()->GetSize(); i++)
		{
			cout << "\t" << input_svAttributeNames.GetConstStringVector()->GetAt(i) << endl;
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

	// Tri des slices selon leur ordre lexicographique, pour rendre le sliceset resultant utilisable
	dataTableSliceSet.GetSlices()->SetCompareFunction(KWDataTableSliceCompareLexicographicIndex);
	dataTableSliceSet.GetSlices()->Sort();

	// On reparametre les nombres d'instances par chunk
	dataTableSliceSet.GetChunkInstanceNumbers()->CopyFrom(
	    shared_ivDataTableSliceSetChunkInstanceNumbers.GetConstIntVector());
	dataTableSliceSet.UpdateTotalInstanceNumber();

	// Construction d'une classe specifique pour lire les attributs impliques dans les arbres
	kwcSliceSetClass = dataTableSliceSet.BuildClassFromAttributeNames(
	    KWClassDomain::GetCurrentDomain()->BuildClassName(sTmp + "SlaveTreesBuilding" +
							      IntToString(GetTaskIndex() + 1)),
	    input_svAttributeNames.GetConstStringVector());

	// Preparation de classe permettant l'analyse des arbres
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

	// Lecture des objets avec tous les attributs impliques
	bOk = dataTableSliceSet.ReadAllObjectsWithClass(kwcSliceSetClass, &oaObjects);

	// Verification minimum de coherence
	if (bOk and shared_learningSpec.GetLearningSpec()->GetInstanceNumber() != oaObjects.GetSize())
	{
		bOk = false;
		AddError(
		    "Data table slices corrupted " + FileService::GetURIUserLabel(sliceDatabase.GetDatabaseName()) +
		    "\n\t" + IntToString(oaObjects.GetSize()) + " read records for " +
		    IntToString(shared_learningSpec.GetLearningSpec()->GetInstanceNumber()) + " expected instances");
	}

#ifdef GENERATE_PYTHON_REPORTING_TRACES
	AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
			 IntToString(GetTaskIndex() + 1) + "\t" + ALString("Instances number : \t") +
			 IntToString(shared_learningSpec.GetLearningSpec()->GetInstanceNumber()));
	AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
			 IntToString(GetTaskIndex() + 1) + "\t" + ALString("Variables number : \t") +
			 IntToString(input_svAttributeNames.GetSize()));
	AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
			 IntToString(GetTaskIndex() + 1) + "\t" + ALString("Grant memory : \t") +
			 ALString(LongintToHumanReadableString(GetSlaveResourceGrant()->GetMemory())));
	DTTimer_SlaveProcess.Start();
#endif

	KWClass* kwcUpdatedClass = shared_learningSpec.GetLearningSpec()->GetClass();

	randomForestParameter.CopyFrom(input_forestParameter->GetForestParameter());

	nBuidTreeNumber = 0;

	// Calcul des arbres
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		// Parametrage du chargeur de tuple
		slaveTupleTableLoader.SetInputClass(kwcSliceSetClass);
		slaveTupleTableLoader.SetInputDatabaseObjects(&oaObjects);

		bOk = not TaskProgression::IsInterruptionRequested();
		if (bOk)
		{
			const boolean bRegressionWithEqualFreqDiscretization =
			    (slaveTupleTableLoader.GetInputExtraAttributeType() == KWType::Continuous and
				     randomForestParameter.GetDiscretizationTargetMethod() ==
					 DTForestParameter::DISCRETIZATION_EQUAL_FREQUENCY
				 ? true
				 : false);

			const boolean bRegressionWithMODLDiscretization =
			    (slaveTupleTableLoader.GetInputExtraAttributeType() == KWType::Continuous and
				     (randomForestParameter.GetDiscretizationTargetMethod() ==
					  DTForestParameter::DISCRETIZATION_MODL or
				      randomForestParameter.GetDiscretizationTargetMethod() ==
					  DTForestParameter::DISCRETIZATION_BINARY_EQUAL_FREQUENCY)
				 ? true
				 : false);

			if (bRegressionWithEqualFreqDiscretization or bRegressionWithMODLDiscretization)
				// modifier le learningSpec (attribut cible et dictionnaire) afin de transformer une
				// regression en probleme de classification
				slaveLearningSpec =
				    InitializeRegressionLearningSpec(shared_learningSpec.GetLearningSpec());
			else
				slaveLearningSpec = shared_learningSpec.GetLearningSpec();

			if (bRegressionWithEqualFreqDiscretization)
				// le slaveTupleTableLoader va etre modifie et les stats cibles du learningSpec vont
				// etre recalculees cette modification s'applique a tous les arbres qui seront calcules
				// par l'esclave
				InitializeEqualFreqDiscretization(&slaveTupleTableLoader, slaveLearningSpec);

			if (not bRegressionWithMODLDiscretization)
			{
				blOrigine.Initialize(slaveLearningSpec, &slaveTupleTableLoader, &oaObjects);

				// creation d'un learningspec dedie a tous les arbres de cet esclave
				learningSpecTree = new KWLearningSpec;
				learningSpecTree->CopyFrom(slaveLearningSpec);

				oaInputAttributeStats = BuildRootAttributeStats(
				    learningSpecTree, &blOrigine, shared_odAttributeStats->GetObjectDictionary());
			}

			// Calcul du nombre de variables initiales
			nNativeVariableNumber = slaveLearningSpec->GetInitialAttributeNumber();

			// creation pour un slice de selection :
			// - des arbres dttree
			// - des rapport KWAttributeStats
			// - des rapport json
			// - des specification des arbres

			bOk = not TaskProgression::IsInterruptionRequested();

			if (bOk)
			{
				TaskProgression::DisplayMainLabel("Trees building and stats computing");
				TaskProgression::DisplayLabel("");

				// Creation d'attributs
				int nMaxCreatedAttributeNumberSlice =
				    input_oaAttributeSelections->GetObjectArray()->GetSize();

				// trier les selections d'attributs par ordre d index croissants, afin de rendre les
				// sorties identiques
				input_oaAttributeSelections->GetObjectArray()->SetCompareFunction(
				    DTAttributeSelectionCompareAttributesIndex);
				input_oaAttributeSelections->GetObjectArray()->Sort();

				// Creation des attributs arbres
				for (nAttribute = 0; nAttribute < nMaxCreatedAttributeNumberSlice; nAttribute++)
				{
					// Progression pour la premiere etape du slave process (entre 0% et 50%)
					TaskProgression::DisplayProgression(((nAttribute + 1) * 50) /
									    nMaxCreatedAttributeNumberSlice);

					attributegenerator =
					    cast(DTAttributeSelection*,
						 input_oaAttributeSelections->GetObjectArray()->GetAt(nAttribute));
					attributegenerator->InitializeTreeAttributesFromClass(kwcSliceSetClass);

					if (bRegressionWithMODLDiscretization)
					{
						// dans ce cas, la discretisation est effectuee selon des coupures
						// potentiellement differentes pour chaque arbre : le
						// slaveTupleTableLoader va etre modifie pour chaque arbre et les stats
						// cibles du learningSpec vont etre recalculees
						if (randomForestParameter.GetDiscretizationTargetMethod() ==
						    DTForestParameter::DISCRETIZATION_MODL)
							InitializeMODLDiscretization(
							    &slaveTupleTableLoader, slaveLearningSpec,
							    *input_cvIntervalValues.GetConstContinuousVector());
						else
							InitializeBinaryEQFDiscretization(
							    &slaveTupleTableLoader, slaveLearningSpec,
							    *input_cvIntervalValues.GetConstContinuousVector(),
							    input_ivSplitValues.GetAt(attributegenerator->GetIndex()));

						blOrigine.Initialize(slaveLearningSpec, &slaveTupleTableLoader,
								     &oaObjects);

						// creation d'un learningspec dedie a cet arbre
						if (learningSpecTree != NULL)
							delete learningSpecTree; // nettoyage de la config de l'arbre
										 // precedent, si necessaire
						learningSpecTree = new KWLearningSpec;
						learningSpecTree->CopyFrom(slaveLearningSpec);

						if (oaInputAttributeStats != NULL)
						{
							oaInputAttributeStats->DeleteAll();
							delete oaInputAttributeStats;
						}
						oaInputAttributeStats = BuildRootAttributeStats(
						    learningSpecTree, &blOrigine,
						    shared_odAttributeStats->GetObjectDictionary());
					}

					nBuidTreeNumber++;

					oldseedtree = GetRandomSeed();
					SetRandomSeed(attributegenerator->GetSeed());

					if (TaskProgression::IsInterruptionRequested())
					{
						bOk = false;
						break;
					}

					// initialisation de l'arbre
					dttree =
					    CreateDecisionTree(learningSpecTree, &slaveTupleTableLoader, &oaObjects,
							       oaInputAttributeStats, attributegenerator);

#ifdef GENERATE_PYTHON_REPORTING_TRACES
					DTTimer_ComputeDecisionTree.Start();
#endif

					if (TaskProgression::IsInterruptionRequested())
					{
						bOk = false;
						break;
					}

					// Calcul de l'arbre
					if (not dttree or not ComputeTree(learningSpecTree, &blOrigine, dttree))
					{
						bOk = false;

						if (dttree)
						{
							delete dttree;
							dttree = NULL;
						}

#ifdef GENERATE_PYTHON_REPORTING_TRACES
						DTTimer_ComputeDecisionTree.Stop();
#endif
						break;
					}
#ifdef GENERATE_PYTHON_REPORTING_TRACES
					DTTimer_ComputeDecisionTree.Stop();
#endif

					if (bOk)
					{
						// Creation de la spec de l'arbre a partir de l'arbre calcule
						reportTreeSpec = new DTDecisionTreeSpec;
						reportTreeSpec->InitFromDecisionTree(dttree);
						// detection des doublons
						key = reportTreeSpec->ComputeHashValue();
						// filtre les arbre qui
						//  1 sont similaire key exist deja
						//  2 au moins une profondeur de 3 (2 niveau de groupage discretisation)
						if (ndTreeSingleton.Lookup(key) == NULL and dttree->GetTreeDepth() > 2)
						{
							// choix du nom de la variables
							svariablename = kwcUpdatedClass->BuildAttributeName(
							    "Tree_" +
							    ALString(IntToString(attributegenerator->GetIndex() + 1)));
							attribute = reportTreeSpec->BuildAttribute(svariablename);
							ndTreeSingleton.SetAt(key, attribute);
							SetConstructedAttributeCost(
							    attribute, reportTreeSpec->GetConstructionCost() +
									   shared_learningSpec.GetLearningSpec()
									       ->GetSelectionCost());
							oaCreatedAttributes.Add(attribute);
							if (shared_learningSpec.GetLearningSpec()
								->GetTargetAttributeType() == KWType::Symbol)
								oatupletable.Add(BuildTupleTableForClassification(
								    dttree, svariablename));
							else
								oatupletable.Add(BuildTupleTableForRegression(
								    shared_learningSpec.GetLearningSpec(), dttree,
								    svariablename));

							// Insertion dans le rapport de creation
							reportTreeSpec->SetTreeVariableName(svariablename);

							output_oaDecisionTreeSpecs->GetObjectArray()->Add(
							    reportTreeSpec);
						}
						else
						{
							delete reportTreeSpec;
						}
					}

					// netoyage de l'arbre courant
					delete dttree;
					dttree = NULL;
					SetRandomSeed(oldseedtree);
				}
				oaInputAttributeStats->DeleteAll();
				delete oaInputAttributeStats;
			}
			delete learningSpecTree;
		}

		oaObjects.DeleteAll();

		if (bOk)
		{
			// calcul des stats sur les attributs arbres qui ont ete crees par l'esclave

			// On commence a passer tous les attributs de la classe en not loaded
			kwcUpdatedClass->SetAllAttributesLoaded(false);
			shared_learningSpec.GetLearningSpec()->GetDatabase()->SetClassName(
			    shared_learningSpec.GetLearningSpec()->GetClass()->GetName());

			// Ajout des attributs dans la classe
			for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
				// createdAttribute->SetName(kwcUpdatedClass->BuildAttributeName(createdAttribute->GetName()));
				createdAttribute->CompleteTypeInfo(kwcUpdatedClass);
				kwcUpdatedClass->InsertAttribute(createdAttribute);
			}
			// Compilation de la classe
			kwcUpdatedClass->Compile();
			//////
			for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}

				// Progression pour la seconde etape du slave process (entre 50% et 100%)
				TaskProgression::DisplayProgression(50 + ((nAttribute + 1) * 50) /
									     oaCreatedAttributes.GetSize());

				ttattribut = cast(KWTupleTable*, oatupletable.GetAt(nAttribute));

				//// Creation et initialisation d'un objet de stats pour l'attribut
				attributeStats = new KWAttributeStats();
				attributeStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());
				attributeStats->SetAttributeName(ttattribut->GetAttributeNameAt(0));
				attributeStats->SetAttributeType(KWType::Symbol);

				//// Calcul des statistitique univariee a partir de la table de tuples
				attributeStats->ComputeStats(ttattribut);

				// on ne garde pas les arbres a level nul
				if (attributeStats->GetLevel() == 0)
				{
					// supprimer le reportTreeSpec correspondant
					for (int nTreeSpec = 0;
					     nTreeSpec < output_oaDecisionTreeSpecs->GetObjectArray()->GetSize();
					     nTreeSpec++)
					{
						DTDecisionTreeSpec* decisionTreeSpec = cast(
						    DTDecisionTreeSpec*,
						    output_oaDecisionTreeSpecs->GetObjectArray()->GetAt(nTreeSpec));

						if (decisionTreeSpec->GetTreeVariableName() ==
						    attributeStats->GetAttributeName())
						{
							output_oaDecisionTreeSpecs->GetObjectArray()->RemoveAt(
							    nTreeSpec);
							delete decisionTreeSpec;
						}
					}
					// ajout des attributs de level zero a nettoyer
					oaZeroLevelAttributs.Add(oaCreatedAttributes.GetAt(nAttribute));
					// supression de attributeStats de la variable de level zero
					delete attributeStats;
				}
				else
					output_oaAttributeStats->GetObjectArray()->Add(attributeStats);
			}

			// netoyage des attributs de level zero
			if (oaZeroLevelAttributs.GetSize() > 0)
			{
				for (nAttribute = 0; nAttribute < oaZeroLevelAttributs.GetSize(); nAttribute++)
				{
					createdAttribute = cast(KWAttribute*, oaZeroLevelAttributs.GetAt(nAttribute));
					kwcUpdatedClass->DeleteAttribute(createdAttribute->GetName());
				}
				kwcUpdatedClass->Compile();
			}

			oatupletable.DeleteAll();

			// On repasse tous les attributs de la classe en loaded
			kwcUpdatedClass->SetAllAttributesLoaded(true);
			kwcUpdatedClass->Compile();
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Nettoyage si echec
	if (not bOk)
	{
		assert(output_oaAttributeStats->GetObjectArray()->GetSize() == 0);

		// Suppression des attributs de la classe
		if (kwcUpdatedClass != NULL)
		{
			for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
			{
				createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
				kwcUpdatedClass->DeleteAttribute(createdAttribute->GetName());
			}
			kwcUpdatedClass->Compile();
		}
		oaCreatedAttributes.SetSize(0);
		oatupletable.DeleteAll();
	}

	// Message si erreur
	if (not bOk)
	{
		if (TaskProgression::IsInterruptionRequested())
			AddWarning("Interrupted by user");
		else
			AddError("Interrupted because of errors");
	}
	else
	{
#ifdef GENERATE_PYTHON_REPORTING_TRACES
		if (DTTimer_ComputeDecisionTree.IsStarted())
			DTTimer_ComputeDecisionTree.Stop();
		DTTimer_SlaveProcess.Stop();
		AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
				 IntToString(GetTaskIndex() + 1) + "\t" + ALString("ComputeTree total time: \t") +
				 DoubleToString(DTTimer_ComputeDecisionTree.GetElapsedTime()));
		AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
				 IntToString(GetTaskIndex() + 1) + "\t" + ALString("SlaveProcess total time: \t") +
				 DoubleToString(DTTimer_SlaveProcess.GetElapsedTime()));
		AddSimpleMessage(ALString("Slave\t") + IntToString(GetProcessId()) + ", task index " +
				 IntToString(GetTaskIndex() + 1) + "\t" + ALString("Trees number : \t") +
				 IntToString(nBuidTreeNumber));
		Global::SetMaxErrorFlowNumber(oldMaxErrorFlowNumber);
#endif

		ensure(bOk or oaCreatedAttributes.GetSize() == 0);
	}

	// Nettoyage
	dataTableSliceSet.GetSlices()->RemoveAll();
	KWClassDomain::GetCurrentDomain()->DeleteClass(kwcSliceSetClass->GetName());

	if (shared_learningSpec.GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous)
	{
		// nettoyer les donnes fictives ayant servi a transformer temporairement une regression en
		// classification
		if (slaveTupleTableLoader.GetInputExtraAttributeSymbolValues() != NULL)
		{
			delete slaveLearningSpec;
			delete slaveTupleTableLoader.GetInputExtraAttributeSymbolValues();
			KWClassDomain::GetCurrentDomain()->DeleteClass(
			    shared_learningSpec.GetLearningSpec()->GetTargetAttributeName() + "_categorical");
		}
	}

	// Nettoyage des sorties si erreur
	if (not bOk)
	{
		output_oaAttributeStats->GetObjectArray()->DeleteAll();
		output_oaDecisionTreeSpecs->GetObjectArray()->DeleteAll();
	}
	return bOk;
}

// *************************************************************************************************************************
//                                         methodes de calcul des ressources *
// *************************************************************************************************************************

boolean DTDecisionTreeCreationTask::ComputeResourceRequirements()
{
	int nMaxSlaveProcessNumber = 0;
	longint lSharedMemory = 0;
	longint lMasterMemory = 0;
	longint lBiggestTreeMemory = 0;
	const KWTupleTable* targetTupleTable;
	const int bTracerResourcePreviousState = PLParallelTask::GetTracerResources();

	if (bMasterTraceOn)
	{
		PLParallelTask::SetTracerResources(1);
		cout << endl << "ComputeResourceRequirements :" << endl;
	}

	// Entete trace
	if (GetForestParameter()->GetDecisionTreeParameter()->GetVerboseMode())
		AddSimpleMessage("Tracing resource estimations");

	targetTupleTable = shared_TargetTupleTable.GetTupleTable();

	nMasterDatabaseObjectNumber = targetTupleTable->GetTotalFrequency();

	// Memorisation des nombre d'instance par chunk du dataTableSliseSet, creation des DTAttributeSelections du
	// Forest
	ComputeTaskInputs();

	// initialisation des parametres permettant les diverses estimations de memoire (esclave, maitre...)
	InitializeMemoryEstimations();

	// nombre de process esclaves : ne pas en demander plus qu'il n'y a d'arbres a creer, car c'est inutile
	nMaxSlaveProcessNumber = ComputeMaxSlaveProcessNumber();
	assert(nMaxSlaveProcessNumber > 0);

	// Estimation de la memoire partagee
	lSharedMemory = ComputeSharedNecessaryMemory();

	// Estimation de la memoire pour le maitre
	lMasterMemory = ComputeMasterNecessaryMemory();

	// mémoire necessaire pour calculer le plus gros arbre possible sur 1 esclave
	lBiggestTreeMemory = ComputeBiggestTreeNecessaryMemory();

	// Mise a jour des demandes de resources
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nMaxSlaveProcessNumber);
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->Set(lSharedMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(lMasterMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(lBiggestTreeMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(lBiggestTreeMemory *
									      nMaxCreatedAttributeNumber);

	if (bMasterTraceOn)
	{
		cout << endl << "FIN ComputeResourceRequirements" << endl;
		PLParallelTask::SetTracerResources(bTracerResourcePreviousState);
	}

	return true; // Ne renvoie faux que dans le cas ou l'on est incapable d'effectuer le calcul des ressources
}

void DTDecisionTreeCreationTask::ComputeTaskInputs()
{
	ALString sMessage;

	require(IsMasterProcess());
	require(IsRunning());
	require(oaInputAttributeSelectionStats.GetSize() == 0);
	require(shared_odAttributeStats->GetObjectDictionary()->GetCount() == 0);

	// Memorisation des nombre d'instance par chunk du dataTableSliseSet
	shared_ivDataTableSliceSetChunkInstanceNumbers.GetIntVector()->CopyFrom(
	    masterDataTableSliceSet->GetChunkInstanceNumbers());

	forestattributeselection->Initialization(odMasterInputAttributeStats);
	BuildForestAttributeSelections(*forestattributeselection, nMaxCreatedAttributeNumber);
	oaInputAttributeSelectionStats.CopyFrom(forestattributeselection->GetAttributeSelections());
}

void DTDecisionTreeCreationTask::BuildForestAttributeSelections(DTForestAttributeSelection& forestAttributeSelection,
								int nmaxCreatedAttributeNumber)
{
	// calcul du nbre d'attributs potentiellement utilisables dans l'arbre (on retire 1 pour l'attribut cible)

	if (randomForestParameter.GetTreesVariablesSelection() == DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL)
		forestAttributeSelection.BuildForestSelections(nmaxCreatedAttributeNumber,
							       randomForestParameter.GetVariableNumberMin());
	else
		forestAttributeSelection.BuildForestUniformSelections(
		    nmaxCreatedAttributeNumber, randomForestParameter.GetTreesVariablesSelection(),
		    randomForestParameter.GetAttributePercentage());

	nMasterForestMaxAttributesSelectionNumber = forestattributeselection->GetMaxAttributesNumber();
}

int DTDecisionTreeCreationTask::ComputeMaxSlaveProcessNumber() const
{
	int result = RMResourceConstraints::GetMaxCoreNumberOnCluster();

	if (result > nMaxCreatedAttributeNumber)
		result = nMaxCreatedAttributeNumber;

	return result;
}

longint DTDecisionTreeCreationTask::ComputeMasterNecessaryMemory()
{
	longint lResult;
	boolean bLocalTrace = false;

	assert(lMasterAllResultsMeanMemory > 0);
	assert(lMasterMedianStatMemory > 0);

	lResult = lMasterAllResultsMeanMemory;

	// on estime grossierement la taille des attributesStats des arbres retournes par les esclaves, en multipliant
	// la taille mediane d'un attributeStats par le nombre d'arbres a realiser
	lResult += (lMasterMedianStatMemory * nMaxCreatedAttributeNumber);

	// Trace de deboggage
	if (bLocalTrace)
	{
		ALString sMessage = "\nMaster memory estimation:\n";
		sMessage += "\tMean overall result (trees specs + stats) mem = " +
			    ALString(LongintToHumanReadableString(lMasterAllResultsMeanMemory));
		sMessage +=
		    "\n\tmedian attribute stats mem * trees number = " +
		    ALString(LongintToHumanReadableString(lMasterMedianStatMemory * nMaxCreatedAttributeNumber));
		sMessage += "\n\tMaster mem = " + ALString(LongintToHumanReadableString(lResult)) + "\n";
		cout << sMessage;
	}
	return lResult;
}

longint DTDecisionTreeCreationTask::ComputeSharedNecessaryMemory()
{
	longint lSharedMemory;
	longint lNecessaryTargetAttributeMemory;
	KWLearningSpec* learningSpec;
	const KWTupleTable* targetTupleTable;
	boolean bLocalTrace = false;

	assert(lMasterAllResultsMeanMemory > 0);

	// memoire partagee entre le maitre et tous les esclaves:
	//   . stockage des variables partagees : les KWAttributeStats d'input + les KWAttributeStats d'output +
	//   memmoire necessaire a la prise en compte des target attribute . stockage des resultats des arbres calcule
	//   par les esclaves

	learningSpec = shared_learningSpec.GetLearningSpec();
	targetTupleTable = shared_TargetTupleTable.GetTupleTable();
	lNecessaryTargetAttributeMemory = ComputeNecessaryTargetAttributeMemory(learningSpec, targetTupleTable);

	lSharedMemory = lNecessaryTargetAttributeMemory + lMasterAllResultsMeanMemory;

	// Trace de deboggage
	if (bLocalTrace)
	{
		ALString sMessage = "\n\nShared memory estimation:\n";
		sMessage += "\ttarget attribute          mem = " +
			    ALString(LongintToHumanReadableString(lNecessaryTargetAttributeMemory)) + "\n";
		sMessage += "\tMean overall result (trees + stats) mem = " +
			    ALString(LongintToHumanReadableString(lMasterAllResultsMeanMemory)) + "\n";
		sMessage +=
		    "\tshared                    mem = " + ALString(LongintToHumanReadableString(lSharedMemory)) + "\n";
		cout << sMessage;
	}

	return lSharedMemory;
}

longint DTDecisionTreeCreationTask::ComputeBiggestTreeNecessaryMemory()
{
	/*
	Memoire necessaire pour 1 esclave, pour calculer le plus gros arbre possible
	*/

	longint lResult;
	boolean bLocalTrace = false;

	assert(lMasterMedianStatMemory > 0);
	assert(lMasterOneAttributeValueMemory > 0);
	assert(lMasterTreeWorkingMemory > 0);
	assert(nMasterForestMaxAttributesSelectionNumber > 0);

	// ATTENTION toute modification de cette formule doit etre reporter dans la fonction
	// ComputeOneSlaveMaxLoadableAttributeNumber
	lResult =
	    (nMasterForestMaxAttributesSelectionNumber * (lMasterMedianStatMemory + lMasterOneAttributeValueMemory)) +
	    lMasterTreeWorkingMemory + lMasterTreeResultMeanMemory;
	lResult = longint(1.3 * lResult); // marge de securite

	// Trace de deboggage
	if (bLocalTrace)
	{
		ALString sMessage = "\nComputeBiggestTreeNecessaryMemory\n";
		sMessage +=
		    "\t\tMedian stats mem = " + ALString(LongintToHumanReadableString(lMasterMedianStatMemory)) + "\n";
		sMessage += "\t\tForest max attributes selection number = " +
			    ALString(LongintToString(nMasterForestMaxAttributesSelectionNumber)) + "\n";
		sMessage +=
		    "\t\tTree working mem = " + ALString(LongintToHumanReadableString(lMasterTreeWorkingMemory)) + "\n";
		sMessage += "\t\tTree result mean mem = " +
			    ALString(LongintToHumanReadableString(lMasterTreeResultMeanMemory)) + "\n";
		sMessage += "\t\tOne Attribute Value mem = " +
			    ALString(LongintToHumanReadableString(lMasterOneAttributeValueMemory)) + "\n";
		sMessage += "\t\tNecessary memory on 1 slave, for computing the biggest possible tree :" +
			    ALString(LongintToHumanReadableString(lResult)) + "\n";
		cout << sMessage;
	}

	return lResult;
}

void DTDecisionTreeCreationTask::InitializeMemoryEstimations()
{
	bool bLocalTrace = false;

	assert(nMasterForestMaxAttributesSelectionNumber > 0);

	// memoire mediane d'1 attributeStats
	lMasterMedianStatMemory = ComputeMedianStatMemory(odMasterInputAttributeStats);

	// memoire moyenne d'1 treeSpec, pour 64 noeuds par arbre.
	lMasterMeanTreeSpecMemory =
	    ComputeMeanTreeSpecMemory(odMasterInputAttributeStats) * nMasterDatabaseObjectNumber / 64;

	// memoire pour stocker le resultat d'1 arbre
	lMasterTreeResultMeanMemory = lMasterMeanTreeSpecMemory + lMasterMedianStatMemory;

	// memoire pour stocker le resultat de tous les arbres demandes
	lMasterAllResultsMeanMemory = nMaxCreatedAttributeNumber * lMasterTreeResultMeanMemory;

	// Memoire pour stocker une valeur d'attribut, pour tous les objets
	// Attention, il faudra prendre en compte le cas sparse de facon plus fine
	lMasterOneAttributeValueMemory = sizeof(KWValue) * nMasterDatabaseObjectNumber;
	lMasterOneAttributeValueMemory *= 2;

	// Memoire de travail pour gerer un arbre
	// Objet a vide, plus un pointeur dans tous les noeuds de l'arbre, le tout par enregistrement
	lMasterEmptyObjectSize = sizeof(KWObject) + sizeof(KWObject*);
	lMasterTreeWorkingMemory = lMasterEmptyObjectSize * nMasterDatabaseObjectNumber;
	lMasterTreeWorkingMemory += (lMasterOneAttributeValueMemory * nMasterForestMaxAttributesSelectionNumber);
	lMasterTreeWorkingMemory *= 2;

	// Trace de deboggage
	if (bLocalTrace)
	{
		ALString sMessage = "\nInitializeMemoryEstimations\n";
		sMessage +=
		    "\t\tdatabase objects number = " + ALString(IntToString(nMasterDatabaseObjectNumber)) + "\n";
		sMessage += "\t\tInput attributes stats number = " +
			    ALString(IntToString(odMasterInputAttributeStats->GetCount())) + "\n";
		sMessage += "\t\tTrees number = " + ALString(IntToString(nMaxCreatedAttributeNumber)) + "\n";
		sMessage +=
		    "\t\tMedian stats mem = " + ALString(LongintToHumanReadableString(lMasterMedianStatMemory)) + "\n";
		sMessage +=
		    "\t\tMean tree spec mem = " + ALString(LongintToHumanReadableString(lMasterMeanTreeSpecMemory)) +
		    "\n";
		sMessage += "\t\tForest max attributes selection number = " +
			    ALString(LongintToString(nMasterForestMaxAttributesSelectionNumber)) + "\n";
		sMessage +=
		    "\t\tTree working mem = " + ALString(LongintToHumanReadableString(lMasterTreeWorkingMemory)) + "\n";
		sMessage += "\t\tOne Attribute Value mem = " +
			    ALString(LongintToHumanReadableString(lMasterOneAttributeValueMemory)) + "\n";
		sMessage += "\t\tTree result mean mem = " +
			    ALString(LongintToHumanReadableString(lMasterTreeResultMeanMemory)) + "\n";
		sMessage += "\t\tAll trees results mem = " +
			    ALString(LongintToHumanReadableString(lMasterAllResultsMeanMemory)) + "\n";
		cout << sMessage;
	}
}

int DTDecisionTreeCreationTask::ComputeOneSlaveMaxLoadableAttributeNumber(const int lGrantedlavesNumber,
									  const longint lGrantedMinSlaveMemory)
{
	// Calcul du nombre max d'attributs pour 1 esclave, compte tenu de ce qui a ete alloue par la librairie
	// parallele

	longint lResult;
	bool bLocalTrace = false;

	assert(lGrantedMinSlaveMemory > 0);
	assert(lMasterMedianStatMemory > 0);
	assert(lMasterTreeWorkingMemory > 0);
	assert(lMasterOneAttributeValueMemory > 0);
	assert(nMasterForestMaxAttributesSelectionNumber > 0);
	assert(lGrantedlavesNumber > 0);

	/* en termes de taille memoire, 1 attribut chargeable sur 1 esclave correspond a l'addition de :
		- la taille memoire mediane d'un KWAttributeStats (lMasterMedianStatMemory)
		- la memoire necessaire pour stocker une valeur d'attribut, pour tous les objets de la database
	(lMasterOneAttributeValueMemory)

	Pour calculer le nombre d'attributs chargeables, la memoire attribuee par le framework pour chaque esclave
	(lGrantedMinSlaveMemory) doit etre diminuee de :
		- la memoire de travail necessaire pour gerer un arbre vide (lMasterTreeWorkingMemory)
		- la memoire necessaire pour stocker 1 resultat (lMasterTreeResultMeanMemory)

	ATTENTION toute modification de cette formule doit etre reporter dans la fonction
	ComputeBiggestTreeNecessaryMemory
	*/

	lResult = (lGrantedMinSlaveMemory - lMasterTreeWorkingMemory - lMasterTreeResultMeanMemory) /
		  (lMasterMedianStatMemory + lMasterOneAttributeValueMemory);

	// Affichage
	// Trace de deboggage
	if (bLocalTrace)
	{
		ALString sMessage;
		sMessage = "\n\nComputeOneSlaveMaxLoadableAttributeNumber:\n";
		sMessage +=
		    "\t\tlGranted Min Slave mem = " + ALString(LongintToHumanReadableString(lGrantedMinSlaveMemory)) +
		    "\n";
		sMessage += "\t\tlGranted Slaves number = " + ALString(LongintToString(lGrantedlavesNumber)) + "\n";
		sMessage +=
		    "\t\tMedian stats mem = " + ALString(LongintToHumanReadableString(lMasterMedianStatMemory)) + "\n";
		sMessage +=
		    "\t\tTree working mem = " + ALString(LongintToHumanReadableString(lMasterTreeWorkingMemory)) + "\n";
		sMessage += "\t\tTree result mean mem = " +
			    ALString(LongintToHumanReadableString(lMasterTreeResultMeanMemory)) + "\n";
		sMessage += "\t\tOne Attribute Value mem = " +
			    ALString(LongintToHumanReadableString(lMasterOneAttributeValueMemory)) + "\n";
		sMessage += "\t\tForest max attributes selection number = " +
			    ALString(LongintToString(nMasterForestMaxAttributesSelectionNumber)) + "\n";
		sMessage +=
		    "\t\tOne slave max loadable attributes number computing : " + ALString(LongintToString(lResult)) +
		    "\n";
		cout << sMessage;
	}

	ensure(lResult >= nMasterForestMaxAttributesSelectionNumber and lResult > 0 and lResult <= INT32_MAX);

	return (int)lResult;
}

longint DTDecisionTreeCreationTask::ComputeMedianStatMemory(ObjectDictionary* odInputAttributeStats) const
{
	longint lWorkingMemorySize = 0;
	longint nbvar;
	ALString sIdentifier;
	POSITION position;
	Object* object;
	KWAttributeStats* attributeStats;
	LongintVector cvValues;
	longint cMedian = 0;

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

	if (nbvar > 0)
	{
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
				cMedian = (cvValues.GetAt(cvValues.GetSize() / 2 - 1) +
					   cvValues.GetAt(cvValues.GetSize() / 2)) /
					  2;
			else
				cMedian = cvValues.GetAt(cvValues.GetSize() / 2);
		}
	}

	return cMedian;
}

longint DTDecisionTreeCreationTask::ComputeMeanTreeSpecMemory(ObjectDictionary* odInputAttributeStats) const
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
	ensure(nbvar > 0);

	return lResultMemorySize / nbvar;
}

void DTDecisionTreeCreationTask::BuildAttributeSelectionsSlices(const ObjectArray* oaAttributeSelections,
								ObjectArray* oaAttributeSelectionsSlices,
								int nMaxloadVariablesForOneSlave,
								int nGrantedSlaveNumber)
{
	int idxSlice;
	int idxAttributeSelection;
	ObjectDictionary odSliceAttributes;
	DTAttributeSelection* attributeSelection;
	DTAttributeSelectionsSlices* attributeSelectionSlices;
	int idxFirstSlice;
	const boolean bLocalTrace = false;
	require(oaAttributeSelections != NULL);
	require(oaAttributeSelectionsSlices != NULL);
	assert(nMaxloadVariablesForOneSlave > 0);

	if (oaAttributeSelectionsSlices->GetSize() > 0)
		oaAttributeSelectionsSlices->DeleteAll();

	// Alimentation d'un dictionnaire, dont les cles sont les noms des attributs et les objet sont les tranches
	// (KWDataTableSlice) les contenant
	masterDataTableSliceSet->FillSliceAttributes(&odSliceAttributes);

	attributeSelectionSlices = NULL;

	// trier les selections d'attributs par ordre de tailles decroissantes, afin d'optimiser le traitement des
	// slices par les esclaves
	ObjectArray oaRemainingAttributesSelections;
	oaRemainingAttributesSelections.CopyFrom(oaAttributeSelections);
	oaRemainingAttributesSelections.SetCompareFunction(DTAttributeSelectionCompareAttributesNumber);
	oaRemainingAttributesSelections.Sort();

	int nbSlavesToFill = 0;

	while ((nbSlavesToFill = ComputeSlavesNumberToFill(&oaRemainingAttributesSelections, nGrantedSlaveNumber)) > 0)
	{
		// on cree autant de groupes de selections d'attributs, qu'il y a d'esclaves a remplir lors de cette
		// iteration
		ObjectArray oaIterationaAttributeSelectionsSlices;

		for (int i = 0; i < nbSlavesToFill; i++)
		{
			attributeSelectionSlices = new DTAttributeSelectionsSlices;
			oaIterationaAttributeSelectionsSlices.Add(attributeSelectionSlices);
		}

		// on affecte le plus possible de selections d'attributs restantes, et de la maniere la plus egale
		// possible, sur les groupes de selections d'attributs qu'on vient juste de creer
		idxFirstSlice = 0;

		for (idxAttributeSelection = 0; idxAttributeSelection < oaRemainingAttributesSelections.GetSize();
		     idxAttributeSelection++)
		{
			attributeSelection =
			    cast(DTAttributeSelection*, oaRemainingAttributesSelections.GetAt(idxAttributeSelection));

			if (idxFirstSlice >= oaIterationaAttributeSelectionsSlices.GetSize())
				idxFirstSlice = 0; // on continue d'essayer remplir les groupes de selections, en
						   // repartant du 1er groupe de selections

			for (idxSlice = idxFirstSlice; idxSlice < oaIterationaAttributeSelectionsSlices.GetSize();
			     idxSlice++)
			{
				attributeSelectionSlices = cast(DTAttributeSelectionsSlices*,
								oaIterationaAttributeSelectionsSlices.GetAt(idxSlice));

				int iUnionNbAttributes =
				    attributeSelectionSlices->UnionAttributesCount(attributeSelection);
				if (iUnionNbAttributes <= nMaxloadVariablesForOneSlave)
				{
					attributeSelectionSlices->AddAttributeSelection(attributeSelection,
											&odSliceAttributes);
					assert(attributeSelectionSlices->Check());
					idxFirstSlice = idxSlice + 1;
					oaRemainingAttributesSelections.RemoveAt(idxAttributeSelection);
					idxAttributeSelection =
					    -1; // on parcourt les selections restantes depuis le debut, en essayant de
						// les repartir sur les groupes de selections
					break;
				}
			}
		}

		// on ajoute les groupes de selections crees a cette iteration, a la liste globale des groupes de
		// selections
		for (int i = 0; i < oaIterationaAttributeSelectionsSlices.GetSize(); i++)
		{
			attributeSelectionSlices =
			    cast(DTAttributeSelectionsSlices*, oaIterationaAttributeSelectionsSlices.GetAt(i));
			oaAttributeSelectionsSlices->Add(attributeSelectionSlices);
		}
		oaIterationaAttributeSelectionsSlices.RemoveAll();
	}

	ensure(oaRemainingAttributesSelections.GetSize() == 0);

	if (bLocalTrace)
	{
		cout << endl << endl << "liste des groupes de selections (slices) : " << endl;
		for (int i = 0; i < oaAttributeSelectionsSlices->GetSize(); i++)
		{
			cout << "=======================" << endl;
			attributeSelectionSlices =
			    cast(DTAttributeSelectionsSlices*, oaAttributeSelectionsSlices->GetAt(i));
			attributeSelectionSlices->Write(cout);
		}
	}
}

int DTDecisionTreeCreationTask::ComputeSlavesNumberToFill(const ObjectArray* oaRemainingAttributesSelections,
							  const int nGrantedSlaveNumber) const
{
	// calcule sur combien d'esclaves on doit repartir les selections d'attributs non encore affectees a un esclave
	assert(oaRemainingAttributesSelections != NULL);

	if (oaRemainingAttributesSelections->GetSize() < 2)
		return oaRemainingAttributesSelections->GetSize();

	int maxSelectionsNumber = oaRemainingAttributesSelections->GetSize() / 2;

	if (maxSelectionsNumber > nGrantedSlaveNumber)
		return nGrantedSlaveNumber;
	else
		return maxSelectionsNumber;
}

// *************************************************************************************************************************
//                                         methodes de creation des arbres *
// *************************************************************************************************************************

DTDecisionTree* DTDecisionTreeCreationTask::CreateDecisionTree(KWLearningSpec* learningSpec,
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
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_NEW_BIN_LABEL)
	{
		currentTree->SetCostClass(new DTDecisionBinaryTreeCost);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(2);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(2);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("DTMODL");
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName("DTMODL");
	}
	else
		currentTree->SetCostClass(new DTDecisionTreeRecursiveCost);

	if (randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber() > 1)
	{
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(
		    randomForestParameter.GetDecisionTreeParameter()->GetMaxChildrenNumber());
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
	}
	else if (randomForestParameter.GetDecisionTreeParameter()->GetTreeCost() == DTGlobalTag::REC_TRI_LABEL)
	{
		currentTree->GetCostClass()->SetDTCriterionMode(4);
		learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(3);
		learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(3);
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

	currentTree->SetParameters(randomForestParameter.GetDecisionTreeParameter());

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

ObjectArray* DTDecisionTreeCreationTask::BuildRootAttributeStats(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine,
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
		    learningSpec, blOrigine->GetTupleLoader(), &oaInputAttribute, false, oaOutputAttributeStats);
	}
	else
	{
		odInputAttributeStats->ExportObjectArray(oaOutputAttributeStats);
	}

	return oaOutputAttributeStats;
}

boolean DTDecisionTreeCreationTask::ComputeTree(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine,
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

KWTupleTable* DTDecisionTreeCreationTask::BuildTupleTableForClassification(DTDecisionTree* dttree,
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

	univariateTupleTable->SetUpdateMode(false);

	delete oaleaves;

	return univariateTupleTable;
}

KWTupleTable* DTDecisionTreeCreationTask::BuildTupleTableForRegression(const KWLearningSpec* learningSpec,
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

void DTDecisionTreeCreationTask::UnloadNonInformativeAttributes(KWClass* kwclass,
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

void DTDecisionTreeCreationTask::UnloadLessInformativeAttribute(KWClass* kwclass,
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

void DTDecisionTreeCreationTask::LoadOnlySelectedAttributes(KWClass* kwcClass,
							    DTAttributeSelectionsSlices* attributeselectionsSlices)
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

void DTDecisionTreeCreationTask::LoadOnlyMostInformativeAttributes(KWClass* kwcClass,
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

int DTDecisionTreeCreationTask::GetNodeVariableNumber() const
{
	assert(nMasterDatabaseObjectNumber > 0);
	return nMasterDatabaseObjectNumber / 64;
}

void DTDecisionTreeCreationTask::CleanTaskInputs()
{
	delete forestattributeselection;
	forestattributeselection = NULL;
	shared_odAttributeStats->GetObjectDictionary()->RemoveAll();
	oaInputAttributeSelectionsSlices.DeleteAll();
}

void DTDecisionTreeCreationTask::WriteAttributeSelectionsSlices(const ObjectArray* oaAttributeSelectionsSlices,
								ostream& ost) const
{
	int i;
	DTAttributeSelectionsSlices attributeSelectionsSlicesLabel;
	DTAttributeSelectionsSlices* attributeSelectionsSlices;

	// Creation d'un objet DTAttributeSelectionsSlices par paire d'attribut
	ost << "Array of " << attributeSelectionsSlicesLabel.GetClassLabel() << ": "
	    << oaAttributeSelectionsSlices->GetSize() << "\n";
	for (i = 0; i < oaAttributeSelectionsSlices->GetSize(); i++)
	{
		attributeSelectionsSlices = cast(DTAttributeSelectionsSlices*, oaAttributeSelectionsSlices->GetAt(i));
		ost << *attributeSelectionsSlices << endl;
	}
}

KWLearningSpec* DTDecisionTreeCreationTask::InitializeRegressionLearningSpec(const KWLearningSpec* learningSpec)
{
	// on transforme la cible continue en cible categorielle, en effectuant au prealable une dicretisation equalFreq
	// sur la cible continue

	DTBaseLoader bl;
	KWLearningSpec* newLearningSpec = NULL;
	KWClass* newClass = NULL;
	KWAttribute* newTarget = NULL;

	assert(learningSpec != NULL);

	newLearningSpec = learningSpec->Clone();
	newClass = learningSpec->GetClass()->Clone();
	newClass->SetName(newClass->GetName() + "_classification");
	newTarget = new KWAttribute;

	newTarget->SetName(learningSpec->GetTargetAttributeName() + "_categorical");
	newTarget->SetType(KWType::Symbol);
	newClass->InsertAttribute(newTarget);
	KWClassDomain::GetCurrentDomain()->InsertClass(newClass);
	newClass->Compile();
	KWClassDomain::GetCurrentDomain()->Compile();
	assert(newClass->Check());

	newLearningSpec->SetClass(newClass);
	newLearningSpec->SetTargetAttributeName(newTarget->GetName());
	newLearningSpec->GetDatabase()->SetClassName(newClass->GetName());

	return newLearningSpec;
}

void DTDecisionTreeCreationTask::InitializeEqualFreqDiscretization(KWTupleTableLoader* tupleTableLoader,
								   KWLearningSpec* learningSpec)
{
	// on transforme la cible continue en cible categorielle, en effectuant au prealable une dicretisation equalFreq
	// sur la cible continue

	KWTupleTable* targetTupleTableClassification = NULL;
	boolean bOk = true;
	DTBaseLoader bl;
	SymbolVector* svTargetValues = NULL;

	assert(learningSpec != NULL);
	assert(tupleTableLoader != NULL);
	assert(tupleTableLoader->GetInputExtraAttributeType() == KWType::Continuous);
	assert(randomForestParameter.GetDiscretizationTargetMethod() ==
	       DTForestParameter::DISCRETIZATION_EQUAL_FREQUENCY);

	svTargetValues = EqualFreqDiscretizeContinuousTarget(tupleTableLoader,
							     randomForestParameter.GetMaxIntervalsNumberForTarget());

	assert(svTargetValues != NULL);

	tupleTableLoader->SetInputExtraAttributeName(learningSpec->GetTargetAttributeName());
	tupleTableLoader->SetInputExtraAttributeSymbolValues(svTargetValues);
	tupleTableLoader->SetInputExtraAttributeType(KWType::Symbol);
	tupleTableLoader->SetCheckDatabaseObjectClass(false);
	targetTupleTableClassification = cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable());
	bl.LoadTupleTableFromSymbolValues(learningSpec->GetClass(), learningSpec->GetTargetAttributeName(),
					  svTargetValues, targetTupleTableClassification);
	tupleTableLoader->SetInputClass(learningSpec->GetClass());

	// calcul des stats cibles
	learningSpec->ComputeTargetStats(targetTupleTableClassification);

	bOk = bOk and learningSpec->Check();
	bOk = bOk and tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() <= 1;
	bOk = bOk and (tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() == 0 or
		       learningSpec->GetTargetAttributeName() != "");
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNameAt(0) ==
			   learningSpec->GetTargetAttributeName());
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeTypeAt(0) ==
			   learningSpec->GetTargetAttributeType());
	assert(bOk);
}

void DTDecisionTreeCreationTask::InitializeMODLDiscretization(KWTupleTableLoader* tupleTableLoader,
							      KWLearningSpec* learningSpec,
							      const ContinuousVector& cvIntervalValues)
{
	// on transforme la cible continue en cible categorielle, en effectuant au prealable une dicretisation MODL sur
	// la cible continue

	KWTupleTable* targetTupleTableClassification = NULL;
	boolean bOk = true;
	DTBaseLoader bl;
	SymbolVector* svTargetValues = NULL;

	assert(learningSpec != NULL);
	assert(tupleTableLoader != NULL);
	assert(randomForestParameter.GetDiscretizationTargetMethod() == DTForestParameter::DISCRETIZATION_MODL);

	svTargetValues = MODLDiscretizeContinuousTarget(
	    tupleTableLoader, randomForestParameter.GetMaxIntervalsNumberForTarget(), cvIntervalValues, 0);
	assert(svTargetValues != NULL);

	tupleTableLoader->SetInputExtraAttributeName(learningSpec->GetTargetAttributeName());
	if (tupleTableLoader->GetInputExtraAttributeSymbolValues() != NULL)
		delete slaveTupleTableLoader.GetInputExtraAttributeSymbolValues();
	tupleTableLoader->SetInputExtraAttributeSymbolValues(svTargetValues);
	tupleTableLoader->SetInputExtraAttributeType(KWType::Symbol);
	tupleTableLoader->SetCheckDatabaseObjectClass(false);
	targetTupleTableClassification = cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable());
	bl.LoadTupleTableFromSymbolValues(learningSpec->GetClass(), learningSpec->GetTargetAttributeName(),
					  svTargetValues, targetTupleTableClassification);
	tupleTableLoader->SetInputClass(learningSpec->GetClass());

	// calcul des stats cibles
	learningSpec->ComputeTargetStats(targetTupleTableClassification);

	bOk = bOk and learningSpec->Check();
	bOk = bOk and tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() <= 1;
	bOk = bOk and (tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() == 0 or
		       learningSpec->GetTargetAttributeName() != "");
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNameAt(0) ==
			   learningSpec->GetTargetAttributeName());
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeTypeAt(0) ==
			   learningSpec->GetTargetAttributeType());
	assert(bOk);
}

void DTDecisionTreeCreationTask::InitializeBinaryEQFDiscretization(KWTupleTableLoader* tupleTableLoader,
								   KWLearningSpec* learningSpec,
								   const ContinuousVector& cvIntervalValues,
								   const int nSplitIndex)
{
	// on transforme la cible continue en cible categorielle, en effectuant au prealable une dicretisation MODL sur
	// la cible continue

	KWTupleTable* targetTupleTableClassification = NULL;
	boolean bOk = true;
	DTBaseLoader bl;
	SymbolVector* svTargetValues = NULL;

	assert(learningSpec != NULL);
	assert(tupleTableLoader != NULL);
	assert(randomForestParameter.GetDiscretizationTargetMethod() ==
	       DTForestParameter::DISCRETIZATION_BINARY_EQUAL_FREQUENCY);

	svTargetValues = MODLDiscretizeContinuousTarget(
	    tupleTableLoader, randomForestParameter.GetMaxIntervalsNumberForTarget(), cvIntervalValues, nSplitIndex);
	assert(svTargetValues != NULL);

	tupleTableLoader->SetInputExtraAttributeName(learningSpec->GetTargetAttributeName());
	if (tupleTableLoader->GetInputExtraAttributeSymbolValues() != NULL)
		delete slaveTupleTableLoader.GetInputExtraAttributeSymbolValues();
	tupleTableLoader->SetInputExtraAttributeSymbolValues(svTargetValues);
	tupleTableLoader->SetInputExtraAttributeType(KWType::Symbol);
	tupleTableLoader->SetCheckDatabaseObjectClass(false);
	targetTupleTableClassification = cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable());
	bl.LoadTupleTableFromSymbolValues(learningSpec->GetClass(), learningSpec->GetTargetAttributeName(),
					  svTargetValues, targetTupleTableClassification);
	tupleTableLoader->SetInputClass(learningSpec->GetClass());

	// calcul des stats cibles
	learningSpec->ComputeTargetStats(targetTupleTableClassification);

	bOk = bOk and learningSpec->Check();
	bOk = bOk and tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() <= 1;
	bOk = bOk and (tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNumber() == 0 or
		       learningSpec->GetTargetAttributeName() != "");
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeNameAt(0) ==
			   learningSpec->GetTargetAttributeName());
	bOk = bOk and (learningSpec->GetTargetAttributeName() == "" or
		       tupleTableLoader->GetInputExtraAttributeTupleTable()->GetAttributeTypeAt(0) ==
			   learningSpec->GetTargetAttributeType());
	assert(bOk);
}

SymbolVector* DTDecisionTreeCreationTask::EqualFreqDiscretizeContinuousTarget(KWTupleTableLoader* tupleTableLoader,
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
	assert(tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize() == svTargetValues->GetSize());

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

SymbolVector* DTDecisionTreeCreationTask::MODLDiscretizeContinuousTarget(KWTupleTableLoader* tupleTableLoader,
									 const int nMaxIntervalsNumber,
									 const ContinuousVector& cvInput,
									 const int nSplitIndex) const
{
	SymbolVector* svTargetValues = NULL;
	SymbolVector svTargetIn;
	ContinuousVector cvChosenIntervals;
	ContinuousVector cvInputIntervalValues;
	ObjectDictionary odIntervals;
	Object o;
	boolean bDisplayValues = false;
	Continuous cValue;
	ALString s;

	require(nMaxIntervalsNumber >= 2);
	cvInputIntervalValues.CopyFrom(&cvInput);
	if (randomForestParameter.GetDiscretizationTargetMethod() == DTForestParameter::DISCRETIZATION_MODL)
	{
		cvInputIntervalValues.Shuffle();
		if (cvInputIntervalValues.GetSize() > nMaxIntervalsNumber - 1)
			cvInputIntervalValues.SetSize(nMaxIntervalsNumber - 1);

		// supprimer les doublons
		for (int nInterval = 0; nInterval < cvInputIntervalValues.GetSize(); nInterval++)
		{
			const char* c = DoubleToString(cvInputIntervalValues.GetAt(nInterval));
			if (odIntervals.Lookup(c) != NULL)
				continue;
			odIntervals.SetAt(c, &o);
			cvChosenIntervals.Add(cvInputIntervalValues.GetAt(nInterval));
		}

		cvChosenIntervals.Add(KWContinuous::GetMinValue());
		cvChosenIntervals.Add(KWContinuous::GetMaxValue());
		cvChosenIntervals.Sort();
	}
	else
	{
		cvChosenIntervals.CopyFrom(&cvInput);
	}

	if (bDisplayValues)
		cvChosenIntervals.Write(cout);

	svTargetValues = new SymbolVector;
	if (randomForestParameter.GetDiscretizationTargetMethod() == DTForestParameter::DISCRETIZATION_MODL)
	{
		for (int nInterval = 0; nInterval < cvChosenIntervals.GetSize(); nInterval++)
		{
			s = "I" + ALString(IntToString(nInterval));
			svTargetIn.Add((Symbol)s);
		}
		for (int i = 0; i < tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize(); i++)
		{
			// pour chaque valeur de target continue, determiner a quel intervalle elle appartient
			cValue = tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetAt(i);

			for (int nInterval = 0; nInterval < cvChosenIntervals.GetSize(); nInterval++)
			{
				if (nInterval == cvChosenIntervals.GetSize() - 1 or
				    (cValue >= cvChosenIntervals.GetAt(nInterval) and
				     cValue < cvChosenIntervals.GetAt(nInterval + 1)))
				{
					svTargetValues->Add(svTargetIn.GetAt(nInterval));
					break;
				}
			}
		}
	}
	else
	{
		Symbol sI0("I0");
		Symbol sI1("I1");

		for (int i = 0; i < tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize(); i++)
		{
			// pour chaque valeur de target continue, determiner a quel intervalle elle appartient
			cValue = tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetAt(i);

			if (cValue <= cvChosenIntervals.GetAt(nSplitIndex))
			{
				svTargetValues->Add(sI0);
			}
			else
			{
				svTargetValues->Add(sI1);
			}
		}
	}
	assert(svTargetValues != NULL);
	assert(tupleTableLoader->GetInputExtraAttributeContinuousValues()->GetSize() == svTargetValues->GetSize());

	return svTargetValues;
}

ALString DTDecisionTreeCreationTask::GetReportPrefix() const
{
	return "tree";
}

KWLearningReport* DTDecisionTreeCreationTask::GetCreationReport()
{
	return &masterOutputCreationReport;
}

void DTDecisionTreeCreationTask::CopyAttributeCreationSpecFrom(
    const KDDataPreparationAttributeCreationTask* attributeCreationTask)
{
	// Appel de la methode ancetre
	KDDataPreparationAttributeCreationTask::CopyAttributeCreationSpecFrom(attributeCreationTask);

	// Recopie des attributs specifiques
	sCreatedAttributePrefix = cast(DTDecisionTreeCreationTask*, attributeCreationTask)->GetCreatedAttributePrefix();
	GetForestParameter()->CopyFrom(cast(DTDecisionTreeCreationTask*, attributeCreationTask)->GetForestParameter());
}

const ALString DTDecisionTreeCreationTask::GetTaskName() const
{
	return "Decision Tree variable creation";
}

PLParallelTask* DTDecisionTreeCreationTask::Create() const
{
	return new DTDecisionTreeCreationTask;
}

void DTDecisionTreeCreationTask::SetCreatedAttributePrefix(const ALString& sValue)
{
	sCreatedAttributePrefix = sValue;
}

const ALString& DTDecisionTreeCreationTask::GetCreatedAttributePrefix() const
{
	return sCreatedAttributePrefix;
}

void DTDecisionTreeCreationTask::ReferenceTargetIntervalValues(const ObjectDictionary* odAttributeStats)
{
	ObjectArray oaAttributeStats;
	ContinuousVector cv;
	ContinuousVector* cvTargetValues = NULL;
	KWQuantileIntervalBuilder quantileBuilder;

	if (randomForestParameter.GetDiscretizationTargetMethod() == DTForestParameter::DISCRETIZATION_MODL)
	{
		odAttributeStats->ExportObjectArray(&oaAttributeStats);

		for (int i = 0; i < oaAttributeStats.GetSize(); i++)
		{
			KWAttributeStats* stats = cast(KWAttributeStats*, oaAttributeStats.GetAt(i));
			const KWDGSAttributePartition* partition = stats->GetPreparedDataGridStats()->GetAttributeAt(
			    stats->GetPreparedDataGridStats()->GetFirstTargetAttributeIndex());
			KWDGSAttributeDiscretization* kwdgs = cast(KWDGSAttributeDiscretization*, partition);

			for (int iInterval = 0; iInterval < kwdgs->GetIntervalBoundNumber(); iInterval++)
				cv.Add(kwdgs->GetIntervalBoundAt(iInterval));
		}
	}
	else
	{
		// Initialisation
		cvTargetValues = masterTupleTableLoader->GetInputExtraAttributeContinuousValues()->Clone();
		require(cvTargetValues != NULL);
		cvTargetValues->Sort();
		quantileBuilder.InitializeValues(cvTargetValues);
		delete cvTargetValues;

		// Calcul des quantiles
		quantileBuilder.ComputeQuantiles(randomForestParameter.GetMaxIntervalsNumberForTarget());

		for (int nInterval = 0; nInterval < quantileBuilder.GetIntervalNumber(); nInterval++)
		{
			cv.Add(quantileBuilder.GetIntervalUpperBoundAt(nInterval));
		}
	}
	input_cvIntervalValues.GetContinuousVector()->CopyFrom(&cv);
}

int DAttributeArrayCompareBlocks(const void* elem1, const void* elem2)
{
	KWAttribute* treeatt1;
	KWAttribute* treeatt2;
	const KWAttributeBlock* attributeBlock1;
	const KWAttributeBlock* attributeBlock2;
	int nCompare = 0;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	treeatt1 = cast(KWAttribute*, *(Object**)elem1);
	treeatt2 = cast(KWAttribute*, *(Object**)elem2);
	attributeBlock1 = treeatt1->GetAttributeBlock();
	attributeBlock2 = treeatt2->GetAttributeBlock();

	// Comparaison des bloc
	if (attributeBlock1 == attributeBlock2)
		nCompare = 0;
	else if (attributeBlock1 == NULL)
		nCompare = 1;
	else if (attributeBlock2 == NULL)
		nCompare = -1;
	else
	{
		nCompare = -(attributeBlock1->GetAttributeNumber() - attributeBlock2->GetAttributeNumber());
		if (nCompare == 0)
			nCompare = attributeBlock1->GetName().Compare(attributeBlock2->GetName());
	}
	return nCompare;
}

int DTTreeSpecsCompareNames(const void* elem1, const void* elem2)
{
	int start;
	ALString as1;
	ALString as2;
	int i1;
	int i2;

	DTDecisionTreeSpec* s1 = (DTDecisionTreeSpec*)*(Object**)elem1;
	DTDecisionTreeSpec* s2 = (DTDecisionTreeSpec*)*(Object**)elem2;

	// extraire la partie numerique du nom de l'arbre, pour s'assurer que les arbres seront classes par ordre de
	// creation, et non par ordre alphabetique
	as1 = s1->GetTreeVariableName();
	start = as1.Find("_");
	assert(start != 0);
	i1 = StringToInt(as1.Mid(start + 1));

	as2 = s2->GetTreeVariableName();
	start = as2.Find("_");
	assert(start != 0);
	i2 = StringToInt(as2.Mid(start + 1));

	if (i1 < i2)
		return -1;
	else if (i1 > i2)
		return 1;
	else
		return 0;
}

int DTLearningReportCompareSortName(const void* elem1, const void* elem2)
{
	KWAttributeStats* s1;
	KWAttributeStats* s2;
	int start;
	ALString as1;
	ALString as2;
	int i1;
	int i2;

	check(elem1);
	check(elem2);

	// Acces aux objets
	s1 = cast(KWAttributeStats*, *(Object**)elem1);
	s2 = cast(KWAttributeStats*, *(Object**)elem2);

	as1 = s1->GetAttributeName();
	start = as1.Find("_");
	assert(start != 0);
	i1 = StringToInt(as1.Mid(start + 1));

	as2 = s2->GetAttributeName();
	start = as2.Find("_");
	assert(start != 0);
	i2 = StringToInt(as2.Mid(start + 1));

	if (i1 < i2)
		return -1;
	else if (i1 > i2)
		return 1;
	else
		return 0;
}
