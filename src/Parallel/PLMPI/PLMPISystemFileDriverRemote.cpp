// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPISystemFileDriverRemote.h"

PLMPISystemFileDriverRemote::PLMPISystemFileDriverRemote() {}

PLMPISystemFileDriverRemote::~PLMPISystemFileDriverRemote() {}

const char* PLMPISystemFileDriverRemote::GetDriverName() const
{
	return "Remote driver";
}

const char* PLMPISystemFileDriverRemote::GetScheme() const
{
	return "file";
}

const char* PLMPISystemFileDriverRemote::GetVersion() const
{
	return "1.0.0";
}

boolean PLMPISystemFileDriverRemote::IsReadOnly() const
{
	return false;
}

boolean PLMPISystemFileDriverRemote::Connect()
{
	// TODO peut etre que le lancement des FileServer devrait etre fait ici
	return true;
}

boolean PLMPISystemFileDriverRemote::Disconnect()
{
	return true;
}

boolean PLMPISystemFileDriverRemote::IsConnected() const
{
	return true;
}

longint PLMPISystemFileDriverRemote::GetSystemPreferredBufferSize() const
{
	return 64 * lKB;
}

boolean PLMPISystemFileDriverRemote::FileExists(const char* sFilePathName) const
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sFileName;
	ALString sHostName;
	boolean bIsFileExist;

	require(FileService::GetURIScheme(sFilePathName) == GetScheme());
	bIsFileExist = false;

	sFileName = FileService::GetURIFilePathName(sFilePathName);
	sHostName = FileService::GetURIHostName(sFilePathName);

	if (sHostName != GetLocalHostName())
	{

		// Lancement des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StartFileServers();

		// Recuperation du rang du serveur de fichier qui correspond au host
		bOk = PLMPITaskDriver::GetFileServerRank(sHostName, this);

		// Requete aupres du serveur de fichier idoine
		if (bOk)
		{
			// Envoi du nom du fichier
			context.Send(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_FILE_EXISTS);
			serializer.OpenForWrite(&context);
			serializer.PutString(sFileName);
			serializer.Close();

			// Reception de la reponse
			context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_FILE_EXISTS);
			serializer.OpenForRead(&context);
			bIsFileExist = serializer.GetBoolean();
			serializer.Close();
		}

		// Arret des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StopFileServers();
	}
	else
	{
		bIsFileExist = FileService::FileExists(sFileName);
	}
	return bIsFileExist;
}

boolean PLMPISystemFileDriverRemote::DirExists(const char* sFilePathName) const
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sFileName;
	ALString sHostName;
	boolean bIsFileExist;

	require(FileService::GetURIScheme(sFilePathName) == GetScheme());
	bIsFileExist = false;

	sFileName = FileService::GetURIFilePathName(sFilePathName);
	sHostName = FileService::GetURIHostName(sFilePathName);

	if (sHostName != GetLocalHostName())
	{

		// Lancement des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StartFileServers();

		// Recuperation du rang du serveur de fichier qui correspond au host
		bOk = PLMPITaskDriver::GetFileServerRank(sHostName, this);

		// Requete aupres du serveur de fichier idoine
		if (bOk)
		{
			// Envoi du nom du fichier
			context.Send(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_DIR_EXISTS);
			serializer.OpenForWrite(&context);
			serializer.PutString(sFileName);
			serializer.Close();

			// Reception de la reponse
			context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_DIR_EXISTS);
			serializer.OpenForRead(&context);
			bIsFileExist = serializer.GetBoolean();
			serializer.Close();
		}

		// Arret des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StopFileServers();
	}
	else
	{
		bIsFileExist = FileService::DirExists(sFileName);
	}
	return bIsFileExist;
}

longint PLMPISystemFileDriverRemote::GetFileSize(const char* sFilePathName) const
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	longint lFileSize;
	ALString sHostName;
	ALString sFileName;

	require(FileService::GetURIScheme(sFilePathName) == GetScheme());

	sFileName = FileService::GetURIFilePathName(sFilePathName);
	sHostName = FileService::GetURIHostName(sFilePathName);

	// Lancement des serveurs de fichier
	if (GetProcessId() == 0)
		PLMPITaskDriver::GetDriver()->StartFileServers();

	lFileSize = 0;
	bOk = PLMPITaskDriver::GetFileServerRank(sHostName, this);
	if (bOk)
	{
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
			    "<< Start remote access : GetFileSize");

		// Envoi du nom du fichier
		context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_SIZE);
		serializer.OpenForWrite(&context);
		serializer.PutString(sFileName);
		serializer.Close();

		// Reception de la taille
		context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_SIZE);
		serializer.OpenForRead(&context);
		lFileSize = serializer.GetLongint();
		serializer.Close();
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
			    "<< End remote access : GetFileSize");
	}

	// Arret des serveurs de fichier
	if (GetProcessId() == 0)
		PLMPITaskDriver::GetDriver()->StopFileServers();

	return lFileSize;
}

void* PLMPISystemFileDriverRemote::Open(const char* sFilePathName, char cMode)
{

	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sMessage;
	RemoteFile* file;

	require(cMode == 'r');

	require(FileService::GetURIScheme(sFilePathName) == GetScheme());

	// Lancement des serveurs de fichier
	if (GetProcessId() == 0)
		PLMPITaskDriver::GetDriver()->StartFileServers();

	bOk = PLMPITaskDriver::GetDriver()->GetFileServerRank(FileService::GetURIHostName(sFilePathName), this);

	if (bOk)
	{
		file = new RemoteFile;
		file->lPos = 0;
		file->bIsOpen = true;
		file->sFileName = sFilePathName;
		return file;
	}
	else
	{
		// Arret des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StopFileServers();
		return NULL;
	}
}

boolean PLMPISystemFileDriverRemote::Close(void* stream)
{
	RemoteFile* file;

	assert(stream != NULL);
	file = (RemoteFile*)stream; // TODO la methode cast ne fonctionne pas, pourquoi ?

	assert(file->bIsOpen == true);
	delete file;
	file = NULL;

	// Arret des serveurs de fichier
	if (GetProcessId() == 0)
		PLMPITaskDriver::GetDriver()->StopFileServers();
	return true;
}

longint PLMPISystemFileDriverRemote::Fread(void* ptr, size_t size, size_t count, void* stream)
{
	longint lRes;
	RemoteFile* file;
	boolean bOk;
	PLMPIMsgContext context;
	PLSerializer serializer;
	int nSizeToRecv;
	int nPos;
	int nLocalSize;
	MPI_Status status;
	// TODO etudier les perfs en faisant varier la taille du buffer
	const int nBlocSize = InputBufferedFile::InternalGetBlockSize();

	assert(stream != NULL);
	file = (RemoteFile*)stream;
	assert(file->bIsOpen == true);

	lRes = -1;
	errno = 0;
	bOk = PLMPITaskDriver::GetDriver()->GetFileServerRank(FileService::GetURIHostName(file->sFileName), this);

	if (bOk)
	{
		// Envoi du nom du fichier, de la position et de la taille du buffer
		// On envoie egalement la taille du bloc : le buffer resultat n'est pas envoye en entier mais par blocs
		context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_FREAD);
		serializer.OpenForWrite(&context);
		serializer.PutString(FileService::GetURIFilePathName(file->sFileName));
		serializer.PutLongint(file->lPos);
		serializer.PutLongint(size);
		serializer.PutLongint(count);
		serializer.PutInt(nBlocSize);
		serializer.Close();

		// Reception du nombre d'octets lus et du code d'erreur
		context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_FREAD);
		serializer.OpenForRead(&context);
		lRes = serializer.GetLongint();
		errno = serializer.GetInt();
		serializer.Close();

		// Reception du buffer (on n'utilise pas de serializer pour ne pas avoir a recopier la memoire)
		assert(lRes <= INT_MAX);
		nSizeToRecv = (int)lRes;
		nPos = 0;
		while (nSizeToRecv > 0)
		{
			nLocalSize = min(nSizeToRecv, nBlocSize);
			MPI_Recv(&((char*)ptr)[nPos], nLocalSize, MPI_CHAR,
				 PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_FREAD, MPI_COMM_WORLD,
				 &status);
			debug(; int nCount; MPI_Get_count(&status, MPI_CHAR, &nCount); assert(nCount == nLocalSize););

			nPos += nLocalSize;
			nSizeToRecv -= nLocalSize;
		}

		// Mise a jour de la position courante
		file->lPos += lRes;
	}
	if (errno != 0)
		lRes = -1;
	return lRes;
}

boolean PLMPISystemFileDriverRemote::SeekPositionInFile(longint lPosition, void* stream)
{
	RemoteFile* file;
	assert(stream != NULL);
	file = (RemoteFile*)stream;
	assert(file->bIsOpen == true);

	file->lPos = lPosition;

	return true;
}

const char* PLMPISystemFileDriverRemote::GetLastErrorMessage() const
{
	return strerror(errno);
}

longint PLMPISystemFileDriverRemote::Fwrite(const void* ptr, size_t size, size_t count, void* stream)
{
	assert(false);
	return 0;
}

boolean PLMPISystemFileDriverRemote::Flush(void* stream)
{
	assert(false);
	return false;
}

boolean PLMPISystemFileDriverRemote::RemoveFile(const char* sFilePathName) const
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sFileName;
	ALString sHostName;

	require(FileService::GetURIScheme(sFilePathName) == GetScheme());
	sFileName = FileService::GetURIFilePathName(sFilePathName);
	sHostName = FileService::GetURIHostName(sFilePathName);

	if (sHostName != GetLocalHostName())
	{
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
			    "<< Start remote access : RemoveFile");

		// Lancement des serveurs de fichiers
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StartFileServers();

		// Recuperation du rang du serveur de fichier qui correspond au host
		bOk = PLMPITaskDriver::GetFileServerRank(sHostName, this);

		// Requete
		if (bOk)
		{
			// Envoi du nom du fichier
			context.Send(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_REMOVE);
			serializer.OpenForWrite(&context);
			serializer.PutString(sFileName);
			serializer.Close();

			// Reception du tag de succes/echec
			context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_REMOVE);
			serializer.OpenForRead(&context);
			bOk = serializer.GetBoolean();
			serializer.Close();
		}

		// Arret des serveurs de fichier
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StopFileServers();

		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
			    "<< End remote access : RemoveFile");
	}
	else
	{
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
			    "<< Start IO write : RemoveFile");
		bOk = FileService::RemoveFile(sFileName);
		if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
			PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< End IO write : RemoveFile");
	}
	return bOk;
}

boolean PLMPISystemFileDriverRemote::MakeDirectory(const char* sPathName) const
{
	assert(false);
	return false;
}

boolean PLMPISystemFileDriverRemote::RemoveDirectory(const char* sFilePathName) const
{
	assert(false);
	return false;
}

longint PLMPISystemFileDriverRemote::GetDiskFreeSpace(const char* sPathName) const
{
	assert(false);
	return 0;
}

boolean PLMPISystemFileDriverRemote::ReserveExtraSize(longint lSize, void* stream)
{
	assert(false);
	return false;
}
