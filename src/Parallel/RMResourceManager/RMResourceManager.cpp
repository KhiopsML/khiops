// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMResourceManager.h"

RMStandardResourceDriver* RMResourceManager::resourceDriver = NULL;
RMResourceSystem RMResourceManager::resourceSystem;

///////////////////////////////////////////////////////////////////////
// Classe RMResourceManager
RMResourceManager::RMResourceManager() {}

RMResourceManager::~RMResourceManager() {}

RMResourceSystem* RMResourceManager::GetResourceSystem()
{
	return &resourceSystem;
}

int RMResourceManager::GetPhysicalCoreNumber()
{
	return resourceSystem.GetPhysicalCoreNumber();
}

int RMResourceManager::GetLogicalProcessNumber()
{
	return resourceSystem.GetLogicalProcessNumber();
}

longint RMResourceManager::GetTmpDirFreeSpace()
{
	return GetResourceDriver()->GetTmpDirFreeSpace();
}

longint RMResourceManager::GetRemainingAvailableMemory()
{
	return GetResourceDriver()->GetRemainingAvailableMemory();
}

longint RMResourceManager::GetTotalAvailableMemory()
{
	return GetResourceDriver()->GetTotalAvailableMemory();
}

longint RMResourceManager::GetHeapLogicalMemory()
{
	return RMStandardResourceDriver::PhysicalToLogical(MEMORY, MemGetHeapMemory());
}

longint RMResourceManager::GetSystemMemoryReserve()
{
	return SystemFileDriverCreator::GetMaxPreferredBufferSize() +
	       RMStandardResourceDriver::PhysicalToLogical(MEMORY, UIObject::GetUserInterfaceMemoryReserve() +
								       MemGetPhysicalMemoryReserve() +
								       MemGetAllocatorReserve());
}

void RMResourceManager::DisplayIgnoreMemoryLimitMessage()
{
	Global::AddSimpleMessage(
	    "WARNING: As memory limit is ignored, the program continues but may run out of memory and crash");
}

ALString RMResourceManager::BuildMissingMemoryMessage(longint lNecessaryMemory)
{
	ALString sMessage;
	longint lRemainingMemory;
	longint lMissingMemory;

	require(lNecessaryMemory > 0);

	// Calcul de la memoire manquante
	lRemainingMemory = GetRemainingAvailableMemory();
	lMissingMemory = -1;
	if (lNecessaryMemory > lRemainingMemory)
		lMissingMemory = lNecessaryMemory - lRemainingMemory;
	assert(lMissingMemory >= 0);

	// Message tenant compte de l'overhead d'allocation
	sMessage = sMessage + " (needs extra " + ActualMemoryToString(lMissingMemory) + ")";
	return sMessage;
}

ALString RMResourceManager::ActualMemoryToString(longint lMemory)
{
	const double dOverhead = 0.15;
	longint lUserMemory;
	ALString sStringMemory;

	require(lMemory > 0);

	lUserMemory = (longint)ceil((1 + dOverhead) * lMemory * (1 + MemGetAllocatorOverhead()));
	lUserMemory += lMB;
	sStringMemory = LongintToHumanReadableString(lUserMemory);
	return sStringMemory;
}

ALString RMResourceManager::BuildMemoryLimitMessage()
{
	return "You may increase the memory limit in the system parameters";
}

void RMResourceManager::SetResourceDriver(RMStandardResourceDriver* driver)
{
	resourceDriver = driver;
}

RMStandardResourceDriver* RMResourceManager::GetResourceDriver()
{
	if (resourceDriver == NULL)
		resourceDriver = RMStandardResourceDriver::GetDriver();
	return resourceDriver;
}

///////////////////////////////////////////////////////////////////////
// Classe RMResourceRequirement

RMResourceRequirement::RMResourceRequirement()
{
	oaResources.Add(new RMPhysicalResource);
	oaResources.Add(new RMPhysicalResource);
}

RMResourceRequirement::~RMResourceRequirement()
{
	oaResources.DeleteAll();
}

RMResourceRequirement* RMResourceRequirement::Clone() const
{
	RMResourceRequirement* clone;
	clone = new RMResourceRequirement;
	clone->CopyFrom(this);
	return clone;
}

void RMResourceRequirement::CopyFrom(const RMResourceRequirement* requirement)
{
	int i;
	RMPhysicalResource* physicalResource;
	require(requirement->Check());

	oaResources.DeleteAll();
	for (i = 0; i < requirement->oaResources.GetSize(); i++)
	{
		physicalResource = new RMPhysicalResource;
		physicalResource->CopyFrom(cast(RMPhysicalResource*, requirement->oaResources.GetAt(i)));
		oaResources.Add(physicalResource);
	}
}

RMPhysicalResource* RMResourceRequirement::GetMemory() const
{
	return GetResource(MEMORY);
}

RMPhysicalResource* RMResourceRequirement::GetDisk() const
{
	return GetResource(DISK);
}

boolean RMResourceRequirement::Check() const
{
	int i = 0;
	for (i = 0; i < oaResources.GetSize(); i++)
	{
		if (not cast(RMPhysicalResource*, oaResources.GetAt(i))->Check())
			return false;
	}
	return true;
}

void RMResourceRequirement::Write(ostream& ost) const
{
	ost << "\t" << ResourceToString(MEMORY) << " " << *oaResources.GetAt(MEMORY) << endl
	    << "\t" << ResourceToString(DISK) << " " << *oaResources.GetAt(DISK) << endl;
}
void RMResourceRequirement::WriteDetails(ostream& ost) const
{
	ost << "\t" << ResourceToString(MEMORY) << " ";
	cast(RMPhysicalResource*, oaResources.GetAt(MEMORY))->WriteDetails(ost);
	ost << endl << "\t" << ResourceToString(DISK) << " ";
	cast(RMPhysicalResource*, oaResources.GetAt(DISK))->WriteDetails(ost);
	ost << endl;
}

///////////////////////////////////////////////////////////////////////
// Classe RMPhysicalResource

RMPhysicalResource::RMPhysicalResource()
{
	lMin = 0;
	lMax = 0;
}

RMPhysicalResource::~RMPhysicalResource() {}

RMPhysicalResource* RMPhysicalResource::Clone() const
{
	RMPhysicalResource* clone;
	clone = new RMPhysicalResource;
	clone->CopyFrom(this);
	return clone;
}

void RMPhysicalResource::CopyFrom(const RMPhysicalResource* resource)
{
	require(resource->Check());
	lMin = resource->lMin;
	lMax = resource->lMax;
}

void RMPhysicalResource::SetMax(longint lValue)
{
	lMax = lValue;
}

void RMPhysicalResource::SetMin(longint lValue)
{
	lMin = lValue;
}

void RMPhysicalResource::UpgradeMin(longint lDeltaMin)
{
	require(lDeltaMin >= 0);
	if (lMin + lDeltaMin >= 0)
		lMin += lDeltaMin;
	else
		lMin = LLONG_MAX;
}

void RMPhysicalResource::UpgradeMax(longint lDeltaMax)
{
	require(lDeltaMax >= 0);
	if (lMax + lDeltaMax >= 0)
		lMax += lDeltaMax;
	else
		lMax = LLONG_MAX;
}

void RMPhysicalResource::Set(longint lMinMax)
{
	lMin = lMinMax;
	lMax = lMinMax;
}

void RMPhysicalResource::Write(ostream& ost) const
{
	ost << "min : " << LongintToHumanReadableString(lMin) << " max : ";
	if (lMax == LLONG_MAX)
		ost << "INF";
	else
		ost << LongintToHumanReadableString(lMax);
}

void RMPhysicalResource::WriteDetails(ostream& ost) const
{
	ost << "min : " << lMin << " max : ";
	if (lMax == LLONG_MAX)
		ost << "INF";
	else
		ost << lMax;
}

boolean RMPhysicalResource::Check() const
{
	return lMax >= lMin and lMin >= 0;
}
