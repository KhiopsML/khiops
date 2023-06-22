// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPIFileServerSlave.h"

ObjectArray* PLMPIFileServerSlave::oaUserMessages = NULL;

PLMPIFileServerSlave::PLMPIFileServerSlave()
{
	// Classe singleton : oaUserMessages ne peut etre allouer qu'une seule fois
	require(oaUserMessages == NULL);
	oaUserMessages = new ObjectArray;

	// Positionnement du gestionnaire de messages
	Error::SetDisplayErrorFunction(CatchError);
	PLMPITaskDriver::GetDriver()->GetTracerPerformance()->SetActiveMode(false);
}

PLMPIFileServerSlave::~PLMPIFileServerSlave()
{
	assert(oaUserMessages->GetSize() == 0);
	oaUserMessages->DeleteAll();
	delete oaUserMessages;
	oaUserMessages = NULL;
}

void PLMPIFileServerSlave::CatchError(const Error* e)
{
	if (e->GetGravity() == Error::GravityFatalError)
	{
		cout << Error::BuildDisplayMessage(e)
		     << endl; // TODO a remplacer par Global::ShowError(e) qui n'est pas accessible, voir avec Marc
		MPI_Abort(MPI_COMM_WORLD, 0);
	}
	else
	{
		PLErrorWithIndex* error = new PLErrorWithIndex;
		error->SetError(e->Clone());
		oaUserMessages->Add(error);
	}
}

void PLMPIFileServerSlave::Run(boolean& bOrderToQuit)
{
	boolean bStopOrder;
	int nNewMessage;
	MPI_Status status;
	Timer timer;
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sParallelLogFileName;
	ALString sNewFileName;
	bStopOrder = false;   // Condition d'arret : a-t-on recu une notification d'arret
	bOrderToQuit = false; // Arret d'urgence : il faudra transmettre l'information a  PLMPISlaveLauncher

	tTimeBeforeSleep.Start();
	while (not bStopOrder)
	{
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &nNewMessage, &status);
		if (nNewMessage)
		{

			switch (status.MPI_TAG)
			{
			case MASTER_STOP_FILE_SERVERS:

				assert(status.MPI_SOURCE == 0);

				// Arret normal sur ordre du maitre
				MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_STOP_FILE_SERVERS, MPI_COMM_WORLD, &status);
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, MASTER_STOP_FILE_SERVERS);
				bStopOrder = true;

				if (PLMPITaskDriver::GetDriver()->GetTracerPerformance()->GetActiveMode())
				{
					PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace("<< Job stop [" +
												       sTaskName + "]");
					PLMPITaskDriver::GetDriver()->GetTracerPerformance()->PrintTracesToFile();
					PLMPITaskDriver::GetDriver()->GetTracerPerformance()->Clean();
				}
				break;

			case MASTER_QUIT:

				assert(status.MPI_SOURCE == 0);

				// Arret anormal sur ordre du maitre
				MPI_Recv(NULL, 0, MPI_CHAR, 0, MASTER_QUIT, MPI_COMM_WORLD, &status);
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddRecv(0, MASTER_QUIT);
				bStopOrder = true;
				bOrderToQuit = true;
				break;

			case MASTER_RESOURCES:

				// Reception du message
				context.Recv(MPI_COMM_WORLD, 0, MASTER_RESOURCES);
				serializer.OpenForRead(&context);
				debug(serializer.GetInt(); serializer.GetInt(););

				serializer.Close();

				// Initialisation des ressources
				PLMPITaskDriver::GetDriver()->InitializeResourceSystem();
				break;

			case FILE_SERVER_REQUEST_SIZE:

				// Taille du fichier
				// Reponse a la methode RemoteHandlers::GetFileSize
				GetFileSize(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_FILE_EXISTS:

				// Existence du fichier
				// Reponse a la methode PLMPITaskDriver::Remote_FileExists
				GetFileExist(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_DIR_EXISTS:

				// Existence du repertoir
				// Reponse a la methode PLMPITaskDriver::Remote_DirExists
				GetDirExist(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_REMOVE:

				// Suppression du fichier
				// Reponse a la methode PLMPITaskDriver::Remote_RemoveFile
				RemoveFile(status.MPI_SOURCE);
				break;

			case MASTER_LOG_FILE:

				context.Recv(MPI_COMM_WORLD, 0, MASTER_LOG_FILE);
				serializer.OpenForRead(&context);
				sParallelLogFileName = serializer.GetString();
				sTaskName = serializer.GetString();
				serializer.Close();

				sNewFileName =
				    FileService::BuildFileName(FileService::GetFilePrefix(sParallelLogFileName) + "_" +
								   sTaskName + "_" + IntToString(GetProcessId()),
							       FileService::GetFileSuffix(sParallelLogFileName));

				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->SetFileName(
				    FileService::BuildFilePathName(FileService::GetPathName(sParallelLogFileName),
								   sNewFileName));

				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->SetActiveMode(true);
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->SetSynchronousMode(false);
				PLMPITaskDriver::GetDriver()->GetTracerPerformance()->AddTrace(
				    "<< Job start [" + sTaskName + "," + GetLocalHostName() + "," + "FileServer" + "]");
				break;

			case MASTER_TRACER_MPI:
				GetTracerMPI()->SetActiveMode(true);

				context.Recv(MPI_COMM_WORLD, 0, MASTER_TRACER_MPI);
				serializer.OpenForRead(&context);
				sParallelLogFileName = serializer.GetString();
				if (sParallelLogFileName != "")
				{
					sNewFileName = FileService::BuildFileName(
					    FileService::GetFilePrefix(sParallelLogFileName) + "_" + sTaskName + "_" +
						IntToString(GetProcessId()),
					    FileService::GetFileSuffix(sParallelLogFileName));

					GetTracerMPI()->SetFileName(FileService::BuildFilePathName(
					    FileService::GetPathName(sParallelLogFileName), sNewFileName));
				}

				GetTracerMPI()->SetShortDescription(serializer.GetBoolean());
				GetTracerMPI()->SetSynchronousMode(serializer.GetBoolean());
				GetTracerMPI()->SetTimeDecorationMode(serializer.GetBoolean());
				serializer.Close();
				break;
			case FILE_SERVER_FREAD:
				Fread(status.MPI_SOURCE);
				break;

			default:
				cout << "FileServer : Unexpected message emitted by " << IntToString(status.MPI_SOURCE)
				     << " with tag " << IntToString(status.MPI_TAG) << endl;
			}

			// Remise a zero du timer de mise en sommeil
			tTimeBeforeSleep.Reset();
			tTimeBeforeSleep.Start();
		}

		// Mise en sommeil si on n'a rien fait depuis 10 ms
		if (tTimeBeforeSleep.GetElapsedTime() > PLMPITaskDriver::TIME_BEFORE_SLEEP)
		{
			SystemSleep(0.001);
		}
	}

	// Deconnexion du communicateur Process, appel de cette methode chez le maitre et chez les esclaves
	MPI_Barrier(*PLMPITaskDriver::GetProcessComm());        // Necessaire pour MPICH
	MPI_Comm_disconnect(PLMPITaskDriver::GetProcessComm()); // DISCONNECT COMM_PROCESS
}

void PLMPIFileServerSlave::Fread(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	char* sBuffer;
	longint lBeginPos;
	longint lCount;
	longint lSize;
	longint lRead;
	FILE* file;
	boolean bOk;
	int nSerializedErrno;
	int nPos;
	int nSendSize;
	int nBlocSize;

	lRead = 0;
	errno = 0;

	// Reception de la demande
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddRecv(nRank, FILE_SERVER_FREAD);
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_FREAD);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	lBeginPos = serializer.GetLongint();
	lSize = serializer.GetLongint();
	lCount = serializer.GetLongint();
	nBlocSize = serializer.GetInt();
	serializer.Close();

	// Allocation d'un buffer de grande taille
	assert(lCount <= INT_MAX);
	sBuffer = GetHugeBuffer((int)lCount);

	// Ouverture et lecture du fichier
	bOk = FileService::OpenInputBinaryFile(sFileName, file);
	if (bOk)
	{
		bOk = FileService::SeekPositionInBinaryFile(file, lBeginPos);

		if (bOk)
		{
			assert(lSize <= INT_MAX);
			lRead = fread(sBuffer, (size_t)lSize, (size_t)lCount, file);
		}

		// En cas d'erreur on renvoie un bufefr vide
		if (not bOk or ferror(file))
		{
			lRead = 0;
			nSerializedErrno = errno;
		}
		else
			nSerializedErrno = 0;

		fclose(file);
	}
	else
	{
		nSerializedErrno = errno;
	}

	// Envoi du resultat
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(nRank, FILE_SERVER_FREAD);
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_FREAD);
	serializer.OpenForWrite(&context);
	serializer.PutLongint(lRead);
	serializer.PutInt(nSerializedErrno);
	serializer.Close();

	// Envoi du buffer sans passer par un serializer pour ne pas avoir de recopie memoire
	nPos = 0;
	while (lRead > 0)
	{
		assert(lRead <= INT_MAX);
		nSendSize = min(nBlocSize, (int)lRead);
		MPI_Send(&sBuffer[nPos], nSendSize, MPI_CHAR, nRank, FILE_SERVER_FREAD, MPI_COMM_WORLD);
		nPos += nSendSize;
		lRead -= nSendSize;
	}
}

void PLMPIFileServerSlave::GetFileSize(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	longint lFileSize;

	// Reception du nom du fichier
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_SIZE);
	serializer.Initialize();
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	AddPerformanceTrace("<< Start IO read : GetFileSize");
	lFileSize = FileService::GetFileSize(sFileName);
	AddPerformanceTrace("<< End IO read : GetFileSize");

	// Envoi de la taille du fichier
	AddPerformanceTrace("<< Start SEND : GetFileSize");
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_SIZE);
	serializer.OpenForWrite(&context);
	serializer.PutLongint(lFileSize);
	serializer.Close();
	AddPerformanceTrace("<< End SEND : GetFileSize");
}

void PLMPIFileServerSlave::GetFileExist(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	boolean bFileExist;

	// Reception du nom du fichier
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_FILE_EXISTS);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	// Envoi de l'existence du fichier
	AddPerformanceTrace("<< Start IO read : FileExist");
	bFileExist = FileService::FileExists(sFileName);
	AddPerformanceTrace("<< End IO read : FileExist");

	AddPerformanceTrace("<< Start SEND : FileExist");
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_FILE_EXISTS);
	serializer.OpenForWrite(&context);
	serializer.PutBoolean(bFileExist);
	serializer.Close();
	AddPerformanceTrace("<< End SEND : FileExist");
}

void PLMPIFileServerSlave::GetDirExist(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	boolean bFileExist;

	// Reception du nom du fichier
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_DIR_EXISTS);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	// Envoi de l'existence du fichier
	AddPerformanceTrace("<< Start IO read : DirExist");
	bFileExist = FileService::DirExists(sFileName);
	AddPerformanceTrace("<< End IO read : DirExist");

	AddPerformanceTrace("<< Start SEND : DirExist");
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_DIR_EXISTS);
	serializer.OpenForWrite(&context);
	serializer.PutBoolean(bFileExist);
	serializer.Close();
	AddPerformanceTrace("<< End SEND : DirExist");
}

void PLMPIFileServerSlave::RemoveFile(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	boolean bOk;

	// Reception du nom du fichier
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_REMOVE);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	// Suppression du fichier
	AddPerformanceTrace("<< Start IO write : RemoveFile");
	bOk = FileService::RemoveFile(sFileName);
	AddPerformanceTrace("<< End IO write : RemoveFile");

	// Envoi du succes/echec
	AddPerformanceTrace("<< Start SEND : RemoveFile");
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_REMOVE);
	serializer.OpenForWrite(&context);
	serializer.PutBoolean(bOk);
	serializer.Close();
	AddPerformanceTrace("<< End SEND : RemoveFile");
}

void PLMPIFileServerSlave::SerializeErrors(PLSerializer* serializer) const
{
	require(serializer->IsOpenForWrite());

	PLShared_ObjectArray* shared_oa;

	// Serialisation des messages (warning) a envoyer vers l'utilisateur
	if (oaUserMessages->GetSize() > 0)
	{
		serializer->PutBoolean(true);
		shared_oa = new PLShared_ObjectArray(new PLSharedErrorWithIndex);
		shared_oa->SerializeObject(serializer, oaUserMessages);
		delete shared_oa;
		oaUserMessages->DeleteAll();
	}
	else
		serializer->PutBoolean(false);
}