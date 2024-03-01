// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWResultFilePathBuilder.h"

KWResultFilePathBuilder::KWResultFilePathBuilder() {}

KWResultFilePathBuilder::~KWResultFilePathBuilder() {}

void KWResultFilePathBuilder::SetInputFilePathName(const ALString& sValue)
{
	sInputFilePathName = sValue;
}

const ALString& KWResultFilePathBuilder::GetInputFilePathName() const
{
	return sInputFilePathName;
}

void KWResultFilePathBuilder::SetOutputFilePathName(const ALString& sValue)
{
	sOutputFilePathName = sValue;
}

const ALString& KWResultFilePathBuilder::GetOutputFilePathName() const
{
	return sOutputFilePathName;
}

void KWResultFilePathBuilder::SetFileSuffix(const ALString& sValue)
{
	require(sValue == "" or CheckSuffix(sValue));

	sFileSuffix = sValue;
}

const ALString& KWResultFilePathBuilder::GetFileSuffix() const
{
	return sFileSuffix;
}

void KWResultFilePathBuilder::SetForceSuffix(boolean bValue)
{
	bForceSuffix = bValue;
}

boolean KWResultFilePathBuilder::GetForceSuffix()
{
	return bForceSuffix;
}

void KWResultFilePathBuilder::SetLearningApiMode(boolean bValue)
{
	bIsLearningApiModeInitialized = true;
	bLearningApiMode = bValue;
}

boolean KWResultFilePathBuilder::GetLearningApiMode()
{
	// Determination du mode au premier appel
	if (not bIsLearningApiModeInitialized)
	{
		ALString sLearningApiMode;

		// Recherche des variables d'environnement
		sLearningApiMode = p_getenv("KHIOPS_API_MODE");
		sLearningApiMode.MakeLower();

		// Determination du mode
		if (sLearningApiMode == "true")
			bLearningApiMode = true;
		else if (sLearningApiMode == "false")
			bLearningApiMode = false;

		// Memorisation du flag d'initialisation
		bIsLearningApiModeInitialized = true;
	}
	return bLearningApiMode;
}

boolean KWResultFilePathBuilder::CheckResultDirectory(const ALString& sErrorCategory) const
{
	boolean bOk = true;
	ALString sResultPathName;

	require(GetOutputFilePathName() != "");

	// Verification uniquement s'il y a un path dans le fichier en sortie
	if (FileService::GetPathName(GetOutputFilePathName()) != "")
	{
		// Construction du nom de chemin en sortie
		sResultPathName = BuildResultDirectoryPathName();
		bOk = CheckResultDirectory(sResultPathName, sErrorCategory);
	}
	return bOk;
}

const ALString KWResultFilePathBuilder::BuildResultFilePathName() const
{
	ALString sResultPathName;
	ALString sResultFilePathName;
	ALString sOutputSuffix;
	ALString sLowerOutputSuffix;

	require(GetOutputFilePathName() != "");

	// Mode api
	if (GetLearningApiMode())
		sResultFilePathName = GetOutputFilePathName();
	// Mode standard
	else
	{
		// Calcul du repertoire effectif des resultats
		sResultPathName = BuildResultDirectoryPathName();

		// On construit le nom complet du fichier
		sResultFilePathName =
		    FileService::BuildFilePathName(sResultPathName, FileService::GetFileName(GetOutputFilePathName()));

		// Traitement du suffixe si necessaire
		if (GetFileSuffix() != "")
			sResultFilePathName = UpdateFileSuffix(sResultFilePathName, GetFileSuffix());
	}
	return sResultFilePathName;
}

const ALString KWResultFilePathBuilder::BuildOtherResultFilePathName(const ALString sOtherSuffix) const
{
	ALString sResultFilePathName;
	ALString sResultSuffix;
	ALString sLowerResultSuffix;

	require(sOtherSuffix != "");
	require(sOtherSuffix.GetAt(0) != '.');
	require(FileService::GetPathName(sOtherSuffix) == "");
	require(GetOutputFilePathName() != "");

	// Recherche du chemin du fichier resultat en sortie
	sResultFilePathName = BuildResultFilePathName();

	// Remplacement ou concatenation de son suffixe
	sResultSuffix = FileService::GetFileSuffix(sResultFilePathName);
	sLowerResultSuffix = sResultSuffix;
	sLowerResultSuffix.MakeLower();
	if (sResultSuffix == "")
		sResultSuffix = sOtherSuffix;
	else if (GetFileSuffix() != "" and sLowerResultSuffix == GetFileSuffix())
		sResultSuffix = sOtherSuffix;
	else
		sResultSuffix = sResultSuffix + '.' + sOtherSuffix;
	sResultFilePathName = FileService::SetFileSuffix(sResultFilePathName, sResultSuffix);
	return sResultFilePathName;
}

const ALString KWResultFilePathBuilder::BuildResultDirectoryPathName() const
{
	ALString sInputPathName;
	ALString sOutputPathName;
	ALString sResultPathName;

	require(GetOutputFilePathName() != "");

	// Acces aux paths des fichier en parametre
	sInputPathName = FileService::GetPathName(GetInputFilePathName());
	sOutputPathName = FileService::GetPathName(GetOutputFilePathName());

	// Mode api
	if (GetLearningApiMode())
		sResultPathName = sOutputPathName;
	// Mode standard
	else
	{
		// Si le repertoire en sortie n'est pas specifie, on utilise celui en entree
		if (sOutputPathName == "")
			sResultPathName = sInputPathName;
		// S'il est absolu ou si c'est une URI, on le prend tel quel
		else if (FileService::IsAbsoluteFilePathName(sOutputPathName) or
			 (FileService::GetURIScheme(sOutputPathName) != ""))
			sResultPathName = sOutputPathName;
		// s'il est relatif, on le concatene a celui de la base d'apprentissage
		else
			sResultPathName = FileService::BuildFilePathName(sInputPathName, sOutputPathName);
	}
	return sResultPathName;
}

boolean KWResultFilePathBuilder::CheckFileSuffix(const ALString& sFilePathName, const ALString& sSuffix,
						 const ALString& sErrorCategory)
{
	boolean bOk = true;
	ALString sOutputSuffix;
	ALString sLowerOutputSuffix;

	// Verification de l'extension en mode standard
	if (bOk and not GetLearningApiMode())
	{
		// Verification si necessaire du suffixe
		if (sSuffix != "" and not GetForceSuffix())
		{
			// Recherche du suffixe en entree
			sOutputSuffix = FileService::GetFileSuffix(sFilePathName);
			sLowerOutputSuffix = sOutputSuffix;
			sLowerOutputSuffix.MakeLower();

			// Test de validite du suffix
			bOk = sLowerOutputSuffix == sSuffix;
			if (not bOk)
			{
				if (sLowerOutputSuffix == "")
					Global::AddError(sErrorCategory, "",
							 "Extension ." + sSuffix + " missing in result file " +
							     sFilePathName);
				else
					Global::AddError(sErrorCategory, "",
							 "Extension ." + sSuffix + " expected in result file " +
							     sFilePathName);
			}
		}
	}
	return bOk;
}

boolean KWResultFilePathBuilder::CheckResultDirectory(const ALString& sPathName, const ALString& sErrorCategory)
{
	boolean bOk = true;

	// Verification du chemin et tentative de construction si necessaire
	if (sPathName != "" and not PLRemoteFileService::DirExists(sPathName))
	{
		bOk = PLRemoteFileService::MakeDirectories(sPathName);
		if (not bOk)
			Global::AddError(sErrorCategory, "", "Unable to create result directory (" + sPathName + ")");
	}
	return bOk;
}

const ALString KWResultFilePathBuilder::UpdateFileSuffix(const ALString& sFilePathName, const ALString& sSuffix)
{
	ALString sOutputSuffix;
	ALString sLowerOutputSuffix;
	ALString sResultFilePathName;

	require(sFilePathName != "");
	require(sSuffix != "" and CheckSuffix(sSuffix));

	// Resultat par defaut
	sResultFilePathName = sFilePathName;

	// Modification potentielle en mode standard
	if (not GetLearningApiMode())
	{
		// Recherche du suffixe en entree
		sOutputSuffix = FileService::GetFileSuffix(sFilePathName);
		sLowerOutputSuffix = sOutputSuffix;
		sLowerOutputSuffix.MakeLower();

		// Normalisation du suffixe si necessaire
		if (sLowerOutputSuffix == sSuffix)
		{
			if (sLowerOutputSuffix != sOutputSuffix)
				sResultFilePathName = FileService::SetFileSuffix(sFilePathName, sSuffix);
		}
		// On force le suffixe si necessaire
		else if (GetForceSuffix())
		{
			sResultFilePathName = sFilePathName + '.' + sSuffix;
		}
	}
	return sResultFilePathName;
}

const ALString KWResultFilePathBuilder::GetClassLabel() const
{
	return "Result file class builder";
}

void KWResultFilePathBuilder::Test()
{
	KWResultFilePathBuilder testFilePathBuilder;
	StringVector svTestOutputDirectoryNames;
	StringVector svTestOutputFileNames;
	ALString sTestOutputFilePathName;
	int nApiMode;
	int nFileSuffix;
	int nForceSuffix;
	int nDirectory;
	int nFile;
	int nSourceURI;

	// Initialisation de chemins en sortie
	svTestOutputDirectoryNames.Add("");
	svTestOutputDirectoryNames.Add("C:/");
	svTestOutputDirectoryNames.Add(".");
	svTestOutputDirectoryNames.Add("..");
	svTestOutputDirectoryNames.Add("Dir1");
	svTestOutputDirectoryNames.Add(FileService::BuildFilePathName("Dir1", "Dir2"));
	svTestOutputDirectoryNames.Add(FileService::GetTmpDir());
	svTestOutputDirectoryNames.Add("hdfs://hector/dir1/dir2");

	// Initialisation de fichiers en sortie
	svTestOutputFileNames.Add("test.txt");
	svTestOutputFileNames.Add("test.TXT");
	svTestOutputFileNames.Add(".txt");
	svTestOutputFileNames.Add("test.dat");
	svTestOutputFileNames.Add("test");

	// Initialisation du path builder
	testFilePathBuilder.SetInputFilePathName("/data/sample/MyData.txt");

	// Test de la gestion du suffixe
	cout << "\n" << testFilePathBuilder.GetClassLabel() << ": test suffix management\n";
	cout << "Mode\tSuffix\tEnforce\tOutput file path\tResult file path\tOther result file path\n";
	bIsLearningApiModeInitialized = true;
	for (nApiMode = 0; nApiMode <= 1; nApiMode++)
	{
		bLearningApiMode = nApiMode == 1;

		// Boucle sur la presence d'une suffixe obligatoire
		for (nFileSuffix = 0; nFileSuffix <= 1; nFileSuffix++)
		{
			if (nFileSuffix == 0)
				testFilePathBuilder.SetFileSuffix("");
			else
				testFilePathBuilder.SetFileSuffix("txt");

			// Boucle sur l'option de gestion des suffixee
			for (nForceSuffix = 0; nForceSuffix <= 1; nForceSuffix++)
			{
				testFilePathBuilder.SetForceSuffix(nForceSuffix == 1);

				// Boucle sur tous les fichiers possibles en sortie
				for (nFile = 0; nFile < svTestOutputFileNames.GetSize(); nFile++)
				{
					testFilePathBuilder.SetOutputFilePathName(svTestOutputFileNames.GetAt(nFile));

					// Affichage des resultats
					if (bLearningApiMode)
						cout << "API\t";
					else
						cout << "Std\t";
					cout << testFilePathBuilder.GetFileSuffix() << "\t";
					cout << BooleanToString(testFilePathBuilder.GetForceSuffix()) << "\t";
					cout << FileService::GetPortableTmpFilePathName(
						    testFilePathBuilder.GetOutputFilePathName())
					     << "\t";
					cout << FileService::GetPortableTmpFilePathName(
						    testFilePathBuilder.BuildResultFilePathName())
					     << "\t";
					cout << FileService::GetPortableTmpFilePathName(
						    testFilePathBuilder.BuildOtherResultFilePathName("model.kdic"))
					     << "\n";
				}
			}
		}
	}

	// Test de la gestion de fabrication du chemin
	for (nSourceURI = 0; nSourceURI <= 1; nSourceURI++)
	{
		if (nSourceURI == 0)
			testFilePathBuilder.SetInputFilePathName("/data/sample/MyData.txt");
		else
			testFilePathBuilder.SetInputFilePathName("hdfs://data/sample/MyData.txt");

		cout << "\n"
		     << testFilePathBuilder.GetClassLabel() << ": test output path  management with input path "
		     << testFilePathBuilder.GetInputFilePathName() << "\n";
		cout << "Mode\tOutput file path\tResult file path\n";
		bIsLearningApiModeInitialized = true;
		for (nApiMode = 0; nApiMode <= 1; nApiMode++)
		{
			bLearningApiMode = nApiMode == 1;

			// Boucle sur l'option de gestion des suffixee
			for (nForceSuffix = 0; nForceSuffix <= 1; nForceSuffix++)
			{
				testFilePathBuilder.SetForceSuffix(nForceSuffix == 1);

				// Boucle sur tous les chemins de fichiers possible en sortie
				for (nDirectory = 0; nDirectory < svTestOutputDirectoryNames.GetSize(); nDirectory++)
				{
					sTestOutputFilePathName =
					    FileService::BuildFilePathName(svTestOutputDirectoryNames.GetAt(nDirectory),
									   svTestOutputFileNames.GetAt(0));
					testFilePathBuilder.SetOutputFilePathName(sTestOutputFilePathName);

					// Affichage des resultats
					if (bLearningApiMode)
						cout << "API\t";
					else
						cout << "Std\t";
					cout << FileService::GetPortableTmpFilePathName(
						    testFilePathBuilder.GetOutputFilePathName())
					     << "\t";
					cout << FileService::GetPortableTmpFilePathName(
						    testFilePathBuilder.BuildResultFilePathName())
					     << "\n";
				}
			}
		}
	}

	// Nettoyage
	bIsLearningApiModeInitialized = false;
	bLearningApiMode = false;
	testFilePathBuilder.SetForceSuffix(false);
}

boolean KWResultFilePathBuilder::CheckSuffix(const ALString& sSuffix)
{
	boolean bOk = true;
	ALString sLowerSuffix;

	require(sSuffix != "");

	sLowerSuffix = sSuffix;
	sLowerSuffix.MakeLower();
	bOk = bOk and sLowerSuffix == sSuffix;
	bOk = bOk and sSuffix.FindOneOf(" ./\\") == -1;
	return bOk;
}

boolean KWResultFilePathBuilder::bForceSuffix = true;
boolean KWResultFilePathBuilder::bIsLearningApiModeInitialized = false;
boolean KWResultFilePathBuilder::bLearningApiMode = false;
