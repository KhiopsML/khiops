// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDFeatureConstruction;

#include "KWClass.h"
#include "KWLearningReport.h"
#include "KDConstructionDomain.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDFeatureConstruction
// Pilotage de la construction de variables
class KDFeatureConstruction : public KWLearningReport
{
public:
	// Constructeur et destructeur
	KDFeatureConstruction();
	~KDFeatureConstruction();

	// Parametrage de construction: ensemble des regles de construction utilisables
	// Attention, parametrage obligatoire (initialement a NULL)
	// Memoire: appartient a l'appelant
	void SetConstructionDomain(KDConstructionDomain* constructionDomainParam);
	KDConstructionDomain* GetConstructionDomain() const;

	// Calcul des couts de construction pour les attributs initiaux d'une classe
	// Methode avancee appelable sur une classe quelconque, ayant meme attribut cible que dans le learningSpec
	// Tous les attributs initiaux ont un cout homogene
	void ComputeInitialAttributeCosts(KWClass* kwcValue) const;

	// Import des couts de construction par attribut a a partir des meta-donnes (Cost) des attributs
	// Methode avancee appelable sur une classe quelconque
	// Renvoie true si ensemble des cout coherents, et importe effectivement les couts
	// Sinon, renvoie false avec des messages d'erreur
	boolean ImportAttributeMetaDataCosts(KWClass* kwcValue);

	// Test si les cout des attribut sont correctement initialises
	boolean AreAttributeCostsInitialized(KWClass* kwcValue) const;

	// Service de collecte des attributs crees dans une classe construite, par rapport a une classe initiale
	// En sortie, on fournit les attributs utilises construits references par leur nom
	void CollectConstructedAttributes(const KWClass* kwcInitialClass, const KWClass* kwcConstructedClass,
					  ObjectDictionary* odConstructedAttributes) const;

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Domaine de construction
	KDConstructionDomain* constructionDomain;
};
