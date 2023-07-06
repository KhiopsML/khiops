// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHStreamBining.h"

////////////////////////////////////////////////////////////////
// Classe MHStreamBiningMixed
// Gestion de la binarisation d'un ensemble de valeurs
// avec des bin orientes pour moitie largeur egale et
// pour moitie virgule flotante
class MHStreamBiningMixed : public MHStreamBining
{
public:
	// Constructeur
	MHStreamBiningMixed();
	~MHStreamBiningMixed();

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion en flux de la binarisation d'un ensemble de valeurs

	// Initialisation de la gestion du flux
	void InitializeStream() override;

	// Indique si le flux est initialise
	boolean IsStreamInitialized() const override;

	// Ajout d'une valeur
	void AddStreamValue(Continuous cValue) override;

	// Finalisation de la gestion du flux
	void FinalizeStream() override;

	// Statistiques globales sur le flux
	Continuous GetStreamLowerValue() const override;
	Continuous GetStreamUpperValue() const override;
	int GetStreamFrequency() const override;

	// Obtention des bins du resume dans un tableau
	void ExportStreamBins(ObjectArray* oaStreamBins) const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Gestion de la binarisation oriente largeur egale et virgule flotante
	MHStreamBining floatingPointStreamBining;
	MHStreamBining equalWidthStreamBining;
};
