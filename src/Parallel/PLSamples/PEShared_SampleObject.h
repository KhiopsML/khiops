// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLSharedObject.h"

////////////////////////////////////////////////////////////////////////////
// Classe PEShared_SampleObject.
// Exemple d'implementation d'un PLSharedObject
// La classe serialisee est SampleObject
//
class PEShared_SampleObject : public PLSharedObject
{
public:
	PEShared_SampleObject();
	~PEShared_SampleObject();

	// Acces au SampleObject
	void SetSampleObject(SampleObject* so);
	SampleObject* GetSampleObject();

	// Methodes de test
	static void Test();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PEShared_SampleObject::SetSampleObject(SampleObject* so)
{
	SetObject(so);
}

inline SampleObject* PEShared_SampleObject::GetSampleObject()
{
	// Simple cast
	return cast(SampleObject*, GetObject());
}

inline Object* PEShared_SampleObject::Create() const
{
	return new SampleObject;
}
