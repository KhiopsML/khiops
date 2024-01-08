// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWPredictorSpec.h"
#include "KWPredictorView.h"
#include "KWPreprocessingSpecView.h"
#include "KWAttributeConstructionSpecView.h"
#include "KWPredictorUnivariate.h"
#include "KWPredictorNaiveBayes.h"
#include "KWPredictorSelectiveNaiveBayes.h"
#include "KWPredictorSelectiveNaiveBayesView.h"

////////////////////////////////////////////////////////////////////////////////
// Classe generique portant sur la vue des specifications
// d'un predicteur avec ses parametres de preprocessing
// Cette vue permet d'editer les caracteristiques detailles d'un predicteur,
// mais ni son nom ni son type, qui doivent avoir ete alimentes prealablement
class KWPredictorSpecView : public UIObjectView
{
public:
	// Constructeur
	KWPredictorSpecView();
	~KWPredictorSpecView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Parametrage du type d'interface (par defaut: statique)
	//    Static: le type de predictor provient d'objet edite, et
	//      ne peut plus etre change par la suite (il est maquette
	//      directement dans une sous-fenetre
	//    Dynamique: le type de predicteur est modifiable par l'utilisateur,
	//      ses specifications sont editables au moyen d'une action ouvrant
	//      une nouvelle fenetre
	// Le parametrage des predicteurs disponibles et de leurs vues associees
	// est administre dans les classes KWPredictor et KWPredictorView
	void SetStaticPredictorView(boolean bValue);
	boolean GetStaticPredictorView() const;

	// Parametrage des specifications
	// Lors du premier parametrage, l'interface est choisi en fonction
	// du type Fixe ou Dynamique de l'interface
	void SetObject(Object* object) override;
	KWPredictorSpec* GetPredictorSpec();

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction du type d'interface statique
	// On renvoie true si une sous-fiche de vue de predictor a ete cree
	boolean InitializeStaticView(KWPredictorSpec* predictorSpec);

	// Construction du type d'interface dynamique
	void InitializeDynamicView(KWPredictorSpec* predictorSpec);

	// Action d'ouverture d'une sous-fiche d'edition d'un predictor
	void InspectPredictor();

	// Type d'interface
	boolean bIsViewInitialized;
	boolean bStaticPredictorView;
};
