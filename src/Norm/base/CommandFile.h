// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CommandFile;

#include "Object.h"
#include "ALString.h"
#include "Vector.h"
#include "Longint.h"
#include "FileService.h"
#include "PLRemoteFileService.h"
#include "JSONObject.h"

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

	// Mode batch, pour creer une erreur fatale en d'erreur de commande (defaut: false)
	// En mode GUI, on continue dans la GUI apres avoir simplement signale l'erreur
	void SetBatchMode(boolean bValue);
	boolean GetBatchMode() const;

	// Indique qu'il faut afficher les sorties dans la console
	// Determine par le parametrage du fichier de commande en sortie
	boolean GetPrintOutputInConsole() const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Personnalisation du fichier de commande en entree par des paires (Search value, Replace value).
	// Chaque paire est appliquee sur les operandes de chaque ligne de commande en entree avant son execution

	// Ajout d'une paire (SearchValue, ReplaceValue) pour la personnalisation des commandes en entree
	void AddInputSearchReplaceValues(const ALString& sSearchValue, const ALString& sReplaceValue);

	// Nombre de paires de personnalisation
	int GetInputSearchReplaceValueNumber() const;

	// Acces aux valeurs d'une paire de personnalisation
	const ALString& GetInputSearchValueAt(int nIndex) const;
	const ALString& GetInputReplaceValueAt(int nIndex) const;

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
	// - valeur de type boolean, string ou number
	//   - cle dans le scenario a remplacer par la valeur
	// - valeur de type array
	//   - la cle du tableau permet d'identifier (loop key) un bloc de lignes dans le scenario,
	//     pour une structure de boucle (LOOP)
	//   - la valeur est associee dans le json a un array contenant des object json, tous de la meme structure,
	//     avec la meme liste de paires cle/valeur de type string ou number
	//   - on duplique les lignes de la boucle autant de fois qu'il y d'objets dans le tableau,
	//     en effectuant les search/replace selon les cles/valeur de l'objet courant
	// - valeur de type boolean pour les blocs (IF)
	//   - la cle du boolean permet d'identifier (if key) un bloc de lignes dans le scenario,
	//     pour une structure de test (IF)
	//   - on prend en compte les lignes du test selon la valeur true ou false associee a la cle
	//
	// # Contraintes sur la structure du json
	// Seule une petite partie de l'expressivite du format json est geree
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
	// - chaque cle dans le scenario est utilisable dans le json si elle est entouree de '__'
	//   - exemple, une cle name dans le json est utilisable avec la valeur a remplacer name dans le scenario
	// - chaque cle dans le json doit etre utilisee dans le scenario, et reciproquement
	//   - exception: si une cle de tableau peut etre absent du json, cela equivaut a un tableau vide
	// - chaque cle dans un array du json ne peut etre utilisee que dans la boucle correspondante du scenario
	// - dans le cas d'une cle dont la valeur est une string, la ligne du scenario utilisant cette cle devra etre
	//   terminee par " // commentaire" afin d'autoriser les valeurs contenant la sous-chaine '//'
	//
	// # Tolerance sur les cles de json manquantes ou de valeur null
	// En cas de cle manquante ou associee a la valeur null dans l'objet json principal,
	// les lignes correspondantes du scenario ne sont pas traitees
	// - pour une ligne simple: on ignore la ligne
	// - pour un bloc IF ou LOOP: on ignore tout le bloc
	// Cette tolerance permet de simplifier la gestion des valeurs facultatives.
	// Par contre, cette possibilite n'est pas autorisee pour les lignes a l'interieur d'un bloc LOOP,
	// qui reposent sur une structure devant etre coherente pour tous les objets du tableau de parametrage json.
	// Afin de faciliter le debogage des scenarios et de trouver des cles jamais utilisees,
	// un warning est emis pour toute cle manquante dans le json, si Khiops est lance avec l'option -O
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

	// Ouverture du fichier de commande en entree, avec son son eventuel fichier de parametrage json
	boolean OpenInputCommandFile();
	boolean IsInputCommandFileOpened() const;

	// Ouverture du fichier de commande en sortie
	boolean OpenOutputCommandFile();
	boolean IsOutputCommandFileOpened() const;

	// Indique si on est en train de traiter les commande, en entree ou en sortie
	boolean AreCommandFilesOpened() const;

	// Fermeture des fichiers de commandes
	// La fermeture peut provoquer un erreur fatale en cas d'erreur d'analyse des commandes
	void CloseInputCommandFile();
	void CloseOutputCommandFile();
	void CloseCommandFiles();

	// Lecture d'une commande
	// Renvoie false si pas de commande valide disponible, sinon un vecteur de chaines de caracteres representant
	// le parsing de IdentifierPath et une valeur optionnelle
	// Cette methode peut etre appelee meme en l'absence de fichier de commande en sortie
	boolean ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue);

	// Indique que l'on a fini de lire et traiter les commandes
	boolean IsInputCommandEnd() const;

	// Ecriture d'une commande
	// Cette methode peut etre appelee meme en l'absence de fichier de de commande en sortie
	void WriteOutputCommand(const ALString& sIdentifierPath, const ALString& sValue, const ALString& sLabel);

	// Ecriture d'une header de fichier de commande, consistant en lignes de commentaire en expliquant le fonctionnement
	void WriteOutputCommandHeader();

	// Mode lecture/ecriture d'un fichier de de commande, sans executer les commandes
	// Cela permet de tester la validite des fichier de command eet de parametres en entree
	// et d'effectuer les transformations en fichier de commande natif, sans parametres
	boolean ReadWriteCommandFiles();

	// Personnalisation des messages d'erreur
	void AddInputCommandFileError(const ALString& sMessage) const;
	void AddInputParameterFileError(const ALString& sMessage) const;
	void AddOutputCommandFileError(const ALString& sMessage) const;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Variante affichable d'une valeur, en completant si necessaire par des "..."
	const ALString GetPrintableValue(const ALString& sValue) const;

	///////////////////////////////////////////////////////////////
	// Gestion du fichier de parametre json et de sa verification

	// Chargement du fichier json en entree et verification de sa validite
	boolean LoadJsonParameters();

	// Test de la validite d'une cle json, avec creation si necessaire d'un message d'erreur
	// complet, y compris la valeur testee
	boolean CheckJsonKey(const ALString& sValue, ALString& sMessage) const;

	// Test si une valeur correspond a une cle json au format camelCase
	boolean IsCamelCaseJsonKey(const ALString& sValue) const;

	// Test si une cle json correspond a un contenu de type byte, donc encode au format base64
	// Une telle cle json au format camelCase doit etre prefixe par byte
	boolean IsByteJsonKey(const ALString& sValue) const;

	// Transformation d'une cle json en sa variante byte ou standard
	// On renvoie la valeur initiale si elle est deja dans sa bonne variante
	const ALString ToByteJsonKey(const ALString& sValue) const;
	const ALString ToStandardJsonKey(const ALString& sValue) const;

	// Transformation d'une cles json en sa variante opposee
	const ALString ToVariantJsonKey(const ALString& sValue) const;

	// Test de la validite d'une valeur de type string, avec creation si necessaire d'un message d'erreur
	// complet, y compris la valeur testee
	boolean CheckStringValue(const ALString& sValue, boolean bCheckBase64Encoding, ALString& sMessage) const;

	// Construit d'un path json pour designer une valeur dans unse structure json
	// Cf. https://jsonpatch.com/
	// On suit les element de structure valides dans le parametrage json
	// La fin du parametrage peut etre non utilises (NULL ou -1)
	ALString BuildJsonPath(JSONMember* member, int nArrayRank, JSONMember* arrayObjectmember) const;

	///////////////////////////////////////////////////////////////
	// Gestion du fichier de commandes en entree

	// Application des recherche/remplacement de valeurs successivement sur une commande
	const ALString ProcessSearchReplaceCommand(const ALString& sInputCommand) const;

	// Types de token possible
	enum
	{
		TokenIf,
		TokenLoop,
		TokenEnd,
		TokenKey,
		TokenOther,
		None
	};

	// Type de bloc pour un token de type bloc
	const ALString& GetBlockType(int nToken) const;

	// Reinitialisation de la gestion du parser
	void ResetParser();

	// Recodage de la ligne de commande en cours en exploitant le parametrage json
	// On renvoie la ligne recodee
	// En cas d'erreur, le booleen en parametre est mis a false, avec emission d'un message d'erreur
	const ALString RecodeCurrentLineUsingJsonParameters(boolean& bOk);

	// Analyse d'une nouvelle ligne de commande pour mettre a jour l'etat du parser
	// Etats possibles, gere par nParserState
	// - TokenIf: en cours de traitement de bloc IF
	// - TokenLoop: en cours de traitement de bloc LOOP
	// - TokenOther: instruction standard
	// Le parametre bContinueAnalysis en sortie indique qu'il faut continuer l'analyse
	// du bloc en cours pour avoir une instruction executable disponible
	// En cas d'erreur, on renvoie false, avec emission d'un message d'erreur
	boolean ParseInputCommand(const ALString& sInputCommand, boolean& bContinueAnalysis);

	// Tokenisation de la ligne de commande d'entree en une suite de tokens
	// On renvoie la liste des types et valeur de tokens en sortie si la syntaxe est valide:
	//   TokenIf TokenKey: debut de bloc if
	//   TokenEnd TokenIf: fin de bloc if
	//   TokenLoop TokenKey: debut de bloc loop
	//   TokenEnd TokenLoop: fin de bloc loop
	//   TokenOther (TokenKey|TokenOther)*: instruction standard, avec commande suivi d'une eventuelle valeur
	// En cas d'erreur, on renvoie false, avec emission d'un message d'erreur
	boolean TokenizeInputCommand(const ALString& sInputCommand, IntVector* ivTokenTypes,
				     StringVector* svTokenValues) const;

	// Decomposition de la ligne de commande d'entree en un premier token,
	// suivi d'une valeur inter-token et de la fin de la ligne
	// En sortie, on renvoie le type de token, et on indique la valeur du token la fin de ligne
	// Il n'y a pas de message d'erreur a ce niveau.
	// Les tokens de type TokenKey possedent a minimal leur delimiteur de debut, et peuvent ne pas etre valides
	int GetFirstInputToken(const ALString& sInputCommand, ALString& sToken, ALString& sInterToken,
			       ALString& sEndLine) const;

	// Affichage d'un vecteur de token issu de l'analyse de la ligne de commande
	void WriteInputCommandTokens(ostream& ost, const IntVector* ivTokenTypes,
				     const StringVector* svTokenValues) const;

	// Verification de la syntaxe d'un token de type cle, devant commencer par son delimiteur
	// En cas d'erreur, on renvoie false, avec emission d'un message d'erreur
	boolean CheckTokenKey(const ALString& sToken) const;

	// Extraction de la cle d'un token de type cle valide entoure de ses delimiteurs
	const ALString ExtractJsonKey(const ALString& sTokenKey) const;

	// Recherche de la valeur associe a une cle dans un objet json
	// On renvoie NULL si non trouve
	JSONValue* LookupJSONValue(JSONObject* jsonObject, const ALString& sKey) const;

	// Test si un vecteur de token contient une cle absente d'un objet json, sous sa forme standard ou byte
	boolean ContainsMissingOrNullJSONValue(JSONObject* jsonObject, const IntVector* ivTokenTypes,
					       const StringVector* svTokenValues) const;

	// Test si une valeur est trimee
	boolean IsValueTrimed(const ALString& sValue) const;

	// Detection des membres non utilises du parametrage json, avec emission de messages d'erreur
	// On renvoie true s'il y a au moins une erreur
	boolean DetectedUnusedJsonParameterMembers() const;

	///////////////////////////////////////////////////////////////
	// Variables de specification des fichiers et parametres de commandes

	// Nom des fichiers
	ALString sInputCommandFileName;
	ALString sOutputCommandFileName;
	ALString sInputParameterFileName;

	// Gestion des chaines des patterns a remplacer par des valeurs dans les fichiers d'input de scenario
	StringVector svInputCommandSearchValues;
	StringVector svInputCommandReplaceValues;

	// Mode batch
	boolean bBatchMode;

	///////////////////////////////////////////////////////////////
	// Variables de gestion des fichiers et parametres de commandes

	// Redirection de la sortie outputCommand vers la console
	boolean bPrintOutputInConsole;

	// Variante locale des noms de fichier de commande, dans le cas de fichiers HDFS
	ALString sLocalInputCommandFileName;
	ALString sLocalOutputCommandFileName;

	// Fichiers de gestion des commandes
	FILE* fInputCommands;
	FILE* fOutputCommands;

	// Object json pour les parametres en entree
	JSONObject jsonParameters;

	///////////////////////////////////////////////////////////////
	// Variables de gestion du parsing du fichier de commande en entree
	// permettant sa gestion en flux
	//
	// Choix d'implementation principaux, avec impacts utilisateurs
	// - le fichier de parametre json est lu et traite en entier de facon prealable, avec une taille limitee
	// - le fichier de commande est traite en flux, ce qui permet de n'avoir aucune limite de taille
	// - toute ligne de commande peut etre commentee, y compris les lignes du langage de pilotage de type IF ou LOOP
	// - les lignes d'un fichier de commande template n'ont pas besoin de se terminer par un commentaire
	// - toute __key__ du fichier de commande doit se trouver dans le fichier json
	// - toute key du fichier json doit etre utilise dans le fichier de commande
	// - les __key__ de parametrage json ne peuvent concerner que la partie parametrage utilisateur d'une valeur
	// - toute erreur ou incoherence dans les fichiers de commande et de parametrage json provoquent une erreur fatale

	// Numero de ligne courant du fichier de commande en entree
	int nParserLineIndex;

	// Gestion de l'etat courant du parser de commande
	int nParserState;

	// Gestion de l'etat courant de la ligne en couurs de parsing
	int nParserCurrentLineState;

	// Cle du bloc en cours de traitement
	ALString sParserBlockKey;

	// Vecteur des types et valeurs des tokens de la ligne en cours de traitement
	IntVector ivParserTokenTypes;
	StringVector svParserTokenValues;

	// Indicateur utilise pour ignorer un bloc d'instruction en cas de cle de bloc
	// absente de l'objet json de parametrage
	boolean bParserIgnoreBlockState;

	// Indicateur de traitement dans le cas d'un bloc de type if en cours
	boolean bParserIfState;

	// Tableau en cours du parametrage json dans le cas d'un bloc loop en cours
	JSONArray* parserLoopJSONArray;

	// Tableau des vecteur de type et valeurs de tokens pour les lignes de bloc loop en cours
	ObjectArray oaParserLoopLinesTokenTypes;
	ObjectArray oaParserLoopLinesTokenValues;

	// Index de la ligne de bloc en cours de traitement
	int nParserLoopLineIndex;

	// Index de l'objet json du tableau en cours de traitement
	int nParserLoopObjectIndex;

	// Indicateur d'erreur du parser dans l'analyse des commande
	// Cet indicateur est mis a jour uniquement dans la methode d'ajout d'erreur
	// Il permet de conditionner la fin de l'analyse des commandes lors de la fermeture
	// du fichier de commande, pour detecter les erreurs de parsing de fin de fichier
	mutable boolean bParserOk;

	// Dictionnaire des membres utilises des parametres json
	// Ce dictionnaire est mise a jour lors de chaque recherche de membre par cle
	// Cela permet de detecter les membres non utilises lors de la fermeture du fichier de commande
	mutable NumericKeyDictionary nkdParserUsedJsonParameterMembers;

	///////////////////////////////////////////////////////////////
	// Constantes sur les mots cles du langage de commande en entree
	// et sur les contrainte de taille des elements de langage

	// Valeurs des tokens du langage de pametrage des commande en entree
	static const ALString sTokenLoop;
	static const ALString sTokenIf;
	static const ALString sTokenEnd;

	// Prefix de commentaire
	static const ALString sCommentPrefix;

	// Delimiteur de cle json dans un fichier de commande
	static const ALString sJsonKeyDelimiter;

	// Prefixe des cle json ayant un contenu de type byte
	static const ALString sByteJsonKeyPrefix;

	// Longueur max d'une ligne de fichier de commande
	static const int nMaxLineLength = 500;

	// Longueur max d'une cle json
	static const int nMaxJsonKeyLength = 100;

	// Longueur max d'une valeur de type chaine de caracteres
	static const int nMaxStringValueLength = 300;

	// Longueur max affichee pour une valeur dans les messages d'erreur
	static const int nMaxPrintableLength = 30;

	// Taille max d'un bloc d'instruction LOOP d'un fichier de commande, en nombre de lignes utiles
	static const int nLoopMaxLineNumber = 1000;

	// Taille max d'un fichier de parametrage
	static const longint lMaxInputParameterFileSize = lMB;

	// Tolerance pour accepter les cles vides ou null dans les fichiers de parametrage json
	static const boolean bAcceptMissingOrNullKeys = true;
};
