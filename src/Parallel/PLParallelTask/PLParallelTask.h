// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "TaskProgression.h"
#include "InputBufferedFile.h"
#include "PLSharedVariable.h"
#include "PLSharedObject.h"
#include "PLSharedVector.h"
#include "PLShared_ResourceRequirement.h"
#include "PLSlaveState.h"
#include "PLTaskDriver.h"
#include "PLTracer.h"
#include "RMParallelResourceManager.h"
#include "RMParallelResourceDriver.h"
#include "RMTaskResourceGrant.h"
#include "PLIncrementalStats.h"
#include "PLRemoteFileService.h"
#include "PLErrorWithIndex.h"
#include "MemoryStatsManager.h"

class PLTaskDriver;
class PLMaster;
class PLSlaveSequential;
class PLMasterSequential;
class RMTaskResourceRequirement;

/////////////////////////////////////////////////////////////////////////////
// Classe PLParallelTask
// Classe de base des programmes paralleles. Elle permet d'instancier un programme
// maitre, un programme esclave, ou un programme sequentiel
// C'est une classe virtuelle qui doit etre implementee pour specifier :
// - le traitement a effectuer par les esclaves (methode  SlaveProcess),
// - L'initialisation des esclaves (SlaveInitialize) leur nettoyage (SlaveFinalize)
// - L'initialisation du maitre (MasterInitialize) son nettoyage (MasterFinalize)
// - Le contenu du travail a effectue pour chaque esclave (MasterPrepareTaskInput)
// - L'aggregation des resultats envoye par les esclaves au maitre (MasterAggregateResults)
//
// Les classes PEHelloWordTask et PEPiTask sont des exemples tres simple d'implementation.
//
class PLParallelTask : public Object
{
public:
	// Constructeur
	PLParallelTask();
	~PLParallelTask();

	// Retourne true quand la tache a ete executee (en echec ou non)
	boolean IsJobDone() const;

	// Renvoie true si la tache s'est bien terminee
	// et qu'il n'y a pas eu d'interruption utilisateur
	boolean IsJobSuccessful() const;

	// Renvoie true si il y a eu une interruption utilisateur
	// et que la tache s'est deroulee sans erreur
	boolean IsTaskInterruptedByUser() const;

	// Duree totale du job
	double GetJobElapsedTime() const;

	// Duree totale passe dans le master, et dans ses methodes Initialize et Finalize
	double GetMasterElapsedTime() const;
	double GetMasterInitializeElapsedTime() const;
	double GetMasterFinalizeElapsedTime() const;

	// Met tous les indicateurs de fin de traitement dans leur etat initial (IsJobSuccessful, IsJobDone, etc...)
	// Ne peut pas etre utilise pendant l'execution de la tache
	void CleanJobResults();

	// Renvoie true si l'execution en parallele est possible. C'est le cas si les conditions suivantes sont reunies
	// :
	//	- le driver est parallele
	//	- le nombre de processeurs de la machine est superieur a 2
	//	- le nombre de processus lances par MPI est superieur a 2
	static boolean IsParallelModeAvailable();

	// Acces a la version. Celle-ci doit etre renseignee avec la version de Learning
	// Il y a une verification de la version entre le smaitre et les differents esclaves.
	static void SetVersion(const ALString& sVersion);
	static ALString& GetVersion();

	// Choix du driver d'execution (driver sequentiel par defaut)
	static void SetDriver(PLTaskDriver* driver);
	static PLTaskDriver* GetDriver();

	///////////////////////////////////////////////
	// Gestion du mode parallele simule
	// Execution du programme en sequentiel, tout en simulant le fonctionnement maitre/esclaves

	// Simulation du parallelisme sur une seule machine
	static void SetParallelSimulated(boolean bTestOn);
	static boolean GetParallelSimulated();

	// Nombre maximum d'esclaves lance en mode parallele simule (par defaut a 8)
	static void SetSimulatedSlaveNumber(int nSlaveNumber);
	static int GetSimulatedSlaveNumber();

	///////////////////////////////////////////////
	// Acces aux exigences

	// Acces aux exigences de la tache en termes de ressources
	// Pour modification (calcul des ressources avant le lancement de la tache uniquement)
	// Les ressources sont reinitialisees a la fin du run
	RMTaskResourceRequirement* GetResourceRequirements() const;

	// Acces aux exigences de la tache en termes de ressources
	// Pour consultation (dans les methodes du maitre uniquement)
	const RMTaskResourceRequirement* GetConstResourceRequirements() const;

	///////////////////////////////////////////////
	// Gestion des libelles

	// Libelle de la tache: (par defaut: GetTaskName(), sauf si GetTaskUserLabel() non vide)
	const ALString GetTaskLabel() const;

	// Personnalisation du libelle de la tache (par defaut: vide)
	const ALString& GetTaskUserLabel() const;
	void SetTaskUserLabel(const ALString& sValue);

	// Libelle de la classe (par defaut: GetTaskLabel)
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////
	// Administration des objets PLParallelTask

	// Enregistrement dans la base de taches
	// Il ne doit pas y avoir deux taches enregistrees avec la meme signature
	// Memoire: les taches enregistrees sont gerees par l'appele
	static void RegisterTask(PLParallelTask* plTask);

	// Recherche par clef
	// Retourne NULL si absent
	static PLParallelTask* LookupTask(const ALString& sSignature);

	// Destruction de toutes les taches enregistrees
	static void DeleteAllTasks();

	// Met a jour le vector passe en parametre avec la signature de chaque tache enregistree
	static void GetRegisteredTaskSignatures(StringVector& svSignatures);

	///////////////////////////////////////////////
	// Outils de debugage

	// Fichier de log pour la mise au point
	static void SetParallelLogFileName(const ALString& sValue);
	static const ALString& GetParallelLogFileName();

	// Mode verbeux, ajoute des messages vers l'utilisateur :
	// - Lors d'echec, Warning qui designe explicitement quelle methode de la tache a echoue
	// - Au lancement de la tache : resume des eigences et des ressources allouees
	// - A la fin de la tache :
	//			- Nombre de processus lances vs utilises
	//			- Nombre d'acces disque simultane
	//			- Duree d'attente du disque par les esclaves
	//			- Duree de chaque acces au disque
	//			- Duree de traitement de chaque slave process (sans le disque et l'attente)
	//			- Nombre de SlaveProcess executes
	//			- Duree de la tache
	static void SetVerbose(int bVerbose);
	static boolean GetVerbose();

	// Activation des traces
	static void SetTracerMPIActive(boolean bTracerON);
	static boolean GetTracerMPIActive();
	static void SetTracerProtocolActive(boolean bTracerON);
	static boolean GetTracerProtocolActive();
	static void SetTracerResources(int nTraceLevel); // 0: pas de trace, 1: trace ON, 2: traces techniques
	static int GetTracerResources();

	///////////////////////////////////////////////
	// Methode de tests avances

	// Methodes des taches paralleles
	enum Method
	{
		NONE,
		SLAVE_INITIALIZE,
		SLAVE_PROCESS,
		SLAVE_FINALIZE,
		MASTER_INITIALIZE,
		MASTER_PREPARE_INPUT,
		MASTER_AGGREGATE,
		MASTER_FINALIZE,
		METHODS_NUMBER
	};

	// Renvoie le nom de la methode qui correspond a l'enum METHOD
	static ALString MethodToString(Method nMethod);
	static Method StringToMethod(const ALString& sMethod);

	// Type de test a effectuer
	enum TestType
	{
		NO_TEST,
		IO_FAILURE_OPEN,
		IO_FAILURE_READ,
		IO_FAILURE_WRITE,
		USER_INTERRUPTION,
		TESTS_NUMBER
	};

	// Renvoie le nom du test qui correspond a l'enum TEST
	static ALString CrashTestToString(TestType nTest);
	static TestType StringToCrashTest(const ALString& sTest);

	// Test des taches paralleles
	// Le test est effectue dans la tache dont la signature est sTaskSignature dans la methode nMethod et lors
	// de l'iteration nCallIndex de cette methode (pour les methodes MasterAggregate MasterPrepareTaskInput et
	// SlaveProcess) Il y a 4 types de tests :
	// - IO_FAILURE_OPEN : la methode Open de SystemFile renvoie Open false dans la methode et a un comportement
	// normal en dehors de celle-ci
	// - IO_FAILURE_READ : la methode Read de SystemFile renvoie Read false dans la methode et a un comportement
	// normal en dehors de celle-ci
	// - IO_FAILURE_WRITE: la methode Flush de SystemFile renvoie Flush false dans la methode et a un comportement
	// normal en dehors de celle-ci
	// - USER_INTERRUPTION : une interruption utilisateur est forcee avant l'appel a la methode
	static void CrashTest(TestType nTestType, const ALString& sTaskSignature, Method nMethod, int nCallIndex);

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Pocessus maitre ou un processus esclave
	static boolean IsMasterProcess();
	static boolean IsSlaveProcess();

	// Indicateur d'execution
	static boolean IsRunning();

	// Test de la methode ComputeStairBufferSize
	static void TestComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, int nBufferStep,
					       longint lFileSize, int nProcessNumber);

protected:
	/////////////////////////////////////////////////////////////////////////////////////////
	//
	//				Methodes virtuelles a reimplementer
	//
	/////////////////////////////////////////////////////////////////////////////////////////

	// Nom unique de la tache, utilise pour lancer l'esclave
	virtual const ALString GetTaskName() const = 0;

	// Renvoie une nouvelle instance
	virtual PLParallelTask* Create() const = 0;

	// Methode dans laquelle on calcule les exigences sur les ressources pour la tache
	// Appelee juste avant le lancement de la tache (avant MasterInitialize)
	// A implementer en appelant GetResourceRequirement et en modifiant les valeurs par defaut
	// Les ressources sont reinitialisees a la fin du run
	// Renvoie false si il y a eu un probleme lors du calacul (les messsages utilisateurs doivent etre implementes
	// en cas d'erreur)
	virtual boolean ComputeResourceRequirements();

	//////////////////////////////////////////////////////////
	// Methodes du PLMaster

	// Initialisation des resultats
	// L'initialisation doit etre complete (penser que la tache peut etre appelee plusieurs fois de suite)
	virtual boolean MasterInitialize() = 0;

	// Met a jour les input avant de lancer l'ordre a l'esclave
	// renvoie true si s'est bien passe. Sinon, le programme va se terminer.
	// dTaskPercent est la taille de la tache en pourcentage du travail total  (entre 0 et 1),
	// a alimenter dans la reimplementation de la methode (par defaut: 0).
	// i.e. si le traitement est decoupe en 10 taches identiques, on aura dTaskPercent a 0.1
	// bIsTaskFinished doit etre mis a true lorsqu'il n'y a plus rien a faire (false par defaut)
	virtual boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) = 0;

	// Aggregation des resultats d'un esclave avec le resultat global.
	// Methode appelee par le maitre apres la reception des resultats envoyes par chaque esclave.
	virtual boolean MasterAggregateResults() = 0;

	// Methode appelee apres a la fin du programme, une fois que tous les esclaves ont termine
	// equivalent du reducer de Hadoop
	// La valeur de bProcessEndedCorrectly est a true si le processus parallele s'est deroule sans erreur et sans
	// interruption utilisateur. Sinon, elle doit etre utilisee pour conditionner les traitement a effectuer Le
	// nettoyage doit etre complet, meme si toute la tache ne s'est pas deroulee correctement
	virtual boolean MasterFinalize(boolean bProcessEndedCorrectly) = 0;

	/////////////////////////////////////////////////////////////////
	// Methodes de l'esclave

	// Methode appelee au lancement du Slave
	// L'initialisation doit etre complete (penser que la tache peut etre appelee plusieurs fois de suite)
	virtual boolean SlaveInitialize() = 0;

	// Methode de traitement appelee par les esclaves
	// equivalent du Map de hadoop
	// lit les taskInput ecrit les taskOutput
	virtual boolean SlaveProcess() = 0;

	// Methode appelee a la fin du Slave (nettoyage)
	// Le nettoyage doit etre complet, meme si toute la tache ne s'est pas deroulee correctement
	// La valeur de bProcessEndedCorrectly est a true si le traitement de l'esclave s'est bien deroule.
	// Sinon, elle doit etre utilisee pour conditionner les traitement a effectuer
	// Le nettoyage doit etre complet, meme si toute la tache ne s'est pas deroulee correctement
	virtual boolean SlaveFinalize(boolean bProcessEndedCorrectly) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////
	//
	//				Methodes utilisees dans les taches par
	//				les utilisateurs de la bibliotheque
	//
	////////////////////////////////////////////////////////////////////////////////////////////

	// Lancement de la tache
	// Renvoie true si la tache s'est executee correctement
	// Renvoie false sinon ou si il y a eu une interruption utilisateur
	// Le flux des erreurs est automatiquement controle
	boolean Run();

	// Declaration d'un parametre partage entre le maitre et l'esclave
	// le parametre sera alors global au maitre et aux esclaves
	// Les shared parameters sont en ecriture uniquement dans les methodes MasterInitialize et  MasterFinalize,
	// en lecture dans toute sles methodes de la tache
	void DeclareSharedParameter(PLSharedVariable* parameter);

	// Declaration d'une variable envoyee par le maitre aux esclave avec l'ordre de traitement
	// c'est le "descriptif" ou les parametres de la tache a executer par l'esclave
	// Les variables task input sont en ecriture uniquement dans la methode MasterPrepareTaskInput,
	// en lecture dans la methode SlaveProcess
	void DeclareTaskInput(PLSharedVariable* parameter);

	// Declaration d'une variable resultat, envoyee par les esclaves au maitre
	// a l'issue de chaque appel a la methode SlaveProcess().
	// A utiliser dans le constructeur de la tache
	// Les variables task output sont en ecriture uniquement dans la methode SlaveProcess,
	// en lecture dans la methode MasterAggregate
	void DeclareTaskOutput(PLSharedVariable* parameter);

	// Nombre de Processus qui travaillent
	// sequentiel = 1, parallele = nombre d'esclaves
	int GetProcessNumber() const;

	/////////////////////////////////////////////////////////////////
	// Gestion des ressources

	// Pour le maitre : ressources allouees pour l'esclave qui va travailler
	//		Utilisable uniquement dans MasterPrepareTaskInput
	// Pour l'esclave : ressources allouees a l'esclave courant
	const RMResourceGrant* GetSlaveResourceGrant() const;

	// Resources allouees pour le master
	// Utilisable uniquement par le master
	const RMResourceGrant* GetMasterResourceGrant() const;

	// Acces aux exigences des esclaves
	// Accessible uniquement depuis le master et les esclaves
	const RMResourceRequirement* GetSlaveResourceRequirement() const;

	// Acces aux ressources allouees a la tache
	// Utilisable uniquement dans MasterInitialize
	const RMTaskResourceGrant* GetTaskResourceGrant() const;

	/////////////////////////////////////////////////////////////////
	// Gestion des messages utilisateurs

	// Les index des lignes sont locaux au slaveProcess courant
	// Le maitre calculera les index absolus lors de l'affichage du message
	void AddLocalError(const ALString& sLabel, int nLineNumber);
	void AddLocalWarning(const ALString& sLabel, int nLineNumber);
	void AddLocalMessage(const ALString& sLabel, int nLineNumber);

	// Nombre de lignes lues par l'esclave
	// Methode a appeler a la fin du SlaveProcess pour que la maitre puisse
	// calculer l'index absolu des lignes
	void SetLocalLineNumber(longint lLocalLineNumber);

	/////////////////////////////////////////////////////////////////
	// Mise en sommeil des esclaves
	// Lorsqu'un esclave est en sommeil il ne recoit plus d'ordre de travail
	// Cela permet de realiser une synchronisation de tous les esclaves et
	// de s'assurer que tous les esclaves realisent une action donnee
	//
	// Si a partir d'un evenement A, tous les esclaves doivent lire un fichier
	// Dans la methode MasterPreparetaskInput, on aura :
	//		if(A) {SetSlaveAtRestAfterProcess(), input_readFile=true;}
	// Dans SlaveProcess, on aura :
	//		if (input_readFile) Read(File);
	// Dans le MasterAggregate :
	//		if (GetRestingSlaveNumber() == GetProcessNumber()) SetAllSlavesAtWork();

	// Ordonne a l'esclave de se mettre en sommeil apres le prochain slaveProcess
	// Il n'executera alors plus de SlaveProcess tant que la methode SetAllSlavesAtWork n'est pas appelee
	// Utilisable uniquement dans MasterPrepareTaskInput
	void SetSlaveAtRestAfterProcess();

	// Ordonne a l'esclave de se mettre en sommeil directement, sans executer de tache et donc
	// sans passer par MasterAggregateResults et sans utiliser ni incrementer le TaskIndex
	// Le pourcentage d'avancement de MasterPrepareTaskInput ne sera pas pris en compte
	// Cela permet de mettre des esclaves en attente, tant que les resultats d'autres esclaves
	// en cours ne sont pas disponibles
	// Utilisable uniquement dans MasterPrepareTaskInput
	void SetSlaveAtRest();

	// Est-ce que l'esclave courant est en sommeil
	// Utilisable uniquement dans MasterAggregateResults
	boolean IsSlaveAtRest() const;

	// Retourne le nombre d'esclave en sommeil
	int GetRestingSlaveNumber() const;

	// Remet tous les esclaves qui sont en sommeil dans l'etat normal : ils pourront executer les taches
	// Utilisable uniquement dans MasterAggregateResults
	void SetAllSlavesAtWork();

	/////////////////////////////////////////////////////////////////
	// Gestion des fichiers temporaires uniques cree par l'esclave
	//
	// Les fichiers temporaires crees dans les esclaves dans les SlaveProcess sont collectes
	// par le maitre lors des AggregateResults via des variables partagees en output contenant
	// les noms de ces fichiers temporaires
	// En cas d'erreur ou d'interruption utilisateur, il faut detruire ces fichiers temporaires.
	// 1) Si l'erreur intervient dans un SlaveProcess, l'esclave detruit les eventuels fichiers crees
	// et n'envoie pas les noms des fichiers temporaires au maitre.
	// 2) Sinon, le maitre peut demander la destruction de tous les fichiers temporaires qu'il
	// a collectes lors des AggregateResults.
	// 3) Il reste le cas d'un arret de la tache alors que des esclaves ont fini sans erreur leur
	// SlaveProcess, mais que le maitre n'a pas effectue les AggegateResults correspondants, qui
	// ne seront pas traites pour arreter la tache au plus vite. Dans ce cas, l'esclave doit
	// les memoriser, pour pouvoir les detruire lors du SlaveFinalize.
	// Au debut du SlaveProcess suivant, il est assure que ses fichiers temporaires precedents ont
	// ete memorises par le maitre dans le AggregateResults, et il peut repartir d'une liste vide

	// Ajout d'un fichier ou plusieurs fichiers a la liste des fichiers temporaires cree par l'esclave
	// A appeler en fin de SlaveProcess si pas d'erreur
	void SlaveRegisterUniqueTmpFile(const ALString& sFileName);
	void SlaveRegisterUniqueTmpFiles(const StringVector* svFileNames);

	// Mise a vide de la liste des fichiers temporaires
	// Methode appelee systematiquement en debut de SlaveProcess et
	// dans SlaveFinalize s'il n'y ni erreur ni interruption
	void SlaveInitializeRegisteredUniqueTmpFiles();

	// Destruction des fichiers temporaires creees par l'esclave
	// Methode appelee systematiquement dans SlaveFinalize si erreur ou interruption
	void SlaveDeleteRegisteredUniqueTmpFiles();

	/////////////////////////////////////////////////////////////////
	// Methodes avancees

	// Mode hyper-actif : le maitre et les eclaves ne s'endorment jamais +  reception efficace des message
	// A utiliser uniquement si necessaire, les processus utilisent toute la CPU
	// Utilisable uniquement dans MasterInitialize
	void SetBoostMode(boolean bBoost);
	boolean GetBoostMode() const;

	// Methode utilitaire pour l'affectation de la taille du buffer de lecture pour eviter que les esclaves accedent
	// au fichier tous en meme temps. La taille est comprise entre nBufferSizeMin et nBufferSizeMax
	// - pour les GetProcessNumber() premiers chunks, on fait une marche d'escalier reguliere de nBufferSizeMin a
	// nBufferSizeMax
	// - pour le milieu du fichier on renvoie une valeur aleatoire entre 0.8*nBufferSizeMax et nBufferSizeMax
	// - pour la fin on donne nBufferSizeMin (c'est la fin du traitement quand on peut donner 2 nBufferSizeMin a
	// chaque esclave ou 							quand il n'y a plus que la moitie des
	// esclaves qui vont travailler) La valeur retournee est un multiple de la taille d'un bloc et plus petite que
	// InputBufferedFile::GetMaxBufferSize()
	//
	// Ne peut etre appele que dans MasterPrepareTaskInput
	int ComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, int nBufferSizeStep, longint lFileProcessed,
				   longint lFileSize) const;

	// Permet de specifier si on affiche les messages de tous les esclaves pendant l'initialisation
	// Si bValue=true, seuls les messages du premier esclave a emmettre seront affiches (mode par defaut)
	void SetSlaveInitializeErrorsOnce(boolean bValue);
	boolean GetSlaveInitializeErrorsOnce() const;

	// Idem SetSlaveInitializeErrorsOnce pour SlaveFinalize
	void SetSlaveFinalizeErrorsOnce(boolean bValue);
	boolean GetSlaveFinalizeErrorsOnce() const;

	// Retourne les identifiants des esclaves qui vont contribuer a la tache.
	// Aucune supposition ne doit etre faite sur ces identifiants (pas de sequence, ordre etc...)
	// Utilisable uniquement dans le MasterInitialize
	const IntVector* GetGrantedSlaveProcessIds() const;

	////////////////////////////////////////////////////////////////////////////////////////////
	//
	//									Methodes techniques (privees)
	//
	////////////////////////////////////////////////////////////////////////////////////////////

	// Mode de fonctionnement
	boolean IsSequential() const;
	boolean IsParallelSimulated() const;
	boolean IsParallel() const;

	// Methode qui renvoie true si elle sont appelees dans une methode de l'esclave (resp. du maitre)
	// Le comportement de ces methodes est independant du type de run (seqentiel, simule ou parallel)
	// Utilisees dans les require
	boolean IsInSlaveMethod() const;
	boolean IsInMasterMethod() const;

	// Pour l'esclave : index de la tache en cours de traitement (pour l'esclave)
	// Dans MasterPrepareTaskInput : index de la prochaine sous-tache lancee pour le maitre
	// Dans MasterAggregate : index de la sous-tache qui vient de finir (effectuee par l'esclave qui a envoye les
	// resultats)
	int GetTaskIndex() const;

private:
	// Mode de fonctionnement parallele (esclave)
	void SetToSlaveMode();

	// Methode appelee dans le main du programme maitre
	// lance nb_Slaves programmes escalves. Le programme esclave est nomme d'apres GetTaskName()
	boolean RunAsMaster();

	// Methode appelee pour lancer le programme en sequentiel
	boolean RunAsSequential();

	// Methode appelee pour lancer le programme en mode test
	// Le programme est sequentiel mais les obkets slave et master sont crees
	// Il y a serialisation et deserialisation des donnes, mode tres proche, d'un vrai parallelisme
	boolean RunAsSimulatedParallel(int nSlaveNumber);

	// Boucle de traitement sur les esclaves
	boolean SequentialLoop();

	// Signature de la tache : taskName + nombre de variables paragees
	// Permet de differencier deux taches meme si elles ont le meme taskName
	ALString GetTaskSignature() const;

	// Wrappers des methodes du maitre et des esclaves, ils sont communs aux
	// modes parallele sequentiel et simule,
	boolean CallSlaveInitialize();
	boolean CallSlaveProcess();
	boolean CallSlaveFinalize(boolean bProcessOk);
	boolean CallMasterInitialize();
	boolean CallMasterFinalize(boolean bProcessOk);
	boolean CallMasterAggregate();
	boolean CallMasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished, const PLSlaveState* slave);

	// Calcul de la taille du bufefr pour lire un fichier
	// appelee dans ComputeStairBufferSize et TestComputeStairBufferSize
	static int InternalComputeStairBufferSize(int nBufferSizeMin, int nBufferSizeMax, int nMultiple,
						  longint lFileProcessed, longint lFileSize, int nProcessNumber,
						  int nTaskIndex);

	// Nom de la tache en cours
	static ALString GetCurrentTaskName();

	// Libelle utilisateur de la tache
	ALString sTaskUserLabel;

	// Temps de traitement
	Timer tJob;
	Timer tMaster;
	Timer tMasterInitialize;
	Timer tMasterFinalize;

	static boolean bParallelSimulated;
	static int nSimulatedSlaveNumber;
	static ALString sVersion;

	// Methode a appeler en fin de programme pour arreter tous les esclaves
	static void Exit(int nExitCode);

	// Type de la tache : sequentiel (defaut) maitre ou esclave
	enum Mode
	{
		SEQUENTIAL,
		MASTER,
		SLAVE,
		PARALLEL_SIMULATED
	};

	// Mode de fonctionnement
	// en complement de IsSequential()
	boolean IsMaster() const;
	boolean IsSlave() const;

	//////////////////////////////////////////
	// Gestion de l'etat des esclaves

	// pour debuggage : donne l'etat de stous les esclaves
	void PrintSlavesStates() const;

	// Renvoie un slave dont le status est READY
	// Renvoie null si il n'y a aucun slave READY
	PLSlaveState* GetReadySlave();

	// Renvoie un slave dont le status est READY parmi ceux passes en parametre
	// Les esclaves deja initialises sont privilegies
	PLSlaveState* GetReadySlaveOnHost(ObjectArray* oaSlaves);

	// Renvoie le slave dont le rang est rank
	// Renvoie NULL si aucun esclave n'a ce rang
	PLSlaveState* GetSlaveWithRank(int nRank) const;

	// Tous les esclaves ont passe le Finalize
	boolean IsAllSlavesEnding() const;

	// Methodes de serialisation / deserialisation qui doivent etre
	// utilisees lors de l'envoi des variables partagees de sortie (TaskOutput)
	// et lors de la reception des variables d'entree (TaskInput) et des parametres (Parameters)
	// Lorsque bClean est a True, les variables sont nettoyees (appel de Clean()) juste apres la serialisation
	void SerializeSharedVariables(PLSerializer* serializer, ObjectArray* oaVariables, boolean bClean) const;
	void DeserializeSharedVariables(PLSerializer* serializer, ObjectArray* oaVariables) const;

	// Initialisation des attributs de la tache a partir des variables partagees
	// et creation du repertoire applicatif
	void InitializeParametersFromSharedVariables();

	// Erreur indexee  generique
	void AddLocalGenericError(int nGravity, const ALString& sLabel, longint lLineNumber);

	// Mise a jour de la duree de vie du repertoire temporaire
	// N'est mise a jour que si il s'est ecoule moins d'une heure depuis la derniere mise a jour
	void TouchTmpDir();
	Timer tTempTouch;

	// type de la tache : sequentiel (defaut) maitre ou esclave
	Mode runningMode;

	// Nombre de warnings/errors envoyes
	int nMessagesNumber;
	int nSimpleMessagesNumber;
	int nWarningsNumber;
	int nErrorsNumber;

	// Est-ce que le dernier run s'est bien termine
	boolean bLastRunOk;

	// Est-ce que la tache a ete interrompue
	boolean bIsTaskInterruptedByUser;

	// Nombre de process esclave
	int nWorkingProcessNumber;

	// Parametres  de la classe Global
	PLShared_Boolean input_bSilentMode;
	PLShared_Int shared_nMaxErrorFlowNumber;

	// Parametre sur la taille max des lignes des InputBufferedFile
	PLShared_Int shared_nMaxLineLength;

	// Parametre sur la taille max de la la heap, pour les crash test sur la memoire
	PLShared_Longint shared_lMaxHeapSize;

	// Nom de la tache
	PLShared_String shared_sTaskUserLabel;

	// Exigences pour les esclaves
	PLShared_ResourceRequirement shared_slaveResourceRequirement;

	// Tracers
	PLShared_Tracer shared_tracerProtocol;
	PLShared_Tracer shared_tracerMPI;
	PLShared_Tracer shared_tracerPerformance;

	// Parametres de tracers (on n'utilise pas les variables partagees car c'est envoye en dehors du protocole
	// standard)
	static ALString sParallelLogFileName;

	// Mode simule uniquement : position de l'esclave acteuellement en traitement
	int nCurrentSlavePosition;

	// Mode verbose
	PLShared_Boolean input_bVerbose;

	// Nombre de taches traitees (avec celles en cours )
	int nTaskProcessedNumber;
	PLShared_Int input_nTaskProcessedNumber;

	// Index de la tache traitee par l'esclave qui vient de finir
	int nSlaveTaskIndex;

	// Nombre de lignes lues par l'esclave
	PLShared_Longint output_lLocalLineNumber;

	// Est-ce que le slaveprocess s'est bien termine
	PLShared_Boolean output_bSlaveProcessOk;

	// Exigences sur les ressources de la tache
	RMTaskResourceRequirement* requirements;

	// Rang du prochain esclave qui va travailler
	int nNextWorkingSlaveRank;

	// Est-ce que la tache a ete lancee?
	boolean bIsJobDone;

	// Critere d'arret pour les taches paralleles
	boolean bJobIsTerminated;

	// Dans quelle methode on se trouve
	// Utilise pour contraindre  le contexte d'utilisation de certaine methode
	// et pour empecher de lancer des taches imbriquees les unes dans les autres (c'est pourquoi cette variable est
	// statique)
	static Method method;

	// Status du programme : en cours d'execution ou non
	// utilise dans les assertion
	static boolean bIsRunning;

	// Erreur fatale levee en mode slave
	// mis a jour dans AddFatalError
	boolean bSlaveFatalError;

	// Sortie de la boucle de processing en mode sequentiel
	// mis a jour dans MasterBreak()
	boolean bUserBreak;

	// Parametres envoyes par le Master aux esclaves avant le traitement
	ObjectArray oaSharedParameters; // dictionnaire name / PLSharedParameter

	// Parametres envoyes par les maitre a chaque esclave avec l'ordre de travail
	// c'est le "descriptif" du travail a faire
	ObjectArray oaInputVariables; // dictionnaire name / PLSharedParameter

	// Resultats envoyes par les esclaves au Master a la fin de chaque sous tache
	ObjectArray oaOutputVariables; // dictionnaire name / PLSharedParameter

	// Pour s'assurer que les shared parametres sont declares dans la methode
	// RegisterSharedparameter();
	boolean bCanRegisterSharedparameter;

	// Representation interne de l'etat de chaque esclave (status, nombre de ligne lu, position dans le fichier...)
	// Ce tableau doit etre  rempli et vid'e dans la methode Run
	ObjectArray oaSlaves; // Tableau de PLSlaveState

	// Tableau des esclaves indexes par leur rang
	ObjectArray oaSlavesByRank;

	// Liste des rangs des esclaves
	IntVector ivGrantedSlaveIds;

	// dictionaire host / esclaves
	ObjectDictionary odHosts;

	// Est-ce qu'on affiche les messages de tous les esclaves lors de leur initialisation/Finalisation
	boolean bSlaveInitializeErrorsOnce;
	boolean bSlaveFinalizeErrorsOnce;

	// Statistiques sur les durees
	PLShared_Double output_dProcessingTime;
	PLSharedIncrementalStats output_statsIOReadDuration;
	PLSharedIncrementalStats output_statsIORemoteReadDuration;
	PLIncrementalStats stats_IOReadDuration; // Temps de lecture (acces local)
	PLIncrementalStats stats_ProcessDuration;
	PLIncrementalStats stats_IORemoteReadDuration; // Temps de lecture distant (acces local + envoi)

	// Gestion des permissions de parametres et des variables resultats
	// pour garantir une bonne utilisation via les asserts
	// Ne fait rien si NDEBUG n'est pas defini (assertions)
	void SetSharedVariablesRO(const ObjectArray*);
	void SetSharedVariablesRW(const ObjectArray*);
	void SetSharedVariablesNoPermission(const ObjectArray*);

	// Driver courant qui pointe soit vers un driver parallele soit vers un driver sequentiel (par defaut)
	static PLTaskDriver* currentDriver;

	// Administration des objets Task
	static ObjectDictionary* odTasks;

	// Messages vers l'utilisateur (nombre de procs lances etc...)
	static boolean bVerbose;

	// Activation des tracers
	static boolean bTracerMPIActive;
	static boolean bTracerProtocolActive;
	static int nTracerResources;

	// Nom de la tache en cours
	static ALString sCurrentTaskName;

	// Nombre de tache instanciees
	static int nInstanciatedTaskNumber;

	// Nombre de Run effectues (utile pour le nom de la tache dans les performances)
	static int nRunNumber;

	// Crash test
	static TestType nCrashTestType;
	static ALString sCrashTestTaskSignature;
	static Method nCrashTestMethod;
	static int nCrashTestCallIndex;
	PLShared_Int shared_nCrashTestType;
	PLShared_Int shared_nCrashTestMethod;
	PLShared_Int shared_nCrashTestCallIndex;

	// Compteurs du nombre d'appels aux methodes SlaveProcess, PrepareTaskInput et MasterAggregate
	int nPrepareTaskInputCount;
	int nMasterAggregateCount;

	// Mode boost : les esclaves ne font plus de prob et SystemSleep
	PLShared_Boolean shared_bBoostedMode;

	// Nom de la tache dans les traces de performance
	// on ajoute le nombre d'instances au nom de la tache : la meme tache peut etre lancee plusieurs fois
	ALString sPerformanceTaskName;

	// Liste de tous les fichiers temporaires enregistres par l'esclave
	StringVector svSlaveRegisteredUniqueTmpFiles;

	// Est-ce que l'esclave qui instancie cette tache est initialise
	boolean bIsSlaveInitialized;

	// Tableau des messages utilisateurs en attente d'affichage
	// Cet tableau est alloue et detruit dans la classe PLMPISlave
	static ObjectArray* oaUserMessages;

	// Nombre de lignes actuellement traitees du fichier principal (master buffer)
	longint lGlobalLineNumber;

	// La methode AddLocalGenericError a ete appele dans le dernier SlaveProcess
	boolean bAddLocalGenericErrorCall;

	// La methode SetLocalLineNumberCall a ete appele dans le dernier SlaveProcess
	boolean bSetLocalLineNumberCall;

	// L'esclave courant est mis en sommeil pendant le MasterPrepareInput
	// Le SlaveProcess ne sera pas execute
	boolean bSlaveAtRestWithoutProcessing;

	// Titres de la barre de progression pendant les differentes phases de la tache parallele
	const ALString PROGRESSION_MSG_MASTER_INITIALIZE = "Initialization";
	const ALString PROGRESSION_MSG_MASTER_FINALIZE = "finalization";
	const ALString PROGRESSION_MSG_SLAVE_INITIALIZE = "workers initialization";
	const ALString PROGRESSION_MSG_SLAVE_FINALIZE = "workers finalization";
	const ALString PROGRESSION_MSG_PROCESSING = "processing";

	// Classes et fonctions friend
	friend class PLMaster;
	friend class PLMPISlave;
	friend class PLMPIMaster;
	friend class PLMPITaskDriver;
	friend class PLMPISlaveLauncher;
	friend class PLMasterSequential;
	friend class PLSlaveSequential;
	friend class RMParallelResourceManager; // Pour acces a ivFileServerRanks
	friend class RMParallelResourceManager; // Pour acces a ivFileServerRanks
	friend class KWKeyExtractor;            // Pour acces aux methodes AddLocalError ..
	friend class KWCrashTestParametersView;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLParallelTask::SetSimulatedSlaveNumber(int nSlaveNumber)
{
	require(nSlaveNumber > 0);
	nSimulatedSlaveNumber = nSlaveNumber;
}

inline int PLParallelTask::GetSimulatedSlaveNumber()
{
	return nSimulatedSlaveNumber;
}

inline void PLParallelTask::SetParallelSimulated(boolean bValue)
{
	bParallelSimulated = bValue;
}

inline boolean PLParallelTask::GetParallelSimulated()
{
	return bParallelSimulated;
}

inline void PLParallelTask::SetVerbose(int bValue)
{
	bVerbose = bValue;
}

inline boolean PLParallelTask::GetVerbose()
{
	return bVerbose;
}

inline boolean PLParallelTask::IsParallel() const
{
	require(bIsRunning);
	return not(runningMode == SEQUENTIAL) and not(runningMode == PARALLEL_SIMULATED);
}

inline boolean PLParallelTask::IsSequential() const
{
	require(bIsRunning);
	return runningMode == SEQUENTIAL;
}

inline boolean PLParallelTask::IsParallelSimulated() const
{
	return GetParallelSimulated();
}

inline boolean PLParallelTask::ComputeResourceRequirements()
{
	return true;
}

inline boolean PLParallelTask::IsMaster() const
{
	require(bIsRunning);
	return runningMode == MASTER;
}

inline boolean PLParallelTask::IsSlave() const
{
	require(bIsRunning);
	return runningMode == SLAVE;
}

inline boolean PLParallelTask::IsRunning()
{
	return bIsRunning;
}

inline int PLParallelTask::GetProcessNumber() const
{
	require(bIsRunning == true);
	return nWorkingProcessNumber;
}

inline boolean PLParallelTask::IsMasterProcess()
{
	return GetProcessId() == 0;
}

inline boolean PLParallelTask::IsSlaveProcess()
{
	return GetProcessId() != 0;
}

inline void PLParallelTask::SetSlaveInitializeErrorsOnce(boolean bValue)
{
	bSlaveInitializeErrorsOnce = bValue;
}

inline boolean PLParallelTask::GetSlaveInitializeErrorsOnce() const
{
	return bSlaveInitializeErrorsOnce;
}

inline void PLParallelTask::SetSlaveFinalizeErrorsOnce(boolean bValue)
{
	bSlaveFinalizeErrorsOnce = bValue;
}

inline boolean PLParallelTask::GetSlaveFinalizeErrorsOnce() const
{
	return bSlaveFinalizeErrorsOnce;
}

inline PLSlaveState* PLParallelTask::GetSlaveWithRank(int rank) const
{
	return cast(PLSlaveState*, oaSlavesByRank.GetAt(rank));
}

inline void PLParallelTask::SetTracerMPIActive(boolean bTracerON)
{
	bTracerMPIActive = bTracerON;
}

inline boolean PLParallelTask::GetTracerMPIActive()
{
	return bTracerMPIActive;
}

inline void PLParallelTask::SetTracerProtocolActive(boolean bTracerON)
{
	bTracerProtocolActive = bTracerON;
}

inline boolean PLParallelTask::GetTracerProtocolActive()
{
	return bTracerProtocolActive;
}

inline void PLParallelTask::SetTracerResources(int nTraceOn)
{
	nTracerResources = nTraceOn;
}

inline int PLParallelTask::GetTracerResources()
{
	return nTracerResources;
}

inline void PLParallelTask::SetBoostMode(boolean bBoost)
{
	require(method == Method::MASTER_INITIALIZE);
	shared_bBoostedMode = bBoost;
}

inline boolean PLParallelTask::GetBoostMode() const
{
	return shared_bBoostedMode;
}

inline boolean PLParallelTask::IsSlaveAtRest() const
{
	require(method == MASTER_AGGREGATE);
	return GetSlaveWithRank(nNextWorkingSlaveRank)->GetAtRest();
}