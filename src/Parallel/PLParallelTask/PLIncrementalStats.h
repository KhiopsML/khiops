// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "Vector.h"
#include "PLSharedObject.h"

class PLIncrementalStats;
class PLSharedIncrementalStats;

////////////////////////////////////////////////////////////////////////////
// Classe PLIncrementalStats
// Calcule et affichage de statistiques d'une distribution de valeurs (moyenne, ecart-type, min et max)
class PLIncrementalStats : public Object
{
public:
	// Constructeurs
	PLIncrementalStats();
	~PLIncrementalStats();

	// Description de l'indicateur
	void SetDescription(const ALString sName);

	// Ajout d'une valeur
	void AddValue(double dValue);

	// Mise a jour a partir d'un autre distribution
	void AddStats(const PLIncrementalStats* stats);

	// Acces aux statistiques
	double GetMin() const;
	double GetMax() const;
	double GetMean() const;
	double GetStdDev() const;
	double GetSum() const;
	int GetValueNumber() const;

	// Ecriture
	void Write(ostream& ost) const override;

	// Remise a zero
	void Reset();

protected:
	// Arrondi la valeur a deux chiffres apres la virgule
	double ToTwoDigits(double dValue) const;

	// Attributs
	double dMin;
	double dMax;
	double dTotal;
	int nValueNumber;
	double dTotalSquare;
	ALString sDescription;

	friend class PLSharedIncrementalStats;
};

////////////////////////////////////////////////////////////////////////////
// Classe PLSharedIncrementalStats
// Serialisation de la classe PLIncrementalStats
class PLSharedIncrementalStats : public PLSharedObject
{
public:
	// Constructeur
	PLSharedIncrementalStats();
	~PLSharedIncrementalStats();

	// Acces au bufferedFile
	void SetStats(PLIncrementalStats* stat);
	PLIncrementalStats* GetStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
