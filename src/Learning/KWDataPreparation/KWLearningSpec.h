// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWLearningService;
class KWLearningSpec;
class KWDescriptiveStats;
class PLShared_LearningSpec;

#include "KWVersion.h"
#include "KWClass.h"
#include "KWObject.h"
#include "KWDatabase.h"
#include "KWDatabaseIndexer.h"
#include "KWTupleTable.h"
#include "KWPreprocessingSpec.h"
#include "KWDataGridStats.h"
#include "PLSharedObject.h"

/////////////////////////////////////////////////////////////
// Classe KWLearningService
// Classe parametree par des specifications d'apprentissage,
// destinees a etre partagee par toute une gamme de services.
// Cette classe est destinee a etre classe ancetre de toutes
// les classes de service ayant trait a l'apprentissage
class KWLearningService : public Object
{
public:
	// Constructeur et destructeur
	KWLearningService();
	~KWLearningService();

	// Parametrage par des specifications d'apprentissage
	// Memoire: les specifications sont referencees
	// et destinee a etre partagees par plusieurs services
	virtual void SetLearningSpec(KWLearningSpec* specification);
	KWLearningSpec* GetLearningSpec() const;

	//////////////////////////////////////////////////////////
	// Acces direct aux elements de specification
	// Prerequis: validite des specifications (Check)

	// Description courte
	const ALString& GetShortDescription() const;

	// Classe
	KWClass* GetClass() const;

	// Base de donnees
	KWDatabase* GetDatabase() const;

	// Attribut cible
	const ALString& GetTargetAttributeName() const;

	// Type de l'attribut cible (Continuous, Symbol, ou None en nom supervise)
	int GetTargetAttributeType() const;

	// Modalite cible principale
	Symbol& GetMainTargetModality() const;

	// Parametrage des algorithme de pretraitement (discretisation et groupage)
	// Le nombre total d'attribut est parametre dans les structures de couts des algorithmes
	KWPreprocessingSpec* GetPreprocessingSpec();

	//////////////////////////////////////////////////////////////
	// Statistiques calculees a partir de la base (cf KWLearningSpec)

	// Nombre d'instances de la base
	int GetInstanceNumber() const;

	// Statistique descriptives de l'attribut cible
	KWDescriptiveStats* GetTargetDescriptiveStats() const;

	// Statistique des valeurs de l'attribut cible
	KWDataGridStats* GetTargetValueStats() const;

	// Index de la modalite principale dans les valeurs cibles (-1 eventuellement)
	virtual int GetMainTargetModalityIndex() const;

	// Couts de construction, preparation et des donnees du modele nul
	double GetNullCost() const;
	double GetNullConstructionCost() const;
	double GetNullPreparationCost() const;
	double GetNullDataCost() const;

	////////////////////////////////////////////
	// Parametrage avance

	// Nombre max de valeurs prise en comptes dans les rapports
	static int GetMaxModalityNumber();

	// Verification de la presence et de la validite des specifications
	boolean Check() const override;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Indique si le groupage de la cible est effectue par defaut ou pas
	boolean IsTargetGrouped() const;

	//////////////////////////////////////////////////////////
	//// Implementation
protected:
	KWLearningSpec* learningSpec;
};

//////////////////////////////////////////////////
// Classe KWLearningSpec
// Specification d'un probleme d'apprentissage
// Services de base
class KWLearningSpec : public Object
{
public:
	// Constructeur et destructeur
	KWLearningSpec();
	~KWLearningSpec();

	// Description courte du probleme d'apprentissage
	void SetShortDescription(const ALString& sValue);
	const ALString& GetShortDescription() const;

	///////////////////////////////////////////////////////
	// Definition de la classe et des donnees a discretiser
	// Les donnees de specification sont uniquement referencees

	// Classe concernee
	// Provoque la reinitialisation des statistiques sur l'eventuel attribut cible
	//  sauf si la classe ne change pas de nom
	// Memoire: la classe n'est que referencee (geree par l'appelant)
	void SetClass(KWClass* kwcValue);
	KWClass* GetClass() const;

	// Base de donnees
	// Provoque la reinitialisation des statistiques sur l'eventuel attribut cible
	// Memoire: la base n'est que referencee (geree par l'appelant)
	void SetDatabase(KWDatabase* databaseObjects);
	KWDatabase* GetDatabase() const;

	// Attribut cible
	// Si un attribut cible est specifie, on est dans le cas de
	// l'apprentissage supervise
	// Provoque la reinitialisation des statistiques sur l'eventuel attribut cible
	void SetTargetAttributeName(const ALString& sValue);
	const ALString& GetTargetAttributeName() const;

	// Type de l'attribut cible (Continuous, Symbol, ou None en nom supervise)
	// Automatiquement synchronise avec la classe et le nom de l'attribut cible
	int GetTargetAttributeType() const;

	// Modalite principale de l'attribut cible
	// Utile dans le cas d'un attribut cible booleenne pour presenter tous les
	// resultats par rapport a une unique valeur de reference
	// Si valeur non trouvee parmi es valeurs cibles, option ignoree
	void SetMainTargetModality(const Symbol& sValue);
	Symbol& GetMainTargetModality() const;

	// Parametrage des algorithme de pretraitement (discretisation et groupage)
	// Par defaut: MODL
	// Memoire: appartient aux specifications
	KWPreprocessingSpec* GetPreprocessingSpec();

	///////////////////////////////////////////////////////////////////////////////
	// Calcul de statistiques descriptives sur l'attribut cible

	// Calcul des statistiques de l'attribut cible (descriptives et des valeurs)
	// La table de tuples en parametre peut ne contenir aucun attribut le cas non supervise
	// (mais donne acces a l'effectif de la base), et contient l'attribut cible en premiere position sinon
	boolean ComputeTargetStats(const KWTupleTable* targetTupleTable);

	// Indique si les stats sont calculees
	// La modification des specification d'apprentissage (base, dictionnaire, attribut cible...)
	// invalide la calcul des stats
	boolean IsTargetStatsComputed() const;

	// Nombre d'instances de la base
	int GetInstanceNumber() const;

	// Statistique descriptives de l'attribut cible
	// Retourne une objet KWDescriptiveContinuousStats ou KWDescriptiveSymbolStats, ou null
	// selon le type de l'attribut
	// Automatiquement synchronise avec le type de l'attribut cible
	// Memoire: appartient aux specifications
	KWDescriptiveStats* GetTargetDescriptiveStats() const;

	// Statistique des valeurs de l'attribut cible
	// Retourne un objet KWDataGridsStats univarie dans le cas supervise, null sinon
	// Memoire: appartient aux specifications
	KWDataGridStats* GetTargetValueStats() const;

	// Index de la modalite principale dans les valeurs cibles (-1 eventuellement)
	int GetMainTargetModalityIndex() const;

	// Couts de construction, preparation et des donnees du modele null
	// Retourne 0 dans le cas non supervise
	double GetNullCost() const;
	double GetNullConstructionCost() const;
	double GetNullPreparationCost() const;
	double GetNullDataCost() const;

	/////////////////////////////////////////////////////////////////////////
	// Parametrage avance pour reutiliser l'indexation d'une base, prealable
	// aux taches paralleles traitant les base de donnees.
	// Cet objet est ici juste memorise pour y avoir acces en permanence,
	// et de le reutiliser partout ou c'est nceessaire.
	// Aucune synchronisation avec la Database des LearningSpec n'est effectuee.
	// Cette synchronisation est effectuée dans la classe KWDatabaseTask qui
	// prend en parametre la database et le DatabaseIndexer dans sa methode
	// principale RunDatabaseTask

	// Acces au service d'indexation d'une base
	KWDatabaseIndexer* GetDatabaseIndexer();

	///////////////////////////////////////////////////////////////////////
	// Parametrage des familles de construction de variables
	// Une variable peut-etre initialement presente dans la classe de depart,
	// ou construite en multi-table, via des arbres, ou via des paires de variables.
	// Les arbres ou les paires ne peuvent par contre exploiter que des variables
	// initiales ou construites en multi-table.
	// On parametre ici l'utilisation de chaque famille de construction de variable,
	// si elle est a la fois specifiee par l'utilisateur et possible en theorie/
	// Par exemple, meme si on veut des arbres, ce n'est pas possible en regression,
	// ou si il n'y a qu'une variable initiale et pas de construction de variables.
	// Cela permet d'avoir ensuite acces au nombre de familles effectivement utilisees,
	// et aux cout de construction a utiliser:
	//   . n: nombre de variable initiales
	//   . F: nombre de variable construites
	//   . cout du choix de la famille: ln(n+F)

	// Parametrage du nombre de variables initiales presente dans la classe de depart
	void SetInitialAttributeNumber(int nValue);
	int GetInitialAttributeNumber() const;

	// Construction de variable en multi-table
	void SetMultiTableConstruction(boolean bValue);
	boolean GetMultiTableConstruction() const;

	// Construction de variable poour le texte
	void SetTextConstruction(boolean bValue);
	boolean GetTextConstruction() const;

	// Construction de variables de type arbres, uniquement en classification
	void SetTrees(boolean bValue);
	boolean GetTrees() const;

	// Construction de variables de type paires, comptees uniquement en classification supervise
	void SetAttributePairs(boolean bValue);
	boolean GetAttributePairs() const;

	// Nombre de famille de construction de variables
	int GetConstructionFamilyNumber() const;

	// Cout du choix d'un attribut en analyse univariee: ln(n+F)
	// A ce cout, il faut ajouter le cout de construction pour les attributs non initiaux
	double GetSelectionCost() const;

	// Cout du choix d'un attribut pour les methodes avancee de construction, par arbres ou paires.
	// Ces methodes ne peuvent utiliser que des attributs initiaux ou issus de la construction
	// de variables multi-table.
	// Valeur du parametre a utiliser:
	//   . pour les arbres: methode GetTextConstructionUsedByTrees
	//   . pour les paires: true
	double GetBasicSelectionCost(boolean bUsingTextConstruction) const;

	////////////////////////////////////////////////////////////////////
	// Parametrage avance

	// Indique si les arbres peuvent utiliser les variables construites a partir des texte (defaut: false)
	// Attention: n'appeler cette methode si necessaire qu'une seule fois, dans le main du programme
	//
	// Les variables de type arbre n’utilisent que des variables construites, au moins dans un premier temps.
	// De fait, les variables de type texte sont en general très nombreuses et impliquent un tres grand nombre
	// de variables individuellement tres peu informatives (faible level).Les arbres sont moins performants
	// sur ce type de distribution de variables, et il ne semble pas raisonnable de risquer de degrader leur
	// performance pour un gain potentiel probablement negligeable. La combinaison de plusieurs variables de type
	// texte, ou de variable de type texte et multi-table, ne présente probablement pas d’apport significatif.
	static void SetTextConstructionUsedByTrees(boolean bValue);
	static boolean GetTextConstructionUsedByTrees();

	// Nombre max de valeurs prise en comptes dans les rapports (defaut: 1000000)
	// notament pour les dimensions des grilles de preparation des donnees
	static void SetMaxModalityNumber(int nValue);
	static int GetMaxModalityNumber();

	// Indique s'il faut grouper la cible, c'est a dire si cela est demande en pretraitement et
	// que l'on est dans le cas de la classification supervisee avec des pretraitements MODL
	boolean IsTargetGrouped() const;

	////////////////////////////////////////////////////////////////////
	// Services avances

	// Calcul des stats sur les valeurs d'un attribut Continuous
	// On alimente une grille univariee dont l'attribut est de la classe KWDGSAttributeContinuousValues
	// Prerequis: la table de tuples en entree doit comporter l'attribut en premiere position
	boolean ComputeContinuousValueStats(const ALString& sAttributeName, const KWTupleTable* tupleTable,
					    KWDataGridStats* valueStats);

	// Calcul des stats sur les valeurs d'un attribut Symbol
	//   SortByDecreasingFrequencies: tri par effectif decroissant, sinon, par nom
	//     Dans ce cas, on garde au plus une valeur singleton d'effectif 1, les autres etant
	//      fusionneees dans la valeur speciale Symbol::GetStarValue
	// On alimente une grille univariee dont l'attribut est de la classe KWDGSAttributeSymbolValues
	// Les valeurs sont triees par effectifs decroissant dans la grille de valeurs resultats
	// Prerequis: la table de tuples en entree doit comporter l'attribut en premiere position
	boolean ComputeSymbolValueStats(const ALString& sAttributeName, const KWTupleTable* tupleTable,
					boolean bSortByDecreasingFrequencies, KWDataGridStats* valueStats);

	///////////////////////////////////////////////////////////////////
	// Fonctionnalites standard

	// Duplication
	// Les objets references Class et Database sont les memes, le reste est duplique
	KWLearningSpec* Clone() const;

	// Recopie
	void CopyFrom(const KWLearningSpec* kwlsSource);

	// Recopie uniquement des statistiques sur la cible
	// Attention, methode avancee pouvant entrainer des incoherences
	// Permet de rappatrier ces infos, si elles ont ete invalidee par parametrage du dictionnaire ou de la database
	void CopyTargetStatsFrom(const KWLearningSpec* kwlsSource);

	// Verification de la validite de la definition
	boolean Check() const override;

	// Parametrage de la verification ou non de la presence de l'attribut cible dans la classe (defaut: true)
	// Methode avancee, utile si l'on a deja collecte les stats sur l'attribut cible, mais que l'on en
	// a plus besoin de cet attribut pour l'analyse des attributs descriptifs
	void SetCheckTargetAttribute(boolean bValue);
	boolean GetCheckTargetAttribute() const;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Reinitialisation des statistiques descriptives
	void ResetTargetStats();

	// Calcul des couts du model null
	void ComputeNullCost();

	// Calcul de l'index de la valeur cible principale (-1 si aucune)
	// parmi les valeurs cibles
	int ComputeMainTargetModalityIndex() const;

	// Attributs principaux
	ALString sShortDescription;
	KWClass* kwcClass;
	KWDatabase* database;
	ALString sTargetAttributeName;
	int nTargetAttributeType;
	mutable Symbol sMainTargetModality;
	KWPreprocessingSpec preprocessingSpec;
	boolean bIsTargetStatsComputed;
	int nInstanceNumber;
	KWDescriptiveStats* targetDescriptiveStats;
	KWDataGridStats* targetValueStats;
	int nMainTargetModalityIndex;

	// Service d'indexation d'une base
	KWDatabaseIndexer databaseIndexer;

	// Couts du modele null
	double dNullConstructionCost;
	double dNullPreparationCost;
	double dNullDataCost;

	// Parametrage des cout de selection, via les familles de construction de variables utilisees
	int nInitialAttributeNumber;
	boolean bMultiTableConstruction;
	boolean bTextConstruction;
	boolean bTrees;
	boolean bAttributePairs;
	int nConstructionFamilyNumber;

	// Parametres avances
	boolean bCheckTargetAttribute;
	static boolean bTextConstructionUsedByTrees;
	static int nMaxModalityNumber;

	friend class PLShared_LearningSpec;
};

////////////////////////////////////////////////////////////
// Classe PLShared_LearningSpec
//	 Serialisation de la classe KWLearningSpec
class PLShared_LearningSpec : public PLSharedObject
{
public:
	// Constructeur
	PLShared_LearningSpec();
	~PLShared_LearningSpec();

	// Acces aux spec
	void SetLearningSpec(KWLearningSpec* learningSpec);
	KWLearningSpec* GetLearningSpec() const;

	////////////////////////////////////////////////////////////////
	// Finalisation des specifications de la variable partagee
	// Attention, seules les caracteristiques propres aux specification d'apprentissage
	// sont serialisees (y compris les statistiques sur l'attribut cible si elle sont presentes)
	// Les complements de specification (Classe et Database) sont des objets externes
	// references, devant etre rataches explicitement par appel des methode concernees
	// de KWLearningSpec

	// Methode a appeler apres la deserialisation pour finaliser les specifications
	// Les LearningSpec sont dans un etat valide apres l'appel de cette methode
	void FinalizeSpecification(KWClass* kwcValue, KWDatabase* databaseObjects);

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////////////
// Implementation en inline

inline void KWLearningService::SetLearningSpec(KWLearningSpec* specification)
{
	learningSpec = specification;
}

inline KWLearningSpec* KWLearningService::GetLearningSpec() const
{
	return learningSpec;
}

inline KWClass* KWLearningService::GetClass() const
{
	require(Check());
	return learningSpec->GetClass();
}

inline KWDatabase* KWLearningService::GetDatabase() const
{
	require(Check());
	return learningSpec->GetDatabase();
}

inline const ALString& KWLearningService::GetShortDescription() const
{
	require(Check());
	return learningSpec->GetShortDescription();
}

inline const ALString& KWLearningService::GetTargetAttributeName() const
{
	require(Check());
	return learningSpec->GetTargetAttributeName();
}

inline int KWLearningService::GetTargetAttributeType() const
{
	require(Check());
	return learningSpec->GetTargetAttributeType();
}

inline Symbol& KWLearningService::GetMainTargetModality() const
{
	require(Check());
	return learningSpec->GetMainTargetModality();
}

inline int KWLearningService::GetInstanceNumber() const
{
	return learningSpec->GetInstanceNumber();
}

inline KWDescriptiveStats* KWLearningService::GetTargetDescriptiveStats() const
{
	return learningSpec->GetTargetDescriptiveStats();
}

inline KWDataGridStats* KWLearningService::GetTargetValueStats() const
{
	return learningSpec->GetTargetValueStats();
}

inline int KWLearningService::GetMainTargetModalityIndex() const
{
	return learningSpec->GetMainTargetModalityIndex();
}

inline double KWLearningService::GetNullCost() const
{
	return learningSpec->GetNullCost();
}

inline double KWLearningService::GetNullConstructionCost() const
{
	return learningSpec->GetNullConstructionCost();
}

inline double KWLearningService::GetNullPreparationCost() const
{
	return learningSpec->GetNullPreparationCost();
}

inline double KWLearningService::GetNullDataCost() const
{
	return learningSpec->GetNullDataCost();
}

inline int KWLearningService::GetMaxModalityNumber()
{
	return KWLearningSpec::GetMaxModalityNumber();
}

inline boolean KWLearningService::IsTargetGrouped() const
{
	require(Check());
	return learningSpec->IsTargetGrouped();
}

///////////////////////////////////////////////////

inline KWClass* KWLearningSpec::GetClass() const
{
	return kwcClass;
}

inline KWDatabase* KWLearningSpec::GetDatabase() const
{
	return database;
}

inline const ALString& KWLearningSpec::GetTargetAttributeName() const
{
	return sTargetAttributeName;
}

inline int KWLearningSpec::GetTargetAttributeType() const
{
	return nTargetAttributeType;
}

inline void KWLearningSpec::SetMainTargetModality(const Symbol& sValue)
{
	sMainTargetModality = sValue;
	nMainTargetModalityIndex = ComputeMainTargetModalityIndex();
}

inline Symbol& KWLearningSpec::GetMainTargetModality() const
{
	return sMainTargetModality;
}

inline KWPreprocessingSpec* KWLearningSpec::GetPreprocessingSpec()
{
	return &preprocessingSpec;
}

inline boolean KWLearningSpec::IsTargetStatsComputed() const
{
	return bIsTargetStatsComputed;
}

inline int KWLearningSpec::GetInstanceNumber() const
{
	require(bIsTargetStatsComputed);
	return nInstanceNumber;
}

inline KWDescriptiveStats* KWLearningSpec::GetTargetDescriptiveStats() const
{
	require(bIsTargetStatsComputed);
	return targetDescriptiveStats;
}

inline KWDataGridStats* KWLearningSpec::GetTargetValueStats() const
{
	require(bIsTargetStatsComputed);
	return targetValueStats;
}

inline int KWLearningSpec::GetMainTargetModalityIndex() const
{
	require(bIsTargetStatsComputed);
	return nMainTargetModalityIndex;
}

inline double KWLearningSpec::GetNullCost() const
{
	require(bIsTargetStatsComputed);
	return dNullConstructionCost + dNullPreparationCost + dNullDataCost;
}

inline double KWLearningSpec::GetNullConstructionCost() const
{
	require(bIsTargetStatsComputed);
	return dNullConstructionCost;
}

inline double KWLearningSpec::GetNullPreparationCost() const
{
	require(bIsTargetStatsComputed);
	return dNullPreparationCost;
}

inline double KWLearningSpec::GetNullDataCost() const
{
	require(bIsTargetStatsComputed);
	return dNullDataCost;
}

inline KWDatabaseIndexer* KWLearningSpec::GetDatabaseIndexer()
{
	return &databaseIndexer;
}

inline void KWLearningSpec::SetInitialAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nInitialAttributeNumber = nValue;
}

inline int KWLearningSpec::GetInitialAttributeNumber() const
{
	require(nInitialAttributeNumber != -1);
	return nInitialAttributeNumber;
}

inline void KWLearningSpec::SetMultiTableConstruction(boolean bValue)
{
	if (bMultiTableConstruction)
		nConstructionFamilyNumber--;
	bMultiTableConstruction = bValue;
	if (bMultiTableConstruction)
		nConstructionFamilyNumber++;
}

inline boolean KWLearningSpec::GetMultiTableConstruction() const
{
	return bMultiTableConstruction;
}

inline void KWLearningSpec::SetTextConstruction(boolean bValue)
{
	if (bTextConstruction)
		nConstructionFamilyNumber--;
	bTextConstruction = bValue;
	if (bTextConstruction)
		nConstructionFamilyNumber++;
}

inline boolean KWLearningSpec::GetTextConstruction() const
{
	return bTextConstruction;
}

inline void KWLearningSpec::SetTrees(boolean bValue)
{
	if (bTrees)
		nConstructionFamilyNumber--;
	bTrees = bValue;
	if (bTrees)
		nConstructionFamilyNumber++;
}

inline boolean KWLearningSpec::GetTrees() const
{
	return bTrees;
}

inline void KWLearningSpec::SetAttributePairs(boolean bValue)
{
	if (bAttributePairs)
		nConstructionFamilyNumber--;
	bAttributePairs = bValue;
	if (bAttributePairs)
		nConstructionFamilyNumber++;
}

inline boolean KWLearningSpec::GetAttributePairs() const
{
	return bAttributePairs;
}

inline int KWLearningSpec::GetConstructionFamilyNumber() const
{
	require(not bTrees or GetTargetAttributeType() == KWType::Symbol);
	require(not bAttributePairs or GetTargetAttributeType() == KWType::Symbol);
	ensure(nConstructionFamilyNumber == bMultiTableConstruction + bTextConstruction + bTrees + bAttributePairs);
	return nConstructionFamilyNumber;
}

inline double KWLearningSpec::GetSelectionCost() const
{
	return log(max(1, GetInitialAttributeNumber() + GetConstructionFamilyNumber()));
}

inline double KWLearningSpec::GetBasicSelectionCost(boolean bUsingTextConstruction) const
{
	if (bUsingTextConstruction)
		return log(max(1, GetInitialAttributeNumber() + bMultiTableConstruction + bTextConstruction));
	else
		return log(max(1, GetInitialAttributeNumber() + bMultiTableConstruction));
}

inline void KWLearningSpec::SetTextConstructionUsedByTrees(boolean bValue)
{
	bTextConstructionUsedByTrees = bValue;
}

inline boolean KWLearningSpec::GetTextConstructionUsedByTrees()
{
	return bTextConstructionUsedByTrees;
}

inline void KWLearningSpec::SetMaxModalityNumber(int nValue)
{
	require(nValue >= 0);
	nMaxModalityNumber = nValue;
}

inline int KWLearningSpec::GetMaxModalityNumber()
{
	return nMaxModalityNumber;
}
