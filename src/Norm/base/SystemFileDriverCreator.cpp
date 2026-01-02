// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriverCreator.h"

ObjectArray* SystemFileDriverCreator::oaSystemFileDriver = NULL;
SystemFileDriverANSI SystemFileDriverCreator::driverANSI;
int SystemFileDriverCreator::nExternalDriverNumber = 0;
boolean SystemFileDriverCreator::bIsRegistered = false;

/////////////////////////////////////////////
// Implementation de la classe SystemFileDriverCreator

int SystemFileDriverCreator::RegisterExternalDrivers()
{
	ALString sLibraryPath;
	StringVector svDirectoryNames;
	StringVector svFileNames;
	ALString sLibraryName;
	SystemFileDriverLibrary* driverLibrary;
	ALString sLibraryScheme;
	SystemFileDriver* registeredDriver;
	boolean bOk = true;
	int i;
	int nDriver;
	ALString sTmp;

	require(not bIsRegistered);

	if (oaSystemFileDriver == NULL)
		oaSystemFileDriver = new ObjectArray;
	nExternalDriverNumber = 0;

	// On cherche les drivers dans le chemin renseigne par la variable d'environement KHIOPS_DRIVERS_PATH
	// Si elle n'est pas renseigne, on cherche dans les chemins par defaut.

	// Teste si KHIOPS_DRIVERS_PATH est renseignee avec une valeur correcte
	sLibraryPath = p_getenv("KHIOPS_DRIVERS_PATH");
	if (sLibraryPath != "")
	{
		if (!FileService::DirExists(sLibraryPath))
		{
			Global::AddError(
			    "", "", "Drivers location directory missing (KHIOPS_DRIVERS_PATH=" + sLibraryPath + ")");
			bOk = false;
		}
	}

	// Si KHIOPS_DRIVERS_PATH n'est pas renseignee, on cherche dans les valeurs par defaut
	if (bOk and sLibraryPath == "")
	{
#ifdef _WIN32
		sLibraryPath = p_getenv("KHIOPS_HOME");
		sLibraryPath += "\\bin";
#elif defined __linux__
		sLibraryPath = "/usr/lib/";
#elif defined __APPLE__ // TODO: a verifier
		sLibraryPath = "/usr/lib/";
#endif
	}

	if (bOk)
	{
		bOk = FileService::GetDirectoryContentExtended(sLibraryPath, &svDirectoryNames, &svFileNames);
		// On ne teste pas le retour de la methode car ca revient a tester si KHIOPS_HOME\bin existe et si Khiops est installe, il existe.
		// En revanche ca pose probleme si on lance MODL en standalone (avec khy_test ou directement en ligne de commande, sans passer par khiops_env)
		// car KHIOPS_HOME n'est pas defini et on va chercher dans le repertoire \lib qui n'existe pas. Mais c'est un cas particulier
		// dans lequel on ne veut pas charger les drivers (et donc on ne veut pas d'erreur)
		// Sur Linux il n'y a pas de probleme potentiel : /usr/lib existe toujours
	}

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
				bOk = driverLibrary->LoadLibrary(
				    FileService::BuildFilePathName(sLibraryPath, sLibraryName));

				// Nettoyage si erreur
				if (not bOk)
				{
					delete driverLibrary;

					// Message signalant uniquement si process maitre
					if (GetProcessId() == 0)
						Global::AddWarning("File driver", sLibraryName,
								   sTmp + "Failed to load file driver from directory " +
								       sLibraryPath);
				}
				// On continue sinon
				else
				{
					// On teste si le driver n'existe pas deja sinon
					sLibraryScheme = driverLibrary->GetScheme();
					for (nDriver = 0; nDriver < oaSystemFileDriver->GetSize(); nDriver++)
					{
						assert(oaSystemFileDriver->GetAt(nDriver) != NULL);
						registeredDriver =
						    cast(SystemFileDriver*, oaSystemFileDriver->GetAt(nDriver));

						// Erreur si schema existant deja gere
						// On passe par la variable sLibraryScheme pour faire les comparaisons
						// avec GetScheme, qui renvoie un char*
						bOk = sLibraryScheme != registeredDriver->GetScheme();
						if (not bOk)
						{
							delete driverLibrary;
							if (GetProcessId() == 0)
								Global::AddSimpleMessage(
								    "Failed to load file driver " + sLibraryName +
								    " from directory " + sLibraryPath +
								    " because of previously loaded file driver '" +
								    registeredDriver->GetDriverName() +
								    "' for URI scheme '" +
								    registeredDriver->GetScheme() + "'");
							break;
						}
					}

					// Enregistrement du driver si oK
					if (bOk)
					{
						nExternalDriverNumber++;
						oaSystemFileDriver->Add(driverLibrary);
					}
				}
			}
		}
	}
	bIsRegistered = true;
	return nExternalDriverNumber;
}

int SystemFileDriverCreator::GetExternalDriverNumber()
{
	return nExternalDriverNumber;
}

int SystemFileDriverCreator::GetDriverNumber()
{
	int nDriverNumber = 0;
	if (oaSystemFileDriver != NULL)
		nDriverNumber = oaSystemFileDriver->GetSize();
	return nDriverNumber;
}

void SystemFileDriverCreator::RegisterDriver(SystemFileDriver* driver)
{
	if (oaSystemFileDriver == NULL)
		oaSystemFileDriver = new ObjectArray;
	oaSystemFileDriver->Add(driver);
}

void SystemFileDriverCreator::UnregisterDrivers()
{
	int i;
	SystemFileDriver* driver;

	if (oaSystemFileDriver != NULL)
	{
		for (i = 0; i < oaSystemFileDriver->GetSize(); i++)
		{
			if (oaSystemFileDriver->GetAt(i) != NULL)
			{
				driver = cast(SystemFileDriver*, oaSystemFileDriver->GetAt(i));

				// Deconnexion et dechargement (on ne pourra plus appeler de methodes du driver)
				if (driver->IsConnected())
					driver->Disconnect();
				delete driver;
			}
		}

		delete oaSystemFileDriver;
		oaSystemFileDriver = NULL;
	}
	nExternalDriverNumber = 0;
}

SystemFileDriver* SystemFileDriverCreator::LookupDriver(const ALString& sURIFilePathName, const Object* errorSender)
{
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
	if (sScheme.IsEmpty())
		driver = &driverANSI;
	// Sinon, on recherche parmi les driver qui ont ete enregistres
	else
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

	// Si aucun driver n'est trouve c'est que le schema de l'URI n'est pas connu
	if (driver == NULL)
	{
		sMessage = "there is no driver available for the URI scheme '" + sScheme + "://'";
		if (errorSender != NULL)
			errorSender->AddError(sMessage);
		else
			Global::AddError("", "", sMessage);
		return NULL;
	}
	else
	{
		// TODO charger la library a ce moment la
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

boolean SystemFileDriverCreator::IsDriverRegisteredForScheme(const ALString& sScheme)
{
	int i;
	SystemFileDriver* registeredDriver;

	// On parcourt tous les drivers pour trouver celui qui traite le scheme
	for (i = 0; i < oaSystemFileDriver->GetSize(); i++)
	{
		registeredDriver = cast(SystemFileDriver*, oaSystemFileDriver->GetAt(i));
		if (sScheme.Compare(registeredDriver->GetScheme()) == 0)
		{
			return true;
		}
	}
	return false;
}

longint SystemFileDriverCreator::GetMaxPreferredBufferSize()
{
	longint lMaxPrefferedSize;
	int i;

	lMaxPrefferedSize = driverANSI.GetSystemPreferredBufferSize();
	if (oaSystemFileDriver != NULL)
	{
		// Parcours des drivers externes
		for (i = 0; i < oaSystemFileDriver->GetSize(); i++)
		{
			lMaxPrefferedSize =
			    max(lMaxPrefferedSize,
				cast(SystemFileDriver*, oaSystemFileDriver->GetAt(i))->GetSystemPreferredBufferSize());
		}
	}
	return lMaxPrefferedSize;
}

const SystemFileDriver* SystemFileDriverCreator::GetRegisteredDriverAt(int nIndex)
{
	require(nIndex < GetExternalDriverNumber());

	if (oaSystemFileDriver == NULL)
		return NULL;

	return cast(SystemFileDriver*, oaSystemFileDriver->GetAt(nIndex));
}

boolean SystemFileDriverCreator::IsKhiopsDriverName(const ALString& sFileName)
{
	const ALString sPrefix = "libkhiopsdriver_file_";
#ifdef _WIN32
	const ALString sExtension = "dll";
#elif defined __linux__
	const ALString sExtension = "so";
#elif defined __APPLE__ // TODO: a verifier
	const ALString sExtension = "dylib";
#endif
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
