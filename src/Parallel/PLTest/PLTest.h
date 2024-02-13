// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PEFileSearchTask.h"
#include "PEHelloWorldTask.h"
#include "PEIOParallelTestTask.h"
#include "PEPiView.h"
#include "PEIOParallelTestTask.h"
#include "PEProtocolTestTask.h"
#include "PESerializerTestTask.h"
#include "PESerializerLongTestTask.h"
#include "PELullabyTask.h"
#include "PEProgressionTask.h"

#include "PLSharedVariable.h"
#include "PLSharedVector.h"
#include "PLSharedObject.h"
#include "PLShared_TaskResourceGrant.h"
#include "PLShared_ResourceRequirement.h"

#ifdef USE_MPI
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#endif // USE_MPI
