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

	// Specification des paires d'attributs a evaluer
	// Memoire: appartient a l'appelant
	void SetAttributePairsSpec(KWAttributePairsSpec* spec);
	KWAttributePairsSpec* GetAttributePairsSpec();

	//////////////////////////////////////////////////////////////////////////////
	// Calcul des statistiques
	//   . calcul des donnees a partir de la classe d'analyse et de la base en entree
	//   . calcul des statistiques univariees
	//   . calcul des statistiques bivariee si necessaire
	//   . construction de nouveaux attributs et preperation, selon le parametrage
	//     de KDDataPreparationAttributeCreationTask

	// Calcul de statistiques univariee et bivariee
	// Si le suivi des taches est active, cette methode alimente le
	// suivi des taches (MainLabel, Progression et Label) et gere l'interruption
	// des taches (auquel cas, les statistiques ne sont pas calculees)

	// Tache interruptible (tester IsStatsComputed pour exploiter les resultats)
	boolean ComputeStats() override;

	//////////////////////////////////////////////////////////////////////////////
	// Acces aux resultats de calcul des statistiques

	// Acces a toutes les statistiques de preparation disponibles (univariee, bivariees, crees...)
	// Accessible uniquement si statistiques calculees
	// Le tableau contient des objets KWDataPreparationStats
	// Memoire: le tableau retourne et son contenu appartiennent a l'appele
	ObjectArray* GetAllPreparedStats();

	// Acces aux statistiques univariees par attributs
	// (dans le meme ordre que dans la classe, mais avec l'attribut cible en moins)
	// Accessible uniquement si statistiques calculees
	// Le tableau contient des objets KWAttributeStats
	// Memoire: le tableau retourne et son contenu appartiennent a l'appele
	ObjectArray* GetAttributeStats();

	// Acces par nom aux attributs (acces rapide par dictionnaire)
	// Renvoie NULL si non trouve
	KWAttributeStats* LookupAttributeStats(const ALString& sAttributeName);

	// Acces aux statistiques bivariees par paire d'attributs
	// Dans chaque paire, les attributs sont ordonnes par ordre alphabetique
	// Le tableau contient des objets KWAttributePairStats
	ObjectArray* GetAttributePairStats();

	// Acces aux statistiques univaries par attributs crees par la tache
	// de creation KDDataPreparationAttributeCreationTask::GetGlobalCreationTask(),
	// typiquement utilise pour la dreation de variable de type arbre
	// Ces attributs crees n'apparaissent pas de la rapport de preparation
	// Le tableau contient des objets KWAttributeStats
	ObjectArray* GetCreatedAttributeStats();

	// Nombre d'attributs evalues, natifs (y compris dans les blocs natifs) et construits
	int GetEvaluatedAttributeNumber() const;
	int GetNativeAttributeNumber() const;
	int GetConstructedAttributeNumber() const;

	// Nombre d'attributs informatifs (dont le niveau d'evaluation est non nul)
	int GetInformativeAttributeNumber() const;

	// Nombre d'attributs cree informatifs (dont le niveau d'evaluation est non nul)
	int GetInformativeCreatedAttributeNumber() const;

	// Nombre de paires d'attributs informatifs (dont le niveau d'evaluation est non nul en non supervise,
	// ou dont le DeltaLevel est pstrictement positif en supervise)
	int GetInformativeAttributePairNumber() const;

	// Nombre d'attributs utilises par type
	int GetUsedAttributeNumberForType(int nType) const;

	// Temps de calcul en secondes
	double GetTotalComputeTime() const;

	//////////////////////////////////////////////////////////////////////////////
	// Methodes avancee pour acceder a la tache de creation des attributs

	// Acces au service de creation des attributs, ue fois es attribut crees (potentiellement NULL)
	const KDDataPreparationAttributeCreationTask* GetAttributeCreationTask() const;

	// Supression/destruction de la base preparee
	void RemoveAttributeCreationTask();
	void DeleteAttributeCreationTask();

	//////////////////////////////////////////////////////////////////////////////
	// Methodes avancee pour gerer directement les resultats de calculs de statistiques

	// Ajout d'un resultat de statistique univariee
	void AddAttributeStats(KWAttributeStats* attributeStats);

	// Ajout d'un resultat de statistique bivariee
	void AddAttributePairStats(KWAttributePairStats* attributePairStats);

	// Ajout d'un resultat de statistique univariee d'un attribut cree
	void AddCreatedAttributeStats(KWAttributeStats* createdAttributeStats);

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
	// Ecriture de rapport

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	virtual void WriteReport(ostream& ost) override;

	// Parametrage de l'ecriture des rapports (tout est a true par defaut)
	void SetWriteOptionStats1D(boolean bValue);
	boolean GetWriteOptionStats1D() const;
	void SetWriteOptionStats2D(boolean bValue);
	boolean GetWriteOptionStats2D() const;

	// Indique si des variables ont demander a etre creees
	boolean IsCreationRequired() const;

	// Parametrage d'un rapport dedie a la specification de la construction d'attribut
	// Ce rapport sera inserer dans le rapport de preparation s'il est specifie, et sera ignore sinon
	// Memoire: appartient a l'appele
	void SetAttributeConstructionReport(KWLearningReport* report);
	KWLearningReport* GetAttributeConstructionReport() const;

	// Parametrage de l'ecriture des rapports des attributs crees
	void SetWriteOptionStatsCreated(boolean bValue);
	boolean GetWriteOptionStatsCreated() const;

	// Parametrage de l'ecriture de la partie detaillee des rapports
	void SetWriteOptionDetailedStats(boolean bValue);
	boolean GetWriteOptionDetailedStats() const;

	// Parametrage de toutes les options en une seule fois
	virtual void SetAllWriteOptions(boolean bValue);

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

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
	void UseUnivariateBasicSelectionCosts();

	// Restitution des cout des variables du dictionnaires issues de la preparation univariee
	void RestoreUnivariateSelectionCosts();

	// Modification generique des cout de selection des variables
	void InternalUpdateUnivariateSelectionCosts(double dPreviousSelectionCost, double dNewSelectionCost);

	// Nettoyage des donnees de travail et des resultats d'analyse
	virtual void CleanWorkingData();

	// Creation generique des objets de preparation de stats,
	// permettant de les specialiser dans une sous-classe
	virtual KWAttributeStats* CreateAttributeStats() const;
	virtual KWAttributePairStats* CreateAttributePairStats() const;

	// Nombre de valeurs cibles dans le cas categoriel, considere comme important
	virtual int GetTargetValueLargeNumber(int nDatabaseSize);

	// Specification de l'analyse bivariee
	KWAttributePairsSpec* attributePairSpec;

	// Gestion de toutes les stats de preparation
	ObjectArray oaAllPreparedStats;

	// Gestion des stats univariees
	ObjectArray oaAttributeStats;
	ObjectDictionary odAttributeStats;

	// Gestion des stats bivariees
	ObjectArray oaAttributePairStats;

	// Gestion des stats des attributs crees
	ObjectArray oaCreatedAttributeStats;

	// Tache de creation des attributs
	KDDataPreparationAttributeCreationTask* attributeCreationTask;

	// Base de donnees decoupee en tranches
	KWDataTableSliceSet* dataTableSliceSet;

	// Vecteur de valeurs d'une cible categorielle
	SymbolVector* svSymbolTargetValues;

	// Vecteur de valeurs d'une cible continue
	ContinuousVector* cvContinuousTargetValues;

	// Timer pour la mesure des performances
	Timer timerTotal;

	// Option d'ecriture des rapports
	KWLearningReport* attributeConstructionReport;
	boolean bWriteOptionStats1D;
	boolean bWriteOptionStatsCreated;
	boolean bWriteOptionStats2D;
	boolean bWriteOptionDetailedStats;
};
