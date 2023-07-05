// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWAnalysisResults
//    Results
class KWAnalysisResults : public Object
{
public:
	// Constructeur
	KWAnalysisResults();
	~KWAnalysisResults();

	// Copie et duplication
	void CopyFrom(const KWAnalysisResults* aSource);
	KWAnalysisResults* Clone() const;

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

	// Preparation report
	const ALString& GetPreparationFileName() const;
	void SetPreparationFileName(const ALString& sValue);

	// Text preparation report
	const ALString& GetTextPreparationFileName() const;
	void SetTextPreparationFileName(const ALString& sValue);

	// Tree preparation report
	const ALString& GetTreePreparationFileName() const;
	void SetTreePreparationFileName(const ALString& sValue);

	// 2D preparation report
	const ALString& GetPreparation2DFileName() const;
	void SetPreparation2DFileName(const ALString& sValue);

	// Modeling dictionary file
	const ALString& GetModelingDictionaryFileName() const;
	void SetModelingDictionaryFileName(const ALString& sValue);

	// Modeling report
	const ALString& GetModelingFileName() const;
	void SetModelingFileName(const ALString& sValue);

	// Train evaluation report
	const ALString& GetTrainEvaluationFileName() const;
	void SetTrainEvaluationFileName(const ALString& sValue);

	// Test evaluation report
	const ALString& GetTestEvaluationFileName() const;
	void SetTestEvaluationFileName(const ALString& sValue);

	// Visualization report
	const ALString& GetVisualizationFileName() const;
	void SetVisualizationFileName(const ALString& sValue);

	// JSON report
	const ALString& GetJSONFileName() const;
	void SetJSONFileName(const ALString& sValue);

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
	ALString sPreparationFileName;
	ALString sTextPreparationFileName;
	ALString sTreePreparationFileName;
	ALString sPreparation2DFileName;
	ALString sModelingDictionaryFileName;
	ALString sModelingFileName;
	ALString sTrainEvaluationFileName;
	ALString sTestEvaluationFileName;
	ALString sVisualizationFileName;
	ALString sJSONFileName;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAnalysisResults::GetResultFilesDirectory() const
{
	return sResultFilesDirectory;
}

inline void KWAnalysisResults::SetResultFilesDirectory(const ALString& sValue)
{
	sResultFilesDirectory = sValue;
}

inline const ALString& KWAnalysisResults::GetResultFilesPrefix() const
{
	return sResultFilesPrefix;
}

inline void KWAnalysisResults::SetResultFilesPrefix(const ALString& sValue)
{
	sResultFilesPrefix = sValue;
}

inline const ALString& KWAnalysisResults::GetShortDescription() const
{
	return sShortDescription;
}

inline void KWAnalysisResults::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

inline const ALString& KWAnalysisResults::GetPreparationFileName() const
{
	return sPreparationFileName;
}

inline void KWAnalysisResults::SetPreparationFileName(const ALString& sValue)
{
	sPreparationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetTextPreparationFileName() const
{
	return sTextPreparationFileName;
}

inline void KWAnalysisResults::SetTextPreparationFileName(const ALString& sValue)
{
	sTextPreparationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetTreePreparationFileName() const
{
	return sTreePreparationFileName;
}

inline void KWAnalysisResults::SetTreePreparationFileName(const ALString& sValue)
{
	sTreePreparationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetPreparation2DFileName() const
{
	return sPreparation2DFileName;
}

inline void KWAnalysisResults::SetPreparation2DFileName(const ALString& sValue)
{
	sPreparation2DFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetModelingDictionaryFileName() const
{
	return sModelingDictionaryFileName;
}

inline void KWAnalysisResults::SetModelingDictionaryFileName(const ALString& sValue)
{
	sModelingDictionaryFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetModelingFileName() const
{
	return sModelingFileName;
}

inline void KWAnalysisResults::SetModelingFileName(const ALString& sValue)
{
	sModelingFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetTrainEvaluationFileName() const
{
	return sTrainEvaluationFileName;
}

inline void KWAnalysisResults::SetTrainEvaluationFileName(const ALString& sValue)
{
	sTrainEvaluationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetTestEvaluationFileName() const
{
	return sTestEvaluationFileName;
}

inline void KWAnalysisResults::SetTestEvaluationFileName(const ALString& sValue)
{
	sTestEvaluationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetVisualizationFileName() const
{
	return sVisualizationFileName;
}

inline void KWAnalysisResults::SetVisualizationFileName(const ALString& sValue)
{
	sVisualizationFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetJSONFileName() const
{
	return sJSONFileName;
}

inline void KWAnalysisResults::SetJSONFileName(const ALString& sValue)
{
	sJSONFileName = sValue;
}

// ## Custom inlines

// ##