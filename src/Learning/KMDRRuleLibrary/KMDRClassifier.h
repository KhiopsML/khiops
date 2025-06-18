// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWDRPredictor.h"
#include "KWDataGrid.h"
#include "KWDataGridStats.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWDRDataGrid.h"

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour un predicteur kmean

///////////////////////////////////////////////////////////////
// Classe KMDRClassifier

class KMDRClassifier : public KWDRClassifier
{
public:
	KMDRClassifier();
	~KMDRClassifier();

	KWDerivationRule* Create() const;

	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const;

	/** Verification que la regle est completement renseignee */
	boolean CheckOperandsCompleteness(KWClass* kwcOwnerClass) const;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const;

	/** Reimplementation des services de classification */
	// Valeur predite (defaut: renvoie "")
	virtual Symbol ComputeTargetValue() const;

	// Probabilite de la valeur predite (defaut: renvoie 1)
	virtual Continuous ComputeTargetProb() const;

	// Probabilite d'une valeur particuliere (defaut: 1 si "", 0 sinon)
	virtual Continuous ComputeTargetProbAt(const Symbol& sValue) const;

	// Valeur predite en prenant en compte un biais (defaut: renvoie "")
	// Chaque probabilite conditionnelle (ou score) est modifiee par un offset,
	// avant d'en prendre le max
	// La methode est tolerante a un vecteur de taille differente du nombre de
	// valeurs cibles: les valeurs en trop sont ignoree, celles en moins sont
	// considerees comme nulles
	virtual Symbol ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const;

	////////////////////////////////////////////////////////////////////
	// Compilation de la regle et services associes

	/** Compilation redefinie pour optimisation */
	void Compile(KWClass* kwcOwnerClass);

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	/** Test si compilation optimisee */
	boolean IsOptimized() const;

	/** Vecteur des probabilites conditionnelles */
	mutable ContinuousVector cvTargetProbs;

	/** Vecteurs des valeurs cibles */
	SymbolVector svTargetValues;

	/** Probabilite conditionnelle par defaut, pour les classes inconnues */
	mutable Continuous cUnknownTargetProb;

	/** Fraicheur d'optimisation */
	int nOptimizationFreshness;

	int nFirstDataGridOperand;

	KWAttribute* idClusterAttribute;
};

inline boolean KMDRClassifier::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}
