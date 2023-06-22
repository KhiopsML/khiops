// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeConstructionSpec.h"

KWAttributeConstructionSpec::KWAttributeConstructionSpec()
{
	nMaxConstructedAttributeNumber = 0;
	nMaxTextFeatureNumber = 0;
	nMaxTreeNumber = 0;

	// On initialise la tache de creation de variable par rapport au parametrage global
	attributeCreationTask = KDDataPreparationAttributeCreationTask::CloneGlobalCreationTask();

#ifdef DEPRECATED_V10
	{
		// Supprimer MandatoryAttributeInPairs et RecodingClass de KWAttributeConstructionSpec.dd et regenerer
		// la classe DEPRECATED V10
		DEPRECATEDrecodingSpec = NULL;
		DEPRECATEDSourceSubObjets = NULL;
		bRecodingClass = false;
	}
#endif // DEPRECATED_V10
}

KWAttributeConstructionSpec::~KWAttributeConstructionSpec()
{
	if (attributeCreationTask != NULL)
		delete attributeCreationTask;
}

void KWAttributeConstructionSpec::CopyFrom(const KWAttributeConstructionSpec* aSource)
{
	require(aSource != NULL);

	nMaxConstructedAttributeNumber = aSource->nMaxConstructedAttributeNumber;
	nMaxTextFeatureNumber = aSource->nMaxTextFeatureNumber;
	nMaxTreeNumber = aSource->nMaxTreeNumber;
	SetMaxAttributePairNumber(aSource->GetMaxAttributePairNumber());
#ifdef DEPRECATED_V10
	{
		sMandatoryAttributeInPairs = aSource->sMandatoryAttributeInPairs;
		bRecodingClass = aSource->bRecodingClass;
	}
#endif // DEPRECATED_V10
}

KWAttributeConstructionSpec* KWAttributeConstructionSpec::Clone() const
{
	KWAttributeConstructionSpec* aClone;

	aClone = new KWAttributeConstructionSpec;
	aClone->CopyFrom(this);
	return aClone;
}

KDConstructionDomain* KWAttributeConstructionSpec::GetConstructionDomain()
{
#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		if (DEPRECATEDSourceSubObjets != NULL)
			return &DEPRECATEDSourceSubObjets->constructionDomain;
	}
#endif // DEPRECATED_V10
	return &constructionDomain;
}

KDTextFeatureSpec* KWAttributeConstructionSpec::GetTextFeatureSpec()
{
	return &textFeatureSpec;
}

KDDataPreparationAttributeCreationTask* KWAttributeConstructionSpec::GetAttributeCreationParameters()
{
#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		if (DEPRECATEDSourceSubObjets != NULL)
			return DEPRECATEDSourceSubObjets->attributeCreationTask;
	}
#endif // DEPRECATED_V10
	return attributeCreationTask;
}

KWAttributePairsSpec* KWAttributeConstructionSpec::GetAttributePairsSpec()
{
	return &attributePairsSpec;
}

void KWAttributeConstructionSpec::SpecifyLearningSpecConstructionFamilies(KWLearningSpec* learningSpec,
									  boolean bIsMultiTableConstructionPossible,
									  boolean bIsTextConstructionPossible)
{
	require(learningSpec->Check());
	require(learningSpec->GetInitialAttributeNumber() != -1);

	// En non supervise, on ne compte pas les paires de variables comme une famille
	learningSpec->SetMultiTableConstruction(GetMaxConstructedAttributeNumber() > 0 and
						bIsMultiTableConstructionPossible);
	learningSpec->SetTextConstruction(GetMaxTextFeatureNumber() > 0 and bIsTextConstructionPossible);
	learningSpec->SetTrees(
	    GetMaxTreeNumber() > 0 and learningSpec->GetTargetAttributeType() == KWType::Symbol and
	    (learningSpec->GetInitialAttributeNumber() + learningSpec->GetMultiTableConstruction() >= 2));
	learningSpec->SetAttributePairs(
	    GetMaxAttributePairNumber() > 0 and learningSpec->GetTargetAttributeType() == KWType::Symbol and
	    (learningSpec->GetInitialAttributeNumber() + learningSpec->GetMultiTableConstruction() +
		 learningSpec->GetTextConstruction() >=
	     2));
}

#ifdef DEPRECATED_V10
KWRecodingSpec* KWAttributeConstructionSpec::DEPRECATEDGetRecodingSpec()
{
	return DEPRECATEDrecodingSpec;
}

void KWAttributeConstructionSpec::DEPRECATEDSetRecodingSpec(KWRecodingSpec* spec)
{
	DEPRECATEDrecodingSpec = spec;
}

void KWAttributeConstructionSpec::DEPRECATEDSetSourceSubObjets(KWAttributeConstructionSpec* source)
{
	DEPRECATEDSourceSubObjets = source;
}
#endif // DEPRECATED_V10

void KWAttributeConstructionSpec::WriteHeaderLineReport(ostream& ost)
{
	ost << "Constr. vars\t";
	if (GetLearningTextVariableMode())
		ost << "Text vars\t";
	ost << "Tree vars\t";
	ost << "Var pairs\t";
}

void KWAttributeConstructionSpec::WriteLineReport(ostream& ost)
{
	ost << GetMaxConstructedAttributeNumber() << "\t";
	if (GetLearningTextVariableMode())
		ost << GetMaxTextFeatureNumber() << "\t";
	ost << GetMaxTreeNumber() << "\t";
	ost << GetMaxAttributePairNumber() << "\t";
}

void KWAttributeConstructionSpec::Write(ostream& ost) const
{
	ost << "Max number of constructed variables\t" << GetMaxConstructedAttributeNumber() << "\n";
	ost << "Max number of text features\t" << GetMaxTextFeatureNumber() << "\n";
	ost << "Max number of trees\t" << GetMaxTreeNumber() << "\n";
	ost << "Max number of variable pairs\t" << GetMaxAttributePairNumber() << "\n";
#ifdef DEPRECATED_V10
	{
		ost << "Only pairs with variable (deprecated)\t" << GetMandatoryAttributeInPairs() << "\n";
		ost << "Build recoding dictionary (deprecated)\t" << BooleanToString(GetRecodingClass()) << "\n";
	}
#endif // DEPRECATED_V10
}

const ALString KWAttributeConstructionSpec::GetClassLabel() const
{
	return "Feature engineering parameters";
}

const ALString KWAttributeConstructionSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}