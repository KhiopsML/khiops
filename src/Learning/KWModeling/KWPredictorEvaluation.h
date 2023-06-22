// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorEvaluation;
class KWClassifierEvaluation;
class KWRegressorEvaluation;

#include "KWLearningSpec.h"
#include "KWPredictor.h"
#include "KWDatabase.h"
#include "KWDataGridStats.h"
#include "KWPredictorEvaluationTask.h"

////////////////////////////////////////////////////////////////////////////////
// Service d'evaluation d'un predicteur sur une base de donnees
//
// Cette classe permettre de:
// - Declencher l'evaluation d'un predicteur sur un base de donnees via la methode Evaluate
// - Acces direct aux criteres obtenus lors d'une evaluation
// - Ecriture des rapports (format texte tabule ou json) d'une evaluation effectuee
class KWPredictorEvaluation : public KWLearningReport
{
public:
	// Constructeur
	KWPredictorEvaluation();
	~KWPredictorEvaluation();

	// Type de predicteur, a redefinir dans une sous-classe (defaut: KWType::None)
	//   KWType::Symbol: classification
	//   KWType::Continuous: regression
	//   KWType::None: non supervise
	virtual int GetTargetType() const;

	// Evaluation d'un predicteur sur une base
	virtual void Evaluate(KWPredictor* predictor, KWDatabase* database);

	// Reinitialisation de la classe
	virtual void Initialize();

	// Reinitialisation de tous les criteres
	virtual void InitializeCriteria();

	// Nom du predicteur
	const ALString& GetPredictorName() const;

	// Nom de la base d'evaluation
	const ALString& GetDatabaseName() const;

	// Nombre d'instances de la base d'evaluation
	// Ne pas confondre avec celui de la base d'apprentissage, accessible depuis GetInstanceNumber()
	longint GetEvaluationInstanceNumber() const;

	//////////////////////////////////////////////////////////////////////////////
	// Ecriture de rapports
	// Methodes supplementaires a celle de KWLearningReport

	// Rapport global: tabulaire, sur les courbes de performance, puis detaille
	// Le libelle d'evaluation vaut typiquement "Train" ou "Test"
	void WriteFullReportFile(const ALString& sFileName, const ALString& sEvaluationLabel,
				 ObjectArray* oaPredictorEvaluations);

	virtual void WriteFullReport(ostream& ost, const ALString& sEvaluationLabel,
				     ObjectArray* oaPredictorEvaluations);

	// Selection et tri des rapports ayant des courbes de performance
	// Memoire: le tableau en entree et son contenu appartiennent a l'appelant,
	// le tableau en sortie egalement, son contenu etant une partie du tableau en entree
	void SelectPerformanceCurvesReport(const ObjectArray* oaPredictorEvaluations,
					   ObjectArray* oaSortedPredictorEvaluations);

	// Rapport sur les courbes de performance
	virtual void WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations);

	// Flag de prise en compte de courbes de performance (defaut: false)
	virtual boolean IsPerformanceCurveReported() const;

	// Verification de la coherence d'un ensemble d'evaluation
	//  (meme base d'evaluation, meme type de prediction...)
	boolean CheckPredictorEvaluations(ObjectArray* oaPredictorEvaluations) const;

	// Criteres de tri permettant de trier differents objets d'un rapport
	const ALString GetSortName() const override;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture JSON du contenu d'un rapport global
	virtual void WriteJSONFullReportFields(JSONFile* fJSON, const ALString& sEvaluationLabel,
					       ObjectArray* oaPredictorEvaluations);

	// Rapport JSON sur les courbes de performance
	virtual void WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations);

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////
	// Le calcul de metriques d'evaluation est sous-traite a une tache parallele
	//
	// Cette dependance cree une double arborescence de classes : Chaque classe *Evaluation
	// a un tache parallele correspondant *EvaluationTask la quelle est
	// instancee avec la methode virtuelle protegee CreatePredictorEvaluationTask
	//
	// Les classes sous-traitantes *EvaluationTask sont considerees comme faisant partie
	// de l'implementation de cette classe et ses descendants. Donc elles sont declarees
	// en tant que "friend", pouvant ecrire directement sur des instances des classes *Evaluation

	// Cree un objet de tache parallele pour la sous-traitance de l'evaluation
	virtual KWPredictorEvaluationTask* CreatePredictorEvaluationTask();

	// Reimplementation de la methode ancetre, pour ou interdire l'acces
	// (et eviter la confusion avec GetEvaluationInstanceNumber())
	int GetInstanceNumber() const;

	// Reimplementation de la methode ancetre, pour permettre un calcul de l'index
	// par rapport a la matrice de confusion au cas ou les TargetStats ne sont pas calculables
	// (par exemple, pour l'evaluation differee d'un classifieur)
	int GetMainTargetModalityIndex() const override;

	// Nom du predicteur courant
	ALString sPredictorName;

	// Caracteristique de la base d'evaluation
	// Permet de memorise le nom et les criteres de selection
	KWDatabase evaluationDatabaseSpec;

	// Nombre d'instances: Le seul critere d'evaluation pour le predicteur de base
	longint lInstanceEvaluationNumber;

	// La classe d'evaluation sous-traitante ecrit directement sur une instance de cette classe
	friend class KWPredictorEvaluationTask;
};

////////////////////////////////////////////////////////////////////////////////
// Service d'evaluation d'un classifieur
class KWClassifierEvaluation : public KWPredictorEvaluation
{
public:
	// Constructeur
	KWClassifierEvaluation();
	~KWClassifierEvaluation();

	// Type de predicteur
	int GetTargetType() const override;

	// Evaluation d'un predicteur (classifieur) sur une base
	void Evaluate(KWPredictor* predictor, KWDatabase* database) override;

	// Reinitialisation de tous les criteres
	void InitializeCriteria() override;

	//////////////////////////////////////////////////////////////////////////////
	// Criteres d'evaluation

	// Taux de bonne prediction
	double GetAccuracy() const;

	// Taux de bonne prediction equilibre
	double GetBalancedAccuracy() const;

	// Taux de bonne prediction de la classe majoritaire
	double GetMajorityAccuracy() const;

	// Entropie cible
	double GetTargetEntropy() const;

	// Aire sous la courbe de ROC (Area Under Curve)
	double GetAUC() const;

	// Taux de compression (plus exactement: son complement a 1)
	//  CR = 1 - sum(-log(prob(Tj/X))) / N*entropy(Target)
	double GetCompressionRate() const;

	// Acces aux valeurs cibles identifies lors de l'apprentissage
	int GetPredictorTargetValueNumber() const;
	Symbol& GetPredictorTargetValueAt(int nTarget) const;

	// Matrice de confusion, definie par une grille de statistiques bidimensionnelles,
	// avec une liste de valeurs sources (modalites predites) et cibles (modalites cibles)
	// La valeur speciale StarValue est presente en fin des listes sources
	// et cible pour tenir compte des valeurs inconnues en apprentissage.
	// Attention :
	//   - La matrice est tronquee si elle trop grande (cf. KWClassifierConfusionMatrixEvaluation)
	//   - Les valeurs predits ou effectifs de la matrice de confusion ne sont pas
	//     necessairement aux memes index que les valeurs cibles du predicteur
	// Memoire : les objets appartiennent a l'appele, et peuvent etre modifies par l'appelant
	const KWDGSAttributeSymbolValues* GetPredictedModalities() const;
	const KWDGSAttributeSymbolValues* GetActualModalities() const;
	const KWDataGridStats* GetConfusionMatrix() const;

	// Acces aux courbes de lift
	int GetPartileNumber() const;
	double GetSampleSizeRatioAt(int nIndex) const;

	// Nombre de valeurs cibles pour lequelles la courbe de lift a ete calculee
	// La modalite cible principale si specifiee est toujours calculee
	int GetComputedLiftCurveNumber() const;

	// Index de valeur cible du predicteur pour un index de courbe de lift
	int GetPredictorTargetIndexAtLiftCurveIndex(int nLiftCurveIndex) const;

	// Index dans dans le tableau des courbes de lift pour un index de valeur cible du predicteur
	// Renvoie -1 si la courbe de lift correspondante n'a pas ete calculee
	int GetLiftCurveIndexAt(int nPredictorTarget) const;

	// Valeurs de lift par modalites cibles
	double GetRandomLiftAt(int nPredictorTarget, int nIndex) const;
	double GetOptimalLiftAt(int nPredictorTarget, int nIndex) const;
	double GetClassifierLiftAt(int nPredictorTarget, int nIndex) const;

	// Ecriture de la matrice de confusion et des courbes de lift (sans entete)
	void WriteConfusionMatrixReport(ostream& ost);
	void WriteLiftCurveReport(ostream& ost);

	// Ecriture d'une serie de courbes de lift sur un meme tableau de resultats
	// Les evaluations doivent etre compatibles (meme probleme evalue sur la
	// meme base de test, controle par rapport a l'objet evaluation en cours)
	// Toutes les courbes des predicteurs du tableau sont presentees
	// sur des colonnes successives, apres le lift aleatoire et le lift optimal
	void WriteLiftCurveReportArray(ostream& ost, ObjectArray* oaClassifierEvaluations);

	//////////////////////////////////////////////////////////////////////////////
	// Gestion du rapport general

	// Rapport sur les courbes de performance: redefinition des methodes generiques
	void WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations) override;
	boolean IsPerformanceCurveReported() const override;

	// Ecriture d'un rapport abrege
	void WriteReport(ostream& ost) override;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture de la matrice de confusion
	void WriteJSONConfusionMatrixReport(JSONFile* fJSON);

	// Ecriture d'une serie de courbe de lift sur un meme tableau de resultats
	// Le lift aleatoire n'est pas ecrit
	void WriteJSONLiftCurveReportArray(JSONFile* fJSON, ObjectArray* oaClassifierEvaluations);

	// Rapport sur les courbes de performance: redefinition des methodes generiques
	void WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations) override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) override;

	// Test d'integrite
	boolean CheckEvaluation() const;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Cree une instance de tache parallele pour la sous-traitance de l'evaluation du predicteur
	KWPredictorEvaluationTask* CreatePredictorEvaluationTask() override;

	// Reimplementation de la methode ancetre, pour permettre un calcul de l'index
	// par rapport a la matrice de confusion
	int GetMainTargetModalityIndex() const override;

	// Index de la courbe de lift de la modalite cible principale dans le tableau des courbes de lift
	int GetMainTargetModalityLiftIndex() const;

	// Index de la valeur cible principale pour le predicteur evalue
	// Lors d'une evaluation d'un predicteur deja calcule, il depend du predicteur,
	// et non des learning spec en cours
	int nPredictorMainTargetModalityIndex;

	// Nombre max de valeurs cible pour lesquelles on evalue la courbe de lift
	static const int nMaxLiftEvaluationNumber = 100;

	// Tableau de DoubleVector pour les courbes de lift evaluees
	// Si la valeur cible principale n'est dans les nMaxLiftEvaluationNumber premieres,
	// sa courbe est la derniere du tableau
	ObjectArray oaAllLiftCurveValues;

	// Criteres d'evaluation
	double dAccuracy;
	double dBalancedAccuracy;
	double dMajorityAccuracy;
	double dTargetEntropy;
	double dAUC;
	double dCompressionRate;
	SymbolVector svTrainedTargetModalities;
	KWDataGridStats dgsConfusionMatrix;
	IntVector ivActualModalityFrequencies;

	// La classe d'evaluation sous-traitant ecris directement sur une instance de cette classe
	friend class KWClassifierEvaluationTask;
};

///////////////////////////////////////////////////////////////////////////////
// Service d'evaluation d'un regresseur
class KWRegressorEvaluation : public KWPredictorEvaluation
{
public:
	// Constructeur
	KWRegressorEvaluation();
	~KWRegressorEvaluation();

	// Type de predicteur
	int GetTargetType() const override;

	// Reinitialisation de tous les criteres
	void InitializeCriteria() override;

	//////////////////////////////////////////////////////////////////////////////
	// Evaluation sur les valeurs

	// RMSE: Root Mean Square Error
	//   sqrt(1/N Sum_i (y - f(x)))
	double GetRMSE() const;

	// MAE: Mean Absolute Error
	//   1/N Sum_i |y - f(x)|
	double GetMAE() const;

	// NLPD: negative log predictive density
	//   - 1/N  Sum_i log(p(y_i/x_i))
	double GetNLPD() const;

	// Nombre de valeurs manquantes en test
	// En cas de valeur manquantes, les evaluations sur les rangs restent valides
	// En effet, en statistique d'ordre, on considere ici la valeur manquante comme ayant
	// un rang inferieur a celui de toutes les "vraies" valeurs.
	// Par contre, les evaluations sur les valeurs ne sont plus effectuee que sur une sous-partie des instances
	// Un warning est emis lors de l'evaluation si necessaire
	longint GetTargetMissingValueNumber() const;

	//////////////////////////////////////////////////////////////////////////////
	// Evaluation sur les rangs

	// RankRMSE: Root Mean Square Error
	//   sqrt(1/N Sum_i (r(y) - r(f(x))))
	double GetRankRMSE() const;

	// RankMAE: Mean Absolute Error
	//   1/N Sum_i |r(y) - r(f(x))|
	double GetRankMAE() const;

	// RankNLPD: negative log predictive density
	//   - 1/N  Sum_i log(p(r(y_i)/x_i))
	double GetRankNLPD() const;

	//////////////////////////////////////////////////////////////////////////////
	// Courbe de REC sur les rangs
	// Les instance en test sur trie avec erreur croissante
	// En abscise, on reporte la valeur de l'erreur (comrise entre  0 et 1
	// sur les rangs), et en ordonnee la proportion de l'echantillon telle
	// que l'erreur soit inferieur au seuil en abcisse

	// Acces aux caracteristique de la courbe de REC
	int GetPartileNumber() const;
	double GetSampleSizeRatioAt(int nIndex) const;
	double GetRankRECAt(int nIndex) const;

	// Ecriture de la courbe de REC
	void WriteRankRECCurveReport(ostream& ost);

	// Ecriture d'une serie de courbes de REC sur un meme tableau de resultats
	// Les evaluations doivent etre compatibles (meme probleme evalue sur la
	// meme base de test, controle par rapport a l'objet evaluation en cours))
	// Toutes les courbes de REC des predicteurs du tableau sont presentees
	// sur des colonnes successives
	void WriteRankRECCurveReportArray(ostream& ost, ObjectArray* oaRegressorEvaluations);

	//////////////////////////////////////////////////////////////////////////////
	// Gestion du rapport general

	// Rapport sur les courbes de performance: redefinition des methodes generiques
	void WritePerformanceCurveReportArray(ostream& ost, ObjectArray* oaPredictorEvaluations) override;
	boolean IsPerformanceCurveReported() const override;

	// Ecriture d'un rapport abrege
	void WriteReport(ostream& ost) override;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture d'une serie de courbe de lift sur un meme tableau de resultats
	// Le lift aleatoire n'est pas ecrit
	void WriteJSONRankRECCurveReportArray(JSONFile* fJSON, ObjectArray* oaRegressorEvaluations);

	// Rapport sur les courbes de performance: redefinition des methodes generiques
	void WriteJSONPerformanceCurveReportArray(JSONFile* fJSON, ObjectArray* oaPredictorEvaluations) override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Cree une objet de tache parallele pour la sous-traitance de l'evaluation
	KWPredictorEvaluationTask* CreatePredictorEvaluationTask() override;

	// Criteres d'evaluation
	double dRMSE;
	double dMAE;
	double dNLPD;
	longint lTargetMissingValueNumber;
	double dRankRMSE;
	double dRankMAE;
	double dRankNLPD;
	DoubleVector dvRankRECCurveValues;

	// La classe d'evaluation sous-traitante ecrit directement sur une instance de cette classe
	friend class KWRegressorEvaluationTask;
};
