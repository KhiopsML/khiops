// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWObject.h"

KWObject::KWObject(const KWClass* kwcNew, longint lIndex)
{
	require(kwcNew != NULL);
	require(kwcNew->IsCompiled());
	require(lIndex > 0);

	// Initialisation par defaut
	kwcClass = kwcNew;
	lCreationIndex = lIndex;
	values.attributeValues = NULL;
	debug(nObjectLoadedDataItemNumber = 0);
	debug(nFreshness = 0);

	// Initialisation de l'objet
	Init();
}

KWObject::~KWObject()
{
	DeleteAttributes();
}

void KWObject::DeleteAttributes()
{
	int i;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjectArray;
	KWLoadIndex liLoadIndex;
	KWLoadIndex liInternalLoadIndex;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->IsCompiled());

	// Destruction des valeurs
	if (values.attributeValues != NULL)
	{
		// Dereferencement des valeurs de type Symbol
		// (pour assurer la gestion correcte du compteur de reference des Symbol)
		for (i = 0; i < kwcClass->GetLoadedDenseSymbolAttributeNumber(); i++)
		{
			attribute = kwcClass->GetLoadedDenseSymbolAttributeAt(i);

			// Dereferencement sauf si valeur interdite
			liLoadIndex = attribute->GetLoadIndex();
			assert(liLoadIndex.IsDense());
			if (not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolForbidenValue())
				GetAt(liLoadIndex.GetDenseIndex()).ResetSymbol();
		}

		// Dereferencement des valeurs de type Text
		// (pour assurer la gestion correcte du compteur de reference des Text)
		for (i = 0; i < kwcClass->GetLoadedTextAttributeNumber(); i++)
		{
			attribute = kwcClass->GetLoadedTextAttributeAt(i);

			// Dereferencement sauf si valeur interdite
			liLoadIndex = attribute->GetLoadIndex();
			assert(liLoadIndex.IsDense());
			if (not GetAt(liLoadIndex.GetDenseIndex()).IsTextForbidenValue())
				GetAt(liLoadIndex.GetDenseIndex()).ResetText();
		}

		// Destruction des blocs de valeurs
		for (i = 0; i < kwcClass->GetLoadedAttributeBlockNumber(); i++)
		{
			attributeBlock = kwcClass->GetLoadedAttributeBlockAt(i);

			// Destruction du bloc, sauf si valeur interdite
			liLoadIndex = attributeBlock->GetLoadIndex();
			assert(liLoadIndex.IsDense());
			if (attributeBlock->GetType() == KWType::Continuous)
			{
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue())
					delete GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
			}
			else if (attributeBlock->GetType() == KWType::Symbol)
			{
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue())
					delete GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
			}
			else
			{
				assert(attributeBlock->GetType() == KWType::ObjectArray);
				// Il ne faut pas detruire le contenu des ObjectArray du bloc, qui sont necessairement
				// calcules
				assert(attributeBlock->GetDerivationRule() != NULL);
				assert(attributeBlock->GetDerivationRule()->GetReference());
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue())
					delete GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
			}
		}

		// Destruction des valeurs de type Object ou ObjectArray inclus
		for (i = 0; i < kwcClass->GetLoadedRelationAttributeNumber(); i++)
		{
			attribute = kwcClass->GetLoadedRelationAttributeAt(i);
			liLoadIndex = attribute->GetLoadIndex();
			assert(liLoadIndex.IsDense());

			// Cas des Object
			if (attribute->GetType() == KWType::Object)
			{
				// Si attribut inclu
				if (not attribute->GetReference())
				{
					// Destruction sauf si NULL ou derive mais non calcule
					if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue())
					{
						kwoUsedObject = GetAt(liLoadIndex.GetDenseIndex()).GetObject();
						if (kwoUsedObject != NULL)
							delete kwoUsedObject;
					}
				}
			}
			// Cas des ObjectArray
			else
			{
				assert(attribute->GetType() == KWType::ObjectArray);

				// Destruction sauf si NULL ou derive mais non calcule
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue())
				{
					oaUsedObjectArray = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
					if (oaUsedObjectArray != NULL)
					{
						// Destruction du contenu si multi-inclu
						if (not attribute->GetReference())
							oaUsedObjectArray->DeleteAll();

						// Destruction du container
						// Les containers appartiennent a l'objet, qu'ils soient natifs ou
						// proviennent d'une regle, auquel cas ils ont ete dupliques
						delete oaUsedObjectArray;
					}
				}
			}
		}

		// Destruction des valeurs de type Object ou ObjectArray inclus natifs non charges en memoire
		for (i = 0; i < kwcClass->GetUnloadedNativeRelationAttributeNumber(); i++)
		{
			attribute = kwcClass->GetUnloadedNativeRelationAttributeAt(i);
			liInternalLoadIndex = attribute->GetInternalLoadIndex();
			assert(liInternalLoadIndex.IsDense());

			// Verifications de coherence
			assert(KWType::IsRelation(attribute->GetType()));
			assert(not attribute->IsInBlock());
			assert(attribute->GetDerivationRule() == NULL);
			assert(attribute->GetLoaded() == false);
			assert(not attribute->GetReference());
			assert(not GetAt(liInternalLoadIndex.GetDenseIndex()).IsObjectForbidenValue());

			// Cas des Object
			if (attribute->GetType() == KWType::Object)
			{
				kwoUsedObject = GetAt(liInternalLoadIndex.GetDenseIndex()).GetObject();
				if (kwoUsedObject != NULL)
					delete kwoUsedObject;
			}
			// Cas des ObjectArray
			else
			{
				assert(attribute->GetType() == KWType::ObjectArray);
				oaUsedObjectArray = GetAt(liInternalLoadIndex.GetDenseIndex()).GetObjectArray();
				if (oaUsedObjectArray != NULL)
				{
					// Destruction du contenu multi-inclu et de son container
					oaUsedObjectArray->DeleteAll();
					delete oaUsedObjectArray;
				}
			}
		}

		// Pas de destruction des valeurs de type Structure
		// Destruction du tableau d'attributs de base
		DeleteValueVector(values, kwcClass->GetTotalInternallyLoadedDataItemNumber());
	}
}

void KWObject::Init()
{
	int i;
	int nTotalLoadedDataItemNumber;
	int nTotalInternallyLoadedDataItemNumber;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	// Necessite que la classe soit valide
	require(kwcClass->IsCompiled());

	// Destruction si necessaire des attributs
	if (values.attributeValues != NULL)

	{
		DeleteAttributes();
		values.attributeValues = NULL;
	}

	// Creation du container d'attributs
	nTotalInternallyLoadedDataItemNumber = kwcClass->GetTotalInternallyLoadedDataItemNumber();
	nTotalLoadedDataItemNumber = kwcClass->GetLoadedDataItemNumber();
	if (nTotalInternallyLoadedDataItemNumber > 0)
	{
		// Creation du vecteur de valeur et memorisation de son type de taille
		values = NewValueVector(nTotalInternallyLoadedDataItemNumber);
		SetSmallSize(nTotalInternallyLoadedDataItemNumber <= nBlockSize);

		// Initialisation des valeurs pour les attributs charges en memoire
		// Les autres sont forcement natifs
		for (i = 0; i < nTotalLoadedDataItemNumber; i++)
		{
			dataItem = kwcClass->GetLoadedDataItemAt(i);

			// On verifie l'initialisation de la valeur a 0
			assert(GetAt(i).GetContinuous() == 0);

			// Valeur speciale si attribut derive
			if (dataItem->IsAttribute())
			{
				attribute = cast(KWAttribute*, dataItem);
				assert(attribute->GetLoadIndex().IsDense());
				if (attribute->GetDerivationRule() != NULL)
					GetAt(i).SetTypeForbidenValue(attribute->GetType());
			}
			// Valeur speciale si attribut derive de bloc
			else
			{
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				if (attributeBlock->GetDerivationRule() != NULL)
					GetAt(i).SetTypeForbidenValue(attributeBlock->GetBlockType());
			}
		}
	}

	// Memorisation des informations de coherence
	debug(nObjectLoadedDataItemNumber = kwcClass->GetTotalInternallyLoadedDataItemNumber());
	debug(nFreshness = kwcClass->GetFreshness());
}

void KWObject::ComputeAllValues()
{
	int nAttribute;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjects;
	int nObject;

	require(kwcClass->IsCompiled());
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));

	// Calcul de toutes les valeurs a transferer
	for (nAttribute = kwcClass->GetLoadedDataItemNumber() - 1; nAttribute >= 0; nAttribute--)
	{
		dataItem = kwcClass->GetLoadedDataItemAt(nAttribute);

		// Calcul des attributs derives
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);
			if (attribute->GetDerivationRule() != NULL)
			{
				// Calcul selon le type
				switch (attribute->GetType())
				{
				case KWType::Symbol:
					ComputeSymbolValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Continuous:
					ComputeContinuousValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Date:
					ComputeDateValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Time:
					ComputeTimeValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Timestamp:
					ComputeTimestampValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Text:
					ComputeTextValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Object:
					ComputeObjectValueAt(attribute->GetLoadIndex());
					break;
				case KWType::ObjectArray:
					ComputeObjectArrayValueAt(attribute->GetLoadIndex());
					break;
				case KWType::Structure:
					ComputeStructureValueAt(attribute->GetLoadIndex());
					break;
				}
			}

			// Propagation aux sous-objets de la composition
			if (attribute->GetType() == KWType::Object)

			{
				if (not attribute->GetReference())
				{
					kwoUsedObject = GetObjectValueAt(attribute->GetLoadIndex());
					if (kwoUsedObject != NULL)
						kwoUsedObject->ComputeAllValues();
				}
			}
			// Propagation aux tableaux de sous-objets de la composition
			else if (attribute->GetType() == KWType::ObjectArray)
			{
				if (not attribute->GetReference())
				{
					oaUsedObjects = GetObjectArrayValueAt(attribute->GetLoadIndex());
					if (oaUsedObjects != NULL)
					{
						for (nObject = 0; nObject < oaUsedObjects->GetSize(); nObject++)
						{
							kwoUsedObject = cast(KWObject*, oaUsedObjects->GetAt(nObject));
							if (kwoUsedObject != NULL)
								kwoUsedObject->ComputeAllValues();
						}
					}
				}
			}
		}
		// Calcul des blocks d'attributs derives
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);
			if (attributeBlock->GetDerivationRule() != NULL)
			{
				// Calcul selon le type
				switch (attributeBlock->GetBlockType())
				{
				case KWType::SymbolValueBlock:
					ComputeSymbolValueBlockAt(attributeBlock->GetLoadIndex());
					break;
				case KWType::ContinuousValueBlock:
					ComputeContinuousValueBlockAt(attributeBlock->GetLoadIndex());
					break;
				case KWType::ObjectArrayValueBlock:
					ComputeObjectArrayValueBlockAt(attributeBlock->GetLoadIndex());
					break;
				}
			}
		}
	}
}

boolean KWObject::Check() const
{
	boolean bResult = true;
	KWLoadIndex liLoadIndex;

	assert(kwcClass != NULL);
	assert(kwcClass->GetName().GetLength() >= 0);
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));

	// Coherence de la KWClass
	if (not GetClass()->Check())
	{
		AddError("Incorrect dictionary " + GetClass()->GetName());
		bResult = false;
	}

	// Tests dependant de la coherence avec la KWClass
	if (bResult)
	{
		int i;
		int j;
		KWAttribute* attribute;
		Symbol sValue;
		KWObject* kwoUsedObject;
		ObjectArray* oaUsedObjectArray;

		// Coherence des KWClass des attributs de type Object et ObjectArray, et propagation
		for (i = 0; i < GetClass()->GetLoadedRelationAttributeNumber(); i++)
		{
			attribute = GetClass()->GetLoadedRelationAttributeAt(i);
			liLoadIndex = attribute->GetLoadIndex();
			assert(liLoadIndex.IsDense());

			// Cas des attributs Object
			if (attribute->GetType() == KWType::Object)
			{
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue())
				{
					kwoUsedObject = GetAt(liLoadIndex.GetDenseIndex()).GetObject();

					// Cas d'un attribut inclus: il doit y a voir concordance exacte
					//  entre la classe declaree de l'attribut et celle de l'objet
					// Cas d'un attribut reference: il doit y a voir concordance "approximative"
					//  entre la classe declaree de l'attribut et celle de l'objet, qui si elle
					//  n'est pas identique doit correspondre "physiquement" (cf KWDatabase: classes
					//  logique et physique)
					if (not CheckAttributeObjectClass(attribute, kwoUsedObject))
					{
						AddError("The dictionary of the instance in variable " +
							 attribute->GetName() + " is " +
							 kwoUsedObject->GetClass()->GetName() + " and should be " +
							 attribute->GetClass()->GetName());
						bResult = false;
						break;
					}
				}
			}
			// Cas des attributs ObjectArray
			else
			{
				assert(attribute->GetType() == KWType::ObjectArray);

				if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue())
				{
					oaUsedObjectArray = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
					if (oaUsedObjectArray != NULL)
					{
						// Parcours des objets du tableau
						for (j = 0; j < oaUsedObjectArray->GetSize(); j++)
						{
							kwoUsedObject = cast(KWObject*, oaUsedObjectArray->GetAt(j));
							// Verification similaire au cas des attributs objets inclus ou
							// references
							if (not CheckAttributeObjectClass(attribute, kwoUsedObject))
							{
								AddError(
								    "The dictionary of the instances in variable " +
								    attribute->GetName() + " is " +
								    kwoUsedObject->GetClass()->GetName() +
								    " and should be " +
								    attribute->GetClass()->GetName());
								bResult = false;
								break;
							}

							// Propagation pour les objets inclus
							if (bResult and not attribute->GetReference())
								bResult = kwoUsedObject->Check();
						}
					}
				}

				// Arret si erreurs
				if (not bResult)
					break;
			}
		}
	}

	return bResult;
}

void KWObject::Write(ostream& ost) const
{
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjectArray;
	Object* oUsedStructure;
	KWContinuousValueBlock* continuousValueBlock;
	KWSymbolValueBlock* symbolValueBlock;
	KWObjectArrayValueBlock* objectArrayValueBlock;
	int i;
	int j;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(GetClass()->Check());

	// Impression de l'entete de l'objet
	if (GetClass()->GetRoot())
		ost << GetClass()->GetName() << "\n";
	ost << "{[" << GetCreationIndex() << "]\n";

	// Impression des attributs charges en memoire
	for (i = 0; i < GetClass()->GetLoadedDataItemNumber(); i++)
	{
		dataItem = GetClass()->GetLoadedDataItemAt(i);

		// Cas des attributs
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);

			// Nom de l'attribut
			ost << attribute->GetName() << ": ";

			// Valeur Continuous
			if (attribute->GetType() == KWType::Continuous)
				ost << KWContinuous::ContinuousToString(
					   ComputeContinuousValueAt(attribute->GetLoadIndex()))
				    << "\n";
			// Valeur Symbol
			else if (attribute->GetType() == KWType::Symbol)
				ost << ComputeSymbolValueAt(attribute->GetLoadIndex()) << "\n";
			// Valeur Date
			else if (attribute->GetType() == KWType::Date)
				ost << ComputeDateValueAt(attribute->GetLoadIndex()) << "\n";
			// Valeur Time
			else if (attribute->GetType() == KWType::Time)
				ost << ComputeTimeValueAt(attribute->GetLoadIndex()) << "\n";
			// Valeur Timestamp
			else if (attribute->GetType() == KWType::Timestamp)
				ost << ComputeTimestampValueAt(attribute->GetLoadIndex()) << "\n";
			// Valeur Text
			else if (attribute->GetType() == KWType::Text)
				ost << ComputeTextValueAt(attribute->GetLoadIndex()) << "\n";
			// Valeur Object
			else if (attribute->GetType() == KWType::Object)
			{
				kwoUsedObject = ComputeObjectValueAt(attribute->GetLoadIndex());
				if (kwoUsedObject == NULL)
					ost << "[NULL]\n";
				else
				{
					ost << "[" << kwoUsedObject->GetCreationIndex() << "]\n";
					if (not attribute->GetReference())
						ost << *kwoUsedObject;
				}
			}
			// Valeur ObjectArray
			else if (attribute->GetType() == KWType::ObjectArray)
			{
				oaUsedObjectArray = ComputeObjectArrayValueAt(attribute->GetLoadIndex());
				if (oaUsedObjectArray == NULL)
					ost << "[NULL]\n";
				else
				{
					// Parcours des elements du tableau
					ost << "\n{\n";
					for (j = 0; j < oaUsedObjectArray->GetSize(); j++)
					{
						kwoUsedObject = cast(KWObject*, oaUsedObjectArray->GetAt(j));

						// Test si objet multi inclu ou multi reference
						if (kwoUsedObject == NULL)
							ost << "[NULL]\n";
						else
						{
							ost << "[" << kwoUsedObject->GetCreationIndex() << "] ";
							if (not attribute->GetReference())
								ost << *kwoUsedObject;
							else
								ost << "\n";
						}
					}
					ost << "}\n";
				}
			}
			// Valeur Structure
			else if (attribute->GetType() == KWType::Structure)
			{
				oUsedStructure = ComputeStructureValueAt(attribute->GetLoadIndex());
				if (oUsedStructure == NULL)
					ost << "[NULL]\n";
				else
				{
					ost << "[" << oUsedStructure << "]\n";
					ost << *oUsedStructure;
				}
			}
		}
		// Cas des blocs d'attributs
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);

			// Bloc de valeurs Continuous
			if (attributeBlock->GetType() == KWType::Continuous)
			{
				continuousValueBlock = ComputeContinuousValueBlockAt(attributeBlock->GetLoadIndex());
				if (continuousValueBlock->GetValueNumber() > 0)
				{
					ost << "{\n";
					for (j = 0; j < continuousValueBlock->GetValueNumber(); j++)
					{
						attribute = attributeBlock->GetLoadedAttributeAt(
						    continuousValueBlock->GetAttributeSparseIndexAt(j));
						ost << attribute->GetName() << ": ";
						ost << KWContinuous::ContinuousToString(
							   continuousValueBlock->GetValueAt(j))
						    << "\n";
					}
					ost << "}\n";
				}
			}
			// Bloc de valeurs Symbol
			else if (attributeBlock->GetType() == KWType::Symbol)
			{
				symbolValueBlock = ComputeSymbolValueBlockAt(attributeBlock->GetLoadIndex());
				if (symbolValueBlock->GetValueNumber() > 0)
				{
					ost << "{\n";
					for (j = 0; j < symbolValueBlock->GetValueNumber(); j++)
					{
						attribute = attributeBlock->GetLoadedAttributeAt(
						    symbolValueBlock->GetAttributeSparseIndexAt(j));
						ost << attribute->GetName() << ": ";
						ost << symbolValueBlock->GetValueAt(j) << "\n";
					}
					ost << "}\n";
				}
			}
			// Bloc de valeurs ObjectArray
			else if (attributeBlock->GetType() == KWType::ObjectArray)
			{
				objectArrayValueBlock = ComputeObjectArrayValueBlockAt(attributeBlock->GetLoadIndex());
				if (objectArrayValueBlock->GetValueNumber() > 0)
				{
					ost << "{\n";
					for (j = 0; j < objectArrayValueBlock->GetValueNumber(); j++)
					{
						attribute = attributeBlock->GetLoadedAttributeAt(
						    objectArrayValueBlock->GetAttributeSparseIndexAt(j));
						ost << attribute->GetName() << ": ";

						// Ecriture du tableau d'objet
						oaUsedObjectArray = objectArrayValueBlock->GetValueAt(j);
						if (oaUsedObjectArray == NULL)
							ost << "[NULL]\n";
						else
						{
							// Parcours des elements du tableau
							ost << "\n{\n";
							for (j = 0; j < oaUsedObjectArray->GetSize(); j++)
							{
								kwoUsedObject =
								    cast(KWObject*, oaUsedObjectArray->GetAt(j));

								// Test si objet multi inclu ou multi reference
								if (kwoUsedObject == NULL)
									ost << "[NULL]\n";
								else
								{
									ost << "[" << kwoUsedObject->GetCreationIndex()
									    << "] ";
									if (not attribute->GetReference())
										ost << *kwoUsedObject;
									else
										ost << "\n";
								}
							}
							ost << "}\n";
						}
					}
					ost << "}\n";
				}
			}
		}
	}

	// Impression de la fin de l'objet
	ost << "}\n";
}

longint KWObject::GetUsedMemory() const
{
	longint lUsedMemory;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWLoadIndex liLoadIndex;
	KWLoadIndex liInternalLoadIndex;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjectArray;
	int i;
	int j;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));

	// Initialisation avec la taille de l'objet de base
	lUsedMemory = sizeof(KWObject) + 2 * sizeof(void*);
	lUsedMemory += GetClass()->GetLoadedDataItemNumber() * sizeof(KWType);

	// Parcours de la composition en memoire pour completer la taille
	for (i = 0; i < kwcClass->GetLoadedRelationAttributeNumber(); i++)
	{
		attribute = kwcClass->GetLoadedRelationAttributeAt(i);
		liLoadIndex = attribute->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Valeur Object de la composition
		if (attribute->GetType() == KWType::Object and not attribute->GetReference() and
		    attribute->GetDerivationRule() == NULL)
		{
			kwoUsedObject = GetObjectValueAt(liLoadIndex);
			if (kwoUsedObject != NULL)
				lUsedMemory += kwoUsedObject->GetUsedMemory();
		}
		// Valeur ObjectArray
		else if (attribute->GetType() == KWType::ObjectArray and not attribute->GetReference() and
			 attribute->GetDerivationRule() == NULL)
		{
			oaUsedObjectArray = GetObjectArrayValueAt(liLoadIndex);
			if (oaUsedObjectArray != NULL)
			{
				// Parcours des elements du tableau
				for (j = 0; j < oaUsedObjectArray->GetSize(); j++)
				{
					kwoUsedObject = cast(KWObject*, oaUsedObjectArray->GetAt(j));
					if (kwoUsedObject != NULL)
						lUsedMemory += kwoUsedObject->GetUsedMemory();
				}
			}
		}
	}

	// Parcours des blocs de valeurs
	for (i = 0; i < kwcClass->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcClass->GetLoadedAttributeBlockAt(i);
		liLoadIndex = attributeBlock->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Bloc de valeurs
		if (attributeBlock->GetType() == KWType::Continuous)
			lUsedMemory += GetContinuousValueBlockAt(liLoadIndex)->GetUsedMemory();
		else if (attributeBlock->GetType() == KWType::Symbol)
			lUsedMemory += GetSymbolValueBlockAt(liLoadIndex)->GetUsedMemory();
		// On ne prend pas en compte le contenu de ObjectArray des blocs, ceux-ci etant references
		else if (attributeBlock->GetType() == KWType::ObjectArray)
			lUsedMemory += GetObjectArrayValueBlockAt(liLoadIndex)->GetUsedMemory();
	}

	// Parcours des valeurs de type Object ou ObjectArray inclus natifs non charges en memoire
	for (i = 0; i < kwcClass->GetUnloadedNativeRelationAttributeNumber(); i++)
	{
		attribute = kwcClass->GetUnloadedNativeRelationAttributeAt(i);
		liInternalLoadIndex = attribute->GetInternalLoadIndex();
		assert(liInternalLoadIndex.IsDense());

		// Verifications de coherence
		assert(KWType::IsRelation(attribute->GetType()));
		assert(attribute->GetDerivationRule() == NULL);
		assert(attribute->GetLoaded() == false);
		assert(not attribute->GetReference());
		assert(not GetAt(liInternalLoadIndex.GetDenseIndex()).IsObjectForbidenValue());

		// Cas des Object
		if (attribute->GetType() == KWType::Object)
		{
			kwoUsedObject = GetObjectValueAt(liInternalLoadIndex);
			if (kwoUsedObject != NULL)
				lUsedMemory += kwoUsedObject->GetUsedMemory();
		}
		// Cas des ObjectArray
		else
		{
			assert(attribute->GetType() == KWType::ObjectArray);
			oaUsedObjectArray = GetObjectArrayValueAt(liInternalLoadIndex);
			if (oaUsedObjectArray != NULL)
			{
				// Parcours des elements du tableau
				for (j = 0; j < oaUsedObjectArray->GetSize(); j++)
				{
					kwoUsedObject = cast(KWObject*, oaUsedObjectArray->GetAt(j));
					if (kwoUsedObject != NULL)
						lUsedMemory += kwoUsedObject->GetUsedMemory();
				}
			}
		}
	}
	return lUsedMemory;
}

const ALString KWObject::GetClassLabel() const
{
	return "Instance";
}

const ALString KWObject::GetObjectLabel() const
{
	char sPointer[20];
	KWObjectKey objectKey;
	ALString sObjectLabel;

	// On se base sur la cle de l'objet s'il y en a une
	if (GetClass()->IsKeyLoaded())
	{
		objectKey.InitializeFromObject(this);
		sObjectLabel = objectKey.GetObjectLabel();
	}
	// Sinon, sur prend son adresse memoire
	else
	{
		sprintf(sPointer, "[%p]", this);
		sObjectLabel = sPointer;
	}
	return GetClass()->GetName() + " " + sObjectLabel;
}

KWObject* KWObject::CreateObject(KWClass* refClass, longint lObjectIndex)
{
	const ALString sSymbolPrefix = "S";
	const ALString sTextPrefix = "Text";
	KWObject* kwoObject;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int i;
	longint lValue;
	longint lStartValue;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjectArray;
	KWContinuousValueBlock* cvbValue;
	KWSymbolValueBlock* svbValue;
	ALString sBlockField;
	int j;
	boolean bOk;
	ALString sMessage;

	require(refClass != NULL);
	require(refClass->IsCompiled());
	require(lObjectIndex > 0);

	// Creation de l'objet
	kwoObject = new KWObject(refClass, lObjectIndex);

	// Calcul de la valeur de depart en fonction de l'index de l'objet et du nombre de valeurs
	lStartValue = 1;
	while (lStartValue < kwoObject->GetClass()->GetLoadedAttributeNumber())
		lStartValue *= 10;
	lStartValue *= lObjectIndex;

	// Alimentation de valeurs des attributs denses
	lValue = lStartValue + 1;
	for (i = 0; i < kwoObject->GetClass()->GetLoadedDenseAttributeNumber(); i++)
	{
		attribute = kwoObject->GetClass()->GetLoadedDenseAttributeAt(i);

		// Alimentation si attribut non derive
		if (attribute->GetDerivationRule() == NULL)
		{
			// Valeur Continuous
			if (attribute->GetType() == KWType::Continuous)
				kwoObject->SetContinuousValueAt(attribute->GetLoadIndex(), (Continuous)lValue);
			// Valeur Symbol
			else if (attribute->GetType() == KWType::Symbol)
				kwoObject->SetSymbolValueAt(attribute->GetLoadIndex(),
							    Symbol(sSymbolPrefix + LongintToString(lValue)));
			// Valeur Text
			else if (attribute->GetType() == KWType::Text)
				kwoObject->SetTextValueAt(attribute->GetLoadIndex(),
							  Symbol(sTextPrefix + LongintToString(lValue)));
			// Valeur Object
			else if (attribute->GetType() == KWType::Object)
			{
				// Ajout d'un sous objet dans le cas inclus
				if (not attribute->GetReference())
				{
					kwoUsedObject = CreateObject(attribute->GetClass(), lObjectIndex);
					kwoObject->SetObjectValueAt(attribute->GetLoadIndex(), kwoUsedObject);
				}
			}
			// Valeur ObjectArray
			else if (attribute->GetType() == KWType::ObjectArray)
			{
				// Ajout de deux objets dans le cas multi-inclus
				if (not attribute->GetReference())
				{
					oaUsedObjectArray = new ObjectArray;
					oaUsedObjectArray->Add(
					    CreateObject(attribute->GetClass(), 2 * lObjectIndex - 1));
					oaUsedObjectArray->Add(CreateObject(attribute->GetClass(), 2 * lObjectIndex));
					kwoObject->SetObjectArrayValueAt(attribute->GetLoadIndex(), oaUsedObjectArray);
				}
			}
			// Valeur Structure
			else if (attribute->GetType() == KWType::Structure)
			{
				kwoObject->SetStructureValueAt(attribute->GetLoadIndex(), NULL);
			}
		}
		lValue++;
	}

	// Alimentation des blocs, avec une valeur sur 2, 4, 8...
	for (i = 0; i < kwoObject->GetClass()->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwoObject->GetClass()->GetLoadedAttributeBlockAt(i);

		// Alimentation si bloc d'attributs non derive
		if (attributeBlock->GetDerivationRule() == NULL)
		{
			// Bloc de valeurs Continuous
			if (attributeBlock->GetType() == KWType::Continuous)
			{
				// Specification des valeurs du bloc sous forme chaine de caracteres
				sBlockField = "";
				for (j = 0; j < attributeBlock->GetLoadedAttributeNumber(); j++)
				{
					if ((lObjectIndex + j) % (2 << j) != 0)
						continue;
					attribute = attributeBlock->GetLoadedAttributeAt(j);
					if (sBlockField.GetLength() > 0)
						sBlockField += ' ';
					sBlockField += attribute->GetMetaData()->GetStringValueAt(
					    KWAttributeBlock::GetAttributeKeyMetaDataKey());
					sBlockField += ':';
					sBlockField += LongintToString(lValue);
					lValue++;
				}

				// Creation d'un bloc de valeurs
				cvbValue = KWContinuousValueBlock::BuildBlockFromField(
				    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sBlockField,
				    attributeBlock->GetContinuousDefaultValue(), bOk, sMessage);
				if (not bOk)
					attributeBlock->AddError(sMessage);
				kwoObject->SetContinuousValueBlockAt(attributeBlock->GetLoadIndex(), cvbValue);
			}
			// Bloc de valeurs Symbol
			else if (attributeBlock->GetType() == KWType::Symbol)
			{
				// Specification des valeurs du bloc sous forme chaine de caracteres
				sBlockField = "";
				for (j = 0; j < attributeBlock->GetLoadedAttributeNumber(); j++)
				{
					if ((lObjectIndex + j) % (2 << j) != 0)
						continue;
					attribute = attributeBlock->GetLoadedAttributeAt(j);
					if (sBlockField.GetLength() > 0)
						sBlockField += ' ';
					sBlockField += attribute->GetMetaData()->GetStringValueAt(
					    KWAttributeBlock::GetAttributeKeyMetaDataKey());
					sBlockField += ':';
					sBlockField += Symbol(sSymbolPrefix + LongintToString(lValue));
					lValue++;
				}

				// Creation d'un bloc de valeurs
				svbValue = KWSymbolValueBlock::BuildBlockFromField(
				    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sBlockField,
				    attributeBlock->GetSymbolDefaultValue(), bOk, sMessage);
				if (not bOk)
					attributeBlock->AddError(sMessage);
				kwoObject->SetSymbolValueBlockAt(attributeBlock->GetLoadIndex(), svbValue);
			}
		}
	}
	ensure(kwoObject->Check());
	return kwoObject;
}

void KWObject::Test()
{
	KWClass* testClass;
	KWClass* attributeClass;
	KWObject* kwoObject;

	// Creation d'une classe de test
	attributeClass = KWClass::CreateClass("AttributeClass", 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, false, NULL);
	testClass = KWClass::CreateClass("TestClass", 1, 2, 2, 1, 1, 1, 1, 2, 2, 0, false, attributeClass);
	testClass->SetRoot(true);
	KWClassDomain::GetCurrentDomain()->InsertClass(attributeClass);
	KWClassDomain::GetCurrentDomain()->InsertClass(testClass);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Creation d'un objet
	kwoObject = CreateObject(testClass, 1);

	// Tests
	kwoObject->AddMessage("Tests on an instance");
	kwoObject->Check();
	cout << *kwoObject << endl;
	delete kwoObject;

	// Destruction de toutes les classes enregistrees
	KWClassDomain::DeleteAllDomains();
}

void KWObject::Mutate(const KWClass* kwcNewClass, const NumericKeyDictionary* nkdUnusedNativeAttributesToKeep)
{
	const KWClass* previousClass;
	const KWAttribute* previousAttribute;
	ObjectValues previousValues;
	boolean bPreviousSmallSize;
	int i;
	int nLoadedDataItemNumber;
	int nTotalInternallyLoadedDataItemNumber;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* newAttributeBlock;
	KWContinuousValueBlock* continuousValueBlock;
	KWContinuousValueBlock* newContinuousValueBlock;
	KWSymbolValueBlock* symbolValueBlock;
	KWSymbolValueBlock* newSymbolValueBlock;
	KWObjectArrayValueBlock* objectArrayValueBlock;
	KWObjectArrayValueBlock* newObjectArrayValueBlock;
	KWObject* kwoUsedObject;
	ObjectArray* oaUsedObjectArray;
	int nObject;
	KWClass* kwcMutationClass;
	KWLoadIndex liLoadIndex;
	KWLoadIndex liInternalLoadIndex;
	int nNumber;
	int nNewNumber;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(GetClass()->IsCompiled());
	require(kwcNewClass != NULL);
	require(kwcNewClass->IsCompiled());
	require(kwcClass->GetName() == kwcNewClass->GetName());
	require(kwcNewClass->GetLoadedAttributeNumber() <= kwcClass->GetLoadedAttributeNumber());
	require(kwcNewClass->GetLoadedDataItemNumber() <= kwcClass->GetLoadedDataItemNumber());
	require(nkdUnusedNativeAttributesToKeep != NULL);

	// Aucune action si meme classe
	if (kwcNewClass == kwcClass)
		return;

	// Dereferencement des valeurs de type Symbol
	// (pour assurer la gestion correcte du compteur de references des Symbol)
	nNumber = kwcClass->GetLoadedDenseSymbolAttributeNumber();
	nNewNumber = kwcNewClass->GetLoadedDenseSymbolAttributeNumber();
	for (i = nNewNumber; i < nNumber; i++)
	{
		attribute = kwcClass->GetLoadedDenseSymbolAttributeAt(i);

		// Si attribut non garde
		liLoadIndex = attribute->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Dereferencement sauf si valeur interdite
		if (not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolForbidenValue())
			GetAt(liLoadIndex.GetDenseIndex()).ResetSymbol();
	}

	// Dereferencement des valeurs de type Text
	// (pour assurer la gestion correcte du compteur de references des Text)
	nNumber = kwcClass->GetLoadedTextAttributeNumber();
	nNewNumber = kwcNewClass->GetLoadedTextAttributeNumber();
	for (i = nNewNumber; i < nNumber; i++)
	{
		attribute = kwcClass->GetLoadedTextAttributeAt(i);

		// Si attribut non garde
		liLoadIndex = attribute->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Dereferencement sauf si valeur interdite
		if (not GetAt(liLoadIndex.GetDenseIndex()).IsTextForbidenValue())
			GetAt(liLoadIndex.GetDenseIndex()).ResetText();
	}

	// Destruction des valeurs de type Object ou ObjectArray inclus non gardees
	nNumber = kwcClass->GetLoadedRelationAttributeNumber();
	nNewNumber = kwcNewClass->GetLoadedRelationAttributeNumber();
	for (i = nNewNumber; i < nNumber; i++)
	{
		attribute = kwcClass->GetLoadedRelationAttributeAt(i);
		liLoadIndex = attribute->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Cas des Object
		if (attribute->GetType() == KWType::Object)
		{
			// Si attribut inclu et non garde
			if (not attribute->GetReference())
			{
				// Destruction uniquement si attribut calcule
				// Si attribut natif: on le gardera en InternalLoad dans la nouvelle classe,
				// car l'objet est potentiellement reference par ailleur
				if (attribute->GetDerivationRule() != NULL)
				{
					assert(liLoadIndex.GetDenseIndex() >= kwcNewClass->GetLoadedDataItemNumber());

					// Destruction sauf si NULL ou derive mais non calcule
					if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue())
					{
						kwoUsedObject = GetAt(liLoadIndex.GetDenseIndex()).GetObject();
						if (kwoUsedObject != NULL)
							delete kwoUsedObject;
					}
				}
			}
		}
		// Cas des ObjectArray
		else
		{
			assert(attribute->GetType() == KWType::ObjectArray);

			// Destruction uniquement si attribut calcule
			// Si attribut natif: on le gardera en InternalLoad dans la nouvelle classe,
			// car l'objet est potentiellement reference par ailleur
			if (attribute->GetDerivationRule() != NULL)
			{
				assert(liLoadIndex.GetDenseIndex() >= kwcNewClass->GetLoadedDataItemNumber());

				// Destruction sauf si NULL ou derive mais non calcule
				if (not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue())
				{
					oaUsedObjectArray = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
					if (oaUsedObjectArray != NULL)
					{
						// Destruction du contenu si multi-inclu
						if (not attribute->GetReference())
							oaUsedObjectArray->DeleteAll();

						// Destruction du container
						// Les containers appartiennent a l'objet, qu'ils soient natifs ou
						// proviennent d'une regle, auquel cas ils ont ete dupliques
						delete oaUsedObjectArray;
					}
				}
			}
		}
	}

	// Pas de destruction des valeurs de type Structure non gardees

	// Destruction des blocs de valeurs non gardees
	nNumber = kwcClass->GetLoadedAttributeBlockNumber();
	nNewNumber = kwcNewClass->GetLoadedAttributeBlockNumber();
	for (i = nNewNumber; i < nNumber; i++)
	{
		attributeBlock = kwcClass->GetLoadedAttributeBlockAt(i);
		liLoadIndex = attributeBlock->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Destruction du bloc non garde
		if (attributeBlock->GetType() == KWType::Continuous)
		{
			continuousValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
			check(continuousValueBlock);
			delete continuousValueBlock;
		}
		else if (attributeBlock->GetType() == KWType::Symbol)
		{
			symbolValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
			check(symbolValueBlock);
			delete symbolValueBlock;
		}
		else if (attributeBlock->GetType() == KWType::ObjectArray)
		{
			objectArrayValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
			check(objectArrayValueBlock);
			// Il ne faut pas detruire le contenu des ObjectArray du bloc, qui sont necessairement calcules
			assert(attributeBlock->GetDerivationRule() != NULL);
			assert(attributeBlock->GetDerivationRule()->GetReference());
			delete objectArrayValueBlock;
		}
	}

	// Nettoyage des blocs de valeurs gardes, potentiellement partiellement
	for (i = 0; i < kwcNewClass->GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcClass->GetLoadedAttributeBlockAt(i);
		liLoadIndex = attributeBlock->GetLoadIndex();
		assert(liLoadIndex.IsDense());

		// Acces au bloc dans la nouvelle classe
		newAttributeBlock = kwcNewClass->GetLoadedAttributeBlockAt(i);
		assert(liLoadIndex == newAttributeBlock->GetLoadIndex());
		assert(newAttributeBlock->GetLoadedAttributeNumber() <= attributeBlock->GetLoadedAttributeNumber());

		// Nettoyage du bloc si necessaire
		// Il faut eventuellement supprimer les variables surnumeraires, qui n'avaient ete chargees
		// dans la classe physique que pour permettre le calcul des attributs derives
		if (newAttributeBlock->GetLoadedAttributeNumber() < attributeBlock->GetLoadedAttributeNumber())
		{
			assert(attributeBlock->GetLoadedAttributeMutationIndexes() != NULL);

			if (attributeBlock->GetType() == KWType::Continuous)
			{
				// Acces au bloc courant
				continuousValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
				check(continuousValueBlock);

				// Remplacement par un bloc extrait pour les nouveaux index
				newContinuousValueBlock = KWContinuousValueBlock::ExtractBlockSubset(
				    continuousValueBlock, attributeBlock->GetLoadedAttributeMutationIndexes());
				GetAt(liLoadIndex.GetDenseIndex()).SetContinuousValueBlock(newContinuousValueBlock);
				delete continuousValueBlock;
			}
			else if (attributeBlock->GetType() == KWType::Symbol)
			{
				// Acces au bloc courant
				symbolValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
				check(symbolValueBlock);

				// Remplacement par un bloc extrait pour les nouveaux index
				newSymbolValueBlock = KWSymbolValueBlock::ExtractBlockSubset(
				    symbolValueBlock, attributeBlock->GetLoadedAttributeMutationIndexes());
				GetAt(liLoadIndex.GetDenseIndex()).SetSymbolValueBlock(newSymbolValueBlock);
				delete symbolValueBlock;
			}
			else if (attributeBlock->GetType() == KWType::ObjectArray)
			{
				// Acces au bloc courant
				objectArrayValueBlock = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
				check(objectArrayValueBlock);

				// Remplacement par un bloc extrait pour les nouveaux index
				newObjectArrayValueBlock = KWObjectArrayValueBlock::ExtractBlockSubset(
				    objectArrayValueBlock, attributeBlock->GetLoadedAttributeMutationIndexes());
				GetAt(liLoadIndex.GetDenseIndex()).SetObjectArrayValueBlock(newObjectArrayValueBlock);
				// Il ne faut pas detruire le contenu des ObjectArray du bloc, qui sont necessairement
				// calcules
				assert(attributeBlock->GetDerivationRule() != NULL);
				assert(attributeBlock->GetDerivationRule()->GetReference());
				delete objectArrayValueBlock;
			}
		}
	}

	// Memorisation des anciennes valeurs
	previousClass = kwcClass;
	previousValues = values;
	bPreviousSmallSize = (previousClass->GetTotalInternallyLoadedDataItemNumber() <= nBlockSize);

	// Initialisation
	kwcClass = kwcNewClass;
	values.attributeValues = NULL;
	debug(nObjectLoadedDataItemNumber = 0);
	debug(nFreshness = 0);

	// Creation du container d'attributs
	nTotalInternallyLoadedDataItemNumber = kwcClass->GetTotalInternallyLoadedDataItemNumber();
	nLoadedDataItemNumber = kwcClass->GetLoadedDataItemNumber();
	if (nTotalInternallyLoadedDataItemNumber > 0)
	{
		// Creation du vecteur de valeur et memorisation de son type de taille
		values = NewValueVector(nTotalInternallyLoadedDataItemNumber);
		SetSmallSize(nTotalInternallyLoadedDataItemNumber <= nBlockSize);

		// Transfert des valeurs chargees en memoire
		for (i = 0; i < nLoadedDataItemNumber; i++)
			GetAt(i) = GetValueAt(previousValues, bPreviousSmallSize, i);

		// Transfert et mutation des Object et ObjectArray inclus natifs non charges en memoire
		for (i = 0; i < kwcClass->GetUnloadedNativeRelationAttributeNumber(); i++)
		{
			attribute = kwcClass->GetUnloadedNativeRelationAttributeAt(i);
			liInternalLoadIndex = attribute->GetInternalLoadIndex();
			assert(liInternalLoadIndex.IsDense());

			// Verifications de coherence
			assert(KWType::IsRelation(attribute->GetType()));
			assert(attribute->GetDerivationRule() == NULL);
			assert(attribute->GetLoaded() == false);
			assert(not attribute->GetReference());
			assert(not GetAt(liInternalLoadIndex.GetDenseIndex()).IsObjectForbidenValue());

			// Traitement si attribut precedent present dans la classe d'origine
			previousAttribute = previousClass->LookupAttribute(attribute->GetName());
			if (previousAttribute != NULL)
			{
				liLoadIndex = previousAttribute->GetLoadIndex();
				assert(liLoadIndex.IsDense());

				// Cas des Object
				if (attribute->GetType() == KWType::Object)
				{
					// Acces a l'objet precedent
					kwoUsedObject =
					    GetValueAt(previousValues, bPreviousSmallSize, liLoadIndex.GetDenseIndex())
						.GetObject();

					// Transfert et mutation de l'objet si necessaire, destruction sinon
					if (kwoUsedObject != NULL)
					{
						// Test de coherence
						assert(kwoUsedObject->GetClass()->GetName() ==
						       attribute->GetClass()->GetName());
						assert(kwoUsedObject->GetClass()->GetDomain() ==
						       previousClass->GetDomain());
						assert(kwoUsedObject->GetClass() != attribute->GetClass());

						// Transfert si objet a garder
						if (nkdUnusedNativeAttributesToKeep->Lookup((NUMERIC)attribute) ==
						    attribute)
						{
							GetAt(liInternalLoadIndex.GetDenseIndex())
							    .SetObject(kwoUsedObject);
							kwoUsedObject->Mutate(attribute->GetClass(),
									      nkdUnusedNativeAttributesToKeep);
						}
						// Destruction sinon
						else
							delete kwoUsedObject;
					}
				}
				// Cas des ObjectArray
				else
				{
					assert(attribute->GetType() == KWType::ObjectArray);

					// Acces au tableau precedent
					oaUsedObjectArray =
					    GetValueAt(previousValues, bPreviousSmallSize, liLoadIndex.GetDenseIndex())
						.GetObjectArray();

					// Transfert et mutation de l'objet si necessaire, destruction sinon
					if (oaUsedObjectArray != NULL)
					{
						// Transfert si objet a garder
						if (nkdUnusedNativeAttributesToKeep->Lookup((NUMERIC)attribute) ==
						    attribute)
						{
							// Transfert du tableau
							GetAt(liInternalLoadIndex.GetDenseIndex())
							    .SetObjectArray(oaUsedObjectArray);

							// Parcours des objets du tableau pour mutation eventuelle
							for (nObject = 0; nObject < oaUsedObjectArray->GetSize();
							     nObject++)
							{
								kwoUsedObject =
								    cast(KWObject*, oaUsedObjectArray->GetAt(nObject));

								// Test de coherence
								assert(kwoUsedObject->GetClass()->GetName() ==
								       attribute->GetClass()->GetName());
								assert(kwoUsedObject->GetClass()->GetDomain() ==
								       previousClass->GetDomain());
								assert(kwoUsedObject->GetClass() !=
								       attribute->GetClass());

								// Mutation
								kwoUsedObject->Mutate(attribute->GetClass(),
										      nkdUnusedNativeAttributesToKeep);
							}
						}
						// Destruction sinon
						else
						{
							oaUsedObjectArray->DeleteAll();
							delete oaUsedObjectArray;
						}
					}
				}
			}
		}

		// Mutation des attributs Object ou ObjetArray transferres si necessaire
		for (i = 0; i < kwcClass->GetLoadedRelationAttributeNumber(); i++)
		{
			attribute = kwcClass->GetLoadedRelationAttributeAt(i);
			liLoadIndex = attribute->GetLoadIndex();
			assert(liLoadIndex.IsDense());

			// Cas des Object
			if (attribute->GetType() == KWType::Object)
			{
				// Si attribut inclu: mutation
				if (not attribute->GetReference() and
				    not GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue())
				{
					kwoUsedObject = GetAt(liLoadIndex.GetDenseIndex()).GetObject();
					if (kwoUsedObject != NULL and
					    kwoUsedObject->GetClass()->GetDomain() != kwcClass->GetDomain())
					{
						// Recherche de la classe destination
						kwcMutationClass = kwcClass->GetDomain()->LookupClass(
						    kwoUsedObject->GetClass()->GetName());

						// Si non trouve, supression
						if (kwcMutationClass == NULL)
						{
							delete kwoUsedObject;
							GetAt(liLoadIndex.GetDenseIndex()).SetObject(NULL);
						}
						// Sinon, mutation
						else
							kwoUsedObject->Mutate(kwcMutationClass,
									      nkdUnusedNativeAttributesToKeep);
					}
				}
			}
			// Cas des ObjectArray
			else
			{
				assert(attribute->GetType() == KWType::ObjectArray);

				// Si attribut multi-inclu: mutation
				if (not attribute->GetReference() and
				    not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue())
				{
					oaUsedObjectArray = GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
					if (oaUsedObjectArray != NULL)
					{
						// Parcours des objets du tableau pour mutation eventuelle
						kwcMutationClass = NULL;
						for (nObject = 0; nObject < oaUsedObjectArray->GetSize(); nObject++)
						{
							kwoUsedObject =
							    cast(KWObject*, oaUsedObjectArray->GetAt(nObject));
							if (kwoUsedObject != NULL and
							    kwoUsedObject->GetClass()->GetDomain() !=
								kwcClass->GetDomain())
							{
								// Recherche de la classe destination
								if (kwcMutationClass == NULL)
									kwcMutationClass =
									    kwcClass->GetDomain()->LookupClass(
										kwoUsedObject->GetClass()->GetName());

								// Si non trouve, supression
								if (kwcMutationClass == NULL)
								{
									delete kwoUsedObject;
									oaUsedObjectArray->SetAt(nObject, NULL);
								}
								// Sinon, mutation
								else
									kwoUsedObject->Mutate(
									    kwcMutationClass,
									    nkdUnusedNativeAttributesToKeep);
							}
						}
					}
				}
			}
		}
	}

	// Destruction des valeurs precedentes, geree par la destruction de l'objet precedent memorise
	if (previousValues.attributeValues != NULL)
		DeleteValueVector(previousValues, previousClass->GetTotalInternallyLoadedDataItemNumber());

	// Memorisation des informations de coherence
	debug(nObjectLoadedDataItemNumber = kwcClass->GetTotalInternallyLoadedDataItemNumber());
	debug(nFreshness = kwcClass->GetFreshness());
}

KWObject::ObjectValues KWObject::NewValueVector(int nSize)
{
	ObjectValues newValues;
	int nBlockNumber;
	int i;

	require(nSize > 0);

	// Cas mono-block
	if (nSize <= nBlockSize)
	{
		newValues.attributeValues = (KWValue*)NewMemoryBlock(nSize * sizeof(KWValue));

		// Initialisation des valeurs a 0 (compatible avec KWType)
		memset(newValues.attributeValues, 0, nSize * sizeof(KWValue));
	}
	// Cas multi-block
	else
	{
		nBlockNumber = (nSize - 1) / nBlockSize + 1;
		newValues.attributeValueArrays = (KWValue**)NewMemoryBlock(nBlockNumber * sizeof(KWValue*));
		for (i = nBlockNumber - 2; i >= 0; i--)
		{
			newValues.attributeValueArrays[i] = (KWValue*)NewMemoryBlock(nBlockSize * sizeof(KWValue));
			memset(newValues.attributeValueArrays[i], 0, nBlockSize * sizeof(KWValue));
		}
		if (nSize % nBlockSize > 0)
		{
			newValues.attributeValueArrays[nBlockNumber - 1] =
			    (KWValue*)NewMemoryBlock((nSize % nBlockSize) * sizeof(KWValue));
			memset(newValues.attributeValueArrays[nBlockNumber - 1], 0,
			       (nSize % nBlockSize) * sizeof(KWValue));
		}
		else
		{
			newValues.attributeValueArrays[nBlockNumber - 1] =
			    (KWValue*)NewMemoryBlock(nBlockSize * sizeof(KWValue));
			memset(newValues.attributeValueArrays[nBlockNumber - 1], 0, nBlockSize * sizeof(KWValue));
		}
	}

	ensure(newValues.attributeValues != NULL);
	return newValues;
}

void KWObject::DeleteValueVector(ObjectValues valuesToDelete, int nSize)
{
	int i;
	int nBlockNumber;

	require(valuesToDelete.attributeValues != NULL);
	require(nSize > 0);

	// Desallocation
	if (nSize <= nBlockSize)
	{
		DeleteMemoryBlock(valuesToDelete.attributeValues);
	}
	else
	{
		nBlockNumber = (nSize - 1) / nBlockSize + 1;
		for (i = nBlockNumber - 1; i >= 0; i--)
			DeleteMemoryBlock(valuesToDelete.attributeValueArrays[i]);
		DeleteMemoryBlock(valuesToDelete.attributeValueArrays);
	}
}

int KWObjectCompareCreationIndex(const void* elem1, const void* elem2)
{
	int nResult;
	KWObject* object1;
	KWObject* object2;

	// Acces aux partitions
	object1 = cast(KWObject*, *(Object**)elem1);
	object2 = cast(KWObject*, *(Object**)elem2);

	// Comparaison
	nResult = CompareLongint(object1->GetCreationIndex(), object2->GetCreationIndex());
	return nResult;
}
