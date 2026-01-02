// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorEvaluationTask;
class KWClassifierEvaluationTask;
class KWRegressorEvaluationTask;
class KWConfusionMatrixEvaluation;
class KWAucEvaluation;
class KWClassifierInstanceEvaluation;
class KWReservoirSampler;
class PLShared_ClassifierInstanceEvaluation;

#include "KWPredictor.h"
#include "KWPredictorEvaluation.h"
#include "KWDatabase.h"
#include "KWDatabaseTask.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Tache parallele d'evaluation d'un predicteur sur une base de donnees
// Classe algorithmique de service, dont les resultats sont destine a alimenter
// un rapports d'evaluation (cf hierarchie de classe de KWPredictorEvaluation)
class KWPredictorEvaluationTask : public KWDatabaseTask
{
public:
	// Constructeur
	KWPredictorEvaluationTask();
	~KWPredictorEvaluationTask();

	// Evaluation d'un predicteur sur une base.
	// Stockage de resultats sur l'objet mandataire KWPredictorEvaluation
	virtual boolean Evaluate(KWPredictor* predictor, KWDatabase* database,
				 KWPredictorEvaluation* predictorEvaluation);

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Initialisation et nettoyage des informations du predicteurs necessaires pour l'evaluation
	// Cettes variables sont partagees entre le maitre et les esclaves
	virtual void InitializePredictorSharedVariables(KWPredictor* predictor);
	virtual void CleanPredictorSharedVariables();

	// Calcul de l'index de chargement (LoadIndex) d'un attribut
	// Renvoie un index invalide si l'attribut est NULL (non obligatoire)
	KWLoadIndex GetLoadIndex(KWAttribute* predictionAttribute) const;

	// Reimplementation des etapes du DatabaseTask (methodes virtuelles)
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;

	// Objet d'evaluation demandeur de la tache et ou l'on stocke les resultats
	KWPredictorEvaluation* predictorEvaluation;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Tache parallele d'evaluation d'un classifieur sur une base de donnees
class KWClassifierEvaluationTask : public KWPredictorEvaluationTask
{
public:
	// Constructeur
	KWClassifierEvaluationTask();
	~KWClassifierEvaluationTask();

	///////////////////////////////////////////////////////////////////////////////
	////  Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////
	// Pour l'evaluation d'un classifieur, ll y a trois objets qui se aggregent dans chaque esclave:
	// - La matrice de confusion
	// - Les evaluations d'instances pour le calcul de l'AUC et courbes de lift
	// - La vraisemblasce totale
	//
	// La vraisemblance et la matrice de confusion sont associatives, et donc il suffit de calculer
	// ses versions locaux a chaque esclave et ensuite les aggreger dans le maitre.
	// Dans la finalisation la plupart des indicateurs s'en deduissent de la matrice de confusion.
	//
	// Pour l'AUC et les courbes de lift, la situation est un peu plus delicate car il faut avoir
	// l'integralite des evaluation d'instances pour pouvoir la calculer.
	// Si la base de donnes a une grande taille cela peut entrainer des soucis de memoire
	// et donc la paralellisation n'est pas trivial comme dans les cas precedantes.
	//
	// Donc, pour calculer l'AUC et les courbes dans chaque esclave on utilise un reservoir sampler et
	// dans chaque sous-tache on reenvoie:
	//    - L'echantillon de KWClassifierEvaluationInstance
	//    - Le nombre d'objets vus et la taille de l'echantillon
	// Le reservoir est reintialise a chaque sous-tache
	//
	// Le maitre pour sa part a egalement un reservoir sampler, mais aussi un taux d'echantillonnage.
	// Ce taux est intialement a 1 et :
	//   - Si l'esclave a echantillonne avec un taux plus petit que celui du maitre alors il
	//     reechantillone avec proba (proba esclave)/(proba maitre) et il rajoute l'integralite de
	//     l'echantillon de l'esclave
	//   - Si l'esclave a echantillonne avec un taux plus grande que celui du maitre alors il ajoute
	//     l'echantillon de l'esclave avec proba (proba maitre)/(proba esclave)
	//
	// A la fin, la collection (echantillonne ou non) de KWClassifierEvaluationInstance s'utilise pour
	// estimer l'AUC et les courbes de lift du classifieur

	// Reimplementation des methodes virtuelles (en gros les etapes) de DatabaseTask
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Initialisation des variables de travail lies au predicteur
	void InitializePredictorSharedVariables(KWPredictor* predictor) override;

	// Nettoyage des variables de travail lies au predicteur
	void CleanPredictorSharedVariables() override;

	// Index de la courbe de lift de la modalite cible principale dans le tableau des courbes de lift
	// TODO: Elle est necessaire, mais je soupconne que ca peut se simplifier
	int GetMainTargetModalityLiftIndex() const;

	// Nombre de valeurs cibles pour lequelles la courbe de lift a ete calculee
	// La modalite cible principale si specifiee est toujours calculee
	int GetComputedLiftCurveNumber() const;

	// Index de valeur cible du predicteur pour un index de courbe de lift
	int GetPredictorTargetIndexAtLiftCurveIndex(int nLiftCurveIndex) const;

	// Calcul de la capacite d'un echantillonneur en fonction de la memoire disponible
	int ComputeSamplerCapacity(longint lMemory, int nTargetValueNumber) const;

	// Calcul de la memoire attribuee exclusivement a la tache (exclue celle de la classe ancetre)
	longint ComputeTaskSelfMemory(longint lTaskGrantedMemory, RMPhysicalResource* taskMemoryRequirement,
				      RMPhysicalResource* parentTaskMemoryRequirement) const;

	// Calcul de la taille memoire d'une evaluation d'instance
	longint ComputeInstanceEvaluationNecessaryMemory(int nTargetValueNumber) const;

	// Mise a jour de l'echantillonneur du maitre
	void MasterAggregateResultsUpdateSampler();

	// Nombre max de valeurs cible pour lesquelles on evalue la courbe de lift
	static const int nMaxLiftEvaluationNumber = 100;

	// Reference a l'objet mandataire de l'evaluation: les resultats sont stockes ici
	KWClassifierEvaluation* classifierEvaluation;

	//////////////////////////////////////////////////////////////////////////////
	// Variables du maitre

	// Service d'evaluation des matrices de confusion du maitre (global)
	KWConfusionMatrixEvaluation* masterConfMatrixEvaluation;

	// Services d'evaluation de l'AUC et des courbes de lift
	KWAucEvaluation* masterAucEvaluation;

	// Echantilloneur d'instances d'evaluation du maitre (pour le calcul de l'AUC)
	// Attention, la taille du reservoir (Capacity) est correcte, mais le nombre d'objet traites (SeenObjectNumber)
	// peut etre inferieur au nombre total d'objet vues par les esclaves, si un de ceux-ci a sature
	KWReservoirSampler* masterInstanceEvaluationSampler;

	// Probabilite d'echantillonnage utilise dans le reservoir du maitre, qui peut etre struictement inferieure a 1
	// si un esclave passe etait sature, et que le maitre l'a integre sans toutefois remplir entierement son
	// propre reservoir: il garde alors une proba strictement inferieure a 1, bien que n'etant pas plein
	double dMasterSamplingProb;

	// Tableau des echantillons des esclaves (contiennent des tableaux de KWClassifierInstanceEvaluation).
	// Il est indexe par le index de sous-tache (GetTaskIndex()). Il est necessaire pour parcourir les
	// sorties de sous-taches dans le meme ordre en sequentiel et ainsi qu'en parallele. Pour ce-faire
	// on accumule les echantillons des esclaves de maniere provisoire dans ce tableau (dans la
	// position donnee par GetTaskIndex()) et on les ajoute a l'echantillon du maitre au fur et a
	// mesure que l'on peut faire dans l'ordre de sous-taches.
	// NB: La probabilite que ce tableau soit rempli est negligeable (eg. la sous-tache 0 ne retourne jamais)
	ObjectArray oaAllInstanceEvaluationSamples;

	// Nombre total d'objets vus pour la constitution des echantillons de chaque esclave
	LongintVector lvSeenInstanceEvaluationNumberPerSample;

	// Indice du prochain echantillon issu d'un esclave a ajouter a l'echantillon du maitre
	int nCurrentSample;

	// Indique si la tache calcule l'AUC
	boolean bIsAucEvaluated;

	// Exigences de memoire de classe ancetre (PLDatabaseTask)
	RMPhysicalResource databaseTaskMasterMemoryRequirement;
	RMPhysicalResource databaseTaskSlaveMemoryRequirement;

	//////////////////////////////////////////////////////////////////////////////
	// Variables de l'esclave

	// Echantilloneur d'instances d'evaluation du esclaves (pour le calcul de l'AUC)
	KWReservoirSampler* slaveInstanceEvaluationSampler;

	// Service d'evaluation des matrices de confusion des esclaves (local)
	KWConfusionMatrixEvaluation* slaveConfusionMatrixEvaluation;

	//////////////////////////////////////////////////////////////////////////////
	// Inputs et outputs des esclaves

	// Signale a l'esclave s'il doit collecter les evaluation d'instances pour le calcul d'AUC
	PLShared_Boolean input_bIsAucEvaluated;

	// Matrice de confusion de sortie d'un esclave sous forme de DataGridStat
	PLShared_DataGridStats output_confusionMatrix;

	// Ratio de compression de sortie d'un esclave
	PLShared_Double output_dCompressionRate;

	// Instances d'evaluation de l'esclave. Potentiellement sous-echantillonees
	PLShared_ObjectArray* output_oaInstanceEvaluationSample;

	// Nombre d'instances d'evaluation vus par l'esclave dans la sous-tache
	PLShared_Longint output_lSeenInstanceEvaluationNumber;

	//////////////////////////////////////////////////////////////////////////////
	// Variables partagees

	// Index de chargement du attribut cible
	PLShared_LoadIndex shared_liTargetAttribute;

	// Index de chargement du attribut de prediction
	PLShared_LoadIndex shared_liPredictionAttribute;

	// Index de chargement des probabilites
	PLShared_LoadIndexVector shared_livProbAttributes;

	// Nombre de valeurs/modalites predites du attribut cible
	PLShared_Int shared_nTargetValueNumber;

	// Modalites predites du attribut cible
	PLShared_SymbolVector shared_svPredictedModalities;

	// Taille maximale du tableau d'instances d'evaluation du esclave
	PLShared_Int shared_nSlaveInstanceEvaluationCapacity;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Tache parallele d'evaluation d'un regresseur sur une base de donnees
class KWRegressorEvaluationTask : public KWPredictorEvaluationTask
{
public:
	// Constructeur
	KWRegressorEvaluationTask();
	~KWRegressorEvaluationTask();

	//////////////////////////////////////////////////////////////////////////////
	////  Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////
	// L'evaluation d'un regresseur est "embarrasingly parallel" pour tous les criteres associatifs
	// Dans ce cas il suffit de les aggreger dans esclaves et ensuite dans le maitre.
	//
	// Le seul critere non associatif est la courbe de REC. Pour la calculer on prends
	// une mesure plus simple que pour le calcul d'AUC d'un classifieur :
	// Si on depasse la capacite on ne la calcule pas.

	// Implementation des methodes virtuelles de DatabaseTask
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Initialisation et nettoyage des variables de travail liees au prediteur
	void InitializePredictorSharedVariables(KWPredictor* predictor) override;
	void CleanPredictorSharedVariables() override;

	// Objet mandataire de la tache
	KWRegressorEvaluation* regressorEvaluation;

	/////////////////////////////////////////////////////////
	// Variables du maitre

	// Indique si la tache calcule la courbe de REC
	boolean bIsRecCurveCalculated;

	// Vecteur d'erreurs du maitre
	DoubleVector dvRankAbsoluteErrors;

	// Capacite maximale du vecteur d'erreurs, cote maitre et cote esclave
	int nMasterErrorVectorMaxCapacity;
	int nSlaveErrorVectorMaxCapacity;

	// Parametres partages
	PLShared_LoadIndex shared_liTargetAttribute;
	PLShared_LoadIndex shared_liMeanAttribute;
	PLShared_LoadIndex shared_liDensityAttribute;
	PLShared_LoadIndex shared_liTargetRankAttribute;
	PLShared_LoadIndex shared_liMeanRankAttribute;
	PLShared_LoadIndex shared_liRankDensityAttribute;

	// Variables en entre et sortie des esclaves
	PLShared_Boolean input_bIsRecCurveCalculated;
	PLShared_Int output_nTargetMissingValueNumber;
	PLShared_Double output_dRMSE;
	PLShared_Double output_dMAE;
	PLShared_Double output_dNLPD;
	PLShared_Double output_dRankRMSE;
	PLShared_Double output_dRankMAE;
	PLShared_Double output_dRankNLPD;
	PLShared_DoubleVector output_dvRankAbsoluteErrors;
	PLShared_Boolean output_bIsRecCurveVectorOverflowed;
	PLShared_Int output_nSlaveCapacityOverflowSize;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Evaluation online de la matrice de confusion d'un classifieur
class KWConfusionMatrixEvaluation : public Object
{
public:
	// Constructeur
	KWConfusionMatrixEvaluation();
	~KWConfusionMatrixEvaluation();

	// Initialisation/reinitialisation (zero instances)
	void Initialize();

	// Ajout d'une valeur predite pour initialiser la matrice de confusion
	void AddPredictedTarget(const Symbol& sPredictedTarget);

	// Ajout d'une evaluation d'instance a memoriser dans la matrice de confusion
	void AddInstanceEvaluation(const Symbol& sPredictedTarget, const Symbol& sActualTarget);

	// Ajout des instances d'une matrice de confusion sauvgardee dans un DataGridStats
	void AddEvaluatedMatrix(const KWDataGridStats* dgsConfusionMatrix);

	// Calcul des criteres de prediction a partir des instances d'evaluation
	double ComputeAccuracy() const;
	double ComputeBalancedAccuracy() const;
	double ComputeMajorityAccuracy() const;
	double ComputeTargetEntropy() const;

	// Mise a jour de la matrice de confusion dans la grille en parametre
	void ExportDataGridStats(KWDataGridStats* dgsConfusionMatrix) const;

	// Taille max de la matrice de confusion en nombre de modalites (defaut: 1000)
	// Pour de tres grandes tailles l'interet de la matrice est faible et pose problemes de memoire
	// Au dela de la taille max, la matrice de confusion est tronquee
	static int GetMaxSize();
	static void SetMaxSize(int nValue);

	// Test d'integrite
	boolean Check() const override;

	// Affichage de la matrice
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout d'une evaluation d'instances a memoriser dans la matrice de confusion
	void AddInstances(const Symbol& sPredictedTarget, const Symbol& sActualTarget, int nFrequency);

	// Calcul des effectifs par cellule, ligne et colonne
	int ComputeFrequencyAt(const Symbol& sPredictedTarget, const Symbol& sActualTarget) const;
	int ComputeCumulatedActualFrequencyAt(const Symbol& sActualTarget) const;
	int ComputeCumulatedPredictedFrequencyAt(const Symbol& sPredictedTarget) const;

	// Creation des modalites pour un DataGridStats (avec la StarValue a la fin si besoin)
	KWDGSAttributeSymbolValues* CreateModalities(SymbolVector* svModalities, const ALString& sAttributeName) const;

	// Modalites predites sous forme de vecteur
	SymbolVector svPredictedValues;

	// Modalites reelles sous forme de vecteur
	SymbolVector svActualValues;

	// Modalites predites de la matrice de confusion indexes par leur valeur (Symbol)
	NumericKeyDictionary nkdPredictedValues;

	// Modalites reelles de la matrice de confusion indexes par leur valeur (Symbol)
	NumericKeyDictionary nkdActualValues;

	// Effectifs de la matrice de confusion : Pour chaque valeur predite, on a un dictionnaire
	// de IntObject memorisant l'effectif et l'indexe par les valeurs reelles
	NumericKeyDictionary nkdPredictedValuesActualFreqs;
	ObjectArray oaConfusionMatrixFreqs;

	// Nombre d'effectifs totaux
	int nTotalFrequency;

	// Taille maximale de la matrice de confusion
	static int nMaxSize;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Service Evaluation de l'AUC d'un classifieur
class KWAucEvaluation : public Object
{
public:
	// Constructeur
	KWAucEvaluation();
	~KWAucEvaluation();

	// Initialisation des donnees necessaires au calcul de l'AUC
	void Initialize();

	// Memorisation du nombre de valeurs cibles (obligatoire, apres initialisation)
	void SetTargetValueNumber(int nValue);

	// Acces au nombre de valeurs cibles
	int GetTargetValueNumber() const;

	// Memorisation de l'addresse du tableau d'instances d'evaluation
	// Memoire: Responsabilite de l'appelant
	void SetInstanceEvaluations(ObjectArray* oaEvaluations);

	// Evaluation globale de l'AUC du predicteur sur l'ensemble des modalites
	// Utilisation de la methode precedante en ponderant par la frequence de chaque modalite cible
	// Dans le cas particulier ou il n'y a que deux modalites cibles la methode est applicable
	// meme si seul le premier vecteur de score a ete parametre
	double ComputeGlobalAUCValue();

	// Evaluation du predicteur par la surface sous la courbe de ROC
	// En effet de bord, le tableau des scores est trie
	double ComputeAUCValueAt(int nTargetValueIndex);

	// Calcul de la courbe de lift pour une modalite cible donnee
	// Le vecteur resultant contient (k+1) valeurs, correspondant aux k
	// partiles demandes (en tenant compte des lifts extremes a 0% et 100%)
	// Memoire: le vecteur resultat est passe en troisieme parametre
	void ComputeLiftCurveAt(int nTargetValueIndex, int nPartileNumber, DoubleVector* dvLiftValues);

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tri des evaluations selon les score d'une modalites cible
	void SortInstanceEvaluationsAt(int nTargetValueIndex);

	// Nombre de valeurs cible
	int nTargetValueNumber;

	// Tableau d'evaluations (KWClassifierEvaluationInstance)
	ObjectArray* oaInstanceEvaluations;

	// Epsilon pour gerer le probleme de precision numerique
	static double dEpsilon;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Evaluation d'une instance
// Contient la classe reelle et le vecteur de probas conditionnelles aux classes
class KWClassifierInstanceEvaluation : public Object
{
public:
	// Constructeur
	KWClassifierInstanceEvaluation();
	~KWClassifierInstanceEvaluation();

	// Parametrage du nombre de valeurs/modalites cibles (et reinitialisation)
	void SetTargetValueNumber(int nValue);
	int GetTargetValueNumber() const;

	// Parametrage d'un index de valeur cible "reel"
	// On parametre une valeur non vue en apprentissage en utilisant TargetValueNumber comme index
	void SetActualTargetIndex(int nActualTargetIndex);
	int GetActualTargetIndex();

	// Parametrage d'une probabilite de valeur cible
	// ccepte un score non necessairement compris entre 0 et 1 pour ranker les instances
	void SetTargetProbAt(int nTargetIndex, Continuous cProb);
	Continuous GetTargetProbAt(int nTargetIndex) const;

	// Parametrage d'une valeur de tri
	void SetSortValue(Continuous cValue);
	Continuous GetSortValue() const;

protected:
	// Index de la valeur cible reel
	int nActualTargetIndex;

	// Vecteur des probabilites des valeurs de la cible
	ContinuousVector cvTargetProbs;

	// Valeur de tri
	Continuous cSortValue;

	// La version partagee de cette classe est completement liee a l'implementation
	friend class PLShared_ClassifierInstanceEvaluation;

	// Fonction auxilier pour trier des tableaux d'evaluations d'instances
	friend int KWClassifierInstanceEvaluationCompare(const void* elem1, const void* elem2);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Echantillonneur uniforme en ligne d'objets avec taille maximale
// Implementation via la methode de "Reservoir Sampling" (Vitter, 1985)
class KWReservoirSampler : public Object
{
public:
	// Constructeur
	KWReservoirSampler();
	~KWReservoirSampler();

	// Parametrage de la capacite maximale de l'echantillon
	void SetCapacity(int nCapacity);
	int GetCapacity();

	// Parametrage de la graine du RNG
	void SetSamplerRandomSeed(longint lNewRandomSeed);
	longint GetSamplerRandomSeed() const;

	// Nombre d'objets echantillonnes
	int GetSampleSize() const;

	// Nombre d'objets vus
	longint GetSeenObjectNumber() const;

	// Acces aux objets echantillonees
	// Memoire: Responsabilite de l'appele
	Object* GetSampledObjectAt(int nIndex) const;
	ObjectArray* GetSampledObjects();

	// Ajout d'un objet a l'echantillon
	// Si l'echantillon est sature alors l'objet est ajoute au hasard de facon que
	// la collection finale est un echantillon uniforme et independant des objets vus
	// Dans le cas que l'objet n'est pas ajoute alors il est detruit
	// Memoire: Responsabilite de l'appele
	void Add(Object* object);
	void AddWithProb(Object* object, double dProb);

	// Ajout d'un tableau d'objets a l'echantillon
	// Memoire: Responsabilite de l'appele (objets potentiellement detruits)
	void AddArray(const ObjectArray* objects);
	void AddArrayWithProb(const ObjectArray* objects, double dProb);

	// Reechantillonnage de l'echantillon avec une probabilite donnee
	void Resample(double dProb);

	// Elimination de toutes les references aux objets echantillones (ne libere pas la memoire)
	void RemoveAll();

	// Destruction de tous les objets echantillones
	void DeleteAll();

	// Status d'overflow du reservoir: true si l'on a vu plus d'objets que la capacite maximale
	boolean IsOverflowed();

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Longint pseudo-aleatoire entre 0 et lMax
	longint RandomLongint(longint lMax);

	// Nombre maximal d'objects dans le tableau
	int nCapacity;

	// Graine du RNG
	longint lRandomSeed;

	// Nombre total d'objets vus
	longint lSeenObjectNumber;

	// Echantillon
	ObjectArray oaSampledObjects;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ClassifierInstanceEvaluation
// Serialisation de la classe KWClassifierEvaluationInstance
class PLShared_ClassifierInstanceEvaluation : public PLSharedObject
{
public:
	// Constructor
	PLShared_ClassifierInstanceEvaluation();
	~PLShared_ClassifierInstanceEvaluation();

	// Sauvegarde d'un ClassifierEvaluationInstance
	void SetClassifierInstanceEvaluation(KWClassifierInstanceEvaluation* evaluation);

	// Obtient le CleanPredictorSharedVariables enveloppe
	KWClassifierInstanceEvaluation* GetClassifierInstanceEvaluation();

	// Serialisation d'un ClassifierEvaluationInstance (reimplementation)
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;

	// Deserialisation d'un ClassifierEvaluationInstance (reimplementation)
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

protected:
	// Wrapper generique du constructeur de ClassifierEvaluationInstance
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Methodes en inline

inline KWLoadIndex KWPredictorEvaluationTask::GetLoadIndex(KWAttribute* attribute) const
{
	KWLoadIndex liInvalid;

	require(attribute == NULL or (attribute->GetLoaded() and attribute->GetLoadIndex().IsValid()));

	if (attribute != NULL)
		return attribute->GetLoadIndex();
	else
		return liInvalid;
}

inline void KWAucEvaluation::SetTargetValueNumber(int nValue)
{
	require(nValue >= 0);
	nTargetValueNumber = nValue;
}

inline int KWAucEvaluation::GetTargetValueNumber() const
{
	return nTargetValueNumber;
}

inline KWClassifierInstanceEvaluation::KWClassifierInstanceEvaluation()
{
	cSortValue = 0;
	nActualTargetIndex = 0;
}

inline KWClassifierInstanceEvaluation::~KWClassifierInstanceEvaluation() {}

inline void KWClassifierInstanceEvaluation::SetTargetValueNumber(int nValue)
{
	require(nValue >= 0);
	cvTargetProbs.SetSize(nValue);
	cvTargetProbs.Initialize();
	nActualTargetIndex = 0;
}

inline int KWClassifierInstanceEvaluation::GetTargetValueNumber() const
{
	return cvTargetProbs.GetSize();
}

inline void KWClassifierInstanceEvaluation::SetActualTargetIndex(int nTargetIndex)
{
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber() + 1);
	nActualTargetIndex = nTargetIndex;
}

inline int KWClassifierInstanceEvaluation::GetActualTargetIndex()
{
	return nActualTargetIndex;
}

inline void KWClassifierInstanceEvaluation::SetTargetProbAt(int nTargetIndex, Continuous cProb)
{
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());
	cvTargetProbs.SetAt(nTargetIndex, cProb);
}

inline Continuous KWClassifierInstanceEvaluation::GetTargetProbAt(int nTargetIndex) const
{
	return cvTargetProbs.GetAt(nTargetIndex);
}

inline void KWClassifierInstanceEvaluation::SetSortValue(Continuous cValue)
{
	cSortValue = cValue;
}

inline Continuous KWClassifierInstanceEvaluation::GetSortValue() const
{
	return cSortValue;
}
