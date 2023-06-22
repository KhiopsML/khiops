// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Standard.h"
#include <stdlib.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////
// Gestion de la memoire
//
// Note sur la portabilite 32 bits/64 bits
// On adopte les convention suivante pour l'utilisation des differents
// variante de type entier
//    void*: pour tout ce qui est pointeur
//    uintptr_t: pour caster un pointeur en type numerique, ce qui permet d'effectuer
//               des calculs de difference et des comparaisons
//    size_t: pour tout ce qui est taille de pointeur
//    size_t: pour tout compteur (potentiellement grand)
//    int: pour l'affichage des tailles dont on sait qu'elle tiennent sur 32 bit, meme si elles
//            sont de type size_t (en utilisant des printf("%d"))
//    longint: pour l'affichage des compteurs, en les castant en longint
//            ce qui permet de se rendre independant du type 32 bits ou 64 bits de size_t
//            On utilise des printf("%lld") pour une precision entiere maximale

// Declaration d'une variable definie dans standard.cpp
extern int nStandardGlobalExit;

// Statistiques globales collectees en permanence sur la taille  de la heap
static const longint MemHeapInitialSystemMemory =
    4194304; // Taille systeme initiale estimee, au lancement d'un programme
static longint MemHeapMemory = MemHeapInitialSystemMemory;               // Taille de heap courante
static longint MemHeapMaxRequestedMemory = MemHeapInitialSystemMemory;   // Taille de heap max
static longint MemHeapTotalRequestedMemory = MemHeapInitialSystemMemory; // Total des allocations sur la heap
static longint MemHeapCurrentSegmentNumber = 0; // Nombre courant de segments systemes alloues sur la heap

// Taille max de la heap a ne pas depasser
static longint lMemMaxHeapSize = 0;

// Gestion des erreurs fatales: declaration de la methode
static void MemFatalError(const char* sAllocErrorMessage);

// Mise a jour des statiques globales
inline void MemHeapUpdateAlloc(size_t nSize)
{
	// Mise a jour des stats
	MemHeapMemory += nSize;
	if (MemHeapMemory > MemHeapMaxRequestedMemory)
		MemHeapMaxRequestedMemory = MemHeapMemory;
	MemHeapTotalRequestedMemory += nSize;

	// test de depassement memoire
	if (lMemMaxHeapSize > 0 && MemHeapMemory > lMemMaxHeapSize)
	{
		char sMessage[100];
		sprintf(sMessage, "Memory user overflow: heap size beyond user limit (%lld)\n", lMemMaxHeapSize);
		MemFatalError(sMessage);
		GlobalExit();
	}
}

inline void MemHeapUpdateFree(size_t nSize)
{
	MemHeapMemory -= nSize;
}

// Parametrage de la gestion des statistiques fines d'allocation
// A activer pour une connaissance fine de ce qui a ete alloue, en debug ou release
#define MEMSTATSMAXSIZE 1000000
#undef MEMSTATSCOLLECT
// #define MEMSTATSCOLLECT

// Activation des operations de controle de la gestion memoire
#ifdef NDEBUG
#define NOMEMCONTROL
#define MemDebug(exp)
#else
#define MemDebug(exp) exp
#endif

#ifdef RELEASENEWMEM
// Memoire provisionnee pour la gestion des erreurs fatales
// En effet, le traitement avance (avec interface utilisateur) des message
// d'erreur peut demander de la memoire, indisponible en cas d'erreur fatale de l'allocateur
// On permet alors d'initialiser une reserve de memoire, liberee avant le traitement des erreurs fatales
// (on prend une taille superieur a celle des thread Java (cf UIObject::GraphicGetJNIEnv():
// sDefaultStackSize="-Xss512k")
static size_t nMemFatalErrorReserveMemorySize = 1048576;
// On utilise un unnamed namespace pour forcer la localite dees utilisation et eviter els probleme potentiels de DLL
namespace
{
static void* pMemFatalErrorReserveMemory = NULL;
} // namespace

// Reservation de la memoire de traitement des erreurs fatales
static void MemAllocFatalErrorReserveMemory()
{
	if (pMemFatalErrorReserveMemory == NULL)
	{
		MemHeapUpdateAlloc(nMemFatalErrorReserveMemorySize);
		pMemFatalErrorReserveMemory = p_hugemalloc(nMemFatalErrorReserveMemorySize);
	}
}
#endif // RELEASENEWMEM

// Gestion des erreurs fatales
static void MemFatalError(const char* sAllocErrorMessage)
{
#ifdef RELEASENEWMEM
	// Liberation de la reserve de memoire pour le traitement de l'erreur fatale
	if (pMemFatalErrorReserveMemory != NULL)
	{
		MemHeapUpdateFree(nMemFatalErrorReserveMemorySize);
		p_hugefree(pMemFatalErrorReserveMemory);
		pMemFatalErrorReserveMemory = NULL;
	}
#endif // RELEASENEWMEM

	// Traitement de l'erreur fatale
	if (MemGetAllocErrorHandler() != NULL)
		MemGetAllocErrorHandler()(sAllocErrorMessage);
}

// Gestionnaire par defaut des erreurs d'allocation
void fMemDefaultAllocErrorHandler(const char* sAllocErrorMessage)
{
	FILE* fError;

	// Message dans la session DOS
	if (GetProcessId() == 0)
		printf("%s", sAllocErrorMessage);
	else
		printf("Process %d: %s", GetProcessId(), sAllocErrorMessage);

	// Message dans un fichier
	fError = p_fopen("MemError.log", "w");
	if (GetProcessId() == 0)
		fprintf(fError, "%s", sAllocErrorMessage);
	else
		fprintf(fError, "Process %d: %s", GetProcessId(), sAllocErrorMessage);
#ifdef MEMSTATSCOLLECT
	MemPrintHeapStats(fError);
#endif
	fclose(fError);

	GlobalExit();
}
static MemAllocErrorHandler fMemAllocErrorHandler = fMemDefaultAllocErrorHandler;

// Allocation d'un block
void* MemAlloc(size_t nSize);

// Liberation d'un block
void MemFree(void* pBlock);

// Renvoie la taille effectivement alouee d'un bloc
// Pour les systemes ne possedant pas une telle routine pour l'allocateur
// standard, on renverra MaxBlockSize+1 pour les blocks aloues par malloc
// Longueur d'un block
size_t MemBlockSize(void* pBlock);

//////////////////////////////////////////////////////////////////////
//                    Gestion du nouvel allocateur
//
// Le nouvel allocateur est un subclassing de l'allocateur standard,
// optimise pour l'allocation des blocks de petite taille (<= 256),
// de moyenne taille (<= 32768) et de grande taille (> 32768).
// Il repose sur l'allocation de blocks de taille fixe pour les petites et
// moyennes tailles, ce qui permet  de ne gerer que la position des "trous"
// dans la heap (et non leur taille).
// Toute est base sur des segments de taille fixes (64 ko), ce qui permet
// d'utiliser les ressources du systeme avec une granularite constante.
// Pour les blocks de petites tailles, on a des segments specialise par
// taille augmentant de facon lineaire entre 0 et 256 par pas de
// sizeof(void*) (4 ou 8 octet selon le mode 32 bits ou 64 bits).
// Pour les blocks de moyenne taille, on a des segments specialise par
// taille augmentant de facon exponetielle entre 256 et 32768, par facteur 2
// (en tenant compte de l'overhead par block pour mieux remplir chaque segment).
// Pour les blocks de grande taille, on alloue systematique un multiple de
// la taille de base de segment, ce multiple etant une puissance de deux.
// Cette strategie reduit de facon drastique le morcellement de la memoire.
//
// La heap du nouvel allocateur est code sous la structure "MemHeap".
// Il gere un tableau de heaps specialisees dans l'allocation de blocks de
// taille fixes ("FixedSizeHeap"), un tableau de segments ("MemSegment"),
// qui sont aloues au fur et a mesure des besoins des FixedSizeHeap, et une
// liste chainees (parmi ces segments) de segments libres.
//
// Une FixedSizeHeap est une liste chainee de segments (specialises pour une
// taille donnee). Le premier segment (s'il existe) de cette liste doit toujours
// etre disponible pour une prochaine allocation. Cela signifie que quand un
// segment devient plein (suite a une allocation), il est retire de la
// tete de liste et place en que de liste.
// Quand il redevient non plein (suite a une liberation), il est rajoute
// en tete de liste (ainsi, en general, les segments sont d'autant plus pleins
// qu'ils sont proches de la tete de liste).
// Un nouveau segment est aloue des qu'il n'y a plus de memoire disponible
// dans une FixedSizeHeap.
//
// Chaque segment est un tableau de blocks de taille fixe, et gere la sub-
// allocation dans ce tableau. Il maintient une liste chainee de "trous", en
// stockant dans les blocks libres l'adresse intra-segment du block suivant.
// La sub-allocation des segment est realisee au fil de l'eau, au fur et
// a mesure des allocations.
//
// Pour gerer la liberation de la memoire, chaque block est en fait aloue
// avec la place d'un pointeur supplementaire, en offset -sizeof(void*).
// Cet pointeur reference le segment possedant le block (ou NULL si le
// block a ete alloue par malloc).
//
// Pour gerer la liberation des segments complets, un compteur de block
// est gere dans chaque segment. Quand un segment devient vide, il est libere
// aupres de l'allocateur systeme
//
// L'allocateur est dedie aux programme faisant de tres nombreuses allocations,
// et il est gourmand en memoire pour les petits programmes, en reservant des
// segments memoires de grandes tailles pour chaque taille d'allocation, ce qui
// consomment beaucoup de memoire inutilement
// Pour remedier a ce probleme, on prereserve un (unique) segment de petite
// taille par taille d'allocation, ce qui permet de minimiser la memoire
// consommee quand tres peu de blocs memoire sont necessaires.
//
// Le mode controle de la memoire (NOMEMCONTROL pour le desactiver) coincide
// avec la compilation en mode debug. Dans ce mode, on peut controler les
// erreurs utilisateurs suivantes:
//    - non liberation de memoire (Warning)
//	  - desallocation deux fois d'un meme block (erreur fatale)
//    - utilisation d'un block desalloue (erreur fatale)
//    - ecrasement memoire (erreur fatale)
// Pour cela, on alloue de la place supplementaire pour chaque block:
//     Offset (en multiple de sizeof(void*)
//		-5: adresse du segment memoire
//      -4: adresse du prochain block libre
//		-3: status (MemStatusInit, MemStatusAlloc, MemStatusFree)
//      -2: alloc number (identifiant: numero global d'allocation du block)
//		-1: MemPattern (pour detecter les ecrasements memoires)
//       0: User data
//      AllocSize: MemPattern
// En mode release, les seules information des blocks sont:
//		-1: adresse du segment memoire
//       0: adresse du prochain block libre, ou user data

// Taille des plus grands blocks alouables dans des segment dedie aux petites tailles de blocks
const size_t MemSmallAllocMaxSize = 256;

// Taille des plus grands blocks alouables dans des segment dedie aux tailles moyennes de blocks
// Il faut tenir compte de l'overhead des blocks
const size_t MemMediumAllocMaxSize = MemSegmentByteSize;

// Tableau des tailles d'allocations des blocks de taille moyenne
// On a prevu une initialisation statique a 0, a completer par l'appel de la methode MemInitMediumAllSizes
static size_t MemMediumAllSizes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Taille alloue pour les segments systeme
// Chaque segment systeme est alloue avec une grande taille (en general multiple de 4 ko ou 64 ko),
// alors que les bloc utilisateurs, jusqu'a la taille maximale des segments utilisateurs (par exemple: 64 ko)
// doivent etre alloues avec un overhead (une taille d'un pointeur permettant de gerer les desallocations).
// Afin de tirer partie au maximum de la taille des segments systemes, ceux-ci sont constitues d'un multiple
// des tailles de bloc utilisateurs (y compris pour les plus grandes tailles de blocs utilisateurs)
// Au dela de cette taille, l'allocateur systeme est appele directement
const size_t MemSystemSegmentByteSize = MemSegmentByteSize * 8 - 8 * sizeof(void*);

// Taille disponible des segments  en multiple de sizeof(void*)
// La taille est choisie pour pouvoir allouer exactement des segments de taille utilisateur une puissance de 2
const size_t MemSystemSegmentSize = (MemSystemSegmentByteSize - sizeof(void*)) / sizeof(void*);

// Nombre maximal de heap de taille fixes
//   Pour les petites tailles: une par multiple de sizeof(void*) jusqu'a MemSmallAllocMaxSize
//   Pour les moyennes tailles: une par facteur 1.5 et 2 de MemSmallAllocMaxSize jusqu'a MemMediumAllocMaxSize
// La constante suivante est verifiee dans la methode MemCheckAllocParameters
// Cette constante est a ajuster si l'on change la taille des segments (16 pour 64 ko, 18 pour 128 ko, 200 pour 256
// ko...)
const size_t MemMediumSegmentNumber = 16;
const size_t MemFixedSizeHeapMaxNumber = MemSmallAllocMaxSize / sizeof(void*) + MemMediumSegmentNumber;

// Tableau des index de FixedSizeHeap par index de taille (en multiple de MemSmallAllocMaxSize/2)
static int MemMediumFixedSizeHeapIndexes[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Calcul de l'index de FixedSizeHeap pour une taille d'allocation moyenne
inline void MemSetMediumFixedSizeHeapIndex(size_t nSize, int nIndex)
{
	assert(MemSmallAllocMaxSize < nSize and nSize <= MemSegmentByteSize);
	assert(MemSmallAllocMaxSize / sizeof(void*) <= nIndex and nIndex < MemFixedSizeHeapMaxNumber);
	MemMediumFixedSizeHeapIndexes[(nSize + (MemSmallAllocMaxSize / 2) - 1) / (MemSmallAllocMaxSize / 2)] = nIndex;
}
inline int MemGetMediumFixedSizeHeapIndex(size_t nSize)
{
	assert(MemSmallAllocMaxSize < nSize and nSize <= MemSegmentByteSize);
	return MemMediumFixedSizeHeapIndexes[(nSize + (MemSmallAllocMaxSize / 2) - 1) / (MemSmallAllocMaxSize / 2)];
}

// Initialisation des tailles de blocks
void MemInitMediumAllSizes()
{
	size_t i;
	size_t nSize;
	size_t nMediumSize;
	size_t nMediumIndex;
	size_t nFixedSizeHeapIndex;

	// On cree des tailles intermediaires a partir de la plus grande des petites tailles
	// avec des paire (taille*1.5, taille*2) pour un echelonnement progressif des tailles
	nSize = sizeof(MemMediumAllSizes) / sizeof(size_t);
	nMediumSize = MemSmallAllocMaxSize;
	i = 0;
	while (i < nSize)
	{
		MemMediumAllSizes[i] = nMediumSize + nMediumSize / 2;
		assert(MemMediumAllSizes[i] <= MemSegmentByteSize);
		if (MemMediumAllSizes[i] >= MemSegmentByteSize)
			break;
		i++;
		nMediumSize *= 2;
		MemMediumAllSizes[i] = nMediumSize;
		assert(MemMediumAllSizes[i] <= MemSegmentByteSize);
		if (MemMediumAllSizes[i] >= MemSegmentByteSize)
			break;
		i++;
	}

	// Initialisation du tableau des index de FixedSizedHead pour les tailles moyennes
	assert(sizeof(MemMediumFixedSizeHeapIndexes) / sizeof(int) > MemSegmentByteSize / MemSmallAllocMaxSize);
	nSize = MemSmallAllocMaxSize;
	while (nSize < MemSegmentByteSize)
	{
		nSize += MemSmallAllocMaxSize / 2;

		// Recherche de l'index de la heap responsable de l'allocation
		nMediumIndex = 0;
		while (nSize > MemMediumAllSizes[nMediumIndex])
		{
			nMediumIndex++;
			assert(nMediumIndex < sizeof(MemMediumAllSizes) / sizeof(size_t));
		}

		// Initialisation de l'index
		nFixedSizeHeapIndex = MemSmallAllocMaxSize / sizeof(void*) + nMediumIndex;
		MemSetMediumFixedSizeHeapIndex(nSize, (int)nFixedSizeHeapIndex);
	}
}

// Affichage du tableau des tailles de block
void MemShowAllocParameters()
{
	int i;

	if (GetProcessId() != 0)
		printf("Process %d\n", GetProcessId());
	printf("Size of (void*)\t%lld\n", (longint)sizeof(void*));
	printf("Segment size\t%lld\n", (longint)MemSegmentByteSize);
	printf("Allocated Segment size\t%lld\n", (longint)MemSystemSegmentByteSize);
	printf("Small alloc max size\t%lld\n", (longint)MemSmallAllocMaxSize);
	printf("Medium alloc max size\t%lld\n", (longint)MemMediumAllocMaxSize);

	// Affichage des taille de block de taille moyenne
	printf("Medium alloc all sizes\n");
	i = 0;
	while (MemMediumAllSizes[i] <= MemMediumAllocMaxSize)
	{
		printf("\t%lld\n", (longint)MemMediumAllSizes[i]);
		i++;
	}
}

// Verification des coherence des parametres d'allocation (a appeler dans des asserts)
int MemCheckAllocParameters()
{
	boolean bOk = true;
	double dPower2;
	size_t i;

	bOk = bOk && MemSmallAllocMaxSize <= INT_MAX;
	bOk = bOk && MemMediumAllocMaxSize <= INT_MAX;
	bOk = bOk && MemSegmentByteSize <= INT_MAX;
	bOk = bOk && MemSmallAllocMaxSize <= MemMediumAllocMaxSize;
	bOk = bOk && MemMediumAllocMaxSize <= MemSegmentByteSize;
	bOk = bOk && MemSystemSegmentByteSize >= MemSegmentByteSize + sizeof(void*);
	dPower2 = log(MemSmallAllocMaxSize * 1.0) / log(2.0);
	bOk = bOk && fabs(dPower2 - (int)dPower2) < 1e-10;
	dPower2 = log(MemMediumAllocMaxSize * 1.0) / log(2.0);
	bOk = bOk && fabs(dPower2 - (int)dPower2) < 1e-10;
	dPower2 = log(MemSegmentByteSize * 1.0) / log(2.0);
	bOk = bOk && fabs(dPower2 - (int)dPower2) < 1e-10;
	dPower2 = MemSmallAllocMaxSize / sizeof(void*) +
		  2 * log(MemMediumAllocMaxSize * 1.0 / MemSmallAllocMaxSize) / log(2.0);
	bOk = bOk && fabs(MemFixedSizeHeapMaxNumber - dPower2) < 1e-10;

	// Verification des tailles d'allocation des blocks de tailles moyenne
	for (i = 0; i < sizeof(MemMediumAllSizes) / sizeof(size_t); i++)
	{
		bOk = bOk && MemMediumAllSizes[i] > MemSmallAllocMaxSize;
		bOk = bOk && MemMediumAllSizes[i] <= MemMediumAllocMaxSize;
		if (i > 0)
			bOk = bOk && MemMediumAllSizes[i] > MemMediumAllSizes[i - 1];
		if (MemMediumAllSizes[i] >= MemMediumAllocMaxSize)
		{
			assert(i > 0);
			assert(MemMediumSegmentNumber == i + 1);
			bOk = bOk && MemMediumAllSizes[i] == MemMediumAllocMaxSize;
			break;
		}
	}

	return bOk;
}

// Verification de la coherence entre une taille de bloc (exprimee en multiple de sizeof(void*))
// et un index de FixedSizeHeap (a appeler dans des asserts)
int MemCheckBlockSize(size_t nBlockSize, size_t nFixedSizeHeapIndex)
{
	boolean bOk = true;
	size_t nMediumIndex;

	// Verifications standards
	bOk = bOk && nBlockSize > 0;
	bOk = bOk && nFixedSizeHeapIndex < MemFixedSizeHeapMaxNumber;

	// Cas des petites tailles
	if (nBlockSize <= MemSmallAllocMaxSize / sizeof(void*))
	{
		// La taille du block donne l'index de la FixedSizeHeap dediee
		bOk = bOk && nFixedSizeHeapIndex + 1 == nBlockSize;
	}
	// Cas des moyennes tailles
	else if (nBlockSize <= MemMediumAllocMaxSize / sizeof(void*))
	{
		// Recherche de l'index de la heap responsable de l'allocation
		nMediumIndex = 0;
		while (nBlockSize > MemMediumAllSizes[nMediumIndex] / sizeof(void*))
		{
			nMediumIndex++;
			assert(nMediumIndex < sizeof(MemMediumAllSizes) / sizeof(size_t));
		}
		bOk = bOk && (nBlockSize == MemMediumAllSizes[nMediumIndex] / sizeof(void*));

		// Cela fournit un index de FixedSizeHeap dediee, apres les FixedSizeHeap de petite taille
		bOk = bOk && nFixedSizeHeapIndex == MemSmallAllocMaxSize / sizeof(void*) + nMediumIndex;

		// Verification du tableau des index de FixedFizeHeap
		bOk =
		    bOk and (size_t) MemGetMediumFixedSizeHeapIndex(nBlockSize * sizeof(void*)) == nFixedSizeHeapIndex;
	}
	// Cas des grandes tailles
	else
	{
		// Les grandes tailles ne sont pas traitees dans les segment
		bOk = 0;
	}
	return bOk;
}

#ifndef NOMEMCONTROL
// Offset des informations des blocks en mode controle memoire
const int MemOffsetStatus = -3;
const int MemOffsetAllocNumber = -2;
const int MemOffsetPattern = -1;

// Taille supplementaire reservee pour le controle memoire
const int MemControlHeaderSize = 4;
const int MemControlOverhead = 5;
#endif // NOMEMCONTROL

// Status
static void* MemPattern = NULL;
static void* MemStatusInit = NULL;
static void* MemStatusAlloc = NULL;
static void* MemStatusFree = NULL;
static void* MemStatusMicrosoftNoMansland = NULL;

// Numero d'allocation pour test d'arret
static longint lMemAllocIndexExit = 0;

// Taille d'allocation pour test d'arret
static int nMemAllocSizeExit = 0;

// Flag d'allocation system pour test d'arret
#ifndef NODEBUG
static boolean bMemSystemAllocExit = false;
#endif // NODEBUG

// Block d'allocation pour test d'arret
static void* pMemAllocBlockExit = NULL;

//////////////////////////////////////////////////////////////////////
//                            Gestion des blocs

// Un block permet de donner acces a taille(block) - sizeof(void*)
// de memoire.
// A l'offset -sizeof(void*) on trouve l'adresse du segment (ou
// NULL si allocation avec malloc)
// Les premiers octets contiennent un pointeur sur le block suivant
// (ou NULL s'il n'y a pas de block suivant)

//////////////////////////////////////////////////////////////////////
//                 	 Declaration de MemSegment

// Definition de la structure d'un segment
// Il est plus efficace d'avoir les donnees (pData) comme resultat
// d'une allocation explicite plutot que comme un tableau inclu dans
// la structure du segment. Cela permet d'avoir l'ensemble des structure
// de segments prealloue avec la structure de la heap de facon contigue
// en memoire, et ameliore ainsi les manipulations sur les segments.
// Un segment etant de taille limitee (typiquement 64 ko), l'utilisation
// du type int pour nRealBlockSize, nFixedSizeHeapIndex et nNbBlock est valide
// Les segments sont faits pour etre organises sous forme de liste doublement chainee
// Les donnees du segment avec une taille de 1, mais en fait alloue  selon la taille
// necessaire, cela revient a gerer une structure de taille variable determinee dynamiquement
typedef struct _MemSegment
{
	boolean bIsPredefined;           // Flag indiquant si le segment est predefini (jamais desalloue)
	int nRealBlockSize;              // Taille totale des blocks (chaque block memorise un pointeur sur son segment)
	size_t nFixedSizeHeapIndex;      // Index de la FixedSizeHeap correspondant dans la structure MemHeap
	void* pFirstBlock;               // Adresse dans le segment du premier "trou"
	int nNbBlock;                    // Nombre de blocks aloues
	void* pLastAllocatedBlock;       // Dernier block alloue (NULL si tous alloues)
	void* pLastSegmentBlock;         // Dernier block du segment
	struct _MemSegment* prevSegment; // Adresse du segment precedent
	struct _MemSegment* nextSegment; // Adresse du segment suivant
	size_t nSegmentTotalAllocSize;
	void* pData[1]; // Segment memoire
} MemSegment;

//////////////////////////////////////////////////////////////////////
//                 	    Declaration de Heap

// Taille memoire disponible dans la heap de base pour stocker les segments predefinis
// Petits segments pour les petits tailles, segments standard sinon
const size_t MemSmallPredefinedSegmentByteSize =
    MemSegmentByteSize /
    8; // La taille allouee sera ajustee de facon inferieure de facon a contenir un nombre exacte de blocs
const size_t MemMediumPredefinedSegmentByteSize =
    MemSegmentByteSize; // La taille allouee sera ajustee de facon inferieure de facon a contenir un nombre exacte de
			// blocs

// 		Definition de la structure d'une heap
// Les FixedSizeHeap sont dans un tableaux.
// Les tableau de stockage permettent d'alouer la structure d'une Heap
// en une seule fois, et de garantir ainsi que cette struture tient en
// un minimum de pages memoires.
// Chaque FixedSizeHeap est une liste chainee de segments dediee a une certaine taille d'allocation
// La liste chainee de MemSegmentArray stocke la liste des segments libres, regroupes
// en tableaux: chaque tableau permet d'avoir un grand nombre de segments definis en un seul bloc memoire,
// et la liste permet neanmoins d'agrandir si necessaire la liste des segments utilises.
// Les segments predefinis sont des segments de petites tailles, alloues une fois pour toutes et
// jamais liberes, pour optimiser la taille utilisee dans les programme faisant peu d'allocations
// Les donnees de segments predefinis pPredefinedData sont prevus avec une taille de 1, mais en fait alloue
// selon la taille necessaire (MemAllPredefinedSegmentsSize), cela revient a gerer une structure de taille variable
// determinee dynamiquement
// Le pointeur vers un segment libre reference 0 (NULL) ou un segment libre, pret a l'emploi. Cela permet d'eviter
// des phenomenes de boucle de creation/destruction de segment possible dans certains usages avec effet de bord,
// en gardant un buffer d'un segment en reserve
typedef struct _MemHeap
{
	size_t nHeapTotalAllocSize;   // Taille totale allouee pour la structure MemHeap en comptant le tableau des
				      // segments predefinis
	int nFreeSegmentNumber;       // Nombre de segments libre
	MemSegment* headFreeSegments; // Pointeur vers le debut de la list des segments libres
	MemSegment* fixedSizeHeapHeadSegments[MemFixedSizeHeapMaxNumber]; // Tableau des pointeurs sur les les premiers
									  // segments par type de taille d'allocation
	MemSegment* fixedSizeHeapTailSegments[MemFixedSizeHeapMaxNumber]; // Tableau des pointeurs sur les derniers
									  // segments par type de taille d'allocation
	MemSegment* predefinedSegments[MemFixedSizeHeapMaxNumber];        // Tableau de segments predefinis
	void* pPredefinedData[1];                                         // Donnees des segments predefinis
} MemHeap;

/////////////////////////////////////////////////////////////////////////////
//                     Declaration de MemStat

#ifdef MEMSTATSCOLLECT

static longint* memDetailedStats = NULL;
static longint memTotalStats = 0;
static longint memTotalSizeStats = 0;

void MemStatsInit()
{
	if (memDetailedStats == NULL)
	{
		MemHeapUpdateAlloc((MEMSTATSMAXSIZE + 1) * sizeof(longint));
		memDetailedStats = (longint*)p_hugemalloc((MEMSTATSMAXSIZE + 1) * sizeof(longint));
		for (int i = 0; i <= MEMSTATSMAXSIZE; i++)
			memDetailedStats[i] = 0;
		memTotalStats = 0;
		memTotalSizeStats = 0;
	}
}

inline void MemStatsUpdateAlloc(size_t nSize)
{
	MemStatsInit();
	if (nSize <= MEMSTATSMAXSIZE)
		memDetailedStats[nSize] = memDetailedStats[nSize] + 1;
	memTotalStats++;
	memTotalSizeStats += nSize;
}

void MemStatsDisplay()
{
	if (GetProcessId() != 0)
		printf("Process %d\n", GetProcessId());
	printf("Block size\tNumber\n");
	if (memDetailedStats != NULL)
	{
		for (int i = 0; i <= MEMSTATSMAXSIZE; i++)
		{
			if (memDetailedStats[i] > 0)
				printf("%d\t%lld\n", i, memDetailedStats[i]);
		}
	}
	printf("Total\t%lld\n", memTotalStats);
	printf("Total size\t%lld\n", memTotalSizeStats);
}

#endif // MEMSTATSCOLLECT

// statistique d'allocations
typedef struct _MemStat
{
	longint lTotalAlloc;         // nombre total d'allocations
	longint lTotalFree;          // nombre total de liberations
	longint lTotalRequestedSize; // taille totale demandee
	longint lTotalGrantedSize;   // taille totale alouee
	longint lTotalFreeSize;      // taille totale liberee
	longint lMaxAlloc;           // nombre maximale d'allocations
	longint lMaxGrantedSize;     // taille maximale alouee
} MemStat;

// Mise a jour des statistiques lors d'une allocation
void MemUpdateAlloc(MemStat* memStats, size_t nSize, void* pBlock);

// Mise a jour des statistiques lors d'une liberation
void MemUpdateFree(MemStat* memStats, void* pBlock);

// Declaration de la variable globale de gestion des statistiques d'allocation,
// pour le suivi exhaustif des stats d'allocation en mode debug
#ifndef NODEBUG
static MemStat memGlobalStats = {(size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0};
#endif // NODEBUG

// Declaration de la variable utilisateur de gestion des statistiques d'allocation
// puis le suivi utilisateur des stats d'allocation via une fonction MemStatsHandler, meme en mode release
static MemStat memUserHandlerStats = {(size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0, (size_t)0};

// Declaration de la variable du handler utilisateur de gestion des statistiques d'allocation
static MemStatsHandler memUserHanderFunction = 0;

// Flag indiquant que le handler est en cours d'appel
// Cela permettra une potentielle boucle infini qi le handler lui-meme fait des allocations
static boolean memUserHanderFunctionCalled = false;

// Declaration de la variable de frequence de rafraichissement des statistiques d'allocation
static longint memUserHanderCallFrequency = 0L;

// Correction de la taille d'un bloc en fonction du mode debug
inline size_t DebugMemBlockSize(void* pBlock)
{
#ifndef NOMEMCONTROL
	return MemBlockSize(pBlock) - sizeof(void*) * MemControlOverhead;
#else
	return MemBlockSize(pBlock);
#endif
}

inline void MemUpdateAlloc(MemStat* memStats, size_t nSize, void* pBlock)
{
	memStats->lTotalAlloc += 1;
	memStats->lTotalRequestedSize += nSize;
	memStats->lTotalGrantedSize += DebugMemBlockSize(pBlock);
	if (memStats->lTotalAlloc - memStats->lTotalFree > memStats->lMaxAlloc)
		memStats->lMaxAlloc = memStats->lTotalAlloc - memStats->lTotalFree;
	if (memStats->lTotalGrantedSize - memStats->lTotalFreeSize > memStats->lMaxGrantedSize)
		memStats->lMaxGrantedSize = memStats->lTotalGrantedSize - memStats->lTotalFreeSize;
}

inline void MemUpdateFree(MemStat* memStats, void* pBlock)
{
	memStats->lTotalFree += 1;
	memStats->lTotalFreeSize += DebugMemBlockSize(pBlock);
}

#ifndef NOMEMCONTROL

// Messages d'erreur
void MemAllocError(int bPrepare, void* pBlock, size_t nSize, const char* sMessage);
void MemAllocWarning(void* pBlock, const char* sMessage);
void MemFreeError(int bPrepare, void* pBlock, const char* sMessage);

// Preparation d'un block a allouer
// Affiche une erreur en cas de detection de memoire corrompue
int DebugMemCheckAndPrepareAllocBlock(void* pBlock, size_t nSize, int bPrepare);

// Allocation en mode controle memoire
void* DebugMemAlloc(size_t nSize);

// Preparation d'un block a liberer
// Affiche une erreur en cas de detection de memoire corrompue
int DebugMemCheckAndPrepareFreeBlock(void* pBlock, int bPrepare);

// Liberation en mode controle memoire
void DebugMemFree(void* pBlock);

#endif

//////////////////////////////////////////////////////////////////////
// 			     Gestions des segments

// Allocation d'un segment
MemSegment* SegNew();

// Initialisation d'un segment
void SegInit(MemSegment* self);

// Allocation et preparation d'un segment (->NULL si echec)
void SegPrepare(MemSegment* self, size_t nBlockSize, size_t nFixedSizeHeapIndex, size_t nSegmentTotalAllocSize,
		MemSegment* prevSegment, MemSegment* nextSegment);

// Liberation d'un segment
void SegFree(MemSegment* self);

// Verification de l'ensemble des blocs alloues (ou libres) d'un segment
// Renvoie le nombre de blocks alloues (ou libres)
int CheckAllocBlocks(MemSegment* self, void** pBlocks);
int CheckFreeBlocks(MemSegment* self);

// Modification du pointeur de segment attache a un block
// Ajout de void derriere inline pour portage UNIX
inline void MemBlockSetSegment(void* pBlock, MemSegment* pSeg)
{
	assert(pBlock != NULL);
	((MemSegment**)pBlock)[-1] = pSeg;
}

// Consultation du numero de segment attache a un block
inline MemSegment* MemBlockGetSegment(void* pBlock)
{
	assert(pBlock != NULL);
	return ((MemSegment**)pBlock)[-1];
}

// Modification de l'adresse intra-segment d'un trou
// Ajout de void derriere inline pour portage UNIX
inline void MemBlockSetNextBlock(void* pBlock, void* pNextBlock)
{
	assert(pBlock != NULL);
	((void**)pBlock)[0] = pNextBlock;
}

// Consultation de l'adresse intra-segment d'un trou
inline void* MemBlockGetNextBlock(void* pBlock)
{
	assert(pBlock != NULL);
	return ((void**)pBlock)[0];
}

// Acces a un offset (multiple de void*) dans un block
inline void* MemBlockGetAdressAt(void* pBlock, int nOffset)
{
	assert(pBlock != NULL);
	return &((void**)pBlock)[nOffset];
}

// Acces a une valeur dans un block
inline void* MemBlockGetValueAt(void* pBlock, int nOffset)
{
	assert(pBlock != NULL);
	return ((void**)pBlock)[nOffset];
}

// Modification a un offset (multiple de void*) dans un block
inline void MemBlockSetValueAt(void* pBlock, int nOffset, void* pValue)
{
	assert(pBlock != NULL);
	((void**)pBlock)[nOffset] = pValue;
}

// Acces a une valeur de taille dans un block
inline size_t MemBlockGetSizeValueAt(void* pBlock, int nOffset)
{
	assert(pBlock != NULL);
	return size_t(((void**)pBlock)[nOffset]);
}

// Modification d'une taille a un offset (multiple de void*) dans un block
inline void MemBlockSetSizeValueAt(void* pBlock, int nOffset, size_t nSize)
{
	assert(pBlock != NULL);
	((void**)pBlock)[nOffset] = (void*)nSize;
}

inline MemSegment* SegNew()
{
	// Allocation du segment
	MemHeapCurrentSegmentNumber++;
	MemHeapUpdateAlloc(MemSystemSegmentByteSize);
	return (MemSegment*)p_hugemalloc(MemSystemSegmentByteSize);
}

inline void SegInit(MemSegment* self)
{
	assert(self != NULL);

	self->bIsPredefined = false;
	self->nRealBlockSize = 0;
	self->nFixedSizeHeapIndex = 0;
	self->pFirstBlock = NULL;
	self->nNbBlock = 0;
	self->pLastAllocatedBlock = NULL;
	self->pLastSegmentBlock = NULL;
	self->prevSegment = NULL;
	self->nextSegment = NULL;
	self->nSegmentTotalAllocSize = 0;
}

void SegPrepare(MemSegment* self, size_t nBlockSize, size_t nFixedSizeHeapIndex, size_t nSegmentTotalAllocSize,
		MemSegment* prevSegment, MemSegment* nextSegment)
{
	void* pBlock;
	void** pData;
	size_t nDataSize;

	assert(self != NULL);
	assert(nBlockSize > 0);
	assert(0 <= nFixedSizeHeapIndex);
	assert(nFixedSizeHeapIndex < MemFixedSizeHeapMaxNumber);
	assert(nBlockSize <= MemMediumAllocMaxSize / sizeof(void*));
	assert(MemCheckBlockSize(nBlockSize, nFixedSizeHeapIndex));

	// Caracteristiques de la partie donnees du segment
	pData = self->pData;
	nDataSize = (nSegmentTotalAllocSize * sizeof(void*) - (sizeof(MemSegment) - sizeof(void*))) / sizeof(void*);

	// Remplissage du segment avec un pattern predefini
#ifndef NOMEMCONTROL
	for (size_t i = 0; i < nDataSize; i++)
		pData[i] = (void*)MemPattern;
#endif

	// Initialisation du segment
	self->nRealBlockSize = (int)nBlockSize + 1;
	self->nFixedSizeHeapIndex = nFixedSizeHeapIndex;
	self->nNbBlock = 0;
	self->pFirstBlock = MemBlockGetAdressAt(pData, 1);
	self->nSegmentTotalAllocSize = nSegmentTotalAllocSize;
	self->prevSegment = prevSegment;
	self->nextSegment = nextSegment;

	// Calcul de l'adresse du dernier block utilisable
	self->pLastSegmentBlock =
	    MemBlockGetAdressAt(pData, (int)(1 + self->nRealBlockSize * (nDataSize / self->nRealBlockSize - 1)));

	// Chainage du premier block du segment
	pBlock = self->pFirstBlock;
	MemBlockSetSegment(pBlock, self);
	MemBlockSetNextBlock(pBlock, NULL);
	if (pBlock == self->pLastSegmentBlock)
		self->pLastAllocatedBlock = NULL;
	else
		self->pLastAllocatedBlock = pBlock;
}

#ifndef NOMEMCONTROL

// La variable pCheckAllocBlocks doit etre alloue dans la heap (la stack pourrait exploser)
// et permettre de contenir le contenu complet d'un segment systeme
int CheckAllocBlocks(MemSegment* self, void** pCheckAllocBlocks)
{
	void* pFirstSegmentBlock;
	void* pBlock;
	void* pCheckBlock;
	int i;
	int j;

	assert(self != NULL);
	assert(pCheckAllocBlocks != NULL);

	// Dans un premier temps, on initialise le tableau avec
	// tous les blocs allouables
	pFirstSegmentBlock = MemBlockGetAdressAt(self->pData, 1);
	pBlock = pFirstSegmentBlock;
	i = 0;
	while (pBlock <= self->pLastSegmentBlock)
	{
		pCheckAllocBlocks[i] = pBlock;
		pBlock = MemBlockGetAdressAt(pBlock, self->nRealBlockSize);
		i++;
	}

	// On parcours ensuite les trous du segments pour mettre
	// les adresse des blocks correspondant a NULL
	pBlock = self->pFirstBlock;
	while (pBlock != NULL)
	{
		i = (int)(((uintptr_t)pBlock - (uintptr_t)pFirstSegmentBlock) / (sizeof(void*) * self->nRealBlockSize));
		pCheckAllocBlocks[i] = NULL;
		pBlock = MemBlockGetNextBlock(pBlock);
	}

	// Enfin, on compacte le resultat en supprimant les trous
	i = 0;
	j = 0;
	while (i < self->nNbBlock)
	{
		if (pCheckAllocBlocks[j] != NULL)
		{
			pCheckAllocBlocks[i] = pCheckAllocBlocks[j];
			i++;
		}
		j++;
	}

	// Verification des blocs
	for (j = 0; j < self->nNbBlock; j++)
	{
		pBlock = pCheckAllocBlocks[j];
		pCheckBlock = MemBlockGetAdressAt(pBlock, MemControlHeaderSize);

		// Message pour tout block alloue
		if (DebugMemCheckAndPrepareFreeBlock(pCheckBlock, 0))
			MemAllocWarning(pCheckBlock, "Block not free");
	}

	return self->nNbBlock;
}

int CheckFreeBlocks(MemSegment* self)
{
	void* pBlock;
	void* pCheckBlock;
	int i;

	assert(self != NULL);

	// On parcours les trous du segments pour mettre
	// les adresse des blocks correspondant
	pBlock = self->pFirstBlock;
	i = 0;
	while (pBlock != NULL)
	{
		// Verification du bloc
		pCheckBlock = MemBlockGetAdressAt(pBlock, MemControlHeaderSize);
		if (!DebugMemCheckAndPrepareAllocBlock(
			pCheckBlock, sizeof(void*) * (self->nRealBlockSize - 1 - MemControlOverhead), 0))
			GlobalExit();

		// Bloc suivant
		pBlock = MemBlockGetNextBlock(pBlock);
		i++;
	}
	return i;
}
#endif // NOMEMCONTROL

inline void SegFree(MemSegment* self)
{
	assert(self != NULL);
	assert(not self->bIsPredefined);

	MemDebug(self->nRealBlockSize = 0);
	MemDebug(self->pFirstBlock = NULL);
	MemDebug(self->nNbBlock = 0);
	MemDebug(self->pLastAllocatedBlock = NULL);
	MemDebug(self->pLastSegmentBlock = NULL);
	MemDebug(self->prevSegment = NULL);
	MemDebug(self->nextSegment = NULL);
	MemHeapCurrentSegmentNumber--;
	MemHeapUpdateFree(MemSystemSegmentByteSize);
	p_hugefree(self);
}

int SegIsCorrupted(MemSegment* self)
{
	int nCorrupted = 0;
	void* pBlock;
	MemSegment* psegCurrent;

	assert(self != NULL);

	// Verification des blocks alloues du segment
	pBlock = MemBlockGetAdressAt(self->pData, 1);
	while (pBlock <= self->pLastAllocatedBlock)
	{
		psegCurrent = MemBlockGetSegment(pBlock);
		if (psegCurrent != self)
		{
			if (GetProcessId() == 0)
				printf("Memory:\tBlock %p corrompu.\n", pBlock);
			else
				printf("Process %d: Memory:\tBlock %p corrompu.\n", GetProcessId(), pBlock);
			if (GetProcessId() != 0)
				nCorrupted = 1;
		}
		pBlock = MemBlockGetAdressAt(pBlock, self->nRealBlockSize);
	}
	return nCorrupted;
}

void SegPrintStats(MemSegment* self)
{
	assert(self != NULL);

	if (GetProcessId() == 0)
		printf("Segment (blocksize=%d): %d blocks dont %d aloues\n", (int)self->nRealBlockSize - 1,
		       (int)(MemSystemSegmentSize / self->nRealBlockSize), (int)self->nNbBlock);
	else
		printf("Process %d: Segment (blocksize=%d): %d blocks dont %d aloues\n", GetProcessId(),
		       (int)self->nRealBlockSize - 1, (int)(MemSystemSegmentSize / self->nRealBlockSize),
		       (int)self->nNbBlock);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
///       Implementation de la bibliotheque de gestion de la memoire

#ifndef NOMEMCONTROL

// Methodes de diagnostique des problemes memoire
void MemPrintStat(FILE* fOutput);
void MemCompleteCheck(FILE* fOutput);

// Diagnostique des problemes memoire appele apres la fin du main
void CompleteCheck()
{
	if (nStandardGlobalExit == 0)
	{
		MemCompleteCheck(stdout);
	}
}

#endif

// Variable globale, stockant la structure de la Heap
// On utilise un unnamed namespace pour forcer la localite dees utilisation et eviter els probleme potentiels de DLL
namespace
{
static MemHeap* pHeap = NULL;
static boolean bIsHeapInitialized = false;
} // namespace

// Fonction de gestion de la heap
void HeapInit();
void HeapClose();
int HeapCheckFixedSizeHeap(MemSegment* psegSearched);

// Initialisation de l'adresse du pointeur avec le meme pattern pour chaque octet
void MemInitPointer(void** pPointer, unsigned char cPattern)
{
	unsigned char* cTab = (unsigned char*)pPointer;
	size_t i;
	for (i = 0; i < sizeof(void*); i++)
		cTab[i] = cPattern;
}

// Intialisation des status utilises en debug
void MemPatternInit()
{
	MemInitPointer(&MemPattern, (unsigned char)0x55);
	MemInitPointer(&MemStatusInit, (unsigned char)0x55);
	MemInitPointer(&MemStatusAlloc, (unsigned char)0x5D);
	MemInitPointer(&MemStatusFree, (unsigned char)0x9D);
	MemInitPointer(&MemStatusMicrosoftNoMansland, (unsigned char)0xFD);
}

extern void SuppressCrashHandlers();

void HeapCleanExit()
{
	// On enpeche Windows d'ouvrir une boite de dialogue utilisateur
	SuppressCrashHandlers();
}

// Initialisation de la structure de heap
void HeapInit()
{
	if (not bIsHeapInitialized)
	{
		bIsHeapInitialized = true;

		// On profite de l'initialisation de la heap pour effectuer un parametrage pour l'ensemble de tous les
		// programmes On force ici un format d'exposant a deux digits sous Windows, pour etre compatible avec
		// Linux dans toutes les sorties, ce qui facilite la reproductibilite des tests (par defaut: exposant a
		// trois digits sous Windows, et a deux digits sous Linux) On force ici les locale, pour unifier les
		// conversion entre double et chaines de caracteres (point decimal)
#if defined _MSC_VER and _MSC_VER < 1400
		_set_output_format(_TWO_DIGIT_EXPONENT);
#endif

		// On profite de l'initialisation de la heap pour effectuer un parametrage sous linux
		// pour permettre l'ecriture de fichier dump en mode alpha
#if defined __UNIX__ and defined __ALPHA__
		// Pour forcer les crash dumps
		rlimit limit;
		limit.rlim_cur = RLIM_INFINITY;
		limit.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &limit);
#endif // defined __UNIX__ and defined __ALPHA__

		// On force un locale independant de la machine, pour assurer unicite des conversions numeriques et des
		// tris
		p_SetApplicationLocale();

		// Les traitements suivants ne sont effectues qu'avec le nouvel allocateur
#ifdef RELEASENEWMEM
		{
			size_t i;
			size_t nBlockSize;
			size_t nEmptySegmentSize;
			size_t nBlocNumber;
			size_t nPredefinedSegmentSize;
			size_t nPredefinedDataOffset;
			size_t nHeapTotalAllocSize;
			size_t nPrefedinedSegmentsBlocSizes[MemFixedSizeHeapMaxNumber];
			size_t nPrefedinedSegmentsAllocSizes[MemFixedSizeHeapMaxNumber];
			MemSegment* predefinedSegment;

			// Reservation de la memoire de traitement des erreurs fatales
			MemAllocFatalErrorReserveMemory();

			// Initialisation des status
			MemPatternInit();

			// Initialisation des tailles de blocks
			MemInitMediumAllSizes();

			// Verification des parametres de l'allocateur
			assert(MemCheckAllocParameters());

			// Nettoyage de la heap (inutile en mode release)
			debug(atexit(HeapClose));

			// Enregistrement d'une fonction de sortie rapide quand le programme est termine
			// Cette rustine est utilisee de facon preventive contre un bug java, qui
			// plante de temps en temps apres la fin du programme quand le screen saver se met en route
			// http://gloomyfish.wordpress.com/2009/05/29/how-to-fix-this-java-swing-application-error/
			// On ne l'appele qu'a la toute fin du programme, ce qui permet a Windows de planter si
			// necessaire
			atexit(HeapCleanExit);

			// Affichage de problemes memoire en fin du main, en mode debug
#ifndef NOMEMCONTROL
			atexit(CompleteCheck);
#endif

			// Affichage des statistiques detaillees d'allocation
#ifdef MEMSTATSCOLLECT
			MemStatsInit();
			atexit(MemStatsDisplay);
#endif
			// Taille d'un segment vide en unite void*
			nEmptySegmentSize = (sizeof(MemSegment) - 1) / sizeof(void*);

			// Initialisation des tailles des FixedSizeHeap et de la taille a allouer pour la heap
			nHeapTotalAllocSize = sizeof(MemHeap) - sizeof(void*);
			nBlockSize = 0;
			for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
			{
				// Calcul des parametres de taille du segment predefini
				if (nBlockSize < MemSmallAllocMaxSize / sizeof(void*))
				{
					nBlockSize += 1;
					nPrefedinedSegmentsBlocSizes[i] = nBlockSize;

					// Nombre de blocs
					nPredefinedSegmentSize = MemSmallPredefinedSegmentByteSize / sizeof(void*);
					nBlocNumber = nPredefinedSegmentSize / (nBlockSize + 1);

					// Correction vers le bas de la taille pour contenir un nombre de blocs de
					// taille relle (avec un pointeur en plus) exacte
					nPredefinedSegmentSize = nEmptySegmentSize + nBlocNumber * (nBlockSize + 1);
				}
				else
				{
					nBlockSize =
					    MemMediumAllSizes[i - MemSmallAllocMaxSize / sizeof(void*)] / sizeof(void*);
					nPrefedinedSegmentsBlocSizes[i] = nBlockSize;

					// Nombre de blocs
					nPredefinedSegmentSize = MemMediumPredefinedSegmentByteSize / sizeof(void*);
					nBlocNumber = nPredefinedSegmentSize / nBlockSize;
					if (nBlocNumber > 32)
						nBlocNumber = 32;

					// Correction vers le hautde la taille pour contenir un nombre de blocs de
					// taille relle (avec un pointeur en plus) exacte
					nPredefinedSegmentSize = nEmptySegmentSize + nBlocNumber * (nBlockSize + 1);
				}

				// Mise a jour des caracteristiques
				nPrefedinedSegmentsAllocSizes[i] = nPredefinedSegmentSize;
				nHeapTotalAllocSize += nPredefinedSegmentSize * sizeof(void*);
			}

			// Allocation de la structure de la heap
			assert(pHeap == NULL);
			MemHeapUpdateAlloc(nHeapTotalAllocSize);
			pHeap = (MemHeap*)p_hugemalloc(nHeapTotalAllocSize);
			if (pHeap == NULL)
			{
				MemFatalError("Memory overflow (heap allocation error)\n");

				// Arret
				return;
			}
			pHeap->nHeapTotalAllocSize = nHeapTotalAllocSize;

			// Initialisation a vide de la liste des segments vides
			pHeap->nFreeSegmentNumber = 0;
			pHeap->headFreeSegments = NULL;

			// Initialisation des segments predefinis
			nPredefinedDataOffset = 0;
			for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
			{
				// Initialisation du segment predefini
				nBlockSize = nPrefedinedSegmentsBlocSizes[i];
				nPredefinedSegmentSize = nPrefedinedSegmentsAllocSizes[i];
				predefinedSegment = (MemSegment*)&(pHeap->pPredefinedData[nPredefinedDataOffset]);
				SegInit(predefinedSegment);
				pHeap->predefinedSegments[i] = predefinedSegment;
				predefinedSegment->bIsPredefined = true;
				SegPrepare(predefinedSegment, nBlockSize, i, nPredefinedSegmentSize, NULL, NULL);
				nPredefinedDataOffset += nPredefinedSegmentSize;
				assert(nPredefinedDataOffset <= nHeapTotalAllocSize);

				// Chainage dans les fixed heap
				pHeap->fixedSizeHeapHeadSegments[i] = predefinedSegment;
				pHeap->fixedSizeHeapTailSegments[i] = predefinedSegment;
			}
			assert(sizeof(MemHeap) - sizeof(void*) + nPredefinedDataOffset * sizeof(void*) ==
			       nHeapTotalAllocSize);
		}
#endif // RELEASENEWMEM
	}
}

// Destruction de la heap
void HeapClose()
{
	MemSegment* psegCurrent;
	MemSegment* psegCurrentToDelete;
	size_t i;

	if (pHeap != NULL)
	{
		// Liberation si necessaire des segments libres
		psegCurrent = pHeap->headFreeSegments;
		while (psegCurrent != NULL)
		{
			assert(not psegCurrent->bIsPredefined);
			psegCurrentToDelete = psegCurrent;
			psegCurrent = psegCurrent->nextSegment;
			MemHeapUpdateFree(MemSystemSegmentByteSize);
			p_hugefree(psegCurrentToDelete);
		}
		pHeap->nFreeSegmentNumber = 0;

		// Liberation des segments par taille de heap
		for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
		{
			psegCurrent = pHeap->fixedSizeHeapHeadSegments[i];
			while (psegCurrent != NULL)
			{
				// Destruction du segment s'il n'est pas predefini
				psegCurrentToDelete = psegCurrent;
				psegCurrent = psegCurrent->nextSegment;
				if (not psegCurrentToDelete->bIsPredefined)
				{
					MemHeapUpdateFree(MemSystemSegmentByteSize);
					p_hugefree(psegCurrentToDelete);
				}
			}

			// On remet les segements predefinis pour avoir une structure coherente
			pHeap->fixedSizeHeapHeadSegments[i] = pHeap->predefinedSegments[i];
			pHeap->fixedSizeHeapHeadSegments[i] = pHeap->predefinedSegments[i];
		}

		// Liberation de la heap
		p_hugefree(pHeap);
		pHeap = NULL;
	}
}

// Visual C++: supression des Warning
#ifdef _MSC_VER
#pragma warning(disable : 6385) // disable C6385 warning (pour un controle excessif sur pHeap->fixedSizeHeapHeadSegments
				// dans HeapCheckFixedSizeHeapLecture)
#endif                          // _MSC_VER

// Verification d'un segment d'un FixedSizeHeap
int HeapCheckFixedSizeHeap(MemSegment* psegSearched)
{
	int nResult = 1;
	size_t nFixedSizeHeapIndex;
	MemSegment* psegHead;
	MemSegment* psegTail;
	MemSegment* psegCurrent;
	int nSearchedNumber;
	int nPredefinedNumber;
	int nFullSegment;

	assert(pHeap != NULL);
	assert(psegSearched != NULL);

	// Verification de l'index
	nFixedSizeHeapIndex = psegSearched->nFixedSizeHeapIndex;
	assert(0 <= nFixedSizeHeapIndex and nFixedSizeHeapIndex < MemFixedSizeHeapMaxNumber);

	// Acces aux extremites de liste des segments
	psegHead = pHeap->fixedSizeHeapHeadSegments[nFixedSizeHeapIndex];
	psegTail = pHeap->fixedSizeHeapTailSegments[nFixedSizeHeapIndex];
	assert(psegHead != NULL);
	assert(psegHead->nFixedSizeHeapIndex == nFixedSizeHeapIndex);
	assert(psegTail != NULL);
	assert(psegTail->nFixedSizeHeapIndex == nFixedSizeHeapIndex);

	// Parcours des segments de la heap
	nSearchedNumber = 0;
	nPredefinedNumber = 0;
	psegCurrent = psegHead;
	nFullSegment = 0;
	while (psegCurrent != NULL)
	{
		// Verification du chainage
		assert(psegCurrent->prevSegment == NULL or psegCurrent->prevSegment->nextSegment == psegCurrent);
		assert(psegCurrent->nextSegment == NULL or psegCurrent->nextSegment->prevSegment == psegCurrent);
		assert(psegCurrent->prevSegment != NULL or psegCurrent == psegHead);
		assert(psegCurrent->nextSegment != NULL or psegCurrent == psegTail);

		// Detection des segments speciaux
		if (psegCurrent == psegSearched)
			nSearchedNumber++;
		if (psegCurrent->bIsPredefined)
			nPredefinedNumber++;
		assert(not psegCurrent->bIsPredefined or psegCurrent == pHeap->predefinedSegments[nFixedSizeHeapIndex]);

		// Test si les segments plein sont consecutive et en fin de liste
		if (psegCurrent->pFirstBlock == NULL)
			nFullSegment = 1;
		assert(psegCurrent->pFirstBlock != NULL or nFullSegment == 1);

		// Segment suivant
		psegCurrent = psegCurrent->nextSegment;
	}
	assert(nSearchedNumber == 1);
	assert(nPredefinedNumber == 1);
	return nResult;
}

//////////////////////////////////////////////////////////////////
// Fonctions principales de gestion memoire

inline void* MemAlloc(size_t nSize)
{
	void* pBlock;

	assert(nSize > 0);
#ifndef NOMEMCONTROL
	assert(nSize > MemControlOverhead * sizeof(void*));
#endif

	// Allocation quand la heap n'est pas initialisee, ce qui peut arriver pour
	// des constructeurs statiques de la libraire standard (stream...)
	if (pHeap == NULL)
		HeapInit();

	// Allocation subclassee
	if (nSize <= MemMediumAllocMaxSize)
	{
		MemSegment* psegHeadCurrent;
		MemSegment* psegTailCurrent;
		MemSegment* psegCurrent;
		MemSegment* psegNew;
		size_t nBlockSize;
		size_t nFixedSizeHeapIndex;

		// Determination de la FixedSizeHeap responsable de l'allocation
		// Cas des petites tailles
		if (nSize <= MemSmallAllocMaxSize)
		{
			nFixedSizeHeapIndex = (int)(nSize - 1) / sizeof(void*);
			nBlockSize = nFixedSizeHeapIndex + 1;
		}
		// Cas des moyennes tailles
		else
		{
			size_t nRequestedSize;
			size_t nIndex;

			// Recherche de l'index de la heap responsable de l'allocation
			nFixedSizeHeapIndex = MemGetMediumFixedSizeHeapIndex(nSize);
			nIndex = nFixedSizeHeapIndex - MemSmallAllocMaxSize / sizeof(void*);
			nRequestedSize = MemMediumAllSizes[nIndex];
			nBlockSize = nRequestedSize / sizeof(void*);
			assert(nRequestedSize <= MemMediumAllocMaxSize);
			assert(nSize <= nRequestedSize);
			assert(nIndex == 0 or nSize > MemMediumAllSizes[nIndex - 1]);
		}
		assert(nFixedSizeHeapIndex < MemFixedSizeHeapMaxNumber);
		assert(MemCheckBlockSize(nBlockSize, nFixedSizeHeapIndex));
		psegHeadCurrent = pHeap->fixedSizeHeapHeadSegments[nFixedSizeHeapIndex];

		// Determination du segment responsable de l'allocation
		psegCurrent = psegHeadCurrent;
		assert(psegCurrent != NULL);

		// Creation si necessaire d'un nouveau segment
		if (psegCurrent->pFirstBlock == NULL)
		{
			// On recupere un segment libre si possible
			if (pHeap->headFreeSegments != NULL)
			{
				psegNew = pHeap->headFreeSegments;
				pHeap->headFreeSegments = psegNew->nextSegment;
				pHeap->nFreeSegmentNumber--;
				assert(pHeap->headFreeSegments == NULL or pHeap->nFreeSegmentNumber > 0);
			}
			// Sinon, on en alloue un nouveau
			else
			{
				// Tentative de creation d'un nouveau segment
				psegNew = SegNew();

				// Erreur: plus de segment
				if (psegNew == NULL)
				{
					MemFatalError("Memory overflow (unable to allocate new segment in heap)\n");
					return NULL;
				}
			}

			// Preparation du segment
			SegInit(psegNew);
			SegPrepare(psegNew, nBlockSize, nFixedSizeHeapIndex, MemSystemSegmentSize, NULL, psegCurrent);
			psegCurrent->prevSegment = psegNew;
			psegCurrent = psegNew;

			// Ajout en tete de la heap
			assert(psegCurrent->nextSegment == psegHeadCurrent);
			psegHeadCurrent = psegCurrent;
			pHeap->fixedSizeHeapHeadSegments[nFixedSizeHeapIndex] = psegHeadCurrent;
			assert(HeapCheckFixedSizeHeap(psegCurrent));
		}
		assert(psegCurrent != NULL);
		assert(psegCurrent->pFirstBlock != NULL);
		assert(psegCurrent->prevSegment == NULL);

		// Allocation dans le segment courant
		pBlock = psegCurrent->pFirstBlock;
		psegCurrent->pFirstBlock = MemBlockGetNextBlock(pBlock);

		// Maintien de la liste chainee de la FixedSizeHeap si segment plein
		if (psegCurrent->pFirstBlock == NULL)
		{
			void* pNewBlock;

			// On regarde si on peut encore allouer des blocks dans le segment
			if (psegCurrent->pLastAllocatedBlock != NULL)
			{
				// Chainage du premier block du segment
				pNewBlock =
				    MemBlockGetAdressAt(psegCurrent->pLastAllocatedBlock, psegCurrent->nRealBlockSize);
				MemBlockSetSegment(pNewBlock, psegCurrent);
				MemBlockSetNextBlock(pNewBlock, NULL);
				if (pNewBlock == psegCurrent->pLastSegmentBlock)
					psegCurrent->pLastAllocatedBlock = NULL;
				else
					psegCurrent->pLastAllocatedBlock = pNewBlock;
				psegCurrent->pFirstBlock = pNewBlock;
			}
			// Sinon, on met le segment plein en fin de heap
			else
			{
				assert(psegHeadCurrent == psegCurrent);

				// A faire uniquement si le segment n'est pas deja en queue de liste pour avoir en tete
				// de liste les segment ou on peut allouer
				psegTailCurrent = pHeap->fixedSizeHeapTailSegments[nFixedSizeHeapIndex];
				if (psegTailCurrent != psegCurrent)
				{
					// On supprime de la tete de liste
					psegHeadCurrent = psegCurrent->nextSegment;
					pHeap->fixedSizeHeapHeadSegments[nFixedSizeHeapIndex] = psegHeadCurrent;
					assert(psegHeadCurrent != NULL);
					psegHeadCurrent->prevSegment = NULL;

					// On rajoute en queue de liste
					psegCurrent->nextSegment = NULL;
					psegCurrent->prevSegment = psegTailCurrent;
					psegCurrent->prevSegment->nextSegment = psegCurrent;
					psegTailCurrent = psegCurrent;
					pHeap->fixedSizeHeapTailSegments[nFixedSizeHeapIndex] = psegTailCurrent;
				}
				assert(HeapCheckFixedSizeHeap(psegCurrent));
			}
		}
		psegCurrent->nNbBlock++;

		assert(MemBlockGetSegment(pBlock) == psegCurrent);
	}
	else // allocation classique
	{
		// On alloue la taille demandee plus de la place pour des information supplementaires
		//   position -2: nombre de segments
		//   position -1: NULL, pour indiquer qu'il s'agit d'une allocation systeme
		//   position 0: les donnees utiles
		size_t nTrueSize = nSize + 2 * sizeof(void*);
		size_t nRequestedSize;

		assert(nSize > MemMediumAllocMaxSize);

		// Gestion des tailles superieures a celle d'un segment
		nRequestedSize = ((nTrueSize + MemSegmentByteSize - 1) / MemSegmentByteSize) * MemSegmentByteSize;
		assert(nRequestedSize >= nTrueSize);

		// Allocation classique
		MemHeapUpdateAlloc(nRequestedSize);
		pBlock = (void*)p_hugemalloc(nRequestedSize);
		if (pBlock == NULL)
		{
			if (MemGetAllocErrorHandler() != NULL)
			{
				char sMessage[100];
				sprintf(sMessage, "Memory overflow (malloc allocation error (%lld))\n", (longint)nSize);
				MemFatalError(sMessage);
			}
			return NULL;
		}
		else
		{
			// On memorise la taille allouee au debut du block
			MemBlockSetSizeValueAt(pBlock, 0, nRequestedSize);

			// On renvoie le block alloue a partir de la memoire utile
			pBlock = MemBlockGetAdressAt(pBlock, 2);

			// On marque le block par NULL, ce qui permettra d'invoquer la desallocation classique
			MemBlockSetSegment(pBlock, NULL);
		}
	}

	// Mise a jour des statistiques utilisateurs
	if (memUserHanderCallFrequency > 0)
	{
		// On corrige la taille demande selon le mode
#ifndef NOMEMCONTROL
		MemUpdateAlloc(&memUserHandlerStats, nSize - MemControlOverhead * sizeof(void*), pBlock);
#else
		MemUpdateAlloc(&memUserHandlerStats, nSize, pBlock);
#endif

		// Appel du handler selon la frequence demandee
		if (memUserHanderFunction != NULL and
		    (memUserHandlerStats.lTotalAlloc + memUserHandlerStats.lTotalFree) % memUserHanderCallFrequency ==
			0)
		{
			if (not memUserHanderFunctionCalled)
			{
				memUserHanderFunctionCalled = true;
				memUserHanderFunction();
				memUserHanderFunctionCalled = false;
			}
		}
	}
	return (pBlock);
}

inline void MemFree(void* pBlock)
{
	MemSegment* psegCurrent;
	MemSegment* psegHeadCurrent;
	MemSegment* psegTailCurrent;

	assert(pHeap != NULL);
	assert(pBlock != NULL);

	// Mise a jour des statistiques utilisateurs
	if (memUserHanderCallFrequency > 0)
	{
		MemUpdateFree(&memUserHandlerStats, pBlock);

		// Appel du handler selon la frequence demandee
		if (memUserHanderFunction != NULL and
		    (memUserHandlerStats.lTotalAlloc + memUserHandlerStats.lTotalFree) % memUserHanderCallFrequency ==
			0)
		{
			if (not memUserHanderFunctionCalled)
			{
				memUserHanderFunctionCalled = true;
				memUserHanderFunction();
				memUserHanderFunctionCalled = false;
			}
		}
	}

	psegCurrent = MemBlockGetSegment(pBlock);
#ifdef _MSC_VER
	assert(psegCurrent != MemStatusMicrosoftNoMansland);
#endif                           //_MSC_VER
	if (psegCurrent != NULL) // liberation subclassee
	{
		assert(psegCurrent->nNbBlock >= 1);

		// Rajout en tete de la liste chainee de la FixedSizeHeap si segment etait plein
		if (psegCurrent->pFirstBlock == NULL)
		{
			// Determination de la FixedSizeHeap responsable de l'allocation
			psegHeadCurrent = pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex];
			psegTailCurrent = pHeap->fixedSizeHeapTailSegments[psegCurrent->nFixedSizeHeapIndex];

			// Deplacement a faire uniquement si le segment n'est pas deja en tete de liste
			assert(psegHeadCurrent != psegTailCurrent or psegCurrent == psegTailCurrent);
			assert(psegHeadCurrent != psegTailCurrent or psegCurrent->bIsPredefined);
			if (psegCurrent != psegHeadCurrent)
			{
				// Supression du segment de sa position courante
				if (psegCurrent->nextSegment == NULL)
				{
					assert(psegTailCurrent == psegCurrent);
					psegTailCurrent = psegCurrent->prevSegment;
					pHeap->fixedSizeHeapTailSegments[psegCurrent->nFixedSizeHeapIndex] =
					    psegTailCurrent;
					assert(psegTailCurrent != NULL);
				}
				else
					psegCurrent->nextSegment->prevSegment = psegCurrent->prevSegment;
				if (psegCurrent->prevSegment == NULL)
				{
					assert(psegHeadCurrent == psegCurrent);
					psegHeadCurrent = psegCurrent->nextSegment;
					pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex] =
					    psegHeadCurrent;
					assert(psegHeadCurrent != NULL);
				}
				else
					psegCurrent->prevSegment->nextSegment = psegCurrent->nextSegment;

				// Rajout en tete de liste
				psegCurrent->nextSegment = psegHeadCurrent;
				psegCurrent->nextSegment->prevSegment = psegCurrent;
				psegCurrent->prevSegment = NULL;
				psegHeadCurrent = psegCurrent;
				pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex] = psegHeadCurrent;
			}
			assert(HeapCheckFixedSizeHeap(psegCurrent));
		}

		// liberation du block dans le segment
		assert(psegCurrent->pFirstBlock == NULL || MemBlockGetSegment(psegCurrent->pFirstBlock) == psegCurrent);
		MemBlockSetNextBlock(pBlock, psegCurrent->pFirstBlock);
		psegCurrent->pFirstBlock = pBlock;
		assert(psegCurrent->pFirstBlock != NULL);
		psegCurrent->nNbBlock--;

		// Traitement special si le segment devient vide et s'il n'est pas predefini
		if (psegCurrent->nNbBlock == 0 and not psegCurrent->bIsPredefined)
		{
#ifndef NOMEMCONTROL
			// Verification des blocks desaloues
			CheckFreeBlocks(psegCurrent);
#endif
			// Determination de la FixedSizeHeap responsable de l'allocation
			psegHeadCurrent = pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex];
			psegTailCurrent = pHeap->fixedSizeHeapTailSegments[psegCurrent->nFixedSizeHeapIndex];

			// Supression du segment de sa position courante
			if (psegCurrent->nextSegment == NULL)
			{
				assert(psegTailCurrent == psegCurrent);
				psegTailCurrent = psegCurrent->prevSegment;
				pHeap->fixedSizeHeapTailSegments[psegCurrent->nFixedSizeHeapIndex] = psegTailCurrent;
			}
			else
				psegCurrent->nextSegment->prevSegment = psegCurrent->prevSegment;
			if (psegCurrent->prevSegment == NULL)
			{
				assert(psegHeadCurrent == psegCurrent);
				psegHeadCurrent = psegCurrent->nextSegment;
				pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex] = psegHeadCurrent;
			}
			else
				psegCurrent->prevSegment->nextSegment = psegCurrent->nextSegment;

			// On memorise le segment comme segment libre si possible
			// On autorise un nombre max de segments libre et une proportion max de segments libres
			assert(HeapCheckFixedSizeHeap(psegHeadCurrent));
			assert(not psegCurrent->bIsPredefined);
			if (pHeap->nFreeSegmentNumber < 16 and
			    pHeap->nFreeSegmentNumber <= MemHeapCurrentSegmentNumber / 8)
			{
				// Ajout d'un segment dans la liste des segments libre
				psegCurrent->nextSegment = pHeap->headFreeSegments;
				pHeap->headFreeSegments = psegCurrent;
				pHeap->nFreeSegmentNumber++;
			}
			// Sinon, on le libere
			else
			{
				SegFree(psegCurrent);

				// On supprime egalement un segment libre s'il y en trop, pour garder toujours une
				// proprotion max de segments libre Le dernier segment libre ne sera peut-etre jamais
				// libere, mais ce n'est pas grave
				if (pHeap->nFreeSegmentNumber > 1 + MemHeapCurrentSegmentNumber / 8)
				{
					// On recupere le segment en te de liste des segments libres
					psegCurrent = pHeap->headFreeSegments;
					pHeap->headFreeSegments = psegCurrent->nextSegment;
					pHeap->nFreeSegmentNumber--;
					assert(pHeap->headFreeSegments == NULL or pHeap->nFreeSegmentNumber > 0);

					// On le detruit
					SegFree(psegCurrent);
				}
			}
		}
	}
	else // liberation standard
	{
		size_t nRequestedSize;

		// On recupere la taille allouee en position -2
		nRequestedSize = MemBlockGetSizeValueAt(pBlock, -2);

		// L'adresse d'allocation est en position 2 avant l'adresse utile
		MemHeapUpdateFree(nRequestedSize);
		p_hugefree(MemBlockGetAdressAt(pBlock, -2));
	}
}

size_t MemBlockSize(void* pBlock)
{
	MemSegment* psegCurrent;
	size_t nRequestedSize;

	assert(pHeap != NULL);

	psegCurrent = MemBlockGetSegment(pBlock);
	if (psegCurrent != NULL) // allocateur subclasse
		return sizeof(void*) * (psegCurrent->nRealBlockSize - 1);
	else // Allocation systeme
	{
		// On recupere la taille allouee en position -2
		nRequestedSize = MemBlockGetSizeValueAt(pBlock, -2);
		return nRequestedSize;
	}
}

/////////////////////////////////////////////////////////////////////////////
//                       Implementation de MemStat                         //
/////////////////////////////////////////////////////////////////////////////

#ifndef NOMEMCONTROL

void MemPrintStat(FILE* fOutput)
{
	if (GetProcessId() != 0)
		fprintf(fOutput, "Process %d: ", GetProcessId());
	fprintf(fOutput, "Memory stats (number of pointers, and memory space)\n");
	fprintf(fOutput, "  Alloc: %lld  Free: %lld  MaxAlloc: %lld\n", memGlobalStats.lTotalAlloc,
		memGlobalStats.lTotalFree, memGlobalStats.lMaxAlloc);
	fprintf(fOutput, "  Requested: %lld  Granted: %lld  Free: %lld  MaxGranted: %lld\n",
		memGlobalStats.lTotalRequestedSize, memGlobalStats.lTotalGrantedSize, memGlobalStats.lTotalFreeSize,
		memGlobalStats.lMaxGrantedSize);
	fflush(fOutput);
}

void MemCompleteCheck(FILE* fOutput)
{
	MemSegment* psegCurrent;
	int i;
	int nMinPredefinedBlockSize;
	void** pCheckAllocBlocks;

	// Parcours des segments non libres de la heap
	if (pHeap != NULL)
	{
		// Allocation de la variable permettant de controler les segments
		pCheckAllocBlocks = (void**)p_hugemalloc(MemSystemSegmentSize * sizeof(void*));

		// Taille minimum des blocs permettant un controle de pattern dans les blocs
		nMinPredefinedBlockSize = MemControlOverhead;

		// Verification des segments de la heap
		for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
		{
			psegCurrent = pHeap->fixedSizeHeapHeadSegments[i];
			while (psegCurrent != NULL)
			{
				if (!SegIsCorrupted(psegCurrent) and
				    psegCurrent->nRealBlockSize > nMinPredefinedBlockSize)
				{
					// Verification des blocks desaloues
					CheckFreeBlocks(psegCurrent);

					// Verification des blocks aloues
					CheckAllocBlocks(psegCurrent, pCheckAllocBlocks);
				}
				psegCurrent = psegCurrent->nextSegment;
			}
		}

		// Nettoyage
		p_hugefree(pCheckAllocBlocks);
	}

	// Statistiques globales
	fprintf(fOutput, "\n");
	MemPrintStat(fOutput);

	// Indication des erreurs
	if (memGlobalStats.lTotalAlloc > memGlobalStats.lTotalFree)
	{
		fprintf(fOutput, "\n");
		if (GetProcessId() != 0)
			fprintf(stdout, "Process %d: ", GetProcessId());
		fprintf(fOutput, "NewMem Warning: Block not free: %lld",
			memGlobalStats.lTotalAlloc - memGlobalStats.lTotalFree);
		fprintf(fOutput, "\tMemory not free: %lld",
			memGlobalStats.lTotalGrantedSize - memGlobalStats.lTotalFreeSize);
		fprintf(fOutput, "\n");
	}

	fflush(fOutput);
}

//////////////////////////////////////////////////////////////////
// Gestion des message d'erreur memoire

static const int nMemMaxMessageNumber = 20;
static int nMemMessageNumber = 0;

void MemAllocError(int bPrepare, void* pBlock, size_t nSize, const char* sMessage)
{
	size_t nBlockId;

	if (nMemMessageNumber < nMemMaxMessageNumber)
	{
		// Recherche de l'identifiant du bloc (son numero d'allocation)
		nBlockId = (size_t)MemBlockGetValueAt(pBlock, MemOffsetAllocNumber);
		if (GetProcessId() != 0)
			fprintf(stdout, "Process %d: ", GetProcessId());
		if (bPrepare)
			fprintf(stdout, "Memory alloc error id=%lld %p (%lld): %s\n", (longint)nBlockId, pBlock,
				(longint)nSize, sMessage);
		else
			fprintf(stdout, "Memory free error id=%lld %p (%lld): %s\n", (longint)nBlockId, pBlock,
				(longint)nSize, sMessage);
	}
	if (nMemMessageNumber == nMemMaxMessageNumber)
		fprintf(stdout, "...\n");
	nMemMessageNumber++;
}

void MemAllocWarning(void* pBlock, const char* sMessage)
{
	void* pAllocBlock;
	MemSegment* psegCurrent;
	size_t nBlockId;

	if (nMemMessageNumber < nMemMaxMessageNumber)
	{
		// Acces au segment
		pAllocBlock = MemBlockGetAdressAt(pBlock, -MemControlHeaderSize);
		psegCurrent = MemBlockGetSegment(pAllocBlock);
		assert(psegCurrent != NULL);

		// Recherche de l'identifiant du bloc (son numero d'allocation)
		nBlockId = (size_t)MemBlockGetValueAt(pBlock, MemOffsetAllocNumber);
		if (GetProcessId() != 0)
			fprintf(stdout, "Process %d: ", GetProcessId());
		fprintf(stdout, "NewMem alloc block warning id=%lld %p (%lld): %s\n", (longint)nBlockId, pBlock,
			(longint)sizeof(void*) * (psegCurrent->nRealBlockSize - 1 - MemControlOverhead), sMessage);
	}
	if (nMemMessageNumber == nMemMaxMessageNumber)
		fprintf(stdout, "...\n");
	nMemMessageNumber++;
}

void MemFreeError(int bPrepare, void* pBlock, const char* sMessage)
{
	size_t nBlockId;

	if (nMemMessageNumber < nMemMaxMessageNumber)
	{
		// Recherche de l'identifiant du bloc (son numero d'allocation)
		nBlockId = (size_t)MemBlockGetValueAt(pBlock, MemOffsetAllocNumber);
		if (GetProcessId() != 0)
			fprintf(stdout, "Process %d: ", GetProcessId());
		if (bPrepare)
			fprintf(stdout, "NewMem free error id=%lld %p: %s\n", (longint)nBlockId, pBlock, sMessage);
		else
			fprintf(stdout, "NewMem alloc block error id=%lld %p: %s\n", (longint)nBlockId, pBlock,
				sMessage);
	}
	if (nMemMessageNumber == nMemMaxMessageNumber)
		fprintf(stdout, "...\n");
	nMemMessageNumber++;
}

////////////////////////////////////////////////////////////////////////
// Gestion de la memoire en mode controle

int DebugMemCheckAndPrepareAllocBlock(void* pBlock, size_t nSize, int bPrepare)
{
	int bOk = 1;
	void* pAllocBlock;
	MemSegment* psegCurrent;
	MemSegment* psegSegment;
	int nPatternFillSize;
	int i;

	// Acces au segment
	pAllocBlock = MemBlockGetAdressAt(pBlock, -MemControlHeaderSize);
	psegCurrent = MemBlockGetSegment(pAllocBlock);

	// Verification de l'adresse du segment
	if (psegCurrent != NULL)
	{
		// Cas d'un segment predefini
		if (psegCurrent->bIsPredefined)
		{
			// Ce segment doit etre celui declare dans la Heap
			if (psegCurrent != pHeap->predefinedSegments[psegCurrent->nFixedSizeHeapIndex])
				return 0;
		}
		// Cas d'un segment standard
		else
		{
			// Recherche du segment dans sa heap dediee
			bOk = 0;
			psegSegment = pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex];
			while (psegSegment != NULL)
			{
				if (psegCurrent == psegSegment)
				{
					bOk = 1;
					break;
				}
				psegSegment = psegSegment->nextSegment;
			}

			// Erreur si aucun tableau de segment ne contient le segment
			if (bOk == 0)
			{
				MemAllocError(bPrepare, pBlock, nSize, "Block unknown");
				return bOk;
			}
		}
	}

	// Si allocation systeme: on initialise le block dans un etat coherent
	if (psegCurrent == NULL)
	{
		// On affecte le pattern de debut uniquement
		if (bPrepare)
			MemBlockSetValueAt(pBlock, MemOffsetPattern, (void*)MemPattern);
	}
	// Sinon, on verifie sa coherence
	else
	{
		// Test de l'etat du block
		if (MemBlockGetValueAt(pBlock, MemOffsetStatus) != MemStatusInit &&
		    MemBlockGetValueAt(pBlock, MemOffsetStatus) != MemStatusFree)
		{
			MemAllocError(bPrepare, pBlock, nSize, "Block not free");
			bOk = 0;
		}

		// Test des patterns
		if (MemBlockGetValueAt(pBlock, MemOffsetPattern) != MemPattern)
		{
			MemAllocError(bPrepare, pBlock, nSize, "Block corrupted in header");
			bOk = 0;
		}
		if (MemBlockGetValueAt(pBlock, (int)(psegCurrent->nRealBlockSize - MemControlOverhead - 1)) !=
		    MemPattern)
		{
			MemAllocError(bPrepare, pBlock, nSize, "Block corrupted in trailer");
			bOk = 0;
		}

		// Test de remplissage par des pattern
		nPatternFillSize = psegCurrent->nRealBlockSize - MemControlOverhead - 1;
		for (i = 0; i < nPatternFillSize; i++)
		{
			if (MemBlockGetValueAt(pBlock, i) != MemPattern)
			{
				MemAllocError(bPrepare, pBlock, nSize, "Block corrupted");
				bOk = 0;
				break;
			}
		}
	}

	// Initialisation du status
	if (bPrepare)
		MemBlockSetValueAt(pBlock, MemOffsetStatus, (void*)MemStatusAlloc);

	// Initialisation du numero d'allocation
	// On memorise le numero d'allocation dans un espace reserve a un pointeur
	if (bPrepare)
		MemBlockSetValueAt(pBlock, MemOffsetAllocNumber, (void*)memGlobalStats.lTotalAlloc);

	return bOk;
}

void* DebugMemAlloc(size_t nSize)
{
	void* pAllocBlock;
	void* pBlock;

	assert(nSize > 0);

	// Allocation effective
	pAllocBlock = MemAlloc(nSize + MemControlOverhead * sizeof(void*));

	// Test de reussite de l'allocation
	if (pAllocBlock == NULL)
	{
		if (MemGetAllocErrorHandler() != NULL)
		{
			char sMessage[100];
			sprintf(sMessage, "Memory overflow (unable to allocate memory (%lld))\n",
				(longint)(nSize + MemControlOverhead * sizeof(void*)));
			MemFatalError(sMessage);
		}
		return NULL;
	}

	// Mise a jour des statistiques globales
	MemUpdateAlloc(&memGlobalStats, nSize, pAllocBlock);

	// Acces a la partie donnee utilisateur du block
	pBlock = MemBlockGetAdressAt(pAllocBlock, MemControlHeaderSize);

	// Test de coherence du block
	if (!DebugMemCheckAndPrepareAllocBlock(pBlock, nSize, 1))
		GlobalExit();

	// Test d'arret au ieme alloc
	if (pMemAllocBlockExit == NULL and lMemAllocIndexExit != 0L and
	    memGlobalStats.lTotalAlloc == lMemAllocIndexExit)
	{
		MemAllocError(1, pBlock, nSize, "User exit (at block index)");
		GlobalExit();
	}

	// Test d'arret au bloc de taille donnee
	if (pMemAllocBlockExit == NULL and nSize == (size_t)nMemAllocSizeExit)
	{
		MemAllocError(1, pBlock, nSize, "User exit (at block size)");
		GlobalExit();
	}

	// Test d'arret au bloc pour une allocation system
	if (pMemAllocBlockExit == NULL and bMemSystemAllocExit and nSize > MemSegmentByteSize)
	{
		MemAllocError(1, pBlock, nSize, "User exit (at system block)");
		GlobalExit();
	}

	// Test d'arret au bloc donne
	if (pMemAllocBlockExit != NULL and pMemAllocBlockExit == pBlock and
	    memGlobalStats.lTotalAlloc >= lMemAllocIndexExit and nSize >= (size_t)nMemAllocSizeExit)
	{
		MemAllocError(1, pBlock, nSize, "User exit (at given block)");
		GlobalExit();
	}

	// On renvoie le debut de la partie donnee utilisateur du block
	return pBlock;
}

int DebugMemCheckAndPrepareFreeBlock(void* pBlock, int bPrepare)
{
	int bOk = 1;
	void* pAllocBlock;
	MemSegment* psegCurrent;
	MemSegment* psegSegment;
	int i;
	int nPatternFillSize;

	// Acces au segment
	pAllocBlock = MemBlockGetAdressAt(pBlock, -MemControlHeaderSize);
	psegCurrent = MemBlockGetSegment(pAllocBlock);

	// Verification de l'adresse du segment
	if (psegCurrent != NULL)
	{
		// Cas d'un segment predefini
		if (psegCurrent->bIsPredefined)
		{
			// Ce segment doit etre celui declare dans la Heap
			if (psegCurrent != pHeap->predefinedSegments[psegCurrent->nFixedSizeHeapIndex])
				return 0;
		}
		// Cas d'un segment standard
		else
		{
			// Recherche du segment dans sa heap dediee
			bOk = 0;
			psegSegment = pHeap->fixedSizeHeapHeadSegments[psegCurrent->nFixedSizeHeapIndex];
			while (psegSegment != NULL)
			{
				if (psegCurrent == psegSegment)
				{
					bOk = 1;
					break;
				}
				psegSegment = psegSegment->nextSegment;
			}

			// Erreur si aucun tableau de segment ne contient le segment
			if (bOk == 0)
			{
				MemFreeError(bPrepare, pBlock, "Block unknown");
				return bOk;
			}
		}
	}

	// Test de l'etat du block
	if (MemBlockGetValueAt(pBlock, MemOffsetStatus) != MemStatusAlloc)
	{
		if (MemBlockGetValueAt(pBlock, MemOffsetStatus) == MemStatusFree)
		{
			MemFreeError(bPrepare, pBlock, "Block allready free");
			bOk = 0;
		}
		else
		{
			MemFreeError(bPrepare, pBlock, "Block never alloc");
			bOk = 0;
		}
	}

	// Test des patterns
	if (MemBlockGetValueAt(pBlock, MemOffsetPattern) != MemPattern)
	{
		MemFreeError(bPrepare, pBlock, "Block corrupted in header");
		bOk = 0;
	}
	if (psegCurrent != NULL and
	    MemBlockGetValueAt(pBlock, psegCurrent->nRealBlockSize - MemControlOverhead - 1) != MemPattern)
	{
		MemFreeError(bPrepare, pBlock, "Block corrupted in trailer");
		bOk = 0;
	}

	// Remplissage par des pattern
	if (psegCurrent != NULL and bPrepare)
	{
		nPatternFillSize = psegCurrent->nRealBlockSize - MemControlOverhead - 1;
		for (i = 0; i < nPatternFillSize; i++)
		{
			MemBlockSetValueAt(pBlock, i, (void*)MemPattern);
		}
	}

	// Initialisation du status
	if (bPrepare)
		MemBlockSetValueAt(pBlock, MemOffsetStatus, (void*)MemStatusFree);

	// On laisse le numero d'allocation

	return bOk;
}

void DebugMemFree(void* pBlock)
{
	assert(pBlock != NULL);

	// Pour eviter une boucle dans les erreur
	// gerees dans le exit normal
	if (nStandardGlobalExit == 0)
	{
		// Cas particulier Microsoft: en mode DEBUG
		// l'allocateur est redefini, et on capture des
		// delete sans avoir eu les new correspondant
#ifdef _MSC_VER
		if (MemBlockGetValueAt(pBlock, -1) == MemStatusMicrosoftNoMansland)
			p_hugefree(pBlock);
		else
#endif //_MSC_VER
		{
			void* pAllocBlock;

			// Preparation du bloc a desallouer
			if (!DebugMemCheckAndPrepareFreeBlock(pBlock, 1))
				GlobalExit();

			// On recherche le block prealablement alloue
			pAllocBlock = MemBlockGetAdressAt(pBlock, -MemControlHeaderSize);

			// Mise a jour des statistiques globales
			MemUpdateFree(&memGlobalStats, pAllocBlock);

			// Liberation effective
			MemFree(pAllocBlock);
		}
	}
}

#endif // NOMEMCONTROL

///////////////////////////////////////////////////////////////
// Methode de controle des allocation

void MemSetAllocIndexExit(longint lAllocIndex)
{
	lMemAllocIndexExit = lAllocIndex;
}

longint MemGetAllocIndexExit()
{
	return lMemAllocIndexExit;
}

void MemSetAllocSizeExit(int nAllocSize)
{
	nMemAllocSizeExit = nAllocSize;
}

int MemGetAllocSizeExit()
{
	return nMemAllocSizeExit;
}

void MemSetAllocBlockExit(void* pAllocBlock)
{
	pMemAllocBlockExit = pAllocBlock;
}

void* MemGetAllocBlockExit()
{
	return pMemAllocBlockExit;
}

void MemSetMaxHeapSize(longint lSize)
{
	require(lSize >= 0);
	lMemMaxHeapSize = lSize;
}

longint MemGetMaxHeapSize()
{
	return lMemMaxHeapSize;
}

////////////////////////////////////////////////////////////////////////
// Gestion des erreurs d'allocation

void MemSetAllocErrorHandler(MemAllocErrorHandler fMemAllocError)
{
	fMemAllocErrorHandler = fMemAllocError;
}

MemAllocErrorHandler MemGetAllocErrorHandler()
{
	return fMemAllocErrorHandler;
}

MemAllocErrorHandler MemGetDefaultAllocErrorHandler()
{
	return fMemDefaultAllocErrorHandler;
}

//////////////////////////////////////////////////////////////////////////////////////
// Utilisation du "nifty ccounter trick" pour forcer l'initialisatiopn de la heap
// avant tout constructeur d'un objet statique (hros librairies standard: sdtream...)
// En declarant un objet statique PrivateHeapInitializer dans chaque header, on
// force l'appel au constructeur de la heap (appele une seule fois) a etre appele
// avant toute construction d'objet statique

// On initialise la heap une seule fois
PrivateHeapInitializer::PrivateHeapInitializer()
{
	// On l'appelle systematiquement la fonction meme en cas dr'allocation standard,
	// car HeapInit effectue certaines initialisations globales
	if (not bIsHeapInitialized)
		HeapInit();
}

// On ne detruit pas la heap (inutile)
PrivateHeapInitializer::~PrivateHeapInitializer() {}

//////////////////////////////////////////////////
// Reimplementation des operateurs new et delete
// Il apparait a l'usage que bien que 20% plus
// econome en memoire systeme et 3 fois plus rapide
// en allocation/liberation de la memoire, l'allocateur
// par defaut provoque moins de "defaut de page" systeme
// qui sont couteux en temps, et augmentent globalement
// le temps d'execution des programmes

// Mode debug: nouvel allocateur avec operations
//             de controle de la memoire
// Mode release: ancien allocateur
// Mode release NewMem: nouvel allocateur

//////////////////////////////////////////////////////////////////////////////////////
// Allocateurs specifiques

#ifdef RELEASENEWMEM

void* SystemObject::operator new(size_t nSize)
{
#ifdef MEMSTATSCOLLECT
	MemStatsUpdateAlloc(nSize);
#endif
#ifndef NOMEMCONTROL
	return (nSize == 0 ? NULL : DebugMemAlloc(nSize));
#else
#ifdef RELEASENEWMEM
	return (nSize == 0 ? NULL : MemAlloc(nSize));
#else
	return malloc(nSize);
#endif // RELEASENEWMEM
#endif
}

void SystemObject::operator delete(void* pBlock)
{
#ifndef NOMEMCONTROL
	if (pBlock != NULL)
		DebugMemFree(pBlock);
#else
#ifdef RELEASENEWMEM
	if (pBlock != NULL)
		MemFree(pBlock);
#else
	free(pBlock);
#endif // RELEASENEWMEM
#endif
}
#endif // RELEASENEWMEM

char* SystemObject::NewCharArray(size_t nArraySize)
{
	assert(sizeof(char) == 1);
	return (char*)NewMemoryBlock(nArraySize);
}

void SystemObject::DeleteCharArray(char* cCharArray)
{
	DeleteMemoryBlock(cCharArray);
}

int* SystemObject::NewIntArray(size_t nArraySize)
{
	return (int*)NewMemoryBlock(nArraySize * sizeof(int));
}

void SystemObject::DeleteIntArray(int* nIntArray)
{
	DeleteMemoryBlock(nIntArray);
}

void* SystemObject::NewMemoryBlock(size_t nBlockSize)
{
#ifdef MEMSTATSCOLLECT
	MemStatsUpdateAlloc(nBlockSize);
#endif
#ifndef NOMEMCONTROL
	return (nBlockSize == 0 ? NULL : DebugMemAlloc(nBlockSize));
#else
#ifdef RELEASENEWMEM
	return (nBlockSize == 0 ? NULL : MemAlloc(nBlockSize));
#else
	// Allocation standard, non dediee grande taille
	return malloc(nBlockSize);
#endif // RELEASENEWMEM
#endif
}

void SystemObject::DeleteMemoryBlock(void* pBlock)
{
#ifndef NOMEMCONTROL
	if (pBlock != NULL)
		DebugMemFree(pBlock);
#else
#ifdef RELEASENEWMEM
	if (pBlock != NULL)
		MemFree(pBlock);
#else
	// Destruction standard, non dediee grande taille
	free(pBlock);
#endif // RELEASENEWMEM
#endif
}

// Following defines added to compute the MemGetAdressablePhysicalMemory values
#define SIZE_OF_SIZE_T_32_BITS_SYSTEM 4
#define SIZE_OF_SIZE_T_64_BITS_SYSTEM 8

double MemGetAllocatorOverhead()
{
	// On utilise une variable locale pour eviter le warning "conditional expression is constant"
	int nSizeOfSizeT = sizeof(size_t);

	if (nSizeOfSizeT == SIZE_OF_SIZE_T_32_BITS_SYSTEM)
	{
		return 0.5;
	}
	else
	{
		// En 64 bits, l'overhead de gestion de la memoire est plus important
		return 0.6;
	}
}

longint MemGetAllocatorReserve()
{
	return MemFixedSizeHeapMaxNumber * MemSystemSegmentByteSize;
}

longint MemGetHeapMemory()
{
#ifdef RELEASENEWMEM
	// Prise en compte de la heap, en enlevant les segments libres
	return MemHeapMemory - pHeap->nFreeSegmentNumber * MemSystemSegmentByteSize;
#else
	return MemGetCurrentProcessVirtualMemory();
#endif // RELEASENEWMEM
}

longint MemGetMaxHeapRequestedMemory()
{
#ifdef RELEASENEWMEM
	return MemHeapMaxRequestedMemory;
#else
	return 0;
#endif // RELEASENEWMEM
}

longint MemGetTotalHeapRequestedMemory()
{
#ifdef RELEASENEWMEM
	return MemHeapTotalRequestedMemory;
#else
	return 0;
#endif // RELEASENEWMEM
}

void MemPrintHeapStats(FILE* fOutput)
{
#ifdef RELEASENEWMEM
	MemSegment* psegCurrent;
	size_t nDataSize;
	int i;

	// Entete
	if (GetProcessId() != 0)
		fprintf(stdout, "Process %d: ", GetProcessId());
	fprintf(fOutput, "Heap stats\n");
	fprintf(fOutput, "\tHeap memory\t%lld\n", MemGetHeapMemory());
	fprintf(fOutput, "\tHeap max requested memory\t%lld\n", MemGetMaxHeapRequestedMemory());
	fprintf(fOutput, "\tHeap total requested memory\t%lld\n", MemGetTotalHeapRequestedMemory());
	fprintf(fOutput, "\tSegments\t%lld\n", MemHeapCurrentSegmentNumber);
	fprintf(fOutput, "Segment\tpData\tnData\tSegmentSize\tRealBlockSize\tNbBloc\tAlloc\n");

	// Parcours des segments non libres de la heap
	if (pHeap != NULL)
	{
		// Affichage pour les segments de la heap
		for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
		{
			psegCurrent = pHeap->fixedSizeHeapHeadSegments[i];
			while (psegCurrent != NULL)
			{
				if (psegCurrent->nNbBlock > 0)
				{
					// Affichage des stats
					nDataSize = (psegCurrent->nSegmentTotalAllocSize * sizeof(void*) -
						     (sizeof(MemSegment) - sizeof(void*))) /
						    sizeof(void*);
					fprintf(fOutput, "%d\t", i);
					fprintf(fOutput, "%p\t", psegCurrent->pData);
					fprintf(fOutput, "%lld\t", (longint)(uintptr_t)psegCurrent->pData);
					fprintf(fOutput, "%d\t",
						(int)(psegCurrent->nSegmentTotalAllocSize * sizeof(void*)));
					fprintf(fOutput, "%d\t", (int)(psegCurrent->nRealBlockSize * sizeof(void*)));
					fprintf(fOutput, "%d\t", (int)(nDataSize / psegCurrent->nRealBlockSize));
					fprintf(fOutput, "%d\n", (int)psegCurrent->nNbBlock);
				}
				psegCurrent = psegCurrent->nextSegment;
			}
		}
	}

	fflush(fOutput);
#else
	if (GetProcessId() != 0)
		fprintf(stdout, "Process %d: ", GetProcessId());
	fprintf(fOutput, "Heap stats: not implemented\n");
	fflush(fOutput);
#endif // RELEASENEWMEM
}

////////////////////////////////////////////////////////////////////////////////////////////

void MemSetStatsHandler(MemStatsHandler fMemStatsFunction, longint lCallFrequency)
{
	assert(lCallFrequency >= 0);
	require(fMemStatsFunction == NULL or lCallFrequency > 0);
	require(memUserHanderCallFrequency == 0 or lCallFrequency == 0);

	// Reinitialisation des stats d'allocation
	memUserHandlerStats.lTotalAlloc = 0;
	memUserHandlerStats.lTotalFree = 0;
	memUserHandlerStats.lTotalRequestedSize = 0;
	memUserHandlerStats.lTotalGrantedSize = 0;
	memUserHandlerStats.lTotalFreeSize = 0;
	memUserHandlerStats.lMaxAlloc = 0;
	memUserHandlerStats.lMaxGrantedSize = 0;

#ifdef RELEASENEWMEM
	// Initialisation avec la memoire courante si on initialise le handler
	if (lCallFrequency > 0)
	{
		MemSegment* psegCurrent;
		int i;

		// Parcours des segments de la heap pour identifier le nombre d'allocations en cours et leur taille
		if (pHeap != NULL)
		{
			// Affichage pour les segments de la heap
			for (i = 0; i < MemFixedSizeHeapMaxNumber; i++)
			{
				psegCurrent = pHeap->fixedSizeHeapHeadSegments[i];
				while (psegCurrent != NULL)
				{
					if (psegCurrent->nNbBlock > 0)
					{
						memUserHandlerStats.lTotalAlloc += psegCurrent->nNbBlock;
						memUserHandlerStats.lTotalGrantedSize +=
						    psegCurrent->nNbBlock * sizeof(void*) *
						    (psegCurrent->nRealBlockSize - 1);
					}
					psegCurrent = psegCurrent->nextSegment;
				}
			}
		}

		// Initialisation heuristique des autre statistiques
		memUserHandlerStats.lTotalRequestedSize = memUserHandlerStats.lTotalGrantedSize;
		memUserHandlerStats.lMaxAlloc = memUserHandlerStats.lTotalAlloc;
		memUserHandlerStats.lMaxGrantedSize = memUserHandlerStats.lTotalGrantedSize;
	}
#endif // RELEASENEWMEM

	// Parametrage du handler
	memUserHanderFunction = fMemStatsFunction;
	memUserHanderCallFrequency = lCallFrequency;
}

MemStatsHandler MemGetStatsHandler()
{
	return memUserHanderFunction;
}

longint MemGetStatsCallFrequency()
{
	return memUserHanderCallFrequency;
}

longint MemGetAllocNumber()
{
	return memUserHandlerStats.lTotalAlloc - memUserHandlerStats.lTotalFree;
}

longint MemGetGrantedSize()
{
	return memUserHandlerStats.lTotalGrantedSize - memUserHandlerStats.lTotalFreeSize;
}

longint MemGetMaxAllocNumber()
{
	return memUserHandlerStats.lMaxAlloc;
}

longint MemGetMaxGrantedSize()
{
	return memUserHandlerStats.lMaxGrantedSize;
}

longint MemGetTotalAllocNumber()
{
	return memUserHandlerStats.lTotalAlloc;
}

longint MemGetTotalFreeNumber()
{
	return memUserHandlerStats.lTotalFree;
}

longint MemGetTotalRequestedSize()
{
	return memUserHandlerStats.lTotalRequestedSize;
}

longint MemGetTotalGrantedSize()
{
	return memUserHandlerStats.lTotalGrantedSize;
}

longint MemGetTotalFreeSize()
{
	return memUserHandlerStats.lTotalFreeSize;
}