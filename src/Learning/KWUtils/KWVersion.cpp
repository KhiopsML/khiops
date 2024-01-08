// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWVersion.h"
#include "KWKhiopsVersion.h"

// Following defines added to compute the MemGetAdressablePhysicalMemory values
#define SIZE_OF_SIZE_T_32_BITS_SYSTEM 4
#define SIZE_OF_SIZE_T_64_BITS_SYSTEM 8

static ALString sKWLearningReportHeaderLine;
static boolean bKWLearningReporHeaderLineModified = false;
static ALString sKWLearningApplicationName = "Khiops";
static ALString sKWLearningModuleName;
static ALString sKWLearningAboutImage = "";
static ALString sKWLearningWebSite = "";
static ALString sKWLearningVersion;
static boolean bKWLearningVersionModified = false;

int GetMajorVersion(const ALString& sFullVersion)
{
	ALString sMajorVersion;
	int i;

	require(sFullVersion != "");
	require(isdigit(sFullVersion[0]));

	// Recherche du point de depart
	i = 0;
	while (i < sFullVersion.GetLength())
	{
		if (isdigit(sFullVersion.GetAt(i)))
			sMajorVersion += sFullVersion.GetAt(i);
		else
			break;
		i++;
	}
	assert(1 <= sMajorVersion.GetLength() and sMajorVersion.GetLength() <= 3);
	return StringToInt(sMajorVersion);
}

int GetMinorVersion(const ALString& sFullVersion)
{
	ALString sMinorVersion;
	int i;

	require(sFullVersion != "");
	require(isdigit(sFullVersion[0]));

	// Recherche du separateur .
	i = sFullVersion.Find('.');
	assert(i > 0);
	i++;
	while (i < sFullVersion.GetLength())
	{
		if (isdigit(sFullVersion.GetAt(i)))
			sMinorVersion += sFullVersion.GetAt(i);
		else
			break;
		i++;
	}
	assert(sMinorVersion.GetLength() <= 1);
	if (sMinorVersion == "")
		return 0;
	else
		return StringToInt(sMinorVersion);
}

const ALString GetLearningApplicationName()
{
	return sKWLearningApplicationName;
}

void SetLearningApplicationName(const ALString& sValue)
{
	sKWLearningApplicationName = sValue;
}

const ALString GetLearningModuleName()
{
	return sKWLearningModuleName;
}

void SetLearningModuleName(const ALString& sValue)
{
	sKWLearningModuleName = sValue;
}

const ALString GetLearningCommandName()
{
	ALString sCommandName;
	sCommandName = sKWLearningApplicationName;
	if (sKWLearningModuleName != "")
		sCommandName += "_" + sKWLearningModuleName;
	sCommandName.MakeLower();
	return sCommandName;
}

const ALString GetLearningFullApplicationName()
{
	if (sKWLearningModuleName == "")
		return sKWLearningApplicationName;
	else
		return sKWLearningApplicationName + " " + sKWLearningModuleName;
}

const ALString GetLearningVersion()
{
	if (not bKWLearningVersionModified)
		// On passe par la constante definie dans KWKhiopsVersion.h
		return KHIOPS_VERSION;
	else
		return sKWLearningVersion;
}

void SetLearningVersion(const ALString& sValue)
{
	require(sValue != "");
	sKWLearningVersion = sValue;
	bKWLearningVersionModified = true;
}

const ALString GetLearningCopyrightLabel()
{
	return KHIOPS_COPYRIGHT_LABEL;
}

const ALString GetLearningAboutImage()
{
	return sKWLearningAboutImage;
}

void SetLearningAboutImage(const ALString& sValue)
{
	sKWLearningAboutImage = sValue;
}

const ALString GetLearningWebSite()
{
	return sKWLearningWebSite;
}

void SetLearningWebSite(const ALString& sValue)
{
	sKWLearningWebSite = sValue;
}

const ALString GetLearningSystemType()
{
	// On utilise une variable locale pour eviter le warning "conditional expression is constant"
	int nSizeOfSizeT = sizeof(size_t);

	if (nSizeOfSizeT == SIZE_OF_SIZE_T_32_BITS_SYSTEM)
		return "(32-bit)";
	else
		return "(64-bit)";
}

const ALString GetLearningFullName()
{
	ALString sFullName;

	sFullName = GetLearningApplicationName();
	if (GetLearningModuleName() != "")
		sFullName += " " + GetLearningModuleName();
	sFullName += " " + GetLearningVersion();
	return sFullName;
}

const ALString GetLearningMainWindowTitle()
{
	ALString sTitle;

	sTitle = GetLearningApplicationName();
	if (GetLearningModuleName() != "")
		sTitle += " " + GetLearningModuleName();
	return sTitle;
}

const ALString GetLearningShellBanner()
{
	ALString sBanner;

	sBanner = GetLearningApplicationName();
	sBanner += " ";
	if (GetLearningModuleName() != "")
		sBanner += GetLearningModuleName() + " ";
	sBanner += GetLearningVersion();
	sBanner += " ";
	sBanner += GetLearningSystemType();
	sBanner += "\n   ";
	sBanner += GetLearningCopyrightLabel();
	return sBanner;
}

const ALString GetLearningReportHeaderLine()
{
	ALString sReportHeaderLine;

	// Valeur par defaut de la ligne d'entete
	if (not bKWLearningReporHeaderLineModified)
	{
		sReportHeaderLine = "#";
		sReportHeaderLine += GetLearningApplicationName();
		sReportHeaderLine += " ";
		sReportHeaderLine += GetLearningVersion();
	}
	// Valeur utilisateur
	else
		sReportHeaderLine = sKWLearningReportHeaderLine;
	return sReportHeaderLine;
}

void SetLearningReportHeaderLine(const ALString& sNewReportHeaderLine)
{
	sKWLearningReportHeaderLine = sNewReportHeaderLine;
	bKWLearningReporHeaderLineModified = true;
}

static boolean bLearningDefaultRawGuiModeMode = false;

void SetLearningDefaultRawGuiModeMode(boolean bValue)
{
	bLearningDefaultRawGuiModeMode = bValue;
}

boolean GetLearningDefaultRawGuiModeMode()
{
	return bLearningDefaultRawGuiModeMode;
}

boolean GetLearningRawGuiModeMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningRawGuiMode;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sLearningRawGuiMode;

		// Recherche des variables d'environnement
		sLearningRawGuiMode = p_getenv("KHIOPS_RAW_GUI");
		sLearningRawGuiMode.MakeLower();

		// Determination du mode d'interface
		if (sLearningRawGuiMode == "true")
			bLearningRawGuiMode = true;
		else if (sLearningRawGuiMode == "false")
			bLearningRawGuiMode = false;
		// Par defaut sinon
		else
			bLearningRawGuiMode = GetLearningDefaultRawGuiModeMode();

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningRawGuiMode;
}

boolean GetLearningMultiTableMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningMultiTableMode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sLearningMultiTableMode;

		// Recherche des variables d'environnement
		sLearningMultiTableMode = p_getenv("KhiopsMultiTableMode");
		sLearningMultiTableMode.MakeLower();

		// Determination du mode multi-tables
		if (sLearningMultiTableMode == "true")
			bLearningMultiTableMode = true;
		else if (sLearningMultiTableMode == "false")
			bLearningMultiTableMode = false;
		// Par defaut: true
		else
			bLearningMultiTableMode = true;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningMultiTableMode;
}

boolean GetLearningExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningExpertMode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningExpertMode;

		// Recherche des variables d'environnement
		sUserName = p_getenv("USERNAME");
		sUserName.MakeLower();
		sLearningExpertMode = p_getenv("KhiopsExpertMode");
		sLearningExpertMode.MakeLower();

		// Determination du mode expert (en debug pour Marc ou Carine)
		debug(bLearningExpertMode = (sUserName == "miib6422") or (sUserName == "mgtt5712"));
		if (sLearningExpertMode == "true")
			bLearningExpertMode = true;
		else if (sLearningExpertMode == "false")
			bLearningExpertMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningExpertMode;
}

boolean GetLearningCrashTestMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningCrashTestMode = false;

	// Determination du mode CrashTest au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningCrashTestMode;

		// Recherche des variables d'environnement
		sUserName = p_getenv("USERNAME");
		sUserName.MakeLower();
		sLearningCrashTestMode = p_getenv("KhiopsCrashTestMode");
		sLearningCrashTestMode.MakeLower();

		// Determination du mode CrashTest (en debug pour Marc ou Carine)
		debug(bLearningCrashTestMode = (sUserName == "miib6422") or (sUserName == "mgtt5712"));
		if (sLearningCrashTestMode == "true")
			bLearningCrashTestMode = true;
		else if (sLearningCrashTestMode == "false")
			bLearningCrashTestMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningCrashTestMode;
}

boolean GetLearningTextVariableMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningTextVariableMode = false;

	// Determination du mode au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningTextVariableMode;

		// Recherche des variables d'environnement
		sUserName = p_getenv("USERNAME");
		sUserName.MakeLower();
		sLearningTextVariableMode = p_getenv("KhiopsTextVariableMode");
		sLearningTextVariableMode.MakeLower();

		// Determination du mode (en debug)
		// DDD		debug(bLearningTextVariableMode = (sUserName == "miib6422"));
		if (sLearningTextVariableMode == "true")
			bLearningTextVariableMode = true;
		else if (sLearningTextVariableMode == "false")
			bLearningTextVariableMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningTextVariableMode;
}

boolean GetLearningInterpretationMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningInterpretationMode = false;

	// Determination du mode au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningInterpretationMode;

		// Recherche des variables d'environnement
		sUserName = p_getenv("USERNAME");
		sUserName.MakeLower();
		sLearningInterpretationMode = p_getenv("KhiopsInterpretationMode");
		sLearningInterpretationMode.MakeLower();

		// Determination du mode (en debug)
		// DDD 		debug(bLearningInterpretationMode = (sUserName == "miib6422"));
		if (sLearningInterpretationMode == "true")
			bLearningInterpretationMode = true;
		else if (sLearningInterpretationMode == "false")
			bLearningInterpretationMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningInterpretationMode;
}

boolean GetPreparationTraceMode()
{
	static boolean bIsInitialized = false;
	static boolean bPreparationTraceMode = false;

	// Determination du mode au premier appel
	if (not bIsInitialized)
	{
		ALString sPreparationTraceMode;

		// Recherche des variables d'environnement
		sPreparationTraceMode = p_getenv("KhiopsPreparationTraceMode");
		sPreparationTraceMode.MakeLower();

		// Determination du mode expert
		if (sPreparationTraceMode == "true")
			bPreparationTraceMode = true;
		else if (sPreparationTraceMode == "false")
			bPreparationTraceMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bPreparationTraceMode;
}

boolean GetIOTraceMode()
{
	static boolean bIsInitialized = false;
	static boolean bIOTraceMode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sIOTraceMode;

		// Recherche des variables d'environnement
		sIOTraceMode = p_getenv("KhiopsIOTraceMode");
		sIOTraceMode.MakeLower();

		// Determination du mode expert
		if (sIOTraceMode == "true")
			bIOTraceMode = true;
		else if (sIOTraceMode == "false")
			bIOTraceMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bIOTraceMode;
}

boolean GetForestExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bForestExpertMode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sForestExpertMode;

		// Recherche des variables d'environnement
		sForestExpertMode = p_getenv("KhiopsForestExpertMode");
		sForestExpertMode.MakeLower();

		// Determination du mode expert
		if (sForestExpertMode == "true")
			bForestExpertMode = true;
		else if (sForestExpertMode == "false")
			bForestExpertMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bForestExpertMode;
}

boolean GetForceSNBV9ExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bForceSNBV9Mode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sForceSNBV9Mode;

		// Recherche des variables d'environnement
		sForceSNBV9Mode = p_getenv("KhiopsForceSNBV9Mode");
		sForceSNBV9Mode.MakeLower();

		// Determination du mode expert
		if (sForceSNBV9Mode == "true")
			bForceSNBV9Mode = true;
		else if (sForceSNBV9Mode == "false")
			bForceSNBV9Mode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bForceSNBV9Mode;
}

boolean GetLearningCoclusteringExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningCoclusteringExpertMode = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningCoclusteringExpertMode;

		// Recherche des variables d'environnement
		sLearningCoclusteringExpertMode = p_getenv("KhiopsCoclusteringExpertMode");
		sLearningCoclusteringExpertMode.MakeLower();

		// Determination du mode expert
		if (sLearningCoclusteringExpertMode == "true")
			bLearningCoclusteringExpertMode = true;
		else if (sLearningCoclusteringExpertMode == "false")
			bLearningCoclusteringExpertMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bLearningCoclusteringExpertMode;
}

boolean GetParallelExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bParallelExpertMode = false;

	// Determination du mode parallele au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sExpertParallelMode;

		// Recherche des variables d'environnement
		sUserName = p_getenv("USERNAME");
		sUserName.MakeLower();
		sExpertParallelMode = p_getenv("KhiopsExpertParallelMode");
		sExpertParallelMode.MakeLower();

		// Determination du mode parallele
		if (sExpertParallelMode == "true")
			bParallelExpertMode = true;
		else if (sExpertParallelMode == "false")
			bParallelExpertMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bParallelExpertMode;
}

int GetParallelTraceMode()
{
	static boolean bIsInitialized = false;
	static int nParallelTraceMode = 0;
	int nValue;

	// Determination du mode parallele au premier appel
	if (not bIsInitialized)
	{
		ALString sParallelTraceMode;

		// Recherche des variables d'environnement
		sParallelTraceMode = p_getenv("KhiopsParallelTrace");

		// Transformation en entier
		sParallelTraceMode.TrimLeft();
		sParallelTraceMode.TrimRight();
		if (sParallelTraceMode.GetLength() > 1)
			nParallelTraceMode = 0;
		else
		{
			nValue = StringToInt(sParallelTraceMode);
			if (nValue > 0 and nValue < 4)
				nParallelTraceMode = nValue;
			else
				nParallelTraceMode = 0;
		}

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return nParallelTraceMode;
}

boolean GetLearningPriorStudyMode()
{
	static boolean bIsInitialized = false;
	static boolean bLearningPriorStudyMode = false;

	// Determination du mode etude des prior au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sLearningPriorStudyMode;

		// Recherche des variables d'environnement
		sLearningPriorStudyMode = p_getenv("KhiopsPriorStudyMode");
		sLearningPriorStudyMode.MakeLower();

		// Determination du mode etude des prior
		if (sLearningPriorStudyMode == "true")
			bLearningPriorStudyMode = true;
		else if (sLearningPriorStudyMode == "false")
			bLearningPriorStudyMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return GetLearningExpertMode() and bLearningPriorStudyMode;
}

boolean GetDistanceStudyMode()
{
	static boolean bIsInitialized = false;
	static boolean bDistanceStudyMode = false;

	// Determination du mode parallele au premier appel
	if (not bIsInitialized)
	{
		ALString sUserName;
		ALString sDistanceStudyMode;

		// Recherche des variables d'environnement
		sDistanceStudyMode = p_getenv("KhiopsDistanceStudyMode");
		sDistanceStudyMode.MakeLower();

		// Determination du mode
		if (sDistanceStudyMode == "true")
			bDistanceStudyMode = true;
		else if (sDistanceStudyMode == "false")
			bDistanceStudyMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bDistanceStudyMode;
}
