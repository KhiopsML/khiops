// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBTargetPart;
class SNBPredictorSelectionDataCostCalculator;

class SNBSingletonTargetPart;
class SNBClassifierSelectionDataCostCalculator;

class SNBIntervalTargetPart;
class SNBRegressorSelectionDataCostCalculator;

class SNBGroupTargetPart;
class SNBGroupTargetPartSignatureSchema;
class SNBGeneralizedClassifierSelectionDataCostCalculator;

#include "KWLearningSpec.h"
#include "SNBDataTableBinarySliceSet.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Partie d'une partition des valeurs de la modalite cible (classe abstraite)
// Donne acces aux scores non-normalises de chaque instance d'une base de donnees d'apprentissage
// Le score pour l'instance i est donne par :
//
//   log P(Y = C_j) + Sum_k w_k * log P(X_ik| Y = C_j )  (classification)
//
//   Sum_k w_k * log P( X_ik | Y = I_m) (regression)
//
//   Sum_k w_k * log P( X_ik | Y = G_jm) (classification generalise : cible groupee)
//
//   C_j  : j-eme partie cible (celle representee par cet objet)
//   X_ik : Valeur du k-eme attribut pour l'instance i
//   w_k  : Poids du k-eme attribut
//
// Le sous-classes implementent les cas ou la partition de la cible est compose de :
//   - SNBSingletonTargetPart : Cas de la classification, utilise par SNBClassifierSelectionDataCostCalculator
//   - SNBIntervalTargetPart : Cas de la regression, utilise par KWRegressionSelectionDataCostCalculator
//   - SNBGroupTargetPart : Cas de la classification generalisee, utilise par SNBGeneralizedClassifierSelectionDataCostCalculator
class SNBTargetPart : public Object
{
public:
	// Constructeur
	SNBTargetPart();
	~SNBTargetPart();

	// Score non-normalise de la partie pour chaque instance i : log P(Y = C_j) + Sum_k w_k * log P( X_ik | Y = C_j )
	//   C_j  : j-eme partie cible (celle representee par cet objet)
	//   X_ik : Valeur du k-eme attribut pour l'instance i
	//   w_k  : Poids du k-eme attribut
	ContinuousVector* GetScores();

	// Rapport synthetique destine a rentrer dans un tableau
	virtual void WriteHeaderLineReport(ostream& ost) const = 0;
	virtual void WriteLineReport(ostream& ost) const = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Score non-normalise pour chaque instance: voir GetScores
	ContinuousVector cvScores;
};
//////////////////////////////////////////////////////////////////////////////////////////////////
// L'ecriture de cette methode, tres souvent utilisee, a ete particulierement optimisee :
//  - memorisation dans des variables locales de donnees de travail
//  - precalcul des parties constantes des couts (dans Initialize)
//  - memorisation dans des variables locales les limites des boucles (pour eviter leur reevaluation)
//
// Rationale de l'algorithme pour l'estmation de couts de donnees :
// Rappelons que le score non-normalise pour l'instance i et la modalite j est donnee par
//
//   score_ij = -log P(Y = C_j) + Sum_k w_k log P( X_ik | Y = C_j ).
//
// Notons que modalite j est associee a un objet KWSingletonPart qui contient son vecteur de scores.
// Pour le calcul, d'abord on ecrit la "vraisemblance ponderee" (voir nota ci-dessous)
// de l'instance i via un softmax :
//
//   P( Y = C_m | X ) ~  P(Y = C_m) * Prod_k P(X_ik | Y = C_m)^w_k
//        (regress)   ~  P(Rang(Y) = R_m) * Prod_k P(X_ik | Y = R_m)^w_k
//                    ~  P(Y in Part(R_m)) * Prod_k P(X_ik | Y in Part(R_m))
//
//        (group)     ~  P(Y = C_m | Y in G_l) P( Y in G_l ) * Prod_k P( X_ik | Y in G_l)
//
//                    ~   (#C_m)/(# Inst. in G_l) * (# Inst. in G_l)/N * Prod_k( X_ik | Y in G_l)
//
//                    = exp(score_im) / Sum_j exp(score_ij)
//
// P(Rang(Y) = R_m) = P(Y in I_l) * P(Rang(Y) = R_m | Y in P_l)
//                  = 1/N_l * P(Rang(Y) = R_m | Y in P_l)
//
// ou l'indice m = m(i) est tel que la valeur de Y dans l'instance i est C_m, i.e y_i = C_m.
// Maintenant on passe le numerateur au denominateur
//
//   P( Y = C_m | X ) = 1 / Sum_j exp(score_ij - score_im)
//                    = 1 / (1 + Sum_{j != m} exp(score_ij - score_im))
//
// Et donc le cout de l'instance est donnee par
//
//  cout_i = -log P( Y = C_m | X ) = log (1 + Sum_{j != m} exp(score_ij - score_im))        (1)
//
// Pour l'estabilite numerique, avant de passer au dernier log, on regularise la quantite
// exp(cout_i) par la methode de Laplace
//
//    exp(cout_i) -> (exp(cout_i) * N + epsilon) / (N + J * epsilon)                        (2)
//
// ou l'epsilon de Laplace a ete calcule pendant l'initialisation. On define S_i et T comme le
// numerateur et le denominateur de formule de Laplace (2). On utilise aussi une exponentielle
// tronquee pour eviter des overflows :
//
//   exp_tr(x) = min(exp(x), dMaxExpScore)
//
// ou dMaxExpScore a ete pre-calculee dans l'initialisation.
//
// Pour chaque instance i, l'algorithme ci-dessous calcule d'abord l'inverse exp(-cout_i) en
// parcourant toutes les modalites cibles.
// Ensuite il applique l'approximation de Laplace pour obtenir S_i pour chaque instance i. Un fois
// parcourues toutes les instances on rend
//
//   DataCost = - Sum_i log(S_i) + N * log(T)
//
// Nota : P( Y = C_m | X ) n'est pas une probabilite sauf si tous les w_k == 1
//
class SNBPredictorSelectionDataCostCalculator : public KWLearningService
{
public:
	// Constructeur
	SNBPredictorSelectionDataCostCalculator();
	~SNBPredictorSelectionDataCostCalculator();

	////////////////////////////////////////////
	// Initialisation et destruction

	// Parametrage du SNBDataTableBinarySliceSet de la database d'apprentissage
	void SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* dataTableBinarySliceSet);
	SNBDataTableBinarySliceSet* GetDataTableBinarySliceSet() const;

	// Allocation de la calculatrice : echec s'il n'y a pas assez de memoire pour les vecteur de scores
	virtual boolean Create();

	// Initialisation/reinitialisation
	void Initialize();

	// Destruction
	virtual void Delete() = 0;

	////////////////////////////////////////////////////////////////////////////
	// Calcul du cout de la selection et modification de la partition cible

	// Acces au cout de selection
	double GetSelectionDataCost() const;

	// Mise a jour de la calculatrice avec un changement de poids d'un attribut.
	// Facultatif : mettre a jour la structure de partie cible
	boolean IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight,
					boolean bUpdateTargetPartition);
	boolean DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight,
					boolean bUpdateTargetPartition);

	// Defait la derniere mise a jour de la calculatrice, ne peut etre utilise qu'une fois
	boolean UndoLastModification();

	/////////////////////////////////////////////////////////////////////////
	// Gestion des objets SNBTargetPart
	// Ces methodes permettent reutiliser des objets deja alloues

	// Aquisition d'une partie pret a l'utilisation
	// Renvoie NULL si probleme d'allocation du vecteur de scores
	virtual SNBTargetPart* GetOrCreatePart();

	// Libere une partie, elle n'est pas detruit pour une eventuelle reutilisation
	virtual void ReleasePart(SNBTargetPart* targetPart);

	// Creation generique d'une partie (sous-classe de SNBTargetPart)
	virtual SNBTargetPart* CreatePart() = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///  Implementation
protected:
	// Acces a la cible
	int GetTargetValueNumber() const;
	Symbol& GetTargetValueAt(int nTargetValue) const;
	int GetTargetValueFrequencyAt(int nTargetValue) const;

	// Implementation de l'initialization
	virtual void InitializeTargetPartition() = 0;
	virtual void InitializeDataCostState() = 0;

	// Mise a jour de la partition de la cible quand un attribut rentre dans la selection
	virtual void UpdateTargetPartitionWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) = 0;

	// Mise a jour de la partition de la cible quand un attribut sort de la selection
	virtual void
	UpdateTargetPartitionWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) = 0;

	// Mise a jour des scores avec un modification de poids d'un attribut
	virtual boolean
	UpdateTargetPartScoresWithWeightedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
						    Continuous cDeltaWeight) = 0;

	// Mise a jour du cout de selection a partir de l'etat actuel des scores de la calculatrice
	virtual boolean UpdateDataCost() = 0;

	// Mise a jour du cout de selection a partir de l'etat actuel des scores de la calculatrice dans le cas d'un attribut sparse
	// Il est necessaire pour savoir les indexes des instances a changer
	virtual boolean UpdateDataCostWithSparseAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) = 0;

	// True si l'on peut faire la derniere modification
	boolean IsUndoAllowed();

	/////////////////////////
	// Parametres

	// Learning database de l'appentissage
	SNBDataTableBinarySliceSet* binarySliceSet;

	////////////////////////////////
	// Objets de travail

	// Parties de la cible
	ObjectArray oaTargetParts;

	// Objets *TargetPart reutilisables (cache)
	ObjectList olReleasedPartsCache;

	// Nombre d'instances caste en double
	double dInstanceNumber;

	// Epsilon de Laplace
	double dLaplaceEpsilon;

	// Denominateur de l'estimation de Laplace
	double dLaplaceDenominator;

	// Valeur maximal de du score d'une instance
	Continuous cMaxScore;

	// Valeur maximal de l'exponentiel du score d'une instance
	double dMaxExpScore;

	// Cout de la selection courante
	double dSelectionDataCost;

	// Couts non normalise par instance de la selection courante
	// Sa memorisation est necessaire pour gerer le cas sparse
	DoubleVector dvInstanceNonNormalizedDataCosts;

	// Attribut de la derniere modification
	SNBDataTableBinarySliceSetAttribute* lastModificationAttribute;

	// True si la derniere modification etait une augmentation du poids d'un attribute
	boolean bLastModificationWasIncrease;

	// Poids de la derniere modification
	Continuous cLastModificationDeltaWeight;

	// Cout de la derniere modification
	double dLastModificationSelectionDataCost;

	// Couts non normalises par instance de la derniere modification
	DoubleVector dvLastModificationInstanceNonNormalizedDataCosts;

	// True si la derniere modification a change la partition cible
	boolean bLastModificationUpdatedTargetPartition;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Element d'une partition de la cible en singletons
// Correspond au cas de la classification standard : une partie pour chaque modalite cible
class SNBSingletonTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBSingletonTargetPart();
	~SNBSingletonTargetPart();

	// Parametrage de l'index valeur cible associee
	void SetTargetValueIndex(int nIndex);
	int GetTargetValueIndex() const;

	// Reimplementation interface SNBTargetPart
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	int nTargetValue;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculatrice de couts de donnees pour la classification
class SNBClassifierSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBClassifierSelectionDataCostCalculator();
	~SNBClassifierSelectionDataCostCalculator();

	// Reimplementation de l'interface de SNBPredictorSelectionDataCostCalculator
	//boolean IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight, boolean UpdateTargetPartition) override;
	//boolean UndoLastModification() override;
	boolean Create() override;
	void Delete() override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// True si l'objet est dans un etat valide
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nTargetPartNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Reimplementation de l'interface SNBPredictorSelectionDataCostCalculator
	void InitializeTargetPartition() override;
	void InitializeDataCostState() override;
	void UpdateTargetPartitionWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cDeltaWeight) override;
	boolean UpdateDataCost() override;
	boolean UpdateDataCostWithSparseAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;

	// Calcul du cout de selection non normalise pour une instance donnee
	double ComputeInstanceNonNormalizedDataCost(int nInstance) const;

	// Verification de la coherence de la partition cible
	boolean CheckParts() const;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Element d'une partition d'une cible numerique en intervalles
// Correspond au cas de la regression
class SNBIntervalTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBIntervalTargetPart();
	~SNBIntervalTargetPart();

	// Effectif de l'intervalle
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Effectif cumule, en tenant compte les intervalles precedents
	void SetCumulativeFrequency(int nValue);
	int GetCumulativeFrequency() const;

	// Compteur d'utilisation de partie cible (identifiee par son effectif cumule)
	// Permet de gerer l'ajout/supression des parties cibles lors des MAJ par des partitions cibles univariees
	void SetRefCount(int nValue);
	int GetRefCount() const;

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Nombre d'instances dans l'intervalle
	int nFrequency;

	// Nombre d'instances cumuleess jusqu'a cet intervalle
	int nCumulativeFrequency;

	// Nombre de references
	int nRefCount;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculatrice de couts de donnees pour la regression
class SNBRegressorSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBRegressorSelectionDataCostCalculator();
	~SNBRegressorSelectionDataCostCalculator();

	// Reimplementation de l'interface de SNBPredictorSelectionDataCostCalculator
	boolean Create() override;
	void Delete() override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// Ecriture standard
	void Write(ostream& ost) const override;

	// Verification de la coherence
	boolean Check() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nDistinctTargetValueNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Reimplementation de l'interface SNBPredictorSelectionDataCostCalculator
	void InitializeTargetPartition() override;
	void InitializeDataCostState() override;
	void UpdateTargetPartitionWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cDeltaWeight) override;
	boolean UpdateDataCost() override;
	boolean UpdateDataCostWithSparseAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;

	// Calcul du cout de selection non normalise pour une instance donnee
	double ComputeInstanceNonNormalizedDataCost(int nInstance) const;

	// Vue de liste de la partition cible
	ObjectList olTargetPartition;

	// Vecteur d'index de partie cible pour chaque instance de la base (ordonnee par valeur cible)
	IntVector ivTargetPartIndexesByRank;

	// Vecteur d'index de partie cible d'un attribut par partie cible multivarie
	IntVector ivAttributeTargetPartIndexByTargetPartIndex;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion de la specification d'une signature cible
// On definit la signature cible d'une valeur, ou d'un groupe de valeurs, comme le vecteur
// d'index de partie cible par attribut selectionne
// Deux valeurs ayant meme signature cible doivent appartenir a la meme partie, et une partie
// ne doit contenir que des valeurs de meme signature.
class SNBGroupTargetPartSignatureSchema : public Object
{
public:
	// Constructeur
	SNBGroupTargetPartSignatureSchema();
	~SNBGroupTargetPartSignatureSchema();

	// Initialisation avec aucun attribut
	void Initialize();

	//////////////////////////////////
	// Acces aux attributs

	// Taille de la signature cible
	int GetSize() const;

	// Acces a un attribut de la signature cible par son index
	SNBDataTableBinarySliceSetAttribute* GetAttributeAt(int nSignatureAttribute) const;

	// Acces a l'index de la signature d'un attribut; retourne -1 s'il n'en fait pas partie)
	int GetSignatureIndexAt(const SNBDataTableBinarySliceSetAttribute* attribute) const;

	// Ajout d'un index d'attribut dans la signature: son index est le dernier de la signature
	void AddAttribute(const SNBDataTableBinarySliceSetAttribute* attribute);

	// Supression d'un attribut de la signature
	// Le dernier attribut de la signatureprend l'index de l'attribut supprime
	void RemoveAttribute(const SNBDataTableBinarySliceSetAttribute* attribute);

	// True si l'attribut appartient a la signature
	boolean Contains(const SNBDataTableBinarySliceSetAttribute* attribute) const;

	//////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nAttributeNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Dictionnaire des paires KWSortableObject (SNBDataTableBinarySliceSetAttribute, index)
	// permettant d'associer chaque attribut selectionne a son index dans la signature
	// La cle du dictionnaire est l'objet SNBDataTableBinarySliceSetAttribute
	NumericKeyDictionary nkdAttributeIndexes;

	// Tableau des attributs de la signature cible
	ObjectArray oaAttributes;
};

// Comparaison de parties d'apres leur signature
int KWGroupTargetPartCompareTargetSignature(const void* elem1, const void* elem2);

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible
// Cas de la classification ou une partie correspond a un ensemble de valeurs
class SNBGroupTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBGroupTargetPart();
	~SNBGroupTargetPart();

	// Effectif de la partie
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Signature cible de la partie
	IntVector* GetSignature();
	int GetSignatureSize() const;
	int GetGroupIndexAt(int nSignature) const;

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Libelle
	const ALString GetObjectLabel() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nTargetValueNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nombre d'individus dans la partition
	int nFrequency;

	// Signature de la partition
	IntVector ivTargetSignature;
};

// Met a jour un vecteur de scores avec le score de l'attribut pour une cible donnee
// Pour un individu i dans le vecteur d'entree, valeur cible, attribut k et poids w
// la transformation est donee par :
//
//   score_i -> score_i + w * log P( X_k = x_ik | Y = y_j )
//

class SNBGeneralizedClassifierSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBGeneralizedClassifierSelectionDataCostCalculator();
	~SNBGeneralizedClassifierSelectionDataCostCalculator();

	// Reimplementation de l'interface de SNBPredictorSelectionDataCostCalculator
	boolean Create() override;
	void Delete() override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Ecriture standard
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber, int nTargetValueNumber);

	/////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Reimplementation de l'interface SNBPredictorSelectionDataCostCalculator
	void InitializeTargetPartition() override;
	void InitializeDataCostState() override;
	void UpdateTargetPartitionWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cDeltaWeight) override;
	boolean UpdateDataCost() override;
	boolean UpdateDataCostWithSparseAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) override;

	// Calcul du cout de selection non normalise pour une instance donnee
	double ComputeInstanceNonNormalizedDataCost(int nInstance) const;

	// Initialisation des services d'indexation de groupes cible pour les attributs
	void InitializeTargetValueGroupMatchings();
	void CleanTargetValueGroupMatchings();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des signatures
	// La signature d'une valeur est le vecteur d'index des groupes cibles correspondant
	// a la valeur, pour chaque attribut present dans la signature

	// Mise a jour d'une signature par ajout d'un index de groupe (en fin de signature)
	void UpdateSignatureWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue,
					       SNBGroupTargetPart* groupTargetPart) const;

	// Mise a jour d'une signature par supression d'un index de groupe
	// Le dernier attribut de la signature prend l'index de celui supprime
	void UpgradeTargetSignatureWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
							int nTargetValue, SNBGroupTargetPart* groupTargetPart) const;

	// Verficiation d'une partie cible par rapport a un indice de valeur cible
	boolean CheckTargetSignature(SNBGroupTargetPart* groupTargetPart, int nTargetValue) const;

	// Index de groupe pour un attribut et valeur cible donne
	int GetTargetValueGroupIndexAt(const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables de travail utilisees pour accelerer le calcul du cout de donnees
	// Ces variables de travail sont preallouees une fois pour toutes, et utilisee uniquement dans la
	// methode GetSelectionDataCost

	// Tableau codifiant la relation [TargetValueIndex -> GroupTargetPart]
	ObjectArray oaTargetPartsByTargetValueIndex;

	// Dictionnaire indexe par SNBDataTableBinarySliceSetAttribute's contenant des vecteurs d'entiers
	// Chaque vecteur codifie la relation [TargetValue -> AttributeTargetGroup] pour chaque attribut
	// Cette structure permet donc de codifier la relation [(Attribute, TargetValue) -> AttributeTargetGroup]
	// qui est implemente dans la methode GetTargetValueGroupIndexAt
	// Il n'est pas modifie au cours des calculs
	NumericKeyDictionary nkdTargetValueGroupMatchingsByAttribute;

	// Schema des signatures des parties de la cible
	SNBGroupTargetPartSignatureSchema signatureSchema;
};

inline double SNBClassifierSelectionDataCostCalculator::ComputeInstanceNonNormalizedDataCost(int nChunkInstance) const
{
	const boolean bDisplay = false;
	int nActualTarget;
	SNBTargetPart* actualTargetPart;
	double dInstanceInverseProb;
	int nTargetPartNumber;
	int nTargetPart;
	SNBTargetPart* targetPart;
	Continuous cDeltaScore;
	double dInstanceLaplaceNumerator;

	require(0 <= nChunkInstance and
		nChunkInstance < GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());

	// Recherche du vecteur de probabilite pour la classe cible reelle
	nActualTarget = GetDataTableBinarySliceSet()->GetTargetValueIndexAtInitializedChunkInstance(nChunkInstance);
	actualTargetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nActualTarget));

	dInstanceInverseProb = 0.0;
	nTargetPartNumber = oaTargetParts.GetSize();
	for (nTargetPart = 0; nTargetPart < nTargetPartNumber; nTargetPart++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTargetPart));

		// Cas cible egal a la cible reelle de l'instance : contribution de 1 (pas d'appel a std::exp, voir formule (1))
		if (targetPart == actualTargetPart)
			dInstanceInverseProb += 1.0;
		// Cas general : Calcul complet (utilise un appel a la fonction std::exp)
		else
		{
			// Difference de score pour la classe j
			cDeltaScore = targetPart->GetScores()->GetAt(nChunkInstance) -
				      actualTargetPart->GetScores()->GetAt(nChunkInstance);

			// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
			dInstanceInverseProb += cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore);
		}
	}
	// Calcul du numerateur de Laplace
	assert(dInstanceInverseProb >= 1);
	dInstanceLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;

	// Trace de debbogage
	if (bDisplay)
	{
		cout << "\t" << nChunkInstance << "\t" << nActualTarget << "\t" << nActualTarget << "\t"
		     << dInstanceLaplaceNumerator / dLaplaceDenominator << "\t"
		     << -log(dInstanceLaplaceNumerator / dLaplaceDenominator);
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTargetPart));
			cout << "\t" << targetPart->GetScores()->GetAt(nChunkInstance);
		}
		cout << "\n";
	}

	ensure(0 < dInstanceLaplaceNumerator and dInstanceLaplaceNumerator < dLaplaceDenominator);
	return -log(dInstanceLaplaceNumerator);
}

inline double SNBRegressorSelectionDataCostCalculator::ComputeInstanceNonNormalizedDataCost(int nChunkInstance) const
{
	const boolean bDisplay = false;
	int nActualTarget;
	SNBIntervalTargetPart* actualTargetPart;
	double dInstanceInverseProb;
	int nTargetPartNumber;
	int nTargetPart;
	SNBIntervalTargetPart* targetPart;
	Continuous cDeltaScore;
	double dInstanceLaplaceNumerator;

	// Recherche du vecteur de probabilite pour la classe cible reelle
	nActualTarget = GetDataTableBinarySliceSet()->GetTargetValueIndexAtInitializedChunkInstance(nChunkInstance);
	actualTargetPart =
	    cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(ivTargetPartIndexesByRank.GetAt(nActualTarget)));

	dInstanceInverseProb = 0.0;
	nTargetPartNumber = oaTargetParts.GetSize();
	for (nTargetPart = 0; nTargetPart < nTargetPartNumber; nTargetPart++)
	{
		targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));

		// Cas cible egal a la cible reelle de l'instance : contribution de #partie (pas d'appel a std::exp, voir formule (1))
		if (targetPart == actualTargetPart)
			dInstanceInverseProb += actualTargetPart->GetFrequency();
		// Cas general : Calcul complet (utilise un appel a la fonction std::exp)
		else
		{
			// Difference de score pour l'intervalle cible en cours
			cDeltaScore = targetPart->GetScores()->GetAt(nChunkInstance) -
				      actualTargetPart->GetScores()->GetAt(nChunkInstance);

			// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
			dInstanceInverseProb +=
			    targetPart->GetFrequency() * (cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore));
		}
	}
	// Calcul du numerateur de Laplace
	assert(dInstanceInverseProb >= 1);
	dInstanceLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "\t" << nChunkInstance << "\t" << nActualTarget << "\t"
		     << ivTargetPartIndexesByRank.GetAt(nActualTarget) << "\t"
		     << dInstanceLaplaceNumerator / dLaplaceDenominator << "\t"
		     << -log(dInstanceLaplaceNumerator / dLaplaceDenominator);
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));
			cout << "\t" << targetPart->GetScores()->GetAt(nChunkInstance);
		}
		cout << "\n";
	}

	ensure(0 < dInstanceLaplaceNumerator and dInstanceLaplaceNumerator < dLaplaceDenominator);
	return -log(dInstanceLaplaceNumerator);
}

inline double
SNBGeneralizedClassifierSelectionDataCostCalculator::ComputeInstanceNonNormalizedDataCost(int nChunkInstance) const
{
	const boolean bDisplay = false;
	int nActualTarget;
	SNBGroupTargetPart* actualTargetPart;
	double dInstanceInverseProb;
	int nTargetPart;
	SNBGroupTargetPart* targetPart;
	Continuous cDeltaScore;
	double dInstanceLaplaceNumerator;

	// Recherche du vecteur de probabilite pour la classe cible reelle
	nActualTarget = GetDataTableBinarySliceSet()->GetTargetValueIndexAtInitializedChunkInstance(nChunkInstance);
	actualTargetPart = cast(SNBGroupTargetPart*, oaTargetPartsByTargetValueIndex.GetAt(nActualTarget));

	dInstanceInverseProb = 0.0;
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nTargetPart));

		// Cas particulier pour eviter les calculs
		if (targetPart == actualTargetPart)
			dInstanceInverseProb += targetPart->GetFrequency();
		// Cas general, impliquant un calcul d'exponentiel
		else
		{
			// Difference de score pour le groupe cible en cours
			cDeltaScore = targetPart->GetScores()->GetAt(nChunkInstance) -
				      actualTargetPart->GetScores()->GetAt(nChunkInstance);

			// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
			dInstanceInverseProb +=
			    targetPart->GetFrequency() * (cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore));
		}
	}
	assert(dInstanceInverseProb >= 1);

	// Mise a jour de l'inverse de la probabilite, avec l'effectif de la valeur cible
	dInstanceInverseProb /= GetTargetValueFrequencyAt(nActualTarget);

	// Calcul du numerateur de Laplace
	assert(dInstanceInverseProb >= 1);
	dInstanceLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "\t" << nChunkInstance << "\t" << nActualTarget << "\t"
		     << oaTargetPartsByTargetValueIndex.GetAt(nActualTarget) << "\t"
		     << dInstanceLaplaceNumerator / dLaplaceDenominator << "\t"
		     << -log(dInstanceLaplaceNumerator / dLaplaceDenominator);
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nTargetPart));
			cout << "\t" << targetPart->GetScores()->GetAt(nChunkInstance);
		}
		cout << "\n";
	}

	ensure(0 < dInstanceLaplaceNumerator and dInstanceLaplaceNumerator < dLaplaceDenominator);
	return -log(dInstanceLaplaceNumerator);
}
