// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLMPITracer.h"
#include "PLParallelTask.h"
#include "TaskProgression.h"
#include "PLMPITaskDriver.h"
#include "PLMPIMessageManager.h"
#include "PLSharedObject.h"
#include "PLShared_TaskResourceGrant.h"
#include "PLIncrementalStats.h"
#include "PLSerializer.h"
#include "PLMPITaskDriver.h"

////////////////////////////////////////////////////////////////////////////
// Classe PLMPIMaster.
// Cette classe technique implemente un maitre MPI.
// Celui-ci lance plusieurs esclaves et leur distribue les taches.
// Cette classe contient tous les appels a MPI cote maitre
//
class PLMPIMaster : public Object
{
public:
	// Constructeur,
	PLMPIMaster(PLParallelTask*);
	~PLMPIMaster();

	// Lancement des esclaves
	// Traitement
	boolean Run();

	// Methode appelee en cas d'erreur fatale
	static void MasterFatalError();

	// Acces a la tache
	PLParallelTask* GetTask() const;

	const ALString GetClassLabel() const override;

	// Redefinition des methodes de gestion des erreurs : redirection vers la tache
	void AddSimpleMessage(const ALString& sLabel) const override;
	void AddMessage(const ALString& sLabel) const override;
	void AddWarning(const ALString& sLabel) const override;
	void AddError(const ALString& sLabel) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////
	// Methodes a implementer lors de la construction de nouveaux
	// types de taches dans la bibliotheque

	// Reception et gestion des messages envoyes par les esclaves
	// Renvoie le tag du message recu
	int ReceiveAndProcessMessage(int nTag, int nSource);

	// Reception sans traitement du message correspondant au status passe en parametre..
	// appele dans la methode DischaregePendingCommunication
	// Doit etre modifiee si ReceiveAndProcessMessage est reimplementee
	// pour prendre en compte des nouveaux messages,
	void ReceivePendingMessage(MPI_Status status);

	////////////////////////////////////////////////////////
	// Methodes utilitaires

	// Renvoie true si on n'attend pas de messages, on n'a pas besoin d'etre tres reactif
	// (par exemple, si on vient de donner l'ordre de lecture, on attend la fin de lecture)
	boolean CanSleep() const;

	////////////////////////////////////////////////////////
	// Communication avec l'esclave (ordres)

	// Donne l'orde de travail a l'esclave passe en parametre
	// lui envoie prealablement les taskParameters
	void GiveNewJob(PLSlaveState*, double dTaskPercent);

	// Test la presence de nouveaux messages
	boolean CheckNewMessage(int source, MPI_Comm com, MPI_Status& status, int nTag);

	// Ferme toutes les connections avec les esclaves (bloquant : operation collective)
	// ATTENTION : Les communicateurs MPI ne seront plus valides apres l'appel de cette methode
	// A surcharger pour ajouter les deconnections des classe filles
	void Disconnect();

	// Demande d'arret aux esclave, le booleen en parametre indique aux esclaves
	// si c'est un arret normal (bok=true) ou un arret cause par un echec (bok=false)
	void SendStop(boolean bOk);

	// Modifie les fenetres RMA si le quota de message est depasse
	void UpdateMaxErrorFlow();

	// Acces aux tracers
	PLMPITracer* GetTracerMPI();
	PLMPITracer* GetTracerPerformance();
	PLMPITracer* GetTracerProtocol();

	// Acces au driver MPI
	const PLMPITaskDriver* GetDriver() const;

	// Methode principale appelee apres le spawn
	// methode qui contient toutes les communications
	boolean Process();

	////////////////////////////////////////////////////////
	// Attributs

	// Est-ce qu'on a recu au moins un message
	boolean bNewMessage;

	// Tache a laquelle est ratache le master
	PLParallelTask* task;

	// Est-ce qu'au moins un ordre a ete donne
	// utilise dans le destructeur
	boolean bSpawnedDone;

	// Statistiques sur le nombre d'esclaves qui ont effectivement travaille
	PLIncrementalStats statsWorkingSlave;

	// Nombre d'esclaves qui travaillent actuellement
	int nWorkingSlaves;

private:
	////////////////////////////////////////////////////////
	// Communication avec l'esclave (ordres)

	// Envoi un message non bloquant de demand d'arret a chaque esclave de la tache
	void NotifyInterruptionRequested();

	////////////////////////////////////////////////////////
	// Gestion de la progression des esclaves

	// Remise a 0 de la progression de tous les esclaves
	void ResetSlavesProgression();

	// Calcul de la progression globale en fonction de celle de chaque esclave
	// Le calcul n'est pas le meme suivant la tache realisee par les esclaves :
	// 	- soit on est dans l'initialisation ou la finbalisation
	//	- soit on est dans la boucle des SlaveProcess
	int ComputeGlobalProgression(boolean bSlaveProcess);

	////////////////////////////////////////////////////////
	// Methodes techniques

	// Recoit toutes les communications emises par l'esclave de rang nRank et de tag nTag
	// utilise lors de la reception de fin de traitement pour decharger toutes les progressions
	void DischargePendingCommunication(int nRank, int nTag);

	boolean FindPosOfRank(ObjectList& slavesList, int nValue, POSITION&);

	////////////////////////////////////////////////////////
	// Attributs

	// Liste ordonnee des travailleurs en cours
	// le premier qui a commence a travailler est le premier de la liste
	// utilisee pour
	//					- savoir qui peut travailler
	//					- l'envoi des warnings (seul le premier envoie directement vers
	// l'utilisateur)
	//					- savoir si il y a des esclaves en train de travailler
	ObjectList workers;

	// Pour n'envoyer l'arret qu'une seule fois
	boolean bStopOrderDone;

	// Demande d'arret utilisateur
	boolean bInterruptionRequested;

	// Un esclave a rencontre une erreur
	boolean bSlaveError;

	// Le maitre a rencontre une erreur
	boolean bMasterError;

	boolean bIsMaxErrorReached;
	boolean bIsMaxWarningReached;
	boolean bIsMaxMessageReached;

	// Temps avant endormissement
	Timer tTimeBeforeSleep;

	// Buffer utilise pour la methode DischargePendingCommunication
	// on sait que la taille des messages ne depasse pas celel d'un bloc
	char sBufferDischarge[MemSegmentByteSize];

	// Progression aggregee
	double dGlobalProgression;

	// Pour assertion seulement
	// on verifie que la progression ne decroit pas
	int nOldProgression;

	// true des lors qu'au moins un esclave a commence le SlaveProcess
	boolean bIsProcessing;

	// Nombre d'esclaves qui ont lances SlaveInitialize
	int nInitialisationCount;

	// Nombre d'esclaves qui ont lances SlaveFinalize
	int nFinalisationCount;

	// Rang du premier esclave qui envoie un message pendant les SlaveInitialize
	int nFirstSlaveInitializeMessageRank;
	int nFirstSlaveFinalizeMessageRank;

	PLMPIMessageManager messageManager;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline PLParallelTask* PLMPIMaster::GetTask() const
{
	return task;
}

inline PLMPITracer* PLMPIMaster::GetTracerMPI()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerMPI());
}

inline PLMPITracer* PLMPIMaster::GetTracerPerformance()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerPerformance());
}

inline PLMPITracer* PLMPIMaster::GetTracerProtocol()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerProtocol());
}

inline boolean PLMPIMaster::CanSleep() const
{
	return not bNewMessage;
}

inline boolean PLMPIMaster::CheckNewMessage(int source, MPI_Comm com, MPI_Status& status, int nTag)
{
	bNewMessage = false;
	int nNewMessage;

	MPI_Iprobe(source, nTag, com, &nNewMessage, &status);
	if (nNewMessage == 1)
		bNewMessage = true;
	return bNewMessage;
}

inline const PLMPITaskDriver* PLMPIMaster::GetDriver() const
{
	return cast(PLMPITaskDriver*, GetTask()->GetDriver());
}