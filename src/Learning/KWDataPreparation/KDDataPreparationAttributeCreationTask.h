// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDDataPreparationAttributeCreationTask;
class KDDPBivariateCrossProductsCreationTask;

#include "KWDataPreparationTask.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWAttributeStats.h"
#include "KWDRMath.h"
#include "KWDRString.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDataPreparationAttributeCreationTask
// Classe ancetre d'un service de creation de nouveaux attributs et de leur preparation
class KDDataPreparationAttributeCreationTask : public KWDataPreparationTask
{
public:
	// Constructeur
	KDDataPreparationAttributeCreationTask();
	~KDDataPreparationAttributeCreationTask();

	// Parametrage du nombre max d'attribut a creer
	void SetMaxCreatedAttributeNumber(int nValue);
	int GetMaxCreatedAttributeNumber() const;

	// Calcul de la preparation des donnees en parallele
	// La methode construit de nouveaux attributs et leur preparation
	// En entree:
	//  . learningSpec: les specifications d'apprentissage, comprenant le dictionnaire et la base base source
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    incorporees en tant que ExtraAttribute dans le TupleTableLoader
	//  . dataTableSliceSet: la specification du decoupage de la base en tranche, avec les fichiers par tranche
	//  calcules . odInputAttributeStats: dictionnaire des preparation d'attributs (KWAttributeStats) par attribut
	//  du dictionnaire en entree
	// En sortie:
	//  . oaInputAttributeStats: tableau des preparations d'attributs (KWAttributeStats) par attribut construit
	//                           rajoute au dictionnaire en entree
	// En cas de succes, le dictionnaire en entree est enrichi et compile avec de nouveaux attributs, et pour chaque
	// attribut, sont cout est mis a jour et sa preparation est fournie. En cas d'erreur, le dictionnaire en entree
	// est restitue dans son etat d'origine, et aucune preparation n'est disponible Methode interruptible, retourne
	// false si erreur ou interruption (avec message), true sinon
	virtual boolean CreatePreparedAttributes(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
						 KWDataTableSliceSet* dataTableSliceSet,
						 ObjectDictionary* odInputAttributeStats,
						 ObjectArray* oaOutputAttributeStats);

	// Renvoie un rapport dedie aux variables construites (renvoie null par defaut)
	virtual KWLearningReport* GetCreationReport();

	// Prefixe utilise pour fabriquer un nom de rapport (defaut: "Created")
	virtual ALString GetReportPrefix() const;

	///////////////////////////////////////////////////////
	// Administration de la tache de creation d'attributs

	// Methode virtuelle de recopie des specification de creation d'attributs
	virtual void CopyAttributeCreationSpecFrom(const KDDataPreparationAttributeCreationTask* attributeCreationTask);

	// Memorisation global d'une tache de creation d'attributs (defaut: NULL)
	// Supprimer l'eventuelle tache precedente
	// Memoire: la tache appartient a l'appele, et devra etre detruit en appelant la methode avec NULL en parametre
	static void SetGlobalCreationTask(KDDataPreparationAttributeCreationTask* attributeCreationTask);

	// Recherche de la tache courante de creation d'attributs (defaut: NULL)
	// Cette tache pourra etre modifiee pour la parametrer
	static KDDataPreparationAttributeCreationTask* GetGlobalCreationTask();

	// Recherche et duplication de la tache courante de creation d'attributs, pour l'utiliser
	// Retourne NULL si inexistant
	static KDDataPreparationAttributeCreationTask* CloneGlobalCreationTask();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametrage du cout de construction d'un attribut construit, avec memorisation du cout dans les meta-donnees
	// et indication que la variable est construite (non initiale)
	void SetConstructedAttributeCost(KWAttribute* attribute, double dCost) const;

	// Test si tout attribut d'entre est prepare dans un des deux dictionnaires de preparation
	// Methode interne a utiliser dans les assertions
	boolean CheckPreparedAttributes(KWLearningSpec* learningSpec, ObjectDictionary* odInputAttributeStats,
					ObjectArray* oaOutputAttributeStats) const;

	// Nombre max d'attributs a creer
	int nMaxCreatedAttributeNumber;

	// Tache globale de creation d'attribut
	static KDDataPreparationAttributeCreationTask* globalAttributeCreationTask;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDPBivariateCrossProductsCreationTask
// Exemple simple de service de creation de nouveaux attributs et de leur preparation
// Creation de paire d'attributs basee sur une discretisation non supervisee prealable
// des attributs numeriques, et sur la concatenation des valeurs par paire d'attributs
class KDDPBivariateCrossProductsCreationTask : public KDDataPreparationAttributeCreationTask
{
public:
	// Constructeur
	KDDPBivariateCrossProductsCreationTask();
	~KDDPBivariateCrossProductsCreationTask();

	// Prefixe des variables a cree (defaut: "")
	void SetCreatedAttributePrefix(const ALString& sValue);
	const ALString& GetCreatedAttributePrefix() const;

	// Redefinition de la methode de creation d'attributs
	boolean CreatePreparedAttributes(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					 KWDataTableSliceSet* dataTableSliceSet,
					 ObjectDictionary* odInputAttributeStats,
					 ObjectArray* oaOutputAttributeStats) override;

	// Recopie des specification de creation d'attributs
	void
	CopyAttributeCreationSpecFrom(const KDDataPreparationAttributeCreationTask* attributeCreationTask) override;

	// Prefixe utilise pour fabriquer un nom de rapport
	ALString GetReportPrefix() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Collecte d'un ensemple aleatoire de paires d'indices (firstIndex, secondIndex), pour un index max en entree
	void CollectRandomPairIndexes(int nMaxIndex, int nMaxPairNumber, IntVector* ivPairFirstIndexes,
				      IntVector* ivPairSecondIndexes) const;

	// Creation d'un attribut a partir de deux attributs, et parametrage de son cout dans la regle de calcul
	// L'attribut cree appartient a l'appele, mais n'est pas dans une classe
	KWAttribute* CreateAttributeFrom(KWAttribute* attribute1, KWAttribute* attribute2,
					 KWAttributeStats* attributeStats1, KWAttributeStats* attributeStats2,
					 double dCost) const;

	// Creation d'un operande pour une regle de concatenation
	KWDerivationRuleOperand* CreateConcatOperandFrom(KWAttribute* attribute,
							 KWAttributeStats* attributeStats) const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean SlaveProcess() override;

	// Prefixe des variables a cree (defaut: "")
	ALString sCreatedAttributePrefix;
};
