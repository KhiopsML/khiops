// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KDConstructionDomain.h"
#include "KDTextFeatureSpec.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KWAttributePairsSpec.h"

////////////////////////////////////////////////////////////
// Classe KWAttributeConstructionSpec
//    Feature engineering parameters
class KWAttributeConstructionSpec : public Object
{
public:
	// Constructeur
	KWAttributeConstructionSpec();
	~KWAttributeConstructionSpec();

	// Copie et duplication
	void CopyFrom(const KWAttributeConstructionSpec* aSource);
	KWAttributeConstructionSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Indicateur pour ne garder dans les rapport que les variables construites selectionnees par le predicteur SNB
	boolean GetKeepSelectedAttributesOnly() const;
	void SetKeepSelectedAttributesOnly(boolean bValue);

	// Max number of constructed variables
	int GetMaxConstructedAttributeNumber() const;
	void SetMaxConstructedAttributeNumber(int nValue);

	// Max number of text features
	int GetMaxTextFeatureNumber() const;
	void SetMaxTextFeatureNumber(int nValue);

	// Max number of trees
	int GetMaxTreeNumber() const;
	void SetMaxTreeNumber(int nValue);

	// Max number of variable pairs
	int GetMaxAttributePairNumber() const;
	void SetMaxAttributePairNumber(int nValue);

	// Parametres de construction de variables
	KDConstructionDomain* GetConstructionDomain();

	// Parametres des variables de type texte
	KDTextFeatureSpec* GetTextFeatureSpec();

	// Parametres de creation de variables (eg trees)
	// Peut renvoyer NULL
	KDDataPreparationAttributeCreationTask* GetAttributeCreationParameters();

	// Parametrage de l'analyse des paires de variables
	KWAttributePairsSpec* GetAttributePairsSpec();

	// Parametrage des familles de construction de variable dans les learningSpec
	// On parametre les familles non seulement specifiees, mais egalement effectivement utilisables
	// Pour la construction multi-table ou texte cela necessite un parametre specifique
	// Sinon, il suffit qu'il y ait au moins deux variables (construites ou non) pour les arbres ou les paires
	void SpecifyLearningSpecConstructionFamilies(KWLearningSpec* learningSpec,
						     boolean bIsMultiTableConstructionPossible,
						     boolean bIsTextConstructionPossible);

	// Ecriture de rapport lignes sur les specification du classifier
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	// Borne du nombre max de variables construites
	static const int nLargestMaxConstructedAttributeNumber = 100000;

	// Borne du nombre max de variables de type texte
	static const int nLargestMaxTextFeatureNumber = 100000;

	// Borne du nombre max d'arbres
	static const int nLargestMaxTreeNumber = 1000;

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	boolean bKeepSelectedAttributesOnly;
	int nMaxConstructedAttributeNumber;
	int nMaxTextFeatureNumber;
	int nMaxTreeNumber;

	// Parametres de construction de variable
	KDConstructionDomain constructionDomain;

	// Parametres des variables de type texte
	KDTextFeatureSpec textFeatureSpec;

	// Parametres de creation de variable (eg trees)
	KDDataPreparationAttributeCreationTask* attributeCreationTask;

	// Parametrage de l'analyse des paires de variables
	KWAttributePairsSpec attributePairsSpec;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWAttributeConstructionSpec::GetKeepSelectedAttributesOnly() const
{
	return bKeepSelectedAttributesOnly;
}

inline void KWAttributeConstructionSpec::SetKeepSelectedAttributesOnly(boolean bValue)
{
	bKeepSelectedAttributesOnly = bValue;
}

inline int KWAttributeConstructionSpec::GetMaxConstructedAttributeNumber() const
{
	return nMaxConstructedAttributeNumber;
}

inline void KWAttributeConstructionSpec::SetMaxConstructedAttributeNumber(int nValue)
{
	nMaxConstructedAttributeNumber = nValue;
}

inline int KWAttributeConstructionSpec::GetMaxTextFeatureNumber() const
{
	return nMaxTextFeatureNumber;
}

inline void KWAttributeConstructionSpec::SetMaxTextFeatureNumber(int nValue)
{
	nMaxTextFeatureNumber = nValue;
}

inline int KWAttributeConstructionSpec::GetMaxTreeNumber() const
{
	return nMaxTreeNumber;
}

inline void KWAttributeConstructionSpec::SetMaxTreeNumber(int nValue)
{
	nMaxTreeNumber = nValue;
}

inline int KWAttributeConstructionSpec::GetMaxAttributePairNumber() const
{
	return attributePairsSpec.GetMaxAttributePairNumber();
}

inline void KWAttributeConstructionSpec::SetMaxAttributePairNumber(int nValue)
{
	attributePairsSpec.SetMaxAttributePairNumber(nValue);
}
