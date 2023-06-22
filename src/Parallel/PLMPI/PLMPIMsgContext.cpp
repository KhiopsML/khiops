// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPIMsgContext.h"

PLMPIMsgContext::PLMPIMsgContext()
{
	nRank = -1;
	communicator = MPI_COMM_NULL;
	nTag = -1;
}
PLMPIMsgContext::~PLMPIMsgContext() {}

void PLMPIMsgContext::Write(ostream& ost) const
{
	switch (nMsgType)
	{
	case 0:
		ost << "SEND TO " << nRank << " with TAG " << nTag;
		break;
	case 1:
		ost << "RECV FROM " << nRank << " with TAG " << nTag;
		break;
	case 2:
		ost << "BCAST";
		break;
	case 3:
		ost << "RSEND TO " << nRank << " with TAG " << nTag;
		break;
	default:
		ost << "ERROR";
	}
}

void PLMPIMsgContext::Send(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0);

	nMsgType = SEND;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}

void PLMPIMsgContext::Rsend(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0);

	nMsgType = RSEND;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}

void PLMPIMsgContext::Recv(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0 or nRankValue == MPI_ANY_SOURCE);

	nMsgType = RECV;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}
void PLMPIMsgContext::Bcast(const MPI_Comm& comm)
{
	assert(comm != MPI_COMM_NULL);

	nMsgType = BCAST;
	communicator = comm;
}

MPI_Comm PLMPIMsgContext::GetCommunicator() const
{
	return communicator;
}

int PLMPIMsgContext::GetRank() const
{
	return nRank;
}

int PLMPIMsgContext::GetTag() const
{
	return nTag;
}