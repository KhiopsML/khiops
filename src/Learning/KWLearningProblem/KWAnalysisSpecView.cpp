// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAnalysisSpecView.h"

KWAnalysisSpecView::KWAnalysisSpecView()
{
	SetIdentifier("KWAnalysisSpec");
	SetLabel("Parameters");
	AddStringField("TargetAttributeName", "Target variable", "");
	AddStringField("MainTargetModality", "Main target value", "");

	// ## Custom constructor

	// Ajout de sous-fiches
	AddCardField("PredictorsSpec", "Predictors", new KWModelingSpecView);
	AddCardField("RecodersSpec", "Recoders", new KWRecoderSpecView);
	AddCardField("PreprocessingSpec", "Preprocessing", new KWPreprocessingSpecView);
	AddCardField("SystemParameters", "System parameters", new KWSystemParametersView);
	AddCardField("CrashTestParameters", "Crash test parameters", new KWCrashTestParametersView);

	// Parametrage de la visibilite de l'onglet des crash tests
	GetFieldAt("CrashTestParameters")->SetVisible(GetLearningCrashTestMode());

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: fiches obsolete, conserves de facon cachee en V10 pour compatibilite ascendante des
		// scenarios
		AddCardField("ModelingSpec", "Predictors (deprecated)", new KWModelingSpecView);
		AddCardField("AttributeConstructionSpec", "Variable construction (deprecated)",
			     new KWAttributeConstructionSpecView);
		GetFieldAt("ModelingSpec")->SetVisible(false);
		GetFieldAt("AttributeConstructionSpec")->SetVisible(false);
		DEPRECATEDModelingSpec = new KWModelingSpec;
		DEPRECATEDModelingSpecReference = new KWModelingSpec;
	}
#endif // DEPRECATED_V10

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Info-bulles
	GetFieldAt("TargetAttributeName")
	    ->SetHelpText("Name of the target variable."
			  "\n The learning task is classification if the target variable is categorical,"
			  "\n regression if it is numerical."
			  "\n If the target variable is not specified, the task is unsupervised learning.");
	GetFieldAt("MainTargetModality")
	    ->SetHelpText("Value of the target variable in case of classification,"
			  "\n for the lift curves in the evaluation reports.");

	// Short cuts
	GetFieldAt("PredictorsSpec")->SetShortCut('E');
	GetFieldAt("RecodersSpec")->SetShortCut('O');
	GetFieldAt("PreprocessingSpec")->SetShortCut('G');
	GetFieldAt("SystemParameters")->SetShortCut('S');
	GetFieldAt("CrashTestParameters")->SetShortCut('C');

	// ##
}

KWAnalysisSpecView::~KWAnalysisSpecView(){
// ## Custom destructor

#ifdef DEPRECATED_V10
    {// DEPRECATED V10
     delete DEPRECATEDModelingSpec;
delete DEPRECATEDModelingSpecReference;
}
#endif // DEPRECATED_V10

// ##
}

KWAnalysisSpec* KWAnalysisSpecView::GetKWAnalysisSpec()
{
	require(objValue != NULL);
	return cast(KWAnalysisSpec*, objValue);
}

void KWAnalysisSpecView::EventUpdate(Object* object)
{
	KWAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisSpec*, object);
	editedObject->SetTargetAttributeName(GetStringValueAt("TargetAttributeName"));
	editedObject->SetMainTargetModality(GetStringValueAt("MainTargetModality"));

	// ## Custom update

#ifdef DEPRECATED_V10
	{
		// OBSOLETE V10
		static boolean bDeprecatedWarningRecoderEmited = false;
		static boolean bDeprecatedWarningPairsEmited = false;
		static boolean bDeprecatedWarningModelingEmited = false;
		static boolean bDeprecatedWarningConstructionEmited = false;
		KWAnalysisSpec* analysisSpec;

		// Acces a l'objet edite
		analysisSpec = cast(KWAnalysisSpec*, object);

		// On parametre la demande de recodage dans le nouvel onglet a partir de celle specifiee dans l'onglet
		// obsolete
		if (DEPRECATEDAttributeConstructionSpec.GetRecodingClass())
		{
			analysisSpec->GetRecoderSpec()->SetRecoder(true);
			if (not bDeprecatedWarningRecoderEmited)
			{
				AddWarning("Pane 'Variable construction' is deprecated since Khiops V10 :"
					   "\n  use new pane 'Parameters/Recoders' instead to specify a recoder");
				bDeprecatedWarningRecoderEmited = true;
			}
		}

		// On recopie si necessaire le parametrage d'analyse des paires d'attributs
		if (DEPRECATEDAttributeConstructionSpec.GetMandatoryAttributeInPairs() != "")
		{
			analysisSpec->GetModelingSpec()
			    ->GetAttributeConstructionSpec()
			    ->GetAttributePairsSpec()
			    ->SetMandatoryAttributeInPairs(
				DEPRECATEDAttributeConstructionSpec.GetMandatoryAttributeInPairs());
			if (not bDeprecatedWarningPairsEmited)
			{
				AddWarning("Pane 'Variable construction' is deprecated since Khiops V10 :"
					   "\n  use new pane 'Parameters/Predictors/Advanced predictor parameters' "
					   "instead to specify variable pair parameters");
				bDeprecatedWarningPairsEmited = true;
			}
		}

		// On recopie les specifications de construction d'attribut si necessaire
		if (DEPRECATEDAttributeConstructionSpec.GetMaxConstructedAttributeNumber() !=
			DEPRECATEDAttributeConstructionSpecReference.GetMaxConstructedAttributeNumber() or
		    DEPRECATEDAttributeConstructionSpec.GetMaxTreeNumber() !=
			DEPRECATEDAttributeConstructionSpecReference.GetMaxTreeNumber() or
		    DEPRECATEDAttributeConstructionSpec.GetMaxAttributePairNumber() !=
			DEPRECATEDAttributeConstructionSpecReference.GetMaxAttributePairNumber())
		{
			analysisSpec->GetModelingSpec()
			    ->GetAttributeConstructionSpec()
			    ->SetMaxConstructedAttributeNumber(
				DEPRECATEDAttributeConstructionSpec.GetMaxConstructedAttributeNumber());
			analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTreeNumber(
			    DEPRECATEDAttributeConstructionSpec.GetMaxTreeNumber());
			analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxAttributePairNumber(
			    DEPRECATEDAttributeConstructionSpec.GetMaxAttributePairNumber());
			if (not bDeprecatedWarningConstructionEmited)
			{
				AddWarning("Pane 'Variable construction' is deprecated since Khiops V10 :"
					   "\n  use new pane 'Parameters/Predictors/Feature engineering' instead to "
					   "specify max variable numbers to construct");
				bDeprecatedWarningConstructionEmited = true;
			}
		}

		// On recopie les specifications de modelisation si necessaire
		if (DEPRECATEDModelingSpec->DEPRECATEDIsUpdated(DEPRECATEDModelingSpecReference))
		{
			analysisSpec->GetModelingSpec()->DEPRECATEDCopyFrom(DEPRECATEDModelingSpec);
			if (not bDeprecatedWarningModelingEmited)
			{
				AddWarning(
				    "Former pane 'Predictors' is deprecated since Khiops V10 :"
				    "\n  use new pane 'Parameters/Predictors/Advanced predictor parameters' instead");
				bDeprecatedWarningModelingEmited = true;
			}
		}
	}
#endif // DEPRECATED_V10

	// ##
}

void KWAnalysisSpecView::EventRefresh(Object* object)
{
	KWAnalysisSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisSpec*, object);
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());
	SetStringValueAt("MainTargetModality", editedObject->GetMainTargetModality());

	// ## Custom refresh

	// ##
}

const ALString KWAnalysisSpecView::GetClassLabel() const
{
	return "Parameters";
}

// ## Method implementation

const ALString KWAnalysisSpecView::GetObjectLabel() const
{
	return "";
}

void KWAnalysisSpecView::SetObject(Object* object)
{
	KWAnalysisSpec* analysisSpec;

	require(object != NULL);

	// Acces a l'objet edite
	analysisSpec = cast(KWAnalysisSpec*, object);

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		// On fait utiliser le recodingSpec de l'onglet RecoderSpec pour faire editer le meme objet par l'onglet
		// obsolete AttributeConstructionSpec ce qui permet d'assurer la compatibilite ascendante des scenarios
		analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->DEPRECATEDSetRecodingSpec(
		    analysisSpec->GetRecoderSpec()->GetRecodingSpec());
		// DEPRECATED V10: On edite un autre objet DEPRECATEDAttributeConstructionSpec (different de celui de
		// ModelingSpec) pour detecter l'utilisation de l'onglet obsolete, en permettant de la comparer les
		// valeurs avec les valeurs initiales
		DEPRECATEDAttributeConstructionSpec.DEPRECATEDSetRecodingSpec(
		    analysisSpec->GetRecoderSpec()->GetRecodingSpec());
		cast(KWAttributeConstructionSpecView*, GetFieldAt("AttributeConstructionSpec"))
		    ->SetObject(&DEPRECATEDAttributeConstructionSpec);
		DEPRECATEDAttributeConstructionSpec.CopyFrom(
		    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec());
		DEPRECATEDAttributeConstructionSpecReference.CopyFrom(
		    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec());
		// DEPRECATED V10: On edite un autre objet DEPRECATEDModelingSpec pour detecter
		// l'utilisation de l'onglet obsolete, en permettant de la comparer les valeurs avec les valeurs
		// initiales
		cast(KWModelingSpecView*, GetFieldAt("ModelingSpec"))->SetObject(DEPRECATEDModelingSpec);
		DEPRECATEDModelingSpec->CopyFrom(analysisSpec->GetModelingSpec());
		DEPRECATEDModelingSpecReference->CopyFrom(analysisSpec->GetModelingSpec());
		// On fait editer les sous-objet des nouvelle fiches par les onglets obsoletes
		DEPRECATEDAttributeConstructionSpec.DEPRECATEDSetSourceSubObjets(
		    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec());
		DEPRECATEDModelingSpec->DEPRECATEDSetSourceSubObjets(analysisSpec->GetModelingSpec());
	}
#endif // DEPRECATED_V10

	// Parametrages des sous-fiches par les sous-objets
	cast(KWModelingSpecView*, GetFieldAt("PredictorsSpec"))->SetObject(analysisSpec->GetModelingSpec());
	cast(KWRecoderSpecView*, GetFieldAt("RecodersSpec"))->SetObject(analysisSpec->GetRecoderSpec());
	cast(KWPreprocessingSpecView*, GetFieldAt("PreprocessingSpec"))
	    ->SetObject(analysisSpec->GetPreprocessingSpec());

	// Cas particulier de la vue sur les parametres systemes
	// Cette vue ne travaille sur des variables statiques de KWSystemResource et KWLearningSpec
	// On lui passe neanmoins en parametre un objet quelconque non NUL pour forcer les mises a jour
	// entre donnees et interface
	cast(KWSystemParametersView*, GetFieldAt("SystemParameters"))->SetObject(object);

	// Idem pour les parametre de crash test
	cast(KWCrashTestParametersView*, GetFieldAt("CrashTestParameters"))->SetObject(object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##