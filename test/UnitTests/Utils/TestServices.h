// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "FileService.h"
#include "gtest/gtest.h"
#include <fcntl.h>

#ifdef _WIN32
#include "sys\stat.h"
#endif

#ifndef STDOUT_FILENO
// Non defini sous Windows: file descriptor 1 == "stdout"
#define STDOUT_FILENO 1
#endif

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
//  ./results.ref/Integer_full.txt.
// Lors de la mise au point, le fichier Integer_full.txt est cree dans le repertoire
// results et le test echoue car il n'y a pas de fichier de reference. Il suffit
// de copier ce fichier dans le repertoire results.ref pour que le test reussisse.
// Pour que le test soit valide sur differentes machines, il faut si necessaire remplacer
// dans le fichier de reference les chemins propres a l'environnement par des tags, a savoir :
//  - le repertoire temporaire par @TMP_DIR@
//	- le repertoire qui contient le projet des tests unitaires par @ROOT_DIR@
// Cela concerne essentiellement les tests impliquant des fichiers temporaires.
//
// Le test est reussi lorsque les 2 fichiers sont identiques (a l'exception des
// lignes qui commencent par "SYS" qui peuvent etre differentes)
//
#define KHIOPS_TEST(test_suite, test_name, method_to_test)                                                             \
	TEST(test_suite, test_name)                                                                                    \
	{                                                                                                              \
		boolean bOk;                                                                                           \
		bOk = TestAndCompareResults(FileService::GetPathName(__FILE__), #test_suite, #test_name,               \
					    method_to_test);                                                           \
		EXPECT_TRUE(bOk);                                                                                      \
	}

// Idem KHIOPS_TEST pour les avec parametre
#define KHIOPS_TEST_P(test_suite, test_name, method_to_test)                                                           \
	TEST_P(test_suite, test_name)                                                                                  \
	{                                                                                                              \
		boolean bOk;                                                                                           \
		bOk = TestAndCompareResults(FileService::GetPathName(__FILE__), #test_suite, #test_name,               \
					    method_to_test);                                                           \
		EXPECT_TRUE(bOk);                                                                                      \
	}

// Comparaison de 2 fichiers ligne par ligne
// Si deux lignes contiennent le token 'SYS', elles sont comparees uniquement jusqu'a ce token: tout ce qui est derriere
// est ignore. Il y a egalement une tolerance pour le caractere separateur dans les chemins '\' qui est remplace par '/'
// Renvoie true si les 2 fichiers existent et sont identiques
boolean FileCompareForTest(const ALString& sFileNameReference, const ALString& sFileNameTest);

// Lance la methode de test et compare la sortie standard avec un fichier de reference
boolean TestAndCompareResults(const char* sTestPath, const char* test_suite, const char* test_name,
			      void (*method_to_test)());
