// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDescriptiveStats.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveStats

KWDescriptiveStats::KWDescriptiveStats()
{
	bIsStatsComputed = false;
	nValueNumber = 0;
	nMissingValueNumber = 0;
	nSparseMissingValueNumber = 0;
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

void KWDescriptiveStats::Init()
{
	bIsStatsComputed = false;
	nValueNumber = 0;
	nMissingValueNumber = 0;
}

boolean KWDescriptiveStats::ComputeStats()
{
	// Ne doit pas etre appele pour cette classe et ses sous-classes
	assert(false);
	return bIsStatsComputed;
}

boolean KWDescriptiveStats::ComputeStats(const KWTupleTable* tupleTable)
{
	// Initialisation
	Init();
	nSparseMissingValueNumber = tupleTable->GetSparseMissingValueNumber();

	return true;
}

int KWDescriptiveStats::GetValueNumber() const
{
	require(IsStatsComputed());
	return nValueNumber;
}

int KWDescriptiveStats::GetMissingValueNumber() const
{
	require(IsStatsComputed());
	return nMissingValueNumber;
}

int KWDescriptiveStats::GetSparseMissingValueNumber() const
{
	require(IsStatsComputed());
	return nSparseMissingValueNumber;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveContinuousStats

KWDescriptiveContinuousStats::KWDescriptiveContinuousStats()
{
	cMin = 0;
	cMax = 0;
	cMean = 0;
	cStandardDeviation = 0;
}

KWDescriptiveContinuousStats::~KWDescriptiveContinuousStats() {}

void KWDescriptiveContinuousStats::Init()
{
	// Appel a la methode ancetre
	KWDescriptiveStats::Init();

	// Par defaut, les statistiques descriptives sont manquantes
	cMin = KWContinuous::GetMissingValue();
	cMax = KWContinuous::GetMissingValue();
	cMean = KWContinuous::GetMissingValue();
	cStandardDeviation = KWContinuous::GetMissingValue();
}

boolean KWDescriptiveContinuousStats::ComputeStats(const KWTupleTable* tupleTable)
{
	int nTuple;
	const KWTuple* tuple;
	Continuous cValue;
	int nFirstActualValueTuple;
	double dAttributeSum;
	double dAttributeSquareSum;
	Continuous cRef;
	double dValue;
	int nFilledValueNumber;

	require(Check());
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Continuous);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);

	// Appel a la methode ancetre
	bIsStatsComputed = KWDescriptiveStats::ComputeStats(tupleTable);

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

		// Calcul des statistiques descriptives si au moins une valeur non manquante
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
				// On va collecter les index et effectifs des valeurs d'effectif important en une passe
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
				cMean = (dAttributeSum / nFilledValueNumber);
				assert(cMin <= cMean and cMean <= cMax);

				// Calcul de l'ecart type
				cStandardDeviation = sqrt(
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

void KWDescriptiveContinuousStats::WriteReport(ostream& ost)
{
	require(IsStatsComputed());

	// On passe par la methode de la classe KWContinuous pour gerer le cas des valeurs manquantes
	ost << "Values\t" << GetValueNumber() << "\n";
	ost << "Min\t" << KWContinuous::ContinuousToString(GetMin()) << "\n";
	ost << "Max\t" << KWContinuous::ContinuousToString(GetMax()) << "\n";
	ost << "Mean\t" << KWContinuous::ContinuousToString(GetMean()) << "\n";
	ost << "Std dev\t" << KWContinuous::ContinuousToString(GetStandardDeviation()) << "\n";
	ost << "Missing number\t" << GetMissingValueNumber() << "\n";
	ost << "Sparse missing number\t" << GetSparseMissingValueNumber() << "\n";
}

void KWDescriptiveContinuousStats::WriteHeaderLineReport(ostream& ost)
{
	require(IsStatsComputed());

	ost << "Values\t";
	ost << "Min\t";
	ost << "Max\t";
	ost << "Mean\t";
	ost << "Std dev\t";
	ost << "Missing number\t";
	ost << "Sparse missing number";
}

void KWDescriptiveContinuousStats::WriteLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// On passe par la methode de la classe KWContinuous pour gerer le cas des valeurs manquantes
	ost << GetValueNumber() << "\t";
	ost << KWContinuous::ContinuousToString(GetMin()) << "\t";
	ost << KWContinuous::ContinuousToString(GetMax()) << "\t";
	ost << KWContinuous::ContinuousToString(GetMean()) << "\t";
	ost << KWContinuous::ContinuousToString(GetStandardDeviation()) << "\t";
	ost << GetMissingValueNumber() << "\t";
	ost << GetSparseMissingValueNumber();
}

void KWDescriptiveContinuousStats::WriteJSONFields(JSONFile* fJSON)
{
	require(IsStatsComputed());

	fJSON->WriteKeyInt("values", GetValueNumber());
	fJSON->WriteKeyContinuous("min", GetMin());
	fJSON->WriteKeyContinuous("max", GetMax());
	fJSON->WriteKeyContinuous("mean", GetMean());
	fJSON->WriteKeyContinuous("stdDev", GetStandardDeviation());
	fJSON->WriteKeyInt("missingNumber", GetMissingValueNumber());
	fJSON->WriteKeyInt("sparseMissingNumber", GetSparseMissingValueNumber());
}

KWDescriptiveStats* KWDescriptiveContinuousStats::Clone() const
{
	KWDescriptiveContinuousStats* continuousStatsClone;

	continuousStatsClone = new KWDescriptiveContinuousStats;
	continuousStatsClone->SetLearningSpec(GetLearningSpec());
	continuousStatsClone->SetAttributeName(GetAttributeName());
	continuousStatsClone->bIsStatsComputed = bIsStatsComputed;
	continuousStatsClone->nValueNumber = nValueNumber;
	continuousStatsClone->cMin = cMin;
	continuousStatsClone->cMax = cMax;
	continuousStatsClone->cMean = cMean;
	continuousStatsClone->cStandardDeviation = cStandardDeviation;
	continuousStatsClone->nMissingValueNumber = nMissingValueNumber;
	continuousStatsClone->nSparseMissingValueNumber = nSparseMissingValueNumber;

	return continuousStatsClone;
}

longint KWDescriptiveContinuousStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDescriptiveContinuousStats);
	lUsedMemory += sAttributeName.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveSymbolStats

KWDescriptiveSymbolStats::KWDescriptiveSymbolStats()
{
	dEntropy = 0;
	nModeFrequency = 0;
	nTotalFrequency = 0;
}

KWDescriptiveSymbolStats::~KWDescriptiveSymbolStats() {}

void KWDescriptiveSymbolStats::Init()
{
	// Appel a la methode ancetre
	KWDescriptiveStats::Init();

	// Mise a zero des statistiques
	dEntropy = 0;
	sMode.Reset();
	nModeFrequency = 0;
	nTotalFrequency = 0;
}

boolean KWDescriptiveSymbolStats::ComputeStats(const KWTupleTable* tupleTable)
{
	Symbol sRef;
	int nValueFrequency;
	int nPreviousValueFrequency;
	int nTuple;
	const KWTuple* tuple;
	Symbol sValue;
	double dProba;

	require(Check());
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require(not GetLearningSpec()->GetCheckTargetAttribute() or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);

	// Appel a la methode ancetre
	bIsStatsComputed = KWDescriptiveStats::ComputeStats(tupleTable);

	// Calcul des stats dans le cas ou la base est non vide
	if (bIsStatsComputed and tupleTable->GetTotalFrequency() > 0)
	{
		// On passe toutes les valeurs manquantes
		for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
		{
			tuple = tupleTable->GetAt(nTuple);

			// Acces a la valeur
			sValue = tuple->GetSymbolAt(0);

			// Arret si valeur non manquante
			if (sValue == Symbol(""))
				nMissingValueNumber += tuple->GetFrequency();
			else
				break;
		}
	}

	// Calcul des stats seulement quand la table de tuples est non vide
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
		assert(nMissingValueNumber >= 0);

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

void KWDescriptiveSymbolStats::WriteReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << "Values\t" << GetValueNumber() << "\n";
	ost << "Init. entr.\t" << GetEntropy() << "\n";
	ost << "Mode\t" << TSV::Export(GetMode().GetValue()) << "\n";
	ost << "Mode coverage\t" << GetModeFrequency() << "\n";
	ost << "Missing number\t" << GetMissingValueNumber() << "\n";
	ost << "Sparse missing number\t" << GetSparseMissingValueNumber() << "\n";
}

void KWDescriptiveSymbolStats::WriteHeaderLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << "Values\t";
	ost << "Mode\t";
	ost << "Mode coverage\t";
	ost << "Missing number\t";
	ost << "Sparse missing number";
}

void KWDescriptiveSymbolStats::WriteLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	ost << GetValueNumber() << "\t";
	ost << TSV::Export(GetMode().GetValue()) << "\t";
	if (nTotalFrequency == 0)
		ost << "0\t";
	else
		ost << GetModeFrequency() / (double)nTotalFrequency << "\t";
	ost << GetMissingValueNumber() << "\t";
	ost << GetSparseMissingValueNumber();
}

void KWDescriptiveSymbolStats::WriteJSONFields(JSONFile* fJSON)
{
	require(IsStatsComputed());

	// Statistiques descriptives
	fJSON->WriteKeyInt("values", GetValueNumber());
	fJSON->WriteKeyString("mode", GetMode().GetValue());
	fJSON->WriteKeyDouble("modeFrequency", GetModeFrequency());
	fJSON->WriteKeyInt("missingNumber", GetMissingValueNumber());
	fJSON->WriteKeyInt("sparseMissingNumber", GetSparseMissingValueNumber());
}

KWDescriptiveStats* KWDescriptiveSymbolStats::Clone() const
{
	KWDescriptiveSymbolStats* symbolStatsClone;

	symbolStatsClone = new KWDescriptiveSymbolStats;
	symbolStatsClone->SetLearningSpec(GetLearningSpec());
	symbolStatsClone->SetAttributeName(GetAttributeName());
	symbolStatsClone->bIsStatsComputed = bIsStatsComputed;
	symbolStatsClone->nValueNumber = nValueNumber;
	symbolStatsClone->dEntropy = dEntropy;
	symbolStatsClone->sMode = sMode;
	symbolStatsClone->nModeFrequency = nModeFrequency;
	symbolStatsClone->nTotalFrequency = nTotalFrequency;
	return symbolStatsClone;
}

longint KWDescriptiveSymbolStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDescriptiveSymbolStats);
	lUsedMemory += sAttributeName.GetUsedMemory();

	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DescriptiveStats

PLShared_DescriptiveStats::PLShared_DescriptiveStats() {}

PLShared_DescriptiveStats::~PLShared_DescriptiveStats() {}

void PLShared_DescriptiveStats::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	KWDescriptiveStats* descriptiveStats;

	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_LearningReport::SerializeObject(serializer, object);

	// Serialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveStats*, object);
	serializer->PutString(descriptiveStats->sAttributeName);
	serializer->PutInt(descriptiveStats->nValueNumber);
	serializer->PutInt(descriptiveStats->nMissingValueNumber);
	serializer->PutInt(descriptiveStats->nSparseMissingValueNumber);
}

void PLShared_DescriptiveStats::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	KWDescriptiveStats* descriptiveStats;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_LearningReport::DeserializeObject(serializer, object);

	// Deserialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveStats*, object);
	descriptiveStats->SetAttributeName(serializer->GetString());
	descriptiveStats->nValueNumber = serializer->GetInt();
	descriptiveStats->nMissingValueNumber = serializer->GetInt();
	descriptiveStats->nSparseMissingValueNumber = serializer->GetInt();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DescriptiveContinuousStats

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

void PLShared_DescriptiveContinuousStats::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	PLShared_Continuous cMin;
	PLShared_Continuous cMax;
	PLShared_Continuous cMean;
	PLShared_Continuous cStandardDeviation;
	KWDescriptiveContinuousStats* descriptiveStats;
	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::SerializeObject(serializer, object);

	// Serialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveContinuousStats*, object);
	cMin = descriptiveStats->GetMin();
	cMin.Serialize(serializer);
	cMax = descriptiveStats->GetMax();
	cMax.Serialize(serializer);
	cMean = descriptiveStats->GetMean();
	cMean.Serialize(serializer);
	cStandardDeviation = descriptiveStats->GetStandardDeviation();
	cStandardDeviation.Serialize(serializer);
}

void PLShared_DescriptiveContinuousStats::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	KWDescriptiveContinuousStats* descriptiveStats;
	PLShared_Continuous sharedContinuous;
	PLShared_Continuous cMax;
	PLShared_Continuous cMean;
	PLShared_Continuous cStandardDeviation;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::DeserializeObject(serializer, object);

	// Deserialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveContinuousStats*, object);
	sharedContinuous.Deserialize(serializer);
	descriptiveStats->cMin = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	descriptiveStats->cMax = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	descriptiveStats->cMean = sharedContinuous;
	sharedContinuous.Deserialize(serializer);
	descriptiveStats->cStandardDeviation = sharedContinuous;
}

Object* PLShared_DescriptiveContinuousStats::Create() const
{
	return new KWDescriptiveContinuousStats;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DescriptiveSymbolStats

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

void PLShared_DescriptiveSymbolStats::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	KWDescriptiveSymbolStats* descriptiveStats;
	PLShared_Symbol sharedSymbol;

	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::SerializeObject(serializer, object);

	// Serialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveSymbolStats*, object);
	serializer->PutDouble(descriptiveStats->GetEntropy());
	sharedSymbol = descriptiveStats->sMode;
	sharedSymbol.Serialize(serializer);
	serializer->PutInt(descriptiveStats->nModeFrequency);
	serializer->PutInt(descriptiveStats->nTotalFrequency);
}

void PLShared_DescriptiveSymbolStats::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	KWDescriptiveSymbolStats* descriptiveStats;
	PLShared_Symbol sharedSymbol;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_DescriptiveStats::DeserializeObject(serializer, object);

	// Deserialization des attributs specifiques
	descriptiveStats = cast(KWDescriptiveSymbolStats*, object);
	descriptiveStats->dEntropy = serializer->GetDouble();
	sharedSymbol.Deserialize(serializer);
	descriptiveStats->sMode = sharedSymbol;
	descriptiveStats->nModeFrequency = serializer->GetInt();
	descriptiveStats->nTotalFrequency = serializer->GetInt();
}

Object* PLShared_DescriptiveSymbolStats::Create() const
{
	return new KWDescriptiveSymbolStats;
}
