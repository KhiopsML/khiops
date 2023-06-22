// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "LMLicenseManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// Classe LMLicenseManager

boolean LMLicenseManager::Initialize()
{
	ALString sErrorMessage;
	require(not IsInitialized());

	singleton.bIsInitialized = LMLicenseService::Initialize();
	if (not singleton.bIsInitialized)
		singleton.AddError("Failed to initialize license manager");

	return singleton.bIsInitialized;
}

boolean LMLicenseManager::IsInitialized()
{
	return singleton.bIsInitialized;
}

void LMLicenseManager::Close()
{
	LMLicenseService::Close();
	singleton.bIsInitialized = false;
}

void LMLicenseManager::DeclarePredefinedLicense(int nLicenseId)
{
	LMLicense license;
	require(0 <= nLicenseId and nLicenseId < FirstFreeId);

	InitializePredefinedLicense(&license, nLicenseId);
	DeclareLicense(license.GetId(), license.GetLabel(), license.GetName(), license.GetVersion());
}

void LMLicenseManager::DeclareLicense(int nLicenseId, const ALString& sLicenseLabel, const ALString& sLicenseName,
				      const ALString& sLicenseVersion)
{
	LMLicense* license;
#ifdef __ANDROID__
	return;
#endif // __ANDROID
	require(singleton.GetLicense() == NULL);

	// Creation, initialisation et memorisation de la licence
	license = new LMLicense;
	license->SetId(nLicenseId);
	license->SetLabel(sLicenseLabel);
	license->SetName(sLicenseName);
	license->SetVersion(sLicenseVersion);

	// La seule licence est a mettre en debut de tableau
	singleton.globalLicense = license;
}

void LMLicenseManager::DeleteAllLicenses()
{
	if (singleton.globalLicense != NULL)
	{
		delete singleton.globalLicense;
		singleton.globalLicense = NULL;
	}
}

void LMLicenseManager::ShowLicenseStatus()
{
#ifdef __ANDROID__
	return;
#endif // __ANDROID

	LMLicense* license;
	boolean bOk;
	boolean bLicenseFound;
	ALString sExpirationDate;
	int nRemainingDays;
	boolean bWarnings;
	ALString sTmp;

	// On ne montre les infos que s'il y a eu initialisation du gestionnaire de license
	// et des infos de date par license
	bWarnings = false;
	bLicenseFound = false;
	if (singleton.IsInitialized())
	{
		// Premiere passe de detection des licences actives
		license = singleton.GetLicense();
		if (license != NULL)
		{
			// Recherche de la date d'expiration de la licence
			bOk = LicenseServiceGetProductKeyExpirationDate(license->GetName(), license->GetVersion(),
									sExpirationDate);
			if (bOk)
			{
				bLicenseFound = true;

				// Calcul du nombre de jours restant avant expiration
				nRemainingDays = LMLicenseService::GetRemainingDays(sExpirationDate);

				// Message s'il reste peu de jours
				if (nRemainingDays <= nMaxRemainingDays)
				{
					bWarnings = true;

					// Si license expiree, on affichage la date d'expiration
					if (nRemainingDays < 0)
						license->AddWarning(sTmp + "License expired at " + sExpirationDate);
					// Sinon, on affiche le nombre de jours restant
					else
					{
						if (nRemainingDays <= 1)
							license->AddWarning(sTmp + "License expire in " +
									    IntToString(nRemainingDays) + " day");
						else
							license->AddWarning(sTmp + "License expire in " +
									    IntToString(nRemainingDays) + " days");
					}
				}
			}
		}
	}

	// Message d'erreur si aucune licence trouvee
	if (not bLicenseFound)
	{
		if (singleton.GetLicense() == NULL)
			singleton.AddError("No license found");
		else
			singleton.AddError("No license found for " + singleton.GetLicense()->GetLabel());
	}

	// Messages d'information si necessaire
	if (bWarnings or not bLicenseFound)
	{
		singleton.AddSimpleMessage("");
		ShowLicenseFullInformation();
	}
}

void LMLicenseManager::ShowLicenseFullInformation()
{
#ifdef __ANDROID__
	return;
#endif // __ANDROID

	LMLicense* license;
	boolean bOk;
	ALString sExpirationDate;
	int nRemainingDays;
	ALString sTmp;

	// Numero de version de Khiops
	singleton.AddSimpleMessage(GetLearningShellBanner());
	singleton.AddSimpleMessage("");

	// Info sur le license manager si initialisation OK
	singleton.AddSimpleMessage("\tLicense directory: " + LMLicenseService::GetLicenseFileDirectory());
	singleton.AddSimpleMessage("\tLicense file: " + LMLicenseService::GetLicenseFileName());

	// Ouvre a nouveau gestionnaire de licences pour avoir un diagnostic si necessaire
	LMLicenseManager::Close();
	LMLicenseManager::Initialize();

	// Infos completes par licence
	singleton.AddSimpleMessage("");
	license = singleton.GetLicense();
	if (license != NULL)
	{
		// Recherche de la date d'expiration de la licence, avec message si erreur
		bOk = LicenseServiceGetProductKeyExpirationDate(license->GetName(), license->GetVersion(),
								sExpirationDate);

		// Erreur si non trouvee pour licence principale
		if (not bOk)
		{
			license->AddMessage("No license found");
		}
		// Informations de date si pas d'erreur
		// Informations y compris sur licence secondaire si elle a ete installe au moins une fois
		else
		{
			// Calcul du nombre de jours restant avant expiration
			nRemainingDays = LMLicenseService::GetRemainingDays(sExpirationDate);

			if (nRemainingDays < 0)
				license->AddMessage(sTmp + "License expired at " + sExpirationDate);
			// Sinon, on affiche le nombre de jours restant
			else
			{
				if (nRemainingDays <= 1)
					license->AddMessage(sTmp + "License expire at " + sExpirationDate + " in " +
							    IntToString(nRemainingDays) + " day");
				else if (sExpirationDate == LMLicenseService::GetPerpetualLimitDate())
					license->AddMessage(sTmp + "Perpetual license");
				else
					license->AddMessage(sTmp + "License expire at " + sExpirationDate + " in " +
							    IntToString(nRemainingDays) + " days");
			}
		}
	}

	// Affichage du processus pour obtenir une licence
	singleton.AddSimpleMessage("");
	singleton.AddSimpleMessage("\tComputer name\t" + LMLicenseService::GetComputerName());
	singleton.AddSimpleMessage("\tMachine ID\t" + LMLicenseService::GetMachineID());
	singleton.AddSimpleMessage("To obtain or renew your license, go to www.khiops.com with");
	singleton.AddSimpleMessage("  your computer name and machine ID, and ask for a license file.");
	singleton.AddSimpleMessage("The web site will send you an e-mail with the license file.");
	singleton.AddSimpleMessage("Go in the license management menu to update your license.");
	singleton.AddSimpleMessage("");
}

boolean LMLicenseManager::UpdateLicenseFile()
{
	boolean bOk;
	LMLicense* license;
	UIFileChooserCard registerCard;
	ALString sSourceLicenseFilePath;

	// Reinitialisation des informations de validite collectee sur la licence
	license = singleton.GetLicense();
	if (license != NULL)
	{
		license->SetRequested(false);
		license->SetValid(false);
	}

	// Ouverture du FileChooser
	registerCard.SetApproveActionHelpText("Register new licenses in license directory.\n"
					      "All valid licenses relative to the current machine are updated, with "
					      "update messages only the current application.");
	sSourceLicenseFilePath = registerCard.ChooseFile("License manager", "Update license file", "FileChooser",
							 "License\nlic", "NewLicenseFile", "New license file", "");

	// Sauvegarde du fichier de license
	bOk = false;
	if (sSourceLicenseFilePath != "")
	{
		bOk = UpdateLicenseFromFile(sSourceLicenseFilePath);
	}
	return bOk;
}

boolean LMLicenseManager::UpdateLicenseFromFile(const ALString& sSourceLicenseFilePath)
{
	boolean bOk;
	int i;
	LMLicense* license;
	ALString sLicensesAndVersions;
	ALString sErrorMessage;
	IntVector ivIds;
	StringVector svFeatures;
	StringVector svVersions;
	IntVector ivFoundFeatures;
	IntVector ivUpdatedFeatures;
	int nLicenceNumber;
	LMLicense predefinedLicense;
	boolean bFoundLicences;
	boolean bUpdatedLicences;

	// Test prealable d'existence du fichier de license
	bOk = (FileService::Exist(sSourceLicenseFilePath));
	if (not bOk)
		singleton.AddError("License file " + sSourceLicenseFilePath + " not found");

	if (not bOk)
		singleton.AddError("Failed to update license file");

	// Enregistrement du fichier de license
	if (bOk)
	{
		// Collecte de toutes les infos de licences predefinies, pour permettre la mise jour de toutes les
		// licences valides pour la machine
		nLicenceNumber = 0;
		for (i = 0; i < FirstFreeId; i++)
		{
			InitializePredefinedLicense(&predefinedLicense, i);

			// Memorisation dans un tableau du nom et de la version
			ivIds.Add(predefinedLicense.GetId());
			svFeatures.Add(predefinedLicense.GetName());
			svVersions.Add(predefinedLicense.GetVersion());
			ivUpdatedFeatures.Add(0);
			nLicenceNumber++;
		}

		// Collecte des infos de licence de l'application, avec potentiellement des licences non predefinies
		nLicenceNumber = 0;
		license = singleton.GetLicense();
		if (license != NULL)
		{
			// Memorisation dans un tableau du nom et de la version si licence non predefinie
			if (license->GetId() >= FirstFreeId)
			{
				ivIds.Add(license->GetId());
				svFeatures.Add(license->GetName());
				svVersions.Add(license->GetVersion());
				ivUpdatedFeatures.Add(0);
				nLicenceNumber++;
			}
		}

		// Mise a jour du fichier de licence
		bOk = LMLicenseService::UpdateLicenseFile(sSourceLicenseFilePath, &svFeatures, &svVersions,
							  &ivFoundFeatures, &ivUpdatedFeatures);

		// Messages uniquement sur les licences applicatives mises a jour
		bUpdatedLicences = false;
		if (bOk)
		{
			for (i = 0; i < ivUpdatedFeatures.GetSize(); i++)
			{
				// Test si licence mise a jour
				if (ivUpdatedFeatures.GetAt(i) == 1)
				{
					// Message si licence applicative
					license = singleton.GetLicense();
					if (license != NULL and license->GetId() == ivIds.GetAt(i))
					{
						license->AddMessage("License updated");
						bUpdatedLicences = true;
					}
				}
			}
		}

		// Recherche si des licence ont ete trouvee si pas de mise a jour
		bFoundLicences = bOk;
		if (not bOk)
		{
			for (i = 0; i < ivFoundFeatures.GetSize(); i++)
			{
				// Test si licence mise a jour
				if (ivFoundFeatures.GetAt(i) == 1)
				{
					// Message si licence applicative
					license = singleton.GetLicense();
					if (license != NULL and license->GetId() == ivIds.GetAt(i))
					{
						bFoundLicences = true;
						break;
					}
				}
			}
		}

		// Message synthetique
		if (bOk)
		{
			// Cas ou des licences applicatives ont ete mise a jour
			if (bUpdatedLicences)
				singleton.AddMessage("Succeeded to update license file");
			// Cas ou des licences "cachees" on ete mise a jour"
			else
				singleton.AddWarning("No license found for current application");
		}
		else
		{
			// Cas ou aucune licence valide n'a ete trouvee
			if (not bFoundLicences)
				singleton.AddWarning("No valid license found for current application");
			// Cas ou aucune licence recente n'a ete trouvee
			else
				singleton.AddWarning("No recent license found for current application");
		}
	}
	return bOk;
}

int LMLicenseManager::GetRemainingDays()
{
	LMLicense* license;
	boolean bOk;
	ALString sExpirationDate;
	int nRemainingDays;
	ALString sTmp;

	// Ouvre a nouveau gestionnaire de licences pour avoir un diagnostic si necessaire
	LMLicenseManager::Close();
	LMLicenseManager::Initialize();

	// Infos completes par licence
	bOk = true;
	nRemainingDays = 0;
	license = singleton.GetLicense();
	if (license != NULL)
	{
		// Recherche de la date d'expiration de la licence, avec message si erreur
		bOk = LicenseServiceGetProductKeyExpirationDate(license->GetName(), license->GetVersion(),
								sExpirationDate);

		// Informations de date si pas d'erreur
		// Informations y compris sur licence secondaire si elle a ete installe au moins une fois
		if (bOk)
		{
			// Calcul du nombre de jours restant avant expiration
			nRemainingDays = LMLicenseService::GetRemainingDays(sExpirationDate);

			if (nRemainingDays < 0)
				bOk = false;
		}
	}
	if (bOk)
		return nRemainingDays;
	else
		return 0;
}

void LMLicenseManager::OpenLicenseManagementCard()
{
#ifdef __ANDROID__
	return;
#endif // __ANDROID

	LMLicenseManagementView licenceManagementView;
	licenceManagementView.Open();
}

#ifdef DEPRECATED_V10
void LMLicenseManager::DEPRECATEDAddLicenseManagementMenu(UIObjectView* parentView)
{
	require(parentView != NULL);

	// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite ascendante des
	// scenarios
	parentView->AddCardField("LicenseManager", "License management", new LMDEPRECATEDLicenseManagerView);
	parentView->GetFieldAt("LicenseManager")->SetVisible(false);
}
#endif // DEPRECATED_V10

boolean LMLicenseManager::RequestLicenseKey()
{
#ifdef __ANDROID__
	return true;
#endif // __ANDROID__

	boolean bOk;
	LMLicense* license;

	// Recherche de la licence
	license = singleton.GetLicense();
	bOk = (license != NULL);
	assert(bOk);

	// Test si license disponible pour la licence
	if (bOk)
		bOk = InternalRequestLicenseKey(license->GetId(), true);
	return bOk;
}

void LMLicenseManager::InitializePredefinedLicense(LMLicense* license, int nLicenseId)
{
	boolean bIsV9 = false;
	boolean bIsV10 = true;
	ALString sTmp;

	require(license != NULL);
	require(0 <= nLicenseId and nLicenseId < FirstFreeId);

	if (nLicenseId == Khiops)
		InitializeLicense(license, Khiops, "Khiops", "khiops", "8.0");
	else if (nLicenseId == KhiopsCoclustering)
		InitializeLicense(license, KhiopsCoclustering, "Khiops coclustering",
				  sTmp + "khiops" + '-' + "coclustering", "8.0");
	else if (nLicenseId == KhiopsEnneade)
		InitializeLicense(license, KhiopsEnneade, "Enneade", sTmp + "khiops" + '-' + "enneade", "8.0");
	else if (nLicenseId == KhiopsInterpretation)
		InitializeLicense(license, KhiopsInterpretation, "Khiops interpretation",
				  sTmp + "khiops" + '-' + "interpretation", "8.0");
	else if (nLicenseId == KhiopsPatatext)
		InitializeLicense(license, KhiopsPatatext, "Patatext", sTmp + "khiops" + '-' + "patatext", "8.0");
	else if (nLicenseId == KhiopsKhiolog)
		InitializeLicense(license, KhiopsKhiolog, "KhioLog", sTmp + "khiops" + '-' + "khiolog", "8.0");
	else if (nLicenseId == KhiopsKhiotree)
		InitializeLicense(license, KhiopsKhiotree, "KhioTree", sTmp + "khiops" + '-' + "khiotree", "8.0");
	else if (nLicenseId == KhiopsUplift)
		InitializeLicense(license, KhiopsUplift, "Khiops uplift", sTmp + "khiops" + '-' + "uplift", "8.0");
	else if (nLicenseId == KhiopsKhiorule)
		InitializeLicense(license, KhiopsKhiorule, "KhioRule", sTmp + "khiops" + '-' + "khiorule", "8.0");

	// Modification de l'encodage de la licence pour la version 9
	if (bIsV9)
	{
		license->SetLabel(license->GetLabel() + " 9");
		license->SetName(license->GetName() + '_' + '9' + '@' + "Orange" + "Labs" + "2018");
		license->SetVersion("9.0.0");
	}

	// Modification de l'encodage de la licence pour la version 10
	if (bIsV10)
	{
		license->SetLabel(license->GetLabel() + " 10");
		license->SetName(sTmp + "Orange" + "LAN" + 'N' + "ION" + IntToString(2021) + ':' + license->GetName() +
				 '_' + 'T' + 'E' + 'N');
		license->SetVersion("10.0.0");
	}
}

void LMLicenseManager::InitializeLicense(LMLicense* license, int nId, const ALString& sLabel, const ALString& sName,
					 const ALString& sVersion)
{
	require(license != NULL);
	license->SetId(nId);
	license->SetLabel(sLabel);
	license->SetName(sName);
	license->SetVersion(sVersion);
}

boolean LMLicenseManager::InternalRequestLicenseKey(int nLicenseId, boolean bShowMessages)
{
	LMLicense* license;
	boolean bOk;
	ALString sErrorMessage;
	ALString sExpirationDate;
	int nRemainingDays;
	boolean bMessageDisplayed;
	ALString sTmp;

	// Recherche des caracteristiques de la licence
	license = singleton.GetLicense();
	bOk = (license != NULL and license->GetId() == nLicenseId);
	assert(bOk);

	// On prend l'indicateur de validite de la licence
	bOk = license->GetValid();

	// On demande la licence qu'une seule fois
	bMessageDisplayed = false;
	if (not bOk and not license->GetRequested())
	{
		// Memorisation que la licence a ete demandee
		license->SetRequested(true);
		assert(not license->GetValid());

		// Test si license disponible pour la licence
		bOk = LicenseServiceRequestProductKey(license->GetName(), license->GetVersion());

		// Message d'erreur si necessaire
		if (not bOk and bShowMessages)
		{
			// Message utilisateur
			license->AddError(sTmp + "No license found");
			bMessageDisplayed = true;
		}

		// Test si licence disponible
		if (bOk)
		{
			// Recherche de la date d'expiration de la licence
			bOk = LicenseServiceGetProductKeyExpirationDate(license->GetName(), license->GetVersion(),
									sExpirationDate);

			// Evaluation de l'expiration de la licence
			if (bOk)
			{
				// Calcul du nombre de jours restant avant expiration
				nRemainingDays = LMLicenseService::GetRemainingDays(sExpirationDate);

				// Message si licence expiree
				if (nRemainingDays < 0 and bShowMessages)
				{
					license->AddError(sTmp + "License expired at " + sExpirationDate);
					bMessageDisplayed = true;
					bOk = false;
				}
			}
		}

		// Message d'information sur la gestion des licences si KO
		if (not bOk and bShowMessages)
		{
			singleton.AddSimpleMessage("");
			ShowLicenseFullInformation();
		}

		// Memorisation de la validite de la licence
		license->SetValid(bOk);
	}

	// Message d'information succsins sur la gestion des licences si KO et pas de message emis
	if (not bOk and bShowMessages and not bMessageDisplayed)
	{
		license->AddError(sTmp + "No valid license found");
	}
	return bOk;
}

boolean LMLicenseManager::LicenseServiceRequestProductKey(const ALString& sFeatureName, const ALString& sFeatureVersion)
{
	boolean bOk;
	int nMaxTrialNumber = 3;
	int nTrial;
	Timer timer;

	// Boucle de tentative de demande de licence
	bOk = true;
	for (nTrial = 0; nTrial < nMaxTrialNumber; nTrial++)
	{
		// Tentative d'obtention de la licence
		bOk = LMLicenseService::RequestProductKey(sFeatureName, sFeatureVersion);
		if (bOk)
			break;

		// Temporisation avant autre tentative
		// (utile egalement pour gener les hackers?)
		timer.Start();
		while (timer.GetElapsedTime() < 0.1)
		{
			int i;
			double dMax;
			double dCurrent;
			dMax = pow(3.141592653, 10.0);
			i = 1;
			dCurrent = 0;
			while (dCurrent < dMax)
			{
				dCurrent += log(i * 1.0);
				i++;
			}
		}
		timer.Stop();
		timer.Reset();
	}
	return bOk;
}

boolean LMLicenseManager::LicenseServiceGetProductKeyExpirationDate(const ALString& sFeatureName,
								    const ALString& sFeatureVersion,
								    ALString& sExpirationDate)
{
	boolean bOk;
	int nMaxTrialNumber = 3;
	int nTrial;
	Timer timer;

	// Boucle de tentative de demande de date d'expiration de la licence
	bOk = true;
	for (nTrial = 0; nTrial < nMaxTrialNumber; nTrial++)
	{
		// Tentative d'obtention de la licence
		bOk = LMLicenseService::GetProductKeyExpirationDate(sFeatureName, sFeatureVersion, sExpirationDate);
		if (bOk)
			break;

		// Temporisation avant autre tentative
		// (utile egalement pour gener les hackers?)
		timer.Start();
		while (timer.GetElapsedTime() < 0.1)
		{
			int i;
			double dMax;
			double dCurrent;
			dMax = pow(3.141592653, 10.0);
			i = 1;
			dCurrent = 0;
			while (dCurrent < dMax)
			{
				dCurrent += log(i * 1.0);
				i++;
			}
		}
		timer.Stop();
		timer.Reset();
	}
	return bOk;
}

LMLicenseManager LMLicenseManager::singleton;

LMLicenseManager::LMLicenseManager()
{
	bIsInitialized = false;
	globalLicense = NULL;
}

LMLicenseManager::~LMLicenseManager()
{
	if (globalLicense != NULL)
		delete globalLicense;
}

LMLicense* LMLicenseManager::GetLicense()
{
	return globalLicense;
}

const ALString LMLicenseManager::GetClassLabel() const
{
	return "License manager";
}

const ALString LMLicenseManager::GetObjectLabel() const
{
	return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Classe LMLicense

LMLicense::LMLicense()
{
	nId = 0;
	bRequested = false;
	bValid = false;
}

LMLicense::~LMLicense() {}

void LMLicense::SetId(int nValue)
{
	require(nValue >= 0);
	nId = nValue;
}

int LMLicense::GetId() const
{
	return nId;
}

void LMLicense::SetLabel(const ALString& sValue)
{
	require(sValue != "");
	sLabel = sValue;
}

const ALString& LMLicense::GetLabel() const
{
	return sLabel;
}

void LMLicense::SetName(const ALString& sValue)
{
	require(sValue != "");
	sName = sValue;
}

const ALString& LMLicense::GetName() const
{
	return sName;
}

void LMLicense::SetVersion(const ALString& sValue)
{
	require(sValue != "");
	sVersion = sValue;
}

const ALString& LMLicense::GetVersion() const
{
	return sVersion;
}

void LMLicense::SetRequested(boolean bValue)
{
	bRequested = bValue;
}

boolean LMLicense::GetRequested() const
{
	return bRequested;
}

void LMLicense::SetValid(boolean bValue)
{
	bValid = bValue;
}

boolean LMLicense::GetValid() const
{
	return bValid;
}

const ALString LMLicense::GetClassLabel() const
{
	return "License";
}

const ALString LMLicense::GetObjectLabel() const
{
	return sLabel;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Classe LMLicenseManagementView

LMLicenseManagementView::LMLicenseManagementView()
{
	// Libelles
	SetIdentifier("LMLicenseManagement");
	SetLabel("License management");

	// Declaration des actions
	AddAction("ActionUpdateLicenseFile", "Update license file...",
		  (ActionMethod)(&LMLicenseManagementView::ActionUpdateLicenseFile));
	GetActionAt("ActionUpdateLicenseFile")->SetStyle("Button");

	// Info-bulles
	GetActionAt("ActionUpdateLicenseFile")
	    ->SetHelpText("Update license file."
			  "\n Opens a dialog box to choose the file containing the license to update.");

	// Short cuts
	GetActionAt("ActionUpdateLicenseFile")->SetShortCut('U');

	// Initialisation du contenu de la fiche
	InitializeCardContent();
}

LMLicenseManagementView::~LMLicenseManagementView() {}

void LMLicenseManagementView::ActionUpdateLicenseFile()
{
	LMLicenseManager::UpdateLicenseFile();

	// Mise a jour du contenu de la fiche, potentiellement changee si la licence a ete prise en compte
	UpdateCardContent();
}

void LMLicenseManagementView::InitializeCardContent()
{
	require(GetFieldNumber() == 0);

	// Affichage des informations en demandant une mise a jour initiale
	AddStringField("Information", "", "");
	GetFieldAt("Information")->SetStyle("FormattedLabel");
	UpdateCardContent();

	// Affichage des informations a utiliser pour obtenir une licence
	AddStringField("ComputerName", "Computer name", LMLicenseService::GetComputerName());
	GetFieldAt("ComputerName")->SetStyle("SelectableLabel");
	AddStringField("MachineID", "Machine ID", LMLicenseService::GetMachineID());
	GetFieldAt("MachineID")->SetStyle("SelectableLabel");

	// Site web
	AddStringField("WebSite", "Web site", "<html> <a href=\"\">www.khiops.com</a> </html>");
	GetFieldAt("WebSite")->SetStyle("UriLabel");
	GetFieldAt("WebSite")->SetParameters("www.khiops.com");
}

void LMLicenseManagementView::UpdateCardContent()
{
	LMLicense* license;
	boolean bOk;
	ALString sExpirationDate;
	int nRemainingDays;
	ALString sInformation;
	ALString sCopyrightLabel;

	require(GetFieldNumber() > 0);
	require(GetFieldIndex("Information") >= 0);

	// Formatage html du copyright
	sCopyrightLabel = GetLearningCopyrightLabel();
	assert(sCopyrightLabel.Find("(c)") == 0);
	sCopyrightLabel = " &copy " + GetLearningCopyrightLabel().Right(GetLearningCopyrightLabel().GetLength() - 3);

	// Informations generales
	sInformation = "<html> ";
	sInformation += "<h3> " + GetLearningFullApplicationName() + " </h3> ";
	sInformation += "<p> Version " + GetLearningVersion() + " </p> ";
	sInformation += "<p> " + sCopyrightLabel + " </p> ";
	sInformation += "<p> </p>";

	// Information sur la licence
	sInformation += "<h3> License information </h3> ";
	sInformation += "<p> Directory: " + LMLicenseService::GetLicenseFileDirectory() + " </p> ";
	sInformation += "<p> File: " + LMLicenseService::GetLicenseFileName() + " </p> ";

	// Ouvre a nouveau gestionnaire de licences pour avoir un diagnostic si necessaire
	LMLicenseManager::Close();
	LMLicenseManager::Initialize();

	// Infos completes par licence
	license = LMLicenseManager::singleton.GetLicense();
	if (license != NULL)
	{
		// Recherche de la date d'expiration de la licence, avec message si erreur
		bOk = LMLicenseManager::LicenseServiceGetProductKeyExpirationDate(
		    license->GetName(), license->GetVersion(), sExpirationDate);

		// Erreur si non trouvee pour licence principale
		if (not bOk)
		{
			sInformation += "<p> No license found </p> ";
		}
		// Informations de date si pas d'erreur
		// Informations y compris sur licence secondaire si elle a ete installe au moins une fois
		else
		{
			// Calcul du nombre de jours restant avant expiration
			nRemainingDays = LMLicenseService::GetRemainingDays(sExpirationDate);
			if (nRemainingDays < 0)
				sInformation += "<p> License expired at " + sExpirationDate + " </p> ";
			// Sinon, on affiche le nombre de jours restant
			else
			{
				if (nRemainingDays <= 1)
					sInformation += "<p> License expire at " + sExpirationDate + " in " +
							IntToString(nRemainingDays) + " day </p> ";
				else if (sExpirationDate == LMLicenseService::GetPerpetualLimitDate())
					sInformation += "<p> Perpetual license </p> ";
				else
					sInformation += "<p> License expire at " + sExpirationDate + " in " +
							IntToString(nRemainingDays) + " days </p> ";
			}
		}
	}

	// Affichage du processus pour obtenir une licence
	sInformation += "<p> </p> ";
	sInformation += "<h3> Installing and Updating License Files </h3> ";
	sInformation += "<p> To obtain or renew your license, go to www.khiops.com with"
			" your 'computer name' and 'machine ID'</p>";
	sInformation += "<p> and ask for a license file.</p>";
	sInformation += "<p> The web site will send you an e-mail with the license file. </p>";
	sInformation += "<p> Then, click on the '" + GetActionAt("ActionUpdateLicenseFile")->GetLabel() +
			"' button to update your license.</p>";
	sInformation += "<p> </p> ";
	sInformation += "<p> </p> ";
	sInformation += "<p> The two fields below are selectable for copy and paste. </p> ";

	// Mise a jour d'un champ infiormation
	sInformation += "</html>";
	SetStringValueAt("Information", sInformation);
}

#ifdef DEPRECATED_V10
///////////////////////////////////////////////////////////////////////////////////////////////
// Classe LMDEPRECATEDLicenseManagerView

LMDEPRECATEDLicenseManagerView::LMDEPRECATEDLicenseManagerView()
{
	// Libelles
	SetIdentifier("LMLicenseManager");
	SetLabel("License management");

	// Declaration des actions
	AddAction("ActionShowLicenseFullInformation", "Show license information",
		  (ActionMethod)(&LMDEPRECATEDLicenseManagerView::ActionShowLicenseFullInformation));
	AddAction("ActionUpdateLicenseFile", "Update license file...",
		  (ActionMethod)(&LMDEPRECATEDLicenseManagerView::ActionUpdateLicenseFile));

	// Info-bulles
	GetActionAt("ActionShowLicenseFullInformation")
	    ->SetHelpText("Shows license information with numbers of days before expiration."
			  "\n Describes the process to apply to get licenses.");
	GetActionAt("ActionUpdateLicenseFile")
	    ->SetHelpText("Update license file."
			  "\n Opens a dialog box to choose the file containing the licenses to update.");
}

LMDEPRECATEDLicenseManagerView::~LMDEPRECATEDLicenseManagerView() {}

void LMDEPRECATEDLicenseManagerView::ActionShowLicenseFullInformation()
{
	// Warning utilisateur
	AddWarning(
	    "Menu 'Show license information' is deprecated since Khiops V10 : use menu 'License manager...' instead");
	LMLicenseManager::ShowLicenseFullInformation();
}

void LMDEPRECATEDLicenseManagerView::ActionUpdateLicenseFile()
{
	AddWarning(
	    "Menu 'Update license file...' is deprecated since Khiops V10: use menu 'License manager...' instead");
	LMLicenseManager::UpdateLicenseFile();
}
#endif // DEPRECATED_V10
