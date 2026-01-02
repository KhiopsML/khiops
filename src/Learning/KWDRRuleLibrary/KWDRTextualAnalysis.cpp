// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTextualAnalysis.h"

void KWDRRegisterTextualAnalysisRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTextNgrams);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListNgrams);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextWords);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListWords);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokens);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListTokens);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextInit);
	KWDRRegisterTextualAnalysisPROTORules();
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenizationRule

KWDRTokenizationRule::KWDRTokenizationRule()
{
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);
	textTokenizer = NULL;
	nDynamicCompileFreshness = 0;
}

KWDRTokenizationRule::~KWDRTokenizationRule()
{
	assert(textTokenizer != NULL);
	delete textTokenizer;
}

int KWDRTokenizationRule::GetVarKeyType() const
{
	return KWType::Symbol;
}

KWContinuousValueBlock*
KWDRTokenizationRule::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
							const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbResult;
	KWDerivationRuleOperand* valueOperand;
	SymbolVector* svValues;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Acces au premier operande
	valueOperand = GetFirstOperand();
	assert(KWType::IsTextBased(valueOperand->GetType()));

	// Tokenisation selon le type de l'operande
	if (valueOperand->GetType() == KWType::Text)
		cvbResult = textTokenizer->BuildBlockFromSymbol(GetFirstOperand()->GetTextValue(kwoObject));
	else
	{
		svValues = GetFirstOperand()->GetTextListValue(kwoObject);

		// Cas d'un vecteur vide ou d'une seule valeur vide
		if (svValues == NULL or svValues->GetSize() == 0)
			cvbResult = KWContinuousValueBlock::NewValueBlock(0);
		// Cas general
		else
			cvbResult = textTokenizer->BuildBlockFromSymbolVector(svValues);
	}
	return cvbResult;
}

Continuous KWDRTokenizationRule::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

const ALString KWDRTokenizationRule::BuildAttributeKeyFromToken(const ALString& sToken) const
{
	if (sToken.GetLength() > 0 and (GetMaxTokenLength() == 0 or sToken.GetLength() <= GetMaxTokenLength()))
		return TextService::ByteStringToWord(sToken);
	else
		return "";
}

const ALString KWDRTokenizationRule::BuildTokenFromAttributeKey(const ALString& sToken) const
{
	return TextService::WordToByteString(sToken);
}

int KWDRTokenizationRule::GetMaxTokenLength() const
{
	return 0;
}

boolean KWDRTokenizationRule::CheckBlockAttributes(const KWClass* kwcOwnerClass,
						   const KWAttributeBlock* attributeBlock) const
{
	boolean bOk;
	KWAttribute* attribute;
	ALString sAttributeKeyMetaDataKey;
	ALString sAttributeKey;
	ALString sToken;

	require(kwcOwnerClass != NULL);
	require(attributeBlock != NULL);

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Verification de la validite des VarKey du bloc, qui doivent correspondre a des encodage de tokens
	if (bOk)
	{
		// Parcours des attributs du blocs
		sAttributeKeyMetaDataKey = attributeBlock->GetAttributeKeyMetaDataKey();
		attribute = attributeBlock->GetFirstAttribute();
		while (attribute != NULL)
		{
			// Traitement sur l'attribut en cours
			sAttributeKey = attribute->GetConstMetaData()->GetStringValueAt(sAttributeKeyMetaDataKey);

			// Calcul du token correspondant
			sToken = BuildTokenFromAttributeKey(sAttributeKey);

			// Arret si erreur
			if (sToken == "")
			{
				AddError("Meta-data " + sAttributeKeyMetaDataKey + " (" +
					 attribute->GetConstMetaData()->GetExternalValueAt(sAttributeKeyMetaDataKey) +
					 ") related to variable " + attribute->GetName() +
					 " in the block should correspond to the encoding of a text token");
				bOk = false;
				break;
			}

			// Arret si fin du bloc
			if (attribute == attributeBlock->GetLastAttribute())
				break;
			attributeBlock->GetParentClass()->GetNextAttribute(attribute);
		}
	}
	return bOk;
}

KWDRTokenizationRule* KWDRTokenizationRule::CreateTokenizationRule(int textualType, const ALString& sTextFeatures)
{
	KWDRTokenizationRule* textVariableBlockRule;

	require(KWType::IsTextBased(textualType));
	require(KWTextTokenizer::CheckTextFeatures(sTextFeatures));

	if (sTextFeatures == "ngrams")
	{
		if (textualType == KWType::Text)
			textVariableBlockRule = new KWDRTextNgrams;
		else
			textVariableBlockRule = new KWDRTextListNgrams;
	}
	else if (sTextFeatures == "words")
	{
		if (textualType == KWType::Text)
			textVariableBlockRule = new KWDRTextWords;
		else
			textVariableBlockRule = new KWDRTextListWords;
	}
	else
	{
		if (textualType == KWType::Text)
			textVariableBlockRule = new KWDRTextTokens;
		else
			textVariableBlockRule = new KWDRTextListTokens;
	}
	return textVariableBlockRule;
}

longint KWDRTokenizationRule::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDRTokenizationRule);
	if (textTokenizer != NULL)
		lUsedMemory += textTokenizer->GetUsedMemory();
	return lUsedMemory;
}

void KWDRTokenizationRule::DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const
{
	StringVector svDeploymentTokens;
	const KWIndexedCKeyBlock* indexedCKeyBlock;
	ALString sToken;
	int i;

	require(indexedKeyBlock != NULL);
	require(indexedKeyBlock->GetVarKeyType() == KWType::Symbol);
	require(Check());
	require(IsCompiled());
	require(textTokenizer != NULL);

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Creation du vecteur de tokens a deployer
		indexedCKeyBlock = cast(const KWIndexedCKeyBlock*, indexedKeyBlock);
		svDeploymentTokens.SetSize(indexedKeyBlock->GetKeyNumber());
		for (i = 0; i < indexedKeyBlock->GetKeyNumber(); i++)
		{
			sToken = BuildTokenFromAttributeKey(indexedCKeyBlock->GetKeyAt(i).GetValue());
			svDeploymentTokens.SetAt(i, sToken);
		}

		// Specification de deploiement pour le tokeniser
		textTokenizer->SetDeploymentTokens(&svDeploymentTokens);

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextNgrams

KWDRTextNgrams::KWDRTextNgrams()
{
	SetName("TextNgrams");
	SetLabel("Ngram counts from a text value");

	// Specification du tokenizer
	textTokenizer = new KWTextNgramTokenizer;
}

KWDRTextNgrams::~KWDRTextNgrams() {}

KWDerivationRule* KWDRTextNgrams::Create() const
{
	return new KWDRTextNgrams;
}

int KWDRTextNgrams::GetMaxTokenLength() const
{
	return KWTextNgramTokenizer::GetMaxNgramLength();
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListNgrams

KWDRTextListNgrams::KWDRTextListNgrams()
{
	// Specialisation de la classe mere
	SetName("TextListNgrams");
	SetLabel("Ngram counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListNgrams::~KWDRTextListNgrams() {}

KWDerivationRule* KWDRTextListNgrams::Create() const
{
	return new KWDRTextListNgrams;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextWords

KWDRTextWords::KWDRTextWords()
{
	SetName("TextWords");
	SetLabel("Word counts from a text value");

	// Specification du tokenizer
	textTokenizer = new KWTextWordTokenizer;
}

KWDRTextWords::~KWDRTextWords() {}

KWDerivationRule* KWDRTextWords::Create() const
{
	return new KWDRTextWords;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListWords

KWDRTextListWords::KWDRTextListWords()
{
	// Specialisation de la classe mere
	SetName("TextListWords");
	SetLabel("Word counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListWords::~KWDRTextListWords() {}

KWDerivationRule* KWDRTextListWords::Create() const
{
	return new KWDRTextListWords;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokens

KWDRTextTokens::KWDRTextTokens()
{
	SetName("TextTokens");
	SetLabel("Token counts from a text value");

	// Specification du tokenizer
	textTokenizer = new KWTextTokenizer;
}

KWDRTextTokens::~KWDRTextTokens() {}

KWDerivationRule* KWDRTextTokens::Create() const
{
	return new KWDRTextTokens;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListTokens

KWDRTextListTokens::KWDRTextListTokens()
{
	// Specialisation de la classe mere
	SetName("TextListTokens");
	SetLabel("Token counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListTokens::~KWDRTextListTokens() {}

KWDerivationRule* KWDRTextListTokens::Create() const
{
	return new KWDRTextListTokens;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextInit

KWDRTextInit::KWDRTextInit()
{
	SetName("TextInit");
	SetLabel("Initialization of a text variable");
	SetType(KWType::Text);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Symbol);

	// On impose que les deux derniers operandes soient des constantes
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRTextInit::~KWDRTextInit() {}

KWDerivationRule* KWDRTextInit::Create() const
{
	return new KWDRTextInit;
}

Symbol KWDRTextInit::ComputeTextResult(const KWObject* kwoObject) const
{
	ALString sResult;
	int nSize;
	char cValue;

	// Construction d'un chaine de la longueur souhaitee
	nSize = int(floor(GetFirstOperand()->GetContinuousConstant() + 0.5));
	cValue = GetSecondOperand()->GetSymbolConstant().GetAt(0);
	sResult = ALString(cValue, nSize);
	return Symbol(sResult, nSize);
}

boolean KWDRTextInit::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Continuous cValue;
	int nValue;
	Symbol sValue;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des operandes
	if (bOk)
	{
		assert(GetOperandNumber() == 2);

		// Taille du texe a generer
		cValue = GetFirstOperand()->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(cValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 0)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must be positive");
			bOk = false;
		}
		else if (nValue > InputBufferedFile::nMaxFieldSize)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must less than " + IntToString(InputBufferedFile::nMaxFieldSize));
			bOk = false;
		}

		// Longuer du deuxieme operande, qui doit etre reduit a un seul caractere
		sValue = GetSecondOperand()->GetSymbolConstant();
		if (sValue.GetLength() != 1)
		{
			AddError(sTmp + "Second operand (" + sValue + ") must contain one single character");
			bOk = false;
		}
	}
	return bOk;
}
