// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLErrorWithIndex.h"

PLErrorWithIndex::PLErrorWithIndex()
{
	error = NULL;
	lIndex = -1;
}
PLErrorWithIndex::~PLErrorWithIndex()
{
	if (error != NULL)
		delete error;
}

PLErrorWithIndex* PLErrorWithIndex::Clone() const
{
	PLErrorWithIndex* clone;
	clone = new PLErrorWithIndex;
	clone->SetError(this->error->Clone());
	clone->SetIndex(this->lIndex);
	return clone;
}
///////////////////////////////////////////////////////////////////////
// Implementation de la classe PLShared_Error

PLSharedErrorWithIndex::PLSharedErrorWithIndex() {}

PLSharedErrorWithIndex::~PLSharedErrorWithIndex() {}

void PLSharedErrorWithIndex::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	assert(o != NULL);
	const PLErrorWithIndex* e = cast(PLErrorWithIndex*, o);
	require(serializer->IsOpenForWrite());

	// Serialisation de l'erreur
	serializer->PutString(e->GetError()->GetCategory());
	serializer->PutInt(e->GetError()->GetGravity());
	serializer->PutString(e->GetError()->GetLabel());
	serializer->PutString(e->GetError()->GetLocalisation());

	// et de l'index
	serializer->PutLongint(e->GetIndex());
}

void PLSharedErrorWithIndex::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLErrorWithIndex* errorWithIndex = cast(PLErrorWithIndex*, o);
	Error* error;
	require(serializer->IsOpenForRead());
	error = new Error;
	error->SetCategory(serializer->GetString());
	error->SetGravity(serializer->GetInt());
	error->SetLabel(serializer->GetString());
	error->SetLocalisation(serializer->GetString());
	errorWithIndex->SetError(error);
	errorWithIndex->SetIndex(serializer->GetLongint());
}
