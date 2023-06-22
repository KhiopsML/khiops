// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLParallelTask.h"

PLTaskDriver* PLParallelTask::currentDriver = &PLTaskDriver::driver;
boolean PLParallelTask::bIsRunning = false;
ObjectDictionary* PLParallelTask::odTasks = NULL;
ALString PLParallelTask::sParallelLogFileName = "";
boolean PLParallelTask::bParallelSimulated = false;
int PLParallelTask::nSimulatedSlaveNumber = 8;
PLParallelTask::METHOD PLParallelTask::method = PLParallelTask::NONE;
ALString PLParallelTask::sVersion = "";
boolean PLParallelTask::bVerbose = false;
int PLParallelTask::nInstanciatedTaskNumber = 0;
int PLParallelTask::nRunNumber = 0;
boolean PLParallelTask::bTracerMPIActive = false;
boolean PLParallelTask::bTracerProtocolActive = false;
int PLParallelTask::nTracerResources = 0;
ALString PLParallelTask::sCurrentTaskName = "";
ObjectArray* PLParallelTask::oaUserMessages = NULL;

PLParallelTask::PLParallelTask()
{
	runningMode = SEQUENTIAL;
	nWarningsNumber = 0;
	nErrorsNumber = 0;
	nMessagesNumber = 0;
	nSimpleMessagesNumber = 0;
	nTaskProcessedNumber = 0;
	nNextWorkingSlaveRank = -1;
	bIsJobDone = false;
	bJobIsTerminated = false;
	bSlaveFatalError = false;
	bUserBreak = false;
	bCanRegisterSharedparameter = false;
	bCanRegisterSharedparameter = true;
	requirements = new RMTaskResourceRequirement;
	nInstanciatedTaskNumber++;
	bLastRunOk = true;
	bIsTaskInterruptedByUser = false;
	stats_IOReadDuration.SetDescription("IO Read");
	stats_IORemoteReadDuration.SetDescription("IO remote read");
	stats_ProcessDuration.SetDescription("SlaveProcess time");
	sPerformanceTaskName = "";
	bIsSlaveInitialized = false;
	lGlobalLineNumber = 0;
	bAddLocalGenericErrorCall = false;
	bSlaveInitializeErrorsOnce = true;
	bSlaveFinalizeErrorsOnce = true;
	bSlaveAtRestWithoutProcessing = false;

	// Declaration des variables partagees qui contiennent les constantes systeme
	DeclareSharedParameter(&shared_sUserTmpDir);
	DeclareSharedParameter(&shared_sApplicationName);
	DeclareSharedParameter(&input_bVerbose);
	DeclareSharedParameter(&shared_nMaxErrorFlowNumber);
	DeclareSharedParameter(&shared_nCrashTestMode);
	DeclareSharedParameter(&shared_slaveResourceRequirement);
	DeclareSharedParameter(&shared_sTaskUserLabel);
	DeclareSharedParameter(&shared_bBoostedMode);
	DeclareTaskInput(&input_bSilentMode);
	DeclareTaskInput(&input_nTaskProcessedNumber);
	DeclareTaskOutput(&output_statsIOReadDuration);
	DeclareTaskOutput(&output_dProcessingTime);
	DeclareTaskOutput(&output_statsIORemoteReadDuration);
	DeclareTaskOutput(&output_lLocalLineNumber);
	DeclareTaskOutput(&output_bSlaveProcessOk);

	// Initialisation des parametres partages
	shared_sUserTmpDir.SetValue(FileService::GetUserTmpDir());
	shared_sApplicationName.SetValue(FileService::GetApplicationName());
	shared_nMaxErrorFlowNumber = Global::GetMaxErrorFlowNumber();
	shared_nCrashTestMode = CRASH_NONE;
	shared_bBoostedMode = false;

	PLParallelTask::GetDriver()->GetIOReadingStats()->Reset();
	PLParallelTask::GetDriver()->GetIORemoteReadingStats()->Reset();
}

PLParallelTask::~PLParallelTask()
{
	delete requirements;
}

void PLParallelTask::SetDriver(PLTaskDriver* driverValue)
{
	require(not bIsRunning);
	require(driverValue != NULL);

	// On ne peut changer que si le driver est sequentiel, pour un driver parallele
	require(currentDriver->GetTechnology() == PLTaskDriver::GetDriver()->GetTechnology() and
		driverValue->GetTechnology() != PLTaskDriver::GetDriver()->GetTechnology());

	// Affectation du nouveau driver
	currentDriver = driverValue;
}

PLTaskDriver* PLParallelTask::GetDriver()
{
	return currentDriver;
}

double PLParallelTask::GetJobElapsedTime() const
{
	require(bIsJobDone);
	return tJob.GetElapsedTime();
}

double PLParallelTask::GetMasterElapsedTime() const
{
	require(bIsJobDone);
	return tMaster.GetElapsedTime() + tMasterInitialize.GetElapsedTime() + tMasterFinalize.GetElapsedTime();
}

double PLParallelTask::GetMasterInitializeElapsedTime() const
{
	require(bIsJobDone);
	return tMasterInitialize.GetElapsedTime();
}

double PLParallelTask::GetMasterFinalizeElapsedTime() const
{
	require(bIsJobDone);
	return tMasterFinalize.GetElapsedTime();
}

void PLParallelTask::CleanJobResults()
{
	tJob.Reset();
	tMaster.Reset();
	tMasterInitialize.Reset();
	tMasterFinalize.Reset();

	bIsJobDone = false;
	bIsRunning = false;
	bLastRunOk = true;
	bIsTaskInterruptedByUser = false;
}

void PLParallelTask::SetVersion(const ALString& sValue)
{
	require(sValue != "");
	sVersion = sValue;
}

ALString& PLParallelTask::GetVersion()
{
	assert(sVersion != "");
	return sVersion;
}

boolean PLParallelTask::IsJobDone() const
{
	return bIsJobDone;
}

boolean PLParallelTask::IsJobSuccessful() const
{
	require(bIsJobDone);
	return bLastRunOk;
}

boolean PLParallelTask::IsParallelModeAvailable()
{
	return GetDriver()->IsParallelModeAvailable();
}

boolean PLParallelTask::IsTaskInterruptedByUser() const
{
	require(bIsJobDone);
	return bIsTaskInterruptedByUser;
}

boolean PLParallelTask::IsInSlaveMethod() const
{
	switch (method)
	{
	case PLParallelTask::SLAVE_INITIALIZE:
		return true;
	case PLParallelTask::SLAVE_FINALIZE:
		return true;
	case PLParallelTask::SLAVE_PROCESS:
		return true;
	default:
		return false;
	}
}

boolean PLParallelTask::IsInMasterMethod() const
{
	switch (method)
	{
	case MASTER_INITIALIZE:
		return true;
	case MASTER_AGGREGATE:
		return true;
	case MASTER_PREPARE_INPUT:
		return true;
	case MASTER_FINALIZE:
		return true;
	default:
		return false;
	}
}

RMTaskResourceRequirement* PLParallelTask::GetResourceRequirements() const
{
	return requirements;
}

const RMTaskResourceRequirement* PLParallelTask::GetConstResourceRequirements() const
{
	require(IsInMasterMethod());
	return requirements;
}

const ALString PLParallelTask::GetTaskLabel() const
{
	if (sTaskUserLabel != "")
		return sTaskUserLabel;
	else
		return GetTaskName();
}

const ALString& PLParallelTask::GetTaskUserLabel() const
{
	return sTaskUserLabel;
}

void PLParallelTask::SetTaskUserLabel(const ALString& sValue)
{
	sTaskUserLabel = sValue;
}

const ALString PLParallelTask::GetClassLabel() const
{
	return GetTaskLabel();
}

void PLParallelTask::RegisterTask(PLParallelTask* plTask)
{
	require(plTask != NULL);
	require(plTask->GetTaskName() != "");
	require(odTasks == NULL or odTasks->Lookup(plTask->GetTaskSignature()) == NULL);

	// Creation si necessaire du dictionnaire de taches
	if (odTasks == NULL)
		odTasks = new ObjectDictionary;

	// Memorisation de la tache
	odTasks->SetAt(plTask->GetTaskSignature(), plTask);
}

PLParallelTask* PLParallelTask::LookupTask(const ALString& sSignature)
{
	require(sSignature != "");

	// Creation si necessaire du dictionnaire de taches
	if (odTasks == NULL)
		odTasks = new ObjectDictionary;
	return cast(PLParallelTask*, odTasks->Lookup(sSignature));
}

void PLParallelTask::DeleteAllTasks()
{
	if (odTasks != NULL)
	{
		odTasks->DeleteAll();
		delete odTasks;
		odTasks = NULL;
	}
	ensure(odTasks == NULL);
}

void PLParallelTask::SetParallelLogFileName(const ALString& sValue)
{
	sParallelLogFileName = sValue;
}

const ALString& PLParallelTask::GetParallelLogFileName()
{
	return sParallelLogFileName;
}

void PLParallelTask::SetCrashTestMode(int nMode)
{
	require(nMode < PLParallelTask::CRASH_TEST_NUMBER);
	shared_nCrashTestMode = nMode;
}

boolean PLParallelTask::Run()
{
	boolean bOk;
	ALString sTmp;
	PLTaskDriver* oldDriver;
	ALString sMessage;
	boolean bNothingToDo; // Vrai si il y a au max 0 slaveProcess

	// Verification que la tache est enregistree
	require(PLParallelTask::LookupTask(GetTaskSignature()) != NULL);

	// Sortie au plus tot en cas de demande utilisateur
	if (TaskProgression::IsInterruptionRequested())
	{
		bIsJobDone = true;
		bLastRunOk = false;
		bIsTaskInterruptedByUser = true;
		return false;
	}

	// Trace memoire
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " Run Begin");

	// Initialisations
	bCanRegisterSharedparameter = false;
	bIsJobDone = false;
	nTaskProcessedNumber = 0;
	oldDriver = currentDriver;
	bOk = true;
	nSlaveTaskIndex = 0;
	bNothingToDo = false;
	nRunNumber++;
	sCurrentTaskName = GetTaskName();
	bSlaveAtRestWithoutProcessing = false;
	input_bVerbose = bVerbose;

	// Controle du flux des erreurs
	Global::ActivateErrorFlowControl();

	// Evaluation des ressources disponibles
	if (runningMode != SLAVE)
	{
		bIsRunning = true;

		// Specification des exigences
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog("Task " + GetTaskName() + " .ComputeResourceRequirements Begin");
		bOk = ComputeResourceRequirements();
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog("Task " + GetTaskName() + " .ComputeResourceRequirements End");
		bNothingToDo = GetResourceRequirements()->GetMaxSlaveProcessNumber() == 0;

		// Allocation des ressources disponibles
		if (bOk)
		{
			require(GetResourceRequirements()->Check());

			bIsRunning = false;
			RMParallelResourceDriver::grantedResources = new RMTaskResourceGrant;

			// Calcul de l'allocation optimale des ressources (sauf si il y a 0 slaveProcess)
			if (not bNothingToDo)
			{
				// Lancement des serveurs de fichiers
				if (not GetParallelSimulated())
					GetDriver()->StartFileServers();

				// Allocation des ressources
				RMParallelResourceManager::ComputeGrantedResources(
				    GetResourceRequirements(), RMParallelResourceDriver::grantedResources);
				if (RMParallelResourceDriver::grantedResources->IsEmpty())
				{
					AddError(
					    RMParallelResourceDriver::grantedResources->GetMissingResourceMessage());
					bOk = false;
				}
			}

			// Affectation des shared_requirement
			shared_slaveResourceRequirement.SetRequirement(
			    GetResourceRequirements()->GetSlaveRequirement()->Clone());
			shared_sTaskUserLabel.SetValue(GetTaskUserLabel());

			// Nombre de processus qui travaillent
			nWorkingProcessNumber = RMParallelResourceDriver::grantedResources->GetSlaveNumber();
		}

		if (PLParallelTask::GetVerbose())
		{
			// TODO passer ça dans le if du dessus
			// Affichage des exigences
			AddMessage(sTmp + "requirements (min) for slave : memory " +
				   LongintToHumanReadableString(
				       GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->GetMin()) +
				   "   disk " +
				   LongintToHumanReadableString(
				       GetResourceRequirements()->GetSlaveRequirement()->GetDisk()->GetMin()));
			if (GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->GetMin() > 0 or
			    GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->GetMin() > 0)
				AddMessage(
				    sTmp + "global requirement for slave : memory " +
				    LongintToHumanReadableString(
					GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->GetMin()) +
				    "   disk " +
				    LongintToHumanReadableString(
					GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->GetMin()));
			if (GetResourceRequirements()->GetMaxSlaveProcessNumber() != INT_MAX)
				AddMessage(sTmp + "requirement : max SlaveProcess number " +
					   IntToString(GetResourceRequirements()->GetMaxSlaveProcessNumber()));

			// Affichage des ressources allouees
			AddMessage(sTmp + "granted : slave number " +
				   IntToString(RMParallelResourceDriver::grantedResources->GetSlaveNumber()));
			AddMessage(sTmp + "granted : slave (min) memory " +
				   LongintToHumanReadableString(
				       RMParallelResourceDriver::grantedResources->GetMinSlaveMemory()) +
				   "   disk " +
				   LongintToHumanReadableString(
				       RMParallelResourceDriver::grantedResources->GetMinSlaveDisk()));
		}
	}

	// Parametrage des tracers
	// MPI
	GetDriver()->GetTracerMPI()->SetActiveMode(GetTracerMPIActive());
	GetDriver()->GetTracerMPI()->SetSynchronousMode(true);
	GetDriver()->GetTracerMPI()->SetTimeDecorationMode(true);
	GetDriver()->GetTracerMPI()->SetShortDescription(false);

	// Protocole
	GetDriver()->GetTracerProtocol()->SetActiveMode(GetTracerProtocolActive());
	GetDriver()->GetTracerProtocol()->SetSynchronousMode(true);

	// Perf
	GetDriver()->GetTracerPerformance()->SetActiveMode(false);

	// interdiction d'acces aux output et input variables
	// l'acces est declare pour chaque methode publique
	SetSharedVariablesNoPermission(&oaInputVariables);
	SetSharedVariablesNoPermission(&oaOutputVariables);

	if (bOk and not bNothingToDo)
	{
		/////////////////////////////
		// Creation du driver

		if (GetParallelSimulated())
		{
			runningMode = PARALLEL_SIMULATED;

			// On passe le driver en sequentiel
			currentDriver = &PLTaskDriver::driver;
		}
		else
			// Sequentiel ou parallele ?
			if (runningMode != SLAVE)
			{
				if (currentDriver->GetTechnology() == PLTaskDriver::GetSequentialTechnology())
				{
					runningMode = SEQUENTIAL;
					if (bVerbose and
					    not RMParallelResourceDriver::grantedResources->IsSequentialTask())
						AddWarning("Slave number is not taken into account because the "
							   "execution driver is sequential");
				}
				else
				{
					if (RMParallelResourceDriver::grantedResources->IsSequentialTask())
						runningMode = SEQUENTIAL;
					else
						runningMode = MASTER;
				}
			}

		// Mise en place du driver de ressources parallele
		RMResourceManager::SetResourceDriver(RMParallelResourceDriver::GetDriver());

		// Parametrage du tracer de performance avec ecriture dans un fichier
		// (le nom du fichier est envoye aux esclaves par ailleurs)
		sPerformanceTaskName = sTmp + IntToString(nRunNumber) + "_" + GetTaskName();
		if (not GetParallelLogFileName().IsEmpty())
		{
			GetDriver()->GetTracerPerformance()->SetActiveMode(true);
			GetDriver()->GetTracerPerformance()->SetTimeDecorationMode(true);
			GetDriver()->GetTracerPerformance()->SetSynchronousMode(false);

			ALString sNewFileName =
			    FileService::BuildFileName(FileService::GetFilePrefix(GetParallelLogFileName()) + "_" +
							   sPerformanceTaskName + "_" + IntToString(GetProcessId()),
						       FileService::GetFileSuffix(GetParallelLogFileName()));

			GetDriver()->GetTracerPerformance()->SetFileName(FileService::BuildFilePathName(
			    FileService::GetPathName(GetParallelLogFileName()), sNewFileName));
		}

		// Lancement du programme
		bOk = true;
		switch (runningMode)
		{
		case PLParallelTask::SEQUENTIAL:
			tJob.Start();

			// Execution en sequentiel
			bOk = RunAsSequential();

			// Affichage de la duree
			tJob.Stop();
			if (GetDriver()->GetTracerPerformance()->GetActiveMode())
				GetDriver()->GetTracerPerformance()->AddTrace(
				    sTmp + "<< processing time in seconds [" + sPerformanceTaskName + "," +
				    IntToString(0) + "] : " + DoubleToString(tJob.GetElapsedTime()));
			if (GetVerbose())
			{
				AddMessage(sTmp + "subtask number " + IntToString(GetTaskIndex()));
				AddMessage(sTmp + "task duration " + DoubleToString(tJob.GetElapsedTime()) + " s");
			}
			break;

		case PLParallelTask::PARALLEL_SIMULATED:

			require(GetDriver()->GetTechnology() == PLTaskDriver::GetSequentialTechnology());
			tJob.Start();

			// Execution en parallele simule
			bOk = RunAsSimulatedParallel(nWorkingProcessNumber);

			// Affichage de la duree
			tJob.Stop();
			if (GetDriver()->GetTracerPerformance()->GetActiveMode())
				GetDriver()->GetTracerPerformance()->AddTrace(
				    sTmp + "<< processing time in seconds [" + sPerformanceTaskName + "," +
				    IntToString(0) + "] : " + DoubleToString(tJob.GetElapsedTime()));
			if (GetVerbose())
			{
				AddMessage(sTmp + "subtask number " + IntToString(GetTaskIndex()));
				AddMessage(sTmp + "task duration " + DoubleToString(tJob.GetElapsedTime()) + " s");
			}

			break;

		case PLParallelTask::MASTER:
			require(GetDriver() != NULL);
			require(GetDriver()->GetTechnology() == PLTaskDriver::GetMpiTechnology());

			if (GetDriver()->GetTracerMPI()->GetActiveMode())
			{
				GetDriver()->GetTracerMPI()->AddTrace("Run task " + GetTaskLabel());
			}
			else if (GetDriver()->GetTracerProtocol()->GetActiveMode())
			{
				GetDriver()->GetTracerProtocol()->AddTrace("Run task " + GetTaskLabel());
			}
			else if (GetTracerResources() > 0)
				cout << "Run task " + GetTaskLabel() << endl;

			tJob.Start();

			// Creation et lancement du maitre
			bOk = RunAsMaster();
			tJob.Stop();
			if (GetDriver()->GetTracerPerformance()->GetActiveMode())
				GetDriver()->GetTracerPerformance()->AddTrace(
				    sTmp + "<< processing time in seconds [" + sPerformanceTaskName + "," +
				    IntToString(nWorkingProcessNumber) +
				    "] : " + DoubleToString(tJob.GetElapsedTime()));

			if (GetVerbose())
			{
				AddMessage(stats_IOReadDuration.WriteString());
				AddMessage(stats_ProcessDuration.WriteString());
				AddMessage(stats_IORemoteReadDuration.WriteString());
				AddMessage(sTmp + "remote access number " +
					   IntToString(stats_IORemoteReadDuration.GetValueNumber()) + " / " +
					   IntToString(stats_IOReadDuration.GetValueNumber()));
				AddMessage(sTmp + "subtask number " + IntToString(GetTaskIndex()));
				AddMessage(sTmp + "task duration " + DoubleToString(tJob.GetElapsedTime()) + " s");
			}

			break;

		case PLParallelTask::SLAVE:
			require(GetDriver() != NULL);
			require(GetDriver()->GetTechnology() == PLTaskDriver::GetMpiTechnology());
			require(bIsRunning == false);

			bIsRunning = true;

			// Creation et lancement de l'esclave
			GetDriver()->RunSlave(this);
			bIsRunning = false;

			break;
		}

		// Ecriture des traces
		if (GetDriver()->GetTracerPerformance()->GetActiveMode() and
		    not GetDriver()->GetTracerPerformance()->GetFileName().IsEmpty())
		{
			GetDriver()->GetTracerPerformance()->PrintTracesToFile();
			GetDriver()->GetTracerPerformance()->Clean();
		}

		// Remise des variables dans l'etat initial
		SetSharedVariablesRW(&oaSharedParameters);
		SetSharedVariablesRW(&oaInputVariables);
		SetSharedVariablesRW(&oaOutputVariables);

		// Mise en place du driver de ressources par defaut : sequentiel
		RMResourceManager::SetResourceDriver(RMStandardResourceDriver::GetDriver());
	}
	bIsJobDone = true;

	// Desactivation du Controle du flux des erreurs
	Global::DesactivateErrorFlowControl();

	// Nettoyage
	if (RMParallelResourceDriver::grantedResources != NULL)
	{
		delete RMParallelResourceDriver::grantedResources;
		RMParallelResourceDriver::grantedResources = NULL;
	}

	// Arret des serveurs de fichiers
	if (runningMode != SLAVE and not bNothingToDo)
	{
		GetDriver()->StopFileServers();
	}

	// Initialisation des exigences
	delete requirements;
	requirements = new RMTaskResourceRequirement;

	// Recopie des informations necessaies entre les 2 drivers
	oldDriver->GetTracerMPI()->CopyFrom(currentDriver->GetTracerMPI());
	oldDriver->GetTracerPerformance()->CopyFrom(currentDriver->GetTracerPerformance());
	oldDriver->GetTracerProtocol()->CopyFrom(currentDriver->GetTracerProtocol());

	// Restitution du driver initial
	currentDriver = oldDriver;
	bLastRunOk = bOk;
	bIsTaskInterruptedByUser = TaskProgression::IsInterruptionRequested();
	sCurrentTaskName = "";

	// Trace memoire
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " Run End");
	return bOk;
}

void PLParallelTask::DeclareSharedParameter(PLSharedVariable* parameter)
{
	require(this->bCanRegisterSharedparameter);
	require(parameter != NULL);
	require(not parameter->bIsDeclared);

	parameter->Clean();
	parameter->bIsDeclared = true;
	parameter->SetReadWrite();
	oaSharedParameters.Add(parameter);
}

void PLParallelTask::DeclareTaskInput(PLSharedVariable* parameter)
{
	require(this->bCanRegisterSharedparameter);
	require(parameter != NULL);
	require(not parameter->bIsDeclared);

	parameter->Clean();
	parameter->bIsDeclared = true;
	parameter->SetNoPermission();
	oaInputVariables.Add(parameter);
}

void PLParallelTask::DeclareTaskOutput(PLSharedVariable* variable)
{
	require(this->bCanRegisterSharedparameter);
	require(variable != NULL);
	require(not variable->bIsDeclared);

	variable->Clean();
	variable->bIsDeclared = true;
	variable->SetNoPermission();
	oaOutputVariables.Add(variable);
}

const RMResourceGrant* PLParallelTask::GetSlaveResourceGrant() const
{
	RMResourceGrant* resource;

	// Cas du maitre
	if (IsInMasterMethod())
	{
		require(method == MASTER_PREPARE_INPUT);

		resource = RMParallelResourceDriver::grantedResources->GetResourceAtRank(nNextWorkingSlaveRank);
		assert(resource != NULL);
		ensure(resource->GetRank() == nNextWorkingSlaveRank);
	}
	// Cas de l'esclave
	else
	{
		require(IsInSlaveMethod());

		// En parallele
		if (IsParallel())
		{
			resource = RMParallelResourceDriver::grantedResources->GetResourceAtRank(GetProcessId());
			assert(resource != NULL);
			ensure(resource->GetRank() == GetProcessId());
		}
		// En sequentiel et simule
		else
		{
			resource = RMParallelResourceDriver::grantedResources->GetResourceAtRank(1);
			assert(resource != NULL);
		}
	}
	return resource;
}

const RMResourceGrant* PLParallelTask::GetMasterResourceGrant() const
{
	RMResourceGrant* resource;

	require(IsInMasterMethod());

	resource = RMParallelResourceDriver::grantedResources->GetResourceAtRank(0);
	assert(resource != NULL);
	ensure(resource->GetRank() == 0);
	return resource;
}

const RMResourceRequirement* PLParallelTask::GetSlaveResourceRequirement() const
{
	require(IsInSlaveMethod());
	return shared_slaveResourceRequirement.GetRequirement();
}

const RMTaskResourceGrant* PLParallelTask::GetTaskResourceGrant() const
{
	require(method == MASTER_INITIALIZE);
	return RMParallelResourceDriver::grantedResources;
}

void PLParallelTask::AddLocalError(const ALString& sLabel, int nLineNumber)
{
	AddLocalGenericError(Error::GravityError, sLabel, nLineNumber);
}

void PLParallelTask::AddLocalWarning(const ALString& sLabel, int nLineNumber)
{
	AddLocalGenericError(Error::GravityWarning, sLabel, nLineNumber);
}

void PLParallelTask::AddLocalMessage(const ALString& sLabel, int nLineNumber)
{
	AddLocalGenericError(Error::GravityMessage, sLabel, nLineNumber);
}

void PLParallelTask::SetLocalLineNumber(longint lValue)
{
	require(method == PLParallelTask::SLAVE_PROCESS);
	require(lValue >= 0);
	bSetLocalLineNumberCall = true;
	if (IsParallel())
		output_lLocalLineNumber = lValue;
	else
		lGlobalLineNumber += lValue;
}

void PLParallelTask::SetSlaveAtRestAfterProcess()
{
	PLSlaveState* slave;
	ALString sTmp;

	require(method == MASTER_PREPARE_INPUT);

	slave = GetSlaveWithRank(nNextWorkingSlaveRank);
	assert(slave != NULL);
	slave->SetAtRest(true);
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace(sTmp + "Set slave " + IntToString(nNextWorkingSlaveRank) +
							   " at rest");
}

void PLParallelTask::SetSlaveAtRest()
{
	SetSlaveAtRestAfterProcess();
	assert(bSlaveAtRestWithoutProcessing == false);
	bSlaveAtRestWithoutProcessing = true;
}

int PLParallelTask::GetRestingSlaveNumber() const
{
	int i;
	int nSlaveNumber;

	nSlaveNumber = 0;
	for (i = 0; i < oaSlaves.GetSize(); i++)
	{
		if (cast(PLSlaveState*, oaSlaves.GetAt(i))->GetAtRest())
			nSlaveNumber++;
	}
	return nSlaveNumber;
}

void PLParallelTask::SetAllSlavesAtWork()
{
	int i;

	require(method == MASTER_AGGREGATE);
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Set all slaves at work");
	for (i = 0; i < oaSlaves.GetSize(); i++)
	{
		cast(PLSlaveState*, oaSlaves.GetAt(i))->SetAtRest(false);
	}
}

int PLParallelTask::ComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, longint lFileProcessed,
					   longint lFileSize) const
{
	int nStepSize;
	ALString sTmp;

	require(method == PLParallelTask::MASTER_PREPARE_INPUT);
	nStepSize = InternalComputeStairBufferSize(nBufferSizeMin, nBufferSizeMax, lFileProcessed, lFileSize,
						   GetProcessNumber(), GetTaskIndex());
	GetDriver()->GetTracerMPI()->AddTrace(sTmp + "Buffer step size = " + LongintToHumanReadableString(nStepSize));
	return nStepSize;
}

const IntVector* PLParallelTask::GetGrantedSlaveProcessIds() const
{
	require(method == METHOD::MASTER_INITIALIZE);
	assert(ivGrantedSlaveIds.GetSize() == nWorkingProcessNumber);
	return &ivGrantedSlaveIds;
}

void PLParallelTask::SetToSlaveMode()
{
	require(not bIsRunning);
	runningMode = SLAVE;
}

boolean PLParallelTask::RunAsMaster()
{
	boolean bProcessingOk;
	boolean bOk;

	require(bIsRunning == false);

	// Initialisations
	bIsRunning = true;
	bProcessingOk = true;

	// Creation du repertoire temporaire
	bOk = FileService::CreateApplicationTmpDir();
	require(FileService::CheckApplicationTmpDir());
	if (not bOk)
		return false;

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	// Fonction principale du maitre
	bProcessingOk = GetDriver()->RunMaster(this);

	bIsRunning = false;
	return bProcessingOk;
}

boolean PLParallelTask::RunAsSequential()
{
	boolean bMasterInitializeOk;
	boolean bMasterFinalizeOk;
	boolean bSlaveInitializeOk;
	boolean bProcessingOk;
	boolean bSlaveFinalizeOk;
	boolean bOk;
	int i;

	require(bIsRunning == false);

	// Initialisations
	bIsRunning = true;
	bJobIsTerminated = false;
	bMasterInitializeOk = true;
	bMasterFinalizeOk = true;
	bSlaveInitializeOk = true;
	bSlaveFinalizeOk = true;
	bProcessingOk = true;
	nNextWorkingSlaveRank = 1;

	// Creation du repertoire temporaire
	bOk = FileService::CreateApplicationTmpDir();
	require(FileService::CheckApplicationTmpDir());
	if (not bOk)
		return false;

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	// Fermeture des droits en ecriture pour les output variables
	SetSharedVariablesRO(&oaOutputVariables);

	// Ajout d'un seul nesclave dans la liste
	ivGrantedSlaveIds.Add(1);

	// Initialisation du maitre
	SetProcessId(0);
	bMasterInitializeOk = CallMasterInitialize();

	// On continue si Ok
	if (bMasterInitializeOk)
	{
		SetProcessId(1);

		bSlaveInitializeOk = CallSlaveInitialize();
		if (bSlaveInitializeOk)
		{
			TaskProgression::BeginTask();
			TaskProgression::DisplayMainLabel(GetTaskLabel());
			TaskProgression::DisplayLabel(PROGRESSION_MSG_PROCESSING);

			// Boucle de traitement sur les esclaves
			bProcessingOk = SequentialLoop();
			TaskProgression::EndTask();
		}

		SetProcessId(1);
		bSlaveFinalizeOk = CallSlaveFinalize(bProcessingOk and bSlaveInitializeOk);

	} // if (bMasterInitializeOk)

	SetProcessId(0);
	bMasterFinalizeOk =
	    CallMasterFinalize(bMasterInitializeOk and bSlaveInitializeOk and bProcessingOk and bSlaveFinalizeOk);

	// Nettoyage des sharedVariables taskInput
	for (i = 0; i < oaInputVariables.GetSize(); i++)
	{
		cast(PLSharedVariable*, oaInputVariables.GetAt(i))->Clean();
	}

	ivGrantedSlaveIds.SetSize(0);

	// Terminaison
	bIsRunning = false;
	return bMasterInitializeOk and bSlaveInitializeOk and bProcessingOk and bSlaveFinalizeOk and
	       bMasterFinalizeOk and not TaskProgression::IsInterruptionRequested();
}

boolean PLParallelTask::RunAsSimulatedParallel(int nSlavesNumber)
{
	double dTaskPercentage;
	int nProgression;
	int i;
	ALString sTmp;
	PLSlaveState* slaveState;
	PLParallelTask* slaveInstance;
	ObjectArray oaSlaveInstances;
	PLSerializer parametersSerializer;
	PLSerializer inputSerializer;
	PLSerializer outputSerializer;
	boolean bMasterInitializeOk;
	boolean bMasterFinalizeOk;
	boolean bSlaveInitializeOk;
	boolean bSlaveProcessOk;
	boolean bMasterAggregateOk;
	boolean bSlaveFinalizeOk;
	boolean bMasterPrepareTaskInputOk;
	boolean bOk;
	double dGlobalProgression;

	require(bIsRunning == false);

	// Initialisation
	bMasterInitializeOk = true;
	bMasterFinalizeOk = true;
	bSlaveInitializeOk = true;
	bSlaveProcessOk = true;
	bMasterAggregateOk = true;
	bSlaveFinalizeOk = true;
	bMasterPrepareTaskInputOk = true;
	bOk = true;
	nNextWorkingSlaveRank = 0;
	bJobIsTerminated = false;
	nCurrentSlavePosition = 0;
	dGlobalProgression = 0;

	// Creation du repertoire temporaire
	bOk = FileService::CreateApplicationTmpDir();
	require(FileService::CheckApplicationTmpDir());
	if (not bOk)
		return false;

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	cout << "\n***** Warning : simulated parallel mode of " << GetTaskLabel() << " (" << nSlavesNumber
	     << " slaves) *****\n"
	     << endl;

	// Tableau indexe par le rang : le rang 0 est le maitre
	oaSlavesByRank.Add(NULL);

	// Creation des esclaves
	for (i = 0; i < nSlavesNumber; i++)
	{
		// Creation d'une nouvelle tache pour l'esclave
		// elle n'est pas initialisee par les setters comme this
		slaveInstance = Create(); // TODO CopyFrom(this);
		slaveInstance->runningMode = PARALLEL_SIMULATED;
		oaSlaveInstances.Add(slaveInstance);

		// Creation de l'etat de l'esclave
		slaveState = new PLSlaveState;
		slaveState->SetRank(i + 1);
		slaveState->SetHostName("simulated host");
		slaveState->SetState(PLSlaveState::READY);
		slaveState->SetProgression(0);
		oaSlaves.Add(slaveState);
		oaSlavesByRank.Add(slaveState);
		ivGrantedSlaveIds.Add(slaveState->GetRank());
	}

	// On brasse la liste des rangs pour que les utilisateurs de la lib
	// ne fassent pas d'hypothese sur les rangs (ordre, sequence, etc...)
	ivGrantedSlaveIds.Shuffle();

	// Initialisation du maitre
	bIsRunning = true;
	SetProcessId(0);
	bMasterInitializeOk = CallMasterInitialize();

	if (bMasterInitializeOk)
	{
		// Transfert des parametres [Master -> Slave]
		parametersSerializer.OpenForWrite(NULL);
		PLParallelTask::SerializeSharedVariables(&parametersSerializer, &oaSharedParameters, false);
		parametersSerializer.Close();
		for (i = 0; i < oaSlaveInstances.GetSize(); i++)
		{
			slaveInstance = cast(PLParallelTask*, oaSlaveInstances.GetAt(i));
			parametersSerializer.OpenForRead(NULL);
			PLParallelTask::DeserializeSharedVariables(&parametersSerializer,
								   &slaveInstance->oaSharedParameters);
			parametersSerializer.Close();
			slaveInstance->InitializeParametersFromSharedVariables();
		}

		// Initialisation des esclaves
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel(GetTaskLabel());
		TaskProgression::DisplayLabel(PROGRESSION_MSG_SLAVE_INITIALIZE);
		TaskProgression::DisplayProgression(0);
		for (i = 0; i < oaSlaveInstances.GetSize(); i++)
		{
			slaveInstance = cast(PLParallelTask*, oaSlaveInstances.GetAt(i));
			SetProcessId(cast(PLSlaveState*, oaSlaves.GetAt(i))->GetRank());
			bSlaveInitializeOk = slaveInstance->CallSlaveInitialize();
			if (not bSlaveInitializeOk)
				break;
			TaskProgression::DisplayProgression((i * 100) / oaSlaveInstances.GetSize());
		}
		TaskProgression::DisplayProgression(100);
		TaskProgression::EndTask();

		//  Boucle de travail  : preparation, processing, aggregation
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel(GetTaskLabel());
		TaskProgression::DisplayLabel(PROGRESSION_MSG_PROCESSING);

		// Lancement d'un nouveau job par un esclave
		dGlobalProgression = 0;
		bSlaveProcessOk = true;

		while (bSlaveInitializeOk and not TaskProgression::IsInterruptionRequested())
		{
			slaveState = GetReadySlave();
			assert(slaveState != NULL);

			slaveInstance = cast(PLParallelTask*, oaSlaveInstances.GetAt(slaveState->GetRank() - 1));

			SetProcessId(0);
			bMasterPrepareTaskInputOk =
			    CallMasterPrepareTaskInput(dTaskPercentage, bJobIsTerminated, slaveState);
			assert(not(bJobIsTerminated and bSlaveAtRestWithoutProcessing));

			if (bJobIsTerminated)
				dTaskPercentage = 1;
			ensure(0 <= dTaskPercentage and dTaskPercentage <= 1);

			slaveState->SetTaskPercent(dTaskPercentage);
			if (bJobIsTerminated)
				break;

			if (not bSlaveAtRestWithoutProcessing)
			{
				// Transfert des variables d'entree [Master -> Slave]
				inputSerializer.OpenForWrite(NULL);
				PLParallelTask::SerializeSharedVariables(&inputSerializer, &oaInputVariables, true);
				inputSerializer.Close();

				SetSharedVariablesRW(&slaveInstance->oaInputVariables);
				inputSerializer.OpenForRead(NULL);
				PLParallelTask::DeserializeSharedVariables(&inputSerializer,
									   &slaveInstance->oaInputVariables);
				inputSerializer.Close();
				slaveInstance->nTaskProcessedNumber = slaveInstance->input_nTaskProcessedNumber;
				SetSharedVariablesNoPermission(&slaveInstance->oaInputVariables);

				// Suivi de tache
				if (TaskProgression::IsInterruptionRequested())
					break;

				//////////////////////////////////////////////
				// Traitement principal : appel de SlaveProcess
				slaveState->SetTaskIndex(nTaskProcessedNumber);
				slaveState->SetState(PLSlaveState::PROCESSING);

				SetProcessId(slaveState->GetRank());
				bSlaveProcessOk =
				    slaveInstance
					->CallSlaveProcess(); // TODO il faut passer le crashtest au slaveinstance

				nSlaveTaskIndex = slaveState->GetTaskIndex();
				slaveState->SetState(PLSlaveState::READY);
				SetSharedVariablesRW(&oaInputVariables);

				// Suivi de tache
				if (TaskProgression::IsInterruptionRequested())
				{
					break;
				}

				// Netoyage des sharedVariables taskInput de l'esclave
				for (i = 0; i < oaInputVariables.GetSize(); i++)
				{
					cast(PLSharedVariable*, slaveInstance->oaInputVariables.GetAt(i))->Clean();
				}
				SetSharedVariablesNoPermission(&oaInputVariables);

				// Transfert des resultats de l'esclave [Slave -> Master]
				outputSerializer.OpenForWrite(NULL);
				PLParallelTask::SerializeSharedVariables(&outputSerializer,
									 &slaveInstance->oaOutputVariables, true);
				outputSerializer.Close();
				SetSharedVariablesRW(&oaOutputVariables);
				outputSerializer.OpenForRead(NULL);
				PLParallelTask::DeserializeSharedVariables(&outputSerializer, &oaOutputVariables);
				outputSerializer.Close();

				if (not bSlaveProcessOk)
				{
					if (bVerbose and not TaskProgression::IsInterruptionRequested())
						AddError(sTmp + "An error occurred in worker " +
							 IntToString(slaveState->GetRank()) + " processing (subtask " +
							 IntToString(GetTaskIndex()) + ")");
					break;
				}

				// Affichage de la progression globale
				dGlobalProgression += dTaskPercentage;
				nProgression = (int)floor(dGlobalProgression * 100);
				if (nProgression > 100)
					nProgression = 100;
				TaskProgression::DisplayProgression(nProgression);

				//////////////////////////////////////////
				// Aggregation des resultats
				SetProcessId(0);
				bMasterAggregateOk = CallMasterAggregate();
				if (not bMasterAggregateOk)
					break;

				nTaskProcessedNumber++;
			}

			// Reinitialisation du flag qui est mis a jour dans MasterPrepareTaskInput
			bSlaveAtRestWithoutProcessing = false;

		} // while (not bInterruptionRequested)
		TaskProgression::EndTask();

		//////////////////////////////////////////////
		// Finalisation des esclaves
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel(GetTaskLabel());
		TaskProgression::DisplayLabel(PROGRESSION_MSG_SLAVE_FINALIZE);
		bOk = true;
		for (i = 0; i < oaSlaves.GetSize(); i++)
		{
			TaskProgression::BeginTask();
			TaskProgression::DisplayMainLabel(GetTaskLabel() + " worker " + IntToString(i) + "/" +
							  IntToString(oaSlaveInstances.GetSize()));
			slaveState = cast(PLSlaveState*, oaSlaves.GetAt(i));
			slaveInstance = cast(PLParallelTask*, oaSlaveInstances.GetAt(slaveState->GetRank() - 1));

			// Seulement si ils ont ete initialises
			if (slaveInstance->bIsSlaveInitialized == true)
			{
				SetProcessId(slaveState->GetRank());
				bOk = slaveInstance->CallSlaveFinalize(bSlaveInitializeOk and bSlaveProcessOk);
				bSlaveFinalizeOk = bSlaveFinalizeOk and bOk;
			}
			TaskProgression::EndTask();
			TaskProgression::DisplayProgression(i * 100 / oaSlaves.GetSize());
		}
		TaskProgression::EndTask();
	}

	//////////////////////////////////////////////
	// Finalisation du maitre
	SetProcessId(0);
	bMasterFinalizeOk = CallMasterFinalize(bSlaveProcessOk and bMasterInitializeOk and bMasterAggregateOk and
					       bSlaveFinalizeOk and bSlaveInitializeOk);
	bIsRunning = false;
	// Nettoyage
	oaSlaveInstances.DeleteAll();
	oaSlaves.DeleteAll();
	ivGrantedSlaveIds.SetSize(0);

	return bSlaveInitializeOk and bSlaveProcessOk and bSlaveFinalizeOk and bMasterInitializeOk and
	       bMasterPrepareTaskInputOk and bMasterAggregateOk and bMasterFinalizeOk and
	       not TaskProgression::IsInterruptionRequested();
}

boolean PLParallelTask::SequentialLoop()
{
	boolean bSlaveProcessOk;
	boolean bMasterAggregateOk;
	boolean bMasterPrepareTaskInputOk;
	ALString sTmp;
	int i;
	int nProgression;
	double dTaskPercentage;
	PLSlaveState* slave;
	double dGlobalProgression;

	bSlaveProcessOk = true;
	bMasterAggregateOk = true;
	bMasterPrepareTaskInputOk = true;
	dGlobalProgression = 0;

	// On ajoute le seul esclave a la liste des esclaves
	slave = new PLSlaveState();
	slave->SetRank(1);
	slave->SetState(PLSlaveState::READY);
	oaSlaves.Add(slave);
	oaSlavesByRank.Add(NULL);
	oaSlavesByRank.Add(slave);

	while (not bUserBreak)
	{
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Netoyage des sharedVariables taskInput
		for (i = 0; i < oaInputVariables.GetSize(); i++)
		{
			cast(PLSharedVariable*, oaInputVariables.GetAt(i))->Clean();
		}

		// Construction des input de la prochaine tache a effectuer
		// et calcul de la taille de la tache en % du travail total
		SetProcessId(0);
		bMasterPrepareTaskInputOk = CallMasterPrepareTaskInput(dTaskPercentage, bJobIsTerminated, slave);

		// Arret si termine
		if (bJobIsTerminated)
			break;

		if (not bSlaveAtRestWithoutProcessing)
		{
			// Mise a jour du numero de la tache
			input_nTaskProcessedNumber.SetReadWrite();
			input_nTaskProcessedNumber = nTaskProcessedNumber;
			input_nTaskProcessedNumber.SetReadOnly();

			SetProcessId(1);
			bSlaveProcessOk = CallSlaveProcess();

			// Traitement par appel de l'esclave (en gerant les acces aux variables partagees)
			nSlaveTaskIndex = nTaskProcessedNumber;

			if (not bSlaveProcessOk or TaskProgression::IsInterruptionRequested())
				break;

			// Affichage de la progression globale
			dGlobalProgression += dTaskPercentage;
			nProgression = (int)floor(dGlobalProgression * 100);
			if (nProgression > 100)
				nProgression = 100;
			TaskProgression::DisplayProgression(nProgression);

			SetProcessId(0);
			bMasterAggregateOk = CallMasterAggregate();
			if (not bMasterAggregateOk)
				break;

			// Incrementation du numero de la tache
			nTaskProcessedNumber++;
		}

		// Reinitialisation du flag qui est mis a jour dans MasterPrepareTaskInput
		bSlaveAtRestWithoutProcessing = false;
	}
	// Nettoyage
	oaSlaves.DeleteAll();
	return bMasterAggregateOk and bSlaveProcessOk and bMasterPrepareTaskInputOk;
}

ALString PLParallelTask::GetTaskSignature() const
{
	return GetTaskName() + "_" + IntToString(oaInputVariables.GetSize()) + "_" +
	       IntToString(oaOutputVariables.GetSize()) + "_" + IntToString(oaSharedParameters.GetSize());
}

ALString PLParallelTask::GetCurrentTaskName()
{
	return sCurrentTaskName;
}

boolean PLParallelTask::CallSlaveInitialize()
{
	boolean bOk;

	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< Begin SlaveInitialize");

	// Fermeture des droits en ecriture pour les shared et les input variables
	SetSharedVariablesRO(&oaSharedParameters);
	SetSharedVariablesRO(&oaInputVariables);
	require(method == NONE);
	method = SLAVE_INITIALIZE;
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("In SlaveInitialize");
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveInitialize Begin");
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskLabel());
	TaskProgression::DisplayLabel(PROGRESSION_MSG_SLAVE_INITIALIZE);

	// Lancement de la methode applicative
	bOk = SlaveInitialize();

	TaskProgression::EndTask();
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveInitialize End");
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Out SlaveInitialize", bOk);
	method = NONE;

	// CrashTest
	if (GetCrashTestMode() == CRASH_SLAVE_INITIALIZE)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}

	if (bVerbose and not bOk and not TaskProgression::IsInterruptionRequested())
		AddError("An error occurred in worker initialization");

	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< End SlaveInitialize");

	bIsSlaveInitialized = true;
	return bOk;
}

boolean PLParallelTask::CallSlaveProcess()
{
	boolean bOk;
	ALString sTmp;

	SetSharedVariablesRW(&oaOutputVariables);
	SetSharedVariablesRO(&oaInputVariables);

	// Initialisation
	require(method == NONE);
	method = SLAVE_PROCESS;
	bAddLocalGenericErrorCall = false;
	bSetLocalLineNumberCall = false;
	lGlobalLineNumber = 0;

	TaskProgression::BeginTask();
	TaskProgression::DisplayProgression(0);

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	// Mise a jour des attributs recus en input
	nTaskProcessedNumber = input_nTaskProcessedNumber;

	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< Begin SlaveProcess");

	// Traitement principal de l'esclave
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace(sTmp + "In SlaveProcess (task " +
							   IntToString(GetTaskIndex()) + ")");
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveProcess " + IntToString(GetTaskIndex()) +
					   " Begin");

	bOk = SlaveProcess();
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveProcess " + IntToString(GetTaskIndex()) +
					   " End");
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace(
		    sTmp + "Out SlaveProcess (task " + IntToString(GetTaskIndex()) + ")", bOk);

	// Si on appelle AddLocalGenericErrorCall il faut appeler
	// SetLocalLineNumberCall a la fin du SlaveProcess
	assert(not bAddLocalGenericErrorCall or bSetLocalLineNumberCall);
	method = NONE;

	// CrashTest
	if (GetCrashTestMode() == CRASH_SLAVE_PROCESS and GetTaskIndex() == 0)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}

	if (bVerbose and not bOk and not TaskProgression::IsInterruptionRequested())
		AddError(sTmp + "An error occurred in worker while processing (subtask " + IntToString(GetTaskIndex()) +
			 ")");

	SetSharedVariablesNoPermission(&oaOutputVariables);
	SetSharedVariablesNoPermission(&oaInputVariables);

	TaskProgression::EndTask();
	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< End SlaveProcess");
	return bOk;
}

boolean PLParallelTask::CallSlaveFinalize(boolean bProcessOk)
{
	boolean bOk;
	require(method == NONE);

	// CrashTest
	if (GetCrashTestMode() == CRASH_SLAVE_FINALIZE)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}

	method = SLAVE_FINALIZE;

	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< Begin SlaveFinalize");
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("In SlaveFinalize", bProcessOk);
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveFinalize Begin");
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskLabel());
	TaskProgression::DisplayLabel(PROGRESSION_MSG_SLAVE_FINALIZE);
	bOk = SlaveFinalize(bProcessOk);
	TaskProgression::EndTask();
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .SlaveFinalize End");
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Out SlaveFinalize", bOk);
	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< End SlaveFinalize");
	method = NONE;

	if (bVerbose and not bOk and not TaskProgression::IsInterruptionRequested())
		AddError("An error occurred in worker finalization");

	return bOk;
}

boolean PLParallelTask::CallMasterInitialize()
{
	boolean bOk;
	tMasterInitialize.Start();
	require(method == NONE);
	method = MASTER_INITIALIZE;
	if (GetDriver()->GetTracerPerformance()->GetActiveMode())
		GetDriver()->GetTracerPerformance()->AddTrace("<< Job start [" + sPerformanceTaskName + "," +
							      GetLocalHostName() + "," +
							      IntToString(nWorkingProcessNumber) + "]");

	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("In MasterInitialize [" + GetTaskName() + "]");
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskLabel());
	TaskProgression::DisplayLabel(PROGRESSION_MSG_MASTER_INITIALIZE);
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterInitialize Begin");
	bOk = MasterInitialize();

	// Fin de l'autorisation d'ecriture des shared parameters
	SetSharedVariablesRO(&oaSharedParameters);
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterInitialize End");
	TaskProgression::EndTask();
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Out MasterInitialize", bOk);
	method = NONE;
	tMasterInitialize.Stop();

	// CrashTest
	if (GetCrashTestMode() == CRASH_MASTER_INITIALIZE)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}

	if (bVerbose and not bOk and not TaskProgression::IsInterruptionRequested())
		AddError("An error occurred in task initialisation");

	return bOk;
}

boolean PLParallelTask::CallMasterFinalize(boolean bProcessOk)
{
	boolean bOk;

	// CrashTest
	if (GetCrashTestMode() == CRASH_MASTER_FINALIZE)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}

	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("In MasterFinalize", bProcessOk);
	tMasterFinalize.Start();
	require(method == NONE);
	method = MASTER_FINALIZE;
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskLabel());
	TaskProgression::DisplayLabel(PROGRESSION_MSG_MASTER_FINALIZE);
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterFinalize Begin");
	SetSharedVariablesRW(&oaSharedParameters);
	bOk = MasterFinalize(bProcessOk);
	SetSharedVariablesRO(&oaSharedParameters);
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterFinalize End");
	TaskProgression::EndTask();
	method = NONE;
	tMasterFinalize.Stop();

	// Aggregation des statistiques
	stats_IOReadDuration.AddStats(&GetDriver()->statsIOReadDuration);
	GetDriver()->statsIOReadDuration.Reset();
	stats_IORemoteReadDuration.AddStats(&GetDriver()->statsIORemoteReadDuration);
	GetDriver()->statsIORemoteReadDuration.Reset();

	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Out MasterFinalize", bOk);

	if (bVerbose and bProcessOk and not bOk)
	{
		if (not TaskProgression::IsInterruptionRequested())
			AddError("An error occurred in task finalization");
		else
			AddWarning("Task interrupted by user");
	}
	return bOk;
}

boolean PLParallelTask::CallMasterAggregate()
{
	boolean bOk;
	ALString sTmp;
	int i;

	// Autorisation de lecture pour les output variables
	SetSharedVariablesRO(&oaOutputVariables);
	tMaster.Start();
	require(method == NONE);
	method = MASTER_AGGREGATE;

	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace(sTmp + "In MasterAggregateResults");
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterAggregateResults " +
					   IntToString(GetTaskIndex()) + " Begin");
	bOk = MasterAggregateResults();
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterAggregateResults " +
					   IntToString(GetTaskIndex()) + " End");
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("Out MasterAggregateResults", bOk);
	method = NONE;
	tMaster.Stop();

	// Aggregation des statistiques
	stats_IOReadDuration.AddStats(output_statsIOReadDuration.GetStats());
	stats_ProcessDuration.AddValue(output_dProcessingTime);
	stats_IORemoteReadDuration.AddStats(output_statsIORemoteReadDuration.GetStats());

	// Nettoyage des sharedVariables taskOutput du master
	for (i = 0; i < oaOutputVariables.GetSize(); i++)
	{
		cast(PLSharedVariable*, oaOutputVariables.GetAt(i))->Clean();
	}

	// Fin de l'autorisation de lecture
	SetSharedVariablesNoPermission(&oaOutputVariables);

	if (not bOk)
	{
		if (bVerbose and not TaskProgression::IsInterruptionRequested())
			AddError(sTmp + "An error occurred in agregation of results on subtask " +
				 IntToString(GetTaskIndex()));
	}

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	// CrashTest
	if (GetCrashTestMode() == PLParallelTask::CRASH_MASTER_AGGREGATE)
	{
		cout << "Crash test ON !!" << endl;
		RemoveTempDirContent();
	}
	return bOk;
}

boolean PLParallelTask::CallMasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished, PLSlaveState* slave)
{
	boolean bOk;
	ALString sTmp;

	// Autorisation d'ecriture pour les input variables
	SetSharedVariablesRW(&oaInputVariables);

	// Positionnement du prochain esclave qui va travailler
	nNextWorkingSlaveRank = slave->GetRank();

	// Par defaut, le pourcentage d'avancement est a 0
	dTaskPercent = 0;

	tMaster.Start();
	require(method == NONE);
	method = MASTER_PREPARE_INPUT;
	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
		GetDriver()->GetTracerProtocol()->AddTrace("In MasterPrepareTaskInput");
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterPrepareTaskInput " +
					   IntToString(GetTaskIndex()) + " Begin");
	bOk = MasterPrepareTaskInput(dTaskPercent, bJobIsTerminated);
	assert(not(bJobIsTerminated and bSlaveAtRestWithoutProcessing));
	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog("Task " + GetTaskName() + " .MasterPrepareTaskInput " +
					   IntToString(GetTaskIndex()) + " End");

	if (GetDriver()->GetTracerProtocol()->GetActiveMode())
	{
		if (not bJobIsTerminated)
			GetDriver()->GetTracerProtocol()->AddTrace(
			    sTmp + "Out MasterPrepareTaskInput " + DoubleToString(dTaskPercent), bOk);
		else
			GetDriver()->GetTracerProtocol()->AddTrace(
			    sTmp + "Out MasterPrepareTaskInput " + DoubleToString(dTaskPercent) + " DONE", bOk);
	}
	TouchTmpDir();
	method = NONE;
	tMaster.Stop();
	assert(0 <= dTaskPercent and dTaskPercent <= 1);

	input_nTaskProcessedNumber = nTaskProcessedNumber;

	// Fin d'autorisation d'ecriture
	SetSharedVariablesNoPermission(&oaInputVariables);

	// Arret si erreur
	if (not bOk)
	{
		if (bVerbose and not TaskProgression::IsInterruptionRequested())
			AddError("An error occurred in input preparation");
	}
	return bOk;
}

int PLParallelTask::InternalComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, longint lFileProcessed,
						   longint lFileSize, int nProcessNumber, int nTaskIndex)
{
	int nStepSize;
	int nStepPerSlave;
	int nRandomMin;
	longint lFileSizeRemaining;
	int nComputedBufferSizeMax;
	ALString sTmp;
	boolean bStart;
	boolean bCruise;
	boolean bStop;

	require(nProcessNumber > 0);
	require(nBufferSizeMin > 0);
	require(nBufferSizeMax > 0);
	require(nBufferSizeMin <= nBufferSizeMax);
	require(lFileProcessed < lFileSize);
	require(lFileProcessed >= 0);
	require(lFileSize > 0);

	bStart = false;
	bCruise = false;
	bStop = false;
	nStepSize = -1;

	////////////////////////////////////////////////////
	// Cas du sequentiel
	if (nProcessNumber == 1)
	{
		// On renvoie le max entre la taille du fichier et le buffer max
		if (nBufferSizeMax > lFileSize)
			return InputBufferedFile::FitBufferSize((int)lFileSize);
		else
			return nBufferSizeMax;
	}

	// Il ne faut pas forcement prendre tout le buffer max, par exemple si il est plus
	// grand que le fichier on aura un traitement sequentiel. On considere que chaque esclave doit
	// realiser au moins 2 traitements.
	int nMax = nProcessNumber * 2 * nBufferSizeMax;
	if (nMax <= 0)
		nMax = INT_MAX;
	if (nMax > lFileSize)
	{
		nComputedBufferSizeMax = (int)lFileSize / (2 * nProcessNumber);
	}
	else
		nComputedBufferSizeMax = nBufferSizeMax;

	if (nComputedBufferSizeMax < nBufferSizeMin)
		nComputedBufferSizeMax = nBufferSizeMin;

	//////////////////////////////////////////////////////
	// Cas standard : on evalue dans quel phase on se trouve

	// Debut du traitement : le premier traitement de chaque esclave
	if (nTaskIndex < nProcessNumber)
	{
		bStart = true;
	}

	// Fin de traitement : quand il n'y a du travail que pour la moitie des esclaves
	lFileSizeRemaining = lFileSize - lFileProcessed;
	if (lFileSizeRemaining < (nProcessNumber / 2) * nComputedBufferSizeMax)
	{
		bStop = true;
	}

	// Si on peut etre a la fois en start et en stop, on choisit stop
	if (bStart and bStop)
		bStart = false;

	// Si ni start ni stop, on est en phase nominale
	if (not bStart and not bStop)
		bCruise = true;

	//////////////////////////////////////////////////////
	// Traitement suivant la phase
	if (bStart)
	{
		// On fait une marche d'escalier reguliere entre nBufferSizeMin et nMaxBufferSize
		// Calcul de la hauteur de la marche
		nStepPerSlave = (nComputedBufferSizeMax - nBufferSizeMin) / nProcessNumber;
		nStepSize = (nTaskIndex + 1) * nStepPerSlave;
	}
	if (bStop)
	{
		assert(nStepSize == -1);
		// En fin de course on donne un buffer de plus en plus petit (sans decroitre trop vite)
		nStepSize = (int)lFileSizeRemaining / (nProcessNumber / 2);
	}
	if (bCruise)
	{
		assert(nStepSize == -1);

		// Taille aleatoire entre 0.8 * nBufferSizeMax et nBufferSizeMax
		// (sauf si 0.8*nBufferSizeMax plus petit que nBufferSizeMin)
		nRandomMin = max(nBufferSizeMin, int(0.8 * nComputedBufferSizeMax));
		nStepSize = nRandomMin + RandomInt(nComputedBufferSizeMax - nRandomMin);
	}
	assert(nStepSize != -1);

	// Controle des bornes
	if (nStepSize < nBufferSizeMin)
		nStepSize = nBufferSizeMin;
	if (nStepSize > nComputedBufferSizeMax)
		nStepSize = nComputedBufferSizeMax;

	// Arrondi pour tomber sur une taille de bloc de CharVector
	nStepSize = (int)floor(nStepSize / MemSegmentByteSize) * MemSegmentByteSize;

	// Le buffer doit etre plus grand qu'un bloc et plus petit que 2 Go - 8Mo -1
	nStepSize = InputBufferedFile::FitBufferSize(nStepSize);

	// On ajuste la taille si on depasse la taille du fichier
	if (lFileProcessed + nStepSize > lFileSize)
		nStepSize = (int)(lFileSize - lFileProcessed);

	ensure(nStepSize > 0);
	GetDriver()->GetTracerMPI()->AddTrace(sTmp + "Buffer step size = " + LongintToHumanReadableString(nStepSize));
	return nStepSize;
}

void PLParallelTask::SetSharedVariablesRO(ObjectArray* oaValue)
{
#ifndef NDEBUG
	int i;

	require(oaValue != NULL);

	// Parcours des parametres
	for (i = 0; i < oaValue->GetSize(); i++)
	{
		cast(PLSharedVariable*, oaValue->GetAt(i))->SetReadOnly();
	}
#endif // NDEBUG
}

void PLParallelTask::SetSharedVariablesRW(ObjectArray* oaValue)
{
#ifndef NDEBUG
	int i;

	require(oaValue != NULL);

	// Parcours des parametres
	for (i = 0; i < oaValue->GetSize(); i++)
	{
		cast(PLSharedVariable*, oaValue->GetAt(i))->SetReadWrite();
	}
#endif // NDEBUG
}

void PLParallelTask::SetSharedVariablesNoPermission(ObjectArray* oaValue)
{
#ifndef NDEBUG
	int i;

	require(oaValue != NULL);

	// Parcours des parametres
	for (i = 0; i < oaValue->GetSize(); i++)
	{
		cast(PLSharedVariable*, oaValue->GetAt(i))->SetNoPermission();
	}
#endif // NDEBUG
}

void PLParallelTask::RemoveTempDirContent() const
{
	StringVector svDirectoryNames;
	StringVector svFileNames;
	int i;

	FileService::GetDirectoryContent(FileService::GetApplicationTmpDir(), &svDirectoryNames, &svFileNames);

	// Suppresssion de chaque fichier
	for (i = 0; i < svFileNames.GetSize(); i++)
	{
		FileService::RemoveFile(
		    FileService::BuildFilePathName(FileService::GetApplicationTmpDir(), svFileNames.GetAt(i)));
	}

	// Suppression de chaque repertoire
	for (i = 0; i < svDirectoryNames.GetSize(); i++)
	{
		FileService::RemoveDirectory(
		    FileService::BuildFilePathName(FileService::GetApplicationTmpDir(), svDirectoryNames.GetAt(i)));
	}
}

void PLParallelTask::Exit(int nExitCode)
{
	// Arret propre des esclaves en cas de sortie normale ou si aucune tache n'est en cours et
	// qu'aucun serveur de fichiers n'est lance
	if (nExitCode == EXIT_SUCCESS or
	    (not PLParallelTask::IsRunning() and not PLTaskDriver::IsFileServersLaunched()))
		GetDriver()->StopSlaves();
	else
		GetDriver()->Abort();
}

void PLParallelTask::TouchTmpDir()
{
	if (not tTempTouch.IsStarted() or tTempTouch.GetElapsedTime() > 3600)
	{
		FileService::TouchApplicationTmpDir(86400);
		tTempTouch.Reset();
		tTempTouch.Start();
	}
}

void PLParallelTask::PrintSlavesStates() const
{
	int i;
	for (i = 0; i < oaSlaves.GetSize(); i++)
	{
		cast(PLSlaveState*, oaSlaves.GetAt(i))->Write(cout);
	}
}

PLSlaveState* PLParallelTask::GetReadySlave()
{
	PLSlaveState* readySlave;
	ALString sHost;
	Object* oaSlavesOnHost;
	if (IsParallel())
	{
		POSITION position = odHosts.GetStartPosition();
		while (position != NULL)
		{
			odHosts.GetNextAssoc(position, sHost, oaSlavesOnHost);
			readySlave = GetReadySlaveOnHost(cast(ObjectArray*, oaSlavesOnHost));
			if (readySlave != NULL)
				return readySlave;
		}
		return NULL;
	}
	else
	{
		assert(IsParallelSimulated());
		readySlave = cast(PLSlaveState*, oaSlaves.GetAt(nCurrentSlavePosition));
		assert(readySlave->IsReady());

		// Position suivante pour le prochain esclave
		nCurrentSlavePosition++;
		if (nCurrentSlavePosition == oaSlaves.GetSize())
			nCurrentSlavePosition = 0;

		// Si il est en sommeil on renvoie le prochain...
		if (readySlave->GetAtRest())
		{
			//... si il y en a un
			if (GetRestingSlaveNumber() == oaSlaves.GetSize())
				return NULL;
			else
				return GetReadySlave();
		}
		return readySlave;
	}
}

PLSlaveState* PLParallelTask::GetReadySlaveOnHost(ObjectArray* oaSlavesInHost)
{
	PLSlaveState* readySlave;
	int i;

	// On cherche en priorite les esclaves qui sont deja initialises
	for (i = 0; i < oaSlavesInHost->GetSize(); i++)
	{
		assert(oaSlavesInHost->GetAt(i) != NULL);
		readySlave = cast(PLSlaveState*, oaSlavesInHost->GetAt(i));
		if (readySlave->IsReady() and not readySlave->GetAtRest())
			return readySlave;
	}

	// Seconde boucle pour rendre un esclave dans l'etat VOID
	for (i = 0; i < oaSlavesInHost->GetSize(); i++)
	{
		assert(oaSlavesInHost->GetAt(i) != NULL);
		readySlave = cast(PLSlaveState*, oaSlavesInHost->GetAt(i));
		if (readySlave->IsVoid() and not readySlave->GetAtRest())
			return readySlave;
	}
	return NULL;
}

void PLParallelTask::SerializeSharedVariables(PLSerializer* serializer, ObjectArray* oaVariables, boolean bClean) const
{
	Object* oElement;
	PLSharedVariable* sharedVariable;

	require(oaVariables != NULL);
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	// Parcours des variables partagees resultats pour serialisation
	for (int i = 0; i < oaVariables->GetSize(); i++)
	{
		oElement = oaVariables->GetAt(i);
		sharedVariable = cast(PLSharedVariable*, oElement);
		sharedVariable->Serialize(serializer);
		if (bClean)
			sharedVariable->Clean();
	}
}

void PLParallelTask::DeserializeSharedVariables(PLSerializer* serializer, ObjectArray* oaVariables) const
{
	int i;
	Object* o;

	require(oaVariables != NULL);
	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	for (i = 0; i < oaVariables->GetSize(); i++)
	{
		// Deserialisation de la variable
		o = oaVariables->GetAt(i);
		assert(o != NULL);
		cast(PLSharedVariable*, o)->Deserialize(serializer);
	}
}

boolean PLParallelTask::InitializeParametersFromSharedVariables()
{
	boolean bOk;

	bOk = true;

	// Mise a jour du nombre de messages max
	Global::SetMaxErrorFlowNumber(shared_nMaxErrorFlowNumber);

	// Mise a jour des UserTmpDir et ApplicationName et creation du repertoire temporaire
	if (FileService::GetUserTmpDir() != shared_sUserTmpDir.GetValue())
		FileService::SetUserTmpDir(shared_sUserTmpDir.GetValue());
	if (FileService::GetApplicationName() != shared_sApplicationName.GetValue())
		FileService::SetApplicationName(shared_sApplicationName.GetValue());

	bOk = FileService::CreateApplicationTmpDir();
	require(FileService::CheckApplicationTmpDir());

	// Mise a jour de la duree de vie du repertoire temporaire
	TouchTmpDir();

	// Mise a jour du nom de l'application
	SetTaskUserLabel(shared_sTaskUserLabel.GetValue());

	return bOk;
}

boolean PLParallelTask::IsAllSlavesEnding() const
{
	int i;

	for (i = 0; i < oaSlaves.GetSize(); i++)
	{
		if (not cast(PLSlaveState*, oaSlaves.GetAt(i))->IsEnding())
		{
			return false;
		}
	}
	return true;
}

void PLParallelTask::AddLocalGenericError(int nGravity, const ALString& sLabel, longint lLineNumber)
{
	Error error;
	PLErrorWithIndex* errorWithIndex;
	ALString sLabelWithIndex;
	ALString sTmp;

	assert(method == PLParallelTask::SLAVE_PROCESS);
	assert(lLineNumber > 0);
	bAddLocalGenericErrorCall = true;

	if (IsParallel())
	{
		// Construction d'un objet erreur
		error.Initialize(nGravity, GetClassLabel(), GetObjectLabel(), sLabel);

		// Construction de l'erreur avec index
		errorWithIndex = new PLErrorWithIndex;
		errorWithIndex->SetIndex(lLineNumber);
		errorWithIndex->SetError(error.Clone());

		// Ajout a la liste des messages a afficher
		oaUserMessages->Add(errorWithIndex);
	}
	else
	{

		// Ajout de la decoration
		sLabelWithIndex =
		    sTmp + "Record " + LongintToReadableString(lGlobalLineNumber + lLineNumber) + " : " + sLabel;

		// Construction et affichage de l'erreur
		error.Initialize(nGravity, GetClassLabel(), GetObjectLabel(), sLabelWithIndex);
		Global::AddErrorObject(&error);
	}
}

int PLParallelTask::GetTaskIndex() const
{
	if (method == MASTER_AGGREGATE)
		return nSlaveTaskIndex;
	else
		return nTaskProcessedNumber;
}

void PLParallelTask::TestComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, longint lFileSize,
						int nProcessNumber)
{
	int i;
	longint lFileProcessed;
	int nBufferSize;

	require(nBufferSizeMin > 0);
	require(nBufferSizeMax > nBufferSizeMin);
	require(lFileSize > 0);
	require(nProcessNumber > 0);

	cout << "nBufferSizeMin " << LongintToHumanReadableString(nBufferSizeMin) << "\t"
	     << "nBufferSizeMax " << LongintToHumanReadableString(nBufferSizeMax) << "\t"
	     << "lFileSize " << LongintToHumanReadableString(lFileSize) << endl
	     << endl
	     << "TaskId"
	     << "\t"
	     << "buffer_size" << endl;
	lFileProcessed = 0;
	i = 0;
	while (lFileProcessed < lFileSize)
	{
		nBufferSize = PLParallelTask::InternalComputeStairBufferSize(
		    nBufferSizeMin, nBufferSizeMax, lFileProcessed, lFileSize, nProcessNumber, i);
		cout << i << "\t" << nBufferSize << endl;
		lFileProcessed += nBufferSize;
		i++;
	}
	cout << endl;
}
