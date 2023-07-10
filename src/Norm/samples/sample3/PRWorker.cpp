// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRWorker.h"

PRWorker::PRWorker()
{
	// ## Custom constructor

	// Valeur initiale des attributs
	sReportFileName = "rapport.xls";

	// ##
}

PRWorker::~PRWorker()
{
	// ## Custom destructor

	// Destruction des sous-objets du tableau
	oaChildren.DeleteAll();

	// ##
}

void PRWorker::CopyFrom(const PRWorker* aSource)
{
	require(aSource != NULL);

	sFirstName = aSource->sFirstName;
	sFamilyName = aSource->sFamilyName;
	sReportFileName = aSource->sReportFileName;

	// ## Custom copyfrom

	// ##
}

PRWorker* PRWorker::Clone() const
{
	PRWorker* aClone;

	aClone = new PRWorker;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void PRWorker::Write(ostream& ost) const
{
	ost << "Prenom\t" << GetFirstName() << "\n";
	ost << "Nom\t" << GetFamilyName() << "\n";
	ost << "Rapport\t" << GetReportFileName() << "\n";
}

const ALString PRWorker::GetClassLabel() const
{
	return "Employe";
}

// ## Method implementation

const ALString PRWorker::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

ObjectArray* PRWorker::GetChildren()
{
	return &oaChildren;
}

PRAddress* PRWorker::GetProfessionalAddress()
{
	return &professionalAddress;
}

PRAddress* PRWorker::GetPersonalAddress()
{
	return &personalAddress;
}

void PRWorker::WriteReport()
{
	fstream ost;
	boolean bOk;
	ObjectArray oaSortedChildren;
	PRChild* child;
	int nChild;

	// Ecriture du rapport
	bOk = FileService::OpenOutputFile(GetReportFileName(), ost);
	if (bOk)
	{
		// Ecriture des informations principales
		ost << GetClassLabel() << "\n";
		ost << "Prenom\t" << GetFirstName() << "\n";
		ost << "Nom\t" << GetFamilyName() << "\n";

		// Ecriture des informations sur les enfants
		if (GetChildren()->GetSize() > 0)
		{
			// Tri des enfants par age
			oaSortedChildren.CopyFrom(GetChildren());
			oaSortedChildren.SetCompareFunction(PRChildCompareAge);
			oaSortedChildren.Sort();

			// Ecriture dans le rapport
			ost << "\n";
			ost << "Enfants\n";
			ost << "Nom\tAge\n";
			for (nChild = 0; nChild < oaSortedChildren.GetSize(); nChild++)
			{
				child = cast(PRChild*, oaSortedChildren.GetAt(nChild));
				ost << child->GetFirstName() << "\t" << child->GetAge() << "\n";
			}
		}

		// Ecriture des informations sur les adresses
		// (par appel implcite aux methodes Write(ostream& ost) des objets utilises)
		ost << "\n";
		ost << "Adresse personelle\n";
		ost << *GetPersonalAddress() << "\n";
		ost << "\n";
		ost << "Adresse professionnelle\n";
		ost << *GetProfessionalAddress() << "\n";

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(GetReportFileName(), ost);
	}
}

// ##
