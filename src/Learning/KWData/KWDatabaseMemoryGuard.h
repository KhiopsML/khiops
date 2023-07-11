// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDatabaseMemoryGuard;

#include "Object.h"
#include "MemoryManager.h"
#include "Ermgt.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// Class KWDatabaseMemoryGuard;
// Service de protection memoire pour la lecture des instances d'une base, notamment multi-table
// En effet, la lecture d'un enregistrement da la table principale peut impliquer potentiellement
// un tres grande nombre d'enregistrement de tables secondaires, pouvant potentiellement de pas
// tenir en memoire, et un graphe de calcul exploitant des variables de travail pouvant
// egalement poser un probleme sur sur-utilisation de la memoire.
// On parle dans ce cas d'instances "elephant".
// Le service de protection des acces memoire permet de monitorer la memoire lors de la lecture
// des enregistrements et du calcul des variables derivees de facon a eviter preventivement
// les depassements memoires et a les disgnostiquer le plus precisement possible
class KWDatabaseMemoryGuard : public Object
{
public:
	// Constructeur
	KWDatabaseMemoryGuard();
	~KWDatabaseMemoryGuard();

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametrage des limites a respecter pour la lecture d'un enregistrement principal
	// et de tous ses enregistrements de tables secondaires

	// Reinitialisation complete du parametrage et des donnees d'exploitation des limites
	void Reset();

	// Nombre d'enregistrements au dela duquel une alerte est declenchee
	// Cela ne declenche qu'un warning informatif en cas de depassement
	// Parametrage inactif si 0
	void SetMaxSecondaryRecordNumber(longint lValue);
	longint GetMaxSecondaryRecordNumber() const;

	// Limite de la memoire utilisable pour une instance pour gerer l'ensemble de la lecture et du calcul des
	// attributs derivee Attention: il s'agit ici d'une limite memoire physique dans la RAM, avec prise en compte de
	// l'overhead d'allocation Cela concerne toutes les methdoes liees a la memoire de cette classe Parametrage
	// inactif si 0
	void SetSingleInstanceMemoryLimit(longint lValue);
	longint GetSingleInstanceMemoryLimit() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des lectures et des calculs d'attributs d'un enregistrement

	// Initialisation du suivi des limites pour un nouvel enregistrement principal
	// A appeler avant la premiere lecture
	void Init();

	// Parametrage de la cle de l'objet principal, pour les messages d'erreur (facultatatif)
	void SetMainObjectKey(const ALString& sValue);
	const ALString& GetMainObjectKey() const;

	// Prise en compte de la lecture d'un nouvel enregistrement secondaire
	// A appeler apres la lecture des enregistrements
	void AddReadSecondaryRecord();

	// Prise en compte du calcul d'un nouvel attribut
	// A appeler apres la lecture de chaque nouvel enregistrement
	void AddComputedAttribute();

	// Mise a jour apres un nettoyage memoire, permettant potentiellement d'annuler le depassement de la limite
	// memoire
	void UpdateAfterMemoryCleaning();

	//////////////////////////////////////////////////////////////////////////////////////
	// Exploitation des limites
	// On peut ici emettre des warning specialises en fonction du type de limite atteinte ou depassee

	// Indique si le nombre max d'enregistrement est atteint, si le parametrage est actif
	boolean IsMaxSecondaryRecordNumberReached() const;

	// Nombre de fois ou on a du nettoyer la memoire pour continuer le calcul des attributs
	int GetMemoryCleaningNumber() const;

	// Indique si l'instance est tres large, mais a pu etre traitee
	boolean IsSingleInstanceVeryLarge() const;

	// Libelle personnalise associe a une instance de tres grande taille, mais ayant pu etre calculee correctement
	const ALString GetSingleInstanceVeryLargeLabel();

	// Indique si la limite memoire est atteinte, si le parametrage est actif
	// Cet indicateur est declenchee des que la limite a ete depasse une seule fois, et n'est reinitialise qu'avec
	// Init
	boolean IsSingleInstanceMemoryLimitReached() const;

	// Indique si la cause du depassement memoire est liee a la lecture des enregistrements
	boolean IsSingleInstanceMemoryLimitReachedDuringRead() const;

	// Libelle personnalise associe au depassement de la limite memoire, dans le cas de la lecture des
	// renregistrements ou dans le cas du calcul des attributs, avec perte du calcul des attributs derives,
	// remplaces par des valeur manquantes
	const ALString GetSingleInstanceMemoryLimitLabel();

	// Afin de permettre aux utilisateur d'obtenir ces warning, meme en cas de controle de flow des erreurs,
	// il faut installer un handler specifique pour ignorer le controle de flow des erreurs (cf.
	// Global::SetErrorFlowIgnoreFunction) Ces methodes sont a appeler a l'ouverture et la fermeture d'une
	// datatebase
	static void InstallMemoryGuardErrorFlowIgnoreFunction();
	static void UninstallMemoryGuardErrorFlowIgnoreFunction();

	//////////////////////////////////////////////////////////////////////////////////////
	// Statistiques sur les operations effectuees

	// Nombre d'enregistrements secondaires lus avant atteinte de la limite
	longint GetReadSecondaryRecordNumberBeforeLimit() const;

	// Nombre total d'enregistrements secondaires lus
	longint GetTotalReadSecondaryRecordNumber() const;

	// Nombre d'attributs calcules avant atteinte de la limite
	int GetComputedAttributeNumberBeforeLimit() const;

	// Nombre total d'attributs calcules
	int GetTotalComputedAttributeNumber() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Informations sur l'utilisation de la memoire dans la heap

	// Memoire initiale au moment de l'initialisation
	longint GetInitialHeapMemory() const;

	// Memoire courante apres la derniere operation de lecture ou de calcul
	longint GetCurrentHeapMemory() const;

	// Memoire courante utilisee, par difference avec la memoire initiale
	longint GetCurrentUsedHeapMemory() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametre avance pour effectuer des tests de crash par scenario
	// S'il sont actifs, les parametres de crash sont utlise en priorite
	// sur les parametres standard

	// Nombre d'enregistrements secondaires au dela duquel une alerte est declenchee
	// Cela ne declenche qu'un warning informatif en cas de depassement
	// Parametrage inactif si 0
	static void SetCrashTestMaxSecondaryRecordNumber(longint lValue);
	static longint GetCrashTestMaxSecondaryRecordNumber();

	// Limite a ne pas depasser de la memoire utilisable dans la heap pour gerer l'ensemble de la lecture et du
	// calcul des attributs derivee Parametrage inactif si 0
	static void SetCrashTestSingleInstanceMemoryLimit(longint lValue);
	static longint GetCrashTestSingleInstanceMemoryLimit();

	//////////////////////////////////////////////////////////////////////////////////////
	// Services standards

	// Affichage detaille des
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Constantes pour le dimensionnement des instances de tres grande taille

	// Bornes inf et sup des nombres max de records secondaires, pour le dimensionnement des taches
	static const longint GetDefautMinSecondaryRecordNumberLowerBound();
	static const longint GetDefautMaxSecondaryRecordNumberUpperBound();

	// Min et max du facteur entre le nombre de records secondaire et le nombre moyen de records secondaires
	static const int GetDefautMinSecondaryRecordNumberFactor();
	static const int GetDefautMaxSecondaryRecordNumberFactor();

	//////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des instances elephants, dont le nombre de records secondaires est tel
	// que l'instance ne peut pas etre traitee entierement en memoire.
	//
	// L'objet DatabaseMemoryGuard est un objet porte par une Database permettant de
	// controler en permanence l'ocuppation memoire, apres chaque lecture de record
	// et apres chaque calcul d'attribut des KWObject.
	// En effet, les calculs d'attribut impliquent une utilisation memoire potentiellement
	// importante en presence de nombreux records secondaire. Par exemple, une seule selection
	// sur un million de records occupe potentiellement 8 MB (un pointeur par record).
	//
	// L'objet DatabaseMemoryGuard permet de gerer les comportements suivants:
	//   . warning si beaucoup de records secondaires, mais traitement complet sans probleme
	//   . erreur si pas assez de memoire pour lire tous les records secondaires
	//   . erreur si pas assez de memoire pour calculer tous les attribut derives, apres
	//     avoir lus correctement tous les records secondaires
	// En cas d'erreur, on garde neanmoins les valeurs des variables natives de l'instance principale,
	// et on met toutes les autre valeurs a missing. Il s'agit d'un erreur pour forcer l'utilisateur a
	// analyser le probleme, mais cette erreur n'est pas bloquante dans le sens ou l'instance elephant
	// est neanmoins traitee, meme si c'est en mode degrade.
	//
	// En cas de probleme memoire pour le calcul des attributs derives, on tente une continuation du calcul
	// en nettoyant si necessaire les variables temporaires, generees pour accelerer l'ensemble de tous
	// les calculs liee a une instances. Pour cela (dans KWObject::ComputeAllValues), apres chaque calcul
	// d'attribut, si la limite memoire est atteinte, on nettoie les variables temporaires pour recuperer de
	// la memoire, et on continue si la memoire liberee est suffisante. Ainsi, on perd en temps de calcul
	// ce que l'on gagne en memoire. Le gain en memoire peut ainsi etre tres important et permettre de gerer
	// correctement des instances elephants de plus grande taille.
	// Il est a noter que l'on pourrait chercher a optimiser l'ordre des calculs de facon a minimiser le
	// nombre de nettoyages necessaires, et meme a nettoyer optimalement uniquement les variables temporaires
	// qui ne sont plus necessaires. Ce type de probleme est une variante de "graph topological ordering"
	// proche du probleme de "one-shot (black) pebbling".
	//  cf. https://cs.stackexchange.com/questions/60772/finding-an-optimal-topological-ordering
	// Il s'agit d'un probleme dont meme l'aproximation est NP-complet.
	// La solution optimale n’est donc pas atteignable, et les heuristiques de resolution du probleme
	// sont non triviales a mettre au point. De plus, pour assurer une empreinte memoire minimale, on devra
	// se ramener potentiellement a la solution pragmatique, qui periodiquement peut etre amenee a tout nettoyer.
	// Le rapport cout-benefice de cette solution etendue est alors extrement defavorable, avec un cout
	// exorbitant pour potentiellement ameliorer le temps de calcul dans des cas rares pour gerer le cas
	// deja tres rare des instances elephants. Cette solution est abandonnee.
	//
	// Le dimensionnement des limites en nombre de records secondaires et en memoire est a la charge des taches.
	// Cela ne peut etre qu'un dimensionnement heuristique rapide, qui ne doit pas impacter l'efficacite
	// des traitements nominaux en l'absence d'instances elephants.
	// Il est a noter que la detection des instances elephants peut differer en fonction de la tache
	// (extraction des quantiles, preparation, evaluation, deploiement) et des ressources disponibles.
	// Il est ici important de ne pas ignorer les instances elephants (on les garde ici de facon degradee),
	// de facon a garantir que le le nombre d'instances traitees soit le meme tout au long du processus de
	// modelisation, alors que certaines instances elephants peuvent etre detectees on non selon les phases
	// du processus.

	// Variables d'instance
	ALString sMainObjectKey;
	longint lSingleInstanceMemoryLimit;
	longint lInitialHeapMemory;
	longint lMaxHeapMemory;
	longint lCurrentHeapMemory;
	longint lMaxSecondaryRecordNumber;
	longint lReadSecondaryRecordNumberBeforeLimit;
	longint lTotalReadSecondaryRecordNumber;
	int nComputedAttributeNumberBeforeLimit;
	int nTotalComputedAttributeNumber;
	int nMemoryCleaningNumber;
	boolean bIsSingleInstanceMemoryLimitReached;

	// Variable d'instances a prendre en compte, selon la valeur des parametres de crash test
	longint lActualMaxSecondaryRecordNumber;
	longint lActualSingleInstanceMemoryLimit;

	// Parametres des crash tests
	static longint lCrashTestMaxSecondaryRecordNumber;
	static longint lCrashTestSingleInstanceMemoryLimit;

	// Bornes inf et sup des nombres max de records secondaires
	static const longint lDefautMinSecondaryRecordNumberLowerBound = 100000;
	static const longint lDefautMaxSecondaryRecordNumberUpperBound = 10000000;

	// Min et max du facteur entre le nombre de records secondaires et le nombre moyen de records secondaires
	static const int nDefautMinSecondaryRecordNumberFactor = 100;
	static const int nDefautMaxSecondaryRecordNumberFactor = 10000;

	///////////////////////////////////////////////////////////////////////////////////////////
	// Gestion de la methode specifique pour ignorer le controle de flow des erreurs

	// Implementation de la methode specifique pour ignorer le controle de flow des erreurs
	static boolean MemoryGuardErrorFlowIgnoreFunction(const Error* e, boolean bDisplay);

	// Compteur d'installation de la methode specifiques, pour permettre plusieurs utilisation simultannees
	// On ne desisnatelle que si ce compteur repasse a 0
	static int nMemoryGuardFunctionUseCount;

	// Compteur d'erreur par type de message
	static int nMemoryGuardInformationWarningNumber;
	static int nMemoryGuardRecoveryWarningNumber;

	// Partie des libelle permettant d'identifier  les message emius par le memory guard
	static const ALString sMemoryGuardLabelPrefix;
	static const ALString sMemoryGuardRecoveryLabelSuffix;
};

///////////////////////
// Methodes en inline

inline boolean KWDatabaseMemoryGuard::IsMaxSecondaryRecordNumberReached() const
{
	return lActualMaxSecondaryRecordNumber > 0 and
	       GetTotalReadSecondaryRecordNumber() > lActualMaxSecondaryRecordNumber;
}

inline boolean KWDatabaseMemoryGuard::IsSingleInstanceMemoryLimitReached() const
{
	return bIsSingleInstanceMemoryLimitReached;
}

inline const longint KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberLowerBound()
{
	return lDefautMinSecondaryRecordNumberLowerBound;
}

inline const longint KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberUpperBound()
{
	return lDefautMaxSecondaryRecordNumberUpperBound;
}

inline const int KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberFactor()
{
	return nDefautMinSecondaryRecordNumberFactor;
}

inline const int KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberFactor()
{
	return nDefautMaxSecondaryRecordNumberFactor;
}
