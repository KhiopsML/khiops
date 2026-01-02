// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"
#include "KWDRAll.h"
#include "KWDRTablePartition.h"
#include "KWDRTableBlock.h"

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructionRule
// Regle de construction de variable, base sur une regle de derivation
class KDConstructionRule : public Object
{
public:
	// Constructeur et destructeur
	KDConstructionRule();
	~KDConstructionRule();

	/////////////////////////////////////////////////////////////////////////////
	// Specification d'une regle

	// Nom de la regle de construction
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

	// Priorite associee a la regle (defaut: 0)
	// Sans effet autre que sur le tri a l'interface pour instant
	void SetPriority(int nValue);
	int GetPriority() const;

	// Famille de regle: unite de regroupement des regles
	void SetFamilyName(const ALString& sValue);
	const ALString& GetFamilyName() const;

	// Libelle de la regle de construction
	void SetLabel(const ALString& sValue);
	const ALString GetLabel() const;

	// Test s'il s'agit de la regle predefinie de selection d'une partie d'un ObjectArray
	// Cette regle predefinie est traitee de facon specifique pour la gestion de ses operandes
	boolean IsSelectionRule() const;

	// Regle de derivation
	// Memoire: appartient a l'appele
	void SetDerivationRule(const KWDerivationRule* derivationRule);
	const KWDerivationRule* GetDerivationRule() const;

	// Regle de derivation de type statistique de partition
	// pour gerer le cas des blocs sparse, ou des regles portant sur une meme selection
	// peuvent etre regroupee en blocs sparse
	// Memoire: appartient a l'appele
	void SetPartitionStatsRule(const KWDRTablePartitionStats* partitionStatsRule);
	const KWDRTablePartitionStats* GetPartitionStatsRule() const;

	// Regle de derivation de type statistique de bloc
	// pour gerer le cas des blocs sparse, ou des regles portant sur premier operande
	// de type relation (Entity ou Table) et un deuxieme operande simple dans un bloc,
	// peut etre regroupee en une regle avec meme premier operande et un deuxieme
	// operande de type bloc sparse
	// Memoire: appartient a l'appele
	void SetValueBlockRule(const KWDRValueBlockRule* valueBlockRule);
	const KWDRValueBlockRule* GetValueBlockRule() const;

	// Type de la regle de construction
	int GetType() const;

	// Nombre d'operandes
	int GetOperandNumber() const;

	// Acces aux operandes: ceux de la regle de derivation
	const KWDerivationRuleOperand* GetOperandAt(int nIndex) const;

	// Initialisation d'une regle de construction a partir d'une regle de derivation
	// Initialisation de la famille, du nom, du libelle et de la regle
	// Memoire: la regle de derivation en parametre appartient a l'appele apres l'appel de la methode
	// L'ancienne eventuelle regle est detruite
	void InitializeFromDerivationRule(const ALString& sFamily, const KWDerivationRule* derivationRule);

	// Regle effectivement utilise
	// Par defaut: true
	void SetUsed(boolean bValue);
	boolean GetUsed() const;

	///////////////////////////////////////////////////////////////////////////
	// Specialisation de la regle pour une classe donnee

	// Classe sur laquelle porte la regle
	void SetClassName(const ALString& sValue);
	const ALString& GetClassName() const;

	// Niveau de recursion (defaut: 1)
	// Indique a quel niveau de recursion une regle est utilisable
	// Les regles de faible niveau de recursion seront utilisee en priorite
	void SetRecursionLevel(int nValue);
	int GetRecursionLevel() const;

	////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Duplication
	KDConstructionRule* Clone() const;

	// Verification de l'integrite
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	int nPriority;
	ALString sFamilyName;
	ALString sName;
	ALString sLabel;
	const KWDerivationRule* rule;
	const KWDRTablePartitionStats* partitionRule;
	const KWDRValueBlockRule* blockRule;
	boolean bUsed;
	ALString sClassName;
	int nRecursionLevel;
	boolean bIsSelectionRule;
};

// Fonction de comparaison sur le nom de famille puis de regle
int KWConstructionRuleCompare(const void* first, const void* second);

// Fonction de comparaison sur le niveau de recursion, puis nombre d'operandes, nom de famille et de regle
int KWConstructionRuleCompareRecursionLevel(const void* first, const void* second);
