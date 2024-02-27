// Copyright (c) 2024 Orange. All rights reserved.
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

	// Truncation management heuristic
	boolean GetTruncationManagementHeuristic() const;
	void SetTruncationManagementHeuristic(boolean bValue);

	// Singularity removal heuristic
	boolean GetSingularityRemovalHeuristic() const;
	void SetSingularityRemovalHeuristic(boolean bValue);

	// Epsilon bin width
	double GetEpsilonBinWidth() const;
	void SetEpsilonBinWidth(double dValue);

	// Optimal algorithm
	boolean GetOptimalAlgorithm() const;
	void SetOptimalAlgorithm(boolean bValue);

	// Export result histograms
	const ALString& GetExportResultHistograms() const;
	void SetExportResultHistograms(const ALString& sValue);

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

	// Verification de l'integrite des specification
	boolean Check() const override;

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

	///////////////////////////////////////////////////////
	// Specification du contexte de l'optimisation pour
	// le parametrage des noms de fichier de log interne

	// Indique que l'on travaille avec les donnees en mode troncature (defaut: false)
	void SetTruncationMode(boolean bValue);
	boolean GetTruncationMode() const;

	// Indique que l'on travaille sur les variations de valeurs (defaut: false)
	void SetDeltaValues(boolean bValue);
	boolean GetDeltaValues() const;

	// Methode de test
	static void Test();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sHistogramCriterion;
	int nDeltaCentralBinExponent;
	boolean bTruncationManagementHeuristic;
	boolean bSingularityRemovalHeuristic;
	double dEpsilonBinWidth;
	boolean bOptimalAlgorithm;
	ALString sExportResultHistograms;
	boolean bExportInternalLogFiles;
	int nFileFormat;
	ALString sResultFilesDirectory;
	ALString sResultFilesPrefix;

	// ## Custom implementation

	// Nom de base des fichiers interne de log, en fonction du contexte de l'algorithme
	virtual const ALString GetInternalLogBaseFileName() const;

	// Indicateurs de fonctionnement interne, pour le parametrage des nom des fichiers de log
	boolean bTruncationMode;
	boolean bDeltaValues;

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

inline double MHHistogramSpec::GetEpsilonBinWidth() const
{
	return dEpsilonBinWidth;
}

inline void MHHistogramSpec::SetEpsilonBinWidth(double dValue)
{
	dEpsilonBinWidth = dValue;
}

inline boolean MHHistogramSpec::GetOptimalAlgorithm() const
{
	return bOptimalAlgorithm;
}

inline void MHHistogramSpec::SetOptimalAlgorithm(boolean bValue)
{
	bOptimalAlgorithm = bValue;
}

inline const ALString& MHHistogramSpec::GetExportResultHistograms() const
{
	return sExportResultHistograms;
}

inline void MHHistogramSpec::SetExportResultHistograms(const ALString& sValue)
{
	sExportResultHistograms = sValue;
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
