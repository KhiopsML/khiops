// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTokenCounts.h"

class KWDRTokenize;
class KWDRTokenCounts;
class KWDRCharNGramCounts;
class KWDRStudyCharNGramCounts;
class KWDRTextInit;

void KWDRRegisterTextAdvancedRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenize);
	if (GetLearningTextVariableMode())
	{
		KWDerivationRule::RegisterDerivationRule(new KWDRMultipleCharNGramCounts);
		KWDerivationRule::RegisterDerivationRule(new KWDRCharNGramCounts);
		KWDerivationRule::RegisterDerivationRule(new KWDRStudyCharNGramCounts);
		KWDerivationRule::RegisterDerivationRule(new KWDRTextInit);
	}
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

LongintVector KWDRCharNGramCounts::lvNGramMasks;

KWDRCharNGramCounts::KWDRCharNGramCounts()
{
	SetName("CharNGramCounts");
	SetLabel("Char n-gram counts from a text value");
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

KWDRCharNGramCounts::~KWDRCharNGramCounts() {}

int KWDRCharNGramCounts::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRCharNGramCounts::Create() const
{
	return new KWDRCharNGramCounts;
}

KWContinuousValueBlock*
KWDRCharNGramCounts::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
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

Continuous KWDRCharNGramCounts::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

KWContinuousValueBlock* KWDRCharNGramCounts::ComputeCharNgramCounts(const Symbol& sValue,
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
	if (ivSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
	{
		nPreviousSize = ivSparseCounts.GetSize();
		ivSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

		// Initialisation des nouvelles valeur avec la valeur par defaut
		for (i = nPreviousSize; i < indexedKeyBlock->GetKeyNumber(); i++)
			ivSparseCounts.SetAt(i, nDefaultCount);
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
					if (ivSparseCounts.GetAt(nSparseIndex) == nDefaultCount)
					{
						ivSparseCounts.SetAt(nSparseIndex, 0);
						ivUsedSparseIndexes.Add(nSparseIndex);
					}

					// Mise a jour du compte du ngramme
					ivSparseCounts.UpgradeAt(nSparseIndex, nRandomSign);
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
			cvbTokenCounts->SetValueAt(n, ivSparseCounts.GetAt(nSparseIndex));
			assert(n == 0 or cvbTokenCounts->GetAttributeSparseIndexAt(n) >
					     cvbTokenCounts->GetAttributeSparseIndexAt(n - 1));

			// Reinitialisation du tableau de travail
			ivSparseCounts.SetAt(nSparseIndex, nDefaultCount);
		}
	}
	ensure(cvbTokenCounts->SearchValueIndex(nDefaultCount) == -1);
	return cvbTokenCounts;
}

void KWDRCharNGramCounts::Compile(KWClass* kwcOwnerClass)
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
	ivSparseCounts.SetSize(0);
}

boolean KWDRCharNGramCounts::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
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

boolean KWDRCharNGramCounts::CheckBlockAttributes(const KWClass* kwcOwnerClass,
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

longint KWDRCharNGramCounts::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRCharNGramCounts);
	lUsedMemory += ivSparseCounts.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////

IntVector KWDRMultipleCharNGramCounts::ivNGramLengths;
IntVector KWDRMultipleCharNGramCounts::ivHashTableSizes;
IntVector KWDRMultipleCharNGramCounts::ivHashTableCumulatedSizes;
LongintVector KWDRMultipleCharNGramCounts::lvNGramMasks;

KWDRMultipleCharNGramCounts::KWDRMultipleCharNGramCounts()
{
	SetName("MultipleCharNGramCounts");
	SetLabel("Multiple char n-gram counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(2);

	// Premier operande de type texte
	GetFirstOperand()->SetType(KWType::Text);

	// Deuxieme operande: taille du block
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);

	// Parametres de la regle
	nUsedHashTableNumber = 0;

	// Initialisation des variables globales
	InitializeGlobalVariables();
}

KWDRMultipleCharNGramCounts::~KWDRMultipleCharNGramCounts() {}

int KWDRMultipleCharNGramCounts::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRMultipleCharNGramCounts::Create() const
{
	return new KWDRMultipleCharNGramCounts;
}

KWContinuousValueBlock*
KWDRMultipleCharNGramCounts::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
							       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);
	cvbTokenCounts = ComputeAllTablesCharNgramCounts(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRMultipleCharNGramCounts::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

KWContinuousValueBlock*
KWDRMultipleCharNGramCounts::ComputeAllTablesCharNgramCounts(const Symbol& sValue,
							     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const boolean bDisplay = false;
	int nBlockSize;
	KWContinuousValueBlock* cvbTokenCounts;
	IntVector ivUsedSparseIndexes;
	const char* sStringValue;
	int nTextLength;
	int nLastNGramLength;
	int nHashTableIndex;
	int nHashTableFirstIndex;
	int nHashTableLastIndex;
	int n;
	int nNGramLength;
	longint lNGramMask;
	longint lNgramValue;
	longint lRandom;
	longint lCukooHash;
	int nNGramStartNKey;
	int nStartNKey;
	int nNKey;
	int nSparseIndex;
	char* sIndex;
	int i;

	require(AreGlobalVariablesInitialized());
	require(indexedKeyBlock != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sValue.GetValue();
	if (bDisplay)
	{
		cout << GetName() << "\t" << sStringValue << endl;
		cout << "\tLength\tSize\tn\tngram\tindex\thash\tkey\tsparse index\tcount" << endl;
	}

	// Taille du texte
	nTextLength = sValue.GetLength();

	// Acces a la taille du block de variable
	nBlockSize = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

	// Retaillage si necessaire du tableau sparse des parties
	if (ivSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
		ivSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

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
		// On boucle sur les longueurs de n-grammes
		nHashTableLastIndex = -1;
		nLastNGramLength = 0;
		if (nUsedHashTableNumber > 0)
			nLastNGramLength = ivNGramLengths.GetAt(nUsedHashTableNumber - 1);
		for (nNGramLength = 1; nNGramLength <= nLastNGramLength; nNGramLength++)
		{
			// Arret si texte trop court
			if (nTextLength < nNGramLength)
				break;

			// Calcul des index de tables de hashage concernees
			nHashTableFirstIndex = nHashTableLastIndex + 1;
			nHashTableLastIndex = nHashTableFirstIndex;
			while (nHashTableLastIndex + 1 < nUsedHashTableNumber and
			       ivNGramLengths.GetAt(nHashTableLastIndex + 1) == nNGramLength)
				nHashTableLastIndex++;

			// Verifications
			assert(ivNGramLengths.GetAt(nHashTableFirstIndex) == nNGramLength);
			assert(nHashTableFirstIndex == 0 or
			       ivNGramLengths.GetAt(nHashTableFirstIndex - 1) < nNGramLength);
			assert(ivNGramLengths.GetAt(nHashTableLastIndex) == nNGramLength);

			// Cle de depart pour les tables pour cette longueur de ngramms
			nNGramStartNKey = 1 + ivHashTableCumulatedSizes.GetAt(nHashTableFirstIndex);

			// Analyse du texte pour une longueur de n-grammes donnee
			lNGramMask = lvNGramMasks.GetAt(nNGramLength);

			// Initialisation du debut premier n-gramme
			lNgramValue = 0;
			for (n = 0; n < nNGramLength - 1; n++)
			{
				// On multiplie l'index courant par 256
				lNgramValue <<= 8;

				// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
				lNgramValue += (unsigned char)sStringValue[n];
			}

			// Parcours des n-grammes jusqu'a la fin du texte
			for (n = nNGramLength - 1; n < nTextLength; n++)
			{
				// On multiplie l'index courant par 256
				lNgramValue <<= 8;

				// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
				lNgramValue += (unsigned char)sStringValue[n];

				// On tronque a la longueur du pattern en appliquant le masque
				lNgramValue &= lNGramMask;
				assert(lNgramValue >= 0);

				// On prend un nombre aleatoire a partir de la valeur de ngramme
				lRandom = IthRandomLongint(lNgramValue);

				// Alimentation des tables de hashage pour cette longueur de ngramme
				lCukooHash = lRandom;
				nStartNKey = nNGramStartNKey;
				for (nHashTableIndex = nHashTableFirstIndex; nHashTableIndex <= nHashTableLastIndex;
				     nHashTableIndex++)
				{
					// Calcul d'une cle de n-gram projetee aleatoirement sur sa table a partir de
					// nStartNKey
					nNKey = abs(lCukooHash) % ivHashTableSizes.GetAt(nHashTableIndex);
					nNKey += nStartNKey;

					// Recherche de l'index sparse correspondant
					nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

					// Test s'il faut memoriser le compte du n-gramme
					if (nSparseIndex != -1)
					{
						assert(nSparseIndex < ivSparseCounts.GetSize());

						// Memorisation si necessaire de l'index sparse utilise
						if (ivSparseCounts.GetAt(nSparseIndex) == 0)
							ivUsedSparseIndexes.Add(nSparseIndex);

						// Mise a jour du compte du ngramme
						ivSparseCounts.UpgradeAt(nSparseIndex, 1);
					}

					// Affichage des resultats intermediaires, pour la mise au point
					if (bDisplay)
					{
						cout << "\t" << nNGramLength << "\t"
						     << ivHashTableSizes.GetAt(nHashTableIndex) << "\t" << n << "\t";

						// Affichage du n-gramme collecte dans la valeur lNgramValue
						sIndex = (char*)&lNgramValue;
						for (i = 7; i >= 0; i--)
						{
							if (sIndex[i] != 0)
								cout << sIndex[i];
						}

						// Affichage des variables de calcul
						cout << "\t" << lNgramValue << "\t#" << lCukooHash << "\t" << nNKey
						     << "\t" << nSparseIndex << "\t";
						if (nSparseIndex != -1)
							cout << ivSparseCounts.GetAt(nSparseIndex);
						cout << "\n";
					}

					// Position de depart pour la table suivante
					nStartNKey += ivHashTableSizes.GetAt(nHashTableIndex);

					// Variante de la valeur hashage pour la table suivante, pour eviter les
					// collisions
					lCukooHash += lNgramValue;
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
			cvbTokenCounts->SetValueAt(n, ivSparseCounts.GetAt(nSparseIndex));
			assert(n == 0 or cvbTokenCounts->GetAttributeSparseIndexAt(n) >
					     cvbTokenCounts->GetAttributeSparseIndexAt(n - 1));

			// Reinitialisation du vecteur de travail
			ivSparseCounts.SetAt(nSparseIndex, 0);
		}
	}

	// Affichage du resultat
	if (bDisplay)
	{
		cout << "Result block:\n";
		for (n = 0; n < cvbTokenCounts->GetValueNumber(); n++)
		{
			cout << "\t " << cvbTokenCounts->GetAttributeSparseIndexAt(n) << "\t";
			cout << KWContinuous::ContinuousToString(cvbTokenCounts->GetValueAt(n)) << "\n";
			if (n > 1000)
			{
				cout << "\t...\n";
				break;
			}
		}
		cout << "\n";
	}
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	ensure(CheckCharNgramCountsValueBlock(sValue, indexedKeyBlock, cvbTokenCounts));
	return cvbTokenCounts;
}

boolean KWDRMultipleCharNGramCounts::CheckCharNgramCountsValueBlock(const Symbol& sValue,
								    const KWIndexedKeyBlock* indexedKeyBlock,
								    const KWContinuousValueBlock* valueBlock) const
{
	boolean bOk = true;
	int nTextLength;
	int nBlockSize;
	boolean bIsFullBlock;
	IntVector ivCumulatedNGramCounts;
	int i;
	int nSparseIndex;
	int nNKey;
	int nHashTableIndex;
	int nNGramLength;
	int nHashTableCumulatedSize;

	require(AreGlobalVariablesInitialized());
	require(indexedKeyBlock != NULL);
	require(valueBlock != NULL);

	// Taille du texte
	nTextLength = sValue.GetLength();

	// Cas particulier d'un chaine vide
	if (nTextLength == 0)
		bOk = valueBlock->GetValueNumber() == 0;
	// Cas d'une chaine non vide
	else
	{
		// Acces a la taille du block de variable
		nBlockSize = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

		// Test si si le block est plein
		bIsFullBlock = (indexedKeyBlock->GetKeyNumber() == nBlockSize);
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(1));
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(nBlockSize));

		// Initialisation d'un vecteur de comptes de n-grammes par taille de table hashage
		ivCumulatedNGramCounts.SetSize(nUsedHashTableNumber);

		// Comptage du nombre cumule de n-grammes par taille de block
		for (i = 0; i < valueBlock->GetValueNumber(); i++)
		{
			// Acces a la cle de n-gramme
			nSparseIndex = valueBlock->GetAttributeSparseIndexAt(i);
			nNKey = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyAt(nSparseIndex);

			// Recherche de l'index de la table de hashage correspondante
			nHashTableCumulatedSize = 0;
			nHashTableIndex = -1;
			while (nNKey > nHashTableCumulatedSize)
			{
				nHashTableIndex++;
				nHashTableCumulatedSize += ivHashTableSizes.GetAt(nHashTableIndex);
			}

			// Mise a jour du compte dans cette table de hashage
			ivCumulatedNGramCounts.UpgradeAt(nHashTableIndex, (int)valueBlock->GetValueAt(i));
		}

		// Verification des comptes cumules de n-grammes
		for (nHashTableIndex = 0; nHashTableIndex < nUsedHashTableNumber; nHashTableIndex++)
		{
			nNGramLength = ivNGramLengths.GetAt(nHashTableIndex);

			// Cas de texte de longueur inferieure aux n-grammes
			if (nTextLength < nNGramLength)
				bOk = bOk and ivCumulatedNGramCounts.GetAt(nHashTableIndex) == 0;
			// Cas d'un comptage exhaustif si le block est plein avant la derniere table
			else if (bIsFullBlock and nHashTableIndex < nUsedHashTableNumber - 1)
				bOk = bOk and
				      ivCumulatedNGramCounts.GetAt(nHashTableIndex) == nTextLength + 1 - nNGramLength;
			// Cas d'un comptage exhaustif si le block est plein pour la derniere table
			else if (bIsFullBlock and nHashTableIndex == nUsedHashTableNumber - 1 and
				 indexedKeyBlock->GetKeyNumber() == (int)(pow(2, nHashTableIndex + 1) - 1))
				bOk = bOk and
				      ivCumulatedNGramCounts.GetAt(nHashTableIndex) == nTextLength + 1 - nNGramLength;
			// Cas general
			else
				bOk = bOk and
				      ivCumulatedNGramCounts.GetAt(nHashTableIndex) <= nTextLength + 1 - nNGramLength;
		}
	}
	return bOk;
}

void KWDRMultipleCharNGramCounts::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation de la regle
	Optimize();

	// Reinitialisation du vecteur de travail sparse des comptes
	// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
	ivSparseCounts.SetSize(0);
}

boolean KWDRMultipleCharNGramCounts::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
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

		// Acces a la taille du block, en deuxieme operande
		dValue = GetOperandAt(1)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Size of variable block (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 2 must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Size of variable block (" + IntToString(nValue) +
				 ") in operand 2 must be at least 1");
			bOk = false;
		}
		else if (nValue > 1000000)
		{
			AddError(sTmp + "Size of variable block (" + IntToString(nValue) +
				 ") in operand 2 must be at most 1000000");
			bOk = false;
		}
	}
	return bOk;
}

boolean KWDRMultipleCharNGramCounts::CheckBlockAttributes(const KWClass* kwcOwnerClass,
							  const KWAttributeBlock* attributeBlock) const
{
	boolean bResult = true;
	int nBlockSize;
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

		// Acces a la taille du block, en deuxieme operande
		nBlockSize = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey plus grand que le nombre de n-grammes
			if (nVarKey > nBlockSize)
			{
				// Messages d'erreur
				attributeBlock->AddError(sTmp + "Variable " + checkedAttribute->GetName() +
							 " has its VarKey=" + IntToString(nVarKey) +
							 " greater than the size of the variable block (" +
							 IntToString(nBlockSize) + ") obtained using rule " +
							 GetName());
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

longint KWDRMultipleCharNGramCounts::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRMultipleCharNGramCounts);
	lUsedMemory += ivSparseCounts.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

int KWDRMultipleCharNGramCounts::GetMaxHashTableNumber()
{
	return ivNGramLengths.GetSize();
}

int KWDRMultipleCharNGramCounts::GetHashTableNGramLengthAt(int nIndex)
{
	return ivNGramLengths.GetAt(nIndex);
}

int KWDRMultipleCharNGramCounts::GetHashTableSizeAt(int nIndex)
{
	return ivHashTableSizes.GetAt(nIndex);
}

void KWDRMultipleCharNGramCounts::Test()
{
	const int nBlockSize = 100000;
	KWDRMultipleCharNGramCounts charNgramCounts;
	KWIndexedNKeyBlock indexedKeyBlock;
	int i;

	// Initialisation de la regle
	charNgramCounts.GetSecondOperand()->SetContinuousConstant(nBlockSize);
	charNgramCounts.Optimize();

	// Initialisation des ngrammes a compter
	for (i = 1; i <= nBlockSize; i++)
		indexedKeyBlock.AddKey(i);
	cout << "NGrams: " << indexedKeyBlock << endl;

	// Test avec plusieurs chaines
	indexedKeyBlock.IndexKeys();
	charNgramCounts.TestCount("", &indexedKeyBlock);
	charNgramCounts.TestCount("le", &indexedKeyBlock);
	charNgramCounts.TestCount("bonjour", &indexedKeyBlock);
	charNgramCounts.TestCount("le monde est grand", &indexedKeyBlock);
	charNgramCounts.TestCount("coucou", &indexedKeyBlock);
	charNgramCounts.TestCount("le soir et le matin", &indexedKeyBlock);
}

void KWDRMultipleCharNGramCounts::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeAllTablesCharNgramCounts(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
	delete valueBlock;
}

void KWDRMultipleCharNGramCounts::Optimize()
{
	int nBlockSize;
	int nCumulatedSize;

	require(lvNGramMasks.GetSize() > 0);

	// Acces a la taille du block, en deuxieme operande
	nBlockSize = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

	// Calcul des caracteristiques specifiques a la regle
	nUsedHashTableNumber = 0;
	nCumulatedSize = 0;
	while (nCumulatedSize < nBlockSize)
	{
		nCumulatedSize += ivHashTableSizes.GetAt(nUsedHashTableNumber);
		nUsedHashTableNumber++;
	}

	// DDD
	//  On peut optimiser en fonction de la valeur de la premiere et de la derniere cle utilisee du bloc
}

void KWDRMultipleCharNGramCounts::InitializeGlobalVariables()
{
	boolean bDisplay = false;
	int nGramLength;
	int nHashTableSize;
	longint lNGramMask;
	int i;

	// Calcul des specifications globales communes a toutes les regles, une fois pour toutes
	if (lvNGramMasks.GetSize() == 0)
	{
		assert(ivNGramLengths.GetSize() == 0);

		///////////////////////////////////////////////////////////////////////
		// Choix d'une serie de tables de hashage de taille croissante par
		// longueur de ngrammes
		// On n'en prend que deux par longueur de n-gramme, pour eviter les collision
		// tout en evitant une redondance effective
		// Les longueurs choisies ont ete ajustee empiriquement pour avoir un bon
		// compromis entre le nombre de variables global et la performance predictive

		// Table de hashage de taille 1 pour les 1-gramme: correspond a la longueur des texte
		ivNGramLengths.Add(1);
		ivHashTableSizes.Add(1);

		// Pour les 1-grammes, il y au plus 256 1-grammes distincts
		// On commence a des tailles de 16 au minimum, par puissance de 2, jusqu'a 128
		nHashTableSize = 16;
		while (nHashTableSize <= 128)
		{
			ivNGramLengths.Add(1);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}
		assert(nHashTableSize == 256);

		// Pour les 2-grammes, on prend les trois tailles successives suivantes
		nHashTableSize = 256;
		while (nHashTableSize <= 1024)
		{
			ivNGramLengths.Add(2);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}

		// Pour les 3-grammes, on prend les trois tailles successives suivantes
		nHashTableSize = 2048;
		while (nHashTableSize <= 8192)
		{
			ivNGramLengths.Add(3);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}

		// Par longeur de n-gramme a partir de 4, on ne prend plus que deux tailles, toujours les memes
		for (nGramLength = 4; nGramLength <= nMaxNGramLength; nGramLength++)
		{
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(16384);
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(32768);
		}

		// DDD
		DEPRECATEDInitializeGlobalVariables();

		///////////////////////////////////////////////////////////////////////
		// Finalisation de l'initialisation

		// Calcul des tailles cumulees des tables de hashages
		ivHashTableCumulatedSizes.Add(0);
		for (i = 0; i < ivNGramLengths.GetSize(); i++)
			ivHashTableCumulatedSizes.Add(
			    ivHashTableCumulatedSizes.GetAt(ivHashTableCumulatedSizes.GetSize() - 1) +
			    ivHashTableSizes.GetAt(i));

		// Calcul des masques par nGramme: on met tous les bits a 1 partout sur la longueur des n-grammes
		lvNGramMasks.SetSize(nMaxNGramLength + 1);
		for (nGramLength = 1; nGramLength <= nMaxNGramLength; nGramLength++)
		{
			lNGramMask = 0;
			for (i = 0; i < nGramLength; i++)
			{
				lNGramMask <<= 8;
				lNGramMask += 255;
			}
			lvNGramMasks.SetAt(nGramLength, lNGramMask);
		}

		// Affichage
		if (bDisplay)
		{
			cout << "N-Gram\tHash table size\n";
			for (i = 0; i < ivNGramLengths.GetSize(); i++)
			{
				cout << ivNGramLengths.GetAt(i) << "\t" << ivHashTableSizes.GetAt(i) << "\t"
				     << ivHashTableCumulatedSizes.GetAt(i + 1) << "\n";
			}
			cout << "N-Gram\tMask\n";
			for (nGramLength = 1; nGramLength <= nMaxNGramLength; nGramLength++)
				cout << nGramLength << "\t" << lvNGramMasks.GetAt(nGramLength) << "\n";
		}
	}
	ensure(AreGlobalVariablesInitialized());
}

void KWDRMultipleCharNGramCounts::DEPRECATEDInitializeGlobalVariables()
{
	ALString sLearningTextVariableNgramPolicy;
	int nGramLength;
	int nHashTableSize;
	int nCumulatedSize;
	const double dSizeRatio = 2 * sqrt(2);
	int i;

	assert(ivHashTableCumulatedSizes.GetSize() == 0);

	// Recherche d'un parametre externe sur la politiques de getsion des tables de hashage par longueur de n-gramme
	sLearningTextVariableNgramPolicy = p_getenv("KhiopsTextVariableNGramPolicy");
	sLearningTextVariableNgramPolicy.MakeLower();

	// Pise en compte si necessaire
	if (sLearningTextVariableNgramPolicy != "")
	{
		// Message utilisateur
		Global::AddSimpleMessage("KhiopsTextVariableNGramPolicy: " + sLearningTextVariableNgramPolicy);

		// Premiere politique (V10.0.1)
		if (sLearningTextVariableNgramPolicy == "first")
		{
			ivNGramLengths.SetSize(0);
			ivHashTableSizes.SetSize(0);

			// Calcul des longueurs de ngrammes et tailles de tables de hashage
			nGramLength = 1;
			nHashTableSize = 1;
			for (i = 0; i < 8; i++)
			{
				ivNGramLengths.Add(nGramLength);
				ivHashTableSizes.Add(nHashTableSize);
				nHashTableSize *= 2;
			}
			for (nGramLength = 2; nGramLength <= nMaxNGramLength; nGramLength++)
			{
				for (i = 0; i < 3; i++)
				{
					ivNGramLengths.Add(nGramLength);
					ivHashTableSizes.Add(nHashTableSize);
					nHashTableSize *= 2;
				}
			}
		}

		// Seconde politique (V10.0.1)
		if (sLearningTextVariableNgramPolicy == "second")
		{
			ivNGramLengths.SetSize(0);
			ivHashTableSizes.SetSize(0);

			///////////////////////////////////////////////////////////////////////
			// Choix d'une serie de tables de hashage de taille croissante par
			// longueur de ngrammes
			// On n'en prend que deux par longueur de n-gramme, pour eviter les collision
			// tout en evitant une redondance effective
			// Les longueurs choisies ont ete ajustee empiriquement pour avoir un bon
			// compromis entre le nombre de variables global et la performance predictive

			// Table de hashage de taille 1 pour les 1-gramme: correspond a la longueur des texte
			nCumulatedSize = 0;
			ivNGramLengths.Add(1);
			ivHashTableSizes.Add(1);
			nCumulatedSize += ivHashTableSizes.GetAt(ivHashTableSizes.GetSize() - 1);

			// Il y au plus 256 1-grammes distincts
			// Choix d'une premiere table de tres petite taille
			ivNGramLengths.Add(1);
			ivHashTableSizes.Add(16);
			nCumulatedSize += ivHashTableSizes.GetAt(ivHashTableSizes.GetSize() - 1);

			// Choix d'une second table de grande taille
			// Le nombre est choisi a environ 128, de telle facon que tous les n-grammes de longueur
			// inferieure a 4 puissent tenir dans un bloc de longueur inferieure a 100000,
			// ce qui correspond a un nombre max de variable constryuites pour les textes
			ivNGramLengths.Add(1);
			ivHashTableSizes.Add(126);
			nCumulatedSize += ivHashTableSizes.GetAt(ivHashTableSizes.GetSize() - 1);

			// Par longeur de n-gramme, on prend deux taille successives selon le meme ratio
			for (nGramLength = 2; nGramLength <= nMaxNGramLength; nGramLength++)
			{
				for (i = 0; i < 2; i++)
				{
					ivNGramLengths.Add(nGramLength);
					nHashTableSize =
					    int(ivHashTableSizes.GetAt(ivHashTableSizes.GetSize() - 1) * dSizeRatio);
					ivHashTableSizes.Add(nHashTableSize);
					nCumulatedSize += ivHashTableSizes.GetAt(ivHashTableSizes.GetSize() - 1);
					assert(nCumulatedSize <= 100000 or nGramLength > 4);
				}
			}
		}
	}
}

boolean KWDRMultipleCharNGramCounts::AreGlobalVariablesInitialized()
{
	return lvNGramMasks.GetSize() >= 0;
}

////////////////////////////////////////////////////////////////////////////

KWDRStudyCharNGramCounts::KWDRStudyCharNGramCounts()
{
	SetName("StudyCharNGramCounts");
	SetLabel("Study char n-gram counts from a text value");
	AddOperand(new KWDerivationRuleOperand);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(3)->SetType(KWType::Continuous);
	GetOperandAt(4)->SetType(KWType::Continuous);

	// On impose que les deux derniers operandes soient des constantes
	GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(4)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRStudyCharNGramCounts::~KWDRStudyCharNGramCounts() {}

KWDerivationRule* KWDRStudyCharNGramCounts::Create() const
{
	return new KWDRStudyCharNGramCounts;
}

void KWDRStudyCharNGramCounts::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDRCharNGramCounts::Compile(kwcOwnerClass);

	// Parametrage avance
	nLastNGramLength = int(floor(GetOperandAt(3)->GetContinuousConstant() + 0.5));
	bWithRandomSign = GetOperandAt(4)->GetContinuousConstant() != 0;
}

boolean KWDRStudyCharNGramCounts::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
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

////////////////////////////////////////////////////////////////////////////

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