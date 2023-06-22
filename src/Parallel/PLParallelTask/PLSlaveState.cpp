// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSlaveState.h"

PLSlaveState::PLSlaveState()
{
	Initialize();
}

PLSlaveState::~PLSlaveState() {}

void PLSlaveState::Initialize()
{
	state = VOID;
	nRank = 0;
	nProgression = 0;
	dPercentOfTheJob = 0;
	bHasWorked = false;
	sHostName = "";
	bMustRest = false;
	nTaskIndex = -1;
}

void PLSlaveState::SetRank(int nValue)
{
	require(nValue >= 0);
	nRank = nValue;
}

int PLSlaveState::GetRank() const
{
	return nRank;
}

void PLSlaveState::SetHostName(ALString sValue)
{
	require(sValue != "");
	sHostName = sValue;
}

ALString PLSlaveState::GetHostName() const
{
	return sHostName;
}

void PLSlaveState::SetProgression(int nValue)
{
	require(0 <= nValue and nValue <= 100);
	nProgression = nValue;
}

void PLSlaveState::SetState(State nState)
{

	assert(VOID <= nState and nState < UNKNOWN);
	// cout << "STATE(" << nRank << ") " << PrintState() << " => " << GetStateAsString(nState) << endl;
	state = nState;
}

void PLSlaveState::SetAtRest(boolean bRest)
{
	bMustRest = bRest;
}
boolean PLSlaveState::GetAtRest() const
{
	return bMustRest;
}

void PLSlaveState::SetTaskIndex(int nIndex)
{
	require(nIndex >= 0);
	nTaskIndex = nIndex;
}

int PLSlaveState::GetTaskIndex() const
{
	return nTaskIndex;
}

void PLSlaveState::SetTaskPercent(double dPercent)
{
	assert(0 <= dPercent and dPercent <= 1);
	dPercentOfTheJob = dPercent;
}

double PLSlaveState::GetTaskPercent() const
{
	return dPercentOfTheJob;
}

const ALString PLSlaveState::PrintState() const
{
	return GetStateAsString(state);
}

const ALString PLSlaveState::GetStateAsString(State nState)
{
	ensure(VOID <= nState and nState <= UNKNOWN);

	switch (nState)
	{
	case VOID:
		return "VOID";
		break;
	case INIT:
		return "INIT";
		break;
	case READY:
		return "READY";
		break;
	case PROCESSING:
		return "PROCESSING";
		break;
	case FINALIZE:
		return "FINALIZE";
		break;
	case DONE:
		return "DONE";
		break;
	default:
		return "UNKNOWN";
		break;
	}
}

void PLSlaveState::Write(ostream& ost) const
{
	ost << nRank << " on " << sHostName << " " << PrintState() << " progression : " << IntToString(GetProgression())
	    << " Task% : " << DoubleToString(dPercentOfTheJob * 100);
	if (bMustRest)
		cout << " AT REST";
	cout << endl;
}
