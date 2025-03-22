// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCDeploymentSpecView.h"

CCDeploymentSpecView::CCDeploymentSpecView()
{
	SetIdentifier("CCDeploymentSpec");
	SetLabel("Deployment parameters");
	AddStringField("InputClassName", "Input dictionary", "");
	AddStringField("InputObjectArrayAttributeName", "Input table variable", "");
	AddStringField("DeployedAttributeName", "Coclustering deployed variable", "");
	AddBooleanField("BuildPredictedClusterAttribute", "Build predicted cluster variable", false);
	AddBooleanField("BuildClusterDistanceAttributes", "Build inter-cluster distance variables", false);
	AddBooleanField("BuildFrequencyRecodingAttributes", "Build frequency recoding variables", false);
	AddStringField("OutputAttributesPrefix", "Output variables prefix", "");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("InputClassName")
	    ->SetHelpText("Name of the dictionary that corresponds to the deployment database"
			  "\n that contains the instances of interest.");
	GetFieldAt("InputObjectArrayAttributeName")
	    ->SetHelpText("Name of the table variable in the input dictionary"
			  "\n that contains the detailed record for each instance of interest.");
	GetFieldAt("DeployedAttributeName")
	    ->SetHelpText("Name of the deployed variable, i.e. one of the coclustering variables,"
			  "\n which represents the entity of interest.");
	GetFieldAt("BuildPredictedClusterAttribute")
	    ->SetHelpText("Indicate that the deployment model must generate a new variable"
			  "\n containing the label of the cluster of the entity of interest.");
	GetFieldAt("BuildClusterDistanceAttributes")
	    ->SetHelpText("Indicate that the deployment model must generate new variables"
			  "\n representing the distance of the entity of interest to each cluster.");
	GetFieldAt("BuildFrequencyRecodingAttributes")
	    ->SetHelpText("Indicates that the deployment model must generate new variables"
			  "\n representing the frequency per cluster of the other coclustering variables.");
	GetFieldAt("OutputAttributesPrefix")
	    ->SetHelpText("Prefix added to the deployment variables in the deployment dictionary.");

	// ##
}

CCDeploymentSpecView::~CCDeploymentSpecView()
{
	// ## Custom destructor

	// ##
}

CCDeploymentSpec* CCDeploymentSpecView::GetCCDeploymentSpec()
{
	require(objValue != NULL);
	return cast(CCDeploymentSpec*, objValue);
}

void CCDeploymentSpecView::EventUpdate(Object* object)
{
	CCDeploymentSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCDeploymentSpec*, object);
	editedObject->SetInputClassName(GetStringValueAt("InputClassName"));
	editedObject->SetInputObjectArrayAttributeName(GetStringValueAt("InputObjectArrayAttributeName"));
	editedObject->SetDeployedAttributeName(GetStringValueAt("DeployedAttributeName"));
	editedObject->SetBuildPredictedClusterAttribute(GetBooleanValueAt("BuildPredictedClusterAttribute"));
	editedObject->SetBuildClusterDistanceAttributes(GetBooleanValueAt("BuildClusterDistanceAttributes"));
	editedObject->SetBuildFrequencyRecodingAttributes(GetBooleanValueAt("BuildFrequencyRecodingAttributes"));
	editedObject->SetOutputAttributesPrefix(GetStringValueAt("OutputAttributesPrefix"));

	// ## Custom update

	// ##
}

void CCDeploymentSpecView::EventRefresh(Object* object)
{
	CCDeploymentSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCDeploymentSpec*, object);
	SetStringValueAt("InputClassName", editedObject->GetInputClassName());
	SetStringValueAt("InputObjectArrayAttributeName", editedObject->GetInputObjectArrayAttributeName());
	SetStringValueAt("DeployedAttributeName", editedObject->GetDeployedAttributeName());
	SetBooleanValueAt("BuildPredictedClusterAttribute", editedObject->GetBuildPredictedClusterAttribute());
	SetBooleanValueAt("BuildClusterDistanceAttributes", editedObject->GetBuildClusterDistanceAttributes());
	SetBooleanValueAt("BuildFrequencyRecodingAttributes", editedObject->GetBuildFrequencyRecodingAttributes());
	SetStringValueAt("OutputAttributesPrefix", editedObject->GetOutputAttributesPrefix());

	// ## Custom refresh

	// ##
}

const ALString CCDeploymentSpecView::GetClassLabel() const
{
	return "Deployment parameters";
}

// ## Method implementation

// ##
