// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHMODLHistogramAnalysisStats;
class MHMODLHistogramStats;
class PLShared_MODLHistogramAnalysisStats;
class PLShared_MODLHistogramStats;

#include "Object.h"
#include "KWDiscretizerMODL.h"
#include "PLSharedObject.h"

/////////////////////////////////////////////////////////////////////////////////
// Resultats de discretisation non supervise MODL
// avec la liste des histogrammes obtenus ainsi que les indicateurs utiles
// pour l'interpretation
// On ne memorise que ce qui est necessaire pur les rapports en sortie
class MHMODLHistogramAnalysisStats : public KWMODLHistogramResults
{
public:
	// Constructeur
	MHMODLHistogramAnalysisStats();
	~MHMODLHistogramAnalysisStats();

	// Nom du discretiseur a l'origine des results
	const ALString GetDiscretizerName() const override;

	////////////////////////////////////////////////////////////////////
	// Specification des histogrammes

	// Acces aux tableau des histogrammes (MHMODLHistogramStats) optimises suite a la discretisation
	// Les histogrammes optimises sont ordonnes par niveau hierarchique croissant, de 0 au niveau optimal
	// Plus eventuellement un dernier histogramme "raw", en derniere position
	// Le tableau et son contenu appartiennent a l'appele
	ObjectArray* GetHistograms();

	// Nombre d'histogrammes
	int GetHistogramNumber() const;

	// Acces a un histogramme
	const MHMODLHistogramStats* GetHistogramAt(int nIndex) const;

	// Nombre d'histograme interpretables
	void SetInterpretableHistogramNumber(int nValue);
	int GetInterpretableHistogramNumber() const;

	// Acces a l'histogramme interpretable le plus precise
	const MHMODLHistogramStats* GetMostAccurateInterpretableHistogram() const;

	////////////////////////////////////////////////////////////////////
	// Indicateurs associes a l'ensemble des histogrammes

	// Bornes des histogrammes, qui ne sont pas necessairement les valeur min et max du jeux de donnees
	Continuous GetDomainLowerBound() const override;
	Continuous GetDomainUpperBound() const override;

	// Valeur du central bin exponent pour les premiers histogrammes
	void SetCentralBinExponent(int nValue);
	int GetCentralBinExponent() const;

	// Valeur du central bin exponent pour le dernier histogramme
	void SetLastCentralBinExponent(int nValue);
	int GetLastCentralBinExponent() const;

	// Nombre d'intervalles singuliers en moins par rapport a l'histogramme le plus fin
	void SetRemovedSingularIntervalsNumber(int nValue);
	int GetRemovedSingularIntervalsNumber() const;

	// Epsilon de troncature
	void SetTruncationEpsilon(double dValue);
	double GetTruncationEpsilon() const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Ecriture d'un rapport JSON
	void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) const override;

	// Verification de la validite
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tableau des histogrammes
	ObjectArray oaHistograms;
	int nInterpretableHistogramNumber;

	// Indicateurs sur l'ensmeble des histogrammes
	int nCentralBinExponent;
	int nLastCentralBinExponent;
	int nRemovedSingularIntervalsNumber;
	double dTruncationEpsilon;
};

/////////////////////////////////////////////////////////////////////////////////
// Histogramme MODL issue de la discretisation non supervise MODL
// On ne memorise que la structure des intervalles et les indicateurs necessaires
// au rapports en sortie, dans un but de stockage parcimonieux des resultstd e modelisation
class MHMODLHistogramStats : public Object
{
public:
	// Constructeur
	MHMODLHistogramStats();
	~MHMODLHistogramStats();

	////////////////////////////////////////////////////////////////////
	// Specification des intervalles

	// Nombre d'intervalles
	// Entraine le dimensionnement des vecteur de bornes et d'effectifs
	void SetIntervalNumber(int nValue);
	int GetIntervalNumber() const;

	// Bornes des intervalles
	ContinuousVector* GetIntervalBounds();

	// Effectif des intervalles
	IntVector* GetIntervalFrequencies();

	// Acces aux catacteristiques des intervalles via des methodes const
	Continuous GetIntervalLowerBoundAt(int nIndex) const;
	Continuous GetIntervalUpperBoundAt(int nIndex) const;
	int GetIntervalFrequencyAt(int nIndex) const;

	// Borne inf du domaine numerique, celle du premier intervalle
	Continuous GetDomainLowerBound() const;

	// Borne sup du domaine numerique, celle du dernier intervalle
	Continuous GetDomainUpperBound() const;

	// Effectif total
	int ComputeTotalFrequency() const;

	////////////////////////////////////////////////////////////////////
	// Indicateurs associe a l'histogramme (cf. classe MHHistogram)

	// Granularite
	void SetGranularity(int nValue);
	int GetGranularity() const;

	// Nombre d'intervalles de type peak
	void SetPeakIntervalNumber(int nValue);
	int GetPeakIntervalNumber() const;

	// Nombre d'intervalles de type spike
	void SetSpikeIntervalNumber(int nValue);
	int GetSpikeIntervalNumber() const;

	// Nombre d'intervalles vides
	void SetEmptyIntervalNumber(int nValue);
	int GetEmptyIntervalNumber() const;

	// Level normalise
	void SetNormalizedLevel(double dValue);
	double GetNormalizedLevel() const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Ecriture des bornes et effectif des intervalle dans rapport JSON
	void WriteJSONIntervalReport(JSONFile* fJSON) const;

	// Verification de la validite
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Definitions des intervalles
	ContinuousVector cvIntervalBounds;
	IntVector ivIntervalFrequencies;

	// Indicateurs de l'histogramme
	int nGranularity;
	int nPeakIntervalNumber;
	int nSpikeIntervalNumber;
	int nEmptyIntervalNumber;
	double dNormalizedLevel;
};

////////////////////////////////////////////////////////////
// Classe PLShared_MODLHistogramAnalysisStats
//	 Serialisation de la classe MHMODLHistogramAnalysisStats
class PLShared_MODLHistogramAnalysisStats : public PLSharedObject
{
public:
	// Constructeur
	PLShared_MODLHistogramAnalysisStats();
	~PLShared_MODLHistogramAnalysisStats();

	// Acces a l'objetb serialise
	void SetMODLHistogramAnalysisStats(MHMODLHistogramAnalysisStats* histogramAnalysisStats);
	MHMODLHistogramAnalysisStats* GetMODLHistogramAnalysisStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe PLShared_MODLHistogramStats
//	 Serialisation de la classe MHMODLHistogramStats
class PLShared_MODLHistogramStats : public PLSharedObject
{
public:
	// Constructeur
	PLShared_MODLHistogramStats();
	~PLShared_MODLHistogramStats();

	// Acces a l'objetb serialise
	void SetMODLHistogramStats(MHMODLHistogramStats* histogramStats);
	MHMODLHistogramStats* GetMODLHistogramStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
