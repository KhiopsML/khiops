// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWCrashTestParametersView.h"

KWCrashTestParametersView::KWCrashTestParametersView()
{
	SetIdentifier("KWCrashTestParameters");
	SetLabel("Crash test parameters");
	AddStringField("CrashTestTask", "Crash test - task", "");
	AddStringField("CrashTestType", "Crash test - type", "");
	AddStringField("CrashTestMethod", "Crash test - method", "");
	AddIntField("CrashTestCallIndex", "Crash test - call index", 1);
	AddIntField("CrashTestMaxLineLength", "Crash test - max line length", InputBufferedFile::GetMaxLineLength());
	AddIntField("CrashTestMaxSecondaryRecordNumber",
		    "Crash test - max secondary record number per multi-table instance",
		    int(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber()));
	AddIntField("CrashTestSingleInstanceMemoryLimit", "Crash test - memory limit per multi-table instance in MB",
		    int(KWDatabaseMemoryGuard::GetCrashTestSingleInstanceMemoryLimit()));
	AddIntField("CrashTestMaxHeapSizePerCore", "Crash test - memory limit per core in MB",
		    int(MemGetMaxHeapSize() / lMB));

	// Gestion des parametre des crash test
	GetFieldAt("CrashTestTask")->SetStyle("ComboBox");
	GetFieldAt("CrashTestType")->SetStyle("ComboBox");
	GetFieldAt("CrashTestMethod")->SetStyle("ComboBox");
	cast(UIIntElement*, GetFieldAt("CrashTestCallIndex"))->SetMinValue(1);
	cast(UIIntElement*, GetFieldAt("CrashTestCallIndex"))->SetDefaultValue(1);
	cast(UIIntElement*, GetFieldAt("CrashTestMaxLineLength"))->SetMinValue(1);
	cast(UIIntElement*, GetFieldAt("CrashTestMaxLineLength"))
	    ->SetDefaultValue(InputBufferedFile::GetMaxLineLength());
	cast(UIIntElement*, GetFieldAt("CrashTestMaxSecondaryRecordNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("CrashTestSingleInstanceMemoryLimit"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("CrashTestMaxHeapSizePerCore"))->SetMinValue(0);

	// Initialisation des valeurs des parametres de crash concernant les taches
	InitializeTaskCrashTestFields();

	// Ajout des actions
	AddAction("ResetParameters", "Reset parameters", (ActionMethod)(&KWCrashTestParametersView::ResetParameters));
	cast(UIAction*, GetActionAt("ResetParameters"))->SetStyle("Button");

	// Info-bulles
	GetFieldAt("CrashTestTask")->SetHelpText("Name of task for crash test (expert).");
	GetFieldAt("CrashTestType")->SetHelpText("Type of crash for crash test (expert).");
	GetFieldAt("CrashTestMethod")->SetHelpText("Name of task method for crash test (expert).");
	GetFieldAt("CrashTestCallIndex")->SetHelpText("Call index of task method for crash test (expert).");
	GetFieldAt("CrashTestMaxLineLength")->SetHelpText("Max line length in input data files (expert).");
	GetFieldAt("CrashTestMaxSecondaryRecordNumber")
	    ->SetHelpText("Max record number per multi-table instance."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, a warning is issued.");
	GetFieldAt("CrashTestSingleInstanceMemoryLimit")
	    ->SetHelpText("Memory limit per multi-table instance in MB."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, an error is issued and the derived variables of the instance "
			  "are missing.");
	GetFieldAt("CrashTestMaxHeapSizePerCore")
	    ->SetHelpText("Memory limit per core in MB."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, the program ends in a fatal error.");
	GetActionAt("ResetParameters")->SetHelpText("Reset all crash parametres to their defaut value.");
}

KWCrashTestParametersView::~KWCrashTestParametersView() {}

void KWCrashTestParametersView::EventUpdate(Object* object)
{
	PLParallelTask::sCrashTestTaskSignature = GetStringValueAt("CrashTestTask");
	PLParallelTask::nCrashTestMethod = PLParallelTask::StringToMethod(GetStringValueAt("CrashTestMethod"));
	PLParallelTask::nCrashTestType = PLParallelTask::StringToCrashTest(GetStringValueAt("CrashTestType"));
	PLParallelTask::nCrashTestCallIndex = GetIntValueAt("CrashTestCallIndex");
	InputBufferedFile::SetMaxLineLength(GetIntValueAt("CrashTestMaxLineLength"));
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(GetIntValueAt("CrashTestMaxSecondaryRecordNumber"));
	KWDatabaseMemoryGuard::SetCrashTestSingleInstanceMemoryLimit(
	    GetIntValueAt("CrashTestSingleInstanceMemoryLimit") * lMB);
	MemSetMaxHeapSize(GetIntValueAt("CrashTestMaxHeapSizePerCore") * lMB);
}

void KWCrashTestParametersView::EventRefresh(Object* object)
{
	SetStringValueAt("CrashTestTask", PLParallelTask::sCrashTestTaskSignature);
	SetStringValueAt("CrashTestMethod", PLParallelTask::MethodToString(PLParallelTask::nCrashTestMethod));
	SetStringValueAt("CrashTestType", PLParallelTask::CrashTestToString(PLParallelTask::nCrashTestType));
	SetIntValueAt("CrashTestCallIndex", PLParallelTask::nCrashTestCallIndex);
	SetIntValueAt("CrashTestMaxLineLength", InputBufferedFile::GetMaxLineLength());
	SetIntValueAt("CrashTestMaxSecondaryRecordNumber",
		      int(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber()));
	SetIntValueAt("CrashTestSingleInstanceMemoryLimit",
		      int(KWDatabaseMemoryGuard::GetCrashTestSingleInstanceMemoryLimit() / lMB));
	SetIntValueAt("CrashTestMaxHeapSizePerCore", int(MemGetMaxHeapSize() / lMB));
}

void KWCrashTestParametersView::ResetParameters()
{
	PLParallelTask::sCrashTestTaskSignature = "";
	PLParallelTask::nCrashTestMethod = PLParallelTask::NONE;
	PLParallelTask::nCrashTestType = PLParallelTask::NO_TEST;
	PLParallelTask::nCrashTestCallIndex = 1;
	InputBufferedFile::SetMaxLineLength(8 * lMB);
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(0);
	KWDatabaseMemoryGuard::SetCrashTestSingleInstanceMemoryLimit(0);
	MemSetMaxHeapSize(0);
}

const ALString KWCrashTestParametersView::GetClassLabel() const
{
	return "System parameters";
}

void KWCrashTestParametersView::InitializeTaskCrashTestFields()
{
	StringVector svTaskSignatures;
	ALString sTaskList;
	ALString sMethodList;
	ALString sTestList;
	int i;

	// Initialisation de la liste des taches
	sTaskList = "\n";
	PLParallelTask::GetRegisteredTaskSignatures(svTaskSignatures);
	svTaskSignatures.Sort();
	for (i = 0; i < svTaskSignatures.GetSize(); i++)
	{
		if (i > 0)
			sTaskList += "\n";
		sTaskList += svTaskSignatures.GetAt(i);
	}
	GetFieldAt("CrashTestTask")->SetParameters(sTaskList);

	// Initialisation de la liste des tests
	for (i = 0; i < PLParallelTask::TestType::TESTS_NUMBER; i++)
	{
		if (i > 0)
			sTestList += "\n";
		sTestList += PLParallelTask::CrashTestToString((PLParallelTask::TestType)i);
	}
	GetFieldAt("CrashTestType")->SetParameters(sTestList);

	// Initialisation de la liste des methodes
	for (i = 0; i < PLParallelTask::Method::METHODS_NUMBER; i++)
	{
		if (i > 0)
			sMethodList += "\n";
		sMethodList += PLParallelTask::MethodToString((PLParallelTask::Method)i);
	}
	GetFieldAt("CrashTestMethod")->SetParameters(sMethodList);
}