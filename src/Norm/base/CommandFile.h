// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CommandFile;

#include "Object.h"
#include "ALString.h"
#include "Vector.h"
#include "PLRemoteFileService.h"
#include "FileService.h"

///////////////////////////////////////////////////////////////////////////////////////
// Classe CommandFile
// Classe qui permet de gerer les fichiers de commandes en entree et en sortie
// parametres par la ligne de commande
class CommandFile : public Object
{
public:
	// Constructeur
	CommandFile();
	~CommandFile();

	// Reinitialisation du parametrage
	void Reset();

	///////////////////////////////////////////////////////////////////////////////////////
	// Parametrage des fichiers de commandes en entree et en sortie

	// Fichier de commande en entree
	void SetInputCommandFileName(const ALString& sFileName);
	const ALString& GetInputCommandFileName() const;

	// Fichier de commande en sortie
	void SetOutputCommandFileName(const ALString& sFileName);
	const ALString& GetOutputCommandFileName() const;

	// Indique qu'il faut afficher les sorties dans la console
	// Determine par le parametrage du fichier de commande en sortie
	boolean GetPrintOutputInConsole() const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Personnalisation du fichier de commande en entree par des paires (Search value, Replace value).
	// Chaque paire est appliquee sur les operandes de chaque ligne de commande en entree avant son execution

	// Ajout d'une paire (SearchValue, ReplaceValue) pour la personnalisation des commandes en entree
	void AddInputSearchReplaceValues(const ALString& sSearchValue, const ALString& sReplaceValue);

	// Nombre de paires de personnalisation
	int GetInputSearchReplaceValueNumber();

	// Acces aux valeurs d'une paire de personnalisation
	const ALString& GetInputSearchValueAt(int nIndex);
	const ALString& GetInputReplaceValueAt(int nIndex);

	// Nettoyage de toutes les paires
	void DeleteAllInputSearchReplaceValues();

	///////////////////////////////////////////////////////////////////////////////////////
	// Personnalisation par un fichier de parametres au format json
	// Cette option de personnalisation par un fichier json est exclusive de la
	// personnalisation par des paires search/replace
	//
	// # Structure de controle dans les scenarios
	// On ajoute quelques structures de controle dans les scenario pour permettre
	// un pilotage complet des operations de search/replace.
	// Les structures de controle sont materialisees par des instructions en UPPER CASE sur des lignes dediees.
	//
	// ## Boucle
	// Une structure de boucle permet d'entourer un bloc de lignes de scenario entre deux instructions
	//     LOOP <loop key>
	//     END LOOP
	// Toutes les lignes d'un bloc de boucle sont repetees autant de fois que necessaire.
	// ## Test
	// Une structure de test permet d'entourer un bloc de lignes de scenario entre deux instructions
	//     IF <if key>
	//     END IF
	// Toutes les lignes d'un bloc de test sont prise en compte conditionnellement au test.
	//
	// # Parametrage par une structure de donnees contenu dans le fichier json
	// Le fichier json contient une serie de paires cle/valeur:
	// - valeur de type string ou number
	//   - cle dans le scenario a remplacer par la valeur
	// - valeur de type array
	//   - la cle du tableau permet d'identifier (loop key) un bloc de lignes dans le scenario,
	//     pour une structure de boucle (LOOP)
	//   - la valeur est associee dans le json a un array contenant des object json, tous de la meme structure,
	//     avec la meme liste de paires cle/valeur de type string ou number
	//   - on duplique les lignes de la boucle autant de fois qu'il y d'objets dans le tableau,
	//     en effectuant les search/replace selon les cles/valeur de l'objet courant
	// - valeur de type boolean
	//   - la cle du boolean permet d'identifier (if key) un bloc de lignes dans le scenario,
	//     pour une structure de test (IF)
	//   - on prend en compte les lignes du test selon la valeur true ou false associee a la cle
	//
	// # Contraintes sur la structure du json
	// Seule une petite partie de l'expressivite du format json est geree
	// - pas de valeur nul
	// - pas de recursion dans la structure: la seul usage autorise et celui d'un array contenant des object
	// Contraintes sur les cles
	// - les cles utilisees dans le json doivent etre distinctes
	//   - pour l'object a la racine d'objet json principal
	//   - pour chaque tableau, localement au tableau, et entre les cle du tableau et celle de l'objet englobant
	// - les cles ont une syntaxe de variables de langage: uniquement des caracteres alpha-numeriques
	//   - format camelCase obligatoire, coherent avec les recommandations: https://jsonapi.org/recommendations/
	// - aucune cle ne doit etre une sous-partie d'une autre cle
	//   - evite les ambiguites selon l'ordre de remplacement dans les search/replace
	// Liens entre cles dans le json et dans le scenario
	// - chaque cle dans le scenario est utilisable dans le json si elle est entouree de '__' (double tiret du 8)
	//   - exemple, une cle name dans le json est utilisable avec la valeur a remplacer name dans le scenario
	// - chaque cle dans le json doit etre utilisee dans le scenario, et reciproquement
	//   - exception: si une cle de tableau peut etre absent du json, cela equivaut a un tableau vide
	// - chaque cle dans un array du json ne peut etre utilisee que dans la boucle correspondante du scenario
	// - dans le cas d'une cle dont la valeur est une string, la ligne du scenario utilisant cette cle devra etre
	//   terminee par " // commentaire" afin d'autoriser les valeurs contenant la sous-chaine '//'
	//
	// # Choix d'encodage
	// On choisit un encodage UTF-8 systematique pour le json en parametre de Khiops, selon la norme Json.
	// Dans le cas de parametres dont les valeurs peuvent etre soit des strings UTF-8, soit des chaines de bytes,
	// on etend le format json de la facon suivantes.
	// Pour un parametre concerne (ex: dataPath):
	// - le fichier scenario reste inchange, avec utilisation de __dataPath__
	// - le fichier json en parametre peut comporter les deux variantes de la valeurs
	//   - chaine de caracteres UTF-8: nom de la variable et valeur sans encodage
	//     - exemple:  "dataPath = "UTF-8 string value"_
	//   - chaine de bytes: nom de la variable prefixe par _byte_, la premiere lettre du nom de la variable
	//     en majuscule, et valeur avec encodage Base64
	//     - cf. https://fr.wikipedia.org/wiki/Base64
	//     - exemple:  "byteDataPath" = "dmFsdWUK"
	// Au moment de l'ecriture du scenario en sortie, on recherche la cle correspondante dans le json ou
	// sa variante avec prefixe "byte" pour decoder ou non la valeur dans le search/replace.
	// Contraintes specifique sur la structure du json:
	// - chaque cle associe a une valeur chaine de caracteres, et presente dans le scenario, doit exister
	//   sous une seule des deux variantes, avec ou sans prefixe "byte".

	// Fichier de parametre json en entree
	void SetInputParameterFileName(const ALString& sFileName);
	const ALString& GetInputParameterFileName() const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Exploitation du parametrage

	// Verification de la validite des parametres, avec emission de messages d'erreurs
	boolean Check() const override;

	// Ouverture du fichier de commande en entree, avec son son eventuele fichier de parametrage json
	boolean OpenInputCommandFile();
	boolean IsInputCommandFileOpened() const;

	// Ouverture du fichier de commande en sortie
	boolean OpenOutputCommandFile();
	boolean IsOutputCommandFileOpened() const;

	// Indique si on est en train de traiter les commande, en entree ou en sortie
	boolean IsCommandFileOpened() const;

	// Ouverture des fichiers de commandes
	void CloseCommandFiles();

	// Lecture d'une commande
	// Renvoie false si pas de commande, sinon un vecteur de chaines de caracteres representant
	// le parsing de IdentifierPath et une valeur optionnelle
	boolean ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue);

	// Ecriture d'une commande
	void WriteOutputCommand(const ALString& sIdentifierPath, const ALString& sValue, const ALString& sLabel);

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Application des recherche/remplacement de valeurs successivement sur une commande
	const ALString ProcessSearchReplaceCommand(const ALString& sInputCommand);

	// Nom des fichiers
	ALString sInputCommandFileName;
	ALString sOutputCommandFileName;
	ALString sInputParameterFileName;

	// Variante locale des noms de fichier de commande, dans le cas de fichiers HDFS
	ALString sLocalInputCommandFileName;
	ALString sLocalOutputCommandFileName;

	// Fichiers de gestion des commandes
	FILE* fInputCommands;
	FILE* fOutputCommands;

	// Redirection de la sortie outputCommand vers la console
	boolean bPrintOutputInConsole;

	// Gestion des chaines des patterns a remplacer par des valeurs dans les fichiers d'input de scenario
	StringVector svInputCommandSearchValues;
	StringVector svInputCommandReplaceValues;
};
