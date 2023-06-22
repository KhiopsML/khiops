// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ProfileStats.h"

ProfileStats::ProfileStats()
{
	nCallNumber = 0;
	nStartLevel = 0;
}

ProfileStats::~ProfileStats() {}

void ProfileStats::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString& ProfileStats::GetLabel() const
{
	return sLabel;
}

void ProfileStats::Reset()
{
	timer.Reset();
	nCallNumber = 0;
	nStartLevel = 0;
}

void ProfileStats::WriteHeaderLineReport(ostream& ost)
{
	ost << "Label"
	    << "\t";
	ost << "Calls"
	    << "\t";
	ost << "Time";
}

void ProfileStats::WriteLineReport(ostream& ost)
{
	ost << sLabel << "\t";
	ost << nCallNumber << "\t";
	ost << GetElapsedTime();
}

void ProfileStats::WriteArrayLineReportFile(const ALString& sFileName, const ALString& sContextLabel,
					    ObjectArray* oaProfileStats)
{
	fstream ost;
	boolean bOk;

	bOk = FileService::OpenOutputFile(sFileName, ost);
	if (bOk)
	{
		WriteArrayLineReport(ost, sContextLabel, oaProfileStats);
		bOk = FileService::CloseOutputFile(sFileName, ost);

		// Destruction du rapport si erreur
		if (not bOk)
			FileService::RemoveFile(sFileName);
	}
}

void ProfileStats::WriteArrayLineReport(ostream& ost, const ALString& sContextLabel, ObjectArray* oaProfileStats)
{
	ProfileStats* profileStats;
	int n;

	require(oaProfileStats != NULL);

	// Parcours des elements du tableau
	for (n = 0; n < oaProfileStats->GetSize(); n++)
	{
		profileStats = cast(ProfileStats*, oaProfileStats->GetAt(n));
		if (n == 0)
		{
			ost << "Context\t";
			profileStats->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		ost << sContextLabel << "\t";
		profileStats->WriteLineReport(ost);
		ost << "\n";
	}
}

void ProfileStats::ResetAll(ObjectArray* oaProfileStats)
{
	ProfileStats* profileStats;
	int n;

	require(oaProfileStats != NULL);

	// Parcours des elements du tableau
	for (n = 0; n < oaProfileStats->GetSize(); n++)
	{
		profileStats = cast(ProfileStats*, oaProfileStats->GetAt(n));
		profileStats->Reset();
	}
}