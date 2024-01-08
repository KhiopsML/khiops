// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLTaskDriver.h"

IntVector* PLTaskDriver::ivFileServers = NULL;
PLTaskDriver PLTaskDriver::driver;
boolean PLTaskDriver::bFileServerOn = false;
int PLTaskDriver::nFileServerStartNumber = 0;
boolean PLTaskDriver::bFileServerOnSingleHost;

PLTaskDriver::PLTaskDriver()
{
	tracerMPI = NULL;
	tracerPerformance = NULL;
	tracerProtocol = NULL;
	if (ivFileServers == NULL)
		ivFileServers = new IntVector;
}

PLTaskDriver::~PLTaskDriver()
{
	if (ivFileServers != NULL)
	{
		delete ivFileServers;
		ivFileServers = NULL;
	}

	if (tracerMPI != NULL)
		delete tracerMPI;

	if (tracerPerformance != NULL)
		delete tracerPerformance;

	if (tracerProtocol != NULL)
		delete tracerProtocol;
}

PLTaskDriver* PLTaskDriver::Clone() const
{
	return new PLTaskDriver;
}

const ALString PLTaskDriver::GetTechnology() const
{
	return GetSequentialTechnology();
}

ALString PLTaskDriver::GetSequentialTechnology()
{
	return "SEQUENTIAL_DRIVER";
}

ALString PLTaskDriver::GetMpiTechnology()
{
	return "MPI_DRIVER";
}

void PLTaskDriver::RunSlave(PLParallelTask* task)
{
	assert(false);
}

boolean PLTaskDriver::RunMaster(PLParallelTask* task)
{
	assert(false);
	return true;
}

PLTracer* PLTaskDriver::GetTracerMPI()
{
	if (tracerMPI == NULL)
		tracerMPI = CreateTracer();
	return tracerMPI;
}

PLTracer* PLTaskDriver::GetTracerPerformance()
{
	if (tracerPerformance == NULL)
		tracerPerformance = CreateTracer();
	return tracerPerformance;
}

PLTracer* PLTaskDriver::GetTracerProtocol()
{
	if (tracerProtocol == NULL)
		tracerProtocol = CreateTracer();
	return tracerProtocol;
}

PLTracer* PLTaskDriver::CreateTracer() const
{
	return new PLTracer;
}

void PLTaskDriver::InitializeResourceSystem()
{
	RMResourceManager::GetResourceSystem()->InitializeFromLocaleHost();
	RMTaskResourceRequirement::GetMasterSystemAtStart()->GetMemory()->Set(
	    RMResourceManager::GetHeapLogicalMemory());
}

void PLTaskDriver::MasterInitializeResourceSystem()
{
	InitializeResourceSystem();
}

boolean PLTaskDriver::IsFileServer(int nRank) const
{
	int i;

	for (i = 0; i < ivFileServers->GetSize(); i++)
	{
		if (nRank == ivFileServers->GetAt(i))
			return true;
	}
	return false;
}

PLIncrementalStats* PLTaskDriver::GetIOReadingStats()
{
	return &statsIOReadDuration;
}

PLIncrementalStats* PLTaskDriver::GetIORemoteReadingStats()
{
	return &statsIORemoteReadDuration;
}

void PLTaskDriver::SetFileServerOnSingleHost(boolean bValue)
{
	PLRemoteFileService::SetRemoteIsNeverLocal(true);
	bFileServerOnSingleHost = bValue;
}

boolean PLTaskDriver::GetFileServerOnSingleHost()
{
	return bFileServerOnSingleHost;
}
