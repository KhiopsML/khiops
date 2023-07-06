// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClassStats;

class KWDatabaseBasicStatsTask;
class KWDatabaseSlicerTask;
class KWDataPreparationUnivariateTask;
class KWDataPreparationBivariateTask;
class KDDataPreparationAttributeCreationTask;
class KDDataPreparationAttributeCreationTask;

#include "TaskProgression.h"
#include "KWAttributePairsSpec.h"
#include "KWLearningReport.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWDescriptiveStats.h"
#include "KWAttributeStats.h"
#include "KWAttributeSubsetStats.h"
#include "KWDataTableSliceSet.h"
#include "Timer.h"

///////////////////////////////////////////////////////
// Statistiques sur une classe et ses attributs pour
// un probleme d'apprentissage
class KWClassStats : public KWLearningReport
{
public:
	// Constructeur
	KWClassStats();
	~KWClassStats();

	//////////////////////////////////////////////////////////////////////////////
	// Specifications de construction de variable
	//
	// La construction de variable se base sur les familles suivantes
	//  . MultiTable: en exploitant le schema multi-table, hors variables de type texte
	//  . Text: en exploitant les variable de type texte (Text ou TextList) presente dans le schema multi-table
	//  . Tree: en combinant des variables au moyen des arbres
	//  . 2D: en analysant des paires de variables
	// De facon a maitriser la combinatoire des interaction possible et a facilite l'interpretabilite
	// des modeles, les familles sont explote selon les relation suivantes:
	//  . MultiTable et Text sont deux familles independantes (sans variable Text, ou uniquement les variables Text)
	//    exploitant le schema multi-table
	//  . Tree n'exploite que des variables de type MultiTable
	//  . 2D n'explote que des variables de type MultiTable
	// Un meta-prior de construction de variable est defini dans la classe KWLearningSpec
	// (cf. Parametrage des familles de construction de variables)
	// La classe d'analyse du LearningSpec doit deja etre construites avec toutes les variables
	// des famille MultiTable et Text. Il faut egalement parametrer les dictionnaires des variables
	// concernees au moyen des methodes ci-dessous.
	// L'analyse bivariee se fait sur la base des variables MultiTable et est parametree
	// ci-dessous par un objet de type KWAttributePairsSpec
	// La construction des arbres se fait egalement sur la base des variables MultiTables et est parametree
	// par la tache de creation des arbre (cf. methode GetAttributeTreeConstructionTask)

	// Specification des attributs de la classe d'analyse, construits pour le multi-tables
	// Memoire: appartient a l'appelant
	ObjectDictionary* GetMultiTableConstructionSpec();

	// Specification des attributs de la classe d'analyse, construits pour les variables de type texte
	// Memoire: appartient a l'appelant
	ObjectDictionary* GetTextConstructionSpec();

	// Specification des paires d'attributs a evaluer (obligatoire)
	// Memoire: appartient a l'appelant
	void SetAttributePairsSpec(const KWAttributePairsSpec* spec);
	const KWAttributePairsSpec* GetAttributePairsSpec() const;

	//////////////////////////////////////////////////////////////////////////////
	// Calcul des statistiques
	//   . calcul des donnees a partir de la classe d'analyse et de la base en entree
	//   . calcul des statistiques univariees
	//   . calcul des statistiques bivariee si necessaire
	//   . construction de nouveaux attributs et preparation, selon le parametrage
	//     de KDDataPreparationAttributeCreationTask

	// Calcul de statistiques univariee et bivariee
	// Si le suivi des taches est active, cette methode alimente le
	// suivi des taches (MainLabel, Progression et Label) et gere l'interruption
	// des taches (auquel cas, les statistiques ne sont pas calculees)

	// Tache interruptible (tester IsStatsComputed pour exploiter les resultats)
	boolean ComputeStats() override;

	//////////////////////////////////////////////////////////////////////////////
	// Acces aux resultats de calcul des statistiques

	// Acces a toutes les statistiques de preparation disponibles (univariee initiaux, multi-table ou texte, arbres,
	// bivariees) Accessible uniquement si statistiques calculees Le tableau contient des objets
	// KWDataPreparationStats Memoire: le tableau retourne et son contenu appartiennent a l'appele
	ObjectArray* GetAllPreparedStats();

	// Acces aux statistiques univariees par attributs
	// Concerne les attributs de type initiaux ou multi-table uniquement
	// (dans le meme ordre que dans la classe, mais sans l'attribut cible ni les attributs de type texte)
	// Accessible uniquement si statistiques calculees
	// Le tableau contient des objets KWAttributeStats
	// Memoire: le tableau retourne et son contenu appartiennent a l'appele
	ObjectArray* GetAttributeStats();

	// Acces par nom aux attributs de type initiaux ou multi-tables uniquement (acces rapide par dictionnaire)
	// Renvoie NULL si non trouve
	KWAttributeStats* LookupAttributeStats(const ALString& sAttributeName) const;

	// Acces aux statistiques bivariees par paire d'attributs
	// Dans chaque paire, les attributs sont ordonnes par ordre alphabetique
	// Le tableau contient des objets KWAttributePairStats
	ObjectArray* GetAttributePairStats();

	// Acces aux statistiques univaries par attribut de type texte
	// Ces attributs crees n'apparaissent pas dans le rapport de preparation
	// Le tableau contient des objets KWAttributeStats
	ObjectArray* GetTextAttributeStats();

	// Acces aux statistiques univaries par attribut de type arbre
	// Ces attributs crees n'apparaissent pas dans le rapport de preparation
	// Le tableau contient des objets KWAttributeStats
	ObjectArray* GetTreeAttributeStats();

	// Nombre d'attributs evalues, natifs (y compris dans les blocs natifs) et construits
	int GetEvaluatedAttributeNumber() const;
	int GetNativeAttributeNumber() const;
	int GetConstructedAttributeNumber() const;

	// Nombre total d'attributs informatifs (dont le niveau d'evaluation est non nul)
	// Cumul de tous les types d'attributs ou paires d'attributs informatifs
	int GetTotalInformativeAttributeNumber() const;

	// Nombre d'attributs informatifs de base (natif et multi-tables) (dont le niveau d'evaluation est non nul)
	int GetInformativeAttributeNumber() const;

	// Nombre d'attributs de type texte informatifs (dont le niveau d'evaluation est non nul)
	int GetInformativeTextAttributeNumber() const;

	// Nombre d'attributs de type arbre informatifs (dont le niveau d'evaluation est non nul)
	int GetInformativeTreeAttributeNumber() const;

	// Nombre de paires d'attributs informatifs (dont le niveau d'evaluation est non nul en non supervise,
	// ou dont le DeltaLevel est strictement positif en supervise)
	int GetInformativeAttributePairNumber() const;

	// Nombre d'attributs utilises par type
	int GetUsedAttributeNumberForType(int nType) const;

	// Temps de calcul en secondes
	double GetTotalComputeTime() const;

	//////////////////////////////////////////////////////////////////////////////
	// Methodes avancee pour acceder a la tache de creation des attributs de type arbre
	//
	// La tache de creation des arbre est developpe dabs la librairie DTForest.
	// Pour des raison de modularite, cette tache est parametree de facon generique par
	// la methode KDDataPreparationAttributeCreationTask::GetGlobalCreationTask(),
	// utilise lors de la methode ComputeStats.

	// Acces a la tache de construction des attributs de type arbre, une fois ces attributs crees dans ComputeStas
	// (potentiellement NULL)
	const KDDataPreparationAttributeCreationTask* GetAttributeTreeConstructionTask() const;

	// Supression/destruction de la tache de construction des arbres
	void RemoveAttributeTreeConstructionTask();
	void DeleteAttributeTreeConstructionTask();

	//////////////////////////////////////////////////////////////////////////////
	// Methodes avancee pour paramter directement les resultats de calculs de statistiques
	// Utile notamment pour la tache de preparation bivariee

	// Ajout d'un resultat de statistique univariee
	void AddAttributeStats(KWAttributeStats* attributeStats);

	//////////////////////////////////////////////////////////////////////////////
	// Methodes avancee pour gerer la base preparee, decoupee en tranche
	// Cette base est contituee d'un ensmeble de fichiers issue d'une mise a
	// plat de la base, pour permettre son traitement efficace en parallele
	// On donne egalement acces aux vecteur de valeurs cibles, a utiliser
	// conjointement avec la base preparee

	// Acces au sliceset
	// Accessible uniquement si statistiques calculees
	KWDataTableSliceSet* GetDataTableSliceSet() const;

	// Parametrage du sliceset
	// Attention: methode avancee
	void SetDataTableSliceSet(KWDataTableSliceSet* sliceSet);

	// Supression/destruction de la base preparee
	void RemoveDataTableSliceSet();
	void DeleteDataTableSliceSet();

	// Acces au vecteur des valeurs cibles categorielles. Renvoie NULL si la cible est continue
	SymbolVector* GetSymbolTargetValues() const;

	// Acces au vecteur des valeurs cibles continues. Renvoie NULL si la cible est categorielle
	ContinuousVector* GetContinuousTargetValues() const;

	// Paramettrage du vecteur de variables cible
	// Attention: methode avancee
	void SetSymbolTargetValues(SymbolVector* svValues);
	void SetContinuousTargetValues(ContinuousVector* cvValues);

	// Supression/destruction de la variable cible
	void RemoveTargetValues();
	void DeleteTargetValues();

	// Supression/destruction de tous les resultats d'analyse et de la base preparee
	void RemoveAll();
	void DeleteAll();

	///////////////////////////////////////////////////////////
	// Parametrage des attributs prepares selectionnes par l'ensemble des predicteurs
	// Cela permet de specialiser les rapports de preparation, en specifiant les
	// nombre d'attributs selectionnes et en ne gardant dans attributs construits selectionnes
	// afin de gagner de la place

	// Option d'ecriture des rapports de preparation pour les nombres de variables selectionnees (defautl: false)
	void SetWriteSelectedAttributeNumbers(boolean bValue);
	boolean GetWriteSelectedAttributeNumbers() const;

	// Indicateur pour ne garder dans les rapports que les variables construites selectionnees
	void SetKeepSelectedAttributesOnly(boolean bValue);
	boolean GetKeepSelectedAttributesOnly() const;

	// Dictionnaire pour specifier les KWDataPreparationStats selectionnes directement par un predicteur
	// Permet de calculer les nombres d'attributs selectionnes directement et de les avoir dans les rapports
	// Memoire: appartient a l'appele, ainsi que son contenu
	NumericKeyDictionary* GetSelectedDataPreparationStats();

	// Dictionnaire pour specifier les KWDataPreparationStats selectionnes recursivement par un predicteur,
	// par exemple via des preparation bivariee ou via des grilles
	// Permet de garder dans les rapports tout attribut selectionne directement ou indirectement
	// Memoire: appartient a l'appele, ainsi que son contenu
	NumericKeyDictionary* GetRecursivelySelectedDataPreparationStats();

	// Verification de la specification des variables selectionnees directement ou non
	boolean CheckSelectedDataPreparationStatsSpec() const;

	// Nombre d'attributs selectionnes de base (natif et multi-tables)
	int GetSelectedAttributeNumber() const;

	// Nombre d'attributs selectionnes de type texte
	int GetSelectedTextAttributeNumber() const;

	// Nombre d'attributs selectionnes de type arbre
	int GetSelectedTreeAttributeNumber() const;

	// Nombre de paires d'attributs selectionnees
	int GetSelectedAttributePairNumber() const;

	///////////////////////////////////////////////////////////
	// Ecriture de rapport
	// Une seule des options doit etre actrive parmi 1D, 2D, Text ou Tree

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	void WriteReport(ostream& ost) override;

	// Parametrage de l'ecriture des rapports des attribut natif ou construits (defaut: false)
	void SetWriteOptionStatsNativeOrConstructed(boolean bValue);
	boolean GetWriteOptionStatsNativeOrConstructed() const;

	// Parametrage d'un rapport dedie a la specification de la construction d'attribut de type arbres
	// Ce rapport sera insere dans le rapport de preparation s'il est specifie, et sera ignore sinon
	// Memoire: appartient a l'appele
	void SetAttributeTreeConstructionReport(KWLearningReport* report);
	KWLearningReport* GetAttributeTreeConstructionReport() const;

	// Indique si des variables de type arbre doivent etre construites
	boolean IsTreeConstructionRequired() const;

	// Parametrage de l'ecriture des rapports des attributs de type texte (defaut: false)
	void SetWriteOptionStatsText(boolean bValue);
	boolean GetWriteOptionStatsText() const;

	// Parametrage de l'ecriture des rapports des attributs de type arbre (defaut: false)
	void SetWriteOptionStatsTrees(boolean bValue);
	boolean GetWriteOptionStatsTrees() const;

	// Parametrage de l'ecriture des rapports univarie: native ou constuit, text ou abre
	boolean GetWriteOptionStats1D() const;

	// Parametrage de l'ecriture des rapports des paires d'attributs (defaut: false)
	void SetWriteOptionStats2D(boolean bValue);
	boolean GetWriteOptionStats2D() const;

	// Parametrage de l'ecriture de la partie detaillee des rapports (defaut: true)
	void SetWriteDetailedStats(boolean bValue);
	boolean GetWriteDetailedStats() const;

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

	// Verification de la validite des specifications
	boolean Check() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Changement des cout des variables du dictionnaires issues la preparation univariee
	// pour se preparer aux constructions multivaries, paires de variable et arbre
	// En effet, le cout de selection global (KWLearningSpec::GetSelectionCost) integre
	// toutes les familles de construction de variables utilisees, alors que les familles
	// multivaries selectionnent elle même uniquement des variable initiales ou construites
	// avec un cout de selection basique (KWLearningSpec::GetBasicSelectionCost)
	// Cette mise a jour se fait a la fois dans la classe principale et dans toutes celles du sliceset
	void UseUnivariateBasicSelectionCosts(boolean bUsingTextAttributes);

	// Restitution des cout des variables du dictionnaires issues de la preparation univariee
	void RestoreUnivariateSelectionCosts(boolean bUsingTextAttributes);

	// Modification generique des cout de selection des variables
	void InternalUpdateUnivariateSelectionCosts(double dPreviousSelectionCost, double dNewSelectionCost);

	// Nettoyage des donnees de travail et des resultats d'analyse
	virtual void CleanWorkingData();

	// Creation generique des objets de preparation de stats,
	// permettant de les specialiser dans une sous-classe
	virtual KWAttributeStats* TreeAttributeStats() const;
	virtual KWAttributePairStats* CreateAttributePairStats() const;

	// Nombre de valeurs cibles dans le cas categoriel, considere comme important
	virtual int GetTargetValueLargeNumber(int nDatabaseSize);

	// Verification de la specification d'un ensmeble de variable de construction
	boolean CheckConstructionAttributes(const ObjectDictionary* odConstructedAttributes) const;

	////////////////////////////////////////////////////////////
	// Service pour les rapports

	// Calcul du nombre d'attributs prepares informatifs
	int ComputeInformativeDataPreparationStats(const ObjectArray* oaInputDataPreparationStats) const;

	// Calcul du nombre d'attributs prepares references depuis un dictionnaires
	int ComputeSelectedDataPreparationStats(const ObjectArray* oaInputDataPreparationStats) const;

	// Filtrage d'un tableau d'attribut prepares pour ne gardant que ceux qui natif ou selectionnes directement ou
	// indirectement
	void FilterSelectedDataPreparationStats(const ObjectArray* oaInputDataPreparationStats,
						ObjectArray* oaFilteredDataPreparationStats) const;

	// Filtrage d'un tableau d'attribut prepares pour ne garde que ceux qui sont references par un dictionnaire
	// Les tableaux en sortie appartiennent a l'appelant, et leur contenu sont une sous partie du tableau en entree
	void DispatchAttributeStatsByType(const ObjectArray* oaInputAttributeStats, ObjectArray* oaSymbolAttributeStats,
					  ObjectArray* oaContinuousAttributeStats) const;

	// Specification du dictionnaire des variable construites en multi-tables
	ObjectDictionary odMultiTableConstructedAttributes;

	// Specification du dictionnaire des variable construites pour le texte
	ObjectDictionary odTextConstructedAttributes;

	// Specification de l'analyse bivariee
	const KWAttributePairsSpec* attributePairSpec;

	// Gestion de toutes les stats de preparation
	ObjectArray oaAllPreparedStats;

	// Gestion des stats univariees
	ObjectArray oaAttributeStats;
	ObjectDictionary odAttributeStats;

	// Gestion des stats bivariees
	ObjectArray oaAttributePairStats;

	// Gestion des stats des attributs de type texte
	ObjectArray oaTextAttributeStats;

	// Gestion des stats des attributs de type arbre
	ObjectArray oaTreeAttributeStats;

	// Tache de construction des attributs de type arbre
	KDDataPreparationAttributeCreationTask* attributeTreeConstructionTask;

	// Base de donnees decoupee en tranches
	KWDataTableSliceSet* dataTableSliceSet;

	// Vecteur de valeurs d'une cible categorielle
	SymbolVector* svSymbolTargetValues;

	// Vecteur de valeurs d'une cible continue
	ContinuousVector* cvContinuousTargetValues;

	// Timer pour la mesure des performances
	Timer timerTotal;

	// Option d'ecriture des rapports de preparation pour les nombres de variables selectionnees (defautl: false)
	boolean bWriteSelectedAttributeNumbers;

	// Indicateur pour ne garder dans les rapports que les variables construites selectionnees
	boolean bKeepSelectedAttributesOnly;

	// Dictionnaire des KWDataPreparationStats du ClassStats selectionnes
	NumericKeyDictionary nkdSelectedDataPreparationStats;

	// Dictionnaire des KWDataPreparationStats du selectionnes recursivement
	NumericKeyDictionary nkdRecursivelySelectedDataPreparationStats;

	// Option d'ecriture des rapports
	KWLearningReport* attributeTreeConstructionReport;
	boolean bWriteOptionStatsNativeOrConstructed;
	boolean bWriteOptionStatsText;
	boolean bWriteOptionStatsTrees;
	boolean bWriteOptionStats2D;
	boolean bWriteDetailedStats;
};