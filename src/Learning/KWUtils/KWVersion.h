// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"

//////////////////////////////////////////////////////////////////////////////
// Gestion des noms d'application, de module et de la version de l'executable

// Nom de l'application (defaut: Khiops)
const ALString GetLearningApplicationName();
void SetLearningApplicationName(const ALString& sValue);

// Nom du module (defaut: vide)
// Exemples:
//   Data preparation and scoring
//   Deployment
//   Vizualization
//   Interpretation
//   Coclustering
//   Covizualization
const ALString GetLearningModuleName();
void SetLearningModuleName(const ALString& sValue);

// Nom de la commande lancee : Application_module (en minuscule sur linux)
const ALString GetLearningCommandName();

// Nom complet de l'application, avec son eventuel module
const ALString GetLearningFullApplicationName();

// Numero de version de la bibliotheque et des applications
// Possibilite de le changer si necessaire
const ALString GetLearningVersion();
void SetLearningVersion(const ALString& sValue);

// Copyright orange
const ALString GetLearningCopyrightLabel();

// Image pour le menu about (defaut: "")
const ALString GetLearningAboutImage();
void SetLearningAboutImage(const ALString& sValue);

// Site web (defaut: "")
const ALString GetLearningWebSite();
void SetLearningWebSite(const ALString& sValue);

// Type de systeme: 32 ou 64 bit
const ALString GetLearningSystemType();

// Dans le cas d'un coclustering individus * variables : prise en compte d'un groupe poubelle fourre-tout pour l'attribut VarPart
const boolean GetVarPartAttributeGarbage();
void SetVarPartAttributeGarbage(const boolean bValue);

// Dans le cas d'un coclustering individus * variables : prise en compte d'un groupe poubelle fourre-tout pour les innerAttributes
const boolean GetInnerAttributeGarbage();
void SetInnerAttributeGarbage(const boolean bValue);

//////////////////////////////////////////////////////////////////////////////////////
// Politique d'affichage des noms

// Nom complet de l'application
//   <application name> <module name> <version>
const ALString GetLearningFullName();

// Titre de la fenetre principale de l'application (le copyright est ajoute automatiquement a toutes les fenetres)
//   <application name> <module name>
const ALString GetLearningMainWindowTitle();

// Banniere a afficher dans le shell au lancement de l'application depuis le main
//   <application name> <module name> <version> <edition> \n  <copyright label>
const ALString GetLearningShellBanner();

// Ligne d'entete des fichiers de rapport genere, a inserer en premiere ligne de tout rapport genere, sauf si vide
//   #<application name> <version>
const ALString GetLearningReportHeaderLine();

// Modification de la ligne d'entete si la valeur par defaut ne convient pas
// Possibilite de supression de ligne d'ente si vide
void SetLearningReportHeaderLine(const ALString& sNewReportHeaderLine);

///////////////////////////////////////////////////////////////////////////////////////
// Modes avances pour le controle de mise a disposition de certaines fonctionnalites

// Parametrage par du type d'interface par defaut (defaut: false)
// Le mode basique est utile pur des environnements de type cloud, pour permettre de specifier un path dans
// le systeme de gestion de fichier du cloud. Ainsi, le champ de la boite de dialogue peut etre saisi soit
// directement avec un URI, soit via un bouton FileChooser
void SetLearningDefaultRawGuiModeMode(boolean bValue);
boolean GetLearningDefaultRawGuiModeMode();

// Indicateur du mode d'interface basique des fiches de saisie d'un nom de fichier
// Par defaut, on prend le comportement indique par GetLearningDefaultRawGuiModeMode()
// Ce mode est controlable par la variable d'environnement KhiopsRawGuidMode a true ou false
// pour forcer un comportement different
boolean GetLearningRawGuiModeMode();

// Indicateur du mode expert de l'outil (permet d'activer certains services additionnels)
// Ce mode expert est controlable par la variable d'environnement KhiopsExpertMode a true ou false
boolean GetLearningExpertMode();

// Memoire par defaut a utiliser
// Est equivalent a un choix par utilisateur de la memoire maximum a utiliser en utilisant le parametrage
// de l'onglet des parametres systemes
// Ce mode expert est controlable par la variable d'environnement KhiopsDefaultMemoryLimit
// Renvoie 0 si non specifie ou invalide
int GetLearningDefaultMemoryLimit();

// Indicateur du mode de l'outil avec gestion d'un controle strict des limites memoire (defaut: false)
// Quand il est a true, tout depassement d'une limite memoire specifie par l'utilisateur provoque
// un crash memoire, gere par un parametrage de l'allocateur
// Ce mode expert est controlable par la variable d'environnement KhiopsHardMemoryLimitMode a true ou false
boolean GetLearningHardMemoryLimitMode();

// Indicateur du mode de l'outil avec gestion des parametres de crash test
// Ce mode expert est controlable par la variable d'environnement KhiopsCrashTestMode a true ou false
boolean GetLearningCrashTestMode();

// Indicateur du mode de l'outil avec gestion du mode fast exist pour l'execution d'un scenario
// Cf. UIObject::SetFastExitMode
// Ce mode expert est controlable par la variable d'environnement KhiopsFastExitMode a true ou false
boolean GetLearningFastExitMode();

// Indicateur du mode trace pour le dimensionnement des taches de preparation
// Ce mode expert est controlable par la variable d'environnement KhiopsPreparationTraceMode a true ou false
boolean GetPreparationTraceMode();

// Indicateur du mode trace des acces IO
// Ce mode expert est controlable par la variable d'environnement KhiopsIOTraceMode a true ou false
boolean GetIOTraceMode();

// Indicateur du mode expert pour la creation des forets d'arbres
// Ce mode expert est controlable par la variable d'environnement KhiopsForestExpertMode a true ou false
boolean GetForestExpertMode();

// Indicateur du mode expert de l'outil de coclustering (permet d'activer certains services additionnels)
// Ce mode expert est controlable par la variable d'environnement KhiopsCoclusteringExpertMode a true ou false
boolean GetLearningCoclusteringExpertMode();

// Indicateur du mode expert de l'outil de coclustering (permet d'activer la fonctionnalite de coclustering instances *
// variables) Ce mode expert est controlable par la variable d'environnement KhiopsCoclusteringIVExpertMode a true ou
// false
boolean GetLearningCoclusteringIVExpertMode();

// Indicateur du mode parallelisation expert de l'outil (en mode expert uniquement)
// Permet d'activer certains services additionnels du mode parallele
// Ce mode est controlable par la variable d'environnement KhiopsExpertParallelMode a true ou false
boolean GetParallelExpertMode();

// Indicateur du mode de trace dans les tache paralleles
// Renvoie 0 (pas de traces), 1 (traces sur les ressources), 2 (traces ressources detaillees), 3 (2+ traces des appels
// aux methodes de PLParallelTask)
int GetParallelTraceMode();

// Indicateur du lancement d'un serveur de fichier sur un systeme mono-machine. Les serveurs sont normalement instancies sur
// un cluster de machine. Cet indicateur permet de tester le driver de fichier distant sans cluster.
boolean GetFileServerActivated();

// Indicateur du mode d'etude des prior dans le cadre de la construction de variable (en mode expert uniquement)
// Ce mode est controlable par la variable d'environnement KhiopsPriorStudyMode a true ou false
boolean GetLearningPriorStudyMode();

// Indicateur du mode d'etude des distances (en mode expert uniquement)
// Permet d'activer certains services de recodage additionnels
// Ce mode est controlable par la variable d'environnement KhiopsDistanceStudyMode a true ou false
boolean GetDistanceStudyMode();

// Indicateur du mode ou le SNB force l'utilisation des variables denses pour les block sparse
boolean GetSNBForceDenseMode();
