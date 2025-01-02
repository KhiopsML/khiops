// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeConstructionReport.h"

KWAttributeConstructionReport::KWAttributeConstructionReport()
{
	attributeConstructionSpec = NULL;
}

KWAttributeConstructionReport::~KWAttributeConstructionReport() {}

void KWAttributeConstructionReport::SetAttributeConstructionSpec(const KWAttributeConstructionSpec* spec)
{
	attributeConstructionSpec = spec;
}

const KWAttributeConstructionSpec* KWAttributeConstructionReport::GetAttributeConstructionSpec() const
{
	return attributeConstructionSpec;
}

void KWAttributeConstructionReport::WriteReport(ostream& ost)
{
	if (attributeConstructionSpec != NULL)
	{
		ost << "Max number of constructed variables\t"
		    << attributeConstructionSpec->GetMaxConstructedAttributeNumber() << "\n";
		if (GetLearningTextVariableMode())
			ost << "Max number of text features\t" << attributeConstructionSpec->GetMaxTextFeatureNumber()
			    << "\n";
		ost << "Max number of trees\t" << attributeConstructionSpec->GetMaxTreeNumber() << "\n";
		ost << "Max number of variable pairs\t" << attributeConstructionSpec->GetMaxAttributePairNumber()
		    << "\n";
	}
}

void KWAttributeConstructionReport::WriteJSONReport(JSONFile* fJSON)
{
	if (attributeConstructionSpec != NULL)
	{
		fJSON->BeginKeyObject("featureEngineering");
		fJSON->WriteKeyInt("maxNumberOfConstructedVariables",
				   attributeConstructionSpec->GetMaxConstructedAttributeNumber());
		if (GetLearningTextVariableMode())
			fJSON->WriteKeyInt("maxNumberOfTextFeatures",
					   attributeConstructionSpec->GetMaxTextFeatureNumber());
		fJSON->WriteKeyInt("maxNumberOfTrees", attributeConstructionSpec->GetMaxTreeNumber());
		fJSON->WriteKeyInt("maxNumberOfVariablePairs", attributeConstructionSpec->GetMaxAttributePairNumber());
		fJSON->EndObject();
	}
}
