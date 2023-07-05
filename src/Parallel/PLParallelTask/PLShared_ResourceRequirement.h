// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLSharedObject.h"
#include "RMResourceManager.h"

class PLShared_PhysicalResource;
class PLShared_ResourceRequirement;

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ResourceRequirement
// Serialisation de RMResourceRequirement
//
class PLShared_ResourceRequirement : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ResourceRequirement();
	~PLShared_ResourceRequirement();

	// Acces a l'exigence
	void SetRequirement(RMResourceRequirement*);
	RMResourceRequirement* GetRequirement() const;

	// Methode de test
	static boolean Test();

	/// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/////// Implementation

protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_PhysicalResource
// Serialisation de RMPhysicalResource
//
class PLShared_PhysicalResource : public PLSharedObject
{
public:
	// Constructeur
	PLShared_PhysicalResource();
	~PLShared_PhysicalResource();

	// Acces a la ressource
	void SetResource(RMPhysicalResource*);
	RMPhysicalResource* GetResource();

	/// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/////// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLShared_ResourceRequirement::SetRequirement(RMResourceRequirement* r)
{
	SetObject(r);
}

inline RMResourceRequirement* PLShared_ResourceRequirement::GetRequirement() const
{
	return cast(RMResourceRequirement*, GetObject());
}

inline void PLShared_PhysicalResource::SetResource(RMPhysicalResource* p)
{
	SetObject(p);
}

inline RMPhysicalResource* PLShared_PhysicalResource::GetResource()
{
	return cast(RMPhysicalResource*, GetObject());
}