// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWDatabase.h"
#include "KWResultFilePathBuilder.h"
#include "KWVersion.h"

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

	// Coclustering report
	const ALString& GetCoclusteringFileName() const;
	void SetCoclusteringFileName(const ALString& sValue);

	// Short description
	const ALString& GetShortDescription() const;
	void SetShortDescription(const ALString& sValue);

	// Export as khc
	boolean GetExportAsKhc() const;
	void SetExportAsKhc(boolean bValue);

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

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Parametrage du LearningProblem, pour avoir acces a son service de recherche de chemin de fichier
	void SetTrainDatabase(const KWDatabase* database);
	const KWDatabase* GetTrainDatabase() const;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sCoclusteringFileName;
	ALString sShortDescription;
	boolean bExportAsKhc;
	ALString sInputCoclusteringFileName;
	ALString sClusterFileName;
	ALString sPostProcessedCoclusteringFileName;
	ALString sCoclusteringDictionaryFileName;

	// ## Custom implementation

	// Base de donnees servant a obtenir le chemin des fichier en entree
	const KWDatabase* trainDatabase;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCAnalysisResults::GetCoclusteringFileName() const
{
	return sCoclusteringFileName;
}

inline void CCAnalysisResults::SetCoclusteringFileName(const ALString& sValue)
{
	sCoclusteringFileName = sValue;
}

inline const ALString& CCAnalysisResults::GetShortDescription() const
{
	return sShortDescription;
}

inline void CCAnalysisResults::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

inline boolean CCAnalysisResults::GetExportAsKhc() const
{
	return bExportAsKhc;
}

inline void CCAnalysisResults::SetExportAsKhc(boolean bValue)
{
	bExportAsKhc = bValue;
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

// ## Custom inlines

// ##