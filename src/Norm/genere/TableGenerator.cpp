// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

TableGenerator::TableGenerator()
{
	attAttributeTable = NULL;
	qsAttributeRangServices = NULL;
	attFieldTable = NULL;
	qsFieldRangServices = NULL;
	bGenereModel = true;
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

const ALString& TableGenerator::GetSpecificModelClassName() const
{
	return sSpecificModelClassName;
}

void TableGenerator::SetSpecificModelClassName(const ALString& sValue)
{
	sSpecificModelClassName = sValue;
}

const ALString& TableGenerator::GetClassUserLabel() const
{
	return sClassUserLabel;
}

void TableGenerator::SetClassUserLabel(const ALString& sValue)
{
	sClassUserLabel = sValue;
}

const ALString TableGenerator::GetModelClassName() const
{
	if (GetSpecificModelClassName() != "")
		return GetSpecificModelClassName();
	else
		return GetClassName();
}

const ALString TableGenerator::GetViewClassName() const
{
	return GetClassName() + "View";
}

const ALString TableGenerator::GetArrayViewClassName() const
{
	return GetClassName() + "ArrayView";
}

const ALString TableGenerator::GetModelSuperClassName() const
{
	if (GetSuperClassName() == "")
		return "Object";
	else
		return GetSuperClassName();
}

const ALString TableGenerator::GetViewSuperClassName() const
{
	if (GetSuperClassName() == "")
		return "UIObjectView";
	else
		return GetSuperClassName() + "View";
}

const ALString TableGenerator::GetArrayViewSuperClassName() const
{
	if (GetSuperClassName() == "")
		return "UIObjectArrayView";
	else
		return GetSuperClassName() + "ArrayView";
}

void TableGenerator::SetGenereModel(boolean bValue)
{
	bGenereModel = bValue;
}

boolean TableGenerator::GetGenereModel() const
{
	return bGenereModel;
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

void TableGenerator::SetOutputDir(const ALString& sValue)
{
	sOutputDir = sValue;
}

const ALString& TableGenerator::GetOutputDir() const
{
	return sOutputDir;
}

boolean TableGenerator::CheckClassName(const ALString& sValue) const
{
	boolean bOk = true;
	int i;
	char c;

	require(sClassName != "");

	for (i = 0; i < sValue.GetLength(); i++)
	{
		c = sValue.GetAt(i);
		if (i == 0 and isdigit(c))
		{
			bOk = false;
			Error("Class name <" + sValue + "> must not start with a digit");
			break;
		}
		if (not isalnum(c) and c != '_')
		{
			bOk = false;
			Error("Class name <" + sValue + "> must contain alphanumeric chracters only");
			break;
		}
	}
	return bOk;
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
	cout << "error: " << sMessage << endl;
}

const ALString TableGenerator::BuildBackupFileName(const ALString& sFileName, const ALString& sWhich) const
{
	require(FileService::GetPathName(sFileName) == "");
	require(sWhich == "U" or sWhich == "G");
	return FileService::BuildFilePathName(
	    FileService::GetTmpDir(), FileService::BuildFileName(FileService::GetFilePrefix(sFileName),
								 sWhich + "." + FileService::GetFileSuffix(sFileName)));
}

void TableGenerator::ConsolidateFiles(const ALString& sFileName) const
{
	SectionTable stUser;
	SectionTable stGenerated;
	fstream fstUser;
	fstream fstBackup;
	fstream fstGenerated;
	fstream fstFinal;
	ALString sInitialFilePathName;
	ALString sGeneratedFileName;

	// Nom du fichier genere
	sGeneratedFileName = BuildBackupFileName(sFileName, "G");

	// Path complet du fichier initial
	sInitialFilePathName = FileService::BuildFilePathName(GetOutputDir(), sFileName);

	// Ouverture du fichier initial et chargement de ses sections
	if (FileService::FileExists(sInitialFilePathName))
	{
		FileService::OpenInputFile(sInitialFilePathName, fstUser);
		if (fstUser.is_open())
		{
			// On charge les sections
			stUser.SetFileName(sInitialFilePathName);
			stUser.Load(fstUser);
			if (not stUser.IsValid())
				Error("Les sections du fichier utilisateur sont incoherentes");
			FileService::CloseInputFile(sInitialFilePathName, fstUser);
		}
	}

	// Ouverture du fichier de backup du genere et chargement de ses sections
	FileService::OpenInputFile(sGeneratedFileName, fstGenerated);
	if (not fstGenerated.is_open())
	{
		Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		return;
	}
	else
	{
		stGenerated.SetFileName(sGeneratedFileName);
		stGenerated.Load(fstGenerated);
		if (not stGenerated.IsValid())
			Error("Les sections du fichier genere sont incoherentes");
		FileService::CloseInputFile(sGeneratedFileName, fstGenerated);
	}

	// Consolidation
	if (not stUser.IsValid() or not stGenerated.IsValid())
	{
		Error("Arret a cause des erreurs");
		return;
	}
	stGenerated.SetFileName(sInitialFilePathName);
	stGenerated.ImportSectionsFrom(&stUser);
	FileService::OpenOutputFile(sInitialFilePathName, fstFinal);
	if (not fstFinal.is_open())
		Error("Impossible d'ouvrir le fichier " + sInitialFilePathName);
	else
	{
		stGenerated.Unload(fstFinal);
		FileService::CloseOutputFile(sInitialFilePathName, fstFinal);
	}

	// Nettoyage des fichiers intermediaires
	FileService::RemoveFile(sGeneratedFileName);
}

void TableGenerator::Genere() const
{
	ALString sFileName;
	ALString sGeneratedFileName;
	fstream fstFile;

	require(GetClassName() != "" and CheckClassName(GetClassName()));
	require(GetSuperClassName() == "" or CheckClassName(GetSuperClassName()));

	// Generation de la classe <ClassName>
	if (GetGenereModel())
	{
		// Generation du header de la classe
		sFileName = GetClassName() + ".h";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeH(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}

		// Generation du source de la classe
		sFileName = GetClassName() + ".cpp";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeC(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}
	}

	// Generation de la classe <ClassName>View
	if (GetGenereView())
	{
		// Generation du header de la vue fiche
		sFileName = GetClassName() + "View.h";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeViewH(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}

		// Generation du source de la vue fiche
		sFileName = GetClassName() + "View.cpp";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeViewC(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}
	}

	// Generation de la classe <ClassName>ArrayView
	if (GetGenereArrayView())
	{
		// Generation du header de la vue liste
		sFileName = GetClassName() + "ArrayView.h";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeArrayViewH(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}

		// Generation du source de la vue liste
		sFileName = GetClassName() + "ArrayView.cpp";
		sGeneratedFileName = BuildBackupFileName(sFileName, "G");
		FileService::RemoveFile(sGeneratedFileName);
		FileService::OpenOutputFile(sGeneratedFileName, fstFile);
		if (not fstFile.is_open())
			Error("Impossible d'ouvrir le fichier " + sGeneratedFileName);
		else
		{
			GenerateAttributeArrayViewC(fstFile);
			FileService::CloseOutputFile(sGeneratedFileName, fstFile);
			ConsolidateFiles(sFileName);
		}
	}
}

void TableGenerator::GenereWith(const ALString& sName, const ALString& sSpecificModelName, const ALString& sSuperName,
				const ALString& sLabel, const ALString& sAttributeFileName)
{
	IntVector ivMandatoryFieldIndexes;
	AttributeTable* attTable;
	int i;
	Attribute* att;
	boolean bOk;

	// Initialisation des attributs
	SetClassName(sName);
	SetSpecificModelClassName(sSpecificModelName);
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
		GetAttributeTable()->AddError("The attributes must have distinct ranks");
	}

	// Verification de la syntaxe des nom de classe
	bOk = bOk and CheckClassName(GetClassName());
	bOk = bOk and CheckClassName(GetSpecificModelClassName());
	bOk = bOk and CheckClassName(GetSuperClassName());
	if (bOk and GetSuperClassName() != "" and GetClassName() == GetSuperClassName())
	{
		bOk = false;
		Error("Class name <" + GetClassName() + "> must be different from super class name <" +
		      GetSuperClassName() + ">");
	}
	if (bOk and GetSuperClassName() != "" and GetSpecificModelClassName() != "" and
	    GetSpecificModelClassName() == GetSuperClassName())
	{
		bOk = false;
		Error("Specific model class name <" + GetSpecificModelClassName() +
		      "> must be different from super class name <" + GetSuperClassName() + ">");
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

void TableGenerator::GenerateCopyrightHeader(ostream& ost) const
{
	time_t lCurrentTime;
	struct tm* dateCurrent;
	int nCurrentYear;

	// Recherche de l'annee dans la date courante
	time(&lCurrentTime);
	dateCurrent = p_localtime(&lCurrentTime);
	nCurrentYear = dateCurrent->tm_year + 1900;

	// Notice de copyright
	ost << "// Copyright (c) 2023-" << nCurrentYear << " Orange. All rights reserved.\n ";
	ost << "// This software is distributed under the BSD 3-Clause-clear License, the text of which is available\n";
	ost << "// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the \"LICENSE\" file for more "
	       "details.\n";
	ost << "\n";
}

void TableGenerator::GenerateFileHeader(ostream& ost) const
{
	ALString sCommentLign('/', 60);

	if (not GetGenereUserSection())
		return;

	ost << sCommentLign << "\n";
	ost << "// File generated with Genere tool"
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
	ost << "\t///// Implementation"
	    << "\n";
}

void TableGenerator::GenerateUserCodeSection(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "// ## " << sIdentifier << "\n";
	ost << ""
	    << "\n";
	ost << sIndent << "// ##"
	    << "\n";
}

void TableGenerator::GenerateUserCodeHeader(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "// ## " << sIdentifier << "\n";
}

void TableGenerator::GenerateUserCodeTrailer(ostream& ost, const ALString& sIndent, const ALString& sIdentifier,
					     boolean bNewLine) const
{
	if (not GetGenereUserSection())
		return;

	ost << sIndent << "// ##";
	if (bNewLine)
		ost << "\n";
}
