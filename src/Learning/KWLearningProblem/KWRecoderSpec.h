// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWRecodingSpec.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWRecoderSpec
//    Recoders
class KWRecoderSpec : public Object
{
public:
	// Constructeur
	KWRecoderSpec();
	~KWRecoderSpec();

	// Copie et duplication
	void CopyFrom(const KWRecoderSpec* aSource);
	KWRecoderSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Build recoder
	boolean GetRecoder() const;
	void SetRecoder(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Parametres de recodage
	KWRecodingSpec* GetRecodingSpec();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bRecoder;

	// ## Custom implementation

	// Parametres de recodage
	KWRecodingSpec recodingSpec;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWRecoderSpec::GetRecoder() const
{
	return bRecoder;
}

inline void KWRecoderSpec::SetRecoder(boolean bValue)
{
	bRecoder = bValue;
}

// ## Custom inlines

// ##
