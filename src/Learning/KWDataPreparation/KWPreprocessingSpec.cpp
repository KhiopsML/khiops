// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPreprocessingSpec.h"

//////////////////////////////////////////////////////////////////////////
// Classe KWPreprocessingSpec

KWPreprocessingSpec::KWPreprocessingSpec()
{
	bTargetGrouped = false;
}

KWPreprocessingSpec::~KWPreprocessingSpec() {}

boolean KWPreprocessingSpec::GetTargetGrouped() const
{
	return bTargetGrouped;
}

void KWPreprocessingSpec::SetTargetGrouped(boolean bValue)
{
	bTargetGrouped = bValue;
}

KWDiscretizerSpec* KWPreprocessingSpec::GetDiscretizerSpec()
{
	return &discretizerSpec;
}

KWGrouperSpec* KWPreprocessingSpec::GetGrouperSpec()
{
	return &grouperSpec;
}

KWDataGridOptimizerParameters* KWPreprocessingSpec::GetDataGridOptimizerParameters()
{
	return &dataGridOptimizerParameters;
}

void KWPreprocessingSpec::WriteHeaderLineReport(ostream& ost)
{
	ost << "Grouped target\t";
	ost << "Discretization\tMin freq.\tMax nb\tParam. D.\t";
	ost << "Value grouping\tMin freq.\tMax nb\tParam. G.\t";
	ost << "DG algorithm\tLevel\tUnivariate initialization\tPre-optimize\tOptimize\tPost-optimize\t";
}

void KWPreprocessingSpec::WriteLineReport(int nTargetAttributeType, ostream& ost)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::Continuous or
		nTargetAttributeType == KWType::None);

	ost << BooleanToString(bTargetGrouped) << "\t";
	ost << discretizerSpec.GetMethodName(nTargetAttributeType) << "\t" << discretizerSpec.GetMinIntervalFrequency()
	    << "\t" << discretizerSpec.GetMaxIntervalNumber() << "\t" << discretizerSpec.GetParam() << "\t";
	ost << grouperSpec.GetMethodName(nTargetAttributeType) << "\t" << grouperSpec.GetMinGroupFrequency() << "\t"
	    << grouperSpec.GetMaxGroupNumber() << "\t" << grouperSpec.GetParam() << "\t";
	ost << dataGridOptimizerParameters.GetOptimizationAlgorithm() << "\t"
	    << dataGridOptimizerParameters.GetOptimizationLevel() << "\t"
	    << dataGridOptimizerParameters.GetUnivariateInitialization() << "\t"
	    << dataGridOptimizerParameters.GetPreOptimize() << "\t" << dataGridOptimizerParameters.GetOptimize() << "\t"
	    << dataGridOptimizerParameters.GetPostOptimize() << "\t";
}

boolean KWPreprocessingSpec::Check() const
{
	boolean bOk = true;

	// On force la verification des deux algorithmes pour avoir le
	// maximum de messages d'erreurs
	bOk = discretizerSpec.Check() and bOk;
	bOk = grouperSpec.Check() and bOk;
	bOk = dataGridOptimizerParameters.Check() and bOk;
	return bOk;
}

boolean KWPreprocessingSpec::CheckForTargetType(int nTargetAttributeType) const
{
	boolean bOk = true;

	// On force la verification des deux algorithmes pour avoir le
	// maximum de messages d'erreurs
	bOk = discretizerSpec.CheckForTargetType(nTargetAttributeType) and bOk;
	bOk = grouperSpec.CheckForTargetType(nTargetAttributeType) and bOk;
	bOk = dataGridOptimizerParameters.Check() and bOk;
	return bOk;
}

void KWPreprocessingSpec::CopyFrom(const KWPreprocessingSpec* kwpsSource)
{
	require(kwpsSource != NULL);
	bTargetGrouped = kwpsSource->bTargetGrouped;
	discretizerSpec.CopyFrom(&(kwpsSource->discretizerSpec));
	grouperSpec.CopyFrom(&(kwpsSource->grouperSpec));
	dataGridOptimizerParameters.CopyFrom(&(kwpsSource->dataGridOptimizerParameters));
}

KWPreprocessingSpec* KWPreprocessingSpec::Clone() const
{
	KWPreprocessingSpec* kwpsClone;

	kwpsClone = new KWPreprocessingSpec;
	kwpsClone->CopyFrom(this);
	return kwpsClone;
}

int KWPreprocessingSpec::GetFreshness() const
{
	return discretizerSpec.GetFreshness() + grouperSpec.GetFreshness() + dataGridOptimizerParameters.GetFreshness();
}

const ALString KWPreprocessingSpec::GetClassLabel() const
{
	return "Preprocessing method";
}

const ALString KWPreprocessingSpec::GetObjectLabel() const
{
	ALString sLabel;
	sLabel = discretizerSpec.GetObjectLabel() + " " + grouperSpec.GetObjectLabel();
	sLabel.TrimLeft();
	sLabel.TrimRight();
	return sLabel;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_PreprocessingSpec

PLShared_PreprocessingSpec::PLShared_PreprocessingSpec() {}

PLShared_PreprocessingSpec::~PLShared_PreprocessingSpec() {}

void PLShared_PreprocessingSpec::SetPreprocessingSpec(KWPreprocessingSpec* preprocessingSpec)
{
	require(preprocessingSpec != NULL);
	SetObject(preprocessingSpec);
}

KWPreprocessingSpec* PLShared_PreprocessingSpec::GetPreprocessingSpec()
{
	return cast(KWPreprocessingSpec*, GetObject());
}

void PLShared_PreprocessingSpec::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWPreprocessingSpec* preprocessingSpec;
	PLShared_DiscretizerSpec sharedDiscretizerSpec;
	PLShared_GrouperSpec sharedGrouperSpec;
	PLShared_DataGridOptimizerParameters sharedDataGridOptimizerParameters;

	require(serializer->IsOpenForWrite());

	preprocessingSpec = cast(KWPreprocessingSpec*, o);
	serializer->PutBoolean(preprocessingSpec->GetTargetGrouped());
	sharedDiscretizerSpec.SerializeObject(serializer, preprocessingSpec->GetDiscretizerSpec());
	sharedGrouperSpec.SerializeObject(serializer, preprocessingSpec->GetGrouperSpec());
	sharedDataGridOptimizerParameters.SerializeObject(serializer,
							  preprocessingSpec->GetDataGridOptimizerParameters());
}

void PLShared_PreprocessingSpec::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWPreprocessingSpec* preprocessingSpec;
	PLShared_DiscretizerSpec sharedDiscretizerSpec;
	PLShared_GrouperSpec sharedGrouperSpec;
	PLShared_DataGridOptimizerParameters sharedDataGridOptimizerParameters;

	require(serializer->IsOpenForRead());

	preprocessingSpec = cast(KWPreprocessingSpec*, o);
	preprocessingSpec->SetTargetGrouped(serializer->GetBoolean());
	sharedDiscretizerSpec.DeserializeObject(serializer, preprocessingSpec->GetDiscretizerSpec());
	sharedGrouperSpec.DeserializeObject(serializer, preprocessingSpec->GetGrouperSpec());
	sharedDataGridOptimizerParameters.DeserializeObject(serializer,
							    preprocessingSpec->GetDataGridOptimizerParameters());
}

Object* PLShared_PreprocessingSpec::Create() const
{
	return new KWPreprocessingSpec;
}