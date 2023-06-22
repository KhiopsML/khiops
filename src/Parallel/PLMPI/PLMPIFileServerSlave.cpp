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
			case FILE_SERVER_OPEN:

				// Remplissage du buffer :
				// Reponse a la methode  RemoteHandlers::Fill
				timer.Reset();
				timer.Start();
				OpenFile(status.MPI_SOURCE);
				timer.Stop();
				break;

			case FILE_SERVER_REQUEST_FILL:

				// Remplissage du buffer :
				// Reponse a la methode  RemoteHandlers::Fill
				timer.Reset();
				timer.Start();
				Fill(status.MPI_SOURCE);
				timer.Stop();
				break;

			case FILE_SERVER_REQUEST_EOL:

				// Position de fin de ligne
				// Reponse a la methode RemoteHandlers::FindEol
				FindEOL(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_SIZE:

				// Taille du fichier
				// Reponse a la methode RemoteHandlers::GetFileSize
				GetFileSize(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_HEADER:

				// Remplissage avec le header
				// Reponse a la methode PLBufferedFileDriverRemote::FillWithHeaderLine
				FillHeader(status.MPI_SOURCE);
				break;

			case FILE_SERVER_REQUEST_EXIST:

				// Existence du fichier
				// Reponse a la methode PLMPITaskDriver::Remote_FileExist
				GetFileExist(status.MPI_SOURCE);
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
			case FILE_SERVER_REQUEST_FILL_REQUEST:
				Fill(status.MPI_SOURCE);
				break;
			case FILE_SERVER_REQUEST_READ:
				Read(status.MPI_SOURCE);
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

void PLMPIFileServerSlave::OpenFile(int nRank) const
{
	PLSerializer serializer;
	PLMPIMsgContext context;
	ALString sFileName;
	longint lFileSize;

	// Reception du nom du fichier
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_OPEN);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	assert(sFileName != "");
	assert(FileService::GetURIScheme(sFileName) == "");

	errno = 0;
	lFileSize = FileService::GetFileSize(sFileName);

	// Si la taille est nulle, c'est peut etre que le fichier n'existe pas
	if (lFileSize == 0 and errno != 0)
	{
		lFileSize = -1;
	}

	// Envoi de la taille et de errno (qui devra etre pris en compte si errno!=0)
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_OPEN);
	serializer.OpenForWrite(&context);
	serializer.PutLongint(lFileSize);
	serializer.PutInt(errno);
	serializer.Close();
}

void PLMPIFileServerSlave::Fill(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	int nBufferSize;
	ALString sTimestampId;
	longint lBeginPos;
	longint lFileSize;
	boolean bVerbose = false;
	boolean bOk = false;
	SystemFile* fileHandle;
	longint lBufferBeginPos;
	int nBeginLinePos;
	int nBufferSizeLimit;
	boolean bBolFound;
	int nExtraPartPos;
	longint lCummulatedSentSize;
	int nYetToRead;
	boolean bEolFound;
	int nSizeToRead;
	int nLocalRead;
	int nStartPos;
	char* sBuffer;
	boolean bIsError;
	int nEolPos;
	int nCurrentBufferSize;
	boolean bEof;
	boolean bIsSkippedLine;
	int nSizeToSend;
	boolean bIsOpen;
	ALString sTmp;
	boolean bHaveToSearchEol;

	sBuffer = NewCharArray(InputBufferedFile::InternalGetBlockSize());
	bIsSkippedLine = false;
	bEolFound = false;
	bBolFound = false;
	bIsError = false;
	bEof = false;
	nCurrentBufferSize = 0;
	lCummulatedSentSize = 0;
	nBeginLinePos = 0;
	lBufferBeginPos = 0;
	nBufferSizeLimit = 0;
	nSizeToSend = 0;
	fileHandle = NULL;
	nEolPos = 0;

	GetTracerMPI()->SetActiveMode(false);

	// Reception de la demande
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddRecv(nRank, FILE_SERVER_REQUEST_FILL_REQUEST);
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_FILL_REQUEST);
	serializer.OpenForRead(&context);
	sTimestampId = serializer.GetString();
	sFileName = serializer.GetString();
	lBeginPos = serializer.GetLongint();
	nBufferSize = serializer.GetInt();
	lFileSize = serializer.GetLongint();
	serializer.Close();

	if (bVerbose)
		cout << endl
		     << "[" << GetProcessId() << "] "
		     << "Fill at pos " << LongintToReadableString(lBeginPos) << " chunk size "
		     << LongintToReadableString(nBufferSize) << " file size " << LongintToReadableString(lFileSize)
		     << endl;

	bIsOpen = false;
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenInputFile(sFileName);
	if (bOk)
	{
		bIsOpen = true;
		bOk =
		    InputBufferedFile::FindBol(lBeginPos, nBufferSize, lFileSize, fileHandle, nBeginLinePos, bBolFound);
		lBufferBeginPos = lBeginPos + nBeginLinePos;
		if (bBolFound)
			fileHandle->SeekPositionInFile(lBufferBeginPos);

		if (lBeginPos + nBufferSizeLimit > lFileSize)
			nBufferSizeLimit = (int)(lFileSize - lBeginPos);
		if (bVerbose)
		{
			if (bBolFound)
				cout << endl
				     << "[" << GetProcessId() << "] "
				     << "Begin line at " << LongintToReadableString(lBufferBeginPos) << " (initial pos "
				     << LongintToReadableString(lBeginPos) << ", nBeginLinePos "
				     << LongintToReadableString(nBeginLinePos) << ")"
				     << " Buffer Size " << LongintToReadableString(nBufferSize) << " file size "
				     << LongintToHumanReadableString(lFileSize) << endl;
			else
				cout << endl
				     << "[" << GetProcessId() << "] "
				     << "Begin line not found "
				     << " (initial pos " << LongintToReadableString(lBeginPos) << ")"
				     << " Buffer Size " << LongintToHumanReadableString(nBufferSize) << endl;
		}
	}

	if (bOk)
	{
		// Position a partir de laquelle il faut rechercher eol
		nExtraPartPos = nBufferSize - nBeginLinePos - 1;

		// Taille limite du buffer pour ne pas detecter des lignes plus grandes que GetMaxLineLength
		nBufferSizeLimit = nExtraPartPos + InputBufferedFile::GetMaxLineLength();

		// Lecture bloc par bloc pour trouver le prochain
		nYetToRead = nBufferSizeLimit;
		bEolFound = false;

		while (bBolFound and not bEolFound and nYetToRead > 0 and not bEof and bOk)
		{
			if (nYetToRead > InputBufferedFile::InternalGetBlockSize())
				nSizeToRead = InputBufferedFile::InternalGetBlockSize();
			else
				nSizeToRead = nYetToRead;

			// Lecture d'un bloc
			nLocalRead = (int)fileHandle->Read(sBuffer, InputBufferedFile::InternalGetElementSize(),
							   (size_t)nSizeToRead);
			nYetToRead -= nLocalRead;
			bOk = nLocalRead != -1;
			if (not bOk)
			{
				bIsError = true;
				AddError(fileHandle->GetLastErrorMessage());
			}
			else
				bEof = nLocalRead != nSizeToRead;
			assert(not bIsError or nLocalRead == nSizeToRead or bEof);

			// Recherche de eol dans le bloc qu'on vient d'ajouter, mais seulement dans la partie
			// supplementaire, apres nBufferSize - nBeginLinePos -1
			if (bOk)
			{
				// Soit on est avant nExtraPartPos => on ne cherche pas eol,
				// soit on est a cheval sur nExtraPartPos => on ne cherche qu'a partir de
				// nExtraPartPos), soit on a depasse nExtraPartPos => on chereche dans tout le bloc
				bHaveToSearchEol = true;
				nStartPos = -1;
				if (lCummulatedSentSize < nExtraPartPos)
				{
					if (nExtraPartPos < lCummulatedSentSize + nLocalRead)
					{
						// A cheval : on va chercher eol a partir de nExtraPartPos
						nStartPos = nExtraPartPos - (int)lCummulatedSentSize;
					}
					else
					{
						// Avant : on ne cherche pas eol
						bHaveToSearchEol = false;

						// Mais si fin de fichier, eol trouvee
						if (bEof)
						{
							bEolFound = true;
							bEof = true;
						}
					}
				}
				else
				{
					// On a depasse nExtraPartPos, on va chercher eol dans tout le bloc
					nStartPos = 0;
				}

				// Recherche de eol a partir de nStartPos
				if (bHaveToSearchEol)
				{
					assert(nStartPos != -1);
					for (nEolPos = nStartPos; nEolPos < nLocalRead; nEolPos++)
					{
						if (lCummulatedSentSize + nEolPos == nBufferSizeLimit)
							break;

						if (sBuffer[nEolPos] == '\n')
						{
							bEolFound = true;
							break;
						}
					}

					if (bEolFound)
						nSizeToSend = nEolPos + 1;
					else
						nSizeToSend = nLocalRead;
				}
				else
					nSizeToSend = nLocalRead;

				assert(nSizeToSend <= InputBufferedFile::InternalGetBlockSize());
				if (GetTracerMPI()->GetActiveMode())
					GetTracerMPI()->AddSend(nRank, FILE_SERVER_REQUEST_FILL);

				MPI_Send(sBuffer, nSizeToSend, MPI_CHAR, nRank, FILE_SERVER_REQUEST_FILL,
					 MPI_COMM_WORLD);
				nCurrentBufferSize += nSizeToSend;
				lCummulatedSentSize += nSizeToSend;
				if (bVerbose and bEolFound)
					cout << "[" << GetProcessId() << "] "
					     << "Find eol at " << LongintToReadableString(nEolPos) << " => "
					     << LongintToReadableString(nCurrentBufferSize + lBeginPos + nBeginLinePos)
					     << endl;
			}
		}

		if (not bEof and lBeginPos + lCummulatedSentSize == lFileSize)
			bEof = true;

		if (bEof)
		{
			bEolFound = true;
		}

		if (bOk and not bEolFound and not bEof)
		{
			// Si on n'a pas trouve de eol et qu'on a trouve un debut de ligne
			// On indique qu'on saute cette ligne et on remplit le buffer au minimum
			if (bBolFound)
			{
				bIsSkippedLine = true;
			}
			else
			{
				// On n'a trouve ni debut ni fin, on ne garde rien
				nCurrentBufferSize = 0;
			}
		}
	}

	if (bIsOpen)
		fileHandle->CloseInputFile(sFileName);

	if (fileHandle != NULL)
		delete fileHandle;

	// Envoi d'un message de taille nulle si le dernier message est de la taille d'un bloc (eof ou eol sur fin de
	// bloc) ou si il y a eu une erreur (il faut envoyer un dernier message de taille < MemSegmentByteSize) ou si
	// rien n'a ete envoye (debut ou fin de ligne non trouve)
	if (not bOk or lCummulatedSentSize == 0 or nSizeToSend == InputBufferedFile::InternalGetBlockSize())
	{
		MPI_Send(NULL, 0, MPI_CHAR, nRank, FILE_SERVER_REQUEST_FILL, MPI_COMM_WORLD);
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddSend(nRank, FILE_SERVER_REQUEST_FILL);
	}

	// En cas d'erreur le buffer est vide
	if (not bOk or not bBolFound)
		nCurrentBufferSize = 0;

	// Envoi du dernier message qui contient toutes les informations contextuelles
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddSend(nRank, FILE_SERVER_REQUEST_FILL_INFO);
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_FILL_INFO);
	serializer.OpenForWrite(&context);

	// Serialisation des messages (warning) a envoyer vers l'utilisateur
	SerializeErrors(&serializer);

	// Serialisation du tag d'erreur
	serializer.PutBoolean(bIsError);

	// Serialisation des attributs...
	if (not bIsError)
	{
		serializer.PutBoolean(bBolFound);
		serializer.PutBoolean(bEolFound); // Si not bEolFound le driver devra rechercher la premiere fin de
						  // ligne dans les donnees recues
		serializer.PutBoolean(bIsSkippedLine);
		serializer.PutBoolean(bEof);
		serializer.PutLongint(lBufferBeginPos);
		serializer.PutInt(nCurrentBufferSize);
	}
	serializer.Close();

	DeleteCharArray(sBuffer);
}

void PLMPIFileServerSlave::Read(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	int nBufferSize;
	ALString sTimestampId;
	longint lBeginPos;
	longint lFileSize;
	boolean bVerbose = false;
	boolean bOk = false;
	SystemFile* fileHandle;
	longint lBufferBeginPos;
	int nBeginLinePos;
	int nBufferSizeLimit;
	longint lCummulatedSentSize;
	int nYetToRead;
	int nSizeToRead;
	int nLocalRead;
	char* sBuffer;
	boolean bIsError;
	int nCurrentBufferSize;
	boolean bEof;
	int nSizeToSend;
	boolean bIsOpen;
	ALString sTmp;

	sBuffer = NewCharArray(InputBufferedFile::InternalGetBlockSize());
	bIsError = false;
	bEof = false;
	nCurrentBufferSize = 0;
	lCummulatedSentSize = 0;
	nBeginLinePos = 0;
	lBufferBeginPos = 0;
	nBufferSizeLimit = 0;
	nSizeToSend = 0;
	fileHandle = NULL;

	GetTracerMPI()->SetActiveMode(false);

	// Reception de la demande
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddRecv(nRank, FILE_SERVER_REQUEST_READ);
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_READ);
	serializer.OpenForRead(&context);
	sTimestampId = serializer.GetString();
	sFileName = serializer.GetString();
	lBeginPos = serializer.GetLongint();
	nBufferSize = serializer.GetInt();
	lFileSize = serializer.GetLongint();
	serializer.Close();

	if (bVerbose)
		cout << endl
		     << "[" << GetProcessId() << "] "
		     << "Read at pos " << LongintToReadableString(lBeginPos) << " chunk size "
		     << LongintToReadableString(nBufferSize) << " file size " << LongintToReadableString(lFileSize)
		     << endl;

	bIsOpen = false;
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenInputFile(sFileName);
	if (bOk)
	{
		bIsOpen = true;
		fileHandle->SeekPositionInFile(lBeginPos);
	}
	nYetToRead = nBufferSize;

	if (not bOk)
	{
		MPI_Send(NULL, 0, MPI_CHAR, nRank, FILE_SERVER_REQUEST_READ, MPI_COMM_WORLD);
	}

	while (nYetToRead > 0 and not bEof and bOk)
	{
		if (nYetToRead > InputBufferedFile::InternalGetBlockSize())
			nSizeToRead = InputBufferedFile::InternalGetBlockSize();
		else
			nSizeToRead = nYetToRead;

		// Lecture d'un bloc
		nLocalRead =
		    (int)fileHandle->Read(sBuffer, InputBufferedFile::InternalGetElementSize(), (size_t)nSizeToRead);
		nYetToRead -= nLocalRead;
		bOk = nLocalRead != -1;
		if (not bOk)
		{
			bIsError = true;
			AddError(fileHandle->GetLastErrorMessage());
		}
		else
			bEof = nLocalRead != nSizeToRead;
		assert(not bIsError or nLocalRead == nSizeToRead or bEof);

		MPI_Send(sBuffer, nLocalRead, MPI_CHAR, nRank, FILE_SERVER_REQUEST_READ, MPI_COMM_WORLD);
	}

	if (bIsOpen)
		fileHandle->CloseInputFile(sFileName);

	if (fileHandle != NULL)
		delete fileHandle;

	if (not bOk)
	{
		// Envoi des erreurs
		if (GetTracerMPI()->GetActiveMode())
			GetTracerMPI()->AddSend(nRank, FILE_SERVER_REQUEST_READ_ERROR);
		context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_READ_ERROR);
		serializer.OpenForWrite(&context);
		// Serialisation des messages (warning) a envoyer vers l'utilisateur
		SerializeErrors(&serializer);
		serializer.Close();
	}
	DeleteCharArray(sBuffer);
}

void PLMPIFileServerSlave::FillHeader(int nRank) const
{
	PLMPIMsgContext context;
	PLSerializer serializer;
	ALString sFileName;
	int nBufferSize;
	CharVector cvHeaderLine;
	InputBufferedFile bufferedFile;
	boolean bLineTooLong = false;
	boolean bOk;

	// Reception de la demande
	if (GetTracerMPI()->GetActiveMode())
		GetTracerMPI()->AddRecv(nRank, FILE_SERVER_REQUEST_HEADER);
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_HEADER);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	nBufferSize = serializer.GetInt();
	serializer.Close();

	// Ouverture du buffer local de lecture
	bufferedFile.SetBufferSize(nBufferSize);
	bufferedFile.SetFileName(sFileName);
	bOk = bufferedFile.Open();
	if (bOk)
	{
		// Remplissage de la premier eligne en local
		bLineTooLong = not bufferedFile.FillWithHeaderLine();
		bOk = not bufferedFile.IsError();
	}

	// Envoi de la ligne si  il n'y a pas eu d'erreur et des messages utilisateurs
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_HEADER);
	serializer.OpenForWrite(&context);

	// Serialisation des messages (warning) a envoyer vers l'utilisateur
	SerializeErrors(&serializer);
	serializer.PutBoolean(bOk);
	serializer.PutBoolean(bLineTooLong);

	// Serialisation du buffer
	if (bOk and not bLineTooLong)
		serializer.PutCharVector(bufferedFile.GetBuffer());
	serializer.Close();

	if (bufferedFile.IsOpened())
		bufferedFile.Close();
}

void PLMPIFileServerSlave::FindEOL(int nRank) const
{
	ALString sFileName;
	longint lBeginPos;
	longint lPos;
	boolean bIsError;
	boolean bEolFound;
	InputBufferedFile bufferedFile;
	PLSerializer serializer;
	PLMPIMsgContext context;

	lPos = 0;
	bEolFound = false;

	// Reception de la demande de buffer
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_EOL);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	lBeginPos = serializer.GetLongint();
	serializer.Close();

	// Initialisation de l'InputBufferedFile local
	bufferedFile.SetFileName(sFileName);
	AddPerformanceTrace("<< Start IO read : FindEOL");

	// Ouverture du fichier en lecture et remplissage du buffer
	bIsError = not bufferedFile.Open();
	if (not bIsError)
	{
		lPos = bufferedFile.FindEolPosition(lBeginPos, bEolFound);
		bIsError = bufferedFile.IsError();
		bufferedFile.Close();
	}
	AddPerformanceTrace("<< End IO read : FindEOL");

	AddPerformanceTrace("<< Start SEND : FindEOL");

	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_EOL);
	serializer.OpenForWrite(&context);

	// Serialisation des messages (warning) a envoyer vers l'utilisateur
	SerializeErrors(&serializer);

	// Serialisation du tag d'erreur
	serializer.PutBoolean(bIsError);

	// Serialisation des attributs...
	if (not bIsError)
	{
		serializer.PutLongint(lPos);
		serializer.PutBoolean(bEolFound);
	}
	serializer.Close();
	AddPerformanceTrace("<< End SEND EOL : FindEOL");
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
	context.Recv(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_EXIST);
	serializer.OpenForRead(&context);
	sFileName = serializer.GetString();
	serializer.Close();

	// Envoi de l'existence du fichier
	AddPerformanceTrace("<< Start IO read : FileExist");
	bFileExist = FileService::Exist(sFileName);
	AddPerformanceTrace("<< End IO read : FileExist");

	AddPerformanceTrace("<< Start SEND : FileExist");
	context.Send(MPI_COMM_WORLD, nRank, FILE_SERVER_REQUEST_EXIST);
	serializer.OpenForWrite(&context);
	serializer.PutBoolean(bFileExist);
	serializer.Close();
	AddPerformanceTrace("<< End SEND : FileExist");
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