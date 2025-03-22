Sample1: Exemple d'utilisation basique des composants de la librairie LearningEnv
=================================================================================

Fonctionnalites:
	Edition d'un probleme d'apprentissage
	Affichage des attributs d'un dictionnaire
	Affichage des valeurs d'une instance d'une base de donnees
	Calcul des statistiques descriptives en utilisant la methode EqualFrequency

Fichiers sources:	
	test.cpp			Lancement de l'application
	test.h
	SampleOneLearningProject.cpp		Lancement du projet
	SampleOneLearningProject.h
	SampleOneLearningProblem.cpp		Gestion du probleme d'analyse descriptive
	SampleOneLearningProblem.h
	SampleOneLearningProblemView.cpp	Vue sur le probleme d'analyse descriptive
	SampleOneLearningProblemView.h
		
Jeu d'essai avec la base Iris: voir Data/readme.txt

Installation:
	Creer un projet utilisant Norm et LearningEnv 
	Compiler les fichiers sources de l'exemple

Utilisation: voir Data/readme.txt

=====================================================
Exemples d'excercices additionnels:

1. apprendre à manipuler les dictionnaires
  - lire l'API des classes KWClassDomain, KWClass
  - étendre la méthode ShowClass:
    - sauvegader le domaine de classe courante dans un nouveau fichier avec préfixe "S"
      (cf. class FileService de Norm pour manipuler les path, prefixe", suffixe...)
    - créer un nouveau domaine en clonant la classe avec un préfixe N_, et sauvegarder
      dans un fichier de préfixe N_
    - cloner la classe avec un préfixe C_ dans le même domaine, et sauvegarder le tout
      dans un fichier de préfixe C_
 
2. apprendre à manipuler les base de données
  - lire l'API des classes KWDatabase, KWSTDatabase, KWMTDatabase,
    KWSTDatabaseTextFile, KWMTDatabaseTextFile
  - étendre la méthode ShowObject (ou créer une nouvelle méthode)
    - utiliser deux variables locales 
       KWSTDatabaseTextFile stDatabase;
       KWMTDatabaseTextFile mtDatabase;
    - initialiser la version MT ou ST à partir de GetLearningProblem()->GetTrainDatabase()
      - initialiser la bonne version (st ou mt) selon les caractéristiques de
        GetLearningProblem()->GetTrainDatabase(), en initialisant correctement tout ce qui est nécessaire
      - réimplémenter la méthode de lecture de la base en utilisant ces variables
        locales correctement initialisée, plutôt que GetLearningProblem()->GetTrainDatabase()
   - ajout d'une nouvelle méthode SaveDatabase
      - sauvegarde du ou des fichiers de la base (ou mt) dans des fichiers préfixés par S_

3. apprendre à modifier un dictionnaire
  - lire l'API des classes KWAttribute, KWDerivationRule, KWDrivationRuleOperand
    et de la règle de dérivation Sum (classe KWDRSSum)
  - ajouter une nouvelle variable numérique de nom SumVariables, qui est calculée comme la somme
    de toutes les variables numériques de la classe en cours
  - faire un Check de la classe modifier, la compiler, et la sauvegarder dans
    un fichier de préfixe S_
    
4. apprendre à manipuler les KWObject
  - lire l'API de KWObject
  - faire une variante de la méthode ShowObject, qui calcule toutes les moyennes des variables
    numériques et les affiche dans la console (ShowSimpleMessage)
 
