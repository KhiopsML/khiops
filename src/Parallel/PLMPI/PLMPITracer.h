// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLTracer.h"
#include "PLMPIMasterSlaveTags.h"

///////////////////////////////////////////////////////////////////////////////
// Classe PLMPITracer
// Traces  dediees aux messages MPI
//
class PLMPITracer : public PLTracer
{
public:
	// Constructeur
	PLMPITracer();
	~PLMPITracer();

	// ajoute un message relatif a un Send
	void AddSend(int nReceiverRank, int tag);

	// Ajoute un message relatif a un broadcast
	void AddBroadcast(int tag);

	// Ajoute un message relatif a un Recv
	void AddRecv(int source, int tag);
};
