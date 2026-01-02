// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MemVector.h"

void MemVector::Delete(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
		       const int nElementSize)
{
	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	if (memHugeVector.pValues != NULL)
	{
		int i;
		int nBlockNumber;

		// Desallocation
		if (nAllocSize <= nBlockSize)
			DeleteCharArray(memHugeVector.pValues);
		else
		{
			nBlockNumber = (nAllocSize - 1) / nBlockSize + 1;
			for (i = nBlockNumber - 1; i >= 0; i--)
				DeleteCharArray(memHugeVector.pValueBlocks[i]);
			DeleteMemoryBlock(memHugeVector.pValueBlocks);
		}

		// Mise a jour
		memHugeVector.pValues = NULL;
		nSize = 0;
		nAllocSize = 0;
	}
	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

// Retourne la puissance de 2 la plus proche au dessus de nSize
// Pourrais etre utile si l'on essayait d'allore les tableau de blocs par puissance de 2,
// en permettant de retourner la taille alouee du tablau de blocs sans avoir a la stocker
// Pour l'instant, ce code n'est pas justifie:
//  . complexe a mettre en oeuvre et a maintenir, risque de bug
//  . les vecteur de tres grande taille sont rares
//  . meme dans ce cas la, ou le retaillage des tableau de blocs a une complexite quadratique
//   celle ci O((N/BlockSize)^2) reste quasi-lineaire avec N, d'ou un gain de temps escompte negligeable
// Globalement, le ratio cout/gain n'est pas interessant
int MemVectorGetUpperPowerOfTwo(int nSize)
{
	unsigned long v = nSize;
	require(nSize > 0);
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	ensure(v / 2 <= (unsigned long)nSize);
	ensure((unsigned long)nSize <= v);
	ensure(2 * (v / 2) == v);
	return v;
}

// On tente de retailler le vecteur
// En mode standard, en cas de memoire insuffisante, cela plante en erreur fatale
// En mode avec allocation protegee (appel depuis SetLargeSize), on essaie d'allouer
// le nouveau vecteur, sans planter: sinon, on garde le vecteur initial
void MemVector::SetSize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			const int nElementSize, int nNewSize)
{
	int i;
	int nStartBlock;
	int nStopBlock;
	int nBlockNumber;
	int nNewAllocSize;
	int nNewBlockNumber;
	char* pNewValues;
	char** pNewValueBlocks;
	longint lTmp;

	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	require(nNewSize >= 0);
	require(nNewSize <= INT_MAX / nElementSize);

	// Cas particulier: pas de changement de taille
	if (nNewSize == nSize)
		return;

	// Calcul des index des blocks concernes
	if (nSize > 0)
	{
		nStartBlock = (nSize - 1) / nBlockSize;
		nBlockNumber = nStartBlock + 1;
	}
	else
	{
		nStartBlock = 0;
		nBlockNumber = 0;
	}
	if (nNewSize > 0)
	{
		nStopBlock = (nNewSize - 1) / nBlockSize;
		nNewBlockNumber = nStopBlock + 1;
	}
	else
	{
		nStopBlock = 0;
		nNewBlockNumber = 0;
	}

	// Desallocation si nouvelle taille est 0
	if (nNewSize == 0)
	{
		// Desallocation de l'unique block
		if (nAllocSize <= nBlockSize)
		{
			if (memHugeVector.pValues != NULL)
				DeleteCharArray(memHugeVector.pValues);
		}
		else
		// Desallocation de tous les blocks du tableau de blocks
		{
			assert(memHugeVector.pValueBlocks != NULL);
			nBlockNumber = (nAllocSize - 1) / nBlockSize + 1;
			for (i = nBlockNumber - 1; i >= 0; i--)
				DeleteCharArray(memHugeVector.pValueBlocks[i]);
			DeleteMemoryBlock(memHugeVector.pValueBlocks);
		}
		memHugeVector.pValues = NULL;
		nSize = 0;
		nAllocSize = 0;
	}

	// Simple diminution de taille si nouvelle taille inferieure
	else if (nNewSize <= nSize)
	{
		// Desallocation potentielle des blocks inutiles
		if (nBlockNumber > 1)
		{
			// Desallocation des blocs en trop
			// Pas de reallocation du tableau de bloc a une plus petite taille (gain marginal)
			for (i = nBlockNumber - 1; i >= nNewBlockNumber; i--)
				DeleteCharArray(memHugeVector.pValueBlocks[i]);

			// Passage si possible en mode mono-block
			if (nNewBlockNumber == 1)
			{
				pNewValues = memHugeVector.pValueBlocks[0];

				// Destruction du tableau de block
				DeleteMemoryBlock(memHugeVector.pValueBlocks);

				// Memorisation de l'ancien premier block
				memHugeVector.pValues = pNewValues;
			}

			// Memorisation de la nouvelle taille allouee
			nAllocSize = nNewBlockNumber * nBlockSize;
		}

		// Diminution eventuelle de la taille en cas de petite taille
		if (nNewBlockNumber == 1)
		{
			assert(nAllocSize <= nBlockSize);

			// Retaillage si trop de memoire utilisee
			if (nNewSize <= nAllocSize / 2)
			{
				nNewAllocSize = nNewSize;

				// Allocation du nouveau tableau
				pNewValues = NewCharArray(nNewAllocSize * nElementSize);

				// On quite si erreur: on reste sur l'ancien vecteur inchange
				if (pNewValues == NULL)
				{
					ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
					return;
				}

				// Memorisation de la taille allouee
				nAllocSize = nNewAllocSize;

				// Recopie de la partie commune
				if (memHugeVector.pValues != NULL)
					memcpy(pNewValues, memHugeVector.pValues, nNewSize * nElementSize);

				// Nettoyage
				if (memHugeVector.pValues != NULL)
					DeleteCharArray(memHugeVector.pValues);

				// Memorisation du nouveau block
				memHugeVector.pValues = pNewValues;
			}
		}

		nSize = nNewSize;
	}

	// Reallocation (eventuelle) si nouvelle taille superieure
	else
	{
		assert(nNewSize > nSize);

		// Reallocation et transfert des valeurs
		if (nNewSize > nAllocSize)
		{
			// Cas des petites tailles
			if (nNewBlockNumber == 1)
			{
				// Calcul de la nouvelle taille
				if (nNewSize > 2 * nSize)
					nNewAllocSize = nNewSize;
				else
					nNewAllocSize = 2 * nSize;

				// Ajustement si necessaire a exactement la taille d'un block
				if (nNewAllocSize >= nBlockSize / 2)
					nNewAllocSize = nBlockSize;

				// Allocation du nouveau tableau
				pNewValues = NewCharArray(nNewAllocSize * nElementSize);

				// On quite si erreur: on reste sur l'ancien vecteur inchange
				if (pNewValues == NULL)
				{
					ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
					return;
				}

				// Memorisation de la taille allouee
				nAllocSize = nNewAllocSize;

				// Recopie de la partie commune
				if (memHugeVector.pValues != NULL)
					memcpy(pNewValues, memHugeVector.pValues, nSize * nElementSize);

				// Nettoyage
				if (memHugeVector.pValues != NULL)
					DeleteCharArray(memHugeVector.pValues);

				// Memorisation du nouveau block
				memHugeVector.pValues = pNewValues;
			}
			// Cas des grandes tailles
			else
			{
				assert(nNewBlockNumber > 1);

				// Cas ou il y avait initialement au plus un block
				if (nBlockNumber <= 1)
				{
					assert(nAllocSize <= nBlockSize);

					// Si ce block n'etait pas plein, il faut le reallouer a sa taille maximale
					pNewValues = memHugeVector.pValues;
					if (nAllocSize < nBlockSize and nSize > 0)
					{
						assert(memHugeVector.pValues != NULL);

						// Allocation du nouveau tableau
						pNewValues = NewCharArray(nBlockSize * nElementSize);

						// On quite si erreur: on reste sur l'ancien vecteur inchange
						if (pNewValues == NULL)
						{
							ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize,
								     nElementSize));
							return;
						}

						// Recopie de la partie commune
						memcpy(pNewValues, memHugeVector.pValues, nSize * nElementSize);

						// Nettoyage
						DeleteCharArray(memHugeVector.pValues);

						// On memorise le nouveau bloc
						// Meme si ca plante apres, on gardera ce nouvel etat
						memHugeVector.pValues = pNewValues;
						nAllocSize = nBlockSize;
					}

					// Allocation du tableau de blocs
					pNewValueBlocks = (char**)NewMemoryBlock(nNewBlockNumber * sizeof(char*));

					// On quite si erreur: on reste sur l'ancien vecteur inchange
					if (pNewValueBlocks == NULL)
					{
						ensure(
						    Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
						return;
					}

					// Memorisation du premier block
					if (pNewValues != NULL)
						pNewValueBlocks[0] = pNewValues;

					// Initialisation a NULL des blocs suivants
					for (i = nBlockNumber; i < nNewBlockNumber; i++)
						pNewValueBlocks[i] = NULL;

					// Allocation des blocs suivants
					for (i = nBlockNumber; i < nNewBlockNumber; i++)
					{
						pNewValues = NewCharArray(nBlockSize * nElementSize);
						pNewValueBlocks[i] = pNewValues;

						// Arret de l'allocation si erreur: on detruit ce qui a etet alloue
						if (pNewValues == NULL)
						{
							// Destruction des blocs alloues
							i--;
							while (i >= nBlockNumber)
							{
								pNewValues = pNewValueBlocks[i];
								DeleteCharArray(pNewValues);
								i--;
							}

							// Destruction du tableau de blocs
							DeleteMemoryBlock(pNewValueBlocks);

							// On quitte
							ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize,
								     nElementSize));
							return;
						}
					}

					// Memorisation du nouveau tableau de blocs, uniquement quand tout est ok
					memHugeVector.pValueBlocks = pNewValueBlocks;
				}
				// Cas ou il y avait deja plusieurs blocks
				else
				{
					assert(nBlockNumber > 1);
					assert(nAllocSize > nBlockSize);

					// Allocation d'un nouveau tableau de blocs
					pNewValueBlocks = (char**)NewMemoryBlock(nNewBlockNumber * sizeof(char*));

					// On quite si erreur: on reste sur l'ancien vecteur inchange
					if (pNewValueBlocks == NULL)
					{
						ensure(
						    Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
						return;
					}

					// Recopie des ancien blocks
					memcpy(pNewValueBlocks, memHugeVector.pValueBlocks,
					       nBlockNumber * sizeof(char*));

					// Allocation des blocs suivants
					for (i = nBlockNumber; i < nNewBlockNumber; i++)
					{
						pNewValues = NewCharArray(nBlockSize * nElementSize);
						pNewValueBlocks[i] = pNewValues;

						// Arret de l'allocation si erreur: on detruit ce qui a etet alloue
						if (pNewValues == NULL)
						{
							// Destruction des blocs alloues
							i--;
							while (i >= nBlockNumber)
							{
								pNewValues = pNewValueBlocks[i];
								DeleteCharArray(pNewValues);
								i--;
							}

							// Destruction du tableau de blocs
							DeleteMemoryBlock(pNewValueBlocks);

							// On quitte
							ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize,
								     nElementSize));
							return;
						}
					}

					// Memorisation du nouveau tableau de blocs, uniquement quand tout est ok
					DeleteMemoryBlock(memHugeVector.pValueBlocks);
					memHugeVector.pValueBlocks = pNewValueBlocks;
				}

				// Memorisation de la taille allouee
				// Attention a l'effet de bord quand avec le nombre maximal de segments permet
				// d'atteindre 2Go (2 Go = INT_MAX+1) Dans ce cas, on se limite a la taille maximum
				// utilisable
				lTmp = nNewBlockNumber;
				lTmp *= nBlockSize;
				assert(lTmp - 1 <= INT_MAX);
				if (lTmp <= INT_MAX)
					nAllocSize = (int)lTmp;
				else
					nAllocSize = INT_MAX;
			}
		}

		// Finalisation: on met a zero la partie supplementaire du tableau
		assert(nNewSize <= nAllocSize);
		// Initialisation a zero de la partie supplementaire
		if (nAllocSize <= nBlockSize)
			memset(&(memHugeVector.pValues[nSize * nElementSize]), 0, (nNewSize - nSize) * nElementSize);
		else
		{
			// Si un seul block concerne, on initialise a zero la partie supplementaire
			if (nStartBlock == nStopBlock)
			{
				assert(nNewSize - nSize <= nBlockSize);
				memset(&(memHugeVector.pValueBlocks[nStartBlock][(nSize % nBlockSize) * nElementSize]),
				       0, (nNewSize - nSize) * nElementSize);
			}
			// Initialisation multi-block necessaire
			else
			{
				// Initialisation a 0 de la fin du dernier block courant (nStartBlock)
				if (nSize % nBlockSize > 0)
					memset(&(memHugeVector
						     .pValueBlocks[nStartBlock][(nSize % nBlockSize) * nElementSize]),
					       0, (nBlockSize - nSize % nBlockSize) * nElementSize);

				// Initialisation a 0 des blocks intermediaires concernes
				for (i = nBlockNumber; i < nNewBlockNumber - 1; i++)
					memset(memHugeVector.pValueBlocks[i], 0, nBlockSize * nElementSize);

				// Initialisation a 0 du debut du dernier block
				if (nNewSize % nBlockSize > 0)
					memset(memHugeVector.pValueBlocks[nStopBlock], 0,
					       (nNewSize % nBlockSize) * nElementSize);
				else
					memset(memHugeVector.pValueBlocks[nStopBlock], 0, nBlockSize * nElementSize);
			}
		}

		// Fin des initialisations
		nSize = nNewSize;
	}
	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

void MemVector::Initialize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			   const int nElementSize)
{
	int i;
	int nBlockNumber;

	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));

	// Cas mono-block
	if (nAllocSize <= nBlockSize)
		memset(memHugeVector.pValues, 0, nSize * nElementSize);
	// Cas multi-block
	else
	{
		nBlockNumber = (nAllocSize - 1) / nBlockSize + 1;
		for (i = nBlockNumber - 2; i >= 0; i--)
			memset(memHugeVector.pValueBlocks[i], 0, nBlockSize * nElementSize);
		if (nSize % nBlockSize > 0)
			memset(memHugeVector.pValueBlocks[nBlockNumber - 1], 0, (nSize % nBlockSize) * nElementSize);
		else
			memset(memHugeVector.pValueBlocks[nBlockNumber - 1], 0, nBlockSize * nElementSize);
	}

	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

// Push d'un block libre dans une pile reduite a deux elements
static inline void MemVectorPushFreeBlock(char*& pHeadFreeBlock, char*& pTailFreeBlock, char* pFreeBlock)
{
	if (pHeadFreeBlock == NULL)
		pHeadFreeBlock = pFreeBlock;
	else
	{
		assert(pTailFreeBlock == NULL);
		pTailFreeBlock = pFreeBlock;
	}
}

// Pop d'un block libre dans une pile reduite a deux elements
static inline char* MemVectorPopFreeBlock(char*& pHeadFreeBlock, char*& pTailFreeBlock)
{
	char* pFreeBlock = NULL;
	if (pTailFreeBlock != NULL)
	{
		pFreeBlock = pTailFreeBlock;
		pTailFreeBlock = NULL;
	}
	else
	{
		assert(pHeadFreeBlock != NULL);
		pFreeBlock = pHeadFreeBlock;
		pHeadFreeBlock = NULL;
	}
	check(pFreeBlock);
	return pFreeBlock;
}

// Recopie d'un element d'un vecteur vers un autre
static inline void MemVectorCopyElement(char* pTarget, char* pSource, const int nElementSize)
{
	require(pTarget != NULL);
	require(pSource != NULL);
	require(nElementSize > 0);

	// Implementation optimisee
	// Les compilateurs sont capables de reperer un memcpy avec une taille constante de petite taille
	// et de les remplacer par des instruction machine directe
	if (nElementSize == 4)
		memcpy(pTarget, pSource, 4);
	else if (nElementSize == 8)
		memcpy(pTarget, pSource, 8);
	else
	{
		// Plus efficace qu'un memcpy si l'on suppose une taille pas trop grande
		int i = nElementSize;
		while (i > 0)
		{
			*pTarget = *pSource;
			pTarget++;
			pSource++;
			i--;
		}
	}
}

void MemVector::Sort(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
		     const int nElementSize, MemVectorCompareFunction fCompareFunction)
{
	int nBlockNumber;
	int nBlock;
	int nMergeBlockNumber;
	int iGlobal1;
	int iGlobal2;
	int iLastGlobal1;
	int iLastGlobal2;
	int iGlobal;
	char* pValueBlock1;
	char* pValueBlock2;
	char* pValueBlock;
	int iLocal1;
	int iLocal2;
	int iLocal;
	char** pSourceValueBlocks;
	char** pTargetValueBlocks;
	char* pHeadFreeBlock;
	char* pTailFreeBlock;

	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	require(fCompareFunction != NULL);

	if (nSize > 0)
	{
		nBlockNumber = (nAllocSize - 1) / nBlockSize + 1;
		assert(nBlockNumber >= 1);

		// Cas d'un seul block
		if (nBlockNumber == 1)
		{
			assert(memHugeVector.pValues != NULL);
			qsort((void*)(memHugeVector.pValues), nSize, nElementSize, fCompareFunction);
		}
		// Cas multi-block
		else
		{
			assert(memHugeVector.pValueBlocks != NULL);

			////////////////////////////////////////////
			// Premiere etape: on tri chacun des blocs

			// Tri des permiers blocs
			for (nBlock = 0; nBlock < nBlockNumber - 1; nBlock++)
				qsort((void*)(memHugeVector.pValueBlocks[nBlock]), nBlockSize, nElementSize,
				      fCompareFunction);

			// Tri du dernier blocks (par forcement complet)
			if (nSize % nBlockSize > 0)
				qsort((void*)(memHugeVector.pValueBlocks[nBlock]), nSize % nBlockSize, nElementSize,
				      fCompareFunction);
			else
				qsort((void*)(memHugeVector.pValueBlocks[nBlock]), nBlockSize, nElementSize,
				      fCompareFunction);

			////////////////////////////////////////////////////////////////////////////////////
			// Seconde etape: utilisation de l'algorithme MergeSort pour fusionner les blocks

			// Creation d'un second tableau de vecteurs
			pSourceValueBlocks = memHugeVector.pValueBlocks;
			pTargetValueBlocks = (char**)NewMemoryBlock(nBlockNumber * sizeof(char*));

			// Creation de deux blocks temporaire sous forme d'une pile
			// Deux blocks sont en effet suffisant pour transferer les valeurs
			pHeadFreeBlock = NewCharArray(nBlockSize * nElementSize);
			pTailFreeBlock = NewCharArray(nBlockSize * nElementSize);

			// On tri les blocs par paquet (de un block initialement), en doublant la taille des paquets a
			// chaque passe, jusqu'a ce que tout soit trie dans la derniere passe
			nMergeBlockNumber = 1;
			pValueBlock = NULL;
			pValueBlock1 = NULL;
			pValueBlock2 = NULL;
			while (nMergeBlockNumber < nBlockNumber)
			{
				// Initialisation des index globaux
				iGlobal = 0;
				iGlobal1 = 0;
				iGlobal2 = nMergeBlockNumber * nBlockSize;

				// Initialisation des index locaux aux blocs
				iLocal1 = 0;
				iLocal2 = 0;
				iLocal = 0;

				// Parcours de tous les blocs pour tri par paquets de taille MergeBlockNumber
				while (iGlobal1 < nSize)
				{
					// Determination de l'index global de fin du premier paquet
					iLastGlobal1 = iGlobal1 + nMergeBlockNumber * nBlockSize;
					if (iLastGlobal1 > nSize)
						iLastGlobal1 = nSize;

					// Determination de l'index global de fin du second paquet
					iLastGlobal2 = iGlobal2 + nMergeBlockNumber * nBlockSize;
					if (iLastGlobal2 > nSize)
						iLastGlobal2 = nSize;

					// Parcours synchronise des deux paquets a trier
					while (iGlobal1 < iLastGlobal1 and iGlobal2 < iLastGlobal2)
					{
						assert(iGlobal < iLastGlobal2);
						assert(iLocal == iGlobal % nBlockSize);
						assert(iLocal1 == iGlobal1 % nBlockSize);
						assert(iLocal2 == iGlobal2 % nBlockSize);

						// Reactualisation des blocks de travail
						if (iLocal1 == 0)
							pValueBlock1 = pSourceValueBlocks[iGlobal1 / nBlockSize];
						if (iLocal2 == 0)
							pValueBlock2 = pSourceValueBlocks[iGlobal2 / nBlockSize];
						if (iLocal == 0)
						{
							// Pop d'un block libre
							pValueBlock =
							    MemVectorPopFreeBlock(pHeadFreeBlock, pTailFreeBlock);
							pTargetValueBlocks[iGlobal / nBlockSize] = pValueBlock;
						}

						// Comparaison des elements
						if (fCompareFunction(&(pValueBlock1[iLocal1 * nElementSize]),
								     &(pValueBlock2[iLocal2 * nElementSize])) <= 0)
						{
							MemVectorCopyElement(
							    (char*)&(pValueBlock[iLocal * nElementSize]),
							    (char*)&(pValueBlock1[iLocal1 * nElementSize]),
							    nElementSize);

							// Mise a jour des index globaux et locaux
							iGlobal1++;
							iGlobal++;
							iLocal1++;
							iLocal++;

							// Push du block libre
							if (iLocal1 == nBlockSize or iGlobal1 == nSize)
								MemVectorPushFreeBlock(pHeadFreeBlock, pTailFreeBlock,
										       pValueBlock1);

							// Reactualisation des index locaux
							if (iLocal1 == nBlockSize)
								iLocal1 = 0;
							if (iLocal == nBlockSize)
								iLocal = 0;
						}
						else
						{
							MemVectorCopyElement(
							    (char*)&(pValueBlock[iLocal * nElementSize]),
							    (char*)&(pValueBlock2[iLocal2 * nElementSize]),
							    nElementSize);

							// Mise a jour des index globaux et locaux
							iGlobal2++;
							iGlobal++;
							iLocal2++;
							iLocal++;

							// Push du block libre
							if (iLocal2 == nBlockSize or iGlobal2 == nSize)
								MemVectorPushFreeBlock(pHeadFreeBlock, pTailFreeBlock,
										       pValueBlock2);

							// Reactualisation des index locaux
							if (iLocal2 == nBlockSize)
								iLocal2 = 0;
							if (iLocal == nBlockSize)
								iLocal = 0;
						}
					}

					// Recopie de la fin du premier paquet
					while (iGlobal1 < iLastGlobal1)
					{
						// Reactualisation des blocks de travail
						if (iLocal1 == 0)
							pValueBlock1 =
							    memHugeVector.pValueBlocks[iGlobal1 / nBlockSize];
						if (iLocal == 0)
						{
							// Pop d'un block libre
							pValueBlock =
							    MemVectorPopFreeBlock(pHeadFreeBlock, pTailFreeBlock);
							pTargetValueBlocks[iGlobal / nBlockSize] = pValueBlock;
						}

						// Copie de la valeur
						MemVectorCopyElement((char*)&(pValueBlock[iLocal * nElementSize]),
								     (char*)&(pValueBlock1[iLocal1 * nElementSize]),
								     nElementSize);

						// Mise a jour des index globaux et locaux
						iGlobal1++;
						iGlobal++;
						iLocal1++;
						iLocal++;

						// Push du block libre
						if (iLocal1 == nBlockSize or iGlobal1 == nSize)
							MemVectorPushFreeBlock(pHeadFreeBlock, pTailFreeBlock,
									       pValueBlock1);

						// Reactualisation des index locaux
						if (iLocal1 == nBlockSize)
							iLocal1 = 0;
						if (iLocal == nBlockSize)
							iLocal = 0;
					}

					// Recopie de la fin du second paquet
					while (iGlobal2 < iLastGlobal2)
					{
						// Reactualisation des blocks de travail
						if (iLocal2 == 0)
							pValueBlock2 =
							    memHugeVector.pValueBlocks[iGlobal2 / nBlockSize];
						if (iLocal == 0)
						{
							// Pop d'un block libre
							pValueBlock =
							    MemVectorPopFreeBlock(pHeadFreeBlock, pTailFreeBlock);
							pTargetValueBlocks[iGlobal / nBlockSize] = pValueBlock;
						}

						// Copie de la valeur
						MemVectorCopyElement((char*)&(pValueBlock[iLocal * nElementSize]),
								     (char*)&(pValueBlock2[iLocal2 * nElementSize]),
								     nElementSize);

						// Mise a jour des index globaux et locaux
						iGlobal2++;
						iGlobal++;
						iLocal2++;
						iLocal++;

						// Push du block libre
						if (iLocal2 == nBlockSize or iGlobal2 == nSize)
							MemVectorPushFreeBlock(pHeadFreeBlock, pTailFreeBlock,
									       pValueBlock2);

						// Reactualisation des index locaux
						if (iLocal2 == nBlockSize)
							iLocal2 = 0;
						if (iLocal == nBlockSize)
							iLocal = 0;
					}

					// Passage au blocs suivants
					assert(iGlobal1 == iLastGlobal1);
					assert(iGlobal2 == iLastGlobal2 or iLastGlobal2 == nSize);
					iGlobal1 += nMergeBlockNumber * nBlockSize;
					iGlobal2 += nMergeBlockNumber * nBlockSize;
				}
				assert(iGlobal == nSize);

				// On inverse le role des deux tableaux de vecteurs
				memHugeVector.pValueBlocks = pTargetValueBlocks;
				pTargetValueBlocks = pSourceValueBlocks;
				pSourceValueBlocks = memHugeVector.pValueBlocks;
				assert(pHeadFreeBlock != NULL);
				assert(pTailFreeBlock != NULL);

				// Doublement du nombre de blocks par paquet
				nMergeBlockNumber *= 2;
			}

			// Liberation
			assert(pHeadFreeBlock != NULL);
			DeleteCharArray(pHeadFreeBlock);
			DeleteCharArray(pTailFreeBlock);
			DeleteMemoryBlock(pTargetValueBlocks);
		}
	}
	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

void MemVector::CopyFrom(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			 const int nElementSize, const MemHugeVector& memSourceHugeVector, const int& nSourceSize,
			 const int& nSourceAllocSize)
{
	int nBlockNumber;
	int i;

	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	require(Check(memSourceHugeVector, nSourceSize, nSourceAllocSize, nBlockSize, nElementSize));

	// Cas particulier ou source egale cible
	if (memHugeVector.pValues == memSourceHugeVector.pValues)
	{
		assert(nSize == nSourceSize);
		assert(nAllocSize == nSourceAllocSize);
		return;
	}

	// Retaillage
	// Ce retaillage entraine une potentielle initialisation partielle a zero (si la nouvelle taille
	// est plus grande que l'ancienne)
	// Cette initialisation est inutile, car les valeur seront ecrasee par la recopie.
	// Une optimisation serait possible, mais un peu fastidieuse et moins maintenable
	SetSize(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nSourceSize);

	// Recopie des valeurs sources
	if (nSize > 0)
	{
		assert(memHugeVector.pValues != NULL);

		// Recopie des anciennes valeurs si un seul block
		if (nAllocSize <= nBlockSize)
		{
			assert(nSourceAllocSize <= nBlockSize);
			memcpy(memHugeVector.pValues, memSourceHugeVector.pValues, nSize * nElementSize);
		}
		else
		// Recopie de chaque blocks
		{
			nBlockNumber = ((nSize - 1) / nBlockSize) + 1;

			// Recopie integrale des premiers blocks
			for (i = 0; i < nBlockNumber - 1; i++)
				memcpy(memHugeVector.pValueBlocks[i], memSourceHugeVector.pValueBlocks[i],
				       nBlockSize * nElementSize);

			// Recopie partielle du dernier block
			if (nSize % nBlockSize > 0)
				memcpy(memHugeVector.pValueBlocks[nBlockNumber - 1],
				       memSourceHugeVector.pValueBlocks[nBlockNumber - 1],
				       (nSize % nBlockSize) * nElementSize);
			else
				memcpy(memHugeVector.pValueBlocks[nBlockNumber - 1],
				       memSourceHugeVector.pValueBlocks[nBlockNumber - 1], nBlockSize * nElementSize);
		}
	}
	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

void MemVector::ImportBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize, int nBlockSize, int nElementSize,
			     int nIndex, int nElementNumber, const char* cByteBuffer)
{
	int nSizeToCopy;
	int nLocalCopy;
	int nSourcePos;
	int nDestPos;

	require(0 <= nIndex and nIndex < nSize);
	require(nIndex + nElementNumber <= nSize);
	require(cByteBuffer != NULL);
	require(nElementSize * longint(nElementNumber) <= INT_MAX);

	// Cas mono-bloc
	if (nAllocSize <= nBlockSize)
		memcpy(&memHugeVector.pValues[nIndex * nElementSize], cByteBuffer,
		       (size_t)nElementNumber * nElementSize);
	else
	// Cas multi-blocs
	{
		nSizeToCopy = nElementNumber;
		nDestPos = nIndex * nElementSize;
		nSourcePos = 0;
		while (nSizeToCopy > 0)
		{
			// On ne copie pas plus que ce que le bloc courant peut contenir
			nLocalCopy = min(nBlockSize - (nDestPos % nBlockSize), nSizeToCopy);

			// Copie memoire
			memcpy(&memHugeVector.pValueBlocks[nDestPos / nBlockSize][nDestPos % nBlockSize],
			       &cByteBuffer[nSourcePos], (size_t)nLocalCopy * nElementSize);
			nSizeToCopy -= nLocalCopy;
			nSourcePos += nLocalCopy;
			nDestPos += nLocalCopy;
		}
	}
}

void MemVector::ExportBuffer(const MemHugeVector& memHugeVector, int nSize, int nAllocSize, int nBlockSize,
			     int nElementSize, int nIndex, int nElementNumber, char* cByteBuffer)
{
	require(cByteBuffer != NULL);
	require(nIndex + nElementNumber <= nSize);
	int nSizeToCopy;
	int nLocalCopy;
	int nSourcePos;
	int nDestPos;

	require(0 <= nIndex and nIndex < nSize);
	require(nIndex + nElementNumber <= nSize);
	require(cByteBuffer != NULL);
	require(nElementSize * longint(nElementNumber) <= INT_MAX);

	// Cas mono-bloc
	if (nAllocSize <= nBlockSize)
		memcpy(cByteBuffer, &memHugeVector.pValues[nIndex * nElementSize],
		       (size_t)nElementNumber * nElementSize);
	else
	{
		nSourcePos = nIndex * nElementSize;
		nSizeToCopy = nElementNumber;
		nDestPos = 0;
		while (nSizeToCopy > 0)
		{
			// On ne copie pas plus que ce que contient le bloc courant
			nLocalCopy = min(nSizeToCopy, nBlockSize - (nSourcePos % nBlockSize));
			memcpy(&cByteBuffer[nDestPos],
			       &memHugeVector.pValueBlocks[nSourcePos / nBlockSize][nSourcePos % nBlockSize],
			       (size_t)nLocalCopy * nElementSize);
			nSourcePos += nLocalCopy;
			nSizeToCopy -= nLocalCopy;
			nDestPos += nLocalCopy;
		}
	}
}

boolean MemVector::SetLargeSize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
				const int nElementSize, int nNewSize)
{
	boolean bOk;
	MemAllocErrorHandler fAllocErrorHandler;

	require(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	require(nNewSize >= 0);
	require(nNewSize <= INT_MAX / nElementSize);

	// Allocation en utilisant temporairement l'allocateur sans sortie fatale en cas d'erreur
	fAllocErrorHandler = MemGetAllocErrorHandler();
	MemSetAllocErrorHandler(NULL);

	// Retaillage standard
	SetSize(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nNewSize);
	assert(nSize <= nNewSize);
	bOk = (nSize == nNewSize);

	// On restitue le gestionnaire d'allocation precedent
	MemSetAllocErrorHandler(fAllocErrorHandler);

	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
	return bOk;
}

boolean MemVector::Check(const MemHugeVector& memHugeVector, const int& nSize, const int& nAllocSize,
			 const int nBlockSize, const int nElementSize)
{
	boolean bOk = true;
	bOk = bOk and nSize >= 0;
	bOk = bOk and nSize <= nAllocSize;
	bOk = bOk and nBlockSize > 0;
	bOk = bOk and nElementSize > 0;
	bOk = bOk and MemSegmentByteSize == (size_t)(nBlockSize * nElementSize);
	if (nAllocSize == 0)
		bOk = bOk and memHugeVector.pValues == NULL;
	else if (nAllocSize <= nBlockSize)
		bOk = bOk and memHugeVector.pValues != NULL;
	else
		bOk = bOk and memHugeVector.pValueBlocks != NULL;
	return bOk;
}

//////////////////////////////////////////////////////////////////////////////////
// Classe PointerVector

void PointerVector::SwapFrom(PointerVector* pvSource)
{
	PointerVector pvTmp;

	require(pvSource != NULL);

	// Recopie du vecteur source dans le vecteur temporaire
	pvTmp.nSize = pvSource->nSize;
	pvTmp.nAllocSize = pvSource->nAllocSize;
	pvTmp.pData = pvSource->pData;

	// Recopie du vecteur courant dans le vecteur source
	pvSource->nSize = nSize;
	pvSource->nAllocSize = nAllocSize;
	pvSource->pData = pData;

	// Recopie du vecteur temporaire dans le vecteur courant
	nSize = pvTmp.nSize;
	nAllocSize = pvTmp.nAllocSize;
	pData = pvTmp.pData;

	// Reinitialisation du vecteur temporaire pour eviter la destruction de son contenu
	pvTmp.nSize = 0;
	pvTmp.nAllocSize = 0;
	pvTmp.pData.pValues = NULL;
}

void PointerVector::CopyFrom(const PointerVector* ivSource)
{
	require(ivSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, ivSource->pData.hugeVector,
			    ivSource->nSize, ivSource->nAllocSize);
}

PointerVector* PointerVector::Clone() const
{
	PointerVector* ivClone;

	ivClone = new PointerVector;

	// Recopie
	ivClone->CopyFrom(this);
	return ivClone;
}

boolean PointerVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

longint PointerVector::GetUsedMemory() const
{
	return nAllocSize * sizeof(void*);
}
