// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPredictor.h"
#include "KWPreprocessingSpec.h"
#include "KWAttributeConstructionSpec.h"

//////////////////////////////////////////////////////////////////////////
// Specification d'un predicteur et de ses pretraitement associes
// Les parametres des algorithmes sont specifiees de facon generique,
// puis peuvent etre verifiees, et permettre l'instanciation des
// algorithmes correspondants
class KWPredictorSpec : public Object
{
public:
	// Constructeur
	KWPredictorSpec();
	~KWPredictorSpec();

	// Nom du predicteur
	// (doit correspondre a un des predicteurs enregistres dans KWPredictor)
	void SetPredictorName(const ALString& sValue);
	const ALString& GetPredictorName() const;

	// Type de l'attribut cible (Continuous, Symbol, ou None en non supervise)
	void SetTargetAttributeType(int nValue);
	int GetTargetAttributeType() const;

	// Libelle utilisateur du predicteur
	void SetPredictorLabel(const ALString& sValue);
	const ALString& GetPredictorLabel() const;

	// Acces au predicteur pour en permettre le parametrage specifique
	// La classe retournee est une sous-classe de KWPredictor correspondant
	// a celle qui a ete enregistree par les methodes d'administration
	// dans KWPredictor. Si le nom (ou le type) a change, une nouvelle instance de predictor
	// sera retournee (et les eventuelle modifications precedentes seront perdues)
	// Memoire: appartient a l'appele
	KWPredictor* GetPredictor() const;

	// Specification de la construction d'attributs
	// Memoire: appartient a l'appele
	KWAttributeConstructionSpec* GetAttributeConstructionSpec();

	// Specification de preprocessing
	// Memoire: appartient a l'appele
	KWPreprocessingSpec* GetPreprocessingSpec();

	// Test d'integrite
	boolean Check() const override;

	// Test si un predictor est autorise (par rapport au filtre)
	// Un filtre est une liste de nom de predicteurs separes par des ';'
	// Si pas de filtre, tout est autorise
	static boolean IsPredictorUsable(const ALString& sPredictorName, const ALString& sPredictorFilter);

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sPredictorName;
	int nTargetAttributeType;
	ALString sPredictorLabel;
	mutable KWPredictor* predictor;
	KWAttributeConstructionSpec attributeConstructionSpec;
	KWPreprocessingSpec preprocessingSpec;
};
