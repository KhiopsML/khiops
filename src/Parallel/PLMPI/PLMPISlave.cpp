// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPISlave.h"

boolean PLMPISlave::bPendingFatalError = false;
boolean PLMPISlave::bIsInstanciated = false;
IntVector* PLMPISlave::ivGravityReached = NULL;
DisplayErrorFunction PLMPISlave::currentDisplayErrorFunction = NULL;

PLMPISlave::PLMPISlave(PLParallelTask* t)
{
	int color;
	ALString sNewFileName;
	PLShared_TaskResourceGrant shared_rg;
	PLMPIMsgContext context;
	PLSerializer serializer;
	boolean bIsGUI;

	bBoostedMode = false;
	bIsWorking = false;
	color = MPI_UNDEFINED;
	task = t;
	assert(task->oaUserMessages == NULL);
	task->oaUserMessages = new ObjectArray;
	ivGravityReached = new IntVector;
	ivGravityReached->SetSize(3);

	// Reception du nombre d'esclaves
	MPI_Bcast(&task->nWorkingProcessNumber, 1, MPI_INT, 0, *PLMPITaskDriver::GetTaskComm());

	// Reception des parametres...
	context.Bcast(*PLMPITaskDriver::GetTaskComm());
	serializer.OpenForRead(&context); // BCast parameters

	// ... du fichier de logs,
	PLParallelTask::sParallelLogFileName = serializer.GetString();

	// ... du tracer de la performance,
	GetTask()->sPerformanceTaskName = serializer.GetString();

	// ... et des resources allouees
	RMParallelResourceDriver::grantedResources = new RMTaskResourceGrant;
	shared_rg.DeserializeObject(&serializer, RMParallelResourceDriver::grantedResources);

	// ... et de la presence d'une IHM
	bIsGUI = serializer.GetBoolean();

	// .. et de la liste des serveurs de fichier
	PLShared_ObjectDictionary shared_odFileServer(new PLShared_IntObject);
	shared_odFileServer.DeserializeObject(&serializer,
					      &cast(PLMPITaskDriver*, PLParallelTask::GetDriver())->odFileServers);
	serializer.Close();

	// Quand on utilise une IHM, les esclaves peuvent recevoir une notification d'interruption.
	// L'instance de TaskProgression chez les esclaves est textual, donc par defaut non-interruptible
	TaskProgression::SetInterruptible(bIsGUI);

	POSITION position = cast(PLMPITaskDriver*, PLParallelTask::GetDriver())->odFileServers.GetStartPosition();
	ALString sKey;
	Object* oElement;
	while (position != NULL)
	{
		cast(PLMPITaskDriver*, PLParallelTask::GetDriver())
		    ->odFileServers.GetNextAssoc(position, sKey, oElement);
	}

	// Activation des traces ... ou non
	if (not GetTask()->GetParallelLogFileName().IsEmpty())
	{
		GetTracerPerformance()->SetActiveMode(true);
		GetTracerPerformance()->SetTimeDecorationMode(true);
		GetTracerPerformance()->SetSynchronousMode(false);

		sNewFileName =
		    FileService::BuildFileName(FileService::GetFilePrefix(GetTask()->GetParallelLogFileName()) + "_" +
						   GetTask()->sPerformanceTaskName + "_" + IntToString(GetProcessId()),
					       FileService::GetFileSuffix(GetTask()->GetParallelLogFileName()));

		GetTracerPerformance()->SetFileName(FileService::BuildFilePathName(
		    FileService::GetPathName(GetTask()->GetParallelLogFileName()), sNewFileName));
	}

	// Ajout de parametres separes par des ","
	if (GetTracerPerformance()->GetActiveMode())
		GetTracerPerformance()->AddTrace("<< Job start [" + GetTask()->sPerformanceTaskName + "," +
						 GetLocalHostName() + "," + IntToString(task->nWorkingProcessNumber) +
						 "]");

	// Mise en place du gestionnaire de progression (qui egalement met a jour le nombre d'erreur max)
	progressionManager = new PLMPISlaveProgressionManager;
	progressionManager->SetMaxErrorReached(ivGravityReached);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::SetManager(progressionManager);
	TaskProgression::Start();

	// Positionnement du gestionnaire de messages
	currentDisplayErrorFunction = Error::GetDisplayErrorFunction();
	Error::SetDisplayErrorFunction(DisplayError);

	// Les variables partagees sont en lecture seule
	task->SetSharedVariablesRO(&task->oaSharedParameters);
}

PLMPISlave::~PLMPISlave()
{
	int i;

	if (*PLMPITaskDriver::GetTaskComm() == MPI_COMM_NULL)
		return;

	// Nettoyage de toutes les variables partagees
	for (i = 0; i < task->oaInputVariables.GetSize(); i++)
	{
		cast(PLSharedVariable*, task->oaInputVariables.GetAt(i))->Clean();
	}

	for (i = 0; i < task->oaOutputVariables.GetSize(); i++)
	{
		cast(PLSharedVariable*, task->oaOutputVariables.GetAt(i))->Clean();
	}

	for (i = 0; i < task->oaSharedParameters.GetSize(); i++)
	{
		cast(PLSharedVariable*, task->oaSharedParameters.GetAt(i))->Clean();
	}

	cast(PLMPITaskDriver*, PLParallelTask::GetDriver())->odFileServers.DeleteAll();

	TaskProgression::Stop();
	delete task->oaUserMessages;
	task->oaUserMessages = NULL;
	delete progressionManager;
	delete ivGravityReached;

	Error::SetDisplayErrorFunction(currentDisplayErrorFunction);
	currentDisplayErrorFunction = NULL;
}

void PLMPISlave::NotifyDone(boolean bOk)
{
	PLMPIMsgContext context;
	PLSerializer serializer;

	context.Send(MPI_COMM_WORLD, 0, SLAVE_DONE);
	serializer.OpenForWrite(&context);
	serializer.PutBoolean(bOk);
	serializer.Close();
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(0, SLAVE_DONE);
}

void PLMPISlave::SendResults()
{
	PLMPIMsgContext context;
	PLSerializer serializer;

	// Envoi du resultat de la serialisation
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(0, SLAVE_END_PROCESSING);
	context.Send(MPI_COMM_WORLD, 0, SLAVE_END_PROCESSING);
	serializer.OpenForWrite(&context);
	GetTask()->SerializeSharedVariables(&serializer, &GetTask()->oaOutputVariables, true);
	serializer.Close();
}

boolean PLMPISlave::Process()
{
	boolean bStopOrder;
	boolean bOk;
	int nbTasks; // Nombre de tache allouees aux esclaves (pour trace)
	ALString sTmp;
	int i;
	MPI_Status status;
	PLShared_HostResource shared_resource;
	boolean bNewMessage;
	PLMPIMsgContext context;
	int nMessageTag;
	PLSerializer serializer;

	bStopOrder = false; // Condition d'arret : a-t-on recu une notification d'arret
	bOk = true;

	SetRandomSeed(GetProcessId());
	GetTask()->SetVerbose(GetTask()->input_bVerbose);
	nbTasks = 0;
	tTimeBeforeSleep.Start();
	while (not bStopOrder and not TaskProgression::IsInterruptionRequested())
	{
		if (bBoostedMode)
		{
			// Initialise la reception du message (bloc par bloc)
			// la reception est complete seulement apres deserialisation
			context.Recv(MPI_COMM_WORLD, 0, MPI_ANY_TAG);
			serializer.OpenForRead(&context);
			nMessageTag = context.GetTag();
			bNewMessage = true;
		}
		else
		{
			// On test si il y a un nouveau message
			bNewMessage = CheckNewMessage(0, MPI_COMM_WORLD, status);
			nMessageTag = status.MPI_TAG;
		}

		if (bNewMessage)
		{
			switch (nMessageTag)
			{
			case MASTER_TASK_INPUT:

				// On recoit les inputs de la tache
				if (not bBoostedMode)
				{
					context.Recv(MPI_COMM_WORLD, 0, MASTER_TASK_INPUT);
					serializer.OpenForRead(&context);
				}
				task->SetSharedVariablesRW(&task->oaInputVariables);
				task->DeserializeSharedVariables(&serializer, &GetTask()->oaInputVariables);
				serializer.Close();

				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, MASTER_TASK_INPUT);

				if (not task->bIsSlaveInitialized)
				{
					progressionManager->SetSlaveState(State::INIT);

					// Initialisation
					bOk = task->CallSlaveInitialize();

					// Envoi des messages utilisateurs
					SendUserMessages(0);

					// Envoi de la fin d'initialisation
					// Avec l'index de la tache courante : pour que le maitre puisse ordonner les
					// messages utilisateur
					if (GetTracerMPI()->GetActiveMode())
						GetTracerMPI()->AddSend(0, SLAVE_INITIALIZE_DONE);
					context.Send(MPI_COMM_WORLD, 0, SLAVE_INITIALIZE_DONE);
					serializer.OpenForWrite(&context);
					serializer.PutInt(task->input_nTaskProcessedNumber);
					serializer.Close();

					// En cas d'erreur, envoi d'une demande d'arret
					if (not bOk)
						NotifyAbnormalExit();
				}

				// On lance le SlaveProcess seulement si le SlaveInitialize s'est bien passe
				if (bOk)
				{
					bIsWorking = true;
					nbTasks++;

					// Mise a jour des parametres de la classe Global
					Global::SetSilentMode(task->input_bSilentMode);

					// Positionnement de l'etat en cours pour que le gestionnaire de progression
					// l'envoie avec les messages de progression
					progressionManager->SetSlaveState(State::PROCESSING);

					// On force la verification de l'interruption : on l'aura immediatement dans le
					// SlaveProcess
					progressionManager->IsInterruptionRequested();

					// Traitement : appel de SlaveProcess
					bOk = task->CallSlaveProcess();

					// Ajout de variables a envoyer
					task->SetSharedVariablesRW(&task->oaOutputVariables);
					GetTask()->output_dProcessingTime = tProcessing.GetElapsedTime();
					GetTask()->output_statsIOReadDuration.GetStats()->AddStats(
					    &PLMPITaskDriver::GetDriver()->statsIOReadDuration);
					PLMPITaskDriver::GetDriver()->statsIOReadDuration.Reset();
					GetTask()->output_statsIORemoteReadDuration.GetStats()->AddStats(
					    &PLMPITaskDriver::GetDriver()->statsIORemoteReadDuration);
					PLMPITaskDriver::GetDriver()->statsIORemoteReadDuration.Reset();
					task->output_bSlaveProcessOk = bOk;
					task->SetSharedVariablesNoPermission(&task->oaOutputVariables);

					// Envoi des messages utilisateurs
					SendUserMessages(1);

					// Nettoyage des variables partagees d'entree
					for (i = 0; i < task->oaInputVariables.GetSize(); i++)
					{
						cast(PLSharedVariable*, task->oaInputVariables.GetAt(i))->Clean();
					}

					// Envoi les resultats
					// (meme en cas d'erreur : sinon le maitre ne saura pas que cet esclave a fini
					// de travailler)
					SendResults();
				}
				break;

			case MASTER_STOP_ORDER:
				if (not bIsWorking and GetTracerProtocol()->GetActiveMode())
					GetTracerProtocol()->AddTrace("have done nothing ...");

				// On recoit le message
				if (not bBoostedMode)
				{
					context.Recv(MPI_COMM_WORLD, 0, MASTER_STOP_ORDER);
					serializer.OpenForRead(&context);
				}
				bOk = serializer.GetBoolean();
				serializer.Close();
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, MASTER_STOP_ORDER);
				if (GetTracerProtocol()->GetActiveMode())
					GetTracerProtocol()->AddTrace(sTmp + "have processed " + IntToString(nbTasks) +
								      " sub Tasks");

				bStopOrder = true;
				break;
			case INTERRUPTION_REQUESTED:

				if (not bBoostedMode)
				{
					context.Recv(MPI_COMM_WORLD, 0, INTERRUPTION_REQUESTED);
					serializer.OpenForRead(&context);
				}

				serializer.Close();
				progressionManager->SetInterruptionRequested(true);
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, INTERRUPTION_REQUESTED);
				break;

			case MAX_ERROR_FLOW:

				if (not bBoostedMode)
				{
					context.Recv(MPI_COMM_WORLD, 0, MAX_ERROR_FLOW);
					serializer.OpenForRead(&context);
				}
				serializer.GetIntVector(ivGravityReached);
				serializer.Close();
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, MAX_ERROR_FLOW);
				break;

			default:
				assert(false);
				serializer.Close();
				if (GetTracerProtocol()->GetActiveMode())
					GetTracerProtocol()->AddTrace(sTmp + "Unexpected message received with tag " +
								      GetTagAsString(nMessageTag));
				if (GetTask()->GetVerbose())
					AddError(sTmp + "unexpected message with status " +
						 GetTagAsString(nMessageTag));
			} // switch

			// Remise a zero du timer de mise en sommeil
			tTimeBeforeSleep.Reset();
			tTimeBeforeSleep.Start();
		} // if (messageTag!=0)

		if (not bBoostedMode and tTimeBeforeSleep.GetElapsedTime() > PLMPITaskDriver::TIME_BEFORE_SLEEP)
		{
			SystemSleep(0.001);
		}
	}

	if (GetTracerPerformance()->GetActiveMode())
		GetTracerPerformance()->AddTrace("<< Job stop [" + GetTask()->sPerformanceTaskName + "]");

	return bOk;
}

void PLMPISlave::NotifyAbnormalExit()
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(0, SLAVE_NEED_STOP);
	context.Send(MPI_COMM_WORLD, 0, SLAVE_NEED_STOP);
	serializer.OpenForWrite(&context);
	serializer.Close();
}

void PLMPISlave::NotifyFatalError(Error* error)
{
	PLSharedErrorWithIndex shared_error;
	PLErrorWithIndex* errorIndex;
	PLMPIMsgContext context;

	require(error != NULL);
	require(error->GetGravity() == Error::GravityFatalError);
	context.Send(MPI_COMM_WORLD, 0, SLAVE_FATAL_ERROR);

	if (not bPendingFatalError)
	{
		bPendingFatalError = true;
		errorIndex = new PLErrorWithIndex;
		errorIndex->SetError(error);
		errorIndex->SetIndex(-1);

		// Serialisation et envoi de l'erreur
		shared_error.SetErrorIndex(errorIndex);
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddSend(0, SLAVE_FATAL_ERROR);
		PLMPITaskDriver::SendSharedObject(&shared_error, &context);
	}
}

void PLMPISlave::SlaveAbort(int nExitCode)
{
	if (not bPendingFatalError)
		PLParallelTask::GetDriver()->Abort();
}

int PLMPISlave::WaitForMessage(int nTag, boolean& bInterruptionRequested) const
{
	MPI_Status status;
	int nThereIsAmsg;
	int nSize;

	nThereIsAmsg = 0;
	while (not nThereIsAmsg)
	{
		MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &nThereIsAmsg, &status);
		if (TaskProgression::IsInterruptionRequested())
		{
			bInterruptionRequested = true;
			return 0;
		}
	}
	bInterruptionRequested = false;
	MPI_Get_count(&status, MPI_CHAR, &nSize);
	return nSize;
}

void PLMPISlave::DischargePendigCommunication()
{
	MPI_Status status;
	int nThereArePendingMessages;
	PLMPIMsgContext mpiContext;
	ALString sTmp;

	nThereArePendingMessages = 1;
	while (nThereArePendingMessages != 0)
	{
		MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &nThereArePendingMessages, &status);
		if (nThereArePendingMessages)
		{
			// TODO est-ce qu'on peut utiliser NULL comme buffer ?
			MPI_Recv(sBufferDischarge, MemSegmentByteSize, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG,
				 MPI_COMM_WORLD, &status);
			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddRecv(0, status.MPI_TAG);
			if (PLParallelTask::GetVerbose())
				TraceWithRank(sTmp + "discharge pending comm from " + IntToString(status.MPI_SOURCE) +
					      " with tag " + GetTagAsString(status.MPI_TAG));
		}
	}
}

void PLMPISlave::DisplayError(const Error* e)
{
	PLErrorWithIndex* errorIndex;

	// Cas des erreurs fatales
	if (e->GetGravity() == Error::GravityFatalError)
	{
		// Envoi au master
		NotifyFatalError(e->Clone());
	}
	// Cas d'une erreur avant depassement de limite du controle de flow, ou d'une erreur echapant au controle de
	// flow des erreurs
	else if (not IsMaxErrorFlowReachedPerGravity(e->GetGravity()) or Global::IgnoreErrorFlow(e))
	{
		// Construction d'une erreur indexee par le numero de ligne
		// Pour etre coherent avec les methodes PLParallelTask::AddLocalError
		// Ici c'est un erreur non indexee donc le numero de ligne est -1
		errorIndex = new PLErrorWithIndex;
		errorIndex->SetError(e->Clone());
		errorIndex->SetIndex(-1);

		// Ajout aux messages a envoyer
		PLParallelTask::oaUserMessages->Add(errorIndex);
	}
	// Cas des erreurs ignorees
	else
	{
		// Suppression des messages stockes de ce type
		DeleteMessagesWithGravity(e->GetGravity());
	}
}

void PLMPISlave::DeleteMessagesWithGravity(int nGravity)
{
	PLErrorWithIndex* error;
	int nIndex;
	int i;

	require(nGravity == Error::GravityMessage or nGravity == Error::GravityWarning or
		nGravity == Error::GravityError);

	// Supression  des erreurs de la bonne gravite, en deplacant les erreurs restantes a leur nouvel index
	nIndex = 0;
	for (i = 0; i < PLParallelTask::oaUserMessages->GetSize(); i++)
	{
		error = cast(PLErrorWithIndex*, PLParallelTask::oaUserMessages->GetAt(i));

		// Supression, sauf si l'errreur echape au controle de flow des erreurs
		if (error->GetError()->GetGravity() == nGravity and not Global::IgnoreErrorFlow(error->GetError()))
			delete error;
		else
		{
			PLParallelTask::oaUserMessages->SetAt(nIndex, error);
			nIndex++;
		}
	}

	// On retaille le tableau avec le nouveu nombre d'erreurs
	PLParallelTask::oaUserMessages->SetSize(nIndex);
}

void PLMPISlave::SendUserMessages(int nMethod)
{
	PLShared_ObjectArray shared_oa(new PLSharedErrorWithIndex);
	PLMPIMsgContext context;
	boolean bIsDeclared;
	PLSerializer serializer;

	assert(nMethod == 0 or nMethod == 1 or nMethod == 2);
	if (PLParallelTask::oaUserMessages->GetSize() == 0)
		return;

	// Suppression des messages qui on deja atteind le max : ce n'est pas la peine de les envoyer
	if (IsMaxErrorFlowReachedPerGravity(Error::GravityError))
		DeleteMessagesWithGravity(Error::GravityError);
	if (IsMaxErrorFlowReachedPerGravity(Error::GravityMessage))
		DeleteMessagesWithGravity(Error::GravityMessage);
	if (IsMaxErrorFlowReachedPerGravity(Error::GravityWarning))
		DeleteMessagesWithGravity(Error::GravityWarning);

	if (PLParallelTask::oaUserMessages->GetSize() == 0)
		return;

	// Envoi de toutes les erreurs
	shared_oa.SetObjectArray(PLParallelTask::oaUserMessages);
	context.Send(MPI_COMM_WORLD, 0, SLAVE_USER_MESSAGE);
	bIsDeclared = shared_oa.bIsDeclared;
	shared_oa.bIsDeclared = true;
	serializer.OpenForWrite(&context);
	serializer.PutInt(nMethod);
	shared_oa.Serialize(&serializer);
	serializer.Close();
	shared_oa.bIsDeclared = bIsDeclared;

	// Nettoyage
	PLParallelTask::oaUserMessages->DeleteAll();
	shared_oa.SetObjectArray(NULL);

	// On doit recreer le tableau des messages, car celui en cours est mis dans une variable partagee, et sera
	// detruit suite a l'envoi
	PLParallelTask::oaUserMessages = new ObjectArray;
	ensure(PLParallelTask::oaUserMessages->GetSize() == 0);
}

boolean PLMPISlave::IsMaxErrorFlowReachedPerGravity(int nErrorGravity)
{
	// Cas de l'erreur fatale
	if (nErrorGravity > 3)
		return false;
	return ivGravityReached->GetAt(nErrorGravity);
}

void PLMPISlave::Run()
{
	ALString sNewFileName;
	boolean bOk;
	boolean bFinalizeOk;
	ALString sTmp;
	PLMPIMsgContext context;
	PLSerializer serializer;

	bOk = true;
	bFinalizeOk = true;

	if (*PLMPITaskDriver::GetTaskComm() == MPI_COMM_NULL)
	{
		AddFatalError("Null MPI communicator");
	}

	// Nettoyage des traces
	PLParallelTask::GetDriver()->GetIOReadingStats()->Reset();
	PLParallelTask::GetDriver()->GetIORemoteReadingStats()->Reset();

	// Affectation des droits des shared variables
	task->SetSharedVariablesRO(&task->oaInputVariables);
	task->SetSharedVariablesRW(&task->oaSharedParameters);
	task->SetSharedVariablesRO(&task->oaOutputVariables);

	// Positionnement de l'etat en cours pour que le gestionnaire de progression
	// l'envoie avec les messages de progression
	progressionManager->SetSlaveState(State::VOID);

	// Reception des parametres
	context.Bcast(*PLMPITaskDriver::GetTaskComm());
	serializer.OpenForRead(&context); // BCast task parameters
	task->DeserializeSharedVariables(&serializer, &GetTask()->oaSharedParameters);
	serializer.Close();
	task->SetSharedVariablesRO(&task->oaSharedParameters);
	GetTask()->InitializeParametersFromSharedVariables();

	// Activation du mode boost
	bBoostedMode = GetTask()->shared_bBoostedMode;

	// Mise a jour du nom de l'application
	task->SetTaskUserLabel(task->shared_sTaskUserLabel.GetValue());

	// Boucle de traitement dirigee par le maitre
	// En cas d'erreur, le message qui contient les resultats et qui est envoye au maitre
	// contient un boolean a False. Lors de sa reception le maitre ordonne a tous les esclaves de s'arreter,
	// c'est le seul moyen de sortir de la boucle
	bOk = Process();

	if (task->bIsSlaveInitialized)
	{
		// Positionnement de l'etat en cours pour que le gestionnaire de progression
		// l'envoie avec les messages de progression
		progressionManager->SetSlaveState(State::FINALIZE);

		// On force la verification de l'interruption : on l'aura immediatement dans le SlaveFinalize
		progressionManager->IsInterruptionRequested();

		// Finalisation
		bFinalizeOk = task->CallSlaveFinalize(bOk);

		// On force la verification de l'interruption : au cas ou elle a ete envoye pendant le SlaveFinalize
		// sans avoir ete recue
		progressionManager->IsInterruptionRequested();

		// Envoi des messages utilisateurs
		SendUserMessages(2);

		// Notification de fin de traitement
		NotifyDone(bFinalizeOk);
	}

	// Nettoyage du HugeBuffer pour eviter qu'il soit alloue tout le reste du programe
	DeleteHugeBuffer();

	// A partir d'ici on est sur que le maitre n'enverra plus de messages
	MPI_Barrier(*PLMPITaskDriver::GetTaskComm()); // BARRIER MSG

	// Reception de tous les message du maitre qui sont encore dans les tuyaux
	DischargePendigCommunication();

	// Barriere indispensable pour s'assurer qu'on ne decharge pas les messages de la prochaine tache
	// ou des ordres aux serveurs de fichiers
	MPI_Barrier(*PLMPITaskDriver::GetTaskComm()); // BARRIER MSG 2
}

const ALString PLMPISlave::GetClassLabel() const
{
	return "PLMPISlave";
}
