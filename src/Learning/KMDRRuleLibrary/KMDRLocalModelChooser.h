// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef KMDR_LOCAL_MODEL_CHOOSER_H
#define KMDR_LOCAL_MODEL_CHOOSER_H

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWDRPredictor.h"
#include "KWDataGrid.h"
#include "KWDataGridStats.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWDRDataGrid.h"

///////////////////////////////////////////////////////////////
// Classe KMDRLocalModelChooser
// le role de cette regle de derivation est de choisir le bon classifieur local pour
// une instance donnee, afin de le transmettre ensuite aux attributs de prediction

class KMDRLocalModelChooser : public KWDerivationRule
{
public:
	KMDRLocalModelChooser();
	~KMDRLocalModelChooser();

	KWDerivationRule* Create() const;

	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const;

	/** Verification que la regle est completement renseignee */
	boolean CheckOperandsCompleteness(KWClass* kwcOwnerClass) const;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const;

	/** Compilation redefinie pour optimisation */
	void Compile(KWClass* kwcOwnerClass);

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	/** Test si compilation optimisee */
	boolean IsOptimized() const;

	/** Fraicheur d'optimisation */
	int nOptimizationFreshness;

	int nFirstDataGridOperand;

	KWAttribute* idClusterAttribute;
};

inline boolean KMDRLocalModelChooser::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

#endif // KMDRLocalModelChooser_H
