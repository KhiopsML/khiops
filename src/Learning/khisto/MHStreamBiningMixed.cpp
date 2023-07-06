// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHStreamBiningMixed.h"

MHStreamBiningMixed::MHStreamBiningMixed()
{
	floatingPointStreamBining.SetFloatingPointGrid(true);
	equalWidthStreamBining.SetFloatingPointGrid(false);
}

MHStreamBiningMixed::~MHStreamBiningMixed() {}

void MHStreamBiningMixed::InitializeStream()
{
	// On parametre chacun des flux pour traiter la moitie des bins
	// Plus precisement, on prend la moitie+1, car le nombre total de bins de deux flux de taille K et K' est
	// inferieur K+K'-1
	floatingPointStreamBining.SetMaxBinNumber((GetMaxBinNumber() + 2) / 2);
	equalWidthStreamBining.SetMaxBinNumber(
	    max(1, GetMaxBinNumber() + 1 - floatingPointStreamBining.GetMaxBinNumber()));

	// On parametre egalement l'option de saturation
	floatingPointStreamBining.SetSaturatedBins(GetSaturatedBins());
	equalWidthStreamBining.SetSaturatedBins(GetSaturatedBins());

	// Initialisation de chaque flux
	floatingPointStreamBining.InitializeStream();
	equalWidthStreamBining.InitializeStream();
}

boolean MHStreamBiningMixed::IsStreamInitialized() const
{
	return floatingPointStreamBining.IsStreamInitialized();
}

void MHStreamBiningMixed::AddStreamValue(Continuous cValue)
{
	floatingPointStreamBining.AddStreamValue(cValue);
	equalWidthStreamBining.AddStreamValue(cValue);
}

void MHStreamBiningMixed::FinalizeStream()
{
	floatingPointStreamBining.FinalizeStream();
	equalWidthStreamBining.FinalizeStream();
}

Continuous MHStreamBiningMixed::GetStreamLowerValue() const
{
	return floatingPointStreamBining.GetStreamLowerValue();
}

Continuous MHStreamBiningMixed::GetStreamUpperValue() const
{
	return floatingPointStreamBining.GetStreamUpperValue();
}

int MHStreamBiningMixed::GetStreamFrequency() const
{
	return floatingPointStreamBining.GetStreamFrequency();
}

void MHStreamBiningMixed::ExportStreamBins(ObjectArray* oaStreamBins) const
{
	ObjectArray oaFloatingPointStreamBins;
	ObjectArray oaEqualWidthStreamBins;
	int nFloatingPointIndex;
	int nEqualWidthIndex;
	MHBin* floatingPointBin;
	MHBin* equalWidthBin;
	MHBin generalBin;
	int nTotalFrequency;
	int nFloatingPointBinRemainingFrequency;
	int nEqualWidthBinRemainingFrequency;

	require(IsStreamInitialized());
	require(floatingPointStreamBining.globalStatsBin.GetFrequency() ==
		equalWidthStreamBining.globalStatsBin.GetFrequency());
	require(oaStreamBins != NULL);
	require(oaStreamBins->GetSize() == 0);

	// Cas particulier d'au plus un seul bin
	if (floatingPointStreamBining.slBins.GetCount() <= 1 and equalWidthStreamBining.slBins.GetCount() <= 1)
	{
		if (floatingPointStreamBining.globalStatsBin.GetFrequency() > 0)
			oaStreamBins->Add(floatingPointStreamBining.globalStatsBin.Clone());
	}
	// Cas ou un des flux n'a qu'un seul bin
	else if (floatingPointStreamBining.slBins.GetCount() <= 1 or equalWidthStreamBining.slBins.GetCount() <= 1)
	{
		if (floatingPointStreamBining.slBins.GetCount() <= 1)
			equalWidthStreamBining.ExportStreamBins(oaStreamBins);
		else
			floatingPointStreamBining.ExportStreamBins(oaStreamBins);
	}
	// Cas general
	else
	{
		// Export des bins de chaque type
		equalWidthStreamBining.ExportStreamBins(&oaEqualWidthStreamBins);
		floatingPointStreamBining.ExportStreamBins(&oaFloatingPointStreamBins);

		// Initialisation du premier bin virgule flotante
		nFloatingPointIndex = 0;
		floatingPointBin = NULL;
		nFloatingPointBinRemainingFrequency = 0;

		// Initialisation du premier bin largeur egale
		nEqualWidthIndex = 0;
		equalWidthBin = NULL;
		nEqualWidthBinRemainingFrequency = 0;

		// Parcours synchronise des bins pour les exporter
		nTotalFrequency = 0;
		while (nTotalFrequency < floatingPointStreamBining.globalStatsBin.GetFrequency())
		{
			assert(nFloatingPointBinRemainingFrequency >= 0);
			assert(nEqualWidthBinRemainingFrequency >= 0);

			// Preparation du bin virgule flotante suivant
			if (nFloatingPointBinRemainingFrequency == 0)
			{
				floatingPointBin = cast(MHBin*, oaFloatingPointStreamBins.GetAt(nFloatingPointIndex));
				nFloatingPointIndex++;
				nFloatingPointBinRemainingFrequency = floatingPointBin->GetFrequency();

				// Parametrage de la valeur inf du prochain bin general
				generalBin.SetLowerValue(floatingPointBin->GetLowerValue());
			}

			// Preparation du bin largeur egale suivant
			if (nEqualWidthBinRemainingFrequency == 0)
			{
				equalWidthBin = cast(MHBin*, oaEqualWidthStreamBins.GetAt(nEqualWidthIndex));
				nEqualWidthIndex++;
				nEqualWidthBinRemainingFrequency = equalWidthBin->GetFrequency();

				// Parametrage de la valeur inf du prochain bin general
				generalBin.SetLowerValue(equalWidthBin->GetLowerValue());
			}

			// Alimentation avec le bin virgule flotante si sa borne sup est la plus proche
			if (floatingPointBin->GetUpperValue() < equalWidthBin->GetUpperValue())
			{
				// Parametrage
				generalBin.SetUpperValue(floatingPointBin->GetUpperValue());
				generalBin.SetFrequency(nFloatingPointBinRemainingFrequency);

				// Decompte de l'effectif de chaque bin
				nFloatingPointBinRemainingFrequency = 0;
				nEqualWidthBinRemainingFrequency -= generalBin.GetFrequency();
			}
			// Alimentation avec le bin largeur egale si sa borne sup est la plus proche
			else if (floatingPointBin->GetUpperValue() > equalWidthBin->GetUpperValue())
			{
				// Parametrage
				generalBin.SetUpperValue(equalWidthBin->GetUpperValue());
				generalBin.SetFrequency(nEqualWidthBinRemainingFrequency);

				// Decompte de l'effectif de chaque bin
				nEqualWidthBinRemainingFrequency = 0;
				nFloatingPointBinRemainingFrequency -= generalBin.GetFrequency();
			}
			// On prend en compte les deux bins si egalite
			else
			{
				// Parametrage
				generalBin.SetUpperValue(floatingPointBin->GetUpperValue());
				generalBin.SetFrequency(nFloatingPointBinRemainingFrequency);

				// Decompte de l'effectif de chaque bin
				nEqualWidthBinRemainingFrequency -= generalBin.GetFrequency();
				nFloatingPointBinRemainingFrequency -= generalBin.GetFrequency();
			}

			// Ecriture du bin
			oaStreamBins->Add(generalBin.Clone());
			nTotalFrequency += generalBin.GetFrequency();
		}
		assert(nTotalFrequency == floatingPointStreamBining.globalStatsBin.GetFrequency());
		assert(nFloatingPointBinRemainingFrequency == 0);
		assert(nEqualWidthBinRemainingFrequency == 0);
		assert(nFloatingPointIndex == oaFloatingPointStreamBins.GetSize());
		assert(nEqualWidthIndex == oaEqualWidthStreamBins.GetSize());

		// Nettoyage
		oaFloatingPointStreamBins.DeleteAll();
		oaEqualWidthStreamBins.DeleteAll();
	}
	ensure(GetBinArrayTotalFrequency(oaStreamBins) == GetStreamFrequency());
}