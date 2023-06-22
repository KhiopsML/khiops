// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"

// TODO: passer une taiile en parametre pour que l'ilmplementation ne depende plus de InputBufferedFile
//  Et retiallier le HugeBuffer si necessaire (appel avec une taille plus grande)

// Acces a un buffer de grande taille
// methode reservee aux classes InputBufferedFile et SystemFileDriver
char* GetHugeBuffer();
