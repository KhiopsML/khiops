// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "FileService.h"
#include "gtest/gtest.h"

// clang-format off

// Macro qui facilite l'ecriture d'un test unitaire lorsqu'on a deja une methode
// de test qui ecrit ses resultats dans la sortie standard.
//
// les parametres sont les suivants :
// - test_suite : nom de la suite de tests (CF. GoogleTest)
// - test_name : nom du test (CF. GoogleTest)
// - method_to_test : la methode de test a executer
// Par exemple, pour la methode static void Integer::Test(); de la classe
// Integer, on aura
//
//      KHIOPS_TEST(Integer, full, Integer::Test())
//
// Cet appel a la macro va lancer la methode Integer::Test() en redirigeant la
// sortie standard vers le fichier ./results/Integer_full.txt et ensuite
// comparer ce fichier avec le fichier de reference
//  ./results.ref/Integer_full.txt. Ce fichier doit avoir ete cree par ailleurs
//  lors de la mise au point de la
// methode Integer::Test().
//
// Le test est reussit lorsque les 2 fichiers sont identiques (a l'exception des
// lignes qui commencent par "SYS" qui peuvent etre differentes)
//
#define KHIOPS_TEST(test_suite, test_name, method_to_test)                                 \
        TEST(test_suite, test_name)                                                        \
        {                                                                                  \
                                                                                           \
                ALString sTestFileName;                                                    \
                boolean bOk;                                                               \
                ALString sTmp;                                                             \
                ALString sTestPath;                                                        \
                ALString sFileName;                                                        \
                ofstream fileStream;                                                       \
                                                                                           \
                /* Nommmage du ficher de sortie d'apres test suit et test name de          \
                 * googleTest*/                                                            \
                sFileName = sTmp + #test_suite + "_" + #test_name + ".txt";                \
                                                                                           \
                /* Le chemin des repertoires results et results.ref est deduit du chemin   \
                 * du fichier cpp qui appelle cette macro */                               \
                sTestPath = FileService::GetPathName(__FILE__);                            \
                                                                                           \
                /*Creation du repertoire results si necessaire */                          \
                if (not FileService::DirExists(sTestPath + "results"))                     \
                        FileService::CreateNewDirectory(sTestPath + "results");            \
                sTestFileName =                                                            \
                    sTestPath + "results" + FileService::GetFileSeparator() + sFileName;   \
                                                                                           \
                /* Sauvegarde du buffer associe a cout */                                  \
                streambuf* coutBuf = std::cout.rdbuf();                                    \
                                                                                           \
                /* Redirection de cout vers le stream dedie au batch */                    \
                fileStream.open(sTestFileName);                                            \
                cout.rdbuf(fileStream.rdbuf());                                            \
                                                                                           \
                /* Lancement de la methode de test de la classe */                         \
                method_to_test;                                                            \
                                                                                           \
                /* On restitue cout dans son etat initial */                               \
                cout.rdbuf(coutBuf);                                                       \
                fileStream.close();                                                        \
                                                                                           \
                /* comparaison du fichier issu de la sortie standrd avec le fichier de     \
                 * reference */                                                            \
                bOk = FileCompareForTest(sTestFileName,                                    \
                                         sTestPath + "results.ref" +                       \
                                             FileService::GetFileSeparator() + sFileName); \
                                                                                           \
                EXPECT_TRUE(bOk);                                                          \
        }

// Idem KHIOPS_TEST pour les avec parametre
#define KHIOPS_TEST_P(test_suite, test_name, method_to_test)                               \
        TEST_P(test_suite, test_name)                                                      \
        {                                                                                  \
                                                                                           \
                ALString sTestFileName;                                                    \
                boolean bOk;                                                               \
                ALString sTmp;                                                             \
                ALString sTestPath;                                                        \
                ALString sFileName;                                                        \
                ofstream fileStream;                                                       \
                                                                                           \
                /* Nommmage du ficher de sortie d'apres test suit et test name de          \
                 * googleTest*/                                                            \
                sFileName = sTmp + #test_suite + "_" + #test_name + ".txt";                \
                                                                                           \
                /* Le chemin des repertoires results et results.ref est deduit du chemin   \
                 * du fichier cpp qui appelle cette macro */                               \
                sTestPath = FileService::GetPathName(__FILE__);                            \
                                                                                           \
                /*Creation du repertoire results si necessaire */                          \
                if (not FileService::DirExists(sTestPath + "results"))                     \
                        FileService::CreateNewDirectory(sTestPath + "results");            \
                sTestFileName =                                                            \
                    sTestPath + "results" + FileService::GetFileSeparator() + sFileName;   \
                                                                                           \
                /* Sauvegarde du buffer associe a cout */                                  \
                streambuf* coutBuf = std::cout.rdbuf();                                    \
                                                                                           \
                /* Redirection de cout vers le stream dedie au batch */                    \
                fileStream.open(sTestFileName);                                            \
                cout.rdbuf(fileStream.rdbuf());                                            \
                                                                                           \
                /* Lancement de la methode de test de la classe */                         \
                method_to_test;                                                            \
                                                                                           \
                /* On restitue cout dans son etat initial */                               \
                cout.rdbuf(coutBuf);                                                       \
                fileStream.close();                                                        \
                                                                                           \
                /* comparaison du fichier issu de la sortie standrd avec le fichier de     \
                 * reference */                                                            \
                bOk = FileCompareForTest(sTestFileName,                                    \
                                         sTestPath + "results.ref" +                       \
                                             FileService::GetFileSeparator() + sFileName); \
                                                                                           \
                EXPECT_TRUE(bOk);                                                          \
        }
// clang-format on

// Comparaison de 2 fichiers ligne par ligne
// Les fichiers peuvent differer pour les lignes qui commencent par 'SYS'
// Renvoie true si les 2 fichiers existent et sont identiques
inline boolean FileCompareForTest(const ALString& sFileName1, const ALString& sFileName2)
{
	FILE* file1;
	FILE* file2;
	boolean bOk1;
	boolean bOk2;
	char c1;
	char c2;
	boolean bSame = true;
	const char sys[] = "SYS";
	int nLineIndex;
	const int sizeMax = 512;
	char line1[sizeMax];
	char line2[sizeMax];

	if (not FileService::FileExists(sFileName1))
	{
		cout << "File " << sFileName1 << " is missing" << endl;
		return false;
	}

	if (not FileService::FileExists(sFileName2))
	{
		cout << "File " << sFileName2 << " is missing" << endl;
		return false;
	}

	// Ouverture des fichiers
	bOk1 = FileService::OpenInputBinaryFile(sFileName1, file1);
	if (bOk1)
	{
		bOk2 = FileService::OpenInputBinaryFile(sFileName2, file2);
		if (not bOk2)
		{
			fclose(file1);
			return false;
		}
	}

	// Comparaison ligne par ligne
	if (bOk1 and bOk2)
	{
		nLineIndex = 0;
		while (fgets(line1, sizeof(line1), file1) != NULL)
		{
			nLineIndex++;
			if (fgets(line2, sizeof(line2), file2) == NULL)
			{
				bSame = false;
				break;
			}

			// Si les 2 lignes sont differentes et qu'elles ne commencent pas toutes
			// les 2 par SYS, les fichiers sont differents
			if (not(memcmp(line1, sys, strlen(sys) - 1) == 0 and
				memcmp(line2, sys, strlen(sys) - 1) == 0) and
			    (strcmp(line1, line2) != 0))
			{
				bSame = false;
				break;
			}
		}

		if (not bSame)
			cout << "error at line " << nLineIndex << endl;
	}

	// Fermeture des fichiers
	if (file1 != NULL)
		fclose(file1);
	if (file2 != NULL)
		fclose(file2);
	return bSame;
}