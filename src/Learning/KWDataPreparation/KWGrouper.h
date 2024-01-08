// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWGrouper;
class KWGrouperBasicGrouping;

#include "KWVersion.h"
#include "KWFrequencyVector.h"
#include "KWStat.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme generique de fusion des lignes d'une table de contingence
// Contient des methodes generales, specialisables par algorithme
// dans des sous-classes
class KWGrouper : public Object
{
public:
	// Constructeur
	KWGrouper();
	~KWGrouper();

	// Nom de l'algorithme
	virtual const ALString GetName() const = 0;

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplementer dans les sous-classes
	// La reimplementation typique est:
	//      KWGrouper* KWSpecificGrouper::Create() const
	//      {
	//          return new KWSpecificGrouper;
	//      }
	virtual KWGrouper* Create() const = 0;

	// Indique que le discretizer fait partie de la famille MODL. Par defaut: false
	// Il s'agit d'heriter de KWGrouperMODLFamily, avec les services de calcul de cout associes
	virtual boolean IsMODLFamily() const;

	// Parametre principal
	// L'acces a ce parametre est redefini de facon specifique dans les sous-classes
	double GetParam() const;
	void SetParam(double dValue);

	// Effectif minimum par groupe (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme
	// s'il vaut 0
	int GetMinGroupFrequency() const;
	void SetMinGroupFrequency(int nValue);

	// Nombre maximum de groupes (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme
	// s'il vaut 0
	int GetMaxGroupNumber() const;
	void SetMaxGroupNumber(int nValue);

	// Activation du preprocessing de la table (defaut: false)
	// Ce parametre est mis a true lors de l'utilisation en non supervise
	boolean GetActivePreprocessing() const;
	void SetActivePreprocessing(boolean bValue);

	// Duplication d'un groupeur
	KWGrouper* Clone() const;

	// Recopie des specifications du grouper
	virtual void CopyFrom(const KWGrouper* kwdSource);

	/////////////////////////////////////////////////////////////////////////////
	// Methodes de groupage des lignes d'un tableau de contingence
	//
	// On calcule la meilleure table de contingence cible par fusion de lignes
	// selon un critere defini par algorithme.
	//
	// Parametres generaux
	//    kwctSource: table de contingence initiale
	//            les lignes de la table doivent etre triees par effectif decroissant
	//    kwctTarget: table de contingence de la nouvelle loi
	//                NULL si code retour false
	//    ivGroups: vecteur d'entiers indiquant pour chaque index de
	//            ligne de la table de contingence initiale, le rang de la
	//            ligne fusionnee dans la nouvelle table de contingence
	//            NULL si code retour false
	//            Le vecteur ivGroup peut etre plus petit que la table source:
	//            le dernier groupe reference par ivGroups est valable pour toutes
	//            les lignes de la fin de la table initiale
	//    code retour: evaluation de la qualite du resultat par l'algorithme
	//          renvoie 0 si pas de nouvelle table, ou si methode sans evaluation
	//
	// MEMORY: La liberation des parametres en retour est de la responsabilite
	// de l'appelant. Attention, ces parametres peuvent etre NULL.

	// Groupage des modalites, avec gestion des contraintes d'effectif minimum
	// par groupe et de nombre max de groupes
	virtual void Group(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const;

	/////////////////////////////////////////////////////////////////////////
	// Administration des groupeurs, soit supervise, soit non supervise
	// selon le TargetAttributeType (Symbol ou None)

	// Enregistrement dans la base des groupeurs
	// Il ne doit pas y avoir deux groupeurs enregistres avec le meme nom
	// Memoire: les groupeurs enregistres sont geres par l'appele
	static void RegisterGrouper(int nTargetAttributeType, KWGrouper* grouper);

	// Recherche par cle
	// Retourne NULL si absent
	static KWGrouper* LookupGrouper(int nTargetAttributeType, const ALString& sName);

	// Recherche par cle et duplication
	// Permet d'obtenir un groupeur pret a etre instancie
	// Retourne NULL si absent
	static KWGrouper* CloneGrouper(int nTargetAttributeType, const ALString& sName);

	// Export de tous les groupeurs enregistres
	// Memoire: le contenu du tableau appartient a l'appele
	static void ExportAllGroupers(int nTargetAttributeType, ObjectArray* oaGroupers);

	// Suppresion/destruction de tous les predicteurs enregistrees
	static void RemoveAllGroupers();
	static void DeleteAllGroupers();

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
	// Acces aux groupers selon l'attribut cible
	static ObjectDictionary* GetGroupers(int nTargetAttributeType);

	//////////////////////////////////////////////////////////////////
	// Methodes interne pour le groupage
	// La methode principale construit si necessaire une table preprocesse
	// puis effectue le groupage sur cette table

	// Groupage des modalites pour une table preprocessee
	// Cette methode gere egalement la contrainte du nombre max de groupes
	// pour le nombre max de groupe final
	// Par defaut: duplication de la table source
	virtual void GroupPreprocessedTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					    IntVector*& ivGroups) const;

	// Calcul d'une table preprocesse tenant compte des contraintes liees a la methode
	// On retourne NULL s'il n'y a pas besoin de proprocessing
	virtual KWFrequencyTable* BuildPreprocessedTable(KWFrequencyTable* table) const;

	// Calcul de la frequence min par ligne liee aux contraintes de la methode de groupage
	// Par defaut: on prend en compte uniquement la contrainte MinGroupFrequency
	virtual int ComputePreprocessedMinLineFrequency(KWFrequencyTable* table) const;

	// Calcul du nombre maximum de ligne liee aux contraintes de la methode de groupage
	// Par defaut: on prend le nombre de lignes de la table sauf si MaxGroupNumber=1
	// (la contrainte MaxGroupNumber est assuree par la methode de groupage,
	// pas par le preprocessing)
	virtual int ComputePreprocessedMaxLineNumber(KWFrequencyTable* table) const;

	//////////////////////////////////////////////////////////////////
	// Methodes utilitaires pour le pretraitement d'une table de contingence
	// La table doit etre trie par effectif ligne decroissant

	// Calcul du nombre de lignes d'une table apres filtrage par un nombre de ligne
	// maximum et un effectif taille minimum par ligne.
	// La ligne resultant de la fusion des lignes surnumeraires ou en sous-effectif
	// doit egalement respecter la contrainte d'effectif minimum par ligne
	int ComputeTableReducedLineNumber(KWFrequencyTable* table, int nMaxLineNumber, int nMinLineFrequency) const;

	// Construction d'une table de contingence par en fusionnant toutes les lignes
	// surnumeraires de fin de table sur la derniere ligne)
	KWFrequencyTable* BuildReducedTable(KWFrequencyTable* table, int nNewLineNumber) const;

	// Parametres de l'algorithme
	double dParam;
	int nMinGroupFrequency;
	int nMaxGroupNumber;
	boolean bActivePreprocessing;

	// Administration des groupers
	static ObjectDictionary odSupervisedGroupers;
	static ObjectDictionary odUnsupervisedGroupers;
};

// Fonction de comparaison sur le nom d'un grouper
int KWGrouperCompareName(const void* first, const void* second);

//////////////////////////////////////////////////////////////////////////////////
// Algorithme BasicGrouping de groupage non supervise des lignes d'une table
// de contingence, en fusionnant les modalites tant qu'elle n'atteignent pas
// l'effectif minimal et tant qu'elles ne sont pas en nombre trop important
class KWGrouperBasicGrouping : public KWGrouper
{
public:
	// Constructeur
	KWGrouperBasicGrouping();
	~KWGrouperBasicGrouping();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWGrouper* Create() const override;

	// Verification des parametres
	boolean Check() const override;

	// Libelles utilisateur
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul du nombre maximum de lignes liees aux contraintes de la methode de groupage
	int ComputePreprocessedMaxLineNumber(KWFrequencyTable* table) const override;
};
