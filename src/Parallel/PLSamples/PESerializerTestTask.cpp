// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PESerializerTestTask.h"

PESerializerTestTask::PESerializerTestTask()
{
	DeclareTaskInput(&input_sString);
	DeclareTaskInput(&input_sEmptyString);
	DeclareTaskInput(&input_sLargeString);
	DeclareTaskInput(&input_cvCharVector);
	DeclareTaskInput(&input_cvLargeCharVector);
	DeclareTaskInput(&input_cvEmptyCharVector);
	DeclareTaskInput(&input_ivIntVector);
	DeclareTaskInput(&input_char);

	DeclareSharedParameter(&shared_nLargeCharVectorSize);
	DeclareSharedParameter(&shared_nLargeStringSize);

	nLargeCharVectorSize = 0;
	nLargeStringSize = 0;
	sSimpleStringToSerialize = "A simple string";
	sSimpleCharVectorToSerialize = "A simple CharVector !!";
}

PESerializerTestTask::~PESerializerTestTask() {}

boolean PESerializerTestTask::TestSerializer(int nLargeCharVectorSizeValue, int nLargeStringSizeValue)
{
	boolean bOk;

	// Parametres
	this->nLargeCharVectorSize = nLargeStringSizeValue;
	this->nLargeStringSize = nLargeStringSizeValue;
	bOk = Run();
	return bOk;
}

void PESerializerTestTask::Test()
{
	PESerializerTestTask task;
	boolean bOk;
	bOk = task.TestSerializer(1, 1);
	bOk = task.TestSerializer(0, 0);
	bOk = task.TestSerializer(MemSegmentByteSize, MemSegmentByteSize);
	bOk = task.TestSerializer(2 * MemSegmentByteSize + 3, 2 * MemSegmentByteSize - 1);
	bOk = task.TestSerializer(2 * MemSegmentByteSize - 3, 2 * MemSegmentByteSize + 1);
	if (not bOk)
		cout << "Test failed" << endl;
}

const ALString PESerializerTestTask::GetTaskName() const
{
	return "Serializer Test Task";
}

PLParallelTask* PESerializerTestTask::Create() const
{
	return new PESerializerTestTask;
}

boolean PESerializerTestTask::MasterInitialize()
{
	shared_nLargeCharVectorSize = nLargeCharVectorSize;
	shared_nLargeStringSize = nLargeStringSize;
	return true;
}

boolean PESerializerTestTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	int i;
	ALString sLargeString;
	CharVector cvCharVector;
	CharVector cvLargeCharVector;
	CharVector cvEmptyCharVector;
	IntVector ivIntvector;

	// String Simple
	input_sString.SetValue(sSimpleStringToSerialize);

	// Grande String
	for (i = 0; i < nLargeStringSize; i++)
	{
		sLargeString += "b";
	}
	input_sLargeString.SetValue(sLargeString);

	// CharVector
	for (i = 0; i < sSimpleCharVectorToSerialize.GetLength(); i++)
	{
		cvCharVector.Add(sSimpleCharVectorToSerialize.GetAt(i));
	}
	input_cvCharVector.GetCharVector()->CopyFrom(&cvCharVector);

	// Grand CharVector
	cvLargeCharVector.SetSize(nLargeCharVectorSize);
	for (i = 0; i < nLargeCharVectorSize; i++)
	{
		cvLargeCharVector.SetAt(i, 'c');
	}
	input_cvLargeCharVector.GetCharVector()->CopyFrom(&cvLargeCharVector);

	// Char
	input_char.SetValue('a');

	// String Vide
	input_sEmptyString.SetValue("");

	// Char Vector vide
	input_cvEmptyCharVector.GetCharVector()->CopyFrom(&cvEmptyCharVector);

	// IntVector
	for (i = 0; i < 100000; i++)
	{
		ivIntvector.Add(i);
	}
	input_ivIntVector.GetIntVector()->CopyFrom(&ivIntvector);

	// On ne fait qu'une passe
	if (GetTaskIndex() == 1)
		bIsTaskFinished = true;
	return true;
}

boolean PESerializerTestTask::MasterAggregateResults()
{
	return true;
}

boolean PESerializerTestTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PESerializerTestTask::SlaveInitialize()
{
	return true;
}

boolean PESerializerTestTask::SlaveProcess()
{
	ALString sLargeString;
	ALString sSimpleString;
	CharVector* cvCharVector;
	const IntVector* ivIntvector;
	boolean bOk;
	int i;
	int nCount;
	char c;

	bOk = true;

	// String Simple
	sSimpleString = input_sString.GetValue();
	AddMessage("Get simple String : " + sSimpleString);
	bOk = sSimpleStringToSerialize == sSimpleString;
	assert(bOk);

	// Grande String
	if (bOk)
	{
		sLargeString = input_sLargeString.GetValue();
		bOk = sLargeString.GetLength() == shared_nLargeStringSize;
		assert(bOk);
	}

	if (bOk)
	{
		nCount = 0;
		for (i = 0; i < shared_nLargeStringSize; i++)
		{
			if (sLargeString.GetAt(i) == 'b')
				nCount++;
		}
		bOk = nCount == shared_nLargeStringSize;
		if (bOk)
			AddMessage("Receive Large string as expected");
		assert(bOk);
	}

	// CharVector
	cvCharVector = NULL;
	if (bOk)
	{
		cvCharVector = input_cvCharVector.GetCharVector();
		bOk = cvCharVector->GetSize() == sSimpleCharVectorToSerialize.GetLength();
		assert(bOk);
	}
	if (bOk and cvCharVector != NULL)
	{
		sLargeString = "";
		for (i = 0; i < cvCharVector->GetSize(); i++)
		{
			sLargeString += cvCharVector->GetAt(i);
		}
		AddMessage("Get simple CharVector : " + sLargeString);

		bOk = sLargeString == sSimpleCharVectorToSerialize;
		assert(bOk);
	}

	// Grand CharVector
	if (bOk)
	{
		cvCharVector = input_cvLargeCharVector.GetCharVector();
		bOk = cvCharVector->GetSize() == shared_nLargeCharVectorSize;
		assert(bOk);
	}
	if (bOk)
	{
		nCount = 0;
		for (i = 0; i < shared_nLargeCharVectorSize; i++)
		{
			if (cvCharVector->GetAt(i) == 'c')
				nCount++;
		}
		bOk = nCount == shared_nLargeCharVectorSize;
		if (bOk)
			AddMessage("Receive Large CharVector as expected");
		assert(bOk);
	}

	// Char

	if (bOk)
	{
		c = input_char.GetValue();
		bOk = c == 'a';
		if (bOk)
			AddMessage("Receive char as expected");
		assert(bOk);
	}

	// String Vide
	if (bOk)
	{
		sSimpleString = input_sEmptyString.GetValue();
		bOk = sSimpleString == "";
		if (bOk)
			AddMessage("Receive empty string as expected");
		assert(bOk);
	}
	// Char Vector vide
	if (bOk)
	{
		cvCharVector = input_cvEmptyCharVector.GetCharVector();
		bOk = cvCharVector->GetSize() == 0;
		if (bOk)
			AddMessage("Receive empty CharVector as expected");
		assert(bOk);
	}
	if (bOk)
	{
		ivIntvector = input_ivIntVector.GetConstIntVector();
		bOk = input_ivIntVector.GetConstIntVector()->GetSize() == 100000;
		assert(bOk);
		for (i = 0; i < 100000; i++)
		{
			if (input_ivIntVector.GetConstIntVector()->GetAt(i) != i)
			{
				bOk = false;
				break;
			}
		}
		if (bOk)
			AddMessage("Receive intVector as expected");
		assert(bOk);
	}

	if (bOk)
		AddMessage("all tests have been successfully passed");
	else
		AddMessage("ERROR ERROR ERROR ERROR ERROR ERROR ERROR ");
	return bOk;
}

boolean PESerializerTestTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
