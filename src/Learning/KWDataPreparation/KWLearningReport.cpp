// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningReport.h"

///////////////////////////////////////////////////////////////////////////////////
// Classe KWLearningReport

KWLearningReport::KWLearningReport()
{
	bIsStatsComputed = false;
}

KWLearningReport::~KWLearningReport() {}

boolean KWLearningReport::ComputeStats()
{
	return bIsStatsComputed;
}

boolean KWLearningReport::IsStatsComputed() const
{
	return bIsStatsComputed;
}

void KWLearningReport::WriteReportFile(const ALString& sFileName)
{
	fstream ost;
	boolean bOk;
	ALString sLocalFileName;

	require(IsStatsComputed());

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Write report Begin");

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, ost);
	if (bOk)
	{
		if (GetLearningReportHeaderLine() != "")
			ost << GetLearningReportHeaderLine() << "\n";
		WriteReport(ost);
		bOk = FileService::CloseOutputFile(sLocalFileName, ost);

		// Destruction du rapport si erreur
		if (not bOk)
			FileService::RemoveFile(sLocalFileName);
	}

	if (bOk)
	{
		// Copie vers HDFS
		PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);
	}

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Write report End");
}

void KWLearningReport::WriteReport(ostream& ost)
{
	ost << flush;
}

void KWLearningReport::WriteHeaderLineReport(ostream& ost)
{
	ost << flush;
}

void KWLearningReport::WriteLineReport(ostream& ost)
{
	ost << flush;
}

boolean KWLearningReport::IsReported() const
{
	return true;
}

boolean KWLearningReport::IsLineReported() const
{
	return true;
}

const ALString KWLearningReport::GetSortName() const
{
	return "";
}

double KWLearningReport::GetSortValue() const
{
	return 0;
}

int KWLearningReport::CompareName(const KWLearningReport* otherReport) const
{
	return GetSortName().Compare(otherReport->GetSortName());
}

int KWLearningReport::CompareValue(const KWLearningReport* otherReport) const
{
	int nCompare;
	longint lSortValue1;
	longint lSortValue2;

	// On se base sur un comparaison a dix decimales pres
	if (GetSortValue() >= 0)
		lSortValue1 = longint(GetSortValue() * 1e10);
	else
		lSortValue1 = -longint(-GetSortValue() * 1e10);
	if (otherReport->GetSortValue() >= 0)
		lSortValue2 = longint(otherReport->GetSortValue() * 1e10);
	else
		lSortValue2 = -longint(-otherReport->GetSortValue() * 1e10);

	// Comparaison par
	nCompare = -CompareLongint(lSortValue1, lSortValue2);

	// En cas d'egalite, on se base sur le nom
	if (nCompare == 0)
		nCompare = CompareName(otherReport);
	return nCompare;
}

void KWLearningReport::SetIdentifier(const ALString& sValue)
{
	sIdentifier = sValue;
}

const ALString& KWLearningReport::GetIdentifier() const
{
	return sIdentifier;
}

void KWLearningReport::ComputeRankIdentifiers(ObjectArray* oaLearningReports)
{
	const double dEpsilon = 1e-10;
	ObjectArray oaSortedReports;
	KWLearningReport* learningReport;
	int nReport;
	int i;
	int nMaxDigitNumber;
	int nDigitNumber;
	ALString sNewIdentifier;

	require(oaLearningReports != NULL);

	// Affichage si tableau non vide
	if (oaLearningReports->GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.CopyFrom(oaLearningReports);
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Calcul du nombre max de chiffres necessaires
		nMaxDigitNumber = 1 + (int)floor(dEpsilon + log((double)oaLearningReports->GetSize()) / log(10.0));

		// Parcours des rapports
		for (nReport = 0; nReport < oaSortedReports.GetSize(); nReport++)
		{
			learningReport = cast(KWLearningReport*, oaSortedReports.GetAt(nReport));

			// Calcul du nombre de chiffre necessaires
			nDigitNumber = 1 + (int)floor(dEpsilon + log((double)(nReport + 1)) / log(10.0));

			// Fabrication de l'identifiant, en ajoutant des caracteres '0' a gauche si necessaire
			// pour avoir une longueur fixe
			sNewIdentifier = "R";
			for (i = nDigitNumber; i < nMaxDigitNumber; i++)
				sNewIdentifier += '0';
			sNewIdentifier += IntToString(nReport + 1);

			// Memorisation de l'identifiant
			learningReport->SetIdentifier(sNewIdentifier);
		}
	}
}

void KWLearningReport::WriteArrayLineReport(ostream& ost, const ALString& sTitle, ObjectArray* oaLearningReports)
{
	ObjectArray oaSortedReports;
	int i;
	KWLearningReport* learningReport;

	require(oaLearningReports != NULL);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		learningReport = cast(KWLearningReport*, oaLearningReports->GetAt(i));
		if (learningReport->IsLineReported())
			oaSortedReports.Add(learningReport);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Parcours des rapports
		if (sTitle != "")
			ost << "\n\n" << sTitle << "\n\n";
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			learningReport = cast(KWLearningReport*, oaSortedReports.GetAt(i));

			// Ligne d'entete
			if (i == 0)
			{
				learningReport->WriteHeaderLineReport(ost);
				ost << "\n";
			}

			// Ligne de de stats
			learningReport->WriteLineReport(ost);
			ost << "\n";
		}
	}
}

void KWLearningReport::WriteArrayReport(ostream& ost, const ALString& sTitle, ObjectArray* oaLearningReports)
{
	ObjectArray oaSortedReports;
	int i;
	KWLearningReport* learningReport;

	require(oaLearningReports != NULL);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		learningReport = cast(KWLearningReport*, oaLearningReports->GetAt(i));
		if (learningReport->IsReported())
			oaSortedReports.Add(learningReport);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Parcours des rapports
		if (sTitle != "")
		{
			ost << "\n\n----------------------------------------";
			ost << "----------------------------------------";
			ost << "\n\n" << sTitle << "\n\n";
		}
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			learningReport = cast(KWLearningReport*, oaSortedReports.GetAt(i));

			// Rapport detaille
			ost << "\n";
			if (i > 0)
				ost << "----------------------------------------------\n";
			learningReport->WriteReport(ost);
		}
	}
}

void KWLearningReport::WriteJSONFields(JSONFile* fJSON) {}

void KWLearningReport::WriteJSONReport(JSONFile* fJSON)
{
	fJSON->BeginObject();
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWLearningReport::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey)
{
	fJSON->BeginKeyObject(sKey);
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWLearningReport::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) {}

boolean KWLearningReport::IsJSONReported(boolean bSummary) const
{
	return bSummary or IsReported();
}

void KWLearningReport::WriteJSONArrayReport(JSONFile* fJSON, const ALString& sArrayKey, ObjectArray* oaLearningReports,
					    boolean bSummary)
{
	ObjectArray oaSortedReports;
	int i;
	KWLearningReport* learningReport;

	require(oaLearningReports != NULL);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		learningReport = cast(KWLearningReport*, oaLearningReports->GetAt(i));
		if (learningReport->IsJSONReported(bSummary))
			oaSortedReports.Add(learningReport);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Ecriture du tableau au format JSON
		fJSON->BeginKeyArray(sArrayKey);
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			learningReport = cast(KWLearningReport*, oaSortedReports.GetAt(i));
			fJSON->BeginObject();
			learningReport->WriteJSONArrayFields(fJSON, bSummary);
			fJSON->EndObject();
		}
		fJSON->EndArray();
	}
}

void KWLearningReport::WriteJSONDictionaryReport(JSONFile* fJSON, const ALString& sDictionaryKey,
						 ObjectArray* oaLearningReports, boolean bSummary)
{
	ObjectArray oaSortedReports;
	int i;
	KWLearningReport* learningReport;

	require(oaLearningReports != NULL);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		learningReport = cast(KWLearningReport*, oaLearningReports->GetAt(i));
		if (learningReport->IsJSONReported(bSummary))
			oaSortedReports.Add(learningReport);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Ecriture du tableau au format JSON
		fJSON->BeginKeyObject(sDictionaryKey);
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			learningReport = cast(KWLearningReport*, oaSortedReports.GetAt(i));
			fJSON->BeginKeyObject(learningReport->GetIdentifier());
			learningReport->WriteJSONArrayFields(fJSON, bSummary);
			fJSON->EndObject();
		}
		fJSON->EndObject();
	}
}

int KWLearningReportCompareSortName(const void* elem1, const void* elem2)
{
	KWLearningReport* report1;
	KWLearningReport* report2;

	check(elem1);
	check(elem2);

	// Acces aux objets
	report1 = cast(KWLearningReport*, *(Object**)elem1);
	report2 = cast(KWLearningReport*, *(Object**)elem2);

	// Retour
	return report1->CompareName(report2);
}

int KWLearningReportCompareSortValue(const void* elem1, const void* elem2)
{
	KWLearningReport* report1;
	KWLearningReport* report2;

	check(elem1);
	check(elem2);

	// Acces aux objets
	report1 = cast(KWLearningReport*, *(Object**)elem1);
	report2 = cast(KWLearningReport*, *(Object**)elem2);

	// Retour
	return report1->CompareValue(report2);
}

///////////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationStats

KWDataPreparationStats::KWDataPreparationStats()
{
	bIsStatsComputed = false;
	preparedDataGridStats = NULL;
	dPreparedLevel = 0;
	dPreparationCost = 0;
	dConstructionCost = 0;
	dDataCost = 0;
}

KWDataPreparationStats::~KWDataPreparationStats()
{
	// Nettoyage en dur, sans recours a la methode virtuelle CleanDataPreparationResults
	// (il n'est pas "conseille" d'utiliser des methodes virtuelles dans les destructeurs)
	if (preparedDataGridStats != NULL)
		delete preparedDataGridStats;
	preparedDataGridStats = NULL;
}

boolean KWDataPreparationStats::ComputeStats()
{
	// Ne doit pas etre appele pour cette classe et ses sous-classes
	assert(false);
	return bIsStatsComputed;
}

boolean KWDataPreparationStats::ComputeStats(const KWTupleTable* tupleTable)
{
	return bIsStatsComputed;
}

int KWDataPreparationStats::CompareName(const KWLearningReport* otherReport) const
{
	const KWDataPreparationStats* otherPreparationStats;
	int nCompare;
	int i;

	// Acces a l'objet compare
	otherPreparationStats = cast(const KWDataPreparationStats*, otherReport);

	// Comparaison sur le nombre d'attributs
	nCompare = GetAttributeNumber() - otherPreparationStats->GetAttributeNumber();

	// Comparaison sur les d'attributs
	if (nCompare == 0)
	{
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			nCompare = GetAttributeNameAt(i).Compare(otherPreparationStats->GetAttributeNameAt(i));
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

KWDataGridStats* KWDataPreparationStats::GetPreparedDataGridStats() const
{
	require(IsStatsComputed());
	return preparedDataGridStats;
}

int KWDataPreparationStats::GetPreparedAttributeNumber() const
{
	if (preparedDataGridStats == NULL)
		return 0;
	else
		return preparedDataGridStats->GetPredictionAttributeNumber();
}

void KWDataPreparationStats::SetLevel(double dValue)
{
	require(dValue >= 0);
	dPreparedLevel = dValue;
}

double KWDataPreparationStats::GetLevel() const
{
	return dPreparedLevel;
}

boolean KWDataPreparationStats::IsInformative() const
{
	return GetSortValue() > 0;
}

void KWDataPreparationStats::SetConstructionCost(double dValue)
{
	require(dValue >= 0);
	dConstructionCost = dValue;
	if (dConstructionCost < dEpsilonCost)
		dConstructionCost = 0;
}

double KWDataPreparationStats::GetConstructionCost() const
{
	return dConstructionCost;
}

void KWDataPreparationStats::SetPreparationCost(double dValue)
{
	require(dValue >= 0);
	dPreparationCost = dValue;
	if (dPreparationCost < dEpsilonCost)
		dPreparationCost = 0;
}

double KWDataPreparationStats::GetPreparationCost() const
{
	return dPreparationCost;
}

double KWDataPreparationStats::GetModelCost() const
{
	return dConstructionCost + dPreparationCost;
}

void KWDataPreparationStats::SetDataCost(double dValue)
{
	require(dValue >= 0);
	dDataCost = dValue;
	if (dDataCost < dEpsilonCost)
		dDataCost = 0;
}

double KWDataPreparationStats::GetDataCost() const
{
	return dDataCost;
}

double KWDataPreparationStats::GetTotalCost() const
{
	return GetModelCost() + dDataCost;
}

void KWDataPreparationStats::ComputeLevel()
{
	require(dPreparedLevel == 0);

	// Arret si pas de cout MODL
	if (dPreparationCost == 0 or GetNullCost() == 0)
		return;
	// Calcul sinon
	else
	{
		// Calcul du Level
		dPreparedLevel = 1 - GetTotalCost() / GetNullCost();

		// Mise a 0 si grille nulle ou une seul partie explicative
		if (preparedDataGridStats == NULL)
			dPreparedLevel = 0;
		else if (preparedDataGridStats->ComputeSourceInformativeAttributeNumber() == 0)
			dPreparedLevel = 0;

		// On met a zero si negatif (ou tres proche de zero)
		if (dPreparedLevel < dEpsilonCost)
		{
			dPreparedLevel = 0;

			// On met a 0 les cout de preparation et de donnees
			SetConstructionCost(GetNullConstructionCost());
			SetPreparationCost(GetNullPreparationCost());
			SetDataCost(GetNullDataCost());

			// Destruction de la grille si necessaire
			if (preparedDataGridStats != NULL)
				delete preparedDataGridStats;
			preparedDataGridStats = NULL;
		}
	}
}

boolean KWDataPreparationStats::GetWriteCosts() const
{
	return true;
}

longint KWDataPreparationStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDataPreparationStats);
	if (preparedDataGridStats != NULL)
		lUsedMemory += preparedDataGridStats->GetUsedMemory();
	return lUsedMemory;
}

double KWDataPreparationStats::GetSortValue() const
{
	require(IsStatsComputed());

	if (preparedDataGridStats != NULL)
		return GetLevel();
	else
		return 0;
}

void KWDataPreparationStats::CleanDataPreparationResults()
{
	if (preparedDataGridStats != NULL)
		delete preparedDataGridStats;
	preparedDataGridStats = NULL;
	dPreparedLevel = 0;
	dConstructionCost = 0;
	dPreparationCost = 0;
	dDataCost = 0;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_LearningReport

PLShared_LearningReport::PLShared_LearningReport() {}

PLShared_LearningReport::~PLShared_LearningReport() {}

void PLShared_LearningReport::SetLearningReport(KWLearningReport* LearningReport)
{
	require(LearningReport != NULL);
	SetObject(LearningReport);
}

KWLearningReport* PLShared_LearningReport::GetLearningReport()
{
	return cast(KWLearningReport*, GetObject());
}

void PLShared_LearningReport::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWLearningReport* learningReport;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	learningReport = cast(KWLearningReport*, o);
	serializer->PutBoolean(learningReport->bIsStatsComputed);
	serializer->PutString(learningReport->sIdentifier);
}

void PLShared_LearningReport::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWLearningReport* learningReport;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	learningReport = cast(KWLearningReport*, o);
	learningReport->bIsStatsComputed = serializer->GetBoolean();
	learningReport->sIdentifier = serializer->GetString();
}

Object* PLShared_LearningReport::Create() const
{
	return new KWLearningReport;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DataPreparationStats

PLShared_DataPreparationStats::PLShared_DataPreparationStats() {}

PLShared_DataPreparationStats::~PLShared_DataPreparationStats() {}

void PLShared_DataPreparationStats::SetDataPreparationStats(KWDataPreparationStats* DataPreparationStats)
{
	require(DataPreparationStats != NULL);
	SetObject(DataPreparationStats);
}

KWDataPreparationStats* PLShared_DataPreparationStats::GetDataPreparationStats()
{
	return cast(KWDataPreparationStats*, GetObject());
}

void PLShared_DataPreparationStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDataPreparationStats* dataPreparationStats;
	PLShared_DataGridStats shared_dataGrid;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Appel de la methode ancetre
	PLShared_LearningReport::SerializeObject(serializer, o);

	// Serialisation specifique
	dataPreparationStats = cast(KWDataPreparationStats*, o);
	AddNull(serializer, dataPreparationStats->preparedDataGridStats);
	if (dataPreparationStats->preparedDataGridStats != NULL)
		shared_dataGrid.SerializeObject(serializer, dataPreparationStats->preparedDataGridStats);
	serializer->PutDouble(dataPreparationStats->dPreparedLevel);
	serializer->PutDouble(dataPreparationStats->dPreparationCost);
	serializer->PutDouble(dataPreparationStats->dConstructionCost);
	serializer->PutDouble(dataPreparationStats->dDataCost);
}

void PLShared_DataPreparationStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDataPreparationStats* dataPreparationStats;
	PLShared_DataGridStats shared_dataGrid;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Appel de la methode ancetre
	PLShared_LearningReport::DeserializeObject(serializer, o);

	// Deserialisation specifique
	dataPreparationStats = cast(KWDataPreparationStats*, o);
	assert(dataPreparationStats->preparedDataGridStats == NULL);
	if (not GetNull(serializer))
	{
		dataPreparationStats->preparedDataGridStats = new KWDataGridStats;
		shared_dataGrid.DeserializeObject(serializer, dataPreparationStats->preparedDataGridStats);
	}
	dataPreparationStats->dPreparedLevel = serializer->GetDouble();
	dataPreparationStats->dPreparationCost = serializer->GetDouble();
	dataPreparationStats->dConstructionCost = serializer->GetDouble();
	dataPreparationStats->dDataCost = serializer->GetDouble();
}
