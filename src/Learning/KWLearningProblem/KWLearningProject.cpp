// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProject.h"

KWLearningProject::KWLearningProject() {}

KWLearningProject::~KWLearningProject() {}

void KWLearningProject::Start(int argc, char** argv)
{
	ALString sTmp;

	// Enregistrements des drivers pour l'acces au fichiers (hdfs,s3 ...)
	// On doit le faire ici, avant la gestion des options de ligne de commandes, qui peuvent
	// concerner des fichiers sur un systeme de fichier distant
	// Note: en cas d'erreur, les messages sont dans la console
	// Et avant l'ouverture du fichier du MemoryStatsManager
	SystemFileDriverCreator::RegisterExternalDrivers();

	// Parametrage des logs memoires depuis les variables d'environnement
	//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
	// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
	// Sinon, on ne fait rien, sans message d'erreur
	// Pour avoir toutes les stats: KhiopsMemStatsLogToCollect=16383
	// Pour la trace des IO: KhiopsIOTraceMode
	if (GetIOTraceMode())
		FileService::SetIOStatsActive(true);
	MemoryStatsManager::OpenLogFileFromEnvVars(true);

	// Parametrage si necessaire d'un mode de fonctionnement basique des boites de dialogue de type FileChooser
	if (GetLearningRawGuiModeMode())
		UIFileChooserCard::SetDefaultStyle("");

	// Parametrage de la gestion des traces paralleles
	if (GetParallelTraceMode() >= 1)
		PLParallelTask::SetTracerResources(1);
	if (GetParallelTraceMode() >= 2)
	{
		PLParallelTask::SetTracerResources(2);
		PLParallelTask::SetTracerProtocolActive(true);
	}
	if (GetParallelTraceMode() >= 3)
	{
		PLParallelTask::SetTracerResources(3);
		PLParallelTask::SetTracerMPIActive(true);
	}
	// Ajout du nom du host dans les logs
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog(
		    sTmp + "hostname [H" +
		    IntToString(RMResourceManager::GetResourceSystem()->GetHostIndex(GetProcessId())) + "]");

	// Initialisation de l'environnement d'apprentissage
	MemoryStatsManager::AddLog(GetClassLabel() + " OpenLearningEnvironnement Begin");
	OpenLearningEnvironnement();
	MemoryStatsManager::AddLog(GetClassLabel() + " OpenLearningEnvironnement End");

	// Parametrage du repertoire applicatif des fichiers temporaires
	// (le LearningApplicationName peut avoir ete modifie dans une sous classe)
	FileService::SetApplicationName(GetLearningApplicationName());

	// Parametrage du mode d'interface graphique en fonction des drivers de fichiers enregistres
	SetLearningDefaultRawGuiModeMode(SystemFileDriverCreator::GetExternalDriverNumber());

	// Seul endroit ou on capture les exceptions, pour le lancement du projet principal
	try
	{
		// Lancement de la partie maitre ou esclave
		if (PLParallelTask::IsMasterProcess())
			StartMaster(argc, argv);
		else
			StartSlave();
	}
	// Pour rappel, la bibliotheque Norm gere deja les cas suivant:
	//   . memory overflow pour les allocations gerees par SystemObject et ses sous classe par l'allocateur de Norm
	//   . signal (de type segmentation fault, ou ctrl break par l'utilisateur), capturees dans la classe Global
	// Les exceptions capturees ci-dessous ici sont par exemple celles de l'allocateur standard pour les allocations
	// depuis les classes systeme, comme les stream.
	//
	// Les erreurs arithmetiques de type divide by zero sont en principe capturees par un signal, mais ce
	// comportement implemente sous linux fonctionne differemment sous windows, selon les options de compilation
	// virgule flottante, selon les gestion proprietaires microsoft d'exceptions systemes par __try __except, en
	// concurrence avec la la gestion des exceptions standard C++, et selon la possibilite de parametrer un handler
	// global de gestion des exceptions non capturees. Ces specificites microsoft ne sont pas utilisees. On
	// n'utilise pas non plus la possibilite de capturer toutes les exceptions par un "catch(...)", car cela entre
	// en competition avec la gestion par signal, et on perd alors les informations precises sur la cause des
	// problemes, notamment le message de "segmentation fault" gere par un signal. Le "divide by zero" n'est pas
	// capture sous windows, mais on prefere ce compromis plutot que d'utiliser le "catch(...)" qui le capture, mais
	// sans aucune information sur la nature du probleme Enfin, des tests avec la methode set_terminate permettant
	// de positionner un handler de gestion des exceptions non captures se sont montres non concluants : on ne fait
	// que perdre des informations traitees sinon.
	catch (std::exception& e)
	{
		Global::AddFatalError("Global exception", "", e.what());
	}

	// Terminaison de l'environnment d'apprentissage
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement Begin");
	CloseLearningEnvironnement();
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement End");

	// Fermeture du fichier de stats memoire
	MemoryStatsManager::CloseLogFile();

	// Liberation des drivers de fichier
	SystemFileDriverCreator::UnregisterDrivers();
}

void KWLearningProject::Begin()
{
	// Initialisation de l'environnement d'apprentissage
	MemoryStatsManager::AddLog(GetClassLabel() + " OpenLearningEnvironnement Begin");
	OpenLearningEnvironnement();
	MemoryStatsManager::AddLog(GetClassLabel() + " OpenLearningEnvironnement End");

	// Parametrage du repertoire applicatif des fichiers temporaires
	FileService::SetApplicationName(GetLearningApplicationName());

	// Lancement de l'esclave
	if (not PLParallelTask::IsMasterProcess())
		PLParallelTask::GetDriver()->StartSlave();
}

void KWLearningProject::End()
{
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement Begin");
	CloseLearningEnvironnement();
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement End");
}

const ALString KWLearningProject::GetClassLabel() const
{
	return "Learning project";
}

void KWLearningProject::StartMaster(int argc, char** argv)
{
	Object* learningProblem;
	UIObjectView* learningProblemView;
	UIQuestionCard quitCard;
	boolean bContinue;
	ALString sTmpDirFromEnv;
	CommandLineOption* optionGetVersion;
	CommandLineOption* optionStatus;

	require(PLParallelTask::IsMasterProcess());

	// Verification que le packing des structures est correct pour KWValueBlock
	// Cette verification technique permet d'automatiser la detection de problemes
	// lies a la compilation croisee, pour le portage vers les plate-formes linux
	KWValueBlock::CheckPacking();

	// Suppression des options car dans le cas d'appeles successifs a StartMaster
	// Les options s'accumulent (on a ce cas notamment dans MODL_dll)
	UIObject::GetCommandLineOptions()->DeleteAllOptions();

	// Ajout d'options a l'interface
	UIObject::GetCommandLineOptions()->SetCommandName(GetLearningCommandName());

	// Option pour obtenir la version
	optionGetVersion = new CommandLineOption;
	optionGetVersion->SetFlag('v');
	optionGetVersion->AddDescriptionLine("print version");
	optionGetVersion->SetGroup(1);
	optionGetVersion->SetFinal(true);
	optionGetVersion->SetMethod(ShowVersion);
	optionGetVersion->SetSingle(true);
	UIObject::GetCommandLineOptions()->AddOption(optionGetVersion);

	optionStatus = new CommandLineOption;
	optionStatus->SetFlag('s');
	optionStatus->AddDescriptionLine("print system information");
	optionStatus->SetGroup(1);
	optionStatus->SetFinal(true);
	optionStatus->SetMethod(ShowSystemInformation);
	optionStatus->SetSingle(true);
	UIObject::GetCommandLineOptions()->AddOption(optionStatus);

	// Analyse de la ligne de commande
	UIObject::ParseMainParameters(argc, argv);

	// Baniere de l'application, sauf en mode batch
	if (not UIObject::IsBatchMode())
		cout << GetLearningShellBanner() << endl;

	// Parametrage du mode fast exist
	UIObject::SetFastExitMode(GetLearningFastExitMode());

	// Parametrage du repertoire temporaire via les variables d'environnement
	// (la valeur du repertoire temporaire peut etre modifiee par l'IHM)
	sTmpDirFromEnv = p_getenv("KHIOPS_TMP_DIR");
	if (sTmpDirFromEnv != "")
		FileService::SetUserTmpDir(sTmpDirFromEnv);

	// Evaluation des ressources disponibles
	PLParallelTask::GetDriver()->MasterInitializeResourceSystem();

	// Acces au projet et a sa vue
	learningProblem = CreateGenericLearningProblem();
	learningProblemView = CreateGenericLearningProblemView();

	// Ouverture de la fenetre de lancement, avec demande de confirmation
	bContinue = true;
	learningProblemView->SetObject(learningProblem);
	while (bContinue)
	{
		// Ouverture de la fenetre principale
		learningProblemView->Open();

		// Demande de confirmation
		bContinue = not quitCard.GetAnswer(learningProblemView->GetLabel(), "Warning",
						   "Are you sure you want to quit?");
	}

	// Destruction du projet et de sa vue
	delete learningProblemView;
	delete learningProblem;

	// Fermeture des fichiers de commandes input, output et erreurs
	UIObject::CleanCommandLineManagement();

	// Dechargement de la DLL jvm, potentiellement chargee soit pour l'IHM, soit pour HDFS
	// Et cela n'est pas un probleme d'appeler cette methode si la DLL jvm
	// n'a pas ete chargee
	UIObject::FreeJNIEnv();
}

void KWLearningProject::StartSlave()
{
	require(not PLParallelTask::IsMasterProcess());

	// Lancement de l'esclave
	PLParallelTask::GetDriver()->StartSlave();

	// Dechargement de la DLL jvm, potentiellement chargee  pour HDFS
	UIObject::FreeJNIEnv();
}

void KWLearningProject::OpenLearningEnvironnement()
{
	KWSTDatabaseTextFile defaultDatabaseTechnology;
	KWMTDatabaseTextFile multiTableDatabaseTechnology;

	// Parametrage du nom du module applicatif
	SetLearningModuleName("");

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops.gif");

	////////////////////////////////////////////////////////////////
	// Initialisations generales de l'environnement d'apprentissage

	// Enregistrement des technologies de bases de donnees
	KWDatabase::RegisterDatabaseTechnology(new KWSTDatabaseTextFile);
	KWDatabase::RegisterDatabaseTechnology(new KWMTDatabaseTextFile);
	KWDatabase::SetDefaultTechnologyName(multiTableDatabaseTechnology.GetTechnologyName());

	// Enregistrement des vues sur les technologies de bases de donnees (uniquement pour le maitre)
	KWDatabaseView::RegisterDatabaseTechnologyView(new KWSTDatabaseTextFileView);
	KWDatabaseView::RegisterDatabaseTechnologyView(new KWMTDatabaseTextFileView);

	// Enregistrement des regles de derivation
	KWDRRegisterAllRules();
	KWDRRegisterPredictorRules();
	KWDRRegisterPreprocessingRules();
	KWDRRegisterDataGridRules();
	KWDRRegisterTablePartitionRules();
	KWDRRegisterTableBlockRules();
	KWDRRegisterDataGridBlockRules();
	KWDRRegisterDataGridDeploymentRules();
	KWDRRegisterNBPredictorRules();
	KIDRRegisterInterpretationRules();
	KIDRRegisterAllRules();

	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODL);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new MHDiscretizerTruncationMODLHistogram);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualFrequency);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperMODL);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperBasicGrouping);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperMODLBasic);
	KWGrouper::RegisterGrouper(KWType::None, new KWGrouperBasicGrouping);
	KWGrouper::RegisterGrouper(KWType::None, new KWGrouperUnsupervisedMODL);

	// Enregistrement de predicteurs
	KWPredictor::RegisterPredictor(new KWPredictorBaseline);
	KWPredictor::RegisterPredictor(new KWPredictorUnivariate);
	KWPredictor::RegisterPredictor(new KWPredictorBivariate);
	KWPredictor::RegisterPredictor(new KWPredictorNaiveBayes);
	KWPredictor::RegisterPredictor(new SNBPredictorSelectiveNaiveBayes);
	KWPredictor::RegisterPredictor(new KWPredictorDataGrid);

	// Enregistrement de vues sur les predicteurs (uniquement pour le maitre)
	KWPredictorView::RegisterPredictorView(new SNBPredictorSelectiveNaiveBayesView);
	KWPredictorView::RegisterPredictorView(new KWPredictorDataGridView);

	////////////////////////////////////////////////////////////////
	// Initialisations des taches paralleles

	// Affectation de la version pour le maitre et les esclaves
	PLParallelTask::SetVersion(GetLearningVersion());

	// Plus de traces utilisateur en mode parallele expert
	if (GetParallelExpertMode())
		PLParallelTask::SetVerbose(true);

	// Declaration des taches paralleles
	PLParallelTask::RegisterTask(new KWFileIndexerTask);
	PLParallelTask::RegisterTask(new KWFileKeyExtractorTask);
	PLParallelTask::RegisterTask(new KWChunkSorterTask);
	PLParallelTask::RegisterTask(new KWKeySampleExtractorTask);
	PLParallelTask::RegisterTask(new KWSortedChunkBuilderTask);
	PLParallelTask::RegisterTask(new KWKeySizeEvaluatorTask);
	PLParallelTask::RegisterTask(new KWKeyPositionSampleExtractorTask);
	PLParallelTask::RegisterTask(new KWKeyPositionFinderTask);
	PLParallelTask::RegisterTask(new KWDatabaseCheckTask);
	PLParallelTask::RegisterTask(new KWDatabaseTransferTask);
	PLParallelTask::RegisterTask(new KWDatabaseBasicStatsTask);
	PLParallelTask::RegisterTask(new KWDatabaseSlicerTask);
	PLParallelTask::RegisterTask(new KWDataPreparationUnivariateTask);
	PLParallelTask::RegisterTask(new KWDataPreparationBivariateTask);
	PLParallelTask::RegisterTask(new KWClassifierEvaluationTask);
	PLParallelTask::RegisterTask(new KWRegressorEvaluationTask);
	PLParallelTask::RegisterTask(new KWClassifierUnivariateEvaluationTask);
	PLParallelTask::RegisterTask(new KWRegressorUnivariateEvaluationTask);
	PLParallelTask::RegisterTask(new SNBPredictorSelectiveNaiveBayesTrainingTask);
	PLParallelTask::RegisterTask(new KDSelectionOperandSamplingTask);
	PLParallelTask::RegisterTask(new DTDecisionTreeCreationTask);
	PLParallelTask::RegisterTask(new KDTextTokenSampleCollectionTask);
}

void KWLearningProject::CloseLearningEnvironnement()
{
	// Arret des esclaves
	PLParallelTask::GetDriver()->StopSlaves();

	// Nettoyage de l'administration des classifieurs
	KWPredictorView::DeleteAllPredictorViews();
	KWPredictor::DeleteAllPredictors();

	// Nettoyage des methodes de pretraitement
	KWDiscretizer::DeleteAllDiscretizers();
	KWGrouper::DeleteAllGroupers();

	// Nettoyage de la tache globale de creation d'attributs et de sa vue
	KDDataPreparationAttributeCreationTask::SetGlobalCreationTask(NULL);
	KDDataPreparationAttributeCreationTaskView::SetGlobalCreationTaskView(NULL);

	// Nettoyage des containers globaux
	KWClassDomain::DeleteAllDomains();
	KWDerivationRule::DeleteAllDerivationRules();

	// Nettoyage des technologies de bases de donnees
	KWDatabaseView::DeleteAllDatabaseTechnologyViews();
	KWDatabase::SetDefaultTechnologyName("");
	KWDatabase::DeleteAllDatabaseTechnologies();

	// Nettoyage des taches
	PLParallelTask::DeleteAllTasks();
}

KWLearningProblem* KWLearningProject::CreateLearningProblem()
{
	return new KWLearningProblem;
}

KWLearningProblemView* KWLearningProject::CreateLearningProblemView()
{
	return new KWLearningProblemView;
}

Object* KWLearningProject::CreateGenericLearningProblem()
{
	return CreateLearningProblem();
}

UIObjectView* KWLearningProject::CreateGenericLearningProblemView()
{
	return CreateLearningProblemView();
}

boolean KWLearningProject::ShowVersion(const ALString& sValue)
{
	cout << GetLearningApplicationName() << " ";
	if (GetLearningModuleName() != "")
		cout << GetLearningModuleName() << " ";
	cout << GetLearningVersion() << endl;
	return true;
}

boolean KWLearningProject::ShowSystemInformation(const ALString& sValue)
{
	int i;
	const SystemFileDriver* fileDriver;
	ALString sTmp;
	StringVector svEnvironmentVariables;
	ALString sEnv;
	ALString sEnvValue;
	boolean bEnvVarDefined;

	// Version
	ShowVersion(sTmp);
	cout << endl;

	// Drivers
	if (SystemFileDriverCreator::GetDriverNumber() > 0)
	{
		cout << "Drivers:" << endl;
		for (i = 0; i < SystemFileDriverCreator::GetDriverNumber(); i++)
		{
			fileDriver = SystemFileDriverCreator::GetRegisteredDriverAt(i);
			cout << "\t" << fileDriver->GetDriverName() << " (" << fileDriver->GetVersion()
			     << ") for URI scheme '" << fileDriver->GetScheme() << "'" << endl;
		}
	}

	// Affichage des variables d'environement propres a Khiops, seulement si elles sont definies
	svEnvironmentVariables.Add("KHIOPS_RAW_GUI");
	svEnvironmentVariables.Add("KHIOPS_TMP_DIR");
	svEnvironmentVariables.Add("KHIOPS_HOME");
	svEnvironmentVariables.Add("KHIOPS_API_MODE");
	svEnvironmentVariables.Add("KHIOPS_MEMORY_LIMIT");
	svEnvironmentVariables.Add("KHIOPS_DRIVERS_PATH");
	svEnvironmentVariables.Sort();
	bEnvVarDefined = false;
	cout << "Environment variables:" << endl;
	for (i = 0; i < svEnvironmentVariables.GetSize(); i++)
	{
		sEnv = svEnvironmentVariables.GetAt(i);
		sEnvValue = p_getenv(sEnv);
		if (sEnvValue != "")
		{
			cout << "\t" << sEnv << "\t" << sEnvValue << endl;
			bEnvVarDefined = true;
		}
	}
	if (not bEnvVarDefined)
		cout << "\tNone" << endl;

	svEnvironmentVariables.Initialize();
	bEnvVarDefined = false;
	svEnvironmentVariables.Add("KhiopsExpertMode");
	svEnvironmentVariables.Add("KhiopsDefaultMemoryLimit");
	svEnvironmentVariables.Add("KhiopsHardMemoryLimitMode");
	svEnvironmentVariables.Add("KhiopsCrashTestMode");
	svEnvironmentVariables.Add("KhiopsFastExitMode");
	svEnvironmentVariables.Add("KhiopsPreparationTraceMode");
	svEnvironmentVariables.Add("KhiopsIOTraceMode");
	svEnvironmentVariables.Add("KhiopsForestExpertMode");
	svEnvironmentVariables.Add("KhiopsCoclusteringExpertMode");
	svEnvironmentVariables.Add("KhiopsCoclusteringIVExpertMode");
	svEnvironmentVariables.Add("KhiopsExpertParallelMode");
	svEnvironmentVariables.Add("KhiopsParallelTrace");
	svEnvironmentVariables.Add("KhiopsFileServerActivated");
	svEnvironmentVariables.Add("hiopsPriorStudyModeRAW");
	svEnvironmentVariables.Add("KhiopsDistanceStudyMode");
	svEnvironmentVariables.Sort();

	// Affichage des variables d'environement techniques
	cout << "Internal environment variables:" << endl;
	for (i = 0; i < svEnvironmentVariables.GetSize(); i++)
	{
		sEnv = svEnvironmentVariables.GetAt(i);
		sEnvValue = p_getenv(sEnv);
		if (sEnvValue != "")
		{
			cout << "\t" << sEnv << "\t" << sEnvValue << endl;
			bEnvVarDefined = true;
		}
	}
	if (not bEnvVarDefined)
		cout << "\tNone" << endl;

	// Resources
	cout << *RMResourceManager::GetResourceSystem();

	// System
	cout << "System\n";
	cout << GetSystemInfos();
	return true;
}
