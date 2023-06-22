// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWStructureRule.h"

///////////////////////////////////////////////////////////////
// Classe KWDRStructureRule

KWDRStructureRule::KWDRStructureRule()
{
	SetName("StructureRule");
	SetLabel("Structure rule");
	SetType(KWType::Structure);
	SetStructureName("StructureRule");
	bStructureInterface = false;
}

KWDRStructureRule::~KWDRStructureRule() {}

boolean KWDRStructureRule::GetStructureInterface() const
{
	return bStructureInterface;
}

KWDerivationRule* KWDRStructureRule::Create() const
{
	return new KWDRStructureRule;
}

Object* KWDRStructureRule::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

boolean KWDRStructureRule::CheckStructureDefinition() const
{
	return true;
}

void KWDRStructureRule::CompileStructure(KWClass* kwcOwnerClass) {}

void KWDRStructureRule::CopyStructureFrom(const KWDerivationRule* kwdrSource) {}

void KWDRStructureRule::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	bStructureInterface = true;
}

void KWDRStructureRule::CleanCompiledBaseInterface() {}

void KWDRStructureRule::WriteStructureUsedRule(ostream& ost) const
{
	require(bStructureInterface);
	KWDerivationRule::WriteUsedRule(ost);
}

void KWDRStructureRule::Compile(KWClass* kwcOwnerClass)
{
	require(kwcOwnerClass != NULL);
	require(CheckCompleteness(kwcOwnerClass));
	require(kwcOwnerClass->IsIndexed());

	// Compilation uniquement si necessaire
	if (kwcOwnerClass != kwcClass or not IsCompiled())
	{
		// Gestion de la classe
		kwcClass = kwcOwnerClass;
		nClassFreshness = kwcClass->GetFreshness();

		// Transfert si neccessaire des bornes de l'interface de base vers un vecteur
		if (not bStructureInterface)
		{
			// Transfert de la specification de base de la regle source
			// vers la specification de structure de la regle en cours
			BuildStructureFromBase(this);

			// Passage en mode interface de structure
			bStructureInterface = true;
		}

		// Compilation de l'interface de structure de la regle de derivation
		CompileStructure(kwcOwnerClass);

		// Nettoyage de l'interface de base une fois la regle compilee
		CleanCompiledBaseInterface();

		// Enregistrement de la fraicheur de compilation
		nCompileFreshness = nFreshness;
	}
	ensure(bStructureInterface);
	ensure(IsCompiled());
}

void KWDRStructureRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	// Copie de l'interface structure de la regle
	CopyStructureFrom(kwdrSource);
	bStructureInterface = cast(KWDRStructureRule*, kwdrSource)->bStructureInterface;

	// Copie standard (qui recopie entre autre les flags de fraicheur)
	KWDerivationRule::CopyFrom(kwdrSource);
}

boolean KWDRStructureRule::CheckDefinition() const
{
	boolean bOk = true;

	// Verification de base
	bOk = KWDerivationRule::CheckDefinition();

	// Verification des operandes si mode non structure
	if (bOk and GetOperandNumber() > 0)
	{
		// La regle ne doit pas etre a scope multiple
		if (bOk and GetMultipleScope())
		{
			AddError("Structure rule must not exploit operands with multiple scopes");
			bOk = false;
		}

		// On verifie que l'operande est de type simple
		if (bOk and not KWType::IsSimple(GetFirstOperand()->GetType()))
		{
			AddError("Structure rule must an operand with simple type");
			bOk = false;
		}

		// On verifie que l'operande est constant
		if (bOk and GetFirstOperand()->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
		{
			AddError("Structure rule must an operand with constant value");
			bOk = false;
		}
	}

	// Verification de structure
	if (bOk)
	{
		KWDRStructureRule* temporaryRule = NULL;
		const KWDRStructureRule* checkedRule;

		// Soit on est deja en interface de structure,
		// soit on utilise une regle tempooraire permettant d'effectuer les tests
		checkedRule = this;
		if (not bStructureInterface)
		{
			temporaryRule = cast(KWDRStructureRule*, Create());

			// Transfert de la specification de base de la regle source
			temporaryRule->BuildStructureFromBase(this);
			checkedRule = temporaryRule;
		}

		// Verification de l'interface de structure
		bOk = checkedRule->CheckStructureDefinition();

		// Nettoyage
		if (temporaryRule != NULL)
			delete temporaryRule;
	}
	return bOk;
}

int KWDRStructureRule::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, la comparaison doit etre reimplementee si on est en interface de structure
	assert(nDiff != 0 or not GetStructureInterface());
	return nDiff;
}

longint KWDRStructureRule::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRStructureRule) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

void KWDRStructureRule::Write(ostream& ost) const
{
	if (not bStructureInterface)
		KWDerivationRule::Write(ost);
	else
		WriteUsedRule(ost);
}

void KWDRStructureRule::WriteUsedRule(ostream& ost) const
{
	if (not bStructureInterface)
		KWDerivationRule::WriteUsedRule(ost);
	else
		WriteStructureUsedRule(ost);
}

boolean KWDRStructureRule::IsStructureRule() const
{
	return true;
}