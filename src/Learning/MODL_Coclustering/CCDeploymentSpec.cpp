// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCDeploymentSpec.h"

CCDeploymentSpec::CCDeploymentSpec()
{
	bBuildPredictedClusterAttribute = false;
	bBuildClusterDistanceAttributes = false;
	bBuildFrequencyRecodingAttributes = false;

	// ## Custom constructor

	sOutputAttributesPrefix = "P_";

	// Le deploiement par distribution est actif pour les clusters
	bBuildPredictedClusterAttribute = true;
	bBuildClusterDistanceAttributes = false;
	bBuildFrequencyRecodingAttributes = false;

	// ##
}

CCDeploymentSpec::~CCDeploymentSpec()
{
	// ## Custom destructor

	// ##
}

void CCDeploymentSpec::CopyFrom(const CCDeploymentSpec* aSource)
{
	require(aSource != NULL);

	sInputClassName = aSource->sInputClassName;
	sInputObjectArrayAttributeName = aSource->sInputObjectArrayAttributeName;
	sDeployedAttributeName = aSource->sDeployedAttributeName;
	bBuildPredictedClusterAttribute = aSource->bBuildPredictedClusterAttribute;
	bBuildClusterDistanceAttributes = aSource->bBuildClusterDistanceAttributes;
	bBuildFrequencyRecodingAttributes = aSource->bBuildFrequencyRecodingAttributes;
	sOutputAttributesPrefix = aSource->sOutputAttributesPrefix;

	// ## Custom copyfrom

	// ##
}

CCDeploymentSpec* CCDeploymentSpec::Clone() const
{
	CCDeploymentSpec* aClone;

	aClone = new CCDeploymentSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void CCDeploymentSpec::Write(ostream& ost) const
{
	ost << "Input dictionary\t" << GetInputClassName() << "\n";
	if (GetInputObjectArrayAttributeName() != "")
		ost << "Input table variable\t" << GetInputObjectArrayAttributeName() << "\n";
	ost << "Coclustering deployed variable\t" << GetDeployedAttributeName() << "\n";
	ost << "Build predicted cluster variable\t" << BooleanToString(GetBuildPredictedClusterAttribute()) << "\n";
	ost << "Build inter-cluster distance variables\t" << BooleanToString(GetBuildClusterDistanceAttributes())
	    << "\n";
	ost << "Build frequency recoding variables\t" << BooleanToString(GetBuildFrequencyRecodingAttributes()) << "\n";
	ost << "Output variables prefix\t" << GetOutputAttributesPrefix() << "\n";
}

const ALString CCDeploymentSpec::GetClassLabel() const
{
	return "Deployment parameters";
}

// ## Method implementation

const ALString CCDeploymentSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean CCDeploymentSpec::GetBuildDistributionBasedAttributes() const
{
	return GetBuildPredictedClusterAttribute() or GetBuildClusterDistanceAttributes() or
	       GetBuildFrequencyRecodingAttributes();
}

boolean CCDeploymentSpec::PrepareCoclusteringDeployment(const CCHierarchicalDataGrid* coclusteringDataGrid,
							KWClassDomain*& deploymentDomain)
{
	boolean bOk = true;
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	CCHDGAttribute* hdgDeploymentAttribute;
	KWClass* kwcInputClass;
	KWClass* kwcDeploymentClass;
	ALString sDistributionAttributeName;
	KWAttribute* distributionAttribute;
	KWAttribute* frequencyVectorAttribute;
	KWDataGridStats coclusteringDataGridStats;
	KWAttribute* coclusteringAttribute;
	KWAttribute* labelVectorAttribute;
	KWAttribute* distributionValueAttribute;
	KWAttribute* dataGridDeploymentAttribute;
	KWAttribute* predictedPartIndexAttribute;
	KWAttribute* predictedPartLabelAttribute;
	ObjectArray oaDistributionValueAttributes;

	require(coclusteringDataGrid != NULL);
	require(not coclusteringDataGrid->IsVarPartDataGrid());
	require(deploymentDomain == NULL);

	// Test d'integrite pour le deploiement, en fonction des specifications et de la grille de coclustering
	if (bOk)
		bOk = CheckDeploymentSpec(coclusteringDataGrid);

	// Deploiement effectif
	if (bOk)
	{
		// Recherche de l'attribut de deploiement dans la grille
		hdgDeploymentAttribute =
		    cast(CCHDGAttribute*, coclusteringDataGrid->SearchAttribute(GetDeployedAttributeName()));

		//////////////////////////////////////////////////////////////////////////////////////
		// Creation du domaine de dploiement et identification des classe et attributs utiles

		// Creation du domaine de deploiement a partir de la classe de deploiement
		kwcInputClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetInputClassName());
		check(kwcInputClass);
		deploymentDomain = KWClassDomain::GetCurrentDomain()->CloneFromClass(kwcInputClass);

		// Recherche de la classe racine de deploiement
		kwcDeploymentClass = deploymentDomain->LookupClass(GetInputClassName());
		check(kwcDeploymentClass);

		// Recherche de l'attribut de distribution
		distributionAttribute = NULL;
		if (GetBuildDistributionBasedAttributes())
			distributionAttribute = kwcDeploymentClass->LookupAttribute(GetInputObjectArrayAttributeName());
		assert(not GetBuildDistributionBasedAttributes() or distributionAttribute != NULL);

		/////////////////////////////////////////////////////////////////////
		// Ajout d'attribut pour le deploiement de coclustering

		// Ajout d'un attribut de type grille dans une classe de deploiement
		coclusteringAttribute = AddDataGridAttribute(kwcDeploymentClass, coclusteringDataGrid);

		// Ajout d'un attribut pour les libelles associes a l'attribut
		labelVectorAttribute = AddLabelVectorAttributeAt(kwcDeploymentClass, hdgDeploymentAttribute);

		// Creation d'attributs exploitant l'attribut de distribution
		if (GetBuildDistributionBasedAttributes())
		{
			// Creation d'attributs de distribution des valeurs a partir d'une table secondaire
			for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
			{
				hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

				// Si attribut different de l'attribut de deploiement
				if (hdgAttribute != hdgDeploymentAttribute)
				{
					distributionValueAttribute = AddValueVectorAttributeAt(
					    kwcDeploymentClass, distributionAttribute, hdgAttribute);
					oaDistributionValueAttributes.Add(distributionValueAttribute);
				}
			}

			// Creation d'un attribut pour le vecteur de frequence optionnel
			frequencyVectorAttribute = NULL;
			if (coclusteringDataGrid->GetFrequencyAttributeName() != "")
				frequencyVectorAttribute =
				    AddFrequencyVectorAttribute(kwcDeploymentClass, distributionAttribute,
								coclusteringDataGrid->GetFrequencyAttributeName());

			// Creation d'un attribut de deploiement de grille
			dataGridDeploymentAttribute = AddDataGridDeploymentAttribute(
			    kwcDeploymentClass, coclusteringAttribute, &oaDistributionValueAttributes,
			    frequencyVectorAttribute, hdgDeploymentAttribute);

			// Creation d'un attribut de prediction de la partie de deploiement
			if (GetBuildPredictedClusterAttribute())
			{
				predictedPartIndexAttribute = AddPredictedPartIndexAttribute(
				    kwcDeploymentClass, dataGridDeploymentAttribute, hdgDeploymentAttribute);
				predictedPartLabelAttribute =
				    AddPartLabelAttribute(kwcDeploymentClass, predictedPartIndexAttribute,
							  labelVectorAttribute, hdgDeploymentAttribute, "Predicted");
				assert(predictedPartLabelAttribute != NULL); // Pour eviter un warning
			}

			// Creation d'attributs de distance pour l'attribut de deploiement
			if (GetBuildClusterDistanceAttributes())
			{
				AddPredictedPartDistancesAttributes(kwcDeploymentClass, dataGridDeploymentAttribute,
								    hdgDeploymentAttribute);
			}

			// Creation d'attributs d'effectif pour les attributs de distribution
			if (GetBuildFrequencyRecodingAttributes())
			{
				for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber();
				     nAttribute++)
				{
					hdgAttribute =
					    cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

					// Si attribut different de l'attribut de deploiement
					if (hdgAttribute != hdgDeploymentAttribute)
					{
						AddPredictedPartFrequenciesAttributesAt(
						    kwcDeploymentClass, dataGridDeploymentAttribute, hdgAttribute);
					}
				}
			}
		}

		// Completion des infos
		deploymentDomain->CompleteTypeInfo();

		// Test de validite du domaine de classe
		deploymentDomain->Check();
	}

	return bOk;
}

boolean CCDeploymentSpec::PrepareVarPartCoclusteringDeployment(const CCHierarchicalDataGrid* coclusteringDataGrid,
							       KWClassDomain*& deploymentDomain)
{
	boolean bOk = true;
	int nAttribute;
	CCHDGAttribute* hdgDeploymentAttribute;
	KWClass* kwcInputClass;
	KWClass* kwcDeploymentClass;
	ALString sDistributionAttributeName;
	KWDataGridStats coclusteringDataGridStats;
	KWAttribute* coclusteringAttribute;
	KWAttribute* labelVectorAttribute;
	KWAttribute* distributionValueAttribute;
	KWAttribute* dataGridDeploymentAttribute;
	KWAttribute* predictedPartIndexAttribute;
	KWAttribute* predictedPartLabelAttribute;
	ObjectArray oaDistributionValueAttributes;
	KWAttribute* innerVariablePartitionAttribute;
	KWAttribute* innerVariablePartitionIndexAttribute;
	KWAttribute* innerVariableVarPartLabelsAttribute;
	KWAttribute* varPartLabelInnerAttribute;
	KWDRSymbolVector* varPartVariableRule;
	ObjectArray oaVarPartLabelAttributes;
	KWDGAttribute* innerAttribute;
	KWDGAttribute* varPartAttribute;
	KWDerivationRuleOperand* innerVariableOperand;

	require(GetVarPartDeploymentMode());
	require(coclusteringDataGrid != NULL);
	require(coclusteringDataGrid->IsVarPartDataGrid());
	require(deploymentDomain == NULL);

	// Test d'integrite pour le deploiement, en fonction des specifications et de la grille de coclustering : a adapter
	if (bOk)
		bOk = CheckDeploymentSpec(coclusteringDataGrid);

	// Deploiement effectif
	if (bOk)
	{
		// Recherche de l'attribut de deploiement dans la grille
		hdgDeploymentAttribute =
		    cast(CCHDGAttribute*, coclusteringDataGrid->SearchAttribute(GetDeployedAttributeName()));

		//////////////////////////////////////////////////////////////////////////////////////
		// Creation du domaine de dploiement et identification des classe et attributs utiles

		// Creation du domaine de deploiement a partir de la classe de deploiement
		kwcInputClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetInputClassName());
		check(kwcInputClass);
		deploymentDomain = KWClassDomain::GetCurrentDomain()->CloneFromClass(kwcInputClass);

		// Recherche de la classe racine de deploiement
		kwcDeploymentClass = deploymentDomain->LookupClass(GetInputClassName());
		check(kwcDeploymentClass);

		/////////////////////////////////////////////////////////////////////
		// Ajout d'attribut pour le deploiement de coclustering

		// Ajout d'un attribut de type grille dans une classe de deploiement
		coclusteringAttribute = AddDataGridAttribute(kwcDeploymentClass, coclusteringDataGrid);

		// Ajout d'un attribut pour les libelles associes a l'attribut
		labelVectorAttribute = AddLabelVectorAttributeAt(kwcDeploymentClass, hdgDeploymentAttribute);

		// Creation d'attributs exploitant l'attribut de distribution
		varPartAttribute = coclusteringDataGrid->GetVarPartAttribute();
		distributionValueAttribute = new KWAttribute;
		distributionValueAttribute->SetName(
		    kwcDeploymentClass->BuildAttributeName(GetOutputAttributesPrefix() + "VariablesSet"));
		oaDistributionValueAttributes.Add(distributionValueAttribute);
		varPartVariableRule = new KWDRSymbolVector;
		varPartVariableRule->DeleteAllOperands();
		distributionValueAttribute->SetDerivationRule(varPartVariableRule);

		// Parcours des innerVariables
		for (nAttribute = 0; nAttribute < varPartAttribute->GetInnerAttributeNumber(); nAttribute++)
		{
			innerAttribute = varPartAttribute->GetInnerAttributeAt(nAttribute);

			// Creation d'un attribute des labels des VarPart de l'innerVariable
			innerVariableVarPartLabelsAttribute =
			    AddInnerAttributeVarPartLabelsAttribute(kwcDeploymentClass, innerAttribute);

			// Creation d'un attribut de partition pour l'innerVariable
			innerVariablePartitionAttribute =
			    AddInnerAttributePartitionAttribute(kwcDeploymentClass, innerAttribute);

			// Creation d'un attribut d'index de la partition de l'innerVariable
			innerVariablePartitionIndexAttribute = AddInnerAttributePartitionIndexAttribute(
			    kwcDeploymentClass, innerVariablePartitionAttribute, innerAttribute);

			// Creation d'un attribut donnant le label de la VarPart de l'innerVariable
			varPartLabelInnerAttribute = AddInnerAttributePartitionLabelAttribute(
			    kwcDeploymentClass, innerVariableVarPartLabelsAttribute,
			    innerVariablePartitionIndexAttribute);

			// Insertion de cet attribut dans la regle de la variable VarPart
			innerVariableOperand = new KWDerivationRuleOperand;
			innerVariableOperand->SetType(KWType::Symbol);
			innerVariableOperand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			innerVariableOperand->SetAttributeName(varPartLabelInnerAttribute->GetName());
			varPartVariableRule->AddOperand(innerVariableOperand);
		}
		kwcDeploymentClass->InsertAttribute(distributionValueAttribute);

		// Creation d'un attribut de deploiement de grille
		dataGridDeploymentAttribute =
		    AddDataGridDeploymentAttribute(kwcDeploymentClass, coclusteringAttribute,
						   &oaDistributionValueAttributes, NULL, hdgDeploymentAttribute);

		// Creation d'un attribut de prediction de la partie de deploiement
		predictedPartIndexAttribute = AddPredictedPartIndexAttribute(
		    kwcDeploymentClass, dataGridDeploymentAttribute, hdgDeploymentAttribute);
		predictedPartLabelAttribute =
		    AddPartLabelAttribute(kwcDeploymentClass, predictedPartIndexAttribute, labelVectorAttribute,
					  hdgDeploymentAttribute, "Predicted");
		assert(predictedPartLabelAttribute != NULL); // Pour eviter un warning

		// Completion des infos
		deploymentDomain->CompleteTypeInfo();

		// Test de validite du domaine de classe
		deploymentDomain->Check();
	}

	return bOk;
}

boolean CCDeploymentSpec::CheckDeploymentSpec(const CCHierarchicalDataGrid* coclusteringDataGrid) const
{
	boolean bOk = true;
	KWClass* kwcDeploymentClass;
	KWAttribute* kwaDistributionAttribute;

	require(coclusteringDataGrid != NULL);

	// Test des cas de coclustering traites
	if (bOk and coclusteringDataGrid->GetAttributeNumber() < 2)
	{
		bOk = false;
		AddError("Coclustering data grid must have at least two variables");
	}

	// Test de presence de l'attribut de deploiement
	if (bOk and GetDeployedAttributeName() == "")
	{
		bOk = false;
		AddError("Missing coclustering deployment variable name");
	}
	if (bOk and coclusteringDataGrid->SearchAttribute(GetDeployedAttributeName()) == NULL)
	{
		bOk = false;
		AddError("Coclustering deployment variable " + GetDeployedAttributeName() +
			 " not found in coclustering data grid");
	}

	// Un dictionnaire de deploiement doit etre specifie
	if (bOk and GetInputClassName() == "")
	{
		bOk = false;
		AddError("Missing input dictionary name");
	}

	// Le dictionnaire de deploiement doit exister
	kwcDeploymentClass = NULL;
	if (bOk)
		kwcDeploymentClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetInputClassName());
	if (bOk and kwcDeploymentClass == NULL)
	{
		bOk = false;
		AddError("Input dictionary " + GetInputClassName() + " not found");
	}

	// Erreur si aucune construction d'attribut de deploiement n'est demandee
	if (bOk and not GetBuildDistributionBasedAttributes())
	{
		bOk = false;
		AddError("No deployment variable must be built");
	}

	if (not coclusteringDataGrid->IsVarPartDataGrid())
	{
		// L'attribut de deploiement doit exister s'il est specifie
		kwaDistributionAttribute = NULL;
		if (bOk and GetInputObjectArrayAttributeName() != "")
			kwaDistributionAttribute =
			    kwcDeploymentClass->LookupAttribute(GetInputObjectArrayAttributeName());
		if (bOk and GetInputObjectArrayAttributeName() != "" and kwaDistributionAttribute == NULL)
		{
			bOk = false;
			AddError("Input table variable " + GetInputObjectArrayAttributeName() +
				 " not found in dictionary " + kwcDeploymentClass->GetName());
		}

		// Dans le cas de construction d'attributs exploitant la distribution, l'attribut de distribution doit exister
		// et etre valide dans la classe de deploiement
		if (bOk and GetBuildDistributionBasedAttributes())
		{
			// Existence de l'attribut de distribution
			if (bOk and GetInputObjectArrayAttributeName() == "")
			{
				bOk = false;
				AddError("Missing input table variable name");
			}
			assert(not bOk or kwaDistributionAttribute != NULL);

			// Validite de l'attribut de distribution
			if (bOk and not CheckAttributeConsistencyWithCoclusteringDataGrid(kwaDistributionAttribute,
											  coclusteringDataGrid))
			{
				bOk = false;
				AddError("Input table variable " + GetInputObjectArrayAttributeName() +
					 " in dictionary " + kwcDeploymentClass->GetName() +
					 " is not consistent with the coclustering variables");
			}
		}
	}
	return bOk;
}

boolean CCDeploymentSpec::CheckAttributeConsistencyWithPostProcessedSpec(const KWAttribute* kwaAttribute,
									 CCPostProcessingSpec* postProcessingSpec) const
{
	boolean bOk = true;
	KWClass* kwcDistributionClass;
	KWAttribute* kwaDistributionAttribute;
	KWAttribute* kwaFrequencyAttribute;
	int nAttribute;
	CCPostProcessedAttribute* postProcessAttribute;

	require(kwaAttribute != NULL);
	require(kwaAttribute->Check());
	require(postProcessingSpec != NULL);

	// L'attribut doit etre de type ObjectArray
	if (kwaAttribute->GetType() != KWType::ObjectArray)
		bOk = false;

	// Acces a la classe de distribution
	kwcDistributionClass = NULL;
	if (bOk)
	{
		kwcDistributionClass = kwaAttribute->GetClass();
		bOk = kwcDistributionClass != NULL;
	}

	// Il doit y avoir un coclustering avec au moins deux attributs
	if (bOk)
		bOk = postProcessingSpec->GetPostProcessedAttributes()->GetSize() >= 2;

	// Verification de l'attribut de frequence
	if (bOk)
	{
		if (postProcessingSpec->GetFrequencyAttributeName() != "")
		{
			kwaFrequencyAttribute =
			    kwcDistributionClass->LookupAttribute(postProcessingSpec->GetFrequencyAttributeName());
			if (kwaFrequencyAttribute == NULL or kwaFrequencyAttribute->GetType() != KWType::Continuous)
				bOk = false;
		}
	}

	// Verification des attributs de la classe de distribution
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < postProcessingSpec->GetPostProcessedAttributes()->GetSize();
		     nAttribute++)
		{
			postProcessAttribute =
			    cast(CCPostProcessedAttribute*,
				 postProcessingSpec->GetPostProcessedAttributes()->GetAt(nAttribute));

			// On ne teste que les attributs de distributions (hors attribut de deploiement)
			if (postProcessAttribute->GetName() != GetDeployedAttributeName())
			{
				kwaDistributionAttribute =
				    kwcDistributionClass->LookupAttribute(postProcessAttribute->GetName());
				if (kwaDistributionAttribute == NULL or
				    kwaDistributionAttribute->GetType() !=
					KWType::ToType(postProcessAttribute->GetType()))
				{
					bOk = false;
					break;
				}
			}
		}
	}
	return bOk;
}

boolean CCDeploymentSpec::CheckAttributeConsistencyWithCoclusteringDataGrid(
    const KWAttribute* kwaAttribute, const CCHierarchicalDataGrid* coclusteringDataGrid) const
{
	boolean bOk = true;
	KWClass* kwcDistributionClass;
	KWAttribute* kwaDistributionAttribute;
	KWAttribute* kwaFrequencyAttribute;
	int nAttribute;
	CCHDGAttribute* coclusteringAttribute;

	require(kwaAttribute != NULL);
	require(kwaAttribute->Check());
	require(coclusteringDataGrid != NULL);
	require(not coclusteringDataGrid->IsVarPartDataGrid());

	// L'attribut doit etre de type ObjectArray
	if (kwaAttribute->GetType() != KWType::ObjectArray)
		bOk = false;

	// Acces a la classe de distribution
	kwcDistributionClass = NULL;
	if (bOk)
	{
		kwcDistributionClass = kwaAttribute->GetClass();
		bOk = kwcDistributionClass != NULL;
	}

	// Il doit y avoir un coclustering avec au moins deux attributs
	if (bOk)
		bOk = coclusteringDataGrid->GetAttributeNumber() >= 2;

	// Verification de l'attribut de frequence
	if (bOk)
	{
		if (coclusteringDataGrid->GetFrequencyAttributeName() != "")
		{
			kwaFrequencyAttribute =
			    kwcDistributionClass->LookupAttribute(coclusteringDataGrid->GetFrequencyAttributeName());
			if (kwaFrequencyAttribute == NULL or kwaFrequencyAttribute->GetType() != KWType::Continuous)
				bOk = false;
		}
	}

	// Verification des attributs de la classe de distribution
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			coclusteringAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

			// On ne teste que les attributs de distributions (hors attribut de deploiement)
			if (coclusteringAttribute->GetAttributeName() != GetDeployedAttributeName())
			{
				kwaDistributionAttribute =
				    kwcDistributionClass->LookupAttribute(coclusteringAttribute->GetAttributeName());
				if (kwaDistributionAttribute == NULL or
				    kwaDistributionAttribute->GetType() != coclusteringAttribute->GetAttributeType())
				{
					bOk = false;
					break;
				}
			}
		}
	}
	return bOk;
}

boolean CCDeploymentSpec::CheckClassConsistencyWithPostProcessedSpec(const KWClass* kwcClass,
								     CCPostProcessingSpec* postProcessingSpec) const
{
	KWAttribute* kwaAttribute;

	require(kwcClass != NULL);
	require(postProcessingSpec != NULL);

	// Test de chaque attribut
	kwaAttribute = kwcClass->GetHeadAttribute();
	while (kwaAttribute != NULL)
	{
		if (CheckAttributeConsistencyWithPostProcessedSpec(kwaAttribute, postProcessingSpec))
			return true;
		kwcClass->GetNextAttribute(kwaAttribute);
	}
	return false;
}

void CCDeploymentSpec::FillInputClassAndAttributeNames(CCPostProcessingSpec* postProcessingSpec)
{
	KWClass* kwcInputClass;
	int nClass;
	KWClass* kwcClass;
	KWAttribute* kwaInputObjectArrayAttribute;
	KWAttribute* kwaAttribute;

	require(postProcessingSpec != NULL);

	// On tente les alimentations si necessaire
	if (GetBuildDistributionBasedAttributes() and postProcessingSpec->GetPostProcessedAttributes()->GetSize() >= 2)
	{
		// Alimentation du dictionnaire si le dictionnaire courant n'est pas compatible
		kwcInputClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetInputClassName());
		if (kwcInputClass == NULL or
		    not CheckClassConsistencyWithPostProcessedSpec(kwcInputClass, postProcessingSpec))
		{
			// Recherche du premier dictionnaire compatible
			kwcInputClass = NULL;
			for (nClass = 0; nClass < KWClassDomain::GetCurrentDomain()->GetClassNumber(); nClass++)
			{
				kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(nClass);
				if (CheckClassConsistencyWithPostProcessedSpec(kwcClass, postProcessingSpec))
				{
					kwcInputClass = kwcClass;
					break;
				}
			}

			// Memorisation du nom du dictionnaire
			if (kwcInputClass == NULL)
				SetInputClassName("");
			else
				SetInputClassName(kwcInputClass->GetName());
		}

		// Si pas de dictionnaire compatible, on vide l'attribut courant
		if (GetInputClassName() == "")
			SetInputObjectArrayAttributeName("");
		// Sinon, on cherche un attribut compatible
		else
		{
			assert(kwcInputClass != NULL);
			assert(kwcInputClass->GetName() == GetInputClassName());

			// Alimentation de l'attribut si l'attribut courant n'est pas compatible
			kwaInputObjectArrayAttribute =
			    kwcInputClass->LookupAttribute(GetInputObjectArrayAttributeName());
			if (kwaInputObjectArrayAttribute == NULL or
			    not CheckAttributeConsistencyWithPostProcessedSpec(kwaInputObjectArrayAttribute,
									       postProcessingSpec))
			{
				// Recherche du premier attribut compatible
				kwaInputObjectArrayAttribute = NULL;
				kwaAttribute = kwcInputClass->GetHeadAttribute();
				while (kwaAttribute != NULL)
				{
					if (CheckAttributeConsistencyWithPostProcessedSpec(kwaAttribute,
											   postProcessingSpec))
					{
						kwaInputObjectArrayAttribute = kwaAttribute;
						break;
					}
					kwcInputClass->GetNextAttribute(kwaAttribute);
				}

				// Memorisation du nom de l'attribut
				if (kwaInputObjectArrayAttribute == NULL)
					SetInputObjectArrayAttributeName("");
				else
					SetInputObjectArrayAttributeName(kwaInputObjectArrayAttribute->GetName());
			}
		}
	}
}

KWAttribute* CCDeploymentSpec::AddDataGridAttribute(KWClass* kwcDeploymentClass,
						    const CCHierarchicalDataGrid* coclusteringDataGrid) const
{
	KWDataGridStats coclusteringDataGridStats;
	KWDRDataGrid* dgRule;
	KWAttribute* dgAttribute;

	require(kwcDeploymentClass != NULL);
	require(coclusteringDataGrid != NULL);

	// Export de la grille de coclustering vers un objet intermediaire de stats de grilles
	// Cas d'une grille variable * variable
	if (not coclusteringDataGrid->IsVarPartDataGrid())
		coclusteringDataGrid->ExportDataGridStats(&coclusteringDataGridStats);
	else
		coclusteringDataGrid->ExportVarPartDataGridStats(&coclusteringDataGridStats);

	// Creation d'un attribut de grille
	dgRule = new KWDRDataGrid;
	dgRule->ImportDataGridStats(&coclusteringDataGridStats, true);
	dgAttribute = new KWAttribute;
	dgAttribute->SetName(kwcDeploymentClass->BuildAttributeName(GetOutputAttributesPrefix() + "Coclustering"));
	dgAttribute->SetDerivationRule(dgRule);
	dgAttribute->SetUsed(false);
	dgAttribute->SetLabel(coclusteringDataGridStats.ExportVariableNames());
	kwcDeploymentClass->InsertAttribute(dgAttribute);
	return dgAttribute;
}

KWAttribute* CCDeploymentSpec::AddParentClusterIndexAttribute(KWClass* kwcDeploymentClass,
							      KWAttribute* kwaDataGridAttribute,
							      const CCHDGAttribute* hdgDeploymentAttribute) const
{
	KWDRPartIndexAt* partIndexAtRule;
	KWAttribute* partIndexAtAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaDataGridAttribute != NULL);
	require(kwaDataGridAttribute->GetParentClass() == kwcDeploymentClass);
	require(hdgDeploymentAttribute != NULL);

	// Creation d'une regle de deploiement de grille, en la parametrant par l'ensemble des distributions de valeurs
	partIndexAtRule = new KWDRPartIndexAt;
	partIndexAtRule->DeleteAllVariableOperands();
	partIndexAtRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	partIndexAtRule->GetFirstOperand()->SetAttributeName(kwaDataGridAttribute->GetName());
	partIndexAtRule->GetSecondOperand()->SetContinuousConstant(
	    (Continuous)(hdgDeploymentAttribute->GetAttributeIndex() + 1));
	partIndexAtRule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	partIndexAtRule->GetOperandAt(2)->SetAttributeName(hdgDeploymentAttribute->GetAttributeName());

	// Creation d'un attribut de deploiement de grille
	partIndexAtAttribute = new KWAttribute;
	partIndexAtAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDeploymentAttribute->GetAttributeName() + "ClusterIndex"));
	partIndexAtAttribute->SetDerivationRule(partIndexAtRule);
	partIndexAtAttribute->SetUsed(false);
	partIndexAtAttribute->SetLabel("Cluster index for variable " + hdgDeploymentAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(partIndexAtAttribute);
	return partIndexAtAttribute;
}

KWAttribute* CCDeploymentSpec::AddValueVectorAttributeAt(KWClass* kwcDeploymentClass,
							 KWAttribute* kwaObjectArrayAttribute,
							 const CCHDGAttribute* hdgDistributionAttribute) const
{
	KWDerivationRule* valueVectorRule;
	KWAttribute* distributionValueAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaObjectArrayAttribute != NULL);
	require(kwaObjectArrayAttribute->GetParentClass() == kwcDeploymentClass);
	require(kwaObjectArrayAttribute->GetType() == KWType::ObjectArray);
	require(hdgDistributionAttribute != NULL);
	require(kwaObjectArrayAttribute->GetClass()->LookupAttribute(hdgDistributionAttribute->GetAttributeName()) !=
		NULL);
	require(kwaObjectArrayAttribute->GetClass()
		    ->LookupAttribute(hdgDistributionAttribute->GetAttributeName())
		    ->GetType() == hdgDistributionAttribute->GetAttributeType());

	// Creation d'un attribut de distribution des valeurs
	if (hdgDistributionAttribute->GetAttributeType() == KWType::Symbol)
		valueVectorRule = new KWDRTableSymbolVector;
	else
		valueVectorRule = new KWDRTableContinuousVector;
	valueVectorRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueVectorRule->GetFirstOperand()->SetAttributeName(kwaObjectArrayAttribute->GetName());
	valueVectorRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueVectorRule->GetSecondOperand()->SetAttributeName(hdgDistributionAttribute->GetAttributeName());
	distributionValueAttribute = new KWAttribute;
	distributionValueAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDistributionAttribute->GetAttributeName() + "Set"));
	distributionValueAttribute->SetDerivationRule(valueVectorRule);
	distributionValueAttribute->SetUsed(false);
	distributionValueAttribute->SetLabel("Value distribution for variable " +
					     hdgDistributionAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(distributionValueAttribute);
	return distributionValueAttribute;
}

KWAttribute* CCDeploymentSpec::AddFrequencyVectorAttribute(KWClass* kwcDeploymentClass,
							   KWAttribute* kwaObjectArrayAttribute,
							   const ALString& sFrequencyAttributeName) const
{
	KWDerivationRule* frequencyVectorRule;
	KWAttribute* frequencyVectorAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaObjectArrayAttribute != NULL);
	require(kwaObjectArrayAttribute->GetParentClass() == kwcDeploymentClass);
	require(kwaObjectArrayAttribute->GetType() == KWType::ObjectArray);
	require(kwaObjectArrayAttribute->GetClass()->LookupAttribute(sFrequencyAttributeName) != NULL);
	require(kwaObjectArrayAttribute->GetClass()->LookupAttribute(sFrequencyAttributeName)->GetType() ==
		KWType::Continuous);

	// Creation d'un attribut de vecteur de frequence
	frequencyVectorRule = new KWDRTableContinuousVector;
	frequencyVectorRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	frequencyVectorRule->GetFirstOperand()->SetAttributeName(kwaObjectArrayAttribute->GetName());
	frequencyVectorRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	frequencyVectorRule->GetSecondOperand()->SetAttributeName(sFrequencyAttributeName);
	frequencyVectorAttribute = new KWAttribute;
	frequencyVectorAttribute->SetName(
	    kwcDeploymentClass->BuildAttributeName(GetOutputAttributesPrefix() + sFrequencyAttributeName + "Vector"));
	frequencyVectorAttribute->SetDerivationRule(frequencyVectorRule);
	frequencyVectorAttribute->SetUsed(false);
	frequencyVectorAttribute->SetLabel("Frequency vector from variable " + sFrequencyAttributeName);
	kwcDeploymentClass->InsertAttribute(frequencyVectorAttribute);
	return frequencyVectorAttribute;
}

KWAttribute* CCDeploymentSpec::AddDataGridDeploymentAttribute(KWClass* kwcDeploymentClass,
							      KWAttribute* kwaDataGridAttribute,
							      ObjectArray* oaValueVectorAttributes,
							      KWAttribute* kwaFrequencyVectorAttribute,
							      const CCHDGAttribute* hdgDeploymentAttribute) const
{
	KWDRDataGridDeployment* dataGridDeploymentRule;
	KWAttribute* dataGridDeploymentAttribute;
	KWAttribute* kwaValueVectorAttribute;
	KWDerivationRuleOperand* operand;
	int nAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaDataGridAttribute != NULL);
	require(kwaDataGridAttribute->GetParentClass() == kwcDeploymentClass);
	require(oaValueVectorAttributes != NULL);
	require(oaValueVectorAttributes->GetSize() > 0);
	require(hdgDeploymentAttribute != NULL);

	// Creation d'une regle de deploiement de grille, en la parametrant par l'ensemble des distributions de valeurs
	dataGridDeploymentRule = new KWDRDataGridDeployment;
	dataGridDeploymentRule->DeleteAllVariableOperands();
	dataGridDeploymentRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	dataGridDeploymentRule->GetFirstOperand()->SetAttributeName(kwaDataGridAttribute->GetName());
	dataGridDeploymentRule->GetSecondOperand()->SetContinuousConstant(
	    (Continuous)(hdgDeploymentAttribute->GetAttributeIndex() + 1));
	for (nAttribute = 0; nAttribute < oaValueVectorAttributes->GetSize(); nAttribute++)
	{
		kwaValueVectorAttribute = cast(KWAttribute*, oaValueVectorAttributes->GetAt(nAttribute));
		assert(kwaValueVectorAttribute->GetParentClass() == kwcDeploymentClass);
		assert(kwaValueVectorAttribute->GetDerivationRule() != NULL);

		// Ajout d'un operande de distribution de valeurs
		operand = new KWDerivationRuleOperand;
		dataGridDeploymentRule->AddOperand(operand);
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeName(kwaValueVectorAttribute->GetName());
	}

	// Ajout d'un operande pour le vecteur de distribution optionnel
	if (kwaFrequencyVectorAttribute != NULL)
	{
		operand = new KWDerivationRuleOperand;
		dataGridDeploymentRule->AddOperand(operand);
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeName(kwaFrequencyVectorAttribute->GetName());
	}

	// Creation d'un attribut de deploiement de grille
	dataGridDeploymentAttribute = new KWAttribute;
	dataGridDeploymentAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + "DeployedCoclusteringAt" + hdgDeploymentAttribute->GetAttributeName()));
	dataGridDeploymentAttribute->SetDerivationRule(dataGridDeploymentRule);
	dataGridDeploymentAttribute->SetUsed(false);
	dataGridDeploymentAttribute->SetLabel("Deployed coclustering for variable " +
					      hdgDeploymentAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(dataGridDeploymentAttribute);
	return dataGridDeploymentAttribute;
}

KWAttribute* CCDeploymentSpec::AddPredictedPartIndexAttribute(KWClass* kwcDeploymentClass,
							      KWAttribute* kwaDataGridDeploymentAttribute,
							      const CCHDGAttribute* hdgDeploymentAttribute) const
{
	KWDRPredictedPartIndex* predictedPartIndexRule;
	KWAttribute* predictedPartIndexAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaDataGridDeploymentAttribute != NULL);
	require(kwaDataGridDeploymentAttribute->GetParentClass() == kwcDeploymentClass);
	require(hdgDeploymentAttribute != NULL);

	// Creation d'un attribut de prediction de l'index de la partie cible
	predictedPartIndexRule = new KWDRPredictedPartIndex;
	predictedPartIndexRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	predictedPartIndexRule->GetFirstOperand()->SetAttributeName(kwaDataGridDeploymentAttribute->GetName());
	predictedPartIndexAttribute = new KWAttribute;
	predictedPartIndexAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDeploymentAttribute->GetAttributeName() + "Index"));
	predictedPartIndexAttribute->SetDerivationRule(predictedPartIndexRule);
	predictedPartIndexAttribute->SetUsed(false);
	predictedPartIndexAttribute->SetLabel("Predicted cluster index for variable " +
					      hdgDeploymentAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(predictedPartIndexAttribute);
	return predictedPartIndexAttribute;
}

void CCDeploymentSpec::AddPredictedPartDistancesAttributes(KWClass* kwcDeploymentClass,
							   KWAttribute* kwaDataGridDeploymentAttribute,
							   const CCHDGAttribute* hdgDeploymentAttribute) const
{
	KWDRPredictedPartDistances* predictedPartDistancesRule;
	KWAttribute* predictedPartDistancesAttribute;
	KWDRContinuousValueAt* valueAtRule;
	KWAttribute* distanceAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	int nPart;

	require(kwcDeploymentClass != NULL);
	require(kwaDataGridDeploymentAttribute != NULL);
	require(kwaDataGridDeploymentAttribute->GetParentClass() == kwcDeploymentClass);
	require(hdgDeploymentAttribute != NULL);

	// Creation d'un attribut des distances de l'attribut de distribution
	predictedPartDistancesRule = new KWDRPredictedPartDistances;
	predictedPartDistancesRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	predictedPartDistancesRule->GetFirstOperand()->SetAttributeName(kwaDataGridDeploymentAttribute->GetName());
	predictedPartDistancesAttribute = new KWAttribute;
	predictedPartDistancesAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDeploymentAttribute->GetAttributeName() + "PartDistances"));
	predictedPartDistancesAttribute->SetDerivationRule(predictedPartDistancesRule);
	predictedPartDistancesAttribute->SetUsed(false);
	predictedPartDistancesAttribute->SetLabel("Vector of cluster distances for variable " +
						  hdgDeploymentAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(predictedPartDistancesAttribute);

	// Creation d'un attribut de distance par partie de l'attribut de distribution
	// Parcours des parties
	dgPart = hdgDeploymentAttribute->GetHeadPart();
	nPart = 0;
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);

		// Creation d'une regle d'extraction de valeur du vecteur des distance
		valueAtRule = new KWDRContinuousValueAt;
		valueAtRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		valueAtRule->GetFirstOperand()->SetAttributeName(predictedPartDistancesAttribute->GetName());
		valueAtRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		valueAtRule->GetSecondOperand()->SetContinuousConstant((Continuous)(nPart + 1));

		// Creation d'un attribut de distances de la partie
		distanceAttribute = new KWAttribute;
		distanceAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
		    GetOutputAttributesPrefix() + hdgDeploymentAttribute->GetAttributeName() + "Distance" +
		    hdgPart->GetUserLabel()));
		distanceAttribute->SetDerivationRule(valueAtRule);
		kwcDeploymentClass->InsertAttribute(distanceAttribute);

		// Partie suivante
		hdgDeploymentAttribute->GetNextPart(dgPart);
		nPart++;
	}
}

void CCDeploymentSpec::AddPredictedPartFrequenciesAttributesAt(KWClass* kwcDeploymentClass,
							       KWAttribute* kwaDataGridDeploymentAttribute,
							       const CCHDGAttribute* hdgDistributionAttribute) const
{
	KWDRPredictedPartFrequenciesAt* predictedPartFrequenciesAtRule;
	KWAttribute* predictedPartFrequenciesAtAttribute;
	KWDRContinuousValueAt* valueAtRule;
	KWAttribute* frequencyAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	int nPart;

	require(kwcDeploymentClass != NULL);
	require(kwaDataGridDeploymentAttribute != NULL);
	require(kwaDataGridDeploymentAttribute->GetParentClass() == kwcDeploymentClass);
	require(hdgDistributionAttribute != NULL);

	// Creation d'un attribut des effectifs de l'attribut de distribution
	predictedPartFrequenciesAtRule = new KWDRPredictedPartFrequenciesAt;
	predictedPartFrequenciesAtRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	predictedPartFrequenciesAtRule->GetFirstOperand()->SetAttributeName(kwaDataGridDeploymentAttribute->GetName());
	predictedPartFrequenciesAtRule->GetSecondOperand()->SetContinuousConstant(
	    (Continuous)(hdgDistributionAttribute->GetAttributeIndex() + 1));
	predictedPartFrequenciesAtAttribute = new KWAttribute;
	predictedPartFrequenciesAtAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDistributionAttribute->GetAttributeName() + "PartFrequencies"));
	predictedPartFrequenciesAtAttribute->SetDerivationRule(predictedPartFrequenciesAtRule);
	predictedPartFrequenciesAtAttribute->SetUsed(false);
	predictedPartFrequenciesAtAttribute->SetLabel("Vector of cluster frequencies for variable " +
						      hdgDistributionAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(predictedPartFrequenciesAtAttribute);

	// Creation d'un attribut d'effectif par partie de l'attribut de distribution
	// Parcours des parties
	dgPart = hdgDistributionAttribute->GetHeadPart();
	nPart = 0;
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);

		// Creation d'une regle d'extraction de valeur du vecteur des effectif
		valueAtRule = new KWDRContinuousValueAt;
		valueAtRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		valueAtRule->GetFirstOperand()->SetAttributeName(predictedPartFrequenciesAtAttribute->GetName());
		valueAtRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		valueAtRule->GetSecondOperand()->SetContinuousConstant((Continuous)(nPart + 1));

		// Creation d'un attribut d'effectifs de la partie
		frequencyAttribute = new KWAttribute;
		frequencyAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
		    GetOutputAttributesPrefix() + hdgDistributionAttribute->GetAttributeName() + "Frequency" +
		    hdgPart->GetUserLabel()));
		frequencyAttribute->SetDerivationRule(valueAtRule);
		kwcDeploymentClass->InsertAttribute(frequencyAttribute);

		// Partie suivante
		hdgDistributionAttribute->GetNextPart(dgPart);
		nPart++;
	}
}

KWAttribute* CCDeploymentSpec::AddPartLabelAttribute(KWClass* kwcDeploymentClass, KWAttribute* kwaIndexAttribute,
						     KWAttribute* kwaLabelVectorAttribute,
						     const CCHDGAttribute* hdgDeploymentAttribute,
						     const ALString& sPrefix) const
{
	KWDRSymbolValueAt* symbolValueAtRule;
	KWAttribute* partLabelAtAttribute;

	require(kwcDeploymentClass != NULL);
	require(kwaIndexAttribute != NULL);
	require(kwaIndexAttribute->GetParentClass() == kwcDeploymentClass);
	require(kwaLabelVectorAttribute != NULL);
	require(kwaLabelVectorAttribute->GetParentClass() == kwcDeploymentClass);
	require(hdgDeploymentAttribute != NULL);
	require(sPrefix != "");

	// Creation d'un attribut de prediction du libelle de la partie cible
	symbolValueAtRule = new KWDRSymbolValueAt;
	symbolValueAtRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	symbolValueAtRule->GetFirstOperand()->SetAttributeName(kwaLabelVectorAttribute->GetName());
	symbolValueAtRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	symbolValueAtRule->GetSecondOperand()->SetAttributeName(kwaIndexAttribute->GetName());
	partLabelAtAttribute = new KWAttribute;
	partLabelAtAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgDeploymentAttribute->GetAttributeName() + sPrefix + "Label"));
	partLabelAtAttribute->SetDerivationRule(symbolValueAtRule);
	partLabelAtAttribute->SetLabel(sPrefix + " label for variable " + hdgDeploymentAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(partLabelAtAttribute);
	return partLabelAtAttribute;
}

KWAttribute* CCDeploymentSpec::AddLabelVectorAttributeAt(KWClass* kwcDeploymentClass,
							 const CCHDGAttribute* hdgAttribute) const
{
	KWDRSymbolVector* labelVectorRule;
	KWAttribute* labelVectorAttribute;
	int nPart;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;

	require(kwcDeploymentClass != NULL);
	require(hdgAttribute != NULL);

	// Creation d'un attribut de vecteur de valeurs
	labelVectorRule = new KWDRSymbolVector;
	labelVectorRule->SetValueNumber(hdgAttribute->GetPartNumber());
	dgPart = hdgAttribute->GetHeadPart();
	nPart = 0;
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		labelVectorRule->SetValueAt(nPart, Symbol(hdgPart->GetUserLabel()));

		// Partie suivante
		hdgAttribute->GetNextPart(dgPart);
		nPart++;
	}
	labelVectorAttribute = new KWAttribute;
	labelVectorAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + hdgAttribute->GetAttributeName() + "Labels"));
	labelVectorAttribute->SetDerivationRule(labelVectorRule);
	labelVectorAttribute->SetUsed(false);
	labelVectorAttribute->SetLabel("Cluster labels for variable " + hdgAttribute->GetAttributeName());
	kwcDeploymentClass->InsertAttribute(labelVectorAttribute);
	return labelVectorAttribute;
}

KWAttribute* CCDeploymentSpec::AddInnerAttributePartitionAttribute(KWClass* kwcDeploymentClass,
								   KWDGAttribute* innerAttribute) const
{
	KWAttribute* dgAttribute;
	KWDRIntervalBounds* intervalBoundsRule;
	KWDRValueGroups* valueGroupsRule;
	KWDerivationRuleOperand* valueGroupOperand;
	KWDRValueGroup* valueGroupRule;
	ObjectArray* oaParts;
	ObjectArray* oaValues;
	int nPart;
	int nValue;

	require(GetVarPartDeploymentMode());
	require(kwcDeploymentClass != NULL);
	require(innerAttribute != NULL);
	require(innerAttribute->IsInnerAttribute());

	dgAttribute = new KWAttribute;
	dgAttribute->SetName(kwcDeploymentClass->BuildAttributeName(GetOutputAttributesPrefix() +
								    innerAttribute->GetAttributeName() + "Partition"));

	oaParts = new ObjectArray;
	innerAttribute->ExportParts(oaParts);

	if (innerAttribute->GetAttributeType() == KWType::Continuous)
	{
		intervalBoundsRule = new KWDRIntervalBounds;

		dgAttribute->SetDerivationRule(intervalBoundsRule);
		intervalBoundsRule->SetIntervalBoundNumber(oaParts->GetSize() - 1);
		for (nPart = 0; nPart < oaParts->GetSize() - 1; nPart++)
		{
			intervalBoundsRule->SetIntervalBoundAt(
			    nPart, cast(KWDGPart*, oaParts->GetAt(nPart))->GetInterval()->GetUpperBound());
		}
	}
	else
	{
		valueGroupsRule = new KWDRValueGroups;
		valueGroupsRule->DeleteAllOperands();

		dgAttribute->SetDerivationRule(valueGroupsRule);

		for (nPart = 0; nPart < oaParts->GetSize(); nPart++)
		{
			// Creation d'un operande pour le groupe
			valueGroupOperand = new KWDerivationRuleOperand;
			valueGroupOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			valueGroupOperand->SetType(KWType::Structure);

			// Creation d'un nouveau groupe
			valueGroupRule = new KWDRValueGroup;
			valueGroupOperand->SetDerivationRule(valueGroupRule);
			valueGroupOperand->SetStructureName(valueGroupRule->GetStructureName());
			valueGroupOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			valueGroupOperand->SetType(KWType::Structure);

			// Ajout des valeurs du groupe
			oaValues = new ObjectArray;
			cast(KWDGPart*, oaParts->GetAt(nPart))->GetSymbolValueSet()->ExportValues(oaValues);

			valueGroupRule->SetValueNumber(oaValues->GetSize());
			for (nValue = 0; nValue < oaValues->GetSize(); nValue++)
			{
				valueGroupRule->SetValueAt(nValue,
							   cast(KWDGValue*, oaValues->GetAt(nValue))->GetSymbolValue());
			}
			delete oaValues;
			valueGroupsRule->AddOperand(valueGroupOperand);
		}
	}
	delete oaParts;
	dgAttribute->SetUsed(false);
	kwcDeploymentClass->InsertAttribute(dgAttribute);
	return dgAttribute;
}

KWAttribute* CCDeploymentSpec::AddInnerAttributePartitionIndexAttribute(KWClass* kwcDeploymentClass,
									KWAttribute* ivPartitionAttribute,
									KWDGAttribute* innerAttribute) const
{
	KWDRGroupIndex* groupIndexRule;
	KWDRIntervalIndex* intervalIndexRule;
	KWAttribute* dgAttribute;
	KWDerivationRuleOperand* partIndexOperand1;
	KWDerivationRuleOperand* partIndexOperand2;

	require(GetVarPartDeploymentMode());
	require(kwcDeploymentClass != NULL);
	require(ivPartitionAttribute != NULL);
	require(innerAttribute != NULL);
	require(innerAttribute->IsInnerAttribute());

	dgAttribute = new KWAttribute;
	dgAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + innerAttribute->GetAttributeName() + "PartitionIndex"));

	if (innerAttribute->GetAttributeType() == KWType::Continuous)
	{
		intervalIndexRule = new KWDRIntervalIndex;
		intervalIndexRule->DeleteAllOperands();

		dgAttribute->SetDerivationRule(intervalIndexRule);

		partIndexOperand1 = new KWDerivationRuleOperand;
		partIndexOperand1->SetType(KWType::Structure);
		partIndexOperand1->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		partIndexOperand1->SetAttributeName(ivPartitionAttribute->GetName());
		intervalIndexRule->AddOperand(partIndexOperand1);

		partIndexOperand2 = new KWDerivationRuleOperand;
		partIndexOperand2->SetType(KWType::Continuous);
		partIndexOperand2->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		partIndexOperand2->SetAttributeName(innerAttribute->GetAttributeName());
		intervalIndexRule->AddOperand(partIndexOperand2);
	}
	else
	{
		groupIndexRule = new KWDRGroupIndex;
		groupIndexRule->DeleteAllOperands();

		dgAttribute->SetDerivationRule(groupIndexRule);

		partIndexOperand1 = new KWDerivationRuleOperand;
		partIndexOperand1->SetType(KWType::Structure);
		partIndexOperand1->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		partIndexOperand1->SetAttributeName(ivPartitionAttribute->GetName());
		groupIndexRule->AddOperand(partIndexOperand1);

		partIndexOperand2 = new KWDerivationRuleOperand;
		partIndexOperand2->SetType(KWType::Symbol);
		partIndexOperand2->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		partIndexOperand2->SetAttributeName(innerAttribute->GetAttributeName());
		groupIndexRule->AddOperand(partIndexOperand2);
	}
	dgAttribute->SetUsed(false);
	kwcDeploymentClass->InsertAttribute(dgAttribute);
	return dgAttribute;
}

KWAttribute* CCDeploymentSpec::AddInnerAttributeVarPartLabelsAttribute(KWClass* kwcDeploymentClass,
								       KWDGAttribute* innerAttribute) const
{
	KWAttribute* dgAttribute;
	KWDRSymbolVector* vectorRule;
	ObjectArray* oaParts;
	int nPart;

	require(GetVarPartDeploymentMode());
	require(kwcDeploymentClass != NULL);
	require(innerAttribute != NULL);
	require(innerAttribute->IsInnerAttribute());

	dgAttribute = new KWAttribute;
	dgAttribute->SetName(kwcDeploymentClass->BuildAttributeName(
	    GetOutputAttributesPrefix() + innerAttribute->GetAttributeName() + "VarPartLabels"));

	vectorRule = new KWDRSymbolVector;
	dgAttribute->SetDerivationRule(vectorRule);

	oaParts = new ObjectArray;
	innerAttribute->ExportParts(oaParts);
	vectorRule->SetValueNumber(oaParts->GetSize());
	for (nPart = 0; nPart < oaParts->GetSize(); nPart++)
	{
		vectorRule->SetValueAt(nPart, Symbol(cast(KWDGPart*, oaParts->GetAt(nPart))->GetVarPartLabel()));
	}
	delete oaParts;

	dgAttribute->SetUsed(false);
	kwcDeploymentClass->InsertAttribute(dgAttribute);
	return dgAttribute;
}

KWAttribute* CCDeploymentSpec::AddInnerAttributePartitionLabelAttribute(KWClass* kwcDeploymentClass,
									KWAttribute* ivVarPartLabelsAttribute,
									KWAttribute* ivIndexAttribute) const
{
	KWDRSymbolValueAt* varPartRule;
	KWAttribute* dgAttribute;
	KWDerivationRuleOperand* partIndexOperand1;
	KWDerivationRuleOperand* partIndexOperand2;

	require(GetVarPartDeploymentMode());
	require(kwcDeploymentClass != NULL);
	require(ivVarPartLabelsAttribute != NULL);
	require(ivIndexAttribute != NULL);

	dgAttribute = new KWAttribute;
	dgAttribute->SetName(kwcDeploymentClass->BuildAttributeName(GetOutputAttributesPrefix() + "VarPartLabel" +
								    ivIndexAttribute->GetName()));

	varPartRule = new KWDRSymbolValueAt;
	varPartRule->DeleteAllOperands();

	dgAttribute->SetDerivationRule(varPartRule);

	partIndexOperand1 = new KWDerivationRuleOperand;
	partIndexOperand1->SetType(KWType::Structure);
	partIndexOperand1->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	partIndexOperand1->SetAttributeName(ivVarPartLabelsAttribute->GetName());
	varPartRule->AddOperand(partIndexOperand1);

	partIndexOperand2 = new KWDerivationRuleOperand;
	partIndexOperand2->SetType(KWType::Continuous);
	partIndexOperand2->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	partIndexOperand2->SetAttributeName(ivIndexAttribute->GetName());
	varPartRule->AddOperand(partIndexOperand2);

	dgAttribute->SetUsed(false);
	kwcDeploymentClass->InsertAttribute(dgAttribute);
	return dgAttribute;
}
// ##
