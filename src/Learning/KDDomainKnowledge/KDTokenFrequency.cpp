// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDTokenFrequency.h"

int KDTokenFrequency::GetSpaceCharNumber() const
{
	int nNumber;
	int i;

	nNumber = 0;
	for (i = 0; i < sToken.GetLength(); i++)
	{
		if (isspace(sToken.GetAt(i)))
			nNumber++;
	}
	return nNumber;
}

int KDTokenFrequency::GetPunctuationCharNumber() const
{
	int nNumber;
	int i;

	nNumber = 0;
	for (i = 0; i < sToken.GetLength(); i++)
	{
		if (ispunct(sToken.GetAt(i)))
			nNumber++;
	}
	return nNumber;
}

void KDTokenFrequency::Write(ostream& ost) const
{
	ost << "(" << sToken << ", " << lFrequency << ")";
}

int KDTokenFrequencyCompareToken(const void* elem1, const void* elem2)
{
	return cast(KDTokenFrequency*, *(Object**)elem1)->CompareToken(cast(KDTokenFrequency*, *(Object**)elem2));
}

int KDTokenFrequencyCompareFrequency(const void* elem1, const void* elem2)
{
	return cast(KDTokenFrequency*, *(Object**)elem1)->CompareFrequency(cast(KDTokenFrequency*, *(Object**)elem2));
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLShared_TokenFrequency

PLShared_TokenFrequency::PLShared_TokenFrequency() {}

PLShared_TokenFrequency::~PLShared_TokenFrequency() {}

void PLShared_TokenFrequency::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const KDTokenFrequency* tfObject;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	tfObject = cast(KDTokenFrequency*, o);
	serializer->PutString(tfObject->GetToken());
	serializer->PutLongint(tfObject->GetFrequency());
}

void PLShared_TokenFrequency::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KDTokenFrequency* tfObject;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	tfObject = cast(KDTokenFrequency*, o);
	tfObject->SetToken(serializer->GetString());
	tfObject->SetFrequency(serializer->GetLongint());
}
