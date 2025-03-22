// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "FileService.h"

class Error;

// Prototype d'une fonction d'affichage d'une erreur a l'utilisateur
// Doit renvoyer true s'il faut afficher le message d'erreur,
// meme si le nombre d'erreur max est atteint
// Le parametre bDisplay indique si la fonction est appelee au moment de l'affichage,
// ou uniquement pour raison de tests, ce qui permet de gerer un contexte dedie
// a l'affichage effectif des erreur.
typedef boolean (*ErrorFlowIgnoreFunction)(const Error* e, boolean bDisplay);

/////////////////////////////////////////////////////////////////////////
// Gestion centralisee des erreurs                                     //
// Cette centralisation de la gestion des erreurs, et particulierement //
// de leur Display, permet une meilleurs portabilite des messages      //
//
// La classe global n'herite pas de Object (inutile: methodes statiques uniquement),
// et cela permet d'eviter le conflit avec la methode Global::AddSimpleMessage
class Global : public SystemObject
{
public:
	/////////////////////////////////////////////////////////////
	// Methodes permettant de declencher l'affichage d'une erreur

	// Message simple
	static void AddSimpleMessage(const ALString& sLabelValue);

	// Message
	static void AddMessage(const ALString& sCategoryValue, const ALString& sLocalisationValue,
			       const ALString& sLabelValue);

	// Warning
	static void AddWarning(const ALString& sCategoryValue, const ALString& sLocalisationValue,
			       const ALString& sLabelValue);

	// Erreur
	static void AddError(const ALString& sCategoryValue, const ALString& sLocalisationValue,
			     const ALString& sLabelValue);

	// Erreur fatale
	static void AddFatalError(const ALString& sCategoryValue, const ALString& sLocalisationValue,
				  const ALString& sLabelValue);

	// Objet erreur banalise
	static void AddErrorObject(const Error* eValue);

	////////////////////////////////////////////////////////
	// Controle du flux d'erreurs a emettre
	//
	// Activation/desactivation du controle de flux des erreurs
	//  Apres l'activation du controle, un compteur est
	//  initialise, et des qu'il atteint le nombre max specifie,
	//  les erreurs ne sont plus emises vers l'utilisateur.
	// (sauf une fois toutes les puissances de 10 ou tous les millions)
	//  Cette methode est utile pour des controles massifs,
	//  pour lesquels on veut informer l'utilisateur des
	//  premieres erreurs, mais pas lui envoyer des milliers
	//  de messages.
	// Les methodes ActivateErrorFlowControl et DesactivateErrorFlowControl
	// sont toujours a appeler par paire, avant et apres une gestion massive d'erreur.
	// Elles peuvent etre utilise dans des methodes s'appelant recursivement:
	//   - chaque nouvel appel a ActivateErrorFlowControl ne fait que "s'empiler"
	//   - les erreurs ne seront emises a nouveau que quand il y aura eu autant de
	//     DesactivateErrorFlowControl que d'ActivateErrorFlowControl empiles

	// Activation du controle de flux des erreurs
	// Il faut rappeler cette methode avant toute gestion
	// massive d'erreur
	static void ActivateErrorFlowControl();

	// Desactivation du controle de flux des erreurs
	// Il faut appeler cette methode apres toute gestion
	// massive d'erreur, pour reautoriser l'emission d'erreurs
	static void DesactivateErrorFlowControl();

	// Nombre max d'erreurs emises lors le controle de flux est active
	// Il s'agit d'une limite par type d'erreur (Message, Warning, Error)
	// Cela permet d'obtenir des erreurs, meme si la limite est atteinte pour
	// les messages ou les warnings
	// Les FatalError sont toujours prioritaires, et non soumise a ce controle
	// Par defaut: 20
	static void SetMaxErrorFlowNumber(int nValue);
	static int GetMaxErrorFlowNumber();

	// Indique si le nombre d'erreur, warning et message est atteint, et qu'aucun
	// nouveau message ne peut plus etre affiche
	static boolean IsMaxErrorFlowReached();
	static boolean IsMaxErrorFlowReachedPerGravity(int nErrorGravity);

	//////////////////////////////////////////////////////////////////////
	// Parametrage avance pour ignorer le controle de flux d'erreur
	// pour une objet Error specifique
	// Permet d'afficher une erreur, meme si le nombre max d'erreur a afficher est atteint

	// Indique s'il faut ignorer le controle de flow des erreurs
	// Renvoie false si pas de parametrage specifique (par defaut)
	// Sinon, renvoie le code retour de la fonction specifique parametree dans SetErrorFlowIgnoreFunction
	static boolean IgnoreErrorFlow(const Error* e);

	// Definition d'une methode specifique pour ignorer le controle de flow des erreurs
	// Par defaut: NUL, pas de parametrage
	static void SetErrorFlowIgnoreFunction(ErrorFlowIgnoreFunction fErrorFlowIgnore);
	static ErrorFlowIgnoreFunction GetErrorFlowIgnoreFunction();

	//////////////////////////////////////////////////////////
	// Parametrages globaux

	// Gestion d'un mode silencieux
	//     Par defaut: false
	// Dans ce mode, les erreurs ne sont plus emises
	static void SetSilentMode(boolean bValue);
	static boolean GetSilentMode();

	// Traitement des erreur comme des warning
	//     Par defaut: false
	// Dans ce mode, les erreurs ne sont emises sous sorme de warning
	static void SetErrorAsWarningMode(boolean bValue);
	static boolean GetErrorAsWarningMode();

	// Gestion d'un fichier de log des erreurs
	//    Par defaut: aucun
	// Quand on precise un fichier de log des erreurs, toute
	// erreur emise (en mode non silencieux) sera consignee dans ce fichier
	// Les erreurs de l'allocateur sont par la meme occasion redirigees sur
	// la gestion centralisee des erreurs (cf AllocErrorDisplayMessageFunction)
	static boolean SetErrorLogFileName(const ALString& sValue);
	static const ALString GetErrorLogFileName();

	// Est-ce qu'il y a eu au moins une erreur
	static boolean IsAtLeastOneError();

	// Activation de la gestion des signaux via des erreurs, pour afficher des messages d'erreur explicites
	// A appeler en debut du main des executable.
	// Attention, comportement indefini dans les environnements multi-thread.
	// A ne pas appeler pour une DLL (par exemple, crash observe depuis Java sous linux)
	// Ne fait rien en Debug ou RelWithDebInfo (alpha)
	static void ActivateSignalErrorManagement();
	static boolean IsSignalErrorManagementActivated();

	///////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Indique s'il faut ignorer le controle de flow des erreurs, au moment de l'affichage
	// A la difference de IgnoreErrorFlow qui n'effectue qu'un test, on peut exploiter un contexte
	// d'usage pour l'affichage pour controler finement les erreurs a afficher effectivement
	static boolean IgnoreErrorFlowForDisplay(const Error* e);

	// Handler de signal
	static void SignalHandler(int nSigNum);

	// Singleton de la classe Global, permettant d'initialiser automatiquement les handlers de gestion des signaux
	static Global singletonSignalManager;

	// Ajout d'une erreur avec toutes ses caracteristiques detaillee
	static void AddErrorObjectValued(int nGravityValue, const ALString& sCategoryValue,
					 const ALString& sLocalisationValue, const ALString& sLabelValue);

	// Ajout d'une erreur
	static void ShowError(const Error* e);

	// Niveau d'empilement de la gestion du controle d'erreur
	static int nErrorFlowControlLevel;

	// Nombre max d'erreur a afficher pour l'utilisateur
	static int nMaxErrorFlowNumber;

	// Nombre d'erreurs accumulee
	// On utilise des longint, pour se premunir des tres grands nombre d'erreurs
	static longint lErrorFlowMessageNumber;
	static longint lErrorFlowWarningNumber;
	static longint lErrorFlowErrorNumber;

	// Mode silencieux
	static boolean bSilentMode;

	// Mode du traitement des erreurs comme des warning
	static boolean bErrorAsWarningMode;

	// Gestion du fichier d'erreur
	static ALString sErrorLogFileName;
	static fstream fstError;

	// Vrai si il y a eu au moins une erreur
	static boolean bIsAtLeastOneError;

	// Affichage des messages dans la console
	// Avec l'utilisation de /dev/stdout comme fichier
	static boolean bPrintMessagesInConsole;

	// Methode pour ignorer le controle de flow des erreurs
	static ErrorFlowIgnoreFunction fErrorFlowIgnoreFunction;

	// Mode d'activation de la gestion des erreurs pour les signaux
	static boolean bIsSignalErrorManagementActivated;
};

// Prototype d'une fonction d'affichage d'une erreur a l'utilisateur
typedef void (*DisplayErrorFunction)(const Error* e);

/////////////////////////////////////////////////////////
// Classe de gestion des erreurs                       //
// Definition d'un format uniforme de message d'erreur //
/////////////////////////////////////////////////////////
class Error : public Object
{
public:
	// Constructeur
	Error();
	~Error();

	// Gravite des erreurs
	enum
	{
		GravityMessage,
		GravityWarning,
		GravityError,
		GravityFatalError
	};
	int GetGravity() const;
	void SetGravity(int nValue);

	// Categorie d'erreur
	const ALString& GetCategory() const;
	void SetCategory(const ALString& sValue);

	// Localisation de l'erreur
	const ALString& GetLocalisation() const;
	void SetLocalisation(const ALString& sValue);

	// Libelle de l'erreur
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

	// Initialisation complete
	void Initialize(int nGravityValue, const ALString& sCategoryValue, const ALString& sLocalisationValue,
			const ALString& sLabelValue);

	// Copie
	void CopyFrom(const Error* eSource);

	// Duplication
	Error* Clone() const;

	// Affichage
	void Display() const;
	void Write(ostream& ost) const override;

	// Libelle associe a la gravite
	static ALString GetGravityLabel(int nGravity);

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////////
	// Affichage d'une ligne de message a l'utilisateur
	// Toutes les erreurs sont presentees a l'utilisateur en passant par
	// cette methode

	// Construction d'un libelle d'erreur utilisateur complet
	static const ALString BuildDisplayMessage(const Error* e);

	// Affichage d'une erreur a l'utilisateur
	static void DisplayError(const Error* e);

	// Redefinition de la methode d'affichage des messages
	// Aucun affichage si methode NULL
	static void SetDisplayErrorFunction(DisplayErrorFunction fDisplayError);
	static DisplayErrorFunction GetDisplayErrorFunction();

	// Methode d'affichage par defaut: message dans la session DOS
	static DisplayErrorFunction GetDefaultDisplayErrorFunction();

	// methode de test globale
	static void Test();

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs d'un objet erreur
	int nGravity;           // Gravite de l'erreur
	ALString sCategory;     // Categorie d'erreur
	ALString sLocalisation; // Localisation de l'erreur
	ALString sLabel;        // Libelle

	// Methode d'affichage d'un message
	static DisplayErrorFunction fDisplayErrorFunction;
};

// Fonction d'affichage par defaut d'une ligne de message
// permettant de parametrer l'affichage des erreurs.
void ErrorDefaultDisplayErrorFunction(const Error* e);

// Fonction d'affichage d'une erreur de l'allocateur (cf MemSetAllocErrorHandler)
// Redirigee vers un Error::AddSimpleMessage avant d'appeler la fonction
// d'affichage par defaut de l'allocateur (qui provoque une sortie du programme)
void AllocErrorDisplayMessageFunction(const char* sAllocErrorMessage);

// Methodes en inline

inline Error::Error()
{
	nGravity = GravityMessage;
}

inline Error::~Error() {}

inline int Error::GetGravity() const
{
	ensure(nGravity == GravityMessage or nGravity == GravityWarning or nGravity == GravityError or
	       nGravity == GravityFatalError);
	return nGravity;
}

inline void Error::SetGravity(int nValue)
{
	require(nValue == GravityMessage or nValue == GravityWarning or nValue == GravityError or
		nValue == GravityFatalError);
	nGravity = nValue;
}

inline const ALString& Error::GetCategory() const
{
	return sCategory;
}

inline void Error::SetCategory(const ALString& sValue)
{
	sCategory = sValue;
}

inline const ALString& Error::GetLocalisation() const
{
	return sLocalisation;
}

inline void Error::SetLocalisation(const ALString& sValue)
{
	sLocalisation = sValue;
}

inline const ALString& Error::GetLabel() const
{
	return sLabel;
}

inline void Error::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}
