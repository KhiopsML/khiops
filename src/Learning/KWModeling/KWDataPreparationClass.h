// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPreparationClass;
class KWDataPreparationAttribute;
class KWDataPreparationTargetAttribute;

#include "KWLearningSpec.h"
#include "KWClass.h"
#include "KWClassStats.h"
#include "KWDRDataGrid.h"
#include "KWDataGridStats.h"
#include "KWDRMath.h"
#include "KWDRCompare.h"
#include "KWDRDataGridBlock.h"

///////////////////////////////////////////////////////////////////////////////
// Classe de gestion de la preparation des donnees
// Construction d'un dictionnaire de recodage des attributs a partir des
// statistiques descriptives
// Gestion des attributs et de leur recodage, en mode supervise ou non
class KWDataPreparationClass : public KWLearningService
{
public:
	// Constructeur
	KWDataPreparationClass();
	~KWDataPreparationClass();

	/////////////////////////////////////////////////////////////////////
	// Calcul des specifications de preparation
	// Creation d'une classe de preparation et de specification permettant
	// de reperer les attributs de preparation dans cette classe.
	// La classe est associee a un domaine de classe (valide, mais non
	// enregistre par l'ensemble des domaines de KWClassDomain), et peut
	// donc etre compilee directement
	// Ce domaine peut egalement contenir d'autre classes dans le cas multi-tables

	// Calcul des specifications de preparation a partir des statistiques descriptives univariees et bivariees
	void ComputeDataPreparationFromClassStats(KWClassStats* classStats);

	// Calcul des specifications de preparation a partir de statistiques multivariees
	// (le classStats est passe pour evaluer la memoire disponible)
	void ComputeDataPreparationFromAttributeSubsetStats(KWClassStats* classStats,
							    KWAttributeSubsetStats* attributeSubsetStats);

	// Nettoyage des specifications de preparation, avec ou sans destruction
	// de la classe de preparation et de son domaine
	void RemoveDataPreparation();
	void DeleteDataPreparation();

	/////////////////////////////////////////////////////////////////////
	// Acces aux specifications de preparation
	//  - un dictionnaire de preparation et son domaine
	//  - un tableau d'attribut de preparation pour les donnees sources
	//  - un attribut de preparation cible dans le cas supervise

	// Classe de preparation des donnees et son domaine
	// Cette classe contient les attributs avant et apres recodage
	// Memoire: la classe rendue (et son domaine) appartient a l'appele et est enregistre dans son propre domaine;
	//  il sera libere par l'appele, sauf en cas d'appel a RemoveDataPreparation qui permet a l'appelant
	//  d'en prendre possesion
	KWClass* GetDataPreparationClass();
	KWClassDomain* GetDataPreparationDomain();

	// Acces indexe aux specifications de preparation de l'attribut cible
	// Renvoie NULL dans le cas non supervise
	// Memoire: appartient a l'appele
	KWDataPreparationTargetAttribute* GetDataPreparationTargetAttribute();

	// Acces indexe aux specifications de preparation des attributs
	// Les attribut prepares ont leur index initialise en coherence avec le container
	// (references aux attributs avant et apres recodage, et a leurs statistiques)
	// Memoire: le container et son contenu appartiennent a l'appele
	ObjectArray* GetDataPreparationAttributes();

	// Ajout d'un block d'attributs prepares
	// Tous les attributs natifs correspondants doivent appartenir a un seul bloc
	KWAttribute* AddDataGridBlock(ObjectArray* oaDataGridBlockDataPreparationAttributes,
				      const ALString& sNameInfix);

	// Ajout d'un block d'indexes
	// Tous les attributs natifs correspondants doivent appartenir a un seul bloc
	KWAttributeBlock* AddPreparedIndexingAttributeBlock(KWAttribute* dataGridBlockAttribute,
							    ObjectArray* oaDataGridBlockDataPreparationAttributes);
	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Test de validite de la preparation
	boolean CheckDataPreparation() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Specifications de preparation
	KWClass* kwcDataPreparationClass;
	KWClassDomain* kwcdDataPreparationDomain;
	ObjectArray oaDataPreparationAttributes;
	KWDataPreparationTargetAttribute* dataPreparationTargetAttribute;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationAttribute
// Attribut de gestion de la preparation des donnees
// Lien entre attribut natif et recode, et ses statistiques descriptives
class KWDataPreparationAttribute : public Object
{
public:
	// Constructeur
	KWDataPreparationAttribute();
	~KWDataPreparationAttribute();

	// Index d'acces a l'attribut
	void SetIndex(int nValue);
	int GetIndex() const;

	//////////////////////////////////////////////////////////////////////////////////
	// Methodes d'initialisation a partir d'une classe de travail et d'une statistique
	// univariee ou multivariee
	//   - creation de l'attribut de recodage dans la classe (non recompilee)
	//   - memorisation des specifications de preparation completes (sauf index)

	// Initialisation a partir de donnees de preparation, notamment des
	// attributs de la grille de preparation et de la meta-donnee Level
	void InitFromDataPreparationStats(KWClass* kwcClass, KWDataPreparationStats* preparationStats);

	// Cle de meta donnee pour le Level
	static const ALString& GetLevelMetaDataKey();

	// Reinitialisation
	void Reset();

	//////////////////////////////////////////////////////////////////////////
	// Specifications de preparation d'un attribut recode
	// Les mises a jour sont non controlees par des assertions, ce qui permet une
	// initialisation souple des specifications
	// En consultation, des controles de coherences sont effectues par assertion

	// Attribut natif
	void SetNativeAttribute(KWAttribute* kwaNativeAttribute);
	KWAttribute* GetNativeAttribute();

	// True s'il y a un seul attribut natif et il est dans un bloc sparse
	boolean IsNativeAttributeInBlock();

	// Attribut prepare base sur une regle KWDRDataGrid
	// Il s'agit de la grille de preparation des attributs sources
	// (attributs natifs a l'origine de la grille) pour la prediction de l'attribut cible
	void SetPreparedAttribute(KWAttribute* kwaPreparedAttribute);
	KWAttribute* GetPreparedAttribute() const;

	// Statistiques de preparation (grille de preparation, evaluation...)
	void SetPreparedStats(KWDataPreparationStats* kwdpsAttributeStats);
	KWDataPreparationStats* GetPreparedStats() const;

	// True si l'attribut est predictif de la cible (disponible seulement dans le cas supervise)
	bool IsInformativeOnTarget() const;

	//////////////////////////////////////////////////////////////////////////
	// Specifications complementaires pour un attribut recode,
	// calcule a partir de deux ou plusieurs attributs natifs

	// Premier attribut natif (methodes d'emploi equivalent a Set/GetNativeAttribute)
	void SetNativeAttribute1(KWAttribute* kwaNativeAttribute);
	KWAttribute* GetNativeAttribute1();

	// Deuxieme attribut natif (utile pour un acces directe pour les paires d'attributs)
	// Renvoie NULL si pas de deuxieme attribut
	void SetNativeAttribute2(KWAttribute* kwaNativeAttribute);
	KWAttribute* GetNativeAttribute2();

	// Parametrage du nombre d'attributs natifs
	void SetNativeAttributeNumber(int nValue);
	int GetNativeAttributeNumber() const;

	// Acces aux attributs natifs (identique aux methodes precedentes pour les deux premiers)
	void SetNativeAttributeAt(int nAttribute, KWAttribute* kwaNativeAttribute);
	KWAttribute* GetNativeAttributeAt(int nAttribute);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Creation d'attributs ou de regles de preparation, exploitant la grille de donnees portee par
	// le PreparedAttribute.
	// Les attributs crees sont ajoutes a la classe de preparation
	// Les methodes sont basees sur la grille et l'utilisation des attributs predictifs
	// (attributs sources dans le cas supervise, tous les attributs dans le cas non supervise)
	// pour indexer la partie source de la grille, et des informations dediees a l'instance courante

	// Ajout d'un attribut d'indexation de partie source liee a la preparation en grille de donnees
	//  En univarie: index d'intervalle ou de groupes
	//  En multivarie: index de cellule dans une grille
	KWAttribute* AddPreparedIndexingAttribute();

	// Ajout d'un attribut d'identifiant de la partie source, afin de permettre un recodage
	KWAttribute* AddPreparedIdAttribute();

	// Ajout d'un attribut de libelle de la partie source, afin de permettre un recodage interpretable
	KWAttribute* AddPreparedLabelAttribute();

	// Ajout d'attributs de recodage disjonctif complet (0-1 binarization) de l'identifiant de partie
	// Le tableau en parametre (reinitialise) contient les attributs ajoutes, autant que de parties sources
	void AddPreparedBinarizationAttributes(ObjectArray* oaAddedAttributes);

	// Ajout d'attributs d'information conditionnelle de la source sachant la cible (en log(proba))
	// Le tableau en parametre (reinitialise) contient les attributs ajoutes, autant que de parties cibles
	void AddPreparedSourceConditionalInfoAttributes(ObjectArray* oaAddedAttributes);

	// Creation d'une regle de statistiques (KWDRDataGridStats) afin de parametrer les predicteurs
	KWDRDataGridStats* CreatePreparedStatsRule();

	///////////////////////////////////////////////////////////////////////////////////////////
	// Creation d'attributs ou de regles de preparation, exploitant les statistiques de
	// preparation des donnees, dans le cas continu

	// Ajout d'un attribut centre-reduit
	KWAttribute* AddPreparedCenterReducedAttribute();

	// Ajout d'un attribut normalise entre 0 et 1
	KWAttribute* AddPreparedNormalizedAttribute();

	// Ajout d'un attribut normalise par son rang
	KWAttribute* AddPreparedRankNormalizedAttribute();

	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Calcul d'un nom decrivant le ou les attributs natifs
	const ALString ComputeNativeAttributeName() const;

	// Test de validite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation

	/////////////////////////////////////////////////////////
	// Methodes pour l'etude des distances (mode expert)

	// Ajout d'attributs pour l'etude des distances (mode expert uniquement)
	void AddPreparedDistanceStudyAttributes(ObjectArray* oaAddedAttributes);

	// Ajout d'un attribut de self distance
	KWAttribute* AddPreparedRankNormalizedSelfDistanceAttribute();

	// Ajout d'attribut de distance pour un attribut categoriel
	void AddPreparedCategoricalDistanceAttributes1(ObjectArray* oaAddedAttributes);
	void AddPreparedCategoricalDistanceAttributes2(ObjectArray* oaAddedAttributes);

protected:
	// Initialisation d'une regle de type grille (basee sur une sous-classe de KWDRDataGridRule)
	void InitDataGridRule(KWDRDataGridRule* dataGridRule);

	// Ajout d'un attribut de preparation base sur une regle de type grille
	KWAttribute* AddDataPreparationRuleAttribute(KWDerivationRule* preparationRule,
						     const ALString& sAttributePrefix);

	// Verification de la consistence d'une specification
	boolean CheckSpecification(const ObjectArray* oaCheckedNativeAttributes, KWAttribute* kwaPreparedAttribute,
				   KWDataPreparationStats* kwdpsAttributeStats) const;

	// Specifications de la preparation d'un attribut
	int nIndex;
	KWAttribute* preparedAttribute;
	KWDataPreparationStats* preparedStats;
	ObjectArray oaNativeAttributes;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationTargetAttribute
// Attribut de gestion de la preparation des donnees  pour l'attribut cible
// Lien entre attribut natif et recode, et ses statistiques descriptives
class KWDataPreparationTargetAttribute : public Object
{
public:
	// Constructeur
	KWDataPreparationTargetAttribute();
	~KWDataPreparationTargetAttribute();

	//////////////////////////////////////////////////////////////////////////////////
	// Methode d'initialisation a partir d'une classe de travail et d'une statistique
	// univariee de la cible
	//   - creation de l'attribut de recodage dans la classe (non recompilee)
	//   - memorisation des specifications de preparation completes (sauf index)

	// Initialisation a partir de donnees de preparation, notamment des
	// attributs de la grille de preparation
	void InitFromAttributeValueStats(KWClass* kwcClass, KWDataGridStats* kwdgsAttributeValueStats);

	// Reinitialisation
	void Reset();

	//////////////////////////////////////////////////////////////////////////
	// Specifications de preparation d'un attribut recode

	// Attribut natif
	void SetNativeAttribute(KWAttribute* kwaNativeAttribute);
	KWAttribute* GetNativeAttribute();

	// Attribut prepare base sur une regle KWDRDataGrid
	// Il s'agit de la grille de preparation univariee de l'attribut cible
	// Cela permet de memoriser les statistiques sur les valeurs cibles
	void SetPreparedAttribute(KWAttribute* kwaPreparedAttribute);
	KWAttribute* GetPreparedAttribute();

	// Statistiques de preparation de l'attribut (grille des valeurs)
	void SetAttributeValueStats(KWDataGridStats* kwdgsAttributeValueStats);
	KWDataGridStats* GetAttributeValueStats();

	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Ajout d'un attribut d'indexation d'une valeur dans la grille univariee des valeurs de l'attribut
	// L'attribut native est indexe parmi les valeurs de la grille
	//  En symbolique: rang de la valeur dans la liste des valeurs de l'attribut
	//  En continuous: rang de l'instance, selon l'ordre des valeurs de l'attribut,
	//                 en cas d'egalite, renvoie le rang moyen correspondant a la valeur
	// Prerequis: spec valides
	// Renvoie l'attribut d'indexation cree, qui vient d'etre ajoute dans la base de preparation
	KWAttribute* AddPreparedIndexingAttribute();

	// Test de validite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Verification de la consistence d'une specification
	boolean CheckSpecification(KWAttribute* kwaNativeAttribute, KWAttribute* kwaPreparedAttribute,
				   KWDataGridStats* kwdgsAttributeValueStats) const;

	// Specifications de la preparation d'un attribut
	KWAttribute* nativeAttribute;
	KWAttribute* preparedAttribute;
	KWDataGridStats* attributeValueStats;
};

// Comparaison de deux preparations d'attribut par importance predictive decroissante
// Le tri se fait selon le caractere obligatoire d'abord, puis sur la valeur predictive
int KWDataPreparationAttributeCompareSortValue(const void* elem1, const void* elem2);

// Comparaison de deux preparation d'attribut venant d'un bloc sparse par son VarKey
int KWDataPreparationAttributeCompareVarKey(const void* elem1, const void* elem2);
