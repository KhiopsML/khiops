// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class UIObject;
class UIData;
class UIUnit;
class UICard;
class UIObjectView;
class UIConfirmationCard;
class UIQuestionCard;
class UIFileChooserCard;
class UIList;
class UIObjectArrayView;
class UIElement;
class UIBooleanElement;
class UICharElement;
class UIIntElement;
class UIDoubleElement;
class UIStringElement;
class UIAction;

#include "Object.h"
#include "Vector.h"
#include "Ermgt.h"
#include "TaskProgression.h"
#include "PLRemoteFileService.h"
#include "FileService.h"
#include "CommandLine.h"

// Directive de compilation dediee au developpement des composants UI
// Seule la librairie Base a besoin effectivement de <jni.h> pour etre compilee
// (donc du define UIDEV)
// Les sources utilisant la librairie Base n'ont besoin que du header et des
// pseudo-declarations a des pointeurs JNI qui ne sont jamais accedes
#ifdef UIDEV
#include "jni_wrapper.h"
// Pointeur vers les fonctions java
typedef jint(JNICALL* PtrCreateJavaVM)(JavaVM**, void**, void*);
typedef jint(JNICALL* PtrGetCreateJavaVM)(JavaVM**, jsize, jsize*);
typedef jint(JNICALL* PtrGetVersion)(JavaVM**);
#else
// Reproduction simplifiee des declarations du JDK
// pour eviter d'imposer la presence du JDK,
// permet d'installer uniquement le run-time Java aux utilisateurs de Norm
struct JNIEnv_;
typedef JNIEnv_ JNIEnv;
struct JavaVM_;
typedef JavaVM_ JavaVM;
struct _jmethodID;
typedef struct _jmethodID* jmethodID;
class _jclass
{
};
typedef _jclass* jclass;
class _jobject
{
};
typedef _jobject* jobject;
class _jstring : public _jobject
{
};
typedef _jstring* jstring;
#define JNICALL
#endif

class UIObject;
class UIData;
class UIUnit;
class UICard;
class UIList;
class UIElement;
class UIAction;

///////////////////////////////////////////////////////////
// Definition et utilisation d'une interface utilisateur
// Hierarchie des classes:
//  UIObject
//      UIData
//          UIUnit
//              UICard
//                 UIObjectView
//                 UIConfirmationCard
//                 UIQuestionCard
//                 UIFileChooserCard
//              UIList
//                 UIObjectArrayView
//          UIElement
//              UIBooleanElement
//              UICharElement
//              UIIntElement
//              UIDoubleElement
//              UIStringElement
//      UIAction
//
// Les composants UI permettent de definir rapidement et simplement une
// interface utilisateur pour des programmes d'optimisation:
//      . consultation/mise a jour des donnees elementaires regroupees
//        au sein d'unites d'interface de type fiche ou liste
//      . declenchement d'actions utilisateurs
// Ces composants permettent un dialogue avec l'utilisateur dans une
// session shell en affichant les donnees de l'interface, en interrogeant
// l'utilisateur pour la saisie de parametres ou d'un code action.
// L'application peut ainsi etre completement realisee et testee simplement
// en C++.
// Un mode de fonctionnement en interface graphique permet d'habiller
// automatiquement cette interface utilisateur avec des fenetres, boites
// de saisie et menus deroulants, sans aucun developpement supplementaire.
// (methode SetUIMode de UIObject)
//
// L'utilisation des composants d'interface se fait uniquement au moyen des
// classes UICard et UIList. Il suffit de declarer la listes des donnees
// et actions gerees par l'unite d'interface, et d'appeler la methode Open.
// Une API permet d'echanger les donnees entre le programme et
// l'unite d'interface.
//
// On offre egalement des services d'enregistrement et de re-execution
// d'une session d'utilisation de unites d'interface
// Dans un fichier contenant des commandes, on peut utiliser "//" comme
// commentaire.
//
// Il suffit d'appeler la methode ParseMainParameters de UIObject pour specifier
// les options de stockages des commandes d'une session de menus.
// Les parametres de la ligne de commande peuvent etre:
//   -i <input command file>: fichier des commandes a re-executer
//   -o <output command file>: fichier de memorisation des commandes
//   -e <error log file>: log
//   -t: <task progression log file>: avancement des taches
//   -b: mode batch
//   -h: aide
// La methode ParseMainParameters analyse la ligne de commande,
// gere ses erreurs, et si OK prepare l'execution des fonctionnalites

//////////////////////////////////////////////////
// Classe UIObject
// Ancetre des classe d'interface utilisateur
class UIObject : public Object
{
public:
	// Identifiant de l'objet d'interface
	const ALString& GetIdentifier() const;

	// L'identifiant est impose par construction des elements de l'interface, sauf pour la fenetre racine
	void SetIdentifier(const ALString& sValue);

	// Libelle associe a l'objet d'interface
	// Le libelle peut potrentiellement etre formate, entre des balises <html>
	/*! (titre d'une fenetre, d'un groupbox,
	 libelle d'une donnee elementaire, d'une action...)*/
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Version du libelle sans formattage
	const ALString GetUnformattedLabel() const;

	// Visibilite d'un composant (defaut: true)
	/*! Un composant non visible (action ou donnee n'apparait pas
	a l'interface, mais est utilisable par programme)*/
	void SetVisible(boolean bValue);
	//
	boolean GetVisible() const;

	// Texte d'aide utilisateur sur un composant
	/*! Le texte peut contenir des retours a la ligne ('\n')*/
	void SetHelpText(const ALString& sValue);
	//
	const ALString& GetHelpText() const;

	// Style d'un composant (aucun par defaut)
	/*! Le style permet d'influer sur le choix de representation du
	 composant par l'interface.
	 Les valeurs possibles sont donnees dans les sous-classes gerant les styles.
	 Un style vide revient au choix du style par defaut*/
	void SetStyle(const ALString& sValue);
	const ALString& GetStyle() const;

	// Parametres des styles
	const ALString& GetParameters() const;
	void SetParameters(const ALString& sValue);

	// Lettres mnemonique permettant un acces direct aux actions ou aux donnees
	const char& GetShortCut() const;
	void SetShortCut(const char& cValue);

	// Parametrage de l'icone a utiliser dans toutes les fenetre d'interface
	// L'icone (un fichier .gif) doit se trouver dans un .jar accessible dans le classpath
	// Si parametre vide ou invalide, l'icone java par defaut est utilisee
	static void SetIconImage(const ALString& sIconImage);
	static ALString GetIconImage();

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Libelle utilisateur: le libelle
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////////
	// Gestion du mode de presentation de l'interface (defaut: Graphic)
	enum
	{
		Textual,
		Graphic
	};

	// Si on echoue a passer en mode Graphic (probleme java), on passe
	// inconditionnellement en Textual et on renvoie false
	static boolean SetUIMode(int nValue);
	static int GetUIMode();

	// Affichage d'un message dans une unite de presentation globale dediee
	static void DisplayMessage(const ALString& sMessage);

	// Acces a la version du moteur UI C++
	static const ALString GetUIEngineVersion();

	///////////////////////////////////////////////////////////////////////////
	// Analyse de la ligne de commande du main, pour positionner
	// les fichiers d'entree et de sortie des commandes memorisees
	// Possibilite d'avoir un mode "verbeux" indiquant
	// quelles commandes ont ete rejouees
	// (autodocumente dans l'aide en ligne associee)
	// Erreur fatale si echec
	static void ParseMainParameters(int argc, char** argv);

	// Acces a la ligne de commande et a ses options
	// Permet d'ajouter ou de modifier les options
	static CommandLine* GetCommandLineOptions();

	// Indique si on est en mode batch (defaut: false)
	// Ce mode peut etre modifie suite a un parametrage de la ligne de commande
	static boolean IsBatchMode();

	// Indique si le mode textuel peut-etre utilise de facon interactive
	// avec des saisies d'actiosn ou de valeurs de champs depuis une fenetre shell
	// Sort en erreur fatale si on ariive dans ce mode et qu'il n'est pas autorise
	// (defaut: false)
	static void SetTextualInteractiveModeAllowed(boolean bValue);
	static boolean GetTextualInteractiveModeAllowed();

	///////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class UIUnit;
	friend class UICard;
	friend class UIList;
	friend class UIObjectArrayView;

	// Constructeur
	UIObject();

	// Destructeur
	~UIObject();

	// Test si un identifiant est valide
	boolean IsIdentifier(const ALString& sValue) const;

	// Test d'acces a une classe Java
	static boolean IsJavaClassAvailable(const ALString& sClassName);

	// Objet d'interface parent: seul les unite d'interface terminale ont
	// leur parent a NULL
	// Utilisation interne seulement
	void SetParent(UIObject* value);
	UIObject* GetParent() const;

	// Chemin d'identifiant (separes par des '.') depuis une unite d'interface terminale
	// (vide pour celles ci, sinon de la forme <grand parent Id>.<Parent Id>.<Id>
	ALString GetIdentifierPath() const;

	// Identifiant global
	// Chemin d'identifiant (separes par des '.') depuis une unite d'interface terminale
	ALString GetGlobalIdentifier() const;

	// Traduction d'un text en format html
	ALString GetHtmlText(const ALString sText) const;

	////////////////////////////////////////////////////////////////
	// Methodes de gestion de l'interface en mode textuel
	// Norme: toutes ces methodes sont prefixees par Textual

	// Lecture d'une entree utilisateur
	const ALString& TextualReadInput();

	// Ecriture d'une sortie utilisateur
	void TextualDisplayOutput(const ALString& sValue);

	// Affichage d'un titre
	void TextualDisplayTitle(const ALString& sValue);

	///////////////////////////////////////////////////////////////
	// Methodes de gestion de l'interface en mode graphique
	// Norme: toutes ces methodes sont prefixees par Graphic*

public:
	// Estimation de la memoire (en octets) utilise par l'interface
	// Importante si on est en mode graphique, nulle sinon
	static longint GetUserInterfaceMemoryReserve();

	// Obtention de l'environnement Java JNI, utile pour toutes les methodes JNI
	// (l'environnement est alloue a la premiere utilisation, et desalloue
	// automatiquement des qu'il n'y a plus de UIObject en memoire)
	static JNIEnv* GetJNIEnv();

	// Dechargement de la dll JVM si necessaire
	static void FreeJNIEnv();

	// Obtention de l'environement Java JNI, avace interface graphique
	static JNIEnv* GraphicGetJNIEnv();

	// Verification de la compatibilite des moteurs d'interface C++ et Java
	// Erreur fatal si incompatibilite
	static boolean CheckUserInterfaceEngineCompatibility(JNIEnv* env);

	// Affichage de l'exception java (a appeler avant une erreur fatale liee a Java)
	// Sert essentiellement a faciliter la mise au point java
	// L'affichage se fait dans la fenetre de shell
	// Apres l'appel, env passe a NULL (pour eviter la reentrance des erreurs)
	static void DisplayJavaException(JNIEnv* env);

	// Obtention du cls (classe id) Java d'une classe
	static jclass GraphicGetClassID(const char* sClassName);

	// Obtention du cls (classe id) Java de la classe GUIAction
	static jclass GraphicGetGUIActionID();

	// Obtention du cls (classe id) Java de la classe GUICard
	static jclass GraphicGetGUICardID();

	// Obtention du cls (classe id) Java de la classe GUIList
	static jclass GraphicGetGUIListID();

	// Obtention du cls (classe id) Java de la classe GUIObject
	static jclass GraphicGetGUIObjectID();

	// Obtention du cls (classe id) Java de la classe GUIUnit
	static jclass GraphicGetGUIUnitID();

	// Obtention du cls (classe id) Java de la classe GUIElement
	static jclass GraphicGetGUIElementID();

	// Obtention du mid (method id) Java d'un methode d'instance
	static jmethodID GraphicGetMethodID(jclass cls, const char* sClassName, const char* sMethodName,
					    const char* sMethodSignature);

	// Obtention du mid (method id) Java d'un methode de classe
	static jmethodID GraphicGetStaticMethodID(jclass cls, const char* sClassName, const char* sMethodName,
						  const char* sMethodSignature);

	// Transcodage des chaines de caracteres entre java vers C++, en utilisant le meilleur encodage possible
	static const ALString FromJstring(JNIEnv* envValue, const jstring value);
	static const jstring ToJstring(JNIEnv* envValue, const ALString& sValue);

	// Transcodage natif en imposant le type d'encodage (UTF8, sinon ASCII)
	static const ALString NativeFromJstring(JNIEnv* envValue, const jstring value, boolean bIsUTF8);
	static const jstring NativeToJstring(JNIEnv* envValue, const ALString& sValue, boolean bIsUTF8);

	// Test si une chaine de caracteres est encodee en UTF8
	static boolean IsUTF8(JNIEnv* envValue, const ALString& sValue);

protected:
	/////////////////////////////////////////////////////////////
	// Gestion des fichier d'entree et de sortie
	// Le contenu d'un fichier d'entree-sortie de commandes est constituee
	// d'une suite de lignes de type
	//      <ActionIdentifierPath>        // <action comment>
	//      <FieldIdentifierPath> <value> // <field comment>
	//      <ListIdentifierPath>.List.Index <value>  // List index comment
	//      <ListIdentifierPath>.List.Key <value>  // List key comment

	// Ouverture des fichiers d'entree-sortie des commandes
	static boolean OpenInputCommandFile(const ALString& sFileName);
	static boolean OpenOutputCommandFile(const ALString& sFileName);
	static boolean ReplaceCommand(const ALString& sSearchReplacePattern);
	static boolean BatchCommand(const ALString& sParameter);
	static boolean ErrorCommand(const ALString& sErrorLog);
	static boolean TaskProgressionCommand(const ALString& sTaskFile);

	// Fermeture des fichiers d'entree-sortie des commandes
	static void CloseCommandFiles();

	// Fonction de sortie utilisateur pour fermer les fichiers d'entre-sortie de commande
	static void ExitHandlerCloseCommandFiles(int nExitCode);

	// Lecture d'une commande
	// renvoie false si pas de commande, sinon un vecteur de chaines de caracteres representant le
	// parsing de IdentifierPath et une valeur optionnelle
	static boolean ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue);

	// Ecriture d'une commande
	static void WriteOutputCommand(const ALString& sIdentifierPath, const ALString& sValue, const ALString& sLabel);
	void WriteOutputFieldCommand(const ALString& sValue) const;
	void WriteOutputActionCommand() const;

	// Initialisation des managers de message (d'erreur et d'avancement)
	// Conditionne par le mode batch
	static void InitializeMessageManagers();

	/////////////////////////////////////////////////////////////
	// Personnalisation des fichiers de commandes en entree par des
	// paires (Search value, Replace value).
	// Chaque paire est appliquee sur les operandes de chaque ligne de commande en
	// entree avant son execution

	// Ajout d'une paire (SearchValue, ReplaceValue) pour la personnalisation des commandes en entree
	static void AddInputSearchReplaceValues(const ALString& sSearchValue, const ALString& sReplaceValue);

	// Application des recherche/remplacement de valeurs successivement sur une commande
	static const ALString ProcessSearchReplaceCommand(const ALString& sInputCommand);

	// Nombre de paires de personnalisation
	static int GetInputSearchReplaceValueNumber();

	// Acces aux valeurs d'une paire de personnalisation
	static const ALString& GetInputSearchValueAt(int nIndex);
	static const ALString& GetInputReplaceValueAt(int nIndex);

	// Nettoyage de toutes les paires
	static void DeleteAllInputSearchReplaceValues();

	// Verifie que les noms de fichiers passes en parametres sont coherents
	static boolean CheckOptions(const ObjectArray& oaOptions);

	// Attributs d'instance
	ALString sLabel;
	ALString sIdentifier;
	ALString sHelpText;
	ALString sStyle;
	UIObject* uioParent;
	ALString sParameters;
	boolean bVisible;
	char cShortCut;

	// Attributs de classe
	static int nUIMode;
	static int nInstanceNumber;
	static boolean bVerboseCommandReplay;
	static boolean bBatchMode;
	static boolean bTextualInteractiveModeAllowed;
	static ALString sIconImageJarPath;
	static ALString sLocalInputCommandsFileName;
	static ALString sLocalOutputCommandsFileName;
	static ALString sInputCommandFileName;
	static ALString sOutputCommandFileName;
	static ALString sLocalErrorLogFileName;
	static ALString sErrorLogFileName;
	static ALString sTaskProgressionLogFileName;
	static CommandLine commandLineOptions;

	// Fichiers de gestion des scenarii
	static FILE* fInputCommands;
	static FILE* fOutputCommands;

	// Redirection de la sortie outputCommand vers la console
	static boolean bPrintOutputInConsole;

	// Gestion des chaines des patterns a remplacer par des valeurs dans les fichiers d'input de scenario
	static StringVector svInputCommandSearchValues;
	static StringVector svInputCommandReplaceValues;

	// Dictionnaire de bufferisation des commandes de selection d'index dans les listes
	// Permet de ne memoriser que la derniere selection avant une action, et evite
	// ainsi de surcharger les fichiers de scenarios par de nombreux changements d'index
	static ObjectDictionary odListIndexCommands;

	// Vrai si la bibliotheque dynamique jvm.dll/so a ete chargee
	static boolean bIsJVMLoaded;

	// Handle vers la DLL jvm
	static void* jvmHandle;

	friend class KWLearningProject; // Pour acces a static  sLocalErrorLogFileName et sErrorLogFileName et
					// bIsHDFSerrorLogFileName;
};

// Fonction d'affichage d'une ligne de message permettant de parametrer
// l'affichage des erreurs.
// Si la classe Error n'a pas de parametrage specifique (different de NULL ou defaut), cette fonction
// est automatiquement parametree des qu'un object d'interface est instancie
void UIObjectDisplayErrorFunction(const Error* e);

/////////////////////////////////////////////////////////////////////////
// Classe UITaskProgression
// Implementation d'une interface utilisateur de suivi de progression des
// tache, avec une sous-classe de TaskProgressionManager.
// Cette classe est inactive en mode UIObject::Textual.
// Si la classe TaskProgression n'a pas de manager specifie, cette classe
// est automatiquement parametree des qu'un object d'interface est instancie
class UITaskProgression : public TaskProgressionManager
{
public:
	// Redefinition des methodes virtuelles de TaskProgressionManager
	void Start() override;
	boolean IsInterruptionRequested() override;
	void Stop() override;
	void SetLevelNumber(int nValue) override;
	void SetCurrentLevel(int nValue) override;
	void SetTitle(const ALString& sValue) override;
	void SetMainLabel(const ALString& sValue) override;
	void SetLabel(const ALString& sValue) override;
	void SetProgression(int nValue) override;
	boolean IsInterruptionResponsive() const override;

	// Acces au gestion de suivi global
	static UITaskProgression* GetManager();

	///// Implementation
protected:
	// Acces a l'objet java GUITaskProgression
	jobject GetGuiTaskProgressionManager();
};

//////////////////////////////////////////////////
// Classe UIData
// Donnees de l'interface utilisateur
class UIData : public UIObject
{
public:
	// Indique si un composant est editable
	// Pour un champs terminal: champs read-write ou read-only
	// Pour une fiche: aucun effet direct
	// Pour une liste: liste en consultation ou en mise a jour
	// Pour une fiche ou une liste, propagation du caractere editable aux sous-composants
	//   au moment de l'appel de SetEditable
	virtual void SetEditable(boolean bValue);
	boolean GetEditable() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class UIUnit;
	friend class UICard;
	friend class UIList;

	// Constructeur
	UIData();

	// Indique si les donnees sont terminales (classe UIElement)
	virtual boolean IsElement() const;

	// Indique si la donnees est presentable sous forme liste
	virtual boolean IsListable() const = 0;

	// Gestion des conteneurs de valeurs des UIUnit (cf UIUnit pour l'explication sur la
	// gestion des definitions de l'interface, et des valeurs correspondantes)
	enum
	{
		Card,
		List,
		Boolean,
		Char,
		Int,
		Double,
		String
	};
	virtual int GetDataType() const = 0;
	virtual Object* ValueContainerCreate() = 0;
	virtual void ValueContainerAddItem(Object* container) = 0;
	virtual void ValueContainerInsertItemAt(Object* container, int nIndex) = 0;
	virtual void ValueContainerRemoveItemAt(Object* container, int nIndex) = 0;

	// Affichage en mode textuel, dans une fiche ou dans une liste
	virtual void TextualCardDisplay(Object* container, int nIndex) = 0;
	virtual void TextualListDisplay(Object* container, int nIndex) = 0;

	// Ajout d'un champs de donnee en mode graphique dans l'objet Java associe
	virtual void GraphicAddField(jobject guiUnit) = 0;

	// Style effectif qui peut etre different selon le caractere editable ou non
	virtual const ALString GetActualStyle() const;

	// Attributs d'instance
	boolean bEditable;
};

// Prototype d'une methode handler d'action
typedef void (UIUnit::*ActionMethod)(void);

//////////////////////////////////////////////////
// Classe UIUnit
// Unite de presentation
class UIUnit : public UIData
{
public:
	// Ouverture de l'unite d'interface
	virtual void Open();
	boolean IsOpened() const;

	// Redefinitaion du SetEditable (pour la propagation aux composants)
	void SetEditable(boolean bValue) override;

	/////////////////////////////////////////////////////////////////////
	// Definition de l'interface
	// La definition ne peut pas etre modifiee quand l'unite est ouverte

	// Ajout de donnees elementaires
	void AddBooleanField(const ALString& sFieldId, const ALString& sFieldLabel, boolean bDefaultValue);
	//
	void AddCharField(const ALString& sFieldId, const ALString& sFieldLabel, char cDefaultValue);
	//
	void AddIntField(const ALString& sFieldId, const ALString& sFieldLabel, int nDefaultValue);
	//
	void AddRangedIntField(const ALString& sFieldId, const ALString& sFieldLabel, int nDefaultValue, int nMin,
			       int nMax);
	//
	void AddDoubleField(const ALString& sFieldId, const ALString& sFieldLabel, double dDefaultValue);
	//
	void AddRangedDoubleField(const ALString& sFieldId, const ALString& sFieldLabel, double dDefaultValue,
				  double dMin, double dMax);
	//
	void AddStringField(const ALString& sFieldId, const ALString& sFieldLabel, const ALString& sDefaultValue);
	//
	void AddRangedStringField(const ALString& sFieldId, const ALString& sFieldLabel, const ALString& sDefaultValue,
				  int nMinLength, int nMaxLength);

	// Ajout d'une action
	// L'action doit etre implemente dans une sous-classe avec le
	// prototype de ActionMethod
	void AddAction(const ALString& sActionId, const ALString& sFieldLabel, ActionMethod amActionMethod);

	///////////////////////////////////////////////////////////////
	// Acces generiques aux champs et aux actions,
	// par leur identifiant ou par leur index
	// Permet de modifier le style, la visibilite ou le caractere editable
	// d'un champs
	// Permet de modifier le style ou la visibilite d'un action

	// Nombre de champs
	int GetFieldNumber() const;

	// Acces a un champs par son index
	UIData* GetFieldAtIndex(int nFieldIndex);

	// Recherche de l'index d'un champ
	// Retourne -1 si index non trouve
	int GetFieldIndex(const ALString& sKey) const;

	// Acces a un champs par son identifiant
	UIData* GetFieldAt(const ALString& sFieldId);

	// Destruction d'un champ
	void DeleteFieldAt(const ALString& sFieldId);

	// Deplacement d'un champ en indiquant l'identifiant de son champ suivant (vide pour aller en fin)
	// Provoque un decallage des autres champs
	// Permet de remaquetter une unite de presentation
	void MoveFieldBefore(const ALString& sFieldId, const ALString& sNextFieldId);

	// Nombre d'actions
	int GetActionNumber() const;

	// Acces a une action par son index
	UIAction* GetActionAtIndex(int nActionIndex);

	// Recherche de l'index d'un champ
	// Retourne -1 si index non trouve
	int GetActionIndex(const ALString& sKey) const;

	// Acces a une action par son identifiant
	UIAction* GetActionAt(const ALString& sActionId);

	// Destruction d'une action
	void DeleteActionAt(const ALString& sActionId);

	// Deplacement d'une action en indiquant l'identifiant de son action precedente (vide pour aller en fin)
	// Provoque un decallage des autres actions
	// Permet de remaquetter une unite de presentation
	void MoveActionBefore(const ALString& sActionId, const ALString& sNextActionId);

	///////////////////////////////////////////////////////////////
	// Acces en consultation et mise a jour des donnees manipulees
	// par l'unite d'interface
	// L'acces se fait par l'identifiant du champs
	// Le type du champs doit etre compatible avec la methode utilisee

	void SetBooleanValueAt(const ALString& sFieldId, boolean bValue);
	boolean GetBooleanValueAt(const ALString& sFieldId) const;
	//
	void SetCharValueAt(const ALString& sFieldId, char cValue);
	char GetCharValueAt(const ALString& sFieldId) const;
	//
	void SetIntValueAt(const ALString& sFieldId, int nValue);
	int GetIntValueAt(const ALString& sFieldId) const;
	//
	void SetDoubleValueAt(const ALString& sFieldId, double dValue);
	double GetDoubleValueAt(const ALString& sFieldId) const;
	//
	void SetStringValueAt(const ALString& sFieldId, const ALString& sValue);
	const ALString& GetStringValueAt(const ALString& sFieldId) const;

	// Execution d'une action d'apres son code
	void ExecuteUserActionAt(const ALString& sActionId);

	//////////////////////////////////////////////////////////////////////////
	// Methodes rapide d'acquisition de donnees, par ouverture d'une boite
	// de dialogue monochamps et attente de la saisie utilisateur

	boolean GetBooleanValue(const ALString& sFieldLabel, boolean bInitialValue);
	//
	char GetCharValue(const ALString& sFieldLabel, char cInitialValue);
	//
	int GetIntValue(const ALString& sFieldLabel, int nInitialValue);
	//
	int GetRangedIntValue(const ALString& sFieldLabel, int nInitialValue, int nMin, int nMax);
	//
	double GetDoubleValue(const ALString& sFieldLabel, double dInitialValue);
	//
	double GetRangedDoubleValue(const ALString& sFieldLabel, double dInitialValue, double dMin, double dMax);
	//
	const ALString& GetStringValue(const ALString& sFieldLabel, const ALString& sInitialValue);
	//
	const ALString& GetRangedStringValue(const ALString& sFieldLabel, const ALString& sInitialValue, int nMinLength,
					     int nMaxLength);

	/////////////////////////////////////////////////////////////////////
	// Handler redefinissable dans des sous-classes
	// Permet de synchroniser l'unite d'interface et les donnees
	// qu'elle represente
	// Par defaut: ces handlers ne font rien

	// Rafraichissement de l'interface par les donnees
	// Cet handler est appele avant l'ouverture et apres chaque action,
	// de l'unite d'interface vers ses sous-unites
	virtual void EventRefresh();

	// Mise a jour des donnees par l'interface
	// Cet handler est appele apres la fermeture et avant chaque action,
	// des sous unites vers l'unite d'interface
	virtual void EventUpdate();

	// Indicateur de fraicheur des donnees contenues
	// (incremente des qu'il y des modifications de valeurs dans un des
	// champs ou dans une sous-fiche ou sous-liste)
	int GetFreshness() const;

	// Verification de l'integrite (recursivement aux sous-composants)
	boolean Check() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur
	UIUnit();
	~UIUnit();

	// Propagation des refresh et des updates
	void PropagateRefresh();
	void PropagateUpdate();

	// Action de sortie de l'interface
	// Identifiant de l'action: "Exit"
	void ActionExit();

	// Fermeture de la fenetre par l'action predefinie "Exit"
	// Cet handler est appele apres la fermeture
	// Attention: usage avance a n'utiliser que si vraiment necessaire
	// Ne doit effectuer qu'un traitement atomique
	virtual void EventExit();

	// Action de rafraichissement de l'interface
	// Identifiant de l'action: "Refresh"
	void ActionRefresh();

	///////////////////////////////////////////////////////////////////////
	// Definition de l'interface

	// Ajout d'un champs dans l'interface, (avec gestion des conteneurs de valeur)
	void AddField(UIData* uiElement);

	// Reimplementation des methodes virtuelles de UIData
	boolean IsListable() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;

	// Gestion des actions
	// On renvoie true tant que l'action executee n'est pas l'action de sortie
	boolean TextualDealWithActions();

	// Attente de la saisie utilisateur d'un index d'action
	int TextualReadActionIndex();

	// En mode graphique, fabrication de l'objet Java associe
	jobject GraphicBuildGUIUnit();

	////////////////////////////////////////////////////////////////////
	// Definition de l'interface
	// Les donnees et les actions sont definies dans un dictionnaire
	// pour etre accessible par cle, et dans un tableau, pour preserver
	// l'ordre de creation

	// Test d'existence d'une cle
	boolean IsKeyUsed(const ALString& sKey) const;

	// Methodes utilitaires de gestion de l'enregistrement des commandes
	// (utilisee pour l'interface Java)
	void WriteOutputUnitFieldCommand(const ALString& sFieldId, const ALString& sValue) const;
	void WriteOutputUnitActionCommand(const ALString& sActionId) const;
	void WriteOutputUnitListIndexCommand(const ALString& sValue) const;
	friend void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitFieldCommand(JNIEnv* env, jobject guiUnit,
										    jstring fieldId, jstring value);
	friend void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitActionCommand(JNIEnv* env, jobject guiUnit,
										     jstring actionId);
	friend void JNICALL Java_normGUI_engine_GUIUnit_writeOutputUnitListIndexCommand(JNIEnv* env, jobject guiUnit,
											jstring sValue);

	// Conteneurs de definition de l'interface
	ObjectDictionary odUIObjects;
	ObjectArray oaUIDatas;
	ObjectArray oaUIActions;

	// Indicateur d'ouverture
	boolean bIsOpened;

	///////////////////////////////////////////////////////////////////////
	// Valeur des donnees de l'interface
	// Ces donnees sont gerees de la meme facon pour une liste et une fiche
	// Il s'agit de faciliter les modifications de definition de l'interface
	// et des donnees sous forme fiche, de minimiser l'encombrement memoire
	// sous forme liste.
	// On choisit d'avoir un tableau de conteneurs types par variables:
	//      boolean, char, int -> IntVector
	//      double -> DoubleVector
	//      ALString -> StringVector
	// et seulement dans le cas d'une fiche
	//      UICard -> UICard (pas de tableau car interdit dans une liste)
	//      UIList -> UIList (pas de tableau car interdit dans une liste)
	// La gestion de ces conteneurs est faites au moyen de methodes
	// virtuelles definies dans UIData
	ObjectArray oaUnitValues;

	// Index de l'item courant (0 pour les fiches)
	int nCurrentItemIndex;

	// Nombre d'item (1 pour les fiches)
	int nItemNumber;

	// Indicateur de fraicheur, incremente pour toute modification des valeurs
	int nFreshness;

	friend class UICard;
	friend class UIList;
	friend class UIObjectArrayView;
};

//////////////////////////////////////////////////
// Classe UICard
// Presentation de donnees en mode fiche
// Une grande partie des methodes utiles proviennent de UIUnit
class UICard : public UIUnit
{
public:
	// Constructeur
	UICard();
	~UICard();

	///////////////////////////////
	// Definition de l'interface

	// Style: par defaut vide
	// Valeurs possibles:
	//   vide: fenetre standard avec des sous-fiches et sous-listes
	//   TabbedPanes: fenetre avec un onglet par sous-fiche ou sous-liste
	//
	// Parametres des styles: par defaut vide
	// Valeurs possibles:
	//   vide: une bordure est affichee autour de la fiche ou de la liste, avec son libelle
	//   NoBorder: il n'y a pas de bordure, et les champs sont maquettes comme les champs
	//             de la fiche appelante

	// Ajout de donnees de type unite d'interface
	// Memoire: les objets pases en parametres deviennent propriete
	// de l'UICard agregeante (l'appelant doit les allouer, mais pas les liberer)
	void AddCardField(const ALString& sFieldId, const ALString& sFieldLabel, UICard* card);
	void AddListField(const ALString& sFieldId, const ALString& sFieldLabel, UIList* list);

	// Remplacement d'une donnee de type unite d'interface, en reprenant toutes ses caracteristiques:
	// identifiant, libelle, help text, style, parametres, short cut...
	// Memoire: l'ancienne version est detruite, la nouvelle appartient a l'UICard agregeante
	void ReplaceCardField(const ALString& sFieldId, UICard* newCard);
	void ReplaceListField(const ALString& sFieldId, UIList* newList);

	///////////////////////////////////////////////////////////////
	// Acces en consultation et mise a jour des donnees manipulees
	// par l'unite d'interface
	// Il faut acceder a l'unite d'interface, puis utiliser son API
	// pour acceder a ses champs

	UICard* GetCardValueAt(const ALString& sFieldId);
	UIList* GetListValueAt(const ALString& sFieldId);

	// Verification de l'integrite
	boolean Check() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	int GetDataType() const override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
};

////////////////////////////////////////////////////////////////////////
// Classe UIConfirmationCard
// Fiche contenant deux actions Ok et Exit sous forme de boutons
// Le libelle de la fenetre (par defaut "Confirmation") est parametrable
// (ainsi que son identifiant)
// Les libelles par defaut "Ok" "Annuler" sont parametrables avec
// les methodes de UIAction
class UIConfirmationCard : public UICard
{
public:
	// Constructeur
	UIConfirmationCard();
	~UIConfirmationCard();

	// Ouverture avec en retour le resultat de confirmation
	boolean OpenAndConfirm();

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Action OK redefinie
	void OK();

	// Resultat de confirmation
	boolean bConfirmed;
};

////////////////////////////////////////////////////////////////////////
// Classe UIQuestionCard
// Fiche ouvrant permettant de poser une quetsion a l'utilisateur
class UIQuestionCard : public UICard
{
public:
	// Constructeur
	UIQuestionCard();
	~UIQuestionCard();

	// Ouverture d'une boite de dialogue permettant de choisir un nom de fichier ou de repertoire
	// Entree:
	//   . DialogBoxTitle: titre de la boite de dialogue
	//   . QuestionType: "Information", "Question", "Message", "Warning", "Error"
	//   . Question: libelle de la question
	// Sortie: true si Yes, false si No, ou sortie de la boite de dialogue
	boolean GetAnswer(const ALString& sDialogBoxTitle, const ALString& sQuestionType, const ALString& sQuestion);

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Cette fonctionnalite gere les scenarios de facon compatible entre une fiche standard de type UICard et
	// une fiche dediee geree de facon specifique cote java

	// Redefinition de l'action de sortie, associee au choix No
	void EventExit() override;

	// Action OK, associee au choix Yes
	void OK();

	// Resultat de confirmation
	boolean bConfirmed;
};

////////////////////////////////////////////////////////////////////////
// Classe UIFileChooserCard
// Fiche ouvrant permettant de choisir un nom de fichier.
class UIFileChooserCard : public UICard
{
public:
	// Constructeur
	UIFileChooserCard();
	~UIFileChooserCard();

	// Style: valeurs possibles
	//  FileChooser: ouverture directe du FileChooser (defaut)
	//  vide: ouverture d'une boite de dialogue ne comportant qu'un seul champ
	// Le style vide est utile pur des environnement de type cloud, pour permettre de specifier un path dans
	// le systeme de gestion de fichier du cloud. Ainsi, le champ de la boite de dialogue peut etre saisi soit
	// directement avec un URI, soit via un bouton FileChooser

	// Style par defaut (par defaut: FileChooser)
	// Permet de changer le style utilise lors de la creation des objets UIFileChooserCard
	static void SetDefaultStyle(const ALString& sValue);
	static const ALString& GetDefaultStyle();

	// Ouverture d'une boite de dialogue permettant de choisir un nom de fichier ou de repertoire
	// Entree:
	//   . DialogBoxTitle: titre de la boite de dialogue
	//   . ApproveActionLabel: libelle du bouton de l'action de validation du choix dans la boite de dialogue
	//   . ChooseType: type de choix, "FileChooser" ou "DirectoryChooser"
	//   . ChooseParameters: parametre de choix des type de fichier, separe par '\n' (cf. Style FileChooser de
	//   UIStringElement)
	//     (exemple: "Data\ntxt\ncsv" produit le libelle "Data Files (*.txt;*.csv)" dans le FileChooser avec le
	//     filtre correspondant)
	//   . PathFieldIdentifier: identifiant du champ contenant le chemin, pour enregistrement dans les scenarios
	//   . PathFieldLabel: libelle du champ contenant le chemin, pour enregistrement dans les scenarios
	//   . InputFilePath: chemin du fichier ou repertoire en entree
	// Sortie: path du fichier ou repertoire choisi en sortie, vide si l'utilisateur est sorti par Cancel
	const ALString ChooseFile(const ALString& sDialogBoxTitle, const ALString& sApproveActionLabel,
				  const ALString& sChooseType, const ALString& sChooseParameters,
				  const ALString& sPathFieldIdentifier, const ALString& sPathFieldLabel,
				  const ALString& sInputFilePath);

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametrage avance

	// Aide sur l'action de de validation
	void SetApproveActionHelpText(const ALString& sValue);
	const ALString& GetApproveActionHelpText();

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Cette fonctionnalite gere les scenarios de facon compatible ascendante avec les anciennes fiches de type
	// UIConfirmationCard utilisees avec un seul champ de type UIStringElement et de stule FileChooser ou
	// DirectoryChooser Dans la precendente version, on ouvrait un boite de dialogue avec un seul champ, permettant
	// d'ouvrir le FileChooser. Dans cette version, on ouvre directement le FileChooser du systeme, ce  qui
	// simplifie l'ergonomie en faisant economiser deux clics, pour ouvir et fermer le FileChooser

	// Redefinition de l'action de sortie, associee au choix No
	void EventExit() override;

	// Action OK, pour compatibilite ascendante avec les anciennes boite de dialogue
	void OK();

	// Resultat de confirmation
	boolean bConfirmed;

	// Style par defaut
	static ALString sDefaultStyle;
};

///////////////////////////////////////////////////////////////////
// Classe UIObjectView
// Edition d'un objet
// La classe gere un objet qui lui est passe en parametre
// Moyennant l'implementation de quelques methodes a
// reimplementer dans des sous-classes, cet objet est
// synchronise avec l'interface pour les actions utilisateur
// L'API generique (Get/SetObject) de cette classe permet de
// l'utiliser pour implementer des composants d'interface
// de plus haut niveau
class UIObjectView : public UICard
{
public:
	// Constructeur
	// Le constructeur permet de decrire les champs de l'interface
	UIObjectView();
	~UIObjectView();

	// Mise en place de l'objet (qui peut etre NULL)
	// La methode stocke l'objet passe en parametre, puis appelle EventRefresh
	virtual void SetObject(Object* object);

	// Acces a l'objet gere
	// Cette methode appelle EventUpdate avant de rendre l'objet
	virtual Object* GetObject();

	// Acces direct a l'objet gere, sans declencher le EventUpdate
	virtual Object* GetRawObject();

	//////////////////////////////////////////////////////////////////////////////
	// Methodes a reimplementer obligatoirement
	// Ces methodes ne sont appelee (automatiquement) avec l'objet courant,
	// uniquement si ce dernier est non NULL.
	// Il est necessaire de caster l'objet pour effectuer les transferts de valeur

	// Mise a jour de l'objet par les valeurs de l'interface
	virtual void EventUpdate(Object* object) = 0;

	// Mise a jour des valeurs de l'interface par l'objet
	virtual void EventRefresh(Object* object) = 0;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Rafraichissement de l'interface par les donnees
	void EventRefresh() override;

	// Mise a jour des donnees par l'interface
	void EventUpdate() override;

	// Objet edite a gerer
	Object* objValue;
};

//////////////////////////////////////////////////
// Classe UIList
// Presentation de donnees en mode liste
// Une grande partie des methodes utiles proviennent de UIUnit
class UIList : public UIUnit
{
public:
	// Constructeur
	UIList();
	~UIList();

	// Ergonomie de la liste: par defaut CellEditable
	// Valeurs possibles:
	//    CellEditable: les cellules sont editbales directement, comme dans Excel
	//    Inspectable: les cellules sont non editable, et une action d'inspection
	//       permet d'ouvrir une sous-fiche d'edition des champs d'une ligne
	// Le choix de l'ergonomie est une facilite correspondant en fait au parametrage
	// du caractere Editable des champs de la liste et du caractere Visible
	// des actions InsertItemBefore, InsertItemAfter, InspectItem, RemoveItem
	// (seule l'action InspectItem est concernee si la liste est Editable)
	int ergonomy;
	enum
	{
		CellEditable,
		Inspectable
	};
	void SetErgonomy(int nValue);
	int GetErgonomy() const;

	// Definition du nombre de lignes a affichees (defaut: 0, comportement automatique)
	void SetLineNumber(int nValue);
	int GetLineNumber() const;

	// Definition de la largeur additionnelle en nombre de caracteres, affichee en plus dans la derniere colonne
	// (defaut: 0)
	void SetLastColumnExtraWidth(int nValue);
	int GetLastColumnExtraWidth() const;

	// Redefinition du SetEditable (pour la propagation aux composant et les
	// actions d'edition de la liste)
	void SetEditable(boolean bValue) override;

	///////////////////////////////////////////////////////////////
	// Parametrage d'un champ cle pour l'enregistrement des scenario
	// Par defaut, la selection d'une ligne est memorise par son index
	// dans les scenarios (cf "List.Index")
	// En specifiant un champ cle (de type chaine de caracteres), on
	// permet de memoriser la selection d'un ligne par la valeur de
	// ce champ cle (cf "List.Key")
	// Lorsqu'un scenario est rejoue, la premiere ligne dont la cle
	// correspond est selectionnee (sinon, on utilise la premiere ligne)

	// Specification du champ a utiliser pour la cle (vide par defaut)
	void SetKeyFieldId(const ALString& sFieldId);
	const ALString& GetKeyFieldId() const;

	////////////////////////////////////////////////////////////////////
	// La description des champs de la liste se fait en utilisant l'API
	// de UIUnit

	///////////////////////////////////////////////////////////////
	// Acces en consultation et mise a jour des donnees manipulees
	// par l'unite d'interface
	// On trouve ici les methodes pour gerer les items de la liste
	// (ajout, suppression, selection de l'item courant)
	// La modification des champs se fait sur l'item courament
	// selectionne de la liste

	int GetItemNumber();

	// Ajout d'item: l'item ajoute est automatiquement selectionne
	void AddItem();
	void InsertItemAt(int nIndex);

	// Supresion d'item
	void RemoveItemAt(int nIndex);
	void RemoveAllItems();

	// Gestion de l'item courant. Entre -1 si aucune selection et
	// nItemNumber-1. L'item courant est utilise pour les parcours, il est mis
	// a jour avec l'item selectionne a chaque changement d'index au niveau de
	// l'interface.
	void SetCurrentItemIndex(int nIndex);
	int GetCurrentItemIndex() const;

	// Gestion de l'item graphiquement selectionne. Entre -1 si aucune
	// selection et nItemNumber-1. Il permet de lancer une action sur l'item
	// selectionne. L'item selectionne met a jour l'item courant a chaque
	// changement de selection.
	void SetSelectedItemIndex(int nIndex);
	int GetSelectedItemIndex() const;

	// Gestion de l'item graphiquement selectionne, en utilisant
	// le champ cle (optionnel)
	void SetSelectedItemKey(const ALString& sKey);
	const ALString GetSelectedItemKey() const;

	/////////////////////////////////////////////////////////////////////
	// Handler redefinissable dans des sous-classes
	// Permet de synchroniser l'unite d'interface et les donnees
	// qu'elle represente
	// Par defaut: ces handlers ne font rien

	// Insertion d'un item par l'utilisateur
	// L'item est insere a l'interface, et ce handler, appele apres l'insertion,
	// permet de creer un item de donnee correspondant
	virtual void EventInsertItemAt(int nIndex);

	// Mise a jour d'un item par l'utilisateur
	// Ce handler est appele apres les mise a jour utilisateurs sur un item
	virtual void EventUpdateItemAt(int nIndex);

	// Suppression d'un item par l'utilisateur
	// L'item est supprime a l'interface, et ce handler, appele avant la suppression,
	// permet de detruire l'item de donnee correspondant
	virtual void EventRemoveItemAt(int nIndex);

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////
	// Personnalisation de la fiche de saisie d'un item du tableau
	// Changement de fiche a utiliser pour les mises a jour
	// Le type d'objet gere par cette fiche doit etre le meme que celui du tableau d'objets
	// Par defaut, ou si NULL, cette fiche est geree automatiquement
	// Memoire: la fiche passee en parametre devient geree par l'objet, et
	// doit avoir ete allouee par l'appelant
	void SetItemCard(UICard* view);

	// Consultation de la fiche courament utilisee (NULL si aucune)
	UICard* GetItemCard();

	///// Implementation
protected:
	// Gestion des parametres sous la forme de deux champs
	// La valeurs des champs est 0 s'ils ont leur valeur par defaut
	void ReadParameters(int& nLineNumber, int& nLastColumnExtraWith) const;
	void WriteParameters(int nLineNumber, int nLastColumnExtraWith);

	/////////////////////////////////////////////////////////////////////
	// Actions predefinies d'edition de la liste
	// Ces actions sont inactivee (par leur visibilite) quand la liste est non editable
	// Les handler Event... sont declenchee par ces actions
	// Redefinissable dans des sous-classes

	// Action d'insertion d'un item avant la position courante
	// Identifiant: "InsertItemBefore"
	virtual void ActionInsertItemBefore();

	// Action d'insertion d'un item apres la position courante
	// Identifiant: "InsertItemAfter"
	virtual void ActionInsertItemAfter();

	// Action de mise a jour de l'item
	// Identifiant: "InspectItem"
	virtual void ActionInspectItem();

	// Action de suppression de l'item a la position courante
	// Identifiant: "RemoveItem"
	virtual void ActionRemoveItem();

	// Selection en mode textuel d'un item de la liste
	void TextualSelectItemIndex();

	// Reimplementation des methodes virtuelles de UIData
	int GetDataType() const override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	int nSelectedItemIndex;

	// Fiche de mise a jour
	UICard* itemCard;

	// Identifiant du champ cle
	ALString sKeyFieldId;

	// Creation de la sous fiche par defaut
	void CreateItemCard();
};

///////////////////////////////////////////////////////////////////
// Classe UIObjectArrayView
// Edition d'un tableau d'objet
// La classe gere un tableau d'objets qui lui est passe en parametre
// Moyennant l'implementation de quelques methodes virtuelles a
// reimplementer dans des sous-classes, ce tableau d'objet est
// synchronise en permanence avec l'interface pour les actions utilisateur
// L'API generique (Get/SetObjectArray) de cette classe permet de
// l'utiliser pour implementer des composants d'interface
// de plus haut niveau
class UIObjectArrayView : public UIList
{
public:
	// Constructeur
	// Le constructeur permet de decrire les champs de l'interface
	UIObjectArrayView();
	~UIObjectArrayView();

	// Mise en place du tableau d'objets
	// L'interface se reactualise en fonction du contenu de ce tableau
	virtual void SetObjectArray(ObjectArray* value);

	// Acces au tableau d'objets gere
	virtual ObjectArray* GetObjectArray();

	// Copie (si possible) des textes d'aide present
	// dans la fiche
	void CopyCardHelpTexts();

	///////////////////////////////////////////////////////////////////
	// Methodes a reimplementer obligatoirement dans les sous-classes
	// Ces methodes ne sont appelee (automatiquement) avec l'objet correspondant
	// a l'index courant (CurrentItemIndex), uniquement si cet objet est non NULL.
	// Il est necessaire de caster l'objet pour effectuer les transferts de valeur

	// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur
	virtual Object* EventNew() = 0;

	// Mise a jour de l'objet correspondant a l'index courant par les valeurs de l'interface
	virtual void EventUpdate(Object* object) = 0;

	// Mise a jour des valeurs de l'interface par l'objet correspondant a l'index courant
	virtual void EventRefresh(Object* object) = 0;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////
	// Personnalisation de la fiche de saisie d'un item du tableau
	// Changement de fiche a utiliser pour les mises a jour
	// Le type d'objet gere par cette fiche doit etre le meme que celui du tableau d'objets
	// Par defaut, ou si NULL, cette fiche est geree automatiquement
	// Memoire: la fiche passee en parametre devient geree par l'objet, et
	// doit avoir ete allouee par l'appelant
	void SetItemView(UIObjectView* view);

	// Consultation de la fiche courament utilisee (NULL si aucune)
	UIObjectView* GetItemView();

	///// Implementation
protected:
	// Creation (par EventNew) d'un objet correspondant a l'index, et insertion
	// dans le tableau
	void EventInsertItemAt(int nIndex) override;

	// Destruction de l'objet correspondant a l'index, et supression de l'objet du tableau
	void EventRemoveItemAt(int nIndex) override;

	// Rafraichissement de l'interface par les donnees
	void EventRefresh() override;

	// Mise a jour des donnees par l'interface
	void EventUpdate() override;

	// Action de mise a jour de l'item
	// Identifiant: "InspectItem"
	void ActionInspectItem() override;

	// Tableau a gerer
	ObjectArray* array;

	// Fiche de mise a jour
	UIObjectView* itemView;
};

//////////////////////////////////////////////////////////
// Classe UIElement
// Donnee elementaire
// Cette classe interne et ses classes heritieres sont utilisees
// par la classe UIUnit
// pour memoriser sa composition en champs
class UIElement : public UIData
{
public:
	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Parametres de declenchement d'un refresh suite a toute modification de la donnee (defaut: false)
	boolean GetTriggerRefresh() const;
	void SetTriggerRefresh(boolean bValue);

	/////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur
	UIElement();

	// Reimplementation des methodes virtuelles de UIData
	boolean IsElement() const override;
	boolean IsListable() const override;

	// Acces rapide a la representation chaine de caracteres de la valeur d'un champs
	virtual const char* const GetFieldStringValue(Object* container, int nIndex) = 0;
	virtual void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) = 0;

	// Duplication de l'element, rendu sous forme generique de UIElement
	virtual UIElement* CloneAsElement() const = 0;

	friend class UIUnit;
	friend class UIList;

	// Declenchement d'un refressh suite a modification de valeur
	boolean bTriggerRefresh;
};

//////////////////////////////////////////////////////////
// Classe UIStringElement
// Donnee elementaire de type ALString
class UIStringElement : public UIElement
{
public:
	// Constructeur
	UIStringElement();
	~UIStringElement();

	// Style: par defaut TextField (ou vide)
	// Valeurs possibles: TextField, ComboBox, EditableComboBox,
	//                    HelpedComboBox, FileChooser, DirectoryChooser, Password, RadioButton
	//                    TextArea, FormattedLabel, SelectableLabel, UriLabel
	//
	// Pour le style ComboBox, EditableComboBox ou RadioButton, les Parameters contiennent
	//   la liste des valeurs separees par des '\n' (exemple: "Red\nBlue\nYellow")
	//
	// Pour le style HelpedComboBox, les Parameters contiennent le nom d'un champ
	//   d'une liste dont le chemin est specifie dans la fenetre (exemple: "myCard.mylist:myfield).
	//
	// Pour le style FileChooser, les Parameters contiennent un libelle de type de fichier suivi
	//   d'une liste d'extension, chaque parametre etant separe par '\n'
	//   (exemple: "Data\ntxt\ncsv" produit le libelle "Data Files (*.txt;*.csv)" dans le FileChooser avec le filtre
	//   correspondant)
	//
	// Pour les styles FormattedLabel, SelectableLabel, UrlLabel, le champ est affiche avec son libele et sa valeur
	// sur deux colonnes
	//   comme dans les autres cas. Si le libelle est vide, le champ utilise toute la ligne.
	// Pour le style TextArea, le champ utilise systematiquement toute la ligne. Le libelle, s'il est present, est
	//   affiche au dessus de la zone de texte
	//
	// Pour le style TextArea, les Parameters sont soit vides, soit il contiennent la taille de la zone de tetxte
	//   sous la forme de deux valeurs separees par des '\n'. Exemple: "10\n50"
	//
	// Pour les styles FormattedLabel, SelectableLabel le champ est presente sous la forme d'un libelle non
	// saisissable, meme en mode editable,
	//   et il peut accepter soit un texte libre, soit un format html, s'il est encadre par les balises <html> et
	//   </html>. Le style SelectableLabel est similaire au style FormattedLabel, mais le texte, non editable, peut
	//   etre selectionne
	//
	// Pour les styles FormattedLabel, les Parameters peuvent contenir un nom de fichiers image (format jpg, png,
	// gif), present dans un jar
	//    et accessible via un chemin dans le jar (exemple: images/sample.gif). Dans ce cas, l'image sera affichee
	//    de facon centree et le texte sera affiche par dessus.
	//
	// Pour le style UriLabel, les Parameters doivent contenir le nom de l'uri a utiliser (exemple:
	// "www.google.com"), et la chaine de
	//   caractere editee le code html dela zone cliquable (ex: "<html> Search engine <a href=\"\">google</a>
	//   </html>")

	// Valeur par defaut
	void SetDefaultValue(const ALString& sValue);
	const ALString& GetDefaultValue() const;

	// Longueur minimale
	void SetMinLength(int nValue);
	int GetMinLength() const;

	// Longueur maximale
	void SetMaxLength(int nValue);
	int GetMaxLength() const;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	// Controle d'integrite d'une valeur (sans messages d'erreur)
	boolean CheckValue(const ALString& sValue) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	const ALString GetActualStyle() const override;
	int GetDataType() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	const char* const GetFieldStringValue(Object* container, int nIndex) override;
	void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) override;
	UIElement* CloneAsElement() const override;

	// Attributs
	ALString sDefaultValue;
	int nMinLength;
	int nMaxLength;
};

//////////////////////////////////////////////////////////
// Classe UIBooleanElement
// Donnee elementaire de type boolean
class UIBooleanElement : public UIElement
{
public:
	// Constructeur
	UIBooleanElement();
	~UIBooleanElement();

	// Style: par defaut CheckBox (ou vide)
	// Valeurs possibles: TextField, CheckBox, ComboBox, RadioButton

	// Valeur par defaut
	void SetDefaultValue(boolean bValue);
	boolean GetDefaultValue() const;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	const ALString GetActualStyle() const override;
	int GetDataType() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	const char* const GetFieldStringValue(Object* container, int nIndex) override;
	void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) override;
	UIElement* CloneAsElement() const override;

	// Attributs
	boolean bDefaultValue;
};

//////////////////////////////////////////////////////////
// Classe UICharElement
// Donnee elementaire de type char
class UICharElement : public UIElement
{
public:
	// Constructeur
	UICharElement();
	~UICharElement();

	// Style: par defaut TextField (ou vide)
	// Valeurs possibles: TextField, ComboBox, EditableComboBox, RadioButton
	//
	// Pour le style ComboBox, EditableComboBox ou RadioButton, les Parameters contiennent
	//   la liste des valeurs separees par des '\n' (exemple: "R\nG\nB")

	// Valeur par defaut
	void SetDefaultValue(char cValue);
	char GetDefaultValue() const;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	const ALString GetActualStyle() const override;
	int GetDataType() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	const char* const GetFieldStringValue(Object* container, int nIndex) override;
	void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) override;
	UIElement* CloneAsElement() const override;

	// Attributs
	char cDefaultValue;
};

//////////////////////////////////////////////////////////
// Classe UIIntElement
// Donnee elementaire de type int
class UIIntElement : public UIElement
{
public:
	// Constructeur
	UIIntElement();
	~UIIntElement();

	// Style: par defaut TextField (ou vide)
	// Valeurs possibles: TextField, ComboBox, EditableComboBox, RadioButton, Spinner, Slider
	//
	// Pour le style ComboBox, EditableComboBox ou RadioButton, les Parameters contiennent
	//   la liste des valeurs separees par des '\n' (exemple: "1\n2\n3")

	// Valeur par defaut
	void SetDefaultValue(int nValue);
	int GetDefaultValue() const;

	// Valeur minimale
	void SetMinValue(int nValue);
	int GetMinValue() const;

	// Valeur maximale
	void SetMaxValue(int nValue);
	int GetMaxValue() const;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	// Controle d'integrite d'une valeur (sans messages d'erreur)
	boolean CheckValue(int nValue) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	const ALString GetActualStyle() const override;
	int GetDataType() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	const char* const GetFieldStringValue(Object* container, int nIndex) override;
	void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) override;
	UIElement* CloneAsElement() const override;

	// Attributs
	int nDefaultValue;
	int nMinValue;
	int nMaxValue;
};

//////////////////////////////////////////////////////////
// Classe UIDoubleElement
// Donnee elementaire de type double
class UIDoubleElement : public UIElement
{
public:
	// Constructeur
	UIDoubleElement();
	~UIDoubleElement();

	// Style: par defaut TextField (ou vide)
	// Valeurs possibles: TextField, ComboBox, EditableComboBox, RadioButton, Spinner
	//
	// Pour le style ComboBox, EditableComboBox ou RadioButton, les Parameters contiennent
	//   la liste des valeurs separees par des '\n' (exemple: "1\n2\n3")
	//
	// Pour le style Spinner, les Parameters contiennent le nombre de chiffre apres la virgule a a afficher (defaut:
	// 0) Par exemple, si on precise 2, les reel seront affiches au 1/100 pres et on pourra les choisir avec cette
	// precision

	// Valeur par defaut
	void SetDefaultValue(double dValue);
	double GetDefaultValue() const;

	// Valeur minimale
	void SetMinValue(double dValue);
	double GetMinValue() const;

	// Valeur maximale
	void SetMaxValue(double dValue);
	double GetMaxValue() const;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	// Controle d'integrite d'une valeur (sans messages d'erreur)
	boolean CheckValue(double dValue) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de UIData
	const ALString GetActualStyle() const override;
	int GetDataType() const override;
	Object* ValueContainerCreate() override;
	void ValueContainerAddItem(Object* container) override;
	void ValueContainerInsertItemAt(Object* container, int nIndex) override;
	void ValueContainerRemoveItemAt(Object* container, int nIndex) override;
	void TextualCardDisplay(Object* container, int nIndex) override;
	void TextualListDisplay(Object* container, int nIndex) override;
	void GraphicAddField(jobject guiUnit) override;
	const char* const GetFieldStringValue(Object* container, int nIndex) override;
	void SetFieldStringValue(Object* container, int nIndex, const char* const sValue) override;
	UIElement* CloneAsElement() const override;

	// Attributs
	double dDefaultValue;
	double dMinValue;
	double dMaxValue;
};

//////////////////////////////////////////////////
// Classe UIAction
// Action utilisateur
class UIAction : public UIObject
{
public:
	// Constructeur
	UIAction();
	~UIAction();

	////////////////////////////////////////////////////////////////////////
	// Style: par defaut vide
	// Valeurs possibles:
	//     vide: action maquettee dans les menus, et en plus dans les popup
	//           pour les actions de liste
	//     Button: action maquettee sous forme de boutton
	//     SmallButton: action maquettee sous forme de boutton de plus petite taille
	//                  et insere a gauche dans la colonne des libelles
	// Si l'on veut une action simultanement en menu et bouton, il suffit
	// de declarer deux action avec la meme methodes et deux styles differents
	//
	// L'action d'identifiant "Exit" est une action systeme automatiquement
	// associee aux fiches/listes principales, qui provoque la sortie de la
	// fenetre par fermeture de la fenetre ou par le boutton "Close" (libelle
	// par defaut de l'action "Exit").
	//
	// On peut associer une action utilisateur autre que "Exit" a un comportement
	// de fermeture de la fenetre en cours, en utilisant SetParameters("Exit")
	// En style menu, est sera positionnee en fin de menu
	//
	// Les actions "Refresh" et "Help" sont predefinies et ne doivent pas etre
	// utilisees pour definir des cations utilisateur

	// Methode a executer lorsque l'action est declenchee
	void SetActionMethod(ActionMethod method);
	ActionMethod GetActionMethod() const;

	// Cle d'acceleration pour lancer une commande
	/*! La cle est un code touche precede d'un ou plusieurs modifieurs
	(choisis parmi shift | control | ctrl | meta | alt | button1 | button2 | button3)
	Le code touche peut etre etre un code special (INSERT, DELETE...)
	Exemples: control A, shift alt DELETE */
	void SetAccelKey(const ALString& sValue);
	const ALString& GetAccelKey() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Controle d'integrite des specifications (avec messages d'erreur)
	boolean Check() const override;

	///// Implementation
protected:
	// Attributs
	ActionMethod actionMethod;

	// Commande
	ALString sAccelKey;
};

////////////////////////////////////////////////////////////////////////
// Classe SampleObjectView
// Editeur de SampleObject, pour tester UIObjectView
class SampleObjectView : public UIObjectView
{
public:
	// Constructeur
	SampleObjectView();
	~SampleObjectView();

	/////////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;
};

////////////////////////////////////////////////////////////////////////
// Classe SampleObjectArrayView
// Editeur de tableau SampleObject, pour tester UIObjectArrayView
class SampleObjectArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	SampleObjectArrayView();
	~SampleObjectArrayView();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur
	Object* EventNew() override;

	// Mise a jour de l'objet correspondant a l'index courant par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet correspondant a l'index courant
	void EventRefresh(Object* object) override;
};

/////////////////////////////////////
// Methodes en inline

inline int UIUnit::GetFieldIndex(const ALString& sKey) const
{
	int nIndex;
	UIData* uiElement;

	for (nIndex = oaUIDatas.GetSize() - 1; nIndex >= 0; nIndex--)
	{
		uiElement = cast(UIData*, oaUIDatas.GetAt(nIndex));
		if (uiElement->GetIdentifier() == sKey)
			return nIndex;
	}
	return -1;
}
