// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTupleTableLoader;

#include "KWTupleTable.h"
#include "KWDatabase.h"

///////////////////////////////////////////////////////////////////////////////////
// Service d'alimentation d'une table de tuples a partir d'un tableau
// d'objets d'une base de donnees
class KWTupleTableLoader : public Object
{
public:
	/// Constructeur
	KWTupleTableLoader();
	~KWTupleTableLoader();

	/////////////////////////////////////////////////////////////////////////////////////
	// Parametrage du service, en specifiant la classe et les objets d'une base en entree
	// plus un attribut supplementaire optionnel, specifie par son nom,
	// son type, ses valeurs (dans le meme ordre que les objets de la base en entree)
	// et la table de tuple de ses valeurs
	// L'attribut supplementaire est naturellement destine a accueillir l'attribut
	// cible dans les analyses supervisees, et doit au minimum contenir une table
	// de tuple sans attribut (mais initialise avec le bon effectif) dans le cas non supervise

	// Dictionnaire en entree
	void SetInputClass(const KWClass* kwcValue);
	const KWClass* GetInputClass() const;

	// Tableau d'objets (KWObject) de la base en entree
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	void SetInputDatabaseObjects(const ObjectArray* oaDatabaseObjects);
	const ObjectArray* GetInputDatabaseObjects() const;

	// Nom de l'attribut supplementaire a utiliser (vide si non utilise)
	void SetInputExtraAttributeName(const ALString& sValue);
	const ALString& GetInputExtraAttributeName() const;

	// Type de l'attribut supplementaire (Unknown aucune specification)
	void SetInputExtraAttributeType(int nValue);
	int GetInputExtraAttributeType() const;

	// Valeurs de l'attribut supplementaire dans le cas d'un type Symbol
	// Memoire: le vecteur appartient a l'appelant
	void SetInputExtraAttributeSymbolValues(const SymbolVector* svValues);
	const SymbolVector* GetInputExtraAttributeSymbolValues() const;

	// Valeurs de l'attribut supplementaire dans le cas d'un type Continuous
	// Memoire: le vecteur appartient a l'appelant
	void SetInputExtraAttributeContinuousValues(const ContinuousVector* cvValues);
	const ContinuousVector* GetInputExtraAttributeContinuousValues() const;

	// Table de tuple memorisant les valeurs de l'attribut en entree avec leur effectif
	// Memoire: la table de tuple appartient a l'appelant
	void SetInputExtraAttributeTupleTable(const KWTupleTable* tupleTable);
	const KWTupleTable* GetInputExtraAttributeTupleTable() const;

	// Verification de la specification des parametres en entree
	// Il doit y avoir la base d'objet en entree et/ou l'attribut supplementaire
	// et ses valeurs, specifies de facon coherente
	boolean CheckInputs() const;

	// Parametrage de la verification ou non de la coherence entre la classe en entree et les objets de la base
	// Methode avancee, utile si l'on change la classe apres avoir avoir charge les objets de la base
	// Attention: usage risque
	void SetCheckDatabaseObjectClass(boolean bValue);
	boolean GetCheckDatabaseObjectClass() const;

	// Supression de tous les parametres en entree
	void RemoveAllInputs();

	// Supression/destruction des parametres relatif a l'attribut supplmementaire
	void RemoveExtraAttributeInputs();
	void DeleteExtraAttributeInputs();

	// Creation et specification de tous les parametres en entree pour l'attribut supplementaire
	// a partir des donnees disponibles dans la base en entree
	// Seule cette methode impose la presence de l'attribut supplementaire dans le dictionnaire et la base
	// Seule la table de tuple est initialisee en l'absence d'attribut supplementaire
	void CreateAllExtraInputParameters(const ALString& sExtraAttributeName);

	/////////////////////////////////////////////////////////////////////////////
	// Alimentation d'un vecteur de valeurs a partir de la base en entree
	// independamment de l'attribut supplementaire
	// Alimentation directe d'une table de tuple
	// Peut servir a parametrer les valeurs de l'attribut supplementaire

	// Alimentation d'un vecteur de valeurs Continuous
	void LoadContinuousValues(const ALString& sAttributeName, ContinuousVector* cvOutputValues);

	// Alimentation d'un vecteur de valeurs Continuous
	void LoadSymbolValues(const ALString& sAttributeName, SymbolVector* svOutputValues);

	// Alimentation d'une table de tuples univariee a partir d'un vecteur de valeur
	void LoadTupleTableFromContinuousValues(const ALString& sAttributeName, const ContinuousVector* cvInputValues,
						KWTupleTable* outputTupleTable);
	void LoadTupleTableFromSymbolValues(const ALString& sAttributeName, const SymbolVector* svInputValues,
					    KWTupleTable* outputTupleTable);

	// Alimentation d'une table de tuples sans valeur a partir d'un nombre d'instance
	// Permet de gerer les effet de bord, avec des tables de tuple ayant 0 attribut (cas non supervise)
	void LoadTupleTableFromFrequency(int nInputFrequency, KWTupleTable* outputTupleTable);

	/////////////////////////////////////////////////////////////////////////////
	// Alimentation a partir d'un ou plusieurs attributs du tableau d'objets
	// (KWObject) d'une base de donnees, plus optionnellement de l'attribut
	// supplementaire dont les valeurs doivent etre synchronisees avec les objets du tableau.
	// Tous les attributs en entree doivent etre de type simple et distincts deux a deux.
	// La table de tuple en sortie est nettoyee, puis specifiee selon le type
	// des attributs en entree, et enfin alimentee a partir des objets de la base

	// Alimentation univariee, plus eventuellement l'attribut supplementaire
	void LoadUnivariate(const ALString& sInputAttributeName, KWTupleTable* outputTupleTable) const;

	// Alimentation bivariee, plus eventuellement l'attribut supplementaire
	void LoadBivariate(const ALString& sInputAttributeName1, const ALString& sInputAttributeName2,
			   KWTupleTable* outputTupleTable) const;

	// Alimentation multivariee, plus eventuellement l'attribut supplementaire
	void LoadMultivariate(const StringVector* svInputAttributeNames, KWTupleTable* outputTupleTable) const;

	/////////////////////////////////////////////////////////////////////////////
	// Alimentation a partir d'un blocs d'attributs du tableau d'objets et
	// d'un dictionnaire d'attributs specifies dans ce bloc, plus optionnellement
	// de l'attribut supplementaire.
	// En sortie, le dictionnaire contient une table du tuple par attribut specifie
	//
	// L'utilisation de deux phases, Initialize et Finalize, permet d'optimiser la memoire.
	// Lors de l'initialization, toutes les phases sont en mode Update, plus consommateur en
	// memoire, mais elles ne contiennent que les valeurs effectivement presentes dans la base.
	// Lors de la finalisation, il faut integrer les tuples associes a la valeur manquante,
	// potentiellement nombreux dans le cas de la classification avec de nombreuses valeurs
	// cible ou dans le cas de la regression. La table finalisee, en mode non Update, peut
	// donc parfois etre plus volumineuse.
	// Pour gagner en memoire, on peut donc finaliser, exploiter et detruire chaque table
	// finalise une apres l'autre.

	// Alimentation univariee, plus eventuellement l'attribut supplementaire,
	// pour tous les attributs specifies du bloc en entree
	// Les tables de tuples sont creees et memorisees dans le dictionnaire en sortie
	// Seules les valeurs presentes ont ete lues : les tables de tuples sont donc toujours en etat Update
	// et doivent etre finalisees pour etre utilisables
	void BlockLoadUnivariateInitialize(const ALString& sInputAttributeBlockName,
					   ObjectDictionary* odInputAttributes,
					   ObjectDictionary* odOutputTupleTables) const;

	// Finalisation d'une table de tuple initialise avec la methode d'alimentation par bloc
	void BlockLoadUnivariateFinalize(const ALString& sInputAttributeBlockName,
					 KWTupleTable* outputTupleTable) const;

	// Alimentation bivariee (avec le deuxieme attribut en entree), plus eventuellement
	// l'attribut supplementaire, pour tous les attributs specifies du bloc en entree
	// Les tables de tuples sont creees et memorisees dans le dictionnaire en sortie
	// avec pour chaque table bivariee, les attributs dans l'ordre alphabetique
	// L'utilisation des deux phases n'est pas possible ici, en raison d'un contexte de
	// de travail important a gerer (variables temporaires liees aux entree, traitement
	// mutualise de recalcul de table de tuples univariee) necessaire pour finaliser
	// l'ensemble des tables de tuples
	void BlockLoadBivariate(const ALString& sInputAttributeBlockName, ObjectDictionary* odInputAttributes,
				const ALString& sInputAttributeName2, ObjectDictionary* odOutputTupleTables) const;

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	// Parametre en entree du service
	const KWClass* kwcInputClass;
	const ObjectArray* oaInputDatabaseObjects;
	ALString sInputExtraAttributeName;
	int nInputExtraAttributeType;
	const SymbolVector* svInputExtraAttributeSymbolValues;
	const ContinuousVector* cvInputExtraAttributeContinuousValues;
	const KWTupleTable* inputExtraAttributeTupleTable;
	boolean bCheckDatabaseObjectsClass;
};

//////////////////////////////////////////////////////
// Methodes en inline

inline void KWTupleTableLoader::SetInputExtraAttributeName(const ALString& sValue)
{
	sInputExtraAttributeName = sValue;
}

inline const ALString& KWTupleTableLoader::GetInputExtraAttributeName() const
{
	return sInputExtraAttributeName;
}

inline void KWTupleTableLoader::SetInputExtraAttributeType(int nValue)
{
	require(nValue == KWType::Symbol or nValue == KWType::Continuous or nValue == KWType::None or
		nValue == KWType::Unknown);
	nInputExtraAttributeType = nValue;
}

inline int KWTupleTableLoader::GetInputExtraAttributeType() const
{
	return nInputExtraAttributeType;
}
