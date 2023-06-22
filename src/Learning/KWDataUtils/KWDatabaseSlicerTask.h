// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDatabaseSlicerTask;
class KWDatabaseSlicerOutputBufferedFile;

#include "KWDatabaseTask.h"
#include "KWDataTableSliceSet.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseSlicerTask
// Calcul de statistiques de base en parallele
class KWDatabaseSlicerTask : public KWDatabaseTask
{
public:
	// Constructeur
	KWDatabaseSlicerTask();
	~KWDatabaseSlicerTask();

	// Decoupage d'une base de donne en une liste de tranches en repartissant les attributs
	// de la table principale dans les troncons, sous formes d'attributs natifs
	// Chaque troncon contient des attributs numeriques ou categoriels, non calcules,
	// potentiellement au format sparse, avec leur information de cout dans un dictionnaire
	// Chaque troncon est une liste d'ordonnee d'un a plusieurs fichiers plats
	// Les messages affiches ne concernent que la base traitee en entree
	// En entree:
	//  . une base de donnees mono ou multi-tables, dont les attributs ou blocs d'attributs sont a partitionner
	//  . un eventuel attribut cible, a ne pas prendre en compte
	//  . le nombre total d'instances
	//  . le nombre max d'attribut par troncon
	// En sortie:
	//  . la specifications complete de l'ensemble des troncons (KWDatabaseSliceSet)
	//  . les fichiers correspondant aux troncons
	//  . les stats de volumetrie a jour dans le sliceSet en sortie
	//
	// Le nombre max d'attributs par troncon en entre est a calculer par la methode
	// KWDataPreparationTask::ComputeMaxLoadableAttributeNumber, qui se base sur une heuristique "raisonnable" pour
	// tenir compte de la sparsite des blocs d'attributs. Au debut de la tache, on n'exploite que des elements
	// heuristiques de dimensionnement. A la fin de la tache, on a collecte des statistiques de volumetrie dans le
	// DataTableSliceSet:
	//   . partition des variables et blocs de variables en slices
	//   . nombre d'individus par chunk
	//   . nombre de valeurs effectives par bloc d'attributs
	//   . taille sur disque occupee par variable dense Symbol
	//   . taille de chaque fichier de co-chunk
	// Ces statistiques de volumetrie pourront etre exploitees par les taches suivantes pour affiner leur
	// dimensionnement
	boolean SliceDatabase(const KWDatabase* sourceDatabase, const ALString& sTargetAttributName,
			      int nInstanceNumber, int nMaxAttributeNumberPerSlice,
			      KWDataTableSliceSet* outputDataTableSliceSet);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////
	// La collecte des statistiques lors du decoupage en tranches permet ensuite
	// d'affiner les algorithme de dimensionnement des taches.
	// Il s'agit d'un compromis entre d'une part rapidite de collecte, volume des
	// informations collectees, a la fois pour le transfert entre esclaves et maitre,
	// et pour le stockage dans les tranches, d'autre part l'interet pour le
	// dimensionnement fin des taches, et enfin le cout de developpement et
	// maintenance de cette collecte et de son exploitation.
	// A cet effet, on a choisi de collecter le nombre de valeurs par bloc sparse,
	// qui impacte la taille memoire pour stocker ces valeurs, et la taille pour
	// les traiter via des table de contingence multiples par bloc sparse, et
	// la place occupees sur disque par les variables categorielles denses,
	// qui  impacte la taille memoire pour stocker ces valeurs.
	//
	// On n'a pas traite le cas des valeurs categorielles dans les blocs sparse,
	// car il s'agit d'un cas d'usage potentiellement rare, et plus couteux a traiter.
	// On a egalement que des statistique globale de nombre de valeurs et de taille
	// occupee, tres rapides a collecter, mais pas de statistiques sur le nombre
	// de valeurs distinctes, potentiellement utiles pour affiner le dimensionnement,
	// mais beaucoup plus couteuses a collecter. Le dimensionnement exploitant
	// les statistiques collectees devra donc encore reposer sur des heuristiques
	// et rester en partie speculatif. Un dimensioonement exact est trop couteux
	// a mettre en place, car il necessiterait potentiellement d'avoir toutes les
	// informations qui ne sont disponibles qu'a posteriori, une fois les taches effectuees.

	// Parametrage de la classe pour la preparation, en ne chargeant que les attributs a preparer
	void InitClassForPreparation(KWClass* kwcClass, const ALString& sTargetAttributName);

	// Reinitialisation de la classe, avec tous les attributs a charger
	void ResetClass(KWClass* kwcClass);

	// Reimplementation de l'affichage des messages
	void DisplaySpecificTaskMessage() override;

	// Reimplementation de methodes de gestion du plan d'indexation des bases
	// pour prendre en compte la taille sur le disque en sortie
	boolean ComputeDatabaseOpenInformation() override;

	// Memoire min et max necessaire pour l'ecriture des tranches par le splicer
	longint GetEmptyOutputNecessaryMemory();
	longint GetMinOutputNecessaryMemory();
	longint GetMaxOutputNecessaryMemory();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveInitializePrepareDatabase() override;
	boolean SlaveProcess() override;
	boolean SlaveProcessStartDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Methode de l'esclave de destruction des fichiers en sortee par tranche et dereferencement de ces fichiers
	// dans les tranches et les noms de fichier dans la variable en sortie
	void DeleteSliceFiles();

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Specification du decoupage de la base en tranches
	// La specification des dictionnaires par tranche est partagee entre le maitre et les esclaves
	// Par contre, la specification des fichiers par tranche, initialement vide, et completee
	// par le maitre au fur et a mesure des agregations des resultats des esclaves
	PLShared_DataTableSliceSet shared_DataTableSliceSet;

	// Nombre total de blocs sur l'ensemble des tranches
	PLShared_Int shared_nTotalBlockNumber;

	// Nombre total d'attributs denses Symbol sur l'ensemble des tranches
	PLShared_Int shared_nTotalDenseSymbolAttributeNumber;

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves

	// Nombre d'enregistrement ecrits
	PLShared_Longint output_lWrittenObjects;

	// Fichiers ecrits par l'esclave pour chacune des tranches
	// URI: indispensable, car partage entre maitre et esclave
	PLShared_StringVector output_svSliceFileNames;

	// Taille des fichiers ecrits par l'esclave pour chacune des tranches
	PLShared_LongintVector output_lvSliceFileSizes;

	// Nombre total de valeurs par bloc d'attributs ecrits par l'esclave pour l'ensemble de toutes les tranches
	// Les blocs sont ici indexes selon l'ordre des tranches, sur la base de
	//  Total(N) = NbBlock(Slice1)+NbBlock(Slice2)+...+NbBlock(SliceN),
	// avec les plages d'index par tranche suivantes:
	//   Slice 1: 0 a Total(1)-1
	//   Slice 2: Total(1) a Total(2)-1
	//   ...
	PLShared_LongintVector output_lvAllAttributeBlockValueNumbers;

	// Taille occupee sur disque par attribut dense Symbol pour l'ensemble de toutes les tranches
	// Les blocs sont ici indexes selon l'ordre des tranches, sur la base de
	//  Total(N) = NbDenseSymbolAttribute(Slice1)+NbDenseSymbolAttribute(Slice2)+...+NbDenseSymbolAttribute(SliceN),
	// avec les plages d'index par tranche suivantes:
	//   Slice 1: 0 a Total(1)-1
	//   Slice 2: Total(1) a Total(2)-1
	//   ...
	// Attention, dans les slices finalisees, l'indexation est sur l'ensemble de tous les atrributs denses,
	// et non seulement sur les attributs dense Symbol
	PLShared_LongintVector output_lvAllDenseSymbolAttributeDiskSizes;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	// Buffer en sortie pour l'ensemble des tranches du fichier
	// Au fur et a mesure du parcours du fichier en entree, toutes les tranches
	// sont ecrites dans un seul buffer, en memorisant les offsets de debut de ligne par tranche.
	// Il s'agit en fait d'un buffer memoire qui peut etre de grande taille.
	// Quand il est flushe, les fichiers par tranches sont ouvert un a un en append, et chacun recoit
	// les lignes qui le concerne
	// Cela permet d'eviter d'ouvrir un fichier par tranche, ce qui pose le probleme potentiel
	// d'un trop grand nombre de fichiers ouvert et d'une trop grande place memoire occupee par
	// l'ensemble des fichiers ouverts
	KWDatabaseSlicerOutputBufferedFile* allSliceOutputBuffer;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseSlicerOutputBufferedFile
// Specialisation de la classe OutputBufferedFile pour le decoupage d'un fichier en tranche
// Quand on doit decouper un fichier en de nombreuses tranches, cela pourrait entrainer
// la creation de nombreux buffers de fichier, par tranche, ce qui occupe trop de memoire
// De facon alternative, on cree un seul fichier en memorisant les offset des debuts de lignes
// Cela permet de remplacer par un seul buffer.
// Au momment du Flush, on ecrit dans chaque fichier de tranche les lignes qui lui sont associees.
class KWDatabaseSlicerOutputBufferedFile : public OutputBufferedFile
{
public:
	// Constructeur
	KWDatabaseSlicerOutputBufferedFile();
	~KWDatabaseSlicerOutputBufferedFile();

	///////////////////////////////////////////////////////////////////////////////////
	// Parametrage par un DataTableSliceSet et par la liste des fichiers en sortie

	// Parametrage de la specification des tranches
	// Memoire: appartient a l'appelant
	void SetDataTableSliceSet(KWDataTableSliceSet* sliceSet);
	KWDataTableSliceSet* GetDataTableSliceSet();

	// Parametrage des noms des fichiers de la tranche
	// URI: nom de fichiers potentiellement distants
	// Memoire: appartient a l'appele
	StringVector* GetSliceFileNames();

	// Memoire minimum necessaire
	static longint GetMinimumNecessaryMemory();

	///////////////////////////////////////////////////////////////////////////////////
	// Methodes a utiliser pour ecrire dans les fichier en sortie

	// Ecriture d'une instance du sliceSet en le dispatchant sur l'ensemble des fichiers de tranche en sortie
	// On met egalement a jour le nombre de valeurs par bloc ainsi que la taille occupee sur le fichier par
	// les attributs Symbol dense
	// Peut positionner une erreur grave
	boolean WriteObject(const KWObject* kwoObject, LongintVector* lvAllAttributeBlockValueNumbers,
			    LongintVector* lvAllDenseSymbolAttributeBlockFileSizes);

	// Redefinition des methode d'ouverture et de fermeture, pour ne pas dependre d'un fichier
	boolean Open() override;
	boolean Close() override;
	boolean IsOpened() const override;

	// Reimplementation de la methode Flush, pour ecriture dans chaque fichier par tranche
	void Flush() override;

	// Redefinition de l'ecriture d'une fin de ligne (methode non virtuelle), avec memorisation
	// d'une position dans le buffer, permettant d'assosicier la ligne a une tranche
	void WriteEOL();

	/////////////////////////////////////////////////////////////
	////// Implementation
protected:
	// Positions des lignes dans le buffer
	// La premiere position correspond a la fin de la premiere ligne a ecrire dans le fichier
	IntVector ivLineOffsets;

	// Index de la prochaine tranche a ecrire, en tete du buffer courant
	int nNextSliceIndex;

	// Specification des tranches
	KWDataTableSliceSet* dataTableSliceSet;

	// Specification des nom de fichiers des tranches
	// URI: noms de fichiers potentiellement distant
	StringVector svSliceFileNames;

	// Buffer de fichier en sortie pour ecrire la tranche courante
	OutputBufferedFile* outputSliceFile;
};

///////////////////////////
// Methodes en inline

inline boolean KWDatabaseSlicerOutputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

inline void KWDatabaseSlicerOutputBufferedFile::WriteEOL()
{
	// Appel de la methode ancetre
	OutputBufferedFile::WriteEOL();

	// Memorisation du debut de la ligne suivante, sauf s'il y a eu un flush avant
	if (GetCurrentBufferSize() > 0)
		ivLineOffsets.Add(GetCurrentBufferSize());
	// Sinon, il faut changer de prochaine tranche a ecrire
	else
		nNextSliceIndex = (nNextSliceIndex + 1) + dataTableSliceSet->GetSliceNumber();
}
