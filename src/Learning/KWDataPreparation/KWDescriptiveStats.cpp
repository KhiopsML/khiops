// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDescriptiveStats.h"

///////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveStats

KWDescriptiveStats::KWDescriptiveStats()
{
	bIsStatsComputed = false;
	nValueNumber = 0;
}

KWDescriptiveStats::~KWDescriptiveStats() {}

void KWDescriptiveStats::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
}

const ALString& KWDescriptiveStats::GetAttributeName() const
{
	return sAttributeName;
}

int KWDescriptiveStats::GetValueNumber() const
{
	require(IsStatsComputed());
	return nValueNumber;
}

boolean KWDescriptiveStats::ComputeStats()
{
	// Ne doit pas etre appele pour cette classe et ses sous-classes
	assert(false);
	return bIsStatsComputed;
}

void KWDescriptiveStats::Init()
{
	bIsStatsComputed = false;
	nValueNumber = 0;
}

const ALString KWDescriptiveStats::GetSortName() const
{
	return GetAttributeName();
}

double KWDescriptiveStats::GetSortValue() const
{
	require(IsStatsComputed());

	return (double)nValueNumber;
}

///////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveContinuousStats

KWDescriptiveContinuousStats::KWDescriptiveContinuousStats()
{
	cMin = 0;
	cMax = 0;
	cMean = 0;
	cStandardDeviation = 0;
	nMissingValueNumber = 0;
}

KWDescriptiveContinuousStats::~KWDescriptiveContinuousStats() {}

Continuous KWDescriptiveContinuousStats::GetMin() const
{
	require(IsStatsComputed());
	return cMin;
}

Continuous KWDescriptiveContinuousStats::GetMax() const
{
	require(IsStatsComputed());
	return cMax;
}

Continuous KWDescriptiveContinuousStats::GetMean() const
{
	require(IsStatsComputed());
	return cMean;
}

Continuous KWDescriptiveContinuousStats::GetStandardDeviation() const
{
	require(IsStatsComputed());
	return cStandardDeviation;
}

int KWDescriptiveContinuousStats::GetMissingValueNumber() const
{
	require(IsStatsComputed());
	return nMissingValueNumber;
}

boolean KWDescriptiveContinuousStats::ComputeStats(const KWTupleTable* tupleTable)
{
	int nTuple;
	int nFirstActualValueTuple;
	const KWTuple* tuple;
	Continuous cRef;
	Continuous cValue;
	double dValue;
	double dAttributeSum;
	double dAttributeSquareSum;
	int nFilledValueNumber;

	require(Check());
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Continuous);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);

	// Initialisation
	Init();
	bIsStatsComputed = true;

	// Calcul des stats dans le cas ou la base est non vide
	if (bIsStatsComputed and tupleTable->GetTotalFrequency() > 0)
	{
		// On passe toutes les valeurs manquantes
		for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
		{
			tuple = tupleTable->GetAt(nTuple);

			// Acces a la valeur
			cValue = tuple->GetContinuousAt(0);

			// Arret si valeur non manquante
			if (cValue == KWContinuous::GetMissingValue())
				nMissingValueNumber += tuple->GetFrequency();
			else
				break;
		}

		// Index du premier tuple avec une valeur non manquante
		nFirstActualValueTuple = nTuple;
		assert(nFirstActualValueTuple == 0 or
		       tupleTable->GetAt(nFirstActualValueTuple - 1)->GetContinuousAt(0) ==
			   KWContinuous::GetMissingValue());

		// Statistiques descriptives si au moins une valeur non manquante
		if (nMissingValueNumber < tupleTable->GetTotalFrequency())
		{
			// Recherche de la valeur Min: premier objet restant
			tuple = tupleTable->GetAt(nFirstActualValueTuple);
			cMin = tuple->GetContinuousAt(0);

			// Recherche de la valeur Max: dernier objet
			tuple = tupleTable->GetAt(tupleTable->GetSize() - 1);
			cMax = tuple->GetContinuousAt(0);

			// Cas particulier: Min=Max
			if (cMin == cMax)
			{
				cMean = cMin;
				cStandardDeviation = 0;
				nValueNumber = 1;
			}
			// Sinon, calcul des statistiques
			else
			{
				// Initialisation
				dAttributeSum = 0;
				dAttributeSquareSum = 0;

				// Parcours des objets de la base pour le calcul des statistiques
				// On va collecter les index et effectifs des valeurs d'effectif
				// important en une passe.
				cRef = 0;
				for (nTuple = nFirstActualValueTuple; nTuple < tupleTable->GetSize(); nTuple++)
				{
					tuple = tupleTable->GetAt(nTuple);

					// Acces a la valeur
					cValue = tuple->GetContinuousAt(0);
					dValue = cValue;

					// Mise a jour des sommes
					dAttributeSum += dValue * tuple->GetFrequency();
					dAttributeSquareSum += dValue * dValue * tuple->GetFrequency();

					// Test si premiere valeur, ou si valeur differente
					if (nValueNumber == 0 or cValue != cRef)
						nValueNumber++;

					// Changement de reference
					assert(nValueNumber == 1 or cRef <= cValue);
					cRef = cValue;
				}

				// Calcul de la moyenne
				nFilledValueNumber = tupleTable->GetTotalFrequency() - nMissingValueNumber;
				assert(nFilledValueNumber > 0);
				cMean = (Continuous)(dAttributeSum / nFilledValueNumber);
				assert(cMin <= cMean and cMean <= cMax);

				// Calcul de l'ecart type
				cStandardDeviation = (Continuous)sqrt(
				    fabs((dAttributeSquareSum - dAttributeSum * dAttributeSum / nFilledValueNumber) /
					 nFilledValueNumber));
				assert(cStandardDeviation >= 0);
			}
		}

		// Incrementation du nombre de valeurs si valeur manquante
		if (nMissingValueNumber > 0)
			nValueNumber++;
	}

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		Init();
	return bIsStatsComputed;
}

void KWDescriptiveContinuousStats::Init()
{
	KWDescriptiveStats::Init();

	// Par defaut, les statistiques descriptives sont manquantes
	cMin = KWContinuous::GetMissingValue();
	cMax = KWContinuous::GetMissingValue();
	cMean = KWContinuous::GetMissingValue();
	cStandardDeviation = KWContinuous::GetMissingValue();
	nMissingValueNumber = 0;
}

KWDescriptiveStats* KWDescriptiveContinuousStats::Clone() const
{
	KWDescriptiveContinuousStats* kwdcsClone;

	kwdcsClone = new KWDescriptiveContinuousStats;
	kwdcsClone->SetLearningSpec(GetLearningSpec());
	kwdcsClone->SetAttributeName(GetAttributeName());
	kwdcsClone->bIsStatsComputed = bIsStatsComputed;
	kwdcsClone->nValueNumber = nValueNumber;
	kwdcsClone->cMin = cMin;
	kwdcsClone->cMax = cMax;
	kwdcsClone->cMean = cMean;
	kwdcsClone->cStandardDeviation = cStandardDeviation;
	kwdcsClone->nMissingValueNumber = nMissingValueNumber;
	return kwdcsClone;
}

longint KWDescriptiveContinuousStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDescriptiveContinuousStats);
	lUsedMemory += sAttributeName.GetUsedMemory();
	return lUsedMemory;
}

void KWDescriptiveContinuousStats::WriteReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	// On passe par la methode de la classe KWContinuous pour gerer le cas des valeurs manquantes
	ost << "Values\t" << GetValueNumber() << "\n";
	ost << "Min\t" << KWContinuous::ContinuousToString(GetMin()) << "\n";
	ost << "Max\t" << KWContinuous::ContinuousToString(GetMax()) << "\n";
	ost << "Mean\t" << KWContinuous::ContinuousToString(GetMean()) << "\n";
	ost << "Std dev\t" << KWContinuous::ContinuousToString(GetStandardDeviation()) << "\n";
	ost << "Missing number\t" << GetMissingValueNumber() << "\n";
}

void KWDescriptiveContinuousStats::WriteJSONFields(JSONFile* fJSON)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	fJSON->WriteKeyInt("values", GetValueNumber());
	fJSON->WriteKeyContinuous("min", GetMin());
	fJSON->WriteKeyContinuous("max", GetMax());
	fJSON->WriteKeyContinuous("mean", GetMean());
	fJSON->WriteKeyContinuous("stdDev", GetStandardDeviation());
	fJSON->WriteKeyInt("missingNumber", GetMissingValueNumber());
}

void KWDescriptiveContinuousStats::WriteHeaderLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << "Values\t";
	ost << "Min\t";
	ost << "Max\t";
	ost << "Mean\t";
	ost << "Std dev\t";
	ost << "Missing number";
}

void KWDescriptiveContinuousStats::WriteLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	// On passe par la methode de la classe KWContinuous pour gerer le cas des valeurs manquantes
	ost << GetValueNumber() << "\t";
	ost << KWContinuous::ContinuousToString(GetMin()) << "\t";
	ost << KWContinuous::ContinuousToString(GetMax()) << "\t";
	ost << KWContinuous::ContinuousToString(GetMean()) << "\t";
	ost << KWContinuous::ContinuousToString(GetStandardDeviation()) << "\t";
	ost << GetMissingValueNumber();
}

///////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveSymbolStats

KWDescriptiveSymbolStats::KWDescriptiveSymbolStats()
{
	dEntropy = 0;
	nModeFrequency = 0;
	nTotalFrequency = 0;
}

KWDescriptiveSymbolStats::~KWDescriptiveSymbolStats() {}

double KWDescriptiveSymbolStats::GetEntropy() const
{
	require(IsStatsComputed());

	return dEntropy;
}

Symbol& KWDescriptiveSymbolStats::GetMode() const
{
	require(IsStatsComputed());

	return sMode;
}

int KWDescriptiveSymbolStats::GetModeFrequency() const
{
	require(IsStatsComputed());

	return nModeFrequency;
}

int KWDescriptiveSymbolStats::GetTotalFrequency() const
{
	require(IsStatsComputed());

	return nTotalFrequency;
}

boolean KWDescriptiveSymbolStats::ComputeStats(const KWTupleTable* tupleTable)
{
	int nTuple;
	const KWTuple* tuple;
	int nValueFrequency;
	int nPreviousValueFrequency;
	Symbol sValue;
	Symbol sRef;
	double dProba;

	require(Check());
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);

	// Initialisation
	Init();
	bIsStatsComputed = true;

	// Calcul des stats dans le cas ou la table de tuple est non vide
	if (tupleTable->GetTotalFrequency() > 0)
	{
		// Parcours des objets de la base pour le calcul des statistiques (recherche du mode,
		// de sa frequence, du nombre de modalites distinctes et de l'entropie de l'attribut)
		sRef.Reset();
		nValueFrequency = 0;
		nPreviousValueFrequency = 0;
		for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
		{
			tuple = tupleTable->GetAt(nTuple);

			// Acces a la valeur
			sValue = tuple->GetSymbolAt(0);
			assert(nValueNumber == 0 or sValue >= sRef);

			// Test si premier objet, ou si valeur differente
			if (nTuple == 0 or sValue != sRef)
			{
				nValueNumber++;
				nPreviousValueFrequency = nValueFrequency;
				nValueFrequency = tuple->GetFrequency();
			}
			else
				nValueFrequency += tuple->GetFrequency();

			// Test si dernier objet d'une valeur
			if (nTuple > 0 and sValue != sRef)
			{
				// Mise a jour de l'effectif total
				nTotalFrequency += nPreviousValueFrequency;

				// Mise a jour de l'entropie de l'attribut considere
				// Calcul de la frequence normalisee de la modalite
				dProba = (double)nPreviousValueFrequency / (double)tupleTable->GetTotalFrequency();

				// Ajout de sa contribution a l'entropie
				dEntropy += dProba * log(dProba);

				// Selection (eventuelle) de la modalite pour le mode
				if (nPreviousValueFrequency >= nModeFrequency)
				{
					// On garde la valeur la plus frequente en prenant la premiere par ordre
					// lexicographique, ce qui permet la reproductibilite des tests
					if (nPreviousValueFrequency > nModeFrequency or sRef.CompareValue(sMode) < 0)
					{
						sMode = sRef;
						nModeFrequency = nPreviousValueFrequency;
					}
				}
			}

			// Test si dernier tuple, a traiter necessairement
			if (nTuple == tupleTable->GetSize() - 1)
			{
				// On se ramene au cas general pour avoir une code identique
				nPreviousValueFrequency = nValueFrequency;
				sRef = sValue;

				// Mise a jour de l'effectif total
				nTotalFrequency += nPreviousValueFrequency;

				// Mise a jour de l'entropie de l'attribut considere
				// Calcul de la frequence normalisee de la modalite
				dProba = (double)nPreviousValueFrequency / (double)tupleTable->GetTotalFrequency();

				// Ajout de sa contribution a l'entropie
				dEntropy += dProba * log(dProba);

				// Selection (eventuelle) de la modalite pour le mode
				if (nPreviousValueFrequency >= nModeFrequency)
				{
					// On garde la valeur la plus frequente en prenant la premiere par ordre
					// lexicographique, ce qui permet la reproductibilite des tests
					if (nPreviousValueFrequency > nModeFrequency or sRef.CompareValue(sMode) < 0)
					{
						sMode = sRef;
						nModeFrequency = nPreviousValueFrequency;
					}
				}
			}

			// Changement de reference
			assert(nValueNumber == 1 or sRef <= sValue);
			sRef = sValue;
		}
		assert(nValueNumber >= 1 or tupleTable->GetTotalFrequency() == 0);
		assert(nTotalFrequency == tupleTable->GetTotalFrequency());

		// Normalisation de l'entropie en base 2 et seuillage
		dEntropy /= -log(2.0);
		assert(dEntropy > -1e-10);
		dEntropy = fabs(dEntropy);
		if (dEntropy < 1e-10)
			dEntropy = 0;
	}

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		Init();
	return bIsStatsComputed;
}

void KWDescriptiveSymbolStats::Init()
{
	KWDescriptiveStats::Init();
	dEntropy = 0;
	sMode.Reset();
	nModeFrequency = 0;
	nTotalFrequency = 0;
}

KWDescriptiveStats* KWDescriptiveSymbolStats::Clone() const
{
	KWDescriptiveSymbolStats* kwdssClone;

	kwdssClone = new KWDescriptiveSymbolStats;
	kwdssClone->SetLearningSpec(GetLearningSpec());
	kwdssClone->SetAttributeName(GetAttributeName());
	kwdssClone->bIsStatsComputed = bIsStatsComputed;
	kwdssClone->nValueNumber = nValueNumber;
	kwdssClone->dEntropy = dEntropy;
	kwdssClone->sMode = sMode;
	kwdssClone->nModeFrequency = nModeFrequency;
	kwdssClone->nTotalFrequency = nTotalFrequency;
	return kwdssClone;
}

longint KWDescriptiveSymbolStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDescriptiveSymbolStats);
	lUsedMemory += sAttributeName.GetUsedMemory();
	return lUsedMemory;
}

void KWDescriptiveSymbolStats::WriteReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << "Values\t" << GetValueNumber() << "\n";
	ost << "Init. entr.\t" << GetEntropy() << "\n";
	ost << "Mode\t" << GetMode() << "\n";
	ost << "Mode coverage\t" << GetModeFrequency() << "\n";
}

void KWDescriptiveSymbolStats::WriteHeaderLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << "Values\t";
	ost << "Mode\t";
	ost << "Mode coverage";
}

void KWDescriptiveSymbolStats::WriteLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << GetValueNumber() << "\t";
	ost << GetMode() << "\t";
	if (nTotalFrequency == 0)
		ost << "0";
	else
		ost << GetModeFrequency() / (double)nTotalFrequency;
}

void KWDescriptiveSymbolStats::WriteJSONFields(JSONFile* fJSON)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	fJSON->WriteKeyInt("values", GetValueNumber());
	fJSON->WriteKeyString("mode", GetMode().GetValue());
	fJSON->WriteKeyDouble("modeFrequency", GetModeFrequency());
}

////////////////////////////////////////////////////////////////////
// Implementation de PLShared_DescriptiveStats

PLShared_DescriptiveStats::PLShared_DescriptiveStats() {}

PLShared_DescriptiveStats::~PLShared_DescriptiveStats() {}

void PLShared_DescriptiveStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDescriptiveStats* decriptiveStats;

	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_LearningReport::SerializeObject(serializer, o);

	// Serialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveStats*, o);
	serializer->PutString(decriptiveStats->sAttributeName);
	serializer->PutInt(decriptiveStats->nValueNumber);
}

void PLShared_DescriptiveStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDescriptiveStats* decriptiveStats;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_LearningReport::DeserializeObject(serializer, o);

	// Deserialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveStats*, o);
	decriptiveStats->SetAttributeName(serializer->GetString());
	decriptiveStats->nValueNumber = serializer->GetInt();
}

////////////////////////////////////////////////////////////////////
// Implementation de PLShared_DescriptiveContinuousStats

PLShared_DescriptiveContinuousStats::PLShared_DescriptiveContinuousStats() {}

PLShared_DescriptiveContinuousStats::~PLShared_DescriptiveContinuousStats() {}

void PLShared_DescriptiveContinuousStats::SetDescriptiveStats(KWDescriptiveContinuousStats* descriptiveStats)
{
	require(descriptiveStats != NULL);
	SetObject(descriptiveStats);
}

KWDescriptiveContinuousStats* PLShared_DescriptiveContinuousStats::GetDescriptiveStats()
{
	return cast(KWDescriptiveContinuousStats*, GetObject());
}

void PLShared_DescriptiveContinuousStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLShared_Continuous cMin;
	PLShared_Continuous cMax;
	PLShared_Continuous cMean;
	PLShared_Continuous cStandardDeviation;
	KWDescriptiveContinuousStats* decriptiveStats;
	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::SerializeObject(serializer, o);

	// Serialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveContinuousStats*, o);
	cMin = decriptiveStats->GetMin();
	cMin.Serialize(serializer);
	cMax = decriptiveStats->GetMax();
	cMax.Serialize(serializer);
	cMean = decriptiveStats->GetMean();
	cMean.Serialize(serializer);
	cStandardDeviation = decriptiveStats->GetStandardDeviation();
	cStandardDeviation.Serialize(serializer);
	serializer->PutInt(decriptiveStats->GetMissingValueNumber());
}

void PLShared_DescriptiveContinuousStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDescriptiveContinuousStats* decriptiveStats;
	PLShared_Continuous sharedContinuous;
	PLShared_Continuous cMax;
	PLShared_Continuous cMean;
	PLShared_Continuous cStandardDeviation;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::DeserializeObject(serializer, o);

	// Deserialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveContinuousStats*, o);
	sharedContinuous.Deserialize(serializer);
	decriptiveStats->cMin = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	decriptiveStats->cMax = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	decriptiveStats->cMean = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	decriptiveStats->cStandardDeviation = sharedContinuous;
	decriptiveStats->nMissingValueNumber = serializer->GetInt();
}

Object* PLShared_DescriptiveContinuousStats::Create() const
{
	return new KWDescriptiveContinuousStats;
}

////////////////////////////////////////////////////////////////////
// Implementation de PLShared_DescriptiveSymbolStats

PLShared_DescriptiveSymbolStats::PLShared_DescriptiveSymbolStats() {}

PLShared_DescriptiveSymbolStats::~PLShared_DescriptiveSymbolStats() {}

void PLShared_DescriptiveSymbolStats::SetDescriptiveStats(KWDescriptiveSymbolStats* descriptiveStats)
{
	require(descriptiveStats != NULL);
	SetObject(descriptiveStats);
}

KWDescriptiveSymbolStats* PLShared_DescriptiveSymbolStats::GetDescriptiveStats()
{
	return cast(KWDescriptiveSymbolStats*, GetObject());
}

void PLShared_DescriptiveSymbolStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDescriptiveSymbolStats* decriptiveStats;
	PLShared_Symbol sharedSymbol;

	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::SerializeObject(serializer, o);

	// Serialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveSymbolStats*, o);
	serializer->PutDouble(decriptiveStats->GetEntropy());
	sharedSymbol = decriptiveStats->sMode;
	sharedSymbol.Serialize(serializer);
	serializer->PutInt(decriptiveStats->nModeFrequency);
	serializer->PutInt(decriptiveStats->nTotalFrequency);
}

void PLShared_DescriptiveSymbolStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDescriptiveSymbolStats* decriptiveStats;
	PLShared_Symbol sharedSymbol;
	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::DeserializeObject(serializer, o);

	// Deserialization des attributs specifiques
	decriptiveStats = cast(KWDescriptiveSymbolStats*, o);
	decriptiveStats->dEntropy = serializer->GetDouble();
	sharedSymbol.Deserialize(serializer);
	decriptiveStats->sMode = sharedSymbol;
	decriptiveStats->nModeFrequency = serializer->GetInt();
	decriptiveStats->nTotalFrequency = serializer->GetInt();
}

Object* PLShared_DescriptiveSymbolStats::Create() const
{
	return new KWDescriptiveSymbolStats;
}
