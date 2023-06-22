// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_BufferedFile.h"

PLShared_InputBufferedFile::PLShared_InputBufferedFile() {}

PLShared_InputBufferedFile::~PLShared_InputBufferedFile() {}

void PLShared_InputBufferedFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	InputBufferedFile* ibf = cast(InputBufferedFile*, o);

	require(serializer->IsOpenForRead());

	ibf->SetBufferSize(serializer->GetInt());
	ibf->SetFieldSeparator(serializer->GetChar());
	ibf->SetFileName(serializer->GetString());
	ibf->SetHeaderLineUsed(serializer->GetBoolean());
}

void PLShared_InputBufferedFile::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	InputBufferedFile* ibf = cast(InputBufferedFile*, o);

	require(serializer->IsOpenForWrite());

	serializer->PutInt(ibf->GetBufferSize());
	serializer->PutChar(ibf->GetFieldSeparator());
	serializer->PutString(ibf->GetFileName());
	serializer->PutBoolean(ibf->GetHeaderLineUsed());
}

PLShared_OutputBufferedFile::PLShared_OutputBufferedFile() {}

PLShared_OutputBufferedFile::~PLShared_OutputBufferedFile() {}

void PLShared_OutputBufferedFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	OutputBufferedFile* obf = cast(OutputBufferedFile*, o);

	require(serializer->IsOpenForRead());

	obf->SetBufferSize(serializer->GetInt());
	obf->SetFieldSeparator(serializer->GetChar());
	obf->SetFileName(serializer->GetString());
	obf->SetHeaderLineUsed(serializer->GetBoolean());
}

void PLShared_OutputBufferedFile::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	OutputBufferedFile* obf = cast(OutputBufferedFile*, o);

	require(serializer->IsOpenForWrite());

	serializer->PutInt(obf->GetBufferSize());
	serializer->PutChar(obf->GetFieldSeparator());
	serializer->PutString(obf->GetFileName());
	serializer->PutBoolean(obf->GetHeaderLineUsed());
}