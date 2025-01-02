// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "KWClass.h"
#include "KWDerivationRule.h"
#include "KDEntity.h"

//////////////////////////////////////////////////////////////////////////////////////
// Classe KDDomainConstraint
// Une contrainte de domaine est une specification utilisateur permettant
// de limiter l'application de certaines regles et de
// leurs operandes a certains dictionnaires et leurs variables
// Par exemple:
//    {Rule1, Rule2} in {Class1, Class2}
//    {Rule1.1} in {Class1.Var1, Class1.Var3}
// Class prototype, non utilisee actuellement
class KDDomainConstraint : public Object
{
public:
	// Constructeur et destructeur
	KDDomainConstraint();
	~KDDomainConstraint();

	// Ensemble des regles couvertes par la contrainte
	KDEntitySet* GetCoveredRules();

	// Ensemble des dictionnaires contraignant l'application des regles
	KDEntitySet* GetConstrainedClasses();

	// Verification de l'integrite
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	KDEntitySet coveredRules;
	KDEntitySet constrainedClasses;
};
