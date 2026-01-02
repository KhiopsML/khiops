// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeConstructionSpec.h"

KWAttributeConstructionSpec::KWAttributeConstructionSpec()
{
	bKeepSelectedAttributesOnly = true;
	nMaxConstructedAttributeNumber = 0;
	nMaxTextFeatureNumber = 0;
	nMaxTreeNumber = 0;

	// On initialise la tache de creation de variable par rapport au parametrage global
	attributeCreationTask = KDDataPreparationAttributeCreationTask::CloneGlobalCreationTask();
}

KWAttributeConstructionSpec::~KWAttributeConstructionSpec()
{
	if (attributeCreationTask != NULL)
		delete attributeCreationTask;
}

void KWAttributeConstructionSpec::CopyFrom(const KWAttributeConstructionSpec* aSource)
{
	require(aSource != NULL);

	bKeepSelectedAttributesOnly = aSource->bKeepSelectedAttributesOnly;
	nMaxConstructedAttributeNumber = aSource->nMaxConstructedAttributeNumber;
	nMaxTextFeatureNumber = aSource->nMaxTextFeatureNumber;
	nMaxTreeNumber = aSource->nMaxTreeNumber;
	SetMaxAttributePairNumber(aSource->GetMaxAttributePairNumber());
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
	return &constructionDomain;
}

KDTextFeatureSpec* KWAttributeConstructionSpec::GetTextFeatureSpec()
{
	return &textFeatureSpec;
}

KDDataPreparationAttributeCreationTask* KWAttributeConstructionSpec::GetAttributeCreationParameters()
{
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

void KWAttributeConstructionSpec::WriteHeaderLineReport(ostream& ost)
{
	ost << "Constr. vars\t";
	ost << "Text vars\t";
	ost << "Tree vars\t";
	ost << "Var pairs\t";
}

void KWAttributeConstructionSpec::WriteLineReport(ostream& ost)
{
	ost << GetMaxConstructedAttributeNumber() << "\t";
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
