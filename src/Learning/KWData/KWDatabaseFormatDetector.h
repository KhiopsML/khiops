// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDatabaseFormatDetector;
class KWHeaderLineAnalyser;
class RewindableInputBufferedFile;

#include "KWDatabase.h"
#include "KWSTDatabaseTextFile.h"
#include "KWDataTableDriverTextFile.h"
#include "KWCharFrequencyVector.h"
#include "IntVectorSorter.h"

////////////////////////////////////////////////////////////
// Classe KWDatabaseFormatDetector
// Detection du format des fichiers d'une base donnees:
//    . presence de ligne d'entete
//    . separateur de champ
class KWDatabaseFormatDetector : public Object
{
public:
	// Constructeur
	KWDatabaseFormatDetector();
	~KWDatabaseFormatDetector();

	// Parametrage de la base a analyser
	void SetDatabase(KWDatabase* database);
	KWDatabase* GetDatabase() const;

	// Utilisation ou non du dictionnaire pour detecter le format (defaut: true)
	// Si le dictionnaire est utilisable, on l'utilise s'il est specifie, et sinon
	// on se rabat sur le comportement sans dictionnaire
	void SetUsingClass(boolean bValue);
	boolean GetUsingClass();

	// Detection du format
	// En presence de dictionnaire, la detection du format se fait uniquement par analyse de la premiere
	// ligne du fichier associee a la premiere table.
	// Sinon, la detection se base sur l'analyse des premiere lignes du fichier
	// En cas de succes, les champs de format de la base sont mis a jour.
	// Sinon, des warnings ou des erreurs sont emis
	boolean DetectFileFormat();

	// Affichage des premiere lignes de la base
	void ShowFirstLines(int nMaxLineNumber);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Mise a jour du format de la base
	void UpdateDatabaseFormat(boolean bHeaderLineUsed, char cFieldSeparator);

	// Detection de format en utilisant une classe
	boolean DetectFileFormatUsingClass(const KWClass* kwcClass, RewindableInputBufferedFile* inputFile);
	boolean DetectFileFormatUsingClassWithHeaderLine(const KWClass* kwcClass,
							 RewindableInputBufferedFile* inputFile);
	boolean DetectFileFormatUsingClassWithoutHeaderLine(const KWClass* kwcClass,
							    RewindableInputBufferedFile* inputFile);

	// Detection de format sans utiliser de classe
	boolean DetectFileFormatWithoutClass(RewindableInputBufferedFile* inputFile);

	// Affichage de l'entete d'un fichier
	void ShowHeadLines(RewindableInputBufferedFile* inputFile);

	// Affichage parametrable de lignes d'un fichier ouvert en lecture
	void ShowCurrentLines(InputBufferedFile* inputFile, const ALString& sTitle, boolean bEmptyLineBefore,
			      boolean bEmptyLineAfter, int nMaxLineNumber);

	// Fonction cosmetique de gestion du pluriel
	// Passage au pluriel pour un nombre strictement plus grand que 1
	// Regles: ""->"s"; "is"->"are", "has",->"have"
	const ALString Plural(const ALString& sSource, int nNumber) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Methode de gestion des separateurs

	// Priorite d'un separateur, 0 etant la meilleure
	int ComputeSeparatorPriority(char cSeparator) const;

	// Tri des separateurs par priorite decroissante
	void SortSeparators(CharVector* cvSeparators) const;

	// Calcul d'un libelle sur les separateurs, en excluant un separateurs
	const ALString BuildSeparatorsLabel(CharVector* cvSeparators, char cSeparator) const;

	// Indique si un caractere est utilisable comme separateur de champ
	boolean IsInvalidSeparator(char c) const;

	// Initialisation d'un vecteur des separateurs valides
	void InitializeInvalidSeparators();

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Methode d'analyse du format du fichier

	// Recherche de noms de tous les attributs ou blocs d'attributs obligatoires, natifs et utilises de facon
	// directe ou indirecte
	void BuildMandatoryDataItemNames(const KWClass* kwcClass, StringVector* svMandatoryDataItemNames) const;

	// Calcul du nombre de lignes dont le nombre de champ est celui attendu
	// Attention, nExpectedFieldNumber peut etre egal a -1 s'il y avait eu une erreur lors du parsing de la premiere
	// ligne
	int ComputeCorrectLineNumber(IntVector* ivLineFieldNumbers, int nExpectedFieldNumber) const;

	// Collecte du nombre de champs par ligne d'un fichier ouvert en lecture pour les lignes suivantes du buffer
	// warning quelque soit le format du fichier. On ignore les lignes vides.
	void CollectLineFieldNumbers(RewindableInputBufferedFile* inputFile, int nMaxLineNumber,
				     IntVector* ivLineFieldNumbers) const;

	// Calcul du nombre de champs de la prochaine ligne d'un fichier ouvert en lecture
	// On compte 0 champs dans le cas particulier de lignes vides
	// On renvoie le nombre de champs lus si pas d'erreur, -1 sinon
	int ComputeLineFieldNumber(RewindableInputBufferedFile* inputFile) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Methode d'analyse avancees du format du fichier

	// Liste de caracteres dont la presence est obligatoire en au moins un exemplaire danbs une ligne d'entete
	const ALString GetHeaderLineMandatoryChars() const;

	// Calcul d'un indicateur d'homogeneite vis a vis des types des lignes d'un fichier
	// On n'exploite que les lignes ayant le bon nombre de champs et on calcul le type des champs de chaque ligne
	// On en deduit les types les plus frequents par champ, et l'indicateur et celui qui conduit a miximiser le
	// nombre total de champs reconnus dans leur type
	int ComputeTypeConsistency(RewindableInputBufferedFile* inputFile, int nMaxLineNumber, int nExpectedFieldNumber,
				   int& nAnalysedLineNumber, int& nCorrectLineNumber) const;

	// Calcul des types de valeur pour un vecteur de champ
	// Les types sont definis dans KWType
	void ComputeFieldTypes(const StringVector* svFields, CharVector* cvFieldTypes) const;

	// Alimentation des champs d'une ligne vers un vecteur de valeur en sortie, avec un nombre de champ attendu
	// Renvoie true si on a pu lire le nombre exact de champs attendus
	// Sinon, on renvoie false, et le vecteur de champ est inexploitable
	boolean ReadLineFields(RewindableInputBufferedFile* inputFile, int nExpectedFieldNumber,
			       StringVector* svFields) const;

	// Collecte de stats sur les caracteres utilisees dans les premieres lignes d'un fichier
	// On alimente un vecteur d'effectif par caractere, contenant pour chaque caractere le min et le max des
	// utilisations par ligne en ignorant les lignes vides ou trop longues (qui sont ignoree avec un warning lors
	// des lectures de table, quelque soit le format) On renvoie le nombre de lignes non vides effectivement
	// analysees
	int CollectCharFrequenciesLineStats(RewindableInputBufferedFile* inputFile, int nMaxLineNumber,
					    KWCharFrequencyVector* cfvLineMinCharFrequencies,
					    KWCharFrequencyVector* cfvLineMaxCharFrequencies) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Methode de gestion de la solution courante et de son amelioration

	// Initialisation de la solution courante
	void InitCurrentSolution(char cSeparator);

	// Initialisation de la meilleure solution
	void InitBestSolution();

	// Mise a jour de la meilleure solution avec la solution courante
	// Retourne trie en cas d'amelioration
	boolean UpdateBestSolution();

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables d'instances

	// Memorisation des donnees gerees par la fiche
	KWDatabase* analysedDatabase;
	boolean bUsingClass;

	////////////////////////////////////
	// Variables de travail

	// Nombre max de lignes analysee
	const int nMaxAnalysedLineNumber = 100;

	// Nombre min de ligne correcte pour de pas afficher les premieres lignes du fichier
	const int nMinCorrectLineNumber = 3;

	// Nombre et taille max des lignes affichees
	const int nMaxShownLineNumber = 5;
	const int nMaxShownLineLength = 200;

	// Separators invalides
	KWCharFrequencyVector cfvInvalidSeparators;

	// Solution courante
	// Les criteres d'evaluation sont geres de facon lexicographique
	char cCurrentSeparator;
	int nCurrentMatchedFieldNumber; // Nombre de champ reconnu de la ligne d'entete, a maximiser
	int nCurrentUnknownFieldNumber; // Nombre de champ surnumeraire de la ligne d'entete, a minimiser
	int nCurrentCorrectLineNumber;  // Nombre de lignes compatible avec le separateur, a maximiser
	int nCurrentTypeConsistency;    // Nombre de champs coherent avec leur type, a maximiser
	int nCurrentFieldNumber;        // Nombre de champ de la premiere ligne, a minimiser
	int nCurrentSeparatorPriority;  // Priorite du separateur, a minimiser
	int nCurrentAnalysedLineNumber; // Nombre de lignes analysees, hors critere

	// Meilleure solution
	CharVector cvBestSeparatorList;
	char cBestSeparator;
	int nBestMatchedFieldNumber;
	int nBestUnknownFieldNumber;
	int nBestCorrectLineNumber;
	int nBestTypeConsistency;
	int nBestFieldNumber;
	int nBestSeparatorPriority;
	int nBestAnalysedLineNumber;

	// Indicateur d'affichage des details d'optimisation
	boolean bShowDetails;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Classe technique d'aide a l'analyse d'un ligne d'entete
class KWHeaderLineAnalyser : public Object
{
public:
	// Constructeur
	KWHeaderLineAnalyser();
	~KWHeaderLineAnalyser();

	//////////////////////////////////////////////////////////////
	// Acces aux nom des champs

	// (Re)initialisation
	void Initialize();

	// Lecture d'une ligne de fichier pour alimenter une liste de noms de champs
	// Le fichier en entree est ouvert en lecture avec avec un separateur de champ correctement parametre.
	// En sortie, on alimente un dictionnaire de noms de champs
	// On arrete des qu'il y a incoherence:
	//   . un champs vide
	//   . un champ duplique
	//   . une erreur de parsing
	// On renvoie true si pas d'erreur
	boolean FillFromFile(InputBufferedFile* inputFile);

	// Nombre de champs
	int GetSize() const;

	// Acces aux nom des champs
	const ALString& GetAt(int i) const;

	// Test de presence d'un champ
	boolean IsPresent(const ALString& sValue) const;

	///////////////////////////////////////////////////////////////
	// Methodes d'analyse des champs de la ligne

	// calcule combien de noms en parametre sont presents
	int ComputePresentNameNumber(StringVector* svNames) const;

	// Calcul combien de nom de champs comporte au moins un caractere obligatoire
	int ComputeValidNameNumber(const ALString& sMandatoryChars) const;

	///////////////////////////////////////////////////////////////
	// Methode de services standard

	// Affichage d'un resume avec les premiers champs
	void Write(ostream& ost) const override;

	// Ecriture complete sur une ligne sous forme tabulaire
	void FullWrite(ostream& ost) const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vecteur et dictionnaire des champs
	StringVector svFieldNames;
	ObjectDictionary odFieldNames;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Specialisation de InputBufferedFile pour pouvoir revenir au depart du buffer de lecture
class RewindableInputBufferedFile : public InputBufferedFile
{
public:
	// Constructeur
	RewindableInputBufferedFile();
	~RewindableInputBufferedFile();

	// Retour au debut du buffer
	void RewindBuffer();
};
