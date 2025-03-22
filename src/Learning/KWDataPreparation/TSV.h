// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"

//////////////////////////////////////////////////////////////////////////
// Gestion du format TSV (Tabulation SeparatedValue)
class TSV : public Object
{
public:
	// Export d'une valeur pour qu'elle soit lisible dans un fichier tabule (TSV), comme excel
	// On l'entourre de double quotes si necessaire
	static ALString Export(const ALString& sValue);
};
