// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDSelectionOperandDataSampler.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandDataSampler

KDSelectionOperandDataSampler::KDSelectionOperandDataSampler()
{
	nMaxSampleSize = 0;
	lDatabaseFileSize = 0;
	lRandomPrimeFactor = 0;
}

KDSelectionOperandDataSampler::~KDSelectionOperandDataSampler()
{
	CleanAll();
}

void KDSelectionOperandDataSampler::CleanData()
{
	KDClassSelectionData* classSelectionData;
	int i;

	// Reinitialisation des resultats par classe
	for (i = 0; i < oaClassSelectionData.GetSize(); i++)
	{
		classSelectionData = cast(KDClassSelectionData*, oaClassSelectionData.GetAt(i));
		classSelectionData->CleanData();
	}
}

void KDSelectionOperandDataSampler::CleanAll()
{
	// Nettoyage
	CleanData();

	// Destruction des resultats d'analyse
	odClassSelectionData.RemoveAll();
	oaClassSelectionData.DeleteAll();
}

const ALString& KDSelectionOperandDataSampler::GetDatabaseName()
{
	return sDatabaseName;
}

const ObjectArray* KDSelectionOperandDataSampler::GetClassSelectionData() const
{
	return &oaClassSelectionData;
}

KDClassSelectionData* KDSelectionOperandDataSampler::LookupClassSelectionData(const ALString& sClassName) const
{
	return cast(KDClassSelectionData*, odClassSelectionData.Lookup(sClassName));
}

void KDSelectionOperandDataSampler::SetMaxSampleSize(int nValue)
{
	require(nValue >= 0);
	nMaxSampleSize = nValue;
}

int KDSelectionOperandDataSampler::GetMaxSampleSize() const
{
	return nMaxSampleSize;
}

int KDSelectionOperandDataSampler::ComputeMaxSampleSize()
{
	const int nMinObjectNumberPerPartile = lKB;
	const int nMaxSize = lMB;
	const longint lMinTotalValueNumber = 128 * lKB;
	int nSampleSize;
	int nSelectionOperandNumber;
	int nMaxSelectionOperandGranularity;
	KDClassSelectionData* classSelectionData;
	int i;
	longint lTotalValueNumber;

	// Calcul de stats sur les operandes de selection utilises
	nSelectionOperandNumber = 0;
	nMaxSelectionOperandGranularity = 0;
	for (i = 0; i < GetClassSelectionData()->GetSize(); i++)
	{
		classSelectionData = cast(KDClassSelectionData*, GetClassSelectionData()->GetAt(i));

		// Mise a jour des stats
		nSelectionOperandNumber += classSelectionData->GetClassSelectionOperandData()->GetSize();
		nMaxSelectionOperandGranularity =
		    max(nMaxSelectionOperandGranularity, classSelectionData->GetMaxOperandGranularity());
	}

	// Nombre minimum d'objets a garder dans les echantillons pour avoir une estimation suffisament fine des
	// partiles
	nSampleSize = nMaxSelectionOperandGranularity * nMinObjectNumberPerPartile;
	nSampleSize = min(nSampleSize, nMaxSize);

	// On augmente ce nombre pour avoir des echantillons avec 2 partiles seulement ou avec quelques dizaines de
	// partiles Cela permet de stabiliser les resultats dans le cas de petits nombres de patiles par operande de
	// selection
	lTotalValueNumber = nSampleSize * nSelectionOperandNumber;
	if (nSampleSize > 0)
	{
		while (lTotalValueNumber < lMinTotalValueNumber)
		{
			nSampleSize *= 2;
			lTotalValueNumber *= 2;
		}
	}
	return nSampleSize;
}

int KDSelectionOperandDataSampler::GetTotalSelectionOperandNumber() const
{
	int nTotalSelectionOperandNumber;
	KDClassSelectionData* classSelectionData;
	int nClass;

	// Parcours des classes pour compter les operandes de selection
	nTotalSelectionOperandNumber = 0;
	for (nClass = 0; nClass < GetClassSelectionData()->GetSize(); nClass++)
	{
		classSelectionData = cast(KDClassSelectionData*, GetClassSelectionData()->GetAt(nClass));
		nTotalSelectionOperandNumber += classSelectionData->GetClassSelectionOperandData()->GetSize();
	}
	return nTotalSelectionOperandNumber;
}

KDSelectionOperandDataSampler* KDSelectionOperandDataSampler::CloneSpec() const
{
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	KDClassSelectionData* classSelectionData;
	KDClassSelectionOperandData* selectionOperandData;
	KDClassSelectionData* sourceClassSelectionData;
	KDClassSelectionOperandData* sourceSelectionOperandData;
	int nClass;
	int nOperand;

	require(Check());

	// Creation du sampler
	selectionOperandDataSampler = new KDSelectionOperandDataSampler;

	// Recopie des informations principales
	selectionOperandDataSampler->sDatabaseName = sDatabaseName;
	selectionOperandDataSampler->nMaxSampleSize = nMaxSampleSize;
	selectionOperandDataSampler->lDatabaseFileSize = lDatabaseFileSize;
	selectionOperandDataSampler->lRandomPrimeFactor = lRandomPrimeFactor;

	// Ajout des classes de selection
	for (nClass = 0; nClass < GetClassSelectionData()->GetSize(); nClass++)
	{
		sourceClassSelectionData = cast(KDClassSelectionData*, GetClassSelectionData()->GetAt(nClass));

		// Ajout d'une classe de collecte de donnees
		classSelectionData = new KDClassSelectionData;
		classSelectionData->sClassName = sourceClassSelectionData->GetClassName();
		selectionOperandDataSampler->oaClassSelectionData.Add(classSelectionData);

		// Ajout des operandes de seletcion
		for (nOperand = 0; nOperand < sourceClassSelectionData->GetClassSelectionOperandData()->GetSize();
		     nOperand++)
		{
			sourceSelectionOperandData =
			    cast(KDClassSelectionOperandData*,
				 sourceClassSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));

			// Ajout d'une operande de collecte de donnees
			selectionOperandData = new KDClassSelectionOperandData;
			selectionOperandData->sSelectionAttributeName =
			    sourceSelectionOperandData->sSelectionAttributeName;
			selectionOperandData->selectionAttribute = sourceSelectionOperandData->GetSelectionAttribute();
			classSelectionData->oaClassSelectionOperandData.Add(selectionOperandData);
		}
	}

	// Mise en phase du dictionnaire des classes
	selectionOperandDataSampler->RefreshClassSelectionData();
	ensure(selectionOperandDataSampler->Check());
	return selectionOperandDataSampler;
}

void KDSelectionOperandDataSampler::SwapClassSelectionData(
    KDSelectionOperandDataSampler* otherSelectionOperandDataSampler)
{
	ObjectArray oaSwap;

	require(Check());
	require(otherSelectionOperandDataSampler != NULL);
	require(otherSelectionOperandDataSampler != this);
	require(otherSelectionOperandDataSampler->Check());

	// Echange du contenu par classe
	oaSwap.CopyFrom(&oaClassSelectionData);
	oaClassSelectionData.CopyFrom(&otherSelectionOperandDataSampler->oaClassSelectionData);
	otherSelectionOperandDataSampler->oaClassSelectionData.CopyFrom(&oaSwap);

	// Mise en phase des dictionnaire de classe
	RefreshClassSelectionData();
	otherSelectionOperandDataSampler->RefreshClassSelectionData();
}

void KDSelectionOperandDataSampler::AggregateClassSelectionData(
    KDSelectionOperandDataSampler* sourceSelectionOperandDataSampler)
{
	KDClassSelectionData* classSelectionData;
	KDClassSelectionOperandData* selectionOperandData;
	KDClassSelectionData* sourceClassSelectionData;
	KDClassSelectionOperandData* sourceSelectionOperandData;
	int nClass;
	int nOperand;
	KDClassSelectionObjectRef* sourceSelectionObjectRef;
	KDClassSelectionObjectRef* selectionObjectRef;
	POSITION foundPosition;
	int nSourceValueIndex;
	int nValueIndex;
	KWAttribute* selectionAttribute;

	require(Check());
	require(sourceSelectionOperandDataSampler != NULL);
	require(sourceSelectionOperandDataSampler != this);
	require(sourceSelectionOperandDataSampler->Check());
	require(sourceSelectionOperandDataSampler->GetClassSelectionData()->GetSize() ==
		GetClassSelectionData()->GetSize());
	require(sourceSelectionOperandDataSampler->GetMaxSampleSize() == GetMaxSampleSize());
	require(sourceSelectionOperandDataSampler->lDatabaseFileSize == lDatabaseFileSize);
	require(sourceSelectionOperandDataSampler->lRandomPrimeFactor == lRandomPrimeFactor);

	// Parcours des classes de selection
	for (nClass = 0; nClass < sourceSelectionOperandDataSampler->GetClassSelectionData()->GetSize(); nClass++)
	{
		sourceClassSelectionData = cast(
		    KDClassSelectionData*, sourceSelectionOperandDataSampler->GetClassSelectionData()->GetAt(nClass));
		classSelectionData = cast(KDClassSelectionData*, GetClassSelectionData()->GetAt(nClass));
		assert(sourceClassSelectionData->GetClassName() == classSelectionData->GetClassName());
		assert(sourceClassSelectionData->GetSampleSize() <=
		       sourceSelectionOperandDataSampler->GetMaxSampleSize());
		assert(sourceClassSelectionData->GetClassSelectionOperandData()->GetSize() ==
		       classSelectionData->GetClassSelectionOperandData()->GetSize());
		assert(classSelectionData->GetSampleSize() <= GetMaxSampleSize());
		assert(classSelectionData->GetObjectSelectionProbThreshold() == 0);

		// Prise en compte des objets analyses supplementaires
		classSelectionData->SetAnalysedObjectNumber(classSelectionData->GetAnalyzedObjectNumber() +
							    sourceClassSelectionData->GetAnalyzedObjectNumber());

		// Prise en compte des objets selectionnes sources pour les integrer dans l'echantillon courant
		while (sourceClassSelectionData->GetSelectionObjects()->GetCount() > 0)
		{
			// On depile la prochaine reference d'objet source
			sourceSelectionObjectRef = cast(KDClassSelectionObjectRef*,
							sourceClassSelectionData->GetSelectionObjects()->RemoveHead());
			nSourceValueIndex = sourceSelectionObjectRef->GetValueIndex();

			// On cherche si elle est deja prise en compte, ce qui peut arriver dans le cas de donnees des
			// tables externes
			foundPosition = classSelectionData->GetSelectionObjects()->Find(sourceSelectionObjectRef);

			// Ajout potentiel de l'objet s'il n'est pas deja prise en compte
			nValueIndex = -2;
			if (foundPosition != NULL)
				delete sourceSelectionObjectRef;
			else
			{
				// Ajout de l'objet s'il reste de la place
				if (classSelectionData->GetSelectionObjects()->GetCount() < nMaxSampleSize)
				{
					// Index de valeur en derniere position
					nValueIndex = classSelectionData->GetSelectionObjects()->GetCount();
					sourceSelectionObjectRef->SetValueIndex(nValueIndex);

					// Memorisation dans la liste triee des objets selectionnes
					classSelectionData->GetSelectionObjects()->Add(sourceSelectionObjectRef);
				}
				// Sinon, on regarde si on a une proba de selection suffisante
				else
				{
					// Rercherche en tete de liste de la reference d'objet avec la plus petite proba
					// de selection
					selectionObjectRef = cast(KDClassSelectionObjectRef*,
								  classSelectionData->GetSelectionObjects()->GetHead());

					// On remplace cet objet par le nouvel objet si on a une meilleure proba
					if (sourceSelectionObjectRef->Compare(selectionObjectRef) > 0)
					{
						// On memorise l'index de valeur que l'on va reutiliser
						nValueIndex = selectionObjectRef->GetValueIndex();
						sourceSelectionObjectRef->SetValueIndex(nValueIndex);

						// On supprime l'ancien objet de la liste triee
						classSelectionData->GetSelectionObjects()->RemoveHead();
						delete selectionObjectRef;

						// Memorisation dans la liste triee des objets selectionnes
						classSelectionData->GetSelectionObjects()->Add(
						    sourceSelectionObjectRef);
					}
					// Sinon, on ignore l'objet
					else
					{
						nValueIndex = -1;
						delete sourceSelectionObjectRef;
					}
				}
			}

			// Prise en compte des valeurs si l'objet est ajoute
			if (nValueIndex >= 0)
			{
				// Parcours des operandes de selection
				for (nOperand = 0;
				     nOperand < sourceClassSelectionData->GetClassSelectionOperandData()->GetSize();
				     nOperand++)
				{
					sourceSelectionOperandData = cast(
					    KDClassSelectionOperandData*,
					    sourceClassSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));
					selectionOperandData =
					    cast(KDClassSelectionOperandData*,
						 classSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));
					assert(sourceSelectionOperandData->GetSelectionAttributeName() ==
					       selectionOperandData->GetSelectionAttributeName());

					// Acces a l'attribut de selection
					selectionAttribute = selectionOperandData->GetSelectionAttribute();
					check(selectionAttribute);
					assert(KWType::IsSimple(selectionAttribute->GetType()));

					// Mise a jour des valeurs de selection selon le type de l'operande de selection
					if (selectionAttribute->GetType() == KWType::Symbol)
					{
						// Remplacement ou ajout de la valeur selon son index
						if (nValueIndex ==
						    selectionOperandData->GetSymbolInputData()->GetSize())
							selectionOperandData->GetSymbolInputData()->Add(
							    sourceSelectionOperandData->GetSymbolInputData()->GetAt(
								nSourceValueIndex));
						else
							selectionOperandData->GetSymbolInputData()->SetAt(
							    nValueIndex,
							    sourceSelectionOperandData->GetSymbolInputData()->GetAt(
								nSourceValueIndex));
						assert(selectionOperandData->GetSymbolInputData()->GetSize() ==
						       classSelectionData->GetSelectionObjects()->GetCount());
					}
					else
					{
						// Remplacement ou ajout de la valeur selon son index
						if (nValueIndex ==
						    selectionOperandData->GetContinuousInputData()->GetSize())
							selectionOperandData->GetContinuousInputData()->Add(
							    sourceSelectionOperandData->GetContinuousInputData()->GetAt(
								nSourceValueIndex));
						else
							selectionOperandData->GetContinuousInputData()->SetAt(
							    nValueIndex,
							    sourceSelectionOperandData->GetContinuousInputData()->GetAt(
								nSourceValueIndex));
						assert(selectionOperandData->GetContinuousInputData()->GetSize() ==
						       classSelectionData->GetSelectionObjects()->GetCount());
					}
				}
			}
		}

		// Nettoyage de la classe source
		assert(sourceClassSelectionData->GetSelectionObjects()->GetCount() == 0);
		sourceClassSelectionData->CleanData();
	}
	ensure(Check());
}

boolean KDSelectionOperandDataSampler::Check() const
{
	boolean bOk = true;
	KDClassSelectionData* classSelectionData;
	int nClass;
	KWClass* kwcClass;
	KDClassSelectionOperandData* classSelectionOperandData;
	int nOperand;
	KWAttribute* attribute;
	IntVector ivCheckValueIndexes;
	POSITION position;
	KDClassSelectionObjectRef* previousSelectionObjectRef;
	KDClassSelectionObjectRef* selectionObjectRef;

	// Parcours des classes pour verifier les attributs de selection
	for (nClass = 0; nClass < oaClassSelectionData.GetSize(); nClass++)
	{
		classSelectionData = cast(KDClassSelectionData*, oaClassSelectionData.GetAt(nClass));

		// Recherche de la classe specifiee
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(classSelectionData->GetClassName());
		bOk = bOk and kwcClass != NULL;
		assert(bOk);

		// Verification de l'unicite des reference aux objets et des index de valeurs
		ivCheckValueIndexes.SetSize(classSelectionData->slSelectedObjects.GetCount());
		ivCheckValueIndexes.Initialize();
		selectionObjectRef = NULL;
		position = classSelectionData->slSelectedObjects.GetHeadPosition();
		while (position != NULL)
		{
			previousSelectionObjectRef = selectionObjectRef;
			selectionObjectRef =
			    cast(KDClassSelectionObjectRef*, classSelectionData->slSelectedObjects.GetNext(position));

			// Verification de l'unicite des references aux objets
			bOk = bOk and (previousSelectionObjectRef == NULL or
				       previousSelectionObjectRef->Compare(selectionObjectRef) < 0);

			// Verification de l'index
			bOk = bOk and selectionObjectRef->GetValueIndex() >= 0;
			bOk = bOk and
			      selectionObjectRef->GetValueIndex() < classSelectionData->slSelectedObjects.GetCount();
			assert(bOk);
			if (bOk)
			{
				bOk = bOk and ivCheckValueIndexes.GetAt(selectionObjectRef->GetValueIndex()) == 0;
				assert(bOk);
				ivCheckValueIndexes.SetAt(selectionObjectRef->GetValueIndex(), 1);
			}
		}
		assert(bOk);

		// Parcours des operandes pour verifier leur attribut de selection
		if (bOk)
		{
			for (nOperand = 0; nOperand < classSelectionData->GetClassSelectionOperandData()->GetSize();
			     nOperand++)
			{
				classSelectionOperandData =
				    cast(KDClassSelectionOperandData*,
					 classSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));

				// Verification de l'attribut de selection
				attribute =
				    kwcClass->LookupAttribute(classSelectionOperandData->GetSelectionAttributeName());
				bOk = bOk and attribute != NULL;
				bOk = bOk and classSelectionOperandData->selectionAttribute == attribute;
				assert(bOk);

				// Verification de la taille des donnees collectees
				bOk = bOk and (classSelectionOperandData->cvContinuousInputData.GetSize() == 0 or
					       classSelectionOperandData->svSymbolInputData.GetSize() == 0);
				bOk = bOk and (classSelectionData->slSelectedObjects.GetCount() ==
						   classSelectionOperandData->cvContinuousInputData.GetSize() or
					       classSelectionData->slSelectedObjects.GetCount() ==
						   classSelectionOperandData->svSymbolInputData.GetSize());
				assert(bOk);
			}
		}
	}
	return bOk;
}

void KDSelectionOperandDataSampler::Write(ostream& ost) const
{
	KDClassSelectionData* classSelectionData;
	int nClass;

	// Parcours des classes
	ost << GetClassLabel() << " " << GetObjectLabel() << " " << GetMaxSampleSize() << "\n";
	for (nClass = 0; nClass < oaClassSelectionData.GetSize(); nClass++)
	{
		classSelectionData = cast(KDClassSelectionData*, oaClassSelectionData.GetAt(nClass));
		ost << "\t" << classSelectionData->GetClassName() << "("
		    << classSelectionData->GetClassSelectionOperandData()->GetSize() << ")"
		    << " " << classSelectionData->GetSampleSize() << " "
		    << classSelectionData->GetObjectSelectionProbThreshold() << ": "
		    << classSelectionData->GetAnalyzedObjectNumber() << " "
		    << classSelectionData->GetSampleMinSelectionProb() << "\n";
	}
}

longint KDSelectionOperandDataSampler::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KDSelectionOperandDataSampler);
	lUsedMemory += sDatabaseName.GetUsedMemory() - sizeof(ALString);
	lUsedMemory += oaClassSelectionData.GetOverallUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += odClassSelectionData.GetUsedMemory() - sizeof(ObjectDictionary);
	lUsedMemory += nkdAllReferencedObjects.GetUsedMemory() - sizeof(NumericKeyDictionary);
	lUsedMemory += nkdAllAnalyzedReferencedObjects.GetUsedMemory() - sizeof(NumericKeyDictionary);
	return lUsedMemory;
}

const ALString KDSelectionOperandDataSampler::GetClassLabel() const
{
	return "Selection operand data sampler";
}

const ALString KDSelectionOperandDataSampler::GetObjectLabel() const
{
	return sDatabaseName;
}

void KDSelectionOperandDataSampler::ExtractSelectionObjects(const KWObject* kwoObject, longint lMainObjectIndex,
							    longint& lSubObjectIndex,
							    NumericKeyDictionary* nkdAllSubObjects)
{
	const KWClass* kwcClass;
	int nAttribute;
	KWAttribute* attribute;
	KWObject* kwoSubObject;
	ObjectArray* oaSubObjects;
	int i;
	int nAttributeBlock;
	KWAttributeBlock* attributeBlock;
	KWObjectArrayValueBlock* valueBlock;
	int nValue;
	KDClassSelectionData* classSelectionData;
	boolean bExtractObjectValues;

	require(kwoObject != NULL);
	require(lMainObjectIndex >= 0);
	require(lSubObjectIndex >= 0);
	require(nkdAllSubObjects != NULL);
	require(nkdAllSubObjects->Lookup(kwoObject) == NULL);

	// Si objet principal, memorisation des objets des tables externes analyses
	bExtractObjectValues = true;
	if (lMainObjectIndex > 0)
	{
		// Si objet reference, memorisation de l'objet dans liste des objet references analyses
		if (nkdAllReferencedObjects.Lookup(kwoObject) != NULL)
		{
			// Arret si objet enregistre dans les objets references deja analyses
			if (nkdAllAnalyzedReferencedObjects.Lookup(kwoObject) != NULL)
				return;

			// Enregistrement sinon
			nkdAllAnalyzedReferencedObjects.SetAt(kwoObject, cast(Object*, kwoObject));

			// On n'extrait pas les valeurs de l'objet: ce sera fait specifiquement pour les tables externes
			// apres l'analyse de tous les objets principaux
			bExtractObjectValues = false;
		}
		// Sinon, memorisation de l'objet dans la liste des sous-objets explores
		else
		{
			lSubObjectIndex++;
			nkdAllSubObjects->SetAt(kwoObject, cast(Object*, kwoObject));
		}
	}
	// Si objet d'un table externe
	else
	{
		// Memorisation dans la liste des objets explores pour n'explorer les objets externes qu'une seule fois
		lSubObjectIndex++;
		nkdAllSubObjects->SetAt(kwoObject, cast(Object*, kwoObject));

		// On extrait les valeurs de l'objet que s'il a ete reference depuis les objets principaux
		bExtractObjectValues = (nkdAllAnalyzedReferencedObjects.Lookup(kwoObject) != NULL);
	}

	// Acces a la classe
	kwcClass = kwoObject->GetClass();
	check(kwcClass);

	// Extraction des valeurs de l'objet
	if (bExtractObjectValues)
	{
		// On determine s'il s'agit d'une classe de selection
		classSelectionData = cast(KDClassSelectionData*, odClassSelectionData.Lookup(kwcClass->GetName()));

		// Extraction des valeurs de selection du sous-objets
		if (classSelectionData != NULL)
			ExtractSelectionObjectValues(classSelectionData, kwoObject, lMainObjectIndex, lSubObjectIndex);
	}

	// Parcours de la structure Object et ObjectArray de l'objet
	for (nAttribute = 0; nAttribute < kwcClass->GetLoadedRelationAttributeNumber(); nAttribute++)
	{
		attribute = kwcClass->GetLoadedRelationAttributeAt(nAttribute);

		// Cas Object
		if (attribute->GetType() == KWType::Object)
		{
			kwoSubObject = kwoObject->GetObjectValueAt(attribute->GetLoadIndex());

			// Propagation au sous-objet
			if (kwoSubObject != NULL and nkdAllSubObjects->Lookup(kwoSubObject) == NULL)
				ExtractSelectionObjects(kwoSubObject, lMainObjectIndex, lSubObjectIndex,
							nkdAllSubObjects);
		}
		// Cas ObjectArray
		else
		{
			assert(attribute->GetType() == KWType::ObjectArray);

			// Propagation au sous-objet
			oaSubObjects = kwoObject->GetObjectArrayValueAt(attribute->GetLoadIndex());
			if (oaSubObjects != NULL)
			{
				// Propagation aux sous-objets
				for (i = 0; i < oaSubObjects->GetSize(); i++)
				{
					kwoSubObject = cast(KWObject*, oaSubObjects->GetAt(i));

					// Propagation au sous-objet
					if (kwoSubObject != NULL and nkdAllSubObjects->Lookup(kwoSubObject) == NULL)
						ExtractSelectionObjects(kwoSubObject, lMainObjectIndex, lSubObjectIndex,
									nkdAllSubObjects);
				}
			}
		}
	}

	// Parcours des blocs de type ObjectArray
	for (nAttributeBlock = 0; nAttributeBlock < kwcClass->GetLoadedAttributeBlockNumber(); nAttributeBlock++)
	{
		attributeBlock = kwcClass->GetLoadedAttributeBlockAt(nAttributeBlock);

		// Traitement des blocs de de type ObjectArray
		if (attributeBlock->GetType() == KWType::ObjectArray)
		{
			valueBlock = kwoObject->GetObjectArrayValueBlockAt(attributeBlock->GetLoadIndex());

			// Parcours des valeurs du bloc
			for (nValue = 0; nValue < valueBlock->GetValueNumber(); nValue++)
			{
				// Propagation au sous-objet
				oaSubObjects = valueBlock->GetValueAt(nValue);
				check(oaSubObjects);

				// Propagation aux sous-objets
				for (i = 0; i < oaSubObjects->GetSize(); i++)
				{
					kwoSubObject = cast(KWObject*, oaSubObjects->GetAt(i));

					// Propagation au sous-objet
					if (kwoSubObject != NULL and nkdAllSubObjects->Lookup(kwoSubObject) == NULL)
						ExtractSelectionObjects(kwoSubObject, lMainObjectIndex, lSubObjectIndex,
									nkdAllSubObjects);
				}
			}
		}
	}
}

void KDSelectionOperandDataSampler::ExtractSelectionObjectValues(KDClassSelectionData* classSelectionData,
								 const KWObject* kwoObject, longint lMainObjectIndex,
								 longint lSubObjectIndex)
{
	int nOperand;
	KDClassSelectionOperandData* selectionOperandData;
	KWAttribute* selectionAttribute;
	double dSelectionProb;
	int nValueIndex;
	KDClassSelectionObjectRef newSelectionObjectRef;
	KDClassSelectionObjectRef* selectionObjectRef;

	require(classSelectionData != NULL);
	require(kwoObject != NULL);
	require(kwoObject->GetClass()->GetName() == classSelectionData->GetClassName());
	require(lMainObjectIndex >= 0);
	require(lSubObjectIndex >= 1);

	// Incrementation du nombre d'objets analyses
	classSelectionData->SetAnalysedObjectNumber(classSelectionData->GetAnalyzedObjectNumber() + 1);

	// Au lieu d'utiliser l'algorithme de Reservoir Sampling (Vitter, 1985) pour garder au plus
	// nMaxAnalysedObjectNumber selon un echantillonnage i.i.d. de l'ensemble des objets traites, on genere un
	// nombre aleatoire de facon reproductible en fonction du couple (MinObjectIndex, SubObjectIndex), et on ne
	// garde l'objet que si ce nombre aleatoire depasse un seuil de selection
	dSelectionProb = GetObjectRandomDouble(lMainObjectIndex, lSubObjectIndex);

	// On test si on a un seuil suffisant
	nValueIndex = -2;
	if (dSelectionProb < classSelectionData->GetObjectSelectionProbThreshold())
		nValueIndex = -1;
	else
	{
		// Initialisation d'une reference sur un objet de selection
		newSelectionObjectRef.SetMainObjectIndex(lMainObjectIndex);
		newSelectionObjectRef.SetSubObjectIndex(lSubObjectIndex);
		newSelectionObjectRef.SetSelectionProb(dSelectionProb);

		// Ajout d'un objet s'il reste de la place
		if (classSelectionData->GetSelectionObjects()->GetCount() < nMaxSampleSize)
		{
			// Index de valeur en derniere position
			nValueIndex = classSelectionData->GetSelectionObjects()->GetCount();

			// Creation d'un object avec un index de valeur en derniere position
			selectionObjectRef = newSelectionObjectRef.Clone();
			selectionObjectRef->SetValueIndex(nValueIndex);

			// Memorisation dans la liste triee des objets selectionnes
			classSelectionData->GetSelectionObjects()->Add(selectionObjectRef);
		}
		// Sinon, on regarde si on a une proba de selection suffisante
		else
		{
			// Rercherche en tete de liste de la reference d'objet avec la plus petite proba de selection
			selectionObjectRef =
			    cast(KDClassSelectionObjectRef*, classSelectionData->GetSelectionObjects()->GetHead());

			// On remplace cet objet par le nouvel objet si on a une meilleure proba
			if (newSelectionObjectRef.Compare(selectionObjectRef) > 0)
			{
				// On memorise l'index de valeur que l'on va reutiliser
				nValueIndex = selectionObjectRef->GetValueIndex();

				// On supprime l'objet de la liste triee
				classSelectionData->GetSelectionObjects()->RemoveHead();

				// On le modifie en recopiant la reference de l'objet et l'index e valeur
				selectionObjectRef->CopyFrom(&newSelectionObjectRef);
				selectionObjectRef->SetValueIndex(nValueIndex);

				// Memorisation dans la liste triee des objets selectionnes
				classSelectionData->GetSelectionObjects()->Add(selectionObjectRef);
			}
			// Sinon, on ignore l'objet
			else
				nValueIndex = -1;
		}
	}
	assert(-1 <= nValueIndex and nValueIndex < classSelectionData->GetSelectionObjects()->GetCount());

	// Parcours des operandes de selection si les valeurs de l'objets doivent etre memorises
	if (nValueIndex >= 0)
	{
		for (nOperand = 0; nOperand < classSelectionData->GetClassSelectionOperandData()->GetSize(); nOperand++)
		{
			// Acces a l'operande de selection
			selectionOperandData =
			    cast(KDClassSelectionOperandData*,
				 classSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));

			// Acces a l'attribut de selection
			selectionAttribute = selectionOperandData->GetSelectionAttribute();
			check(selectionAttribute);
			assert(selectionAttribute->GetParentClass()->GetName() == kwoObject->GetClass()->GetName());
			assert(KWType::IsSimple(selectionAttribute->GetType()));
			assert(selectionAttribute->GetUsed());

			// Mise a jour des valeurs de selection selon le type de l'operande de selection
			if (selectionAttribute->GetType() == KWType::Symbol)
			{
				// Remplacement ou ajout de la valeur selon son index
				if (nValueIndex == selectionOperandData->GetSymbolInputData()->GetSize())
					selectionOperandData->GetSymbolInputData()->Add(
					    kwoObject->GetSymbolValueAt(selectionAttribute->GetLoadIndex()));
				else
					selectionOperandData->GetSymbolInputData()->SetAt(
					    nValueIndex,
					    kwoObject->GetSymbolValueAt(selectionAttribute->GetLoadIndex()));
				assert(selectionOperandData->GetSymbolInputData()->GetSize() ==
				       classSelectionData->GetSelectionObjects()->GetCount());
			}
			else
			{
				// Remplacement ou ajout de la valeur selon son index
				if (nValueIndex == selectionOperandData->GetContinuousInputData()->GetSize())
					selectionOperandData->GetContinuousInputData()->Add(
					    kwoObject->GetContinuousValueAt(selectionAttribute->GetLoadIndex()));
				else
					selectionOperandData->GetContinuousInputData()->SetAt(
					    nValueIndex,
					    kwoObject->GetContinuousValueAt(selectionAttribute->GetLoadIndex()));
				assert(selectionOperandData->GetContinuousInputData()->GetSize() ==
				       classSelectionData->GetSelectionObjects()->GetCount());
			}
		}
	}
}

void KDSelectionOperandDataSampler::ExtractAllSelectionReferencedObjects(KWMTDatabase* inputDatabase)
{
	boolean bDisplay = false;
	const KWObjectReferenceResolver* objectReferenceSolver;
	ObjectArray oaRootClasses;
	int nClass;
	KWClass* rootClass;
	ObjectArray oaRootObjects;
	int nObject;
	KWObject* rootObject;
	NumericKeyDictionary nkdAllSubObjects;
	longint lSubObjectIndex = 0;

	require(inputDatabase != NULL);
	require(inputDatabase->IsMultiTableTechnology());
	require(inputDatabase->IsOpenedForRead());

	// Pas d'analyse si aucun objet reference
	if (nkdAllAnalyzedReferencedObjects.GetCount() == 0)
		return;

	// Affichage
	if (bDisplay)
	{
		cout << "All referenced entities\t" << nkdAllReferencedObjects.GetCount() << endl;
		cout << "All analyzed entities\t" << nkdAllAnalyzedReferencedObjects.GetCount() << endl;
	}

	// Acces au container des objet references
	objectReferenceSolver = inputDatabase->GetObjectReferenceSolver();

	// Parcours des classes des objet references
	// Les classes sont triees, ce qui garantie leur ordre de parcours
	objectReferenceSolver->ExportClasses(&oaRootClasses);
	for (nClass = 0; nClass < oaRootClasses.GetSize(); nClass++)
	{
		rootClass = cast(KWClass*, oaRootClasses.GetAt(nClass));

		// Affichage
		if (bDisplay)
			cout << "\t" << rootClass->GetName() << "\t"
			     << objectReferenceSolver->GetObjectNumber(rootClass) << endl;

		// Acces aux objets references pour cette classe
		objectReferenceSolver->ExportObjects(rootClass, &oaRootObjects);

		// Tri pour granatir l'ordre de parcours
		oaRootObjects.SetCompareFunction(KWObjectCompareCreationIndex);
		oaRootObjects.Sort();

		// Analyse des objets
		// Le tri des classes externes, puis le tri des objets par classe garantit que l'index attribue a chaque
		// objet racine externe est unique et reproductible
		// Note sur l'optimisation:
		//   . on pourrait eviter le tri des objets en utilisant directement leur CraetionIndex, mais ce serait
		//   plus
		//     complique a gerer dans le cas de plusieurs tables externes. Et le temps de tri est de toutes
		//     facon negligeable devant le temps de destruction de tous les objest externes lors de la fermeture
		//     de la base
		//  .
		for (nObject = 0; nObject < oaRootObjects.GetSize(); nObject++)
		{
			rootObject = cast(KWObject*, oaRootObjects.GetAt(nObject));

			// Analyse de l'objet pour enregistrer les objets des classes de selection
			if (nkdAllSubObjects.Lookup(rootObject) == NULL)
				ExtractSelectionObjects(rootObject, 0, lSubObjectIndex, &nkdAllSubObjects);
		}
	}
}

void KDSelectionOperandDataSampler::RegisterAllReferencedObjects(KWMTDatabase* inputDatabase)
{
	boolean bDisplay = false;
	const KWObjectReferenceResolver* objectReferenceSolver;
	ObjectArray oaRootClasses;
	int nClass;
	KWClass* rootClass;
	ObjectArray oaRootObjects;
	int nObject;
	KWObject* rootObject;

	require(inputDatabase != NULL);
	require(inputDatabase->IsMultiTableTechnology());
	require(inputDatabase->IsOpenedForRead());
	require(nkdAllReferencedObjects.GetCount() == 0);
	require(nkdAllAnalyzedReferencedObjects.GetCount() == 0);

	// Acces au container des objet references
	objectReferenceSolver = inputDatabase->GetObjectReferenceSolver();

	// Parcours des classes des objet references
	objectReferenceSolver->ExportClasses(&oaRootClasses);
	for (nClass = 0; nClass < oaRootClasses.GetSize(); nClass++)
	{
		rootClass = cast(KWClass*, oaRootClasses.GetAt(nClass));

		// Affichage
		if (bDisplay)
			cout << "\t" << rootClass->GetName() << "\t"
			     << objectReferenceSolver->GetObjectNumber(rootClass) << endl;

		// Acces aux objet references pour cette classe
		objectReferenceSolver->ExportObjects(rootClass, &oaRootObjects);
		for (nObject = 0; nObject < oaRootObjects.GetSize(); nObject++)
		{
			rootObject = cast(KWObject*, oaRootObjects.GetAt(nObject));

			// Enregistrement de l'objet racine et de sa composition
			RegisterReferencedObject(rootObject);
		}
	}

	// Affichage
	if (bDisplay)
		cout << "AllReferencedEntities\t" << nkdAllReferencedObjects.GetCount() << endl;
}

void KDSelectionOperandDataSampler::RegisterReferencedObject(KWObject* kwoObject)
{
	const KWClass* kwcClass;
	int nAttribute;
	KWAttribute* attribute;
	KWObject* kwoSubObject;
	ObjectArray* oaSubObjects;
	int i;
	int nAttributeBlock;
	KWAttributeBlock* attributeBlock;
	KWObjectArrayValueBlock* valueBlock;
	int nValue;

	require(kwoObject != NULL);

	// Arret si objet null ou deja enregistre
	if (kwoObject == NULL or nkdAllReferencedObjects.Lookup(kwoObject) != NULL)
		return;

	// Memorisation de l'objet
	nkdAllReferencedObjects.SetAt(kwoObject, kwoObject);

	// Acces a la classe
	kwcClass = kwoObject->GetClass();
	check(kwcClass);

	// Parcours de la structure Object et ObjectArray de l'objet
	for (nAttribute = 0; nAttribute < kwcClass->GetLoadedRelationAttributeNumber(); nAttribute++)
	{
		attribute = kwcClass->GetLoadedRelationAttributeAt(nAttribute);

		// Cas Object
		if (attribute->GetType() == KWType::Object)
		{
			kwoSubObject = kwoObject->GetObjectValueAt(attribute->GetLoadIndex());

			// Propagation au sous-objet
			RegisterReferencedObject(kwoSubObject);
		}
		// Cas ObjectArray
		else
		{
			assert(attribute->GetType() == KWType::ObjectArray);

			// Propagation aux sous-objets
			oaSubObjects = kwoObject->GetObjectArrayValueAt(attribute->GetLoadIndex());
			if (oaSubObjects != NULL)
			{
				// Propagation aux sous-objets
				for (i = 0; i < oaSubObjects->GetSize(); i++)
				{
					kwoSubObject = cast(KWObject*, oaSubObjects->GetAt(i));

					// Propagation au sous-objet
					RegisterReferencedObject(kwoSubObject);
				}
			}
		}
	}

	// Parcours des blocs de type ObjectArray
	for (nAttributeBlock = 0; nAttributeBlock < kwcClass->GetLoadedAttributeBlockNumber(); nAttributeBlock++)
	{
		attributeBlock = kwcClass->GetLoadedAttributeBlockAt(nAttributeBlock);

		// Traitement des blocs de de type ObjectArray
		if (attributeBlock->GetType() == KWType::ObjectArray)
		{
			valueBlock = kwoObject->GetObjectArrayValueBlockAt(attributeBlock->GetLoadIndex());

			// Parcours des valeurs du bloc
			for (nValue = 0; nValue < valueBlock->GetValueNumber(); nValue++)
			{
				// Propagation au sous-objet
				oaSubObjects = valueBlock->GetValueAt(nValue);
				check(oaSubObjects);

				// Propagation aux sous-objets
				for (i = 0; i < oaSubObjects->GetSize(); i++)
				{
					kwoSubObject = cast(KWObject*, oaSubObjects->GetAt(i));

					// Propagation au sous-objet
					RegisterReferencedObject(kwoSubObject);
				}
			}
		}
	}
}

void KDSelectionOperandDataSampler::CleanAllReferencedObjects()
{
	nkdAllAnalyzedReferencedObjects.RemoveAll();
	nkdAllReferencedObjects.RemoveAll();
}

void KDSelectionOperandDataSampler::SetDatabaseName(const ALString& sValue)
{
	sDatabaseName = sValue;
}

void KDSelectionOperandDataSampler::SetDatabaseFileSize(longint lValue)
{
	require(lValue >= 0);

	lDatabaseFileSize = lValue;

	// On recherche un nombre premier majorant la taille de la table principale, qui servira a genere les nombre
	// aleatoire de facon reproductible
	lRandomPrimeFactor = GetNextPrimeNumber(lDatabaseFileSize);
}

void KDSelectionOperandDataSampler::RefreshClassSelectionData()
{
	KDClassSelectionData* classSelectionData;
	int i;

	// On les classes du tableau dans le dictionnaire
	odClassSelectionData.RemoveAll();
	for (i = 0; i < oaClassSelectionData.GetSize(); i++)
	{
		classSelectionData = cast(KDClassSelectionData*, oaClassSelectionData.GetAt(i));
		odClassSelectionData.SetAt(classSelectionData->GetClassName(), classSelectionData);
	}
}

// Tableau de nombres premiers superieurs aux puissances de 2 pour des longint
// pour servir de base a la generation de nombre aleatoires associes
// a des enregistrements en multi-tables
static const longint lKDSelectionOperandDataSamplerPrimeSizes[] = {65537,
								   131101,
								   262147,
								   524309,
								   1048583,
								   2097169,
								   4194319,
								   8388617,
								   16777259,
								   33554467,
								   67108879,
								   134217757,
								   268435459,
								   536870923,
								   1073741827,
								   2147483659,
								   4294967311,
								   8589934609,
								   17179869209,
								   34359738421,
								   68719476767,
								   137438953481,
								   274877906951,
								   549755813911,
								   1099511627791,
								   2199023255579,
								   4398046511119,
								   8796093022237,
								   17592186044423,
								   35184372088891,
								   70368744177679,
								   140737488355333,
								   281474976710677,
								   562949953421381,
								   1125899906842679,
								   2251799813685269,
								   4503599627370517,
								   9007199254740997,
								   18014398509482143,
								   36028797018963971,
								   72057594037928017,
								   144115188075855881,
								   288230376151711813,
								   576460752303423619,
								   1152921504606847009,
								   2305843009213693967,
								   4611686018427388039};

longint KDSelectionOperandDataSampler::GetNextPrimeNumber(longint lMinValue) const
{
	const int nDictionaryPrimeSizeNumber = sizeof(lKDSelectionOperandDataSamplerPrimeSizes) / sizeof(longint);
	longint lPrimeNumber = 0;
	int i;

	require(0 <= lMinValue and
		lMinValue <= lKDSelectionOperandDataSamplerPrimeSizes[nDictionaryPrimeSizeNumber - 1]);

	// Rend la taille de table superieure ou egale a une taille donnee
	for (i = 0; i < nDictionaryPrimeSizeNumber; i++)
	{
		lPrimeNumber = lKDSelectionOperandDataSamplerPrimeSizes[i];
		if (lPrimeNumber >= lMinValue)
			break;
	}
	assert(0 < lPrimeNumber);
	return lPrimeNumber;
}

double KDSelectionOperandDataSampler::GetObjectRandomDouble(longint lMainIndex, longint lSecondaryIndex) const
{
	longint lIndex;
	double dResult;

	require(lRandomPrimeFactor > 0);

	// Calcul d'un index global par a technique des leepfrog
	lIndex = lMainIndex + lRandomPrimeFactor * lSecondaryIndex;
	if (lIndex < 0)
		lIndex += LLONG_MAX;
	dResult = IthRandomDouble(lIndex);
	return dResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionData

KDClassSelectionData::KDClassSelectionData()
{
	slSelectedObjects.SetCompareFunction(KDClassSelectionObjectRefCompare);
	nMaxOperandGranularity = 0;
	dObjectSelectionProbThreshold = 0;
	lAnalysedObjectNumber = 0;
}

KDClassSelectionData::~KDClassSelectionData()
{
	CleanAll();
}

const ALString KDClassSelectionData::GetClassName() const
{
	return sClassName;
}

const ObjectArray* KDClassSelectionData::GetClassSelectionOperandData() const
{
	return &oaClassSelectionOperandData;
}

int KDClassSelectionData::GetMaxOperandGranularity() const
{
	return nMaxOperandGranularity;
}

void KDClassSelectionData::SetObjectSelectionProbThreshold(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dObjectSelectionProbThreshold = dValue;
}

double KDClassSelectionData::GetObjectSelectionProbThreshold() const
{
	return dObjectSelectionProbThreshold;
}

int KDClassSelectionData::GetSampleSize() const
{
	return slSelectedObjects.GetCount();
}

double KDClassSelectionData::GetSampleMinSelectionProb() const
{
	if (slSelectedObjects.GetCount() == 0)
		return 0;
	else
		return cast(KDClassSelectionObjectRef*, slSelectedObjects.GetHead())->GetSelectionProb();
}

void KDClassSelectionData::SetAnalysedObjectNumber(longint lValue)
{
	require(lValue >= 0);
	lAnalysedObjectNumber = lValue;
}

longint KDClassSelectionData::GetAnalyzedObjectNumber() const
{
	return lAnalysedObjectNumber;
}

void KDClassSelectionData::CleanData()
{
	int i;
	KDClassSelectionOperandData* classSelectionOperandData;

	// Reinitialisationd des statistiques globales
	lAnalysedObjectNumber = 0;

	// Nettoyage des stats des operandes de selection
	for (i = 0; i < oaClassSelectionOperandData.GetSize(); i++)
	{
		classSelectionOperandData = cast(KDClassSelectionOperandData*, oaClassSelectionOperandData.GetAt(i));
		classSelectionOperandData->CleanData();
	}
}

void KDClassSelectionData::CleanAll()
{
	// Reinitialisationd des statistiques globales
	lAnalysedObjectNumber = 0;

	// Destruction des references d'objets selectionnes
	slSelectedObjects.DeleteAll();

	// Destruction des operandes de selection
	oaClassSelectionOperandData.DeleteAll();
}

void KDClassSelectionData::Write(ostream& ost) const
{
	KDClassSelectionOperandData* classSelectionOperandData;
	int nOperand;

	// Parcours des classes
	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";
	for (nOperand = 0; nOperand < oaClassSelectionOperandData.GetSize(); nOperand++)
	{
		classSelectionOperandData =
		    cast(KDClassSelectionOperandData*, oaClassSelectionOperandData.GetAt(nOperand));
		ost << "\t" << classSelectionOperandData->GetSelectionAttributeName() << "\n";
	}
}

longint KDClassSelectionData::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KDClassSelectionData);
	lUsedMemory += sClassName.GetUsedMemory() - sizeof(ALString);
	lUsedMemory += oaClassSelectionOperandData.GetOverallUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += slSelectedObjects.GetCount() *
		       (slSelectedObjects.GetUsedMemoryPerElement() + sizeof(KDClassSelectionObjectRef));
	return lUsedMemory;
}

const ALString KDClassSelectionData::GetClassLabel() const
{
	return "Dictionary selection data";
}

const ALString KDClassSelectionData::GetObjectLabel() const
{
	return GetClassName();
}

SortedList* KDClassSelectionData::GetSelectionObjects()
{
	return &slSelectedObjects;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionOperandData

KDClassSelectionOperandData::KDClassSelectionOperandData()
{
	selectionAttribute = NULL;
}

KDClassSelectionOperandData::~KDClassSelectionOperandData()
{
	CleanData();
}

const ALString KDClassSelectionOperandData::GetSelectionAttributeName() const
{
	return sSelectionAttributeName;
}

void KDClassSelectionOperandData::CleanData()
{
	cvContinuousInputData.SetSize(0);
	svSymbolInputData.SetSize(0);
}

longint KDClassSelectionOperandData::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KDClassSelectionOperandData);
	lUsedMemory += sSelectionAttributeName.GetUsedMemory() - sizeof(ALString);
	lUsedMemory += cvContinuousInputData.GetUsedMemory() - sizeof(ContinuousVector);
	lUsedMemory += svSymbolInputData.GetUsedMemory() - sizeof(SymbolVector);
	return lUsedMemory;
}

const ALString KDClassSelectionOperandData::GetClassLabel() const
{
	return "Dictionary selection operand data";
}

const ALString KDClassSelectionOperandData::GetObjectLabel() const
{
	if (selectionAttribute == NULL)
		return "";
	else
		return selectionAttribute->GetName();
}

void KDClassSelectionOperandData::SetSelectionAttribute(KWAttribute* attribute)
{
	selectionAttribute = attribute;
}

KWAttribute* KDClassSelectionOperandData::GetSelectionAttribute() const
{
	return selectionAttribute;
}

ContinuousVector* KDClassSelectionOperandData::GetContinuousInputData()
{
	require(selectionAttribute != NULL and selectionAttribute->GetType() == KWType::Continuous);
	return &cvContinuousInputData;
}

SymbolVector* KDClassSelectionOperandData::GetSymbolInputData()
{
	require(selectionAttribute != NULL and selectionAttribute->GetType() == KWType::Symbol);
	return &svSymbolInputData;
}

int KDSparseClassSelectionOperandDataCompare(const void* elem1, const void* elem2)
{
	KDClassSelectionOperandData* selectionOperandStast1;
	KDClassSelectionOperandData* selectionOperandStast2;

	// Acces aux donnees a comparer
	selectionOperandStast1 = cast(KDClassSelectionOperandData*, *(Object**)elem1);
	selectionOperandStast2 = cast(KDClassSelectionOperandData*, *(Object**)elem2);

	// Comparaison des nom des attributs de selection
	return selectionOperandStast1->GetSelectionAttribute()->GetName().Compare(
	    selectionOperandStast2->GetSelectionAttribute()->GetName());
}

////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionObjectRef

KDClassSelectionObjectRef::KDClassSelectionObjectRef()
{
	lMainObjectIndex = 0;
	lSubObjectIndex = 0;
	dSelectionProb = 0;
	nValueIndex = 0;
}

KDClassSelectionObjectRef::~KDClassSelectionObjectRef() {}

void KDClassSelectionObjectRef::SetMainObjectIndex(longint lValue)
{
	require(lValue >= 0);
	lMainObjectIndex = lValue;
}

longint KDClassSelectionObjectRef::GetMainObjectIndex() const
{
	return lMainObjectIndex;
}

void KDClassSelectionObjectRef::SetSubObjectIndex(longint lValue)
{
	require(lValue >= 0);
	lSubObjectIndex = lValue;
}

longint KDClassSelectionObjectRef::GetSubObjectIndex() const
{
	return lSubObjectIndex;
}

void KDClassSelectionObjectRef::SetSelectionProb(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dSelectionProb = dValue;
}

double KDClassSelectionObjectRef::GetSelectionProb() const
{
	return dSelectionProb;
}

void KDClassSelectionObjectRef::SetValueIndex(int nValue)
{
	require(nValue >= 0);
	nValueIndex = nValue;
}

int KDClassSelectionObjectRef::GetValueIndex() const
{
	return nValueIndex;
}

void KDClassSelectionObjectRef::CopyFrom(const KDClassSelectionObjectRef* aSource)
{
	require(aSource != NULL);
	lMainObjectIndex = aSource->lMainObjectIndex;
	lSubObjectIndex = aSource->lSubObjectIndex;
	dSelectionProb = aSource->dSelectionProb;
	nValueIndex = aSource->nValueIndex;
}

KDClassSelectionObjectRef* KDClassSelectionObjectRef::Clone() const
{
	KDClassSelectionObjectRef* aClone;
	aClone = new KDClassSelectionObjectRef;
	aClone->CopyFrom(this);
	return aClone;
}

int KDClassSelectionObjectRef::Compare(const KDClassSelectionObjectRef* otherObject) const
{
	int nCompare;

	// Comparaison sur la valeur aleatoire
	nCompare = CompareDouble(GetSelectionProb(), otherObject->GetSelectionProb());

	// Comparaison sur les index d'identification de l'objet en cas d'egalite
	if (nCompare == 0)
		nCompare = CompareLongint(GetMainObjectIndex(), otherObject->GetMainObjectIndex());
	if (nCompare == 0)
		nCompare = CompareLongint(GetSubObjectIndex(), otherObject->GetSubObjectIndex());
	return nCompare;
}

void KDClassSelectionObjectRef::Write(ostream& ost) const
{
	ost << '(' << lMainObjectIndex << ", " << lSubObjectIndex << ')';
}

const ALString KDClassSelectionObjectRef::GetClassLabel() const
{
	return "Selection object ref";
}

const ALString KDClassSelectionObjectRef::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = '(';
	sLabel += LongintToString(lMainObjectIndex);
	sLabel = ", ";
	sLabel += LongintToString(lSubObjectIndex);
	sLabel = ')';
	return sLabel;
}

int KDClassSelectionObjectRefCompare(const void* elem1, const void* elem2)
{
	KDClassSelectionObjectRef* selectionObjectRef1;
	KDClassSelectionObjectRef* selectionObjectRef2;
	int nCompare;

	// Acces aux donnees a comparer
	selectionObjectRef1 = cast(KDClassSelectionObjectRef*, *(Object**)elem1);
	selectionObjectRef2 = cast(KDClassSelectionObjectRef*, *(Object**)elem2);

	// Comparaison des objets
	nCompare = selectionObjectRef1->Compare(selectionObjectRef2);
	return nCompare;
}

///////////////////////////////////////////////////////
// Classe PLShared_SelectionOperandDataSampler

PLShared_SelectionOperandDataSampler::PLShared_SelectionOperandDataSampler() {}

PLShared_SelectionOperandDataSampler::~PLShared_SelectionOperandDataSampler() {}

void PLShared_SelectionOperandDataSampler::SetSelectionOperandDataSampler(
    KDSelectionOperandDataSampler* selectionOperandDataSampler)
{
	require(selectionOperandDataSampler != NULL);
	SetObject(selectionOperandDataSampler);
}

KDSelectionOperandDataSampler* PLShared_SelectionOperandDataSampler::GetSelectionOperandDataSampler()
{
	return cast(KDSelectionOperandDataSampler*, GetObject());
}

void PLShared_SelectionOperandDataSampler::FinalizeSpecification()
{
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	KDClassSelectionData* classSelectionData;
	int nClass;
	KWClass* kwcClass;
	KDClassSelectionOperandData* classSelectionOperandData;
	int nOperand;
	KWAttribute* attribute;

	// Acces a l'objet
	selectionOperandDataSampler = GetSelectionOperandDataSampler();

	// Parcours des classes pour parametrer les attributs de selection
	for (nClass = 0; nClass < selectionOperandDataSampler->oaClassSelectionData.GetSize(); nClass++)
	{
		classSelectionData =
		    cast(KDClassSelectionData*, selectionOperandDataSampler->oaClassSelectionData.GetAt(nClass));

		// Recherche de la classe specifiee
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(classSelectionData->GetClassName());
		check(kwcClass);

		// Parcours des operandes pour specifier leur attribut de selection
		for (nOperand = 0; nOperand < classSelectionData->GetClassSelectionOperandData()->GetSize(); nOperand++)
		{
			classSelectionOperandData =
			    cast(KDClassSelectionOperandData*,
				 classSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));
			attribute = kwcClass->LookupAttribute(classSelectionOperandData->GetSelectionAttributeName());
			check(attribute);

			// Parametrage de l'attribut
			classSelectionOperandData->selectionAttribute = attribute;
		}
	}
	ensure(selectionOperandDataSampler->Check());
}

void PLShared_SelectionOperandDataSampler::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	PLShared_ObjectArray shared_oaClassSelectionData(new PLShared_ClassSelectionData);

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	selectionOperandDataSampler = cast(KDSelectionOperandDataSampler*, o);

	// Informations principales
	serializer->PutString(selectionOperandDataSampler->sDatabaseName);
	serializer->PutInt(selectionOperandDataSampler->nMaxSampleSize);
	serializer->PutLongint(selectionOperandDataSampler->lDatabaseFileSize);
	serializer->PutLongint(selectionOperandDataSampler->lRandomPrimeFactor);

	// Serialisation des donnees par classe
	shared_oaClassSelectionData.SerializeObject(serializer, &selectionOperandDataSampler->oaClassSelectionData);
}

void PLShared_SelectionOperandDataSampler::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	PLShared_ObjectArray shared_oaClassSelectionData(new PLShared_ClassSelectionData);

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	selectionOperandDataSampler = cast(KDSelectionOperandDataSampler*, o);

	// Informations principales
	selectionOperandDataSampler->sDatabaseName = serializer->GetString();
	selectionOperandDataSampler->nMaxSampleSize = serializer->GetInt();
	selectionOperandDataSampler->lDatabaseFileSize = serializer->GetLongint();
	selectionOperandDataSampler->lRandomPrimeFactor = serializer->GetLongint();

	// Deserialisation des donnees par classe
	shared_oaClassSelectionData.DeserializeObject(serializer, &selectionOperandDataSampler->oaClassSelectionData);

	// Mise en phase du dictionnaire des classes
	selectionOperandDataSampler->RefreshClassSelectionData();
	ensure(selectionOperandDataSampler->oaClassSelectionData.GetSize() ==
	       selectionOperandDataSampler->odClassSelectionData.GetCount());
}

Object* PLShared_SelectionOperandDataSampler::Create() const
{
	return new KDSelectionOperandDataSampler;
}

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionData

PLShared_ClassSelectionData::PLShared_ClassSelectionData() {}

PLShared_ClassSelectionData::~PLShared_ClassSelectionData() {}

void PLShared_ClassSelectionData::SetClassSelectionData(KDClassSelectionData* classSelectionData)
{
	require(classSelectionData != NULL);
	SetObject(classSelectionData);
}

KDClassSelectionData* PLShared_ClassSelectionData::GetClassSelectionData()
{
	return cast(KDClassSelectionData*, GetObject());
}

void PLShared_ClassSelectionData::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KDClassSelectionData* classSelectionData;
	PLShared_ObjectArray shared_oaClassSelectionOperandData(new PLShared_ClassSelectionOperandData);
	PLShared_ObjectArray shared_oaClassSelectionObjectRef(new PLShared_ClassSelectionObjectRef);
	ObjectArray oaClassSelectionObjectRef;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	classSelectionData = cast(KDClassSelectionData*, o);

	// Nom de la classe
	serializer->PutString(classSelectionData->sClassName);

	// Granularite max de tous les operandes de selection
	serializer->PutInt(classSelectionData->nMaxOperandGranularity);

	// Serialisation des donnees par operande
	shared_oaClassSelectionOperandData.SerializeObject(serializer,
							   &classSelectionData->oaClassSelectionOperandData);

	// Serialisation des objets selectionnes en passant par un tableau temporaire
	classSelectionData->slSelectedObjects.ExportObjectArray(&oaClassSelectionObjectRef);
	shared_oaClassSelectionObjectRef.SerializeObject(serializer, &oaClassSelectionObjectRef);

	// Seuil de selection des objets
	serializer->PutDouble(classSelectionData->dObjectSelectionProbThreshold);

	// Nombre d'objets analyses
	serializer->PutLongint(classSelectionData->lAnalysedObjectNumber);
}

void PLShared_ClassSelectionData::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KDClassSelectionData* classSelectionData;
	PLShared_ObjectArray shared_oaClassSelectionOperandData(new PLShared_ClassSelectionOperandData);
	PLShared_ObjectArray shared_oaClassSelectionObjectRef(new PLShared_ClassSelectionObjectRef);
	ObjectArray oaClassSelectionObjectRef;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	classSelectionData = cast(KDClassSelectionData*, o);

	// Nom de la classe
	classSelectionData->sClassName = serializer->GetString();

	// Granularite max de tous les operandes de selection
	classSelectionData->nMaxOperandGranularity = serializer->GetInt();

	// Deserialisation des donnees par operande
	shared_oaClassSelectionOperandData.DeserializeObject(serializer,
							     &classSelectionData->oaClassSelectionOperandData);

	// Deserialisation des objets selectionnes en passant par un tableau temporaire
	shared_oaClassSelectionObjectRef.DeserializeObject(serializer, &oaClassSelectionObjectRef);
	oaClassSelectionObjectRef.SetCompareFunction(classSelectionData->slSelectedObjects.GetCompareFunction());
	oaClassSelectionObjectRef.ExportSortedList(&classSelectionData->slSelectedObjects);

	// Seuil de selection des objets
	classSelectionData->dObjectSelectionProbThreshold = serializer->GetDouble();

	// Nombre d'objets analyses
	classSelectionData->lAnalysedObjectNumber = serializer->GetLongint();
}

Object* PLShared_ClassSelectionData::Create() const
{
	return new KDClassSelectionData;
}

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionOperandData

PLShared_ClassSelectionOperandData::PLShared_ClassSelectionOperandData() {}

PLShared_ClassSelectionOperandData::~PLShared_ClassSelectionOperandData() {}

void PLShared_ClassSelectionOperandData::SetClassSelectionOperandData(
    KDClassSelectionOperandData* classSelectionOperandData)
{
	require(classSelectionOperandData != NULL);
	SetObject(classSelectionOperandData);
}

KDClassSelectionOperandData* PLShared_ClassSelectionOperandData::GetClassSelectionOperandData()
{
	return cast(KDClassSelectionOperandData*, GetObject());
}

void PLShared_ClassSelectionOperandData::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KDClassSelectionOperandData* classSelectionOperandData;
	PLShared_ContinuousVector sharedContinuousVector;
	PLShared_SymbolVector sharedSymbolVector;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	classSelectionOperandData = cast(KDClassSelectionOperandData*, o);

	// Nom de l'attribut de selection
	serializer->PutString(classSelectionOperandData->sSelectionAttributeName);

	// Donnees collectees
	sharedContinuousVector.SerializeObject(serializer, &classSelectionOperandData->cvContinuousInputData);
	sharedSymbolVector.SerializeObject(serializer, &classSelectionOperandData->svSymbolInputData);
}

void PLShared_ClassSelectionOperandData::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KDClassSelectionOperandData* classSelectionOperandData;
	PLShared_ContinuousVector sharedContinuousVector;
	PLShared_SymbolVector sharedSymbolVector;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	classSelectionOperandData = cast(KDClassSelectionOperandData*, o);

	// Nom de l'attribut de selection
	classSelectionOperandData->sSelectionAttributeName = serializer->GetString();

	// Donnees collectees
	sharedContinuousVector.DeserializeObject(serializer, &classSelectionOperandData->cvContinuousInputData);
	sharedSymbolVector.DeserializeObject(serializer, &classSelectionOperandData->svSymbolInputData);
}

Object* PLShared_ClassSelectionOperandData::Create() const
{
	return new KDClassSelectionOperandData;
}

///////////////////////////////////////////////////////
// Classe PLShared_ClassSelectionObjectRef

PLShared_ClassSelectionObjectRef::PLShared_ClassSelectionObjectRef() {}

PLShared_ClassSelectionObjectRef::~PLShared_ClassSelectionObjectRef() {}

void PLShared_ClassSelectionObjectRef::SetClassSelectionObjectRef(KDClassSelectionObjectRef* classSelectionObjectRef)
{
	require(classSelectionObjectRef != NULL);
	SetObject(classSelectionObjectRef);
}

KDClassSelectionObjectRef* PLShared_ClassSelectionObjectRef::GetClassSelectionObjectRef()
{
	return cast(KDClassSelectionObjectRef*, GetObject());
}

void PLShared_ClassSelectionObjectRef::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KDClassSelectionObjectRef* classSelectionObjectRef;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	classSelectionObjectRef = cast(KDClassSelectionObjectRef*, o);

	// Attributs de l'objets
	serializer->PutLongint(classSelectionObjectRef->lMainObjectIndex);
	serializer->PutLongint(classSelectionObjectRef->lSubObjectIndex);
	serializer->PutDouble(classSelectionObjectRef->dSelectionProb);
	serializer->PutInt(classSelectionObjectRef->nValueIndex);
}

void PLShared_ClassSelectionObjectRef::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KDClassSelectionObjectRef* classSelectionObjectRef;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	classSelectionObjectRef = cast(KDClassSelectionObjectRef*, o);

	// Attributs de l'objets
	classSelectionObjectRef->lMainObjectIndex = serializer->GetLongint();
	classSelectionObjectRef->lSubObjectIndex = serializer->GetLongint();
	classSelectionObjectRef->dSelectionProb = serializer->GetDouble();
	classSelectionObjectRef->nValueIndex = serializer->GetInt();
}

Object* PLShared_ClassSelectionObjectRef::Create() const
{
	return new KDClassSelectionObjectRef;
}
