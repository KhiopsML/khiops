// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWRecodingSpec
//    Recoding parameters
class KWRecodingSpec : public Object
{
public:
	// Constructeur
	KWRecodingSpec();
	~KWRecodingSpec();

	// Copie et duplication
	void CopyFrom(const KWRecodingSpec* aSource);
	KWRecodingSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Keep informative variables only
	boolean GetFilterAttributes() const;
	void SetFilterAttributes(boolean bValue);

	// Max number of filtered variables
	int GetMaxFilteredAttributeNumber() const;
	void SetMaxFilteredAttributeNumber(int nValue);

	// Keep initial categorical variables
	boolean GetKeepInitialSymbolAttributes() const;
	void SetKeepInitialSymbolAttributes(boolean bValue);

	// Keep initial numerical variables
	boolean GetKeepInitialContinuousAttributes() const;
	void SetKeepInitialContinuousAttributes(boolean bValue);

	// Categorical recoding method
	const ALString& GetRecodeSymbolAttributes() const;
	void SetRecodeSymbolAttributes(const ALString& sValue);

	// Numerical recoding method
	const ALString& GetRecodeContinuousAttributes() const;
	void SetRecodeContinuousAttributes(const ALString& sValue);

	// Pairs recoding method
	const ALString& GetRecodeBivariateAttributes() const;
	void SetRecodeBivariateAttributes(const ALString& sValue);

	// Recode using prob distance (expert)
	boolean GetRecodeProbabilisticDistance() const;
	void SetRecodeProbabilisticDistance(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	//////////////////////////////////////////////////////////////////////////////
	// Description fine des parametres
	//
	// MaxFilteredAttributeNumber
	//  Apres pretraitements, la liste des attributs selectionnees est tronquees en supprimant
	//  les moins importants
	//  Par defaut: 0 (signifie pas de maximum)

	// SymbolRecodingMethod: applique sur la base du pretraitement de discretization/groupement de valeurs
	// pour les variables Symbol ou les paires de variables
	//   part Id: identifiant de partie (intervalle ou groupe)
	//   part Label: libelle de partie (intervalle ou groupe)
	//   0-1 binarization: recodage disjonctif complet de l'identifiant de partie
	//                    (autant d'attributs que de parties sources)
	//   conditional info: information conditionnelle de la source sachant la cible (-log(P(X|Y))
	//                      (autant d'attributs que de parties cibles)
	//
	// ContinuousRecodingMethod: comme pour SymbolRecodingMethod, avec des methodes supplementaires dans
	// le cas Continuous, basees sur le calcul des statistiques descriptives des variables continues
	//   center-reduction: (valeur-moyenne)/variance
	//   0-1 normalization: (valeur-min)/(max-min)
	//   rank normalization: rang moyen de la valeur, normalise entre 0 et 1

	// Verification des parametres
	boolean CheckMaxFilteredAttributeNumber(int nValue) const;
	boolean CheckSymbolRecodingMethod(const ALString& sValue) const;
	boolean CheckContinuousRecodingMethod(const ALString& sValue) const;
	boolean CheckBivariateRecodingMethod(const ALString& sValue) const;

	// Verification globale
	boolean Check() const override;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bFilterAttributes;
	int nMaxFilteredAttributeNumber;
	boolean bKeepInitialSymbolAttributes;
	boolean bKeepInitialContinuousAttributes;
	ALString sRecodeSymbolAttributes;
	ALString sRecodeContinuousAttributes;
	ALString sRecodeBivariateAttributes;
	boolean bRecodeProbabilisticDistance;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWRecodingSpec::GetFilterAttributes() const
{
	return bFilterAttributes;
}

inline void KWRecodingSpec::SetFilterAttributes(boolean bValue)
{
	bFilterAttributes = bValue;
}

inline int KWRecodingSpec::GetMaxFilteredAttributeNumber() const
{
	return nMaxFilteredAttributeNumber;
}

inline void KWRecodingSpec::SetMaxFilteredAttributeNumber(int nValue)
{
	nMaxFilteredAttributeNumber = nValue;
}

inline boolean KWRecodingSpec::GetKeepInitialSymbolAttributes() const
{
	return bKeepInitialSymbolAttributes;
}

inline void KWRecodingSpec::SetKeepInitialSymbolAttributes(boolean bValue)
{
	bKeepInitialSymbolAttributes = bValue;
}

inline boolean KWRecodingSpec::GetKeepInitialContinuousAttributes() const
{
	return bKeepInitialContinuousAttributes;
}

inline void KWRecodingSpec::SetKeepInitialContinuousAttributes(boolean bValue)
{
	bKeepInitialContinuousAttributes = bValue;
}

inline const ALString& KWRecodingSpec::GetRecodeSymbolAttributes() const
{
	return sRecodeSymbolAttributes;
}

inline void KWRecodingSpec::SetRecodeSymbolAttributes(const ALString& sValue)
{
	sRecodeSymbolAttributes = sValue;
}

inline const ALString& KWRecodingSpec::GetRecodeContinuousAttributes() const
{
	return sRecodeContinuousAttributes;
}

inline void KWRecodingSpec::SetRecodeContinuousAttributes(const ALString& sValue)
{
	sRecodeContinuousAttributes = sValue;
}

inline const ALString& KWRecodingSpec::GetRecodeBivariateAttributes() const
{
	return sRecodeBivariateAttributes;
}

inline void KWRecodingSpec::SetRecodeBivariateAttributes(const ALString& sValue)
{
	sRecodeBivariateAttributes = sValue;
}

inline boolean KWRecodingSpec::GetRecodeProbabilisticDistance() const
{
	return bRecodeProbabilisticDistance;
}

inline void KWRecodingSpec::SetRecodeProbabilisticDistance(boolean bValue)
{
	bRecodeProbabilisticDistance = bValue;
}

// ## Custom inlines

// ##
