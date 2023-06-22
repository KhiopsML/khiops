// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

TableGenerator::TableGenerator()
{
	attAttributeTable = NULL;
	qsAttributeRangServices = NULL;
	attFieldTable = NULL;
	qsFieldRangServices = NULL;
	bGenereManagement = false;
	bGenereView = true;
	bGenereArrayView = true;
	bGenereUserSection = true;
}

TableGenerator::~TableGenerator()
{
	if (attAttributeTable != NULL)
	{
		attAttributeTable->DeleteAll();
		delete attAttributeTable;
		check(attFieldTable);
		delete attFieldTable;
	}
}

const ALString& TableGenerator::GetClassName() const
{
	return sClassName;
}

void TableGenerator::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

const ALString& TableGenerator::GetSuperClassName() const
{
	return sSuperClassName;
}

void TableGenerator::SetSuperClassName(const ALString& sValue)
{
	sSuperClassName = sValue;
}

const ALString& TableGenerator::GetClassUserLabel() const
{
	return sClassUserLabel;
}

void TableGenerator::SetClassUserLabel(const ALString& sValue)
{
	sClassUserLabel = sValue;
}

void TableGenerator::SetGenereManagement(boolean bValue)
{
	bGenereManagement = bValue;
}

boolean TableGenerator::GetGenereManagement() const
{
	return bGenereManagement;
}

void TableGenerator::SetGenereView(boolean bValue)
{
	bGenereView = bValue;
	if (bGenereView == false)
	{
		bGenereArrayView = false;
	}
}

boolean TableGenerator::GetGenereView() const
{
	return bGenereView;
}

void TableGenerator::SetGenereArrayView(boolean bValue)
{
	bGenereArrayView = bValue;
	if (bGenereArrayView == true)
		bGenereView = true;
}

boolean TableGenerator::GetGenereArrayView() const
{
	return bGenereArrayView;
}

void TableGenerator::SetGenereUserSection(boolean bValue)
{
	bGenereUserSection = bValue;
}

boolean TableGenerator::GetGenereUserSection() const
{
	return bGenereUserSection;
}

int TableGenerator::GetFieldNumber() const
{
	require(qsFieldRangServices != NULL);

	return qsFieldRangServices->GetSize();
}

Attribute* TableGenerator::GetFieldAt(int i) const
{
	require(qsFieldRangServices != NULL);

	return cast(Attribute*, qsFieldRangServices->GetAt(i));
}

int TableGenerator::GetAttributeNumber() const
{
	require(qsAttributeRangServices != NULL);

	return qsAttributeRangServices->GetSize();
}

Attribute* TableGenerator::GetAttributeAt(int i) const
{
	require(qsAttributeRangServices != NULL);

	return cast(Attribute*, qsAttributeRangServices->GetAt(i));
}

AttributeTable* TableGenerator::GetAttributeTable() const
{
	require(attAttributeTable != NULL);

	return attAttributeTable;
}

void TableGenerator::SetAttributeTable(AttributeTable* attTable)
{
	int i;
	Attribute* att;

	require(attTable != NULL);

	// Nettoyage eventuel
	if (attAttributeTable != NULL)
	{
		attAttributeTable->DeleteAll();
		delete attAttributeTable;
		check(attFieldTable);
		delete attFieldTable;
	}

	// Initialisation de la table des attributs
	attAttributeTable = attTable;
	qsAttributeRangServices = attAttributeTable->RankServices();

	// Initialisation de la table des attributs de type Field
	attFieldTable = new AttributeTable;
	for (i = 0; i < attAttributeTable->GetSize(); i++)
	{
		att = cast(Attribute*, attAttributeTable->GetAt(i));
		if (att->IsField())
			attFieldTable->Insert(att);
	}
	qsFieldRangServices = attFieldTable->RankServices();
}

void TableGenerator::Error(const ALString& sMessage) const
{
	cout << "erreur: " << sMessage << endl;
}

const ALString TableGenerator::Backup(const ALString& sFileName, const ALString& sWhich) const
{
	const ALString sTmpDir = "C:\\temp\\";

	require(sWhich == "U" or sWhich == "G");
	return sTmpDir + sFileName + "." + sWhich;
}

void TableGenerator::ConsolidateFiles(const ALString& sFileName) const
{
	SectionTable stUser;
	SectionTable stGenerated;
	fstream fstUser;
	fstream fstBackup;
	fstream fstGenerated;
	fstream fstFinal;

	// Ouverture du fichier utilisateur et chargement de ses sections
	FileService::OpenInputFile(sFileName, fstUser);
	if (not fstUser.is_open())
		; // (s'il n'y a pas de fichier utilisateur, stUser est vide)
	else
	{
		// On le charge
		stUser.SetFileName(sFileName);
		stUser.Load(fstUser);
		if (not stUser.IsValid())
			Error("Les sections du fichier utilisateur sont incoherentes");
		FileService::CloseInputFile(sFileName, fstUser);

		// On en fait un backup
		FileService::RemoveFile(Backup(sFileName, "U"));
		FileService::OpenOutputFile(Backup(sFileName, "U"), fstBackup);
		if (not fstBackup.is_open())
		{
			Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "U"));
			return;
		}
		else
		{
			stUser.SetFileName(Backup(sFileName, "U"));
			stUser.Unload(fstBackup);
			FileService::CloseOutputFile(Backup(sFileName, "U"), fstBackup);
		}
	}

	// Ouverture du fichier de backup du genere et chargement de ses sections
	FileService::OpenInputFile(Backup(sFileName, "G"), fstGenerated);
	if (not fstGenerated.is_open())
	{
		Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
		return;
	}
	else
	{
		stGenerated.SetFileName(Backup(sFileName, "G"));
		stGenerated.Load(fstGenerated);
		if (not stGenerated.IsValid())
			Error("Les sections du fichier genere sont incoherentes");
		FileService::CloseInputFile(Backup(sFileName, "G"), fstGenerated);
	}

	// Consolidation
	if (not stUser.IsValid() or not stGenerated.IsValid())
	{
		Error("Arret a cause des erreurs");
		return;
	}
	stGenerated.SetFileName(sFileName);
	stGenerated.ImportSectionsFrom(&stUser);
	FileService::RemoveFile(sFileName);
	FileService::OpenOutputFile(sFileName, fstFinal);
	if (not fstFinal.is_open())
		Error("Impossible d'ouvrir le fichier " + sFileName);
	else
	{
		stGenerated.Unload(fstFinal);
		FileService::CloseOutputFile(sFileName, fstFinal);
	}
}

void TableGenerator::Genere() const
{
	ALString sFileName;
	fstream fstFile;

	// Generation du header de la classe
	sFileName = GetClassName() + ".h";
	FileService::RemoveFile(Backup(sFileName, "G"));
	FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
	if (not fstFile.is_open())
		Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
	else
	{
		GenerateAttributeH(fstFile);
		FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
		ConsolidateFiles(sFileName);
	}

	// Generation du source de la classe
	sFileName = GetClassName() + ".cpp";
	FileService::RemoveFile(Backup(sFileName, "G"));
	FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
	if (not fstFile.is_open())
		Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
	else
	{
		GenerateAttributeC(fstFile);
		FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
		ConsolidateFiles(sFileName);
	}

	if (GetGenereView())
	{
		// Generation du header de la vue fiche
		sFileName = GetClassName() + "View.h";
		FileService::RemoveFile(Backup(sFileName, "G"));
		FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
		else
		{
			GenerateAttributeViewH(fstFile);
			FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
			ConsolidateFiles(sFileName);
		}

		// Generation du source de la vue fiche
		sFileName = GetClassName() + "View.cpp";
		FileService::RemoveFile(Backup(sFileName, "G"));
		FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
		else
		{
			GenerateAttributeViewC(fstFile);
			FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
			ConsolidateFiles(sFileName);
		}
	}

	if (GetGenereArrayView())
	{
		// Generation du header de la vue liste
		sFileName = GetClassName() + "ArrayView.h";
		FileService::RemoveFile(Backup(sFileName, "G"));
		FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
		else
		{
			GenerateAttributeArrayViewH(fstFile);
			FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
			ConsolidateFiles(sFileName);
		}

		// Generation du source de la vue liste
		sFileName = GetClassName() + "ArrayView.cpp";
		FileService::RemoveFile(Backup(sFileName, "G"));
		FileService::OpenOutputFile(Backup(sFileName, "G"), fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + Backup(sFileName, "G"));
		else
		{
			GenerateAttributeArrayViewC(fstFile);
			FileService::CloseOutputFile(Backup(sFileName, "G"), fstFile);
			ConsolidateFiles(sFileName);
		}
	}
}

void TableGenerator::GenereWith(const ALString& sName, const ALString& sSuperName, const ALString& sLabel,
				const ALString& sAttributeFileName)
{
	IntVector ivMandatoryFieldIndexes;
	AttributeTable* attTable;
	int i;
	Attribute* att;
	boolean bOk;

	// Initialisation des attributs
	SetClassName(sName);
	SetSuperClassName(sSuperName);
	SetClassUserLabel(sLabel);
	attTable = new AttributeTable;

	// Specification des champs obligatoires
	ivMandatoryFieldIndexes.Add(Attribute::Rank);
	ivMandatoryFieldIndexes.Add(Attribute::Name);
	ivMandatoryFieldIndexes.Add(Attribute::Type);

	// Chargement et verification des donnees
	bOk = attTable->LoadFields(sAttributeFileName, &ivMandatoryFieldIndexes);
	for (i = 0; i < attTable->GetSize(); i++)
	{
		att = cast(Attribute*, attTable->GetAt(i));
		att->NormalizeValues();
	}
	bOk = bOk and attTable->Check();
	SetAttributeTable(attTable);

	// Verification de l'existence de doublons bases sur le rang
	check(qsAttributeRangServices);
	if (qsAttributeRangServices->CheckDoubles() == false)
	{
		bOk = false;
		GetAttributeTable()->AddError("Les attributs doivent avoir des rangs differents");
	}

	// Test de compatibilite de l'option de generation
	// des fonctionnalites de gestion
	if (attTable->GetPermanentFieldsNumber() > 0 and GetGenereManagement() == false)
	{
		GetAttributeTable()->AddWarning("stored attribute is deprecated");
	}

	// Generation
	if (bOk)
	{
		// Calcul de valeurs par defaut pour les attributs
		// non renseignes
		for (i = 0; i < attTable->GetSize(); i++)
		{
			att = cast(Attribute*, attTable->GetAt(i));
			att->ComputeDefaultValues();
		}

		// Generation
		Genere();
	}
}

void TableGenerator::GenerateFileHeader(ostream& ost) const
{
	ALString sCommentLign('/', 60);

	if (not GetGenereUserSection())
		return;

	ost << sCommentLign << "\n";
	ost << "// " << CurrentTimestamp() << "\n";
	ost << "// File generated  with GenereTable"
	    << "\n";
	ost << "// Insert your specific code inside \"//## \" sections"
	    << "\n";
}

void TableGenerator::GenereClassHeaderComment(ostream& ost, const ALString& sClassFamily) const
{
	ALString sCommentLign('/', 60);

	ost << sCommentLign << "\n";
	ost << "// Classe " << GetClassName() << sClassFamily << "\n";
	ost << "//    " << GetClassUserLabel() << "\n";
}

void TableGenerator::GenereTitledComment(ostream& ost, const ALString& sIndent, const ALString& sComment) const
{
	ALString sCommentLign('/', 60 - sIndent.GetLength());

	ost << sIndent << sCommentLign << "\n";
	if (sComment != "")
		ost << sIndent << "// " << sComment << "\n";
}

void TableGenerator::GenereImplementationComment(ostream& ost) const
{
	ALString sCommentLign('/', 56);

	ost << "\t" << sCommentLign << "\n";
	ost << "\t//// Implementation"
	    << "\n";
}

void TableGenerator::GenerateUserCodeSection(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "//## " << sIdentifier << "\n";
	ost << ""
	    << "\n";
	ost << sIndent << "//##"
	    << "\n";
}

void TableGenerator::GenerateUserCodeHeader(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "//## " << sIdentifier << "\n";
}

void TableGenerator::GenerateUserCodeTrailer(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "//##"
	    << "\n";
}

void TableGenerator::GenereDoc(ostream& ost) const
{
	int nCurrent;
	Attribute* att;

	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		ost << att->GetType() << "\t" << att->GetName() << "\t" << att->GetStorageName() << "\t"
		    << att->GetLabel() << "\n";
	}
}