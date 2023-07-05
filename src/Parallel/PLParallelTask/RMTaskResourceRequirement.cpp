// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMTaskResourceRequirement.h"

///////////////////////////////////////////////////////////////////////
// Implementation de RMTaskResourceRequirement

RMResourceRequirement RMTaskResourceRequirement::masterSystemAtStart;
RMResourceRequirement RMTaskResourceRequirement::slaveSystemAtStart;
longint RMTaskResourceRequirement::lMinProcessDisk = 0;
longint RMTaskResourceRequirement::lMinProcessMem = 8 * lMB;

RMTaskResourceRequirement::RMTaskResourceRequirement()
{
	int nResourceType;

	masterRequirement = new RMResourceRequirement;
	slaveRequirement = new RMResourceRequirement;
	globalSlaveRequirement = new RMResourceRequirement;
	sharedRequirement = new RMResourceRequirement;
	nSlaveProcessNumber =
	    INT_MAX - 1; // on enleve 1 pour que  nSlaveProcessNumber +1 = INT_MAX (= nb de proc sur le systeme)

	// On initialise le max des ressources globales et shared a 0 (par defaut a INT_MAX)
	// On considere que c'est des ressources optionelles : on ne veut pas les prendre en compte par defaut
	globalSlaveRequirement->GetDisk()->Set(0);
	globalSlaveRequirement->GetMemory()->Set(0);
	sharedRequirement->GetDisk()->Set(0);
	sharedRequirement->GetMemory()->Set(0);

	// Politiques d'allocation
	ivResourcesPolicy.SetSize(RESOURCES_NUMBER);
	for (nResourceType = 0; nResourceType < RESOURCES_NUMBER; nResourceType++)
	{
		ivResourcesPolicy.SetAt(nResourceType, slavePreferred);
	}

	// Politiques de parallelisation
	parallelisationPolicy = horizontal;

	// Initialisation des ressources au demarrage avec des valeurs par defaut si necessaire
	// Ca devrait etre la taille de la heap lors de l'initialisation des ressources
	if (slaveSystemAtStart.GetMemory()->GetMin() == 0)
		slaveSystemAtStart.GetMemory()->Set(lMinProcessMem);
	if (slaveSystemAtStart.GetDisk()->GetMin() == 0)
		slaveSystemAtStart.GetDisk()->Set(lMinProcessDisk);
	if (masterSystemAtStart.GetMemory()->GetMin() == 0)
		masterSystemAtStart.GetMemory()->Set(lMinProcessMem);
	if (masterSystemAtStart.GetDisk()->GetMin() == 0)
		masterSystemAtStart.GetDisk()->Set(lMinProcessDisk);
}

RMTaskResourceRequirement::~RMTaskResourceRequirement()
{
	delete masterRequirement;
	delete slaveRequirement;
	delete sharedRequirement;
	delete globalSlaveRequirement;
}

RMTaskResourceRequirement* RMTaskResourceRequirement::Clone() const
{
	RMTaskResourceRequirement* clone;
	clone = new RMTaskResourceRequirement;
	clone->CopyFrom(this);
	return clone;
}

void RMTaskResourceRequirement::CopyFrom(const RMTaskResourceRequirement* trRequirement)
{
	require(trRequirement->Check());

	// Recopie des exigences
	masterRequirement->CopyFrom(trRequirement->GetMasterRequirement());
	sharedRequirement->CopyFrom(trRequirement->GetSharedRequirement());
	slaveRequirement->CopyFrom(trRequirement->GetSlaveRequirement());
	globalSlaveRequirement->CopyFrom(trRequirement->GetGlobalSlaveRequirement());
	nSlaveProcessNumber = trRequirement->GetMaxSlaveProcessNumber();

	// Recopie des politiques
	ivResourcesPolicy.CopyFrom(&trRequirement->ivResourcesPolicy);
	parallelisationPolicy = trRequirement->parallelisationPolicy;
}

void RMTaskResourceRequirement::Write(ostream& ost) const
{
	ost << "--   Task requirements    --" << endl;
	ost << "Slave requirement: " << endl << *slaveRequirement;
	ost << "Master requirement: " << endl << *masterRequirement;
	ost << "Shared variables requirement: " << endl << *sharedRequirement;
	ost << "Slave global requirement: " << endl << *globalSlaveRequirement;
	ost << "Slave system at start: " << endl << slaveSystemAtStart;
	ost << "Master system at start: " << endl << masterSystemAtStart;
	ost << "Number of slaves processes: ";
	if (nSlaveProcessNumber >= INT_MAX - 1)
		ost << "INF" << endl;
	else
		ost << IntToString(nSlaveProcessNumber) << endl;
	ost << "memory policy: " << ResourcePolicyToString(ivResourcesPolicy.GetAt(0)) << endl;
	ost << "disk   policy: " << ResourcePolicyToString(ivResourcesPolicy.GetAt(1)) << endl;
	ost << "paral. policy: " << ParallelisationPolicyToString(parallelisationPolicy) << endl;
}

void RMTaskResourceRequirement::WriteDetails(ostream& ost) const
{
	ost << "--   Task requirements (B)    --" << endl;

	ost << "Slave requirement: " << endl;
	slaveRequirement->WriteDetails(ost);
	ost << "Master requirement: " << endl;
	masterRequirement->WriteDetails(ost);
	ost << "Shared variables requirement: " << endl;
	sharedRequirement->WriteDetails(ost);
	ost << "Slave global requirement: " << endl;
	globalSlaveRequirement->WriteDetails(ost);
	ost << "Slave system at start: " << endl;
	slaveSystemAtStart.WriteDetails(ost);
	ost << "Master system at start: " << endl;
	masterSystemAtStart.WriteDetails(ost);
}

ALString RMTaskResourceRequirement::ResourcePolicyToString(int policy)
{
	switch (policy)
	{
	case masterPreferred:
		return "master first";
	case slavePreferred:
		return "slave first";
	case globalPreferred:
		return "global first";
	case balanced:
		return "balanced";
	default:
		assert(false);
		return "Undefined";
	}
}

ALString RMTaskResourceRequirement::ParallelisationPolicyToString(int policy)
{
	switch (policy)
	{
	case horizontal:
		return "horizontal";
	case vertical:
		return "vertical  ";
	default:
		assert(false);
		return "Undefined";
	}
}

boolean RMTaskResourceRequirement::Check() const
{
	boolean bOk;

	bOk = true;
	if (not masterRequirement->Check())
	{
		AddError("Master requirements are not consitent");
		bOk = false;
	}
	if (not slaveRequirement->Check())
	{
		AddError("Slave requirements are not consitent");
		bOk = false;
	}
	if (not sharedRequirement->Check())
	{
		AddError("Shared requirements are not consitent");
		bOk = false;
	}
	if (not globalSlaveRequirement->Check())
	{
		AddError("Global slave requirements are not consitent");
		bOk = false;
	}
	return bOk;
}

void RMTaskResourceRequirement::SetResourceAllocationPolicy(int nResourceType, ALLOCATION_POLICY policy)
{
	require(nResourceType < RESOURCES_NUMBER);
	ivResourcesPolicy.SetAt(nResourceType, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY
RMTaskResourceRequirement::GetResourceAllocationPolicy(int nResourceType) const
{
	require(nResourceType < RESOURCES_NUMBER);
	return (ALLOCATION_POLICY)ivResourcesPolicy.GetAt(nResourceType);
}

void RMTaskResourceRequirement::SetMemoryAllocationPolicy(ALLOCATION_POLICY policy)
{
	SetResourceAllocationPolicy(MEMORY, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY RMTaskResourceRequirement::GetMemoryAllocationPolicy() const
{
	return GetResourceAllocationPolicy(MEMORY);
}

void RMTaskResourceRequirement::SetDiskAllocationPolicy(ALLOCATION_POLICY policy)
{
	SetResourceAllocationPolicy(DISK, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY RMTaskResourceRequirement::GetDiskAllocationPolicy() const
{
	return GetResourceAllocationPolicy(DISK);
}

void RMTaskResourceRequirement::SetParallelisationPolicy(PARALLELISATION_POLICY policy)
{
	parallelisationPolicy = policy;
}

RMTaskResourceRequirement::PARALLELISATION_POLICY RMTaskResourceRequirement::GetParallelisationPolicy() const
{
	return parallelisationPolicy;
}

RMResourceRequirement* RMTaskResourceRequirement::GetMasterRequirement() const
{
	return masterRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetSlaveRequirement() const
{
	return slaveRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetGlobalSlaveRequirement() const
{
	return globalSlaveRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetSharedRequirement() const
{
	return sharedRequirement;
}

void RMTaskResourceRequirement::SetMaxSlaveProcessNumber(int nValue)
{
	require(nValue >= 0);
	nSlaveProcessNumber = nValue;
}

int RMTaskResourceRequirement::GetMaxSlaveProcessNumber() const
{
	return nSlaveProcessNumber;
}