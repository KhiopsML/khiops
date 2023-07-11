// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPITracer.h"

PLMPITracer::PLMPITracer()
{
	bShortDescription = false;
}

PLMPITracer::~PLMPITracer() {}

void PLMPITracer::AddSend(int nReceiverRank, int tag)
{
	ALString sTmp;
	if (GetShortDescription())
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " S " + IntToString(nReceiverRank) + " : " +
				 IntToString(tag));
	else
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " SEND " + GetTagAsString(tag) + " to " +
				 IntToString(nReceiverRank));
}

void PLMPITracer::AddBroadcast(int tag)
{
	ALString sTmp;
	if (GetShortDescription())
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " B " + " " + IntToString(tag));
	else
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " BCAST - " + GetTagAsString(tag));
}

void PLMPITracer::AddRecv(int source, int tag)
{
	ALString sTmp;
	if (GetShortDescription())
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " R " + IntToString(source) + " : " +
				 IntToString(tag));
	else
		AddTraceAsString(sTmp + IntToString(GetProcessId()) + " RECV " + GetTagAsString(tag) + " from " +
				 IntToString(source));
}
