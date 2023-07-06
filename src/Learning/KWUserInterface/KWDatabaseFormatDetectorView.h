// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDatabaseFormatDetectorView;

#include "UserInterface.h"
#include "KWDatabaseFormatDetector.h"

////////////////////////////////////////////////////////////
// Classe KWDatabaseFormatDetectorView
// Detection du format des fichiers d'une base donnees:
//    . presence de ligne d'entete
//    . separateur de champ
// Cette fiche ne contient qu'un action "Detect database format"
class KWDatabaseFormatDetectorView : public UICard
{
public:
	// Constructeur
	KWDatabaseFormatDetectorView();
	~KWDatabaseFormatDetectorView();

	// Parametrage de la base a analyser
	void SetDatabase(KWDatabase* database);
	KWDatabase* GetDatabase() const;

	// Utilisation ou non du dictionnaire pour detecter le format (defaut: true)
	// Si le dictionnaire est utilisable, on l'utilise s'il est specifie, et sinon
	// on se rabat sur le comprtement sans dictionnaire
	void SetUsingClass(boolean bValue);
	boolean GetUsingClass();

	// Detection du format
	void DetectFileFormat();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Detecteur de format
	KWDatabaseFormatDetector databaseFormatDetector;
};