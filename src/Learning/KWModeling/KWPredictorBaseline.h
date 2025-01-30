// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPredictor.h"
#include "KWPredictorNaiveBayes.h"

//////////////////////////////////////////////////////////////////////////////
// Predicteur de base, n'exploitant aucun attribut predictif
// Le predicteur doit etre parametre par un objet KWClassStats correctement
// initialise pour l'apprentissage.
class KWPredictorBaseline : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	KWPredictorBaseline();
	~KWPredictorBaseline();

	// Constructeur generique
	KWPredictor* Create() const;

	// Nom du classifier
	const ALString GetName() const;

	// Prefixe du predicteur
	const ALString GetPrefix() const;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain();
};
