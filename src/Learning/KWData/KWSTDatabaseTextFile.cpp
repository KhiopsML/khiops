// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSTDatabaseTextFile.h"

KWSTDatabaseTextFile::KWSTDatabaseTextFile()
{
	// Parametrage du driver de table en remplacant celui de la classe ancetre
	assert(dataTableDriverCreator != NULL);
	delete dataTableDriverCreator;
	dataTableDriverCreator = new KWDataTableDriverTextFile;
}

KWSTDatabaseTextFile::~KWSTDatabaseTextFile() {}

KWDatabase* KWSTDatabaseTextFile::Create() const
{
	return new KWSTDatabaseTextFile;
}

ALString KWSTDatabaseTextFile::GetTechnologyName() const
{
	return "Single table text file";
}

void KWSTDatabaseTextFile::SetHeaderLineUsed(boolean bValue)
{
	cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->SetHeaderLineUsed(bValue);
}

boolean KWSTDatabaseTextFile::GetHeaderLineUsed() const
{
	return cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->GetHeaderLineUsed();
}

void KWSTDatabaseTextFile::SetFieldSeparator(char cValue)
{
	cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->SetFieldSeparator(cValue);
}

char KWSTDatabaseTextFile::GetFieldSeparator() const
{
	return cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->GetFieldSeparator();
}

void KWSTDatabaseTextFile::TestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
					 const ALString& sReadFileName, const ALString& sWriteFileName)
{
	KWSTDatabaseTextFile readDatabase;
	KWSTDatabaseTextFile writeFileDatabase;
	KWDatabase writeNullDatabase;
	KWDatabase* writeDatabase;
	ALString sTestClassName;
	KWClass* testClass;
	clock_t tStart;
	clock_t tStop;
	double dTransferTime;

	// Initialisations
	sTestClassName = sClassName;
	if (sTestClassName == "")
		sTestClassName = "TestClass";
	readDatabase.SetClassName(sTestClassName);
	readDatabase.SetDatabaseName(sReadFileName);
	if (sWriteFileName == "")
		writeDatabase = &writeNullDatabase;
	else
		writeDatabase = &writeFileDatabase;
	writeDatabase->SetClassName(sTestClassName);
	writeDatabase->SetDatabaseName(sWriteFileName);

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
		cout << *testClass << "\n\t" << testClass->GetAttributeNumber() << " variables" << endl;

	// Operations efefctuees si classe valide
	if (testClass != NULL and testClass->Check())
	{
		// Evaluation du nombre d'instances
		cout << "Evaluation of the number of instances" << endl;
		cout << "\t" << readDatabase.GetEstimatedObjectNumber() << " instances" << endl;

		// Transfert des instances
		cout << "Read/write of instances" << endl;
		tStart = clock();
		readDatabase.ReadAll();
		writeDatabase->WriteAll(&readDatabase);
		readDatabase.DeleteAll();
		tStop = clock();
		dTransferTime = ((double)(tStop - tStart) / CLOCKS_PER_SEC);
		cout << "\tRead/write time: " << dTransferTime << endl;
	}
}

void KWSTDatabaseTextFile::Test()
{
	KWDRCopySymbol refCopySymbolRule;
	KWClass* testClass;
	KWSTDatabaseTextFile database;
	KWSTDatabaseTextFile databaseTarget;
	KWDerivationRule* refRule;
	ObjectArray oaAvailableRules;
	KWDerivationRule* availableRule;
	KWAttribute* attribute;
	ALString sTmpDir;
	int i;
	boolean bExport;

	// Creation d'objets en memoire
	sTmpDir = FileService::GetTmpDir();
	database.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database.txt"));
	database.TestCreateObjects(100);

	// Ecriture puis lecture
	cout << "Write then read " << FileService::GetPortableTmpFilePathName(database.GetDatabaseName()) << endl;
	database.WriteAll(&database);
	database.TestRead();

	// Relecture puis ecriture
	cout << "Read all" << endl;
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_W.txt"));
	databaseTarget.SetClassName(database.GetClassName());
	database.ReadAll();

	// Ecriture avec entete
	cout << "Write with header line" << endl;
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_H.txt"));
	databaseTarget.WriteAll(&database);

	// Ecriture sans entete
	cout << "Write without header line" << endl;
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_NoH.txt"));
	databaseTarget.SetHeaderLineUsed(false);
	databaseTarget.WriteAll(&database);

	// Reconstruction de la classe a partir du fichier sans entete
	cout << "Re-build of dictionary from the file without header line" << endl;
	database.DeleteAll();
	databaseTarget.DeleteAll();
	KWClassDomain::DeleteAllDomains();
	databaseTarget.ComputeClass();
	testClass = KWClassDomain::GetCurrentDomain()->LookupClass(database.GetClassName());
	check(testClass);
	cout << *testClass << endl;
	databaseTarget.ReadAll();

	// Reconstruction de la classe a partir du fichier avec entete
	cout << "Re-build of dictionary from the file with header line" << endl;
	database.DeleteAll();
	databaseTarget.DeleteAll();
	KWClassDomain::DeleteAllDomains();
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database.txt"));
	databaseTarget.SetHeaderLineUsed(true);
	databaseTarget.ComputeClass();
	testClass = KWClassDomain::GetCurrentDomain()->LookupClass(database.GetClassName());
	check(testClass);
	cout << *testClass << endl;
	databaseTarget.ReadAll();

	// Transfert en deux sous-echantillons
	cout << "Transfer in two sub-samples" << endl;
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_Te25.txt"));
	database.SetSampleNumberPercentage(25);
	database.ReadAll();
	databaseTarget.WriteAll(&database);
	database.DeleteAll();
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_Te75.txt"));
	database.SetModeExcludeSample(true);
	database.ReadAll();
	databaseTarget.WriteAll(&database);
	database.DeleteAll();
	database.SetSampleNumberPercentage(100);
	database.SetModeExcludeSample(false);

	// Selection
	testClass = KWClassDomain::GetCurrentDomain()->LookupClass(database.GetClassName());
	check(testClass);
	assert(testClass->GetUsedAttributeNumberForType(KWType::Continuous) > 0);
	assert(testClass->LookupAttribute("AttN1") != NULL);
	database.SetSelectionAttribute("AttN1");
	database.SetSelectionValue("15");
	cout << "Selection for condition " << database.GetSelectionAttribute() << "=" << database.GetSelectionValue()
	     << endl;
	databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_Tselect.txt"));
	database.ReadAll();
	databaseTarget.WriteAll(&database);
	database.DeleteAll();
	database.SetSelectionAttribute("");
	database.SetSelectionValue("");

	// Tests sur les regles de derivation et la bufferisation des lectures
	refRule = KWDerivationRule::LookupDerivationRule(refCopySymbolRule.GetName());
	if (refRule != NULL)
	{
		cout << "Tests on derivation rules and read buffers" << endl;

		// Recherche des regles de derivation applicables
		attribute = testClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Creation si possible d'une regle
			if (attribute->GetType() == KWType::Symbol)
			{
				availableRule = refRule->Clone();
				availableRule->SetClassName(testClass->GetName());
				availableRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				availableRule->GetFirstOperand()->SetAttributeName(attribute->GetName());
				oaAvailableRules.Add(availableRule);
			}

			// Attribut suivant
			testClass->GetNextAttribute(attribute);
		}

		// Insertion dans la classe
		database.DeleteAll();
		databaseTarget.DeleteAll();
		for (i = 0; i < oaAvailableRules.GetSize(); i++)
		{
			availableRule = cast(KWDerivationRule*, oaAvailableRules.GetAt(i));
			attribute = new KWAttribute;
			attribute->SetType(availableRule->GetType());
			attribute->SetDerivationRule(availableRule);
			attribute->SetName(availableRule->ComputeAttributeName());
			testClass->InsertAttribute(attribute);
		}
		testClass->Compile();

		// Transfert avec calcul des attributs derives
		cout << "Transfer with computation of derived variables" << endl;
		databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_AllD.txt"));
		database.ReadAll();
		databaseTarget.WriteAll(&database);
		database.DeleteAll();

		// Transfert avec calcul des attributs derives en deux parties
		cout << "Transfer with computation of derived variables in two parts" << endl;
		bExport = true;
		attribute = testClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->SetUsed(bExport);
			attribute->SetLoaded(bExport);
			bExport = not bExport;
			testClass->GetNextAttribute(attribute);
		}
		testClass->Compile();
		cout << *testClass << endl;
		databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_HalfD1.txt"));
		database.ReadAll();
		databaseTarget.WriteAll(&database);
		database.DeleteAll();

		// Seconde partie du transfert
		bExport = false;
		attribute = testClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->SetUsed(bExport);
			attribute->SetLoaded(bExport);
			bExport = not bExport;
			testClass->GetNextAttribute(attribute);
		}
		testClass->Compile();
		cout << *testClass << endl;
		databaseTarget.SetDatabaseName(FileService::BuildFilePathName(sTmpDir, "database_HalfD2.txt"));
		database.ReadAll();
		databaseTarget.WriteAll(&database);
		database.DeleteAll();

		// Relecture de la seconde partie en demandant tous les champs
		cout << "Re-read of the second part, asking for all the fields except AttN2" << endl;
		attribute = testClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->SetUsed(true);
			attribute->SetLoaded(true);
			testClass->GetNextAttribute(attribute);
		}
		testClass->DeleteAttribute("AttN2");
		testClass->Compile();
		cout << *testClass << endl;
		cout << "\t(three warnings and five errors expected)" << endl;
		databaseTarget.ReadAll();
	}

	// Nettoyage
	database.DeleteAll();
	databaseTarget.DeleteAll();
	KWClassDomain::DeleteAllDomains();
}
