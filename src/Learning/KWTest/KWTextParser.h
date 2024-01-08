// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWSTDatabaseTextFile.h"
#include "FileService.h"

/////////////////////////////////////////////////////////////////////////////////
// Text parser
// Transformation d'une base de texte en un fichier lignes-colonnes en
// representation sac de mots
class KWTextParser : public Object
{
public:
	// Constructeur
	KWTextParser();
	~KWTextParser();

	// Fichier d'entree de reference contenant tous les textes
	void SetReferenceFileName(const ALString& sFileName);
	const ALString& GetReferenceFileName() const;

	// Fichier d'entree a transcoder
	void SetInputFileName(const ALString& sFileName);
	const ALString& GetInputFileName() const;

	// Fichier de sortie
	void SetOutputFileName(const ALString& sFileName);
	const ALString& GetOutputFileName() const;

	// Longueur max des mots en caracteres (defaut: 8)
	void SetMaxWordLength(int nValue);
	int GetMaxWordLength() const;

	// Frequence min des mots en caracteres (defaut: 3)
	void SetMinWordFrequency(int nValue);
	int GetMinWordFrequency() const;

	// Transcodification du fichier
	// On peut construire un fichier pour la classification et/ou pour le coclustering
	// Si aucune construction n'est demandee, on ne produit que les statistiques de pretraitement
	void Transcode(boolean bBuildClassificationFile, boolean bBuildCoclusteringFile);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void TextParserTest(int argc, char** argv);

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction d'un dictionnaire de mots local a un texte,
	// par parsing des lignes de ce texte.
	// A la fin de cette analyse, on se trouve en debut du texte suivant dans le fichier d'entree
	// Retourne true si OK, false si fin de fichier
	boolean BuildTextWordDictionary(FILE* fInput, ObjectDictionary* odWords);
	void BuildLineWordDictionary(char* sLine, ObjectDictionary* odWords);

	// Variantes d'analyse d'un texte
	// Base NOVA du challenge WCCI2006
	boolean BuildTextWordDictionaryNOVA(FILE* fInput, ObjectDictionary* odWords);
	// Un text par ligne
	boolean BuildTextWordDictionarySingleLine(FILE* fInput, ObjectDictionary* odWords);

	// Construction du dictionnaire de tous les mots
	void BuildReferenceWordDictionary();

	// Filtrage d'un dictionnaire (eventuellement de reference)
	void FilterWordDictionary(ObjectDictionary* odWords, boolean bReference);
	boolean IsWordNumerical(const ALString& sWord);
	boolean IsWordPartlyNumerical(const ALString& sWord);
	boolean IsWordPunctuation(const ALString& sWord);
	void WordToLower(SampleObject* word);
	void WordTruncate(SampleObject* word, int nMaxLength);

	// Affichage de tous les mots
	void WriteWords(ObjectDictionary* odWords, ostream& ost) const;

	// Ajout d'une occurence de mot dans le dictionnaire
	void UpdateWordDictionary(ObjectDictionary* odWords, const ALString& sWord);

	// Lecture d'une ligne d'un fichier
	void ReadLine(char* sBuffer, int nBufferMaxSize, FILE* fInput);

	// Dictionnaire des mots de la representation
	// Chaque mot est un SampleObject (String: valeur du mot, Int: nombre total d'utilisation)
	ObjectDictionary odReferenceWords;

	// Nom des fichiers
	ALString sReferenceFileName;
	ALString sInputFileName;
	ALString sOutputFileName;

	// Parametres
	int nMaxWordLength;
	int nMinWordFrequency;

	// Buffer de lecture
	char* sFileBuffer;
	char* sWordBuffer;
	static const int nFileBufferMaxSize;
	ALString sCurrentFileName;
	int nLineOffset;
	int nTextOffset;
};

// Comparaison de deux mots stockes dans des SampleObject
int SampleObjectCompareWord(const void* elem1, const void* elem2);
int SampleObjectCompareFrequency(const void* elem1, const void* elem2);
