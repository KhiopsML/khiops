// Copyright (c) 2024 Orange. All rights reserved.
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

static longint lprod(int n, longint l)
{
	if (l == LLONG_MAX)
		return LLONG_MAX;
	else
		return n * l;
}

static void PrintResource(const ALString& sName, longint lValue)
{
	ALString sValue;
	if (lValue == LONG_MAX)
		sValue = "INF";
	else
		sValue = LongintToHumanReadableString(lValue);
	cout << sName << " " << sValue;
	if (PLParallelTask::GetTracerResources() == 3)
		cout << " => " << LongintToReadableString(lValue);
	cout << endl;
}

///////////////////////////////////////////////////////////////////////
// Implementation de RMParallelResourceManager

RMParallelResourceManager::RMParallelResourceManager()
{
	clusterResources = NULL;
	taskRequirements = NULL;
}

RMParallelResourceManager::~RMParallelResourceManager()
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
	PLSolution* solution;
	Timer t;

	if (PLParallelTask::GetTracerResources() > 0)
	{
		if (PLParallelTask::GetCurrentTaskName() != "")
			cout << endl << "Task " << PLParallelTask::GetCurrentTaskName() << endl;
		else
			cout << endl << "Outside task" << endl;

		cout << endl << endl << *resourceSystem;
		PrintResource("\tSlave reserve", RMParallelResourceManager::GetSlaveReserve(MEMORY));
		PrintResource("\tMaster reserve", RMParallelResourceManager::GetMasterReserve(MEMORY));
		cout << endl;
		cout << *resourceRequirement << endl;
		cout << RMResourceConstraints::ToString();
	}

	// Initialisation du gestionnaire de ressources
	manager.SetRequirement(resourceRequirement);
	manager.SetResourceCluster(resourceSystem);

	// Calcul des ressources pour le systeme
	solution = manager.Solve();
	// cout << *solution << endl;

	manager.BuildGrantedResources(solution, resourceSystem, grantedResource);

	delete solution;
	if (PLParallelTask::GetTracerResources() > 0)
	{
		cout << *grantedResource << endl;
	}

	// Verification des resultats
	debug(manager.Check(grantedResource));
	return not grantedResource->IsEmpty();
}

const ALString RMParallelResourceManager::GetClassLabel() const
{
	return "RMParallelResourceManager";
}

void RMParallelResourceManager::Test()
{
	PLTestCluster test;

	PLParallelTask::SetVerbose(false);
	PLParallelTask::SetSimulatedSlaveNumber(50);
	PLParallelTask::SetParallelSimulated(false);

	test.Reset();
	test.SetTestLabel("unbalanced cluster");
	test.SetCluster(RMResourceSystem::CreateUnbalancedCluster());
	test.SetMaxCoreNumberOnSystem(8);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test sans contraintes
	test.Reset();
	test.SetTestLabel("100 process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 1000 * lGB, 1000 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test avec la contrainte globale
	test.Reset();
	test.SetTestLabel("100 process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 1000 * lGB, 1000 * lGB, 0));
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(100 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 1 * lGB, 1 * lGB, 0));
	test.SetTestLabel("Max proc with 1 Gb");
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 2 Gb");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 2 * lGB, 1 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on host");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 10 * lGB, 1 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test avec les contraintes utilisateurs
	// On aura le memes resultats que le test precedent
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.SetDiskLimitPerHost(1 * lGB);
	test.SetMemoryLimitPerHost(10 * lGB);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("2 Gb on host and 200 Mo disk per salve");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1000, 2 * lGB, 1 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->Set(200 * lMB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Test de non rgeression, en 32 Bits (2 Gb allouables)
	test.Reset();
	test.SetTestLabel("Non regression 32 bits");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 4, 2 * lGB, 0, 0));
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 1");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 2");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, avec contrainte memoire pour le sesclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Une seule machine programme sequentiel
	test.Reset();
	test.SetTestLabel("mono machines : sequential");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 100, 10 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(4 * lGB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.Solve();

	// Une seule machine, un seul slaveProcess
	test.Reset();
	test.SetTestLabel("mono machines : 1 slave process");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 1, 10 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->Set(25 * lMB);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(16 * lMB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetDisk()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->SetMaxSlaveProcessNumber(1);
	test.Solve();

	// le test suivant est lance uniquement si on est en mode graphique, ce qui n'est pas le cas
	// dans les tests unitaires
	if (UIObject::GetUIMode() == UIObject::Graphic)
	{
		// Une seule machine, un seul processeur, l'esclave demande plus que ce qu'il y a sur le host
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
	}

	// Regression
	test.Reset();
	test.SetTestLabel("regression : Master requirement 300 KB and granted 0");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(1, 16, 1153433600, 0, 0));
	test.SetMasterSystemAtStart(8539455);
	test.SetSlaveSystemAtStart(7228835);
	test.SetMemoryLimitPerHost(1153433600);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(16474230);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->Set(358692);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
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
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);

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

	// Plusieurs machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 0));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(10 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.Solve();

	// Plusieurs machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 1));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);

	test.Solve();

	// Plusieurs machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2 - Global Constraint");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(10, 100, 100 * lGB, 100 * lGB, 2));
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
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
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
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

	// Plusieurs machines, avec contrainte memoire pour les esclaves + contrainte globale
	// Test de performance
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint - 100 hosts");
	test.SetCluster(RMResourceSystem::CreateSyntheticCluster(100, 20, 100 * lGB, 100 * lGB, 0));
	test.SetMasterSystemAtStart(4000000);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
	test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);
	test.GetTaskRequirement()->GetMasterRequirement()->GetMemory()->SetMax(LLONG_MAX);
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
}

void RMParallelResourceManager::TestPerformances()
{
	int nClasseNumber;
	int nCpus;
	int nHostNumber;

	PLTestCluster test;

	PLParallelTask::SetVerbose(false);
	PLParallelTask::SetSimulatedSlaveNumber(50);
	PLParallelTask::SetParallelSimulated(false);

	cout << "hosts\tcpus\tclasses\ttime(s)" << endl;

	nHostNumber = 10;
	while (nHostNumber <= 1000)
	{
		nCpus = 16;
		while (nCpus <= 128)
		{
			nClasseNumber = 1;
			while (nClasseNumber <= 64)
			{
				if (nClasseNumber <= nHostNumber)
				{
					test.Reset();
					test.SetVerbose(false);
					test.SetCluster(RMResourceSystem::CreateSyntheticClusterWithClasses(
					    nHostNumber, nCpus, 100 * lGB, 100 * lGB, nClasseNumber));
					test.SetMasterSystemAtStart(4000000);
					test.GetTaskRequirement()->GetSlaveRequirement()->GetMemory()->SetMin(1 * lGB);
					test.GetTaskRequirement()->GetGlobalSlaveRequirement()->GetMemory()->Set(1 *
														 lGB);
					test.Solve();
					cout << nHostNumber << "\t" << nCpus << "\t" << nClasseNumber << "\t"
					     << test.GetElapsedTime() << endl;
				}
				nClasseNumber *= 2;
			}
			nCpus *= 2;
		}
		nHostNumber *= 10;
	}
}

void RMParallelResourceManager::Clean()
{
	clusterResources = NULL;
	taskRequirements = NULL;
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

PLSolution* RMParallelResourceManager::Solve()
{

	ALString sSignature;
	PLSolution* solution;
	IntVector ivHostCountPerProcNumber;
	boolean bIsSequential;
	PLSolution* sequentialSolution;
	Timer t;
	ALString sTmp;
	int nClassesNumber;

	require(taskRequirements->Check());

	bIsSequential = false;
	if (clusterResources->GetLogicalProcessNumber() == 1 and not PLParallelTask::GetParallelSimulated())
		bIsSequential = true;

	// Construction de la solution qu
	solution = BuildInitialSolution();
	nClassesNumber = solution->GetHostClassSolutionNumber();
	if (PLParallelTask::GetTracerResources() > 0)
	{
		cout << "Classes number: " << nClassesNumber << endl;
		for (int i = 0; i < nClassesNumber; i++)
		{
			cout << "\t" << solution->GetHostSolutionAt(i)->GetHostClass()->GetDefinition()->GetSignature()
			     << endl;
		}
		cout << endl;
	}

	t.Start();
	// Resolution dans le cas non sequentiel
	if (not bIsSequential or PLParallelTask::GetParallelSimulated())
	{
		GreedySolve(solution);
		PostOptimize(solution);
	}

	// Cas ou on preferera une solution sequentielle
	// - la solution n'est pas valide
	// - il n'y a qu'un seul process
	// - il y a 2 process sur une seule machine
	// Si il n'y a pas de solutions valide avec plusieurs machine ou si il n'y a qu'un coeur
	// On cherche une solution sequentielle
	if (bIsSequential or solution->GetUsedHostNumber() == 0 or solution->GetUsedProcessNumber() == 1 or
	    (solution->GetUsedHostNumber() == 1 and solution->GetUsedProcessNumber() == 2 and
	     not PLParallelTask::GetParallelSimulated()))
	{
		// Calcul d'une solution sequentielle
		sequentialSolution = BuildSequentialSolution();
		sequentialSolution->Evaluate(true);
		solution->CopyFrom(sequentialSolution);
		delete sequentialSolution;
	}
	else
	{
		// Si il y a 2 machines et un seul esclave, on preferera une solution sequentielle si elle existe
		// Sinon on gardera la solution avec 2 machines (qui est tres inefficace mais qui a le merite d'exister)
		if (solution->GetUsedHostNumber() == 2 and solution->GetUsedProcessNumber() == 2)
		{
			sequentialSolution = BuildSequentialSolution();
			sequentialSolution->Evaluate(true);
			if (sequentialSolution->IsValid())
			{
				solution->CopyFrom(sequentialSolution);
			}
			delete sequentialSolution;
		}
	}
	t.Stop();
	if (t.GetElapsedTime() > 1)
	{
		AddWarning(sTmp + "More than 1 second was spent to compute the optimal cluster resources (" +
			   IntToString(clusterResources->GetHostNumber()) + " hosts" + ", " +
			   IntToString(clusterResources->GetPhysicalCoreNumber()) + " cores" + ", " +
			   IntToString(nClassesNumber) + " classes of hosts)");
	}
	ensure(solution->Check());
	return solution;
}

void RMParallelResourceManager::GreedySolve(PLSolution* solution)
{
	longint lMasterHiddenResource;
	longint lSlaveHiddenResource;
	ObjectArray oaHostCountPerProcNumber;
	PLSolution* bestLocaleSolution;
	PLSolution* currentSolution;
	ObjectArray solutions;
	PLHostClassSolution* hostClassSolution;
	int nProcNumber;
	int i;
	int nProcMax;
	int nProcCount;
	const boolean bSequential = false;

	if (PLParallelTask::GetTracerResources() == 3)
	{
		taskRequirements->WriteDetails(cout);
		cout << endl;
		lMasterHiddenResource = RMParallelResourceManager::GetMasterHiddenResource(bSequential, MEMORY);
		lSlaveHiddenResource = RMParallelResourceManager::GetSlaveHiddenResource(bSequential, MEMORY);
		PrintResource(ResourceToString(MEMORY) + " master hidden resource", lMasterHiddenResource);
		PrintResource(ResourceToString(MEMORY) + " slave  hidden resource:", lSlaveHiddenResource);
	}

	// Calcul du nombre de processus max sur tout le cluster
	nProcMax = INT_MAX;
	nProcMax = min(nProcMax, taskRequirements->GetMaxSlaveProcessNumber() + 1);
	if (PLParallelTask::GetParallelSimulated())
		nProcMax = min(nProcMax, PLParallelTask::GetSimulatedSlaveNumber() + 1);
	else
		nProcMax = min(nProcMax, RMResourceConstraints::GetMaxCoreNumberOnCluster());
	assert(nProcMax >= 0);

	// Algorithme glouton : a chaque iteration, on ajoute un processus.
	solution->Evaluate(bSequential);
	currentSolution = solution->Clone();
	nProcCount = 0;

	while (currentSolution->GetUsedProcessNumber() < nProcMax)
	{
		bestLocaleSolution = NULL;

		// Construction et comparaison de toutes les solutions avec 1 processus en plus
		for (i = 0; i < currentSolution->GetHostClassSolutionNumber(); i++)
		{
			hostClassSolution = currentSolution->GetHostSolutionAt(i);
			for (nProcNumber = 0; nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize();
			     nProcNumber++)
			{
				if (hostClassSolution->AllowsOnMoreProc(nProcNumber))
				{
					// Ajout d'un processus a la solution courante
					currentSolution->AddProcessusOnHost(i, nProcNumber);

					// Si c'est la premiere solution evaluee, on la garde, meme si elle est non
					// valide : elle peut l'etre dans les iterations prochaines qui ajouteront des
					// processus
					if (bestLocaleSolution == NULL)
						bestLocaleSolution = currentSolution->Clone();
					else
					{
						// On garde la meilleure des solution de cette classe de machine
						if (currentSolution->CompareTo(bestLocaleSolution) > 0)
						{
							bestLocaleSolution->CopyFrom(currentSolution);
						}
					}

					// Suppression du processus pour revenir a la solution initiale
					currentSolution->RemoveProcessusOnHost(i, nProcNumber + 1);
				}
			}
		}

		// Mise a jour de la solution courante avec la meilleure solution locale
		if (bestLocaleSolution != NULL)
		{
			currentSolution->CopyFrom(bestLocaleSolution);
			if (bestLocaleSolution->IsValid())
			{
				solution->CopyFrom(bestLocaleSolution);
			}
			delete bestLocaleSolution;
		}
		else
			break;
	}
	delete currentSolution;
}

void RMParallelResourceManager::PostOptimize(PLSolution* solution) {}

PLSolution* RMParallelResourceManager::BuildSequentialSolution() const
{
	int i;
	RMHostResource* host;
	PLHostClass* hostClass;
	PLSolution* solution;
	PLHostClassSolution* hostSolution;
	IntVector ivHostCountPerProcNumber;

	host = NULL;

	// Recherche de la machine maitre
	for (i = 0; i < clusterResources->GetHostNumber(); i++)
	{
		host = clusterResources->GetHostResourceAt(i);
		if (host->IsMasterHost())
			break;
	}
	assert(host != NULL);
	hostClass = new PLHostClass;
	hostClass->AddHost(host);
	hostClass->FitDefinition();

	// Initialisation de la meilleure solution : au depart la solution vide
	solution = new PLSolution;
	solution->SetTaskRequirements(taskRequirements);
	hostSolution = new PLHostClassSolution;
	hostSolution->SetHostClass(hostClass);
	if (PLParallelTask::GetParallelSimulated())
		ivHostCountPerProcNumber.SetSize(PLParallelTask::GetSimulatedSlaveNumber() + 2);
	else
		ivHostCountPerProcNumber.SetSize(hostClass->GetDefinition()->GetProcNumber() + 1);
	ivHostCountPerProcNumber.Initialize();
	ivHostCountPerProcNumber.SetAt(1, 1);
	hostSolution->SetHostCountPerProcNumber(&ivHostCountPerProcNumber);
	solution->AddHostSolution(hostSolution);

	ensure(solution->Check());
	delete hostClass;
	return solution;
}

PLSolution* RMParallelResourceManager::BuildInitialSolution() const
{
	longint lBound;
	LongintVector lvMemoryBounds;
	LongintVector lvDiskBounds;
	RMHostResource* host;
	PLHostClassDefinition* hostClassDefinition;
	PLHostClass* hostClass;
	ObjectDictionary odHostClasses; // Dictionnaire de signature / PLHostClass
	PLSolution* solution;
	PLHostClassSolution* hostSolution;
	POSITION position;
	ALString sSignature;
	Object* oElement;
	int i;

	const boolean bHugeDensity = false;

	// Construction des bornes pour la definition des classes de machines
	if (bHugeDensity)
	{
		// Pour les tests on definit les classe par pas de 1 Mo
		lBound = 512 * lKB;
		while (lBound <= 512 * lGB)
		{
			lvMemoryBounds.Add(lBound);
			lBound += 1 * lMB;
		}
		lvMemoryBounds.Add(LONG_MAX);

		lBound = 1 * lGB;
		while (lBound <= 10 * lTB)
		{
			lvDiskBounds.Add(lBound);
			lBound += 1 * lMB;
		}
		lvDiskBounds.Add(LONG_MAX);
	}
	else
	{
		lBound = 512 * lKB;
		while (lBound <= 512 * lGB)
		{
			lvMemoryBounds.Add(lBound);
			lBound = (longint)(sqrt(2) * lBound);
		}
		lvMemoryBounds.Add(LONG_MAX);

		lBound = 1 * lGB;
		while (lBound <= 10 * lTB)
		{
			lvDiskBounds.Add(lBound);
			lBound = (longint)(sqrt(2) * lBound);
		}
		lvDiskBounds.Add(LONG_MAX);
	}

	// Construction des classes de machines
	for (i = 0; i < clusterResources->GetHostNumber(); i++)
	{
		host = clusterResources->GetHostResourceAt(i);

		if (host->GetLogicalProcessNumber() != 0)
		{
			// Construction de la definition de la classe
			hostClassDefinition = PLHostClassDefinition::BuildClassDefinitionForHost(
			    taskRequirements, host, &lvMemoryBounds, &lvDiskBounds);

			// Recherche d'une classe qui a la meme definition
			hostClass = cast(PLHostClass*, odHostClasses.Lookup(hostClassDefinition->GetSignature()));

			if (hostClass == NULL)
			{
				// Si il n'existe pas de classe pour cette definition, on en cree une nouvelle
				hostClass = new PLHostClass;
				hostClass->SetDefinition(hostClassDefinition);
				odHostClasses.SetAt(hostClassDefinition->GetSignature(), hostClass);
			}

			// Ajout de la machine dans sa classe
			hostClass->AddHost(host);

			delete hostClassDefinition;
		}
	}

	// Reduction des definitions pour 'coller' au machines
	// et ajout a la solution
	solution = new PLSolution;
	solution->SetTaskRequirements(taskRequirements);
	position = odHostClasses.GetStartPosition();
	while (position != NULL)
	{
		// Reduction
		odHostClasses.GetNextAssoc(position, sSignature, oElement);
		hostClass = cast(PLHostClass*, oElement);
		hostClass->FitDefinition();

		// Ajout a la solution
		hostSolution = new PLHostClassSolution;
		hostSolution->SetHostClass(hostClass);
		solution->AddHostSolution(hostSolution);
	}

	// Tri des solutions pour avoir un resultats reproductible
	solution->SortHostClasses();
	odHostClasses.DeleteAll();

	return solution;
}

void RMParallelResourceManager::BuildGrantedResources(const PLSolution* solution,
						      const RMResourceSystem* resourceSystem,
						      RMTaskResourceGrant* grantedResource) const
{
	PLHostClassSolution* hostClassSolution;
	RMHostResource* host;
	int nProcNumber;
	int nRank;
	int nRT;
	int i;
	int j;
	int k;
	int nHostNumber;
	int nHostNumberForProc;
	int nHostIndex;
	boolean bIsSequential;
	PLSolution* lastSolution;

	require(grantedResource != NULL);
	grantedResource->Initialize();
	if (solution->IsValid())
	{

		// Transformation de la solution en une nouvelle solution qui ne contient que des singletons
		// On va pouvoir utiliser la methode Evaluate() pour saturer les ressources
		lastSolution = solution->Expand();
		bIsSequential = lastSolution->GetUsedProcessNumber() == 1;
		lastSolution->Evaluate(bIsSequential);

		// Recopie des ressources allouees
		for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
		{
			grantedResource->masterResourceGrant->GetResource()->SetValue(
			    nRT, lastSolution->GetMasterResource()->GetValue(nRT));
			grantedResource->masterResourceGrant->GetSharedResource()->SetValue(
			    nRT, lastSolution->GetSharedResource()->GetValue(nRT));
			grantedResource->slaveResourceGrant->GetResource()->SetValue(
			    nRT, lsum(lastSolution->GetSlaveResource()->GetValue(nRT),
				      lastSolution->GetGlobalResource()->GetValue(nRT)));
			grantedResource->slaveResourceGrant->GetSharedResource()->SetValue(
			    nRT, lastSolution->GetSharedResource()->GetValue(nRT));
		}

		// Construction du vecteur qui contient 1 a l'index nRank, si le process de rang nRank a des ressources
		// pour travailler Pour la taille de ce tableau, c'est le nombre de processus logique auquel on ajoute
		// le nombre de host car il y a un FileServer par host qui n'est pas pris en compte dans
		// GetLogicalProcessNumber ils sont par contre pris en compte dans le rang des esclaves
		if (PLParallelTask::GetParallelSimulated())
			grantedResource->ivProcessWithResources.SetSize(PLParallelTask::GetSimulatedSlaveNumber() + 1);
		else
			grantedResource->ivProcessWithResources.SetSize(resourceSystem->GetLogicalProcessNumber() +
									resourceSystem->GetHostNumber());

		// Comptage du nombre de host dans la solution
		nHostNumber = 0;
		for (i = 0; i < lastSolution->GetHostClassSolutionNumber(); i++)
		{
			hostClassSolution = lastSolution->GetHostSolutionAt(i);
			nHostNumber += hostClassSolution->GetHostClass()->GetHostNumber() -
				       hostClassSolution->GetHostCountPerProcNumber()->GetAt(0);
		}

		// Construction des granted resources
		while (grantedResource->odHostProcNumber.GetCount() != nHostNumber)
		{
			for (i = 0; i < lastSolution->GetHostClassSolutionNumber(); i++)
			{
				hostClassSolution = lastSolution->GetHostSolutionAt(i);
				nHostIndex = 0;
				for (nProcNumber = 1;
				     nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize();
				     nProcNumber++)
				{
					nHostNumberForProc =
					    hostClassSolution->GetHostCountPerProcNumber()->GetAt(nProcNumber);
					for (j = 0; j < nHostNumberForProc; j++)
					{
						host = cast(
						    RMHostResource*,
						    hostClassSolution->GetHostClass()->GetHosts()->GetAt(nHostIndex));
						nHostIndex++;
						grantedResource->SetHostCoreNumber(host->GetHostName(), nProcNumber);
						for (k = 0; k < nProcNumber; k++)
						{
							if (PLParallelTask::GetParallelSimulated())
								nRank = k;
							else
								nRank = host->GetRanks()->GetAt(k);
							grantedResource->ivProcessWithResources.SetAt(nRank, 1);
						}
					}
				}
			}
		}
		delete lastSolution;
	}
	else
	{
		grantedResource->nProcessNumber = 0;
		grantedResource->rcMissingResources->CopyFrom(solution->GetMissingResources());

		// La machine sur laquelle il manque des resource sest necessairement le maitre, car si il n'y a pas de
		// place pour lancer le job en multi-machines, on essaye en sequentiel sur la machine du maitre
		grantedResource->sHostMissingResource = resourceSystem->GetMasterHostResource()->GetHostName();
	}
}

boolean RMParallelResourceManager::Check(const RMTaskResourceGrant* taskResourceGrant) const
{
	int nRT;
	boolean bOK = true;

	if (taskResourceGrant->GetSlaveNumber() == 0)
		return true;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		if (PLParallelTask::GetParallelSimulated() or taskResourceGrant->GetSlaveNumber() > 1)
		{
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) >= taskRequirements->GetMasterMin(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetSlaveResource(nRT) >=
					lsum(taskRequirements->GetSlaveMin(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin() /
						 taskResourceGrant->GetSlaveNumber());
			assert(bOK);
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) <= taskRequirements->GetMasterMax(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetSlaveResource(nRT) <=
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
			    bOK and taskResourceGrant->GetSlaveResource(nRT) >=
					lsum(taskRequirements->GetSlaveMin(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin());
			assert(bOK);
			bOK = bOK and taskResourceGrant->GetMasterResource(nRT) <= taskRequirements->GetMasterMax(nRT);
			assert(bOK);
			bOK =
			    bOK and taskResourceGrant->GetSlaveResource(nRT) <=
					lsum(taskRequirements->GetSlaveMax(nRT),
					     taskRequirements->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax());
			assert(bOK);
		}
	}
	return bOK;
}

longint RMParallelResourceManager::GetMasterReserve(int nResourceType)
{
	require(nResourceType < RESOURCES_NUMBER);

	if (nResourceType == MEMORY)
		return SystemFileDriverCreator::GetMaxPreferredBufferSize() +
		       RMStandardResourceDriver::PhysicalToLogical(MEMORY, UIObject::GetUserInterfaceMemoryReserve() +
									       MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint RMParallelResourceManager::GetSlaveReserve(int nResourceType)
{
	require(nResourceType < RESOURCES_NUMBER);
	if (nResourceType == MEMORY)
		return SystemFileDriverCreator::GetMaxPreferredBufferSize() +
		       RMStandardResourceDriver::PhysicalToLogical(MEMORY, MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint RMParallelResourceManager::GetMasterHiddenResource(boolean bIsSequential, int nRT)
{
	longint lHiddenResource;
	lHiddenResource =
	    RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(nRT)->GetMin() + GetMasterReserve(nRT);

	// On ajoute 2 tailles de blocs pour la serialisation
	if (not bIsSequential and nRT == MEMORY)
		lHiddenResource += 2 * MemSegmentByteSize;
	return lHiddenResource;
}

longint RMParallelResourceManager::GetSlaveHiddenResource(boolean bIsSequential, int nRT)
{
	longint lHiddenResource;
	if (bIsSequential)
		lHiddenResource = 0;
	else
	{
		lHiddenResource = RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(nRT)->GetMin() +
				  GetSlaveReserve(nRT);

		// On ajoute 2 tailles de blocs pour la serialisation
		if (nRT == MEMORY)
			lHiddenResource += 2 * MemSegmentByteSize;
	}
	return lHiddenResource;
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLHostClassDefinition
PLHostClassDefinition::PLHostClassDefinition()
{
	resourceMin = new RMResourceContainer;
	resourceMax = new RMResourceContainer;
	bIsMasterClass = false;
	nProcNumber = 0;
}

PLHostClassDefinition::~PLHostClassDefinition()
{
	delete resourceMin;
	delete resourceMax;
}

void PLHostClassDefinition::CopyFrom(const PLHostClassDefinition* source)
{
	require(source->Check());
	resourceMin->CopyFrom(source->resourceMin);
	resourceMax->CopyFrom(source->resourceMax);
	nProcNumber = source->nProcNumber;
	bIsMasterClass = source->bIsMasterClass;
}

void PLHostClassDefinition::SetMasterClass(boolean bIsMaster)
{
	bIsMasterClass = bIsMaster;
}
boolean PLHostClassDefinition::GetMasterClass() const
{
	return bIsMasterClass;
}

void PLHostClassDefinition::SetProcNumber(int nProc)
{
	nProcNumber = nProc;
}

int PLHostClassDefinition::GetProcNumber() const
{
	return nProcNumber;
}

void PLHostClassDefinition::SetMemoryMin(longint lMemory)
{
	resourceMin->SetValue(MEMORY, lMemory);
}

longint PLHostClassDefinition::GetMemoryMin() const
{
	return resourceMin->GetValue(MEMORY);
}

void PLHostClassDefinition::SetMemoryMax(longint lMemory)
{
	resourceMax->SetValue(MEMORY, lMemory);
}

longint PLHostClassDefinition::GetMemoryMax() const
{
	return resourceMax->GetValue(MEMORY);
}

void PLHostClassDefinition::SetDiskMin(longint lDisk)
{
	resourceMin->SetValue(DISK, lDisk);
}

longint PLHostClassDefinition::GetDiskMin() const
{
	return resourceMin->GetValue(DISK);
}

void PLHostClassDefinition::SetDiskMax(longint lDisk)
{
	resourceMax->SetValue(DISK, lDisk);
}
longint PLHostClassDefinition::GetDiskMax() const
{
	return resourceMax->GetValue(DISK);
}

RMResourceContainer* PLHostClassDefinition::GetResourceMin() const
{
	return resourceMin;
}

RMResourceContainer* PLHostClassDefinition::GetResourceMax() const
{
	return resourceMax;
}

ALString PLHostClassDefinition::GetSignature() const
{
	ALString sSignature;
	if (bIsMasterClass)
		sSignature = "master_class";
	else
	{
		sSignature += IntToString(nProcNumber);
		sSignature += "_";
		sSignature += LongintToString(resourceMin->GetValue(MEMORY));
		sSignature += "_";
		sSignature += LongintToString(resourceMax->GetValue(MEMORY));
		sSignature += "_";
		sSignature += LongintToString(resourceMin->GetValue(DISK));
		sSignature += "_";
		sSignature += LongintToString(resourceMax->GetValue(DISK));
	}
	return sSignature;
}

PLHostClassDefinition* PLHostClassDefinition::BuildClassDefinitionForHost(const RMTaskResourceRequirement* requirements,
									  const RMHostResource* host,
									  const LongintVector* lvMemoryBounds,
									  const LongintVector* lvDiskBounds)
{
	PLHostClassDefinition* classDefinition;
	longint lHostResource;
	longint lBoundMax;
	longint lBoundMin;
	int nRT;
	int i;
	longint lHostOverallMax;

	classDefinition = new PLHostClassDefinition;
	classDefinition->SetMasterClass(host->IsMasterHost());

	// Nombre de processus
	classDefinition->SetProcNumber(host->GetLogicalProcessNumber());

	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		// Resource minimale utilisse par la tache sur cette machine en prenant en compte une approximation
		// des ressources globales (cette approximation est grossiere car on les divise par le nombre d'esclaves
		// du host et non le nombre d'esclave global) L'idee est d'eviter de construire des classes inutiles : on
		// ne construira qu'une seule classe qui a plus que cette resource minimale
		lHostOverallMax = requirements->GetMasterMin(nRT) + requirements->GetSharedMin(nRT) +
				  host->GetLogicalProcessNumber() *
				      (requirements->GetSlaveMin(nRT) + requirements->GetSharedMin(nRT)) +
				  requirements->GetSlaveGlobalMin(nRT) / host->GetLogicalProcessNumber();
		lHostResource = host->GetResourceFree(nRT);
		if (lHostResource >= lHostOverallMax)
		{
			classDefinition->GetResourceMin()->SetValue(nRT, lHostOverallMax);
			classDefinition->GetResourceMax()->SetValue(nRT, LLONG_MAX);
		}
		else
		{
			lBoundMin = 0;
			lBoundMax = 0;
			for (i = 0; i < lvMemoryBounds->GetSize(); i++)
			{
				if (nRT == MEMORY)
					lBoundMax = lvMemoryBounds->GetAt(i);
				else
					lBoundMax = lvDiskBounds->GetAt(i);

				if (lBoundMin <= lHostResource and lHostResource < lBoundMax)
				{
					classDefinition->GetResourceMin()->SetValue(nRT, lBoundMin);
					classDefinition->GetResourceMax()->SetValue(nRT, lBoundMax);
					break;
				}
				else
				{
					lBoundMin = lBoundMax;
				}
			}
		}
	}

	ensure(classDefinition->Check());
	return classDefinition;
}

void PLHostClassDefinition::Write(ostream& ost) const
{
	int nRT;
	ost << GetSignature() << endl;
	ost << "\t#procs " << IntToString(nProcNumber) << endl;

	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		ost << "\t" << ResourceToString(nRT) << " [" << LongintToHumanReadableString(resourceMin->GetValue(nRT))
		    << " ; " << LongintToHumanReadableString(resourceMax->GetValue(nRT)) << "[" << endl;
	}
}

boolean PLHostClassDefinition::Check() const
{
	boolean bOk;
	bOk = true;
	bOk = resourceMin->GetValue(MEMORY) < resourceMax->GetValue(MEMORY) and bOk;
	ensure(bOk);
	bOk = resourceMin->GetValue(DISK) < resourceMax->GetValue(DISK) and bOk;
	ensure(bOk);
	bOk = nProcNumber >= 0 and bOk;
	ensure(bOk);
	return bOk;
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLHostClass

PLHostClass::PLHostClass()
{
	bContainsMaster = false;
	definition = new PLHostClassDefinition;
}

PLHostClass::~PLHostClass()
{
	oaHosts.RemoveAll();
	delete definition;
}

PLHostClass* PLHostClass::Clone() const
{
	PLHostClass* clone;
	clone = new PLHostClass;
	clone->CopyFrom(this);
	return clone;
}

void PLHostClass::CopyFrom(const PLHostClass* hostClassSource)
{
	require(hostClassSource->Check());
	bContainsMaster = hostClassSource->bContainsMaster;
	definition->CopyFrom(hostClassSource->definition);
	oaHosts.CopyFrom(&hostClassSource->oaHosts);
}

void PLHostClass::SetDefinition(PLHostClassDefinition* dValue)
{
	require(dValue->Check());
	definition->CopyFrom(dValue);
}

PLHostClassDefinition* PLHostClass::GetDefinition() const
{
	return definition;
}

void PLHostClass::FitDefinition()
{
	int i;
	longint lMemoryMax;
	longint lMemoryMin;
	longint lDiskMax;
	longint lDiskMin;
	RMHostResource* host;

	lMemoryMax = 0;
	lMemoryMin = LLONG_MAX;
	lDiskMax = 0;
	lDiskMin = LLONG_MAX;
	if (oaHosts.GetSize() == 0)
		return;

	// Parcours de toutes les machines pour avoir les min et max des ressources utilisees
	for (i = 0; i < oaHosts.GetSize(); i++)
	{
		host = cast(RMHostResource*, oaHosts.GetAt(i));
		definition->SetProcNumber(host->GetLogicalProcessNumber());
		lMemoryMax = max(lMemoryMax, host->GetPhysicalMemory());
		lMemoryMin = min(lMemoryMin, host->GetPhysicalMemory());
		lDiskMax = max(lDiskMax, host->GetDiskFreeSpace());
		lDiskMin = min(lDiskMin, host->GetDiskFreeSpace());
	}

	require(lMemoryMax < LLONG_MAX and lDiskMax < LLONG_MAX);

	// On ajoute 1 aux max car c'est une borne stricte
	lMemoryMax++;
	lDiskMax++;

	definition->SetDiskMin(lDiskMin);
	definition->SetDiskMax(lDiskMax);
	definition->SetMemoryMin(lMemoryMin);
	definition->SetMemoryMax(lMemoryMax);
	ensure(definition->Check());
	ensure(Check());
}

void PLHostClass::AddHost(RMHostResource* host)
{
	// On regarde si la machine contient le master
	if (host->IsMasterHost())
		bContainsMaster = true;

	// Ajout de la machine dans la liste
	oaHosts.Add(host);
}

const ObjectArray* PLHostClass::GetHosts() const
{
	return &oaHosts;
}

longint PLHostClass::GetAvaiblableResource(int nRT) const
{
	longint lResourceFree;
	longint lHostResource;

	lResourceFree = GetDefinition()->GetResourceMin()->GetValue(nRT);
	lHostResource = min(lResourceFree, RMResourceConstraints::GetResourceLimit(nRT) * lMB);
	lHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lHostResource);

	return lHostResource;
}

int CompareHosts(const void* elem1, const void* elem2)
{
	RMHostResource* host1;
	RMHostResource* host2;
	int nCompare;
	const longint lResourcePrecision = 100 * lMB;

	check(elem1);
	check(elem2);

	// Acces aux objets
	host1 = cast(RMHostResource*, *(Object**)elem1);
	host2 = cast(RMHostResource*, *(Object**)elem2);

	assert(host1->GetLogicalProcessNumber() == host2->GetLogicalProcessNumber());

	// Comparaison a 100 MB pres
	nCompare = CompareLongint(lResourcePrecision * (host1->GetPhysicalMemory() / lResourcePrecision),
				  lResourcePrecision * (host2->GetPhysicalMemory() / lResourcePrecision));
	if (nCompare == 0)
	{
		nCompare = CompareLongint(lResourcePrecision * (host1->GetDiskFreeSpace() / lResourcePrecision),
					  lResourcePrecision * (host2->GetDiskFreeSpace() / lResourcePrecision));
	}
	return nCompare;
}

void PLHostClass::SortHosts()
{
	oaHosts.SetCompareFunction(CompareHosts);
	oaHosts.Sort();
}

PLSolutionResources* PLHostClass::SaturateResource(const RMTaskResourceRequirement* taskRequirements,
						   int nProcNumberOnHost, int nProcNumberOnCluster,
						   boolean bIsSequential) const
{
	double dPercentage;
	longint lMasterMin;          // Exigence
	longint lSlaveMin;           // Exigence
	longint lGlobalMin;          // Exigence
	longint lSharedMin;          // Exigence
	longint lMasterMax;          // Exigence
	longint lSlaveMax;           // Exigence
	longint lGlobalMax;          // Exigence
	longint lSharedMax;          // Exigence
	longint lHostOverallMin;     // Exigences minimales sur cette machine
	longint lHostOverallMax;     // Exigences maximales sur cette machine
	longint lHostResource;       // Ressources physiques disponibles sur cette machine
	longint lHostUsableResource; // Ressources Physiques disponibles en prenant en compte les exigences max
	longint lHostExtraResource;  // Ressources a distribuer apres avoir alloue le min
	longint lResourceFull; // Ressources maximales qu'on peut allouer en prenant en compte les resources diponibles
	longint lResourceBounded; // Ressources maximales qu'on peut allouer en prenant en compte les exigences max
	int nSlaveNumberOnCluster;
	int nSlaveNumberOnHost;
	int nMasterNumberOnHost;
	PLSolutionResources* resources;
	boolean bBalanced;
	boolean bMasterPreferred;
	boolean bSlavePreferred;
	boolean bGlobalPreferred;
	int nRT;

	if (IsMasterClass())
	{
		nMasterNumberOnHost = 1;
		nSlaveNumberOnHost = nProcNumberOnHost - 1;
	}
	else
	{
		nMasterNumberOnHost = 0;
		nSlaveNumberOnHost = nProcNumberOnHost;
	}

	nSlaveNumberOnCluster = nProcNumberOnCluster;
	if (nSlaveNumberOnCluster > 1)
		nSlaveNumberOnCluster--;

	if (bIsSequential)
	{
		nMasterNumberOnHost = 1;
		nSlaveNumberOnHost = 1;
		nSlaveNumberOnCluster = 1;
	}

	resources = new PLSolutionResources;
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		lHostResource = GetDefinition()->GetResourceMin()->GetValue(nRT);
		lHostResource = min(lHostResource, RMResourceConstraints::GetResourceLimit(nRT) * lMB);
		lHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lHostResource);

		bBalanced = taskRequirements->GetResourceAllocationPolicy(nRT) == RMTaskResourceRequirement::balanced;
		bMasterPreferred =
		    taskRequirements->GetResourceAllocationPolicy(nRT) == RMTaskResourceRequirement::masterPreferred and
		    nMasterNumberOnHost > 0;
		bSlavePreferred =
		    taskRequirements->GetResourceAllocationPolicy(nRT) == RMTaskResourceRequirement::slavePreferred and
		    nSlaveNumberOnHost > 0;
		bGlobalPreferred =
		    taskRequirements->GetResourceAllocationPolicy(nRT) == RMTaskResourceRequirement::globalPreferred and
		    nSlaveNumberOnHost > 0;
		lMasterMax = taskRequirements->GetMasterMax(nRT);
		lSlaveMax = taskRequirements->GetSlaveMax(nRT);
		lGlobalMax = taskRequirements->GetSlaveGlobalMax(nRT);
		lSharedMax = taskRequirements->GetSharedMax(nRT);
		lMasterMin = taskRequirements->GetMasterMin(nRT);
		lSlaveMin = taskRequirements->GetSlaveMin(nRT);
		lGlobalMin = taskRequirements->GetSlaveGlobalMin(nRT);
		lSharedMin = taskRequirements->GetSharedMin(nRT);

		// Pour eviter les problemes d'infini dans les calculs (INF+INF=INF) On borne les max par la memoire
		// disonible
		if (lSharedMax == LLONG_MAX)
			lSharedMax = lHostResource;
		if (lGlobalMax == LLONG_MAX)
			lGlobalMax = lHostResource;
		if (lMasterMax == LLONG_MAX)
			lMasterMax = lHostResource;
		if (lSlaveMax == LLONG_MAX)
			lSlaveMax = lHostResource;

		// Calcul de la somme ressources min et max
		if (bIsSequential)
		{
			lHostOverallMin = lMasterMin + lSlaveMin + lSharedMin + lGlobalMin +
					  RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT);
			lHostOverallMax = lsum(lMasterMax, lSlaveMax, lSharedMax, lGlobalMax,
					       RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT));
		}
		else
		{
			lHostOverallMin =
			    nMasterNumberOnHost *
				(lMasterMin + RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT) +
				 lSharedMin) +
			    nSlaveNumberOnHost *
				(lSlaveMin + RMParallelResourceManager::GetSlaveHiddenResource(bIsSequential, nRT) +
				 lGlobalMin / nSlaveNumberOnCluster + lSharedMin);
			lHostOverallMax = lsum(
			    lprod((nSlaveNumberOnHost + nMasterNumberOnHost), lSharedMax),
			    lprod(nSlaveNumberOnHost, lGlobalMax / nSlaveNumberOnCluster),
			    lprod(nMasterNumberOnHost,
				  lsum(lMasterMax,
				       RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT))),
			    lprod(nSlaveNumberOnHost, lsum(lSlaveMax, RMParallelResourceManager::GetSlaveHiddenResource(
									  bIsSequential, nRT))));
		}

		assert(lHostOverallMin <= lHostOverallMax);

		// On borne les ressources disponibles par le max qu'on veut allouer
		lHostUsableResource = min(lHostResource, lHostOverallMax);

		// On sature completement une exigence suivant les preferences
		// Note : lors du calcul des resources disponibles pour les esclaves, on effectue une division entiere.
		// Le reste de cette division n'est donc pas alloue, c'est pourquoi il y a une tolerance dans les assertions.
		//
		if (not bBalanced)
		{
			if (bGlobalPreferred and nSlaveNumberOnHost > 0)
			{
				lHostExtraResource = lHostUsableResource - lHostOverallMin;
				assert(lHostExtraResource >= 0);
				lResourceFull = (lGlobalMin + lHostExtraResource) / nSlaveNumberOnHost;
				lResourceBounded = lGlobalMax / nSlaveNumberOnCluster;

				resources->SetGlobalResource(nRT,
							     min((lGlobalMin + lHostExtraResource) / nSlaveNumberOnHost,
								 lGlobalMax / nSlaveNumberOnCluster));
				lHostUsableResource -=
				    nSlaveNumberOnHost * resources->GetGlobalResource()->GetValue(nRT);
				assert(lHostUsableResource > 0);

				// Les ressources globales sont saturees, on met a jour les ressources min et max sans
				// prendre en compte les ressources globales pour calculer le pourcentage de repartition
				// des autres ressources
				lHostOverallMin -= nSlaveNumberOnHost * lGlobalMin / nSlaveNumberOnCluster;
				lHostOverallMax -= nSlaveNumberOnHost * lGlobalMax / nSlaveNumberOnCluster;

				// Les ressources disponibles doivent etre inferieures aux exigences max.
				// Pour cette verification, on prend en compte le reste de la division entiere qui n'a pas ete alloue.
				assert(lHostUsableResource - nSlaveNumberOnCluster <= lHostOverallMax);
			}

			if (bMasterPreferred and nMasterNumberOnHost > 0)
			{
				lHostExtraResource = lHostUsableResource - lHostOverallMin;
				assert(lHostExtraResource >= 0);

				resources->SetMasterResource(nRT, min(lMasterMin + lHostExtraResource, lMasterMax));
				lHostUsableResource -= resources->GetMasterResource()->GetValue(nRT);
				assert(lHostUsableResource >= 0);

				// Mise a jour des ressources min et max pour calculer le pourcentage de repartion des
				// autres ressources
				lHostOverallMin -= lMasterMin;
				lHostOverallMax -= lMasterMax;

				assert(lHostUsableResource <= lHostOverallMax);
			}
			if (bSlavePreferred and nSlaveNumberOnHost > 0)
			{
				lHostExtraResource = lHostUsableResource - lHostOverallMin;
				assert(lHostExtraResource >= 0);

				resources->SetSlaveResource(
				    nRT, min(lSlaveMin + lHostExtraResource / nSlaveNumberOnHost, lSlaveMax));
				lHostUsableResource -=
				    nSlaveNumberOnHost * resources->GetSlaveResource()->GetValue(nRT);

				assert(lHostUsableResource >= 0);

				// Mise a jour des ressources min et max pour calculer le pourcentage de repartion des
				// autres ressources
				lHostOverallMin = lHostOverallMin - nSlaveNumberOnHost * lSlaveMin;
				lHostOverallMax = lHostOverallMax - nSlaveNumberOnHost * lSlaveMax;

				// Les ressources disponibles doivent etre inferieures aux exigences max.
				// Pour cette verification, on prend en compte le reste de la division entiere qui n'a pas ete alloue.
				assert(lHostUsableResource - nSlaveNumberOnHost <= lHostOverallMax);
			}
		}

		assert(lHostOverallMin <= lHostOverallMax);
		assert(lHostUsableResource >= 0);
		assert(lHostUsableResource >= lHostOverallMin);

		// Calcul du pourcentage de repartition : on cherche a attribuer une saturation proportionnelle aux
		// exigences et donner le meme pourcentage a toutes les exigences
		if (lHostOverallMax == lHostOverallMin)
		{
			// Dans le cas ou les max == les mins, la saturation n'est pas necessaire
			dPercentage = 0;
		}
		else
		{
			// Il peut y avoir une difference due aux restes da la division entiere.
			// On corrige la difference en bornant le pourcentage a 1
			assert(lHostUsableResource <= lHostOverallMax + nSlaveNumberOnCluster);
			dPercentage =
			    (lHostUsableResource - lHostOverallMin) * 1.0 / (lHostOverallMax - lHostOverallMin);
			if (dPercentage > 1)
				dPercentage = 1;
		}

		assert(dPercentage <= 1 and dPercentage >= 0);

		//////////////////////////////////////////////////////
		// Saturation des ressources pour ce pourcentage
		if (not bMasterPreferred)
		{
			if (nMasterNumberOnHost == 1)
			{
				resources->SetMasterResource(
				    nRT, lsum(lMasterMin, longint(dPercentage * (lMasterMax - lMasterMin))));
			}
			else
			{
				assert(resources->GetMasterResource()->GetValue(nRT) == 0);
			}
		}
		if (not bSlavePreferred)
		{
			if (nSlaveNumberOnHost > 0)
				resources->SetSlaveResource(
				    nRT, lsum(lSlaveMin, longint(dPercentage * (lSlaveMax - lSlaveMin))));
			else
			{
				assert(resources->GetSlaveResource()->GetValue(nRT) == 0);
			}
		}
		if (not bGlobalPreferred)
		{
			if (nSlaveNumberOnHost > 0)
				resources->SetGlobalResource(
				    nRT, lsum(lGlobalMin, longint(dPercentage * (lGlobalMax - lGlobalMin))) /
					     nSlaveNumberOnCluster);
			else
			{
				assert(resources->GetGlobalResource()->GetValue(nRT) == 0);
			}
		}

		resources->SetSharedResource(nRT, lsum(lSharedMin, longint(dPercentage * (lSharedMax - lSharedMin))));

		assert(nMasterNumberOnHost == 0 or
		       resources->GetMasterResource()->GetValue(nRT) <= taskRequirements->GetMasterMax(nRT));
		assert(nSlaveNumberOnHost == 0 or
		       resources->GetSlaveResource()->GetValue(nRT) <= taskRequirements->GetSlaveMax(nRT));
		assert(nSlaveNumberOnHost == 0 or resources->GetGlobalResource()->GetValue(nRT) <=
						      taskRequirements->GetSlaveGlobalMax(nRT) / nSlaveNumberOnCluster);
		assert(nMasterNumberOnHost == 0 or
		       resources->GetMasterResource()->GetValue(nRT) >= taskRequirements->GetMasterMin(nRT));
		assert(nSlaveNumberOnHost == 0 or
		       resources->GetSlaveResource()->GetValue(nRT) >= taskRequirements->GetSlaveMin(nRT));
		assert(nSlaveNumberOnHost == 0 or resources->GetGlobalResource()->GetValue(nRT) >=
						      taskRequirements->GetSlaveGlobalMin(nRT) / nSlaveNumberOnCluster);

		if (nMasterNumberOnHost == 0)
		{
			resources->SetMasterResource(nRT, -1);
		}

		if (nSlaveNumberOnHost > 0)
		{
			assert(resources->GetSlaveResource()->GetValue(nRT) >= lSlaveMin or bSlavePreferred);
			assert(resources->GetSlaveResource()->GetValue(nRT) <= lSlaveMax or bSlavePreferred);

			// On borne par le min a cause de problemes d'arrondi
			resources->SetGlobalResource(nRT, max(resources->GetGlobalResource()->GetValue(nRT),
							      lGlobalMin / nSlaveNumberOnCluster));
			assert(resources->GetGlobalResource()->GetValue(nRT) <= lGlobalMax / nSlaveNumberOnCluster or
			       bGlobalPreferred);
		}
		else
		{
			resources->SetSlaveResource(nRT, -1);
			resources->SetGlobalResource(nRT, -1);
		}
	}
	return resources;
}

void PLHostClass::Write(ostream& ost) const
{
	definition->Write(ost);
	ost << "#hosts " << IntToString(oaHosts.GetSize()) << endl;
}

boolean PLHostClass::Check() const
{
	int i;
	RMHostResource* host;
	boolean bOk;

	bOk = true;
	bOk = (not IsMasterClass() or oaHosts.GetSize() == 1) and bOk;
	ensure(bOk);
	for (i = 0; i < oaHosts.GetSize(); i++)
	{
		host = cast(RMHostResource*, oaHosts.GetAt(i));
		bOk = host->GetResourceFree(MEMORY) >= definition->GetMemoryMin() and bOk;
		ensure(bOk);
		bOk = host->GetResourceFree(MEMORY) < definition->GetMemoryMax() and bOk;
		ensure(bOk);
		bOk = host->GetResourceFree(DISK) >= definition->GetDiskMin() and bOk;
		ensure(bOk);
		bOk = host->GetResourceFree(DISK) < definition->GetDiskMax() and bOk;
		ensure(bOk);
		bOk = host->GetLogicalProcessNumber() == definition->GetProcNumber() and bOk;
		ensure(bOk);
	}
	return bOk;
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLHostClassSolution

PLHostClassSolution::PLHostClassSolution()
{
	hostClass = new PLHostClass;
}

PLHostClassSolution::~PLHostClassSolution()
{
	delete hostClass;
}

PLHostClassSolution* PLHostClassSolution::Clone() const
{
	PLHostClassSolution* clone;
	clone = new PLHostClassSolution;
	clone->CopyFrom(this);
	return clone;
}

void PLHostClassSolution::CopyFrom(const PLHostClassSolution* solutionSource)
{
	hostClass->CopyFrom(solutionSource->hostClass);
	ivHostCountPerProcNumber.CopyFrom(&solutionSource->ivHostCountPerProcNumber);
}

void PLHostClassSolution::SetHostClass(const PLHostClass* host)
{
	require(host->Check());
	hostClass->CopyFrom(host);
	if (PLParallelTask::GetParallelSimulated())
		ivHostCountPerProcNumber.SetSize(PLParallelTask::GetSimulatedSlaveNumber() + 2);
	else
		ivHostCountPerProcNumber.SetSize(host->GetDefinition()->GetProcNumber() + 1);
	ivHostCountPerProcNumber.Initialize();
	ivHostCountPerProcNumber.SetAt(0, host->GetHostNumber());
}

void PLHostClassSolution::SetHostCountPerProcNumber(const IntVector* ivValue)
{
	require(ivValue->GetSize() == ivHostCountPerProcNumber.GetSize());
	ivHostCountPerProcNumber.CopyFrom(ivValue);
	ensure(Check());
}

boolean PLHostClassSolution::AllowsOnMoreProc(int nProcHost) const
{
	int i;
	const boolean bVerbose = false;
	if (bVerbose)
	{
		cout << "AllowsOnMoreProc ";
		for (i = 0; i < ivHostCountPerProcNumber.GetSize(); i++)
			cout << ivHostCountPerProcNumber.GetAt(i) << " ";
	}
	if (ivHostCountPerProcNumber.GetAt(nProcHost) != 0 and   // Si il y a des machines qui ont nProcHost procs
	    nProcHost != ivHostCountPerProcNumber.GetSize() - 1) // que nProcHost n'est pas le nombre max de proc
	{
		if (bVerbose)
			cout << " OK" << endl << endl;
		return true;
	}
	if (bVerbose)
	{
		cout << " FAILED" << endl << endl;
	}
	return false;
}

void PLHostClassSolution::SortHosts()
{
	hostClass->SortHosts();
}

void PLHostClassSolution::Write(ostream& ost) const
{
	const boolean bVerbose = false;
	int i;
	hostClass->Write(ost);
	for (i = 0; i < ivHostCountPerProcNumber.GetSize(); i++)
	{
		if (ivHostCountPerProcNumber.GetAt(i) != 0)
			ost << IntToString(ivHostCountPerProcNumber.GetAt(i)) << " hosts with " << IntToString(i)
			    << " procs" << endl;
	}
	ost << endl;
	if (bVerbose)
	{
		ost << "\t";
		for (i = 0; i < ivHostCountPerProcNumber.GetSize(); i++)
		{
			ost << ivHostCountPerProcNumber.GetAt(i) << " ";
		}
		ost << endl;
	}
}

boolean PLHostClassSolution::Check() const
{
	int i;
	int nTotalHosts;
	boolean bOk;

	require(hostClass != NULL);
	bOk = hostClass->Check();
	ensure(bOk);
	bOk = (PLParallelTask::GetParallelSimulated() or
	       ivHostCountPerProcNumber.GetSize() == hostClass->GetDefinition()->GetProcNumber() + 1) and
	      bOk;
	ensure(bOk);

	// On verifie qu ele nombr de machine de la solution correspond au nombre de machines de la classe
	nTotalHosts = 0;
	for (i = 0; i < ivHostCountPerProcNumber.GetSize(); i++)
	{
		nTotalHosts += ivHostCountPerProcNumber.GetAt(i);
	}
	bOk = nTotalHosts == hostClass->GetHostNumber() and bOk;
	ensure(bOk);

	return bOk;
}

void PLHostClassSolution::PrintHostCountPerProcNumber() const
{
	int i;
	for (i = 0; i < ivHostCountPerProcNumber.GetSize(); i++)
	{
		cout << ivHostCountPerProcNumber.GetAt(i) << " ";
	}
	cout << endl;
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLSolution

PLSolution::PLSolution()
{
	nUsedHost = 0;
	nUsedProcs = 0;
	bIsEvaluated = false;
	taskRequirements = NULL;
	bGlobalConstraintIsValid = false;
	resources = new PLSolutionResources;
	oldResources = new PLSolutionResources;
	rcMissingResources = new RMResourceContainer;
	nMasterNumber = 0;
	bLastIsAdd = false;
	bOldGlobalConstraintIsValid = false;
}

PLSolution::~PLSolution()
{
	oaHostClassSolution.DeleteAll();
	taskRequirements = NULL;
	delete resources;
	delete oldResources;
	delete rcMissingResources;
}

int PLSolution::CompareTo(const PLSolution* otherSolution) const
{

	int nCompare;
	int nRT;
	require(bIsEvaluated and otherSolution->bIsEvaluated);

	if (not bGlobalConstraintIsValid and not otherSolution->bGlobalConstraintIsValid)
	{
		return 0;
	}
	if (not bGlobalConstraintIsValid)
	{
		return -1;
	}
	if (not otherSolution->bGlobalConstraintIsValid)
	{
		return 1;
	}

	// Nombre de processus utilises
	nCompare = nUsedProcs - otherSolution->nUsedProcs;

	// Etalement : on privilegie la parallelisation horizontale
	if (nCompare == 0)
	{
		nCompare = nUsedHost - otherSolution->nUsedHost;

		// Sauf si la politique de parallelisation est vertical
		if (taskRequirements->GetParallelisationPolicy() == RMTaskResourceRequirement::vertical)
		{
			nCompare = -nCompare;
		}
	}

	// On compare les ressources suivant la politique d'allocation en commencant implicitement par la memoire
	if (nCompare == 0)
	{
		for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
		{
			switch (taskRequirements->GetResourceAllocationPolicy(nRT))
			{
			case RMTaskResourceRequirement::ALLOCATION_POLICY::slavePreferred:
				nCompare = CompareLongint(resources->GetSlaveResource()->GetValue(nRT),
							  otherSolution->resources->GetSlaveResource()->GetValue(nRT));
				break;
			case RMTaskResourceRequirement::ALLOCATION_POLICY::masterPreferred:
				nCompare = CompareLongint(resources->GetMasterResource()->GetValue(nRT),
							  otherSolution->resources->GetMasterResource()->GetValue(nRT));
				break;

			case RMTaskResourceRequirement::ALLOCATION_POLICY::globalPreferred:
				nCompare = CompareLongint(resources->GetGlobalResource()->GetValue(nRT),
							  otherSolution->resources->GetGlobalResource()->GetValue(nRT));
				break;
			case RMTaskResourceRequirement::ALLOCATION_POLICY::balanced:
				// Dans la cas balanced on compare les sommes des ressources (on sait que le nombre de
				// proc est le meme pour les 2 solutions)
				nCompare =
				    CompareLongint(lsum(resources->GetSlaveResource()->GetValue(nRT),
							resources->GetMasterResource()->GetValue(nRT),
							resources->GetGlobalResource()->GetValue(nRT)),
						   lsum(otherSolution->resources->GetSlaveResource()->GetValue(nRT),
							otherSolution->resources->GetMasterResource()->GetValue(nRT),
							otherSolution->resources->GetGlobalResource()->GetValue(nRT)));
				break;
			default:
				assert(false);
			}
			if (nCompare != 0)
				break;
		}
	}

	// Si les politiques d'allocation ne permettent pas de trancher, on compare comme dans la politique balanced
	if (nCompare == 0)
	{
		for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
		{
			nCompare = CompareLongint(lsum(resources->GetSlaveResource()->GetValue(nRT),
						       resources->GetMasterResource()->GetValue(nRT),
						       resources->GetGlobalResource()->GetValue(nRT)),
						  lsum(otherSolution->resources->GetSlaveResource()->GetValue(nRT),
						       otherSolution->resources->GetMasterResource()->GetValue(nRT),
						       otherSolution->resources->GetGlobalResource()->GetValue(nRT)));

			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

PLSolution* PLSolution::Clone() const
{
	PLSolution* clone;
	clone = new PLSolution;
	clone->CopyFrom(this);
	return clone;
}

void PLSolution::CopyFrom(const PLSolution* solutionSource)
{
	int i;
	oaHostClassSolution.DeleteAll();
	for (i = 0; i < solutionSource->oaHostClassSolution.GetSize(); i++)
	{
		oaHostClassSolution.Add(
		    cast(PLHostClassSolution*, solutionSource->oaHostClassSolution.GetAt(i))->Clone());
	}
	resources->CopyFrom(solutionSource->resources);
	rcMissingResources->CopyFrom(solutionSource->rcMissingResources);
	bIsEvaluated = solutionSource->bIsEvaluated;
	bGlobalConstraintIsValid = solutionSource->bGlobalConstraintIsValid;
	taskRequirements = solutionSource->taskRequirements;
	nUsedProcs = solutionSource->nUsedProcs;
	nUsedHost = solutionSource->nUsedHost;
	nMasterNumber = solutionSource->nMasterNumber;
}

void PLSolution::AddHostSolution(PLHostClassSolution* classSolution)
{
	require(classSolution->Check());
	oaHostClassSolution.Add(classSolution);
}

int CompareHostClassSolution(const void* elem1, const void* elem2)
{
	PLHostClassSolution* solution1;
	PLHostClassSolution* solution2;
	int nCompare = 0;

	check(elem1);
	check(elem2);

	// Acces aux objets
	solution1 = cast(PLHostClassSolution*, *(Object**)elem1);
	solution2 = cast(PLHostClassSolution*, *(Object**)elem2);

	require(not solution1->GetHostClass()->IsMasterClass() or not solution2->GetHostClass()->IsMasterClass());

	// La classe qui contient la machine maitre est la plus petite,
	// ensuite on compare le nombre de processeurs, la memoire, puis le disque,
	if (solution1->GetHostClass()->IsMasterClass())
		nCompare = -1;
	else if (solution2->GetHostClass()->IsMasterClass())
		nCompare = 1;
	else
	{
		nCompare = CompareLongint(solution1->GetHostClass()->GetDefinition()->GetProcNumber(),
					  solution2->GetHostClass()->GetDefinition()->GetProcNumber());
		if (nCompare == 0)
			nCompare = CompareLongint(solution1->GetHostClass()->GetDefinition()->GetMemoryMin(),
						  solution2->GetHostClass()->GetDefinition()->GetMemoryMin());
		if (nCompare == 0)
			nCompare = CompareLongint(solution1->GetHostClass()->GetDefinition()->GetDiskMin(),
						  solution2->GetHostClass()->GetDefinition()->GetDiskMin());
	}

	assert(nCompare != 0);
	return nCompare;
}

void PLSolution::SortHostClasses()
{
	oaHostClassSolution.SetCompareFunction(CompareHostClassSolution);
	oaHostClassSolution.Sort();
}

void PLSolution::SetTaskRequirements(const RMTaskResourceRequirement* requirements)
{
	taskRequirements = requirements;
}

const RMTaskResourceRequirement* PLSolution::GetTaskRequirements() const
{
	return taskRequirements;
}

void PLSolution::Evaluate(boolean bIsSequential)
{
	int i;
	int j;
	PLHostClassSolution* hostSolution;
	IntVector ivMasterProcHost;
	PLHostClassSolution* hostClassSolution;
	const PLHostClass* hostClass;

	bIsEvaluated = true;

	// Nombre de machines utilisees
	nUsedHost = 0;
	nUsedProcs = 0;
	for (i = 0; i < oaHostClassSolution.GetSize(); i++)
	{
		hostSolution = GetHostSolutionAt(i);

		// Nombre de machines - nombre de machine qui ont 0 processus
		nUsedHost +=
		    hostSolution->GetHostClass()->GetHostNumber() - hostSolution->GetHostCountPerProcNumber()->GetAt(0);
		ensure(nUsedHost >= 0);

		// Nombre de process
		for (j = 0; j < hostSolution->GetHostCountPerProcNumber()->GetSize(); j++)
		{
			nUsedProcs += j * hostSolution->GetHostCountPerProcNumber()->GetAt(j);
		}
	}

	// Verification qu'il y a au moins un processus sur la machine maitre
	for (i = 0; i < GetHostClassSolutionNumber(); i++)
	{
		hostClassSolution = GetHostSolutionAt(i);
		hostClass = hostClassSolution->GetHostClass();
		if (hostClass->IsMasterClass())
		{
			bGlobalConstraintIsValid =
			    hostClassSolution->GetHostCountPerProcNumber()->GetAt(0) != hostClass->GetHostNumber();
			break;
		}
	}

	// Verification des contraintes sur le nombre de processus
	if (bGlobalConstraintIsValid)
		bGlobalConstraintIsValid = FitProcessNumber();

	// Ressources utilisees
	bGlobalConstraintIsValid = FitResources(bIsSequential);
	if (bGlobalConstraintIsValid)
	{
		SaturateResource(bIsSequential);
	}
}

int PLSolution::GetUsedHostNumber() const
{
	require(bIsEvaluated);
	return nUsedHost;
}

const RMResourceContainer* PLSolution::GetMissingResources() const
{
	require(bIsEvaluated);
	return rcMissingResources;
}

PLSolution* PLSolution::Expand() const
{
	PLSolution* lastSolution;
	RMHostResource* host;
	PLHostClass* hostClass;
	PLHostClassSolution* hostClassSolution;
	PLHostClassSolution* finalHostClassSolution;
	const IntVector* ivHostCountPerProcNumber;
	IntVector ivFinalHostCountPerProcNumber;
	int i;
	int j;
	int nProcNumber;
	int nHostNumber;
	int nHostIndex;

	require(Check());
	lastSolution = new PLSolution;
	lastSolution->SetTaskRequirements(GetTaskRequirements());
	for (j = 0; j < GetHostClassSolutionNumber(); j++)
	{
		nHostIndex = 0;
		hostClassSolution = GetHostSolutionAt(j);
		ivHostCountPerProcNumber = hostClassSolution->GetHostCountPerProcNumber();
		for (nProcNumber = 1; nProcNumber < ivHostCountPerProcNumber->GetSize(); nProcNumber++)
		{
			nHostNumber = ivHostCountPerProcNumber->GetAt(nProcNumber);

			// Tri decroissant des hosts sur les ressources pour choisir les grosses machines en premier
			hostClassSolution->SortHosts();

			for (i = 0; i < nHostNumber; i++)
			{
				host = cast(RMHostResource*,
					    hostClassSolution->GetHostClass()->GetHosts()->GetAt(nHostIndex));
				nHostIndex++;

				// Construction de la classe singleton
				hostClass = new PLHostClass;
				hostClass->AddHost(host);
				hostClass->FitDefinition();
				ensure(hostClass->Check());

				// Affectation du nombre de processus instancies
				if (PLParallelTask::GetParallelSimulated())
					ivFinalHostCountPerProcNumber.SetSize(
					    PLParallelTask::GetSimulatedSlaveNumber() + 2);
				else
					ivFinalHostCountPerProcNumber.SetSize(host->GetLogicalProcessNumber() + 1);

				ivFinalHostCountPerProcNumber.Initialize();
				ivFinalHostCountPerProcNumber.SetAt(nProcNumber, 1);
				finalHostClassSolution = new PLHostClassSolution;
				finalHostClassSolution->SetHostClass(hostClass);
				finalHostClassSolution->SetHostCountPerProcNumber(&ivFinalHostCountPerProcNumber);
				delete hostClass;

				// Ajout a la solution
				lastSolution->AddHostSolution(finalHostClassSolution);
			}
		}
	}
	lastSolution->nUsedHost = nUsedHost;
	lastSolution->nUsedProcs = nUsedProcs;
	return lastSolution;
}

boolean PLSolution::IsValid() const
{
	require(bIsEvaluated);
	return bGlobalConstraintIsValid;
}

void PLSolution::Write(ostream& ost) const
{
	int i;
	ost << "Solution:" << endl;
	cout << "#procs " << GetUsedProcessNumber() << endl;
	cout << "#hosts " << GetUsedHostNumber() << endl;
	cout << "valid: " << BooleanToString(IsValid()) << endl;
	cout << *resources << endl;
	for (i = 0; i < GetHostClassSolutionNumber(); i++)
	{
		GetHostSolutionAt(i)->Write(ost);
		ost << endl;
	}
}

boolean PLSolution::Check() const
{
	int i;
	boolean bOk;
	for (i = 0; i < GetHostClassSolutionNumber(); i++)
	{
		bOk = GetHostSolutionAt(i)->Check();
		if (not bOk)
			return false;
	}
	return true;
}

boolean PLSolution::FitMinimalRequirements(int nRT, longint& lMissingResource, boolean bIsSequential) const
{
	ALString sHostName;
	PLHostClassSolution* hostClassSolution;
	const PLHostClass* hostClass;
	boolean bResourceIsMissing;
	int nSlaveNumberOnCluster;
	int i;
	int nProcNumber;
	IntVector ivResourcesMissingForProcNumber;
	LongintVector lvResourceMissing;
	boolean bResourceOkForMaster;
	longint lLocalMissingResource;
	longint lUsedResource;
	longint lHostUsableResource;
	longint lMasterSum;
	longint lSlaveMin;

	// Nombre d'esclave utilises sur le cluster
	nSlaveNumberOnCluster = GetUsedProcessNumber();
	if (nSlaveNumberOnCluster == 0)
		return true;
	if (nSlaveNumberOnCluster > 1)
		nSlaveNumberOnCluster--;

	// TODO lMissingResource contient les ressources manquantes de la premiere machine
	// est-ce qu'on souhaite avoir la somme des ressources manquantes ou la liste des
	// ressources manquantes (peut etre le nom de la machine)
	lMissingResource = LLONG_MAX;
	bResourceIsMissing = false;
	bResourceOkForMaster = false;
	lvResourceMissing.SetSize(0);

	// Cas sequentiel : il n'y a qu'une classe de machines
	if (bIsSequential)
	{
		assert(GetHostClassSolutionNumber() == 1);
		hostClassSolution = GetHostSolutionAt(0);
		hostClass = hostClassSolution->GetHostClass();
		assert(hostClass->IsMasterClass());
		ivResourcesMissingForProcNumber.SetSize(0);
		lHostUsableResource = hostClass->GetAvaiblableResource(nRT);
		lUsedResource = taskRequirements->GetMasterMin(nRT) + taskRequirements->GetSlaveMin(nRT) +
				taskRequirements->GetSharedMin(nRT) + taskRequirements->GetSlaveGlobalMin(nRT) +
				RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT);
		lLocalMissingResource = lUsedResource - lHostUsableResource;
		if (lLocalMissingResource > 0)
		{
			ivResourcesMissingForProcNumber.Add(1);
			lvResourceMissing.Add(lLocalMissingResource);
			bResourceIsMissing = true;
		}
	}
	else
	// Cas parallele
	{

		// Precalcul des ressources utilises par le maitre et chaque esclave avant la boucle pour plus
		// d'efficacite
		lMasterSum = taskRequirements->GetMasterMin(nRT) +
			     RMParallelResourceManager::GetMasterHiddenResource(bIsSequential, nRT) +
			     taskRequirements->GetSharedMin(nRT);
		lSlaveMin = taskRequirements->GetSlaveMin(nRT) +
			    RMParallelResourceManager::GetSlaveHiddenResource(bIsSequential, nRT) +
			    taskRequirements->GetSlaveGlobalMin(nRT) / nSlaveNumberOnCluster +
			    taskRequirements->GetSharedMin(nRT);

		// Iteration sur les classes de machines (PLHostClass)
		for (i = 0; i < GetHostClassSolutionNumber(); i++)
		{
			hostClassSolution = GetHostSolutionAt(i);
			hostClass = hostClassSolution->GetHostClass();
			ivResourcesMissingForProcNumber.SetSize(0);
			lHostUsableResource = hostClass->GetAvaiblableResource(nRT);

			// Pour etre plus efficace, on ecrit du code specifique dans le cas de la classe qui contient le
			// maitre dans l'autre cas, la classe ne contient que des esclaves
			if (hostClassSolution->GetHostClass()->IsMasterClass())
			{
				// Iteration sur le nombre de processus instancies
				for (nProcNumber = 1;
				     nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize();
				     nProcNumber++)
				{
					// Si il y a au moins une machine qui a nProcNumber procs
					if (hostClassSolution->GetHostCountPerProcNumber()->GetAt(nProcNumber) != 0)
					{
						// Resources necessaires sur ce host
						lUsedResource = lMasterSum + ((longint)nProcNumber - 1) * lSlaveMin;

						// Ressources disponibles sur le host
						lLocalMissingResource = lUsedResource - lHostUsableResource;
						if (lLocalMissingResource > 0)
						{
							ivResourcesMissingForProcNumber.Add(nProcNumber);
							lvResourceMissing.Add(lLocalMissingResource);
							bResourceIsMissing = true;
						}
					}
				}
			}
			else
			{
				// Classe sans maitre

				// Iteration sur le nombre de processus instancies
				for (nProcNumber = 1;
				     nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize();
				     nProcNumber++)
				{
					// Si il y a au moins une machine qui a nProcNumber procs
					if (hostClassSolution->GetHostCountPerProcNumber()->GetAt(nProcNumber) != 0)
					{
						// Resources necessaires sur ce host
						lUsedResource = nProcNumber * lSlaveMin;

						// Ressources disponibles sur le host
						lLocalMissingResource = lUsedResource - lHostUsableResource;
						if (lLocalMissingResource > 0)
						{
							ivResourcesMissingForProcNumber.Add(nProcNumber);
							lvResourceMissing.Add(lLocalMissingResource);
							bResourceIsMissing = true;
							break;
						}
					}
				}
			}
			if (bResourceIsMissing)
				break;
		}
	}
	if (bResourceIsMissing)
	{
		lvResourceMissing.Sort();
		lMissingResource = lvResourceMissing.GetAt(0);
	}

	// On ne renvoie que les ressources manquantes (pas les ressources disponibles)
	if (not bResourceIsMissing)
		lMissingResource = 0;
	return not bResourceIsMissing;
}

boolean PLSolution::FitResources(boolean bIsSequential)
{

	ALString sHostName;
	longint lMissingResource;
	int nRT;
	boolean bFitGlobalConstraint;

	bFitGlobalConstraint = true;

	// Verification de l'exigence GetGlobalSlaveRequirement
	for (nRT = 0; nRT < RESOURCES_NUMBER; nRT++)
	{
		// Verification, qu'il y a assez de ressource sur chaque host
		bFitGlobalConstraint = FitMinimalRequirements(nRT, lMissingResource, bIsSequential);
		if (not bFitGlobalConstraint)
		{
			rcMissingResources->SetValue(nRT, lMissingResource);
			break;
		}
	}
	assert(bFitGlobalConstraint or rcMissingResources->GetValue(MEMORY) > 0 or
	       rcMissingResources->GetValue(DISK) > 0);

	return bFitGlobalConstraint;
}

boolean PLSolution::FitProcessNumber() const
{
	int nExtraProcessNumber1;
	int nExtraProcessNumber2;
	int nExtraProcNumber;

	if (PLParallelTask::GetParallelSimulated())
		nExtraProcessNumber1 = GetUsedProcessNumber() - PLParallelTask::GetSimulatedSlaveNumber() - 1;
	else
		nExtraProcessNumber1 = GetUsedProcessNumber() - RMResourceConstraints::GetMaxCoreNumberOnCluster();
	nExtraProcessNumber2 = GetUsedProcessNumber() - 1 - taskRequirements->GetMaxSlaveProcessNumber();
	nExtraProcNumber = max(nExtraProcessNumber1, nExtraProcessNumber2);

	if (nExtraProcNumber > 0)
		return false;
	return true;
}

void PLSolution::SaturateResource(boolean bIsSequential)
{
	int i;
	PLHostClassSolution* hostClassSolution;
	const PLHostClass* hostClass;
	int nProcNumber;
	int nHostNumber;
	PLSolutionResources* localResources;

	resources->SetSlaveResource(MEMORY, LLONG_MAX);
	resources->SetGlobalResource(MEMORY, LLONG_MAX);
	resources->SetSharedResource(MEMORY, LLONG_MAX);
	resources->SetSlaveResource(DISK, LLONG_MAX);
	resources->SetGlobalResource(DISK, LLONG_MAX);
	resources->SetSharedResource(DISK, LLONG_MAX);

	// Iteration sur les classes de machines (PLHostClass)
	for (i = 0; i < GetHostClassSolutionNumber(); i++)
	{
		hostClassSolution = GetHostSolutionAt(i);
		hostClass = hostClassSolution->GetHostClass();

		// Iteration sur le nombre de processus instancies
		for (nProcNumber = 1; nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize();
		     nProcNumber++)
		{
			nHostNumber = hostClassSolution->GetHostCountPerProcNumber()->GetAt(nProcNumber);

			// Si il y a au moins une machine qui a nProcNumber procs
			if (nHostNumber != 0)
			{

				// Saturation des ressources pour les machines de cette classes et ce nombre de
				// processeurs utilises
				localResources = hostClass->SaturateResource(GetTaskRequirements(), nProcNumber,
									     GetUsedProcessNumber(), bIsSequential);

				// Affectation de la ressource au maitre
				if (localResources->GetMasterResource()->GetValue(MEMORY) != -1)
					resources->GetMasterResource()->CopyFrom(localResources->GetMasterResource());

				// On ne garde que le minimum sur tous les hosts pour avoir les memes ressources
				// allouees a tous les esclaves
				if (localResources->GetSlaveResource()->GetValue(MEMORY) != -1)
				{
					resources->GetSlaveResource()->UpdateMin(localResources->GetSlaveResource());
					resources->GetGlobalResource()->UpdateMin(localResources->GetGlobalResource());
				}
				resources->GetSharedResource()->UpdateMin(localResources->GetSharedResource());
				delete localResources;
			}
		}
	}
}

void PLSolution::AddProcessusOnHost(int nHostClassIndex, int nProcNumber)
{
	PLHostClassSolution* hostClassSolution;
	const boolean bIsSequential = false;
	PLSolutionResources* hostResources;

	bLastIsAdd = true;

	assert(nHostClassIndex < GetHostClassSolutionNumber());
	hostClassSolution = GetHostSolutionAt(nHostClassIndex);

	assert(nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize());

	hostClassSolution->ivHostCountPerProcNumber.UpgradeAt(nProcNumber, -1);
	assert(hostClassSolution->ivHostCountPerProcNumber.GetAt(nProcNumber) >= 0);
	hostClassSolution->ivHostCountPerProcNumber.UpgradeAt(nProcNumber + 1, 1);

	if (nMasterNumber == 0 and hostClassSolution->GetHostClass()->IsMasterClass())
		nMasterNumber = 1;

	// Mise a jour du nombre de machines et du nombre de procesus utilises
	// Si on ajoute un proc sur une machine qui n'en a pas => ajout d'une machine
	if (nProcNumber == 0)
		nUsedHost++;
	nUsedProcs++;

	// Sauvegarde de bGlobalConstraintIsValid  pour la reafecter dans la methode RemoveProcessusOnHost
	bOldGlobalConstraintIsValid = bGlobalConstraintIsValid;

	// Verification des contraintes
	bGlobalConstraintIsValid = true;

	// Verification qu'il y a au moins un processus sur la machine maitre
	if (nMasterNumber == 0)
		bGlobalConstraintIsValid = false;

	// Verification des contraintes sur le nombre de processus
	if (bGlobalConstraintIsValid)
	{
		bGlobalConstraintIsValid = FitProcessNumber();
	}

	// Verification des contraintes sur les ressources
	if (bGlobalConstraintIsValid)
	{
		bGlobalConstraintIsValid = FitResources(bIsSequential);
	}

	// Sauvegarde des ressources courantes pour les reafecter dans la methode RemoveProcessusOnHost
	oldResources->CopyFrom(resources);

	// Mise a jour des ressources
	if (bGlobalConstraintIsValid)
	{
		hostResources = hostClassSolution->GetHostClass()->SaturateResource(taskRequirements, nProcNumber + 1,
										    nUsedProcs, bIsSequential);

		// Affectation de la ressource au maitre
		if (hostResources->GetMasterResource()->GetValue(MEMORY) != -1)
			resources->GetMasterResource()->CopyFrom(hostResources->GetMasterResource());

		// On ne garde que le minimum sur tous les hosts pour avoir les memes ressources allouees
		// a tous les esclaves
		if (hostResources->GetSlaveResource()->GetValue(MEMORY) != -1)
		{
			resources->GetSlaveResource()->UpdateMin(hostResources->GetSlaveResource());
			resources->GetGlobalResource()->UpdateMin(hostResources->GetGlobalResource());
		}
		resources->GetSharedResource()->UpdateMin(hostResources->GetSharedResource());
		delete hostResources;
	}
}

void PLSolution::RemoveProcessusOnHost(int nHostClassIndex, int nProcNumber)
{
	PLHostClassSolution* hostClassSolution;

	assert(bLastIsAdd);
	bLastIsAdd = false;

	assert(nHostClassIndex < GetHostClassSolutionNumber());
	hostClassSolution = GetHostSolutionAt(nHostClassIndex);

	assert(nProcNumber > 0);
	assert(nProcNumber < hostClassSolution->GetHostCountPerProcNumber()->GetSize());

	hostClassSolution->ivHostCountPerProcNumber.UpgradeAt(nProcNumber, -1);
	assert(hostClassSolution->ivHostCountPerProcNumber.GetAt(nProcNumber) >= 0);
	hostClassSolution->ivHostCountPerProcNumber.UpgradeAt(nProcNumber - 1, 1);

	// Sur la classe master, si toutes les machines ont 0 procs, il n'y a plus de master
	if (hostClassSolution->GetHostClass()->IsMasterClass() and nMasterNumber == 1 and
	    hostClassSolution->GetHostCountPerProcNumber()->GetAt(0) ==
		hostClassSolution->GetHostClass()->GetHostNumber())
		nMasterNumber = 0;

	// Mise a jour du nombre de machines et du nombre de procesus utilises
	// Si on supprime un proc sur une machine qui n'en a pas => suppression d'une machine
	if (nProcNumber == 1)
		nUsedHost--;
	nUsedProcs--;

	// On reprend l'information de validite calculee avant le dernier ajout
	bGlobalConstraintIsValid = bOldGlobalConstraintIsValid;

	resources->CopyFrom(oldResources);
}

///////////////////////////////////////////////////////////////////////
// Implementation de PLSolutionResources

// Constructeur
PLSolutionResources::PLSolutionResources()
{
	rcSlaveResource = new RMResourceContainer;
	rcMasterResource = new RMResourceContainer;
	rcSharedResource = new RMResourceContainer;
	rcGlobalResource = new RMResourceContainer;
}

PLSolutionResources::~PLSolutionResources()
{
	delete rcSlaveResource;
	delete rcMasterResource;
	delete rcSharedResource;
	delete rcGlobalResource;
}

PLSolutionResources* PLSolutionResources::Clone() const
{
	PLSolutionResources* clone;
	clone = new PLSolutionResources;
	clone->CopyFrom(this);
	return clone;
}

void PLSolutionResources::CopyFrom(const PLSolutionResources* source)
{
	rcSlaveResource->CopyFrom(source->GetSlaveResource());
	rcMasterResource->CopyFrom(source->GetMasterResource());
	rcSharedResource->CopyFrom(source->GetSharedResource());
	rcGlobalResource->CopyFrom(source->GetGlobalResource());
}

void PLSolutionResources::SetSlaveResource(int nRT, longint lValue)
{
	rcSlaveResource->SetValue(nRT, lValue);
}

void PLSolutionResources::SetMasterResource(int nRT, longint lValue)
{
	rcMasterResource->SetValue(nRT, lValue);
}

void PLSolutionResources::SetSharedResource(int nRT, longint lValue)
{
	rcSharedResource->SetValue(nRT, lValue);
}

void PLSolutionResources::SetGlobalResource(int nRT, longint lValue)
{
	rcGlobalResource->SetValue(nRT, lValue);
}

void PLSolutionResources::Write(ostream& ost) const
{
	ost << "master " << *rcMasterResource << endl;
	ost << "slave  " << *rcSlaveResource << endl;
	ost << "shared " << *rcSharedResource << endl;
	ost << "global " << *rcGlobalResource << endl;
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLTestCluster

// Constructeur
PLTestCluster::PLTestCluster()
{
	cluster = NULL;
	taskRequirement = new RMTaskResourceRequirement;
	Reset();
}

PLTestCluster::~PLTestCluster()
{
	delete taskRequirement;
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

void PLTestCluster::SetVerbose(boolean bIsVerbose)
{
	bVerbose = bIsVerbose;
}

boolean PLTestCluster::GetVerbose() const
{
	return bVerbose;
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
	bVerbose = true;
	nMaxCoreNumberPerHost = -1;
	nMaxCoreNumberOnSystem = -1;
	lMemoryLimitPerHost = LLONG_MAX;
	lDiskLimitPerHost = LLONG_MAX;
	lMasterSystemAtStart = 8 * lMB;
	lSlaveSystemAtStart = 8 * lMB;

	RMTaskResourceRequirement emptyRequirements;
	taskRequirement->CopyFrom(&emptyRequirements);

	sName = "Test Cluster";
}

RMTaskResourceRequirement* PLTestCluster::GetTaskRequirement()
{
	return taskRequirement;
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

	require(lMemoryLimitPerHost != -1);
	require(lDiskLimitPerHost != -1);
	require(cluster != NULL);

	resources = new RMTaskResourceGrant;

	// Affichage des traces de l'allocation des ressources
	if (bVerbose)
		PLParallelTask::SetTracerResources(2);

	// Modification des ressources au lancement
	RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->Set(lSlaveSystemAtStart);
	RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->Set(lMasterSystemAtStart);

	// Affichae d'une entete du test
	if (bVerbose)
	{
		cout << endl << "----------------------------------" << endl;
		cout << "\t\t" << sName << endl;
		cout << endl;
	}

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
	timer.Reset();
	timer.Start();
	RMParallelResourceManager::ComputeGrantedResourcesForCluster(cluster, taskRequirement, resources);
	timer.Stop();

	// Affichage des ressources manquantes
	if (resources->IsEmpty())
		cout << resources->GetMissingResourceMessage() << endl;

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

double PLTestCluster::GetElapsedTime() const
{
	return timer.GetElapsedTime();
}
