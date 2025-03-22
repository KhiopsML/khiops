// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouper.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWGrouper

KWGrouper::KWGrouper()
{
	dParam = 0;
	nMinGroupFrequency = 0;
	nMaxGroupNumber = 0;
	bActivePreprocessing = false;
}

KWGrouper::~KWGrouper() {}

const ALString KWGrouper::GetName() const
{
	return "???";
}

boolean KWGrouper::IsMODLFamily() const
{
	return false;
}

double KWGrouper::GetParam() const
{
	return dParam;
}

void KWGrouper::SetParam(double dValue)
{
	require(dValue >= 0);
	dParam = dValue;
}

int KWGrouper::GetMinGroupFrequency() const
{
	return nMinGroupFrequency;
}

void KWGrouper::SetMinGroupFrequency(int nValue)
{
	require(nValue >= 0);
	nMinGroupFrequency = nValue;
}

int KWGrouper::GetMaxGroupNumber() const
{
	return nMaxGroupNumber;
}

void KWGrouper::SetMaxGroupNumber(int nValue)
{
	require(nValue >= 0);
	nMaxGroupNumber = nValue;
}

boolean KWGrouper::GetActivePreprocessing() const
{
	return bActivePreprocessing;
}

void KWGrouper::SetActivePreprocessing(boolean bValue)
{
	bActivePreprocessing = bValue;
}

KWGrouper* KWGrouper::Clone() const
{
	KWGrouper* kwgClone;

	kwgClone = Create();
	kwgClone->CopyFrom(this);
	return kwgClone;
}

void KWGrouper::CopyFrom(const KWGrouper* kwgSource)
{
	require(kwgSource != NULL);

	dParam = kwgSource->dParam;
	nMinGroupFrequency = kwgSource->nMinGroupFrequency;
	nMaxGroupNumber = kwgSource->nMaxGroupNumber;
}

void KWGrouper::Group(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const
{
	KWFrequencyTable* kwftPreprocessedSource;

	require(kwftSource != NULL);
	require(kwftSource->IsTableSortedBySourceFrequency(false));

	// Initialisation des resultats
	kwftPreprocessedSource = NULL;
	kwftTarget = NULL;
	ivGroups = NULL;

	// Preprocessing de la table
	// On desactive le preprocessing de la table en mode granularite supervise : la granularite et la poubelle
	// agissent automatiquement sur la taille min d'un groupe et le nombre max de groupes
	// CH V9 TODO : a la suppression du mode V8, seul le grouper KWGrouperBasicGrouping utilisera ce preprocessing.
	// On deplacera les methodes BuildPreprocessedTable et ComputePreprocessed<Min,Max>LineNumber dans
	// KWGrouperBasicGrouping On supprimera bActivePreprocessing
	if (GetActivePreprocessing())
		kwftPreprocessedSource = BuildPreprocessedTable(kwftSource);
	else
	{
		// Cas d'un groupage specifie par l'utilisateur a un seul groupe
		if (GetMaxGroupNumber() == 1)
			kwftPreprocessedSource = BuildReducedTable(kwftSource, 1);
	}

	if (kwftPreprocessedSource == NULL)
		kwftPreprocessedSource = kwftSource;

	// Groupage de la table preprocesse, avec cas particulier ou il n'y a pas de groupage a faire
	if (kwftPreprocessedSource->GetFrequencyVectorNumber() <= 1)
		KWGrouper::GroupPreprocessedTable(kwftPreprocessedSource, kwftTarget, ivGroups);
	// Cas general: on appele le groupage sur la table preprocesse
	else
	{
		GroupPreprocessedTable(kwftPreprocessedSource, kwftTarget, ivGroups);
	}

	assert(ivGroups->GetSize() <= kwftPreprocessedSource->GetFrequencyVectorNumber());

	// Nettoyage eventuel de la table preprocessee
	if (kwftPreprocessedSource != kwftSource)
		delete kwftPreprocessedSource;

	// Tri de la table par effectif total de la ligne puis par effectif de la premiere modalite
	kwftTarget->SortTableBySourceAndFirstModalityFrequency(ivGroups);

	// Verification de la coherence des resultats de groupage
	ensure(kwftTarget != NULL);
	ensure(ivGroups != NULL);
	ensure(kwftTarget->GetFrequencyVectorSize() == kwftSource->GetFrequencyVectorSize());
	ensure(kwftTarget->GetTotalFrequency() == kwftSource->GetTotalFrequency());
	ensure(kwftTarget->GetFrequencyVectorNumber() <= kwftSource->GetFrequencyVectorNumber());
	ensure(GetMaxGroupNumber() == 0 or kwftTarget->GetFrequencyVectorNumber() <= GetMaxGroupNumber());
	ensure(ivGroups->GetSize() <= kwftSource->GetTotalFrequency());
}

void KWGrouper::RegisterGrouper(int nTargetAttributeType, KWGrouper* grouper)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);
	require(grouper != NULL);
	require(grouper->GetName() != "");
	require(GetGroupers(nTargetAttributeType)->Lookup(grouper->GetName()) == NULL);

	// Memorisation du Grouper
	GetGroupers(nTargetAttributeType)->SetAt(grouper->GetName(), grouper);
}

KWGrouper* KWGrouper::LookupGrouper(int nTargetAttributeType, const ALString& sName)
{
	KWGrouper* grouper;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche du predicteur du bon type
	grouper = cast(KWGrouper*, GetGroupers(nTargetAttributeType)->Lookup(sName));
	return grouper;
}

KWGrouper* KWGrouper::CloneGrouper(int nTargetAttributeType, const ALString& sName)
{
	KWGrouper* referenceGrouper;

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche d'un Grouper de meme nom
	referenceGrouper = cast(KWGrouper*, GetGroupers(nTargetAttributeType)->Lookup(sName));

	// Retour de son Clone si possible
	if (referenceGrouper != NULL)
		return referenceGrouper->Clone();
	else
		return NULL;
}

void KWGrouper::ExportAllGroupers(int nTargetAttributeType, ObjectArray* oaGroupers)
{
	require(oaGroupers != NULL);

	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);

	// Recherche des predicteurs du bon type
	oaGroupers->RemoveAll();
	GetGroupers(nTargetAttributeType)->ExportObjectArray(oaGroupers);

	// Tri des predicteurs avant de retourner le tableau
	oaGroupers->SetCompareFunction(KWGrouperCompareName);
	oaGroupers->Sort();
}

void KWGrouper::RemoveAllGroupers()
{
	odSupervisedGroupers.RemoveAll();
	odUnsupervisedGroupers.RemoveAll();
}

void KWGrouper::DeleteAllGroupers()
{
	odSupervisedGroupers.DeleteAll();
	odUnsupervisedGroupers.DeleteAll();
}

////////////////////////////////////////////////////////////////////////////

void KWGrouper::GroupPreprocessedTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				       IntVector*& ivGroups) const
{
	int i;

	require(kwftSource != NULL);

	// Tout le groupage a ete effectue dans le preprocessing initial: on se contente
	// de dupliquer la table d'effectifs
	kwftTarget = kwftSource->Clone();

	// Creation du tableau d'index elementaire pour ce ce cas ou aucun groupage
	// effectif n'a ete realise
	ivGroups = new IntVector;
	ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
	for (i = 0; i < ivGroups->GetSize(); i++)
		ivGroups->SetAt(i, i);
}

boolean KWGrouper::Check() const
{
	return false;
}

const ALString KWGrouper::GetClassLabel() const
{
	return "Value grouping";
}

const ALString KWGrouper::GetObjectLabel() const
{
	if (GetParam() > 0)
		return GetName() + "(" + IntToString(GetMinGroupFrequency()) + ", " + IntToString(GetMaxGroupNumber()) +
		       ", " + DoubleToString(GetParam()) + ")";
	else if (GetMinGroupFrequency() > 0 or GetMaxGroupNumber() > 0)
		return GetName() + "(" + IntToString(GetMinGroupFrequency()) + ", " + IntToString(GetMaxGroupNumber()) +
		       ")";
	else
		return GetName();
}

ObjectDictionary* KWGrouper::GetGroupers(int nTargetAttributeType)
{
	require(nTargetAttributeType == KWType::Symbol or nTargetAttributeType == KWType::None);
	if (nTargetAttributeType == KWType::Symbol)
		return &odSupervisedGroupers;
	else
		return &odUnsupervisedGroupers;
}

KWFrequencyTable* KWGrouper::BuildPreprocessedTable(KWFrequencyTable* table) const
{
	KWFrequencyTable* preprocessedTable;
	int nPreprocessedMinLineFrequency;
	int nPreprocessedMaxLineNumber;
	int nPreprocessedLineNumber;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));

	// Calcul du nombre de lignes de la table preprocessee
	nPreprocessedMinLineFrequency = ComputePreprocessedMinLineFrequency(table);
	nPreprocessedMaxLineNumber = ComputePreprocessedMaxLineNumber(table);
	nPreprocessedLineNumber =
	    ComputeTableReducedLineNumber(table, nPreprocessedMaxLineNumber, nPreprocessedMinLineFrequency);
	assert(nPreprocessedLineNumber <= table->GetFrequencyVectorNumber());

	// Construction de la table preprocesse si necessaire
	preprocessedTable = NULL;
	if (nPreprocessedLineNumber < table->GetFrequencyVectorNumber())
		preprocessedTable = BuildReducedTable(table, nPreprocessedLineNumber);

	return preprocessedTable;
}

int KWGrouper::ComputePreprocessedMinLineFrequency(KWFrequencyTable* table) const
{
	int nMinLineFrequency;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));

	// Par defaut, on impose un effectif minimum de 2
	// Attention, il s'agit d'un patch pour contourner le probleme de sur-apprentissage identifie chez AMEX
	// La correction propre sera prise en compte par le refonte des prior (groupe fourre-tout et groupe poubelle)
	nMinLineFrequency = GetMinGroupFrequency();
	if (nMinLineFrequency == 0)
		nMinLineFrequency = 2;
	return nMinLineFrequency;
}

int KWGrouper::ComputePreprocessedMaxLineNumber(KWFrequencyTable* table) const
{
	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));

	if (GetMaxGroupNumber() == 1)
		return 1;
	else
		return table->GetFrequencyVectorNumber();
}

int KWGrouper::ComputeTableReducedLineNumber(KWFrequencyTable* table, int nMaxLineNumber, int nMinLineFrequency) const
{
	int nSource;
	int nActualMaxLineNumber;
	int nReducedLineNumber;
	int nLastLineFrequency;
	int nLineFrequency;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));
	require(nMaxLineNumber >= 1 or (table->GetFrequencyVectorNumber() == 0 and nMaxLineNumber >= 0));
	require(nMinLineFrequency >= 0);

	// Initialisation du nombre de ligne reduit en partant du nombre max
	nActualMaxLineNumber = nMaxLineNumber;
	if (nActualMaxLineNumber > table->GetFrequencyVectorNumber())
		nActualMaxLineNumber = table->GetFrequencyVectorNumber();

	// Parcours des lignes depuis la fin pour elaguer les lignes en sous-effectif
	nReducedLineNumber = nActualMaxLineNumber;
	if (table->GetFrequencyVectorNumber() > 0 and
	    (nMinLineFrequency > 1 or nActualMaxLineNumber < table->GetFrequencyVectorNumber()))
	{
		// Parcours de la table pour elaguer les lignes en sous-effectif
		nLastLineFrequency = table->GetTotalFrequency();
		for (nSource = 0; nSource < nActualMaxLineNumber; nSource++)
		{
			nLineFrequency = table->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();
			nLastLineFrequency -= nLineFrequency;

			// Arret si l'on rencontre une ligne d'effectif insuffisant
			if (nLineFrequency < nMinLineFrequency)
			{
				// Si la derniere ligne cumulee  avec la ligne courante a un
				// effectif insuffisant, il faut la fusionner avec la precedente
				if (nLastLineFrequency + nLineFrequency < nMinLineFrequency)
					nReducedLineNumber = nSource;
				else
					nReducedLineNumber = nSource + 1;
				break;
			}
		}

		// On doit laisser au moins une ligne
		if (nReducedLineNumber <= 0)
			nReducedLineNumber = 1;
	}

	ensure(nReducedLineNumber <= table->GetFrequencyVectorNumber());
	ensure(nReducedLineNumber >= 1 or (table->GetFrequencyVectorNumber() == 0 and nReducedLineNumber == 0));
	return nReducedLineNumber;
}

KWFrequencyTable* KWGrouper::BuildReducedTable(KWFrequencyTable* table, int nNewLineNumber) const
{
	KWFrequencyTable* reducedTable;
	int nSource;
	int nTarget;
	KWDenseFrequencyVector* kwdfvReducedFrequencyVector;
	KWDenseFrequencyVector* kwdfvSourceFrequencyVector;
	IntVector* ivReducedFrequencies;
	IntVector* ivSourceFrequencies;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));
	require(nNewLineNumber <= table->GetFrequencyVectorNumber());
	require(nNewLineNumber >= 1 or (table->GetFrequencyVectorNumber() == 0 and nNewLineNumber == 0));

	// Creation de la table
	reducedTable = new KWFrequencyTable;
	reducedTable->SetFrequencyVectorCreator(table->GetFrequencyVectorCreator()->Clone());
	reducedTable->SetFrequencyVectorNumber(nNewLineNumber);

	// Initialisation par defaut des nombres de valeurs
	reducedTable->SetInitialValueNumber(table->GetInitialValueNumber());
	reducedTable->SetGranularizedValueNumber(table->GetGranularizedValueNumber());
	if (nNewLineNumber < table->GetGranularizedValueNumber())
	{
		reducedTable->SetGranularizedValueNumber(nNewLineNumber);
	}

	// Initialisation du debut de la table (tout sauf la derniere ligne)
	for (nSource = 0; nSource < nNewLineNumber - 1; nSource++)
	{
		// Acces au vecteur de la table source (sense etre en representation dense)
		kwdfvSourceFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(nSource));
		ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();

		// Acces au vecteur de la table reduite (sense etre en representation dense)
		kwdfvReducedFrequencyVector =
		    cast(KWDenseFrequencyVector*, reducedTable->GetFrequencyVectorAt(nSource));
		ivReducedFrequencies = kwdfvReducedFrequencyVector->GetFrequencyVector();

		// Recopie
		ivReducedFrequencies->CopyFrom(ivSourceFrequencies);
	}

	// Initialisation du dernier vecteur de la table (tout sauf la derniere ligne)
	// Acces au dernier vecteur de la table reduite (sense etre en representation dense)
	kwdfvReducedFrequencyVector = cast(KWDenseFrequencyVector*, reducedTable->GetFrequencyVectorAt(nSource));
	ivReducedFrequencies = kwdfvReducedFrequencyVector->GetFrequencyVector();
	ivReducedFrequencies->SetSize(table->GetFrequencyVectorSize());

	for (nSource = nNewLineNumber - 1; nSource < table->GetFrequencyVectorNumber(); nSource++)
	{
		// Acces au vecteur de la table source (sense etre en representation dense)
		kwdfvSourceFrequencyVector = cast(KWDenseFrequencyVector*, table->GetFrequencyVectorAt(nSource));
		ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();

		// Recopie
		for (nTarget = 0; nTarget < reducedTable->GetFrequencyVectorSize(); nTarget++)
		{
			ivReducedFrequencies->UpgradeAt(nTarget, ivSourceFrequencies->GetAt(nTarget));
		}
	}

	ensure(reducedTable->GetTotalFrequency() == table->GetTotalFrequency());
	return reducedTable;
}

ObjectDictionary KWGrouper::odSupervisedGroupers;
ObjectDictionary KWGrouper::odUnsupervisedGroupers;

int KWGrouperCompareName(const void* first, const void* second)
{
	KWGrouper* aFirst;
	KWGrouper* aSecond;
	int nResult;

	aFirst = cast(KWGrouper*, *(Object**)first);
	aSecond = cast(KWGrouper*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWGrouperBasicGrouping

KWGrouperBasicGrouping::KWGrouperBasicGrouping()
{
	// Activation du pretraitement effectue dans la methode Group pour ce grouper qui n'utilise pas le mode
	// granularite/poubelle
	bActivePreprocessing = true;
}

KWGrouperBasicGrouping::~KWGrouperBasicGrouping() {}

const ALString KWGrouperBasicGrouping::GetName() const
{
	return "BasicGrouping";
}

KWGrouper* KWGrouperBasicGrouping::Create() const
{
	return new KWGrouperBasicGrouping;
}

int KWGrouperBasicGrouping::ComputePreprocessedMaxLineNumber(KWFrequencyTable* table) const
{
	const int nDefaultMaxGroupNumber = 10;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));

	if (GetMaxGroupNumber() == 0)
		return nDefaultMaxGroupNumber;
	else
		return GetMaxGroupNumber();
}

boolean KWGrouperBasicGrouping::Check() const
{
	boolean bOk;
	bOk = dParam == 0;
	if (not bOk)
		AddError("The main parameter of the algorithm must be 0");
	return bOk;
}

const ALString KWGrouperBasicGrouping::GetObjectLabel() const
{
	if (GetMinGroupFrequency() == 0 and GetMaxGroupNumber() == 0)
		return GetName();
	else
		return GetName() + "(" + IntToString(GetMinGroupFrequency()) + ", " + IntToString(GetMaxGroupNumber()) +
		       ")";
}
