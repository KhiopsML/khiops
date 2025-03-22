// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLSharedObject.h"
#include "RMResourceSystem.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_HostResource
// Serialisation de RMHostResource
//
class PLShared_HostResource : public PLSharedObject
{
public:
	// Constructeur
	PLShared_HostResource();
	~PLShared_HostResource();

	// Acces au RMHostResource
	void SetHostResource(RMHostResource* hr);
	RMHostResource* GetHostResource();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

inline void PLShared_HostResource::SetHostResource(RMHostResource* hr)
{
	SetObject(hr);
}

inline RMHostResource* PLShared_HostResource::GetHostResource()
{
	return cast(RMHostResource*, GetObject());
}

inline Object* PLShared_HostResource::Create() const
{
	return new RMHostResource;
}
