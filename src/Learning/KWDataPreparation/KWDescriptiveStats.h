// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDescriptiveStats;
class KWDescriptiveContinuousStats;
class KWDescriptiveSymbolStats;
class PLShared_DescriptiveContinuousStats;
class PLShared_DescriptiveSymbolStats;

#include "KWLearningReport.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////
// Classe KWDescriptiveStats
// Service generique de calcul des statistiques descriptives d'un attribut
class KWDescriptiveStats : public KWLearningReport
{
public:
	// Constructeur et destructeur
	KWDescriptiveStats();
	~KWDescriptiveStats();

	// Acces au nom de l'attribut considere
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	// Nombre de valeurs differentes
	int GetValueNumber() const;

	// Dans cette classe et ses sous-classe, la methode ComputeStats doit etre appelee
	// avec une table de tuples ayant l'attribut courant en premier
	boolean ComputeStats() final;
	virtual boolean ComputeStats(const KWTupleTable* tupleTable) = 0;

	// (Re)Initialisation des statistiques descriptives
	virtual void Init();

	// Duplication
	virtual KWDescriptiveStats* Clone() const = 0;

	// Criteres de tri permettant de trier differents objets d'un rapport
	const ALString GetSortName() const override;
	double GetSortValue() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	ALString sAttributeName;
	int nValueNumber;
	friend class PLShared_DescriptiveStats;
};

///////////////////////////////////////////////////////
// Classe KWDescriptiveContinuousStats
// Calcul des statistiques descriptives d'un attribut de type Continuous
class KWDescriptiveContinuousStats : public KWDescriptiveStats
{
public:
	// Constructeur et destructeur
	KWDescriptiveContinuousStats();
	~KWDescriptiveContinuousStats();

	// Calcul des statistiques descriptives a partir de la base de tuple
	// Doit positionner l'indicateur a vrai en fin de calcul
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	// (Re)Initialisation des statistiques descriptives
	void Init() override;

	// Duplication
	KWDescriptiveStats* Clone() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Statistique descriptives classiques
	Continuous GetMin() const;
	Continuous GetMax() const;
	Continuous GetMean() const;
	Continuous GetStandardDeviation() const;
	int GetMissingValueNumber() const;

	// Ecriture d'un rapport destine a rentrer dans un rapport englobant
	void WriteReport(ostream& ost) override;

	// Rapport synthetique destine a rentrer dans un tableau englobant
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	// Ecriture d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	Continuous cMin;
	Continuous cMax;
	Continuous cMean;
	Continuous cStandardDeviation;
	int nMissingValueNumber;
	friend class PLShared_DescriptiveContinuousStats;
};

///////////////////////////////////////////////////////
// Classe KWDescriptiveSymbolStats
// Calcul des statistiques descriptives d'un attribut de type Symbol
class KWDescriptiveSymbolStats : public KWDescriptiveStats
{
public:
	// Constructeur et destructeur
	KWDescriptiveSymbolStats();
	~KWDescriptiveSymbolStats();

	// Calcul des statistiques descriptives a partir de la base de tuple
	// Doit positionner l'indicateur a vrai en fin de calcul
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	// (Re)Initialisation des statistiques descriptives
	void Init() override;

	// Duplication
	KWDescriptiveStats* Clone() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Statistique descriptives classiques
	// Entropie initiale
	double GetEntropy() const;

	// Mode, et sa frequence
	Symbol& GetMode() const;
	int GetModeFrequency() const;

	// Frequence totale
	int GetTotalFrequency() const;

	// Ecriture d'un rapport destine a rentrer dans un rapport englobant
	void WriteReport(ostream& ost) override;

	// Rapport synthetique destine a rentrer dans un tableau englobant
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	// Ecriture d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	double dEntropy;
	mutable Symbol sMode;
	int nModeFrequency;
	int nTotalFrequency;
	friend class PLShared_DescriptiveSymbolStats;
};

///////////////////////////////////////////////////////
// Classe PLShared_DescriptiveStats
// Serialisation de la classe KWDescriptiveStats
class PLShared_DescriptiveStats : public PLShared_LearningReport
{
public:
	// Constructeur
	PLShared_DescriptiveStats();
	~PLShared_DescriptiveStats();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_DescriptiveContinuousStats
// Serialisation de la classe KWDescriptiveContinuousStats
class PLShared_DescriptiveContinuousStats : public PLShared_DescriptiveStats
{
public:
	// Constructeur
	PLShared_DescriptiveContinuousStats();
	~PLShared_DescriptiveContinuousStats();

	// Acces aux statistiques
	void SetDescriptiveStats(KWDescriptiveContinuousStats* descriptiveStats);
	KWDescriptiveContinuousStats* GetDescriptiveStats();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_DescriptiveSymbolStats
// Serialisation de la classe KWDescriptiveSymbolStats
class PLShared_DescriptiveSymbolStats : public PLShared_DescriptiveStats
{
public:
	// Constructeur
	PLShared_DescriptiveSymbolStats();
	~PLShared_DescriptiveSymbolStats();

	// Acces aux statistiques
	void SetDescriptiveStats(KWDescriptiveSymbolStats* descriptiveStats);
	KWDescriptiveSymbolStats* GetDescriptiveStats();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
