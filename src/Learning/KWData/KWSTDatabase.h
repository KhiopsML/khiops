// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabase.h"
#include "KWDataTableDriver.h"

//////////////////////////////////////////////////////////////////////
// Base simple table (STDatabase: Single Table Database)
class KWSTDatabase : public KWDatabase
{
public:
	// Constructeur
	KWSTDatabase();
	~KWSTDatabase();

	// Creation dynamique
	KWDatabase* Create() const override;

	// Verification du format de la base
	boolean CheckFormat() const override;

	// Recopie des attributs de definition
	void CopyFrom(const KWDatabase* kwdSource) override;

	// Comparaison des attributs de definition avec une autre base du meme type
	int Compare(const KWDatabase* kwdSource) const override;

	// Reimplementation de la methode de parametrage du mode d'affichage des messages
	// Propagation au driver
	void SetVerboseMode(boolean bValue) override;
	void SetSilentMode(boolean bValue) override;

	// Acces au driver de table utilise pour les acces aux fichiers
	KWDataTableDriver* GetDataTableDriver() override;

	// Memoire utilisee par la database pour son fonctionnement
	longint GetUsedMemory() const override;

	// Memoire necessaire pour ouvrir la base
	longint ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Creation de driver, a l'usage des mappings des tables principales et secondaires
	virtual KWDataTableDriver* CreateDataTableDriver() const;

	// Reimplementation d'une partie des methodes virtuelles de KWDatabase
	boolean BuildDatabaseClass(KWClass* kwcDatabaseClass) override;
	boolean IsTypeInitializationManaged() const override;
	boolean PhysicalOpenForRead() override;
	boolean PhysicalOpenForWrite() override;
	boolean IsPhysicalEnd() const override;
	KWObject* PhysicalRead() override;
	void PhysicalSkip() override;
	void PhysicalWrite(const KWObject* kwoObject) override;
	boolean PhysicalClose() override;
	void PhysicalDeleteDatabase() override;
	longint GetPhysicalEstimatedObjectNumber() override;
	double GetPhysicalReadPercentage() override;
	longint GetPhysicalRecordIndex() const override;

	// Driver de gestion des acces a une table
	// Template pour la creation de driver du bon type
	KWDataTableDriver* dataTableDriverCreator;
};