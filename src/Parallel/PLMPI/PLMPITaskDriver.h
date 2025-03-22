// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLMPImpi_wrapper.h"
#include "PLTaskDriver.h"
#include "RMResourceSystem.h"
#include "PLMPITracer.h"
#include "TaskProgression.h"
#include "PLShared_HostResource.h"
#include "PLMPIMasterSlaveTags.h"
#include "PLMPIMsgContext.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PLMPITaskDriver
// Driver parallele des taches paralleles.
// Les services sont implementes en utilsant MPI.
//
class PLMPITaskDriver : public PLTaskDriver
{
public:
	// Constructeur
	PLMPITaskDriver();
	~PLMPITaskDriver();

	// Communicateur qui contient le maitre et les esclaves qui travaillent (sans le serveurs de fichiers et les
	// esclaves qui ne travaillent pas)
	static void SetTaskComm(const MPI_Comm&);
	static MPI_Comm* GetTaskComm();

	// Communicateur qui contient tout le monde sauf les serveurs de fichiers
	static void SetProcessComm(const MPI_Comm&);
	static MPI_Comm* GetProcessComm();

	// Acces au driver statique
	static PLMPITaskDriver* GetDriver();

	// Methodes virtuelles
	PLTaskDriver* Clone() const override;
	const ALString GetTechnology() const override;
	void RunSlave(PLParallelTask* task) override;
	boolean RunMaster(PLParallelTask* task) override;

	// Initialisation paralleles des ressources systeme
	// Doit etre appele simultanement par le maitre et chaque esclave
	void InitializeResourceSystem() override;

	// Initialisation a partir du Master :
	// Le maitre demande aux esclaves d'initialiser les ressources, puis appel de InitializeResourceSystem
	// ne fait rien si appele depuis l'esclave
	void MasterInitializeResourceSystem() override;

	// Verification que tous les processus ont les memes versions
	// Emmet une erreur fatal si ca n'est pas le cas
	// Doit etre appele simultanement par le maitre et chaque esclave
	static void CheckVersion();

	// Lancement des esclaves
	// Suivant les ordres recus, les esclaves peuvent s'instancier en workers (slaves), server de fichier ou sortir
	// Sortie de la fonction lorsque le maitre demande l'arret
	// Utilisable uniquement par les esclaves
	void StartSlave() override;

	// Ordre d'arret a tous les processus MPI
	// appel depuis le maitre
	void StopSlaves() override;

	// Instanciation des esclaves en FileServer
	// Utilisable depuis le maitre
	// Les esclaves doivent etre prealablement lances (methode StartSlave)
	// Doit etre appele avant de lancer une tache car cette methode initialise le communicateur des process
	// (GetProcessComm())
	void StartFileServers() override;
	void StopFileServers() override;

	// Envoi le contenu du serializer par blocs de 64 Ko (suit la structure du CharVector sous-jacent)
	void SendBlock(PLSerializer* serializer, PLMsgContext*) override;
	void BCastBlock(PLSerializer* serializer, PLMsgContext*) override;
	void RecvBlock(PLSerializer* serializer, PLMsgContext*) override;

	// Envoi d'un objet partage // TODO methode tres peu utilisee, remove ??
	static void SendSharedObject(PLSharedObject* shared_object, PLMPIMsgContext* context);

	// Reception d'un objet partage
	// Retourne le rang de l'emetteur // TODO methode tres peu utilisee, remove ??
	static int RecvSharedObject(PLSharedObject* shared_object, PLMPIMsgContext* context);

	const ALString GetClassLabel() const override;

	// Redefinition de la gestion des erreurs MPI
	static void ErrorHandler(MPI_Comm* comm, int* err, ...);

	// Duree des acces IO dans les InputBufferedFile
	PLIncrementalStats* GetIOReadDurationStats();

	// Duree des acces distants dans les InputBufferedFile
	// = temps de la requete + acces local (GetIODuration)
	PLIncrementalStats* GetIORemoteReadStats();

	// Duree en ms avant endormissement
	const static double TIME_BEFORE_SLEEP;

protected:
	// Methodes statiques de demande d'acces au disque
	// acces en lecture nRW=0
	// Acces en ecriture nRW=1
	static void RequestIO(int nRW);
	static void ReleaseIO(int nRW);

	// Selection des process qui vont devenir serveurs de fichiers
	// Le tableau construit est trie
	void SelectFileServerRanks();

	// Communicateur qui contient le maitre et les esclaves qui travaillent (sans le serveurs de fichiers et les
	// esclaves qui ne travaillent pas)
	static MPI_Comm commTask;

	// Communicateur qui contient le maitre et les esclaves qui travaillent (sans le serveurs de fichiers et les
	// esclaves qui ne travaillent pas)
	static MPI_Comm commProcesses;

	virtual PLTracer* CreateTracer() const override;
	virtual void Abort() const override;

	// Taille maximale d'un nom de host (utilisee dans l'initialisation des ressources)
	static const int HOST_NAME_SIZE = 256;

	// Gardes fous pour s'assurer que l'initialisation et la finalisation
	// ne sont appelees qu'une seule fois.
	static boolean bIsInitialized;
	static boolean bIsFinalized;

	// Instance statique utilisee pour envoyer les erreurs dans les methodes statiques
	static PLMPITaskDriver mpiDriver;

	Timer tSend;
	Timer tRecv;

	// Nombre d'acces disques demandes imbriques
	static int nIoRequestNumber;

	/////////////////////////////////////////////////
	// Gestion de l'acces aux fichiers distants

	// Donne le rang du serveur de fichier qui est sur le host passe en parametre.
	// Le rang du serveur est stocke dans l'attribut nFileServerRank
	static boolean GetFileServerRank(const ALString& sHostName, const Object* errorSender);

	// Rang du serveur de fichier sur lequel est le fichier en cours de traitement
	static int nFileServerRank;

	// Liste des serveurs de fichiers du cluster
	ObjectDictionary odFileServers; // host + rank dans MPI_COMM_WORLD

	friend class PLMPISystemFileDriverRemote;
	friend class PLMPIMaster;
	friend class PLMPISlave;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLMPITaskDriver::SendBlock(PLSerializer* serializer, PLMsgContext* context)
{
	PLMPIMsgContext* mpiContext;
	MPI_Request request;

	require(context != NULL);

	mpiContext = cast(PLMPIMsgContext*, context);
	require(mpiContext->nMsgType == MSGTYPE::SEND or mpiContext->nMsgType == MSGTYPE::RSEND or
		mpiContext->nMsgType == MSGTYPE::ISEND);
	check(serializer);
	require(serializer->bIsOpenForWrite);
	require(mpiContext->GetCommunicator() != MPI_COMM_NULL);

	// Le vecteur est plus petit qu'un bloc
	assert(serializer->nBufferPosition <= serializer->InternalGetBlockSize());
	switch (mpiContext->nMsgType)
	{
	case MSGTYPE::SEND:
		MPI_Send(serializer->InternalGetMonoBlockBuffer(), serializer->nBufferPosition, MPI_CHAR,
			 mpiContext->GetRank(), mpiContext->GetTag(), mpiContext->GetCommunicator());
		break;
	case MSGTYPE::RSEND:

		MPI_Rsend(serializer->InternalGetMonoBlockBuffer(), serializer->nBufferPosition, MPI_CHAR,
			  mpiContext->GetRank(), mpiContext->GetTag(), mpiContext->GetCommunicator());
		break;
	case MSGTYPE::ISEND:
		MPI_Isend(serializer->InternalGetMonoBlockBuffer(), serializer->nBufferPosition, MPI_CHAR,
			  mpiContext->GetRank(), mpiContext->GetTag(), mpiContext->GetCommunicator(), &request);
		break;
	default:
		assert(false);
		AddFatalError("context unknown");
	}
}

inline void PLMPITaskDriver::RecvBlock(PLSerializer* serializer, PLMsgContext* context)
{
	PLMPIMsgContext* mpiContext;
	MPI_Status status;

	require(context != NULL);

	mpiContext = cast(PLMPIMsgContext*, context);
	require(mpiContext->GetCommunicator() != MPI_COMM_NULL);
	require(mpiContext->nMsgType == MSGTYPE::RECV);
	require(serializer->bIsOpenForRead);

	if (mpiContext->nMsgType != MSGTYPE::BCAST)
	{
		tRecv.Start();
		// on n'utilise pas MPI_Probe pour avoir la taille d'abord car ca ralenti et on a ABORT pour le
		// MPI_Rsend qui est en face En outre ca ne sert a rien car on connait la taille max qui est celle d'un
		// bloc
		MPI_Recv(serializer->InternalGetMonoBlockBuffer(), serializer->InternalGetBlockSize(), MPI_CHAR,
			 mpiContext->GetRank(), mpiContext->GetTag(), mpiContext->GetCommunicator(), &status);
		tRecv.Stop();

		// Mise a jour du rang et du tag en cas de reception avce ANY_RANK ou ANY_TAG
		mpiContext->nRank = status.MPI_SOURCE;
		mpiContext->nTag = status.MPI_TAG;
	}
}

inline PLMPITaskDriver* PLMPITaskDriver::GetDriver()
{
	return &mpiDriver;
}

inline const ALString PLMPITaskDriver::GetTechnology() const
{
	return GetMpiTechnology();
}

inline PLTracer* PLMPITaskDriver::CreateTracer() const
{
	return new PLMPITracer;
}

inline MPI_Comm* PLMPITaskDriver::GetTaskComm()
{
	return &commTask;
}

inline MPI_Comm* PLMPITaskDriver::GetProcessComm()
{
	return &commProcesses;
}
