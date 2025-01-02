// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPredictorNaiveBayes.h"
#include "CMDRMajority.h"
#include "KWClassStats.h"

/////////////////////////////////////////////////////////////////////
// Classe CMMajorityClassifier
// Les fonctionnalites de cette classe permettent une classification
// par la classe majoritaire.
// Le coeur de tout classifieur est constitue par sa methode InternalTrain().
// Ici, il s'agit de determiner la modalite cible majoritaire.
class CMMajorityClassifier : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	CMMajorityClassifier();
	~CMMajorityClassifier();

	///////////////////////////////////////////////////////////////////////////
	// Parametrage obligatoire du predicteur, a redefinir dans la sous-classe

	// Type de predicteur disponible
	//   KWType::Symbol: classification
	//   KWType::Continuous: regression
	//   KWType::None: non supervise
	boolean IsTargetTypeManaged(int nType) const;

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWPredictor* KWSpecificPredictor::Create() const
	//      {
	//          return new KWSpecificPredictor;
	//      }
	KWPredictor* Create() const;

	// Nom du predicteur
	// Doit etre reimplementer dans les sous-classes
	const ALString GetName() const;

	// Prefixe du predicteur, utilisable pour le nommage de la classe en deploiement (par defaut: "P_")
	const ALString GetPrefix() const;

	///////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain();

	// Construction d'un predicteur bayesien naif a partir d'un tableau d'attributs prepares
	// La methode filtre les attributs inutiles (partition source singleton ou poids nul)
	// Cela permet de specifier facilement des variantes de predicteur Bayesien dans des sous classes
	// Parametres:
	//     dataPreparationClass: classe de preparation a completer avec les attributs du predicteur
	//     oaDataPreparationUsedAttributes: les attributs prepares a utiliser
	//	   cvDataPreparationWeights: poids des attributs (indexes par leur index dans la dataPreparationClass)

	void InternalTrainMC1(KWDataPreparationClass* dataPreparationClass,
			      ObjectArray* oaDataPreparationUsedAttributes);
	void InternalTrainMC2(KWTrainedClassifier* trainedClassifier, KWDataPreparationClass* dataPreparationClass,
			      ObjectArray* oaDataPreparationUsedAttributes);

	// Ajout de l'attribut classifieur
	KWAttribute* AddClassifierAttribute(KWTrainedClassifier* trainedRegressor,
					    ObjectArray* oaDataPreparationUsedAttributes);
};
