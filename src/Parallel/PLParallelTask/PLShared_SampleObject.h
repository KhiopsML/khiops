// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLSharedObject.h"

////////////////////////////////////////////////////////////////////////////
// Classe PLShared_SampleObject.
// Exemple d'implementation d'un PLSharedObject
// La classe serialisee est SampleObject
//
class PLShared_SampleObject : public PLSharedObject
{
public:
	PLShared_SampleObject();
	~PLShared_SampleObject();

	// Acces au SampleObject
	void SetSampleObject(SampleObject* so);
	SampleObject* GetSampleObject();

	// Methodes de test
	static void Test();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLShared_SampleObject::SetSampleObject(SampleObject* so)
{
	SetObject(so);
}

inline SampleObject* PLShared_SampleObject::GetSampleObject()
{
	// Simple cast
	return cast(SampleObject*, GetObject());
}

inline Object* PLShared_SampleObject::Create() const
{
	return new SampleObject;
}
