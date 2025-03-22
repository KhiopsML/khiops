// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSortBuckets.h"
#include "PLRemoteFileService.h"
#include "PLParallelTask.h"

KWSortBuckets::KWSortBuckets()
{
	bIsIndexComputed = false;
}

KWSortBuckets::~KWSortBuckets()
{
	oaBuckets.DeleteAll();
}

void KWSortBuckets::Build(KWSortBucket* mainBucket, const ObjectArray* oaSplits)
{
	boolean bDisplay = false;
	int i;
	KWSortBucket* bucket;
	const KWKey* lastKey;
	const KWKey* currentKey;
	boolean bIsLastBucketSingleton;

	require(mainBucket != NULL);
	require(mainBucket->Check());
	require(oaSplits != NULL);
	require(oaSplits->GetSize() > 0);

	// Nettoyage initial
	DeleteAll();

	// Premier bucket : ]-inf, premier split[
	lastKey = cast(KWKey*, oaSplits->GetAt(0));
	bucket = new KWSortBucket;
	bucket->GetLowerBound()->CopyFrom(mainBucket->GetLowerBound());
	bucket->GetUpperBound()->CopyFrom(cast(KWKey*, oaSplits->GetAt(0)));
	bucket->SetLowerBoundExcluded(mainBucket->GetLowerBoundExcluded());
	bIsLastBucketSingleton = bucket->IsSingleton();
	bucket->SetUpperBoundExcluded(not bucket->IsSingleton());
	oaBuckets.Add(bucket);

	// Boucle sur les splits restants
	for (i = 1; i < oaSplits->GetSize(); i++)
	{
		currentKey = cast(KWKey*, oaSplits->GetAt(i));
		assert(lastKey->GetSize() == 0 or currentKey->GetSize() == 0 or lastKey->Compare(currentKey) <= 0);

		// Si le nouveau split correspond a un singleton deja enregistre, on ignore le split (deja traite)
		if (not(bIsLastBucketSingleton and currentKey->Compare(lastKey) == 0))
		{
			bucket = new KWSortBucket;
			bucket->GetLowerBound()->CopyFrom(lastKey);
			bucket->GetUpperBound()->CopyFrom(currentKey);
			bucket->SetLowerBoundExcluded(bIsLastBucketSingleton);
			bIsLastBucketSingleton = (bucket->GetLowerBound()->Compare(bucket->GetUpperBound()) == 0);
			if (bIsLastBucketSingleton)
			{
				bucket->SetLowerBoundExcluded(false);
				bucket->SetUpperBoundExcluded(false);
			}
			assert(bucket->Check());
			oaBuckets.Add(bucket);
		}
		lastKey = currentKey;
	}

	// Dernier bucket : ]dernier split,+inf[
	bucket = new KWSortBucket;
	bucket->GetLowerBound()->CopyFrom(lastKey);
	bucket->GetUpperBound()->CopyFrom(mainBucket->GetUpperBound());
	bucket->SetLowerBoundExcluded(bIsLastBucketSingleton);
	bucket->SetUpperBoundExcluded(mainBucket->GetUpperBoundExcluded());
	oaBuckets.Add(bucket);

	// Affichage des buckets construits
	if (bDisplay)
	{
		cout << "\t\tBuckets : " << endl;
		cout << *this << endl;
		cout << endl;
	}
	ensure(Check());
}

void KWSortBuckets::Initialize(const ObjectArray* oaSourceBuckets)
{
	KWSortBucket* bucket;
	int i;

	require(oaSourceBuckets != NULL);
	require(oaSourceBuckets->GetSize() != 0);

	// Nettoyage initial
	DeleteAll();

	// Ajout des buckets et calcul de l'occupation memoire des buckets
	for (i = 0; i < oaSourceBuckets->GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaSourceBuckets->GetAt(i));
		oaBuckets.Add(bucket->Clone());
	}
	ensure(Check());
}

void KWSortBuckets::AddBucket(KWSortBucket* sourceBucket)
{
	require(sourceBucket != NULL);
	oaBuckets.Add(sourceBucket);
	bIsIndexComputed = false;
	ensure(Check());
}

void KWSortBuckets::SplitLargeBucket(KWSortBucket* largeBucket, KWSortBuckets* subBuckets)
{
	KWSortBucket* bucket;
	int i;

	require(largeBucket != NULL);
	require(largeBucket->Check());
	require(subBuckets != NULL);
	require(subBuckets->Check());
	require(largeBucket->GetLowerBound()->Compare(
		    cast(KWSortBucket*, subBuckets->GetBucketAt(0))->GetLowerBound()) == 0);
	require(largeBucket->GetUpperBound()->Compare(
		    cast(KWSortBucket*, subBuckets->GetBucketAt(subBuckets->GetBucketNumber() - 1))->GetUpperBound()) ==
		0);

	// Parcours des chunks pour trouver l'index du chunk a remplacer
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));
		if (bucket == largeBucket)
			break;
	}
	assert(i < oaBuckets.GetSize());

	// Suppression du chunk de la liste
	oaBuckets.RemoveAt(i);

	// Insertion des sous-buckets
	oaBuckets.InsertObjectArrayAt(i, &subBuckets->oaBuckets);

	// Nettoyage
	delete largeBucket;
	subBuckets->RemoveAll();
	bIsIndexComputed = false;
	ensure(Check());
}

void KWSortBuckets::RemoveAll()
{
	oaBuckets.RemoveAll();
	oaDistinctKeys.RemoveAll();
	ivBucketIndexes.SetSize(0);
	bIsIndexComputed = false;
}

void KWSortBuckets::DeleteAll()
{
	oaBuckets.DeleteAll();
	oaDistinctKeys.RemoveAll();
	ivBucketIndexes.SetSize(0);
	bIsIndexComputed = false;
}

void KWSortBuckets::IndexBuckets()
{
	KWSortBucket* bucket;
	KWKey* currentKey;
	KWKey* lastKey;
	int i;

	require(Check());

	// Initialisation des resultats
	oaDistinctKeys.RemoveAll();
	ivBucketIndexes.SetSize(0);

	// Extraction des clefs distinctes et des index de buckets correspondants
	lastKey = NULL;
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));

		// Ajout de la cle inferieure sauf pour le premier bucket
		if (i > 0)
		{
			currentKey = bucket->GetLowerBound();
			if (currentKey != lastKey)
			{
				oaDistinctKeys.Add(currentKey);
				ivBucketIndexes.Add(i);
				lastKey = currentKey;
			}
		}

		// Ajout de la cle superieure sauf pour le dernier bucket
		if (i < oaBuckets.GetSize() - 1)
		{
			currentKey = bucket->GetUpperBound();
			if (currentKey != lastKey)
			{
				oaDistinctKeys.Add(currentKey);
				ivBucketIndexes.Add(i);
				lastKey = currentKey;
			}
		}
	}
	bIsIndexComputed = true;
}

void KWSortBuckets::DeleteBucketFiles()
{
	KWSortBucket* bucket;
	int i, j;

	for (i = 0; i < GetBucketNumber(); i++)
	{
		bucket = GetBucketAt(i);
		for (j = 0; j < bucket->GetChunkFileNames()->GetSize(); j++)
		{
			FileService::RemoveFile(bucket->GetChunkFileNames()->GetAt(j));
		}
	}
	ensure(CheckFileNames());
}

KWSortBucket* KWSortBuckets::SearchBucket(const KWKey* key) const
{
	int nLowerIndex;
	int nUpperIndex;
	int nKeyIndex;
	int nBucketIndexMin;
	int nBucketIndexMax;
	KWKey* pivotKey;
	KWSortBucket* bucket;
	int nIndex;
	int nCompare;

	require(key != NULL);
	require(oaBuckets.GetSize() > 0);
	require(bIsIndexComputed);
	require(cast(KWSortBucket*, oaBuckets.GetAt(0))->IsLessOrEqualThan(key));
	require(cast(KWSortBucket*, oaBuckets.GetAt(GetBucketNumber() - 1))->IsGreaterOrEqualThan(key));

	// Recherche dichotomique si il y a beaucoup de buckets
	if (oaBuckets.GetSize() > 10)
	{
		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = oaDistinctKeys.GetSize() - 1;

		// Recherche dichotomique de l'intervalle
		nCompare = 0;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			nKeyIndex = (nLowerIndex + nUpperIndex + 1) / 2;

			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			pivotKey = cast(KWKey*, oaDistinctKeys.GetAt(nKeyIndex));
			nCompare = key->Compare(pivotKey);
			if (nCompare > 0)
				nLowerIndex = nKeyIndex;
			else
				nUpperIndex = nKeyIndex;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// Recherche des index de buckets correspondant
		nBucketIndexMin = ivBucketIndexes.GetAt(nLowerIndex);
		nBucketIndexMax = ivBucketIndexes.GetAt(nUpperIndex);

		// Correction pour la borne sup
		if (nBucketIndexMax < oaBuckets.GetSize() - 1)
			nBucketIndexMax++;
	}
	else
	{
		// Initialisation des index extremites pour la recherche sequentielle
		nBucketIndexMin = 0;
		nBucketIndexMax = oaBuckets.GetSize() - 1;
	}
	assert(cast(KWSortBucket*, oaBuckets.GetAt(nBucketIndexMin))->IsLessOrEqualThan(key));
	assert(cast(KWSortBucket*, oaBuckets.GetAt(nBucketIndexMax))->IsGreaterOrEqualThan(key));

	// Recherche sequentielle
	for (nIndex = nBucketIndexMin; nIndex < nBucketIndexMax; nIndex++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(nIndex));

		// On a fini si le bucket est plus grand que la cle
		if (bucket->IsGreaterOrEqualThan(key))
		{
			assert(bucket->Contains(key));
			return bucket;
		}
	}

	// Si pas trouve, c'est forcement le dernier bucket
	bucket = cast(KWSortBucket*, oaBuckets.GetAt(nBucketIndexMax));
	assert(bucket->Contains(key));
	return bucket;
}

void KWSortBuckets::AddLineAtKey(KWKey* key, CharVector* cvLine)
{
	KWSortBucket* bucket;

	require(bIsIndexComputed);

	bucket = SearchBucket(key);
	assert(bucket != NULL);
	bucket->AddLine(cvLine);
}

KWSortBucket* KWSortBuckets::GetOverweightBucket(int nChunkSize) const
{
	KWSortBucket* bucket;
	KWSortBucket* resultBucket;
	int i;

	// Demarrage des serveurs pour eviter des multiples demarrages
	PLParallelTask::GetDriver()->StartFileServers();

	// Parcours des chunks
	resultBucket = NULL;
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));

		// Trop gros seulement si ce n'est pas un singleton
		if (not bucket->IsSingleton() and bucket->GetChunkSize() > nChunkSize)
		{
			resultBucket = bucket;
			break;
		}
	}

	// Arret des serveurs de fichiers
	PLParallelTask::GetDriver()->StopFileServers();
	return resultBucket;
}

void KWSortBuckets::CopyFrom(const KWSortBuckets* sbSource)
{
	oaBuckets.CopyFrom(&sbSource->oaBuckets);
	oaDistinctKeys.CopyFrom(&sbSource->oaDistinctKeys);
	ivBucketIndexes.CopyFrom(&sbSource->ivBucketIndexes);
	bIsIndexComputed = sbSource->bIsIndexComputed;
}

boolean KWSortBuckets::Check() const
{
	boolean bOk = true;
	int i;
	KWSortBucket* bucket;
	KWSortBucket* lastBucket;

	// Il doit y avoir au moins un bucket
	if (bOk and oaBuckets.GetSize() == 0)
	{
		bOk = false;
		AddError("No bucket");
	}

	// Verification de la validite et de l'ordre des buckets
	lastBucket = NULL;
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));
		bOk = bOk and bucket->Check();
		if (lastBucket != NULL)
		{
			if (bOk and lastBucket->GetUpperBound()->Compare(bucket->GetLowerBound()) != 0)
			{
				bOk = false;
				lastBucket->AddError("Upper bound should be equal to lower bound of next bucket");
			}
			if (bOk and lastBucket->GetUpperBoundExcluded() == bucket->GetLowerBoundExcluded())
			{
				bOk = false;
				if (lastBucket->GetUpperBoundExcluded())
					lastBucket->AddError(
					    "Upper bound and lower bound of next bucket should not be both excluded");
				else
					lastBucket->AddError(
					    "Upper bound and lower bound of next bucket should not be both included");
			}
		}
		lastBucket = bucket;
	}
	return bOk;
}

boolean KWSortBuckets::CheckFileNames() const
{
	boolean bOk = true;
	int i;
	KWSortBucket* bucket;
	ObjectDictionary odBuckets;

	// Verification de la presence et de l'unicite des noms de fichier
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));

		// Nom de fichier
		if (bucket->GetOutputFileName() == "")
		{
			bucket->AddError("Missing file name");
			bOk = false;
			break;
		}

		// Unicite des noms de fichier
		if (odBuckets.Lookup(bucket->GetOutputFileName()) != NULL)
		{
			bucket->AddError("File name (" + bucket->GetOutputFileName() + ") used by several buckets");
			bOk = false;
			break;
		}
		else
			odBuckets.SetAt(bucket->GetOutputFileName(), bucket);
	}
	return bOk;
}

void KWSortBuckets::Write(ostream& ost) const
{
	int i;
	KWSortBucket* bucket;

	ost << GetClassLabel() << endl;
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));
		ost << "\t" << *bucket << " - ";
		if (bucket->GetSorted())
			ost << "sorted ";
		else
			ost << "not sorted ";
		ost << bucket->GetChunkFileNames()->GetSize() << " chunks - " << bucket->GetOutputFileName() << endl;
	}
}

void KWSortBuckets::WriteWithFirstLines(ostream& ost, int nLineNumber) const
{
	int i;
	KWSortBucket* bucket;

	require(nLineNumber >= 0);

	ost << GetClassLabel() << endl;
	for (i = 0; i < oaBuckets.GetSize(); i++)
	{
		bucket = cast(KWSortBucket*, oaBuckets.GetAt(i));
		ost << "\t" << *bucket << endl;
		KWArtificialDataset::DisplayFileFirstLines(bucket->GetChunkFileNames()->GetAt(0), nLineNumber);
	}
}

const ObjectArray* KWSortBuckets::GetBuckets() const
{
	return &oaBuckets;
}

const ALString KWSortBuckets::GetClassLabel() const
{
	return "Sort buckets";
}

void KWSortBuckets::Test()
{
	const int nKeyNumber = 20;
	KWSortBuckets testBuckets;
	KWSortBucket* bucket;
	ObjectArray oaSplits;
	IntVector ivKeys;
	KWKey searchKey;
	KWKey* key;
	int nKey;
	ALString sKey;
	KWSortBucket mainBucket;
	IntVector ivTestedKeys;
	KWSortBuckets subBuckets;
	KWSortBucket* largeBucket;
	int i;

	// Creation des cles
	for (i = 0; i < nKeyNumber; i++)
	{
		key = new KWKey;
		key->SetSize(1);
		nKey = 100 * nKeyNumber + 10 * (11 + nKeyNumber - nKeyNumber / (i + 1) + i / 3);
		ivKeys.Add(nKey);
		key->SetAt(0, IntToString(nKey));
		oaSplits.Add(key);
	}

	// Creation des buckets a partir des cle
	testBuckets.Build(&mainBucket, &oaSplits);
	testBuckets.IndexBuckets();

	// Recherche d'un bucket a splitter
	largeBucket = testBuckets.GetBucketAt(1);
	assert(not largeBucket->IsSingleton());

	// Creation de deux sous-buckets
	bucket = largeBucket->Clone();
	sKey = bucket->GetLowerBound()->GetAt(0);
	sKey.SetAt(sKey.GetLength() - 1, '5');
	bucket->GetUpperBound()->SetAt(0, sKey);
	subBuckets.AddBucket(bucket);
	bucket = largeBucket->Clone();
	bucket->GetLowerBound()->SetAt(0, sKey);
	subBuckets.AddBucket(bucket);

	// Eclatement du bucket a splitter
	testBuckets.SplitLargeBucket(largeBucket, &subBuckets);
	testBuckets.IndexBuckets();

	// Affchage des buckets
	cout << testBuckets;

	// Creation des cles a tester
	ivTestedKeys.Add(ivKeys.GetAt(0) - 5);
	for (i = 0; i < nKeyNumber; i++)
	{
		nKey = ivKeys.GetAt(i);
		if (i == 0 or nKey != ivKeys.GetAt(i - 1))
		{
			ivTestedKeys.Add(nKey);
			ivTestedKeys.Add(nKey + 5);
		}
	}

	// Recherche de buckets par cle
	searchKey.SetSize(1);
	for (i = 0; i < ivTestedKeys.GetSize(); i++)
	{
		nKey = ivTestedKeys.GetAt(i);
		searchKey.SetAt(0, IntToString(nKey));
		bucket = testBuckets.SearchBucket(&searchKey);
		cout << searchKey << " found in " << *bucket << endl;
	}

	// Nettoyage
	oaSplits.DeleteAll();
	testBuckets.DeleteAll();
}

//////////////////////////////////////////////////////////////
// Implementation de la classe KWSortBucket

KWSortBucket::KWSortBucket()
{
	bLowerBoundExcluded = true;
	bUpperBoundExcluded = true;
	bSorted = false;
	lFileSize = -1;
}

KWSortBucket::~KWSortBucket() {}

KWSortBucket* KWSortBucket::Clone() const
{
	KWSortBucket* clonedBucket = new KWSortBucket;
	clonedBucket->CopyFrom(this);
	return clonedBucket;
}

void KWSortBucket::CopyFrom(const KWSortBucket* bSource)
{
	check(bSource);
	bSorted = bSource->bSorted;
	svChunkFileNames.CopyFrom(&bSource->svChunkFileNames);
	kLowerBound.CopyFrom(&bSource->kLowerBound);
	kUpperBound.CopyFrom(&bSource->kUpperBound);
	cvLines.CopyFrom(&bSource->cvLines);
	bUpperBoundExcluded = bSource->bUpperBoundExcluded;
	bLowerBoundExcluded = bSource->bLowerBoundExcluded;
	sId = bSource->sId;
	sOutputFileName = bSource->sOutputFileName;
	lFileSize = bSource->lFileSize;
}

boolean KWSortBucket::Check() const
{
	boolean bOk = true;

	// Test sur les bornes infinies
	if (bOk and kLowerBound.GetSize() == 0 and not GetLowerBoundExcluded())
	{
		bOk = false;
		AddError("Infinite lower bound should be excluded");
	}
	if (bOk and kUpperBound.GetSize() == 0 and not GetUpperBoundExcluded())
	{
		bOk = false;
		AddError("Infinite upper bound should be excluded");
	}

	// Test sur la taille des cles
	if (bOk and kLowerBound.GetSize() != 0 and kUpperBound.GetSize() != 0 and
	    kLowerBound.GetSize() != kUpperBound.GetSize())
	{
		bOk = false;
		AddError("Bound keys should be of same size");
	}

	// Test sur les singletons
	if (bOk and kLowerBound.GetSize() != 0 and kUpperBound.GetSize() != 0 and
	    kLowerBound.Compare(&kUpperBound) == 0 and (GetLowerBoundExcluded() or GetUpperBoundExcluded()))
	{
		bOk = false;
		AddError("Bounds should be included in case of singleton");
	}

	// Test sur les intervalles standard
	if (bOk and kLowerBound.GetSize() != 0 and kUpperBound.GetSize() != 0 and kLowerBound.Compare(&kUpperBound) > 0)
	{
		bOk = false;
		AddError("Lower bound should be less or equal than upper bound");
	}
	return bOk;
}

void KWSortBucket::Write(ostream& ost) const
{
	ost << GetObjectLabel();
}

const ALString KWSortBucket::GetClassLabel() const
{
	return "Bucket";
}

const ALString KWSortBucket::GetObjectLabel() const
{
	ALString sLabel;

	if (kLowerBound.GetSize() == 0)
		sLabel = "]-inf;";
	else
	{
		if (GetLowerBoundExcluded())
			sLabel = "]" + kLowerBound.GetObjectLabel() + "; ";
		else
			sLabel = "[" + kLowerBound.GetObjectLabel() + "; ";
	}
	if (kUpperBound.GetSize() == 0)
		sLabel += "+inf[";
	else
	{
		if (GetUpperBoundExcluded())
			sLabel += kUpperBound.GetObjectLabel() + "[";
		else
			sLabel += kUpperBound.GetObjectLabel() + "]";
	}
	return sLabel;
}

longint KWSortBucket::GetUsedMemory() const
{
	return sizeof(KWSortBucket) + cvLines.GetUsedMemory() - sizeof(CharVector) + svChunkFileNames.GetUsedMemory() -
	       sizeof(StringVector) + sOutputFileName.GetUsedMemory() - sizeof(ALString) + kLowerBound.GetUsedMemory() -
	       sizeof(KWKey) + kUpperBound.GetUsedMemory() - sizeof(KWKey);
}

void KWSortBucket::SetOutputFileName(const ALString& sValue)
{
	require(sValue != "");
	sOutputFileName = sValue;
}

const ALString& KWSortBucket::GetOutputFileName() const
{
	return sOutputFileName;
}

void KWSortBucket::SetChunkFileNames(StringVector* svValue)
{
	require(svValue != NULL);
	svChunkFileNames.CopyFrom(svValue);
}

void KWSortBucket::AddChunkFileName(const ALString& sChunkFileName)
{
	require(sChunkFileName != "");
	svChunkFileNames.Add(sChunkFileName);
}

const StringVector* KWSortBucket::GetChunkFileNames() const
{
	return &svChunkFileNames;
}

void KWSortBucket::RemoveChunkFileNames()
{
	svChunkFileNames.SetSize(0);
}

void KWSortBucket::AddLine(const CharVector* cvLine)
{
	require(cvLine != NULL);
	int i;

	// Ajout de la ligne
	for (i = 0; i < cvLine->GetSize(); i++)
		cvLines.Add(cvLine->GetAt(i));
}

CharVector* KWSortBucket::GetChunk()
{
	return &cvLines;
}

void KWSortBucket::SetId(const ALString& sValue)
{
	sId = sValue;
}

ALString KWSortBucket::GetId() const
{
	return sId;
}

longint KWSortBucket::GetChunkSize()
{
	int i;

	// Si on n'a pas la taille (deja calculee ou affectee) on la calcule
	if (lFileSize == -1)
	{
		lFileSize = 0;
		if (svChunkFileNames.GetSize() > 0)
		{
			for (i = 0; i < svChunkFileNames.GetSize(); i++)
			{
				lFileSize += PLRemoteFileService::GetFileSize(svChunkFileNames.GetAt(i));
			}
		}
		else
		{
			ensure(GetSorted());
			lFileSize = PLRemoteFileService::GetFileSize(sOutputFileName);
		}
	}
	return lFileSize;
}

void KWSortBucket::SetChunkFileSize(longint lSize)
{
	lFileSize = lSize;
}

void KWSortBucket::SetSorted()
{
	bSorted = true;
}

boolean KWSortBucket::GetSorted() const
{
	return bSorted or IsSingleton();
}

///////////////////////////////////////////////////////////////////////
// Implementation de la classe PLShared_SortBucket

PLShared_SortBucket::PLShared_SortBucket() {}

PLShared_SortBucket::~PLShared_SortBucket() {}

void PLShared_SortBucket::SetBucket(KWSortBucket* bucket)
{
	require(bucket != NULL);
	SetObject(bucket);
}

KWSortBucket* PLShared_SortBucket::GetBucket()
{
	return cast(KWSortBucket*, GetObject());
}

void PLShared_SortBucket::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLShared_Key shared_key;
	KWSortBucket* bucket;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	bucket = cast(KWSortBucket*, o);

	// Ecriture des deux bornes (Key)
	shared_key.SerializeObject(serializer, bucket->GetLowerBound());
	shared_key.SerializeObject(serializer, bucket->GetUpperBound());

	// Ecriture des attributs booleens
	serializer->PutBoolean(bucket->bSorted);

	// Nom du fichier
	serializer->PutString(bucket->GetOutputFileName());

	// Min excluded
	serializer->PutBoolean(bucket->bLowerBoundExcluded);

	// Max excluded
	serializer->PutBoolean(bucket->bUpperBoundExcluded);

	// Id
	serializer->PutString(bucket->sId);
}

void PLShared_SortBucket::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWSortBucket* bucket;
	PLShared_Key shared_key;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	bucket = cast(KWSortBucket*, o);
	shared_key.DeserializeObject(serializer, bucket->GetLowerBound());
	shared_key.DeserializeObject(serializer, bucket->GetUpperBound());
	bucket->bSorted = serializer->GetBoolean();
	bucket->sOutputFileName = serializer->GetString();
	bucket->bLowerBoundExcluded = serializer->GetBoolean();
	bucket->bUpperBoundExcluded = serializer->GetBoolean();
	bucket->SetId(serializer->GetString());
}

int KWSortBucketCompareChunkSize(const void* first, const void* second)
{
	KWSortBucket* aFirst;
	KWSortBucket* aSecond;

	aFirst = cast(KWSortBucket*, *(Object**)first);
	aSecond = cast(KWSortBucket*, *(Object**)second);

	return (aFirst->GetChunk()->GetSize() == aSecond->GetChunk()->GetSize()
		    ? 0
		    : (aFirst->GetChunk()->GetSize() > aSecond->GetChunk()->GetSize() ? -1 : 1));
}
