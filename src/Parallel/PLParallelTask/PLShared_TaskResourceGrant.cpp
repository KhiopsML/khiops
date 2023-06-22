// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_TaskResourceGrant.h"

PLShared_ResourceGrant::PLShared_ResourceGrant() {}

PLShared_ResourceGrant::~PLShared_ResourceGrant() {}

void PLShared_ResourceGrant::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	RMResourceGrant* rg;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	rg = cast(RMResourceGrant*, o);
	serializer->PutInt(rg->nRank);
	serializer->PutLongintVector(&rg->lvResource);
	serializer->PutLongintVector(&rg->lvSharedResource);
}

void PLShared_ResourceGrant::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMResourceGrant* rg;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	rg = cast(RMResourceGrant*, o);
	rg->nRank = serializer->GetInt();
	serializer->GetLongintVector(&rg->lvResource);
	serializer->GetLongintVector(&rg->lvSharedResource);
}

PLShared_TaskResourceGrant::PLShared_TaskResourceGrant() {}

PLShared_TaskResourceGrant::~PLShared_TaskResourceGrant() {}

void PLShared_TaskResourceGrant::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	RMTaskResourceGrant* trg;
	PLShared_ObjectArray shared_oa(new PLShared_ResourceGrant);

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	trg = cast(RMTaskResourceGrant*, o);
	shared_oa.SerializeObject(serializer, &trg->oaResourceGrant);
}

void PLShared_TaskResourceGrant::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMTaskResourceGrant* trg;
	PLShared_ObjectArray shared_oa(new PLShared_ResourceGrant);

	require(serializer->IsOpenForRead());
	require(o != NULL);

	trg = cast(RMTaskResourceGrant*, o);
	shared_oa.DeserializeObject(serializer, &trg->oaResourceGrant);

	// Reindexation des ressources
	trg->ReindexRank();
}

void PLShared_TaskResourceGrant::Test()
{
	RMResourceSystem* cluster;
	RMTaskResourceGrant* resourcesIn;
	RMTaskResourceGrant* resourcesOut;
	RMTaskResourceRequirement taskRequirement;
	PLSerializer serializer;
	PLShared_TaskResourceGrant sharedResource;
	int nHostNumber = 1;
	int nCoresNumberPerHost = 16;
	int nSystemConfig = 0;
	longint lMemoryPerHost = 40 * lGB;
	longint lDiskPerHost = 100 * lGB;

	// Creation d'un systeme synthetique
	cluster = RMResourceSystem::CreateSyntheticCluster(nHostNumber, nCoresNumberPerHost, lMemoryPerHost,
							   lDiskPerHost, nSystemConfig);

	// Resolution
	resourcesIn = new RMTaskResourceGrant;
	RMParallelResourceManager::ComputeGrantedResourcesForCluster(cluster, &taskRequirement, resourcesIn);

	// Affichage des resources
	cout << *resourcesIn;

	// Serialisation
	serializer.OpenForWrite(NULL);
	sharedResource.SerializeObject(&serializer, resourcesIn);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	resourcesOut = new RMTaskResourceGrant;
	sharedResource.DeserializeObject(&serializer, resourcesOut);
	serializer.Close();

	// Affichage des resources
	cout << endl << "\t-- After deserialization --" << endl;
	cout << *resourcesOut;

	// Nettoyage
	delete resourcesIn;
	delete resourcesOut;
	delete cluster;
}