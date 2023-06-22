// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriverCreator.h"

ObjectArray* SystemFileDriverCreator::oaSystemFileDriver = NULL;
SystemFileDriverANSI SystemFileDriverCreator::driverANSI;
boolean SystemFileDriverCreator::bIsRegistered = false;

/////////////////////////////////////////////
// Implementation de la classe SystemFileDriverCreator

void SystemFileDriverCreator::RegisterDrivers()
{

	ALString sLibraryPath;
	StringVector svDirectoryNames;
	StringVector svFileNames;
	ALString sLibraryName;
	SystemFileDriverLibrary* driverLibrary;
	boolean bOk;
	int i;
	boolean bVerbose = false;

	require(not bIsRegistered);

	if (oaSystemFileDriver == NULL)
		oaSystemFileDriver = new ObjectArray;

#ifdef __UNIX__
	sLibraryPath = "/usr/lib/";
#else
	sLibraryPath = p_getenv("KhiopsRoot");
#endif //__UNIX__
	bOk = FileService::GetDirectoryContent(sLibraryPath, &svDirectoryNames, &svFileNames);
	if (bOk)
	{
		// Parcours du repertoire et chargement de tous les fichiers qui ont un nom de la forme
		// libkhiopsdriver_file_SCHEME.so/dll
		for (i = 0; i < svFileNames.GetSize(); i++)
		{
			sLibraryName = svFileNames.GetAt(i);
			if (IsKhiopsDriverName(sLibraryName))
			{
				driverLibrary = new SystemFileDriverLibrary;
				bOk = driverLibrary->LoadLibrary(sLibraryName);
				if (bVerbose)
					cout << "load " << sLibraryName << " " << BooleanToString(bOk) << endl;

				assert(bOk);
				if (bOk)
					oaSystemFileDriver->Add(driverLibrary);
			}
		}
	}
	bIsRegistered = true;
}

void SystemFileDriverCreator::UnregisterDrivers()
{
	int i;
	SystemFileDriver* driver;
	ALString sEmpty;
	boolean bIsANSI;

	require(bIsRegistered);

	for (i = 0; i < oaSystemFileDriver->GetSize(); i++)
	{
		driver = cast(SystemFileDriver*, oaSystemFileDriver->GetAt(i));
		bIsANSI = sEmpty.Compare(driver->GetScheme()) == 0;

		// Deconnexion et dechargement (on ne pourra plus appeler de methodes du driver)
		if (driver->IsConnected())
			driver->Disconnect();

		// Cas particulier du driver ANSI qui n'est pas un pointeur
		if (bIsANSI)
			delete driver;
	}
	bIsRegistered = false;
	oaSystemFileDriver->DeleteAll();
	delete oaSystemFileDriver;
	oaSystemFileDriver = NULL;
}

SystemFileDriver* SystemFileDriverCreator::LookupDriver(const ALString& sURIFilePathName, const Object* errorSender)
{
	debug(boolean bStrongAssert = false);
	ALString sScheme;
	ALString sMessage;
	SystemFileDriver* driver;
	SystemFileDriver* registeredDriver;
	int i;

	// Recherche du schema
	sScheme = FileService::GetURIScheme(sURIFilePathName);

	// Doit etre enregistre ou fichier local ou fichier distant sur localhost
	// En principe, l'utilisateur ne doit pas utiliser le remote scheme
	// S'il le fait depuis l'interface, on cherchera le fichier en local avec un driver ansi, et cela provoquera un
	// message d'erreur On laisse neanmoins cette assertion en mode debug pour verifier que l'on n'utilisera pas de
	// remote scheme en fonctionnement "normal"
	debug(bStrongAssert = true);
	assert(not bStrongAssert or sScheme != FileService::sRemoteScheme or
	       FileService::GetURIHostName(sURIFilePathName) == GetLocalHostName());

	// Verification que l'URI a une forme attendue
	if (not FileService::IsURIWellFormed(sURIFilePathName))
	{
		sMessage = "the file URI is not well-formed (" + sURIFilePathName + ")";

		if (errorSender != NULL)
			errorSender->AddError(sMessage);
		else
			Global::AddError("", "", sMessage);
		return NULL;
	}

	// Si le scheme est vide: c'est le driver ANSI
	driver = NULL;
	if (sScheme.IsEmpty() or sScheme == FileService::sRemoteScheme)
		driver = &driverANSI;
	// Sinon, on recherche parmi les driver s'ils ont ete enregistres
	else if (bIsRegistered)
	{
		// On parcourt tous les drivers pour trouver celui qui traite le scheme
		for (i = 0; i < oaSystemFileDriver->GetSize(); i++)
		{
			registeredDriver = cast(SystemFileDriver*, oaSystemFileDriver->GetAt(i));
			if (sScheme.Compare(registeredDriver->GetScheme()) == 0)
			{
				driver = registeredDriver;
				break;
			}
		}
	}

	// Si aucun driver n'est trouve c'est que le schema de l'URI n'est pas connue
	if (driver == NULL)
	{
		sMessage = "there is no driver avalaible for the scheme '" + sScheme + "://'";
		if (errorSender != NULL)
			errorSender->AddError(sMessage);
		else
			Global::AddError("", "", sMessage);
		return NULL;
	}
	else
	{
		// TODO charger la library a  ce moment la
		if (not driver->IsConnected())
			driver->Connect();
		if (not driver->IsConnected())
		{
			delete driver;
			driver = NULL;
		}
	}

	return driver;
}

boolean SystemFileDriverCreator::IsKhiopsDriverName(const ALString& sFileName)
{
	const ALString sPrefix = "libkhiopsdriver_file_";
#ifdef __UNIX__
	const ALString sExtension = "so";
#else  //__UNIX__
	const ALString sExtension = "dll";
#endif //__UNIX__
	ALString sScheme;
	boolean bOk = true;
	int i;
	char c;
	if (sFileName.Find(sPrefix) == -1)
		return false;
	if (FileService::GetFileSuffix(sFileName) != sExtension)
		return false;

	// on verifie que la partie scheme ne contient que des caracteres alpha-numeriques
	bOk = true;
	for (i = sPrefix.GetLength(); i < sFileName.GetLength(); i++)
	{
		c = sFileName.GetAt(i);
		if (c == '.')
			break;
		bOk = isalnum(c) != 0;
		if (not bOk)
			break;
	}

	return bOk;
}
