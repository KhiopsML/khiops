// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabase.h"
#include "KWMTDatabase.h"
#include "KWClassDomain.h"
#include "KWPredictor.h"
#include "KWEvaluatedPredictorSpec.h"
#include "KWLearningErrorManager.h"

////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluator
//    Evaluation d'un ou plusieurs predicteurs reconnus
//     par analyse des dictionnaires du domaine courant,
//     a partir d'une base de donnes en parametre
class KWPredictorEvaluator : public Object
{
public:
	// Constructeur
	KWPredictorEvaluator();
	~KWPredictorEvaluator();

	////////////////////////////////////////////////////////
	// Parametrage de l'evaluateur de predicteurs

	// Evaluation report
	const ALString& GetEvaluationFileName() const;
	void SetEvaluationFileName(const ALString& sValue);

	// Indique si on export le rapport d'evaluation (defaut: true)
	// Dans ce cas, le fichier JSON aura le meme nom que le rapport d'evaluation, avec extension .khj
	boolean GetExportJSON() const;
	void SetExportJSON(boolean bValue);

	// Suffix des fichiers de rapports au format json: json, ou khj depuis les rapports au format Khiops V10
	static const ALString GetJSONReportSuffix();

	// Nom du chemin complet du rapport d'evaluation
	// Si le repertoire n'est pas precise dans le nom du fichier d'evaluation,
	// on utilise le repertoire de la base
	const ALString GetEvaluationFilePathName() const;

	// Nom du chemin complet du rapport JSON, base sur le nom du rapport d'evaluation
	const ALString GetJSONFilePathName() const;

	// Main target value
	const ALString& GetMainTargetModality() const;
	void SetMainTargetModality(const ALString& sValue);

	// Acces a la base d'evaluation
	// Memoire: la base appartient a l'appele
	KWDatabase* GetEvaluationDatabase();

	// Specification des predicteur evalues (KWEvaluatedPredictorSpec)
	// Permet d'identifier les predicteurs potentiels parmi l'ensemble des dictionnaires charges en memoire
	// Memoire: le tableau et son contenu appartiennent a l'appele
	ObjectArray* GetEvaluatedPredictorSpecs();

	///////////////////////////////////////////////////////////////////////
	// Methodes principales d'alimentation et d'exploitation des
	// specifications des predicteurs a evaluer

	// Alimentation des specifications des predicteurs a evaluer en fonction
	// des dictionnaires charges dans le domaine de dictionnaires (ceux-ci sont preserves)
	// Initialisation du dictionnaire de la base d'evaluation (dictionnaire initial des predicteurs)
	// Les specifications precedentes (le choix des predicteurs a evaluer) sont preservees
	// La bsae d'evaluation est reparametree pour tenir compte des dictionnaires initiaux
	// Message d'information sur le nombre de predicteurs reconnus (ou warning si aucun)
	void FillEvaluatedPredictorSpecs();

	// Construction d'un domaine "initial" a partir des specifications d'un predicteur
	// en se basant sur les noms des classes initiales
	KWClassDomain* BuildInitialDomainPredictor(KWTrainedPredictor* trainedPredictor);

	// Acces au domaine des classes initiales construit par la methode FillEvaluatedPredictorSpecs
	// Permet de parametrer correctement la base d'evaluation
	// Retourne NULL si vide, sinon un domaine compile mais non enregistre dans l'ensemble des domaines
	KWClassDomain* GetInitialClassesDomain();

	// Recherche de l'ensemble de predicteurs dont les specifications sont selectionnees,
	// evaluation sur la base d'evaluation, et memorisation dans le rapport d'evaluation
	// Message d'erreur si incoherence
	// Suivi de tache, interruptible par l'utilisateur
	void EvaluatePredictorSpecs();

	///////////////////////////////////////////////////////////////////////
	// Methodes d'evaluation d'un ensemble de predicteurs
	// Ces methodes sont utilisables en batch, sans passer par les
	// specifications de predicteurs de GetEvaluatedPredictorSpecs()

	// Evaluation d'un ensemble de predicteurs
	// Le libelle d'evaluation vaut typiquement Train ou Test
	// Memoire: le contenu du tableau en sortie est a liberer par l'appelant
	void EvaluatePredictors(ObjectArray* oaPredictors, KWDatabase* database, const ALString& sEvaluationLabel,
				ObjectArray* oaOutputPredictorEvaluations);

	// Ecriture d'un rapport d'evaluation
	void WriteEvaluationReport(const ALString& sEvaluationReportName, const ALString& sEvaluationLabel,
				   ObjectArray* oaPredictorEvaluations);

	// Ecriture d'un rapport JSON
	void WriteJSONReport(const ALString& sJSONReportName, const ALString& sEvaluationLabel,
			     ObjectArray* oaPredictorEvaluations);

	// Evaluation d'un predicteurs
	// Retourne NULL si probleme d'evaluation
	// Memoire: l'objet retourne est a liberer par l'appelant
	KWPredictorEvaluation* EvaluatePredictor(KWPredictor* predictor, KWDatabase* database,
						 const ALString& sEvaluationLabel);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////
	// Methode detaillees de gestion des predicteurs appris a evaluer
	// Permet une utilisation des services de la classe a des fins batch
	// Un predicteur appris (KWTrainedPredictor) est associe a un dictionnaire de prediction

	// Construction d'un tableau de predicteurs appris a evaluer a partir du sous-ensemble choisi
	// parmi leurs specifications, et dupliquant les dictionnaires necessaires depuis le domaine
	// Prerequis: les dictionnaires du domaine correspondant aux specifications doivent
	// effectivement etre des predicteurs
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	// Les dictionnaires references par les KWTrainedPredictor du tableau resultat appartiennent
	// a leur predicteur
	void BuildEvaluatedTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors);

	// Verification de la coherence de predicteurs appris: meme type de prediction, meme attribut cible
	// Emet des messages d'erreur
	boolean CheckEvaluatedTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors);

	// Evaluation d'un ensemble de predicteurs appris sur la base d'evaluation, et memorisation
	// dans le rapport d'evaluation
	// Emet des messages d'erreur
	// Memoire: le contenu du tableau en sortie est a liberer par l'appelant
	void EvaluateTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors,
				       ObjectArray* oaOutputPredictorEvaluations);

	// Renommage des classes d'un domaine
	void DomainRenameClassesWithInitialNames(KWClassDomain* kwcdDomain);

	// Test si les classes d'un domaine sont compatibles avec les noms des classes initiales (pas de message
	// d'erreur)
	boolean DomainCheckClassesInitialNames(KWClassDomain* kwcdDomain);

	// Test si les attributs natifs d'une classe source sont contenuis dans une classe cible
	boolean ClassCheckNativeAttributes(KWClass* sourceClass, KWClass* targetClass);

	// Renommage des classes d'une base pour passer aux classes initiales
	// Effacement des classes si le domaine initiale est NULL
	void RenameDatabaseClasses(KWDatabase* database, KWClassDomain* kwcdInitialDomain);

	// Attributs de la classe
	ALString sEvaluationFileName;
	boolean bExportJSON;
	ALString sMainTargetModality;
	KWDatabase* evaluationDatabase;
	ObjectArray oaEvaluatedPredictorSpecs;

	// Domaine des classes initiales, permettant de parametrer la base d'evaluation
	KWClassDomain* kwcdInitialClassesDomain;

	// Domaine courant au moment de la creation du domaine des classes initiales
	KWClassDomain* kwcdInitialCurrentDomain;
};

//////////////////////////////////////////////////////////////////////////////
// Predicteur externe reconstruit a partir d'un KWTrainedPredictor
// (initialise a partir d'un dictionnaire externe correspondant a un predicteur)
class KWPredictorExternal : public KWPredictor
{
public:
	// Constructeur
	KWPredictorExternal();
	~KWPredictorExternal();

	// Type de predicteur disponible: classification et regression
	boolean IsTargetTypeManaged(int nType) const;

	// Constructeur generique
	KWPredictor* Create() const;

	// Nom du predicteur
	const ALString GetName() const;

	// Prefixe du predicteur
	const ALString GetPrefix() const;

	// Parametrage du predicteur par un objet KWTrainedPredictor externe
	// Memoire: le KWTrainedPredictor appartient a l'appele
	// Il est integre au predicteur des l'apppel de Train(), et n'est plus disponible ensuite
	void SetExternalTrainedPredictor(KWTrainedPredictor* inputTrainedPredictor);
	KWTrainedPredictor* GetExternalTrainedPredictor();

	// Dereferencement du predicteur appris
	// Remplacement par NULL, sans provoquer sa destruction
	void UnreferenceTrainedPredictor();

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain();

	// Memorisation
	KWTrainedPredictor* externalTrainedPredictor;
};
