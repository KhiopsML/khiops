// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPIMsgContext.h"

PLMPIMsgContext::PLMPIMsgContext()
{
	nRank = MPI_ANY_SOURCE;
	communicator = MPI_COMM_NULL;
	nTag = MPI_ANY_TAG;
}
PLMPIMsgContext::~PLMPIMsgContext() {}

void PLMPIMsgContext::Write(ostream& ost) const
{
	switch (nMsgType)
	{
	case MSGTYPE::SEND:
		ost << "SEND TO " << nRank << " with TAG " << nTag;
		break;
	case MSGTYPE::RECV:
		ost << "RECV FROM " << nRank << " with TAG " << nTag;
		break;
	case MSGTYPE::BCAST:
		ost << "BCAST";
		break;
	case MSGTYPE::RSEND:
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

	nMsgType = MSGTYPE::SEND;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}

void PLMPIMsgContext::Rsend(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0);

	nMsgType = MSGTYPE::RSEND;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}

void PLMPIMsgContext::Isend(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0);

	nMsgType = MSGTYPE::ISEND;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}

void PLMPIMsgContext::Recv(const MPI_Comm& comm, int nRankValue, int nTagValue)
{
	assert(comm != MPI_COMM_NULL);
	assert(nRankValue >= 0 or nRankValue == MPI_ANY_SOURCE);

	nMsgType = MSGTYPE::RECV;
	communicator = comm;
	this->nTag = nTagValue;
	this->nRank = nRankValue;
}
void PLMPIMsgContext::Bcast(const MPI_Comm& comm)
{
	assert(comm != MPI_COMM_NULL);

	nMsgType = MSGTYPE::BCAST;
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
