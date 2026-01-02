// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWPredictorSpec.h"
#include "KWPredictorSpecView.h"

////////////////////////////////////////////////////////////////////////////////
// Classe generique portant sur la vue des specifications
// d'un predicteur avec ses parametres de preprocessing
class KWPredictorSpecArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	KWPredictorSpecArrayView();
	~KWPredictorSpecArrayView();

	// Filtrage des predicteurs specifiables par la liste editable,
	// sous forme de la liste des noms de predictors separes par des ';'
	// Les predicteurs enregistres dans KWPredictor sont par defaut
	// tous utilisables (pas de filtre). Sinon, ils doivent egalement
	// faire partie du filtre pour etre utilisables
	void SetPredictorFilter(const ALString& sPredictorNames);
	const ALString& GetPredictorFilter();

	// Type de predicteur (cf KWPredictorSpec)
	// Conditionne le type de predicteurs que l'on peut creer
	// Doit etre initialise avant l'ouverture de la vue
	void SetTargetAttributeType(int nValue);
	int GetTargetAttributeType() const;

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur
	Object* EventNew() override;

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Redefinition de methodes pour un parametrage fin du comportement
	void ActionInsertItemBefore() override;
	void ActionInsertItemAfter() override;
	void ActionInspectItem() override;

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Liste des predicteurs autorises
	ALString sPredictorFilter;

	// Type de predicteur
	int nTargetAttributeType;
};
