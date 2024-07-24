// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWAttributePairsSpec;

#include "KWAttributePairName.h"
#include "KWClass.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"
#include "KWAttributeSubsetStats.h"
#include "KWAttributeStats.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWAttributePairsSpec
//    Variable pairs parameters
class KWAttributePairsSpec : public Object
{
public:
	// Constructeur
	KWAttributePairsSpec();
	~KWAttributePairsSpec();

	// Nombe max de paires d'attributs qui sera analyse
	int GetMaxAttributePairNumber() const;
	void SetMaxAttributePairNumber(int nValue);

	// Indicateur d'analyse de toutes les paires, dans la limite du nombre max
	boolean GetAllAttributePairs() const;
	void SetAllAttributePairs(boolean bValue);

	////////////////////////////////////////////////////////////////////////
	// Specification des paires d'attributs specifiques
	//
	// Ces paires sont specifiees independamment de la classe les contenant,
	// d'une part pour des raison de suplesse sur l'odre des specification,
	// d'autre part parce que certaines paires peuvent impliquer des variables
	// construites qui sont inconnnues du dictionnaire au moment de la specification.
	// Les paires doivent neanmoins etre valides, avec des noms de variable syntaxiquement corrects.
	// L'ordre des noms de variable est indifferent au moment de la specification, et
	// les doublons seront detectes quelque soit l'ordre de ces noms de variables
	// Une paire peut contenir une seule variable specifiee, ce qui signifie qu'il faut prendre
	// en compte toutes les paires impliquant cette variable.

	// Acces a la liste des paires de variables specifiques, specifiee par des objet KWAttributePairName
	// Memoire: appartient a l'appele
	ObjectArray* GetSpecificAttributePairs();

	// Import de paires depuis un fichier tabulaire a deux colonnes et sans entete
	// Les paires invalides ou redondantes du fichier sont ignorees avec un warning
	// Les paires valide sont concatenees en fin de tableau
	void ImportAttributePairs(const ALString& sFileName);

	// Export de toutes les paires vers un fichier tabulaire a deux colonnes et sans entete
	// Les paires invalides ou redondantes de la liste sont ignorees avec un warning
	void ExportAllAttributePairs(const ALString& sFileName);

	// Supression des paires en doubles, avec message de compte rendu si besoin
	// Les paires invalides ou redondantes de la liste sont supprimees sans warning
	void DeleteDuplicateAttributePairs();

	// Verification que le nombre de paires max est superieure ou egal au nombre de paires specifique
	// avec warning utilisateur
	void CheckAttributePairNumbers() const;

	// Borne du nombre max de paires de variables
	static const int nLargestMaxAttributePairNumber = 100000;

	////////////////////////////////////////////////////////////////////////
	// Selection des paires d'attributs a analyser effectivement
	//
	// Une fois specifiees et en utilisant la classe, les paires a analyser
	// effectivement sont calculees en ignorant avec warning les paires
	// impliquant des variables absentes de la classe.
	// Les paires specifique sont prioritaires par rapport a toutes les
	// paires si celles si-sont demandees. Si le nombre total de paires
	// depasse le max, on priorise en prenant d'abord les paires impliquant
	// les attributs de plus fort level dans le cas supervise, puis
	// par ordre lexicographique.
	// Les paires impliquant un attribut ayant une seule valeur son ignorees

	// Parametrage de la classe contenant les paires de variables a specifier
	// Optionnel au moment de la specification des paires de variables,
	// car la classe n'est pas necessairement connue ni complete a ce moment
	// Obligatoire au moment de la selection des paires a analyser
	void SetClassName(const ALString& sValue);
	const ALString& GetClassName() const;

	// Calcul du nombre maximal de paires d'attributs dont on demande l'analyse
	int GetMaxRequestedAttributePairNumber(const ALString& sTargetAttributeName) const;

	// Selection des paires d'attributs a analyser
	// Les paires sont verifiees prealablement, avec emission de warning pour les paires avec des attributs
	// inexistants En sortie, le tableau oaAttributePairStats contient les specifications des paires d'attributs
	// concernees sous la forme d'objets KWAttributePairStats dont seuls les noms des attributs de la paire sont
	// specifies Les stats univariees en entree permettent de prioriser les paire a analyser en cas de depassement
	// du max Memoire: le contenu du tableau resultat appartient a l'appelant
	void SelectAttributePairStats(const ObjectArray* oaAttributeStats, ObjectArray* oaAttributePairStats) const;

	// Selection des attributs participant potentiellement aux paires d'attributs a evaluer, en supprimant
	// les attributs ayant une seule valeur et en les triant par level decroissant dans le cas supervise
	// Ces attributs seront a la base de la selection des paires d'attributs
	// En sortie, le dictionnaire odAttributeStats contient tous les attributs accessible par nom
	// et le tableau oaAttributeStats contient les stats des attributs concernes (KWAttributeStats)
	// tri par priorite decroissante
	// Memoire: le contenu du tableau resultat appartient a l'appelant
	void SelectAttributeStatsForAttributePairs(const ObjectArray* oaAttributeStats,
						   ObjectDictionary* odAttributeStats,
						   ObjectArray* oaSelectedAttributeStats) const;

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
	int nMaxAttributePairNumber;
	boolean bAllAttributePairs;

	// Tableau des paires
	ObjectArray oaSpecificAttributePairs;

	// Nom de la classe contenant les paires de variables
	ALString sClassName;

	// Contexte de l'analyse en cours pour personnaliser les messages d'erreur
	// Le ContextType est "" s'il s'agit de la liste des variable specifique, est specialise
	// pour les operation portant sur un fichier d'import/export
	// Le numero de ligne est celui de la liste ou du fichier selon le contexte (0 si non utilise)
	// Li'index d'attribut est 1 ou 2 pour designer le premier ou le second attribut de la paire (0 si non utilise)
	ALString sContextLabel;
	longint lContextLineIndex;
	int nContextAttributeIndex;
};
