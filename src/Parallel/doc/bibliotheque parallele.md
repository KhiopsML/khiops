# Bibliothèque parallèle pour Khiops - Documentation utilisateur <!-- omit in toc -->

<div align="right">Bruno Guerraz  - Janvier 2023</div>


- [1. Introduction](#1-introduction)
- [2. Le framework MPI](#2-le-framework-mpi)
- [3. Architecture de la bibliothèque parallèle](#3-architecture-de-la-bibliothèque-parallèle)
  - [3.1. La classe PLParallelTask](#31-la-classe-plparalleltask)
    - [3.1.1. Le mode parallèle simulé](#311-le-mode-parallèle-simulé)
    - [3.1.2. Retour de fonctions](#312-retour-de-fonctions)
  - [3.2. Le cycle de vie des processus dans Khiops](#32-le-cycle-de-vie-des-processus-dans-khiops)
- [4. Les services de Norm et Learning](#4-les-services-de-norm-et-learning)
  - [4.1. Les messages](#41-les-messages)
  - [4.2. La progression](#42-la-progression)
  - [4.3. L'arrêt utilisateur](#43-larrêt-utilisateur)
  - [4.4. Les assertions](#44-les-assertions)
- [5. Hello World](#5-hello-world)
- [6. L’échange de données : les variables partagées](#6-léchange-de-données-les-variables-partagées)
  - [6.1. Les types simples](#61-les-types-simples)
  - [6.2. Les conteneurs](#62-les-conteneurs)
  - [6.3. Construction d’un nouveau type](#63-construction-dun-nouveau-type)
    - [6.3.1. Sérialisation et désérialisation](#631-sérialisation-et-désérialisation)
    - [6.3.2. Sérialisation des objets structurés](#632-sérialisation-des-objets-structurés)
- [7. Le calcul de Pi](#7-le-calcul-de-pi)
- [8. La gestion des ressources](#8-la-gestion-des-ressources)
- [9. Utilisation avancée](#9-utilisation-avancée)
  - [9.1. Les chunks](#91-les-chunks)
  - [9.2. Gestion des fichiers temporaires](#92-gestion-des-fichiers-temporaires)
  - [9.3. Mode Boost](#93-mode-boost)
  - [9.4. Mise en sommeil des esclaves](#94-mise-en-sommeil-des-esclaves)
  - [9.5. Affichage des numéros de ligne](#95-affichage-des-numéros-de-ligne)
  - [9.6. Calcul automatique de la taille du buffer](#96-calcul-automatique-de-la-taille-du-buffer)
  - [9.7. Système de log](#97-système-de-log)
  - [9.8. MemoryStatVisualizer](#98-memorystatvisualizer)
- [10. Liste des classes de PLParallelTask](#10-liste-des-classes-de-plparalleltask)
  - [10.1. Les classes principales](#101-les-classes-principales)
    - [10.1.1. Classe d’entrée de la bibliothèque parallèle](#1011-classe-dentrée-de-la-bibliothèque-parallèle)
    - [10.1.2. Les variables partagées](#1012-les-variables-partagées)
    - [10.1.3. Les classe de service](#1013-les-classe-de-service)
  - [10.2. Les classes d’exemples](#102-les-classes-dexemples)
  - [10.3. Les classes techniques](#103-les-classes-techniques)
  - [10.4. Les classes techniques MPI](#104-les-classes-techniques-mpi)
- [11. Pense-bête et recommandations](#11-pense-bête-et-recommandations)
  - [11.1. Le rôle des variables](#111-le-rôle-des-variables)
  - [11.2. Le constructeur de la classe](#112-le-constructeur-de-la-classe)
  - [11.3. La mise au point : en séquentiel, le test : en parallèle simulé](#113-la-mise-au-point-en-séquentiel-le-test-en-parallèle-simulé)
  - [11.4. Enregistrement des variables partagées](#114-enregistrement-des-variables-partagées)
  - [11.5. Enregistrement des nouvelles tâches](#115-enregistrement-des-nouvelles-tâches)


<div style="page-break-after: always;"></div>

# 1. Introduction

Ce document est le point d’entrée pour développer des applications parallèles en utilisant la bibliothèque PLParallelTask. Le développeur devra également s’appuyer sur les classes d’exemples auto-documentées contenues dans la bibliothèque. Nous ne présentons pas l’intégralité de l’API loin s’en faut, celle-ci étant abondamment documentée directement dans les headers de la bibliothèque.

# 2. Le framework MPI

La bibliothèque parallèle utilise MPI, MPI pour Message Passing Interface est une norme qui définit une bibliothèque de fonctions. Elle permet de faire des programmes parallèles par passage de messages entre processus. Comme dans tous les modèles de programmation à passage de messages :

- Chaque processus exécute un programme différent.
- Toutes les variables du programme sont privées et sont stockées dans la mémoire locale de chaque processus.
- Une variable est échangée entre 2 processus à travers l’appel d’une fonction.

MPI est un framework pérenne, stable et mature. En effet, la première version de la norme date de 1993 ; il y a eu 2 nouvelles versions depuis, en 1997 et en 2012. Chaque version ajoute des fonctionnalités et de nouveaux concepts mais l’API ne change pas. Un programme écrit en 1994 compile toujours avec les bibliothèques de MPI-3. MPI est implémentée en C, C++ et Fortran dans de nombreuses bibliothèques et sur la quasi-totalité des plateformes (machines multi-cœurs, cluster de machines et supercalculateurs massivement parallèles). Nous utilisons deux implémentations de MPI : MPICH pour Linux et MS-MPI pour Windows.
Dans tous les programmes MPI, ce sont plusieurs instances du même programme qui sont lancées simultanément. A chaque instance du programme, MPI assigne un rang différent. Un programme MPI typique commence par un test sur le rang afin de déterminer son rôle,  maître ou esclave.

```
si rang == 0
alors
    Implémentation du maître
sinon
    Implémentation de l’esclave
```

Un programme MPI doit être lancé avec la commande mpiexec suivi du nombre de processus désirés. Par exemple la commande mpiexec –n 5 myProg.exe lance 5 instances du programme myProg.

# 3. Architecture de la bibliothèque parallèle

La bibliothèque parallèle est intégrée dans l’ensemble des bibliothèques de Khiops (l’environnement Learning). Cette bibliothèque permet  de développer facilement de nouvelles fonctionnalités parallèles dans Khiops. De même, la parallélisation des fonctionnalités existantes ne devrait pas avoir beaucoup d’impact sur le code existant.

Tout au long du développement de cette bibliothèque, une attention particulière a été apportée à l’architecture de celle-ci et à son interaction avec le code actuel de Khiops : la bibliothèque de parallélisation est indépendante de Khiops dans le sens où il est très facile de débrancher la bibliothèque pour revenir à un programme séquentiel. C’est une garantie de stabilité et de maintenabilité de Khiops.

La bibliothèque parallèle est constituée de 2 bibliothèques : la bibliothèque destinée aux utilisateurs, `PLParallelTask` et la bibliothèque contenant les appels aux fonctions MPI, `PLMPI`. La bibliothèque `PLMPI` est une bibliothèque technique qui pourrait être remplacée par une bibliothèque utilisant un autre framework que MPI. A ces deux bibliothèques s’ajoute `PLSamples` qui contient plusieurs exemples à vocation didactique.

L’unique bibliothèque utilisateur est `PLParallelTask`. Elle contient la classe principale éponyme et un ensemble de classes utilitaires. Cette classe est le point d’entrée de la bibliothèque.

Le découpage en 2 bibliothèques n’est pas uniquement fonctionnel. On peut n’utiliser que la bibliothèque PLParallelTask, sans ajouter PLMPI et sans installer MPI. On pourra alors développer des programmes qui se conformeront au framework et qui s’exécuteront en séquentiel. L’ajout ultérieur de PLMPI et des bibliothèques MPI permettra d’exécuter exactement le même code mais cette fois en parallèle.

## 3.1. La classe PLParallelTask

Le point d’entrée pour créer une application parallèle est la classe `PLParallelTask`. Celle-ci offre un « framework » de programmation distribuée à travers l’utilisation de méthodes virtuelles utilisées par le maître et les esclaves. Il suffit de créer une classe qui hérite de celle-ci pour créer une application parallèle.

Les méthodes préfixées par `Master`  sont utilisées par le maître et les méthodes préfixées par `Slave`  sont utilisées par les esclaves.

Les méthodes utilisées par l’esclave sont les suivantes :

| Méthode         | Description                      |
| --------------- | -------------------------------- |
| SlaveInitialize | Initialisation de l’esclave      |
| SlaveProcess    | Traitement effectué en parallèle |
| SlaveFinalize   | Nettoyage de l’esclave           |

Les méthodes utilisées par le maître sont les suivantes:

| Méthode                | Description                                                                          |
| ---------------------- | ------------------------------------------------------------------------------------ |
| MasterInitialize       | Initialisation du programme maître                                                   |
| MasterPrepareTaskInput | Préparation  des données d’entrées nécessaires au traitement effectué par un esclave |
| MasterAggregateResults | Agrégation des résultats envoyés par un esclave                                      |
| MasterFinalize         | Nettoyage du maître                                                                  |

Le diagramme ci-dessous illustre la séquence d’appels de ces méthodes sur deux processus distincts.

![master-slave](./master-slaves.png#center)

La méthode `Run` exécute la tâche parallèle. Le développeur ne gère pas explicitement le lancement du maître et des esclaves : la méthode `Run` lance le maître mais les esclaves sont instanciés dynamiquement; Ceci est détaillé dans la section sur [le cycle de vie des processus](#32-le-cycle-de-vie-des-processus-dans-khiops). Le nombre d’esclaves est calculé automatiquement par le framework (Cf. section sur la [gestion des ressources](#8-la-gestion-des-ressources)).

Lorsqu’il n’y a pas assez de cœurs disponibles, le programme est lancé en séquentiel. Une tâche parallèle exécutée en séquentiel n’appelle aucune méthode MPI et n’utilise pas la couche de gestion du protocole maître/esclaves : elle aussi légère que possible. La séquence d’appel des méthodes dans le cas séquentiel est illustrée sur le diagramme ci-dessous.

![serial](./serial.png#center)

C’est le même code « utilisateur » qui est appelé en séquentiel et en parallèle. Cela permet de réaliser la mise au point et le débogage en séquentiel. Ceci est très confortable car le débogage d’un programme parallèle est très compliqué.

### 3.1.1. Le mode parallèle simulé

La bibliothèque propose en plus un mode « parallèle simulé », dans ce mode le programme est exécuté dans un seul processus. Contrairement au mode séquentiel, l’objectif n’est pas d’être le plus léger possible mais d’être le plus proche du code parallèle sans utilisé MPI. Pour exécuter le programme avec ce mode il faut ajouter l’instruction suivante au début du programme :
```cpp
PLParallelTask::SetParallelSimulated(true);
```
Si le programme s’exécute correctement dans le mode parallèle simulé, il s’exécutera correctement en parallèle.

### 3.1.2. Retour de fonctions

Les fonctions virtuelles à implémenter doivent renvoyer true si tout s’est bien déroulé et false sinon. Si l’une de ces fonctions renvoie false, alors la tâche parallèle va s’arrêter automatiquement et la méthode Run va également renvoyer false.

Dans les méthodes `SlaveFinalize(boolean &bProcessEndedCorrectly)` et  `MasterFinalize(boolean &bProcessEndedCorrectly)` le paramètre passé par référence est affecté à false si un problème a été rencontré, cela permet d’adapter le comportement de cette fonction.

## 3.2. Le cycle de vie des processus dans Khiops

Tous les traitements de Khiops ne sont pas parallélisés et dans certains algorithmes on peut avoir besoin de moins d’esclaves que le nombre d’instances du programme lancées au départ. C’est pourquoi les esclaves sont encapsulés dans un esclave « dormant » celui-ci ne fait qu’attendre l’ordre d’instanciation d’un esclave.

Pour résumer, il y a toujours plusieurs instances de MODL qui sont exécutées mais elles n’utilisent pas tout le temps la CPU. Le graphique suivant illustre un fonctionnement possible de Khiops en parallèle. Dans cette exemple,  MPI lance 9 processus en parallèle, il y a donc 1 maître et 8 esclaves. L’exécution de Khiops peut être découpée en 7 phases :

- La première phase est séquentielle : tous les esclaves sont en sommeil.
- La deuxième phase est parallèle, les 2 premiers esclaves sont instanciés par une tâche : ils travaillent. Tous les autres sont en sommeil.
- La phase 3 est séquentielle : comme pour la première phase, aucun esclave n’instancie de tâche.
- La phase 4 est parallèle, seul dernier esclave est en sommeil.
- Les phases 5 et 7 sont séquentielles.
- La phase 6 est parallèle.
Il est à noter que les esclaves en sommeil n’utilisent quasiment pas de CPU.

![time line](time-line.png#center)

Pour que les esclaves dormants puissent instancier une tâche à la demande du maître il faut préalablement déclarer toutes les tâches au début du programme. Un mécanisme d’administration est prévu à cet effet dans la classe PLParallelTask. Il est constitué de 3 méthodes :

```cpp
static void RegisterTask(PLParallelTask* plTask);
static PLParallelTask* LookupTask(const ALString& sName);
static void DeleteAllTasks();
```

La première consiste à enregistrer les taches parallèles. Une fois qu’une tâche est enregistrée, la deuxième méthode permet de renvoyer une instance de celle-ci à partir de son taskName. C’est ce mécanisme qui permet aux esclaves dormants d’instancier une tâche à la demande du maître (cette demande contenant le taskName).

La méthode `RegisterTask` doit être appelée pour toutes les tâches dans la méthode main du programme, comme c’est le cas dans le fichier MODL.cpp. Ainsi quand un esclave est réveillé pour une tâche décidée par le maître, il peut instancier un objet de la bonne classe pour converser avec le maître.

TODO dessin

# 4. Les services de Norm et Learning

Tous les mécanismes « génériques »  de Norm et Learning sont utilisables en parallèle et sont pour la plupart utilisables de la même façon qu’auparavant.

## 4.1. Les messages

Les messages utilisateurs s’utilisent de la même façon que dans un programme séquentiel. Les messages sont envoyés automatiquement des esclaves vers le maître qui les gère de façon centralisée. En revanche, les libelles utilisateurs associés aux objets ne sont pas transmis automatiquement entre le maître et les esclaves. Il faut donc faire très attention en implémentant la méthode GetObjectLabel(). En effet, dans la plupart des cas il faut passer par une variable partagée (Cf. la [section dédiée](#1012-les-variables-partagées)) pour que cette méthode ait le même comportement chez le maître et chez les esclaves.

## 4.2. La progression

La progression est traditionnellement mise à jour par la méthode DisplayProgression. Pour une application parallèle, celle-ci est mise à jour de 2 manières, l’une à gros grain, dans le maître et l’autre plus fine dans l’esclave.

Dans le maître, la méthode `MasterPrepareTaskInput(double &bTaskPercent)` prépare la tâche à effectuer par le prochain esclave en attente. L’utilisateur peut évaluer le pourcentage du travail total que représente la tâche en cours de préparation et l’affecter au paramètre passé par référence. A partir de cette information, le programme a une vue « macro » de sa progression.

Pour affiner cette progression, on peut utiliser la méthode standard `DisplayProgression` à l’intérieur de la méthode `SlaveProcess` pour indiquer le pourcentage d’avancement de la tâche en cours. Ce pourcentage est envoyé automatiquement au maître ; celui-ci agrège les progressions de tous les esclaves et peut ainsi affiner la progression globale. Si la fonction `DisplayProgression` n’est pas appelée dans `SlaveProcess`, alors la barre d’avancement se remplira par gros blocs à chaque fois qu’un esclave aura fini son traitement.

Le tableau ci-dessous donne des exemples d’avancement pour une tâche parallèle constituée de 2 sous-tâches dont l’une représente 60% de la tâche complète et l’autre 40%.

| % esclave 1 | % esclave 2 | Avancement du programme (en %) |
| ----------- | ----------- | ------------------------------ |
| $0$         | $1$         | $0*0,6+1*0,4=0,4$              |
| $50$        | $50$        | $50*0,6+50*0,4=50$             |
| $50$        | $100$       | $50*0,6+100*0,4=70$            |

## 4.3. L'arrêt utilisateur

Lorsque l’utilisateur d’un programme clique sur le bouton stop de la barre de progression, l’information est automatiquement transmise aux esclaves. C’est-à-dire que la méthode `IsInterruptionRequested()` renverra `true`, même si elle est appelée dans les méthodes des esclaves alors que l’interruption est détectée par le maître.

## 4.4. Les assertions

Le mécanisme des assertions fonctionne également en parallèle, pour faciliter le débogage, le rang de l’esclave est inséré dans le message.

# 5. Hello World

La classe `PEHelloWorldTask` donne un exemple excessivement simple d’un programme parallèle. Dans ce programme chaque esclave affiche son rang et l’itération (le nombre de fois où il est passé dans la méthode SlaveProcess). L’essentiel du programme est dans les méthodes :

- **SlaveProcess** : pour l’affichage de « hello world ».
- **SlaveInitialize** : pour l’initialisation du nombre d’itération par esclave.
- **MasterPrepareTaskInput** : pour la condition d’arrêt du traitement.

# 6. L’échange de données : les variables partagées

Les échanges de données entre le maître et les esclaves sont effectués par le biais des « variables partagées ». Les variables partagées sont gérées par des classes qui correspondent aux types simples de C++ et à quelques types plus avancés. On peut facilement construire de nouveaux types de variables partagées. Il y a 3 usages différents des variables partagées :

- les entrées des esclaves (input) : celles-ci sont envoyées par le maître aux esclaves après chaque appel de la méthode MasterPrepareTaskInput. Les esclaves reçoivent ces variables avant chaque appel SlaveProcess.
- les sorties des esclaves : c’est le résultat de la fonction SlaveProcess, elles sont envoyées par les esclaves après l’exécution de la méthode SlaveProcess(). Le maître les reçoit avant la méthode MasterAggregateResults.
- les constantes : le maître envoie ces variables aux esclaves une fois pour toute après MasterInitialize, les esclaves les reçoivent avant SlaveInitialize.

Pour utiliser une variable partagée, il faut la déclarer dans le constructeur de la classe. Suivant l’usage de la variable il y a 3 façons de déclarer une variable :

- `DeclareSharedParameter` : pour les constantes
- `DeclareTaskInput` : pour les entrées
- `DeclareTaskOutput` : pour les résultats

Les accès en écriture sont contrôlés par des assertions. La modification des variables partagées est autorisée dans les méthodes suivantes :

- SharedParameter :
  - MasterInitialize
- TaskInput :
  - MasterInitialize
  - MasterPrepareTaskInput
  - MasterAggregateResults
- TaskOutput :
  - SlaveInitialize
  - SlaveProcess

La figure suivante illustre les moments où sont échangées les variables partagées entre le maître et les esclaves suivant leur type.

![variable partagées](shared-variables.png#center)

## 6.1. Les types simples

La bibliothèque parallèle met à disposition les types simples  qui correspondent à ceux utilisés dans Norm et Learning :

|                  |          |
| ---------------- | -------- |
| PLShared_Boolean | boolean  |
| PLShared_Double  | double   |
| PLShared_Int     | int      |
| PLShared_LongInt | longint  |
| PLShared_String  | ALString |
| PLShared_Char    | Char     |

Toutes ces classes héritent de la classe virtuelle PLSharedVariable. A ces types simples s’ajoutent leurs conteneurs :

|                        |               |
| ---------------------- | ------------- |
| PLShared_StringVector  | StringVector  |
| PLShared_IntVector     | IntVector     |
| PLShared_LongintVector | LongintVector |
| PLShared_DoubleVector  | DoubleVector  |

Ces classes héritent de la classe `PLSharedObject`. Contrairement à la classe `PLSharedVariable`, on peut sérialiser des listes ou des tableaux d’objet qui implémentent la classe `PLSharedObject`. C’est l’objet du paragraphe suivant.

## 6.2. Les conteneurs

La plupart des conteneurs de Norm sont sérialisés à savoir :

|                           |                  |
| ------------------------- | ---------------- |
| PLShared_ObjectArray      | ObjectArray      |
| PLShared_ObjectDictionary | ObjectDictionary |
| PLShared_ObjectList       | ObjectList       |

Les objets contenus dans un conteneur partagé doivent impérativement implémenter la classe virtuelle PLSharedObject.

## 6.3. Construction d’un nouveau type

Par convention, toutes les variables partagées sont préfixées par `PLShared_` et ont les méthodes `SetValue` et `GetValue` pour avoir accès au type à sérialiser.

Une variable partagée hérite de la classe PLSharedObject, il faut implémenter les méthodes virtuelles suivantes :

- `Create` : Création d’un nouvel objet à sérialiser (nécessaire lors de la désérialisation)
- `DeserializeObject` : désérialisation
- `SerializeObject` : sérialisation

### 6.3.1. Sérialisation et désérialisation

La sérialisation et la dé-sérialisation  sont réalisées à l’aide de la classe PLSerializer. Cette classe offre un service de sérialisation et désérialisation pour tous les types simples. Elle doit être utilisée comme un système d'E/S classique :

- en écriture, on ajoute successivement des objets à sérialiser
- en lecture on désérialise successivement ces objets (dans l'ordre initial)

On peut facilement l'utiliser pour sérialiser des objets plus complexes.

Les méthodes `SerializeObject`  et `DeserializeObject`  de `PLSharedObject` ont en paramètre un pointeur sur un PLSerializer déjà ouvert. Il suffit donc d’écrire les attributs de la classe  dans la méthode SerializeObject  et de lire dans le même sens dans la méthode DeserializeObject via l’API de PLSerializer.

Par exemple pour une classe « City » avec les attributs name et population les méthodes de sérialisation seraient les suivantes :

```cpp
void PLSharedVariable_City::SerializeObject(PLSerializer* serializer)
{
    serializer->PutString(name);
    serializer->PutString(population);
}

void PLSharedVariable_City::DeserializeObject(PLSerializer* serializer)
{
    name = serializer->GetString();
    population = serializer->GetInt();
}
```

La classe `PLShared_SampleObject` donne un exemple complet pour construire un nouveau type de variable partagée.

### 6.3.2. Sérialisation des objets structurés

On souhaite maintenant sérialiser la classe Country qui a comme attribut name, population et capital. L’attribut capital est de type City, et on a déjà la sérialisation de cette classe dans `PLSharedVariable_City` (Cf. [paragraphe précédent](#631-sérialisation-et-désérialisation)). Pour ne pas dupliquer le code de la classe `PLSharedVariable_City` on utilise les méthodes `SerializeObject` et `DeserializeObject` qui permettent de réutiliser une classe sérialisée. Dans notre exemple, les méthodes de sérialisation seront les suivantes :

```cpp
void PLSharedVariable_Country::SerializeObject(PLSerializer* serializer)
{
KWShared_City shared_capital;

    serializer->PutString(name);
    serializer->PutString(population);

    // La serialisation de la capitale est sous-traitée
    shared_capital.AddToSerializer(serializer, capital);

}

void PLSharedVariable_Country::DeserializeObject(PLSerializer* serializer)
{
    KWShared_City shared_capital;

    name = serializer->GetString();
    population = serializer->GetInt();

    // La déserialisation de la capitale est sous-trairée
    shared_capital.GetFromSerializer(serializer, capital);
}
```

# 7. Le calcul de Pi

La classe `PEPiTask` est un exemple simple et très répandu en calcul parallèle, il s’agit d’estimer le nombre Pi par une intégrale calculée avec la méthode des trapèzes. Le programme séquentiel est le suivant :

```cpp
static double SerialPi()
{
 double sum = 0.0;
 double step = 1.0 / (double)num_steps;
 for (int i = 0; i < num_steps; i++)
 {
  double x = (i + 0.5) * step;
  sum = sum + 4.0 / (1.0 + x * x);
 }
 return step * sum;
}
```

Pour paralléliser ce programme on coupe l’itération en autant d’esclaves. Cette classe utilise des variables partagées (en constante, en entrée de l’esclave et en sortie de l’esclave) et la progression. C’est un exemple simple et complet.

# 8. La gestion des ressources

Le nombre d’esclaves qui peuvent être lancés en parallèle dépend des ressources disponibles sur la machine, et des ressources nécessaires à l’exécution du maître et de chaque esclave. En effet si le maître et les esclaves ont besoin chacun de 2 Go de mémoire vive pour s’exécuter correctement et si le système a 8 Go, on ne doit pas lancer plus de 3 esclaves même si le système à 200 cœurs.

La classe RMParallelResourceManager permet de calculer le nombre d’esclaves en leur allouant les ressources nécessaires à partir des exigences de l’utilisateur, des ressources du système et des ressources nécessaires à l’exécution de la tâche.

Le calcul est effectué automatiquement au lancement de la tâche, et le maître lance ainsi autant d’esclaves que possible tout en respectant toutes les contraintes (utilisateur, système, tâche).

Les ressources du système sont découvertes automatiquement (RAM, nombre de cœurs etc…). Les contraintes utilisateurs sont recensées dans la classe RMResourceConstraints. Elles sont toutes initialisées via l’interface utilisateur (fichier temporaire, utilisation de la mémoire vive, …). Les ressources nécessaires à la tâche sont à déclarer en implémentant la méthode virtuelle ComputeResourceRequirements de la classe PLParallelTask. On doit déclarer les exigences suivantes, pour le maître et les esclaves :

- La mémoire minimum : si le processus n’a pas au moins cette mémoire, il ne pourra pas s’exécuter.
- La mémoire maximum : le processus n’allouera jamais plus que cette mémoire, il n’en a pas besoin.
- L’espace disque minimum.
- L’espace disque maximum.

On doit également prévoir la mémoire utilisée par les variables partagées. Et pour finir, on peut choisir une politique d’allocation pour le disque et pour la mémoire. Suivant les politiques choisies, on donnera le maximum au maître, le maximum aux esclaves ou les ressources seront reparties équitablement. Les politiques par défaut privilégient les esclaves.

Par défaut, c’est-à-dire sans implémenter la méthode ComputeResourceRequirements, les minimums sont à 0 et les maximums à l’infini, c’est-à-dire que les tâches seront lancées avec tous les processus disponibles et les ressources seront réparties entre tous les esclaves.

Dans l’exemple suivant, les esclaves ont besoin chacun d’une taille de buffer comme mémoire vive et Ils n’ont pas besoin de plus. Le maître quant à lui n’a pas besoin de mémoire, en revanche il a besoin de tout le disque dur.

```cpp
void MyTask::ComputeResourceRequirements()
{
    // Exigences de la tache
    GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(BufferedFile::nDefaultBufferSize);
    GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(BufferedFile::nDefaultBufferSize);
    GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(0);

    // Politique d’allocation
    GetResourceRequirements()->SetDiskAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
 }
```

A l’intérieur de la tâche parallèle, les ressources allouées sont accessibles via les méthodes `PLParallelTask::GetSlaveResourceGrant()` et `PLParallelTask::GetMasterResourceGrant()`.

# 9. Utilisation avancée

Cette section liste des outils avancés offerts par la bibliothèque parallèle. La description détaillée de ces outils est disponible directement dans le header correspondant.

## 9.1. Les chunks

La classe `PLFileConcatenater` offre des services pour la concaténation et la suppression de fichiers découpés en chunks.

## 9.2. Gestion des fichiers temporaires

La classe PLParallelTask offre un mécanisme de suppression automatique des fichiers temporaires créés par les esclaves. Il s’agit des méthodes :

- `PLParallelTask::SlaveRegisterUniqueTmpFile`
- `PLParallelTask::SlaveDeleteRegisteredUniqueTmpFiles`

## 9.3. Mode Boost

Dans le cas où les esclaves effectuent un traitement très court dans le SlaveProcess, il peut-être utile d’optimiser le temps pris par le framework. C’est ce que permet le mode « boost ». Il est mis en œuvre avec la méthode `PLParallelTask::SetBoostMode`.

## 9.4. Mise en sommeil des esclaves

Lorsqu'un esclave est en sommeil il ne reçoit plus d'ordre de travail. Cela permet de réaliser une synchronisation de tous les esclaves et de s'assurer que tous les esclaves réalisent une action donnée. Les méthodes utilisées sont les suivantes :

- `PLParallelTask::SetSlaveAtRestAfterProcess`
- `PLParallelTask::SetAllSlavesAtWork`

## 9.5. Affichage des numéros de ligne

Lors de la lecture d’un fichier en parallèle, on peut émettre un message utilisateur qui contient  le numéro de la ligne en cours de lecture par un esclave en utilisant les méthodes PLParallelTask::SetLocalLineNumber et PLParallelTask::AddLocalError.

## 9.6. Calcul automatique de la taille du buffer

Pour éviter que les esclaves lisent le même fichier au même moment il est utile de faire varier la taille du buffer de lecture. La méthode PLParallelTask::ComputeStairBufferSize construit une taille de buffer différente suivant la position dans le fichier.

## 9.7. Système de log

Il y a 3 tracers intégrés dans la classe ParallelTask qui permettent de mettre au point ou déboguer.

- traces du protocole : ce tracer affiche dans la console chaque appel à une méthode du protocole maître-esclaves (MasterInitialize, SlaveProcess, etc.).
- traces des ressources : ce tracer affiche dans la console les ressources du système, les ressources nécessaires à la tâche et les ressources allouées à la tâche.
- Traces MPI : ce tracer affiche dans la console la plupart des appels aux méthodes MPI.

Ces tracers sont activables via les méthode SetTracerProtocolActive SetTracerResources et  SetTracerMPIActive. On peut également les activer à l’aide de la variable d’environnement.

## 9.8. MemoryStatVisualizer

La classe MemoryStatsManager de la bibliothèque Norm permet de collecter des statistiques pour auditer l'utilisation de la mémoire. La bibliothèque python MemoryStatsVisualizer permet d’agréger ces statistiques et de les afficher sous forme de graphique. Elle est disponible sur [git](https://gitlab.tech.orange/khiops_private/MemoryStatsVisualizer).

Ces 2 outils sont très pratiques pour analyser finement le comportement d’un programme parallèle car on peut visualiser le comportement de chaque processus, maître ou esclave.

# 10. Liste des classes de PLParallelTask

## 10.1. Les classes principales

### 10.1.1. Classe d’entrée de la bibliothèque parallèle

La création d’une application parallèle s’effectue en héritant de la classe
PLParallelTask. Celle-ci permet de construire une application parallèle.

### 10.1.2. Les variables partagées

Les variables partagées pour les types simples (fichier PLSharedVariable.h) :

- PLShared_Boolean
- PLShared_Double
- PLShared_Int
- PLShared_LongInt
- PLShared_String
- PLShared_Char

Les variables partagées pour les vecteurs (fichier PLSharedVector.h) :

- PLShared_StringVector
- PLShared_IntVector
- PLShared_LongintVector
- PLShared_DoubleVector
- PLShared_CharVector

Les variables partagées pour les tableaux (fichier PLSharedObject) :

- PLShared_ObjectArray
- PLShared_ObjectDictionary
- PLShared_ObjectList

Exemple de variable partagée :

- PLShared_SampleObject

### 10.1.3. Les classe de service

- PLSerializer : service de de/sérialisation pour tous les types simples.
- RMParallelResourceManager : Gestionnaire de ressources

## 10.2. Les classes d’exemples

Les classes ci-dessous sont des classes de test et d’exemple de la bibliothèque parallèle.  Elles offrent un excellent point d’entrée pour les utilisateurs de la bibliothèque. La liste ci-dessous et triée suivant la complexité de ces exemples.

- PEHelloWorldTask : exemple simplissime d’utilisation de la classe PLParallelTask
- PEProtocolTestTask : classe de test très légère, sans aucun traitement.
- PEPiTask: exemple d’utilisation de la classe PLParallelTask avec utilisation de variables partagées.
- PEPiView: interface utilisateur de la classe ParallelPi.
- PEPi : wrapper de la classe PEPiTask géré par la classe PEPiView
- PEIOParallelTestTask: exemple de lecture d’un fichier en parallèle
- PEFileSearchTask : grep sur un fichier, utilisation de la gestion des ressources.

| Classe               | Variables partagées |  I/O  | Gestion des ressources |
| -------------------- | :-----------------: | :---: | :--------------------: |
| PEHelloWorldTask     |                     |       |                        |
| PEProtocolTestTask   |          X          |       |                        |
| PEPiTask             |          X          |       |                        |
| PEIOParallelTestTask |          X          |   X   |                        |
| PEFileSearchTask     |          X          |   X   |           X            |

## 10.3. Les classes techniques

Ces classes sont techniques et ne doivent pas être utilisées.

- PLMaster : classe virtuelle qui définit l’interface d’un maître, elle est implémentée en MPI par la classe PLMasterMPI.
- PLMasterSequential : instanciation du maître en parallèle simulé.
- PLSlave: classe virtuelle qui définit l’interface d’un esclave, elle est implémentée en MPI par la classe PLSlaveMPI.
- PLSlaveState : classe qui permet de refléter l'état d'un esclave et de gérer ses messages utilisateurs.
- PLTaskDriver : Driver séquentiel des taches parallèles.
- PLTracer : service de trace pour le débogage MPI.
- PLSharedVariable : classe virtuelle ancêtre de toutes les classes de variables partagées.
- RMResourceSystem : Recense les ressources du système, utilise dans le gestionnaire de ressources.
- RMTaskResourceRequirement : Ressources demandées pour une tâche (fichier RMParallelResourceManager.h)
- RMTaskResourceGrant : Ressources allouées pour une tache (fichier RMParallelResourceManager.h)


## 10.4. Les classes techniques MPI

Les classes suivantes implémentent tous les appels à MPI :

- PLMPIMaster : maître MPI qui donne des ordres aux esclaves.
- PLMPIMessageHandler : Gestion des messages utilisateurs coté esclave.
- PLMPISlave : Esclave MPI, va de pair avec la classe PLMPIMaster.
- PLMPISlaveProgressionManager : Gestion de l’avancement et de l’arrêt utilisateur des esclaves.
- PLMPITaskDriver : drivers MPI pour les tâches parallèles.
- PLMPITracer : Traces dédiées aux messages MPI.

# 11. Pense-bête et recommandations

## 11.1. Le rôle des variables

Une seule classe définit le comportement du maître et des esclaves et ceci peut conduire à une mauvaise utilisation des attributs de la classe. En effet, une variable de classe affectée dans `MasterInitialize` n’aura pas la même valeur dans les méthodes dédiées à l’esclave (préfixée par `Slave`). C’est pourquoi il est indispensable d’identifier clairement le rôle de chaque variable de la classe. Il y a les variables de l’esclave et les variables du maître. Les variables de l’esclave doivent être utilisées uniquement dans les méthodes préfixées par `Slave` et les variables du maître doivent être utilisées uniquement dans les méthodes du maître (préfixées par `Master`).

Si une variable doit être utilisée à la fois par le maître et par l’esclave il faut utiliser les variables partagées. Pour les identifier clairement, le nom de celle-ci doit être préfixé par `input_`, `output_` ou `shared_` selon leur rôle.

## 11.2. Le constructeur de la classe

L’utilisation du constructeur de la classe n’est pas recommandée : celui-ci est appelé par le maître et par l’esclave. A moins d’un cas très particulier où le maître et l’esclave ont besoin d’initialiser la même structure, il est préférable d’utiliser SlaveInitialize pour initialiser toutes les variables de l’esclave et MasterInitialize pour initialiser toutes les variables du maître.

## 11.3. La mise au point : en séquentiel, le test : en parallèle simulé

La bibliothèque parallèle permet avec le même code source d’exécuter le programme en parallèle et en séquentiel. Il est extrêmement recommander de mettre au point et de tester le programme en séquentiel (avec une utilisation standard du debugger).

Il ne faut pas oublier le mode parallèle simulé qui permet de tester le programme en étant le plus proche possible du parallèle tout en restant en séquentiel. Dans ce mode, le maître et chaque esclave sont des objets différents et il y a envoi de message et sérialisation entre le maître et les esclaves, comme en parallèle.

==Si le programme se comporte correctement en parallèle simulé il se comportera correctement en parallèle.==

## 11.4. Enregistrement des variables partagées

Il ne suffit pas de déclarer une variable partagée pour qu’elle soit effectivement partagée entre le maître et les esclaves, il ne faut pas oublier de l’enregistrer dans la méthode RegisterSharedVariables() avec l’une des méthodes `DeclareSharedParameter`, `DeclareTaskInput` ou `DeclareTaskOutput`.

## 11.5. Enregistrement des nouvelles tâches

Pour que Khiops exécute une tâche parallèle il faut l’enregistrer (dans les fichiers MODL.cpp et Test.cpp) avec la méthode `PLParallelTask::RegisterTask`.