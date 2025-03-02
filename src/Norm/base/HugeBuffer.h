// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"

/////////////////////////////////////////////////////////////////////////////
// Gestion d'un buffer global de grande taille
//
// Ce buffer est cree a la demande, retaille uniquement si necessaire, par agrandissement,
// ou detruit a la demande. Notez que comme il est global il doit etre utilise
// de facon non-concurrente sur un processus (risque de data race).
//
// Ce buffer automatiquement detruit en fin de programme
//
// Services reserves aux classes InputBufferedFile et SystemFileDriver

// Acces au buffer global de grande taille, cree automatiquement si necessaire
char* GetHugeBuffer(unsigned int nHugeSize);

// Renvoie l'adresse du buffer de grande taille en cours, NULL s'il n'est pas alloue
// Potentiellement utile pour les assertions
char* GetHugeBufferAdress();

// Taille du buffer en cours
int GetHugeBufferSize();

// Destruction du HugeBuffer (buffer global)
// Cette methode, appelee automatiquement en fin de programme, peut etre appelee a tout momemt pour liberer de la
// memoire
void DeleteHugeBuffer();
