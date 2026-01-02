// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridStats.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridStats

KWDataGridStats::KWDataGridStats()
{
	nSourceAttributeNumber = 0;
	nMainTargetModalityIndex = -1;
	nGranularity = 0;
	cvJSONAttributeDomainLowerBounds = NULL;
	cvJSONAttributeDomainUpperBounds = NULL;
}

KWDataGridStats::~KWDataGridStats()
{
	oaAttributes.DeleteAll();
	assert(cvJSONAttributeDomainLowerBounds == NULL);
	assert(cvJSONAttributeDomainUpperBounds == NULL);
}

void KWDataGridStats::SetGranularity(int nValue)
{
	require(nValue >= 0);
	nGranularity = nValue;
}
int KWDataGridStats::GetGranularity() const
{
	return nGranularity;
}

void KWDataGridStats::AddAttribute(const KWDGSAttributePartition* attribute)
{
	require(attribute != NULL);
	require(attribute->Check());
	require(ivCellFrequencies.GetSize() == 0);

	// Destruction prealable de toutes les cellules
	DeleteAllCells();

	// Ajout de l'attribut
	oaAttributes.Add(cast(Object*, attribute));
}

int KWDataGridStats::GetAttributeNumber() const
{
	return oaAttributes.GetSize();
}

void KWDataGridStats::SetSourceAttributeNumber(int nValue)
{
	require(0 <= nValue and nValue <= GetAttributeNumber());
	require(GetAttributeNumber() == 0 or nValue < GetAttributeNumber());
	nSourceAttributeNumber = nValue;
}

int KWDataGridStats::GetSourceAttributeNumber() const
{
	return nSourceAttributeNumber;
}

int KWDataGridStats::GetFirstTargetAttributeIndex() const
{
	return nSourceAttributeNumber;
}

int KWDataGridStats::GetTargetAttributeNumber() const
{
	require(GetAttributeNumber() - GetSourceAttributeNumber() > 0);
	return GetAttributeNumber() - GetSourceAttributeNumber();
}

int KWDataGridStats::GetPredictionAttributeNumber() const
{
	if (GetSourceAttributeNumber() > 0)
		return GetSourceAttributeNumber();
	else
		return GetAttributeNumber();
}

const KWDGSAttributePartition* KWDataGridStats::GetAttributeAt(int nAttributeIndex) const
{
	return cast(const KWDGSAttributePartition*, oaAttributes.GetAt(nAttributeIndex));
}

const KWDGSAttributePartition* KWDataGridStats::SearchAttribute(const ALString& sAttributeName) const
{
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	// Recherche sequentielle
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		if (attribute->GetAttributeName() == sAttributeName)
			return attribute;
	}
	return NULL;
}

boolean KWDataGridStats::CheckAttributes() const
{
	boolean bOk = true;
	ALString sTmp;
	int nAttribute;
	const KWDGSAttributePartition* attribute;
	ObjectDictionary odAttributes;
	double dDataGridSize;
	int nMaxDataGridSize;

	// Verification du nombre d'attributs
	if (GetAttributeNumber() == 0)
	{
		AddError("No variable in data grid");
		bOk = false;
	}

	// Verification du nombre d'attributs en entree (entre 0 et K-1)
	if (GetSourceAttributeNumber() >= GetAttributeNumber())
	{
		AddError(sTmp + "The number of source variables (" + IntToString(GetSourceAttributeNumber()) +
			 ") must be stricly inferior to the total number of variables (" +
			 IntToString(GetAttributeNumber()) + ")");
		bOk = false;
	}

	// Verification de l'unicite du nom des attributs de la grille
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			assert(attribute->Check());

			// Utilisation d'un dictionnaire pour tester l'utilisation des attributs
			if (odAttributes.Lookup(attribute->GetAttributeName()) == NULL)
				odAttributes.SetAt(attribute->GetAttributeName(), cast(Object*, attribute));
			else
			{
				AddError("Variable " + attribute->GetAttributeName() + " is used several times");
				bOk = false;
				break;
			}
		}
	}

	// Verification de la taille max de la grille
	if (bOk)
	{
		// Calcul de la taille de la grille
		dDataGridSize = 1;
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			dDataGridSize *= attribute->GetPartNumber();
		}

		// Test de depassement de la capacite d'indexage des cellules
		nMaxDataGridSize = INT_MAX / 2;
		if (dDataGridSize > nMaxDataGridSize)
		{
			AddError(sTmp + "Data grid size (" + DoubleToString(dDataGridSize) + ") too large");
			bOk = false;
		}
	}

	return bOk;
}

void KWDataGridStats::DeleteAll()
{
	DeleteAllCells();
	oaAttributes.DeleteAll();
	nSourceAttributeNumber = 0;
	nMainTargetModalityIndex = -1;
}

void KWDataGridStats::CreateAllCells()
{
	require(CheckAttributes());
	require(ivCellFrequencies.GetSize() == 0);
	ivCellFrequencies.SetSize(ComputeTotalGridSize());
}

boolean KWDataGridStats::CheckCells() const
{
	return ivCellFrequencies.GetSize() == ComputeTotalGridSize();
}

void KWDataGridStats::DeleteAllCells()
{
	ivCellFrequencies.SetSize(0);
}

int KWDataGridStats::ComputeGridFrequency() const
{
	int nGridFrequency;
	int nCell;

	require(CheckCells());

	// Comptage des effectifs des cellules
	nGridFrequency = 0;
	for (nCell = 0; nCell < ivCellFrequencies.GetSize(); nCell++)
	{
		assert(nGridFrequency * 1.0 + ivCellFrequencies.GetAt(nCell) < INT_MAX);
		nGridFrequency += ivCellFrequencies.GetAt(nCell);
	}
	return nGridFrequency;
}

int KWDataGridStats::ComputeSourceCellNumber() const
{
	ObjectArray oaSourceCells;
	int nSourceCellNumber;

	require(CheckCells());
	require(GetSourceAttributeNumber() > 0);

	// Export des cellules sources uniquement, pour comptage
	// On se base par defaut sur le premier attribut cible
	ExportSourceCellsAt(&oaSourceCells, GetFirstTargetAttributeIndex());

	// Comptage des cellules non vides
	nSourceCellNumber = oaSourceCells.GetSize();

	// Nettoyage
	oaSourceCells.DeleteAll();
	return nSourceCellNumber;
}

int KWDataGridStats::ComputeTotalCellNumber() const
{
	int nCellNumber;
	int nCell;

	require(CheckCells());

	// Comptage des cellules non vides
	nCellNumber = 0;
	for (nCell = 0; nCell < ivCellFrequencies.GetSize(); nCell++)
	{
		if (ivCellFrequencies.GetAt(nCell) > 0)
			nCellNumber++;
	}
	return nCellNumber;
}

int KWDataGridStats::ComputeCellNumber() const
{
	if (GetSourceAttributeNumber() > 0)
		return ComputeSourceCellNumber();
	else
		return ComputeTotalCellNumber();
}

int KWDataGridStats::ComputeSourceGridSize() const
{
	int nSourceGridSize;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(CheckAttributes());

	// Calcul de la taille de la grille
	nSourceGridSize = 1;
	for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		assert(nSourceGridSize * 1.0 * attribute->GetPartNumber() < INT_MAX);
		nSourceGridSize *= attribute->GetPartNumber();
	}
	return nSourceGridSize;
}

int KWDataGridStats::ComputeTargetGridSize() const
{
	int nTargetGridSize;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(CheckAttributes());

	// Calcul de la taille de la grille
	nTargetGridSize = 1;
	for (nAttribute = GetFirstTargetAttributeIndex(); nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		assert(nTargetGridSize * 1.0 * attribute->GetPartNumber() < INT_MAX);
		nTargetGridSize *= attribute->GetPartNumber();
	}
	return nTargetGridSize;
}

int KWDataGridStats::ComputeTotalGridSize() const
{
	int nGridSize;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(CheckAttributes());

	// Calcul de la taille de la grille
	nGridSize = 1;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		assert(nGridSize * 1.0 * attribute->GetPartNumber() < INT_MAX);
		nGridSize *= attribute->GetPartNumber();
	}
	return nGridSize;
}

int KWDataGridStats::ComputeGridSize() const
{
	if (GetSourceAttributeNumber() > 0)
		return ComputeSourceGridSize();
	else
		return ComputeTotalGridSize();
}

int KWDataGridStats::ComputeSourceInformativeAttributeNumber() const
{
	int nInformativeAttributeNumber;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(CheckAttributes());

	// Calcul du nombre d'attributs sources informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		if (attribute->GetPartNumber() > 1)
			nInformativeAttributeNumber++;
	}
	return nInformativeAttributeNumber;
}

int KWDataGridStats::ComputeTotalInformativeAttributeNumber() const
{
	int nInformativeAttributeNumber;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(CheckAttributes());

	// Calcul du nombre d'attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		if (attribute->GetPartNumber() > 1)
			nInformativeAttributeNumber++;
	}
	return nInformativeAttributeNumber;
}

int KWDataGridStats::ComputeInformativeAttributeNumber() const
{
	if (GetSourceAttributeNumber() > 0)
		return ComputeSourceInformativeAttributeNumber();
	else
		return ComputeTotalInformativeAttributeNumber();
}

void KWDataGridStats::SetMainTargetModalityIndex(int nValue)
{
	require(nMainTargetModalityIndex >= -1);
	nMainTargetModalityIndex = nValue;
}

int KWDataGridStats::GetMainTargetModalityIndex() const
{
	return nMainTargetModalityIndex;
}

void KWDataGridStats::Write(ostream& ost) const
{
	require(Check());
	WritePartial(ost, true, true);
}

void KWDataGridStats::WriteDefault(ostream& ost) const
{
	require(Check());
	if (GetTargetAttributeNumber() == GetAttributeNumber())
		WritePartial(ost, true, true);
	else
		WritePartial(ost, true, false);
}

void KWDataGridStats::WritePartial(ostream& ost, boolean bSource, boolean bTarget) const
{
	boolean bWritten;

	require(Check());

	// Description des attributs cibles
	if (bTarget)
	{
		// Nom des variables
		bWritten = WriteAttributeArrayLineReports(ost, false, true);
		if (bWritten)
			ost << "\n";

		// Parties de variables
		bWritten = WriteAttributePartArrayLineReports(ost, false, true);
		if (bWritten)
			ost << "\n";
	}

	// Description des attributs source
	if (bSource)
	{
		// Nom des variables
		bWritten = WriteAttributeArrayLineReports(ost, true, false);
		if (bWritten)
			ost << "\n";

		// Parties de variables
		bWritten = WriteAttributePartArrayLineReports(ost, true, false);
		if (bWritten)
			ost << "\n";

		// Affichage des effectifs par cellule dans un tableau croise dans les cas a deux attributs
		if (GetSourceAttributeNumber() == 2 or
		    (GetSourceAttributeNumber() == 0 and GetTargetAttributeNumber() == 2))
		{
			WriteFrequencyCrossTable(ost);
			ost << "\n";
		}

		// Affichage des statistiques pour la classe cible principale dans un tableau croise
		// dans le cas a deux attributs sources et un attribut cible
		if (GetSourceAttributeNumber() == 2 and GetTargetAttributeNumber() == 1 and
		    GetMainTargetModalityIndex() >= 0)
		{
			WriteTargetStatsCrossTableAt(ost, GetMainTargetModalityIndex());
			ost << "\n";
		}

		// Detail des cellules, sauf si non supervise et un seul attribut cible (detail deja affiche avec les
		// parties)
		if (ComputeCellNumber() > 0)
		{
			if (GetSourceAttributeNumber() > 0 or GetTargetAttributeNumber() > 1)
				WriteCellArrayLineReport(ost);
		}
	}
}

boolean KWDataGridStats::WriteAttributeArrayLineReports(ostream& ost, boolean bSource, boolean bTarget) const
{
	boolean bWritten = false;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(Check());

	// Liste des attributs cibles
	if (bTarget and GetTargetAttributeNumber() >= 1)
	{
		// Affichage du titre (avec la mention Target seulement s'il y a au moins une variable source, ou si
		// force)
		if (GetSourceAttributeNumber() > 0 or bSource)
			ost << "Target variable";
		else
			ost << "Variable";
		if (GetTargetAttributeNumber() > 1)
			ost << "s";

		// Cas particulier d'un seul attribut cible
		if (GetTargetAttributeNumber() == 1)
		{
			bWritten = true;
			attribute = GetAttributeAt(GetFirstTargetAttributeIndex());
			ost << "\t";
			ost << KWType::ToString(attribute->GetAttributeType()) << "\t"
			    << TSV::Export(attribute->GetAttributeName()) << "\n";

			// Ajout de l'information sur la valeur cible principale
			if (GetMainTargetModalityIndex() >= 0 and bSource)
			{
				ost << "Main target value\t";
				WriteMainTargetModality(ost);
				ost << "\n";
			}
		}

		// Cas a plusieurs attributs cibles
		if (GetTargetAttributeNumber() > 1)
		{
			bWritten = true;
			ost << "\n";
			ost << "\tType\tName\n";
			for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
			{
				attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);
				ost << "\t" << KWType::ToString(attribute->GetAttributeType()) << "\t"
				    << TSV::Export(attribute->GetAttributeName()) << "\n";
			}
		}
	}

	// Liste des attributs source
	if (bSource and GetSourceAttributeNumber() >= 1)
	{
		// Affichage du titre
		ost << "Variable";
		if (GetSourceAttributeNumber() > 1)
			ost << "s";

		// Cas particulier d'un seul attribut source
		if (GetSourceAttributeNumber() == 1)
		{
			bWritten = true;
			ost << "\t";
			ost << KWType::ToString(GetAttributeAt(0)->GetAttributeType()) << "\t"
			    << TSV::Export(GetAttributeAt(0)->GetAttributeName()) << "\n";

			if (KWFrequencyTable::GetWriteGranularityAndGarbage())
			{
				ost << "BestGranularity \t MaxGranularity \n";
				ost << GetGranularity() << "\t"
				    << (int)ceil(log(ComputeGridFrequency() * 1.0) / log(2.0)) << "\n";

				// Affichage groupe poubelle pour attribut categoriel
				if (GetAttributeAt(0)->GetAttributeType() == KWType::Symbol)
					ost << "GarbagePresence \t"
					    << (cast(KWDGSAttributeGrouping*, GetAttributeAt(0))
						    ->GetGarbageGroupIndex() > -1);
			}
		}

		// Cas a plusieurs attributs sources
		if (GetSourceAttributeNumber() > 1)
		{
			bWritten = true;
			ost << "\n";
			if (not KWFrequencyTable::GetWriteGranularityAndGarbage())
				ost << "\tType\tName\n";
			else
				ost << "\tType\tName\tGranularity\tGarbagePresence\n";
			for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
			{
				attribute = GetAttributeAt(nAttribute);
				ost << "\t" << KWType::ToString(attribute->GetAttributeType()) << "\t"
				    << TSV::Export(attribute->GetAttributeName());
				if (KWFrequencyTable::GetWriteGranularityAndGarbage())
				{
					ost << "\t" << GetGranularity();
					if (attribute->GetAttributeType() == KWType::Symbol)
						ost << "\t"
						    << (cast(KWDGSAttributeGrouping*, attribute)
							    ->GetGarbageGroupIndex() > -1)
						    << "\n";
					else
						ost << "\t" << 0 << "\n";
				}
				ost << "\n";
			}
		}
	}
	return bWritten;
}

boolean KWDataGridStats::WriteAttributePartArrayLineReports(ostream& ost, boolean bSource, boolean bTarget) const
{
	boolean bWritten = false;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(Check());

	// Liste des parties des attributs cibles
	if (bTarget and GetTargetAttributeNumber() >= 1)
	{
		bWritten = true;

		// Affichage du titre (avec la mention Target seulement s'il y a au moins une variable source, ou si
		// force)
		if (GetSourceAttributeNumber() > 0 or bSource)
			ost << "Target variable";
		else
			ost << "Variable";
		if (GetTargetAttributeNumber() > 1)
			ost << "s";
		ost << " stats\n";

		// Boucle sur tous les attributs cibles
		for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);

			// On rappele le nom de l'attribut uniquement s'il y en a plusieurs
			if (GetTargetAttributeNumber() > 1)
				ost << TSV::Export(attribute->GetAttributeName()) << "\n";

			// Ecriture des details des parties
			WriteAttributePartArrayLineReportAt(ost, GetFirstTargetAttributeIndex() + nAttribute, true);
		}
	}

	// Liste des attributs source
	// sauf si un seul attribut continu source
	if (bSource and GetSourceAttributeNumber() >= 1 and
	    not(GetSourceAttributeNumber() == 1 and GetAttributeAt(0)->GetAttributeType() == KWType::Continuous))
	{
		bWritten = true;

		// Affichage du titre
		if (GetSourceAttributeNumber() == 1)
			ost << "Variable stats"
			    << "\n";
		else
			ost << "Variables stats"
			    << "\n";

		// Boucle sur les attributs sources
		for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);

			// On rappele le nom de l'attribut uniquement s'il y en a plusieurs
			if (GetSourceAttributeNumber() > 1)
				ost << TSV::Export(attribute->GetAttributeName()) << "\n";

			// Ecriture des details des parties
			WriteAttributePartArrayLineReportAt(ost, nAttribute, (GetSourceAttributeNumber() > 1));
		}
	}
	return bWritten;
}

void KWDataGridStats::WriteAttributePartArrayLineReportAt(ostream& ost, int nAttribute, boolean bWriteFrequencies) const
{
	const KWDGSAttributePartition* attribute;
	IntVector ivPartFrequencies;
	int nTotalFrequency;
	int nPart;
	int nPartFrequency;

	require(Check());
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());

	// Acces a l'attribut
	attribute = GetAttributeAt(nAttribute);

	// Acces aux effectifs par parties
	ExportAttributePartFrequenciesAt(nAttribute, &ivPartFrequencies);

	// Calcul de l'effectif total
	nTotalFrequency = 0;
	for (nPart = 0; nPart < ivPartFrequencies.GetSize(); nPart++)
		nTotalFrequency += ivPartFrequencies.GetAt(nPart);
	assert(nTotalFrequency == ComputeGridFrequency());

	// Ligne d'entete
	attribute->WritePartHeader(ost);
	if (bWriteFrequencies)
		ost << "\tFrequency\tCoverage";
	if (attribute->IsPartDetailsReported())
	{
		ost << "\t";
		attribute->WritePartDetailsHeader(ost);
	}
	ost << "\n";

	// Details par partie
	for (nPart = 0; nPart < attribute->GetPartNumber(); nPart++)
	{
		// Nom de la partie
		attribute->WritePartAt(ost, nPart);

		// Statistiques
		if (bWriteFrequencies)
		{
			nPartFrequency = ivPartFrequencies.GetAt(nPart);
			assert(0 <= nPartFrequency and nPartFrequency <= nTotalFrequency);
			ost << "\t" << nPartFrequency;
			if (nTotalFrequency == 0)
				ost << "\t0";
			else
				ost << "\t" << nPartFrequency * 1.0 / nTotalFrequency;
		}

		// Details
		if (attribute->IsPartDetailsReported())
		{
			ost << "\t";
			attribute->WritePartDetailsAt(ost, nPart);
		}
		ost << "\n";
	}
}

void KWDataGridStats::WriteFrequencyCrossTable(ostream& ost) const
{
	const KWDGSAttributePartition* attribute1;
	const KWDGSAttributePartition* attribute2;
	ObjectArray oaNonEmptyCells;
	ObjectArray oaAllCells;
	int nCell;
	KWDGSCell* cell;
	int nPart1;
	int nPart2;
	IntVector ivAttribute1PartFrequencies;
	IntVector ivAttribute2PartFrequencies;
	int nGridFrequency;

	require(GetSourceAttributeNumber() == 2 or
		(GetSourceAttributeNumber() == 0 and GetTargetAttributeNumber() == 2));

	// Acces aux attributs a traiter
	attribute1 = GetAttributeAt(0);
	attribute2 = GetAttributeAt(1);

	// Collecte des cellules dans le cas supervise (on calcul leur effectif cumule)
	if (GetSourceAttributeNumber() == 2)
		ExportSourceCellsAt(&oaNonEmptyCells, GetFirstTargetAttributeIndex());
	// Collecte des cellules dans le cas non supervise
	else
		ExportAllCells(&oaNonEmptyCells);

	// Rangement dans un tableau de toutes les cellules
	oaAllCells.SetSize(attribute1->GetPartNumber() * attribute2->GetPartNumber());
	assert(oaNonEmptyCells.GetSize() <= oaAllCells.GetSize());
	for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
	{
		cell = cast(KWDGSCell*, oaNonEmptyCells.GetAt(nCell));
		assert(cell->GetAttributeNumber() == 2);

		// Rercherche des index des parties de la cellules
		nPart1 = cell->GetPartIndexes()->GetAt(0);
		nPart2 = cell->GetPartIndexes()->GetAt(1);
		assert(0 <= nPart1 and nPart1 < attribute1->GetPartNumber());
		assert(0 <= nPart2 and nPart2 < attribute2->GetPartNumber());

		// Rangement
		oaAllCells.SetAt(nPart1 + attribute1->GetPartNumber() * nPart2, cell);
	}

	// Calcul des effectifs par partie pour chaque attribut de la grille
	ExportAttributePartFrequenciesAt(0, &ivAttribute1PartFrequencies);
	ExportAttributePartFrequenciesAt(1, &ivAttribute2PartFrequencies);

	// Calcul de l'effectif total
	nGridFrequency = 0;
	for (nPart1 = 0; nPart1 < ivAttribute1PartFrequencies.GetSize(); nPart1++)
		nGridFrequency += ivAttribute1PartFrequencies.GetAt(nPart1);
	assert(nGridFrequency == ComputeGridFrequency());

	// Entete
	ost << "Cell frequencies\n";
	ost << "\t" << TSV::Export(attribute2->GetAttributeName()) << "\n";

	// Libelles des parties du second attribut
	ost << TSV::Export(attribute1->GetAttributeName());
	for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
	{
		ost << "\t";
		attribute2->WritePartAt(ost, nPart2);
	}
	ost << "\tTotal\tCoverage\n";

	// Affichage des effectifs du tableau croise
	for (nPart1 = 0; nPart1 < attribute1->GetPartNumber(); nPart1++)
	{
		// Libelle de la partie
		attribute1->WritePartAt(ost, nPart1);

		// Effectif par cellule
		for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
		{
			cell = cast(KWDGSCell*, oaAllCells.GetAt(nPart1 + attribute1->GetPartNumber() * nPart2));

			// Effectif nul si pas de cellule
			if (cell == NULL)
				ost << "\t" << 0;
			else
				ost << "\t" << cell->GetCellFrequency();
		}

		// Totaux par partie de l'attribut 1
		ost << "\t" << ivAttribute1PartFrequencies.GetAt(nPart1);

		// Couverture par partie de l'attribut 1
		if (nGridFrequency == 0)
			ost << "\t" << 0;
		else
			ost << "\t" << ivAttribute1PartFrequencies.GetAt(nPart1) * 1.0 / nGridFrequency;
		ost << "\n";
	}

	// Totaux par partie de l'attribut 2
	ost << "Total";
	nGridFrequency = 0;
	for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
	{
		nGridFrequency += ivAttribute2PartFrequencies.GetAt(nPart2);
		ost << "\t" << ivAttribute2PartFrequencies.GetAt(nPart2);
	}
	ost << "\t" << nGridFrequency << "\n";

	// Totaux par partie de l'attribut 2
	ost << "Coverage";
	for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
	{
		if (nGridFrequency == 0)
			ost << "\t" << 0;
		else
			ost << "\t" << ivAttribute2PartFrequencies.GetAt(nPart2) * 1.0 / nGridFrequency;
	}
	ost << "\n";

	// Nettoyage
	oaNonEmptyCells.DeleteAll();
}

void KWDataGridStats::WriteTargetStatsCrossTableAt(ostream& ost, int nTargetIndex) const
{
	const KWDGSAttributePartition* attribute1;
	const KWDGSAttributePartition* attribute2;
	ObjectArray oaNonEmptyCells;
	ObjectArray oaAllCells;
	int nCell;
	KWDGSSourceCell* cell;
	int nPart1;
	int nPart2;

	require(GetSourceAttributeNumber() == 2 and GetTargetAttributeNumber() == 1 and
		GetMainTargetModalityIndex() >= 0);

	// Acces aux attributs a traiter
	attribute1 = GetAttributeAt(0);
	attribute2 = GetAttributeAt(1);

	// Collecte des cellules dans le cas supervise (on calcul leur effectif cumule)
	ExportSourceCellsAt(&oaNonEmptyCells, GetFirstTargetAttributeIndex());

	// Rangement dans un tableau de toutes les cellules
	oaAllCells.SetSize(attribute1->GetPartNumber() * attribute2->GetPartNumber());
	assert(oaNonEmptyCells.GetSize() <= oaAllCells.GetSize());
	for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
	{
		cell = cast(KWDGSSourceCell*, oaNonEmptyCells.GetAt(nCell));
		assert(cell->GetAttributeNumber() == 2);

		// Rercherche des index des parties de la cellules
		nPart1 = cell->GetPartIndexes()->GetAt(0);
		nPart2 = cell->GetPartIndexes()->GetAt(1);
		assert(0 <= nPart1 and nPart1 < attribute1->GetPartNumber());
		assert(0 <= nPart2 and nPart2 < attribute2->GetPartNumber());

		// Rangement
		oaAllCells.SetAt(nPart1 + attribute1->GetPartNumber() * nPart2, cell);
	}

	// Entete
	ost << "% target value\t";
	WriteMainTargetModality(ost);
	ost << "\n";
	ost << "\t" << TSV::Export(attribute2->GetAttributeName()) << "\n";

	// Libelles des parties du second attribut
	ost << TSV::Export(attribute1->GetAttributeName());
	for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
	{
		ost << "\t";
		attribute2->WritePartAt(ost, nPart2);
	}
	ost << "\n";

	// Affichage des effectifs du tableau croise
	for (nPart1 = 0; nPart1 < attribute1->GetPartNumber(); nPart1++)
	{
		// Libelle de la partie
		attribute1->WritePartAt(ost, nPart1);

		// Proportion dans la cellule de la valeur cible principale
		for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
		{
			cell = cast(KWDGSSourceCell*, oaAllCells.GetAt(nPart1 + attribute1->GetPartNumber() * nPart2));

			// Proportion nulle si pas de cellule
			if (cell == NULL)
				ost << "\t" << 0;
			else
				ost << "\t"
				    << cell->GetTargetPartFrequencies()->GetAt(GetMainTargetModalityIndex()) * 1.0 /
					   cell->GetCellFrequency();
		}
		ost << "\n";
	}

	// Nettoyage
	oaNonEmptyCells.DeleteAll();
}

void KWDataGridStats::WriteCellArrayLineReport(ostream& ost) const
{
	require(Check());

	if (GetTargetAttributeNumber() == GetAttributeNumber())
		WriteUnsupervisedCellArrayLineReport(ost);
	else
		WriteSupervisedCellArrayLineReport(ost);
}

boolean KWDataGridStats::CheckMainTargetModalityIndex() const
{
	boolean bOk = true;

	// Verification de l'index de la valeur cible principale
	if (nMainTargetModalityIndex != -1)
	{
		assert(nMainTargetModalityIndex >= 0);

		// Il doit y a voir un unique attribut cible
		if (GetTargetAttributeNumber() != 1)
		{
			bOk = false;
			AddError("The index of the main target value should not be specified in case of several target "
				 "variables");
		}
		// Cet attribut cible doit etre de type Symbol
		else if (GetAttributeAt(GetFirstTargetAttributeIndex())->GetAttributeType() != KWType::Symbol)
		{
			bOk = false;
			AddError("The target variable " +
				 GetAttributeAt(GetFirstTargetAttributeIndex())->GetAttributeName() +
				 " should be categorical");
		}
		// L'index doit etre valide
		else if (nMainTargetModalityIndex >= GetAttributeAt(GetFirstTargetAttributeIndex())->GetPartNumber())
		{
			bOk = false;
			AddError("The index of the main target value is beyond the number of target values");
		}
	}
	return bOk;
}

void KWDataGridStats::WriteMainTargetModality(ostream& ost) const
{
	require(CheckMainTargetModalityIndex());
	GetAttributeAt(GetFirstTargetAttributeIndex())->WritePartAt(ost, GetMainTargetModalityIndex());
}

void KWDataGridStats::SetJSONAttributeDomainLowerBounds(const ContinuousVector* cvValues) const
{
	cvJSONAttributeDomainLowerBounds = cvValues;
}

const ContinuousVector* KWDataGridStats::GetJSONAttributeDomainLowerBounds() const
{
	return cvJSONAttributeDomainLowerBounds;
}

void KWDataGridStats::SetJSONAttributeDomainUpperBounds(const ContinuousVector* cvValues) const
{
	cvJSONAttributeDomainUpperBounds = cvValues;
}

const ContinuousVector* KWDataGridStats::GetJSONAttributeDomainUpperBounds() const
{
	return cvJSONAttributeDomainUpperBounds;
}

void KWDataGridStats::WriteJSONFields(JSONFile* fJSON) const
{
	int nAttribute;
	const KWDGSAttributeDiscretization attributeDiscretization;
	KWDGSAttributePartition* attribute;
	KWDGSAttributePartition* attribute1;
	KWDGSAttributePartition* attribute2;
	IntVector ivPartIndexes;
	boolean bShowCellInterest;
	ObjectArray oaNonEmptyCells;
	KWDGSCell* cell;
	KWDGSSourceCell* sourceCell;
	int nPart1;
	int nPart2;
	int nFrequency;
	int nCell;
	int nCellId;
	ALString sTmp;

	require(GetTargetAttributeNumber() == 1 or GetTargetAttributeNumber() == GetAttributeNumber());
	require(cvJSONAttributeDomainLowerBounds != NULL and cvJSONAttributeDomainUpperBounds != NULL);
	require(cvJSONAttributeDomainLowerBounds->GetSize() == cvJSONAttributeDomainUpperBounds->GetSize());
	require(cvJSONAttributeDomainLowerBounds->GetSize() == GetAttributeNumber());

	// On determine s'il faut afficher les interets des cellules
	bShowCellInterest = GetTargetAttributeNumber() == 1 and GetSourceAttributeNumber() >= 1;

	// Tag supervise
	fJSON->WriteKeyBoolean("isSupervised", bShowCellInterest);

	// Ecriture des attributs et de leur partition
	fJSON->BeginKeyArray("dimensions");
	for (nAttribute = 0; nAttribute < oaAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWDGSAttributePartition*, oaAttributes.GetAt(nAttribute));

		// Cas specifique de la discretisation
		if (attribute->GetClassLabel() == attributeDiscretization.GetClassLabel())
		{
			// On utilise les bornes pour ecrire les intervalles extremes avec leur vraies bornes
			fJSON->BeginObject();
			cast(KWDGSAttributeDiscretization*, attribute)
			    ->WriteJSONFieldsWithBounds(fJSON, cvJSONAttributeDomainLowerBounds->GetAt(nAttribute),
							cvJSONAttributeDomainUpperBounds->GetAt(nAttribute));
			fJSON->EndObject();
		}
		// Cas general
		else
			attribute->WriteJSONReport(fJSON);
	}
	fJSON->EndArray();

	// Effectifs dans le cas univarie
	if (GetAttributeNumber() == 1)
	{
		assert(not bShowCellInterest);
		attribute1 = cast(KWDGSAttributePartition*, oaAttributes.GetAt(0));
		fJSON->BeginKeyList("frequencies");
		for (nPart1 = 0; nPart1 < attribute1->GetPartNumber(); nPart1++)
		{
			nFrequency = GetUnivariateCellFrequencyAt(nPart1);
			fJSON->WriteInt(nFrequency);
		}
		fJSON->EndList();
	}
	// Effectifs dans le cas multivarie
	else
	{
		// Collecte des cellules dans le cas supervise
		if (GetTargetAttributeNumber() == 1)
			ExportSourceCellsAt(&oaNonEmptyCells, GetFirstTargetAttributeIndex());
		// Collecte des cellules dans le cas non supervise
		else
			ExportAllCells(&oaNonEmptyCells);

		// Cas supervise univarie
		if (GetSourceAttributeNumber() == 1 and GetTargetAttributeNumber() == 1)
		{
			assert(bShowCellInterest);

			// Acces aux deux attributs
			attribute1 = cast(KWDGSAttributePartition*, oaAttributes.GetAt(0));
			attribute2 = cast(KWDGSAttributePartition*, oaAttributes.GetAt(1));

			// Effectifs cible par partie source, y compris pour les parties sources vides
			fJSON->BeginKeyArray("partTargetFrequencies");
			for (nPart1 = 0; nPart1 < attribute1->GetPartNumber(); nPart1++)
			{
				fJSON->BeginList();
				for (nPart2 = 0; nPart2 < attribute2->GetPartNumber(); nPart2++)
				{
					nFrequency = GetBivariateCellFrequencyAt(nPart1, nPart2);
					fJSON->WriteInt(nFrequency);
				}
				fJSON->EndList();
			}
			fJSON->EndArray();

			// Interet par partie source, y compris pour les parties sources vides
			nPart1 = 0;
			fJSON->BeginKeyList("partInterests");
			for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
			{
				sourceCell = cast(KWDGSSourceCell*, oaNonEmptyCells.GetAt(nCell));

				// Ecriture d'un interet de 0 pour les parties vides avant la cellule courante
				while (nPart1 < sourceCell->GetPartIndexes()->GetAt(0))
				{
					fJSON->WriteDouble(0);
					nPart1++;
				}

				// Ecriture de l'interet de la cellule courante
				fJSON->WriteDouble(sourceCell->GetInterest());
				nPart1++;
			}
			// Ecriture d'un interet de 0 pour les dernieres parties vides
			while (nPart1 < attribute1->GetPartNumber())
			{
				fJSON->WriteDouble(0);
				nPart1++;
			}
			fJSON->EndList();
		}
		// Cas necessitant des cellules
		else
		{
			// Description des id des cellules
			fJSON->BeginKeyList("cellIds");
			for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
			{
				cell = cast(KWDGSCell*, oaNonEmptyCells.GetAt(nCell));

				// Identifiant de la cellule
				if (GetTargetAttributeNumber() == 1)
					nCellId = ComputeSourceCellIndex(cell->GetPartIndexes());
				else
					nCellId = ComputeCellIndex(cell->GetPartIndexes());
				fJSON->WriteString(sTmp + "C" + IntToString(nCellId + 1));
			}
			fJSON->EndList();

			// Description des index des cellules
			fJSON->BeginKeyArray("cellPartIndexes");
			for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
			{
				cell = cast(KWDGSCell*, oaNonEmptyCells.GetAt(nCell));

				// Index des parties
				fJSON->BeginList();
				for (nAttribute = 0; nAttribute < cell->GetPartIndexes()->GetSize(); nAttribute++)
					fJSON->WriteInt(cell->GetPartIndexes()->GetAt(nAttribute));
				fJSON->EndList();
			}
			fJSON->EndArray();

			// Cas supervise
			if (GetTargetAttributeNumber() == 1)
			{
				// Effectif des cellules
				fJSON->BeginKeyArray("cellTargetFrequencies");
				for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
				{
					sourceCell = cast(KWDGSSourceCell*, oaNonEmptyCells.GetAt(nCell));

					// Index des parties
					fJSON->BeginList();
					for (nPart2 = 0; nPart2 < sourceCell->GetTargetPartFrequencies()->GetSize();
					     nPart2++)
						fJSON->WriteInt(sourceCell->GetTargetPartFrequencies()->GetAt(nPart2));
					fJSON->EndList();
				}
				fJSON->EndArray();

				// Interet des cellules
				fJSON->BeginKeyList("cellInterests");
				for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
				{
					sourceCell = cast(KWDGSSourceCell*, oaNonEmptyCells.GetAt(nCell));
					fJSON->WriteDouble(sourceCell->GetInterest());
				}
				fJSON->EndList();
			}
			// Cas non supervise
			else
			{
				// Effectif des cellules
				fJSON->BeginKeyList("cellFrequencies");
				for (nCell = 0; nCell < oaNonEmptyCells.GetSize(); nCell++)
				{
					cell = cast(KWDGSCell*, oaNonEmptyCells.GetAt(nCell));
					fJSON->WriteInt(cell->GetCellFrequency());
				}
				fJSON->EndList();
			}
		}

		// Nettoyage
		oaNonEmptyCells.DeleteAll();
	}
}

void KWDataGridStats::WriteJSONReport(JSONFile* fJSON) const
{
	fJSON->BeginObject();
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWDataGridStats::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) const
{
	fJSON->BeginKeyObject(sKey);
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWDataGridStats::WriteJSONKeyValueFrequencies(JSONFile* fJSON, const ALString& sKey) const
{
	KWDGSAttributeSymbolValues* kwdgaSymbolValues;
	int nValue;

	require(GetAttributeNumber() == 1);
	require(GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(GetAttributeAt(0)->ArePartsSingletons());

	// Acces a la partition de l'attribut en valeurs
	kwdgaSymbolValues = cast(KWDGSAttributeSymbolValues*, GetAttributeAt(0));
	assert(kwdgaSymbolValues->GetValueNumber() == ComputeCellNumber());

	// Ecriture des valeurs
	fJSON->BeginKeyObject(sKey);
	fJSON->BeginKeyList("values");
	for (nValue = 0; nValue < kwdgaSymbolValues->GetValueNumber(); nValue++)
		fJSON->WriteString(kwdgaSymbolValues->GetValueAt(nValue).GetValue());
	fJSON->EndList();

	// Ecriture des effectifs
	fJSON->BeginKeyList("frequencies");
	for (nValue = 0; nValue < kwdgaSymbolValues->GetValueNumber(); nValue++)
		fJSON->WriteInt(GetUnivariateCellFrequencyAt(nValue));
	fJSON->EndList();
	fJSON->EndObject();
}

void KWDataGridStats::ExportAllCells(ObjectArray* oaCells) const
{
	int nCell;
	int nIndex;
	KWDGSCell* cell;

	require(Check());
	require(oaCells != NULL);
	require(oaCells->GetSize() == 0);

	// Initialisation de la taille du tableau en sortie
	oaCells->SetSize(ComputeTotalCellNumber());

	// Parcours de la grille pour creer les cellules non vides
	nCell = 0;
	for (nIndex = 0; nIndex < ivCellFrequencies.GetSize(); nIndex++)
	{
		// On ne cree que les cellules non vides
		if (ivCellFrequencies.GetAt(nIndex) > 0)
		{
			// Creation et ajout de la cellule
			cell = new KWDGSCell;
			oaCells->SetAt(nCell, cell);
			nCell++;

			// Initialisation de la cellules
			cell->GetPartIndexes()->SetSize(GetAttributeNumber());
			ComputePartIndexes(nIndex, cell->GetPartIndexes());
			cell->SetCellFrequency(ivCellFrequencies.GetAt(nIndex));
		}
	}
	assert(nCell == oaCells->GetSize());
}

void KWDataGridStats::ExportSourceCellsAt(ObjectArray* oaSourceCells, int nTargetAttribute) const
{
	int nCellFrequency;
	int nIndex;
	int nSourceIndex;
	KWDGSSourceCell* sourceCell;
	ObjectArray oaAllSourceCells;
	int nSourceGridSize;
	IntVector ivPartIndexes;
	int nAttribute;
	int nTargetPart;
	int nTargetPartNumber;
	double dGridFrequency;
	IntVector ivTargetFrequencies;
	double dTargetProb;
	double dGlobalTargetProb;
	double dInterest;
	double dTotalInterest;

	require(Check());
	require(oaSourceCells != NULL);
	require(oaSourceCells->GetSize() == 0);
	require(GetFirstTargetAttributeIndex() <= nTargetAttribute and nTargetAttribute < GetAttributeNumber());

	// Initialisations
	ivPartIndexes.SetSize(GetAttributeNumber());
	nTargetPartNumber = GetAttributeAt(nTargetAttribute)->GetPartNumber();

	// Creation d'un tableau pour accueillir les cellules sources
	nSourceGridSize = ComputeSourceGridSize();
	oaAllSourceCells.SetSize(nSourceGridSize);

	// Parcours de la grille pour identifier et alimenter les cellules sources non vides
	dGridFrequency = 0;
	for (nIndex = 0; nIndex < ivCellFrequencies.GetSize(); nIndex++)
	{
		nCellFrequency = ivCellFrequencies.GetAt(nIndex);
		dGridFrequency += nCellFrequency;

		// On ne traite que les cellules non vides
		if (nCellFrequency > 0)
		{
			// Calcul des index de parties des cellules
			ComputePartIndexes(nIndex, &ivPartIndexes);

			// Calcul de l'index source correspondant
			nSourceIndex = ComputeSourceCellIndex(&ivPartIndexes);
			assert(0 <= nSourceIndex and nSourceIndex < nSourceGridSize);

			// Recherche de la cellule source correspondante (creation si necessaire)
			sourceCell = cast(KWDGSSourceCell*, oaAllSourceCells.GetAt(nSourceIndex));
			if (sourceCell == NULL)
			{
				// Creation de la cellule source
				sourceCell = new KWDGSSourceCell;
				oaAllSourceCells.SetAt(nSourceIndex, sourceCell);

				// Initialisation de son vecteur d'index
				sourceCell->GetPartIndexes()->SetSize(GetSourceAttributeNumber());
				for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
					sourceCell->GetPartIndexes()->SetAt(nAttribute,
									    ivPartIndexes.GetAt(nAttribute));

				// Initialisation de son vecteur d'effectif par partie cible
				sourceCell->GetTargetPartFrequencies()->SetSize(nTargetPartNumber);
			}

			// Mise a jour des effectifs de la cellule
			sourceCell->SetCellFrequency(sourceCell->GetCellFrequency() + nCellFrequency);
			nTargetPart = ivPartIndexes.GetAt(nTargetAttribute);
			sourceCell->GetTargetPartFrequencies()->UpgradeAt(nTargetPart, nCellFrequency);
		}
	}

	// Parcours de toutes les cellules sources pour ne retenir que celles qui sont non vides
	for (nSourceIndex = 0; nSourceIndex < oaAllSourceCells.GetSize(); nSourceIndex++)
	{
		sourceCell = cast(KWDGSSourceCell*, oaAllSourceCells.GetAt(nSourceIndex));
		if (sourceCell != NULL)
		{
			assert(sourceCell->Check());
			oaSourceCells->Add(sourceCell);
		}
	}

	//////////////////////////////////////////////////////////////
	// Calcul d'un level par cellule

	// Export des effectif par partie cible
	ExportAttributePartFrequenciesAt(nTargetAttribute, &ivTargetFrequencies);

	// Premiere passe de calcul des indicateurs
	dTotalInterest = 0;
	for (nIndex = 0; nIndex < oaSourceCells->GetSize(); nIndex++)
	{
		sourceCell = cast(KWDGSSourceCell*, oaSourceCells->GetAt(nIndex));
		nCellFrequency = sourceCell->GetCellFrequency();

		// Calcul de la divergence de Kull-back Leibler entre la distribution local a la cellule
		// et la distribution globale, ponderee par l'effectif de la cellule
		dInterest = 0;
		for (nTargetPart = 0; nTargetPart < sourceCell->GetTargetPartFrequencies()->GetSize(); nTargetPart++)
		{
			if (sourceCell->GetTargetPartFrequencies()->GetAt(nTargetPart) > 0)
			{
				dTargetProb =
				    sourceCell->GetTargetPartFrequencies()->GetAt(nTargetPart) * 1.0 / nCellFrequency;
				dGlobalTargetProb = ivTargetFrequencies.GetAt(nTargetPart) / dGridFrequency;
				dInterest += dTargetProb * log(dTargetProb / dGlobalTargetProb);
			}
		}
		dInterest *= nCellFrequency / dGridFrequency;

		// Gestion des probleme potentiel de precision numerique
		assert(dInterest >= -1e-5);
		if (dInterest < 0)
			dInterest = 0;

		// Memorisation de l'indicateur
		sourceCell->SetInterest(dInterest);
		dTotalInterest += dInterest;
	}

	// Deuxieme passe de normalisation, pour avoir un poucentage
	for (nIndex = 0; nIndex < oaSourceCells->GetSize(); nIndex++)
	{
		sourceCell = cast(KWDGSSourceCell*, oaSourceCells->GetAt(nIndex));
		if (dTotalInterest > 0)
			sourceCell->SetInterest(sourceCell->GetInterest() / dTotalInterest);
	}
}

void KWDataGridStats::ExportAttributePartFrequenciesAt(int nAttribute, IntVector* ivPartFrequencies) const
{
	const KWDGSAttributePartition* attribute;
	int nCell;
	int nCellFrequency;
	IntVector ivPartIndexes;

	require(CheckCells());
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());
	require(ivPartFrequencies != NULL);

	// Initialisation de la taille du vecteur resultat
	attribute = GetAttributeAt(nAttribute);
	ivPartFrequencies->SetSize(attribute->GetPartNumber());

	// Parcours des cellules pour alimenter les effectifs par partie
	ivPartIndexes.SetSize(GetAttributeNumber());
	for (nCell = 0; nCell < ivCellFrequencies.GetSize(); nCell++)
	{
		nCellFrequency = ivCellFrequencies.GetAt(nCell);

		// Calcul des index de partie correspondant a la cellule
		ComputePartIndexes(nCell, &ivPartIndexes);

		// Mise a jour des effectif des parties de l'attribut
		ivPartFrequencies->UpgradeAt(ivPartIndexes.GetAt(nAttribute), nCellFrequency);
	}
}

void KWDataGridStats::ExportAttributeDataGridStatsAt(int nAttribute, KWDataGridStats* univariateDataGridStats) const
{
	require(CheckCells());
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());
	require(univariateDataGridStats != NULL);

	// Nettoyage initial
	univariateDataGridStats->DeleteAll();

	// Creation de l'attribut
	univariateDataGridStats->AddAttribute(GetAttributeAt(nAttribute)->Clone());

	// Calcul des effectifs des cellules
	ExportAttributePartFrequenciesAt(nAttribute, &(univariateDataGridStats->ivCellFrequencies));

	// Index de la valeur cible principale
	if (GetTargetAttributeNumber() == 1 and GetFirstTargetAttributeIndex() == nAttribute)
		univariateDataGridStats->SetMainTargetModalityIndex(GetMainTargetModalityIndex());
}

boolean KWDataGridStats::CheckPartFrequencies() const
{
	boolean bOk = true;
	int nAttribute;
	IntVector ivPartFrequencies;
	int nPart;
	ALString sTmp;

	require(CheckCells());

	// Verification par attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		ExportAttributePartFrequenciesAt(nAttribute, &ivPartFrequencies);

		// Verification par partie
		for (nPart = 0; nPart < ivPartFrequencies.GetSize(); nPart++)
		{
			if (ivPartFrequencies.GetAt(nPart) == 0)
			{
				AddError(sTmp + "Part " + IntToString(nPart + 1) + " of variable " +
					 GetAttributeAt(nAttribute)->GetAttributeName() + " is empty");
				bOk = false;
				break;
			}
		}
		if (not bOk)
			break;
	}
	return bOk;
}

boolean KWDataGridStats::Check() const
{
	boolean bOk = true;
	boolean bCheckPartFrequencies = false;

	// Verification de la structure
	bOk = bOk and CheckAttributes();
	bOk = bOk and CheckCells();
	if (bCheckPartFrequencies)
		bOk = bOk and CheckPartFrequencies();
	bOk = bOk and CheckMainTargetModalityIndex();
	return bOk;
}

longint KWDataGridStats::GetUsedMemory() const
{
	longint lUsedMemory;
	int nAttribute;

	lUsedMemory = sizeof(KWDataGridStats) - sizeof(Object);
	lUsedMemory += ivCellFrequencies.GetUsedMemory();
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		lUsedMemory += GetAttributeAt(nAttribute)->GetUsedMemory();
	return lUsedMemory;
}

KWDataGridStats* KWDataGridStats::Clone() const
{
	KWDataGridStats* kwdgsClone;

	kwdgsClone = new KWDataGridStats;
	kwdgsClone->CopyFrom(this);
	return kwdgsClone;
}

void KWDataGridStats::CopyFrom(const KWDataGridStats* kwdgsSource)
{
	int nAttribute;

	require(kwdgsSource != NULL);

	// Nettoyage initial
	DeleteAll();

	// Duplication des attributs
	for (nAttribute = 0; nAttribute < kwdgsSource->GetAttributeNumber(); nAttribute++)
		AddAttribute(kwdgsSource->GetAttributeAt(nAttribute)->Clone());
	SetSourceAttributeNumber(kwdgsSource->GetSourceAttributeNumber());

	// Recopie des effectifs des cellules
	ivCellFrequencies.CopyFrom(&(kwdgsSource->ivCellFrequencies));

	// Nombre d'attributs sources
	SetSourceAttributeNumber(kwdgsSource->GetSourceAttributeNumber());

	// Index de la valeur cible principale
	SetMainTargetModalityIndex(kwdgsSource->GetMainTargetModalityIndex());

	// Duplication granularite
	SetGranularity(kwdgsSource->GetGranularity());
}

int KWDataGridStats::Compare(const KWDataGridStats* kwdgsSource) const
{
	int nCompare;
	int nAttribute;
	int nCell;

	require(kwdgsSource != NULL);

	// Nombre d'attributs
	nCompare = GetAttributeNumber() - kwdgsSource->GetAttributeNumber();
	if (nCompare == 0)
		nCompare = GetSourceAttributeNumber() - kwdgsSource->GetSourceAttributeNumber();

	// Attributs
	if (nCompare == 0)
	{
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			nCompare = GetAttributeAt(nAttribute)->Compare(kwdgsSource->GetAttributeAt(nAttribute));
			if (nCompare != 0)
				break;
		}
	}

	// Cellules
	if (nCompare == 0)
		nCompare = ivCellFrequencies.GetSize() - kwdgsSource->ivCellFrequencies.GetSize();
	if (nCompare == 0)
	{
		for (nCell = 0; nCell < ivCellFrequencies.GetSize(); nCell++)
		{
			nCompare = ivCellFrequencies.GetAt(nCell) - kwdgsSource->ivCellFrequencies.GetAt(nCell);
			if (nCompare != 0)
				break;
		}
	}

	// Index de la valeur cible principale
	if (nCompare == 0)
		nCompare = GetMainTargetModalityIndex() - kwdgsSource->GetMainTargetModalityIndex();

	// Granularite
	if (nCompare == 0)
		nCompare = GetGranularity() - kwdgsSource->GetGranularity();

	return nCompare;
}

const ALString KWDataGridStats::ExportVariableNames() const
{
	const ALString sDataGridLabel = "DataGrid";
	int nAttribute;
	const KWDGSAttributePartition* attributePartition;
	ALString sDataGridVariableNames;

	// Creation d'un libelle sur les variables de la grille
	sDataGridVariableNames = sDataGridLabel + "(";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attributePartition = GetAttributeAt(nAttribute);
		if (nAttribute > 0)
			sDataGridVariableNames += ", ";
		sDataGridVariableNames += KWClass::GetExternalName(attributePartition->GetAttributeName());
	}
	sDataGridVariableNames += ")";

	return sDataGridVariableNames;
}

boolean KWDataGridStats::ImportVariableNames(const ALString& sValue)
{
	boolean bOk = true;
	StringVector svVariableNames;
	ALString sVariableName;
	ObjectDictionary odVariableNames;
	const ALString sDataGridLabel = "DataGrid";
	int nAttribute;
	KWDGSAttributePartition* attributePartition;
	int nLength;
	int nPosLabel;
	int nPos;
	int nVarStart;

	// Longueur de la chaine a parser
	nLength = sValue.GetLength();
	nPos = 0;
	nVarStart = 0;

	// On ignore les ' ' en debut
	if (bOk)
	{
		nPos = FindNextInformativeChar(sValue, 0);
		bOk = (nPos != -1);
	}

	// On recherche le mot cle de depart
	if (bOk)
	{
		nPosLabel = sValue.Find(sDataGridLabel);
		bOk = (nPosLabel == nPos);
		if (bOk)
			nPos += sDataGridLabel.GetLength();
	}

	// Recherche de la parenthese ouvrante
	if (bOk)
	{
		nPos = FindNextInformativeChar(sValue, nPos);
		bOk = (nPos != -1);
		if (bOk)
			bOk = (sValue.GetAt(nPos) == '(');
		if (bOk)
		{
			nPos++;
			nPos = FindNextInformativeChar(sValue, nPos);
			bOk = (nPos != -1);
		}
	}

	// Boucle de recherche des nom de variables
	if (bOk)
	{
		// Recherche des variables jusqu'a la parenthese fermante
		while (bOk and sValue.GetAt(nPos) != ')')
		{
			// Cas d'un nom de variable "externe" (avec back-quote)
			if (sValue.GetAt(nPos) == '`')
			{
				nPos++;
				bOk = (nPos < nLength);
				if (bOk)
				{
					nVarStart = nPos;
					while (nPos < nLength and sValue.GetAt(nPos) != '`')
						nPos++;
					bOk = (nPos < nLength and sValue.GetAt(nPos) == '`');
				}

				// Extraction de la variable
				if (bOk)
				{
					sVariableName = sValue.Mid(nVarStart, nPos - nVarStart);
					nPos++;
					bOk = (nPos < nLength);
				}
			}
			// Cas d'un nom de variable standard
			else
			{
				nVarStart = nPos;
				while (nPos < nLength and sValue.GetAt(nPos) != ',' and sValue.GetAt(nPos) != ')')
					nPos++;
				bOk = (nPos < nLength and (sValue.GetAt(nPos) == ',' or sValue.GetAt(nPos) == ')'));

				// Recherche de la variable
				if (bOk)
				{
					sVariableName = sValue.Mid(nVarStart, nPos - nVarStart);
					sVariableName.TrimLeft();
					sVariableName.TrimRight();
				}
			}

			// Memorisation de la variable
			if (bOk)
			{
				bOk = (sVariableName.GetLength() > 0);
				if (bOk)
					bOk = (sVariableName.GetLength() <= KWClass::GetNameMaxLength());
				if (bOk)
					bOk = (odVariableNames.Lookup(sVariableName) == NULL);
				if (bOk)
				{
					assert(KWClass::CheckName(sVariableName, KWClass::Attribute, NULL));
					svVariableNames.Add(sVariableName);
					odVariableNames.SetAt(sVariableName, &odVariableNames);
				}
			}

			// Extraction des caracteres jusqu'au prochain token a traiter
			if (bOk)
			{
				while (nPos < nLength and sValue.GetAt(nPos) != ',' and sValue.GetAt(nPos) != ')')
					nPos++;
				bOk = (nPos < nLength);
				if (bOk and sValue.GetAt(nPos) == ',')
				{
					nPos++;
					bOk = (nPos < nLength);
					if (bOk)
					{
						nPos = FindNextInformativeChar(sValue, nPos);
						bOk = (nPos != -1 and sValue.GetAt(nPos) != ')');
					}
				}
			}
		}
	}

	// Le nombre de variable doit etre compatible avec celui de la grile
	if (bOk)
		bOk = (svVariableNames.GetSize() == oaAttributes.GetSize());

	// Import des nom des variables de la grille
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < oaAttributes.GetSize(); nAttribute++)
		{
			attributePartition = cast(KWDGSAttributePartition*, oaAttributes.GetAt(nAttribute));

			// Remplacement du nom de la variable
			attributePartition->SetAttributeName(svVariableNames.GetAt(nAttribute));
		}
	}
	return bOk;
}

const ALString KWDataGridStats::GetClassLabel() const
{
	return "Data grid";
}

const ALString KWDataGridStats::GetObjectLabel() const
{
	ALString sTmp;

	// Libelle base les nombres d'attributs source et cible
	// On ne passe par les methodes d'acces standard, car les specifications peuvent
	// etre dans un etat invalide
	return sTmp + "(" + IntToString(GetSourceAttributeNumber()) + ", " +
	       IntToString(oaAttributes.GetSize() - GetSourceAttributeNumber()) + ")";
}

KWDataGridStats* KWDataGridStats::CreateTestDataGrid(int nSymbolAttributeNumber, int nContinuousAttributeNumber,
						     boolean bSingletonPartAttributes, int nSourceAttributeNumber,
						     int nAttributePartNumber, int nMaxCellInstanceNumber)
{
	KWDataGridStats* testDataGrid;
	const ALString sAttributePrefix = "Att";
	int nAttribute;
	KWDGSAttributePartition* attribute;
	IntVector ivPartIndexes;
	int nCellNumber;
	int nCell;
	int nCellFrequency;
	int nTotalFrequency;

	require(nSymbolAttributeNumber >= 0);
	require(nContinuousAttributeNumber >= 0);
	require(nSourceAttributeNumber >= 0);
	require(nAttributePartNumber >= 0);
	require(nMaxCellInstanceNumber >= 0);

	// Creation du DataGrid
	testDataGrid = new KWDataGridStats;

	// Creation des attributs
	for (nAttribute = 0; nAttribute < nSymbolAttributeNumber + nContinuousAttributeNumber; nAttribute++)
	{
		// Test si attribut symbolique
		if (nAttribute < nSymbolAttributeNumber)
		{
			// Test si partition en singletons
			if (bSingletonPartAttributes)
				attribute = KWDGSAttributeSymbolValues::CreateTestAttribute(nAttributePartNumber);
			else
				attribute = KWDGSAttributeGrouping::CreateTestAttribute(nAttributePartNumber);
		}
		// Sinon, attribut continu
		else
		{
			// Test si partition en singletons
			if (bSingletonPartAttributes)
				attribute = KWDGSAttributeContinuousValues::CreateTestAttribute(nAttributePartNumber);
			else
				attribute = KWDGSAttributeDiscretization::CreateTestAttribute(nAttributePartNumber);
		}

		// Nom l'attribut
		attribute->SetAttributeName(sAttributePrefix + IntToString(nAttribute + 1));

		// Ajout dans la grille
		testDataGrid->AddAttribute(attribute);
	}

	// Parametrage du nombre d'attributs sources
	testDataGrid->SetSourceAttributeNumber(nSourceAttributeNumber);
	assert(testDataGrid->CheckAttributes());

	// Creation des cellules
	testDataGrid->CreateAllCells();

	// Ajout d'instances dans le DataGrid
	nCellNumber = testDataGrid->ComputeTotalGridSize();
	nTotalFrequency = 0;
	ivPartIndexes.SetSize(testDataGrid->GetAttributeNumber());
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		testDataGrid->ComputePartIndexes(nCell, &ivPartIndexes);
		nCellFrequency = 1 + nCell % nMaxCellInstanceNumber;
		nTotalFrequency += nCellFrequency;
		testDataGrid->SetCellFrequencyAt(&ivPartIndexes, nCellFrequency);
	}
	assert(testDataGrid->Check());
	assert(nTotalFrequency <= testDataGrid->ComputeGridFrequency());

	// Specification si possible d'un index de modalite cible principale
	if (testDataGrid->GetTargetAttributeNumber() == 1 and
	    testDataGrid->GetAttributeAt(testDataGrid->GetFirstTargetAttributeIndex())->GetAttributeType() ==
		KWType::Symbol)
		testDataGrid->SetMainTargetModalityIndex(0);

	return testDataGrid;
}

void KWDataGridStats::Test()
{
	KWDataGridStats* testDataGrid;

	// Non supervise
	cout << "Unsupervised (1)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(0, 1, false, 0, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
	//
	cout << "Unsupervised (2)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(1, 1, false, 0, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
	//
	cout << "Unsupervised simple(3)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(2, 1, true, 0, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;

	// Supervise
	cout << "Supervised (1, 1)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(1, 1, false, 1, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
	//
	cout << "Supervised simple (2, 1)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(2, 1, true, 2, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
	//
	cout << "Supervised(1, 2)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(1, 2, false, 1, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
	//
	cout << "Supervised(2, 2)\n--------------------\n";
	testDataGrid = CreateTestDataGrid(2, 2, false, 2, 2, 3);
	cout << *testDataGrid << endl;
	delete testDataGrid;
}

////////////////////////////////////////

void KWDataGridStats::WriteSupervisedCellArrayLineReport(ostream& ost) const
{
	int nAttribute;
	int nPart;
	int nCell;
	boolean bWriteCellIndex;
	int nCellIndex;
	const KWDGSAttributePartition* attribute;
	ObjectArray oaCells;
	KWDGSCell* cell;
	int nCellNumber;
	int nCellFrequency;
	double dGridFrequency;
	ObjectArray oaAllSourceCellViews;
	ObjectArray* oaSourceCells;
	KWDGSSourceCell* sourceCell;
	IntVector ivPartFrequencies;
	boolean bShowCellInterest;

	require(GetSourceAttributeNumber() > 0);
	require(GetTargetAttributeNumber() < GetAttributeNumber());

	// On determine s'il faut afficher les interets des cellules
	bShowCellInterest = GetTargetAttributeNumber() == 1 and GetSourceAttributeNumber() >= 1;

	// Dans un premier temps, on collecte les cellules sources avec leur probabilites
	// conditionnelles, ceci pour chaque attribut cible
	oaAllSourceCellViews.SetSize(GetTargetAttributeNumber());
	nCellNumber = 0;
	for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
	{
		// Creation du tableau de cellule
		oaSourceCells = new ObjectArray;
		oaAllSourceCellViews.SetAt(nAttribute, oaSourceCells);

		// Initialisation et tri, pour l'attribut cible
		ExportSourceCellsAt(oaSourceCells, GetFirstTargetAttributeIndex() + nAttribute);
		nCellNumber = oaSourceCells->GetSize();

		// Tri des cellule par interet, uniquement dans le cas univarie cible (il ne peut y a avoir qu'un tri)
		if (GetTargetAttributeNumber() == 1)
		{
			// Tri dans le cas multi-varie source, ou univarie source si categoriel
			if (GetSourceAttributeNumber() > 1 or GetAttributeAt(0)->GetAttributeType() == KWType::Symbol)
				SortSourceCells(oaSourceCells, nAttribute);
		}
	}

	// On affiche les index de cellules uniquement dans le cas multivarie
	bWriteCellIndex = GetSourceAttributeNumber() > 1;

	// Titre dans le cas univarie
	if (GetSourceAttributeNumber() == 1)
		ost << "% target values"
		    << "\n";
	// Titre quand il y a au moins deux attributs source
	else
		ost << "Cells\t" << nCellNumber << "\n";

	/////////////////////////////////////////////////////
	// Affichage de l'entete

	// S'il y a plus de deux attributs cibles, on affiche leur nom dans une premiere ligne
	if (GetTargetAttributeNumber() > 1)
	{
		// On passe les attributs sources
		if (bWriteCellIndex)
			ost << "\t";
		for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
			ost << "\t";

		// Nom d'attribut par attribut cible
		for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);
			ost << TSV::Export(attribute->GetAttributeName());

			// On passe les parties
			for (nPart = 0; nPart < attribute->GetPartNumber(); nPart++)
				ost << "\t";
		}
		ost << "\n";
	}

	// Identifiant de la cellule
	if (bWriteCellIndex)
		ost << "Cell Id\t";

	// Un seul attribut source: on affiche uniquement le type de partie
	if (GetSourceAttributeNumber() == 1)
	{
		attribute = GetAttributeAt(0);
		attribute->WritePartHeader(ost);
		ost << "\t";
	}
	// Plusieurs attributs sources: on rappelle le nom de chaque attribut
	else
	{
		for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			ost << TSV::Export(attribute->GetAttributeName()) << "\t";
		}
	}

	// Parties des attributs cibles, pour affichage des probabilites conditionnelles par partie
	for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);

		// On affiche les libelles des parties
		for (nPart = 0; nPart < attribute->GetPartNumber(); nPart++)
		{
			attribute->WritePartAt(ost, nPart);
			ost << "\t";
		}
	}

	// Interest de la cellule
	if (bShowCellInterest)
		ost << "Interest\t";

	// Fin de l'entete: effectif de la cellule
	ost << "Frequency\tCoverage\n";

	///////////////////////////////////////////////////////////////////////////////////////
	// Affichage des cellules
	// Dans ce cas, on affiche la partie referencant la cellule pour chaque attribut source,
	// puis, par attribut cible, les proportions (probabilites conditionnelles) par partie

	// Effectif de la grille
	dGridFrequency = ComputeGridFrequency();

	// Affichage des cellules
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		// Recherche de la cellule dans le tableau de cellules du premier attribut cible
		oaSourceCells = cast(ObjectArray*, oaAllSourceCellViews.GetAt(0));
		cell = cast(KWDGSCell*, oaSourceCells->GetAt(nCell));

		// Identifiant source de la cellule
		nCellIndex = ComputeSourceCellIndex(cell->GetPartIndexes());
		if (bWriteCellIndex)
			ost << "C" << nCellIndex + 1 << "\t";

		// Affichage des libelles des parties sources de la cellule
		for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			attribute->WritePartAt(ost, cell->GetPartIndexes()->GetAt(nAttribute));
			ost << "\t";
		}

		// Effectif de la cellule
		nCellFrequency = cell->GetCellFrequency();
		assert(nCellFrequency > 0);

		// Affichage des probabilites conditionnelles par attribut cible
		for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);

			// Acces au tableau de cellules correspondant
			oaSourceCells = cast(ObjectArray*, oaAllSourceCellViews.GetAt(nAttribute));

			// Acces a la cellule correspondante et a ses statistiques par partie cible
			sourceCell = cast(KWDGSSourceCell*, oaSourceCells->GetAt(nCell));
			assert(cell->GetCellFrequency() == sourceCell->GetCellFrequency());
			assert(cell->GetPartIndexes()->GetAt(0) == sourceCell->GetPartIndexes()->GetAt(0));
			assert(cell->GetPartIndexes()->GetAt(GetSourceAttributeNumber() - 1) ==
			       sourceCell->GetPartIndexes()->GetAt(GetSourceAttributeNumber() - 1));

			// Affichage des probabilites conditionnelles
			assert(sourceCell->GetCellFrequency() > 0);
			for (nPart = 0; nPart < attribute->GetPartNumber(); nPart++)
				ost << sourceCell->GetTargetPartFrequencies()->GetAt(nPart) * 1.0 / nCellFrequency
				    << "\t";
		}

		// Calcul si necessaire de l'interet de la cellule
		if (bShowCellInterest)
		{
			// Acces a la cellule correspondante
			oaSourceCells = cast(ObjectArray*, oaAllSourceCellViews.GetAt(0));
			sourceCell = cast(KWDGSSourceCell*, oaSourceCells->GetAt(nCell));

			// Affichage de sa Interest
			ost << sourceCell->GetInterest() << "\t";
		}

		// Affichage de l'effectif et de la couverture de la cellule
		ost << nCellFrequency << "\t";
		if (dGridFrequency == 0)
			ost << "0\n";
		else
			ost << nCellFrequency / dGridFrequency << "\n";
	}

	// Affichage des totaux
	if (bWriteCellIndex)
		ost << "\t";
	ost << "Total";
	for (nAttribute = 0; nAttribute < GetSourceAttributeNumber(); nAttribute++)
		ost << "\t";
	for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(GetFirstTargetAttributeIndex() + nAttribute);

		// Probabilite conditionnelles par defaut par partie
		ExportAttributePartFrequenciesAt(GetFirstTargetAttributeIndex() + nAttribute, &ivPartFrequencies);
		for (nPart = 0; nPart < attribute->GetPartNumber(); nPart++)
		{
			if (dGridFrequency == 0)
				ost << "0\t";
			else
				ost << ivPartFrequencies.GetAt(nPart) / dGridFrequency << "\t";
		}
	}
	if (bShowCellInterest)
	{
		if (nCellNumber > 1)
			ost << "1\t";
		else
			ost << "0\t";
	}
	ost << (int)dGridFrequency << "\t" << 1 << "\n";

	// Nettoyage
	for (nAttribute = 0; nAttribute < GetTargetAttributeNumber(); nAttribute++)
	{
		oaSourceCells = cast(ObjectArray*, oaAllSourceCellViews.GetAt(nAttribute));
		oaSourceCells->DeleteAll();
	}
	oaAllSourceCellViews.DeleteAll();
}

void KWDataGridStats::SortSourceCells(ObjectArray* oaSourceCells, int nTargetAttributeIndex) const
{
	require(oaSourceCells != NULL);
	require(0 <= nTargetAttributeIndex and nTargetAttributeIndex < GetTargetAttributeNumber());
	oaSourceCells->SetCompareFunction(KWDGSSourceCellCompareDecreasingInterest);
	oaSourceCells->Sort();
}

int KWDataGridStats::FindNextInformativeChar(const ALString& sSearchedString, int nStartPos) const
{
	int nPos;

	require(0 <= nStartPos and nStartPos <= sSearchedString.GetLength());

	nPos = nStartPos;
	while (nPos < sSearchedString.GetLength())
	{
		if (sSearchedString.GetAt(nPos) != ' ')
			return nPos;
		nPos++;
	}
	return -1;
}

void KWDataGridStats::WriteUnsupervisedCellArrayLineReport(ostream& ost) const
{
	int nAttribute;
	int nCell;
	int nCellIndex;
	boolean bWriteCellIndex;
	const KWDGSAttributePartition* attribute;
	ObjectArray oaCells;
	KWDGSCell* cell;
	int nCellNumber;
	int nCellFrequency;
	double dGridFrequency;

	require(GetTargetAttributeNumber() == GetAttributeNumber());

	// On affiche les index de cellules uniquement dans le cas multivarie
	bWriteCellIndex = GetTargetAttributeNumber() > 1;

	// Titre
	ost << "Cells\t" << ComputeCellNumber() << "\n";

	// Identifiant de la cellule
	if (bWriteCellIndex)
		ost << "Cell Id\t";

	// Affichage de l'entete
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		ost << TSV::Export(attribute->GetAttributeName()) << "\t";
	}
	ost << "Frequency\tCoverage\n";

	// Effectif de la grille
	dGridFrequency = ComputeGridFrequency();

	// Tri des cellules par effectif decroissant en les rentrant prealablement dans un tableau
	ExportAllCells(&oaCells);
	oaCells.SetCompareFunction(KWDGSCellCompareDecreasingFrequency);
	oaCells.Sort();

	// Affichage des cellules
	nCellNumber = oaCells.GetSize();
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		cell = cast(KWDGSCell*, oaCells.GetAt(nCell));

		// Identifiant de la cellule
		nCellIndex = ComputeCellIndex(cell->GetPartIndexes());
		if (bWriteCellIndex)
			ost << "C" << nCellIndex + 1 << "\t";

		// Affichage des libelles des parties de la cellule
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			attribute->WritePartAt(ost, cell->GetPartIndexes()->GetAt(nAttribute));
			ost << "\t";
		}

		// Effectif de la cellule
		nCellFrequency = cell->GetCellFrequency();

		// Affichage de l'effectif de la cellule
		ost << nCellFrequency << "\t";
		if (dGridFrequency == 0)
			ost << "0\n";
		else
			ost << nCellFrequency / dGridFrequency << "\n";
	}

	// Affichage des totaux
	if (bWriteCellIndex)
		ost << "\t";
	ost << "Total";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		ost << "\t";
	ost << (int)dGridFrequency << "\t" << 1 << "\n";

	// Nettoyage
	oaCells.DeleteAll();
}

boolean KWDataGridStats::InternalCheckPartIndexes(const IntVector* ivPartIndexes, int nFirstAttributeIndex,
						  int nLastAttributeIndex) const
{
	int nPartIndex;
	int nAttribute;
	const KWDGSAttributePartition* attribute;

	require(ivPartIndexes != NULL);
	require(0 <= nFirstAttributeIndex and nFirstAttributeIndex <= GetAttributeNumber());
	require(nFirstAttributeIndex <= nLastAttributeIndex or nLastAttributeIndex == -1);
	require(nLastAttributeIndex < ivPartIndexes->GetSize() and ivPartIndexes->GetSize() <= GetAttributeNumber());

	// Verification des index de chaque partie
	for (nAttribute = nFirstAttributeIndex; nAttribute <= nLastAttributeIndex; nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		nPartIndex = ivPartIndexes->GetAt(nAttribute);
		assert(0 <= nPartIndex and nPartIndex < attribute->GetPartNumber());
	}
	return true;
}

int KWDataGridStatsCompare(const void* elem1, const void* elem2)
{
	KWDataGridStats* dataGridStats1;
	KWDataGridStats* dataGridStats2;
	int nAttribute;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la parties
	dataGridStats1 = cast(KWDataGridStats*, *(Object**)elem1);
	dataGridStats2 = cast(KWDataGridStats*, *(Object**)elem2);

	// Comparaison des noms des premiers attributs
	nDiff = 0;
	for (nAttribute = 0; nAttribute < dataGridStats1->GetAttributeNumber(); nAttribute++)
	{
		// Comparaison du nom de l'attribut courant, s'il y a assez d'attribut dans l'autre grille
		if (nAttribute < dataGridStats2->GetAttributeNumber())
			nDiff = dataGridStats1->GetAttributeAt(nAttribute)
				    ->GetAttributeName()
				    .Compare(dataGridStats2->GetAttributeAt(nAttribute)->GetAttributeName());
		else
			break;
		if (nDiff != 0)
			break;
	}

	// Si egalite, comparaison sur les nombres d'attributs
	if (nDiff == 0)
		nDiff = dataGridStats1->GetAttributeNumber() - dataGridStats2->GetAttributeNumber();
	return nDiff;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributePartition

KWDGSAttributePartition::KWDGSAttributePartition()
{
	nInitialValueNumber = 0;
	nGranularizedValueNumber = 0;
}

KWDGSAttributePartition::~KWDGSAttributePartition() {}

void KWDGSAttributePartition::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
}

const ALString& KWDGSAttributePartition::GetAttributeName() const
{
	return sAttributeName;
}

void KWDGSAttributePartition::SetInitialValueNumber(int nValue)
{
	require(nValue >= 0);
	nInitialValueNumber = nValue;
}
int KWDGSAttributePartition::GetInitialValueNumber() const
{
	return nInitialValueNumber;
}
void KWDGSAttributePartition::SetGranularizedValueNumber(int nValue)
{
	require(nValue >= 0);
	nGranularizedValueNumber = nValue;
}
int KWDGSAttributePartition::GetGranularizedValueNumber() const
{
	return nGranularizedValueNumber;
}

int KWDGSAttributePartition::ComputeContinuousPartIndex(Continuous cValue) const
{
	require(Check());
	require(GetAttributeType() == KWType::Continuous);
	assert(false);
	return 0;
}

int KWDGSAttributePartition::ComputeSymbolPartIndex(const Symbol& sValue) const
{
	require(Check());
	require(GetAttributeType() == KWType::Symbol);
	assert(false);
	return 0;
}

boolean KWDGSAttributePartition::Check() const
{
	boolean bOk = true;

	// Verification de la presencce du nom
	if (GetAttributeName() == "")
	{
		AddError("Missing variable name");
		bOk = false;
	}

	// Verification dans le cas d'au moins une partie
	assert(GetPartNumber() >= 0);
	// Verification affaiblie
	// Dans le cas d'un attribut continu, le nombre de parties est >=1
	// On peut avoir une seule partie en l'absence ou en presence d'instances
	// Le nombre d'instances est toujours > 0 a partir d'un nombre de parties >=2
	assert(GetPartNumber() <= 2 or GetInitialValueNumber() > 0);
	assert(GetPartNumber() <= 2 or GetGranularizedValueNumber() > 0);

	return bOk;
}

longint KWDGSAttributePartition::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDGSAttributePartition) - sizeof(Object);
	return lUsedMemory;
}

void KWDGSAttributePartition::Write(ostream& ost) const
{
	int nPart;

	// En tete de l'attribut
	ost << "Variable"
	    << "\t" << KWType::ToString(GetAttributeType()) << "\t" << TSV::Export(GetAttributeName()) << "\n";

	// Ligne d'entete
	ost << "\t";
	WritePartHeader(ost);
	if (IsPartDetailsReported())
	{
		ost << "\t";
		WritePartDetailsHeader(ost);
	}
	ost << "\n";

	// Parties de l'attribut
	for (nPart = 0; nPart < GetPartNumber(); nPart++)
	{
		ost << "\t";
		WritePartAt(ost, nPart);
		if (IsPartDetailsReported())
		{
			ost << "\t";
			WritePartDetailsAt(ost, nPart);
		}
		ost << "\n";
	}
}

void KWDGSAttributePartition::WritePartHeader(ostream& ost) const
{
	ost << "Part";
}

void KWDGSAttributePartition::WritePartAt(ostream& ost, int nPartIndex) const
{
	ost << 'P' << nPartIndex + 1;
}

boolean KWDGSAttributePartition::IsPartDetailsReported() const
{
	return false;
}

void KWDGSAttributePartition::WritePartDetailsHeader(ostream& ost) const {}

void KWDGSAttributePartition::WritePartDetailsAt(ostream& ost, int nPartIndex) const {}

void KWDGSAttributePartition::WriteJSONFields(JSONFile* fJSON) const
{
	int nPart;

	// Entete
	fJSON->WriteKeyString("variable", GetAttributeName());
	fJSON->WriteKeyString("type", KWType::ToString(GetAttributeType()));
	fJSON->WriteKeyString("partitionType", GetClassLabel());

	// Partition
	if (ArePartsSingletons())
	{
		fJSON->BeginKeyList("partition");
		for (nPart = 0; nPart < GetPartNumber(); nPart++)
			WriteJSONPartFieldsAt(fJSON, nPart);
		fJSON->EndList();
	}
	else
	{
		fJSON->BeginKeyArray("partition");
		for (nPart = 0; nPart < GetPartNumber(); nPart++)
			WriteJSONPartFieldsAt(fJSON, nPart);
		fJSON->EndArray();
	}
}

void KWDGSAttributePartition::WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) const {}

void KWDGSAttributePartition::WriteJSONReport(JSONFile* fJSON) const
{
	fJSON->BeginObject();
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWDGSAttributePartition::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) const
{
	fJSON->BeginKeyObject(sKey);
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

KWDGSAttributePartition* KWDGSAttributePartition::Clone() const
{
	KWDGSAttributePartition* kwdgsapClone;

	kwdgsapClone = Create();
	kwdgsapClone->CopyFrom(this);
	return kwdgsapClone;
}

void KWDGSAttributePartition::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	require(kwdgsapSource != NULL);

	SetAttributeName(kwdgsapSource->GetAttributeName());
	nInitialValueNumber = kwdgsapSource->nInitialValueNumber;
	nGranularizedValueNumber = kwdgsapSource->nGranularizedValueNumber;
}

int KWDGSAttributePartition::Compare(const KWDGSAttributePartition* kwdgsapSource) const
{
	int nCompare;

	require(kwdgsapSource != NULL);

	nCompare = GetAttributeName().Compare(kwdgsapSource->GetAttributeName());
	if (nCompare == 0)
		nCompare = GetAttributeType() - kwdgsapSource->GetAttributeType();
	if (nCompare == 0)
		nCompare = ArePartsSingletons() - kwdgsapSource->ArePartsSingletons();
	if (nCompare == 0)
		nCompare = GetPartNumber() - kwdgsapSource->GetPartNumber();
	if (nCompare == 0)
		nCompare = GetInitialValueNumber() - kwdgsapSource->GetInitialValueNumber();
	if (nCompare == 0)
		nCompare = GetGranularizedValueNumber() - kwdgsapSource->GetGranularizedValueNumber();
	return nCompare;
}

const ALString KWDGSAttributePartition::GetClassLabel() const
{
	return "Variable parts";
}

const ALString KWDGSAttributePartition::GetObjectLabel() const
{
	return GetAttributeName();
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeDiscretization

KWDGSAttributeDiscretization::KWDGSAttributeDiscretization() {}

KWDGSAttributeDiscretization::~KWDGSAttributeDiscretization() {}

int KWDGSAttributeDiscretization::GetAttributeType() const
{
	return KWType::Continuous;
}

boolean KWDGSAttributeDiscretization::ArePartsSingletons() const
{
	return false;
}

void KWDGSAttributeDiscretization::SetPartNumber(int nValue)
{
	require(nValue >= 1);
	cvIntervalBounds.SetSize(nValue - 1);
}

int KWDGSAttributeDiscretization::GetPartNumber() const
{
	return cvIntervalBounds.GetSize() + 1;
}

int KWDGSAttributeDiscretization::ComputeContinuousPartIndex(Continuous cValue) const
{
	int nIndex;

	require(Check());

	// Parcours des bornes des intervalles
	for (nIndex = 0; nIndex < cvIntervalBounds.GetSize(); nIndex++)
	{
		if (cValue <= cvIntervalBounds.GetAt(nIndex))
			return nIndex;
	}
	return cvIntervalBounds.GetSize();
}

boolean KWDGSAttributeDiscretization::Check() const
{
	boolean bOk;
	int nBound;
	ALString sTmp;

	// Verification standard
	bOk = KWDGSAttributePartition::Check();

	// Verification de l'unicite et de l'ordre des bornes de discretization
	if (bOk)
	{
		for (nBound = 1; nBound < cvIntervalBounds.GetSize(); nBound++)
		{
			assert(KWContinuous::GetMissingValue() < KWContinuous::GetMinValue());
			assert(KWContinuous::GetMissingValue() <= cvIntervalBounds.GetAt(nBound - 1));
			assert(cvIntervalBounds.GetAt(nBound - 1) < KWContinuous::GetMaxValue());

			// Tolerance si egalite des bornes, possible dans les limites de precision numerique
			if (not(cvIntervalBounds.GetAt(nBound - 1) <= cvIntervalBounds.GetAt(nBound)))
			{
				AddError(sTmp + "Interval bound " +
					 KWContinuous::ContinuousToString(cvIntervalBounds.GetAt(nBound - 1)) +
					 " is not inferior to the following bound");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

longint KWDGSAttributeDiscretization::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGSAttributePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDGSAttributeDiscretization) - sizeof(KWDGSAttributePartition);
	lUsedMemory += cvIntervalBounds.GetUsedMemory();
	return lUsedMemory;
}

void KWDGSAttributeDiscretization::WritePartHeader(ostream& ost) const
{
	ost << "Interval";
}

void KWDGSAttributeDiscretization::WritePartAt(ostream& ost, int nPartIndex) const
{
	boolean bMissingValue;

	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	// Flag de presence de la valeur manquante dans la discretisation
	bMissingValue =
	    cvIntervalBounds.GetSize() >= 1 and cvIntervalBounds.GetAt(0) == KWContinuous::GetMissingValue();

	// Cas particulier d'un intervalle reduit a la valeur manquante
	if (nPartIndex == 0 and bMissingValue)
		ost << "Missing";
	// Cas standard
	else
	{
		if (nPartIndex == 0 or (nPartIndex == 1 and bMissingValue))
			ost << "]-inf,";
		else
			ost << "]" << KWContinuous::ContinuousToString(cvIntervalBounds.GetAt(nPartIndex - 1)) << ",";
		if (nPartIndex == GetPartNumber() - 1)
			ost << "+inf[";
		else
			ost << KWContinuous::ContinuousToString(cvIntervalBounds.GetAt(nPartIndex)) << "]";
	}
}

KWDGSAttributePartition* KWDGSAttributeDiscretization::Create() const
{
	return new KWDGSAttributeDiscretization;
}

void KWDGSAttributeDiscretization::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	KWDGSAttributeDiscretization* sourcePartition;

	require(kwdgsapSource != NULL);

	// Copie de la methode ancetre
	KWDGSAttributePartition::CopyFrom(kwdgsapSource);

	// Copie specifique
	sourcePartition = cast(KWDGSAttributeDiscretization*, kwdgsapSource);
	cvIntervalBounds.CopyFrom(&(sourcePartition->cvIntervalBounds));
}

int KWDGSAttributeDiscretization::Compare(const KWDGSAttributePartition* kwdgsapSource) const
{
	int nCompare;
	int n;
	KWDGSAttributeDiscretization* sourcePartition;

	require(kwdgsapSource != NULL);

	// Appel de la methode ancetre
	nCompare = KWDGSAttributePartition::Compare(kwdgsapSource);

	// Comparaison specifique
	if (nCompare == 0)
	{
		sourcePartition = cast(KWDGSAttributeDiscretization*, kwdgsapSource);
		assert(cvIntervalBounds.GetSize() == sourcePartition->cvIntervalBounds.GetSize());
		for (n = 0; n < cvIntervalBounds.GetSize(); n++)
		{
			nCompare = KWContinuous::Compare(cvIntervalBounds.GetAt(n),
							 sourcePartition->cvIntervalBounds.GetAt(n));
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

const ALString KWDGSAttributeDiscretization::GetClassLabel() const
{
	return "Intervals";
}

void KWDGSAttributeDiscretization::WriteJSONFieldsWithBounds(JSONFile* fJSON, Continuous cMin, Continuous cMax) const
{
	require(cMin <= cMax);

	int nPart;

	// Entete
	fJSON->WriteKeyString("variable", GetAttributeName());
	fJSON->WriteKeyString("type", KWType::ToString(GetAttributeType()));
	fJSON->WriteKeyString("partitionType", GetClassLabel());

	// Partition
	fJSON->BeginKeyArray("partition");
	for (nPart = 0; nPart < GetPartNumber(); nPart++)
		WriteJSONPartFieldsAtWithBounds(fJSON, nPart, cMin, cMax);
	fJSON->EndArray();
}

void KWDGSAttributeDiscretization::WriteJSONPartFieldsAtWithBounds(JSONFile* fJSON, int nPartIndex, Continuous cMin,
								   Continuous cMax) const
{
	boolean bMissingValue;

	require(0 <= nPartIndex and nPartIndex < GetPartNumber());
	require(cMin <= cMax);

	// Flag de presence de la valeur manquante dans la discretisation
	bMissingValue =
	    cvIntervalBounds.GetSize() >= 1 and cvIntervalBounds.GetAt(0) == KWContinuous::GetMissingValue();

	// Cas particulier d'un intervalle reduit a la valeur manquante
	fJSON->BeginList();
	if (nPartIndex == 0 and bMissingValue)
	{
		// Tableau vide
	}
	// Cas standard
	else
	{
		// Borne inf
		if (nPartIndex == 0 or (nPartIndex == 1 and bMissingValue))
			fJSON->WriteContinuous(cMin);
		else
			fJSON->WriteContinuous(cvIntervalBounds.GetAt(nPartIndex - 1));

		// Borne suf
		if (nPartIndex == GetPartNumber() - 1)
			fJSON->WriteContinuous(cMax);
		else
			fJSON->WriteContinuous(cvIntervalBounds.GetAt(nPartIndex));
	}
	fJSON->EndList();
}

int KWDGSAttributeDiscretization::GetIntervalBoundNumber() const
{
	return cvIntervalBounds.GetSize();
}

void KWDGSAttributeDiscretization::SetIntervalBoundAt(int nIndex, Continuous cValue)
{
	require(0 <= nIndex and nIndex < cvIntervalBounds.GetSize());
	cvIntervalBounds.SetAt(nIndex, cValue);
}

Continuous KWDGSAttributeDiscretization::GetIntervalBoundAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < cvIntervalBounds.GetSize());
	return cvIntervalBounds.GetAt(nIndex);
}

KWDGSAttributeDiscretization* KWDGSAttributeDiscretization::CreateTestAttribute(int nPartNumber)
{
	KWDGSAttributeDiscretization* attribute;
	int nBound;

	require(nPartNumber >= 0);

	// Creation de l'attribut
	attribute = new KWDGSAttributeDiscretization;
	attribute->SetAttributeName("Att");

	// Creation des bornes des intervalles
	attribute->SetInitialValueNumber(nPartNumber);
	attribute->SetGranularizedValueNumber(nPartNumber);
	attribute->SetPartNumber(nPartNumber);
	for (nBound = 0; nBound < attribute->GetIntervalBoundNumber(); nBound++)
		attribute->SetIntervalBoundAt(nBound, (Continuous)(1.0 + nBound));
	ensure(attribute->Check());
	return attribute;
}

void KWDGSAttributeDiscretization::Test()
{
	KWDGSAttributePartition* attribute;

	attribute = CreateTestAttribute(4);
	attribute->Write(cout);
	cout << endl;
	delete attribute;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeGrouping

KWDGSAttributeGrouping::KWDGSAttributeGrouping()
{
	nGarbageGroupIndex = -1;
	nGarbageModalityNumber = 0;
	nCatchAllValueNumber = 0;
}

KWDGSAttributeGrouping::~KWDGSAttributeGrouping() {}

void KWDGSAttributeGrouping::SetGarbageGroupIndex(int nValue)
{
	nGarbageGroupIndex = nValue;
}

int KWDGSAttributeGrouping::GetGarbageGroupIndex() const
{
	return nGarbageGroupIndex;
}

void KWDGSAttributeGrouping::SetGarbageModalityNumber(int nValue)
{
	require(nValue >= 0);
	nGarbageModalityNumber = nValue;
}

int KWDGSAttributeGrouping::GetGarbageModalityNumber() const
{
	require(nGarbageGroupIndex >= -1);

	if (nGarbageGroupIndex == -1)
		return 0;
	else
		return nGarbageModalityNumber;
}

void KWDGSAttributeGrouping::SetCatchAllValueNumber(int nValue)
{
	require(nValue >= 0);
	nCatchAllValueNumber = nValue;
}
int KWDGSAttributeGrouping::GetCatchAllValueNumber() const
{
	return nCatchAllValueNumber;
}

int KWDGSAttributeGrouping::GetAttributeType() const
{
	return KWType::Symbol;
}

boolean KWDGSAttributeGrouping::ArePartsSingletons() const
{
	return false;
}

void KWDGSAttributeGrouping::SetPartNumber(int nValue)
{
	require(nValue >= 0);
	ivGroupFirstValueIndexes.SetSize(nValue);
}

int KWDGSAttributeGrouping::GetPartNumber() const
{
	return ivGroupFirstValueIndexes.GetSize();
}

int KWDGSAttributeGrouping::ComputeSymbolPartIndex(const Symbol& sValue) const
{
	int nGroup;
	int nValue;
	int nDefaultGroup;

	require(Check());

	// Parcours des groupes
	nDefaultGroup = -1;
	for (nGroup = 0; nGroup < GetGroupNumber(); nGroup++)
	{
		// Parcours des valeurs du groupe
		for (nValue = GetGroupFirstValueIndexAt(nGroup); nValue <= GetGroupLastValueIndexAt(nGroup); nValue++)
		{
			// Si valeur egale: on a trouve le groupe
			if (GetValueAt(nValue) == sValue)
				return nGroup;

			// On recherche le groupe par defaut contenant la StarValue
			if (GetValueAt(nValue) == Symbol::GetStarValue())
			{
				assert(nDefaultGroup == -1);
				nDefaultGroup = nGroup;
			}
		}
	}

	// Si groupe non trouve: on retourne le groupe par defaut
	assert(nDefaultGroup >= 0);
	return nDefaultGroup;
}

boolean KWDGSAttributeGrouping::Check() const
{
	boolean bOk;
	int nGroup;
	ALString sTmp;
	SymbolVector svSortedValues;
	int nValue;
	boolean bStarValueFound;

	// Verification standard
	bOk = KWDGSAttributePartition::Check();

	// Verification de l'unicite des valeurs, en se basant sur un tri de ces valeurs
	// (plus econome en memoire que l'utilisation d'un dictionnaire)
	svSortedValues.CopyFrom(&svValues);
	svSortedValues.SortKeys();
	if (bOk)
	{
		for (nValue = 1; nValue < svSortedValues.GetSize(); nValue++)
		{
			if (not(svSortedValues.GetAt(nValue - 1) < svSortedValues.GetAt(nValue)))
			{
				AddError(sTmp + "Value \"" + svSortedValues.GetAt(nValue - 1).GetValue() +
					 "\" is used several times");
				bOk = false;
				break;
			}
		}
	}

	// Verification de la presence de la StarValue
	if (bOk)
	{
		bStarValueFound = false;
		for (nValue = 0; nValue < svValues.GetSize(); nValue++)
		{
			if (svValues.GetAt(nValue) == Symbol::GetStarValue())
			{
				bStarValueFound = true;
				break;
			}
		}

		// Erreur si pas de de StarValue
		if (not bStarValueFound)
		{
			AddError(sTmp + "Special grouping value " + Symbol::GetStarValue() +
				 " is not specified in any part");
			bOk = false;
		}
	}

	// Verification de l'unicite et de l'ordre des index de premiere valeur de groupe
	if (bOk)
	{
		assert(ivGroupFirstValueIndexes.GetSize() > 0);

		// Verification du premier index
		if (ivGroupFirstValueIndexes.GetAt(0) != 0)
		{
			AddError(sTmp + "The index of first value of first group should be 0");
			bOk = false;
		}

		// Verification de l'ordre des index
		for (nGroup = 1; nGroup < ivGroupFirstValueIndexes.GetSize(); nGroup++)
		{
			if (not(ivGroupFirstValueIndexes.GetAt(nGroup - 1) < ivGroupFirstValueIndexes.GetAt(nGroup)))
			{
				AddError(sTmp + "The index of the first value of group " + IntToString(nGroup) +
					 " is not inferior to that of the following group");
				bOk = false;
				break;
			}
		}

		// Verification du dernier index
		if (ivGroupFirstValueIndexes.GetAt(ivGroupFirstValueIndexes.GetSize() - 1) >= svValues.GetSize())
		{
			AddError(sTmp +
				 "Index of first value of last group should be inferior to the number of values");
			bOk = false;
		}
	}
	return bOk;
}

longint KWDGSAttributeGrouping::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGSAttributePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDGSAttributeGrouping) - sizeof(KWDGSAttributePartition);
	lUsedMemory += svValues.GetUsedMemory();
	lUsedMemory += ivGroupFirstValueIndexes.GetUsedMemory();
	return lUsedMemory;
}

void KWDGSAttributeGrouping::WritePartHeader(ostream& ost) const
{
	ost << "Group";
}

void KWDGSAttributeGrouping::WritePartAt(ostream& ost, int nPartIndex) const
{
	int nValue;
	int nFirstValue;
	int nLastValue;
	int nNumber;
	ALString sGroupName;

	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	// Libelle base sur l'ensemble des valeurs
	nFirstValue = GetGroupFirstValueIndexAt(nPartIndex);
	nLastValue = GetGroupLastValueIndexAt(nPartIndex);
	sGroupName = "{";
	nNumber = 0;
	for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
	{
		// On n'utilise la modalite speciale pour fabriquer le libelle
		if (GetValueAt(nValue) != Symbol::GetStarValue())
		{
			// Prise en compte si moins de trois valeurs
			if (nNumber < 3)
			{
				if (nNumber > 0)
					sGroupName += ", ";
				sGroupName += GetValueAt(nValue);
				nNumber++;
			}
			// Arret si au moins quatre valeurs
			else
			{
				sGroupName += ", ...";
				break;
			}
		}
	}
	sGroupName += "}";

	// Affichage du groupe apres formatage
	ost << TSV::Export(sGroupName);
}

boolean KWDGSAttributeGrouping::IsPartDetailsReported() const
{
	return true;
}

void KWDGSAttributeGrouping::WritePartDetailsHeader(ostream& ost) const
{
	ost << "Size\tValue list";
}

void KWDGSAttributeGrouping::WritePartDetailsAt(ostream& ost, int nPartIndex) const
{
	int nValue;
	int nFirstValue;
	int nLastValue;
	int nNumber;
	boolean bStarValue;

	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	// Calcul du nombre de valeurs, en excluant la StarValue
	nFirstValue = GetGroupFirstValueIndexAt(nPartIndex);
	nLastValue = GetGroupLastValueIndexAt(nPartIndex);
	nNumber = 0;
	bStarValue = false;
	for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
	{
		if (GetValueAt(nValue) != Symbol::GetStarValue())
			nNumber++;
		else
			bStarValue = true;
	}
	assert(abs(nNumber - GetGroupValueNumberAt(nPartIndex)) <= 1);

	// Affichage du nombre de valeurs

	if (bStarValue)
	{
		// Fourre tout non poubelle
		if (nGarbageGroupIndex != nPartIndex)
			ost << nNumber + nCatchAllValueNumber;
		// Poubelle (qui peut par ailleurs contenir la super-modalite representante du fourre-tout)
		else
			ost << nGarbageModalityNumber + nCatchAllValueNumber;
	}
	else
		ost << nNumber;

	// Liste des valeurs hors valeur speciale
	nNumber = 0;
	for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
	{
		ost << "\t" << TSV::Export(GetValueAt(nValue).GetValue());
		nNumber++;
	}
}

void KWDGSAttributeGrouping::WriteJSONFields(JSONFile* fJSON) const
{
	int nDefaultPartIndex;
	int nPart;
	int nValue;
	int nFirstValue;
	int nLastValue;

	// Appel de la methode ancetre
	KWDGSAttributePartition::WriteJSONFields(fJSON);

	// Recherche de l'index de la partie par defaut
	nDefaultPartIndex = -1;
	for (nPart = 0; nPart < GetPartNumber(); nPart++)
	{
		// Recherche dans la partie courante
		nFirstValue = GetGroupFirstValueIndexAt(nPart);
		nLastValue = GetGroupLastValueIndexAt(nPart);
		for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
		{
			if (GetValueAt(nValue) == Symbol::GetStarValue())
			{
				nDefaultPartIndex = nPart;
				break;
			}
		}
	}
	assert(nDefaultPartIndex != -1);

	// Indication de groupe par defaut
	fJSON->WriteKeyInt("defaultGroupIndex", nDefaultPartIndex);
}

void KWDGSAttributeGrouping::WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) const
{
	ALString sPartLabel;
	int nValue;
	int nFirstValue;
	int nLastValue;

	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	// Valeurs de la partie
	fJSON->BeginList();
	nFirstValue = GetGroupFirstValueIndexAt(nPartIndex);
	nLastValue = GetGroupLastValueIndexAt(nPartIndex);
	for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
	{
		if (GetValueAt(nValue) != Symbol::GetStarValue())
			fJSON->WriteString(GetValueAt(nValue).GetValue());
	}
	fJSON->EndList();
}

KWDGSAttributePartition* KWDGSAttributeGrouping::Create() const
{
	return new KWDGSAttributeGrouping;
}

void KWDGSAttributeGrouping::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	KWDGSAttributeGrouping* sourcePartition;

	require(kwdgsapSource != NULL);

	// Copie de la methode ancetre
	KWDGSAttributePartition::CopyFrom(kwdgsapSource);

	// Copie specifique
	sourcePartition = cast(KWDGSAttributeGrouping*, kwdgsapSource);
	svValues.CopyFrom(&(sourcePartition->svValues));
	ivGroupFirstValueIndexes.CopyFrom(&(sourcePartition->ivGroupFirstValueIndexes));

	nGarbageGroupIndex = sourcePartition->nGarbageGroupIndex;
	nGarbageModalityNumber = sourcePartition->nGarbageModalityNumber;
	nCatchAllValueNumber = sourcePartition->nCatchAllValueNumber;
}

int KWDGSAttributeGrouping::Compare(const KWDGSAttributePartition* kwdgsapSource) const
{
	int nCompare;
	int n;
	KWDGSAttributeGrouping* sourcePartition;

	require(kwdgsapSource != NULL);

	// Appel de la methode ancetre
	nCompare = KWDGSAttributePartition::Compare(kwdgsapSource);

	// Comparaison specifique
	if (nCompare == 0)
	{
		sourcePartition = cast(KWDGSAttributeGrouping*, kwdgsapSource);

		// Nombre de valeurs et de groupes
		if (nCompare == 0)
			nCompare = GetKeptValueNumber() - sourcePartition->GetKeptValueNumber();
		if (nCompare == 0)
			nCompare = GetGroupNumber() - sourcePartition->GetGroupNumber();

		// Valeurs
		if (nCompare == 0)
			for (n = 0; n < GetKeptValueNumber(); n++)
			{
				nCompare = GetValueAt(n).Compare(sourcePartition->GetValueAt(n));
				if (nCompare != 0)
					break;
			}

		// Groupes
		if (nCompare == 0)
			for (n = 0; n < GetGroupNumber(); n++)
			{
				nCompare = GetGroupFirstValueIndexAt(n) - sourcePartition->GetGroupFirstValueIndexAt(n);
				if (nCompare != 0)
					break;
			}

		// Index du groupe poubelle
		if (nCompare == 0)
			nCompare = GetGarbageGroupIndex() - sourcePartition->GetGarbageGroupIndex();
	}
	return nCompare;
}

const ALString KWDGSAttributeGrouping::GetClassLabel() const
{
	return "Value groups";
}

void KWDGSAttributeGrouping::SetKeptValueNumber(int nValue)
{
	require(nValue >= 0);
	svValues.SetSize(nValue);
}

int KWDGSAttributeGrouping::GetKeptValueNumber() const
{
	return svValues.GetSize();
}

void KWDGSAttributeGrouping::SetValueAt(int nIndex, const Symbol& sValue)
{
	svValues.SetAt(nIndex, sValue);
}

Symbol& KWDGSAttributeGrouping::GetValueAt(int nIndex) const
{
	return svValues.GetAt(nIndex);
}

int KWDGSAttributeGrouping::GetGroupNumber() const
{
	return GetPartNumber();
}

void KWDGSAttributeGrouping::SetGroupFirstValueIndexAt(int nGroupIndex, int nFirstValueIndex)
{
	require(nFirstValueIndex >= 0);
	ivGroupFirstValueIndexes.SetAt(nGroupIndex, nFirstValueIndex);
}

int KWDGSAttributeGrouping::GetGroupFirstValueIndexAt(int nGroupIndex) const
{
	return ivGroupFirstValueIndexes.GetAt(nGroupIndex);
}

int KWDGSAttributeGrouping::GetGroupLastValueIndexAt(int nGroupIndex) const
{
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());

	// On renvoie soit l'index de la valeur precedant la premiere valeur du groupe suivant
	if (nGroupIndex < ivGroupFirstValueIndexes.GetSize() - 1)
		return ivGroupFirstValueIndexes.GetAt(nGroupIndex + 1) - 1;
	// Soit l'index de la derniere valeur
	else
		return svValues.GetSize() - 1;
}

int KWDGSAttributeGrouping::GetGroupValueNumberAt(int nGroupIndex) const
{
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());

	return GetGroupLastValueIndexAt(nGroupIndex) - GetGroupFirstValueIndexAt(nGroupIndex) + 1;
}

KWDGSAttributeGrouping* KWDGSAttributeGrouping::CreateTestAttribute(int nPartNumber)
{
	KWDGSAttributeGrouping* attribute;
	const ALString sValuePrefix = "V";
	const int nGroupSize = 3;
	int nValue;
	int nGroup;

	require(nPartNumber >= 0);

	// Creation de l'attribut
	attribute = new KWDGSAttributeGrouping;
	attribute->SetAttributeName("Att");

	// Creation des valeurs
	attribute->SetKeptValueNumber(nPartNumber * nGroupSize);
	for (nValue = 0; nValue < attribute->GetKeptValueNumber(); nValue++)
		attribute->SetValueAt(nValue, Symbol(sValuePrefix + IntToString(nValue + 1)));

	// On met la StarValue en derniere valeur
	attribute->SetValueAt(attribute->GetKeptValueNumber() - 1, Symbol::GetStarValue());

	// Creation des groupes
	attribute->GetKeptValueNumber();
	attribute->SetGranularizedValueNumber(nPartNumber);
	attribute->SetPartNumber(nPartNumber);
	for (nGroup = 0; nGroup < attribute->GetGroupNumber(); nGroup++)
		attribute->SetGroupFirstValueIndexAt(nGroup, nGroup * nGroupSize);
	ensure(attribute->Check());
	return attribute;
}

void KWDGSAttributeGrouping::Test()
{
	KWDGSAttributePartition* attribute;

	attribute = CreateTestAttribute(4);
	attribute->Write(cout);
	cout << endl;
	delete attribute;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeContinuousValues

KWDGSAttributeContinuousValues::KWDGSAttributeContinuousValues() {}

KWDGSAttributeContinuousValues::~KWDGSAttributeContinuousValues() {}

int KWDGSAttributeContinuousValues::GetAttributeType() const
{
	return KWType::Continuous;
}

boolean KWDGSAttributeContinuousValues::ArePartsSingletons() const
{
	return true;
}

void KWDGSAttributeContinuousValues::SetPartNumber(int nValue)
{
	cvValues.SetSize(nValue);
}

int KWDGSAttributeContinuousValues::GetPartNumber() const
{
	return cvValues.GetSize();
}

int KWDGSAttributeContinuousValues::ComputeContinuousPartIndex(Continuous cValue) const
{
	int nIndex;

	require(Check());

	// Parcours des valeurs
	for (nIndex = 0; nIndex < cvValues.GetSize(); nIndex++)
	{
		if (cValue == cvValues.GetAt(nIndex))
			return nIndex;
	}

	// Si non trouve: on retourne -1
	return -1;
}

boolean KWDGSAttributeContinuousValues::Check() const
{
	boolean bOk;
	ALString sTmp;
	int nValue;

	// Verification standard
	bOk = KWDGSAttributePartition::Check();

	// Verification de l'unicite et de l'ordre des valeurs
	if (bOk)
	{
		for (nValue = 1; nValue < cvValues.GetSize(); nValue++)
		{
			assert(KWContinuous::GetMissingValue() < KWContinuous::GetMinValue());
			assert(KWContinuous::GetMissingValue() < cvValues.GetAt(nValue));
			assert(cvValues.GetAt(nValue - 1) < KWContinuous::GetMaxValue());
			if (not(cvValues.GetAt(nValue - 1) < cvValues.GetAt(nValue)))
			{
				AddError(sTmp + "Value \"" +
					 KWContinuous::ContinuousToString(cvValues.GetAt(nValue - 1)) +
					 "\" is used several times");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

longint KWDGSAttributeContinuousValues::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGSAttributePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDGSAttributeContinuousValues) - sizeof(KWDGSAttributePartition);
	lUsedMemory += cvValues.GetUsedMemory();
	return lUsedMemory;
}

void KWDGSAttributeContinuousValues::WritePartHeader(ostream& ost) const
{
	ost << "Value";
}

void KWDGSAttributeContinuousValues::WritePartAt(ostream& ost, int nPartIndex) const
{
	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	if (cvValues.GetAt(nPartIndex) == KWContinuous::GetMissingValue())
		ost << "Missing";
	else
		ost << cvValues.GetAt(nPartIndex);
}

void KWDGSAttributeContinuousValues::WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) const
{
	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	if (cvValues.GetAt(nPartIndex) == KWContinuous::GetMissingValue())
		fJSON->WriteString("Missing");
	else
		fJSON->WriteString(KWContinuous::ContinuousToString(cvValues.GetAt(nPartIndex)));
}

KWDGSAttributePartition* KWDGSAttributeContinuousValues::Create() const
{
	return new KWDGSAttributeContinuousValues;
}

void KWDGSAttributeContinuousValues::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	KWDGSAttributeContinuousValues* sourcePartition;

	require(kwdgsapSource != NULL);

	// Copie de la methode ancetre
	KWDGSAttributePartition::CopyFrom(kwdgsapSource);

	// Copie specifique
	sourcePartition = cast(KWDGSAttributeContinuousValues*, kwdgsapSource);
	cvValues.CopyFrom(&(sourcePartition->cvValues));
}

int KWDGSAttributeContinuousValues::Compare(const KWDGSAttributePartition* kwdgsapSource) const
{
	int nCompare;
	int n;
	KWDGSAttributeContinuousValues* sourcePartition;

	require(kwdgsapSource != NULL);

	// Appel de la methode ancetre
	nCompare = KWDGSAttributePartition::Compare(kwdgsapSource);

	// Comparaison specifique
	if (nCompare == 0)
	{
		sourcePartition = cast(KWDGSAttributeContinuousValues*, kwdgsapSource);
		assert(cvValues.GetSize() == sourcePartition->cvValues.GetSize());
		for (n = 0; n < cvValues.GetSize(); n++)
		{
			nCompare = KWContinuous::Compare(cvValues.GetAt(n), sourcePartition->cvValues.GetAt(n));
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

const ALString KWDGSAttributeContinuousValues::GetClassLabel() const
{
	return "Values";
}

int KWDGSAttributeContinuousValues::GetValueNumber() const
{
	return GetPartNumber();
}

void KWDGSAttributeContinuousValues::SetValueAt(int nIndex, Continuous cValue)
{
	cvValues.SetAt(nIndex, cValue);
}

Continuous KWDGSAttributeContinuousValues::GetValueAt(int nIndex) const
{
	return cvValues.GetAt(nIndex);
}

KWDGSAttributeContinuousValues* KWDGSAttributeContinuousValues::CreateTestAttribute(int nPartNumber)
{
	KWDGSAttributeContinuousValues* attribute;
	const ALString sValuePrefix = "V";
	int nValue;

	require(nPartNumber >= 0);

	// Creation de l'attribut
	attribute = new KWDGSAttributeContinuousValues;
	attribute->SetAttributeName("Att");

	// Creation des valeurs
	attribute->SetInitialValueNumber(nPartNumber);
	attribute->SetGranularizedValueNumber(nPartNumber);
	attribute->SetPartNumber(nPartNumber);
	for (nValue = 0; nValue < attribute->GetValueNumber(); nValue++)
		attribute->SetValueAt(nValue, (Continuous)(1.0 + nValue));
	ensure(attribute->Check());
	return attribute;
}

void KWDGSAttributeContinuousValues::Test()
{
	KWDGSAttributePartition* attribute;

	attribute = CreateTestAttribute(4);
	attribute->Write(cout);
	cout << endl;
	delete attribute;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeSymbolValues

KWDGSAttributeSymbolValues::KWDGSAttributeSymbolValues() {}

KWDGSAttributeSymbolValues::~KWDGSAttributeSymbolValues() {}

int KWDGSAttributeSymbolValues::GetAttributeType() const
{
	return KWType::Symbol;
}

boolean KWDGSAttributeSymbolValues::ArePartsSingletons() const
{
	return true;
}

void KWDGSAttributeSymbolValues::SetPartNumber(int nValue)
{
	svValues.SetSize(nValue);
}

int KWDGSAttributeSymbolValues::GetPartNumber() const
{
	return svValues.GetSize();
}

int KWDGSAttributeSymbolValues::ComputeSymbolPartIndex(const Symbol& sValue) const
{
	int nIndex;
	int nDefaultGroup;

	require(Check());

	// Parcours des valeurs
	nDefaultGroup = -1;
	for (nIndex = 0; nIndex < svValues.GetSize(); nIndex++)
	{
		if (sValue == svValues.GetAt(nIndex))
			return nIndex;

		// On recherche le groupe par defaut contenant la StarValue
		if (svValues.GetAt(nIndex) == Symbol::GetStarValue())
		{
			assert(nDefaultGroup == -1);
			nDefaultGroup = nIndex;
		}
	}

	// Si non trouve: on retourne l'index par defaut (-1 s'il n'en existe pas)
	return nDefaultGroup;
}

boolean KWDGSAttributeSymbolValues::Check() const
{
	boolean bOk;
	ALString sTmp;
	SymbolVector svSortedValues;
	int nValue;

	// Verification standard
	bOk = KWDGSAttributePartition::Check();

	// Verification de l'unicite des valeurs, en se basant sur un tri de ces valeurs
	// (plus econome en memoire que l'utilisation d'un dictionnaire)
	svSortedValues.CopyFrom(&svValues);
	svSortedValues.SortKeys();
	if (bOk)
	{
		for (nValue = 1; nValue < svSortedValues.GetSize(); nValue++)
		{
			if (not(svSortedValues.GetAt(nValue - 1) < svSortedValues.GetAt(nValue)))
			{
				AddError(sTmp + "Value \"" + svSortedValues.GetAt(nValue - 1).GetValue() +
					 "\" is used several times");
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

longint KWDGSAttributeSymbolValues::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGSAttributePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDGSAttributeSymbolValues) - sizeof(KWDGSAttributePartition);
	lUsedMemory += svValues.GetUsedMemory();
	return lUsedMemory;
}

void KWDGSAttributeSymbolValues::WritePartHeader(ostream& ost) const
{
	ost << "Value";
}

void KWDGSAttributeSymbolValues::WritePartAt(ostream& ost, int nPartIndex) const
{
	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	ost << TSV::Export(svValues.GetAt(nPartIndex).GetValue());
}

void KWDGSAttributeSymbolValues::WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) const
{
	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	fJSON->WriteString(svValues.GetAt(nPartIndex).GetValue());
}

KWDGSAttributePartition* KWDGSAttributeSymbolValues::Create() const
{
	return new KWDGSAttributeSymbolValues;
}

void KWDGSAttributeSymbolValues::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	KWDGSAttributeSymbolValues* sourcePartition;

	require(kwdgsapSource != NULL);

	// Copie de la methode ancetre
	KWDGSAttributePartition::CopyFrom(kwdgsapSource);

	// Copie specifique
	sourcePartition = cast(KWDGSAttributeSymbolValues*, kwdgsapSource);
	svValues.CopyFrom(&(sourcePartition->svValues));
}

int KWDGSAttributeSymbolValues::Compare(const KWDGSAttributePartition* kwdgsapSource) const
{
	int nCompare;
	int n;
	KWDGSAttributeSymbolValues* sourcePartition;

	require(kwdgsapSource != NULL);

	// Appel de la methode ancetre
	nCompare = KWDGSAttributePartition::Compare(kwdgsapSource);

	// Comparaison specifique
	if (nCompare == 0)
	{
		sourcePartition = cast(KWDGSAttributeSymbolValues*, kwdgsapSource);

		// Nombre de valeurs et de groupes
		if (nCompare == 0)
			nCompare = GetValueNumber() - sourcePartition->GetValueNumber();

		// Valeurs
		if (nCompare == 0)
			for (n = 0; n < GetValueNumber(); n++)
			{
				nCompare = GetValueAt(n).Compare(sourcePartition->GetValueAt(n));
				if (nCompare != 0)
					break;
			}
	}
	return nCompare;
}

const ALString KWDGSAttributeSymbolValues::GetClassLabel() const
{
	return "Values";
}

int KWDGSAttributeSymbolValues::GetValueNumber() const
{
	return GetPartNumber();
}

void KWDGSAttributeSymbolValues::SetValueAt(int nIndex, const Symbol& sValue)
{
	svValues.SetAt(nIndex, sValue);
}

Symbol& KWDGSAttributeSymbolValues::GetValueAt(int nIndex) const
{
	return svValues.GetAt(nIndex);
}

KWDGSAttributeSymbolValues* KWDGSAttributeSymbolValues::CreateTestAttribute(int nPartNumber)
{
	KWDGSAttributeSymbolValues* attribute;
	const ALString sValuePrefix = "V";
	int nValue;

	require(nPartNumber >= 0);

	// Creation de l'attribut
	attribute = new KWDGSAttributeSymbolValues;
	attribute->SetAttributeName("Att");

	// Creation des valeurs
	attribute->SetInitialValueNumber(nPartNumber);
	attribute->SetGranularizedValueNumber(nPartNumber);
	attribute->SetPartNumber(nPartNumber);
	for (nValue = 0; nValue < attribute->GetValueNumber(); nValue++)
		attribute->SetValueAt(nValue, Symbol(sValuePrefix + IntToString(nValue + 1)));
	ensure(attribute->Check());
	return attribute;
}

void KWDGSAttributeSymbolValues::Test()
{
	KWDGSAttributePartition* attribute;

	attribute = CreateTestAttribute(4);
	attribute->Write(cout);
	cout << endl;
	delete attribute;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSAttributeVirtualValues

KWDGSAttributeVirtualValues::KWDGSAttributeVirtualValues()
{
	nPartNumber = 0;
}

KWDGSAttributeVirtualValues::~KWDGSAttributeVirtualValues() {}

int KWDGSAttributeVirtualValues::GetAttributeType() const
{
	return KWType::Unknown;
}

boolean KWDGSAttributeVirtualValues::ArePartsSingletons() const
{
	return true;
}

void KWDGSAttributeVirtualValues::SetPartNumber(int nValue)
{
	require(nValue >= 0);
	nPartNumber = nValue;
}

int KWDGSAttributeVirtualValues::GetPartNumber() const
{
	return nPartNumber;
}

longint KWDGSAttributeVirtualValues::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGSAttributePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDGSAttributeVirtualValues) - sizeof(KWDGSAttributePartition);
	return lUsedMemory;
}

void KWDGSAttributeVirtualValues::WriteJSONPartFieldsAt(JSONFile* fJSON, int nPartIndex) const
{
	ALString sTmp;
	require(0 <= nPartIndex and nPartIndex < GetPartNumber());

	fJSON->WriteString(sTmp + "V" + IntToString(nPartIndex + 1));
}

KWDGSAttributePartition* KWDGSAttributeVirtualValues::Create() const
{
	return new KWDGSAttributeVirtualValues;
}

void KWDGSAttributeVirtualValues::CopyFrom(const KWDGSAttributePartition* kwdgsapSource)
{
	require(kwdgsapSource != NULL);

	KWDGSAttributePartition::CopyFrom(kwdgsapSource);
	SetPartNumber(kwdgsapSource->GetPartNumber());
}

KWDGSAttributeVirtualValues* KWDGSAttributeVirtualValues::CreateTestAttribute(int nPartNumber)
{
	KWDGSAttributeVirtualValues* attribute;

	require(nPartNumber >= 0);

	// Creation de l'attribut
	attribute = new KWDGSAttributeVirtualValues;
	attribute->SetAttributeName("Att");
	attribute->SetPartNumber(nPartNumber);
	return attribute;
}

void KWDGSAttributeVirtualValues::Test()
{
	KWDGSAttributePartition* attribute;

	attribute = CreateTestAttribute(4);
	attribute->Write(cout);
	cout << endl;
	delete attribute;
}

////////////////////////////////////////////////////////////////////
// Classe KWDGSCell

int KWDGSCellCompareIndexes(const void* elem1, const void* elem2)
{
	KWDGSCell* cell1;
	KWDGSCell* cell2;
	int nPartCompare;
	int nAttributeNumber;
	int nIndex;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGSCell*, *(Object**)elem1);
	cell2 = cast(KWDGSCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison lexicographique des parties references
	nAttributeNumber = cell1->GetAttributeNumber();
	for (nIndex = 0; nIndex < nAttributeNumber; nIndex++)
	{
		nPartCompare = cell1->GetPartIndexes()->GetAt(nIndex) - cell2->GetPartIndexes()->GetAt(nIndex);
		if (nPartCompare != 0)
			return nPartCompare;
	}
	return 0;
}

int KWDGSCellCompareDecreasingFrequency(const void* elem1, const void* elem2)
{
	KWDGSCell* cell1;
	KWDGSCell* cell2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGSCell*, *(Object**)elem1);
	cell2 = cast(KWDGSCell*, *(Object**)elem2);
	assert(cell1->GetPartIndexes()->GetSize() == cell2->GetPartIndexes()->GetSize());

	// Comparaison par effectif decroissant
	nCompare = -cell1->GetCellFrequency() + cell2->GetCellFrequency();

	// Prise en compte des valeurs des parties en cas d'egalite
	if (nCompare == 0)
		nCompare = KWDGSCellCompareIndexes(elem1, elem2);
	return nCompare;
}

int KWDGSSourceCellCompareDecreasingInterest(const void* elem1, const void* elem2)
{
	KWDGSSourceCell* cell1;
	KWDGSSourceCell* cell2;
	double dCompare;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGSSourceCell*, *(Object**)elem1);
	cell2 = cast(KWDGSSourceCell*, *(Object**)elem2);
	assert(cell1->GetPartIndexes()->GetSize() == cell2->GetPartIndexes()->GetSize());

	// Comparaison par Interest decroissante
	dCompare = -cell1->GetInterest() + cell2->GetInterest();
	if (dCompare > 0)
		nCompare = 1;
	else if (dCompare < 0)
		nCompare = -1;
	else
		nCompare = 0;

	// Prise en compte de l'effectif en cas d'egalite
	if (nCompare == 0)
		nCompare = KWDGSCellCompareDecreasingFrequency(elem1, elem2);
	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DataGridStats

PLShared_DataGridStats::PLShared_DataGridStats() {}

PLShared_DataGridStats::~PLShared_DataGridStats() {}

void PLShared_DataGridStats::SetDataGridStats(KWDataGridStats* dataGrid)
{
	require(dataGrid != NULL);
	SetObject(dataGrid);
}

KWDataGridStats* PLShared_DataGridStats::GetDataGridStats()
{
	return cast(KWDataGridStats*, GetObject());
}

boolean PLShared_DataGridStats::TestDataGrid(KWDataGridStats* testDataGrid)
{
	KWDataGridStats* serializedDataGrid;
	PLShared_DataGridStats shared_dataGrid;
	PLSerializer serializer;
	boolean bTest;

	serializedDataGrid = new KWDataGridStats;
	cout << "Grille initiale" << endl;
	cout << *testDataGrid << endl;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_dataGrid.SerializeObject(&serializer, testDataGrid);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_dataGrid.DeserializeObject(&serializer, serializedDataGrid);
	serializer.Close();

	cout << endl << "Grille serialisee" << endl;
	cout << *serializedDataGrid;
	bTest = serializedDataGrid->Compare(testDataGrid) == 0;
	delete serializedDataGrid;
	delete testDataGrid;
	return bTest;
}

boolean PLShared_DataGridStats::Test()
{
	boolean bTest;
	bTest = true;

	// Non supervise
	cout << "Unsupervised (1)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(0, 1, false, 0, 2, 3));

	//
	cout << "Unsupervised (2)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(1, 1, false, 0, 2, 3));

	//
	cout << "Unsupervised simple(3)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(2, 1, true, 0, 2, 3));

	// Supervise
	cout << "Supervised (1, 1)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(1, 1, false, 1, 2, 3));

	//
	cout << "Supervised simple (2, 1)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(2, 1, true, 2, 2, 3));

	//
	cout << "Supervised(1, 2)\n--------------------\n";
	TestDataGrid(KWDataGridStats::CreateTestDataGrid(1, 2, false, 1, 2, 3));

	//
	cout << "Supervised(2, 2)\n--------------------\n";
	bTest = bTest and TestDataGrid(KWDataGridStats::CreateTestDataGrid(2, 2, false, 2, 2, 3));
	return bTest;
}

void PLShared_DataGridStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDataGridStats* dataGrid;
	int i;
	KWDGSAttributePartition* attribute;
	PLShared_DGSAttributeDiscretization sharedAttributeDiscretization;
	PLShared_DGSAttributeGrouping sharedAttributeGrouping;
	PLShared_DGSAttributeContinuousValues sharedAttributeContinuousValues;
	PLShared_DGSAttributeSymbolValues sharedAttributeSymbolValues;

	require(serializer->IsOpenForWrite());

	dataGrid = cast(KWDataGridStats*, o);

	serializer->PutInt(dataGrid->GetSourceAttributeNumber());
	serializer->PutIntVector(&dataGrid->ivCellFrequencies);
	serializer->PutInt(dataGrid->GetMainTargetModalityIndex());

	// Serialization de la Granularite
	serializer->PutInt(dataGrid->GetGranularity());

	// Serialisation du nombre d'attributs
	serializer->PutInt(dataGrid->oaAttributes.GetSize());

	// Serialisation de chaque attribut
	for (i = 0; i < dataGrid->oaAttributes.GetSize(); i++)
	{
		attribute = cast(KWDGSAttributePartition*, dataGrid->oaAttributes.GetAt(i));
		serializer->PutInt(attribute->GetAttributeType());
		serializer->PutBoolean(attribute->ArePartsSingletons());

		// Serialisation de l'attribut.
		// On determine son type grace au type (continu ou symbique) et a la nature de ses parties (singleton ou
		// non)
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			if (attribute->ArePartsSingletons())
			{
				// type KWDGSAttributeSymbolValues
				sharedAttributeSymbolValues.SerializeObject(serializer, attribute);
			}
			else
			{
				// type KWDGSAttributeGrouping
				sharedAttributeGrouping.SerializeObject(serializer, attribute);
			}
		}
		else
		{
			if (attribute->ArePartsSingletons())
			{
				// type KWDGSAttributeContinuousValues
				sharedAttributeContinuousValues.SerializeObject(serializer, attribute);
			}
			else
			{
				// type KWDGSAttributeDiscretization
				sharedAttributeDiscretization.SerializeObject(serializer, attribute);
			}
		}
	}
}

void PLShared_DataGridStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDataGridStats* dataGrid;
	int nAttributNumber;
	int i;
	int nAttributeType;
	boolean bArePartsSingletons;
	PLShared_DGSAttributeDiscretization sharedAttributeDiscretization;
	PLShared_DGSAttributeGrouping sharedAttributeGrouping;
	PLShared_DGSAttributeContinuousValues sharedAttributeContinuousValues;
	PLShared_DGSAttributeSymbolValues sharedAttributeSymbolValues;
	KWDGSAttributePartition* attribute;

	require(serializer->IsOpenForRead());

	attribute = NULL;
	dataGrid = cast(KWDataGridStats*, o);
	dataGrid->nSourceAttributeNumber = serializer->GetInt();
	serializer->GetIntVector(&dataGrid->ivCellFrequencies);
	dataGrid->SetMainTargetModalityIndex(serializer->GetInt());

	// Deserialization de la Granularite
	dataGrid->SetGranularity(serializer->GetInt());

	// Deserialisation du nombre d'attributs
	nAttributNumber = serializer->GetInt();

	// Deserialisation de chaque attribut
	for (i = 0; i < nAttributNumber; i++)
	{
		nAttributeType = serializer->GetInt();
		bArePartsSingletons = serializer->GetBoolean();

		// Deserialization de l'attribut.
		// On determine son type grace au type (continu ou symbique) et a la nature de ses parties (singleton ou
		// non)
		if (nAttributeType == KWType::Symbol)
		{
			if (bArePartsSingletons)
			{
				// type KWDGSAttributeSymbolValues
				attribute = new KWDGSAttributeSymbolValues;
				sharedAttributeSymbolValues.DeserializeObject(serializer, attribute);
			}
			else
			{
				// type KWDGSAttributeGrouping
				attribute = new KWDGSAttributeGrouping;
				sharedAttributeGrouping.DeserializeObject(serializer, attribute);
			}
		}
		else
		{
			if (bArePartsSingletons)
			{
				// type KWDGSAttributeContinuousValues
				attribute = new KWDGSAttributeContinuousValues;
				sharedAttributeContinuousValues.DeserializeObject(serializer, attribute);
			}
			else
			{
				// type KWDGSAttributeDiscretization
				attribute = new KWDGSAttributeDiscretization;
				sharedAttributeDiscretization.DeserializeObject(serializer, attribute);
			}
		}

		// Ajout de l'attribut deserialise dans la grille
		dataGrid->oaAttributes.Add(attribute);
	}
}

Object* PLShared_DataGridStats::Create() const
{
	return new KWDataGridStats;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributePartition

PLShared_DGSAttributePartition::PLShared_DGSAttributePartition() {}

PLShared_DGSAttributePartition::~PLShared_DGSAttributePartition() {}

void PLShared_DGSAttributePartition::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDGSAttributePartition* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForWrite());

	attribute = cast(KWDGSAttributePartition*, o);

	// Serialisation des attributs de la classe
	serializer->PutString(attribute->GetAttributeName());
	serializer->PutInt(attribute->GetInitialValueNumber());
	serializer->PutInt(attribute->GetGranularizedValueNumber());
}

void PLShared_DGSAttributePartition::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDGSAttributePartition* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForRead());

	attribute = cast(KWDGSAttributePartition*, o);

	// Deserialization des attributs de la classe
	attribute->SetAttributeName(serializer->GetString());
	attribute->SetInitialValueNumber(serializer->GetInt());
	attribute->SetGranularizedValueNumber(serializer->GetInt());
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeDiscretization

PLShared_DGSAttributeDiscretization::PLShared_DGSAttributeDiscretization() {}

PLShared_DGSAttributeDiscretization::~PLShared_DGSAttributeDiscretization() {}

void PLShared_DGSAttributeDiscretization::SetAttributePartition(KWDGSAttributeDiscretization* kwdgsAttribute)
{
	require(kwdgsAttribute != NULL);
	SetObject(kwdgsAttribute);
}

KWDGSAttributeDiscretization* PLShared_DGSAttributeDiscretization::GetAttributePartition()
{
	return cast(KWDGSAttributeDiscretization*, GetObject());
}

boolean PLShared_DGSAttributeDiscretization::Test()
{
	KWDGSAttributeDiscretization* attribute;
	KWDGSAttributeDiscretization* attribute2;
	PLShared_DGSAttributeDiscretization shared_attribute;
	PLSerializer serializer;
	boolean bTest;

	attribute = KWDGSAttributeDiscretization::CreateTestAttribute(4);
	attribute2 = new KWDGSAttributeDiscretization;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_attribute.SerializeObject(&serializer, attribute);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_attribute.DeserializeObject(&serializer, attribute2);
	serializer.Close();

	cout << "Attribut initial : " << endl;
	cout << *attribute << endl;

	cout << "Attribut deserialise : " << endl;
	cout << *attribute2 << endl;

	bTest = attribute->Compare(attribute2);
	delete attribute;
	delete attribute2;
	return bTest;
}

void PLShared_DGSAttributeDiscretization::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDGSAttributeDiscretization* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForWrite());

	attribute = cast(KWDGSAttributeDiscretization*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::SerializeObject(serializer, attribute);

	// Serialization du vecteur de continus
	sharedVector.SerializeObject(serializer, &attribute->cvIntervalBounds);
}

void PLShared_DGSAttributeDiscretization::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDGSAttributeDiscretization* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForRead());

	attribute = cast(KWDGSAttributeDiscretization*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::DeserializeObject(serializer, attribute);

	// Deserialization du vecteur de continus
	sharedVector.DeserializeObject(serializer, &attribute->cvIntervalBounds);
}

Object* PLShared_DGSAttributeDiscretization::Create() const
{
	return new KWDGSAttributeDiscretization;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeGrouping

PLShared_DGSAttributeGrouping::PLShared_DGSAttributeGrouping() {}

PLShared_DGSAttributeGrouping::~PLShared_DGSAttributeGrouping() {}

void PLShared_DGSAttributeGrouping::SetAttributePartition(KWDGSAttributeGrouping* kwdgsAttribute)
{
	require(kwdgsAttribute != NULL);
	SetObject(kwdgsAttribute);
}

KWDGSAttributeGrouping* PLShared_DGSAttributeGrouping::GetAttributePartition()
{
	return cast(KWDGSAttributeGrouping*, GetObject());
}

boolean PLShared_DGSAttributeGrouping::Test()
{
	KWDGSAttributeGrouping* attribute;
	KWDGSAttributeGrouping* attribute2;
	PLShared_DGSAttributeGrouping shared_attribute;
	PLSerializer serializer;
	boolean bTest;

	attribute = KWDGSAttributeGrouping::CreateTestAttribute(4);
	attribute2 = new KWDGSAttributeGrouping;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_attribute.SerializeObject(&serializer, attribute);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_attribute.DeserializeObject(&serializer, attribute2);
	serializer.Close();

	cout << "Attribut initial : " << endl;
	cout << *attribute << endl;

	cout << "Attribut deserialise : " << endl;
	cout << *attribute2 << endl;

	bTest = attribute->Compare(attribute2);
	delete attribute;
	delete attribute2;
	return bTest;
}

void PLShared_DGSAttributeGrouping::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDGSAttributeGrouping* attribute;
	PLShared_SymbolVector sharedVector;
	require(serializer->IsOpenForWrite());

	attribute = cast(KWDGSAttributeGrouping*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::SerializeObject(serializer, attribute);

	// serialisation des attributs
	sharedVector.SerializeObject(serializer, &attribute->svValues);
	serializer->PutIntVector(&attribute->ivGroupFirstValueIndexes);
	serializer->PutInt(attribute->GetGarbageGroupIndex());
	serializer->PutInt(attribute->GetGarbageModalityNumber());
	serializer->PutInt(attribute->GetCatchAllValueNumber());
}

void PLShared_DGSAttributeGrouping::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDGSAttributeGrouping* attribute;
	PLShared_SymbolVector sharedVector;

	require(serializer->IsOpenForRead());

	attribute = cast(KWDGSAttributeGrouping*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::DeserializeObject(serializer, attribute);

	// Deserialization des attributs
	sharedVector.DeserializeObject(serializer, &attribute->svValues);
	serializer->GetIntVector(&attribute->ivGroupFirstValueIndexes);
	attribute->SetGarbageGroupIndex(serializer->GetInt());
	attribute->SetGarbageModalityNumber(serializer->GetInt());
	attribute->SetCatchAllValueNumber(serializer->GetInt());
}

Object* PLShared_DGSAttributeGrouping::Create() const
{
	return new KWDGSAttributeGrouping;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeContinuousValues

PLShared_DGSAttributeContinuousValues::PLShared_DGSAttributeContinuousValues() {}

PLShared_DGSAttributeContinuousValues::~PLShared_DGSAttributeContinuousValues() {}

void PLShared_DGSAttributeContinuousValues::SetAttributePartition(KWDGSAttributeContinuousValues* kwdgsAttribute)
{
	require(kwdgsAttribute != NULL);
	SetObject(kwdgsAttribute);
}

KWDGSAttributeContinuousValues* PLShared_DGSAttributeContinuousValues::GetAttributePartition()
{
	return cast(KWDGSAttributeContinuousValues*, GetObject());
}

boolean PLShared_DGSAttributeContinuousValues::Test()
{
	KWDGSAttributeContinuousValues* attribute;
	KWDGSAttributeContinuousValues* attribute2;
	PLShared_DGSAttributeContinuousValues shared_attribute;
	PLSerializer serializer;
	boolean bTest;

	attribute = KWDGSAttributeContinuousValues::CreateTestAttribute(4);
	attribute2 = new KWDGSAttributeContinuousValues;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_attribute.SerializeObject(&serializer, attribute);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_attribute.DeserializeObject(&serializer, attribute2);
	serializer.Close();

	cout << "Attribut initial : " << endl;
	cout << *attribute << endl;

	cout << "Attribut deserialise : " << endl;
	cout << *attribute2 << endl;

	bTest = attribute->Compare(attribute2);
	delete attribute;
	delete attribute2;
	return bTest;
}

void PLShared_DGSAttributeContinuousValues::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDGSAttributeContinuousValues* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForWrite());

	attribute = cast(KWDGSAttributeContinuousValues*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::SerializeObject(serializer, attribute);

	// Serialisation des attributs
	sharedVector.SerializeObject(serializer, &attribute->cvValues);
}

void PLShared_DGSAttributeContinuousValues::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDGSAttributeContinuousValues* attribute;
	PLShared_ContinuousVector sharedVector;

	require(serializer->IsOpenForRead());

	attribute = cast(KWDGSAttributeContinuousValues*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::DeserializeObject(serializer, attribute);

	// Deserialization des attributs
	sharedVector.DeserializeObject(serializer, &attribute->cvValues);
}

Object* PLShared_DGSAttributeContinuousValues::Create() const
{
	return new KWDGSAttributeContinuousValues;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DGSAttributeSymbolValues

PLShared_DGSAttributeSymbolValues::PLShared_DGSAttributeSymbolValues() {}

PLShared_DGSAttributeSymbolValues::~PLShared_DGSAttributeSymbolValues() {}

void PLShared_DGSAttributeSymbolValues::SetAttributePartition(KWDGSAttributeSymbolValues* kwdgsAttribute)
{
	require(kwdgsAttribute != NULL);
	SetObject(kwdgsAttribute);
}

KWDGSAttributeSymbolValues* PLShared_DGSAttributeSymbolValues::GetAttributePartition()
{
	return cast(KWDGSAttributeSymbolValues*, GetObject());
}

boolean PLShared_DGSAttributeSymbolValues::Test()
{
	KWDGSAttributeSymbolValues* attribute;
	KWDGSAttributeSymbolValues* attribute2;
	PLShared_DGSAttributeSymbolValues shared_attribute;
	PLSerializer serializer;
	boolean bTest;

	attribute = KWDGSAttributeSymbolValues::CreateTestAttribute(4);
	attribute2 = new KWDGSAttributeSymbolValues;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_attribute.SerializeObject(&serializer, attribute);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_attribute.DeserializeObject(&serializer, attribute2);
	serializer.Close();

	cout << "Attribut initial : " << endl;
	cout << *attribute << endl;

	cout << "Attribut deserialise : " << endl;
	cout << *attribute2 << endl;

	bTest = attribute->Compare(attribute2);
	delete attribute;
	delete attribute2;
	return bTest;
}

void PLShared_DGSAttributeSymbolValues::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDGSAttributeSymbolValues* attribute;
	PLShared_SymbolVector sharedVector;

	require(serializer->IsOpenForWrite());

	attribute = cast(KWDGSAttributeSymbolValues*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::SerializeObject(serializer, attribute);

	// Serialisation des attributs
	sharedVector.SerializeObject(serializer, &attribute->svValues);
}

void PLShared_DGSAttributeSymbolValues::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDGSAttributeSymbolValues* attribute;
	PLShared_SymbolVector sharedVector;

	require(serializer->IsOpenForRead());

	attribute = cast(KWDGSAttributeSymbolValues*, o);

	// Appel de la methode ancetre
	PLShared_DGSAttributePartition::DeserializeObject(serializer, attribute);

	// Deserialization des attributs
	sharedVector.DeserializeObject(serializer, &attribute->svValues);
}

Object* PLShared_DGSAttributeSymbolValues::Create() const
{
	return new KWDGSAttributeSymbolValues;
}
