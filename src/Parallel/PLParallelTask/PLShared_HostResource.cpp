// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_HostResource.h"

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_HostResource

PLShared_HostResource::PLShared_HostResource()
{
	// On force bIsDeclared a true car cette classe a pour vocation a etre envoyee en dehors du schema maitre
	// esclave classique
	bIsDeclared = true;
}

PLShared_HostResource::~PLShared_HostResource() {}

void PLShared_HostResource::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMHostResource* hr;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	hr = cast(RMHostResource*, o);
	hr->SetHostName(serializer->GetString());
	hr->SetPhysicalMemory(serializer->GetLongint());
	hr->SetDiskFreeSpace(serializer->GetLongint());
	hr->SetPhysicalCoresNumber(serializer->GetInt());
	serializer->GetIntVector(&hr->ivRanks);
}

void PLShared_HostResource::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	RMHostResource* hr;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	hr = cast(RMHostResource*, o);
	serializer->PutString(hr->GetHostName());
	serializer->PutLongint(hr->GetPhysicalMemory());
	serializer->PutLongint(hr->GetDiskFreeSpace());
	serializer->PutInt(hr->GetPhysicalCoreNumber());
	serializer->PutIntVector(&hr->ivRanks);
}