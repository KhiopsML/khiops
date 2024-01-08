// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridCosts;
class KWDataGridClassificationCosts;
class KWDataGridClusteringCosts;
class KWDataGridRegressionCosts;
class KWDataGridGeneralizedClassificationCosts;

#include "KWDataGrid.h"
#include "KWDataGridMerger.h"
#include "KWStat.h"

////////////////////////////////////////////////////////////////////////////
// Definition de la structure des couts d'une grille de donnees
// Les couts par entite, nuls par defaut, sont redefinissable dans des sous-classes
class KWDataGridCosts : public Object
{
public:
	// Constructeur
	KWDataGridCosts();
	~KWDataGridCosts();

	// Duplication
	virtual KWDataGridCosts* Clone() const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees, portant
	// sur la grille, les attributs, les parties et les cellules
	// Methodes virtuelles, renvoyant 0, a reimplementer dans les sous-classes

	// Parametrage d'un cout de selection de famille de modele (defaut: 0)
	// C'est utile si les grilles sont une famille de modeles parmi d'autres, et qu'il faut
	// alors prendre en compte ce cout en cas de modele non null
	// C'est le cas pour les grilles en classification supervisee, qui sont une facon de produire
	// des variables en complement de la construction de variables multi-table et des arbres
	// Ce cout est a prendre en compte dans la methode ComputeDataGridCost
	// Parametrage d'un cout de selection de famille de modele (defaut: 0)
	void SetModelFamilySelectionCost(double dValue);
	virtual double GetModelFamilySelectionCost() const;

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attribut informatif sont ceux ayant strictement plus de une partie
	virtual double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
					   int nInformativeAttributeNumber) const;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	virtual double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const;

	// Calcul du cout local d'une partie
	virtual double ComputePartCost(const KWDGPart* part) const;

	// Calcul du cout local de l'union de deux parties
	virtual double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const;

	// Calcul du cout local d'une cellule
	virtual double ComputeCellCost(const KWDGCell* cell) const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul du cout total d'une grille de donnees
	//	- cout de la grille
	//  - cout des attributs
	//  - cout des parties
	//  - cout des valeurs pour les attributs symboliques
	//  - cout des cellules
	// Une initialisation prealable est necessaire pour evaluer une fois pour toutes
	// les couts par defaut (cout par attribut non informatif, cout des valeurs symboliques)
	// Le cout global peut alors etre evalue y compris sur une grille partielle,
	// contenant un sous-ensemble des attribut

	// Taux de compression d'une grille (1 - TotalCost/TotalDefaultCost)
	double ComputeDataGridCompressionCoefficient(const KWDataGrid* dataGrid) const;

	// Calcul du cout total d'un DataGrid
	double ComputeDataGridTotalCost(const KWDataGrid* dataGrid) const;

	// Calcul du cout total d'un DataGridMerger
	// Similaire au cout total d'un DataGrid, en utilisant les cout bufferises
	// dans les methodes GetCost() des entites du DataGridMerger
	// Des verifications sont effectuees en modes debug
	double ComputeDataGridMergerTotalCost(const KWDataGridMerger* dataGridMerger) const;

	// Verification de la consistance d'un DataGrid avec la structure de couts constants
	boolean CheckDataGrid(const KWDataGrid* dataGrid) const;

	// Initialisation des couts constants pour une grille de donnees
	void InitializeDefaultCosts(const KWDataGrid* dataGrid);
	void CleanDefaultCosts();
	boolean IsInitialized() const;

	// Acces aux cout total de la grille par defaut
	double GetTotalDefaultCost() const;

	// Acces aux cout constant par attribut non informatif (cout attribut plus celui de son unique partie)
	int GetTotalAttributeNumber() const;
	const ALString& GetAttributeNameAt(int nIndex) const;
	double GetAttributeDefaultCostAt(int nIndex) const;

	// Acces aux cout des valeurs symboliques
	double GetAllValuesDefaultCost() const;

	// Calcul du cout par defaut des attributs (et parties) absent d'une grille
	double ComputeDataGridTotalMissingAttributeCost(const KWDataGrid* dataGrid) const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts cumulatifs par entite de la grille des donnees
	// Ces couts agregent les couts des sous-entites, mais ne prennent pas en compte:
	//    - les couts des valeurs symboliques
	//    - les cout des attributs hors DataGrid

	// Calcul du cout cumulatif du DataGrid (avec ses attributs, parties et cellules)
	double ComputeDataGridCumulativeCost(const KWDataGrid* dataGrid) const;

	// Calcul du cout cumulatif d'un attribut (avec ses parties et cellules)
	double ComputeAttributeCumulativeCost(const KWDGAttribute* attribute) const;

	// Calcul du cout cumulatif de la partie (avec ses cellules)
	double ComputePartCumulativeCost(const KWDGPart* part) const;

	/////////////////////////////////////////////////////////////////////////
	// Gestion des couts par valeur d'une partie d'un attribut symbolique
	// Ces couts ne sont pas pris en compte dans l'optimisation d'une grille,
	// car ils sont independant de la structure de la grille: il ne constituent
	// qu'un surcout constant.
	// Ils ne sont ici geres que pour les rapports, en permettant l'utilisation
	// de critere d'evaluation complet et normalise

	// Calcul du cout local d'une valeur d'un attribut symbolique
	virtual double ComputeValueCost(const KWDGValue* value) const;

	// Calcul du cout global (constant) de toutes les valeurs du DataGrid
	virtual double ComputeDataGridAllValuesCost(const KWDataGrid* dataGrid) const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts de lie au modele ou aux donnees
	// Utilisation a titre informatif pour distribuer les cout totaux utilises
	// dans les algorithmes, soit sur la partie modele, soit sur la partie donnees

	// Cout de modele par entite
	// Methodes a redefinir (par defaut: 0)
	virtual double ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
						int nInformativeAttributeNumber) const;
	virtual double ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const;
	virtual double ComputePartModelCost(const KWDGPart* part) const;
	virtual double ComputeCellModelCost(const KWDGCell* cell) const;
	virtual double ComputeValueModelCost(const KWDGValue* value) const;

	// Cout de construction par entite
	// Methodes a redefinir (par defaut: 0)
	virtual double ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
						       int nInformativeAttributeNumber) const;
	virtual double ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const;
	virtual double ComputePartConstructionCost(const KWDGPart* part) const;
	virtual double ComputeCellConstructionCost(const KWDGCell* cell) const;
	virtual double ComputeValueConstructionCost(const KWDGValue* value) const;

	// Cout des preparation par entite (deduit de PreparationCost=ModelCost-ConstructionCost)
	double ComputeDataGridPreparationCost(const KWDataGrid* dataGrid, double dLnGridSize,
					      int nInformativeAttributeNumber) const;
	double ComputeAttributePreparationCost(const KWDGAttribute* attribute, int nPartitionSize) const;
	double ComputePartPreparationCost(const KWDGPart* part) const;
	double ComputeCellPreparationCost(const KWDGCell* cell) const;
	double ComputeValuePreparationCost(const KWDGValue* value) const;

	// Cout des donnees connaissant le modele par entite (deduit de DataCost=Cost-ModelCost)
	double ComputeDataGridDataCost(const KWDataGrid* dataGrid, double dLnGridSize,
				       int nInformativeAttributeNumber) const;
	double ComputeAttributeDataCost(const KWDGAttribute* attribute, int nPartitionSize) const;
	double ComputePartDataCost(const KWDGPart* part) const;
	double ComputeCellDataCost(const KWDGCell* cell) const;
	double ComputeValueDataCost(const KWDGValue* value) const;

	// Calcul du cout de model et des donnees d'un DataGrid
	// On fait l'hypothese que les couts de modeles sont nuls pour les attributs non utilises
	// (ce qui evite une methode analogue a ComputeDataGridTotalMissingAttributeCost) et que
	// les couts de modele des valeurs sont null (ce qui evite l'analogue de GetAllValuesDefaultCost)
	double ComputeDataGridTotalModelCost(const KWDataGrid* dataGrid) const;
	double ComputeDataGridTotalConstructionCost(const KWDataGrid* dataGrid) const;
	double ComputeDataGridTotalPreparationCost(const KWDataGrid* dataGrid) const;
	double ComputeDataGridTotalDataCost(const KWDataGrid* dataGrid) const;

	/////////////////////////////////////////////////////////////////////////
	// Gestion des strutures de cout non additives, avec des cout de partie
	// source qui dependent du nombre de partie de l'attribut cible

	// Calcul de la variation de cout local d'une partie suite a decrementation du nombre de parties cibles
	// Utile pour les couts non additif, quand le cout des partie sources depend du nombre de parties cibles
	// Correspond ComputePartCost() [CurrentTargetPartNumber-1] - ComputePartCost() [CurrentTargetPartNumber]
	// Permet d'optimiser les calcul de couts
	virtual double ComputePartTargetDeltaCost(const KWDGPart* part) const;

	///////////////////////////////////////////////////////////////////////////
	// Affichage des couts par entite

	// Affichage des couts de toutes les entites du DataGrid
	void WriteDataGridAllCosts(const KWDataGrid* dataGrid, ostream& ost) const;
	void WriteDataGridCost(const KWDataGrid* dataGrid, ostream& ost) const;

	// Affichage du cout d'un attribut
	void WriteAttributeCostHeaderLine(const KWDGAttribute* attribute, ostream& ost) const;
	void WriteAttributeCostLine(const KWDGAttribute* attribute, ostream& ost) const;

	// Affichage du cout d'une partie
	void WritePartCostHeaderLine(const KWDGPart* part, ostream& ost) const;
	void WritePartCostLine(const KWDGPart* part, ostream& ost) const;

	// Affichage du cout d'une cellule
	void WriteCellCostHeaderLine(const KWDGCell* cell, ostream& ost) const;
	void WriteCellCostLine(const KWDGCell* cell, ostream& ost) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////
	//// Implementation
protected:
	// Cout de selection de la famille de modeles
	double dModelFamilySelectionCost;

	// Memorisation de la structure des couts par defaut
	double dTotalDefaultCost;
	KWDataGridMerger* dataGridDefaultCosts;
	double dAllValuesDefaultCost;
	static const double dEpsilon;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas de la classification supervisee
class KWDataGridClassificationCosts : public KWDataGridCosts
{
public:
	// Constructeur
	KWDataGridClassificationCosts();
	~KWDataGridClassificationCosts();

	// Duplication
	KWDataGridCosts* Clone() const override;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attributs informatifs sont ceux ayant strictement plus de une partie
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	// Le nombre de parties est le nombre total de parties.
	// En cas de presence d'une poubelle il s'agit du nombre de parties informatives de l'attribut + 1
	double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const override;

	// Calcul du cout local d'une partie
	double ComputePartCost(const KWDGPart* part) const override;

	// Calcul du cout local de l'union de deux parties
	double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const override;

	// Calcul du cout local d'une cellule
	double ComputeCellCost(const KWDGCell* cell) const override;

	// Cout de modele par entite
	double ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
					       int nInformativeAttributeNumber) const override;
	double ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
					int nInformativeAttributeNumber) const override;
	double ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputePartModelCost(const KWDGPart* part) const override;
	double ComputeCellModelCost(const KWDGCell* cell) const override;
	double ComputeValueModelCost(const KWDGValue* value) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas du clustering
class KWDataGridClusteringCosts : public KWDataGridCosts
{
public:
	// Constructeur
	KWDataGridClusteringCosts();
	~KWDataGridClusteringCosts();

	// Duplication
	KWDataGridCosts* Clone() const override;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attributs informatifs sont ceux ayant strictement plus de une partie
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	// Le nombre de parties est le nombre total de parties.
	// En cas de presence d'une poubelle il s'agit du nombre de parties informatives de l'attribut + 1
	double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const override;

	// Calcul du cout local d'une partie
	double ComputePartCost(const KWDGPart* part) const override;

	// Calcul du cout local de l'union de deux parties
	double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const override;

	// Calcul du cout local d'une cellule
	double ComputeCellCost(const KWDGCell* cell) const override;

	// Calcul du cout local d'une valeur d'un attribut symbolique
	double ComputeValueCost(const KWDGValue* value) const override;

	// Cout de modele par entite
	double ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
					       int nInformativeAttributeNumber) const override;
	double ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
					int nInformativeAttributeNumber) const override;
	double ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputePartModelCost(const KWDGPart* part) const override;
	double ComputeCellModelCost(const KWDGCell* cell) const override;
	double ComputeValueModelCost(const KWDGValue* value) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas de la regression
class KWDataGridRegressionCosts : public KWDataGridCosts
{
public:
	// Constructeur
	KWDataGridRegressionCosts();
	~KWDataGridRegressionCosts();

	// Duplication
	KWDataGridCosts* Clone() const override;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees
	// Implemente uniquement pour l'analyse univariee, avec un attribut source et
	// un attribut cible categoriel

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attributs informatifs sont ceux ayant strictement plus de une partie
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	// Le nombre de parties est le nombre total de parties.
	// En cas de presence d'une poubelle il s'agit du nombre de parties informatives de l'attribut + 1
	double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const override;

	// Calcul du cout local d'une partie
	double ComputePartCost(const KWDGPart* part) const override;

	// Calcul du cout local de l'union de deux parties
	double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const override;

	// Calcul de la variation de cout local d'une partie suite a decrementation du nombre de parties cibles
	double ComputePartTargetDeltaCost(const KWDGPart* part) const override;

	// Calcul du cout local d'une cellule
	double ComputeCellCost(const KWDGCell* cell) const override;

	// Cout de modele par entite
	double ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
					       int nInformativeAttributeNumber) const override;
	double ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
					int nInformativeAttributeNumber) const override;
	double ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputePartModelCost(const KWDGPart* part) const override;
	double ComputeCellModelCost(const KWDGCell* cell) const override;
	double ComputeValueModelCost(const KWDGValue* value) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas de la classification
// avec groupement des valeurs cibles
class KWDataGridGeneralizedClassificationCosts : public KWDataGridCosts
{
public:
	// Constructeur
	KWDataGridGeneralizedClassificationCosts();
	~KWDataGridGeneralizedClassificationCosts();

	// Duplication
	KWDataGridCosts* Clone() const override;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees
	// Implemente uniquement pour l'analyse univariee, avec un attribut source et
	// un attrribut cible categoriel

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attributs informatifs sont ceux ayant strictement plus de une partie
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	// Le nombre de parties est le nombre total de parties.
	// En cas de presence d'une poubelle il s'agit du nombre de parties informatives de l'attribut + 1
	double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const override;

	// Calcul du cout local d'une partie
	double ComputePartCost(const KWDGPart* part) const override;

	// Calcul du cout local de l'union de deux parties
	double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const override;

	// Calcul de la variation de cout local d'une partie suite a decrementation du nombre de parties cibles
	double ComputePartTargetDeltaCost(const KWDGPart* part) const override;

	// Calcul du cout local d'une cellule
	double ComputeCellCost(const KWDGCell* cell) const override;

	// Calcul du cout local d'une valeur d'un attribut symbolique
	double ComputeValueCost(const KWDGValue* value) const override;

	// Calcul du cout global (constant) de toutes les valeurs du DataGrid
	double ComputeDataGridAllValuesCost(const KWDataGrid* dataGrid) const override;

	// Cout de modele par entite
	double ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
					       int nInformativeAttributeNumber) const override;
	double ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
					int nInformativeAttributeNumber) const override;
	double ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const override;
	double ComputePartModelCost(const KWDGPart* part) const override;
	double ComputeCellModelCost(const KWDGCell* cell) const override;
	double ComputeValueModelCost(const KWDGValue* value) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};
