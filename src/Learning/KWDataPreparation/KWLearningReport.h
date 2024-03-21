// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWLearningReport;
class KWDataPreparationStats;
class PLShared_LearningReport;
class PLShared_DataPreparationStats;

#include "FileService.h"
#include "KWLearningSpec.h"
#include "KWTupleTable.h"
#include "KWDataGridStats.h"
#include "TSV.h"
#include "JSONFile.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////
// Classe KWLearningReport
// Services generiques pour la production de rapport
// concernant les statistiques et l'apprentissage
class KWLearningReport : public KWLearningService
{
public:
	// Constructeur
	KWLearningReport();
	~KWLearningReport();

	// Calcul de statistiques (une fois la base chargee en memoire)
	// Doit positionner l'indicateur en fin de calcul
	// Renvoie false si erreur ou interruption utilisateur
	// Renvoie true si calcul effectue completement
	virtual boolean ComputeStats();

	// Indique si les stats sont calculees
	boolean IsStatsComputed() const;

	////////////////////////////////////////////////////////
	// Gestion du rapport general

	// Ecriture d'un rapport
	void WriteReportFile(const ALString& sFileName);
	virtual void WriteReport(ostream& ost);

	// Rapport synthetique destine a rentrer dans un tableau
	// Ces methodes ne doivent pas creer de retour charriot ('\n') en fin de ligne, de facon a permettre
	// leur reimplementation. Ces retours charriot sont a generer par les methodes qui les utilisent.
	virtual void WriteHeaderLineReport(ostream& ost);
	virtual void WriteLineReport(ostream& ost);

	// Parametrage de la prise en compte dans les rapports (par defaut: true)
	virtual boolean IsReported() const;
	virtual boolean IsLineReported() const;

	// Criteres de tri permettant de trier differents objets d'un rapport
	virtual const ALString GetSortName() const;
	virtual double GetSortValue() const;

	// Comparaison sur le nom (par defaut: base sur le SortName directement)
	virtual int CompareName(const KWLearningReport* otherReport) const;

	// Comparaison sur la valeur, puis sur le nom (par defaut: base sur la SortValue et le SortName directement)
	virtual int CompareValue(const KWLearningReport* otherReport) const;

	// Identifiant
	void SetIdentifier(const ALString& sValue);
	const ALString& GetIdentifier() const;

	// Calcul d'identifiants base sur les rangs suite a un tri de tableau de rapport par SortValue
	void ComputeRankIdentifiers(ObjectArray* oaLearningReports);

	// Rapport synthetique pour un tableau de stats
	virtual void WriteArrayLineReport(ostream& ost, const ALString& sTitle, ObjectArray* oaLearningReports);

	// Rapport detaille pour un tableau de stats
	virtual void WriteArrayReport(ostream& ost, const ALString& sTitle, ObjectArray* oaLearningReports);

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture du contenu d'un rapport JSON
	virtual void WriteJSONFields(JSONFile* fJSON);

	// Ecriture d'un rapport JSON
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	virtual void WriteJSONReport(JSONFile* fJSON);
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey);

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	// On distingue les champs de resume, synthetique,
	// des champs de detail, pour avoir l'information complete
	virtual void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary);

	// Parametrage de la prise en compte dans les rapports
	// (par defaut: true si Summary=true, IsReported sinon)
	virtual boolean IsJSONReported(boolean bSummary) const;

	// Ecriture d'un tableau de rapport JSON
	virtual void WriteJSONArrayReport(JSONFile* fJSON, const ALString& sArrayKey, ObjectArray* oaLearningReports,
					  boolean bSummary);

	// Ecriture d'un dictionnaire de rapport JSON, chacun identifie par son Identifier
	virtual void WriteJSONDictionaryReport(JSONFile* fJSON, const ALString& sDictionaryKey,
					       ObjectArray* oaLearningReports, boolean bSummary);

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	boolean bIsStatsComputed;
	ALString sIdentifier;
	friend class PLShared_LearningReport;
};

// Fonctions de comparaison globale ou sur le nom seulement
int KWLearningReportCompareSortName(const void* elem1, const void* elem2);
int KWLearningReportCompareSortValue(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////
// Classe KWDataPreparationStats
// Services generiques pour la memorisation de calculs
// statistiques ou de preparation des donnees pour un
// attribut ou un agregat
class KWDataPreparationStats : public KWLearningReport
{
public:
	// Constructeur
	KWDataPreparationStats();
	~KWDataPreparationStats();

	// Dans cette classe et ses sous-classe, la methode ComputeStats doit etre appelee
	// avec une table de tuples
	boolean ComputeStats() final;
	virtual boolean ComputeStats(const KWTupleTable* tupleTable);

	////////////////////////////////////////////////////////////////////////////////
	// Specification de la preparation, pour un attribut ou un ensemble d'attributs
	// Attention: une fois prepare, une grille de preparation peut contenir moins
	// d'attributs que le nombre d'attributs initiaux (cas non informatif).
	// A redefinir dans les sous-classes

	// Nombre d'attributs a utiliser
	virtual int GetAttributeNumber() const = 0;

	// Parametrage des attributs
	virtual const ALString& GetAttributeNameAt(int nIndex) const = 0;

	// Comparaison sur le nom, basee sur l'ensemble des attributs
	int CompareName(const KWLearningReport* otherReport) const override;

	//////////////////////////////////////////////////////////////////////
	// Resultats de la preparation de donnees
	// Les donnees ci-dessous doivent etre alimentee dans les sous-classes
	// dans la methode ComputeStats

	// Statistiques liees a la preparation de l'attribut au moyen d'une grille
	// Il s'agit du partitionnement conjoint (groupage ou discretisation)
	// de l'attribut cible et de(s) attribut(s) source(s).
	// Retourne NULL si pas de preparation interessante
	// Memoire: la grille retournee appartiennent a l'appele
	KWDataGridStats* GetPreparedDataGridStats() const;

	// Nombre d'attributs entrant dans la preparation
	// Valeur deduite de la grille de preparation
	int GetPreparedAttributeNumber() const;

	// Level de la grille de preparation
	void SetLevel(double dValue);
	double GetLevel() const;

	// Indique si la variable est informative
	// Base sur le SortValue plutot que sur le Level, pour pouvoir etre utilisable
	// de facon generique, comme pour les paires de variables
	boolean IsInformative() const;

	// Criteres de tri a redefinir dans les sous-classe
	// Il s'agit par defaut du Level par defaut, mais cela peut etre specialise
	// comme dans le cas du DeltaLevel pour les paires
	double GetSortValue() const override;

	/////////////////////////////////////////////////////////
	// Structure des couts dans le cas de traitement MODL

	// Cout de construction
	void SetConstructionCost(double dValue);
	double GetConstructionCost() const;

	// Cout de preparation
	void SetPreparationCost(double dValue);
	double GetPreparationCost() const;

	// Cout de modelisation: Construction + Preparation
	double GetModelCost() const;

	// Cout des donnees connaissant le model
	void SetDataCost(double dValue);
	double GetDataCost() const;

	// Cout total: Model + Data
	double GetTotalCost() const;

	// Calcul du Level a partir des cout MODL
	//  Valide uniquement dans le cas de cout MODL (PreparationCost != 0)
	//  Calcul du level: 1 - TotalCost/NullCost
	//  La grille de preparation est detruite si le level passe a 0
	void ComputeLevel();

	// Affichage des donnees de cout (par defaut: true)
	boolean GetWriteCosts() const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Seuil pour passer les valeur de cout ou de level a zero, pour ameliorer
	// la lisibilite des rapports
	const double dEpsilonCost = 1e-10;

	// Nettoyage des resultats de preparation de donnees
	virtual void CleanDataPreparationResults();

	// Attributs
	KWDataGridStats* preparedDataGridStats;
	double dPreparedLevel;
	double dPreparationCost;
	double dConstructionCost;
	double dDataCost;
	friend class PLShared_DataPreparationStats;
};

///////////////////////////////////////////////////////
// Classe PLShared_LearningReport
// Serialisation de la classe KWLearningReport
class PLShared_LearningReport : public PLSharedObject
{
public:
	// Constructeur
	PLShared_LearningReport();
	~PLShared_LearningReport();

	// Acces aux statistiques
	void SetLearningReport(KWLearningReport* attributeStats);
	KWLearningReport* GetLearningReport();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////
// Classe PLShared_DataPreparationStats
// Serialisation de la classe KWDataPreparationStats
class PLShared_DataPreparationStats : public PLShared_LearningReport
{
public:
	// Constructeur
	PLShared_DataPreparationStats();
	~PLShared_DataPreparationStats();

	// Acces aux statistiques
	void SetDataPreparationStats(KWDataPreparationStats* attributeStats);
	KWDataPreparationStats* GetDataPreparationStats();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
};
