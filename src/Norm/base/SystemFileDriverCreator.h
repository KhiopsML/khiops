// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "SystemFileDriver.h"
#include "Object.h"
#include "SystemFileDriverANSI.h"
#include "SystemFileDriverLibrary.h"

///////////////////////////////////////////////////////////////////////////
// Classe SystemFileDriverCreator
// Classe qui cree des drivers de fichiers (HDFS...)
class SystemFileDriverCreator : public Object
{
public:
	// Instanciation des drivers a partir de bibliotheques dynamiques
	// Retourne le nombre de drivers correctement instancies
	static int RegisterExternalDrivers();

	// Renvoie true su les dribers on ete instanicies (correctement ou non)
	static boolean IsExternalDriversRegistered();

	// Retourne le nombre de drivers instancies a partir de bibliotheques dynamiques
	static int GetExternalDriverNumber();

	// Enregistrement d'un driver a partir d'un objet
	static void RegisterDriver(SystemFileDriver* driver);

	// Nettoyage de tous les drivers (objet ou DLL)
	static void UnregisterDrivers();

	// Acces au driver adapte a l'URI passee en parametre
	// La detection de la technologie est automatique et basee sur les schema de l'URI
	// Renvoie NULL en cas d'echec
	static SystemFileDriver* LookupDriver(const ALString& sURI, const Object* errorSender);

	// Renvoie true si il ya un driver disponible pour le scheme passe en parametre
	static boolean IsDriverRegisteredForScheme(const ALString& sScheme);

	// Renvoie le plus grand preferred buffer size de tous les drivers (y compris le driver ANSI)
	static longint GetMaxPreferredBufferSize();

	// Acces aux drivers pour consultation
	static const SystemFileDriver* GetRegisteredDriverAt(int nIndex);

protected:
	// Renvoie true si le nom du fichier en parametre correspond aux nom des drivers
	// libkhiopsdriver_file_SCHEME.so/dll ou SCHEME est une suite alphanumerique sans symboles
	static boolean IsKhiopsDriverName(const ALString& sFileName);

	// Drivers pour les fichiers distants et HDFS
	// La liste des types d'URI gerees correspond a celle definies par FileService::IsURITypeManaged
	// On la gere "en dur" plutot que de facon generique, car leur nombre ne va pas augmenter
	static ObjectArray* oaSystemFileDriver;

	// Driver pour les fichiers locaux, toujours disponible
	static SystemFileDriverANSI driverANSI;

	// Nombre de drivers externes, different de la taille de oaSystemFileDriver car ce tableau
	// contient egalement le drivers objets
	static int nExternalDriverNumber;

	static boolean bIsRegistered;
};

inline boolean SystemFileDriverCreator::IsExternalDriversRegistered()
{
	return bIsRegistered;
}
