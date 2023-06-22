// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassSpecView.h"
#include "KWSTDatabaseTextFile.h"
#include "KWMTDatabaseTextFile.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWClassDomain.h"
#include "LMLicenseManager.h"
#include "KWFileKeyExtractorTask.h"
#include "KWMTClassBuilderView.h"

////////////////////////////////////////////////////////////
// Classe KWDataTableKeyExtractorView
//    Parametrage du tri d'une tables (attributs natifs uniquement)
//     d'un fichier de donnees origine vers un fichier destination
//     en utilisant un dictionnaire pour lire le fichier d'originefiltrer les attributs
class KWDataTableKeyExtractorView : public UICard
{
public:
	// Constructeur
	KWDataTableKeyExtractorView();
	~KWDataTableKeyExtractorView();

	// Initialisation des specifications du fichier d'origine
	// a presenter a l'utilisateur
	// Si NULL, reinitialisation de toutes les valeurs a vide
	// La classe assure sinon la permanence des dernieres saisies
	// utilisateurs entre deux ouvertures successives
	void InitializeSourceDataTable(KWDatabase* database);

	// Reimplementation de la methode Open
	void Open() override;

	// Action de tri (avec suivi de progression de la tache)
	void ExtractKeysFromDataTable();

	// Creation d'un dictionnaire multi-table
	void BuildMultiTableClass();

	// Redefinition de la methode de mise a jour des donnees, pour synchroniser
	// le dictionnaire associe aux bases source et cibles
	void EventUpdate() override;

	// Mise a jour des valeurs de l'interface par l'objet, pour gerer les listes d'aide
	void EventRefresh() override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification de la validite des specifications
	// Attenttion: ce n'est pas la methode Check de UICard (qui conditionne l'ouverture de la fenetre)
	boolean CheckSpec(KWSTDatabaseTextFile* workingTargetDataTable) const;

	// Memorisation des donnees gerees par la fiche
	ALString sInputClassName;
	KWSTDatabaseTextFile sourceDataTable;
	KWSTDatabaseTextFile targetDataTable;
};
