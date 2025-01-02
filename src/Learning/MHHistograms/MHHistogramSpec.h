// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "FileService.h"
#include "KWContinuous.h"
#include "MHContinuousLimits.h"

// ##

////////////////////////////////////////////////////////////
// Classe MHHistogramSpec
//    Histogram parameters
class MHHistogramSpec : public Object
{
public:
	// Constructeur
	MHHistogramSpec();
	~MHHistogramSpec();

	// Copie et duplication
	void CopyFrom(const MHHistogramSpec* aSource);
	MHHistogramSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Histogram criterion
	const ALString& GetHistogramCriterion() const;
	void SetHistogramCriterion(const ALString& sValue);

	// Delta central bin exponent
	int GetDeltaCentralBinExponent() const;
	void SetDeltaCentralBinExponent(int nValue);

	// Max hierarchy level
	int GetMaxHierarchyLevel() const;
	void SetMaxHierarchyLevel(int nValue);

	// Max interval number
	int GetMaxIntervalNumber() const;
	void SetMaxIntervalNumber(int nValue);

	// Max coarsened histogram number
	int GetMaxCoarsenedHistogramNumber() const;
	void SetMaxCoarsenedHistogramNumber(int nValue);

	// Truncation management heuristic
	boolean GetTruncationManagementHeuristic() const;
	void SetTruncationManagementHeuristic(boolean bValue);

	// Singularity removal heuristic
	boolean GetSingularityRemovalHeuristic() const;
	void SetSingularityRemovalHeuristic(boolean bValue);

	// Outlier management heuristic
	boolean GetOutlierManagementHeuristic() const;
	void SetOutlierManagementHeuristic(boolean bValue);

	// Epsilon bin width
	double GetEpsilonBinWidth() const;
	void SetEpsilonBinWidth(double dValue);

	// Epsilon bin number
	int GetEpsilonBinNumber() const;
	void SetEpsilonBinNumber(int nValue);

	// Optimal algorithm
	boolean GetOptimalAlgorithm() const;
	void SetOptimalAlgorithm(boolean bValue);

	// Export internal log files
	boolean GetExportInternalLogFiles() const;
	void SetExportInternalLogFiles(boolean bValue);

	// File format
	int GetFileFormat() const;
	void SetFileFormat(int nValue);

	// Result files directory
	const ALString& GetResultFilesDirectory() const;
	void SetResultFilesDirectory(const ALString& sValue);

	// Result files prefix
	const ALString& GetResultFilesPrefix() const;
	void SetResultFilesPrefix(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Version de l'algorithme
	static const ALString GetVersion();

	// Indique si un critere est granularise
	boolean GetGranularizedModel() const;

	// Verification de l'integrite des specification
	boolean Check() const;

	// Ajustement des specifications en modifiant les valeurs des parametres a ignorer
	void FitSpecificationToCriterion();

	/////////////////////////////////////////////////////////////////////////////////////
	// Gestion des nombres de bin
	//
	// Le EpsilonBinNumber des parametres donne le nombre max de bins elementaires
	// a utiliser pour les histogrammes. Sa valeur par defaut est DefaultEpsilonBinNumber
	//
	// En pratique, il faut eventuelle corriger ce nombre max de bin en fonction de la plage
	// de valeur consideree, pour gere les limites de la precision des mantisse du codage
	// des valeurs numeriques, quand le nombre de valeurs numeriques codable par bin n'est
	// pas assez grand et les valeurs continues se comportent alors de facon discrete.
	// C'est le role de la methode

	// Nombre max de partiles pour une plage de valeur donnee
	// On tente d'eviter de voir apparaitre des valeurs numeriques de type Dirac
	// dans le cas de plage de valeur tres petites aux limites de la precision numerique
	// Dans le pire des cas, cela peut se reduire a un seul partile
	int ComputeMaxPartileNumber(int nTotalFrequency, Continuous cMinValue, Continuous cMaxValue) const;

	// Calcul d'un epsilon pour un bin number et une plage de valeur donnee
	// Comme dans Kontkanen et Myllymaki, on fait demarrer le premier bin a MinValue-epsilon/2
	// Dans le cas d'un seul bin (ce qui est forcement le cas s'il n'y a qu'une valeur), on ajuste
	// le epsilon de façon relative a la plage de valeur ou la valeur pour la centrer
	// On fait egalement en sorte
	Continuous ComputeEpsilonBinLength(int nTotalBinNumber, Continuous cMinValue, Continuous cMaxValue) const;

	// Calcul de la borne inf et de la borne sup d'un histogramme pour un bin number et une plage de valeur donnee
	Continuous ComputeHistogramLowerBound(int nTotalBinNumber, Continuous cMinValue, Continuous cMaxValue) const;
	Continuous ComputeHistogramUpperBound(int nTotalBinNumber, Continuous cMinValue, Continuous cMaxValue) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Gestion des nombres de bin specifiquement pour les outliers

	// Epsilon bin number utilise pour la detection des outliers
	int GetOutlierEpsilonBinNumber() const;

	// Effectif maximum par bin utilise pour la detection des outliers
	int GetOutlierMaxBinFrequency(int nDatasetFrequency) const;

	// Maximum bin number: E=10^9
	static int GetMaxEpsilonBinNumber();

	///////////////////////////////////////////////////////
	// Gestion des noms de fichier de results ou de log

	// Nom du fichier resultat de l'histogramme
	const ALString GetHistogramFileName(const ALString& sSuffix) const;

	// Nom du fichier de log interne
	// Renvoie vide si pas d'export des fichiers de log interne
	// Trois type de fichiers de logs sont gere:
	//  . Trace, en format libre, pour le pilotage golbal des algorithme
	//  . Histogram: resultat d'un histogramme interne
	//  . Optimisation: resultat de la boucle d'optimisation de l'algorithme
	//          de recherche de granularite optimale
	const ALString GetInternalTraceFileName() const;
	const ALString GetInternalHistogramFileName() const;
	const ALString GetInternalOptimizationFileName(const ALString& sOptimizationPrefix) const;

	// Nom du fichier de donnees log transformees pour la gestion des outliers
	// Renvoie vide si pas d'export des fichiers de log interne
	const ALString GetInternalOutlierLogTrDataFileName() const;

	///////////////////////////////////////////////////////
	// Specification du contexte de l'optimisation pour
	// le parametrage des noms de fichier de log interne

	// Indique que l'on travaille avec les donnees en mode troncature (defaut: false)
	void SetTruncationMode(boolean bValue);
	boolean GetTruncationMode() const;

	// Indique que l'on travaille sur les variations de valeurs (defaut: false)
	void SetDeltaValues(boolean bValue);
	boolean GetDeltaValues() const;

	// Indique que l'on travaille sur les donnees log transformees (defaut: false)
	// Permet de parametrer indirectement le nom des fichiers de log
	void SetLogTrValues(boolean bValue);
	boolean GetLogTrValues() const;

	// Indique un index de split des donnees pour l'algorithme de gestion des outliers
	// (defaut: -1, signifie qu'il n'y a pas de split)
	void SetOutlierSplitIndex(int nValue);
	int GetOutlierSplitIndex() const;

	// Indique une gestion de frontiere entre deux splits pour l'algorithme de gestion des outliers (defaut: false)
	void SetOutlierBoundary(boolean bValue);
	int GetOutlierBoundary() const;

	// Methode de test
	static void Test();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sHistogramCriterion;
	int nDeltaCentralBinExponent;
	int nMaxHierarchyLevel;
	int nMaxIntervalNumber;
	int nMaxCoarsenedHistogramNumber;
	boolean bTruncationManagementHeuristic;
	boolean bSingularityRemovalHeuristic;
	boolean bOutlierManagementHeuristic;
	double dEpsilonBinWidth;
	int nEpsilonBinNumber;
	boolean bOptimalAlgorithm;
	boolean bExportInternalLogFiles;
	int nFileFormat;
	ALString sResultFilesDirectory;
	ALString sResultFilesPrefix;

	// ## Custom implementation

	// Calcul d'un epsilon pour un bin number et une plage de valeur donnee
	// Il s'agit d'une methode interne qui permettra de calcul la borne inf et sup
	// d'une plage de valeurs. Si celles-ci sont corrigees pour des raison de precsion numerique,
	// le epsilon length "externe" sera corrige en conseequence
	Continuous InternalComputeEpsilonBinLength(int nTotalBinNumber, Continuous cMinValue,
						   Continuous cMaxValue) const;

	// Nom de base des fichiers interne de log, en fonction du contexte de l'algorithme
	const ALString GetInternalLogBaseFileName() const;

	// Indicarteeurs de fonctionnement interne, pour le parametrage des nom des fichiers de log
	boolean bTruncationMode;
	boolean bDeltaValues;
	boolean bLogTrValues;
	int nOutlierSplitIndex;
	boolean bOutlierBoundary;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& MHHistogramSpec::GetHistogramCriterion() const
{
	return sHistogramCriterion;
}

inline void MHHistogramSpec::SetHistogramCriterion(const ALString& sValue)
{
	sHistogramCriterion = sValue;
}

inline int MHHistogramSpec::GetDeltaCentralBinExponent() const
{
	return nDeltaCentralBinExponent;
}

inline void MHHistogramSpec::SetDeltaCentralBinExponent(int nValue)
{
	nDeltaCentralBinExponent = nValue;
}

inline int MHHistogramSpec::GetMaxHierarchyLevel() const
{
	return nMaxHierarchyLevel;
}

inline void MHHistogramSpec::SetMaxHierarchyLevel(int nValue)
{
	nMaxHierarchyLevel = nValue;
}

inline int MHHistogramSpec::GetMaxIntervalNumber() const
{
	return nMaxIntervalNumber;
}

inline void MHHistogramSpec::SetMaxIntervalNumber(int nValue)
{
	nMaxIntervalNumber = nValue;
}

inline int MHHistogramSpec::GetMaxCoarsenedHistogramNumber() const
{
	return nMaxCoarsenedHistogramNumber;
}

inline void MHHistogramSpec::SetMaxCoarsenedHistogramNumber(int nValue)
{
	nMaxCoarsenedHistogramNumber = nValue;
}

inline boolean MHHistogramSpec::GetTruncationManagementHeuristic() const
{
	return bTruncationManagementHeuristic;
}

inline void MHHistogramSpec::SetTruncationManagementHeuristic(boolean bValue)
{
	bTruncationManagementHeuristic = bValue;
}

inline boolean MHHistogramSpec::GetSingularityRemovalHeuristic() const
{
	return bSingularityRemovalHeuristic;
}

inline void MHHistogramSpec::SetSingularityRemovalHeuristic(boolean bValue)
{
	bSingularityRemovalHeuristic = bValue;
}

inline boolean MHHistogramSpec::GetOutlierManagementHeuristic() const
{
	return bOutlierManagementHeuristic;
}

inline void MHHistogramSpec::SetOutlierManagementHeuristic(boolean bValue)
{
	bOutlierManagementHeuristic = bValue;
}

inline double MHHistogramSpec::GetEpsilonBinWidth() const
{
	return dEpsilonBinWidth;
}

inline void MHHistogramSpec::SetEpsilonBinWidth(double dValue)
{
	dEpsilonBinWidth = dValue;
}

inline int MHHistogramSpec::GetEpsilonBinNumber() const
{
	return nEpsilonBinNumber;
}

inline void MHHistogramSpec::SetEpsilonBinNumber(int nValue)
{
	nEpsilonBinNumber = nValue;
}

inline boolean MHHistogramSpec::GetOptimalAlgorithm() const
{
	return bOptimalAlgorithm;
}

inline void MHHistogramSpec::SetOptimalAlgorithm(boolean bValue)
{
	bOptimalAlgorithm = bValue;
}

inline boolean MHHistogramSpec::GetExportInternalLogFiles() const
{
	return bExportInternalLogFiles;
}

inline void MHHistogramSpec::SetExportInternalLogFiles(boolean bValue)
{
	bExportInternalLogFiles = bValue;
}

inline int MHHistogramSpec::GetFileFormat() const
{
	return nFileFormat;
}

inline void MHHistogramSpec::SetFileFormat(int nValue)
{
	nFileFormat = nValue;
}

inline const ALString& MHHistogramSpec::GetResultFilesDirectory() const
{
	return sResultFilesDirectory;
}

inline void MHHistogramSpec::SetResultFilesDirectory(const ALString& sValue)
{
	sResultFilesDirectory = sValue;
}

inline const ALString& MHHistogramSpec::GetResultFilesPrefix() const
{
	return sResultFilesPrefix;
}

inline void MHHistogramSpec::SetResultFilesPrefix(const ALString& sValue)
{
	sResultFilesPrefix = sValue;
}

// ## Custom inlines

// ##
