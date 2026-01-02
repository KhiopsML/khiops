// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLShared_ResourceRequirement.h"

///////////////////////////////////////////////////////////////////////
// Classe PLShared_ResourceRequirement
PLShared_ResourceRequirement::PLShared_ResourceRequirement() {}

PLShared_ResourceRequirement::~PLShared_ResourceRequirement() {}

void PLShared_ResourceRequirement::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const RMResourceRequirement* r;
	PLShared_ObjectArray shared_oa(new PLShared_PhysicalResource);

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	r = cast(RMResourceRequirement*, o);
	shared_oa.SerializeObject(serializer, &r->oaResources);
}

void PLShared_ResourceRequirement::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMResourceRequirement* r;
	PLShared_ObjectArray shared_oa(new PLShared_PhysicalResource);

	require(serializer->IsOpenForRead());
	require(o != NULL);

	r = cast(RMResourceRequirement*, o);
	r->oaResources.DeleteAll();
	shared_oa.DeserializeObject(serializer, &r->oaResources);
}

Object* PLShared_ResourceRequirement::Create() const
{
	return new RMResourceRequirement;
}

boolean PLShared_ResourceRequirement::Test()
{
	RMResourceRequirement* requirementToSerialize;
	RMResourceRequirement* requirementFromSerialize;
	PLShared_ResourceRequirement shared;
	PLSerializer serializer;

	// Premier test avec les valeurs par defaut
	// Serialisation
	requirementToSerialize = new RMResourceRequirement;
	serializer.OpenForWrite(NULL);
	shared.SerializeObject(&serializer, requirementToSerialize);
	serializer.Close();

	// De Serialisation
	requirementFromSerialize = new RMResourceRequirement;
	serializer.OpenForRead(NULL);
	shared.DeserializeObject(&serializer, requirementFromSerialize);
	serializer.Close();
	cout << "To serializer " << endl << *requirementToSerialize << endl;
	cout << "From serializer " << endl << *requirementFromSerialize << endl;
	delete requirementToSerialize;
	delete requirementFromSerialize;

	// Second test avec des  valeurs particulieres
	// Serialisation
	serializer.Initialize();
	requirementToSerialize = new RMResourceRequirement;
	requirementToSerialize->GetMemory()->SetMin(1 * lGB);
	requirementToSerialize->GetMemory()->SetMax(2 * lGB);
	requirementToSerialize->GetDisk()->SetMin(1 * lTB);
	requirementToSerialize->GetDisk()->SetMax(2 * lTB);
	serializer.OpenForWrite(NULL);
	shared.SerializeObject(&serializer, requirementToSerialize);
	serializer.Close();

	// De Serialisation
	requirementFromSerialize = new RMResourceRequirement;
	serializer.OpenForRead(NULL);
	shared.SerializeObject(&serializer, requirementFromSerialize);
	serializer.Close();
	cout << "To serializer " << endl << *requirementToSerialize << endl;
	cout << "From serializer " << endl << *requirementFromSerialize << endl;
	delete requirementToSerialize;
	delete requirementFromSerialize;

	return true;
}

///////////////////////////////////////////////////////////////////////
// Classe PLShared_PhysicalResource
PLShared_PhysicalResource::PLShared_PhysicalResource() {}

PLShared_PhysicalResource::~PLShared_PhysicalResource() {}

void PLShared_PhysicalResource::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const RMPhysicalResource* pr;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	pr = cast(RMPhysicalResource*, o);
	serializer->PutLongint(pr->GetMin());
	serializer->PutLongint(pr->GetMax());
}

void PLShared_PhysicalResource::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	RMPhysicalResource* pr;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	pr = cast(RMPhysicalResource*, o);
	pr->SetMin(serializer->GetLongint());
	pr->SetMax(serializer->GetLongint());
}

Object* PLShared_PhysicalResource::Create() const
{
	return new RMPhysicalResource;
}
