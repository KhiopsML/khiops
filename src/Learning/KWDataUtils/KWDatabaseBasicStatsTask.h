// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseBasicStatsTask
// Calcul de statistiques de base en parallele
class KWDatabaseBasicStatsTask : public KWDatabaseTask
{
public:
	// Constructeur
	KWDatabaseBasicStatsTask();
	~KWDatabaseBasicStatsTask();

	// Calcul de statistiques de base en parallele, dans la limite d'un nombre max en fonction des ressources
	//  . comptage du nombre d'enregistrements
	//  . collecte eventuelle d'un vecteur de valeurs pour un attribut cible, si specifie
	// En sortie:
	//  . lRecordNumber contient le nombre d'enregistrements lus (0 si erreur)
	//  . lCollectedObjectNumber contient le nombre d'enregistrements traites, sous ensemble des enregistrements lus
	//  (0 si erreur) . svCollectedValues ou cvCollectedValues contient les valeurs collectes, dans l'ordre de la
	//  base (vide si erreur)
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean CollectBasicStats(const KWDatabase* sourceDatabase, const ALString& sTargetAttributeName,
				  longint& lRecordNumber, longint& lCollectedObjectNumber,
				  SymbolVector* svCollectedValues, ContinuousVector* cvCollectedValues);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Nom de l'attribut cible
	PLShared_String shared_sTargetAttributeName;

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves

	// Flag de collecte des valeurs par l'esclave
	PLShared_Boolean input_bCollectValues;

	// Vecteur de valeurs numeriques ou categorielles
	PLShared_SymbolVector output_svReadValues;
	PLShared_ContinuousVector output_cvReadValues;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	// Atttribut cible
	KWAttribute* slaveTargetAttribute;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Indicateur de collecte des valeurs
	boolean bMasterCollectValues;

	// Vecteur total des valeurs numeriques ou categorielles
	SymbolVector svReadValues;
	ContinuousVector cvReadValues;

	// Tableau de tous les resultats des esclaves (vecteurs de valeurs)
	// Uniquement si collecte des valeurs
	ObjectArray oaAllSlaveResults;

	// Attribut cible
	KWAttribute* masterTargetAttribute;

	// Dictionnaire des valeurs cible dans la cas categoriel
	NumericKeyDictionary nkdAllSymbolValues;

	// Memoire de stockage des valeurs par le maitre
	longint lMasterAllValuesUsedMemory;

	// Memoire allouee au stockage des valeurs par le maitre
	longint lMasterAllValuesGrantedMemory;

	// Nombre max de valeurs distinctes dans le cas categoriel
	static const int nMaxSymbolValueNumber = 1000000;
};
