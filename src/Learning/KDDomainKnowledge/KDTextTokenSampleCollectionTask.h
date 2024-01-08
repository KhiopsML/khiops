// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDTextTokenSampleCollectionTask;

#include "KWDatabaseTask.h"
#include "KDTokenFrequency.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KDTextTokenSampleCollectionTask
// Classe technique utilisee par la classe KDTextFeatureConstruction
//
// Analyse de la base pour extraire les tokesn les plus frequent par variable de type texte
class KDTextTokenSampleCollectionTask : public KWDatabaseTask
{
public:
	// Constructeur
	KDTextTokenSampleCollectionTask();
	~KDTextTokenSampleCollectionTask();

	// Lecture de la base pour collecter un echantillon de tokens les plus frequents par variable secondaire de type
	// texte Entree: 	 . sourceDatabase: base a analyser, correctement parametree avec uniquement des
	// variables de type texte (Text ou TextList)
	//     en used dans la classe principale
	//   . ivTokenNumbers: nombre de tokens a extraire par variable de type texte, dans le meme ordre que les
	//   variables de type texte
	// Sortie:
	// 	 . oaCollectedTokenSamples: tableau de samples de token (ObjectArray de KDTokenFrequency), dans le meme
	// ordre que les variables de type texte Methode interruptible, retourne false si erreur ou interruption (avec
	// message), true sinon Memoire: le tableau en sortie appartient a l'appelant, et doit etre initialement vide.
	// Les tableaux en sortie et leur contenu en tokens appartiennent a l'appele
	boolean CollectTokenSamples(const KWDatabase* sourceDatabase, const IntVector* ivTokenNumbers,
				    ObjectArray* oaCollectedTokenSamples);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////
	// Specification, implementation et test de la tache KDTextTokenSampleCollectionTask
	//
	// Objectif:
	//  . implementer la collecte des echantillons de tokens selon les specification de la methode
	//    CollectTokenSamples, dont une implementation prototype basique est disponible (cf. ci-dessous)
	//
	// Probleme 1: implementation en parallele efficace:
	//     . implementation en tant que sous-tache de KWDatabaseTask
	//        . appel de la methode principale RunDatabaseTask(sourceDatabase) depuis CollectTokenSamples
	//        . cf. exemple minimaliste dans la tache KWDatabaseCheckTask
	//        . cf. exemple basique dans la tache KWDatabaseBasicStatsTask
	//        . cf. exemple analogue et plus complexe dans la tache KDSelectionOperandSamplingTask
	//     . dimensionnement correct de la tache
	//     . reproductibilite des resultats, en sequentiel ou parallele, quel que soit le nombre de coeurs
	//         . cf. KDSelectionOperandSamplingTask
	//     . efficacite des IO
	//         . ne faire qu'une passe sur la base initiale
	//         . eventuellement, on peut réutiliser des fichiers temporaires pour stocker et trier
	//           les paire (token, frequency)
	//         . sinon, mettre au point un échantillonnage reproductible en parallele
	//
	// Probleme 2: choix de tokenisation
	//    . contexte: analyse des textes suivant deux mode
	//        . par defaut: n-gramme de bytes
	//            . pros:
	//              . bonnes performances, surtout avec des petit corpus
	//              . robuste aux texte mal rediges (ex: sms, tweets...)
	//              . ne demande pas de passe préalable de collecte de stats sur les texte
	//              . universel: pourrait permettre d'analyser le contenu d'executables pour detecter des virus
	//              . rapide a calculer
	//            . cons
	//              . beaucoup de variable generes, dont de nombreuse inutiles
	//              . boite noire, non interpretable
	//              . faut-il une version de collecte des ngrammes les plus frequents, en alternative au projections
	//              aleatoire via des tables de hashage?
	//        . sur option: tokenisation, en utilisant les tokens les plus frequents du corpus
	//            . pros:
	//              . interpretabilite
	//              . variables generees en nombre plus restreint, liees au corpus
	//              . peu ameliorer les performances dans certains cas
	//              . permet d'incorporer des connaissance du domaine, en pretraitant le corpus correctement
	//            . cons
	//              . une passe de collecte des tokens prealable est necessaire, avec impact sur les IO
	//              . moins automatique, depend de la qualite des pretraitements
	//   . options de tokenisation a investiguer
	//        . dans l'implementation prototype, les deux options suivantes on ete testees
	//            . decoupage en mot separes par des espaces (cf. PROTOAnalyseTextValueBasic)
	//            . decoupage en mots separes par des espaces, en isolant des sequences de ponctuation (cf.
	//            PROTOAnalyseTextValueUsingPunctuation) . l'option avec ponctuation obtient de bien meilleurs
	//            resultats
	//        . faut-il incorporer des pretraitements plus evolues?
	//            . options basiques "universelles"
	//              . suppression des caracteres accentues?
	//              . passage en minuscules?
	//              . traitement a part des chiffres?
	//              ...
	//            . options avancees dependant de la langue
	//              . stemmatisation?
	//              . lemmatisation?
	//              . correction orthographique?
	//              ...
	//         . parametrage utilisateur
	//              . minimaliste: une case a cocher ngrams/tokens?
	//              . idem, mais avec quelques options basiques incorporees?
	//              . choix a trois options: ngrammes/tokenisation elementaire/tokenisation basique?
	//              . boite de dialogue avec choix de pretraitements inspirés de Patatext, dont les plus standard
	//              sont coches par defaut?
	//   . autre piste a explorer
	//      . developper une bibliotheque de regles de derivation dediees au pretraitement des textes
	//   . petit probleme a resoudre
	//       . construction des noms de variables compatible avec utf8
	//         . que faire si un token extrait n'est pas utf8?
	//
	// Plan de travail
	//   . etape 1: specifier et valider les choix de conception et d'implementation, pour les problemes 1 et 2
	//   (independamment) . etape 2: implementation, tests dans un environnement independant
	//       . utilisation des bibliotheques Norm, Parallel et Learning
	//       . activation des variables de type Text et TextList par la variable d'environnement
	//       KhiopsTextVariableMode=true . implementation dans sous-classe de  KDTextTokenSampleCollectionTask .
	//       test de la collecte des token independante de Learning (parametrage par un dictionnaire et une base de
	//       donnees . utilisation des base de texte disponibles dans LearningTest\TextDatasets
	//   . etape 3: integration dans Learning
	//       . test d'integration
	//       . refactoring si necessaire
	//       . necessite d'avoir les potentiels prétraitement de texte disponible a la fois depuis la classe
	//       KDTextTokenSampleCollectionTask
	//         pour extraire les tokens et dans des regles de derivation pour le deploiement (cf. classe
	//         KWDRTextTokens)

	//////////////////////////////////////////////////////////////////////////////////////////
	// Implementation prototype permettant de tester les entree-sortie de la tache et son
	// integration dans le frawework de construction de variables de type texte
	//
	// Implementation basique non paralleise, sans dimensionnement, et avec probleme de scalabilite
	// A terme, cette implementation sera supprimee

	// Prototype "virtuel" de la methode principale
	// Construit un resultat conforme au specification avec des token "synthetiques", sans aucune analyse de la base
	boolean PROTOVirtualCollectTokenSamples(const KWDatabase* sourceDatabase);

	// Prototype de la methode principale, avec analyse de la base
	// Implementation en sequentiel, sans dimensionnement et sans passer par les methode de la tache
	boolean PROTOCollectTokenSamples(const KWDatabase* sourceDatabase);

	// Prototype d'analyse d'un objet
	boolean PROTOAanalyseDatabaseObject(const KWObject* kwoObject, ObjectArray* oaTokenDictionaries);

	// Prototype d'analyse d'une variable de type texte
	// redirige sur la variance choisie
	void PROTOAnalyseTextValue(const Symbol& sTextValue, ObjectDictionary* odTokenDictionary);

	// Prototype d'analyse d'une variable de type texte
	// Le texte est tokenise en sequences de caracteres de type espace (ignorees),
	//  ou de tout autre caracteres (gardees)
	void PROTOAnalyseTextValueBasic(const Symbol& sTextValue, ObjectDictionary* odTokenDictionary);

	// Prototype d'analyse d'une variable de type texte
	// Le texte est tokenise en sequences de caracteres de type espace (ignorees),
	//  de caracteres ponctuation uniquement (gardees),
	//  ou de tout autre caracteres (gardees)
	void PROTOAnalyseTextValueUsingPunctuation(const Symbol& sTextValue, ObjectDictionary* odTokenDictionary);

	// Prototype d'analyse d'une variable de type TextList
	void PROTOAanalyseTextListValue(const SymbolVector* svTextListValue, ObjectDictionary* odTokenDictionary);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Implementation des methodes de KWDatabaseTask pour paralleliser la collecter des tokens

	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitializePrepareDatabase() override;
	boolean SlaveInitializeOpenDatabase() override;
	boolean SlaveProcessStartDatabase() override;
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	////////////////////////////////////////////////////
	// Variables du maitre

	// Nombre de tokens a extraire par variable de typen texte, copie du parametre principal en entree
	const IntVector* ivMasterTokenNumbers;

	// Tableuax de samples de topkesn extraits par variable de type txete, copy du parametre principale en sortie
	ObjectArray* oaMasterCollectedTokenSamples;
};
