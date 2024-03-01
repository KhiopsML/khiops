// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWResultFilePathBuilder;

#include "PLRemoteFileService.h"

///////////////////////////////////////////////////////////////////////////////
// Service de fabrication d'un path de fichier
class KWResultFilePathBuilder : public Object
{
public:
	// Constructeur
	KWResultFilePathBuilder();
	~KWResultFilePathBuilder();

	//////////////////////////////////////////////////////////////////////////////
	// Specification des parametres

	// Nom du fichier en entree
	void SetInputFilePathName(const ALString& sValue);
	const ALString& GetInputFilePathName() const;

	// Nom du fichier en sortie
	void SetOutputFilePathName(const ALString& sValue);
	const ALString& GetOutputFilePathName() const;

	// Parametrage de l'extension (ex: "txt")
	// Facultif, ignore si vide
	void SetFileSuffix(const ALString& sValue);
	const ALString& GetFileSuffix() const;

	// Parametrage de la gestion de l'extension (defaut: true)
	// Si le mode est a true, on ajoute l'extension si elle n'est pas presente
	// Sinon, cela provoque une erreur
	// Ce comportement est inactif en mode API, ou on tolere n'importe quelle extension
	static void SetForceSuffix(boolean bValue);
	static boolean GetForceSuffix();

	// Mode API, controle par defaut la variable d'environnement KHIOPS_API_MODE
	static void SetLearningApiMode(boolean bValue);
	static boolean GetLearningApiMode();

	//////////////////////////////////////////////////////////////////////////////
	// Services de fabrication d'un chemin complet de fichier de resultat
	// a partir des noms de fichier specifie en entree et sortie

	// Verification du chemin de repertoire complet en sortie
	// On tente de construire les repertoires en sortie
	// On rend false si echec, avec message d'erreur exploitant le parametre de categorie d'erreur
	boolean CheckResultDirectory(const ALString& sErrorCategory) const;

	// Construction d'un chemin de fichier de resultat
	// Les parametres en entree doivent être valides
	// En mode standard:
	//   - auto-completion de l'extension, selon ForceSuffix
	//   - si chemin en sortie absolu: on le garde tel quel
	//     - sinon, on le concatene avec celui du fichier en entree s'il est present
	// En mode api
	//   - on prend le chemin tel quel
	//     - path par rapport au repertoire courant
	//     - extension acceptee telle quelle
	const ALString BuildResultFilePathName() const;

	// Construction d'un chemin de fichier complet en sortie avec une extension alternative
	// En mode standard:
	//   - remplacement de l'extension de base par l'extension alternative
	// En mode api
	//   - si extension du fichier en sortie est valide: remplacement
	//     - sinon, concatenation en fin de fichier
	const ALString BuildOtherResultFilePathName(const ALString sOtherSuffix) const;

	// Construction d'un chemin de repertoire complet en sortie
	// En mode standard:
	//   - si chemin en sortie absolu: on le garde tel quel
	//     - sinon, on le concatene avec celui du fichier en entree
	// En mode api
	//   - on prend le chemin tel quel: le path par rapport au repertoire courant
	const ALString BuildResultDirectoryPathName() const;

	//////////////////////////////////////////////////////////////////////////////
	// Services de classe pouvant etre appele directement, dont le fonctionnnement
	// depend de mode API

	// Verification du suffixe d'un fichier
	// On rend false si echec, avec message d'erreur exploitant le parametre de categorie d'erreur
	static boolean CheckFileSuffix(const ALString& sFilePathName, const ALString& sSuffix,
				       const ALString& sErrorCategory);

	// Verification d'un chemin de repertoire de resultat
	// On tente de construire les repertoires intermediaires
	// On rend false si echec, avec message d'erreur exploitant le parametre de categorie d'erreur
	static boolean CheckResultDirectory(const ALString& sPathName, const ALString& sErrorCategory);

	// Modification du suffixe d'un fichier selon le mode API
	static const ALString UpdateFileSuffix(const ALString& sFilePathName, const ALString& sSuffix);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification de la syntaxe d'un suffixe
	static boolean CheckSuffix(const ALString& sSuffix);

	// Parametres principaux
	ALString sInputFilePathName;
	ALString sOutputFilePathName;
	ALString sFileSuffix;

	// Gestion des extensions
	static boolean bForceSuffix;

	// Gestion du mode API
	static boolean bIsLearningApiModeInitialized;
	static boolean bLearningApiMode;
};
