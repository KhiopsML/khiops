// Copyright (c) 2023-2026 Orange. All rights reserved.
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

	// Nombre maximum de parties (defaut: 0)
	// Permet de specifier une contrainte d'interpretabilite des resultats de pretraitement
	// Ce parametre est determine automatiquement par l'algorithme s'il vaut 0
	int GetMaxPartNumber() const;
	void SetMaxPartNumber(int nValue);

	// Temps d'optimisation (en secondes)
	// Par defaut: 0, ce qui signifie que ce parametre n'est pas actif
	// Arret des que possible des que le temps d'optimisation depasse le temps ecoule
	int GetOptimizationTime() const;
	void SetOptimizationTime(int nValue);

	// Niveau d'optimization
	// Sinon, meta-heuristique (level = intensite de recherche sur une echelle logarithmique)
	int GetOptimizationLevel() const;
	void SetOptimizationLevel(int nValue);

	// Niveau d'optimisation par defaut
	// Par defaut: 4 (donc 2^4=16 pour l'intensite de recherche)
	static int GetDefaultOptimizationLevel();

	// Initialisation basee sur des partitions univariees (default: true)
	boolean GetUnivariateInitialization() const;
	void SetUnivariateInitialization(boolean bValue);

	// Pre-optimise chaque solution
	boolean GetPreOptimize() const;
	void SetPreOptimize(boolean bValue);

	// Optimise chaque solution
	boolean GetOptimize() const;
	void SetOptimize(boolean bValue);

	// Post-optimise chaque solution
	boolean GetPostOptimize() const;
	void SetPostOptimize(boolean bValue);

	// Post-fusion des parties de variable mitoyennes d'un meme cluster pour le coclustering instances * variables
	boolean GetVarPartPostMerge() const;
	void SetVarPartPostMerge(boolean bValue);

	// VarPart Post-optimisation des parties de variable (modification des frontieres) pour le coclustering
	// instances * variables
	boolean GetVarPartPostOptimize() const;
	void SetVarPartPostOptimize(boolean bValue);

	// Parametre interne, pour personnalisation avancee
	const ALString& GetInternalParameter() const;
	void SetInternalParameter(const ALString& sValue);

	// Profiling de l'optimisation, avec memorisation dans des fichier localise au meme endroit que le rapport de coclustering
	// - details de l'optimisation dans un fichier json
	// - statistiques de l'optimisation dans un fichier tabulaire
	boolean GetOptimizationProfiling() const;
	void SetOptimizationProfiling(boolean bValue);

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

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	int nMaxPartNumber;
	int nOptimizationTime;
	int nOptimizationLevel;
	boolean bUnivariateInitialization;
	boolean bPreOptimize;
	boolean bOptimize;
	boolean bPostOptimize;
	boolean bVarPartPostMerge;
	boolean bVarPartPostOptimize;
	ALString sInternalParameter;
	boolean bOptimizationProfiling;
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
