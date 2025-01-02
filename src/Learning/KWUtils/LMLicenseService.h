// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Vector.h"
#include "Ermgt.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// Classe technique pour la gestion de licence
// Classe temporairement rappatriee de Norm V9.1.4, pour pouvoir utiliser Norm.V9.2.1
// (qui n'a plus cette class), dans le cadre du patch de Khiops V10.1.1
class LMLicenseService : public Object
{
public:
	//////////////////////////////////////////////////////////////////
	// Acces au parametrage de la gestion des licences

	// Nom du repertoire contenant le fichier de licence
	// Gere par la variable d'environnement KHIOPS_LICENSE_FILE_DIR
	static const ALString GetLicenseFileDirectory();

	// Nom du fichier de licence
	// Gere par la variable d'environnement KHIOPS_LICENSE_FILE_NAME
	static const ALString GetLicenseFileName();

	//////////////////////////////////////////////////////////////////
	// Initialisation
	// Doit etre effectue une fois pour toute avant toute operation
	// de gestion des licences

	// Initialisation du gestionnaire de licence
	// Renvoie true si l'installation des licences est correcte
	// Emission de message d'erreur sinon
	static boolean Initialize();
	static boolean IsInitialized();

	// Fermeture du gestionnaire de licence
	static boolean Close();

	////////////////////////////////////////////////////////////////////
	// Gestion du fichier de licence

	// Recherche du nom de l'ordinateur
	static const ALString GetComputerName();

	// Recherche de l'identifiant de machine
	static const ALString GetMachineID();

	// Mise a jour du fichier de licence
	// On passe la liste des features a extraire du fichier, et on met a jour les features
	// corespondants du fichier de licence si la date limite est plus recente
	// Met a jour un vecteur de boolean indiquant quelles fonctionnalites ont ete trouvees parmi
	// les licences actuelle, et lesquelle peuvent ete mises a jour (nouvelle date d'expiration)
	// Renvoie false si aucune licence na ete mise a jour (aucune trouvee, ou probleme fichier)
	// Emission de message d'erreur generiques uniquement (lies a la gestion de fichier principalement)
	// Les message par fonctionnalite effectivement mise a jour sont a la charge de l'appelant
	static boolean UpdateLicenseFile(const ALString& sSourceLicenseFilePath, const StringVector* svFeatureNames,
					 const StringVector* svFeatureVersions, IntVector* ivFoundFeatures,
					 IntVector* ivUpdatedFeatures);

	////////////////////////////////////////////////////////////////////
	// Gestion des cles par fonctionnalite et version

	// Demande d'une cle
	// Renvoie false si non trouve (sans message d'erreur)
	static boolean RequestProductKey(const ALString& sFeatureName, const ALString& sFeatureVersion);

	// Obtention d'une date d'expiration, au format "YYYY-MM-DD"
	// Renvoie false si le produit n'est pas trouve quelle que soit sa date d'expiration (sans message d'erreur)
	static boolean GetProductKeyExpirationDate(const ALString& sFeatureName, const ALString& sFeatureVersion,
						   ALString& sExpirationDate);

	// Calcul d'un nombre de jour entre la date courante et une date de reference
	static int GetRemainingDays(const ALString& sExpirationDate);

	// Date d'expiration correspondant a une licence perpetuelle
	static const ALString GetPerpetualLimitDate();

	//////////////////////////////////////////
	///// Implementation
protected:
	// Ajout d'un message d'erreur
	static void AddGlobalError(const ALString& sLabel);

	// Nom d'application a partir d'un nom et numero de verion
	static ALString BuildFeatureFullName(const ALString& sFeatureName, const ALString& sFeatureVersion);

	// Acces au fichier de licence avec chemin complet
	// Renvoie "" si fichier inexistant (et emet un message utilisateur en mode non silencieux)
	static const ALString FindLicenseFilePath(boolean bSilent);

	// Acces au repertoire du fichier de licence
	// Renvoie "" si repertoire inexistant (et emet un message utilisateur en mode non silencieux)
	static const ALString FindLicenseFileDir(boolean bSilent);

	///////////////////////////////////////////////////////////////////////////////
	// Gestion du machine ID et des product keys

	// Lecture d'un fichier de licence avec alimentation dans un tableau de cles
	static boolean ReadLicenseFile(const ALString& sLicenseFilePath, int nMaxLineNumber,
				       StringVector* svLicenseFileKeys);

	// Test si une cle est valide syntaxiquement (sans message d'erreur)
	static boolean IsKeySyntaxValid(const ALString& sLicenseKey);

	// Recherche de la date liee a une cle de produit
	// Renvoie false si date non decodable (sans message d'erreur)
	// car cle non conforme a la machine ou fonctionnalite encodee
	static boolean LookupProductKeyLimitDate(const ALString& sMachineID, const ALString& sFeatureFullName,
						 const ALString& sProductKey, int& nYear, int& nMonth, int& nDay);

	// Fabrication du machine ID
	static const ALString BuildMachineIDHeader(const ALString& sComputerName);
	static const ALString BuildMachineID();

	// Verification de la conformite d'un code d'identification
	// Messages utilisateur en cas d'erreur
	static boolean CheckMachineID(const ALString& sComputerName, const ALString& sMachineID);

	////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires de base

	// Hascode d'une chaine de caractere
	static int GetHashKey(const ALString& sValue);

	// Conversion d'un entier en chaine de caractere lisible
	// de la forme XX-XX-XX-XX (avec caracteres alpha-numeriques)
	static const ALString IntToReadableString(int nValue);

	// Conversion d'une chaine de caractere lisible XX-XX-XX-XX
	// (avec caracteres alpha-numeriques) en un entier
	static int ReadableStringToInt(const ALString& sValue);

	// Permutation aleatoire des caracteres d'une chaine de caractere
	static void ShuffleString(ALString& sValue);

	// Generateur aleatoire (local a la classe)
	static int GetRandomIndex(int nMax);

	// Verification de la date courante de l'ordinateur
	// On tente de detecter si l'ordinateur n'a pas ete antidate
	static const boolean CheckCurrentDate();

	// Verification du format d'une date YYYY-MM-DD dans une plage valide (sans message d'erreur)
	static boolean CheckDateString(const ALString& sDateValue);

	// Formatage d'une date
	static const ALString GetFormattedDate(int nYear, int nMonth, int nDay);

	// Extraction des informations jour, mois, an a partir d'une date valide au format YYYY-MM-DD
	static int GetDateYear(const ALString& sDateValue);
	static int GetDateMonth(const ALString& sDateValue);
	static int GetDateDay(const ALString& sDateValue);

	// Variable de classes
	static boolean bIsInitialised;
	static StringVector svLicenseKeys;

	// Limites des dates gerees
	// Pour n'avoir que des annees bssexitiles "normales" (sans regle des siecles)
	// et pour tenir compte de la limite des time_t du C ANSI
	static const int nLimitDateMinYear = 2001;
	static const int nLimitDateMaxYear = 2037;

	// Constantes
	static const ALString sLicenseFileDirEnvVarName;
	static const ALString sLicenseFileNameEnvVarName;
	static const int nKeyHeaderLength;
	static const int nKeyLength;
	static int nRandomIndexSeed;
	static int nMaxLicenceKeyNumber;
};
