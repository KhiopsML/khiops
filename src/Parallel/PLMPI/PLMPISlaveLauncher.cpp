// Copyright (c) 2023 Orange. All rights reserved.
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
	PLParallelTask* taskToLaunch;
	PLParallelTask* clonedTaskToLaunch;
	PLMPIMsgContext context;

	IntVector ivExcludeSlaves;
	PLSerializer serializer;
	MPI_Errhandler errHandler;
	IntVector ivServerRanks;
	int nColor;
	boolean bIsWorkingSlave;

	require(GetProcessId() != 0);
	require(MPI_COMM_WORLD != MPI_COMM_NULL);

	// Pas d'appel a la JVM dans les esclaves
	UIObject::SetUIMode(UIObject::Textual);

	// Boucle principale : les exe attendent les ordres du master : lancer une nouvelle tache (instantiation d'une
	// tache) ou fin du programme
	bOrderToQuit = false;
	while (not bOrderToQuit)
	{
		SystemSleep(0.001);

		// Initialisation des ressources a la demande du maitre, au lancement du programme
		SlaveInitializeResourceSystem();

		// Lancement des serveurs de fichiers
		if (IsTimeToLaunchFileServer(&ivServerRanks))
		{
			LaunchFileServer(&ivServerRanks, bOrderToQuit);
		}

		if (IsFileServerEnd())
		{

			// Deconnexion du communicateur Process, appel de cette methode chez le maitre et chez les
			// esclaves
			MPI_Barrier(*PLMPITaskDriver::GetProcessComm());        // Necessaire pour MPICH
			MPI_Comm_disconnect(PLMPITaskDriver::GetProcessComm()); // DISCONNECT COMM_PROCESS
		}

		// Instantiation d'une tache parallele, a la demande du maitre
		if (IsSlaveToLaunch(sTaskName))
		{

			assert(*PLMPITaskDriver::GetProcessComm() != MPI_COMM_NULL);

			// Bcast des exclus
			context.Bcast(*PLMPITaskDriver::GetProcessComm());
			serializer.OpenForRead(&context);
			serializer.GetIntVector(&ivExcludeSlaves);
			serializer.Close();

			// Est-ce que cet esclave travaille ?
			bIsWorkingSlave = not ivExcludeSlaves.GetAt(GetProcessId());

			// Creation d'un nouveau comminicateur qui contient le maitre et les eaclaves qui travaillent
			bIsWorkingSlave ? nColor = 1 : nColor = 0;
			MPI_Comm_split(*PLMPITaskDriver::GetProcessComm(), nColor, GetProcessId(),
				       PLMPITaskDriver::GetTaskComm());

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
				// PLMPISlave::Disconnect();// BARRIER DISCONNECTION
				MPI_Barrier(*PLMPITaskDriver::GetTaskComm());
				MPI_Comm_disconnect(PLMPITaskDriver::GetTaskComm());
			}
		}

		// Test si on a un ordre de fin de programme (bOrderToQuit peu etre mis a jour dans FileServer)
		bOrderToQuit = bOrderToQuit or ReceiveOrderToQuit();
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

void PLMPISlaveLauncher::SlaveInitializeResourceSystem()
{
	PLSerializer serializer;
	int flag;
	MPI_Status status;
	PLMPIMsgContext context;

	require(GetProcessId() != 0);

	// Est-ce que le maitre demande une initialisation
	MPI_Iprobe(0, MASTER_RESOURCES, MPI_COMM_WORLD, &flag, &status);
	if (flag)
	{
		// Reception du message
		context.Recv(MPI_COMM_WORLD, 0, MASTER_RESOURCES);
		serializer.OpenForRead(&context);
		RMResourceConstraints::SetMemoryLimit(serializer.GetInt());
		RMResourceConstraints::SetMemoryLimitPerProc(serializer.GetInt());
		RMResourceConstraints::SetDiskLimit(serializer.GetInt());
		serializer.Close();

		// Initialisation des ressources
		PLMPITaskDriver::GetDriver()->InitializeResourceSystem();
	}
}

boolean PLMPISlaveLauncher::IsSlaveToLaunch(ALString& sTaskName)
{
	int flag;
	MPI_Status status;
	PLSerializer serializer;
	PLMPIMsgContext context;

	require(GetProcessId() != 0);

	MPI_Iprobe(0, MASTER_LAUNCH_WORKERS, MPI_COMM_WORLD, &flag, &status);
	if (flag)
	{
		context.Recv(MPI_COMM_WORLD, 0, MASTER_LAUNCH_WORKERS);
		serializer.OpenForRead(&context);
		sTaskName = serializer.GetString();
		serializer.Close();

		ensure(sTaskName != "");
		return true;
	}
	else
	{
		sTaskName = "";
		return false;
	}
}

boolean PLMPISlaveLauncher::IsTimeToLaunchFileServer(IntVector* serverRanks)
{
	int flag;
	MPI_Status status;
	PLSerializer serializer;
	PLMPIMsgContext context;

	require(GetProcessId() != 0);
	require(serverRanks != NULL);

	MPI_Iprobe(0, MASTER_LAUNCH_FILE_SERVERS, MPI_COMM_WORLD, &flag, &status);
	if (flag)
	{
		context.Recv(MPI_COMM_WORLD, 0, MASTER_LAUNCH_FILE_SERVERS);
		serializer.OpenForRead(&context);
		serializer.GetIntVector(serverRanks);
		serializer.Close();
		return true;
	}
	else
	{
		serverRanks->SetSize(0);
		return false;
	}
}

boolean PLMPISlaveLauncher::IsFileServerEnd()
{
	int flag;
	MPI_Status status;

	require(GetProcessId() != 0);

	MPI_Iprobe(0, MASTER_STOP_FILE_SERVERS, MPI_COMM_WORLD, &flag, &status);
	if (flag)
	{
		MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_STOP_FILE_SERVERS, MPI_COMM_WORLD, &status);
		return true;
	}
	else
	{
		return false;
	}
}
boolean PLMPISlaveLauncher::ReceiveOrderToQuit()
{
	MPI_Status status;
	int nOngoingMessage;

	require(GetProcessId() != 0);

	MPI_Iprobe(0, MASTER_QUIT, MPI_COMM_WORLD, &nOngoingMessage, &status);
	if (nOngoingMessage)
	{
		if (PLMPITaskDriver::GetDriver()->GetTracerMPI()->GetActiveMode())
			cast(PLMPITracer*, PLMPITaskDriver::GetDriver()->GetTracerMPI())->AddRecv(0, MASTER_QUIT);
		MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_QUIT, MPI_COMM_WORLD, &status);
		return true;
	}
	return false;
}