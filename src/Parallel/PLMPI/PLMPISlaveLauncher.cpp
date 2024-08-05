// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPISlaveLauncher.h"
#include "PLMPITaskDriver.h"

PLMPISlaveLauncher::PLMPISlaveLauncher() {}

PLMPISlaveLauncher::~PLMPISlaveLauncher() {}

void PLMPISlaveLauncher::Launch()
{
	boolean bOrderToQuit;
	ALString sTaskName;
	PLMPIMsgContext context;
	IntVector ivExcludeSlaves;
	PLSerializer serializer;
	IntVector ivServerRanks;

	require(GetProcessId() != 0);
	require(MPI_COMM_WORLD != MPI_COMM_NULL);

	// Pas d'appel a la JVM dans les esclaves
	UIObject::SetUIMode(UIObject::Textual);

	// Boucle principale : les processus attendent les ordres du master : lancer une nouvelle tache (instantiation
	// d'une tache) ou fin du programme
	bOrderToQuit = false;
	MPI_Status status;
	int nFlag;
	ivServerRanks.SetSize(0);
	while (not bOrderToQuit)
	{
		SystemSleep(0.001);
		MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &nFlag, &status);
		if (nFlag == 1)
		{
			assert(status.MPI_SOURCE == 0);
			switch (status.MPI_TAG)
			{
			case MASTER_RESOURCES:
				context.Recv(MPI_COMM_WORLD, 0, MASTER_RESOURCES);
				serializer.OpenForRead(&context);
				RMResourceConstraints::SetMemoryLimit(serializer.GetInt());
				RMResourceConstraints::SetDiskLimit(serializer.GetInt());
				serializer.Close();

				// Initialisation des ressources
				PLMPITaskDriver::GetDriver()->InitializeResourceSystem();
				break;

			case MASTER_LAUNCH_FILE_SERVERS:
				context.Recv(MPI_COMM_WORLD, 0, MASTER_LAUNCH_FILE_SERVERS);
				serializer.OpenForRead(&context);
				serializer.GetIntVector(&ivServerRanks);
				serializer.Close();
				LaunchFileServer(&ivServerRanks, bOrderToQuit);
				break;

			case MASTER_STOP_FILE_SERVERS:

				MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_STOP_FILE_SERVERS, MPI_COMM_WORLD, &status);

				// Deconnexion du communicateur Process, appel de cette methode chez le maitre et chez
				// les esclaves
				MPI_Barrier(*PLMPITaskDriver::GetProcessComm());        // Necessaire pour MPICH
				MPI_Comm_disconnect(PLMPITaskDriver::GetProcessComm()); // DISCONNECT COMM_PROCESS
				break;

			case MASTER_LAUNCH_WORKERS:
				context.Recv(MPI_COMM_WORLD, 0, MASTER_LAUNCH_WORKERS);
				serializer.OpenForRead(&context);
				sTaskName = serializer.GetString();
				serializer.Close();
				LaunchSlave(&ivExcludeSlaves, sTaskName);
				break;

			case MASTER_QUIT:
				if (PLMPITaskDriver::GetDriver()->GetTracerMPI()->GetActiveMode())
					cast(PLMPITracer*, PLMPITaskDriver::GetDriver()->GetTracerMPI())
					    ->AddRecv(0, MASTER_QUIT);
				MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_QUIT, MPI_COMM_WORLD, &status);
				bOrderToQuit = true;
				break;

			default:
				Global::AddError("", "",
						 "Unexpected message in the master slave protocol " +
						     GetTagAsString(status.MPI_TAG));
				assert(false);
				break;
			}
		}
	}
}

void PLMPISlaveLauncher::LaunchSlave(IntVector* ivExcludeSlaves, const ALString& sTaskName)
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	PLParallelTask* taskToLaunch;
	PLParallelTask* clonedTaskToLaunch;
	boolean bIsWorkingSlave;
	MPI_Errhandler errHandler;
	int nColor;
	assert(*PLMPITaskDriver::GetProcessComm() != MPI_COMM_NULL);

	// Bcast des exclus
	context.Bcast(*PLMPITaskDriver::GetProcessComm());
	serializer.OpenForRead(&context);
	serializer.GetIntVector(ivExcludeSlaves);
	serializer.Close();

	// Est-ce que cet esclave travaille ?
	bIsWorkingSlave = not ivExcludeSlaves->GetAt(GetProcessId());

	// Creation d'un nouveau communicateur qui contient le maitre et les esclaves qui travaillent
	bIsWorkingSlave ? nColor = 1 : nColor = 0;
	MPI_Comm_split(*PLMPITaskDriver::GetProcessComm(), nColor, GetProcessId(), PLMPITaskDriver::GetTaskComm());

	// Si l'esclave fait partie du groupe
	if (bIsWorkingSlave)
	{
		ensure(*PLMPITaskDriver::GetTaskComm() != MPI_COMM_NULL);

		// Modification du error handler
		MPI_Comm_create_errhandler(PLMPITaskDriver::ErrorHandler, &errHandler);
		MPI_Comm_set_errhandler(*PLMPITaskDriver::GetTaskComm(), errHandler);

		// Envoi du nom de la machine
		context.Send(MPI_COMM_WORLD, 0, SLAVE_RANKS_HOSTNAME);
		serializer.OpenForWrite(&context);
		serializer.PutString(GetLocalHostName());
		serializer.Close();

		// Recherche de la tache a lancer
		taskToLaunch = PLParallelTask::LookupTask(sTaskName);
		if (taskToLaunch == NULL)
		{
			Global::AddFatalError("SlaveLauncher", "", "Task " + sTaskName + " not found");
			MPI_Abort(MPI_COMM_WORLD, 1);
		}

		// Creation et lancement de la tache
		clonedTaskToLaunch = taskToLaunch->Create();
		clonedTaskToLaunch->SetToSlaveMode();
		clonedTaskToLaunch->Run();
		delete clonedTaskToLaunch;

		// Deconnexion
		MPI_Comm_disconnect(PLMPITaskDriver::GetTaskComm());
	}
}

void PLMPISlaveLauncher::LaunchFileServer(IntVector* ivServerRanks, boolean& bOrderToQuit)
{
	PLMPIMsgContext context;
	int i;
	boolean bIsFileServer; // Est-ce que l'esclave est un serveur de fichier
	int nColor;

	bOrderToQuit = false;

	// Rang de l'esclave
	bIsFileServer = false;
	for (i = 0; i < ivServerRanks->GetSize(); i++)
	{
		if (GetProcessId() == ivServerRanks->GetAt(i))
		{
			bIsFileServer = true;
			break;
		}
	}

	// Affectation de la couleur : si serveur de fichier => n'appartient pas au communicateur master/slaves
	bIsFileServer ? nColor = 0 : nColor = 1;

	// Construction du communicateur master/slaves
	MPI_Comm_split(MPI_COMM_WORLD, nColor, GetProcessId(), PLMPITaskDriver::GetProcessComm());
	assert(PLMPITaskDriver::GetProcessComm() != NULL);

	// Lancement du serveur de fichiers
	if (bIsFileServer)
	{
		PLMPIFileServerSlave* fileServer = new PLMPIFileServerSlave;
		fileServer->Run(bOrderToQuit);
		delete fileServer;
	}
}
