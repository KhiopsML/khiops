// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Longint.h"
#include "Portability.h"

////////////////////////////////////////////////////////////////////////////
// Gestion des assertions, du debug et de la memoire
//
// Il existe cinq types d'instructions speciales, permettant d'augmenter
// la robustesse du code. Ces instructions sont tunables en utilisant des
// directives #define:
//    Pre-condition: require(exp); (NOREQUIRE)
//    Post-condition: ensure(exp); (NOENSURE)
//    Assertion: assert(exp); (NDEBUG)
//    Test contre NULL: check(designateur) (NOCHECK)
//    Debug: debug(instruction); (NODEBUG)
//
// Par ailleurs, il est possible de faire des cast securises entre
// sous classes de la classe Object
// Il suffit d'utiliser la syntaxe
//      cast(type*, object)
//   au lieu de
//     (type*)object
// En version active, le controle est base sur les mecanismes RTTI
// du C++, et se comporte comme une exception dans le cas d'un cast invalide
// On peut desactiver le controle par la directive
//    #define NOCASTCONTROL
//
// Le define NOALL est a utiliser pour la compilation optimisee du code.
// Il suffit de commenter ou decommenter les options suivantes(ou de le
// controler par des define a la compilation) pour gerer le niveau de
// controles du programme.
////////////////////////////////////////////////////////////////////////////

// Defines de controles des assertions et du debug
// #define NODEBUG
// #define NDEBUG
// #define NOREQUIRE
// #define NOENSURE
// #define NOCASTCONTROL
// #define NOCHECK

// Tous les #NO...
// #define NOALL

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion des types elementaires
// Les variables de type simple doivent etre prefixees par une lettre dediee au titre de normes de programmation.
// Les types elementaires les suivants:
//   boolean: (prefixe: b) variable booleenne (valeur true et false, operateurs and, or, not)
//   int: (prefixe: n (ou i, j..)) entier (calcul, index des tableaux)
//   ALString: (prefixe: s) chaines de caracteres
//   double: (prefixe: d) valeur reelles, pour tout calcul en virgule flotante
//   longint: (prefixe: l) entier long, uniquement pour les ressources systemes
//                         (taille de fichier, index dans fichier, taille de RAM...)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Type et operateurs booleens
#if not defined __UNIX__ or defined __C11__
typedef bool boolean;
#else
typedef int boolean;
#define true 1
#define false 0
#endif // __C11__

// Comparaison par type, pour implementation dans les methodes de comparaison
// Pour les entiers: une difference suffit? Pour les chaines de caracteres, utilise la methode Compare de ALString.
inline int CompareBoolean(boolean b1, boolean b2)
{
	return (b1 != false) - (b2 != false);
}
inline int CompareDouble(double d1, double d2)
{
	if (d1 == d2)
		return 0;
	else
		return d1 > d2 ? 1 : -1;
}
inline int CompareLongint(longint l1, longint l2)
{
	if (l1 == l2)
		return 0;
	else
		return l1 > l2 ? 1 : -1;
}

////////////////////////////////////////////
// Fonctions de type utilitaires standard //
////////////////////////////////////////////

// Conversion d'une duree en chaine de caractere,
// sous la forme hh:mm:ss.cc
const char* const SecondsToString(double dValue);

// Conversions des types simples vers les chaines de caracteres
const char* const IntToString(int nValue);
const char* const LongintToString(longint lValue);
const char* const FloatToString(float fValue);
const char* const DoubleToString(double dValue);
const char* const CharToString(char cValue);
const char* const BooleanToString(boolean bValue);
const char* const PointerToString(const void* pValue);

// Renvoie une copie de la chaine, dans une
// variable statique(sans probleme de memoire)
const char* const CharsToString(const char* sValue);

// Conversions des chaines de caracteres vers les types simples
int StringToInt(const char* sValue);
longint StringToLongint(const char* sValue);
float StringToFloat(const char* sValue);
double StringToDouble(const char* sValue);
char StringToChar(const char* sValue);
boolean StringToBoolean(const char* sValue);

// Acquisition de donnees elementaires
// Utiliser de preference les classes UserInterface
// Ces fonctions sont utiles des test de bas niveau dans les classes de base(container...)
// Elles sont implementees au niveau des session shell, avec une valeur defaut prise en compte
// quand l'utilisateur se contente d'un retour charriot
boolean AcquireBoolean(const char* const sLabel, boolean bDefaultValue);
int AcquireInt(const char* const sLabel, int nDefaultValue);
int AcquireRangedInt(const char* const sLabel, int nMin, int nMax, int nDefaultValue);
double AcquireDouble(const char* const sLabel, double dDefaultValue);
double AcquireRangedDouble(const char* const sLabel, double dMin, double dMax, double dDefaultValue);
const char* const AcquireString(const char* const sLabel, const char* const sDefaultValue);
//(En cas de depassement, la chaine et tronquee)
const char* const AcquireRangedString(const char* const sLabel, int nMinLength, int nMaxLength,
				      const char* const sDefaultValue);
longint AcquireLongint(const char* const sLabel, longint lDefaultValue);
longint AcquireRangedLongint(const char* const sLabel, longint lMin, longint lMax, longint lDefaultValue);

// Gestion d'un mode batch global(par defaut false)
// En mode batch, les fonctions Acquire... en mode batch retournent systematique la valeur
// par defaut en parametre, sans interaction utilisateur
void SetAcquireBatchMode(boolean bValue);
boolean GetAcquireBatchMode();

/////////////////////////////////////
// Generateur de nombre aleatoires //
/////////////////////////////////////

// Graine initiale de la suite pseudo-aleatoire
//(permet de reproduire la meme suite)
void SetRandomSeed(int nSeed);
int GetRandomSeed();

// Reel aleatoire, compris entre 0 et 1 inclus
double RandomDouble();

// Entier aleatoire, compris entre 0 et nMax inclus
int RandomInt(int nMax);

// Acces directement au ieme nombre aleatoire
// Fonctions independantes des precedentes, sans graine initiale
// L'index est un entier long
double IthRandomDouble(longint lIndex);     // Entre 0 et 1
longint IthRandomLongint(longint lIndex);   // Longint quelconque
int IthRandomInt(longint lIndex, int nMax); // Entre 0 et nMax inclus

// Hashage d'une chaine de caracteres
int HashValue(const char* sString);

// Mise a jour d'une valeur de hash existante
// Permet par exemple de hash un vecteur iterativement
int UpdateHashValue(int nCurrentHashValue, int nHashValue);
longint LongintUpdateHashValue(longint lCurrentHashValue, longint lHashValue);

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#ifdef NOALL
#define NODEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#define NOREQUIRE
#define NOENSURE
#define NOCHECK
#define NOCASTCONTROL
#endif

//////////////////////////////////////////////////
// Gestion des assertions et du debug           //
// Declaration des fonctions et macros privees  //
//////////////////////////////////////////////////

// Gestion d'une erreur de contrat
void _AssertionFailure(const char* __cond, const char* __file, const unsigned __line);
void _RequireFailure(const char* __cond, const char* __file, const unsigned __line);
void _EnsureFailure(const char* __cond, const char* __file, const unsigned __line);
void _CheckFailure(const char* __cond, const char* __file, const unsigned __line);
void _CastFailure(const char* __type, const char* __object, const char* __file, const unsigned __line);

// Sortie brutale du programme
// En mettant un point d'arret dans la fonction  correspondante, l'utilisation d'un debugger symbolique
// permet d'obtenir la cause de l'erreur avec la pile d'appel au moment de la sortie
// Le GlobalExit ferme tous les fichier, appelle la fonction optionnelle UserExit, puis le exit systeme
void GlobalExit();

// Idem GlobalExit mais appelle exit(0) (succes)
void GlobalExitOnSuccess();

// Prototype d'une fonction de sortie utilisateur, appelee juste avant un exit() systeme
// Le parametre est le code retour (0 ou 1) qui sera retourne par l'application
typedef void (*UserExitHandler)(int);

// Ajout d'une fonction de sortie utilisateur (par defaut, il n'y en a pas)
// Appele en cas de sortie brutale
void AddUserExitHandler(UserExitHandler fUserExit);

// assertion
#undef assert
#ifdef NDEBUG
#define assert(p) ((void)0)
#else
#define assert(p) ((p) ? (void)0 : (void)_AssertionFailure(#p, __FILE__, __LINE__))
#endif

// pre-condition
#undef require
#ifdef NOREQUIRE
#define require(ignore) ((void)0)
#else
#define require(p) ((p) ? (void)0 : (void)_RequireFailure(#p, __FILE__, __LINE__))
#endif

// post-condition
#undef ensure
#ifdef NOENSURE
#define ensure(ignore) ((void)0)
#else
#define ensure(p) ((p) ? (void)0 : (void)_EnsureFailure(#p, __FILE__, __LINE__))
#endif

// test contre NULL
#undef check
#ifdef NOCHECK
#define check(ignore) ((void)0)
#else
#define check(p) ((p) ? (void)0 : (void)_CheckFailure(#p, __FILE__, __LINE__))
#endif

////////////////////////
// cast securise

// Verification de l'option de compilation RTTI du C++
#ifndef NOCASTCONTROL
#ifndef _CPPRTTI
#error "In safe cast version, use RTTI compile option"
#endif
#endif

// Object buffer pour les controle de cast, pour eviter
// plusieurs evaluation de object(et donc poser un probleme
// pour les expressions a effet de bord)
class Object;
extern const Object* objectCastControlBuffer;

#ifdef _MSC_VER
// C28182: "unreference the NULL pointer..." (pour la macro cast)
#pragma warning(disable : 28182) // disable 28182 warning
#endif

// Test de cast
// Portage unix : ajout du typage du pointeur (type)NULL au lieu de NULL
// Note:  le define est un peu "tricky".
//  On passe par une affectation temporaire a la variable objectCastControlBuffer definie dans standard.
//  Cela permet de faire les controles par une expression simple cast(MyClass*, myObject), au prix
//  d'un define peu orthodoxe. Cela peut genere un warning avec certains compilateurs.
#undef cast
#ifdef NOCASTCONTROL
#define cast(type, object) ((type)(object))
#else
#define cast(type, object)                                                                                             \
	(((objectCastControlBuffer = (object)) == NULL ? (type)((Object*)objectCastControlBuffer)                      \
	  : dynamic_cast<type>((Object*)objectCastControlBuffer) != NULL                                               \
	      ? (type)((Object*)objectCastControlBuffer)                                                               \
	      : (_CastFailure(#type, #object, __FILE__, __LINE__), (type)NULL)))
#endif

// Instruction de debug
#undef debug
#ifdef NODEBUG
#define debug(exp)
#else
#define debug(exp) exp
#endif

// Fonction de hash Jenkins one at a time
inline int HashValue(const char* sString)
{
	unsigned int nHash = 0;
	const char* sChars = sString;
	while (*sChars)
	{
		nHash += *sChars;
		nHash += (nHash << 10);
		nHash ^= (nHash >> 6);
		sChars++;
	}
	nHash += (nHash << 3);
	nHash ^= (nHash >> 11);
	nHash += (nHash << 15);
	return nHash;
}

inline int UpdateHashValue(int nCurrentHashValue, int nHashValue)
{
	return nCurrentHashValue * 31 + nHashValue;
}

inline longint LongintUpdateHashValue(longint lCurrentHashValue, longint lHashValue)
{
	return lCurrentHashValue * 31 + lHashValue;
}

// Gestion du multi-process
// Le process principal (maitre) a pour ID 0 (valeur par defaut)
int GetProcessId();

// L'ID d'un process peut etre change pour un process secondaire (esclave)
// Utile notamment pour identifier les messages d'erreurs systeme et les lier a un process(par exemple avec MPI)
// Attention: usage reserve a la bibliotheque parallele
void SetProcessId(int nValue);

// Traces pour debugger les applications paralleles
// Ajoute la rang devant la trace et l'ecrit dans la sortie standard
void TraceMaster(const char* sTrace);
void TraceSlave(const char* sTrace);
void TraceWithRank(const char* sTrace);

// Gestion de la memoire
#include "MemoryManager.h"
