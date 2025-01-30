// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPreprocessingSpec.h"

KWDiscretizerSpec::KWDiscretizerSpec()
{
	sSupervisedMethodName = "MODL";
	sUnsupervisedMethodName = "EqualWidth";
	dParam = 0;
	nMinIntervalFrequency = 0;
	nMaxIntervalNumber = 0;
	nFreshness = 0;
	supervisedDiscretizer = NULL;
	unsupervisedDiscretizer = NULL;
	nSupervisedDiscretizerFreshness = 0;
	nUnsupervisedDiscretizerFreshness = 0;
}

KWDiscretizerSpec::~KWDiscretizerSpec()
{
	if (supervisedDiscretizer != NULL)
		delete supervisedDiscretizer;
	if (unsupervisedDiscretizer != NULL)
		delete unsupervisedDiscretizer;
}

const ALString KWDiscretizerSpec::GetMethodName(int nTargetAttributeType) const
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::Continuous or
		nTargetAttributeType == KWType::None);

	if (nTargetAttributeType == KWType::None)
		return GetUnsupervisedMethodName();
	else if (nTargetAttributeType == KWType::Symbol)
		return GetSupervisedMethodName();
	else
		return "MODL";
}

const ALString KWDiscretizerSpec::GetMethodLabel(int nTargetAttributeType) const
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::Continuous or
		nTargetAttributeType == KWType::None);

	if (GetParam() > 0)
		return GetMethodName(nTargetAttributeType) + "(" + IntToString(GetMinIntervalFrequency()) + ", " +
		       IntToString(GetMaxIntervalNumber()) + ", " + DoubleToString(GetParam()) + ")";
	else if (GetMinIntervalFrequency() > 0 or GetMaxIntervalNumber() > 0)
		return GetMethodName(nTargetAttributeType) + "(" + IntToString(GetMinIntervalFrequency()) + ", " +
		       IntToString(GetMaxIntervalNumber()) + ")";
	else
		return GetMethodName(nTargetAttributeType);
}

const ALString& KWDiscretizerSpec::GetSupervisedMethodName() const
{
	return sSupervisedMethodName;
}

void KWDiscretizerSpec::SetSupervisedMethodName(const ALString& sValue)
{
	if (sValue != sSupervisedMethodName)
	{
		sSupervisedMethodName = sValue;
		nFreshness++;
	}
}

const ALString& KWDiscretizerSpec::GetUnsupervisedMethodName() const
{
	return sUnsupervisedMethodName;
}

void KWDiscretizerSpec::SetUnsupervisedMethodName(const ALString& sValue)
{
	if (sValue != sUnsupervisedMethodName)
	{
		sUnsupervisedMethodName = sValue;
		nFreshness++;
	}
}

double KWDiscretizerSpec::GetParam() const
{
	return dParam;
}

void KWDiscretizerSpec::SetParam(double dValue)
{
	require(dValue >= 0);
	if (dValue != dParam)
	{
		dParam = dValue;
		nFreshness++;
	}
}

int KWDiscretizerSpec::GetMinIntervalFrequency() const
{
	return nMinIntervalFrequency;
}

void KWDiscretizerSpec::SetMinIntervalFrequency(int nValue)
{
	require(nValue >= 0);
	if (nValue != nMinIntervalFrequency)
	{
		nMinIntervalFrequency = nValue;
		nFreshness++;
	}
}

int KWDiscretizerSpec::GetMaxIntervalNumber() const
{
	return nMaxIntervalNumber;
}

void KWDiscretizerSpec::SetMaxIntervalNumber(int nValue)
{
	require(nValue >= 0);
	if (nValue != nMaxIntervalNumber)
	{
		nMaxIntervalNumber = nValue;
		nFreshness++;
	}
}

boolean KWDiscretizerSpec::Check() const
{
	boolean bOk = true;

	bOk = CheckForTargetType(KWType::None) and bOk;
	bOk = CheckForTargetType(KWType::Symbol) and bOk;
	return bOk;
}

boolean KWDiscretizerSpec::CheckForTargetType(int nTargetAttributeType) const
{
	boolean bOk = true;
	if (nTargetAttributeType == KWType::None)
	{
		if (GetUnsupervisedMethodName() != "None")
			bOk = GetDiscretizer(KWType::None) != NULL and GetDiscretizer(KWType::None)->Check() and bOk;
	}
	else if (nTargetAttributeType == KWType::Symbol)
		bOk = GetDiscretizer(KWType::Symbol) != NULL and GetDiscretizer(KWType::Symbol)->Check() and bOk;
	return bOk;
}

void KWDiscretizerSpec::CopyFrom(const KWDiscretizerSpec* kwdsSource)
{
	require(kwdsSource != NULL);

	SetSupervisedMethodName(kwdsSource->GetSupervisedMethodName());
	SetUnsupervisedMethodName(kwdsSource->GetUnsupervisedMethodName());
	SetParam(kwdsSource->GetParam());
	SetMinIntervalFrequency(kwdsSource->GetMinIntervalFrequency());
	SetMaxIntervalNumber(kwdsSource->GetMaxIntervalNumber());
	nFreshness = kwdsSource->GetFreshness() + 1;
	nSupervisedDiscretizerFreshness = 0;
	nUnsupervisedDiscretizerFreshness = 0;
}

KWDiscretizerSpec* KWDiscretizerSpec::Clone() const
{
	KWDiscretizerSpec* kwdsClone;

	kwdsClone = new KWDiscretizerSpec;
	kwdsClone->CopyFrom(this);
	return kwdsClone;
}

const KWDiscretizer* KWDiscretizerSpec::GetDiscretizer(int nTargetAttributeType) const
{
	KWDiscretizer* discretizer;
	int nDiscretizerFreshness;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Parametrage du mode supervise ou non supervise pour rendre le reste de la methode generique
	if (nTargetAttributeType == KWType::None)
	{
		discretizer = unsupervisedDiscretizer;
		nDiscretizerFreshness = nUnsupervisedDiscretizerFreshness;
	}
	else
	{
		discretizer = supervisedDiscretizer;
		nDiscretizerFreshness = nSupervisedDiscretizerFreshness;
	}

	// On construit le discretiser s'il n'est pas disponible ou que ses
	// specifications ont ete modifiees
	if (discretizer == NULL or nDiscretizerFreshness != GetFreshness())
	{
		// Nettoyage eventuel
		if (discretizer != NULL)
			delete discretizer;
		discretizer = NULL;

		// Si necessaire, on demande a l'administration des discretizers
		// d'en fabriquer un d'apres son nom
		if (GetMethodName(nTargetAttributeType) == "")
			Global::AddError(GetClassLabel(), GetMethodName(nTargetAttributeType), "Missing method name");
		// On ne peut demander None qu'en non supervise
		else if (not(GetMethodName(nTargetAttributeType) == "None" and nTargetAttributeType == KWType::None))
		{
			discretizer = KWDiscretizer::CloneDiscretizer(GetMethodName(nTargetAttributeType));
			if (discretizer == NULL)
				Global::AddError(GetClassLabel(), GetMethodName(nTargetAttributeType),
						 "Unknown method");
		}

		// Parametrage
		if (discretizer != NULL)
		{
			discretizer->SetParam(GetParam());
			discretizer->SetMinIntervalFrequency(GetMinIntervalFrequency());
			discretizer->SetMaxIntervalNumber(GetMaxIntervalNumber());
		}

		// Destruction si non valide
		if (discretizer != NULL and not discretizer->Check())
		{
			delete discretizer;
			discretizer = NULL;
		}
	}

	// Memorisation du discretizer selon son type
	if (nTargetAttributeType == KWType::None)
	{
		unsupervisedDiscretizer = discretizer;
		nUnsupervisedDiscretizerFreshness = nFreshness;
	}
	else
	{
		supervisedDiscretizer = discretizer;
		nSupervisedDiscretizerFreshness = nFreshness;
	}
	return discretizer;
}

int KWDiscretizerSpec::GetFreshness() const
{
	return nFreshness;
}

const ALString KWDiscretizerSpec::GetClassLabel() const
{
	return "Discretization";
}

const ALString KWDiscretizerSpec::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DiscretizerSpec

PLShared_DiscretizerSpec::PLShared_DiscretizerSpec() {}

PLShared_DiscretizerSpec::~PLShared_DiscretizerSpec() {}

void PLShared_DiscretizerSpec::SetDiscretizerSpec(KWDiscretizerSpec* discretizerSpec)
{
	require(discretizerSpec != NULL);
	SetObject(discretizerSpec);
}

KWDiscretizerSpec* PLShared_DiscretizerSpec::GetDiscretizerSpec()
{
	return cast(KWDiscretizerSpec*, GetObject());
}

void PLShared_DiscretizerSpec::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDiscretizerSpec* discretizerSpec;

	require(serializer->IsOpenForWrite());

	discretizerSpec = cast(KWDiscretizerSpec*, o);
	serializer->PutString(discretizerSpec->GetSupervisedMethodName());
	serializer->PutString(discretizerSpec->GetUnsupervisedMethodName());
	serializer->PutDouble(discretizerSpec->GetParam());
	serializer->PutInt(discretizerSpec->GetMinIntervalFrequency());
	serializer->PutInt(discretizerSpec->GetMaxIntervalNumber());
}

void PLShared_DiscretizerSpec::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDiscretizerSpec* discretizerSpec;

	require(serializer->IsOpenForRead());

	discretizerSpec = cast(KWDiscretizerSpec*, o);
	discretizerSpec->SetSupervisedMethodName(serializer->GetString());
	discretizerSpec->SetUnsupervisedMethodName(serializer->GetString());
	discretizerSpec->SetParam(serializer->GetDouble());
	discretizerSpec->SetMinIntervalFrequency(serializer->GetInt());
	discretizerSpec->SetMaxIntervalNumber(serializer->GetInt());
}

Object* PLShared_DiscretizerSpec::Create() const
{
	return new KWDiscretizerSpec;
}
