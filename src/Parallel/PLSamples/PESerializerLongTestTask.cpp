// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PESerializerLongTestTask.h"

PESerializerLongTestTask::PESerializerLongTestTask()
{
	DeclareTaskInput(&input_sString);
	DeclareTaskInput(&input_nStringLength);
	nMaxStringSize = 0;
}

PESerializerLongTestTask::~PESerializerLongTestTask() {}

boolean PESerializerLongTestTask::TestSerializer(int nStringLength)
{
	boolean bOk;

	require(nStringLength >= 0);

	// Parametres
	nMaxStringSize = nStringLength;
	bOk = Run();
	return bOk;
}

void PESerializerLongTestTask::Test()
{
	PESerializerLongTestTask task;
	boolean bOk;
	bOk = task.TestSerializer(2 * MemSegmentByteSize);
	if (not bOk)
		cout << "Test failed" << endl;
	else
		cout << "Test OK" << endl;
}

const ALString PESerializerLongTestTask::GetTaskName() const
{
	return "Serializer LongTest Task";
}

PLParallelTask* PESerializerLongTestTask::Create() const
{
	return new PESerializerLongTestTask;
}

boolean PESerializerLongTestTask::MasterInitialize()
{
	return true;
}

boolean PESerializerLongTestTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Serialisation de la string
	input_sString.SetValue(sStringToSerialize);
	input_nStringLength = sStringToSerialize.GetLength();

	// Ajout d'un caractere a la string pour le prochain MasterPrepareTaskInput
	sStringToSerialize += "s";

	// On termine quand la plus grande chaien a ete envoyee
	if (sStringToSerialize.GetLength() > nMaxStringSize)
		bIsTaskFinished = true;
	return true;
}

boolean PESerializerLongTestTask::MasterAggregateResults()
{
	return true;
}

boolean PESerializerLongTestTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PESerializerLongTestTask::SlaveInitialize()
{
	return true;
}

boolean PESerializerLongTestTask::SlaveProcess()
{
	boolean bOk;
	int i;
	ALString sTmp;

	sStringToSerialize = input_sString.GetValue();
	bOk = sStringToSerialize.GetLength() == input_nStringLength;
	assert(bOk);
	if (bOk)
	{
		for (i = 0; i < sStringToSerialize.GetLength(); i++)
		{
			if (sStringToSerialize.GetAt(i) != 's')
			{
				bOk = false;
				AddError(sTmp + "Wrong character at index " + IntToString(i));
				assert(false);
				break;
			}
		}
	}

	if (not bOk)
		AddError(sTmp + "Test failed for length " + IntToString(input_nStringLength));
	return bOk;
}

boolean PESerializerLongTestTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
