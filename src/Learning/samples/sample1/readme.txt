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

1. apprendre � manipuler les dictionnaires
  - lire l'API des classes KWClassDomain, KWClass
  - �tendre la m�thode ShowClass:
    - sauvegader le domaine de classe courante dans un nouveau fichier avec pr�fixe "S"
      (cf. class FileService de Norm pour manipuler les path, prefixe", suffixe...)
    - cr�er un nouveau domaine en clonant la classe avec un pr�fixe N_, et sauvegarder
      dans un fichier de pr�fixe N_
    - cloner la classe avec un pr�fixe C_ dans le m�me domaine, et sauvegarder le tout
      dans un fichier de pr�fixe C_
 
2. apprendre � manipuler les base de donn�es
  - lire l'API des classes KWDatabase, KWSTDatabase, KWMTDatabase,
    KWSTDatabaseTextFile, KWMTDatabaseTextFile
  - �tendre la m�thode ShowObject (ou cr�er une nouvelle m�thode)
    - utiliser deux variables locales 
       KWSTDatabaseTextFile stDatabase;
       KWMTDatabaseTextFile mtDatabase;
    - initialiser la version MT ou ST � partir de GetLearningProblem()->GetTrainDatabase()
      - initialiser la bonne version (st ou mt) selon les caract�ristiques de
        GetLearningProblem()->GetTrainDatabase(), en initialisant correctement tout ce qui est n�cessaire
      - r�impl�menter la m�thode de lecture de la base en utilisant ces variables
        locales correctement initialis�e, plut�t que GetLearningProblem()->GetTrainDatabase()
   - ajout d'une nouvelle m�thode SaveDatabase
      - sauvegarde du ou des fichiers de la base (ou mt) dans des fichiers pr�fix�s par S_

3. apprendre � modifier un dictionnaire
  - lire l'API des classes KWAttribute, KWDerivationRule, KWDrivationRuleOperand
    et de la r�gle de d�rivation Sum (classe KWDRSSum)
  - ajouter une nouvelle variable num�rique de nom SumVariables, qui est calcul�e comme la somme
    de toutes les variables num�riques de la classe en cours
  - faire un Check de la classe modifier, la compiler, et la sauvegarder dans
    un fichier de pr�fixe S_
    
4. apprendre � manipuler les KWObject
  - lire l'API de KWObject
  - faire une variante de la m�thode ShowObject, qui calcule toutes les moyennes des variables
    num�riques et les affiche dans la console (ShowSimpleMessage)
 
