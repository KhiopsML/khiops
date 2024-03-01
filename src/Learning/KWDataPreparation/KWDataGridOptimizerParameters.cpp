// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizerParameters.h"

KWDataGridOptimizerParameters::KWDataGridOptimizerParameters()
{
	nMaxPartNumber = 0;
	sOptimizationAlgorithm = "VNS";
	nOptimizationTime = 0;
	nOptimizationLevel = 4;
	bUnivariateInitialization = true;
	bPreOptimize = true;
	bOptimize = true;
	bPostOptimize = true;
	// CH IV Begin
	bVarPartPostMerge = true;
	bVarPartPostOptimize = true;
	// CH IV End
	sInternalParameter = "";
	bDisplayDetails = false;
	nFreshness = 0;
}

KWDataGridOptimizerParameters::~KWDataGridOptimizerParameters() {}

int KWDataGridOptimizerParameters::GetMaxPartNumber() const
{
	return nMaxPartNumber;
}

void KWDataGridOptimizerParameters::SetMaxPartNumber(int nValue)
{
	require(nValue >= 0);
	nMaxPartNumber = nValue;
	nFreshness++;
}

const ALString& KWDataGridOptimizerParameters::GetOptimizationAlgorithm() const
{
	return sOptimizationAlgorithm;
}

void KWDataGridOptimizerParameters::SetOptimizationAlgorithm(const ALString& sValue)
{
	require(sValue == "Greedy" or sValue == "MultiStart" or sValue == "VNS" or sValue == "None");
	sOptimizationAlgorithm = sValue;
	nFreshness++;
}

int KWDataGridOptimizerParameters::GetOptimizationTime() const
{
	return nOptimizationTime;
}

void KWDataGridOptimizerParameters::SetOptimizationTime(int nValue)
{
	require(nValue >= 0);
	nOptimizationTime = nValue;
	nFreshness++;
}

int KWDataGridOptimizerParameters::GetOptimizationLevel() const
{
	return nOptimizationLevel;
}

void KWDataGridOptimizerParameters::SetOptimizationLevel(int nValue)
{
	require(nValue >= 0);
	nOptimizationLevel = nValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::GetUnivariateInitialization() const
{
	return bUnivariateInitialization;
}

void KWDataGridOptimizerParameters::SetUnivariateInitialization(boolean bValue)
{
	bUnivariateInitialization = bValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::GetPreOptimize() const
{
	return bPreOptimize;
}

void KWDataGridOptimizerParameters::SetPreOptimize(boolean bValue)
{
	bPreOptimize = bValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::GetOptimize() const
{
	return bOptimize;
}

void KWDataGridOptimizerParameters::SetOptimize(boolean bValue)
{
	bOptimize = bValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::GetPostOptimize() const
{
	return bPostOptimize;
}

void KWDataGridOptimizerParameters::SetPostOptimize(boolean bValue)
{
	bPostOptimize = bValue;
	nFreshness++;
}
// CH IV Begin
boolean KWDataGridOptimizerParameters::GetVarPartPostMerge() const
{
	return bVarPartPostMerge;
}

void KWDataGridOptimizerParameters::SetVarPartPostMerge(boolean bValue)
{
	bVarPartPostMerge = bValue;
}

boolean KWDataGridOptimizerParameters::GetVarPartPostOptimize() const
{
	return bVarPartPostOptimize;
}

void KWDataGridOptimizerParameters::SetVarPartPostOptimize(boolean bValue)
{
	bVarPartPostOptimize = bValue;
}
// CH IV End
const ALString& KWDataGridOptimizerParameters::GetInternalParameter() const
{
	return sInternalParameter;
}

void KWDataGridOptimizerParameters::SetInternalParameter(const ALString& sValue)
{
	sInternalParameter = sValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::GetDisplayDetails() const
{
	return bDisplayDetails;
}

void KWDataGridOptimizerParameters::SetDisplayDetails(boolean bValue)
{
	bDisplayDetails = bValue;
	nFreshness++;
}

boolean KWDataGridOptimizerParameters::Check() const
{
	return true;
}

void KWDataGridOptimizerParameters::CopyFrom(const KWDataGridOptimizerParameters* kwdgopSource)
{
	sOptimizationAlgorithm = kwdgopSource->sOptimizationAlgorithm;
	nMaxPartNumber = kwdgopSource->nMaxPartNumber;
	nOptimizationTime = kwdgopSource->nOptimizationTime;
	nOptimizationLevel = kwdgopSource->nOptimizationLevel;
	bUnivariateInitialization = kwdgopSource->bUnivariateInitialization;
	bPreOptimize = kwdgopSource->bPreOptimize;
	bOptimize = kwdgopSource->bOptimize;
	bPostOptimize = kwdgopSource->bPostOptimize;
	// CH IV Begin
	bVarPartPostMerge = kwdgopSource->bVarPartPostMerge;
	bVarPartPostOptimize = kwdgopSource->bVarPartPostOptimize;
	// CH IV End
	sInternalParameter = kwdgopSource->sInternalParameter;
	bDisplayDetails = kwdgopSource->bDisplayDetails;
}

KWDataGridOptimizerParameters* KWDataGridOptimizerParameters::Clone() const
{
	KWDataGridOptimizerParameters* aClone;

	aClone = new KWDataGridOptimizerParameters;
	aClone->CopyFrom(this);

	return aClone;
}

int KWDataGridOptimizerParameters::GetFreshness() const
{
	return nFreshness;
}

void KWDataGridOptimizerParameters::Write(ostream& ost) const
{
	ost << GetClassLabel() << "(";
	ost << sOptimizationAlgorithm << ", ";
	ost << nMaxPartNumber << ", ";
	ost << nOptimizationTime << ", ";
	ost << nOptimizationLevel << ", ";
	ost << bUnivariateInitialization << ", ";
	ost << bPreOptimize << ", ";
	ost << bOptimize << ", ";
	ost << bPostOptimize << ", ";
	ost << sInternalParameter << ", ";
	ost << bDisplayDetails << ")";
}

const ALString KWDataGridOptimizerParameters::GetClassLabel() const
{
	return "Data Grid optimization";
}

const ALString KWDataGridOptimizerParameters::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = sLabel + sOptimizationAlgorithm + "(" + IntToString(GetOptimizationLevel()) + ")";
	if (GetPreOptimize())
		sLabel += "+PreOpt";
	if (GetOptimize())
		sLabel += "+Opt";
	if (GetPostOptimize())
		sLabel += "+PostOpt";
	// CH IV Begin
	if (GetVarPartPostMerge())
		sLabel += "+VarPartPostMerge";
	if (GetVarPartPostOptimize())
		sLabel += "+VarPartPostOpt";
	// CH IV End
	if (GetInternalParameter() != "")
		sLabel += "(" + sInternalParameter + ")";
	return sLabel;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DataGridOptimizerParameters

PLShared_DataGridOptimizerParameters::PLShared_DataGridOptimizerParameters() {}

PLShared_DataGridOptimizerParameters::~PLShared_DataGridOptimizerParameters() {}

void PLShared_DataGridOptimizerParameters::SetDataGridOptimizerParameters(
    KWDataGridOptimizerParameters* dataGridOptimizerParameters)
{
	require(dataGridOptimizerParameters != NULL);
	SetObject(dataGridOptimizerParameters);
}

KWDataGridOptimizerParameters* PLShared_DataGridOptimizerParameters::GetDataGridOptimizerParameters()
{
	return cast(KWDataGridOptimizerParameters*, GetObject());
}

void PLShared_DataGridOptimizerParameters::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDataGridOptimizerParameters* dataGridOptimizerParameters;

	require(serializer->IsOpenForWrite());

	dataGridOptimizerParameters = cast(KWDataGridOptimizerParameters*, o);
	serializer->PutInt(dataGridOptimizerParameters->GetMaxPartNumber());
	serializer->PutString(dataGridOptimizerParameters->GetOptimizationAlgorithm());
	serializer->PutInt(dataGridOptimizerParameters->GetOptimizationTime());
	serializer->PutInt(dataGridOptimizerParameters->GetOptimizationLevel());
	serializer->PutBoolean(dataGridOptimizerParameters->GetUnivariateInitialization());
	serializer->PutBoolean(dataGridOptimizerParameters->GetPreOptimize());
	serializer->PutBoolean(dataGridOptimizerParameters->GetOptimize());
	serializer->PutBoolean(dataGridOptimizerParameters->GetPostOptimize());
	// CH IV Begin
	serializer->PutBoolean(dataGridOptimizerParameters->GetVarPartPostMerge());
	serializer->PutBoolean(dataGridOptimizerParameters->GetVarPartPostOptimize());
	// CH IV End
	serializer->PutBoolean(dataGridOptimizerParameters->GetDisplayDetails());
}

void PLShared_DataGridOptimizerParameters::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDataGridOptimizerParameters* dataGridOptimizerParameters;

	require(serializer->IsOpenForRead());

	dataGridOptimizerParameters = cast(KWDataGridOptimizerParameters*, o);
	dataGridOptimizerParameters->SetMaxPartNumber(serializer->GetInt());
	dataGridOptimizerParameters->SetOptimizationAlgorithm(serializer->GetString());
	dataGridOptimizerParameters->SetOptimizationTime(serializer->GetInt());
	dataGridOptimizerParameters->SetOptimizationLevel(serializer->GetInt());
	dataGridOptimizerParameters->SetUnivariateInitialization(serializer->GetBoolean());
	dataGridOptimizerParameters->SetPreOptimize(serializer->GetBoolean());
	dataGridOptimizerParameters->SetOptimize(serializer->GetBoolean());
	dataGridOptimizerParameters->SetPostOptimize(serializer->GetBoolean());
	// CH IV Begin
	dataGridOptimizerParameters->SetVarPartPostMerge(serializer->GetBoolean());
	dataGridOptimizerParameters->SetVarPartPostOptimize(serializer->GetBoolean());
	// CH IV End
	dataGridOptimizerParameters->SetDisplayDetails(serializer->GetBoolean());
}

Object* PLShared_DataGridOptimizerParameters::Create() const
{
	return new KWDataGridOptimizerParameters;
}
