// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KDConstructionDomainView.h"

KDConstructionDomainView::KDConstructionDomainView()
{
	SetIdentifier("KDConstructionDomain");
	SetLabel("Variable construction parameters");
	AddBooleanField("InterpretableNames", "Build interpretable names", false);
	AddBooleanField("RuleOptimization", "Optimize constructed rules", false);
	AddBooleanField("SparseOptimization", "Build sparse rules", false);
	AddIntField("SparseBlockMinSize", "Sparse blokc min size", 0);
	AddBooleanField("ImportAttributeConstructionCosts", "Import variable construction costs (expert)", false);
	AddBooleanField("ConstructionRegularization", "Construction regularization (expert)", false);

	// Parametrage des styles;
	GetFieldAt("InterpretableNames")->SetStyle("CheckBox");
	GetFieldAt("RuleOptimization")->SetStyle("CheckBox");
	GetFieldAt("SparseOptimization")->SetStyle("CheckBox");
	GetFieldAt("SparseBlockMinSize")->SetStyle("Spinner");
	GetFieldAt("ImportAttributeConstructionCosts")->SetStyle("CheckBox");
	GetFieldAt("ConstructionRegularization")->SetStyle("CheckBox");

	// ## Custom constructor

	KDConstructionDomain defaultConstructionDomain;

	// Parametrage des nombre min et max
	cast(UIIntElement*, GetFieldAt("SparseBlockMinSize"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("SparseBlockMinSize"))->SetMaxValue(1000);

	// Ajout d'une liste des attributs
	KDConstructionRuleArrayView* constructionRuleArrayView = new KDConstructionRuleArrayView;
	AddListField("ConstructionRules", "Construction rules", constructionRuleArrayView);

	// On passe le tableau en mode non editable, sauf pour le champ utilise
	constructionRuleArrayView->SetEditable(false);
	constructionRuleArrayView->GetFieldAt("Used")->SetEditable(true);

	// Parametrage du nombre de regles afficheees
	constructionRuleArrayView->SetLineNumber(min(25, defaultConstructionDomain.GetConstructionRuleNumber()));

	// Parametrage d'une taille supplementaires sur la derniere colonne, en nombre de caracteres
	constructionRuleArrayView->SetLastColumnExtraWidth(35);

	// Le parametrage expert n'est visible qu'en mode expert
	boolean bConstructionExpertMode = false;
	GetFieldAt("InterpretableNames")->SetVisible(bConstructionExpertMode and GetLearningExpertMode());
	GetFieldAt("RuleOptimization")->SetVisible(bConstructionExpertMode and GetLearningExpertMode());
	GetFieldAt("SparseOptimization")->SetVisible(bConstructionExpertMode and GetLearningExpertMode());
	GetFieldAt("SparseBlockMinSize")->SetVisible(bConstructionExpertMode and GetLearningExpertMode());
	GetFieldAt("ConstructionRegularization")->SetVisible(bConstructionExpertMode and GetLearningPriorStudyMode());
	GetFieldAt("ImportAttributeConstructionCosts")->SetVisible(GetLearningExpertMode());

	// Ajout des actions
	AddAction("DefaultSelection", "Default", (ActionMethod)(&KDConstructionDomainView::DefaultSelection));
	AddAction("SelectAll", "Select all", (ActionMethod)(&KDConstructionDomainView::SelectAll));
	AddAction("UnselectAll", "Unselect all", (ActionMethod)(&KDConstructionDomainView::UnselectAll));
	GetActionAt("DefaultSelection")->SetStyle("Button");
	GetActionAt("SelectAll")->SetStyle("Button");
	GetActionAt("UnselectAll")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("InterpretableNames")
	    ->SetHelpText("Build interpretable name for the automatically constructed variables.");
	GetFieldAt("RuleOptimization")
	    ->SetHelpText("Optimize the overall computation of all the constructed variables.");
	GetFieldAt("SparseOptimization")
	    ->SetHelpText("Optimize the constructed variables using sparse variable blocks.");
	GetFieldAt("SparseBlockMinSize")->SetHelpText("Minimum size for sparse variable blocks.");
	GetActionAt("DefaultSelection")
	    ->SetHelpText("Select the default construction rules."
			  "\n By default, date and time rules are not selected."
			  "\n They are interesting for exploratory analysis."
			  "\n For supervised analysis, they should be used with caution"
			  "\n as the deployment period may be different from the training period.");
	GetActionAt("SelectAll")->SetHelpText("Select all construction rules.");
	GetActionAt("UnselectAll")->SetHelpText("Unselect all construction rules.");

	// Short cuts
	GetActionAt("DefaultSelection")->SetShortCut('D');
	GetActionAt("SelectAll")->SetShortCut('S');
	GetActionAt("UnselectAll")->SetShortCut('U');

	// ##
}

KDConstructionDomainView::~KDConstructionDomainView()
{
	// ## Custom destructor

	// ##
}

KDConstructionDomain* KDConstructionDomainView::GetKDConstructionDomain()
{
	require(objValue != NULL);
	return cast(KDConstructionDomain*, objValue);
}

void KDConstructionDomainView::EventUpdate(Object* object)
{
	KDConstructionDomain* editedObject;

	require(object != NULL);

	editedObject = cast(KDConstructionDomain*, object);
	editedObject->SetInterpretableNames(GetBooleanValueAt("InterpretableNames"));
	editedObject->SetRuleOptimization(GetBooleanValueAt("RuleOptimization"));
	editedObject->SetSparseOptimization(GetBooleanValueAt("SparseOptimization"));
	editedObject->SetSparseBlockMinSize(GetIntValueAt("SparseBlockMinSize"));
	editedObject->SetImportAttributeConstructionCosts(GetBooleanValueAt("ImportAttributeConstructionCosts"));
	editedObject->SetConstructionRegularization(GetBooleanValueAt("ConstructionRegularization"));

	// ## Custom update

	// ##
}

void KDConstructionDomainView::EventRefresh(Object* object)
{
	KDConstructionDomain* editedObject;

	require(object != NULL);

	editedObject = cast(KDConstructionDomain*, object);
	SetBooleanValueAt("InterpretableNames", editedObject->GetInterpretableNames());
	SetBooleanValueAt("RuleOptimization", editedObject->GetRuleOptimization());
	SetBooleanValueAt("SparseOptimization", editedObject->GetSparseOptimization());
	SetIntValueAt("SparseBlockMinSize", editedObject->GetSparseBlockMinSize());
	SetBooleanValueAt("ImportAttributeConstructionCosts", editedObject->GetImportAttributeConstructionCosts());
	SetBooleanValueAt("ConstructionRegularization", editedObject->GetConstructionRegularization());

	// ## Custom refresh

	// ##
}

const ALString KDConstructionDomainView::GetClassLabel() const
{
	return "Variable construction parameters";
}

// ## Method implementation

void KDConstructionDomainView::DefaultSelection()
{
	GetConstructionDomain()->SelectDefaultConstructionRules();
}

void KDConstructionDomainView::SelectAll()
{
	KDConstructionDomain* constructionDomain;
	int nRule;

	constructionDomain = GetConstructionDomain();
	for (nRule = 0; nRule < constructionDomain->GetConstructionRuleNumber(); nRule++)
		constructionDomain->GetConstructionRuleAt(nRule)->SetUsed(true);
}

void KDConstructionDomainView::UnselectAll()
{
	KDConstructionDomain* constructionDomain;
	int nRule;

	constructionDomain = GetConstructionDomain();
	for (nRule = 0; nRule < constructionDomain->GetConstructionRuleNumber(); nRule++)
		constructionDomain->GetConstructionRuleAt(nRule)->SetUsed(false);
}

void KDConstructionDomainView::SetObject(Object* object)
{
	KDConstructionDomain* constructionDomain;

	require(object != NULL);

	// Acces a l'objet edite
	constructionDomain = cast(KDConstructionDomain*, object);

	// Parametrage de la sous-liste
	cast(KDConstructionRuleArrayView*, GetFieldAt("ConstructionRules"))
	    ->SetObjectArray(constructionDomain->GetConstructionRules());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KDConstructionDomain* KDConstructionDomainView::GetConstructionDomain()
{
	return cast(KDConstructionDomain*, objValue);
}

// ##
