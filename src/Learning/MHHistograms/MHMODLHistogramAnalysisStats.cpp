// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHMODLHistogramAnalysisStats.h"

///////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramAnalysisStats

MHMODLHistogramAnalysisStats::MHMODLHistogramAnalysisStats()
{
	nInterpretableHistogramNumber = 0;
	nCentralBinExponent = 0;
	nLastCentralBinExponent = 0;
	nRemovedSingularIntervalsNumber = 0;
	dTruncationEpsilon = 0;
}

MHMODLHistogramAnalysisStats::~MHMODLHistogramAnalysisStats()
{
	oaHistograms.DeleteAll();
}

const ALString MHMODLHistogramAnalysisStats::GetDiscretizerName() const
{
	return "MODL";
}

ObjectArray* MHMODLHistogramAnalysisStats::GetHistograms()
{
	return &oaHistograms;
}

int MHMODLHistogramAnalysisStats::GetHistogramNumber() const
{
	return oaHistograms.GetSize();
}

const MHMODLHistogramStats* MHMODLHistogramAnalysisStats::GetHistogramAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetHistogramNumber());
	return cast(const MHMODLHistogramStats*, oaHistograms.GetAt(nIndex));
}

void MHMODLHistogramAnalysisStats::SetInterpretableHistogramNumber(int nValue)
{
	require(nValue >= 0);
	nInterpretableHistogramNumber = nValue;
}

int MHMODLHistogramAnalysisStats::GetInterpretableHistogramNumber() const
{
	return nInterpretableHistogramNumber;
}

const MHMODLHistogramStats* MHMODLHistogramAnalysisStats::GetMostAccurateInterpretableHistogram() const
{
	const MHMODLHistogramStats* histogram;

	require(oaHistograms.GetSize() != 0);
	require(oaHistograms.GetSize() - 1 <= nInterpretableHistogramNumber and
		nInterpretableHistogramNumber <= oaHistograms.GetSize());

	// Recherche de l'histogramme interpretable le plus fin
	histogram = cast(const MHMODLHistogramStats*, oaHistograms.GetAt(nInterpretableHistogramNumber - 1));
	return histogram;
}

Continuous MHMODLHistogramAnalysisStats::GetDomainLowerBound() const
{
	require(oaHistograms.GetSize() != 0);
	return GetMostAccurateInterpretableHistogram()->GetDomainLowerBound();
}

Continuous MHMODLHistogramAnalysisStats::GetDomainUpperBound() const
{
	require(oaHistograms.GetSize() != 0);
	return GetMostAccurateInterpretableHistogram()->GetDomainUpperBound();
}

void MHMODLHistogramAnalysisStats::SetCentralBinExponent(int nValue)
{
	nCentralBinExponent = nValue;
}

int MHMODLHistogramAnalysisStats::GetCentralBinExponent() const
{
	return nCentralBinExponent;
}

void MHMODLHistogramAnalysisStats::SetLastCentralBinExponent(int nValue)
{
	nLastCentralBinExponent = nValue;
}

int MHMODLHistogramAnalysisStats::GetLastCentralBinExponent() const
{
	return nLastCentralBinExponent;
}

void MHMODLHistogramAnalysisStats::SetRemovedSingularIntervalsNumber(int nValue)
{
	require(nValue >= 0);
	nRemovedSingularIntervalsNumber = nValue;
}

int MHMODLHistogramAnalysisStats::GetRemovedSingularIntervalsNumber() const
{
	return nRemovedSingularIntervalsNumber;
}

void MHMODLHistogramAnalysisStats::SetTruncationEpsilon(double dValue)
{
	require(dValue >= 0);
	dTruncationEpsilon = dValue;
}

double MHMODLHistogramAnalysisStats::GetTruncationEpsilon() const
{
	return dTruncationEpsilon;
}

void MHMODLHistogramAnalysisStats::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey)
{
	int i;
	const MHMODLHistogramStats* histogram;
	const MHMODLHistogramStats* mostAccurateInterpretableHistogram;

	require(fJSON != NULL);
	require(sKey != "");
	require(Check());

	// Recherche de l'histogramme interpretable le plus precis
	mostAccurateInterpretableHistogram = GetMostAccurateInterpretableHistogram();

	// Debut de l'objet
	fJSON->BeginKeyObject(sKey);

	// Nombre d'histogrammes
	fJSON->WriteKeyInt("histogramNumber", GetHistogramNumber());

	// Nombre d'histogramms interpretables
	fJSON->WriteKeyInt("interpretableHistogramNumber", GetInterpretableHistogramNumber());

	// Epsilon de troncature
	fJSON->WriteKeyContinuous("truncationEpsilon", GetTruncationEpsilon());

	// Nombre d'intervalle de singularites supprimes
	fJSON->WriteKeyInt("removedSingularIntervalNumber", GetRemovedSingularIntervalsNumber());

	// Vecteur des granularites
	fJSON->BeginKeyList("granularities");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteInt(histogram->GetGranularity());
	}
	fJSON->EndList();

	// Vecteur des nombres d'intervalles
	fJSON->BeginKeyList("intervalNumbers");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteInt(histogram->GetIntervalNumber());
	}
	fJSON->EndList();

	// Vecteur des nombres d'intervalles de type peak
	fJSON->BeginKeyList("peakIntervalNumbers");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteInt(histogram->GetPeakIntervalNumber());
	}
	fJSON->EndList();

	// Vecteur des nombres d'intervalles de type spike
	fJSON->BeginKeyList("spikeIntervalNumbers");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteInt(histogram->GetSpikeIntervalNumber());
	}
	fJSON->EndList();

	// Vecteur des nombres d'intervalles vides
	fJSON->BeginKeyList("emptyIntervalNumbers");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteInt(histogram->GetEmptyIntervalNumber());
	}
	fJSON->EndList();

	// Vecteur des level
	fJSON->BeginKeyList("levels");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		fJSON->WriteContinuous(histogram->GetNormalizedLevel());
	}
	fJSON->EndList();

	// Vecteur des taux d'information
	fJSON->BeginKeyList("informationRates");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		if (mostAccurateInterpretableHistogram->GetNormalizedLevel() != 0)
			fJSON->WriteContinuous(histogram->GetNormalizedLevel() * 100 /
					       mostAccurateInterpretableHistogram->GetNormalizedLevel());
		else
			fJSON->WriteContinuous(0);
	}
	fJSON->EndList();

	// Tableau des histogrammes
	fJSON->BeginKeyArray("histograms");
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		histogram->WriteJSONIntervalReport(fJSON);
	}
	fJSON->EndArray();

	// Fin de l'objet
	fJSON->EndObject();
}

boolean MHMODLHistogramAnalysisStats::Check() const
{
	boolean bOk = true;
	int i;
	const MHMODLHistogramStats* histogram;
	int nTotalFrequency;

	// Verifications de base
	bOk = bOk and GetHistogramNumber() >= 1;
	bOk = bOk and GetInterpretableHistogramNumber() <= GetHistogramNumber();
	bOk = bOk and GetInterpretableHistogramNumber() >= GetHistogramNumber() - 1;
	assert(bOk);

	// Tableau des histogrammes
	if (bOk)
	{
		nTotalFrequency = GetHistogramAt(0)->ComputeTotalFrequency();
		for (i = 0; i < GetHistogramNumber(); i++)
		{
			histogram = GetHistogramAt(i);
			bOk = bOk and histogram->Check();
			assert(bOk);
			if (i > 0)
			{
				bOk = bOk and GetHistogramAt(i - 1)->GetGranularity() <= histogram->GetGranularity();
				bOk = bOk and GetHistogramAt(i - 1)->GetGranularity() < histogram->GetGranularity() or
				      i == GetHistogramNumber() - 1;
				bOk = bOk and GetHistogramAt(i - 1)->GetNormalizedLevel() <
						  histogram->GetNormalizedLevel() or
				      i == GetHistogramNumber() - 1;
				bOk = bOk and histogram->ComputeTotalFrequency() == nTotalFrequency;
				assert(bOk);
			}
		}
	}
	return bOk;
}

longint MHMODLHistogramAnalysisStats::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;
	const MHMODLHistogramStats* histogram;

	lUsedMemory = sizeof(MHMODLHistogramAnalysisStats);
	lUsedMemory += oaHistograms.GetUsedMemory() - sizeof(ObjectArray);
	for (i = 0; i < GetHistogramNumber(); i++)
	{
		histogram = GetHistogramAt(i);
		lUsedMemory += histogram->GetUsedMemory();
	}
	return lUsedMemory;
}

const ALString MHMODLHistogramAnalysisStats::GetClassLabel() const
{
	return "MODL Histogram analysis stats";
}

const ALString MHMODLHistogramAnalysisStats::GetObjectLabel() const
{
	return "";
}

///////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramStats

MHMODLHistogramStats::MHMODLHistogramStats()
{
	nGranularity = 0;
	nPeakIntervalNumber = 0;
	nSpikeIntervalNumber = 0;
	nEmptyIntervalNumber = 0;
	dNormalizedLevel = 0;
}

MHMODLHistogramStats::~MHMODLHistogramStats() {}

void MHMODLHistogramStats::SetIntervalNumber(int nValue)
{
	require(nValue >= 0);

	if (nValue == 0)
		cvIntervalBounds.SetSize(0);
	else
		cvIntervalBounds.SetSize(nValue + 1);
	ivIntervalFrequencies.SetSize(nValue);
}

int MHMODLHistogramStats::GetIntervalNumber() const
{
	return ivIntervalFrequencies.GetSize();
}

ContinuousVector* MHMODLHistogramStats::GetIntervalBounds()
{
	return &cvIntervalBounds;
}

IntVector* MHMODLHistogramStats::GetIntervalFrequencies()
{
	return &ivIntervalFrequencies;
}

Continuous MHMODLHistogramStats::GetIntervalLowerBoundAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetIntervalNumber());
	return cvIntervalBounds.GetAt(nIndex);
}

Continuous MHMODLHistogramStats::GetIntervalUpperBoundAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetIntervalNumber());
	return cvIntervalBounds.GetAt(nIndex + 1);
}

int MHMODLHistogramStats::GetIntervalFrequencyAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetIntervalNumber());
	return ivIntervalFrequencies.GetAt(nIndex);
}

Continuous MHMODLHistogramStats::GetDomainLowerBound() const
{
	require(GetIntervalNumber() >= 1);
	return cvIntervalBounds.GetAt(0);
}

Continuous MHMODLHistogramStats::GetDomainUpperBound() const
{
	require(GetIntervalNumber() >= 1);
	return cvIntervalBounds.GetAt(GetIntervalNumber());
}

int MHMODLHistogramStats::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int i;

	nTotalFrequency = 0;
	for (i = 0; i < ivIntervalFrequencies.GetSize(); i++)
	{
		nTotalFrequency += ivIntervalFrequencies.GetAt(i);
		assert(nTotalFrequency >= 0);
	}
	return nTotalFrequency;
}

void MHMODLHistogramStats::SetGranularity(int nValue)
{
	require(nValue >= 0);
	nGranularity = nValue;
}

int MHMODLHistogramStats::GetGranularity() const
{
	return nGranularity;
}

void MHMODLHistogramStats::SetPeakIntervalNumber(int nValue)
{
	require(nValue >= 0);
	nPeakIntervalNumber = nValue;
}

int MHMODLHistogramStats::GetPeakIntervalNumber() const
{
	return nPeakIntervalNumber;
}

void MHMODLHistogramStats::SetSpikeIntervalNumber(int nValue)
{
	require(nValue >= 0);
	nSpikeIntervalNumber = nValue;
}

int MHMODLHistogramStats::GetSpikeIntervalNumber() const
{
	return nSpikeIntervalNumber;
}

void MHMODLHistogramStats::SetEmptyIntervalNumber(int nValue)
{
	require(nValue >= 0);
	nEmptyIntervalNumber = nValue;
}

int MHMODLHistogramStats::GetEmptyIntervalNumber() const
{
	return nEmptyIntervalNumber;
}

void MHMODLHistogramStats::SetNormalizedLevel(double dValue)
{
	require(dValue >= 0);
	dNormalizedLevel = dValue;
}

double MHMODLHistogramStats::GetNormalizedLevel() const
{
	return dNormalizedLevel;
}

void MHMODLHistogramStats::WriteJSONIntervalReport(JSONFile* fJSON) const
{
	int i;

	require(fJSON != NULL);
	require(Check());

	// Debut de l'objet
	fJSON->BeginObject();

	// Ecriture des bornes
	fJSON->BeginKeyList("bounds");
	for (i = 0; i < cvIntervalBounds.GetSize(); i++)
		fJSON->WriteContinuous(cvIntervalBounds.GetAt(i));
	fJSON->EndList();

	// Ecriture des effectifs
	fJSON->BeginKeyList("frequencies");
	for (i = 0; i < ivIntervalFrequencies.GetSize(); i++)
		fJSON->WriteContinuous(ivIntervalFrequencies.GetAt(i));
	fJSON->EndList();

	// Fin de l'objet
	fJSON->EndObject();
}

boolean MHMODLHistogramStats::Check() const
{
	boolean bOk = true;
	int i;

	bOk = bOk and GetIntervalNumber() > 0;
	bOk = bOk and cvIntervalBounds.GetSize() == ivIntervalFrequencies.GetSize() + 1;
	for (i = 0; i < GetIntervalNumber(); i++)
	{
		bOk = bOk and cvIntervalBounds.GetAt(i) < cvIntervalBounds.GetAt(i + 1);
		bOk = bOk and ivIntervalFrequencies.GetAt(i) >= 0;
	}
	bOk = bOk and ivIntervalFrequencies.GetAt(0) > 0;
	bOk = bOk and ivIntervalFrequencies.GetAt(GetIntervalNumber() - 1) > 0;
	return bOk;
}

longint MHMODLHistogramStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(MHMODLHistogramStats);
	lUsedMemory += ivIntervalFrequencies.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += cvIntervalBounds.GetUsedMemory() - sizeof(ContinuousVector);
	return lUsedMemory;
}

const ALString MHMODLHistogramStats::GetClassLabel() const
{
	return "MODL Histogram stats";
}

const ALString MHMODLHistogramStats::GetObjectLabel() const
{
	return "";
}

////////////////////////////////////////////////////////////
// Classe PLShared_MODLHistogramAnalysisStats

PLShared_MODLHistogramAnalysisStats::PLShared_MODLHistogramAnalysisStats() {}

PLShared_MODLHistogramAnalysisStats::~PLShared_MODLHistogramAnalysisStats() {}

void PLShared_MODLHistogramAnalysisStats::SetMODLHistogramAnalysisStats(
    MHMODLHistogramAnalysisStats* histogramAnalysisStats)
{
	require(histogramAnalysisStats != NULL);
	SetObject(histogramAnalysisStats);
}

MHMODLHistogramAnalysisStats* PLShared_MODLHistogramAnalysisStats::GetMODLHistogramAnalysisStats()
{
	return cast(MHMODLHistogramAnalysisStats*, GetObject());
}

void PLShared_MODLHistogramAnalysisStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	MHMODLHistogramAnalysisStats* histogramAnalysisStats;
	PLShared_ObjectArray shared_oaHistogramStats(new PLShared_MODLHistogramStats);

	require(serializer->IsOpenForWrite());
	require(o != NULL);
	require(cast(MHMODLHistogramAnalysisStats*, o)->Check());

	// Serialisation des indicateurs
	histogramAnalysisStats = cast(MHMODLHistogramAnalysisStats*, o);
	serializer->PutInt(histogramAnalysisStats->GetInterpretableHistogramNumber());
	serializer->PutInt(histogramAnalysisStats->GetCentralBinExponent());
	serializer->PutInt(histogramAnalysisStats->GetLastCentralBinExponent());
	serializer->PutInt(histogramAnalysisStats->GetRemovedSingularIntervalsNumber());
	serializer->PutDouble(histogramAnalysisStats->GetTruncationEpsilon());

	// Serialisation des histogrammes
	shared_oaHistogramStats.SerializeObject(serializer, histogramAnalysisStats->GetHistograms());
}

void PLShared_MODLHistogramAnalysisStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	MHMODLHistogramAnalysisStats* histogramAnalysisStats;
	PLShared_ObjectArray shared_oaHistogramStats(new PLShared_MODLHistogramStats);

	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Serialisation des indicateurs
	histogramAnalysisStats = cast(MHMODLHistogramAnalysisStats*, o);
	histogramAnalysisStats->SetInterpretableHistogramNumber(serializer->GetInt());
	histogramAnalysisStats->SetCentralBinExponent(serializer->GetInt());
	histogramAnalysisStats->SetLastCentralBinExponent(serializer->GetInt());
	histogramAnalysisStats->SetRemovedSingularIntervalsNumber(serializer->GetInt());
	histogramAnalysisStats->SetTruncationEpsilon(serializer->GetDouble());

	// Serialisation des histogrammes
	shared_oaHistogramStats.DeserializeObject(serializer, histogramAnalysisStats->GetHistograms());
	ensure(histogramAnalysisStats->Check());
}

Object* PLShared_MODLHistogramAnalysisStats::Create() const
{
	return new MHMODLHistogramAnalysisStats;
}

////////////////////////////////////////////////////////////
// Classe PLShared_MODLHistogramAnalysisStats

PLShared_MODLHistogramStats::PLShared_MODLHistogramStats() {}

PLShared_MODLHistogramStats::~PLShared_MODLHistogramStats() {}

void PLShared_MODLHistogramStats::SetMODLHistogramStats(MHMODLHistogramStats* histogramStats)
{
	require(histogramStats != NULL);
	SetObject(histogramStats);
}

MHMODLHistogramStats* PLShared_MODLHistogramStats::GetMODLHistogramStats()
{
	return cast(MHMODLHistogramStats*, GetObject());
}

void PLShared_MODLHistogramStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	MHMODLHistogramStats* histogramStats;
	PLShared_ContinuousVector sharedContinuousVector;

	require(serializer->IsOpenForWrite());
	require(o != NULL);
	require(cast(MHMODLHistogramStats*, o)->Check());

	// Serialisation des indicateurs
	histogramStats = cast(MHMODLHistogramStats*, o);
	serializer->PutInt(histogramStats->GetGranularity());
	serializer->PutInt(histogramStats->GetPeakIntervalNumber());
	serializer->PutInt(histogramStats->GetSpikeIntervalNumber());
	serializer->PutInt(histogramStats->GetEmptyIntervalNumber());
	serializer->PutDouble(histogramStats->GetNormalizedLevel());

	// Serialisation des intervalles
	sharedContinuousVector.SerializeObject(serializer, histogramStats->GetIntervalBounds());
	serializer->PutIntVector(histogramStats->GetIntervalFrequencies());
}

void PLShared_MODLHistogramStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	MHMODLHistogramStats* histogramStats;
	PLShared_ContinuousVector sharedContinuousVector;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Serialisation des indicateurs
	histogramStats = cast(MHMODLHistogramStats*, o);
	histogramStats->SetGranularity(serializer->GetInt());
	histogramStats->SetPeakIntervalNumber(serializer->GetInt());
	histogramStats->SetSpikeIntervalNumber(serializer->GetInt());
	histogramStats->SetEmptyIntervalNumber(serializer->GetInt());
	histogramStats->SetNormalizedLevel(serializer->GetDouble());

	// Serialisation des intervalles
	sharedContinuousVector.DeserializeObject(serializer, histogramStats->GetIntervalBounds());
	serializer->GetIntVector(histogramStats->GetIntervalFrequencies());
	ensure(histogramStats->Check());
}

Object* PLShared_MODLHistogramStats::Create() const
{
	return new MHMODLHistogramStats;
}
