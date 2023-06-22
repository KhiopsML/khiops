// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_TaskResourceGrant.h"

PLShared_ResourceGrant::PLShared_ResourceGrant() {}

PLShared_ResourceGrant::~PLShared_ResourceGrant() {}

void PLShared_ResourceGrant::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const RMResourceGrant* rg;
	PLShared_ResourceContainer sharedContainer;
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	rg = cast(RMResourceGrant*, o);
	sharedContainer.SerializeObject(serializer, rg->rcResource);
	sharedContainer.SerializeObject(serializer, rg->rcSharedResource);
}

void PLShared_ResourceGrant::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMResourceGrant* rg;
	PLShared_ResourceContainer sharedContainer;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	rg = cast(RMResourceGrant*, o);
	sharedContainer.DeserializeObject(serializer, rg->rcResource);
	sharedContainer.DeserializeObject(serializer, rg->rcSharedResource);
}

PLShared_TaskResourceGrant::PLShared_TaskResourceGrant() {}

PLShared_TaskResourceGrant::~PLShared_TaskResourceGrant() {}

void PLShared_TaskResourceGrant::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const RMTaskResourceGrant* trg;
	PLShared_ResourceContainer sharedContainer;
	PLShared_ResourceGrant sharedResourceGrant;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	trg = cast(RMTaskResourceGrant*, o);
	sharedResourceGrant.SerializeObject(serializer, trg->slaveResourceGrant);
	sharedResourceGrant.SerializeObject(serializer, trg->masterResourceGrant);
	sharedContainer.SerializeObject(serializer, trg->rcMissingResources);
	serializer->PutBoolean(trg->bMissingResourceForProcConstraint);
	serializer->PutBoolean(trg->bMissingResourceForGlobalConstraint);
	serializer->PutString(trg->sHostMissingResource);
	serializer->PutIntVector(&trg->ivProcessWithResources);
	serializer->PutInt(trg->nProcessNumber);
}

void PLShared_TaskResourceGrant::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMTaskResourceGrant* trg;
	PLShared_ResourceContainer sharedContainer;
	PLShared_ResourceGrant sharedResourceGrant;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	trg = cast(RMTaskResourceGrant*, o);
	sharedResourceGrant.DeserializeObject(serializer, trg->slaveResourceGrant);
	sharedResourceGrant.DeserializeObject(serializer, trg->masterResourceGrant);
	sharedContainer.DeserializeObject(serializer, trg->rcMissingResources);
	trg->bMissingResourceForProcConstraint = serializer->GetBoolean();
	trg->bMissingResourceForGlobalConstraint = serializer->GetBoolean();
	trg->sHostMissingResource = serializer->GetString();
	serializer->GetIntVector(&trg->ivProcessWithResources);
	trg->nProcessNumber = serializer->GetInt();
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