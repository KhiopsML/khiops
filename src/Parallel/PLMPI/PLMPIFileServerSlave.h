// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLMPImpi_wrapper.h"
#include "PLMPIMasterSlaveTags.h"
#include "SystemResource.h"
#include "Object.h"
#include "PLMPITaskDriver.h"
#include "PLParallelTask.h"
#include "PLErrorWithIndex.h"
#include "InputBufferedFile.h"
#include "HugeBuffer.h"

////////////////////////////////////////////////////////////////////////////
// Classe PLMPIFileServerSlave
// Cette classe technique implemente un esclave serveur de fichier
// C'est un singleton
class PLMPIFileServerSlave : public Object
{
public:
	// constructeur
	PLMPIFileServerSlave();
	~PLMPIFileServerSlave();

	// Boucle de traitement
	// Sortie si ordre du maitre
	void Run(boolean& bOrderToQuit);

protected:
	// lit et envoi un buffer
	void Fread(int nRank) const;

	// Envoie de la taille du fichier vers l'esclave de rang nRank
	void GetFileSize(int nRank) const;

	// Envoie true si le fichier existe
	void GetFileExist(int nRank) const;

	// Envoie true si le repertoir existe
	void GetDirExist(int nRank) const;

	// Suppression du fichier
	void RemoveFile(int nrank) const;

	// DisplayErrorFunction : Stockage des erreurs utilisateurs (warnings)
	// MPI_Abort si erreur fatale
	static void CatchError(const Error* e);

	// Serialisation et suppression des erreurs rencontrees
	void SerializeErrors(PLSerializer*) const;

	// Ajout d'une trace pour la perfoirmance, si le tracer est actif
	void AddPerformanceTrace(const ALString&) const;

	// Acces au tracer MPI
	PLMPITracer* GetTracerMPI() const;

	// Stockage des erreurs
	static ObjectArray* oaUserMessages;

	// Temps avant endormissement
	Timer tTimeBeforeSleep;

	ALString sTaskName; // // pour le fichier de log
};

inline PLMPITracer* PLMPIFileServerSlave::GetTracerMPI() const
{
	return cast(PLMPITracer*, PLMPITaskDriver::GetDriver()->GetTracerMPI());
}

inline void PLMPIFileServerSlave::AddPerformanceTrace(const ALString& sTrace) const
{
	if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(sTrace);
}