// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWAttributeSubsetStats;
class KWAttributePairStats;
class PLShared_AttributeSubsetStats;
class PLShared_AttributePairStats;

#include "KWVersion.h"
#include "KWLearningReport.h"
#include "KWClassStats.h"
#include "KWDRPreprocessing.h"
#include "KWSortableIndex.h"
#include "KWDataGridOptimizer.h"
#include "KWDataGridCosts.h"
#include "KWDataGridManager.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////
// Classe KWAttributeSubsetStats
// Statistiques sur un sous ensemble d'attributs consideres conjointement
class KWAttributeSubsetStats : public KWDataPreparationStats
{
public:
	// Constructeur
	KWAttributeSubsetStats();
	~KWAttributeSubsetStats();

	// Nombre d'attributs a utiliser
	void SetAttributeNumber(int nValue);
	int GetAttributeNumber() const override;

	// Parametrage des attributs
	// Memoire: les specifications des attributs sont a gerer par l'appelant
	void SetAttributeNameAt(int nIndex, const ALString& sAttributeName);
	const ALString& GetAttributeNameAt(int nIndex) const override;

	// Indique si on souhaite partitionner la variable cible dans le cas supervise
	// Utilise uniquement dans le cas supervise
	void SetTargetAttributePartitioned(boolean bValue);
	boolean GetTargetAttributePartitioned() const;

	// Parametrage (facultatif) par des statistiques sur le probleme d'apprentissage
	// Permet l'utilisation des statistiques univariees pour optimiser les grilles de donnees
	// Memoire: les specifications sont referencees et destinee a etre partagees
	void SetClassStats(KWClassStats* stats);
	KWClassStats* GetClassStats() const;

	// Verification de la validite des specifications
	virtual boolean CheckSpecifications() const;

	// Calcul de statistiques et des donnees de preparation (grilles, cout MODL...)
	// Les attributs doivent etre initialises et leurs stats calculees
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	// Redefinition des criteres de tri (le SortValue correspond a l'evaluation de la grille preparee)
	const ALString GetSortName() const override;
	double GetSortValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////////
	// Rapport sur les resultats
	// Accessible uniquement si statistiques calculees

	// Ecriture d'un rapport sur la grille de donnees
	// Precede d'un tableau synthetique sur les grilles evaluees en cas de grilles multiples
	void WriteReport(ostream& ost) const override;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;

	/////////////////////////////////////////////////////////
	// Services avances, utilisable dans d'autres contextes

	// Creation d'une structure de cout selon le type de probleme (classification, regression, non supervise)
	// Memoire: appartient a l'appelant
	virtual KWDataGridCosts* CreateDataGridCost() const;

	// Creation d'une grille complete a partir des specifications des attributs a utiliser
	// Suivi de tache lors de la creation de la grille
	// Renvoie NULL si probleme ou interruption utilisateur
	// Memoire: appartient a l'appelant
	KWDataGrid* CreateDataGrid(const KWTupleTable* tupleTable);

	// Methode appelee lors de l'optimisation a chaque etape d'optimisation
	// Implementation vide par defaut
	virtual void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
					    const KWDataGrid* initialGranularizedDataGrid, boolean bIsLastSaving) const;

	// Parametrage avance
	// Pre-granularisation des attributs numeriques cible (regression) et des attributs numeriques explicatifs en
	// analyse non supervisee (co-clustering) Cette pre-granularisation permet :
	// - en regression, de limiter le nombre de valeurs cible au seuil sqrt(N log N), superieur a l'ecart type
	// theorique de la precision de prediction en sqrt(N)
	// - en coclustering
	// Defaut : true
	static boolean GetPregranularizedNumericalAttributes();
	static void SetPregranularizedNumericalAttributes(boolean bValue);

	/////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////
	// Alimentation d'un DataGrid a partir de la base de donnees
	// Contrairement au cas univarie, il n'y a pas de possibilite de
	// pretraitement par regroupement des intervalles ou groupes purs
	// dans le cas supervise. Neanmoins, seuls les attributs informatifs
	// (non reduit a une seul partie) sont pris en compte
	// La grille initiale est donc complete (avec une partie par valeur)
	// dans tous les cas (supervise et non supervise).
	// Le suivi d'avancement est gere, pour une tache pilotee par CreateDataGrid

	// Creation des partitions (en intervalles ou valeurs) d'un attribut de DataGrid
	// Renvoie true si correctement initialisee, false sinon
	virtual boolean CreateAttributeIntervals(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute);
	virtual boolean CreateAttributeValueSets(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute);

	// Creation d'une partition pre-granularise d'un attribut numerique de DataGrid
	// Renvoie true si correctement initialise, false sinon
	// En entree : nMaxValueNumber est le nombre maximal de valeur apres la pre-granularisation
	virtual boolean CreateAttributePreGranularizedIntervals(const KWTupleTable* attributeTupleTable,
								KWDGAttribute* dgAttribute, int nMaxValueNumber);

	// Alimentation des cellules d'un DataGrid dont les attributs et parties
	// sont correctement initialises
	// Renvoie true si cellule correctement initialisee, false sinon (sans nettoyage des celulles crees)
	virtual boolean CreateDataGridCells(const KWTupleTable* tupleTable, KWDataGrid* dataGrid);

	//////////////////////////////////////////////////////////
	// Services divers

	// Calcul des donnes de preparation (KWDataGridStats) a partir d'une grille
	virtual void ComputeDataPreparationResults(const KWDataGrid* dataGrid);

	// Attribut de statistiques
	KWClassStats* classStats;

	// Nom des attributs
	StringVector svAttributeNames;

	// Specification du type d'apprentissage (partition de la cible ou pas)
	// Indique si l'attribut cible fait partie des attributs du subset, et doit donc etre partitionne egalement
	// Les cas suivants sont possibles:
	//   - classification non supervisee:
	//		. TargetAttributeName == ""
	//      . IsTargetAttributePartitioned: false
	//   - classification supervisee sans groupage de la cible
	//		. TargetAttributeName != ""
	//      . LearningSpec.IsTargetGrouped = false
	//      . IsTargetAttributePartitioned = false
	//   - classification supervisee avec groupage de la cible
	//		. TargetAttributeName != ""
	//      . LearningSpec.IsTargetGrouped = true
	//      . IsTargetAttributePartitioned = true
	//   - regression supervisee sans groupage de la cible
	//		. TargetAttributeName != ""
	//      . IsTargetAttributePartitioned = true
	// Note: A terme, cette option peut etre deduite de LearningSpec.IsTargetGrouped
	boolean bIsTargetAttributePartitioned;

	// Contrainte de nombre max de cellules pour la gestion preventive de la memoire
	// Parametre avance. Par defaut: 0 signifie qu'il n'y a pas de contrainte
	int nMaxCellNumberConstraint;

	// Pre-granularisation des attributs numeriques cible (regression) et des attributs numeriques explicatifs en
	// analyse non supervisee (co-clustering)
	static boolean bPregranularizedNumericalAttributes;

	friend class PLShared_AttributeSubsetStats;
};

///////////////////////////////////////////////////////
// Classe KWAttributePairStats
// Specialisation de KWAttributeSubsetStats pour personnaliser les rapport
class KWAttributePairStats : public KWAttributeSubsetStats
{
public:
	// Constructeur (deux attributs sont declares)
	KWAttributePairStats();
	~KWAttributePairStats();

	// Parametrage des attributs (specialisation des Set/GetAttributeNameAt)
	void SetAttributeName1(const ALString& sAttributeName);
	const ALString& GetAttributeName1() const;
	void SetAttributeName2(const ALString& sAttributeName);
	const ALString& GetAttributeName2() const;

	// Redefinition de la methode dans le cas bivarie
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	// Specialisation de la structure de cout pour l'analyse bivariee
	KWDataGridCosts* CreateDataGridCost() const override;

	/////////////////////////////////////////////////////
	// Rapport sur les resultats
	// Accessible uniquement si statistiques calculees

	// Evaluation relative de la grille preparee
	// DeltaLevel = Level(att1, att2) - Level(att1) - Level(att2)
	// Egal au Level dans le cas non supervise
	double GetDeltaLevel() const;

	// Redefinition du critere de tri (avec le DeltaLevel)
	double GetSortValue() const override;

	// Ecriture d'un rapport sur la grille de donnees
	// Precede d'un tableau synthetique sur les grilles evaluees en cas de grilles multiples
	void WriteReport(ostream& ost) const override;
	boolean IsReported() const override;

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_AttributeSubsetStats
//	 Serialisation de la classe KWAttributeSubsetStats
class PLShared_AttributeSubsetStats : public PLShared_DataPreparationStats
{
public:
	// Constructeur
	PLShared_AttributeSubsetStats();
	~PLShared_AttributeSubsetStats();

	// Acces aux statistiques
	void SetAttributeSubset(KWAttributeSubsetStats* attributeSubsetStats);
	KWAttributeSubsetStats* GetAttributeSubset();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_AttributePairStats
//	 Serialisation de la classe KWAttributePairStats
class PLShared_AttributePairStats : public PLShared_AttributeSubsetStats
{
public:
	// Constructeur
	PLShared_AttributePairStats();
	~PLShared_AttributePairStats();

	// Acces aux statistiques
	void SetAttributePair(KWAttributePairStats* attributeSubsetStats);
	KWAttributePairStats* GetAttributePair();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	Object* Create() const override;
};
