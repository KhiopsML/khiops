// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWAttributeStats;
class PLShared_AttributeStats;

#include "KWLearningReport.h"
#include "KWDescriptiveStats.h"
#include "KWDiscretizerMODL.h"
#include "KWGrouperMODL.h"
#include "KWDRPreprocessing.h"
#include "KWDataGrid.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////
// Classe KWAttributeStats
// Calcul de statistiques (descriptives + partitionnement) sur un attribut source
// unique de type Symbol ou Continuous pour un probleme d'apprentissage supervise
// ou non supervise
class KWAttributeStats : public KWDataPreparationStats
{
public:
	// Constructeur et destructeur
	KWAttributeStats();
	~KWAttributeStats();

	// Acces au nom de l'attribut considere
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	// Redefinition des methodes virtuelles
	int GetAttributeNumber() const override;
	const ALString& GetAttributeNameAt(int nIndex) const override;

	// Type de l'attribut (Symbol ou Continuous)
	// Le type n'est modifiable qu'une seule fois, pour l'initialisation
	void SetAttributeType(int nType);
	const int GetAttributeType() const;

	// Calcul de statistiques
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Acces aux statistiques descriptives (type de stats dependant du type de l'attribut)
	KWDescriptiveStats* GetDescriptiveStats();

	// Acces aux stats par valeur dans le cas Symbol
	// Les valeurs sont triees par effectif decroissant, avec au plus une valeur d'effectif 1,
	// les autres etant fusionnees sous la StarValue
	KWDataGridStats* GetSymbolValueStats();

	/////////////////////////////////////////////////////
	// Rapport sur les resultats
	// Accessible uniquement si statistiques calculees

	// Ecriture d'un rapport sur l'attribut
	void WriteReport(ostream& ost) const override;
	boolean IsReported() const override;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;

	// Redefinition des criteres de tri des rapports
	const ALString GetSortName() const override;
	double GetSortValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage des resultats de preparation de donnees
	void CleanDataPreparationResults() override;

	///////////////////////////////////////////////////////////
	// Methode de pretraitement d'un attribut source
	// Construction d'une grille de preparation en sortie,
	// ainsi que de la valeur d'evaluation (via SortValue)

	///////////////////////////////////////////////////////////
	// Methodes pour le groupage et la discretization
	// dans le cas non supervise

	// Discretisation non supervisee d'un attribut continu
	// Prerequis: les donnees doivent etre triees suivant l'attribut a discretiser
	virtual void UnsupervisedDiscretize(const KWTupleTable* tupleTable);

	// Groupement des valeurs non supervise d'un attribut symbolique
	virtual void UnsupervisedGroup(const KWTupleTable* tupleTable);

	///////////////////////////////////////////////////////////
	// Methodes pour la discretisation d'un attribut Continuous
	// pour la classification supervisee

	// Discretisation d'un attribut continu
	// Met a jour les attributs resultats de discretisation
	// Prerequis: les donnees doivent etre triees suivant l'attribut a discretiser
	virtual void Discretize(const KWTupleTable* tupleTable);

	// CH V9 TODO: a fusionner a terme avec la methode suivante et renommer en
	//             ComputeInitialContinuousFrequencyTable(boolean bMergePureIntervals)
	// Creation d'une table de contingence initiale a partir des objets
	// Il n'y a pas de fusion des intervalles purs dans cette methode contrairement
	// a ce qui est fait dans ComputeInitialContinuousTable()
	virtual KWFrequencyTable*
	ComputeInitialContinuousFrequencyTableWithoutPureIntervals(const KWTupleTable* tupleTable);

	// CH V9 TODO: cette methode est elle encore reellement utile
	// Creation d'une table de contingence initiale a partir des objets
	// Creations d'intervalles initiaux purs (ne contenant qu'une sorte de modalite
	// cible) ou mono-valeur, pour accelerer le travail des methodes de discretisation,
	// qui ne coupent jamais au milieu d'un intervalle pur
	virtual KWFrequencyTable* ComputeInitialContinuousFrequencyTable(const KWTupleTable* tupleTable);

	// Creation du vecteur des valeurs sources des instances
	virtual ContinuousVector* ComputeInitialSourceValues(const KWTupleTable* tupleTable);

	// Creation du vecteur des index des valeurs cibles des instances
	virtual IntVector* ComputeInitialTargetIndexes(const KWTupleTable* tupleTable);

	// Creation de la grille de preparation a partir de la table d'effectifs de discretisation
	// Si le parametre cvBounds est NULL, les bornes des intervalels sont determinees
	// en fonction des effectifs des intervalles et des valeurs des instances.
	// Sinon, les bornes utilises sont directement celle de cvBounds
	// Prerequis: les donnees doivent etre triees suivant l'attribut a discretiser
	// Memoire: l'objet retournee doit etre liberee par l'appelant
	virtual void BuildPreparedDiscretizationDataGridStats(const KWTupleTable* tupleTable,
							      const KWFrequencyTable* kwftDiscretizedTable,
							      const ContinuousVector* cvBounds);

	///////////////////////////////////////////////////////////
	// Methodes pour le groupage d'un attribut Symbol
	// pour la classification supervisee

	// Groupement des valeurs d'un attribut symbolique
	// Met a jour les attributs resultats de groupage
	virtual void Group(const KWTupleTable* tupleTable);

	// Creation d'une table d'effectifs initiale et du vecteur des valeurs
	// de l'attribut source associe par parcours de la base triee
	virtual void BuildInitialFrequencyTable(const KWTupleTable* tupleTable, int nSourceValueNumber,
						SymbolVector*& svInitialSourceModalities,
						KWFrequencyTable*& kwftInitialTable) const;

	// Creation de la grille de preparation a partir de la table initiale
	// et index de groupes pour le debut de la table
	// Memoire: l'objet retournee doit etre liberee par l'appelant
	// Ajout de l'information de presence d'une poubelle pour bien placer StarValue
	virtual void BuildPreparedGroupingDataGridStats(SymbolVector* svInitialSourceModalities,
							KWFrequencyTable* kwftGroupedTable, IntVector* ivGroups);

	///////////////////////////////////////////////////////////
	// Methodes pour le partitionnement par grille d'un attribut
	// source dans le cas d'un attribut cible numerique ou
	// symbolique a grand nombre de valeurs (a grouper)

	// Partitionnement simultane des attributs source et cible
	virtual void Group2D(const KWTupleTable* tupleTable);

	// Calcul d'une evaluation, basee sur le Level MODL, ou sinon sur un calcul d'entropie
	// (1 - MutualEntropy/TargetEntropy)
	virtual void ComputeDefaultEvaluation();

	// Attributs de base
	ALString sAttributeName;
	int nAttributeType;
	KWDescriptiveStats* kwDescriptiveStats;

	// Liste des valeurs elementaires dans le cas des atrribut categoriels
	KWDataGridStats* symbolValueStats;

	// Resultats de discretisation non supervise MODL
	KWMODLHistogramResults* modlHistogramResults;

	// Calsle en friend pour les variable partagees des methodes parallelisees
	friend class PLShared_AttributeStats;
};

// Comparaison par informativite; par nom si match nul
int KWAttributeStatsCompareLevel(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////
// Classe PLShared_AttributeStats
// Serialisation de la classe KWAttributeStats
class PLShared_AttributeStats : public PLShared_DataPreparationStats
{
public:
	// Constructeur
	PLShared_AttributeStats();
	~PLShared_AttributeStats();

	// Acces aux statistiques
	void SetAttributeStats(KWAttributeStats* attributeStats);
	KWAttributeStats* GetAttributeStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

#include "KWAttributeSubsetStats.h"
