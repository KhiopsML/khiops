// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMTaskResourceGrant.h"

///////////////////////////////////////////////////////////////////////
// Implementation de  RMResourceContainer

RMResourceContainer::RMResourceContainer()
{
	lvResources.SetSize(RESOURCES_NUMBER);
}
RMResourceContainer::~RMResourceContainer() {}

void RMResourceContainer::CopyFrom(const RMResourceContainer* container)
{
	lvResources.CopyFrom(&container->lvResources);
}

RMResourceContainer* RMResourceContainer::Clone() const
{
	RMResourceContainer* clone;
	clone = new RMResourceContainer;
	clone->CopyFrom(this);
	return clone;
}

void RMResourceContainer::SetValue(int nResourceType, longint lValue)
{
	require(nResourceType < RESOURCES_NUMBER);
	lvResources.SetAt(nResourceType, lValue);
}

void RMResourceContainer::AddValue(int nResourceType, longint lValue)
{
	longint lValue1;
	require(nResourceType < RESOURCES_NUMBER);
	lValue1 = lvResources.GetAt(nResourceType);
	if (lValue1 > LLONG_MAX - lValue)
		lvResources.SetAt(nResourceType, LLONG_MAX);
	else
		lvResources.SetAt(nResourceType, lValue1 + lValue);
}

void RMResourceContainer::UpdateMin(const RMResourceContainer* other)
{
	int nRT;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		if (other->GetValue(nRT) < GetValue(nRT))
			SetValue(nRT, other->GetValue(nRT));
	}
}

void RMResourceContainer::Initialize()
{
	lvResources.Initialize();
}

void RMResourceContainer::Write(ostream& ost) const
{
	int nRT;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		if (GetValue(nRT) == LLONG_MAX)
			ost << "MAX";
		else
			ost << LongintToHumanReadableString(GetValue(nRT));
		if (nRT != RESOURCES_NUMBER - 1)
			ost << "\t";
	}
}

//////////////////////////////////////////////////////////////////////
// Classe PLShared_ResourceContainer

PLShared_ResourceContainer::PLShared_ResourceContainer() {}
PLShared_ResourceContainer::~PLShared_ResourceContainer() {}

void PLShared_ResourceContainer::SetResourceContainer(RMResourceContainer* rmResourceContainer)
{
	SetObject(rmResourceContainer);
}

RMResourceContainer* PLShared_ResourceContainer::GetResourceContainer()
{
	return cast(RMResourceContainer*, GetObject());
}

void PLShared_ResourceContainer::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const RMResourceContainer* container;
	container = cast(RMResourceContainer*, o);
	serializer->PutLongintVector(&container->lvResources);
}
void PLShared_ResourceContainer::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMResourceContainer* container;
	container = cast(RMResourceContainer*, o);
	serializer->GetLongintVector(&container->lvResources);
}

Object* PLShared_ResourceContainer::Create() const
{
	return new RMResourceContainer;
}

///////////////////////////////////////////////////////////////////////
// Classe RMResourceGrant

RMResourceGrant::RMResourceGrant()
{
	rcResource = new RMResourceContainer;
	rcSharedResource = new RMResourceContainer;
}

RMResourceGrant::~RMResourceGrant()
{
	delete rcResource;
	delete rcSharedResource;
	rcResource = NULL;
	rcSharedResource = NULL;
}

void RMResourceGrant::SetMemory(longint lValue)
{
	rcResource->SetValue(MEMORY, lValue);
}

longint RMResourceGrant::GetMemory() const
{
	return GetResource()->GetValue(MEMORY);
}

void RMResourceGrant::SetDiskSpace(longint lValue)
{
	rcResource->SetValue(DISK, lValue);
}

longint RMResourceGrant::GetDiskSpace() const
{
	return GetResource()->GetValue(DISK);
}

void RMResourceGrant::SetSharedMemory(longint lMemory)
{
	rcSharedResource->SetValue(MEMORY, lMemory);
}

longint RMResourceGrant::GetSharedMemory() const
{
	return GetSharedResource()->GetValue(MEMORY);
}

void RMResourceGrant::SetSharedDisk(longint lDisk)
{
	rcSharedResource->SetValue(DISK, lDisk);
}
longint RMResourceGrant::GetSharedDisk() const
{
	return GetSharedResource()->GetValue(DISK);
}

RMResourceContainer* RMResourceGrant::GetSharedResource() const
{
	return rcSharedResource;
}

RMResourceContainer* RMResourceGrant::GetResource() const
{
	return rcResource;
}

boolean RMResourceGrant::Check() const
{
	int nResource;
	for (nResource = 0; nResource < RESOURCES_NUMBER; nResource++)
	{
		if (GetResource()->GetValue(nResource) == -1 or GetSharedResource()->GetValue(nResource) == -1)
			return false;
	}
	return true;
}

void RMResourceGrant::Write(ostream& ost) const
{
	ost << "Resource grant"
	    << "\t" << *rcResource << endl;
	ost << "\t"
	    << "Shared " << *rcSharedResource << endl;
}

///////////////////////////////////////////////////////////////////////
// Classe RMTaskResourceGrant

RMTaskResourceGrant::RMTaskResourceGrant()
{
	bMissingResourceForProcConstraint = false;
	bMissingResourceForGlobalConstraint = false;
	slaveResourceGrant = new RMResourceGrant;
	masterResourceGrant = new RMResourceGrant;
	rcMissingResources = new RMResourceContainer;
	nProcessNumber = 0;
}

RMTaskResourceGrant::~RMTaskResourceGrant()
{
	odHostProcNumber.DeleteAll();
	delete slaveResourceGrant;
	delete masterResourceGrant;
	delete rcMissingResources;
	slaveResourceGrant = NULL;
	masterResourceGrant = NULL;
	rcMissingResources = NULL;
}

longint RMTaskResourceGrant::GetMissingMemory() const
{
	return rcMissingResources->GetValue(MEMORY);
}
longint RMTaskResourceGrant::GetMissingDisk() const
{
	return rcMissingResources->GetValue(DISK);
}

ALString RMTaskResourceGrant::GetMissingResourceMessage()
{
	ALString sMessage;
	ALString sTmp;
	ALString sMissingResource;
	boolean bMissingMemory;
	boolean bMissingDisk;

	require(GetMissingMemory() > 0 or GetMissingDisk() > 0);
	require(IsEmpty());

	bMissingMemory = GetMissingMemory() > 0;
	bMissingDisk = GetMissingDisk() > 0;

	if (bMissingMemory)
	{
		sMissingResource = RMResourceManager::ActualMemoryToString(GetMissingMemory());
	}
	else
	{
		sMissingResource = LongintToHumanReadableString(longint(1.1 * GetMissingDisk()));
	}

	if (bMissingResourceForProcConstraint)
	{
		if (bMissingMemory)
		{
			sMessage = "requirements on memory exceed user constraint per processus (need extra " +
				   sMissingResource + " of additional memory)";
		}
		else
		{
			sMessage = "requirements on disk space exceed user constraint per processus (need extra " +
				   sMissingResource +
				   " of additional disk space on the Khiops temporary files directory " +
				   FileService::GetTmpDir() + ")";
		}
	}
	else
	{
		if (bMissingResourceForGlobalConstraint)
		{
			if (bMissingMemory)
			{
				sMessage = "system resources are not sufficient to run the task (need " +
					   sMissingResource + " of additional memory";
			}
			else
			{
				sMessage = "system resources are not sufficient to run the task (need " +
					   sMissingResource +
					   " of additional disk space on the Khiops temporary files directory " +
					   FileService::GetTmpDir();
			}

			if (RMResourceManager::GetResourceSystem()->GetHostNumber() != 1)
				sMessage += " on " + sHostMissingResource;
			sMessage += ")";
		}
		else
		{
			if (bMissingMemory)
			{
				sMessage = "system resources are not sufficient to run the task (need " +
					   sMissingResource + " of additional memory";
			}
			else
			{
				sMessage = "system resources are not sufficient to run the task (need " +
					   sMissingResource +
					   " of additional disk space on the Khiops temporary files directory " +
					   FileService::GetTmpDir();
			}
			if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1)
				sMessage += " on " + sHostMissingResource;
			sMessage += ")";
		}
	}
	return sMessage;
}

void RMTaskResourceGrant::Initialize()
{
	rcMissingResources->Initialize();
	bMissingResourceForProcConstraint = false;
	odHostProcNumber.DeleteAll();
	ivProcessWithResources.SetSize(0);
	nProcessNumber = 0;
}

void RMTaskResourceGrant::SetHostCoreNumber(const ALString& sHostName, int nProcNumber)
{
	IntObject* io;
	io = new IntObject;
	io->SetInt(nProcNumber);
	odHostProcNumber.SetAt(sHostName, io);
	nProcessNumber += nProcNumber;
}

void RMTaskResourceGrant::Write(ostream& ost) const
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	const IntObject* ioProcNumber;
	StringVector svHostName;
	int i;
	int nRT;
	longint lValue;
	int nDisplayedValueCount;

	ost << "--    Granted resources    --" << endl;
	if (IsEmpty())
	{
		ost << " No resource available" << endl;
	}
	else
	{
		ost << "#procs " << GetProcessNumber() << endl;
		for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
		{
			cout << ResourceToString(nRT) << endl;
			lValue = masterResourceGrant->GetResource()->GetValue(nRT);
			cout << "  master " << LongintToHumanReadableString(lValue) << " -> "
			     << LongintToReadableString(lValue);
			cout << endl;

			lValue = slaveResourceGrant->GetResource()->GetValue(nRT);
			cout << "  slave  " << LongintToHumanReadableString(lValue) << " -> "
			     << LongintToReadableString(lValue);
			cout << endl;

			lValue = slaveResourceGrant->GetSharedResource()->GetValue(nRT);
			cout << "  shared " << LongintToHumanReadableString(lValue) << " -> "
			     << LongintToReadableString(lValue);
			cout << endl;
		}
		cout << endl;

		// Affichage de la liste des hosts, on passe par un vecteur
		// pour les afficher dans l'ordre alphabetique

		position = odHostProcNumber.GetStartPosition();
		while (position != NULL)
		{
			odHostProcNumber.GetNextAssoc(position, sHostName, oElement);
			svHostName.Add(sHostName);
		}
		svHostName.Sort();
		for (i = 0; i < svHostName.GetSize(); i++)
		{
			if (i == 20)
			{
				ost << "..." << endl;
				break;
			}
			sHostName = svHostName.GetAt(i);
			ioProcNumber = cast(IntObject*, odHostProcNumber.Lookup(sHostName));
			ost << sHostName << "\t" << ioProcNumber->GetInt() << " procs" << endl;
		}

		// Affichage du rang des 100 premiers processus qui travaillent
		cout << "Ranks involved :";
		nDisplayedValueCount = 0;
		for (i = 0; i < ivProcessWithResources.GetSize(); i++)
		{
			if (nDisplayedValueCount == 101)
			{
				cout << " ...";
				break;
			}

			if (ivProcessWithResources.GetAt(i) == 1)
			{
				cout << " " << i;
				nDisplayedValueCount++;
			}
		}
		cout << endl;
	}
}

boolean RMTaskResourceGrant::IsResourcesForProcess(int nRank) const
{
	require(nRank < ivProcessWithResources.GetSize());
	return ivProcessWithResources.GetAt(nRank) == 1;
}

longint RMTaskResourceGrant::GetMasterMemory() const
{
	return GetMasterResource(MEMORY);
}

longint RMTaskResourceGrant::GetMasterDisk() const
{
	return GetMasterResource(DISK);
}

longint RMTaskResourceGrant::GetMasterResource(int nResourceType) const
{
	return masterResourceGrant->GetResource()->GetValue(nResourceType);
}

longint RMTaskResourceGrant::GetSharedMemory() const
{
	return GetSharedResource(MEMORY);
}

longint RMTaskResourceGrant::GetSharedResource(int nResourceType) const
{
	assert(masterResourceGrant->GetSharedResource()->GetValue(nResourceType) ==
	       slaveResourceGrant->GetSharedResource()->GetValue(nResourceType));
	return masterResourceGrant->GetSharedResource()->GetValue(nResourceType);
}

longint RMTaskResourceGrant::GetSlaveMemory() const
{
	return GetSlaveResource(MEMORY);
}

longint RMTaskResourceGrant::GetSlaveDisk() const
{
	return GetSlaveResource(DISK);
}

longint RMTaskResourceGrant::GetSlaveResource(int nRT) const
{
	return slaveResourceGrant->GetResource()->GetValue(nRT);
}

const RMResourceGrant* RMTaskResourceGrant::GetGrantedResourceForSlave() const
{
	return slaveResourceGrant;
}

const RMResourceGrant* RMTaskResourceGrant::GetGrantedResourceForMaster() const
{
	return masterResourceGrant;
}

int RMTaskResourceGrant::GetProcessNumber() const
{
	return nProcessNumber;
}

int RMTaskResourceGrant::GetSlaveNumber() const
{
	if (nProcessNumber <= 1)
		return nProcessNumber;
	return nProcessNumber - 1;
}

boolean RMTaskResourceGrant::IsSequentialTask() const
{

	return nProcessNumber == 1;
}

boolean RMTaskResourceGrant::IsEmpty() const
{
	return nProcessNumber == 0;
}
