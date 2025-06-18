// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProblem.h"

////////////////////////////////////////////////////////////
// Classe KNITransferProblem
//    Transfer de database via l'API KNI
// Cela permet de tester le transfer multi-table KNI avec
// les meme scenarios que pour Khiops
class KNITransferProblem : public KWLearningProblem
{
public:
	// Constructeur
	KNITransferProblem();
	~KNITransferProblem();
};
