// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWPredictor.h"
#include "KWTrainParametersView.h"

////////////////////////////////////////////////////////////////////////////////
// Classe generique portant sur la vue d'un classifieur
class KWPredictorView : public UIObjectView
{
public:
	// Constructeur
	// (doit donner un nom au predictor)
	KWPredictorView();
	~KWPredictorView();

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWPredictorView* KWSpecificPredictorView::Create() const
	//      {
	//          return new KWSpecificPredictorView;
	//      }
	virtual KWPredictorView* Create() const;

	// Nom du predicteur edite
	const ALString& GetName() const;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes
	// (en castant l'objet en parametre)

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par le predicteur specifique
	void EventRefresh(Object* object) override;

	// Acces au predicteur
	void SetObject(Object* object) override;
	KWPredictor* GetPredictor();

	// Libelles
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////////
	// Administration des vues de predictors

	// Enregistrement dans la base des predictorViews
	// Il ne doit pas y avoir deux view enregistrees avec le meme nom
	// Memoire: les vues enregistrees sont gerees par l'appele
	static void RegisterPredictorView(KWPredictorView* predictorView);

	// Recherche par cle
	// Retourne NULL si absent
	static KWPredictorView* LookupPredictorView(const ALString& sName);

	// Recherche par cle et duplication
	// Permet d'obtenir un predictorView pret a etre instancie
	// Retourne NULL si absent
	static KWPredictorView* ClonePredictorView(const ALString& sName);

	// Export de toutes les vues de classifieurs enregistrees
	// Memoire: le contenu du tableau en retour appartient a l'appele
	static void ExportAllPredictorViews(ObjectArray* oaPredictorViews);

	// Destruction de toutes les vues de classifieurs enregistrees
	static void DeleteAllPredictorViews();

	////////////////////////////////////////////////////////
	///// Implemenattion
protected:
	// Nom du classifieur
	ALString sName;

	// Administration des predictorViews
	static ObjectDictionary* odPredictorViews;
};

// Fonction de comparaison sur le nom d'une vue de classifieur
int KWPredictorViewCompareName(const void* first, const void* second);