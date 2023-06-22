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
	static void RegisterDrivers();
	static void UnregisterDrivers();

	// Acces au driver adapte a l'URI passee en parametre
	// La detection de la technologie est automatique et basse sur les schema de l'URI
	// Renvoie NULL en cas d'echec
	static SystemFileDriver* LookupDriver(const ALString& sScheme, const Object* errorSender);

protected:
	// Renvoie true si le nom du fichier en parametre corresepond aux nom des drivers
	// libkhiopsdriver_file_SCHEME.so/dll ou SCHEME est une suite alphanumerique sans symboles
	static boolean IsKhiopsDriverName(const ALString& sFileName);

	// Drivers pour les fichiers distants et HDFS
	// La liste des types d'URI gerees correspond a celle definies par FileService::IsURITypeManaged
	// On la gere "en dur" plutot que de facon generique, car leur nombre ne va pas augmenter
	static ObjectArray* oaSystemFileDriver;
	static boolean bIsRegistered;

	// Driver pour les fichiers locaux, toujours disponible
	static SystemFileDriverANSI driverANSI;
};