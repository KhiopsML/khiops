// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLSharedObject.h"
#include "RMResourceManager.h"
#include "RMParallelResourceManager.h"

class PLShared_ResourceGrant;
class PLShared_TaskResourceGrant;

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ResourceGrant
// Serialisation de RMResourceGrant
//
class PLShared_ResourceGrant : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ResourceGrant();
	~PLShared_ResourceGrant();

	// Acces au RMResourceGrant
	void SetResourceGrant(RMResourceGrant* hr);
	RMResourceGrant* GetResourceGrant();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_TaskResourceGrant
// Serialisation de RMTaskResourceGrant
//
class PLShared_TaskResourceGrant : public PLSharedObject
{
public:
	// Constructeur
	PLShared_TaskResourceGrant();
	~PLShared_TaskResourceGrant();

	// Acces au RMResourceGrant
	void SetTaskResourceGrant(RMTaskResourceGrant* hr);
	RMTaskResourceGrant* GetTaskResourceGrant();

	// Test
	static void Test();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

inline void PLShared_ResourceGrant::SetResourceGrant(RMResourceGrant* hr)
{
	SetObject(hr);
}

inline RMResourceGrant* PLShared_ResourceGrant::GetResourceGrant()
{
	return cast(RMResourceGrant*, GetObject());
}

inline Object* PLShared_ResourceGrant::Create() const
{
	return new RMResourceGrant;
}

inline void PLShared_TaskResourceGrant::SetTaskResourceGrant(RMTaskResourceGrant* hr)
{
	SetObject(hr);
}

inline RMTaskResourceGrant* PLShared_TaskResourceGrant::GetTaskResourceGrant()
{
	return cast(RMTaskResourceGrant*, GetObject());
}

inline Object* PLShared_TaskResourceGrant::Create() const
{
	return new RMTaskResourceGrant;
}
