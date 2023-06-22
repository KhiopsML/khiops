// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "RMParallelResourceManager.h"

///////////////////////////////////////////////////////////////////////
// Implementation de  RMParallelResourceManager

RMParallelResourceManager::RMParallelResourceManager()
{
	resourceSystem = NULL;
	resourceRequirement = NULL;
}

RMParallelResourceManager::~RMParallelResourceManager()
{
	resourceSystem = NULL;
	resourceRequirement = NULL;
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
	bRet = ComputeGrantedResourcesForSystem(newSystem, resourceRequirement, grantedResource);
	delete newSystem;

	return bRet;
}

void RMParallelResourceManager::Test()
{
	PLTestCluster test;

	PLParallelTask::SetVerbose(false);
	PLParallelTask::SetSimulatedSlaveNumber(50);
	PLParallelTask::SetParallelSimulated(false);

	PLParallelTask::SetTracerResources(2);

	// Test sans contraintes
	test.SetTestLabel("100 process");
	test.SetHostNumber(1);
	test.SetHostMemory(1000 * lGB);
	test.SetHostDisk(1000 * lGB);
	test.SetHostCores(100);
	test.Solve();

	// Test avec la contrainte globale
	test.Reset();
	test.SetTestLabel("100 process");
	test.SetHostNumber(1);
	test.SetHostMemory(1000 * lGB);
	test.SetHostDisk(1000 * lGB);
	test.SetHostCores(100);
	test.SetSlaveGlobalMemoryMin(100 * lGB);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 1 Gb");
	test.SetHostNumber(1);
	test.SetHostMemory(1 * lGB);
	test.SetHostDisk(1 * lGB);
	test.SetHostCores(1000);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 2 Gb");
	test.SetHostNumber(1);
	test.SetHostMemory(2 * lGB);
	test.SetHostDisk(1 * lGB);
	test.SetHostCores(1000);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on host");
	test.SetHostNumber(1);
	test.SetHostMemory(10 * lGB);
	test.SetHostDisk(1 * lGB);
	test.SetHostCores(1000);
	test.Solve();

	// Test avec les contraintes utilisateurs
	// On aura le memes resultats que le test precedent
	test.Reset();
	test.SetTestLabel("Max proc with 10 Gb on constraint");
	test.SetHostNumber(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(1000);
	test.SetDiskLimitPerHost(1 * lGB);
	test.SetMemoryLimitPerHost(10 * lGB);
	test.Solve();

	// Test avec les contraintes utilisateurs
	test.Reset();
	test.SetTestLabel("Max proc with constraint : 400 MB RAM per proc");
	test.SetHostNumber(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(1000);
	test.SetMemoryLimitPerProc(400 * lMB);
	test.Solve();

	test.Reset();
	test.SetTestLabel("Max proc with constraint : 500 MB RAM per proc");
	test.SetHostNumber(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(1000);
	test.SetMemoryLimitPerProc(500 * lMB);
	test.Solve();

	// Test avec les contraintes de reserves
	test.Reset();
	test.SetTestLabel("2 Gb on host and 200 Mo disk per salve");
	test.SetHostNumber(1);
	test.SetHostMemory(2 * lGB);
	test.SetHostDisk(1 * lGB);
	test.SetHostCores(1000);
	test.SetConstraintSlaveDisk(200 * lMB, LLONG_MAX);
	test.Solve();

	// Test de non rgeression, en 32 Bits (2 Gb allouables)
	test.Reset();
	test.SetTestLabel("Non regression 32 bits");
	test.SetHostNumber(1);
	test.SetHostCores(4);
	test.SetHostMemory(2 * lGB);
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca
	test.SetConstraintSlaveMemory(1 * lGB, 1 * lGB);
	test.SetConstraintMasterMemory(8 * lMB, 8 * lMB);
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 0");
	test.SetHostNumber(10);
	test.SetSystemConfig(0);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 1");
	test.SetHostNumber(10);
	test.SetSystemConfig(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.Solve();

	// Plusiuers machines, sans contrainte
	test.Reset();
	test.SetTestLabel("multi machines : 100 process config 2");
	test.SetHostNumber(10);
	test.SetSystemConfig(2);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour le sesclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0");
	test.SetHostNumber(10);
	test.SetSystemConfig(0);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1");
	test.SetHostNumber(10);
	test.SetSystemConfig(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetMasterSystemAtStart(4000000);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2");
	test.SetHostNumber(10);
	test.SetSystemConfig(2);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.Solve();

	// Une seule machine programme sequentiel
	test.Reset();
	test.SetTestLabel("mono machines : sequential");
	test.SetHostNumber(1);
	test.SetHostMemory(10 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetConstraintSlaveMemory(4 * lGB, LLONG_MAX);
	test.SetConstraintMasterMemory(1 * lGB, LLONG_MAX);
	test.Solve();

	// Une seule machine, un seul slaveProcess
	test.Reset();
	test.SetTestLabel("mono machines : 1 slave process");
	test.SetHostNumber(1);
	test.SetHostMemory(10 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(1);
	test.SetConstraintSlaveMemory(25 * lMB, 25 * lMB);
	test.SetConstraintMasterMemory(16 * lMB, 16 * lMB);
	test.SetSlaveProcessNumber(1);
	test.Solve();

	// Une seule machine, un seul processeur, l'esclave demande plus que ce qu'il y a sur le host
	UIObject::SetUIMode(UIObject::Graphic);
	test.Reset();
	test.SetTestLabel("mono machines : 1 core + Graphical");
	test.SetHostNumber(1);
	test.SetHostCores(1);
	test.SetMasterSystemAtStart(8931728);
	test.SetSlaveSystemAtStart(8 * lMB);
	test.SetMemoryLimitPerHost(1998 * lMB); // En 32 bits il faut ajouter ca

	test.SetConstraintSlaveMemory(1 * lGB, 1717986918); // 1.6 Gb
	test.SetConstraintMasterMemory(16 * lMB, 16 * lMB);
	test.SetHostMemory(2 * lGB);
	test.Solve();

	// Idem en batch cette fois on peut allouer
	UIObject::SetUIMode(UIObject::Textual);
	test.Reset();
	test.SetTestLabel("mono machines : 1 core + Textual");
	test.SetHostNumber(1);
	test.SetHostCores(1);
	test.SetMasterSystemAtStart(8931728);
	test.SetSlaveSystemAtStart(8 * lMB);
	test.SetMemoryLimitPerHost(1998 * lMB);             // En 32 bits il faut ajouter ca
	test.SetConstraintSlaveMemory(1 * lGB, 1717986918); // 1.6 Gb
	test.SetConstraintMasterMemory(16 * lMB, 16 * lMB);
	test.SetHostMemory(2 * lGB);
	test.Solve();
	UIObject::SetUIMode(UIObject::Graphic);

	// Regression
	test.Reset();
	test.SetTestLabel("regression : Master requirement 300 KB and granted 0");
	test.SetHostNumber(1);
	test.SetHostCores(16);
	test.SetHostMemory(1153433600);
	test.SetMasterSystemAtStart(8539455);
	test.SetSlaveSystemAtStart(7228835);
	test.SetMemoryLimitPerHost(1153433600);
	test.SetConstraintSlaveMemory(16474230, LLONG_MAX);
	test.SetConstraintMasterMemory(358692, 358692);
	test.Solve();

	// Regression
	test.Reset();
	test.SetTestLabel("regression : slave is low");
	test.SetHostNumber(1);
	test.SetHostCores(4);
	test.SetHostMemory(2147483648);
	test.SetMasterSystemAtStart(19417088);
	test.SetSlaveSystemAtStart(5436608);
	test.SetConstraintSlaveMemory(3 * lMB, 23 * lMB);
	test.SetConstraintMasterMemory(17 * lMB, 17 * lMB);
	test.Solve();

	// Regression : shared variable
	test.Reset();
	test.SetTestLabel("regression");
	test.SetHostNumber(1);
	test.SetHostCores(1);
	test.SetHostMemory(2147483648);
	test.SetMasterSystemAtStart(90018512);
	test.SetSlaveSystemAtStart(0);
	test.SetConstraintSlaveMemory(8553561, LLONG_MAX);
	test.SetConstraintMasterMemory(391500000, LLONG_MAX);
	test.SetConstraintSharedMemory(136259321, LLONG_MAX);
	test.Solve();

	// Contrainte globale
	test.Reset();
	test.SetTestLabel("Global Constraint");
	test.SetHostNumber(1);
	test.SetHostCores(100);
	test.SetHostMemory(10 * lGB);
	test.SetConstraintSlaveMemory(1 * lGB, 1 * lGB);
	test.SetConstraintMasterMemory(1 * lGB, 1 * lGB);
	test.SetSlaveGlobalMemoryMin(1 * lGB);
	test.SetSlaveGlobalMemoryMax(100 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour le sesclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 0 - Global Constraint");
	test.SetHostNumber(10);
	test.SetSystemConfig(0);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.SetSlaveGlobalMemoryMin(10 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 1 - Global Constraint");
	test.SetHostNumber(10);
	test.SetSystemConfig(1);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetMasterSystemAtStart(4000000);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.SetSlaveGlobalMemoryMin(1 * lGB);
	test.Solve();

	// Plusiuers machines, avec contrainte memoire pour les esclaves + contrainte globale
	test.Reset();
	test.SetTestLabel("multi machines : 10 GB per slave, config 2 - Global Constraint");
	test.SetHostNumber(10);
	test.SetSystemConfig(2);
	test.SetHostMemory(100 * lGB);
	test.SetHostDisk(100 * lGB);
	test.SetHostCores(100);
	test.SetConstraintSlaveMemory(1 * lGB, LLONG_MAX);
	test.SetSlaveGlobalMemoryMin(1 * lGB);
	test.Solve();

	// Bug de Felipe : contrainte globale max
	test.Reset();
	test.SetTestLabel("1 machine : global constraint with min and max on slaves");
	test.SetHostNumber(1);
	test.SetHostMemory(2 * lGB);
	test.SetHostDisk(40 * lGB);
	test.SetHostCores(2);
	test.SetConstraintSlaveMemory(30 * lKB, 36 * lKB);
	test.SetConstraintMasterMemory(30 * lKB, 30 * lKB);
	test.SetConstraintSharedMemory(650 * lMB, 650 * lMB);
	test.SetSlaveGlobalMemoryMin(1 * lMB);
	test.SetSlaveGlobalMemoryMax(22 * lMB);
	test.Solve();
}

void RMParallelResourceManager::SetRequirement(const RMTaskResourceRequirement* rq)
{
	require(rq != NULL);
	resourceRequirement = rq;
}

void RMParallelResourceManager::SetSystem(const RMResourceSystem* rs)
{
	require(rs != NULL);
	resourceSystem = rs;
}

boolean
RMParallelResourceManager::ComputeGrantedResourcesForSystem(const RMResourceSystem* resourceSystem,
							    const RMTaskResourceRequirement* resourceRequirement,
							    RMTaskResourceGrant* grantedResource)
{
	require(grantedResource != NULL);

	RMParallelResourceManager manager;

	if (PLParallelTask::GetTracerResources() > 0)
	{
		if (PLParallelTask::GetCurrentTaskName() != "")
			cout << endl << "Task " << PLParallelTask::GetCurrentTaskName() << endl;
		else
			cout << endl << "Outside task" << endl;

		cout << endl << endl << *resourceSystem;
		cout << "\tSlave reserve  " << LongintToHumanReadableString(GetSlaveReserve(0)) << endl;
		cout << "\tMaster reserve " << LongintToHumanReadableString(GetMasterReserve(0)) << endl << endl;
		cout << *resourceRequirement << endl;
		cout << RMResourceConstraints::ToString();
	}

	// Initialisation du gestionnaire de ressources
	manager.SetRequirement(resourceRequirement);
	manager.SetSystem(resourceSystem);

	// Calcul des ressources pour le systeme
	manager.KnapsackSolve(false, grantedResource);

	// TODO sur un cluster a priori il ne faudra pas essaye de lancer en sequentiel.... par contre lancer un maitre
	// et un esclave !!!
	if (grantedResource->IsEmpty())
	{
		// Si il n'y a pas de ressources pour le systeme courant, on essaye d'allouer le programme en sequentiel
		manager.KnapsackSolve(true, grantedResource);
	}

	if (PLParallelTask::GetTracerResources() > 0)
	{
		cout << *grantedResource << endl;
	}

	// Verification des resultats
	require(manager.Check(grantedResource));
	return not grantedResource->IsEmpty();
}

void RMParallelResourceManager::KnapsackSolve(boolean bSequential, RMTaskResourceGrant* grantedResources) const
{
	boolean bVerbose;
	ObjectArray oaClasses;
	ObjectArray oaHosts; // Tablerau de HostResource, bijection entre ce tableau et le tableau des classes oaClasses
	PLClass* plClass;
	int nRT;
	int nProcNumber;
	int nBegin;
	ObjectArray oaItems;
	PLItem* item;
	const RMHostResource* hostResource;
	int nMaxProcResource;
	int nMaxProc;
	int nHostIndex;
	int nHostNumber;
	double dEtalement;
	PLKnapsackResources problemSolver;
	int nWeight;
	longint lAvailableHostResource;
	longint lMissingResource;
	longint lUsedResourceOnHost;
	int i;
	longint lSlaveResource;
	longint lMasterResource;

	require(resourceRequirement != NULL);
	require(resourceSystem != NULL);
	require(grantedResources != NULL);

	// Initialisation
	bVerbose = false;
	grantedResources->Initialize();

	// On force le nombre de host a 1 en simule
	if (PLParallelTask::GetParallelSimulated() or bSequential)
		nHostNumber = 1;
	else
		nHostNumber = resourceSystem->GetHostNumber();

	// Initialisation du probleme du sac a dos : classe PLKnapsackProblem
	for (nHostIndex = 0; nHostIndex < nHostNumber; nHostIndex++)
	{
		// En simule, il n'y a qu'un seul host et c'est le master
		if (PLParallelTask::GetParallelSimulated() or bSequential)
			hostResource = resourceSystem->GetMasterHostResource();
		else
			hostResource = resourceSystem->GetHostResourceAt(nHostIndex);
		nMaxProc = INT_MAX;

		if (PLParallelTask::GetTracerResources() == 2)
			cout << "host " << nHostIndex << endl;

		// Pour chaque ressource
		for (nRT = 0; nRT < UNKNOWN; nRT++)
		{
			if (PLParallelTask::GetTracerResources() == 2)
			{
				cout << ResourceToString((Resource)nRT) << " (physical)  free  "
				     << LongintToHumanReadableString(hostResource->GetResourceFree(nRT)) << "\t->\t"
				     << hostResource->GetResourceFree(nRT) << endl;
				cout << ResourceToString((Resource)nRT) << " (physical) limit "
				     << LongintToHumanReadableString(RMResourceConstraints::GetResourceLimit(nRT) * lMB)
				     << "\t->\t" << RMResourceConstraints::GetResourceLimit(nRT) * lMB << endl;
			}

			// Ressource disponible sur le host
			lAvailableHostResource =
			    min(hostResource->GetResourceFree(nRT), RMResourceConstraints::GetResourceLimit(nRT) * lMB);

			// Passage de cette resource en logique
			lAvailableHostResource =
			    RMStandardResourceDriver::PhysicalToLogical(nRT, lAvailableHostResource);

			if (PLParallelTask::GetTracerResources() == 2)
			{
				cout << ResourceToString((Resource)nRT) << " (logical)  available  "
				     << LongintToHumanReadableString(lAvailableHostResource) << "\t->\t"
				     << lAvailableHostResource << endl;
			}

			// Nombre de processus max en respectant les contraintes memoires
			nMaxProcResource = ComputeProcessNumber(bSequential, nRT, lAvailableHostResource,
								hostResource->IsMasterHost(), lMissingResource);

			// La ressource manquante qui interesse l'utilisateur est celle du master host
			if (hostResource->IsMasterHost())
				grantedResources->lvMissingResources.SetAt(nRT, lMissingResource);

			// Est-ce que les exigences de la taches sont coherentes avec les contraintes par processus
			// Test pour le processus maitre
			lMissingResource = RMStandardResourceDriver::PhysicalToLogical(
					       nRT, RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB) -
					   GetMasterMin(nRT) - GetMasterHiddenResource(bSequential, nRT);
			if (lMissingResource < 0)
			{
				nMaxProcResource = 0;
				grantedResources->bMissingResourceForProcConstraint = true;
				grantedResources->lvMissingResources.SetAt(nRT, -lMissingResource);
			}
			else
			{
				// Test pour le processus esclave
				lMissingResource = RMStandardResourceDriver::PhysicalToLogical(
						       nRT, RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB) -
						   GetSlaveMin(nRT) - GetSlaveHiddenResource(bSequential, nRT);
				if (lMissingResource < 0)
				{
					nMaxProcResource = 0;
					grantedResources->bMissingResourceForProcConstraint = true;
					grantedResources->lvMissingResources.SetAt(nRT, -lMissingResource);
				}
			}

			// Nombre de processus max en respectant les contraintes memoire par processus
			if (nMaxProcResource != 0)
				while (min(hostResource->GetResourceFree(nRT),
					   RMResourceConstraints::GetResourceLimit(nRT) * lMB) /
					   nMaxProcResource >
				       RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB)
					nMaxProcResource--;

			// On prend le minimum sur toutes les ressources
			nMaxProc = min(nMaxProc, nMaxProcResource);

			if (PLParallelTask::GetTracerResources() == 2)
			{
				cout << "Sequential mode " << bSequential << endl;
				cout << "Slave reserve " << LongintToHumanReadableString(GetSlaveReserve(nRT))
				     << "\t->\t" << LongintToReadableString(GetSlaveReserve(nRT)) << endl;
				cout << "Master reserve " << LongintToHumanReadableString(GetMasterReserve(nRT))
				     << "\t->\t" << LongintToReadableString(GetMasterReserve(nRT)) << endl;
				cout << "User Interface Memory Reserve "
				     << LongintToHumanReadableString(UIObject::GetUserInterfaceMemoryReserve())
				     << "\t->\t" << LongintToReadableString(UIObject::GetUserInterfaceMemoryReserve())
				     << endl;
				cout << "Physical Memory Reserve "
				     << LongintToHumanReadableString(MemGetPhysicalMemoryReserve()) << "\t->\t"
				     << LongintToReadableString(UIObject::GetUserInterfaceMemoryReserve()) << endl;
				cout << "Allocator Reserve " << LongintToHumanReadableString(MemGetAllocatorReserve())
				     << "\t->\t" << LongintToReadableString(MemGetAllocatorReserve()) << endl;
				cout << "Mem for serialization " << LongintToHumanReadableString(2 * MemSegmentByteSize)
				     << "\t->\t" << LongintToReadableString(2 * MemSegmentByteSize) << endl;
				cout << "Is master host " << hostResource->IsMasterHost() << endl;
				cout << "Violating proc limit " << grantedResources->bMissingResourceForProcConstraint
				     << endl;
				cout << "Resource limit per proc "
				     << LongintToHumanReadableString(
					    RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB)
				     << endl;
				cout << "Master requirement " << LongintToHumanReadableString(GetMasterMin(nRT))
				     << "\t->\t" << LongintToReadableString(GetMasterMin(nRT)) << endl;
				cout << "Slave requirement " << LongintToHumanReadableString(GetSlaveMin(nRT))
				     << "\t->\t" << LongintToReadableString(GetSlaveMin(nRT)) << endl;
				cout << "Shared requirement " << LongintToHumanReadableString(GetSharedMin(nRT))
				     << "\t->\t" << LongintToReadableString(GetSharedMin(nRT)) << endl;
				cout << "Slave System at start "
				     << LongintToHumanReadableString(
					    resourceRequirement->GetSlaveSystemAtStart()->GetResource(nRT)->GetMin())
				     << "\t->\t"
				     << LongintToReadableString(
					    resourceRequirement->GetSlaveSystemAtStart()->GetResource(nRT)->GetMin())
				     << endl;
				cout << "Master System at start "
				     << LongintToHumanReadableString(
					    resourceRequirement->GetMasterSystemAtStart()->GetResource(nRT)->GetMin())
				     << "\t->\t"
				     << LongintToReadableString(
					    resourceRequirement->GetMasterSystemAtStart()->GetResource(nRT)->GetMin())
				     << endl;
				cout << "Missing ressource "
				     << LongintToHumanReadableString(grantedResources->lvMissingResources.GetAt(nRT))
				     << "\t->\t"
				     << LongintToReadableString(grantedResources->lvMissingResources.GetAt(nRT)) << endl
				     << endl;
			}
		}

		// On borne ...
		if (not bSequential)
		{
			// ... par le nombre de processus logique de la machine
			if (not PLParallelTask::GetParallelSimulated())
				nMaxProc = min(nMaxProc, hostResource->GetLogicalProcessNumber());

			// et par la contrainte utilisateur du nombre de pocessus par machine
			nMaxProc = min(nMaxProc, RMResourceConstraints::GetMaxCoreNumberPerHost());
		}

		if (PLParallelTask::GetParallelSimulated())
		{
			nMaxProc = min(nMaxProc, PLParallelTask::GetSimulatedSlaveNumber() + 1);
		}

		// En sequentiel on a 2 processus : le maitre et l'esclave
		if (bSequential)
		{
			nMaxProc = min(2, nMaxProc);
			nBegin = 2;
		}
		else
		{
			nBegin = 1;
		}

		// Construction d'une solution pour chaque nombre possible de processus
		dEtalement = 0;
		plClass = new PLClass;
		oaItems.SetSize(0);
		for (nProcNumber = nBegin; nProcNumber <= nMaxProc; nProcNumber++)
		{
			item = new PLItem;
			item->SetWeight(nProcNumber);

			// Ajout d'une valeur fictive pour forcer a prendre la premiere solution dans la classe du
			// master
			if (hostResource->IsMasterHost())
				item->Add(1);
			else
				item->Add(0);

			// Construction de l'item a partir des ressources consommees
			for (nRT = 0; nRT < UNKNOWN; nRT++)
			{
				// Calcul des ressouces consommes pour les exigences minimales pour le maitre, chaque
				// esclave et sur la machine
				lUsedResourceOnHost = MinimumAllocation(bSequential, nRT, nProcNumber, hostResource,
									lSlaveResource, lMasterResource);
				item->Add(lUsedResourceOnHost);
			}

			dEtalement += 1.0 / nProcNumber;
			item->Add((longint)(dEtalement * 100000)); // Multiplication par 1000 pour etre entier (On garde
								   // une bonne precision pour 128 processeurs)
			oaItems.Add(item);
		}
		// Ajout de la classe au probleme du sac a dos
		if (oaItems.GetSize() > 0)
		{
			plClass->SetItems(&oaItems);
			problemSolver.AddClass(plClass);
			oaClasses.Add(plClass);
			oaHosts.Add(hostResource->Clone());
		}
		else
		{
			delete plClass;

			// Si c'est le master et qu'il n'y a pas de solution sur ce host, il n'y a aucune solution
			// possible
			if (hostResource->IsMasterHost())
			{
				oaClasses.DeleteAll();
				problemSolver.RemoveClasses();
				oaHosts.RemoveAll();
				break;
			}
		}
	} // for (nHostIndex = 0; nHostIndex < nHostNumber; nHostIndex++)

	// Poids max du sac a dos = nombre de processus max sur le systeme
	nWeight = INT_MAX;
	nWeight = min(nWeight, resourceRequirement->GetMaxSlaveProcessNumber() + 1);

	if (not PLParallelTask::GetParallelSimulated())
	{
		nWeight = min(nWeight, resourceSystem->GetLogicalProcessNumber());
		nWeight = min(nWeight, RMResourceConstraints::GetMaxCoreNumber());
		nWeight = min(nWeight, RMResourceConstraints::GetMaxProcessNumber());
	}
	assert(nWeight >= 0);
	if (bSequential and nWeight != 0)
		nWeight = 2;
	problemSolver.SetWeight(nWeight);

	// Affichage du probleme
	if (bVerbose)
		cout << problemSolver << endl;

	// Resolution
	problemSolver.SetHostResources(&oaHosts);
	problemSolver.SetGrantedResource(grantedResources);
	problemSolver.SetRequirement(resourceRequirement);
	problemSolver.Solve();

	// Affichage des resultats
	if (bVerbose)
	{
		cout << "Solutions " << endl;
		for (i = 0; i < oaClasses.GetSize(); i++)
			cout << *oaClasses.GetAt(i) << endl << endl;
	}

	// Construction des grantedResources a partir des classes du sac a dos
	// Ajout des ressources en extra
	TransformClassesToResources(bSequential, &oaClasses, &oaHosts, grantedResources);

	// Nettoyage
	oaClasses.DeleteAll();
	oaHosts.DeleteAll();
}

void RMParallelResourceManager::TransformClassesToResources(boolean bSequential, const ObjectArray* oaClasses,
							    const ObjectArray* oaHosts,
							    RMTaskResourceGrant* grantedResources) const
{
	RMResourceGrant* resourceGrant;
	RMResourceGrant* masterResourceGrant;
	RMResourceGrant* slaveResourceGrant;
	RMResourceContainer extraSlaveResource;
	RMResourceContainer extraMasterResource;
	RMResourceContainer rcSlaveResource;
	RMResourceContainer rcMasterResource;
	int nProcNumber;
	PLItem* item;
	const RMHostResource* hostResource;
	int i;
	int j;
	int nRT;
	longint lMasterResource;
	longint lSlaveResource;
	longint lExtraSlaveResource;
	longint lExtraMasterResource;

	require(grantedResources != NULL);

	// Calcul du nombre de processus total
	nProcNumber = 0;
	for (i = 0; i < oaClasses->GetSize(); i++)
	{
		item = cast(PLClass*, oaClasses->GetAt(i))->GetValidItem();
		if (item != NULL)
			nProcNumber += item->GetWeight();
	}

	ensure(bSequential or nProcNumber <= RMResourceConstraints::GetMaxProcessNumber());
	ensure(PLParallelTask::GetParallelSimulated() or bSequential or
	       nProcNumber <= RMResourceConstraints::GetMaxCoreNumber());
	ensure(bSequential or nProcNumber <= resourceRequirement->GetMaxSlaveProcessNumber() + 1);

	// Si on est en parallele on doit avoir plus de 2 processus
	if (nProcNumber <= 2 and not bSequential)
		return;

	// Si plus d'un processus (en sequentiel on a 2 processus)
	if (nProcNumber > 1)
	{
		// Affectation des resultats dans les ressources
		for (i = 0; i < oaClasses->GetSize(); i++)
		{
			// Acces au host
			hostResource = cast(RMHostResource*, oaHosts->GetAt(i));

			// et a la solution corresondante
			item = cast(PLClass*, oaClasses->GetAt(i))->GetValidItem();

			// Si il y a une solution sur ce host
			if (item != NULL)
			{
				//////////////////////////////////////////////////////////////////
				// Calcul des ressources diponibles apres avoir alloue le min sur cette machine
				for (nRT = 0; nRT < UNKNOWN; nRT++)
				{
					// Allocation du minimum pour respecter les exigences
					MinimumAllocation(bSequential, nRT, item->GetWeight(), hostResource,
							  lSlaveResource, lMasterResource);

					// Ajout des ressources de la contrainte globale
					lSlaveResource += resourceRequirement->GetGlobalSlaveRequirement()
							      ->GetResource(nRT)
							      ->GetMin() /
							  (nProcNumber - 1);
					rcSlaveResource.SetValue(nRT, lSlaveResource);
					rcMasterResource.SetValue(nRT, lMasterResource);

					// Calcul des ressources en extra
					ComputeExtraResource(nRT, bSequential, item->GetWeight(), nProcNumber,
							     hostResource, lSlaveResource, lMasterResource,
							     lExtraSlaveResource, lExtraMasterResource);
					extraSlaveResource.SetValue(nRT, lExtraSlaveResource);
					extraMasterResource.SetValue(nRT, lExtraMasterResource);
				}

				//////////////////////////////////////////////////////////////
				// Construction d'un resourceGrant par processus (par rang MPI)
				if (bSequential)
				{
					masterResourceGrant = new RMResourceGrant;
					slaveResourceGrant = new RMResourceGrant;
					masterResourceGrant->SetRank(0);
					slaveResourceGrant->SetRank(1);

					// Allocation de chaque ressource
					for (nRT = 0; nRT < UNKNOWN; nRT++)
					{
						masterResourceGrant->SetResource(
						    nRT, rcMasterResource.GetValue(nRT) +
							     extraMasterResource.GetValue(nRT) -
							     GetMasterHiddenResource(bSequential, nRT));
						masterResourceGrant->SetSharedResource(nRT, GetSharedMin(nRT));
						slaveResourceGrant->SetResource(nRT,
										rcSlaveResource.GetValue(nRT) +
										    extraSlaveResource.GetValue(nRT));
						slaveResourceGrant->SetSharedResource(nRT, 0);
					}

					grantedResources->AddResource(masterResourceGrant);
					grantedResources->AddResource(slaveResourceGrant);
				}
				else
				{
					// Pour chaque rang
					for (j = 0; j < item->GetWeight(); j++)
					{
						resourceGrant = new RMResourceGrant;
						if (PLParallelTask::GetParallelSimulated())
							resourceGrant->SetRank(j);
						else
							resourceGrant->SetRank(hostResource->GetRanks()->GetAt(j));

						// Pour chaque ressource
						for (nRT = 0; nRT < UNKNOWN; nRT++)
						{
							if (resourceGrant->GetRank() == 0)
							{
								resourceGrant->SetResource(
								    nRT, rcMasterResource.GetValue(nRT) +
									     extraMasterResource.GetValue(nRT) -
									     GetMasterHiddenResource(bSequential, nRT));
								resourceGrant->SetSharedResource(nRT,
												 GetSharedMin(nRT));
							}
							else
							{
								resourceGrant->SetResource(
								    nRT, rcSlaveResource.GetValue(nRT) +
									     extraSlaveResource.GetValue(nRT) -
									     GetSlaveHiddenResource(bSequential, nRT));
								resourceGrant->SetSharedResource(nRT,
												 GetSharedMin(nRT));
							}
						}
						grantedResources->AddResource(resourceGrant);
					}
				}
			}
		}
	}
}

// Somme de de longint en gerant le depassement
static longint lsum(longint l1, longint l2)
{
	if (l1 > LLONG_MAX - l2)
		return LLONG_MAX;
	return l1 + l2;
}

boolean RMParallelResourceManager::Check(RMTaskResourceGrant* taskResourceGrant) const
{
	boolean bOk;
	longint lSlaveMemory;
	longint lSlaveDisk;
	int nProcNumberOnSystem;
	longint lGlobalMemoryForSlaveMin;
	longint lGlobalDiskForSlaveMin;
	longint lGlobalMemoryForSlaveMax;
	longint lGlobalDiskForSlaveMax;

	require(taskResourceGrant != NULL);
	require(resourceRequirement != NULL);

	bOk = true;

	if (taskResourceGrant->IsEmpty())
		return true;

	nProcNumberOnSystem = taskResourceGrant->GetProcessNumber();
	lGlobalMemoryForSlaveMin = 0;
	lGlobalDiskForSlaveMin = 0;
	lGlobalMemoryForSlaveMax = 0;
	lGlobalDiskForSlaveMax = 0;
	if (nProcNumberOnSystem > 1)
	{
		lGlobalMemoryForSlaveMin =
		    resourceRequirement->GetGlobalSlaveRequirement()->GetResource(MEMORY)->GetMin() /
		    (nProcNumberOnSystem - 1);
		lGlobalMemoryForSlaveMax =
		    resourceRequirement->GetGlobalSlaveRequirement()->GetResource(MEMORY)->GetMax() /
		    (nProcNumberOnSystem - 1);
		lGlobalDiskForSlaveMin = resourceRequirement->GetGlobalSlaveRequirement()->GetResource(DISK)->GetMin() /
					 (nProcNumberOnSystem - 1);
		lGlobalDiskForSlaveMax = resourceRequirement->GetGlobalSlaveRequirement()->GetResource(DISK)->GetMax() /
					 (nProcNumberOnSystem - 1);
	}
	else
	{
		lGlobalMemoryForSlaveMin =
		    resourceRequirement->GetGlobalSlaveRequirement()->GetResource(MEMORY)->GetMin();
		lGlobalMemoryForSlaveMax =
		    resourceRequirement->GetGlobalSlaveRequirement()->GetResource(MEMORY)->GetMax();
		lGlobalDiskForSlaveMin = resourceRequirement->GetGlobalSlaveRequirement()->GetResource(DISK)->GetMin();
		lGlobalDiskForSlaveMax = resourceRequirement->GetGlobalSlaveRequirement()->GetResource(DISK)->GetMax();
	}

	// Verification des ressources allouees au maitre
	// Memoire
	bOk = bOk and resourceRequirement->GetMasterRequirement()->GetMemory()->GetMin() <=
			  taskResourceGrant->GetMasterMemory();
	require(bOk);
	bOk = bOk and resourceRequirement->GetMasterRequirement()->GetMemory()->GetMax() >=
			  taskResourceGrant->GetMasterMemory();
	require(bOk);

	bOk = bOk and
	      RMStandardResourceDriver::PhysicalToLogical(MEMORY, RMResourceConstraints::GetMemoryLimitPerProc() *
								      lMB) >= taskResourceGrant->GetMasterMemory();
	require(bOk);

	// Disque
	bOk = bOk and
	      resourceRequirement->GetMasterRequirement()->GetDisk()->GetMin() <= taskResourceGrant->GetMasterDisk();
	require(bOk);
	bOk = bOk and
	      resourceRequirement->GetMasterRequirement()->GetDisk()->GetMax() >= taskResourceGrant->GetMasterDisk();
	require(bOk);
	bOk = bOk and RMStandardResourceDriver::PhysicalToLogical(DISK, RMResourceConstraints::GetDiskLimitPerProc() *
									    lMB) >= taskResourceGrant->GetMasterDisk();
	require(bOk);

	// Verification des ressources allouees a chaque esclave

	// Exigences Memoire
	lSlaveMemory = taskResourceGrant->GetMinSlaveMemory();
	bOk = bOk and resourceRequirement->GetSlaveRequirement()->GetMemory()->GetMin() + lGlobalMemoryForSlaveMin <=
			  lSlaveMemory;
	require(bOk);
	bOk = bOk and lsum(resourceRequirement->GetSlaveRequirement()->GetMemory()->GetMax(),
			   lGlobalMemoryForSlaveMax) >= lSlaveMemory;
	require(bOk);

	// Exigences Disque
	lSlaveDisk = taskResourceGrant->GetMinSlaveDisk();
	bOk = bOk and lsum(resourceRequirement->GetSlaveRequirement()->GetDisk()->GetMin(), lGlobalDiskForSlaveMin) <=
			  lSlaveDisk;
	require(bOk);
	bOk = bOk and lsum(resourceRequirement->GetSlaveRequirement()->GetDisk()->GetMax(), lGlobalDiskForSlaveMax) >=
			  lSlaveDisk;
	require(bOk);

	// Contrainte memoire
	bOk = bOk and RMStandardResourceDriver::PhysicalToLogical(
			  MEMORY, RMResourceConstraints::GetMemoryLimitPerProc() * lMB) >= lSlaveMemory;
	require(bOk);

	// Contrainte disque
	bOk = bOk and RMStandardResourceDriver::PhysicalToLogical(DISK, RMResourceConstraints::GetDiskLimitPerProc() *
									    lMB) >= lSlaveDisk;
	require(bOk);

	// Nombre de slaveProcess
	bOk = bOk and taskResourceGrant->GetSlaveNumber() <= resourceRequirement->GetMaxSlaveProcessNumber();
	require(bOk);

	// Nombre de processus
	bOk = bOk and taskResourceGrant->GetProcessNumber() <= RMResourceConstraints::GetMaxProcessNumber();
	require(bOk);

	// Cas sequentiel
	if (taskResourceGrant->IsSequentialTask())
	{
		bOk = bOk and taskResourceGrant->GetMasterDisk() + taskResourceGrant->GetMinSlaveDisk() <
				  RMStandardResourceDriver::PhysicalToLogical(
				      DISK, RMResourceConstraints::GetDiskLimitPerProc() * lMB);
		require(bOk);

		bOk = bOk and taskResourceGrant->GetMasterMemory() + taskResourceGrant->GetMinSlaveMemory() <
				  RMStandardResourceDriver::PhysicalToLogical(
				      MEMORY, RMResourceConstraints::GetMemoryLimitPerProc() * lMB);
		require(bOk);
	}
	return bOk;
}

void RMParallelResourceManager::ComputeExtraResource(int nRT, boolean bSequential, int nProcNumberOnHost,
						     int nProcNumberOnSystem, const RMHostResource* hostResource,
						     longint lSlaveResource, longint lMasterResource,
						     longint& lExtraSlaveResource, longint& lExtraMasterResource) const
{
	boolean bAnySlave;
	boolean bAnyMaster;
	int nSlaveNumber;
	int nMasterNumber;
	longint lRemainingOnHost;
	longint lHostResource;

	require(not bSequential or (bSequential and nProcNumberOnSystem == 2));

	// Y a-t-il un maitre et/ou des esclaves sur la machine
	hostResource->IsMasterHost() ? bAnyMaster = true : bAnyMaster = false;
	nProcNumberOnHost == 1 and hostResource->IsMasterHost() ? bAnySlave = false : bAnySlave = true;

	// Nombre d'esclaves et de maitre sur la machine
	bAnyMaster ? nSlaveNumber = nProcNumberOnHost - 1 : nSlaveNumber = nProcNumberOnHost;
	bAnyMaster ? nMasterNumber = 1 : nMasterNumber = 0;

	// Ressources disponibles sur le host
	lHostResource = min(hostResource->GetResourceFree(nRT), RMResourceConstraints::GetResourceLimit(nRT) * lMB);
	lHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lHostResource);

	// Calcul du surplus de ressources
	lRemainingOnHost = lHostResource - GetUsedResource(nProcNumberOnHost, hostResource->IsMasterHost(),
							   lMasterResource, lSlaveResource);

	assert(lRemainingOnHost >= 0);
	switch (resourceRequirement->GetResourceAllocationPolicy(nRT))
	{
	case RMTaskResourceRequirement::slavePreferred:

		// Allocation du max pour les esclaves
		if (not bAnySlave)
			lExtraSlaveResource = 0;
		else
		{
			// Max allouable avec ce qui reste
			lExtraSlaveResource = lRemainingOnHost / nSlaveNumber;

			// Reduction sous les contraintes
			ShrinkForSlaveUnderConstraints(bSequential, nRT, nSlaveNumber, nProcNumberOnSystem - 1,
						       nMasterNumber, lExtraSlaveResource);
		}

		// Allocation du restant au maitre
		lExtraMasterResource = lRemainingOnHost - nSlaveNumber * lExtraSlaveResource;
		if (lExtraMasterResource > 0 and bAnyMaster)
		{
			// Reduction sous les contraintes
			ShrinkForMasterUnderConstraints(bSequential, nRT, nSlaveNumber, nMasterNumber,
							lExtraMasterResource);
		}
		else
			lExtraMasterResource = 0;
		break;
	case RMTaskResourceRequirement::masterPreferred:
		if (not bAnyMaster)
			lExtraMasterResource = 0;
		else
		{
			lExtraMasterResource = lRemainingOnHost;

			// Reduction sous les contraintes
			ShrinkForMasterUnderConstraints(bSequential, nRT, nSlaveNumber, nMasterNumber,
							lExtraMasterResource);
		}

		// Allocation du restant aux esclaves
		if (bAnySlave)
		{
			lExtraSlaveResource = (lRemainingOnHost - lExtraMasterResource) / nSlaveNumber;
			if (lExtraSlaveResource > 0)
			{
				// Reduction sous les contraintes
				ShrinkForSlaveUnderConstraints(bSequential, nRT, nSlaveNumber, nProcNumberOnSystem - 1,
							       nMasterNumber, lExtraSlaveResource);
			}
		}
		else
			lExtraSlaveResource = 0;

		break;
	default:
		assert(false);
	}
	assert(lExtraSlaveResource >= 0);
	assert(lExtraMasterResource >= 0);
}

void RMParallelResourceManager::ShrinkForSlaveUnderConstraints(boolean bIsSequential, int nRT, int nSlaveNumberOnHost,
							       int nSlaveNumberOnSystem, int nMasterNumber,
							       longint& lRemainingResource) const
{
	longint lResourceLimit;
	longint lResourceLimitPerProc;
	longint lSlaveResource;
	longint lMasterResource;

	require(lRemainingResource >= 0);
	lResourceLimit =
	    RMStandardResourceDriver::PhysicalToLogical(nRT, RMResourceConstraints::GetResourceLimit(nRT) * lMB);
	lResourceLimitPerProc =
	    RMStandardResourceDriver::PhysicalToLogical(nRT, RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB);

	lSlaveResource = GetSlaveMin(nRT) + GetSlaveHiddenResource(bIsSequential, nRT);
	lMasterResource = GetMasterMin(nRT) + GetMasterHiddenResource(bIsSequential, nRT);

	// Max allouable pour les exigences de la tache
	lRemainingResource =
	    min(lRemainingResource, lsum(GetSlaveMax(nRT) - GetSlaveMin(nRT),
					 (GetSlaveGlobalMax(nRT) - GetSlaveGlobalMin(nRT)) / nSlaveNumberOnSystem));

	// Max allouable pour les contraintes sur le processus
	lRemainingResource = min(lRemainingResource, lResourceLimitPerProc - lSlaveResource);

	// Max allouable pour les contraintes sur la machine
	lRemainingResource =
	    min(lRemainingResource,
		(lResourceLimit - nMasterNumber * lMasterResource) / nSlaveNumberOnHost - lSlaveResource);
	require(lRemainingResource >= 0);
}

void RMParallelResourceManager::ShrinkForMasterUnderConstraints(boolean bIsSequential, int nRT, int nSlaveNumber,
								int nMasterNumber, longint& lRemainingResource) const
{
	longint lResourceLimit;
	longint lResourceLimitPerProc;
	longint lSlaveResource;
	longint lMasterResource;

	require(lRemainingResource >= 0);

	lResourceLimit =
	    RMStandardResourceDriver::PhysicalToLogical(nRT, RMResourceConstraints::GetResourceLimit(nRT) * lMB);
	lResourceLimitPerProc =
	    RMStandardResourceDriver::PhysicalToLogical(nRT, RMResourceConstraints::GetResourceLimitPerProc(nRT) * lMB);

	lSlaveResource = GetSlaveMin(nRT) + GetSlaveHiddenResource(bIsSequential, nRT);
	lMasterResource = GetMasterMin(nRT) + GetMasterHiddenResource(bIsSequential, nRT);

	// Max allouable pour les exigences de la tache
	lRemainingResource = min(lRemainingResource, GetMasterMax(nRT) - GetMasterMin(nRT));

	// Max allouable pour les contraintes sur le processus
	lRemainingResource = min(lRemainingResource, lResourceLimitPerProc - lMasterResource);

	// Max allouable pour les contraintes sur la machine
	lRemainingResource = min(lRemainingResource, lResourceLimit - nSlaveNumber * lSlaveResource - lMasterResource);

	require(lRemainingResource >= 0);
}

longint RMParallelResourceManager::MinimumAllocation(boolean bIsSequential, int nRT, int nProcNumber,
						     const RMHostResource* hostResource, longint& lSlaveResource,
						     longint& lMasterResource) const
{
	longint lResourceMin;

	require(nRT < UNKNOWN);

	require(nProcNumber > 0);
	require(not bIsSequential or (bIsSequential and nProcNumber == 2));

	// Allocation des ressources minimales necessaires au maitre et aux esclaves
	lSlaveResource = GetSlaveMin(nRT) + GetSlaveHiddenResource(bIsSequential, nRT);
	lMasterResource = GetMasterMin(nRT) + GetMasterHiddenResource(bIsSequential, nRT);

	// Memoire utilise sur le host pour les resources minimales
	lResourceMin = GetUsedResource(nProcNumber, hostResource->IsMasterHost(), lMasterResource, lSlaveResource);

	assert(lResourceMin >= 0);
	return lResourceMin;
}

int RMParallelResourceManager::ComputeProcessNumber(boolean bSequential, int nRT, longint lLogicalHostResource,
						    boolean bIsMasterHost, longint& lMissingResource) const
{
	longint lProc;
	int nMaster;
	longint lSlaveRequirements;
	longint lMasterRequirements;
	longint lRemainingResource;
	longint lMasterAtStart;
	longint lSlaveAtStart;

	lMissingResource = 0;

	if (bIsMasterHost)
		nMaster = 1;
	else
		nMaster = 0;

	lMasterRequirements = GetMasterMin(nRT) + GetSharedMin(nRT);
	lSlaveRequirements = GetSlaveMin(nRT);
	if (not bSequential)
		lSlaveRequirements += GetSharedMin(nRT);

	// Ressources requises au demarrage
	lMasterAtStart =
	    resourceRequirement->GetMasterSystemAtStart()->GetResource(nRT)->GetMin() + GetMasterReserve(nRT);
	if (bSequential)
		// En sequentiel la reserve et la ressource au demarage sont deja pris en compte chez le maitre
		lSlaveAtStart = 0;
	else
		lSlaveAtStart =
		    resourceRequirement->GetSlaveSystemAtStart()->GetResource(nRT)->GetMin() + GetSlaveReserve(nRT);

	// On test si il y a assez de place pour le master sur le host
	if (bIsMasterHost)
	{
		if (bSequential)
		{
			// En sequentiel il y a les ressources du maitre et d'un esclave
			lRemainingResource =
			    lLogicalHostResource - lMasterAtStart - lMasterRequirements - lSlaveRequirements;
		}
		else
		{
			// En parallele, la machine peut contenir un maitre sans esclaves (cluster)
			lRemainingResource = lLogicalHostResource - lMasterAtStart - lMasterRequirements;
		}
		if (lRemainingResource < 0)
		{
			lMissingResource = -lRemainingResource;
			return 0;
		}
	}

	// Calcul du nombre de proc a partir de la contrainte issue de la tache (9)
	if (lSlaveRequirements + lSlaveAtStart != 0) // Test pour eviter la division par 0
	{
		// Si les ressources consommees par un esclave sont plus grandes que celles du systeme
		if (lSlaveRequirements + lSlaveAtStart >=
		    lLogicalHostResource - nMaster * (lMasterRequirements + lMasterAtStart))
		{
			if (bIsMasterHost and not bSequential)
			{
				// Si on est sur la machine du maitre, on n'alloue qu'un seul proc : le maitre
				lProc = 1;
			}
			else
			{
				// Si on n'est pas sur la machine du maitre, on ne peut pas allouer d'esclaves : calcule
				// des ressources manquantes
				lMissingResource = nMaster * (lMasterRequirements + lMasterAtStart) +
						   lSlaveRequirements + lSlaveAtStart - lLogicalHostResource;
				lProc = 0;
			}
		}
		else
		{
			lProc = (lLogicalHostResource - nMaster * (lMasterRequirements + lMasterAtStart)) /
				    (lSlaveRequirements + lSlaveAtStart) +
				nMaster;
		}
	}
	else
	{
		// On peut allouer autant d'esclaves que l'on veut
		return INT_MAX;
	}

	// Bornage de la solution
	if (lProc < 0)
		lProc = 0;
	if (lProc > INT_MAX)
		lProc = INT_MAX;
	return (int)lProc;
}

longint RMParallelResourceManager::GetUsedResource(int nProcNumber, boolean bIsMasterHost, longint lMasterUse,
						   longint lSlaveUse) const
{
	int nMaster;
	longint lUsedResource;

	if (bIsMasterHost)
		nMaster = 1;
	else
		nMaster = 0;

	lUsedResource = nMaster * lMasterUse + (nProcNumber - nMaster) * lSlaveUse;
	assert(lUsedResource >= 0);
	return lUsedResource;
}

longint RMParallelResourceManager::GetMasterReserve(int nResourceType)
{
	require(nResourceType < UNKNOWN);
	if (nResourceType == MEMORY)
		return RMStandardResourceDriver::PhysicalToLogical(MEMORY, UIObject::GetUserInterfaceMemoryReserve() +
									       MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint RMParallelResourceManager::GetSlaveReserve(int nResourceType)
{
	require(nResourceType < UNKNOWN);
	if (nResourceType == MEMORY)
		return RMStandardResourceDriver::PhysicalToLogical(MEMORY, MemGetPhysicalMemoryReserve() +
									       MemGetAllocatorReserve());
	else
		return 0;
}

longint RMParallelResourceManager::GetMasterHiddenResource(boolean bIsSequential, int nRT) const
{
	longint lHiddenResource;
	lHiddenResource = resourceRequirement->GetMasterSystemAtStart()->GetResource(nRT)->GetMin() +
			  GetMasterReserve(nRT) + GetSharedMin(nRT);

	// On ajoute 2 tailles de blocs pour la serialisation
	if (not bIsSequential and nRT == MEMORY)
		lHiddenResource += 2 * MemSegmentByteSize;
	return lHiddenResource;
}

longint RMParallelResourceManager::GetSlaveHiddenResource(boolean bIsSequential, int nRT) const
{
	longint lHiddenResource;
	if (bIsSequential)
		lHiddenResource = 0;
	else
	{
		lHiddenResource = resourceRequirement->GetSlaveSystemAtStart()->GetResource(nRT)->GetMin() +
				  GetSlaveReserve(nRT) + GetSharedMin(nRT);

		// On ajoute 2 tailles de blocs pour la serialisation
		if (nRT == MEMORY)
			lHiddenResource += 2 * MemSegmentByteSize;
	}
	return lHiddenResource;
}

longint RMParallelResourceManager::GetSlaveMin(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetSlaveRequirement()->GetResource(nRT)->GetMin();
}

longint RMParallelResourceManager::GetSlaveMax(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetSlaveRequirement()->GetResource(nRT)->GetMax();
}

longint RMParallelResourceManager::GetMasterMin(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetMasterRequirement()->GetResource(nRT)->GetMin();
}

longint RMParallelResourceManager::GetMasterMax(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetMasterRequirement()->GetResource(nRT)->GetMax();
}

longint RMParallelResourceManager::GetSharedMin(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetSharedRequirement()->GetResource(nRT)->GetMin();
}

longint RMParallelResourceManager::GetSlaveGlobalMin(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMin();
}

longint RMParallelResourceManager::GetSlaveGlobalMax(int nRT) const
{
	require(nRT < UNKNOWN);
	require(resourceRequirement != NULL);
	return resourceRequirement->GetGlobalSlaveRequirement()->GetResource(nRT)->GetMax();
}

///////////////////////////////////////////////////////////////////////
// Implementation de  RMResourceContainer

RMResourceContainer::RMResourceContainer()
{
	lvResources.SetSize(UNKNOWN);
}
RMResourceContainer::~RMResourceContainer() {}

RMResourceContainer* RMResourceContainer::Clone() const
{
	RMResourceContainer* clone;
	clone = new RMResourceContainer;
	clone->lvResources.CopyFrom(&lvResources);
	return clone;
}

longint RMResourceContainer::GetValue(int nResourceType) const
{
	require(nResourceType < UNKNOWN);
	return lvResources.GetAt(nResourceType);
}
void RMResourceContainer::SetValue(int nResourceType, longint lValue)
{
	require(nResourceType < UNKNOWN);
	lvResources.SetAt(nResourceType, lValue);
}

///////////////////////////////////////////////////////////////////////
// Implementation de  PLTestCluster

// Constructeur
PLTestCluster::PLTestCluster()
{
	Reset();
}

PLTestCluster::~PLTestCluster() {}

void PLTestCluster::SetTestLabel(const ALString& sValue)
{
	sName = sValue;
}

void PLTestCluster::SetHostNumber(int nValue)
{
	require(nValue > 0);
	nHostNumber = nValue;
}

void PLTestCluster::SetHostCores(int nCoresNumber)
{
	require(nCoresNumber > 0);
	nCoresNumberPerHost = nCoresNumber;
}

void PLTestCluster::SetHostMemory(longint lMemory)
{
	require(lMemory > 0);
	lMemoryPerHost = lMemory;
}

void PLTestCluster::SetHostDisk(longint lDisk)
{
	require(lDisk > 0);
	lDiskPerHost = lDisk;
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

void PLTestCluster::SetSystemConfig(int nConfig)
{
	require(nConfig >= 0);
	require(nConfig <= 2);
	nSystemConfig = nConfig;
}

void PLTestCluster::Reset()
{
	nHostNumber = -1;
	nCoresNumberPerHost = -1;
	nMaxCoreNumberPerHost = -1;
	nMaxCoreNumberOnSystem = -1;
	lMemoryPerHost = 0;
	lDiskPerHost = 0;
	lMinMasterMemory = 0;
	lMaxMasterMemory = LLONG_MAX;
	lMinSlaveMemory = 0;
	lMaxSlaveMemory = LLONG_MAX;
	lMinMasterDisk = 0;
	lMaxMasterDisk = LLONG_MAX;
	lMinSlaveDisk = 0;
	lMaxSlaveDisk = LLONG_MAX;
	lMinSharedMemory = 0;
	lMaxSharedMemory = LLONG_MAX;
	nProcessNumber = INT_MAX - 1;
	lMemoryLimitPerHost = LLONG_MAX;
	lDiskLimitPerHost = LLONG_MAX;
	lMemoryLimitPerProc = LLONG_MAX;
	lDiskLimitPerProc = LLONG_MAX;
	nSystemConfig = 0;
	lMasterSystemAtStart = 8 * lMB;
	lSlaveSystemAtStart = 8 * lMB;
	lMinSlaveGlobalMemory = 0;
	lMaxSlaveGlobalMemory = LLONG_MAX;
	lMinGlobalConstraintDisk = 0;
	sName = "Test Cluster";
}

void PLTestCluster::SetConstraintMasterMemory(longint lMin, longint lMax)
{
	require(lMin <= lMax);
	lMinMasterMemory = lMin;
	lMaxMasterMemory = lMax;
}

void PLTestCluster::SetConstraintSlaveMemory(longint lMin, longint lMax)
{
	require(lMin <= lMax);
	lMinSlaveMemory = lMin;
	lMaxSlaveMemory = lMax;
}

void PLTestCluster::SetConstraintMasterDisk(longint lMin, longint lMax)
{
	require(lMin <= lMax);
	lMinMasterDisk = lMin;
	lMaxMasterDisk = lMax;
}

void PLTestCluster::SetConstraintSlaveDisk(longint lMin, longint lMax)
{
	require(lMin <= lMax);
	lMinSlaveDisk = lMin;
	lMaxSlaveDisk = lMax;
}

void PLTestCluster::SetConstraintSharedMemory(longint lMin, longint lMax)
{
	require(lMin <= lMax);
	lMinSharedMemory = lMin;
	lMaxSharedMemory = lMax;
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

void PLTestCluster::SetSlaveProcessNumber(int nSlaveProcess)
{
	require(nSlaveProcess > 0);
	nProcessNumber = nSlaveProcess;
}

void PLTestCluster::SetSlaveGlobalMemoryMin(longint lMin)
{
	require(lMin >= 0);
	lMinSlaveGlobalMemory = lMin;
}

void PLTestCluster::SetSlaveGlobalMemoryMax(longint lMax)
{
	require(lMax >= 0);
	lMaxSlaveGlobalMemory = lMax;
}

void PLTestCluster::SetSlaveGlobalDiskMin(longint lMin)
{
	require(lMin >= 0);
	lMinGlobalConstraintDisk = lMin;
}

void PLTestCluster::SetMemoryLimitPerProc(longint lMax)
{
	require(lMax >= 0);
	lMemoryLimitPerProc = lMax;
}

void PLTestCluster::SetDiskLimitPerProc(longint lMax)
{
	require(lMax >= 0);
	lDiskLimitPerProc = lMax;
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
	RMResourceSystem* cluster;
	RMTaskResourceRequirement taskRequirement;
	RMTaskResourceGrant* resources;
	const int nCurrentMemoryLimit = RMResourceConstraints::GetMemoryLimit();
	const int nCurrentDiskLimit = RMResourceConstraints::GetDiskLimit();
	const int nCurrentCoreLimit = RMResourceConstraints::GetMaxCoreNumber();
	const int nCurrentCoreLimitOnHost = RMResourceConstraints::GetMaxCoreNumberPerHost();
	const longint lCurrentSlaveAtStart =
	    RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->GetMin();
	const longint lCurrentMasterAtStart =
	    RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->GetMin();
	const boolean bCurrentTracer = PLParallelTask::GetTracerResources();

	require(nHostNumber > 0);
	require(nCoresNumberPerHost > 0);
	require(lMemoryPerHost >= 0);
	require(lDiskPerHost >= 0);
	require(lMinMasterMemory != -1);
	require(lMaxMasterMemory != -1);
	require(lMinSlaveMemory != -1);
	require(lMaxSlaveMemory != -1);
	require(lMinMasterDisk != -1);
	require(lMaxMasterDisk != -1);
	require(lMinSlaveDisk != -1);
	require(lMaxSlaveDisk != -1);
	require(nProcessNumber != -1);
	require(lMemoryLimitPerHost != -1);
	require(lDiskLimitPerHost != -1);
	require(lMemoryLimitPerProc != -1);
	require(lDiskLimitPerProc != -1);

	resources = new RMTaskResourceGrant;

	// Affichage des traces de l'allocation des ressources
	PLParallelTask::SetTracerResources(2);

	// Creation d'un systeme synthetique
	cluster = RMResourceSystem::CreateSyntheticCluster(nHostNumber, nCoresNumberPerHost, lMemoryPerHost,
							   lDiskPerHost, nSystemConfig);

	// Modification des ressources au lancement
	RMTaskResourceRequirement::GetSlaveSystemAtStart()->GetResource(MEMORY)->Set(lSlaveSystemAtStart);
	RMTaskResourceRequirement::GetMasterSystemAtStart()->GetResource(MEMORY)->Set(lMasterSystemAtStart);

	// Affichae d'une entete du test
	cout << endl << "----------------------------------" << endl;
	cout << "\t\t" << sName << endl;
	cout << endl;

	// Construction des contraintes du programme
	// Du maitre
	taskRequirement.GetMasterRequirement()->GetMemory()->SetMin(lMinMasterMemory);
	taskRequirement.GetMasterRequirement()->GetMemory()->SetMax(lMaxMasterMemory);
	taskRequirement.GetMasterRequirement()->GetDisk()->SetMin(lMinMasterDisk);
	taskRequirement.GetMasterRequirement()->GetDisk()->SetMax(lMaxMasterDisk);

	// Des esclaves
	taskRequirement.GetSlaveRequirement()->GetMemory()->SetMin(lMinSlaveMemory);
	taskRequirement.GetSlaveRequirement()->GetMemory()->SetMax(lMaxSlaveMemory);
	taskRequirement.GetSlaveRequirement()->GetDisk()->SetMin(lMinSlaveDisk);
	taskRequirement.GetSlaveRequirement()->GetDisk()->SetMax(lMaxSlaveDisk);
	taskRequirement.GetGlobalSlaveRequirement()->GetMemory()->SetMin(lMinSlaveGlobalMemory);
	taskRequirement.GetGlobalSlaveRequirement()->GetMemory()->SetMax(lMaxSlaveGlobalMemory);
	taskRequirement.GetGlobalSlaveRequirement()->GetDisk()->SetMin(lMinGlobalConstraintDisk);

	// Des shared
	taskRequirement.GetSharedRequirement()->GetMemory()->SetMin(lMinSharedMemory);
	taskRequirement.GetSharedRequirement()->GetMemory()->SetMax(lMaxSharedMemory);

	// Nb de slaveProcess
	taskRequirement.SetMaxSlaveProcessNumber(nProcessNumber);
	taskRequirement.GetMasterSystemAtStart();

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

	// Disque par processus
	if (lDiskLimitPerProc == LLONG_MAX)
		RMResourceConstraints::SetDiskLimitPerProc(INT_MAX);
	else
		RMResourceConstraints::SetDiskLimitPerProc((int)(lDiskLimitPerProc / lMB));

	// Memoire par processus
	if (lMemoryLimitPerProc == LLONG_MAX)
		RMResourceConstraints::SetMemoryLimitPerProc(INT_MAX);
	else
		RMResourceConstraints::SetMemoryLimitPerProc((int)(lMemoryLimitPerProc / lMB));

	// Nombre de coeurs par machine
	if (nMaxCoreNumberPerHost == -1)
		RMResourceConstraints::SetMaxCoreNumberPerHost(INT_MAX);
	else
		RMResourceConstraints::SetMaxCoreNumberPerHost(nMaxCoreNumberPerHost);

	// Nombre de coeurs sur le systeme
	if (nMaxCoreNumberOnSystem == -1)
		RMResourceConstraints::SetMaxCoreNumber(INT_MAX);
	else
		RMResourceConstraints::SetMaxCoreNumber(nMaxCoreNumberOnSystem);

	// Resolution
	RMParallelResourceManager::ComputeGrantedResourcesForSystem(cluster, &taskRequirement, resources);

	// Affichage des ressource smanquantes
	if (resources->IsEmpty())
		cout << resources->GetMissingResourceMessage() << endl;

	// On verifie que les esclaves ont dans les bornes
	assert(resources->IsEmpty() or resources->GetMinSlaveMemory() >=
					   taskRequirement.GetSlaveRequirement()->GetMemory()->GetMin() +
					       taskRequirement.GetGlobalSlaveRequirement()->GetMemory()->GetMin() /
						   resources->GetSlaveNumber());
	assert(resources->IsEmpty() or resources->GetMinSlaveMemory() <=
					   lsum(taskRequirement.GetSlaveRequirement()->GetMemory()->GetMax(),
						taskRequirement.GetGlobalSlaveRequirement()->GetMemory()->GetMax() /
						    resources->GetSlaveNumber()));

	// Nettoyage
	delete resources;
	delete cluster;

	// Restitution des contraintes du systeme courant
	RMResourceConstraints::SetMemoryLimit(nCurrentMemoryLimit);
	RMResourceConstraints::SetDiskLimit(nCurrentDiskLimit);
	RMResourceConstraints::SetMaxCoreNumber(nCurrentCoreLimit);
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
	ivResourcesPolicy.SetSize(UNKNOWN);
	for (nResourceType = 0; nResourceType < UNKNOWN; nResourceType++)
	{
		ivResourcesPolicy.SetAt(nResourceType, slavePreferred);
	}

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
}

void RMTaskResourceRequirement::Write(ostream& ost) const
{
	ost << "--   Task requirements    --" << endl;
	ost << "Slave requirement :" << endl << *slaveRequirement;
	ost << "Master requirement :" << endl << *masterRequirement;
	ost << "Shared variables requirement :" << endl << *sharedRequirement;
	ost << "Slave global requirement :" << endl << *globalSlaveRequirement;
	ost << "Slave system at start :" << endl << slaveSystemAtStart;
	ost << "Master system at start :" << endl << masterSystemAtStart;
	ost << "Number of slaves processes : ";
	if (nSlaveProcessNumber == INT_MAX)
		ost << "INF" << endl;
	else
		ost << IntToString(nSlaveProcessNumber) << endl;
}

ALString RMTaskResourceRequirement::PolicyToString(POLICY policy)
{
	switch (policy)
	{
	case masterPreferred:
		return "Master Preferred";
		break;
	case slavePreferred:
		return "Slave Preferred";
		break;
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

///////////////////////////////////////////////////////////////////////
// Implementation de RMTaskResourceGrant

RMTaskResourceGrant::RMTaskResourceGrant()
{
	lvMissingResources.SetSize(UNKNOWN);
	bMissingResourceForProcConstraint = false;
	bMissingResourceForGlobalConstraint = false;
}

RMTaskResourceGrant::~RMTaskResourceGrant()
{
	oaResourceGrantWithRankIndex.RemoveAll();
	oaResourceGrant.DeleteAll();
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
	require(oaResourceGrant.GetSize() > 1);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(0))->GetMemory());
}

longint RMTaskResourceGrant::GetMasterDisk() const
{
	require(oaResourceGrant.GetSize() > 1);
	return (cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(0))->GetDiskSpace());
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
	return lvMissingResources.GetAt(MEMORY);
}
longint RMTaskResourceGrant::GetMissingDisk()
{
	return lvMissingResources.GetAt(DISK);
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
	lvMissingResources.Initialize();
	bMissingResourceForProcConstraint = false;
	oaResourceGrantWithRankIndex.RemoveAll();
	oaResourceGrant.DeleteAll();
}

void RMTaskResourceGrant::Write(ostream& ost) const
{
	int i;
	RMResourceGrant* resource;

	ost << "--    Granted resources    --" << endl;
	if (IsEmpty())
	{
		ost << " No resource available" << endl;
	}
	else
	{
		ost << "Rank"
		    << "\t"
		    << "Mem "
		    << "\t"
		    << "Disk "
		    << "\t"
		    << "Shared" << endl;

		// Affichage de la ressource de rang 0  : la maitre
		resource = cast(RMResourceGrant*, oaResourceGrantWithRankIndex.GetAt(0));
		ost << resource->GetRank() << "\t" << LongintToHumanReadableString(resource->GetMemory()) << "\t"
		    << LongintToHumanReadableString(resource->GetDiskSpace()) << "\t"
		    << LongintToHumanReadableString(resource->GetSharedMemory()) << endl;

		for (i = 0; i < oaResourceGrant.GetSize(); i++)
		{
			resource = cast(RMResourceGrant*, oaResourceGrant.GetAt(i));
			if (resource->GetRank() != 0)
			{
				ost << resource->GetRank() << "\t"
				    << LongintToHumanReadableString(resource->GetMemory()) << "\t"
				    << LongintToHumanReadableString(resource->GetDiskSpace()) << "\t"
				    << LongintToHumanReadableString(resource->GetSharedMemory()) << endl;

				if (i == 9 and oaResourceGrant.GetSize() > 11)
				{
					cout << "... " << oaResourceGrant.GetSize() - 10 << " remaining resources"
					     << endl;
					break;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
// Classe RMResourceGrant

RMResourceGrant::RMResourceGrant()
{
	int nResource;

	nRank = -1;
	lvResource.SetSize(UNKNOWN);
	lvSharedResource.SetSize(UNKNOWN);

	// Initialisation de toutes les ressources a -1
	for (nResource = 0; nResource < UNKNOWN; nResource++)
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
	require(nResource < UNKNOWN);
	require(lValue >= 0);
	lvSharedResource.SetAt(nResource, lValue);
}

longint RMResourceGrant::GetSharedResource(int nResource) const
{
	require(nResource < UNKNOWN);
	return lvSharedResource.GetAt(nResource);
}

void RMResourceGrant::SetResource(int nResource, longint lValue)
{
	require(nResource < UNKNOWN);
	require(lValue >= 0);
	lvResource.SetAt(nResource, lValue);
}

longint RMResourceGrant::GetResource(int nResource) const
{
	require(nResource < UNKNOWN);
	return lvResource.GetAt(nResource);
}

boolean RMResourceGrant::Check() const
{
	int nResource;
	for (nResource = 0; nResource < UNKNOWN; nResource++)
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

////////////////////////////////////////////////////
// Implementation de la classe PLKnapsackResources

PLKnapsackResources::PLKnapsackResources()
{
	resourceRequirement = NULL;
	grantedResource = NULL;
	oaHostResources = NULL;
}

PLKnapsackResources::~PLKnapsackResources()
{
	oaHostResources = NULL;
}

void PLKnapsackResources::SetRequirement(const RMTaskResourceRequirement* rq)
{
	require(rq != NULL);
	resourceRequirement = rq;
}

void PLKnapsackResources::SetGrantedResource(RMTaskResourceGrant* resource)
{
	require(resource != NULL);
	grantedResource = resource;
}

void PLKnapsackResources::SetHostResources(const ObjectArray* oaHosts)
{
	require(oaHosts != NULL);
	oaHostResources = oaHosts;
}

boolean PLKnapsackResources::DoesSolutionFitGlobalConstraint()
{
	PLClass* myClass;
	PLItem* item;
	int i;
	int nRT;
	int nProcNumberOnSystem;
	int nProcNumberOnHost;
	RMHostResource* resource;
	longint lRemainingResource;
	boolean bIsMissingResources;
	longint lGlobalResourceForSlave;
	longint lHostResource;

	require(oaHostResources != NULL);
	require(resourceRequirement != NULL);
	require(oaClasses.GetSize() == oaHostResources->GetSize());

	// Calcul du poids de la solution courante
	nProcNumberOnSystem = 0;
	for (i = 0; i < oaClasses.GetSize(); i++)
	{
		item = cast(PLItem*, cast(PLClass*, oaClasses.GetAt(i))->GetCurrentItem());
		if (item != NULL)
			nProcNumberOnSystem += item->GetWeight();
	}
	bIsMissingResources = false;

	// Pour chaque classe et ressource de Host correspondante
	for (i = 0; i < oaClasses.GetSize(); i++)
	{
		myClass = cast(PLClass*, oaClasses.GetAt(i));
		resource = cast(RMHostResource*, oaHostResources->GetAt(i));
		assert(myClass != NULL);
		item = myClass->GetCurrentItem();

		if (item != NULL)
		{
			nProcNumberOnHost = item->GetWeight();
			if (item->GetValueAt(0) == 1)
				nProcNumberOnHost--;

			for (nRT = 0; nRT < UNKNOWN; nRT++)
			{
				// Resource a ajouter a chaque esclave  pour prendre en compte la contrainte globale
				lGlobalResourceForSlave = 0;
				if (nProcNumberOnSystem > 1)
					lGlobalResourceForSlave = resourceRequirement->GetGlobalSlaveRequirement()
								      ->GetResource(nRT)
								      ->GetMin() /
								  (nProcNumberOnSystem - 1);

				// Ressources disponibles sur le host
				lHostResource = min(resource->GetResourceFree(nRT),
						    RMResourceConstraints::GetResourceLimit(nRT) * lMB);
				lHostResource = RMStandardResourceDriver::PhysicalToLogical(nRT, lHostResource);

				// Ressource restante
				lRemainingResource = lHostResource - (item->GetValueAt(nRT + 1) +
								      nProcNumberOnHost * lGlobalResourceForSlave);
				if (lRemainingResource < 0)
				{
					grantedResource->lvMissingResources.SetAt(nRT, -lRemainingResource);
					grantedResource->bMissingResourceForGlobalConstraint = true;
					grantedResource->sHostMissingResource = resource->GetHostName();
					bIsMissingResources = true;
				}
			}
		}
		if (bIsMissingResources)
			break;
	}

	return not bIsMissingResources;
}