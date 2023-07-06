// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWUnivariatePartitionCosts;
class KWMODLDiscretizationCosts;
class KWMODLGroupingCosts;

#include "KWStat.h"
#include "KWFrequencyVector.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariatePartitionCosts
// Definition de la structure des couts d'une partition univariee des donnees
// Les couts par entite, nuls par defaut, sont redefinissable dans des sous-classes
// La partition est definie par le nombre de parties. Chaque partie est representee
// par un vecteur d'effectifs (sous-classe de KWFrequencyVector).
// Des parametres globaux (nombre de valeurs total...) peuvent egalement etre
// utilise dans des sous-classes, afin de parametrer les couts
class KWUnivariatePartitionCosts : public Object
{
public:
	// Constructeur
	KWUnivariatePartitionCosts();
	~KWUnivariatePartitionCosts();

	// Duplication (avec recopie des valeurs de parametrage)
	KWUnivariatePartitionCosts* Clone() const;

	// Creation d'un structure de cout de la meme classe
	virtual KWUnivariatePartitionCosts* Create() const;

	// Recopie du parametrage d'un objet de la meme classe
	virtual void CopyFrom(const KWUnivariatePartitionCosts* sourceCosts);

	// Acces a un createur d'instance de vecteur d'effectif, compatible avec les methodes de calcul de cout
	// Par defaut: renvoie un KWDenseFrequencyVector
	const KWFrequencyVector* GetFrequencyVectorCreator() const;

	//////////////////////////////////////////////////////////////
	// Parametrage, influant sur la structure de cout

	// Acces a la granularite courante utilisee pour evaluer les couts
	// Est modifie par le discretiseur ou le grouper MODL a chaque changement de granularite (fait dans
	// InitializeWorkingData)
	void SetGranularity(int nValue);
	int GetGranularity() const;

	// Acces au nombre total d'instances necessaire pour calculer la granularite maximale
	// Fixe une fois pour toutes (independant de la granularite)
	void SetTotalInstanceNumber(int nValue);
	int GetTotalInstanceNumber() const;

	// Nombre de valeurs possibles de la variable
	// Il s'agit du nombre de partiles issus de la granularisation de la table
	// note N_G dans le cas numerique et V_G dans le cas categoriel
	// Ce parametrage est donc a effectuer a chaque changement de granularite (fait dans InitializeWorkingData)
	void SetValueNumber(int nValue);
	int GetValueNumber() const;

	// Nombre de classes cibles
	void SetClassValueNumber(int nValue);
	int GetClassValueNumber() const;

	// Cout d'un attribut, utilise pour la regularisation des regles de construction
	// Usage avance; par defaut 0
	void SetAttributeCost(double dValue);
	double GetAttributeCost() const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la partition
	// Methodes virtuelles, renvoyant 0, a reimplementer dans les sous-classes

	// Calcul du cout de partition
	// Attention, seule la variante compatible avec la situation (discretisation ou groupage) doit etre
	// reimplementee Par securite ces deux methodes renvoient assert(false) par defaut pour que la bonne methode
	// soit utilisee Discretisation
	virtual double ComputePartitionCost(int nPartNumber) const;
	// Groupage
	// Cette nouvelle methode est creee pour integrer le cout de la poubelle dans le cas du groupage
	// nPartNumber est le nombre total de parties i.e. le nombre de parties informatives + 1  en presence d'une
	// poubelle)
	virtual double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const;

	// Calcul de la variation de cout de partition suite a decrementation du nombre de parties
	// Attention, seule la variante compatible avec la situation (discretisation ou groupage) doit etre
	// reimplementee Par securite ces deux methodes renvoient assert(false) par defaut pour que la bonne methode
	// soit utilisee Discretisation Par defaut, retourne ComputeModelCost(nPartNumber-1) -
	// ComputeModelCost(nPartNumber) Permet d'optimiser les calcul de couts
	virtual double ComputePartitionDeltaCost(int nPartNumber) const;
	// Groupage
	// Cette nouvelle methode est creee pour integrer le cout de la poubelle dans le cas du groupage
	// nPartNumber est le nombre total de parties informatives
	// (i.e. le nombre de parties informatives + 1 groupe poubelle  en presence d'une poubelle)
	virtual double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const;

	// Calcul du cout local d'une partie
	virtual double ComputePartCost(const KWFrequencyVector* part) const;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	virtual double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const;

	///////////////////////////////////////////////////////////////////////////
	// Affichage des couts par entite

	// Affichage des couts de toutes les partie de la partition
	void WritePartitionAllCosts(const KWFrequencyTable* partTable, ostream& ost) const;

	// Affichage du cout d'un attribut
	// La methode prend en parametre la taille de la partie poubelle pour avoir un prototype
	// unique commun aux discretisations et aux groupages
	// Dans le cas d'une discretisation, ce parametre n'est pas utilise car il y a une redirection vers
	// ComputePartitionCost(int nPartNumber) Dans le cas d'un groupage, ce parametre est utilise car il y a une
	// redirection vers ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) nPartNumber doit
	// correspondre au nombre total de parties (nombre de parties informatives +1 s'il y a une partie poubelle)
	// virtual void WritePartitionCost(int nPartNumber, ostream& ost) const;
	virtual void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const;

	// Affichage du cout d'une partie
	void WritePartCost(const KWFrequencyVector* part, ostream& ost) const;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts de lie au modele ou aux donnees
	// Utilisation a titre informatif pour distribuer les cout totaux utilises
	// dans les algorithmes, soit sur la partie modele, soit sur la partie donnees

	// Une fois la refonte des nouveau prior, on a envisage un refactoring
	// reposant sur les principes suivants:
	//  . structure definissant les modeles independantes des classes de cout
	//  . donc les modeles (KWFrequencyTable) contiennent les infos de granularite,
	//    GarbageGroup... necessaires au calcul des cout
	//  . les classes de cout pour les PartitionCost sont parametres par les modeles,
	//      et non pas des entiers
	//      (int nPartNumber, int nGarbageModalityNumber, nGranularity...)
	//  . unification des KWFrequencyVector, KWMODLLine...
	// Bilan : en pratique cette modification alourdirait la solution car les
	// structures KWMODLLine et KWMODLGroup sont plus legeres que KWFrequencyTable
	// qu'il serait necessaire de creer pour chaque appel de ces methodes de couts. Plus leger de conserver un
	// parametre avec deux entiers.

	// Cout de modele par entite
	// Methodes a redefinir (par defaut: 0)
	// Cette methode prend en parametre la taille de la partie poubelle pour avoir un prototype
	// unique commun aux discretisations et aux groupages
	// Dans le cas d'une discretisation, ce parametre n'est pas utilise car il y a une redirection vers
	// ComputePartitionCost(int nPartNumber) Dans le cas d'un groupage, ce parametre est utilise car il y a une
	// redirection vers ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) nPartNumber doit
	// correspondre au nombre total de parties (= nombre de parties informatives +1 s'il y a une partie poubelle)
	virtual double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const;
	virtual double ComputePartModelCost(const KWFrequencyVector* part) const;

	// Cout de construction par entite
	// Methodes a redefinir (par defaut: 0)
	virtual double ComputePartitionConstructionCost(int nPartNumber) const;
	virtual double ComputePartConstructionCost(const KWFrequencyVector* part) const;

	// Cout des preparation par entite (deduit de PreparationCost=ModelCost-ConstructionCost)
	// Ajout argument taille poubelle (methode non utilisee actuellement)
	double ComputePartitionPreparationCost(int nPartNumber, int nGarbageModalityNumber) const;
	double ComputePartPreparationCost(const KWFrequencyVector* part) const;

	// Cout des donnees connaissant le modele par entite (deduit de DataCost=Cost-ModelCost)
	// Ajout argument taille poubelle (methode non utilisee actuellement)
	double ComputePartitionDataCost(int nPartNumber, int nGarbageModalityNumber) const;
	double ComputePartDataCost(const KWFrequencyVector* part) const;

	// Calcul du cout de model et des donnees
	double ComputePartitionGlobalModelCost(const KWFrequencyTable* partTable) const;
	double ComputePartitionGlobalConstructionCost(const KWFrequencyTable* partTable) const;
	double ComputePartitionGlobalPreparationCost(const KWFrequencyTable* partTable) const;
	double ComputePartitionGlobalDataCost(const KWFrequencyTable* partTable) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametres pour les calculs de cout
	mutable int nGranularity;
	int nTotalInstanceNumber;
	int nValueNumber;
	int nClassValueNumber;
	double dAttributeCost;

	// Objet createur de vecteur d'effectif
	KWFrequencyVector* kwfvFrequencyVectorCreator;

	// Epsilon
	static const double dEpsilon;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWMODLDiscretizationCosts
// Definition de la structure des couts d'une discretisation MODL
class KWMODLDiscretizationCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	KWMODLDiscretizationCosts();
	~KWMODLDiscretizationCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const override;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWMODLGroupingCosts
// Definition de la structure des couts d'une discretisation MODL
class KWMODLGroupingCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	KWMODLGroupingCosts();
	~KWMODLGroupingCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	// Le parametre nPartNumber designe le nombre total de parties
	// Une partition avec groupe poubelle non vide contient donc une partition en nPartNumber-1 groupes + 1 groupe
	// poubelle
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;

	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const override;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariateNullPartitionCosts
// Variante d'une structure de cout, en ignorant les cout de partition
class KWUnivariateNullPartitionCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	KWUnivariateNullPartitionCosts();
	~KWUnivariateNullPartitionCosts();

	// Parametrage d'une structure de cout, pour reprendre les cout de partie,
	// mais ignorer els cout de partition
	// Par defaut: NULL (tous les couts a 0)
	// Memoire: l'objet appartient a l'appele (le Set remplace et detruit le parametre precedent)
	void SetUnivariatePartitionCosts(KWUnivariatePartitionCosts* kwupcCosts);
	KWUnivariatePartitionCosts* GetUnivariatePartitionCosts() const;

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Recopie du parametrage d'un objet de la meme classe
	void CopyFrom(const KWUnivariatePartitionCosts* sourceCosts) override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Couts de partitionnement univarie de reference
	KWUnivariatePartitionCosts* univariatePartitionCosts;
};

///////////////////////////////////////////////////////////////////////
// Methodes en inline

inline void KWUnivariatePartitionCosts::SetValueNumber(int nValue)
{
	require(nValue >= 0);
	nValueNumber = nValue;
}

inline int KWUnivariatePartitionCosts::GetValueNumber() const
{
	return nValueNumber;
}

inline void KWUnivariatePartitionCosts::SetClassValueNumber(int nValue)
{
	require(nClassValueNumber >= 0);
	nClassValueNumber = nValue;
}

inline int KWUnivariatePartitionCosts::GetClassValueNumber() const
{
	return nClassValueNumber;
}

inline void KWUnivariatePartitionCosts::SetAttributeCost(double dValue)
{
	require(dValue >= 0);
	dAttributeCost = dValue;
}

inline double KWUnivariatePartitionCosts::GetAttributeCost() const
{
	return dAttributeCost;
}

inline const KWFrequencyVector* KWUnivariatePartitionCosts::GetFrequencyVectorCreator() const
{
	return kwfvFrequencyVectorCreator;
}

inline void KWUnivariatePartitionCosts::SetGranularity(int nValue)
{
	require(nValue >= 0);
	nGranularity = nValue;
}
inline int KWUnivariatePartitionCosts::GetGranularity() const
{
	return nGranularity;
}
inline void KWUnivariatePartitionCosts::SetTotalInstanceNumber(int nValue)
{
	require(nValue >= 0);
	nTotalInstanceNumber = nValue;
}
inline int KWUnivariatePartitionCosts::GetTotalInstanceNumber() const
{
	return nTotalInstanceNumber;
}