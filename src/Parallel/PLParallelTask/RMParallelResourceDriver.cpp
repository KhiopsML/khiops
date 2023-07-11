// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMParallelResourceDriver.h"

RMTaskResourceGrant* RMParallelResourceDriver::grantedResources = NULL;
RMParallelResourceDriver* RMParallelResourceDriver::parallelResourceDriver = NULL;
RMParallelResourceDriver RMParallelResourceDriver::singleton;

longint RMParallelResourceDriver::GetRemainingAvailableMemory() const
{
	longint lRemainingMemory;

	require(grantedResources != NULL);

	// Si on est dans une tache parallele
	if (PLParallelTask::IsRunning())
	{
		lRemainingMemory = GetRemainingAvailableResource(MEMORY, RMResourceManager::GetHeapLogicalMemory());
	}
	else
	{
		require(GetProcessId() == 0);
		lRemainingMemory = RMStandardResourceDriver::GetRemainingAvailableMemory();
	}
	return lRemainingMemory;
}

longint RMParallelResourceDriver::GetTotalAvailableMemory() const
{
	longint lAvailableMemory;
	require(grantedResources != NULL);

	// Si on est dans une tache parallele
	if (PLParallelTask::IsRunning())
	{

		if (PLParallelTask::IsMasterProcess())
			// Memoire destinee au maitre
			lAvailableMemory = grantedResources->GetMasterResource(MEMORY);
		else
			// Memoire destinee a l'esclave
			lAvailableMemory = grantedResources->GetSlaveResource(MEMORY);
	}
	else
	{
		ensure(GetProcessId() == 0);
		lAvailableMemory = RMStandardResourceDriver::GetTotalAvailableMemory();
	}
	return lAvailableMemory;
}

longint RMParallelResourceDriver::GetTmpDirFreeSpace() const
{
	longint lRemainingDisk;

	require(grantedResources != NULL);

	// Si on est dans une tache parallele
	if (PLParallelTask::IsRunning())
	{
		lRemainingDisk = GetRemainingAvailableResource(DISK, RMStandardResourceDriver::GetTmpDirFreeSpace());
	}
	else
	{
		ensure(GetProcessId() == 0);
		lRemainingDisk = RMStandardResourceDriver::GetTmpDirFreeSpace();
	}
	return lRemainingDisk;
}

longint RMParallelResourceDriver::GetRemainingAvailableResource(Resource resource, longint lCurrentResource) const
{
	longint lRemaininResource;
	longint lMasterResource;
	longint lSlaveResource;
	longint lSharedResource;
	longint lSystemAtStart;
	longint lGrantedResource;

	require(grantedResources != NULL);

	// Exigences du systeme
	lSystemAtStart = RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(resource)->GetMin();

	if (PLParallelTask::GetParallelSimulated())
	{
		lSharedResource = grantedResources->GetSharedResource(resource);
		lMasterResource = grantedResources->GetMasterResource(resource);
		lSlaveResource = grantedResources->GetSlaveResource(resource);

		// Memoire disponible en ne depassant pas  nbSlave * (Mem pour chaque slave + shared) + Mem pour master
		// +shared + system
		lRemaininResource = min(grantedResources->GetSlaveNumber() * lSlaveResource + lMasterResource,
					grantedResources->GetSlaveNumber() * (lSlaveResource + lSharedResource) +
					    lMasterResource + lSharedResource + lSystemAtStart - lCurrentResource);
	}
	else if (grantedResources->IsSequentialTask())
	{
		lMasterResource = grantedResources->GetMasterResource(resource);
		lSlaveResource = grantedResources->GetSlaveResource(resource);
		lSharedResource = grantedResources->GetSharedResource(resource);

		// On ne distingue pas la memoire destinee au maitre de celle destinee a l'esclave
		lRemaininResource =
		    min(lMasterResource + lSlaveResource,
			lMasterResource + lSlaveResource + lSharedResource + lSystemAtStart - lCurrentResource);
	}
	else
	{
		if (PLParallelTask::IsMasterProcess())
			lGrantedResource = grantedResources->GetMasterResource(resource);
		else
			lGrantedResource = grantedResources->GetSlaveResource(resource);
		lSharedResource = grantedResources->GetSharedResource(resource);
		if (GetProcessId() == 0)
		{
			lSystemAtStart =
			    RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(resource)->GetMin();
		}
		else
		{
			lSystemAtStart =
			    RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(resource)->GetMin();
		}

		// Memoire disponible en ne depassant pas la memoire destinee au processus courant + memoire partagee
		lRemaininResource =
		    min(lGrantedResource, lGrantedResource + lSharedResource + lSystemAtStart - lCurrentResource);
	}
	return max(0LL, lRemaininResource);
}

RMParallelResourceDriver::RMParallelResourceDriver() {}

RMParallelResourceDriver::~RMParallelResourceDriver()
{
	// Le destructeur du singleton (statique) sera appele en fin de programme,
	// ce qui forcera la destructiondu driver s'il a etet cree
	if (parallelResourceDriver != NULL)
	{
		RMParallelResourceDriver* driverToDelete = parallelResourceDriver;
		parallelResourceDriver = NULL; // Pour eviter de le detruire plusieurs fois
		delete driverToDelete;
	}
}
