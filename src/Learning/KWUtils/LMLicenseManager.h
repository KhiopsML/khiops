// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class LMLicenseManager;
class LMLicense;

#include "Standard.h"
#include "Object.h"
#include "UserInterface.h"
#include "KWVersion.h"
#include "LMLicenseService.h"
#include "Timer.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// Gestion de licenses applicatives
// Fonctions statiques, les plus simples possibles a utiliser
// Les probleme sont diagnostiques via des messages utilisateurs
class LMLicenseManager : public Object
{
public:
	//////////////////////////////////////////////////////////////////////
	// Initialisation et terminaison, a faire une seule fois
	// en debut et fin de programme

	// Initialisation du manager de license, a faire en debut de programme
	// Erreur si probleme, avec affichage du probleme
	static boolean Initialize();
	static boolean IsInitialized();

	// Terminaison globale, a appeler pour liberer les ressources en fin de programme
	static void Close();

	//////////////////////////////////////////////////////////////////////
	// Declaration des licences, a faire apres l'initialisation
	// Les licences sont  decrites par:
	//   Id: identifiant numerique permettant de les referencer dans les methodes
	//   Label: libelle utilisateur pour les messages
	//   Name: nom interne pour la protection
	//   Version: numero de version interne pour la protection
	//
	// Une application est protegee avec une et une seule licence principale (ex: Khiops)

	// Licences predefinies
	// Il y en a une par application de la famille Khiops
	enum
	{
		Khiops,               // Licence Khiops
		KhiopsCoclustering,   // Licence Khiops coclustering
		KhiopsEnneade,        // Licence Enneade
		KhiopsInterpretation, // Licence Khiops interpretation
		KhiopsPatatext,       // Licence Patatext
		KhiopsKhiolog,        // Licence Khiolog
		KhiopsKhiotree,       // Licence Khiotree
		KhiopsUplift,         // Licence Uplift
		KhiopsKhiorule,       // Licence Khiorule
		FirstFreeId           // Id de depart en cas de definition d'autres fonctionnalites a proteger
	};

	// Declaration d'une licence predefinie
	// Une et une seule licence doit etre declaree
	static void DeclarePredefinedLicense(int nLicenseId);

	// Fonction avancee: declaration d'une nouvelle licence
	static void DeclareLicense(int nLicenseId, const ALString& sLicenseLabel, const ALString& sLicenseName,
				   const ALString& sLicenseVersion);

	// Supression de toutes les licences
	static void DeleteAllLicenses();

	//////////////////////////////////////////////////////////////////////
	// Gestion des licences
	// Usage typique:
	//  Appeler ShowLicenseStatus en debut de programme, apres initialisation du manager et declaration des licences
	//  Creer une action dont l'implementation appelle OpenLicenseManagementCard dans le constructeur d'une fenetre
	//   pour ajouter un menu de gestion des licenses

	// Affichage des infos de status rapide sur les licenses
	// Infos uniquement si licence active avec date d'expiration inferieure a 30 jour ou si aucune licence active
	// Pas d'info sinon
	static void ShowLicenseStatus();

	// Fonctionnalites avancees: implementation des actions de menu
	// Affichage dans la fenetre de log des informations completes sur la licence sur le processus d'obtention de
	// licence
	static void ShowLicenseFullInformation();

	// Fonctionnalites avancees: implementation des actions de menu
	// Mise a jour d'un fichier de license, en recopiant les lignes valide du fichier en parametre
	// dans le repertoire de gestion des licenses
	static boolean UpdateLicenseFile();

	// Mise a jour d'un fichier de license a partir du fichier passe en parametre
	static boolean UpdateLicenseFromFile(const ALString& sFilePath);

	// Renvoie le nombre de jours avant la date d'expiration
	// Renvoie 0 si la licence n'est pas active
	static int GetRemainingDays();

	// Fonctionnalites avancees: ouverture d'une fiche avec les informations completes sur la licence,
	// sur le processus d'obtention de licence, et ayant un bouton de mise a jour directe des licences
	static void OpenLicenseManagementCard();

#ifdef DEPRECATED_V10
	// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite ascendante des
	// scenarios Gestion des licenses dans un item de menu, ayant deux sous-menu (infos de licenses et update de
	// license) Il suffit d'appeler cette methode en fin de constructeur d'une fenetre pour ajouter l'item de menu
	// de gestion des licenses L'ajout est effectif uniquement s'il y a des licences crees
	static void DEPRECATEDAddLicenseManagementMenu(UIObjectView* parentView);
#endif // DEPRECATED_V10

	//////////////////////////////////////////////////////////////////////////////
	// Execution controlee d'une fonctionnalite sous licence
	// Processus
	//   . demander une cle de licence pour une fonctionnalite
	//   . si OK:
	//      . executer la fonctionnalite
	//      . relacher la cle
	//   . si KO:
	//      . affiche un message base sur le libelle de la fonctionnalite

	// Demande de cle de licence pour une fonctionnalite protegee par licence
	// Ok si licence trouvee et non expiree
	// Si KO, affiche un message d'erreur
	static boolean RequestLicenseKey();

	//////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class LMLicenseManagementView;

	// Initialisation d'une licence predefinie
	static void InitializePredefinedLicense(LMLicense* license, int nLicenseId);
	static void InitializeLicense(LMLicense* license, int nId, const ALString& sLabel, const ALString& sName,
				      const ALString& sVersion);

	// Demande de cle de licence
	// Ok si licence trouvee et non expiree
	// Si KO, affiche un message d'erreur si demande
	static boolean InternalRequestLicenseKey(int nLicenseId, boolean bShowMessages);

	// Redefinition des methodes de LMLicenseService, en integrant plusieurs et une temporisation
	// On fait plusieurs essais, pour le cas ou il y aurait des conflits d'acces aux
	// ressources systemes permettant de calcul la cle de la machine
	static boolean LicenseServiceRequestProductKey(const ALString& sFeatureName, const ALString& sFeatureVersion);
	static boolean LicenseServiceGetProductKeyExpirationDate(const ALString& sFeatureName,
								 const ALString& sFeatureVersion,
								 ALString& sExpirationDate);

	// Nombre de jour max avant expiration des licences, pour les messages d'information
	static const int nMaxRemainingDays = 30;

	///////////////////////////////////////////////////////////////////////////
	// Gestion d'une instance, pour gerer un pattern singleton pour le manager

	// Singleton permettant de piloter la creation et destruction du manager
	static LMLicenseManager singleton;

	// Constructeur
	LMLicenseManager();
	~LMLicenseManager();

	// Recherche de la licence
	// Renvoie NULL si non trouve
	LMLicense* GetLicense();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Donnes d'instance
	boolean bIsInitialized;
	LMLicense* globalLicense;
};

///////////////////////////////////////////////////////////////////////////////////////////////
// Classe interne pour la definition d'une license
class LMLicense : public Object
{
public:
	LMLicense();
	~LMLicense();

	//  Id: identifiant numerique permettant de les referencer dans les methodes
	void SetId(int nValue);
	int GetId() const;

	// Libelle utilisateur pour les messages
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Nom interne pour la protection
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

	// Numero de version interne pour la protection
	void SetVersion(const ALString& sValue);
	const ALString& GetVersion() const;

	// Indique si la license a ete demandee au moins une fois
	void SetRequested(boolean bValue);
	boolean GetRequested() const;

	// Indique si la license est valide
	void SetValid(boolean bValue);
	boolean GetValid() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////
	///// Implementation
protected:
	int nId;
	ALString sLabel;
	ALString sName;
	ALString sVersion;
	boolean bRequested;
	boolean bValid;
};

///////////////////////////////////////////////////////////////////////////////////////////////
// Classe interne pour la gestion des licenses par une boite de dialogue
class LMLicenseManagementView : public UICard
{
public:
	LMLicenseManagementView();
	~LMLicenseManagementView();

	// Ouverture d'une boite de dialogue pour enregister un fichier de license
	void ActionUpdateLicenseFile();

	//////////////////////////////////////////////
	///// Implementation
protected:
	// Initialisation du contenu de la fiche
	void InitializeCardContent();

	// Mise a jour du contenu de la fiche
	void UpdateCardContent();
};

#ifdef DEPRECATED_V10
// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite ascendante des scenarios
///////////////////////////////////////////////////////////////////////////////////////////////
// Classe interne pour la gestion des licenses par une menu
class LMDEPRECATEDLicenseManagerView : public UICard
{
public:
	LMDEPRECATEDLicenseManagerView();
	~LMDEPRECATEDLicenseManagerView();

	// Affichage dans la fenetre de log des infos completes sur la gestion des licenses
	//  MachineId
	//  Liste des licences et leur status
	//  Message sur la facon d'obtenir une license
	void ActionShowLicenseFullInformation();

	// Ouverture d'une boite de dialogue pour enregister un fichier de license
	void ActionUpdateLicenseFile();
};
#endif // DEPRECATED_V10
