// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLMPISlaveProgressionManager.h"
#include "PLParallelTask.h"
#include "RMResourceSystem.h"
#include "PLMPITracer.h"
#include "PLMPITaskDriver.h"
#include "PLMPIMessageManager.h"
#include "PLShared_TaskResourceGrant.h"

///////////////////////////////////////////////////////////////////////////////
// Classe PLMPISlave
// Implementation avec MPI d'un esclave.
// Cette classe va de pair avec la classe PLMPIMaster et le driver PLMPITaskDriver
//
class PLMPISlave : public Object
{
public:
	// Constructeur
	PLMPISlave(PLParallelTask*);
	~PLMPISlave();

	// Execution du programme esclave
	void Run();

	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////////////
	// Methodes techniques privees utilisees pour les arret anormaux
	// comme les assertion et les erreurs fatales

	// Notifie qu'on souhaite s'arreter car on a rencontrer une erreur
	// Apres reception de ce message, la PLMPIMaster va donner l'ordre a tout le monde de s'arreter
	void NotifyAbnormalExit();

	// Notifie qu'on rencontre une erreur fatal
	static void NotifyFatalError(Error* error);

	// Sortie brutale de tous les processus
	// Saut si une erreur fatale a ete emise auparavant
	static void SlaveAbort(int nExitCode);

	// Acces aux tracer
	static PLMPITracer* GetTracerMPI();
	static PLMPITracer* GetTracerPerformance();
	static PLMPITracer* GetTracerProtocol();

	// Acces a la tache
	PLParallelTask* GetTask() const;

protected:
	// Reception des tous les messages (sans les traiter) emis par le maitre
	void DischargePendigCommunication();

	// Tache a laquelle est lie l'esclave
	PLParallelTask* task;

	// Pour s'assurer qu'une seule instance est construite
	static boolean bIsInstanciated;

	// Test la presence de nouveaux messages
	boolean CheckNewMessage(int source, MPI_Comm com, MPI_Status& status) const;

	// Acces au driver MPI
	const PLMPITaskDriver* GetDriver() const;

private:
	// Traitement principal
	// Boucle de traitement :
	//  - execution de SlaveProcess sur ordre du maitre
	//	- envoi des resultats du SlaveProcess
	// Sortie de la boucle sur reception de l'ordre d'arret du maitre (c'est le seul moyen de sortir de cette
	// methode) Les resultats envoyes au maitre contiennent un booleen (output_bSlaveProcessOk), en cas d'erreur
	// sans le SlaveProcess, celui-ci est mis a False et lors de sa reception le maitre donne l'ordre a tous les
	// esclaves de s'arreter Renvoie true si tout s'est bien passe dans l'ensemble du traitement (maitre + ensemble
	// des esclaves)
	boolean Process();

	// Notifie qu'on a fini (apres finalize)
	void NotifyDone(boolean bOk);

	// Cette methode permet de rendre les MPI_Recv non bloquants pour les demande d'interruption.
	// Attend tant qu'un message venant du maitre avec le tag passe en parametre n'arrive pas.
	// Le message n'est pas recu, il convient de recevoir le message via MPI_Recv
	// Si une demande d'arret est effective pendant l'attente, la fonction retourne (meme si le message attendu n'a
	// pas ete emis) et bInterruptionRequested est a true Retourne la taille du message
	int WaitForMessage(int nTag, boolean& bInterruptionRequested) const;

	// Envoi des resultats au Master avec serialisation des resultats
	void SendResults();

	// Implementation du display de la classe Error
	// Stockage des messages si le max des messages n'a pas ete atteind (via fenetre RMA)
	// L'envoi des messages est efefctue a l'issue du slaveProcess
	static void DisplayError(const Error* e);

	// Indique si le nombre d'erreur, warning et message est atteint, et qu'aucun
	// nouveau message ne peut plus etre affiche
	static boolean IsMaxErrorFlowReachedPerGravity(int nErrorGravity);

	static void DeleteMessagesWithGravity(int nGravity);

	// Envoie les messages utilisateur au maitre
	// en specifiant la methode dans laquelle ils ont ete emis
	// nMethode=0,1 ou 2 respectivement pour SlaveIintialize, SlaveProcess et SlaveFinaliaze
	void SendUserMessages(int nMethod);

	// Gestionnaire de progression
	PLMPISlaveProgressionManager* progressionManager;

	// Pour les traces : est-ce que l'esclave a fait quelque chose
	boolean bIsWorking;

	// Temps de traitement de chaque slaveprocess
	Timer tProcessing;

	// Temps avant endormissement
	Timer tTimeBeforeSleep;

	// Erreur fatale en cours
	static boolean bPendingFatalError;

	// Pas de SystemSleep et pas de Probe
	boolean bBoostedMode;

	// Tableau qui pour chaque type d'erreur, indique aux esclaves qu'il est
	// inutile d'envoyer de nouveaux messages i.e. quand IsMaxErrorFlowReachedPerGravity == true
	static IntVector* ivGravityReached;

	// Buffer utilise pour la methode DischargePendingCommunication
	// on sait que la taille des messages ne depasse pas celel d'un bloc
	char sBufferDischarge[MemSegmentByteSize];

	static DisplayErrorFunction currentDisplayErrorFunction;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline PLParallelTask* PLMPISlave::GetTask() const
{
	return task;
}

inline PLMPITracer* PLMPISlave::GetTracerMPI()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerMPI());
}

inline PLMPITracer* PLMPISlave::GetTracerPerformance()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerPerformance());
}

inline PLMPITracer* PLMPISlave::GetTracerProtocol()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerProtocol());
}

inline boolean PLMPISlave::CheckNewMessage(int source, MPI_Comm com, MPI_Status& status) const
{
	int nNewMessage;

	nNewMessage = 0;
	MPI_Iprobe(source, MPI_ANY_TAG, com, &nNewMessage, &status);
	return nNewMessage;
}

inline const PLMPITaskDriver* PLMPISlave::GetDriver() const
{
	return cast(PLMPITaskDriver*, GetTask()->GetDriver());
}
