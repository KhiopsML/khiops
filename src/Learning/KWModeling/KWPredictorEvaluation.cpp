// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorEvaluation.h"

////////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluation

KWPredictorEvaluation::KWPredictorEvaluation()
{
	lInstanceEvaluationNumber = 0;
}

KWPredictorEvaluation::~KWPredictorEvaluation() {}

int KWPredictorEvaluation::GetTargetType() const
{
	return KWType::None;
}

void KWPredictorEvaluation::Evaluate(KWPredictor* predictor, KWDatabase* database)
{
	boolean bOk = true;
	KWTrainedPredictor* trainedPredictor;
	KWLearningSpec currentLearningSpec;
	KWClassDomain* currentDomain;
	KWClassDomain* evaluationDomain;
	KWDatabase* evaluationDatabase;
	KWPredictorEvaluationTask* predictorEvaluationTask;

	require(predictor != NULL);
	require(database != NULL);
	require(predictor->IsTrained());
	require(KWType::IsPredictorType(predictor->GetTargetAttributeType()));
	require(database->GetObjects()->GetSize() == 0);

	// Initialisation des criteres d'evaluation
	InitializeCriteria();

	// Memorisation du contexte d'evaluation
	sPredictorName = predictor->GetObjectLabel();
	evaluationDatabaseSpec.CopyFrom(database);
	SetLearningSpec(predictor->GetLearningSpec());

	// Personnalisation du dictionnaire de deploiement pour l'evaluation
	trainedPredictor = predictor->GetTrainedPredictor();
	trainedPredictor->PrepareDeploymentClass(true, true);

	// Changement du LearningSpec courant a celui du predicteur
	currentLearningSpec.CopyFrom(predictor->GetLearningSpec());

	// Mise en place du domaine d'evaluation du predicteur et compilation
	currentDomain = KWClassDomain::GetCurrentDomain();
	evaluationDomain = trainedPredictor->GetPredictorDomain();
	if (evaluationDomain != currentDomain)
	{
		evaluationDomain->SetName("Evaluation");
		KWClassDomain::SetCurrentDomain(evaluationDomain);
	}
	evaluationDomain->Compile();

	// Clonage de la base d'evaluation, pour ne pas interagir avec les spec d'apprentissage en cours
	evaluationDatabase = database->Clone();
	evaluationDatabase->SetClassName(trainedPredictor->GetPredictorClass()->GetName());

	// Parametrage de la base et de la classe d'evaluation
	predictor->GetLearningSpec()->SetDatabase(evaluationDatabase);
	predictor->GetLearningSpec()->SetClass(trainedPredictor->GetPredictorClass());

	// Lancement de la tache d'evaluation sous-traitante
	// Lors de la execution de la methode Evaluate de la tache el ecrit les resultats
	// directement dans l'objet courant car elle a ete declaree en tant que "friend"
	predictorEvaluationTask = CreatePredictorEvaluationTask();
	bOk = predictorEvaluationTask->Evaluate(predictor, evaluationDatabase, this);

	// Restitution de l'etat initial
	predictor->GetLearningSpec()->CopyFrom(&currentLearningSpec);
	if (evaluationDomain != currentDomain)
		KWClassDomain::SetCurrentDomain(currentDomain);
	trainedPredictor->PrepareDeploymentClass(true, false);

	// Reinitialisation en cas d'echec
	if (bOk)
		bIsStatsComputed = true;
	else
		Initialize();

	// Nettoyage
	delete predictorEvaluationTask;
	delete evaluationDatabase;
}

void KWPredictorEvaluation::Initialize()
{
	KWDatabase emptyDatabase;

	bIsStatsComputed = false;
	sPredictorName = "";
	evaluationDatabaseSpec.CopyFrom(&emptyDatabase);
	InitializeCriteria();
}

void KWPredictorEvaluation::InitializeCriteria()
{
	lInstanceEvaluationNumber = 0;
}

const ALString& KWPredictorEvaluation::GetPredictorName() const
{
	return sPredictorName;
}

const ALString& KWPredictorEvaluation::GetDatabaseName() const
{
	return evaluationDatabaseSpec.GetDatabaseName();
}

longint KWPredictorEvaluation::GetEvaluationInstanceNumber() const
{
	return lInstanceEvaluationNumber;
}

void KWPredictorEvaluation::WriteFullReportFile(const ALString& sFileName, const ALString& sEvaluationLabel,
						ObjectArray* oaPredictorEvaluations)
{
	fstream ost;
	boolean bOk;
	ALString sLocalFileName;

	require(oaPredictorEvaluations != NULL);

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, ost);
	if (bOk)
	{
		if (GetLearningReportHeaderLine() != "")
			ost << GetLearningReportHeaderLine() << "\n";
		WriteFullReport(ost, sEvaluationLabel, oaPredictorEvaluations);
		bOk = FileService::CloseOutputFile(sLocalFileName, ost);

		// Destruction du rapport si erreur
		if (not bOk)
			FileService::RemoveFile(sLocalFileName);
	}
	if (bOk)
	{
		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);
	}
}

void KWPredictorEvaluation::WriteFullReport(ostream& ost, const ALString& sEvaluationLabel,
					    ObjectArray* oaPredictorEvaluations)
{
	ObjectArray oaSortedPredictorEvaluations;

	require(oaPredictorEvaluations != NULL);
	require(CheckPredictorEvaluations(oaPredictorEvaluations));

	// Titre et caracteristiques de la base d'evaluation
	ost << sEvaluationLabel << " ";
	ost << "evaluation report"
	    << "\n";
	ost << "\n";
	ost << "Dictionary"
	    << "\t" << GetClass()->GetName() << "\n";
	if (GetTargetAttributeName() != "")
	{
		ost << "Target variable"
		    << "\t" << KWType::ToString(GetTargetAttributeType()) << "\t" << GetTargetAttributeName() << "\n";
		if (GetMainTargetModalityIndex() >= 0)
			ost << "Main target value"
			    << "\t" << GetMainTargetModality() << "\n";
	}

	// Base de donnes
	ost << "Database\t" << evaluationDatabaseSpec.GetDatabaseName() << "\n";

	// Taux d'echantillonnage
	ost << "Sample percentage\t" << evaluationDatabaseSpec.GetSampleNumberPercentage() << "\n";
	ost << "Sampling mode\t" << evaluationDatabaseSpec.GetSamplingMode() << "\n";

	// Variable de selection
	ost << "Selection variable\t" << evaluationDatabaseSpec.GetSelectionAttribute() << "\n";
	ost << "Selection value\t" << evaluationDatabaseSpec.GetSelectionValue() << "\n";

	// Nombre d'instances
	ost << "Instances\t" << GetEvaluationInstanceNumber() << "\n";

	// Tableau synthetique des performances des predicteurs
	WriteArrayLineReport(ost, "Predictors performance", oaPredictorEvaluations);

	// Tableau detaille des performances des predicteurs
	WriteArrayReport(ost, "Predictors detailed performance", oaPredictorEvaluations);

	// Rapport sur les courbes de performance
	SelectPerformanceCurvesReport(oaPredictorEvaluations, &oaSortedPredictorEvaluations);
	if (oaSortedPredictorEvaluations.GetSize() > 0)
		WritePerformanceCurveReportArray(ost, &oaSortedPredictorEvaluations);
}

void KWPredictorEvaluation::SelectPerformanceCurvesReport(const ObjectArray* oaPredictorEvaluations,
							  ObjectArray* oaSortedPredictorEvaluations)
{
	KWPredictorEvaluation* predictorEvaluation;
	int i;

	require(oaPredictorEvaluations != NULL);
	require(oaSortedPredictorEvaluations != NULL);
	require(oaSortedPredictorEvaluations->GetSize() == 0);

	// Recherche des rapports a ecrire
	for (i = 0; i < oaPredictorEvaluations->GetSize(); i++)
	{
		predictorEvaluation = cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(i));
		if (predictorEvaluation->IsReported() and predictorEvaluation->IsPerformanceCurveReported())
			oaSortedPredictorEvaluations->Add(predictorEvaluation);
	}

	// Tri par importance
	oaSortedPredictorEvaluations->SetCompareFunction(KWLearningReportCompareSortValue);
	oaSortedPredictorEvaluations->Sort();
}

void KWPredictorEvaluation::WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations) {}

boolean KWPredictorEvaluation::IsPerformanceCurveReported() const
{
	return true;
}

boolean KWPredictorEvaluation::CheckPredictorEvaluations(ObjectArray* oaPredictorEvaluations) const
{
	boolean bOk = true;
	KWPredictorEvaluation* firstPredictorEvaluation;
	KWPredictorEvaluation* predictorEvaluation;
	int i;

	require(oaPredictorEvaluations != NULL);

	// Recherche de la premiere evaluation, servant de reference
	firstPredictorEvaluation = NULL;
	if (oaPredictorEvaluations->GetSize() > 0)
		firstPredictorEvaluation = cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(0));

	// Comparaison des caracteristiques des evaluations a l'evaluation de reference
	for (i = 1; i < oaPredictorEvaluations->GetSize(); i++)
	{
		predictorEvaluation = cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(i));

		// Comparaison des caracteristiques
		assert(predictorEvaluation->GetLearningSpec() == firstPredictorEvaluation->GetLearningSpec());
		if (predictorEvaluation->GetDatabaseName() != firstPredictorEvaluation->GetDatabaseName())
			bOk = false;

		// Pour le nombre d'instances, on tolere la cas d'un valeur non nulle et d'une valeur nulle,
		// correspondant a une erreur d'acces a une base
		if (predictorEvaluation->GetEvaluationInstanceNumber() !=
			firstPredictorEvaluation->GetEvaluationInstanceNumber() and
		    predictorEvaluation->GetEvaluationInstanceNumber() > 0 and
		    firstPredictorEvaluation->GetEvaluationInstanceNumber() > 0)
			bOk = false;
	}
	return bOk;
}

const ALString KWPredictorEvaluation::GetSortName() const
{
	return GetPredictorName();
}

void KWPredictorEvaluation::WriteJSONFullReportFields(JSONFile* fJSON, const ALString& sEvaluationLabel,
						      ObjectArray* oaPredictorEvaluations)
{
	ObjectArray oaSortedPredictorEvaluations;

	require(oaPredictorEvaluations != NULL);
	require(CheckPredictorEvaluations(oaPredictorEvaluations));

	// Titre et caracteristiques de la base d'evaluation
	fJSON->WriteKeyString("reportType", "Evaluation");
	fJSON->WriteKeyString("evaluationType", sEvaluationLabel);

	// Description du probleme d'apprentissage
	fJSON->BeginKeyObject("summary");
	fJSON->WriteKeyString("dictionary", GetClass()->GetName());

	// Base de donnees
	evaluationDatabaseSpec.WriteJSONFields(fJSON);
	fJSON->WriteKeyLongint("instances", GetEvaluationInstanceNumber());

	// Cas ou l'attribut cible n'est pas renseigne
	if (GetTargetAttributeType() == KWType::None)
	{
		fJSON->WriteKeyString("learningTask", "Unsupervised analysis");
	}
	// Autres cas
	else
	{
		// Cas ou l'attribut cible est continu
		if (GetTargetAttributeType() == KWType::Continuous)
			fJSON->WriteKeyString("learningTask", "Regression analysis");

		// Cas ou l'attribut cible est categoriel
		else if (GetTargetAttributeType() == KWType::Symbol)
			fJSON->WriteKeyString("learningTask", "Classification analysis");
	}

	// Informations eventuelles sur l'attribut cible
	if (GetTargetAttributeName() != "")
	{
		fJSON->WriteKeyString("targetVariable", GetTargetAttributeName());
		if (GetTargetAttributeType() == KWType::Symbol and GetMainTargetModalityIndex() != -1)
			fJSON->WriteKeyString("mainTargetValue", GetMainTargetModality().GetValue());
	}

	// Fin de description du probleme d'apprentissage
	fJSON->EndObject();

	// Calcul des identifiants des rapports bases sur leur rang
	ComputeRankIdentifiers(oaPredictorEvaluations);

	// Tableau synthetique des performances des predicteurs
	WriteJSONArrayReport(fJSON, "predictorsPerformance", oaPredictorEvaluations, true);

	// Tableau detaille des performances des predicteurs
	WriteJSONDictionaryReport(fJSON, "predictorsDetailedPerformance", oaPredictorEvaluations, false);

	// Rapport sur les courbes de performance
	SelectPerformanceCurvesReport(oaPredictorEvaluations, &oaSortedPredictorEvaluations);
	if (oaSortedPredictorEvaluations.GetSize() > 0)
		WriteJSONPerformanceCurveReportArray(fJSON, &oaSortedPredictorEvaluations);
}

void KWPredictorEvaluation::WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations)
{
}

const ALString KWPredictorEvaluation::GetClassLabel() const
{
	return "Evaluation";
}

const ALString KWPredictorEvaluation::GetObjectLabel() const
{
	return GetPredictorName();
}

int KWPredictorEvaluation::GetInstanceNumber() const
{
	// Cette methode ne doit pas etre appelee a ce niveau de l'hierarchie
	assert(false);
	return KWLearningService::GetInstanceNumber();
}

KWPredictorEvaluationTask* KWPredictorEvaluation::CreatePredictorEvaluationTask()
{
	return new KWPredictorEvaluationTask;
}

int KWPredictorEvaluation::GetMainTargetModalityIndex() const
{
	if (GetLearningSpec()->IsTargetStatsComputed())
		return KWLearningReport::GetMainTargetModalityIndex();
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierEvaluation

KWClassifierEvaluation::KWClassifierEvaluation()
{
	dAccuracy = 0;
	dBalancedAccuracy = 0;
	dMajorityAccuracy = 0;
	dTargetEntropy = 0;
	dAUC = 0;
	dCompressionRate = 0;
	nPredictorMainTargetModalityIndex = -1;
}

KWClassifierEvaluation::~KWClassifierEvaluation()
{
	oaAllLiftCurveValues.DeleteAll();
}

void KWClassifierEvaluation::Evaluate(KWPredictor* predictor, KWDatabase* database)
{
	int nTargetIndex;
	KWTrainedClassifier* classifier;

	// Memorisation des modalites cibles de l'index de la modalite cible principale
	classifier = predictor->GetTrainedClassifier();
	svTrainedTargetModalities.SetSize(classifier->GetTargetValueNumber());
	for (nTargetIndex = 0; nTargetIndex < classifier->GetTargetValueNumber(); nTargetIndex++)
	{
		svTrainedTargetModalities.SetAt(nTargetIndex, classifier->GetTargetValueAt(nTargetIndex));
		if (classifier->GetTargetValueAt(nTargetIndex) == predictor->GetMainTargetModality())
			nPredictorMainTargetModalityIndex = nTargetIndex;
	}

	// Appel a la methode ancetre
	KWPredictorEvaluation::Evaluate(predictor, database);
}

int KWClassifierEvaluation::GetTargetType() const
{
	return KWType::Symbol;
}

void KWClassifierEvaluation::InitializeCriteria()
{
	// Appel a la methode ancetre
	KWPredictorEvaluation::InitializeCriteria();

	// Initialisation des criteres courants
	dAccuracy = 0;
	dBalancedAccuracy = 0;
	dMajorityAccuracy = 0;
	dTargetEntropy = 0;
	dAUC = 0;
	dCompressionRate = 0;
	dgsConfusionMatrix.DeleteAll();
	oaAllLiftCurveValues.DeleteAll();
}

double KWClassifierEvaluation::GetAccuracy() const
{
	return dAccuracy;
}

double KWClassifierEvaluation::GetBalancedAccuracy() const
{
	return dBalancedAccuracy;
}

double KWClassifierEvaluation::GetMajorityAccuracy() const
{
	return dMajorityAccuracy;
}

double KWClassifierEvaluation::GetTargetEntropy() const
{
	return dTargetEntropy;
}

double KWClassifierEvaluation::GetAUC() const
{
	return dAUC;
}

double KWClassifierEvaluation::GetCompressionRate() const
{
	return dCompressionRate;
}

int KWClassifierEvaluation::GetPredictorTargetValueNumber() const
{
	return svTrainedTargetModalities.GetSize();
}

Symbol& KWClassifierEvaluation::GetPredictorTargetValueAt(int nTarget) const
{
	return svTrainedTargetModalities.GetAt(nTarget);
}

const KWDGSAttributeSymbolValues* KWClassifierEvaluation::GetPredictedModalities() const
{
	require(dgsConfusionMatrix.GetAttributeNumber() == 2);
	return cast(KWDGSAttributeSymbolValues*, dgsConfusionMatrix.GetAttributeAt(0));
}

const KWDGSAttributeSymbolValues* KWClassifierEvaluation::GetActualModalities() const
{
	require(dgsConfusionMatrix.GetAttributeNumber() == 2);
	return cast(KWDGSAttributeSymbolValues*, dgsConfusionMatrix.GetAttributeAt(1));
}

const KWDataGridStats* KWClassifierEvaluation::GetConfusionMatrix() const
{
	require(dgsConfusionMatrix.GetAttributeNumber() == 2);
	return &dgsConfusionMatrix;
}

int KWClassifierEvaluation::GetPartileNumber() const
{
	if (oaAllLiftCurveValues.GetSize() == 0)
		return 0;
	else
		return cast(DoubleVector*, oaAllLiftCurveValues.GetAt(0))->GetSize() - 1;
}

double KWClassifierEvaluation::GetSampleSizeRatioAt(int nIndex) const
{
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());

	return nIndex * 1.0 / GetPartileNumber();
}

int KWClassifierEvaluation::GetComputedLiftCurveNumber() const
{
	return oaAllLiftCurveValues.GetSize();
}

int KWClassifierEvaluation::GetPredictorTargetIndexAtLiftCurveIndex(int nLiftCurveIndex) const
{
	require(0 <= nLiftCurveIndex and nLiftCurveIndex < GetComputedLiftCurveNumber());

	if (nLiftCurveIndex == oaAllLiftCurveValues.GetSize() - 1 and
	    GetMainTargetModalityLiftIndex() == oaAllLiftCurveValues.GetSize() - 1)
		return GetMainTargetModalityIndex();
	else
		return nLiftCurveIndex;
}

int KWClassifierEvaluation::GetLiftCurveIndexAt(int nPredictorTarget) const
{
	boolean bIsMainTargetModalityAtEnd;

	require(0 <= nPredictorTarget and nPredictorTarget < GetPredictedModalities()->GetValueNumber());

	// On determine si la modalite cible principale a du etre mise en fin de tableau
	bIsMainTargetModalityAtEnd =
	    GetMainTargetModalityIndex() != -1 and GetMainTargetModalityIndex() >= oaAllLiftCurveValues.GetSize();

	// Si c'est la modalite cible principale
	if (nPredictorTarget == GetMainTargetModalityIndex())
	{
		if (bIsMainTargetModalityAtEnd)
			return oaAllLiftCurveValues.GetSize() - 1;
		else
			return nPredictorTarget;
	}
	// Si ce n'est pas pas de modalite cible principale
	else
	{
		// Si la place est prise par la modalite cible principale, on n'a pas pu calculer la courbe de lift
		if (bIsMainTargetModalityAtEnd and nPredictorTarget == oaAllLiftCurveValues.GetSize() - 1)
			return -1;
		else if (nPredictorTarget < oaAllLiftCurveValues.GetSize())
			return nPredictorTarget;
		else
			return -1;
	}
}

double KWClassifierEvaluation::GetRandomLiftAt(int nPredictorTarget, int nIndex) const
{
	require(GetLiftCurveIndexAt(nPredictorTarget) != -1);
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());

	return nIndex * 1.0 / GetPartileNumber();
}

double KWClassifierEvaluation::GetOptimalLiftAt(int nPredictorTarget, int nIndex) const
{
	double dTargetValueRatio;
	double dSampleSizeRatio;
	int nTargetIndex;

	require(GetLiftCurveIndexAt(nPredictorTarget) != -1);
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());
	require(dgsConfusionMatrix.ComputeGridFrequency() == GetEvaluationInstanceNumber());
	require(ivActualModalityFrequencies.GetSize() == GetActualModalities()->GetValueNumber());

	// Calcul de la proportion de l'echantillon
	dSampleSizeRatio = GetSampleSizeRatioAt(nIndex);

	// Calcul de la proportion de modalite cible
	nTargetIndex = GetActualModalities()->ComputeSymbolPartIndex(GetPredictorTargetValueAt(nPredictorTarget));
	dTargetValueRatio = 0;
	if (GetEvaluationInstanceNumber() > 0)
		dTargetValueRatio =
		    ivActualModalityFrequencies.GetAt(nTargetIndex) * 1.0 / GetEvaluationInstanceNumber();
	assert(dTargetValueRatio <= 1);

	// Retour du lift optimal
	if (dSampleSizeRatio >= dTargetValueRatio)
		return 1;
	else
		return dSampleSizeRatio / dTargetValueRatio;
}

double KWClassifierEvaluation::GetClassifierLiftAt(int nPredictorTarget, int nIndex) const
{
	int nLiftIndex;
	DoubleVector* dvLiftCurveValues;

	require(GetLiftCurveIndexAt(nPredictorTarget) != -1);
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());

	// Acces a la courbe de lift demandee
	nLiftIndex = GetLiftCurveIndexAt(nPredictorTarget);
	dvLiftCurveValues = cast(DoubleVector*, oaAllLiftCurveValues.GetAt(nLiftIndex));

	// Acces a la valeur de lift
	return dvLiftCurveValues->GetAt(nIndex);
}

void KWClassifierEvaluation::WriteConfusionMatrixReport(ostream& ost)
{
	int i;
	int iMax;
	int j;
	int jMax;
	IntVector ivPredictedModalityFrequencies;

	require(Check());
	require(dgsConfusionMatrix.ComputeGridFrequency() == GetEvaluationInstanceNumber());
	require(ivActualModalityFrequencies.GetSize() == GetActualModalities()->GetValueNumber());

	// Titre
	ost << "Confusion matrix\n";

	// Calcul de l'index des dernieres modalites a prendre en compte: on ignore
	// la derniere modalite predite si elle est egale a la valeur
	// speciale StarValue et qu'elle est vide
	dgsConfusionMatrix.ExportAttributePartFrequenciesAt(0, &ivPredictedModalityFrequencies);
	iMax = GetPredictedModalities()->GetValueNumber() - 1;
	if (GetPredictedModalities()->GetValueAt(iMax) == Symbol::GetStarValue() and
	    ivPredictedModalityFrequencies.GetAt(iMax) == 0)
		iMax--;

	// Idem avec les modalite effectives (les effectifs sont deja calcules)
	jMax = GetActualModalities()->GetValueNumber() - 1;
	if (GetActualModalities()->GetValueAt(jMax) == Symbol::GetStarValue() and
	    ivActualModalityFrequencies.GetAt(jMax) == 0)
		jMax--;

	// Matrice de confusion
	for (j = 0; j <= jMax; j++)
		ost << "\t" << GetActualModalities()->GetValueAt(j);
	ost << "\n";
	for (i = 0; i <= iMax; i++)
	{
		ost << "$" << GetPredictedModalities()->GetValueAt(i);
		for (j = 0; j <= jMax; j++)
			ost << "\t" << GetConfusionMatrix()->GetBivariateCellFrequencyAt(i, j);
		ost << "\n";
	}
}

void KWClassifierEvaluation::WriteLiftCurveReport(ostream& ost)
{
	ObjectArray oaClassifierEvaluations;

	require(Check());
	require(IsPerformanceCurveReported());

	// Reutilisation de la methode avec un tableau d'evaluation en parametres
	oaClassifierEvaluations.Add(this);
	WriteLiftCurveReportArray(ost, &oaClassifierEvaluations);
}

void KWClassifierEvaluation::WriteLiftCurveReportArray(ostream& ost, ObjectArray* oaClassifierEvaluations)
{
	int nIndex;
	KWClassifierEvaluation* classifierEvaluation;
	int nClassifier;
	int n;
	int nPredictorTarget;

	require(Check());
	require(oaClassifierEvaluations != NULL);
	require(oaClassifierEvaluations->GetSize() == 0 or
		cast(KWClassifierEvaluation*, oaClassifierEvaluations->GetAt(0))->IsPerformanceCurveReported());

	// Affichage des courbes de lift
	for (n = 0; n < oaAllLiftCurveValues.GetSize(); n++)
	{
		// Recherche de l'index de valeur cible du predicteur
		nPredictorTarget = GetPredictorTargetIndexAtLiftCurveIndex(n);

		// Titre de la courbe base sur le nom de la modalite
		if (n > 0)
			ost << "\n";
		ost << "Lift curves\t" << GetPredictorTargetValueAt(nPredictorTarget) << "\n";

		// Ligne d'entete
		ost << "Size\tRandom\tOptimal";
		for (nClassifier = 0; nClassifier < oaClassifierEvaluations->GetSize(); nClassifier++)
		{
			classifierEvaluation =
			    cast(KWClassifierEvaluation*, oaClassifierEvaluations->GetAt(nClassifier));

			// Verification de coherence
			require(classifierEvaluation != NULL);
			require(classifierEvaluation->Check());
			require(classifierEvaluation->GetLearningSpec() == GetLearningSpec());
			require(classifierEvaluation->GetDatabaseName() == GetDatabaseName());
			require(classifierEvaluation->IsPerformanceCurveReported());
			require(classifierEvaluation->GetPartileNumber() == GetPartileNumber());
			require(classifierEvaluation->GetLiftCurveIndexAt(nPredictorTarget) != -1);

			// Libelle du classifier
			ost << "\t" << classifierEvaluation->GetPredictorName();
		}
		ost << "\n";

		// Donnees de la courbe de lift
		for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
		{
			// Donnees communes a tous les classifieurs
			ost << 100 * GetSampleSizeRatioAt(nIndex) << "\t"
			    << 100 * GetRandomLiftAt(nPredictorTarget, nIndex) << "\t"
			    << 100 * GetOptimalLiftAt(nPredictorTarget, nIndex);

			// Lift par classifier
			for (nClassifier = 0; nClassifier < oaClassifierEvaluations->GetSize(); nClassifier++)
			{
				classifierEvaluation =
				    cast(KWClassifierEvaluation*, oaClassifierEvaluations->GetAt(nClassifier));

				// Valeur du lift
				ost << "\t"
				    << 100 * classifierEvaluation->GetClassifierLiftAt(nPredictorTarget, nIndex);
			}
			ost << "\n";
		}
	}
}

void KWClassifierEvaluation::WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations)
{
	require(oaPredictorEvaluations != NULL);

	ost << "\n\n\n----------------------------------------------\n";
	WriteLiftCurveReportArray(ost, oaPredictorEvaluations);
}

boolean KWClassifierEvaluation::IsPerformanceCurveReported() const
{
	// Test si courbe de performance effectivement calculee
	return GetPartileNumber() > 0;
}

void KWClassifierEvaluation::WriteReport(ostream& ost)
{
	require(Check());

	// Donnees de base
	ost << "Classifier\t" << GetPredictorName() << "\n";
	ost << "Accuracy\t" << GetAccuracy() << "\n";
	ost << "Compression\t" << GetCompressionRate() << "\n";
	ost << "AUC\t" << GetAUC() << "\n";

	// Matrice de confusion
	ost << "\n";
	WriteConfusionMatrixReport(ost);
}

void KWClassifierEvaluation::WriteHeaderLineReport(ostream& ost)
{
	require(Check());
	ost << "Classifier\t";
	ost << "Accuracy\t";
	ost << "Compression\t";
	ost << "AUC";
}

void KWClassifierEvaluation::WriteLineReport(ostream& ost)
{
	require(Check());
	ost << GetPredictorName() << "\t";
	ost << GetAccuracy() << "\t";
	ost << GetCompressionRate() << "\t";
	ost << GetAUC();
}

void KWClassifierEvaluation::WriteJSONConfusionMatrixReport(JSONFile* fJSON)
{
	int i;
	int iMax;
	int j;
	int jMax;
	IntVector ivPredictedModalityFrequencies;

	require(fJSON != NULL);
	require(Check());
	require(dgsConfusionMatrix.ComputeGridFrequency() == GetEvaluationInstanceNumber());
	require(ivActualModalityFrequencies.GetSize() == GetActualModalities()->GetValueNumber());

	// Titre
	fJSON->BeginKeyObject("confusionMatrix");

	// Calcul de l'index des dernieres modalites a prendre en compte: on ignore
	// la derniere modalite predite si elle est egale a la valeur
	// speciale StarValue et qu'elle est vide
	dgsConfusionMatrix.ExportAttributePartFrequenciesAt(0, &ivPredictedModalityFrequencies);
	iMax = GetPredictedModalities()->GetValueNumber() - 1;
	if (GetPredictedModalities()->GetValueAt(iMax) == Symbol::GetStarValue() and
	    ivPredictedModalityFrequencies.GetAt(iMax) == 0)
		iMax--;

	// Idem avec les modalite effectives (les effectifs sont deja calcules)
	jMax = GetActualModalities()->GetValueNumber() - 1;
	if (GetActualModalities()->GetValueAt(jMax) == Symbol::GetStarValue() and
	    ivActualModalityFrequencies.GetAt(jMax) == 0)
		jMax--;

	// Matrice de confusion
	fJSON->BeginKeyList("values");
	for (j = 0; j <= jMax; j++)
		fJSON->WriteString(GetActualModalities()->GetValueAt(j).GetValue());
	fJSON->EndList();
	fJSON->BeginKeyArray("matrix");
	for (i = 0; i <= iMax; i++)
	{
		fJSON->BeginList();
		for (j = 0; j <= jMax; j++)
			fJSON->WriteInt(GetConfusionMatrix()->GetBivariateCellFrequencyAt(i, j));
		fJSON->EndList();
	}
	fJSON->EndArray();
	fJSON->EndObject();
}

void KWClassifierEvaluation::WriteJSONLiftCurveReportArray(JSONFile* fJSON, ObjectArray* oaClassifierEvaluations)
{
	int nIndex;
	KWClassifierEvaluation* classifierEvaluation;
	int nClassifier;
	int n;
	int nPredictorTarget;

	require(fJSON != NULL);
	require(Check());
	require(oaClassifierEvaluations != NULL);
	require(oaClassifierEvaluations->GetSize() == 0 or
		cast(KWClassifierEvaluation*, oaClassifierEvaluations->GetAt(0))->IsPerformanceCurveReported());

	// Affichage des courbes de lift
	fJSON->BeginKeyArray("liftCurves");
	for (n = 0; n < oaAllLiftCurveValues.GetSize(); n++)
	{
		// Recherche de l'index de valeur cible du predicteur
		nPredictorTarget = GetPredictorTargetIndexAtLiftCurveIndex(n);

		// Nom de la modalite
		fJSON->BeginObject();
		fJSON->WriteKeyString("targetValue", GetPredictorTargetValueAt(nPredictorTarget).GetValue());
		fJSON->BeginKeyArray("curves");

		// Courbe de lift optimale
		fJSON->BeginObject();
		fJSON->WriteKeyString("classifier", "Optimal");
		fJSON->BeginKeyList("values");
		for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
			fJSON->WriteDouble(100 * GetOptimalLiftAt(nPredictorTarget, nIndex));
		fJSON->EndList();
		fJSON->EndObject();

		// Courbe de lift par classifier
		for (nClassifier = 0; nClassifier < oaClassifierEvaluations->GetSize(); nClassifier++)
		{
			classifierEvaluation =
			    cast(KWClassifierEvaluation*, oaClassifierEvaluations->GetAt(nClassifier));

			fJSON->BeginObject();
			fJSON->WriteKeyString("classifier", classifierEvaluation->GetPredictorName());
			fJSON->BeginKeyList("values");
			for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
				fJSON->WriteDouble(100 *
						   classifierEvaluation->GetClassifierLiftAt(nPredictorTarget, nIndex));
			fJSON->EndList();
			fJSON->EndObject();
		}

		fJSON->EndArray();
		fJSON->EndObject();
	}
	fJSON->EndArray();
}

void KWClassifierEvaluation::WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations)
{
	require(fJSON != NULL);
	require(oaPredictorEvaluations != NULL);

	WriteJSONLiftCurveReportArray(fJSON, oaPredictorEvaluations);
}

void KWClassifierEvaluation::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
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
		fJSON->WriteKeyDouble("accuracy", GetAccuracy());
		fJSON->WriteKeyDouble("compression", GetCompressionRate());
		fJSON->WriteKeyDouble("auc", GetAUC());
	}
	// Donnees de detail
	else
		WriteJSONConfusionMatrixReport(fJSON);
}

boolean KWClassifierEvaluation::CheckEvaluation() const
{
	boolean bOk;
	DoubleVector* dvLiftCurveValues;
	int i;
	int n;

	// Verification de base
	bOk = KWLearningReport::Check();

	// Verification des tailles des donnees
	if (bOk)
		bOk = GetPredictedModalities()->GetValueNumber() > 1 and
		      GetPredictedModalities()->GetValueNumber() == GetActualModalities()->GetValueNumber();
	assert(GetActualModalities()->GetValueNumber() == ivActualModalityFrequencies.GetSize());

	// Verification des nombres d'instances
	if (bOk)
		bOk = GetConfusionMatrix()->ComputeGridFrequency() == GetEvaluationInstanceNumber();

	// Verification des courbes de lift
	if (bOk and GetPartileNumber() > 0)
	{
		// Parcours des courbes de lift
		for (n = 0; n < oaAllLiftCurveValues.GetSize(); n++)
		{
			dvLiftCurveValues = cast(DoubleVector*, oaAllLiftCurveValues.GetAt(n));

			// Verification de la coherence du nombre de partile utilises
			bOk = bOk and dvLiftCurveValues->GetSize() == GetPartileNumber() + 1;

			// Verification de la corube de lift
			bOk = bOk and dvLiftCurveValues->GetAt(0) == 0;
			bOk = bOk and dvLiftCurveValues->GetAt(GetPartileNumber()) == 1;
			for (i = 1; i < dvLiftCurveValues->GetSize(); i++)
			{
				bOk = bOk and dvLiftCurveValues->GetAt(i) >= 0;
				bOk = bOk and dvLiftCurveValues->GetAt(i) <= 1;
				bOk = bOk and dvLiftCurveValues->GetAt(i) >= dvLiftCurveValues->GetAt(i - 1);
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

KWPredictorEvaluationTask* KWClassifierEvaluation::CreatePredictorEvaluationTask()
{
	return new KWClassifierEvaluationTask;
}

int KWClassifierEvaluation::GetMainTargetModalityIndex() const
{
	return nPredictorMainTargetModalityIndex;
}

int KWClassifierEvaluation::GetMainTargetModalityLiftIndex() const
{
	if (GetMainTargetModalityIndex() == -1)
		return -1;
	else
	{
		// Si la modalite cible principale a un index dans les premiere courbe de lift
		if (GetMainTargetModalityIndex() < oaAllLiftCurveValues.GetSize())
			return GetMainTargetModalityIndex();
		// Sinon, c'est la dernier courbe de lift memorisee
		else
			return oaAllLiftCurveValues.GetSize() - 1;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWRegressorEvaluation

KWRegressorEvaluation::KWRegressorEvaluation()
{
	dRMSE = 0;
	dMAE = 0;
	dNLPD = 0;
	lTargetMissingValueNumber = 0;
	dRankRMSE = 0;
	dRankMAE = 0;
	dRankNLPD = 0;
}

KWRegressorEvaluation::~KWRegressorEvaluation() {}

int KWRegressorEvaluation::GetTargetType() const
{
	return KWType::Continuous;
}

void KWRegressorEvaluation::InitializeCriteria()
{
	// Appel a la methode ancetre
	KWPredictorEvaluation::InitializeCriteria();

	// Initialisation des criteres courants
	dRMSE = 0;
	dMAE = 0;
	dNLPD = 0;
	lTargetMissingValueNumber = 0;
	dRankRMSE = 0;
	dRankMAE = 0;
	dRankNLPD = 0;
	dvRankRECCurveValues.SetSize(0);
}

double KWRegressorEvaluation::GetRMSE() const
{
	return dRMSE;
}

double KWRegressorEvaluation::GetMAE() const
{
	return dMAE;
}

double KWRegressorEvaluation::GetNLPD() const
{
	return dNLPD;
}

longint KWRegressorEvaluation::GetTargetMissingValueNumber() const
{
	return lTargetMissingValueNumber;
}

double KWRegressorEvaluation::GetRankRMSE() const
{
	return dRankRMSE;
}

double KWRegressorEvaluation::GetRankMAE() const
{
	return dRankMAE;
}

double KWRegressorEvaluation::GetRankNLPD() const
{
	return dRankNLPD;
}

int KWRegressorEvaluation::GetPartileNumber() const
{
	if (dvRankRECCurveValues.GetSize() == 0)
		return 0;
	else
		return dvRankRECCurveValues.GetSize() - 1;
}

double KWRegressorEvaluation::GetSampleSizeRatioAt(int nIndex) const
{
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());

	return nIndex * 1.0 / GetPartileNumber();
}

double KWRegressorEvaluation::GetRankRECAt(int nIndex) const
{
	require(GetPartileNumber() > 0);
	require(0 <= nIndex and nIndex <= GetPartileNumber());
	require(Check());

	return dvRankRECCurveValues.GetAt(nIndex);
}

void KWRegressorEvaluation::WriteRankRECCurveReport(ostream& ost)
{
	int nIndex;

	require(Check());
	require(IsPerformanceCurveReported());

	// Ligne d'entete
	ost << "Size\t" << GetPredictorName() << "\n";

	// Donnees de la courbe de lift
	for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
	{
		ost << 100 * GetSampleSizeRatioAt(nIndex) << "\t" << 100 * GetRankRECAt(nIndex) << "\n";
	}
}

void KWRegressorEvaluation::WriteRankRECCurveReportArray(ostream& ost, ObjectArray* oaRegressorEvaluations)
{
	int nIndex;
	KWRegressorEvaluation* regressorEvaluation;
	int nRegressor;

	require(Check());
	require(oaRegressorEvaluations != NULL);
	require(oaRegressorEvaluations->GetSize() == 0 or
		cast(KWRegressorEvaluation*, oaRegressorEvaluations->GetAt(0))->IsPerformanceCurveReported());

	// Ligne d'entete
	ost << "Size";
	for (nRegressor = 0; nRegressor < oaRegressorEvaluations->GetSize(); nRegressor++)
	{
		regressorEvaluation = cast(KWRegressorEvaluation*, oaRegressorEvaluations->GetAt(nRegressor));

		// Verification de coherence
		require(regressorEvaluation != NULL);
		require(regressorEvaluation->Check());
		require(regressorEvaluation->GetLearningSpec() == GetLearningSpec());
		require(regressorEvaluation->GetDatabaseName() == GetDatabaseName());
		require(regressorEvaluation->IsPerformanceCurveReported());
		require(regressorEvaluation->GetPartileNumber() == GetPartileNumber());

		// Libelle du regressor
		ost << "\t" << regressorEvaluation->GetPredictorName();
	}
	ost << "\n";

	// Donnees de la courbe de REC
	for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
	{
		// Donnees communes a tous les regresseurs
		ost << 100 * GetSampleSizeRatioAt(nIndex);

		// RankREC par regresseur
		for (nRegressor = 0; nRegressor < oaRegressorEvaluations->GetSize(); nRegressor++)
		{
			regressorEvaluation = cast(KWRegressorEvaluation*, oaRegressorEvaluations->GetAt(nRegressor));

			// Valeur du REC
			ost << "\t" << 100 * regressorEvaluation->GetRankRECAt(nIndex);
		}
		ost << "\n";
	}
}

void KWRegressorEvaluation::WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations)
{
	require(oaPredictorEvaluations != NULL);

	ost << "\n\n\n----------------------------------------------\n";
	ost << "REC curves\n";
	WriteRankRECCurveReportArray(ost, oaPredictorEvaluations);
}

boolean KWRegressorEvaluation::IsPerformanceCurveReported() const
{
	// Test si courbe de performance effectivement calculee
	return GetPartileNumber() > 0;
}

void KWRegressorEvaluation::WriteReport(ostream& ost)
{
	require(Check());

	// Donnees de base
	ost << "Regressor\t" << GetPredictorName() << "\n";
	if (GetEvaluationInstanceNumber() > GetTargetMissingValueNumber())
	{
		ost << "RMSE\t" << GetRMSE() << "\n";
		ost << "MAE\t" << GetMAE() << "\n";
		ost << "NLPD\t" << GetNLPD() << "\n";
		ost << "RankRMSE\t" << GetRankRMSE() << "\n";
		ost << "RankMAE\t" << GetRankMAE() << "\n";
		ost << "RankNLPD\t" << GetRankNLPD() << "\n";
	}
}

void KWRegressorEvaluation::WriteHeaderLineReport(ostream& ost)
{
	require(Check());
	ost << "Regressor\tRMSE\tMAE\tNLPD\tRankRMSE\tRankMAE\tRankNLPD";
}

void KWRegressorEvaluation::WriteLineReport(ostream& ost)
{
	require(Check());
	ost << GetPredictorName() << "\t";
	if (GetEvaluationInstanceNumber() > 0)
	{
		ost << GetRMSE() << "\t" << GetMAE() << "\t" << GetNLPD() << "\t";
		ost << GetRankRMSE() << "\t" << GetRankMAE() << "\t" << GetRankNLPD();
	}
	else
		ost << "\t\t\t\t\t";
}

void KWRegressorEvaluation::WriteJSONRankRECCurveReportArray(JSONFile* fJSON, ObjectArray* oaRegressorEvaluations)
{
	int nIndex;
	KWRegressorEvaluation* regressorEvaluation;
	int nRegressor;

	require(Check());
	require(oaRegressorEvaluations != NULL);
	require(oaRegressorEvaluations->GetSize() == 0 or
		cast(KWRegressorEvaluation*, oaRegressorEvaluations->GetAt(0))->IsPerformanceCurveReported());

	// ECriture des courbes de REC
	fJSON->BeginKeyArray("recCurves");
	for (nRegressor = 0; nRegressor < oaRegressorEvaluations->GetSize(); nRegressor++)
	{
		regressorEvaluation = cast(KWRegressorEvaluation*, oaRegressorEvaluations->GetAt(nRegressor));

		// Verification de coherence
		require(regressorEvaluation != NULL);
		require(regressorEvaluation->Check());
		require(regressorEvaluation->GetLearningSpec() == GetLearningSpec());
		require(regressorEvaluation->GetDatabaseName() == GetDatabaseName());
		require(regressorEvaluation->IsPerformanceCurveReported());
		require(regressorEvaluation->GetPartileNumber() == GetPartileNumber());
		fJSON->BeginObject();

		// Ecriture de la courbe de REC
		fJSON->WriteKeyString("regressor", regressorEvaluation->GetPredictorName());
		fJSON->BeginKeyList("values");
		for (nIndex = 0; nIndex <= GetPartileNumber(); nIndex++)
			fJSON->WriteDouble(100 * regressorEvaluation->GetRankRECAt(nIndex));
		fJSON->EndList();
		fJSON->EndObject();
	}
	fJSON->EndArray();
}

void KWRegressorEvaluation::WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations)
{
	require(fJSON != NULL);
	require(oaPredictorEvaluations != NULL);

	WriteJSONRankRECCurveReportArray(fJSON, oaPredictorEvaluations);
}

void KWRegressorEvaluation::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
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
		if (GetEvaluationInstanceNumber() > 0)
		{
			fJSON->WriteKeyDouble("rmse", GetRMSE());
			fJSON->WriteKeyDouble("mae", GetMAE());
			fJSON->WriteKeyDouble("nlpd", GetNLPD());
			fJSON->WriteKeyDouble("rankRmse", GetRankRMSE());
			fJSON->WriteKeyDouble("rankMae", GetRankMAE());
			fJSON->WriteKeyDouble("rankNlpd", GetRankNLPD());
		}
		else
		{
			fJSON->WriteKeyNull("rmse");
			fJSON->WriteKeyNull("mae");
			fJSON->WriteKeyNull("nlpd");
			fJSON->WriteKeyNull("rankRmse");
			fJSON->WriteKeyNull("rankMae");
			fJSON->WriteKeyNull("rankNlpd");
		}
	}
}

boolean KWRegressorEvaluation::IsJSONReported(boolean bSummary) const
{
	return bSummary;
}

KWPredictorEvaluationTask* KWRegressorEvaluation::CreatePredictorEvaluationTask()
{
	return new KWRegressorEvaluationTask;
}
