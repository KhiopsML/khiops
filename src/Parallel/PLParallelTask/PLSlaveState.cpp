// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSlaveState.h"

const ALString PLSlaveState::sVOID = "VOID";
const ALString PLSlaveState::sINIT = "INIT";
const ALString PLSlaveState::sREADY = "READY";
const ALString PLSlaveState::sPROCESSING = "PROCESSING";
const ALString PLSlaveState::sFINALIZE = "FINALIZE";
const ALString PLSlaveState::sDONE = "DONE";
const ALString PLSlaveState::sUNKNOW = "UNKNOW";

PLSlaveState::PLSlaveState()
{
	Initialize();
}

PLSlaveState::~PLSlaveState() {}

void PLSlaveState::Initialize()
{
	state = State::VOID;
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

const ALString& PLSlaveState::PrintState() const
{
	return GetStateAsString(state);
}

const ALString& PLSlaveState::GetStateAsString(State nState)
{

	switch (nState)
	{
	case State::VOID:
		return sVOID;
	case State::INIT:
		return sINIT;
	case State::READY:
		return sREADY;
	case State::PROCESSING:
		return sPROCESSING;
	case State::FINALIZE:
		return sFINALIZE;
	case State::DONE:
		return sDONE;
	case State::UNKNOW:
		return sUNKNOW;
	default:
		return sUNKNOW;
	}
}

State PLSlaveState::IntToState(int nState)
{
	switch (nState)
	{
	case 0:
		return State::VOID;
	case 1:
		return State::INIT;
	case 2:
		return State::READY;
	case 3:
		return State::PROCESSING;
	case 4:
		return State::FINALIZE;
	case 5:
		return State::DONE;
	case 6:
		return State::UNKNOW;
	default:
		return State::UNKNOW;
	}
}
int PLSlaveState::StateToInt(State nState)
{
	switch (nState)
	{
	case State::VOID:
		return 0;
	case State::INIT:
		return 1;
	case State::READY:
		return 2;
	case State::PROCESSING:
		return 3;
	case State::FINALIZE:
		return 4;
	case State::DONE:
		return 5;
	case State::UNKNOW:
		return 6;
	default:
		return 6;
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
