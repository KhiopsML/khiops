// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseCheckTask
// Verification de base en parallele
class KWDatabaseCheckTask : public KWDatabaseTask
{
public:
	// Constructeur
	KWDatabaseCheckTask();
	~KWDatabaseCheckTask();

	// Verification de l'integrite d'une base
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean CheckDatabase(const KWDatabase* sourceDatabase);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Le simple fait d'heriter de KWDatabaseCheckTask va permetre de relire toute la base
	// et de detecter toutes les erreurs sans aucun traitement additionnel

	// Reimplementation de l'affichage des messages
	void DisplaySpecificTaskMessage() override;

	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
};
