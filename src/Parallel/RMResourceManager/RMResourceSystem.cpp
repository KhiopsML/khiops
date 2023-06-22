// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMResourceSystem.h"
#include "RMResourceManager.h"

int RMResourceSystem::GetHostNumber() const
{
	require(bIsInitialize);

	return oaHostResources.GetSize();
}

const RMHostResource* RMResourceSystem::GetMasterHostResource() const
{
	RMHostResource* resource;
	int i;

	require(bIsInitialize);
	require(GetProcessId() == 0);

	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		resource = cast(RMHostResource*, oaHostResources.GetAt(i));
		if (resource->IsMasterHost())
			return resource;
	}
	assert(false);
	return NULL;
}

boolean RMResourceSystem::IsInitialized() const
{
	return bIsInitialize;
}

void RMResourceSystem::SetInitialized()
{
	bIsInitialize = true;
}

void RMResourceSystem::SetHostNameAt(int ithHost, const ALString& sHostName)
{
	require(ithHost < oaHostResources.GetSize());
	require(sHostName != "");

	cast(RMHostResource*, oaHostResources.GetAt(ithHost))->SetHostName(sHostName);
}

ALString RMResourceSystem::GetHostNameAt(int ith) const
{
	require(ith < oaHostResources.GetSize());
	return cast(RMHostResource*, oaHostResources.GetAt(ith))->GetHostName();
}

void RMResourceSystem::SetHostDiskAt(int ithHost, longint lDiskSpace)
{
	require(ithHost < oaHostResources.GetSize());

	cast(RMHostResource*, oaHostResources.GetAt(ithHost))->SetDiskFreeSpace(lDiskSpace);
}

longint RMResourceSystem::GetHostDiskAt(int ith) const
{
	require(ith < oaHostResources.GetSize());
	return cast(RMHostResource*, oaHostResources.GetAt(ith))->GetDiskFreeSpace();
}

void RMResourceSystem::AddHostResource(RMHostResource* resource)
{
	require(resource != NULL);
	oaHostResources.Add(resource);
}

const RMHostResource* RMResourceSystem::GetHostResourceAt(int ith) const
{
	require(ith < oaHostResources.GetSize());
	return cast(RMHostResource*, oaHostResources.GetAt(ith));
}

int RMResourceSystem::GetPhysicalCoreNumber() const
{
	int i;
	int nProcess;

	require(bIsInitialize);

	// Parcours des ressources
	nProcess = 0;
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		nProcess += cast(RMHostResource*, oaHostResources.GetAt(i))->GetPhysicalCoreNumber();
	}
	return nProcess;
}

int RMResourceSystem::GetHostIndex(int processId) const
{
	int nIndex = 10;
	IntVector* iv;
	int i;
	boolean bFound = false;

	for (nIndex = 0; nIndex < oaHostResources.GetSize(); nIndex++)
	{
		iv = &cast(RMHostResource*, oaHostResources.GetAt(nIndex))->ivRanks;

		for (i = 0; i < iv->GetSize(); i++)
		{
			if (iv->GetAt(i) == processId)
			{
				bFound = true;
				break;
			}
		}
		if (bFound)
			break;
	}
	if (bFound)
	{
		return nIndex;
	}
	else
		return -1;
}

int RMResourceSystem::GetLogicalProcessNumber() const
{
	int i;
	int nProcess;

	require(bIsInitialize);

	// Parcours des ressources
	nProcess = 0;
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		nProcess += cast(RMHostResource*, oaHostResources.GetAt(i))->GetLogicalProcessNumber();
	}
	return nProcess;
}

longint RMResourceSystem::GetPhysicalMemory() const
{
	longint lAvailableMemory;
	int i;

	require(bIsInitialize);

	// Parcours des ressources
	lAvailableMemory = 0;
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		lAvailableMemory += cast(RMHostResource*, oaHostResources.GetAt(i))->GetPhysicalMemory();
	}
	return lAvailableMemory;
}

longint RMResourceSystem::GetDiskFreeSpace() const
{
	int i;
	longint lAvailableDiskSpace;

	require(bIsInitialize);

	// Parcours des ressources
	lAvailableDiskSpace = 0;
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		lAvailableDiskSpace += cast(RMHostResource*, oaHostResources.GetAt(i))->GetDiskFreeSpace();
	}
	return lAvailableDiskSpace;
}

RMResourceSystem::RMResourceSystem()
{
	InitializeFromLocaleHost();
}

RMResourceSystem::~RMResourceSystem()
{
	oaHostResources.DeleteAll();
}

void RMResourceSystem::InitializeFromLocaleHost()
{
	RMHostResource* hostResource;
	RMStandardResourceDriver* currentDriver;

	// Mise en place du driver de ressources standard
	currentDriver = RMResourceManager::GetResourceDriver();
	RMResourceManager::SetResourceDriver(RMStandardResourceDriver::GetDriver());

	// Nettoyage si necessaire
	if (bIsInitialize)
		Reset();

	// Initialisation avec une seule machine mono-processeur : Sequentiel
	hostResource = new RMHostResource;
	hostResource->SetHostName("localhost");
	hostResource->AddProcessusRank(0);
	hostResource->SetPhysicalMemory(min(MemGetAdressablePhysicalMemory(), MemGetAvailablePhysicalMemory()));
	hostResource->SetDiskFreeSpace(RMResourceManager::GetTmpDirFreeSpace());
	hostResource->SetPhysicalCoresNumber(SystemGetProcessorNumber());
	AddHostResource(hostResource);
	bIsInitialize = true;

	// Reaffectation du driver initial
	RMResourceManager::SetResourceDriver(currentDriver);
}

void RMResourceSystem::Reset()
{
	oaHostResources.DeleteAll();
	bIsInitialize = false;
}

void RMResourceSystem::RemoveProcesses(const IntVector* processToRemove)
{
	IntVector ivNewRanks;
	RMHostResource* resource;
	int i, j;
	IntVector removeByRank;
	int nCurrentServer;
	int nRank;

	if (processToRemove->GetSize() == 0)
		return;

	// Creation du tableau des process a supprimer indexe par le rang
	j = 0;
	nCurrentServer = processToRemove->GetAt(j);
	for (i = 0; i < GetLogicalProcessNumber(); i++)
	{
		if (i == nCurrentServer)
		{
			removeByRank.Add(1);
			j++;
			if (j < processToRemove->GetSize())
				nCurrentServer = processToRemove->GetAt(j);
		}
		else
			removeByRank.Add(0);
	}

	// Pour chaque ressource, modification de la liste des rangs
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		ivNewRanks.SetSize(0);
		resource = cast(RMHostResource*, oaHostResources.GetAt(i));

		// On copie tous les rangs sauf si il est dans processToReomve
		for (j = 0; j < resource->ivRanks.GetSize(); j++)
		{
			nRank = resource->ivRanks.GetAt(j);
			if (removeByRank.GetAt(nRank) == 0)
				ivNewRanks.Add(nRank);
		}

		// Remplacement de la liste des rangs par la nouvelle
		resource->ivRanks.CopyFrom(&ivNewRanks);
	}
}

RMResourceSystem* RMResourceSystem::Clone() const
{
	RMResourceSystem* clone;
	int i;

	clone = new RMResourceSystem;
	clone->Reset();
	clone->bIsInitialize = bIsInitialize;

	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		clone->oaHostResources.Add(cast(RMHostResource*, oaHostResources.GetAt(i))->Clone());
	}
	return clone;
}

void RMResourceSystem::Write(ostream& ost) const
{
	int i;

	require(bIsInitialize);

	// Ecriture des ressources globales
	ost << "System resources :" << endl;
	ost << "\t"
	    << "Host number " << GetHostNumber() << endl;
	ost << "\t"
	    << "Physical cores on system " << GetPhysicalCoreNumber() << endl;
	ost << "\t"
	    << "Logical processes on system " << GetLogicalProcessNumber() << endl;
	ost << "\t"
	    << "Available memory on system " << LongintToHumanReadableString(GetPhysicalMemory()) << " (Logical "
	    << LongintToHumanReadableString(RMStandardResourceDriver::PhysicalToLogical(MEMORY, GetPhysicalMemory()))
	    << ")" << endl;
	ost << "\t"
	    << "Available disk space on system " << LongintToHumanReadableString(GetDiskFreeSpace()) << endl;

	// ost << endl <<"\t"<< "Resources per host : " << endl << endl;
	ost << "\t"
	    << "hostname"
	    << "\t"
	    << "MPI ranks"
	    << "\t"
	    << "logical memory"
	    << "\t"
	    << "disk"
	    << "\t"
	    << "cores" << endl;

	// Parcours des ressources
	for (i = 0; i < oaHostResources.GetSize(); i++)
	{
		ost << "\t";
		cast(RMHostResource*, oaHostResources.GetAt(i))->Write(ost);
	}
	//	ost << endl;
}

RMResourceSystem* RMResourceSystem::CreateAdhocCluster()
{
	RMHostResource* host1;
	RMHostResource* host2;
	RMHostResource* host3;
	RMHostResource* host4;
	RMHostResource* host5;
	RMHostResource* host6;
	RMResourceSystem* cluster;

	cluster = new RMResourceSystem;
	cluster->Reset();

	host1 = new RMHostResource;
	host2 = new RMHostResource;
	host3 = new RMHostResource;
	host4 = new RMHostResource;
	host5 = new RMHostResource;
	host6 = new RMHostResource;

	for (int i = 9; i < 15; i++)
		host1->AddProcessusRank(i);

	host2->AddProcessusRank(0);
	for (int i = 2; i < 7; i++)
		host2->AddProcessusRank(i);

	for (int i = 33; i < 39; i++)
		host3->AddProcessusRank(i);

	for (int i = 25; i < 31; i++)
		host4->AddProcessusRank(i);

	for (int i = 17; i < 23; i++)
		host5->AddProcessusRank(i);

	for (int i = 41; i < 47; i++)
		host6->AddProcessusRank(i);

	host1->SetDiskFreeSpace(longint(18.8 * lGB));
	host1->SetPhysicalMemory(longint(39.1 * lGB));
	host1->SetHostName("l-neobi-3");
	host1->SetPhysicalCoresNumber(100);

	host2->SetDiskFreeSpace(longint(18.9 * lGB));
	host2->SetPhysicalMemory(longint(78.6 * lGB));
	host2->SetHostName("l-neobi-8");
	host2->SetPhysicalCoresNumber(100);

	host3->SetDiskFreeSpace(longint(18.8 * lGB));
	host3->SetPhysicalMemory(longint(39.1 * lGB));
	host3->SetHostName("l-neobi-2");
	host3->SetPhysicalCoresNumber(100);

	host4->SetDiskFreeSpace(longint(18.8 * lGB));
	host4->SetPhysicalMemory(longint(78.4 * lGB));
	host4->SetHostName("l-neobi-10");
	host4->SetPhysicalCoresNumber(100);

	host5->SetDiskFreeSpace(longint(26.5 * lGB));
	host5->SetPhysicalMemory(longint(39.2 * lGB));
	host5->SetHostName("l-neobi-1");
	host5->SetPhysicalCoresNumber(100);

	host6->SetDiskFreeSpace(longint(18.9 * lGB));
	host6->SetPhysicalMemory(longint(39.1 * lGB));
	host6->SetHostName("l-neobi-4");
	host6->SetPhysicalCoresNumber(100);

	cluster->AddHostResource(host1);
	cluster->AddHostResource(host2);
	cluster->AddHostResource(host3);
	cluster->AddHostResource(host4);
	cluster->AddHostResource(host5);
	cluster->AddHostResource(host6);

	cluster->SetInitialized();
	return cluster;
}

RMResourceSystem* RMResourceSystem::CreateSyntheticCluster(int nHostNumber, int nProcNumber, longint lPhysicalMemory,
							   longint lDiskFreeSpace, int nSystemConfig)
{
	RMHostResource* hostResource;
	int i, j, nRank;
	ALString sTmp;
	RMResourceSystem* cluster;
	int nCoresNumber;

	require(nHostNumber > 0);
	require(nProcNumber > 0);
	require(lPhysicalMemory >= 0);
	require(lDiskFreeSpace >= 0);

	cluster = new RMResourceSystem;
	cluster->Reset();
	nCoresNumber = 0;

	// Creation de chaque host
	nRank = 0;
	for (i = 0; i < nHostNumber; i++)
	{
		hostResource = new RMHostResource;

		// Ajout des coeurs
		switch (nSystemConfig)
		{
		case 0:
			// Meme nombre de coeurs pour toute sles machines
			for (j = nRank; j < nRank + nProcNumber; j++)
				hostResource->AddProcessusRank(j);
			nRank = j;
			hostResource->SetPhysicalCoresNumber(nProcNumber);
			break;
		case 1:
			// Taille des machines croissante
			nCoresNumber = min(i + 1, nProcNumber);
			for (j = nRank; j < nRank + nCoresNumber; j++)
				hostResource->AddProcessusRank(j);
			nRank = j;
			hostResource->SetPhysicalCoresNumber(nCoresNumber);
			break;
		case 2:
			// Taille des machine decroissante
			nCoresNumber = min(nHostNumber - i, nProcNumber);
			for (j = nRank; j < nRank + nCoresNumber; j++)
				hostResource->AddProcessusRank(j);
			nRank = j;
			hostResource->SetPhysicalCoresNumber(nCoresNumber);
			break;
		default:
			assert(false);
		}

		// Resources disque et memoire
		if (nSystemConfig == 0)
		{
			hostResource->SetDiskFreeSpace(lDiskFreeSpace);
			hostResource->SetPhysicalMemory(lPhysicalMemory);
		}
		else
		{
			hostResource->SetDiskFreeSpace(lDiskFreeSpace * nCoresNumber / nProcNumber);
			hostResource->SetPhysicalMemory(lPhysicalMemory * nCoresNumber / nProcNumber);
		}

		// Nom de host
		hostResource->SetHostName(sTmp + "host_" + IntToString(i));

		// Ajout au cluster
		cluster->AddHostResource(hostResource);
	}
	cluster->SetInitialized();
	return cluster;
}

//////////////////////////////////////////////////////
// Classe RMHostResource

RMHostResource::~RMHostResource() {}

const ALString& RMHostResource::GetHostName() const
{
	return sHostName;
}

longint RMHostResource::GetPhysicalMemory() const
{
	return GetResourceFree(MEMORY);
}

longint RMHostResource::GetDiskFreeSpace() const
{
	return GetResourceFree(DISK);
}

void RMHostResource::SetHostName(const ALString& sValue)
{
	require(not sValue.IsEmpty());
	sHostName = sValue;
}

void RMHostResource::SetResourceFree(int nResourceType, longint nValue)
{
	require(nResourceType < RESOURCES_NUMBER);
	require(nValue >= 0);
	lvResources.SetAt(nResourceType, nValue);
}

longint RMHostResource::GetResourceFree(int nResourceType) const
{
	require(nResourceType < RESOURCES_NUMBER);
	return lvResources.GetAt(nResourceType);
}

void RMHostResource::SetPhysicalMemory(longint lMemory)
{
	SetResourceFree(MEMORY, lMemory);
}

void RMHostResource::SetDiskFreeSpace(longint lSpace)
{
	SetResourceFree(DISK, lSpace);
}

int RMHostResource::GetLogicalProcessNumber() const
{
	return ivRanks.GetSize();
}

int RMHostResource::GetPhysicalCoreNumber() const
{
	return nPhysicalCoresNumber;
}

const IntVector* RMHostResource::GetRanks() const
{
	return &ivRanks;
}

RMHostResource::RMHostResource()
{
	sHostName = "";
	lvResources.SetSize(RESOURCES_NUMBER);
	lvResources.Initialize();
	nPhysicalCoresNumber = 0;
}

RMHostResource* RMHostResource::Clone() const
{
	RMHostResource* clone;
	clone = new RMHostResource;
	clone->sHostName = sHostName;
	clone->lvResources.CopyFrom(&lvResources);
	clone->nPhysicalCoresNumber = nPhysicalCoresNumber;
	clone->ivRanks.CopyFrom(&ivRanks);
	return clone;
}

void RMHostResource::AddProcessusRank(int nRank)
{
	// On verifie que le rang n'est pas deja present
	debug(; int i; for (i = 0; i < ivRanks.GetSize(); i++) {
		if (ivRanks.GetAt(i) == nRank)
		{
			assert(false);
		}
	});

	ivRanks.Add(nRank);
}

void RMHostResource::SetPhysicalCoresNumber(int nCoresNumber)
{
	require(nCoresNumber > 0);
	nPhysicalCoresNumber = nCoresNumber;
}

boolean RMHostResource::IsMasterHost() const
{
	int i;
	for (i = 0; i < ivRanks.GetSize(); i++)
	{
		if (ivRanks.GetAt(i) == 0)
		{
			return true;
		}
	}
	return false;
}

void RMHostResource::Write(ostream& ost) const
{
	int i;

	// HostName
	ost << GetHostName() << "\t";

	// Ranks
	for (i = 0; i < GetRanks()->GetSize(); i++)
	{
		if (i == 8)
		{
			// On n'affiche que les 8 premiers
			ost << "...\t";
			break;
		}
		ost << GetRanks()->GetAt(i);
		if (i != GetRanks()->GetSize() - 1)
			ost << ",";
		else
			ost << "\t";
	}
	// memory
	ost << LongintToHumanReadableString(RMStandardResourceDriver::PhysicalToLogical(MEMORY, GetPhysicalMemory()))
	    << "\t";

	// Disk
	ost << LongintToHumanReadableString(GetDiskFreeSpace()) << "\t";

	// Coeurs
	ost << GetPhysicalCoreNumber() << endl;
}