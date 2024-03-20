// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPIMaster.h"

PLMPIMaster::PLMPIMaster(PLParallelTask* t)
{
	ALString sTmp;
	nWorkingSlaves = 0;
	task = t;
	bNewMessage = false;
	bSpawnedDone = false;
	bStopOrderDone = false;
	bInterruptionRequested = false;
	bSlaveError = false;
	bMasterError = false;
	bIsMaxErrorReached = false;
	bIsMaxWarningReached = false;
	bIsMaxMessageReached = false;
	dGlobalProgression = 0;
	nOldProgression = 0;
	bIsProcessing = false;
	nInitialisationCount = 0;
	nFinalisationCount = 0;
	nFirstSlaveInitializeMessageRank = -1;
	nFirstSlaveFinalizeMessageRank = -1;
	sBufferDischarge[0] = '\0';
}

PLMPIMaster::~PLMPIMaster()
{
	int i;

	if (bSpawnedDone)
	{
		// Nettoyage des variables partagees
		for (i = 0; i < task->oaInputVariables.GetSize(); i++)
		{
			cast(PLSharedVariable*, task->oaInputVariables.GetAt(i))->Clean();
		}

		for (i = 0; i < task->oaOutputVariables.GetSize(); i++)
		{
			cast(PLSharedVariable*, task->oaOutputVariables.GetAt(i))->Clean();
		}
	}
}

void PLMPIMaster::Disconnect()
{
	// On traite tous les messages qui sont dans les tuyaux..
	DischargePendingCommunication(MPI_ANY_SOURCE, MPI_ANY_TAG);

	// Deconnexion
	MPI_Comm_disconnect(PLMPITaskDriver::GetTaskComm());
}

void PLMPIMaster::UpdateMaxErrorFlow()
{
	boolean bNeedToUpdate;
	IntVector ivGravityReached;
	PLSerializer serializer;
	PLMPIMsgContext context;
	int i;

	bNeedToUpdate = false;
	ivGravityReached.SetSize(3);

	// On teste si on doit mettre a jour (on ne met a jour qu'une seule fois pour chaque type d'erreur)
	if (not bIsMaxErrorReached and Global::IsMaxErrorFlowReachedPerGravity(Error::GravityError))
	{
		bNeedToUpdate = true;
		bIsMaxErrorReached = true;
	}

	if (not bIsMaxWarningReached and Global::IsMaxErrorFlowReachedPerGravity(Error::GravityWarning))
	{
		bNeedToUpdate = true;
		bIsMaxWarningReached = true;
	}

	if (not bIsMaxMessageReached and Global::IsMaxErrorFlowReachedPerGravity(Error::GravityMessage))
	{
		bNeedToUpdate = true;
		bIsMaxMessageReached = true;
	}

	if (bNeedToUpdate)
	{
		// Initialisation du tableau
		ivGravityReached.SetAt(Error::GravityError,
				       Global::IsMaxErrorFlowReachedPerGravity(Error::GravityError));
		ivGravityReached.SetAt(Error::GravityWarning,
				       Global::IsMaxErrorFlowReachedPerGravity(Error::GravityWarning));
		ivGravityReached.SetAt(Error::GravityMessage,
				       Global::IsMaxErrorFlowReachedPerGravity(Error::GravityMessage));

		// Envoi du tableau a tous les esclaves
		for (i = 0; i < GetTask()->ivGrantedSlaveIds.GetSize(); i++)
		{
			context.Send(MPI_COMM_WORLD, GetTask()->ivGrantedSlaveIds.GetAt(i), MAX_ERROR_FLOW);
			serializer.OpenForWrite(&context);
			serializer.PutIntVector(&ivGravityReached);
			serializer.Close();
		}
	}
}

boolean PLMPIMaster::Run()
{
	boolean bOk;
	ALString sTmp;
	PLSlaveState* slave;
	int i;
	IntVector ivExcludeSlaves;
	PLSerializer serializer;
	PLShared_TaskResourceGrant shared_rg;
	PLMPIMsgContext context;
	ALString sHostName;

	// Nettoyage des traces
	PLParallelTask::GetDriver()->GetIOReadingStats()->Reset();
	PLParallelTask::GetDriver()->GetIORemoteReadingStats()->Reset();

	nWorkingSlaves = 0;
	assert(messageManager.IsEmpty());

	// [Envoi de message vers la classe PLMPISlaveLauncher]

	// Si l'esclave de rang i est exclu ivExcludeSlaves[i]==1
	// on rempli le premier indice avec une valeur par defaut
	ivExcludeSlaves.Add(0);

	// Envoi du nom de la tache a tous les esclaves (autres que serveurs de fichiers)
	for (i = 1; i < RMResourceManager::GetLogicalProcessNumber(); i++)
	{
		if (not PLMPITaskDriver::GetDriver()->IsFileServer(i))
		{
			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddSend(i, MASTER_LAUNCH_WORKERS);

			context.Send(MPI_COMM_WORLD, i, MASTER_LAUNCH_WORKERS);
			serializer.OpenForWrite(&context);
			serializer.PutString(task->GetTaskSignature());
			serializer.Close();

			// Est-ce que l'esclave travaille
			if (RMParallelResourceDriver::grantedResources->IsResourcesForProcess(i))
				ivExcludeSlaves.Add(0);
			else
				ivExcludeSlaves.Add(1);
		}
		else
		{
			// On ajoute une valeur pour les serveurs de fichier pour que le tableau ait la bonne taille (on
			// y a accede par le rank de l'esclave)
			ivExcludeSlaves.Add(1);
		}
	}

	////////////////////////////////////////
	// Creation d'un intracommunicateur pour les esclaves qui doivent travailler
	// Les autres auront un intracom null

	// Bcast des exclus
	context.Bcast(*PLMPITaskDriver::GetProcessComm());
	serializer.OpenForWrite(&context);
	serializer.PutIntVector(&ivExcludeSlaves);
	serializer.Close();

	// Creation d'un nouveau comminicateur qui contient le maitre et les esclaves qui travaillent
	MPI_Comm_split(*PLMPITaskDriver::GetProcessComm(), 1, GetProcessId(), PLMPITaskDriver::GetTaskComm());
	ensure(*PLMPITaskDriver::GetTaskComm() != MPI_COMM_NULL);

	// Verification de la taille du communicateur
	debug(; int nSize; MPI_Comm_size(*PLMPITaskDriver::GetTaskComm(), &nSize);
	      assert(nSize == task->nWorkingProcessNumber + 1););

	// Envoi du nom du log du tracer de performance aux serveurs de fichiers
	if (PLParallelTask::sParallelLogFileName.GetLength() > 0)
	{
		for (i = 1; i < RMResourceManager::GetLogicalProcessNumber(); i++)
		{
			if (PLMPITaskDriver::GetDriver()->IsFileServer(i))
			{
				context.Send(MPI_COMM_WORLD, i, MASTER_LOG_FILE);
				serializer.OpenForWrite(&context);
				serializer.PutString(PLParallelTask::sParallelLogFileName);
				serializer.PutString(GetTask()->sPerformanceTaskName);
				serializer.Close();
			}
		}
	}

	// Envoi de l'etat du tracer MPI aux serveurs de fichiers
	if (PLParallelTask::GetDriver()->GetTracerMPI()->GetActiveMode())
	{
		for (i = 1; i < RMResourceManager::GetLogicalProcessNumber(); i++)
		{
			if (PLMPITaskDriver::GetDriver()->IsFileServer(i))
			{
				context.Send(MPI_COMM_WORLD, i, MASTER_TRACER_MPI);
				serializer.OpenForWrite(&context);
				serializer.PutString(PLParallelTask::GetDriver()->GetTracerMPI()->GetFileName());
				serializer.PutBoolean(
				    PLParallelTask::GetDriver()->GetTracerMPI()->GetShortDescription());
				serializer.PutBoolean(
				    PLParallelTask::GetDriver()->GetTracerMPI()->GetSynchronousMode());
				serializer.PutBoolean(
				    PLParallelTask::GetDriver()->GetTracerMPI()->GetTimeDecorationMode());
				serializer.Close();
			}
		}
	}

	// [Envoi de message vers la classe PLMPISlave]

	// Initialisation du tableau des esclaves
	GetTask()->oaSlavesByRank.SetSize(RMResourceManager::GetLogicalProcessNumber());

	// Reception du nom du host,  de chaque esclave
	ObjectArray* oaSlavesInHost;
	for (i = 0; i < task->nWorkingProcessNumber; i++)
	{
		context.Recv(MPI_COMM_WORLD, MPI_ANY_SOURCE, SLAVE_RANKS_HOSTNAME);
		serializer.OpenForRead(&context);
		sHostName = serializer.GetString();
		serializer.Close();

		// Construction de l'esclave
		slave = new PLSlaveState;
		slave->SetRank(context.GetRank());
		slave->SetHostName(sHostName);
		slave->SetProgression(0);
		slave->SetTaskPercent(1.0 / task->nWorkingProcessNumber); // Pour l'initialisation
		GetTask()->oaSlaves.Add(slave);

		// Ajout de l'esclave dans le dictionaire hsostName / liste des esclaves
		oaSlavesInHost = cast(ObjectArray*, GetTask()->odHosts.Lookup(sHostName));
		if (oaSlavesInHost == NULL)
		{
			oaSlavesInHost = new ObjectArray;
			GetTask()->odHosts.SetAt(sHostName, oaSlavesInHost);
		}
		oaSlavesInHost->Add(slave);

		// Ajout de l'esclave dans le tableau indexe par les rangs
		GetTask()->oaSlavesByRank.SetAt(slave->GetRank(), slave);
		GetTask()->ivGrantedSlaveIds.Add(slave->GetRank());
	}

	// On brasse la liste des rangs pour que les utilisateurs de la lib
	// ne fassent pas d'hypothese sur les rangs (ordre, sequence, etc...)
	debug(GetTask()->ivGrantedSlaveIds.Shuffle());

	// Envoi du nombre d'esclave lances a tous les processus
	MPI_Bcast(&task->nWorkingProcessNumber, 1, MPI_INT, 0, *PLMPITaskDriver::GetTaskComm());

	// Envoi des parametres ...
	context.Bcast(*PLMPITaskDriver::GetDriver()->GetTaskComm());
	serializer.OpenForWrite(&context);

	// ... du fichier de logs,
	serializer.PutString(PLParallelTask::sParallelLogFileName);

	// ... du tracer de la performance,
	assert(GetTask()->sPerformanceTaskName != "");
	serializer.PutString(GetTask()->sPerformanceTaskName);

	// ...  des resources allouees
	shared_rg.SerializeObject(&serializer, RMParallelResourceDriver::grantedResources);

	// .. et de la liste des serveurs de fichier
	PLShared_ObjectDictionary shared_odFileServer(new PLShared_IntObject);
	shared_odFileServer.SerializeObject(&serializer, &PLMPITaskDriver::GetDriver()->odFileServers);
	serializer.Close(); // BCast parameters

	// Nommage des statistiques
	statsWorkingSlave.SetDescription(sTmp + "Working slaves (over " + IntToString(task->GetProcessNumber()) + ")");

	// Traitement :
	//		- MasterInitialize,
	//		- boucle sur les SlaveProcess
	//		- MasterFinalize
	bOk = Process();

	// Deconnexion MPI de l'intracom (on reste toujours connecte dans MPI_COMM_WORLD)
	Disconnect();

	// Reinitialisation des esclaves
	for (i = 0; i < GetTask()->oaSlaves.GetSize(); i++)
	{
		slave = cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i));
		slave->Initialize();
	}

	// Affichage des stats sur le nombre d'esclaves
	if (PLParallelTask::GetVerbose())
		AddMessage(statsWorkingSlave.WriteString());

	nWorkingSlaves = 0;
	GetTask()->oaSlaves.DeleteAll();
	GetTask()->odHosts.DeleteAll();
	GetTask()->ivGrantedSlaveIds.SetSize(0);

	return bOk;
}

void PLMPIMaster::SendStop(boolean bOk)
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	int i;

	if (not bStopOrderDone)
	{
		bStopOrderDone = true;

		for (i = 0; i < GetTask()->oaSlaves.GetSize(); i++)
		{
			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddSend(cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i))->GetRank(),
							MASTER_STOP_ORDER);
			context.Send(MPI_COMM_WORLD, cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i))->GetRank(),
				     MASTER_STOP_ORDER);
			serializer.OpenForWrite(&context);
			serializer.PutBoolean(bOk);
			serializer.Close();
		}
	}
}

boolean PLMPIMaster::Process()
{
	MPI_Status receivedStatus;
	longint lNbPolling;
	double dTaskPercentage;
	ALString sTmp;
	PLSerializer serializer;
	boolean bMasterPrepareTaskInputOk;
	boolean bMasterFinalizeOk;
	boolean bMasterInitializeOk;
	PLMPIMsgContext context;
	PLSlaveState* theWorker;
	int i;

	lNbPolling = 0;
	dGlobalProgression = 0;
	bMasterPrepareTaskInputOk = true;
	bStopOrderDone = false;
	bInterruptionRequested = false;
	bIsProcessing = false;
	nInitialisationCount = 0;
	nFinalisationCount = 0;
	bSlaveError = false;
	bMasterError = false;

	// Est-ce que le traitement global est termine :
	//      les esclaves ont realise toutes les taches qu'ils avaient a faire et
	//		il n'y a plus de tache a lancer
	task->bJobIsTerminated = false;

	// Initialisation du maitre
	task->SetSharedVariablesNoPermission(&task->oaInputVariables);
	bMasterInitializeOk = GetTask()->CallMasterInitialize();

	// Envoi des parametres
	context.Bcast(*PLMPITaskDriver::GetTaskComm());
	serializer.OpenForWrite(&context);
	GetTask()->SerializeSharedVariables(&serializer, &GetTask()->oaSharedParameters, false);
	serializer.Close(); // BCast task parameters

	if (not bMasterInitializeOk)
	{
		// Notification d'arret aux esclaves
		SendStop(false);
	}

	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTask()->GetTaskLabel());
	TaskProgression::DisplayLabel(GetTask()->PROGRESSION_MSG_SLAVE_INITIALIZE);

	tTimeBeforeSleep.Start();
	nOldProgression = 0;
	dGlobalProgression = 0;

	// Si une demande d'arret a eu lieu pendant le master initialize
	// On demande l'arret immediatement
	if (TaskProgression::IsInterruptionRequested())
	{
		SendStop(false);
		bInterruptionRequested = true;
	}
	else
	{
		// Boucle de travail
		while (bMasterInitializeOk and bMasterPrepareTaskInputOk and
		       (nFinalisationCount < nInitialisationCount or not bIsProcessing))
		{
			lNbPolling++;

			// Gestion de l'interruption
			if (TaskProgression::IsInterruptionRequested() and
			    not bMasterError) // En cas d'erreur un message d'arret a deje ete envoye
			{
				NotifyInterruptionRequested();
			}

			// Test si le nombre de messages utilisateur est arrive a saturation
			UpdateMaxErrorFlow();

			// Boucle de lancement de tous les esclaves disponibles
			theWorker = GetTask()->GetReadySlave();
			while (theWorker != NULL and not bInterruptionRequested and not task->bJobIsTerminated and
			       not bSlaveError and not bMasterError)
			{
				dTaskPercentage = 0;

				// Preparation des donnees
				bMasterPrepareTaskInputOk = task->CallMasterPrepareTaskInput(
				    dTaskPercentage, task->bJobIsTerminated, theWorker);

				// Arret des esclaves en cas de probleme
				if (not bMasterPrepareTaskInputOk)
					bMasterError = true;

				assert(not(task->bJobIsTerminated and task->bSlaveAtRestWithoutProcessing));

				// On donne l'ordre de lancer l'esclave si il n'a pas ete mis en sommeil
				if (not task->bJobIsTerminated and bMasterPrepareTaskInputOk and
				    not task->bSlaveAtRestWithoutProcessing)
				{
					// Reception de tous les messages de progression issus de l'esclave
					DischargePendingCommunication(theWorker->GetRank(), SLAVE_PROGRESSION);

					// Lancement de l'eclave
					GiveNewJob(theWorker, dTaskPercentage);
				}

				// Reinitialisation du flag qui est mis a jour dans MasterPrepareTaskInput
				task->bSlaveAtRestWithoutProcessing = false;

				// Prochain esclave disponible
				theWorker = GetTask()->GetReadySlave();
			}

			// Reception et traitement d'un message
			if (task->shared_bBoostedMode and not task->bJobIsTerminated)
				ReceiveAndProcessMessage(MPI_ANY_TAG, MPI_ANY_SOURCE);
			else
			{
				if (CheckNewMessage(MPI_ANY_SOURCE, MPI_COMM_WORLD, receivedStatus, MPI_ANY_TAG))
					ReceiveAndProcessMessage(receivedStatus.MPI_TAG, receivedStatus.MPI_SOURCE);
			}

			// Arret des esclaves
			// Si plus personne ne travaille et si il n'y a plus de taches a lancer
			// => le traitement est termine
			if ((bMasterError or bSlaveError or (task->bJobIsTerminated and workers.IsEmpty())) and
			    not bStopOrderDone)
			{
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddTrace("Processing done");
				if (GetTracerProtocol()->GetActiveMode())
					GetTracerProtocol()->AddTrace("Processing done");

				// Notification d'arret aux esclaves
				SendStop(not(bMasterError or bSlaveError));

				TaskProgression::EndTask();

				// Remise a 0 de la progression de tous les esclaves
				ResetSlavesProgression();

				// Changement de l'etat de chaque esclave
				for (i = 0; i < task->oaSlaves.GetSize(); i++)
				{
					cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i))->SetState(State::FINALIZE);
				}

				// Demarrage de la tache de finalisation
				TaskProgression::BeginTask();
				TaskProgression::DisplayMainLabel(GetTask()->GetTaskLabel());
				TaskProgression::DisplayLabel(GetTask()->PROGRESSION_MSG_SLAVE_FINALIZE);
			}

			// Affichage de la progression suivant la position dans la tache (slaveInitialize ou
			// SlaveProcess)
			if (bIsProcessing)
				TaskProgression::DisplayProgression(ComputeGlobalProgression(true));
			else
				TaskProgression::DisplayProgression(ComputeGlobalProgression(false));

			// On dort un tout petit peu quand il n'y a rien a faire, ca laisse quand meme beaucoup de place
			// aux autres
			if (not GetTask()->shared_bBoostedMode and
			    tTimeBeforeSleep.GetElapsedTime() > PLMPITaskDriver::TIME_BEFORE_SLEEP and
			    CanSleep()) // TODO revoir cette methode la CPU du maitre est tres occupee ...
			{
				SystemSleep(0.001);
			}
		}
	}
	TaskProgression::EndTask();

	// A partir d'ici on est sur que les esclaves n'enverront plus de messages
	MPI_Barrier(*PLMPITaskDriver::GetTaskComm()); // BARRIER MSG

	// Il peut rester des messages de type warning
	while (CheckNewMessage(MPI_ANY_SOURCE, MPI_COMM_WORLD, receivedStatus, MPI_ANY_TAG))
	{
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(receivedStatus.MPI_SOURCE, receivedStatus.MPI_TAG);

		// On recoit les messages pour les afficher et les fin de slaveProcess et slaveInitialize, pour avoir
		// les etats et les index corrects, on pourra ainsi afficher correctement les messages
		switch (receivedStatus.MPI_TAG)
		{
		case SLAVE_END_PROCESSING:
			ReceiveAndProcessMessage(SLAVE_END_PROCESSING, receivedStatus.MPI_SOURCE);
			break;
		case SLAVE_INITIALIZE_DONE:
			ReceiveAndProcessMessage(SLAVE_INITIALIZE_DONE, receivedStatus.MPI_SOURCE);
			break;
		case SLAVE_USER_MESSAGE:
			ReceiveAndProcessMessage(SLAVE_USER_MESSAGE, receivedStatus.MPI_SOURCE);
			break;
		default:
			// Sinon on ne fait que recevoir
			ReceivePendingMessage(receivedStatus);
		}
	}

	// Tout les messages on ete traites par les esclaves, on peut continuer
	MPI_Barrier(*PLMPITaskDriver::GetTaskComm()); // BARRIER MSG 2

	// Affichage des derniers messages utilisateur
	messageManager.PrintMessages();
	assert(messageManager.IsEmpty());

	// Finalisation du maitre
	bMasterFinalizeOk =
	    GetTask()->CallMasterFinalize(not bInterruptionRequested and not bSlaveError and not bMasterError);

	if (GetTracerPerformance()->GetActiveMode())
	{
		GetTracerPerformance()->AddTrace("DONE");
		GetTracerPerformance()->AddTrace(sTmp + "nb message probing " + LongintToReadableString(lNbPolling));
	}

	// On test l'interruption ici pour renvoyer false en cas d'interruption
	if (not bInterruptionRequested)
		bInterruptionRequested = TaskProgression::IsInterruptionRequested();
	bIsProcessing = false;
	return not bInterruptionRequested and bMasterFinalizeOk and bMasterInitializeOk and not bSlaveError and
	       not bMasterError;
}

int PLMPIMaster::ReceiveAndProcessMessage(int nAnyTag, int nAnySource)
{
	ALString sTmp;
	PLSlaveState* aSlave;
	POSITION position;
	PLSerializer serializer;
	PLShared_ObjectArray shared_oa(new PLSharedErrorWithIndex);
	PLSharedErrorWithIndex shared_error;
	Error* fataleError;
	PLMPIMsgContext context;
	State nProgressionSlaveState;
	int i;
	int nSlaveTaskIndex;
	int nSlaveProgression;
	boolean bFinalizeOk;
	boolean bSlaveProcessOnce;
	boolean bOk;
	int nSource;
	int nTag;

	context.Recv(MPI_COMM_WORLD, nAnySource, nAnyTag);
	serializer.OpenForRead(&context);
	nSource = context.GetRank();
	nTag = context.GetTag();
	bOk = true;
	bSlaveProcessOnce = false;
	switch (nTag)
	{
	case SLAVE_INITIALIZE_DONE:

		// L'esclave a termine l'initialisation : il peut travailler
		nSlaveTaskIndex = serializer.GetInt();
		serializer.Close();
		aSlave = GetTask()->GetSlaveWithRank(nSource);
		assert(bSlaveError or bMasterError or bInterruptionRequested or aSlave->GetState() == State::INIT);
		aSlave->SetState(State::PROCESSING);
		aSlave->SetTaskIndex(nSlaveTaskIndex);
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, nTag);

		if (not bIsProcessing)
		{
			bIsProcessing = true;
			// C'est le premier esclave a travailler, on va chager de progressionTask
			// il faut reinitialiser la progression des esclaves en cours d'initialisation
			// pour que la progression de l'initialisation ne soit pas prise en compte pendant le processing
			TaskProgression::EndTask();
			ResetSlavesProgression();
			TaskProgression::BeginTask();
			TaskProgression::DisplayMainLabel(GetTask()->GetTaskLabel());
			TaskProgression::DisplayLabel(GetTask()->PROGRESSION_MSG_PROCESSING);
		}
		break;

	case SLAVE_END_PROCESSING:

		// Recuperation de l'esclave qui a fini
		aSlave = GetTask()->GetSlaveWithRank(nSource);
		assert(bSlaveError or bMasterError or bInterruptionRequested or
		       aSlave->GetState() == State::PROCESSING);

		// Mise a jour de la progression globale
		dGlobalProgression += aSlave->GetTaskPercent() * 100;

		// Mise a jour de l'index de la derniere tache
		task->nSlaveTaskIndex = aSlave->GetTaskIndex();

		// Mise a jour de l'esclave qui a fini
		aSlave->SetState(State::READY);
		aSlave->SetProgression(0);

		// On enleve l'esclave qui a fini de la liste des travailleurs
		bOk = FindPosOfRank(workers, nSource, position);
		assert(bOk);
		workers.RemoveAt(position);

		// Decrementation du nombre d'esclaves qui travaillent
		nWorkingSlaves--;

		// Autorisation de lecture pour les output variables
		task->SetSharedVariablesRW(&task->oaOutputVariables);
		task->DeserializeSharedVariables(&serializer, &task->oaOutputVariables);
		serializer.Close();

		// Reception des variables partagees
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, nTag);

		// Si le slaveProcess a echoue on demande l'arret
		if (task->output_bSlaveProcessOk == false)
			bSlaveError = true;

		// Gestion des messages vers l'utilisateur
		// Stockage de l'index de la tache et du nombre de ligne lues pour cette tache
		messageManager.SetTaskLineNumber(aSlave->GetTaskIndex(), task->output_lLocalLineNumber);

		// Affichage des messages si c'est possible
		// (si les messages des taches precedentes ont ete affiches)
		messageManager.PrintMessages();

		// Aggregation
		if (not bSlaveError)
		{
			GetTask()->nNextWorkingSlaveRank = nSource;

			// Aggregation des resultats
			bOk = task->CallMasterAggregate();
			if (not bOk)
				bMasterError = true;
		}

		// Reception de tous les messages de progression issus de l'esclave
		DischargePendingCommunication(nSource, SLAVE_PROGRESSION);
		bSlaveProcessOnce = true;

		break;

	case SLAVE_NEED_STOP:

		// l'esclave a besoin de s'arreter : il a probablement rencontre une erreur
		serializer.Close();
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, nTag);

		// Notification d'arret a tous les esclaves
		bSlaveError = true;

		// Stockage de l'index de la tache et du nombre de ligne lues pour cette tache
		// Sauf si aucun SlaveProcess n'a eu lieu (erreur dans SlaveInitialize)
		if (bSlaveProcessOnce)
		{
			aSlave = GetTask()->GetSlaveWithRank(nSource);
			messageManager.SetTaskLineNumber(aSlave->GetTaskIndex(), 0);
		}

		// Affichage des messages si c'est possible
		// (si les messages des taches precedentes ont ete affiches)
		messageManager.PrintMessages();

		break;

	case SLAVE_FATAL_ERROR:

		shared_error.Deserialize(&serializer);
		serializer.Close();
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, nTag);
		fataleError = shared_error.GetErrorIndex()->GetError()->Clone();
		Global::AddErrorObject(fataleError);
		break;

	case SLAVE_PROGRESSION:

		// Progression en pourcent de la tache courante de l'esclave
		nSlaveProgression = serializer.GetInt();
		nProgressionSlaveState = PLSlaveState::IntToState(serializer.GetInt());
		serializer.Close();
		aSlave = GetTask()->GetSlaveWithRank(nSource);

		// On traite la progression seulement si :
		//	- le message est synchrone (l'esclave qui l'a envoye a le meme etat que celui qui le recoit)
		// et l'une ou l'autre de ces conditions :
		//	- l'esclave est en phase d'initialisation et aucun esclave n'est en slaveProcess (bIsProcessing)
		//	- l'esclave est en phase de processing
		//	- l'esclave est en phase de finalisation
		if (aSlave->GetState() == nProgressionSlaveState)
		{
			assert(not aSlave->IsProcessing() or bIsProcessing);
			if (aSlave->IsProcessing() or aSlave->IsFinalize() or (aSlave->IsInit() and not bIsProcessing))
				aSlave->SetProgression(nSlaveProgression);
		}
		break;

	case SLAVE_USER_MESSAGE:

		// Message vers l'utilisateur (Warning, error ..)
		int nMethod;

		// Reception du message
		nMethod = serializer.GetInt();
		shared_oa.Deserialize(&serializer);
		serializer.Close();

		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, SLAVE_USER_MESSAGE);

		// Stockage ou affichage suivant le moment ou les messages ont ete emis (Initialize process finalize)
		switch (nMethod)
		{
		case 0:
			// SlaveInitialize
			// On n'affiche que les message du premier esclave qui a emis si bSlaveInitializeErrorsOnce
			// Sinon on affiche les messages de tous les esclave
			if (task->bSlaveInitializeErrorsOnce and nFirstSlaveInitializeMessageRank == -1)
			{
				nFirstSlaveInitializeMessageRank = nSource;
			}
			if (not task->bSlaveInitializeErrorsOnce or nSource == nFirstSlaveInitializeMessageRank)
			{
				for (i = 0; i < shared_oa.GetObjectArray()->GetSize(); i++)
					Global::AddErrorObject(
					    cast(PLErrorWithIndex*, shared_oa.GetObjectArray()->GetAt(i))->GetError());
			}
			break;
		case 1:
			// Slave Process
			// Stockage des messages
			for (i = 0; i < shared_oa.GetObjectArray()->GetSize(); i++)
			{
				messageManager.AddMessage(
				    GetTask()->GetSlaveWithRank(nSource)->GetTaskIndex(),
				    cast(PLErrorWithIndex*, shared_oa.GetObjectArray()->GetAt(i))->Clone());
			}
			break;
		case 2:
			// Slave Finalize
			// On n'affiche que les message du premier esclave qui a emis si bSlaveFinalizeErrorsOnce
			// Sinon on affiche les messages de tous les esclave
			if (task->bSlaveFinalizeErrorsOnce and nFirstSlaveFinalizeMessageRank == -1)
			{
				nFirstSlaveFinalizeMessageRank = nSource;
			}
			if (not task->bSlaveFinalizeErrorsOnce or nSource == nFirstSlaveFinalizeMessageRank)
			{
				for (i = 0; i < shared_oa.GetObjectArray()->GetSize(); i++)
					Global::AddErrorObject(
					    cast(PLErrorWithIndex*, shared_oa.GetObjectArray()->GetAt(i))->GetError());
			}
			break;
		default:
			assert(false);
		}

		// Nettoyage
		shared_oa.Clean();
		break;

	case SLAVE_DONE:
		// Message de terminaison
		// envoye par les esclaves apres le SlaveFinalize: TOUT est fini
		bFinalizeOk = serializer.GetBoolean();
		serializer.Close();
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(context.GetRank(), context.GetTag());

		nFinalisationCount++;
		assert(nFinalisationCount <= nInitialisationCount);

		aSlave = GetTask()->GetSlaveWithRank(nSource);
		assert(bMasterError or bSlaveError or bInterruptionRequested or aSlave->GetState() == State::FINALIZE or
		       task->shared_nCrashTestType == PLParallelTask::TestType::USER_INTERRUPTION);
		aSlave->SetState(State::DONE);
		aSlave->SetProgression(0);

		if (not bFinalizeOk)
		{
			bSlaveError = true;
			if (GetTask()->GetVerbose())
				AddError("An error occurred in worker finalization");
		}
		break;

	default:
		serializer.Close();
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(nSource, nTag);
		if (GetTask()->GetVerbose())
			AddError(sTmp + "unexpected message with status " + IntToString(nTag));
		if (GetTracerProtocol()->GetActiveMode())
			GetTracerProtocol()->AddTrace(sTmp + "Unexpected message received with tag " +
						      GetTagAsString(nTag));

		assert(false);
	}

	// Remise a zero du timer de mise en sommeil
	tTimeBeforeSleep.Reset();
	tTimeBeforeSleep.Start();
	return nTag;
}

void PLMPIMaster::GiveNewJob(PLSlaveState* slave, double dTaskPercent)
{

	PLSerializer serializer;
	PLMPIMsgContext context;

	check(slave);
	bSpawnedDone = true;

	// Initialisation de l'etat de l'esclave
	slave->SetTaskPercent(dTaskPercent);
	slave->SetProgression(0);

	// Si l'esclave n'est pas encore initialise, il ne passe pas dans l'etat processing
	// il passera dans cet etat a la reception de sa fin d'initialisation
	// Ceci a une incidence dur l'affichage de la progression
	if (slave->IsReady())
	{
		slave->SetState(State::PROCESSING);

		// On met a jour l'index de la tache seulement si l'esclave est deja initialise
		// Sinon elle sera mise a jour apres le SlaveInitialize (l'esclave enverra l'index de la
		// tache avec le message de fin d'initialisation)
		slave->SetTaskIndex(task->nTaskProcessedNumber);
	}
	else
	{
		slave->SetState(State::INIT);
		nInitialisationCount++;
	}

	// Envoi du TaskId et du mode Silencieux
	task->SetSharedVariablesRW(&task->oaInputVariables);
	task->input_nTaskProcessedNumber = task->nTaskProcessedNumber;
	task->input_bSilentMode = Global::GetSilentMode();
	task->SetSharedVariablesNoPermission(&task->oaInputVariables);

	// On envoie tous les parametres de la tache
	context.Send(MPI_COMM_WORLD, slave->GetRank(), MASTER_TASK_INPUT); // TODO Passer cet envoi en non bloquant
	serializer.OpenForWrite(&context);
	task->SerializeSharedVariables(&serializer, &GetTask()->oaInputVariables, true);
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(slave->GetRank(), MASTER_TASK_INPUT);
	serializer.Close();

	// Incrementation du nombre d'esclaves qui travaillent
	nWorkingSlaves++;
	workers.AddTail(slave);

	// Incrementation du taskId
	task->nTaskProcessedNumber++;
	messageManager.AddTaskId(task->nTaskProcessedNumber);

	// Statistiques sur les esclaves
	statsWorkingSlave.AddValue(nWorkingSlaves);
}

void PLMPIMaster::ResetSlavesProgression()
{
	PLSlaveState* slave;
	int i;

	// re-initialisation des indicateurs de progression
	nOldProgression = 0;
	dGlobalProgression = 0;

	// Initialisation de la progression de chaque esclave
	for (i = 0; i < GetTask()->oaSlaves.GetSize(); i++)
	{
		slave = cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i));
		slave->SetProgression(0);
	}
}

int PLMPIMaster::ComputeGlobalProgression(boolean bSlaveProcess)
{
	double dCurrentProgression;
	int i;
	int nProgression;
	PLSlaveState* slave;
	double dTaskPart;
	boolean bDisplay = false;

	// Progression deja effectuee
	dCurrentProgression = dGlobalProgression;
	if (bDisplay)
	{
		cout << "Progression :" << endl;
		cout << "\told " << nOldProgression << endl;
	}
	// On ajoute la progression de chaque esclave
	for (i = 0; i < GetTask()->oaSlaves.GetSize(); i++)
	{
		slave = cast(PLSlaveState*, GetTask()->oaSlaves.GetAt(i));

		// Calcul de la part de la tache globale realisee par l'esclave
		if (bSlaveProcess)
			dTaskPart = slave->GetTaskPercent();
		else
			dTaskPart = 1.0 / GetTask()->oaSlaves.GetSize();

		// Ajout de la progression de l'esclave courant
		if (bDisplay)
			cout << "\t" << dCurrentProgression << " += " << slave->GetProgression() << " * " << dTaskPart
			     << endl;
		dCurrentProgression += slave->GetProgression() * dTaskPart;
	}
	nProgression = (int)floor(dCurrentProgression + 0.5);
	if (bDisplay)
		cout << "=> " << nProgression << endl;

	// On empeche la barre d'avancement de reculer
	// c'est un bug qui sera leve par l'assertion en debug
	assert(nOldProgression <= nProgression or TaskProgression::IsInterruptionRequested());
	if (nOldProgression > nProgression)
		nProgression = nOldProgression;
	else
		nOldProgression = nProgression;

	// On borne le resultat car la somme des progressions
	// des esclaves peut etre > 100 a cause de pbms d'arrondi
	if (nProgression > 100)
		nProgression = 100;
	return nProgression;
}

void PLMPIMaster::NotifyInterruptionRequested()
{
	int nSlaveRank;
	int i;
	PLMPIMsgContext context;
	PLSerializer serializer;

	if (not bInterruptionRequested)
	{
		if (GetTracerProtocol()->GetActiveMode())
			GetTracerProtocol()->AddTrace("Send Interruption requested");

		for (i = 0; i < GetTask()->ivGrantedSlaveIds.GetSize(); i++)
		{
			nSlaveRank = GetTask()->ivGrantedSlaveIds.GetAt(i);
			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddSend(nSlaveRank, INTERRUPTION_REQUESTED);

			// On envoie en utilisant un serializer, car le message peut etre recu par
			// PLMPISlave et celui-ci attend un serializer
			context.Isend(MPI_COMM_WORLD, nSlaveRank, INTERRUPTION_REQUESTED);
			serializer.OpenForWrite(&context);
			serializer.Close();
		}
		bInterruptionRequested = true;
		bStopOrderDone = true;
	}
}

void PLMPIMaster::MasterFatalError()
{
	MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
}

const ALString PLMPIMaster::GetClassLabel() const
{
	ALString s = "PLMPIMaster";
	return s + ":" + task->GetClassLabel();
}

void PLMPIMaster::AddSimpleMessage(const ALString& sLabel) const
{
	task->AddSimpleMessage(sLabel);
}

void PLMPIMaster::AddMessage(const ALString& sLabel) const
{
	task->AddMessage(sLabel);
}

void PLMPIMaster::AddWarning(const ALString& sLabel) const
{
	task->AddWarning(sLabel);
}

void PLMPIMaster::AddError(const ALString& sLabel) const
{
	task->AddError(sLabel);
}

void PLMPIMaster::DischargePendingCommunication(int nRank, int nTag)
{
	boolean bDisplay = false;
	MPI_Status status;
	int nThereArePendingMessages;
	ALString sTmp;

	// Tant qu'il y a des messages dans les tuyaux
	nThereArePendingMessages = 1;
	while (nThereArePendingMessages)
	{
		// Test non bloquant de la presence de messages
		MPI_Iprobe(nRank, nTag, MPI_COMM_WORLD, &nThereArePendingMessages, &status);
		if (nThereArePendingMessages)
		{
			if (PLParallelTask::GetVerbose())
				TraceWithRank(sTmp + "discharge pending comm from " + IntToString(status.MPI_SOURCE) +
					      " with tag " + GetTagAsString(status.MPI_TAG));
			if (bDisplay)
				cout << GetProcessId() << " "
				     << "Discharge pending com" << endl;

			// Reception du message
			ReceivePendingMessage(status);
		}
	}
}

void PLMPIMaster::ReceivePendingMessage(MPI_Status status)
{
	// Simple reception du message : on connait sa taille max
	MPI_Recv(sBufferDischarge, MemSegmentByteSize, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,
		 &status);
}

boolean PLMPIMaster::FindPosOfRank(ObjectList& slavesList, int nValue, POSITION& pos)
{
	boolean bOk;
	PLSlaveState* aSlave;

	bOk = false;
	aSlave = NULL;
	pos = slavesList.GetHeadPosition();
	while (pos != NULL)
	{
		aSlave = cast(PLSlaveState*, slavesList.GetNext(pos));
		if (aSlave->GetRank() == nValue)
		{
			bOk = true;
			break;
		}
	}
	if (bOk)
	{
		pos = slavesList.Find(aSlave, NULL);
		ensure(pos != NULL);
	}
	return bOk;
}
