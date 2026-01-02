// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"

// Extraction de la version majeure d'une version complete
// Par exemple: renvoie 10 pour la version 10.1.4i
int GetMajorVersion(const ALString& sFullVersion);

// Extraction de la version majeure d'une version complete
// Par exemple: renvoie 1 pour la version 10.1.4i
int GetMinorVersion(const ALString& sFullVersion);
