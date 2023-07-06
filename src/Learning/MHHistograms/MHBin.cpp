// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHBin.h"

boolean MHBin::Check() const
{
	return cUpperValue >= cLowerValue and nFrequency > 0;
}

void MHBin::CopyFrom(const MHBin* aSource)
{
	require(aSource != NULL);

	cLowerValue = aSource->cLowerValue;
	cUpperValue = aSource->cUpperValue;
	nFrequency = aSource->nFrequency;
}

MHBin* MHBin::Clone() const
{
	MHBin* binClone;
	binClone = new MHBin;
	binClone->CopyFrom(this);
	return binClone;
}

void MHBin::WriteHeaderLine(ostream& ost) const
{
	ost << "Lower value\tUpper value\tFrequency";
}

void MHBin::Write(ostream& ost) const
{
	ost << KWContinuous::ContinuousToString(GetLowerValue()) << '\t';
	ost << KWContinuous::ContinuousToString(GetUpperValue()) << '\t';
	ost << KWContinuous::ContinuousToString(GetFrequency());
}

void MHBin::SortBinArray(ObjectArray* oaBins)
{
	require(oaBins != NULL);

	oaBins->SetCompareFunction(MHBinCompare);
	oaBins->Sort();
}

int MHBinCompare(const void* elem1, const void* elem2)
{
	MHBin* bin1;
	MHBin* bin2;

	bin1 = cast(MHBin*, *(Object**)elem1);
	bin2 = cast(MHBin*, *(Object**)elem2);
	return KWContinuous::Compare(bin1->GetUpperValue(), bin2->GetUpperValue());
}