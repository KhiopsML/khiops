// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

/////////////////////////////////////////////////////////////
// Portabilite                                             //
// Des define permettent les variantes de compilateur      //
//   __UNIX__      Unix(HPUX teste)                        //
//   _MSC_VER      Visual C++(defini par le compilateur)   //
//   __WGPP__	   Windows gpp                             //
//   __clang__	   Compilateur utilise pour Android        //
// Il faut positionner ci dessous le define necessaire     //
/////////////////////////////////////////////////////////////

// Verification qu'une plate-forme au moins est active
#if !defined _MSC_VER && !defined __UNIX__ && !defined __WGPP__ && !defined __ANDROID__
#error _MSC_VER, __UNIX__, __WGPP__ or __ANDROID__ undefined
#endif

// Parametrage our la compilation sur ANDROID
#ifdef __ANDROID__
#ifndef __UNIX__
#define __UNIX__
#endif //__UNIX__
#ifndef __clang__
#error clang compiler must be used on Android platform
#endif //__clang__
#endif //__ANDROID__

// Tous les include aux librairies systemes sont faite dans ce header,
// ce qui permet de limiter les eventuels problemes de portabilite
#ifdef __UNIX__
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <sys/time.h>
#endif

#if defined __UNIX__ || defined __WGPP__
#include <dirent.h>
#endif

// Visual C++: supression des Warning
#ifdef _MSC_VER
#pragma warning(disable : 4514)  // disable C4514 warning(unreferenced inline function has been removed)
#pragma warning(disable : 4710)  // disable C4710 warning
#pragma warning(disable : 4530)  // disable C4530 warning(compilation sans gestion des exceptions)
#pragma warning(disable : 4100)  // disable C4710 warning(unreferenced formal parameter)
#pragma warning(disable : 26444) // disable C26444 warning(string constante passee en parametre de type ALString)
#pragma warning(disable : 26451) // disable C26451 warning(depassement arithmetique)
#pragma warning(disable : 6011)  // disable 6011 warning(suppression de la reference du pointeur null)
#define _CRT_SECURE_NO_DEPRECATE // disable CRT deprecated warning.
#elif __GNUC__

// Pour clang et gcc, on utilise les options -Wall ou -Wextra
// qui sont quand meme trop verbeuses, on enleve les warnings suivants
// Le warning unused-but-set-variable de gcc est interessant pour detecter des problemes potentiels,
// mais on ne l'active pas en permanence car il y a beaucoup de "faux positifs"
// Il est conseille de l'active temporairement, par exemple avant chaque release majeure

#ifdef __clang__
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsequence-point"
#pragma clang diagnostic ignored "-Wmisleading-indentation"

// warnings actives en avec le flag extra
#pragma clang diagnostic ignored "-Wunused-parameter"

#else //__clang__
// Avec gcc
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsequence-point"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

// warnings actives en avec le flag extra
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif //__clang__
#endif // _MSC_VER

// Definition de macro pour desctiver les warnings
// copier/coller de https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/
// clang-format off
#if defined(_MSC_VER)
	#define DISABLE_WARNING_PUSH __pragma(warning(push))
	#define DISABLE_WARNING_POP __pragma(warning(pop))
	#define DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))
#elif defined(__GNUC__) || defined(__clang__)
	#define DO_PRAGMA(X) _Pragma(#X)
	#define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
	#define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
	#define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)
#else
	#define DISABLE_WARNING_PUSH
	#define DISABLE_WARNING_POP
#endif

// Definition des macros pour chaque compileur :
// Il faut definir la macro 4 fois : pour microsoft (_MSC_VER), clang, gcc et les autres
// Les warnings sont identifies par un numero pour microsoft, par une string sur gcc et clang
// Les warnings definis pour clang ne sont pas forcement definis pour gcc (et vice versa)
// Pour desactiver un warning dans le code il faut les 3 instructions suivantes
// 			DISABLE_WARNING_PUSH
//			code pour lequel le warning XXX n'est pas actif
// 			DISABLE_WARNING_XXX
// 			DISABLE_WARNING_POP
#if defined(_MSC_VER)
	#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(4100)
	#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(4505)
	#define DISABLE_WARNING_CLASS_MEMACCESS
	#define DISABLE_WARNING_UNUSED_FUNCTION
	#define DISABLE_WARNING_UNINITIALIZED
#elif defined(__clang__)
	#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused-parameter)
	#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused-function)
	#define DISABLE_WARNING_CLASS_MEMACCESS
	#define DISABLE_WARNING_UNUSED_FUNCTION DISABLE_WARNING(-Wunused-function)
	#define DISABLE_WARNING_UNINITIALIZED DISABLE_WARNING(-Wuninitialized)
#elif defined(__GNUC__)
	#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused-parameter)
	#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused-function)
	#define DISABLE_WARNING_CLASS_MEMACCESS DISABLE_WARNING(-Wclass-memaccess)
	#define DISABLE_WARNING_UNUSED_FUNCTION DISABLE_WARNING(-Wunused-function)
	#define DISABLE_WARNING_UNINITIALIZED DISABLE_WARNING(-Wuninitialized)
#else
	#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
	#define DISABLE_WARNING_UNREFERENCED_FUNCTION
	#define DISABLE_WARNING_CLASS_MEMACCESS
	#define DISABLE_WARNING_UNUSED_FUNCTION
	#define DISABLE_WARNING_UNINITIALIZED
#endif
// clang-format on
#ifdef _MSC_VER
#include <malloc.h>
#include <direct.h>
#include <io.h>
// Pour enpecher temporairement le formatage clang, qui colle and avec &&
// clang-format off
#define or ||
#define and &&
#define not !
// clang-format on
#endif

#ifdef __WGPP__
#include <io.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <csignal>
#include <iomanip>

#include <sys/types.h>
#include <sys/stat.h>

// Gestion de la portabilite pour certaines methodes specifiques
// Encapsulation de quelques fonctions standards C, en les prefixant par p_
// Les methodes sont identiques dans le cas gcc, et reimplementees pour MS Visual C++ 2008
// (qui a "declasse" ces fonctions en DEPRECATED)
// Il manque les methodes open, sscanf, sprintf

// Declaration de la structure stat
#if defined __UNIX__ or defined __WGPP__
typedef struct stat struct_stat;
#else
typedef struct _stat struct_stat;
#endif

// Test si la version de gcc est inferieure a 4.7
// On en a besoin pour les overrides (v 4.7) et les Rvalue references (v 4.3)  du C++11
#if defined __UNIX__ and not defined __C11__
#define override
#define final
#endif // defined __UNIX__ and not defined __C11__

// Liste des methodes reimplementees
const char* p_getenv(const char* varname);
int p_setenv(char* varname, const char* value);
struct tm* p_localtime(const time_t* timer);
struct tm* p_gmtime(const time_t* timer);
int p_stat(const char* path, struct_stat* buf);
FILE* p_fopen(const char* filename, const char* mode);
char* p_strcpy(char* strDestination, const char* strSource);
char* p_strncpy(char* strDest, const char* strSource, size_t count);
char* p_strcat(char* strDestination, const char* strSource);

// Le locale de l'application est parametre de facon a etre independant de la machine,
// pour assurer l'unicite des conversions numeriques et de leur format d'export, des tris,
// et des comparaisons entre chaines de caracteres
//
// Parametrage des locales: toute fonction de FileService base sur un nom de fichier entoure son appel
// d'un parametrage prealable au locale de la machine, puis restore le locale de l'application
// Dans le cas d'un besoin de parametrage de locale specifique, il est necessaire de repositionner
// ensuite le locale par defaut par appel a p_SetApplicationLocale.
void p_SetMachineLocale();
void p_SetApplicationLocale();

// Methode pour l'allocation de segments de grande taille
void* p_hugemalloc(size_t size);
void p_hugefree(void* memblock);

// Methodes de parcours fichiers sous Windows, pour implementation de FileService::GetDirectoryContent
// Il s'agit ici de methode "wrapper" vers les methodes natives de l'API Windows, ayant un prototype
// compatible entre Norm et Windows.h
void* p_NewFileData();
void p_DeleteFileData(void* pFileData);
void* p_FindFirstFile(const char* dirname, void* pFileData);
int p_FindNextFile(void* handle, void* pFileData);
const char* p_GetFileName(void* pFileData);
int p_IsDirectory(void* pFileData);
void p_FindClose(void* handle);

// Taille des buffers pour les 2 routines suivantes
#define BUFFER_LENGTH 512

// Renvoie un buffer de taille 500 max basee sur une variable statique
// Permet d'optimiser la memoire en evitant allocation et desallocation de variables de travail temporaires
// Jusqu'a 10 appels sont possibles simultanement avant de reutiliser le meme emplacement memoire
// Au dela, la fonction "boucle" et redonne acces aux memes buffers(au risque d'acces concurents)
// Fonction "privee" avancee
char* StandardGetBuffer();

// Renvoie une chaine lue depuis un fichier d'entree, et rangee dans le buffer en parametre(fonction "privee")
void StandardGetInputString(char* sBuffer, FILE* fInput);

/////////////////////////////////////////////////////////////////
// chargement de librairies partagees

// Taille maximale des messages construits par LoadSharedLibrary
// Il faut donc prevoir un buffer de cette taille, plus 1 pour le caractere fin de chaine
#define SHARED_LIBRARY_MESSAGE_LENGTH 512

// Chargement d'une librairie partagee.
// sLibraryPath : nom et chemin complet de la librairie (avec son extension).
// sErrorMessage : sortie qui vaut vide si le chargement a reussi, le message d'erreur sinon
// valeur de retour : handle sur la librairie ouverte, ou NULL si echec du chargement
void* LoadSharedLibrary(const char* sLibraryPath, char* sErrorMessage);

// Recuperation de l'adresse d'une fonction d'une librairie partagee.
// libraryHandle : handle valide sur une librairie deja chargee
// sfunctionName : nom de la fonction dont on veut recuperer l'adresse
// valeur de retour : adresse de la fonction, ou NULL si echec de la recherche
void* GetSharedLibraryFunction(void* libraryHandle, const char* sfunctionName);

// Liberation des ressources d'une librairie partagee deja chargee.
// libraryHandle : handle valide sur une librairie deja chargee
// valeur de retour : true si liberation effectuee, false sinon
int FreeSharedLibrary(void* libraryHandle);

/////////////////////////////////////////////////////////////////
// Implementation standard pour gcc, et pour MS VC++ avant 2005
#if defined __UNIX__ or defined __WGPP__ or _MSC_VER < 1400

inline int p_setenv(const char* varname, const char* value)
{
	return setenv(varname, value, 1);
}

inline struct tm* p_localtime(const time_t* timer)
{
	return localtime(timer);
}

inline struct tm* p_gmtime(const time_t* timer)
{
	return gmtime(timer);
}

inline int p_stat(const char* path, struct_stat* buf)
{
#if defined __UNIX__ or defined __WGPP__
	return stat(path, buf);
#else
	return _stat(path, buf);
#endif
}

inline FILE* p_fopen(const char* filename, const char* mode)
{
	struct_stat st;
	FILE* file;

	p_SetMachineLocale();
	file = fopen(filename, mode);
	p_SetApplicationLocale();

	// En lecture fopen renvoie un descripteur valide sur les repertoires
	// Si c'est un repertoire on met le pointeur a NULL
	if (file != NULL and mode[0] == 'r')
	{
		stat(filename, &st);
		if (S_ISDIR(st.st_mode))
		{
			fclose(file);
			file = NULL;
		}
	}
	return file;
}

inline char* p_strcpy(char* strDestination, const char* strSource)
{
	return strcpy(strDestination, strSource);
}

inline char* p_strncpy(char* strDest, const char* strSource, size_t count)
{
	return strncpy(strDest, strSource, count);
}

inline char* p_strcat(char* strDestination, const char* strSource)
{
	return strcat(strDestination, strSource);
}

#endif //  __UNIX__, __WGPP__, _MSC_VER < 1400

////////////////////////////////////////////////////
// Reimplementation pour MS Visual C++ 2005, 2008
#if _MSC_VER >= 1400

inline int p_setenv(const char* varname, const char* value)
{
	return _putenv_s(varname, value);
}

inline int p_stat(const char* path, struct_stat* buf)
{
	return _stat(path, buf);
}

inline FILE* p_fopen(const char* filename, const char* mode)
{
	errno_t err;
	FILE* stream;
	p_SetMachineLocale();
	err = fopen_s(&stream, filename, mode);
	p_SetApplicationLocale();
	if (err != 0)
		return NULL;
	else
		return stream;
}

inline char* p_strcpy(char* strDestination, const char* strSource)
{
	errno_t err = strcpy_s(strDestination, strlen(strSource) + 1, strSource);
	//	assert(err == 0);
	err = 0; // Pour eviter le warning en mode release
	return strDestination;
}

inline char* p_strncpy(char* strDestination, const char* strSource, size_t count)
{
	errno_t err = strncpy_s(strDestination, strlen(strSource) + 1, strSource, count);
	//	assert(err == 0);
	err = 0; // Pour eviter le warning en mode release
	return strDestination;
}

inline char* p_strcat(char* strDestination, const char* strSource)
{
	errno_t err = strcat_s(strDestination, strlen(strDestination) + strlen(strSource) + 1, strSource);
	//	assert(err == 0);
	err = 0; // Pour eviter le warning en mode release
	return strDestination;
}

#endif // _MSC_VER >= 1400
