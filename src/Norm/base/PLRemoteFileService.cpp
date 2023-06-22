// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLRemoteFileService.h"

int PLRemoteFileService::nFileHdfsIndex = 0;

boolean PLRemoteFileService::Exist(const ALString& sFileURI)
{
	BufferedFileDriver* bufferedfileDriver;
	boolean bExists;
	bExists = false;
	if (BufferedFile::GetFileDriverCreator()->IsLocal(sFileURI))
	{
		bExists = SystemFile::Exist(sFileURI);
	}
	else
	{
		bufferedfileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sFileURI, NULL);
		if (bufferedfileDriver != NULL)
		{
			bExists = bufferedfileDriver->Exist(sFileURI);
			delete bufferedfileDriver;
		}
	}
	return bExists;
}

longint PLRemoteFileService::GetFileSize(const ALString& sFileURI)
{
	BufferedFileDriver* bufferedfileDriver;
	longint lFileSize;

	lFileSize = 0;
	if (BufferedFile::GetFileDriverCreator()->IsLocal(sFileURI))
	{
		lFileSize = SystemFile::GetFileSize(sFileURI);
	}
	else
	{
		bufferedfileDriver =
		    InputBufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sFileURI, NULL);
		if (bufferedfileDriver != NULL)
		{
			lFileSize = bufferedfileDriver->GetFileSize(sFileURI);
			delete bufferedfileDriver;
		}
	}
	return lFileSize;
}

boolean PLRemoteFileService::RemoveFile(const ALString& sFileURI)
{
	BufferedFileDriver* bufferedfileDriver;
	boolean bOk;
	ALString sScheme;

	bOk = false;
	if (BufferedFile::GetFileDriverCreator()->IsLocal(sFileURI))
	{

		bOk = SystemFile::RemoveFile(sFileURI);
	}
	else
	{
		bufferedfileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sFileURI, NULL);
		if (bufferedfileDriver != NULL)
		{
			bOk = bufferedfileDriver->RemoveFile(sFileURI);
			delete bufferedfileDriver;
		}
	}
	return bOk;
}

boolean PLRemoteFileService::CopyFile(const ALString& sSourceURI, const ALString& sDestPath)
{
	boolean bOk = false;
	BufferedFileDriver* bufferedFileDriver;

	// Le cas fichier distant en destination n'est pas traite
	assert(FileService::GetURIScheme(sDestPath) != FileService::sRemoteScheme);

	if (BufferedFile::GetFileDriverCreator()->IsLocal(sSourceURI))
	{
		// Le fichier source est local ou distant avec un host local
		if (FileService::GetURIScheme(sSourceURI) == FileService::sRemoteScheme)
		{
			assert(FileService::GetURIHostName(sSourceURI) == GetLocalHostName());
			bOk = CopyFileLocal(FileService::GetURIFilePathName(sSourceURI), sDestPath);
		}
		else
		{
			bOk = CopyFileLocal(sSourceURI, sDestPath);
		}
	}
	else
	{
		// Le fichier source est distant
		assert(FileService::GetURIScheme(sSourceURI) == FileService::sRemoteScheme);
		bufferedFileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sSourceURI, NULL);
		if (bufferedFileDriver != NULL)
		{
			bOk = bufferedFileDriver->CopyFile(sSourceURI, sDestPath);
			delete bufferedFileDriver;
		}
	}
	return bOk;
}

boolean PLRemoteFileService::CreateEmptyFile(const ALString& sFilePathName)
{
	BufferedFileDriver* bufferedfileDriver;

	boolean bOk = false;

	if (BufferedFile::GetFileDriverCreator()->IsLocal(sFilePathName))
	{
		bOk = SystemFile::CreateEmptyFile(sFilePathName);
	}
	else
	{
		bufferedfileDriver =
		    BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sFilePathName, NULL);
		if (bufferedfileDriver != NULL)
		{
			bOk = bufferedfileDriver->CreateEmptyFile(sFilePathName);
			delete bufferedfileDriver;
		}
	}
	return bOk;
}

boolean PLRemoteFileService::MakeDirectory(const ALString& sPathName)
{
	BufferedFileDriver* bufferedfileDriver;
	boolean bOk = false;

	if (BufferedFile::GetFileDriverCreator()->IsLocal(sPathName))
	{
		bOk = SystemFile::MakeDirectory(sPathName);
	}
	else
	{
		bufferedfileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sPathName, NULL);
		if (bufferedfileDriver != NULL)
		{
			bOk = bufferedfileDriver->MakeDirectory(sPathName);
			delete bufferedfileDriver;
		}
	}
	return bOk;
}

boolean PLRemoteFileService::MakeDirectories(const ALString& sPathName)
{
	BufferedFileDriver* bufferedfileDriver;
	boolean bOk = false;

	if (BufferedFile::GetFileDriverCreator()->IsLocal(sPathName))
	{
		bOk = SystemFile::MakeDirectories(sPathName);
	}
	else
	{
		bufferedfileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sPathName, NULL);
		if (bufferedfileDriver != NULL)
		{
			bOk = bufferedfileDriver->MakeDirectories(sPathName);
			delete bufferedfileDriver;
		}
	}
	return bOk;
}

longint PLRemoteFileService::GetDiskFreeSpace(const ALString& sPathName)
{
	BufferedFileDriver* bufferedfileDriver;
	longint lFreeSpace;

	lFreeSpace = 0;
	if (BufferedFile::GetFileDriverCreator()->IsLocal(sPathName))
	{
		lFreeSpace = SystemFile::GetDiskFreeSpace(sPathName);
	}
	else
	{
		bufferedfileDriver = BufferedFile::GetFileDriverCreator()->CreateBufferedFileDriver(sPathName, NULL);
		if (bufferedfileDriver != NULL)
		{
			lFreeSpace = bufferedfileDriver->GetDiskFreeSpace(sPathName);
			delete bufferedfileDriver;
		}
	}
	return lFreeSpace;
}

const ALString PLRemoteFileService::URItoUserString(const ALString& sURI)
{
	ALString sUserFileName;
	// require(FileService::IsURI(sURI));

	/*if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1)
		sUserFileName = sURI;
		else
		sUserFileName = FileService::GetURIFileName(sURI);*/
	// TODO on n'a pas acces a RMResourceManager::GetResourceSystem()->GetHostNumber()
	return sURI;
}

boolean PLRemoteFileService::BuildOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName)
{
	ALString sTmpDir;
	boolean bOk = true;
	ALString sCheme;
	ALString sFileName;

	sCheme = FileService::GetURIScheme(sPathName);

	// Si le fichier est sur hdfs, on le cree d'abord en local et on le copiera ensuite sur HDFS
	if (sCheme != FileService::sRemoteScheme and sCheme != "")
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

boolean PLRemoteFileService::TestCount(const ALString& sFileURI, int nBufferSize)
{
	InputBufferedFile inputFile;
	boolean bOk;
	longint lBeginPos;
	longint lLinesCountWithNextField;
	longint lLinesCountWithBuffer;
	longint lLinesCountNumberWithSkip;
	boolean bEol;
	char* sField;
	int nFieldError;

	lLinesCountNumberWithSkip = 0;
	lLinesCountWithNextField = 0;
	lLinesCountWithBuffer = 0;

	cout << "File name " << sFileURI << endl;
	cout << "Exist " << PLRemoteFileService::Exist(sFileURI) << endl;
	cout << "File size " << PLRemoteFileService::GetFileSize(sFileURI) << endl;

	inputFile.SetFileName(sFileURI);
	assert(nBufferSize > 0);
	cout << "Buffer Size " << LongintToHumanReadableString(nBufferSize) << endl;
	inputFile.SetBufferSize(nBufferSize);
	bOk = inputFile.Open();
	if (bOk)
	{

		assert(inputFile.IsOpened());
		cout << "Input file size " << inputFile.GetFileSize() << endl;
		lBeginPos = 0;
		while (not inputFile.IsLastBuffer())
		{
			// Lecture d'un buffer
			inputFile.Fill(lBeginPos);
			lBeginPos += nBufferSize;
			if (inputFile.GetBufferSkippedLine())
				lLinesCountWithNextField++;
			lLinesCountWithBuffer += inputFile.GetBufferLineNumber();

			// Analyse du buffer
			while (not inputFile.IsBufferEnd())
			{
				bEol = false;
				int nField = 0;
				while (not bEol)
				{
					bEol = inputFile.GetNextField(sField, nFieldError);
					nField++;
				}
				cout << "# fields " << nField << endl;
				lLinesCountWithNextField++;
			}
		}
		assert(inputFile.IsLastBuffer());
		// Fermeture
		inputFile.Close();
	}

	inputFile.SetBufferSize(nBufferSize);
	inputFile.Open();
	lLinesCountNumberWithSkip = 0;
	if (inputFile.IsOpened())
	{
		lBeginPos = 0;
		while (not inputFile.IsLastBuffer())
		{
			// Lecture d'un buffer
			inputFile.Fill(lBeginPos);
			lBeginPos += nBufferSize;
			if (inputFile.GetBufferSkippedLine())
				lLinesCountNumberWithSkip++;

			// Analyse du buffer
			while (not inputFile.IsBufferEnd())
			{
				inputFile.SkipLine();
				lLinesCountNumberWithSkip++;
			}
		}
		assert(inputFile.IsLastBuffer());

		// Fermeture
		inputFile.Close();
	}

	cout << "lLinesCountNumberWithSkip " << lLinesCountNumberWithSkip << endl;
	cout << "lLinesCountWithNextField " << lLinesCountWithNextField << endl;
	cout << "lLinesCountWithBuffer " << lLinesCountWithBuffer << endl;
	cout << endl;
	bOk =
	    lLinesCountNumberWithSkip == lLinesCountWithNextField and lLinesCountWithNextField == lLinesCountWithBuffer;
	return bOk;
}

boolean PLRemoteFileService::CopyFileLocal(const ALString& sSourceURI, const ALString& sDestURI)
{
	boolean bOk;
	assert(FileService::GetURIScheme(sDestURI) != FileService::sRemoteScheme);
	assert(FileService::GetURIScheme(sSourceURI) != FileService::sRemoteScheme);

	if (FileService::GetURIScheme(sSourceURI) != "")
	{
		// Fichier source sur hdfs
		bOk = SystemFile::CopyFileToLocal(sSourceURI, sDestURI);
	}
	else
	{
		// Fichier destination sur hdfs
		bOk = SystemFile::CopyFileFromLocal(sSourceURI, sDestURI);
	}

	return bOk;
}