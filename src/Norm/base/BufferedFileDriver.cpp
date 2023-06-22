// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "BufferedFileDriver.h"

BufferedFileDriver* BufferedFileDriverCreator::driverRemote = NULL;

void BufferedFileDriverCreator::SetDriverRemote(BufferedFileDriver* driver)
{
	driverRemote = driver;
}

BufferedFileDriver* BufferedFileDriverCreator::GetDriverRemote()
{
	return driverRemote;
}

boolean BufferedFileDriverCreator::IsLocal(const ALString& sSourcePath) const
{
	ALString sScheme;
	boolean bLocal;
	sScheme = FileService::GetURIScheme(sSourcePath);

	bLocal = false;
	if (sScheme != FileService::sRemoteScheme or GetDriverRemote() == NULL)
		bLocal = true;
	else if (not bRemoteIsNeverLocal and FileService::GetURIHostName(sSourcePath) == GetLocalHostName())
		bLocal = true;
	return bLocal;
}
