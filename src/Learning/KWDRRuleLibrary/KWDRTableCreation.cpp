// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTableCreation.h"

void KWDRRegisterTableCreationRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRProtoBuildTableView);
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRelationCreationRule

KWDRRelationCreationRule::KWDRRelationCreationRule()
{
	kwcCompiledTargetClass = NULL;
}

KWDRRelationCreationRule::~KWDRRelationCreationRule() {}
boolean KWDRRelationCreationRule::GetReference() const
{
	require(KWType::IsRelation(GetType()));
	return false;
}

boolean KWDRRelationCreationRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWClass* kwcSourceClass;
	KWClass* kwcTargetClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification du premier operande dans le cas d'une alimenattion de type vue
	if (bOk and IsFirstOperandViewType())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche des classes source et cible
		kwcSourceClass = kwcOwnerClass->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
		kwcTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
		assert(kwcSourceClass != NULL);
		assert(kwcTargetClass != NULL);

		// Parcours des attributs natif de la classe cible
		targetAttribute = kwcTargetClass->GetHeadAttribute();
		while (targetAttribute != NULL)
		{
			// On ne traite que les attributs natifs
			if (targetAttribute->GetAnyDerivationRule() == NULL)
			{
				assert(KWType::IsData(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());

				// Erreur si pas d'attribut correspondant trouve
				if (sourceAttribute == NULL)
				{
					AddError("In output dictionary " + kwcTargetClass->GetName() +
						 ", no variable found for secondary variable " +
						 targetAttribute->GetName() + " in input dictionary " +
						 kwcSourceClass->GetName() + " of first operand");
					bOk = false;
				}
				// Erreur si le type trouve est incorrect
				else if (sourceAttribute->GetType() != targetAttribute->GetType())
				{
					AddError("In output dictionary " + kwcTargetClass->GetName() +
						 ", variable found with different type (" +
						 KWType::ToString(targetAttribute->GetType()) + " versus " +
						 KWType::ToString(sourceAttribute->GetType()) +
						 ") for secondary variable " + targetAttribute->GetName() +
						 " in input dictionary " + kwcSourceClass->GetName() +
						 " of first operand");
					bOk = false;
				}

				// Erreur si le type de relation est incorrect
				if (bOk and KWType::IsRelation(sourceAttribute->GetType()) and
				    sourceAttribute->GetClass()->GetName() != targetAttribute->GetClass()->GetName())
				{
					AddError("In output dictionary " + kwcTargetClass->GetName() +
						 ", variable found with different type of " +
						 KWType::ToString(sourceAttribute->GetType()) + " (" +
						 targetAttribute->GetClass()->GetName() + " versus " +
						 sourceAttribute->GetClass()->GetName() + ") for secondary variable " +
						 targetAttribute->GetName() + " in input dictionary " +
						 kwcSourceClass->GetName() + " of first operand");
					bOk = false;
				}

				// Erreur dans le cas d'un bloc si bloc different ou si VarKey different
				if (bOk and (sourceAttribute->IsInBlock() or targetAttribute->IsInBlock()))
				{
					//DDD Pour l'instant, on interdit les attributs cibles dans un bloc
					if (targetAttribute->IsInBlock())
					{
						AddError("In output dictionary " + kwcTargetClass->GetName() +
							 ", variable " + targetAttribute->GetName() + " in block " +
							 targetAttribute->GetAttributeBlock()->GetName() +
							 " not allowed");
						bOk = false;
					}
					//DDD EN COURS
					// Erreur si nom de bloc ifferent

					// Erreur si type de VarKey different

					// Erreur si VarKey different
				}

				// Arret si erreur
				if (not bOk)
					break;
			}

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(targetAttribute);
		}
	}
	return bOk;
}

void KWDRRelationCreationRule::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;
	KWClass* kwcSourceClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	int nAttribute;

	require(kwcOwnerClass != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Compilation dans le cas d'un premier operande utilise pour une alimentation de type vue
	if (IsFirstOperandViewType())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche et memorisation de la classe source
		kwcCompiledTargetClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());
		assert(kwcCompiledTargetClass != NULL);

		// Recherche de la classe source
		kwcSourceClass = kwcOwnerClass->GetDomain()->LookupClass(GetFirstOperand()->GetObjectClassName());
		assert(kwcSourceClass != NULL);

		// Trace initiale
		if (bTrace)
		{
			cout << "Compile rule " << GetName() << "\n";
			cout << "  Owner dictionary\t" << kwcOwnerClass->GetName() << "\t"
			     << kwcOwnerClass->GetDomain()->GetName() << "\t" << kwcOwnerClass->GetCompileFreshness()
			     << "\t" << kwcOwnerClass << "\n";
			cout << "  Source dictionary\t" << kwcSourceClass->GetName() << "\t"
			     << kwcSourceClass->GetDomain()->GetName() << "\t" << kwcSourceClass->GetCompileFreshness()
			     << "\t" << kwcSourceClass << "\n";
			cout << "  Target dictionary\t" << kwcCompiledTargetClass->GetName() << "\t"
			     << kwcCompiledTargetClass->GetDomain()->GetName() << "\t"
			     << kwcCompiledTargetClass->GetCompileFreshness() << "\t" << kwcCompiledTargetClass << "\n";
			cout << "\tVariable\tType\tSource index\tTarget index\n";
		}

		// Reinitialisation des resultats de compilation
		livSourceAttributeLoadIndexes.SetSize(0);
		livTargetAttributeLoadIndexes.SetSize(0);
		ivTargetAttributeTypes.SetSize(0);

		// Parcours des attributs natifs de la classe cible charges en memoire
		for (nAttribute = 0; nAttribute < kwcCompiledTargetClass->GetLoadedDenseAttributeNumber(); nAttribute++)
		{
			targetAttribute = kwcCompiledTargetClass->GetLoadedDenseAttributeAt(nAttribute);

			// On ne traite que les attributs natifs utilises
			if (targetAttribute->GetDerivationRule() == NULL)
			{
				assert(KWType::IsStored(targetAttribute->GetType()) or
				       KWType::IsRelation(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());
				assert(sourceAttribute != NULL);
				assert(sourceAttribute->GetType() == targetAttribute->GetType());

				// Memorisation des infos de chargement si l'attribut source est charge
				// Il peut ne pas etre charge dans le dictionnaire "logique", mais il sera
				// de toute facon charge dans le dictionnaire "physique"
				// Cf. gestion des dictionnaire logiques et physiques dans KWDatabase
				if (sourceAttribute->GetLoaded())
				{
					livSourceAttributeLoadIndexes.Add(sourceAttribute->GetLoadIndex());
					livTargetAttributeLoadIndexes.Add(targetAttribute->GetLoadIndex());
					ivTargetAttributeTypes.Add(targetAttribute->GetType());

					// Trace par attribut gere par une alimentation de type vue
					if (bTrace)
					{
						cout << "\t" << sourceAttribute->GetName();
						cout << "\t" << KWType::ToString(sourceAttribute->GetType());
						cout << "\t" << sourceAttribute->GetLoadIndex();
						cout << "\t" << targetAttribute->GetLoadIndex();
						cout << "\n";
					}
				}
			}
		}
	}
}

void KWDRRelationCreationRule::BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
						      NumericKeyDictionary* nkdAllUsedAttributes) const
{
	KWClass* kwcSourceClass;
	KWClass* kwcTargetClass;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;
	KWDerivationRule* sourceAttributeRule;

	require(derivedAttribute != NULL);
	require(KWType::IsRelation(derivedAttribute->GetType()));
	require(derivedAttribute->GetClass()->GetName() == GetObjectClassName());

	// Appel de la methode ancetre
	KWDerivationRule::BuildAllUsedAttributes(derivedAttribute, nkdAllUsedAttributes);

	// Verification du premier operande dans le cas d'une alimentation de type vue
	if (IsFirstOperandViewType())
	{
		assert(GetOperandNumber() > 0);
		assert(KWType::IsRelation(GetFirstOperand()->GetType()));

		// Recherche des classes source et cible
		kwcSourceClass = derivedAttribute->GetParentClass()->GetDomain()->LookupClass(
		    GetFirstOperand()->GetObjectClassName());
		kwcTargetClass = derivedAttribute->GetClass();
		assert(kwcSourceClass != NULL);
		assert(kwcTargetClass != NULL);

		// Parcours des attributs natifs de la classe cible
		targetAttribute = kwcTargetClass->GetHeadAttribute();
		while (targetAttribute != NULL)
		{
			// On ne traite que les attributs natifs utilises
			//DDD if (targetAttribute->GetUsed() and targetAttribute->GetDerivationRule() == NULL)
			if (targetAttribute->GetDerivationRule() == NULL)
			{
				assert(KWType::IsData(targetAttribute->GetType()));

				// Recherche d'un attribut natif source de meme nom
				sourceAttribute = kwcSourceClass->LookupAttribute(targetAttribute->GetName());
				assert(sourceAttribute != NULL);
				assert(sourceAttribute->GetType() == targetAttribute->GetType());

				// Analyse de l'attribut si necessaire
				if (nkdAllUsedAttributes->Lookup(sourceAttribute) == NULL)
				{
					// Memorisation de l'attribut dans le dictionnaire
					nkdAllUsedAttributes->SetAt(sourceAttribute, sourceAttribute);

					// Acces a la regle d'attribut ou de bloc
					sourceAttributeRule = sourceAttribute->GetAnyDerivationRule();
					if (sourceAttributeRule != NULL)
						sourceAttributeRule->BuildAllUsedAttributes(sourceAttribute,
											    nkdAllUsedAttributes);
				}
			}

			// Attribut suivant
			kwcTargetClass->GetNextAttribute(targetAttribute);
		}
	}
}

boolean KWDRRelationCreationRule::IsFirstOperandViewType() const
{
	return true;
}

void KWDRRelationCreationRule::CopyObjectCommonNativeAttributes(const KWObject* kwoSourceObject,
								KWObject* kwoTargetObject) const
{
	ObjectArray* oaSourceObjectArray;
	ObjectArray* oaTargetObjectArray;
	int nAttribute;
	int nType;
	KWLoadIndex liSource;
	KWLoadIndex liTarget;

	require(IsCompiled());
	require(kwoSourceObject != NULL);
	require(kwoTargetObject != NULL);

	// Recopie des valeurs des attributs commun
	for (nAttribute = 0; nAttribute < ivTargetAttributeTypes.GetSize(); nAttribute++)
	{
		nType = ivTargetAttributeTypes.GetAt(nAttribute);
		liSource = livSourceAttributeLoadIndexes.GetAt(nAttribute);
		liTarget = livTargetAttributeLoadIndexes.GetAt(nAttribute);

		// Recopie de la valeur selon le type
		switch (nType)
		{
		case KWType::Symbol:
			kwoTargetObject->SetSymbolValueAt(liTarget, kwoSourceObject->ComputeSymbolValueAt(liSource));
			break;
		case KWType::Continuous:
			kwoTargetObject->SetContinuousValueAt(liTarget,
							      kwoSourceObject->ComputeContinuousValueAt(liSource));
			break;
		case KWType::Date:
			kwoTargetObject->SetDateValueAt(liTarget, kwoSourceObject->ComputeDateValueAt(liSource));
			break;
		case KWType::Time:
			kwoTargetObject->SetTimeValueAt(liTarget, kwoSourceObject->ComputeTimeValueAt(liSource));
			break;
		case KWType::Timestamp:
			kwoTargetObject->SetTimestampValueAt(liTarget,
							     kwoSourceObject->ComputeTimestampValueAt(liSource));
			break;
		case KWType::TimestampTZ:
			kwoTargetObject->SetTimestampTZValueAt(liTarget,
							       kwoSourceObject->ComputeTimestampTZValueAt(liSource));
			break;
		case KWType::Text:
			kwoTargetObject->SetTextValueAt(liTarget, kwoSourceObject->ComputeTextValueAt(liSource));
			break;
		case KWType::Object:
			kwoTargetObject->SetObjectValueAt(liTarget, kwoSourceObject->ComputeObjectValueAt(liSource));
			break;
		case KWType::ObjectArray:
			oaSourceObjectArray = kwoSourceObject->ComputeObjectArrayValueAt(liSource);
			oaTargetObjectArray = NULL;
			if (oaSourceObjectArray != NULL)
				oaTargetObjectArray = oaSourceObjectArray->Clone();
			kwoTargetObject->SetObjectArrayValueAt(liTarget, oaTargetObjectArray);
			break;
		default:
			assert(false);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRelationCreationRule

KWDRTableCreationRule::KWDRTableCreationRule()
{
	SetType(KWType::ObjectArray);
}

KWDRTableCreationRule::~KWDRTableCreationRule() {}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRProtoBuildTableView

KWDRProtoBuildTableView::KWDRProtoBuildTableView()
{
	SetName("ProtoBuildTableView");
	SetLabel("Table view");
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
}

KWDRProtoBuildTableView::~KWDRProtoBuildTableView() {}

KWDerivationRule* KWDRProtoBuildTableView::Create() const
{
	return new KWDRProtoBuildTableView;
}

ObjectArray* KWDRProtoBuildTableView::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoContainedObject;
	KWObject* kwoCopiedContainedObject;
	int nObject;

	require(IsCompiled());

	// Calcul du resultat
	oaResult.SetSize(0);

	// Duplication du tableau d'entree
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Copie du tableau en entree
		oaResult.SetSize(oaObjectArrayOperand->GetSize());
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
			kwoCopiedContainedObject = NewTargetObject(nObject + 1);
			CopyObjectCommonNativeAttributes(kwoContainedObject, kwoCopiedContainedObject);
			oaResult.SetAt(nObject, kwoCopiedContainedObject);
		}
	}
	return &oaResult;
}
