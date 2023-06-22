// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// #include "DTGlobalTag.h"

#include "KWLearningReport.h"

/// Classe des parametres d'un classifieur en arbre de decision

class DTDecisionTreeParameter : public Object
{
public:
	/// Constructeur
	DTDecisionTreeParameter();
	~DTDecisionTreeParameter();

	/// Copie et duplication des parametres de l'arbre
	DTDecisionTreeParameter* Clone() const;
	void CopyFrom(const DTDecisionTreeParameter*);

	////////////////////////////////////////////////////////
	// Acces aux attributs

	/// Acces au critere global de l'arbre
	const ALString& GetTreeCost() const;
	void SetTreeCost(const ALString& sValue);

	/// Acces au nombre max de fils par noeud
	int GetMaxChildrenNumber() const;
	void SetMaxChildrenNumber(int nNumber);

	/// Acces au nombre max de noeuds internes
	int GetMaxInternalNodesNumber() const;
	void SetMaxInternalNodesNumber(int nNumber);

	/// Acces au nombre max de feuilles
	int GetMaxLeavesNumber() const;
	void SetMaxLeavesNumber(int nNumber);

	/// Acces au nombre min d'instances par feuille
	int GetMinInstancesPerLeaveNumber() const;
	void SetMinInstancesPerLeaveNumber(int nNumber);

	/// Acces a la profondeur max
	int GetMaxDepth() const;
	void SetMaxDepth(int nNumber);

	/// affichage verbeux
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

	/// Acces le methode de discretization
	ALString GetDiscretizationMethod() const;
	void SetDiscretizationMethod(ALString);

	/// Acces le methode de groupage
	ALString GetGroupingMethod() const;
	void SetGroupingMethod(ALString);

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	void Write(ostream& ost);

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class PLShared_DecisionTreeParameter;

	/// Critere de cout de l'arbre
	ALString sTreeCost;

	/// Nombre maximum de fils
	int nMaxChildren;

	/// Profondeur max de l'arbre
	int nMaxDepth;

	/// Nbre max de feuilles
	int nMaxLeavesNumber;

	/// Nbre max de noeuds internes
	int nMaxInternalNodesNumber;

	// nb min d'elements par feuille
	int nMinInstancesPerLeaveNumber;

	/// Chargement ou pas des variables non informatives
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
