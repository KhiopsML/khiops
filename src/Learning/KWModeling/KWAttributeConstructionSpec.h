// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KDConstructionDomain.h"
#include "KDTextFeatureSpec.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KWAttributePairsSpec.h"

#ifdef DEPRECATED_V10
#include "KWRecodingSpec.h"
#endif // DEPRECATED_V10

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

#ifdef DEPRECATED_V10
	// Only pairs with variable (deprecated)
	const ALString& GetMandatoryAttributeInPairs() const;
	void SetMandatoryAttributeInPairs(const ALString& sValue);

	// Build recoding dictionary (deprecated)
	boolean GetRecodingClass() const;
	void SetRecodingClass(boolean bValue);

	// DEPRECATED V10: champ obsolete, conserve de facon cachee en V10 pour compatibilite ascendante des scenarios
	// Parametres de recodage
	// Ces parametres sont maintenant geres dans l'onglet de recodage
	// On va pointer ici vers le meme objet edite, gere par l'appelant
	KWRecodingSpec* DEPRECATEDGetRecodingSpec();
	void DEPRECATEDSetRecodingSpec(KWRecodingSpec* spec);

	// DEPRECATED V10: champ obsolete, conserve de facon cachee en V10 pour compatibilite ascendante des scenarios
	void DEPRECATEDSetSourceSubObjets(KWAttributeConstructionSpec* source);
#endif // DEPRECATED_V10

	// Ecriture de rapport lignes sur les specification du classifier
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	// Borne du nombre max de variables construites
	static const int nLargestMaxConstructedAttributeNumber = 100000;

	// Borne du nombre max de variables de type texte
	static const int nLargestMaxTextFeatureNumber = 200000;

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

#ifdef DEPRECATED_V10
	ALString sMandatoryAttributeInPairs;
	boolean bRecodingClass;

	// DEPRECATED V10: champ obsolete, conserve de facon cachee en V10 pour compatibilite ascendante des scenarios
	// Parametres de recodage
	KWRecodingSpec* DEPRECATEDrecodingSpec;

	// DEPRECATED V10: memorisation de l'objet edite source, pour que les onglets obsolete editent les nouveaux
	// sous-objets
	KWAttributeConstructionSpec* DEPRECATEDSourceSubObjets;
#endif // DEPRECATED_V10
};

////////////////////////////////////////////////////////////
// Implementations inline

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

#ifdef DEPRECATED_V10
inline const ALString& KWAttributeConstructionSpec::GetMandatoryAttributeInPairs() const
{
	return sMandatoryAttributeInPairs;
}

inline void KWAttributeConstructionSpec::SetMandatoryAttributeInPairs(const ALString& sValue)
{
	sMandatoryAttributeInPairs = sValue;
}

inline boolean KWAttributeConstructionSpec::GetRecodingClass() const
{
	return bRecodingClass;
}

inline void KWAttributeConstructionSpec::SetRecodingClass(boolean bValue)
{
	bRecodingClass = bValue;
}
#endif // DEPRECATED_V10
