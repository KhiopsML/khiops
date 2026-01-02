// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassDomain.h"
#include "KWDatabase.h"
#include "KWDatabaseView.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWMTDatabaseTextFileView.h"
#include "KWDatabaseTransferTask.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "KWResultFilePathBuilder.h"

////////////////////////////////////////////////////////////
// Classe KWDatabaseTransferView
//    Parametrage du transfert des instances d'un
//     fichier de donnees origine vers un fichier destination
//     en utilisant un dictionnaire pour filtrer les attributs
class KWDatabaseTransferView : public UICard
{
public:
	// Constructeur
	KWDatabaseTransferView();
	~KWDatabaseTransferView();

	// Initialisation des specifications du fichier d'origine a presenter a l'utilisateur
	// Si NULL, reinitialisation de toutes les valeurs a vide
	// La classe assure sinon la permanence des dernieres saisies utilisateurs
	// entre deux ouvertures successives
	// Le chemin des sortie est base en priorite sur celui de la base en sortie, puis celui de la base
	// en entree, puis celui du fichier dictionnaire
	void InitializeSourceDatabase(KWDatabase* database, const ALString& sDatabaseClassFileName);

	// Reimplementation de la methode Open
	void Open() override;

	// Action de transfert (avec suivi de progression de la tache)
	void TransferDatabase();

	// Action de construction d'un dictionnaire de la base transferee
	void BuildTransferredClass();

	// Redefinition de la methode de mise a jour des donnees, pour synchroniser
	// le dictionnaire associe aux bases source et cible
	void EventUpdate() override;
	void EventRefresh() override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Indique si le format de sortie est dense
	boolean IsDenseOutputFormat() const;

	// Recherche et si necessaire creation d'une classe transferee dans le domaine transfere
	// a partir d'une classe source
	// Le transfer se fait au format sparse ou dense selon les specifications de l'interface
	KWClass* InternalBuildTransferredClass(KWClassDomain* transferredClassDomain, const KWClass* transferClass);

	// Preparation de la classe de transfer (et de sa composition), en ne mettant en Load que les attributs
	// transferables de facon a optimiser le nombre d'attributs calcules a traiter
	void PrepareTransferClass(KWClass* transferClass);

	// Nettoyage de la classe de transfert, en remettant tout en Load
	void CleanTransferClass(KWClass* transferClass);

	// Memorisation des donnees gerees par la fiche
	ALString sClassName;
	ALString sClassFileName;
	KWDatabase* sourceDatabase;
	KWDatabase* targetDatabase;

	// Memorisation des vues sur les bases source et cibles
	KWDatabaseView* sourceDatabaseView;
	KWDatabaseView* targetDatabaseView;
};
