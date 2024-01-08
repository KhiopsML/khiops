// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "PLMPIFileServerSlave.h"
#include "PLParallelTask.h"
#include "PLMPISlave.h"

//////////////////////////////////////////////////////////
// Classe  PLMPISlaveLauncher
// Classe qui lance et arrete les esclaves et le serveurs de fichiers
//
class PLMPISlaveLauncher : public Object
{
public:
	PLMPISlaveLauncher();
	~PLMPISlaveLauncher();

	static void Launch();

protected:
	static void LaunchFileServer(IntVector* ivServerRanks, boolean& bOrderToQuit);
	static void LaunchSlave(IntVector* ivExcludeSlaves, const ALString& sTaskName);
};
