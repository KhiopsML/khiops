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
// Predicteur Bayesien naif selectif
//  - Il doit etre parametre par un objet KWClassStats correctement initialise pour l'apprentissage
//  - Les statistiques ne seront reevaluees que si necessaire
//  - L'algorithme de optimisation de la selection de variables est sous-traitee a une tache
//  parallele SNBPredictorSelectiveNaiveBayesTrainingTask
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
