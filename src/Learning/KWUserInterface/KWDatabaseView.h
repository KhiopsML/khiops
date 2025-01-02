// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWDatabase.h"

// ## Custom includes

#include "KWDatabaseSpecView.h"
#include "KWMTDatabase.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWDatabaseView
//    Database
// Editeur de KWDatabase
class KWDatabaseView : public UIObjectView
{
public:
	// Constructeur
	KWDatabaseView();
	~KWDatabaseView();

	// Acces a l'objet edite
	KWDatabase* GetKWDatabase();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Acces aux vues pirncipales sur les specifications de la base
	// La vue de Data est generique, car elle differe selon le mode mono-table ou multi-table
	UIObjectView* GetDataView();
	KWDatabaseSamplingView* GetSamplingView();
	KWDatabaseSelectionView* GetSelectionView();

	// Acces a la base
	void SetObject(Object* object) override;
	KWDatabase* GetDatabase();

	// Indique si on est en mode multi-table
	virtual boolean IsMultiTableTechnology() const;

	// Parametrage du nombre de table editable, si le mode le permet
	virtual void SetEditableTableNumber(int nValue);
	virtual int GetEditableTableNumber() const;

	// Passage en mode de lecture basique
	// En mode lecture basique, seule la fiche de saisie Data de la base est visible (avec le detecteur de format),
	// les fiches de Sampling et Selection devenant invisibles
	virtual void ToBasicReadMode();

	// Passage en mode ecriture seulement
	// En mode ecriture seulement, seule la fiche de saisie Data de la base est visible (sans le detecteur de
	// format), les fiches de Sampling et Selection devenant invisibles
	virtual void ToWriteOnlyMode();

	// Specialisation des listes d'aide, en ajoutant le chemin d'acces a la fiche si elle n'est
	// pas destinee a etre ouverte directement, mais en tant que sous-fiche.
	// Par exemple, appeler SetHelpListViewPath("TrainDatabase") si la fiche est une sous-fiche
	// d'identifiant "TrainDatabase" dans la vue principale
	void SetHelpListViewPath(const ALString& sViewPath);
	const ALString GetHelpListViewPath();

	////////////////////////////////////////////////////////////////////////
	// Fonctionnalite avancee
	// La vue sur la base de train permet d'editer la base de test en etant parametre par:
	//  . la base de test
	//  . la vue sur la base de test
	//  . une nouvelle action FillTestDatabaseSettingsAction
	//  . une nouvelle action InspectTestDatabaseSettingsAction
	//  . un nouveau champ TestDatabaseSpecificationMode indiquant le type de specification de la base de test:
	//     . Complementary: on prend le complementaire de la base de train, et l'inspection est en read-only
	//     . Specific: on peux avoir une base de test specific, et l'inspection est en read-write
	//     . None: il n'y a pas de base de test, et l'inspection donne vide en mode read-only
	// La vue sur la base de test permet d'importer les specifications de la base de train en etant parametre par:
	//  . la base de train
	//  . une nouvelle action FillTestDatabaseSettingsAction
	//  . une nouvelle action ImportTrainDatabaseSettingsAction
	//  . un nouveau champ TestDatabaseSpecificationMode indiquant le type de specification de la base de test, en
	//  read-only

	// Parametrage de la base d'apprentissage
	// Memoire: appartient a l'appelant
	void SetTrainDatabase(KWDatabase* trainDatabaseSettings);
	KWDatabase* GetTrainDatabase();

	// Parametrage de la base de test
	// Memoire: appartient a l'appelant
	void SetTestDatabase(KWDatabase* testDatabaseSettings);
	KWDatabase* GetTestDatabase();

	// Parametrage de la vue sur la base de test
	// Memoire: appartient a l'appelant
	void SetTestDatabaseView(KWDatabaseView* testDatabaseViewSettings);
	KWDatabaseView* GetTestDatabaseView();

	// Ajout d'une action d'import/export des parametres entre une base
	// d'apprentissage et une base de test, a specifier avant l'ouverture de la fenetre
	// Les parametres en test sont ceux de l'apprentissage, avec
	// inversion du parametre "DiscardMode"
	void AddFillTestDatabaseSettingsAction();
	void AddImportTrainDatabaseSettingsAction();

	// Ajout d'une action d'inspection des parametres de la base de test
	void AddInspectTestDatabaseSettingsAction();

	// Ajout d'un champ memorisant le type de specification de la base de test
	void AddTestDatabaseSpecificationMode();

	// Mise a jour de la base de test en fonction du choix de type de base de test:
	//  . Complementary: on prend le complementaire de la base de train, et l'inspection est en read-only
	//  . Specific: on peux avoir une base de test specific, et l'inspection est en read-write
	//  . None: il n'y a pas de base de test, et l'inspection donne vide en mode read-only
	void UpdateTestDatabase();

	// Action d'inspection des parametres de la base de test
	// Le comportement est parametre par le choix de type de base de test:
	//  . Complementary: on prend le complementaire de la base de train, et l'inspection est en read-only
	//  . Specific: on peux avoir une base de test specific, et l'inspection est en read-write
	//  . None: il n'y a pas de base de test, et l'inspection donne vide en mode read-only
	void InspectTestDatabaseSettings();

	// Action d'import/export entre base d'apprentissage et de test
	// (sans effet si celles-ci ne sont pas parametrees
	void FillTestDatabaseSettings();
	void ImportTrainDatabaseSettings();

	//////////////////////////////////////////////////////////////////
	// Administration des vues sur les technologies de KWDatabase
	// L'objectif est de pouvoir implementer un service d'apprentissage
	// utilisant une base de donnees independamment de sa technologie

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWDatabaseView* KWSpecificDatabaseView::Create() const
	//      {
	//          return new KWSpecificDatabaseView;
	//      }
	virtual KWDatabaseView* Create() const;

	// Nom d'une technologie de base de donnees editee ("" par defaut)
	// Methodes a reimplementer dans les technologies reellement instanciables
	virtual ALString GetTechnologyName() const;

	// Creation d'une vue sur la technologie de base de donnees par defaut
	// Prerequis: la technologie correspondant doit etre enregistree ainsi que sa vue
	static KWDatabaseView* CreateDefaultDatabaseTechnologyView();

	// Enregistrement dans la base des KWDatabaseView
	// Il ne doit pas y avoir deux view enregistrees avec le meme nom
	// Memoire: les vues enregistrees sont gerees par l'appele
	static void RegisterDatabaseTechnologyView(KWDatabaseView* databaseView);

	// Recherche par cle
	// Retourne NULL si absent
	static KWDatabaseView* LookupDatabaseTechnologyView(const ALString& sTechnologyName);

	// Recherche par cle et duplication
	// Permet d'obtenir un KWDatabaseView pret a etre instancie
	// Retourne NULL si absent
	static KWDatabaseView* CloneDatabaseTechnologyView(const ALString& sTechnologyName);

	// Export de toutes les vues de technologies de base de donnees enregistrees
	// Memoire: le contenu du tableau en retour appartient a l'appele
	static void ExportAllDatabaseTechnologyViews(ObjectArray* oaDatabaseViews);

	// Destruction de tous les vues sur les technologies de base de donnees
	static void DeleteAllDatabaseTechnologyViews();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	////////////////////////////////////////////////////////////////////////////
	// Structure de l'interface de saisie des Database
	//
	// KWDatabaseView: classe ancetre generale associee a l'edition d'une KWDatabase
	//   KWSTDatabaseTextFileView: sous classe dediee a KWSTDatabaseTextFile (mono-table)
	//   KWMTDatabaseTextFileView: sous classe dediee a KWMTDatabaseTextFile (multi-table)
	//
	// La fiche KWDatabaseView comporte une sous-fiche de gestion des specifications generiques
	//  KWDatabaseSpecView
	//    - fiche cachee (affichee sans cadre visible)
	//    - trois sous-fiches, pour passage en onglet
	//       - Data: KWSTDatabaseTextFileDataView (mono-table) ou KWMTDatabaseTextFileDataView (multi-table)
	//       - Sampling: KWDatabaseSamplingView
	//       - Selection: KWDatabaseSelectionView
	//
	// Toutes les vues sont des vue generiques sur KWDatabase, sauf:
	//   - KWSTDatabaseTextFileDataView: vue specialisee sur KWSTDatabaseTextFile
	//   - KWMTDatabaseTextFileDataView: vue specialisee sur KWMTDatabaseTextFile
	// Les vues de Data gerent le ou les fichier, plus le detecteur de format, plus le format tabulaire

	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// Gestion des listes d'aide
	KWClassAttributeHelpList selectionAttributeHelpList;
	KWDatabaseAttributeValuesHelpList dbSelectionValuesHelpList;

	// Parametres local des liste d'aide pour l'attribut et ka valeur de selection
	static const ALString sDefaultSelectionAttributeParameters;
	static const ALString sDefaultSelectionValueParameters;

	// Bases d'apprentissage et de test
	KWDatabase* trainDatabase;
	KWDatabase* testDatabase;

	// Dernier mode de specification utilise pour la base de test
	ALString sLastTestDatabaseSpecificationMode;

	// Dernier classe utilisee pour la base de test
	ALString sLastTestDatabaseClassName;

	// Vue sur la base de test
	KWDatabaseView* testDatabaseView;

	// Administration des KWDatabaseView
	static ObjectDictionary* odDatabaseTechnologyViews;

	// ##
};

// ## Custom inlines

// ##
