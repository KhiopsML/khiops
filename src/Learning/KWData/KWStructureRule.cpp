// Copyright (c) 2023-2026 Orange. All rights reserved.
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

boolean KWDRStructureRule::AreConstantOperandsMandatory() const
{
	return true;
}

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
	require(AreConstantOperandsMandatory());

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

int KWDRStructureRule::FullCompareStructure(const KWDerivationRule* rule) const
{
	return true;
}

void KWDRStructureRule::Compile(KWClass* kwcOwnerClass)
{
	require(kwcOwnerClass != NULL);
	require(CheckCompleteness(kwcOwnerClass));
	require(kwcOwnerClass->IsIndexed());

	// Passage a une interface de type structure dans le cas constant
	if (AreConstantOperandsMandatory() or CheckConstantOperands(false))
	{
		// Compilation uniquement si necessaire
		if (kwcOwnerClass != kwcClass or not IsCompiled())
		{
			// Gestion de la classe
			kwcClass = kwcOwnerClass;
			nClassFreshness = kwcClass->GetFreshness();

			// Transfert si neccessaire des operandes vers une interface de type structure
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
	}
	// Sinon, compilation standard
	else
		KWDerivationRule::Compile(kwcOwnerClass);
	ensure(bStructureInterface or oaOperands.GetSize() > 0);
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

	// On ne peut avoir a la fois des operandes et une interface de type structure,
	// sauf dans le cas ou les operandes ne sont pas obligatoirement constants
	require(not(GetOperandNumber() > 0 and GetStructureInterface()) or not AreConstantOperandsMandatory());

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
		if (bOk and AreConstantOperandsMandatory() and
		    GetFirstOperand()->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
		{
			AddError("Structure rule must an operand with constant value");
			bOk = false;
		}
	}

	// Verification de structure
	if (bOk and CheckConstantOperands(false))
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

boolean KWDRStructureRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsFamily(ruleFamily);

	// Verification que les operandes sont constantes
	if (bOk and AreConstantOperandsMandatory())
		bOk = CheckConstantOperands(true);
	return bOk;
}

boolean KWDRStructureRule::CheckConstantOperands(boolean bVerbose) const
{
	boolean bOk = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	// Verification des operandes
	for (i = 0; i < oaOperands.GetSize(); i++)
	{
		operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

		// Verification de l'origine constante uniquement dans cas des type simples
		if (KWType::IsSimple(operand->GetType()) and
		    operand->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
		{
			if (bVerbose)
				AddError(sTmp + "Operand " + IntToString(i + 1) + " (" + operand->GetObjectLabel() +
					 ") with origin " +
					 operand->OriginToString(operand->GetOrigin(), operand->GetType()) +
					 " should be constant");
			bOk = false;
			break;
		}
	}
	return bOk;
}

int KWDRStructureRule::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;

	require(rule != NULL);

	// Redirection vers l'interface de structure si les operandes ont ete detruits
	if (GetOperandNumber() == 0 and rule->GetOperandNumber() == 0)
		nDiff = FullCompareStructure(rule);
	// Sinon, on appelle la methode standard
	else
		nDiff = KWDerivationRule::FullCompare(rule);
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
	// Redirection vers l'interface de structure si les operande ont ete detruits
	if (GetOperandNumber() == 0)
		WriteStructureUsedRule(ost);
	else
		KWDerivationRule::Write(ost);
}

void KWDRStructureRule::WriteUsedRule(ostream& ost) const
{
	// Redirection vers l'interface de structure si les operandes ont ete detruits
	if (GetOperandNumber() == 0)
		WriteStructureUsedRule(ost);
	else
		KWDerivationRule::WriteUsedRule(ost);
}

boolean KWDRStructureRule::IsStructureRule() const
{
	return true;
}
