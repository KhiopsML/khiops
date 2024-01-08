// Copyright (c) 2024 Orange. All rights reserved.
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

	// Test si une classe (ou sa composition) contient des blocs d'attributs a transferer
	boolean IsClassWithTransferredAttributeBlocks(const KWClass* transferClass) const;

	// Creation d'un domaine ou la classe a transferer (et les classes de sa composition) sont transformees
	// au format dense, a partir d'une classe source.
	// Tous les attributs sparse utilises sont renommes, mis en unused, et recopies dans des attributs dense
	// de meme nom que les attributs initiaux
	// Cela permet de gerer la transformation d'un format sparse vers un format dense au moyen d'un dictionnaire
	// dedie, plutot qu'au moyen d'un flag a positionner dans les database et a propager dans les tables, drivers,
	// en parallele, ce qui ne serait pas maintenable. On a ici une solution qui reste localisee dans une seule
	// methode
	KWClassDomain* InternalBuildDenseClassDomain(const KWClass* transferClass);

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
