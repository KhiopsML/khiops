// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSystemParametersView.h"

KWSystemParametersView::KWSystemParametersView()
{
	int nModalityNumber;
	int nErrorMessageNumber;
	int nMaxMemory;
	int nMinMemory;
	int nDefaultMemory;
	int nMaxProcNumber;
	ALString sMemoryLimit;
	longint lEnvMemoryLimit;
	int nRound;
	int i;

	SetIdentifier("KWSystemParameters");
	SetLabel("System parameters");
	AddIntField("MaxItemNumberInReports", "Max number of items in reports", 0);
	AddIntField("MaxErrorMessageNumberInLog", "Max number of error messages in log", 0);
	AddIntField("OptimizationTime", "Min optimization time in seconds", 0);
	AddIntField("MemoryLimit", "Memory limit in MB", 0);
	AddBooleanField("IgnoreMemoryLimit", "Ignore memory limit", false);
	AddIntField("MaxCoreNumber", "Max number of processor cores", 1);
	AddStringField("ParallelLogFileName", "Parallel log file", "");
	AddBooleanField("ParallelSimulated", "Parallel simulated mode", false);
	AddStringField("TemporaryDirectoryName", "Temp file directory", "");

	// Parametrage des styles
	GetFieldAt("MaxItemNumberInReports")->SetStyle("Spinner");
	GetFieldAt("MaxErrorMessageNumberInLog")->SetStyle("Spinner");
	GetFieldAt("OptimizationTime")->SetStyle("Spinner");
	GetFieldAt("MemoryLimit")->SetStyle("Slider");
	GetFieldAt("IgnoreMemoryLimit")->SetStyle("CheckBox");
	GetFieldAt("MaxCoreNumber")->SetStyle("Slider");
	GetFieldAt("ParallelLogFileName")->SetStyle("DirectoryChooser");
	GetFieldAt("ParallelSimulated")->SetStyle("CheckBox");
	GetFieldAt("TemporaryDirectoryName")->SetStyle("DirectoryChooser");

	// Parametrage des limites du nombre max d'items dans les rapports
	cast(UIIntElement*, GetFieldAt("MaxItemNumberInReports"))->SetMinValue(10);
	cast(UIIntElement*, GetFieldAt("MaxItemNumberInReports"))->SetMaxValue(1000000);
	nModalityNumber = KWLearningSpec::GetMaxModalityNumber();
	if (nModalityNumber < 10)
		nModalityNumber = 10;
	if (nModalityNumber > 1000000)
		nModalityNumber = 1000000;
	KWLearningSpec::SetMaxModalityNumber(nModalityNumber);
	cast(UIIntElement*, GetFieldAt("MaxItemNumberInReports"))
	    ->SetDefaultValue(KWLearningSpec::GetMaxModalityNumber());

	// Parametrage des limites du nombre max de messages d'erreur dans les log
	cast(UIIntElement*, GetFieldAt("MaxErrorMessageNumberInLog"))->SetMinValue(10);
	cast(UIIntElement*, GetFieldAt("MaxErrorMessageNumberInLog"))->SetMaxValue(10000);
	nErrorMessageNumber = Global::GetMaxErrorFlowNumber();
	if (nErrorMessageNumber < 10)
		nErrorMessageNumber = 10;
	if (nErrorMessageNumber > 10000)
		nErrorMessageNumber = 10000;
	Global::SetMaxErrorFlowNumber(nErrorMessageNumber);
	cast(UIIntElement*, GetFieldAt("MaxErrorMessageNumberInLog"))->SetDefaultValue(Global::GetMaxErrorFlowNumber());

	// Limites du temps d'optimisation
	RMResourceConstraints::SetOptimizationTime(0);
	cast(UIIntElement*, GetFieldAt("OptimizationTime"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("OptimizationTime"))->SetMaxValue(100000000);
	cast(UIIntElement*, GetFieldAt("OptimizationTime"))
	    ->SetDefaultValue(RMResourceConstraints::GetOptimizationTime());

	// Limites de la taille memoire en MB de chaque machine
	// Par defaut, on utilise 90% de la memoire max (notamment pour les machines n'ayant pas de fichier swap)
	// Sur un cluster c'est la max alloue a chaque machine (et non la memoire de tout le cluster)

	// Caclul du min avec un arrondi a la dizaine ou a la centaine selon la presence ou l'absence de la GUI java
	nMinMemory = int(RMResourceManager::GetSystemMemoryReserve() * (1 + MemGetAllocatorOverhead()) / lMB);
	if (UIObject::IsBatchMode())
		nRound = 10;
	else
		nRound = 100;
	nMinMemory = nRound * int(ceil(nMinMemory / (nRound * 1.0)));

	// Calcul du max : c'est la maximum de la memoire disponible sur chaque machine
	nMaxMemory = 0;
	for (i = 0; i < RMResourceManager::GetResourceSystem()->GetHostNumber(); i++)
	{
		const RMHostResource* hr = RMResourceManager::GetResourceSystem()->GetHostResourceAt(i);
		nMaxMemory = max(nMaxMemory, int(hr->GetPhysicalMemory() / lMB));
	}
	nMaxMemory = min(nMaxMemory, int(MemGetAdressablePhysicalMemory() / lMB));

	// Valeur par defaut de la memoire (variable d'environement ou 90% de le memoire max)
	sMemoryLimit = p_getenv("KHIOPS_MEMORY_LIMIT");
	if (sMemoryLimit == "")
	{
		sMemoryLimit = p_getenv("KhiopsMemoryLimit"); // DEPRECATED
		if (sMemoryLimit != "")
			AddWarning("'KhiopsMemoryLimit' is deprecated and is now replaced by 'KHIOPS_MEMORY_LIMIT'");
	}
	lEnvMemoryLimit = 0;
	if (sMemoryLimit != "")
	{
		// Utilisation de StringToLongint car atol renvoie 0 en cas de pbm (contrairement a atoi)
		lEnvMemoryLimit = StringToLongint(sMemoryLimit);
	}
	if (lEnvMemoryLimit != 0 and lEnvMemoryLimit <= nMaxMemory and lEnvMemoryLimit >= nMinMemory)
	{
		nMaxMemory = (int)lEnvMemoryLimit;
		nDefaultMemory = (int)lEnvMemoryLimit;
	}
	else
		nDefaultMemory = nMaxMemory - nMaxMemory / 10;

	cast(UIIntElement*, GetFieldAt("MemoryLimit"))->SetMinValue(nMinMemory);
	cast(UIIntElement*, GetFieldAt("MemoryLimit"))->SetMaxValue(nMaxMemory);
	cast(UIIntElement*, GetFieldAt("MemoryLimit"))->SetDefaultValue(nDefaultMemory);
	RMResourceConstraints::SetMemoryLimit(nDefaultMemory);

	// Ignorer les limites memoire
	SetBooleanValueAt("IgnoreMemoryLimit", RMResourceConstraints::GetIgnoreMemoryLimit());

	// Limites du nombre de coeurs
	cast(UIIntElement*, GetFieldAt("MaxCoreNumber"))->SetMinValue(1);

	nMaxProcNumber =
	    min(RMResourceManager::GetPhysicalCoreNumber(), max(1, RMResourceManager::GetLogicalProcessNumber() - 1));
	cast(UIIntElement*, GetFieldAt("MaxCoreNumber"))->SetMaxValue(nMaxProcNumber);
	cast(UIIntElement*, GetFieldAt("MaxCoreNumber"))->SetDefaultValue(nMaxProcNumber);

	// Calcul du nombre effectif de processus qu'on utilise (en general DefaultValue +1)
	RMResourceConstraints::SetMaxCoreNumberOnCluster(
	    ComputeCoreNumber(cast(UIIntElement*, GetFieldAt("MaxCoreNumber"))->GetDefaultValue()));

	// On ne peut pas editer le nombre de process a utiliser le mode parallel n'est pas disponible
	if (not PLParallelTask::IsParallelModeAvailable())
		GetFieldAt("MaxCoreNumber")->SetEditable(false);

	// Les parametrages de l'optimisation du temps et du mode risque de gestion de la memoire ne sont pas visibles
	// par defaut
	GetFieldAt("OptimizationTime")->SetVisible(false);
	GetFieldAt("IgnoreMemoryLimit")->SetVisible(false);

	// Mode parallele simule uniquement en mode expert
	if (not GetParallelExpertMode())
		GetFieldAt("ParallelSimulated")->SetVisible(false);

	// Fonctionnalites avancees  en mode expert parallele
	if (not GetParallelExpertMode())
		GetFieldAt("ParallelLogFileName")->SetVisible(false);

	// Info-bulles
	GetFieldAt("MaxItemNumberInReports")
	    ->SetHelpText("Max number of items in reports."
			  "\n Allows to control the size of reports, by limiting the number of reported items,"
			  "\n such as the number of lines or rows in contingency tables, the number of"
			  "\n detailed groups of values or the number of values in each detailed group.");
	GetFieldAt("MaxErrorMessageNumberInLog")
	    ->SetHelpText(
		"Max number of error messages in log."
		"\n Allows to control the size of the log, by limiting the number of messages, warning or errors.");
	GetFieldAt("OptimizationTime")
	    ->SetHelpText("Min optimization in seconds."
			  "\n Allows to specify the min amount of time for the optimization algorithms."
			  "\n By default, this parameter is 0 and the algorithm stops by itself when no significant "
			  "improvement is expected."
			  "\n Otherwise, the optimization is performed at least as long as specified, then stops after "
			  "the next built solution.");
	GetFieldAt("MemoryLimit")
	    ->SetHelpText("Memory limit in MB."
			  "\n Allows to specify the max amount of memory available for the data analysis algorithms."
			  "\n By default, this parameter is set to the limit of the available RAM."
			  "\n This parameter can be decreased in order to keep memory for the other applications.");
	GetFieldAt("IgnoreMemoryLimit")
	    ->SetHelpText("Ignore memory limits."
			  "\n Expert only: risky setting.");
	if (PLParallelTask::IsParallelModeAvailable())
		GetFieldAt("MaxCoreNumber")->SetHelpText("Max number of processor cores to use.");
	else
		GetFieldAt("MaxCoreNumber")
		    ->SetHelpText("Number of processor cores."
				  "\n Tasks cannot run in parallel in this configuration.");
	GetFieldAt("ParallelSimulated")->SetHelpText("Emulation of parallel mode for debug (expert).");
	GetFieldAt("ParallelLogFileName")->SetHelpText("Parallel log file name (expert).");
	GetFieldAt("TemporaryDirectoryName")
	    ->SetHelpText("Name of the directory to use for temporary files."
			  "\n Default: none, the system default temp file directory is then used.");
}

KWSystemParametersView::~KWSystemParametersView() {}

void KWSystemParametersView::EventUpdate(Object* object)
{
	int nRequestedCore;
	ALString sTestFunctionality;

	// On parametre directement les variables statiques correspondantes
	// en ignorant l'objet passe en parametres
	KWLearningSpec::SetMaxModalityNumber(GetIntValueAt("MaxItemNumberInReports"));
	Global::SetMaxErrorFlowNumber(GetIntValueAt("MaxErrorMessageNumberInLog"));
	RMResourceConstraints::SetOptimizationTime(GetIntValueAt("OptimizationTime"));
	RMResourceConstraints::SetMemoryLimit(GetIntValueAt("MemoryLimit"));
	RMResourceConstraints::SetIgnoreMemoryLimit(GetBooleanValueAt("IgnoreMemoryLimit"));

	// Calcul du nombre de processus utilises
	nRequestedCore = GetIntValueAt("MaxCoreNumber");
	RMResourceConstraints::SetMaxCoreNumberOnCluster(ComputeCoreNumber(nRequestedCore));
	PLParallelTask::SetParallelSimulated(GetBooleanValueAt("ParallelSimulated"));
	PLParallelTask::SetParallelLogFileName(GetStringValueAt("ParallelLogFileName"));
	FileService::SetUserTmpDir(GetStringValueAt("TemporaryDirectoryName"));
}

void KWSystemParametersView::EventRefresh(Object* object)
{
	// On parametre directement les variables statiques correspondantes
	// en ignorant l'objet passe en parametres
	SetIntValueAt("MaxItemNumberInReports", KWLearningSpec::GetMaxModalityNumber());
	SetIntValueAt("MaxErrorMessageNumberInLog", Global::GetMaxErrorFlowNumber());
	SetIntValueAt("OptimizationTime", RMResourceConstraints::GetOptimizationTime());
	SetIntValueAt("MemoryLimit", RMResourceConstraints::GetMemoryLimit());
	SetBooleanValueAt("IgnoreMemoryLimit", RMResourceConstraints::GetIgnoreMemoryLimit());
	SetIntValueAt("MaxCoreNumber", ComputeRequestedCoreNumber(RMResourceConstraints::GetMaxCoreNumberOnCluster()));
	SetBooleanValueAt("ParallelSimulated", PLParallelTask::GetParallelSimulated());
	SetStringValueAt("ParallelLogFileName", PLParallelTask::GetParallelLogFileName());
	SetStringValueAt("TemporaryDirectoryName", FileService::GetUserTmpDir());
}

const ALString KWSystemParametersView::GetClassLabel() const
{
	return "System parameters";
}

int KWSystemParametersView::ComputeCoreNumber(int nRequestedCoreNumber) const
{
	int nMPIProcessNumber;
	int nProcessNumber;

	require(nRequestedCoreNumber > 0);

	nMPIProcessNumber = RMResourceManager::GetLogicalProcessNumber();

	// Evaluation du nombre de processus a lancer
	if (nRequestedCoreNumber == 1)
	{
		// Lancement en sequentiel si l'utilisateur ne demande qu'un seul coeur
		nProcessNumber = 1;
	}
	else
	{
		// En parallele, on utilise 1 processus de plus que ce que demande l'utilisateur.
		// Et on n'utilise pas plus de processus qu'il n'y a de processus MPI (d'ou le min)
		// Il est donc recommande de lancer Khiops avec 1 processus MPI de plus que le nombre
		// de coeurs physiques
		nProcessNumber = min(nRequestedCoreNumber + 1, nMPIProcessNumber);
	}
	ensure(nProcessNumber > 0);
	return nProcessNumber;
}

int KWSystemParametersView::ComputeRequestedCoreNumber(int nCoreNumber) const
{
	int nRequestedCore;

	require(nCoreNumber > 0);

	if (nCoreNumber == 1)
		nRequestedCore = 1;
	else
		nRequestedCore = nCoreNumber - 1;
	return nRequestedCore;
}
