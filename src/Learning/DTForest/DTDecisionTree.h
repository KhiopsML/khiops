// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// #include "DTGlobalTag.h"
#include "DTDecisionTreeNode.h"
#include "DTDecisionTreeCost.h"
#include "DTDecisionTreeParameter.h"
#include "DTForestAttributeSelection.h"
#include "DTDecisionTreeDatabaseObject.h"
#include "KWLearningReport.h"

#include "KWDRMath.h"
#include "KWDRCompare.h"
#include "KWDRLogical.h"
#include "KWFrequencyVector.h"
#include "DTAttributeSelection.h"
#include "DTConfig.h"

class DTDecisionTreeCost;
class DTDecisionTreeNodeSplit;
class DTForestParameter;

/// Classe generique de gestion d'un arbre de classification.
/// Un arbre est caracterise par :
/// - le tableau de ses feuilles
/// - le tableau de ses noeuds internes
/// - le dictionnaire des variables utilisees par l'arbre
/// - sa profondeur
/// - son taux de precision pour les donnees d'apprentissage
/// Pour chaque noeud interne, on memorise le partitionnement
/// de sa base parmi ses noeuds fils dans un objet de type DTBaseLoaderSplitter.
/// L'ensemble de ces partitionnements est contenu dans un ObjectArray.
/// On memorise eventuellement des contraintes de structure de l'arbre:
/// - une profondeur max de l'arbre
/// - un nombre max de feuilles
/// - un nombre max de noeuds internes
/// et des contraintes de qualite predictive:
/// - un support minimum d'une feuille
/// - une purete max d'une feuille
/// - une taux de precision max
/// Chaque arbre se caracterise egalement par la maniere dont est calcule son
/// cout. Pour cela on memorise :
/// - un pointeur sur une classe de cout qui decrit un modele de cout de l'arbre
/// - le cout de l'arbre courant et de sa racine, son niveau (cout normalise)
/// Aucune classe de cout n'est instanciee a la construction de l'arbre, cela
/// reste a la charge de l'appelant (un DTClassifier).
/// Le cout ne peut etre evalue que lorsqu'une
/// fonction de cout est specifiee (par exemple un DTDecisionTreeCostRecursive est specifie
/// dans le cas d'un DTPredictorDown).

class DTDecisionTree : public KWLearningReport
{
public:
	/// Constructeur
	DTDecisionTree();
	~DTDecisionTree();

	/** type de tirage (avec ou sans remise, etc) des instances de base de donnees */
	enum class DrawingType
	{
		NoReplacement,           // pas de tirage avec remise
		UseOutOfBag,             // tirage avec remise, choix aleatoire
		WithReplacementAdaBoost, // tirage avec remise, choix pondere (algo AdaBoost)
	};

	boolean ComputeStats();

	/// Duplication de l'arbre
	DTDecisionTree* Clone();

	/// Recopie
	void CopyFrom(const DTDecisionTree* tree);

	/// Acces au noeud racine
	/// Il s'agit du 1er noeud feuille ou du 1er noeud interne s'il y a eu
	/// au moins un partitionnement
	DTDecisionTreeNode* GetRootNode() const;

	/// Acces au tableau des noeuds internes
	ObjectDictionary* GetInternalNodes() const;

	/// Acces au tableau de feuilles
	ObjectDictionary* GetLeaves() const;

	/// Acces au dictionnaire des variables utilisees
	ObjectDictionary* GetUsedVariablesDictionary() const;

	/// Acces a l'evaluation de l'arbre
	double GetEvaluation() const;

	/// Acces a la precision en apprentissage de l'arbre
	double GetTrainingAccuracy() const;

	double GetOutOfBagTrainingAccuracy() const;

	/// Acces aux paramettres d'apprentissage
	DTDecisionTreeParameter* GetParameters() const;
	void SetParameters(DTDecisionTreeParameter* dtParam);

	SymbolVector* GetReferenceTargetModalities() const;

	/** type de tirage (avec ou sans remise) */
	void SetDrawingType(DrawingType);
	DrawingType GetDrawingType() const;

	/// nombre d'objets de l'arbre qui ne sont pas "out of bag" (comprend des exemples qui sont presents plusieurs
	/// fois, si tirage avec remise)
	int GetObjectsNumber() const;

	/// nombre d'objets de l'arbre qui sont "out of bag"
	int GetOutOfBagObjectsNumber() const;

	/// Acces a la profondeur max
	int GetTreeDepth() const;

	/// Acces au nombre de noeud pour chaque profondeur
	IntVector* GetNodeNumberByDepth() const;

	/// Acces au cout de l'arbre
	double GetTreeCost() const;

	/// Acces au cout de construction de l'arbre
	void SetConstructionCost(double);
	double GetConstructionCost() const;

	/// Acces au cout du noeud racine
	double GetRootCost() const;

	/// Acces a la profondeur moyenne
	double GetSonsNumberMean() const;

	/// Acces a la classe de cout
	DTDecisionTreeCost* GetCostClass() const;
	void SetCostClass(DTDecisionTreeCost* cost);

	/// Initialisation du noeud racine avec les specifications d'apprentissage
	/// Calcul des stat MODL 1D et initialisation du vecteur des classes cibles
	void InitializeRootNode(const NumericKeyDictionary* randomForestDatabaseObjects);

	/// Initialisation du cout de l'arbre reduit a sa racine
	void InitializeCostValue(double dValue);

	/// Calcul de la precision en apprentissage de l'arbre
	double ComputeTrainingAccuracy();

	/// Calcul de la precision OOB en apprentissage de l'arbre
	double ComputeOutOfBagTrainingAccuracy();

	/// Renvoie l'evaluation de l'arbre
	/// La classe de cout doit etre specifiee
	double ComputeTreeEvaluation();

	/// Acces au vecteur de filtrage des attributs
	ObjectArray* GetSelectedAttributes() const;
	void SetSelectedAttributes(ObjectArray* oaSelectedAttributes);

	int GetUsableAttributesNumber() const;
	void SetUsableAttributesNumber(const int);

	boolean Check() const;

	void Write(ostream&);

	void WriteNodes(ostream&, const ObjectDictionary*);

	// Ecriture d'un rapport (accessible uniquement si statistiques calculees)
	void WriteReport(ostream& ost);

	void WriteDTArrayLineReport(ostream& ost, const ALString& sTitle, ObjectArray* oaLearningReports,
				    DTDecisionTree* tree);

	void WriteDatabaseObjects(ostream& ost, int maxObjects = 0);

	NumericKeyDictionary* GetDatabaseObjects() const;

	void BuildDatabaseObjectsDictionary(const NumericKeyDictionary* randomForestDatabaseObjects);

	const ALString& GetRootVariableName() const;
	void SetRootVariableName(const ALString& rootvariablename);

	// ObjectArray* oalistSelectedAttributes;

	class TargetModalityCount : public Object
	{
	public:
		TargetModalityCount();
		void CopyFrom(const TargetModalityCount*);
		TargetModalityCount* Clone() const;

		// acces public pour perfs
		Symbol sModality;

		// comptage total, tous echantillons compris
		int iCount;
	};

	DTBaseLoader* GetOrigineBaseLoader();
	void SetOrigineBaseLoader(DTBaseLoader* TableLoader);

	/// Acces a la database
	// ObjectArray* GetObjectsDataBase() const;
	// void SetObjectsDataBase(ObjectArray* cost);

	/// Acces a la selection des attributs
	DTAttributeSelection* GetAttributeSelection();

	void SetAttributeSelection(DTAttributeSelection* generator);
	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// construction d'un arbre
	boolean Build();

	// elaguage des noeuds feuilles, lorsque amelioration du cout
	void Prune();

	// lors de l'etape descendante d'une construction, rechercher les feuilles candidates a la transformation en
	// noeud interne
	NumericKeyDictionary* SelectPossibleSplits();

	// lors de l'etape montante d'une construction, rechercher le noeud interne qui, une fois elague, produirait la
	// meilleure amelioration de cout
	DTDecisionTreeNode* FindBestPrunableInternalNode();

	// lors de l'etape descendante d'une construction, choisir aleatoirement une coupure, parmi celles qui ont
	// ameliore le cout
	DTDecisionTreeNodeSplit* ChooseSplitRandomUniform(NumericKeyDictionary* possibleSplits);

	// lors de l'etape descendante d'une construction, choisir aleatoirement une coupure, parmi celles qui ont
	// ameliore le cout, et en fonction des levels
	DTDecisionTreeNodeSplit* ChooseSplitRandomLevel(NumericKeyDictionary* possibleSplits, boolean bpostmode);

	virtual DTDecisionTreeNodeSplit* ChooseSplitBestTreeCost(NumericKeyDictionary* possibleSplits,
								 const double maxCost);

	ObjectArray* GetImprovingCostSplits(NumericKeyDictionary* possibleSplits, boolean btest);

	// lors de l'etape descendante d'une construction, tester les criteres d'arret sur la structure d'un arbre
	// (profondeur, etc)
	boolean CanAddNodes();

	/// Ajout d'un noeud feuille et mise a jour du niveau de l'arbre
	void AddLeaveNodeAndUpdateTreeDepth(DTDecisionTreeNode* node);

	/// Passage du statut de feuille a celui de noeud interne pour un noeud en specifiant
	/// - son identifiant dans le dictionnaire des feuilles
	/// - le cout (evalue au prealable) dNewCost du nouvel arbre obtenu apres ajout
	/// de ces feuilles
	/// - l'assertion et la table de partition  dans assertion et kwcTable
	/// - le nombre de valeurs initiales nValueNumber de la variable de partitionnement.
	/// Il s'agit du nombre d'individus dans le cas continu et d'un nombre de modalites
	/// le cas categoriel.
	/// - les index de chargement des attributs selectionnes
	// la methode renvoie true si le noeud a pu etre passe du statut feuille a celui de noeud interne, et false
	// sinon.

	boolean SetUpInternalNode(const double dNewCost, const Symbol sNodeKey, KWAttributeStats*,
				  const int nValueNumber, const boolean bIsRegression);

	/// transformer un noeud interne en feuille
	void PruneInternalNode(DTDecisionTreeNode*);

	void RegisterAttributeNameAndOccurenceNumber(const ALString& sAttributeName);

	void UpdateDatabaseObjectsTargetModalities();
	void UpdateDatabaseObjectsNodeIds(DTDecisionTreeNode*);

	int GetDatabaseInstanceId(const NumericKeyDictionary* randomForestDatabaseObjects, KWObject*) const;

	ContinuousVector* ComputeAdaBoostCumulatedWeights(const NumericKeyDictionary* randomForestDatabaseObjects,
							  ObjectArray* kwoObjectsList) const;
	IntVector* SelectRandomWeightedIndexes(const ContinuousVector* cumulatedWeights, const int nbDraws);

	// Recopie l'arbre courant memorise dans l'arbre courant
	// Effectue le nettoyage des noeuds inutilises
	// Met a jour le dictionnaire des variables utilises et le tableau des pointeurs de KWDatabase
	void ReplaceCurrentTreeWithOptimalTree();
	void ReplaceCurrentInternalNodesWithOptimalTree();
	void ReplaceCurrentLeavesWithOptimalTree();

	// Memorise l'arbre courant dans l'arbre optimal
	void MemorizeCurrentTreeInOptimalTree();

	/* retourne un dico d'objets TargetModalityCount (cle = modalite cible) */
	NumericKeyDictionary* ComputeTargetModalitiesCount(DTBaseLoader* bldata);

	// suppression des references a un noeud, dans les dictionnaires, et destruction physique de l'objet
	// DTDecisionTreeNode
	void DeleteNode(DTDecisionTreeNode*);

	boolean CheckInternalNodes() const;
	boolean CheckOptimalInternalNodes() const;
	boolean CheckLeavesNodes() const;
	boolean CheckOptimalLeavesNodes() const;
	boolean CheckSelectedAttributes() const;
	boolean CheckCurrentTree() const;
	boolean CheckOptimalTree() const;

	//////////////////    attributs

	/// Dictionnaire de pointeurs vers les feuilles de l'arbre optimal
	ObjectDictionary* odOptimalLeaveNodes;

	/// Dictionnaire de pointeurs vers les noeuds internes de l'arbre optimal
	ObjectDictionary* odOptimalInternalNodes;

	/// Dictionnaire de pointeurs vers les noeuds internes de l'arbre courant
	ObjectDictionary* odCurrentInternalNodes;

	/// Dictionnaire de pointeurs vers les feuilles de l'arbre courant
	ObjectDictionary* odCurrentLeaveNodes;

	///  Dictionnaire de tous les noeud interet feuille construit
	///  C'est la mémorie des noeuds
	ObjectDictionary odTreeNodes;

	/// Dictionnaire de pointeurs vers les partitionnements successifs de KWDatabases (1 partitionnement par noeud
	/// interne)
	ObjectDictionary odDatabaseSplittersTrain;
	ObjectDictionary odDatabaseSplittersOutOfBag;

	/// Dictionnaire des attributs reellement utilises dans l'arbre (tous les attributs potentiellement utilisables
	/// ne sont pas forcement utilises).
	// Est renseigne au fur et a mesure de la creation de nouveaux noeuds
	//	Cle = nom d'attribut
	//	valeur = IntObject *, qui est le nbre d'occurences de l'attribut dans l'arbre
	ObjectDictionary odUsedVariables;

	/// Profondeur de l'arbre
	int nTreeDepth;

	/// Nombre de noeuds de chaque profondeur
	IntVector ivNodeNumberByDepth;

	/// Classe de calcul de cout de l'arbre (a specifier selon le classifieur)
	DTDecisionTreeCost* treeCost;

	/// Cout de l'arbre
	double dCost;

	/// Cout de l'arbre initial (racine)
	double dRootCost;

	// cout de construction
	double dConstructionCost;

	double dOptimalCost;

	double dBestDecreasedCost;

	double dPreviousCost;

	int nDownStepNumber;

	/// Cout normalise de l'arbre : 1 - cout de l'arbre / cout de la racine
	double dEvaluation;

	/// Taux de precision en apprentissage de l'arbre
	double dTrainingAccuracy;

	/// Taux de precision OOB en apprentissage de l'arbre
	double dOutOfBagTrainingAccuracy;

	/// Pointeur vers les parametres d'apprentissage
	DTDecisionTreeParameter* dtPredictorParameter;

	/// modalites cibles de l'arbre obtenues a la racine
	SymbolVector* svReferenceTargetModalities;

	/// Specifications d'apprentissage de travail (pour ComputeStats des noeuds)
	KWLearningSpec workingLearningSpec;

	// base de donnees des objets
	// ObjectArray* oaObjectsDataBase;

	// attributs selectionnes, dans le cas ou cette selection restera la meme pour tous les noeuds de l'arbre
	ObjectArray* oaSelectedAttributes;
	DTAttributeSelection* SelectedAttributes;

	/** dictionnaire des instances de la base associee a l'arbre (filtre eventuel des instances sur l'attribut de
	sampling , dans le cas d'un uplift). Cle = KWObject *, valeur = DTDecisionTreeDatabaseObject *
	*/
	NumericKeyDictionary* nkdDatabaseObjects;

	DTBaseLoader* origineBaseLoader;
	DTBaseLoader* rootNodeTrainBaseLoader;
	DTBaseLoader* rootNodeOutOfBagBaseLoader;

	int nObjectsNumber;

	int nOutOfBagObjectsNumber;

	DrawingType drawingType;

	ALString RootVariableName;

	int nLastNodeIdentifier;

	int nUsableAttributesNumber;
};

// classe DTDecisionTreeNodeSplit : coupure de noeud potentielle

class DTDecisionTreeNodeSplit : public Object
{
public:
	DTDecisionTreeNodeSplit();
	~DTDecisionTreeNodeSplit();

	void Write(ostream&);

	KWAttributeStats* GetAttributeStats() const;
	void SetAttributeStats(KWAttributeStats*);

	DTDecisionTreeNode* GetSplittableNode() const;
	void SetSplittableNode(DTDecisionTreeNode*);

	double GetTreeCost() const;
	void SetTreeCost(double);

	// ObjectArray & GetSamplingFrequenciesContingencyTables();
	int CompareName(const DTDecisionTreeNodeSplit* otherReport) const;

protected:
	// boolean HasValidDistribution(IntVector & sonDistributionS1, IntVector & sonDistributionS0, double
	// rootNodeFrequencyS1, double rootNodeFrequencyS0);

	KWAttributeStats* attributeStats;
	DTDecisionTreeNode* splittableNode;
	double dTreeCost;
};

inline NumericKeyDictionary* DTDecisionTree::GetDatabaseObjects() const
{
	return nkdDatabaseObjects;
}

inline int DTDecisionTree::GetObjectsNumber() const
{
	return nObjectsNumber;
}

inline int DTDecisionTree::GetUsableAttributesNumber() const
{
	return nUsableAttributesNumber;
}

inline int DTDecisionTree::GetOutOfBagObjectsNumber() const
{
	return nOutOfBagObjectsNumber;
}

inline DTDecisionTree::DrawingType DTDecisionTree::GetDrawingType() const
{
	return drawingType;
}

inline KWAttributeStats* DTDecisionTreeNodeSplit::GetAttributeStats() const
{
	return attributeStats;
}

inline void DTDecisionTreeNodeSplit::SetAttributeStats(KWAttributeStats* a)
{
	attributeStats = a;
}
inline DTDecisionTreeNode* DTDecisionTreeNodeSplit::GetSplittableNode() const
{
	return splittableNode;
}

inline void DTDecisionTreeNodeSplit::SetSplittableNode(DTDecisionTreeNode* d)
{
	splittableNode = d;
}

inline double DTDecisionTreeNodeSplit::GetTreeCost() const
{
	return dTreeCost;
}
inline void DTDecisionTreeNodeSplit::SetTreeCost(double d)
{
	dTreeCost = d;
}

int DTCompareInstancesOnIds(const void* elem1, const void* elem2);
int DTTargetCountCompare(const void* elem1, const void* elem2);
int DTDecisionTreeNodesDepthCompare(const void* elem1, const void* elem2);

inline DTBaseLoader* DTDecisionTree::GetOrigineBaseLoader()
{
	return origineBaseLoader;
}

inline void DTDecisionTree::SetOrigineBaseLoader(DTBaseLoader* TableLoader)
{
	origineBaseLoader = TableLoader;
}

inline DTAttributeSelection* DTDecisionTree::GetAttributeSelection()
{
	return SelectedAttributes;
}

inline void DTDecisionTree::SetAttributeSelection(DTAttributeSelection* generator)
{
	SelectedAttributes = generator;
}

inline int DTDecisionTreeNodeSplit::CompareName(const DTDecisionTreeNodeSplit* otherReport) const
{
	require(GetAttributeStats() != NULL);
	require(otherReport->GetAttributeStats() != NULL);

	return GetAttributeStats()->GetAttributeName().Compare(otherReport->GetAttributeStats()->GetAttributeName());
}
//
// inline ObjectArray* DTDecisionTree::GetObjectsDataBase() const
//{
//	return oaObjectsDataBase;
//}
//
// inline void DTDecisionTree::SetObjectsDataBase(ObjectArray* oadb)
//{
//	oaObjectsDataBase = oadb;
//}

inline int DTSplitCompareSortName(const void* elem1, const void* elem2)
{
	DTDecisionTreeNodeSplit* report1;
	DTDecisionTreeNodeSplit* report2;

	check(elem1);
	check(elem2);

	// Acces aux objets
	report1 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem1);
	report2 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem2);

	// Retour
	return report1->CompareName(report2);
}

inline int DTSplitCompareSortNameID(const void* elem1, const void* elem2)
{
	DTDecisionTreeNodeSplit* report1;
	DTDecisionTreeNodeSplit* report2;

	check(elem1);
	check(elem2);

	// Acces aux objets
	report1 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem1);
	report2 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem2);

	// Retour
	return report1->GetSplittableNode()->GetNodeIdentifier().Compare(
	    report2->GetSplittableNode()->GetNodeIdentifier());
}

inline int DTSplitCompareSortValue(const void* elem1, const void* elem2)
{
	DTDecisionTreeNodeSplit* report1;
	DTDecisionTreeNodeSplit* report2;
	ALString sSortValue1;
	ALString sSortValue2;
	int nCompare;

	check(elem1);
	check(elem2);

	// Acces aux objets
	report1 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem1);
	report2 = cast(DTDecisionTreeNodeSplit*, *(Object**)elem2);

	// Evaluation de la valeur de tri des objets

	// dSortValue1 = report1->GetTreeCost();
	// dSortValue2 = report2->GetTreeCost();
	sSortValue1 = report1->GetSplittableNode()->GetNodeIdentifier();
	sSortValue2 = report2->GetSplittableNode()->GetNodeIdentifier();

	// On se base sur un comparaison a dix decimales pres
	// lSortValue1 = longint(floor(dSortValue1 * 1e10));
	// lSortValue2 = longint(floor(dSortValue2 * 1e10));
	// nCompare = -CompareLongint(lSortValue1, lSortValue2);
	nCompare = sSortValue1.Compare(sSortValue2);

	// Comparaison si necessaire sur le nom
	if (nCompare == 0)
	{
		// comparaison sur le nom de l'attribut du KWAttributeStats
		nCompare = DTSplitCompareSortName(elem1, elem2);
		if (nCompare == 0)
		{
			// comparaison sur l'id de noeud de l'arbre
			nCompare = DTSplitCompareSortNameID(elem1, elem2);
		}
	}

	return nCompare;
}

class PLShared_TargetModalityCount : public PLSharedObject
{
public:
	// Constructor
	PLShared_TargetModalityCount();
	~PLShared_TargetModalityCount();

	void SetTargetModalityCount(DTDecisionTree::TargetModalityCount*);
	DTDecisionTree::TargetModalityCount* GetTargetModalityCount() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	Object* Create() const override;
};
