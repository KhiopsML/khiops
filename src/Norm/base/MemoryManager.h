// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// Le nouvel allocateur etant valide depuis des annees,
// il est desormais active systematiquement par defaut
// On peut se ramener a l'allocateur standard en compilant avec un define STANDARDMEM

#ifndef STANDARDMEM
#define RELEASENEWMEM
#undef NOMEMCONTROL
#else
#define NOMEMCONTROL
#endif // STANDARDMEM

#include "SystemResource.h"
#include "Portability.h"

//////////////////////////////////////////////////////////////////////
//          Bibliotheque de routines de gestion de la memoire
//////////////////////////////////////////////////////////////////////
// Cette bibliotheque redefinit des routine de gestion de la memoire,
// avec une strategie d'allocation particuliere, basee sur des
// segments specialises gerant des blocks de taille fixe.
//
// Ces routines ne seront efficaces en terme d'occupation memoire et de
// rapidite que pour des programme utilisant intensivement la memoire
// dynamique (typiquement, acces tres frequents, et plus de 10 Mo aloues).
// Dans ce cas, le nouvel allocateur utilise environ 20% de memoire
// systeme en moins, et est 3 fois plus rapide (ce qui peut representer
// une acceleration d'envrion 20% de la rapidite totale d'une appliction).
//
// Par defaut, l'allocateur systeme est actif. Il faut se linker avec
// la librairie dediee baseONM pour beneficier du nouvel allocateur
// en mode release.
//
// En mode debug, un diagnostique de la memoire est realise au fur et a
// mesure des allocations, ce qui permet de detecter la plupart des erreurs
// suivantes de gestion de la memoire:
// Un compte rendu d'utilisation est affiche apres la fin du main.
//		- non liberation de memoire
//		- liberation deux fois du meme bloc memoire
//		- utilisation de memoire liberee
//		- ecrasement memoire dans une zone contigue a un bloc alloue
//	En cas d'erreur grave, le programme est arrete immediatement: il
//	  suffit de mettre un point d'arret sur la fonction exit() de
//	  standard.cpp pour localiser le probleme.
// En fin de programme, une statistique des allocations/liberation
// est presentee sur la sortie standard de facon automatique. Les
// blocs non desalloues sont identifies. On peut alors utiliser (dans
// une execution suivante) les fonctions SetMemAllocIndexExit() et
// SetMemAllocSizeExit() pour arreter le programme lors d'une allocation
// donnee (et donc d'identifier facilement le lieu d'allocation de memoire
// non liberee grace a un point d'arret sur le exit() de standard.cpp)

////////////////////////////////////////////////////////////////////////
// Gestion des erreurs d'allocation
//
// Quand il n'y a plus de memoire, l'allocateur renvoie NULL.
// Comme cela peut arriver a n'importe quel endroit dans une application,
// il n'y a en general rien d'autre a faire que de quitter proprement
// l'application apres avoir envoyer un message a l'utilisateur.
// C'est ce que fait la methode par defaut GetDefaultMemAllocErrorHandler
//
// Il est possible de personnaliser cette gestion d'erreur
//
// Une premiere possibilite est d'utiliser MemSetAllocErrorHandler
// pour personnaliser l'affichage de l'erreur.
// Il est conseille d'appeler la methode par defaut
// (GetDefaultMemAllocErrorHandler) si l'on souhaite une sortie propre
// de l'application.
// Il est egalement conseille de faire un affichage tres simple:
// toute nouvelle erreur d'allocation provoquera une sortie definitive
// (de facon a eviter une boucle d'appel infinie).
//
// Une deuxieme possibilite est de parametrer MemSetAllocErrorHandler
// avec la methode NULL. Dans ce cas, on retrouve le comportement standard
// de l'allocateur, qui renvoie NULL sans sortir de l'application.
// Il faut alors traiter explicitement les retours d'allocation en
// fonction des besoins applicatifs.
//
// Un usage coherent de ces possibilites de parametrages est de
// personnaliser les messages utilisateurs en parametrant
// MemSetAllocErrorHandler le plus tot possible dans l'application,
// et si il y a des besoins d'allocations tres specifiques (par
// exemple allocation de tres gros buffers) d'utiliser de facon
// temporaire le parametrage par NULL pour gerer explicitement
// les problemes d'allocation

// Prototype d'une fonction de gestion des erreurs d'allocation
typedef void (*MemAllocErrorHandler)(const char* sAllocErrorMessage);

// Redefinition de la methode d'affichage des messages
// Aucun affichage si methode NULL
void MemSetAllocErrorHandler(MemAllocErrorHandler fMemAllocError);
MemAllocErrorHandler MemGetAllocErrorHandler();

// Methode d'affichage par defaut: message dans la session DOS
MemAllocErrorHandler MemGetDefaultAllocErrorHandler();

// Utilisation du "nifty counter trick" pour forcer l'initialisation de la heap
static class PrivateHeapInitializer
{
public:
	PrivateHeapInitializer();
	~PrivateHeapInitializer();
} privateHeapInitializer;

///////////////////////////////////////////////////////////////////
// Methodes permettant d'identifier les allocations non desallouees
// Ces methodes ne sont actives qu'en mode debug

// Methode permetant de provoquer un arret pendant
// l'allocation du ieme bloc memoire
// Permet en se servant du debuger et en mettant un point
// d'arret sur GlobalExit() de reperer le lieu d'allocation
// d'un bloc non desaloue (diagnostique par MemCompleteCheck())
// Par defaut, 0 signifie pas d'arret
void MemSetAllocIndexExit(longint lAllocIndex);
longint MemGetAllocIndexExit();

// Idem pour une taille donnee de bloc
void MemSetAllocSizeExit(int nAllocSize);
int MemGetAllocSizeExit();

// Idem pour un bloc donne
// Si un block est specifie, les autres methodes provoquant un arret son desactivees.
// Par contre, leur parametre sert a contraindre l'arret par bloc:
//   bloc=pBlock et Index >= nAllocIndex et Size > nAllocSize
void MemSetAllocBlockExit(void* pBlock);
void* MemGetAllocBlockExit();

// Methode limitant physiquement la taille totale allouee,  meme en release
// En cas de passement, une erreur fatale est declenchee, avec sortie par GlobalExit()
// Par defaut, 0 signifie pas de limite
void MemSetMaxHeapSize(longint lSize);
longint MemGetMaxHeapSize();

///////////////////////////
// Gestion de la memoire

/////////////////////////////////////////////////////////////////////////////////////
// Classe ancetre de tout objet dont la memoire est geree par l'allocateur de Norm
// Toute classe doit heriter directement ou indirectement de SystemObject pour
// beneficier automatiquement de l'allocateur de Norm
// Pour les allocations de tableaux de caracteres, d'entiers, ou de blocs quelconques,
// il faut utiliser les allocateurs specifiques.
// Certaines allocations provenant de classes externe, comme les stream du C++,
// ne seront pas geres par l'allocateur de Norm. Cela reste marginal.
//
// L'interet d'avoir un allocateur specifique aux classes plutot qu'une redefiniton
// globale de l'allocateur est de permettre de faire cohabiter un exe et une dll
// developpee avec Norm. En effet, alors que sous windwos l'exe et la dll ont chacun
// une heap, ce n'est pas le cas sous linux, ou l'exe et l'allocateur partagent la heap.
// Cela cree des conflits entrainant des plantages non reproductibles. L'allocation
// specifique par classe permet ainsi de continuer de beneficier de l'allocateur de
// Norm, tout en gardant  une propre heap independante du reste de l'exe.
class SystemObject
{
public:
#if !defined NOMEMCONTROL || defined RELEASENEWMEM
	// Allocateur redefini pour les classes heritant de SystemObject
	static void* operator new(size_t nSize);
	static void operator delete(void* pBlock);

	// Redefinition de l'allocateur pour le placement new (usage avance)
	// Le destructeur n'est specifie que pour eviter un warning sur certains compilateurs
	static void* operator new(size_t nSize, void* where);
	static void operator delete(void* pBlock, void* where);
#endif // !defined NOMEMCONTROL || defined RELEASENEWMEM

	///////////////////////////////////////////////////////////////////////
	// Allocateurs specifiques a appeler explicitement pour tableaux
	// de caracteres ou d'entiers, et les classes ou structures a allouer
	// pour d'autres contextes

	// Allocateur specifique pour les tableaux de caractereres
	static char* NewCharArray(size_t nArraySize);
	static void DeleteCharArray(char* cCharArray);

	// Allocateur specifique pour les tableaux d'entiers
	static int* NewIntArray(size_t nArraySize);
	static void DeleteIntArray(int* nIntArray);

	// Allocateur generique
	// Attention a demander la bonne taille a allouer en utilisant sizeof puis
	// a faire un cast du block memoire retourner
	// Par exemple: char** pStringVector = (char**)NewMemoryBlock(10*sizeof(char*));
	static void* NewMemoryBlock(size_t nBlockSize);
	static void DeleteMemoryBlock(void* pBlock);
};

////////////////////////////////////////////////////////////////////////////////////////
// Methodes avancees permettant d'estimer la memoire de fonctionnement de l'allocateur

// Overhead d'allocation
// Utile pour estimer la memoire reellement occupee suite a des allocations
// Par exemple, un overhead de 0.3 signifie que l'allocateur occupe en moyenne 30% d'espace en
// memoire en plus que ce qui a ete demande
double MemGetAllocatorOverhead();

// Reserve memoire de l'allocateur, qui utilise potentiellement plus de memoire que necessaire
// Utile pour estimer la memoire totale reellement occupee suite a des allocations
// Par exemple, pour une reserve de 25 MB, cela signifie que l'allocateur occupera potentiellement
// 25 MB de RAM de plus que le total dedie au allocation (avec leur overhead)
// Globalement, la place memoire utilisee dans la Heap peut etre estimee(pire des cas) :
//   HeapEstimatedMemory = MemGetAllocatorReserve + NecessaryMemory * (1 + MemGetAllocatorOverhead)
longint MemGetAllocatorReserve();

// Taille des segments memoire geres par l'allocateur
const size_t MemSegmentByteSize = 65536;

////////////////////////////////////////////////////////////////////////////////////////
// Methodes avancees pour acceder aux statistiques sur la heap
//
// On a ici des statistiques sur la heap, mesuree au niveau de segments memoires de grande taille
// gerees par l'allocateur, qui chacun prennent en charge les allocation utilisateurs
// Leur emprunte memoire est proche d'une emprunte memoire systeme, puisque ces segments
// sont issues d'invocations a l'allocateur systeme
// Avec l'allocateur standard, seule la methode MemGetHeapMemory retourne une valeur valide

// Taille approximative de la memoire alloue dans la heap
longint MemGetHeapMemory();

// Taille approximative de la memoire max alloue dans la heap depuis le debut du programme
longint MemGetMaxHeapRequestedMemory();

// Taille approximative de la memoire totale demandee dans la heap depuis le debut du programme
longint MemGetTotalHeapRequestedMemory();

// Methode avancee d'affichage du contenu exhautif de la heap
// avec des statistiques detaille pour tous les segments alloues
void MemPrintHeapStats(FILE* fOutput);

////////////////////////////////////////////////////////////////////////////////////////
// Methodes avancees pour diagnostiquement finement la consommation memoire
// Attention, ces methodes ont potentiellement un impact limite, mais potentiellement
// non negligeable sur les performances, si le handler doit etre appele frequemment
// Elles ne doivent etre utilisee pour le suivi fin de l'utilisation de l'allocateur
//
// On a ici des statistiques sur les allocations, mesuree au niveau des new et delete.
// Leur emprunte memoire correspond a une emprunte memoire utilisateurs, puisque ces
// statistiques tracent toutes les allocations et desallocations gerees par l'allocateur.
//
// Attention, l'emprunte memoire utilisateur peut-etre beaucoup plus petite que
// l'emprunte memoire systeme, pour les raison suivantes:
//   . les segments de la heap sont presque vides quand ils viennent d'etre alloues
//   . il y a un overhead entre les taille demandees et les taille obtenues
//   . et cet overhead est nettement plus important en mode debug
//   . les variables statiques creees avant le debut du main() occupent une place
//     dans la heap et ne peuevtn avoir ete comptabilisees au niveau des allocations
// A l'inverse, le total cumule de la memoire systeme peut etre beaucoup plus petit
// que le total cumule de la memoire utilisateur. C'est le cas par exemple quand on demande
// un tres grande nombre d'allocation/desallocation, qui seront gere par un seul segment
// jamais desalloue ou realloue

// Prototype d'une fonction de gestion de stats memoire
typedef void (*MemStatsHandler)();

// Parametrage d'une methode de gestion des stats d'allocation, qui sera invoquee selon la frequence requise
// Les stats d'allocation sont toutes initialisee a 0 puis comptabilise des qu'une methode est parametree.
// des lors, tout evenement d'allocation, new ou delete, contribue a mettre a jour les stats d'allocation
// La methode en parametre est alors appelee apres chaque evenement d'allocation selon la frequence indiquee
// Les parametrages suivants sont possibles
//   . fMemStatsFunction=NULL, lCallFrequency=0: arret de la collecte des stats
//   . fMemStatsFunction=NULL, lCallFrequency>0: collecte de stats disponibles pour l'utilisateur, sans handler
//   . fMemStatsFunction!=NULL, lCallFrequency>0: collecte de stats et appel regulier du handler
void MemSetStatsHandler(MemStatsHandler fMemStatsFunction, longint lCallFrequency);

// Acces a la methode de gestion des stats d'allocation (NULLL par defaut)
MemStatsHandler MemGetStatsHandler();

// Acces a la frequency d'appel de la methode de gestion des stats d'allocation (0 par defaut)
longint MemGetStatsCallFrequency();

// Acces aux statistiques d'allocation, qui pourront etre utilisees pour l'implementation d'une
// methode de gestion de stats d'allocation, qui pourra collecter ou afficher les stats de son choix
// Renvoie 0 si pas de methode de gestion des stats parametree, ou si allocateur standard utilise
longint MemGetAllocNumber();        // nombre courant d'allocations
longint MemGetGrantedSize();        // taille courante alouee
longint MemGetMaxAllocNumber();     // nombre maximale d'allocations
longint MemGetMaxGrantedSize();     // taille maximale alouee
longint MemGetTotalAllocNumber();   // nombre total d'allocations
longint MemGetTotalFreeNumber();    // nombre total de liberations
longint MemGetTotalRequestedSize(); // taille totale demandee
longint MemGetTotalGrantedSize();   // taille totale alouee
longint MemGetTotalFreeSize();      // taille totale liberee

// Methodes en inline
#if !defined NOMEMCONTROL || defined RELEASENEWMEM
inline void* SystemObject::operator new(size_t nSize, void* where)
{
	return where;
}

inline void SystemObject::operator delete(void* pBlock, void* where)
{
	delete (char*)pBlock;
}
#endif // !defined NOMEMCONTROL || defined RELEASENEWMEM