// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "CCPostProcessedAttribute.h"
#include "CCCoclusteringReport.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCPostProcessingSpec
//    Simplification parameters
class CCPostProcessingSpec : public Object
{
public:
	// Constructeur
	CCPostProcessingSpec();
	~CCPostProcessingSpec();

	// Copie et duplication
	void CopyFrom(const CCPostProcessingSpec* aSource);
	CCPostProcessingSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// ShortDescription
	const ALString& GetShortDescription() const;
	void SetShortDescription(const ALString& sValue);

	// Instance number
	int GetInstanceNumber() const;
	void SetInstanceNumber(int nValue);

	// Non empty cell number
	int GetNonEmptyCellNumber() const;
	void SetNonEmptyCellNumber(int nValue);

	// Cell number
	int GetCellNumber() const;
	void SetCellNumber(int nValue);

	// Max cell number
	int GetMaxCellNumber() const;
	void SetMaxCellNumber(int nValue);

	// Max preserved information
	int GetMaxPreservedInformation() const;
	void SetMaxPreservedInformation(int nValue);

	// Total part number
	int GetTotalPartNumber() const;
	void SetTotalPartNumber(int nValue);

	// Max total part number
	int GetMaxTotalPartNumber() const;
	void SetMaxTotalPartNumber(int nValue);

	// Frequency variable
	const ALString& GetFrequencyAttribute() const;
	void SetFrequencyAttribute(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Tableau des variables (CCPostProcessedAttribute), parametres d'un post-processing coclustering
	ObjectArray* GetPostProcessedAttributes();

	// Post-traitement d'un coclustering present en entree
	// Renvoie le coclustering correctement pretraite en cas de succes
	// Reinitialise ce coclustering en cas d'erreur de specification (avec messages d'erreur)
	boolean PostProcessCoclustering(CCHierarchicalDataGrid* postProcessedCoclusteringDataGrid);

	// Mise a jour des specifications d'un coclustering a partir d'un fichier de rapport de coclustering
	// Remet a vide si pas de fichier d'entree (sans messages d'erreur) ou en cas d'erreur de specification
	void UpdateCoclusteringSpec(const ALString& sCoclusteringReportFileName);

	// Reinitialisation des contraintes de coclustering
	void ResetCoclusteringConstraints();

	// Calcul d'un suffixe a partir des contraintes
	const ALString BuildConstraintSuffix();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sShortDescription;
	int nInstanceNumber;
	int nNonEmptyCellNumber;
	int nCellNumber;
	int nMaxCellNumber;
	int nMaxPreservedInformation;
	int nTotalPartNumber;
	int nMaxTotalPartNumber;
	ALString sFrequencyAttribute;

	// ## Custom implementation

	// Tableau des variables
	ObjectArray oaPostProcessedAttributes;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCPostProcessingSpec::GetShortDescription() const
{
	return sShortDescription;
}

inline void CCPostProcessingSpec::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

inline int CCPostProcessingSpec::GetInstanceNumber() const
{
	return nInstanceNumber;
}

inline void CCPostProcessingSpec::SetInstanceNumber(int nValue)
{
	nInstanceNumber = nValue;
}

inline int CCPostProcessingSpec::GetNonEmptyCellNumber() const
{
	return nNonEmptyCellNumber;
}

inline void CCPostProcessingSpec::SetNonEmptyCellNumber(int nValue)
{
	nNonEmptyCellNumber = nValue;
}

inline int CCPostProcessingSpec::GetCellNumber() const
{
	return nCellNumber;
}

inline void CCPostProcessingSpec::SetCellNumber(int nValue)
{
	nCellNumber = nValue;
}

inline int CCPostProcessingSpec::GetMaxCellNumber() const
{
	return nMaxCellNumber;
}

inline void CCPostProcessingSpec::SetMaxCellNumber(int nValue)
{
	nMaxCellNumber = nValue;
}

inline int CCPostProcessingSpec::GetMaxPreservedInformation() const
{
	return nMaxPreservedInformation;
}

inline void CCPostProcessingSpec::SetMaxPreservedInformation(int nValue)
{
	nMaxPreservedInformation = nValue;
}

inline int CCPostProcessingSpec::GetTotalPartNumber() const
{
	return nTotalPartNumber;
}

inline void CCPostProcessingSpec::SetTotalPartNumber(int nValue)
{
	nTotalPartNumber = nValue;
}

inline int CCPostProcessingSpec::GetMaxTotalPartNumber() const
{
	return nMaxTotalPartNumber;
}

inline void CCPostProcessingSpec::SetMaxTotalPartNumber(int nValue)
{
	nMaxTotalPartNumber = nValue;
}

inline const ALString& CCPostProcessingSpec::GetFrequencyAttribute() const
{
	return sFrequencyAttribute;
}

inline void CCPostProcessingSpec::SetFrequencyAttribute(const ALString& sValue)
{
	sFrequencyAttribute = sValue;
}

// ## Custom inlines

// ##