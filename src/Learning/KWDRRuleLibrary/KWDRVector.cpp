// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRVector.h"

void KWDRRegisterVectorRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolVector);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousVector);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolValueAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousValueAt);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRAsSymbolVector);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsContinuousVector);
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolVector

KWDRSymbolVector::KWDRSymbolVector()
{
	SetName("VectorC");
	SetLabel("Categorical vector");
	SetStructureName("VectorC");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// On n'impose pas l'origine de l'operande a etre constant, car ce n'est pas obligatoire
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRSymbolVector::~KWDRSymbolVector() {}

boolean KWDRSymbolVector::AreConstantOperandsMandatory() const
{
	return false;
}

KWDerivationRule* KWDRSymbolVector::Create() const
{
	return new KWDRSymbolVector;
}

void KWDRSymbolVector::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

void KWDRSymbolVector::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRSymbolVector* kwdrsvSource = cast(const KWDRSymbolVector*, kwdrSource);

	// Copie de la version optimisee du parametrage des valeurs
	svValues.CopyFrom(&kwdrsvSource->svValues);
}

void KWDRSymbolVector::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	debug(const KWDRSymbolVector* kwdrsvSource = cast(const KWDRSymbolVector*, kwdrSource));
	int i;
	KWDerivationRuleOperand* operand;
	Symbol sValue;

	require(kwdrSource->KWDerivationRule::CheckDefinition());
	debug(require(kwdrsvSource->CheckConstantOperands(false)));

	// Recopie des operandes
	svValues.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		sValue = operand->GetSymbolConstant();
		svValues.SetAt(i, sValue);
	}
	bStructureInterface = true;
}

void KWDRSymbolVector::WriteStructureUsedRule(ostream& ost) const
{
	int i;

	require(CheckConstantOperands(false));

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < svValues.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << KWClass::GetExternalSymbolConstant(svValues.GetAt(i));
	}
	ost << ")";
}

int KWDRSymbolVector::FullCompareStructure(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRSymbolVector* ruleSymbolVector;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleSymbolVector = cast(KWDRSymbolVector*, rule);
		assert(GetStructureInterface());
		assert(ruleSymbolVector->GetStructureInterface());

		// Taille du vecteur
		nDiff = svValues.GetSize() - ruleSymbolVector->svValues.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < svValues.GetSize(); i++)
			{
				nDiff = svValues.GetAt(i).CompareValue(ruleSymbolVector->svValues.GetAt(i));
				if (nDiff != 0)
					break;
			}
		}
	}
	return nDiff;
}

Object* KWDRSymbolVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	Symbol sValue;
	int i;

	require(IsCompiled());

	// Cas du mode non constant : initialisation du vecteur de valeurs a partir des operandes
	if (oaOperands.GetSize() > 0)
	{
		assert(not CheckConstantOperands(false));
		svValues.SetSize(GetOperandNumber());
		for (i = 0; i < GetOperandNumber(); i++)
		{
			valueOperand = GetOperandAt(i);
			sValue = valueOperand->GetSymbolValue(kwoObject);
			svValues.SetAt(i, sValue);
		}
		bStructureInterface = true;
	}
	return (Object*)this;
}

longint KWDRSymbolVector::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDRStructureRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRSymbolVector) - sizeof(KWDRStructureRule);
	lUsedMemory += svValues.GetUsedMemory();

	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousVector

KWDRContinuousVector::KWDRContinuousVector()
{
	SetName("Vector");
	SetLabel("Numerical vector");
	SetStructureName("Vector");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// On n'impose pas l'origine de l'operande a etre constant, car ce n'est pas obligatoire
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRContinuousVector::~KWDRContinuousVector() {}

boolean KWDRContinuousVector::AreConstantOperandsMandatory() const
{
	return false;
}

KWDerivationRule* KWDRContinuousVector::Create() const
{
	return new KWDRContinuousVector;
}

void KWDRContinuousVector::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

void KWDRContinuousVector::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRContinuousVector* kwdrsvSource = cast(const KWDRContinuousVector*, kwdrSource);

	// Copie de la version optimisee du parametrage des valeurs
	cvValues.CopyFrom(&kwdrsvSource->cvValues);
}

void KWDRContinuousVector::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	debug(const KWDRContinuousVector* kwdrcvSource = cast(const KWDRContinuousVector*, kwdrSource));
	int i;
	KWDerivationRuleOperand* operand;
	Continuous cValue;

	require(kwdrSource->KWDerivationRule::CheckDefinition());
	debug(require(kwdrcvSource->CheckConstantOperands(false)));

	// Recopie des operandes
	cvValues.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		cValue = operand->GetContinuousConstant();
		cvValues.SetAt(i, cValue);
	}
	bStructureInterface = true;
}

void KWDRContinuousVector::WriteStructureUsedRule(ostream& ost) const
{
	int i;

	require(CheckConstantOperands(false));

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < cvValues.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << KWClass::GetExternalContinuousConstant(cvValues.GetAt(i));
	}
	ost << ")";
}

int KWDRContinuousVector::FullCompareStructure(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRContinuousVector* ruleContinuousVector;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleContinuousVector = cast(KWDRContinuousVector*, rule);
		assert(GetStructureInterface());
		assert(ruleContinuousVector->GetStructureInterface());

		// Taille du vecteur
		nDiff = cvValues.GetSize() - ruleContinuousVector->cvValues.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < cvValues.GetSize(); i++)
			{
				nDiff =
				    KWContinuous::Compare(cvValues.GetAt(i), ruleContinuousVector->cvValues.GetAt(i));
				if (nDiff != 0)
					break;
			}
		}
	}
	return nDiff;
}

Object* KWDRContinuousVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	Continuous cValue;
	int i;

	require(IsCompiled());

	// Cas du mode non constant : initialisation du vecteur de valeurs a partir des operandes
	if (oaOperands.GetSize() > 0)
	{
		assert(not CheckConstantOperands(false));
		cvValues.SetSize(GetOperandNumber());
		for (i = 0; i < GetOperandNumber(); i++)
		{
			valueOperand = GetOperandAt(i);
			cValue = valueOperand->GetContinuousValue(kwoObject);
			cvValues.SetAt(i, cValue);
		}
		bStructureInterface = true;
	}
	return (Object*)this;
}

longint KWDRContinuousVector::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRStructureRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRContinuousVector) - sizeof(KWDRStructureRule);
	lUsedMemory += cvValues.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueAt

KWDRSymbolValueAt::KWDRSymbolValueAt()
{
	SetName("ValueAtC");
	SetLabel("Categorical value at index");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("VectorC");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRSymbolValueAt::~KWDRSymbolValueAt() {}

KWDerivationRule* KWDRSymbolValueAt::Create() const
{
	return new KWDRSymbolValueAt;
}

Symbol KWDRSymbolValueAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRSymbolVector* symbolVector;
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	KWDerivationRuleOperand* firstOperand;
	firstOperand = GetFirstOperand();

	// Recherche de la partition
	symbolVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Recherche de l'index
	nIndex = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

	// Renvoie de la valeur si index valide
	if (1 <= nIndex and nIndex <= symbolVector->GetValueNumber())
		return symbolVector->GetValueAt(nIndex - 1);
	// Sinon, renvoie de la valeur vide
	else
		return Symbol();
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueAt

KWDRContinuousValueAt::KWDRContinuousValueAt()
{
	SetName("ValueAt");
	SetLabel("Numerical value at index");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Vector");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRContinuousValueAt::~KWDRContinuousValueAt() {}

KWDerivationRule* KWDRContinuousValueAt::Create() const
{
	return new KWDRContinuousValueAt;
}

Continuous KWDRContinuousValueAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRContinuousVector* continuousVector;
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	// Recherche de la partition
	continuousVector = cast(KWDRContinuousVector*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Recherche de l'index
	nIndex = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

	// Renvoie de la valeur si index valide
	if (1 <= nIndex and nIndex <= continuousVector->GetValueNumber())
		return continuousVector->GetValueAt(nIndex - 1);
	// Sinon, renvoie de la valeur manquante
	else
		return KWContinuous::GetMissingValue();
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsSymbolVector

KWDRAsSymbolVector::KWDRAsSymbolVector()
{
	SetName("AsVectorC");
	SetLabel("As categorical vector");
	SetType(KWType::Structure);
	SetStructureName("VectorC");
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRAsSymbolVector::~KWDRAsSymbolVector() {}

KWDerivationRule* KWDRAsSymbolVector::Create() const
{
	return new KWDRAsSymbolVector;
}

Object* KWDRAsSymbolVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	Symbol sSymbolTokens;
	const char* sStringTokens;
	char c;
	int nTokenNumber;
	int i;
	boolean bInToken;
	ALString sToken;
	int nTokenStart;

	require(IsCompiled());

	// Recherche du parametre
	sSymbolTokens = GetFirstOperand()->GetSymbolValue(kwoObject);

	// On passe par les methodes de type chaine de caracteres pour des raisons d'efficacite
	sStringTokens = sSymbolTokens.GetValue();

	// Premiere passe pour determiner le nombre de tokens
	nTokenNumber = 0;
	bInToken = false;
	i = 0;
	c = sStringTokens[i];
	while (c != '\0')
	{
		// Test si on est dans un token
		if (c != ' ')
		{
			// Incrementation du nombre de tokens si on vient d'une zone delimiteur
			if (not bInToken)
				nTokenNumber++;
			bInToken = true;
		}
		// Sinon, on est dans un delimiteur
		else
			bInToken = false;

		// Caractere suivant
		i++;
		c = sStringTokens[i];
	}

	// Parametrage du nombre de valeurs du resultats
	symbolVector.SetValueNumber(nTokenNumber);

	// Deuxieme passe pour rechercher les tokens
	nTokenNumber = 0;
	bInToken = false;
	i = 0;
	c = sStringTokens[i];
	nTokenStart = -1;
	while (c != '\0')
	{
		// Test si on est dans un token
		if (c != ' ')
		{
			// Nouveau token si on vient d'une zone delimiteur
			if (not bInToken)
			{
				nTokenStart = i;
				nTokenNumber++;
			}
			bInToken = true;
		}
		// Sinon, on est dans un delimiteur
		else
		{
			// Fin de token si on vient d'une zone token
			if (bInToken)
			{
				// Construction et memorisation du token
				sToken = ALString(sStringTokens + nTokenStart, i - nTokenStart);
				symbolVector.SetValueAt(nTokenNumber - 1, (Symbol)sToken);
			}
			bInToken = false;
		}

		// Caractere suivant
		i++;
		c = sStringTokens[i];

		// Construction et memorisation du token en fin de chaine si necessaire
		if (c == '\0' and bInToken)
		{
			assert(nTokenStart >= 0);
			sToken = ALString(sStringTokens + nTokenStart, i - nTokenStart);
			symbolVector.SetValueAt(nTokenNumber - 1, (Symbol)sToken);
		}
	}
	return &symbolVector;
}

longint KWDRAsSymbolVector::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRAsSymbolVector) - sizeof(KWDerivationRule);
	lUsedMemory += symbolVector.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsContinuousVector

KWDRAsContinuousVector::KWDRAsContinuousVector()
{
	SetName("AsVector");
	SetLabel("As numerical vector");
	SetType(KWType::Structure);
	SetStructureName("Vector");
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRAsContinuousVector::~KWDRAsContinuousVector() {}

KWDerivationRule* KWDRAsContinuousVector::Create() const
{
	return new KWDRAsContinuousVector;
}

Object* KWDRAsContinuousVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	Symbol sSymbolTokens;
	const char* sStringTokens;
	char c;
	int nTokenNumber;
	int i;
	boolean bInToken;
	ALString sToken;
	int nTokenStart;

	require(IsCompiled());

	// Recherche du parametre
	sSymbolTokens = GetFirstOperand()->GetSymbolValue(kwoObject);

	// On passe par les methodes de type chaine de caracteres pour des raisons d'efficacite
	sStringTokens = sSymbolTokens.GetValue();

	// Premiere passe pour determiner le nombre de tokens
	nTokenNumber = 0;
	bInToken = false;
	i = 0;
	c = sStringTokens[i];
	while (c != '\0')
	{
		// Test si on est dans un token
		if (c != ' ')
		{
			// Incrementation du nombre de tokens si on vient d'une zone delimiteur
			if (not bInToken)
				nTokenNumber++;
			bInToken = true;
		}
		// Sinon, on est dans un delimiteur
		else
			bInToken = false;

		// Caractere suivant
		i++;
		c = sStringTokens[i];
	}

	// Parametrage du nombre de valeurs du resultats
	continuousVector.SetValueNumber(nTokenNumber);

	// Deuxieme passe pour rechercher les tokens
	nTokenNumber = 0;
	bInToken = false;
	i = 0;
	c = sStringTokens[i];
	nTokenStart = -1;
	while (c != '\0')
	{
		// Test si on est dans un token
		if (c != ' ')
		{
			// Nouveau token si on vient d'une zone delimiteur
			if (not bInToken)
			{
				nTokenStart = i;
				nTokenNumber++;
			}
			bInToken = true;
		}
		// Sinon, on est dans un delimiteur
		else
		{
			// Fin de token si on vient d'une zone token
			if (bInToken)
			{
				// Construction et memorisation du token
				sToken = ALString(sStringTokens + nTokenStart, i - nTokenStart);
				continuousVector.SetValueAt(nTokenNumber - 1, KWContinuous::StringToContinuous(sToken));
			}
			bInToken = false;
		}

		// Caractere suivant
		i++;
		c = sStringTokens[i];

		// Construction et memorisation du token en fin de chaine si necessaire
		if (c == '\0' and bInToken)
		{
			assert(nTokenStart >= 0);
			sToken = ALString(sStringTokens + nTokenStart, i - nTokenStart);
			continuousVector.SetValueAt(nTokenNumber - 1, KWContinuous::StringToContinuous(sToken));
		}
	}
	return &continuousVector;
}

longint KWDRAsContinuousVector::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRAsContinuousVector) - sizeof(KWDerivationRule);
	lUsedMemory += continuousVector.GetUsedMemory();
	return lUsedMemory;
}
