// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLMPImpi_wrapper.h"
#include "PLSerializer.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PLMPIMsgContext
// Contexte de la serialization
// Permet de definir l'envoi d'un serializer (rang, communicateur etc...)
//

class PLMPIMsgContext : public PLMsgContext
{
public:
	// Constructeur
	PLMPIMsgContext();
	~PLMPIMsgContext();

	// Affichage
	void Write(ostream& ost) const override;

	// Initialisation du contexte
	// Le mode Isend n'est a utiliser que sur des petits buffers et l'information
	// sur la reception du message n'est pas disponible (pas implemente)
	void Send(const MPI_Comm&, int nRank, int nTag);
	void Rsend(const MPI_Comm&, int nRank, int nTag);
	void Isend(const MPI_Comm&, int nRank, int nTag);
	void Recv(const MPI_Comm&, int nRank, int nTag);
	void Bcast(const MPI_Comm&);

	// Accesseurs
	MPI_Comm GetCommunicator() const;
	int GetRank() const;
	int GetTag() const;

protected:
	int nRank;
	MPI_Comm communicator;
	int nTag;

	friend class PLMPITaskDriver;
};
