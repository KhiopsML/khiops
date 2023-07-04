Test de Khiops
==============

LearningTest: creation en mars 2009
Automatisation des tests du logiciel Khiops
Versions synchronisees avec les versions majeures de Khiops
Historisation des versions par la commande MakeLearningTestVersion


Procédure pour effectuer les tests de Khiops sur un autre environement
----------------------------------------------------------------------
Installation de LearningTest sur une nouvelle machine
- copier l'arborescence LearningTest
- personnalisation de l'environnement par un fichier de config learning_test.config dans le répertoire LearningTest\cmd\python
    - voir LearningTest\cmd\python\learning_test_env.py pour la documentation sur le contenun de ce fichier de config
- Installer python
- mettre python dans le path

Utilisation de LearningTest
- ouvrir un shell
- lancer une commande se trouvant dans learningTest\cmd
- lancer les tests, par exemple
  - TestKhiops r Standard Adult
  - TestKhiops r Standard
  - TestAll r
- analyser les résultats, par exemple
	- ApplyCommand errors TestKhiops\Standard, pour avoir une synthése des erreurs/warning sur les tests de TestKhiops\Standard
	- ApplyCommandAll errors, pour la même commande sur tous les tests

Principales commandes
  - helpOptions: doc sur différentes options paramètrables par variable d'environnement
  - testKhiops [version] [testName] ([subTestName]): lance un test sur un répertoire ou une arborescence de test, sous le directory TestKhiops
     - version peut être:
       - "nul" pour ne faire que les comparaisons de résultats
       - "d" ou "r" pour la version de debug ou release de l'environnement de développement
       - un exe se trouvant dans LearningTest\cmd\modl\<MODL>.<version>[.exe]
       - un exe dont la path complet est <version>
  - TestCoclustering: idem pour Coclustering
  - TestKNI: idem pour KNI
  - testAll: pour lancer tous les tests
  - applyCommand [command] [root_path] ([dir_name]): pour exécuter une commande (un service) sur un ou plusieurs jeux de tests
  - applyCommandAll: pour exécuter une commande sur tous les jeux de test
Les commande lancées sans argument sont auto-documentées


Procédure de test
-----------------
Repertoires:
  - doc : documentation
  - cmd : fichier de commandes pour la gestion des tests
  - datasets : un repertoire par jeu de données, comportant une fichier dictionnaire .kdic et un fichier de donnéee .txt
  - MTdatasets : un repertoire par jeu de données multi-table, comportant une fichier dictionnaire .kdic et un fichier de donnéee .txt par table
  - TextDatasets : un repertoire par jeu de données, comportant une fichier dictionnaire .kdic et un fichier de donnéee .txt
  - TestKhiops
  	- Standard: fonctionnalités de base
  	- Classification: tests de classification
  	- Regression: tests de regression
  	- SideEffects: tests d'effets de bord
  	- ...
  - TestCoclustering
	  - Standard: fonctionnalités de base
  	- Bugs: jeux de tests éaborés pour reproduire des bugs et vérifier leur correction
  - TestKNITransfer:
  	- Standard: fonctionnalités de base
  	- MultiTables: test multi-tables
  	- ...

Les sous-repertoires préfixés par y_ ou z_ (ex: z_Work) sont des répertoire de test temporaires.

Dans chaque répertoire de test (par exemple: Classification/Iris)
 - un fichier de scénario (test.prm)
 - un sous-répertoire results, contenant les fi chiers produits par le scenario
 - un sous-répertoire results.ref, contenant la version de référence de ces fichiers

Les fichiers de scénario test.prm doivent étre écrits de façon indépendante de la localisation de LearningTest, en modifiant les paths des fichiers concernés, qui doivent étre relatifs é l'arborescence LearningTest, avec un syntaxe de type linux.
Exemple:
 - "./SNB_Modeling.kdic" pour accéder à un dictionnaire spécifique local au répertoire de test
    - hormis les jeux de données définis dans les arborescences racines de type LearningTest/dataset, les jeux de données peuvent avoir des dictionnaires ou des données spécifique par répertoire de test
 - "../../../datasets/Adult/Adult.txt" pour accéder à un fichier de données d'un dataset
 - "./results/T_Adult.txt" pour un résultat dans sous-répertoire des résultats

Ceci est automatisé par les fichiers de commandes se trouvant dans le répertoire cmd.
- testKhiops lance un test sur un répertoire ou une arborescence de test, sous le directory TestKhiops
  - testKhiops [version] [Test tree dir]
- testCoclustering lance un test sur un répertoire ou une arborescence de test, sous le directory TestCoclustering
- applyCommand lance des commandes un répertoire ou une arborescence de test
    - utititaire generique: cf script python appele
    - lancer sans argument pour avoir la liste des commandes possibles
    - principales commandes: errors (synthése des erreur et warning), logs (logs détaillés des erreurs)
- testAll lance les tests sur toutes les arborescences de test
- testAll64bits lance les tests sur toutes les arborescences de test en mode 64 bits
- applyCommandAll lance des commandes toutes les arborescences de test

