// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTokenFrequency.h"

void KWTokenFrequency::Write(ostream& ost) const
{
	ost << "(" << KWTextService::ByteStringToWord(sToken) << ", " << lFrequency << ")";
}

int KWTokenFrequencyCompareToken(const void* elem1, const void* elem2)
{
	return cast(KWTokenFrequency*, *(Object**)elem1)->CompareToken(cast(KWTokenFrequency*, *(Object**)elem2));
}

int KWTokenFrequencyCompareFrequency(const void* elem1, const void* elem2)
{
	return cast(KWTokenFrequency*, *(Object**)elem1)->CompareFrequency(cast(KWTokenFrequency*, *(Object**)elem2));
}

int KWTokenFrequencyCompareLengthFrequency(const void* elem1, const void* elem2)
{
	return cast(KWTokenFrequency*, *(Object**)elem1)
	    ->CompareLengthFrequency(cast(KWTokenFrequency*, *(Object**)elem2));
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_TokenFrequency

PLShared_TokenFrequency::PLShared_TokenFrequency() {}

PLShared_TokenFrequency::~PLShared_TokenFrequency() {}

void PLShared_TokenFrequency::SetTokenFrequency(KWTokenFrequency* tfObject)
{
	SetObject(tfObject);
}

KWTokenFrequency* PLShared_TokenFrequency::GetTokenFrequency()
{
	return cast(KWTokenFrequency*, GetObject());
}

void PLShared_TokenFrequency::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const KWTokenFrequency* tfObject;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	tfObject = cast(KWTokenFrequency*, o);
	serializer->PutString(tfObject->GetToken());
	serializer->PutLongint(tfObject->GetFrequency());
}

void PLShared_TokenFrequency::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWTokenFrequency* tfObject;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	tfObject = cast(KWTokenFrequency*, o);
	tfObject->SetToken(serializer->GetString());
	tfObject->SetFrequency(serializer->GetLongint());
}

Object* PLShared_TokenFrequency::Create() const
{
	return new KWTokenFrequency;
}
