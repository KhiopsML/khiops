// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeConstructionSpecView.h"

KWAttributeConstructionSpecView::KWAttributeConstructionSpecView()
{
	SetIdentifier("KWAttributeConstructionSpec");
	SetLabel("Feature engineering parameters");
	AddIntField("MaxConstructedAttributeNumber", "Max number of constructed variables", 0);
	AddIntField("MaxTextFeatureNumber", "Max number of text features", 0);
	AddIntField("MaxTreeNumber", "Max number of trees", 0);
	AddIntField("MaxAttributePairNumber", "Max number of variable pairs", 0);
	AddStringField("MandatoryAttributeInPairs", "Only pairs with variable (deprecated)", "");
	AddBooleanField("RecodingClass", "Build recoding dictionary (deprecated)", false);

	// Parametrage des styles;
	GetFieldAt("MaxConstructedAttributeNumber")->SetStyle("Spinner");
	GetFieldAt("MaxTextFeatureNumber")->SetStyle("Spinner");
	GetFieldAt("MaxTreeNumber")->SetStyle("Spinner");
	GetFieldAt("MaxAttributePairNumber")->SetStyle("Spinner");
	GetFieldAt("RecodingClass")->SetStyle("CheckBox");

	// ## Custom constructor

	// Parametrage des nombre min et max
	cast(UIIntElement*, GetFieldAt("MaxConstructedAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxConstructedAttributeNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxConstructedAttributeNumber);
	cast(UIIntElement*, GetFieldAt("MaxTextFeatureNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxTextFeatureNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxTextFeatureNumber);
	cast(UIIntElement*, GetFieldAt("MaxTreeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxTreeNumber"))
	    ->SetMaxValue(KWAttributeConstructionSpec::nLargestMaxTreeNumber);
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxAttributePairNumber"))
	    ->SetMaxValue(KWAttributePairsSpec::nLargestMaxAttributePairNumber);

#ifdef DEPRECATED_V10
	{
		// Declaration des actions DEPRECATED
		AddAction("InspectConstructionDomain", "Variable construction parameters",
			  (ActionMethod)(&KWAttributeConstructionSpecView::DEPRECATEDInspectConstructionDomain));
		GetActionAt("InspectConstructionDomain")->SetStyle("Button");
		AddAction(
		    "InspectAttributeCreationParameters", "Tree construction parameters",
		    (ActionMethod)(&KWAttributeConstructionSpecView::DEPRECATEDInspectAttributeCreationParameters));
		GetActionAt("InspectAttributeCreationParameters")->SetStyle("Button");
		AddAction("InspectRecodingSpec", "Recoding parameters",
			  (ActionMethod)(&KWAttributeConstructionSpecView::DEPRECATEDInspectRecodingSpec));
		GetActionAt("InspectRecodingSpec")->SetStyle("Button");
	}
#endif // DEPRECATED_V10

	// Parametrage de la construction des variables de type texte
	GetFieldAt("MaxTextFeatureNumber")->SetVisible(GetLearningTextVariableMode());

	// Parametrage de la construction des arbres
	GetFieldAt("MaxTreeNumber")
	    ->SetVisible(KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL);

	// Info-bulles
	GetFieldAt("MaxConstructedAttributeNumber")
	    ->SetHelpText("Max number of variables to construct."
			  "\n The constructed variables allow to extract numerical or categorical values"
			  "\n resulting from computing formula applied to existing variables in secondary tables"
			  "\n (e.g. YearDay of a Date variable, Mean of a Numerical variable from a Table Variable).");
	GetFieldAt("MaxTextFeatureNumber")
	    ->SetHelpText("Max number of text features to construct."
			  "\n Text features are constructed from variables of type Text or TextList available in "
			  "either the main table or"
			  "\n a secondary table. The features are constructed by default using n-grams of bytes "
			  "randomly projected on hash tables,"
			  "\n the lengths of the n-grams and the size of the hash tables increasing with the number of "
			  "features constructed.");
	GetFieldAt("MaxTreeNumber")
	    ->SetHelpText("Max number of trees to construct."
			  "\n The constructed trees allow to combine variables, either native or constructed."
			  "\n Construction of trees is not available in regression analysis.");
	GetFieldAt("MaxAttributePairNumber")
	    ->SetHelpText("Max number of variable pairs to analyze during data preparation."
			  "\n The variable pairs are preprocessed using a bivariate discretization method."
			  "\n Pairs of variable are not available in regression analysis");

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: champs et actions obsoletes, conserves de facon cachee en V10 pour compatibilite
		// ascendante des scenarios Champ et action a supprimer a terme de cette vue, et les deux derniers
		// champs sont a supprimer de KWAttributeConstructionSpec.dd pour la generation de code
		GetFieldAt("MandatoryAttributeInPairs")
		    ->SetHelpText("Only the pairs containing the specified variable are preprocessed"
				  "\n (default: no specified mandatory variable)");
		GetFieldAt("RecodingClass")
		    ->SetHelpText("Build a new dictionary that specifies the recoding (discretization or value "
				  "grouping) of all the analyzed variables.");
		GetActionAt("InspectConstructionDomain")
		    ->SetHelpText("Advanced parameters to select the construction rules used for automatic variable "
				  "construction.");
		GetActionAt("InspectRecodingSpec")
		    ->SetHelpText("Advanced parameters to specify which variables to keep or discard"
				  "\n once the data preparation is performed and to choose the recoding method.");
		//
		GetActionAt("InspectAttributeCreationParameters")
		    ->SetVisible(KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
				 KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView() != NULL);
		//
		GetActionAt("InspectConstructionDomain")->SetShortCut('C');
		GetActionAt("InspectAttributeCreationParameters")->SetShortCut('M');
		GetActionAt("InspectRecodingSpec")->SetShortCut('G');
		//
		GetFieldAt("MandatoryAttributeInPairs")->SetVisible(false);
		GetFieldAt("RecodingClass")->SetVisible(false);
		GetActionAt("InspectConstructionDomain")->SetVisible(false);
		GetActionAt("InspectAttributeCreationParameters")->SetVisible(false);
		GetActionAt("InspectRecodingSpec")->SetVisible(false);
	}
#endif // DEPRECATED_V10

	// ##
}

KWAttributeConstructionSpecView::~KWAttributeConstructionSpecView()
{
	// ## Custom destructor

	// ##
}

KWAttributeConstructionSpec* KWAttributeConstructionSpecView::GetKWAttributeConstructionSpec()
{
	require(objValue != NULL);
	return cast(KWAttributeConstructionSpec*, objValue);
}

void KWAttributeConstructionSpecView::EventUpdate(Object* object)
{
	KWAttributeConstructionSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeConstructionSpec*, object);
	editedObject->SetMaxConstructedAttributeNumber(GetIntValueAt("MaxConstructedAttributeNumber"));
	editedObject->SetMaxTextFeatureNumber(GetIntValueAt("MaxTextFeatureNumber"));
	editedObject->SetMaxTreeNumber(GetIntValueAt("MaxTreeNumber"));
	editedObject->SetMaxAttributePairNumber(GetIntValueAt("MaxAttributePairNumber"));
	editedObject->SetMandatoryAttributeInPairs(GetStringValueAt("MandatoryAttributeInPairs"));
	editedObject->SetRecodingClass(GetBooleanValueAt("RecodingClass"));

	// ## Custom update

#ifdef DEPRECATED_V10
	editedObject->SetMandatoryAttributeInPairs(GetStringValueAt("MandatoryAttributeInPairs"));
	editedObject->SetRecodingClass(GetBooleanValueAt("RecodingClass"));
#endif // DEPRECATED_V10

	// ##
}

void KWAttributeConstructionSpecView::EventRefresh(Object* object)
{
	KWAttributeConstructionSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeConstructionSpec*, object);
	SetIntValueAt("MaxConstructedAttributeNumber", editedObject->GetMaxConstructedAttributeNumber());
	SetIntValueAt("MaxTextFeatureNumber", editedObject->GetMaxTextFeatureNumber());
	SetIntValueAt("MaxTreeNumber", editedObject->GetMaxTreeNumber());
	SetIntValueAt("MaxAttributePairNumber", editedObject->GetMaxAttributePairNumber());
	SetStringValueAt("MandatoryAttributeInPairs", editedObject->GetMandatoryAttributeInPairs());
	SetBooleanValueAt("RecodingClass", editedObject->GetRecodingClass());

	// ## Custom refresh

#ifdef DEPRECATED_V10
	SetStringValueAt("MandatoryAttributeInPairs", editedObject->GetMandatoryAttributeInPairs());
	SetBooleanValueAt("RecodingClass", editedObject->GetRecodingClass());
#endif // DEPRECATED_V10

	// ##
}

const ALString KWAttributeConstructionSpecView::GetClassLabel() const
{
	return "Feature engineering parameters";
}

// ## Method implementation

#ifdef DEPRECATED_V10
void KWAttributeConstructionSpecView::DEPRECATEDInspectConstructionDomain()
{
	static boolean bWarningEmited = false;
	KDConstructionDomainView constructionDomainView;
	KWAttributeConstructionSpec* editedObject;

	// Warning utilisateur
	if (not bWarningEmited)
	{
		AddWarning("Button 'Variable construction parameters' is deprecated in this pane since Khiops V10 :"
			   "\n   use button from pane 'Parameters/Predictors/Advanced predictor parameters' instead");
		bWarningEmited = true;
	}

	// Acces a l'objet edite
	editedObject = cast(KWAttributeConstructionSpec*, GetObject());
	check(editedObject);

	// Ouverture de la sous-fiche
	constructionDomainView.SetObject(editedObject->GetConstructionDomain());
	constructionDomainView.Open();
}

void KWAttributeConstructionSpecView::DEPRECATEDInspectAttributeCreationParameters()
{
	static boolean bWarningEmited = false;
	KWAttributeConstructionSpec* editedObject;
	KDDataPreparationAttributeCreationTaskView* editedObjectView;

	// Warning utilisateur
	if (not bWarningEmited)
	{
		AddWarning("Button 'Tree construction parameters' is deprecated in this pane since Khiops V10 :"
			   "\n   use button from pane 'Parameters/Predictors/Advanced predictor parameters' instead");
		bWarningEmited = true;
	}

	// Acces a l'objet edite
	editedObject = cast(KWAttributeConstructionSpec*, GetObject());
	check(editedObject);

	// Message si pas d'objet edite
	if (editedObject->GetAttributeCreationParameters() == NULL)
		AddMessage("No parameter available");
	// Message si pas de vue d'edition des parametres
	else if (KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView() == NULL)
		AddMessage("No view available to update parameters");
	// Sinon, edition de l'objet
	else
	{
		editedObjectView = KDDataPreparationAttributeCreationTaskView::GetGlobalCreationTaskView();

		// Ouverture de la sous-fiche
		editedObjectView->SetObject(editedObject->GetAttributeCreationParameters());
		editedObjectView->Open();
	}
}

void KWAttributeConstructionSpecView::DEPRECATEDInspectRecodingSpec()
{
	static boolean bWarningEmited = false;
	KWRecodingSpecView recodingSpecView;
	KWAttributeConstructionSpec* editedObject;

	// Warning utilisateur
	if (not bWarningEmited)
	{
		AddWarning("Button 'Recoding parameters' is deprecated in this pane since Khiops V10 :"
			   "\n   use button from pane 'Parameters/Predictors/Advanced predictor parameters' instead");
		bWarningEmited = true;
	}

	// Acces a l'objet edite
	editedObject = cast(KWAttributeConstructionSpec*, GetObject());
	check(editedObject);

	// Ouverture de la sous-fiche
	recodingSpecView.SetObject(editedObject->DEPRECATEDGetRecodingSpec());
	recodingSpecView.Open();
}
#endif // DEPRECATED_V10

// ##
