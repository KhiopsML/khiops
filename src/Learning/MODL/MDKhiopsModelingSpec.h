// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"
#include "KWModelingSpec.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe MDKhiopsModelingSpec
//    Modeling parameters
class MDKhiopsModelingSpec : public KWModelingSpec
{
public:
	// Constructeur
	MDKhiopsModelingSpec();
	~MDKhiopsModelingSpec();

	// Copie et duplication
	void CopyFrom(const MDKhiopsModelingSpec* aSource);
	MDKhiopsModelingSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Selective Naive Bayes predictor
	boolean GetSelectiveNaiveBayesPredictor() const;
	void SetSelectiveNaiveBayesPredictor(boolean bValue);

	// MAP Naive Bayes predictor
	boolean GetMAPNaiveBayesPredictor() const;
	void SetMAPNaiveBayesPredictor(boolean bValue);

	// Naive Bayes predictor
	boolean GetNaiveBayesPredictor() const;
	void SetNaiveBayesPredictor(boolean bValue);

	// Data Grid predictor
	boolean GetDataGridPredictor() const;
	void SetDataGridPredictor(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

#ifdef DEPRECATED_V10
	// DEPRECATED V10: test si un champ a ete modifie par rapport a une version de reference
	boolean DEPRECATEDIsUpdated(const KWModelingSpec* source) const override;
	void DEPRECATEDCopyFrom(const KWModelingSpec* source) override;
#endif // DEPRECATED_V10

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bSelectiveNaiveBayesPredictor;
	boolean bMAPNaiveBayesPredictor;
	boolean bNaiveBayesPredictor;
	boolean bDataGridPredictor;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean MDKhiopsModelingSpec::GetSelectiveNaiveBayesPredictor() const
{
	return bSelectiveNaiveBayesPredictor;
}

inline void MDKhiopsModelingSpec::SetSelectiveNaiveBayesPredictor(boolean bValue)
{
	bSelectiveNaiveBayesPredictor = bValue;
}

inline boolean MDKhiopsModelingSpec::GetMAPNaiveBayesPredictor() const
{
	return bMAPNaiveBayesPredictor;
}

inline void MDKhiopsModelingSpec::SetMAPNaiveBayesPredictor(boolean bValue)
{
	bMAPNaiveBayesPredictor = bValue;
}

inline boolean MDKhiopsModelingSpec::GetNaiveBayesPredictor() const
{
	return bNaiveBayesPredictor;
}

inline void MDKhiopsModelingSpec::SetNaiveBayesPredictor(boolean bValue)
{
	bNaiveBayesPredictor = bValue;
}

inline boolean MDKhiopsModelingSpec::GetDataGridPredictor() const
{
	return bDataGridPredictor;
}

inline void MDKhiopsModelingSpec::SetDataGridPredictor(boolean bValue)
{
	bDataGridPredictor = bValue;
}

// ## Custom inlines

// ##
