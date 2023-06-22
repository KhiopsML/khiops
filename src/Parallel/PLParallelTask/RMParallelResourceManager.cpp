// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMParallelResourceManager.h"

// Somme de de longint en gerant le depassement
static longint lsum(longint l1, longint l2)
{
	if (l1 > LLONG_MAX - l2)
		return LLONG_MAX;
	return l1 + l2;
}
static longint lsum(longint l1, longint l2, longint l3)
{
	return lsum(l1, lsum(l2, l3));
}

static longint lsum(longint l1, longint l2, longint l3, longint l4)
{
	return lsum(l1, lsum(l2, l3, l4));
}

static longint lsum(longint l1, longint l2, longint l3, longint l4, longint l5)
{
	return lsum(lsum(l1, l2, l3, l4), l5);
}
///////////////////////////////////////////////////////////////////////
// Implementation de RMParallelResourceManager

RMParallelResourceManager::RMParallelResourceManager()
{
	clusterResources = NULL;
	taskRequirements = NULL;
}

RMParallelResourceManager ::~RMParallelResourceManager()
{
	Clean();
}

boolean RMParallelResourceManager::ComputeGrantedResources(const RMTaskResourceRequirement* resourceRequirement,
							   RMTaskResourceGrant* grantedResource)
{
	RMResourceSystem* newSystem;
	boolean bRet;

	require(resourceRequirement != NULL);
	require(GetProcessId() == 0);

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->MasterInitializeResourceSystem();

	// Copie du systeme et suppression d'un esclave par machine sur la copie
	newSystem = RMResourceManager::GetResourceSystem()->Clone();
	newSystem->RemoveProcesses(PLTaskDriver::GetDriver()->GetFileServers());

	// Calcul des ressources
	bRet = ComputeGrantedResourcesForCluster(newSystem, resourceRequirement, grantedResource);
	delete newSystem;

	return bRet;
}

boolean
RMParallelResourceManager::ComputeGrantedResourcesForCluster(const RMResourceSystem* resourceSystem,
							     const RMTaskResourceRequirement* resourceRequirement,
							     RMTaskResourceGrant* grantedResource)
{
	require(grantedResource != NULL);

	RMParallelResourceManager manager;
	const PLClusterSolution* solution;

	if (PLParallelTask::GetTracerResources() > 0)
	{
		if (PLParallelTask::GetCurrentTaskName() != "")
			cout << endl << "Task " << PLParallelTask::GetCurrentTaskName() << endl;
		else
			cout << endl << "Outside task" << endl;

		cout << endl << endl << *resourceSystem;
		cout << "\tSlave reserve  " << LongintToHumanReadableString(PLHostSolution::GetSlaveReserve(MEMORY))
		     << endl;
		cout << "\tMaster reserve " << LongintToHumanReadableString(PLHostSolution::GetMasterReserve(MEMORY))
		     << endl
		     << endl;
		cout << *resourceRequirement << endl;
		cout << RMResourceConstraints::ToString();
	}

	// Initialisation du gestionnaire de ressources
	manager.SetRequirement(resourceRequirement);
	manager.SetResourceCluster(resourceSystem);

	// Calcul des ressources pour le systeme
	solution = manager.Solve();
	// cout << *solution << endl;
	RMParallelResourceManager::BuildGrantedResources(solution, resourceRequirement, grantedResource);

	if (PLParallelTask::GetTracerResources() > 0)
	{
		cout << *grantedResource << endl;
	}

	// Verification des resultats
	debug(manager.Check(grantedResource));
	return not grantedResource->IsEmpty();
}

void RMParallelResourceManager::BuildGrantedResources(const PLClusterSolution* solution,
						      const RMTaskResourceRequirement* resourceRequirement,
						      RMTaskResourceGrant* grantedResource)
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;
	int nProcNumber;
	RMResourceGrant* resourceGrant;
	int nRank;
	int nRT;

	require(grantedResource != NULL);

	// Recopie des ressources allouees
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		grantedResource->SetMasterResource(nRT, solution->GetMasterResource(nRT));
		grantedResource->SetSharedResource(nRT, solution->GetSharedResource(nRT));
		grantedResource->SetGlobalResource(nRT, solution->GetGlobalResource(nRT));
		grantedResource->SetSlaveResource(nRT, solution->GetSlaveResource(nRT));
	}

	// Construction des RMResourceGrant
	if (solution->GetProcessSolutionNumber() > 0)
	{
		if (solution->GetProcessSolutionNumber() > 1)
		{
			position = solution->GetHostSolutions()->GetStartPosition();
			while (position != NULL)
			{
				solution->GetHostSolutions()->GetNextAssoc(position, sHostName, oElement);
				hostSolution = cast(PLHostSolution*, oElement);

				grantedResource->SetHostCoreNumber(sHostName, hostSolution->GetProcNumber());
				if (hostSolution->GetHost()->IsMasterHost())
				{
					// Cas particulier du master : on s'assure qu'il y ait le processus de rang 0
					// sur la machine
					assert(hostSolution->GetProcNumber() != 0);
					resourceGrant = new RMResourceGrant;
					resourceGrant->SetRank(0);
					resourceGrant->SetDiskSpace(solution->GetMasterResource(DISK));
					resourceGrant->SetMemory(solution->GetMasterResource(MEMORY));
					resourceGrant->SetSharedDisk(solution->GetSharedResource(DISK));
					resourceGrant->SetSharedMemory(solution->GetSharedResource(MEMORY));
					grantedResource->AddResource(resourceGrant);
				}

				// On construit autant de solutions qu'il y a de processus sur la machine
				// (sauf pour le rang 0 cf ci-dessus)
				for (nProcNumber = 0; nProcNumber < hostSolution->GetProcNumber(); nProcNumber++)
				{
					if (PLParallelTask::GetParallelSimulated())
						nRank = nProcNumber;
					else
						nRank = hostSolution->GetHost()->GetRanks()->GetAt(nProcNumber);
					if (nRank != 0)
					{
						resourceGrant = new RMResourceGrant;
						resourceGrant->SetRank(nRank);
						resourceGrant->SetDiskSpace(lsum(solution->GetSlaveResource(DISK),
										 solution->GetGlobalResource(DISK)));
						resourceGrant->SetMemory(lsum(solution->GetSlaveResource(MEMORY),
									      solution->GetGlobalResource(MEMORY)));
						resourceGrant->SetSharedDisk(solution->GetSharedResource(DISK));
						resourceGrant->SetSharedMemory(solution->GetSharedResource(MEMORY));
						grantedResource->AddResource(resourceGrant);
					}
				}
			}
		}
		else
		{
			// Cas sequentiel
			// Ajout du maitre
			resourceGrant = new RMResourceGrant;
			resourceGrant->SetRank(0);
			resourceGrant->SetDiskSpace(solution->GetMasterResource(DISK));
			resourceGrant->SetMemory(solution->GetMasterResource(MEMORY));
			resourceGrant->SetSharedDisk(solution->GetSharedResource(DISK));
			resourceGrant->SetSharedMemory(solution->GetSharedResource(MEMORY));
			grantedResource->AddResource(resourceGrant);

			// Ajout de l'esclave (virtuel)
			resourceGrant = new RMResourceGrant;
			resourceGrant->SetRank(1);
			resourceGrant->SetDiskSpace(
			    lsum(solution->GetSlaveResource(DISK), solution->GetGlobalResource(DISK)));
			resourceGrant->SetMemory(
			    lsum(solution->GetSlaveResource(MEMORY), solution->GetGlobalResource(MEMORY)));
			resourceGrant->SetSharedDisk(0);
			resourceGrant->SetSharedMemory(0);
			grantedResource->AddResource(resourceGrant);

			position = solution->GetHostSolutions()->GetStartPosition();
			while (position != NULL)
			{
				solution->GetHostSolutions()->GetNextAssoc(position, sHostName, oElement);
				hostSolution = cast(PLHostSolution*, oElement);
				grantedResource->SetHostCoreNumber(sHostName, hostSolution->GetProcNumber());
				assert(hostSolution->GetProcNumber() == 1);
			}
		}
	}
	else
	{
		// Cas sans solution
		grantedResource->sHostMissingResource = solution->GetHostNameMissingResources();
		grantedResource->rcMissingResources.SetValue(MEMORY, solution->GetMissingResources(MEMORY));
		grantedResource->rcMissingResources.SetValue(DISK, solution->GetMissingResources(DISK));
	}
}

void RMParallelResourceManager::SetRequirement(const RMTaskResourceRequirement* requirements)
{
	taskRequirements = requirements;
}

const RMTaskResourceRequirement* RMParallelResourceManager::GetRequirements() const
{
	return taskRequirements;
}

void RMParallelResourceManager::SetResourceCluster(const RMResourceSystem* cluster)
{
	clusterResources = cluster;
}

const RMResourceSystem* RMParallelResourceManager::GetResourceCluster() const
{
	return clusterResources;
}

const PLClusterSolution* RMParallelResourceManager::Solve()
{
	PLClusterSolution* sequentialSolution;
	boolean bIsSequential;

	require(taskRequirements->Check());

	bIsSequential = false;
	if (clusterResources->GetLogicalProcessNumber() == 1 and not PLParallelTask::GetParallelSimulated())
		bIsSequential = true;

	bestSolution.SetRequirement(taskRequirements);
	if (not bIsSequential or PLParallelTask::GetParallelSimulated())
	{
		GreedySolve();
		PostOptimize();
	}

	// Cas particuliers ou on preferera une solution sequentielle
	if (bestSolution.GetHostSolutionNumber() == 1)
	{
		// Si il y a une seule machine
		if (bestSolution.GetProcessSolutionNumber() == 1 or // et un seul processus
		    (bestSolution.GetProcessSolutionNumber() == 2 and
		     not PLParallelTask::GetParallelSimulated())) // ou 2 processus en dehors du simule
			bIsSequential = true;
	}

	// Si il n'y a pas de solutions valide avec plusieurs machine ou si il n'y a qu'un coeur
	// On cherche une solution sequentielle
	if (bestSolution.GetHostSolutionNumber() == 0 or bIsSequential)
	{
		// Calcul d'une solution sequentielle
		SequentialSolve();
		bIsSequential = true;
	}
	else
	{
		// Si il y a 2 machines et un seul esclave, on preferera une solution sequentielle si elle existe
		// Sinon on gardera la solution avec 2 machines (qui est tres inefficace mais qui a le merite d'exister)
		if (bestSolution.GetHostSolutionNumber() == 2 and bestSolution.GetProcessSolutionNumber() == 2)
		{
			// Copie de la meilleure solution pour la restituer si besoin
			sequentialSolution = bestSolution.Clone();

			// Calcul d'une solution sequentielle
			SequentialSolve();

			// Restitution de la solution initiale si solution sequentielle n'est pas valide
			if (bestSolution.GetProcessSolutionNumber() == 0)
			{
				bestSolution.CopyFrom(sequentialSolution);
			}
			else
			{
				bIsSequential = true;
			}
			delete sequentialSolution;
		}
	}

	// Saturation de la solution pour avoir le max des ressources possible
	bestSolution.SaturateResources(bIsSequential);
	return &bestSolution;
}

void RMParallelResourceManager::Clean()
{
	bestSolution.Clean();
	clusterResources = NULL;
	taskRequirements = NULL;
}

void RMParallelResourceManager::Test()
{
	PLTestCluster test;

	PLParallelTask::SetVerbose(false);
	PLParallelTask::SetSimulatedSlaveNumber(50);
	PLParallelTask::SetParallelSimulated(false);

	// Test sans contraintes
	test.Reset();
	test.SetTestLabel("100 process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 1000 * lGB, 1000 * lGB, 0));
	test.Solve();

	// Test avec la contrainte globale
	test.Reset();
	test.SetTestLabel("100 process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 1000 * lGB, 1000 * lGB, 0));
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(100 * lGB);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 1 * lGB, 1 * lGB, 0));
	test.SetTestLabel("Max proc with 1 Gb");
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 2 Gb");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 2 * lGB, 1 * lGB, 0));
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on host");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 10 * lGB, 1 * lGB, 0));
	test.Solve();

	// Test avec les contraintes utilisateurs
	// On aura le memes resultats que le test precedent
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 100 * lGB, 100 * lGB, 0));
	test.SetDiskLimitPerHost(1 * lGB);
	test.SetMemoryLimitPerHost(10 * lGB);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("2 Gb on host and 200 Mo disk per salve");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 2 * lGB, 1 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->Set(200 * lMB);
	test.Solve();

	// Test de non rgeression, en 32 Bits (2 Gb allouables)
	test.Reset();
	test.SetTestLabel("Non regression 32 bits");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 4, 2 * lGB, 0, 0));
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 1");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 2");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour le sesclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.Solve();

	// Une seule machine programme sequentiel
	test.Reset();
	test.SetTestLabel("mono machines : sequential");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 10 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(4 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMin(1 * lGB);
	test.Solve();

	// Une seule machine, un seul slaveProcess
	test.Reset();
	test.SetTestLabel("mono machines : 1 slave process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1, 10 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(25 * lMB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(16 * lMB);
	test.GetTaskRequirement()->SetMaxSlaveProcessNumber(1);
	test.Solve();

	// Une seule machine, un seul processeur, l'esclave demande plus que ce qu'il y a sur le host
	UIObject::SetUIMode(UIObject::Graphic);
	test.Reset();
	test.SetTestLabel("mono machines : 1 core + Graphical");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1, 2 * lGB, 0, 0));
	test.SetMasterSystemAtStart(8931728);
	test.SetSlaveSystemAtStart(8 * lMB);
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(1717986918); // 1.6 Gb
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(16 * lMB);
	test.Solve();

	// Idem en batch cette fois on peut allouer
	UIObject::SetUIMode(UIObject::Textual);
	test.Reset();
	test.SetTestLabel("mono machines : 1 core + Textual");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1, 2 * lGB, 0, 0));
	test.SetMasterSystemAtStart(8931728);
	test.SetSlaveSystemAtStart(8 * lMB);
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(1717986918); // 1.6 Gb
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(16 * lMB);
	test.Solve();
	UIObject::SetUIMode(UIObject::Graphic);

	// Regression
	test.Reset();
	test.SetTestLabel("regression : Master requirement 300 KB and granted 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 16, 1153433600, 0, 0));
	test.SetMasterSystemAtStart(8539455);
	test.SetSlaveSystemAtStart(7228835);
	test.SetMemoryLimitPerHost(1153433600);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(16474230);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(358692);
	test.Solve();

	// Regression
	test.Reset();
	test.SetTestLabel("regression : slave is low");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 4, 2147483648, 0, 0));
	test.SetMasterSystemAtStart(19417088);
	test.SetSlaveSystemAtStart(5436608);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(3 * lMB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(23 * lMB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(17 * lMB);
	test.Solve();

	// Regression : shared variable
	test.Reset();
	test.SetTestLabel("regression");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1, 2147483648, 0, 0));
	test.SetMasterSystemAtStart(90018512);
	test.SetSlaveSystemAtStart(0);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(8553561);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMin(391500000);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(136259321);
	test.Solve();

	// Contrainte globale
	test.Reset();
	test.SetTestLabel("Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 10 * lGB, 0, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMax(100 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(10 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.Solve();

	// Bug de Felipe : contrainte globale max
	test.Reset();
	test.SetTestLabel("1 machine : global constraint with min and max on slaves");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 2, 2 * lGB, 40 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(30 * lKB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(36 * lKB);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(650 * lMB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMin(1 * lMB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMax(22 * lMB);
	test.Solve();

	test.Reset();
	test.SetMemoryLimitPerHost(longint(56.3 * lGB));
	test.SetTestLabel("1 machine : global constraint with min and max on slaves");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(3, 6, 135089496064, 20122406912, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(510268);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->Set(90374169);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(404331);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->Set(90374169);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(277892739);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMin(36 * lMB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->SetMax(longint(2.9 * lGB));
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetDisk()->SetMin(longint(2.9 * lGB));
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetDisk()->SetMax(longint(2.9 * lGB));
	test.GetTaskRequirement()->SetMaxSlaveProcessNumber(11);
	test.SetMaxCoreNumberOnSystem(21);
	test.SetMasterSystemAtStart(80 * lMB);
	test.SetSlaveSystemAtStart(10546335);
	test.Solve();

	// Bug sur dbdne avec cluster(avec cluster adhoc)
	test.Reset();
	test.SetTestLabel("1 machine : adhoc cluster with global requirement");
	test.SetCluster(RMResourceSystem::CreateAdhocCluster());
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(longint(579.6 * lMB));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(5 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lMB);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(longint(134.5 * lMB));
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetDisk()->Set(longint(28.5 * lGB));
	test.Solve();

	// Contrainte globale avec 10 grosses machines et un petite
	// on doit avoir un esclave sur la petite
	test.Reset();
	test.SetTestLabel("Global with 100 hosts plus 1 mini host");
	RMResourceSystem* cluster = RMResourceSystem::CreateSyntheticCluster(10, 10, 100 * lGB, 100 * lGB, 0);
	RMHostResource* miniHost = new RMHostResource;
	miniHost->AddProcessusRank(100);
	miniHost->SetPhysicalCoresNumber(1);
	miniHost->SetPhysicalMemory(10 * lGB);
	miniHost->SetHostName("mini");
	cluster->AddHostResource(miniHost);
	test.SetCluster(cluster);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(6 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(8 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(4 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	// Test de performance
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint - 100 hosts");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(100, 20, 100 * lGB, 100 * lGB, 0));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.Solve();

	// Test de la politique de parallelisation
	test.Reset();
	test.SetTestLabel("10 machines : 9 slaves max - horizontal");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(5 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(5 * lGB);
	test.GetTaskRequirement()->SetParallelisationPolicy(RMTaskResourceRequirement::horizontal);
	test.GetTaskRequirement()->SetMaxSlaveProcessNumber(9);
	test.Solve();

	test.Reset();
	test.SetTestLabel("10 machines : 9 slaves max - vertical");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(5 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetSharedRequirement()->GetMemory()->Set(5 * lGB);
	test.GetTaskRequirement()->SetParallelisationPolicy(RMTaskResourceRequirement::vertical);
	test.GetTaskRequirement()->SetMaxSlaveProcessNumber(9);
	test.Solve();

	// int nHost = 0;
	// while (nHost < 1000)
	// {
	// 	nHost += 10;
	// 	Timer t;
	// 	t.Start();

	// 	test.Reset();
	// 	//	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint");
	// 	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(nHost, 8, 100 * lGB, 100 * lGB, 0));
	// 	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	// 	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(10 * lGB);
	// 	test.Solve();
	// 	t.Stop();

	// 	cout << nHost << "\t" << 8 << "\t" << t.GetElapsedTime() << endl;
	// }
}

void RMParallelResourceManager::GreedySolve()
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLClusterSolution* currentSolution;
	PLClusterSolution* newSolution;
	boolean bOk;
	int nProcMax;
	const boolean bSequential = false;
	RMResourceContainer rcMissingResource;
	PLHostSolution* hostSolution;
	const RMHostResource* hostResource;
	int nExtraProc;
	int i;
	const boolean bVerbose = false;
	int nRT;
	longint lMasterHiddenResource;
	longint lSlaveHiddenResource;
	longint lLogicalHostResource;
	PLClusterSolution* bestLocalSolution;

	bestLocalSolution = NULL;
	bestSolution.Clean();
	currentSolution = new PLClusterSolution;
	currentSolution->SetRequirement(taskRequirements);

	if (bVerbose)
		cout << "max proc number for : " << endl;
	if (PLParallelTask::GetTracerResources() == 3)
	{
		taskRequirements->WriteDetails(cout);
		cout << endl;
		lMasterHiddenResource = PLHostSolution::GetMasterHiddenResource(bSequential, MEMORY);
		lSlaveHiddenResource = PLHostSolution::GetSlaveHiddenResource(bSequential, MEMORY);
		cout << ResourceToString(MEMORY) << " master hidden resource: \t" << lMasterHiddenResource << "\t"
		     << LongintToHumanReadableString(lMasterHiddenResource) << endl;
		cout << ResourceToString(MEMORY) << " slave  hidden resource: \t" << lSlaveHiddenResource << "\t"
		     << LongintToHumanReadableString(lSlaveHiddenResource) << endl;
		cout << "Host Name\tmax procs\tmemory(B)\tmemory(MB)\tdisk(B)\tdisk(MB)" << endl;
	}
	// Initialisation de la solution avec 0 processeurs par host
	for (i = 0; i < clusterResources->GetHostNumber(); i++)
	{
		hostResource = clusterResources->GetHostResourceAt(i);
		sHostName = hostResource->GetHostName();

		// Creation d'une solution pour cette machine
		hostSolution = new PLHostSolution;
		hostSolution->SetHost(hostResource);
		hostSolution->SetRequirement(taskRequirements);
		hostSolution->SetProcNumber(0);
		hostSolution->SetClusterResources(clusterResources);

		if (bVerbose)
			cout << "\t" << sHostName << " : \t"
			     << IntToString(hostSolution->ComputeMaxProcessNumber(bSequential)) << endl;

		// Ajout dans la solution globale
		currentSolution->AddHostSolution(hostSolution);

		// Traces
		if (PLParallelTask::GetTracerResources() == 3)
		{
			cout << sHostName << "\t\t" << IntToString(hostSolution->ComputeMaxProcessNumber(bSequential))
			     << "\t";
			for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
			{
				lLogicalHostResource = min(hostResource->GetResourceFree(nRT),
							   RMResourceConstraints::GetResourceLimit(nRT) * lMB);
				lLogicalHostResource =
				    RMStandardResourceDriver::PhysicalToLogical(nRT, lLogicalHostResource);
				cout << "\t" << lLogicalHostResource << "\t"
				     << LongintToHumanReadableString(lLogicalHostResource);
			}
			if (hostResource->IsMasterHost())
				cout << "\t"
				     << "MASTER";
			cout << endl;
		}
	}
	if (PLParallelTask::GetTracerResources() == 3)
	{
		cout << endl;
	}

	// Au depart le meilleure solution est la solution nulle
	bestSolution.CopyFrom(currentSolution);

	// Evaluation de la solution pour les comparaisons futures
	bestSolution.GetQuality()->Evaluate();

	// Calcul du nombre de processus max sur tout le cluster
	nProcMax = INT_MAX;
	nProcMax = min(nProcMax, taskRequirements->GetMaxSlaveProcessNumber() + 1);

	if (PLParallelTask::GetParallelSimulated())
		nProcMax = min(nProcMax, PLParallelTask::GetSimulatedSlaveNumber() + 1);
	else
		nProcMax = min(nProcMax, RMResourceConstraints::GetMaxCoreNumberOnCluster());

	assert(nProcMax >= 0);

	// Algorithme glouton : a chaque iteration, on ajoute un processus.
	while (currentSolution->GetProcessSolutionNumber() <= nProcMax)
	{
		if (bVerbose)
		{

			cout << endl << "**current solution" << endl;
			cout << *currentSolution << endl;
		}

		// Pour chaque machine, on cree une nouvelle solution a partir de la solution
		// courante, en ajoutant un processus. On garde la meilleure de ces solutions.
		// Si aucune solution n'est valide, c'est qu'on ne peut plus ajouter de processus
		position = currentSolution->GetHostSolutions()->GetStartPosition();
		newSolution = NULL;
		bestLocalSolution = NULL;
		while (position != NULL)
		{
			currentSolution->GetHostSolutions()->GetNextAssoc(position, sHostName, oElement);
			newSolution = currentSolution->Clone();
			bOk = newSolution->AddProcessorAt(sHostName);
			if (bOk)
			{
				// Evaluation de la solution pour pouvoir la comparer ensuite (dans la liste triee)
				newSolution->GetQuality()->Evaluate();
				if (bestLocalSolution == NULL)
					bestLocalSolution = newSolution->Clone();
				else
				{
					if (newSolution->GetQuality()->Compare(bestLocalSolution->GetQuality()) > 0)
						bestLocalSolution->CopyFrom(newSolution);
				}
			}

			delete newSolution;
			newSolution = NULL;
		}

		// Si il y a au moins une solution valide, on met a jour la solution courante et la meilleure solution
		if (bestLocalSolution != NULL)
		{
			currentSolution->CopyFrom(bestLocalSolution);
			if (currentSolution->FitGlobalConstraint(&rcMissingResource, nExtraProc) and
			    currentSolution->GetQuality()->Compare(bestSolution.GetQuality()) > 0)
			{
				bestSolution.CopyFrom(currentSolution);
			}
			delete bestLocalSolution;
			bestLocalSolution = NULL;
		}
		else
		{
			// Sortie de l'algo si il n'y a plus de solutions valides en ajoutant des processus
			break;
		}
	}
	if (bestLocalSolution != NULL)
		delete bestLocalSolution;
	delete currentSolution;
}

void RMParallelResourceManager::PostOptimize()
{
	// Si aucune solution n'a etet trouvee par l'algorithme glouton
	// il n'y a rien a optimiser
	if (bestSolution.GetProcessSolutionNumber() == 0)
		return;

	// TODO
}

void RMParallelResourceManager::SequentialSolve()
{
	const RMHostResource* masterHost;
	PLHostSolution* sequentialSolution;
	longint lResourceRequired;
	longint lFreeResource;
	longint lMissingResource;
	int nRT;

	// Identification du master host
	masterHost = clusterResources->GetMasterHostResource();

	assert(masterHost != NULL);

	// Netoyage de la solution
	bestSolution.Clean();

	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{

		// Calcul des ressources necessaires pour la tache en sequentiel
		lResourceRequired = lsum(taskRequirements->GetMasterMin(nRT), taskRequirements->GetSharedMin(nRT),
					 taskRequirements->GetSlaveMin(nRT), taskRequirements->GetSlaveGlobalMin(nRT),
					 PLHostSolution::GetMasterHiddenResource(true, nRT));

		// Calcul des ressources disponibles sur le host
		lFreeResource =
		    min(masterHost->GetResourceFree(nRT), RMResourceConstraints::GetResourceLimit(nRT) * lMB);
		lFreeResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lFreeResource);

		lMissingResource = lResourceRequired - lFreeResource;
		if (lMissingResource > 0)
		{
			bestSolution.SetMissingResource(nRT, lMissingResource);
			bestSolution.SetMissingHostName(masterHost->GetHostName());
		}
	}

	// Construction de la solution si il y a assez de ressources
	if (bestSolution.GetMissingResources(MEMORY) == 0 and bestSolution.GetMissingResources(DISK) == 0)
	{
		// Ajout du master host a la solution
		sequentialSolution = new PLHostSolution;
		sequentialSolution->SetHost(masterHost);
		sequentialSolution->SetRequirement(taskRequirements);
		sequentialSolution->SetProcNumber(1);
		sequentialSolution->SetClusterResources(clusterResources);

		bestSolution.AddHostSolution(sequentialSolution);
	}
	else
	{
		bestSolution.SetMissingHostName(masterHost->GetHostName());
	}
}

boolean RMParallelResourceManager::Check(RMTaskResourceGrant* taskResourceGrant) const
{
	int nRT;
	boolean bOK = true;

	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		if (PLParallelTask::GetParallelSimulated() or taskResourceGrant->GetSlaveNumber() > 1)
		{
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) >= taskRequirements->GetMasterMin(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetMinSlaveResource(nRT) >=
					lsum(taskRequirements->GetSlaveMin(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin() /
						 taskResourceGrant->GetSlaveNumber());
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) <= taskRequirements->GetMasterMax(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetMinSlaveResource(nRT) <=
					lsum(taskRequirements->GetSlaveMax(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax() /
						 taskResourceGrant->GetSlaveNumber());
			assert(bOK);
		}
		else
		{
			assert(taskResourceGrant->GetSlaveNumber() == 1);

			// Sequentiel
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) >= taskRequirements->GetMasterMin(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetMinSlaveResource(nRT) >=
					lsum(taskRequirements->GetSlaveMin(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin());
			assert(bOK);
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) <= taskRequirements->GetMasterMax(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetMinSlaveResource(nRT) <=
					lsum(taskRequirements->GetSlaveMax(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax());
			assert(bOK);
		}
	}
	return bOK;
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLHostSolution

PLHostSolution::PLHostSolution()
{
	nProcNumber = 0;
	host = NULL;
	taskRequirements = NULL;
	clusterResources = NULL;
}

PLHostSolution::~PLHostSolution()
{
	host = NULL;
	taskRequirements = NULL;
	clusterResources = NULL;
}

void PLHostSolution::CopyFrom(const PLHostSolution* clusterSolution)
{
	nProcNumber = clusterSolution->nProcNumber;
	host = clusterSolution->host;
	taskRequirements = clusterSolution->taskRequirements;
	clusterResources = clusterSolution->clusterResources;
}

PLHostSolution* PLHostSolution::Clone() const
{
	PLHostSolution* solution;
	solution = new PLHostSolution;
	solution->CopyFrom(this);
	return solution;
}

void PLHostSolution::SetHost(const RMHostResource* h)
{
	host = h;
}
const RMHostResource* PLHostSolution::GetHost() const
{
	return host;
}
void PLHostSolution::SetRequirement(const RMTaskResourceRequirement* requirements)
{
	taskRequirements = requirements;
}

const RMTaskResourceRequirement* PLHostSolution::GetRequirements() const
{
	return taskRequirements;
}

void PLHostSolution::SetClusterResources(const RMResourceSystem* cluster)
{
	clusterResources = cluster;
}

const RMResourceSystem* PLHostSolution::GetClusterResources() const
{
	return clusterResources;
}

void PLHostSolution::Write(ostream& ost) const
{
	assert(host != NULL);
	ost << "Host: " << GetHost()->GetHostName() << " #procs: " << nProcNumber << endl;
}

int PLHostSolution::ComputeMaxProcessNumber(boolean bSequential) const
{
	longint lProc;
	int nMaster;
	longint lMasterHiddenResource;
	longint lSlaveHidenResource;
	longint lRemainingResource;
	longint lLogicalHostResource;
	longint lSlaveMin;
	longint lMasterMin;
	longint lSharedMin;
	int nHostNumber;
	boolean bIsMasterHost;
	int nProcMax;
	int nRT;

	nProcMax = INT_MAX;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		lMasterHiddenResource = GetMasterHiddenResource(bSequential, nRT);
		lSlaveHidenResource = GetSlaveHiddenResource(bSequential, nRT);
		lSlaveMin = taskRequirements->GetSlaveMin(nRT);
		lSharedMin = taskRequirements->GetSharedMin(nRT);
		lMasterMin = taskRequirements->GetMasterMin(nRT);
		nHostNumber = clusterResources->GetHostNumber();
		bIsMasterHost = host->IsMasterHost();

		// Resource disponible sur le host
		lLogicalHostResource =
		    min(host->GetResourceFree(nRT), RMResourceConstraints::GetResourceLimit(nRT) * lMB);
		lLogicalHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lLogicalHostResource);

		if (host->IsMasterHost())
			nMaster = 1;
		else
			nMaster = 0;

		// On test si il y a assez de place pour le master sur le host
		if (host->IsMasterHost())
		{
			if (bSequential)
			{
				// En sequentiel il y a les ressources du maitre et d'un esclave
				lRemainingResource =
				    lLogicalHostResource - lMasterHiddenResource - lSharedMin - lMasterMin - lSlaveMin;
			}
			else
			{
				// En parallele, la machine peut contenir un maitre sans esclaves (cluster)
				if (nHostNumber > 1)
					lRemainingResource =
					    lLogicalHostResource - lMasterHiddenResource - lSharedMin - lMasterMin;
				else
					// ou un maitre et 2 esclaves (mono machine)
					lRemainingResource = lLogicalHostResource - lMasterHiddenResource - lMasterMin -
							     lSharedMin -
							     2 * (lSlaveHidenResource + lSharedMin + lSlaveMin);
			}
			if (lRemainingResource < 0)
			{
				return 0;
			}
		}

		// Calcul du nombre de proc a partir de la contrainte issue de la tache (9)
		if (lSlaveMin + lSlaveHidenResource + lSharedMin != 0) // Test pour eviter la division par 0
		{
			// Si les ressources consommees par un esclave sont plus grandes que celles du systeme
			if (lSlaveMin + lSlaveHidenResource + lSharedMin >=
			    lLogicalHostResource - nMaster * (lMasterMin + lMasterHiddenResource + lSharedMin))
			{
				if (bIsMasterHost and not bSequential and nHostNumber > 1)
				{
					// Si on est sur la machine du maitre, on n'alloue qu'un seul proc : le maitre
					lProc = 1;
				}
				else
				{
					// Si on n'est pas sur la machine du maitre, on ne peut pas allouer d'esclaves
					lProc = 0;
				}
			}
			else
			{
				lProc = (lLogicalHostResource -
					 nMaster * (lMasterMin + lMasterHiddenResource + lSharedMin)) /
					    (lSlaveMin + lSlaveHidenResource + lSharedMin) +
					nMaster;
			}
		}
		else
		{
			// On peut allouer autant d'esclaves que l'on veut
			lProc = INT_MAX;
		}

		// Bornage de la solution
		if (lProc < 0)
			lProc = 0;
		if (lProc > INT_MAX)
			lProc = INT_MAX;

		// Affectation des resultats
		nProcMax = min(nProcMax, (int)lProc);

		debug(; if (lProc > 0 and not bSequential) {
			int nInitialProcNumber = nProcNumber;
			nProcNumber = (int)lProc;
			longint lUsedResourceOnHost = ComputeAllocation(
			    false, nRT, taskRequirements->GetSlaveMin(nRT) + taskRequirements->GetSharedMin(nRT),
			    taskRequirements->GetMasterMin(nRT) + taskRequirements->GetSharedMin(nRT));
			nProcNumber = nInitialProcNumber;
			assert(lUsedResourceOnHost <= lLogicalHostResource);
		});
	}

	if (PLParallelTask::GetParallelSimulated())
		nProcMax = min(nProcMax, PLParallelTask::GetSimulatedSlaveNumber() + 1);
	else
		nProcMax = min(nProcMax, host->GetLogicalProcessNumber());

	return nProcMax;
}

longint PLHostSolution::ComputeAllocation(boolean bIsSequential, int nRT, longint lSlaveAllocatedResource,
					  longint lMasterAllocatedResource) const
{
	longint lResourceUsed;
	longint lSlaveResourceUsed;
	longint lMasterResourceUsed;
	int nMaster;

	require(nRT < RESOURCES_NUMBER);

	if (bIsSequential)
	{
		if (host->IsMasterHost())
		{
			require(nProcNumber == 1);
			lResourceUsed =
			    lMasterAllocatedResource + lSlaveAllocatedResource + GetMasterHiddenResource(true, nRT);
		}
		else
			lResourceUsed = 0;
	}
	else
	{
		if (nProcNumber == 0)
		{
			lSlaveResourceUsed = 0;
			lMasterResourceUsed = 0;
		}
		else
		{
			// Allocation des ressources minimales necessaires au maitre et aux esclaves
			lSlaveResourceUsed = lSlaveAllocatedResource + GetSlaveHiddenResource(bIsSequential, nRT);
			lMasterResourceUsed = lMasterAllocatedResource + GetMasterHiddenResource(bIsSequential, nRT);
		}

		// Memoire utilisee sur le host pour les resources minimales
		if (host->IsMasterHost())
			nMaster = 1;
		else
			nMaster = 0;
		lResourceUsed = lsum(nMaster * lMasterResourceUsed, (nProcNumber - nMaster) * lSlaveResourceUsed);
	}
	assert(lResourceUsed >= 0);
	return lResourceUsed;
}

longint PLHostSolution::GetMasterReserve(int nResourceType)
{
	require(nResourceType < RESOURCES_NUMBER);
	if (nResourceType == MEMORY)
		return RMStandardResourceDriver::PhysicalToLogical(MEMORY, UIObject::GetUserInterfaceMemoryReserve() +
									       MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint PLHostSolution::GetSlaveReserve(int nResourceType)
{
	require(nResourceType < RESOURCES_NUMBER);
	if (nResourceType == MEMORY)
		return RMStandardResourceDriver::PhysicalToLogical(MEMORY, MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint PLHostSolution::GetMasterHiddenResource(boolean bIsSequential, int nRT)
{
	longint lHiddenResource;
	lHiddenResource = RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(nRT)->GetMin() +
			  GetMasterReserve(nRT) /*+ taskRequirements->GetSharedMin(nRT)*/;

	// On ajoute 2 tailles de blocs pour la serialisation
	if (not bIsSequential and nRT == MEMORY)
		lHiddenResource += 2 * MemSegmentByteSize;
	return lHiddenResource;
}

longint PLHostSolution::GetSlaveHiddenResource(boolean bIsSequential, int nRT)
{
	longint lHiddenResource;
	if (bIsSequential)
		lHiddenResource = 0;
	else
	{
		lHiddenResource = RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(nRT)->GetMin() +
				  GetSlaveReserve(nRT) /*+ taskRequirements->GetSharedMin(nRT)*/;

		// On ajoute 2 tailles de blocs pour la serialisation
		if (nRT == MEMORY)
			lHiddenResource += 2 * MemSegmentByteSize;
	}
	return lHiddenResource;
}
///////////////////////////////////////////////////////////////////////
// Implementation de PLClusterSolution

PLClusterSolution::PLClusterSolution()
{
	taskRequirements = NULL;
	quality = new PLClusterResourceQuality;
	quality->SetSolution(this);
	bGlobalConstraintIsEvaluated = false;
	bGlobalConstraintIsValid = false;
	nHostNumber = 0;
	nProcessNumber = 0;
}

PLClusterSolution::~PLClusterSolution()
{
	Clean();
	delete quality;
}

void PLClusterSolution::CopyFrom(const PLClusterSolution* clusterSolution)
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;

	Clean();

	position = clusterSolution->odHostSolution.GetStartPosition();
	while (position != NULL)
	{
		clusterSolution->odHostSolution.GetNextAssoc(position, sHostName, oElement);
		hostSolution = cast(PLHostSolution*, oElement);
		odHostSolution.SetAt(sHostName, hostSolution->Clone());
	}
	taskRequirements = clusterSolution->taskRequirements;
	quality->CopyFrom(clusterSolution->quality);
	quality->SetSolution(this);
	bGlobalConstraintIsEvaluated = clusterSolution->bGlobalConstraintIsEvaluated;
	bGlobalConstraintIsValid = clusterSolution->bGlobalConstraintIsValid;
	nHostNumber = clusterSolution->nHostNumber;
	nProcessNumber = clusterSolution->nProcessNumber;
}

PLClusterSolution* PLClusterSolution::Clone() const
{
	PLClusterSolution* newSolution;
	newSolution = new PLClusterSolution;
	newSolution->CopyFrom(this);
	return newSolution;
}

const ObjectDictionary* PLClusterSolution::GetHostSolutions() const
{
	return &odHostSolution;
}

void PLClusterSolution::AddHostSolution(PLHostSolution* hostSolution)
{
	assert(odHostSolution.Lookup(hostSolution->GetHost()->GetHostName()) == NULL);
	odHostSolution.SetAt(hostSolution->GetHost()->GetHostName(), hostSolution);
	if (hostSolution->GetProcNumber() != 0)
	{
		nHostNumber++;
		nProcessNumber += hostSolution->GetProcNumber();
	}
}

boolean PLClusterSolution::AddProcessorAt(const ALString& sHostName)
{
	PLHostSolution* hostSolution;
	int nProcNumber;
	int nMaxProcNumber;
	boolean bOk;
	boolean bSequential = false;

	// Acces a la solution actuelle
	hostSolution = cast(PLHostSolution*, GetHostSolutions()->Lookup(sHostName));
	nProcNumber = hostSolution->GetProcNumber();

	// Calcul du nombre de process max sur ce host
	nMaxProcNumber = hostSolution->ComputeMaxProcessNumber(bSequential);
	if (nMaxProcNumber >= nProcNumber + 1)
	{
		bOk = true;
		hostSolution->SetProcNumber(nProcNumber + 1);
		nProcessNumber++;
		if (nProcessNumber == 1)
			nHostNumber++;
		bGlobalConstraintIsEvaluated = false;
		GetQuality()->bIsEvaluated = false;
	}
	else
	{
		bOk = false;
	}

	return bOk;
}

boolean PLClusterSolution::RemoveProcessorAt(const ALString& sHostName)
{
	PLHostSolution* hostSolution;
	int nProcNumber;
	boolean bOk;

	// Acces a la solution actuelle
	hostSolution = cast(PLHostSolution*, GetHostSolutions()->Lookup(sHostName));
	nProcNumber = hostSolution->GetProcNumber();

	// On ne peut pas avoir un nombre de processeurs negatif
	if (nProcNumber > 0)
	{
		bOk = true;
		hostSolution->SetProcNumber(nProcNumber - 1);
		nProcessNumber--;
		if (nProcessNumber == 0)
			nHostNumber--;
		bGlobalConstraintIsEvaluated = false;
		GetQuality()->bIsEvaluated = false;
	}
	else
	{
		bOk = false;
	}
	return bOk;
}

boolean PLClusterSolution::SwitchProcessor(const ALString& sHostNameFrom, const ALString& sHostNameTo)
{
	boolean bOk;

	bOk = RemoveProcessorAt(sHostNameFrom);
	if (bOk)
	{
		bOk = AddProcessorAt(sHostNameTo);

		// Si on ne peut pas ajouter de processeur dans le deuxieme,
		// Il faut revenir en arriere pour le premier
		if (not bOk)
		{
			AddProcessorAt(sHostNameFrom);
			bGlobalConstraintIsEvaluated = true;
			GetQuality()->bIsEvaluated = true;
		}
	}
	return bOk;
}

void PLClusterSolution::SetRequirement(const RMTaskResourceRequirement* requirements)
{
	taskRequirements = requirements;
}

const RMTaskResourceRequirement* PLClusterSolution::GetRequirements() const
{
	return taskRequirements;
}

void PLClusterSolution::SaturateResources(boolean bIsSequential)
{
	double dPercentage;
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;
	longint lAvailableResource;
	longint lFreeResource;
	int nRT;
	longint lSharedMin, lSharedMax;
	longint lGlobalMin, lGlobalMax;
	longint lMasterMin, lMasterMax;
	longint lSlaveMin, lSlaveMax;
	longint lMasterResource;
	longint lSlaveResource;
	longint lSharedResource;
	longint lGlobalResource;
	longint lSumMax;
	longint lSumMin;
	int nMasterNumberOnHost;
	int nSlaveNumberOnHost;
	int nSlaveNumberOnCluster;
	boolean bBalanced;
	boolean bMasterPreferred;
	boolean bSlavePreferred;
	boolean bGlobalPreferred;
	const boolean bVerbose = false;

	bBalanced = false;
	bMasterPreferred = false;
	bSlavePreferred = false;
	bGlobalPreferred = false;
	nMasterNumberOnHost = 0;
	nSlaveNumberOnHost = 0;
	nSlaveNumberOnCluster = max(GetProcessSolutionNumber() - 1, 1);
	rcSlaveResource.SetValue(MEMORY, LLONG_MAX);
	rcSharedResource.SetValue(MEMORY, LLONG_MAX);
	rcGlobalResource.SetValue(MEMORY, LLONG_MAX);
	rcSlaveResource.SetValue(DISK, LLONG_MAX);
	rcSharedResource.SetValue(DISK, LLONG_MAX);
	rcGlobalResource.SetValue(DISK, LLONG_MAX);
	position = odHostSolution.GetStartPosition();
	if (bVerbose)
	{
		cout << endl << "*** Saturate resources for " << GetProcessSolutionNumber() << " processes" << endl;
	}

	while (position != NULL)
	{
		odHostSolution.GetNextAssoc(position, sHostName, oElement);
		hostSolution = cast(PLHostSolution*, oElement);

		if (hostSolution->GetHost()->IsMasterHost())
		{
			nMasterNumberOnHost = 1;
			nSlaveNumberOnHost = hostSolution->GetProcNumber() - 1;
		}
		else
		{
			nMasterNumberOnHost = 0;
			nSlaveNumberOnHost = hostSolution->GetProcNumber();
		}
		if (bIsSequential)
		{
			nMasterNumberOnHost = 1;
			nSlaveNumberOnHost = 1;
		}
		if (bVerbose)
		{
			cout << endl;
			cout << "Host " << hostSolution->GetHost()->GetHostName() << endl;
			cout << "Process Number " << hostSolution->GetProcNumber() << endl << endl;
		}
		if (hostSolution->GetProcNumber() != 0)
		{

			for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
			{

				if (bVerbose)
				{
					if (nRT == 0)
						cout << "MEMORY" << endl;
					else
						cout << "DISK" << endl;
				}

				lSharedMin = taskRequirements->GetSharedMin(nRT);
				lGlobalMin = taskRequirements->GetSlaveGlobalMin(nRT);
				lMasterMin = taskRequirements->GetMasterMin(nRT);
				lSlaveMin = taskRequirements->GetSlaveMin(nRT);
				lSharedMax = taskRequirements->GetSharedMax(nRT);
				lGlobalMax = taskRequirements->GetSlaveGlobalMax(nRT);
				lMasterMax = taskRequirements->GetMasterMax(nRT);
				lSlaveMax = taskRequirements->GetSlaveMax(nRT);
				bBalanced = taskRequirements->GetResourceAllocationPolicy(nRT) ==
					    RMTaskResourceRequirement::globalPreferred;
				bMasterPreferred = taskRequirements->GetResourceAllocationPolicy(nRT) ==
						       RMTaskResourceRequirement::masterPreferred and
						   nMasterNumberOnHost > 0;
				bSlavePreferred = taskRequirements->GetResourceAllocationPolicy(nRT) ==
						      RMTaskResourceRequirement::slavePreferred and
						  nSlaveNumberOnHost > 0;
				bGlobalPreferred = taskRequirements->GetResourceAllocationPolicy(nRT) ==
						       RMTaskResourceRequirement::globalPreferred and
						   nSlaveNumberOnHost > 0;
				lMasterResource = 0;
				lSlaveResource = 0;
				lSharedResource = 0;
				lGlobalResource = 0;

				// Calcul des ressources disponibles sur le host
				lFreeResource = min(hostSolution->GetHost()->GetResourceFree(nRT),
						    RMResourceConstraints::GetResourceLimit(nRT) * lMB);
				lFreeResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lFreeResource);

				// On enleve les reserves
				lFreeResource =
				    lFreeResource -
				    nSlaveNumberOnHost * PLHostSolution::GetSlaveHiddenResource(bIsSequential, nRT) -
				    nMasterNumberOnHost * PLHostSolution::GetMasterHiddenResource(bIsSequential, nRT);

				// Pour eviter les problemes d'infini dans les calculs (INF+INF=INF)
				// On borne les max par la memoire disonible
				if (lSharedMax == LLONG_MAX)
					lSharedMax = lFreeResource;
				if (lGlobalMax == LLONG_MAX)
					lGlobalMax = lFreeResource;
				if (lMasterMax == LLONG_MAX)
					lMasterMax = lFreeResource;
				if (lSlaveMax == LLONG_MAX)
					lSlaveMax = lFreeResource;

				// Calcul de la somme ressources min et max
				if (bIsSequential)
				{

					lSumMax = lsum(lSharedMax, lGlobalMax, lMasterMax, lSlaveMax);
					lSumMin = lsum(lSharedMin, lGlobalMin, lMasterMin, lSlaveMin);
				}
				else
				{
					lSumMax =
					    lsum((nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedMax,
						 nSlaveNumberOnHost * lGlobalMax / nSlaveNumberOnCluster,
						 nMasterNumberOnHost * lMasterMax, nSlaveNumberOnHost * lSlaveMax);
					lSumMin =
					    lsum((nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedMin,
						 nSlaveNumberOnHost * lGlobalMin / nSlaveNumberOnCluster,
						 nMasterNumberOnHost * lMasterMin, nSlaveNumberOnHost * lSlaveMin);
				}

				// On borne les ressources disponibles par le max qu'on veut allouer
				lAvailableResource = min(lFreeResource, lSumMax);
				if (bVerbose)
					cout << "Available resource on host "
					     << LongintToHumanReadableString(lFreeResource) << " ("
					     << LongintToHumanReadableString(lAvailableResource) << ") needed" << endl;

				// On sature completement une exigence suivant les preferences
				// Le min et max sont alors mis a 0 pour le calcul du pourcentage
				if (not bBalanced)
				{
					if (bGlobalPreferred and nSlaveNumberOnHost > 0)
					{
						longint lResourceAfterMin = lAvailableResource - lSumMin;
						lGlobalResource =
						    min((lGlobalMin + lResourceAfterMin) / nSlaveNumberOnHost,
							lGlobalMax / nSlaveNumberOnCluster);
						lAvailableResource =
						    lAvailableResource - nSlaveNumberOnHost * lGlobalResource;
						lGlobalMin = 0;
						lGlobalMax = 0;
					}

					if (bMasterPreferred and nMasterNumberOnHost > 0)
					{
						longint lResourceAfterMin = lAvailableResource - lSumMin;
						lMasterResource = min(lMasterMin + lResourceAfterMin, lMasterMax);
						lAvailableResource = lAvailableResource - lMasterResource;
						lMasterMin = 0;
						lMasterMax = 0;
					}
					if (bSlavePreferred and nSlaveNumberOnHost > 0)
					{
						longint lResourceAfterMin = lAvailableResource - lSumMin;
						lSlaveResource =
						    min(lSlaveMin + lResourceAfterMin / nSlaveNumberOnHost, lSlaveMax);
						lAvailableResource =
						    lAvailableResource - nSlaveNumberOnHost * lSlaveResource;
						lSlaveMin = 0;
						lSlaveMax = 0;
					}

					// Re-calcul de la somme ressources min et max pour prendre en comte les mises a
					// 0
					if (bIsSequential)
					{

						lSumMax = lsum(lSharedMax, lGlobalMax, lMasterMax, lSlaveMax);
						lSumMin = lsum(lSharedMin, lGlobalMin, lMasterMin, lSlaveMin);
					}
					else
					{
						lSumMax = lsum((nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedMax,
							       nSlaveNumberOnHost * lGlobalMax / nSlaveNumberOnCluster,
							       nMasterNumberOnHost * lMasterMax,
							       nSlaveNumberOnHost * lSlaveMax);
						lSumMin = lsum((nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedMin,
							       nSlaveNumberOnHost * lGlobalMin / nSlaveNumberOnCluster,
							       nMasterNumberOnHost * lMasterMin,
							       nSlaveNumberOnHost * lSlaveMin);
					}
				}

				assert(lAvailableResource >= 0);
				assert(lAvailableResource >= lSumMin);

				// Calcul du pourcentage de repartition : on cherche a attribuer une saturation
				// proportionnelle aux exigences et donner le meme pourcentage a toutes les exigences
				if (lSumMax == lSumMin)
				{
					// Dans le cas ou les max == les mins, la saturation n'est pas necessaire
					dPercentage = 0;
				}
				else
					dPercentage = (lAvailableResource - lSumMin) * 1.0 / (lSumMax - lSumMin);

				assert(dPercentage <= 1 and dPercentage >= 0);

				if (not bMasterPreferred)
				{
					lMasterResource = 0;
					if (nMasterNumberOnHost == 1)
						lMasterResource =
						    lsum(lMasterMin, longint(dPercentage * (lMasterMax - lMasterMin)));
				}
				if (not bSlavePreferred)
				{
					lSlaveResource = 0;
					if (nSlaveNumberOnHost > 0)
					{
						lSlaveResource =
						    lsum(lSlaveMin, longint(dPercentage * (lSlaveMax - lSlaveMin)));
					}
				}
				if (not bGlobalPreferred)
				{
					lGlobalResource = 0;
					if (nSlaveNumberOnHost > 0)
					{
						lGlobalResource =
						    lsum(lGlobalMin, longint(dPercentage * (lGlobalMax - lGlobalMin))) /
						    nSlaveNumberOnCluster;
					}
				}

				lSharedResource = lsum(lSharedMin, longint(dPercentage * (lSharedMax - lSharedMin)));
				if (bVerbose)
				{
					cout << "Resource allocated on host "
					     << LongintToHumanReadableString(
						    lsum(nMasterNumberOnHost * lMasterResource,
							 nSlaveNumberOnHost * lSlaveResource,
							 (nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedResource,
							 nSlaveNumberOnHost * lGlobalResource))
					     << endl;
					cout << "Sequential\t" << BooleanToString(bIsSequential) << endl;
					cout << "Simulated\t" << BooleanToString(PLParallelTask::GetParallelSimulated())
					     << endl;
					cout << "Percentage allocation\t" << DoubleToString(dPercentage) << endl;
					cout << "For each slave\t" << LongintToHumanReadableString(lSlaveResource)
					     << endl;
					cout << "For each global\t" << LongintToHumanReadableString(lGlobalResource)
					     << endl;
					cout << "For each shared\t" << LongintToHumanReadableString(lSharedResource)
					     << endl;
					cout << "For master\t"
					     << LongintToHumanReadableString(nMasterNumberOnHost * lMasterResource)
					     << endl
					     << endl;
				}

				// Affectation de la ressource au maitre
				if (hostSolution->GetHost()->IsMasterHost())
				{
					assert(lMasterResource >= lMasterMin);
					assert(lMasterResource <= lMasterMax or bMasterPreferred);
					rcMasterResource.SetValue(nRT, lMasterResource);
				}

				// On ne garde que le minimum sur tous les hosts pour avoir les memes ressources allouee
				// a tous les esclaves
				if (nSlaveNumberOnHost > 0)
				{
					assert(lSlaveResource >= lSlaveMin or bSlavePreferred);
					assert(lSlaveResource <= lSlaveMax or bSlavePreferred);

					// On borne par le min a cause de problemes d'arrondi
					lGlobalResource = max(lGlobalResource, lGlobalMin / nSlaveNumberOnCluster);
					assert(lGlobalResource <= lGlobalMax / nSlaveNumberOnCluster or
					       bGlobalPreferred);

					rcSlaveResource.SetValue(nRT,
								 min(rcSlaveResource.GetValue(nRT), lSlaveResource));
					rcGlobalResource.SetValue(nRT,
								  min(rcGlobalResource.GetValue(nRT), lGlobalResource));
				}
				rcSharedResource.SetValue(nRT, min(rcSharedResource.GetValue(nRT), lSharedResource));
			}
		}
	}
}

longint PLClusterSolution::GetResourceUsed(int nRT) const
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;
	longint lMemory;
	boolean bIsSequential;

	bIsSequential = false;
	if (GetProcessSolutionNumber() == 1)
		bIsSequential = true;

	// Parcours des tous les hosts pour trouver les solutions qui font intervenir au moins un processeur
	lMemory = 0;
	position = odHostSolution.GetStartPosition();
	while (position != NULL)
	{
		odHostSolution.GetNextAssoc(position, sHostName, oElement);
		hostSolution = cast(PLHostSolution*, oElement);
		lMemory += hostSolution->ComputeAllocation(
		    bIsSequential, nRT,
		    rcSlaveResource.GetValue(nRT) + rcSharedResource.GetValue(nRT) + rcGlobalResource.GetValue(nRT),
		    rcMasterResource.GetValue(nRT) + rcSharedResource.GetValue(nRT));
	}
	return lMemory;
}
void PLClusterSolution::SetMissingResource(int nRT, longint lMissingResource)
{
	rcMissingResource.SetValue(nRT, lMissingResource);
}

longint PLClusterSolution::GetMissingResources(int nRT) const
{
	return rcMissingResource.GetValue(nRT);
}

void PLClusterSolution::SetMissingHostName(const ALString& sHostName)
{
	sHostMissingResource = sHostName;
}

ALString PLClusterSolution::GetHostNameMissingResources() const
{
	return sHostMissingResource;
}

void PLClusterSolution::Clean()
{
	odHostSolution.DeleteAll();
	quality->Clean();
	nProcessNumber = 0;
	nHostNumber = 0;
}

boolean PLClusterSolution::FitMinimalRequirements(int nRT, longint& lMissingResource) const
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;
	longint lHostResource;
	boolean bResourceIsMissing;
	longint lSharedMin;
	longint lGlobalMin;
	longint lMasterMin;
	longint lSlaveMin;
	longint lSumMin;
	longint lMasterHiddenResource;
	longint lSlaveHidenResource;
	int nSlaveNumberOnHost;
	int nMasterNumberOnHost;
	int nSlaveNumberOnCluster;

	lSharedMin = taskRequirements->GetSharedMin(nRT);
	lGlobalMin = taskRequirements->GetSlaveGlobalMin(nRT);
	lMasterMin = taskRequirements->GetMasterMin(nRT);
	lSlaveMin = taskRequirements->GetSlaveMin(nRT);
	lMasterHiddenResource = PLHostSolution::GetMasterHiddenResource(false, nRT);
	lSlaveHidenResource = PLHostSolution::GetSlaveHiddenResource(false, nRT);
	nSlaveNumberOnCluster = this->GetProcessSolutionNumber();

	if (nSlaveNumberOnCluster > 1)
		nSlaveNumberOnCluster--;

	// TODO lMissingResource contient les ressources manquantes de la premiere machine
	// est-ce qu'on souhaite avoir la somme des ressources manquantes ou la liste des
	// ressources manquantes (peut etre le nom de la machine)
	lMissingResource = 0;
	bResourceIsMissing = false;

	position = odHostSolution.GetStartPosition();
	while (position != NULL and not bResourceIsMissing)
	{
		odHostSolution.GetNextAssoc(position, sHostName, oElement);
		hostSolution = cast(PLHostSolution*, oElement);

		// Nombre d'esclave
		nSlaveNumberOnHost = hostSolution->GetProcNumber();
		if (hostSolution->GetHost()->IsMasterHost())
		{
			nSlaveNumberOnHost--;
			nMasterNumberOnHost = 1;
		}
		else
			nMasterNumberOnHost = 0;

		// Resources necessaires sur ce host
		lSumMin = lsum((nSlaveNumberOnHost + nMasterNumberOnHost) * lSharedMin,
			       nSlaveNumberOnHost * lGlobalMin / nSlaveNumberOnCluster,
			       nMasterNumberOnHost * (lMasterMin + lMasterHiddenResource),
			       nSlaveNumberOnHost * (lSlaveMin + lSlaveHidenResource));

		// Ressources disponibles sur le host
		lHostResource = min(hostSolution->GetHost()->GetResourceFree(nRT),
				    RMResourceConstraints::GetResourceLimit(nRT) * lMB);
		lHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lHostResource);

		lMissingResource = lSumMin - lHostResource;
		if (lMissingResource > 0)
			bResourceIsMissing = true;
	}

	// On ne renvoie que les ressources manquantes (pas les ressources disponibles)
	if (not bResourceIsMissing)
		lMissingResource = 0;
	return not bResourceIsMissing;
}

boolean PLClusterSolution::FitGlobalConstraint(RMResourceContainer* rcMissingResourceForGlobalConstraint,
					       int& nExtraProcNumber) const
{
	longint lMissingResource;
	int nRT;
	int nExtraProcessNumber1;
	int nExtraProcessNumber2;
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;

	if (not bGlobalConstraintIsEvaluated)
	{
		bGlobalConstraintIsEvaluated = true;
		bGlobalConstraintIsValid = true;

		// Verification qu'il y a au moins un processus sur la machine maitre
		position = odHostSolution.GetStartPosition();
		while (position != NULL)
		{
			odHostSolution.GetNextAssoc(position, sHostName, oElement);
			hostSolution = cast(PLHostSolution*, oElement);
			if (hostSolution->GetHost()->IsMasterHost())
			{
				if (hostSolution->GetProcNumber() == 0)
					bGlobalConstraintIsValid = false;
				break;
			}
		}

		// Verification des contraintes sur le nombre de processus
		if (bGlobalConstraintIsValid)
		{
			if (PLParallelTask::GetParallelSimulated())
				nExtraProcessNumber1 =
				    GetProcessSolutionNumber() - PLParallelTask::GetSimulatedSlaveNumber() - 1;
			else
				nExtraProcessNumber1 =
				    GetProcessSolutionNumber() - RMResourceConstraints::GetMaxCoreNumberOnCluster();
			nExtraProcessNumber2 =
			    GetProcessSolutionNumber() - 1 - taskRequirements->GetMaxSlaveProcessNumber();
			nExtraProcNumber = max(nExtraProcessNumber1, nExtraProcessNumber2);

			if (nExtraProcNumber > 0)
				bGlobalConstraintIsValid = false;
			else
				nExtraProcNumber = 0;
		}

		// Verification de l'exigence GetGlobalSlaveRequirement
		if (bGlobalConstraintIsValid)
		{
			for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
			{

				// Verification, qu'il y a assez de ressource sur chaque host
				bGlobalConstraintIsValid = FitMinimalRequirements(nRT, lMissingResource);

				// de plus la methode CheckAvailableResourceForMaster n'est jamais utilisee
				rcMissingResourceForGlobalConstraint->AddValue(nRT, lMissingResource);
				if (not bGlobalConstraintIsValid)
					break;
			}

			assert(bGlobalConstraintIsValid or rcMissingResourceForGlobalConstraint->GetValue(MEMORY) > 0 or
			       rcMissingResourceForGlobalConstraint->GetValue(DISK) > 0);
		}
	}
	return bGlobalConstraintIsValid;
}

void PLClusterSolution::Write(ostream& ost) const
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	PLHostSolution* hostSolution;
	longint lMemoryAllocated;
	longint lDiskAllocated;
	boolean bIsSequential;
	int nExtraProcForWrite;
	RMResourceContainer rcMissingResourceForWrite;

	bIsSequential = GetProcessSolutionNumber() == 1;

	ost << "#procs:\t" << IntToString(GetProcessSolutionNumber()) << endl;

	if (FitGlobalConstraint(&rcMissingResourceForWrite, nExtraProcForWrite))
	{
		if (odHostSolution.GetCount() > 0)
		{
			ost << "master:\t" << rcMasterResource << endl;
			ost << "slave:\t" << rcSlaveResource << endl;
			ost << "shared:\t" << rcSharedResource << endl;
			ost << "global:\t" << rcGlobalResource << endl;
			ost << endl;

			// Affichage de l'entete
			ost << "hostname"
			    << "\t"
			    << "procs"
			    << "\t"
			    << "memory"
			    << "\t"
			    << "disk" << endl;

			// Parcours des tous les hosts
			position = odHostSolution.GetStartPosition();
			while (position != NULL)
			{
				odHostSolution.GetNextAssoc(position, sHostName, oElement);
				hostSolution = cast(PLHostSolution*, oElement);
				lMemoryAllocated = 0;
				lDiskAllocated = 0;

				if (hostSolution->GetProcNumber() != 0)
				{
					lMemoryAllocated = hostSolution->ComputeAllocation(
					    bIsSequential, MEMORY,
					    rcSlaveResource.GetValue(MEMORY) + rcSharedResource.GetValue(MEMORY) +
						rcGlobalResource.GetValue(MEMORY),
					    rcMasterResource.GetValue(MEMORY) + rcSharedResource.GetValue(MEMORY));
					lDiskAllocated = hostSolution->ComputeAllocation(
					    bIsSequential, DISK,
					    rcSlaveResource.GetValue(DISK) + rcSharedResource.GetValue(DISK) +
						rcGlobalResource.GetValue(DISK),
					    rcMasterResource.GetValue(DISK) + rcSharedResource.GetValue(DISK));
				}
				ost << sHostName << "\t" << hostSolution->GetProcNumber() << "\t"
				    << LongintToHumanReadableString(lMemoryAllocated) << "\t"
				    << LongintToHumanReadableString(lDiskAllocated) << endl;
			}
		}
		else
		{
			ost << "Missing resources on " << sHostMissingResource << "\t" << rcMissingResourceForWrite
			    << endl;
		}
	}
	else
	{
		ost << "not valid" << endl;
	}
}

int CompareClusterSolution(const void* elem1, const void* elem2)
{
	PLClusterSolution* solution1;
	PLClusterSolution* solution2;
	require(elem1 != NULL and elem2 != NULL);
	solution1 = cast(PLClusterSolution*, *(Object**)elem1);
	solution2 = cast(PLClusterSolution*, *(Object**)elem2);
	solution1->quality->Evaluate();
	solution1->quality->Evaluate();
	return solution1->quality->Compare(solution2->quality);
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLClusterResourceQuality
PLClusterResourceQuality::PLClusterResourceQuality()
{
	Clean();
}

PLClusterResourceQuality ::~PLClusterResourceQuality() {}
void PLClusterResourceQuality::CopyFrom(const PLClusterResourceQuality* quality)
{
	nProcNumber = quality->nProcNumber;
	nSpread = quality->nSpread;
	rcMissingResource.CopyFrom(&quality->rcMissingResource);
	nExtraProcessNumber = quality->nExtraProcessNumber;
	bIsEvaluated = quality->bIsEvaluated;
	bGlobalConstraintIsValid = quality->bGlobalConstraintIsValid;
	solution = quality->solution;
}

PLClusterResourceQuality* PLClusterResourceQuality::Clone() const
{
	PLClusterResourceQuality* otherQuality;
	otherQuality = new PLClusterResourceQuality;
	otherQuality->CopyFrom(this);
	return otherQuality;
}

void PLClusterResourceQuality::Evaluate()
{
	if (not bIsEvaluated)
	{
		bIsEvaluated = true;
		bGlobalConstraintIsValid = solution->FitGlobalConstraint(&rcMissingResource, nExtraProcessNumber);
		if (bGlobalConstraintIsValid)
		{
			nProcNumber = solution->GetProcessSolutionNumber();
			nSpread = solution->GetHostSolutionNumber();
			solution->SaturateResources(false);
		}
	}
}

void PLClusterResourceQuality::SetSolution(PLClusterSolution* s)
{
	solution = s;
}
PLClusterSolution* PLClusterResourceQuality::GetSolution() const
{
	return solution;
}

void PLClusterResourceQuality::Write(ostream& ost) const
{
	if (bIsEvaluated)
	{
		ost << BooleanToString(bGlobalConstraintIsValid) << "\t" << IntToString(nProcNumber) << "\t"
		    << IntToString(nSpread) << "\t" << GetSolution()->GetResourceUsed(MEMORY) << "\t"
		    << GetSolution()->GetResourceUsed(DISK) << "\t" << rcMissingResource << "\t"
		    << IntToString(nExtraProcessNumber) << "\t" << endl;
	}
	else
		ost << "NOT EVALUATED" << endl;
}

int PLClusterResourceQuality::Compare(const PLClusterResourceQuality* otherResourceQuality)
{
	int nCompare;
	require(bIsEvaluated);
	require(otherResourceQuality->bIsEvaluated);

	if (bGlobalConstraintIsValid)
	{
		if (otherResourceQuality->bGlobalConstraintIsValid)
		{
			// Nombre de processus utilises
			nCompare = nProcNumber - otherResourceQuality->nProcNumber;

			// Etalement : on privilegie la paralellisation horizontale
			if (nCompare == 0)
			{
				nCompare = nSpread - otherResourceQuality->nSpread;

				// Sauf si la politique de paralelisation est vertical
				if (solution->GetRequirements()->GetParallelisationPolicy() ==
				    RMTaskResourceRequirement::vertical)
				{
					nCompare = -nCompare;
				}
			}

			// La memoire (arrondi au lUsedResourcePrecision)
			if (nCompare == 0)
				nCompare =
				    CompareLongint(lUsedResourcePrecision * (GetSolution()->GetResourceUsed(MEMORY) /
									     lUsedResourcePrecision),
						   lUsedResourcePrecision *
						       (otherResourceQuality->GetSolution()->GetResourceUsed(MEMORY) /
							lUsedResourcePrecision));

			// le disque (arrondi au lUsedResourcePrecision)
			if (nCompare == 0)
				nCompare =
				    CompareLongint(lUsedResourcePrecision *
						       (GetSolution()->GetResourceUsed(DISK) / lUsedResourcePrecision),
						   lUsedResourcePrecision *
						       (otherResourceQuality->GetSolution()->GetResourceUsed(DISK) /
							lUsedResourcePrecision));
		}
		else
		{
			nCompare = 1;
		}
	}
	else
	{
		if (otherResourceQuality->bGlobalConstraintIsValid)
			nCompare = -1;
		else
		{
			// Nombre de processus en trop
			nCompare = otherResourceQuality->nExtraProcessNumber - nExtraProcessNumber;

			// Memoire manquante
			if (nCompare == 0)
				nCompare = CompareLongint(otherResourceQuality->rcMissingResource.GetValue(MEMORY),
							  rcMissingResource.GetValue(MEMORY));

			// Disque manquant
			if (nCompare == 0)
				nCompare = CompareLongint(otherResourceQuality->rcMissingResource.GetValue(DISK),
							  rcMissingResource.GetValue(DISK));
		}
	}
	return nCompare;
}

void PLClusterResourceQuality::Clean()
{
	bGlobalConstraintIsValid = false;
	nProcNumber = 0;
	nSpread = 0;
	nExtraProcessNumber = 0;
	bIsEvaluated = false;
}

///////////////////////////////////////////////////////////////////////
// Implementation de  RMResourceContainer

RMResourceContainer::RMResourceContainer()
{
	lvResources.SetSize(RESOURCES_NUMBER);
}
RMResourceContainer::~RMResourceContainer() {}

void RMResourceContainer::CopyFrom(const RMResourceContainer* container)
{
	lvResources.CopyFrom(&container->lvResources);
}

RMResourceContainer* RMResourceContainer::Clone() const
{
	RMResourceContainer* clone;
	clone = new RMResourceContainer;
	clone->CopyFrom(this);
	return clone;
}

longint RMResourceContainer::GetValue(int nResourceType) const
{
	require(nResourceType < RESOURCES_NUMBER);
	return lvResources.GetAt(nResourceType);
}
void RMResourceContainer::SetValue(int nResourceType, longint lValue)
{
	require(nResourceType < RESOURCES_NUMBER);
	lvResources.SetAt(nResourceType, lValue);
}

void RMResourceContainer::AddValue(int nResourceType, longint lValue)
{
	longint lValue1;
	require(nResourceType < RESOURCES_NUMBER);
	lValue1 = lvResources.GetAt(nResourceType);
	if (lValue1 > LLONG_MAX - lValue)
		lvResources.SetAt(nResourceType, LLONG_MAX);
	else
		lvResources.SetAt(nResourceType, lValue1 + lValue);
}
void RMResourceContainer::Initialize()
{
	lvResources.Initialize();
}

void RMResourceContainer::Write(ostream& ost) const
{
	int nRT;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		if (GetValue(nRT) == LLONG_MAX)
			ost << "MAX";
		else
			ost << LongintToHumanReadableString(GetValue(nRT));
		if (nRT != RESOURCES_NUMBER - 1)
			ost << "\t";
	}
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLTestCluster

// Constructeur
PLTestCluster::PLTestCluster()
{
	cluster = NULL;
	Reset();
}

PLTestCluster::~PLTestCluster()
{
	if (cluster != NULL)
	{
		delete cluster;
		cluster = NULL;
	}
}

void PLTestCluster::SetCluster(RMResourceSystem* clusterValue)
{
	cluster = clusterValue;
}

RMResourceSystem* PLTestCluster::GetCluster() const
{
	return cluster;
}

void PLTestCluster::SetTestLabel(const ALString& sValue)
{
	sName = sValue;
}

void PLTestCluster::SetMasterSystemAtStart(longint lMemory)
{
	require(lMemory >= 0);
	lMasterSystemAtStart = lMemory;
}

void PLTestCluster::SetSlaveSystemAtStart(longint lMemory)
{
	require(lMemory >= 0);
	lSlaveSystemAtStart = lMemory;
}

void PLTestCluster::Reset()
{
	if (cluster != NULL)
	{
		delete cluster;
		cluster = NULL;
	}

	nMaxCoreNumberPerHost = -1;
	nMaxCoreNumberOnSystem = -1;
	lMemoryLimitPerHost = LLONG_MAX;
	lDiskLimitPerHost = LLONG_MAX;
	lMasterSystemAtStart = 8 * lMB;
	lSlaveSystemAtStart = 8 * lMB;

	RMTaskResourceRequirement emptyRequirements;
	taskRequirement.CopyFrom(&emptyRequirements);

	sName = "Test Cluster";
}

RMTaskResourceRequirement* PLTestCluster::GetTaskRequirement()
{
	return &taskRequirement;
}

void PLTestCluster::SetMemoryLimitPerHost(longint lMemory)
{
	require(lMemory >= 0);
	lMemoryLimitPerHost = lMemory;
}

void PLTestCluster::SetDiskLimitPerHost(longint lDisk)
{
	require(lDisk >= 0);
	lDiskLimitPerHost = lDisk;
}

void PLTestCluster::SetMaxCoreNumberPerHost(int nMax)
{
	require(nMax > 0);
	nMaxCoreNumberPerHost = nMax;
}

void PLTestCluster::SetMaxCoreNumberOnSystem(int nMax)
{
	require(nMax > 0);
	nMaxCoreNumberOnSystem = nMax;
}

void PLTestCluster::Solve()
{
	RMTaskResourceGrant* resources;
	const int nCurrentMemoryLimit = RMResourceConstraints::GetMemoryLimit();
	const int nCurrentDiskLimit = RMResourceConstraints::GetDiskLimit();
	const int nCurrentCoreLimit = RMResourceConstraints::GetMaxCoreNumberOnCluster();
	const int nCurrentCoreLimitOnHost = RMResourceConstraints::GetMaxCoreNumberPerHost();
	const longint lCurrentSlaveAtStart =
	    RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->GetMin();
	const longint lCurrentMasterAtStart =
	    RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->GetMin();
	const boolean bCurrentTracer = PLParallelTask::GetTracerResources();
	int nRT;

	require(lMemoryLimitPerHost != -1);
	require(lDiskLimitPerHost != -1);
	require(cluster != NULL);

	resources = new RMTaskResourceGrant;

	// Affichage des traces de l'allocation des ressources
	PLParallelTask::SetTracerResources(2);

	// Modification des ressources au lancement
	RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->Set(lSlaveSystemAtStart);
	RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->Set(lMasterSystemAtStart);

	// Affichae d'une entete du test
	cout << endl << "----------------------------------" << endl;
	cout << "\t\t" << sName << endl;
	cout << endl;

	// Contraintes de l'utilisateur
	// Memoire par machine
	if (lMemoryLimitPerHost == LLONG_MAX)
		RMResourceConstraints::SetMemoryLimit(INT_MAX);
	else
		RMResourceConstraints::SetMemoryLimit((int)(lMemoryLimitPerHost / lMB));

	// Disque par machine
	if (lDiskLimitPerHost == LLONG_MAX)
		RMResourceConstraints::SetDiskLimit(INT_MAX);
	else
		RMResourceConstraints::SetDiskLimit((int)(lDiskLimitPerHost / lMB));

	// Nombre de coeurs par machine
	if (nMaxCoreNumberPerHost == -1)
		RMResourceConstraints::SetMaxCoreNumberPerHost(INT_MAX);
	else
		RMResourceConstraints::SetMaxCoreNumberPerHost(nMaxCoreNumberPerHost);

	// Nombre de coeurs sur le systeme
	if (nMaxCoreNumberOnSystem == -1)
		RMResourceConstraints::SetMaxCoreNumberOnCluster(INT_MAX);
	else
		RMResourceConstraints::SetMaxCoreNumberOnCluster(nMaxCoreNumberOnSystem);

	// Resolution
	RMParallelResourceManager::ComputeGrantedResourcesForCluster(cluster, &taskRequirement, resources);

	// Affichage des ressources manquantes
	if (resources->IsEmpty())
		cout << resources->GetMissingResourceMessage() << endl;

	// On verifie que les esclaves sont dans les bornes
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		// cout << endl;
		// cout << "resources->GetMinSlaveResource(nRT) " <<
		// LongintToHumanReadableString(resources->GetMinSlaveResource(nRT)) << endl; cout <<
		// "taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMin() " <<
		// LongintToHumanReadableString(taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMin()) <<
		// endl; cout << "taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin() " <<
		// LongintToHumanReadableString(taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin())
		// << endl; cout << "resources->GetSlaveNumber() " << resources->GetSlaveNumber() << endl; cout <<
		// "taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin() /
		// resources->GetSlaveNumber() " <<
		// LongintToHumanReadableString(taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin()
		// / resources->GetSlaveNumber()) << endl;
		assert(resources->IsEmpty() or resources->GetSlaveNumber() == 0 or
		       resources->GetMinSlaveResource(nRT) >=
			   lsum(taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMin(),
				taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin() /
				    resources->GetSlaveNumber()));
		// cout << endl;
		// cout << "resources->GetMinSlaveResource(nRT) " <<
		// LongintToHumanReadableString(resources->GetMinSlaveResource(nRT)) << endl; cout <<
		// "taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMax() " <<
		// LongintToHumanReadableString(taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMax()) <<
		// endl; cout << "taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax() " <<
		// LongintToHumanReadableString(taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax())
		// << endl; cout << "resources->GetSlaveNumber() " << resources->GetSlaveNumber() << endl;
		assert(resources->IsEmpty() or resources->GetSlaveNumber() == 0 or
		       resources->GetMinSlaveResource(nRT) <=
			   lsum(taskRequirement.GetSlaveRequirement()->GetResource(nRT)->GetMax(),
				taskRequirement.GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax() /
				    resources->GetSlaveNumber()));
	}

	// Nettoyage
	delete resources;

	// Restitution des contraintes du systeme courant
	RMResourceConstraints::SetMemoryLimit(nCurrentMemoryLimit);
	RMResourceConstraints::SetDiskLimit(nCurrentDiskLimit);
	RMResourceConstraints::SetMaxCoreNumberOnCluster(nCurrentCoreLimit);
	RMResourceConstraints::SetMaxCoreNumberPerHost(nCurrentCoreLimitOnHost);
	RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->Set(lCurrentSlaveAtStart);
	RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->Set(lCurrentMasterAtStart);
	PLParallelTask::SetTracerResources(bCurrentTracer);
}

///////////////////////////////////////////////////////////////////////
// Implementation de RMTaskResourceRequirement

RMResourceRequirement RMTaskResourceRequirement::masterSystemAtStart;
RMResourceRequirement RMTaskResourceRequirement::slaveSystemAtStart;
longint RMTaskResourceRequirement::lMinProcessDisk = 0;
longint RMTaskResourceRequirement::lMinProcessMem = 8 * lMB;

RMTaskResourceRequirement::RMTaskResourceRequirement()
{
	int nResourceType;

	masterRequirement = new RMResourceRequirement;
	slaveRequirement = new RMResourceRequirement;
	globalSlaveRequirement = new RMResourceRequirement;
	sharedRequirement = new RMResourceRequirement;
	nSlaveProcessNumber =
	    INT_MAX - 1; // on enleve 1 pour que  nSlaveProcessNumber +1 = INT_MAX (= nb de proc sur le systeme)

	// On initialise le max des ressources globales et shared a 0 (par defaut a INT_MAX)
	// On considere que c'est des ressources optionelles : on ne veut pas les prendre en compte par defaut
	globalSlaveRequirement->GetDisk()->Set(0);
	globalSlaveRequirement->GetMemory()->Set(0);
	sharedRequirement->GetDisk()->Set(0);
	sharedRequirement->GetMemory()->Set(0);

	// Politiques d'allocation
	ivResourcesPolicy.SetSize(RESOURCES_NUMBER);
	for (nResourceType = 0; nResourceType < RESOURCES_NUMBER; nResourceType++)
	{
		ivResourcesPolicy.SetAt(nResourceType, slavePreferred);
	}

	// Politiques de parallelisation
	parallelisationPolicy = horizontal;

	// Initialisation des ressources au demarrage avec des valeurs par defaut si necessaire
	// Ca devrait etre la taille de la heap lors de l'initialisation des ressources
	if (slaveSystemAtStart.GetMemory()->GetMin() == 0)
		slaveSystemAtStart.GetMemory()->Set(lMinProcessMem);
	if (slaveSystemAtStart.GetDisk()->GetMin() == 0)
		slaveSystemAtStart.GetDisk()->Set(lMinProcessDisk);
	if (masterSystemAtStart.GetMemory()->GetMin() == 0)
		masterSystemAtStart.GetMemory()->Set(lMinProcessMem);
	if (masterSystemAtStart.GetDisk()->GetMin() == 0)
		masterSystemAtStart.GetDisk()->Set(lMinProcessDisk);
}

RMTaskResourceRequirement::~RMTaskResourceRequirement()
{
	delete masterRequirement;
	delete slaveRequirement;
	delete sharedRequirement;
	delete globalSlaveRequirement;
}

RMTaskResourceRequirement* RMTaskResourceRequirement::Clone() const
{
	RMTaskResourceRequirement* clone;
	clone = new RMTaskResourceRequirement;
	clone->CopyFrom(this);
	return clone;
}

void RMTaskResourceRequirement::CopyFrom(const RMTaskResourceRequirement* trRequirement)
{
	require(trRequirement->Check());

	// Recopie des exigences
	masterRequirement->CopyFrom(trRequirement->GetMasterRequirement());
	sharedRequirement->CopyFrom(trRequirement->GetSharedRequirement());
	slaveRequirement->CopyFrom(trRequirement->GetSlaveRequirement());
	globalSlaveRequirement->CopyFrom(trRequirement->GetGlobalSlaveRequirement());
	nSlaveProcessNumber = trRequirement->GetMaxSlaveProcessNumber();

	// Recopie des politiques
	ivResourcesPolicy.CopyFrom(&trRequirement->ivResourcesPolicy);
	parallelisationPolicy = trRequirement->parallelisationPolicy;
}

void RMTaskResourceRequirement::Write(ostream& ost) const
{
	ost << "--   Task requirements    --" << endl;
	ost << "Slave requirement: " << endl << *slaveRequirement;
	ost << "Master requirement: " << endl << *masterRequirement;
	ost << "Shared variables requirement: " << endl << *sharedRequirement;
	ost << "Slave global requirement: " << endl << *globalSlaveRequirement;
	ost << "Slave system at start: " << endl << slaveSystemAtStart;
	ost << "Master system at start: " << endl << masterSystemAtStart;
	ost << "Number of slaves processes: ";
	if (nSlaveProcessNumber >= INT_MAX - 1)
		ost << "INF" << endl;
	else
		ost << IntToString(nSlaveProcessNumber) << endl;
	ost << "memory policy: " << ResourcePolicyToString(ivResourcesPolicy.GetAt(0)) << endl;
	ost << "disk   policy: " << ResourcePolicyToString(ivResourcesPolicy.GetAt(1)) << endl;
	ost << "paral. policy: " << ParallelisationPolicyToString(parallelisationPolicy) << endl;
}

void RMTaskResourceRequirement::WriteDetails(ostream& ost) const
{
	ost << "--   Task requirements (B)    --" << endl;

	ost << "Slave requirement: " << endl;
	slaveRequirement->WriteDetails(ost);
	ost << "Master requirement: " << endl;
	masterRequirement->WriteDetails(ost);
	ost << "Shared variables requirement: " << endl;
	sharedRequirement->WriteDetails(ost);
	ost << "Slave global requirement: " << endl;
	globalSlaveRequirement->WriteDetails(ost);
	ost << "Slave system at start: " << endl;
	slaveSystemAtStart.WriteDetails(ost);
	ost << "Master system at start: " << endl;
	masterSystemAtStart.WriteDetails(ost);
}

ALString RMTaskResourceRequirement::ResourcePolicyToString(int policy)
{
	switch (policy)
	{
	case masterPreferred:
		return "master first";
	case slavePreferred:
		return "slave first";
	case globalPreferred:
		return "global first";
	case balanced:
		return "balanced";
	default:
		assert(false);
		return "Undefined";
	}
}

ALString RMTaskResourceRequirement::ParallelisationPolicyToString(int policy)
{
	switch (policy)
	{
	case horizontal:
		return "horizontal";
	case vertical:
		return "vertical  ";
	default:
		assert(false);
		return "Undefined";
	}
}

boolean RMTaskResourceRequirement::Check() const
{
	boolean bOk;

	bOk = true;
	if (not masterRequirement->Check())
	{
		AddError("Master requirements are not consitent");
		bOk = false;
	}
	if (not slaveRequirement->Check())
	{
		AddError("Slave requirements are not consitent");
		bOk = false;
	}
	if (not sharedRequirement->Check())
	{
		AddError("Shared requirements are not consitent");
		bOk = false;
	}
	if (not globalSlaveRequirement->Check())
	{
		AddError("Global slave requirements are not consitent");
		bOk = false;
	}
	return bOk;
}

void RMTaskResourceRequirement::SetResourceAllocationPolicy(int nResourceType, ALLOCATION_POLICY policy)
{
	require(nResourceType < RESOURCES_NUMBER);
	ivResourcesPolicy.SetAt(nResourceType, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY
RMTaskResourceRequirement::GetResourceAllocationPolicy(int nResourceType) const
{
	require(nResourceType < RESOURCES_NUMBER);
	return (ALLOCATION_POLICY)ivResourcesPolicy.GetAt(nResourceType);
}

void RMTaskResourceRequirement::SetMemoryAllocationPolicy(ALLOCATION_POLICY policy)
{
	SetResourceAllocationPolicy(MEMORY, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY RMTaskResourceRequirement::GetMemoryAllocationPolicy() const
{
	return GetResourceAllocationPolicy(MEMORY);
}

void RMTaskResourceRequirement::SetDiskAllocationPolicy(ALLOCATION_POLICY policy)
{
	SetResourceAllocationPolicy(DISK, policy);
}

RMTaskResourceRequirement::ALLOCATION_POLICY RMTaskResourceRequirement::GetDiskAllocationPolicy() const
{
	return GetResourceAllocationPolicy(DISK);
}

void RMTaskResourceRequirement::SetParallelisationPolicy(PARALLELISATION_POLICY policy)
{
	parallelisationPolicy = policy;
}

RMTaskResourceRequirement::PARALLELISATION_POLICY RMTaskResourceRequirement::GetParallelisationPolicy() const
{
	return parallelisationPolicy;
}

RMResourceRequirement* RMTaskResourceRequirement::GetMasterRequirement() const
{
	return masterRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetSlaveRequirement() const
{
	return slaveRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetGlobalSlaveRequirement() const
{
	return globalSlaveRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetSharedRequirement() const
{
	return sharedRequirement;
}

RMResourceRequirement* RMTaskResourceRequirement::GetMasterSystemAtStart()
{
	return &masterSystemAtStart;
}

RMResourceRequirement* RMTaskResourceRequirement::GetSlaveSystemAtStart()
{
	return &slaveSystemAtStart;
}

void RMTaskResourceRequirement::SetMaxSlaveProcessNumber(int nValue)
{
	require(nValue >= 0);
	nSlaveProcessNumber = nValue;
}

int RMTaskResourceRequirement::GetMaxSlaveProcessNumber() const
{
	return nSlaveProcessNumber;
}

longint RMTaskResourceRequirement::GetSlaveMin(int nResourceType) const
{
	return slaveRequirement->GetResource(nResourceType)->GetMin();
}
longint RMTaskResourceRequirement::GetSlaveMax(int nResourceType) const
{
	return slaveRequirement->GetResource(nResourceType)->GetMax();
}
longint RMTaskResourceRequirement::GetMasterMin(int nResourceType) const
{
	return masterRequirement->GetResource(nResourceType)->GetMin();
}
longint RMTaskResourceRequirement::GetMasterMax(int nResourceType) const
{
	return masterRequirement->GetResource(nResourceType)->GetMax();
}
longint RMTaskResourceRequirement::GetSharedMin(int nResourceType) const
{
	return sharedRequirement->GetResource(nResourceType)->GetMin();
}
longint RMTaskResourceRequirement::GetSharedMax(int nResourceType) const
{
	return sharedRequirement->GetResource(nResourceType)->GetMax();
}
longint RMTaskResourceRequirement::GetSlaveGlobalMin(int nResourceType) const
{
	return globalSlaveRequirement->GetResource(nResourceType)->GetMin();
}
longint RMTaskResourceRequirement::GetSlaveGlobalMax(int nResourceType) const
{
	return globalSlaveRequirement->GetResource(nResourceType)->GetMax();
}

///////////////////////////////////////////////////////////////////////
// Implementation de RMTaskResourceGrant

RMTaskResourceGrant::RMTaskResourceGrant()
{
	bMissingResourceForProcConstraint = false;
	bMissingResourceForGlobalConstraint = false;
}

RMTaskResourceGrant::~RMTaskResourceGrant()
{
	oaResourceGrantWithRankIndex.RemoveAll();
	oaResourceGrant.DeleteAll();
	odHostProcNumber.DeleteAll();
}

int RMTaskResourceGrant::GetProcessNumber() const
{
	if (oaResourceGrant.GetSize() == 2)
		return 1;
	else
		return oaResourceGrant.GetSize();
}

int RMTaskResourceGrant::GetSlaveNumber() const
{
	if (oaResourceGrant.GetSize() == 0)
		return 0;
	if (oaResourceGrant.GetSize() == 2)
		return 1;
	return oaResourceGrant.GetSize() - 1;
}

boolean RMTaskResourceGrant::IsSequentialTask() const
{
	return oaResourceGrant.GetSize() == 2;
}

boolean RMTaskResourceGrant::IsEmpty() const
{
	return oaResourceGrant.GetSize() == 0;
}

longint RMTaskResourceGrant::GetMasterMemory() const
{
	return GetMasterResource(MEMORY);
}

longint RMTaskResourceGrant::GetMasterDisk() const
{
	return GetMasterResource(DISK);
}

longint RMTaskResourceGrant::GetMasterResource(int nResourceType) const
{
	require(oaResourceGrant.GetSize() > 1);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(0))->GetResource(nResourceType));
}

longint RMTaskResourceGrant::GetSlaveMemoryOf(int i) const
{
	require(i > 0);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(i))->GetMemory());
}

longint RMTaskResourceGrant::GetSlaveDiskOf(int i) const
{
	require(i > 0);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(i))->GetDiskSpace());
}

longint RMTaskResourceGrant::GetSharedMemory() const
{
	require(oaResourceGrant.GetSize() > 1);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(0))->GetSharedMemory());
}

longint RMTaskResourceGrant::GetMinSlaveMemory() const
{
	return GetMinSlaveResource(MEMORY);
}

longint RMTaskResourceGrant::GetMinSlaveDisk() const
{
	return GetMinSlaveResource(DISK);
}

longint RMTaskResourceGrant::GetMinSlaveResource(int nRT) const
{
	int i;
	longint lMin;
	RMResourceGrant* resource;

	if (oaResourceGrant.GetSize() == 0)
		return 0;

	lMin = LLONG_MAX;
	for (i = 1; i < oaResourceGrant.GetSize(); i++)
	{
		resource = cast(RMResourceGrant*, oaResourceGrant.GetAt(i));
		if (resource->GetRank() != 0)
			lMin = min(lMin, resource->GetResource(nRT));
	}
	return lMin;
}

longint RMTaskResourceGrant::GetMissingMemory()
{
	return rcMissingResources.GetValue(MEMORY);
}
longint RMTaskResourceGrant::GetMissingDisk()
{
	return rcMissingResources.GetValue(DISK);
}

ALString RMTaskResourceGrant::GetMissingResourceMessage()
{
	ALString sMessage;
	ALString sTmp;
	ALString sMissingResourceType;
	ALString sMissingResource;

	require(GetMissingMemory() > 0 or GetMissingDisk() > 0);
	require(IsEmpty());

	if (GetMissingMemory() > 0)
	{
		sMissingResourceType = "memory";
		sMissingResource = RMResourceManager::ActualMemoryToString(GetMissingMemory());
	}
	else
	{
		sMissingResourceType = "disk";
		sMissingResource = LongintToHumanReadableString(longint(1.1 * GetMissingDisk()));
	}

	if (bMissingResourceForProcConstraint)
	{
		sMessage = "requirements on " + sMissingResourceType +
			   " exceed user constraint per processus : task need extra " + sMissingResourceType + " " +
			   sMissingResource;
	}
	else
	{
		if (bMissingResourceForGlobalConstraint)
		{
			sMissingResource = sMessage =
			    "system resources are not sufficient to run the task (need extra " + sMissingResourceType +
			    " " + sMissingResource;
			if (RMResourceManager::GetResourceSystem()->GetHostNumber() != 1)
				sMessage += " on " + sHostMissingResource;
			sMessage += ")";
		}
		else
		{
			sMessage = "system resources are not sufficient to run the task (need extra " +
				   sMissingResourceType + " " + sMissingResource;
			if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1)
				sMessage += " on master host";
			sMessage += ")";
		}
	}
	return sMessage;
}

RMResourceGrant* RMTaskResourceGrant::GetResourceAtRank(int nRank) const
{
	if (nRank >= oaResourceGrantWithRankIndex.GetSize())
		return NULL;
	return cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(nRank));
}

void RMTaskResourceGrant::AddResource(RMResourceGrant* resource)
{
	int nRank;

	require(resource != NULL);
	require(resource->Check());

	// Ajout aux ressources
	oaResourceGrant.Add(resource);

	// Ajout aux ressources indexees
	nRank = resource->GetRank();
	if (oaResourceGrantWithRankIndex.GetSize() < nRank + 1)
		oaResourceGrantWithRankIndex.SetSize(nRank + 1);
	oaResourceGrantWithRankIndex.SetAt(nRank, resource);
}

void RMTaskResourceGrant::ReindexRank()
{
	int i;
	RMResourceGrant* rGrant;
	oaResourceGrantWithRankIndex.RemoveAll();

	for (i = 0; i < oaResourceGrant.GetSize(); i++)
	{
		rGrant = cast(RMResourceGrant*, oaResourceGrant.GetAt(i));
		if (oaResourceGrantWithRankIndex.GetSize() < rGrant->GetRank() + 1)
			oaResourceGrantWithRankIndex.SetSize(rGrant->GetRank() + 1);
		oaResourceGrantWithRankIndex.SetAt(rGrant->GetRank(), rGrant);
	}
}

void RMTaskResourceGrant::Initialize()
{
	rcMissingResources.Initialize();
	bMissingResourceForProcConstraint = false;
	oaResourceGrantWithRankIndex.RemoveAll();
	oaResourceGrant.DeleteAll();
}

void RMTaskResourceGrant::SetMasterResource(int nRT, longint lResource)
{
	rmMasterResource.SetValue(nRT, lResource);
}

void RMTaskResourceGrant::SetSharedResource(int nRT, longint lResource)
{
	rmSharedResource.SetValue(nRT, lResource);
}

void RMTaskResourceGrant::SetGlobalResource(int nRT, longint lResource)
{
	rmGlobalResource.SetValue(nRT, lResource);
}

void RMTaskResourceGrant::SetSlaveResource(int nRT, longint lResource)
{
	rmSlaveResource.SetValue(nRT, lResource);
}

void RMTaskResourceGrant::SetHostCoreNumber(const ALString& sHostName, int nProcNumber)
{
	IntObject* io;
	io = new IntObject;
	io->SetInt(nProcNumber);
	odHostProcNumber.SetAt(sHostName, io);
}
void RMTaskResourceGrant::Write(ostream& ost) const
{
	POSITION position;
	ALString sHostName;
	Object* oElement;
	IntObject* ioProcNumber;

	ost << "--    Granted resources    --" << endl;
	if (IsEmpty())
	{
		ost << " No resource available" << endl;
	}
	else
	{
		ost << "#procs " << GetProcessNumber() << endl;
		ost << "master " << rmMasterResource << endl;
		ost << "slave  " << rmSlaveResource << endl;
		ost << "shared " << rmSharedResource << endl;
		ost << "global " << rmGlobalResource << endl;

		position = odHostProcNumber.GetStartPosition();
		while (position != NULL)
		{
			odHostProcNumber.GetNextAssoc(position, sHostName, oElement);
			ioProcNumber = cast(IntObject*, oElement);
			ost << sHostName << "\t" << ioProcNumber->GetInt() << " procs" << endl;
		}
	}
}

///////////////////////////////////////////////////////////////////////
// Classe RMResourceGrant

RMResourceGrant::RMResourceGrant()
{
	int nResource;

	nRank = -1;
	lvResource.SetSize(RESOURCES_NUMBER);
	lvSharedResource.SetSize(RESOURCES_NUMBER);

	// Initialisation de toutes les ressources a -1
	for (nResource = 0; nResource < RESOURCES_NUMBER; nResource++)
	{
		lvResource.SetAt(nResource, -1);
		lvSharedResource.SetAt(nResource, -1);
	}
}

RMResourceGrant::~RMResourceGrant() {}

void RMResourceGrant::SetRank(int nValue)
{
	assert(nValue >= 0);
	nRank = nValue;
}

int RMResourceGrant::GetRank() const
{
	return nRank;
}

void RMResourceGrant::SetMemory(longint lValue)
{
	SetResource(MEMORY, lValue);
}

longint RMResourceGrant::GetMemory() const
{
	return GetResource(MEMORY);
}

void RMResourceGrant::SetDiskSpace(longint lValue)
{
	SetResource(DISK, lValue);
}

longint RMResourceGrant::GetDiskSpace() const
{
	return GetResource(DISK);
}

void RMResourceGrant::SetSharedMemory(longint lMemory)
{
	SetSharedResource(MEMORY, lMemory);
}

longint RMResourceGrant::GetSharedMemory() const
{
	return GetSharedResource(MEMORY);
}

void RMResourceGrant::SetSharedDisk(longint lDisk)
{
	SetSharedResource(DISK, lDisk);
}
longint RMResourceGrant::GetSharedDisk() const
{
	return GetSharedResource(DISK);
}

void RMResourceGrant::SetSharedResource(int nResource, longint lValue)
{
	require(nResource < RESOURCES_NUMBER);
	require(lValue >= 0);
	lvSharedResource.SetAt(nResource, lValue);
}

longint RMResourceGrant::GetSharedResource(int nResource) const
{
	require(nResource < RESOURCES_NUMBER);
	return lvSharedResource.GetAt(nResource);
}

void RMResourceGrant::SetResource(int nResource, longint lValue)
{
	require(nResource < RESOURCES_NUMBER);
	require(lValue >= 0);
	lvResource.SetAt(nResource, lValue);
}

longint RMResourceGrant::GetResource(int nResource) const
{
	require(nResource < RESOURCES_NUMBER);
	return lvResource.GetAt(nResource);
}

boolean RMResourceGrant::Check() const
{
	int nResource;
	for (nResource = 0; nResource < RESOURCES_NUMBER; nResource++)
	{
		if (GetResource(nResource) == -1 or GetSharedResource(nResource) == -1)
			return false;
	}
	if (nRank == -1)
		return false;
	return true;
}

void RMResourceGrant::Write(ostream& ost) const
{
	ost << "Resource grant for rank " << nRank << endl;
	ost << "\t"
	    << "Memory : ";
	if (GetMemory() == -1)
		ost << "NOT INITIALIZED !" << endl;
	else
		ost << LongintToHumanReadableString(GetMemory()) << endl;

	ost << "\t"
	    << "Disk space : ";
	if (GetDiskSpace() == -1)
		ost << "NOT INITIALIZED !" << endl;
	else
		ost << LongintToHumanReadableString(GetDiskSpace()) << endl;

	ost << "\t"
	    << "Shared memory : ";
	if (GetSharedMemory() == -1)
		ost << "NOT INITIALIZED !" << endl;
	else
		ost << LongintToHumanReadableString(GetSharedMemory()) << endl;

	ost << "\t"
	    << "Shared disk ";
	if (GetSharedDisk() == -1)
		ost << "NOT INITIALIZED !" << endl;
	else
		ost << LongintToHumanReadableString(GetSharedDisk()) << endl;
}