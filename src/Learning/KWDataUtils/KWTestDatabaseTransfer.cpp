// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTestDatabaseTransfer.h"

KWClass* KWTestDatabaseTransfer::ComputeArtificialClass(KWArtificialDataset* artificialDataset)
{
	KWClass* kwcArtificialClass;
	StringVector svNativeFieldNames;
	StringVector svKeyAttributedNames;
	int i;
	KWAttribute* attribute;

	require(artificialDataset != NULL);

	// Creation d'une classe
	kwcArtificialClass = new KWClass;
	kwcArtificialClass->SetName(FileService::GetFilePrefix(artificialDataset->GetFileName()));

	// Extraction des informations sur les champs
	artificialDataset->ExportNativeFieldNames(&svNativeFieldNames);
	artificialDataset->ExportKeyAttributeNames(&svKeyAttributedNames);

	// Initialisation des attributs
	for (i = 0; i < svNativeFieldNames.GetSize(); i++)
	{
		attribute = new KWAttribute;
		attribute->SetType(KWType::Symbol);
		attribute->SetName(svNativeFieldNames.GetAt(i));
		kwcArtificialClass->InsertAttribute(attribute);
	}

	// Initialisation de la cle
	kwcArtificialClass->SetKeyAttributeNumber(svKeyAttributedNames.GetSize());
	for (i = 0; i < svKeyAttributedNames.GetSize(); i++)
		kwcArtificialClass->SetKeyAttributeNameAt(i, svKeyAttributedNames.GetAt(i));

	// Retour
	return kwcArtificialClass;
}

void KWTestDatabaseTransfer::Test()
{
	STTest();
	MTTest();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ST

void KWTestDatabaseTransfer::STTest()
{
	boolean bOk;
	KWDatabaseTransferTask databaseTransfer;
	longint lWrittenRecordNumber;
	ALString sClassNamePrefix;
	ALString sFileName;
	ALString sReferenceFilePathName;
	ALString sTransferredFilePathName;
	ALString sClassName;
	KWSTDatabaseTextFile databaseSource;
	KWSTDatabaseTextFile databaseTarget;
	KWClass* kwcCreationClass;
	int nContinuousNumber;
	int nSymbolNumber;
	int nLinesNumber;
	ALString sTmpDir;
	ALString sSuffix;
	ALString sTmp;

	/////////////////////////////////////////////////////////////////////
	// Creation du dictionnaire et de la base initiale

	// Nommage des fichiers
	nContinuousNumber = 3;
	nSymbolNumber = 2;
	nLinesNumber = 100;
	sClassNamePrefix = "synthetic_dictionary_";
	sFileName = "synthetic_file_";
	sSuffix = "";
	sSuffix = IntToString(nContinuousNumber);
	sSuffix += "cont_";
	sSuffix += IntToString(nSymbolNumber);
	sSuffix += "symbol_";
	sSuffix += sTmp + IntToString(nLinesNumber) + "lines";

	// Recherche du directory temporaire
	sTmpDir = FileService::GetTmpDir();
	if (sTmpDir.IsEmpty())
	{
		cout << "\nERROR\ttemporary directory not found" << endl;
		return;
	}

	// Creation du dictionnaire
	sClassName = sClassNamePrefix + sSuffix;
	kwcCreationClass = KWClass::CreateClass(sClassName, 0, nSymbolNumber, nContinuousNumber, 0, 0, 0, 0, 0, 0, 0, 0,
						0, false, NULL);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcCreationClass);
	kwcCreationClass->Compile();

	// Creation du nom du fichier
	sReferenceFilePathName = FileService::BuildFilePathName(sTmpDir, sFileName + sSuffix);

	// Initialisation de la table
	databaseSource.SetDatabaseName(sReferenceFilePathName);
	databaseSource.SetClassName(sClassName);

	// Remplissage de la table
	databaseSource.TestCreateObjects(nLinesNumber);

	// Ecriture de la table dans un fichier pour reference
	databaseSource.SetVerboseMode(false);
	bOk = databaseSource.WriteAll(&databaseSource);
	if (not bOk)
	{
		cout << "\nERROR\twhile writing " << sReferenceFilePathName << endl;
		return;
	}

	// Nettoyage des objets
	databaseSource.DeleteAll();

	// Affichage du debut de la base initiale
	KWArtificialDataset::DisplayFileFirstLines(sReferenceFilePathName, 10);

	///////////////////////////////////////////////////////////////////////////
	// Transfert

	// Initialisation de la base cible
	databaseTarget.CopyFrom(&databaseSource);
	sTransferredFilePathName = FileService::BuildFilePathName(sTmpDir, "T_" + sFileName + sSuffix);
	databaseTarget.SetDatabaseName(sTransferredFilePathName);

	// Transfert
	databaseTransfer.SetDisplayAllTaskMessages(false);
	databaseTransfer.Transfer(&databaseSource, &databaseTarget, lWrittenRecordNumber);
	cout << "Transferred instances: " << LongintToReadableString(lWrittenRecordNumber) << endl;
	KWArtificialDataset::DisplayFileFirstLines(sTransferredFilePathName, 10);

	///////////////////////////////////////////////////////////////////////////
	// Nettoyage

	// Suppression du dictionnaire
	KWClassDomain::GetCurrentDomain()->DeleteClass(sClassName);

	// Suppression des fichiers de donnees
	FileService::RemoveFile(sReferenceFilePathName);
	FileService::RemoveFile(sTransferredFilePathName);
}

boolean KWTestDatabaseTransfer::STTestTransfer()
{
	ALString sTmp;
	boolean bTestSucceed;
	ALString sSuffix;
	ALString sClassNamePrefix;
	ALString sFileName;
	ALString sReferenceFilePathName;
	ALString sFileToTransfer;
	ALString sClassName;
	KWSTDatabaseTextFile databaseSource;
	KWClass* kwcCreationClass;
	ALString sTmpDir;
	boolean bOk;
	int i;
	int j;
	int k;
	int l;
	IntVector ivNbVars;
	IntVector ivNbLines;
	int nContinuousNumber;
	int nSymbolNumber;
	int nLinesNumber;
	ALString sWrongLine;
	IntVector ivLinesIndex;

	bTestSucceed = true;

	/////////////////////////////////////////////////////////////////////////
	// Generation de fichiers synthetiques en faisant varier
	//			le nombre de variables numerique
	//			le nombre de variables symboliques
	//			le nombre de lignes
	// pour 1000 variables numeriques, 1000 variables symboliques et 10 000 lignes le fichier fait 106 MB
	//
	// modification de chacun de ces fichiers en ajoutant des lignes
	//		- vides
	//		- trop grandes
	//		- trop petites
	//
	// Comparaison entre le fichier initial et le fichier transfere

	// Nombre de variables possibles dans le dico
	ivNbVars.Add(0);
	ivNbVars.Add(1);
	ivNbVars.Add(1000);

	// Nombre de lignes des fichiers produits
	ivNbLines.Add(0);
	ivNbLines.Add(1);
	ivNbLines.Add(2);
	ivNbLines.Add(10000);

	// Ecriture du header
	cout << "#continuous"
	     << "\t"
	     << "#symbolic"
	     << "\t"
	     << "#lines"
	     << "\t"
	     << "options" << endl;

	// Boucle sur le nombre de variables continues
	for (i = 1; i < ivNbVars.GetSize(); i++)
	{
		nContinuousNumber = ivNbVars.GetAt(i);

		// Boucle sur le nombre de variables symboliques
		for (j = 0; j < ivNbVars.GetSize(); j++)
		{
			nSymbolNumber = ivNbVars.GetAt(j);

			// Creation du dictionnaire
			if (nSymbolNumber != 0 or nContinuousNumber != 0)
			{
				// Generation des fichiers
				// Boucle sur le nombre de lignes dans le fichier
				for (k = 0; k < ivNbLines.GetSize(); k++)
				{
					nLinesNumber = ivNbLines.GetAt(k);

					// Ecriture des parametres
					cout << nContinuousNumber << "\t" << nSymbolNumber << "\t" << nLinesNumber;

					//////////////////////////////////////////////////
					// Creation de la base synthetique de reference

					sTmpDir = FileService::GetTmpDir();
					if (sTmpDir.IsEmpty())
					{
						cout << "\nERROR\ttemporary directory not found" << endl;
						return false;
					}

					sClassNamePrefix = "synthetic_dictionary_";
					sFileName = "synthetic_file_";
					sSuffix = "";
					sSuffix = IntToString(nContinuousNumber);
					sSuffix += "cont_";
					sSuffix += IntToString(nSymbolNumber);
					sSuffix += "symbol_";
					sSuffix += sTmp + IntToString(nLinesNumber) + "lines_";

					// Creation du dictionnaire
					sClassName = sClassNamePrefix + sSuffix;
					kwcCreationClass =
					    KWClass::CreateClass(sClassName, 0, nSymbolNumber, nContinuousNumber, 0, 0,
								 0, 0, 0, 0, 0, 0, 0, false, NULL);
					KWClassDomain::GetCurrentDomain()->InsertClass(kwcCreationClass);
					kwcCreationClass->Compile();

					// Creation du nom du fichier
					sReferenceFilePathName =
					    FileService::BuildFilePathName(sTmpDir, sFileName + sSuffix);

					// Initialisation de la table
					databaseSource.SetDatabaseName(sReferenceFilePathName);
					databaseSource.SetClassName(sClassName);

					// Remplissage de la table
					databaseSource.TestCreateObjects(nLinesNumber);

					// Ecriture de la table dans un fichier pour reference
					databaseSource.SetVerboseMode(false);
					bOk = databaseSource.WriteAll(&databaseSource);
					if (not bOk)
					{
						cout << "\nERROR\twhile writing " << sReferenceFilePathName << endl;
						return false;
					}

					// Nettoyage
					databaseSource.DeleteAll();

					// Test
					cout << "\tstandard file";
					bTestSucceed = STIsTransferOk(&databaseSource, sReferenceFilePathName);
					if (not bTestSucceed)
						return false;

					////////////////////////////////////////////////////
					// Modification de la base de reference
					// en ajoutant des lignes erronees

					sFileToTransfer = sReferenceFilePathName + "_modified";
					databaseSource.SetDatabaseName(sFileToTransfer);

					for (l = 1; l < nLinesNumber; l++)
					{
						if (l % 10 == 0)
						{
							ivLinesIndex.Add(l);
						}
					}
					ivLinesIndex.Add(1);

					////////////////////////////////////////////////////
					// Lignes trop courtes

					// Sauf dans le cas ou il n'y a qu'une variable numerique
					cout << "\t";
					if (nSymbolNumber != 0 or nContinuousNumber != 1)
					{
						cout << "short lines";

						if (nContinuousNumber != 0)
							sWrongLine = "wrong Line";
						else if (nSymbolNumber == 1)
							sWrongLine = sTmp + "Wrong" +
								     databaseSource.GetFieldSeparator() + "Line";
						else
							sWrongLine = "Wrong";

						// Fin de ligne
						sWrongLine += FileService::GetEOL();

						// Ajout de lignes
						bOk = KWArtificialDataset::AddLinesInFile(
						    sReferenceFilePathName, sFileToTransfer, sWrongLine, &ivLinesIndex);
						if (not bOk)
						{
							cout << "\tERROR while altering file" << endl;
							return false;
						}

						// Test
						bTestSucceed = STIsTransferOk(&databaseSource, sReferenceFilePathName);
						if (not bTestSucceed)
							return false;
					}

					////////////////////////////////////////////////////
					// Lignes vides

					// Sauf dans le cas ou il n'y a qu'une variable symbolique ou numerique
					cout << "\t";
					if (not(nContinuousNumber == 0 and nSymbolNumber == 1) and
					    not(nContinuousNumber == 1 and nSymbolNumber == 0))
					{
						cout << "empty lines";

						// Fin de ligne
						sWrongLine = FileService::GetEOL();

						// Ajout de lignes dans le fichier
						bOk = KWArtificialDataset::AddLinesInFile(
						    sReferenceFilePathName, sFileToTransfer, sWrongLine, &ivLinesIndex);
						if (not bOk)
						{
							cout << "\tERROR while altering file" << endl;
							return false;
						}

						// Test
						bTestSucceed = STIsTransferOk(&databaseSource, sReferenceFilePathName);
						if (not bTestSucceed)
							return false;
					}

					////////////////////////////////////////////////////
					// Lignes trop longues

					cout << "\tbig lines";
					sWrongLine = "";
					for (l = 0; l < nSymbolNumber; l++)
					{
						sWrongLine += "toto";
						sWrongLine += databaseSource.GetFieldSeparator();
					}
					for (l = 0; l < nContinuousNumber; l++)
					{
						sWrongLine += "1";
						sWrongLine += databaseSource.GetFieldSeparator();
					}
					sWrongLine += "exceed";
					sWrongLine += FileService::GetEOL();

					// Ajout de lignes dans le fichier
					bOk = KWArtificialDataset::AddLinesInFile(
					    sReferenceFilePathName, sFileToTransfer, sWrongLine, &ivLinesIndex);
					if (not bOk)
					{
						cout << "\tERROR while altering file" << endl;
						return false;
					}

					// Test
					bTestSucceed = STIsTransferOk(&databaseSource, sReferenceFilePathName);
					if (not bTestSucceed)
						return false;
					cout << endl;
					//////////////////////////////////////////////////////////////////
					// Nettoyage

					// Suppression du dictionnaire
					KWClassDomain::GetCurrentDomain()->DeleteClass(sClassName);

					// Suppression du fichier de reference
					if (bTestSucceed)
					{
						FileService::RemoveFile(sFileToTransfer);
						FileService::RemoveFile(sReferenceFilePathName);
					}
					if (not bTestSucceed)
					{
						cout << "\nERROR\t" << endl;
						return false;
					}
					ivLinesIndex.SetSize(0);
				} // Fin boucle sur le nombre de lignes dans le fichier
			}
		}
	}
	return bTestSucceed;
}

boolean KWTestDatabaseTransfer::STIsTransferOk(KWSTDatabaseTextFile* databaseSource,
					       const ALString& sReferenceFilePathName)
{
	KWDatabaseTransferTask databaseTransfer;
	ALString sClassNamePrefix;
	ALString sTransferredFilePathName;
	ALString sTransferFileName;
	ALString sClassName;
	KWSTDatabaseTextFile databaseTarget;
	ALString sTmpDir;
	ALString sSuffix;
	ALString sTmp;
	boolean bTransferDone;
	boolean bTestSucceed;
	longint lWrittenRecordNumber;

	///////////////////////////////////////////////////////////////////////////
	// Transfert

	// Initialisation de la base cible
	databaseTarget.CopyFrom(databaseSource);
	sTransferFileName = databaseSource->GetDatabaseName() + "_Transferred";
	databaseTarget.SetDatabaseName(sTransferFileName);

	// Transfert
	databaseTransfer.SetDisplayAllTaskMessages(false);
	databaseTransfer.Transfer(databaseSource, &databaseTarget, lWrittenRecordNumber);

	// Indicateurs de succes du transfer
	bTransferDone = PLRemoteFileService::FileExists(sTransferFileName);
	bTestSucceed = true;
	if (not bTransferDone)
	{
		cout << "\nERROR\twhile transfering " << databaseSource->GetDatabaseName() << endl;
		bTestSucceed = false;
	}
	else
	{
		// Comparaison entre le fichier de reference et le fichier resultat
		if (not FileService::FileCompare(sTransferFileName, sReferenceFilePathName))
		{
			cout << "ERROR";
			cout << "\t" << sTransferFileName << endl;
			cout << "and" << endl;
			cout << "\t" << sReferenceFilePathName << endl;
			cout << "are different" << endl;
			bTestSucceed = false;
		}
	}

	// Nettoyage
	if (bTestSucceed and bTransferDone)
	{
		// Destruction du fichier resultat
		FileService::RemoveFile(sTransferFileName);
	}
	return bTestSucceed;
}

void KWTestDatabaseTransfer::STMainTestReadWrite(int argc, char** argv)
{
	if (argc != 5)
	{
		cout << "TestReadWrite "
		     << "<ClassFileName> "
		     << "<ClassName> "
		     << "<ReadFileName> "
		     << "<WriteFileName> " << endl;
	}
	else
		STTestReadWrite(argv[1], argv[2], argv[3], argv[4]);
}

void KWTestDatabaseTransfer::STTestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
					     const ALString& sReadFileName, const ALString& sWriteFileName)
{
	KWDatabaseTransferTask databaseTransfer;
	KWSTDatabaseTextFile readDatabase;
	KWSTDatabaseTextFile writeDatabase;
	ALString sTestClassName;
	KWClass* testClass;
	clock_t tStart;
	clock_t tStop;
	longint lWrittentRecordNumber;
	double dTransferTime;

	// Initialisations
	sTestClassName = sClassName;
	if (sTestClassName == "")
		sTestClassName = "TestClass";
	readDatabase.SetClassName(sTestClassName);
	readDatabase.SetDatabaseName(sReadFileName);
	writeDatabase.SetClassName(sTestClassName);
	writeDatabase.SetDatabaseName(sWriteFileName);

	// Gestion de la classe
	KWClassDomain::DeleteAllDomains();
	if (sClassFileName != "")
	{
		cout << "Read dictionary file " << sClassFileName << endl;
		KWClassDomain::GetCurrentDomain()->ReadFile(sClassFileName);
	}
	if (KWClassDomain::GetCurrentDomain()->Check())
		KWClassDomain::GetCurrentDomain()->Compile();

	// Creation eventuelle de la classe a partir du fichier
	if (sClassFileName == "" or KWClassDomain::GetCurrentDomain()->LookupClass(sTestClassName) == NULL)
	{
		cout << "Create dictionary from file\n";
		KWClassDomain::DeleteAllDomains();
		readDatabase.ComputeClass();
	}

	// Statistique
	testClass = KWClassDomain::GetCurrentDomain()->LookupClass(readDatabase.GetClassName());
	if (testClass != NULL)
		cout << "\n\t" << testClass->GetName() << "\t" << testClass->GetAttributeNumber() << " variables"
		     << endl;

	// Operations efefctuees si classe valide
	if (testClass != NULL and testClass->Check())
	{
		// Evaluation du nombre d'instances
		cout << "Evaluation of the number of instances" << endl;
		cout << "\t" << readDatabase.GetEstimatedObjectNumber() << " instances" << endl;

		// Gestion des taches
		TaskProgression::SetTitle("Test " + databaseTransfer.GetTaskLabel());
		if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
			TaskProgression::SetDisplayedLevelNumber(1);
		else
			TaskProgression::SetDisplayedLevelNumber(2);
		TaskProgression::Start();

		// Transfert des instances
		cout << "Transfer of instances" << endl;
		tStart = clock();
		databaseTransfer.SetDisplayAllTaskMessages(false);
		databaseTransfer.Transfer(&readDatabase, &writeDatabase, lWrittentRecordNumber);
		tStop = clock();
		dTransferTime = ((double)(tStop - tStart) / CLOCKS_PER_SEC);
		cout << "\tTransfer time: " << dTransferTime << endl;

		// Gestion des taches
		TaskProgression::Stop();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MT

void KWTestDatabaseTransfer::MTTest()
{
	MTTestWithArtificialDatabase(2, 2, true, 10000, 1, 1, 100000, 10, 1, 0, 1000000, "Standard");
	MTTestWithArtificialDatabase(0, 0, true, 10000, 1, 1, 0, 10, 1, 0, 100000, "Mono-table no key");
	MTTestWithArtificialDatabase(1, 1, true, 10000, 1, 1, 0, 10, 1, 0, 100000, "Mono-table");
	MTTestWithArtificialDatabase(1, 1, true, 100, 2, 1, 0, 10, 1, 100, 200, "Mono-table with duplicates");
	MTTestWithArtificialDatabase(2, 1, true, 10000, 1, 1, 100000, 10, 1, 0, 1000000,
				     "Secondary stats but not use of secondary table");
	MTTestWithArtificialDatabase(2, 1, false, 10000, 1, 1, 100000, 10, 1, 0, 100000, "No use of secondary table");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 1, 0.01, 1000000, 10, 0.01, 10000, 100000,
				     "Heavily sampled, with orphan records");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 5, 1, 100000, 1, 1, 10000, 1000000, "Root duplicates");
	MTTestWithArtificialDatabase(2, 1, false, 100, 2, 1, 0, 10, 1, 100, 200,
				     "Root duplicates with several processes");
	MTTestWithArtificialDatabase(2, 2, true, 300, 2, 0.3, 500, 2, 1, 100, 1000,
				     "Root duplicates and orphans records with several processes");
	MTTestWithArtificialDatabase(2, 2, true, 300, 3, 1, 10000, 10, 1, 100, 1000,
				     "Root duplicates and many orphans records with several processes");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 1, 0, 1000000, 10, 0.01, 10000, 100000, "No root instances");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 1, 0.01, 1000000, 10, 0, 10000, 100000,
				     "No secondary records");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 1, 0.5, 1000000, 10, 0.2, 10000, 1000000,
				     "Medium sampled with varying record number per instance");
	MTTestWithArtificialDatabase(2, 2, true, 1, 1, 1, 1000000, 10, 0.01, 10000, 10000, "One single root instance");
	MTTestWithArtificialDatabase(2, 2, true, 100000, 1, 1, 1000000, 10, 1, 0, 0, "Large");
	MTTestWithArtificialDatabase(3, 3, true, 10000, 1, 1, 100000, 10, 1, 10000, 100000, "Snow-flake schema");
	MTTestWithArtificialDatabase(3, 0, true, 10000, 1, 1, 100000, 10, 1, 10000, 100000,
				     "Snow-flake schema with no used sub-table");
	MTTestWithArtificialDatabase(3, 0, false, 10000, 1, 1, 100000, 10, 1, 10000, 100000,
				     "Snow-flake schema using only root table");
	MTTestWithArtificialDatabase(4, 4, true, 10000, 1, 1, 100000, 10, 1, 10000, 100000, "Full schema");
	MTTestWithArtificialDatabase(4, 0, true, 10000, 1, 1, 100000, 10, 1, 10000, 100000,
				     "Full schema with no used sub-table");
	MTTestWithArtificialDatabase(4, 0, false, 10000, 1, 1, 100000, 10, 1, 10000, 100000,
				     "Full schema using only root table");
}

void KWTestDatabaseTransfer::MTTestWithArtificialDatabase(int nTableNumber, int nUsedTableNumber,
							  boolean bUseBuildRules, int nRootLineNumber,
							  int nRootLineNumberPerKey, double dRootSamplingRate,
							  int nSecondaryLineNumber, int nSecondaryLineNumberPerKey,
							  double dSecondarySamplingRate, int nBufferSize,
							  int lMaxFileSizePerProcess, const ALString& sTestLabel)
{
	boolean bShowTime = false;
	KWDatabaseTransferTask databaseTransfer;
	boolean bOk;
	const int nRootIndex = 0;
	const int nSecondary01Index = 1;
	const int nSecondary0nIndex = 2;
	const int nExternalIndex = 3;
	const int nMaxTableNumber = 4;
	int nTable;
	int nClass;
	ObjectArray oaArtificialDatasets;
	KWArtificialDataset* artificialDataset;
	ObjectArray oaArtificialClasses;
	KWClass* kwcClass;
	KWClass* kwcSubClass;
	StringVector svTransferredFilePathNames;
	ALString sTransferredFilePathName;
	longint lWrittenRecordNumber;
	KWMTDatabaseTextFile databaseSource;
	KWMTDatabaseTextFile databaseTarget;
	KWAttribute* attributeLink;
	KWAttribute* attributeCompute;
	KWDerivationRule* ruleCompute;
	KWDerivationRule* ruleReference;
	KWMTDatabaseMapping* mapping;
	Timer timer;

	require(nTableNumber >= 0);
	require(nUsedTableNumber >= 0);
	require(nUsedTableNumber <= nTableNumber);
	require(nRootLineNumber >= 0);
	require(nRootLineNumberPerKey >= 0);
	require(dRootSamplingRate >= 0);
	require(nSecondaryLineNumber >= 0);
	require(nSecondaryLineNumberPerKey >= 0);
	require(dSecondarySamplingRate >= 0);
	require(nBufferSize >= 0);
	require(lMaxFileSizePerProcess >= 0);

	// Gestion des taches
	TaskProgression::SetTitle("Test " + databaseTransfer.GetTaskLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Affichage d'un entete de test
	cout << endl;
	cout << "=============================================================================" << endl;
	cout << databaseTransfer.GetTaskLabel() << ":"
	     << " Root(" << nRootLineNumber << ", " << nRootLineNumberPerKey << ", " << dRootSamplingRate << ")";
	if (nTableNumber >= 2)
		cout << " Secondary(" << nSecondaryLineNumber << ", " << nSecondaryLineNumberPerKey << ", "
		     << dSecondarySamplingRate << ")";
	cout << endl;
	cout << "  Tables(" << nTableNumber << ", " << nUsedTableNumber << ", " << bUseBuildRules << ")"
	     << " Sizes(" << nBufferSize << ", " << lMaxFileSizePerProcess << ")" << endl;
	cout << "  " << sTestLabel << endl;
	cout << "=============================================================================" << endl;

	/////////////////////////////////////////////////////////////////////
	// Creation de la base initiale et de son dictionnaire

	// Initialisations
	oaArtificialDatasets.SetSize(nMaxTableNumber);
	oaArtificialClasses.SetSize(nMaxTableNumber);
	svTransferredFilePathNames.SetSize(nMaxTableNumber);

	// Creation du fichier principal
	artificialDataset = new KWArtificialDataset;
	oaArtificialDatasets.SetAt(nRootIndex, artificialDataset);
	timer.Reset();
	timer.Start();
	if (nTableNumber == 0)
		artificialDataset->GetKeyFieldIndexes()->SetSize(0);
	artificialDataset->SetLineNumber(nRootLineNumber);
	artificialDataset->SetMaxLineNumberPerKey(nRootLineNumberPerKey);
	artificialDataset->SetSamplingRate(dRootSamplingRate);
	artificialDataset->SetFileName(FileService::BuildFilePathName(FileService::GetTmpDir(), "Root.txt"));
	artificialDataset->CreateDataset();
	artificialDataset->DisplayFirstLines(15);
	timer.Stop();
	if (bShowTime)
		cout << "Creation time: " << timer.GetElapsedTime() << endl;

	// Creation de son dictionnaire
	kwcClass = ComputeArtificialClass(artificialDataset);
	oaArtificialClasses.SetAt(nRootIndex, kwcClass);
	if (nTableNumber >= 1)
		kwcClass->SetRoot(true);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);

	// Creation du fichier secondaire en relation 0-1
	if (nTableNumber >= 3)
	{
		// Utilisation d'une copie du fichier principal
		artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nRootIndex))->Clone();
		oaArtificialDatasets.SetAt(nSecondary01Index, artificialDataset);
		timer.Reset();
		timer.Start();
		artificialDataset->SetFieldNumber(3);
		artificialDataset->GetKeyFieldIndexes()->SetAt(0, 2);
		artificialDataset->SetFileName(
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "Secondary01.txt"));
		artificialDataset->CreateDataset();
		artificialDataset->DisplayFirstLines(15);
		timer.Stop();
		if (bShowTime)
			cout << "Creation time: " << timer.GetElapsedTime() << endl;

		// Creation de son dictionnaire
		kwcClass = ComputeArtificialClass(artificialDataset);
		oaArtificialClasses.SetAt(nSecondary01Index, kwcClass);
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
	}

	// Creation du fichier secondaire en relation 0-n
	if (nTableNumber >= 2)
	{
		// Utilisation de deux champs, avec la cle en fin
		// pour perturber sa position par rapport a la table racine
		artificialDataset = new KWArtificialDataset;
		oaArtificialDatasets.SetAt(nSecondary0nIndex, artificialDataset);
		timer.Reset();
		timer.Start();
		artificialDataset->CopyFrom(cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nRootIndex)));
		artificialDataset->SetFieldNumber(2);
		artificialDataset->GetKeyFieldIndexes()->SetAt(0, 1);
		artificialDataset->SetLineNumber(nSecondaryLineNumber);
		artificialDataset->SetMaxLineNumberPerKey(nSecondaryLineNumberPerKey);
		artificialDataset->SetSamplingRate(dSecondarySamplingRate);
		artificialDataset->SetFileName(
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "Secondary0n.txt"));
		artificialDataset->CreateDataset();
		artificialDataset->DisplayFirstLines(15);
		timer.Stop();
		if (bShowTime)
			cout << "Creation time: " << timer.GetElapsedTime() << endl;

		// Creation de son dictionnaire
		kwcClass = ComputeArtificialClass(artificialDataset);
		oaArtificialClasses.SetAt(nSecondary0nIndex, kwcClass);
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
	}

	// Creation du fichier externe
	if (nTableNumber >= 4)
	{
		// Utilisation d'une copie du fichier principal
		artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nRootIndex))->Clone();
		oaArtificialDatasets.SetAt(nExternalIndex, artificialDataset);
		timer.Reset();
		timer.Start();
		artificialDataset->SetFieldNumber(5);
		artificialDataset->GetKeyFieldIndexes()->SetAt(0, 3);
		artificialDataset->SetFileName(
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "External.txt"));
		artificialDataset->CreateDataset();
		artificialDataset->DisplayFirstLines(15);
		timer.Stop();
		if (bShowTime)
			cout << "Creation time: " << timer.GetElapsedTime() << endl;

		// Creation de son dictionnaire
		kwcClass = ComputeArtificialClass(artificialDataset);
		kwcClass->SetRoot(true);
		oaArtificialClasses.SetAt(nExternalIndex, kwcClass);
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
	}

	// Lien avec la table secondaire 0-n
	if (nTableNumber >= 2)
	{
		// Acces aux tables a lier
		if (nTableNumber == 2)
			kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nRootIndex));
		else
			kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nSecondary01Index));
		kwcSubClass = cast(KWClass*, oaArtificialClasses.GetAt(nSecondary0nIndex));

		// Preparation du schema multi-tables avec un attribut de lien vers la table secondaire
		attributeLink = new KWAttribute;
		attributeLink->SetType(KWType::ObjectArray);
		attributeLink->SetName("Records");
		attributeLink->SetClass(kwcSubClass);
		kwcClass->InsertAttribute(attributeLink);

		// Ajout d'un attribut pour compter les enregistrements
		attributeCompute = new KWAttribute;
		attributeCompute->SetName("RecordCount");
		ruleCompute = new KWDRTableCount;
		ruleCompute->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		ruleCompute->GetFirstOperand()->SetAttributeName(attributeLink->GetName());
		attributeCompute->SetDerivationRule(ruleCompute);
		attributeCompute->CompleteTypeInfo(kwcClass);
		kwcClass->InsertAttribute(attributeCompute);

		// Parametrage d'utilisation de la sous-table et de la regle
		if (nTableNumber == 2)
			attributeLink->SetUsed(nUsedTableNumber >= 2);
		else
			attributeLink->SetUsed(nUsedTableNumber >= 3);
		attributeCompute->SetUsed(bUseBuildRules);
	}

	// Lien avec la table secondaire 0-1
	if (nTableNumber >= 3)
	{
		// Acces aux tables a lier
		kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nRootIndex));
		kwcSubClass = cast(KWClass*, oaArtificialClasses.GetAt(nSecondary01Index));

		// Preparation du schema multi-tables avec un attribut de lien vers la table secondaire
		attributeLink = new KWAttribute;
		attributeLink->SetType(KWType::Object);
		attributeLink->SetName("SubPart");
		attributeLink->SetClass(kwcSubClass);
		kwcClass->InsertAttribute(attributeLink);

		// Ajout d'un attribut pour compter les enregistrements de la table secondaire
		attributeCompute = new KWAttribute;
		attributeCompute->SetName("RecordCount");
		ruleCompute = new KWDRGetContinuousValue;
		ruleCompute->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		ruleCompute->GetFirstOperand()->SetAttributeName(attributeLink->GetName());
		ruleCompute->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		ruleCompute->GetSecondOperand()->SetAttributeName("RecordCount");
		attributeCompute->SetDerivationRule(ruleCompute);
		attributeCompute->CompleteTypeInfo(kwcClass);
		kwcClass->InsertAttribute(attributeCompute);

		// Parametrage d'utilisation de la sous-table et de la regle
		attributeLink->SetUsed(nUsedTableNumber >= 2);
		attributeCompute->SetUsed(bUseBuildRules);
	}

	// Lien avec la table externe
	if (nTableNumber >= 4)
	{
		// Acces aux tables a lier
		kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nRootIndex));
		kwcSubClass = cast(KWClass*, oaArtificialClasses.GetAt(nExternalIndex));
		assert(kwcSubClass->GetKeyAttributeNumber() == 1);

		// Preparation du schema multi-tables avec un attribut de lien vers la table externe
		attributeLink = new KWAttribute;
		attributeLink->SetType(KWType::Object);
		attributeLink->SetName("ExternalRef");
		attributeLink->SetClass(kwcSubClass);
		kwcClass->InsertAttribute(attributeLink);

		// Ajout de la reference
		ruleReference = new KWDRReference;
		ruleReference->SetObjectClassName(kwcSubClass->GetName());
		ruleReference->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		ruleReference->GetFirstOperand()->SetAttributeName(kwcSubClass->GetKeyAttributeNameAt(0));
		attributeLink->SetDerivationRule(ruleReference);
		attributeLink->CompleteTypeInfo(kwcClass);

		// Ajout d'un attribut pour tester la presence de la table externe
		attributeCompute = new KWAttribute;
		attributeCompute->SetName("RefExist");
		ruleCompute = new KWDRExist;
		ruleCompute->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		ruleCompute->GetFirstOperand()->SetAttributeName(attributeLink->GetName());
		attributeCompute->SetDerivationRule(ruleCompute);
		attributeCompute->CompleteTypeInfo(kwcClass);
		kwcClass->InsertAttribute(attributeCompute);

		// Parametrage d'utilisation de la sous-table et de la regle
		attributeLink->SetUsed(nUsedTableNumber >= 4);
		attributeCompute->SetUsed(bUseBuildRules);
	}

	// Compilation du domaine
	KWClassDomain::GetCurrentDomain()->Compile();

	// Initialisation de la base source
	kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nRootIndex));
	artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nRootIndex));
	databaseSource.SetClassName(kwcClass->GetName());
	databaseSource.SetHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
	databaseSource.SetFieldSeparator(artificialDataset->GetFieldSeparator());
	databaseSource.UpdateMultiTableMappings();

	// Parametrage des fichiers sources
	for (nTable = 0; nTable < databaseSource.GetMultiTableMappings()->GetSize(); nTable++)
	{
		mapping = cast(KWMTDatabaseMapping*, databaseSource.GetMultiTableMappings()->GetAt(nTable));

		// Recherche du fichier correspondant au dictionnaire mappe
		for (nClass = 0; nClass < oaArtificialClasses.GetSize(); nClass++)
		{
			kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nClass));
			if (kwcClass != NULL and kwcClass->GetName() == mapping->GetClassName())
			{
				artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nClass));
				mapping->SetDataTableName(artificialDataset->GetFileName());
			}
		}
	}
	assert(databaseSource.Check());

	///////////////////////////////////////////////////////////////////////////
	// Transfert

	// Creation des noms de fichier cibles
	for (nTable = 0; nTable < oaArtificialDatasets.GetSize(); nTable++)
	{
		artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nTable));
		if (artificialDataset != NULL)
		{
			sTransferredFilePathName = FileService::SetFilePrefix(
			    artificialDataset->GetFileName(),
			    "T_" + FileService::GetFilePrefix(artificialDataset->GetFileName()));
			svTransferredFilePathNames.SetAt(nTable, sTransferredFilePathName);
		}
	}

	// Initialisation de la base cible
	databaseTarget.CopyFrom(&databaseSource);
	for (nTable = 0; nTable < databaseTarget.GetMultiTableMappings()->GetSize(); nTable++)
	{
		mapping = cast(KWMTDatabaseMapping*, databaseTarget.GetMultiTableMappings()->GetAt(nTable));

		// Recherche du fichier correspondant au dictionnaire mappe
		for (nClass = 0; nClass < oaArtificialClasses.GetSize(); nClass++)
		{
			kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nClass));
			if (kwcClass != NULL and kwcClass->GetName() == mapping->GetClassName())
			{
				mapping->SetDataTableName(svTransferredFilePathNames.GetAt(nClass));
			}
		}
	}
	databaseTarget.SetVerboseMode(true);
	assert(databaseTarget.CheckPartially(true));

	// Transfert
	databaseTransfer.nForcedBufferSize = nBufferSize;
	databaseTransfer.lForcedMaxFileSizePerProcess = lMaxFileSizePerProcess;
	databaseTransfer.AddSimpleMessage(databaseTransfer.GetClassLabel() + ": " + sTestLabel);
	timer.Reset();
	timer.Start();
	databaseTransfer.SetDisplayAllTaskMessages(false);
	bOk = databaseTransfer.Transfer(&databaseSource, &databaseTarget, lWrittenRecordNumber);
	timer.Stop();
	if (bShowTime)
		cout << "Transfer time: " << timer.GetElapsedTime() << endl;
	cout << "Transferred instances: " << LongintToReadableString(lWrittenRecordNumber) << " (" << bOk << ")"
	     << endl;
	for (nTable = 0; nTable < oaArtificialDatasets.GetSize(); nTable++)
	{
		sTransferredFilePathName = svTransferredFilePathNames.GetAt(nTable);
		if (sTransferredFilePathName != "")
			KWArtificialDataset::DisplayFileFirstLines(sTransferredFilePathName, 15);
	}

	///////////////////////////////////////////////////////////////////////////
	// Nettoyage

	// Suppression des dictionnaires et des fichier de donnees
	for (nTable = 0; nTable < oaArtificialDatasets.GetSize(); nTable++)
	{
		// Destruction du dictionnaire
		kwcClass = cast(KWClass*, oaArtificialClasses.GetAt(nTable));
		if (kwcClass != NULL)
			KWClassDomain::GetCurrentDomain()->DeleteClass(kwcClass->GetName());

		// Suppression des fichiers de donnees
		artificialDataset = cast(KWArtificialDataset*, oaArtificialDatasets.GetAt(nTable));
		if (artificialDataset != NULL)
		{
			artificialDataset->DeleteDataset();
			FileService::RemoveFile(svTransferredFilePathNames.GetAt(nTable));
			delete artificialDataset;
		}
	}

	// Gestion des taches
	TaskProgression::Stop();
}

void KWTestDatabaseTransfer::MTMainTestReadWrite(int argc, char** argv)
{
	if (argc != 7)
	{
		cout << "TestReadWrite "
		     << "<ClassFileName> "
		     << "<ClassName> "
		     << "<RootReadFileName> "
		     << "<SecondaryReadFileName> "
		     << "<RootWriteFileName> "
		     << "<SecondaryWriteFileName>" << endl;
	}
	else
		MTTestReadWrite(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
}

void KWTestDatabaseTransfer::MTTestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
					     const ALString& sRootReadFileName, const ALString& sSecondaryReadFileName,
					     const ALString& sRootWriteFileName,
					     const ALString& sSecondaryWriteFileName)
{
	boolean bOk = true;
	KWDatabaseTransferTask databaseTransfer;
	KWMTDatabaseTextFile readDatabase;
	KWMTDatabaseTextFile writeDatabase;
	longint lWrittentRecordNumber;

	// Gestion de la classe
	KWClassDomain::DeleteAllDomains();
	if (bOk)
	{
		cout << "Read dictionary file " << sClassFileName << endl;
		bOk = KWClassDomain::GetCurrentDomain()->ReadFile(sClassFileName);
	}
	if (bOk)
		bOk = KWClassDomain::GetCurrentDomain()->Check();
	if (bOk)
		KWClassDomain::GetCurrentDomain()->Compile();
	if (bOk and KWClassDomain::GetCurrentDomain()->LookupClass(sClassName) == NULL)
	{
		cout << "Dictionary " + sClassName + " not found" << endl;
		bOk = false;
	}

	// Initialisation de la base source
	if (bOk)
	{
		// Initialisation de la base source
		readDatabase.SetClassName(sClassName);
		readDatabase.SetDatabaseName(sRootReadFileName);
		readDatabase.UpdateMultiTableMappings();

		// Initialisation de la table secondaire
		if (readDatabase.GetMultiTableMappings()->GetSize() == 2)
			cast(KWMTDatabaseMapping*, readDatabase.GetMultiTableMappings()->GetAt(1))
			    ->SetDataTableName(sSecondaryReadFileName);
	}

	// Initialisation de la base cible
	if (bOk)
	{
		writeDatabase.SetClassName(sClassName);
		writeDatabase.SetDatabaseName(sRootWriteFileName);
		writeDatabase.UpdateMultiTableMappings();

		// Initialisation de la table secondaire
		if (writeDatabase.GetMultiTableMappings()->GetSize() == 2)
			cast(KWMTDatabaseMapping*, writeDatabase.GetMultiTableMappings()->GetAt(1))
			    ->SetDataTableName(sSecondaryWriteFileName);
	}

	// Transfert
	if (bOk)
	{
		// Evaluation du nombre d'instances
		cout << "Evaluation of the number of source instances" << endl;
		cout << "\t" << readDatabase.GetEstimatedObjectNumber() << " instances" << endl;

		// Gestion des taches
		TaskProgression::SetTitle("Test " + databaseTransfer.GetTaskLabel());
		if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
			TaskProgression::SetDisplayedLevelNumber(1);
		else
			TaskProgression::SetDisplayedLevelNumber(2);
		TaskProgression::Start();

		// Transfert des instances
		cout << "Transfer of instances" << endl;
		databaseTransfer.SetDisplayAllTaskMessages(false);
		databaseTransfer.Transfer(&readDatabase, &writeDatabase, lWrittentRecordNumber);
		cout << "\tTransfer time: " << databaseTransfer.GetFullJobElapsedTime() << endl;

		// Gestion des taches
		TaskProgression::Stop();
	}

	// Nettoyage
	KWClassDomain::DeleteAllDomains();
}
