// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTextList.h"

void KWDRRegisterTextListRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListSize);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextList);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListConcat);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTextListValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableAllTexts);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableAllTextLists);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextListSize::KWDRTextListSize()
{
	SetName("TextListSize");
	SetLabel("Size of a text list value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListSize::~KWDRTextListSize() {}

KWDerivationRule* KWDRTextListSize::Create() const
{
	return new KWDRTextListSize;
}

Continuous KWDRTextListSize::ComputeContinuousResult(const KWObject* kwoObject) const
{
	SymbolVector* svTextList;

	require(IsCompiled());

	// Nombre de textes dans la liste
	svTextList = GetFirstOperand()->GetTextListValue(kwoObject);
	if (svTextList == NULL)
		return 0;
	else
		return (Continuous)svTextList->GetSize();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextListAt::KWDRTextListAt()
{
	SetName("TextListAt");
	SetLabel("Text value at index");
	SetType(KWType::Text);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::TextList);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTextListAt::~KWDRTextListAt() {}

KWDerivationRule* KWDRTextListAt::Create() const
{
	return new KWDRTextListAt;
}

Symbol KWDRTextListAt::ComputeTextResult(const KWObject* kwoObject) const
{
	SymbolVector* svTextList;
	int nIndex;

	require(IsCompiled());

	// Recherche de la valeur ObjectArray
	svTextList = GetFirstOperand()->GetTextListValue(kwoObject);
	if (svTextList != NULL)
	{
		// Recherche du rang
		nIndex = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

		// On renvoie l'objet si son rang est valide
		if (1 <= nIndex and nIndex <= svTextList->GetSize())
			return svTextList->GetAt(nIndex - 1);
		// On retourne un texte vide sinon
		else
			return Symbol();
	}

	// On renvoie un texte vide si la liste est vide
	return Symbol();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextList::KWDRTextList()
{
	SetName("TextList");
	SetLabel("List of text values");
	SetType(KWType::TextList);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Text);
}

KWDRTextList::~KWDRTextList() {}

KWDerivationRule* KWDRTextList::Create() const
{
	return new KWDRTextList;
}

SymbolVector* KWDRTextList::ComputeTextListResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(IsCompiled());

	// Collecte des textes de la liste
	svResult.SetSize(0);
	svResult.SetSize(GetOperandNumber());
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		svResult.SetAt(nOperand, GetOperandAt(nOperand)->GetTextValue(kwoObject));
	return &svResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextListConcat::KWDRTextListConcat()
{
	SetName("TextListConcat");
	SetLabel("Concatenation of tex list values");
	SetType(KWType::TextList);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListConcat::~KWDRTextListConcat() {}

KWDerivationRule* KWDRTextListConcat::Create() const
{
	return new KWDRTextListConcat;
}

SymbolVector* KWDRTextListConcat::ComputeTextListResult(const KWObject* kwoObject) const
{
	int nOperand;
	SymbolVector* svTextList;
	int nText;

	require(IsCompiled());

	// Collecte des textes de la liste
	svResult.SetSize(0);
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		svTextList = GetOperandAt(nOperand)->GetTextListValue(kwoObject);
		if (svTextList != NULL)
		{
			for (nText = 0; nText < svTextList->GetSize(); nText++)
				svResult.Add(svTextList->GetAt(nText));
		}
	}
	return &svResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRGetTextListValue::KWDRGetTextListValue()
{
	SetName("GetTextList");
	SetLabel("Text list value in a sub-entity");
	SetType(KWType::TextList);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::TextList);
}

KWDRGetTextListValue::~KWDRGetTextListValue() {}

KWDerivationRule* KWDRGetTextListValue::Create() const
{
	return new KWDRGetTextListValue;
}

SymbolVector* KWDRGetTextListValue::ComputeTextListResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	SymbolVector* svTextList;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Object dans le sous-objet
	svResult.SetSize(0);
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject != NULL)
	{
		valueOperand = GetSecondOperand();
		svTextList = valueOperand->GetTextListValue(kwoContainedObject);
		if (svTextList != NULL)
			svResult.CopyFrom(svTextList);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &svResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTableAllTexts::KWDRTableAllTexts()
{
	SetName("TableAllTexts");
	SetLabel("Concatenation of text values in a table");
	SetType(KWType::TextList);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Text);
}

KWDRTableAllTexts::~KWDRTableAllTexts() {}

KWDerivationRule* KWDRTableAllTexts::Create() const
{
	return new KWDRTableAllTexts;
}

SymbolVector* KWDRTableAllTexts::ComputeTextListResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sText;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche des valeurs du container
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	svResult.SetSize(0);
	if (oaObjects != NULL and oaObjects->GetSize() > 0)
	{
		// Parcours des objets du container
		svResult.SetSize(oaObjects->GetSize());
		valueOperand = GetSecondOperand();
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation du texte
			sText = valueOperand->GetTextValue(kwoContainedObject);
			svResult.SetAt(nObject, sText);
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &svResult;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTableAllTextLists::KWDRTableAllTextLists()
{
	SetName("TableAllTextLists");
	SetLabel("Concatenation of text list values in a table");
	SetType(KWType::TextList);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::TextList);
}

KWDRTableAllTextLists::~KWDRTableAllTextLists() {}

KWDerivationRule* KWDRTableAllTextLists::Create() const
{
	return new KWDRTableAllTextLists;
}

SymbolVector* KWDRTableAllTextLists::ComputeTextListResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	SymbolVector* svTextList;
	int nText;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche des valeurs du container
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	svResult.SetSize(0);
	if (oaObjects != NULL and oaObjects->GetSize() > 0)
	{
		// Parcours des objets du container
		valueOperand = GetSecondOperand();
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation des textes de la liste de texte obtenue
			svTextList = valueOperand->GetTextListValue(kwoContainedObject);
			if (svTextList != NULL)
			{
				for (nText = 0; nText < svTextList->GetSize(); nText++)
					svResult.Add(svTextList->GetAt(nText));
			}
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &svResult;
}
