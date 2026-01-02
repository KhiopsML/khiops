// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWSortAttributeNameArrayView.h"
#include "KWClassSpecView.h"
#include "KWSTDatabaseTextFile.h"
#include "KWMTDatabaseTextFile.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWFileSorter.h"
#include "KWClassDomain.h"
#include "KWClassAttributeHelpList.h"

////////////////////////////////////////////////////////////
// Classe KWDataTableSorterView
//    Parametrage du tri d'une tables (attributs natifs uniquement)
//     d'un fichier de donnees origine vers un fichier destination
//     en utilisant un dictionnaire pour lire le fichier d'origine
//     et identifier les attributs natifs
class KWDataTableSorterView : public UICard
{
public:
	// Constructeur
	KWDataTableSorterView();
	~KWDataTableSorterView();

	// Initialisation des specifications du fichier d'origine
	// a presenter a l'utilisateur
	// Si NULL, reinitialisation de toutes les valeurs a vide
	// La classe assure sinon la permanence des dernieres saisies
	// utilisateurs entre deux ouvertures successives
	void InitializeSourceDataTable(KWDatabase* database);

	// Reimplementation de la methode Open
	void Open() override;

	// Action de tri (avec suivi de progression de la tache)
	void SortDataTableByKey();

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
	// Rafraichissement d'une liste d'aide sur les attributs de la classe courante pouvant servir de cle
	void RefreshHelpLists();

	// Verification des noms des attributs
	boolean Check(const ObjectArray* oaCheckedSortAttributeNames) const;

	// Gestion de la liste d'aide
	KWClassAttributeHelpList sortAttributeHelpList;

	// Memorisation des donnees gerees par la fiche
	ALString sSortClassName;
	ObjectArray oaSortAttributeNames;
	KWSTDatabaseTextFile sourceDataTable;
	KWSTDatabaseTextFile targetDataTable;
};
