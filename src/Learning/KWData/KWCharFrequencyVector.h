// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "CharVector.h"
#include "ALString.h"

//////////////////////////////////////////////////////////
// Classe KWCharFrequencyVector
// Vecteur des effectifs de caracteres, pour tous les caracteres possibles
class KWCharFrequencyVector : public Object
{
public:
	KWCharFrequencyVector();
	~KWCharFrequencyVector();

	//////////////////////////////////////////////////////////////
	// Acces aux effectifs des caracteres

	// Taille: nombre de caracteres possibles
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Initialisation de toutes les effctifs a une valeur donnees
	void InitializeFrequencies(int nFrequency);

	// Acces au ith caractere
	char GetCharAtIndex(int nIndex) const;

	// Mise a jour des effectif par caractere
	void SetFrequencyAt(char cValue, int nFrequency);
	int GetFrequencyAt(char cValue) const;

	// Modification de l'effectif d'un caractere
	void UpgradeFrequencyAt(char cValue, int nDeltaFrequency);

	// Test si tous les effectifs sont nuls
	boolean IsZero() const;

	// Effectif totral cumule sur tous les caracteres
	int GetTotalFrequency() const;

	///////////////////////////////////////////////////////////////
	// Operation sur les vecteur d'effectifs

	// Initialisation a partir d'une chaine de caracteres
	void InitializeFromString(const ALString& sValue);
	void UpgradeFromString(const ALString& sValue);

	// Initialisation a partir d'un vecteur de chaines de caracteres
	void InitializeFromStringVector(const StringVector* svStrings);
	void UpgradeFromStringVector(const StringVector* svStrings);

	// Initialisation a partir d'un buffer
	void InitializeFromBuffer(const CharVector* cvBuffer);
	void UpgradeFromBuffer(const CharVector* cvBuffer);

	// Methodes de comparaison, dont le resultat depend de la comparaison sur tous les caracteres possibles
	boolean IsEqual(const KWCharFrequencyVector* cfvComparedChars) const;
	boolean IsGreater(const KWCharFrequencyVector* cfvComparedChars) const;
	boolean IsGreaterOrEqual(const KWCharFrequencyVector* cfvComparedChars) const;
	boolean IsSmaller(const KWCharFrequencyVector* cfvComparedChars) const;
	boolean IsSmallerOrEqual(const KWCharFrequencyVector* cfvComparedChars) const;

	// Nombre de caracteres d'effectifs non nuls
	int GetUsedCharNumber() const;

	// Nombre de caracteres d'effectifs strictement plus grand qu'un seuil
	int GetFrequentCharNumber(int nFrequencyThreshold) const;

	// Filtrage en mettant a 0 les effectifs des caracteres dont l'effectif dans le filtre est strictement positif
	void FilterChars(const KWCharFrequencyVector* cfvFilteredChars);

	// Filtrage en mettant a 0 les effectifs des caracteres donc l'effectif n'atteint pas le seuil en parametre
	void FilterBelowFrequency(int nFrequencyThreshold);

	// Addition d'un autre vecteur d'effectif
	void Add(const KWCharFrequencyVector* cfvAddedChars);

	// Soustraction d'un autre vecteur d'effectif, qui doit plus petit
	void Substract(const KWCharFrequencyVector* cfvSubstractedChars);

	// Minimum entre les effectifs courant et ceux du vecteur en operande
	void Min(const KWCharFrequencyVector* cfvOtherChars);

	// Maximum entre les effectifs courant et ceux du vecteur en operande
	void Max(const KWCharFrequencyVector* cfvOtherChars);

	// On garde les effectifs courants uniquement pour les effectifs a 0 du vecteur en operande
	void Not(const KWCharFrequencyVector* cfvOtherChars);

	///////////////////////////////////////////////////////////////
	// Methodes de services standard

	// Copie a partir d'un vecteur source
	void CopyFrom(const KWCharFrequencyVector* cfvSource);

	// Duplication
	KWCharFrequencyVector* Clone() const;

	// Test d'integrite
	boolean Check() const override;

	// Affichage d'un resume sous forme d'un seul champs de type "{a:7, c:23, H:12, ...}"
	void Write(ostream& ost) const override;

	// Affichage detaille sous forme tabulaire pour tous les caracteres utilises
	void FullWrite(ostream& ost) const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nombre de carareteres
	static const int nCharNumber = 256;

	// Effectif par caractere
	IntVector ivCharFrequencies;

	// Effectif total
	int nTotalFrequency;
};

/////////////////////////////////////////////////////////////////
// Implementations en inline
