// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Vector.h"

class SNBIndexVector;

//////////////////////////////////////////////////////////
// Classe SNBIndexVector
// Vecteur d'index, de taille quelconque.
// Un index est un entier dont l'étendue est limitee 0 <= index < IndexSize,
// ce qui permet de le stocker avec un nombre de bit minimum par index,
// de facon a minimiser la memoire globale utilisee dans le vecteur, au prix de
// temps d'acces plus long.
// Les acces aux vecteurs sont controles par assertions.
class SNBIndexVector : public Object
{
public:
	// Constructeur
	SNBIndexVector();
	~SNBIndexVector();

	// Taille des index de valeur, a parametrer quand la taille du vecteur est nulle
	// Les index utilisable sont alors dans l'intervalles 0 <= index < IndexSize
	void SetValueIndexSize(int nValue);
	int GetValueIndexSize() const;

	// Taille du vecteur, a parametrer apres la taille max des index
	// Les retaillage ne sont pas autorises, hormis remise a la taille 0
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, int nValueIndex);
	int GetAt(int nIndex) const;

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const SNBIndexVector* snbivSource);

	// Duplication
	SNBIndexVector* Clone() const;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur rest de taille nulle)
	boolean SetLargeSize(int nValue);

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Nombre de bits par index de valeur
	int GetBitNumberPerValueIndex() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////////
	// TODO: a adapter si on veut accepter eventuellement des index a -1
	//   . prevoir une taille interne de stockage plus grande (nBitNumberPerValueIndex dimensionne pour
	//   nValueIndexSize+1) . changer les assertions dans les accesseurs pour tolerer les -1 . implementer les
	//   stockages des valeurs dans SetAt avec +1 et SetAt avec -1
	//
	// TODO: etendre pour les vecteurs sparses, qui doivent encoder une paire d'index (nInstanceIndex, nValueIndex)
	//  . possibilite: introduction d'un parametrage additionnele Set|GetInstanceIndexSize
	//  . extension des methode Set|GetAt en
	//     void SetPairAt(int nIndex, int nInstanceIndex, int nValueIndex);
	//     void GetPairAt(int nIndex, int& nInstanceIndex, int& nValueIndex);
	//  . memorization du nBitNumberPerInstanceIndex et du total nBitNumberPerIndexPair
	//  . utilisation du nBitNumberPerIndexPair pour stocker chaque paire d'index et retouver ensuite facilement
	//    les deux index, stockes de facon colocalises
	//
	// TODO:
	//  . exploitation des methodes Import|ExportBuffer de LongintVector (non encore implementees) pour optimiser
	//  les IO . mettre la classe SNBDataTableBinarySliceSetBuffer en friend pour qu'elle accede au vecteur
	//  livStorage interne
	//    afin d'optimiser les IO

	// Nombre de bit par byte
	static const int nBitNumberPerByte = 8;

	// Nombre de bit par unite de stockage
	static const int nBitNumberPerStorageUnit = nBitNumberPerByte * sizeof(longint);

	// Taille du vecteur
	int nSize;

	// Taille des index de valeur
	int nValueIndexSize;

	// Nombre de bits par index de valeur
	int nBitNumberPerValueIndex;

	// Vecteur de longint stockant tous les index
	// On utilise des longint pour pouvoir faire des operations sur des mots de 8 bytes, efficaces en mode 64 bits
	LongintVector lvStorage;
};

// Type unsigned longint pour l'imlementation de la classe
typedef unsigned long long int ulongint;

/////////////////////////////////////////////////////////////////
// Implementations en inline

// Classe SNBIndexVector

inline SNBIndexVector::SNBIndexVector()
{
	nSize = 0;
	nValueIndexSize = 0;
	nBitNumberPerValueIndex = 0;
	assert(sizeof(ulongint) == sizeof(longint));
}

inline SNBIndexVector::~SNBIndexVector() {}

inline int SNBIndexVector::GetSize() const
{
	return nSize;
}

inline void SNBIndexVector::SetAt(int nIndex, int nValueIndex)
{
	longint lTotalBitNumber;
	ulongint ulBitMask1;
	ulongint ulBitMask2;
	int nStartStorageIndex;
	int nStartBitIndex;
	ulongint ulStorage1;
	ulongint ulStorage2;
	ulongint ulValueIndex1;
	ulongint ulValueIndex2;
	int nBitNumber1;
	int nBitNumber2;

	require(0 <= nIndex and nIndex < GetSize());
	require(0 <= nValueIndex and nValueIndex < GetValueIndexSize());

	// Recherche de l'index de stockage de depart ou commence le stockage de l'index
	lTotalBitNumber = longint(nIndex) * nBitNumberPerValueIndex;
	nStartStorageIndex = int(lTotalBitNumber / nBitNumberPerStorageUnit);
	assert(0 <= nStartStorageIndex and nStartStorageIndex < lvStorage.GetSize());

	// Recherche de l'index du bit de depart
	nStartBitIndex = lTotalBitNumber % nBitNumberPerStorageUnit;

	// Cas ou tout tient sur une unite de stockage
	if (nStartBitIndex + nBitNumberPerValueIndex <= nBitNumberPerStorageUnit)
	{
		// Calcul du bitmask avec des 1 positionne sur la longueur de l'index (2^nBitNumberPerIndex-1) a droite
		ulBitMask1 = (1 << nBitNumberPerValueIndex) - 1;

		// Decallage pour faire demarer le bitmask en selon l'index du bit de depart a gauche
		ulBitMask1 = ulBitMask1 << (nBitNumberPerStorageUnit - nStartBitIndex - nBitNumberPerValueIndex);

		// Lecture de l'unite de stockage pour retrouver les informations existantes
		ulStorage1 = (ulongint)lvStorage.GetAt(nStartStorageIndex);

		// On positionne a 0 les bits a modifier
		ulStorage1 &= ~ulBitMask1;

		// On peut maintenant positionner les bits selon la valeur en les decallant comme pour le bit mask
		ulValueIndex1 = (ulongint)nValueIndex;
		ulValueIndex1 = ulValueIndex1 << (nBitNumberPerStorageUnit - nStartBitIndex - nBitNumberPerValueIndex);

		// On les prend en compte
		ulStorage1 |= ulValueIndex1;

		// On memorise la nouvelle valeur
		lvStorage.SetAt(nStartStorageIndex, ulStorage1);
	}
	// Cas ou la valeur et a la frontiere de deux unite de stockage
	else
	{
		assert(nStartBitIndex + nBitNumberPerValueIndex > nBitNumberPerStorageUnit);
		assert(nStartBitIndex + nBitNumberPerValueIndex <= 2 * nBitNumberPerStorageUnit);

		// Calcul des bitmasks avec des 1 positionne sur la longueur de l'index (2^nBitNumberPerIndex-1)
		// sur les deux unite de stockage consecutives
		nBitNumber2 = nStartBitIndex + nBitNumberPerValueIndex - nBitNumberPerStorageUnit;
		nBitNumber1 = nBitNumberPerValueIndex - nBitNumber2;
		ulBitMask1 = (1 << nBitNumber1) - 1;
		ulBitMask2 = (1 << nBitNumber2) - 1;
		ulBitMask2 = ulBitMask2 << (nBitNumberPerStorageUnit - nBitNumber2);

		// Lecture des unites de stockage pour retrouver les informations existantes
		ulStorage1 = (ulongint)lvStorage.GetAt(nStartStorageIndex);
		ulStorage2 = (ulongint)lvStorage.GetAt(nStartStorageIndex + 1);

		// On positionne a 0 les bits a modifier
		ulStorage1 &= ~ulBitMask1;
		ulStorage2 &= ~ulBitMask2;

		// On peut maintenant positionner les bits selon la valeur
		// Les bit de poids fort sont a gauche (unite 1) et ceux de poids faible a droite (unite 2)
		ulValueIndex1 = (ulongint)nValueIndex;
		ulValueIndex2 = ulValueIndex1, ulValueIndex1 = ulValueIndex1 >> nBitNumber2;
		ulValueIndex2 = ulValueIndex2 << (nBitNumberPerStorageUnit - nBitNumber2);

		// On les prend en compte en les ajoutant
		ulStorage1 |= ulValueIndex1;
		ulStorage2 |= ulValueIndex2;

		// On memorise la nouvelle valeur
		lvStorage.SetAt(nStartStorageIndex, ulStorage1);
		lvStorage.SetAt(nStartStorageIndex + 1, ulStorage2);
	}
	ensure(GetAt(nIndex) == nValueIndex);
}

inline int SNBIndexVector::GetAt(int nIndex) const
{
	int nValueIndex;
	longint lTotalBitNumber;
	ulongint ulBitMask1;
	ulongint ulBitMask2;
	int nStartStorageIndex;
	int nStartBitIndex;
	ulongint ulStorage1;
	ulongint ulStorage2;
	int nBitNumber1;
	int nBitNumber2;

	require(0 <= nIndex and nIndex < GetSize());

	// Recherche de l'index de stockage de depart ou commence le stockage de l'index
	lTotalBitNumber = longint(nIndex) * nBitNumberPerValueIndex;
	nStartStorageIndex = int(lTotalBitNumber / nBitNumberPerStorageUnit);
	assert(0 <= nStartStorageIndex and nStartStorageIndex < lvStorage.GetSize());

	// Recherche de l'index du bit de depart
	nStartBitIndex = lTotalBitNumber % nBitNumberPerStorageUnit;

	// Cas ou tout tient sur une unite de stockage
	if (nStartBitIndex + nBitNumberPerValueIndex <= nBitNumberPerStorageUnit)
	{
		// Calcul du bitmask avec des 1 positionne sur la longueur de l'index (2^nBitNumberPerIndex-1) a droite
		ulBitMask1 = (1 << nBitNumberPerValueIndex) - 1;

		// Decallage pour faire demarer le bitmask en selon l'index du bit de depart a gauche
		ulBitMask1 = ulBitMask1 << (nBitNumberPerStorageUnit - nStartBitIndex - nBitNumberPerValueIndex);

		// Lecture de l'unite de stockage pour retrouver les informations existantes
		ulStorage1 = (ulongint)lvStorage.GetAt(nStartStorageIndex);

		// On garde les bit correspondant au masque
		ulStorage1 &= ulBitMask1;

		// On peut maintenant positionner les bits selon la valeur en les decallant de facon inverse
		ulStorage1 = ulStorage1 >> (nBitNumberPerStorageUnit - nStartBitIndex - nBitNumberPerValueIndex);

		// On memorise la valeur resultat dans un entier
		nValueIndex = (int)ulStorage1;
	}
	// Cas ou la valeur et a la frontiere de deux unite de stockage
	else
	{
		assert(nStartBitIndex + nBitNumberPerValueIndex > nBitNumberPerStorageUnit);
		assert(nStartBitIndex + nBitNumberPerValueIndex <= 2 * nBitNumberPerStorageUnit);

		// Calcul des bitmasks avec des 1 positionne sur la longueur de l'index (2^nBitNumberPerIndex-1)
		// sur les deux unite de stockage consecutives
		nBitNumber2 = nStartBitIndex + nBitNumberPerValueIndex - nBitNumberPerStorageUnit;
		nBitNumber1 = nBitNumberPerValueIndex - nBitNumber2;
		ulBitMask1 = (1 << nBitNumber1) - 1;
		ulBitMask2 = (1 << nBitNumber2) - 1;
		ulBitMask2 = ulBitMask2 << (nBitNumberPerStorageUnit - nBitNumber2);

		// Lecture des unites de stockage pour retrouver les informations existantes
		ulStorage1 = (ulongint)lvStorage.GetAt(nStartStorageIndex);
		ulStorage2 = (ulongint)lvStorage.GetAt(nStartStorageIndex + 1);

		// On garde les bit correspondant au masque
		ulStorage1 &= ulBitMask1;
		ulStorage2 &= ulBitMask2;

		// On peut maintenant positionner les bits selon la valeur en les decallant de facon inverse
		ulStorage1 = ulStorage1 << nBitNumber2;
		ulStorage2 = ulStorage2 >> (nBitNumberPerStorageUnit - nBitNumber2);

		// On calul le result en additionnant les bits de poids fort et de poid faible
		// puis en castant en int
		ulStorage1 |= ulStorage2;
		nValueIndex = (int)ulStorage1;
	}
	ensure(0 <= nValueIndex and nValueIndex < GetValueIndexSize());
	return nValueIndex;
}
