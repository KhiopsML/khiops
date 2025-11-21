// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"

// Status utilises pour decorer les messages entre le maitre et les esclaves
enum MESSAGE_TAG
{
	SLAVE_END_PROCESSING,
	SLAVE_DONE,
	SLAVE_FATAL_ERROR,
	SLAVE_INITIALIZE_DONE,
	SLAVE_PROGRESSION,
	SLAVE_RANKS_HOSTNAME,
	SLAVE_USER_MESSAGE,

	MASTER_LAUNCH_WORKERS,
	MASTER_LAUNCH_FILE_SERVERS,
	MASTER_STOP_FILE_SERVERS,
	MASTER_QUIT,
	MASTER_RESOURCES,
	MASTER_STOP_ORDER,
	MASTER_TASK_INPUT,
	MASTER_LOG_FILE,
	MASTER_TRACER_MPI,

	INTERRUPTION_REQUESTED,
	MAX_ERROR_FLOW,

	// Status utilises pour la gestion des ressources
	RESOURCE_HOST,
	RESOURCE_MEMORY,

	// Status utilises pour les acces aux fichiers distants
	FILE_SERVER_REQUEST_SIZE,
	FILE_SERVER_REQUEST_FILE_EXISTS,
	FILE_SERVER_REQUEST_DIR_EXISTS,
	FILE_SERVER_REQUEST_REMOVE,
	FILE_SERVER_FREAD
};

// Status en string pour les traces
const ALString sTags[25] = {"SLAVE_END_PROCESSING", "SLAVE_DONE", "SLAVE_FATAL_ERROR", "SLAVE_INITIALIZE_DONE",
			    "SLAVE_PROGRESSION", "SLAVE_RANKS_HOSTNAME", "SLAVE_USER_MESSAGE", "MASTER_LAUNCH_WORKERS",
			    "MASTER_LAUNCH_FILE_SERVERS", "MASTER_STOP_FILE_SERVERS", "MASTER_QUIT", "MASTER_RESOURCES",
			    "MASTER_STOP_ORDER", "MASTER_TASK_INPUT", "MASTER_LOG_FILE", "MASTER_TRACER_MPI",

			    "INTERRUPTION_REQUESTED", "MAX_ERROR_FLOW",

			    // Status utilises pour la gestion des ressources
			    "RESOURCE_HOST", "RESOURCE_MEMORY",

			    // Status utilises pour les acces aux fichiers distants
			    "FILE_SERVER_REQUEST_SIZE", "FILE_SERVER_REQUEST_FILE_EXISTS",
			    "FILE_SERVER_REQUEST_DIR_EXISTS", "FILE_SERVER_REQUEST_REMOVE", "FILE_SERVER_FREAD"};

inline ALString GetTagAsString(int i)
{
	return sTags[i];
}
