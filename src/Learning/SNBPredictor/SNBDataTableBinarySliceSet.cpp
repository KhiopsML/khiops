// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBDataTableBinarySliceSet.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetLayout
SNBDataTableBinarySliceSetLayout::SNBDataTableBinarySliceSetLayout()
{
	nInstanceNumber = -1;
	nAttributeNumber = -1;
	nChunkNumber = -1;
	nSliceNumber = -1;
}

SNBDataTableBinarySliceSetLayout::~SNBDataTableBinarySliceSetLayout()
{
	CleanWorkingData();
}

void SNBDataTableBinarySliceSetLayout::Initialize(int nSomeInstanceNumber, int nSomeChunkNumber,
						  int nSomeAttributeNumber, int nSomeSliceNumber)
{
	int nMaxSliceAttributeNumber;
	int nAttribute;
	int nSliceAttributeOffset;
	int nSlice;
	int nSliceAttributeNumber;
	int nSliceAttribute;
	int nMaxChunkInstanceNumber;
	int nChunk;
	int nChunkInstanceNumber;
	int nChunkInstanceOffset;

	require(0 < nSomeChunkNumber and nSomeChunkNumber <= nSomeInstanceNumber);
	require(0 < nSomeSliceNumber and nSomeSliceNumber <= nSomeAttributeNumber);

	// Nettoyage d'une initialisation existante
	CleanWorkingData();

	// Parametrage des dimensiones basiques; le nombre de slices se calcule a partir du max specifie
	nInstanceNumber = nSomeInstanceNumber;
	nAttributeNumber = nSomeAttributeNumber;
	nChunkNumber = nSomeChunkNumber;
	nSliceNumber = nSomeSliceNumber;

	// Dimensionement equitatif des nombres d'instances per chunk
	// Les premiers [nInstanceNumber MOD nChunkNumber] chunks contiennent une instance additionnelle
	ivChunkInstanceNumbers.SetSize(nChunkNumber);
	ivChunkInstanceOffsets.SetSize(nChunkNumber);
	nMaxChunkInstanceNumber = nInstanceNumber / nChunkNumber + 1;
	nChunkInstanceOffset = 0;
	for (nChunk = 0; nChunk < nChunkNumber; nChunk++)
	{
		if (nChunk < nInstanceNumber % nChunkNumber)
			nChunkInstanceNumber = nMaxChunkInstanceNumber;
		else
			nChunkInstanceNumber = nMaxChunkInstanceNumber - 1;

		ivChunkInstanceNumbers.SetAt(nChunk, nChunkInstanceNumber);
		ivChunkInstanceOffsets.SetAt(nChunk, nChunkInstanceOffset);
		nChunkInstanceOffset += nChunkInstanceNumber;
	}

	// Initialisation des tailles des slices avec un nombre equitatif des d'attributs
	// Les premieres [nAttributeNumber % nSliceNumber] slices contiennent un attribut additionnelle
	// On initialise egalement :
	//   - Les nombres d'attributs des slices
	//   - Les offsets des slices dans la liste d'attributs
	//   - Le relation [index d'attribut -> index de sa slice]
	//   - La relation [index d'attribut -> index d'attribut relatif a sa slice]
	//   - Les flag de sparse mode de chaque attribut
	ivSliceAttributeNumbers.SetSize(nSliceNumber);
	ivSliceAttributeOffsets.SetSize(nSliceNumber);
	ivAttributeSliceIndexes.SetSize(nAttributeNumber);
	ivAttributeRelativeIndexes.SetSize(nAttributeNumber);
	nMaxSliceAttributeNumber = nAttributeNumber / nSliceNumber + 1;
	nAttribute = 0;
	nSliceAttributeOffset = 0;
	for (nSlice = 0; nSlice < nSliceNumber; nSlice++)
	{
		if (nSlice < nAttributeNumber % nSliceNumber)
			nSliceAttributeNumber = nMaxSliceAttributeNumber;
		else
			nSliceAttributeNumber = nMaxSliceAttributeNumber - 1;

		for (nSliceAttribute = 0; nSliceAttribute < nSliceAttributeNumber; nSliceAttribute++)
		{
			ivAttributeSliceIndexes.SetAt(nAttribute, nSlice);
			ivAttributeRelativeIndexes.SetAt(nAttribute, nSliceAttribute);
			nAttribute++;
		}
		ivSliceAttributeNumbers.SetAt(nSlice, nSliceAttributeNumber);
		ivSliceAttributeOffsets.SetAt(nSlice, nSliceAttributeOffset);
		nSliceAttributeOffset += nSliceAttributeNumber;
	}
	ivAttributeSparseModes.SetSize(nAttributeNumber);
	ivAttributeSparseModes.Initialize();

	ensure(nChunkInstanceOffset == nInstanceNumber);
	ensure(nSliceAttributeOffset == nAttributeNumber);
	ensure(IsInitialized());
	ensure(Check());
}

boolean SNBDataTableBinarySliceSetLayout::IsInitialized() const
{
	return nAttributeNumber > 0 and nInstanceNumber > 0 and nSliceNumber > 0 and nChunkNumber > 0 and
	       ivChunkInstanceNumbers.GetSize() > 0 and ivSliceAttributeNumbers.GetSize() > 0;
}

void SNBDataTableBinarySliceSetLayout::CleanWorkingData()
{
	nInstanceNumber = -1;
	nAttributeNumber = -1;
	nChunkNumber = -1;
	nSliceNumber = -1;
	ivChunkInstanceNumbers.SetSize(0);
	ivSliceAttributeNumbers.SetSize(0);

	ensure(not IsInitialized());
}

int SNBDataTableBinarySliceSetLayout::GetInstanceNumber() const
{
	require(IsInitialized());
	return nInstanceNumber;
}

int SNBDataTableBinarySliceSetLayout::GetChunkNumber() const
{
	require(IsInitialized());
	return nChunkNumber;
}

int SNBDataTableBinarySliceSetLayout::GetMaxChunkInstanceNumber() const
{
	require(IsInitialized());

	// Le premier chunk contient toujours le nombre maximal d'instances
	return GetInstanceNumberAtChunk(0);
}

int SNBDataTableBinarySliceSetLayout::GetInstanceNumberAtChunk(int nChunk) const
{
	require(IsInitialized());
	require(0 <= nChunk and nChunk < GetChunkNumber());
	return ivChunkInstanceNumbers.GetAt(nChunk);
}

int SNBDataTableBinarySliceSetLayout::GetInstanceOffsetAtChunk(int nChunk) const
{
	require(IsInitialized());
	require(0 <= nChunk and nChunk < GetChunkNumber());
	return ivChunkInstanceOffsets.GetAt(nChunk);
}

int SNBDataTableBinarySliceSetLayout::GetAttributeNumber() const
{
	require(IsInitialized());
	return nAttributeNumber;
}

int SNBDataTableBinarySliceSetLayout::GetSliceNumber() const
{
	require(IsInitialized());
	return nSliceNumber;
}

int SNBDataTableBinarySliceSetLayout::GetMaxSliceAttributeNumber() const
{
	require(IsInitialized());

	// La premiere slice contient toujours le nombre maximal d'attributs
	return GetAttributeNumberAtSlice(0);
}

int SNBDataTableBinarySliceSetLayout::GetAttributeNumberAtSlice(int nSlice) const
{
	require(IsInitialized());
	require(0 <= nSlice and nSlice < GetSliceNumber());
	return ivSliceAttributeNumbers.GetAt(nSlice);
}

int SNBDataTableBinarySliceSetLayout::GetAttributeOffsetAtSlice(int nSlice) const
{
	require(IsInitialized());
	require(0 <= nSlice and nSlice < GetSliceNumber());
	return ivSliceAttributeOffsets.GetAt(nSlice);
}

int SNBDataTableBinarySliceSetLayout::GetSliceIndexAtAttribute(int nAttribute) const
{
	require(IsInitialized());
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());
	return ivAttributeSliceIndexes.GetAt(nAttribute);
}

int SNBDataTableBinarySliceSetLayout::GetRelativeIndexAtAttribute(int nAttribute) const
{
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());
	require(IsInitialized());
	return ivAttributeRelativeIndexes.GetAt(nAttribute);
}

boolean SNBDataTableBinarySliceSetLayout::IsAttributeSparseAt(int nAttribute) const
{
	require(IsInitialized());
	return (boolean)ivAttributeSparseModes.GetAt(nAttribute);
}

boolean SNBDataTableBinarySliceSetLayout::Check() const
{
	boolean bOk = true;
	int nChunk;
	int nInstanceCount;
	int nSlice;
	int nAttributeCount;

	if (IsInitialized())
	{
		// Verification des rangs du nombre d'attributs, chunks et slices
		bOk = bOk and 0 < nChunkNumber and nChunkNumber <= nInstanceNumber;
		bOk = bOk and 0 < nSliceNumber and nSliceNumber <= nAttributeNumber;

		// Verification des longueurs des tableaux
		bOk = bOk and ivChunkInstanceNumbers.GetSize() == nChunkNumber;
		bOk = bOk and ivAttributeRelativeIndexes.GetSize() == nAttributeNumber;
		bOk = bOk and ivAttributeSliceIndexes.GetSize() == nAttributeNumber;
		bOk = bOk and ivSliceAttributeNumbers.GetSize() == nSliceNumber;
		bOk = bOk and ivSliceAttributeOffsets.GetSize() == nSliceNumber;

		// Coherence du nombre d'instances
		nInstanceCount = 0;
		for (nChunk = 0; nChunk < nChunkNumber; nChunk++)
			nInstanceCount += ivChunkInstanceNumbers.GetAt(nChunk);
		bOk = bOk and nInstanceCount == nInstanceNumber;

		// Coherence des nombre d'instances par chunk et de ses offsets
		bOk = bOk and ivChunkInstanceOffsets.GetAt(0) == 0;
		for (nChunk = 1; nChunk < nChunkNumber; nChunk++)
		{
			bOk = bOk and
			      ivChunkInstanceOffsets.GetAt(nChunk - 1) + ivChunkInstanceNumbers.GetAt(nChunk - 1) ==
				  ivChunkInstanceOffsets.GetAt(nChunk);
		}

		// Coherence du nombre d'attributs
		nAttributeCount = 0;
		for (nSlice = 0; nSlice < nSliceNumber; nSlice++)
		{
			bOk = bOk and ivSliceAttributeNumbers.GetAt(nSlice) > 0;
			nAttributeCount += ivSliceAttributeNumbers.GetAt(nSlice);
		}
		bOk = bOk and nAttributeCount == nAttributeNumber;

		// Coherence des nombre d'attributs par slice et de ses offsets
		bOk = bOk and ivSliceAttributeOffsets.GetAt(0) == 0;
		for (nSlice = 1; nSlice < nSliceNumber; nSlice++)
		{
			bOk = bOk and
			      ivSliceAttributeOffsets.GetAt(nSlice - 1) + ivSliceAttributeNumbers.GetAt(nSlice - 1) ==
				  ivSliceAttributeOffsets.GetAt(nSlice);
		}
	}
	else
		bOk = false;

	return bOk;
}

void SNBDataTableBinarySliceSetLayout::Write(ostream& ost) const
{
	int nChunk;
	int nSlice;

	require(IsInitialized());

	ost << GetClassLabel() << "\n"
	    << "Instances x Attributes = " << IntToString(GetInstanceNumber()) << " x "
	    << IntToString(GetAttributeNumber()) << "\n"
	    << "Chunks x Slices = " << IntToString(GetChunkNumber()) << " x " << IntToString(GetSliceNumber()) << "\n";

	ost << "Chunk Dimensions (instance offset, instance number)\n";
	for (nChunk = 0; nChunk < GetChunkNumber(); nChunk++)
		ost << "(" << GetInstanceOffsetAtChunk(nChunk) << ", " << GetInstanceNumberAtChunk(nChunk) << ") ";
	ost << "\n";

	ost << "Slice Dimensions (attribute offset, attribute number)\n";
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
		ost << "(" << GetAttributeOffsetAtSlice(nSlice) << ", " << GetAttributeNumberAtSlice(nSlice) << ") ";
}

const ALString SNBDataTableBinarySliceSetLayout::GetClassLabel() const
{
	return "Data Table Slice Set Layout";
}

longint SNBDataTableBinarySliceSetLayout::ComputeNecessaryMemory(int nInstanceNumber, int nChunkNumber,
								 int nAttributeNumber, int nSliceNumber)
{
	ObjectArray oaDummy;

	// Formule de l'estimation :
	//   Objets de la instance +
	//   2 IntVector's de taille nChunkNumber (ivChunkInstanceNumbers & ivChunkInstanceOffsets) +
	//   2 IntVector's de taille nSliceNumber (ivSliceAttributeNumber & ivSliceAttributeOffsets)
	//   2 IntVector's de taille nAttributeNumber (ivAttributeSliceIndexes & ivAttributeRelativeIndexes)
	//   Tableau contenant nChunkNumber IntVector's de taille nSliceNumber (oaBlockSizes)
	//   Tableau contenant nChunkNumber LongintVector's de taille nSliceNumber (oaBlockOffsets)
	return sizeof(SNBDataTableBinarySliceSetLayout) + 2 * (longint)nChunkNumber * sizeof(int) +
	       2 * (longint)nSliceNumber * sizeof(int) + 2 * (longint)nAttributeNumber * sizeof(int) +
	       nChunkNumber *
		   (oaDummy.GetUsedMemoryPerElement() + sizeof(IntVector) + (longint)nSliceNumber * sizeof(int)) +
	       nChunkNumber * (oaDummy.GetUsedMemoryPerElement() + sizeof(LongintVector) +
			       (longint)nSliceNumber * sizeof(longint));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetAttribute

SNBDataTableBinarySliceSetAttribute::SNBDataTableBinarySliceSetAttribute()
{
	nIndex = -1;
	nDataPreparationClassIndex = -1;
	sRecodedAttributeName = "";
	sPreparedAttributeName = "";
	sNativeAttributeName = "";
	dataPreparationStats = NULL;
}

SNBDataTableBinarySliceSetAttribute::~SNBDataTableBinarySliceSetAttribute() {}

void SNBDataTableBinarySliceSetAttribute::SetNativeAttributeName(const ALString& sName)
{
	sNativeAttributeName = sName;
}

const ALString& SNBDataTableBinarySliceSetAttribute::GetNativeAttributeName() const
{
	return sNativeAttributeName;
}

void SNBDataTableBinarySliceSetAttribute::SetPreparedAttributeName(const ALString& sName)
{
	sPreparedAttributeName = sName;
}

const ALString& SNBDataTableBinarySliceSetAttribute::GetPreparedAttributeName() const
{
	return sPreparedAttributeName;
}

void SNBDataTableBinarySliceSetAttribute::SetRecodedAttributeName(const ALString& sName)
{
	sRecodedAttributeName = sName;
}

const ALString& SNBDataTableBinarySliceSetAttribute::GetRecodedAttributeName() const
{
	return sRecodedAttributeName;
}

void SNBDataTableBinarySliceSetAttribute::SetIndex(int nValue)
{
	require(0 <= nValue);
	nIndex = nValue;
}

int SNBDataTableBinarySliceSetAttribute::GetIndex() const
{
	return nIndex;
}

void SNBDataTableBinarySliceSetAttribute::SetDataPreparationClassIndex(int nValue)
{
	require(0 <= nValue);
	nDataPreparationClassIndex = nValue;
}

int SNBDataTableBinarySliceSetAttribute::GetDataPreparationClassIndex() const
{
	return nDataPreparationClassIndex;
}

void SNBDataTableBinarySliceSetAttribute::InitializeFromDataPreparationAttribute(
    KWDataPreparationAttribute* dataPreparationAttribute)
{
	require(dataPreparationAttribute != NULL);
	require(dataPreparationAttribute->Check());

	// Parametrage des statistiques de preparation de l'attribut
	dataPreparationStats = dataPreparationAttribute->GetPreparedStats();

	// Creation de la table de probabilites conditionelles
	conditionalProbas.ImportDataGridStats(dataPreparationStats->GetPreparedDataGridStats(), false, true);

	ensure(dataPreparationStats->Check());
	ensure(conditionalProbas.Check());
	ensure(GetTargetPartition()->GetAttributeType() == KWType::Symbol or
	       GetTargetPartition()->GetAttributeType() == KWType::Continuous);
}

double SNBDataTableBinarySliceSetAttribute::GetConstructionCost() const
{
	require(dataPreparationStats != NULL);
	return dataPreparationStats->GetConstructionCost();
}

double SNBDataTableBinarySliceSetAttribute::GetNullConstructionCost() const
{
	require(dataPreparationStats != NULL);
	return dataPreparationStats->GetNullConstructionCost();
}

double SNBDataTableBinarySliceSetAttribute::GetPreparationCost() const
{
	require(dataPreparationStats != NULL);
	return dataPreparationStats->GetPreparationCost();
}

double SNBDataTableBinarySliceSetAttribute::GetLevel() const
{
	require(dataPreparationStats != NULL);
	return dataPreparationStats->GetLevel();
}

Continuous SNBDataTableBinarySliceSetAttribute::GetLnSourceConditionalProb(int nSourceModalityIndex,
									   int nTargetModalityIndex) const
{
	return conditionalProbas.GetSourceConditionalLogProbAt(nSourceModalityIndex, nTargetModalityIndex);
}

const KWDataGridStats* SNBDataTableBinarySliceSetAttribute::GetPreparedDataGridStats() const
{
	require(dataPreparationStats != NULL);
	return dataPreparationStats->GetPreparedDataGridStats();
}

const KWDGSAttributePartition* SNBDataTableBinarySliceSetAttribute::GetTargetPartition() const
{
	require(dataPreparationStats != NULL);
	return GetPreparedDataGridStats()->GetAttributeAt(GetPreparedDataGridStats()->GetFirstTargetAttributeIndex());
}

boolean SNBDataTableBinarySliceSetAttribute::Check() const
{
	boolean bOk = true;

	bOk = bOk and 0 <= nIndex;
	bOk = bOk and 0 <= nDataPreparationClassIndex;
	bOk = bOk and dataPreparationStats != NULL;
	bOk = bOk and dataPreparationStats->Check();
	bOk = bOk and conditionalProbas.Check();

	return bOk;
}

void SNBDataTableBinarySliceSetAttribute::Write(ostream& ost) const
{
	require(Check());
	ost << "Attribute #" << GetIndex() << ": " << GetNativeAttributeName() << "\n"
	    << "Prepared attribute name: " << GetPreparedAttributeName() << "\n"
	    << "Recoded attribute name: " << GetRecodedAttributeName() << "\n"
	    << "Initial class index: " << GetDataPreparationClassIndex() << "\n"
	    << "Preparation stats: ";
	dataPreparationStats->WriteLineReport(ost);
	ost << "\nProbability table:\n" << conditionalProbas << "\n";
}

longint SNBDataTableBinarySliceSetAttribute::GetUsedMemory() const
{
	longint lUsedMemory;

	// On ne considere pas le KWAttributStats facade
	lUsedMemory = sizeof(SNBDataTableBinarySliceSetAttribute) + sNativeAttributeName.GetUsedMemory() +
		      sPreparedAttributeName.GetUsedMemory() + sRecodedAttributeName.GetUsedMemory() +
		      conditionalProbas.GetUsedMemory();

	return lUsedMemory;
}

int SNBDataTableBinarySliceSetAttributeCompareNativeAttributeName(const void* elem1, const void* elem2)
{
	SNBDataTableBinarySliceSetAttribute* attribute1;
	SNBDataTableBinarySliceSetAttribute* attribute2;

	require(elem1 != NULL and elem2 != NULL);

	// Acces aux objets
	attribute1 = cast(SNBDataTableBinarySliceSetAttribute*, *(Object**)elem1);
	attribute2 = cast(SNBDataTableBinarySliceSetAttribute*, *(Object**)elem2);

	return attribute1->GetNativeAttributeName().Compare(attribute2->GetNativeAttributeName());
}

int SNBDataTableBinarySliceSetAttributeCompareDataPreparationClassIndex(const void* elem1, const void* elem2)
{
	SNBDataTableBinarySliceSetAttribute* attribute1;
	SNBDataTableBinarySliceSetAttribute* attribute2;
	int nClassIndex1;
	int nClassIndex2;
	int nCompare;

	require(elem1 != NULL and elem2 != NULL);

	// Acces aux objets
	attribute1 = cast(SNBDataTableBinarySliceSetAttribute*, *(Object**)elem1);
	attribute2 = cast(SNBDataTableBinarySliceSetAttribute*, *(Object**)elem2);

	// Comparaison par index original de la classe
	nClassIndex1 = attribute1->GetDataPreparationClassIndex();
	nClassIndex2 = attribute2->GetDataPreparationClassIndex();
	if (nClassIndex1 == nClassIndex2)
		nCompare = 0;
	else if (nClassIndex1 < nClassIndex2)
		nCompare = -1;
	else
		nCompare = 1;

	return nCompare;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DataTableBinarySliceSetAttribute

PLShared_DataTableBinarySliceSetAttribute::PLShared_DataTableBinarySliceSetAttribute() {}

PLShared_DataTableBinarySliceSetAttribute::~PLShared_DataTableBinarySliceSetAttribute() {}

void PLShared_DataTableBinarySliceSetAttribute::SetDataTableBinarySliceSetAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(attribute != NULL);
	SetObject(attribute);
}

SNBDataTableBinarySliceSetAttribute*
PLShared_DataTableBinarySliceSetAttribute::GetDataTableBinarySliceSetAttribute() const
{
	return cast(SNBDataTableBinarySliceSetAttribute*, GetObject());
}

void PLShared_DataTableBinarySliceSetAttribute::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	SNBDataTableBinarySliceSetAttribute* attribute;
	PLShared_AttributeStats shared_HelperAttributeStats;
	PLShared_AttributePairStats shared_HelperAttributePairStats;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'attribut
	attribute = cast(SNBDataTableBinarySliceSetAttribute*, o);

	// Serialisation
	serializer->PutString(attribute->sNativeAttributeName);
	serializer->PutString(attribute->sPreparedAttributeName);
	serializer->PutString(attribute->sRecodedAttributeName);
	serializer->PutInt(attribute->nIndex);
	serializer->PutInt(attribute->nDataPreparationClassIndex);
}

void PLShared_DataTableBinarySliceSetAttribute::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	SNBDataTableBinarySliceSetAttribute* attribute;
	PLShared_AttributeStats shared_HelperAttributeStats;
	PLShared_AttributePairStats shared_HelperAttributePairStats;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'attribut
	attribute = cast(SNBDataTableBinarySliceSetAttribute*, o);
	assert(attribute->dataPreparationStats == NULL);

	// Deserialization
	attribute->sNativeAttributeName = serializer->GetString();
	attribute->sPreparedAttributeName = serializer->GetString();
	attribute->sRecodedAttributeName = serializer->GetString();
	attribute->nIndex = serializer->GetInt();
	attribute->nDataPreparationClassIndex = serializer->GetInt();
}

Object* PLShared_DataTableBinarySliceSetAttribute::Create() const
{
	return new SNBDataTableBinarySliceSetAttribute;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetSchema

SNBDataTableBinarySliceSetSchema::SNBDataTableBinarySliceSetSchema() {}

SNBDataTableBinarySliceSetSchema::~SNBDataTableBinarySliceSetSchema()
{
	CleanWorkingData();
}

void SNBDataTableBinarySliceSetSchema::InitializeFromDataPreparationClass(
    KWDataPreparationClass* dataPreparationClass, ObjectArray* oaUsableDataPreparationAttributes,
    StringVector* svUsableDataPreparationAttributeRecodedNames)
{
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	NumericKeyDictionary nkdAttributesByDataPreparationAttribute;

	require(oaUsableDataPreparationAttributes->GetSize() <=
		dataPreparationClass->GetDataPreparationAttributes()->GetSize());
	require(oaUsableDataPreparationAttributes->GetSize() ==
		svUsableDataPreparationAttributeRecodedNames->GetSize());

	// Initialisation presque complete des attributs
	// L'index de l'attribut relatif a la KWDataPreparationClasse s'initialise dans une deuxieme passe
	for (nAttribute = 0; nAttribute < oaUsableDataPreparationAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaUsableDataPreparationAttributes->GetAt(nAttribute));
		assert(SNBDataTableBinarySliceSet::IsPredictive(dataPreparationAttribute));

		// Initialisation de l'attribut
		attribute = new SNBDataTableBinarySliceSetAttribute;
		attribute->SetIndex(oaAttributes.GetSize());
		attribute->SetNativeAttributeName(dataPreparationAttribute->ComputeNativeAttributeName());
		attribute->SetPreparedAttributeName(dataPreparationAttribute->GetPreparedAttribute()->GetName());
		attribute->SetRecodedAttributeName(svUsableDataPreparationAttributeRecodedNames->GetAt(nAttribute));
		attribute->SetDataPreparationClassIndex(dataPreparationAttribute->GetIndex());
		attribute->InitializeFromDataPreparationAttribute(dataPreparationAttribute);
		oaAttributes.Add(attribute);
		odAttributesByNativeAttributeName.SetAt(dataPreparationAttribute->ComputeNativeAttributeName(),
							attribute);
		odAttributesByRecodedAttributeName.SetAt(
		    svUsableDataPreparationAttributeRecodedNames->GetAt(nAttribute), attribute);
	}
	ensure(oaAttributes.GetSize() <= dataPreparationClass->GetDataPreparationAttributes()->GetSize());
	ensure(IsInitialized() or oaAttributes.GetSize() == 0);
	ensure(Check());
}

void SNBDataTableBinarySliceSetSchema::InitializeFromAttributes(const ObjectArray* oaSomeAttributes)
{
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	require(oaSomeAttributes != NULL);
	require(oaSomeAttributes->GetSize() > 0);

	oaAttributes.CopyFrom(oaSomeAttributes);
	for (nAttribute = 0; nAttribute < oaAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nAttribute));
		odAttributesByNativeAttributeName.SetAt(attribute->GetNativeAttributeName(), attribute);
		odAttributesByRecodedAttributeName.SetAt(attribute->GetRecodedAttributeName(), attribute);
	}
	ensure(IsInitialized());
}

boolean SNBDataTableBinarySliceSetSchema::IsInitialized() const
{
	return oaAttributes.GetSize() > 0 and odAttributesByNativeAttributeName.GetCount() > 0 and
	       odAttributesByRecodedAttributeName.GetCount() > 0;
}

void SNBDataTableBinarySliceSetSchema::CleanWorkingData()
{
	oaAttributes.DeleteAll();
}

int SNBDataTableBinarySliceSetSchema::GetAttributeNumber() const
{
	return oaAttributes.GetSize();
}

SNBDataTableBinarySliceSetAttribute* SNBDataTableBinarySliceSetSchema::GetAttributeAt(int nAttribute) const
{
	require(IsInitialized());
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());
	return cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nAttribute));
}

SNBDataTableBinarySliceSetAttribute*
SNBDataTableBinarySliceSetSchema::GetAttributeAtNativeName(const ALString& sAttributeName) const
{
	require(IsInitialized());
	return cast(SNBDataTableBinarySliceSetAttribute*, odAttributesByNativeAttributeName.Lookup(sAttributeName));
}

SNBDataTableBinarySliceSetAttribute*
SNBDataTableBinarySliceSetSchema::GetAttributeAtRecodedAttribute(const KWAttribute* recodedAttribute) const
{
	require(IsInitialized());
	return cast(SNBDataTableBinarySliceSetAttribute*,
		    odAttributesByRecodedAttributeName.Lookup(recodedAttribute->GetName()));
}

boolean SNBDataTableBinarySliceSetSchema::Check() const
{
	boolean bOk = true;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	if (IsInitialized())
	{
		// Coherence du nombre d'attributs
		bOk = bOk and oaAttributes.GetSize() == odAttributesByNativeAttributeName.GetCount();
		bOk = bOk and oaAttributes.GetSize() == odAttributesByRecodedAttributeName.GetCount();

		// Verification des SNBDataTableBinarySliceSetAttribute's
		for (nAttribute = 0; nAttribute < oaAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nAttribute));
			bOk = bOk and attribute->Check();
		}
	}
	return bOk;
}

void SNBDataTableBinarySliceSetSchema::Write(ostream& ost) const
{
	int nAttribute;

	require(Check());

	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		ost << *(GetAttributeAt(nAttribute)) << endl;
}

longint SNBDataTableBinarySliceSetSchema::GetUsedMemory() const
{
	return sizeof(SNBDataTableBinarySliceSetSchema) + oaAttributes.GetOverallUsedMemory() +
	       odAttributesByNativeAttributeName.GetUsedMemory() + odAttributesByRecodedAttributeName.GetUsedMemory();
}

longint SNBDataTableBinarySliceSetSchema::ComputeNecessaryMemory(KWClassStats* classStats)
{
	longint lSchemaMemory;
	ObjectArray* oaAllPreparedStats;
	int nAttribute;
	KWDataPreparationStats* dataPreparationStats;
	longint lTargetValueNumber;
	longint lSourceValueNumber;
	longint lCellNumber;
	longint lTotalInformativeAttributeNumber;
	ObjectArray dummyArray;
	ObjectDictionary dummyDictionary;

	// Memoire propre et pointeurs aux attributs
	lSchemaMemory = sizeof(SNBDataTableBinarySliceSetSchema);

	// Memoire des noms des attributs et des contenus des tables de probabilites pour les attributs informatifs
	oaAllPreparedStats = classStats->GetAllPreparedStats();
	for (nAttribute = 0; nAttribute < oaAllPreparedStats->GetSize(); nAttribute++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaAllPreparedStats->GetAt(nAttribute));

		if (dataPreparationStats->IsInformative())
		{
			lSourceValueNumber = dataPreparationStats->GetPreparedDataGridStats()->ComputeSourceGridSize();
			lTargetValueNumber = dataPreparationStats->GetPreparedDataGridStats()->ComputeTargetGridSize();
			lCellNumber = lSourceValueNumber * lTargetValueNumber;
			lSchemaMemory += sizeof(SNBDataTableBinarySliceSetAttribute) +
					 3 * (dataPreparationStats->GetSortName().GetUsedMemory() + lB) +
					 lSourceValueNumber * sizeof(int) + lTargetValueNumber * sizeof(int) +
					 lCellNumber * sizeof(Continuous);
		}
	}

	// Memoire des conteneurs des attributs
	lTotalInformativeAttributeNumber = (longint)classStats->GetTotalInformativeAttributeNumber();
	lSchemaMemory += lTotalInformativeAttributeNumber *
			 (dummyArray.GetUsedMemoryPerElement() + 2 * dummyDictionary.GetUsedMemoryPerElement());
	return lSchemaMemory;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetRandomizedAttributeIterator

SNBDataTableBinarySliceSetRandomizedAttributeIterator::SNBDataTableBinarySliceSetRandomizedAttributeIterator()
{
	schema = NULL;
	layout = NULL;
}

SNBDataTableBinarySliceSetRandomizedAttributeIterator::~SNBDataTableBinarySliceSetRandomizedAttributeIterator()
{
	CleanWorkingData();
}

void SNBDataTableBinarySliceSetRandomizedAttributeIterator::Initialize(
    const SNBDataTableBinarySliceSetSchema* someSchema, const SNBDataTableBinarySliceSetLayout* someLayout)
{
	int nAttribute;
	int nAttributeOffset;
	int nSlice;
	ObjectArray* oaSliceAttributes;

	require(someSchema != NULL);
	require(someSchema->Check());
	require(someLayout != NULL);
	require(someLayout->Check());
	require(someSchema->GetAttributeNumber() == someLayout->GetAttributeNumber());

	// Remise a l'etat vide
	CleanWorkingData();

	// Sauvegarde des parametres initialisation
	schema = someSchema;
	layout = someLayout;

	// Initialisation des tableaux d'attributs pour l'acces randomise: L'ordre initial est l'original
	oaRandomizedAttributes.SetSize(layout->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < schema->GetAttributeNumber(); nAttribute++)
		oaRandomizedAttributes.SetAt(nAttribute, schema->GetAttributeAt(nAttribute));

	// Memorisation de l'ordre original dans une suite de tableaux
	// Ceci permets de randomiser les attributs localement a chaque slice du layout
	for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
	{
		nAttributeOffset = layout->GetAttributeOffsetAtSlice(nSlice);
		oaSliceAttributes = new ObjectArray;
		oaSliceAttributes->SetSize(layout->GetAttributeNumberAtSlice(nSlice));
		for (nAttribute = 0; nAttribute < oaSliceAttributes->GetSize(); nAttribute++)
			oaSliceAttributes->SetAt(nAttribute, schema->GetAttributeAt(nAttributeOffset + nAttribute));
		oaInitialSliceAttributes.Add(oaSliceAttributes);
	}
	ensure(IsInitialized());
	ensure(Check());
}

boolean SNBDataTableBinarySliceSetRandomizedAttributeIterator::IsInitialized() const
{
	return schema != NULL and layout != NULL and oaInitialSliceAttributes.GetSize() > 0 and
	       oaRandomizedAttributes.GetSize() > 0;
}

void SNBDataTableBinarySliceSetRandomizedAttributeIterator::CleanWorkingData()
{
	int nSlice;
	ObjectArray* oaSliceAttributes;

	schema = NULL;
	layout = NULL;

	oaRandomizedAttributes.RemoveAll();
	for (nSlice = 0; nSlice < oaInitialSliceAttributes.GetSize(); nSlice++)
	{
		oaSliceAttributes = cast(ObjectArray*, oaInitialSliceAttributes.GetAt(nSlice));
		delete oaSliceAttributes;
	}
	oaInitialSliceAttributes.RemoveAll();

	ensure(not IsInitialized());
}

SNBDataTableBinarySliceSetAttribute*
SNBDataTableBinarySliceSetRandomizedAttributeIterator::GetRandomAttributeAt(int nRandomAttribute) const
{
	require(IsInitialized());
	require(0 <= nRandomAttribute and nRandomAttribute < schema->GetAttributeNumber());
	return cast(SNBDataTableBinarySliceSetAttribute*, oaRandomizedAttributes.GetAt(nRandomAttribute));
}

void SNBDataTableBinarySliceSetRandomizedAttributeIterator::Shuffle()
{
	ObjectArray oaSliceShuffler;
	int nAttributeOffset;
	int nSlice;
	ObjectArray* oaSliceAttributes;
	ObjectArray oaAttributeShuffler;
	int nAttribute;

	require(IsInitialized());
	require(Check());

	// Randomisation des indexes de slice et des attributs (randomisation local a la slice)
	oaSliceShuffler.CopyFrom(&oaInitialSliceAttributes);
	oaSliceShuffler.Shuffle();
	nAttributeOffset = 0;
	for (nSlice = 0; nSlice < oaSliceShuffler.GetSize(); nSlice++)
	{
		oaSliceAttributes = cast(ObjectArray*, oaSliceShuffler.GetAt(nSlice));
		oaAttributeShuffler.CopyFrom(oaSliceAttributes);
		oaAttributeShuffler.Shuffle();
		for (nAttribute = 0; nAttribute < oaAttributeShuffler.GetSize(); nAttribute++)
			oaRandomizedAttributes.SetAt(nAttributeOffset + nAttribute,
						     oaAttributeShuffler.GetAt(nAttribute));
		nAttributeOffset += oaAttributeShuffler.GetSize();
	}
	ensure(nAttributeOffset == oaRandomizedAttributes.GetSize());
	ensure(IsInitialized());
	ensure(Check());
}

void SNBDataTableBinarySliceSetRandomizedAttributeIterator::Restore()
{
	int nAttribute;

	require(IsInitialized());
	require(Check());

	for (nAttribute = 0; nAttribute < schema->GetAttributeNumber(); nAttribute++)
		oaRandomizedAttributes.SetAt(nAttribute, schema->GetAttributeAt(nAttribute));

	ensure(IsInitialized());
	ensure(Check());
}

boolean SNBDataTableBinarySliceSetRandomizedAttributeIterator::Check() const
{
	boolean bOk = true;
	int nTotalAttributes;
	int nSlice;
	ObjectArray* oaSliceAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	if (IsInitialized())
	{
		// Concordance du nombre d'attributs
		nTotalAttributes = 0;
		for (nSlice = 0; nSlice < oaInitialSliceAttributes.GetSize(); nSlice++)
		{
			oaSliceAttributes = cast(ObjectArray*, oaInitialSliceAttributes.GetAt(nSlice));
			nTotalAttributes += oaSliceAttributes->GetSize();
		}
		bOk = bOk and nTotalAttributes == schema->GetAttributeNumber();
		bOk = bOk and oaRandomizedAttributes.GetSize() == schema->GetAttributeNumber();

		// Verification que les pointeurs de la randomisation pointent aux memes objets ceux du schema
		if (bOk)
		{
			for (nAttribute = 0; nAttribute < schema->GetAttributeNumber(); nAttribute++)
			{
				attribute = cast(SNBDataTableBinarySliceSetAttribute*,
						 oaRandomizedAttributes.GetAt(nAttribute));
				bOk = bOk and attribute == schema->GetAttributeAt(attribute->GetIndex());
			}
		}
	}
	return bOk;
}

longint SNBDataTableBinarySliceSetRandomizedAttributeIterator::ComputeNecessaryMemory(int nAttributeNumber,
										      int nSliceNumber)
{
	int nMaxAttributesPerSlice;
	ObjectArray oaDummy;

	require(0 < nSliceNumber and nSliceNumber <= nAttributeNumber);

	// Formule pour la memoire (les SNBDataTableBinarySliceSetAttribute pointes n'appartient pas a cette classe):
	//   Tableau de nSliceNumber tableaux contenant nMaxAttributePerSlice KWLearningAttribute*'s
	//   (oaInitialSliceAttributes) + Tableau de nAttributeNumber SNBDataTableBinarySliceSetAttribute*'s
	//   (oaRandomizedAttributes)
	nMaxAttributesPerSlice = nAttributeNumber / nSliceNumber;
	return nSliceNumber * (oaDummy.GetUsedMemoryPerElement() + sizeof(ObjectArray) +
			       nMaxAttributesPerSlice * oaDummy.GetUsedMemoryPerElement()) +
	       nAttributeNumber * oaDummy.GetUsedMemoryPerElement();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetColumn
//
SNBDataTableBinarySliceSetColumn::SNBDataTableBinarySliceSetColumn()
{
	bIsSparse = false;
}

SNBDataTableBinarySliceSetColumn::~SNBDataTableBinarySliceSetColumn() {}

void SNBDataTableBinarySliceSetColumn::SetSparse(boolean bSparseValue)
{
	bIsSparse = bSparseValue;
}

boolean SNBDataTableBinarySliceSetColumn::IsSparse() const
{
	return bIsSparse;
}

int SNBDataTableBinarySliceSetColumn::GetValueNumber() const
{
	int nValueNumber;

	if (IsSparse())
		nValueNumber = GetDataSize() / 2;
	else
		nValueNumber = GetDataSize();

	return nValueNumber;
}

int SNBDataTableBinarySliceSetColumn::GetDenseValueAt(int nValueIndex) const
{
	require(not IsSparse());
	require(0 <= nValueIndex and nValueIndex < GetValueNumber());
	return ivData.GetAt(nValueIndex);
}

int SNBDataTableBinarySliceSetColumn::GetSparseValueAt(int nValueIndex) const
{
	require(IsSparse());
	require(0 <= nValueIndex and nValueIndex < GetValueNumber());
	return ivData.GetAt(2 * nValueIndex + 1);
}

int SNBDataTableBinarySliceSetColumn::GetSparseValueInstanceIndexAt(int nValueIndex) const
{
	require(IsSparse());
	require(0 <= nValueIndex and nValueIndex < GetValueNumber());
	return ivData.GetAt(2 * nValueIndex);
}

boolean SNBDataTableBinarySliceSetColumn::SetDataSize(int nSize)
{
	return ivData.SetLargeSize(nSize);
}

int SNBDataTableBinarySliceSetColumn::GetDataSize() const
{
	return ivData.GetSize();
}

void SNBDataTableBinarySliceSetColumn::SetDataAt(int nDataIndex, int nDataValue)
{
	ivData.SetAt(nDataIndex, nDataValue);
}

int SNBDataTableBinarySliceSetColumn::GetDataAt(int nDataIndex) const
{
	return ivData.GetAt(nDataIndex);
}

void SNBDataTableBinarySliceSetColumn::AddData(int nDataValue)
{
	ivData.Add(nDataValue);
}

boolean SNBDataTableBinarySliceSetColumn::Check() const
{
	boolean bOk = true;
	int nData;
	int nPreviousInstanceIndex;

	// En sparse il doit avoir une quantite pair de donnees
	bOk = bOk and (not bIsSparse or ivData.GetSize() % 2 == 0);

	// Verification que toutes les donnees sont des indices positifs
	for (nData = 0; nData < ivData.GetSize(); nData++)
		bOk = bOk and ivData.GetAt(nData) >= 0;

	// En cas sparse: Verification que les indexes d'instance sont croisantes
	if (bIsSparse and ivData.GetSize() > 0)
	{
		nPreviousInstanceIndex = ivData.GetAt(0);
		for (nData = 2; nData < ivData.GetSize(); nData += 2)
		{
			bOk = bOk and nPreviousInstanceIndex < ivData.GetAt(nData);
			if (not bOk)
				break;
			nPreviousInstanceIndex = ivData.GetAt(nData);
		}
	}

	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetChunkPhysicalLayout
//
SNBDataTableBinarySliceSetChunkPhysicalLayout::SNBDataTableBinarySliceSetChunkPhysicalLayout()
{
	nChunkIndex = -1;
	layout = NULL;
}

SNBDataTableBinarySliceSetChunkPhysicalLayout::~SNBDataTableBinarySliceSetChunkPhysicalLayout() {}

void SNBDataTableBinarySliceSetChunkPhysicalLayout::PartiallyInitialize(
    int nChunk, const SNBDataTableBinarySliceSetLayout* someLayout)
{
	int nSlice;
	int nAttribute;
	require(someLayout != NULL);
	require(someLayout->IsInitialized());
	require(someLayout->Check());
	require(0 <= nChunk and nChunk < someLayout->GetChunkNumber());

	// Parametrage de l'index du chunk et du layout
	nChunkIndex = nChunk;
	layout = someLayout;

	// Initialization partielle des tableaux (bonne taille mais avec des indices invalides)
	lvBlockSizes.SetSize(layout->GetSliceNumber());
	lvBlockOffsets.SetSize(layout->GetSliceNumber());
	for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
	{
		lvBlockSizes.SetAt(nSlice, -1);
		lvBlockOffsets.SetAt(nSlice, -1);
	}
	ivAttributeDataSizes.SetSize(layout->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < layout->GetAttributeNumber(); nAttribute++)
		ivAttributeDataSizes.SetAt(nAttribute, -1);

	ensure(IsPartiallyInitialized());
}

boolean SNBDataTableBinarySliceSetChunkPhysicalLayout::IsPartiallyInitialized() const
{
	return nChunkIndex >= 0 and layout != NULL;
}

const SNBDataTableBinarySliceSetLayout* SNBDataTableBinarySliceSetChunkPhysicalLayout::GetLayout() const
{
	return layout;
}

int SNBDataTableBinarySliceSetChunkPhysicalLayout::GetChunkIndex() const
{
	return nChunkIndex;
}

longint SNBDataTableBinarySliceSetChunkPhysicalLayout::GetBlockSizeAt(int nSlice) const
{
	require(IsInitialized());
	require(0 <= nSlice and nSlice < layout->GetSliceNumber());
	return lvBlockSizes.GetAt(nSlice);
}

longint SNBDataTableBinarySliceSetChunkPhysicalLayout::GetBlockOffsetAt(int nSlice) const
{
	require(IsInitialized());
	require(0 <= nSlice and nSlice < layout->GetSliceNumber());
	return lvBlockOffsets.GetAt(nSlice);
}

void SNBDataTableBinarySliceSetChunkPhysicalLayout::SetAttributeDataSizeAt(int nAttribute, int nAttributeDataSize)
{
	require(IsPartiallyInitialized());
	require(nAttributeDataSize >= 0);
	require(0 <= nAttribute and nAttribute < layout->GetAttributeNumber());
	ivAttributeDataSizes.SetAt(nAttribute, nAttributeDataSize);
}

int SNBDataTableBinarySliceSetChunkPhysicalLayout::GetAttributeDataSizeAt(int nAttribute) const
{
	require(IsInitialized());
	require(0 <= nAttribute and nAttribute < layout->GetAttributeNumber());
	return ivAttributeDataSizes.GetAt(nAttribute);
}

longint SNBDataTableBinarySliceSetChunkPhysicalLayout::GetDataSize() const
{
	longint lSize;
	int nAttribute;

	require(IsInitialized());

	lSize = 0;
	for (nAttribute = 0; nAttribute < layout->GetAttributeNumber(); nAttribute++)
		lSize += GetAttributeDataSizeAt(nAttribute);

	return lSize;
}

void SNBDataTableBinarySliceSetChunkPhysicalLayout::FinishInitialization()
{
	longint lBlockOffset;
	int nSlice;
	longint lBlockSize;
	longint lAttributeDataRelativeOffset;
	int nSliceAttribute;
	int nAttribute;

	require(IsPartiallyInitialized());

	// En ajoutant les tailles de donnees de chaque attribute on calcule
	//  - La taille des blocs
	//  - Les offsets des blocs
	//  - Les offsets relatifs a la slices des offsets de donnees
	// Toutes ces quantites se mesurent en nombre de `int`s
	lBlockOffset = 0;
	for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
	{
		lBlockSize = 0;
		lAttributeDataRelativeOffset = 0;
		for (nSliceAttribute = 0; nSliceAttribute < layout->GetAttributeNumberAtSlice(nSlice);
		     nSliceAttribute++)
		{
			nAttribute = layout->GetAttributeOffsetAtSlice(nSlice) + nSliceAttribute;
			lBlockSize += (longint)ivAttributeDataSizes.GetAt(nAttribute);
			lAttributeDataRelativeOffset += (longint)ivAttributeDataSizes.GetAt(nAttribute);
		}
		lvBlockSizes.SetAt(nSlice, lBlockSize);
		lvBlockOffsets.SetAt(nSlice, lBlockOffset);
		lBlockOffset += lBlockSize;
	}
	ensure(IsInitialized());
}

boolean SNBDataTableBinarySliceSetChunkPhysicalLayout::IsInitialized() const
{
	boolean bIsInitialized = true;
	int nSlice;
	int nAttribute;

	// Verification du layout
	bIsInitialized = bIsInitialized and layout != NULL;
	bIsInitialized = bIsInitialized and layout->IsInitialized();

	// Verfications que les tailles et offsets de blocs ne sont pas invalides
	for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
	{
		bIsInitialized = bIsInitialized and lvBlockSizes.GetAt(nSlice) > 0;
		bIsInitialized = bIsInitialized and lvBlockOffsets.GetAt(nSlice) >= 0;
	}

	// Verification que les tailles et offsets des donneees d'attributs ne sont pas invalides
	for (nAttribute = 0; nAttribute < layout->GetAttributeNumber(); nAttribute++)
		bIsInitialized = bIsInitialized and ivAttributeDataSizes.GetAt(nAttribute) > 0;

	return bIsInitialized;
}

void SNBDataTableBinarySliceSetChunkPhysicalLayout::Write(ostream& ost) const
{
	int nSlice;
	int nAttribute;

	ost << "Physical Layout";
	if (not IsPartiallyInitialized())
		ost << " not initialized\n";
	else
	{
		ost << "Chunk #" << nChunkIndex << "\n";
		ost << "Slice Number: " << layout->GetSliceNumber() << "\n";

		if (IsInitialized())
		{
			ost << "Block Sizes: " << lvBlockSizes.GetAt(0);
			for (nSlice = 1; nSlice < layout->GetSliceNumber(); nSlice++)
				ost << ", " << lvBlockSizes.GetAt(nSlice);
			ost << "\n";
			ost << "Block Offsets: " << lvBlockOffsets.GetAt(0);
			for (nSlice = 1; nSlice < layout->GetSliceNumber(); nSlice++)
				ost << ", " << lvBlockOffsets.GetAt(nSlice);
			ost << "\n";
		}

		ost << "Attribute Number: " << layout->GetAttributeNumber() << "\n";
		if (IsInitialized())
		{
			ost << "Attribute Data Sizes: " << ivAttributeDataSizes.GetAt(0);
			for (nAttribute = 1; nAttribute < layout->GetAttributeNumber(); nAttribute++)
				ost << ", " << ivAttributeDataSizes.GetAt(nAttribute);
			ost << "\n";
		}
	}
	cout << "\n";
}

boolean SNBDataTableBinarySliceSetChunkPhysicalLayout::Check() const
{
	boolean bOk = true;
	longint lCumulatedBlockSize;
	longint lCumulatedAttributeDataSize;
	int nSlice;
	int nAttribute;

	// Verifications d'integrite lors d'une instance partiellement initialisee
	if (IsPartiallyInitialized())
	{
		bOk = bOk and layout != NULL;
		bOk = bOk and layout->IsInitialized();
		bOk = bOk and layout->Check();
		bOk = bOk and (0 <= nChunkIndex and nChunkIndex < layout->GetChunkNumber());
		bOk = bOk and (lvBlockSizes.GetSize() == layout->GetSliceNumber());
		bOk = bOk and (lvBlockOffsets.GetSize() == layout->GetSliceNumber());
		bOk = bOk and (ivAttributeDataSizes.GetSize() == layout->GetAttributeNumber());
	}

	// Verifications d'integrite lors d'une instance intialisee
	if (IsInitialized())
	{
		// Verification de la quadrature des tailles des blocs et attributs avec les offsets des blocs
		lCumulatedBlockSize = 0;
		for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
		{
			lCumulatedAttributeDataSize = 0;
			for (nAttribute = layout->GetAttributeOffsetAtSlice(nSlice);
			     nAttribute <
			     layout->GetAttributeOffsetAtSlice(nSlice) + layout->GetAttributeNumberAtSlice(nSlice);
			     nAttribute++)
			{
				lCumulatedAttributeDataSize += (longint)ivAttributeDataSizes.GetAt(nAttribute);
			}

			bOk = bOk and (lCumulatedBlockSize == lvBlockOffsets.GetAt(nSlice));
			bOk = bOk and (lCumulatedAttributeDataSize == lvBlockSizes.GetAt(nSlice));
			lCumulatedBlockSize += lvBlockSizes.GetAt(nSlice);
		}
	}
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSetChunkBuffer

SNBDataTableBinarySliceSetChunkBuffer::SNBDataTableBinarySliceSetChunkBuffer()
{
	nChunkIndex = -1;
	layout = NULL;
	// TODO FOR FELIPE: a supprimer
	// buffer = NewIntArray(nIntBufferSize);
	fChunkDataFile = NULL;
	nLoadedBlockSliceIndex = -1;
}

SNBDataTableBinarySliceSetChunkBuffer::~SNBDataTableBinarySliceSetChunkBuffer()
{
	CleanWorkingData();
	// TODO FOR FELIPE: a supprimer
	// DeleteIntArray(buffer);
}

void SNBDataTableBinarySliceSetChunkBuffer::SetLayout(const SNBDataTableBinarySliceSetLayout* someLayout)
{
	require(someLayout != NULL);
	require(someLayout->IsInitialized());
	require(someLayout->Check());

	CleanWorkingData();
	layout = someLayout;
}

const SNBDataTableBinarySliceSetLayout* SNBDataTableBinarySliceSetChunkBuffer::GetLayout() const
{
	return layout;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::Initialize(int nChunk, KWClass* recoderClass,
							  KWDataTableSliceSet* sliceSet,
							  const SNBDataTableBinarySliceSetSchema* schema)
{
	boolean bOk = true;

	require(layout != NULL);
	require(layout->Check());
	require(0 <= nChunk and nChunk < layout->GetChunkNumber());
	require(recoderClass->Check());
	require(sliceSet->Check());
	require(schema != NULL);
	require(schema->Check());
	require(schema->GetAttributeNumber() == layout->GetAttributeNumber());

	// Annulation d'une eventuelle initialisation existante
	CleanWorkingData();

	// Parametrage de l'index du chunk
	nChunkIndex = nChunk;

	// Initialization partielle du layout physique
	// Elle continue a s'initialiser lors des appels a la methode InitializeBlockFromSliceSetAt
	physicalLayout.PartiallyInitialize(nChunkIndex, layout);

	// Initialization des objets colonnes
	InitializeColumns();

	// Initialisation des donnees
	if (bOk)
	{
		// Cas d'un seule slice: Un monte le seul bloc on memoire sans creer un fichier
		if (layout->GetSliceNumber() == 1)
			bOk = InitializeBlockFromSliceSetAt(0, recoderClass, sliceSet, schema);
		// Cas de plusieurs slices: On initialise le fichier de chunk
		else
			bOk = InitializeDataFileFromSliceSetAt(recoderClass, sliceSet, schema);
	}

	// Finalisation de l'initialisation du layout physique
	physicalLayout.FinishInitialization();

	ensure(not bOk or IsInitialized());
	ensure(not bOk or Check());
	return bOk;
}

int SNBDataTableBinarySliceSetChunkBuffer::GetChunkIndex() const
{
	require(IsInitialized());
	return nChunkIndex;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::InitializeColumns()
{
	boolean bOk = true;
	int nAttribute;
	SNBDataTableBinarySliceSetColumn* column;

	// Initialisation du tableau du bloc des donnees chargees
	oaLoadedBlock.SetSize(layout->GetMaxSliceAttributeNumber());
	for (nAttribute = 0; nAttribute < oaLoadedBlock.GetSize(); nAttribute++)
	{
		column = new SNBDataTableBinarySliceSetColumn;
		oaLoadedBlock.SetAt(nAttribute, column);
	}
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::InitializeDataFileFromSliceSetAt(
    KWClass* recoderClass, KWDataTableSliceSet* sliceSet, const SNBDataTableBinarySliceSetSchema* schema)
{
	boolean bOk = true;
	int nChunkInstanceNumber;
	int nSlice;
	int nSliceAttributeNumber;
	int nBuffer;
	int nData;
	int nAttribute;
	SNBDataTableBinarySliceSetColumn* column;
	KWLoadIndexVector livLoadedRecodedAttributeIndexes;
	longint lValueNumber;
	int nRequestedBufferSize;
	int* buffer;
	int nIntBufferSize;

	// Creation, indexation, verification de la taille et ouverture du fichier du chunk
	sChunkFilePath = CreateTempFileForChunk(nChunkIndex);
	bOk = bOk and OpenOutputDataFile();

	// Ecriture des indices des blocs correspondant au chunk
	if (bOk)
	{
		// TODO FOR FELIPE: a optimiser en fonction de la taille utile ici, et a adapter aux donnees sparse
		//  Nombre total de valeurs du binary slice set
		lValueNumber = layout->GetInstanceNumber() * (longint)layout->GetAttributeNumber();
		nRequestedBufferSize = max((int)(8 * lMB), GetHugeBufferSize());
		if (lValueNumber * sizeof(int) < nRequestedBufferSize)
			nRequestedBufferSize = int(lValueNumber * sizeof(int));
		assert(nRequestedBufferSize % sizeof(int) == 0);

		// TODO FELIPE
		//  Demande d'un buffer de grande taille
		buffer = (int*)GetHugeBuffer(nRequestedBufferSize);
		nIntBufferSize = nRequestedBufferSize / sizeof(int);

		// Boucle sur les slices pour ecriture du fichier de chunk
		Global::ActivateErrorFlowControl();
		nChunkInstanceNumber = layout->GetInstanceNumberAtChunk(nChunkIndex);
		for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
		{
			// Chargmement en memoire de la slice du binary slice set depuis le slice set
			bOk = bOk and InitializeBlockFromSliceSetAt(nSlice, recoderClass, sliceSet, schema);

			// Ecriture colonnaire (transposee) du bloc vers le fichier binaire du chunk
			if (bOk)
			{
				nBuffer = 0;
				nSliceAttributeNumber = layout->GetAttributeNumberAtSlice(nSlice);
				for (nAttribute = 0; nAttribute < nSliceAttributeNumber; nAttribute++)
				{
					column =
					    cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nAttribute));
					for (nData = 0; nData < column->GetDataSize(); nData++)
					{
						buffer[nBuffer] = column->GetDataAt(nData);
						nBuffer = (nBuffer + 1) % nIntBufferSize;
						if (nBuffer == 0)
						{
							bOk = bOk and WriteToDataFile(buffer, nIntBufferSize);
							if (not bOk)
							{
								AddError("Error when writing to binary data file " +
									 sChunkFilePath);
								break;
							}
						}
					}
					if (not bOk)
						break;
				}

				// Ecriture du reste du buffer s'il reste de donnees dedans
				if (bOk and nBuffer > 0)
				{
					bOk = bOk and WriteToDataFile(buffer, nBuffer);
					if (not bOk)
						AddError("Error when finishing writing to binary data file " +
							 sChunkFilePath);
				}

				if (not bOk)
					break;
			}
			else
				break;
		}
		Global::DesactivateErrorFlowControl();

		// Cloture du fichier quoiqu'il arrive
		bOk = CloseOutputDataFile() and bOk;
	}
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::InitializeBlockFromSliceSetAt(
    int nSlice, KWClass* recoderClass, KWDataTableSliceSet* sliceSet, const SNBDataTableBinarySliceSetSchema* schema)
{
	boolean bOk = true;
	KWLoadIndexVector livLoadedRecodedAttributeIndexes;
	int nSliceAttributeNumber;
	int nSliceAttributeOffset;
	int nChunkInstanceNumber;
	int nSliceNumber;
	double dProgressPercent;
	double dProgressPercentSlice;
	int nChunkInstance;
	KWObject* kwoPartIndices;
	int nSliceAttribute;
	int nAttribute;
	SNBDataTableBinarySliceSetColumn* column;
	Continuous cIndex;

	require(physicalLayout.IsPartiallyInitialized());

	// Chargement des attributs de la slice dans le dictionnaire de recodage
	LoadRecodedAttributesAtSlice(nSlice, recoderClass, schema, &livLoadedRecodedAttributeIndexes);

	// Alias locaux des bornes max (pour ne pas avoir de soucis de perf dans cette boucle critique)
	nSliceAttributeNumber = layout->GetAttributeNumberAtSlice(nSlice);
	nSliceAttributeOffset = layout->GetAttributeOffsetAtSlice(nSlice);
	nChunkInstanceNumber = layout->GetInstanceNumberAtChunk(nChunkIndex);
	nSliceNumber = layout->GetSliceNumber();

	// Lecture des records du bloc depuis le slice set
	Global::ActivateErrorFlowControl();
	dProgressPercent = 0.0;
	dProgressPercentSlice = 0.0;
	sliceSet->SetReadClass(recoderClass);
	sliceSet->OpenForRead();
	sliceSet->SkipMultiple(layout->GetInstanceOffsetAtChunk(nChunkIndex));

	// Initialization des colonnes et son layout physique pour cette tranche
	//   - Cas dense : Allocation immediate de la memoire necessaire et saisie du layout physique
	//   - Cas sparse : L'allocation se fait au fil de l'eau car on ne connait pas sa taille a priori et
	//                  le parametrage du layout physique se fait en fin de methode quand on a les infos
	for (nSliceAttribute = 0; nSliceAttribute < nSliceAttributeNumber; nSliceAttribute++)
	{
		nAttribute = nSliceAttributeOffset + nSliceAttribute;
		column = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nSliceAttribute));
		column->SetSparse(layout->IsAttributeSparseAt(nAttribute));
		if (column->IsSparse())
			column->SetDataSize(0);
		else
		{
			physicalLayout.SetAttributeDataSizeAt(nAttribute,
							      layout->GetInstanceNumberAtChunk(nChunkIndex));
			bOk = bOk and column->SetDataSize(layout->GetInstanceNumberAtChunk(nChunkIndex));
			if (not bOk)
			{
				AddError("Could not allocate block vector for column data");
				break;
			}
		}
		if (bOk)
			oaLoadedBlock.SetAt(nSliceAttribute, column);
	}

	// Lecture des attributs de la slice courante depuis le KWDataTableSliceSet en entree
	if (bOk)
	{
		for (nChunkInstance = 0; nChunkInstance < nChunkInstanceNumber; nChunkInstance++)
		{
			// Lecture de l'objet
			kwoPartIndices = sliceSet->Read();
			bOk = bOk and (kwoPartIndices != NULL);
			if (not bOk)
				break;

			// Trasposition des donnes de l'objet dans aux colonne du bloc
			for (nSliceAttribute = 0; nSliceAttribute < nSliceAttributeNumber; nSliceAttribute++)
			{
				nAttribute = nSliceAttributeOffset + nSliceAttribute;
				column = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nSliceAttribute));
				cIndex = kwoPartIndices->GetContinuousValueAt(
				    livLoadedRecodedAttributeIndexes.GetAt(nSliceAttribute));
				if (column->IsSparse())
				{
					column->AddData(nChunkInstance);
					column->AddData(lrint(cIndex) - 1);
				}
				else
					column->SetDataAt(nChunkInstance, lrint(cIndex) - 1);
			}
			delete kwoPartIndices;

			// Suivi de la tache
			// Ici est le seul endroit de *cette classe* qui change l'etat du TaskProgression.
			// Il y a un leger retard de sa MAJ quand le schema a plusieurs slices et on utilise un fichier de chunk.
			// Moralement, l'ecriture du fichier devrait etre aussi suivi mais cela entrainarait utiliser
			// des etats en plus, des calculs plus tordus et une perte de localite de cette partie du code.
			// La lecture du bloc depuis le fichier binaire est tres rapide et n'a pas besoin de suivi.
			if (TaskProgression::IsInTask() and TaskProgression::IsRefreshNecessary())
			{
				// Mise-a-jour de la barre d'avancement
				dProgressPercentSlice = 100.0 * (nChunkInstance + 1) / nChunkInstanceNumber;
				if (layout->GetSliceNumber() == 1)
					dProgressPercent = dProgressPercentSlice;
				else
					dProgressPercent = 100.0 * nSlice / nSliceNumber +
							   dProgressPercentSlice * (1.0 / nSliceNumber);
				TaskProgression::DisplayProgression(int(dProgressPercent));

				// Arret si il y a une interruption utilisateur
				bOk = bOk and not TaskProgression::IsInterruptionRequested();
				if (not bOk)
					break;
			}
		}
	}
	Global::DesactivateErrorFlowControl();

	// Fermeture du slice set quoi qu'il arrive
	bOk = sliceSet->Close() and bOk;

	// Mise-a-jour du layout physique pour les variables sparse de cette slice
	if (bOk)
	{
		for (nSliceAttribute = 0; nSliceAttribute < nSliceAttributeNumber; nSliceAttribute++)
		{
			nAttribute = nSliceAttributeOffset + nSliceAttribute;
			column = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nSliceAttribute));
			if (column->IsSparse())
				physicalLayout.SetAttributeDataSizeAt(nAttribute, column->GetDataSize());
		}
	}

	// Si succes : mise a jour des indices du bloc charge
	if (bOk)
		nLoadedBlockSliceIndex = nSlice;
	// Si echec : mise a un etat invalide, pas message d'erreur
	// Rationale :
	//  - Les messages d'erreur de bas niveau se font au niveau du KWDataTableSliceSet
	//  - Les messages du niveau utilisateur se font au niveau du SNBSelectiveNaiveBayesTrainingTask
	else
		nLoadedBlockSliceIndex = -1;

	return bOk;
}

const ALString SNBDataTableBinarySliceSetChunkBuffer::CreateTempFileForChunk(int nChunk) const
{
	ALString sFileName;

	require(0 <= nChunk and nChunk < layout->GetChunkNumber());

	sFileName = sFileName + "SNBDataTableBinarySliceSet_Chunk" + IntToString(nChunk) + ".bin";

	return FileService::CreateUniqueTmpFile(sFileName, this);
}

boolean SNBDataTableBinarySliceSetChunkBuffer::OpenOutputDataFile()
{
	boolean bOk;

	require(fChunkDataFile == NULL);
	require(sChunkFilePath != "");

	bOk = FileService::OpenOutputBinaryFile(sChunkFilePath, fChunkDataFile);
	if (not bOk)
		fChunkDataFile = NULL;

	ensure(not bOk or fChunkDataFile != NULL);
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::WriteToDataFile(int* writeBuffer, int nIntNumber) const
{
	boolean bOk = true;
	int nWrittenIntNumber;

	require(writeBuffer != NULL);
	require(nIntNumber >= 0);
	require(fChunkDataFile != NULL);

	nWrittenIntNumber = (int)fwrite(writeBuffer, sizeof(int), nIntNumber, fChunkDataFile);
	bOk = (nWrittenIntNumber == nIntNumber);

	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::CloseOutputDataFile()
{
	boolean bOk;

	require(fChunkDataFile != NULL);
	require(sChunkFilePath != "");

	bOk = FileService::CloseOutputBinaryFile(sChunkFilePath, fChunkDataFile);
	fChunkDataFile = NULL;

	return bOk;
}

void SNBDataTableBinarySliceSetChunkBuffer::LoadRecodedAttributesAtSlice(
    int nSlice, KWClass* recoderClass, const SNBDataTableBinarySliceSetSchema* schema,
    KWLoadIndexVector* livLoadedRecodedAttributeIndexes) const
{
	int nSliceAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWAttribute* recodedAttribute;
	int nLoadedAttribute;

	require(recoderClass->Check());
	require(livLoadedRecodedAttributeIndexes != NULL);

	// Dechargements de tous les attributs et deactivation de la cible
	recoderClass->SetAllAttributesLoaded(false);

	// Chargement dans la classe de recodage des attributs de la slice
	for (nSliceAttribute = 0; nSliceAttribute < layout->GetAttributeNumberAtSlice(nSlice); nSliceAttribute++)
	{
		attribute = schema->GetAttributeAt(layout->GetAttributeOffsetAtSlice(nSlice) + nSliceAttribute);
		recodedAttribute = recoderClass->LookupAttribute(attribute->GetRecodedAttributeName());
		assert(recodedAttribute != NULL);
		recodedAttribute->SetLoaded(true);
	}
	recoderClass->GetDomain()->Compile();

	// Memorisation des load index des attributs charges
	livLoadedRecodedAttributeIndexes->SetSize(recoderClass->GetLoadedAttributeNumber());
	for (nLoadedAttribute = 0; nLoadedAttribute < recoderClass->GetLoadedAttributeNumber(); nLoadedAttribute++)
	{
		recodedAttribute = recoderClass->GetLoadedAttributeAt(nLoadedAttribute);
		livLoadedRecodedAttributeIndexes->SetAt(nLoadedAttribute, recodedAttribute->GetLoadIndex());
	}
	ensure(recoderClass->GetLoadedAttributeNumber() == layout->GetAttributeNumberAtSlice(nSlice));
	ensure(livLoadedRecodedAttributeIndexes->GetSize() == layout->GetAttributeNumberAtSlice(nSlice));
	ensure(recoderClass->IsCompiled());
	ensure(recoderClass->Check());
}

boolean SNBDataTableBinarySliceSetChunkBuffer::IsInitialized() const
{
	boolean bOk = true;

	bOk = bOk and layout != NULL;
	if (bOk)
	{
		bOk = bOk and layout->IsInitialized();
		if (layout->GetSliceNumber() > 1)
			bOk = bOk and sChunkFilePath != "";
		bOk = bOk and physicalLayout.IsInitialized();
	}

	return bOk;
}

void SNBDataTableBinarySliceSetChunkBuffer::CleanWorkingData()
{
	// Elimination du fichier de chunk s'il existe
	if (sChunkFilePath != "")
	{
		FileService::RemoveFile(sChunkFilePath);
		sChunkFilePath = "";
	}

	// Nettoyage des objets de travail
	nChunkIndex = -1;
	oaLoadedBlock.DeleteAll();
	nLoadedBlockSliceIndex = -1;
}

boolean
SNBDataTableBinarySliceSetChunkBuffer::CollectRecodedAttributeIndexes(int nAttribute,
								      SNBDataTableBinarySliceSetColumn*& outputColumn)
{
	boolean bOk = true;
	int nAttributeSliceIndex;
	int nAttributeRelativeIndex;

	require(IsInitialized());
	require(0 <= nAttribute and nAttribute < layout->GetAttributeNumber());

	// Chargement du bloc depuis le disque si besoin
	nAttributeSliceIndex = layout->GetSliceIndexAtAttribute(nAttribute);
	if (nAttributeSliceIndex != nLoadedBlockSliceIndex)
		bOk = LoadBlockAt(nAttributeSliceIndex);

	// On pointe le pointeur de sortie vers le vecteur d'indices de l'attribut
	if (bOk)
	{
		nAttributeRelativeIndex = layout->GetRelativeIndexAtAttribute(nAttribute);
		outputColumn = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nAttributeRelativeIndex));
	}
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::LoadBlockAt(int nSlice)
{
	boolean bOk = true;
	int nColumnData;
	int nAttribute;
	int nRequestedBufferSize;
	int* buffer;
	int nIntBufferSize;
	int nSliceAttribute;
	SNBDataTableBinarySliceSetColumn* column;
	longint lRemainingAttributeIntNumber;
	longint lRemainingIndexNumber;
	longint lReadIndexNumber;
	int nBufferIndex;
	int nToReadIndexNumber;

	require(IsInitialized());
	require(fChunkDataFile == NULL);
	require(physicalLayout.IsInitialized());
	require(0 <= nSlice and nSlice < layout->GetSliceNumber());

	// Verification d'integrite basique du fichier
	bOk = bOk and CheckChunkFile();

	// Ouverture du fichier et positionement au debut du bloc
	bOk = bOk and FileService::OpenInputBinaryFile(sChunkFilePath, fChunkDataFile);
	bOk = bOk and FileService::SeekPositionInBinaryFile(fChunkDataFile,
							    physicalLayout.GetBlockOffsetAt(nSlice) * sizeof(int));

	// Reinitialisation des colonnes du bloc
	if (bOk)
	{
		for (nSliceAttribute = 0; nSliceAttribute < layout->GetAttributeNumberAtSlice(nSlice);
		     nSliceAttribute++)
		{
			nAttribute = layout->GetAttributeOffsetAtSlice(nSlice) + nSliceAttribute;
			column = cast(SNBDataTableBinarySliceSetColumn*,
				      oaLoadedBlock.GetAt(layout->GetRelativeIndexAtAttribute(nAttribute)));
			column->SetSparse(layout->IsAttributeSparseAt(nAttribute));
			column->SetDataSize(physicalLayout.GetAttributeDataSizeAt(nAttribute));
			bOk = column->SetDataSize(physicalLayout.GetAttributeDataSizeAt(nAttribute));
			if (not bOk)
				break;
		}
	}

	// Remplisement des colonnes depuis le fichier de chunk
	if (bOk)
	{
		// Calcul de la taille du buffer de lecture
		nRequestedBufferSize = max((int)(8 * lMB), GetHugeBufferSize());
		if (physicalLayout.GetDataSize() * sizeof(int) < nRequestedBufferSize)
			nRequestedBufferSize = int(physicalLayout.GetDataSize() * sizeof(int));
		assert(nRequestedBufferSize % sizeof(int) == 0);

		// Demande d'un buffer de lecture de grande taille
		buffer = (int*)GetHugeBuffer(nRequestedBufferSize);
		nIntBufferSize = nRequestedBufferSize / sizeof(int);

		// Lecture par colonne du bloc defini par le chunk et la slice
		nAttribute = layout->GetAttributeOffsetAtSlice(nSlice);
		nSliceAttribute = 0;
		nColumnData = 0;
		lRemainingAttributeIntNumber =
		    physicalLayout.GetAttributeDataSizeAt(layout->GetAttributeOffsetAtSlice(nSlice));
		column = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(0));
		lRemainingIndexNumber = physicalLayout.GetBlockSizeAt(nSlice);
		while (lRemainingIndexNumber > 0 and not ferror(fChunkDataFile) and not feof(fChunkDataFile))
		{
			// Lecture selon la taille necessaire, selon que l'on soit arrive ou non a la fin de la slice
			if (lRemainingIndexNumber >= nIntBufferSize)
				nToReadIndexNumber = nIntBufferSize;
			else
				nToReadIndexNumber = (int)lRemainingIndexNumber;
			lReadIndexNumber = (longint)fread(buffer, sizeof(int), nToReadIndexNumber, fChunkDataFile);
			lRemainingIndexNumber -= lReadIndexNumber;
			assert(lReadIndexNumber == (longint)nToReadIndexNumber);

			// Transfer des indexes vers les colonnes du buffer
			for (nBufferIndex = 0; nBufferIndex < lReadIndexNumber; nBufferIndex++)
			{
				// Si pas de donnees restants pour la colonne: Initialisation de la suivante
				if (lRemainingAttributeIntNumber == 0)
				{
					nAttribute++;
					nSliceAttribute++;
					nColumnData = 0;
					lRemainingAttributeIntNumber =
					    physicalLayout.GetAttributeDataSizeAt(nAttribute);
					column = cast(SNBDataTableBinarySliceSetColumn*,
						      oaLoadedBlock.GetAt(nSliceAttribute));
				}

				// Transfer de la prochaine donnee vers sa colonne
				column->SetDataAt(nColumnData, buffer[nBufferIndex]);
				lRemainingAttributeIntNumber--;
				nColumnData++;
			}
		}
		// Verification que on a lu le nombre exacte d'indices & qu'il n'y pas eu d'erreur de lecture
		bOk = bOk and lRemainingAttributeIntNumber == 0;
		bOk = bOk and lRemainingIndexNumber == 0;
		bOk = bOk and not ferror(fChunkDataFile);
		if (not bOk and not ferror(fChunkDataFile))
			AddError("Corrupted binary data file " + sChunkFilePath);
		else if (ferror(fChunkDataFile))
			AddError("IO error when reading binary data file " + sChunkFilePath);

		// Cloture du fichier quoiqu'il arrive
		bOk = FileService::CloseInputBinaryFile(sChunkFilePath, fChunkDataFile) and bOk;
		fChunkDataFile = NULL;
	}

	// Si lecture OK : Mise a jour de l'index de la slice du bloc en memoire
	if (bOk)
		nLoadedBlockSliceIndex = nSlice;
	// Si lecture KO : mise a un etat invalide
	else
		nLoadedBlockSliceIndex = -1;

	ensure(fChunkDataFile == NULL);
	ensure(Check());
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::Check() const
{
	boolean bOk = true;
	int nAttribute;
	SNBDataTableBinarySliceSetColumn* column;

	// Verification du layout
	bOk = bOk and layout != NULL;
	bOk = bOk and layout->Check();
	bOk = bOk and physicalLayout.Check();

	// Verification du bloc charge en memoire
	bOk = bOk and 0 <= nLoadedBlockSliceIndex and nLoadedBlockSliceIndex < layout->GetSliceNumber();
	if (bOk)
	{
		bOk = bOk and oaLoadedBlock.GetSize() == layout->GetMaxSliceAttributeNumber();
		for (nAttribute = 0; nAttribute < oaLoadedBlock.GetSize(); nAttribute++)
		{
			column = cast(SNBDataTableBinarySliceSetColumn*, oaLoadedBlock.GetAt(nAttribute));
			if (not column->IsSparse())
				bOk = bOk and column->GetDataSize() == layout->GetInstanceNumberAtChunk(nChunkIndex);
			bOk = bOk and column->Check();
		}
	}

	// Verification des fichiers de chunk
	if (bOk)
	{
		// Cas de slice unique: Aucun fichie doit etre initialize
		if (layout->GetSliceNumber() == 1)
		{
			bOk = bOk and nLoadedBlockSliceIndex == 0;
			bOk = bOk and sChunkFilePath == "";
		}
		// Cas de plusieurs slices: Verification du fichier de chunk
		else
		{
			bOk = bOk and sChunkFilePath != "";
			bOk = bOk and CheckChunkFile();
		}
	}
	return bOk;
}

boolean SNBDataTableBinarySliceSetChunkBuffer::CheckChunkFile() const
{
	boolean bOk = true;
	longint lDataFileSize;
	longint lExpectedDataFileSize;
	int nSlice;
	ALString sTmp;

	if (bOk)
	{
		bOk = bOk and (sChunkFilePath != "");
		if (not bOk)
			AddError(sTmp + "Chunk no." + IntToString(nChunkIndex) + " has no associated chunk file path");
	}

	if (bOk)
	{
		lDataFileSize = FileService::GetFileSize(sChunkFilePath);
		lExpectedDataFileSize = 0;
		for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
			lExpectedDataFileSize += sizeof(int) * physicalLayout.GetBlockSizeAt(nSlice);

		bOk = bOk and (lDataFileSize == lExpectedDataFileSize);
		if (not bOk)

			AddError("Data file " + sChunkFilePath + " has size " + LongintToString(lDataFileSize) +
				 " bytes " + "instead of the expected " + LongintToString(lExpectedDataFileSize) +
				 " bytes");
	}
	return bOk;
}

const ALString SNBDataTableBinarySliceSetChunkBuffer::GetClassLabel() const
{
	return "Data Table Binary Slice Set Buffer";
}

void SNBDataTableBinarySliceSetChunkBuffer::WriteContentsAsTSV(ostream& ost,
							       const SNBDataTableBinarySliceSetSchema* schema)
{
	const int nMaxInstanceNumber = 200;
	int nDisplayedInstanceNumber;
	boolean bOk = true;
	int nSlice;
	int nInstance;
	int nAttribute;
	int nChunkInstance;
	SNBDataTableBinarySliceSetColumn* column;
	IntVector ivCurrentValueIndexes;

	if (layout == NULL)
		ost << "No layout defined\n";
	else
	{
		if (IsInitialized())
		{
			// Entete du bloc
			ost << "Chunk #" << nChunkIndex << ": Initialized\n";
			if (sChunkFilePath != "")
				ost << "File: " << FileService::GetURIFilePathName(sChunkFilePath) << "\n";
			else
				ost << "File: None (no slicing)\n";
			ost << physicalLayout << "\n";
			ivCurrentValueIndexes.SetSize(layout->GetAttributeNumber());
			for (nAttribute = 0; nAttribute < layout->GetAttributeNumber(); nAttribute++)
				ivCurrentValueIndexes.SetAt(nAttribute, -1);
			for (nSlice = 0; nSlice < layout->GetSliceNumber(); nSlice++)
			{
				nInstance = layout->GetInstanceOffsetAtChunk(nChunkIndex);
				ost << "chunk\tslice\tind\tchind";
				for (nAttribute = layout->GetAttributeOffsetAtSlice(nSlice);
				     nAttribute < layout->GetAttributeOffsetAtSlice(nSlice) +
						      layout->GetAttributeNumberAtSlice(nSlice);
				     nAttribute++)
				{
					ost << "\t" << schema->GetAttributeAt(nAttribute)->GetNativeAttributeName();
					if (layout->IsAttributeSparseAt(nAttribute))
						ost << "[S]";
				}
				ost << "\n";

				// Ecriture des contenus du fichier
				nDisplayedInstanceNumber =
				    min(nMaxInstanceNumber, layout->GetInstanceNumberAtChunk(nChunkIndex));
				for (nChunkInstance = 0; nChunkInstance < nDisplayedInstanceNumber; nChunkInstance++)
				{
					ost << nChunkIndex << "\t" << nSlice << "\t" << nInstance << "\t"
					    << nChunkInstance;
					for (nAttribute = layout->GetAttributeOffsetAtSlice(nSlice);
					     nAttribute < layout->GetAttributeOffsetAtSlice(nSlice) +
							      layout->GetAttributeNumberAtSlice(nSlice);
					     nAttribute++)
					{
						bOk = CollectRecodedAttributeIndexes(nAttribute, column);
						if (column->IsSparse())
						{
							ost << "\t";
							if (ivCurrentValueIndexes.GetAt(nAttribute) == -1)
								ivCurrentValueIndexes.SetAt(nAttribute, 0);

							else if (ivCurrentValueIndexes.GetAt(nAttribute) ==
								 column->GetValueNumber())
								continue;

							if (column->GetSparseValueInstanceIndexAt(
								ivCurrentValueIndexes.GetAt(nAttribute)) ==
							    nChunkInstance)
							{
								ost << column->GetSparseValueAt(
								    ivCurrentValueIndexes.GetAt(nAttribute));
								ivCurrentValueIndexes.UpgradeAt(nAttribute, 1);
							}
						}
						else
						{
							ivCurrentValueIndexes.SetAt(nAttribute, nChunkInstance);
							ost << "\t" << column->GetDenseValueAt(nChunkInstance);
						}
					}
					ost << endl;
					nInstance++;
				}
			}
		}
		else
			ost << ": Not initialized\n";
	}
}

longint SNBDataTableBinarySliceSetChunkBuffer::ComputeNecessaryMemory(int nInstanceNumber, int nChunkNumber,
								      int nAttributeNumber, int nSliceNumber)
{
	ObjectArray oaDummy;

	int nMaxInstancesPerChunk;
	int nMaxAttributesPerSlice;

	require(0 < nChunkNumber and nChunkNumber <= nInstanceNumber);
	require(0 < nSliceNumber and nSliceNumber <= nAttributeNumber);

	nMaxInstancesPerChunk = nInstanceNumber / nChunkNumber;
	nMaxAttributesPerSlice = nAttributeNumber / nSliceNumber;

	// Formule de l'estimation (sans dictionnaire de recodage) :
	//   Objets de la instance +
	//   FILE de lecture/ecriture (fChunkDataFile) +
	//   Tableau de MemSegmentByteSize int's (buffer) +
	//   Vecteur de nAttributes KWLoadIndex's (livLoadedRecodedAttributeIndexes dans InitializeDataFileFromSliceSetAt) +
	//   Tableau de nMaxAttributesPerSlice IntVectors de taille nMaxInstancesPerChunk (oaLoadedBlock)
	return sizeof(SNBDataTableBinarySliceSetChunkBuffer) + sizeof(FILE) +
	       (longint)nAttributeNumber * sizeof(KWLoadIndex) +
	       nMaxAttributesPerSlice * (oaDummy.GetUsedMemoryPerElement() + sizeof(IntVector) +
					 (longint)nMaxInstancesPerChunk * sizeof(int));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBDataTableBinarySliceSet

SNBDataTableBinarySliceSet::SNBDataTableBinarySliceSet()
{
	nInitialAttributeNumber = -1;
	bIsError = false;
	dataPreparationClass = NULL;
}

SNBDataTableBinarySliceSet::~SNBDataTableBinarySliceSet()
{
	CleanWorkingData(true);
}

void SNBDataTableBinarySliceSet::InitializeWithoutChunkBuffer(KWClassStats* classStats, int nChunkNumber,
							      int nMaxAttributes, int nSliceNumber)
{
	// Initialisation du schema d'attributs
	InitializeSchemaFromClassStats(classStats, nMaxAttributes);

	// S'il y a des attributs informatifs on pursuit l'initialisation
	if (HasUsableAttributes())
	{
		// Initialisation des indices des valeurs de la cible
		InitializeTargetValueIndexes(classStats);

		// Initialisation du layout
		layout.Initialize(dataPreparationClass->GetInstanceNumber(), nChunkNumber, schema.GetAttributeNumber(),
				  nSliceNumber);

		// Initialisation de l'iterateur randomise d'attributes
		randomizedAttributeIterator.Initialize(&schema, &layout);
	}
}

void SNBDataTableBinarySliceSet::InitializeSchemaFromClassStats(KWClassStats* classStats, int nMaxAttributes)
{
	int nEffectiveMaxAttributes;
	ObjectArray oaSortedAttributes;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWAttribute* recodedAttribute;
	ObjectArray oaUsedDataPreparationAttributes;
	StringVector svUsedDataPreparationAttributesRecodedNames;

	require(classStats != NULL);
	require(classStats->Check());
	require(0 <= nMaxAttributes);
	require(dataPreparationClass == NULL);

	// Annulation d'une eventuelle initialisation existante
	if (IsInitialized())
		CleanWorkingData(true);

	// Initialisation de la DataPreparationClass en mettant tous ses attributs en Unused
	dataPreparationClass = new KWDataPreparationClass;
	dataPreparationClass->SetLearningSpec(classStats->GetLearningSpec());
	dataPreparationClass->ComputeDataPreparationFromClassStats(classStats);
	dataPreparationClass->GetDataPreparationClass()->SetAllAttributesUsed(false);

	// Tri des attributs de la DataPreparationClass par informativite
	oaSortedAttributes.CopyFrom(dataPreparationClass->GetDataPreparationAttributes());
	oaSortedAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaSortedAttributes.Sort();

	// Calcul du nombre maximal effectif d'attributs pour l'apprentissage
	nInitialAttributeNumber = dataPreparationClass->GetDataPreparationAttributes()->GetSize();
	if (nMaxAttributes > 0)
		nEffectiveMaxAttributes = nMaxAttributes;
	else
		nEffectiveMaxAttributes = nInitialAttributeNumber;

	// Extraction des attributs a utiliser (predictifs + limite du nombre par l'utilisateur)
	// On ajoute egalement chaque attribut recode a la DataPreparationClass et on memorise ses noms
	for (nAttribute = 0; nAttribute < oaSortedAttributes.GetSize(); nAttribute++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaSortedAttributes.GetAt(nAttribute));

		if (IsPredictive(dataPreparationAttribute))
		{
			recodedAttribute = dataPreparationAttribute->AddPreparedIndexingAttribute();
			oaUsedDataPreparationAttributes.Add(dataPreparationAttribute);
			svUsedDataPreparationAttributesRecodedNames.Add(recodedAttribute->GetName());
		}

		if (oaUsedDataPreparationAttributes.GetSize() == nEffectiveMaxAttributes)
			break;
	}

	// Initialisation du schema
	schema.InitializeFromDataPreparationClass(dataPreparationClass, &oaUsedDataPreparationAttributes,
						  &svUsedDataPreparationAttributesRecodedNames);

	ensure(oaUsedDataPreparationAttributes.GetSize() == svUsedDataPreparationAttributesRecodedNames.GetSize());
	ensure(oaUsedDataPreparationAttributes.GetSize() <= nEffectiveMaxAttributes);
	ensure(dataPreparationClass != NULL);
	ensure(schema.IsInitialized() or not HasUsableAttributes());
	ensure(nInitialAttributeNumber > 0 or not HasUsableAttributes());
	ensure(Check());
}

boolean SNBDataTableBinarySliceSet::InitializeBufferAtChunk(int nChunk, KWDataTableSliceSet* sliceSet)
{
	boolean bOk;
	require(dataPreparationClass != NULL);
	require(layout.IsInitialized());
	require(schema.IsInitialized());

	chunkBuffer.SetLayout(&layout);
	bOk = chunkBuffer.Initialize(nChunk, dataPreparationClass->GetDataPreparationClass(), sliceSet, &schema);

	return bOk;
}

boolean SNBDataTableBinarySliceSet::IsPredictive(KWDataPreparationAttribute* dataPreparationAttribute)
{
	KWDataPreparationStats* preparedStats;

	require(dataPreparationAttribute != NULL);
	require(dataPreparationAttribute->GetPreparedStats() != NULL);
	require(dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() != NULL);

	preparedStats = dataPreparationAttribute->GetPreparedStats();

	return preparedStats->GetPreparedDataGridStats() != NULL and
	       preparedStats->GetPreparedDataGridStats()->ComputeSourceGridSize() > 1 and
	       preparedStats->GetSortValue() > 0;
}

void SNBDataTableBinarySliceSet::InitializeTargetValueIndexes(KWClassStats* classStats)
{
	require(ivTargetValueIndexes.GetSize() == 0);

	if (classStats->GetTargetAttributeType() == KWType::Symbol)
		InitializeSymbolTargetValueIndexes(classStats);
	else if (classStats->GetTargetAttributeType() == KWType::Continuous)
		InitializeContinuousTargetValueIndexes(classStats);

	ensure(ivTargetValueIndexes.GetSize() == classStats->GetInstanceNumber());
}

void SNBDataTableBinarySliceSet::InitializeSymbolTargetValueIndexes(KWClassStats* classStats)
{
	const KWDGSAttributePartition* partition;
	int nInstance;
	Symbol sTargetValue;
	IntObject* ioTargetValueIndex;
	NumericKeyDictionary nkdPartsToIndexes;

	require(classStats->GetTargetValueStats() != NULL);
	require(classStats->GetTargetValueStats()->GetAttributeNumber() > 0);

	// Recodage des valeurs cibles pour chaque instance
	// Memorisation des relations [partie -> index] dans un hash pour un recodage efficace
	partition = classStats->GetTargetValueStats()->GetAttributeAt(0);
	for (nInstance = 0; nInstance < classStats->GetInstanceNumber(); nInstance++)
	{
		sTargetValue = classStats->GetSymbolTargetValues()->GetAt(nInstance);
		ioTargetValueIndex = cast(IntObject*, nkdPartsToIndexes.Lookup(sTargetValue.GetNumericKey()));

		if (ioTargetValueIndex == NULL)
		{
			ioTargetValueIndex = new IntObject;
			ioTargetValueIndex->SetInt(partition->ComputeSymbolPartIndex(sTargetValue));
			nkdPartsToIndexes.SetAt(sTargetValue.GetNumericKey(), ioTargetValueIndex);
		}
		ivTargetValueIndexes.Add(ioTargetValueIndex->GetInt());
	}
	nkdPartsToIndexes.DeleteAll();
}

void SNBDataTableBinarySliceSet::InitializeContinuousTargetValueIndexes(KWClassStats* classStats)
{
	IntVector ivPartFrequencies;
	int nTotalFrequency;
	int nPart;
	IntVector ivContinuousInstanceMeanIndex;
	int nFrequency;
	int nInstance;
	Continuous cTargetValue;
	int nTargetValuePartIndex;

	require(classStats->GetTargetValueStats() != NULL);
	require(classStats->GetTargetValueStats()->Check());
	require(classStats->GetTargetValueStats()->GetAttributeNumber() > 0);

	// Creation d'un vecteur qui codifie la relation [index de valeur -> index moyenne d'instance]
	// Index de valeur : Index des differents valeurs de la cible
	// Index moyenne d'instance : Moyenne de tous les indexes des instances avec la meme valeur cible
	classStats->GetTargetValueStats()->ExportAttributePartFrequenciesAt(0, &ivPartFrequencies);
	nTotalFrequency = 0;
	for (nPart = 0; nPart < classStats->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(); nPart++)
	{
		nFrequency = ivPartFrequencies.GetAt(nPart);
		ivContinuousInstanceMeanIndex.Add(nTotalFrequency + nFrequency / 2);
		nTotalFrequency += nFrequency;
	}

	// Obtention de l'index de valeur et memorisation de l'index moyenne d'instance
	for (nInstance = 0; nInstance < classStats->GetInstanceNumber(); nInstance++)
	{
		cTargetValue = classStats->GetContinuousTargetValues()->GetAt(nInstance);
		nTargetValuePartIndex = ComputeContinuousTargetValuePartIndex(cTargetValue, classStats);
		ivTargetValueIndexes.Add(ivContinuousInstanceMeanIndex.GetAt(nTargetValuePartIndex));
	}
}

int SNBDataTableBinarySliceSet::ComputeContinuousTargetValuePartIndex(Continuous cTargetValue,
								      const KWClassStats* classStats) const
{
	const KWDGSAttributeContinuousValues* partition;
	int nLowerIndex;
	int nUpperIndex;
	int nIndex;

	require(classStats->GetTargetValueStats() != NULL);
	require(classStats->GetTargetValueStats()->Check());
	require(classStats->GetTargetValueStats()->GetAttributeNumber() > 0);

	// Initialization du pointeur de la partition des valeurs cibles et les indexes de recherche
	partition = cast(KWDGSAttributeContinuousValues*, classStats->GetTargetValueStats()->GetAttributeAt(0));
	nLowerIndex = 0;
	nUpperIndex = partition->GetPartNumber() - 1;
	nIndex = (nLowerIndex + nUpperIndex) / 2;

	// Recherche dichotomique de l'index de la partie ou se trouve la valeur cible
	// La recherche doit *toujours* reussir
	while (nLowerIndex <= nUpperIndex)
	{
		if (cTargetValue < partition->GetValueAt(nIndex))
			nUpperIndex = nIndex - 1;
		else if (cTargetValue > partition->GetValueAt(nIndex))
			nLowerIndex = nIndex + 1;
		else
			break;
		nIndex = (nLowerIndex + nUpperIndex) / 2;
	}
	ensure(nLowerIndex <= nUpperIndex);
	return nIndex;
}

boolean SNBDataTableBinarySliceSet::IsInitializedWithoutBuffer() const
{
	return layout.IsInitialized() and schema.IsInitialized() and ivTargetValueIndexes.GetSize() > 0 and
	       nInitialAttributeNumber > 0 and not chunkBuffer.IsInitialized();
}

boolean SNBDataTableBinarySliceSet::IsInitialized() const
{
	return layout.IsInitialized() and schema.IsInitialized() and ivTargetValueIndexes.GetSize() > 0 and
	       nInitialAttributeNumber > 0 and chunkBuffer.IsInitialized();
}

boolean SNBDataTableBinarySliceSet::HasUsableAttributes() const
{
	return schema.GetAttributeNumber() > 0;
}

void SNBDataTableBinarySliceSet::CleanWorkingData(boolean bDeleteDataPreparation)
{
	layout.CleanWorkingData();
	schema.CleanWorkingData();
	chunkBuffer.CleanWorkingData();
	randomizedAttributeIterator.CleanWorkingData();
	ivTargetValueIndexes.SetSize(0);

	if (dataPreparationClass != NULL)
	{
		if (bDeleteDataPreparation)
			dataPreparationClass->DeleteDataPreparation();
		else
			dataPreparationClass->RemoveDataPreparation();
		dataPreparationClass->SetLearningSpec(NULL);
		delete dataPreparationClass;
		dataPreparationClass = NULL;
	}
}

int SNBDataTableBinarySliceSet::GetAttributeNumber() const
{
	require(schema.IsInitialized());
	return schema.GetAttributeNumber();
}

int SNBDataTableBinarySliceSet::GetInitialAttributeNumber() const
{
	require(nInitialAttributeNumber >= 0);
	return nInitialAttributeNumber;
}

SNBDataTableBinarySliceSetAttribute* SNBDataTableBinarySliceSet::GetAttributeAt(int nAttribute) const
{
	require(schema.IsInitialized());
	return schema.GetAttributeAt(nAttribute);
}

SNBDataTableBinarySliceSetAttribute*
SNBDataTableBinarySliceSet::GetAttributeAtNativeName(const ALString& sAttributeName) const
{
	require(schema.IsInitialized());
	return schema.GetAttributeAtNativeName(sAttributeName);
}

SNBDataTableBinarySliceSetAttribute*
SNBDataTableBinarySliceSet::GetAttributeAtRecodedAttribute(KWAttribute* recodedAttribute) const
{
	require(schema.IsInitialized());
	return schema.GetAttributeAtRecodedAttribute(recodedAttribute);
}

KWDataPreparationAttribute* SNBDataTableBinarySliceSet::GetDataPreparationAttributeAt(int nAttribute)
{
	ObjectArray* oaDataPreparationAttributes;
	SNBDataTableBinarySliceSetAttribute* attribute;

	require(dataPreparationClass != NULL);
	require(0 <= nAttribute and nAttribute < GetAttributeNumber());

	oaDataPreparationAttributes = dataPreparationClass->GetDataPreparationAttributes();
	attribute = schema.GetAttributeAt(nAttribute);

	return cast(KWDataPreparationAttribute*,
		    oaDataPreparationAttributes->GetAt(attribute->GetDataPreparationClassIndex()));
}

const ALString SNBDataTableBinarySliceSet::GetAttributeNameAt(int nAttribute) const
{
	require(schema.IsInitialized());
	return schema.GetAttributeAt(nAttribute)->GetNativeAttributeName();
}

boolean SNBDataTableBinarySliceSet::ContainsAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) const
{
	require(attribute != NULL);
	require(schema.IsInitialized());

	return attribute == schema.GetAttributeAt(attribute->GetIndex());
}

SNBDataTableBinarySliceSetAttribute* SNBDataTableBinarySliceSet::GetRandomAttributeAt(int nRandomAttribute) const
{
	require(randomizedAttributeIterator.IsInitialized());
	return randomizedAttributeIterator.GetRandomAttributeAt(nRandomAttribute);
}

void SNBDataTableBinarySliceSet::ShuffleRandomAttributeIterator()
{
	require(randomizedAttributeIterator.IsInitialized());
	randomizedAttributeIterator.Shuffle();
}

void SNBDataTableBinarySliceSet::RestoreRandomAttributeIterator()
{
	require(randomizedAttributeIterator.IsInitialized());
	randomizedAttributeIterator.Restore();
}

int SNBDataTableBinarySliceSet::GetInstanceNumber() const
{
	require(layout.IsInitialized());
	return layout.GetInstanceNumber();
}

int SNBDataTableBinarySliceSet::GetActiveInstanceNumber() const
{
	require(IsInitialized());
	return layout.GetInstanceNumberAtChunk(chunkBuffer.GetChunkIndex());
}

int SNBDataTableBinarySliceSet::GetTargetValueIndexAt(int nInstance) const
{
	require(0 <= nInstance and nInstance < layout.GetInstanceNumber());
	require(ivTargetValueIndexes.GetSize() > 0);
	return ivTargetValueIndexes.GetAt(nInstance);
}

int SNBDataTableBinarySliceSet::GetTargetValueIndexAtActiveInstance(int nActiveInstance) const
{
	require(IsInitialized());
	return GetTargetValueIndexAt(layout.GetInstanceOffsetAtChunk(chunkBuffer.GetChunkIndex()) + nActiveInstance);
}

void SNBDataTableBinarySliceSet::ExportTargetPartFrequencies(const SNBDataTableBinarySliceSetAttribute* attribute,
							     IntVector* ivOutput) const
{
	const KWDataGridStats* preparedDataGridStats;
	int nTargetAttribute;

	require(IsInitialized());
	require(ContainsAttribute(attribute));

	preparedDataGridStats = attribute->GetPreparedDataGridStats();
	nTargetAttribute = preparedDataGridStats->GetFirstTargetAttributeIndex();
	preparedDataGridStats->ExportAttributePartFrequenciesAt(nTargetAttribute, ivOutput);
}

boolean SNBDataTableBinarySliceSet::UpdateTargetValueScores(SNBDataTableBinarySliceSetAttribute* attribute, int nTarget,
							    Continuous cDeltaWeight, ContinuousVector* cvScores)
{
	boolean bOk = true;
	SNBDataTableBinarySliceSetColumn* column;
	int nChunkInstanceNumber;
	int nChunkInstance;
	Continuous cLogProb;
	int nColumnValueIndex;
	int nColumnValueNumber;

	require(IsInitialized());
	require(cvScores != NULL);
	require(cvScores->GetSize() == GetActiveInstanceNumber());
	require(ContainsAttribute(attribute));

	// Lecture des indexes de recodage pour l'attribut et chunk courant
	column = NULL;
	bOk = chunkBuffer.CollectRecodedAttributeIndexes(attribute->GetIndex(), column);
	if (not bOk)
		bIsError = true;
	if (bOk)
	{
		// Mise a jour des log-probabilites de la cible dans le vecteur de sortie
		// Cas sparse : On ne met a jour que les scores des instances ou l'attribut n'est pas manquante
		if (column->IsSparse())
		{
			nColumnValueNumber = column->GetValueNumber();
			for (nColumnValueIndex = 0; nColumnValueIndex < nColumnValueNumber; nColumnValueIndex++)
			{
				nChunkInstance = column->GetSparseValueInstanceIndexAt(nColumnValueIndex);
				cLogProb = attribute->GetLnSourceConditionalProb(
				    column->GetSparseValueAt(nColumnValueIndex), nTarget);
				cvScores->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
			}
		}
		// Cas sparse : On ne met a jour les scores de toutes les instances
		else
		{
			nChunkInstanceNumber = layout.GetInstanceNumberAtChunk(chunkBuffer.GetChunkIndex());
			for (nChunkInstance = 0; nChunkInstance < nChunkInstanceNumber; nChunkInstance++)
			{
				cLogProb = attribute->GetLnSourceConditionalProb(
				    column->GetDenseValueAt(nChunkInstance), nTarget);
				cvScores->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
			}
		}
	}
	return bOk;
}

void SNBDataTableBinarySliceSet::WriteContentsAsTSV(ostream& ost)
{
	if (layout.IsInitialized())
	{
		ost << "Binary Slice Set\n";
		ost << layout << "\n\n";
		chunkBuffer.WriteContentsAsTSV(ost, &schema);
		ost << "----------\n";
	}
	else
		ost << "Binary slice set layout not initialized\n";
}

boolean SNBDataTableBinarySliceSet::IsError() const
{
	return bIsError;
}

KWDataPreparationClass* SNBDataTableBinarySliceSet::GetDataPreparationClass()
{
	require(dataPreparationClass != NULL);
	return dataPreparationClass;
}

boolean SNBDataTableBinarySliceSet::Check() const
{
	boolean bOk = true;
	int nSlice;
	int nSliceAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;

	if (dataPreparationClass != NULL)
	{
		// bIsInitialized = bIsInitialized and oaUsedDataPreparationAttributes != NULL;
		bOk = bOk and dataPreparationClass->CheckDataPreparation();
		bOk = bOk and dataPreparationClass->GetDataPreparationClass()->Check();
		// TODO: GetDataPreparationAttributes n'est pas const !
		// bIsInitialized = bIsInitialized and nInitialAttributeNumber == dataPreparationClass.GetDataPreparationAttributes()->GetDataSize();
	}

	if (schema.IsInitialized())
	{
		bOk = bOk and HasUsableAttributes();
		bOk = bOk and schema.Check();
	}

	if (layout.IsInitialized())
	{
		bOk = bOk and schema.IsInitialized();
		bOk = bOk and layout.Check();
		if (nInitialAttributeNumber >= 0)
			bOk = bOk and layout.GetAttributeNumber() <= nInitialAttributeNumber;
	}

	if (randomizedAttributeIterator.IsInitialized())
	{
		bOk = bOk and layout.IsInitialized();
		bOk = bOk and schema.IsInitialized();

		// Coherence des indexes des attributs
		for (nSlice = 0; nSlice < layout.GetSliceNumber(); nSlice++)
		{
			for (nSliceAttribute = 0; nSliceAttribute < layout.GetAttributeNumberAtSlice(nSlice);
			     nSliceAttribute++)
			{
				attribute = cast(
				    SNBDataTableBinarySliceSetAttribute*,
				    schema.GetAttributeAt(layout.GetAttributeOffsetAtSlice(nSlice) + nSliceAttribute));
				bOk = bOk and attribute->GetIndex() ==
						  layout.GetAttributeOffsetAtSlice(nSlice) + nSliceAttribute;
			}
		}
	}

	if (chunkBuffer.IsInitialized())
	{
		bOk = bOk and layout.IsInitialized();
		bOk = bOk and schema.IsInitialized();
		bOk = bOk and chunkBuffer.GetLayout() == &layout;
		bOk = bOk and chunkBuffer.Check();
	}

	if (ivTargetValueIndexes.GetSize() > 0)
	{
		bOk = bOk and layout.IsInitialized();
		bOk = bOk and ivTargetValueIndexes.GetSize() == layout.GetInstanceNumber();
	}

	bOk = bOk and not bIsError;

	return bOk;
}

const ALString SNBDataTableBinarySliceSet::GetClassLabel() const
{
	return "Data Table Binary Slice Set";
}

longint SNBDataTableBinarySliceSet::ComputeTargetValuesNecessaryMemory(int nInstanceNumber)
{
	return (longint)nInstanceNumber * sizeof(int);
}
