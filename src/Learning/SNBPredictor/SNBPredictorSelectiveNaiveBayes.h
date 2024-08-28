// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBPredictorSelectiveNaiveBayes;

#include "KWDataPreparationClass.h"
#include "KWPredictorNaiveBayes.h"
#include "KWSelectionParameters.h"
#include "SNBPredictorSelectiveNaiveBayesTrainingTask.h"
#include "SNBAttributeSelectionScorer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Predicteur Bayesien naif selectif (SNB)
//
// Cette classe implemente l'optimisation de la selection de variables d'un predicteur Bayesien
// naif. Pour pouvoir executer cet algorithme en parallele l'implemenetation de cette optimisation
// est deleguee a la classe SNBPredictorSelectiveNaiveBayesTraniningTask.
//
// L'entree principale pour l'optimisation est l'ensemble de variables discretisees ou groupees par
// rapport a la cible. Ces entrees sont disponibles via une instance de la classe KWClassStats.
//
// Les principales classes utilises pendant l'optimisation sont:
//
// - SNBDataTableBinarySliceSet: Donne acces a la base de donnees recodifie sous forme de ints
// (index d'intervalle ou de groupe pour chaque attribut). La base est coupee en blocs definis par
// des chunks (ensemble contigu d'instances) et des slices (ensemble contigu d'attributs). Chaque
// chunk est destine a un processus esclave; les slices ne sont utilises que pour faire du
// out-of-core processing quand il n'y a pas assez de memoire pour le chunk entier. Dans ce dernier
// cas, le chunk est stocke dans un fichier contenant les donnees en format binaire et colonnaire.
// Les attributs en entree peuvent etre denses ou sparses; dans ce dernier cas la classe ne stocke
// que les valeurs presentes. Cette classe pourvoit d'autres informations et services necessaires
// pour l'optimisation, en particulier elle permet de parcourir les attributs en ordre aleatoire.
//
// - SNBPredictorSelectionScorer: Permet de construire une selection de variables. A tout moment
// elle permet de consulter la selection courante et son cout.
//
// - SNBPredictorSelectionDataCostCalculator: Utilise par SNBPredictorSelectionScorer, elle calcule
// la partie du cout par rappport aux donnees (likelihood). Elle est sous-specialise pour chaque
// type de predicteur (classifieur, regresseur, et classifieur generalise). Elle utilise le
// SNBDataTableBinarySliceSet pour acceder aux donnees.
//
// Plus de detail sur les classes composantes des classes ci-dessus peuvent se trouver dans ses
// fichiers d'entete respectifs.
//
// La procedure de la tache d'apprentissage peut se resumer selon les etapes suivantes:
//
// - Premiere initialisation du SNBDataTableBinarySliceSet par le maitre ce qui permet d'obtenir les
// informations necessaires pour l'estimation de ressources de la tache.
//
// - Recodage de la base de donnees preparee (en ASCII, cf. KWDataTableSliceSet) a une base de
// donnees contenant les index des parties en binaire (cf. SNBDataTableBinarySliceSet). Chaque
// esclave recodifie un seul chunk de la base de donnees et initialise son propre
// SNBDataTableBinarySliceSet pour le lire.
//
// - Premiere passe par la base de donnees pour estimer l'epsilon de comparaison.
//
// - Boucle principale: Pour chaque w dans {1, 0.5, 0.25, ..., 2^(-ceil(log(N+1)))}
//   - Passe d'increment de w le poids de tous les attributs
//   - Passe de decrement de w le poids des attributs dans la selection
//   - S'il y a eu une amelioration repeter les deux passes une fois de plus
//   Les attributs sont parcourus de facon aleatoire a chaque passe. Un increment/decrement du
//   poids d'un attribut n'est garde que s'il ameliore le cout de la selection (en tenant en compte
//   l'epsilon de comparaison).
//
// - Construction du dictionnaire du modele et des rapports avec les poids appris.
//
class SNBPredictorSelectiveNaiveBayes : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	SNBPredictorSelectiveNaiveBayes();
	~SNBPredictorSelectiveNaiveBayes();

	// Type de predicteur disponible: classification et regression
	boolean IsTargetTypeManaged(int nType) const override;

	// Constructeur generique
	KWPredictor* Create() const override;

	// Recopie des specifications du predicteurs
	void CopyFrom(const KWPredictor* sourcePredictor) override;

	// Nom du predictor
	const ALString GetName() const override;

	// Prefixe du predictor
	const ALString GetPrefix() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Acces aux parametrages utilisateur des attributs a evaluer ou selectionner
	// Rappel: on peut acceder a GetTrainParameters() par une methode ancetre

	// Parametres de selection de variables
	// Memoire: appartient a l'appele
	KWSelectionParameters* GetSelectionParameters();

	// Rapport d'apprentissage, avec la description des variables selectionnees
	// Memoire: appartient a l'appele
	KWPredictorSelectionReport* GetPredictorSelectionReport();

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout de meta-donnees (Weight & Importance) aux attributs du classifieur appris
	void FillPredictorAttributeMetaData();

	////////////////////////////
	// Entrainement

	// Reimplementation des methodes virtuelles prives de KWPredictiorNaiveBayes
	boolean InternalTrain() override;
	void CreatePredictorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
					      ObjectArray* oaUsedDataPreparationAttributes,
					      ContinuousVector* cvAttributeWeights) override;
	void CreatePredictorReport() override;

	// Finalisation de l'entrainement quand il n'y a pas d'attribut informatif
	void InternalTrainFinalizeWithEmptyPredictor();

	// Finalisation de l'entrainement quand il n'y a qu'un seul attribut informatif
	void InternalTrainFinalizeWithUnivariatePredictor();

	//////////////////////////////////////////////////////////////////////////////
	// Methodes pour le dimensionnement de la tache d'entrainement

	// Nombre d'attributs a utiliser pendant l'apprentissage
	int ComputeTrainingAttributeNumber() const;

	// Nombre d'attributs sparse a utiliser pendant l'apprentissage
	int ComputeTrainingSparseAttributeNumber();

	// Ratio de la taille moyen par individu maximale (local) et la taille moyen par individu
	double ComputeSparseMemoryFactor();

	// Nombre de valeurs missing sparse des donnees en entree
	longint ComputeTrainingAttributesSparseMissingValueNumber() const;

	// Nombre de valeurs missing sparse par attribut
	// Retourne un vecteur de taille [nombre d'attributs utilises pour l'apprentissage]
	// Le compte est zero pour les attributs denses (ie. normaux, paires et arbres)
	// Memoire : Appelant
	IntVector* ComputeTrainingSparseMissingValueNumberPerAttribute() const;

	////////////////////////////////
	// Objets de travail

	// Parametres de selection de variables
	KWSelectionParameters selectionParameters;

	// Acces interne a la tache d'apprentissage
	friend class SNBPredictorSelectiveNaiveBayesTrainingTask;
};
