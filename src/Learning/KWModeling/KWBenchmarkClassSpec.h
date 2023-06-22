// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClassDomain.h"

////////////////////////////////////////////////////////////
// Classe KWBenchmarkClassSpec
//    Benchmark dictionary
class KWBenchmarkClassSpec : public Object
{
public:
	// Constructeur
	KWBenchmarkClassSpec();
	~KWBenchmarkClassSpec();

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Dictionary file
	const ALString& GetClassFileName() const;
	void SetClassFileName(const ALString& sValue);

	// Dictionary
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Target variable
	const ALString& GetTargetAttributeName() const;
	void SetTargetAttributeName(const ALString& sValue);

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Gestion des classes dans le domaine de classe temporaires
	// Utile notamment pour l'edition d'un mapping dans le cas multi-tables
	// La lecture des classe se fait en mode silencieux, car elle se fait potentiellement souvent
	// Elle est bufferisee par le nom du fichier de classe, et n'est effectuee qu'en cas de changement
	void ReadClasses();
	void DropClasses();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sClassFileName;
	ALString sClassName;
	ALString sTargetAttributeName;

	// Gestion des domaines de classes necessaire a l'edition des spec de benchmark
	KWClassDomain* currentClassDomain;
	KWClassDomain* temporaryClassDomain;

	// Fichier du dernier domaine de classes lu
	ALString sLastReadClassFileName;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWBenchmarkClassSpec::GetClassFileName() const
{
	return sClassFileName;
}

inline void KWBenchmarkClassSpec::SetClassFileName(const ALString& sValue)
{
	sClassFileName = sValue;
}

inline const ALString& KWBenchmarkClassSpec::GetClassName() const
{
	return sClassName;
}

inline void KWBenchmarkClassSpec::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline const ALString& KWBenchmarkClassSpec::GetTargetAttributeName() const
{
	return sTargetAttributeName;
}

inline void KWBenchmarkClassSpec::SetTargetAttributeName(const ALString& sValue)
{
	sTargetAttributeName = sValue;
}
