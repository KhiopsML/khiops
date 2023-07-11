// Copyright (c) 2023 Orange. All rights reserved.
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
	KWDerivationRule::RegisterDerivationRule(new KWDRExtractWords);
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
	GetFirstOperand()->SetType(KWType::Symbol);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRSymbolVector::~KWDRSymbolVector() {}

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
	const KWDRSymbolVector* kwdrsvSource = cast(KWDRSymbolVector*, kwdrSource);

	// Copie de la version optimisee du parametrage des valeurs
	svValues.CopyFrom(&kwdrsvSource->svValues);
}

void KWDRSymbolVector::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;
	Symbol sValue;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

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

int KWDRSymbolVector::FullCompare(const KWDerivationRule* rule) const
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
	GetFirstOperand()->SetType(KWType::Continuous);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRContinuousVector::~KWDRContinuousVector() {}

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
	const KWDRContinuousVector* kwdrsvSource = cast(KWDRContinuousVector*, kwdrSource);

	// Copie de la version optimisee du parametrage des valeurs
	cvValues.CopyFrom(&kwdrsvSource->cvValues);
}

void KWDRContinuousVector::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;
	Continuous cValue;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

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

int KWDRContinuousVector::FullCompare(const KWDerivationRule* rule) const
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

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExtractWords

KWDRExtractWords::KWDRExtractWords()
{
	int nParameter;

	SetName("ExtractWords");
	SetLabel("Extact words");
	SetType(KWType::Structure);
	SetStructureName("VectorC");
	SetOperandNumber(6);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
	GetOperandAt(3)->SetType(KWType::Symbol);
	GetOperandAt(4)->SetType(KWType::Symbol);
	GetOperandAt(5)->SetType(KWType::Continuous);

	// Les parametres d'extraction sont constants
	for (nParameter = 1; nParameter < GetOperandNumber(); nParameter++)
		GetOperandAt(nParameter)->SetOrigin(KWDerivationRuleOperand::OriginConstant);

	// Initialisation de valeurs par defaut valides et coherentes
	GetOperandAt(1)->SetContinuousConstant(1);
	GetOperandAt(2)->SetContinuousConstant(0);
	GetOperandAt(5)->SetContinuousConstant(10);

	// Initialisation des parametres d'optimisation
	nMaxLength = 0;
}

KWDRExtractWords::~KWDRExtractWords() {}

KWDerivationRule* KWDRExtractWords::Create() const
{
	return new KWDRExtractWords;
}

Object* KWDRExtractWords::ComputeStructureResult(const KWObject* kwoObject) const
{
	Symbol sInputString;
	ALString sTranslatedString;
	const char* sStringTokens;
	char c;
	int nTokenNumber;
	int i;
	unsigned char nChar;
	boolean bInToken;
	ALString sToken;
	int nTokenStart;
	int nTokenLength;

	require(IsCompiled());

	// Recherche du de la chaine de caractere en premier parametre
	sInputString = GetFirstOperand()->GetSymbolValue(kwoObject);

	// Recodage des caracteres de la chaines selon les parametres d'extractions
	// On compte les token en meme temps
	sTranslatedString = sInputString.GetValue();
	nTokenNumber = 0;
	bInToken = false;
	for (i = 0; i < sTranslatedString.GetLength(); i++)
	{
		// Transcodage du caractere
		nChar = sTranslatedString.GetAt(i);
		c = sTranslatedChars.GetAt(nChar);
		sTranslatedString.SetAt(i, c);

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
	}

	// On passe par les methodes de type chaine de caracteres pour des raisons d'efficacite
	sStringTokens = sTranslatedString.GetBuffer(0);

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
				// Construction et memorisation du token, en tenant compte de la longueur max
				nTokenLength = i - nTokenStart;
				if (nTokenLength > nMaxLength)
					nTokenLength = nMaxLength;
				sToken = ALString(sStringTokens + nTokenStart, nTokenLength);
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
			nTokenLength = i - nTokenStart;
			if (nTokenLength > nMaxLength)
				nTokenLength = nMaxLength;
			sToken = ALString(sStringTokens + nTokenStart, nTokenLength);
			symbolVector.SetValueAt(nTokenNumber - 1, (Symbol)sToken);
		}
	}
	return &symbolVector;
}

boolean KWDRExtractWords::CheckOperandsDefinition() const
{
	boolean bOk;
	KWDerivationRuleOperand* operand;
	Continuous cValue;
	int nValue;
	Symbol sValue;
	IntVector ivAdditionnalChars;
	int nAdditionnalCharNumber;
	int i;
	unsigned char nChar;
	boolean bIsInt;
	ALString sTmp;

	// Verification de base
	bOk = KWDerivationRule::CheckOperandsDefinition();

	// Verification des parametres d'extraction
	if (bOk)
	{
		// Verification du parametre ToLower (booleen)
		operand = GetOperandAt(1);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
		cValue = operand->GetContinuousConstant();
		bIsInt = KWContinuous::ContinuousToInt(cValue, nValue);
		if (not bIsInt or (nValue != 0 and nValue != 1))
		{
			bOk = false;
			AddError(sTmp + "ToLower parameter (operand 2) " + "(" +
				 KWContinuous::ContinuousToString(cValue) + ")" + ": operand must be equal to 0 or 1");
		}

		// Verification du parametre KeepNumerical (booleen)
		operand = GetOperandAt(2);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
		cValue = operand->GetContinuousConstant();
		bIsInt = KWContinuous::ContinuousToInt(cValue, nValue);
		if (not bIsInt or (nValue != 0 and nValue != 1))
		{
			bOk = false;
			AddError(sTmp + "KeepNumerical parameter (operand 3) " + "(" +
				 KWContinuous::ContinuousToString(cValue) + ")" + ": operand must be equal to 0 or 1");
		}

		// Verification du parametre AdditionalChars
		operand = GetOperandAt(3);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
		sValue = operand->GetSymbolConstant();
		ivAdditionnalChars.SetSize(256);
		nAdditionnalCharNumber = sValue.GetLength();
		for (i = 0; i < nAdditionnalCharNumber; i++)
		{
			// Test si un caractere est specifie plus de une fois
			nChar = (unsigned int)sValue.GetAt(i);
			if (ivAdditionnalChars.GetAt(nChar) != 0)
			{
				bOk = false;
				AddError(sTmp + "AdditionalChars parameter (operand 4): " + " char (" +
					 sValue.GetAt(i) + ") is specified more than once");
				break;
			}
			ivAdditionnalChars.SetAt(nChar, 1);
		}

		// Verification du parametre TranslatedAdditionalChars
		operand = GetOperandAt(4);
		sValue = operand->GetSymbolConstant();
		if (nAdditionnalCharNumber != sValue.GetLength())
		{
			bOk = false;
			AddError(sTmp + "TranslatedAdditionalChars parameter (operand 5): " +
				 " number of translated chars (" + IntToString(sValue.GetLength()) +
				 ") must be the same as the number of additional chars (" +
				 IntToString(nAdditionnalCharNumber) + ")");
		}

		// Verification du parametre MaxLength
		operand = GetOperandAt(5);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
		cValue = operand->GetContinuousConstant();
		bIsInt = KWContinuous::ContinuousToInt(cValue, nValue);
		if (not bIsInt or nValue <= 0)
		{
			bOk = false;
			AddError(sTmp + "MaxLength parameter (operand 6) " + "(" +
				 KWContinuous::ContinuousToString(cValue) + ")" +
				 ": operand must an integer greater than 0");
		}
	}
	return bOk;
}

void KWDRExtractWords::Compile(KWClass* kwcOwnerClass)
{
	boolean bDisplay = false;
	boolean bToLower;
	boolean bKeepNumerical;
	Symbol sAdditionalChars;
	Symbol sTranslatedAdditionalChars;
	int nAdditionnalCharNumber;
	unsigned char nChar;
	int i;
	char c;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du parametre de longueur maximale
	nMaxLength = (int)floor(GetOperandAt(5)->GetContinuousConstant() + 0.5);
	assert(nMaxLength > 0);

	// Recherche des parametres de recodage des caracteres
	bToLower = ((int)floor(GetOperandAt(1)->GetContinuousConstant() + 0.5) != 0);
	bKeepNumerical = ((int)floor(GetOperandAt(2)->GetContinuousConstant() + 0.5) != 0);
	sAdditionalChars = GetOperandAt(3)->GetSymbolConstant();
	sTranslatedAdditionalChars = GetOperandAt(4)->GetSymbolConstant();
	assert(sAdditionalChars.GetLength() == sTranslatedAdditionalChars.GetLength());

	// Initialisation de la chaine de recodage avec des blancs
	sTranslatedChars.Empty();
	for (i = 0; i < 256; i++)
		sTranslatedChars += ' ';

	// Ajout des caracteres alphabetique et numeriques
	for (i = 0; i < 256; i++)
	{
		c = (char)i;
		if (isalpha(c))
		{
			if (bToLower)
				sTranslatedChars.SetAt(i, (char)tolower(c));
			else
				sTranslatedChars.SetAt(i, c);
		}
		if (isdigit(i))
		{
			if (bKeepNumerical)
				sTranslatedChars.SetAt(i, c);
		}
	}

	// Ajout de la version recodee des caracteres additionnels
	nAdditionnalCharNumber = sAdditionalChars.GetLength();
	for (i = 0; i < nAdditionnalCharNumber; i++)
	{
		nChar = sAdditionalChars.GetAt(i);
		sTranslatedChars.SetAt(nChar, sTranslatedAdditionalChars.GetAt(i));
	}

	// Affichage du resultat
	if (bDisplay)
	{
		for (i = 0; i < 256; i++)
			cout << i << "\t" << (char)i << "\t" << sTranslatedChars.GetAt(i) << "\n";
	}
}

longint KWDRExtractWords::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRAsContinuousVector) - sizeof(KWDerivationRule);
	lUsedMemory += symbolVector.GetUsedMemory();
	lUsedMemory += sTranslatedChars.GetLength() + 1;
	return lUsedMemory;
}
