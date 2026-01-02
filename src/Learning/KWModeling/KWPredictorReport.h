// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorReport;
class KWPredictorSelectionReport;
class KWSelectedAttributeReport;

#include "KWLearningSpec.h"
#include "KWPredictor.h"

///////////////////////////////////////////////////////////////////////////////
// Rapport d'apprentissage
// Ces resultat sont alimentes par la methode Train des predicteurs
class KWPredictorReport : public KWLearningReport
{
public:
	// Constructeur
	KWPredictorReport();
	~KWPredictorReport();

	// Nom du predicteur
	void SetPredictorName(const ALString& sValue);
	const ALString& GetPredictorName() const;

	// Nombre d'attributs utilises par le predictor
	void SetUsedAttributeNumber(int nValue);
	int GetUsedAttributeNumber() const;

	///////////////////////////////////////////////////////
	// Ecriture de rapports
	// Methodes supplementaires a celle de KWLearningReport

	// Rapport global: rapport tabulaire, puis detaille
	// Ecriture d'un rapport
	void WriteFullReportFile(const ALString& sFileName, ObjectArray* oaTrainReports);
	virtual void WriteFullReport(ostream& ost, ObjectArray* oaTrainReports);

	// Redefinition des methodes d'ecriture des rapports
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;
	void WriteReport(ostream& ost) const override;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture JSON du contenu d'un rapport global
	virtual void WriteJSONFullReportFields(JSONFile* fJSON, const ObjectArray* oaTrainReports) const;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	// Verification de la coherence d'un ensemble d'evaluation
	//  (meme probleme d'apprentissage)
	boolean CheckTrainReports(const ObjectArray* oaTrainReports) const;

	// Criteres de tri suivant le nom du predicteur
	const ALString GetSortName() const override;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Caracteristiques du predicteur
	ALString sPredictorName;
	int nUsedAttributeNumber;
};

///////////////////////////////////////////////////////////////////////////////
// Rapport d'apprentissage, avec selection d'attributs
// Ces resultat sont alimentes par la methode Train des predicteurs
class KWPredictorSelectionReport : public KWPredictorReport
{
public:
	// Constructeur
	KWPredictorSelectionReport();
	~KWPredictorSelectionReport();

	// Gestion des attributs selectionnes (classe KWSelectedAttributeReport)
	// Memoire: le tableau et son contenu appartiennent a l'appele
	ObjectArray* GetSelectedAttributes();

	// Ecriture d'un rapport detaille du predicteur, avec ses attributs selectionnes
	void WriteReport(ostream& ost) const override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tableau des attributs selectionnes
	ObjectArray oaSelectedAttributes;
};

//////////////////////////////////////////////////////////////////////////////
// Informations sur un attribut selectionne par un predicteur
//
// Chaque variable selectionnee par un predicteur a un nom d'attribut prepare
// et un nom d'attribut natif, que les variables soient natives ou issues
// de la construction de variable en multi-table ou via des arbres ou des textes.
//
// Dans le cas specifique d'une paire, il n'y a pas de variable explicitement cree
// pour la paire, le predicteur exploitant directement les valeurs de chaque variable
// de la paire via son modele de preparation bivariee.
// On genere neanmoins un nom de variable native "virtuel" pour designer chaque paire.
// Et on explicite egalement le fait qu'il s'agit d'une paire, ainsi que le nom de
// chaque variable de la paire
class KWSelectedAttributeReport : public KWLearningReport
{
public:
	// Constructeur
	KWSelectedAttributeReport();
	~KWSelectedAttributeReport();

	// Nom de l'attribut prepare
	void SetPreparedAttributeName(const ALString& sName);
	const ALString& GetPreparedAttributeName() const;

	// Nom de l'attribut source representant la variable
	void SetNativeAttributeName(const ALString& sName);
	const ALString& GetNativeAttributeName() const;

	// Specification du cas paire de variable (defaut: false)
	void SetPair(boolean bValue);
	boolean GetPair() const;

	// Nom du premier attribut de la paire, dans le cas d'une paire
	void SetNativeAttributeName1(const ALString& sName);
	const ALString& SetNativeAttributeName1() const;

	// Nom du second attribut de la paire, dans le cas d'une paire
	void SetNativeAttributeName2(const ALString& sName);
	const ALString& SetNativeAttributeName2() const;

	// Evaluation univariee de l'attribut: apport de l'attribut seul
	void SetUnivariateEvaluation(double dValue);
	double GetUnivariateEvaluation() const;

	// Poids de l'attribut
	void SetWeight(double dValue);
	double GetWeight() const;

	// Importance de l'attribut, combinaison de l'evaluaton univariee et du poids
	void SetImportance(double dValue);
	double GetImportance() const;

	// Criteres de tri permettant de trier differents objets d'un rapport
	const ALString GetSortName() const override;
	double GetSortValue() const override;

	// Redefinition de la methode de comparaison, en se basant sur l'importance, puis le level, puis le nom
	int CompareValue(const KWLearningReport* otherReport) const override;

	// Rapport synthetique pour un tableau de stats
	// On determine d'apres le contenu quels sont les champs a afficher
	// (au minimum: RecodingAttributeName et NativeAttributeName, les autres
	// champs n'etant affiches que s'ils sont utilises (au moins deux valeurs distinctes))
	void WriteArrayLineReport(ostream& ost, const ALString& sTitle,
				  const ObjectArray* oaLearningReports) const override;

	// Rapport synthetique destine a rentrer dans un tableau
	// Tous les champs sont affiches
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const override;

	// Verification de l'integrite
	boolean Check() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sPreparedAttributeName;
	ALString sNativeAttributeName;
	ALString sNativeAttributeName1;
	ALString sNativeAttributeName2;
	double dUnivariateEvaluation;
	double dWeight;
	double dImportance;
	boolean bPair;
};
