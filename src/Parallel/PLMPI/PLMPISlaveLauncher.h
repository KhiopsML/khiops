// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "PLMPIFileServerSlave.h"
#include "PLParallelTask.h"
#include "PLMPISlave.h"

//////////////////////////////////////////////////////////
// Classe  PLMPISlaveLauncher
// Classe qui lance et arrete les esclaves
//
class PLMPISlaveLauncher : public Object
{
public:
	PLMPISlaveLauncher();
	~PLMPISlaveLauncher();

	static void Launch();
	static void LaunchFileServer(IntVector* ivServerRanks, boolean& bOrderToQuit);

protected:
	// Initialisation a partir de l'escalve si le maitre le demande
	static void SlaveInitializeResourceSystem();

	// Fonction non bloquante qui renvoie true si le master demande le lancement d'une tache.
	// La variable sTaskName est alors mise a jour avec le nom de la tache a lancer (mise a jour avec la chaine vide
	// sinon)
	static boolean IsSlaveToLaunch(ALString& sTaskName);

	// Fonction non bloquante qui renvoie true si le maitre a appele la methode StartFileServer
	// Elle renvoie true meme si aucun serveur de fichier n'est lance (mais le process comm a ete initialise par le
	// maitre)
	static boolean IsTimeToLaunchFileServer(IntVector* ivServerRanks);

	// Fonction non bloquante qui renvoie true si le maitre a appele la methode StopFileServer
	static boolean IsFileServerEnd();

	// Fonction non bloquante qui renvoie true si le master demande l'arret de tous les esclaves.
	static boolean ReceiveOrderToQuit();
};
