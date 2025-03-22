// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "Object.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Vector.h"
#include "TaskProgressionManager.h"

class TaskProgression;

/////////////////////////////////////////////////////////////////////////
// Classe TaskProgression
// Gestion de la progression d'une tache avec affichage de details sur la
// tache, de son taux d'avancement et possibilite de l'interrompre.
// L'utilisation de cette classe est documentee ci-dessous pour
// chaque groupe de methodes.
class TaskProgression : public Object
{
public:
	///////////////////////////////////////////////////////////
	// Gestion des etapes du suivi
	//
	// Ces methodes sont a appeler depuis l'endroit ou la tache
	// est lancee, typiquement depuis l'implementation d'une
	// action utilisateur dans une unite d'interface
	// Exemple:
	// {
	//    TaskProgression::SetTitle("Ma tache"):
	//    TaskProgression::Start();
	//    ExecuteMyTask();
	//    TaskProgression::Stop();
	// }

	// Titre
	static void SetTitle(const ALString& sValue);
	static const ALString& GetTitle();

	// Debut du suivi de progression
	// Le vrai depart est legerement differe pour mieux gerer les taches
	// tres courtes (qui n'ont pas besoin de suivi utilisateur)
	static void Start();

	// Test si progression en cours
	static boolean IsStarted();

	// Fin du suivi de progression
	static void Stop();

	////////////////////////////////////////////////////////////////
	// Gestion des niveaux de suivi des taches
	// Un niveau de suivi de tache est associe a une tache, et
	// permet la reutilisation du suivi de la tache associee.
	// Une tache faisant appel a des sous-taches reutilisera ainsi
	// leur suivi automatiquement
	//
	// Ces methodes sont a appeler dans l'implementation d'une tache,
	// le BeginTask et le EndTask encadrant tous les traitements
	// a l'interieur d'une tache.
	// Exemple:
	// {
	//    TaskProgression::BeginTask();
	//    MyTaskInstruction1();
	//    MyTaskInstruction2();
	//    ...
	//    MyTaskInstructionN();
	//    TaskProgression::EndTask();
	// }

	// Debut du suivi du tache, a appeler au debut d'une tache
	static void BeginTask();

	// Fin du suivi du tache, a appeler a la fin d'une tache
	// Reinitialisation des parametres generaux (DisplayedLevelNumber et MaxTaskTime)
	static void EndTask();

	// Indique si une tache a ete demaree
	static boolean IsInTask();

	////////////////////////////////////////////////////////////
	// Parametrages des libelles et du taux de progression
	// Les affichages sont bufferises de facon a eviter une
	// trop grande frequence d'affichage des messages
	// Tout suivi de progression doit se faire a l'interieur
	// d'un niveau de suivi, entre un BeginTask et un EndTask.
	//
	// Ces methodes sont appeler dans les instructions d'une tache.
	// Une utilisation typique est l'utilisation au cours d'une boucle.
	// Exemple:
	// {
	//    ...
	//    // Ma boucle batch de traitements
	//    TaskProgression::DisplayMainLabel("Long traitement de calcul");
	//	  for (i = 0; i < nTotalComputationNumber; i++)
	//    {
	//        MyComputation1();
	//        ...
	//
	//        // Suivi de la tache
	//        TaskProgression::DisplayLabel(sTmp + "Computation " + IntToString(i));
	//        TaskProgression::DisplayProgression(((i+1)*100)/nTotalComputationNumber);
	//        if (TaskProgression::IsInterruptionRequested())
	//           break;
	//    }
	//    ...
	// }

	// Test si une demande d'interruption est arrivee
	// Cette methode peut etre appelee meme hors des taches (sans effet dans ce cas)
	static boolean IsInterruptionRequested();

	// Parametrage d'un comportement ou les taches ne sont interruptible ou non (defaut: true)
	// Dans le cas non interruptible, IsInterruptionRequested renvoie false systematiquement
	static void SetInterruptible(boolean bValue);
	static boolean GetInterruptible();

	// Libelle principal
	static void DisplayMainLabel(const ALString& sValue);

	// Pourcentage de progression (entre 0 et 100)
	static void DisplayProgression(int nValue);

	// Libelle du traitement en cours
	static void DisplayLabel(const ALString& sValue);

	// Remise a vide des libelles MainLabel, Label et de la progression
	static void CleanLabels();

	// Test si un rafraichissement est necessaire
	// A chaque appel, un compteur est incremente, et on ne repond true qu'environ une fois sur 100
	// Permet de conditionner la fabrication des libelle a afficher et les test d'interruption
	// dans les boucle de traitement intensifs, pour limiter la charge de traitement d'avancement des taches
	static boolean IsRefreshNecessary();

	///////////////////////////////////////////////////////////
	// Parametrage avance de la gestion des taches
	// Methodes appelables hors suivi (avant un Start)

	// Nombre de niveaux de suivi des taches montres a l'utilisateur
	// Si une tache appelle une sous-tache, plusieurs niveaux de suivi
	// de taches seront utilises
	// On peut ici controler le nombre de niveaux de suivi visualises
	// (par defaut: 1)
	static void SetDisplayedLevelNumber(int nValue);
	static int GetDisplayedLevelNumber();

	// Gestion d'un delai maximum d'interruption (en secondes)
	// Permet de definir un delai au dela duquel la tache sera interrompue,
	// sans interraction utilisateur
	// Par defaut: 0 (signifie pas de limite)
	static void SetMaxTaskTime(double dValue);
	static double GetMaxTaskTime();

	// Gestion d'un mode silencieux (defaut: false)
	// Dans ce mode, le suivi de tache se fait sans aucun affichage utilisateur ni
	// gestion du log des message de progression
	static void SetSilentMode(boolean bValue);
	static boolean GetSilentMode();

	///////////////////////////////////////////////////////////
	// Parametrage du manager (par defaut: aucun)
	// La classe ne contient que des methodes globales. Par defaut, ces methodes
	// ne font rien. Un TaskProgressionManager doit etre parametre pour obtenir
	// une implementation effectives des fonctionnalites de suivi de progression
	// des taches. Cette "indirection" entre la classe TaskProgression et un
	// manager permet d'implementer les algorithmes en les rendant independant
	// de l'interface utilisateur.
	// Le TaskManager UITaskManager est automatiquement associe des qu'un objet d'interface est utilise.

	// Memoire: le manager appartient a l'appelant
	static void SetManager(TaskProgressionManager* manager);
	static TaskProgressionManager* GetManager();

	// Gestion d'un fichier de log des messages de progression
	//    Par defaut: aucun
	// Quand on precise un fichier de log, seuls les derniers messages de
	// progression sont enregistres regulierement
	// Cela permet de consulter l'etat d'avancement en ouvrant ce fichier de log
	static void SetTaskProgressionLogFileName(const ALString& sValue);
	static const ALString& GetTaskProgressionLogFileName();

	///////////////////////////////////////////////////////////////////
	// Methodes permettant de mettre au point la gestion des demandes
	// d'interruption, en indexant toutes les demandes d'interruption
	// et en permettant de forcer un interruption selon un index donne

	// Nombre total de demandes d'interruptions depuis le debut du programme
	static longint GetInterruptionRequestNumber();

	// Methode permetant de provoquer une interruption a un index donne
	// Par defaut, 0 signifie pas d'arret
	static void SetInterruptionRequestIndex(longint lIndex);
	static longint GetInterruptionRequestIndex();

	// Parametrage externe de l'index de demande d'interruption selon la valeur
	// de la variable d'environnement INTERRUPTION_REQUEST_INDEX
	// En appelant cette methode au debut du main d'un programme, on peut
	// facillement mettre au point un script de lancement du programme
	// pour tester systematiquement les demandes d'interruptions
	static void SetExternalInterruptionRequestIndex();

	/////////////////////
	// Methode de test
	// Batch de comptage, avec suivi de progression
	static void Test();

	// Methode avancee, permet de declencher l'interruption utilisateur
	// par programe
	static void ForceInterruptionRequested();

	///////////////////////////////////////////////////////////
	////// Implementation
protected:
	// Affichage de tout un niveau (MainLabel, Label, Progression)
	// avec rafraichissement egalement des niveau superieurs
	static void DisplayFullLevel();

	// Lancement differe du Start, pour eviter l'affichage du suivi si la tache
	// est trop courte
	static void DifferedStart();

	// Initialisation des variables de travail
	static void Initialize();

	// Etat du suivi
	static boolean bIsStarted;
	static boolean bSilentMode;

	// Duree max de la tache
	static double dMaxTaskTime;

	// Nombre de niveaux de suivi affiches
	static int nDisplayedLevelNumber;
	static int nMaxDisplayedLevelNumber;

	// Niveau de suivi courant
	static int nCurrentLevel;

	// Titre
	static ALString sTitle;

	// Date du dernier affichage global
	static clock_t tLastDisplayTime;

	// Fraicheur de l'affichage, permettant de controler la methode IsRefreshNecessary
	static longint lDisplayFreshness;

	// Memorisation des derniers affichages effectue par niveau, pour bufferisation
	static StringVector svLastDisplayedMainLabels;
	static IntVector ivLastDisplayedProgressions;
	static StringVector svLastDisplayedLabels;

	// Memorisation des derniers affichages demandes par niveau
	static StringVector svLastMainLabels;
	static IntVector ivLastProgressions;
	static StringVector svLastLabels;

	// Date de la derniere demande d'interruption
	static clock_t tLastInterruptionTest;
	static boolean bIsInterruptionRequested;

	// Nombre de demandes d'interruptions
	static longint lInterruptionRequestNumber;

	// Index de demande d'interruption
	static longint lInterruptionRequestIndex;

	// Indique si les taches sont interruptibles
	static boolean bInterruptible;

	// Gestion du lancement differe du suivi de progression
	static clock_t tStartRequested;
	static boolean bIsManagerStarted;

	// Delai minimal entre deux action consecutives
	static const double dMinElapsedTime;

	// Manager en cours d'utilisation
	static TaskProgressionManager* currentManager;

	// Manager de type fichier pour la memorisation (partielle) des messages d'avancement
	// Le currentFileManager est actif si un fichier est parametre
	static FileTaskProgressionManager* currentFileManager;
	static FileTaskProgressionManager fileManager;
};

inline boolean TaskProgression::IsInTask()
{
	return (nCurrentLevel >= 0);
}

inline boolean TaskProgression::IsRefreshNecessary()
{
	lDisplayFreshness++;
	return (lDisplayFreshness % 128) == 0;
}

inline void TaskProgression::SetInterruptible(boolean bValue)
{
	bInterruptible = bValue;
}

inline boolean TaskProgression::GetInterruptible()
{
	return bInterruptible;
}
