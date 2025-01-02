// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouperSpec.h"

KWGrouperSpec::KWGrouperSpec()
{
	sSupervisedMethodName = "MODL";
	sUnsupervisedMethodName = "BasicGrouping";
	dParam = 0;
	nMinGroupFrequency = 0;
	nMaxGroupNumber = 0;
	nFreshness = 0;
	supervisedGrouper = NULL;
	unsupervisedGrouper = NULL;
	nSupervisedGrouperFreshness = 0;
	nUnsupervisedGrouperFreshness = 0;
}

KWGrouperSpec::~KWGrouperSpec()
{
	if (supervisedGrouper != NULL)
		delete supervisedGrouper;
	if (unsupervisedGrouper != NULL)
		delete unsupervisedGrouper;
}

const ALString KWGrouperSpec::GetMethodName(int nTargetAttributeType) const
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

const ALString KWGrouperSpec::GetMethodLabel(int nTargetAttributeType) const
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::Continuous or
		nTargetAttributeType == KWType::None);

	if (GetParam() > 0)
		return GetMethodName(nTargetAttributeType) + "(" + IntToString(GetMinGroupFrequency()) + ", " +
		       IntToString(GetMaxGroupNumber()) + ", " + DoubleToString(GetParam()) + ")";
	else if (GetMinGroupFrequency() > 0 or GetMaxGroupNumber() > 0)
		return GetMethodName(nTargetAttributeType) + "(" + IntToString(GetMinGroupFrequency()) + ", " +
		       IntToString(GetMaxGroupNumber()) + ")";
	else
		return GetMethodName(nTargetAttributeType);
}

const ALString& KWGrouperSpec::GetSupervisedMethodName() const
{
	return sSupervisedMethodName;
}

void KWGrouperSpec::SetSupervisedMethodName(const ALString& sValue)
{
	if (sValue != sSupervisedMethodName)
	{
		sSupervisedMethodName = sValue;
		nFreshness++;
	}
}

const ALString& KWGrouperSpec::GetUnsupervisedMethodName() const
{
	return sUnsupervisedMethodName;
}

void KWGrouperSpec::SetUnsupervisedMethodName(const ALString& sValue)
{
	if (sValue != sUnsupervisedMethodName)
	{
		sUnsupervisedMethodName = sValue;
		nFreshness++;
	}
}

double KWGrouperSpec::GetParam() const
{
	return dParam;
}

void KWGrouperSpec::SetParam(double dValue)
{
	require(dValue >= 0);
	if (dValue != dParam)
	{
		dParam = dValue;
		nFreshness++;
	}
}

int KWGrouperSpec::GetMinGroupFrequency() const
{
	return nMinGroupFrequency;
}

void KWGrouperSpec::SetMinGroupFrequency(int nValue)
{
	require(nValue >= 0);
	if (nValue != nMinGroupFrequency)
	{
		nMinGroupFrequency = nValue;
		nFreshness++;
	}
}

int KWGrouperSpec::GetMaxGroupNumber() const
{
	return nMaxGroupNumber;
}

void KWGrouperSpec::SetMaxGroupNumber(int nValue)
{
	require(nValue >= 0);
	if (nValue != nMaxGroupNumber)
	{
		nMaxGroupNumber = nValue;
		nFreshness++;
	}
}

boolean KWGrouperSpec::Check() const
{
	boolean bOk = true;

	bOk = CheckForTargetType(KWType::None) and bOk;
	bOk = CheckForTargetType(KWType::Symbol) and bOk;
	return bOk;
}

boolean KWGrouperSpec::CheckForTargetType(int nTargetAttributeType) const
{
	boolean bOk = true;
	if (nTargetAttributeType == KWType::None)
	{
		if (GetUnsupervisedMethodName() != "None")
			bOk = GetGrouper(KWType::None) != NULL and GetGrouper(KWType::None)->Check() and bOk;
	}
	else if (nTargetAttributeType == KWType::Symbol)
		bOk = GetGrouper(KWType::Symbol) != NULL and GetGrouper(KWType::Symbol)->Check() and bOk;
	return bOk;
}

void KWGrouperSpec::CopyFrom(const KWGrouperSpec* kwgsSource)
{
	require(kwgsSource != NULL);

	SetSupervisedMethodName(kwgsSource->GetSupervisedMethodName());
	SetUnsupervisedMethodName(kwgsSource->GetUnsupervisedMethodName());
	SetParam(kwgsSource->GetParam());
	SetMinGroupFrequency(kwgsSource->GetMinGroupFrequency());
	SetMaxGroupNumber(kwgsSource->GetMaxGroupNumber());
	nFreshness = kwgsSource->GetFreshness() + 1;
	nSupervisedGrouperFreshness = 0;
	nUnsupervisedGrouperFreshness = 0;
}

KWGrouperSpec* KWGrouperSpec::Clone() const
{
	KWGrouperSpec* kwgsClone;

	kwgsClone = new KWGrouperSpec;
	kwgsClone->CopyFrom(this);
	return kwgsClone;
}

const KWGrouper* KWGrouperSpec::GetGrouper(int nTargetAttributeType) const
{
	KWGrouper* grouper;
	int nGrouperFreshness;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Parametrage du mode supervise ou non supervise pour rendre le reste de la methode generique
	if (nTargetAttributeType == KWType::None)
	{
		grouper = unsupervisedGrouper;
		nGrouperFreshness = nUnsupervisedGrouperFreshness;
	}
	else
	{
		grouper = supervisedGrouper;
		nGrouperFreshness = nSupervisedGrouperFreshness;
	}

	// On construit le grouper s'il n'est pas disponible ou que ses specifications ont ete modifiees
	if (grouper == NULL or nGrouperFreshness != GetFreshness())
	{
		// Nettoyage eventuel
		if (grouper != NULL)
			delete grouper;
		grouper = NULL;

		// Si necessaire, on demande a l'administration des groupers
		// d'en fabriquer un d'apres son nom
		if (GetMethodName(nTargetAttributeType) == "")
			Global::AddError(GetClassLabel(), GetMethodName(nTargetAttributeType), "Missing method name");
		// On ne peut demander None qu'en non supervise
		else if (not(GetMethodName(nTargetAttributeType) == "None" and nTargetAttributeType == KWType::None))
		{
			grouper = KWGrouper::CloneGrouper(GetMethodName(nTargetAttributeType));
			if (grouper == NULL)
				Global::AddError(GetClassLabel(), GetMethodName(nTargetAttributeType),
						 "Unknown method");
		}

		// Parametrage
		if (grouper != NULL)
		{
			grouper->SetParam(GetParam());
			grouper->SetMinGroupFrequency(GetMinGroupFrequency());
			grouper->SetMaxGroupNumber(GetMaxGroupNumber());

			// En non supervise, activation forcee du preprocessing car le mode granularite ne s'appliquera
			// pas
			if (nTargetAttributeType == KWType::None)
				grouper->SetActivePreprocessing(true);
		}

		// Destruction si non valide
		if (grouper != NULL and not grouper->Check())
		{
			delete grouper;
			grouper = NULL;
		}
	}

	// Memorisation du grouper selon son type
	if (nTargetAttributeType == KWType::None)
	{
		unsupervisedGrouper = grouper;
		nUnsupervisedGrouperFreshness = nFreshness;
	}
	else
	{
		supervisedGrouper = grouper;
		nSupervisedGrouperFreshness = nFreshness;
	}
	return grouper;
}

int KWGrouperSpec::GetFreshness() const
{
	return nFreshness;
}

const ALString KWGrouperSpec::GetClassLabel() const
{
	return "Value grouping";
}

const ALString KWGrouperSpec::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_GrouperSpec

PLShared_GrouperSpec::PLShared_GrouperSpec() {}

PLShared_GrouperSpec::~PLShared_GrouperSpec() {}

void PLShared_GrouperSpec::SetGrouperSpec(KWGrouperSpec* grouperSpec)
{
	require(grouperSpec != NULL);
	SetObject(grouperSpec);
}

KWGrouperSpec* PLShared_GrouperSpec::GetGrouperSpec()
{
	return cast(KWGrouperSpec*, GetObject());
}

void PLShared_GrouperSpec::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWGrouperSpec* grouperSpec;

	require(serializer->IsOpenForWrite());

	grouperSpec = cast(KWGrouperSpec*, o);
	serializer->PutString(grouperSpec->GetSupervisedMethodName());
	serializer->PutString(grouperSpec->GetUnsupervisedMethodName());
	serializer->PutDouble(grouperSpec->GetParam());
	serializer->PutInt(grouperSpec->GetMinGroupFrequency());
	serializer->PutInt(grouperSpec->GetMaxGroupNumber());
}

void PLShared_GrouperSpec::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWGrouperSpec* grouperSpec;

	require(serializer->IsOpenForRead());

	grouperSpec = cast(KWGrouperSpec*, o);
	grouperSpec->SetSupervisedMethodName(serializer->GetString());
	grouperSpec->SetUnsupervisedMethodName(serializer->GetString());
	grouperSpec->SetParam(serializer->GetDouble());
	grouperSpec->SetMinGroupFrequency(serializer->GetInt());
	grouperSpec->SetMaxGroupNumber(serializer->GetInt());
}

Object* PLShared_GrouperSpec::Create() const
{
	return new KWGrouperSpec;
}
