// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLBufferedFileDriverRemote.h"

/////////////////////////////////////////////
// Implementation de la classe PLBufferedFileDriverRemote

PLBufferedFileDriverRemote::PLBufferedFileDriverRemote() {}

PLBufferedFileDriverRemote::~PLBufferedFileDriverRemote() {}

PLBufferedFileDriverRemote* PLBufferedFileDriverRemote::Create() const
{
	return new PLBufferedFileDriverRemote;
}

boolean PLBufferedFileDriverRemote::OpenForRead(InputBufferedFile* bufferedFile)
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sMessage;

	require(FileService::GetURIScheme(bufferedFile->GetFileName()) == FileService::sRemoteScheme);

	bOk = PLMPITaskDriver::GetDriver()->GetFileServerRank(FileService::GetURIHostName(bufferedFile->GetFileName()),
							      bufferedFile);

	if (bOk)
	{
		// Envoi du nom du fichier
		context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_OPEN);
		serializer.OpenForWrite(&context);
		serializer.PutString(FileService::GetURIFilePathName(bufferedFile->sFileName));
		serializer.Close();

		// Reception de la taille et de errno
		context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_OPEN);
		serializer.OpenForRead(&context);
		bufferedFile->lFileSize = serializer.GetLongint();
		errno = serializer.GetInt();
		serializer.Close();
		if (bufferedFile->lFileSize == -1)
		{
			bOk = false;
			Global::AddError("File", bufferedFile->GetFileName(),
					 "Unable to open file " + FileService::GetLastSystemIOErrorMessage());
		}
		errno = 0;
	}
	return bOk;
}

boolean PLBufferedFileDriverRemote::OpenForWrite(OutputBufferedFile* buffer)
{
	assert(false);
	AddError("PLBufferedFileDriverRemote::OpenForWrite is not implemented");
	return false;
}

boolean PLBufferedFileDriverRemote::OpenForWriteAppend(OutputBufferedFile* buffer)
{
	assert(false);
	AddError("PLBufferedFileDriverRemote::OpenForWrite is not implemented");
	return false;
}

boolean PLBufferedFileDriverRemote::Close()
{
	return true;
}

void PLBufferedFileDriverRemote::Fill(InputBufferedFile* buffer, longint lBeginPos, boolean& bSkippedLine,
				      longint& lRealBeginPos)
{
	longint lNewBeginPos; // Permet d'optimiser le remplissage lorsque lBeginPos est superieur a lLastEndBufferPos
	boolean bEof;
	ALString sFileName;
	ALString sHostName;
	int nRead;
	boolean bErrorToDeserialize;
	longint lSizeRead;
	int nRealBufferSize;
	boolean bAllocationError;
	boolean bBolFound;
	boolean bEolFound;
	MPI_Status status;
	PLSerializer serializer;
	PLMPIMsgContext context;
	PLShared_ObjectArray* shared_oa;
	ObjectArray oaErrors;
	ALString sTmp;
	int i;

	GetTracerMPI()->SetActiveMode(false);
	nRealBufferSize = 0;
	bAllocationError = false;
	bBolFound = false;
	bEolFound = false;

	// Initialisation du buffer
	buffer->nPositionInBuffer = 0;
	buffer->nReadLineNumber = 0;
	buffer->nReadLineNumber = 0;
	buffer->nCurrentBufferSize = 0;
	require(buffer->nBufferSize >= buffer->InternalGetBlockSize());
	debug(int nPendingMessage = 0; if (PLMPITaskDriver::GetDriver()->nFileServerRank != -1)
		  MPI_Iprobe(PLMPITaskDriver::GetDriver()->nFileServerRank, MPI_ANY_TAG, MPI_COMM_WORLD,
			     &nPendingMessage, &status);
	      assert(not nPendingMessage););

	// Si on demande une position trop grande, inutile de passer par les serveurs de fichiers
	bEof = false;
	if (lBeginPos >= buffer->lFileSize)
	{
		bEof = true;
		lRealBeginPos = buffer->lFileSize;
		return;
	}

	// Optimisation : si lBeginPos est entre les lBeginPos+nChunkSize et lNextFilePos du dernier remplissage
	// On commence a lNextFilePos (car on est sur qu'il n'y a pas de debut de ligne)
	if (lBeginPos >= buffer->lLastEndBufferPos and lBeginPos < buffer->lNextFilePos)
	{
		lNewBeginPos = buffer->lNextFilePos;
	}
	else
	{
		lNewBeginPos = lBeginPos;
	}

	// Allocation du buffer, avec erreur potentielle
	// on alloue la taille du buffer ou la taille du fichier si celle-ci est plus petite qu'un bloc
	if (buffer->nBufferSize > buffer->lFileSize)
		buffer->nAllocatedBufferSize = (int)buffer->lFileSize;
	else
		buffer->nAllocatedBufferSize = buffer->nBufferSize;

	// On arrondi a une taille de bloc
	buffer->nAllocatedBufferSize =
	    ((buffer->nAllocatedBufferSize + buffer->InternalGetBlockSize() - 1) / buffer->InternalGetBlockSize()) *
	    buffer->InternalGetBlockSize();
	buffer->AllocateBuffer();
	if (buffer->bIsError)
	{
		buffer->nCurrentBufferSize = 0;
		lRealBeginPos = lBeginPos;
		buffer->lNextFilePos = -1;
		return;
	}

	sFileName = FileService::GetURIFilePathName(buffer->GetFileName());
	sHostName = FileService::GetURIHostName(buffer->GetFileName());

	// Demande du buffer au serveur
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(PLMPITaskDriver::GetDriver()->nFileServerRank,
					FILE_SERVER_REQUEST_FILL_REQUEST);
	context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_FILL_REQUEST);
	serializer.OpenForWrite(&context);
	serializer.PutString(CurrentPreciseTimestamp());
	serializer.PutString(sFileName);
	serializer.PutLongint(lNewBeginPos);
	serializer.PutInt(buffer->nBufferSize);
	serializer.PutLongint(buffer->GetFileSize());
	serializer.Close();

	// Reception des blocs
	i = 0;
	lSizeRead = 0;
	nRead = buffer->InternalGetBlockSize();
	while (nRead == buffer->InternalGetBlockSize())
	{
		// Allocation supplementaire si on recoit plus que prevu
		if (buffer->nAllocatedBufferSize == lSizeRead)
		{
			buffer->nAllocatedBufferSize += buffer->InternalGetBlockSize();
			buffer->AllocateBuffer();
			if (buffer->bIsError)
			{
				// En cas d'erreur d'allocation, Il faut continuer a recevoir sinon on aura un blocage
				// cote serveur par contre il faut recevoir sur un espace memoire valide et a la fin
				// vider tout le buffer (car erreur)
				bAllocationError = true;
			}
		}

		if (bAllocationError)
			i = 0;
		if (buffer->InternalGetAllocSize() <= buffer->InternalGetBlockSize())
		{
			MPI_Recv(buffer->InternalGetMonoBlockBuffer(), buffer->InternalGetBlockSize(), MPI_CHAR,
				 PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_FILL,
				 MPI_COMM_WORLD, &status);
		}
		else
		{
			MPI_Recv(buffer->InternalGetMultiBlockBuffer(i), buffer->InternalGetBlockSize(), MPI_CHAR,
				 PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_FILL,
				 MPI_COMM_WORLD, &status);
		}

		MPI_Get_count(&status, MPI_CHAR, &nRead);
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(PLMPITaskDriver::GetDriver()->nFileServerRank,
						FILE_SERVER_REQUEST_FILL);
		lSizeRead += nRead;
		i++;
	}

	// Reception du dernier message : les infos
	context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_FILL_INFO);
	serializer.OpenForRead(&context);

	bErrorToDeserialize = serializer.GetBoolean();
	if (bErrorToDeserialize)
	{
		shared_oa = new PLShared_ObjectArray(new PLSharedErrorWithIndex);
		shared_oa->DeserializeObject(&serializer, &oaErrors);
		delete shared_oa;
		for (i = 0; i < oaErrors.GetSize(); i++)
		{
			assert(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetIndex() == -1);
			Global::AddErrorObject(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetError());
		}
		oaErrors.DeleteAll();
	}

	buffer->bIsError = serializer.GetBoolean();
	if (not buffer->bIsError)
	{
		bBolFound = serializer.GetBoolean();
		bEolFound = serializer.GetBoolean();
		bSkippedLine = serializer.GetBoolean();
		bEof = serializer.GetBoolean();
		lRealBeginPos = serializer.GetLongint();
		nRealBufferSize = serializer.GetInt();
	}
	serializer.Close();
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddRecv(PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_FILL_INFO);

	if (bAllocationError or buffer->bIsError)
	{
		buffer->bIsError = true;
		nRealBufferSize = 0;
	}

	if (bEof)
		buffer->lNextFilePos = -1;
	else
	{
		buffer->lNextFilePos = lRealBeginPos + nRealBufferSize;
	}

	// Modification de la taille du buffer
	assert(buffer->nAllocatedBufferSize >= nRealBufferSize);
	buffer->nCurrentBufferSize = nRealBufferSize;

	// Si la fin de ligne n'a pas ete trouvee,
	// on recherche  la derniere fin de ligne et on adapte la taille du buffer
	if (bBolFound and not bEolFound)
	{
		assert(bSkippedLine);
		for (i = nRealBufferSize - 1; i > 0; i--)
		{
			if (buffer->GetBuffer()->GetAt(i) == '\n')
				break;
		}
		buffer->nCurrentBufferSize = i;
	}
	if (not bBolFound)
	{
		buffer->nCurrentBufferSize = 0;
	}

	buffer->nAllocatedBufferSize = buffer->nCurrentBufferSize;
	buffer->PruneBuffer();
	buffer->lLastEndBufferPos = lBeginPos + buffer->nBufferSize;
}

void PLBufferedFileDriverRemote::Read(InputBufferedFile* buffer, longint lBeginPos)
{
	ALString sFileName;
	ALString sHostName;
	int nRead;
	boolean bErrorToDeserialize;
	int nSizeRead;
	boolean bAllocationError;
	MPI_Status status;
	PLSerializer serializer;
	PLMPIMsgContext context;
	PLShared_ObjectArray* shared_oa;
	ObjectArray oaErrors;
	ALString sTmp;
	int i;

	GetTracerMPI()->SetActiveMode(false);

	// Initialisation du buffer
	buffer->nPositionInBuffer = 0;
	buffer->nReadLineNumber = 0;
	buffer->nReadLineNumber = 0;
	buffer->nCurrentBufferSize = 0;
	bAllocationError = false;
	require(buffer->nBufferSize >= buffer->InternalGetBlockSize());
	debug(int nPendingMessage = 0; if (PLMPITaskDriver::GetDriver()->nFileServerRank != -1)
		  MPI_Iprobe(PLMPITaskDriver::GetDriver()->nFileServerRank, MPI_ANY_TAG, MPI_COMM_WORLD,
			     &nPendingMessage, &status);
	      assert(not nPendingMessage););

	// Si on demande une position trop grande, inutile de passer par les serveurs de fichiers
	if (lBeginPos >= buffer->lFileSize)
	{
		return;
	}

	// Allocation du buffer, avec erreur potentielle
	// on alloue la taille du buffer ou la taille du fichier si celle-ci est plus petite qu'un bloc
	if (buffer->nBufferSize > buffer->lFileSize)
		buffer->nAllocatedBufferSize = (int)buffer->lFileSize;
	else
		buffer->nAllocatedBufferSize = buffer->nBufferSize;

	// On arrondi a une taille de bloc
	buffer->nAllocatedBufferSize =
	    ((buffer->nAllocatedBufferSize + buffer->InternalGetBlockSize() - 1) / buffer->InternalGetBlockSize()) *
	    buffer->InternalGetBlockSize();
	buffer->AllocateBuffer();
	if (buffer->bIsError)
	{
		buffer->nCurrentBufferSize = 0;
		buffer->lNextFilePos = -1;
		return;
	}

	sFileName = FileService::GetURIFilePathName(buffer->GetFileName());
	sHostName = FileService::GetURIHostName(buffer->GetFileName());

	// Demande du buffer au serveur
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_READ);
	context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_READ);
	serializer.OpenForWrite(&context);
	serializer.PutString(CurrentPreciseTimestamp());
	serializer.PutString(sFileName);
	serializer.PutLongint(lBeginPos);
	serializer.PutInt(buffer->nBufferSize);
	serializer.PutLongint(buffer->GetFileSize());
	serializer.Close();

	// Reception des blocs
	i = 0;
	nSizeRead = 0;
	nRead = buffer->InternalGetBlockSize();
	while (nRead == buffer->InternalGetBlockSize() and nSizeRead != buffer->nBufferSize)
	{
		// Allocation supplementaire si on recoit plus que prevu
		if (buffer->nAllocatedBufferSize == nSizeRead)
		{
			buffer->nAllocatedBufferSize += buffer->InternalGetBlockSize();
			buffer->AllocateBuffer();
			if (buffer->bIsError)
			{
				// En cas d'erreur d'allocation, Il faut continuer a recevoir sinon on aura un blocage
				// cote serveur par contre il faut recevoir sur un espace memoire valide et a la fin
				// vider tout le buffer (car erreur)
				bAllocationError = true;
			}
		}

		if (bAllocationError)
			i = 0;
		if (buffer->InternalGetAllocSize() <= buffer->InternalGetBlockSize())
		{
			MPI_Recv(buffer->InternalGetMonoBlockBuffer(), buffer->InternalGetBlockSize(), MPI_CHAR,
				 PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_READ,
				 MPI_COMM_WORLD, &status);
		}
		else
		{
			MPI_Recv(buffer->InternalGetMultiBlockBuffer(i), buffer->InternalGetBlockSize(), MPI_CHAR,
				 PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_READ,
				 MPI_COMM_WORLD, &status);
		}

		MPI_Get_count(&status, MPI_CHAR, &nRead);
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddRecv(PLMPITaskDriver::GetDriver()->nFileServerRank,
						FILE_SERVER_REQUEST_READ);
		nSizeRead += nRead;
		i++;
	}
	if (nSizeRead != buffer->nBufferSize and lBeginPos + nSizeRead < buffer->GetFileSize())
	{
		// Reception du dernier message : les infos
		context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank,
			     FILE_SERVER_REQUEST_READ_ERROR);
		serializer.OpenForRead(&context);

		bErrorToDeserialize = serializer.GetBoolean();
		if (bErrorToDeserialize)
		{
			shared_oa = new PLShared_ObjectArray(new PLSharedErrorWithIndex);
			shared_oa->DeserializeObject(&serializer, &oaErrors);
			delete shared_oa;
			for (i = 0; i < oaErrors.GetSize(); i++)
			{
				assert(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetIndex() == -1);
				Global::AddErrorObject(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetError());
			}
			oaErrors.DeleteAll();
		}
		buffer->bIsError = true;
	}

	if (bAllocationError or buffer->bIsError)
	{
		buffer->bIsError = true;
		buffer->nAllocatedBufferSize = 0;
	}
	else
	{
		buffer->nCurrentBufferSize = nSizeRead;
	}
}

longint PLBufferedFileDriverRemote::FindEOL(InputBufferedFile* buffer, longint lBeginPos, boolean& bEolFound)
{
	PLSerializer serializer;
	longint lPos;
	boolean bIsMessage;
	PLShared_ObjectArray* shared_oa;
	int i;
	ObjectArray oaErrors;
	PLMPIMsgContext context;
	ALString sHostName;
	ALString sFileName;

	sFileName = FileService::GetURIFilePathName(buffer->GetFileName());

	require(FileService::GetURIScheme(buffer->GetFileName()) == FileService::sRemoteScheme);
	require(PLMPITaskDriver::GetDriver()->nFileServerRank != -1);

	if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< Start remote access : FindEOL");

	context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_EOL);

	// Demande du buffer au serveur
	serializer.OpenForWrite(&context);
	serializer.PutString(sFileName);
	serializer.PutLongint(lBeginPos);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(&context);

	// ... des messages vers l'utilisateur
	bIsMessage = serializer.GetBoolean();
	if (bIsMessage)
	{
		shared_oa = new PLShared_ObjectArray(new PLSharedErrorWithIndex);
		shared_oa->DeserializeObject(&serializer, &oaErrors);
		delete shared_oa;
		for (i = 0; i < oaErrors.GetSize(); i++)
		{
			assert(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetIndex() == -1);
			Global::AddErrorObject(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetError());
		}
		oaErrors.DeleteAll();
	}

	// ... du resultat
	buffer->bIsError = serializer.GetBoolean();
	if (not buffer->bIsError)
	{
		lPos = serializer.GetLongint();
		bEolFound = serializer.GetBoolean();
	}
	else
	{
		lPos = 0;
		bEolFound = false;
	}

	serializer.Close();

	if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< End remote access : FindEOL");

	return lPos;
}

longint PLBufferedFileDriverRemote::GetFileSize(const ALString& sURI)
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	longint lFileSize;
	ALString sHostName;
	ALString sFileName;

	require(FileService::GetURIScheme(sURI) == FileService::sRemoteScheme);

	sFileName = FileService::GetURIFilePathName(sURI);
	sHostName = FileService::GetURIHostName(sURI);

	// Lancement des serveurs de fichiers
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

boolean PLBufferedFileDriverRemote::FillWithHeaderLine(InputBufferedFile* bufferedFile)
{
	ALString sFileName;
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bLineTooLong;
	boolean bErrorToDeserialize;
	PLShared_ObjectArray* shared_oa;
	ObjectArray oaErrors;
	boolean bOk;
	int nSaveBufferSize;
	CharVector cvHeader;
	int i;

	nSaveBufferSize = bufferedFile->GetBufferSize();

	require(FileService::GetURIScheme(bufferedFile->GetFileName()) == FileService::sRemoteScheme);
	require(PLMPITaskDriver::GetDriver()->nFileServerRank != -1);

	if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
		PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< Start remote access : FindEOL");

	sFileName = FileService::GetURIFilePathName(bufferedFile->GetFileName());
	context.Send(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_HEADER);

	// Demande du buffer au serveur
	serializer.OpenForWrite(&context);
	serializer.PutString(sFileName);
	serializer.PutInt(bufferedFile->nBufferSize);
	serializer.Close();

	// Deserialization
	context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::GetDriver()->nFileServerRank, FILE_SERVER_REQUEST_HEADER);
	serializer.OpenForRead(&context);
	bErrorToDeserialize = serializer.GetBoolean();
	if (bErrorToDeserialize)
	{
		shared_oa = new PLShared_ObjectArray(new PLSharedErrorWithIndex);
		shared_oa->DeserializeObject(&serializer, &oaErrors);
		delete shared_oa;
		for (i = 0; i < oaErrors.GetSize(); i++)
		{
			assert(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetIndex() == -1);
			Global::AddErrorObject(cast(PLErrorWithIndex*, oaErrors.GetAt(i))->GetError());
		}
		oaErrors.DeleteAll();
	}
	bOk = serializer.GetBoolean();
	bLineTooLong = serializer.GetBoolean();

	if (bOk and not bLineTooLong)
	{

		// Re-initialisation de InputBuffer
		// On memorise le FileSize, qui sera reinitialise par le Reset()
		longint lCurrentFileSize = bufferedFile->lFileSize;
		bufferedFile->Reset();
		bufferedFile->lFileSize = lCurrentFileSize;

		// Reset du charVector sous-jacent
		bufferedFile->ResetBuffer();
		bufferedFile->SetBufferSize(nSaveBufferSize);
		bufferedFile->fbBuffer.SetSize(nSaveBufferSize);

		// Reception du CharVector avec ecrasement du buffer
		serializer.GetCharVector(&bufferedFile->fbBuffer.cvBuffer);

		// Mise a jour de la taille du buffer
		bufferedFile->nCurrentBufferSize = bufferedFile->fbBuffer.cvBuffer.GetSize();
	}
	serializer.Close();
	bufferedFile->bIsError = not bOk;
	return not bLineTooLong;
}

boolean PLBufferedFileDriverRemote::Exist(const ALString& sFileURI)
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sFileName;
	ALString sHostName;
	boolean bIsFileExist;

	require(FileService::GetURIScheme(sFileURI) == FileService::sRemoteScheme);
	bIsFileExist = false;

	sFileName = FileService::GetURIFilePathName(sFileURI);
	sHostName = FileService::GetURIHostName(sFileURI);

	if (sHostName != GetLocalHostName())
	{
		// Lancement des serveurs de fichiers
		if (GetProcessId() == 0)
			PLMPITaskDriver::GetDriver()->StartFileServers();

		// Recuperation du rang du serveur de fichier qui correspond au host
		bOk = PLMPITaskDriver::GetFileServerRank(sHostName, this);

		// Requete aupres du serveur de fichier idoine
		if (bOk)
		{
			// Envoi du nom du fichier
			context.Send(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_EXIST);
			serializer.OpenForWrite(&context);
			serializer.PutString(sFileName);
			serializer.Close();

			// Reception de la reponse
			context.Recv(MPI_COMM_WORLD, PLMPITaskDriver::nFileServerRank, FILE_SERVER_REQUEST_EXIST);
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
		bIsFileExist = FileService::Exist(sFileName);
	}
	return bIsFileExist;
}

boolean PLBufferedFileDriverRemote::CreateEmptyFile(const ALString& sFilePathName)
{
	require(FileService::GetURIScheme(sFilePathName) == FileService::sRemoteScheme);
	AddError("Unable to create " + sFilePathName + " (feature not implemented)");
	assert(false);
	return false;
}
boolean PLBufferedFileDriverRemote::RemoveFile(const ALString& sFileURI)
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	boolean bOk;
	ALString sFileName;
	ALString sHostName;

	require(FileService::GetURIScheme(sFileURI) == FileService::sRemoteScheme);
	sFileName = FileService::GetURIFilePathName(sFileURI);
	sHostName = FileService::GetURIHostName(sFileURI);

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

boolean PLBufferedFileDriverRemote::CopyFile(const ALString& sSourceURI, const ALString& sDestURI)
{
	boolean bOk;
	SystemFile* fileHandle;
	InputBufferedFile inputFile;
	longint lPosition;

	// Lancement des serveurs de fichiers si on n'est pas dans une tache
	if (GetProcessId() == 0)
	{
		PLParallelTask::GetDriver()->StartFileServers();
	}

	require(FileService::GetURIScheme(sDestURI) != FileService::sRemoteScheme);

	// Ouverture du fichier de sortie
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenOutputFile(sDestURI);
	if (bOk)
	{
		inputFile.SetFileName(sSourceURI);

		// Ouverture du chunk en lecture
		bOk = inputFile.Open();
		if (bOk)
		{
			// Lecture du chunk par blocs et ecriture dans le fichier de sortie
			lPosition = 0;
			while (lPosition <= inputFile.GetFileSize())
			{
				// Lecture efficace d'un buffer
				bOk = inputFile.BasicFill(lPosition);
				lPosition += inputFile.GetBufferSize();

				if (bOk)
					bOk = inputFile.fbBuffer.WriteToFile(fileHandle,
									     inputFile.GetCurrentBufferSize(), NULL);
				if (not bOk)
					break;
			}

			// Fermeture du chunk
			inputFile.Close();
			if (not bOk)
				PLRemoteFileService::RemoveFile(sDestURI);
		}
		bOk = fileHandle->CloseOutputFile(sDestURI);
	}

	// Nettoyage
	if (fileHandle != NULL)
		delete fileHandle;

	// Arret des serveurs de fichiers
	if (GetProcessId() == 0)
		PLParallelTask::GetDriver()->StopFileServers();

	return bOk;
}

boolean PLBufferedFileDriverRemote::MakeDirectory(const ALString& sPathName)
{
	require(FileService::GetURIScheme(sPathName) == FileService::sRemoteScheme);
	AddError("Unable to create " + sPathName + " (feature not implemented)");
	assert(false);
	return false;
}

boolean PLBufferedFileDriverRemote::MakeDirectories(const ALString& sPathName)
{
	require(FileService::GetURIScheme(sPathName) == FileService::sRemoteScheme);
	AddError("Unable to create " + sPathName + " (feature not implemented)");
	assert(false);
	return false;
}

longint PLBufferedFileDriverRemote::GetDiskFreeSpace(const ALString& sPathName)
{
	require(FileService::GetURIScheme(sPathName) == FileService::sRemoteScheme);
	assert(false);
	return 0;
}