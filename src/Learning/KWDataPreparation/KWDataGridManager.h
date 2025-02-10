// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClassStats.h"
#include "KWDataGrid.h"
#include "KWSortableIndex.h"
#include "KWAttributeStats.h"
#include "KWQuantileBuilder.h"
#include "KWFrequencyVector.h"

/////////////////////////////////////////////////////////////////////////////////////
// Gestion de grilles de donnees
// Copies et transferts partiels ou total du contenu d'une grille source
//  pour alimenter une grille cible.
// Les grilles sources et cible peuvent etre d'une sous-classe quelconque de KWDataGrid.
// Par contre, seules les parties gerees au niveau de KWDataGrid sont explicitement
// gerees (donc, le contenu de la grille en attributs, parties et cellules).
class KWDataGridManager : public Object
{
public:
	// Constructeur
	KWDataGridManager();
	~KWDataGridManager();

	// Copie du contenu d'une grille source vers une grille cible
	// Dans le cas VarPart, les attributs internes sont partages avec la grille source
	void CopyDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Copie profonde du contenu d'une grille source
	// Dans le cas VarPart, les attributs internes sont clones a partir de la grille source
	void CopyDataGridWithInnerAttributesCloned(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de transfert du contenu de la grille source vers la grille cible
	// Les methodes prennent en argument un grille cible initialement vide
	// Pour toutes les methodes de la classe, l'ordre des attributs exportes est
	// compatible avec celui des attributs initiaux de la grille de donnees source

	// Export total (attribut, parties et cellules)
	// Dans le cas VarPart, les attributs internes sont partages avec la grille source
	void ExportDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export total (attribut, parties et cellules)
	// Dans le cas VarPart, les attributs internes sont clones a partir de la grille source
	void ExportDataGridWithInnerAttributesCloned(const KWDataGrid* sourceDataGrid,
						     KWDataGrid* targetDataGrid) const;

	// Export total (attribut, parties et cellules)
	// Cas d'une grille de type VarPart
	// Les innerAttributes de la targetDataGrid sont ceux de la grille de reference
	// L'attribut VarPart est construit avec une PV par VarPartSet, les PV etant celles des innerAttributes de la referenceDataGrid
	// L'attribut Identifer est construit selon deux cas :
	//   - bSourceSimpleAttributeParts = true: on recopie les parties de la grille source
	//   - bSourceSimpleAttributeParts = false: on recopie les parties de la grille de reference
	void ExportDataGridWithSingletonVarParts(const KWDataGrid* sourceDataGrid, const KWDataGrid* referenceDataGrid,
						 KWDataGrid* targetDataGrid, boolean bSourceSimpleAttributeParts) const;

	// Export total (attribut, parties et cellules) en exploitant une grille de reference,
	// avec les parties de VarPart de la grille source, exploitant la partition de la grille de reference
	// Cette grille de reference n'a pas les memes VarParts que la grille source, alors que la grille en sortie
	// reutilise les VarPart de la grille source
	void ExportDataGridWithReferenceVarPartClusters(const KWDataGrid* sourceDataGrid, KWDataGrid* referenceDataGrid,
							KWDataGrid* targetDataGrid);

	// Export d'une grille terminale, avec attributs reduits a une seule partie
	// Ne modifie pas la tokenisation des attributs internes
	void ExportTerminalDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export de la grille du modele nul : une seule partie par attribut et, pour les attributs VarPart,
	// une seule partie de variable
	// Attention: creation de nouveaux attributs internes qui doivent etre detruits par l'appelant
	void ExportNullDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export total (attribut, parties et cellules)
	// Cas d'une grille de type VarPart
	// En entree :
	// DD 461 - sourceDataGrid : preciser en quoi elle differe d'inputDataGrid
	// - inputDataGrid : grille dont on souhaite sur-echantillonner le KWDGInnerAttributes
	// - referenceInnerAttributes : les innerAttributes avec la partition de reference la plus fine sur laquelle on procede au tirage aleatoire
	// - nTargetTokenNumber : nombre total de parties de variables souhaite sur l'ensemble des innerAttributes
	// En sortie :
	// targetDataGrid : nouvelle grille dont le KWDGInnerAttributes a ete sur-echantillonne
	// Les partitions des attributs Identifier et VarPart ne sont pas modifiees
	// Les innerAttributes sont sur-tokenises par tirage aleatoire
	// Le nombre de parties de variables obtenu peut etre inferieur a l'objectif nTargetTokenNumber
	void ExportDataGridWithRandomizedInnerAttributes(const KWDataGrid* sourceDataGrid,
							 const KWDataGrid* inputDataGrid,
							 const KWDGInnerAttributes* referenceInnerAttributes,
							 KWDataGrid* targetDataGrid, int nTargetTokenNumber);

	// Export total (attribut, parties et cellules)
	// Cas d'une grille de type VarPart
	// En entree :
	// DD 461 - sourceDataGrid : preciser en quoi elle differe d'inputDataGrid
	// - inputDataGrid : grille dont on souhaite fusionner le KWDGInnerAttributes
	// - otherMergedInnerAttributes : attributes dont les PV sont issues d'une fusion des innerAttributes de l'inputDataGrid.
	//							Les PV fusionnees doivent appartenir au meme cluster de PV
	// En sortie :
	// targetDataGrid : nouvelle grille dont le KWDGInnerAttributes a ete remplace par une version fusionnee
	// Les partitions des attributs Identifier et VarPart ne sont pas modifiees
	void ExportDataGridWithMergedInnerAttributes(const KWDataGrid* sourceDataGrid, const KWDataGrid* inputDataGrid,
						     const KWDGInnerAttributes* otherMergedInnerAttributes,
						     KWDataGrid* targetDataGrid);

	// CH Etape 2 Antecedent
	// Export total (attribut, parties et cellules)
	// Cas d'une grille de type VarPart
	// En entree :
	// DD 461 sourceDataGrid : en quoi differe-t'elle de inputDataGrid ?
	// DD 461 C'est tokenizedDataGrid qui joue le role de grille source dans les methodes d'Export ?
	// - inputDataGrid : grille en entree
	// En sortie :
	// targetDataGrid : nouvelle grille dont l'identifierAttribute et le KWDGInnerAttributes ne sont pas modifies
	// L'attribut VarPart contient un cluster par partie de variable du KWDGInnerAttributes
	void ExportReferenceDataGridWithGivenInnerAttributes(const KWDataGrid* sourceDataGrid,
							     const KWDataGrid* inputDataGrid,
							     const KWDataGrid* tokenizedDataGrid,
							     KWDataGrid* targetDataGrid);

	/////////////////////////////////////////////////////////////////////////////////////////
	// Service elementaires de transfert du contenu de la grille source vers la grille cible
	// dedies aux attributs, parties et cellules

	// Export des attributs uniquement (plus les specifications des classes cibles)
	// Dans le cas VarPart, les attributs internes sont partages avec la grille source
	void ExportAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export des attributs informatifs uniquement (non reduits a une seule partie)
	void ExportInformativeAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export des parties uniquement (les attributs de la grille cible sont deja exportes)
	// Les attributs cibles peuvent n'etre qu'un sous-ensemble des attributs sources
	void ExportParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Export des parties pour un attribut donne (devant exister dans la grille source et
	// dans la grille cible sans ses parties)
	void ExportAttributeParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
				  const ALString& sAttributeName) const;

	// Export des cellules uniquement (les attributs et parties de la grille cible sont deja exportes)
	// Les attributs cibles peuvent n'etre qu'un sous-ensemble des attributs sources
	// Les parties cibles peuvent former une partition quelconque des parties sources
	void ExportCells(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	///////////////////////////////////////////////////////////////////////////
	// Construction d'une grille de donnees cible en partitionnant aleatoirement
	// la grille de donnees source
	// Les methodes suivantes travaillent uniquement sur la structure des grilles
	// cibles, qui sont vides de cellules avant et apres l'appel de chaque methode.
	// Les methodes ExportRandom... creent une nouvelle grille par generation aleatoire
	// d'attributs et parties.
	// Les methodes AddRandom... contraignent la generation aleatoire par une
	// grille comportant les attributs et parties obligatoires

	// Export d'un sous-ensemble aleatoire des attributs (plus les specifications des classes cibles)
	void ExportRandomAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
				    int nAttributeNumber) const;

	// Export d'une partition aleatoire des parties pour chaque attribut cible.
	// Le nombre de parties effectif peut etre inferieur au nombre specifie, s'il n'y
	// a pas assez de valeurs sources. Le choix de la partition est uniforme
	// (partition des rangs pour les attributs numeriques, des valeurs pour les symboliques).
	void ExportRandomParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
			       int nMeanAttributePartNumber) const;

	// Export d'un sous-ensemble aleatoire des attributs (plus les specifications des classes cibles)
	// en partant d'un ensemble d'attributs obligatoires.
	void AddRandomAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
				 const KWDataGrid* mandatoryDataGrid, int nRequestedAttributeNumber) const;

	// Export d'une partition aleatoire des parties pour chaque attribut cible
	// en partant de partitions aleatoires pour un sous-ensemble d'attributs.
	// Les nouvelles parties sont obtenues en sur-partitionnant les partitions existantes,
	// pour atteindre les nombres de parties a ajouter demandees par type d'attribut.
	// Le nombre de partie a ajouter demande est aleatoire, avec au moins le pourcentage minimum demande.
	// Le nombre de parties reellement ajoutees peut etre inferieur a celui demande, s'il n'y
	// a pas assez de valeurs disponibles pour scinder des parties existantes
	void AddRandomParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
			    const KWDataGrid* mandatoryDataGrid, int nRequestedContinuousPartNumber,
			    int nRequestedSymbolPartNumber, double dMinPercentageAddedPart) const;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Construction d'une grille de donnees cible en granularisant la grille de donnees source

	// Initialisation du dictionnaire des quantile builders
	// Un quantile builder (Group ou Interval) est initialise pour chaque attribut de la grille source.
	// Pour un attribut categoriel, les groupes sont des groupes de modalites
	// Pour un attribut de type parties de variables, les groupes sont des parties de variable
	// Il est range dans un dictionnaire a partir du nom de l'attribut source
	// Une fois initialises, ces quantile builders sont utilises pour les granularisations
	// En sortie, le vecteur ivMaxPartNumbers contient pour chaque attribut le nombre maximal
	// de parties attendu apres granularisation
	// Pour un attribut numerique, il s'agit du nombre de valeurs distinctes
	// Pour un attribut categoriel,  il s'agit du nombre de parties dont l'effectif est > 1,
	// plus une partie en presence de singletons
	void InitializeQuantileBuilders(const KWDataGrid* sourceDataGrid, ObjectDictionary* odQuantilesBuilders,
					IntVector* ivMaxPartNumbers) const;

	// Export d'une grille granularisee pour une granularite commune a tous ses attributs
	// (attribut, parties et cellules)
	void ExportGranularizedDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid, int nGranularity,
					const ObjectDictionary* odQuantilesBuilders) const;

	// Construction d'un quantile builder pour chaque attribut interne dans un attribut de grille de type VarPart
	// ivMaxPartNumbers : nombre maximal de parties pour chaque attribut interne. Utilise pour reperer quand on est
	// arrive a la granularisation maxmimale
	void InitializeInnerAttributesQuantileBuilders(const KWDataGrid* sourceDataGrid,
						       ObjectDictionary* odInnerAttributesQuantilesBuilders,
						       IntVector* ivMaxPartNumbers) const;

	// Creation d'un nouveau KWDGInnerAttributes qui doit etre detruit par l'appelant
	// Attention: creation de nouveaux attributs internes qui doivent etre detruit par l'appelant
	void ExportGranularizedDataGridForVarPartAttributes(
	    const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid, int nGranularity,
	    const ObjectDictionary* odInnerAttributesQuantilesBuilders) const;

	// Calcul des nombres total de parties reel pour chaque niveau de granularisation
	// En entree, on a un dictionnaire de quantile builders pour un ensemble d'attribut, de grille ou internes.
	// En sortie, le vecteur contient pour chaque granularite i de 0 a max la somme des nombres de parties
	// effectivement obtenus par attributs quand on demande 2^i partiles.
	// Le max est detremine par la methode, en s'arretant a la granularite permettant d'obtenir
	// le total de parties des quantile builders
	void ComputeGranularizedTotalPartNumbers(const ObjectDictionary* odQuantilesBuilders,
						 IntVector* ivGranularityTotalPartNumbers) const;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Services avances de construction de grille

	// Export d'une grille en optimisant ses clusters de VarPart par fusion de VarParts
	// - dans le cas numerique: fusion des intervalles contigues
	// - dans le cas categoriel: fusion des modalites de la meme variable categorielle
	// Attention: creation de nouveaux attributs internes qui doivent etre detruits par l'appelant
	// En sortie : renvoie la variation de cout entre la grille source et la grille fusionnee
	double ExportDataGridWithVarPartMergeOptimization(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
							  const KWDataGridCosts* dataGridCosts) const;

	// Mise a jour des groupes de l'attribut VarPart d'une grille de type VarPart
	// En entree:
	//  - ivTargetGroupIndexes: index de groupe cible par index de groupe source
	//  - nTargetGroupNumber: nombre de groupe cibles
	// En sortie, les groupes cibles sont crees en exploitant la partition specifiee pour les groupes sources,
	// et les cellules de la grille cible sont recalculees
	void UpdateVarPartDataGridFromVarPartGroups(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
						    const IntVector* ivTargetGroupIndexes,
						    int nTargetGroupNumber) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de recuperation des partitions univariees pour initialiser
	// une grille cible (de facon compatible avec la grille source)

	// Creation d'une grille cible univariee a partir d'une partition univariee
	// Ajout de la transmission de la granularite a la grille cible
	void BuildUnivariateDataGridFromAttributeStats(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
						       KWAttributeStats* attributeStats) const;

	// Creation de la grille cible a partir du produit cartesien des partitions
	// univariees des attributs de la grille source
	// On utilise au plus log2(N) attributs
	// On renvoie true si on a pu construire une grille avec au moins deux atrributs
	// Ajout de la transmission de la granularite a la grille cible
	boolean BuildDataGridFromClassStats(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
					    KWClassStats* classStats) const;

	// Creation d'un attribut de grille a partir d'une partition univariee deja stockee
	// Le parametrage de l'attribut source (granularite, poubelle si categoriel) est copie
	// vers celui de l'attribut de DataGrid
	void BuildDataGridAttributeFromUnivariateStats(const KWDataGrid* sourceDataGrid, KWDGAttribute* targetAttribute,
						       KWAttributeStats* attributeStats) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de calcul des partitions univariees a la granularite courante
	// pour initialiser la grille cible (de facon compatible avec la grille source)
	// Dans toutes ces methodes, l egalite entre la granularite de la sourceDataGrid
	// et celle de la targetDataGrid est assuree

	// Creation d'une grille univariee contenant un seul des attributs de la grille initiale ayant pour partition
	// la partition optimale a la granularite de la grille initiale
	void BuildUnivariateDataGridFromGranularizedPartition(const KWDataGrid* sourceDataGrid,
							      KWDataGrid* univariateTargetDataGrid, int nAttributeIndex,
							      KWClassStats* classStats) const;

	// Creation d'un attribut de grille a partir d'une partition univariee calculee
	// pour la granularite de l'attribut source
	void BuildDataGridAttributeFromGranularizedPartition(const KWDataGrid* sourceDataGrid,
							     KWDGAttribute* sourceAttribute,
							     KWDGAttribute* targetAttribute,
							     KWClassStats* classStats) const;

	// Creation des parties de l'attribut cible numerique selon une partition univariee specifiee
	// dans une table d'effectifs
	void BuildPartsOfContinuousAttributeFromFrequencyTable(const KWDataGrid* sourceDataGrid,
							       KWDGAttribute* targetAttribute,
							       KWFrequencyTable* kwftTable,
							       const ALString& sAttributeName) const;

	// Creation des parties de l'attribut cible categoriel selon un vecteur de correspondance decrivant un groupage
	// univarie
	void BuildPartsOfSymbolAttributeFromGroupsIndex(const KWDGAttribute* initialAttribute,
							KWDGAttribute* targetAttribute, const IntVector* ivGroups,
							int nGroupNumber, int nGarbageModalityNumber) const;

	// Creation de la grille cible a partir du produit cartesien des partitions
	// univariees des attributs de la grille source
	// Ces partitions sont les partitions optimales pour la granularite courante.
	// (calculees pour l'occasion)
	// On utilise au plus log2(N) attributs
	// On renvoie true si on a pu construire une grille avec au moins deux atrributs
	boolean BuildDataGridFromUnivariateProduct(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
						   KWClassStats* classStats) const;

	///////////////////////////////////////////////////////////////////////////
	// Service de creation d'une table d'effectifs a partir d'un attribut
	// Export d'un attribut de la grille source sous la forme d'une table d'effectifs dense
	void ExportFrequencyTableFromOneAttribute(const KWDataGrid* sourceDataGrid, KWFrequencyTable* kwFrequencyTable,
						  const ALString& sAttributeName) const;

	///////////////////////////////////////////////////////////////////////////
	// Test de compatibilite de la grille cible avec la grille source
	// Le contenu de la cible doit etre une sous-partie du contenu de la source:
	//   - les attributs cibles appartiennent tous a la source
	//   - les parties cibles forment une partition des parties sources
	//   - les cellules cibles sont consistantes avec les cellules sources

	// Test de compatibilite complet
	boolean CheckDataGrid(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite de la granularite
	boolean CheckGranularity(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des valeurs cibles
	boolean CheckTargetValues(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des attributs, qui doivent etre une sous partie
	// des attributs sources
	boolean CheckAttributes(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des parties d'attributs, qui doivent former une partition
	// des partie sources
	boolean CheckParts(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	// Test de compatibilite des cellules
	boolean CheckCells(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const;

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Methode de test
	// Inclut un test d'export avec granularisation
	static void Test(const KWDataGrid* dataGrid);

	/////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////
	// Services principaux d'initialisation d'une grille et de ses attributs

	// Initialisation d'une grille (granularite, eventuelle valeurs cible) avec le bon nombre d'attributs
	// Les attributs ne sont pas initialises
	void InitialiseDataGrid(const KWDataGrid* originDataGrid, KWDataGrid* targetDataGrid,
				int nAttributeNumber) const;

	// Initialisation d'un attribut venant d'etre cree a partir d'un attribut valide
	// On initialise ses caracterisques principales uniquement, pas les parties:
	//  - nom, type, cout, ...
	//  - fourre-tout
	//  - attributs internes, en mode partages avec l'attribut source
	void InitialiseAttribute(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Services d'initialisation des parties d'un attribut

	// Initialisation des parties pour un attribut venant d'etre initialise, sans partie, a partir d'un attribut valide
	void InitialiseAttributeParts(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute) const;

	// Initialisation des parties pour un attribut de type VarPart venant d'etre initialise, sans partie, a partir d'un attribut valide,
	// en creant un clone de ses attributs internes
	void InitialiseVarPartAttributeClonedParts(const KWDGAttribute* sourceAttribute,
						   KWDGAttribute* targetAttribute) const;

	// Initialisation des parties pour un attribut VarPart a partir des PV d'un KWDGInnerAttribute et d'une partition source
	// En entree :
	// - sourceVarPartAttribute : attribute de type VarPart
	// - targetInnerAttributes : KWDGInnerAttributes dont les partitions sont plus fines que celles du KWDGInnerAttribute du sourceVarPartAttribute
	// En sortie :
	// - targetVarPartAttribute :
	// L'attribut cible utilise les targetInnerAttributes
	// La methode construit les parties de l'attribut VarPart cible en conservant les parties de l'attribut VarPart source et en y mettant les PV des attributs internes surtokenises
	void
	InitialiseVarPartAttributeWithNewSurtokenisedInnerAttributes(const KWDGAttribute* sourceVarPartAttribute,
								     const KWDGInnerAttributes* targetInnerAttributes,
								     KWDGAttribute* targetVarPartAttribute) const;

	// Initialisation des parties pour un attribut VarPart a partir des PV merges d'un KWDGInnerAttribute et d'une partition source
	// En entree :
	// - sourceVarPartAttribute : attribute de type VarPart
	// - mergedInnerAttributes : KWDGInnerAttributes dont les partitions sont moins fines que celles du KWDGInnerAttribute du sourceVarPartAttribute
	// En sortie :
	// - targetVarPartAttribute :
	// L'attribut cible utilise les mergedInnerAttributes
	// La methode construit les parties de l'attribut VarPart cible en conservant les parties de l'attribut VarPart source et en y mettant les PV mergees des attributs internes
	void InitialiseVarPartAttributeWithMergedInnerAttributes(const KWDGAttribute* sourceVarPartAttribute,
								 const KWDGInnerAttributes* mergedInnerAttributes,
								 KWDGAttribute* targetVarPartAttribute) const;

	// Initialisation d'une unique partie pour un attribut venant d'etre initialise, sans partie, a partir d'un attribut valide
	void InitialiseAttributeNullPart(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute) const;

	// Export d'une partition aleatoire des parties pour un attribut donne
	void InitialiseAttributeRandomParts(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute,
					    int nPartNumber) const;

	// Ajout aleatoire de partie dans une partition pour un attribut donne
	void AddAttributeRandomParts(const KWDGAttribute* sourceAttribute, KWDGAttribute* mandatoryAttribute,
				     KWDGAttribute* targetAttribute, int nRequestedPartNumber) const;

	// Ajout de partie granularisee pour un attribut donne
	void InitialiseAttributeGranularizedParts(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute,
						  int nGranularity, KWQuantileBuilder* quantileBuilder) const;
	void InitialiseAttributeGranularizedContinuousParts(const KWDGAttribute* sourceAttribute,
							    KWDGAttribute* targetAttribute, int nGranularity,
							    KWQuantileIntervalBuilder* quantileIntervalBuilder) const;
	void InitialiseAttributeGranularizedGroupableParts(const KWDGAttribute* sourceAttribute,
							   KWDGAttribute* targetAttribute, int nGranularity,
							   KWQuantileGroupBuilder* quantileGroupBuilder) const;

	// Test si on est dans le cas d'un attribut source pour l'analyse supervisee
	boolean IsSupervisedInputAttribute(const KWDGAttribute* attribute) const;

	// Verification que deux attributs sont coherents: meme nom, type...
	boolean CheckAttributesConsistency(const KWDGAttribute* attribute1, const KWDGAttribute* attribute2) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Services de creation d'attribut internes

	// Creation d'attributs internes en dupliquant les attributs internes source
	// Le parametre targetDataGrid permet de specialiser les classes utilisees pour les clonages
	// en utilisant lees methode de creation virtuelle specialisees (ex: NewAttribute)
	KWDGInnerAttributes* CloneInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
						  const KWDataGrid* targetDataGrid) const;

	// Creation d'attributs internes avec une seule partie par attribut
	KWDGInnerAttributes* CreateNullInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes) const;

	// Creation d'attributs internes en granularisant les attributs internes source
	KWDGInnerAttributes*
	CreateGranularizedInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes, int nGranularity,
					  const ObjectDictionary* odInnerAttributesQuantilesBuilders) const;

	// Creation d'attributs internes par ajout aleatoire de parties de variable dans chaque attribut
	// parmi les partitions de reference les plus fines pour ces innerAttributes
	KWDGInnerAttributes* CreateRandomInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
							 const KWDGInnerAttributes* referenceInnerAttributes,
							 int nTargetTokenNumber) const;
	/*Contenu
	Boucle sur les innerAttributes et appel des methodes AddContinuousAttributeRandomParts et
	    AddSymbolAttributeRandomParts en garantissant que la somme des nRequestedPartNumber par innerAttribut
		ne depasse pas les nTargetTokenNumber*/

	//////////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Effectue la fusion des PV : dans le cas numerique, fusion des intervalles contigues et dans le cas categoriel
	// fusion des modalites de la meme variable categorielle En sortie : renvoie la variation de cout cumulee pour
	// les clusters impactes par les fusions de PV (ne prend pas en compte la variation du cout de partition)
	double MergePartsForVarPartAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Creation et initialisation d'un quantile builder, et du nombre max de parties a quuantiliser
	void CreateAttributeQuantileBuilder(const KWDGAttribute* attribute, KWQuantileBuilder*& quantileBuilder,
					    int& nMaxPartNumber) const;

	// Export des effectifs des valeurs de la grille initiale vers un attribut categoriel entierement specifie
	// La valeur speciale recoit pour effectif l'ensemble des effectifs manquants
	void ExportAttributeSymbolValueFrequencies(KWDGAttribute* sourceAttribute,
						   KWDGAttribute* targetAttribute) const;

	// Tri des parties d'un attribut source Symbol ou VarPart, selon les groupements
	// de ces parties dans un attribut cible groupe compatible
	// Les parties sources se trouvent dans le tableau resultat, trie correctement
	// par groupe, et en ordre aleatoire a l'interieur de chaque groupe
	//    oaSortedSourceParts: parties sources triees par groupe cible
	//    oaSortedGroupedParts: parties groupees associees aux parties source
	void SortAttributePartsByTargetGroups(const KWDGAttribute* sourceAttribute, KWDGAttribute* groupedAttribute,
					      ObjectArray* oaSortedSourceParts,
					      ObjectArray* oaSortedGroupedParts) const;

	// Initialisation d'un vecteur de nombres aleatoires compris entre 0 et max exclu, ordonnes et tous differents
	void InitRandomIndexVector(IntVector* ivRandomIndexes, int nIndexNumber, int nMaxIndex) const;
};
