// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTGrouperMODL.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe DTGrouperMODLBasic

double DTGrouperMODLBasic::dEpsilon = 1e-6;

DTGrouperMODLBasic::DTGrouperMODLBasic() {}

DTGrouperMODLBasic::~DTGrouperMODLBasic() {}

const ALString DTGrouperMODLBasic::GetName() const
{
	return "MODLBasic";
}

KWGrouper* DTGrouperMODLBasic::Create() const
{
	return new DTGrouperMODLBasic;
}

void DTGrouperMODLBasic::Group(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const
{
	boolean bDisplayBestCosts = false;
	boolean bDisplayAllCosts = false;
	boolean bDisplayDetails = false;
	int nActualMaxGroupNumber;
	int nActualMinGroupFrequency;
	int nReducedLineNumber;
	int nInitialLastMergedLineFrequency;
	int nInitialGarbageMinLineFrequency;
	double dInitialGarbageGroupCost;
	double dInitialModelCost;
	double dGarbageGroupCost;
	double dModelCost;
	double dLineCost;
	double dTotalLineCost;
	double dDeltaCost;
	double dBestDeltaCost;
	int nGroupNumber;
	int nBestGroupNumber;
	IntVector ivGarbageGroupFrequencyVector;
	IntVector ivLineFrequencyVector;
	int nSource;
	int nTarget;

	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorNumber() > 1);
	require(kwftSource->GetFrequencyVectorSize() > 1);
	require(kwftSource->IsTableSortedBySourceFrequency(false));

	// Calcul des contraintes reelles d'effectif minimum par groupe
	// et de nombre maximum de groupes
	nActualMaxGroupNumber = GetMaxGroupNumber();
	if (nActualMaxGroupNumber == 0 or nActualMaxGroupNumber > kwftSource->GetFrequencyVectorNumber())
		nActualMaxGroupNumber = kwftSource->GetFrequencyVectorNumber();
	nActualMinGroupFrequency = GetMinGroupFrequency();
	if (nActualMinGroupFrequency > kwftSource->GetFrequencyVectorAt(0)->ComputeTotalFrequency())
		nActualMinGroupFrequency = kwftSource->GetFrequencyVectorAt(0)->ComputeTotalFrequency();

	// Calcul du nombre de lignes a fusionner inconditionnellement pour des
	// contraintes de pretraitements
	//   - il y en aura au plus MaxGroupNumber
	//   - toutes doivent avoir au moins l'effectif minimum, y compris
	//     le groupe garbage resultant de la fusion des lignes en sous-effectif
	nReducedLineNumber = ComputeTableReducedLineNumber(kwftSource, nActualMaxGroupNumber, nActualMinGroupFrequency);
	nInitialLastMergedLineFrequency =
	    kwftSource->GetFrequencyVectorAt(nReducedLineNumber - 1)->ComputeTotalFrequency();

	// S'il y a eu des fusions, toute ligne d'effectif egale a celui d'une ligne fusionnee
	// doit egalement etre fusionne (par coherence avec l'algorithme principal)
	if (nReducedLineNumber < kwftSource->GetFrequencyVectorNumber())
	{
		while (nReducedLineNumber > 2)
		{
			if (kwftSource->GetFrequencyVectorAt(nReducedLineNumber - 2)->ComputeTotalFrequency() >
			    nInitialLastMergedLineFrequency)
				break;
			else
				nReducedLineNumber--;
		}
		assert(kwftSource->GetFrequencyVectorAt(nReducedLineNumber - 1)->ComputeTotalFrequency() ==
		       nInitialLastMergedLineFrequency);
		assert(nReducedLineNumber == 1 or
		       kwftSource->GetFrequencyVectorAt(nReducedLineNumber - 2)->ComputeTotalFrequency() >
			   nInitialLastMergedLineFrequency);
	}

	// Initialisation du groupe garbage avec la fin de la table
	ivGarbageGroupFrequencyVector.SetSize(kwftSource->GetFrequencyVectorSize());
	for (nSource = nReducedLineNumber - 1; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
	{
		for (nTarget = 0; nTarget < ivGarbageGroupFrequencyVector.GetSize(); nTarget++)
			ivGarbageGroupFrequencyVector.UpgradeAt(
			    nTarget, cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSource))
					 ->GetFrequencyVector()
					 ->GetAt(nTarget));
	}

	// Calcul de la frequence minimum initiale par ligne correspondant au groupe garbage
	nInitialGarbageMinLineFrequency = 1;
	if (nReducedLineNumber < kwftSource->GetFrequencyVectorNumber())
		nInitialGarbageMinLineFrequency = nInitialLastMergedLineFrequency + 1;

	// Memorisation des caracteristiques initiales du groupe garbage
	dInitialModelCost = ComputeGarbageCost(nInitialGarbageMinLineFrequency) +
			    ComputeModelCost(nReducedLineNumber, nActualMaxGroupNumber);
	dInitialGarbageGroupCost = ComputeGroupCost(&ivGarbageGroupFrequencyVector);

	// Affichage de la ligne d'entete des details de calculs
	if (bDisplayAllCosts)
		cout << "Group Nb"
		     << "\t"
		     << "Min fq"
		     << "\t"
		     << "Line cost"
		     << "\t"
		     << "Total line cost"
		     << "\t"
		     << "Min fq cost"
		     << "\t"
		     << "Garbage cost"
		     << "\t"
		     << "Initial min fq cost"
		     << "\t"
		     << "Initial garbage cost"
		     << "\t"
		     << "Delta Cost"
		     << "\t"
		     << "Best Delta Cost"
		     << "\t"
		     << "Best Group Nb" << endl;

	// La table en entree est triee par effectif ligne decroissant et la fin de la
	// table (a partir de nReducedLineNumber) est consideree comme groupee
	// inconditionnellement a cause des contraintes initiales.
	// On va chercher parmi les possibilites suivantes la solution de meilleur
	// cout, ou ce qui revient au meme la meilleure variation de cout.
	// Le critere d'ajout est l'effectif minimum: on ne peut distinguer deux lignes
	// que par leur effectif suite a un tri. Subsequement, les lignes sont ajoutees
	// en bloc de lignes de meme effectifs.
	// On pousse l'algorithme jusqu'a obtenir un seul groupe terminal pour eviter
	// de tomber dans un minimum local. Le meilleur nombre de groupe est memorise.
	ivLineFrequencyVector.SetSize(kwftSource->GetFrequencyVectorSize());
	nGroupNumber = nReducedLineNumber;
	nBestGroupNumber = nGroupNumber;
	dLineCost = 0;
	dGarbageGroupCost = 0;
	dModelCost = 0;
	dTotalLineCost = 0;
	dDeltaCost = 0;
	dBestDeltaCost = 0;
	for (nSource = nReducedLineNumber - 2; nSource >= 0; nSource--)
	{
		// Calcul du cout de la ligne a rajouter au groupe garbage
		for (nTarget = 0; nTarget < ivLineFrequencyVector.GetSize(); nTarget++)
			ivLineFrequencyVector.SetAt(
			    nTarget, cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSource))
					 ->GetFrequencyVector()
					 ->GetAt(nTarget));
		dLineCost = ComputeGroupCost(&ivLineFrequencyVector);
		dTotalLineCost += dLineCost;

		// Cumul de la ligne dans le groupe fourre-tout
		for (nTarget = 0; nTarget < ivLineFrequencyVector.GetSize(); nTarget++)
			ivGarbageGroupFrequencyVector.UpgradeAt(nTarget, ivLineFrequencyVector.GetAt(nTarget));

		// Si detection de fin d'un groupe de lignes de meme effectif
		// (debut de table, ou changement d'effectif), test du nouveau groupage resultant
		if (nSource == 0 or (kwftSource->GetFrequencyVectorAt(nSource - 1)->ComputeTotalFrequency() >
				     kwftSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency()))
		{
			// Calcul des caracteristiques du groupe garbage
			dModelCost =
			    ComputeGarbageCost(kwftSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency() + 1) +
			    ComputeModelCost(nSource + 1, nActualMaxGroupNumber);
			dGarbageGroupCost = ComputeGroupCost(&ivGarbageGroupFrequencyVector);

			// Mise de la variation de cout globale
			dDeltaCost = dModelCost + dGarbageGroupCost - dTotalLineCost -
				     (dInitialModelCost + dInitialGarbageGroupCost);

			// Memorisation si amelioration de la meilleure variation de cout
			if (dDeltaCost < dBestDeltaCost + dEpsilon)
			{
				dBestDeltaCost = dDeltaCost;
				nBestGroupNumber = nSource + 1;

				// Affichage de l'amelioration
				if (bDisplayBestCosts)
					cout << nBestGroupNumber << "\t" << dBestDeltaCost << endl;
			}
		}

		// Affichage des details de calculs
		if (bDisplayAllCosts)
			cout << nSource + 1 << "\t"
			     << kwftSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency() << "\t" << dLineCost
			     << "\t" << dTotalLineCost << "\t" << dModelCost << "\t" << dGarbageGroupCost << "\t"
			     << dInitialModelCost << "\t" << dInitialGarbageGroupCost << "\t" << dDeltaCost << "\t"
			     << dBestDeltaCost << "\t" << nBestGroupNumber << endl;

		// Affichage des details des lignes
		if (bDisplayDetails)
		{
			cout << "\tLine";
			for (nTarget = 0; nTarget < ivLineFrequencyVector.GetSize(); nTarget++)
				cout << "\t" << ivLineFrequencyVector.GetAt(nTarget);
			cout << endl;
			cout << "\tGarbage";
			for (nTarget = 0; nTarget < ivGarbageGroupFrequencyVector.GetSize(); nTarget++)
				cout << "\t" << ivGarbageGroupFrequencyVector.GetAt(nTarget);
			cout << endl;
		}
	}

	// Creation de la table cible
	kwftTarget = BuildReducedTable(kwftSource, nBestGroupNumber);

	// Creation du vecteur d'index des groupes
	ivGroups = new IntVector;
	ivGroups->SetSize(nBestGroupNumber);
	for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
		ivGroups->SetAt(nSource, nSource);

	ensure(kwftTarget->GetTotalFrequency() == kwftSource->GetTotalFrequency());
}

boolean DTGrouperMODLBasic::Check() const
{
	boolean bOk;
	bOk = dParam == 0;
	if (not bOk)
		AddError("The main parameter of the algorithm must be 0");
	return bOk;
}

double DTGrouperMODLBasic::ComputeGarbageCost(int nGarbageMinSize) const
{
	// On utilise pas le nombre de groupe (ComputeModelCost()) pour regulariser
	boolean bUseGarbageCost = false;
	double dCost;

	require(nGarbageMinSize >= 1);

	// Cout optionnel du choix de la contrainte d'effectif min par groupe
	dCost = 0;
	if (bUseGarbageCost)
	{
		// Cout choix entre modele nul et modele informatif
		dCost = log(2.0);

		// Cout du choix de l'effectif minimal des groupes
		dCost = KWStat::NaturalNumbersUniversalCodeLength(nGarbageMinSize);
	}

	return dCost;
}

double DTGrouperMODLBasic::ComputeModelCost(int nGroupNumber, int nValueNumber) const
{
	boolean bUseModelCost = true;
	double dCost;

	require(nGroupNumber >= 1);
	require(nGroupNumber <= nValueNumber);

	// Cout optionnel du choix de la contrainte de nombre de groupes
	dCost = 0;
	if (bUseModelCost)
	{
		// Cout choix entre modele nul et modele informatif
		dCost = log(2.0);

		// Cout de codage du nombre de groupes
		if (nGroupNumber >= 2)
			dCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nGroupNumber - 1, nValueNumber - 1);
	}

	return dCost;
}

double DTGrouperMODLBasic::ComputeGroupCost(IntVector* ivFrequencyVector) const
{
	double dCost;
	int nFrequency;
	int nGroupFrequency;
	int i;

	require(ivFrequencyVector != NULL);
	require(ivFrequencyVector->GetSize() > 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nGroupFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost -= KWStat::LnFactorial(nFrequency);
		nGroupFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nGroupFrequency + ivFrequencyVector->GetSize() - 1);
	dCost -= KWStat::LnFactorial(ivFrequencyVector->GetSize() - 1);
	return dCost;
}
