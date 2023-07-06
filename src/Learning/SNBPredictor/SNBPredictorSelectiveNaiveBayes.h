// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBPredictorSelectiveNaiveBayes;

#include "KWPredictorNaiveBayes.h"
#include "KWSelectionParameters.h"
#include "SNBPredictorSelectiveNaiveBayesTrainingTask.h"
#include "SNBAttributeSelectionScorer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Predicteur bayesien naif selectif
// Il doit etre parametre par un objet KWClassStats correctement initialise pour l'apprentissage.
// Les statistiques ne seront reevaluees que si necessaire
// L'algorithme de optimisation de la selection de variables
// est sous-traitee a une tache parallele SNBPredictorSelectiveNaiveBayesTrainingTask
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
	void CopyFrom(const KWPredictor* kwpSource) override;

	// Nom du predictor
	const ALString GetName() const override;

	// Prefixe du predictor
	const ALString GetPrefix() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametrage utilisateur des attributs a evaluer ou selectionner
	// Rappel: on peut acceder a GetTrainParameters() par une methode ancetre

	// Parametres de selection de variables
	// Memoire: appartient a l'appele
	KWSelectionParameters* GetSelectionParameters();

	// Rapport d'apprentissage, avec la description des variables selectionnees
	// Memoire: appartient a l'appele
	KWPredictorSelectionReport* GetPredictorSelectionReport();

	// Nombre d'attributs a utiliser pendant l'apprentissage
	int GetTrainingAttributeNumber();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout de meta-donnees (Weight, MAP) aux attributs du classifieur appris
	void FillPredictorAttributeMetaData(KWClass* kwcClass);

	////////////////////////////
	// Entrainement

	// Reimplementation de l'apprentissage
	boolean InternalTrain() override;

	// Finalisation de l'entrainement du MAP a partir d'une selection
	void InternalTrainMAP(KWDataPreparationClass* dataPreparationClass,
			      SNBHardAttributeSelection* mapAttributeSelection);

	// Finalisation de l'entrainement quand il n'y a pas d'attribut informatif
	void InternalTrainEmptyPredictor();

	// Finalisation de l'entrainement quand il n'y a qu'un seul attribut informatif
	void InternalTrainUnivariatePredictor();

	// Rapport d'entrainement
	// Redefinition de la methode de creation du rapport, pour integrer la description
	// des variables selectionnees (renvoie un KWPredictorSelectionReport)
	void CreatePredictorReport() override;

	////////////////////////////////
	// Objets de travail

	// Parametres de selection de variables
	KWSelectionParameters selectionParameters;

	// Acces interne aux taches d'apprentissage
	friend class SNBPredictorSNBTrainingTask;
	friend class SNBPredictorSNBEnsembleTrainingTask;
	friend class SNBPredictorSNBDirectTrainingTask;
};
