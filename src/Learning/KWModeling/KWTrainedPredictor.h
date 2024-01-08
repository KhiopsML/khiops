// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTrainedPredictor;
class KWTrainedClassifier;
class KWTrainedRegressor;
class KWTrainedClusterer;
class KWPredictionAttributeSpec;

#include "KWClass.h"
#include "KWSortableIndex.h"

//////////////////////////////////////////////////////////////////////////////////////
// Predicteur issu de l'apprentissage, sous la forme d'un dictionnaire de deploiement
// Permet d'identifier certain attribut de prediction parmi les attributs du dictionnaire.
// Ces attributs sont tagues via des meta-donnees, qui seront utilisee lors de la relecture
// des dictionnaire pour reperer les attributs de prediction
class KWTrainedPredictor : public Object
{
public:
	// Constructeur
	KWTrainedPredictor();
	~KWTrainedPredictor();

	// Nom du predicteur
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

	// Type de predicteur, a redefinir dans une sous-classe
	//   KWType::Symbol: classification
	//   KWType::Continuous: regression
	//   KWType::None: non supervise
	virtual int GetTargetType() const = 0;

	/////////////////////////////////////////////////////////////////////
	// Parametrage et construction de la classe de deploiement

	// Parametrage de la classe de deploiement et de son domaine
	// Reinitialisation du parametrage des attributs de prediction et initialisation des meta-donnees
	// relatives a la classe de prediction
	// Prerequis: doit appartenir a domaine de classe valide (KWClassDomain) non gere parmi les domaines
	//   (l'appartenance a un domaine est utile notament dans le cas multi-classe)
	// Memoire: appartient a l'appele (provoque la supression de la classe precedente et de son domaine)
	virtual void SetPredictorClass(KWClass* aClass, int nPredictorType, const ALString sPredictorLabel);
	KWClass* GetPredictorClass() const;
	KWClassDomain* GetPredictorDomain() const;

	// Nettoyage des specifications du predicteur, avec ou sans destruction
	// de la classe de prediction et de son domaine
	virtual void RemovePredictor();
	virtual void DeletePredictor();

	// Acces generique a l'attribut cible en classification ou regression (NULL par defaut)
	virtual KWAttribute* GetTargetAttribute() const;

	// Creation d'un attribut a partir d'un nom et d'une regle de derivation
	// L'attribut cree est correctement initialise (avec un nom eventuellement different si ce
	// nom existait deja), et insere dans la classe de prediction
	// Memoire: la regle appartient a l'attribut cree, qui lui-meme appartient a la classe de prediction
	KWAttribute* CreatePredictionAttribute(const ALString& sAttributeName, KWDerivationRule* rule);

	// Preparation de la classe pour le deploiement
	//   Mise en Unused de tous les attributs, sauf pour les eventuels attribut cible et de cle,
	//      et les attributs de predictions obligatoires et/ou utiles pour l'evaluation
	void PrepareDeploymentClass(boolean bMandatoryAttributes, boolean bEvaluationAttributes);

	// Simplification de la classe par rapport au domaine initial en parametre
	//   Supression des attributs inutiles au calcul des attributs de prediction
	//     (exceptes ceux qui etaient deja presents dans la classe initiale, et peuvent
	//      par exemple servir comme attribut de selection)
	//  Le domaine initial doit contenir la classe initiale
	void CleanPredictorClass(const KWClassDomain* initialDomain);

	// Import d'une classe existante pour parametrer un predicteur
	// On se base sur les meta-donnees des attributs pour reconnaitre les attributs
	// de prediction, obligatoires ou facultatifs
	// Prerequis: classe de prediction non renseignee, classe existante valide, et
	// appartenant pas a un domaine
	// Renvoie true si import possible, et dans ce cas, la classe est utilisee pour la prediction
	// dans son domaine d'origine (potentiellement partage par d'autres classes de prediction)
	// Renvoie false sinon, sans message d'erreur
	virtual boolean ImportPredictorClass(KWClass* aClass);

	// Test si le predicteur est consistent avec un autre predicteur et peut-etre evalue sur la meme base
	// Emission de message d'erreur si necessaire
	virtual boolean IsConsistentWith(const KWTrainedPredictor* otherPredictor) const;

	///////////////////////////////////////////////////////////////////
	// Parametrage du predicteur et de ses attributs

	// Nombre d'attributs de prediction
	int GetPredictionAttributeNumber() const;

	// Acces generique aux specifications d'un attribut de prediction
	const KWPredictionAttributeSpec* GetPredictionAttributeSpecAt(int nIndex) const;

	// Ajout d'une specification d'attribut de prediction, en fin de liste
	// (donc d'index AttributeNumber-1)
	// Methode avancee permettant par exemple de specifier des attributs de la classe
	// supplementaire devant etre charge en memoire pour des evaluations specifiques
	// a certains predicteurs
	// Memoire: la specification (qui doit etre valide) appartient a l'appelant
	void AddPredictionAttributeSpec(KWPredictionAttributeSpec* predictionAttributeSpec);

	///////////////////////////////////////////////////////////////////
	// Methodes standard

	// Controle d'integrite
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	// Methodes avancees pour le parametrage de mata-donnees dans les classes de prediction:
	// nom de la classe initiale, type de predicteur...
	// Ces methodes sont automatiquement appelees lors du parametrages d'un predicteur.
	// Elles peuvent avoir un interet pour retrouver les infos depuis l'exterieur

	// Parametrage du nom de la classe initiale dans les meta-donnees
	static void SetMetaDataInitialClassName(KWClass* kwcClass, const ALString& sInitialClassName);
	static const ALString GetMetaDataInitialClassName(const KWClass* kwcClass);

	// Parametrage du type de predicteur dans les meta-donnees
	static void SetMetaDataPredictorType(KWClass* kwcClass, int nType);
	static int GetMetaDataPredictorType(const KWClass* kwcClass);

	// Parametrage du libelle de predicteur dans les meta-donnees
	static void SetMetaDataPredictorLabel(KWClass* kwcClass, const ALString& sPredictorType);
	static const ALString GetMetaDataPredictorLabel(const KWClass* kwcClass);

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	////////////////////////////////////////////////////////////////////////////////
	// Parametrage de la gestion generique des attributs de prediction

	// Nombre d'attributs de prediction
	void SetPredictionAttributeNumber(int nValue);

	// Parametrage a utiliser dans les constructeurs des sous-classes pour
	// specifier les attributs de prediction
	void SetPredictionAttributeSpecAt(int nIndex, const ALString& sLabel, int nType, boolean bMandatory,
					  boolean bEvaluation);

	// Parametrage d'un attribut de prediction, a utiliser dans des methodes
	// specifiques des sous-classes
	void SetAttributeAt(int nIndex, KWAttribute* attribute);
	KWAttribute* GetAttributeAt(int nIndex) const;

	// Verification de l'integrite des attributs de predictions
	boolean CheckPredictionAttributes() const;

	// Nom du predicteur
	ALString sName;

	// Dictionnaire de prediction
	mutable KWClass* predictorClass;

	// Tableau des specifications des variables de prediction
	// Dans les sous-classe, les constructeurs doivent alimenter le tableau des specifications
	// et les methodes specifiques d'acces aux variables de prediction doivent alimenter
	// les variables correspondantes
	// Permet d'effectuer des traitement de facon generique
	ObjectArray oaPredictionAttributeSpecs;
};

/////////////////////////////////////////////////////////////////////////////
// Classifieur issu de l'apprentissage
class KWTrainedClassifier : public KWTrainedPredictor
{
public:
	// Constructeur
	KWTrainedClassifier();
	~KWTrainedClassifier();

	// Parametrage de la classe de deploiement et de son domaine
	void SetPredictorClass(KWClass* aClass, int nPredictorType, const ALString sPredictorLabel) override;

	// Nettoyage des specifications du predicteur
	void RemovePredictor() override;

	// Type de predicteur: classifieur
	int GetTargetType() const override;

	/////////////////////////////////////////////////////////////////////
	// Specification des attributs de prediction, referencant des
	// attributs de la classe de prediction

	// Index des attributs de prediction (entre 0 et le nombre d'attributs)
	enum
	{
		TargetAttribute,
		TargetValues,
		Prediction,
		Score,
		PredictionAttributeNumber
	};

	// Attribut cible (obligatoire)
	void SetTargetAttribute(KWAttribute* attribute);
	KWAttribute* GetTargetAttribute() const override;

	// Attribut de memorisation des valeurs cibles
	void SetTargetValuesAttribute(KWAttribute* attribute);
	KWAttribute* GetTargetValuesAttribute() const;

	// Valeur predite (obligatoire)
	void SetPredictionAttribute(KWAttribute* attribute);
	KWAttribute* GetPredictionAttribute() const;

	// Score de prediction (obligatoire)
	void SetScoreAttribute(KWAttribute* attribute);
	KWAttribute* GetScoreAttribute() const;

	// Nombre de valeurs cibles
	// Facultatif: permet de specifier les attributs de probabilites conditionnelles
	void SetTargetValueNumber(int nNumber);
	int GetTargetValueNumber() const;

	// Probabilite conditionnelle par valeur cible
	void SetProbAttributeAt(int nIndex, const Symbol& sTargetValue, KWAttribute* attribute);
	Symbol& GetTargetValueAt(int nIndex) const;
	KWAttribute* GetProbAttributeAt(int nIndex) const;

	//////////////////////////////////////////////////////
	// Divers

	// Import d'un classe existante pour parametrer un predicteur
	boolean ImportPredictorClass(KWClass* aClass) override;

	// Test si le predicteur est consistent avec un autre predicteur
	boolean IsConsistentWith(const KWTrainedPredictor* otherPredictor) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Cle de meta-donnee associee au probabilites cibles
	const ALString& GetTargetProbMetaDataKey() const;

	// Index du premier attribut de proba conditionnelle par valeur cible
	int nFirstProbAttributeIndex;

	// Tableau des specifications des variables pour les probabilites conditionnnelles par classe cible
	// Les valeurs cibles sont effectivement sauvegardeee a fins de controles essentiellement
	// Les specification d'attributs sont concatenees en fin du tableau oaPredictionAttributeSpecs
	SymbolVector svTargetValues;
};

/////////////////////////////////////////////////////////////////////////////
// Regresseur issu de l'apprentissage
class KWTrainedRegressor : public KWTrainedPredictor
{
public:
	// Constructeur
	KWTrainedRegressor();
	~KWTrainedRegressor();

	// Type de predicteur: classifieur
	int GetTargetType() const override;

	/////////////////////////////////////////////////////////////////////
	// Specification des attributs de prediction, referencant des
	// attributs de la classe de prediction

	// Index des attributs de prediction (entre 0 et le nombre d'attributs)
	enum
	{
		TargetAttribute,
		TargetAttributeRank,
		TargetValues,
		Mean,
		Density,
		MeanRank,
		DensityRank,
		PredictionAttributeNumber
	};

	// Attribut cible (obligatoire)
	void SetTargetAttribute(KWAttribute* attribute);
	KWAttribute* GetTargetAttribute() const override;

	// Rang de l'attribut cible
	void SetTargetAttributeRank(KWAttribute* attribute);
	KWAttribute* GetTargetAttributeRank() const;

	// Attribut de memorisation des valeurs cibles
	void SetTargetValuesAttribute(KWAttribute* attribute);
	KWAttribute* GetTargetValuesAttribute() const;

	// Prediction de la valeur moyenne (obligatoire)
	void SetMeanAttribute(KWAttribute* attribute);
	KWAttribute* GetMeanAttribute() const;

	// Prediction de la densite sur les valeurs
	void SetDensityAttribute(KWAttribute* attribute);
	KWAttribute* GetDensityAttribute() const;

	// Prediction du rang moyen
	void SetMeanRankAttribute(KWAttribute* attribute);
	KWAttribute* GetMeanRankAttribute() const;

	// Prediction de la densite sur les rang
	void SetDensityRankAttribute(KWAttribute* attribute);
	KWAttribute* GetDensityRankAttribute() const;

	//////////////////////////////////////////////////////
	// Divers

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Clusterer issu de l'apprentissage (non supervise)
class KWTrainedClusterer : public KWTrainedPredictor
{
public:
	// Constructeur
	KWTrainedClusterer();
	~KWTrainedClusterer();

	// Type de predicteur: classifieur
	int GetTargetType() const override;

	//////////////////////////////////////////////////////
	// Divers

	// Libelles standard
	const ALString GetClassLabel() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Specifications d'un attribut de prediction
// Permet de manipuler les attributs de specification de facon generique
class KWPredictionAttributeSpec : public Object
{
public:
	// Constructeur
	KWPredictionAttributeSpec();
	~KWPredictionAttributeSpec();

	//////////////////////////////////////////////////////////
	// Specification des caracteristiques de l'attribut

	// Libelle sur la fonction de l'attribut
	// Ce libelle sert de cle de meta-donnee, qui sera associe a l'attribut de prediction
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Type de l'attribut
	void SetType(int nValue);
	int GetType() const;

	// Flag sur le caractere obligatoire de l'attribut (defaut: false)
	// Ces attributs seront utilises en deploiement
	void SetMandatory(boolean bValue);
	boolean GetMandatory() const;

	// Flag sur l'utilisation (potentielle) pour l'evaluation
	void SetEvaluation(boolean bValue);
	boolean GetEvaluation() const;

	//////////////////////////////////////////////////////////
	// Specification de l'attribut lui-meme

	// Parametrage de l'attribut
	// Les meta-donnees sont associees a l'attribut au moment de l'appel de cette methode
	void SetAttribute(KWAttribute* kwaValue);
	KWAttribute* GetAttribute() const;

	// Verification de la concordance des specifications avec celles de l'attribut
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sLabel;
	int nType;
	boolean bMandatory;
	boolean bEvaluation;
	KWAttribute* attribute;
};
