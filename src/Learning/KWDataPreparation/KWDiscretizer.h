// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWVersion.h"
#include "KWFrequencyVector.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme generique de fusion des lignes d'une table de contingence
// Contient des methodes generales, specialisables par algorithme
// dans des sous-classes
class KWDiscretizer : public Object
{
public:
	// Constructeur
	KWDiscretizer();
	~KWDiscretizer();

	// Nom de l'algorithme
	virtual const ALString GetName() const = 0;

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWDiscretizer* KWSpecificDiscretizer::Create() const
	//      {
	//          return new KWSpecificDiscretizer;
	//      }
	virtual KWDiscretizer* Create() const = 0;

	// Indique que le discretizer fait partie de la famille MODL. Par defaut: false
	// Il s'agit d'heriter de KWDiscretizerMODLFamily, avec les services de calcul de cout associes
	virtual boolean IsMODLFamily() const;

	// Parametre principal
	// L'acces a ce parametre est redefini de facon specifique dans les sous-classes
	double GetParam() const;
	void SetParam(double dValue);

	// Effectif minimum par intervalle (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme s'il vaut 0
	int GetMinIntervalFrequency() const;
	void SetMinIntervalFrequency(int nValue);

	// Nombre maximum d'intervalles (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme s'il vaut 0
	int GetMaxIntervalNumber() const;
	void SetMaxIntervalNumber(int nValue);

	// Utilisation des valeurs sources pour la discretisation. Par defaut: false
	// Utile par exemple pour les methodes non supervisees
	virtual boolean IsUsingSourceValues() const;

	// Duplication d'un discretiseur
	KWDiscretizer* Clone() const;

	// Recopie des specifications du discretiseur
	virtual void CopyFrom(const KWDiscretizer* kwdSource);

	/////////////////////////////////////////////////////////////////////////////
	// Methodes de partitionnement des lignes adjacentes d'un tableau de contingence
	// Methode a appeler dans le cas ou l'option UseSourceValues est a false
	//
	// On calcule la meilleure table de contingence cible par partitionnement de
	// lignes adjacentes selon un critere defini par algorithme.
	// L'application typique est la discretization d'un attribut continue,
	// en se basant sur l'ordre des instances defini par la table de contingence source.
	//
	// Parametres generaux
	//    kwctSource: table de contingence initiale
	//    kwctTarget: table de contingence de la nouvelle loi
	//    code retour: evaluation de la qualite du resultat par l'algorithme
	//          renvoie 0 si pas de nouvelle table, ou si methode sans evaluation
	//
	// MEMORY: La liberation des parametres en retour est de la responsabilite
	// de l'appelant. Attention, ces parametres peuvent etre NULL.

	// Calcul de la loi agregee pour des regroupements de lignes adjacentes
	virtual void Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	/////////////////////////////////////////////////////////////////////////////
	// Methodes de partitonnement d'une serie de valeur
	// Methode a appeler dans le cas ou l'option UseSourceValues est a true
	//
	// On calcule la meilleure table de contingence cible par partitionnement de
	// lignes adjacentes selon un critere defini par algorithme.
	// L'application typique est la discretization d'un attribut continue,
	// en se basant sur la valeur des instances (devant etre triees par valeur croissante).
	//
	// Parametres generaux
	//    cvSourceValues: valeurs sources des instances a partitionner
	//    ivTargetIndexes:  index des valeurs cibles des instancs a partitionner
	//    nTargetValueNumber: nombre de valeurs cibles possibles
	//    code retour: evaluation de la qualite du resultat par l'algorithme
	//          renvoie 0 si pas de nouvelle table, ou si methode sans evaluation
	//
	// MEMORY: La liberation des parametres en retour est de la responsabilite
	// de l'appelant. Attention, ces parametres peuvent etre NULL.

	// Calcul de la loi agregee pour des regroupements de lignes adjacentes
	virtual void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
				      int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const;

	/////////////////////////////////////////////////////////////////////////
	// Administration des discretiseurs

	// Enregistrement dans la base des discretiseurs
	// Il ne doit pas y avoir deux discretiseurs enregistres avec le meme nom
	// Memoire: les discretiseurs enregistres sont geres par l'appele
	static void RegisterDiscretizer(KWDiscretizer* discretiser);

	// Recherche par cle
	// Retourne NULL si absent
	static KWDiscretizer* LookupDiscretizer(const ALString& sName);

	// Recherche par cle et duplication
	// Permet d'obtenir un discretiseur pret a etre instancie
	// Retourne NULL si absent
	static KWDiscretizer* CloneDiscretizer(const ALString& sName);

	// Export de tous les discretiseurs enregistres
	// Memoire: le contenu du tableau appartient a l'appele
	static void ExportAllDiscretizers(ObjectArray* oaDiscretizers);

	// Suppresion/destruction de tous les predicteurs enregistrees
	static void RemoveAllDiscretizers();
	static void DeleteAllDiscretizers();

	///////////////////////////////////////////////////////////////////////
	// Services standard

	// Verification des parametres
	boolean Check() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Parametres de l'algorithme
	double dParam;
	int nMinIntervalFrequency;
	int nMaxIntervalNumber;

	// Administration des discretiseurs
	static ObjectDictionary* odDiscretizers;
};

// Fonction de comparaison sur le nom d'un discretiseur
int KWDiscretizerCompareName(const void* first, const void* second);
