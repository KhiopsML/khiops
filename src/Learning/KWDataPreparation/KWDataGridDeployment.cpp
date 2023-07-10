// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridDeployment.h"

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridDeployment

KWDataGridDeployment::KWDataGridDeployment()
{
	nDeploymentAttributeIndex = -1;
	dgNewDeploymentPart = NULL;
	nDeploymentIndex = -1;
	nDistributionTotalFrequency = 0;
}

KWDataGridDeployment::~KWDataGridDeployment()
{
	// Reinitialisation de la grille en mode standard pour permettre sa destruction
	DeleteIndexingStructure();
	SetCellUpdateMode(false);

	// Destruction des resultats de deploiement
	oaDistributionFrequencyVectors.DeleteAll();
}

void KWDataGridDeployment::SetDeploymentAttributeIndex(int nValue)
{
	require(0 <= nValue and nValue < GetAttributeNumber());
	nDeploymentAttributeIndex = nValue;
}

int KWDataGridDeployment::GetDeploymentAttributeIndex() const
{
	return nDeploymentAttributeIndex;
}

KWDGAttribute* KWDataGridDeployment::GetDeploymentAttribute()
{
	if (0 <= nDeploymentAttributeIndex and nDeploymentAttributeIndex < GetAttributeNumber())
		return GetAttributeAt(nDeploymentAttributeIndex);
	else
		return NULL;
}

void KWDataGridDeployment::PrepareForDeployment()
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	IntVector* ivFrequencyVector;

	require(Check());
	require(GetDeploymentAttribute() != NULL);
	require(dgNewDeploymentPart == NULL);

	// Nettoyage prealable
	Clean();

	// Initialisation des resultats de deploiement
	nDeploymentIndex = -1;
	oaDistributionFrequencyVectors.SetSize(GetAttributeNumber());
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = GetAttributeAt(nAttribute);

		// Creation d'un vecteur d'effectifs par attribut de distribution
		if (nAttribute != nDeploymentAttributeIndex)
		{
			ivFrequencyVector = new IntVector;
			ivFrequencyVector->SetSize(dgAttribute->GetPartNumber());
			oaDistributionFrequencyVectors.SetAt(nAttribute, ivFrequencyVector);
		}
	}
	dvDeploymentDistances.SetSize(GetDeploymentAttribute()->GetPartNumber());

	// Creation d'une partie a deployer
	dgNewDeploymentPart = cast(KWDGMPart*, GetDeploymentAttribute()->AddPart());
	assert(dgNewDeploymentPart == GetDeploymentAttribute()->GetTailPart());

	// Ajout d'une valeur fictive dans le cas categoriel
	if (GetDeploymentAttribute()->GetAttributeType() == KWType::Symbol)
	{
		dgNewDeploymentPart->GetValueSet()->AddValue(
		    Symbol::BuildNewSymbol(" Coclustering New Deployment Part "));

		// On n'oublie pas de mettre a jour le nombre de valeurs
		GetDeploymentAttribute()->SetInitialValueNumber(GetDeploymentAttribute()->GetInitialValueNumber() + 1);

		// On doit mettre a jour egalement le nombre de valeurs granularisees : il faut que le nombre de
		// parties, qui vient d'etre incrementee, soit inferieur a GranularizedValueNumber Cet effectif n'est
		// pas un parametre du critere de cout pour le co-clustering
		GetDeploymentAttribute()->SetGranularizedValueNumber(
		    GetDeploymentAttribute()->GetGranularizedValueNumber() + 1);
	}

	// Mise a jour des statistiques sur la grille
	UpdateAllStatistics();

	// Initialisation de la structure de couts
	dataGridCosts.InitializeDefaultCosts(this);
	SetDataGridCosts(&dataGridCosts);
	InitializeAllCosts();

	// Initialisation de la table de hash des cellules
	CellDictionaryInit();

	// Construction de la structure d'indexation
	BuildIndexingStructure();

	// Mise en mode creation de cellule
	SetCellUpdateMode(true);

	// Initialisation des index des parties
	InitializePartIndexes();

	ensure(CheckDeploymentPreparation());
}

void KWDataGridDeployment::Clean()
{
	// Reinitialisation de la grille en mode standard
	DeleteIndexingStructure();
	SetCellUpdateMode(false);

	// Destruction des resultats de deploiement
	nDeploymentIndex = -1;
	oaDistributionFrequencyVectors.DeleteAll();
	dvDeploymentDistances.SetSize(0);
	nDistributionTotalFrequency = 0;

	// Destruction si necessaire de la partie de deployment
	if (dgNewDeploymentPart != NULL)
	{
		assert(dgNewDeploymentPart->GetCellNumber() == 0);
		assert(dgNewDeploymentPart->GetAttribute() == GetDeploymentAttribute());
		GetDeploymentAttribute()->DeletePart(dgNewDeploymentPart);
	}
	dgNewDeploymentPart = NULL;
}

void KWDataGridDeployment::ComputeDeploymentStats(const ObjectArray* oaDistributionValueVectors,
						  const IntVector* ivFrequencyVector)
{
	boolean bDisplayNewPart = false;
	boolean bDisplayOptimizationDetails = false;
	KWDGAttribute* dgDeploymentAttribute;
	KWDGAttribute* dgDistributionAttribute;
	KWDGPart* dgDeploymentPart;
	KWDGPart* dgDistributionPart;
	KWDGMCell* dgNewCell;
	KWDGMCell* dgCell;
	KWDGMPartMerge dgpmDeploymentPartMerge;
	ContinuousVector* cvDeploymentValues;
	SymbolVector* svDeploymentValues;
	IntVector* ivDistributionFrequencyVector;
	Continuous cValue;
	Symbol sValue;
	ObjectArray oaParts;
	ObjectArray oaNewCells;
	int nAttribute;
	int nPart;
	int nValue;
	int nCell;
	int nDistributionSize;
	int nFrequency;
	double dMergeCost;
	double dBestMergeCost;

	require(CheckDeploymentPreparation());
	require(oaDistributionValueVectors != NULL);
	require(oaDistributionValueVectors->GetSize() == GetAttributeNumber());
	require(oaDistributionValueVectors->GetAt(nDeploymentAttributeIndex) == NULL);

	// Nettoyage des resultats de deploiement
	nDeploymentIndex = -1;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (nAttribute != nDeploymentAttributeIndex)
			cast(IntVector*, oaDistributionFrequencyVectors.GetAt(nAttribute))->Initialize();
	}
	dvDeploymentDistances.Initialize();

	/////////////////////////////////////////////////////////////////////////////////
	// Initialisation: initialisation de la nouvelle partie

	// Acces a l'attribut de distribution de la grille
	dgDeploymentAttribute = GetDeploymentAttribute();
	assert(dgNewDeploymentPart->GetAttribute() == dgDeploymentAttribute);

	// Calcul du nombre de valeurs dans les distributions,
	// et verification des vecteurs de valeurs par attribut de distribution
	nDistributionSize = -1;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (nAttribute != nDeploymentAttributeIndex)
		{
			dgDistributionAttribute = cast(KWDGMAttribute*, GetAttributeAt(nAttribute));

			// Acces au valeurs de deploiement, et memorisation du nombre de valeurs a traiter
			if (dgDistributionAttribute->GetAttributeType() == KWType::Symbol)
			{
				svDeploymentValues = cast(SymbolVector*, oaDistributionValueVectors->GetAt(nAttribute));
				if (nDistributionSize == -1)
					nDistributionSize = svDeploymentValues->GetSize();
				assert(svDeploymentValues->GetSize() == nDistributionSize);
			}
			else
			{
				assert(dgDistributionAttribute->GetAttributeType() == KWType::Continuous);
				cvDeploymentValues =
				    cast(ContinuousVector*, oaDistributionValueVectors->GetAt(nAttribute));
				if (nDistributionSize == -1)
					nDistributionSize = cvDeploymentValues->GetSize();
				assert(cvDeploymentValues->GetSize() == nDistributionSize);
			}
		}
	}
	assert(nDistributionSize >= 0);

	// Par defaut, l'effectif total est egale a la taille des vecteurs de distribution
	nDistributionTotalFrequency = nDistributionSize;

	// Verification du vecteur d'effectif optionnel
	if (ivFrequencyVector != NULL)
		assert(ivFrequencyVector->GetSize() == nDistributionSize);

	// Creation des cellules de la nouvelle partie a partir des valeurs de ses distributions
	oaParts.SetSize(GetAttributeNumber());
	oaParts.SetAt(nDeploymentAttributeIndex, dgNewDeploymentPart);
	nFrequency = 1;
	for (nValue = 0; nValue < nDistributionSize; nValue++)
	{
		// Acces a l'effectif optionnel (sinon, l'effectif est de 1)
		if (ivFrequencyVector != NULL)
		{
			nFrequency = ivFrequencyVector->GetAt(nValue);
			assert(0 <= nFrequency);

			// Correction de l'effectif total
			nDistributionTotalFrequency += nFrequency - 1;
		}

		// Parcours des attributs de distribution
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			if (nAttribute != nDeploymentAttributeIndex)
			{
				dgDistributionAttribute = GetAttributeAt(nAttribute);

				// Acces au valeurs de deploiement, et recherche de la partie contenant la valeur
				if (dgDistributionAttribute->GetAttributeType() == KWType::Symbol)
				{
					svDeploymentValues =
					    cast(SymbolVector*, oaDistributionValueVectors->GetAt(nAttribute));
					sValue = svDeploymentValues->GetAt(nValue);
					dgDistributionPart = dgDistributionAttribute->LookupSymbolPart(sValue);
				}
				else
				{
					cvDeploymentValues =
					    cast(ContinuousVector*, oaDistributionValueVectors->GetAt(nAttribute));
					cValue = cvDeploymentValues->GetAt(nValue);
					dgDistributionPart = dgDistributionAttribute->LookupContinuousPart(cValue);
				}

				// Parametrage des parties de la cellule a rechercher
				oaParts.SetAt(nAttribute, dgDistributionPart);

				// Mise a jour de l'effectif de la partie de distribution concernee
				nPart = cast(KWDGDPart*, dgDistributionPart)->GetIndex();
				ivDistributionFrequencyVector =
				    cast(IntVector*, oaDistributionFrequencyVectors.GetAt(nAttribute));
				ivDistributionFrequencyVector->UpgradeAt(nPart, nFrequency);
			}
		}

		// Recherche de la cellule correspondante
		dgNewCell = cast(KWDGMCell*, LookupCell(&oaParts));
		if (dgNewCell == NULL)
		{
			dgNewCell = cast(KWDGMCell*, AddCell(&oaParts));
			CellDictionaryAddCell(dgNewCell);
			oaNewCells.Add(dgNewCell);
		}

		// Mise a jour de son effectif
		dgNewCell->SetCellFrequency(dgNewCell->GetCellFrequency() + nFrequency);
	}

	// Mise a jour de l'effectif et du cout de la nouvelle partie
	dgNewDeploymentPart->SetPartFrequency(nDistributionTotalFrequency);
	dgNewDeploymentPart->SetCost(dataGridCosts.ComputePartCost(dgNewDeploymentPart));

	// Calcul des couts de la nouvelle partie et de ses cellules
	for (nCell = 0; nCell < oaNewCells.GetSize(); nCell++)
	{
		dgNewCell = cast(KWDGMCell*, oaNewCells.GetAt(nCell));
		dgNewCell->SetCost(dataGridCosts.ComputeCellCost(dgNewCell));
	}

	// Affichage des caracteristiques de la nouvelle partie a deployer
	if (bDisplayNewPart)
	{
		cout << "New part\n";
		cout << "\tCell\tFrequency\tCost\n";
		for (nCell = 0; nCell < oaNewCells.GetSize(); nCell++)
		{
			dgNewCell = cast(KWDGMCell*, oaNewCells.GetAt(nCell));
			cout << "\t" << dgNewCell->GetObjectLabel() << "\t" << dgNewCell->GetCellFrequency() << "\t"
			     << dgNewCell->GetCost() << "\n";
		}
		cout << endl;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Deploiement: recherche de la partie en deploiement la plus proche

	// Parcours des parties en deploiement pour rechercher la plus proche de la nouvelle partie
	// La nouvelle partie (en fin de liste des parties de l'attribut de deploiement) n'est pas exploree
	assert(dgNewDeploymentPart == GetDeploymentAttribute()->GetTailPart());
	dBestMergeCost = DBL_MAX;
	nDeploymentIndex = -1;
	nPart = 0;
	dgDeploymentPart = GetDeploymentAttribute()->GetHeadPart();
	while (dgDeploymentPart != dgNewDeploymentPart)
	{
		// Creation et initialisation de la fusion
		dgpmDeploymentPartMerge.SetPart1(dgNewDeploymentPart);
		dgpmDeploymentPartMerge.SetPart2(cast(KWDGMPart*, dgDeploymentPart));
		assert(dgpmDeploymentPartMerge.Check());

		// Evaluation de la fusion
		dMergeCost = ComputeMergeCost(&dgpmDeploymentPartMerge);
		dvDeploymentDistances.SetAt(nPart, dMergeCost);
		if (bDisplayOptimizationDetails)
			cout << "\t\tPart " << dgDeploymentPart->GetObjectLabel() << "\t" << dMergeCost;

		// Test si amelioration du meilleurs cout
		if (dMergeCost < dBestMergeCost)
		{
			dBestMergeCost = dMergeCost;
			nDeploymentIndex = nPart;
			if (bDisplayOptimizationDetails)
				cout << "\tBest";
		}
		if (bDisplayOptimizationDetails)
			cout << endl;

		// Partie suivante
		nPart++;
		GetDeploymentAttribute()->GetNextPart(dgDeploymentPart);
	}
	assert(0 <= nDeploymentIndex and nDeploymentIndex < GetDeploymentAttribute()->GetPartNumber() - 1);

	/////////////////////////////////////////////////////////////////////////////////
	// Terminaison: nettoyage de la nouvelle partie

	// Supression des cellules crees
	for (nCell = 0; nCell < oaNewCells.GetSize(); nCell++)
	{
		dgCell = cast(KWDGMCell*, oaNewCells.GetAt(nCell));
		CellDictionaryRemoveCell(dgCell);
		DeleteCell(dgCell);
	}
	assert(dgNewDeploymentPart->GetCellNumber() == 0);
	dgNewDeploymentPart->SetPartFrequency(0);
	dgNewDeploymentPart->SetCost(dataGridCosts.ComputePartCost(dgNewDeploymentPart));

	ensure(CheckDeploymentPreparation());
}

boolean KWDataGridDeployment::IsDeploymentStatsComputed() const
{
	return nDeploymentIndex != -1;
}

int KWDataGridDeployment::GetDeploymentIndex() const
{
	require(IsDeploymentStatsComputed());
	return nDeploymentIndex;
}

const IntVector* KWDataGridDeployment::GetDistributionFrequenciesAt(int nDistributionAttributeIndex) const
{
	require(IsDeploymentStatsComputed());
	require(0 <= nDistributionAttributeIndex and nDistributionAttributeIndex < GetAttributeNumber());
	require(nDistributionAttributeIndex != nDeploymentAttributeIndex);
	return cast(const IntVector*, oaDistributionFrequencyVectors.GetAt(nDistributionAttributeIndex));
}

const DoubleVector* KWDataGridDeployment::GetDeploymentDistances() const
{
	require(IsDeploymentStatsComputed());
	return &dvDeploymentDistances;
}

int KWDataGridDeployment::GetDistributionTotalFrequency() const
{
	require(IsDeploymentStatsComputed());
	return nDistributionTotalFrequency;
}

boolean KWDataGridDeployment::CheckDeploymentPreparation() const
{
	boolean bOk = true;
	int nAttribute;

	require(Check());

	// Verification de la nouvelle partie de deploiement (par des assertion
	assert(dgNewDeploymentPart != NULL);
	assert(dgNewDeploymentPart->GetAttribute()->GetAttributeIndex() == nDeploymentAttributeIndex);
	assert(dgNewDeploymentPart->GetCellNumber() == 0);

	// La structure de cout doit etre initialisee
	if (bOk and GetDataGridCosts() == NULL)
	{
		AddError("Missing cost parameter");
		bOk = false;
	}

	// Les cout doivent etre correctement initialises
	if (bOk and not CheckAllCosts())
	{
		AddError("Costs are not correctly initialized");
		bOk = false;
	}

	// Pour le deploiement, l'attribut de distribution doit etre indexe
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			if (nAttribute != nDeploymentAttributeIndex and not GetAttributeAt(nAttribute)->IsIndexed())
			{
				AddError("Distribution variable " +
					 GetAttributeAt(1 - nDeploymentAttributeIndex)->GetAttributeName() +
					 " should be indexed");
				bOk = false;
				break;
			}
		}
	}

	// Pour le deploiement, la grille doit permettre la creation de cellules
	if (bOk and not GetCellUpdateMode())
	{
		AddError("The data grid should allow the creation of new cells");
		bOk = false;
	}

	// Verification du dictionnaire de cellules
	assert(nCellDictionaryCount >= GetCellNumber());

	return bOk;
}

boolean KWDataGridDeployment::Check() const
{
	boolean bOk = true;
	ALString sTmp;

	// Test de la methode ancetre
	bOk = KWDataGridMerger::Check();

	// Test s'il y a au moins deux variables
	if (bOk and GetAttributeNumber() < 2)
	{
		AddError("The number of variables should be at least two");
		bOk = false;
	}

	return bOk;
}

longint KWDataGridDeployment::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;
	IntVector* ivVector;

	lUsedMemory = KWDataGridMerger::GetUsedMemory();
	lUsedMemory += sizeof(KWDataGridDeployment) - sizeof(KWDataGridMerger);
	lUsedMemory += dvDeploymentDistances.GetUsedMemory() +
		       oaDistributionFrequencyVectors.GetSize() * (sizeof(Object*) + sizeof(IntVector));
	for (i = 0; i < oaDistributionFrequencyVectors.GetSize(); i++)
	{
		ivVector = cast(IntVector*, oaDistributionFrequencyVectors.GetAt(i));
		if (ivVector != NULL)
			lUsedMemory += ivVector->GetUsedMemory();
	}
	return lUsedMemory;
}

const ALString KWDataGridDeployment::GetClassLabel() const
{
	return "Data grid deployment";
}

const ALString KWDataGridDeployment::GetObjectLabel() const
{
	if (GetAttributeNumber() != 2)
		return KWDataGridMerger::GetObjectLabel();
	else
		return "(" + GetAttributeAt(0)->GetAttributeName() + ", " + GetAttributeAt(1)->GetAttributeName() + ")";
}

KWDGAttribute* KWDataGridDeployment::NewAttribute() const
{
	return new KWDGDAttribute;
}

void KWDataGridDeployment::InitializePartIndexes()
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	KWDGDPart* dgpPart;
	int nPart;

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = GetAttributeAt(nAttribute);

		// Indexation des parties de l'attributs
		nPart = 0;
		dgPart = dgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			dgpPart = cast(KWDGDPart*, dgPart);
			dgpPart->SetIndex(nPart);

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
			nPart++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGDAttribute

KWDGDAttribute::KWDGDAttribute() {}

KWDGDAttribute::~KWDGDAttribute() {}

KWDGPart* KWDGDAttribute::NewPart() const
{
	return new KWDGDPart;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGDPart

KWDGDPart::KWDGDPart()
{
	nIndex = 0;
}

KWDGDPart::~KWDGDPart() {}

void KWDGDPart::SetIndex(int nValue)
{
	require(nValue >= 0);
	nIndex = nValue;
}

int KWDGDPart::GetIndex() const
{
	return nIndex;
}
