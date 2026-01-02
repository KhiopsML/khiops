// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTextualAnalysisPROTO.h"

void KWDRRegisterTextualAnalysisPROTORules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTextAllNGrams);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextListAllNGrams);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenize);
	KWDerivationRule::RegisterDerivationRule(new KWDRNGramCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRStudyNGramCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRExtractWords);
}

////////////////////////////////////////////////////////////////////////////

IntVector KWDRTextAllNGrams::ivNGramLengths;
IntVector KWDRTextAllNGrams::ivHashTableSizes;
IntVector KWDRTextAllNGrams::ivHashTableCumulatedSizes;
LongintVector KWDRTextAllNGrams::lvNGramMasks;

KWDRTextAllNGrams::KWDRTextAllNGrams()
{
	SetName("TextAllNGrams");
	SetLabel("All ngram counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);

	// Premier operande de type texte
	GetFirstOperand()->SetType(KWType::Text);

	// Parametres d'optimisation de la regle
	nMaxVarKeyIndex = 0;
	nUsedHashTableNumber = 0;
	nDynamicCompileFreshness = 0;

	// Initialisation des variables globales
	InitializeGlobalVariables();
}

KWDRTextAllNGrams::~KWDRTextAllNGrams() {}

int KWDRTextAllNGrams::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRTextAllNGrams::Create() const
{
	return new KWDRTextAllNGrams;
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Calcul du resultat
	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);
	cvbTokenCounts = ComputeAllTablesCharNgramCountsFromText(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRTextAllNGrams::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

boolean KWDRTextAllNGrams::CheckBlockAttributes(const KWClass* kwcOwnerClass,
						const KWAttributeBlock* attributeBlock) const
{
	boolean bOk;
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
	bOk = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Analyse des cles du bloc d'attribut par rapport a la partition
	if (bOk)
	{
		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey trop grand
			if (nVarKey > GetMaxVarKey())
			{
				attributeBlock->AddError(sTmp + "Variable " + checkedAttribute->GetName() +
							 " has its VarKey=" + IntToString(nVarKey) +
							 " greater than the maximum possible (" +
							 IntToString(GetMaxVarKey()) +
							 ") in a variable block obtained using rule " + GetName());
				bOk = false;
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
	return bOk;
}

longint KWDRTextAllNGrams::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRTextAllNGrams);
	lUsedMemory += lvSparseCounts.GetUsedMemory() - sizeof(LongintVector);
	return lUsedMemory;
}

int KWDRTextAllNGrams::GetMaxHashTableNumber()
{
	return ivNGramLengths.GetSize();
}

int KWDRTextAllNGrams::GetHashTableNGramLengthAt(int nIndex)
{
	return ivNGramLengths.GetAt(nIndex);
}

int KWDRTextAllNGrams::GetHashTableSizeAt(int nIndex)
{
	return ivHashTableSizes.GetAt(nIndex);
}

int KWDRTextAllNGrams::GetMaxVarKey()
{
	require(AreGlobalVariablesInitialized());
	return ivHashTableCumulatedSizes.GetAt(ivHashTableCumulatedSizes.GetSize() - 1);
}

void KWDRTextAllNGrams::Test()
{
	const int nBlockSize = 100000;
	KWDRTextAllNGrams charNgramCounts;
	KWIndexedNKeyBlock indexedKeyBlock;
	int i;

	// Initialisation des ngrams a compter
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

void KWDRTextAllNGrams::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeAllTablesCharNgramCountsFromText(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
	delete valueBlock;
}

void KWDRTextAllNGrams::DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const KWIndexedNKeyBlock* indexedNKeyBlock;
	int nCumulatedSize;
	int nVarKey;
	int n;

	require(indexedKeyBlock != NULL);
	require(Check());
	require(IsCompiled());
	require(KWType::IsValueBlock(GetType()));

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Reinitialisation des parametre a optimiser
		nMaxVarKeyIndex = 0;
		nUsedHashTableNumber = 0;

		// On prend les bons types de blocs de cles
		indexedNKeyBlock = cast(const KWIndexedNKeyBlock*, indexedKeyBlock);

		// Parcours des cles du bloc source
		nMaxVarKeyIndex = 0;
		for (n = 0; n < indexedNKeyBlock->GetKeyNumber(); n++)
		{
			nVarKey = indexedNKeyBlock->GetKeyAt(n);
			nMaxVarKeyIndex = max(nMaxVarKeyIndex, nVarKey);
		}

		// Calcul des caracteristiques specifiques a la regle
		// On pourrait optimiser en fonction de la valeur de la premiere et de la derniere cle utilisee du bloc,
		// mais c'est laborieux a mettre en place. A envisager plus tard en cas de problemes de performance
		// critiques.
		nUsedHashTableNumber = 0;
		nCumulatedSize = 0;
		while (nCumulatedSize < nMaxVarKeyIndex)
		{
			nCumulatedSize += ivHashTableSizes.GetAt(nUsedHashTableNumber);
			nUsedHashTableNumber++;
		}

		// Reinitialisation du vecteur de travail sparse des comptes
		// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
		lvSparseCounts.SetSize(0);

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

void KWDRTextAllNGrams::InitializeGlobalVariables()
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
		// longueur de ngrams
		// On n'en prend que deux par longueur de ngram, pour eviter les collision
		// tout en evitant une redondance effective
		// Les longueurs choisies ont ete ajustee empiriquement pour avoir un bon
		// compromis entre le nombre de variables global et la performance predictive

		// Table de hashage de taille 1 pour les 1gram: correspond a la longueur des texte
		ivNGramLengths.Add(1);
		ivHashTableSizes.Add(1);

		// Pour les 1-grammes, il y au plus 256 1grams distincts
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

		// Par longeur de ngram a partir de 4, on ne prend plus que deux tailles, toujours les memes
		for (nGramLength = 4; nGramLength <= nMaxNGramLength; nGramLength++)
		{
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(16384);
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(32768);
		}

		///////////////////////////////////////////////////////////////////////
		// Finalisation de l'initialisation

		// Calcul des tailles cumulees des tables de hashages
		ivHashTableCumulatedSizes.Add(0);
		for (i = 0; i < ivNGramLengths.GetSize(); i++)
			ivHashTableCumulatedSizes.Add(
			    ivHashTableCumulatedSizes.GetAt(ivHashTableCumulatedSizes.GetSize() - 1) +
			    ivHashTableSizes.GetAt(i));

		// Calcul des masques par nGramme: on met tous les bits a 1 partout sur la longueur des ngrams
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
			cout << "Max var key\t" << GetMaxVarKey() << "\n";
		}
	}
	ensure(AreGlobalVariablesInitialized());
}

boolean KWDRTextAllNGrams::AreGlobalVariablesInitialized()
{
	return lvNGramMasks.GetSize() >= 0;
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeAllTablesCharNgramCountsFromText(const Symbol& sValue,
							   const KWIndexedKeyBlock* indexedKeyBlock) const
{
	SymbolVector svValues;

	svValues.Add(sValue);
	return ComputeAllTablesCharNgramCountsFromTextList(&svValues, indexedKeyBlock);
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeAllTablesCharNgramCountsFromTextList(const SymbolVector* svValues,
							       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const boolean bDisplay = false;
	KWContinuousValueBlock* cvbTokenCounts;
	IntVector ivUsedSparseIndexes;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	const char* sText;
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

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Affichage de l'entete
	if (bDisplay)
	{
		cout << GetName() << "\t" << nValueNumber << endl;
		for (i = 0; i < nValueNumber; i++)
		{
			if (i > 3)
			{
				cout << "\t...";
				break;
			}
			cout << "\t" << svValues->GetAt(i);
		}
		cout << "\tLength\tSize\tn\tngram\tindex\thash\tkey\tsparse index\tcount" << endl;
	}

	// Retaillage si necessaire du tableau sparse des parties
	if (lvSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
		lvSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
	{
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(0);
	}
	// Cas general, avec traitement pour chaque valeur du vecteur en entree et cumul sur l'ensemble.
	// Par valeur, on prend les caracteres un a un pour composer un index, en multipliant par 256 (<<8) apres chaque
	// caractere On aurait pu utiliser des memcpy pour recopier directement les caracteres dans l'index (longint)
	// traite comme un tableau de 8 caracteres. Le probleme est que cela depend de l'ordre des octets utilise
	// par le systeme pour stocker les entiers (endianness), ce qui rendrait la solution non portable
	// (windows et linux sont en little endian, mais ARM et android pas forcement)
	else
	{
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			sText = sValue.GetValue();
			nTextLength = sValue.GetLength();

			// On boucle sur les longueurs de ngrams
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

				// Analyse du texte pour une longueur de ngrams donnee
				lNGramMask = lvNGramMasks.GetAt(nNGramLength);

				// Initialisation du debut premier ngram
				lNgramValue = 0;
				for (n = 0; n < nNGramLength - 1; n++)
				{
					// On multiplie l'index courant par 256
					lNgramValue <<= 8;

					// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
					lNgramValue += (unsigned char)sText[n];
				}

				// Parcours des ngrams jusqu'a la fin du texte
				for (n = nNGramLength - 1; n < nTextLength; n++)
				{
					// On multiplie l'index courant par 256
					lNgramValue <<= 8;

					// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
					lNgramValue += (unsigned char)sText[n];

					// On tronque a la longueur du pattern en appliquant le masque
					lNgramValue &= lNGramMask;
					assert(lNgramValue >= 0);

					// On prend un nombre aleatoire a partir de la valeur de ngramme
					lRandom = IthRandomLongint(lNgramValue);

					// Alimentation des tables de hashage pour cette longueur de ngramme
					lCukooHash = lRandom;
					nStartNKey = nNGramStartNKey;
					for (nHashTableIndex = nHashTableFirstIndex;
					     nHashTableIndex <= nHashTableLastIndex; nHashTableIndex++)
					{
						// Calcul d'une cle de ngram projetee aleatoirement sur sa table a
						// partir de nStartNKey
						nNKey = abs(lCukooHash) % ivHashTableSizes.GetAt(nHashTableIndex);
						nNKey += nStartNKey;

						// Recherche de l'index sparse correspondant
						nSparseIndex =
						    cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

						// Test s'il faut memoriser le compte du ngram
						if (nSparseIndex != -1)
						{
							assert(nSparseIndex < lvSparseCounts.GetSize());

							// Memorisation si necessaire de l'index sparse utilise
							if (lvSparseCounts.GetAt(nSparseIndex) == 0)
								ivUsedSparseIndexes.Add(nSparseIndex);

							// Mise a jour du compte du ngramme
							lvSparseCounts.UpgradeAt(nSparseIndex, 1);
						}

						// Affichage des resultats intermediaires, pour la mise au point
						if (bDisplay)
						{
							cout << "\t" << nNGramLength << "\t"
							     << ivHashTableSizes.GetAt(nHashTableIndex) << "\t" << n
							     << "\t";

							// Affichage du ngram collecte dans la valeur lNgramValue
							sIndex = (char*)&lNgramValue;
							for (i = 7; i >= 0; i--)
							{
								if (sIndex[i] != 0)
									cout << sIndex[i];
							}

							// Affichage des variables de calcul
							cout << "\t" << lNgramValue << "\t#" << lCukooHash << "\t"
							     << nNKey << "\t" << nSparseIndex << "\t";
							if (nSparseIndex != -1)
								cout << lvSparseCounts.GetAt(nSparseIndex);
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
		}

		// Tri des index sparses des parties utilisees
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

			// Reinitialisation du vecteur de travail
			lvSparseCounts.SetAt(nSparseIndex, 0);
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
	ensure(CheckCharNgramCountsValueBlockFromTextList(svValues, indexedKeyBlock, cvbTokenCounts));
	return cvbTokenCounts;
}

boolean KWDRTextAllNGrams::CheckCharNgramCountsValueBlockFromTextList(const SymbolVector* svValues,
								      const KWIndexedKeyBlock* indexedKeyBlock,
								      const KWContinuousValueBlock* valueBlock) const
{
	boolean bOk = true;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	int nTextLength;
	int nMaxTextLength;
	boolean bIsFullBlock;
	LongintVector lvCumulatedNGramCounts;
	LongintVector lvExpectedNGramCounts;
	int i;
	int nSparseIndex;
	int nNKey;
	int nHashTableIndex;
	int nNGramLength;
	int nHashTableCumulatedSize;

	require(AreGlobalVariablesInitialized());
	require(indexedKeyBlock != NULL);
	require(valueBlock != NULL);

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
		bOk = valueBlock->GetValueNumber() == 0;
	// Cas general
	else
	{
		// Initialisation d'un vecteur de comptes de ngrams par taille de table hashage
		lvCumulatedNGramCounts.SetSize(nUsedHashTableNumber);
		lvExpectedNGramCounts.SetSize(nUsedHashTableNumber);

		// Test si le block est plein
		assert(indexedKeyBlock->GetKeyNumber() <= nMaxVarKeyIndex);
		bIsFullBlock = (indexedKeyBlock->GetKeyNumber() == nMaxVarKeyIndex);
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(1));
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(nMaxVarKeyIndex));

		// Boucle sur les valeurs du vecteur pour determiner les comptes cumules attendus
		nMaxTextLength = 0;
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			nTextLength = sValue.GetLength();
			nMaxTextLength = max(nMaxTextLength, nTextLength);

			// Mise a jour des compte totaux attendus sur l'ensemble des valeurs
			for (nHashTableIndex = 0; nHashTableIndex < nUsedHashTableNumber; nHashTableIndex++)
			{
				nNGramLength = ivNGramLengths.GetAt(nHashTableIndex);

				// Cas de texte de longueur inferieure aux ngrams
				if (nTextLength < nNGramLength)
					lvExpectedNGramCounts.UpgradeAt(nHashTableIndex, 0);
				// Cas general
				else
					lvExpectedNGramCounts.UpgradeAt(nHashTableIndex,
									nTextLength + 1 - nNGramLength);
			}
		}

		// Comptage du nombre cumule de ngrams par taille de block
		for (i = 0; i < valueBlock->GetValueNumber(); i++)
		{
			// Acces a la cle de ngram
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
			lvCumulatedNGramCounts.UpgradeAt(nHashTableIndex, (longint)valueBlock->GetValueAt(i));
		}

		// Verification des comptes cumules de ngrams
		for (nHashTableIndex = 0; nHashTableIndex < nUsedHashTableNumber; nHashTableIndex++)
		{
			nNGramLength = ivNGramLengths.GetAt(nHashTableIndex);

			// Cas de texte le plus grand de longueur inferieure aux ngrams
			if (nMaxTextLength < nNGramLength)
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) == 0;
			// Cas d'un comptage exhaustif si le block est plein avant la derniere table
			else if (bIsFullBlock and nHashTableIndex < nUsedHashTableNumber - 1)
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) ==
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			// Cas d'un comptage exhaustif si le block est plein pour la derniere table
			else if (bIsFullBlock and nHashTableIndex == nUsedHashTableNumber - 1 and
				 indexedKeyBlock->GetKeyNumber() ==
				     ivHashTableCumulatedSizes.GetAt(nHashTableIndex + 1))
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) ==
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			// Cas general
			else
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) <=
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			assert(bOk);
		}
	}
	return bOk;
}

////////////////////////////////////////////////////////////////////////////

KWDRTextListAllNGrams::KWDRTextListAllNGrams()
{
	// Specialisation de la classe mere
	SetName("TextListAllNGrams");
	SetLabel("All ngram counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListAllNGrams::~KWDRTextListAllNGrams() {}

KWDerivationRule* KWDRTextListAllNGrams::Create() const
{
	return new KWDRTextListAllNGrams;
}

KWContinuousValueBlock*
KWDRTextListAllNGrams::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
							 const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	SymbolVector* svInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Calcul du resultat
	svInputValue = GetFirstOperand()->GetTextListValue(kwoObject);
	cvbTokenCounts = ComputeAllTablesCharNgramCountsFromTextList(svInputValue, indexedKeyBlock);
	return cvbTokenCounts;
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

	// Initialisation par defaut des caracteristique des ngrams
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

	// Memorisation des caracteristiques des ngrams
	nNGramNumber = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));
	nFirstNGramLength = 1;
	if (GetOperandNumber() >= 2)
		nFirstNGramLength = int(floor(GetOperandAt(2)->GetContinuousConstant() + 0.5));
	assert(1 <= nFirstNGramLength and nFirstNGramLength <= nMaxNGramLength);

	// Premiere taille de ngram a prendre en compte
	nLastNGramLength = nFirstNGramLength;

	// Calcul des masques par nGramme: on met tous les bits a 1 partout sur la longueur des ngrams
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
			AddError(sTmp + "Number of ngrams (" + IntToString(nValue) +
				 ") in operand 2 must be at most 1000000");
			bOk = false;
		}

		// Operande de longueur des ngrams, si present en troisieme operande
		if (bOk and GetOperandNumber() >= 3)
		{
			dValue = GetOperandAt(2)->GetContinuousConstant();
			bOk = KWContinuous::ContinuousToInt(dValue, nValue);
			if (not bOk)
			{
				AddError(sTmp + "Length of ngrams (" + KWContinuous::ContinuousToString(dValue) +
					 ") in operand 3 must be an integer");
				assert(bOk == false);
			}
			else if (nValue < 1)
			{
				AddError(sTmp + "Length of ngrams (" + IntToString(nValue) +
					 ") in operand 3  must be at least 1");
				bOk = false;
			}
			else if (nValue > nMaxNGramLength)
			{
				AddError(sTmp + "Length of ngrams (" + IntToString(nValue) +
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

		// Acces au nombre de ngrams, en deuxieme operande
		nNumber = int(floor(GetOperandAt(1)->GetContinuousConstant() + 0.5));

		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey plus grand que le nombre de ngrams
			if (nVarKey > nNumber)
			{
				// Messages d'erreur
				attributeBlock->AddError(
				    sTmp + "Variable " + checkedAttribute->GetName() +
				    +" has its VarKey=" + IntToString(nVarKey) +
				    " greater than the total number of ngrams (" + IntToString(nNumber) +
				    ") in the ngram block (third operand of rule " + GetName() + ")");
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

	// Parcours des caracteres de la chaines pour en calculer les ngrams
	// En debut de chaine et en fin de chaine, on ne prend qu'une sous-partie
	// des caracteres du ngram
	// Par exemple, pour les 3-grammes de la chaine "Bonjour", on considere
	// "..B", ".Bo", "Bon", "onj", "njo", "jou", "our", "ur.", "r.."
	nTextLength = sValue.GetLength();
	// Cas ou la chaine est vide: aucun ngram
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

				// On calcul un index a partir de la valeur du ngram, en ajoutant un offset
				// pseudo-aleatoire
				lIndex = lNgramValue + lRandomOffset;

				// On bascule en index positif, necessaire pour l'appel a IthRandomInt
				// N'est en principe necessaire que dans de rare cas, uniquement pour les longueurs
				// maximum des ngrams qui atteignent la limite de taille des longint
				assert(lIndex >= 0 or nNGramLength == sizeof(longint));
				if (lIndex < 0)
					lIndex = -lIndex;

				// Calcul d'une cle de ngram projete aleatoiremenent sur [1; nNGrammNumber]
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

				// Test s'il faut memoriser le compte du ngram
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

					// Affichage du ngram collecte dans la valeur lNgramValue
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
	SetLabel("Study ngram counts from a text value");
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

		// Operande de longueur max des ngrams
		dValue = GetOperandAt(3)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(dValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Max length of ngrams (" + KWContinuous::ContinuousToString(dValue) +
				 ") in operand 4 must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 1)
		{
			AddError(sTmp + "Max length of ngrams (" + IntToString(nValue) +
				 ") in operand 4  must be at least 1");
			bOk = false;
		}
		else if (nValue > nMaxNGramLength)
		{
			AddError(sTmp + "Max length of ngrams (" + IntToString(nValue) +
				 ") in operand 4 must be at most " + IntToString(nMaxNGramLength));
			bOk = false;
		}
		else if (dValue < GetOperandAt(2)->GetContinuousConstant())
		{
			AddError(sTmp + "Max length of ngrams (" + IntToString(nValue) +
				 ") in operand 4 must be greater than min length of ngrams in operand 3");
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
