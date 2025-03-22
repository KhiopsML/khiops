// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"

class CommandLine;
class CommandLineOption;

// Prototype de la fonction qui permet de verifier la coherence des flags et parametres saisis
// Le parametre est la liste des options qui ont ete parsees (munies de leurs parametres)
// Renvoie true si l'ensemble options + parametre est coherent
typedef boolean (*GlobalCheckMethod)(const ObjectArray&);

//////////////////////////////////////////////////
// Classe CommandLine
// Classe qui permet de gerer les options passees dans la ligne de commande
// Elle permet d'ajouter des options et de parser la ligne de commande
// L'affichage de l'aide est automatique
class CommandLine : public Object
{
public:
	// Constructeur
	CommandLine();
	~CommandLine();

	// Ajout d'une option
	void AddOption(CommandLineOption* option);

	// Acces aux options
	int GetOptionNumber() const;
	CommandLineOption* GetOptionAt(int nIndex) const;

	// Nom du programme (il sera affiche apres 'Usage:' dans l'aide)
	void SetCommandName(const ALString& sCommand);
	const ALString& GetCommandName() const;

	// Aide globale affichee en preambule de l'aide standard (qui decrit chaque option)
	void SetGlobalHelp(const ALString& sGlobalHelp);
	const ALString& GetGlobalHelp();

	// Parsing de la ligne de commande.
	// Si l'option -h est rencontree, l'aide est affichee est le programme termine en succes
	// Si il n'y a pas eu d'erreur de parsing, les  methodes associees au options sont executees
	// Si il y a eu une erreur, l'aide est affichee et le programme termine en echec
	void ParseMainParameters(int argc, char** argv) const;

	// Acces a la methode de verification de options qui vont etre lancees
	// La methode par defaut renvoie toujours true
	// SI cette methode renvoie false, rien n'est execute et le programme sort en erreur
	const GlobalCheckMethod GetGlobalCheckMethod() const;
	void SetGlobalCheckMethod(GlobalCheckMethod fMethod);

	// Acces au priorites utiliees dans les options deja presentes.
	// Permet d'ajouter une option prioritaire en evitant les collisions avec les priorites des options deja
	// utilisees
	int GetMinUsedPriority() const;
	int GetMaxUsedPriority() const;

	// Supprime toutes les options
	void DeleteAllOptions();

	// Methode de test
	static boolean Test(int argc, char** argv);

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Renvoie l'option associee au flag passe en parametre
	CommandLineOption* GetOptionForFlag(char c) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Affiche l'usage (-h)
	// Les options sont affichees dans l'orde inverse de leur ajout
	// -h est toujours a la fin
	void PrintUsage() const;

	// renvoie true si une option a le flag cFlag
	boolean LookupFlag(char cFlag) const;

	// Methode de verification de la coherence entre les flags ou parametres saisis
	GlobalCheckMethod checkMethod;

	boolean Check() const override;

	ObjectArray* oaOptions;
	ALString sCommandName;
	ALString sGlobalHelp;
	int nOptionIndex;
};

//////////////////////////////////////////////////
// Classe CommandLineOption
// Classe qui definit une option de la ligne de commande
// Une option est constituee
// 		d'un flag (h de -h)
//		d'un parametre optionnel
//		d'une description du parametre
//		d'une description de l'option

// Prototype d'une fonction liee a un parametre
// Si cette methode renvoie false, l'application sort en echec (exit 1)
typedef boolean (*OptionMethod)(const ALString&);

// Comparaison de 2 options basee sur le groupe et l'index
int CommandLineOptionCompareForUsage(const void* elem1, const void* elem2);

// Comparaison de 2 options basee sur la priorite
int CommandLineOptionComparePriority(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////
/// Classe CommandLineOption
// gestion d'une option de ligne de commande
class CommandLineOption : public Object
{
public:
	// Constructeur
	CommandLineOption();
	~CommandLineOption();

	// Duplication
	CommandLineOption* Clone() const;

	// Recopie
	void CopyFrom(const CommandLineOption* option);

	// Acces au flag
	void SetFlag(char c);
	char GetFlag() const;

	// Acces a la description de l'option
	// Le slongues descriptions sont sur plusieurs lignes.
	void AddDescriptionLine(const ALString& sLabel);
	const StringVector* GetDescriptionLines() const;

	// Acces a la methode
	void SetMethod(OptionMethod fMethod);
	OptionMethod GetMethod() const;

	// Est-ce qu'il y a un parametre
	void SetParameterRequired(boolean bParameter);
	boolean GetParameterRequired() const;

	// Acces a la description du parametre
	void SetParameterDescription(const ALString& sParameter);
	const ALString& GetParameterDescription() const;

	// Constantes a utiser pour specifier le type des parametres
	static const ALString sParameterString;
	static const ALString sParameterFile;

	// Est-ce que l'option doit etre utilisee seule (sans autre option)
	// Par defaut false
	void SetSingle(boolean bIsSingle);
	boolean GetSingle() const;

	// Est-ce que l'option peut etre utilise plusieurs fois
	// Par defaut false
	void SetRepetitionAllowed(boolean bRepetion);
	boolean GetRepetitionAllowed();

	// Cette option interdit que d'autres options soient executees apres elle
	// De plus le programme s'arrete juste apres l'execution de la methode associee a l'option
	// Par defaut false
	void SetFinal(boolean bFinal);
	boolean GetFinal() const;

	// Groupe auquel appartient l'option.
	// Affecte l'orde d'affichage des options dans l'aide
	// Les options d'un meme groupe sont affichees ensemble
	// L'affichage inter groupe est ordonne par l'index du groupe
	// Le groupe par defaut est 0
	void SetGroup(int nIndex);
	int GetGroup() const;

	// Priorite associee a l'option
	// Affecte l'orde d'execution des options : l'ordre d'execution suit la priorite
	// Par defaut a l'infini
	void SetPriority(int nIndex);
	int GetPriority() const;

	// Acces aux parametres associes a l'option
	const StringVector* GetParameters();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Est-ce que l'option est utilisee dans la ligne de commande
	// (permet de tester l'unicite)
	void SetUsed(boolean bValue);
	bool GetUsed() const;

	// Acces a l'index qui ordonne les options pour l'affichage
	void SetUsageIndex(int nIndex);
	int GetUsageIndex() const;

	OptionMethod method;
	char cFlag;
	StringVector svDescription;
	boolean bParameterRequired;
	ALString sParameterDescription;
	boolean bIsSingle;
	boolean bRepetitionAlowed;
	int nGroup;
	boolean bIsFinal;

	int nPriority; // Influe sur l'orde d'execution
	boolean bIsUsed;
	StringVector svParameters; // liste des parametres de l'option
	ALString sMessageError; // Message d'erreur renseigne si il y aeu une erreur pendant l'execution de la commande
	int nIndexUsage;        // Ordre d'affichage

	friend class CommandLine;
	friend int CommandLineOptionCompareForUsage(const void* elem1, const void* elem2);
};
