// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDescriptiveStats;
class KWDescriptiveContinuousStats;
class KWDescriptiveSymbolStats;
class PLShared_DescriptiveStats;
class PLShared_DescriptiveContinuousStats;
class PLShared_DescriptiveSymbolStats;

#include "KWLearningReport.h"
#include "PLSharedObject.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
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

	// (Re)Initialisation des statistiques descriptives
	virtual void Init();

	// Dans cette classe et ses sous-classe, la methode ComputeStats doit etre appelee
	// avec une table de tuples ayant l'attribut courant en premier
	// Donc on marque le ComputeStats herite de KWLearningReport comme `final` et on le fait planter
	// avec des assert(false)
	boolean ComputeStats() override final;
	virtual boolean ComputeStats(const KWTupleTable* tupleTable);

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Nombre de valeurs differentes
	int GetValueNumber() const;

	// Nombre des valeurs manquantes
	int GetMissingValueNumber() const;

	// Nombre des valeurs sparse manquantes
	int GetSparseMissingValueNumber() const;

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
	int nMissingValueNumber;
	int nSparseMissingValueNumber;

	// Access privee a la classe de serialisation
	friend class PLShared_DescriptiveStats;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveContinuousStats
// Calcul des statistiques descriptives d'un attribut de type Continuous
class KWDescriptiveContinuousStats : public KWDescriptiveStats
{
public:
	// Constructeur et destructeur
	KWDescriptiveContinuousStats();
	~KWDescriptiveContinuousStats();

	//////////////////////////////////////////
	// Initialisation

	// (Re)Initialisation des statistiques descriptives
	void Init() override;

	// Calcul des statistiques descriptives a partir de la base de tuple
	// Doit positionner l'indicateur a vrai en fin de calcul
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Statistique descriptives classiques
	Continuous GetMin() const;
	Continuous GetMax() const;
	Continuous GetMean() const;
	Continuous GetStandardDeviation() const;

	// Ecriture d'un rapport destine a rentrer dans un rapport englobant
	void WriteReport(ostream& ost) const override;

	// Rapport synthetique destine a rentrer dans un tableau englobant
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) const override;

	////////////////////////////////////////////////
	// Services divers

	// Duplication (retourne un objet generique)
	KWDescriptiveStats* Clone() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	Continuous cMin;
	Continuous cMax;
	Continuous cMean;
	Continuous cStandardDeviation;

	// Acces privee a la classe de serialisation
	friend class PLShared_DescriptiveContinuousStats;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDescriptiveSymbolStats
// Calcul des statistiques descriptives d'un attribut de type Symbol
class KWDescriptiveSymbolStats : public KWDescriptiveStats
{
public:
	// Constructeur
	KWDescriptiveSymbolStats();
	~KWDescriptiveSymbolStats();

	//////////////////////////////////////////
	// Initialisation

	// (Re)Initialisation des statistiques descriptives
	void Init() override;

	// Calcul des statistiques descriptives a partir de la base de tuple
	// Doit positionner l'indicateur a vrai en fin de calcul
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	/////////////////////////////////////////////////////
	// Acces aux statistiques calculees
	// Accessible uniquement si statistiques calculees

	// Entropie initiale
	double GetEntropy() const;

	// Mode, et sa frequence
	Symbol& GetMode() const;
	int GetModeFrequency() const;

	// Frequence totale
	int GetTotalFrequency() const;

	// Ecriture d'un rapport destine a rentrer dans un rapport englobant
	void WriteReport(ostream& ost) const override;

	// Rapport synthetique destine a rentrer dans un tableau englobant
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) const override;

	///////////////////////////////////////////
	// Services Divers

	// Duplication (retourne un objet generique)
	KWDescriptiveStats* Clone() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	double dEntropy;
	mutable Symbol sMode;
	int nModeFrequency;
	int nTotalFrequency;

	// Acces prive a la classe de serialisation
	friend class PLShared_DescriptiveSymbolStats;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DescriptiveStats
// Serialisation de la classe KWDescriptiveStats
class PLShared_DescriptiveStats : public PLShared_LearningReport
{
public:
	// Constructeur
	PLShared_DescriptiveStats();
	~PLShared_DescriptiveStats();

	// Reimplementation de l'interface PLSerialiser
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;
	void DeserializeObject(PLSerializer* serializer, Object* object) const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
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

	// Reimplementation de l'interface PLSerialiser
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;
	void DeserializeObject(PLSerializer* serializer, Object* object) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode Create de PLSerializer
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
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

	// Reimplementation de l'interface PLSerialiser
	void SerializeObject(PLSerializer* serializer, const Object* object) const override;
	void DeserializeObject(PLSerializer* serializer, Object* object) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode Create de PLSerializer
	Object* Create() const override;
};
