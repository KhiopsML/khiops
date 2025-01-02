// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe CCAnalysisResults
//    Results
class CCAnalysisResults : public Object
{
public:
	// Constructeur
	CCAnalysisResults();
	~CCAnalysisResults();

	// Copie et duplication
	void CopyFrom(const CCAnalysisResults* aSource);
	CCAnalysisResults* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Result files directory
	const ALString& GetResultFilesDirectory() const;
	void SetResultFilesDirectory(const ALString& sValue);

	// Result files prefix
	const ALString& GetResultFilesPrefix() const;
	void SetResultFilesPrefix(const ALString& sValue);

	// Short description
	const ALString& GetShortDescription() const;
	void SetShortDescription(const ALString& sValue);

	// Coclustering report
	const ALString& GetCoclusteringFileName() const;
	void SetCoclusteringFileName(const ALString& sValue);

	// Input coclustering report
	const ALString& GetInputCoclusteringFileName() const;
	void SetInputCoclusteringFileName(const ALString& sValue);

	// Cluster table file
	const ALString& GetClusterFileName() const;
	void SetClusterFileName(const ALString& sValue);

	// Simplified coclustering report
	const ALString& GetPostProcessedCoclusteringFileName() const;
	void SetPostProcessedCoclusteringFileName(const ALString& sValue);

	// Coclustering dictionary file
	const ALString& GetCoclusteringDictionaryFileName() const;
	void SetCoclusteringDictionaryFileName(const ALString& sValue);

	// Export JSON
	boolean GetExportJSON() const;
	void SetExportJSON(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sResultFilesDirectory;
	ALString sResultFilesPrefix;
	ALString sShortDescription;
	ALString sCoclusteringFileName;
	ALString sInputCoclusteringFileName;
	ALString sClusterFileName;
	ALString sPostProcessedCoclusteringFileName;
	ALString sCoclusteringDictionaryFileName;
	boolean bExportJSON;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCAnalysisResults::GetResultFilesDirectory() const
{
	return sResultFilesDirectory;
}

inline void CCAnalysisResults::SetResultFilesDirectory(const ALString& sValue)
{
	sResultFilesDirectory = sValue;
}

inline const ALString& CCAnalysisResults::GetResultFilesPrefix() const
{
	return sResultFilesPrefix;
}

inline void CCAnalysisResults::SetResultFilesPrefix(const ALString& sValue)
{
	sResultFilesPrefix = sValue;
}

inline const ALString& CCAnalysisResults::GetShortDescription() const
{
	return sShortDescription;
}

inline void CCAnalysisResults::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

inline const ALString& CCAnalysisResults::GetCoclusteringFileName() const
{
	return sCoclusteringFileName;
}

inline void CCAnalysisResults::SetCoclusteringFileName(const ALString& sValue)
{
	sCoclusteringFileName = sValue;
}

inline const ALString& CCAnalysisResults::GetInputCoclusteringFileName() const
{
	return sInputCoclusteringFileName;
}

inline void CCAnalysisResults::SetInputCoclusteringFileName(const ALString& sValue)
{
	sInputCoclusteringFileName = sValue;
}

inline const ALString& CCAnalysisResults::GetClusterFileName() const
{
	return sClusterFileName;
}

inline void CCAnalysisResults::SetClusterFileName(const ALString& sValue)
{
	sClusterFileName = sValue;
}

inline const ALString& CCAnalysisResults::GetPostProcessedCoclusteringFileName() const
{
	return sPostProcessedCoclusteringFileName;
}

inline void CCAnalysisResults::SetPostProcessedCoclusteringFileName(const ALString& sValue)
{
	sPostProcessedCoclusteringFileName = sValue;
}

inline const ALString& CCAnalysisResults::GetCoclusteringDictionaryFileName() const
{
	return sCoclusteringDictionaryFileName;
}

inline void CCAnalysisResults::SetCoclusteringDictionaryFileName(const ALString& sValue)
{
	sCoclusteringDictionaryFileName = sValue;
}

inline boolean CCAnalysisResults::GetExportJSON() const
{
	return bExportJSON;
}

inline void CCAnalysisResults::SetExportJSON(boolean bValue)
{
	bExportJSON = bValue;
}

// ## Custom inlines

// ##
