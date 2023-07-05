// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWLearningSpec.h"
#include "KWClassStats.h"
#include "KWBenchmarkClassSpec.h"
#include "KWDatabase.h"
#include "KWSTDatabaseTextFile.h"
#include "KWMTDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"

////////////////////////////////////////////////////////////
// Classe KWBenchmarkSpec
//    Donnees de benchmark
class KWBenchmarkSpec : public Object
{
public:
	// Constructeur
	KWBenchmarkSpec();
	~KWBenchmarkSpec();

	// Acces a la specification de la classe
	KWBenchmarkClassSpec* GetBenchmarkClassSpec();

	// Base de donnees
	KWDatabase* GetBenchmarkDatabase();

	// Acces rapides aux specifications principales de la classe et de la base
	const ALString& GetClassFileName() const;
	const ALString& GetClassName() const;
	const ALString& GetTargetAttributeName() const;
	const ALString& GetDatabaseName() const;

	////////////////////////////////////////////////////////
	// Exploitation des parametres une fois valides

	// Fabrication de specifications d'apprentissage operationnelles
	// a partir des specifs
	//   Reinitialisations des statistiques
	//   Chargement du fichier de dictionnaire
	//   Verification de la classe et de l'attribut cible
	//   Test d'existence du fichier de donnees
	// En cas de succes, la methode rend true, et les methodes suivantes
	// sont accessibles. Sinon, des messages d'erreur sont emis
	// Si les specifications changent, les methodes suivantes ne sont plus
	// utilisables et les objets LearningSpec et Database sont detruits.
	void BuildLearningSpec();
	boolean IsLearningSpecValid() const;

	// Acces a l'objet de specification des donnees, et a la base associee
	// Prerequis: l'objet LearninSpec doit avoir ete valide
	// Memoire: L'objet LearningSpec et Database associe appartiennent a l'appele
	KWLearningSpec* GetLearningSpec() const;
	KWDatabase* GetDatabase() const;

	// Destruction de l'objet LearningSpec et Database
	// Ces objets seront sinon detruits avec le destructeur
	void DeleteLearningSpec();

	// Calculs de statistiques sur le benchmark
	// Prerequis: l'objet LearninSpec doit etre valide
	void ComputeBenchmarkStats();

	//////////////////////////////////////////////////////////////////////
	// Acces aux statistiques sur le benchmark
	// Ces donnees sont toujours accessibles sans prerequis, et simplement
	// reinitialises lors des demandes de calcul

	// Taille de la base en nombre d'instance
	int GetDatabaseInstanceNumber() const;

	// Nombre d'attributs (hors attribut cible)
	int GetAttributeNumber() const;
	int GetSymbolAttributeNumber() const;
	int GetContinuousAttributeNumber() const;

	// Type d'attribut cible (Symbol, Continuous ou None)
	int GetTargetAttributeType() const;

	// Nombre de modalites cibles (si classification)
	int GetTargetValueNumber() const;

	// Precision du predicteur predisant la modalite cible majoritaire (si classification)
	double GetTargetMajorityAccuracy() const;

	// Moyenne de la valeur cible (si regression)
	double GetTargetMean() const;

	// Ecart-type de la valeur cible (si regression)
	double GetTargetStandardDeviation() const;

	// Ecriture de rapport lignes sur le benchmark et ses statistiques
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	/////////////////////////////////////////////////////////////////
	// Gestion du decoupage d'une Database pour la cross-validation

	// Calcul (pour une graine aleatoire donnee) d'un vecteur de repartition
	// des instances dans X partie pour un decoupage de cross-validation
	// Les resultats sont memorises dans le vecteur d'index passe en parametres.
	// Les index sont dans ranges dans l'ordre des instances du fichier et
	// correspondent a numero de la partie (le premier index vaut 0).
	// Les eventuels marquages d'instances dans la base sont prealablement reinitialises.
	// Les index des enregistrements permettent d'associer les instance a leur index et
	// de parametrer leur marquage dans la base
	void ComputeFoldIndexes(int nSeed, int nFoldNumber, boolean bStratified, IntVector* ivResultFoldIndexes);

	// Parametrage de la lecture d'une base de donnees en choisissant une partie
	// a utiliser, ou a exclure
	void ComputeDatabaseSelectedInstance(IntVector* ivFoldIndexes, int nFoldRefIndex, boolean bExcluding);

	////////////////////////////////////////////////////////
	// Divers

	// Verification des parametres
	boolean Check() const override;

	// Fraicheur de l'objet, incrementee a chaque modification
	int GetFreshness() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de specification du benchmark
	KWBenchmarkClassSpec* benchmarkClassSpec;
	KWDatabase* benchmarkDatabase;

	// Attribut d'utilisation du benchmark
	KWDatabase* database;
	KWLearningSpec* learningSpec;
	int nLearningSpecFreshness;

	// Index de records associes a chaque enregistrement de la base, pour le parametrage des fold
	// via le marquage de la database
	IntVector ivPhysicalRecordIndexes;

	// Attributs de statistiques sur le benchmark
	int nDatabaseInstanceNumber;
	int nSymbolAttributeNumber;
	int nContinuousAttributeNumber;
	int nTargetAttributeType;
	int nTargetValueNumber;
	double dTargetMajorityAccuracy;
	double dTargetMean;
	double dTargetStandardDeviation;
};