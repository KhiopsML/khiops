// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridOptimizerParameters;
class PLShared_DataGridOptimizerParameters;

#include "Object.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizerParameters
//    Data Grid optimization
class KWDataGridOptimizerParameters : public Object
{
public:
	// Constructeur
	KWDataGridOptimizerParameters();
	~KWDataGridOptimizerParameters();

	// Optimization algorithm: None, Greedy, MultiStart, ou VNS (default)
	const ALString& GetOptimizationAlgorithm() const;
	void SetOptimizationAlgorithm(const ALString& sValue);

	// Optimization time (en secondes)
	// Par defaut: 0, ce qui signifie que ce parametre n'est pas actif
	// Arret des que possible des que le temps d'optimisation depasse le temps ecoule
	int GetOptimizationTime() const;
	void SetOptimizationTime(int nValue);

	// Optimization level
	// Non utilise si algorithme Greedy
	// Sinon, meta-heuristique (level = intensite de recherche sur une echelle logarithmique)
	// Par defaut: 4 (donc 2^4=16 pour l'intensite de recherche)
	int GetOptimizationLevel() const;
	void SetOptimizationLevel(int nValue);

	// Initialization based on univariate partitions (default: true)
	boolean GetUnivariateInitialization() const;
	void SetUnivariateInitialization(boolean bValue);

	// Pre-optimize each solution
	boolean GetPreOptimize() const;
	void SetPreOptimize(boolean bValue);

	// Optimize each solution
	boolean GetOptimize() const;
	void SetOptimize(boolean bValue);

	// Post-optimize each solution
	boolean GetPostOptimize() const;
	void SetPostOptimize(boolean bValue);

	// Parametre interne, pour personnalisation avancee
	const ALString& GetInternalParameter() const;
	void SetInternalParameter(const ALString& sValue);

	// Display details
	boolean GetDisplayDetails() const;
	void SetDisplayDetails(boolean bValue);

	////////////////////////////////////////////////////////
	// Divers

	// Verification des parametres
	boolean Check() const override;

	// Recopie des specifications
	void CopyFrom(const KWDataGridOptimizerParameters* kwdgopSource);

	// Duplication
	KWDataGridOptimizerParameters* Clone() const;

	// Fraicheur de l'objet, incrementee a chaque modification
	int GetFreshness() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sOptimizationAlgorithm;
	int nOptimizationTime;
	int nOptimizationLevel;
	boolean bUnivariateInitialization;
	boolean bPreOptimize;
	boolean bOptimize;
	boolean bPostOptimize;
	ALString sInternalParameter;
	boolean bDisplayDetails;
	int nFreshness;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DataGridOptimizerParameters
//	 Serialisation de la classe KWDataGridOptimizerParameters
class PLShared_DataGridOptimizerParameters : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DataGridOptimizerParameters();
	~PLShared_DataGridOptimizerParameters();

	// Acces aux spec
	void SetDataGridOptimizerParameters(KWDataGridOptimizerParameters* dataGridOptimizerParameters);
	KWDataGridOptimizerParameters* GetDataGridOptimizerParameters();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
