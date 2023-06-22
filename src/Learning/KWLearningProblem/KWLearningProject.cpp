// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProject.h"

KWLearningProject::KWLearningProject() {}

KWLearningProject::~KWLearningProject() {}

void KWLearningProject::Start(int argc, char** argv)
{
	ALString sTmp;

	if (GetParallelTraceMode() < 3)
	{
		if (PLParallelTask::GetTracerResources() == 0)
			PLParallelTask::SetTracerResources(GetParallelTraceMode());
	}
	else
	{
		if (GetParallelTraceMode() == 3)
		{
			PLParallelTask::SetTracerResources(2);
			PLParallelTask::SetTracerProtocolActive(true);
		}
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

	// Enregsitrements des drivers pour l'acces au fichiers (hdfs,s3 ...)
	SystemFileDriverCreator::RegisterDrivers();

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
	// Les exceptions capturees ci-dessous ici sont par exemple celle de l'allocateur standard pour les allocations
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
	// capture pasous windows, mais on prefere ce compromis plutot que d'utiliser le "catch(...)" qui le capture,
	// mais sans aucune information sur la nature du probleme Enin, des tests avec la methode set_terminate
	// permettant de positionner un handler de gestion des exceptions non captures se sont montres non concluants:
	// on ne fait que perdre des informations traitees sinon.
	catch (std::exception& e)
	{
		Global::AddFatalError("Global exception", "", e.what());
	}

	// Liberation des drivers de fichier
	SystemFileDriverCreator::UnregisterDrivers();

	// Terminaison de l'environnment d'apprentissage
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement Begin");
	CloseLearningEnvironnement();
	MemoryStatsManager::AddLog(GetClassLabel() + " CloseLearningEnvironnement End");
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
	CommandLineOption* optionLicenceInfo;
	CommandLineOption* optionLicenceUpdate;
	CommandLineOption* optionGetVersion;

	require(PLParallelTask::IsMasterProcess());

	// Verification que le packing des structures est correct pour KWValueBlock
	// Cette verification technique permet d'automatiser la detection de problemes
	// lies a la compilation croisee, pour le portage vers les plate-formes linux
	KWValueBlock::CheckPacking();

	// Ajout d'options a l'interface
	UIObject::GetCommandLineOptions()->SetCommandName(GetLearningCommandName());

	optionLicenceInfo = new CommandLineOption;
	optionLicenceInfo->SetFlag('l');
	optionLicenceInfo->AddDescriptionLine(
	    "print information to get a license: Computer name, Machine ID and remaining days");
	optionLicenceInfo->AddDescriptionLine("exit with code 0 if khiops finds an active license (code 1 otherwise)");
	optionLicenceInfo->SetSingle(true);
	optionLicenceInfo->SetFinal(true);
	optionLicenceInfo->SetGroup(1);
	optionLicenceInfo->SetMethod(ShowLicenseInfo);
	UIObject::GetCommandLineOptions()->AddOption(optionLicenceInfo);

	optionLicenceUpdate = new CommandLineOption;
	optionLicenceUpdate->SetFlag('u');
	optionLicenceUpdate->AddDescriptionLine("update license");
	optionLicenceUpdate->AddDescriptionLine(
	    "exit with code 0 if the license is successfully updated (code 1 otherwise)");
	optionLicenceUpdate->SetGroup(1);
	optionLicenceUpdate->SetFinal(true);
	optionLicenceUpdate->SetMethod(UpdateLicense);
	optionLicenceUpdate->SetParameterRequired(true);
	optionLicenceUpdate->SetParameterDescription(CommandLineOption::sParameterFile);
	UIObject::GetCommandLineOptions()->AddOption(optionLicenceUpdate);

	optionGetVersion = new CommandLineOption;
	optionGetVersion->SetFlag('v');
	optionGetVersion->AddDescriptionLine("print version");
	optionGetVersion->SetGroup(1);
	optionGetVersion->SetFinal(true);
	optionGetVersion->SetMethod(ShowVersion);
	optionGetVersion->SetSingle(true);
	UIObject::GetCommandLineOptions()->AddOption(optionGetVersion);

	// Analyse de la ligne de commande
	UIObject::ParseMainParameters(argc, argv);

	// Baniere de l'application, sauf en mode batch
	if (not UIObject::IsBatchMode())
		cout << GetLearningShellBanner() << endl;

	// Affichage du start des licences
	LMLicenseManager::ShowLicenseStatus();

	// Parametrage du repertoire temporaire via les variables d'environnement
	// (la valeur du repertoire temporaire peut etre modifiee par l'IHM)
	sTmpDirFromEnv = p_getenv("KhiopsTmpDir");
	if (sTmpDirFromEnv != "")
		FileService::SetUserTmpDir(sTmpDirFromEnv);

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

	// Fermeture des fichiers input output et erreurs
	UIObject::CloseCommandFiles();

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

	// Initialisation de la gestion des licences
	// Fonctionnalites de Modeling et de Scoring
	if (GetProcessId() == 0)
		LMLicenseManager::Initialize();

	// Parametrage du nom du module applicatif
	SetLearningModuleName("");

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops.gif");

	////////////////////////////////////////////////////////////////
	// Initialisations generales de l'environnement d'apprentissage

	// Enregistrement des technologies de bases de donnees
	KWDatabase::RegisterDatabaseTechnology(new KWSTDatabaseTextFile);
	KWDatabase::RegisterDatabaseTechnology(new KWMTDatabaseTextFile);
	if (GetLearningMultiTableMode())
		KWDatabase::SetDefaultTechnologyName(multiTableDatabaseTechnology.GetTechnologyName());
	else
		KWDatabase::SetDefaultTechnologyName(defaultDatabaseTechnology.GetTechnologyName());

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
	KWDRRegisterDataGridDeploymentRules();
	KWDRRegisterNBPredictorRules();

	// Enregistrement des methodes de pretraitement
	KWDiscretizer::RegisterDiscretizer(new KWDiscretizerMODL);
	KWDiscretizer::RegisterDiscretizer(new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(new KWDiscretizerEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(new KWDiscretizerMODLEqualWidth);
	KWDiscretizer::RegisterDiscretizer(new KWDiscretizerMODLEqualFrequency);
	KWGrouper::RegisterGrouper(new KWGrouperMODL);
	KWGrouper::RegisterGrouper(new KWGrouperBasicGrouping);
	KWGrouper::RegisterGrouper(new KWGrouperMODLBasic);

	// Enregistrement de predicteurs
	KWPredictor::RegisterPredictor(new KWPredictorBaseline);
	KWPredictor::RegisterPredictor(new KWPredictorUnivariate);
	KWPredictor::RegisterPredictor(new KWPredictorBivariate);
	KWPredictor::RegisterPredictor(new KWPredictorNaiveBayes);
	if (GetForceSNBV9ExpertMode())
		KWPredictor::RegisterPredictor(new KWPredictorSelectiveNaiveBayes);
	else
		KWPredictor::RegisterPredictor(new SNBPredictorSelectiveNaiveBayes);
	KWPredictor::RegisterPredictor(new KWPredictorDataGrid);

	// Enregistrement de vues sur les predicteurs (uniquement pour le maitre)
	if (GetForceSNBV9ExpertMode())
		KWPredictorView::RegisterPredictorView(new KWPredictorSelectiveNaiveBayesView);
	else
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
	PLParallelTask::RegisterTask(new KWPredictorEvaluationTask);
	PLParallelTask::RegisterTask(new KWClassifierEvaluationTask);
	PLParallelTask::RegisterTask(new KWRegressorEvaluationTask);
	PLParallelTask::RegisterTask(new KWClassifierUnivariateEvaluationTask);
	PLParallelTask::RegisterTask(new KWRegressorUnivariateEvaluationTask);
	if (not GetForceSNBV9ExpertMode())
	{
		PLParallelTask::RegisterTask(new SNBPredictorSNBDirectTrainingTask);
		PLParallelTask::RegisterTask(new SNBPredictorSNBEnsembleTrainingTask);
	}
	PLParallelTask::RegisterTask(new KDSelectionOperandExtractionTask);
}

void KWLearningProject::CloseLearningEnvironnement()
{
	// Arret des esclaves
	PLParallelTask::GetDriver()->StopSlaves();

	// Terminaison de la gestion des licences
	LMLicenseManager::Close();

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

boolean KWLearningProject::ShowLicenseInfo(const ALString& sParam)
{
	int nRemainingDays;

	nRemainingDays = LMLicenseManager::GetRemainingDays();
	cout << "License information:" << endl;
	cout << "Computer name"
	     << "\t" << LMLicenseService::GetComputerName() << endl;
	cout << "Machine ID    "
	     << "\t" << LMLicenseService::GetMachineID() << endl;
	cout << "Remaining days"
	     << "\t" << nRemainingDays << endl;
	cout << "To obtain or renew your license, go to www.khiops.com" << endl;

	return nRemainingDays != 0;
}

boolean KWLearningProject::UpdateLicense(const ALString& sFileName)
{
	return LMLicenseManager::UpdateLicenseFromFile(sFileName);
}

boolean KWLearningProject::ShowVersion(const ALString&)
{
	cout << GetLearningApplicationName() << " ";
	if (GetLearningModuleName() != "")
		cout << GetLearningModuleName() << " ";
	cout << GetLearningVersion() << endl;
	return true;
}

#ifdef KWLearningBatchMode
#ifndef __ANDROID__
/********************************************************************
 * Le source suivant permet de compiler des sources developpes avec *
 * l'environnement Norm, d'utiliser le mode UIObject::Textual et    *
 * de ne pas linker avec jvm.lib (a eviter absoluement).            *
 * Moyennant ces conditions, on peut livrer un executable en mode   *
 * textuel ne necessitant pas l'intallation prealable du JRE Java   *
 ********************************************************************/

extern "C"
{
#ifdef _MSC_VER
	int __stdcall _imp__JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(0);
	}
#endif // _MSC_VER

#ifdef __UNIX__
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(0);
	}
#endif // __UNIX__
}
#endif // __ANDROID__
#endif // KWLearningBatchMode