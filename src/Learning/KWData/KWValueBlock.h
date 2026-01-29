// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWValueBlock;
class KWContinuousValueBlock;
class KWSymbolValueBlock;
class KWObjectArrayValueBlock;

#include "KWSymbol.h"
#include "KWContinuous.h"
#include "KWType.h"
#include "Object.h"
#include "Vector.h"
#include "KWIndexedKeyBlock.h"
#include "KWValueDictionary.h"
#include "KWValueSparseVector.h"
#include "OutputBufferedFile.h"

////////////////////////////////////////////////////////
// Classe KWValueIndexPair
// Class technique, utilisee uniquement pour l'implementation des blocs sparse
// Pas de "padding" des attributs pour optimiser la place memoire
// #ifdef __MSC__
#pragma pack(push)
#pragma pack(1)
struct KWValueIndexPair
{
	int nIndex;
	KWValue value;

	friend class KWValueBlock;
	friend class KWContinuousValueBlock;
	friend class KWSymbolValueBlock;
	friend class KWObjectArrayValueBlock;
};
#pragma pack(pop)

// Sous linux on doit utiliser __attribute__((packed)); mais pragma pack fonctionne sur clang et gcc, on privilegie donc
// la version portable. ATTENTION il faut declarer nIndex avant value sinon le packing n'est pas fait sur linux (on ne
// sait pas pourquoi)

/*#else // Linux

struct __attribute__((__packed__)) KWValueIndexPair
{
	int nIndex;
	KWValue value;

	friend class KWValueBlock;
	friend class KWContinuousValueBlock;
	friend class KWSymbolValueBlock;
	friend class KWObjectArrayValueBlock;
};

#endif*/

/////////////////////////////////////////////////////////////////////
// Classe KWValueBlock: gestion generique d'un bloc de valeurs sparse
// Classe ancetre des classes specialisee par type de valeur
// Chaque valeur est identifiee par un index d'attribut et disponible
// sous la forme d'une paire (attributeIndex, valeur)
class KWValueBlock : public SystemObject
{
public:
	// Nombre de valeurs disponibles
	int GetValueNumber() const;

	///////////////////////////////////////////////////////////////////
	// Acces aux valeurs du bloc, de la premiere a la derniere
	// (entre 0 et ValueNumber)

	// Acces a l'index de chargement sparse de la variable associee a un index de valeur
	int GetAttributeSparseIndexAt(int nValueIndex) const;
	void SetAttributeSparseIndexAt(int nValueIndex, int nSparseIndex);

	///////////////////////////////////////////////////////////////////
	// Acces aux valeurs du bloc par index d'attribut
	// (entre 0 et LastAttributeSparseIndex inclus)
	// Il s'agit d'un bloc sparse, et le nombre de valeur est potentiellement
	// beaucoup plus petit que le nombre d'attributs correspondant

	// Acces a un index de valeur par son index d'attribut (-1 si non trouve)
	// Base sur une recherche dichotomique
	int GetValueIndexAtAttributeSparseIndex(int nAttributeSparseIndex) const;

	// Acces au plus petit index de valeur superieur ou egal a un index d'attribut (-1 si non trouve)
	// Base sur une recherche dichotomique
	int GetSmallestValueIndexBeyondAttributeSparseIndex(int nAttributeSparseIndex) const;

	///////////////////////////////////////////////////////////////////
	// Entree-sortie au format chaine de caracteres
	//
	// Le format est celui d'une suite de paire <cle>:<valeur>, separee par des blancs,
	// avec exactement un blanc entre chaque paire <cle>:<valeur>.
	// La partie valeur peut etre absente, au format <cle>:, ce qui correspond a la valeur manquante
	// dans le cas numerique et a la valeur vide "" dans le cas categoriel.
	// Il y a egalement une tolerance a ne pas avoir le separateur ':', au format <cle> uniquement.
	// Dans ce cas, on associera la cle a la valeur 1 dans le cas numerique et "1" dans le cas categoriel.
	// Les cles sont gerees telles quelles si elle ne contiennent que des caracteres alpha-numeriques,
	// et sont entre simple-quotes (') sinon, avec doublement des simple-quotes internes.
	// Les valeurs numeriques sont gerees telles quelles.
	// Les valeurs categorielles sont telles quelles si elle ne contiennent que des caracteres alpha-numeriques,
	// et sont entre simple-quotes (') sinon, avec doublement des simple-quotes internes.
	// Les cles doivent etre uniques, que ce soit dans le cas de cles numeriques ou categorielles.
	// Dans le cas de cles numeriques, elles doivent etre comprise entre 1 et 1000000000, et ordonnes dans le
	// fichier. Tout probleme de format errone, ou de doublon de cle donne lieu a une erreur, et toutes les valeurs
	// du bloc sont alors perdues.
	// En cas de cle non declaree dans le dictionnaire mais presente dans le champ, la valeur correspondante
	// est simplement ignoree, sans message.
	//
	// Ce format est un format interne, qui est potentiellement transforme en format externe via les
	// methodes des classes InputBufferedFile et OutputBufferedFile. L'utilisation des simples-quotes
	// minimise le risque d'utilisation du caractere d'echappement double quote utilise par ces classes.
	// Le format est egalement compatible avec le format sparse usuel, liste de <cle>:<valeur> avec les
	// cle etant des valeurs entieres et les valeurs des valeurs numeriques.
	// En cas d'absence de ':' suivant une cle, on supposera une valeur 1 dans le cas numerique, et
	// "1" dans le cas categoriel
	//
	// Exemples dans le cas numerique
	//   8:4965 1123:350 3069:6795 3972:7531 4100:7603
	//   mon:1 nom:1.0 est:1 personne:1
	//   mon nom est personne
	//   'aujourdh''hui' il:1 fait beau
	// Exemples dans le cas categoriel
	//   10:rouge 20:bleu trente:'vert pomme'

	////////////////////////////////////////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const;

	// Version silencieuse du test d'integrite, avec alimentation eventuelle d'un message d'erreur
	boolean SilentCheck(ALString& sMessage) const;

	// Libelle de la classe
	const ALString GetClassLabel() const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const;

	// Verifie que la taille de KWValueIndexPair est correcte suite au packing de la structure par le compilateur
	static void CheckPacking();

	// Test
	static void Test();

	//////////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Attention, pour des raisons d'optimisation memoire, cette classe
	// est traite comme un bloc de valeurs de taille fixe.
	// Elle n'herite pas de la classe Object (plus de methodes virtuelles)
	// On ne peut donc pas utiliser ces objet dans des containers de type
	// ObjectArray ou ObjectDictionary
protected:
	// Creation et initialisation de la memoire necessaire au stockage d'un bloc
	// Permet d'implementer les methodes de creation de bloc dans les sous-classes
	//
	// La classe utilise une fonctionnalite avancee du C++, le "placement new operator",
	// pour allouer prealablement la memoire donnee au constructeur, qui usuellement
	// d'une part alloue la memoire, d'autre part l'initialise
	// Ici, on aura une sequence de trois methodes dans les "placement new" exploites dans
	// les methodes NewValueBlock des sous-classes:
	// - appel de GenericAllocValueBlock pour creer un bloc memoire
	// - appel du placement new du C++ avec ce bloc, qui declenche un appel standard du C++
	//   (en utilisant un constructeur ne faisant rien)
	// - appel de GenericInitValueBlock pour initialiser le bloc memoire a la place du constructeur
	static void* GenericAllocValueBlock(int nSize);
	static void GenericInitValueBlock(void* pValueBlockMemory, int nSize);

	// Constructeur prive
	KWValueBlock();
	~KWValueBlock();

	// Initialisation d'un tableau de KWKeyIndex<Value> pour le cas d'un bloc a cle categorielle
	// Chaque valeur du dictionnaire en entree est associee a une cle, dont on peut retrouver l'index
	// grace au bloc de cles indexes (on ignore les valeurs n'ayant pas de cle indexee correspondante,
	// ainsi que les valeurs egales a la valeur par defaut).
	// Les valeurs sont alors memorisees dans le tableau en sortie selon l'ordre de leurs index de cles
	static void InitializeKeyIndexArray(const KWIndexedCKeyBlock* indexedKeyBlock,
					    const KWValueDictionary* valueDictionary, ObjectArray* oaKeyIndexValues);

	//////////////////////////////////////////////////////////////////////
	// Lecture d'une valeur de base a partir d'un buffer d'entree non vide
	// On passe en entree l'offset courant dans le champ a analyser, et cet
	// offset sera mis a jour en sortie pour l'analyse du catactere suivant.
	// La valeur lue est mise a jour en sortie.
	// Le flag nFieldError est positionne en cas d'erreur de parsing du champ
	// Code retour a true si le token est le dernier de la ligne, du buffer ou du fichier

	// Type d'erreur liees au parsing d'un champs
	// En cas d'erreur, le parsing continue pour rattrapper l'erreur, au mieux en fin de champs, au pire en fin de
	// ligne
	enum
	{
		FieldNoError,             // Pas d'erreur
		FieldKeyEmpty,            // Cle vide
		FieldIntKeyStartWithZero, // Cle entiere valan
		FieldIntKeyTooLarge,      // Cle entiere trop longue
		FieldMiddleQuote,         // Quote non double au milieu d'un champ commencant par un quote
		FieldMissingEndQuote,     // Manque un quote en fin d'un champ commencant par un quote
		FieldWrongIntChar,        // Caractere ne pouvant pas etre utilise dans un champ entier
		FieldWrongContinuousChar, // Caractere ne pouvant pas etre utilise dans un champ numerique
		FieldWrongChar,           // Caractere non alpha-numerique dans un champ ne commencant par un quote
		FieldMissingBlank,        // Caractere blanc a tort en debut de champ
		FieldHeadBlank,           // Caractere blanc a tort en debut de champ
		FieldTailBlank,           // Caractere blanc a tort en fin de champ
		FieldDoubledBlank,        // Caractere blanc double a tort
		FieldEnd                  // Flag de fin des messages
	};

	// Libelle d'erreur associe a un type d'erreur
	static const ALString GetFieldErrorLabel(int nFieldError);

	// Lecture d'un caractere blanc de separation entre paires, avec controle d'erreur
	static boolean ReadBlankSeparator(const char* sInputField, int& nCurrentOffset, int& nFieldError);

	// Lecture d'une cle
	static boolean ReadCKey(const char* sInputField, int& nCurrentOffset, ALString& sKey, int& nFieldError);
	static boolean ReadNKey(const char* sInputField, int& nCurrentOffset, ALString& sKey, int& nKey,
				int& nFieldError);

	// Lecture d'une valeur Continuous, qui devra ensuite etre convertie en entier
	static boolean ReadIntValue(const char* sInputField, int& nCurrentOffset, ALString& sValue, int& nFieldError);

	// Lecture d'une valeur Continuous, qui devra ensuite etre convertie en Continuous
	static boolean ReadContinuousValue(const char* sInputField, int& nCurrentOffset, ALString& sValue,
					   int& nFieldError);

	// Lecture d'une valeur Symbol (sans supression des blancs de debut et fin), qui devra ensuite etre convertie en
	// Symbol
	static boolean ReadSymbolValue(const char* sInputField, int& nCurrentOffset, ALString& sValue,
				       int& nFieldError);

	// Lecture d'une valeur chaine de caracteres (sans supression des blancs de debut et fin)
	static boolean ReadStringValue(const char* sInputField, int& nCurrentOffset, ALString& sValue,
				       int& nFieldError);

	// Fabrication d'un message d'erreur complet en prenant un extrait du champ a analyser
	static const ALString BuildErrorMessage(const ALString& sError, const ALString& sCurrentKey,
						const char* sInputField, int nCurrentOffset);

	//////////////////////////////////////////////////////////////////////
	// Ecriture d'une valeur de base dans un buffer de sortie
	// Concatenation en fin de buffer

	// Ecriture d'une cle
	static void WriteKey(ALString& sOutputField, const KWIndexedKeyBlock* indexedKeyBlock,
			     int nAttributeSparseIndex);

	// Ecriture d'une valeur entiere
	static void WriteIntValue(ALString& sOutputField, int nValue);

	// Ecriture d'une valeur Continuous
	static void WriteContinuousValue(ALString& sOutputField, Continuous cValue);

	// Ecriture d'une valeur Symbol
	static void WriteSymbolValue(ALString& sOutputField, const Symbol& sValue);

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion optimise de la memoire pour un bloc de valeur
	//
	// La classe est optimisee pour une empreinte memoire minimale
	// On n'herite pas de Object, ce qui empeche l'utilisation des objets de
	// cette classe dans des container de type ObjectArray, ainsi que des
	// service lies a Object (gestion des erreurs...)
	// Les blocs de valeurs ne sont pas retaillable: la taille est decidez
	// une fois pour toutes lors de la creation de l'objet
	//
	// Lors de l'allocation, on alloue exactement la place pour le nombre de valeurs
	// du bloc (ValueNumber), ainsi que l'ensemble des paires (KWValueIndexPair).
	// On a insi un seul bloc memoire de taille minimum

	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(KWValueIndexPair);
	static const int nSegmentSize = (int)((MemSegmentByteSize - sizeof(int)) / nElementSize);

	// Nombre de valeurs du bloc
	int nValueNumber;

	// Caractere dont l'adresse permettra de trouver le debut du bloc
	// En effet, lors de l'allocation, on alloura la taille necessaire pour stocker
	// un entier (nValueNumber), ainsi que le nombres de paires (value, index)
	// (KWValueIndexPair) du bloc
	// Permet un stockage compact, et une recherche des valeurs individuelle
	// par index de variable selon une dichotomie (O(log(N)), les index
	// de variables etant tries (le vecteur de valeur est a gerer dans les sous-classes)
	char cStartBlock;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KWContinuousValueBlock: gestion d'un bloc de valeurs sparse Continuous
class KWContinuousValueBlock : public KWValueBlock
{
public:
	// Creation d'un bloc de taille donnees
	// Methode statique a utiliser pour la creation d'un bloc de taille fixe donnee
	static KWContinuousValueBlock* NewValueBlock(int nSize);
	~KWContinuousValueBlock();

	// Acces a une valeur par son index
	Continuous GetValueAt(int nValueIndex) const;
	void SetValueAt(int nValueIndex, Continuous cValue);

	// Acces a une valeur par son index d'attribut (valeur par defaut si non trouve)
	Continuous GetValueAtAttributeSparseIndex(int nAttributeSparseIndex, Continuous cDefaultValue) const;

	// Recherche d'une valeur dans le bloc
	// Recherche sequentielle peu efficace, mais permet de verifier par exemple
	// que la valeur par defaut est absente du bloc dans les assertions
	// Renvoie l'index de la valeur si trouve, -1 sinon
	int SearchValueIndex(Continuous cValue) const;

	// Construction d'un bloc dans le cas d'un bloc a cles categorielles
	// a partir d'un dictionnaire de valeurs (KWContinuousKeyIndex)
	// Chaque valeur du dictionnaire en entree est associee a une cle, dont on peut retrouver l'index
	// grace au bloc de cles indexes (on ignore les valeurs n'ayant pas de cle indexee correspondante).
	// Les valeurs sont alors memorisees selon l'ordre de leurs index de cles
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWContinuousValueBlock* BuildBlockFromValueDictionary(const KWIndexedCKeyBlock* indexedKeyBlock,
								     const KWValueDictionary* valueDictionary);

	// Construction d'un bloc a partir d'un vecteur sparse de paires (index sparse, valeur)
	// Le vecteur paires du vecteur en entree doivent etre triees avec unicite des index sparse
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWContinuousValueBlock*
	BuildBlockFromSparseValueVector(const KWContinuousValueSparseVector* valueSparseVector);

	// Creation d'un bloc par reindexation d'une sous-partie de ses valeurs
	// Le vecteur ivNewValueIndexes contient pour chaque index de valeur source l'index de valeur cible si elle est
	// gardee, -1 sinon Le bloc en sortie contient les valeurs a garder, dans l'ordre de leur nouvel index
	static KWContinuousValueBlock* ExtractBlockSubset(const KWContinuousValueBlock* sourceValueBlock,
							  const IntVector* ivNewValueIndexes);

	// Construction d'un bloc par lecture d'un champ texte
	// En cas d'erreur, le bloc est cree a vide, bOk vaut false et le message est alimente avec la cause de l'erreur
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWContinuousValueBlock* BuildBlockFromField(const KWIndexedKeyBlock* indexedKeyBlock,
							   const char* sInputField, Continuous cDefaultValue,
							   boolean& bOk, ALString& sMessage);

	// Ecriture d'un bloc de valeurs
	void WriteField(const KWIndexedKeyBlock* indexedKeyBlock, ALString& sOutputField) const;

	///////////////////////////////////////////////////////////////////////////
	// Services pour la gestion des DataTableSliceSet, ou un bloc de valeurs
	// peut etre decoupe sur plusieurs troncon, en sous-bloc contigus selon
	// l'ordre des SparseIndex

	// Ecriture d'une partie d'un bloc de valeurs, pour une plage de SparseIndex
	// On renvoie le nombre de valeurs ecrites
	int WriteFieldPart(const KWIndexedKeyBlock* indexedKeyBlock, int nFirstSparseIndex, int nLastSparseIndex,
			   ALString& sOutputField) const;

	// Creation d'un bloc par concatenation de deux bloc existants successifs
	// Ces deux blocs sont cense provenir de deux troncons distincts, mais indexes (pour les sparseIndex) selon une
	// meme classe commune Les SparseIndex de chaque bloc sont donc exclusifs, et les concatenation va produire un
	// bloc valide,
	static KWContinuousValueBlock* ConcatValueBlocks(const KWContinuousValueBlock* sourceValueBlock1,
							 const KWContinuousValueBlock* sourceValueBlock2);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Valeur par defaut, par defaut (Missing)
	static Continuous GetDefaultDefaultValue();

	// Clone: alloue et retourne le Clone
	KWContinuousValueBlock* Clone() const;

	// Affichage
	void Write(ostream& ost) const;

	// Test
	static void Test();
	static void TestPerformance();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction d'un bloc par lecture d'un champ texte, par type de VarKey
	static KWContinuousValueBlock* BuildCKeyBlockFromField(const KWIndexedCKeyBlock* indexedKeyBlock,
							       const char* sInputField, Continuous cDefaultValue,
							       boolean& bOk, ALString& sMessage);
	static KWContinuousValueBlock* BuildNKeyBlockFromField(const KWIndexedNKeyBlock* indexedKeyBlock,
							       const char* sInputField, Continuous cDefaultValue,
							       boolean& bOk, ALString& sMessage);

	// Constructeur prive
	KWContinuousValueBlock();
};

// Ecriture dans un stream, comme pour la classe Object
inline ostream& operator<<(ostream& ost, const KWContinuousValueBlock& value)
{
	value.Write(ost);
	return ost;
}

/////////////////////////////////////////////////////////////////////////////////
// Classe KWSymbolValueBlock: gestion d'un bloc de valeurs sparse Symbol
class KWSymbolValueBlock : public KWValueBlock
{
public:
	// Creation d'un bloc de taille donnees
	// Methode statique a utiliser pour la creation d'un bloc de taille fixe donnee
	static KWSymbolValueBlock* NewValueBlock(int nSize);
	~KWSymbolValueBlock();

	// Acces a une valeur par son index
	Symbol& GetValueAt(int nValueIndex) const;
	void SetValueAt(int nValueIndex, const Symbol& sValue);

	// Acces a une valeur par son index d'attribut (valeur par defaut si non trouve)
	Symbol& GetValueAtAttributeSparseIndex(int nAttributeSparseIndex, Symbol& sDefaultValue) const;

	// Recherche d'une valeur dans le bloc
	// Recherche sequentielle peu efficace, mais permet de verifier par exemple
	// que la valeur par defaut est absente du bloc dans les assertions
	// Renvoie l'index de la valeur si trouve, -1 sinon
	int SearchValueIndex(const Symbol& sValue) const;

	// Construction d'un bloc dans le cas d'un bloc a cles categorielles
	// a partir d'un dictionnaire de valeurs (KWSymbolKeyIndex)
	// Chaque valeur du dictionnaire en entree est associee a une cle, dont on peut retrouver l'index
	// grace au bloc de cles indexes (on ignore les valeurs n'ayant pas de cle indexee correspondante).
	// Les valeurs sont alors memorisees selon l'ordre de leurs index de cles
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWSymbolValueBlock* BuildBlockFromValueDictionary(const KWIndexedCKeyBlock* indexedKeyBlock,
								 const KWValueDictionary* valueDictionary);

	// Construction d'un bloc a partir d'un vecteur sparse de paire (index sparse, valeur)
	// Le vecteur paires du vecteur en entree doivent etre triees avec unicite des index sparse
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWSymbolValueBlock* BuildBlockFromSparseValueVector(const KWSymbolValueSparseVector* valueSparseVector);

	// Creation d'un bloc par reindexation d'une sous-partie de ses valeurs
	// Le vecteur ivNewValueIndexes contient pour chaque index de valeur source l'index de valeur cible si elle est
	// gardee, -1 sinon Le bloc en sortie contient les valeurs a garder, dans l'ordre de leur nouvel index
	static KWSymbolValueBlock* ExtractBlockSubset(const KWSymbolValueBlock* sourceValueBlock,
						      const IntVector* ivNewValueIndexes);

	// Lecture d'un champ texte
	// En cas d'erreur, le bloc est cree a vide, bOk vaut false et le message est alimente avec la cause de l'erreur
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWSymbolValueBlock* BuildBlockFromField(const KWIndexedKeyBlock* indexedKeyBlock,
						       const char* sInputField, const Symbol& sDefaultValue,
						       boolean& bOk, ALString& sMessage);

	// Ecriture d'un bloc de valeurs
	void WriteField(const KWIndexedKeyBlock* indexedKeyBlock, ALString& sOutputField) const;

	// Ecriture d'une partie d'un bloc de valeurs
	// On renvoie le nombre de valeurs ecrites
	int WriteFieldPart(const KWIndexedKeyBlock* indexedKeyBlock, int nFirstSparseIndex, int nLastSparseIndex,
			   ALString& sOutputField) const;

	// Creation d'un bloc par concatenation de deux bloc existants successifs
	// Ces deux blocs sont cense provenir de deux troncons distincts, mais indexes (pour les sparseIndex) selon une
	// meme classe commune Les SparseIndex de chaque bloc sont donc exclusifs, et les concatenation va produire un
	// bloc valide,
	static KWSymbolValueBlock* ConcatValueBlocks(const KWSymbolValueBlock* sourceValueBlock1,
						     const KWSymbolValueBlock* sourceValueBlock2);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Valeur par defaut, par defaut ("")
	static Symbol& GetDefaultDefaultValue();

	// Clone: alloue et retourne le Clone
	KWSymbolValueBlock* Clone() const;

	// Affichage
	void Write(ostream& ost) const;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction d'un bloc par lecture d'un champ texte, par type de VarKey
	static KWSymbolValueBlock* BuildCKeyBlockFromField(const KWIndexedCKeyBlock* indexedKeyBlock,
							   const char* sInputField, const Symbol& sDefaultValue,
							   boolean& bOk, ALString& sMessage);
	static KWSymbolValueBlock* BuildNKeyBlockFromField(const KWIndexedNKeyBlock* indexedKeyBlock,
							   const char* sInputField, const Symbol& sDefaultValue,
							   boolean& bOk, ALString& sMessage);

	// Constructeur prive
	KWSymbolValueBlock();
};

// Ecriture dans un stream, comme pour la classe Object
inline ostream& operator<<(ostream& ost, const KWSymbolValueBlock& value)
{
	value.Write(ost);
	return ost;
}

/////////////////////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueBlock: gestion d'un bloc de valeurs sparse ObjectArray
class KWObjectArrayValueBlock : public KWValueBlock
{
public:
	// Creation d'un bloc de taille donnees
	// Methode statique a utiliser pour la creation d'un bloc de taille fixe donnee
	static KWObjectArrayValueBlock* NewValueBlock(int nSize);
	~KWObjectArrayValueBlock();

	// Acces a une valeur par son index
	// Le tableau retourne appartient a l'appele
	void SetValueAt(int nValueIndex, ObjectArray* oaValue);
	ObjectArray* GetValueAt(int nValueIndex) const;

	// Acces a une valeur par son index d'attribut (NULL si non trouve)
	ObjectArray* GetValueAtAttributeSparseIndex(int nAttributeSparseIndex) const;

	// Alimentation du bloc a partir d'un vecteur sparse de paire (index sparse, valeur)
	// Le vecteur paires du vecteur en entree doivent etre triees avec unicite des index sparse
	// Memoire: le bloc en sortie appartient a l'appelant
	static KWObjectArrayValueBlock*
	BuildBlockFromSparseValueVector(const KWObjectArrayValueSparseVector* valueSparseVector);

	// Creation d'un bloc par reindexation d'une sous-partie de ses valeurs
	// Le vecteur ivNewValueIndexes contient pour chaque index de valeur source l'index de valeur cible si elle est
	// gardee, -1 sinon Le bloc en sortie contient les tableaux a garder, dans l'ordre de leur nouvel index Les
	// tableaux du bloc source utilise dans le nouveau bloc sont mis a NULL
	static KWObjectArrayValueBlock* ExtractBlockSubset(KWObjectArrayValueBlock* sourceValueBlock,
							   const IntVector* ivNewValueIndexes);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Clone: alloue et retourne le Clone
	KWObjectArrayValueBlock* Clone() const;

	// Affichage
	void Write(ostream& ost) const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur prive
	KWObjectArrayValueBlock();
};

// Ecriture dans un stream, comme pour la classe Object
inline ostream& operator<<(ostream& ost, const KWObjectArrayValueBlock& value)
{
	value.Write(ost);
	return ost;
}

///////////////////////////////////////////////////////
// Methodes en inline

// Classe KWValueBlock

#ifdef __MSC__
#pragma warning(disable : 26495) // disable C26495 warning("La variable'% variable% 'n'est pas initialisee...")
#endif                           // __MSC__

inline KWValueBlock::KWValueBlock()
{
	// Pas d'initialisation de nValueNumber et cStartBlock, qui sont censes etre initialises par la methode
	// NewValueBlock appelante Cela declenche un warning sous Visual C++ 2019, que l'on peut ignorer
}

#ifdef __MSC__
#pragma warning(default : 26495)
#endif // __MSC__

inline int KWValueBlock::GetValueNumber() const
{
	return nValueNumber;
}

inline int KWValueBlock::GetAttributeSparseIndexAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (
	    nValueNumber <= nSegmentSize
		? ((KWValueIndexPair*)&cStartBlock)[nValueIndex].nIndex
		: ((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize].nIndex);
}

inline void KWValueBlock::SetAttributeSparseIndexAt(int nValueIndex, int nSparseIndex)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	require(0 <= nSparseIndex);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPair*)&cStartBlock)[nValueIndex].nIndex = nSparseIndex;
	else
		((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize].nIndex =
		    nSparseIndex;
}

inline longint KWValueBlock::GetUsedMemory() const
{
	return sizeof(int) + nValueNumber * sizeof(KWValueIndexPair);
}

// Classe KWContinuousValueBlock

inline KWContinuousValueBlock* KWContinuousValueBlock::NewValueBlock(int nSize)
{
	KWContinuousValueBlock* newValueBlock;
	void* pValueBlockMemory;

	// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee
	// (attention, C++ avance)
	pValueBlockMemory = GenericAllocValueBlock(nSize);
	newValueBlock = new (pValueBlockMemory) KWContinuousValueBlock;
	GenericInitValueBlock(pValueBlockMemory, nSize);
	assert(newValueBlock->nValueNumber == nSize);
	return newValueBlock;
}

inline KWContinuousValueBlock::KWContinuousValueBlock() {}

inline KWContinuousValueBlock::~KWContinuousValueBlock() {}

inline Continuous KWContinuousValueBlock::GetValueAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (nValueNumber <= nSegmentSize
		    ? ((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.GetContinuous()
		    : ((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
			  .value.GetContinuous());
}

inline void KWContinuousValueBlock::SetValueAt(int nValueIndex, Continuous cValue)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.SetContinuous(cValue);
	else
		((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
		    .value.SetContinuous(cValue);
}

inline Continuous KWContinuousValueBlock::GetValueAtAttributeSparseIndex(int nAttributeSparseIndex,
									 Continuous cDefaultValue) const
{
	int nValueIndex;

	require(nAttributeSparseIndex >= 0);

	// Recherche de l'index de valeur associee a l'index d'attribut
	nValueIndex = GetValueIndexAtAttributeSparseIndex(nAttributeSparseIndex);
	if (nValueIndex != -1)
		return GetValueAt(nValueIndex);
	else
		return cDefaultValue;
}

inline Continuous KWContinuousValueBlock::GetDefaultDefaultValue()
{
	return KWContinuous::GetMissingValue();
}

// Classe KWSymbolValueBlock

inline KWSymbolValueBlock* KWSymbolValueBlock::NewValueBlock(int nSize)
{
	KWSymbolValueBlock* newValueBlock;
	void* pValueBlockMemory;

	// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee
	// (attention, C++ avance)
	pValueBlockMemory = GenericAllocValueBlock(nSize);
	newValueBlock = new (pValueBlockMemory) KWSymbolValueBlock;
	GenericInitValueBlock(pValueBlockMemory, nSize);
	assert(newValueBlock->nValueNumber == nSize);
	return newValueBlock;
}

inline KWSymbolValueBlock::KWSymbolValueBlock() {}

inline Symbol& KWSymbolValueBlock::GetValueAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (nValueNumber <= nSegmentSize
		    ? ((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.GetSymbol()
		    : ((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
			  .value.GetSymbol());
}

inline void KWSymbolValueBlock::SetValueAt(int nValueIndex, const Symbol& sValue)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.SetSymbol(sValue);
	else
		((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
		    .value.SetSymbol(sValue);
}

inline Symbol& KWSymbolValueBlock::GetValueAtAttributeSparseIndex(int nAttributeSparseIndex,
								  Symbol& sDefaultValue) const
{
	int nValueIndex;

	require(nAttributeSparseIndex >= 0);

	// Recherche de l'index de valeur associee a l'index d'attribut
	nValueIndex = GetValueIndexAtAttributeSparseIndex(nAttributeSparseIndex);
	if (nValueIndex != -1)
		return GetValueAt(nValueIndex);
	else
		return sDefaultValue;
}

// Classe KWObjectArrayValueBlock

inline KWObjectArrayValueBlock* KWObjectArrayValueBlock::NewValueBlock(int nSize)
{
	KWObjectArrayValueBlock* newValueBlock;
	void* pValueBlockMemory;

	// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee
	// (attention, C++ avance)
	pValueBlockMemory = GenericAllocValueBlock(nSize);
	newValueBlock = new (pValueBlockMemory) KWObjectArrayValueBlock;
	GenericInitValueBlock(pValueBlockMemory, nSize);
	assert(newValueBlock->nValueNumber == nSize);
	return newValueBlock;
}

inline KWObjectArrayValueBlock::KWObjectArrayValueBlock() {}

inline ObjectArray* KWObjectArrayValueBlock::GetValueAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (nValueNumber <= nSegmentSize
		    ? ((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.GetObjectArray()
		    : ((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
			  .value.GetObjectArray());
}

inline void KWObjectArrayValueBlock::SetValueAt(int nValueIndex, ObjectArray* oaValue)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	require(GetValueAt(nValueIndex) == NULL);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPair*)&cStartBlock)[nValueIndex].value.SetObjectArray(oaValue);
	else
		((KWValueIndexPair**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
		    .value.SetObjectArray(oaValue);
}

inline ObjectArray* KWObjectArrayValueBlock::GetValueAtAttributeSparseIndex(int nAttributeSparseIndex) const
{
	int nValueIndex;

	require(nAttributeSparseIndex >= 0);

	// Recherche de l'index de valeur associee a l'index d'attribut
	nValueIndex = GetValueIndexAtAttributeSparseIndex(nAttributeSparseIndex);
	if (nValueIndex != -1)
		return GetValueAt(nValueIndex);
	else
		return NULL;
}

inline Symbol& KWSymbolValueBlock::GetDefaultDefaultValue()
{
	static Symbol sEmptySymbol;
	return sEmptySymbol;
}
