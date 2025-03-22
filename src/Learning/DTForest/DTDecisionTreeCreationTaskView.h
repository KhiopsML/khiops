// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KDDataPreparationAttributeCreationTaskView.h"
#include "DTDecisionTreeCreationTask.h"
#include "DTDecisionTreeParameterView.h"
#include "DTForestParameterView.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTaskView
class DTDecisionTreeCreationTaskView : public KDDataPreparationAttributeCreationTaskView
{
public:
	// Constructeur
	DTDecisionTreeCreationTaskView();
	~DTDecisionTreeCreationTaskView();

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par le predicteur specifique
	void EventRefresh(Object* object) override;

	void SetObject(Object* object) override;
};
