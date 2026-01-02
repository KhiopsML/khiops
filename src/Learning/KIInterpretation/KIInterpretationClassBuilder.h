// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KIModelInterpreter;
class KIModelReinforcer;
class KIPredictorAttribute;
class KIInterpretationClassBuilder;

#include "KWClass.h"
#include "KWDRNBPredictor.h"
#include "KWTrainedPredictor.h"
#include "KIDRInterpretation.h"
#include "KIDRReinforcement.h"

///////////////////////////////////////////////////////////////
// Classe KIInterpretationClassBuilder
// Creation d'une classe dediee a l'interpretation
// ou au renforcement d'un predicteur
class KIInterpretationClassBuilder : public Object
{
public:
	// Constructeur
	KIInterpretationClassBuilder();
	~KIInterpretationClassBuilder();

	//////////////////////////////////////////////////////////////////////
	// Import d'un predicteur

	// Import d'un predicteur a partir d'un dictionnaire compile quelconque
	// Renvoie true si on a pu l'importer sans probleme et s'il est interpetable
	// Emet un warning en cas d'un predicteur non interpretable (ex: regresseur)
	boolean ImportPredictor(const KWClass* kwcInputPredictor);

	// Indique si un predicteur a ete importe
	boolean IsPredictorImported() const;

	// Nettoyage
	void Clean();

	//////////////////////////////////////////////////////////////////////
	// Acces aux caracteristiques d'un predicteur importe

	// Acces au dictionnaire du predicteur
	// Retourne NULL si aucun predicteur valide n'a ete importe
	const KWClass* GetPredictorClass() const;

	// Acces a la liste des valeurs cible
	const SymbolVector* GetTargetValues() const;

	// Nombre de variables du predicteur
	int GetPredictorAttributeNumber() const;

	// Acces au tableau des noms de variables du predicteur
	const StringVector* GetPredictorAttributeNames() const;

	// Acces au tableau des noms de variables de grille du predicteur
	const StringVector* GetPredictorDataGridAttributeNames() const;

	// Service de construction du tableau des attribut du predicteur, tries par importance decroissante
	// Memoire: le contenu du tableau comprend des KIPredictorAttributes, a detruire par l'appelant
	void BuildPredictorAttributes(ObjectArray* oaPredictorAttributes) const;

	//////////////////////////////////////////////////////////////////////
	// Construction de dictionnaires d'interpretation et de renforcement
	// La classe construite et son domaine appartiennent a l'appelant,
	// qui doit les liberer

	// Construction d'un dictionnaire d'interpretation
	KWClass* BuildInterpretationClass(const KIModelInterpreter* modelInterpreterSpec) const;

	// Construction d'un dictionnaire de renforcement
	KWClass* BuildReinforcementClass(const KIModelReinforcer* modelReinforcerSpec) const;

	// Cles de meta-donnees pour les dictionnaires et variables d'interpretation
	static const ALString& GetIntepreterMetaDataKey();
	static const ALString& GetShapleyValueRankingMetaDataKey();
	static const ALString& GetContributionAttributeMetaDataKey();
	static const ALString& GetContributionAttributeRankMetaDataKey();
	static const ALString& GetContributionPartRankMetaDataKey();
	static const ALString& GetContributionValueRankMetaDataKey();

	// Cles de meta-donnees pour les dictionnaires et variables de renforcement
	static const ALString& GetReinforcerMetaDataKey();
	static const ALString& GetReinforcementClassMetaDataKey();
	static const ALString& GetLeverAttributeMetaDataKey();
	static const ALString& GetReinforcementInitialScoreMetaDataKey();
	static const ALString& GetReinforcementAttributeRankMetaDataKey();
	static const ALString& GetReinforcementPartRankMetaDataKey();
	static const ALString& GetReinforcementFinalScoreRankMetaDataKey();
	static const ALString& GetReinforcementClassChangeTagRankMetaDataKey();

	// Cles de meta-donnees dediees aux attributs cree pour les pair ou valeur communes
	static const ALString& GetIsPairMetaDataKey();
	static const ALString& GetName1MetaDataKey();
	static const ALString& GetName2MetaDataKey();

	// Cles de meta-donnee ou valeur communes
	static const ALString& GetTargetMetaDataKey();
	static const ALString& GetShapleyLabel();
	static const ALString& GetReinforcementLabel();

	//////////////////////////////////////////////////
	///// Implementation
protected:
	// Creation d'un dictionnaire ayant les meme nom de variables et de blocs qu'un dictionnaire en entree,
	// uniquement pour beneficier des service de creation de nom de variable unique
	// Utile pour la creation de nom de paires de variables uniques
	KWClass* BuildShadowClass(const KWClass* kwcInputClass) const;

	// Import des meta-data des variables du predictor
	void ImportPredictorVariablesMetaData(const KWClass* kwcInputPredictor);

	// Message d'erreur ou de warning lie au predicteur un predicteur
	void AddPredictorError(const KWClass* kwcInputPredictor, const ALString& sLabel) const;
	void AddPredictorWarning(const KWClass* kwcInputPredictor, const ALString& sLabel) const;

	// Construction d'un dictionnaire de service d'interpretation
	// Creation du dictionnaire a partir de la classe
	// - creation potentielle de variables dediees au paires utilisees par le predicteur
	// - tous ses attributs sont mis en unused, sauf sa cle
	// - les eventuelles meta-data sont nettoyees
	KWClass* BuildInterpretationServiceClass(const ALString& sServiceLabel,
						 const StringVector* svServiceMetaDataKeys) const;

	// Creation d'un attribut dediee a une paire utilisee par le predicteur
	KWAttribute* CreatePairAttribute(KWClass* kwcInterpretationServiceClass, const ALString& sPairAttributeName,
					 const ALString& sPairName1, const ALString& sPairName2,
					 const ALString& sPairDataGridAttributeName, double dLevel, double dWeight,
					 double dImportance) const;

	////////////////////////////////////////////////////////////////////////////
	// Creation des attributs du dictionnaite d'interpretation

	// Creation de tous les attributs du dictionnaire d'interpretation
	void CreateInterpretationAttributes(KWClass* kwcInterpretationClass, const KWAttribute* predictorRuleAttribute,
					    const SymbolVector* svInterpretedTargetValues, boolean bIsGlobalRanking,
					    int nContributionAttributeNumber) const;

	// Creation dans la classe de l'attribut gerant l'interpretation
	KWAttribute* CreateInterpreterAttribute(KWClass* kwcInterpretationClass,
						const KWAttribute* predictorRuleAttribute) const;

	// Creation dans la classe d'un attribut de contribution pour un valeur cible et un attribut
	// La regle de construction, a creer par l'appelant, est le parametre principal
	KWAttribute* CreateContributionAttribute(KWClass* kwcInterpretationClass,
						 const KWAttribute* interpreterAttribute,
						 KWDerivationRule* kwdrContributionRule, Symbol sTargetValue,
						 const ALString& sAttributeName,
						 const ALString& sAttributeMetaDataKey) const;

	// Creation dans la classe d'un attribut de contribution pour une valeur cible et et un rang de contribution
	// La regle de construction, a creer par l'appelant, est le parametre principal
	KWAttribute* CreateRankedContributionAttribute(KWClass* kwcInterpretationClass,
						       const KWAttribute* interpreterAttribute,
						       KWDerivationRule* kwdrRankedContributionRule,
						       const ALString& sBaseName, Symbol sTargetValue, int nRank,
						       const ALString& sRankMetaDataKey) const;

	////////////////////////////////////////////////////////////////////////////
	// Creation des attributs du dictionnaite de renforcement

	// Creation de tous les attributs du dictionnaire de renforcement
	void CreateReinforcementAttributes(KWClass* kwcReinforcementClass, const KWAttribute* predictorRuleAttribute,
					   const SymbolVector* svReinforcedTargetValues,
					   const StringVector* svReinforcementAttributes) const;

	// Creation dans la classe de l'attribut gerant le renforcement
	KWAttribute* CreateReinforcerAttribute(KWClass* kwcReinforcementClass,
					       const KWAttribute* predictorRuleAttribute,
					       const StringVector* svReinforcementAttributes) const;

	// Creation dans la classe d'un attribut de renforcement pour une valeur cible et et un rang de renforcement
	// La regle de construction, a creer par l'appelant, est le parametre principal
	// Le rang est a -1 s'il n'est pas utilise
	KWAttribute* CreateRankedReinforcementAttribute(KWClass* kwcReinforcementClass,
							const KWAttribute* reinforcerAttribute,
							KWDerivationRule* kwdrRankedReinforcementRule,
							const ALString& sBaseName, Symbol sTargetValue, int nRank,
							const ALString& sRankMetaDataKey) const;

	////////////////////////////
	// Variables de la classe

	// Dictionnaire du predicteur
	const KWClass* kwcPredictorClass;

	// Nom de l'attribut correspondant a la regle du predicteur
	ALString sPredictorRuleAttributeName;

	// Nom de l'attribut de prediction
	ALString sPredictionAttributeName;

	// Vecteur des valeurs cibles
	SymbolVector svTargetValues;

	// Noms des variables du predicteur
	StringVector svPredictorAttributeNames;

	// Noms des variables de grilles du predicteur
	StringVector svPredictorDataGridAttributeNames;

	// Regles de variables preparees du predicteur dans le cas dense
	ObjectArray oaPredictorDenseAttributeDataGridStatsRules;

	// Donnees de type Weight, Level et Importance par variable du predicteur
	DoubleVector dvPredictorAttributeLevels;
	DoubleVector dvPredictorAttributeWeights;
	DoubleVector dvPredictorAttributeImportances;
};

///////////////////////////////////
// Methodes en inline

inline const KWClass* KIInterpretationClassBuilder::GetPredictorClass() const
{
	require(IsPredictorImported());
	return kwcPredictorClass;
}
