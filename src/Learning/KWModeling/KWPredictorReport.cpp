// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorReport.h"

////////////////////////////////////////////////////////////////////////////////////
// Class KWPredictorReport

KWPredictorReport::KWPredictorReport()
{
	nUsedAttributeNumber = 0;
}

KWPredictorReport::~KWPredictorReport() {}

void KWPredictorReport::SetPredictorName(const ALString& sValue)
{
	sPredictorName = sValue;
}

const ALString& KWPredictorReport::GetPredictorName() const
{
	return sPredictorName;
}

void KWPredictorReport::SetUsedAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nUsedAttributeNumber = nValue;
}

int KWPredictorReport::GetUsedAttributeNumber() const
{
	return nUsedAttributeNumber;
}

void KWPredictorReport::WriteFullReportFile(const ALString& sFileName, ObjectArray* oaTrainReports)
{
	fstream ost;
	boolean bOk;
	ALString sLocalFileName;

	require(oaTrainReports != NULL);

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " Write report Begin");

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);

	// Ouverture du fichier local
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, ost);
	if (bOk)
	{
		if (GetLearningReportHeaderLine() != "")
			ost << GetLearningReportHeaderLine() << "\n";
		WriteFullReport(ost, oaTrainReports);
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
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " Write report End");
}

void KWPredictorReport::WriteFullReport(ostream& ost, ObjectArray* oaTrainReports)
{
	KWPredictorReport* firstReport;

	require(oaTrainReports != NULL);
	require(CheckTrainReports(oaTrainReports));

	// Acces a la premiere evaluation
	firstReport = NULL;
	if (oaTrainReports->GetSize() > 0)
		firstReport = cast(KWPredictorReport*, oaTrainReports->GetAt(0));

	// Titre et caracteristiques de la base d'evaluation
	ost << "Modeling report"
	    << "\n";
	ost << "\n";
	if (firstReport != NULL)
	{
		ost << "Dictionary"
		    << "\t" << TSV::Export(firstReport->GetClass()->GetName()) << "\n";
		if (firstReport->GetTargetAttributeName() != "")
			ost << "Target variable"
			    << "\t" << TSV::Export(firstReport->GetTargetAttributeName()) << "\n";
		ost << "Database\t" << TSV::Export(firstReport->GetDatabase()->GetDatabaseName()) << "\n";

		// Taux d'echantillonnage
		ost << "Sample percentage\t" << firstReport->GetDatabase()->GetSampleNumberPercentage() << "\n";
		ost << "Sampling mode\t" << firstReport->GetDatabase()->GetSamplingMode() << "\n";

		// Variable de selection
		ost << "Selection variable\t" << TSV::Export(firstReport->GetDatabase()->GetSelectionAttribute())
		    << "\n";
		ost << "Selection value\t" << TSV::Export(firstReport->GetDatabase()->GetSelectionValue()) << "\n";
	}

	// Tableau synthetique des predicteurs appris
	WriteArrayLineReport(ost, "Trained predictors", oaTrainReports);

	// Tableau detaille des performances des predicteurs
	WriteArrayReport(ost, "Trained predictors details", oaTrainReports);
}

void KWPredictorReport::WriteHeaderLineReport(ostream& ost)
{
	ost << "Predictor\tVariables";
}

void KWPredictorReport::WriteLineReport(ostream& ost)
{
	ost << TSV::Export(GetPredictorName()) << "\t";
	ost << GetUsedAttributeNumber();
}

void KWPredictorReport::WriteReport(ostream& ost)
{
	ost << "Predictor\t" << TSV::Export(GetPredictorName()) << "\n";
}

void KWPredictorReport::WriteJSONFullReportFields(JSONFile* fJSON, ObjectArray* oaTrainReports)
{
	KWPredictorReport* firstReport;

	require(oaTrainReports != NULL);
	require(CheckTrainReports(oaTrainReports));

	// Acces a la premiere evaluation
	firstReport = NULL;
	if (oaTrainReports->GetSize() > 0)
		firstReport = cast(KWPredictorReport*, oaTrainReports->GetAt(0));

	// Titre et caracteristiques de la base d'apprentissage
	fJSON->WriteKeyString("reportType", "Modeling");

	// Description du probleme d'apprentissage
	fJSON->BeginKeyObject("summary");
	fJSON->WriteKeyString("dictionary", firstReport->GetClass()->GetName());

	// Base de donnees
	firstReport->GetDatabase()->WriteJSONFields(fJSON);

	// Cas ou l'attribut cible n'est pas renseigne
	if (firstReport->GetTargetAttributeType() == KWType::None)
	{
		fJSON->WriteKeyString("learningTask", "Unsupervised analysis");
	}
	// Autres cas
	else
	{
		// Cas ou l'attribut cible est continu
		if (firstReport->GetTargetAttributeType() == KWType::Continuous)
			fJSON->WriteKeyString("learningTask", "Regression analysis");

		// Cas ou l'attribut cible est categoriel
		else if (firstReport->GetTargetAttributeType() == KWType::Symbol)
			fJSON->WriteKeyString("learningTask", "Classification analysis");
	}

	// Informations eventuelles sur l'attribut cible
	if (firstReport->GetTargetAttributeName() != "")
	{
		fJSON->WriteKeyString("targetVariable", firstReport->GetTargetAttributeName());
		if (firstReport->GetTargetAttributeType() == KWType::Symbol and
		    firstReport->GetMainTargetModalityIndex() != -1)
			fJSON->WriteKeyString("mainTargetValue", firstReport->GetMainTargetModality().GetValue());
	}

	// Fin de description du probleme d'apprentissage
	fJSON->EndObject();

	// Calcul des identifiants des rapports bases sur leur rang
	ComputeRankIdentifiers(oaTrainReports);

	// Tableau synthetique des performances des predicteurs
	WriteJSONArrayReport(fJSON, "trainedPredictors", oaTrainReports, true);

	// Tableau detaille des performances des predicteurs
	WriteJSONDictionaryReport(fJSON, "trainedPredictorsDetails", oaTrainReports, false);
}

void KWPredictorReport::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	const ALString sUnivariateFamily = "Univariate";
	require(fJSON != NULL);
	require(Check());

	// Donnees de base
	if (bSummary)
	{
		fJSON->WriteKeyString("rank", GetIdentifier());
		fJSON->WriteKeyString("type", KWType::GetPredictorLabel(GetTargetAttributeType()));
		if (GetPredictorName().Find(sUnivariateFamily) == 0)
			fJSON->WriteKeyString("family", sUnivariateFamily);
		else
			fJSON->WriteKeyString("family", GetPredictorName());
		fJSON->WriteKeyString("name", GetPredictorName());
		fJSON->WriteKeyInt("variables", GetUsedAttributeNumber());
	}
}

boolean KWPredictorReport::IsJSONReported(boolean bSummary) const
{
	// Pas de rapport detaille par defaut
	return bSummary;
}

boolean KWPredictorReport::CheckTrainReports(ObjectArray* oaTrainReports) const
{
	boolean bOk = true;
	KWPredictorReport* firstTrainReport;
	KWPredictorReport* TrainReport;
	int i;

	require(oaTrainReports != NULL);

	// Recherche du premier predicteur, servant de reference
	firstTrainReport = NULL;
	if (oaTrainReports->GetSize() > 0)
		firstTrainReport = cast(KWPredictorReport*, oaTrainReports->GetAt(0));

	// Comparaison des caracteristiques des evaluations a l'evaluation de reference
	for (i = 1; i < oaTrainReports->GetSize(); i++)
	{
		TrainReport = cast(KWPredictorReport*, oaTrainReports->GetAt(i));

		// Comparaison des caracteristiques
		assert(TrainReport->GetLearningSpec() == firstTrainReport->GetLearningSpec());
	}
	return bOk;
}

const ALString KWPredictorReport::GetSortName() const
{
	return sPredictorName;
}

const ALString KWPredictorReport::GetClassLabel() const
{
	return "Predictor report";
}

const ALString KWPredictorReport::GetObjectLabel() const
{
	return sPredictorName;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorSelectionReport

KWPredictorSelectionReport::KWPredictorSelectionReport() {}

KWPredictorSelectionReport::~KWPredictorSelectionReport()
{
	oaSelectedAttributes.DeleteAll();
}

ObjectArray* KWPredictorSelectionReport::GetSelectedAttributes()
{
	return &oaSelectedAttributes;
}

void KWPredictorSelectionReport::WriteReport(ostream& ost)
{
	KWSelectedAttributeReport* firstAttributeReport;

	// Appel de la methode ancetre
	KWPredictorReport::WriteReport(ost);

	// Variables selectionnees
	if (oaSelectedAttributes.GetSize() > 0)
	{
		firstAttributeReport = cast(KWSelectedAttributeReport*, oaSelectedAttributes.GetAt(0));
		firstAttributeReport->WriteArrayLineReport(ost, "Selected variables", &oaSelectedAttributes);
	}
}

void KWPredictorSelectionReport::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	ObjectArray oaSortedReports;
	KWSelectedAttributeReport* attributeReport;
	int i;
	KWSelectedAttributeReport* firstAttributeReport;

	// Appel de la methode ancetre
	KWPredictorReport::WriteJSONArrayFields(fJSON, bSummary);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaSelectedAttributes.GetSize(); i++)
	{
		attributeReport = cast(KWSelectedAttributeReport*, oaSelectedAttributes.GetAt(i));
		if (attributeReport->IsLineReported())
			oaSortedReports.Add(attributeReport);
	}

	// Variables selectionnees
	if (not bSummary and oaSelectedAttributes.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Ecriture du rapport
		firstAttributeReport = cast(KWSelectedAttributeReport*, oaSelectedAttributes.GetAt(0));
		firstAttributeReport->WriteJSONArrayReport(fJSON, "selectedVariables", &oaSelectedAttributes, bSummary);
	}
}

boolean KWPredictorSelectionReport::IsJSONReported(boolean bSummary) const
{
	boolean bIsReported;
	KWSelectedAttributeReport* attributeReport;
	int i;

	// Appel de la methode ancetre
	bIsReported = KWPredictorReport::IsJSONReported(bSummary);

	// Specialisation pour le rapport detaille
	if (not bSummary)
	{
		// Rapport si au moins un attribut doit etre affiche
		bIsReported = false;
		for (i = 0; i < oaSelectedAttributes.GetSize(); i++)
		{
			attributeReport = cast(KWSelectedAttributeReport*, oaSelectedAttributes.GetAt(i));
			if (attributeReport->GetImportance() > 0)
			{
				bIsReported = true;
				break;
			}
		}
	}
	return bIsReported;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWSelectedAttributeReport

KWSelectedAttributeReport::KWSelectedAttributeReport()
{
	dUnivariateEvaluation = 0;
	dWeight = -1;
}

KWSelectedAttributeReport::~KWSelectedAttributeReport() {}

void KWSelectedAttributeReport::SetPreparedAttributeName(const ALString& sName)
{
	sPreparedAttributeName = sName;
}

const ALString& KWSelectedAttributeReport::GetPreparedAttributeName() const
{
	return sPreparedAttributeName;
}

void KWSelectedAttributeReport::SetNativeAttributeName(const ALString& sName)
{
	sNativeAttributeName = sName;
}

const ALString& KWSelectedAttributeReport::GetNativeAttributeName() const
{
	return sNativeAttributeName;
}

void KWSelectedAttributeReport::SetUnivariateEvaluation(double dValue)
{
	require(0 <= dValue);
	dUnivariateEvaluation = dValue;
}

double KWSelectedAttributeReport::GetUnivariateEvaluation() const
{
	return dUnivariateEvaluation;
}

void KWSelectedAttributeReport::SetWeight(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dWeight = dValue;
}

double KWSelectedAttributeReport::GetWeight() const
{
	return dWeight;
}

double KWSelectedAttributeReport::GetImportance() const
{
	if (GetWeight() == -1)
		return -1;
	else
		return sqrt(GetUnivariateEvaluation() * GetWeight());
}

const ALString KWSelectedAttributeReport::GetSortName() const
{
	return sNativeAttributeName;
}

double KWSelectedAttributeReport::GetSortValue() const
{
	if (GetImportance() >= 0)
		return GetImportance();
	else
		return GetUnivariateEvaluation();
}

int KWSelectedAttributeReport::CompareValue(const KWLearningReport* otherReport) const
{
	int nCompare;
	KWSelectedAttributeReport* otherAttributeReport = cast(KWSelectedAttributeReport*, otherReport);
	longint lSortValue1;
	longint lSortValue2;

	// On se base sur un comparaison a dix decimales pres
	lSortValue1 = longint(floor(fabs(GetImportance()) * 1e10));
	lSortValue2 = longint(floor(fabs(otherAttributeReport->GetImportance()) * 1e10));
	assert(lSortValue1 >= 0);
	assert(lSortValue2 >= 0);
	nCompare = -CompareLongint(lSortValue1, lSortValue2);

	// En cas d'egalite, on se base sur l'evaluation univariee
	if (nCompare == 0)
	{
		lSortValue1 = longint(floor(fabs(GetUnivariateEvaluation()) * 1e10));
		lSortValue2 = longint(floor(fabs(otherAttributeReport->GetUnivariateEvaluation()) * 1e10));
		nCompare = -CompareLongint(lSortValue1, lSortValue2);
	}

	// En cas d'egalite, on se base sur le nom
	if (nCompare == 0)
		nCompare = GetSortName().Compare(otherReport->GetSortName());
	return nCompare;
}

void KWSelectedAttributeReport::WriteArrayLineReport(ostream& ost, const ALString& sTitle,
						     ObjectArray* oaLearningReports) const
{
	boolean bWriteUnivariateEvaluation;
	boolean bWriteWeight;
	ObjectArray oaSortedReports;
	int i;
	KWSelectedAttributeReport defaultAttributeReport;
	KWSelectedAttributeReport* attributeReport;

	require(oaLearningReports != NULL);

	// Par defaut, les champs facultatifs ne sont pas a ecrire
	bWriteUnivariateEvaluation = false;
	bWriteWeight = false;

	// On effectue une premiere passe pour determiner les champs ayant a afficher
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		attributeReport = cast(KWSelectedAttributeReport*, oaLearningReports->GetAt(i));

		// Les champs visualisables sont a mettre a jour des qu'il y a une valeur differente de la valeur par
		// defaut
		if (attributeReport->GetUnivariateEvaluation() != defaultAttributeReport.GetUnivariateEvaluation())
			bWriteUnivariateEvaluation = true;
		if (attributeReport->GetWeight() != defaultAttributeReport.GetWeight())
			bWriteWeight = true;
	}

	// Recherche des rapports a ecrire
	for (i = 0; i < oaLearningReports->GetSize(); i++)
	{
		attributeReport = cast(KWSelectedAttributeReport*, oaLearningReports->GetAt(i));
		if (attributeReport->IsLineReported())
			oaSortedReports.Add(attributeReport);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(KWLearningReportCompareSortValue);
		oaSortedReports.Sort();

		// Parcours des rapports
		if (sTitle != "")
			ost << "\n" << sTitle << "\n";
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			attributeReport = cast(KWSelectedAttributeReport*, oaLearningReports->GetAt(i));

			// Ligne d'entete
			if (i == 0)
			{
				ost << "Prepared name";
				ost << "\tName";
				if (bWriteUnivariateEvaluation)
					ost << "\tLevel";
				if (bWriteWeight)
					ost << "\tWeight\tImportance";
				ost << "\n";
			}

			// Ligne de de stats
			ost << TSV::Export(attributeReport->GetPreparedAttributeName());
			ost << "\t" << TSV::Export(attributeReport->GetNativeAttributeName());
			if (bWriteUnivariateEvaluation)
				ost << "\t" << attributeReport->GetUnivariateEvaluation();
			if (bWriteWeight)
				ost << "\t" << attributeReport->GetWeight() << "\t" << attributeReport->GetImportance();
			ost << "\n";
		}
	}
}

void KWSelectedAttributeReport::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Prepared name\tName\tLevel";
	if (GetWeight() > 0)
		ost << "\tWeight\tImportance";
}

void KWSelectedAttributeReport::WriteLineReport(ostream& ost) const
{
	ost << TSV::Export(sPreparedAttributeName) << "\t" << TSV::Export(sNativeAttributeName) << "\t"
	    << dUnivariateEvaluation;
	if (GetWeight() > 0)
	{
		ost << "\t" << GetWeight();
		ost << "\t" << GetImportance();
	}
}

void KWSelectedAttributeReport::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	if (not bSummary)
	{
		fJSON->WriteKeyString("preparedName", sPreparedAttributeName);
		fJSON->WriteKeyString("name", sNativeAttributeName);
		fJSON->WriteKeyDouble("level", dUnivariateEvaluation);
		if (GetWeight() > 0)
		{
			fJSON->WriteKeyDouble("weight", GetWeight());
			fJSON->WriteKeyDouble("importance", GetImportance());
		}
	}
}
