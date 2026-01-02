// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWSTDatabaseTextFile.h"
#include "KWMTDatabaseTextFile.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWClassDomain.h"

////////////////////////////////////////////////////////////
// Classe KWClassBuilderView
//    Extraction d'un dictionnaire a partir d'une table de donnees
//     Possibilite de construire iterativement plusieurs dictionnaires
//     vers un meme fichier de dictionnaire
class KWClassBuilderView : public UICard
{
public:
	// Constructeur
	KWClassBuilderView();
	~KWClassBuilderView();

	// Acces aux specifications de la base origine
	KWSTDatabaseTextFile* GetSourceDataTable();

	// Reimplementation de la methode Open
	// Reinitialisations des statistiques de construction de dictionnaires
	void Open() override;

	// Action de construction de dictionnaire, avec suivi de tache
	void BuildClass();

	// Nombre de dictionnaire construits
	int GetBuildClassNumber() const;

	// Nom du dernier dictionnaire construit
	const ALString& GetLastBuildClassName() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Recherche d'une classe de meme composition parmi les classes existantes
	// Permet de detecter si on construit deux fois une classe a partir des donnees
	// Retourne null si non trouve
	KWClass* SearchSimilarClass(const KWClass* kwcSourceClass) const;

	// Memorisation des donnees gerees par la fiche
	KWSTDatabaseTextFile sourceDataTable;
	UIList* classNameList;
	int nBuildClassNumber;
	ALString sLastBuildClassName;
};
