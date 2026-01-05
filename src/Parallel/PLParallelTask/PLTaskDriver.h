// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "PLTracer.h"
#include "OutputBufferedFile.h"
#include "InputBufferedFile.h"
#include "RMResourceManager.h"
#include "PLFileConcatenater.h"
#include "PLIncrementalStats.h"

class PLParallelTask;
class PLMaster;

///////////////////////////////////////////////////////////////////////////////
// Classe PLTaskDriver
// Driver sequentiel des taches paralleles, centralisation de differents services
// Ces services devront etre surcharges pour les taches paralleles
class PLTaskDriver : public Object
{
public:
	// Constructeur
	PLTaskDriver();
	~PLTaskDriver();

	// Acces au driver statique
	static PLTaskDriver* GetDriver();

	// Duplication
	virtual PLTaskDriver* Clone() const;

	// Nom du driver
	virtual const ALString GetTechnology() const;

	// Nom de la technologie sequentielle
	static ALString GetSequentialTechnology();

	// Nom de la techno MPI
	static ALString GetMpiTechnology();

	// Est-ce qu'on peut executer le programme en parallele
	// c'est le cas si le driver est parallele et si il ya plus de 2 processeurs disponibles
	virtual boolean IsParallelModeAvailable() const;

	// Creation et lancement du maitre et de l'esclave
	virtual void RunSlave(PLParallelTask* task);
	virtual boolean RunMaster(PLParallelTask* task);

	// Initialisation paralleles des ressources systeme
	virtual void InitializeResourceSystem();
	virtual void MasterInitializeResourceSystem();

	// Verification des ressources systeme disponibles : envoi de messages d'erreur si probleme
	virtual void CheckResourceSystem();

	// Acces au service de trace et creation si ecessaire
	PLTracer* GetTracerMPI();
	PLTracer* GetTracerPerformance();
	PLTracer* GetTracerProtocol();

	// Lancement de l'esclave et instanciation des taches paralleles a la demande du maitre
	virtual void StartSlave();

	// Ordre d'arret a tous les processus MPI
	virtual void StopSlaves();

	// Instanciation des esclaves en FileServers
	virtual void StartFileServers();
	virtual void StopFileServers();

	// Arret brutal du programme
	virtual void Abort() const;

	virtual void SendBlock(PLSerializer* serializer, PLMsgContext*);
	virtual void BCastBlock(PLSerializer* serializer, PLMsgContext*);
	virtual void RecvBlock(PLSerializer* serializer, PLMsgContext*);

	// Retourne la liste des serveurs de fichiers
	const IntVector* GetFileServers() const;

	// Retourne True si c'est un processus de rang nRank dans MPI_COMM_WORLD est "serveur de fichiers"
	boolean IsFileServer(int nRank) const;

	// Est-ce que les serveusr de fichiers on ete lances
	static boolean IsFileServersLaunched();

	// Acces aux statistiques d'acces IO
	// Temps d'acces local en lecture et temps d'acces distant en lecture (=acces local + envoi)
	PLIncrementalStats* GetIOReadingStats();
	PLIncrementalStats* GetIORemoteReadingStats();

	// Permet de forcer la lancement d'un serveur de fichier si le systeme ne contient qu'une machine
	// Utile pour les tests
	// TODO acces a cette methode depuis l'IHM pour pouvoir lancer LearningTest avec cette
	static void SetFileServerOnSingleHost(boolean);
	static boolean GetFileServerOnSingleHost();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tracers
	PLTracer* tracerMPI;
	PLTracer* tracerPerformance;
	PLTracer* tracerProtocol;

	// Creation du tracer
	virtual PLTracer* CreateTracer() const;

	// Instance statique de la classe
	static PLTaskDriver driver;

	// Liste des serveurs de fichiers
	static IntVector* ivFileServers;

	// Vrai si les serveurs de fichiers ont ete lances
	static boolean bFileServerOn;
	static int nFileServerStartNumber;

	// Lancer un serveur de fichier si il y a une seule machine (phase de test)
	static boolean bFileServerOnSingleHost;

	// Temps d'acces IO
	Timer tIORead;
	PLIncrementalStats statsIOReadDuration;
	PLIncrementalStats statsIORemoteReadDuration;

	friend class PLParallelTask;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline PLTaskDriver* PLTaskDriver::GetDriver()
{
	return &driver;
}

inline void PLTaskDriver::StopSlaves() {}
inline void PLTaskDriver::StartSlave() {}
inline void PLTaskDriver::StartFileServers() {}
inline void PLTaskDriver::StopFileServers() {}

inline boolean PLTaskDriver::IsParallelModeAvailable() const
{
	return RMResourceManager::GetLogicalProcessNumber() > 2;
}

inline void PLTaskDriver::Abort() const
{
	assert(false);
	return;
}

inline void PLTaskDriver::SendBlock(PLSerializer* serializer, PLMsgContext*) {}
inline void PLTaskDriver::BCastBlock(PLSerializer* serializer, PLMsgContext*) {}
inline void PLTaskDriver::RecvBlock(PLSerializer* serializer, PLMsgContext*) {}

inline const IntVector* PLTaskDriver::GetFileServers() const
{
	return ivFileServers;
}

inline boolean PLTaskDriver::IsFileServersLaunched()
{
	return bFileServerOn;
}
