// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCCoclusteringBuilder.h"

////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringBuilder

CCCoclusteringBuilder::CCCoclusteringBuilder()
{
	coclusteringDataGrid = NULL;
	initialDataGrid = NULL;
	coclusteringDataGridCosts = NULL;
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	bIsDefaultCostComputed = false;
	bExportAsKhc = true;
	// CH IV Begin
	bVarPartCoclustering = false;
	nFirstVarPartsAttributeIndex = -1;
	// CH IV End
}

CCCoclusteringBuilder::~CCCoclusteringBuilder()
{
	CleanCoclusteringResults();
}

const ALString& CCCoclusteringBuilder::GetFrequencyAttribute() const
{
	return sFrequencyAttribute;
}

void CCCoclusteringBuilder::SetFrequencyAttribute(const ALString& sValue)
{
	sFrequencyAttribute = sValue;
}
// CH IV Begin
const ALString& CCCoclusteringBuilder::GetIdentifierAttribute() const
{
	return sIdentifierAttribute;
}

void CCCoclusteringBuilder::SetIdentifierAttribute(const ALString& sValue)
{
	sIdentifierAttribute = sValue;
}

boolean CCCoclusteringBuilder::GetVarPartCoclustering() const
{
	return bVarPartCoclustering;
}

void CCCoclusteringBuilder::SetVarPartCoclustering(boolean bValue)
{
	bVarPartCoclustering = bValue;
}
// CH IV End
boolean CCCoclusteringBuilder::CheckSpecifications() const
{
	boolean bOk;
	KWAttribute* attribute;
	int i;

	// Verification standard
	bOk = KWAttributeSubsetStats::CheckSpecifications();

	// Verification du cas non supervise
	if (bOk and GetTargetAttributeName() != "")
	{
		bOk = false;
		AddError("Coclustering not available in the supervised case");
	}

	// Verification du nombre d'attribut
	if (bOk and GetAttributeNumber() < 2)
	{
		bOk = false;
		AddError("Coclustering available for at least two variables");
	}

	// Verification du type des attributs
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		// Recherche de l'attribut
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(i));

		// Test du type de l'attribut
		if (attribute != NULL and not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			AddError("Variable " + GetAttributeNameAt(i) + " is not of numerical or categorical type");
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// Verification de l'attribut d'effectif
	if (GetFrequencyAttribute() != "")
		if (bOk and sFrequencyAttribute != "")
		{
			// Recherche de la variable correspondante dans le dictionnaire
			attribute = GetClass()->LookupAttribute(sFrequencyAttribute);

			// La variable doit etre presente dans le dictionnaire
			if (attribute == NULL)
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering frequency variable " + sFrequencyAttribute +
						     " unknown in dictionary " + GetClass()->GetName());
			}
			// De type Continuous
			else if (attribute->GetType() != KWType::Continuous)
			{
				bOk = false;
				Global::AddError("", "",
						 "Incorrect type for coclustering frequency variable " +
						     sFrequencyAttribute);
			}
			// Et utilise
			else if (not attribute->GetUsed())
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering frequency variable " + sFrequencyAttribute +
						     " unused in dictionary " + GetClass()->GetName());
			}
			// et differente des attributs de coclustering
			else
			{
				// Parcours des attributs de coclustering
				for (i = 0; i < GetAttributeNumber(); i++)
				{
					if (sFrequencyAttribute == GetAttributeNameAt(i))
					{
						bOk = false;
						Global::AddError(
						    "", "",
						    "Coclustering frequency variable " + sFrequencyAttribute +
							" is already used among the coclustering variables");
						break;
					}
				}
			}
		}

	return bOk;
}

boolean CCCoclusteringBuilder::ComputeCoclustering()
{
	boolean bOk = true;
	KWTupleTableLoader tupleTableLoader;
	StringVector svTupleAttributeNames;
	KWTupleTable tupleTable;
	KWTupleTable tupleFrequencyTable;
	CCCoclusteringOptimizer dataGridOptimizer;
	KWDataGrid optimizedDataGrid;
	KWDataGridManager dataGridManager;
	ALString sTmp;
	// CH IV Begin
	ObjectDictionary odObservationNumbers;
	// CH IV End

	require(Check());
	require(CheckSpecifications());

	// Debut de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::BeginErrorCollection();
	KWLearningErrorManager::AddTask("Coclustering");

	// Debut du pilotage anytime
	AnyTimeStart();

	// Nettoyage des resultats de coclustering
	CleanCoclusteringResults();

	///////////////////////////////////////////////////////////////////////////////////
	// Calcul d'une grille initiale

	// Verification de la memoire necessaire pour charger la base
	if (bOk)
		bOk = CheckMemoryForDatabaseRead(GetDatabase());

	// Alimentation d'une table de tuples comportant les attribut a analyser a partir de la base
	if (bOk and not TaskProgression::IsInterruptionRequested())
	// CH IV Begin
	{
		// Cas ou le coclustering se ramene a un coclustering standard, appel de la methode standard de creation
		// de grille
		if (not GetVarPartCoclustering())
			bOk = FillTupleTableFromDatabase(GetDatabase(), &tupleTable);
		// Sinon, cas de coclustering VarPart
		else
			bOk = FillVarPartTupleTableFromDatabase(GetDatabase(), &tupleTable, odObservationNumbers);
	}
	// CH IV End

	// Calcul de statistiques descriptives globales et par attribut
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk = GetLearningSpec()->ComputeTargetStats(&tupleTable);
	if (bOk and not TaskProgression::IsInterruptionRequested())
		ComputeDescriptiveAttributeStats(&tupleTable, &odDescriptiveStats);

	// Verification de la memoire necessaire pour construire une grille initiale a partir des tuples
	nMaxCellNumberConstraint = 0;
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk =
		    CheckMemoryForDataGridInitialization(GetDatabase(), tupleTable.GetSize(), nMaxCellNumberConstraint);

	// Creation du DataGrid
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		// CH IV Begin
		// Cas ou le coclustering se ramene a un coclustering standard, appel de la methode standard de creation
		// de grille
		if (not GetVarPartCoclustering())
			initialDataGrid = CreateDataGrid(&tupleTable);
		// Sinon, cas de coclustering VarPart
		else
			initialDataGrid = CreateVarPartDataGrid(&tupleTable, odObservationNumbers);
		// CH IV End
		bOk = initialDataGrid != NULL;
	}

	// Supression des tuples, desormais transferes dans la grille
	tupleTable.CleanAll();

	// CH IV Begin
	// Suppression du dictionnaire associe contenant le nombre d'observations par valeur d'identifiant
	odObservationNumbers.DeleteAll();
	// CH IV End

	// On verifie une derniere fois qu'il n'y a pas eu d'interruption
	if (bOk)
		bOk = not TaskProgression::IsInterruptionRequested();

	// Verification de la memoire necessaire pour optimiser le coclustering
	if (bOk)
		bOk = CheckMemoryForDataGridOptimization(initialDataGrid);

	// Arret si grille non creee, avec nettoyage
	if (not bOk)
	{
		if (initialDataGrid != NULL)
		{
			delete initialDataGrid;
			initialDataGrid = NULL;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Optimisation et post-optimisation de la grille

	// Optimisation de la grille, par appel d'une methode virtuelle
	if (bOk and not TaskProgression::IsInterruptionRequested())
		OptimizeDataGrid(initialDataGrid, &optimizedDataGrid);

	// La solution est sauvegardee periodiquement grace au mode anytime
	// Nettoyage si aucune solution n'a encore ete trouvee
	if (coclusteringDataGrid == NULL)
		CleanCoclusteringResults();

	// Nettoyage de la grille initiale (si non deja nettoyee), dont on a plus besoin desormais
	if (initialDataGrid != NULL)
	{
		delete initialDataGrid;
		initialDataGrid = NULL;
	}
	nMaxCellNumberConstraint = 0;

	// Fin du pilotage anytime
	AnyTimeStop();

	// Fin de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::EndErrorCollection();

	// Nettoyage
	bIsStatsComputed = bOk;
	ensure(Check());
	return bIsStatsComputed;
}

void CCCoclusteringBuilder::OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid)
{
	CCCoclusteringOptimizer dataGridOptimizer;
	boolean bDisplayResults = false;
	boolean bDisplayPartitionLevel = false;

	// Creation et initialisation de la structure de couts
	coclusteringDataGridCosts = CreateDataGridCost();

	// Parametrage des couts de l'optimiseur de grille
	dataGridOptimizer.SetDataGridCosts(coclusteringDataGridCosts);

	// Parametrage pour l'optimisation anytime: avoir acces aux ameliorations a chaque etape de l'optimisation
	dataGridOptimizer.SetCoclusteringBuilder(this);

	// Recopie du parametrage d'optimisation des grilles
	dataGridOptimizer.GetParameters()->CopyFrom(GetPreprocessingSpec()->GetDataGridOptimizerParameters());
	dataGridOptimizer.GetParameters()->SetOptimizationTime(RMResourceConstraints::GetOptimizationTime());

	// CH IV Refactoring: supprimer???
	// Parametrage d'un niveau d'optimisation "illimite" si une limite de temps est indiquee
	// debug(dataGridOptimizer.GetParameters()->SetOptimizationLevel(0));
	// debug(cout << "BEWARE: Optimization level set to 0 in debug mode only!!!" << endl);
	// if (dataGridOptimizer.GetParameters()->GetOptimizationTime() > 0)
	// dataGridOptimizer.GetParameters()->SetOptimizationLevel(20);

	// CH IV Begin
	if (bDisplayResults)
	{
		cout << "CCCoclusteringBuilder :: Grille initiale avant optimisation" << endl;
		initialDataGrid->Write(cout);
	}

	// Optimisation de la grille dans le cas d'un coclustering variale * variable
	// Pour s'arreter apres la tache de creation de la grille et verifier que le nettoyage est OK
	if (not initialDataGrid->GetVarPartDataGrid())
	{
		// Initialisation des couts par defaut
		coclusteringDataGridCosts->InitializeDefaultCosts(inputInitialDataGrid);

		if (bDisplayResults)
			cout << "Algorithme d'optimisation variable * variable\n";
		dataGridOptimizer.OptimizeDataGrid(initialDataGrid, optimizedDataGrid);
	}
	// Sinon : optimisation de la grille dans le cas d'un coclustering instances * variables
	else
	{
		if (bDisplayResults)
			cout << "Algorithme d'optimisation instances * variables\n";
		// CH IV a encapsuler dans une methode mais probleme pour l'instant avec dataGridOptimizer utilise comme
		// argument
		// OptimizeVarPartDataGrid(initialDataGrid, optimizedDataGrid, dataGridOptimizer);

		// CH IV a encapsuler dans une methode dediee
		ObjectDictionary odInnerAttributesQuantileBuilders;
		KWDataGrid* nullDataGrid;
		KWDataGrid* partitionedDataGrid;
		KWDataGrid* partitionedOptimizedDataGrid;
		KWDataGridManager dataGridManager;
		IntVector ivMaxPartNumbers;
		int nPrePartitionIndex;
		int nPrePartitionMax;
		boolean bIsLastPrePartitioning;
		double dRequiredIncreasingCoefficient;
		int nValueNumber;
		IntVector ivPreviousPartNumber;
		IntVector ivCurrentPartNumber;
		boolean bIsPrePartitioningSelected;
		double dPartitionBestCost;
		double dBestCost;
		int nInnerAttribute;
		int nAttribute;
		int nInnerAttributeCumulated;
		KWDGAttribute* attribute;
		KWDGAttribute* innerAttribute;
		KWDataGrid* partitionedPostMergedOptimizedDataGrid;
		KWDataGrid partitionedReferencePostMergedDataGrid;
		double dMergedCost;
		double dBestMergedCost;
		double dFusionDeltaCost;
		IntVector ivUsedPrePartitioning;
		boolean bDisplayPrePartitioning = false;
		double dEpsilon = 1e-6;
		double dTotalComputeTime;
		ALString sTmp;

		// On parcourt les differentes tailles de pre-partitionnement des attributs de type VarPart par
		// puissance de 2 Ce parcours est similaire a celui des granularites mais il porte exclusivement sur le
		// partitionnement des parties de variable des attributs internes A chaque pre-partitionnement est
		// associe un cout seuil par defaut : c'est le cout du pseudo-modele nul qui contient 1 cluster par
		// attribut avec le nombre de parties de variables du pre-partitionnement Le vrai modele nul contient un
		// cluster par attribut et une seule partie de variable par attribut interne dans l'axe parties de
		// variable
		nValueNumber = initialDataGrid->GetGridFrequency();
		nPrePartitionMax = (int)ceil(log(nValueNumber * 1.0) / log(2.0));

		// Initialisation
		nPrePartitionIndex = 1;
		bIsLastPrePartitioning = false;

		// Initialisation du facteur d'accroissement requis entre deux pre-partitionnements
		dRequiredIncreasingCoefficient = 2;

		// Initialisation d'un quantile builder pour chaque attribut interne dans un attribut de grile de type
		// de type VarPart La grille initiale comporte un cluster par partie de variable pour ses attributs de
		// grille de type VarPart
		dataGridManager.SetSourceDataGrid(initialDataGrid);
		dataGridManager.InitializeQuantileBuildersForVariablePartsPartitioning(
		    &odInnerAttributesQuantileBuilders, &ivMaxPartNumbers);

		if (bDisplayPrePartitioning)
		{
			cout << "ivMaxPartNumbers\t" << ivMaxPartNumbers;
			cout << flush;
		}

		// Initialisation des vecteurs de nombre de parties courant et precedent
		ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
		ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

		// CH AB AF peut ne pas etre necessaire car ces couts par defaut sont remplaces ensuite par ceux
		// associes au premier partitionnement Initialisation des couts par defaut
		coclusteringDataGridCosts->InitializeDefaultCosts(inputInitialDataGrid);

		// Initialisation de l'optimiseur pour que la grille initiale puisse servir a l'export des grilles avec
		// PV fusionnnees
		dataGridOptimizer.SetInitialDataGrid(initialDataGrid);

		// Export de la grille du (vrai) modele nul : un seul cluster par attribut et une seule partie de
		// variable par attribut interne
		nullDataGrid = new KWDataGrid;
		dataGridManager.ExportNullDataGrid(nullDataGrid);
		dAnyTimeDefaultCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(nullDataGrid);
		dataGridManager.CopyDataGrid(nullDataGrid, optimizedDataGrid);
		dBestCost = dAnyTimeDefaultCost;
		dBestMergedCost = dBestCost;

		// CH AB n'est plus necessaire c'est le cout du vrai modele nul qui doit etre utilise comme reference
		// Initialisation du meilleur cout au cout du modele nul conditionnellement au pre-partitionnement
		// Il ne s'agit donc pas ici du cout du VRAI modele nul (un seul cluster par attribut et une seule
		// partie de variable par attribut interne)
		// dBestCost = coclusteringDataGridCosts->GetTotalDefaultCost();
		// dBestMergedCost = dBestCost;
		// cout << "Cout du modele nul associe a la grille de reference\t" << dBestCost << "\n";

		// Parcours des tailles de pre-partitionnement en parties de variable
		while (nPrePartitionIndex <= nPrePartitionMax and not bIsLastPrePartitioning)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
				break;

			// Pre-partitionnement des attributs internes de la grille initiale
			dataGridManager.SetSourceDataGrid(initialDataGrid);
			partitionedDataGrid = new KWDataGrid;
			dataGridManager.ExportPartitionedDataGridForVarPartAttributes(
			    partitionedDataGrid, nPrePartitionIndex, &odInnerAttributesQuantileBuilders);

			if (bDisplayResults)
			{
				cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement "
				     << nPrePartitionIndex << endl;
				partitionedDataGrid->Write(cout);
			}

			// Etude du nombre de parties des attributs internes pour decider du traitement ou non de ce
			// pre-partitionnement Parcours des attributs
			nInnerAttributeCumulated = 0;
			for (nAttribute = 0; nAttribute < partitionedDataGrid->GetAttributeNumber(); nAttribute++)
			{
				attribute = partitionedDataGrid->GetAttributeAt(nAttribute);

				// Traitement dans le cas VarPart uniquement
				if (attribute->GetAttributeType() == KWType::VarPart)
				{
					for (nInnerAttribute = 0;
					     nInnerAttribute < attribute->GetInnerAttributeNumber(); nInnerAttribute++)
					{
						innerAttribute =
						    attribute->GetDataGrid()
							->GetInnerAttributes()
							->LookupInnerAttribute(
							    attribute->GetInnerAttributeNameAt(nInnerAttribute));

						// Memorisation du nombre de parties de l'attribut interne granularise
						ivCurrentPartNumber.SetAt(nInnerAttributeCumulated,
									  innerAttribute->GetPartNumber());
						nInnerAttributeCumulated++;
					}
				}
			}

			// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
			// pre-partitionnee est la derniere
			bIsLastPrePartitioning = true;

			// Si on n'a pas encore atteint la granularite max pour les attributs internes partitionnes
			if (nPrePartitionIndex < nPrePartitionMax)
			{
				for (nAttribute = 0; nAttribute < ivMaxPartNumbers.GetSize(); nAttribute++)
				{
					// Cas ou le nombre de parties de l'attribut courant est inferieur au nombre max
					// de parties de l'attribut
					if (ivCurrentPartNumber.GetAt(nAttribute) < ivMaxPartNumbers.GetAt(nAttribute))
					{
						bIsLastPrePartitioning = false;
						break;
					}
				}
			}

			// CH a supprimer car granu max sur N nbre total d'observations et pas sur Nk nombre
			// d'observations par variable -> commentaire a comprendre ! Cas ou cette granularite sera la
			// derniere traitee
			if (bIsLastPrePartitioning)
				// On positionne l'index de granularite au maximum afin que l'affichage soit adapte a ce
				// cas
				partitionedDataGrid->GetInnerAttributes()->SetVarPartGranularity(nPrePartitionMax);

			// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
			// pre-partitionnee sera optimise Il faut pour cela qu'elle soit suffisamment differente de la
			// grille analysee precedemment
			bIsPrePartitioningSelected = false;
			for (nAttribute = 0; nAttribute < ivCurrentPartNumber.GetSize(); nAttribute++)
			{
				// Cas d'accroissement suffisant du nombre de parties : le cas d'un attribut suffit pour
				// justifier le traitement de cette granularite
				if ((ivCurrentPartNumber.GetAt(nAttribute) >=
				     ivPreviousPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient) and
				    (ivCurrentPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient <=
				     ivMaxPartNumbers.GetAt(nAttribute)))
				{
					bIsPrePartitioningSelected = true;
					break;
				}
			}

			// On ne traite pas les grilles avec un seul attribut informatif
			if (partitionedDataGrid->GetInformativeAttributeNumber() <= 1)
				bIsPrePartitioningSelected = false;

			// Cas du traitement de la granularite courante
			if (bIsPrePartitioningSelected or bIsLastPrePartitioning)
			{
				// Affichage du niveau de pre-partitionnement et du nombre de parties associe
				if (bDisplayPartitionLevel)
					cout << "Niveau de pre-partitionnement\t" << nPrePartitionIndex
					     << "\tNombre de parties \t" << ivCurrentPartNumber;

				// Memorisation des pre-partitionnements exploites
				ivUsedPrePartitioning.Add(nPrePartitionIndex);

				// Initialisation de la grille pre-partitionnee optimisee
				partitionedOptimizedDataGrid = new KWDataGrid;

				// Initialisation du modele par defaut : ce modele depend du partitionnement des
				// attributs internes
				coclusteringDataGridCosts->InitializeDefaultCosts(partitionedDataGrid);

				// Optimisation de la grille pre-partitionnee
				// Le cout dPartitionBestCost est le cout de la grille antecedente de la meilleure
				// grille post-fusionnee (fusion des parties de variables consecutives dans un meme
				// cluster)
				dPartitionBestCost = dataGridOptimizer.OptimizeDataGrid(partitionedDataGrid,
											partitionedOptimizedDataGrid);

				// Calcul du temps d'optimisation (depuis le debut)
				tAnyTimeTimer.Stop();
				dTotalComputeTime = tAnyTimeTimer.GetElapsedTime(), tAnyTimeTimer.Start();

				if (bDisplayResults)
				{
					cout << "Apres OptimizeGranularizedDataGrid pour Granularite "
					     << nPrePartitionIndex << "\t Cout " << dPartitionBestCost << endl;
					partitionedOptimizedDataGrid->WriteAttributes(cout);
					partitionedOptimizedDataGrid->WriteAttributeParts(cout);
				}

				// Creation d'une grille post-mergee pour cette granularite de pre-partitionnement
				// CH AF Il faudrait aussi recalculer la post-optimisation VarPart de la grille
				// post-mergee mais de toute facon cette meilleure grille aura ete memorisee avec
				// HandleOptimisationStep dans l'algo VNS -> commentaire a comprendre
				partitionedPostMergedOptimizedDataGrid = new KWDataGrid;

				if (partitionedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
				    dataGridOptimizer.GetParameters()->GetVarPartPostMerge())
				{
					dataGridManager.SetSourceDataGrid(partitionedOptimizedDataGrid);
					// Creation d'une nouvelle grille avec nouvelle description des PV et calcul de
					// la variation de cout liee a la fusion des PV
					dFusionDeltaCost = dataGridManager.ExportMergedDataGridForVarPartAttributes(
					    partitionedPostMergedOptimizedDataGrid, coclusteringDataGridCosts);
					assert(not partitionedPostMergedOptimizedDataGrid->GetVarPartsShared());

					// Calcul et verification du cout
					dMergedCost = dPartitionBestCost + dFusionDeltaCost;
					// Le cout precedent devra etre correct
					assert(dMergedCost * (1 - dEpsilon) <
					       coclusteringDataGridCosts->ComputeDataGridTotalCost(
						   partitionedPostMergedOptimizedDataGrid));
					assert(coclusteringDataGridCosts->ComputeDataGridTotalCost(
						   partitionedPostMergedOptimizedDataGrid) <
					       dMergedCost * (1 + dEpsilon));

					if (bDisplayResults)
					{
						cout << "CCOptimize : Grille avant fusion \t" << dPartitionBestCost
						     << "\n";
						cout << "CCOptimize : Grille fusionnee  \t" << dMergedCost << "\n";
						partitionedPostMergedOptimizedDataGrid->Write(cout);
						cout << flush;
					}
				}
				else
					dMergedCost = dPartitionBestCost;

				if (dMergedCost < dBestMergedCost - dEpsilon)
				{
					dBestMergedCost = dMergedCost;
					dBestCost = dPartitionBestCost;

					if (bDisplayResults)
						cout << "CCCoclusteringBuilder : amelioration du cout et memorisation "
							"de la grille sans post-optimisation VarPart"
						     << endl;

					// Memorisation de l'optimum post-fusionne
					if (partitionedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
					    dataGridOptimizer.GetParameters()->GetVarPartPostMerge())
					{
						dataGridManager.CopyDataGrid(partitionedPostMergedOptimizedDataGrid,
									     optimizedDataGrid);

						// Mise a jour de la propriete de la description des parties de variable
						partitionedPostMergedOptimizedDataGrid->SetVarPartsShared(true);
						optimizedDataGrid->SetVarPartsShared(false);
					}

					else
					{
						dataGridManager.CopyDataGrid(partitionedOptimizedDataGrid,
									     optimizedDataGrid);

						// Mise a jour de la propriete de la description des parties de variable
						partitionedOptimizedDataGrid->SetVarPartsShared(true);
						optimizedDataGrid->SetVarPartsShared(false);
					}
				}

				// Cas ou il s'agit de la derniere granularite : on met a jour les infos du coclustering
				if (bIsLastPrePartitioning)
				{
					if (bDisplayResults)
						cout << "CCOptimize :Mise a jour de la memorisation du coclustering "
							"pour la derniere granularite "
						     << endl;

					if (optimizedDataGrid->GetInformativeAttributeNumber() > 0)
					{
						// Construction d'une grille initiale compatible avec les parties de
						// variables fusionnees au niveau des attributs internes Necessaire pour
						// la memorisation de la grille post-mergee La grille source contient
						// des clusters mono-parties de variables, avec des PV issues du
						// pre-partitionnement La grille optimisee contient des clusters de PV,
						// avec des PV eventuellement issues d'une fusion des PV de la grille
						// source On construit une grille qui contient des clusters mono-PV avec
						// les PV issues de la fusion de la grille optimisee
						dataGridManager.SetSourceDataGrid(initialDataGrid);
						dataGridManager.ExportDataGridWithSingletonVarParts(
						    optimizedDataGrid, &partitionedReferencePostMergedDataGrid, true);

						if (bDisplayResults)
							cout << "CCCoclusteringBuilder : memorisation d'une grille "
								"potentiellement sans post-optimisation VarPart"
							     << endl;

						HandleOptimizationStep(optimizedDataGrid,
								       &partitionedReferencePostMergedDataGrid, true);

						if (bDisplayResults)
						{
							cout << "CCOptimize :partitionedReferencePostMergedDataGrid"
							     << endl;
							partitionedReferencePostMergedDataGrid.Write(cout);
							cout << "CCOptimize :Derniere grille apres "
								"HandleOptimizationStep de cout \t"
							     << dBestMergedCost << endl;
							optimizedDataGrid->Write(cout);
						}
					}
					else
					{
						HandleOptimizationStep(optimizedDataGrid, partitionedDataGrid, true);

						if (optimizedDataGrid->GetInnerAttributes() ==
						    partitionedDataGrid->GetInnerAttributes())
						{
							// CH IV Refactoring: nettoyer lignes suivantes?
							// assert pouvait ne pas etre verifie si on avait deja modifie
							// pour nPartitionIndex precedent
							/*cout << "optimizedDataGrid et coclusteringDG meme
							InnerAttributes \t nPartitionIndex" << nPartitionIndex << "\n";
							assert(coclusteringDataGrid->GetVarPartsShared() == true);
							assert(optimizedDataGrid->GetVarPartsShared() == false);*/

							partitionedDataGrid->SetVarPartsShared(true);
							optimizedDataGrid->SetVarPartsShared(false);
						}
					}
				}

				if (coclusteringDataGrid != NULL)
				{
					if (partitionedDataGrid->GetInnerAttributes() ==
					    coclusteringDataGrid->GetInnerAttributes())
					{
						// CH IV Refactoring: nettoyer lignes suivantes?
						/*cout << "partitionedDataGrid et coclusteringDG meme InnerAttributes \t
						   nPartitionIndex" << nPartitionIndex << "\n";
							assert(coclusteringDataGrid->GetVarPartsShared() == true);
							assert(partitionedDataGrid->GetVarPartsShared() == false);*/
						coclusteringDataGrid->SetVarPartsShared(false);
						partitionedDataGrid->SetVarPartsShared(true);
					}

					if (partitionedPostMergedOptimizedDataGrid->GetInnerAttributes() ==
					    coclusteringDataGrid->GetInnerAttributes())
					{
						// CH IV Refactoring: nettoyer lignes suivantes?
						/*cout << "partitionedDataGrid et coclusteringDG meme InnerAttributes \t
						   nPartitionIndex" << nPartitionIndex << "\n";
							assert(coclusteringDataGrid->GetVarPartsShared() == true);
							assert(partitionedDataGrid->GetVarPartsShared() == false);*/
						coclusteringDataGrid->SetVarPartsShared(false);
						partitionedPostMergedOptimizedDataGrid->SetVarPartsShared(true);
					}

					if (optimizedDataGrid->GetInnerAttributes() ==
					    coclusteringDataGrid->GetInnerAttributes())
					{
						// CH IV Refactoring: nettoyer lignes suivantes?
						// assert pouvait ne pas etre verifie si on avait deja modifie pour
						// nPartitioecedent
						/*cout << "optimizedDataGrid et coclusteringDG meme InnerAttributes \t
						   nPartitionIndex" << nPartitionIndex << "\n";
							assert(coclusteringDataGrid->GetVarPartsShared() == true);
							assert(optimizedDataGrid->GetVarPartsShared() == false);*/

						coclusteringDataGrid->SetVarPartsShared(false);
						optimizedDataGrid->SetVarPartsShared(true);
					}
				}

				// Nettoyage
				delete partitionedPostMergedOptimizedDataGrid;
				partitionedReferencePostMergedDataGrid.DeleteAll();

				// Nettoyage de la grille optimisee pour cette granularite
				delete partitionedOptimizedDataGrid;
				partitionedOptimizedDataGrid = NULL;

				// Cas d'un temps limite : mise a jour du temps restant par retrait du temps consacre a
				// cette granularite
				if (dataGridOptimizer.GetParameters()->GetOptimizationTime() > 0)
				{
					// L'utilisation de la totalite du temps global alloue (OptimizationTime) peut
					// conduire a l'arret du parcours des granularites et nuire a la recherche de la
					// grille optimale
					if (dataGridOptimizer.GetParameters()->GetOptimizationTime() -
						dTotalComputeTime <
					    0)
					{
						break;
						// Affichage d'un warning pour eventuelle modification de l'optimisation
						// time
						AddWarning(sTmp +
							   "All the optimization time has been used but maximum "
							   "granularity has not been reached:" +
							   IntToString(nPrePartitionIndex) + " on " +
							   IntToString(nPrePartitionMax) +
							   ". You could obtain better results with greater "
							   "optimization time.");
						if (bDisplayResults)
							cout << "Totalite du temps alloue ecoule apres la granularite "
								"de pre-partitionnement de l'attribut VarPart \t"
							     << nPrePartitionIndex << endl;
					}
				}

				// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
				ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
			}

			// Sinon : pas de traitement pour cette granularite
			else
			{
				if (bDisplayResults)
					cout << "CCOptimize :Pre-partitionnement des attributs internes "
					     << nPrePartitionIndex << " non traite car trop proche de la precedente"
					     << endl;
			}

			// Nettoyage de la grille granularisee
			delete partitionedDataGrid;

			nPrePartitionIndex++;
		}
		if (bDisplayPrePartitioning)
		{
			cout << "Recapitulatif des pre-partitionnements utilises " << endl;
			for (nPrePartitionIndex = 0; nPrePartitionIndex < ivUsedPrePartitioning.GetSize();
			     nPrePartitionIndex++)
				cout << ivUsedPrePartitioning.GetAt(nPrePartitionIndex) << endl;
		}

		// Nettoyage
		odInnerAttributesQuantileBuilders.DeleteAll();
		delete nullDataGrid;
	}
	// CH IV End

	// CH IV probleme est ce que le code qui suit est a conserver
	//  Cas ou la grille terminale n'est pas ameliorable else
	//{
	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	// optimizedDataGrid->SortAttributeParts();
	//}
	//}
}

void CCCoclusteringBuilder::OptimizeVarPartDataGrid(const KWDataGrid* inputInitialDataGrid,
						    KWDataGrid* optimizedDataGrid,
						    CCCoclusteringOptimizer dataGridOptimizer)
{
	boolean bDisplayResults = false;
	boolean bDisplayPartitionLevel = false;
	ObjectDictionary odInnerAttributesQuantileBuilders;
	KWDataGrid* nullDataGrid;
	KWDataGrid* partitionedDataGrid;
	KWDataGrid* partitionedOptimizedDataGrid;
	KWDataGridManager dataGridManager;
	IntVector ivMaxPartNumbers;
	int nPrePartitionIndex;
	int nPrePartitionMax;
	boolean bIsLastPrePartitioning;
	double dRequiredIncreasingCoefficient;
	int nValueNumber;
	IntVector ivPreviousPartNumber;
	IntVector ivCurrentPartNumber;
	boolean bIsPrePartitioningSelected;
	double dPartitionBestCost;
	double dBestCost;
	int nInnerAttribute;
	int nAttribute;
	int nInnerAttributeCumulated;
	KWDGAttribute* attribute;
	KWDGAttribute* innerAttribute;
	KWDataGrid* partitionedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	double dMergedCost;
	double dBestMergedCost;
	double dFusionDeltaCost;
	IntVector ivUsedPrePartitioning;
	boolean bDisplayPrePartitioning = false;
	double dEpsilon = 1e-6;
	double dTotalComputeTime;
	ALString sTmp;

	// On parcourt les differentes tailles de pre-partitionnement des attributs de type VarPart par puissance de 2
	// Ce parcours est similaire a celui des granularites mais il porte exclusivement sur le partitionnement des
	// parties de variable des attributs internes A chaque pre-partitionnement est associe un cout seuil par defaut
	// : c'est le cout du pseudo-modele nul qui contient 1 cluster par attribut avec le nombre de parties de
	// variables du pre-partitionnement Le vrai modele nul contient un cluster par attribut et une seule partie de
	// variable par attribut interne dans l'attribut de grile de type VarPart
	nValueNumber = initialDataGrid->GetGridFrequency();
	nPrePartitionMax = (int)ceil(log(nValueNumber * 1.0) / log(2.0));

	// Initialisation
	nPrePartitionIndex = 1;
	bIsLastPrePartitioning = false;

	// Initialisation du facteur d'accroissement requis entre deux pre-partitionnements
	dRequiredIncreasingCoefficient = 2;

	// Initialisation d'un quantile builder pour chaque attribut interne dans un attribut de grille de type VarPart
	// La grille initiale comporte un cluster par partie de variable pour ses attributs de grille de type VarPart
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridManager.InitializeQuantileBuildersForVariablePartsPartitioning(&odInnerAttributesQuantileBuilders,
									       &ivMaxPartNumbers);

	if (bDisplayPrePartitioning)
	{
		cout << "ivMaxPartNumbers\t" << ivMaxPartNumbers;
		cout << flush;
	}

	// Initialisation des vecteurs de nombre de parties courant et precedent
	ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
	ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

	// CH AB AF peut ne pas etre necessaire car ces couts par defaut sont remplaces ensuite par ceux associes au
	// premier partitionnement Initialisation des couts par defaut
	coclusteringDataGridCosts->InitializeDefaultCosts(inputInitialDataGrid);

	// Initialisation de l'optimiseur pour que la grille initiale puisse servir a l'export des grilles avec PV
	// fusionnnees
	dataGridOptimizer.SetInitialDataGrid(initialDataGrid);

	// Export de la grille du (vrai) modele nul : un seul cluster par attribut et une seule partie de variable par
	// attribut interne
	nullDataGrid = new KWDataGrid;
	dataGridManager.ExportNullDataGrid(nullDataGrid);
	dAnyTimeDefaultCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(nullDataGrid);
	dataGridManager.CopyDataGrid(nullDataGrid, optimizedDataGrid);
	dBestCost = dAnyTimeDefaultCost;
	dBestMergedCost = dBestCost;

	// CH AB n'est plus necessaire c'est le cout du vrai modele nul qui doit etre utilise comme reference
	// Initialisation du meilleur cout au cout du modele nul conditionnellement au pre-partitionnement
	// Il ne s'agit donc pas ici du cout du VRAI modele nul (un seul cluster par attribut et une seule partie de
	// variable par attribut interne)
	// dBestCost = coclusteringDataGridCosts->GetTotalDefaultCost();
	// dBestMergedCost = dBestCost;
	// cout << "Cout du modele nul associe a la grille de reference\t" << dBestCost << "\n";

	// Parcours des tailles de pre-partitionnement en parties de variable
	while (nPrePartitionIndex <= nPrePartitionMax and not bIsLastPrePartitioning)
	{
		// Arret si interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Pre-partitionnement des attributs internes de la grille initiale
		dataGridManager.SetSourceDataGrid(initialDataGrid);
		partitionedDataGrid = new KWDataGrid;
		dataGridManager.ExportPartitionedDataGridForVarPartAttributes(partitionedDataGrid, nPrePartitionIndex,
									      &odInnerAttributesQuantileBuilders);

		if (bDisplayResults)
		{
			cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement " << nPrePartitionIndex
			     << endl;
			partitionedDataGrid->Write(cout);
		}

		// Etude du nombre de parties des attributs internes pour decider du traitement ou non de ce
		// pre-partitionnement Parcours des attributs
		nInnerAttributeCumulated = 0;
		for (nAttribute = 0; nAttribute < partitionedDataGrid->GetAttributeNumber(); nAttribute++)
		{
			attribute = partitionedDataGrid->GetAttributeAt(nAttribute);

			// Traitement dans le cas VarPart uniquement
			if (attribute->GetAttributeType() == KWType::VarPart)
			{
				for (nInnerAttribute = 0; nInnerAttribute < attribute->GetInnerAttributeNumber();
				     nInnerAttribute++)
				{
					innerAttribute =
					    attribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(
						attribute->GetInnerAttributeNameAt(nInnerAttribute));

					// Memorisation du nombre de parties de l'attribut interne granularise
					ivCurrentPartNumber.SetAt(nInnerAttributeCumulated,
								  innerAttribute->GetPartNumber());
					nInnerAttributeCumulated++;
				}
			}
		}

		// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
		// pre-partitionnee est la derniere
		bIsLastPrePartitioning = true;

		// Si on n'a pas encore atteint la granularite max pour les attributs internes partitionnes
		if (nPrePartitionIndex < nPrePartitionMax)
		{
			for (nAttribute = 0; nAttribute < ivMaxPartNumbers.GetSize(); nAttribute++)
			{
				// Cas ou le nombre de parties de l'attribut courant est inferieur au nombre max de
				// parties de l'attribut
				if (ivCurrentPartNumber.GetAt(nAttribute) < ivMaxPartNumbers.GetAt(nAttribute))
				{
					bIsLastPrePartitioning = false;
					break;
				}
			}
		}

		// CH a supprimer car granu max sur N nbre total d'observations et pas sur Nk nombre d'observations par
		// variable -> commentaire a comprendre ! Cas ou cette granularite sera la derniere traitee
		if (bIsLastPrePartitioning)
			// On positionne l'index de granularite au maximum afin que l'affichage soit adapte a ce cas
			partitionedDataGrid->GetInnerAttributes()->SetVarPartGranularity(nPrePartitionMax);

		// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
		// pre-partitionnee sera optimise Il faut pour cela qu'elle soit suffisamment differente de la grille
		// analysee precedemment
		bIsPrePartitioningSelected = false;
		for (nAttribute = 0; nAttribute < ivCurrentPartNumber.GetSize(); nAttribute++)
		{
			// Cas d'accroissement suffisant du nombre de parties : le cas d'un attribut suffit pour
			// justifier le traitement de cette granularite
			if ((ivCurrentPartNumber.GetAt(nAttribute) >=
			     ivPreviousPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient) and
			    (ivCurrentPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient <=
			     ivMaxPartNumbers.GetAt(nAttribute)))
			{
				bIsPrePartitioningSelected = true;
				break;
			}
		}

		// On ne traite pas les grilles avec un seul attribut informatif
		if (partitionedDataGrid->GetInformativeAttributeNumber() <= 1)
			bIsPrePartitioningSelected = false;

		// Cas du traitement de la granularite courante
		if (bIsPrePartitioningSelected or bIsLastPrePartitioning)
		{
			// Affichage du niveau de pre-partitionnement et du nombre de parties associe
			if (bDisplayPartitionLevel)
				cout << "Niveau de pre-partitionnement\t" << nPrePartitionIndex
				     << "\tNombre de parties \t" << ivCurrentPartNumber;

			// Memorisation des pre-partitionnements exploites
			ivUsedPrePartitioning.Add(nPrePartitionIndex);

			// Initialisation de la grille pre-partitionnee optimisee
			partitionedOptimizedDataGrid = new KWDataGrid;

			// Initialisation du modele par defaut : ce modele depend du partitionnement des attributs
			// internes
			coclusteringDataGridCosts->InitializeDefaultCosts(partitionedDataGrid);

			// Optimisation de la grille pre-partitionnee
			// Le cout dPartitionBestCost est le cout de la grille antecedente de la meilleure grille
			// post-fusionnee (fusion des parties de variables consecutives dans un meme cluster)
			dPartitionBestCost =
			    dataGridOptimizer.OptimizeDataGrid(partitionedDataGrid, partitionedOptimizedDataGrid);

			// Calcul du temps d'optimisation (depuis le debut)
			tAnyTimeTimer.Stop();
			dTotalComputeTime = tAnyTimeTimer.GetElapsedTime(), tAnyTimeTimer.Start();

			if (bDisplayResults)
			{
				cout << "Apres OptimizeGranularizedDataGrid pour Granularite " << nPrePartitionIndex
				     << "\t Cout " << dPartitionBestCost << endl;
				partitionedOptimizedDataGrid->WriteAttributes(cout);
				partitionedOptimizedDataGrid->WriteAttributeParts(cout);
			}

			// Creation d'une grille post-mergee pour cette granularite de pre-partitionnement
			// CH AF Il faudrait aussi recalculer la post-optimisation VarPart de la grille post-mergee
			// mais de toute facon cette meilleure grille aura ete memorisee avec HandleOptimisationStep
			// dans l'algo VNS -> commentaire a comprendre
			partitionedPostMergedOptimizedDataGrid = new KWDataGrid;

			if (partitionedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
			    dataGridOptimizer.GetParameters()->GetVarPartPostMerge())
			{
				dataGridManager.SetSourceDataGrid(partitionedOptimizedDataGrid);
				// Creation d'une nouvelle grille avec nouvelle description des PV et calcul de la
				// variation de cout liee a la fusion des PV
				dFusionDeltaCost = dataGridManager.ExportMergedDataGridForVarPartAttributes(
				    partitionedPostMergedOptimizedDataGrid, coclusteringDataGridCosts);
				assert(not partitionedPostMergedOptimizedDataGrid->GetVarPartsShared());

				// Calcul et verification du cout
				dMergedCost = dPartitionBestCost + dFusionDeltaCost;
				// Le cout precedent devra etre correct
				assert(dMergedCost * (1 - dEpsilon) <
				       coclusteringDataGridCosts->ComputeDataGridTotalCost(
					   partitionedPostMergedOptimizedDataGrid));
				assert(coclusteringDataGridCosts->ComputeDataGridTotalCost(
					   partitionedPostMergedOptimizedDataGrid) < dMergedCost * (1 + dEpsilon));

				if (bDisplayResults)
				{
					cout << "CCOptimize : Grille avant fusion \t" << dPartitionBestCost << "\n";
					cout << "CCOptimize : Grille fusionnee  \t" << dMergedCost << "\n";
					partitionedPostMergedOptimizedDataGrid->Write(cout);
					cout << flush;
				}
			}
			else
				dMergedCost = dPartitionBestCost;

			if (dMergedCost < dBestMergedCost - dEpsilon)
			{
				dBestMergedCost = dMergedCost;
				dBestCost = dPartitionBestCost;

				if (bDisplayResults)
					cout << "CCCoclusteringBuilder : amelioration du cout et memorisation de la "
						"grille sans post-optimisation VarPart"
					     << endl;

				// Memorisation de l'optimum post-fusionne
				if (partitionedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
				    dataGridOptimizer.GetParameters()->GetVarPartPostMerge())
				{
					dataGridManager.CopyDataGrid(partitionedPostMergedOptimizedDataGrid,
								     optimizedDataGrid);

					// Mise a jour de la propriete de la description des parties de variable
					partitionedPostMergedOptimizedDataGrid->SetVarPartsShared(true);
					optimizedDataGrid->SetVarPartsShared(false);
				}

				else
				{
					dataGridManager.CopyDataGrid(partitionedOptimizedDataGrid, optimizedDataGrid);

					// Mise a jour de la propriete de la description des parties de variable
					partitionedOptimizedDataGrid->SetVarPartsShared(true);
					optimizedDataGrid->SetVarPartsShared(false);
				}
			}

			// Cas ou il s'agit de la derniere granularite : on met a jour les infos du coclustering
			if (bIsLastPrePartitioning)
			{
				if (bDisplayResults)
					cout << "CCOptimize :Mise a jour de la memorisation du coclustering pour la "
						"derniere granularite "
					     << endl;

				if (optimizedDataGrid->GetInformativeAttributeNumber() > 0)
				{
					// Construction d'une grille initiale compatible avec les parties de variables
					// fusionnees au niveau des attributs internes Necessaire pour la memorisation
					// de la grille post-mergee La grille source contient des clusters mono-parties
					// de variables, avec des PV issues du pre-partitionnement La grille optimisee
					// contient des clusters de PV, avec des PV eventuellement issues d'une fusion
					// des PV de la grille source On construit une grille qui contient des clusters
					// mono-PV avec les PV issues de la fusion de la grille optimisee
					dataGridManager.SetSourceDataGrid(initialDataGrid);
					dataGridManager.ExportDataGridWithSingletonVarParts(
					    optimizedDataGrid, &partitionedReferencePostMergedDataGrid, true);

					if (bDisplayResults)
						cout << "CCCoclusteringBuilder : memorisation d'une grille "
							"potentiellement sans post-optimisation VarPart"
						     << endl;

					HandleOptimizationStep(optimizedDataGrid,
							       &partitionedReferencePostMergedDataGrid, true);

					if (bDisplayResults)
					{
						cout << "CCOptimize :partitionedReferencePostMergedDataGrid" << endl;
						partitionedReferencePostMergedDataGrid.Write(cout);
						cout << "CCOptimize :Derniere grille apres HandleOptimizationStep de "
							"cout \t"
						     << dBestMergedCost << endl;
						optimizedDataGrid->Write(cout);
					}
				}
				else
				{
					HandleOptimizationStep(optimizedDataGrid, partitionedDataGrid, true);

					if (optimizedDataGrid->GetInnerAttributes() ==
					    partitionedDataGrid->GetInnerAttributes())
					{
						// assert pouvait ne pas etre verifie si on avait deja modifie pour
						// nPartitionIndex precedent
						/*cout << "optimizedDataGrid et coclusteringDG meme InnerAttributes \t
					nPartitionIndex" << nPartitionIndex << "\n";
					assert(coclusteringDataGrid->GetVarPartsShared() == true);
					assert(optimizedDataGrid->GetVarPartsShared() == false);*/

						partitionedDataGrid->SetVarPartsShared(true);
						optimizedDataGrid->SetVarPartsShared(false);
					}
				}
			}

			if (coclusteringDataGrid != NULL)
			{
				if (partitionedDataGrid->GetInnerAttributes() ==
				    coclusteringDataGrid->GetInnerAttributes())
				{
					/*cout << "partitionedDataGrid et coclusteringDG meme InnerAttributes \t
					nPartitionIndex" << nPartitionIndex << "\n";
					assert(coclusteringDataGrid->GetVarPartsShared() == true);
					assert(partitionedDataGrid->GetVarPartsShared() == false);*/
					coclusteringDataGrid->SetVarPartsShared(false);
					partitionedDataGrid->SetVarPartsShared(true);
				}

				if (partitionedPostMergedOptimizedDataGrid->GetInnerAttributes() ==
				    coclusteringDataGrid->GetInnerAttributes())
				{
					/*cout << "partitionedDataGrid et coclusteringDG meme InnerAttributes \t
					nPartitionIndex" << nPartitionIndex << "\n";
					assert(coclusteringDataGrid->GetVarPartsShared() == true);
					assert(partitionedDataGrid->GetVarPartsShared() == false);*/
					coclusteringDataGrid->SetVarPartsShared(false);
					partitionedPostMergedOptimizedDataGrid->SetVarPartsShared(true);
				}

				if (optimizedDataGrid->GetInnerAttributes() ==
				    coclusteringDataGrid->GetInnerAttributes())
				{
					// assert pouvait ne pas etre verifie si on avait deja modifie pour
					// nPartitioecedent
					/*cout << "optimizedDataGrid et coclusteringDG meme InnerAttributes \t
					nPartitionIndex" << nPartitionIndex << "\n";
					assert(coclusteringDataGrid->GetVarPartsShared() == true);
					assert(optimizedDataGrid->GetVarPartsShared() == false);*/

					coclusteringDataGrid->SetVarPartsShared(false);
					optimizedDataGrid->SetVarPartsShared(true);
				}
			}

			// Nettoyage
			delete partitionedPostMergedOptimizedDataGrid;
			partitionedReferencePostMergedDataGrid.DeleteAll();

			// Nettoyage de la grille optimisee pour cette granularite
			delete partitionedOptimizedDataGrid;
			partitionedOptimizedDataGrid = NULL;

			// Cas d'un temps limite : mise a jour du temps restant par retrait du temps consacre a cette
			// granularite
			if (dataGridOptimizer.GetParameters()->GetOptimizationTime() > 0)
			{
				// L'utilisation de la totalite du temps global alloue (OptimizationTime) peut conduire
				// a l'arret du parcours des granularites et nuire a la recherche de la grille optimale
				if (dataGridOptimizer.GetParameters()->GetOptimizationTime() - dTotalComputeTime < 0)
				{
					break;
					// Affichage d'un warning pour eventuelle modification de l'optimisation time
					AddWarning(sTmp +
						   "All the optimization time has been used but maximum granularity "
						   "has not been reached:" +
						   IntToString(nPrePartitionIndex) + " on " +
						   IntToString(nPrePartitionMax) +
						   ". You could obtain better results with greater optimization time.");
					if (bDisplayResults)
						cout << "Totalite du temps alloue ecoule apres la granularite de "
							"pre-partitionnement de l'attribut VarPart \t"
						     << nPrePartitionIndex << endl;
				}
			}

			// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
			ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
		}

		// Sinon : pas de traitement pour cette granularite
		else
		{
			if (bDisplayResults)
				cout << "CCOptimize :Pre-partitionnement des attributs internes " << nPrePartitionIndex
				     << " non traite car trop proche de la precedente" << endl;
		}

		// Nettoyage de la grille granularisee
		delete partitionedDataGrid;

		nPrePartitionIndex++;
	}
	if (bDisplayPrePartitioning)
	{
		cout << "Recapitulatif des pre-partitionnements utilises " << endl;
		for (nPrePartitionIndex = 0; nPrePartitionIndex < ivUsedPrePartitioning.GetSize(); nPrePartitionIndex++)
			cout << ivUsedPrePartitioning.GetAt(nPrePartitionIndex) << endl;
	}

	// Nettoyage
	odInnerAttributesQuantileBuilders.DeleteAll();
	delete nullDataGrid;
}

boolean CCCoclusteringBuilder::IsCoclusteringComputed() const
{
	return IsStatsComputed();
}

boolean CCCoclusteringBuilder::IsCoclusteringInformative() const
{
	boolean bInformative;
	bInformative = IsCoclusteringComputed() and coclusteringDataGrid != NULL and
		       (coclusteringDataGrid->GetInformativeAttributeNumber() >= 2);
	return bInformative;
}

// CH IV Begin
void CCCoclusteringBuilder::SetAttributeNumber(int nValue)
{
	require(nValue >= 0);
	svAttributeNames.SetSize(nValue);
	svAxisForAttributeNames.SetSize(nValue);
}

void CCCoclusteringBuilder::SetAxisForAttributeNameAt(int nIndex, const ALString& sAxisName)
{
	require(0 <= nIndex and nIndex < svAxisForAttributeNames.GetSize());
	require(GetAxisForAttributeNameAt(nIndex) == "");
	require(sAxisName != GetTargetAttributeName() or sAxisName == "");
	svAxisForAttributeNames.SetAt(nIndex, sAxisName);
}

const ALString& CCCoclusteringBuilder::GetAxisForAttributeNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svAxisForAttributeNames.GetSize());
	return svAxisForAttributeNames.GetAt(nIndex);
}

const ALString& CCCoclusteringBuilder::GetAxisNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svAxisNames.GetSize());
	return svAxisNames.GetAt(nIndex);
}

boolean CCCoclusteringBuilder::ExtractAndCheckAxisNames()
{
	int nAttributeIndex;
	int nAttributeNumber;
	ALString sAxisName;
	ALString sPreviousAttributeName;
	ALString sPreviousAxisName;
	ALString sTmp;
	boolean bFirstVarPartsAttribute;
	boolean bOk;

	bOk = true;
	bFirstVarPartsAttribute = false;
	svAxisNames.SetSize(0);
	ivInnerAttributesNumber.SetSize(0);

	sPreviousAttributeName = svAttributeNames.GetAt(0);
	sPreviousAxisName = svAxisForAttributeNames.GetAt(0);

	nAttributeIndex = 1;

	while (nAttributeIndex < svAttributeNames.GetSize())
	{
		nAttributeNumber = 1;
		if (sPreviousAxisName == "")
		{
			if (bFirstVarPartsAttribute)
			{
				// Les axes de nom vide (attributs simples) doivent etre decrits en premier
				AddError("Simple variables must be all specified before VarPart variables");
				bOk = false;
			}

			// Memorisation de l'attribut simple
			svAxisNames.Add(sPreviousAxisName);
			ivInnerAttributesNumber.Add(nAttributeNumber);

			// Passage a l'attribut suivant
			sPreviousAxisName = svAxisForAttributeNames.GetAt(nAttributeIndex);

			// Cas ou il n'y a que deux attributs : un attribut Identifier et un seul attribut pour
			// l'attribut VarPart
			if (svAttributeNames.GetSize() == 2)
			{
				svAxisNames.Add(sPreviousAxisName);
				ivInnerAttributesNumber.Add(nAttributeNumber);
			}

			nAttributeIndex++;
		}
		else
		{
			bFirstVarPartsAttribute = true;
			while (nAttributeIndex < svAxisForAttributeNames.GetSize() and
			       svAxisForAttributeNames.GetAt(nAttributeIndex) == sPreviousAxisName)
			{
				nAttributeNumber++;

				// Passage a l'attribut suivant
				sPreviousAxisName = svAxisForAttributeNames.GetAt(nAttributeIndex);
				nAttributeIndex++;
			}

			if (nAttributeNumber == 1)
			{
				// Les axes de nom non vide doivent comporter au moins deux variables
				AddError("Less than two variables are specified for axis\t" + sAxisName);
				bOk = false;
			}
			// Sinon : memorisation de cette configuration valide
			else
			{
				svAxisNames.Add(sPreviousAxisName);
				ivInnerAttributesNumber.Add(nAttributeNumber);

				// Passage a l'attribut suivant
				if (nAttributeIndex < svAxisForAttributeNames.GetSize())
				{
					sPreviousAxisName = svAxisForAttributeNames.GetAt(nAttributeIndex);
					nAttributeIndex++;
				}
			}
		}
	}

	// Cas ou le dernier axe est Simple
	if (sPreviousAxisName == "")
	{
		nAttributeNumber = 1;
		if (bFirstVarPartsAttribute)
		{
			// Les axes de nom vide (attributs simples) doivent etre decrits en premier
			AddError("Simple variables must be all specified before VarPart variables");
			bOk = false;
		}

		// Memorisation de l'attribut simple
		svAxisNames.Add(sPreviousAxisName);
		ivInnerAttributesNumber.Add(nAttributeNumber);
	}

	// Il doit y avoir au moins deux axes
	if (svAxisNames.GetSize() < 2)
	{
		AddError("Less than two axes are specified");
		bOk = false;
	}
	// Il y a un nombre limite d'axes
	else if (svAxisNames.GetSize() > 10)
	{
		AddError(sTmp + "Too many coclustering axes (> " +
			 IntToString(CCVarPartCoclusteringSpec::GetMaxCoclusteringAxisNumber()) + ") are specified");
		bOk = false;
	}

	return bOk;
}

int CCCoclusteringBuilder::GetAxisNumber() const
{
	return svAxisNames.GetSize();
}

KWDataGridCosts* CCCoclusteringBuilder::CreateDataGridCost() const
{
	KWDataGridCosts* dataGridCosts;

	require(Check());
	require(CheckSpecifications());

	// Cas d'un coclustering standard
	if (not GetVarPartCoclustering())
		dataGridCosts = new KWDataGridClusteringCosts;
	// Cas d'un coclustering VarPart
	else
		dataGridCosts = new KWVarPartDataGridClusteringCosts;
	check(dataGridCosts);
	return dataGridCosts;
}

KWDataGrid* CCCoclusteringBuilder::CreateVarPartDataGrid(const KWTupleTable* tupleTable,
							 ObjectDictionary& odObservationNumbers)
{
	KWDataGridManager dataGridManager;
	KWDataGrid* dataGrid;
	int nAttribute;
	KWAttribute* attribute;
	int nTupleAttributeIndex;
	KWDGAttribute* dgAttribute;
	boolean bCellCreationOk;
	ALString sAxisName;
	ALString sAttributName;
	int nAxis;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	boolean bFirstVarPartsAttribute = false;
	int nInitialVarPartNumber;
	KWDGInnerAttributes* initialInnerAttributes;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(GetInstanceNumber() == tupleTable->GetTotalFrequency());
	require(GetTargetAttributeName() == "");
	require(GetVarPartCoclustering());

	// Creation du DataGrid initial
	dataGrid = new KWDataGrid;
	dataGrid->Initialize(GetAxisNumber(), 0);

	// Memorisation mode VarPart pour la grille
	dataGrid->SetVarPartDataGrid(true);

	// Creation de la description initiale des parties de variable
	initialInnerAttributes = new KWDGInnerAttributes;
	dataGrid->SetInnerAttributes(initialInnerAttributes);
	dataGrid->SetVarPartsShared(false);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Initialize instances x variables coclustering");

	// Attribut courant (progressivement charges au fil des axes)
	nAttribute = 0;

	// Initialisation des axes et de leurs parties
	for (nAxis = 0; nAxis < dataGrid->GetAttributeNumber(); nAxis++)
	{
		// Extraction du nom de l'axe
		sAxisName = GetAxisNameAt(nAxis);

		// Cas d'un axe de type categoriel (pas de type parties de variables)
		if (sAxisName == "")
		{
			// Il s'agit alors d'un axe identifiant

			// Les axes simples doivent tous etre renseignes avant les axes de type VarPart
			assert(not bFirstVarPartsAttribute);

			// Recherche de l'index de l'attribut dans la table de tuples
			nTupleAttributeIndex = tupleTable->LookupAttributeIndex(GetAttributeNameAt(nAttribute));
			assert(nTupleAttributeIndex >= 0);

			// Recherche de l'attribut du DataGrid correspondant
			dgAttribute = dataGrid->GetAttributeAt(nAxis);

			// Parametrage de l'attribut du DataGrid
			dgAttribute->SetAttributeName(tupleTable->GetAttributeNameAt(nTupleAttributeIndex));
			dgAttribute->SetAttributeType(tupleTable->GetAttributeTypeAt(nTupleAttributeIndex));
			dgAttribute->SetAttributeTargetFunction(dgAttribute->GetAttributeName() ==
								GetTargetAttributeName());

			// Recuperation du cout de selection/construction de l'attribut hors attribut cible
			if (dgAttribute->GetAttributeName() != GetTargetAttributeName())
			{
				attribute = GetClass()->LookupAttribute(dgAttribute->GetAttributeName());
				check(attribute);
				dgAttribute->SetCost(attribute->GetCost());
			}

			// Creation des parties de l'attribut selon son type
			TaskProgression::DisplayLabel("Initialize " + dgAttribute->GetAttributeName() + " parts");
			// L'attribut Identifiant doit etre de type categoriel
			if (dgAttribute->GetAttributeType() == KWType::Continuous)
				AddWarning("Identifier variable " + dgAttribute->GetAttributeName() +
					   "\t must be categorical variable");
			else
				CreateIdentifierAttributeValueSets(tupleTable, dgAttribute, odObservationNumbers);

			// Test interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
				break;

			// Incrementation de l'attribut courant a charger
			nAttribute++;
		}
		// Sinon, axe de type parties de variables
		else
		{
			// Cas du premier axe de ce type rencontre
			if (not bFirstVarPartsAttribute)
			{
				bFirstVarPartsAttribute = true;

				// Memorisation de son index
				nFirstVarPartsAttributeIndex = nAxis;
			}

			// Recherche de l'attribut du DataGrid correspondant
			dgAttribute = dataGrid->GetAttributeAt(nAxis);

			// Parametrage de l'attribut du DataGrid de type Parties de variables
			dgAttribute->SetAttributeName(sAxisName);
			dgAttribute->SetAttributeType(KWType::VarPart);

			// Pas de cout de selection/construction pour ce type d'attribut
			// Necessairement pas un attribut cible

			// Creation du tableau des attributs internes
			dgAttribute->SetInnerAttributeNumber(ivInnerAttributesNumber.GetAt(nAxis));

			nInitialVarPartNumber = 0;
			for (nInnerAttribute = 0; nInnerAttribute < dgAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				// Recherche de l'index de l'attribut dans la table de tuples
				nTupleAttributeIndex = tupleTable->LookupAttributeIndex(GetAttributeNameAt(nAttribute));
				assert(nTupleAttributeIndex >= 0);

				// Recherche de l'attribut du DataGrid correspondant
				innerAttribute = new KWDGAttribute;

				// Parametrage de l'axe de l'attribut
				innerAttribute->SetOwnerAttributeName(dgAttribute->GetAttributeName());

				// Parametrage de l'attribut du DataGrid
				innerAttribute->SetAttributeName(tupleTable->GetAttributeNameAt(nTupleAttributeIndex));
				innerAttribute->SetAttributeType(tupleTable->GetAttributeTypeAt(nTupleAttributeIndex));
				innerAttribute->SetAttributeTargetFunction(innerAttribute->GetAttributeName() ==
									   GetTargetAttributeName());

				// Ajout de l'attribut dans le dictionnaire des attributs internes de la grille
				dataGrid->GetInnerAttributes()->AddInnerAttribute(innerAttribute);
				dgAttribute->SetInnerAttributeNameAt(nInnerAttribute,
								     innerAttribute->GetAttributeName());

				// Recuperation du cout de selection/construction de l'attribut hors attribut cible
				if (innerAttribute->GetAttributeName() != GetTargetAttributeName())
				{
					attribute = GetClass()->LookupAttribute(innerAttribute->GetAttributeName());
					check(attribute);
					innerAttribute->SetCost(attribute->GetCost());
				}

				// Creation des parties de l'attribut interne selon son type
				TaskProgression::DisplayLabel("Initialize " + dgAttribute->GetAttributeName() +
							      " parts");
				if (innerAttribute->GetAttributeType() == KWType::Continuous)
					CreateAttributeIntervals(tupleTable, innerAttribute);
				else
					CreateAttributeValueSets(tupleTable, innerAttribute);

				// Test interruption utilisateur
				if (TaskProgression::IsInterruptionRequested())
					break;

				// Mise a jour du nombre total de parties de variables de l'attribut
				nInitialVarPartNumber += innerAttribute->GetPartNumber();

				// Incrementation de l'attribut courant a charger
				nAttribute++;
			}

			// Parametrage du nombre initial de parties de variable
			dgAttribute->SetInitialValueNumber(nInitialVarPartNumber);

			// Tri du tableau des noms des attributs internes
			dgAttribute->SortInnerAttributeNames();

			// Creation des parties de l'attribut de grile de type de type VarPart
			// A la creation, une partie est un cluster de parties de variables qui ne contient qu'une
			// partie de variable
			dgAttribute->CreateVarPartsSet();
		}
	}

	// On force le calcul des statistiques sur les attributs informatifs
	if (not TaskProgression::IsInterruptionRequested())
	{
		dataGrid->SetCellUpdateMode(true);
		dataGrid->SetCellUpdateMode(false);
	}

	// Supression des attributs non informatifs
	// DDDDD cela fausse le nombre d'attributs initiaux
	// Cela pose de gros probleme pour gerer les grilles sans attributs
	// dataGrid->DeleteNonInformativeAttributes();

	// Creation des cellules
	bCellCreationOk = true;
	if (not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Initialize cells");

		bCellCreationOk = CreateDataGridCells(tupleTable, dataGrid);
	}

	// Nettoyage des eventuelles parties vides du fait d'observations manquantes
	CleanVarPartDataGrid(dataGrid);

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Destruction de la grille si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested() or not bCellCreationOk)
	{
		delete dataGrid;
		dataGrid = NULL;
	}

	ensure(dataGrid == NULL or dataGrid->Check());

	return dataGrid;
}

void CCCoclusteringBuilder::CleanVarPartDataGrid(KWDataGrid* dataGrid)
{
	ObjectArray oaVarParts;
	int nVarPart;
	int nAxis;
	int nInnerAttribute;
	KWDGAttribute* dgAttribute;
	KWDGAttribute* innerAttribute;

	for (nAxis = 0; nAxis < dataGrid->GetAttributeNumber(); nAxis++)
	{
		dgAttribute = dataGrid->GetAttributeAt(nAxis);
		if (dgAttribute->GetAttributeType() == KWType::VarPart)
		{
			// Export des parties de l'attribut VartPart
			dgAttribute->ExportParts(&oaVarParts);
			for (nVarPart = 0; nVarPart < oaVarParts.GetSize(); nVarPart++)
			{
				if (cast(KWDGPart*, oaVarParts.GetAt(nVarPart))->GetPartFrequency() == 0)
					dgAttribute->DeletePart(cast(KWDGPart*, oaVarParts.GetAt(nVarPart)));
			}
			oaVarParts.SetSize(0);
		}
	}
	// Nettoyage des attributs comportant des valeurs manquantes
	for (nInnerAttribute = 0; nInnerAttribute < dataGrid->GetInnerAttributes()->GetInnerAttributeNumber();
	     nInnerAttribute++)
	{
		innerAttribute = dataGrid->GetInnerAttributes()->GetInnerAttributeAt(nInnerAttribute);
		if (innerAttribute->GetAttributeType() == KWType::Continuous)
		{
			innerAttribute->ExportParts(&oaVarParts);
			for (nVarPart = 0; nVarPart < oaVarParts.GetSize(); nVarPart++)
			{
				if (cast(KWDGPart*, oaVarParts.GetAt(nVarPart))->GetPartFrequency() == 0)
					innerAttribute->DeletePart(cast(KWDGPart*, oaVarParts.GetAt(nVarPart)));
			}
			oaVarParts.SetSize(0);
		}
	}
	dataGrid->UpdateAllStatistics();
}

// boolean CCCoclusteringBuilder::CreateAttributeVarPartsSet(KWDGAttribute* dgAttribute)
//{
//	KWDGPart* part;
//	KWDGVarPart* varPart;
//	KWDGPart* currentPart;
//	KWDGVarPart* currentVarPart;
//	KWDGAttribute* innerAttribute;
//	int nAttribute;
//	boolean bOk = true;
//
//	require(dgAttribute->GetInnerAttributeNumber() > 0);
//
//	for (nAttribute = 0; nAttribute < dgAttribute->GetInnerAttributeNumber(); nAttribute++)
//	{
//		innerAttribute = dgAttribute->GetInnerAttributesAt(nAttribute);
//		currentPart = innerAttribute->GetHeadPart();
//		currentVarPart = cast(KWDGVarPart*, currentPart);
//		// Parcours des parties de variables
//		while (currentVarPart != NULL)
//		{
//			// On cree un cluster par parties de variables
//			part = dgAttribute->AddPart();
//			part->GetVarPartSet()->AddVarPart(currentVarPart);
//
//			// Partie suivante
//			innerAttribute->GetNextPart(currentPart);
//			currentVarPart = cast(KWDGVarPart*, currentPart);
//		}
//	}
//
//	return bOk;
// }

boolean CCCoclusteringBuilder::CreateDataGridCells(const KWTupleTable* tupleTable, KWDataGrid* dataGrid)
{
	boolean bOk = true;
	boolean bDisplayInstanceCreation = false;
	ObjectArray oaParts;
	int nTuple;
	const KWTuple* tuple;
	Continuous cValue;
	Symbol sValue;
	KWDGPart* part;
	IntVector ivAttributeIndexes;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGCell* cell;
	int nCellFrequency;
	ALString sTmp;
	// CH AB
	ObjectArray oaIndexes;
	// int nObservationIndex;           // CH IV Refactoring: supprimer car non utilise
	// IntVector* ivObservationIndexes; // CH IV Refactoring: supprimer car non utilise
	int nInnerAttributeIndex;
	KWDGAttribute* innerAttribute;
	KWDGPart* varPart;
	int nInnerAttribute;
	// CH IV A supprimer
	// boolean bNewInitialization = true;

	// Cas ou le coclustering se ramene a un coclustering standard, appel de la methode standard de creation de
	// cellules
	if (not GetVarPartCoclustering())
		bOk = KWAttributeSubsetStats::CreateDataGridCells(tupleTable, dataGrid);
	// Sinon, cas reel de coclustering VarPart
	else
	{
		require(Check());
		require(CheckSpecifications());
		require(tupleTable != NULL);
		require(tupleTable->GetSize() > 0);
		require(dataGrid != NULL);
		require(dataGrid->GetAttributeNumber() <= tupleTable->GetAttributeNumber());
		require(dataGrid->GetCellNumber() == 0);
		require(dataGrid->Check());
		require(nFirstVarPartsAttributeIndex >= 0);

		// Passage en mode update
		Global::ActivateErrorFlowControl();
		dataGrid->SetCellUpdateMode(true);
		dataGrid->BuildIndexingStructure();

		// Recherche des index des attributs dans la table de tuples contenant les donnees
		ivAttributeIndexes.SetSize(nFirstVarPartsAttributeIndex);

		for (nAttribute = 0; nAttribute < nFirstVarPartsAttributeIndex; nAttribute++)
		{
			dgAttribute = dataGrid->GetAttributeAt(nAttribute);

			// Memorisation de son index
			ivAttributeIndexes.SetAt(nAttribute,
						 tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()));
			assert(ivAttributeIndexes.GetAt(nAttribute) != -1);
		}

		// CH IV a supprimer
		// if (not bNewInitialization)
		//{
		//	// Calcul de toutes les combinaisons possibles d'observations d'attributs pour les axes de type
		// VarPart 	FillObjectArrayVarPartsAttributesIndexes(&oaIndexes, nFirstVarPartsAttributeIndex,
		// dataGrid);
		//}

		// Ajout d'instances dans le DataGrid
		oaParts.SetSize(dataGrid->GetAttributeNumber());

		// oaInnerAttributes.SetSize(dataGrid->GetAttributeNumber());

		// Parcours des tuples
		for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
		{
			tuple = tupleTable->GetAt(nTuple);

			// Progression
			if (periodicTestDisplay.IsTestAllowed(nTuple))
			{
				TaskProgression::DisplayProgression((int)(50 + nTuple * 50.0 / tupleTable->GetSize()));
				if (TaskProgression::IsInterruptionRequested())
					break;
			}

			// Recherche de l'effectif de la cellule
			nCellFrequency = tuple->GetFrequency();

			// Recherche des parties pour les valeurs de l'objet courant pour les attributs de type Simple
			// (avant l'attribut de type VarPart)
			for (nAttribute = 0; nAttribute < nFirstVarPartsAttributeIndex; nAttribute++)
			{
				dgAttribute = dataGrid->GetAttributeAt(nAttribute);

				// Recherche de la partie associee a la valeur selon son type
				if (dgAttribute->GetAttributeType() == KWType::Continuous)
				{
					cValue = tuple->GetContinuousAt(ivAttributeIndexes.GetAt(nAttribute));
					part = dgAttribute->LookupContinuousPart(cValue);
					oaParts.SetAt(nAttribute, part);
					if (bDisplayInstanceCreation)
						cout << cValue << "\t";
				}
				else if (dgAttribute->GetAttributeType() == KWType::Symbol)
				{
					sValue = tuple->GetSymbolAt(ivAttributeIndexes.GetAt(nAttribute));
					part = dgAttribute->LookupSymbolPart(sValue);
					oaParts.SetAt(nAttribute, part);
					if (bDisplayInstanceCreation)
						cout << sValue << "\t";
				}
				// oaInnerAttributes.SetAt(nAttribute, NULL);
			}

			// CH IV Refactoring: nettoyer lignes suivantes?
			// if (not bNewInitialization)
			//{
			//	// Parcours de toutes les configurations d'observations
			//	for (nObservationIndex = 0; nObservationIndex < oaIndexes.GetSize();
			// nObservationIndex++)
			//	{
			//		ivObservationIndexes = cast(IntVector*, oaIndexes.GetAt(nObservationIndex));
			//		// Parcours des axes de type VarPart
			//		// En fait ce parcours ne contient qu'un axe
			//		for (nAttribute = nFirstVarPartsAttributeIndex; nAttribute <
			// dataGrid->GetAttributeNumber(); nAttribute++)
			//		{
			//			// Extraction de l'attribut VarPart
			//			dgAttribute = dataGrid->GetAttributeAt(nAttribute);

			//			// Parcours des attributs internes de attribut de type VarPart
			//			for (nInnerAttribute = 0; nInnerAttribute <
			// dgAttribute->GetInnerAttributeNumber(); nInnerAttribute++)
			//			{
			//				// Extraction de l'attribut interne pour l'observation courante
			//				innerAttribute =
			// dgAttribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(dgAttribute->GetInnerAttributeNameAt(ivObservationIndexes->GetAt(nAttribute
			//- nFirstVarPartsAttributeIndex)));

			//				nInnerAttributeIndex =
			// tupleTable->LookupAttributeIndex(innerAttribute->GetAttributeName());

			//				// Recherche de la partie associee a la valeur selon son type
			//				if (innerAttribute->GetAttributeType() == KWType::Continuous)
			//				{
			//					cValue = tuple->GetContinuousAt(nInnerAttributeIndex);
			//					// Cas d'une valeur manquante
			//					if (cValue == KWContinuous::GetMissingValue())
			//						break;
			//					else
			//						part =
			// innerAttribute->LookupContinuousPart(cValue); 					if
			// (bDisplayInstanceCreation) 						cout << cValue <<
			//"\t";
			//				}
			//				else if (innerAttribute->GetAttributeType() == KWType::Symbol)
			//				{
			//					sValue = tuple->GetSymbolAt(nInnerAttributeIndex);
			//					part = innerAttribute->LookupSymbolPart(sValue);
			//					if (bDisplayInstanceCreation)
			//						cout << sValue << "\t";
			//				}

			//				// Mise a jour de l'effectif de la partie de variable
			//				part->SetPartFrequency(part->GetPartFrequency() +
			// nCellFrequency);

			//				// Extraction de la VarPartSet de l'axe associee a cette partie
			//				varPart = dgAttribute->LookupVarPart(part);
			//				oaParts.SetAt(nAttribute, varPart);
			//			}

			//			// Recherche de la cellule
			//			cell = dataGrid->LookupCell(&oaParts);

			//			// Creation si necessaire, en tenant compte si demande d'une contrainet
			// sur le nombre max de cellules 			if (cell == NULL)
			//			{
			//				// On cree une cellule si l'on ne depasse pas le nombre max
			//				if (nMaxCellNumberConstraint == 0 or dataGrid->GetCellNumber() <
			// nMaxCellNumberConstraint) 					cell =
			// dataGrid->AddCell(&oaParts);
			//				// Sinon, on arrete la creation (sauf en mode memoire "risque")
			//				else
			//				{
			//					// Message d'erreur si limite atteinte
			//					if (dataGrid->GetCellNumber() ==
			// nMaxCellNumberConstraint)
			//					{
			//						AddError(sTmp + "Not enough memory to create
			// initial data grid, too many data grid cells have been created (" +
			// IntToString(nMaxCellNumberConstraint) +
			//") and " +
			// IntToString((int)ceil((tupleTable->GetSize() - nTuple) * 100.0 / tupleTable->GetSize())) +
			//							 "% of the database remains to
			// analyse");
			// AddMessage(RMResourceManager::BuildMemoryLimitMessage());
			//					}

			//					// Creation en mode risque
			//					if (RMResourceConstraints::GetIgnoreMemoryLimit())
			//					{
			//						// Message d'avertissement la premiere fois
			//						if (dataGrid->GetCellNumber() ==
			// nMaxCellNumberConstraint)
			// RMResourceManager::DisplayIgnoreMemoryLimitMessage();

			//						// Creation de la cellule
			//						cell = dataGrid->AddCell(&oaParts);
			//					}
			//					// Sinon, on arrete
			//					else
			//					{
			//						bOk = false;
			//						break;
			//					}
			//				}
			//			}

			//			// Mise a jour de la cellule directement (pas d'attribut cible)
			//			cell->SetCellFrequency(cell->GetCellFrequency() + nCellFrequency);

			//			// Affichage de la cellule
			//			if (bDisplayInstanceCreation)
			//				cout << *cell;
			//		}
			//	}

			//	// Nettoyage
			//	oaIndexes.DeleteAll();
			//}
			//// Nouvelle initialisation
			// else
			//{
			boolean bInnerAttributeOk;

			// Parcours des axes de type VarPart
			// Ce parcours ne contient qu'un axe pour le cas de 2 axes instances * variables
			for (nAttribute = nFirstVarPartsAttributeIndex; nAttribute < dataGrid->GetAttributeNumber();
			     nAttribute++)
			{
				// Extraction de l'attribut VarPart
				dgAttribute = dataGrid->GetAttributeAt(nAttribute);

				// Parcours des attributs interne de attribut de grille de type VarPart
				for (nInnerAttribute = 0; nInnerAttribute < dgAttribute->GetInnerAttributeNumber();
				     nInnerAttribute++)
				{
					// Extraction de l'attribut interne dans l'observation courante
					innerAttribute =
					    dgAttribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(
						dgAttribute->GetInnerAttributeNameAt(nInnerAttribute));

					nInnerAttributeIndex =
					    tupleTable->LookupAttributeIndex(innerAttribute->GetAttributeName());
					bInnerAttributeOk = true;
					part = NULL;

					// Recherche de la partie associee a la valeur selon son type
					if (innerAttribute->GetAttributeType() == KWType::Continuous)
					{
						cValue = tuple->GetContinuousAt(nInnerAttributeIndex);
						// Cas d'une valeur manquante
						if (cValue == KWContinuous::GetMissingValue())
						{
							bInnerAttributeOk = false;
							part = innerAttribute->LookupContinuousPart(cValue);
						}

						else
							part = innerAttribute->LookupContinuousPart(cValue);
						if (bDisplayInstanceCreation)
							cout << cValue << "\t";
					}
					else if (innerAttribute->GetAttributeType() == KWType::Symbol)
					{
						sValue = tuple->GetSymbolAt(nInnerAttributeIndex);
						part = innerAttribute->LookupSymbolPart(sValue);
						if (bDisplayInstanceCreation)
							cout << sValue << "\t";
					}

					// Cas d'une observation valide pour l'attribut interne courant
					if (bInnerAttributeOk)
					{
						require(part != NULL);

						// CH Debug erreur sur le nCellFrequency qui est toujours a 1 ?
						// est ce bien ce que l'on veut faire ?
						// Mise a jour de l'effectif de la partie de variable de l'attribut
						// interne
						part->SetPartFrequency(part->GetPartFrequency() + nCellFrequency);

						// Extraction de la VarPartSet de l'axe associee a cette partie
						varPart = dgAttribute->LookupVarPart(part);
						oaParts.SetAt(nAttribute, varPart);

						// Recherche de la cellule
						cell = dataGrid->LookupCell(&oaParts);

						// Creation si necessaire, en tenant compte si demande d'une contrainet
						// sur le nombre max de cellules
						if (cell == NULL)
						{
							// On cree une cellule si l'on ne depasse pas le nombre max
							if (nMaxCellNumberConstraint == 0 or
							    dataGrid->GetCellNumber() < nMaxCellNumberConstraint)
								cell = dataGrid->AddCell(&oaParts);
							// Sinon, on arrete la creation (sauf en mode memoire "risque")
							else
							{
								// Message d'erreur si limite atteinte
								if (dataGrid->GetCellNumber() ==
								    nMaxCellNumberConstraint)
								{
									AddError(
									    sTmp +
									    "Not enough memory to create initial data "
									    "grid, too many data grid cells have been "
									    "created (" +
									    IntToString(nMaxCellNumberConstraint) +
									    ") and " +
									    IntToString((int)ceil(
										(tupleTable->GetSize() - nTuple) *
										100.0 / tupleTable->GetSize())) +
									    "% of the database remains to analyse");
									AddMessage(RMResourceManager::
										       BuildMemoryLimitMessage());
								}

								// Creation en mode risque
								if (RMResourceConstraints::GetIgnoreMemoryLimit())
								{
									// Message d'avertissement la premiere fois
									if (dataGrid->GetCellNumber() ==
									    nMaxCellNumberConstraint)
										RMResourceManager::
										    DisplayIgnoreMemoryLimitMessage();

									// Creation de la cellule
									cell = dataGrid->AddCell(&oaParts);
								}
								// Sinon, on arrete
								else
								{
									bOk = false;
									break;
								}
							}
						}

						// Mise a jour de la cellule directement (pas d'attribut cible)
						cell->SetCellFrequency(cell->GetCellFrequency() + nCellFrequency);

						// Affichage de la cellule
						if (bDisplayInstanceCreation)
							cout << *cell;
					}
				}
			}
			//} // fin de boucle mode bNewInitialisation
		} //  fin de parcours des tuples

		// Fin du mode update
		dataGrid->SetCellUpdateMode(false);
		dataGrid->DeleteIndexingStructure();
		Global::DesactivateErrorFlowControl();
	}
	return bOk;
}
// CH IV End

const CCHierarchicalDataGrid* CCCoclusteringBuilder::GetCoclusteringDataGrid() const
{
	require(IsCoclusteringComputed());
	return coclusteringDataGrid;
}

const KWDataGridCosts* CCCoclusteringBuilder::GetCoclusteringDataGridCosts() const
{
	require(IsCoclusteringComputed());
	return coclusteringDataGridCosts;
}

void CCCoclusteringBuilder::SetReportFileName(const ALString& sFileName)
{
	sAnyTimeReportFileName = sFileName;
}

const ALString& CCCoclusteringBuilder::GetReportFileName() const
{
	return sAnyTimeReportFileName;
}

boolean CCCoclusteringBuilder::GetExportAsKhc() const
{
	return bExportAsKhc;
}

void CCCoclusteringBuilder::SetExportAsKhc(boolean bValue)
{
	bExportAsKhc = bValue;
}

void CCCoclusteringBuilder::RemoveLastSavedReportFile() const
{
	if (sLastActualAnyTimeReportFileName != "")
	{
		PLRemoteFileService::RemoveFile(sLastActualAnyTimeReportFileName);

		// Destruction eventuelle du rapport au format khc
		if (GetExportAsKhc())
			PLRemoteFileService::RemoveFile(FileService::SetFileSuffix(
			    sLastActualAnyTimeReportFileName, CCCoclusteringReport::GetKhcReportSuffix()));
	}
	sLastActualAnyTimeReportFileName = "";
}

void CCCoclusteringBuilder::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						   const KWDataGrid* initialGranularizedDataGrid,
						   boolean bIsLastSaving) const
{
	boolean bKeepIntermediateReports = false;
	boolean bWriteOk;
	boolean bDisplayResults = false;
	const double dEpsilon = 1e-6;
	double dCost;
	double dLevel;
	double dOptimizationTime;
	ALString sReportFileName;
	KWDataGridManager dataGridManager;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringSizeInfo;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	ALString sTmp;
	int nGranularityMax;

	require(optimizedDataGrid != NULL);
	require(nAnyTimeOptimizationIndex >= 0);

	nGranularityMax = (int)ceil(log(initialDataGrid->GetGridFrequency() * 1.0) / log(2.0));

	// Memorisation du cout par defaut la premiere fois

	if (not bIsDefaultCostComputed)
	{
		assert(nAnyTimeOptimizationIndex == 0);
		// CH IV Begin
		// Dans le cas d'un coclustering instances x variables, la structure de cout depend du
		// pre-partitionnement des attributs internes donc son cout par defaut n'est pas egal au cout du modele
		// nul Le cout du modele nul (avec une partie par attribut interne) est calcule au debut de la methode
		// CCCoclusteringBuilder::OptimizeDataGrid
		assert(dAnyTimeDefaultCost == 0 or optimizedDataGrid->GetVarPartDataGrid());
		if (dAnyTimeDefaultCost == 0)
			dAnyTimeDefaultCost = coclusteringDataGridCosts->GetTotalDefaultCost();
		// CH IV End
		dAnyTimeBestCost = dAnyTimeDefaultCost;
		bIsDefaultCostComputed = true;
	}
	assert(dAnyTimeDefaultCost > 0);

	// Cout de la nouvelle solution
	dCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Test si amelioration
	//  ou si la mise a jour est commandee par l'atteinte de la granularite maximale
	// Les grilles avec un seul attribut informatif ne sont pas sauvegardes
	// Cela signifie qu'une grille legerement plus chere avec deux attributs informatifs rencontree au cours
	// de l'optimisation mais non sauvegardee car non optimale du point de vue du cout peut exister mais
	// n'aura pas ete sauvegardee (cf resultats sur AdultSmall1var cout de la grille initiale granularisee)
	if (optimizedDataGrid->GetInformativeAttributeNumber() >= 2 and
	    (dCost < dAnyTimeBestCost - dEpsilon or
	     (bIsLastSaving and ((coclusteringDataGrid == NULL) or (coclusteringDataGrid->GetGranularity() <
								    initialGranularizedDataGrid->GetGranularity())))))
	{
		dAnyTimeBestCost = dCost;

		// Sauvegarde de la grille
		if (coclusteringDataGrid != NULL)
			delete coclusteringDataGrid;
		coclusteringDataGrid = new CCHierarchicalDataGrid;
		dataGridManager.CopyDataGrid(optimizedDataGrid, coclusteringDataGrid);

		// Memorisation de la description courte
		coclusteringDataGrid->SetShortDescription(GetShortDescription());

		// CH IV Begin
		// Memorisation variable identifiant dans le cas d'un coclustering instances * variables
		if (optimizedDataGrid->GetVarPartDataGrid())
			coclusteringDataGrid->SetIdentifierAttributeName(GetIdentifierAttribute());

		// CH AB tentative pour rendre coclusteringDataGrid proprietaire de son innerAttribute qd necessaire
		// si on arrive ici via une grille post-fusionnee, elle a son innerAttribute sinon, non
		if (optimizedDataGrid->GetVarPartDataGrid() and not optimizedDataGrid->GetVarPartsShared())
		{
			optimizedDataGrid->SetVarPartsShared(true);
			coclusteringDataGrid->SetVarPartsShared(false);
		}
		// CH IV End

		// Calcul de ses infos de hierarchie
		// Cas sans granularisation
		if (initialGranularizedDataGrid == NULL)
			ComputeHierarchicalInfo(initialDataGrid, coclusteringDataGridCosts, coclusteringDataGrid);
		// Avec granularisation : la grille granularisee initiale est la reference pour le calcul des infos
		else
			ComputeHierarchicalInfo(initialGranularizedDataGrid, coclusteringDataGridCosts,
						coclusteringDataGrid);

		// Calcul du temps d'optimisation
		tAnyTimeTimer.Stop();
		dOptimizationTime = tAnyTimeTimer.GetElapsedTime(), tAnyTimeTimer.Start();

		// Calcul du Level
		dLevel = 1 - dAnyTimeBestCost / dAnyTimeDefaultCost;

		// Calcul d'un libelle sur la taille de la grille (nombre de parties par dimension)
		// CH IV Begin
		// Cas d'une grille variable * variable
		if (not GetVarPartCoclustering())
		{
			for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
			{
				dgAttribute = coclusteringDataGrid->SearchAttribute(GetAttributeNameAt(nAttribute));
				if (nAttribute > 0)
					sCoclusteringSizeInfo += "*";
				if (dgAttribute == NULL)
					sCoclusteringSizeInfo += "1";
				else
					sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
			}
		}
		// Sinon, cas d'une grille individu * variable
		else
		{
			// Calcul d'un libelle sur la taille de la grille (nombre de parties par dimension)
			for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
			{
				dgAttribute = coclusteringDataGrid->GetAttributeAt(nAttribute);
				if (nAttribute > 0)
					sCoclusteringSizeInfo += "*";
				if (dgAttribute == NULL)
					sCoclusteringSizeInfo += "1";
				else
					sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
			}

			sCoclusteringSizeInfo += "\tVarPartsNumber :";
			for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
			{
				dgAttribute = coclusteringDataGrid->SearchAttribute(GetAttributeNameAt(nAttribute));
				if (dgAttribute->GetOwnerAttributeName() != "")
				{
					// CH AB Si cette partie reste dans l'affichage definitif, il faudra supprimer
					// le premier "*" redondant
					if (nAttribute > 0)
						sCoclusteringSizeInfo += "*";
					if (dgAttribute == NULL)
						sCoclusteringSizeInfo += "1";
					else
						sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
				}
			}

			// Granularite de pre-partitionnement des attributs internes
			sCoclusteringSizeInfo += "\tVarPartsGranularity :";
			sCoclusteringSizeInfo +=
			    IntToString(coclusteringDataGrid->GetInnerAttributes()->GetVarPartGranularity());

			// Granularite de partitionnement de la grille pre-partitionnee
			sCoclusteringSizeInfo += "\tDgGranularity :";
			sCoclusteringSizeInfo += IntToString(coclusteringDataGrid->GetGranularity());
		}
		// CH IV End

		// Sauvegarde dans un fichier temporaire
		nAnyTimeOptimizationIndex++;
		sReportFileName = AnyTimeBuildTemporaryReportFileName(nAnyTimeOptimizationIndex);
		// Cas de non affichage d'info de granularite : pas en mode granularite ou granularite maximale
		if (initialGranularizedDataGrid->GetGranularity() == 0 or
		    (initialGranularizedDataGrid->GetGranularity() == nGranularityMax or
		     initialGranularizedDataGrid->GetLnGridSize() == initialDataGrid->GetLnGridSize()))
			AddSimpleMessage(sTmp + "  " + SecondsToString((int)dOptimizationTime) + "\t" +
					 "Write intermediate coclustering report " +
					 FileService::GetFileName(sReportFileName) +
					 "\tLevel: " + DoubleToString(dLevel) + "\tSize: " + sCoclusteringSizeInfo);
		else
			AddSimpleMessage(
			    sTmp + "  " + SecondsToString((int)dOptimizationTime) + "\t" +
			    "Write intermediate coclustering report " + FileService::GetFileName(sReportFileName) +
			    "\tLevel: " + DoubleToString(dLevel) + "\tSize: " + sCoclusteringSizeInfo +
			    "\tGranularity: " + IntToString(initialGranularizedDataGrid->GetGranularity()));
		// On supprime le mode verbeux pour les sauvegardes intermediaires
		JSONFile::SetVerboseMode(bIsLastSaving);
		bWriteOk = coclusteringReport.WriteJSONReport(sReportFileName, coclusteringDataGrid);
		JSONFile::SetVerboseMode(true);

		// Sauvegarde au format Khc necessaire
		if (bWriteOk and GetExportAsKhc())
		{
			coclusteringReport.WriteReport(
			    FileService::SetFileSuffix(sReportFileName, CCCoclusteringReport::GetKhcReportSuffix()),
			    coclusteringDataGrid);
		}

		// Destruction de la precedente sauvegarde
		if (not bKeepIntermediateReports and bWriteOk)
			RemoveLastSavedReportFile();

		// Memorisation du nouveau nom du dernier fichier sauvegarde
		if (bWriteOk)
			sLastActualAnyTimeReportFileName = sReportFileName;
	}
	else
	{
		if (bDisplayResults)
			cout << "HandleOptimizationStep :: Grille non sauvegardee car n'apporte pas d'amelioration"
			     << endl;
	}
}

const ALString CCCoclusteringBuilder::GetClassLabel() const
{
	return GetLearningModuleName();
}

boolean CCCoclusteringBuilder::CheckMemoryForDatabaseRead(KWDatabase* database) const
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	PLDatabaseTextFile plDatabaseTextFile;
	longint lAvailableMemory;
	longint lSourceFileSize;
	longint lRecordSize;
	int nAttributeNumber;
	longint lEstimatedRecordNumber;
	longint lNecessaryMemory;
	longint lFileMemory;
	longint lInitialDataGridSize;
	longint lWorkingDataGridSize;
	longint lSizeOfCell;
	double dDatabasePercentage;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);

	// Memoire disponible
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Memoire de base pour ouvrir la base
	// On passe par un objet plDatabaseTextFile, qui dispose de fonctionnalite de dimensionnement des ressources
	lNecessaryMemory = 0;
	plDatabaseTextFile.InitializeFrom(GetDatabase());
	bOk = plDatabaseTextFile.ComputeOpenInformation(true, false, NULL);
	if (bOk)
		lNecessaryMemory = plDatabaseTextFile.GetMinOpenNecessaryMemory();

	// On ne fait une estimation fiable de la memoire necessaire que s'il
	// n'y a pas d'attribut de selection (qui ne permet pas d'estimer le nombre d'enregistrements)
	if (bOk and GetDatabase()->GetSelectionAttribute() == "")
	{
		// Elements de dimensionnement a partir des caracteristiques du fichier et du dictionnaire
		lSourceFileSize = FileService::GetFileSize(database->GetDatabaseName());
		nAttributeNumber = GetClass()->GetLoadedAttributeNumber();
		lRecordSize =
		    sizeof(KWObject) + sizeof(KWObject*) + 2 * sizeof(void*) + sizeof(KWValue) * nAttributeNumber;

		// Pourcentage de la base traite
		if (database->GetModeExcludeSample())
			dDatabasePercentage = 100 - database->GetSampleNumberPercentage();
		else
			dDatabasePercentage = database->GetSampleNumberPercentage();

		// Calcul des caracteristiques memoires disponibles et necessaires pour les enregistrements du fichier
		lEstimatedRecordNumber = (longint)(database->GetEstimatedObjectNumber() * dDatabasePercentage / 100);
		lFileMemory = lEstimatedRecordNumber * lRecordSize;
		lFileMemory += (longint)(lSourceFileSize *
					 (GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) /
					  GetClass()->GetNativeDataItemNumber()) *
					 dDatabasePercentage / 100);
		lNecessaryMemory += lFileMemory;

		// Prise en compte d'une grille initiale "minimale" estimee de facon heuristique
		lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
		lInitialDataGridSize = sizeof(KWDataGrid) + nAttributeNumber * sizeof(KWDGAttribute) +
				       (longint)(ceil(sqrt(lEstimatedRecordNumber * 1.0)) * nAttributeNumber *
						 (lSizeOfCell + sizeof(KWDGMPart) + sizeof(KWDGMPartMerge) +
						  sizeof(KWDGInterval) + sizeof(KWDGValueSet) + sizeof(KWDGValue)));
		lNecessaryMemory += lInitialDataGridSize;

		// Plus une grille de travail, et une pour la meilleure solution (de meme taille que la grille initiale)
		lWorkingDataGridSize = 2 * lInitialDataGridSize;
		lNecessaryMemory += lWorkingDataGridSize;

		// Affichage de stats memoire
		if (bDisplayMemoryStats)
		{
			cout << "CheckMemoryForDatabaseRead" << endl;
			cout << "\tEstimated Record number: " << lEstimatedRecordNumber << endl;
			cout << "\tSource file fize: " << LongintToHumanReadableString(lSourceFileSize) << endl;
			cout << "\tFile memory: " << LongintToHumanReadableString(lFileMemory) << endl;
			cout << "\tInitial data grid: " << LongintToHumanReadableString(lInitialDataGridSize) << endl;
			cout << "\tWorking data grid: " << LongintToHumanReadableString(lWorkingDataGridSize) << endl;
			cout << "\t  Necessary: " << LongintToHumanReadableString(lNecessaryMemory) << endl;
			cout << "\t  Available: " << LongintToHumanReadableString(lAvailableMemory) << endl;
			cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
		}
	}

	// Test si memoire suffisante
	if (bOk and lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to load database " +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringBuilder::FillTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable)
{
	boolean bOk;
	longint lAvailableMemory;
	KWTupleTableLoader tupleTableLoader;
	KWTuple* inputTuple;
	StringVector svTupleAttributeNames;
	int nAttribute;
	KWAttribute* attribute;
	KWLoadIndex liFrequencyAttributeLoadIndex;
	KWLoadIndexVector livLoadIndexes;
	KWObject* kwoObject;
	longint lObjectNumber;
	longint lRecordNumber;
	int nObjectFrequency;
	longint lTotalFrequency;
	PeriodicTest periodicTestInterruption;
	ALString sTmp;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);
	require(tupleTable != NULL);
	require(not tupleTable->GetUpdateMode());
	require(tupleTable->GetSize() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read database " + database->GetDatabaseName());

	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Recherche de l'index de l'attribut d'effectif
	if (GetFrequencyAttribute() != "")
		liFrequencyAttributeLoadIndex = GetClass()->LookupAttribute(GetFrequencyAttribute())->GetLoadIndex();

	// Parametrage du chargeur de tuples
	tupleTableLoader.SetInputClass(GetClass());

	// Initialisation des attributs de la table de tuples
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(nAttribute));
		tupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

		// Memorisation du LoadIndex de l'attribut
		livLoadIndexes.Add(attribute->GetLoadIndex());
	}

	// Passage de la table de tuples en mode edition
	tupleTable->SetUpdateMode(true);
	inputTuple = tupleTable->GetInputTuple();

	// Ouverture de la base en lecture
	bOk = database->OpenForRead();

	// Lecture d'objets dans la base
	lTotalFrequency = 0;
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		lObjectNumber = 0;
		while (not database->IsEnd())
		{
			kwoObject = database->Read();
			lRecordNumber++;

			// Acces a l'effectif de l'objet
			nObjectFrequency = 0;
			if (kwoObject != NULL)
			{
				// Acces a l'effectif, avec warning eventuel
				nObjectFrequency =
				    GetDatabaseObjectFrequency(kwoObject, liFrequencyAttributeLoadIndex, lRecordNumber);
				lTotalFrequency += nObjectFrequency;

				// Destruction de l'objet si effectif null
				if (nObjectFrequency <= 0)
				{
					delete kwoObject;
					kwoObject = NULL;
				}
				// Erreur si effectif total trop important
				else if (lTotalFrequency > INT_MAX)
				{
					Object::AddError(sTmp + "Read database interrupted after record " +
							 LongintToString(lRecordNumber) +
							 " because total frequency is too large (" +
							 LongintToReadableString(lTotalFrequency) + ")");
					delete kwoObject;
					kwoObject = NULL;
					bOk = false;
					break;
				}
			}

			// Gestion de l'objet
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Parametrage du tuple d'entree de la table a cree
				for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
				{
					if (tupleTable->GetAttributeTypeAt(nAttribute) == KWType::Symbol)
						inputTuple->SetSymbolAt(
						    nAttribute,
						    kwoObject->GetSymbolValueAt(livLoadIndexes.GetAt(nAttribute)));
					else
						inputTuple->SetContinuousAt(
						    nAttribute,
						    kwoObject->GetContinuousValueAt(livLoadIndexes.GetAt(nAttribute)));
				}

				// Ajout d'un nouveau tuple apres avoir specifie son effectif
				assert(nObjectFrequency <= INT_MAX - tupleTable->GetTotalFrequency());
				inputTuple->SetFrequency(nObjectFrequency);
				tupleTable->UpdateWithInputTuple();

				// Destruction de l'objet
				delete kwoObject;
				kwoObject = NULL;

				// Test regulierement si il y a assez de memoire
				if (tupleTable->GetSize() % 65536 == 0)
				{
					bOk = CheckMemoryForDataGridInitialization(GetDatabase(), tupleTable->GetSize(),
										   nMaxCellNumberConstraint);
					if (not bOk)
						break;
				}
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}
			assert(kwoObject == NULL);

			// Arret si erreur
			if (database->IsError())
			{
				bOk = false;
				Object::AddError("Read database interrupted because of errors");
				break;
			}

			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
			{
				TaskProgression::DisplayProgression((int)(100 * database->GetReadPercentage()));
				database->DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not database->IsError() and TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			Object::AddWarning("Read database interrupted by user");
		}

		// Fermeture
		bOk = database->Close() and bOk;

		// Message global de compte-rendu
		if (bOk)
		{
			if (lRecordNumber == lObjectNumber)
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lObjectNumber));
			else
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lRecordNumber) +
						     "\tSelected records: " + LongintToReadableString(lObjectNumber));
			if (GetFrequencyAttribute() != "")
				GetDatabase()->AddMessage(
				    sTmp + "Total frequency: " + LongintToReadableString(lTotalFrequency));
		}
	}

	// Finalisation en repassant la table de tuples en mode consultation
	tupleTable->SetUpdateMode(false);
	if (not bOk)
		tupleTable->DeleteAll();

	// Erreur si aucun enregistrement lu
	if (bOk and tupleTable->GetSize() == 0)
	{
		AddError("No record read from database");
		bOk = false;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
	ensure(not tupleTable->GetUpdateMode());
	ensure(bOk or tupleTable->GetSize() == 0);
	return bOk;
}
// CH IV Begin
boolean CCCoclusteringBuilder::FillVarPartTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable,
								 ObjectDictionary& odObservationNumbers)
{
	boolean bOk;
	longint lAvailableMemory;
	KWTupleTableLoader tupleTableLoader;
	KWTuple* inputTuple;
	StringVector svTupleAttributeNames;
	int nAttribute;
	KWAttribute* attribute;
	KWLoadIndex liFrequencyAttributeLoadIndex;
	KWLoadIndex liIdentifierAttributeLoadIndex;
	KWLoadIndexVector livLoadIndexes;
	KWObject* kwoObject;
	longint lObjectNumber;
	longint lRecordNumber;
	int nObjectFrequency;
	longint lTotalFrequency;
	PeriodicTest periodicTestInterruption;
	ALString sTmp;
	int nObjectObservationNumber;
	IntObject* ioObservationNumber;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);
	require(tupleTable != NULL);
	require(not tupleTable->GetUpdateMode());
	require(tupleTable->GetSize() == 0);
	require(GetVarPartCoclustering());
	require(odObservationNumbers.GetCount() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read database " + database->GetDatabaseName());

	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Recherche de l'index de l'attribut d'effectif
	if (GetFrequencyAttribute() != "")
		liFrequencyAttributeLoadIndex = GetClass()->LookupAttribute(GetFrequencyAttribute())->GetLoadIndex();

	// Recherche de l'index de l'attribut d'identifiant
	if (GetIdentifierAttribute() != "")
		liIdentifierAttributeLoadIndex = GetClass()->LookupAttribute(GetIdentifierAttribute())->GetLoadIndex();

	// Parametrage du chargeur de tuples
	tupleTableLoader.SetInputClass(GetClass());

	// Initialisation des attributs de la table de tuples
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(nAttribute));
		tupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

		// Memorisation du LoadIndex de l'attribut
		livLoadIndexes.Add(attribute->GetLoadIndex());
	}

	// Passage de la table de tuples en mode edition
	tupleTable->SetUpdateMode(true);
	inputTuple = tupleTable->GetInputTuple();

	// Ouverture de la base en lecture
	bOk = database->OpenForRead();

	// Lecture d'objets dans la base
	lTotalFrequency = 0;
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		lObjectNumber = 0;
		while (not database->IsEnd())
		{
			kwoObject = database->Read();
			lRecordNumber++;

			// Acces a l'effectif de l'objet
			nObjectFrequency = 0;
			if (kwoObject != NULL)
			{
				// Acces a l'effectif, avec warning eventuel
				nObjectFrequency =
				    GetDatabaseObjectFrequency(kwoObject, liFrequencyAttributeLoadIndex, lRecordNumber);
				lTotalFrequency += nObjectFrequency;

				// Destruction de l'objet si effectif null
				if (nObjectFrequency <= 0)
				{
					delete kwoObject;
					kwoObject = NULL;
				}
				// Erreur si effectif total trop important
				else if (lTotalFrequency > INT_MAX)
				{
					Object::AddError(sTmp + "Read database interrupted after record " +
							 LongintToString(lRecordNumber) +
							 " because total frequency is too large (" +
							 LongintToReadableString(lTotalFrequency) + ")");
					delete kwoObject;
					kwoObject = NULL;
					bOk = false;
					break;
				}

				// Calcul du nombre d'observations de l'objet parmi les attributs internes
				// Renvoie 0 si l'attribut identifiant n'est pas renseigne ou si aucune valeur n'est
				// renseignee pour les attributs interne
				nObjectObservationNumber = GetDatabaseObjectObservationNumber(
				    kwoObject, liIdentifierAttributeLoadIndex, lRecordNumber, livLoadIndexes);

				// Destruction de l'objet si effectif null
				if (nObjectObservationNumber <= 0)
				{
					delete kwoObject;
					kwoObject = NULL;
				}
				// Sinon, memorisation du nombre d'observations de l'object
				else
				{
					// Cas d'un attribut Identifiant de type Symbol
					if (GetClass()->LookupAttribute(GetIdentifierAttribute())->GetType() ==
					    KWType::Symbol)
					{
						// Cas ou la valeur de l'identifiant est nouvelle
						if (odObservationNumbers.Lookup(kwoObject->GetSymbolValueAt(
							liIdentifierAttributeLoadIndex)) == NULL)
						{
							ioObservationNumber = new IntObject;
							ioObservationNumber->SetInt(nObjectObservationNumber *
										    nObjectFrequency);
							odObservationNumbers.SetAt(
							    kwoObject->GetSymbolValueAt(liIdentifierAttributeLoadIndex),
							    ioObservationNumber);
						}
						// Sinon : mise a jour du nombre d'observations
						else
						{
							ioObservationNumber = cast(
							    IntObject*,
							    odObservationNumbers.Lookup(kwoObject->GetSymbolValueAt(
								liIdentifierAttributeLoadIndex)));
							ioObservationNumber->SetInt(ioObservationNumber->GetInt() +
										    nObjectObservationNumber *
											nObjectFrequency);
						}
					}
					// Sinon cas d'un attribut Identifiant de type Continuous
					else if (GetClass()->LookupAttribute(GetIdentifierAttribute())->GetType() ==
						 KWType::Continuous)
					{
						if (odObservationNumbers.Lookup(KWContinuous::ContinuousToString(
							kwoObject->GetContinuousValueAt(
							    liIdentifierAttributeLoadIndex))) == NULL)
						{
							ioObservationNumber = new IntObject;
							ioObservationNumber->SetInt(nObjectObservationNumber *
										    nObjectFrequency);
							odObservationNumbers.SetAt(
							    KWContinuous::ContinuousToString(
								kwoObject->GetContinuousValueAt(
								    liIdentifierAttributeLoadIndex)),
							    ioObservationNumber);
						}
						// Sinon : mise a jour du nombre d'observations
						else
						{
							ioObservationNumber = cast(
							    IntObject*, odObservationNumbers.Lookup(
									    KWContinuous::ContinuousToString(
										kwoObject->GetContinuousValueAt(
										    liIdentifierAttributeLoadIndex))));
							ioObservationNumber->SetInt(ioObservationNumber->GetInt() +
										    nObjectObservationNumber *
											nObjectFrequency);
						}
					}
				}
			}

			// Gestion de l'objet
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Parametrage du tuple d'entree de la table a cree
				for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
				{
					if (tupleTable->GetAttributeTypeAt(nAttribute) == KWType::Symbol)
						inputTuple->SetSymbolAt(
						    nAttribute,
						    kwoObject->GetSymbolValueAt(livLoadIndexes.GetAt(nAttribute)));
					else
						inputTuple->SetContinuousAt(
						    nAttribute,
						    kwoObject->GetContinuousValueAt(livLoadIndexes.GetAt(nAttribute)));
				}

				// Ajout d'un nouveau tuple apres avoir specifie son effectif
				assert(nObjectFrequency <= INT_MAX - tupleTable->GetTotalFrequency());
				inputTuple->SetFrequency(nObjectFrequency);
				tupleTable->UpdateWithInputTuple();

				// Destruction de l'objet
				delete kwoObject;
				kwoObject = NULL;

				// Test regulierement si il y a assez de memoire
				if (tupleTable->GetSize() % 65536 == 0)
				{
					bOk = CheckMemoryForDataGridInitialization(GetDatabase(), tupleTable->GetSize(),
										   nMaxCellNumberConstraint);
					if (not bOk)
						break;
				}
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}
			assert(kwoObject == NULL);

			// Arret si erreur
			if (database->IsError())
			{
				bOk = false;
				Object::AddError("Read database interrupted because of errors");
				break;
			}

			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
			{
				TaskProgression::DisplayProgression((int)(100 * database->GetReadPercentage()));
				database->DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not database->IsError() and TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			Object::AddWarning("Read database interrupted by user");
		}

		// Fermeture
		bOk = database->Close() and bOk;

		// Message global de compte-rendu
		if (bOk)
		{
			if (lRecordNumber == lObjectNumber)
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lObjectNumber));
			else
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lRecordNumber) +
						     "\tSelected records: " + LongintToReadableString(lObjectNumber));
			if (GetFrequencyAttribute() != "")
				GetDatabase()->AddMessage(
				    sTmp + "Total frequency: " + LongintToReadableString(lTotalFrequency));
		}
	}

	// Finalisation en repassant la table de tuples en mode consultation
	tupleTable->SetUpdateMode(false);
	if (not bOk)
		tupleTable->DeleteAll();

	// Erreur si aucun enregistrement lu
	if (bOk and tupleTable->GetSize() == 0)
	{
		AddError("No record read from database");
		bOk = false;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
	ensure(not tupleTable->GetUpdateMode());
	ensure(bOk or tupleTable->GetSize() == 0);
	return bOk;
}

void CCCoclusteringBuilder::FillObjectArrayVarPartsAttributesIndexes(ObjectArray* oaIndexes, int nIndex,
								     KWDataGrid* dataGrid)
{
	IntVector ivCurrentIndexes;
	IntVector ivVarPartInnerAttributesNumbers;
	int nAttributeIndex;
	int nArraySize;
	int nArrayIndex;
	int nIncrementedIndex;
	boolean bIncremented;

	require(oaIndexes != NULL);

	// Initialisation
	nArraySize = 1;
	oaIndexes->DeleteAll();
	oaIndexes->SetSize(nArraySize);

	// Parcours des attributs de type VarPart
	for (nAttributeIndex = nIndex; nAttributeIndex < dataGrid->GetAttributeNumber(); nAttributeIndex++)
	{
		// Extraction du nombre d'attributs internes pour l'attribut VarPart courant
		ivVarPartInnerAttributesNumbers.Add(ivInnerAttributesNumber.GetAt(nAttributeIndex));
		nArraySize *= ivInnerAttributesNumber.GetAt(nAttributeIndex);
	}

	// Taille du nombre d'attributs de type VarPart
	ivCurrentIndexes.SetSize(ivVarPartInnerAttributesNumbers.GetSize());

	// Ajout du premier vecteur d'index
	oaIndexes->SetAt(0, ivCurrentIndexes.Clone());

	// Parcours des combinaisons possibles
	for (nArrayIndex = 1; nArrayIndex < nArraySize; nArrayIndex++)
	{
		bIncremented = false;
		nIncrementedIndex = ivCurrentIndexes.GetSize() - 1;
		while (not bIncremented)
		{
			if (ivCurrentIndexes.GetAt(nIncrementedIndex) <
			    ivVarPartInnerAttributesNumbers.GetAt(nIncrementedIndex) - 1)
			{
				ivCurrentIndexes.SetAt(nIncrementedIndex,
						       ivCurrentIndexes.GetAt(nIncrementedIndex) + 1);
				for (nAttributeIndex = nIncrementedIndex + 1;
				     nAttributeIndex < ivCurrentIndexes.GetSize(); nAttributeIndex++)
				{
					ivCurrentIndexes.SetAt(nAttributeIndex, 0);
				}
				oaIndexes->SetAt(nArrayIndex, ivCurrentIndexes.Clone());
				bIncremented = true;
			}
			else
				nIncrementedIndex--;
		}
	}
}

boolean CCCoclusteringBuilder::CreateIdentifierAttributeIntervals(const KWTupleTable* tupleTable,
								  KWDGAttribute* dgAttribute,
								  ObjectDictionary& odObservationNumbers)
{
	int nTuple;
	const KWTuple* tuple;
	KWTupleTable attributeTupleTable;
	Continuous cSourceValue;
	Continuous cSourceRef;
	KWDGPart* part;
	Continuous cIntervalBound;
	double dProgression;
	int nMaxValueNumber;
	int nMinValueNumber = 500;
	int nFrequency;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()) != -1);
	require(tupleTable->GetAttributeTypeAt(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName())) ==
		KWType::Continuous);
	require(dgAttribute != NULL);
	require(dgAttribute->GetPartNumber() == 0);

	// Construction d'une table de tuples univariee dediee a l'attribut
	tupleTable->BuildUnivariateTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	if (TaskProgression::IsInterruptionRequested())
		return false;

	// CH AB pas sur car pas de double dans odObservationNumber <= plutot ?
	assert(odObservationNumbers.GetCount() == attributeTupleTable.GetSize());

	// Pre-granularisation des attributs numeriques cible (regression) et des attributs numeriques explicatifs en
	// analyse non supervisee (co-clustering) Cette pre-granularisation permet :
	// - en regression, de limiter le nombre de valeurs cible au seuil sqrt(N log N), superieur a l'ecart type
	// theorique de la precision de prediction en sqrt(N)
	// - en coclustering

	// Calcul du nombre maximal de valeurs tolere en fonction du nombre d'instances
	if (attributeTupleTable.GetTotalFrequency() < nMinValueNumber)
		nMaxValueNumber = attributeTupleTable.GetTotalFrequency();
	else
		nMaxValueNumber =
		    nMinValueNumber +
		    (int)ceil(sqrt((attributeTupleTable.GetTotalFrequency() - nMinValueNumber) *
				   log((attributeTupleTable.GetTotalFrequency() - nMinValueNumber)) / log(2.0)));

	// Cas de la pre-granularisation de l'attribut
	if (GetPregranularizedNumericalAttributes() and attributeTupleTable.GetSize() > nMaxValueNumber and
	    (dgAttribute->GetAttributeTargetFunction() or GetTargetAttributeName() == ""))
		CreateAttributePreGranularizedIntervals(&attributeTupleTable, dgAttribute, nMaxValueNumber);
	// Sinon
	else
	{
		// Creation d'une premiere partie, avec sa borne inf (contenant la valeur manquante)
		part = dgAttribute->AddPart();
		part->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
		part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

		// Creation des parties de l'attribut pour chaque tuple
		cSourceRef = KWDGInterval::GetMinLowerBound();

		// Effectif de l'intervalle courant
		nFrequency = 0;

		for (nTuple = 0; nTuple < attributeTupleTable.GetSize(); nTuple++)
		{
			tuple = attributeTupleTable.GetAt(nTuple);

			// Progression
			if (periodicTestDisplay.IsTestAllowed(nTuple))
			{
				// Cas d'un attribut de grille standard, non interne dans un attribut VarPart
				if (dgAttribute->GetOwnerAttributeName() == "")
				{
					// Avancement: au prorata de la base pour l'attribut en cours, en reservant 50
					// pour la creation des cellules
					dProgression = dgAttribute->GetAttributeIndex() * 50.0 /
						       dgAttribute->GetDataGrid()->GetAttributeNumber();
					dProgression += (nTuple * 50.0 / attributeTupleTable.GetSize()) /
							dgAttribute->GetDataGrid()->GetAttributeNumber();
					TaskProgression::DisplayProgression((int)dProgression);
					if (TaskProgression::IsInterruptionRequested())
						return false;
				}
			}

			// Valeur du tuple
			cSourceValue = tuple->GetContinuousAt(0);

			// Memorisation de la valeur de reference initiale pour le premier tuple ou premier tuple avec
			// valeur non manquante

			if (nTuple == 0 or (cSourceRef == KWContinuous::GetMissingValue() and
					    dgAttribute->GetOwnerAttributeName() != ""))
			{
				cSourceRef = cSourceValue;
				nFrequency = tuple->GetFrequency();
			}
			// Cas du dernier tuple d'effectif 1 : il est regroupe avec l'intervalle precedent
			else if (dgAttribute->GetOwnerAttributeName() != "" and
				 nTuple == attributeTupleTable.GetSize() - 1 and tuple->GetFrequency() == 1)
				break;
			// Creation d'un nouvel intervalle sinon
			else
			{
				assert(cSourceValue > cSourceRef);

				if (dgAttribute->GetOwnerAttributeName() == "" or nFrequency > 1)
				{
					// Calcul de la borne sup de l'intervalle courant, comme moyenne de la valeur
					// des deux objets de part et d'autre de l'intervalle
					cIntervalBound =
					    KWContinuous::GetHumanReadableLowerMeanValue(cSourceRef, cSourceValue);

					// Memorisation de la borne sup de l'intervalle en court
					part->GetInterval()->SetUpperBound(cIntervalBound);

					// Creation d'une nouvelle partie avec sa borne inf
					part = dgAttribute->AddPart();
					part->GetInterval()->SetLowerBound(cIntervalBound);
					part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

					// Nouvelle valeur de reference
					cSourceRef = cSourceValue;
					// Reinitialisation de l'effectif de l'intervalle courant
					nFrequency = tuple->GetFrequency();
				}
				else
				{
					// Nouvelle valeur de reference
					cSourceRef = cSourceValue;
					// Mise a jour de l'effectif de l'intervalle courant
					nFrequency += tuple->GetFrequency();
				}
			}
		}
		// Parametrage du nombre total de valeurs (= nombre d'instances)
		dgAttribute->SetInitialValueNumber(attributeTupleTable.GetTotalFrequency());
		dgAttribute->SetGranularizedValueNumber(attributeTupleTable.GetTotalFrequency());
		assert(dgAttribute->GetPartNumber() == attributeTupleTable.GetSize() or
		       GetPregranularizedNumericalAttributes());
		assert(dgAttribute->GetInitialValueNumber() + 1 >= dgAttribute->GetPartNumber());
		ensure(dgAttribute->Check());
	}

	return true;
}

boolean CCCoclusteringBuilder::CreateIdentifierAttributeValueSets(const KWTupleTable* tupleTable,
								  KWDGAttribute* dgAttribute,
								  ObjectDictionary& odObservationNumbers)
{
	int nTuple;
	const KWTuple* tuple;
	KWTupleTable attributeTupleTable;
	KWDGPart* part;
	KWDGValue* value;
	double dProgression;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()) != -1);
	require(tupleTable->GetAttributeTypeAt(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName())) ==
		KWType::Symbol);
	require(dgAttribute != NULL);
	require(dgAttribute->GetPartNumber() == 0);

	// Construction d'une table de tuples univariee dediee a l'attribut
	tupleTable->BuildUnivariateTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	// tupleTable->BuildIdentifierTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	if (TaskProgression::IsInterruptionRequested())
		return false;

	// CH AB pas sur car pas de double dans odObservationNumber <= plutot ?
	assert(odObservationNumbers.GetCount() == attributeTupleTable.GetSize());

	// Tri des tuple par effectif decroissant, puis valeurs croissantes
	attributeTupleTable.SortByDecreasingFrequencies();

	// Creation des parties mono-valeurs de l'attribut pour chaque tuple
	// La modalite speciale est affectee a la valeur la moins frequente arrivee en premier
	for (nTuple = 0; nTuple < attributeTupleTable.GetSize(); nTuple++)
	{
		tuple = attributeTupleTable.GetAt(nTuple);

		// Verifications de coherence
		assert(nTuple == 0 or attributeTupleTable.GetAt(nTuple - 1)->GetFrequency() > tuple->GetFrequency() or
		       attributeTupleTable.GetAt(nTuple - 1)->GetSymbolAt(0).CompareValue(tuple->GetSymbolAt(0)) < 0);

		// Progression
		if (periodicTestDisplay.IsTestAllowed(nTuple))
		{
			// Cas d'un attribut de grille standard, non interne dans un attribut VarPart
			// Sinon GetDataGrid() n'est pas defini
			if (dgAttribute->GetOwnerAttributeName() == "")
			{
				// Avancement: au prorata de la base pour l'attribut en cours, en reservant 50 pour la
				// creation des cellules
				dProgression = dgAttribute->GetAttributeIndex() * 50.0 /
					       dgAttribute->GetDataGrid()->GetAttributeNumber();
				dProgression += (nTuple * 50.0 / attributeTupleTable.GetSize()) /
						dgAttribute->GetDataGrid()->GetAttributeNumber();
				TaskProgression::DisplayProgression((int)dProgression);
				if (TaskProgression::IsInterruptionRequested())
					return false;
			}
		}

		// Creation d'une nouvelle partie mono-valeur
		part = dgAttribute->AddPart();
		value = part->GetValueSet()->AddValue(tuple->GetSymbolAt(0));
		value->SetValueFrequency(
		    cast(IntObject*, odObservationNumbers.Lookup(tuple->GetSymbolAt(0)))->GetInt());
	}
	assert(attributeTupleTable.GetSize() == dgAttribute->GetPartNumber());
	assert(dgAttribute->GetPartNumber() > 0);

	// Initialisation de la partie par defaut, contenant la modalite speciale
	// Compte-tenu du tri prealable des tuples, il s'agit de la derniere partie de l'attribut
	dgAttribute->GetTailPart()->GetValueSet()->AddValue(Symbol::GetStarValue());

	// Parametrage du nombre total de valeurs
	// Pour un attribut categoriel, l'InitialValueNumber ne contient plus la StarValue
	dgAttribute->SetInitialValueNumber(dgAttribute->GetPartNumber());
	// On ne prend pas en compte la StarValue dans Vg
	dgAttribute->SetGranularizedValueNumber(dgAttribute->GetPartNumber());
	ensure(dgAttribute->Check());
	return true;
}

int CCCoclusteringBuilder::GetDatabaseObjectObservationNumber(KWObject* kwoObject,
							      KWLoadIndex liIdentifierAttributeLoadIndex,
							      longint lRecordIndex, KWLoadIndexVector& livLoadIndexes)
{
	int nObjectObservationNumber;
	ALString sTmp;
	int nAttribute;
	KWAttribute* attribute;

	require(kwoObject != NULL);
	require(lRecordIndex >= 1);
	require(livLoadIndexes.GetSize() == GetAttributeNumber());

	nObjectObservationNumber = 0;

	if (liIdentifierAttributeLoadIndex.IsValid())
	{
		// Recherche de la validite du champ identifiant
		if ((GetClass()->LookupAttribute(GetIdentifierAttribute())->GetType() == KWType::Symbol and
		     kwoObject->GetSymbolValueAt(liIdentifierAttributeLoadIndex).IsEmpty()) or
		    (GetClass()->LookupAttribute(GetIdentifierAttribute())->GetType() == KWType::Continuous and
		     kwoObject->GetContinuousValueAt(liIdentifierAttributeLoadIndex) ==
			 KWContinuous::GetMissingValue()))
		{
			GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
						  ", identifier variable (" + GetIdentifierAttribute() +
						  ") with missing value");
			return nObjectObservationNumber;
		}
		// Parcours des attributs internes pour calculer le nombre d'observations
		// Parametrage du tuple d'entree de la table a cree
		for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
		{
			attribute = GetClass()->LookupAttribute(GetAttributeNameAt(nAttribute));

			// Cas d'un attribut interne dans une variable de type VarPart (attribut identifiant exclu)
			if (livLoadIndexes.GetAt(nAttribute) != liIdentifierAttributeLoadIndex)
			{
				if (attribute->GetType() == KWType::Continuous and
				    kwoObject->GetContinuousValueAt(livLoadIndexes.GetAt(nAttribute)) !=
					KWContinuous::GetMissingValue())
					nObjectObservationNumber++;
				else if (attribute->GetType() == KWType::Symbol)
					nObjectObservationNumber++;
			}
		}
	}
	return nObjectObservationNumber;
}
// CH IV End

int CCCoclusteringBuilder::GetDatabaseObjectFrequency(KWObject* kwoObject, KWLoadIndex liFrequencyAttributeLoadIndex,
						      longint lRecordIndex)
{
	int nObjectFrequency;
	Continuous cObjectFrequency;
	ALString sTmp;

	require(kwoObject != NULL);
	require(lRecordIndex >= 1);

	// Recherche de l'effectif de la cellule, en fonction de l'eventuelle variable d'effectif
	nObjectFrequency = 1;
	if (liFrequencyAttributeLoadIndex.IsValid())
	{
		// Recherche de l'effectif
		cObjectFrequency = kwoObject->GetContinuousValueAt(liFrequencyAttributeLoadIndex);
		nObjectFrequency = (int)floor(cObjectFrequency + 0.5);
		if (nObjectFrequency < 0)
			nObjectFrequency = 0;

		// Enregistrement ignore si effectif trop grand
		if (cObjectFrequency > INT_MAX)
		{
			GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
						  ", frequency variable (" + GetFrequencyAttribute() +
						  ") with value too large (" +
						  KWContinuous::ContinuousToString(cObjectFrequency) + ")");

			// On met l'effectif a 0 pour ignorer l'enregistrement
			nObjectFrequency = 0;
		}
		// Enregistrement ignore si effectif negatif ou nul
		else if (cObjectFrequency <= 0)
		{
			GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
						  ", frequency variable (" + GetFrequencyAttribute() +
						  ") with non positive value (" +
						  KWContinuous::ContinuousToString(cObjectFrequency) + ")");
		}
		// Warning si erreur d'arrondi
		else if (fabs(cObjectFrequency - nObjectFrequency) > 0.05)
		{
			if (nObjectFrequency > 0)
			{
				GetDatabase()->AddWarning(sTmp + "Record " + LongintToString(lRecordIndex) +
							  ", frequency variable (" + GetFrequencyAttribute() +
							  ") with non integer value (" +
							  KWContinuous::ContinuousToString(cObjectFrequency) + " -> " +
							  IntToString(nObjectFrequency) + ")");
			}
			else
			{
				GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
							  ", frequency variable (" + GetFrequencyAttribute() +
							  ") with null rounded value (" +
							  KWContinuous::ContinuousToString(cObjectFrequency) + ")");
			}
		}
	}
	return nObjectFrequency;
}

boolean CCCoclusteringBuilder::CheckMemoryForDataGridInitialization(KWDatabase* database, int nTupleNumber,
								    int& nMaxCellNumber) const
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	int nAttributeNumber;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	longint lInitialDataGridSize;
	longint lWorkingDataGridSize;
	longint lSizeOfCell;
	double dMaxCellNumber;
	int nValueNumber;
	int nAttribute;
	KWAttribute* attribute;
	KWDescriptiveStats* descriptiveStats;
	ALString sMessage;
	ALString sTmp;

	require(database != NULL);
	require(database->IsOpenedForRead() or odDescriptiveStats.GetCount() > 0);
	require(nTupleNumber > 0);
	require(odDescriptiveStats.GetCount() == 0 or
		odDescriptiveStats.GetCount() ==
		    GetClass()->GetLoadedAttributeNumber() - (GetFrequencyAttribute() == "" ? 0 : 1));

	// Calcul des caracteristiques memoire disponibles (le fichier est lu a ce moment)
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Stats de bases sur les variables et instances
	nAttributeNumber = GetClass()->GetLoadedAttributeNumber();

	// Prise en compte d'une grille initiale "minimale" estimee de facon fine en prenant en compte les nombres de
	// valeurs par variable
	lNecessaryMemory = 0;
	lInitialDataGridSize = sizeof(KWDataGrid) + sizeof(void*);
	dMaxCellNumber = 1;
	for (nAttribute = 0; nAttribute < GetClass()->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->GetLoadedAttributeAt(nAttribute);

		// Recherche des stats descriptives de l'attribut, sauf si attribut d'effectif
		if (attribute->GetName() != GetFrequencyAttribute())
		{
			// Nombre de valeur de l'attribut si ses statistiques descriptives sont disponible
			descriptiveStats = cast(KWDescriptiveStats*, odDescriptiveStats.Lookup(attribute->GetName()));
			if (descriptiveStats != NULL)
				nValueNumber = descriptiveStats->GetValueNumber();
			// Estimation sinon
			else
				nValueNumber = (int)sqrt(nTupleNumber);

			// Prise en compte de la taille de stockage de l'attribut, de ses parties et valeurs
			dMaxCellNumber *= nValueNumber;
			lInitialDataGridSize += sizeof(KWDGAttribute) + sizeof(void*);
			if (attribute->GetType() == KWType::Continuous)
				lInitialDataGridSize +=
				    nValueNumber * (sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*));
			else if (attribute->GetType() == KWType::Symbol)
				lInitialDataGridSize += nValueNumber * (sizeof(KWDGMPart) + sizeof(KWDGValueSet) +
									sizeof(KWDGValue) + 2 * sizeof(void*));
		}
	}
	if (dMaxCellNumber > nTupleNumber)
		dMaxCellNumber = nTupleNumber;
	lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
	lInitialDataGridSize += (int)ceil(sqrt(dMaxCellNumber)) * nAttributeNumber * lSizeOfCell;
	lNecessaryMemory += lInitialDataGridSize;

	// Plus une grille de travail, et une pour la meilleure solution (de taille estimee minimale)
	lWorkingDataGridSize = sizeof(KWDataGridMerger) + nAttributeNumber * sizeof(KWDGMAttribute) +
			       (int)ceil(sqrt(dMaxCellNumber)) * nAttributeNumber *
				   (lSizeOfCell + sizeof(KWDGMPart) + sizeof(KWDGMPartMerge) + sizeof(KWDGInterval) +
				    sizeof(KWDGValueSet) + sizeof(KWDGValue));
	lWorkingDataGridSize *= 2;
	lNecessaryMemory += lWorkingDataGridSize;

	// Estimation du nombre max de cellules que l'on peut charger en memoire, pour les grilles initiales, de
	// travail, et finales
	nMaxCellNumber = 0;
	if (lNecessaryMemory < lAvailableMemory)
		nMaxCellNumber = (int)((lAvailableMemory - lNecessaryMemory) / (3 * lSizeOfCell));

	// Affichage de stats memoire
	if (bDisplayMemoryStats)
	{
		cout << "CheckMemoryForDataGridInitialization" << endl;
		cout << "\tInitial data grid: " << lInitialDataGridSize << endl;
		cout << "\tWorking data grid: " << lWorkingDataGridSize << endl;
		cout << "\tMax cell number: " << nMaxCellNumber << endl;
		cout << "\t  Necessary: " << lNecessaryMemory << endl;
		cout << "\t  Available: " << lAvailableMemory << endl;
		cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		sMessage = "Not enough memory to create initial data grid ";
		if (database->IsOpenedForRead() and database->GetReadPercentage() < 0.99)
			sMessage += sTmp + "after reading " + IntToString((int)(100 * database->GetReadPercentage())) +
				    "% of the database";
		else
			sMessage += RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory);
		AddError(sMessage);
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringBuilder::CheckMemoryForDataGridOptimization(KWDataGrid* inputInitialDataGrid) const
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	longint lDataGridPostOptimizationSize;
	longint lWorkingDataGridSize;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	longint lTotalValueNumber;
	longint lInitialMaxPartNumber;
	longint lMaxPartNumber;
	longint lPartNumber;
	longint lTotalPartMergeNumber;
	longint lSizeOfCell;

	require(inputInitialDataGrid != NULL);
	require(inputInitialDataGrid->GetAttributeNumber() > 0);

	// Calcul des caracteristiques memoires disponibles, la grille initiale etant deja chargee en memoire
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Calcul de stats sur la grille initiale
	lTotalValueNumber = 0;
	lInitialMaxPartNumber = 1;
	for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = inputInitialDataGrid->GetAttributeAt(nAttribute);

		// Nombre de partie maximale
		lPartNumber = dgAttribute->GetPartNumber();
		if (lPartNumber > lInitialMaxPartNumber)
			lInitialMaxPartNumber = lPartNumber;

		// Nombre total de valeurs
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
			lTotalValueNumber += dgAttribute->GetStoredValueNumber();
	}

	// Prise en compte d'une grille de travail et d'une grille pour la meilleure solution
	lNecessaryMemory = 0;
	lMaxPartNumber = 1 + (int)ceil(pow(inputInitialDataGrid->GetGridFrequency(),
					   1.0 / inputInitialDataGrid->GetAttributeNumber()));
	lWorkingDataGridSize = sizeof(KWDataGridMerger) + sizeof(void*);
	lWorkingDataGridSize += sizeof(KWDataGrid) + sizeof(void*);
	lTotalPartMergeNumber = 0;
	for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = inputInitialDataGrid->GetAttributeAt(nAttribute);

		// Prise en compte de la taille de stockage de l'attribut, de ses parties et valeurs
		lWorkingDataGridSize += 2 * (sizeof(KWDGMAttribute) + sizeof(void*));
		lPartNumber = dgAttribute->GetPartNumber();
		if (lPartNumber > lMaxPartNumber)
			lPartNumber = lMaxPartNumber;
		lTotalPartMergeNumber += (lPartNumber * (lPartNumber - 1)) / 2;
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
			lWorkingDataGridSize +=
			    2 * lPartNumber * (sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*));
		else if (dgAttribute->GetAttributeType() == KWType::Symbol)
			lWorkingDataGridSize += 2 * dgAttribute->GetPartNumber() *
						(sizeof(KWDGMPart) + sizeof(KWDGValueSet) + 2 * sizeof(void*));
		// CH IV Begin
		else if (dgAttribute->GetAttributeType() == KWType::VarPart)
		{
			int nInnerAttribute;
			KWDGAttribute* innerAttribute;
			int nInnerAttributePartNumber;
			for (nInnerAttribute = 0;
			     nInnerAttribute < inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute =
				    inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(nInnerAttribute);
				nInnerAttributePartNumber = innerAttribute->GetPartNumber();
				if (innerAttribute->GetAttributeType() == KWType::Continuous)
					lWorkingDataGridSize +=
					    2 * nInnerAttributePartNumber *
					    (sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*));
				else if (innerAttribute->GetAttributeType() == KWType::Symbol)
					lWorkingDataGridSize +=
					    2 * nInnerAttributePartNumber *
					    (sizeof(KWDGMPart) + sizeof(KWDGValueSet) + 2 * sizeof(void*));
			}
		}
		// CH IV End
	}
	lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
	lWorkingDataGridSize += inputInitialDataGrid->GetCellNumber() * lSizeOfCell +
				lTotalValueNumber * (sizeof(KWDGValue) + sizeof(void*));
	lWorkingDataGridSize += inputInitialDataGrid->GetCellNumber() * lSizeOfCell +
				lTotalValueNumber * (sizeof(KWDGValue) + sizeof(void*));
	lWorkingDataGridSize += lTotalPartMergeNumber * (sizeof(KWDGMPartMerge) + 2 * sizeof(void*));
	lNecessaryMemory += lWorkingDataGridSize;

	// Prise en compte de la memoire de travail pour post-optimisation
	lDataGridPostOptimizationSize =
	    inputInitialDataGrid->GetCellNumber() *
	    (sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*));
	lDataGridPostOptimizationSize += inputInitialDataGrid->GetCellNumber() * sizeof(KWDGPOCellFrequencyVector);
	lDataGridPostOptimizationSize += lInitialMaxPartNumber * (sizeof(KWMODLLineDeepOptimization) + 2 * sizeof(int) +
								  sizeof(KWDGPOPartFrequencyVector));
	lNecessaryMemory += lDataGridPostOptimizationSize;

	// Affichage de stats memoire
	if (bDisplayMemoryStats)
	{
		cout << "CheckMemoryForDataGridOptimization" << endl;
		cout << "\tWorking data grid: " << lWorkingDataGridSize << endl;
		cout << "\tData grid post-optimization: " << lDataGridPostOptimizationSize << endl;
		cout << "\t  Necessary: " << lNecessaryMemory << endl;
		cout << "\t  Available: " << lAvailableMemory << endl;
		cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to optimize data grid " +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}

	return bOk;
}

void CCCoclusteringBuilder::AnyTimeStart() const
{
	// Initialisations
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Reset();
	tAnyTimeTimer.Start();
	sLastActualAnyTimeReportFileName = "";
	bIsDefaultCostComputed = false;
}

void CCCoclusteringBuilder::AnyTimeStop() const
{
	ALString sReportFileName;

	// Initialisations (sauf nom du dernier fichier temporaire, potentiellement a detruire)
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Stop();
	bIsDefaultCostComputed = false;
}

const ALString CCCoclusteringBuilder::AnyTimeBuildTemporaryReportFileName(int nIndex) const
{
	ALString sTemporaryReportFileName;
	ALString sPathName;
	ALString sFilePrefix;
	ALString sFileSuffix;

	require(GetReportFileName() != "");
	require(nIndex >= 1);

	// Extraction des partie du nom du fichier
	sPathName = FileService::GetPathName(GetReportFileName());
	sFilePrefix = FileService::GetFilePrefix(GetReportFileName());
	sFileSuffix = FileService::GetFileSuffix(GetReportFileName());

	// Construction d'un nom de fichier en le suffixant par l'index
	sTemporaryReportFileName = FileService::BuildFilePathName(
	    sPathName, FileService::BuildFileName(sFilePrefix + "(" + IntToString(nIndex) + ")", sFileSuffix));
	return sTemporaryReportFileName;
}

void CCCoclusteringBuilder::CleanCoclusteringResults()
{
	// Nettoyage de la grille et de sa structure de cout
	if (coclusteringDataGrid != NULL)
		delete coclusteringDataGrid;
	if (initialDataGrid != NULL)
		delete initialDataGrid;
	if (coclusteringDataGridCosts != NULL)
		delete coclusteringDataGridCosts;
	coclusteringDataGrid = NULL;
	initialDataGrid = NULL;
	coclusteringDataGridCosts = NULL;
	odDescriptiveStats.DeleteAll();
}

void CCCoclusteringBuilder::ComputeDescriptiveAttributeStats(const KWTupleTable* tupleTable,
							     ObjectDictionary* odOutputDescriptiveStats) const
{
	int nAttribute;
	KWAttribute* attribute;
	KWDescriptiveStats* descriptiveStats;
	KWTupleTable univariateTupleTable;

	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(odOutputDescriptiveStats != NULL);
	require(odOutputDescriptiveStats->GetCount() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Compute univariate descriptive stats");

	// Calcul des stats descriptives par attribut
	for (nAttribute = 0; nAttribute < GetClass()->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->GetLoadedAttributeAt(nAttribute);

		// Suivi de tache
		TaskProgression::DisplayProgression((nAttribute + 1) * 100 / GetClass()->GetLoadedAttributeNumber());
		TaskProgression::DisplayLabel(attribute->GetName());

		// Cas des attributs simples, hors attribut d'effectif
		if (attribute->GetName() != GetFrequencyAttribute() and KWType::IsSimple(attribute->GetType()) and
		    not TaskProgression::IsInterruptionRequested())
		{
			// Creation d'un objet de stats pour l'attribut selon son type
			if (attribute->GetType() == KWType::Continuous)
				descriptiveStats = new KWDescriptiveContinuousStats;
			else
				descriptiveStats = new KWDescriptiveSymbolStats;

			// Initialisation
			descriptiveStats->SetLearningSpec(GetLearningSpec());
			descriptiveStats->SetAttributeName(attribute->GetName());

			// Creation d'une table de tuples univariee a partir de la table de tuples globale
			tupleTable->BuildUnivariateTupleTable(attribute->GetName(), &univariateTupleTable);

			// Calcul des stats
			descriptiveStats->ComputeStats(&univariateTupleTable);

			// Memorisation
			odOutputDescriptiveStats->SetAt(descriptiveStats->GetAttributeName(), descriptiveStats);
		}
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
}

void CCCoclusteringBuilder::ComputeHierarchicalInfo(const KWDataGrid* inputInitialDataGrid,
						    const KWDataGridCosts* dataGridCosts,
						    CCHierarchicalDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	double dBestDataGridTotalCost;

	require(optimizedDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid != NULL);

	// Memorisation des bornes des attributs Continuous
	ComputeContinuousAttributeBounds(optimizedDataGrid);

	// Calcul de la typicalite des attributs
	ComputeAttributeTypicalities(optimizedDataGrid);

	// Calcul de la typicalite des valeurs des attribut categoriels
	ComputeValueTypicalities(inputInitialDataGrid, dataGridCosts, optimizedDataGrid);

	////////////////////////////////////////////////////////////////////////////
	// Calcul de caracteristiques des parties du coclustering
	// en evaluant l'impact des fusions de groupes

	// Creation d'un KWDataGridMerger pour l'evaluation des fusions entre groupes du coclustering
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportDataGrid(&dataGridMerger);

	// Initialisation de la structure de couts
	dataGridMerger.SetDataGridCosts(dataGridCosts);

	// Initialisation des couts des entites du DataGridMerger
	dataGridMerger.InitializeAllCosts();
	dBestDataGridTotalCost = dataGridMerger.GetDataGridCosts()->ComputeDataGridMergerTotalCost(&dataGridMerger);

	// Memorisation des couts
	optimizedDataGrid->SetCost(dBestDataGridTotalCost);
	optimizedDataGrid->SetNullCost(dataGridMerger.GetDataGridCosts()->GetTotalDefaultCost());
	// CH IV Begin
	// Dans le cas d'un coclustering instances x variables, la structure de cout depend du pre-partitionnement des
	// attributs internes donc son cout par defaut n'est pas egal au cout du modele nul Le cout du modele nul (avec
	// une partie par attribut interne) est calcule au debut de la methode CCCoclusteringBuilder::OptimizeDataGrid
	if (optimizedDataGrid->GetVarPartDataGrid())
	{
		optimizedDataGrid->SetNullCost(dAnyTimeDefaultCost);
	}
	// CH IV End

	// Memorisation du contexte d'apprentissage
	optimizedDataGrid->SetInitialAttributeNumber(inputInitialDataGrid->GetAttributeNumber());
	optimizedDataGrid->SetFrequencyAttributeName(GetFrequencyAttribute());
	optimizedDataGrid->GetDatabaseSpec()->CopyFrom(GetDatabase());

	// Initialisation de la table de hash des cellules
	dataGridMerger.CellDictionaryInit();

	// Initialisation de toutes les fusions
	dataGridMerger.InitializeAllPartMerges();
	assert(dataGridMerger.CheckAllPartMerges());

	// Initialisation de la liste des parties triees par nombre de modalites
	dataGridMerger.InitializeAllPartLists();

	// Calcul de l'interet des parties
	ComputePartInterests(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Calcul des hierarchies des parties, en creant de nouvelles parties pour les
	// coder les hierarchies
	ComputePartHierarchies(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Calcul des rangs des parties
	ComputePartRanks(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Tri des valeurs par typicalite decorissante pour les attributs categoriels
	SortAttributePartsAndValues(optimizedDataGrid);
}

void CCCoclusteringBuilder::ComputeAttributeTypicalities(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	CCHDGAttribute* hdgAttribute;

	require(optimizedDataGrid != NULL);

	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Memorisation du nombre de parties initiales, eagl ici au nombre de parties
		hdgAttribute->SetInitialPartNumber(hdgAttribute->GetPartNumber());

		// Pour l'instant, on met toutes les typicalites a 1
		hdgAttribute->SetInterest(1);
	}
}

void CCCoclusteringBuilder::ComputeContinuousAttributeBounds(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	ContinuousVector* cvMinValues;
	ContinuousVector* cvMaxValues;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	ObjectDictionary* odInnerContinuousAttributeIndexes;
	IntObject* io;

	require(optimizedDataGrid != NULL);

	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Recherche des caracteristiques de l'attribut si numerique
		if (hdgAttribute->GetAttributeType() == KWType::Continuous)
		{
			descriptiveContinuousStats = cast(KWDescriptiveContinuousStats*,
							  odDescriptiveStats.Lookup(hdgAttribute->GetAttributeName()));

			// Memorisation de ses bornes
			if (descriptiveContinuousStats != NULL)
			{
				hdgAttribute->SetMin(descriptiveContinuousStats->GetMin());
				hdgAttribute->SetMax(descriptiveContinuousStats->GetMax());
			}
		}
		// CH IV Begin
		// CH IV Refactoring: est-ce utile, si on memorise les PartLabels explicitement
		// Recherche des caracteristiques des attributs internes numeriques
		if (hdgAttribute->GetInnerAttributeNumber() > 0)
		{
			cvMinValues = new ContinuousVector;
			cvMaxValues = new ContinuousVector;
			odInnerContinuousAttributeIndexes = new ObjectDictionary;

			for (nInnerAttribute = 0; nInnerAttribute < hdgAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute = coclusteringDataGrid->GetInnerAttributes()->LookupInnerAttribute(
				    hdgAttribute->GetInnerAttributeNameAt(nInnerAttribute));

				// Cas d'un attribut Continuous
				if (innerAttribute->GetAttributeType() == KWType::Continuous)
				{
					descriptiveContinuousStats =
					    cast(KWDescriptiveContinuousStats*,
						 odDescriptiveStats.Lookup(innerAttribute->GetAttributeName()));
					cvMinValues->Add(descriptiveContinuousStats->GetMin());
					cvMaxValues->Add(descriptiveContinuousStats->GetMax());
					io = new IntObject;
					io->SetInt(cvMinValues->GetSize() - 1);
					odInnerContinuousAttributeIndexes->SetAt(innerAttribute->GetAttributeName(),
										 io);
				}
			}
			if (cvMinValues->GetSize() > 0)
			{
				coclusteringDataGrid->SetInnerAttributeMinValues(cvMinValues);
				coclusteringDataGrid->SetInnerAttributeMaxValues(cvMaxValues);
				coclusteringDataGrid->SetInnerContinuousAttributeIndexes(
				    odInnerContinuousAttributeIndexes);
			}
			else
			{
				delete cvMinValues;
				delete cvMaxValues;
				delete odInnerContinuousAttributeIndexes;
			}
		}
		// CH IV End
	}
}

void CCCoclusteringBuilder::ComputeValueTypicalities(const KWDataGrid* inputInitialDataGrid,
						     const KWDataGridCosts* dataGridCosts,
						     CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;

	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		if (optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol)
			ComputeValueTypicalitiesAt(inputInitialDataGrid, dataGridCosts, optimizedDataGrid, nAttribute);
	}
}

void CCCoclusteringBuilder::ComputeValueTypicalitiesAt(const KWDataGrid* inputInitialDataGrid,
						       const KWDataGridCosts* dataGridCosts,
						       CCHierarchicalDataGrid* optimizedDataGrid, int nAttribute) const
{
	boolean bDisplay = false;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	KWDGAttribute* initialAttribute;
	KWDataGrid* univariateInitialDataGrid;
	KWDGPOGrouper dataGridUnivariateGrouper;
	KWDataGridUnivariateCosts* dataGridUnivariateCosts;
	KWDataGridManager dataGridManager;
	KWFrequencyTable initialFrequencyTable;
	IntVector ivGroups;
	int nGroupNumber;
	int nValueNumber;
	KWFrequencyTable groupedFrequencyTable;
	DoubleVector dvGroupCosts;
	int nValue;
	int nIntraCatchAllValue;
	int nGroup;
	double dOutDeltaCost;
	double dInDeltaCost;
	double dDeltaCost;
	int nOutGroup;
	double dTypicality;
	ObjectArray oaValueParts;
	ObjectArray oaGroupParts;
	KWDGPart* dgValuePart;
	KWDGValue* dgValue;
	NumericKeyDictionary nkdOptimizedAttributeValues;
	CCHDGValue* hdgValue;
	DoubleVector dvTypicalities;
	DoubleVector dvMaxTypicalities;
	int nGarbageGroupIndex;
	int nGarbageModalityNumber;
	IntVector ivGroupModalityNumber;
	int nNewGarbageModalityNumber;
	int nNewGroupNumber;
	int nValueModalityNumber;
	double dElementaryTypicality;

	require(optimizedDataGrid != NULL);
	require(inputInitialDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid->Check());
	require(inputInitialDataGrid->Check());
	require(inputInitialDataGrid->GetAttributeNumber() >= optimizedDataGrid->GetAttributeNumber());
	require(0 <= nAttribute and nAttribute < optimizedDataGrid->GetAttributeNumber());
	require(inputInitialDataGrid->SearchAttribute(
		    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName()) != NULL);
	require(optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Parametrage: on transforme les grilles en tableaux (KWFrequencyTable) pour l'attribut etudie
	// Chaque partie de l'attribut est ordonne de la meme facon dans la grille et le tableau
	// Methode fortement inspiree de KWDGPOGrouper::PostOptimizeDataGrid

	// Collecte des parties contenant les valeurs et les groupes
	initialAttribute =
	    inputInitialDataGrid->SearchAttribute(optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	initialAttribute->ExportParts(&oaValueParts);
	optimizedDataGrid->GetAttributeAt(nAttribute)->ExportParts(&oaGroupParts);

	// Construction d'une grille initiale pour l'optimisation univariee
	univariateInitialDataGrid = dataGridPostOptimizer.BuildUnivariateInitialDataGrid(
	    optimizedDataGrid, inputInitialDataGrid, optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());

	// Verification de la compatibilite entre grille optimisee et grille initiale
	dataGridManager.SetSourceDataGrid(univariateInitialDataGrid);
	assert(dataGridManager.CheckDataGrid(optimizedDataGrid));

	// Parametrage des couts d'optimisation univarie de la grille
	dataGridUnivariateGrouper.SetPostOptimizationAttributeName(
	    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	dataGridUnivariateCosts = cast(KWDataGridUnivariateCosts*, dataGridUnivariateGrouper.GetGroupingCosts());
	dataGridUnivariateCosts->SetPostOptimizationAttributeName(
	    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	dataGridUnivariateCosts->SetDataGridCosts(dataGridCosts);
	dataGridUnivariateCosts->InitializeUnivariateCostParameters(optimizedDataGrid);

	// Construction d'une table d'effectif selon l'attribut a post-optimiser, pour la grille initiale
	nValueNumber = univariateInitialDataGrid->GetAttributeAt(nAttribute)->GetPartNumber();
	dataGridUnivariateGrouper.InitializeFrequencyTableFromDataGrid(&initialFrequencyTable,
								       univariateInitialDataGrid);
	assert(initialFrequencyTable.GetFrequencyVectorNumber() == nValueNumber);

	// Initialisation des index de groupes et de l'index du groupe poubelle si present
	nGarbageGroupIndex = dataGridUnivariateGrouper.InitializeGroupIndexesAndGarbageIndex(
	    &ivGroups, univariateInitialDataGrid, optimizedDataGrid);
	nGarbageModalityNumber = 0;

	// Initialisation d'un tableau d'effectif groupe a partir d'une grille initiale et des index des groupes
	nGroupNumber = optimizedDataGrid->GetAttributeAt(nAttribute)->GetPartNumber();
	dataGridUnivariateGrouper.InitializeGroupedFrequencyTableFromDataGrid(
	    &groupedFrequencyTable, &initialFrequencyTable, &ivGroups, nGroupNumber);
	assert(groupedFrequencyTable.GetFrequencyVectorNumber() == nGroupNumber);

	// Memorisation des valeurs de l'attributs optimise dans un dictionnaire
	assert(nGroupNumber == oaGroupParts.GetSize());
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		dgValuePart = cast(KWDGPart*, oaGroupParts.GetAt(nGroup));
		assert(dgValuePart->GetAttribute() == optimizedDataGrid->GetAttributeAt(nAttribute));

		// Memorisation des valeurs
		dgValue = dgValuePart->GetValueSet()->GetHeadValue();
		while (dgValue != NULL)
		{
			nkdOptimizedAttributeValues.SetAt((dgValue->GetValue().GetNumericKey()), dgValue);
			dgValuePart->GetValueSet()->GetNextValue(dgValue);
		}

		// Memorisation du nombre de modalites du groupe
		ivGroupModalityNumber.Add(dgValuePart->GetValueSet()->GetTrueValueNumber());

		// Cas du groupe poubelle : memorisation du nombre de modalites
		if (nGroup == nGarbageGroupIndex)
		{
			nGarbageModalityNumber = dgValuePart->GetValueSet()->GetTrueValueNumber();
			// CH AB AF adaptation eventuelle VarPart
		}
	}

	// Tri du vecteur de nombre de modalites (tri croissant)
	ivGroupModalityNumber.Sort();

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul des variations de cout lors de deplacement de valeur d'un groupe vers un autre
	// Methode fortement inspiree de KWGrouperMODL::FastPostOptimizeGroups

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		dvGroupCosts.SetAt(nGroup, dataGridUnivariateGrouper.ComputeGroupCost(
					       groupedFrequencyTable.GetFrequencyVectorAt(nGroup)));
	}

	// Affichage de resultats: entete
	if (bDisplay)
		cout << "\nValue\tOutGroup\tGroup\tOutDCost\tInDCost\tDCost\n";

	// Initialisation des typicites max
	dvMaxTypicalities.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvMaxTypicalities.SetAt(nGroup, -DBL_MAX);

	// Parcours de toutes les modalites
	// Il s'agit du parcours des modalites de la table initiale
	// Si la table initiale est issue d'une granularisation, il s'agit des modalites elementaires ou du fourre-tout
	// (super modalite) Dans le cas du fourre-tout, on calcule ici la typicite globale du fourre-tout en envisageant
	// son deplacement (deplacement de toutes les modalites du fourre-tout)
	dvTypicalities.SetSize(nValueNumber);
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		// Recherche du groupe de rattachement de la modalite
		nOutGroup = ivGroups.GetAt(nValue);

		// Nombre de modalites associe : peut etre superieur a 1 dans le cas de la super modalite (fourre-tout)
		// Il doit alors s'agir de la derniere modalite
		nValueModalityNumber = initialFrequencyTable.GetFrequencyVectorAt(nValue)->GetModalityNumber();
		assert(nValueModalityNumber == 1 or nValue == nValueNumber - 1);

		// Calcul du cout du groupe apres le depart de la modalite,
		// en se basant sur les nouveaux effectifs du groupe
		dOutDeltaCost = dataGridUnivariateGrouper.ComputeGroupDiffCost(
		    groupedFrequencyTable.GetFrequencyVectorAt(nOutGroup),
		    initialFrequencyTable.GetFrequencyVectorAt(nValue));
		dOutDeltaCost -= dvGroupCosts.GetAt(nOutGroup);

		// Parcours des groupes cible potentiels
		dTypicality = 0;

		nNewGroupNumber = nGroupNumber;
		// Cas ou la modalite etait la seule de son groupe : la taille de la nouvelle partition est decrementee
		// de 1 : attention si nouveau nombre de groupes = 2, il ne peut pas y avoir de poubelle
		if (groupedFrequencyTable.GetFrequencyVectorAt(nOutGroup)->ComputeTotalFrequency() ==
		    initialFrequencyTable.GetFrequencyVectorAt(nValue)->ComputeTotalFrequency())
			nNewGroupNumber = nGroupNumber - 1;

		for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		{
			// On n'evalue que les nouveaux groupes potentiels
			if (nGroup != nOutGroup)
			{
				// Calcul du cout du groupe apres l'arrivee de la modalite,
				// en se basant sur les nouveaux effectifs du groupe
				dInDeltaCost = dataGridUnivariateGrouper.ComputeGroupUnionCost(
				    groupedFrequencyTable.GetFrequencyVectorAt(nGroup),
				    initialFrequencyTable.GetFrequencyVectorAt(nValue));
				dInDeltaCost -= dvGroupCosts.GetAt(nGroup);

				// Evaluation de la variation de cout globale
				dDeltaCost = dOutDeltaCost + dInDeltaCost;

				// Cas d'une nouvelle partition en deux groupes : partition obligatoirement sans groupe
				// poubelle
				if (nNewGroupNumber == 2)
					dDeltaCost +=
					    dataGridUnivariateGrouper.ComputePartitionCost(nNewGroupNumber, 0) -
					    dataGridUnivariateGrouper.ComputePartitionCost(nGroupNumber,
											   nGarbageModalityNumber);

				// Sinon, cas d'une partition avec groupe poubelle
				else if (nGarbageModalityNumber > 0)
				{
					nNewGarbageModalityNumber = nGarbageModalityNumber;
					// Cas ou la modalite part du groupe poubelle
					if (nOutGroup == nGarbageGroupIndex)
					{
						// Taille du groupe apres le depart de la modalite
						nNewGarbageModalityNumber =
						    nGarbageModalityNumber - nValueModalityNumber;
						// Est ce que le 2nd plus gros groupe devient le groupe poubelle ?
						if (nNewGarbageModalityNumber <
						    ivGroupModalityNumber.GetAt(ivGroupModalityNumber.GetSize() - 2))
							nNewGarbageModalityNumber = ivGroupModalityNumber.GetAt(
							    ivGroupModalityNumber.GetSize() - 2);
					}
					// Comparaison avec le nombre de modalites du groupe d'accueil
					if (nNewGarbageModalityNumber <
					    groupedFrequencyTable.GetFrequencyVectorAt(nGroup)->GetModalityNumber() +
						nValueModalityNumber)
						nNewGarbageModalityNumber =
						    groupedFrequencyTable.GetFrequencyVectorAt(nGroup)
							->GetModalityNumber() +
						    nValueModalityNumber;

					// Variation du cout de partition avec groupe poubelle
					dDeltaCost += dataGridUnivariateGrouper.ComputePartitionCost(
							  nNewGroupNumber, nNewGarbageModalityNumber) -
						      dataGridUnivariateGrouper.ComputePartitionCost(
							  nGroupNumber, nGarbageModalityNumber);
				}

				// CH IV Refactoring: nettoyer lignes suivantes?
				// CH DDD
				if (bDisplay and dDeltaCost < 0)
					cout << "ComputeValueTypicalities :: dDeltaCost < 0\t" << dDeltaCost << endl;

				// assert(dDeltaCost >= 0);

				// Cumul de la typicite
				dTypicality += dDeltaCost;

				// Affichage de resultats: ligne de detail
				if (bDisplay)
					cout << oaValueParts.GetAt(nValue)->GetObjectLabel() << "\t"
					     << oaGroupParts.GetAt(nOutGroup)->GetObjectLabel() << "\t"
					     << oaGroupParts.GetAt(nGroup)->GetObjectLabel() << "\t" << dOutDeltaCost
					     << "\t" << dInDeltaCost << "\t" << dDeltaCost << endl;
			}
		}

		// Calcul de la typicite: variation de cout moyenne lorsque la modalite change de groupe
		if (nGroupNumber > 1)
			dTypicality /= nGroupNumber - 1;

		// Cas d'une modalite elementaire
		if (nValueModalityNumber == 1)
		{
			// Memorisation de la typicalite
			dvTypicalities.SetAt(nValue, dTypicality);

			// Mise a jour typicite max
			if (dTypicality > dvMaxTypicalities.GetAt(nOutGroup))
				dvMaxTypicalities.SetAt(nOutGroup, dTypicality);
		}
		// Sinon : cas du fourre-tout
		else
		{
			// Retaillage du vecteur des typicites qui devient vecteur des typicites par modalite
			// elementaire
			dvTypicalities.SetSize(dvTypicalities.GetSize() + nValueModalityNumber - 1);

			// Acces a la valeur
			dgValuePart = cast(KWDGPart*, oaValueParts.GetAt(nValue));

			dgValue = dgValuePart->GetValueSet()->GetHeadValue();
			// Repartition de la typicite du fourre-tout entre ses modalites elementaires
			for (nIntraCatchAllValue = 0; nIntraCatchAllValue < nValueModalityNumber; nIntraCatchAllValue++)
			{
				// Recherche de la valeur correspondante pour l'attribut optimise
				hdgValue =
				    cast(CCHDGValue*,
					 nkdOptimizedAttributeValues.Lookup((dgValue->GetValue().GetNumericKey())));

				// Calcul de la typicite elementaire = la typicite du fourre-tout * effectif  de la
				// modalite elementaire / effectif total du fourre-tout
				dElementaryTypicality = (dTypicality * hdgValue->GetValueFrequency()) /
							(1.0 * dgValuePart->GetPartFrequency());
				dvTypicalities.SetAt(nValue + nIntraCatchAllValue, dElementaryTypicality);

				// Mise a jour typicite max du groupe
				if (dElementaryTypicality > dvMaxTypicalities.GetAt(nOutGroup))
					dvMaxTypicalities.SetAt(nOutGroup, dElementaryTypicality);

				// Modalite suivante
				dgValuePart->GetValueSet()->GetNextValue(dgValue);
			}
		}
	}

	// Normalisation de typicite dans chaque groupe par la typicite max du groupe
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		// Acces a la valeur
		dgValuePart = cast(KWDGPart*, oaValueParts.GetAt(nValue));
		assert(dgValuePart->GetValueSet()->GetValueNumber() == 1 or
		       // n'est plus garanti car la grille initiale contient un fourre-tout
		       //(dgValuePart->GetValueSet()->GetValueNumber() == 2 and
		       dgValuePart->GetValueSet()->GetTailValue()->GetValue() == Symbol::GetStarValue());

		// Acces au nombre de modalites de la valeur
		nValueModalityNumber = initialFrequencyTable.GetFrequencyVectorAt(nValue)->GetModalityNumber();

		// Recherche du groupe de rattachement de la modalite
		nOutGroup = ivGroups.GetAt(nValue);

		// Cas d'une modalite non fourre-tout
		if (dgValuePart->GetValueSet()->GetTrueValueNumber() == 1)
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			// Recherche de la valeur correspondante pour l'attribut optimise
			hdgValue = cast(CCHDGValue*,
					nkdOptimizedAttributeValues.Lookup((dgValue->GetValue().GetNumericKey())));

			// Memorisation de la typicalite normalisee
			// Mis en defaut avec Breast pour nValue = 14
			// assert(0 <= dvTypicalities.GetAt(nValue) and dvTypicalities.GetAt(nValue) <=
			// dvMaxTypicalities.GetAt(nOutGroup));

			if (dvMaxTypicalities.GetAt(nOutGroup) > 0)
				hdgValue->SetTypicality(dvTypicalities.GetAt(nValue) /
							dvMaxTypicalities.GetAt(nOutGroup));
			else
				hdgValue->SetTypicality(1);
		}
		// Sinon : cas du fourre-tout
		else
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			for (nIntraCatchAllValue = 0; nIntraCatchAllValue < nValueModalityNumber; nIntraCatchAllValue++)
			{
				// Recherche de la valeur correspondante pour l'attribut optimise
				hdgValue =
				    cast(CCHDGValue*,
					 nkdOptimizedAttributeValues.Lookup((dgValue->GetValue().GetNumericKey())));

				if (dvMaxTypicalities.GetAt(nOutGroup) > 0)
					hdgValue->SetTypicality(dvTypicalities.GetAt(nValue + nIntraCatchAllValue) /
								dvMaxTypicalities.GetAt(nOutGroup));
				else
					hdgValue->SetTypicality(1);

				// Modalite suivante
				dgValuePart->GetValueSet()->GetNextValue(dgValue);
			}
		}
	}

	// Nettoyage
	delete univariateInitialDataGrid;
}

void CCCoclusteringBuilder::ComputePartInterests(const KWDataGridMerger* optimizedDataGridMerger,
						 const KWDataGridCosts* dataGridCosts,
						 CCHierarchicalDataGrid* optimizedDataGrid) const
{
	boolean bDisplay = false;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCHDGAttribute* hdgAttribute;
	int nPart;
	KWDGPart* part1;
	KWDGMPart* partM1;
	KWDGPart* part2;
	KWDGMPart* partM2;
	KWDGMPartMerge* partMerge;
	CCHDGPart* hdgPart;
	double dTotalDefaultCost;
	double dBestDataGridTotalCost;
	double dDeltaCost;
	double dInterest;
	double dTotalInterest;
	double dMaxInterest;
	DoubleVector dvInterests;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Cout par defaut et meilleur cout
	dTotalDefaultCost = dataGridCosts->GetTotalDefaultCost();
	dBestDataGridTotalCost =
	    optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(optimizedDataGridMerger);

	// Ecriture des distances inter-cluster pour chaque attribut
	for (nAttribute = 0; nAttribute < optimizedDataGridMerger->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);

		// Recherche de l'attribut de coclustering correspondant
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));
		assert(hdgAttribute->GetAttributeName() == dgAttribute->GetAttributeName());
		assert(hdgAttribute->GetPartNumber() == dgAttribute->GetPartNumber());

		// Affichage des resultats: entete
		if (bDisplay)
			cout << "Attribute\tPart1\tPart2\tDefault\tBest\tMerge\tCost\tInfo\n";

		// Parcours de toutes les parties de l'attribut
		dvInterests.SetSize(dgAttribute->GetPartNumber());
		dMaxInterest = -DBL_MAX;
		part1 = dgAttribute->GetHeadPart();
		nPart = 0;
		while (part1 != NULL)
		{
			partM1 = cast(KWDGMPart*, part1);

			// Parcours de toutes les autres parties de l'attribut pour evaluer leur fusion
			dTotalInterest = 0;
			part2 = dgAttribute->GetHeadPart();
			while (part2 != NULL)
			{
				partM2 = cast(KWDGMPart*, part2);

				// Recherche d'une fusion entre les parties
				partMerge = partM1->LookupPartMerge(partM2);

				// Calcul de la typicite
				dDeltaCost = 0;
				if (partMerge != NULL)
					dDeltaCost = partMerge->GetMergeCost();
				// CH IV
				// Ce MergeCost ne tient pas compte des fusions de PV au sein du nouveau cluster
				dInterest = dDeltaCost / (dTotalDefaultCost - dBestDataGridTotalCost);
				dTotalInterest += dInterest;

				// Affichage des resultats: details
				if (bDisplay)
					cout << dgAttribute->GetAttributeName() << "\t" << part1->GetObjectLabel()
					     << "\t" << part2->GetObjectLabel() << "\t" << dTotalDefaultCost << "\t"
					     << dBestDataGridTotalCost << "\t" << dDeltaCost << "\t"
					     << dBestDataGridTotalCost + dDeltaCost << "\t" << dInterest << "\n";

				// Partie suivante
				dgAttribute->GetNextPart(part2);
			}

			// Memorisation de la typicite moyenne
			if (dgAttribute->GetPartNumber() > 1)
				dInterest = dTotalInterest / (dgAttribute->GetPartNumber() - 1);
			else
				dInterest = 1;
			// coclusteringPart->SetInterest(dInterest);
			dvInterests.SetAt(nPart, dInterest);

			// Mise a jour du max
			if (dInterest > dMaxInterest)
				dMaxInterest = dInterest;

			// Partie suivante
			dgAttribute->GetNextPart(part1);
			nPart++;
		}

		// Normalisation des typicites par leur max
		part1 = hdgAttribute->GetHeadPart();
		debug(part2 = dgAttribute->GetHeadPart();) nPart = 0;
		while (part1 != NULL)
		{
			hdgPart = cast(CCHDGPart*, part1);
			debug(assert(part1->GetObjectLabel() == part2->GetObjectLabel());)

			    // Initialisation du niveau hierarchique
			    hdgPart->SetHierarchicalLevel(1);

			// Mise a jour de la typicalite
			assert(0 <= dvInterests.GetAt(nPart) and dvInterests.GetAt(nPart) <= dMaxInterest);
			if (dMaxInterest > 0)
				hdgPart->SetInterest(dvInterests.GetAt(nPart) / dMaxInterest);
			else
				hdgPart->SetInterest(1);

			// Initialisation du ranh hierarchique avec le nombre total de parties
			hdgPart->SetHierarchicalRank(optimizedDataGrid->GetTotalPartNumber());

			// Partie suivante
			hdgAttribute->GetNextPart(part1);
			debug(dgAttribute->GetNextPart(part2);) nPart++;
		}
	}
}

void CCCoclusteringBuilder::ComputePartHierarchies(KWDataGridMerger* optimizedDataGridMerger,
						   const KWDataGridCosts* dataGridCosts,
						   CCHierarchicalDataGrid* optimizedDataGrid) const
{
	boolean bDisplay = false;
	double dDataGridTotalCost;
	double dBestDataGridTotalCost;
	double dBestDeltaCost;
	double dTotalDefaultCost;
	double dHierarchicalLevel;
	double dInterest;
	KWDGMPartMerge* bestPartMerge;
	boolean bContinue;
	int nCount;
	NumericKeyDictionary nkdHierarchicalParts;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCHDGAttribute* hdgAttribute;
	int nPart;
	KWDGPart* dgPart;
	KWDGMPart* dgmMergedPart;
	KWDGPart* dgPart2;
	CCHDGPart* hdgPart;
	CCHDGPart* hdgParentPart;
	double dEpsilon = 1e-10;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Memorisation dans un dictionnaire des parties de coclustering associee aux parties
	// initiales du merger de grille (qui doivent etre de structure correspondante (mais non egale))
	for (nAttribute = 0; nAttribute < optimizedDataGridMerger->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);

		// Recherche de l'attribut de coclustering correspondant
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));
		assert(hdgAttribute->GetAttributeName() == dgAttribute->GetAttributeName());
		assert(hdgAttribute->GetPartNumber() == dgAttribute->GetPartNumber());

		// Parcours synchronise des parties de l'attribut
		nPart = 0;
		dgPart = dgAttribute->GetHeadPart();
		dgPart2 = hdgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			hdgPart = cast(CCHDGPart*, dgPart2);
			assert(hdgPart->GetObjectLabel() == dgPart->GetObjectLabel());

			// Memorisation de l'association dans le dictionnaire
			nkdHierarchicalParts.SetAt(dgPart, hdgPart);

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
			hdgAttribute->GetNextPart(dgPart2);
			nPart++;
		}
	}
	assert(nkdHierarchicalParts.GetCount() == optimizedDataGridMerger->GetTotalPartNumber());

	// Cout par defaut et meilleur cout
	dTotalDefaultCost = dataGridCosts->GetTotalDefaultCost();
	dDataGridTotalCost =
	    optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(optimizedDataGridMerger);
	dBestDataGridTotalCost = dDataGridTotalCost;

	// Affichage: entete
	if (bDisplay)
		cout << "\nCount\tAttribute\tPart1\tPart2\tBest\tMerge\tCost\tDefault cost\tHierachicalLevel\n";

	// Boucle de recherche d'ameliorations
	nCount = 0;
	bContinue = true;
	while (bContinue)
	{
		nCount++;

		// Recherche de la meilleure amelioration
		dBestDeltaCost = optimizedDataGridMerger->SearchBestPartMergeWithGarbageSearch(bestPartMerge);

		// CH IV
		// Ce cout n'est pas le vrai cout car la fusion n'est pas suivie issue de la fusion des PV adjacent

		bContinue = (bestPartMerge != NULL);
		assert(bContinue or dBestDeltaCost == DBL_MAX);

		// Impact de la meilleure amelioration
		if (bContinue)
		{
			// Affichage des details de la fusion
			dHierarchicalLevel = (dTotalDefaultCost - (dDataGridTotalCost + dBestDeltaCost));
			if (dTotalDefaultCost - dBestDataGridTotalCost > 0)
				dHierarchicalLevel /= dTotalDefaultCost - dBestDataGridTotalCost;
			// En presence d'un groupe poubelle la repartition des couts ne garantit plus dHierarchicalLevel
			// <=1 assert(dHierarchicalLevel <= 1 + dEpsilon);

			if (dHierarchicalLevel > 1 - dEpsilon)
				dHierarchicalLevel = 1;

			// Attention, le hierarchical level peut etre negatif: ici, on arrondi uniquement les
			// presque-zero
			if (fabs(dHierarchicalLevel) < dEpsilon)
				dHierarchicalLevel = 0;

			// Affichage: detail
			if (bDisplay)
				cout << nCount << "\t" << bestPartMerge->GetPart1()->GetAttribute()->GetObjectLabel()
				     << "\t" << bestPartMerge->GetPart1()->GetObjectLabel() << "\t"
				     << bestPartMerge->GetPart2()->GetObjectLabel() << "\t" << dBestDataGridTotalCost
				     << "\t" << dBestDeltaCost << "\t" << dDataGridTotalCost + dBestDeltaCost << "\t"
				     << dTotalDefaultCost << "\t" << dHierarchicalLevel << "\n";

			// Recherche de l'attribut correspondant a la fusion de partie
			nAttribute = bestPartMerge->GetPart1()->GetAttributeIndex();
			dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);
			hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));

			// Creation d'une nouvelle partie de coclustering
			hdgParentPart = hdgAttribute->NewHierarchyPart();
			hdgParentPart->SetHierarchicalLevel(dHierarchicalLevel);

			// Identifiant base sur la distance a la racine
			hdgParentPart->SetPartName(
			    BuildHierachicalPartName(hdgAttribute, optimizedDataGridMerger->GetTotalPartNumber() -
								       optimizedDataGridMerger->GetAttributeNumber()));

			// Rang hierarchique, basee sur le nombre de partie restante (apres la fusion)
			hdgParentPart->SetHierarchicalRank(optimizedDataGridMerger->GetTotalPartNumber() - 1);

			// Lien avec ses partie filles
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup(bestPartMerge->GetPart1()));
			hdgPart->SetParentPart(hdgParentPart);
			hdgParentPart->SetChildPart1(hdgPart);
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup(bestPartMerge->GetPart2()));
			hdgPart->SetParentPart(hdgParentPart);
			hdgParentPart->SetChildPart2(hdgPart);

			// Effectif
			hdgParentPart->SetPartFrequency(hdgParentPart->GetChildPart1()->GetPartFrequency() +
							hdgParentPart->GetChildPart2()->GetPartFrequency());

			// Typicite par moyenne ponderee des typicites des parties filles
			dInterest = (hdgParentPart->GetChildPart1()->GetPartFrequency() *
					 hdgParentPart->GetChildPart1()->GetInterest() +
				     hdgParentPart->GetChildPart2()->GetPartFrequency() *
					 hdgParentPart->GetChildPart2()->GetInterest());
			if (hdgParentPart->GetPartFrequency() > 0)
				dInterest /= hdgParentPart->GetPartFrequency();
			hdgParentPart->SetInterest(dInterest);

			// Realisation de la fusion
			dgmMergedPart = optimizedDataGridMerger->PerformPartMerge(bestPartMerge);
			dDataGridTotalCost += dBestDeltaCost;

			// Pour les attributs Continuous, on utilise le nom d'intervalle comme identifiant de cluster
			// On le fait apres la fusion, pour beneficier de la methode GetObjectLabel des intervalles
			if (dgmMergedPart->GetAttribute()->GetAttributeType() == KWType::Continuous)
			{
				hdgParentPart->SetPartName(dgmMergedPart->GetObjectLabel());

				// Test si la partie fusionne de droite impliquait la valeur manquante
				// Dans ce cas, on rajoute * en tete de l'identifiant de la partie, pour distinguer le
				// cas ]-inf,ub] de *]-inf, ub]
				if (hdgParentPart->GetChildPart1()->GetInterval()->GetUpperBound() ==
					KWContinuous::GetMissingValue() or
				    (hdgParentPart->GetChildPart1()->IsParent() and
				     hdgParentPart->GetChildPart1()->GetPartName().GetAt(0) == '*'))
					hdgParentPart->SetPartName("*" + hdgParentPart->GetPartName());
			}

			// Si derniere partie de l'attribut, on memorise la racine de la hierarchie
			if (dgmMergedPart->GetAttribute()->GetPartNumber() == 1)
				hdgAttribute->SetRootPart(hdgParentPart);

			// Memorisation de la nouvelle partie
			nkdHierarchicalParts.SetAt(dgmMergedPart, hdgParentPart);
		}
	}
}

const ALString CCCoclusteringBuilder::BuildHierachicalPartName(const CCHDGAttribute* hdgAttribute,
							       int nHierarchicalIndex) const
{
	const char cIdentifierPrefix = 'A';
	ALString sPartName;

	require(hdgAttribute != NULL);
	require(nHierarchicalIndex > 0);

	// Construction du nom de la partie
	sPartName = cIdentifierPrefix;
	sPartName.SetAt(0, cIdentifierPrefix + char(hdgAttribute->GetAttributeIndex()));
	sPartName += IntToString(nHierarchicalIndex);
	return sPartName;
}

void CCCoclusteringBuilder::ComputePartRanks(const KWDataGridMerger* optimizedDataGridMerger,
					     const KWDataGridCosts* dataGridCosts,
					     CCHierarchicalDataGrid* optimizedDataGrid) const
{
	ObjectArray oaAttributeParts;
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	CCHDGPart* hdgRootgPart;
	CCHDGPart* hdgPart;
	CCHDGPart* hdgChildPart1;
	CCHDGPart* hdgChildPart2;
	int nPart;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));

		// Export des parties
		oaAttributeParts.SetSize(0);
		hdgAttribute->ExportHierarchyParts(&oaAttributeParts);

		// Parcours de toutes les parties de l'attribut et de sa structure hierarchique
		// pour reordonner les sous-branches de chaque noeud intermediaire
		for (nPart = 0; nPart < oaAttributeParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaAttributeParts.GetAt(nPart));
			assert(not hdgPart->IsLeaf() or hdgPart->Check());

			// Reordonnancement des sous parties par maximum d'interet pour les parties intermediaires de la
			// hierarchie Uniquement dans le cas Symbol
			if (hdgAttribute->GetAttributeType() == KWType::Symbol)
			{
				hdgChildPart1 = hdgPart->GetChildPart1();
				hdgChildPart2 = hdgPart->GetChildPart2();
				if (hdgChildPart1 != NULL and hdgChildPart2 != NULL)
				{
					if (hdgChildPart1->GetInterest() < hdgChildPart2->GetInterest())
					{
						hdgPart->SetChildPart1(hdgChildPart2);
						hdgPart->SetChildPart2(hdgChildPart1);
					}
					// En cas d'egalite des interets des parties, comparaison de leur effectif
					else if (hdgChildPart1->GetInterest() == hdgChildPart2->GetInterest())
					{
						if (hdgChildPart1->GetPartFrequency() <
						    hdgChildPart2->GetPartFrequency())
						{
							hdgPart->SetChildPart1(hdgChildPart2);
							hdgPart->SetChildPart2(hdgChildPart1);
						}
						// En cas d'egalite d'effectif, comparaison lexicographique des noms des
						// parties
						else if (hdgChildPart1->GetPartFrequency() ==
							 hdgChildPart2->GetPartFrequency())
						{
							if (hdgChildPart1->GetPartName() > hdgChildPart2->GetPartName())
							{
								hdgPart->SetChildPart1(hdgChildPart2);
								hdgPart->SetChildPart2(hdgChildPart1);
							}
						}
					}
				}
			}
		}

		// Numerotation (Ranks) des noeuds d'un arbre de partie de coclustering par parcours infixe
		hdgRootgPart = hdgAttribute->GetRootPart();
		check(hdgRootgPart);
		ComputePartInfixRanks(hdgRootgPart);
	}
}

void CCCoclusteringBuilder::ComputePartInfixRanks(CCHDGPart* hdgRootgPart) const
{
	CCHDGPart* hdgPreviousPart;
	CCHDGPart* hdgActualPart;
	CCHDGPart* hdgNextPart;
	int nRank;

	require(hdgRootgPart != NULL);
	require(cast(CCHDGAttribute*, hdgRootgPart->GetAttribute())->GetRootPart() == hdgRootgPart);

	// Parcours de l'arbre en infixe a partir de la racine, pour numerotation des noeuds
	// Emprunte a wikipedia
	// VisiterInfixeIteratif(racine)
	// precedent    := null
	// actuel	:= racine
	// suivant	:= null
	//
	// Tant que (actuel != null) Faire
	//     Si (precedent == pere(actuel)) Alors
	//	  precedent := actuel
	//	  suivant   := gauche(actuel)
	//     FinSi
	//     Si (suivant == null OU precedent == gauche(actuel)) Alors
	//	  Visiter(actuel)
	//	  precedent := actuel
	//	  suivant   := droite(actuel)
	//     FinSi
	//     Si (suivant == null OU precedent == droite(actuel)) Alors
	//	  precedent := actuel
	//	  suivant   := pere(actuel)
	//     FinSi
	//     actuel := suivant
	// FinTantQue
	nRank = 1;
	hdgPreviousPart = NULL;
	hdgActualPart = hdgRootgPart;
	hdgNextPart = NULL;
	while (hdgActualPart != NULL)
	{
		if (hdgPreviousPart == hdgActualPart->GetParentPart())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetChildPart1();
		}
		if (hdgNextPart == NULL or hdgPreviousPart == hdgActualPart->GetChildPart1())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetChildPart2();

			// Numerotation du noeud
			hdgActualPart->SetRank(nRank);
			nRank++;
		}
		if (hdgNextPart == NULL or hdgPreviousPart == hdgActualPart->GetChildPart2())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetParentPart();
		}
		hdgActualPart = hdgNextPart;
	}
}

void CCCoclusteringBuilder::SortAttributePartsAndValues(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgPart;

	require(optimizedDataGrid != NULL);

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGrid->GetAttributeAt(nAttribute);

		// Tri des parties
		cast(CCHDGAttribute*, dgAttribute)->SortPartsByRank();

		// Nommage des parties terminales et tri des valeurs pour les attributs categoriels
		dgPart = dgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			// Tri des valeurs de la partie si attribut categoriel
			if (dgAttribute->GetAttributeType() == KWType::Symbol)
				cast(CCHDGValueSet*, dgPart->GetValueSet())->SortValuesByTypicality();

			// Initialisation du nom de la partie
			cast(CCHDGPart*, dgPart)->SetPartName(dgPart->GetObjectLabel());

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringOptimizer

CCCoclusteringOptimizer::CCCoclusteringOptimizer()
{
	coclusteringBuilder = NULL;
}

CCCoclusteringOptimizer::~CCCoclusteringOptimizer() {}

void CCCoclusteringOptimizer::SetCoclusteringBuilder(const CCCoclusteringBuilder* builder)
{
	coclusteringBuilder = builder;
}

const CCCoclusteringBuilder* CCCoclusteringOptimizer::GetCoclusteringBuilder()
{
	return coclusteringBuilder;
}

void CCCoclusteringOptimizer::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						     const KWDataGrid* initialGranularizedDataGrid,
						     boolean bIsLastSaving) const
{
	// Integration de la granularite
	if (coclusteringBuilder != NULL)
		coclusteringBuilder->HandleOptimizationStep(optimizedDataGrid, initialGranularizedDataGrid,
							    bIsLastSaving);
}
