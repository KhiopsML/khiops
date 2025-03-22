// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRStandard.h"

void KWDRRegisterStandardRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRCopySymbol);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyContinuous);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyTimestampTZ);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextCopy);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopySymbolValueBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRCopyContinuousValueBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsContinuous);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsContinuousError);
	KWDerivationRule::RegisterDerivationRule(new KWDRRecodeMissing);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsSymbol);
	KWDerivationRule::RegisterDerivationRule(new KWDRFromText);
	KWDerivationRule::RegisterDerivationRule(new KWDRToText);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRFormatDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRFormatTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRFormatTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRAsTimestampTZ);
	KWDerivationRule::RegisterDerivationRule(new KWDRFormatTimestampTZ);
}

//////////////////////////////////////////////////////////////////////////////////////

void KWDRConversionRule::SetErrorSender(const Object* errorSender)
{
	oErrorSender = errorSender;
}

const Object* KWDRConversionRule::GetErrorSender()
{
	return oErrorSender;
}

const Object* KWDRConversionRule::oErrorSender = NULL;

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopySymbol::KWDRCopySymbol()
{
	SetName("CopyC");
	SetLabel("Copy of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRCopySymbol::~KWDRCopySymbol() {}

KWDerivationRule* KWDRCopySymbol::Create() const
{
	return new KWDRCopySymbol;
}

Symbol KWDRCopySymbol::ComputeSymbolResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyContinuous::KWDRCopyContinuous()
{
	SetName("Copy");
	SetLabel("Copy of a numerical value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRCopyContinuous::~KWDRCopyContinuous() {}

KWDerivationRule* KWDRCopyContinuous::Create() const
{
	return new KWDRCopyContinuous;
}

Continuous KWDRCopyContinuous::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetContinuousValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyDate::KWDRCopyDate()
{
	SetName("CopyD");
	SetLabel("Copy of a date value");
	SetType(KWType::Date);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRCopyDate::~KWDRCopyDate() {}

KWDerivationRule* KWDRCopyDate::Create() const
{
	return new KWDRCopyDate;
}

Date KWDRCopyDate::ComputeDateResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetDateValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyTime::KWDRCopyTime()
{
	SetName("CopyT");
	SetLabel("Copy of a time value");
	SetType(KWType::Time);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRCopyTime::~KWDRCopyTime() {}

KWDerivationRule* KWDRCopyTime::Create() const
{
	return new KWDRCopyTime;
}

Time KWDRCopyTime::ComputeTimeResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetTimeValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyTimestamp::KWDRCopyTimestamp()
{
	SetName("CopyTS");
	SetLabel("Copy of a timestamp value");
	SetType(KWType::Timestamp);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRCopyTimestamp::~KWDRCopyTimestamp() {}

KWDerivationRule* KWDRCopyTimestamp::Create() const
{
	return new KWDRCopyTimestamp;
}

Timestamp KWDRCopyTimestamp::ComputeTimestampResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetTimestampValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyTimestampTZ::KWDRCopyTimestampTZ()
{
	SetName("CopyTSTZ");
	SetLabel("Copy of a timestampTZ value");
	SetType(KWType::TimestampTZ);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::TimestampTZ);
}

KWDRCopyTimestampTZ::~KWDRCopyTimestampTZ() {}

KWDerivationRule* KWDRCopyTimestampTZ::Create() const
{
	return new KWDRCopyTimestampTZ;
}

TimestampTZ KWDRCopyTimestampTZ::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetTimestampTZValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextCopy::KWDRTextCopy()
{
	SetName("TextCopy");
	SetLabel("Copy of a text value");
	SetType(KWType::Text);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);
}

KWDRTextCopy::~KWDRTextCopy() {}

KWDerivationRule* KWDRTextCopy::Create() const
{
	return new KWDRTextCopy;
}

Symbol KWDRTextCopy::ComputeTextResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetTextValue(kwoObject);
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRValueBlockRule

int KWDRValueBlockRule::GetVarKeyType() const
{
	return nVarKeyType;
}

Continuous KWDRValueBlockRule::GetValueBlockContinuousDefaultValue() const
{
	require(Check());
	require(nSourceValueBlockOperandIndex >= 0);
	assert(GetOperandAt(nSourceValueBlockOperandIndex)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
	return GetOperandAt(nSourceValueBlockOperandIndex)->GetOriginAttributeBlock()->GetContinuousDefaultValue();
}

Symbol& KWDRValueBlockRule::GetValueBlockSymbolDefaultValue() const
{
	require(Check());
	require(nSourceValueBlockOperandIndex >= 0);
	assert(GetOperandAt(nSourceValueBlockOperandIndex)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
	return GetOperandAt(nSourceValueBlockOperandIndex)->GetOriginAttributeBlock()->GetSymbolDefaultValue();
}

boolean KWDRValueBlockRule::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;

	require(kwcOwnerClass != NULL);
	require(nSourceValueBlockOperandIndex >= 0);
	require(KWType::IsValueBlock(GetType()));

	// Methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// On verifie en plus la coherence avec le type de l'attribut retourne
	if (bReturnTypeSameAsOperandType)
	{
		if (GetOperandNumber() > nSourceValueBlockOperandIndex and
		    KWType::IsValueBlock(GetOperandAt(nSourceValueBlockOperandIndex)->GetType()))
		{
			if (GetType() != GetOperandAt(nSourceValueBlockOperandIndex)->GetType())
			{
				bOk = false;
				AddError("Type " + KWType::ToString(GetType()) +
					 " returned by rule inconsistent with that of operand " +
					 IntToString(nSourceValueBlockOperandIndex + 1) + " (" +
					 KWType::ToString(GetOperandAt(nSourceValueBlockOperandIndex)->GetType()) +
					 ")");
			}
		}
	}
	return bOk;
}

void KWDRValueBlockRule::CopyFrom(const KWDerivationRule* kwdrSource)
{
	require(kwdrSource != NULL);

	// Specialisation de la methode ancetre pour recopier le VarKeyType
	KWDerivationRule::CopyFrom(kwdrSource);
	nVarKeyType = cast(KWDRValueBlockRule*, kwdrSource)->nVarKeyType;
	ivNewValueIndexes.SetSize(0);
	bSameValueIndexes = false;
	nDynamicCompileFreshness = 0;
}

void KWDRValueBlockRule::DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const
{
	boolean bDisplay = false;
	KWAttributeBlock* originAttributeBlock;
	const KWIndexedKeyBlock* sourceIndexedKeyBlock;
	const KWIndexedNKeyBlock* indexedNKeyBlock;
	const KWIndexedNKeyBlock* sourceIndexedNKeyBlock;
	const KWIndexedCKeyBlock* indexedCKeyBlock;
	const KWIndexedCKeyBlock* sourceIndexedCKeyBlock;
	int nSourceIndex;
	int nIndex;
	Symbol sKey;
	int nKey;

	require(indexedKeyBlock != NULL);
	require(Check());
	require(IsCompiled());
	require(nSourceValueBlockOperandIndex >= 0);
	require(KWType::IsValueBlock(GetType()));
	require(GetOperandAt(nSourceValueBlockOperandIndex)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Recherche du bloc d'attribut source
		originAttributeBlock = GetOperandAt(nSourceValueBlockOperandIndex)->GetOriginAttributeBlock();

		// Recherche du bloc de cle source
		sourceIndexedKeyBlock = originAttributeBlock->GetLoadedAttributesIndexedKeyBlock();
		assert(sourceIndexedKeyBlock->GetVarKeyType() == indexedKeyBlock->GetVarKeyType());

		// Cas des blocs avec des VarKey categoriels
		if (indexedKeyBlock->GetVarKeyType() == KWType::Symbol)
		{
			// On prend les bons types de blocs de cles
			indexedCKeyBlock = cast(const KWIndexedCKeyBlock*, indexedKeyBlock);
			sourceIndexedCKeyBlock = cast(const KWIndexedCKeyBlock*, sourceIndexedKeyBlock);

			// Parcours des cles du bloc source
			bSameValueIndexes =
			    (sourceIndexedCKeyBlock->GetKeyNumber() == indexedCKeyBlock->GetKeyNumber());
			ivNewValueIndexes.SetSize(sourceIndexedKeyBlock->GetKeyNumber());
			for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
			{
				sKey = sourceIndexedCKeyBlock->GetKeyAt(nSourceIndex);

				// Recherche de l'index dans le bloc cible
				nIndex = indexedCKeyBlock->GetKeyIndex(sKey);

				// Memorisation du nouvel index
				ivNewValueIndexes.SetAt(nSourceIndex, nIndex);

				// Mise a jour de l'indicateur d'index indentiques
				bSameValueIndexes = bSameValueIndexes and (nIndex == nSourceIndex);
			}
		}
		// Cas des blocs avec des VarKey numeriques
		else
		{
			// On prend les bons types de blocs de cles
			indexedNKeyBlock = cast(const KWIndexedNKeyBlock*, indexedKeyBlock);
			sourceIndexedNKeyBlock = cast(const KWIndexedNKeyBlock*, sourceIndexedKeyBlock);

			// Parcours des cles du bloc source
			bSameValueIndexes =
			    (sourceIndexedNKeyBlock->GetKeyNumber() == indexedNKeyBlock->GetKeyNumber());
			ivNewValueIndexes.SetSize(sourceIndexedKeyBlock->GetKeyNumber());
			for (nSourceIndex = 0; nSourceIndex < sourceIndexedKeyBlock->GetKeyNumber(); nSourceIndex++)
			{
				nKey = sourceIndexedNKeyBlock->GetKeyAt(nSourceIndex);

				// Recherche de l'index dans le bloc cible
				nIndex = indexedNKeyBlock->GetKeyIndex(nKey);

				// Memorisation du nouvel index
				ivNewValueIndexes.SetAt(nSourceIndex, nIndex);

				// Mise a jour de l'indicateur d'index indentiques
				bSameValueIndexes = bSameValueIndexes and (nIndex == nSourceIndex);
			}
		}

		// Nettoyage si memes index de valeurs
		if (bSameValueIndexes)
			ivNewValueIndexes.SetSize(0);

		// Affichage
		if (bDisplay)
		{
			cout << GetClassName() << " " << GetName() << endl;
			cout << "\tSource block: " << *sourceIndexedKeyBlock;
			cout << "\tTarget block: " << *indexedKeyBlock;
			cout << "\tSame block: " << BooleanToString(bSameValueIndexes) << "\n";
			cout << "\tNew value indexes: " << ivNewValueIndexes << endl;
		}

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

longint KWDRValueBlockRule::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRValueBlockRule) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

KWDRValueBlockRule::KWDRValueBlockRule()
{
	// On donne un type valide par defaut
	nVarKeyType = KWType::Symbol;

	// Initialisation a -1, pour forcer une specification valide dans les sous-classes
	nSourceValueBlockOperandIndex = -1;

	// Initialisation a true, pour gerer explicitement le type de bloc retourne
	bReturnTypeSameAsOperandType = true;

	// Gestion de la compilation dynamique
	bSameValueIndexes = false;
	nDynamicCompileFreshness = 0;
}

void KWDRValueBlockRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						  NumericKeyDictionary* nkdCompletedAttributes)
{
	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);
	require(nSourceValueBlockOperandIndex >= 0);

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Proparagation des caracteristiques du bloc
	if (GetOperandNumber() > nSourceValueBlockOperandIndex)
	{
		// Type retourne
		if (bReturnTypeSameAsOperandType)
			SetType(GetOperandAt(nSourceValueBlockOperandIndex)->GetType());

		// Type des cles du bloc
		nVarKeyType = GetOperandAt(nSourceValueBlockOperandIndex)->GetVarKeyType();
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopySymbolValueBlock::KWDRCopySymbolValueBlock()
{
	SetName("CopyBlockC");
	SetLabel("Copy of a categorical sparse value block");
	SetType(KWType::SymbolValueBlock);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::SymbolValueBlock);
	nSourceValueBlockOperandIndex = 0;
}

KWDRCopySymbolValueBlock::~KWDRCopySymbolValueBlock() {}

KWDerivationRule* KWDRCopySymbolValueBlock::Create() const
{
	return new KWDRCopySymbolValueBlock;
}

KWSymbolValueBlock*
KWDRCopySymbolValueBlock::ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWSymbolValueBlock* svbResult;
	KWDerivationRuleOperand* valueOperand;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Recherche du bloc source
	valueOperand = GetFirstOperand();
	assert(valueOperand->GetOrigin() != KWDerivationRuleOperand::OriginRule);

	// Extraction du sous-bloc
	if (bSameValueIndexes)
		svbResult = valueOperand->GetSymbolValueBlock(kwoObject)->Clone();
	else
		svbResult = KWSymbolValueBlock::ExtractBlockSubset(valueOperand->GetSymbolValueBlock(kwoObject),
								   &ivNewValueIndexes);
	return svbResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRCopyContinuousValueBlock::KWDRCopyContinuousValueBlock()
{
	SetName("CopyBlock");
	SetLabel("Copy of a numerical sparse value block");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ContinuousValueBlock);
	nSourceValueBlockOperandIndex = 0;
}

KWDRCopyContinuousValueBlock::~KWDRCopyContinuousValueBlock() {}

KWDerivationRule* KWDRCopyContinuousValueBlock::Create() const
{
	return new KWDRCopyContinuousValueBlock;
}

KWContinuousValueBlock*
KWDRCopyContinuousValueBlock::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
								const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbResult;
	KWDerivationRuleOperand* valueOperand;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Recherche du bloc source
	valueOperand = GetFirstOperand();
	assert(valueOperand->GetOrigin() != KWDerivationRuleOperand::OriginRule);

	// Extraction du sous-bloc
	if (bSameValueIndexes)
		cvbResult = valueOperand->GetContinuousValueBlock(kwoObject)->Clone();
	else
		cvbResult = KWContinuousValueBlock::ExtractBlockSubset(valueOperand->GetContinuousValueBlock(kwoObject),
								       &ivNewValueIndexes);
	return cvbResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsContinuous::KWDRAsContinuous()
{
	SetName("AsNumerical");
	SetLabel("Conversion of a categorical value to a numerical value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRAsContinuous::~KWDRAsContinuous() {}

KWDerivationRule* KWDRAsContinuous::Create() const
{
	return new KWDRAsContinuous;
}

Continuous KWDRAsContinuous::ComputeContinuousResult(const KWObject* kwoObject) const
{
	int nError;
	Symbol sValue;
	Continuous cValue;

	require(IsCompiled());

	// Conversion avec message d'erreur
	if (oErrorSender != NULL and GetOperandAt(0)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute and
	    GetOperandAt(0)->GetDerivationRule() == NULL)
	{
		sValue = GetFirstOperand()->GetSymbolValue(kwoObject),
		nError = KWContinuous::StringToContinuousError(sValue, cValue);

		// Test de validite du champ
		if (nError != 0)
			oErrorSender->AddWarning("Dictionary " + kwoObject->GetClass()->GetName() + ", rule " +
						 GetName() + "(" + GetOperandAt(0)->GetAttributeName() + ")" +
						 ": value <" + InputBufferedFile::GetDisplayValue(sValue.GetValue()) +
						 "> converted to <" + KWContinuous::ContinuousToString(cValue) + "> (" +
						 KWContinuous::ErrorLabel(nError) + ")");
		return cValue;
	}
	// Conversion sans message d'erreur
	else
		return KWContinuous::StringToContinuous(GetFirstOperand()->GetSymbolValue(kwoObject));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsContinuousError::KWDRAsContinuousError()
{
	SetName("AsNumericalError");
	SetLabel("Label of a conversion error when converting a categorical value to a numerical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRAsContinuousError::~KWDRAsContinuousError() {}

KWDerivationRule* KWDRAsContinuousError::Create() const
{
	return new KWDRAsContinuousError;
}

Symbol KWDRAsContinuousError::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	int nError;
	require(IsCompiled());

	nError = KWContinuous::StringToContinuousError(GetFirstOperand()->GetSymbolValue(kwoObject), cValue);
	return Symbol(KWContinuous::ErrorLabel(nError));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRecodeMissing::KWDRRecodeMissing()
{
	SetName("RecodeMissing");
	SetLabel("Recode numerical values by changing Missing by a replace value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRRecodeMissing::~KWDRRecodeMissing() {}

KWDerivationRule* KWDRRecodeMissing::Create() const
{
	return new KWDRRecodeMissing;
}

Continuous KWDRRecodeMissing::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(IsCompiled());

	// On renvoie la valeur numerique si elle n'est pas missing
	cValue = GetFirstOperand()->GetContinuousValue(kwoObject);
	if (cValue != KWContinuous::GetMissingValue())
		return cValue;
	// Sinon, on renvoie la valeur de remplacement
	else
	{
		cValue = GetSecondOperand()->GetContinuousValue(kwoObject);
		return cValue;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsSymbol::KWDRAsSymbol()
{
	SetName("AsCategorical");
	SetLabel("Conversion of a numerical value to a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Continuous);
}

KWDRAsSymbol::~KWDRAsSymbol() {}

KWDerivationRule* KWDRAsSymbol::Create() const
{
	return new KWDRAsSymbol;
}

Symbol KWDRAsSymbol::ComputeSymbolResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return Symbol(KWContinuous::ContinuousToString(GetFirstOperand()->GetContinuousValue(kwoObject)));
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFromText::KWDRFromText()
{
	SetName("FromText");
	SetLabel("Conversion of a text value to a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);
}

KWDRFromText::~KWDRFromText() {}

KWDerivationRule* KWDRFromText::Create() const
{
	return new KWDRFromText;
}

Symbol KWDRFromText::ComputeSymbolResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetTextValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRToText::KWDRToText()
{
	SetName("ToText");
	SetLabel("Conversion of a categorical value to a text value");
	SetType(KWType::Text);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRToText::~KWDRToText() {}

KWDerivationRule* KWDRToText::Create() const
{
	return new KWDRToText;
}

Symbol KWDRToText::ComputeTextResult(const KWObject* kwoObject) const
{
	require(IsCompiled());

	return GetFirstOperand()->GetSymbolValue(kwoObject);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsDate::KWDRAsDate()
{
	SetName("AsDate");
	SetLabel("Recode a categorical value into a date using a date format");
	SetType(KWType::Date);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRAsDate::~KWDRAsDate() {}

KWDerivationRule* KWDRAsDate::Create() const
{
	return new KWDRAsDate;
}

Date KWDRAsDate::ComputeDateResult(const KWObject* kwoObject) const
{
	Symbol sDate;
	Date dtDate;

	require(IsCompiled());

	// Acces a l'operande valeur de date
	sDate = GetOperandAt(0)->GetSymbolValue(kwoObject);

	// Concersion en date
	dtDate = dtfDateFormat.StringToDate(sDate);

	// Emission d'une erreur si demande et si transcodage d'un attribut natif
	if (oErrorSender != NULL and GetOperandAt(0)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute and
	    GetOperandAt(0)->GetDerivationRule() == NULL and not sDate.IsEmpty() and not dtDate.Check())
	{
		oErrorSender->AddWarning("Dictionary " + kwoObject->GetClass()->GetName() + ", rule " + GetName() +
					 "(" + GetOperandAt(0)->GetAttributeName() + ", \"" +
					 GetOperandAt(1)->GetSymbolConstant() + "\")" + ": value <" +
					 InputBufferedFile::GetDisplayValue(sDate.GetValue()) +
					 "> converted to <> (invalid date)");
	}
	return dtDate;
}

boolean KWDRAsDate::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sDateFormat;
	KWDateFormat dtfDateFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de date
		sDateFormat = GetSecondOperand()->GetSymbolConstant();
		dtfDateFormatCheck.SetFormatString(sDateFormat.GetValue());

		// Erreur si invalide
		if (not dtfDateFormatCheck.Check())
		{
			AddError(sTmp + "Invalid date format parameter (" + sDateFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRAsDate::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de date
	dtfDateFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(dtfDateFormat.Check());
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFormatDate::KWDRFormatDate()
{
	SetName("FormatDate");
	SetLabel("Format a date into a categorical value using a date format");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Date);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFormatDate::~KWDRFormatDate() {}

KWDerivationRule* KWDRFormatDate::Create() const
{
	return new KWDRFormatDate;
}

Symbol KWDRFormatDate::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sDate;
	Date dtDate;

	require(IsCompiled());

	// Acces a l'operande valeur de date
	dtDate = GetOperandAt(0)->GetDateValue(kwoObject);

	// Formatage de la date
	sDate = dtfDateFormat.DateToString(dtDate);
	return sDate;
}

boolean KWDRFormatDate::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sDateFormat;
	KWDateFormat dtfDateFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de date
		sDateFormat = GetSecondOperand()->GetSymbolConstant();
		dtfDateFormatCheck.SetFormatString(sDateFormat.GetValue());

		// Erreur si invalide
		if (not dtfDateFormatCheck.Check())
		{
			AddError(sTmp + "Invalid date format parameter (" + sDateFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRFormatDate::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de date
	dtfDateFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(dtfDateFormat.Check());
}

longint KWDRFormatDate::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRFormatDate) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsTime::KWDRAsTime()
{
	SetName("AsTime");
	SetLabel("Recode a categorical value into a time using a time format");
	SetType(KWType::Time);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRAsTime::~KWDRAsTime() {}

KWDerivationRule* KWDRAsTime::Create() const
{
	return new KWDRAsTime;
}

Time KWDRAsTime::ComputeTimeResult(const KWObject* kwoObject) const
{
	Symbol sTime;
	Time tmTime;

	require(IsCompiled());

	// Acces a l'operande valeur de time
	sTime = GetOperandAt(0)->GetSymbolValue(kwoObject);

	// Conversion en time
	tmTime = tmfTimeFormat.StringToTime(sTime);

	// Emission d'une erreur si demande et si transcodage d'un attribut natif
	if (oErrorSender != NULL and GetOperandAt(0)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute and
	    GetOperandAt(0)->GetDerivationRule() == NULL and not sTime.IsEmpty() and not tmTime.Check())
	{
		oErrorSender->AddWarning("Dictionary " + kwoObject->GetClass()->GetName() + ", rule " + GetName() +
					 "(" + GetOperandAt(0)->GetAttributeName() + ", \"" +
					 GetOperandAt(1)->GetSymbolConstant() + "\")" + ": value <" +
					 InputBufferedFile::GetDisplayValue(sTime.GetValue()) +
					 "> converted to <> (invalid time)");
	}
	return tmTime;
}

boolean KWDRAsTime::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimeFormat;
	KWTimeFormat tmfTimeFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de time
		sTimeFormat = GetSecondOperand()->GetSymbolConstant();
		tmfTimeFormatCheck.SetFormatString(sTimeFormat.GetValue());

		// Erreur si invalide
		if (not tmfTimeFormatCheck.Check())
		{
			AddError(sTmp + "Invalid time format parameter (" + sTimeFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRAsTime::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de time
	tmfTimeFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tmfTimeFormat.Check());
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFormatTime::KWDRFormatTime()
{
	SetName("FormatTime");
	SetLabel("Format a time into a categorical value using a time format");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Time);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFormatTime::~KWDRFormatTime() {}

KWDerivationRule* KWDRFormatTime::Create() const
{
	return new KWDRFormatTime;
}

Symbol KWDRFormatTime::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sTime;
	Time tmTime;

	require(IsCompiled());

	// Acces a l'operande valeur de time
	tmTime = GetOperandAt(0)->GetTimeValue(kwoObject);

	// Formatage de la time
	sTime = tmfTimeFormat.TimeToString(tmTime);
	return sTime;
}

boolean KWDRFormatTime::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimeFormat;
	KWTimeFormat tmfTimeFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de time
		sTimeFormat = GetSecondOperand()->GetSymbolConstant();
		tmfTimeFormatCheck.SetFormatString(sTimeFormat.GetValue());

		// Erreur si invalide
		if (not tmfTimeFormatCheck.Check())
		{
			AddError(sTmp + "Invalid time format parameter (" + sTimeFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRFormatTime::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de time
	tmfTimeFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tmfTimeFormat.Check());
}

longint KWDRFormatTime::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRFormatTime) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsTimestamp::KWDRAsTimestamp()
{
	SetName("AsTimestamp");
	SetLabel("Recode a categorical value into a timestamp using a timestamp format");
	SetType(KWType::Timestamp);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRAsTimestamp::~KWDRAsTimestamp() {}

KWDerivationRule* KWDRAsTimestamp::Create() const
{
	return new KWDRAsTimestamp;
}

Timestamp KWDRAsTimestamp::ComputeTimestampResult(const KWObject* kwoObject) const
{
	Symbol sTimestamp;
	Timestamp tsTimestamp;

	require(IsCompiled());

	// Acces a l'operande valeur de timestamp
	sTimestamp = GetOperandAt(0)->GetSymbolValue(kwoObject);

	// Conversion en timestamp
	tsTimestamp = tsfTimestampFormat.StringToTimestamp(sTimestamp);

	// Emission d'une erreur si demande et si transcodage d'un attribut natif
	if (oErrorSender != NULL and GetOperandAt(0)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute and
	    GetOperandAt(0)->GetDerivationRule() == NULL and not sTimestamp.IsEmpty() and not tsTimestamp.Check())
	{
		oErrorSender->AddWarning("Dictionary " + kwoObject->GetClass()->GetName() + ", rule " + GetName() +
					 "(" + GetOperandAt(0)->GetAttributeName() + ", \"" +
					 GetOperandAt(1)->GetSymbolConstant() + "\")" + ": value <" +
					 InputBufferedFile::GetDisplayValue(sTimestamp.GetValue()) +
					 "> converted to <> (invalid timestamp)");
	}
	return tsTimestamp;
}

boolean KWDRAsTimestamp::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimestampFormat;
	KWTimestampFormat tsfTimestampFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de timestamp
		sTimestampFormat = GetSecondOperand()->GetSymbolConstant();
		tsfTimestampFormatCheck.SetFormatString(sTimestampFormat.GetValue());

		// Erreur si invalide
		if (not tsfTimestampFormatCheck.Check())
		{
			AddError(sTmp + "Invalid timestamp format parameter (" + sTimestampFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRAsTimestamp::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de timestamp
	tsfTimestampFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tsfTimestampFormat.Check());
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFormatTimestamp::KWDRFormatTimestamp()
{
	SetName("FormatTimestamp");
	SetLabel("Format a timestamp into a categorical value using a timestamp format");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Timestamp);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFormatTimestamp::~KWDRFormatTimestamp() {}

KWDerivationRule* KWDRFormatTimestamp::Create() const
{
	return new KWDRFormatTimestamp;
}

Symbol KWDRFormatTimestamp::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sTimestamp;
	Timestamp tsTimestamp;

	require(IsCompiled());

	// Acces a l'operande valeur de timestamp
	tsTimestamp = GetOperandAt(0)->GetTimestampValue(kwoObject);

	// Formatage de la timestamp
	sTimestamp = tsfTimestampFormat.TimestampToString(tsTimestamp);
	return sTimestamp;
}

boolean KWDRFormatTimestamp::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimestampFormat;
	KWTimestampFormat tsfTimestampFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de timestamp
		sTimestampFormat = GetSecondOperand()->GetSymbolConstant();
		tsfTimestampFormatCheck.SetFormatString(sTimestampFormat.GetValue());

		// Erreur si invalide
		if (not tsfTimestampFormatCheck.Check())
		{
			AddError(sTmp + "Invalid timestamp format parameter (" + sTimestampFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRFormatTimestamp::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de timestamp
	tsfTimestampFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tsfTimestampFormat.Check());
}

longint KWDRFormatTimestamp::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRFormatTimestamp) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAsTimestampTZ::KWDRAsTimestampTZ()
{
	SetName("AsTimestampTZ");
	SetLabel("Recode a categorical value into a timestampTZ using a timestampTZ format");
	SetType(KWType::TimestampTZ);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRAsTimestampTZ::~KWDRAsTimestampTZ() {}

KWDerivationRule* KWDRAsTimestampTZ::Create() const
{
	return new KWDRAsTimestampTZ;
}

TimestampTZ KWDRAsTimestampTZ::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	Symbol sTimestampTZ;
	TimestampTZ tsTimestampTZ;

	require(IsCompiled());

	// Acces a l'operande valeur de timestampTZ
	sTimestampTZ = GetOperandAt(0)->GetSymbolValue(kwoObject);

	// Conversion en timestampTZ
	tsTimestampTZ = tstzfTimestampTZFormat.StringToTimestampTZ(sTimestampTZ);

	// Emission d'une erreur si demande et si transcodage d'un attribut natif
	if (oErrorSender != NULL and GetOperandAt(0)->GetOrigin() == KWDerivationRuleOperand::OriginAttribute and
	    GetOperandAt(0)->GetDerivationRule() == NULL and not sTimestampTZ.IsEmpty() and not tsTimestampTZ.Check())
	{
		oErrorSender->AddWarning("Dictionary " + kwoObject->GetClass()->GetName() + ", rule " + GetName() +
					 "(" + GetOperandAt(0)->GetAttributeName() + ", \"" +
					 GetOperandAt(1)->GetSymbolConstant() + "\")" + ": value <" +
					 InputBufferedFile::GetDisplayValue(sTimestampTZ.GetValue()) +
					 "> converted to <> (invalid timestampTZ)");
	}
	return tsTimestampTZ;
}

boolean KWDRAsTimestampTZ::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimestampTZFormat;
	KWTimestampTZFormat tsfTimestampTZFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de timestampTZ
		sTimestampTZFormat = GetSecondOperand()->GetSymbolConstant();
		tsfTimestampTZFormatCheck.SetFormatString(sTimestampTZFormat.GetValue());

		// Erreur si invalide
		if (not tsfTimestampTZFormatCheck.Check())
		{
			AddError(sTmp + "Invalid timestampTZ format parameter (" + sTimestampTZFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRAsTimestampTZ::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de timestampTZ
	tstzfTimestampTZFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tstzfTimestampTZFormat.Check());
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRFormatTimestampTZ::KWDRFormatTimestampTZ()
{
	SetName("FormatTimestampTZ");
	SetLabel("Format a timestampTZ into a categorical value using a timestampTZ format");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFormatTimestampTZ::~KWDRFormatTimestampTZ() {}

KWDerivationRule* KWDRFormatTimestampTZ::Create() const
{
	return new KWDRFormatTimestampTZ;
}

Symbol KWDRFormatTimestampTZ::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sTimestampTZ;
	TimestampTZ tsTimestampTZ;

	require(IsCompiled());

	// Acces a l'operande valeur de timestampTZ
	tsTimestampTZ = GetOperandAt(0)->GetTimestampTZValue(kwoObject);

	// Formatage de la timestampTZ
	sTimestampTZ = tstzfTimestampTZFormat.TimestampTZToString(tsTimestampTZ);
	return sTimestampTZ;
}

boolean KWDRFormatTimestampTZ::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Symbol sTimestampTZFormat;
	KWTimestampTZFormat tsfTimestampTZFormatCheck;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de l'operande de format
	if (bOk)
	{
		// Parametrage d'un format de timestampTZ
		sTimestampTZFormat = GetSecondOperand()->GetSymbolConstant();
		tsfTimestampTZFormatCheck.SetFormatString(sTimestampTZFormat.GetValue());

		// Erreur si invalide
		if (not tsfTimestampTZFormatCheck.Check())
		{
			AddError(sTmp + "Invalid timestampTZ format parameter (" + sTimestampTZFormat + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRFormatTimestampTZ::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du format de timestampTZ
	tstzfTimestampTZFormat.SetFormatString(GetSecondOperand()->GetSymbolConstant().GetValue());
	assert(tstzfTimestampTZFormat.Check());
}

longint KWDRFormatTimestampTZ::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRFormatTimestampTZ) - sizeof(KWDerivationRule);
	return lUsedMemory;
}
