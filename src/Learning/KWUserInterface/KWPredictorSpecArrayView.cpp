// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSpecArrayView.h"

KWPredictorSpecArrayView::KWPredictorSpecArrayView()
{
	// Parametrage general
	nTargetAttributeType = KWType::Unknown;
	SetIdentifier("Array.KWPredictorSpec");
	SetLabel("Predictors");
	AddStringField("PredictorName", "Name", "");
	AddStringField("PredictorLabel", "Label", "");

	// Card and help prameters
	SetItemView(new KWPredictorSpecView);
	CopyCardHelpTexts();

	// Ergonomie CellEditable pour editer le type de predicteur,
	// avec inspection pour en specifier les details
	SetErgonomy(CellEditable);
	GetActionAt("InspectItem")->SetVisible(true);

	// Le champ de parametrage est en consultattion
	GetFieldAt("PredictorLabel")->SetEditable(false);

	// Champ a utiliser pour les selections
	SetKeyFieldId("PredictorLabel");

	// Parametrage du filtre sur les predicteurs
	SetPredictorFilter("");
}

KWPredictorSpecArrayView::~KWPredictorSpecArrayView() {}

void KWPredictorSpecArrayView::SetPredictorFilter(const ALString& sPredictorNames)
{
	ALString sUsablePredictorNames;
	ALString sDefaultPredictorName;
	int nPredictorNumber;
	ObjectArray oaPredictors;
	int nPredictor;
	KWPredictor* predictor;

	// Memorisation du filtre
	// (necessaire des le depart pour appeler la methode IsPredictorUsable)
	sPredictorFilter = sPredictorNames;

	// Recherche des noms des predicteurs disponibles, si le type de predicteur est initialise
	nPredictorNumber = 0;
	if (GetTargetAttributeType() != KWType::Unknown)
	{
		KWPredictor::ExportAllPredictors(GetTargetAttributeType(), &oaPredictors);
		for (nPredictor = 0; nPredictor < oaPredictors.GetSize(); nPredictor++)
		{
			predictor = cast(KWPredictor*, oaPredictors.GetAt(nPredictor));

			// Test si le nom du predicteur est utilisable
			if (KWPredictorSpec::IsPredictorUsable(predictor->GetName(), sPredictorFilter))
			{
				// Concatenation des noms de predictors
				if (nPredictorNumber > 0)
					sUsablePredictorNames += "\n";
				sUsablePredictorNames += predictor->GetName();

				// Memorisation du premier predicteur utilisable
				if (nPredictorNumber == 0)
					sDefaultPredictorName = predictor->GetName();

				// Comptage du nombre de predicteurs utilisables
				nPredictorNumber++;
			}
		}
	}

	// Passage du champ "nom du predictor" en style combo
	GetFieldAt("PredictorName")->SetStyle("ComboBox");
	GetFieldAt("PredictorName")->SetParameters(sUsablePredictorNames);

	// On indique la valeur par defaut
	cast(UIStringElement*, GetFieldAt("PredictorName"))->SetDefaultValue(sDefaultPredictorName);
}

const ALString& KWPredictorSpecArrayView::GetPredictorFilter()
{
	return sPredictorFilter;
}

void KWPredictorSpecArrayView::SetTargetAttributeType(int nValue)
{
	require(nValue == KWType::Symbol or nValue == KWType::Continuous or nValue == KWType::None);
	nTargetAttributeType = nValue;
}

int KWPredictorSpecArrayView::GetTargetAttributeType() const
{
	return nTargetAttributeType;
}

Object* KWPredictorSpecArrayView::EventNew()
{
	KWPredictorSpec* predictorSpec;

	require(GetTargetAttributeType() != KWType::Unknown);

	// Creation d'une spec de predictor avec la valeur par defaut
	predictorSpec = new KWPredictorSpec;
	predictorSpec->SetPredictorName(cast(UIStringElement*, GetFieldAt("PredictorName"))->GetDefaultValue());
	predictorSpec->SetTargetAttributeType(GetTargetAttributeType());
	return predictorSpec;
}

void KWPredictorSpecArrayView::EventUpdate(Object* object)
{
	KWPredictorSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPredictorSpec*, object);
	editedObject->SetPredictorName(GetStringValueAt("PredictorName"));

	// On force la mise a jour du predicteur specifique associe au nom
	editedObject->GetPredictor();
}

void KWPredictorSpecArrayView::EventRefresh(Object* object)
{
	KWPredictorSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPredictorSpec*, object);
	SetStringValueAt("PredictorName", editedObject->GetPredictorName());
	SetStringValueAt("PredictorLabel", editedObject->GetObjectLabel());
}

void KWPredictorSpecArrayView::ActionInsertItemBefore()
{
	// On desactive temporairement l'action d'edition le temps de l'insertion
	GetActionAt("InspectItem")->SetVisible(false);

	// Methode par defaut
	UIObjectArrayView::ActionInsertItemBefore();

	// On reactive l'action d'edition
	GetActionAt("InspectItem")->SetVisible(true);
}

void KWPredictorSpecArrayView::ActionInsertItemAfter()
{
	// On desactive temporairement l'action d'edition le temps de l'insertion
	GetActionAt("InspectItem")->SetVisible(false);

	// Methode par defaut
	UIObjectArrayView::ActionInsertItemAfter();

	// On reactive l'action d'edition
	GetActionAt("InspectItem")->SetVisible(true);
}

void KWPredictorSpecArrayView::ActionInspectItem()
{
	// On reparametre la fiche d'edition, afin qu'elle s'adapte
	// au contenu a editer
	SetItemView(new KWPredictorSpecView);

	// Methode par defaut
	UIObjectArrayView::ActionInspectItem();
}

void KWPredictorSpecArrayView::Test()
{
	ObjectArray oaPredictorSpecs;
	KWPredictorSpecArrayView* predictorSpecArrayView;

	// Enregistrement de predicteurs
	KWPredictor::RegisterPredictor(new KWPredictorUnivariate);
	KWPredictor::RegisterPredictor(new KWPredictorNaiveBayes);

	// Ouverture de la liste d'edition des specifications des predicteurs
	predictorSpecArrayView = new KWPredictorSpecArrayView;
	predictorSpecArrayView->SetPredictorFilter("Univariate;Naive Bayes");
	predictorSpecArrayView->SetObjectArray(&oaPredictorSpecs);
	predictorSpecArrayView->Open();

	// Nettoyage de l'administration des predicteurs
	KWPredictorView::DeleteAllPredictorViews();
	KWPredictor::DeleteAllPredictors();

	// Nettoyage des predicteurs crees
	oaPredictorSpecs.DeleteAll();
	delete predictorSpecArrayView;
}
