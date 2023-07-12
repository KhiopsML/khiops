// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "FileService.h"
#include "gtest/gtest.h"
#include <fcntl.h>

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
//  lors de la mise au point de la methode Integer::Test().
//
// Le test est reussi lorsque les 2 fichiers sont identiques (a l'exception des
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
                FILE *stream;                                                              \
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
                       /* Redirection de cout vers le stream dedie au batch */             \
                int fdInit = dup(STDOUT_FILENO);                                           \
	        int fd = open(sTestFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);         \
	        dup2(fd,STDOUT_FILENO);                                                    \
	        close(fd);                                                                 \
                                                                                           \
                                                                                           \
                                                                                           \
                                                                                           \
                /* Lancement de la methode de test de la classe */                         \
                method_to_test;                                                            \
                   /* On restitue cout dans son etat initial */                            \
               	dup2(fdInit, STDOUT_FILENO);                                               \
	        close(fdInit);                                                             \
                                                                                           \
                /* comparaison du fichier issu de la sortie standrd avec le fichier de     \
                 * reference */                                                            \
                bOk = FileCompareForTest(                                                  \
                   sTestPath + "results.ref" + FileService::GetFileSeparator() + sFileName \
                   ,sTestFileName                                                          \
                                         );                                                \
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
                FILE *stream;                                                              \
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
                       /* Redirection de cout vers le stream dedie au batch */             \
                int fdInit = dup(STDOUT_FILENO);                                           \
	        int fd = open(sTestFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);         \
	        dup2(fd,STDOUT_FILENO);                                                    \
	        close(fd);                                                                 \
                                                                                           \
                                                                                           \
                                                                                           \
                                                                                           \
                /* Lancement de la methode de test de la classe */                         \
                method_to_test;                                                            \
                   /* On restitue cout dans son etat initial */                            \
               	dup2(fdInit, STDOUT_FILENO);                                               \
	        close(fdInit);                                                             \
                                                                                           \
                /* comparaison du fichier issu de la sortie standrd avec le fichier de     \
                 * reference */                                                            \
                bOk = FileCompareForTest(                                                  \
                   sTestPath + "results.ref" + FileService::GetFileSeparator() + sFileName \
                   ,sTestFileName                                                          \
                                         );                                                \
                                                                                           \
                EXPECT_TRUE(bOk);                                                          \
        }

// clang-format on

// Comparaison de 2 fichiers ligne par ligne
// Les fichiers peuvent differer pour les lignes qui commencent par 'SYS'
// Renvoie true si les 2 fichiers existent et sont identiques
boolean FileCompareForTest(const ALString& sFileName1, const ALString& sFileName2);