// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
// Directive de compilation dediee au developpement de la bibliotheque parallele
// Seul la librairie PLMPI a besoin effectivement de <mpi.h> pour etre compilee
// (donc du define MPI_DEV)
// Les sources utilisant la bibliotheque Parallel n'ont besoin que du header et des
// psuedo-declaration des pointeurs MPI qui ne sont jamais accedes
#ifdef MPI_DEV
#include "mpi.h"
#else // MPI_DEV
// Reproduction simplifiee des declaration de MPI
// pour eviter d'imposer la presence du SDK de MPI,
// permet d'installer uniquement le run-time MPI aux utilisateurs de Parallel

typedef int MPI_Request;
typedef int MPI_Comm;
int MPI_COMM_NULL;
typedef int MPI_Win;

enum MPI_Datatype
{
	MPI_CHAR,
	MPI_INT
};

struct MPI_Status
{
	int count;
	int cancelled;
	int MPI_SOURCE;
	int MPI_TAG;
	int MPI_ERROR;
};

inline int MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return 0;
}

inline int MPI_Isend(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
		     MPI_Request* request)
{
	return 0;
}

inline int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status)
{
	return 0;
}
inline int MPI_Rsend(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return 0;
}

inline int MPI_Wait(MPI_Request* request, MPI_Status* status)
{
	return 0;
}

inline int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status)
{
	return 0;
}

#endif //  MPI_DEV