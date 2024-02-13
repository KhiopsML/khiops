// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSpecView.h"

KWPredictorSpecView::KWPredictorSpecView()
{
	bIsViewInitialized = false;
	bStaticPredictorView = true;
}

KWPredictorSpecView::~KWPredictorSpecView() {}

void KWPredictorSpecView::EventUpdate(Object* object)
{
	KWPredictorSpec* editedObject;

	require(bIsViewInitialized);
	require(object != NULL);

	editedObject = cast(KWPredictorSpec*, object);
	editedObject->SetPredictorName(GetStringValueAt("PredictorName"));
	editedObject->SetPredictorLabel(GetStringValueAt("PredictorLabel"));

	// On force la mise a jour du predicteur specifique associe au nom
	editedObject->GetPredictor();
}

void KWPredictorSpecView::EventRefresh(Object* object)
{
	KWPredictorSpec* editedObject;

	require(bIsViewInitialized);
	require(object != NULL);

	editedObject = cast(KWPredictorSpec*, object);
	SetStringValueAt("PredictorName", editedObject->GetPredictorName());
	SetStringValueAt("PredictorLabel", editedObject->GetPredictorLabel());
}

void KWPredictorSpecView::SetStaticPredictorView(boolean bValue)
{
	require(not bIsViewInitialized);

	bStaticPredictorView = bValue;
}

boolean KWPredictorSpecView::GetStaticPredictorView() const
{
	return bStaticPredictorView;
}

void KWPredictorSpecView::SetObject(Object* object)
{
	KWPredictorSpec* predictorSpec;
	KWPredictorView* predictorView;
	boolean bExistingPredictorView;

	require(object != NULL);

	// Acces a l'objet edite
	predictorSpec = cast(KWPredictorSpec*, object);

	// Initialisation si necessaire de l'interface
	bExistingPredictorView = false;
	if (not bIsViewInitialized)
	{
		if (bStaticPredictorView)
			bExistingPredictorView = InitializeStaticView(predictorSpec);
		else
			InitializeDynamicView(predictorSpec);
	}

	// Parametrage des sous-fiches
	cast(KWAttributeConstructionSpecView*, GetFieldAt("AttributeConstructionSpec"))
	    ->SetObject(predictorSpec->GetAttributeConstructionSpec());
	cast(KWPreprocessingSpecView*, GetFieldAt("PreprocessingSpec"))
	    ->SetObject(predictorSpec->GetPreprocessingSpec());
	if (bExistingPredictorView)
	{
		// Acces a la vue sur le predictor (qui doit etre compatible avec le predictor)
		predictorView = cast(KWPredictorView*, GetFieldAt("Predictor"));
		assert(predictorView->GetName() == predictorSpec->GetPredictorName());

		// Parametrage du predicteur a editer, s'il est valide
		if (predictorSpec->GetPredictor() != NULL)
			predictorView->SetObject(predictorSpec->GetPredictor());
	}

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWPredictorSpec* KWPredictorSpecView::GetPredictorSpec()
{
	return cast(KWPredictorSpec*, objValue);
}

boolean KWPredictorSpecView::InitializeStaticView(KWPredictorSpec* predictorSpec)
{
	KWPredictorView* predictorView;
	KWAttributeConstructionSpecView* attributeConstructionSpecView;

	require(not bIsViewInitialized);
	require(bStaticPredictorView);
	require(predictorSpec != NULL);

	// Parametrage general
	SetIdentifier("KWPredictorSpec");
	SetLabel("Predictor");
	AddStringField("PredictorName", "Name", "");
	AddStringField("PredictorLabel", "Label", "");

	// Le nom du predictor, statique, n'est pas editable
	GetFieldAt("PredictorName")->SetEditable(false);

	// Recherche d'une vue d'edition specifique du predictor
	predictorView = KWPredictorView::ClonePredictorView(predictorSpec->GetPredictorName());

	// Ajout d'une sous fiche specifique au predicteur
	if (predictorView != NULL)
		AddCardField("Predictor", "Parameters", predictorView);

	// Ajout d'une sous fiche pour la construction de variable
	attributeConstructionSpecView = new KWAttributeConstructionSpecView;
#ifdef DEPRECATED_V10
	{
		// (sans les fonctionnalite de recodage)
		attributeConstructionSpecView->GetFieldAt("RecodingClass")->SetVisible(false);
		attributeConstructionSpecView->GetActionAt("InspectRecodingSpec")->SetVisible(false);
	}
#endif // DEPRECATED_V10
	AddCardField("AttributeConstructionSpec", "Variable construction", attributeConstructionSpecView);

	// Ajout d'une sous fiche pour le preprocessing
	AddCardField("PreprocessingSpec", "Preprocessing", new KWPreprocessingSpecView);

	// Passage en ergonomie onglet
	SetStyle("TabbedPanes");

	// Flag d'initialisation
	bIsViewInitialized = true;

	return predictorView != NULL;
}

void KWPredictorSpecView::InitializeDynamicView(KWPredictorSpec* predictorSpec)
{
	ALString sPredictorNames;
	ObjectArray oaPredictors;
	int nPredictor;
	KWPredictor* predictor;

	require(not bIsViewInitialized);
	require(not bStaticPredictorView);
	require(predictorSpec != NULL);

	// Parametrage general
	SetIdentifier("KWPredictorSpec");
	SetLabel("Predictor");
	AddStringField("PredictorName", "Name", "");
	AddStringField("PredictorLabel", "Label", "");

	// Recherche des noms des predicteurs disponibles
	KWPredictor::ExportAllPredictors(predictorSpec->GetTargetAttributeType(), &oaPredictors);
	for (nPredictor = 0; nPredictor < oaPredictors.GetSize(); nPredictor++)
	{
		predictor = cast(KWPredictor*, oaPredictors.GetAt(nPredictor));
		if (nPredictor > 0)
			sPredictorNames += "\n";
		sPredictorNames += predictor->GetName();
	}

	// Passage du champ "nom du predictor" en style combo
	GetFieldAt("PredictorName")->SetStyle("ComboBox");
	GetFieldAt("PredictorName")->SetParameters(sPredictorNames);

	// Ajout des sous-fiches
	AddCardField("AttributeConstructionSpec", "Variable Construction", new KWAttributeConstructionSpecView);
	AddCardField("PreprocessingSpec", "Preprocessing", new KWPreprocessingSpecView);

	// Ajout d'une action sous forme bouton pour editer les parametres du predicteurs
	AddAction("InspectPredictor", "Parameters", (ActionMethod)&KWPredictorSpecView::InspectPredictor);
	GetActionAt("InspectPredictor")->SetStyle("Button");

	// Flag d'initialisation
	bIsViewInitialized = true;
}

void KWPredictorSpecView::InspectPredictor()
{
	KWPredictorSpec* predictorSpec;
	KWPredictorView* predictorView;
	UICard emptyCard;

	require(bIsViewInitialized);
	require(not bStaticPredictorView);
	require(GetPredictorSpec() != NULL);

	// Acces a l'objet edite
	predictorSpec = GetPredictorSpec();

	// Recherche d'une vue d'edition specifique du predictor
	predictorView = KWPredictorView::ClonePredictorView(predictorSpec->GetPredictorName());

	// Ouverture de la fiche d'edition du predicteur
	if (predictorView != NULL and predictorSpec->GetPredictor() != NULL)
	{
		predictorView->SetObject(predictorSpec->GetPredictor());
		predictorView->Open();
	}
	// Message utilisateur sinon, sous forme d'une boite vide
	else
	{
		emptyCard.SetIdentifier("NoPredictorParameters");
		emptyCard.SetLabel(predictorSpec->GetPredictorName());
		emptyCard.GetActionAt("Exit")->SetLabel("No parameters");
		emptyCard.Open();
	}

	// Nettoyage
	if (predictorView != NULL)
		delete predictorView;
}

void KWPredictorSpecView::Test()
{
	KWPredictorSpec predictorSpec;
	KWPredictorSpecView predictorSpecView;

	// Enregistrement de predicteurs
	KWPredictor::RegisterPredictor(new KWPredictorUnivariate);
	KWPredictor::RegisterPredictor(new KWPredictorNaiveBayes);
	KWPredictor::RegisterPredictor(new KWPredictorSelectiveNaiveBayes);

	// Enregistrement de vues sur les predicteurs
	KWPredictorView::RegisterPredictorView(new KWPredictorSelectiveNaiveBayesView);

	// Parametrage de la fenetre
	predictorSpec.SetPredictorName("Selective Naive Bayes");
	predictorSpec.SetTargetAttributeType(KWType::Symbol);
	predictorSpecView.SetStaticPredictorView(false);

	// Ouverture de la fiche d'edition des specification du predicteur
	predictorSpecView.SetObject(&predictorSpec);
	predictorSpecView.Open();

	// Nettoyage de l'administration des predicteurs
	KWPredictorView::DeleteAllPredictorViews();
	KWPredictor::DeleteAllPredictors();
}
