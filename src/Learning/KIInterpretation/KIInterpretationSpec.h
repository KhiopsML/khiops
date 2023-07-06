// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClassSpec.h"
#include "KIInterpretationDictionary.h"

class KIInterpretationDictionary;
class KWClassSpec;

/// Classe rassemblant les parametres d'un interpreteur de scores
class KIInterpretationSpec : public Object
{
public:
	/// constructeur
	KIInterpretationSpec();
	/// destructeur
	~KIInterpretationSpec();

	void SetDefaultParameters();

	KWClassSpec* GetLeverClassSpec() const;

	int GetHowAttributesNumber() const;
	void SetHowAttributesNumber(int nNumber);

	int GetWhyAttributesNumber() const;
	void SetWhyAttributesNumber(int nNumber);

	int GetMaxAttributesNumber() const;
	void SetMaxAttributesNumber(int nNumber);

	boolean GetSortWhyResults() const;
	void SetSortWhyResults(boolean bValue);

	ALString GetHowClass() const;
	void SetHowClass(ALString sValue);

	ALString GetWhyClass() const;
	void SetWhyClass(ALString sValue);

	ALString GetWhyType() const;
	void SetWhyType(ALString sValue);

	boolean IsExpertMode() const;
	void SetExpertMode(boolean bValue);

	void SetInterpretationDictionary(KIInterpretationDictionary*);
	KIInterpretationDictionary* GetInterpretationDictionary() const;

	// Duplication des parametres
	KIInterpretationSpec* Clone() const;

	// Recopie des parametres
	void CopyFrom(KIInterpretationSpec*);

	///////////////////////////////////////////////////////////
	// Ecriture de rapport

	// Ecriture d'un rapport
	// Accessible uniquement si statistiques calculees
	void WriteReport(ostream& ost);

	static const char* PREDICTED_CLASS_LABEL;
	static const char* CLASS_OF_HIGHEST_GAIN_LABEL;
	static const char* ALL_CLASSES_LABEL;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	KIInterpretationDictionary* interpretationDictionary;
	KWClassSpec* leverClassSpec;
	ALString sHowClass;
	ALString sWhyClass;
	ALString sWhyType;
	int nWhyAttributesNumber;
	int nHowAttributesNumber;
	int nVariableMaxNumber;
	boolean bSortWhyResults;
	boolean bExpertMode; // pour supprimer l'ecriture de certaines colonnes des contributions
};

inline KIInterpretationDictionary* KIInterpretationSpec::GetInterpretationDictionary() const
{
	return interpretationDictionary;
}
