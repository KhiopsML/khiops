// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWDRDataGrid.h"
#include "KWDataGridStats.h"
#include "CCCoclusteringReport.h"
#include "KWDRDataGridDeployment.h"
#include "CCPostProcessingSpec.h"
#include "KWClassDomain.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCDeploymentSpec
//    Deployment parameters
class CCDeploymentSpec : public Object
{
public:
	// Constructeur
	CCDeploymentSpec();
	~CCDeploymentSpec();

	// Copie et duplication
	void CopyFrom(const CCDeploymentSpec* aSource);
	CCDeploymentSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Input dictionary
	const ALString& GetInputClassName() const;
	void SetInputClassName(const ALString& sValue);

	// Input table variable
	const ALString& GetInputObjectArrayAttributeName() const;
	void SetInputObjectArrayAttributeName(const ALString& sValue);

	// Coclustering deployed variable
	const ALString& GetDeployedAttributeName() const;
	void SetDeployedAttributeName(const ALString& sValue);

	// Build predicted cluster variable
	boolean GetBuildPredictedClusterAttribute() const;
	void SetBuildPredictedClusterAttribute(boolean bValue);

	// Build inter-cluster distance variables
	boolean GetBuildClusterDistanceAttributes() const;
	void SetBuildClusterDistanceAttributes(boolean bValue);

	// Build frequency recoding variables
	boolean GetBuildFrequencyRecodingAttributes() const;
	void SetBuildFrequencyRecodingAttributes(boolean bValue);

	// Output variables prefix
	const ALString& GetOutputAttributesPrefix() const;
	void SetOutputAttributesPrefix(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Indique si l'on doit generer des attributs bases sur le deploiement de distribution de valeurs
	boolean GetBuildDistributionBasedAttributes() const;

	// Preparation d'un dictionnaire de deploiement de coclustering
	// Renvoie le domaine de dictionnaire correctement cree et initialise en cas de succes
	// Messages d'erreur en cas de probleme
	boolean PrepareCoclusteringDeployment(const CCHierarchicalDataGrid* coclusteringDataGrid,
					      KWClassDomain*& deploymentDomain);

	// Preparation d'un dictionnaire de deploiement de coclustering dans le cas individus * variables
	// Renvoie le domaine de dictionnaire correctement cree et initialise en cas de succes
	// Messages d'erreur en cas de probleme
	boolean PrepareVarPartCoclusteringDeployment(const CCHierarchicalDataGrid* coclusteringDataGrid,
						     KWClassDomain*& deploymentDomain);

	// Test d'integrite pour le deploiement, en fonction des specification et de la grille de coclustering,
	// du dictionnaire de deploiement et des methodes de deploiement demandees
	boolean CheckDeploymentSpec(const CCHierarchicalDataGrid* coclusteringDataGrid) const;

	// Test si un attribut est un attribut de distribution de valeurs compatible pour une deploiement de grille de
	// coclustering Pas de message d'erreur
	boolean CheckAttributeConsistencyWithPostProcessedSpec(const KWAttribute* kwaAttribute,
							       CCPostProcessingSpec* postProcessingSpec) const;
	boolean
	CheckAttributeConsistencyWithCoclusteringDataGrid(const KWAttribute* kwaAttribute,
							  const CCHierarchicalDataGrid* coclusteringDataGrid) const;

	// Test si un dictionnaire contient un attribut de distribution de valeurs compatible
	// pour une deploiement de grille de coclustering
	// Pas de message d'erreur
	boolean CheckClassConsistencyWithPostProcessedSpec(const KWClass* kwcClass,
							   CCPostProcessingSpec* postProcessingSpec) const;

	// Alimentation automatique des informations de dictionnaire et attribut
	// S'il y a generation d'attributs base sur des distributions, on tente de positionner une classe et un attribut
	// de distribution valides
	void FillInputClassAndAttributeNames(CCPostProcessingSpec* postProcessingSpec);

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sInputClassName;
	ALString sInputObjectArrayAttributeName;
	ALString sDeployedAttributeName;
	boolean bBuildPredictedClusterAttribute;
	boolean bBuildClusterDistanceAttributes;
	boolean bBuildFrequencyRecodingAttributes;
	ALString sOutputAttributesPrefix;

	// ## Custom implementation

	// Creation d'un attribut de type grille dans une classe de deploiement
	KWAttribute* AddDataGridAttribute(KWClass* kwcDeploymentClass,
					  const CCHierarchicalDataGrid* coclusteringDataGrid) const;

	// Creation d'un attribut de calcul du cluster parent
	KWAttribute* AddParentClusterIndexAttribute(KWClass* kwcDeploymentClass, KWAttribute* kwaDataGridAttribute,
						    const CCHDGAttribute* hdgDeploymentAttribute) const;

	// Creation d'un attribut de distribution des valeurs a partir d'une table secondaire
	KWAttribute* AddValueVectorAttributeAt(KWClass* kwcDeploymentClass, KWAttribute* kwaObjectArrayAttribute,
					       const CCHDGAttribute* hdgDistributionAttribute) const;

	// Creation d'un attribut de vecteur de frequence partir d'une table secondaire
	KWAttribute* AddFrequencyVectorAttribute(KWClass* kwcDeploymentClass, KWAttribute* kwaObjectArrayAttribute,
						 const ALString& sFrequencyAttributeName) const;

	// Creation d'un attribut de deploiement de grille, l'attribut vecteur de frequence etant optionnel (NULL si
	// absent)
	KWAttribute* AddDataGridDeploymentAttribute(KWClass* kwcDeploymentClass, KWAttribute* kwaDataGridAttribute,
						    ObjectArray* oaValueVectorAttributes,
						    KWAttribute* kwaFrequencyVectorAttribute,
						    const CCHDGAttribute* hdgDeploymentAttribute) const;

	// Creation d'un attribut de prediction d'index de la partie de deploiement
	KWAttribute* AddPredictedPartIndexAttribute(KWClass* kwcDeploymentClass,
						    KWAttribute* kwaDataGridDeploymentAttribute,
						    const CCHDGAttribute* hdgDeploymentAttribute) const;

	// Creation d'attributs de distance pour l'attribut de deploiement
	void AddPredictedPartDistancesAttributes(KWClass* kwcDeploymentClass,
						 KWAttribute* kwaDataGridDeploymentAttribute,
						 const CCHDGAttribute* hdgDeploymentAttribute) const;

	// Creation d'attributs d'effectif pour l'attribut de distribution
	void AddPredictedPartFrequenciesAttributesAt(KWClass* kwcDeploymentClass,
						     KWAttribute* kwaDataGridDeploymentAttribute,
						     const CCHDGAttribute* hdgDistributionAttribute) const;

	// Creation d'un attribut de libelle de partie, a partir d'un attribut d'index, d'un attribut de deploiement et
	// d'un prefix additionnel
	KWAttribute* AddPartLabelAttribute(KWClass* kwcDeploymentClass, KWAttribute* kwaIndexAttribute,
					   KWAttribute* kwaLabelVectorAttribute,
					   const CCHDGAttribute* hdgDeploymentAttribute, const ALString& sPrefix) const;

	// Creation d'un attribut de libelles pour un attribut de grille
	KWAttribute* AddLabelVectorAttributeAt(KWClass* kwcDeploymentClass, const CCHDGAttribute* hdgAttribute) const;

	// Methodes associees a la construction du didctionnaire de deploiement dans le cas VarPart
	// Creation des attributs associes aux innerVariables dans le cas d'un coclustering individus * variables
	KWAttribute* AddInnerAttributePartitionAttribute(KWClass* kwcDeploymentClass, KWDGAttribute* innerAttribute);

	KWAttribute* AddInnerAttributePartitionIndexAttribute(KWClass* kwcDeploymentClass,
							      KWAttribute* ivPartitionAttribute,
							      KWDGAttribute* innerAttribute);

	KWAttribute* AddInnerAttributeVarPartLabelsAttribute(KWClass* kwcDeploymentClass,
							     KWDGAttribute* innerAttribute);

	KWAttribute* AddInnerAttributePartitionLabelAttribute(KWClass* kwcDeploymentClass,
							      KWAttribute* ivVarPartLabelsAttribute,
							      KWAttribute* ivIndexAttribute);
	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCDeploymentSpec::GetInputClassName() const
{
	return sInputClassName;
}

inline void CCDeploymentSpec::SetInputClassName(const ALString& sValue)
{
	sInputClassName = sValue;
}

inline const ALString& CCDeploymentSpec::GetInputObjectArrayAttributeName() const
{
	return sInputObjectArrayAttributeName;
}

inline void CCDeploymentSpec::SetInputObjectArrayAttributeName(const ALString& sValue)
{
	sInputObjectArrayAttributeName = sValue;
}

inline const ALString& CCDeploymentSpec::GetDeployedAttributeName() const
{
	return sDeployedAttributeName;
}

inline void CCDeploymentSpec::SetDeployedAttributeName(const ALString& sValue)
{
	sDeployedAttributeName = sValue;
}

inline boolean CCDeploymentSpec::GetBuildPredictedClusterAttribute() const
{
	return bBuildPredictedClusterAttribute;
}

inline void CCDeploymentSpec::SetBuildPredictedClusterAttribute(boolean bValue)
{
	bBuildPredictedClusterAttribute = bValue;
}

inline boolean CCDeploymentSpec::GetBuildClusterDistanceAttributes() const
{
	return bBuildClusterDistanceAttributes;
}

inline void CCDeploymentSpec::SetBuildClusterDistanceAttributes(boolean bValue)
{
	bBuildClusterDistanceAttributes = bValue;
}

inline boolean CCDeploymentSpec::GetBuildFrequencyRecodingAttributes() const
{
	return bBuildFrequencyRecodingAttributes;
}

inline void CCDeploymentSpec::SetBuildFrequencyRecodingAttributes(boolean bValue)
{
	bBuildFrequencyRecodingAttributes = bValue;
}

inline const ALString& CCDeploymentSpec::GetOutputAttributesPrefix() const
{
	return sOutputAttributesPrefix;
}

inline void CCDeploymentSpec::SetOutputAttributesPrefix(const ALString& sValue)
{
	sOutputAttributesPrefix = sValue;
}

// ## Custom inlines

// ##
