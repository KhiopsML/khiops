// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPreprocessingSpec.h"

//////////////////////////////////////////////////////////////////////////
// Classe KWPreprocessingSpec

KWPreprocessingSpec::KWPreprocessingSpec()
{
	bTargetGrouped = false;
	nMaxPartNumber = 0;
	nMinPartFrequency = 0;
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

int KWPreprocessingSpec::GetMaxPartNumber() const
{
	require(discretizerSpec.GetMaxIntervalNumber() == nMaxPartNumber);
	require(grouperSpec.GetMaxGroupNumber() == nMaxPartNumber);
	require(dataGridOptimizerParameters.GetMaxPartNumber() == nMaxPartNumber);

	return nMaxPartNumber;
}

void KWPreprocessingSpec::SetMaxPartNumber(int nValue)
{
	require(nValue >= 0);

	nMaxPartNumber = nValue;

	// Synchronisation avec les parametres correspondant des methodes de pretraitement
	discretizerSpec.SetMaxIntervalNumber(nMaxPartNumber);
	grouperSpec.SetMaxGroupNumber(nMaxPartNumber);
	dataGridOptimizerParameters.SetMaxPartNumber(nMaxPartNumber);
}

int KWPreprocessingSpec::GetMinPartFrequency() const
{
	require(discretizerSpec.GetMinIntervalFrequency() == nMinPartFrequency);
	require(grouperSpec.GetMinGroupFrequency() == nMinPartFrequency);

	return nMinPartFrequency;
}

void KWPreprocessingSpec::SetMinPartFrequency(int nValue)
{
	require(nValue >= 0);

	nMinPartFrequency = nValue;

	// Synchronisation avec les parametres correspondant des methodes de pretraitement
	discretizerSpec.SetMinIntervalFrequency(nMinPartFrequency);
	grouperSpec.SetMinGroupFrequency(nMinPartFrequency);
}

const ALString& KWPreprocessingSpec::GetDiscretizerUnsupervisedMethodName() const
{
	return discretizerSpec.GetUnsupervisedMethodName();
}

void KWPreprocessingSpec::SetDiscretizerUnsupervisedMethodName(const ALString& sValue)
{
	discretizerSpec.SetUnsupervisedMethodName(sValue);
}

const ALString& KWPreprocessingSpec::GetGrouperUnsupervisedMethodName() const
{
	return grouperSpec.GetUnsupervisedMethodName();
}

void KWPreprocessingSpec::SetGrouperUnsupervisedMethodName(const ALString& sValue)
{
	grouperSpec.SetUnsupervisedMethodName(sValue);
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
	ost << "Max parts\t";
	ost << "Min freq\t";
	ost << "Discretization\tParam. D.\t";
	ost << "Value grouping\tParam. G.\t";
	ost << "DG algorithm\tLevel\tUnivariate initialization\tPre-optimize\tOptimize\tPost-optimize\t";
}

void KWPreprocessingSpec::WriteLineReport(int nTargetAttributeType, ostream& ost)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::Continuous or
		nTargetAttributeType == KWType::None);

	ost << BooleanToString(bTargetGrouped) << "\t";
	ost << nMaxPartNumber << "\t";
	ost << nMinPartFrequency << "\t";
	ost << discretizerSpec.GetMethodName(nTargetAttributeType) << "\t" << discretizerSpec.GetParam() << "\t";
	ost << grouperSpec.GetMethodName(nTargetAttributeType) << "\t" << grouperSpec.GetParam() << "\t";
	ost << "VNS\t" << dataGridOptimizerParameters.GetOptimizationLevel() << "\t"
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

	// On verifie que le nombre maximum de partie est le meme que pour chaque algorithme
	bOk = bOk and nMaxPartNumber == discretizerSpec.GetMaxIntervalNumber();
	bOk = bOk and nMaxPartNumber == grouperSpec.GetMaxGroupNumber();
	bOk = bOk and nMaxPartNumber == dataGridOptimizerParameters.GetMaxPartNumber();

	// On verifie que l'effectif minimum par partie est le meme que pour chaque algorithme
	bOk = bOk and nMinPartFrequency == discretizerSpec.GetMinIntervalFrequency();
	bOk = bOk and nMinPartFrequency == grouperSpec.GetMinGroupFrequency();
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
	nMaxPartNumber = kwpsSource->nMaxPartNumber;
	nMinPartFrequency = kwpsSource->nMinPartFrequency;
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

void KWPreprocessingSpec::Write(ostream& ost) const
{
	ost << GetClassLabel() << "(";
	ost << bTargetGrouped << ", ";
	ost << nMaxPartNumber << ", ";
	ost << nMinPartFrequency << ")\n";
	ost << "\t" << discretizerSpec << "\n";
	ost << "\t" << grouperSpec << "\n";
	ost << "\t" << dataGridOptimizerParameters << "\n";
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
	serializer->PutInt(preprocessingSpec->GetMaxPartNumber());
	serializer->PutInt(preprocessingSpec->GetMinPartFrequency());
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
	preprocessingSpec->SetMaxPartNumber(serializer->GetInt());
	preprocessingSpec->SetMinPartFrequency(serializer->GetInt());
	sharedDiscretizerSpec.DeserializeObject(serializer, preprocessingSpec->GetDiscretizerSpec());
	sharedGrouperSpec.DeserializeObject(serializer, preprocessingSpec->GetGrouperSpec());
	sharedDataGridOptimizerParameters.DeserializeObject(serializer,
							    preprocessingSpec->GetDataGridOptimizerParameters());
}

Object* PLShared_PreprocessingSpec::Create() const
{
	return new KWPreprocessingSpec;
}
