// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "JsonObject.h"

/////////////////////////////////////////////
// Classe JsonObject

JsonObject::JsonObject() {}

JsonObject::~JsonObject() {}

boolean JsonObject::WriteFile(const ALString& sFileName) const
{
	//DDD NOT YET IMPLEMENTED
	return true;
}

void JsonObject::TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName)
{
	JsonObject testJsonObject;

	require(sReadFileName != "");
	require(sWriteFileName != "");

	testJsonObject.ReadFile(sReadFileName);
}

void JsonObject::Test()
{
	TestReadWrite("C:\\temp\\Datasets\\Iris\\AnalysisResults.khj",
		      "C:\\temp\\Datasets\\Iris\\O_AnalysisResults.khj");
}
