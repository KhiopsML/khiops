// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTokenCounts.h"

KWDRTokenize::KWDRTokenize()
{
	SetName("Tokenize");
	SetLabel("Recode a catagorical value with tokens separated by blank chars");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
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
	sInputValue = GetFirstOperand()->GetSymbolValue(kwoObject);

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
}

KWDRCharNGramCounts::KWDRCharNGramCounts()
{
	SetName("CharNGramCounts");
	SetLabel("Char n-gram counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);

	// On impose que les deux derniers operandes soient des constantes
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);

	// Initialisation par defaut des caracteristique des n-grammes
	nNGramLength = 0;
	nNGramNumber = 0;
	lNGramMask = 0;
	lRandomOffset = 0;
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

	sInputValue = GetFirstOperand()->GetSymbolValue(kwoObject);
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
	KWContinuousValueBlock* cvbTokenCounts;
	IntVector ivUsedSparseIndexes;
	const char* sStringValue;
	int nLength;
	int n;
	int nMax;
	longint lNgramValue;
	longint lIndex;
	int nNKey;
	int nSparseIndex;
	char* sIndex;
	int i;

	require(indexedKeyBlock != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sValue.GetValue();
	if (bDisplay)
		cout << GetName() << "\t" << sStringValue << endl;

	// Retaillage si necessaire du tableau sparse des parties
	if (ivSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
		ivSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

	// Parcours des caracteres de la chaines pour en calculer les ngrammes
	// En debut de chaine et en fin de chaine, on ne prend qu'une sous-partie
	// des caracteres du n-gramme
	// Par exemple, pour les 3-grammes de la chaine "Bonjour", on considere
	// "..B", ".Bo", "Bon", "onj", "njo", "jou", "our", "ur.", "r.."
	nLength = (int)strlen(sStringValue);
	// Cas ou la chaine est vide: aucun n-gramme
	if (nLength == 0)
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
		lNgramValue = 0;
		nMax = nLength + nNGramLength - 1;
		for (n = 0; n < nMax; n++)
		{
			// On multiplie l'index courant par 256
			lNgramValue <<= 8;

			// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
			if (n < nLength)
				lNgramValue += (unsigned char)sStringValue[n];

			// On tronque a la longueur du pattern en appliquant le masque
			lNgramValue &= lNGramMask;

			// On calcul un index a partir de la valeur du n-gramme, en ajoutant un offset pseudo-aleatoire
			lIndex = lNgramValue + lRandomOffset;

			// On bascule en index positif, necessaire pour l'appel a IthRandomInt
			// N'est en principe necessaire que dans de rare cas, uniquement pour les longueurs
			// maximum des n-grammes qui atteignent la limite de taille des longint
			assert(lIndex >= 0 or nNGramLength == sizeof(longint));
			if (lIndex < 0)
				lIndex = -lIndex;

			// Calcul d'une cle de n-gram projete aleatoiremenent sur [1; nNGrammNumber]
			nNKey = 1 + IthRandomInt(lIndex, nNGramNumber - 1);

			// Recherche de l'index sparse correspondant
			nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

			// Test s'il faut memoriser le compte du n-gramme
			if (nSparseIndex != -1)
			{
				// Memorisation si necessaire  de l'index sparse utilise
				if (ivSparseCounts.GetAt(nSparseIndex) == 0)
					ivUsedSparseIndexes.Add(nSparseIndex);

				// Mise a jour du compte du ngramme
				ivSparseCounts.UpgradeAt(nSparseIndex, 1);
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
				cout << "\t" << lNgramValue << "\t" << lIndex << "\t" << nNKey << "\t" << nSparseIndex
				     << endl;
			}
		}

		// Tri des index sparses de partie utilises
		ivUsedSparseIndexes.Sort();

		// Construction du bloc de compte de la bonne taille
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(ivUsedSparseIndexes.GetSize());

		// Initialisation du vecteur sparse
		// On exploite ici le fait que les index sparse sont necessairment ordonnes de la
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
			ivSparseCounts.SetAt(nSparseIndex, 0);
		}
	}
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	return cvbTokenCounts;
}

void KWDRCharNGramCounts::Compile(KWClass* kwcOwnerClass)
{
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation des caracteristiques des n-grammes
	nNGramLength = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));
	nNGramNumber = int(floor(GetOperandAt(2)->GetContinuousConstant() + 0.5));
	assert(nNGramLength <= sizeof(longint));

	// Calcul du masque: on met tous les bits a 1 partout sur la longueur des n-grammes
	lNGramMask = 0;
	for (i = 0; i < nNGramLength; i++)
	{
		lNGramMask <<= 8;
		lNGramMask += 255;
	}

	// Clcul du decallage pour generer des hashage differents selon les caracteristiques des nGrammes
	lRandomOffset = nNGramLength;
	lRandomOffset *= 1000000000;
	lRandomOffset += nNGramNumber;
	lRandomOffset *= 1000000000;
	assert(lRandomOffset >= 0);

	// Reinitialisation du vecteur de travail sparse des comptes
	// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
	ivSparseCounts.SetSize(0);
}

boolean KWDRCharNGramCounts::CheckOperandsCompletness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	double dValue;
	int nValue;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompletness(kwcOwnerClass);

	// Verification des operandes
	if (bOk)
	{
		assert(GetOperandNumber() == 3);

		// Operande de longueur des n-grammes
		dValue = GetOperandAt(1)->GetContinuousConstant();
		nValue = int(floor(dValue + 0.5));
		if (fabs(dValue - nValue) > 1e-5)
		{
			AddError(sTmp + "Length of n-grams (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 1 must be an integer");
			bOk = false;
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Length of n-grams (" + IntToString(nValue) +
				 ") in operand 1  must be at least 1");
			bOk = false;
		}
		else if (nValue > 8)
		{
			AddError(sTmp + "Length of n-grams (" + IntToString(nValue) +
				 ") in operand 1 must be at most 8");
			bOk = false;
		}

		// Operande du nombre de  n-grammes
		dValue = GetOperandAt(2)->GetContinuousConstant();
		nValue = int(floor(dValue + 0.5));
		if (fabs(dValue - nValue) > 1e-5)
		{
			AddError(sTmp + "Number of n-grams (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 2 must be an integer");
			bOk = false;
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Number of n-grams (" + IntToString(nValue) +
				 ") in operand 2 must be at least 1");
			bOk = false;
		}
		else if (nValue > 1000000)
		{
			AddError(sTmp + "Number of n-grams (" + IntToString(nValue) +
				 ") in operand 2 must be at most 1000000");
			bOk = false;
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
	require(CheckOperandsCompletness(kwcOwnerClass));
	require(attributeBlock->GetVarKeyType() == KWType::Continuous);

	// Appel de la methode ancetre
	bResult = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Analyse des cles du bloc d'attribut par rapport a la partition
	if (bResult)
	{
		// Acces au nombre de ngrammes
		nNumber = int(floor(GetOperandAt(2)->GetContinuousConstant() + 0.5));

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

void KWDRCharNGramCounts::Test()
{
	KWDRCharNGramCounts charNgramCounts;
	KWIndexedCKeyBlock indexedKeyBlock;

	// Initialisation des tokens a compter, dans l'ordre alphabetique impose
	indexedKeyBlock.AddKey("bonjour");
	indexedKeyBlock.AddKey("le");
	indexedKeyBlock.AddKey("monde");
	indexedKeyBlock.AddKey("tout");
	cout << "Tokens: " << indexedKeyBlock << endl;

	// Test avec plusieurs chaines
	indexedKeyBlock.IndexKeys();
	charNgramCounts.TestCount("bonjour", &indexedKeyBlock);
	charNgramCounts.TestCount("le monde est grand", &indexedKeyBlock);
	charNgramCounts.TestCount("coucou", &indexedKeyBlock);
	charNgramCounts.TestCount("le soir et le matin", &indexedKeyBlock);
}

void KWDRCharNGramCounts::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeCharNgramCounts(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
}