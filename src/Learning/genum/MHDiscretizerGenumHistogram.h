// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDiscretizerMODL.h"
#include "MHGenumHistogramCosts.h"
#include "MHEnumHistogramCost.h"
#include "MHKMHistogramCost.h"
#include "MHGenumHistogramSpec.h"
#include "MHDataSubset.h"
#include "MHGenumHistogram.h"
#include "MHContinuousLimits.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme EqualWidth de discretisation non supervises en intervalles
// de largeur egale (dont le nombre est parametre par MaxIntervalNumber)
class MHDiscretizerGenumHistogram : public KWDiscretizerMODL
{
public:
	// Constructeur
	MHDiscretizerGenumHistogram();
	~MHDiscretizerGenumHistogram();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWDiscretizer* Create() const override;

	// Utilisation des valeurs sources pour la discretisation. Ici: true
	boolean IsUsingSourceValues() const override;

	// Methode principale de la classe ancetre reimplementee
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget, ContinuousVector*& cvBounds) const override;

	// Parametres globaux de l'algorithme de construction des histogrammes
	static MHGenumHistogramSpec* GetHistogramSpec();

	// Methode de calcul direct d'un histogramme a partir d'une liste de valeurs tries
	void ComputeHistogramFromBins(const ContinuousVector* cvSourceValues, MHHistogram*& optimizedHistogram) const;

	/////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode specifique de creation d'un FrequencyTable
	KWFrequencyTable* NewFrequencyTable() const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes principales de discretisation
	// Les vecteurs de valeurs en entree sont supposes tries et sans valeur manquante
	//
	// Les methodes principales gerent des Histogram complets en exploitant la
	// classe MHHistogram. Cette representation utilisateur gere les valeurs,
	// les effectifs, les bornes coupees optimalement..., et est adaptee
	// a l'exploitation par l'utilisateur
	//
	// La plupart des methodes inyterne travaillent avec des HistogramFrequencyTable,
	// exploitant la classe KWFrequencyTable. Cette representation interne ne prend en
	// compte que des paires (Frequency, BinNumber) par intervalle
	// (voir plus loin pour le codage reutilisant KWFrequencyTable), et est adaptee
	// aux algorithmes d'optimisation (impose par la classe mere KWDiscretizerMODL)

	// Methode principale de pilotage de la construction des histogrammes
	// prenant en compte l'ensemble du parametrage, y compris la simplification eventuelle
	void MainDiscretizeValues(const ContinuousVector* cvSourceValues, MHHistogram*& optimizedHistogram) const;

	// Heuristique des gestion des outliers, s'appuyant sur la methode disponible dans le cas typique
	void OutlierDiscretizeValues(const ContinuousVector* cvSourceValues, MHHistogram*& optimizedHistogram) const;

	// Methode de construction des histogrammes dans le cas typique
	// Cette methode prend en compte les elements de parametrage suivant
	//  OptimalAlgorithm
	//  GranularizedModel
	//  TruncationManagementHeuristic
	void TypicalDiscretizeValues(const ContinuousVector* cvSourceValues,
				     KWFrequencyTable*& optimizedHistogramFrequencyTable) const;

	// Discretisation suivant un modele standard d'histogramme
	void StandardDiscretizeValues(const ContinuousVector* cvSourceValues,
				      KWFrequencyTable* initialHistogramFrequencyTable,
				      KWFrequencyTable*& optimizedHistogramFrequencyTable) const;

	// Discretisation suivant un modele granularise d'histogramme
	// C'est ici que l'on gere les limites de la precision numerique en exploitant
	// un nombre de partile max calcul par MHGHistogramSpec::ComputeMaxPartileNumber
	void GranularizedDiscretizeValues(const ContinuousVector* cvSourceValues,
					  KWFrequencyTable* initialHistogramFrequencyTable,
					  KWFrequencyTable*& optimizedHistogramFrequencyTable) const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes de gestion des outliers

	// Construction d'un histogramme sur la log-transformation des valeurs en entree
	// Cette methode calcul de facon intermediaire la log-transformation des valeurs en entree
	// puis renvoie un histogramme sur ce domaine de representation
	// Les frontiere des intervalles etant definie par leur bornes, on peut alors en deduire
	// les frontiere dans l'espace initial
	void DiscretizeLogTrValues(const ContinuousVector* cvSourceValues,
				   KWFrequencyTable*& optimizedLogTrHistogramFrequencyTable) const;

	// Simplification d'un histogramme construit sur la log-transformation des valeurs en
	// le partitionnant en un nombre minimal d'intervalles correspondant chacun a un
	// sous ensemble de valeurs, soit PWCH, soit PICH
	// Alimente en sortie un tableau de sous-ensemble de donnees (MHDataSubset)
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	void SimplifyLogTrHistogram(const ContinuousVector* cvSourceValues,
				    const KWFrequencyTable* optimizedLogTrHistogramFrequencyTable,
				    ObjectArray* oaDataSubsets) const;

	// Decoupage si necessaire des sous-ensembles PICH restants issus des phase precedentes
	// Cela concerne les sous-ensembles correspondant a des intervalles de l'histogramme
	// construit sur la log-transformation des valeurs
	// Certains intervalles, de densite constante sur l'espace des log, peuvent etre trop
	// larges avec leur premier bins tres dense, ce qui les rend PICH
	// On les redecoupe alors pour obtenir des sous intervalles correspondant a des
	// sous-ensemble probablement PWCH, en utilisant l'hypothese de densite uniforme sur l'espace log
	// Le tableau de data subsets est potentiellement agrandi en sorti, avec les anciens
	// data subset PICH remplaces par une serie de nouveaux data subsets plus petits
	void SplitPICHLogTrDataSubsets(const ContinuousVector* cvSourceValues, ObjectArray* oaDataSubsets) const;

	// Decoupage d'un data subset PICH en une serie de data subsets plus petits
	void SplitPICHLogTrDataSubset(const ContinuousVector* cvSourceValues, const MHDataSubset* dataSubset,
				      ObjectArray* oaSplittedDataSubsets) const;

	// Construction de sous-histogrammes pour chaque sous ensemble et chaque frontiere
	// entre deux sous-ensembles adjacents d'une partition de l'ensemble initial
	// En sortie, les sous-histogrammes sont portes par les MHDataSubset du tableau en parametre
	void BuildSubHistograms(const ContinuousVector* cvSourceValues, const ObjectArray* oaDataSubsets) const;

	// Construction d'un histogramme par agregation d'histogrammes des sous-histogrammes
	// disponibles pour chaque sous-ensemble de donnees et sa frontiere
	void BuildAggregatedHistogram(const ContinuousVector* cvSourceValues, const ObjectArray* oaDataSubsets,
				      MHHistogram*& aggregatedHistogram) const;

	// Construction d'un histogramme pour la frontiere entre deux sous-ensemble de donnees
	// en considerant le dernier intervalle du premier sous-histogramme et le premier
	// sous-histogramme du second sous-histogramme
	int BuildBoundaryHistogram(const ContinuousVector* cvSourceValues, MHDataSubset* dataSubset1,
				   MHDataSubset* dataSubset2) const;

	// Parametrage des bornes entre deux intervalles adjacents de la frontiere entre sous-ensembles de donnees
	// En entree:
	//   . SourceValues: ensemble de toutes les valeurs
	//   . dataSubset: sous-ensemble de valeur de l'autre cote de la frontiere (uniquement pour les messages de
	//   debug) . BoundaryFrequencyDistance: indique la position de frontiere entre les deux intervalles par rapport
	//                                a la frontiere entre les sous-ensemble de donnees
	//   . PreviousEpsilonBinLength: epsilon-bin length du data subset precedent
	//   . EpsilonBinLength: epsilon-bin length du data subset
	//   . firstBoundaryInterval, secondBoundaryInterval: intervalles de la frontiere
	// En entree, on suppose que chaque intervalle est non vide et que ses valeurs inferieurs et superieures
	// sont correctement alimentees
	// En sortie, la borne superieur du premier intervalle et la borne inferieure du second intervalle
	// sont modifiees, en fonction de leur position par rapport a la frontiere, du epsilon-bin length a utiliser
	// et de leur densites respectives
	void ComputeBoundaryIntervalBounds(const ContinuousVector* cvSourceValues, MHDataSubset* dataSubset,
					   int nBoundaryFrequencyDistance, double dPreviousEpsilonBinLength,
					   double dEpsilonBinLength, MHHistogramInterval* firstBoundaryInterval,
					   MHHistogramInterval* secondBoundaryInterval) const;

	// Affichage du contebnau d'un tableau de sous ensembles de donnees
	void DisplayDataSubsetsArray(const ALString& sTitle, ObjectArray* oaDataSubsets, ostream& ost) const;

	// Test si un ensemble de donnees est bien ou mal conditionne pour les histogrammes
	// de taille TotalBinNumber (potentiellement different du parametre EpsilonBinNumber)
	// Ici, on ne s'interesse qu'au probleme des outlier. Le probleme de gestion des limites de
	// la precision numerique sera traite independamment dans chaque data subset dans GranularizedDiscretizeValues
	// Utilisation des criteres robustes:
	//   . PWCH: Practically Well Conditioned for Histograms
	//   . PICH: Probably Ill Conditioned for Histograms
	boolean IsDatasetPWCH(const ContinuousVector* cvSourceValues, int nTotalBinNumber) const;
	boolean IsDatasetPICH(const ContinuousVector* cvSourceValues, int nTotalBinNumber) const;

	// Test si un sous ensemble de donnes est bien ou mal conditionne pour les histogrammes
	// Le sous ensemble est defini par les valeurs tels que FirstIndex <= Index < LastIndex
	boolean IsDataSubsetPWCH(const ContinuousVector* cvSourceValues, int nFirstIndex, int nLastIndex,
				 int nTotalBinNumber) const;
	boolean IsDataSubsetPICH(const ContinuousVector* cvSourceValues, int nFirstIndex, int nLastIndex,
				 int nTotalBinNumber) const;

	// Calcul des parametre de log-transformation d'un jeu de donnees
	//   cMimM: min(-x) pour x < 0
	//   cMimP: min(x) pour x > 0
	//   cMimDeltaLogM: min(log(-x)-log(-xNext)) pour xNext < 0 et x < nextX
	//   cMimDeltaLogP: min(log(nextX)-log(x)) pour x > 0 et x < nextX
	void ComputeDataLogTrParameters(const ContinuousVector* cvSourceValues, Continuous& cMinM, Continuous& cMinP,
					Continuous& cMimDeltaLogM, Continuous& cMimDeltaLogP) const;

	// Fonction de log-transformation et son inverse
	Continuous LogTr(Continuous cX, Continuous cMinM, Continuous cMinP, Continuous cMimDeltaLogM,
			 Continuous cMimDeltaLogP) const;
	Continuous InvLogTr(Continuous cLogTrX, Continuous cMinM, Continuous cMinP, Continuous cMimDeltaLogM,
			    Continuous cMimDeltaLogP) const;

	// Calcul de la log transformation d'un vecteur de valeur
	void ComputeLogTrData(const ContinuousVector* cvSourceValues, ContinuousVector* cvLogTrValues) const;

	// Ecriture de la log-transformation d'un jeu de donnees vers un fichier
	// Les parametres InitialLabel et LogTrLabel specifient les libelles de l'entete
	// pour les donnnes dans leur version initiale ou log-transformees
	// Une colonne ne sera pas ecrite du tout si son libelle est vide
	void ExportLogTrData(const ContinuousVector* cvSourceValues, const ALString& sFileName,
			     const ALString& sInitialLabel, const ALString& sLogTrLabel) const;

	// Ecriture d'un jeu de donnees vers un fichier
	void ExportData(const ContinuousVector* cvSourceValues, const ALString& sFileName) const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes de gestion des valeurs tronquees

	// Post-optimisation d'une discretisation pour gerer les valeurs tronquee
	// La table de frequence en sortie est a NULL si il n'y a pas eu besoin de troncature
	// Sinon, on renvoie la nouvelle table optimisee suite au pretraitement de troncature
	// Elle se base donc sur un nombre TruncationBinNumber de bin (caculable par ComputeHistogramTotalBinNumber)
	// inferieur au EpsilonBinNumber des specifications des histogrammes
	void TruncationPostOptimization(const ContinuousVector* cvSourceValues,
					const KWFrequencyTable* histogramFrequencyTable,
					KWFrequencyTable*& postOptimizedlHistogramFrequencyTable) const;

	// Calcul d'un histogrammes pour les variations de valeurs
	// Permet d'identifier la presence de donnees tronquees, qui donneront lieu a des intervalles singuliers
	// Entree:
	//   . cvSourceValues: valeurs sources
	// Sorties:
	//   . cvSourceDeltaValues: vecteur des variations de valeur est alimente en sortie
	//   . optimizedDeltaValueHistogramFrequencyTable: histogramme optimal pour les variations de valeurs
	//   . nLargestNonEmptyIntervalBinLength: longueur en bin du plus grand intervalle non vide dans
	//                                        la table de contingence initiale des variations de valeur
	// Le dernier parametre permet de gerer proprement la longueur des intervalles singulier quand on atteint
	// les limites de la precision numerique
	void DiscretizeDeltaValues(const ContinuousVector* cvSourceValues, ContinuousVector* cvSourceDeltaValues,
				   KWFrequencyTable*& optimizedDeltaValueHistogramFrequencyTable,
				   int& nLargestNonEmptyIntervalBinLength) const;

	// Test pour decider s'il faut pretraiter de facon speciale les valeurs tronquees
	// On se base sur des statistiques de l'histogramme des valeurs et de l'histogramme de variation de valeurs
	// On renvoie 0 s'il n'y a pas de pretraitement particulier
	// Sinon, on renvoie la precision de la troncature (par exemple: 0.1)
	Continuous ComputeTruncationEpsilon(const ContinuousVector* cvSourceValues,
					    const KWFrequencyTable* histogramFrequencyTable) const;

	////////////////////////////////////////////////////////////////////////////////
	// Calcul de statistiques sur les histogrammes

	// Effectif total de l'histogramme
	int ComputeHistogramTotalFrequency(const KWFrequencyTable* histogramFrequencyTable) const;

	// Nombre total de bin elementaires des intervalles de l'histogramme
	int ComputeHistogramTotalBinNumber(const KWFrequencyTable* histogramFrequencyTable) const;

	// Nombre d'intervalles vides d'un histogramme
	int ComputeHistogramEmptyIntervalNumber(const KWFrequencyTable* histogramFrequencyTable) const;

	// Nombre d'intervalles singleton d'un histogramme
	int ComputeHistogramSingletonIntervalNumber(const ContinuousVector* cvSourceValues,
						    const KWFrequencyTable* histogramFrequencyTable) const;

	// Nombre d'intervalles singulier d'un histogramme
	int ComputeHistogramSingularIntervalNumber(const KWFrequencyTable* histogramFrequencyTable) const;

	// Test si un intervalle d'un histogramme est singulier
	boolean IsHistogramSingularInterval(const KWFrequencyTable* histogramFrequencyTable, int nIntervalIndex,
					    int nSingularIntervalBinLength) const;

	// Longueur en bin d'un intervalle singulier
	// Cette longueur depend de la granularite courante
	int ComputeSingularIntervalBinLength(const KWFrequencyTable* histogramFrequencyTable) const;

	// Longueur en bin du plus grand intervalle non vide
	// Dans le cas du traitement des valeurs Continuous aux limites de la precision numerique,
	// certain bin elementaires ne contenant qu'une seule valeur peuvent etre de longueur strictement
	// superieur a 1 si la longueur d'un bin elemenatire est plus petite que l'espace entre
	// deux valeur non separable
	// Par exemple, entre 0 et 10^-100, deux valeurs non separable, il peut y avoir plus de un bin
	int ComputeLargestNonEmptyIntervalBinLength(const KWFrequencyTable* histogramFrequencyTable) const;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires

	// Calcul de l'index de bin d'une valeur pour un point de depart et un epsilon donnes
	// On fournit egalement la premiere et la derniere valeur attendue ainsi que le nombre total de bins, pour gerer
	// correctement les problemes de precision numerique et assurer que les valeurs extremes correspondent au
	// premier et dernier bin
	int ComputeBinIndex(Continuous cX, double dBinStart, double dEpsilonBinLength, Continuous cMinValue,
			    Continuous cMaxValue, int nTotalBinNumber) const;
	int ComputeNextBinIndex(Continuous cX, double dBinStart, double dEpsilonBinLength, Continuous cMinValue,
				Continuous cMaxValue, int nTotalBinNumber) const;

	// Preparation d'une table de contingence dediee a l'optimisation des histogrammes
	// Solution "bidulique" pour reutiliser les structures et algorithmes de discretisation
	// On utilise une table de contingence avec autant de lignes que d'intervalles elementaires
	// en memorisant l'effectif de l'intervalle dans la premiere colonne et sa longueur
	// en epsilon bin dans la deuxieme colonne
	// On utilise egalement le parametre GranularizedValueNumber des KWFrequencyTable pour
	// memoriser le nombre de partiles courant pour les modeles granularises
	//
	// On pre-optimize la table en creant au plus un intervalle vide entre deux valeurs
	// Comme dans Kontkanen et Myllymaki, on fait demarrer le premier bin a MinValue-epsilon/2
	// Entrees:
	//  . cvSourceValues: vecteur trie de toutes les valeurs sources (de taille n)
	//  . nTotalBinNumber: nombre total de bin elementaires a utiliser (E)
	// Sortie:
	//   . histogramme elementaire d'effectif total n et de nombre de bin total E
	// Dans le cas particulier d'une seule et unique valeur, on ignore le parametre
	// nTotalBinNumber pour traiter ce cas de facon specifique
	//
	// Aux limites de la precision numerique, on gardera un TotalBinNumber tres grand au lieu de le reduire.
	// En effet, la solution se basant sur une reduction du nombre de bin elementaires entraiet un effet de
	// bord dans les cas des faibles nombres de bin. Le premier et le dernier bin etant a moitie vides,
	// il sont deux fois moins denses que les autres bins, et sont isoles dans les histogrammes meme
	// dans le cas de distributions uniformes.
	// Donc, on utilise systematiquement une table initiale decoupee en le max de bin elementaires
	// Cela pose de nouveaux probleme aux limites de la precision numeriques:
	//  . les valeurs min et max du dataset peuvent etre confondues avec les bornes inf et sup
	//  . on peut crer des bin elementaires entre deux valeurs non separables
	// On resout ce probleme de la facon suivante:
	//  . utilisation des methodes ComputeClosestLowerBound et ComputeClosestUpperBound de
	//    MHContinuousLimits pour rechercher systematiquement les valeurs separables les plus
	//    proches de toute valeur Continuous
	//   (par exemple, il n'y a aucune valeur entre 10^-100 et 0 ou entre 0 et 10^100)
	//  . on initialise les bornes inf et sup en se decalant si necessaire d'une valeurs
	//    par rapport aux valeurs min et max du data set
	//    (methode ComputeHistogramLowerBound et ComputeHistogramUpperBound de MHHistogramSpec)
	//  . on cree les bin elementaires en mettant au moins deux valeurs par bin si elles ne sont
	//    pas separables (pour eviter d'avoir un intervalle singleton dont on ne pourrait definir
	//    une borne inf et sup)
	void ComputeInitialFrequencyTable(const ContinuousVector* cvSourceValues, int nTotalBinNumber,
					  KWFrequencyTable*& initialHistogramFrequencyTable) const;

	// Construction d'une table de contingence granularisee
	// Attention, quand quand on construit les partiles a partir des bin elementaires, on tient compte du fait
	// que dans certains cas, un bin elementaire non vide peut etre de longueur strictement superieures a 1.
	// Cela entraine des effets de bord complexes a gerer, mais cela assure une protection aux
	// limites de la precision numerique
	void BuildGranularizedFrequencyTable(const KWFrequencyTable* initialHistogramFrequencyTable, int nPartileNumber,
					     KWFrequencyTable*& granularizedHistogramFrequencyTable) const;

	// Ecriture des carracteristiques du jeu de donnees et d'un histogramme algorithmique
	void WriteHistogramFrequencyTable(const ALString& sTitle, const ContinuousVector* cvSourceValues,
					  const KWFrequencyTable* histogramFrequencyTable, ostream& ost) const;
	void WriteHistogramFrequencyTableFile(const ALString& sTitle, const ContinuousVector* cvSourceValues,
					      const KWFrequencyTable* histogramFrequencyTable,
					      const ALString& sFileName) const;

	// Ecriture des carracteristiques du jeu de donnees et d'un histogramme
	void WriteHistogram(const ALString& sTitle, const MHHistogram* histogram, ostream& ost) const;
	void WriteHistogramFile(const ALString& sTitle, const MHHistogram* histogram, const ALString& sFileName) const;

	// Exploitation des valeurs et de l'histogramme optimise
	//   Ecriture des caracteristiques de l'histogramme dans le fichier de log en parametre
	void ExploitHistogram(const MHHistogram* histogram) const;

	// Construction d'un histogramme en sortie a partir d'un histogramme algorithmique
	// On peut ici specifier l'utilisation d'une sous-partie des valeurs
	void BuildOutputHistogram(const ContinuousVector* cvSourceValues, int nFirstIndex, int nLastIndex,
				  const KWFrequencyTable* histogramFrequencyTable, MHHistogram* outputHistogram) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de gestion de la trace du deroullement des algorithme
	// Ces methodes ne sont actives que si les logs internes ont ont ete demandes
	// dans les specification des histogramme, sinon leu appel est sans effet
	// Les methodes de trace prennent en entree des chaines de caracteres sans fin de ligne

	// Ouverture et fermeture de la trace
	// Le nom du fichier de  trace dont le nom est defini dans MHHistogramSpec
	// Une ente est ecrite a l'ouverture, avec les specifications des histogrammes
	void TraceOpen() const;
	void TraceClose() const;

	// Ajout d'une trace pour l'entree ou la sortie d'un bloc algorithmique
	void TraceBeginBlock(const ALString& sBlockName) const;
	void TraceEndBlock(const ALString& sBlockName) const;

	// Ajout d'une ligne de trace
	void Trace(const ALString& sLine) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes necessaire pour s'inserer le frawework de discretisation de Khiops

	// Creation d'une table de contingence standard a partir d'un histogramme
	// On cree donc une table de contingence ne contenant que les effectifs des intervalles.
	// On supprime egalement les intervalles vides, car ils ne sont pas toleres par la methode appelante.
	// On ajoute egalement si necessaire un intervalle pour les valeurs manquantes
	// Dans une version integree a Khiops, il faudra pourvoir utiliser les valeurs pour calculer correctement
	// les bornes des intervalles, comme dans la methode WriteHistogram
	void PreparedCleanedStandardFrequencyTable(const MHHistogram* histogram, int nMissingValueNumber,
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
	//    . TotalBinNumber de la table (et non celui des specifications des histogrammes)
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

	// Gestion de la trace
	mutable fstream fstTrace;
	mutable boolean bIsTraceOpened;
	mutable int nTraceBlockLevel;
	mutable boolean bIsTraceBlockStart;

	// Parametres globaux de construction des histogrammes
	static MHGenumHistogramSpec histogramSpec;
};
