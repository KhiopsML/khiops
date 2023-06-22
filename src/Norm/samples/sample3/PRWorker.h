// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// 2019-10-29 15:14:06
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "FileService.h"
#include "PRChild.h"
#include "PRAddress.h"

// ##

////////////////////////////////////////////////////////////
// Classe PRWorker
//    Employe
class PRWorker : public Object
{
public:
	// Constructeur
	PRWorker();
	~PRWorker();

	// Copie et duplication
	void CopyFrom(const PRWorker* aSource);
	PRWorker* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Prenom
	const ALString& GetFirstName() const;
	void SetFirstName(const ALString& sValue);

	// Nom
	const ALString& GetFamilyName() const;
	void SetFamilyName(const ALString& sValue);

	// Rapport
	const ALString& GetReportFileName() const;
	void SetReportFileName(const ALString& sValue);

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Tableau des enfants
	// Memoire: les objets du tableau appartiennent a l'appele
	ObjectArray* GetChildren();

	// Adresse professionnelle
	PRAddress* GetProfessionalAddress();

	// Adresse personnelle
	PRAddress* GetPersonalAddress();

	// Ecriture d'un rapport
	void WriteReport();

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sFirstName;
	ALString sFamilyName;
	ALString sReportFileName;

	// ## Custom implementation

	// Sous-objets
	ObjectArray oaChildren;
	PRAddress professionalAddress;
	PRAddress personalAddress;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& PRWorker::GetFirstName() const
{
	return sFirstName;
}

inline void PRWorker::SetFirstName(const ALString& sValue)
{
	sFirstName = sValue;
}

inline const ALString& PRWorker::GetFamilyName() const
{
	return sFamilyName;
}

inline void PRWorker::SetFamilyName(const ALString& sValue)
{
	sFamilyName = sValue;
}

inline const ALString& PRWorker::GetReportFileName() const
{
	return sReportFileName;
}

inline void PRWorker::SetReportFileName(const ALString& sValue)
{
	sReportFileName = sValue;
}

// ## Custom inlines

// ##
