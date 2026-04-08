// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "DTDecisionTree.h"
#include "KWLearningReport.h"
#include "DTDecisionTreeParameter.h"

///////////////////////////////////////////////////////////////////////////
// Classe des parametres d'un classifieur Random Forest
//
// Cette classe encapsule les parametres de construction et d'optimisation
// d'une foret d'arbres de decision (Random Forest).
//
// Parametres principaux :
//   - TreesVariablesSelection : mode de selection des variables pour les arbres
//     Valeurs possibles :
//       * "Rank with replacement" : selection par rang avec remplacement (defaut)
//       * "Uniform sampling with replacement" : echantillonnage uniforme avec remplacement
//       * "Level sampling with replacement" : echantillonnage par niveau avec remplacement
//       * "Node uniform sampling with replacement" : echantillonnage uniforme par noeud avec remplacement
//       * "Node level sampling with replacement" : echantillonnage par niveau et par noeud avec remplacement
//
//   - InstancePercentage : pourcentage d'instances retenues par arbre (0.0 a 1.0, defaut: 1.0, mode expert)
//
//   - AttributePercentage : pourcentage de variables retenues par arbre (0.0 a 1.0, defaut: 0.0)
//
//   - VariableNumberMin : nombre minimum de variables a considerer (0 a 20, defaut: 2)
//
//   - RecodeRFDictionary : recodage du dictionnaire Random Forest (true/false, defaut: true, mode expert)
//
//   - DrawingType : type de tirage des instances
//     Valeurs possibles :
//       * "No replacement" : tirage sans remise (defaut)
//       * "Use out of bag" : utilisation des echantillons Out-Of-Bag
//
//   - RandomSeed : graine aleatoire pour la reproductibilite (>= 0, defaut: 0)
//
//   - InitRFOptimisation : methode d'initialisation de l'optimisation des poids
//     Valeurs possibles :
//       * "BESTTREE" : initialisation avec l'arbre au meilleur taux de compression
//       * "ALLTREE" : initialisation avec la moyenne ponderee de tous les arbres
//
//   - OptimizationLoopNumber : nombre de boucles d'optimisation (0 a 500, defaut: 10, mode expert)
//
//   - WeightedClassifier : ponderation du predicteur moyen
//     Valeurs possibles :
//       * "UNIFORM" : ponderation uniforme (defaut)
//       * "COMPRESSION RATE" : ponderation par taux de compression
//
//   - DiscretizationTargetMethod : methode de discretisation de la cible continue
//     Valeurs possibles :
//       * "MODL" : discretisation MODL (defaut)
//       * "BinaryEqualFrequency" : discretisation binaire par frequences egales
//       * "EqualFrequency" : discretisation par frequences egales
//
//   - MaxIntervalsNumberForTarget : nombre maximum d'intervalles pour la discretisation (2 a 64, defaut: 2)
//
//   - WriteDetailedStatistics : ecriture de statistiques detaillees (true/false, defaut: false, mode expert)
//
//   - DecisionTreeParameter : parametres individuels des arbres de decision
//     (voir DTDecisionTreeParameter pour details)
class DTForestParameter : public Object // public DTDecisionTreeParameter
{
public:
	DTForestParameter();
	~DTForestParameter();

	// Acces au mode de selection des variables
	ALString GetTreesVariablesSelection() const;
	void SetTreesVariablesSelection(ALString sValue);

	// Acces au pourcentage d'individus retenus par arbre
	Continuous GetInstancePercentage() const;
	void SetInstancePercentage(Continuous cValue);

	// Acces au pourcentage de variables retenues par arbre
	Continuous GetAttributePercentage() const;
	void SetAttributePercentage(Continuous cValue);

	void SetRecodeRFDictionary(boolean bValue);
	boolean GetRecodeRFDictionary() const;

	// type de tirage (avec ou sans remise)
	void SetDrawingType(DTDecisionTree::DrawingType);
	void SetDrawingType(const ALString typeLabel);
	DTDecisionTree::DrawingType GetDrawingType() const;

	const ALString GetDrawingTypeLabel() const;

	void SetInitRFOptimisation(ALString sValue);
	ALString GetInitRFOptimisation() const;

	int GetOptimizationLoopNumber() const;
	void SetOptimizationLoopNumber(int nNumber);

	// Acces au nombre de variable min
	void SetVariableNumberMin(int nNumber);
	int GetVariableNumberMin() const;

	// Acces a la graine aleatoire
	void SetRandomSeed(int nNumber);
	int GetRandomSeed() const;

	// Acces a l'indicateur de ponderation du classifieur
	void SetWeightedClassifier(ALString sValue);
	ALString GetWeightedClassifier() const;

	// discretisation d'une cible continue
	void SetDiscretizationTargetMethod(ALString sValue);
	ALString GetDiscretizationTargetMethod() const;

	// nbre max d'intervalles pour discretisation Eq Freq d'une target continue
	void SetMaxIntervalsNumberForTarget(int i);
	int GetMaxIntervalsNumberForTarget() const;

	boolean IsWriteDetailedStatistics() const;
	void SetWriteDetailedStatistics(boolean);

	// GEtDecisionTreeParameter
	DTDecisionTreeParameter* GetDecisionTreeParameter();
	void SetDecisionTreeParameter(DTDecisionTreeParameter* decisiontreeparameter);

	// Duplication de l'arbre
	DTForestParameter* Clone() const;

	void CopyFrom(const DTForestParameter* param);

	///////////////////////////////////////////////////////////
	// Ecriture de rapport

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	virtual void WriteReport(ostream& ost) const;

	static const ALString DRAWING_TYPE_NO_REPLACEMENT_LABEL;
	static const ALString DRAWING_TYPE_USE_OUT_OF_BAG_LABEL;
	static const ALString DRAWING_TYPE_ADABOOST_REPLACEMENT_LABEL;
	static const ALString HEURISTIC_NODRAW_LABEL;
	static const ALString DISCRETIZATION_EQUAL_FREQUENCY;
	static const ALString DISCRETIZATION_BINARY_EQUAL_FREQUENCY;
	static const ALString DISCRETIZATION_MODL;

	////////////////////////////////////////////////////////
	// Implementation
protected:
	friend class PLShared_ForestParameter;

	ALString sTreesVariablesSelection;

	// Pourcentage d'indivus utilises pour l'apprentissage
	// de chaque arbre
	Continuous cInstancePercentage;

	// Pourcentage de variables eligibles pour chaque noeud de l'arbre
	Continuous cKeptAttributePercentage;

	boolean bRecodeRFDictionary;

	DTDecisionTree::DrawingType drawingType;

	// graine aleatoire pour generer des arbres differents
	int nRandomSeed;

	// Pourcentage de variables minimun a prendre
	int nVariableNumberMin;

	// type d'initalisation de l'obtimisiation des poids des arbres dans le calcul global
	// BESTTREE : init = l'abres au meilleur compretion rate
	// ALLTREE : init = moyen pondere de tout les arbres
	ALString sInitRFOptimisation;

	int nOptimizationLoopNumber;

	// Ponderation ou pas du predicteur moyen
	ALString sWeightedClassifier;

	// type de discretisation d'une target continue
	ALString sDiscretizationTargetMethod;

	// nbre max d'intervalles pour discretisation Eq Freq d'une target continue
	int nMaxIntervalsNumberForTarget;

	boolean bWriteDetailedStatistics;

	// pararamettre de creation des arbres
	DTDecisionTreeParameter pDecisionTreeParameter;
};

inline DTDecisionTree::DrawingType DTForestParameter::GetDrawingType() const
{
	return drawingType;
}

inline DTDecisionTreeParameter* DTForestParameter::GetDecisionTreeParameter()
{
	return (&pDecisionTreeParameter);
}

inline void DTForestParameter::SetDecisionTreeParameter(DTDecisionTreeParameter* decisiontreeparameter)
{
	pDecisionTreeParameter.CopyFrom(decisiontreeparameter);
}

////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ForestParameter
// Serialisation de la classe DTTForestParameter
class PLShared_ForestParameter : public PLSharedObject
{
public:
	// Constructor
	PLShared_ForestParameter();
	~PLShared_ForestParameter();

	void SetForestParameter(DTForestParameter*);
	DTForestParameter* GetForestParameter() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	PLShared_DecisionTreeParameter* shared_DecisionTreeParameter;

	Object* Create() const override;
};
