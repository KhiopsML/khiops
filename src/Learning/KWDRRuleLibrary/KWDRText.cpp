// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRText.h"

void KWDRRegisterTextRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTextLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTranslate);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexMatch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextToUpper);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextToLower);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextConcat);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextHash);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextEncrypt);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextLength::KWDRTextLength()
{
	TransformSymbolToTextRule();
}

KWDRTextLength::~KWDRTextLength() {}

KWDerivationRule* KWDRTextLength::Create() const
{
	return new KWDRTextLength;
}

KWDRTextLeft::KWDRTextLeft()
{
	TransformSymbolToTextRule();
}

KWDRTextLeft::~KWDRTextLeft() {}

KWDerivationRule* KWDRTextLeft::Create() const
{
	return new KWDRTextLeft;
}

KWDRTextRight::KWDRTextRight()
{
	TransformSymbolToTextRule();
}

KWDRTextRight::~KWDRTextRight() {}

KWDerivationRule* KWDRTextRight::Create() const
{
	return new KWDRTextRight;
}

KWDRTextMiddle::KWDRTextMiddle()
{
	TransformSymbolToTextRule();
}

KWDRTextMiddle::~KWDRTextMiddle() {}

KWDerivationRule* KWDRTextMiddle::Create() const
{
	return new KWDRTextMiddle;
}

KWDRTextTokenLength::KWDRTextTokenLength()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenLength::~KWDRTextTokenLength() {}

KWDerivationRule* KWDRTextTokenLength::Create() const
{
	return new KWDRTextTokenLength;
}

KWDRTextTokenLeft::KWDRTextTokenLeft()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenLeft::~KWDRTextTokenLeft() {}

KWDerivationRule* KWDRTextTokenLeft::Create() const
{
	return new KWDRTextTokenLeft;
}

KWDRTextTokenRight::KWDRTextTokenRight()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenRight::~KWDRTextTokenRight() {}

KWDerivationRule* KWDRTextTokenRight::Create() const
{
	return new KWDRTextTokenRight;
}

KWDRTextTokenMiddle::KWDRTextTokenMiddle()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenMiddle::~KWDRTextTokenMiddle() {}

KWDerivationRule* KWDRTextTokenMiddle::Create() const
{
	return new KWDRTextTokenMiddle;
}

KWDRTextTranslate::KWDRTextTranslate()
{
	TransformSymbolToTextRule();
}

KWDRTextTranslate::~KWDRTextTranslate() {}

KWDerivationRule* KWDRTextTranslate::Create() const
{
	return new KWDRTextTranslate;
}

KWDRTextSearch::KWDRTextSearch()
{
	TransformSymbolToTextRule();
}

KWDRTextSearch::~KWDRTextSearch() {}

KWDerivationRule* KWDRTextSearch::Create() const
{
	return new KWDRTextSearch;
}

KWDRTextReplace::KWDRTextReplace()
{
	TransformSymbolToTextRule();
}

KWDRTextReplace::~KWDRTextReplace() {}

KWDerivationRule* KWDRTextReplace::Create() const
{
	return new KWDRTextReplace;
}

KWDRTextReplaceAll::KWDRTextReplaceAll()
{
	TransformSymbolToTextRule();
}

KWDRTextReplaceAll::~KWDRTextReplaceAll() {}

KWDerivationRule* KWDRTextReplaceAll::Create() const
{
	return new KWDRTextReplaceAll;
}

KWDRTextRegexMatch::KWDRTextRegexMatch()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexMatch::~KWDRTextRegexMatch() {}

KWDerivationRule* KWDRTextRegexMatch::Create() const
{
	return new KWDRTextRegexMatch;
}

KWDRTextRegexSearch::KWDRTextRegexSearch()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexSearch::~KWDRTextRegexSearch() {}

KWDerivationRule* KWDRTextRegexSearch::Create() const
{
	return new KWDRTextRegexSearch;
}

KWDRTextRegexReplace::KWDRTextRegexReplace()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexReplace::~KWDRTextRegexReplace() {}

KWDerivationRule* KWDRTextRegexReplace::Create() const
{
	return new KWDRTextRegexReplace;
}

KWDRTextRegexReplaceAll::KWDRTextRegexReplaceAll()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexReplaceAll::~KWDRTextRegexReplaceAll() {}

KWDerivationRule* KWDRTextRegexReplaceAll::Create() const
{
	return new KWDRTextRegexReplaceAll;
}

KWDRTextToUpper::KWDRTextToUpper()
{
	TransformSymbolToTextRule();
}

KWDRTextToUpper::~KWDRTextToUpper() {}

KWDerivationRule* KWDRTextToUpper::Create() const
{
	return new KWDRTextToUpper;
}

KWDRTextToLower::KWDRTextToLower()
{
	TransformSymbolToTextRule();
}

KWDRTextToLower::~KWDRTextToLower() {}

KWDerivationRule* KWDRTextToLower::Create() const
{
	return new KWDRTextToLower;
}

KWDRTextHash::KWDRTextHash()
{
	TransformSymbolToTextRule();
}

KWDRTextHash::~KWDRTextHash() {}

KWDerivationRule* KWDRTextHash::Create() const
{
	return new KWDRTextHash;
}

KWDRTextConcat::KWDRTextConcat()
{
	TransformSymbolToTextRule();
}

KWDRTextConcat::~KWDRTextConcat() {}

KWDerivationRule* KWDRTextConcat::Create() const
{
	return new KWDRTextConcat;
}

KWDRTextEncrypt::KWDRTextEncrypt()
{
	TransformSymbolToTextRule();
}

KWDRTextEncrypt::~KWDRTextEncrypt() {}

KWDerivationRule* KWDRTextEncrypt::Create() const
{
	return new KWDRTextEncrypt;
}
