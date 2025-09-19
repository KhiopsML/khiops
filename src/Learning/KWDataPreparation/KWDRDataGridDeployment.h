// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour le deploiement des modeles de coclustering
// Ces regles sont developpees localement en attendant leur mise au point et leur
// dispatching parmi les librairies de la bibliotheque Learning

class KWDRPartIndexAt;
class KWDRPartIdAt;
class KWDRDataGridDeployment;
class KWDRPredictedPartIndex;
class KWDRPredictedPartId;
class KWDRPredictedPartDistances;
class KWDRPredictedPartFrequenciesAt;

#include "KWDRMultiTable.h"
#include "KWDRPreprocessing.h"
#include "KWDRDataGrid.h"
#include "KWDataGridStats.h"
#include "KWDataGridDeployment.h"

// Enregistrement de ces regles
void KWDRRegisterDataGridDeploymentRules();

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridAtRule
// Regle de derivation basee sur une grille et le choix d'une
// dimension dans cette grille
//   - premier operande: une grille bidimensionnelle
//   - second operande: choix de l'index d'une dimension de la grille
// Ancetre des regle de deploiement basees sur les grilles, non instanciable
class KWDRDataGridAtRule : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDataGridAtRule();
	~KWDRDataGridAtRule();

	// Verification des operandes d'une regle basee sur une grille
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	virtual void Optimize(KWClass* kwcOwnerClass);

	// Test si optimisee
	boolean IsOptimized() const;

	// Acces a la regle de grille de donnees (referencee uniquement)
	KWDRDataGrid* dataGridRule;

	// Index de l'attribut de deploiement (index interne entre 0 et K-1)
	int nDeploymentAttributeIndex;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridDeployment
// Regle de derivation basee sur une grille et le choix d'une
// dimension dans cette grille, sur laquelle il faut deployer une nouvelle instance
//   - premier operande: une grille K-dimensionnelle
//   - second operande: choix de l'index de deploiement d'une dimension de la grille
//   - operandes a partir du troisieme:
//       . vecteur de valeurs sur les autres dimensions pour un deploiement de distribution
//       . il y en a donc K-1, un par dimension de la grille, hors dimension de deploiement
//       . optionnelement, un Kieme vecteur de frequence peut etre utilise en dernier parametre
//         pour pour ponderer les valeur par un effectif
//
// En cas d'erreurs dans les argument, non detectees a la compilation de la regle (vecteurs
// de valeur de tailles differentes, effectifs negatifs...), tout le deploiement est
// effectue avec des vecteurs tous de taille vide.
// Cette regle construit une Structure(DataGridDeployment), parametre principale des
// des regle de deploiement de distribution basees sur les grilles
class KWDRDataGridDeployment : public KWDRDataGridAtRule
{
public:
	// Constructeur
	KWDRDataGridDeployment();
	~KWDRDataGridDeployment();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Acces aux resultats de deploiement
	const KWDataGridDeployment* GetDeploymentResults() const;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Calcul des statistiques de deploiement
	void ComputeDeploymentStats(const KWObject* kwoObject) const;

	// Service de deployement de grille de donnees
	mutable KWDataGridDeployment dataGridDeployment;

	// Effectif total de la grille
	mutable int nTotalDataGridFrequency;

	// Operande du vecteur de frequences optionnel (NULL si absent)
	mutable KWDerivationRuleOperand* operandeFrequencyVector;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartIndex
// Regle de derivation basee sur le deploiement par distribution
// sur un attribut de grille, renvoyant un index de partie (entre 1 et N)
class KWDRPredictedPartIndex : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPredictedPartIndex();
	~KWDRPredictedPartIndex();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartId
// Regle de derivation basee sur le deploiement par distribution
// sur un attribut de grille, renvoyant un identifiant de partie
class KWDRPredictedPartId : public KWDRPredictedPartIndex
{
public:
	// Constructeur
	KWDRPredictedPartId();
	~KWDRPredictedPartId();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Identifiant des parties
	mutable SymbolVector svPartIds;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartDistances
// Regle de derivation basee sur le deploiement par distribution
// sur un attribut de grille, renvoyant un vecteur de distances
// par partie de l'attribut de deploiement
class KWDRPredictedPartDistances : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPredictedPartDistances();
	~KWDRPredictedPartDistances();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRContinuousVector kwdrcvDistances;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartFrequenciesAt
// Regle de derivation basee sur le deploiement par distribution
// sur un attribut de grille, renvoyant un vecteur d'effectif par partie
// de l'attribut de distribution
class KWDRPredictedPartFrequenciesAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPredictedPartFrequenciesAt();
	~KWDRPredictedPartFrequenciesAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRContinuousVector kwdrcvFrequencies;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridDeploymentDistributionRule
// Regle de derivation basee sur une grille et le choix d'une
// dimension dans cette grille, sur laquelle il faut deployer une nouvelle instance
//   - premier operande: une grille bidimensionnelle
//   - second operande: choix de l'index de deploiement d'une dimension de la grille
//   - operandes a partir du troisieme:
//        vecteur de valeurs sur les autres dimensions pour un deploiement de distribution
// Ancetre des regle de deploiement de distribution basees sur les grilles, non instanciable
class KWDRDataGridDeploymentDistributionRule : public KWDRDataGridAtRule
{
public:
	// Constructeur
	KWDRDataGridDeploymentDistributionRule();
	~KWDRDataGridDeploymentDistributionRule();

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPartIndexAt
// Regle de derivation basee sur le deploiement par valeur
// sur un attribut de grille, renvoyant un index de partie (entre 1 et N)
class KWDRPartIndexAt : public KWDRDataGridAtRule
{
public:
	// Constructeur
	KWDRPartIndexAt();
	~KWDRPartIndexAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	int ComputeValueDeploymentPartIndex(const KWObject* kwoObject) const;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPartIdAt
// Regle de derivation basee sur le deploiement par valeur
// sur un attribut de grille, renvoyant un identifiant de partie
class KWDRPartIdAt : public KWDRPartIndexAt
{
public:
	// Constructeur
	KWDRPartIdAt();
	~KWDRPartIdAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Identifiant des parties
	mutable SymbolVector svPartIds;
};
