// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictor;

#include "KWLearningSpec.h"
#include "KWClass.h"
#include "KWClassStats.h"
#include "KWDataPreparationClass.h"
#include "KWPredictorEvaluation.h"
#include "KWTrainParameters.h"
#include "KWPredictorReport.h"
#include "KWTrainedPredictor.h"

////////////////////////////////////////////////////////////////////////////////
// Classe generique portant sur les fonctionnalites d'un predicteur,
// ancetre de toutes les classes d'apprentissage
class KWPredictor : public KWLearningService
{
public:
	// Constructeur
	KWPredictor();
	~KWPredictor();

	///////////////////////////////////////////////////////////////////////////
	// Parametrage obligatoire du predicteur, a redefinir dans la sous-classe

	// Type de predicteur disponible
	//   KWType::Symbol: classification
	//   KWType::Continuous: regression
	//   KWType::None: non supervise
	virtual boolean IsTargetTypeManaged(int nType) const = 0;

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWPredictor* KWSpecificPredictor::Create() const
	//      {
	//          return new KWSpecificPredictor;
	//      }
	virtual KWPredictor* Create() const = 0;

	// Nom du predicteur
	// Doit etre reimplementer dans les sous-classes
	virtual const ALString GetName() const = 0;

	// Prefixe du predicteur, utilisable pour le nommage de la classe en deploiement (a reimplementer)
	virtual const ALString GetPrefix() const = 0;

	// Suffixe du predicteur, utilisable pour le nommage de la classe en deploiement (par defaut: "")
	virtual const ALString GetSuffix() const;

	////////////////////////////////////////////////////////////////////////
	// Parametrage avance, facultatif

	// Parametre d'apprentissage
	// Memoire: appartient a l'appele
	KWTrainParameters* GetTrainParameters();

	// Duplication d'un predicteur
	KWPredictor* Clone() const;

	// Recopie des specifications du predicteur
	virtual void CopyFrom(const KWPredictor* kwpSource);

	// Parametrage (facultatif) par des statistiques sur le probleme d'apprentissage
	// Memoire: les specifications sont referencees et destinee a etre partagees
	// par plusieurs predicteurs. La specification d'apprentissage (KWLearningSpec)
	// du predictor doit etre la meme que celle des statistiques
	// L'objet de statistique est gere par l'appelant
	void SetClassStats(KWClassStats* stats);
	KWClassStats* GetClassStats() const;

	/////////////////////////////////////////////////////////////////////////
	// Services d'apprentissage

	// Apprentissage: nettoyage, appel de InternalTrain et verification des resultats
	void Train();

	// Teste si apprentissage en cours
	// Permet de specialiser certains require ou ensure
	boolean IsTraining() const;

	// Teste si apprentissage realise (equivaut a la disponibite d'un TrainedPredictor)
	boolean IsTrained() const;

	// Nettoyage des resultats d'apprentissage (PredictorReport et TrainedPredictor), avec ou sans destruction
	// On se retrouve dans un etat not IsTrained
	// Cette methode (destruction) est appelle automatiquement au debut de la methode Train()
	void RemoveTrainedResults();
	void DeleteTrainedResults();

	// Dictionnaire des KWDataPreparationStats selectionnes par le predicteur
	// Il s'agit d'une sous-partie du tableau GetAllPreparedStats du ClassStats en parametre (vide si ClassStats non
	// specifie), qui peuvent provenir d'attributs natifs ou construit, d'arbres, de paires de variables Memoire:
	// appartient a l'appele, et son contenu appartient au ClassStats
	const NumericKeyDictionary* GetSelectedDataPreparationStats() const;

	// Dictionnaire des KWDataPreparationStats selectionnes directement ou indirectement par le predicteur
	// Ce dictionnaire correspond a l'analyse des attribut utilses dans les SelectedDataPreparationStats,
	// comme par exemple les variables preparees utilisees via les paires ou les arbres
	// Memoire: appartient a l'appele, et son contenu appartient au ClassStats
	const NumericKeyDictionary* GetRecursivelySelectedDataPreparationStats() const;

	// Rapport d'apprentissage, oriente interpretation (alors que le TrainedPredictor est oriente deploiement)
	// Memoire: l'objet rendu appartient a l'appele
	KWPredictorReport* GetPredictorReport();

	// Acces au predicteur appris, directement ou par l'intermediaire d'une sous-classe
	// Le predicteur appris est principalement un dictionnaire, oriente evaluation et deploiement
	// Memoire: l'objet rendu appartient a l'appele
	KWTrainedPredictor* GetTrainedPredictor();

	// Methodes specialisee pour acceder au bon type de predicteur appris
	KWTrainedClassifier* GetTrainedClassifier();
	KWTrainedRegressor* GetTrainedRegressor();
	KWTrainedClusterer* GetTrainedClusterer();

	// Evaluation du predicteur sur une base de donnees
	// La methode cree un objet d'une sous-classe de KWPredictorEvaluation, auquel elle confie l'evaluation.
	// Cela permet d'une part d'acceder de facon generique aux resultats d'evaluation (qu'ils soient en
	// classification ou regression), d'autre part de specialiser si necessaire ces resultats
	// Prerequis: le predicteur doit etre appris correstement
	// Attention: l'evaluation demande une lecture de base et peut etre
	// interrompue par l'utilisateur (indique par KWPredictorEvaluation.IsStatsComputed())
	// Memoire: l'objet rendu appartient a l'appelant
	virtual KWPredictorEvaluation* Evaluate(KWDatabase* database);

	/////////////////////////////////////////////////////////////////////////
	// Administration des predicteurs

	// Enregistrement dans la base des predicteurs
	// Il ne doit pas y avoir deux predicteurs enregistres avec le meme nom
	// Memoire: les predicteurs enregistres sont geres par l'appele
	static void RegisterPredictor(KWPredictor* predictor);

	// Recherche par cle et type de prediction
	// Retourne NULL si absent
	static KWPredictor* LookupPredictor(const ALString& sName, int nTargetType);

	// Recherche par cle et duplication
	// Permet d'obtenir un predictor pret a etre instancie
	// Retourne NULL si absent
	static KWPredictor* ClonePredictor(const ALString& sName, int nTargetType);

	// Export de tous les classifieurs enregistrees pour un type donne
	// Memoire: le contenu du tableau appartient a l'appele
	static void ExportAllPredictors(int nTargetType, ObjectArray* oaPredictors);

	// Destruction de tous les predicteurs enregistrees
	static void DeleteAllPredictors();

	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Test de validite des specifications du predicteur (pour lancer l'apprentissage)
	boolean Check() const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Identifiant: par defaut: (prefix + '_') + classname + ('_' + suffix)
	virtual const ALString GetIdentifier() const;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes d'apprentissage
	// La methode Train appel les methodes redefinissables suivantes:
	//   . CreatePredictorReport
	//   . CreateTrainedPredictor
	//   . InternalTrain

	// Creation et initialisation du rapport d'apprentissage
	// Memorisation dans la variable correspondante
	// A redefinir si l'on souhaite completer le rapport par defaut
	virtual void CreatePredictorReport();

	// Creation et initialisation du predicteur selon son type
	// (KWTrainedClassifier ou KWTrainedRegressor)
	// Memorisation dans la variable correspondante
	// En principe, pas a redefinir
	virtual void CreateTrainedPredictor();

	// Methode d'apprentissage a redefinir dans les sous-classes
	// Cette methode, appelee par la methode publique Train, dispose des
	// variables de predicteur (GetTrainedPredictor...) et de rapport (GetPredictorReport)
	// dans un etat initial minimal.
	// Elle devra alimenter ces resultats d'apprentissage, notamment
	// en specifiant la classe et la attribut d'apprentissage du TrainedPredictor
	// La classe ne devra pas etre enregistree dans un domaine.
	// La methode InternalTrain est amenee a lire la database au moyen des methodes
	// globales (ReadAll) ou individuelles (Read). Elle ne doit en aucun cas modifier
	// le parametrage de selection ou marquage des instances de la base d'apprentissage.
	// La methode doit renvoyer true en cas de succes (en cas d'echec, la
	// methode appelant nettoie les donnees d'apprentissage au moyen de DeleteTrainedResults)
	virtual boolean InternalTrain();

	// Collecte des KWDataPreparationStats selectionnes recursivement ou non par le predicteur
	// pour initialiser les dictionnaires SelectedDataPreparationStats et
	// RecursivelySelectedAttributePreparationStats Le tableau en entree contient les KWDataPreparationStats
	// utilises par le predicteur
	virtual void CollectSelectedPreparationStats(ObjectArray* oaUsedDataPreparationStats);

	//////////////////////////////////////////////////////////////////////
	// Memorisation des donnees de statistiques

	// Attribut des statistiques de prepararation
	KWClassStats* classStats;

	// Parametres d'apprentissage
	KWTrainParameters trainParameters;

	// Dictionnaire des KWDataPreparationStats du ClassStats selectionnes
	NumericKeyDictionary nkdSelectedDataPreparationStats;

	// Dictionnaire des KWDataPreparationStats du selectionnes recursivement
	NumericKeyDictionary nkdRecursivelySelectedDataPreparationStats;

	// Resultats d'apprentissage, a creer et alimenter par la methode InternalTrain
	KWPredictorReport* predictorReport;
	KWTrainedPredictor* trainedPredictor;

	// Indicatuer d'apprentissage en cours
	boolean bIsTraining;

	// Administration des predictors
	static ObjectDictionary* odPredictors;
};

// Fonction de comparaison sur le nom d'un predicteur
int KWPredictorCompareName(const void* first, const void* second);