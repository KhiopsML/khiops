// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTextualAnalysisPROTO.h"

void KWDRRegisterTextualAnalysisPROTORules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenize);
	KWDerivationRule::RegisterDerivationRule(new KWDRNGramCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRStudyNGramCounts);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTokenize::KWDRTokenize()
{
	SetName("Tokenize");
	SetLabel("Recode a catagorical value with tokens separated by blank chars");
	SetType(KWType::Text);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);
}

KWDRTokenize::~KWDRTokenize() {}

KWDerivationRule* KWDRTokenize::Create() const
{
	return new KWDRTokenize;
}

Symbol KWDRTokenize::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sInputValue;
	const char* sStringValue;
	char cChar;
	ALString sOutputValue;
	boolean bEnd;
	boolean bNewToken;

	// Recherche des parametres
	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);

	// Acces a la chaine de caractere a analyser
	sStringValue = sInputValue.GetValue();

	// Parcours des caracteres de la chaines pour extraire les tokens
	bEnd = false;
	bNewToken = true;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;

		// Ajout d'un caractere s'il est alphabetique
		if (isalpha(cChar))
		{
			// Ajout d'un caractere blanc si necessaire
			if (bNewToken and sOutputValue.GetLength() > 0)
				sOutputValue += ' ';
			bNewToken = false;

			// Ajout du caractere
			sOutputValue += cChar;
		}
		else
			bNewToken = true;
	}

	// On renvoie la valeur calculee
	return (Symbol)sOutputValue;
}

KWDRTokenCounts::KWDRTokenCounts()
{
	SetName("TokenCounts");
	SetLabel("Token counts from a categorical value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRTokenCounts::~KWDRTokenCounts() {}

int KWDRTokenCounts::GetVarKeyType() const
{
	return KWType::Symbol;
}

KWDerivationRule* KWDRTokenCounts::Create() const
{
	return new KWDRTokenCounts;
}

KWContinuousValueBlock*
KWDRTokenCounts::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						   const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	sInputValue = GetFirstOperand()->GetSymbolValue(kwoObject);
	cvbTokenCounts = ComputeTokenCounts(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRTokenCounts::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

KWContinuousValueBlock* KWDRTokenCounts::ComputeTokenCounts(const Symbol& sValue,
							    const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	KWContinuousValueDictionary cvdTokenCounts;
	const char* sStringValue;
	char cChar;
	ALString sToken;
	Symbol sAttributeKey;
	boolean bEnd;

	require(indexedKeyBlock != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sValue.GetValue();

	// Parcours des caracteres de la chaines pour extraire les tokens
	bEnd = false;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;

		// Ajout d'un caractere s'il n'est pas blanc
		if (not iswspace(cChar) and not bEnd)
			sToken += cChar;
		// Sinon, fin du token
		else
		{
			// On traite les token non vides
			if (sToken.GetLength() > 0)
			{
				// Incrementation du compte lie au token
				sAttributeKey = Symbol(sToken);
				if (cast(KWIndexedCKeyBlock*, indexedKeyBlock)->IsKeyPresent(sAttributeKey))
					cvdTokenCounts.UpgradeAt(sAttributeKey, 1);

				// Reinitialisation du token, sans desallouer sa memoire
				sToken.GetBufferSetLength(0);
			}
		}
	}

	// Construction du bloc de compte
	cvbTokenCounts = KWContinuousValueBlock::BuildBlockFromValueDictionary(
	    cast(KWIndexedCKeyBlock*, indexedKeyBlock), &cvdTokenCounts);
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	return cvbTokenCounts;
}

longint KWDRTokenCounts::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRTokenCounts);
	return lUsedMemory;
}

void KWDRTokenCounts::Test()
{
	KWDRTokenCounts tokenCounts;
	KWIndexedCKeyBlock indexedKeyBlock;

	// Initialisation des tokens a compter, dans l'ordre alphabetique impose
	indexedKeyBlock.AddKey("bonjour");
	indexedKeyBlock.AddKey("le");
	indexedKeyBlock.AddKey("monde");
	indexedKeyBlock.AddKey("tout");
	cout << "Tokens: " << indexedKeyBlock << endl;

	// Test avec plusieurs chaines
	indexedKeyBlock.IndexKeys();
	tokenCounts.TestCount("bonjour", &indexedKeyBlock);
	tokenCounts.TestCount("le monde est grand", &indexedKeyBlock);
	tokenCounts.TestCount("coucou", &indexedKeyBlock);
	tokenCounts.TestCount("le soir et le matin", &indexedKeyBlock);
}

void KWDRTokenCounts::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeTokenCounts(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
	delete valueBlock;
}

//////////////////////////////////////////////////////////////////////////////////////

LongintVector KWDRNGramCounts::lvNGramMasks;

KWDRNGramCounts::KWDRNGramCounts()
{
	SetName("NGramCounts");
	SetLabel("N-gram counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Text);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);

	// On impose que les deux derniers operandes soient des constantes
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);

	// Initialisation par defaut des caracteristique des n-grammes
	nFirstNGramLength = 0;
	nLastNGramLength = 0;
	nNGramNumber = 0;
	lRandomOffset = 0;
	bWithRandomSign = false;
}

KWDRNGramCounts::~KWDRNGramCounts() {}

int KWDRNGramCounts::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRNGramCounts::Create() const
{
	return new KWDRNGramCounts;
}

KWContinuousValueBlock*
KWDRNGramCounts::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						   const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);
	cvbTokenCounts = ComputeCharNgramCounts(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRNGramCounts::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

void KWDRNGramCounts::Compile(KWClass* kwcOwnerClass)
{
	int i;
	int nLength;
	longint lNGramMask;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation des caracteristiques des n-grammes
	nNGramNumber = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));
	nFirstNGramLength = 1;
	if (GetOperandNumber() >= 2)
		nFirstNGramLength = int(floor(GetOperandAt(2)->GetContinuousConstant() + 0.5));
	assert(1 <= nFirstNGramLength and nFirstNGramLength <= nMaxNGramLength);

	// Premiere taille de n-gramme a prendre en compte
	nLastNGramLength = nFirstNGramLength;

	// Calcul des masques par nGramme: on met tous les bits a 1 partout sur la longueur des n-grammes
	if (lvNGramMasks.GetSize() == 0)
	{
		lvNGramMasks.SetSize(nMaxNGramLength + 1);
		for (nLength = 1; nLength <= nMaxNGramLength; nLength++)
		{
			lNGramMask = 0;
			for (i = 0; i < nLength; i++)
			{
				lNGramMask <<= 8;
				lNGramMask += 255;
			}
			lvNGramMasks.SetAt(nLength, lNGramMask);
		}
	}

	// Calcul du decallage pour generer des hashages differents selon les caracteristiques des nGrammes
	lRandomOffset = nLastNGramLength;
	lRandomOffset *= 1000000000;
	lRandomOffset += nNGramNumber;
	lRandomOffset *= 1000000000;
	assert(lRandomOffset >= 0);

	// Reinitialisation du vecteur de travail sparse des comptes
	// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
	lvSparseCounts.SetSize(0);
}

boolean KWDRNGramCounts::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	double dValue;
	int nValue;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des operandes
	if (bOk)
	{
		assert(GetOperandNumber() >= 2);

		// Operande de la taille de la table de hashage, en deuxieme operande
		dValue = GetOperandAt(1)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Size of hashtable (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 2 must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Size of hashtable (" + IntToString(nValue) +
				 ") in operand 2 must be at least 1");
			bOk = false;
		}
		else if (nValue > 1000000)
		{
			AddError(sTmp + "Number of n-grams (" + IntToString(nValue) +
				 ") in operand 2 must be at most 1000000");
			bOk = false;
		}

		// Operande de longueur des n-grammes, si present en troisieme operande
		if (bOk and GetOperandNumber() >= 3)
		{
			dValue = GetOperandAt(2)->GetContinuousConstant();
			bOk = KWContinuous::ContinuousToInt(dValue, nValue);
			if (not bOk)
			{
				AddError(sTmp + "Length of n-grams (" + KWContinuous::ContinuousToString(dValue) +
					 ") in operand 3 must be an integer");
				assert(bOk == false);
			}
			else if (nValue < 1)
			{
				AddError(sTmp + "Length of n-grams (" + IntToString(nValue) +
					 ") in operand 3  must be at least 1");
				bOk = false;
			}
			else if (nValue > nMaxNGramLength)
			{
				AddError(sTmp + "Length of n-grams (" + IntToString(nValue) +
					 ") in operand 3 must be at most " + IntToString(nMaxNGramLength));
				bOk = false;
			}
		}
	}
	return bOk;
}

boolean KWDRNGramCounts::CheckBlockAttributes(const KWClass* kwcOwnerClass,
					      const KWAttributeBlock* attributeBlock) const
{
	boolean bResult = true;
	int nNumber;
	KWAttribute* checkedAttribute;
	int nVarKey;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute()->GetParentClass()->GetDomain() == kwcOwnerClass->GetDomain());
	require(CheckOperandsCompleteness(kwcOwnerClass));
	require(attributeBlock->GetVarKeyType() == KWType::Continuous);

	// Appel de la methode ancetre
	bResult = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Analyse des cles du bloc d'attribut par rapport a la partition
	if (bResult)
	{
		assert(GetOperandNumber() >= 2);

		// Acces au nombre de ngrammes, en deuxieme operande
		nNumber = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey plus grand que le nombre de n-grammes
			if (nVarKey > nNumber)
			{
				// Messages d'erreur
				attributeBlock->AddError(
				    sTmp + "Variable " + checkedAttribute->GetName() +
				    +" has its VarKey=" + IntToString(nVarKey) +
				    " greater than the total number of n-grams (" + IntToString(nNumber) +
				    ") in the n-gram block (third operand of rule " + GetName() + ")");
				bResult = false;
				break;
			}

			// Arret si derniere variable du bloc trouvee
			if (checkedAttribute == attributeBlock->GetLastAttribute())
				break;
			// Sinon, attribut suivant
			else
				checkedAttribute->GetParentClass()->GetNextAttribute(checkedAttribute);
		}
	}
	return bResult;
}

longint KWDRNGramCounts::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRNGramCounts);
	lUsedMemory += lvSparseCounts.GetUsedMemory() - sizeof(LongintVector);
	return lUsedMemory;
}

KWContinuousValueBlock* KWDRNGramCounts::ComputeCharNgramCounts(const Symbol& sValue,
								const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const boolean bDisplay = false;
	static const int nDefaultCount = -INT_MAX;
	KWContinuousValueBlock* cvbTokenCounts;
	IntVector ivUsedSparseIndexes;
	const char* sStringValue;
	int nTextLength;
	int n;
	int nMax;
	int nNGramLength;
	longint lNGramMask;
	longint lNgramValue;
	longint lIndex;
	int nRandom;
	int nNKey;
	int nRandomSign;
	int nSparseIndex;
	char* sIndex;
	int i;
	int nPreviousSize;

	require(indexedKeyBlock != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sValue.GetValue();
	if (bDisplay)
		cout << GetName() << "\t" << sStringValue << endl;

	// Retaillage si necessaire du tableau sparse des parties
	if (lvSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
	{
		nPreviousSize = lvSparseCounts.GetSize();
		lvSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

		// Initialisation des nouvelles valeurs avec la valeur par defaut
		for (i = nPreviousSize; i < indexedKeyBlock->GetKeyNumber(); i++)
			lvSparseCounts.SetAt(i, nDefaultCount);
	}

	// Parcours des caracteres de la chaines pour en calculer les ngrammes
	// En debut de chaine et en fin de chaine, on ne prend qu'une sous-partie
	// des caracteres du n-gramme
	// Par exemple, pour les 3-grammes de la chaine "Bonjour", on considere
	// "..B", ".Bo", "Bon", "onj", "njo", "jou", "our", "ur.", "r.."
	nTextLength = sValue.GetLength();
	// Cas ou la chaine est vide: aucun n-gramme
	if (nTextLength == 0)
	{
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(0);
	}
	// Cas general ou la chaine est non vide
	// On prend les caractere un a un pour composer un index, en multipliant par 256 (<<8) apres chaque caractere
	// On aurait pu utiliser des memcpy pour recopier directement les caracteres dans l'index (longint)
	// traite comme un tableau de 8 caracteres. Le probleme est que cela depend de l'ordre des octets utilise
	// par le systeme pour stocker les entiers (endianness), ce qui rendrait la solution non portable
	// (windows et linux sont en little endian, mais ARM et android pas forcement)
	else
	{
		// On boucle sur les longueurs de nGrammes
		nRandomSign = 1;
		for (nNGramLength = nFirstNGramLength; nNGramLength <= nLastNGramLength; nNGramLength++)
		{
			lNgramValue = 0;
			nMax = nTextLength + nNGramLength - 1;
			lNGramMask = lvNGramMasks.GetAt(nNGramLength);
			for (n = 0; n < nMax; n++)
			{
				// On multiplie l'index courant par 256
				lNgramValue <<= 8;

				// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
				if (n < nTextLength)
					lNgramValue += (unsigned char)sStringValue[n];

				// On tronque a la longueur du pattern en appliquant le masque
				lNgramValue &= lNGramMask;

				// On calcul un index a partir de la valeur du n-gramme, en ajoutant un offset
				// pseudo-aleatoire
				lIndex = lNgramValue + lRandomOffset;

				// On bascule en index positif, necessaire pour l'appel a IthRandomInt
				// N'est en principe necessaire que dans de rare cas, uniquement pour les longueurs
				// maximum des n-grammes qui atteignent la limite de taille des longint
				assert(lIndex >= 0 or nNGramLength == sizeof(longint));
				if (lIndex < 0)
					lIndex = -lIndex;

				// Calcul d'une cle de n-gram projete aleatoiremenent sur [1; nNGrammNumber]
				nRandom = IthRandomInt(lIndex, 2 * (nNGramNumber - 1));
				nNKey = 1 + nRandom / 2;
				assert(1 <= nNKey and nNKey <= nNGramNumber);

				// Prise en compte d'un signe aleatoire
				if (bWithRandomSign)
				{
					nRandomSign = 2 * (nRandom % 2) - 1;
					assert(nRandomSign == -1 or nRandomSign == 1);
				}

				// Recherche de l'index sparse correspondant
				nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

				// Test s'il faut memoriser le compte du n-gramme
				if (nSparseIndex != -1)
				{
					// Memorisation si necessaire de l'index sparse utilise
					if (lvSparseCounts.GetAt(nSparseIndex) == nDefaultCount)
					{
						lvSparseCounts.SetAt(nSparseIndex, 0);
						ivUsedSparseIndexes.Add(nSparseIndex);
					}

					// Mise a jour du compte du ngramme
					lvSparseCounts.UpgradeAt(nSparseIndex, nRandomSign);
				}

				// Affichage des resultats intermediaires, pour la mise au point
				if (bDisplay)
				{
					cout << "\t" << n << "\t";

					// Affichage du n-gramme collecte dans la valeur lNgramValue
					sIndex = (char*)&lNgramValue;
					cout << "\t";
					for (i = 7; i >= 0; i--)
					{
						if (sIndex[i] != 0)
							cout << sIndex[i];
					}

					// Affichage des varaiables de calcul
					cout << "\t" << lNgramValue << "\t" << lIndex << "\t" << nNKey << "\t"
					     << nSparseIndex << endl;
				}
			}
		}

		// Tri des index sparses de parties utilisees
		ivUsedSparseIndexes.Sort();

		// Construction du bloc de compte de la bonne taille
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(ivUsedSparseIndexes.GetSize());

		// Initialisation du vecteur sparse
		// On exploite ici le fait que les index sparse sont necessairement ordonnes de la
		// meme facon que les index de parties (NKey)
		for (n = 0; n < ivUsedSparseIndexes.GetSize(); n++)
		{
			nSparseIndex = ivUsedSparseIndexes.GetAt(n);

			// Memorisation de la paire (index sparse, compte)
			cvbTokenCounts->SetAttributeSparseIndexAt(n, nSparseIndex);
			cvbTokenCounts->SetValueAt(n, (Continuous)lvSparseCounts.GetAt(nSparseIndex));
			assert(n == 0 or cvbTokenCounts->GetAttributeSparseIndexAt(n) >
					     cvbTokenCounts->GetAttributeSparseIndexAt(n - 1));

			// Reinitialisation du tableau de travail
			lvSparseCounts.SetAt(nSparseIndex, nDefaultCount);
		}
	}
	ensure(cvbTokenCounts->SearchValueIndex(nDefaultCount) == -1);
	return cvbTokenCounts;
}

////////////////////////////////////////////////////////////////////////////

KWDRStudyNGramCounts::KWDRStudyNGramCounts()
{
	SetName("StudyNGramCounts");
	SetLabel("Study n-gram counts from a text value");
	AddOperand(new KWDerivationRuleOperand);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(3)->SetType(KWType::Continuous);
	GetOperandAt(4)->SetType(KWType::Continuous);

	// On impose que les deux derniers operandes soient des constantes
	GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(4)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRStudyNGramCounts::~KWDRStudyNGramCounts() {}

KWDerivationRule* KWDRStudyNGramCounts::Create() const
{
	return new KWDRStudyNGramCounts;
}

void KWDRStudyNGramCounts::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDRNGramCounts::Compile(kwcOwnerClass);

	// Parametrage avance
	nLastNGramLength = int(floor(GetOperandAt(3)->GetContinuousConstant() + 0.5));
	bWithRandomSign = GetOperandAt(4)->GetContinuousConstant() != 0;
}

boolean KWDRStudyNGramCounts::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	double dValue;
	int nValue;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des operandes
	if (bOk)
	{
		assert(GetOperandNumber() == 5);

		// Operande de longueur max des n-grammes
		dValue = GetOperandAt(3)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Max length of n-grams (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 4 must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Max length of n-grams (" + IntToString(nValue) +
				 ") in operand 4  must be at least 1");
			bOk = false;
		}
		else if (nValue > nMaxNGramLength)
		{
			AddError(sTmp + "Max length of n-grams (" + IntToString(nValue) +
				 ") in operand 4 must be at most " + IntToString(nMaxNGramLength));
			bOk = false;
		}
		else if (dValue < GetOperandAt(2)->GetContinuousConstant())
		{
			AddError(sTmp + "Max length of n-grams (" + IntToString(nValue) +
				 ") in operand 4 must be greater than min length of n-grams in operand 3");
			bOk = false;
		}

		// Operande de l'utilisation d'un signe pour l'ajout dans la table de hash
		dValue = GetOperandAt(4)->GetContinuousConstant();
		if (dValue != 0 and dValue != 1)
		{
			AddError(sTmp + "Operand 5 (" + KWContinuous::ContinuousToString(dValue) +
				 ") for using a sign to update the hahtable must be 0 or 1");
			bOk = false;
		}
	}
	return bOk;
}
