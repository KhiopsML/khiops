// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLRemoteFileService.h"

int PLRemoteFileService::nFileHdfsIndex = 0;
boolean PLRemoteFileService::bRemoteIsNeverLocal = true;

boolean PLRemoteFileService::FileExists(const ALString& sFileURI)
{
	ALString sLocalFileName;
	SystemFile fileHandle;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileURI))
		sLocalFileName = FileService::GetURIFilePathName(sFileURI);
	else
		sLocalFileName = sFileURI;

	return fileHandle.FileExists(sLocalFileName);
}

boolean PLRemoteFileService::DirExists(const ALString& sFileURI)
{
	ALString sLocalFileName;
	SystemFile fileHandle;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileURI))
		sLocalFileName = FileService::GetURIFilePathName(sFileURI);
	else
		sLocalFileName = sFileURI;

	return fileHandle.DirExists(sLocalFileName);
}

longint PLRemoteFileService::GetFileSize(const ALString& sFileURI)
{
	ALString sLocalFileName;
	SystemFile fileHandle;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileURI))
		sLocalFileName = FileService::GetURIFilePathName(sFileURI);
	else
		sLocalFileName = sFileURI;

	return fileHandle.GetFileSize(sLocalFileName);
}

boolean PLRemoteFileService::RemoveFile(const ALString& sFileURI)
{
	ALString sLocalFileName;
	SystemFile fileHandle;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileURI))
		sLocalFileName = FileService::GetURIFilePathName(sFileURI);
	else
		sLocalFileName = sFileURI;

	return fileHandle.RemoveFile(sLocalFileName);
}

boolean PLRemoteFileService::CopyFile(const ALString& sSourceURI, const ALString& sDestPath)
{
	boolean bOk = false;
	boolean bIsSourceANSI;
	boolean bIsDestANSI;

	// Le cas fichier distant en destination n'est pas traite
	assert(FileService::GetURIScheme(sDestPath) != FileService::sRemoteScheme);
	bIsSourceANSI = false;
	bIsDestANSI = false;

	if (RemoteIsLocal(sSourceURI) or FileService::GetURIScheme(sSourceURI) == "")
		bIsSourceANSI = true;

	if (RemoteIsLocal(sDestPath) or FileService::GetURIScheme(sDestPath) == "")
		bIsDestANSI = true;

	// Pour les copie vers ou depuis ANSI, on utilise les methodes implementees dans les drivers
	if (bIsSourceANSI and not bIsDestANSI)
	{
		// ANSI vers HDFS
		bOk = SystemFile::CopyFileFromLocal(FileService::GetURIFilePathName(sSourceURI), sDestPath);
	}
	else
	{
		if (not bIsSourceANSI and bIsDestANSI)
		{
			// HDFS vers ANSI
			bOk = SystemFile::CopyFileToLocal(sSourceURI, FileService::GetURIFilePathName(sDestPath));
		}
		else
		{
			// Cas generique : copie de HDFS vers S3, REMOTE vers HDFS, REMOTE vers ANSI
			bOk = CopyFileGeneric(sSourceURI, sDestPath);
		}
	}
	return bOk;
}

boolean PLRemoteFileService::CreateEmptyFile(const ALString& sFilePathName)
{
	return SystemFile::CreateEmptyFile(sFilePathName);
}

boolean PLRemoteFileService::MakeDirectory(const ALString& sPathName)
{
	return SystemFile::MakeDirectory(sPathName);
}

boolean PLRemoteFileService::MakeDirectories(const ALString& sPathName)
{
	return SystemFile::MakeDirectories(sPathName);
}

longint PLRemoteFileService::GetDiskFreeSpace(const ALString& sPathName)
{
	return SystemFile::GetDiskFreeSpace(sPathName);
}

int PLRemoteFileService::GetPreferredBufferSize(const ALString& sFileURI)
{
	SystemFileDriver* driver;
	ALString sLocalFileName;
	longint lPreferredSize;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileURI))
		sLocalFileName = FileService::GetURIFilePathName(sFileURI);
	else
		sLocalFileName = sFileURI;

	driver = SystemFileDriverCreator::LookupDriver(sLocalFileName, NULL);
	if (driver == NULL)
		return SystemFile::nDefaultPreferredBufferSize;
	else
	{
		lPreferredSize = driver->GetSystemPreferredBufferSize();

		// On borne par les min et max
		lPreferredSize = min(lPreferredSize, (longint)SystemFile::nMaxPreferredBufferSize);
		lPreferredSize = max(lPreferredSize, (longint)SystemFile::nMinPreferredBufferSize);
		return (int)lPreferredSize;
	}
}

boolean PLRemoteFileService::BuildOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName)
{
	ALString sTmpDir;
	boolean bOk = true;
	ALString sScheme;
	ALString sFileName;

	sScheme = FileService::GetURIScheme(sPathName);

	// Si le fichier est sur hdfs, on le cree d'abord en local et on le copiera ensuite sur HDFS
	if (sScheme != FileService::sRemoteScheme and sScheme != "")
	{
		// On n'utilise pas forcement le repertoire applicatif car il n'a pas encore ete renseigne par
		// l'utilisateur
		if (FileService::GetApplicationTmpDir() != "")
			sTmpDir = FileService::GetApplicationTmpDir();
		else
			sTmpDir = FileService::GetSystemTmpDir();

		// Creation du nouveau nom de fichier dans le repertoire temporaire  : on ajoute le prefixe copy+index+_
		// L'index statique augmente a chaque creation, on evite ainsi les collisions entre noms de fichier
		// (ce qui peut etre problematique car il ya un bug lorsque un fichier CRC existe prealablement a la
		// creation d'un fichier du meme nom dur HDFS)
		sFileName = sTmpDir + FileService::GetFileSeparator() + "copy" + IntToString(++nFileHdfsIndex) + "_" +
			    FileService::GetFileName(sPathName);
		sWorkingFileName = FileService::CreateNewFile(sFileName);
		if (sWorkingFileName == "")
		{
			bOk = false;
			Global::AddError("File", sFileName, "Unable to create temporary file");
		}
	}
	else
	{
		sWorkingFileName = sPathName;
	}

	return bOk;
}

boolean PLRemoteFileService::CleanOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName)
{
	boolean bOk = true;

	// Si le fichier est sur HDFS, on le copie vers HDFS et on supprime la copie locale
	if (sPathName != sWorkingFileName)
	{
		assert(FileService::GetURIScheme(sPathName) != "");

		bOk = PLRemoteFileService::CopyFile(sWorkingFileName, sPathName);
		FileService::RemoveFile(sWorkingFileName);
	}
	sWorkingFileName = "";
	return bOk;
}

boolean PLRemoteFileService::BuildInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName)
{
	ALString sTmpDir;
	boolean bOk = true;

	// Si le fichier est sur hdfs, on le copie en local
	if (FileService::GetURIScheme(sPathName) != "")
	{
		// On n'utilise pas forcement le repertoire applicatif car il n'a pas encore ete renseigne par
		// l'utilisateur
		if (FileService::GetApplicationTmpDir() != "")
			sTmpDir = FileService::GetApplicationTmpDir();
		else
			sTmpDir = FileService::GetSystemTmpDir();

		sWorkingFileName = FileService::CreateNewFile(sTmpDir + FileService::GetFileSeparator() + "copy" +
							      IntToString(++nFileHdfsIndex) + "_" +
							      FileService::GetFileName(sPathName));

		bOk = sWorkingFileName != "";
		if (bOk)
		{
			bOk = PLRemoteFileService::CopyFile(sPathName, sWorkingFileName);
		}
		else
			Global::AddError("file", sPathName, "Unable to create working file");
	}
	else
	{
		sWorkingFileName = sPathName;
	}

	return bOk;
}

void PLRemoteFileService::CleanInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName)
{
	// Si le fichier est sur HDFS, on supprime la copie locale
	if (sPathName != sWorkingFileName)
	{
		assert(FileService::GetURIScheme(sPathName) != "");
		FileService::RemoveFile(sWorkingFileName);
	}
	sWorkingFileName = "";
}

boolean PLRemoteFileService::RemoteIsLocal(const ALString& sURI)
{
	ALString sScheme;
	boolean bIsLocal;
	sScheme = FileService::GetURIScheme(sURI);

	bIsLocal = false;

	// Si le scheme est file://
	if (sScheme == FileService::sRemoteScheme)
	{
		// Si le driver distant est present
		if (SystemFileDriverCreator::IsDriverRegisteredForScheme(FileService::sRemoteScheme))
		{
			// on bypasse le driver si le fichier est sur le host courant
			// (sauf si la constante bRemoteIsNeverLocal est a true)
			if (not bRemoteIsNeverLocal and FileService::GetURIHostName(sURI) == GetLocalHostName())
				bIsLocal = true;
		}
		else
		{
			// Si le driver n'est pas present, si le fichier est sur le host courant, il faut passer par le
			// driver ANSI
			if (FileService::GetURIHostName(sURI) == GetLocalHostName())
				bIsLocal = true;
		}
	}
	return bIsLocal;
}

void PLRemoteFileService::SetRemoteIsNeverLocal(boolean bValue)
{
	bRemoteIsNeverLocal = bValue;
}

boolean PLRemoteFileService::GetRemoteIsNeverLocal()
{
	return bRemoteIsNeverLocal;
}

boolean PLRemoteFileService::FileCompare(const ALString& sFileName1, const ALString& sFileName2)
{
	SystemFile fileHandle1;
	SystemFile fileHandle2;
	longint lFileSize;
	longint lPos;
	char c1;
	char c2;
	boolean bSame;
	boolean bOk;
	char* sBuffer1;
	char* sBuffer2;
	const int nBufferSize = 8 * lMB;
	int i;
	longint lRead1;
	longint lRead2;

	require(PLRemoteFileService::FileExists(sFileName1));
	require(PLRemoteFileService::FileExists(sFileName2));

	// Test sur la taille
	lFileSize = PLRemoteFileService::GetFileSize(sFileName1);
	if (PLRemoteFileService::GetFileSize(sFileName2) != lFileSize)
		return false;

	///////////////////////////////////////
	// Test caractere par caractere

	// Ouverture des fichiers
	bOk = fileHandle1.OpenInputFile(sFileName1);
	if (bOk)
	{
		bOk = fileHandle2.OpenInputFile(sFileName2);
		if (not bOk)
			fileHandle2.CloseInputFile(sFileName2);
	}

	// Comparaison
	bSame = bOk;
	if (bSame)
	{
		sBuffer1 = new char[nBufferSize];
		sBuffer2 = new char[nBufferSize];

		lPos = 0;
		// On parcours tout le fichier, sans se baser sur feof
		// En cas d'erreur, on est ainsi sur de s'arreter sur
		// le premier fichier en erreur (fgetc retourne EOF),
		// et au pire, on arrete apres la taille du fichier
		while (lPos < lFileSize)
		{
			lRead1 = fileHandle1.Read(sBuffer1, 1, nBufferSize);
			lRead2 = fileHandle2.Read(sBuffer2, 1, nBufferSize);
			ensure(lRead1 == lRead2);
			lPos += lRead1;
			for (i = 0; i < lRead1; i++)
			{
				c1 = sBuffer1[i];
				c2 = sBuffer2[i];
				if (c1 != c2)
				{
					bSame = false;
					break;
				}
			}
		}
		delete[] sBuffer1;
		delete[] sBuffer2;
	}

	// Fermeture des fichiers
	fileHandle1.CloseInputFile(sFileName1);
	fileHandle2.CloseInputFile(sFileName2);

	return bSame;
}

boolean PLRemoteFileService::CopyFileGeneric(const ALString& sSourceURI, const ALString& sDestURI)
{
	SystemFile fileInput;
	SystemFile fileOutput;
	char* sBuffer;
	int nRead;
	int nWrite;
	const int nBufferSize = 8 * lMB; // TODO adapter la taille du buffer en fonction du preferedSize
	ALString sTmp;
	boolean bOk;

	errno = 0;
	bOk = fileInput.OpenInputFile(sSourceURI);
	if (bOk)
	{
		bOk = fileOutput.OpenOutputFile(sDestURI);
		if (bOk)
		{
			sBuffer = GetHugeBuffer(8 * lMB);
			nRead = nBufferSize;
			while (nRead == nBufferSize)
			{
				nRead = (int)fileInput.Read(sBuffer, 1, nBufferSize);
				if (nRead == 0 and errno != 0)
					Global::AddError("File", sSourceURI,
							 sTmp + "Unable to read file (" +
							     fileInput.GetLastErrorMessage() + ")");

				if (nRead != 0)
				{
					nWrite = (int)fileOutput.Write(sBuffer, 1, nRead);
					if (nWrite == 0)
					{
						Global::AddError("File", sDestURI,
								 sTmp + "Unable to write file (" +
								     fileOutput.GetLastErrorMessage() + ")");
						bOk = false;
						break;
					}
				}
			}
			if (not fileOutput.CloseOutputFile(sDestURI))
			{
				bOk = false;
				Global::AddError("File", sDestURI,
						 sTmp + "Unable to close file (" + fileOutput.GetLastErrorMessage() +
						     ")");
			}
		}
		fileInput.CloseInputFile(sSourceURI);
	}
	return bOk;
}
