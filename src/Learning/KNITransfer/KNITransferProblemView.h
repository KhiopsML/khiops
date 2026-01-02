// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KNITransferProblem.h"
#include "KWLearningProblemView.h"
#include "KNIDatabaseTransferView.h"

////////////////////////////////////////////////////////////
// Classe KNITransferProblemView
//    Khiops: preparation des donnees
// Editeur de KNITransferProblemView
class KNITransferProblemView : public KWLearningProblemView
{
public:
	// Constructeur
	KNITransferProblemView();
	~KNITransferProblemView();

	// Transfer de database par l'API KNI
	void KNITransferDatabase();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	KNITransferProblem* GetTransferProblem();
};
