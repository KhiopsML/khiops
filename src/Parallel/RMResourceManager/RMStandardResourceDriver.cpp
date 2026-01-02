// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMStandardResourceDriver.h"
#include "RMResourceManager.h"

RMStandardResourceDriver* RMStandardResourceDriver::resourceDriver = NULL;
RMStandardResourceDriver RMStandardResourceDriver::singleton;

longint RMStandardResourceDriver::GetRemainingAvailableMemory() const
{
	// Memoire disponible en ne depassant pas les contraintes utilisateur et en gardant une  reserve pour le systeme
	return max(0LL, GetTotalAvailableMemory() - RMResourceManager::GetSystemMemoryReserve() -
			    RMResourceManager::GetHeapLogicalMemory());
}

longint RMStandardResourceDriver::GetTmpDirFreeSpace() const
{
	longint lFreeMemory;
	lFreeMemory = FileService::GetDiskFreeSpace(FileService::GetTmpDir());

	// Bornee par la limite par proc et la limite sur le systeme (en mono-machine mono-proc c'est la meme chose)
	lFreeMemory = min(lFreeMemory, lMB * RMResourceConstraints::GetDiskLimit());
	return lFreeMemory;
}

longint RMStandardResourceDriver::GetTotalAvailableMemory() const
{
	longint lUserMemory;

	require(GetProcessId() == 0);

	// On part de la memoire disponible
	lUserMemory = min(MemGetAvailablePhysicalMemory(), MemGetAdressablePhysicalMemory());

	// Bornee par les contraintes utilisateur: limite sur la machine (en mono-machine mono-proc c'est la meme chose)
	lUserMemory = min(lUserMemory, lMB * RMResourceConstraints::GetMemoryLimit());

	// Prise en compte de l'overhead d'allocation memoire
	lUserMemory = PhysicalToLogical(MEMORY, lUserMemory);

	return lUserMemory;
}

longint RMStandardResourceDriver::PhysicalToLogical(int nResourceType, longint lPhysicalResource)
{
	longint lLogicalMemory;
	double dOverhead;

	require(nResourceType < RESOURCES_NUMBER);

	// Affectation de l'overhead selon le tuype de ressource
	switch (nResourceType)
	{
	case MEMORY:
		dOverhead = MemGetAllocatorOverhead();
		break;
	case DISK:
		dOverhead = 0;
		break;
	default:
		dOverhead = 0;
		assert(false);
	}

	// Ajout de l'overhead a la ressource physique
	lLogicalMemory = (longint)ceil(lPhysicalResource / (1 + dOverhead));
	return lLogicalMemory;
}

RMStandardResourceDriver::RMStandardResourceDriver() {}

RMStandardResourceDriver::~RMStandardResourceDriver()
{
	// Le destructeur du singleton (statique) sera appele en fin de programme,
	// ce qui forcera la destructiondu driver s'il a etet cree
	if (resourceDriver != NULL)
	{
		RMStandardResourceDriver* driverToDelete = resourceDriver;
		resourceDriver = NULL; // Pour eviter de le detruire plusieurs fois
		delete driverToDelete;
	}
}
