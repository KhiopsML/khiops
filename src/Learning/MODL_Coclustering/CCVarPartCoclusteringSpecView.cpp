// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCVarPartCoclusteringSpecView.h"

CCVarPartCoclusteringSpecView::CCVarPartCoclusteringSpecView()
{
	SetIdentifier("CCVarPartCoclusteringSpec");
	SetLabel("Instances Variables coclustering parameters");
	AddStringField("IdentifierAttributeName", "Identifier variable", "");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("IdentifierAttributeName")
	    ->SetHelpText(
		"Identifier variable (optional)."
		"\n In the case of instances x variables coclustering, name of a variable that contains the identifier "
		"of the records."
		"\n For the 'instances' dimension of the coclustering, such a variable is automatically created if not "
		"specified."
		"\n For the 'variables' dimension of the coclustering, all numerical and categorical variables"
		"\n used in the input dictionary are employed.");
	// ##
}

CCVarPartCoclusteringSpecView::~CCVarPartCoclusteringSpecView()
{
	// ## Custom destructor

	// ##
}

CCVarPartCoclusteringSpec* CCVarPartCoclusteringSpecView::GetCCVarPartCoclusteringSpec()
{
	require(objValue != NULL);
	return cast(CCVarPartCoclusteringSpec*, objValue);
}

void CCVarPartCoclusteringSpecView::EventUpdate(Object* object)
{
	CCVarPartCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCVarPartCoclusteringSpec*, object);
	editedObject->SetIdentifierAttributeName(GetStringValueAt("IdentifierAttributeName"));

	// ## Custom update

	// ##
}

void CCVarPartCoclusteringSpecView::EventRefresh(Object* object)
{
	CCVarPartCoclusteringSpec* editedObject;

	require(object != NULL);

	editedObject = cast(CCVarPartCoclusteringSpec*, object);
	SetStringValueAt("IdentifierAttributeName", editedObject->GetIdentifierAttributeName());

	// ## Custom refresh

	// ##
}

const ALString CCVarPartCoclusteringSpecView::GetClassLabel() const
{
	return "Instances Variables coclustering parameters";
}

// ## Method implementation

void CCVarPartCoclusteringSpecView::SetObject(Object* object)
{
	CCVarPartCoclusteringSpec* coclusteringSpec;

	require(object != NULL);

	// Acces a l'objet edite
	coclusteringSpec = cast(CCVarPartCoclusteringSpec*, object);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

// ##
