// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDTextTokenSampleCollectionTask;

#include "KDTextFeatureSpec.h"
#include "KWDatabaseTask.h"
#include "KWTokenFrequency.h"
#include "KWTextService.h"
#include "KWTextTokenizer.h"

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
	// 	 . oaCollectedTokenSamples: tableau de samples de token (ObjectArray de KWTokenFrequency), dans le meme
	// ordre que les variables de type texte Methode interruptible, retourne false si erreur ou interruption (avec
	// message), true sinon Memoire: le tableau en sortie appartient a l'appelant, et doit etre initialement vide.
	// Les tableaux en sortie et leur contenu en tokens appartiennent a l'appele
	boolean CollectTokenSamples(const KWDatabase* sourceDatabase, const IntVector* ivTokenNumbers,
				    ObjectArray* oaCollectedTokenSamples);

	// Parametrage du type d'attributs de type texte: ngrams (par defaut)
	void SetTextFeatures(const ALString& sValue);
	const ALString& GetTextFeatures() const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////
	// Implementation prototype permettant de tester les entree-sortie de la tache et son
	// integration dans le frawework de construction de variables de type texte
	//
	// Implementation basique non parallelise, sans dimensionnement, et avec probleme de scalabilite
	// A terme, cette implementation sera supprimee

	// Prototype basique de la methode principale
	// Construit un resultat conforme au specification avec des tokens "synthetiques", sans aucune analyse de la
	// base Permet de tester l'integration basique de la classe
	boolean DummyCollectTokenSamples(const KWDatabase* sourceDatabase);

	// Implementation en sequentiel, sans dimensionnement et sans passer par les methode de la tache
	boolean SequentialCollectTokenSamples(const KWDatabase* sourceDatabase);

	// Analyse de la base de donnees pour en extraire les tokens a l'aide de tokenizer par attribut de type texte
	// La base en parametre est ouverte et ses objets analyses, puis la base est fermee
	boolean AnalyseDatabase(KWDatabase* database, ObjectArray* oaTextTokenizers);

	// Analyse d'un objet pour en extraire les tokens a l'aide de tokenizer par attribut de type texte
	boolean AnalyseDatabaseObject(const KWObject* kwoObject, ObjectArray* oaTextTokenizers);

	// Calcul du nombre de token a collecter en mode flux pour assurer la stabilite des resultats
	// En mode flux, on fait une premier passe avec ce nombre de tokens pour avoir les tokens les
	// plus frequents, mais avec une incertitude sur les comptes
	// On effectue alors une second passe avec les tokens ainsi identifie, et on ne garde que
	// les plus frequents selon le nombre demande. Meme s'il n'y a pas de garantie theorique,
	// cela assure en pratique la stabilite des resultats dans la plupart des cas
	int GetMaxStreamCollectedTokenNumber(int nRequestedTokenNumber) const;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Implementation de la collecte des tokens en parallele
	// Deux passes sont necessaire
	//   - la premiere passe collecte en flux un echantillons de tokens parmi les plus frequents
	//     pour gerer l'incertitude kiee aunstream, on collecte plus de tokens que demande
	//   - la deusieme passe prend ces tokens en entree et calcule de facon exacte leur effectif
	//     ce qui permet de renvoyer la sous partie des tokens les plus freqntes, tries
	//     par effectif decroissant
	// Chaque passe est executee en parallele, la premiere puis la seconde
	// Les deux passe etant basees essentiellement sur la tokenisation des textes sont tres proche.
	// On choisit d'implementer ces deux taches dans la meme classe pour factoriser le code, en jouant
	// sur des variante dun parametregae en entree et en sortie.

	// Pilotage de l'ensemble des deux passes
	boolean InternalCollectTokenSamples(const KWDatabase* sourceDatabase);

	// Verification de la coherence des parametres de pilotage des deuxn passes
	boolean CheckPassParameters() const;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des indicateur de performances
	// Cela permet d'afficher la performance en memoire et temps de calcul le temps de la mise
	// au point des algorithme

	// Debut de la collecte des indicateurs de performance
	// On indique si on souhaite les afficher effectivement par la syuite
	void StartCollectPerformanceIndicators(boolean bDisplay);

	// Fin de la collecte des indicateurs
	void StopCollectPerformanceIndicators();

	// Affichage des indicateurs collectes, en associant un livelle ainsi qu'un tableau de tokenizer
	// pour afficher le nombre de tokens collectes
	void DisplayPerformanceIndicators(const ALString& sLabel, const ObjectArray* oaTokenizers);

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
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Type de token
	PLShared_String shared_sTextFeatures;

	// Indicateur pour savoir si n est dans la premiere passe
	PLShared_Boolean shared_bIsFirstPass;

	// Activation du calcul des effectif exact des tokens, c'est a dire qu'une deuxieme passe est necessaire
	PLShared_Boolean shared_bComputeExactTokenFrequencies;

	// Vecteur des nombres de tokens par texte a collecter, pour la premiere passe
	PLShared_IntVector shared_ivFirstPassTokenNumbers;

	// Tableau des tokens specifique dont il faut calcul l'effectif, pour la deuxieme passe
	PLShared_ObjectArray* shared_oaSecondPassSpecificTokens;

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves

	// Tableau des tokens en sortie d'un esclaves
	// Dans la premiere passe, il s'agit d'un echantillon de tokens collecte par l'esclave
	// Dans le seconde passe, il s'agit des token dont il faut calculer l'effectif, avec l'effectif
	// calcule localement par l'esclave
	PLShared_ObjectArray* output_oaTokens;

	////////////////////////////////////////////////////
	// Variables du maitre

	// Nombre de tokens a extraire par variable de typen texte, copie du parametre principal en entree
	const IntVector* ivMasterTokenNumbers;

	// Tableau de samples de topkesn extraits par variable de type txete, copie du parametre principale en sortie
	// En fin de premiere passe, contient l'echantillon de tokens collectes
	// En fin de deuxieme passe, contient les tokens collectes avec leur effectif exact
	ObjectArray* oaMasterCollectedTokenSamples;

	// Tableau des tokenisers de travail, un par texte a analyser
	ObjectArray oaMasterTextTokenizers;

	// Indicateur de performance
	Timer performanceTimer;
	longint lPerformanceInitialHeapMemory;

	////////////////////////////////////////////////////
	// Variables de l'esclave

	// Tableau des tokenisers de travail, un par texte a analyser
	ObjectArray oaSlaveTextTokenizers;
};