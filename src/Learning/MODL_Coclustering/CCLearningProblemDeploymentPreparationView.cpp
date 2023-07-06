// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemDeploymentPreparationView.h"

CCLearningProblemDeploymentPreparationView::CCLearningProblemDeploymentPreparationView()
{
	CCPostProcessingSpecView* postProcessingSpecView;
	CCDeploymentSpecView* deploymentSpecView;
	UIList* inputAttributeNameHelpList;

	// Libelles
	SetIdentifier("CCLearningProblemDeploymentPreparation");
	SetLabel("Coclustering deployment preparation");

	// Ajout de champs rappelant le dictionnaire en entree
	AddStringField("ClassFileName", "Input dictionary file", "");
	GetFieldAt("ClassFileName")->SetEditable(false);

	// Champ du dictionnaire de deploiement en resultat
	AddStringField("CoclusteringDictionaryFileName", "Coclustering dictionary file", "");
	GetFieldAt("CoclusteringDictionaryFileName")->SetStyle("FileChooser");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;
	deploymentSpecView = new CCDeploymentSpecView;

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Simplification parameters", postProcessingSpecView);
	AddCardField("DeploymentSpec", "Deployment parameters", deploymentSpecView);

	// Parametrage de liste d'aide pour le nom de l'attribut de coclustering a deployer
	deploymentSpecView->GetFieldAt("DeployedAttributeName")->SetStyle("HelpedComboBox");
	deploymentSpecView->GetFieldAt("DeployedAttributeName")
	    ->SetParameters("PostProcessingSpec.PostProcessedAttributes:Name");

	// Parametrage de liste d'aide pour le nom du dictionnaire
	deploymentSpecView->GetFieldAt("InputClassName")->SetStyle("EditableComboBox");

	// Initialisation avec la liste des dictionnaires courants
	deploymentSpecView->GetFieldAt("InputClassName")->SetParameters(BuildInputListClassNamesParameter());

	// Creation d'une liste cachee des attributs de la classe en cours
	inputAttributeNameHelpList = new UIList;
	inputAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", inputAttributeNameHelpList);
	inputAttributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom de l'attribut cible
	deploymentSpecView->GetFieldAt("InputObjectArrayAttributeName")->SetStyle("HelpedComboBox");
	deploymentSpecView->GetFieldAt("InputObjectArrayAttributeName")->SetParameters("Attributes:Name");

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Ajout d'actions sous formes de boutons
	AddAction("PrepareDeployment", "Prepare deployment",
		  (ActionMethod)(&CCLearningProblemDeploymentPreparationView::PrepareDeployment));
	GetActionAt("PrepareDeployment")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ClassFileName")
	    ->SetHelpText(
		"Name of the dictionary file that corresponds to the deployment database."
		"\n The input dictionary file must be opened from the main window using the 'Dictionary file' menu.");
	GetFieldAt("CoclusteringDictionaryFileName")
	    ->SetHelpText("Name of the deployment dictionary that contains the coclustering deployment model.");
	GetActionAt("PrepareDeployment")
	    ->SetHelpText(
		"Build a coclustering deployment dictionary."
		"\n The clusters are extracted for a given variable from the simplified coclustering"
		"\n (provided that simplification parameters are specified)."
		"\n"
		"\n Deploying a coclustering model consists in associating each instance of one variable"
		"\n of a coclustering model to the label of its cluster, as well a creating new variables"
		"\n such as the distance of the instance of each cluster."
		"\n The obtained coclustering deployment dictionary allows the user to update a database"
		"\n for a given entity of interest by adding new variables."
		"\n"
		"\n To deploy a coclustering, use the 'Deploy model' functionality of the Khiops back-end tool"
		"\n and apply the deployment dictionary on new data.");

	// Short cuts
	GetFieldAt("PostProcessingSpec")->SetShortCut('M');
	GetFieldAt("DeploymentSpec")->SetShortCut('D');
	GetActionAt("PrepareDeployment")->SetShortCut('P');
}

CCLearningProblemDeploymentPreparationView::~CCLearningProblemDeploymentPreparationView() {}

void CCLearningProblemDeploymentPreparationView::EventUpdate(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventUpdate(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	editedObject->GetAnalysisResults()->SetCoclusteringDictionaryFileName(
	    GetStringValueAt("CoclusteringDictionaryFileName"));

	// Rafraichissement des listes d'aide
	RefreshHelpLists();
}

void CCLearningProblemDeploymentPreparationView::EventRefresh(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventRefresh(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	SetStringValueAt("ClassFileName", editedObject->GetClassFileName());
	SetStringValueAt("CoclusteringDictionaryFileName",
			 editedObject->GetAnalysisResults()->GetCoclusteringDictionaryFileName());
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemDeploymentPreparationView::PrepareDeployment()
{
	// OK si classe correcte et fichiers corrects
	if (GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskPrepareDeployment))
	{
		// Calcul des stats
		GetLearningProblem()->PrepareDeployment();
		AddSimpleMessage("");
	}
}

void CCLearningProblemDeploymentPreparationView::SetObject(Object* object)
{
	CCLearningProblem* learningProblem;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(CCLearningProblem*, object);

	// Parametrage des sous-fiches
	cast(CCPostProcessingSpecView*, GetFieldAt("PostProcessingSpec"))
	    ->SetObject(learningProblem->GetPostProcessingSpec());
	cast(CCDeploymentSpecView*, GetFieldAt("DeploymentSpec"))->SetObject(learningProblem->GetDeploymentSpec());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

const ALString CCLearningProblemDeploymentPreparationView::BuildInputListClassNamesParameter()
{
	ALString sParameter;
	int nClass;

	for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
	{
		if (nClass > 0)
			sParameter += "\n";
		sParameter += KWClassDomain::GetCurrentDomain()->GetClassAt(nClass)->GetName();
	}
	return sParameter;
}

const ALString CCLearningProblemDeploymentPreparationView::BuildCoclusteringSignature()
{
	ALString sSignature;
	int nAttribute;
	CCPostProcessedAttribute* postProcessAttribute;

	// Calcul de la signature du coclustering, fondee sur les noms et types
	for (nAttribute = 0;
	     nAttribute < GetLearningProblem()->GetPostProcessingSpec()->GetPostProcessedAttributes()->GetSize();
	     nAttribute++)
	{
		postProcessAttribute = cast(
		    CCPostProcessedAttribute*,
		    GetLearningProblem()->GetPostProcessingSpec()->GetPostProcessedAttributes()->GetAt(nAttribute));

		// Ajout du type et du nom dans la signature
		sSignature += postProcessAttribute->GetType();
		sSignature += postProcessAttribute->GetName();

		// Separateur en principe interdit dans les nom d'attributs
		sSignature += '`';
	}
	return sSignature;
}

void CCLearningProblemDeploymentPreparationView::RefreshHelpLists()
{
	UIList* inputAttributeNameHelpList;
	KWClass* kwcClass;
	const int nMaxAttributeNumber = 1000;
	int nAttributeNumber;
	KWAttribute* attribute;
	ALString sAttributeHelpListClassName;
	ALString sAttributeHelpListCoclusteringSignature;
	boolean bComputeHelpList;

	// En mode batch, les liste d'aide sont inutiles
	if (UIObject::IsBatchMode())
		return;

	// Test si les valeurs d'aides doivent etre recalculees a cause d'un changement de dictionnaire
	bComputeHelpList = false;
	sAttributeHelpListClassName = GetLearningProblem()->GetDeploymentSpec()->GetInputClassName();
	if (sAttributeHelpListLastClassName != sAttributeHelpListClassName)
		bComputeHelpList = true;

	// Test si les valeurs d'aides doivent etre recalculees a cause d'un changement de dictionnaire
	if (not bComputeHelpList)
	{
		sAttributeHelpListCoclusteringSignature = BuildCoclusteringSignature();
		if (sAttributeHelpListLastCoclusteringSignature != sAttributeHelpListCoclusteringSignature)
			bComputeHelpList = true;
	}

	// Calcul des valeurs d'aide si necessaire
	if (bComputeHelpList)
	{
		// Acces a la list d'aide
		inputAttributeNameHelpList = cast(UIList*, GetFieldAt("Attributes"));

		// On commence par la vider
		inputAttributeNameHelpList->RemoveAllItems();

		// On alimente les attributs si la classe est valide
		kwcClass = NULL;
		if (sAttributeHelpListClassName != "")
			kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sAttributeHelpListClassName);
		if (kwcClass != NULL)
		{
			// Ajout des attributs compatible avec un deploiement de coclustering
			nAttributeNumber = 0;
			attribute = kwcClass->GetHeadAttribute();
			while (attribute != NULL)
			{
				// Ajout si attribut utilisable
				if (GetLearningProblem()
					->GetDeploymentSpec()
					->CheckAttributeConsistencyWithPostProcessedSpec(
					    attribute, GetLearningProblem()->GetPostProcessingSpec()))
				{
					inputAttributeNameHelpList->AddItem();
					inputAttributeNameHelpList->SetStringValueAt("Name", attribute->GetName());

					// Incrementation
					nAttributeNumber++;
					if (nAttributeNumber >= nMaxAttributeNumber)
						break;
				}

				// Passage a l'attribut suivant
				kwcClass->GetNextAttribute(attribute);
			}

			// Si le nombre max est atteint, on remplace la deuxieme moitie des attributs
			// par les attributs de la fin de la classe
			if (inputAttributeNameHelpList->GetItemNumber() >= nMaxAttributeNumber)
			{
				nAttributeNumber = inputAttributeNameHelpList->GetItemNumber();
				attribute = kwcClass->GetTailAttribute();
				while (attribute != NULL)
				{
					// Ajout si attribut utilisable
					if (GetLearningProblem()
						->GetDeploymentSpec()
						->CheckAttributeConsistencyWithPostProcessedSpec(
						    attribute, GetLearningProblem()->GetPostProcessingSpec()))
					{
						inputAttributeNameHelpList->SetCurrentItemIndex(nAttributeNumber - 1);
						inputAttributeNameHelpList->SetStringValueAt("Name",
											     attribute->GetName());

						// Arret quand on a atteint la moitie
						nAttributeNumber--;
						if (nAttributeNumber <= nMaxAttributeNumber / 2)
						{
							// On marque l'attribut en blanc pour indiquer qu'il y en a
							// d'autres
							inputAttributeNameHelpList->SetStringValueAt("Name", "");
							break;
						}
					}

					// Passage a l'attribut precedent
					kwcClass->GetPrevAttribute(attribute);
				}
			}
		}

		// Memorisation des caracteristiques conduisant a un nouveau calcul
		sAttributeHelpListLastClassName = sAttributeHelpListClassName;
		sAttributeHelpListLastCoclusteringSignature = sAttributeHelpListCoclusteringSignature;
	}
}
