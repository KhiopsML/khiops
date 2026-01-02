// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizer

KWDiscretizer::KWDiscretizer()
{
	dParam = 0;
	nMinIntervalFrequency = 0;
	nMaxIntervalNumber = 0;
}

KWDiscretizer::~KWDiscretizer() {}

const ALString KWDiscretizer::GetName() const
{
	return "???";
}

boolean KWDiscretizer::IsMODLFamily() const
{
	return false;
}

double KWDiscretizer::GetParam() const
{
	return dParam;
}

void KWDiscretizer::SetParam(double dValue)
{
	require(dValue >= 0);
	dParam = dValue;
}

int KWDiscretizer::GetMinIntervalFrequency() const
{
	return nMinIntervalFrequency;
}

void KWDiscretizer::SetMinIntervalFrequency(int nValue)
{
	require(nValue >= 0);
	nMinIntervalFrequency = nValue;
}

int KWDiscretizer::GetMaxIntervalNumber() const
{
	return nMaxIntervalNumber;
}

void KWDiscretizer::SetMaxIntervalNumber(int nValue)
{
	require(nValue >= 0);
	nMaxIntervalNumber = nValue;
}

boolean KWDiscretizer::IsUsingSourceValues() const
{
	return false;
}

KWDiscretizer* KWDiscretizer::Clone() const
{
	KWDiscretizer* kwdClone;

	kwdClone = Create();
	kwdClone->CopyFrom(this);
	return kwdClone;
}

void KWDiscretizer::CopyFrom(const KWDiscretizer* kwdSource)
{
	require(kwdSource != NULL);

	dParam = kwdSource->dParam;
	nMinIntervalFrequency = kwdSource->nMinIntervalFrequency;
	nMaxIntervalNumber = kwdSource->nMaxIntervalNumber;
}

void KWDiscretizer::Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	require(kwftSource != NULL);
	require(kwftSource->Check());
	require(not IsUsingSourceValues());
	kwftTarget = NULL;
	assert(false);
}

void KWDiscretizer::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
				     int nTargetValueNumber, KWFrequencyTable*& kwftTarget,
				     ContinuousVector*& cvBounds) const
{
	require(cvSourceValues != NULL);
	require(ivTargetIndexes != NULL);
	require(nTargetValueNumber >= 0);
	require(IsUsingSourceValues());
	kwftTarget = NULL;
	cvBounds = NULL;
	assert(false);
}

boolean KWDiscretizer::Check() const
{
	return false;
}

const ALString KWDiscretizer::GetClassLabel() const
{
	return "Discretization";
}

const ALString KWDiscretizer::GetObjectLabel() const
{
	if (GetParam() > 0)
		return GetName() + "(" + IntToString(GetMinIntervalFrequency()) + ", " +
		       IntToString(GetMaxIntervalNumber()) + ", " + DoubleToString(GetParam()) + ")";
	else if (GetMinIntervalFrequency() > 0 or GetMaxIntervalNumber() > 0)
		return GetName() + "(" + IntToString(GetMinIntervalFrequency()) + ", " +
		       IntToString(GetMaxIntervalNumber()) + ")";
	else
		return GetName();
}

///////////////////////////////////////////////////////////////////////////

void KWDiscretizer::RegisterDiscretizer(int nTargetAttributeType, KWDiscretizer* discretizer)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);
	require(discretizer != NULL);
	require(discretizer->GetName() != "");
	require(GetDiscretizers(nTargetAttributeType)->Lookup(discretizer->GetName()) == NULL);

	// Memorisation du Discretizer
	GetDiscretizers(nTargetAttributeType)->SetAt(discretizer->GetName(), discretizer);
}

KWDiscretizer* KWDiscretizer::LookupDiscretizer(int nTargetAttributeType, const ALString& sName)
{
	KWDiscretizer* discretizer;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche du predicteur du bon type
	discretizer = cast(KWDiscretizer*, GetDiscretizers(nTargetAttributeType)->Lookup(sName));
	return discretizer;
}

KWDiscretizer* KWDiscretizer::CloneDiscretizer(int nTargetAttributeType, const ALString& sName)
{
	KWDiscretizer* referenceDiscretizer;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche d'un Discretizer de meme nom
	referenceDiscretizer = cast(KWDiscretizer*, GetDiscretizers(nTargetAttributeType)->Lookup(sName));

	// Retour de son Clone si possible
	if (referenceDiscretizer != NULL)
		return referenceDiscretizer->Clone();
	else
		return NULL;
}

void KWDiscretizer::ExportAllDiscretizers(int nTargetAttributeType, ObjectArray* oaDiscretizers)
{
	require(oaDiscretizers != NULL);

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche des predicteurs du bon type
	oaDiscretizers->RemoveAll();
	GetDiscretizers(nTargetAttributeType)->ExportObjectArray(oaDiscretizers);

	// Tri des predicteurs avant de retourner le tableau
	oaDiscretizers->SetCompareFunction(KWDiscretizerCompareName);
	oaDiscretizers->Sort();
}

void KWDiscretizer::RemoveAllDiscretizers()
{
	odSupervisedDiscretizers.RemoveAll();
	odUnsupervisedDiscretizers.RemoveAll();
}

void KWDiscretizer::DeleteAllDiscretizers()
{
	odSupervisedDiscretizers.DeleteAll();
	odUnsupervisedDiscretizers.DeleteAll();
}

ObjectDictionary* KWDiscretizer::GetDiscretizers(int nTargetAttributeType)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);
	if (nTargetAttributeType == KWType::Symbol)
		return &odSupervisedDiscretizers;
	else
		return &odUnsupervisedDiscretizers;
}

ObjectDictionary KWDiscretizer::odSupervisedDiscretizers;
ObjectDictionary KWDiscretizer::odUnsupervisedDiscretizers;

int KWDiscretizerCompareName(const void* first, const void* second)
{
	KWDiscretizer* aFirst;
	KWDiscretizer* aSecond;
	int nResult;

	aFirst = cast(KWDiscretizer*, *(Object**)first);
	aSecond = cast(KWDiscretizer*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}
