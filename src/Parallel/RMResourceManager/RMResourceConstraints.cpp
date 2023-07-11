// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMResourceConstraints.h"

int RMResourceConstraints::nMaxCoreNumber = INT_MAX;
int RMResourceConstraints::nMaxCoreNumberOnHost = INT_MAX;
int RMResourceConstraints::nMaxProcessNumber = INT_MAX;
int RMResourceConstraints::nOptimizationTime = 0;
int RMResourceConstraints::nResourceLimit[2] = {INT_MAX, INT_MAX};
boolean RMResourceConstraints::bIgnoreMemoryLimit = false;
ALString RMResourceConstraints::sUserTmpDir = "";

ALString ResourceToString(int nResource)
{
	ALString sRes;

	require(nResource <= RESOURCES_NUMBER);

	switch (nResource)
	{
	case MEMORY:
		sRes = "Memory";
		break;
	case DISK:
		sRes = "Disk  ";
		break;
	case RESOURCES_NUMBER:
		sRes = "RESOURCES_NUMBER";
		break;
	default:
		sRes = "ERROR";
	}
	return sRes;
}

RMResourceConstraints::RMResourceConstraints() {}

RMResourceConstraints::~RMResourceConstraints() {}

void RMResourceConstraints::SetMaxCoreNumberOnCluster(int nCoreNumber)
{
	require(nCoreNumber > 0);
	nMaxCoreNumber = nCoreNumber;
}

int RMResourceConstraints::GetMaxCoreNumberOnCluster()
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

void RMResourceConstraints::SetDiskLimit(int nValue)
{
	SetResourceLimit(DISK, nValue);
}

int RMResourceConstraints::GetDiskLimit()
{
	return GetResourceLimit(DISK);
}

void RMResourceConstraints::SetResourceLimit(int nResourceType, int nValue)
{
	require(nResourceType < RESOURCES_NUMBER);
	require(nValue >= 0);
	nResourceLimit[nResourceType] = nValue;
}

int RMResourceConstraints::GetResourceLimit(int nResourceType)
{
	require(nResourceType < RESOURCES_NUMBER);
	return nResourceLimit[nResourceType];
}

ALString RMResourceConstraints::ToString()
{
	ALString sRes;
	ALString sTmp;

	sTmp = "";
	sRes = "";
	if (nMaxCoreNumber != INT_MAX)
	{
		sRes = sTmp + "nMaxCoreNumber: " + IntToString(nMaxCoreNumber) + "\n";
	}
	if (nMaxCoreNumberOnHost != INT_MAX)
	{
		sRes += sTmp + "nMaxCoreNumberOnHost: " + IntToString(nMaxCoreNumberOnHost) + "\n";
	}
	if (nMaxProcessNumber != INT_MAX)
	{
		sRes += sTmp + "nMaxProcessNumber: " + IntToString(nMaxProcessNumber) + "\n";
	}
	if (nResourceLimit[MEMORY] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimit[MEMORY]: " + LongintToHumanReadableString(nResourceLimit[MEMORY] * lMB) +
			"\n";
	}
	if (nResourceLimit[DISK] != INT_MAX)
	{
		sRes += sTmp + "nResourceLimit[DISK]: " + LongintToHumanReadableString(nResourceLimit[DISK]) + "\n";
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
