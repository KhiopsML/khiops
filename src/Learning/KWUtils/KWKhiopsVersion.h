// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#define KHIOPS_STR(s) #s

// Constantes definies a la fois dans les sources et dans les ressources Visual Studio.
// Les ressources permettent d'afficher des informations sur les executables
// dans le TaskManager de Windows (par exemple)

// Version de Khiops
#define KHIOPS_VERSION KHIOPS_STR(10.3.1-rc.0)
// Les versions release distribuees sont bases sur trois numeros, par exemple KHIOPS_STR(10.2.0)
// Les versions alpha, beta ou release candidate ont un suffixe supplementaire, par exemple :
// - KHIOPS_STR(10.5.0-a.1)
// - KHIOPS_STR(10.5.0-b.3)
// - KHIOPS_STR(10.5.0-rc.2)

// Copyright
#define KHIOPS_COPYRIGHT_LABEL KHIOPS_STR((c) 2023-2025 Orange. All rights reserved.)
