// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseMemoryGuard.h"

longint KWDatabaseMemoryGuard::lCrashTestMaxSecondaryRecordNumber = 0;
longint KWDatabaseMemoryGuard::lCrashTestSingleInstanceMemoryLimit = 0;

KWDatabaseMemoryGuard::KWDatabaseMemoryGuard()
{
	Reset();
}

KWDatabaseMemoryGuard::~KWDatabaseMemoryGuard() {}

void KWDatabaseMemoryGuard::Reset()
{
	lMaxSecondaryRecordNumber = 0;
	lSingleInstanceMemoryLimit = 0;
	Init();
}

void KWDatabaseMemoryGuard::SetMaxSecondaryRecordNumber(longint lValue)
{
	require(lValue >= 0);
	lMaxSecondaryRecordNumber = lValue;
}

longint KWDatabaseMemoryGuard::GetMaxSecondaryRecordNumber() const
{
	return lMaxSecondaryRecordNumber;
}

void KWDatabaseMemoryGuard::SetSingleInstanceMemoryLimit(longint lValue)
{
	require(lValue >= 0);
	lSingleInstanceMemoryLimit = lValue;
}

longint KWDatabaseMemoryGuard::GetSingleInstanceMemoryLimit() const
{
	return lSingleInstanceMemoryLimit;
}

void KWDatabaseMemoryGuard::Init()
{
	// Reinitialisation des donnees de travail
	sMainObjectKey = "";
	bIsSingleInstanceMemoryLimitReached = false;
	lInitialHeapMemory = 0;
	lMaxHeapMemory = 0;
	lCurrentHeapMemory = 0;
	lReadSecondaryRecordNumberBeforeLimit = 0;
	lTotalReadSecondaryRecordNumber = 0;
	nComputedAttributeNumberBeforeLimit = 0;
	nTotalComputedAttributeNumber = 0;
	nMemoryCleaningNumber = 0;

	// Prise en compte de la taille de la heap initiale
	lInitialHeapMemory = MemGetHeapMemory();
	lCurrentHeapMemory = lInitialHeapMemory;

	// Calcul de la limite active la plus petite pour chaque contrainte
	lActualMaxSecondaryRecordNumber = lMaxSecondaryRecordNumber;
	if (lCrashTestMaxSecondaryRecordNumber > 0)
		lActualMaxSecondaryRecordNumber = lCrashTestMaxSecondaryRecordNumber;
	lActualSingleInstanceMemoryLimit = lSingleInstanceMemoryLimit;
	if (lCrashTestSingleInstanceMemoryLimit > 0)
		lActualSingleInstanceMemoryLimit = lCrashTestSingleInstanceMemoryLimit;

	// Initialisation de la limite memoire dure
	lMaxHeapMemory = lInitialHeapMemory + lActualSingleInstanceMemoryLimit;
}

void KWDatabaseMemoryGuard::SetMainObjectKey(const ALString& sValue)
{
	sMainObjectKey = sValue;
}

const ALString& KWDatabaseMemoryGuard::GetMainObjectKey() const
{
	return sMainObjectKey;
}

void KWDatabaseMemoryGuard::AddReadSecondaryRecord()
{
	// Memorisation des stats apres ajout de l'enregistrement
	lTotalReadSecondaryRecordNumber++;
	lCurrentHeapMemory = MemGetHeapMemory();

	// Detection du depassement de la limite si la contrainte est active
	if (not bIsSingleInstanceMemoryLimitReached and lActualSingleInstanceMemoryLimit > 0)
	{
		bIsSingleInstanceMemoryLimitReached = lCurrentHeapMemory > lMaxHeapMemory;

		// Memorisation du nombre d'enregistrement traites avant atteinte de la limitre
		if (not bIsSingleInstanceMemoryLimitReached)
			lReadSecondaryRecordNumberBeforeLimit++;
	}
}

void KWDatabaseMemoryGuard::AddComputedAttribute()
{
	// Memorisation des stats apres ajout d'un calcul d'attribut
	nTotalComputedAttributeNumber++;
	lCurrentHeapMemory = MemGetHeapMemory();

	// Detection du depassement de la limite si la contrainte est active
	if (not bIsSingleInstanceMemoryLimitReached and lActualSingleInstanceMemoryLimit > 0)
	{
		bIsSingleInstanceMemoryLimitReached = lCurrentHeapMemory > lMaxHeapMemory;

		// Memorisation du nombre d'enregistrement traites avant atteinte de la limitre
		if (not bIsSingleInstanceMemoryLimitReached)
			nComputedAttributeNumberBeforeLimit++;
	}
}

void KWDatabaseMemoryGuard::UpdateAfterMemoryCleaning()
{
	// Incrementation du nombre de nettoyages de la memoire
	nMemoryCleaningNumber++;

	// Memorisation de la memoire disponible
	lCurrentHeapMemory = MemGetHeapMemory();

	// Reactualisation de la detection du depassement de la limite si la contrainte est active
	if (lActualSingleInstanceMemoryLimit > 0)
		bIsSingleInstanceMemoryLimitReached = lCurrentHeapMemory > lMaxHeapMemory;
}

int KWDatabaseMemoryGuard::GetMemoryCleaningNumber() const
{
	return nMemoryCleaningNumber;
}

boolean KWDatabaseMemoryGuard::IsSingleInstanceVeryLarge() const
{
	return IsMaxSecondaryRecordNumberReached() or nMemoryCleaningNumber > 0;
}

const ALString KWDatabaseMemoryGuard::GetSingleInstanceVeryLargeLabel()
{
	ALString sLabel;

	require(IsSingleInstanceVeryLarge());
	require(not IsSingleInstanceMemoryLimitReached());

	// Information sur les enregistrements lus
	sLabel = sMemoryGuardLabelPrefix;
	if (sMainObjectKey != "")
		sLabel += sMainObjectKey + " ";
	sLabel += "containing ";
	sLabel += LongintToReadableString(GetTotalReadSecondaryRecordNumber());
	sLabel += " secondary records";

	// Et dans le cas ou la memoire a du etre nettoyee
	if (GetMemoryCleaningNumber() > 0)
		sLabel += " : all variables have been computed using RAM sparingly at the expense of computation time";
	ensure(sLabel.Find(sMemoryGuardLabelPrefix) == 0);
	return sLabel;
}

boolean KWDatabaseMemoryGuard::IsSingleInstanceMemoryLimitReachedDuringRead() const
{
	require(IsSingleInstanceMemoryLimitReached());
	return GetReadSecondaryRecordNumberBeforeLimit() < GetTotalReadSecondaryRecordNumber();
}

const ALString KWDatabaseMemoryGuard::GetSingleInstanceMemoryLimitLabel()
{
	ALString sLabel;

	require(IsSingleInstanceMemoryLimitReached());

	// Information sur les enregistrements lus
	sLabel = sMemoryGuardLabelPrefix;
	if (sMainObjectKey != "")
		sLabel += sMainObjectKey + " ";
	sLabel += "uses too much memory (more than ";
	sLabel += LongintToHumanReadableString(lActualSingleInstanceMemoryLimit);
	sLabel += " of RAM) after reading ";
	sLabel += LongintToReadableString(GetReadSecondaryRecordNumberBeforeLimit());
	sLabel += " secondary records";

	// Cas ou on n'a pas pu lire tous les enregistrements
	if (IsSingleInstanceMemoryLimitReachedDuringRead())
	{
		sLabel += " out of ";
		sLabel += LongintToReadableString(GetTotalReadSecondaryRecordNumber());
	}
	// Cas ou on n'a pas pu calculer tous les attributs derives
	else if (GetComputedAttributeNumberBeforeLimit() < GetTotalComputedAttributeNumber())
	{
		sLabel += " and calculating the values of ";
		sLabel += LongintToReadableString(GetComputedAttributeNumberBeforeLimit());
		sLabel += " variables out of at least ";
		sLabel += LongintToReadableString(GetTotalComputedAttributeNumber());
	}

	// Ajout d'une information sur la gestion de l'enregistrement en cas de probleme
	sLabel += sMemoryGuardRecoveryLabelSuffix;
	ensure(sLabel.Find(sMemoryGuardLabelPrefix) == 0 and sLabel.Find(sMemoryGuardRecoveryLabelSuffix) > 0);
	return sLabel;
}

void KWDatabaseMemoryGuard::InstallMemoryGuardErrorFlowIgnoreFunction()
{
	assert(Global::GetErrorFlowIgnoreFunction() == NULL or nMemoryGuardFunctionUseCount > 0);
	assert(Global::GetErrorFlowIgnoreFunction() == MemoryGuardErrorFlowIgnoreFunction or
	       nMemoryGuardFunctionUseCount == 0);

	// Parametrage de la fonction specifique du memory guard
	Global::SetErrorFlowIgnoreFunction(MemoryGuardErrorFlowIgnoreFunction);

	// Incrementation du compteur d'utilisation
	nMemoryGuardFunctionUseCount++;

	// On reinitialise les statistiques sur les messages specifiques
	// A noter ce code est heuristique: on garantit aunsi qu'un nombre minimum de message sera emis
	// pour la derniere database aouverte, mais les database ouverte precedemment vont egalement en beneficier
	// Ce probleme est sans gravite, etant donne la rarete supposee de ce type de situation
	nMemoryGuardInformationWarningNumber = 0;
	nMemoryGuardRecoveryWarningNumber = 0;
	ensure(nMemoryGuardFunctionUseCount > 0);
}

void KWDatabaseMemoryGuard::UninstallMemoryGuardErrorFlowIgnoreFunction()
{
	assert(Global::GetErrorFlowIgnoreFunction() == NULL or nMemoryGuardFunctionUseCount > 0);
	assert(Global::GetErrorFlowIgnoreFunction() == MemoryGuardErrorFlowIgnoreFunction or
	       nMemoryGuardFunctionUseCount == 0);

	// Decrementation du compteur d'utilisation
	nMemoryGuardFunctionUseCount--;

	// Remise a l'etat initial uniquement si on passe a 0
	if (nMemoryGuardFunctionUseCount == 0)
	{
		Global::SetErrorFlowIgnoreFunction(NULL);
		nMemoryGuardInformationWarningNumber = 0;
		nMemoryGuardRecoveryWarningNumber = 0;
	}
}

longint KWDatabaseMemoryGuard::GetReadSecondaryRecordNumberBeforeLimit() const
{
	return lReadSecondaryRecordNumberBeforeLimit;
}

longint KWDatabaseMemoryGuard::GetTotalReadSecondaryRecordNumber() const
{
	return lTotalReadSecondaryRecordNumber;
}

int KWDatabaseMemoryGuard::GetComputedAttributeNumberBeforeLimit() const
{
	return nComputedAttributeNumberBeforeLimit;
}

int KWDatabaseMemoryGuard::GetTotalComputedAttributeNumber() const
{
	return nTotalComputedAttributeNumber;
}

longint KWDatabaseMemoryGuard::GetInitialHeapMemory() const
{
	return lInitialHeapMemory;
}

longint KWDatabaseMemoryGuard::GetCurrentHeapMemory() const
{
	return lCurrentHeapMemory;
}

longint KWDatabaseMemoryGuard::GetCurrentUsedHeapMemory() const
{
	return lCurrentHeapMemory - lInitialHeapMemory;
}

void KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(longint lValue)
{
	require(lValue >= 0);
	lCrashTestMaxSecondaryRecordNumber = lValue;
}

longint KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber()
{
	return lCrashTestMaxSecondaryRecordNumber;
}

void KWDatabaseMemoryGuard::SetCrashTestSingleInstanceMemoryLimit(longint lValue)
{
	require(lValue >= 0);
	lCrashTestSingleInstanceMemoryLimit = lValue;
}

longint KWDatabaseMemoryGuard::GetCrashTestSingleInstanceMemoryLimit()
{
	return lCrashTestSingleInstanceMemoryLimit;
}

void KWDatabaseMemoryGuard::Write(ostream& ost) const
{
	ost << GetClassLabel() << "\n";
	cout << "\tMainObjectKey\t" << sMainObjectKey << "\n";
	cout << "\tMaxSecondaryRecordNumber\t" << lMaxSecondaryRecordNumber << "\n";
	cout << "\tSingleInstanceMemoryLimit\t" << lSingleInstanceMemoryLimit << "\n";
	cout << "\tIsSingleInstanceMemoryLimitReached\t" << bIsSingleInstanceMemoryLimitReached << "\n";
	cout << "\tInitialHeapMemory\t" << lInitialHeapMemory << "\n";
	cout << "\tMaxHeapMemory\t" << lMaxHeapMemory << "\n";
	cout << "\tCurrentHeapMemory\t" << lCurrentHeapMemory << "\n";
	cout << "\tReadSecondaryRecordNumberBeforeLimit\t" << lReadSecondaryRecordNumberBeforeLimit << "\n";
	cout << "\tTotalReadSecondaryRecordNumber\t" << lTotalReadSecondaryRecordNumber << "\n";
	cout << "\tComputedAttributeNumberBeforeLimit\t" << nComputedAttributeNumberBeforeLimit << "\n";
	cout << "\tTotalComputedAttributeNumber\t" << nTotalComputedAttributeNumber << "\n";
}

const ALString KWDatabaseMemoryGuard::GetClassLabel() const
{
	return "Database memory guard";
}

const ALString KWDatabaseMemoryGuard::GetObjectLabel() const
{
	return "";
}

boolean KWDatabaseMemoryGuard::MemoryGuardErrorFlowIgnoreFunction(const Error* e, boolean bDisplay)
{
	require(e != NULL);

	// Cas d'une erreur de type memory guard
	if (e->GetLabel().Find(sMemoryGuardLabelPrefix) == 0)
	{
		// On controle un flow d'erreur specifique aux warnings de type memory guard, uniquement en mode
		// affichage
		if (bDisplay)
		{
			if (e->GetLabel().Find(sMemoryGuardRecoveryLabelSuffix) > 0)
			{
				nMemoryGuardRecoveryWarningNumber++;
				if (Global::GetMaxErrorFlowNumber() > 0 and
				    nMemoryGuardRecoveryWarningNumber > Global::GetMaxErrorFlowNumber())
					return false;
				else
					return true;
			}
			else
			{
				nMemoryGuardInformationWarningNumber++;
				if (Global::GetMaxErrorFlowNumber() > 0 and
				    nMemoryGuardInformationWarningNumber > Global::GetMaxErrorFlowNumber())
					return false;
				else
					return true;
			}
		}
		// En mode test, on ignore toujours le controle de flow
		else
			return true;
	}
	// Cas d'une erreur standard
	else
		return false;
}

int KWDatabaseMemoryGuard::nMemoryGuardFunctionUseCount = 0;
int KWDatabaseMemoryGuard::nMemoryGuardInformationWarningNumber = 0;
int KWDatabaseMemoryGuard::nMemoryGuardRecoveryWarningNumber = 0;
const ALString KWDatabaseMemoryGuard::sMemoryGuardLabelPrefix = "Single instance ";
const ALString KWDatabaseMemoryGuard::sMemoryGuardRecoveryLabelSuffix =
    " : only the native variables are kept, the others are defined as missing values";
