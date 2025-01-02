// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRRandom;

#include "KWDerivationRule.h"

// Enregistrement de la regle
void KWDRRegisterRandomRule();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRandom
// Reel aleatoire entre 0 et 1
// Le result est reproductible pour des objets ayant un index de creation non nul,
// si l'on part d'un meme dictionnaire (avec variables used ou non) et d'une meme table de donnees
class KWDRRandom : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRandom();
	~KWDRRandom();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class KWClass;

	// Compilation specifique a la regle Random, a appeler par KWClass lors de sa compilation
	// L'object est de parametrer le generateur aleatoire en fonction du nom de la classe,
	// de l'attribut, et du rang d'utilisation de la regle localement a l'attribut
	void InitializeRandomParameters(const ALString& sCompiledClassName, const ALString& sAttributeName,
					int nRuleRankInAttribute);

	// Parametrage du generateur aleatoire
	longint lSeed;
	longint lLeap;
};
