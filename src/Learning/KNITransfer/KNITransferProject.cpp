// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNITransferProject.h"

KNITransferProject::KNITransferProject() {}

KNITransferProject::~KNITransferProject() {}

void KNITransferProject::OpenLearningEnvironnement()
{
	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Declaration des licences
	if (LMLicenseManager::IsEnabled())
		LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::Khiops);

	// Parametrage du nom du module applicatif
	SetLearningModuleName("KNI Transfer");
}

KWLearningProblem* KNITransferProject::CreateLearningProblem()
{
	return new KNITransferProblem;
}

KWLearningProblemView* KNITransferProject::CreateLearningProblemView()
{
	return new KNITransferProblemView;
}
