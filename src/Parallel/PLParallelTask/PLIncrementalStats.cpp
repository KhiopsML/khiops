// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLIncrementalStats.h"

PLIncrementalStats::PLIncrementalStats()
{
	Reset();
}

PLIncrementalStats::~PLIncrementalStats() {}

void PLIncrementalStats::SetDescription(const ALString sName)
{
	sDescription = sName;
}

void PLIncrementalStats::AddValue(double dValue)
{
	nValueNumber++;
	dTotal += dValue;
	dTotalSquare = dTotalSquare + dValue * dValue;
	if (dValue < dMin)
		dMin = dValue;
	if (dValue > dMax)
		dMax = dValue;
}

void PLIncrementalStats::AddStats(const PLIncrementalStats* stats)
{
	nValueNumber += stats->nValueNumber;
	if (dMax < stats->dMax)
		dMax = stats->dMax;
	if (dMin > stats->dMin)
		dMin = stats->dMin;
	dTotal += stats->dTotal;
	dTotalSquare += stats->dTotalSquare;
}

double PLIncrementalStats::GetMin() const
{
	return dMin;
}
double PLIncrementalStats::GetMax() const
{
	return dMax;
}
double PLIncrementalStats::GetMean() const
{
	require(nValueNumber != 0);
	return dTotal / nValueNumber;
}
double PLIncrementalStats::GetStdDev() const
{
	// La soustraction peut etre negative pour des problemes d'arrondi
	if (dTotalSquare / nValueNumber - GetMean() * GetMean() < 0)
		return 0;

	return sqrt(dTotalSquare / nValueNumber - GetMean() * GetMean());
}

double PLIncrementalStats::GetSum() const
{
	return dTotal;
}

int PLIncrementalStats::GetValueNumber() const
{
	return nValueNumber;
}

void PLIncrementalStats::Write(ostream& ost) const
{
	if (nValueNumber == 0)
	{
		ost << sDescription << " no data";
	}
	else
	{
		ost << sDescription << " mean: " << DoubleToString(ToTwoDigits(GetMean())) << " +-"
		    << DoubleToString(ToTwoDigits(GetStdDev()));
		if (GetStdDev() != 0)
			ost << " [" << DoubleToString(ToTwoDigits(dMin)) << " ; " << DoubleToString(ToTwoDigits(dMax))
			    << "]";
		ost << " sum: " << DoubleToString(dTotal);
	}
}

void PLIncrementalStats::Reset()
{
	dMin = DBL_MAX;
	dMax = -DBL_MAX;
	dTotal = 0;
	nValueNumber = 0;
	dTotalSquare = 0;
}

double PLIncrementalStats::ToTwoDigits(double dValue) const
{
	int nValue;
	nValue = (int)(dValue * 100);
	return ((double)nValue) / 100;
}

//////////////////////////////////////////////////////////////////
///// Implementation de la classe PLSharedIncrementalStats

PLSharedIncrementalStats::PLSharedIncrementalStats() {}
PLSharedIncrementalStats::~PLSharedIncrementalStats() {}

void PLSharedIncrementalStats::SetStats(PLIncrementalStats* stat)
{
	SetObject(stat);
}

PLIncrementalStats* PLSharedIncrementalStats::GetStats()
{
	return cast(PLIncrementalStats*, GetObject());
}

void PLSharedIncrementalStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLIncrementalStats* stat = cast(PLIncrementalStats*, o);

	require(serializer->IsOpenForWrite());

	serializer->PutDouble(stat->dMin);
	serializer->PutDouble(stat->dMax);
	serializer->PutDouble(stat->dTotal);
	serializer->PutInt(stat->nValueNumber);
	serializer->PutDouble(stat->dTotalSquare);
	serializer->PutString(stat->sDescription);
}

void PLSharedIncrementalStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLIncrementalStats* stat = cast(PLIncrementalStats*, o);

	require(serializer->IsOpenForRead());

	stat->dMin = serializer->GetDouble();
	stat->dMax = serializer->GetDouble();
	stat->dTotal = serializer->GetDouble();
	stat->nValueNumber = serializer->GetInt();
	stat->dTotalSquare = serializer->GetDouble();
	stat->sDescription = serializer->GetString();
}

Object* PLSharedIncrementalStats::Create() const
{
	return new PLIncrementalStats;
}