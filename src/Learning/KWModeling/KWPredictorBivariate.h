// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorBivariate;

#include "KWPredictor.h"
#include "KWPredictorNaiveBayes.h"

//////////////////////////////////////////////////////////////////////////////
// Predicteur bivarie
// Le predicteur doit etre parametre par un objet KWClassStats correctement
// initialise pour l'apprentissage. Les statistiques ne seront reevaluees que
// si necessaire
class KWPredictorBivariate : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	KWPredictorBivariate();
	~KWPredictorBivariate();

	// Type de predicteur disponible: classification uniquement
	boolean IsTargetTypeManaged(int nType) const override;

	// Constructeur generique
	KWPredictor* Create() const override;

	// Nom du classifier
	const ALString GetName() const override;

	// Prefixe du predicteur
	const ALString GetPrefix() const override;

	// Suffix du predicteur
	const ALString GetSuffix() const override;

	// Recopie des specifications du predicteurs
	void CopyFrom(const KWPredictor* kwpSource) override;

	///////////////////////////////////////////////////////////////////
	// Parametrage du predicteur bivarie

	// Parametrage du type de predicteur: le meilleur bivarie possible,
	// ou en parametrant le nnom de l'attribut (defaut: true)
	void SetBestBivariate(boolean bValue);
	boolean GetBestBivariate() const;

	// Parametrage du nom du premier attribut a utiliser pour le predicteur
	void SetAttributeName1(const ALString& sValue);
	const ALString& GetAttributeName1() const;

	// Parametrage du nom du second attribut a utiliser pour le predicteur
	void SetAttributeName2(const ALString& sValue);
	const ALString& GetAttributeName2() const;

	///////////////////////////////////////////////////////////////////
	// Acces au resultats apres apprentissage
	// La methode Evaluate(database) a ete reimplementee et renvoie un
	// objet KWPredictorClusterEvaluation permettant de comparer les tables
	// de contingences en apprentissage et test selon different criteres

	// Attributs sources pour le predicteur (si apprentissage reussi)
	const ALString& GetSourceAttributeName1() const;
	const ALString& GetSourceAttributeName2() const;

	// Attribut bivarie utilise pour le predicteur (si apprentissage reussi)
	const ALString& GetBivariateAttributeName() const;

	// Recherche de la grille de preparation du predicteur bivarie issu
	// de la phase d'apprentissage
	// Memoire: l'objet rendu appartient a l'appele
	const KWDataGridStats* GetTrainDataGridStats() const;

	// Libelles standard
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Parametrage
	boolean bBestBivariate;
	ALString sAttributeName1;
	ALString sAttributeName2;

	// Resultats d'apprentissage
	ALString sSourceAttributeName1;
	ALString sSourceAttributeName2;
	ALString sBivariateAttributeName;
	KWDataGridStats trainDataGridStats;
};