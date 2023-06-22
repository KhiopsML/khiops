// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMResourceConstraints.h"

int RMResourceConstraints::nMaxCoreNumber = INT_MAX;
int RMResourceConstraints::nMaxCoreNumberOnHost = INT_MAX;
int RMResourceConstraints::nMaxProcessNumber = INT_MAX;
int RMResourceConstraints::nOptimizationTime = 0;
int RMResourceConstraints::nResourceLimit[2] = {INT_MAX, INT_MAX};
int RMResourceConstraints::nResourceLimitPerProc[2] = {INT_MAX, INT_MAX};
boolean RMResourceConstraints::bIgnoreMemoryLimit = false;
ALString RMResourceConstraints::sUserTmpDir = "";

ALString ResourceToString(int nRessource)
{
	ALString sRes;

	require(nRessource <= UNKNOWN);

	switch (nRessource)
	{
	case MEMORY:
		sRes = "Memory";
		break;
	case DISK:
		sRes = "Disk";
		break;
	case UNKNOWN:
		sRes = "UNKNOWN";
		break;
	default:
		sRes = "ERROR";
	}
	return sRes;
}

RMResourceConstraints::RMResourceConstraints() {}

RMResourceConstraints::~RMResourceConstraints() {}

void RMResourceConstraints::SetMaxCoreNumber(int nCoreNumber)
{
	require(nCoreNumber > 0);
	nMaxCoreNumber = nCoreNumber;
}

int RMResourceConstraints::GetMaxCoreNumber()
{
	return nMaxCoreNumber;
}

void RMResourceConstraints::SetMaxCoreNumberPerHost(int nCoreNumber)
{
	require(nCoreNumber > 0);
	nMaxCoreNumberOnHost = nCoreNumber;
}

int RMResourceConstraints::GetMaxCoreNumberPerHost()
{
	return nMaxCoreNumberOnHost;
}

void RMResourceConstraints::SetMaxProcessNumber(int nProcessNumber)
{
	require(nProcessNumber > 0);
	nMaxProcessNumber = nProcessNumber;
}

int RMResourceConstraints::GetMaxProcessNumber()
{
	return nMaxProcessNumber;
}

void RMResourceConstraints::SetOptimizationTime(int nValue)
{
	require(nValue >= 0);
	nOptimizationTime = nValue;
}

int RMResourceConstraints::GetOptimizationTime()
{
	return nOptimizationTime;
}

void RMResourceConstraints::SetMemoryLimit(int nMemory)
{
	SetResourceLimit(MEMORY, nMemory);
}

int RMResourceConstraints::GetMemoryLimit()
{
	return GetResourceLimit(MEMORY);
}

void RMResourceConstraints::SetMemoryLimitPerProc(int nMemory)
{
	SetResourceLimitPerProc(MEMORY, nMemory);
}

int RMResourceConstraints::GetMemoryLimitPerProc()
{
	return GetResourceLimitPerProc(MEMORY);
}

void RMResourceConstraints::SetDiskLimitPerProc(int nValue)
{
	SetResourceLimitPerProc(DISK, nValue);
}

int RMResourceConstraints::GetDiskLimitPerProc()
{
	return GetResourceLimitPerProc(DISK);
}

void RMResourceConstraints::SetDiskLimit(int nValue)
{
	SetResourceLimit(DISK, nValue);
}

int RMResourceConstraints::GetDiskLimit()
{
	return GetResourceLimitPerProc(DISK);
}

void RMResourceConstraints::SetResourceLimit(int nResourceType, int nValue)
{
	require(nResourceType < UNKNOWN);
	require(nValue >= 0);
	nResourceLimit[nResourceType] = nValue;
}

int RMResourceConstraints::GetResourceLimit(int nResourceType)
{
	require(nResourceType < UNKNOWN);
	return nResourceLimit[nResourceType];
}

void RMResourceConstraints::SetResourceLimitPerProc(int nResourceType, int nValue)
{
	require(nResourceType < UNKNOWN);
	require(nValue >= 0);
	nResourceLimitPerProc[nResourceType] = nValue;
}

int RMResourceConstraints::GetResourceLimitPerProc(int nResourceType)
{
	require(nResourceType < UNKNOWN);
	return nResourceLimitPerProc[nResourceType];
}

ALString RMResourceConstraints::ToString()
{
	ALString sRes;
	ALString sTmp;

	sTmp = "";
	sRes = "";
	if (nMaxCoreNumber != INT_MAX)
	{
		sRes = sTmp + "nMaxCoreNumber " + IntToString(nMaxCoreNumber) + "\n";
	}
	if (nMaxCoreNumberOnHost != INT_MAX)
	{
		sRes += sTmp + "nMaxCoreNumberOnHost " + IntToString(nMaxCoreNumberOnHost) + "\n";
	}
	if (nMaxProcessNumber != INT_MAX)
	{
		sRes += sTmp + "nMaxProcessNumber " + IntToString(nMaxProcessNumber) + "\n";
	}
	if (nResourceLimit[MEMORY] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimit[MEMORY] " + LongintToHumanReadableString(nResourceLimit[MEMORY] * lMB) +
			"\n";
	}
	if (nResourceLimit[DISK] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimit[DISK] " + LongintToHumanReadableString(nResourceLimit[DISK]) + "\n";
	}
	if (nResourceLimitPerProc[MEMORY] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimitPerProc[MEMORY] " +
			LongintToHumanReadableString(nResourceLimitPerProc[MEMORY] * lMB) + "\n";
	}
	if (nResourceLimitPerProc[DISK] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimitPerProc[DISK] " +
			LongintToHumanReadableString(nResourceLimitPerProc[DISK]) + "\n";
	}
	if (bIgnoreMemoryLimit)
	{
		sRes += "bIgnoreMemoryLimit is true\n";
	}
	return sRes;
}

void RMResourceConstraints::SetIgnoreMemoryLimit(boolean bValue)
{
	bIgnoreMemoryLimit = bValue;
}

boolean RMResourceConstraints::GetIgnoreMemoryLimit()
{
	return bIgnoreMemoryLimit;
}

void RMResourceConstraints::SetUserTmpDir(const ALString& sValue)
{
	sUserTmpDir = sValue;
}

const ALString& RMResourceConstraints::GetUserTmpDir()
{
	return sUserTmpDir;
}