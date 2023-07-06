// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDiscretizerMODL.h"
#include "MHMODLHistogramAnalysisStats.h"
#include "MHFloatingPointFrequencyTableBuilder.h"
#include "MHMODLHistogramCost.h"
#include "MHHistogramSpec.h"
#include "MHHistogram.h"
#include "MHMODLHistogramVector.h"
#include "MHContinuousLimits.h"

//////////////////////////////////////////////////////////////////////////////////
// Construction d'histogramme en representation virgule flottante
class MHDiscretizerMODLHistogram : public KWDiscretizerMODL
{
public:
	// Constructeur
	MHDiscretizerMODLHistogram();
	~MHDiscretizerMODLHistogram();

	// Nom de l'algorithme
	const ALString GetName() const;

	// Constructeur generique
	KWDiscretizer* Create() const;

	// Utilisation des valeurs sources pour la discretisation. Ici: true
	boolean IsUsingSourceValues() const override;

	// Methode principale de la classe ancetre reimplementee, pour integration dans Khiops
	// Apres l'appel a la methode, le tableau des histogrammes optimises est disponible
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget, ContinuousVector*& cvBounds) const override;

	// Acces aux resultats de discretisation non supervise MODL
	// Les histogrammes optimises sont ordonnes par niveau hierarchique croissants
	// Plus eventuellement un dernier histogramme "raw", en derniere position
	// Renvoie NULL si aucun histogramme na ete construit
	KWMODLHistogramResults* BuildMODLHistogramResults() const override;

	// Creation generique d'une objet de resultats de discretisation non supervise MODL
	// Memoire: l'objet retourne appartient a l'appelant
	KWMODLHistogramResults* CreateMODLHistogramResults() const override;

	// Acces a un objet permettant de gerer la serialisation des resultats de discretisation
	// Memoire: l'objet retourne appartient a l'appele
	const PLSharedObject* GetMODLHistogramResultsSharedObject() const override;

	// Parametres globaux de l'algorithme de construction des histogrammes
	static MHHistogramSpec* GetHistogramSpec();

	// Methode de calcul direct d'un histogramme a partir d'une liste de bins elementaires tries
	// ([lower value, upper value], frequency)
	// On a trois vecteurs en entree:
	//  . cvSourceBinLowerValues
	//  . cvSourceBinUpperValues
	//  . ivSourceBinFrequencies
	// Si cvSourceBinLowerValues=NULL, on est dans le cas d'une initialisation par valeur et on
	// considere les bornes inf egales aux bornes sup
	// Si ivSourceBinFrequencies=NULL, on utilise un effectif de 1 partout
	void ComputeHistogramFromBins(const ContinuousVector* cvSourceBinLowerValues,
				      const ContinuousVector* cvSourceBinUpperValues,
				      const IntVector* ivSourceBinFrequencies) const;

	/////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Acces en friend pour la sous-classe dediee troncature
	friend class MHDiscretizerTruncationMODLHistogram;

	// Methode specifique de creation d'un FrequencyTable
	KWFrequencyTable* NewFrequencyTable() const;

	// Methode de creation d'un frequencyTableBuilder
	virtual MHFloatingPointFrequencyTableBuilder* NewFrequencyTableBuilder() const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes principales de discretisation
	// Les vecteurs de valeurs en entree sont supposes tries et sans valeur manquante
	//
	// Les methodes principales gerent des Histogram complets en exploitant la
	// classe MHHistogram. Cette representation utilisateur gere les valeurs,
	// les effectifs, les bornes coupees optimalement..., et est adaptee
	// a l'exploitation par l'utilisateur
	//
	// La plupart des methodes interne travaillent avec des HistogramFrequencyTable,
	// exploitant la classe KWFrequencyTable. Cette representation interne ne prend en
	// compte que des paires (Frequency, BinNumber) par intervalle
	// (voir plus loin pour le codage reutilisant KWFrequencyTable), et est adaptee
	// aux algorithmes d'optimisation (impose par la classe mere KWDiscretizerMODL)

	// Methode principale de pilotage de la construction des histogrammes
	// prenant en compte l'ensemble du parametrage, sauf l'heuristique de troncature
	// Cette methode produit en sortie un histogramme optimise, et eventuellement
	// un deuxieme histogramme post-traite apres application de contraintes utilisateurs,
	// comme la gestion de la troncature ou une granularite max.
	// Ce deuxieme histogramme est mis a NULL s'il n'y avait pas de contraintes
	// utilisateur ou s'il ne differe pas de l'histogramme optimise
	// Enfin, si le parametre GetMaxCoarsenedHistogramNumber est strictement positif,
	// le troisieme parametre contient un tableau d'histogramme simplifies.
	// Tous les histogrammes en sortie sont a detruire par l'appelant,
	// mais pas le tableau qui doit etre cree et vide en entree
	//
	// Cette methode initialise le frequencyTableBuilder qui sera utilise par les
	// methode d'optimisation de l'histogramme
	virtual void MainDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
					const ContinuousVector* cvSourceBinUpperValues,
					const IntVector* ivSourceBinFrequencies, MHHistogram*& optimizedHistogram,
					MHHistogram*& postprocessedOptimizedHistogram,
					ObjectArray* oaCoarsenedHistograms) const;

	// Optimisation de l'exposant du central bin
	// On prend l'exposant utilisateur si demande
	void OptimizeCentralBinExponent(MHHistogram*& optimizedHistogram, MHHistogram*& postprocessedOptimizedHistogram,
					ObjectArray* oaCoarsenedHistograms) const;

	// Test si un histogramme comporte des intervalles pouvant separer les meme valeurs en changeant l'exposant du
	// central bin
	boolean IsHistogramConsistentWithCentralBinExponent(const MHHistogram* histogram) const;

	// Etude de l'optimisation de l'exposant du central bin
	void StudyOptimizeCentralBinExponent() const;

	// Optimisation de la granularite
	// On prend la granularite max utilisateur si demande
	// On s'arrete a une granularite maximale si l'heuristique de troncature est active
	void OptimizeGranularity(MHHistogram*& optimizedHistogram, MHHistogram*& postprocessedOptimizedHistogram,
				 ObjectArray* oaCoarsenedHistograms) const;

	// Ecriture d'un jeu de donnees vers un fichier
	void ExportData(const ContinuousVector* cvSourceBinLowerValues, const ContinuousVector* cvSourceBinUpperValues,
			const IntVector* ivSourceBinFrequencies, const ALString& sFileName) const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires

	// Export de l'ensemble des histogramme resultats
	// Cette methode est parametree par le parametre GetExportResultHistograms de histogramSpec
	void ExportResultHistograms(const ALString& sExportParameter) const;

	// Effectif total d'un histogramme algorithmique
	int ComputeHistogramTotalFrequency(const KWFrequencyTable* histogramFrequencyTable) const;

	// Ecriture des caracteristiques du jeu de donnees et d'un histogramme algorithmique
	void WriteHistogramFrequencyTable(const ALString& sTitle, const KWFrequencyTable* histogramFrequencyTable,
					  ostream& ost) const;
	void WriteHistogramFrequencyTableFile(const ALString& sTitle, const KWFrequencyTable* histogramFrequencyTable,
					      const ALString& sFileName) const;

	// Ecriture des carracteristiques du jeu de donnees et d'un histogramme
	void WriteHistogram(const ALString& sTitle, const MHHistogram* histogram, ostream& ost) const;
	void WriteHistogramFile(const ALString& sTitle, const MHHistogram* histogram, const ALString& sFileName) const;

	// Construction d'un histogramme en sortie a partir d'un histogramme algorithmique
	void BuildOutputHistogram(const KWFrequencyTable* histogramFrequencyTable, MHHistogram* outputHistogram) const;

	// Finalisation de l'histogramme, avec ajustement des bornes
	// Methode appelle une seule fois, redefinissable
	virtual void FinalizeHistogram(MHHistogram* outputHistogram) const;

	// Ajustement des bornes des histogramme par rapport au valeurs, pour passer a la limite de precision des
	// Continuous et garder des bornes separant les instances dans des intervalles de mêmes effectifs
	void AdjustHistogramIntervalBounds(const MHFloatingPointFrequencyTableBuilder* tableBuilder,
					   MHHistogram* outputHistogram) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de gestion de la trace du deroullement des algorithme
	// Ces methodes ne sont actives que si les logs internes ont ete demandes
	// dans les specifications des histogrammes, sinon leur appel est sans effet
	// Les methodes de trace prennent en entree des chaines de caracteres sans fin de ligne

	// Ouverture et fermeture de la trace
	// Le nom du fichier de  trace dont le nom est defini dans HistogramSpec
	// Une ente est ecrite a l'ouverture, avec les specifications des histogrammes
	void TraceOpen() const;
	void TraceClose() const;

	// Test si trace ouverte
	boolean IsTraceOpened() const;

	// Ajout d'une trace pour l'entree ou la sortie d'un bloc algorithmique
	void TraceBeginBlock(const ALString& sBlockName) const;
	void TraceEndBlock(const ALString& sBlockName) const;

	// Ajout d'une ligne de trace
	void Trace(const ALString& sLine) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes necessaire pour s'inserer le le frawework de discretisation de Khiops

	// Creation d'une table de contingence standard a partir d'un histogramme
	// On cree donc une table de contingence ne contenant les effectifs des intervalles et un vecteur de bornes
	// On ajoute egalement si necessaire un intervalle pour les valeurs manquantes
	void BuildOutputFrequencyTableAndBounds(const MHHistogram* histogram, int nMissingValueNumber,
						KWFrequencyTable*& standardFrequencyTable,
						ContinuousVector*& cvBounds) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion des classes de travail pour les fusions et coupures d'intervalles
	// Reimplementation des methodes virtuelle de la classee ancetre, pour
	// gerer les specificite des vecteurs d'effectif dedie aux grilles

	// Initialisation et controle d'integrite d'un vecteur d'effectif
	void InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const override;
	boolean CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const override;

	// Redefinition de la methode virtuelle, pour parametrer correctement les couts de discretisatiun
	//    . TotalInstanceNumber de la table (pour corriger la solution "bidulique")
	//    . PartileNumber pour la granularite (provient du GranularizedValueNumber de la table)
	void InitializeWorkingData(const KWFrequencyTable* histogramFrequencyTable) const override;
	void CleanWorkingData() const override;

	// Operation de transfert d'effectifs lors de copie, fusions, coupures...
	// Les cout des intervalles sont maintenus en permanence lors de ces operations
	void AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				const KWFrequencyVector* kwfvAddedFrequencyVector) const override;
	void RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				   const KWFrequencyVector* kwfvRemovedFrequencyVector) const override;
	void MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
				      const KWFrequencyVector* kwfvMergedFrequencyVector1,
				      const KWFrequencyVector* kwfvMergedFrequencyVector2) const override;
	void MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					const KWFrequencyVector* kwfvMergedFrequencyVector1,
					const KWFrequencyVector* kwfvMergedFrequencyVector2,
					const KWFrequencyVector* kwfvMergedFrequencyVector3) const override;
	void SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				  KWFrequencyVector* kwfvNewFrequencyVector,
				  const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;
	void MergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
					KWFrequencyVector* kwfvSourceFrequencyVector2,
					const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;
	void MergeMergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
					     const KWFrequencyVector* kwfvSourceFrequencyVector2,
					     KWFrequencyVector* kwfvSourceFrequencyVector3,
					     const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const override;

	// Tableau des histogrammes optimises
	mutable ObjectArray oaResultHistograms;

	// Variable de travail pour la construction des histogrammes
	mutable MHFloatingPointFrequencyTableBuilder* frequencyTableBuilder;

	// Gestion de la trace
	mutable fstream fstTrace;
	mutable boolean bIsTraceOpened;
	mutable int nTraceBlockLevel;
	mutable boolean bIsTraceBlockStart;

	// Parametres globaux de construction des histogrammes
	static MHHistogramSpec histogramSpec;
};