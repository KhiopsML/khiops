// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

class KWAnalysisResults;

#include "KWDatabase.h"
#include "KWResultFilePathBuilder.h"
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

	// Analysis report
	const ALString& GetReportFileName() const;
	void SetReportFileName(const ALString& sValue);

	// Short description
	const ALString& GetShortDescription() const;
	void SetShortDescription(const ALString& sValue);

	// Export as xls
	boolean GetExportAsXls() const;
	void SetExportAsXls(boolean bValue);

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

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Verification du chemin de repertoire complet en sortie
	// On tente de construire les repertoires en sortie
	// On rend false avec message d'erreur si echec
	boolean CheckResultDirectory() const;

	// Construction d'un chemin de fichier en sortie a partir d'un nom de fichier specifie parmi les resultats
	// Si le repertoire des fichiers resultats n'est pas specifie, on prend celui de la base d'apprentissage
	// On prend le fichier de rapport tel quel. Pour les autre fichiers, on se base sur le fichier de rapport
	// en remplacant son suffix par le nom du fichier (ex: AnalysisResults.khj devient AnnalysisResults.model.kdic).
	// On rend vide si le fichier en entree est vide
	// Cf. classe KWResultFilePathBuilder pour le comportement detaille
	const ALString BuildOutputFilePathName(const ALString& sOutputFileName) const;

	// Parametrage du LearningProblem, pour avoir acces a son service de recherche de chemin de fichier
	void SetTrainDatabase(const KWDatabase* database);
	const KWDatabase* GetTrainDatabase() const;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sReportFileName;
	ALString sShortDescription;
	boolean bExportAsXls;
	ALString sPreparationFileName;
	ALString sTextPreparationFileName;
	ALString sTreePreparationFileName;
	ALString sPreparation2DFileName;
	ALString sModelingDictionaryFileName;
	ALString sModelingFileName;
	ALString sTrainEvaluationFileName;
	ALString sTestEvaluationFileName;

	// ## Custom implementation

	// Retourne le service de construction des chemins de fichier en sortie
	const KWResultFilePathBuilder* GetResultFilePathBuilder() const;

	// Base de donnees servant a obtenir le chemin des fichier en entree
	const KWDatabase* trainDatabase;

	// Service de construction du chemin des fichiers en sortie
	mutable KWResultFilePathBuilder resultFilePathBuilder;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAnalysisResults::GetReportFileName() const
{
	return sReportFileName;
}

inline void KWAnalysisResults::SetReportFileName(const ALString& sValue)
{
	sReportFileName = sValue;
}

inline const ALString& KWAnalysisResults::GetShortDescription() const
{
	return sShortDescription;
}

inline void KWAnalysisResults::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

inline boolean KWAnalysisResults::GetExportAsXls() const
{
	return bExportAsXls;
}

inline void KWAnalysisResults::SetExportAsXls(boolean bValue)
{
	bExportAsXls = bValue;
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

// ## Custom inlines

// ##
