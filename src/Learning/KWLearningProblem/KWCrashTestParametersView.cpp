// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	AddIntField("CrashTestIOIndex", "Crash test - IO index", 1);
	AddIntField("CrashTestMaxLineLength", "Crash test - max line length", InputBufferedFile::GetMaxLineLength());
	AddIntField("CrashTestMaxSecondaryRecordNumber",
		    "Crash test - max secondary record number per multi-table instance",
		    int(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber()));
	AddIntField("CrashTestMaxCreatedRecordNumber", "Crash test - max created record number per instance",
		    int(KWDatabaseMemoryGuard::GetCrashTestMaxCreatedRecordNumber()));
	AddIntField("CrashTestMemoryGuardMemoryLimit", "Crash test - memory guard limit in MB",
		    int(KWDatabaseMemoryGuard::GetCrashTestMemoryLimit()));

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
	cast(UIIntElement*, GetFieldAt("CrashTestMaxCreatedRecordNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("CrashTestIOIndex"))->SetMinValue(1);
	cast(UIIntElement*, GetFieldAt("CrashTestIOIndex"))->SetDefaultValue(1);
	cast(UIIntElement*, GetFieldAt("CrashTestMemoryGuardMemoryLimit"))->SetMinValue(0);

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
	GetFieldAt("CrashTestIOIndex")->SetHelpText("IO index for crash test (expert).");
	GetFieldAt("CrashTestMaxLineLength")->SetHelpText("Max line length in input data files (expert).");
	GetFieldAt("CrashTestMaxSecondaryRecordNumber")
	    ->SetHelpText("Max record number per multi-table instance."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, a warning is issued.");
	GetFieldAt("CrashTestMaxCreatedRecordNumber")
	    ->SetHelpText("Max created record number per instance."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, a warning is issued.");
	GetFieldAt("CrashTestMemoryGuardMemoryLimit")
	    ->SetHelpText("Memory guard limit in MB."
			  "\n By default, this parameter is set to 0, meaning that the limit is not active."
			  "\n If the limit is exceeded, a warning is issued and the derived variables of the instance "
			  "are missing."
			  "\t In the case of missing memory for external tables, this results in an error.");
	GetActionAt("ResetParameters")->SetHelpText("Reset all crash parametres to their defaut value.");
}

KWCrashTestParametersView::~KWCrashTestParametersView() {}

void KWCrashTestParametersView::EventUpdate(Object* object)
{
	PLParallelTask::sCrashTestTaskName = GetStringValueAt("CrashTestTask");
	PLParallelTask::nCrashTestMethod = PLParallelTask::StringToMethod(GetStringValueAt("CrashTestMethod"));
	PLParallelTask::nCrashTestType = PLParallelTask::StringToCrashTest(GetStringValueAt("CrashTestType"));
	PLParallelTask::nCrashTestCallIndex = GetIntValueAt("CrashTestCallIndex");
	PLParallelTask::nCrashTestIOIndex = GetIntValueAt("CrashTestIOIndex");
	InputBufferedFile::SetMaxLineLength(GetIntValueAt("CrashTestMaxLineLength"));
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(GetIntValueAt("CrashTestMaxSecondaryRecordNumber"));
	KWDatabaseMemoryGuard::SetCrashTestMaxCreatedRecordNumber(GetIntValueAt("CrashTestMaxCreatedRecordNumber"));
	KWDatabaseMemoryGuard::SetCrashTestMemoryLimit(GetIntValueAt("CrashTestMemoryGuardMemoryLimit") * lMB);
}

void KWCrashTestParametersView::EventRefresh(Object* object)
{
	SetStringValueAt("CrashTestTask", PLParallelTask::sCrashTestTaskName);
	SetStringValueAt("CrashTestMethod", PLParallelTask::MethodToString(PLParallelTask::nCrashTestMethod));
	SetStringValueAt("CrashTestType", PLParallelTask::CrashTestToString(PLParallelTask::nCrashTestType));
	SetIntValueAt("CrashTestCallIndex", PLParallelTask::nCrashTestCallIndex);
	SetIntValueAt("CrashTestIOIndex", PLParallelTask::nCrashTestIOIndex);
	SetIntValueAt("CrashTestMaxLineLength", InputBufferedFile::GetMaxLineLength());
	SetIntValueAt("CrashTestMaxSecondaryRecordNumber",
		      int(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber()));
	SetIntValueAt("CrashTestMaxCreatedRecordNumber",
		      int(KWDatabaseMemoryGuard::GetCrashTestMaxCreatedRecordNumber()));
	SetIntValueAt("CrashTestMemoryGuardMemoryLimit", int(KWDatabaseMemoryGuard::GetCrashTestMemoryLimit() / lMB));
}

void KWCrashTestParametersView::ResetParameters()
{
	PLParallelTask::sCrashTestTaskName = "";
	PLParallelTask::nCrashTestMethod = PLParallelTask::NONE;
	PLParallelTask::nCrashTestType = PLParallelTask::NO_TEST;
	PLParallelTask::nCrashTestCallIndex = 1;
	PLParallelTask::nCrashTestIOIndex = 1;
	InputBufferedFile::SetMaxLineLength(8 * lMB);
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(0);
	KWDatabaseMemoryGuard::SetCrashTestMaxCreatedRecordNumber(0);
	KWDatabaseMemoryGuard::SetCrashTestMemoryLimit(0);
}

const ALString KWCrashTestParametersView::GetClassLabel() const
{
	return "System parameters";
}

void KWCrashTestParametersView::InitializeTaskCrashTestFields()
{
	StringVector svTaskNames;
	ALString sTaskList;
	ALString sMethodList;
	ALString sTestList;
	int i;

	// Initialisation de la liste des taches
	sTaskList = "\n";
	PLParallelTask::GetRegisteredTaskNames(svTaskNames);
	svTaskNames.Sort();
	for (i = 0; i < svTaskNames.GetSize(); i++)
	{
		if (i > 0)
			sTaskList += "\n";
		sTaskList += svTaskNames.GetAt(i);
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
