// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "LMLicenseService.h"

// Ajout pour portage Unix
#ifdef __linux_or_apple__
#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#endif // __linux_or_apple__

boolean LMLicenseService::Initialize()
{
	boolean bOk = true;
	ALString sLicenseFilePath;
	fstream fstLicenseFile;
	ALString sReadKey;

	require(not bIsInitialised);

	// Initialisation
	svLicenseKeys.SetSize(0);

	// On verifie la date courante
	bOk = CheckCurrentDate();
	if (not bOk)
		AddGlobalError("System date has been changed");

	// Acces au fichier de licence
	sLicenseFilePath = FindLicenseFilePath(false);

	// Lecture des cles de licence
	bOk = bOk and (sLicenseFilePath != "");
	if (bOk)
		bOk = ReadLicenseFile(sLicenseFilePath, nMaxLicenceKeyNumber, &svLicenseKeys);
	bIsInitialised = bOk;
	return bIsInitialised;
}

boolean LMLicenseService::IsInitialized()
{
	return bIsInitialised;
}

boolean LMLicenseService::Close()
{
	// Nettoyage des cles de licences
	svLicenseKeys.SetSize(0);
	bIsInitialised = false;
	return true;
}

const ALString LMLicenseService::GetLicenseFileDirectory()
{
	ALString sLicenseFileDir;

	// Recherche du nom du repertoire de licence
	sLicenseFileDir = p_getenv(sLicenseFileDirEnvVarName);
	return sLicenseFileDir;
}

const ALString LMLicenseService::GetLicenseFileName()
{
	ALString sLicenseFileName;

	// Recherche du nom du fichier de licence
	sLicenseFileName = p_getenv(sLicenseFileNameEnvVarName);
	return sLicenseFileName;
}

const ALString LMLicenseService::GetComputerName()
{
	ALString sCOMPUTERNAME;

	sCOMPUTERNAME = GetLocalHostName();

	// Exception sur le generateur de licence sur le site, qui est base
	// sur une image Docker, avec un probleme pour avoir ces information de facon perenne
#ifdef __WWW_KHIOPS_COM___
	sCOMPUTERNAME = "L";
	sCOMPUTERNAME += "i";
	sCOMPUTERNAME += "c";
	sCOMPUTERNAME += "G";
	sCOMPUTERNAME += "e";
	sCOMPUTERNAME += "n";
	sCOMPUTERNAME += "I";
	sCOMPUTERNAME += "n";
	sCOMPUTERNAME += "D";
	sCOMPUTERNAME += "o";
	sCOMPUTERNAME += "c";
	sCOMPUTERNAME += "k";
	sCOMPUTERNAME += "e";
	sCOMPUTERNAME += "r";
#endif
	return sCOMPUTERNAME;
}

const ALString LMLicenseService::GetMachineID()
{
	ALString sMachineID;
	sMachineID = BuildMachineID();
	return sMachineID;
}

boolean LMLicenseService::UpdateLicenseFile(const ALString& sSourceLicenseFilePath, const StringVector* svFeatureNames,
					    const StringVector* svFeatureVersions, IntVector* ivFoundFeatures,
					    IntVector* ivUpdatedFeatures)
{
	boolean bOk = true;
	boolean bUpdateLicences = false;
	StringVector svLicenseFileKeys;
	int i;
	ALString sKey;
	int nFeature;
	ALString sFeatureFullName;
	ALString sMachineID;
	boolean bValidKey;
	int nDay;
	int nMonth;
	int nYear;
	int nLicense;
	ALString sLicenseKey;
	boolean bValidLicenseKey;
	int nLicenseDay;
	int nLicenseMonth;
	int nLicenseYear;
	ALString sLicenseFilePath;
	fstream fstLicenseFile;

	require(svFeatureNames != NULL);
	require(svFeatureVersions != NULL);
	require(ivFoundFeatures != NULL);
	require(ivUpdatedFeatures != NULL);
	require(svFeatureNames->GetSize() == svFeatureVersions->GetSize());

	// Initialisation du resultats
	ivFoundFeatures->SetSize(svFeatureNames->GetSize());
	ivFoundFeatures->Initialize();
	ivUpdatedFeatures->SetSize(svFeatureNames->GetSize());
	ivUpdatedFeatures->Initialize();

	// On force la reinitialisation
	Close();
	Initialize();
	bOk = IsInitialized();

	// Chargement des cles du fichier
	if (bOk)
		bOk = ReadLicenseFile(sSourceLicenseFilePath, nMaxLicenceKeyNumber * 100, &svLicenseFileKeys);

	// Analyse des cles du fichier
	if (bOk)
	{
		// Recherche de l'identifiant de la machine
		sMachineID = BuildMachineID();

		// Parcours des cles du fichier
		for (i = 0; i < svLicenseFileKeys.GetSize(); i++)
		{
			sKey = svLicenseFileKeys.GetAt(i);

			// Parcours des applications a mettre a jour
			for (nFeature = 0; nFeature < svFeatureNames->GetSize(); nFeature++)
			{
				sFeatureFullName = BuildFeatureFullName(svFeatureNames->GetAt(nFeature),
									svFeatureVersions->GetAt(nFeature));

				// Recherche d'une date limite de validite dans la cle
				// montrant que la cle est valide pour la machine et l'application
				bValidKey =
				    LookupProductKeyLimitDate(sMachineID, sFeatureFullName, sKey, nYear, nMonth, nDay);

				// On verifie que la date n'est pas obsolete
				if (bValidKey)
					bValidKey = (GetRemainingDays(GetFormattedDate(nYear, nMonth, nDay)) >= 0);

				// On cherche alors la cle correspondante dans les cle existantes
				if (bValidKey)
				{
					// Recherche d'une license existante correspondante
					bValidLicenseKey = false;
					for (nLicense = 0; nLicense < svLicenseKeys.GetSize(); nLicense++)
					{
						sLicenseKey = svLicenseKeys.GetAt(nLicense);

						// Test s'il s'agit d'une license pour la meme application
						bValidLicenseKey =
						    LookupProductKeyLimitDate(sMachineID, sFeatureFullName, sLicenseKey,
									      nLicenseYear, nLicenseMonth, nLicenseDay);
						if (bValidLicenseKey)
						{
							// On memorise que la fonctionnalite a ete trouvee
							ivFoundFeatures->SetAt(nFeature, 1);

							// Mise a jour de la license si date posterieure
							if ((nYear - nLicenseYear) * 10000 +
								(nMonth - nLicenseMonth) * 100 + (nDay - nLicenseDay) >
							    0)
							{
								svLicenseKeys.SetAt(nLicense, sKey);
								bUpdateLicences = true;
								ivUpdatedFeatures->SetAt(nFeature, 1);
							}

							// Arret puisqu'on a trouve une licence pour la meme application
							break;
						}
					}

					// Creation d'une nouvelle licence si necessaire
					if (not bValidLicenseKey)
					{
						svLicenseKeys.Add(sKey);
						bUpdateLicences = true;
						ivUpdatedFeatures->SetAt(nFeature, 1);
					}
				}
			}
		}

		// Erreur si pas de licence a mettre a jour
		if (not bUpdateLicences)
			bOk = false;
	}

	// Mise a jour du fichier de licence
	if (bOk and bUpdateLicences)
	{
		assert(svLicenseFileKeys.GetSize() > 0);

		// Acces au fichier de licence
		sLicenseFilePath = FindLicenseFilePath(false);

		// Ecriture des licences
		bOk = FileService::OpenOutputFile(sLicenseFilePath, fstLicenseFile);
		if (bOk)
		{
			for (nLicense = 0; nLicense < svLicenseKeys.GetSize(); nLicense++)
			{
				sLicenseKey = svLicenseKeys.GetAt(nLicense);
				fstLicenseFile << sLicenseKey << "\n";
			}
			bOk = FileService::CloseOutputFile(sLicenseFilePath, fstLicenseFile);
		}
	}
	return bOk;
}

boolean LMLicenseService::RequestProductKey(const ALString& sFeatureName, const ALString& sFeatureVersion)
{
	boolean bOk = true;
	ALString sMachineID;
	ALString sFeatureFullName;
	int nLicense;
	ALString sLicenseKey;
	int nLicenseDay;
	int nLicenseMonth;
	int nLicenseYear;
	boolean bValidLicenseKey;

	// Parcours des cles pour rechercher une cle compatible
	bOk = IsInitialized();
	if (bOk)
	{
		// Initialisations des parametres constants
		sMachineID = BuildMachineID();
		sFeatureFullName = BuildFeatureFullName(sFeatureName, sFeatureVersion);

		// Parcours des licences disponibles
		bOk = false;
		for (nLicense = 0; nLicense < svLicenseKeys.GetSize(); nLicense++)
		{
			sLicenseKey = svLicenseKeys.GetAt(nLicense);

			// Test s'il s'agit d'une license pour la meme application
			bValidLicenseKey = LookupProductKeyLimitDate(sMachineID, sFeatureFullName, sLicenseKey,
								     nLicenseYear, nLicenseMonth, nLicenseDay);

			// Si OK, verification de la date
			if (bValidLicenseKey and
			    GetRemainingDays(GetFormattedDate(nLicenseYear, nLicenseMonth, nLicenseDay)) >= 0)
			{
				bOk = true;
				break;
			}
		}
	}
	return bOk;
}

boolean LMLicenseService::GetProductKeyExpirationDate(const ALString& sFeatureName, const ALString& sFeatureVersion,
						      ALString& sExpirationDate)
{
	boolean bOk = true;
	ALString sMachineID;
	ALString sFeatureFullName;
	ALString sProductKey;
	ALString sDate;
	int nDay;
	int nMonth;
	int nYear;
	int nLicense;
	boolean bValidLicenseKey;
	int nMaxRemainingDays;
	int nRemainingDays;

	// Parcours des cles pour rechercher une cle compatible
	bOk = IsInitialized();
	if (bOk)
	{
		// Initialisations des parametres constants
		sMachineID = BuildMachineID();
		sFeatureFullName = BuildFeatureFullName(sFeatureName, sFeatureVersion);

		// Recherche dans les cles de licence
		bOk = false;
		nMaxRemainingDays = -INT_MAX;
		for (nLicense = 0; nLicense < svLicenseKeys.GetSize(); nLicense++)
		{
			sProductKey = svLicenseKeys.GetAt(nLicense);

			// Test s'il s'agit d'une license pour la meme application
			bValidLicenseKey =
			    LookupProductKeyLimitDate(sMachineID, sFeatureFullName, sProductKey, nYear, nMonth, nDay);

			// Si OK, verification de la date
			if (bValidLicenseKey)
			{
				// On cherche la licence valide la plus recente
				nRemainingDays = GetRemainingDays(GetFormattedDate(nYear, nMonth, nDay));
				if (nRemainingDays > nMaxRemainingDays)
				{
					nMaxRemainingDays = nRemainingDays;
					sDate = GetFormattedDate(nYear, nMonth, nDay);
					assert(CheckDateString(sDate));
					sExpirationDate = sDate;
					bOk = true;
				}
			}
		}
	}
	return bOk;
}

int LMLicenseService::GetRemainingDays(const ALString& sExpirationDate)
{
	int nRemainingDays;
	time_t lCurrentTimestamp;
	struct tm* pDateCurrent;
	struct tm dateCurrent;
	struct tm dateExpiration;
	time_t tCurrent;
	time_t tExpiration;

	require(CheckDateString(sExpirationDate));

	// Recherche de la date courante
	time(&lCurrentTimestamp);
	pDateCurrent = p_localtime(&lCurrentTimestamp);

	// Calcul du nombre de jours restants
	// Ce calcul utile les fonction C ANSI basee sur time_t, qui sont valides
	// dans la limite des dates gerees par la classe (revoir apres 2037)
	nRemainingDays = -1;
	if (pDateCurrent != NULL)
	{
		// Extraction des infos de la date courante
		dateCurrent.tm_year = pDateCurrent->tm_year;
		dateCurrent.tm_mon = pDateCurrent->tm_mon;
		dateCurrent.tm_mday = pDateCurrent->tm_mday;
		dateCurrent.tm_hour = 12;
		dateCurrent.tm_min = 0;
		dateCurrent.tm_sec = 0;
		dateCurrent.tm_isdst = 0;
		tCurrent = mktime(&dateCurrent);

		// Extraction des infos de la date d'expiration
		dateExpiration.tm_year = GetDateYear(sExpirationDate) - 1900;
		dateExpiration.tm_mon = GetDateMonth(sExpirationDate) - 1;
		dateExpiration.tm_mday = GetDateDay(sExpirationDate);
		dateExpiration.tm_hour = 12;
		dateExpiration.tm_min = 0;
		dateExpiration.tm_sec = 0;
		dateExpiration.tm_isdst = 0;
		tExpiration = mktime(&dateExpiration);

		// Calcul de la difference entre les dates
		// Tolerance de deux heures pour les problemes potentiels de passage en heure d'ete-hiver
		if (tCurrent != (time_t)-1 and tExpiration != (time_t)-1)
			nRemainingDays = (int)floor((difftime(tExpiration, tCurrent) + 7200) / (60 * 60 * 24));
	}
	return nRemainingDays;
}

const ALString LMLicenseService::GetPerpetualLimitDate()
{
	ALString sPerpetualLimitDate;

	sPerpetualLimitDate = GetFormattedDate(nLimitDateMaxYear, 12, 31);
	assert(CheckDateString(sPerpetualLimitDate));
	return sPerpetualLimitDate;
}

void LMLicenseService::AddGlobalError(const ALString& sLabel)
{
	Global::AddError("License manager", "", sLabel);
}

ALString LMLicenseService::BuildFeatureFullName(const ALString& sFeatureName, const ALString& sFeatureVersion)
{
	return sFeatureName + "_" + sFeatureVersion;
}

// portage Unix
#ifdef __linux_or_apple__

char* GetProcessorName()
{
	const char* sCpuInfoFileName = "/proc/cpuinfo";
	const char* sModelNameTag = "model name";
	size_t nTagLength;
	static char sProcessorName[1000];
	static char sBuffer[1000];
	FILE* fCpuInfo;

	// Initialisation d'une valeur par defaut
	strcpy(sProcessorName, "No processor info");

	// Parsing du fichier d'infos sur le processeur
	fCpuInfo = p_fopen(sCpuInfoFileName, "r");
	if (fCpuInfo != NULL)
	{
		// Lecture du fichier ligne a ligne pour trouver le nom du processeur
		nTagLength = strlen(sModelNameTag);
		while (fgets(sBuffer, sizeof(sBuffer), fCpuInfo) != NULL)
		{
			// Test si on trouve la bonne ligne
			if (strlen(sBuffer) > nTagLength + 3 and strncmp(sBuffer, sModelNameTag, nTagLength) == 0)
			{
				strcpy(sProcessorName, &sBuffer[nTagLength + 3]);
				break;
			}
		}

		// Fermeture du fichier
		fclose(fCpuInfo);
	}

	// On retourne le resultat
	return sProcessorName;
}

#endif // __linux_or_apple__

////////////////////////////////////////////////////////
// Le machine ID est determine a partir des
// informations suivantes:
// 			COMPUTERNAME
// 			OS
// 			PROCESSOR_IDENTIFIER
// 			PROCESSOR_REVISION
//          MAC address
//
// A partir des informations d'identification (en clair), on
// fabrique un machine ID (crypte), par un
// algorithme de type hascodage.
// A partir du machine ID, d'un nom de fonctionnalite et d'une date limite,
// on fabrique une cle produit de la meme facon, en veillant a ce que la
// date limite (et elle seule) soit decodable.
// Ainsi, a chaque execution de l'application, on recalcule
// le machine ID (qui ne change en principe que
// si la machine change), et en deduit la cle produit
// correspondante, qui doit correspondre a un code d'activation
// present dans le fichier de licence.

const ALString LMLicenseService::FindLicenseFilePath(boolean bSilent)
{
	ALString sTmp;
	ALString sLicenseFileDir;
	ALString sLicenseFileName;
	ALString sLicenseFilePath;
	boolean bFileExist;
	fstream fstLicenseFile;
	boolean bOk;

	// Recherche du repertoire de licence
	sLicenseFileDir = FindLicenseFileDir(bSilent);

	// Recherche du fichier de licence
	if (sLicenseFileDir != "")
	{
		// Recherche du nom du fichier de licence
		sLicenseFileName = GetLicenseFileName();

		// Erreur si vide
		if (sLicenseFileName == "")
		{
			if (not bSilent)
				AddGlobalError(sTmp + "License file not specified (env var " +
					       sLicenseFileNameEnvVarName + " is empty)");
		}
		// Test du fichier de licence
		else
		{
			// Test d'existence
			sLicenseFilePath = FileService::BuildFilePathName(sLicenseFileDir, sLicenseFileName);
			bFileExist = FileService::FileExists(sLicenseFilePath);

			// Tentative de creation si le fichier n'existe pas
			if (not bFileExist)
			{
				bOk = FileService::OpenOutputFile(sLicenseFilePath, fstLicenseFile);
				if (bOk)
					fstLicenseFile.close();
			}

			// Erreur si fichier n'existe pas
			bFileExist = FileService::FileExists(sLicenseFilePath);
			if (not bFileExist)
			{
				if (not bSilent)
					AddGlobalError(sTmp + "License file not found (" + sLicenseFilePath + ")");

				// Mise a vide du resultat
				sLicenseFilePath = "";
			}
		}
	}
	return sLicenseFilePath;
}

const ALString LMLicenseService::FindLicenseFileDir(boolean bSilent)
{
	ALString sTmp;
	ALString sLicenseFileDir;
	boolean bDirExist;

	// Recherche du nom du directory de licence
	sLicenseFileDir = GetLicenseFileDirectory();

	// Erreur si vide
	if (sLicenseFileDir == "")
	{
		if (not bSilent)
			AddGlobalError(sTmp + "License directory not specified (env var " + sLicenseFileDirEnvVarName +
				       " is empty)");
	}
	// Test du repertoire sinon
	else
	{
		bDirExist = FileService::DirExists(sLicenseFileDir);
		if (not bDirExist)
		{
			if (not bSilent)
				AddGlobalError(sTmp + "License directory not found (" + sLicenseFileDir + ")");

			// Mise a vide du resultat
			sLicenseFileDir = "";
		}
	}
	return sLicenseFileDir;
}

boolean LMLicenseService::ReadLicenseFile(const ALString& sLicenseFilePath, int nMaxLineNumber,
					  StringVector* svLicenseFileKeys)
{
	boolean bOk = true;
	fstream fstLicenseFile;
	static char sLine[1000];
	int nLineLength;
	ALString sReadKey;

	require(nMaxLineNumber > 0);
	require(svLicenseFileKeys != NULL);

	// Initialisation du resultat
	svLicenseFileKeys->SetSize(0);

	// Lecture des cles de licence
	bOk = FileService::OpenInputFile(sLicenseFilePath, fstLicenseFile);
	if (bOk)
	{
		// Lecture ligne a ligne du fichier de licence
		while (not fstLicenseFile.eof())
		{
			// Lecture d'une ligne
			sLine[0] = '\0';
			fstLicenseFile.getline(sLine, sizeof(sLine));

			// Nettoyage de la fin de la ligne, pour windows et linux
			nLineLength = (int)strlen(sLine);
			while (nLineLength > 0 and (sLine[nLineLength - 1] == '\r' or sLine[nLineLength - 1] == '\n'))
			{
				sLine[nLineLength - 1] = '\0';
				nLineLength--;
			}

			// On ignore l'eventuelle derniere ligne vide
			if (sLine[0] == '\0' and fstLicenseFile.eof())
				continue;

			// Test si trop de lignes dans le fichier
			if (svLicenseKeys.GetSize() >= nMaxLineNumber)
			{
				bOk = false;
				AddGlobalError("License file " + sLicenseFilePath + " contains too many lines");
				break;
			}
			// Test si cle valide
			else if (not IsKeySyntaxValid(sLine))
			{
				bOk = false;
				AddGlobalError("License file " + sLicenseFilePath + " contains invalid lines");
				break;
			}
			// Sinon, on memorise la cle
			else
				svLicenseFileKeys->Add(sLine);
		}

		// On reinitialise en cas d'erreurs
		if (not bOk)
			svLicenseFileKeys->SetSize(0);

		// Fermeture du fichier de licence
		fstLicenseFile.close();
	}
	return bOk;
}

boolean LMLicenseService::IsKeySyntaxValid(const ALString& sLicenseKey)
{
	boolean bOk = true;
	int i;
	char c;

	if (sLicenseKey.GetLength() != nKeyLength)
		bOk = false;
	if (bOk)
	{
		// Analyse des caracteres de la cle
		for (i = 0; i < sLicenseKey.GetLength(); i++)
		{
			c = sLicenseKey.GetAt(i);

			// Si multiple de 3, doit etre '-'
			if ((i + 1) % 3 == 0)
				bOk = bOk and (c == '-');
			// Sinon, doit etre un chifre en hexadecimal
			else
				bOk = bOk and (('0' <= c and c <= '9') or ('A' <= c and c <= 'F'));
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean LMLicenseService::LookupProductKeyLimitDate(const ALString& sMachineID, const ALString& sFeatureFullName,
						    const ALString& sProductKey, int& nYear, int& nMonth, int& nDay)
{
	boolean bIsDateValid = false;
	ALString sFullKey;
	ALString sKey1;
	ALString sKey2;
	ALString sKey3;
	ALString sKey4;
	ALString sKey5;
	int nDateHashKey;
	int nFactor1;
	int nFactor2;
	int nFactor3;
	int nDateKey1;
	int nDateKey2;
	int nDateKey3;
	int nDateKey4;
	int nDateKey5;
	ALString sDate;

	require(sMachineID.GetLength() == nKeyLength);
	require(sProductKey.GetLength() == nKeyLength);

	// On force une graine aleatoire fixee
	nRandomIndexSeed = 13;

	// Fabrication des cle intermediaires par permutation pour la cle d'identification
	sFullKey = sMachineID + sFeatureFullName;
	sKey1 = sFullKey;
	ShuffleString(sKey1);
	sKey2 = sFullKey;
	ShuffleString(sKey2);
	sKey3 = sFullKey;
	ShuffleString(sKey3);
	sKey4 = sFullKey;
	ShuffleString(sKey4);
	sKey5 = sFullKey;
	ShuffleString(sKey5);

	// Recherche des codes bases sur la date
	nDateKey1 = ReadableStringToInt(sProductKey.Mid(0, 11)) - GetHashKey(sKey1);
	nDateKey2 = ReadableStringToInt(sProductKey.Mid(12, 11)) - GetHashKey(sKey2);
	nDateKey3 = ReadableStringToInt(sProductKey.Mid(24, 11)) - GetHashKey(sKey3);
	nDateKey4 = ReadableStringToInt(sProductKey.Mid(36, 11)) - GetHashKey(sKey4);
	nDateKey5 = ReadableStringToInt(sProductKey.Mid(48, 11)) - GetHashKey(sKey5);

	// Recherche de facteurs fonction de la date
	nDateHashKey = nDateKey5;
	nFactor1 = 1000000 + nDateHashKey % 1000000;
	nFactor2 = 1000 + nDateHashKey % 1000;
	nFactor3 = 10 + nDateHashKey % 10;

	// Recherche de la date et verification de sa validite en inversant les formules
	// ayant servies a son encodage
	nMonth = (nDateKey1 - nDateKey3) / nFactor2;
	nDay = (nDateKey2 - nDateKey5 - nFactor1 * nMonth) / nFactor2;
	nYear = (nDateKey3 - nDateKey5 - nFactor3 * nDay) / nFactor1;
	bIsDateValid = 1 <= nYear and nYear <= 99 and 1 <= nMonth and nMonth <= 12 and 1 <= nDay and nDay <= 31;
	if (bIsDateValid)
	{
		sDate = GetFormattedDate(nYear + 2000, nMonth, nDay);
		bIsDateValid = bIsDateValid and CheckDateString(sDate);
		bIsDateValid = bIsDateValid and
			       nDateKey1 == nDateKey5 + nFactor1 * nYear + nFactor2 * nMonth + nFactor3 * nDay and
			       nDateKey2 == nDateKey5 + nFactor1 * nMonth + nFactor2 * nDay and
			       nDateKey3 == nDateKey5 + nFactor1 * nYear + nFactor3 * nDay and
			       nDateKey4 == nDateKey5 + 987654 * nYear + 321 * nMonth * nDay and
			       nDateKey5 == GetHashKey(sDate + sMachineID);
		nYear += 2000;
	}

	// Mise a 0 de la adte si invalide
	if (not bIsDateValid)
	{
		nYear = 0;
		nMonth = 0;
		nDay = 0;
	}
	return bIsDateValid;
}

const ALString LMLicenseService::BuildMachineIDHeader(const ALString& sComputerName)
{
	ALString sKey;
	ALString sMachineIDHeader;

	require(sComputerName != "");

	// Cle ne dependant que du nom de la machine
	sKey = sComputerName;

	// Fabrication de la cle par hashcodage
	sMachineIDHeader = IntToReadableString(GetHashKey(sKey));

	ensure(sMachineIDHeader.GetLength() == nKeyHeaderLength);
	return sMachineIDHeader;
}

#ifdef __linux_or_apple__
// Renvoie les nSizeMax premiers caracteres contenus dans le fichier passe en parametre
// renvoie vide si le fichier n'existe pas
static const char* GetFileFirstLine(const char* sFileName, int nMaxSize)
{
	FILE* file;
	char* sContent = StandardGetBuffer();

	require(nMaxSize <= 1000);

	// Initialisation avec chaine vide
	strcpy(sContent, "");

	// Ouverture du fichier
	file = p_fopen(sFileName, "r");
	if (file != NULL)
	{
		if (fgets(sContent, nMaxSize, file) == NULL)
			strcpy(sContent, "");
		fclose(file);
	}
	return sContent;
}
#endif // __linux_or_apple__

const ALString LMLicenseService::BuildMachineID()
{
	ALString sTmp;
	const ALString sSeparator = ":-:";
	ALString sCOMPUTERNAME;
	ALString sHARDWAREID;
	ALString sOS;
	ALString sPROCESSOR_IDENTIFIER;
	ALString sPROCESSOR_REVISION;
	ALString sMachineIDHeader;
	ALString sBaseKey;
	ALString sKey1;
	ALString sKey2;
	ALString sKey3;
	ALString sKey4;
	ALString sMachineID;

	// Recherche des informations identifiant la machine
	// Portage unix
#ifndef __linux_or_apple__
	sOS = p_getenv("OS");
	sPROCESSOR_IDENTIFIER = p_getenv("PROCESSOR_IDENTIFIER");
	sPROCESSOR_REVISION = p_getenv("PROCESSOR_REVISION");
#else
	struct utsname sysinfo;
	int nRet;
	nRet = uname(&sysinfo);
	sOS = "unknown";
	if (nRet >= 0)
		sOS = sysinfo.sysname;
	sPROCESSOR_IDENTIFIER = GetProcessorName();
	sPROCESSOR_REVISION = "";
#endif // __linux_or_apple__
	sCOMPUTERNAME = GetComputerName();

	// On prend comme identifiant le GUID et si il n'existe pas l'adresse mac
	sHARDWAREID = GetMachineGUID();
	if (sHARDWAREID == "")
		sHARDWAREID = GetMACAddress();
	sHARDWAREID += GetSerialNumber();

	// Exception sur le generateur de licence sur le site, qui est base
	// sur une image Docker, avec un probleme pour avoir ces information de facon perenne
#ifdef __WWW_KHIOPS_COM___
	sHARDWAREID = "W";
	sHARDWAREID += "w";
	sHARDWAREID += "W";
	sHARDWAREID += ".";
	sHARDWAREID += "K";
	sHARDWAREID += "h";
	sHARDWAREID += "I";
	sHARDWAREID += "o";
	sHARDWAREID += "P";
	sHARDWAREID += "s";
	sHARDWAREID += ".";
	sHARDWAREID += "c";
	sHARDWAREID += "O";
	sHARDWAREID += "m";
#endif

	// Les autres cles sont des permutations des autres informations
	nRandomIndexSeed = 13;
	sBaseKey = sCOMPUTERNAME + sSeparator + sOS + sSeparator + sPROCESSOR_IDENTIFIER + sSeparator +
		   sPROCESSOR_REVISION + sSeparator + sHARDWAREID + sSeparator +
		   IntToString(SystemGetProcessorNumber());

	// Ajout d'informations specifiques a Linux pour eviter qu'une licence soit valide sur 2 machines differentes
#ifdef __linux_or_apple__
	sBaseKey += sSeparator + GetFileFirstLine("/sys/devices/virtual/dmi/id/board_name", 40);
	sBaseKey += sSeparator + GetFileFirstLine("/sys/devices/virtual/dmi/id/chassis_vendor", 40);
	sBaseKey += sSeparator + GetFileFirstLine("/sys/devices/virtual/dmi/id/product_name", 40);
#endif //__linux_or_apple__

	sKey1 = sBaseKey;
	ShuffleString(sKey1);
	sKey2 = sBaseKey;
	ShuffleString(sKey2);
	sKey3 = sBaseKey;
	ShuffleString(sKey3);
	sKey4 = sBaseKey;
	ShuffleString(sKey4);

	// Fabrication de la cle finale par hashcodage des cles intermediaires
	sMachineIDHeader = BuildMachineIDHeader(sCOMPUTERNAME);
	sMachineID = sMachineIDHeader + "-" + IntToReadableString(GetHashKey(sKey1)) + "-" +
		     IntToReadableString(GetHashKey(sKey2)) + "-" + IntToReadableString(GetHashKey(sKey3)) + "-" +
		     IntToReadableString(GetHashKey(sKey4));

	ensure(sMachineID.GetLength() == nKeyLength);
	ensure(sMachineID.Left(nKeyHeaderLength) == sMachineIDHeader);
	return sMachineID;
}

boolean LMLicenseService::CheckMachineID(const ALString& sComputerName, const ALString& sMachineID)
{
	boolean bOk = true;

	// Test des parametres
	if (sComputerName == "")
	{
		bOk = false;
		Global::AddSimpleMessage("Computer name missing");
	}

	// Test de la longueur de la cle
	if (sMachineID.GetLength() != nKeyLength)
	{
		bOk = false;
		Global::AddSimpleMessage("Bad machine ID length");
	}

	// Test de la validite du debut de la cle
	if (bOk)
	{
		if (sMachineID.Left(nKeyHeaderLength) != BuildMachineIDHeader(sComputerName))
		{
			bOk = false;
			Global::AddSimpleMessage("Invalid machine ID");
		}
	}

	return bOk;
}

int LMLicenseService::GetHashKey(const ALString& sValue)
{
	unsigned int nHash = 0x99999999;
	const char* key;

	key = sValue;
	while (*key)
		nHash = (nHash << 5) + nHash + *key++;
	return (int)nHash;
}

const ALString LMLicenseService::IntToReadableString(int nValue)
{
	unsigned int nUnsignedValue;
	int nByte0;
	int nByte1;
	int nByte2;
	int nByte3;
	char sBuffer[20];

	// Encodage (tres leger) de la valeur
	nUnsignedValue = (unsigned int)nValue;
	nUnsignedValue = nUnsignedValue * 256 * 256 * 256 * 4 + nUnsignedValue / 64;

	// Recherche des 4 octets de l'entier
	nByte0 = nUnsignedValue % 256;
	nByte1 = (nUnsignedValue / 256) % 256;
	nByte2 = (nUnsignedValue / (256 * 256)) % 256;
	nByte3 = (nUnsignedValue / (256 * 256 * 256)) % 256;

	// Formatage de la chaine de caractere
	sprintf(sBuffer, "%2.2X-%2.2X-%2.2X-%2.2X", nByte2, nByte0, nByte3, nByte1);
	return sBuffer;
}

int LMLicenseService::ReadableStringToInt(const ALString& sValue)
{
	int nResult;
	unsigned int nUnsignedValue;
	int nByte0;
	int nByte1;
	int nByte2;
	int nByte3;

	require(sValue.GetLength() == 11);
	require(sValue.GetAt(2) == '-' and sValue.GetAt(5) == '-' and sValue.GetAt(8) == '-');

	// Recherche des octets codes de la chaine
	if (sscanf(sValue, "%2X-%2X-%2X-%2X", &nByte2, &nByte0, &nByte3, &nByte1) == 0)
	{
		nByte0 = 0;
		nByte1 = 0;
		nByte2 = 0;
		nByte3 = 0;
	}

	// Recodage de la valeur 2280407184
	nUnsignedValue = nByte0 + 256 * (nByte1 + 256 * (nByte2 + 256 * nByte3));
	nUnsignedValue = nUnsignedValue / (256 * 256 * 256 * 4) + nUnsignedValue * 64;
	nResult = (int)nUnsignedValue;
	assert(IntToReadableString(nResult) == sValue);

	return nResult;
}

void LMLicenseService::ShuffleString(ALString& sValue)
{
	int i;
	int iSwap;
	char cSwap;

	// Retour si pas assez d'elements
	if (sValue.GetLength() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < sValue.GetLength(); i++)
	{
		iSwap = GetRandomIndex(i - 1);
		cSwap = sValue.GetAt(iSwap);
		sValue.SetAt(iSwap, sValue.GetAt(i));
		sValue.SetAt(i, cSwap);
	}
}

int LMLicenseService::GetRandomIndex(int nMax)
{
	unsigned int nRandomIndex;
	double dRandom;
	int nRand;

	require(nMax >= 0);

	// Reel aleatoire entre 0 et 1
	nRandomIndex = (unsigned int)nRandomIndexSeed;
	nRandomIndex *= 1103515245;
	nRandomIndex += 12345;
	nRandomIndexSeed = nRandomIndex;
	dRandom = (((unsigned int)(nRandomIndex / 65536) % 32768) / 32768.0);

	// Entier aleatoire entre 0 et nMax
	nRand = (int)floor((nMax + 1) * dRandom);
	if (nRand > nMax)
		nRand--;
	return (nRand);
}

const boolean LMLicenseService::CheckCurrentDate()
{
	boolean bOk = true;
	ALString sLicenseKeyDirPath;
	ALString sTmpDirPath;
	struct_stat statKeyDir;
	struct_stat statTmpDir;
	time_t lCurrentTimestamp;
	const time_t lTolerance = 86400;
	struct tm* dateCurrent;
	int nResult;

	// Acces a la date courante
	time(&lCurrentTimestamp);
	dateCurrent = p_localtime(&lCurrentTimestamp);

	// Acces aux caracteristiques du directory de gestion des cles
	sLicenseKeyDirPath = FindLicenseFileDir(true);
	if (sLicenseKeyDirPath != "")
	{
		// On verifie que la date courante est plus grande que les dates de creation,
		// modification et de dernier acces des directory
		// On prend en compte une tolerance d'une journee
		// car il semble que l'heure systeme subissent un decallage d'une heure sur certains systemes Linux
		// (et qu'il peut y avoir des probleme pour des utilisateurs changeant de fuseau horaire)
		nResult = p_stat(sLicenseKeyDirPath, &statKeyDir);
		if (nResult == 0)
			bOk = bOk and lCurrentTimestamp + lTolerance >= statKeyDir.st_ctime and
			      lCurrentTimestamp + lTolerance >= statKeyDir.st_mtime and
			      lCurrentTimestamp + lTolerance >= statKeyDir.st_atime;
	}

	// Recherche du nom du chemin du directory temporaire
	// On se base sur le fichier temporaire, et non sur le fichier systeme windir,
	// ce qui poserait probleme si un utilisateur bidouille sa date systeme
	// (il ne faudrait pas dans ce cas l'obliger a reinstaller le systeme, alors
	// qu'on peut le faire recreer le repertoire des fichiers temporaires).
	sTmpDirPath = FileService::GetTmpDir();
	if (sTmpDirPath != "")
	{
		nResult = p_stat(sTmpDirPath, &statTmpDir);
		if (nResult == 0)
			bOk = bOk and lCurrentTimestamp + lTolerance >= statTmpDir.st_ctime and
			      lCurrentTimestamp + lTolerance >= statTmpDir.st_mtime and
			      lCurrentTimestamp + lTolerance >= statTmpDir.st_atime;
	}

	// On verifie egalement que la date courante est plus grande que 2015
	if (dateCurrent != NULL)
		bOk = bOk and dateCurrent->tm_year >= 115;
	return bOk;
}

boolean LMLicenseService::CheckDateString(const ALString& sDateValue)
{
	boolean bOk = true;
	int nDay;
	int nMonth;
	int nYear;

	// Longueur de la chaine
	if (sDateValue.GetLength() != 10)
		bOk = false;
	// Format de la chaine
	else if (not isdigit(sDateValue.GetAt(0)) or not isdigit(sDateValue.GetAt(1)) or
		 not isdigit(sDateValue.GetAt(2)) or not isdigit(sDateValue.GetAt(3)) or sDateValue.GetAt(4) != '-' or
		 not isdigit(sDateValue.GetAt(5)) or not isdigit(sDateValue.GetAt(6)) or sDateValue.GetAt(7) != '-' or
		 not isdigit(sDateValue.GetAt(8)) or not isdigit(sDateValue.GetAt(9)))
		bOk = false;
	// Verification stricte
	else
	{
		// On ne passe pas par les methode GetDateYear, GetDateMonth et et GetDateDay
		// qui supposent la date valide (recursion infinie sinon)
		nYear = StringToInt(sDateValue.Left(4));
		nMonth = StringToInt(sDateValue.Mid(5, 2));
		nDay = StringToInt(sDateValue.Right(2));

		// Verification de base
		bOk = bOk and nLimitDateMinYear <= nYear and nYear <= nLimitDateMaxYear and 1 <= nMonth and
		      nMonth <= 12 and 1 <= nDay and nDay <= 31;

		// Verification du jour du mois de fevrier
		if (nMonth == 2)
		{
			if (nYear % 4 == 0)
				bOk = bOk and nDay <= 29;
			else
				bOk = bOk and nDay <= 28;
		}
		// Verification pour les autres mois a trente jour
		else if (nMonth == 4 or nMonth == 6 or nMonth == 9 or nMonth == 11)
			bOk = bOk and nDay <= 30;
	}
	return bOk;
}

const ALString LMLicenseService::GetFormattedDate(int nYear, int nMonth, int nDay)
{
	char sDate[20];
	require(1000 <= nYear and nYear <= 9999);
	require(1 <= nMonth and nMonth <= 12);
	require(1 <= nDay and nDay <= 31);

	sprintf(sDate, "%04d-%02d-%02d", nYear, nMonth, nDay);
	return sDate;
}

int LMLicenseService::GetDateYear(const ALString& sDateValue)
{
	require(CheckDateString(sDateValue));
	return StringToInt(sDateValue.Left(4));
}

int LMLicenseService::GetDateMonth(const ALString& sDateValue)
{
	require(CheckDateString(sDateValue));
	return StringToInt(sDateValue.Mid(5, 2));
}

int LMLicenseService::GetDateDay(const ALString& sDateValue)
{
	require(CheckDateString(sDateValue));
	return StringToInt(sDateValue.Right(2));
}

boolean LMLicenseService::bIsInitialised = false;
StringVector LMLicenseService::svLicenseKeys;

const ALString LMLicenseService::sLicenseFileDirEnvVarName = "KHIOPS_LICENSE_FILE_DIR";
const ALString LMLicenseService::sLicenseFileNameEnvVarName = "KHIOPS_LICENSE_FILE_NAME";
const int LMLicenseService::nKeyHeaderLength = 11;
const int LMLicenseService::nKeyLength = 59;
int LMLicenseService::nRandomIndexSeed = 1;
int LMLicenseService::nMaxLicenceKeyNumber = 1000;