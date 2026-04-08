// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningReport.h"

///////////////////////////////////////////////////////////////////////////
// Classe des parametres d'un classifieur en arbre de decision
//
// Cette classe encapsule les parametres de construction et d'optimisation
// d'un arbre de decision.
//
// Parametres principaux :
//   - TreeCost : critere de cout global de l'arbre
//     Valeurs possibles (mode expert) :
//       * "REC" : Recursive partitioning
//       * "REC Rissanen" : avec correction de Rissanen
//       * "REC Rissanen V1" : variante 1
//       * "REC Rissanen V2" : variante 2
//       * "REC N2" : avec correction N2
//       * "REC New Bin" : nouveau binaire (defaut)
//       * "REC Bin" : binaire
//       * "REC Tri" : ternaire
//       * "Schroder" : nombres de Schroder
//       * "Catalan" : nombres de Catalan
//     Valeurs possibles (mode standard) :
//       * "REC New Bin" : nouveau binaire (defaut)
//       * "REC Bin" : binaire
//
//   - MaxChildrenNumber : nombre maximum de fils par noeud (>= 0, defaut: 0 = illimite)
//
//   - MaxInternalNodesNumber : nombre maximum de noeuds internes (>= 0, defaut: 0 = illimite)
//
//   - MaxLeavesNumber : nombre maximum de feuilles (>= 0, defaut: 0 = illimite, mode expert)
//
//   - MinInstancesPerLeaveNumber : nombre minimum d'instances par feuille (>= 0, defaut: 8, mode expert)
//
//   - MaxDepth : profondeur maximale de l'arbre (>= 0, defaut: 0 = illimite)
//
//   - NodeVariablesSelection : mode de selection des variables a chaque noeud
//     Valeurs possibles :
//       * "Identical" : selection identique a tous les noeuds (defaut)
//       * "Random uniform" : selection aleatoire uniforme
//       * "Random level" : selection aleatoire par niveau
//
//   - AttributesSplitSelection : mode de selection des partages d'attributs
//     Valeurs possibles :
//       * "Best tree cost" : meilleur cout d'arbre (defaut)
//       * "Random uniform" : selection aleatoire uniforme
//       * "Random level" : selection aleatoire par niveau
//
//   - PruningMode : mode d'elagage de l'arbre
//     Valeurs possibles :
//       * "Pre-pruning" : elagage avant construction
//       * "Post-pruning" : elagage apres construction (defaut)
//       * "No pruning" : pas d'elagage
//
//   - DiscretizationMethod : methode de discretisation des variables continues
//     Valeurs possibles :
//       * "MODL" : discretisation MODL (defaut)
//       * "MODL Equal frequency" : MODL avec frequences egales
//
//   - GroupingMethod : methode de groupage des modalites
//     Valeurs possibles :
//       * "MODL" : groupage MODL (defaut)
//       * "Basic grouping" : groupage basique
//
//   - UnloadNonInformativeAttributes : dechargement des variables non informatives (true/false, defaut: true)
//
//   - VerboseMode : mode d'affichage detaille (true/false, defaut: false)
class DTDecisionTreeParameter : public Object
{
public:
	// Constructeur
	DTDecisionTreeParameter();
	~DTDecisionTreeParameter();

	// Copie et duplication des parametres de l'arbre
	DTDecisionTreeParameter* Clone() const;
	void CopyFrom(const DTDecisionTreeParameter*);

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Acces au critere global de l'arbre
	const ALString& GetTreeCost() const;
	void SetTreeCost(const ALString& sValue);

	// Acces au nombre max de fils par noeud
	int GetMaxChildrenNumber() const;
	void SetMaxChildrenNumber(int nNumber);

	// Acces au nombre max de noeuds internes
	int GetMaxInternalNodesNumber() const;
	void SetMaxInternalNodesNumber(int nNumber);

	// Acces au nombre max de feuilles
	int GetMaxLeavesNumber() const;
	void SetMaxLeavesNumber(int nNumber);

	// Acces au nombre min d'instances par feuille
	int GetMinInstancesPerLeaveNumber() const;
	void SetMinInstancesPerLeaveNumber(int nNumber);

	// Acces a la profondeur max
	int GetMaxDepth() const;
	void SetMaxDepth(int nNumber);

	// affichage verbeux
	boolean GetVerboseMode() const;
	void SetVerboseMode(boolean);

	boolean GetUnloadNonInformativeAttributes() const;
	void SetUnloadNonInformativeAttributes(boolean bUnLoad);

	ALString GetNodeVariablesSelection() const;
	void SetNodeVariablesSelection(ALString);

	ALString GetAttributesSplitSelection() const;
	void SetAttributesSplitSelection(ALString);

	ALString GetPruningMode() const;
	void SetPruningMode(ALString);

	// Acces le methode de discretization
	ALString GetDiscretizationMethod() const;
	void SetDiscretizationMethod(ALString);

	// Acces le methode de groupage
	ALString GetGroupingMethod() const;
	void SetGroupingMethod(ALString);

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	void Write(ostream& ost);

	////////////////////////////////////////////////////////
	// Implementation
protected:
	friend class PLShared_DecisionTreeParameter;

	// Critere de cout de l'arbre
	ALString sTreeCost;

	// Nombre maximum de fils
	int nMaxChildren;

	// Profondeur max de l'arbre
	int nMaxDepth;

	// Nbre max de feuilles
	int nMaxLeavesNumber;

	// Nbre max de noeuds internes
	int nMaxInternalNodesNumber;

	// nb min d'elements par feuille
	int nMinInstancesPerLeaveNumber;

	// Chargement ou pas des variables non informatives
	boolean bUnloadNonInformativeAttributes;

	// mode de selection des noeuds
	ALString sNodeVariablesSelection;

	// mode de partage des attributs
	ALString sAttributesSplitSelection;

	ALString sPruningMode;

	ALString sDiscretizationMethod;

	ALString sGroupingMethod;

	boolean bVerboseMode;
};

////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DecisionTreeParameter
// Serialisation de la classe DTDecisionTreeParameter
class PLShared_DecisionTreeParameter : public PLSharedObject
{
public:
	// Constructor
	PLShared_DecisionTreeParameter();
	~PLShared_DecisionTreeParameter();

	void SetDecisionTreeParameter(DTDecisionTreeParameter*);
	DTDecisionTreeParameter* GetDecisionTreeParameter() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	Object* Create() const override;
};
