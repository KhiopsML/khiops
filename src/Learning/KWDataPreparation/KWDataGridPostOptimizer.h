// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataGrid.h"
#include "KWDataGridCosts.h"
#include "KWDataGridManager.h"
#include "KWDiscretizerMODL.h"
#include "KWGrouperMODL.h"
#include "KWSortableIndex.h"

class KWDataGridPostOptimizer;
class KWDGPODiscretizer;
class KWDGPOPartFrequencyVector;
class KWDataGridUnivariateCosts;
class KWDataGridCostParameter;
class KWDGAttributeCostParameter;
class KWDGPartCostParameter;
class KWDGSymbolValueSetCostParameter;
class KWDGVarPartSetCostParameter;
class KWDGCellCostParameter;
class KWDGPOCellFrequencyVector;
class CCVarPartDataGridPostOptimizer;

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridPostOptimizer
// Post-optimisation d'une grille de donnees parametree par une structure de cout
// La post-optimisation procede par des optimisations univariees, en fixant
// a chaque fois les partitions de tous les attributs sauf une, et en optimisant
// la derniere partition
class KWDataGridPostOptimizer : public Object
{
public:
	// Constructeur
	KWDataGridPostOptimizer();
	~KWDataGridPostOptimizer();

	// Parametrage de la structure des couts de la grille de donnees
	// Memoire: les specifications sont referencees et destinees a etre partagees par plusieurs algorithmes
	void SetDataGridCosts(const KWDataGridCosts* kwdgcCosts);
	const KWDataGridCosts* GetDataGridCosts() const;

	// Optimisation d'un grille pour une structure de cout donnees
	// La grille initiale en entree contient la description la plus fine possible des
	// partitions de chaque attribut.
	// En entree, la grille optimisee constitue une solution de depart, compatible avec la grille initiale.
	// En sortie, la grille optimisee est amelioree par la methode de post-optimisation.
	// Le mode pousse (DeepPostOptimization) remet en question fortement les partition univariees, alors
	// que le mode leger se contente de deplacer les frontieres
	// Retourne le cout de codage MODL de la grille post-optimisee
	// CH RefontePrior2-P
	// Modification pour prise en compte du groupe poubelle lors de la post-optimisation univariee d'un attribut
	// categoriel Fin CH RefontePrior2-P
	double PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
				    boolean bDeepPostOptimization) const;

	/////////////////////////////////////////////////////////////////
	///// Methode avancees

	// Construction d'une grille initiale pour l'optimisation univariee, ayant les memes partitions
	// par attribut que la grille optimisee, sauf sur l'attribut a optimiser (dans ce cas, partition la
	// plus fine possible provenant de la grille initiale)
	KWDataGrid* BuildUnivariateInitialDataGrid(const KWDataGrid* optimizedDataGrid,
						   const KWDataGrid* initialDataGrid,
						   const ALString& sPostOptimizationAttributeName) const;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Parametrage de la structure des couts
	const KWDataGridCosts* dataGridCosts;

	// Seuil d'amelioration
	double dEpsilon;
};

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPODiscretizer
// Post-optimisation univariee d'un attribut continu
class KWDGPODiscretizer : public KWDiscretizerMODL
{
public:
	// Constructeur
	KWDGPODiscretizer();
	~KWDGPODiscretizer();

	// Nom de l'attribut post-optimise
	void SetPostOptimizationAttributeName(const ALString& sValue);
	const ALString& GetPostOptimizationAttributeName() const;

	// Optimisation univariee d'une  grille (cf classe KWDataGridPostOptimizer)
	// Retourne le cout de codage MODL de la grille post-optimisee
	// CH RefontePrior2-G
	// Prise en compte granularite
	// Fin CH RefontePrior2-G
	double PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, const KWDataGridCosts* dataGridCosts,
				    KWDataGrid* optimizedDataGrid, boolean bDeepPostOptimization) const;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class CCCoclusteringBuilder;

	/////////////////////////////////////////////////////////////////////////////
	// Gestion des transfert entre grille de donnees et les donnees de travail
	// des methodes de discretisation (tableau d'effectifs et liste d'intervalles)

	// Initialisation d'un tableau d'effectif a partir d'une grille
	// Le createur d'effectif courant est utilise pour le choix de la representation
	// des vecteurs d'effectifs de la table d'effectifs
	void InitializeFrequencyTableFromDataGrid(KWFrequencyTable* kwftFrequencyTable,
						  const KWDataGrid* dataGrid) const;

	// Initialisation d'un vecteur d'effectif a partir d'une partie de la grille,
	// et du dictionnaire associant chaque cellule a une cellule de hash (signature exogene)
	void InitializePartFrequencyVector(KWDGPOPartFrequencyVector* partFrequencyVector, const KWDGPart* part,
					   const NumericKeyDictionary* nkdHashCells) const;

	// Initialisation d'une cellule d'un vecteur d'effectifs a partir de la cellule source et
	// de sa cellule de hash
	void InitializeCellFrequencyVector(KWDGPOCellFrequencyVector* cellFrequencyVector, const KWDGCell* cell,
					   const KWDGCell* hashCell) const;

	// Initialisation d'un dictionnaire qui a chaque cellule de la grille (identifiee par sa
	// signature globale) associe une cellule caracterisee par sa signature partielle (sur les
	// attributs hors attribut de post-optimisation). Deux cellules ayant meme signature partielle
	// sont alors associee a la meme cellule HashCell (representant toutes les cellules ayant meme
	// signature partielle)
	void InitializeHashCellDictionary(NumericKeyDictionary* nkdHashCells, const KWDataGrid* dataGrid) const;

	// Construction de la liste des intervalles a partir du tableau d'effectif initial et des
	// intervalles de la grille optimisee. Les effectifs rendus sont construits par concatenation
	// des vecteurs d'effectif du tableau d'effectifs
	KWMODLLine* BuildIntervalListFromFrequencyTableAndOptimizedDataGrid(KWMODLLine* lineCreator,
									    const KWFrequencyTable* kwftTable,
									    const KWDataGrid* optimizedDataGrid) const;

	// Mise a jour d'une grille a optimiser a partir d'une liste d'intervalle, referencant
	// les parties de la grille initiale a fusionner pour constituer les nouveaux intervalles
	void UpdateDataGridFromIntervalList(KWDataGrid* optimizedDataGrid, const KWDataGrid* initialDataGrid,
					    KWMODLLine* headLine) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion des classes de travail pour les fusions et coupures d'intervalles
	// Reimplementation des methodes virtuelle de la classee ancetre, pour
	// gerer les specificite des vecteurs d'effectif dedie aux grilles

	// Initialisation et controle d'integrite d'un vecteur d'effectif
	void InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const override;
	boolean CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const override;

	// Initialisation de la structure de couts
	void InitializeWorkingData(const KWFrequencyTable* kwftSource) const override;
	void CleanWorkingData() const override;

	// Operation de transfert d'effectifs lors de copie, fusions, coupures...
	// Les cout des intervalles sont maintenus en permanence lors de ces operations
	void AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				const KWFrequencyVector* kwfvAddedFrequencyVector) const override;
	void RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				   const KWFrequencyVector* kwfvRemovedFrequencyVector) const override;
	void MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
				      const KWFrequencyVector* kwfvMergedFrequencyVector1,
				      const KWFrequencyVector* kwfvMergedFrequencyVector2) const override;
	void MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					const KWFrequencyVector* kwfvMergedFrequencyVector1,
					const KWFrequencyVector* kwfvMergedFrequencyVector2,
					const KWFrequencyVector* kwfvMergedFrequencyVector3) const override;
	void SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				  KWFrequencyVector* kwfvNewFrequencyVector,
				  const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;
	void MergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
					KWFrequencyVector* kwfvSourceFrequencyVector2,
					const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;
	void MergeMergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
					     const KWFrequencyVector* kwfvSourceFrequencyVector2,
					     KWFrequencyVector* kwfvSourceFrequencyVector3,
					     const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;

	// Cout par cellule (nouvelle methode)
	double ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const;

	// Nom de l'attribut de post-optimisation
	ALString sPostOptimizationAttributeName;
};

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOGrouper
// Post-optimisation univariee d'un attribut categoriel
class KWDGPOGrouper : public KWGrouperMODL
{
public:
	// Constructeur
	KWDGPOGrouper();
	~KWDGPOGrouper();

	// Nom de l'attribut post-optimise
	// CH RefontePrior2-P
	// utilise egalement pour fixer l'attribut pour lequel on initialise ou post-optimise la poubelle
	// Fin RefontePrior2-P
	void SetPostOptimizationAttributeName(const ALString& sValue);
	const ALString& GetPostOptimizationAttributeName() const;

	// Optimisation univariee d'une  grille (cf classe KWDataGridPostOptimizer)
	// Retourne le cout de codage MODL de la grille post-optimisee
	// CH RefontePrior2-GP
	// Prend en compte la granularite et le groupe poubelle
	// Fin CH RefontePrior2-GP
	double PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, const KWDataGridCosts* dataGridCosts,
				    KWDataGrid* optimizedDataGrid, boolean bDeepPostOptimization) const;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class CCCoclusteringStats;
	friend class CCCoclusteringBuilder;

	/////////////////////////////////////////////////////////////////////////////
	// Gestion des transfert entre grille de donnees et les donnees de travail
	// des methodes de discretisation (tableau d'effectifs et liste d'intervalles)

	// Initialisation d'un tableau d'effectif a partir d'une grille
	// Le createur d'effectif courant est utilise pour le choix de la representation
	// des vecteurs d'effectifs de la table d'effectifs
	// L'ordre des vecteurs d'effectif dans la table d'effectifs est celui des
	// parties dans la grille
	void InitializeFrequencyTableFromDataGrid(KWFrequencyTable* kwftFrequencyTable,
						  const KWDataGrid* dataGrid) const;

	// Initialisation d'un vecteur d'effectif a partir d'une partie de la grille,
	// et du dictionnaire associant chaque cellule a une cellule de hash (signature exogene)
	void InitializePartFrequencyVector(KWDGPOPartFrequencyVector* partFrequencyVector, const KWDGPart* part,
					   const NumericKeyDictionary* nkdHashCells) const;

	// Initialisation d'une cellule d'un vecteur d'effectifs a partir de la cellule source et
	// de sa cellule de hash
	void InitializeCellFrequencyVector(KWDGPOCellFrequencyVector* cellFrequencyVector, const KWDGCell* cell,
					   const KWDGCell* hashCell) const;

	// Initialisation d'un dictionnaire qui a chaque cellule de la grille (identifiee par sa
	// signature globale) associe une cellule caracterisee par sa signature partielle (sur les
	// attributs hors attribut de post-optimisation). Deux cellules ayant meme signature partielle
	// sont alors associee a la meme cellule HashCell (representant toutes les cellules ayant meme
	// signature partielle)
	void InitializeHashCellDictionary(NumericKeyDictionary* nkdHashCells, const KWDataGrid* dataGrid) const;

	// Initialisation d'un vecteur d'affectation de parties aux groupes entre une grille
	// initiale et une grille groupee, en se basant sur l'ordre des parties dans chaque grille
	void InitializeGroupIndexes(IntVector* ivGroups, const KWDataGrid* initialDataGrid,
				    const KWDataGrid* optimizedDataGrid) const;

	// Initialisation d'un vecteur d'affectation de parties aux groupes entre une grille
	// initiale et une grille groupee, en se basant sur l'ordre des parties dans chaque grille
	// En sortie :
	//	En presence d'un groupe poubelle,  renvoie l'index de la partie poubelle dans l'ordre des parties de la
	// grille groupee 	En l'absence de groupe poubelle, renvoie -1
	int InitializeGroupIndexesAndGarbageIndex(IntVector* ivGroups, const KWDataGrid* initialDataGrid,
						  const KWDataGrid* optimizedDataGrid) const;

	// Initialisation d'un tableau d'effectif groupe a partir d'une grille initiale et des index des groupes
	void InitializeGroupedFrequencyTableFromDataGrid(KWFrequencyTable* groupedFrequencyTable,
							 const KWFrequencyTable* initialFrequencyTable,
							 const IntVector* ivGroups, int nGroupNumber) const;

	// Mise a jour d'une grille a optimiser a partir d'une grille initiale et des index de groupes
	// Le groupe poubelle est active comme le groupe contenant le plus de modalites
	void UpdateDataGridWithGarbageFromGroups(KWDataGrid* optimizedDataGrid, const KWDataGrid* initialDataGrid,
						 const IntVector* ivGroups, int nGroupNumber) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion des classes de travail pour les fusions et coupures d'intervalles
	// Reimplementation des methodes virtuelle de la classee ancetre, pour
	// gerer les specificite des vecteurs d'effectif dedie aux grilles

	// Initialisation et controle d'integrite d'un vecteur d'effectif
	void InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const override;
	boolean CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const override;

	// Initialisation de la structure de couts
	void InitializeWorkingData(const KWFrequencyTable* kwftSource, int nInitialValueNumber) const override;
	void CleanWorkingData() const override;

	// Operation de transfert d'effectif lors de copie, fusions, coupures...
	// Les cout des intervalles sont maintenus en permanence lors de ces operations
	// Initialisation du bon nombre de classe d'un vecteur d'effectifs
	void AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				const KWFrequencyVector* kwfvAddedFrequencyVector) const override;
	void RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				   const KWFrequencyVector* kwfvRemovedFrequencyVector) const override;
	void MergeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
				   const KWFrequencyVector* kwfvMergedFrequencyVector1,
				   const KWFrequencyVector* kwfvMergedFrequencyVector2) const override;
	double ComputeGroupUnionCost(const KWFrequencyVector* sourceGroup1,
				     const KWFrequencyVector* sourceGroup2) const override;
	double ComputeGroupDiffCost(const KWFrequencyVector* sourceGroup,
				    const KWFrequencyVector* removedGroup) const override;

	// Cout par cellule (nouvelle methode)
	double ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const;

	// Nom de l'attribut de post-optimisation
	ALString sPostOptimizationAttributeName;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridUnivariateCosts
// Definition de la structure des couts d'une post-discretisation MODL d'une grille
class KWDataGridUnivariateCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	KWDataGridUnivariateCosts();
	~KWDataGridUnivariateCosts();

	/////////////////////////////////////////////////////////////
	// Parametrage de la structure de cout

	// Nom de l'attribut post-optimise
	void SetPostOptimizationAttributeName(const ALString& sValue);
	const ALString& GetPostOptimizationAttributeName() const;

	// Parametrage de la structure des couts de la grille de donnees
	// Memoire: les specifications sont referencees et destinees a etre partagees par plusieurs algorithmes
	void SetDataGridCosts(const KWDataGridCosts* kwdgcCosts);
	const KWDataGridCosts* GetDataGridCosts() const;

	// Initialisation du parametrage des couts univaries a partir d'une grille de depart
	// Prise en compte des couts pour tous les attributs deja optimises
	void InitializeUnivariateCostParameters(const KWDataGrid* optimizedDataGrid);
	boolean IsInitialized() const;

	// Nombre de valeurs de l'attribut
	int GetValueNumber() const;

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Cout de l'union ou de la difference de deux parties (sans leurs cellules)
	double ComputePartUnionCost(const KWFrequencyVector* sourcePart1, const KWFrequencyVector* sourcePart2) const;
	double ComputePartDiffCost(const KWFrequencyVector* sourcePart, const KWFrequencyVector* removedPart) const;

	// Cout par cellule (nouvelle methode)
	double ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const;

	// Cout de modele par entite (non implemente)
	double ComputePartitionModelCost(int nPartNumber) const;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nettoyage du parametrage des couts
	void CleanUnivariateCostParameters();

	// Nom de l'attribut de post-optimisation
	ALString sPostOptimizationAttributeName;

	// Parametrage de la structure des couts
	const KWDataGridCosts* dataGridCosts;

	// Instance de chaque element de la grille permettant de parametrer les couts
	// Ces instances sont en exemplaire unique et n'appartiennent pas a une grille.
	// Elles servent juste de buffer temporaire pour parametrer les methodes de cout
	// de la grille a partir des donnees univariees
	mutable KWDataGridCostParameter* dataGridCostParameter;
	mutable KWDGAttributeCostParameter* attributeCostParameter;
	mutable KWDGPartCostParameter* partCostParameter;
	mutable KWDGCellCostParameter* cellCostParameter;

	// Partie du cout constant portee par les autres attributs et par les valeurs symboliques
	double dExcludedConstantCost;

	// Fraicheur du parametrage et de l'initialisation
	int nFreshness;
	int nInitializationFreshness;

	////////////////////////////////////////////////////////////////////////
	// Parametres de cout dans le cas du partitionnement d'un attribut cible
	// avec une structure de cout non additive

	// En cas de partitionnement cible (partition source fixee), vecteur qui
	// permet de memoriser les effectifs par intervalle source necessaires pour
	// calculer le cout par partie source, a l'aide de la taille de la partition cible
	IntVector ivFixedSourceFrequencies;

	// Vecteur utilise lors de la partition cible pour une source fixe
	// qui contient la somme des couts par partie source
	// Ces couts sont initialises a -1 puis leur valeur est memorisee des
	// qu'ils ont du etre calcules pour une taille de partition cible donnee
	mutable DoubleVector dvTargetPartitionedCost;

	// Parametrage des couts d'attribut source dans le cas du partitionnement de la cible
	mutable KWDGAttributeCostParameter* attributeSourceCostParameter;
	mutable KWDGPartCostParameter* partSourceCostParameter;

	// Parametrage de l'attribut cible dans le cas du partitionnement de la source
	// (pour acceder a la taille de partition cible)
	mutable KWDGAttributeCostParameter* attributeTargetCostParameter;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classe de grille servant de parametre de cout a la classe KWDataGridUnivariateCosts
// Cette sous-classe est en mode "emule" pour permettre d'acceder aux caracteristiques
// servant de parametres aux cout (effectif...), sans que ces caracteristiques soient
// coherentes avec la structure de la grille (cf methode GetEmulated de KWDataGrid)
// Le "bidouillage" est ainsi decouple de la classe KWDataGrid qui reste le plus
// propre possible (via la methode virtuelle GetEmulated)
class KWDataGridCostParameter : public KWDataGrid
{
protected:
	friend class KWDataGridUnivariateCosts;
	boolean GetEmulated() const override;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classes d'attribut de grille servant de parametre de cout
class KWDGAttributeCostParameter : public KWDGAttribute
{
	// Constructeur
	KWDGAttributeCostParameter();
	~KWDGAttributeCostParameter();

	// Redefinition pour acces direct au nombre de modalites du groupe poubelle sans passer
	// par la garbagePart non accessible en mode "emule"
	void SetGarbageModalityNumber(int nValue);
	int GetGarbageModalityNumber() const;

protected:
	KWDGPart* NewPart() const;
	friend class KWDataGridUnivariateCosts;
	boolean GetEmulated() const override;
	int nGarbageModalityNumber;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classes de partie d'attribut de grille servant de parametre de cout
class KWDGPartCostParameter : public KWDGPart
{
protected:
	friend class KWDataGridUnivariateCosts;
	void SetPartType(int nValue);
	boolean GetEmulated() const override;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classes de ValueSet de grille servant de parametre de cout
class KWDGSymbolValueSetCostParameter : public KWDGSymbolValueSet
{
protected:
	friend class KWDataGridUnivariateCosts;
	boolean GetEmulated() const override;
};

////////////////////////////////////////////////////////////////////////////////////////////
// CH IV Begin
// Sous-classes de VarPartSet de grille servant de parametre de cout
class KWDGVarPartSetCostParameter : public KWDGVarPartSet
{
protected:
	friend class KWDataGridUnivariateCosts;
	boolean GetEmulated() const override;
};
// CH IV End

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classes de cellule de grille servant de parametre de cout
class KWDGCellCostParameter : public KWDGCell
{
protected:
	friend class KWDataGridUnivariateCosts;
	boolean GetEmulated() const override;
};

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOPartFrequencyVector
// Vecteur creux des effectifs d'une partie d'un attribut d'une grille
// utilise dans le cas de la post-optimisation univariee
// Regroupe l'ensemble des cellules d'une partie, accessible par
// table de hashage pour exploiter la structure creuse des grilles.
// La cle de hashage est basee sur la signature partielle (restreinte
// aux K-1 attribut non optimises)
class KWDGPOPartFrequencyVector : public KWFrequencyVector
{
public:
	// Constructeur
	KWDGPOPartFrequencyVector();
	~KWDGPOPartFrequencyVector();

	// Taille d'un vecteur d'effectif
	// Potentiellement sans limite: renvoie donc l'entier max
	int GetSize() const override;

	////////////////////////////////////////////////////////////////
	// Caracteristiques de la partie
	// La gestion de ces caracteristiques est a la charge des classes
	// utilisatrices, ce qui permet des optimisation. L'effectif et
	// le cout des cellules sont neanmoins  controles (par assertion)
	// lors de leur utilisation.

	// Cout local d'une partie, somme des couts de ses cellules
	void SetCellCost(double dValue);
	double GetCellCost() const;

	// Effectif total du vecteur d'effectif, somme des effectif de ses cellules
	void SetFrequency(int nValue);
	int GetFrequency() const;

	////////////////////////////////////////////////////
	// Gestion des cellules d'une partie

	// Nombre de cellules
	int GetCellNumber() const;

	// Rercherche d'une cellule par son identifiant de hash
	// Renvoie la cellule s'il en existe, NULL sinon
	KWDGPOCellFrequencyVector* LookupCell(const Object* cellHashObject);

	// Ajout d'une cellule
	// (il ne doit pas y avoir de cellule ayant meme identifiant)
	// Memoire: la cellule ajoutee appartient a l'appele
	void AddCell(KWDGPOCellFrequencyVector* cellFrequencyVector);

	// Suppression d'une cellule
	// (la cellule a supprimer doit exister dans la partie)
	// Memoire: la cellule en parametre est detruite
	void DeleteCell(KWDGPOCellFrequencyVector* cellFrequencyVector);

	// Parcours des cellules de la partie
	// Exemple:
	//    position = myFrequencyVector->GetHeadPosition();
	//    while (position != NULL)
	//    {
	//       myCell = myFrequencyVector->GetNextPosition(position);
	//       ...
	//    }
	POSITION GetHeadPosition() const;
	KWDGPOCellFrequencyVector* GetNextPosition(POSITION& nextPosition) const;

	////////////////////////////////////////////////////////////////
	// Redefinition des methodes generiques des vecteurs d'effectifs

	// Createur, renvoyant une instance du meme type
	KWFrequencyVector* Create() const override;

	// Copie a partir d'un vecteur source
	void CopyFrom(const KWFrequencyVector* kwfvSource) override;

	// Duplication (y compris du contenu)
	KWFrequencyVector* Clone() const override;

	// Calcul du cout total a partir des cellules
	double ComputeTotalCellCost() const;

	// Calcul de l'effectif total a partir des cellules
	int ComputeTotalFrequency() const override;

	// Rapport synthetique destine a rentrer dans une sous-partie de tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Affichage complet de la partie et de ses cellules
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Cout
	double dCellCost;

	// Effectif
	int nFrequency;

	// Dictionnaire des vecteurs d'effectif par cellule
	// La cle d'acces a une cellule est sa signature partielle (HashObject de la cellule)
	NumericKeyDictionary nkdCellFrequencyVectors;
};

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOCellFrequencyVector
// Vecteur d'effectifs des cellules d'une grille
// Les methodes de cette classe reprennent celles de la classe KWDGCell pour
// la partie gestion des effectifs, mais ne gerent pas le rattachement a la grille
class KWDGPOCellFrequencyVector : public Object
{
public:
	// Constructeur
	KWDGPOCellFrequencyVector();
	~KWDGPOCellFrequencyVector();

	// Cout local de la cellule
	void SetCost(double dValue);
	double GetCost() const;

	// Memorisation d'une objet servant a identifier la classe cible associee a une cellule
	void SetHashObject(const Object* object);
	const Object* GetHashObject() const;

	////////////////////////////////////////////////////////////////////////////////
	// Contenu de la cellule en nombre d'instance
	// Dans le cas non supervise, on a acces a l'effectif total de la cellule (Frequency)
	// Dans le cas supervise (GetTargetValueNumber() > 0), on a acces aux effectifs par
	// classe cible (on ne peut plus appeler directement SetFrequency, mais le GetFrequency
	// reste coherent)

	// Effectif de la cellule
	int GetCellFrequency() const;
	void SetCellFrequency(int nFrequency);

	// Nombre de classes cible (0 dans le cas non supervise)
	int GetTargetValueNumber() const;

	// Effectif par classe cible
	int GetTargetFrequencyAt(int nTarget) const;
	void SetTargetFrequencyAt(int nTarget, int nFrequency);
	void UpgradeTargetFrequencyAt(int nTarget, int nDeltaFrequency);

	// Copie a partir d'une autre cellule
	// Copie de l'identifiant, du cout, retaillage eventuel du vecteur de compteurs d'effectifs
	void CopyFrom(const KWDGPOCellFrequencyVector* cell);

	// Mise a jour du contenu d'une cellule en prenant en compte le contenu d'une autre cellule (ayant meme
	// identifiant)
	void AddFrequenciesFrom(const KWDGPOCellFrequencyVector* cell);
	void RemoveFrequenciesFrom(const KWDGPOCellFrequencyVector* cell);

	// Initialisation du contenu d'une cellule par fusion du contenu de deux cellules (de meme identifiant)
	void MergeFrequenciesFrom(const KWDGPOCellFrequencyVector* cell1, const KWDGPOCellFrequencyVector* cell2);

	///////////////////////////////
	// Services divers

	// Rapport synthetique destine a rentrer dans une sous-partie de tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDGPODiscretizer;
	friend class KWDGPOGrouper;
	friend class KWDataGridUnivariateCosts;

	// Effectif total de la cellule par cumul de l'effectif par classe cible (coherent avec l'effectif de la
	// cellule)
	int ComputeTotalFrequency() const;

	// Cout
	double dCost;

	// Effectif de la cellule
	int nCellFrequency;

	// Vecteur de comptage des effectifs par classe cible
	IntVector ivFrequencyVector;

	// Object identifiant la cellule
	const Object* oHashObject;
};

// CH IV Begin
// CH IV Refactoring: renommages internes apres analyse
//////////////////////////////////////////////////////////////////////////////////
// Classe CCVarPartDataGridPostOptimizer
// Post-optimisation de l'attribut VarPart d'une grille de donnees de type VarPart [[[parametree par une structure de
// cout ]]] L'attribut Identifier est fige durant toute cette post-optimisation La post-optimisation procede par des
// deplacements de PV elementaires d'une grille de reference vers des clusters permettant une fusion de cette PV avec
// d'autres PV presentes
class CCVarPartDataGridPostOptimizer : public Object
{
public:
	// Constructeur
	CCVarPartDataGridPostOptimizer();
	~CCVarPartDataGridPostOptimizer();

	// Nom de l'attribut de type VarPart post-optimise
	void SetPostOptimizationAttributeName(const ALString& sValue);
	const ALString& GetPostOptimizationAttributeName() const;

	// CH IV Refactoring: supprimer tout ce qui est commente dans cette classe?

	// Parametrage de la structure des couts de la grille de donnees
	// Memoire: les specifications sont referencees et destinees a etre partagees par plusieurs algorithmes
	// void SetDataGridCosts(const KWDataGridCosts* kwdgcCosts);
	// const KWDataGridCosts* GetDataGridCosts() const;

	// Optimisation de l'attribut VarPart d'un grille [[[pour une structure de cout donnees ]]]
	// La grille de reference en entree contient la description la plus fine possible des
	// parties de variable de chaque attribut interne dans l'attribut VarPart
	// En entree, la grille optimisee constitue une solution de depart, dont les attributs internes ont ete
	// post-fusionnes par rapport a la grille de reference En sortie, la grille optimisee est amelioree par la
	// methode de post-optimisation.
	// [[[Le mode pousse (DeepPostOptimization) remet en question fortement les partition univariees, alors
	// que le mode leger se contente de deplacer les frontieres
	// Retourne le cout de codage MODL de la grille post-optimisee]]]
	// double PostOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,boolean
	// bDeepPostOptimization) const;

	// Optimisation legere
	// Seuls les deplacements des parties de variables unternes numeriques (intervalles) sont envisagees
	// Cette optimisation ne remet donc pas en question les partitions des attributs internes
	// Elle teste les deplacements de PV qui conduisent a une ou deux fusions de PV et prend en compte la variation
	// de cout du prior lie a ce deplacement Elle ne teste que les deplacements d'intervalles (attribut interne
	// numerique) En entree :
	// - referenceDataGrid : grille identique a optimizedDataGrid avec pour l'attribut VarPart 1 cluster par PV des
	// attributs internes
	// - sInnerAttributeName : nom de l'attribut interne dont l'affectation des intervalles dans les clusters de PV
	// est optimise
	// - grille optimise : amelioree par la post-optimisation, avec la meme partition des attributs internes (les
	// fusions d'intervalles restent a effectuer) En sortie : ivGroups : pour chaque cluster mono-PV de la grille de
	// reference, donne son cluster dans la grille optimisee
	// double PostOptimizeLightVarPartDataGrid(const KWDataGrid* referenceDataGrid, ALString sInnerAttributeName,
	// KWDataGrid* optimizedDataGrid, IntVector* ivGroups) const;

	// La variation de cout se calcule en partant de la meme grille initiale pour tous les deplacements
	// Renvoie vrai s'il existe au moins une amelioration
	// Pour tous les attributs internes numeriques et categoriels
	// En entree:
	//  - referenceDataGrid contient la grille de reference avec des clusters contenant une seule PV
	//    a partir des PV de la grille optimisee
	// En sortie:
	//	- optimizedDataGrid : grille dont on post-optimise les parties de variable
	//	- ivGroups : vecteur d'index qui indique pour chaque PV l'index du cluster de PV
	//    auquel elle appartient, mis a jour au cours de cette post-optimisation
	boolean PostOptimizeLightVarPartDataGrid(const KWDataGrid* referenceDataGrid, KWDataGrid* optimizedDataGrid,
						 IntVector* ivGroups) const;

	/////////////////////////////////////////////////////////////////
	///// Methode avancees

	// Construction d'une grille initiale pour l'optimisation univariee, ayant les memes partitions
	// par attribut que la grille optimisee, sauf sur l'attribut a optimiser (dans ce cas, partition la
	// plus fine possible provenant de la grille initiale)
	// KWDataGrid* BuildUnivariateInitialDataGrid(const KWDataGrid* optimizedDataGrid,
	//				   const KWDataGrid* initialDataGrid,
	//			   const ALString& sPostOptimizationAttributeName) const;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Parametrage de la structure des couts
	// const KWDataGridCosts* dataGridCosts;

	// Methodes dediees de calcul de variation de cout par attribut, par cluster ...

	// Variation de cout lors du deplacement d'un intervalle entier d'un attribut interne numerique vers un cluster
	// qui permet au moins une fusion d'intervalles En entree :
	// - l'attribut VarPart
	// - la variation du nombre de clusters : 1 si le depart de l'intervalle vide completement son cluster de
	// depart, 0 sinon
	// - la variation du nombre de parties de variable : 1 si l'intervalle fusionne avec un intervalle de son
	// cluster d'arrivee, 2 s'il fusionne avec deux intervalles de son cluster d'arrivee
	double ComputeVarPartsContinuousAttributeVariationCost(KWDGAttribute* attribute, int nClusterNumberVariation,
							       int nVarPartsNumberVariation,
							       ALString sInnerAttributeName) const;

	double ComputeVarPartsSymbolAttributeVariationCost(KWDGAttribute* attribute, int nClusterNumberVariation,
							   int nVarPartsNumberVariation, ALString sInnerAttributeName,
							   KWDGPart* varPartIn, KWDGPart* varPartOut,
							   KWDGPart* innerPart) const;

	// Variation de cout d'un cluster lors du depart ou de l'arrivee d'une partie de variable
	// En entree :
	// nObservationNumberVariation : variation de l'effectif du cluster i.e. effectif de la partie de variable qui
	// quitte/arrive dans le cluster (part) L'effectif est positif si le cluster accueille une nouvelle partie de
	// variable, negatif si la partie de variable part du cluster nVarPartsNumberVariation : variation du nombre de
	// parties de variable du cluster
	//								-1 si la partie de variable quitte le cluster
	//								0 si la partie de variable arrive dans le
	// cluster avec une
	// fusion simple 								-1 si la partie de variable
	// arrive dans le cluster avec une fusion double

	// Si on prend en entree KWDGPart* de la PV deplacee avec ses cellules, on aura l'effectif total et la liste des
	// cellules pour calculer la variation de cout du aux cellules A faire par cluster, depart et arrivee ou
	// globalement ? Tester la grille exportee avec un cluster par PV deplacee En entree cluster de depart ou
	// d'arrivee bDeparture : la partie de variable part du cluster
	double ComputeClusterVariationCost(KWDGPart* cluster, int nObservationNumberVariation,
					   int nVarPartsNumberVariation, boolean bDeparture) const;

	// Variation de cout des cellules d'un cluster suite au depart ou a l'arrivee d'une des PV du cluster
	// bDeparture : true la PV part du cluster; false la PV arrive dans le cluster
	double ComputeClusterCellVariationCost(KWDGPart* cluster, KWDGPart* varPart, boolean bDeparture) const;

	// Seuil d'amelioration
	double dEpsilon;

	// Nom de l'attribut VarPart de post-optimisation
	ALString sPostOptimizationAttributeName;
};
// CH IV Begin
