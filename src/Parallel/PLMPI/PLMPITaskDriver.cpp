// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPITaskDriver.h"

#include "PLMPIMaster.h"
#include "PLMPISlave.h"
#include "PLMPIFileServerSlave.h"
#include "PLMPISlaveLauncher.h"

MPI_Comm PLMPITaskDriver::commTask = MPI_COMM_NULL;
MPI_Comm PLMPITaskDriver::commProcesses = MPI_COMM_NULL;
boolean PLMPITaskDriver::bIsInitialized = false;
boolean PLMPITaskDriver::bIsFinalized = false;
PLMPITaskDriver PLMPITaskDriver::mpiDriver;
int PLMPITaskDriver::nIoRequestNumber = 0;
int PLMPITaskDriver::nFileServerRank = -1;
const double PLMPITaskDriver::TIME_BEFORE_SLEEP = 0.1;

PLMPITaskDriver::PLMPITaskDriver()
{
	MPI_Errhandler errHandler;
	int nRank;
	int nThreadProvided;

	if (not bIsInitialized)
	{
		// Remplace MPI_Init : Initialisation sans thread safe, peut-etre que MPI est plus efficace
		MPI_Init_thread(NULL, NULL, MPI_THREAD_SINGLE, &nThreadProvided);
		MPI_Comm_rank(MPI_COMM_WORLD, &nRank);
		SetProcessId(nRank);
		bIsInitialized = true;

		MPI_Comm_create_errhandler(ErrorHandler, &errHandler);
		MPI_Comm_set_errhandler(MPI_COMM_WORLD, errHandler);

		// Redefinition de la methode de sortie utilisateur
		// Appel en cas de Fatal Error
		if (PLParallelTask::IsMasterProcess())
		{
			AddUserExitHandler(PLParallelTask::Exit);
		}
		else
		{
			// N'a un effet qu'en cas d'assertions
			AddUserExitHandler(PLMPISlave::SlaveAbort);
		}

		// Handler pour traces
		FileBuffer::SetRequestIOFunction(RequestIO);
		FileBuffer::SetReleaseIOFunction(ReleaseIO);
	}
}

PLMPITaskDriver::~PLMPITaskDriver()
{
	FileBuffer::SetReleaseIOFunction(NULL);
	FileBuffer::SetRequestIOFunction(NULL);
	MPI_Finalize();
}

void PLMPITaskDriver::StartSlave()
{
	PLMPISlaveLauncher::Launch();
}

void PLMPITaskDriver::StopSlaves()
{
	int nSize;
	int i;

	if (GetProcessId() == 0)
	{
		// On donne l'ordre aux esclaves de s'arreter : sortie du programme
		MPI_Comm_size(MPI_COMM_WORLD, &nSize);
		for (i = 1; i < nSize; i++)
		{
			if (GetTracerMPI()->GetActiveMode())
				cast(PLMPITracer*, GetTracerMPI())->AddSend(i, MASTER_QUIT);
			MPI_Send(NULL, 0, MPI_CHAR, i, MASTER_QUIT, MPI_COMM_WORLD);
		}
	}
}

void PLMPITaskDriver::StartFileServers()
{
	int i;
	PLMPIMsgContext context;
	PLSerializer serializer;

	require(GetProcessId() == 0);

	nFileServerStartNumber++;
	if (nFileServerStartNumber == 1)
	{
		require(not bFileServerOn);
		require(commTask == MPI_COMM_NULL);

		// On active les serveurs de fichier seulement si il y a plus d'un host
		// Ou si bFileServerOnSingleHost est actif ET qu'on a lance khiops avec mpi
		bFileServerOn = (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or
				 (bFileServerOnSingleHost and
				  RMResourceManager::GetResourceSystem()->GetLogicalProcessNumber() > 1));

		// Construction de la liste des serveurs de fichiers
		if (bFileServerOn)
			SelectFileServerRanks();

		// Lancement des serveurs de fichiers
		for (i = 1; i < RMResourceManager::GetLogicalProcessNumber(); i++)
		{
			if (GetTracerMPI()->GetActiveMode())
				cast(PLMPITracer*, GetTracerMPI())->AddSend(i, MASTER_LAUNCH_FILE_SERVERS);
			context.Send(MPI_COMM_WORLD, i, MASTER_LAUNCH_FILE_SERVERS);
			serializer.OpenForWrite(&context);
			serializer.PutIntVector(ivFileServers);
			serializer.Close();
		}

		// Construction du communicateur master/slaves
		MPI_Comm_split(MPI_COMM_WORLD, 1, GetProcessId(), &commProcesses);
		debug(; int nSize; MPI_Comm_size(commProcesses, &nSize);
		      ensure(nSize == RMResourceManager::GetLogicalProcessNumber() - ivFileServers->GetSize()););

		// Mise en place des drivers pour les acces aux fichiers distants
		if (bFileServerOn)
		{
			InputBufferedFile::GetFileDriverCreator()->SetDriverRemote(new PLBufferedFileDriverRemote);
		}
	}
}

void PLMPITaskDriver::StopFileServers()
{
	int i;
	int nCommSize;

	require(GetProcessId() == 0);

	nFileServerStartNumber--;
	if (nFileServerStartNumber == 0)
	{
		require(not PLParallelTask::IsRunning());

		// Arret des serveurs de fichier
		// On envoie a TOUS les process : esclaves (qui travaillent ou non) et serveurs de fichiers
		// Ca n'est pas un Bcast car la reception est non bloquante (Iprobe)
		// La reception pour les esclaves est effectuee dans PLMPISlaveLauncher::IsFileServerEnd()
		// Pour les serveurs de fichiers dans PLMPIFileServerSlave::Run
		MPI_Comm_size(MPI_COMM_WORLD, &nCommSize);
		for (i = 1; i < nCommSize; i++)
		{
			if (GetTracerMPI()->GetActiveMode())
				cast(PLMPITracer*, GetTracerMPI())->AddSend(i, MASTER_STOP_FILE_SERVERS);
			MPI_Send(NULL, 0, MPI_CHAR, i, MASTER_STOP_FILE_SERVERS, MPI_COMM_WORLD);
		}

		// Nettoyage
		PLMPITaskDriver::GetDriver()->odFileServers.DeleteAll();
		ivFileServers->SetSize(0);

		// Remise en place des drivers : acces aux fichiers uniquement en local
		if (bFileServerOn)
		{
			assert(InputBufferedFile::GetFileDriverCreator()->GetDriverRemote() != NULL);
			delete InputBufferedFile::GetFileDriverCreator()->GetDriverRemote();
			InputBufferedFile::GetFileDriverCreator()->SetDriverRemote(NULL);
		}
		bFileServerOn = false;

		// La deconnexion est bloquante, elle a lieu chez les esclaves et chez les serveurs de fichiers
		MPI_Barrier(*PLMPITaskDriver::GetProcessComm()); // Necessaire pour MPICH
		MPI_Comm_disconnect(&commProcesses);             // DISCONNECT COMM_PROCESS
	}
}

void PLMPITaskDriver::BCastBlock(PLSerializer* serializer, PLMsgContext* context)
{
	PLMPIMsgContext* mpiContext;

	check(serializer);
	require(serializer->bIsOpenForRead or serializer->bIsOpenForWrite);

	mpiContext = cast(PLMPIMsgContext*, context);
	require(mpiContext->GetCommunicator() != MPI_COMM_NULL);
	require(mpiContext->nMsgType == PLMPIMsgContext::BCAST);
	MPI_Bcast(serializer->InternalGetMonoBlockBuffer(), serializer->InternalGetBlockSize(), MPI_CHAR, 0,
		  mpiContext->GetCommunicator());
}

PLTaskDriver* PLMPITaskDriver::Clone() const
{
	return new PLMPITaskDriver;
}

void PLMPITaskDriver::RunSlave(PLParallelTask* task)
{
	PLMPISlave* slave;

	// Creation de l'esclave
	slave = new PLMPISlave(task);

	// Execution
	slave->Run();

	// Nettoyage
	delete slave;
}

boolean PLMPITaskDriver::RunMaster(PLParallelTask* task)
{
	PLMPIMaster* master;
	boolean bOk;

	// Creation du maitre
	master = new PLMPIMaster(task);

	// Execution
	bOk = master->Run();

	// Nettoyage
	delete master;
	return bOk;
}

void PLMPITaskDriver::ErrorHandler(MPI_Comm* comm, int* err, ...)
{
	char error_string[MPI_MAX_ERROR_STRING];
	int length_of_error_string;

	if (*err != MPI_SUCCESS)
	{
		// Affichage de l'erreur
		MPI_Error_string(*err, error_string, &length_of_error_string);
		mpiDriver.AddError(error_string);

		// Arret brutal du programme
		MPI_Abort(*comm, *err);
	}
}

void PLMPITaskDriver::RequestIO(int nRW)
{
	nIoRequestNumber++;

	if (nIoRequestNumber == 1)
	{
		PLMPITaskDriver::GetDriver()->tIORead.Start();
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		{
			if (nRW == 0)
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< Start IO read");
			else
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< Start IO write");
		}
	}
}

void PLMPITaskDriver::ReleaseIO(int nRW)
{
	nIoRequestNumber--;

	if (nIoRequestNumber == 0)
	{
		PLMPITaskDriver::GetDriver()->tIORead.Stop();
		PLMPITaskDriver::GetDriver()->statsIOReadDuration.AddValue(
		    PLMPITaskDriver::GetDriver()->tIORead.GetElapsedTime());
		PLMPITaskDriver::GetDriver()->tIORead.Reset();

		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		{
			if (nRW == 0)
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< End IO read");
			else
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< End IO write");
		}
	}
}

void PLMPITaskDriver::SelectFileServerRanks()
{
	int i;
	int j;
	int nRank;
	IntObject* ioRank;

	// Reinitialisation de la liste des serveurs
	ivFileServers->SetSize(0);

	// Parcours des hosts
	for (i = 0; i < RMResourceManager::GetResourceSystem()->GetHostNumber(); i++)
	{
		// Pour chaque rang du host
		for (j = 0; j < RMResourceManager::GetResourceSystem()->GetHostResourceAt(i)->GetRanks()->GetSize();
		     j++)
		{
			// On prend le premier different de 0 (le maitre)
			nRank = RMResourceManager::GetResourceSystem()->GetHostResourceAt(i)->GetRanks()->GetAt(j);
			if (nRank != 0)
			{
				ivFileServers->Add(nRank);

				// Construction du dictionnaire host / rang du serveur
				ioRank = new IntObject;
				ioRank->SetInt(nRank);
				odFileServers.SetAt(
				    RMResourceManager::GetResourceSystem()->GetHostResourceAt(i)->GetHostName(),
				    ioRank);
				break;
			}
		}
	}
	ivFileServers->Sort();
	ensure(ivFileServers->GetSize() == RMResourceManager::GetResourceSystem()->GetHostNumber());
}

PLIncrementalStats* PLMPITaskDriver::GetIOReadDurationStats()
{
	return &statsIOReadDuration;
}

PLIncrementalStats* PLMPITaskDriver::GetIORemoteReadStats()
{
	return &statsIORemoteReadDuration;
}

void PLMPITaskDriver::SendSharedObject(PLSharedObject* shared_object, PLMPIMsgContext* context)
{
	PLSerializer serializer;
	boolean bIsDeclared;

	require(context->nMsgType == PLMPIMsgContext::SEND);
	require(context->GetCommunicator() != MPI_COMM_NULL);

	// Copie du boolean avant modification
	bIsDeclared = shared_object->bIsDeclared;

	// Serialisation de l'objet
	shared_object->bIsDeclared = true;
	serializer.OpenForWrite(context);
	shared_object->Serialize(&serializer);
	serializer.Close();

	// Remise en etat initial
	shared_object->bIsDeclared = bIsDeclared;
}

int PLMPITaskDriver::RecvSharedObject(PLSharedObject* shared_object, PLMPIMsgContext* context)
{
	PLSerializer serializer;

	require(context->nMsgType == PLMPIMsgContext::RECV);
	require(context->GetCommunicator() != MPI_COMM_NULL);

	// Deserialisation
	serializer.OpenForRead(context);
	shared_object->Deserialize(&serializer);
	serializer.Close();
	return context->nRank;
}

void PLMPITaskDriver::InitializeResourceSystem()
{
	ALString sHostName;
	StringVector svHosts;
	LongintVector lvMemory;
	LongintVector lvDiskSpace;
	IntVector ivProcNumberOnHost;
	int i;
	int nMyRank;
	int nRank;
	ALString sTmp;
	ObjectDictionary odHostResources;
	POSITION position;
	ALString sKey;
	Object* oElement;
	PLShared_HostResource shared_hostResource;
	PLShared_ObjectArray* oaSharedResource;
	int nMessageNumber;
	MPI_Comm privateCommunicator;
	int nProcessNumber;
	longint lUsedPhysicalMemory;
	PLSerializer serializer;
	longint lSlaveHeap;
	PLMPIMsgContext context;

	oaSharedResource = new PLShared_ObjectArray(new PLShared_HostResource);
	oaSharedResource->bIsDeclared = true;

	// Creation d'un communicateur prive valable uniquement dans cette methode
	MPI_Comm_dup(MPI_COMM_WORLD, &privateCommunicator);
	MPI_Comm_size(privateCommunicator, &nProcessNumber);

	// Rang du process dans le communicateur MPI
	MPI_Comm_rank(privateCommunicator, &nMyRank);

	// Dimensionnement des ressources
	svHosts.SetSize(nProcessNumber);
	lvMemory.SetSize(nProcessNumber);
	lvDiskSpace.SetSize(nProcessNumber);
	ivProcNumberOnHost.SetSize(nProcessNumber);

	// Construction du hostName
	sHostName = GetLocalHostName();

	if (nMyRank != 0)
	{
		// Suppression de l'emission des erreurs, notamment pour CreateApplicationTmpDir
		// A ce niveau on ne sait pas les transmettre a l'utilisateur
		DisplayErrorFunction errorFunction = Error::GetDisplayErrorFunction();
		Error::SetDisplayErrorFunction(NULL);

		// Reception du repertoire temporaire (qui a un impact sur le calcul de l'espace disque)
		context.Bcast(MPI_COMM_WORLD);
		serializer.OpenForRead(&context);
		FileService::SetUserTmpDir(serializer.GetString());
		serializer.Close();

		// Creation du repertoire temporaire si necessaire
		FileService::CreateApplicationTmpDir();
		require(FileService::CheckApplicationTmpDir());

		// Initialisation des ressources
		RMResourceManager::GetResourceSystem()->InitializeFromLocaleHost();
		RMResourceManager::GetResourceSystem()->SetHostNameAt(0, sHostName);

		// Serialisation et envoi au master
		context.Send(privateCommunicator, 0, RESOURCE_HOST);
		shared_hostResource.SetHostResource(
		    RMResourceManager::GetResourceSystem()->GetHostResourceAt(0)->Clone());
		SendSharedObject(&shared_hostResource, &context);
		shared_hostResource.SetHostResource(NULL);

		// Envoi de la memoire actuellement utilisee et du succes ou non de creation du repertoire temporaire
		lUsedPhysicalMemory = RMResourceManager::GetHeapLogicalMemory();
		context.Send(privateCommunicator, 0, RESOURCE_MEMORY);
		serializer.OpenForWrite(&context);
		serializer.PutLongint(lUsedPhysicalMemory);
		serializer.Close();

		// Reception de la liste des hosts
		context.Bcast(privateCommunicator);
		serializer.OpenForRead(&context);
		oaSharedResource->Deserialize(&serializer);
		serializer.Close();
		RMResourceManager::GetResourceSystem()->oaHostResources.DeleteAll();
		RMResourceManager::GetResourceSystem()->oaHostResources.CopyFrom(oaSharedResource->GetObjectArray());

		Error::SetDisplayErrorFunction(errorFunction);
	}
	else
	{
		// Nettoyage prealable des ressources
		RMResourceManager::GetResourceSystem()->Reset();

		// Envoi du repertoire temporaire
		context.Bcast(MPI_COMM_WORLD);
		serializer.OpenForWrite(&context);
		serializer.PutString(FileService::GetUserTmpDir());
		serializer.Close();

		///////////////////////////////////////////////////////////////
		// Reception des ressources de chaque esclave

		// Reception d'un message de chaque esclave
		nMessageNumber = 0;
		lSlaveHeap = 0;
		while (nMessageNumber != nProcessNumber - 1)
		{
			// Reception des ressources
			context.Recv(privateCommunicator, MPI_ANY_SOURCE, RESOURCE_HOST);
			nRank = RecvSharedObject(&shared_hostResource, &context);

			// Reception de la memoire utilisee
			context.Recv(privateCommunicator, nRank, RESOURCE_MEMORY);
			serializer.OpenForRead(&context);
			lUsedPhysicalMemory = serializer.GetLongint();
			serializer.Close();
			assert(lUsedPhysicalMemory != 0LL);
			if (lSlaveHeap < lUsedPhysicalMemory)
				lSlaveHeap = lUsedPhysicalMemory;

			// Stockage des donnees
			lvMemory.SetAt(nRank, shared_hostResource.GetHostResource()->GetPhysicalMemory());
			lvDiskSpace.SetAt(nRank, shared_hostResource.GetHostResource()->GetDiskFreeSpace());
			ivProcNumberOnHost.SetAt(nRank, shared_hostResource.GetHostResource()->GetPhysicalCoreNumber());
			svHosts.SetAt(nRank, shared_hostResource.GetHostResource()->GetHostName());
			nMessageNumber++;
		}

		// Initialisation de la memoire utilisee au lancement du programme pour les esclaves
		RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetMemory()->Set(lSlaveHeap);

		// Ajout du host et de la memoire du master
		lvMemory.SetAt(0, min(MemGetAvailablePhysicalMemory(), MemGetAdressablePhysicalMemory()));
		svHosts.SetAt(0, sHostName);
		lvDiskSpace.SetAt(0, RMResourceManager::GetTmpDirFreeSpace());
		ivProcNumberOnHost.SetAt(0, SystemGetProcessorNumber());
		RMTaskResourceRequirement::GetMasterSystemAtStart()->GetMemory()->Set(
		    RMResourceManager::GetHeapLogicalMemory());

		/////////////////////////////////////////////////////////
		// Mise en forme des donnees

		// Ajout des hosts et des memoires dans le dictionnaire
		for (i = 0; i < nProcessNumber; i++)
		{
			RMHostResource* hr = cast(RMHostResource*, odHostResources.Lookup(svHosts.GetAt(i)));
			if (hr == NULL)
			{
				hr = new RMHostResource;
				hr->SetHostName(svHosts.GetAt(i));
				hr->SetPhysicalMemory(lvMemory.GetAt(i));
				hr->SetDiskFreeSpace(lvDiskSpace.GetAt(i));
				hr->SetPhysicalCoresNumber(ivProcNumberOnHost.GetAt(i));
				hr->AddProcessusRank(i);
				odHostResources.SetAt(svHosts.GetAt(i), hr);
			}
			else
			{
				// On prend les plus petites valeurs recensees pour chaque host
				if (hr->GetPhysicalMemory() > lvMemory.GetAt(i))
					hr->SetPhysicalMemory(lvMemory.GetAt(i));
				if (hr->GetDiskFreeSpace() > lvDiskSpace.GetAt(i))
					hr->SetDiskFreeSpace(lvDiskSpace.GetAt(i));

				hr->AddProcessusRank(i);
			}
		}

		position = odHostResources.GetStartPosition();
		while (position != NULL)
		{
			odHostResources.GetNextAssoc(position, sKey, oElement);
			RMResourceManager::GetResourceSystem()->AddHostResource(cast(RMHostResource*, oElement));
			oaSharedResource->GetObjectArray()->Add(cast(RMHostResource*, oElement));
		}
		odHostResources.RemoveAll();

		// Envoi de la liste des hosts a tous les esclaves
		context.Bcast(privateCommunicator);
		serializer.OpenForWrite(&context);
		oaSharedResource->Serialize(&serializer);
		serializer.Close();
	}
	oaSharedResource->GetObjectArray()->RemoveAll();
	delete oaSharedResource;

	RMResourceManager::GetResourceSystem()->SetInitialized();
	MPI_Comm_free(&privateCommunicator);
}

void PLMPITaskDriver::MasterInitializeResourceSystem()
{
	int i;
	int nRank;
	int nProcessNumber;
	PLSerializer serializer;
	PLMPIMsgContext context;

	// En mode simule, on ne fait pas appel a MPI
	if (PLParallelTask::GetParallelSimulated())
	{
		RMResourceManager::GetResourceSystem()->InitializeFromLocaleHost();
		RMTaskResourceRequirement::GetMasterSystemAtStart()->GetMemory()->Set(
		    RMResourceManager::GetHeapLogicalMemory());
		return;
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &nRank);
	if (nRank != 0)
	{
		assert(false);
		return;
	}

	MPI_Comm_size(MPI_COMM_WORLD, &nProcessNumber);
	for (i = 1; i < nProcessNumber; i++)
	{
		context.Send(MPI_COMM_WORLD, i, MASTER_RESOURCES);
		serializer.OpenForWrite(&context);
		serializer.PutInt(RMResourceConstraints::GetMemoryLimit());
		serializer.PutInt(RMResourceConstraints::GetMemoryLimitPerProc());
		serializer.PutInt(RMResourceConstraints::GetDiskLimit());
		serializer.Close();
	}

	// Initialisation des ressources
	InitializeResourceSystem();
}

void PLMPITaskDriver::CheckVersion()
{
	PLSerializer serializer;
	int nSize;
	int i;
	ALString sSlaveVersion;
	ALString sSlaveHost;
	ALString sMessage;
	boolean bWrongVersion;
	PLMPIMsgContext context;

	require(PLParallelTask::GetVersion() != "");

	if (PLParallelTask::IsMasterProcess())
	{
		MPI_Comm_size(MPI_COMM_WORLD, &nSize);
		sMessage = "Bad version numbers (expected " + PLParallelTask::GetVersion() + ") :";
		bWrongVersion = false;

		// Reception de chaque numero de version + nom du host
		for (i = 1; i < nSize; i++)
		{
			// Deserialisation
			context.Recv(MPI_COMM_WORLD, i, 0);
			serializer.OpenForRead(&context);
			sSlaveHost = serializer.GetString();
			sSlaveVersion = serializer.GetString();
			serializer.Close();

			// Si la version n'est pas bonne, construction du message
			if (sSlaveVersion != PLParallelTask::GetVersion())
			{
				sMessage += "\n\tversion " + sSlaveVersion + " on " + sSlaveHost;
				bWrongVersion = true;
			}
		}

		// Erreur fatale en cas de mauvaise version
		if (bWrongVersion)
			mpiDriver.AddFatalError(sMessage);
	}
	else
	{
		// Serialisation du host et de la version
		context.Send(MPI_COMM_WORLD, 0, 0);
		serializer.OpenForWrite(&context);
		serializer.PutString(GetLocalHostName());
		serializer.PutString(PLParallelTask::GetVersion());
		serializer.Close();
	}
}

const ALString PLMPITaskDriver::GetClassLabel() const
{
	return "MPI driver";
}

void PLMPITaskDriver::Abort() const
{
	int nSize;

	// On n'appelle MPI_ABORT que si il y a plus d'un process
	// Sinon c'est inutile est ca pollue la sortie
	MPI_Comm_size(MPI_COMM_WORLD, &nSize);
	if (nSize > 1)
		MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
}

void PLMPITaskDriver::SetTaskComm(const MPI_Comm& comm)
{
	require(comm != MPI_COMM_NULL);
	commTask = comm;
}

void PLMPITaskDriver::SetProcessComm(const MPI_Comm& comm)
{
	require(comm != MPI_COMM_NULL);
	commProcesses = comm;
}

boolean PLMPITaskDriver::GetFileServerRank(const ALString& sHostName, const Object* errorSender)
{
	IntObject* ioRank;
	boolean bOk;

	// Recherche du rang du serveur de fichier qui est sur le host
	ioRank = cast(IntObject*, GetDriver()->odFileServers.Lookup(sHostName));
	if (ioRank == NULL)
	{
		bOk = false;
		errorSender->AddError(sHostName + " is unreachable");
		assert(false);
	}
	else
	{
		bOk = true;
		nFileServerRank = ioRank->GetInt();
		ensure(nFileServerRank != -1);
	}
	return bOk;
}
