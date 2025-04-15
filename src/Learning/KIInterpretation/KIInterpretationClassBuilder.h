// Copyright (c) 2023-2025 Orange. All rights reserved.
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

	// Import d'un predicteur a partir d'un dictionnaire quelconque
	// Renvoie true si on a pu l'importer sans probleme et s'il est interpetable
	// Emet un warning en cas d'un predicteur non interpretable (ex: regresseur)
	boolean ImportPredictor(KWClass* kwcInputPredictor);

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

	// Acces au tableau des noms variables du predicteur
	const StringVector* GetPredictorAttributeNames() const;

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

	// Cles de meta donnees pour les dictionnaires et variables d'interpretation
	static const ALString& GetIntepreterMetaDataKey();
	static const ALString& GetShapleyValueRankingMetaDataKey();
	static const ALString& GetContributionAttributeMetaDataKey();
	static const ALString& GetContributionAttributeRankMetaDataKey();
	static const ALString& GetContributionPartRankMetaDataKey();
	static const ALString& GetContributionValueRankMetaDataKey();

	// Cles de meta donnees pour les dictionnaires et variables de reneforcement
	static const ALString& GetReinforcerMetaDataKey();
	static const ALString& GetReinforcementClassMetaDataKey();
	static const ALString& GetLeverAttributeMetaDataKey();
	static const ALString& GetReinforcementInitialScoreMetaDataKey();
	static const ALString& GetReinforcementAttributeRankMetaDataKey();
	static const ALString& GetReinforcementPartRankMetaDataKey();
	static const ALString& GetReinforcementFinalScoreRankMetaDataKey();
	static const ALString& GetReinforcementClassChangeTagRankMetaDataKey();

	// Cles de meta-donne ou valeur communes
	static const ALString& GetTargetMetaDataKey();
	static const ALString& GetShapleyLabel();
	static const ALString& GetReinforcementLabel();

	//////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction d'un dictionnaire de service d'interpretation
	// Creation du dictionnaire a partir de la classe
	// - tous ses attributs sont mis en unused, sauf sa cle
	// - les eventuelles mata-data sont nettoyees
	KWClass* BuildInterpretationServiceClass(const ALString& sServiceLabel,
						 const StringVector* svServiceMetaDataKeys) const;

	// Test de compatibilite du dictionnaire a ne pas utiliser l'option group target Values
	boolean IsClassifierClassUsingTargetValueGrouping(KWClass* kwcClassifier) const;

	// Test de compatibilite du dictionnaire a ne pas utiliser les discretisation bivariees, non actuellement gerees
	boolean IsClassifierClassUsingBivariatePreprocessing(KWClass* kwcClassifier) const;

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
	KWClass* kwcPredictorClass;

	// Nom de l'attribut correspondant a la regle du predicteur
	ALString sPredictorRuleAttributeName;

	// Nom de l'attribut de prediction
	ALString sPredictionAttributeName;

	// Vecteur des valeurs cibles
	SymbolVector svTargetValues;

	// Noms des variables du predicteur
	StringVector svPredictorAttributeNames;
};

///////////////////////////////////
// Methodes en inline

inline const KWClass* KIInterpretationClassBuilder::GetPredictorClass() const
{
	require(IsPredictorImported());
	return kwcPredictorClass;
}
