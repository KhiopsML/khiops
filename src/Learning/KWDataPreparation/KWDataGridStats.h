// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridStats;
class KWDGSAttributePartition;
class KWDGSAttributeDiscretization;
class KWDGSAttributeGrouping;
class KWDGSAttributeContinuousValues;
class KWDGSAttributeSymbolValues;
class KWDGSAttributeVirtualValues;
class KWDGSCell;
class PLShared_DataGridStats;
class PLShared_DGSAttributeDiscretization;
class PLShared_DGSAttributeGrouping;
class PLShared_DGSAttributeContinuousValues;
class PLShared_DGSAttributeSymbolValues;

#include "Vector.h"
#include "KWType.h"
#include "KWSymbol.h"
#include "KWContinuous.h"
#include "TSV.h"
#include "KWFrequencyVector.h"
#include "KWClass.h"
#include "JSONFile.h"
#include "PLSharedObject.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridStats
// Structure de donnees permettant de representer l'ensemble des enregistrements
// d'une base de donnees, sous forme d'un produit cartesien des attributs.
// Attension, cette classe est adaptee aux grilles pleines, dont le nombre de
// cellules non vides est de l'ordre de grandeurs du nombre de cellules.
//
// Cette classe est le pendant en version figee de la classe KWDataGrid, qui est
// elle permet les evolutions dynamiques des grilles de donnees et est adaptee
// aux algorithmes d'optimisation.
// Cette structure est particulierement adaptee au stockage compact des donnees,
// notamment en vue de la memorisation des resultats d'optimisation et de la
// production de rapports.
class KWDataGridStats : public Object
{
public:
	// Constructeur
	KWDataGridStats();
	~KWDataGridStats();

	/////////////////////////////////////////////////////////////////////////
	// Specification des donnees d'un DataGridStats
	// Un DataGridStats est defini par ses attributs et les effectifs
	// de ses cellules
	//
	// La specification d'un DataGrid se fait selon les etapes suivantes:
	//   - ajout des attributs, par la methodes AddAttribute
	//   - specification du nombre d'attribut en entree
	//   - specification des effectifs des cellules
	// Cette structure est faite pour etre initialisee une seule fois: toute
	// mise a jour est couteuse et revient a tout recommencer depuis le depart.
	//
	// La structure est optimisee en occupation memoire, pas en exploitation des
	// donnees. Elle doit etre reservee essentiellement a la production de rapports,
	// a la memorisation de resultats, et au transfert avec les classes
	// manipulant des grilles de donnees (optimisation, table de probabilites
	// conditionnelles, regles de derivations).
	// Ces classes assurent la conversion par des methodes
	//    ImportDataGridStats(const KWDataGridStats* dataGridStats)
	//    ExportDataGridStats(KWDataGridStats* dataGridStats)
	//
	// Memoire: l'integralite de la structure du DataGrid est geree par l'objet courant

	// Memorisation de la granularite du DataGrid
	void SetGranularity(int nValue);
	int GetGranularity() const;

	////////////////////////////////////////////////////////////////
	// Creation des attributs du DataGrid

	// Ajout d'un attribut de grille (sous-classe de KWDGSAttributePartition), en fin de liste
	// L'attribut doit etre specifie et valide, et ne pourra plus etre modifie
	// Les effectifs des cellules sont automatiquement nettoyes
	void AddAttribute(const KWDGSAttributePartition* attribute);

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Specification du nombre d'attributs source
	//   Par defaut 0: grille non supervisee
	//   N-1: classification supervisee ou regression selon le type du dernier attribut
	void SetSourceAttributeNumber(int nValue);
	int GetSourceAttributeNumber() const;

	// Acces aux attribut cibles, qui sont necessairement apres les attributs source
	int GetFirstTargetAttributeIndex() const;
	int GetTargetAttributeNumber() const;

	// Nombre d'attribut utiles pour la prediction: les attributs sources dans le cas
	// supervise, tous les attribut dans le cas non supervise
	// Le premier attribut de prediction est donc toujours d'index 0
	int GetPredictionAttributeNumber() const;

	// Acces aux attributs du DataGrid
	// L'appelant se sert de ces points d'entree pour consulter la structure du DataGrid
	const KWDGSAttributePartition* GetAttributeAt(int nAttributeIndex) const;

	// Recherche d'un attribut par son nom (recherche couteuse, par parcours exhaustif des attributs)
	// Renvoie NULL si non trouve
	const KWDGSAttributePartition* SearchAttribute(const ALString& sAttributeName) const;

	// Nettoyage complet: on se retrouve dans l'etat initial
	void DeleteAll();

	// Verification de la specification des attributs
	//   - validite unitaire de chaque attribut
	//   - unicite des noms des attributs
	//   - taille maximum de la grille
	boolean CheckAttributes() const;

	////////////////////////////////////////////////////////////////
	// Mise-a-jour des cellules du DataGrid
	// Methodes a utiliser quand tous les attributs sont correctement initialises
	// L'effectif des cellule ne peut etre mise a jour qu'une seule fois, et
	// doit donc etre a 0 avant mise a jour

	// Creation de toutes les cellules, avec un effectif de 0 par defaut
	void CreateAllCells();

	// Mise a jour des effectifs des cellules
	void SetCellFrequencyAt(const IntVector* ivPartIndexes, int nFrequency);
	int GetCellFrequencyAt(const IntVector* ivPartIndexes) const;

	// Mise a jour des effectifs des cellules, dans le cas univarie
	void SetUnivariateCellFrequencyAt(int nPart, int nFrequency);
	int GetUnivariateCellFrequencyAt(int nPart) const;

	// Mise a jour des effectifs des cellules, dans le cas bivarie
	void SetBivariateCellFrequencyAt(int nPart1, int nPart2, int nFrequency);
	int GetBivariateCellFrequencyAt(int nPart1, int nPart2) const;

	// Verification de la validite des cellules
	boolean CheckCells() const;

	// Destruction de toutes les cellules
	void DeleteAllCells();

	////////////////////////////////////////////////////////////////
	// Statistiques sur la grille
	// Methodes potentiellement couteuses en calcul

	// Effectif de la grille
	int ComputeGridFrequency() const;

	// Nombre de cellules (non vides)
	int ComputeSourceCellNumber() const;
	int ComputeTotalCellNumber() const;

	// Taille de la grille en nombre total de cellules
	// Il s'agit du produit des nombres de parties par attribut
	int ComputeSourceGridSize() const;
	int ComputeTargetGridSize() const;
	int ComputeTotalGridSize() const;

	// Calcul du nombre d'attribut informatifs (ayant strictement plus de une partie)
	// Nombre d'attributs source dans le cas supervise, cibles sinon
	int ComputeSourceInformativeAttributeNumber() const;
	int ComputeTotalInformativeAttributeNumber() const;

	// Redirection des methodes en supervise sur la version Source, en
	// non supervise sur la version Total
	int ComputeCellNumber() const;
	int ComputeGridSize() const;
	int ComputeInformativeAttributeNumber() const;

	////////////////////////////////////////////////////////////////////////
	// Methodes de reporting
	// Ces methodes sont utilisee dans tous les contextes permis par les grilles
	//   . Supervise: un attribut cible symbolique (classification) ou continuous (regression)
	//       - preparation univariee: discretisation ou groupage
	//       - preparation bivariee: grille 2D
	//       - preparation multivariee: grille KD
	//   . Non supervise
	//       - preparation multivariee: grille KD
	//   . Cas general: nombre quelconque d'attribut sources et cibles
	//
	// Dans le cas supervise, les details sont donnes par attribut pour les attribut sources,
	// par valeur pour les attribut cibles (avec les probabilites conditionnelles)
	// Dans le cas non supervise, les details sont donnes par attribut, avec des effectifs
	// (les probabilites jointes sont trop "creuses" pour etre intelligibles).
	//
	// Les rapports s'adaptent aux caracteristiques des grilles
	//    . distinction des cas univarie, bivarie et multivarie
	//    . presentation des variables sous forme liste uniquement dans le cas multivarie
	//    . pas de detail sur les variables cibles dans le cas supervise (car factorise)
	//    . detail sur les parties uniquement dans le cas du groupement de valeurs (sauf si multivarie)
	//    . affichage des tableaux croises dans le cas bivarie
	//    ...
	// Les methodes Write... booleennes indiquent en retour si au moins une information a ete ecrite
	// (suite a la nature des donnees et a des choix de presentation).
	// Les autres methodes ecrivent systematiquement des informations.

	// Affichage global de toutes les informations
	void Write(ostream& ost) const override;

	// Affichage par defaut: uniquement source dans le cas supervise, tout sinon
	void WriteDefault(ostream& ost) const;

	// Affichage en choississant les details sur les attributs sources et/ou cibles
	// Les attributs cibles sont distingues des attribuuts source par l'utilisation d'un libelle "Target"
	// uniquement s'il existe au moins un attribut source (sinon, dans le cas non supervise, tous
	// les attributs sont cible et "Target" est sous-entendu).
	// On peut neamoins forcer l'utilisation de "Target" meme en non supervise en utilisant
	// la combinaison (bSource and bTarget) meme s'il n'y a pas d'attributs sources
	void WritePartial(ostream& ost, boolean bSource, boolean bTarget) const;

	// Affichage des attributs
	//   (Type, Name, Parts)
	boolean WriteAttributeArrayLineReports(ostream& ost, boolean bSource, boolean bTarget) const;

	// Affichage des parties des attributs, sources dans le cas supervise, tous sinon
	boolean WriteAttributePartArrayLineReports(ostream& ost, boolean bSource, boolean bTarget) const;
	void WriteAttributePartArrayLineReportAt(ostream& ost, int nAttribute, boolean bWriteFrequencies) const;

	// Affichage des effectifs par cellule dans un tableau croise
	// Dans chaque cellule du tableau, on affichage la proportion de la classe en parametre
	// Methode utilisable uniquement dans le cas a deux attributs sources ou deux attributs non supervises
	void WriteFrequencyCrossTable(ostream& ost) const;

	// Affichage des statistiques par classe cible dans un tableau croise
	// Dans chaque cellule du tableau, on affichage la proportion de la classe en parametre
	// Methode utilisable uniquement dans le cas a deux attributs sources et un attribut cible
	void WriteTargetStatsCrossTableAt(ostream& ost, int nTargetIndex) const;

	// Affichage des detail des cellules
	// Dans le cas supervise, chaque cellule est caracterisee par ses parties sur les attributs sources
	// et ses proportions par valeur sur les attributs cibles
	// Dans le cas supervise, chaque cellule est caracterisee par ses parties sur tous les attributs
	// (Part1, Part2,..., PartK, [TargetVal1, TargetVal2...,] Frequency, Coverage)
	void WriteCellArrayLineReport(ostream& ost) const;

	// Parametrage de la valeur cible principale par son index (defaut: -1)
	// Cela permet une personnalisation de certains rapports
	// (uniquement s'il y a un seul attribut cible et si cet index est valide)
	void SetMainTargetModalityIndex(int nValue);
	int GetMainTargetModalityIndex() const;

	// Verification de la validite de l'index de valeur cible
	boolean CheckMainTargetModalityIndex() const;

	// Affichage de la valeur cible principale
	void WriteMainTargetModality(ostream& ost) const;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON
	//
	// Il faut avoir parametrer un vecteur des bornes inf et sup de domaine numerique
	// par attribut pour permettre l'ecritures des partition en intervalles, avec les valeurs
	// des bornes extremes (soit bornes de domaines, soit valeur min et max par attribut).
	// Ces vecteur appartienne a l'appelant, et ne doivent etre parametre que le temps
	// de l'appel des methodes d'ecriture JSON

	// Vecteur des bornes inf et sup de domaine numerique par attribut
	void SetJSONAttributeDomainLowerBounds(const ContinuousVector* cvValues);
	const ContinuousVector* GetJSONAttributeDomainLowerBounds() const;

	// Vecteur des valeurs max par attribut
	void SetJSONAttributeDomainUpperBounds(const ContinuousVector* cvValues);
	const ContinuousVector* GetJSONAttributeDomainUpperBounds() const;

	// Ecriture du contenu d'un rapport JSON
	virtual void WriteJSONFields(JSONFile* fJSON);

	// Ecriture d'un rapport JSON
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	virtual void WriteJSONReport(JSONFile* fJSON);
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey);

	// Ecriture des effectifs des valeurs, dans le cas particulier d'une grille reduite
	// a un attribut de valeurs categorielles
	// On ecrit un objet, puis une suite de paire (valeur: effectif)
	virtual void WriteJSONKeyValueFrequencies(JSONFile* fJSON, const ALString& sKey);

	///////////////////////////////
	// Services divers

	// Verification d'un vecteur d'index par rapport a la specifications des attributs
	boolean CheckPartIndexes(const IntVector* ivPartIndexes) const;

	// Service de transcodage entre un vecteur d'index de parties et un index globale de cellule
	// La taille du vecteur d'index doit etre le nombre total d'attributs
	int ComputeCellIndex(const IntVector* ivPartIndexes) const;
	void ComputePartIndexes(int nCellIndex, IntVector* ivPartIndexes) const;

	// Services similaires avec les cellules source
	// La taille du vecteur d'index doit etre au minimum le nombre d'attributs sources
	boolean CheckSourcePartIndexes(const IntVector* ivPartIndexes) const;
	int ComputeSourceCellIndex(const IntVector* ivPartIndexes) const;
	void ComputeSourcePartIndexes(int nCellIndex, IntVector* ivPartIndexes) const;

	// Services similaires avec les cellules cibles
	// La taille du vecteur d'index doit etre le nombre total d'attributs
	boolean CheckTargetPartIndexes(const IntVector* ivPartIndexes) const;
	int ComputeTargetCellIndex(const IntVector* ivPartIndexes) const;
	void ComputeTargetPartIndexes(int nCellIndex, IntVector* ivPartIndexes) const;

	// Export des cellules non vides dans un tableau de KWDGSCell
	// Le tableau resultat doit etre a vide en entree
	// Memoire: le contenu du tableau appartient a l'appelant
	void ExportAllCells(ObjectArray* oaCells) const;

	// Export des cellules non vides sources uniquement dans un tableau de KWDGSSourceCell,
	// pour un attribut cible donne
	// Leur index se base sur les attributs sources (les premiers de la grille)
	// Memoire: le contenu du tableau appartient a l'appelant
	void ExportSourceCellsAt(ObjectArray* oaSourceCells, int nTargetAttribute) const;

	// Calcul des effectifs par partie pour un attribut de la grille
	// Le resultat est rendu dans le vecteur d'entier en parametre
	void ExportAttributePartFrequenciesAt(int nAttribute, IntVector* ivPartFrequencies) const;

	// Export d'une grille reduite a un attribut, c'est a dire une partition inivariee et ses effectifs
	// Le resultat est rendu dans la grille en parametre
	void ExportAttributeDataGridStatsAt(int nAttribute, KWDataGridStats* univariateDataGridStats) const;

	// Verification des effectifs des parties, qui doivent toutes etre non vides
	boolean CheckPartFrequencies() const;

	// Verification de la validite de la structure du DataGrid
	// Controle d'integrite global (attributs, parties, cellules...)
	// Attention: operation couteuse en O(k.n^2)
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Duplication (s'appuie sur Create et CopyFrom)
	KWDataGridStats* Clone() const;

	// Recopie
	void CopyFrom(const KWDataGridStats* kwdgsSource);

	// Comparaison
	int Compare(const KWDataGridStats* kwdgsSource) const;

	// Export d'un libelle contenant la liste des variables
	const ALString ExportVariableNames() const;

	// Import des nom des variables a partir d'un libelle (exporte)
	// Retourne true en cas de validite du libelle a importer et de sa coherence avec les variables de la grille
	boolean ImportVariableNames(const ALString& sValue);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un DataGrid
	static KWDataGridStats* CreateTestDataGrid(int nSymbolAttributeNumber, int nContinuousAttributeNumber,
						   boolean bSingletonPartAttributes, int nSourceAttributeNumber,
						   int nAttributePartNumber, int nMaxCellInstanceNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	//// Implementation
protected:
	// Recherche du prochain caractere non blanc a partir d'une position
	// Retourne -1 si non trouve
	int FindNextInformativeChar(const ALString& sSearchedString, int nStartPos) const;

	// Affichage des detail des cellules, dans les cas supervise et non supervise
	void WriteUnsupervisedCellArrayLineReport(ostream& ost) const;
	void WriteSupervisedCellArrayLineReport(ostream& ost) const;

	// Tri des cellules sources pour un attribut cible donne
	// Methode reimplementable pour personnaliser les rapports,
	// en particulier l'ordonnancement des parties de type groupes de valeur
	virtual void SortSourceCells(ObjectArray* oaSourceCells, int nTargetAttributeIndex) const;

	// Services d'indexation des cellules avec un nombre quelconque d'attributs
	boolean InternalCheckPartIndexes(const IntVector* ivPartIndexes, int nFirstAttributeIndex,
					 int nLastAttributeIndex) const;
	int InternalComputeCellIndex(const IntVector* ivPartIndexes, int nFirstAttributeIndex,
				     int nLastAttributeIndex) const;
	void InternalComputePartIndexes(int nCellIndex, IntVector* ivPartIndexes, int nFirstAttributeIndex,
					int nLastAttributeIndex) const;

	// Tableau des attributs du DataGrid
	ObjectArray oaAttributes;

	// Nombre d'attributs sources
	int nSourceAttributeNumber;

	// Vecteur des effectifs des cellules
	IntVector ivCellFrequencies;

	// Index de la valeur cible principale
	int nMainTargetModalityIndex;

	// Granularite
	int nGranularity;

	// Vecteur des valeurs min et max par intervalle, pour l'ecriture des discretisation JSON
	const ContinuousVector* cvJSONAttributeDomainLowerBounds;
	const ContinuousVector* cvJSONAttributeDomainUpperBounds;

	friend class PLShared_DataGridStats;
};

// Comparaison de deux grilles de donnees, sur les nom des attributs de la grille
int KWDataGridStatsCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributePartition
// Attribut d'un DataGridStats
// L'attribut est caracterise principalement par:
//          Nom de l'attribut
//			Type de l'attribut (Symbol ou Continuous)
//          Nature des parties (singleton ou non)
//			Nombre de parties de l'attribut
//			Liste des parties de l'attribut
class KWDGSAttributePartition : public Object
{
public:
	// Constructeur
	KWDGSAttributePartition();
	~KWDGSAttributePartition();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Nom de l'attribut
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	// Type de l'attribut (Symbol ou Continuous)
	virtual int GetAttributeType() const = 0;

	// Nature des parties (singleton ou non)
	virtual boolean ArePartsSingletons() const = 0;

	// Nombre de parties
	// La specification des parties se fait dans les sous-classes
	virtual void SetPartNumber(int nValue) = 0;
	virtual int GetPartNumber() const = 0;

	// Acces au nombre initial de modalites
	void SetInitialValueNumber(int nValue);
	int GetInitialValueNumber() const;

	// Acces au nombre de modalites apres granularisation
	// Dans le cas numerique, il s'agit de Ng, le nombre theorique = 2 ^ granularite
	// Dans le cas categoriel, il s'agit de Vg, le nombre de partiles obtenu effectivement
	void SetGranularizedValueNumber(int nValue);
	int GetGranularizedValueNumber() const;

	///////////////////////////////
	// Services divers

	// Recherche de l'index d'une partie pour une valeur donnee
	// Methodes potentiellement couteuses en calcul
	virtual int ComputeContinuousPartIndex(Continuous cValue) const;
	virtual int ComputeSymbolPartIndex(const Symbol& sValue) const;

	// Controle d'integrite local a l'attribut
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage complet
	void Write(ostream& ost) const override;

	// Affichage uniquement du libelle d'une partie
	virtual void WritePartHeader(ostream& ost) const;
	virtual void WritePartAt(ostream& ost, int nPartIndex) const;

	// Affichage uniquement du libelle pour les details d'une partie (par defaut: aucun details)
	virtual boolean IsPartDetailsReported() const;
	virtual void WritePartDetailsHeader(ostream& ost) const;
	virtual void WritePartDetailsAt(ostream& ost, int nPartIndex) const;

	// Ecriture du contenu d'un rapport JSON
	virtual void WriteJSONFields(JSONFile* fJSON);

	// Ecriture du contenu d'une partie JSON
	// Soit une seule valeur, soit la description d'une partie
	virtual void WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex);

	// Ecriture d'un rapport JSON
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	virtual void WriteJSONReport(JSONFile* fJSON);
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey);

	// Creation generique
	virtual KWDGSAttributePartition* Create() const = 0;

	// Duplication (s'appuie sur Create et CopyFrom)
	KWDGSAttributePartition* Clone() const;

	// Recopie
	virtual void CopyFrom(const KWDGSAttributePartition* kwdgsapSource);

	// Comparaison
	virtual int Compare(const KWDGSAttributePartition* kwdgsapSource) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	ALString sAttributeName;

	int nInitialValueNumber;
	int nGranularizedValueNumber;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeDiscretization
// Attribut d'un DataGridStats, discretisation d'un attribut continu
class KWDGSAttributeDiscretization : public KWDGSAttributePartition
{
public:
	// Constructeur
	KWDGSAttributeDiscretization();
	~KWDGSAttributeDiscretization();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Redefinition des methodes virtuelles
	int GetAttributeType() const override;
	boolean ArePartsSingletons() const override;
	void SetPartNumber(int nValue) override;
	int GetPartNumber() const override;
	int ComputeContinuousPartIndex(Continuous cValue) const override;
	boolean Check() const override;
	longint GetUsedMemory() const override;
	void WritePartHeader(ostream& ost) const override;
	void WritePartAt(ostream& ost, int nPartIndex) const override;
	KWDGSAttributePartition* Create() const override;
	void CopyFrom(const KWDGSAttributePartition* kwdgsapSource) override;
	int Compare(const KWDGSAttributePartition* kwdgsapSource) const override;
	const ALString GetClassLabel() const override;

	// Methode d'ecriture JSON specifique, avec les valeurs min et max de l'attribut
	void WriteJSONFieldsWithBounds(JSONFile* fJSON, Continuous cMin, Continuous cMax);
	void WriteJSONPartFieldsAtWithBounds(JSONFile* fJSON, int nPartIndex, Continuous cMin, Continuous cMax);

	// Mise a jour des bornes des intervalles
	// Attention: le nombre de bornes est egal au nombre d'intervalles moins un
	int GetIntervalBoundNumber() const;
	void SetIntervalBoundAt(int nIndex, Continuous cValue);
	Continuous GetIntervalBoundAt(int nIndex) const;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un attribut test
	static KWDGSAttributeDiscretization* CreateTestAttribute(int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	ContinuousVector cvIntervalBounds;
	friend class PLShared_DGSAttributeDiscretization;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeGrouping
// Attribut d'un DataGridStats, groupement des valeurs d'un attribut Symbol
class KWDGSAttributeGrouping : public KWDGSAttributePartition
{
public:
	// Constructeur
	KWDGSAttributeGrouping();
	~KWDGSAttributeGrouping();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Redefinition des methodes virtuelles
	int GetAttributeType() const override;
	boolean ArePartsSingletons() const override;
	void SetPartNumber(int nValue) override;
	int GetPartNumber() const override;
	int ComputeSymbolPartIndex(const Symbol& sValue) const override;
	boolean Check() const override;
	longint GetUsedMemory() const override;
	void WritePartHeader(ostream& ost) const override;
	void WritePartAt(ostream& ost, int nPartIndex) const override;
	boolean IsPartDetailsReported() const override;
	void WritePartDetailsHeader(ostream& ost) const override;
	void WritePartDetailsAt(ostream& ost, int nPartIndex) const override;
	void WriteJSONFields(JSONFile* fJSON) override;
	void WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) override;
	KWDGSAttributePartition* Create() const override;
	void CopyFrom(const KWDGSAttributePartition* kwdgsapSource) override;
	int Compare(const KWDGSAttributePartition* kwdgsapSource) const override;
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////////
	// Specifications des groupes de valeurs
	// On specifie d'une part les valeurs dans un tableau,
	// d'autre part pour chaque groupe l'index de sa premiere
	// valeur dans le tableau de valeur

	// Nombre de valeurs memorisees apres nettoyage eventuel
	// En presence d'un fourre-tout (avec ou sans poubelle), seule la modalite d'effectif le plus eleve est
	// conservee dans le fourre-tout + StarValue En presence d'un groupe poubelle sans fourre-tout, seule la
	// modalite d'effectif le plus eleve est conservee dans la poubelle + StarValue
	void SetKeptValueNumber(int nValue);
	int GetKeptValueNumber() const;

	// Specification des valeurs
	// Une au moins d'entres-elles doit etre la StarValue
	// En presence d'un fourre-tout (avec ou sans poubelle), la StarValue est dans le fourre-tout
	// En presence d'un groupe poubelle sans fourre-tout, la StarValue est dans le groupe poubelle
	// En l'absence de groupe poubelle et de fourre-tout, la StarValue est dans le groupe de la modalite de plus
	// faible effectif
	void SetValueAt(int nIndex, const Symbol& sValue);
	Symbol& GetValueAt(int nIndex) const;

	// Specification des groupes de valeurs par l'index de leur premiere valeur
	int GetGroupNumber() const;
	void SetGroupFirstValueIndexAt(int nGroupIndex, int nFirstValueIndex);
	int GetGroupFirstValueIndexAt(int nGroupIndex) const;

	// Informations complementaires sur le groupe
	int GetGroupLastValueIndexAt(int nGroupIndex) const;
	int GetGroupValueNumberAt(int nGroupIndex) const;

	// Acces a l'index du groupe poubelle
	// Vaut -1 en l'absence de groupe poubelle
	void SetGarbageGroupIndex(int nValue);
	int GetGarbageGroupIndex() const;

	// Acces la taille du groupe poubelle
	// Renvoie 0 si nGarbageGroupIndex=-1
	// Le nombre de modalites du groupe poubelle sinon
	void SetGarbageModalityNumber(int nValue);
	int GetGarbageModalityNumber() const;

	// Acces au nombre de modalites du fourre-tout
	// On decrit ici le nombre de modalites associees a la super-modalite qui represente le fourre-tout (la modalite
	// d'effectif le plus eleve) Renvoie 0 en l'absence de fourre-tout En presence de fourre-tout,
	// nGranularizedValueNumber + nCatchAllValueNumber = nInitialValueNumber
	void SetCatchAllValueNumber(int nValue);
	int GetCatchAllValueNumber() const;
	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un attribut test
	static KWDGSAttributeGrouping* CreateTestAttribute(int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	SymbolVector svValues;
	IntVector ivGroupFirstValueIndexes;
	int nGarbageGroupIndex;
	int nGarbageModalityNumber;
	int nCatchAllValueNumber;
	friend class PLShared_DGSAttributeGrouping;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeContinuousValues
// Attribut d'un DataGrid, ensemble de parties singletons continues
class KWDGSAttributeContinuousValues : public KWDGSAttributePartition
{
public:
	// Constructeur
	KWDGSAttributeContinuousValues();
	~KWDGSAttributeContinuousValues();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Redefinition des methodes virtuelles
	int GetAttributeType() const override;
	boolean ArePartsSingletons() const override;
	void SetPartNumber(int nValue) override;
	int GetPartNumber() const override;
	int ComputeContinuousPartIndex(Continuous cValue) const override;
	boolean Check() const override;
	longint GetUsedMemory() const override;
	void WritePartHeader(ostream& ost) const override;
	void WritePartAt(ostream& ost, int nPartIndex) const override;
	void WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) override;
	KWDGSAttributePartition* Create() const override;
	void CopyFrom(const KWDGSAttributePartition* kwdgsapSource) override;
	int Compare(const KWDGSAttributePartition* kwdgsapSource) const override;
	const ALString GetClassLabel() const override;

	// Mise a jour des valeurs (chaque partie est un singleton)
	// Le nombre de valeur est egal au nombre de parties
	int GetValueNumber() const;
	void SetValueAt(int nIndex, Continuous cValue);
	Continuous GetValueAt(int nIndex) const;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un attribut test
	static KWDGSAttributeContinuousValues* CreateTestAttribute(int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	ContinuousVector cvValues;
	friend class PLShared_DGSAttributeContinuousValues;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeSymbolValues
// Attribut d'un DataGrid, ensemble de parties singletons symboliques
class KWDGSAttributeSymbolValues : public KWDGSAttributePartition
{
public:
	// Constructeur
	KWDGSAttributeSymbolValues();
	~KWDGSAttributeSymbolValues();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Redefinition des methodes virtuelles
	// S'il n'y a pas de StarValue, la methode ComputeSymbolPartIndex renvoie -1 si aucune valeur ne convient
	int GetAttributeType() const override;
	boolean ArePartsSingletons() const override;
	void SetPartNumber(int nValue) override;
	int GetPartNumber() const override;
	int ComputeSymbolPartIndex(const Symbol& sValue) const override;
	boolean Check() const override;
	longint GetUsedMemory() const override;
	void WritePartHeader(ostream& ost) const override;
	void WritePartAt(ostream& ost, int nPartIndex) const override;
	void WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) override;
	KWDGSAttributePartition* Create() const override;
	void CopyFrom(const KWDGSAttributePartition* kwdgsapSource) override;
	int Compare(const KWDGSAttributePartition* kwdgsapSource) const override;
	const ALString GetClassLabel() const override;

	// Mise a jour des valeurs (chaque partie est un singleton)
	// Le nombre de valeurs est egal au nombre de parties
	// Une au plus d'entres-elles peut etre la StarValue (facultatif)
	int GetValueNumber() const;
	void SetValueAt(int nIndex, const Symbol& sValue);
	Symbol& GetValueAt(int nIndex) const;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un attribut test
	static KWDGSAttributeSymbolValues* CreateTestAttribute(int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	SymbolVector svValues;
	friend class PLShared_DGSAttributeSymbolValues;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeVirtualValues
// Attribut d'un DataGrid, dont on ne specifie pas le type
// Permet d'utiliser une grille essentiellement pour la gestion des effectifs
class KWDGSAttributeVirtualValues : public KWDGSAttributePartition
{
public:
	// Constructeur
	KWDGSAttributeVirtualValues();
	~KWDGSAttributeVirtualValues();

	////////////////////////////////
	// Specifications de l'attribut
	// Les methodes virtuelles sont a redefinir dans les sous-classes

	// Redefinition des methodes virtuelles (type: KWType::Unknown; Singletons: true))
	int GetAttributeType() const override;
	boolean ArePartsSingletons() const override;
	void SetPartNumber(int nValue) override;
	int GetPartNumber() const override;
	longint GetUsedMemory() const override;
	void WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) override;
	KWDGSAttributePartition* Create() const override;
	void CopyFrom(const KWDGSAttributePartition* kwdgsapSource) override;

	// Creation d'un attribut test
	static KWDGSAttributeVirtualValues* CreateTestAttribute(int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	int nPartNumber;
};

////////////////////////////////////////////////////////////////////
// Classe KWDGSCell
// Classe de travail, utilisee essentiellement pour la creation de rapports
class KWDGSCell : public Object
{
public:
	// Constructeur
	KWDGSCell();
	~KWDGSCell();

	// Acces au vecteur d'index des parties de la cellule
	IntVector* GetPartIndexes();

	// Nombre d'attributs lies a la cellule
	int GetAttributeNumber() const;

	// Effectif de la cellule
	void SetCellFrequency(int nValue);
	int GetCellFrequency() const;

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	IntVector ivPartIndexes;
	int nCellFrequency;
};

////////////////////////////////////////////////////////////////////
// Classe KWDGSSourceCell
// Specialisation de KWDGSCell, pour gerer les effectifs par
// partie d'un attribut cible ainsi qu'un critere d'interet par cellule
class KWDGSSourceCell : public KWDGSCell
{
public:
	// Constructeur
	KWDGSSourceCell();
	~KWDGSSourceCell();

	// Acces au vecteur des effectif par partie cible
	IntVector* GetTargetPartFrequencies();

	// Interest de la cellule
	void SetInterest(double dValue);
	double GetInterest() const;

	// Integrite: la somme des effectifs par partie cible doit etre la meme que l'effectif de la cellule
	boolean Check();

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	IntVector ivTargetPartFrequencies;
	double dInterest;
};

// Comparaison de deux cellules, sur la base des parties referencees, pour l'affichage (acces par valeur)
int KWDGSCellCompareIndexes(const void* elem1, const void* elem2);

// Comparaison de deux cellules par effectif decroissant (puis sur la base des parties referencees si egalite)
int KWDGSCellCompareDecreasingFrequency(const void* elem1, const void* elem2);

// Comparaison de deux cellules source par interet decroissant (puis par effectif decroissant, puis sur la base des
// parties referencees si egalite)
int KWDGSSourceCellCompareDecreasingInterest(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////
// Classe PLShared_DataGridStats
//	 Serialisation de la classe KWDataGridStats
class PLShared_DataGridStats : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DataGridStats();
	~PLShared_DataGridStats();

	// Acces a la grille
	void SetDataGridStats(KWDataGridStats* dataGrid);
	KWDataGridStats* GetDataGridStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static boolean Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;

	// Test de la serialisation de la grille passee en parametre (celle-ci est detruite a la fin du test)
	static boolean TestDataGrid(KWDataGridStats* testDataGrid);
};

////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributePartition
//	 Serialisation de la classe KWDGSAttributeDiscretization
class PLShared_DGSAttributePartition : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DGSAttributePartition();
	~PLShared_DGSAttributePartition();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeDiscretization
//	 Serialisation de la classe KWDGSAttributeDiscretization
class PLShared_DGSAttributeDiscretization : public PLShared_DGSAttributePartition
{
public:
	// Constructeur
	PLShared_DGSAttributeDiscretization();
	~PLShared_DGSAttributeDiscretization();

	// Acces a la partition de l'attribut
	void SetAttributePartition(KWDGSAttributeDiscretization* kwdgsAttribute);
	KWDGSAttributeDiscretization* GetAttributePartition();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static boolean Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeGrouping
//	 Serialisation de la classe KWDGSAttributeGrouping
class PLShared_DGSAttributeGrouping : public PLShared_DGSAttributePartition
{
public:
	// Constructeur
	PLShared_DGSAttributeGrouping();
	~PLShared_DGSAttributeGrouping();

	// Acces a la partition de l'attribut
	void SetAttributePartition(KWDGSAttributeGrouping* kwdgsAttribute);
	KWDGSAttributeGrouping* GetAttributePartition();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static boolean Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeContinuousValues
//	 Serialisation de la classe KWDGSAttributeContinuousValues
class PLShared_DGSAttributeContinuousValues : public PLShared_DGSAttributePartition
{
public:
	// Constructeur
	PLShared_DGSAttributeContinuousValues();
	~PLShared_DGSAttributeContinuousValues();

	// Acces a la partition de l'attribut
	void SetAttributePartition(KWDGSAttributeContinuousValues* kwdgsAttribute);
	KWDGSAttributeContinuousValues* GetAttributePartition();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static boolean Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeSymbolValues
//	 Serialisation de la classe KWDGSAttributeSymbolValues
class PLShared_DGSAttributeSymbolValues : public PLShared_DGSAttributePartition
{
public:
	// Constructeur
	PLShared_DGSAttributeSymbolValues();
	~PLShared_DGSAttributeSymbolValues();

	// Acces a la partition de l'attribut
	void SetAttributePartition(KWDGSAttributeSymbolValues* kwdgsAttribute);
	KWDGSAttributeSymbolValues* GetAttributePartition();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static boolean Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////
// Methodes en inline

inline void KWDataGridStats::SetCellFrequencyAt(const IntVector* ivPartIndexes, int nFrequency)
{
	int nCellIndex;

	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(ivPartIndexes != NULL);
	require(nFrequency >= 0);

	nCellIndex = ComputeCellIndex(ivPartIndexes);
	assert(ivCellFrequencies.GetAt(nCellIndex) == 0);
	ivCellFrequencies.SetAt(nCellIndex, nFrequency);
}

inline int KWDataGridStats::GetCellFrequencyAt(const IntVector* ivPartIndexes) const
{
	int nCellIndex;

	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(ivPartIndexes != NULL);

	nCellIndex = ComputeCellIndex(ivPartIndexes);
	return ivCellFrequencies.GetAt(nCellIndex);
}

inline void KWDataGridStats::SetUnivariateCellFrequencyAt(int nPart, int nFrequency)
{
	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(GetAttributeNumber() == 1);
	require(0 <= nPart and nPart < GetAttributeAt(0)->GetPartNumber());
	require(nFrequency >= 0);

	assert(ivCellFrequencies.GetAt(nPart) == 0);
	ivCellFrequencies.SetAt(nPart, nFrequency);
}

inline int KWDataGridStats::GetUnivariateCellFrequencyAt(int nPart) const
{
	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(GetAttributeNumber() == 1);
	require(0 <= nPart and nPart < GetAttributeAt(0)->GetPartNumber());

	return ivCellFrequencies.GetAt(nPart);
}

inline void KWDataGridStats::SetBivariateCellFrequencyAt(int nPart1, int nPart2, int nFrequency)
{
	int nCellIndex;
	debug(IntVector ivPartIndexes);

	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(GetAttributeNumber() == 2);
	require(0 <= nPart1 and nPart1 < GetAttributeAt(0)->GetPartNumber());
	require(0 <= nPart2 and nPart2 < GetAttributeAt(1)->GetPartNumber());
	require(nFrequency >= 0);

	// Index de la cellule, avec verification
	nCellIndex = nPart1 + nPart2 * GetAttributeAt(0)->GetPartNumber();
	debug(ivPartIndexes.SetSize(2));
	debug(ivPartIndexes.SetAt(0, nPart1));
	debug(ivPartIndexes.SetAt(1, nPart2));
	debug(assert(nCellIndex == ComputeCellIndex(&ivPartIndexes)));
	assert(ivCellFrequencies.GetAt(nCellIndex) == 0);
	ivCellFrequencies.SetAt(nCellIndex, nFrequency);
}

inline int KWDataGridStats::GetBivariateCellFrequencyAt(int nPart1, int nPart2) const
{
	int nCellIndex;
	debug(IntVector ivPartIndexes);

	require(ivCellFrequencies.GetSize() == ComputeTotalGridSize());
	require(GetAttributeNumber() == 2);
	require(0 <= nPart1 and nPart1 < GetAttributeAt(0)->GetPartNumber());
	require(0 <= nPart2 and nPart2 < GetAttributeAt(1)->GetPartNumber());

	// Index de la cellule, avec verification
	nCellIndex = nPart1 + nPart2 * GetAttributeAt(0)->GetPartNumber();
	debug(ivPartIndexes.SetSize(2));
	debug(ivPartIndexes.SetAt(0, nPart1));
	debug(ivPartIndexes.SetAt(1, nPart2));
	debug(assert(nCellIndex == ComputeCellIndex(&ivPartIndexes)));
	return ivCellFrequencies.GetAt(nCellIndex);
}

inline boolean KWDataGridStats::CheckPartIndexes(const IntVector* ivPartIndexes) const
{
	require(ivPartIndexes != NULL);
	require(ivPartIndexes->GetSize() == GetAttributeNumber());
	return InternalCheckPartIndexes(ivPartIndexes, 0, GetAttributeNumber() - 1);
}

inline int KWDataGridStats::ComputeCellIndex(const IntVector* ivPartIndexes) const
{
	require(CheckPartIndexes(ivPartIndexes));
	return InternalComputeCellIndex(ivPartIndexes, 0, GetAttributeNumber() - 1);
}

inline void KWDataGridStats::ComputePartIndexes(int nCellIndex, IntVector* ivPartIndexes) const
{
	require(0 <= nCellIndex and nCellIndex < ComputeTotalGridSize());
	require(ivPartIndexes != NULL);
	require(ivPartIndexes->GetSize() == GetAttributeNumber());
	InternalComputePartIndexes(nCellIndex, ivPartIndexes, 0, GetAttributeNumber() - 1);
}

inline boolean KWDataGridStats::CheckSourcePartIndexes(const IntVector* ivPartIndexes) const
{
	require(ivPartIndexes != NULL);
	require(GetSourceAttributeNumber() <= ivPartIndexes->GetSize() and
		ivPartIndexes->GetSize() <= GetAttributeNumber());
	return InternalCheckPartIndexes(ivPartIndexes, 0, GetSourceAttributeNumber() - 1);
}

inline int KWDataGridStats::ComputeSourceCellIndex(const IntVector* ivPartIndexes) const
{
	require(CheckSourcePartIndexes(ivPartIndexes));
	return InternalComputeCellIndex(ivPartIndexes, 0, GetSourceAttributeNumber() - 1);
}

inline void KWDataGridStats::ComputeSourcePartIndexes(int nCellIndex, IntVector* ivPartIndexes) const
{
	require(0 <= nCellIndex and nCellIndex < ComputeTotalGridSize());
	require(ivPartIndexes != NULL);
	require(GetSourceAttributeNumber() <= ivPartIndexes->GetSize() and
		ivPartIndexes->GetSize() <= GetAttributeNumber());
	InternalComputePartIndexes(nCellIndex, ivPartIndexes, 0, GetSourceAttributeNumber() - 1);
}

inline boolean KWDataGridStats::CheckTargetPartIndexes(const IntVector* ivPartIndexes) const
{
	require(ivPartIndexes != NULL);
	require(ivPartIndexes->GetSize() == GetAttributeNumber());
	return InternalCheckPartIndexes(ivPartIndexes, GetFirstTargetAttributeIndex(), GetAttributeNumber() - 1);
}

inline int KWDataGridStats::ComputeTargetCellIndex(const IntVector* ivPartIndexes) const
{
	require(CheckTargetPartIndexes(ivPartIndexes));
	return InternalComputeCellIndex(ivPartIndexes, GetFirstTargetAttributeIndex(), GetAttributeNumber() - 1);
}

inline void KWDataGridStats::ComputeTargetPartIndexes(int nCellIndex, IntVector* ivPartIndexes) const
{
	require(0 <= nCellIndex and nCellIndex < ComputeTotalGridSize());
	require(ivPartIndexes != NULL);
	require(ivPartIndexes->GetSize() == GetAttributeNumber());
	InternalComputePartIndexes(nCellIndex, ivPartIndexes, GetFirstTargetAttributeIndex(), GetAttributeNumber() - 1);
}

inline int KWDataGridStats::InternalComputeCellIndex(const IntVector* ivPartIndexes, int nFirstAttributeIndex,
						     int nLastAttributeIndex) const
{
	int nCellIndex;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(InternalCheckPartIndexes(ivPartIndexes, nFirstAttributeIndex, nLastAttributeIndex));

	// Calcul de l'index de la cellule dans la grille
	nCellIndex = 0;
	for (nAttribute = nLastAttributeIndex; nAttribute >= nFirstAttributeIndex; nAttribute--)
	{
		attribute = GetAttributeAt(nAttribute);
		if (nAttribute < nLastAttributeIndex)
			nCellIndex *= attribute->GetPartNumber();
		nCellIndex += ivPartIndexes->GetAt(nAttribute);
	}
	ensure(0 <= nCellIndex and nCellIndex < ComputeTotalGridSize());
	ensure(ivPartIndexes->GetSize() > GetSourceAttributeNumber() or nCellIndex < ComputeSourceGridSize());
	return nCellIndex;
}

inline void KWDataGridStats::InternalComputePartIndexes(int nCellIndex, IntVector* ivPartIndexes,
							int nFirstAttributeIndex, int nLastAttributeIndex) const
{
	int nIndex;
	int nPartIndex;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(0 <= nCellIndex and nCellIndex < ComputeTotalGridSize());
	require(0 <= nFirstAttributeIndex and nFirstAttributeIndex <= nLastAttributeIndex);
	require(0 <= nLastAttributeIndex and nLastAttributeIndex < GetAttributeNumber());
	require(nLastAttributeIndex < ivPartIndexes->GetSize());

	// Calcul de l'index de la cellule dans la grille
	nIndex = nCellIndex;
	for (nAttribute = nFirstAttributeIndex; nAttribute <= nLastAttributeIndex; nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		nPartIndex = nIndex % attribute->GetPartNumber();
		ivPartIndexes->SetAt(nAttribute, nPartIndex);
		if (nAttribute < nLastAttributeIndex)
			nIndex /= attribute->GetPartNumber();
	}
	ensure(nCellIndex == InternalComputeCellIndex(ivPartIndexes, nFirstAttributeIndex, nLastAttributeIndex));
}

inline KWDGSCell::KWDGSCell()
{
	nCellFrequency = 0;
}

inline KWDGSCell::~KWDGSCell() {}

inline IntVector* KWDGSCell::GetPartIndexes()
{
	return &ivPartIndexes;
}

inline int KWDGSCell::GetAttributeNumber() const
{
	return ivPartIndexes.GetSize();
}

inline void KWDGSCell::SetCellFrequency(int nValue)
{
	require(nValue >= 0);
	nCellFrequency = nValue;
}

inline int KWDGSCell::GetCellFrequency() const
{
	return nCellFrequency;
}

inline KWDGSSourceCell::KWDGSSourceCell()
{
	dInterest = 0;
}

inline KWDGSSourceCell::~KWDGSSourceCell() {}

inline IntVector* KWDGSSourceCell::GetTargetPartFrequencies()
{
	return &ivTargetPartFrequencies;
}

inline void KWDGSSourceCell::SetInterest(double dValue)
{
	dInterest = dValue;
}

inline double KWDGSSourceCell::GetInterest() const
{
	return dInterest;
}

inline boolean KWDGSSourceCell::Check()
{
	int i;
	int nTotal = 0;
	for (i = 0; i < ivTargetPartFrequencies.GetSize(); i++)
	{
		assert(ivTargetPartFrequencies.GetAt(i) >= 0);
		nTotal += ivTargetPartFrequencies.GetAt(i);
	}
	return nTotal == nCellFrequency;
}