// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseTransferView.h"
#include "KNIRecodeMTFiles.h"
#include "KhiopsNativeInterface.h"

////////////////////////////////////////////////////////////
// Classe KWDatabaseTransferView
//    Specialisation de KWDatabaseTransferView pour reimplementer
//     le transfert de database avec KNI
class KNIDatabaseTransferView : public KWDatabaseTransferView
{
public:
	// Constructeur
	KNIDatabaseTransferView();
	~KNIDatabaseTransferView();

	// Parametrage du fichier dictionnaire, necessaire pour l'API KNI
	void SetClassFileName(const ALString& sValue);
	const ALString& GetClassFileName() const;

	// Action de transfert implementee avec KNI
	void KNITransferDatabase();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametrage des index des cles d'un fichier d'entree
	void FillKeyIndexes(const KWMTDatabaseMapping* mapping, KNIInputFile* inputFileSpec) const;

	// Nom du fichier de dictionnaire
	ALString sClassFileName;
};
