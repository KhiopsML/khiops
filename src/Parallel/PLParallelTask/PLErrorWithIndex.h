// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Ermgt.h"
#include "Object.h"
#include "PLSharedObject.h"

//////////////////////////////////////////////////////////////////////////
// Classe PLErrorWithIndex
// Classe Error de Norm + un index de numero de ligne local
class PLErrorWithIndex : public Object
{
public:
	PLErrorWithIndex();
	~PLErrorWithIndex();
	void SetError(Error* error);
	Error* GetError() const;
	void SetIndex(longint nIndex);
	longint GetIndex() const;
	PLErrorWithIndex* Clone() const;

protected:
	longint lIndex;
	Error* error;
};

//////////////////////////////////////////////////////////////////////////
// Classe PLSharedErrorWithIndex
// Serialisation de la classe PLErrorWithIndex
class PLSharedErrorWithIndex : public PLSharedObject
{
public:
	// Constructeur
	PLSharedErrorWithIndex();
	~PLSharedErrorWithIndex();

	// Acces a l'erreur
	void SetErrorIndex(PLErrorWithIndex* e);
	PLErrorWithIndex* GetErrorIndex();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	Object* Create() const override;
};

inline void PLErrorWithIndex::SetError(Error* errorValue)
{
	error = errorValue;
}
inline Error* PLErrorWithIndex::GetError() const
{
	return error;
}
inline void PLErrorWithIndex::SetIndex(longint lIndexValue)
{
	lIndex = lIndexValue;
}

inline longint PLErrorWithIndex::GetIndex() const
{
	return lIndex;
}

inline void PLSharedErrorWithIndex::SetErrorIndex(PLErrorWithIndex* e)
{
	SetObject(e);
}

inline PLErrorWithIndex* PLSharedErrorWithIndex::GetErrorIndex()
{
	return cast(PLErrorWithIndex*, GetObject());
}

inline Object* PLSharedErrorWithIndex::Create() const
{
	return new PLErrorWithIndex;
}
