// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "FileService.h"

const ALString FileService::sRemoteScheme = "file";
boolean FileService::bIOStats = false;

////////////////////////////////////////////
// Implementation de la classe FileService

boolean FileService::FileExists(const ALString& sPathName)
{
	boolean bIsFile = false;

	p_SetMachineLocale();
#ifdef _WIN32
	struct __stat64 fileStat;
	if (_stat64(sPathName, &fileStat) == 0)
		bIsFile = ((fileStat.st_mode & S_IFMT) == S_IFREG);
#else
	struct stat s;
	if (stat(sPathName, &s) == 0)
		bIsFile = ((s.st_mode & S_IFMT) == S_IFREG);
#endif // _WIN32
	p_SetApplicationLocale();

	return bIsFile;
}

boolean FileService::DirExists(const ALString& sPathName)
{
	boolean bIsDirectory = false;

	p_SetMachineLocale();
#ifdef _WIN32
	boolean bExist;

	bExist = _access(sPathName, 0) != -1;
	if (bExist)
	{
		// On test si ca n'est pas un fichier, car sous Windows, la racine ("C:") existe mais n'est
		// consideree par l'API _stat64 ni comme une fichier ni comme un repertoire
		boolean bIsFile = false;
		struct __stat64 fileStat;
		if (_stat64(sPathName, &fileStat) == 0)
			bIsFile = ((fileStat.st_mode & S_IFMT) == S_IFREG);
		bIsDirectory = not bIsFile;
	}
#else // _WIN32

	struct stat s;
	if (stat(sPathName, &s) == 0)
		bIsDirectory = ((s.st_mode & S_IFMT) == S_IFDIR);

#endif // _WIN32
	p_SetApplicationLocale();

	return bIsDirectory;
}

boolean FileService::SetFileMode(const ALString& sFilePathName, boolean bReadOnly)
{
	boolean bOk;

	p_SetMachineLocale();

#ifdef _WIN32
	if (bReadOnly)
		bOk = _chmod(sFilePathName, _S_IREAD) == 0;
	else
		bOk = _chmod(sFilePathName, _S_IREAD | _S_IWRITE) == 0;
#else
	if (bReadOnly)
		// Lecture par owner  S_IRUSR, groupe S_IRGRP
		bOk = chmod(sFilePathName, S_IRUSR | S_IRGRP) == 0;
	else
		// idem + ecriture owner S_IWUSR
		bOk = chmod(sFilePathName, S_IRUSR | S_IRGRP | S_IWUSR) == 0;
		// Pour Visual C++
#endif
	p_SetApplicationLocale();
	return bOk;
}

longint FileService::GetFileSize(const ALString& sFilePathName)
{
	longint lFileSize;
	int nError;

	p_SetMachineLocale();

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#ifdef _WIN32
	struct __stat64 fileStat;
	nError = _stat64(sFilePathName, &fileStat);
#elif defined(__APPLE__)
	struct stat fileStat;
	nError = stat(sFilePathName, &fileStat);
#else
	struct stat64 fileStat;
	nError = stat64(sFilePathName, &fileStat);
#endif

	p_SetApplicationLocale();
	if (nError != 0)
		lFileSize = 0;
	else
		lFileSize = fileStat.st_size;
	return lFileSize;
}

boolean FileService::CreateEmptyFile(const ALString& sFilePathName)
{
	FILE* fFile;

	// La fonction p_fopen gere deja les locale correctement
	fFile = p_fopen(sFilePathName, "wb");
	if (fFile != NULL)
		fclose(fFile);
	return (fFile != NULL);
}

boolean FileService::RemoveFile(const ALString& sFilePathName)
{
	boolean bOk;
	p_SetMachineLocale();
	bOk = remove(sFilePathName) == 0;
	p_SetApplicationLocale();
	return bOk;
}

boolean FileService::OpenInputFile(const ALString& sFilePathName, fstream& fst)
{
	boolean bOk;

	p_SetMachineLocale();
	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		fst.open(sFilePathName, ios::in);
		bOk = fst.is_open();
		if (not bOk)
			Global::AddError("File", sFilePathName, "Unable to open file " + GetLastSystemIOErrorMessage());
	}
	p_SetApplicationLocale();
	return bOk;
}

boolean FileService::OpenOutputFile(const ALString& sFilePathName, fstream& fst)
{
	boolean bOk;

	p_SetMachineLocale();
	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		fst.open(sFilePathName, ios::out);
		bOk = fst.is_open();
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 "Unable to open output file " + GetLastSystemIOErrorMessage());
	}
	p_SetApplicationLocale();
	return bOk;
}

boolean FileService::OpenOutputFileForAppend(const ALString& sFilePathName, fstream& fst)
{
	boolean bOk;

	p_SetMachineLocale();
	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file for append (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		fst.open(sFilePathName, ios::out | ios::app);
		bOk = fst.is_open();
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 "Unable to open output file for append " + GetLastSystemIOErrorMessage());
	}
	p_SetApplicationLocale();
	return bOk;
}

boolean FileService::CloseInputFile(const ALString& sFilePathName, fstream& fst)
{
	boolean bOk = true;

	require(fst.is_open());

	// Pas d'erreur en cas de fichier en lecture
	fst.close();
	return bOk;
}

boolean FileService::CloseOutputFile(const ALString& sFilePathName, fstream& fst)
{
	boolean bOk = true;

	require(fst.is_open());

	// Flush du contenu du rapport juste avant la fermeture pour detecter une erreur en ecriture
	fst.flush();
	if (fst.fail())
	{
		Global::AddError("File", sFilePathName,
				 "Physical error when writing data to file " + GetLastSystemIOErrorMessage());
		bOk = false;
	}
	fst.close();
	return bOk;
}

boolean FileService::OpenInputBinaryFile(const ALString& sFilePathName, FILE*& fFile)
{
	boolean bOk;

	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		// La fonction p_fopen gere deja les locale correctement
		fFile = p_fopen(sFilePathName, "rb");
		bOk = (fFile != NULL);
		if (not bOk)
			Global::AddError("File", sFilePathName, "Unable to open file " + GetLastSystemIOErrorMessage());
	}
	return bOk;
}

boolean FileService::SeekPositionInBinaryFile(FILE* fFile, longint lStartPosition)
{
	boolean bOk;

	require(lStartPosition >= 0);
	bOk = SystemSeekPositionInBinaryFile(fFile, lStartPosition, SEEK_SET);
	return bOk;
}

boolean FileService::OpenOutputBinaryFile(const ALString& sFilePathName, FILE*& fFile)
{
	boolean bOk;

	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		// La fonction p_fopen gere deja les locale correctement
		fFile = p_fopen(sFilePathName, "wb");
		bOk = (fFile != NULL);
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 "Unable to open output file " + GetLastSystemIOErrorMessage());
	}
	return bOk;
}

boolean FileService::OpenOutputBinaryFileForAppend(const ALString& sFilePathName, FILE*& fFile)
{
	boolean bOk;

	// Test si nom de fichier renseigne
	bOk = (sFilePathName != "");
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file for append (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		// La fonction p_fopen gere deja les locale correctement
		// On ouvre le fichier en mode "r+b" (et non "ab") pour pouvoir utiliser des Seek par la suite,
		// et exploiter la methode ReserveExtraSize sans interaction avec le mode append
		fFile = p_fopen(sFilePathName, "r+b");
		bOk = (fFile != NULL);
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 "Unable to open output file for append " + GetLastSystemIOErrorMessage());

		// Pour se mettre en append, on se deplace vers la fin de fichier
		if (bOk)
		{
			bOk = SystemSeekPositionInBinaryFile(fFile, 0, SEEK_END);

			// Fermeture du fichier si erreur
			if (not bOk)
			{
				Global::AddError("File", sFilePathName,
						 "Unable to call lseek to end of file " +
						     GetLastSystemIOErrorMessage());

				// Fermeture directe du fichier, pour eviter un message d'erreur additionnel
				fclose(fFile);
			}
		}
	}
	return bOk;
}

// Fonction temporaire pour evaluer l'impact du mode avec ou sans reservation de taille
boolean GetReserveExtraSizeExpertMode()
{
	static boolean bIsInitialized = false;
	static boolean bReserveExtraSizeExpertMode = true;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sReserveExtraSizeExpertMode;

		// Recherche des variables d'environnement
		sReserveExtraSizeExpertMode = p_getenv("KhiopsReserveExtraSizeExpertMode");
		sReserveExtraSizeExpertMode.MakeLower();

		// Determination du mode expert
		if (sReserveExtraSizeExpertMode == "true")
			bReserveExtraSizeExpertMode = true;
		else if (sReserveExtraSizeExpertMode == "false")
			bReserveExtraSizeExpertMode = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bReserveExtraSizeExpertMode;
}

boolean FileService::ReserveExtraSize(FILE* fFile, longint lSize)
{
	boolean bOk = true;
	longint lRead;

	require(fFile != NULL);
	require(lSize >= 0);

	// Reference: https://stackoverflow.com/questions/7775027/how-to-create-file-of-x-size
	//            https://www.linuxquestions.org/questions/programming-9/how-to-create-a-file-of-pre-defined-size-in-c-789667
	// Le principe est ici de se deplacer un octet avant la fin de fichier, d'ecrire un octet,
	// puis de se replacer a l'endroit d'insertion courant pour les ecriture
	// Attention, la place reserve doit absolument etre ecrite pour produire un fichier valide

	// Pour evaluer l'impact de cette methode sur les performances depuis un variable d'enironnement
	if (not GetReserveExtraSizeExpertMode())
		return true;

	// Uniquement si taille non nulle
	if (lSize > 0)
	{
		// Positionnement un octet avant la fin de fichier
		bOk = SystemSeekPositionInBinaryFile(fFile, lSize - 1, SEEK_CUR);

		// Ecriture d'un octet en fin de fichier, pour en reserver la taille
		if (bOk)
		{
			lRead = fwrite("", 1, sizeof(char), fFile);
			bOk = lRead == 1;
			if (not bOk)
				Global::AddError("File", "",
						 "Unable to write a byte at the end of file " +
						     GetLastSystemIOErrorMessage());
		}
		// On se redeplace a la position courante initiale
		if (bOk)
		{
			bOk = SystemSeekPositionInBinaryFile(fFile, -lSize, SEEK_CUR);
		}
	}
	return bOk;
}

const ALString& FileService::GetEOL()
{
#ifdef _WIN32
	static const ALString sEOL = "\r\n";
#else
	static const ALString sEOL = "\n";
#endif
	return sEOL;
}

boolean FileService::CloseInputBinaryFile(const ALString& sFilePathName, FILE*& fFile)
{
	boolean bOk = true;
	int nError;

	require(fFile != NULL);

	nError = fclose(fFile);
	if (nError != 0)
	{
		Global::AddError("File", sFilePathName,
				 " Physical error when closing input file " + GetLastSystemIOErrorMessage());
		bOk = false;
	}
	fFile = NULL;
	return bOk;
}

boolean FileService::CloseOutputBinaryFile(const ALString& sFilePathName, FILE*& fFile)
{
	boolean bOk = true;
	int nErrorClose;

	require(fFile != NULL);

	// Flush du contenu du rapport juste avant la fermeture pour detecter une erreur en ecriture
	fflush(fFile);
	if (ferror(fFile) != 0)
	{
		Global::AddError("File", sFilePathName,
				 "Physical error when writing data to file " + GetLastSystemIOErrorMessage());
		bOk = false;
	}

	// Fermeture effective du fichier, avec message si necessaire et s'il n'y en a pas deja eu
	nErrorClose = fclose(fFile);
	if (nErrorClose != 0 and bOk)
	{
		Global::AddError("File", sFilePathName,
				 " Physical error when closing output file " + GetLastSystemIOErrorMessage());
		bOk = false;
	}
	fFile = NULL;
	return bOk;
}

const ALString FileService::GetLastSystemIOErrorMessage()
{
	ALString sMessage;

	if (errno != 0)
	{
		sMessage = "(";
		sMessage += strerror(errno);
		sMessage += ")";
	}
	return sMessage;
}

boolean FileService::GetDirectoryContentExtended(const ALString& sPathName, StringVector* svDirectoryNames,
						 StringVector* svFileNames)
{
#ifdef _WIN32
	return GetDirectoryContent(sPathName, svDirectoryNames, svFileNames);
#else
	boolean bOk = true;
	DIR* dir;
	struct dirent* ent;
	int nRet;

	require(svDirectoryNames != NULL);
	require(svDirectoryNames->GetSize() == 0);
	require(svFileNames != NULL);
	require(svFileNames->GetSize() == 0);

	p_SetMachineLocale();
	if ((dir = opendir(sPathName)) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL)
		{
			// On traite tous les fichiers, sauf les fichiers predefinis "." et ".."
			if (strcmp(ent->d_name, ".") != 0 and strcmp(ent->d_name, "..") != 0)
			{
				if (ent->d_type == DT_DIR)
					svDirectoryNames->Add(ent->d_name);
				if (ent->d_type == DT_REG)
					svFileNames->Add(ent->d_name);
				if (ent->d_type == DT_LNK)
				{
					struct stat sb;

					nRet = stat(BuildFilePathName(sPathName, ent->d_name), &sb);
					if (nRet != -1)
					{
						switch (sb.st_mode & S_IFMT)
						{
						case S_IFDIR:
							svDirectoryNames->Add(ent->d_name);
							break;
						case S_IFREG:
							svFileNames->Add(ent->d_name);
							break;
						default:
							break;
						}
					}
				}
			}
		}
		closedir(dir);
	}
	else
		bOk = false;
	p_SetApplicationLocale();
	return bOk;
#endif
}

boolean FileService::GetDirectoryContent(const ALString& sPathName, StringVector* svDirectoryNames,
					 StringVector* svFileNames)
{
#ifdef _WIN32
	boolean bOk = true;
	void* hFind = NULL;
	void* pFileData;
	const char* sFileName;

	require(svDirectoryNames != NULL);
	require(svDirectoryNames->GetSize() == 0);
	require(svFileNames != NULL);
	require(svFileNames->GetSize() == 0);

	p_SetMachineLocale();
	// Recherche la premiere entite du repertoire
	pFileData = p_NewFileData();
	hFind = p_FindFirstFile(sPathName, pFileData);
	if (hFind == NULL)
		bOk = false;
	// Parcours du repertoire
	else
	{
		do
		{
			// On traite tous les fichiers, sauf les fichiers predefinis "." et ".."
			sFileName = p_GetFileName(pFileData);
			if (strcmp(sFileName, ".") != 0 && strcmp(sFileName, "..") != 0)
			{
				// Memorisation parmi les repertoires ou,les fichiers
				if (p_IsDirectory(pFileData))
					svDirectoryNames->Add(sFileName);
				else
					svFileNames->Add(sFileName);
			}
		} while (p_FindNextFile(hFind, pFileData)); // Fichier suivants
	}

	// Nettoyage
	p_FindClose(hFind);
	p_DeleteFileData(pFileData);
	p_SetApplicationLocale();
	return bOk;
#else
	boolean bOk = true;
	DIR* dir;
	struct dirent* ent;

	require(svDirectoryNames != NULL);
	require(svDirectoryNames->GetSize() == 0);
	require(svFileNames != NULL);
	require(svFileNames->GetSize() == 0);

	p_SetMachineLocale();
	if ((dir = opendir(sPathName)) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL)
		{
			// On traite tous les fichiers, sauf les fichiers predefinis "." et ".."
			if (strcmp(ent->d_name, ".") != 0 and strcmp(ent->d_name, "..") != 0)
			{
				if (ent->d_type == DT_DIR)
					svDirectoryNames->Add(ent->d_name);
				if (ent->d_type == DT_REG)
					svFileNames->Add(ent->d_name);
			}
		}
		closedir(dir);
	}
	else
		bOk = false;
	p_SetApplicationLocale();
	return bOk;
#endif
}

boolean FileService::MakeDirectory(const ALString& sPathName)
{
#ifdef _WIN32
	int nError;
	p_SetMachineLocale();
	nError = _mkdir(sPathName);
	p_SetApplicationLocale();
	return nError == 0;
#else
	int nError;
	p_SetMachineLocale();
	nError = mkdir(sPathName, S_IRWXU);
	p_SetApplicationLocale();
	return nError == 0;
#endif
}

boolean FileService::MakeDirectories(const ALString& sPathName)
{
	boolean bOk;
	int nEnd;
	ALString sDirectory;

	// Parcours des repertoire intermediaires
	nEnd = 0;
	while (nEnd < sPathName.GetLength())
	{
		// Extraction du directory intermediaire suivant
		while (nEnd < sPathName.GetLength() and not IsFileSeparator(sPathName.GetAt(nEnd)))
			nEnd++;
		sDirectory = sPathName.Left(nEnd);
		nEnd++;

		// Creation du directory
		if (sDirectory != "" and not DirExists(sDirectory))
		{
			bOk = MakeDirectory(sDirectory);
			if (not bOk)
				return false;
		}
	}
	return DirExists(sPathName);
}

boolean FileService::RemoveDirectory(const ALString& sPathName)
{
#ifdef _WIN32
	int nError;
	p_SetMachineLocale();
	nError = _rmdir(sPathName);
	p_SetApplicationLocale();
	return nError == 0;
#else
	int nError;
	p_SetMachineLocale();
	nError = rmdir(sPathName);
	p_SetApplicationLocale();
	return nError == 0;
#endif
}

////////////////////////////////////////////////////////////////////////////

char FileService::GetFileSeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

boolean FileService::IsFileSeparator(char c)
{
#ifdef _WIN32
	return c == GetFileSeparator() or c == '/';
#else
	return c == GetFileSeparator();
#endif
}

boolean FileService::IsPathInFilePath(const ALString& sFilePathName)
{
	int nPos;
	int nSeparatorPos;

	// Recherche du dernier caractere separateur de chemin
	nSeparatorPos = -1;
	for (nPos = sFilePathName.GetLength() - 1; nPos >= 0; nPos--)
	{
		if (IsFileSeparator(sFilePathName[nPos]))
		{
			nSeparatorPos = nPos;
			break;
		}
	}
	return nSeparatorPos != -1;
}

const ALString FileService::GetPathName(const ALString& sFilePathName)
{
	ALString sPathName;
	int nPos;
	int nSeparatorPos;
	ALString sScheme;
	char cFileSeparator;

	// Attention au cas specifique des URI
	sScheme = GetURIScheme(sFilePathName);
	cFileSeparator = GetFileSeparator();
	if (sScheme != "")
		cFileSeparator = GetURIFileSeparator();

	// Recherche du dernier caractere separateur de chemin
	nSeparatorPos = -1;
	for (nPos = sFilePathName.GetLength() - 1; nPos >= 0; nPos--)
	{
		if (IsFileSeparator(sFilePathName[nPos]))
		{
			nSeparatorPos = nPos;

			// On supprime les eventuels separators immediatement avant
			while (nSeparatorPos > 0 and IsFileSeparator(sFilePathName[nSeparatorPos - 1]))
				nSeparatorPos--;
			break;
		}
	}

	// On retourne la partie chemin suivi du separateur (sinon, vide)
	if (nSeparatorPos >= 0)
		sPathName = sFilePathName.Left(nSeparatorPos) + cFileSeparator;

	// Dans le cas d'une URI, on ajoute 2 '/' pour en avoir 3
	if (sScheme != "" and sPathName.GetLength() < sScheme.GetLength() + 3)
		sPathName = sPathName + cFileSeparator + cFileSeparator;
	return sPathName;
}

const ALString FileService::SetPathName(const ALString& sFilePathName, const ALString& sPathName)
{
	return BuildFilePathName(sPathName, GetFileName(sFilePathName));
}

const ALString FileService::GetFileName(const ALString& sFilePathName)
{
	ALString sFileName;
	int nPos;
	int nSeparatorPos;

	// Recherche du dernier caractere separateur de chemin
	nSeparatorPos = -1;
	for (nPos = sFilePathName.GetLength() - 1; nPos >= 0; nPos--)
	{
		if (IsFileSeparator(sFilePathName[nPos]))
		{
			nSeparatorPos = nPos;
			break;
		}
	}

	// On retourne la partie nom de fichier
	if (nSeparatorPos >= 0)
		sFileName = sFilePathName.Right(sFilePathName.GetLength() - nSeparatorPos - 1);
	else
		sFileName = sFilePathName;
	return sFileName;
}

const ALString FileService::SetFileName(const ALString& sFilePathName, const ALString& sFileName)
{
	return BuildFilePathName(GetPathName(sFilePathName), sFileName);
}

const ALString FileService::GetFilePrefix(const ALString& sFilePathName)
{
	ALString sFilePrefix;
	ALString sFileName;
	int nSeparatorPos;

	// Recherche du nom de fichier
	sFileName = GetFileName(sFilePathName);

	// Recherche du dernier caractere separateur de suffixe
	nSeparatorPos = sFileName.ReverseFind('.');

	// On retourne la partie prefixe
	if (nSeparatorPos >= 0)
		sFilePrefix = sFileName.Left(nSeparatorPos);
	else
		sFilePrefix = sFileName;
	return sFilePrefix;
}

const ALString FileService::SetFilePrefix(const ALString& sFilePathName, const ALString& sFilePrefix)
{
	return BuildFilePathName(GetPathName(sFilePathName), BuildFileName(sFilePrefix, GetFileSuffix(sFilePathName)));
}

const ALString FileService::GetFileSuffix(const ALString& sFilePathName)
{
	ALString sFileSuffix;
	ALString sFileName;
	int nSeparatorPos;

	// Recherche du nom de fichier
	sFileName = GetFileName(sFilePathName);

	// Recherche du dernier caractere separateur de suffixe
	nSeparatorPos = sFileName.ReverseFind('.');

	// On retourne la partie suffixe
	if (nSeparatorPos >= 0)
		sFileSuffix = sFileName.Right(sFileName.GetLength() - nSeparatorPos - 1);
	else
		sFileSuffix = "";
	return sFileSuffix;
}

const ALString FileService::SetFileSuffix(const ALString& sFilePathName, const ALString& sFileSuffix)
{
	return BuildFilePathName(GetPathName(sFilePathName), BuildFileName(GetFilePrefix(sFilePathName), sFileSuffix));
}

const ALString FileService::BuildFileName(const ALString& sFilePrefix, const ALString& sFileSuffix)
{
	if (sFilePrefix == "")
	{
		if (sFileSuffix == "")
			return "";
		else
			return '.' + sFileSuffix;
	}
	else if (sFileSuffix == "")
		return sFilePrefix;
	else
		return sFilePrefix + '.' + sFileSuffix;
}

const ALString FileService::BuildFilePathName(const ALString& sPathName, const ALString& sFileName)
{
	ALString sNormalizedPathName;
	int nSeparatorPos;
	int nPos;
	ALString sScheme;
	char cFileSeparator;
	ALString sPathOfFileName;
	boolean bIsStartingCurrentPath;

	require(not IsAbsoluteFilePathName(sFileName));

	if (sPathName == "")
		return sFileName;
	else if (sFileName == "")
		return sPathName;
	else
	{
		// Normalisation de la forme du path name avec separateur potentiel en fin
		sNormalizedPathName = GetPathName(sPathName + GetFileSeparator());

		// Attention au cas specifique des URI
		sScheme = GetURIScheme(sNormalizedPathName);
		cFileSeparator = GetFileSeparator();
		if (sScheme != "")
			cFileSeparator = GetURIFileSeparator();

		// Tolerance pour les chemin se terminant par un separateur
		assert(sNormalizedPathName.GetLength() > 0);
		nSeparatorPos = -1;
		nPos = sNormalizedPathName.GetLength() - 1;
		if (IsFileSeparator(sNormalizedPathName.GetAt(nPos)))
		{
			nSeparatorPos = nPos;

			// On supprime les eventuels separateurs immediatement avant
			if (sScheme == "")
			{
				// cas d'un fichier standard
				while (nSeparatorPos > 0 and IsFileSeparator(sNormalizedPathName[nSeparatorPos - 1]))
					nSeparatorPos--;
			}
			else
			{
				// Cas d'une URI ou on ne supprime pas les separateurs avant scheme:///
				while (nSeparatorPos > sScheme.GetLength() + 3 and
				       IsFileSeparator(sNormalizedPathName[nSeparatorPos - 1]))
					nSeparatorPos--;
			}

			// On supprime les doublons de separateur
			sNormalizedPathName = sNormalizedPathName.Left(nSeparatorPos);
		}

		// caracteristique du path du fichier
		sPathOfFileName = GetPathName(sFileName);
		bIsStartingCurrentPath =
		    sFileName == "." or
		    (sFileName.GetLength() >= 2 and sFileName.GetAt(0) == '.' and IsFileSeparator(sFileName.GetAt(1)));

		// Construction du chemin complet, en traitant le cas particulier du repertoire courant suivi
		// d'un fichier lui meme avec un path de type courant
		if (sNormalizedPathName == "." and sPathOfFileName != "" and sPathOfFileName.GetAt(0) == '.')
			return sFileName;
		else if (sNormalizedPathName.GetLength() > 1 and bIsStartingCurrentPath)
			return sNormalizedPathName + sFileName.Right(sFileName.GetLength() - 1);
		else
			return sNormalizedPathName + cFileSeparator + sFileName;
	}
}

boolean FileService::IsAbsoluteFilePathName(const ALString& sURIFilePathName)
{
	boolean bIsAbsolutePath;
	ALString sFilePathName;

	sFilePathName = FileService::GetURIFilePathName(sURIFilePathName);

	// Reference: wikipedia absolute path ou wikipedia root path
	// Ici, on se contente d'une regle simplifiee
	//   - sous unix: nom du chemin commencant par '/'
	//   - sous windows: nom du chemin [drive letter:]\ ou \\[server name]\[volume]
#ifdef _WIN32
	// Test si on commence par "\\"
	bIsAbsolutePath =
	    sFilePathName.GetLength() > 1 and (sFilePathName.GetAt(0) == '\\' and sFilePathName.GetAt(1) == '\\');
	if (not bIsAbsolutePath)
	{
		int nColonsPos;

		// Test si on commence par "[drive letter:]\"
		nColonsPos = sFilePathName.Find(':');
		bIsAbsolutePath =
		    nColonsPos >= 0 and sFilePathName.GetLength() > nColonsPos + 1 and
		    (sFilePathName.GetAt(nColonsPos + 1) == '/' or sFilePathName.GetAt(nColonsPos + 1) == '\\');
	}
#else
	bIsAbsolutePath = sFilePathName.GetLength() > 0 and sFilePathName.GetAt(0) == '/';
#endif
	return bIsAbsolutePath;
}

////////////////////////////////////////////////////////////////////////////

char FileService::GetPathSeparator()
{
#ifdef _WIN32
	return ';';
#else
	return ':';
#endif // UNIX
}

const ALString FileService::GetPathDirectoryList()
{
	ALString sPathDirectoryList;

	// Recherche de la liste des directory du path dans la variable d'environnement
	sPathDirectoryList = p_getenv("PATH");

	// Ajout du directory courant
	sPathDirectoryList += GetPathSeparator();
	sPathDirectoryList += '.';
	return sPathDirectoryList;
}

const ALString FileService::GetFilePath(const ALString& sFileName, const ALString& sDirectoryList)
{
	char cPathSeparator = GetPathSeparator();
	ALString sFilePath;
	ALString sDirectory;
	ALString sRemainingDirectories;
	int nBegin;
	int nEnd;

	// Test d'existance dans le directory courant
	if (FileExists(sFileName))
		return sFileName;

	// Parcours des directories
	nBegin = 0;
	nEnd = 0;
	while (nEnd < sDirectoryList.GetLength())
	{
		// Extraction du directory suivant
		while (nEnd < sDirectoryList.GetLength() and sDirectoryList.GetAt(nEnd) != cPathSeparator)
			nEnd++;
		sDirectory = sDirectoryList.Mid(nBegin, nEnd - nBegin);
		nBegin = nEnd + 1;
		nEnd = nBegin;

		// Recherche si le fichier existe dans le directory
		sFilePath = BuildFilePathName(sDirectory, sFileName);
		if (FileExists(sFilePath))
		{
			return sFilePath;
		}
	}
	return "";
}

const ALString FileService::CreateNewFile(const ALString& sBaseFilePathName)
{
	ALString sPathName;
	ALString sFilePrefix;
	ALString sFileSuffix;
	boolean bNewFile;
	ALString sNewFileName;
	int nId;
	const int nMaxId = 10000;

	// Decomposition du nom du fichier
	sPathName = GetPathName(sBaseFilePathName);
	sFilePrefix = GetFilePrefix(sBaseFilePathName);
	sFileSuffix = GetFileSuffix(sBaseFilePathName);

	// Test d'existence avec le fichier de base
	sNewFileName = sBaseFilePathName;
	bNewFile = not FileExists(sNewFileName);

	// Tentative de creation si possible
	if (bNewFile)
		bNewFile = CreateEmptyFile(sNewFileName);

	// Boucle de recherche d'un nom de fichier inexistant
	nId = 0;
	while (not bNewFile)
	{
		nId++;

		// Arret si trop de tentatives, pour eviter une boucle infinie
		if (nId >= nMaxId)
		{
			sNewFileName = "";
			break;
		}

		// Construction d'un nouveau nom de fichier
		sNewFileName =
		    BuildFilePathName(sPathName, BuildFileName(sFilePrefix + "_" + IntToString(nId), sFileSuffix));

		// Test d'existence avec le nouveau nom
		bNewFile = not FileExists(sNewFileName);

		// Si nouveau nom de fichier, on tente de creer le fichier
		if (bNewFile)
			bNewFile = CreateEmptyFile(sNewFileName);
	}
	return sNewFileName;
}

const ALString FileService::CreateNewDirectory(const ALString& sBasePathName)
{
	ALString sPathName;
	ALString sDirectoryPrefix;
	ALString sDirectorySuffix;
	boolean bNewDirectory;
	ALString sNewDirectoryName;
	int nId;
	const int nMaxId = 10000;

	// Decomposition du nom du repertoire
	sPathName = GetPathName(sBasePathName);
	sDirectoryPrefix = GetFilePrefix(sBasePathName);
	sDirectorySuffix = GetFileSuffix(sBasePathName);

	// Test d'existence avec le repertoire de base
	sNewDirectoryName = sBasePathName;
	bNewDirectory = not DirExists(sNewDirectoryName);

	// Tentative de creation si possible
	if (bNewDirectory)
		bNewDirectory = MakeDirectory(sNewDirectoryName);

	// Boucle de recherche d'un nom de repertoire inexistant
	nId = 0;
	while (not bNewDirectory)
	{
		nId++;

		// Arret si trop de tentatives, pour eviter une boucle infinie
		if (nId >= nMaxId)
		{
			sNewDirectoryName = "";
			break;
		}

		// Construction d'un nouveau nom de repertoire
		sNewDirectoryName = BuildFilePathName(
		    sPathName, BuildFileName(sDirectoryPrefix + "_" + IntToString(nId), sDirectorySuffix));

		// Test d'existence avec le nouveau nom
		bNewDirectory = not DirExists(sNewDirectoryName);

		// Si nouveau nom de fichier, on tente de creer le repertoire
		if (bNewDirectory)
			bNewDirectory = MakeDirectory(sNewDirectoryName);
	}
	return sNewDirectoryName;
}

const ALString FileService::GetTmpDir()
{
	ALString sTmpDir;
	sTmpDir = sUserTmpDir;
	if (sTmpDir == "")
		sTmpDir = GetSystemTmpDir();
	return sTmpDir;
}

const ALString FileService::GetSystemTmpDir()
{
	ALString sTmpDir;

	// Recherche de directory temporaire dans TEMP
	sTmpDir = p_getenv("TEMP");

	// Test si directory inexistant
	if (sTmpDir != "" and not DirExists(sTmpDir))
		sTmpDir = "";

	// Recherche de directory temporaire dans TMP
	if (sTmpDir == "")
	{
		sTmpDir = p_getenv("TMP");

		// Test si directory inexistant
		if (sTmpDir != "" and not DirExists(sTmpDir))
			sTmpDir = "";
	}

	// Recherche de directory temporaire dans TMPDIR
	if (sTmpDir == "")
	{
		sTmpDir = p_getenv("TMPDIR");

		// Test si directory inexistant
		if (sTmpDir != "" and not DirExists(sTmpDir))
			sTmpDir = "";
	}

#ifdef _WIN32
	// Encore un essai avec "\temp"
	if (sTmpDir == "")
	{
		sTmpDir = "C:";
		sTmpDir += GetFileSeparator();
		sTmpDir += "temp";

		// Test si directory inexistant
		if (not DirExists(sTmpDir))
			sTmpDir = "";
	}
#else
	// Dans les systemes unix, le repertoire temporaire par defaut est "/tmp"
	if (sTmpDir == "")
	{
		sTmpDir = GetFileSeparator();
		sTmpDir += "tmp";

		// Test si directory inexistant
		if (not DirExists(sTmpDir))
			sTmpDir = "";
	}
#endif // _WIN32

// Sur apple, le chemin du repertoire temporaire se termine par un '/'
// ce qui n'est pas le cas sur windows et linux. Pour unifier, on supprime
// ce dernier caractere
#ifdef __APPLE__
	assert(sTmpDir.GetAt(sTmpDir.GetLength() - 1) == GetFileSeparator());
	return sTmpDir.Left(sTmpDir.GetLength() - 1);
#else
	return sTmpDir;
#endif
}

void FileService::SetUserTmpDir(const ALString& sPathName)
{
	if (sUserTmpDir != sPathName)
	{
		sUserTmpDir = sPathName;
		nApplicationTmpDirFreshness++;
	}
}

const ALString FileService::GetUserTmpDir()
{
	return sUserTmpDir;
}

void FileService::SetApplicationName(const ALString& sName)
{
	require(sName != "");
	require(GetFileName(sApplicationName) == sApplicationName);
	if (sApplicationName != sName)
	{
		sApplicationName = sName;
		nApplicationTmpDirFreshness++;
	}
}

const ALString FileService::GetApplicationName()
{
	return sApplicationName;
}

// Fonction de destruction du repertoire applicatif des fichiers temporaires
// Doit etre declaree avant l'appel du atexit
void FileServiceApplicationTmpDirAutomaticRemove()
{
	FileService::DeleteApplicationTmpDir();
}

boolean FileService::CreateApplicationTmpDir()
{
	boolean bOk = true;
	ALString sFullApplicationName;
	FILE* fApplicationTmpDirAnchorFile;
	ALString sSystemTmpDir;

	require(sApplicationName != "");
	require(GetFileName(sApplicationName) == sApplicationName);

	// Si deja cree, on sort
	if (nApplicationTmpDirCreationFreshness == nApplicationTmpDirFreshness and sApplicationTmpDir != "" and
	    FileService::DirExists(sApplicationTmpDir) and
	    FileService::FileExists(BuildFilePathName(sApplicationTmpDir, GetAnchorFileName())))
		return true;

	// Enregistrement de la fonction de destruction des fichiers temporaires
	// Ce n'est fait qu'une seule fois grace a la variable booleenne
	if (not bApplicationTmpDirAutomaticRemove)
	{
		atexit(FileServiceApplicationTmpDirAutomaticRemove);
		bApplicationTmpDirAutomaticRemove = true;
	}

	// Destruction prealable de l'ancien repertoire
	if (sApplicationTmpDir != "")
		DeleteApplicationTmpDir();

	// Nom d'application en mode maitre esclave, pour avoir un repertoire dedie pour le maitre et par esclave
	sFullApplicationName = sApplicationName;
	if (GetProcessId() > 0)
		sFullApplicationName += IntToString(GetProcessId());
	sApplicationTmpDir = BuildFilePathName(GetTmpDir(), GetTmpPrefix() + sFullApplicationName);

	// Nettoyage des repertoires inactifs precedents
	// On le fait a chaque fois, mais en fait cela ne devrait se passer que s'il y a un changement
	// dans le nom de repertoire temporaire ou dans le nom de l'application
	CleanExpiredApplicationTmpDirs(sApplicationTmpDir);

	// Creation du repertoire temporaire utilisateur si different de la valeur par defaut (vide)
	if (bOk and sUserTmpDir != "")
	{
		// On verifie d'abord que le chemin est absolu
		if (not IsAbsoluteFilePathName(sUserTmpDir))
		{
			bOk = false;
			Global::AddError("Temp file directory", sUserTmpDir,
					 "Temp file directory must be an absolute path");
		}
		// Tentative de creation si necessaire
		else if (not DirExists(sUserTmpDir))
		{
			bOk = MakeDirectories(sUserTmpDir);
			if (bOk)
				bOk = DirExists(sUserTmpDir);
			if (not bOk)
				Global::AddError("Temp file directory", sUserTmpDir,
						 "Unable to create temp file directory");
		}
	}

	// Verification du repertoire systeme utilisateur temporaire si pas de specification de repertoire temporaire
	// utilisateur
	if (bOk and sUserTmpDir == "")
	{
		sSystemTmpDir = GetSystemTmpDir();

		// On verifie d'abord que le chemin est specifie
		if (sSystemTmpDir == "")
		{
			bOk = false;
			Global::AddError("Default system temp file directory", sSystemTmpDir,
					 "Temp file directory not specified");
		}
		// On verifie que le chemin est absolu
		else if (not IsAbsoluteFilePathName(sSystemTmpDir))
		{
			bOk = false;
			Global::AddError("Default system temp file directory", sSystemTmpDir,
					 "Temp file directory must be an absolute path");
		}
	}

	// Test l'existence du repertoire temporaire (le repertoire systeme peut avoir disparu)
	if (bOk and not DirExists(GetTmpDir()))
	{
		// Emission du message d'erreur
		// (emis uniquement pour cette cause)
		bOk = CheckApplicationTmpDir();
	}

	// Tentative de creation du nouveau repertoire applicatif
	if (bOk)
	{
		sApplicationTmpDir = CreateNewDirectory(sApplicationTmpDir);
		if (sApplicationTmpDir == "")
		{
			Global::AddError("Temp file directory", GetTmpDir(),
					 "Unable to create temp application directory (for " + sApplicationName + ")");
			bOk = false;
		}
	}

	// Creation et ouverture du fichier anchor, pour rendre le repertoire actif
	if (bOk)
	{
		assert(sApplicationTmpDir != "");

		// Ouverture du fichier en ecriture
		// La fonction p_fopen gere deja les locale correctement
		fApplicationTmpDirAnchorFile = p_fopen(BuildFilePathName(sApplicationTmpDir, GetAnchorFileName()), "w");

		// Nettoyage si echec
		if (fApplicationTmpDirAnchorFile == NULL)
		{
			Global::AddError("Application temp directory", sApplicationTmpDir,
					 "Unable to create file " + GetAnchorFileName() + " in temp directory");
			RemoveDirectory(sApplicationTmpDir);
			sApplicationTmpDir = "";
			bOk = false;
		}
		// Fermeture sinon
		else
			fclose(fApplicationTmpDirAnchorFile);
	}

	// Memorisation de la fraicheur de creation
	if (bOk)
		nApplicationTmpDirCreationFreshness = nApplicationTmpDirFreshness;

	// On initialise la date d'expiration avec une heure de delai
	// Cela ne peut etre fait que si tout s'est passe avec succes
	if (bOk)
		TouchApplicationTmpDir(3600);
	return bOk;
}

boolean FileService::CheckApplicationTmpDir()
{
	boolean bOk = true;
	ALString sCategoryLabel;
	ALString sTmpDir;

	// Personnalisation des messages d'erreur
	sTmpDir = GetTmpDir();
	if (sTmpDir == sUserTmpDir)
		sCategoryLabel = "Temp file directory";
	else
		sCategoryLabel = "Default system temp file directory";

	// Test si repertoire utilisateur specifie
	if (bOk)
	{
		bOk = sTmpDir != "";
		if (not bOk)
			Global::AddError(sCategoryLabel, sTmpDir, "Directory not specified");
	}

	// Test d'existence du repertoire utilisateur
	if (bOk)
	{
		bOk = FileService::DirExists(sTmpDir);
		if (not bOk)
			Global::AddError(sCategoryLabel, sTmpDir, "Directory does not exist");
	}

	// Test s'il y a de la place
	if (bOk)
	{
		bOk = FileService::GetDiskFreeSpace(sTmpDir) >= lMB;
		if (not bOk)
			Global::AddError(sCategoryLabel, sTmpDir, "Less than one MB available");
	}

	// Test d'existence du repertoire applicatif
	if (bOk and sApplicationTmpDir == "")
	{
		Global::AddError("Application temp directory", "", "Directory not created");
		bOk = false;
	}
	if (bOk and not FileService::DirExists(sApplicationTmpDir))
	{
		Global::AddError("Application temp directory", sApplicationTmpDir, "Directory does not exist");
		bOk = false;
	}
	return bOk;
}

const ALString FileService::GetApplicationTmpDir()
{
	// On renvoie son nom s'il a ete cree
	if (nApplicationTmpDirCreationFreshness == nApplicationTmpDirFreshness)
		return sApplicationTmpDir;
	// Sinon, on renvoie chaine vide
	else
		return "";
}

const ALString FileService::CreateTmpFile(const ALString& sBaseName, const Object* errorSender)
{
	ALString sFilePathName;

	require(sBaseName != "");
	require(GetFileName(sBaseName) == sBaseName);

	// Test si le repertoire applicatif a ete cree
	if (GetApplicationTmpDir() == "")
	{
		Global::AddError("Application temp directory", "", "Directory not created");
	}
	// Creation du fichier temporaire
	else
	{
		sFilePathName = CreateNewFile(BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sBaseName));

		// Erreur si fichier non cree
		if (sFilePathName == "" and errorSender != NULL)
			errorSender->AddError("Unable to create temporary file " + sBaseName + " in directory " +
					      GetApplicationTmpDir() + " " + GetLastSystemIOErrorMessage());
	}
	return sFilePathName;
}

const ALString FileService::CreateTmpDirectory(const ALString& sBaseName, const Object* errorSender)
{
	ALString sDirPathName;

	require(sBaseName != "");
	require(GetFileName(sBaseName) == sBaseName);

	// Test si le repertoire applicatif a ete cree
	if (GetApplicationTmpDir() == "")
	{
		Global::AddError("Application temp directory", "", "Directory not created");
	}
	// Creation du repertoire temporaire
	else
	{
		sDirPathName =
		    CreateNewDirectory(BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sBaseName));

		// Erreur si repertoire non cree
		if (sDirPathName == "" and errorSender != NULL)
			errorSender->AddError("Unable to create temporary directory " + sBaseName + " in directory " +
					      GetApplicationTmpDir() + " " + GetLastSystemIOErrorMessage());
	}
	return sDirPathName;
}

const ALString FileService::CreateUniqueTmpFile(const ALString& sBaseName, const Object* errorSender)
{
	ALString sFilePathName;
	boolean bOk;

	require(sBaseName != "");
	require(GetFileName(sBaseName) == sBaseName);

	// Test si le repertoire applicatif a ete cree
	if (GetApplicationTmpDir() == "")
	{
		Global::AddError("Application temp directory", "", "Directory not created");
	}
	// Creation du fichier temporaire
	else
	{
		sFilePathName = BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sBaseName);

		// Erreur si fichier deja existant
		if (FileExists(sFilePathName))
		{
			sFilePathName = "";
			if (errorSender != NULL)
				errorSender->AddError("Temporary file " + sBaseName + " already exists in directory " +
						      GetApplicationTmpDir());
		}
		// Creation sinon
		else
		{
			bOk = CreateEmptyFile(sFilePathName);

			// Erreur si fichier non cree
			if (not bOk)
			{
				sFilePathName = "";
				if (errorSender != NULL)
					errorSender->AddError("Unable to create temporary file " + sBaseName +
							      " in directory " + GetApplicationTmpDir() + " " +
							      GetLastSystemIOErrorMessage());
			}
		}
	}
	return sFilePathName;
}

const ALString FileService::CreateUniqueTmpDirectory(const ALString& sBaseName, const Object* errorSender)
{
	ALString sDirPathName;
	boolean bOk;

	require(sBaseName != "");
	require(GetFileName(sBaseName) == sBaseName);

	// Test si le repertoire applicatif a ete cree
	if (GetApplicationTmpDir() == "")
	{
		Global::AddError("Application temp directory", "", "Directory not created");
	}
	// Creation du repertoire temporaire
	else
	{
		sDirPathName = BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sBaseName);

		// Erreur si fichier deja existant
		if (FileExists(sDirPathName))
		{
			sDirPathName = "";
			if (errorSender != NULL)
				errorSender->AddError("Temporary directory " + sBaseName +
						      " already exists in directory " + GetApplicationTmpDir());
		}
		// Creation sinon
		else
		{
			bOk = MakeDirectory(sDirPathName);

			// Erreur si fichier non cree
			if (not bOk)
			{
				sDirPathName = "";
				if (errorSender != NULL)
					errorSender->AddError("Unable to create temporary directory " + sBaseName +
							      " in directory " + GetApplicationTmpDir() + " " +
							      GetLastSystemIOErrorMessage());
			}
		}
	}
	return sDirPathName;
}

void FileService::SetApplicationTmpDirAutoDeletion(boolean bValue)
{
	bApplicationTmpDirAutoDeletion = bValue;
}

boolean FileService::GetApplicationTmpDirAutoDeletion()
{
	return bApplicationTmpDirAutoDeletion;
}

void FileService::TouchApplicationTmpDir(int nRemainingSeconds)
{
	FILE* fApplicationTmpDirAnchorFile;
	time_t tCurrentTimestamp;
	const int nMaxRemainingSeconds = 366 * 24 * 3600;
	struct tm* pGMTExpirationDate;
	require(nRemainingSeconds >= 0);

	// On n'effectue le traitement que si le repertoire temporaire existe
	if (GetApplicationTmpDir() != "")
	{
		// Recherche de la date courante
		time(&tCurrentTimestamp);

		// Ajout du delai
		tCurrentTimestamp += min(nRemainingSeconds, nMaxRemainingSeconds);

		// Conversion en timestamps GMT
		pGMTExpirationDate = p_gmtime(&tCurrentTimestamp);

		// Ecriture dans le fichier anchor du libelle associe a la date d'expiration
		// La fonction p_fopen gere deja les locale correctement
		fApplicationTmpDirAnchorFile =
		    p_fopen(BuildFilePathName(GetApplicationTmpDir(), GetAnchorFileName()), "w");
		if (fApplicationTmpDirAnchorFile != NULL)
		{
			fprintf(fApplicationTmpDirAnchorFile, "GMT expiration date %04d-%02d-%02d %02d:%02d:%02d",
				pGMTExpirationDate->tm_year + 1900, pGMTExpirationDate->tm_mon + 1,
				pGMTExpirationDate->tm_mday, pGMTExpirationDate->tm_hour, pGMTExpirationDate->tm_min,
				pGMTExpirationDate->tm_sec);
			fclose(fApplicationTmpDirAnchorFile);
		}
	}
}

void FileService::UntouchApplicationTmpDir()
{
	FILE* fApplicationTmpDirAnchorFile;

	// On n'effectue le traitement que si le repertoire temporaire existe
	if (GetApplicationTmpDir() != "")
	{
		// Remise a vide du fichier anchor
		// La fonction p_fopen gere deja les locale correctement
		fApplicationTmpDirAnchorFile =
		    p_fopen(BuildFilePathName(GetApplicationTmpDir(), GetAnchorFileName()), "w");
		if (fApplicationTmpDirAnchorFile != NULL)
			fclose(fApplicationTmpDirAnchorFile);
	}
}

ALString FileService::GetURIScheme(const ALString& sURI)
{
	int nPos;
	ALString sScheme;

	// On renvoie la partie qui precede '://' si elle contient au moins deux caracteres (pour ne pas prendre en
	// compte c: sous windows)
	nPos = sURI.Find("://");

	// Si on a trouve ':' (nPos!=-1) et qu'il y au moins 2 caracteres avant (nPos>1)
	if (nPos > 1)
		sScheme = sURI.Left(nPos);
	return sScheme;
}

boolean FileService::IsURIWellFormed(const ALString& sURI)
{
	ALString sScheme;
	int nNextSlashPos;
	int i;

	// On accepte les chemins standards
	sScheme = GetURIScheme(sURI);
	if (sScheme == "")
		return true;
	// Sinon, on verifie le reste de la syntaxe
	else
	{
		// Verification de base
		assert(sURI.GetLength() > sScheme.GetLength() + 2);
		assert(sURI.GetAt(sScheme.GetLength()) == ':');
		assert(sURI.GetAt(sScheme.GetLength() + 1) == '/');
		assert(sURI.GetAt(sScheme.GetLength() + 2) == '/');

		// Recherche de la position du prochain '/' suivant, en interdisant les ':'
		nNextSlashPos = -1;
		for (i = sScheme.GetLength() + 3; i < sURI.GetLength(); i++)
		{
			if (sURI.GetAt(i) == ':')
				break;
			if (sURI.GetAt(i) == '/')
			{
				nNextSlashPos = i;
				break;
			}
		}

		// Ok si un slash a ete trouve
		return (nNextSlashPos != -1);
	}
}

const ALString FileService::BuildURI(const ALString& sScheme, const ALString& sHostName, const ALString& sFileName)
{
	ALString sRelativePath;

	require(GetURIScheme(sFileName) == "");

	assert(sScheme != "" or (sScheme == "" and sHostName == ""));

	// Cas particulier des chemin locaux : interdits dans les "vraies" URI mais
	// pour etre compatible avec le code existant, on ajoute "./" devant
	if (not IsAbsoluteFilePathName(sFileName))
	{
		// On ne rajoute "./" que s'il n'y a pas deja "." au debut
		if (sFileName.GetLength() >= 1 and sFileName.GetAt(0) != '.')
			sRelativePath = sRelativePath + "." + GetURIFileSeparator();
	}
	return sScheme + "://" + sHostName + "/" + sRelativePath + sFileName;
}

const ALString FileService::BuildLocalURI(const ALString& sFileName)
{
	return BuildURI(sRemoteScheme, GetLocalHostName(), sFileName);
}

const ALString FileService::GetURIHostName(const ALString& sFileURI)
{
	ALString sHostName;
	ALString sScheme;
	int nNextSlashPos;
	int i;

	// Extraction du schema
	sScheme = GetURIScheme(sFileURI);

	// Recherche du host dans le cas d'un schema
	if (sScheme != "")
	{
		// Verification de base
		assert(sFileURI.GetLength() > sScheme.GetLength() + 2);
		assert(sFileURI.GetAt(sScheme.GetLength()) == ':');
		assert(sFileURI.GetAt(sScheme.GetLength() + 1) == '/');
		assert(sFileURI.GetAt(sScheme.GetLength() + 2) == '/');

		// Recherche de la position du prochain '/' suivant en interdisant les ':'
		nNextSlashPos = -1;
		for (i = sScheme.GetLength() + 3; i < sFileURI.GetLength(); i++)
		{
			if (sFileURI.GetAt(i) == ':')
				break;
			if (sFileURI.GetAt(i) == '/')
			{
				nNextSlashPos = i;
				break;
			}
		};

		// Ok si un slash a ete trouve, sinon on rend un hostname vide
		if (nNextSlashPos != -1)
			sHostName = sFileURI.Mid(sScheme.GetLength() + 3, nNextSlashPos - (sScheme.GetLength() + 3));
	}
	return sHostName;
}

boolean FileService::IsLocalURI(const ALString& sFileURI)
{
	return FileService::GetURIScheme(sFileURI) == "" or
	       (FileService::GetURIScheme(sFileURI) == sRemoteScheme and
		FileService::GetURIHostName(sFileURI) == GetLocalHostName());
}

const ALString FileService::GetURIFilePathName(const ALString& sFileURI)
{
	ALString sFilePathName;
	ALString sScheme;
	int nNextSlashPos;
	int i;

	// Extraction du schema
	sScheme = GetURIScheme(sFileURI);

	// Recherche du chemin du fichier dans le cas d'un schema
	if (sScheme != "")
	{
		// Verification de base
		assert(sFileURI.GetLength() > sScheme.GetLength() + 2);
		assert(sFileURI.GetAt(sScheme.GetLength()) == ':');
		assert(sFileURI.GetAt(sScheme.GetLength() + 1) == '/');
		assert(sFileURI.GetAt(sScheme.GetLength() + 2) == '/');

		// Recherche de la position du prochain '/' suivant en interdisant les ':'
		nNextSlashPos = -1;
		for (i = sScheme.GetLength() + 3; i < sFileURI.GetLength(); i++)
		{
			if (sFileURI.GetAt(i) == ':')
				break;
			if (sFileURI.GetAt(i) == '/')
			{
				nNextSlashPos = i;
				break;
			}
		}

		// Cas ou le chemin ne contient que scheme:///
		if (nNextSlashPos == sFileURI.GetLength() - 1)
		{
			sFilePathName = "/";
		}
		else
		{
			// Ok si un slash a ete trouve
			if (nNextSlashPos != -1)
			{
#ifdef _WIN32
				// Sur windows on renvoie C:\XXX et non /C:\XXX
				sFilePathName = sFileURI.Right(sFileURI.GetLength() - nNextSlashPos - 1);
#else
				// Par contre, sur linux on renvoie le slash : /home/XXX
				// Mais si c'est un chemin relatif, on ne prend pas le slash
				if (sFileURI.GetLength() > nNextSlashPos and sFileURI.GetAt(nNextSlashPos + 1) == '.')
					sFilePathName = sFileURI.Right(sFileURI.GetLength() - nNextSlashPos - 1);
				else
					sFilePathName = sFileURI.Right(sFileURI.GetLength() - nNextSlashPos);
#endif // _WIN32
			}
		}
	}

	// On renvoie le chemin du fichier si on a pu l'extraire
	if (sFilePathName != "")
		return sFilePathName;
	// Sinon, on renvoie le chemin initial
	else
		return sFileURI;
}

const ALString FileService::GetURIUserLabel(const ALString& sFileURI)
{
	if (bURISmartLabels)
	{
		if ((FileService::GetURIScheme(sFileURI) == sRemoteScheme) and
		    (FileService::GetURIHostName(sFileURI) == GetLocalHostName()))
			return FileService::GetURIFilePathName(sFileURI);
		else
			return sFileURI;
	}
	else
		return sFileURI;
}

void FileService::SetIOStatsActive(boolean bIsActive)
{
	bIOStats = bIsActive;
}

boolean FileService::GetIOStatsActive()
{
	return bIOStats;
}

boolean FileService::GetURISmartLabels()
{
	return bURISmartLabels;
}

void FileService::SetURISmartLabels(boolean bValue)
{
	bURISmartLabels = bValue;
}

ALString FileService::GetSystemNulFileName()
{
#ifdef _WIN32
	return "NUL";
#else
	return "/dev/null";
#endif
}

boolean FileService::FileCompare(const ALString& sFileName1, const ALString& sFileName2)
{
	FILE* file1;
	FILE* file2;
	longint lFileSize;
	longint lPos;
	char c1;
	char c2;
	boolean bSame;

	require(FileService::FileExists(sFileName1));
	require(FileService::FileExists(sFileName2));

	// Test sur la taille
	lFileSize = FileService::GetFileSize(sFileName1);
	if (FileService::GetFileSize(sFileName2) != lFileSize)
		return false;

	///////////////////////////////////////
	// Test caractere par caractere

	// Ouverture des fichiers
	FileService::OpenInputBinaryFile(sFileName1, file1);
	FileService::OpenInputBinaryFile(sFileName2, file2);

	// Comparaison
	bSame = (file1 != NULL and file2 != NULL);
	if (bSame)
	{
		lPos = 0;
		// On parcours tout le fichier, sans se baser sur feof
		// En cas d'erreur, on est ainsi sur de s'arreter sur
		// le premier fichier en erreur (fgetc retourne EOF),
		// et au pire, on arrete apres la taille du fichier
		while (lPos < lFileSize)
		{
			lPos++;
			c1 = (char)fgetc(file1);
			c2 = (char)fgetc(file2);
			if (c1 != c2)
			{
				bSame = false;
				break;
			}
		}
	}

	// Fermeture des fichiers
	if (file1 != NULL)
		fclose(file1);
	if (file2 != NULL)
		fclose(file2);

	return bSame;
}

longint FileService::GetDiskFreeSpace(const ALString& sPathName)
{
	longint lFreeDiskSpace = 0;

	require(not FileExists(sPathName));
	if (sPathName == "")
		lFreeDiskSpace = DiskGetFreeSpace(".");
	else
		lFreeDiskSpace = DiskGetFreeSpace(sPathName);
	return lFreeDiskSpace;
}

const ALString FileService::GetPortableTmpFilePathName(const ALString& sFilePathName)
{
	ALString sAppTmpDir;
	ALString sTmpDir;
	const ALString sAppTmpDirAlias = "APPTMPDIR";
	const ALString sTmpDirAlias = "TMPDIR";
	int nTmpDirLength;
	ALString sPortableTmpFilePathName;

	// Remplacement eventuel du nom de chemin par le repertoire applicatif puis systeme temporaire
	sAppTmpDir = GetApplicationTmpDir();
	sTmpDir = GetTmpDir();
	sPortableTmpFilePathName = sFilePathName;
	if (sAppTmpDir != "" and sAppTmpDir.GetLength() <= sFilePathName.GetLength() and
	    sFilePathName.Left(sAppTmpDir.GetLength()) == sAppTmpDir)
	{
		nTmpDirLength = sAppTmpDir.GetLength();
		if (sFilePathName.GetLength() > nTmpDirLength and
		    sFilePathName.GetAt(nTmpDirLength) == GetFileSeparator())
			nTmpDirLength++;
		sPortableTmpFilePathName =
		    BuildFilePathName(sAppTmpDirAlias, sFilePathName.Right(sFilePathName.GetLength() - nTmpDirLength));
	}
	if (sTmpDir.GetLength() <= sFilePathName.GetLength() and sFilePathName.Left(sTmpDir.GetLength()) == sTmpDir)
	{
		nTmpDirLength = sTmpDir.GetLength();
		if (sFilePathName.GetLength() > nTmpDirLength and
		    sFilePathName.GetAt(nTmpDirLength) == GetFileSeparator())
			nTmpDirLength++;
		sPortableTmpFilePathName =
		    BuildFilePathName(sTmpDirAlias, sFilePathName.Right(sFilePathName.GetLength() - nTmpDirLength));
	}

	// Uniformation du caractere separateur de chemin
#ifdef _WIN32
	for (int i = 0; i < sPortableTmpFilePathName.GetLength(); i++)
	{
		if (sPortableTmpFilePathName.GetAt(i) == '\\')
			sPortableTmpFilePathName.SetAt(i, '/');
	}
#endif
	return sPortableTmpFilePathName;
}

void FileService::Test()
{
	int i;
	ALString sFileName;
	ALString sPathName;
	ALString sFilePathName;
	ALString sFilePathName2;
	ALString sFilePathName3;
	ALString sSearchedFile;
	ALString sNewDirName;
	ALString sNewSuperDirName;
	ALString sFullDirName;
	ALString sURIFilePathName;
	StringVector svFileNames;
	StringVector svDirectoryNames;
	fstream fst;
	boolean bOk;

	// Test de base
	cout << "Test de la classe FileService" << endl;
	cout << "SYS PATH\tSeparateur des fichiers dans les chemins : " << GetFileSeparator() << endl;
	cout << "SYS PATH\tSeparateur de chemins dans le path : " << GetPathSeparator() << endl;
	cout << "SYS PATH\tListe des repertoires du path : " << GetPathDirectoryList() << endl;
	cout << "SYS PATH\tRecherche du repertoire des fichiers temporaires : " << GetTmpDir() << endl;
	cout << "SYS PATH\tEspace disque disponible dans le repertoire des fichiers temporaires : "
	     << LongintToHumanReadableString(GetDiskFreeSpace(GetTmpDir())) << endl;
	cout << "SYS PATH\tEspace disque disponible dans un repertoire inexistant : "
	     << GetDiskFreeSpace("////toto////") << endl;
	cout << "Test si le repertoire des fichiers temporaires est absolu: " << IsAbsoluteFilePathName(GetTmpDir())
	     << endl;
	cout << endl << "Dans la suite, le repertoire des fichiers temporaires est designe par TMPDIR" << endl << endl;

	// Test d'ouverture d'un fichier au nom incorrect
	cout << "SYS PATH\tOuverture d'un fichier au nom incorrect: " << OpenInputFile("////toto////", fst) << endl;

	// Test de presence de chemin
	cout << endl << "Indique si un chemin de fichier comporte une partie chemin" << endl;
	sFileName = "toto.txt";
	sFilePathName = FileService::GetFileSeparator() + sFileName;
	sFilePathName2 = "chemin" + sFilePathName;
	sFilePathName3 = GetFileSeparator() + sFilePathName2;
	//
	cout << GetPortableTmpFilePathName(sFileName) << "->" << IsPathInFilePath(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << IsPathInFilePath(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << IsPathInFilePath(sFilePathName2) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName3) << "->" << IsPathInFilePath(sFilePathName3) << endl;

	// Indique si un chemin est absolu
	cout << endl << "Indique si un chemin de fichier est absolu" << endl;
	cout << GetPortableTmpFilePathName(sFileName) << "->" << IsAbsoluteFilePathName(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << IsAbsoluteFilePathName(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << IsAbsoluteFilePathName(sFilePathName2) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName3) << "->" << IsAbsoluteFilePathName(sFilePathName3) << endl;

	// Extraction de nom de chemin
	cout << endl << "Extraction de la partie chemin depuis un chemin de fichier" << endl;
	cout << GetPortableTmpFilePathName(sFileName) << "->" << GetPathName(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << GetPathName(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << GetPathName(sFilePathName2) << endl;

	// Extraction de nom de fichier
	cout << endl << "Extraction de la partie fichier depuis un chemin de fichier" << endl;
	cout << GetPortableTmpFilePathName(sFileName) << "->" << GetFileName(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << GetFileName(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << GetFileName(sFilePathName2) << endl;

	// Extraction de prefixe de fichier
	cout << endl << "Extraction de la partie prefixe de fichier depuis un chemin de fichier" << endl;
	cout << GetPortableTmpFilePathName(sFileName) << "->" << GetFilePrefix(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << GetFilePrefix(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << GetFilePrefix(sFilePathName2) << endl;

	// Extraction de suffixe de fichier
	cout << endl << "Extraction de la partie suffixe de fichier depuis un chemin de fichier" << endl;
	cout << GetPortableTmpFilePathName(sFileName) << "->" << GetFileSuffix(sFileName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName) << "->" << GetFileSuffix(sFilePathName) << endl;
	cout << GetPortableTmpFilePathName(sFilePathName2) << "->" << GetFileSuffix(sFilePathName2) << endl;

	// Test de GetPathName
	cout << endl << "Extract path name" << endl;
#ifdef _WIN32
	sFilePathName = "c:\\standard\\linux\\path\\file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "c:\\standard\\linux\\path\\\\\\\\file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "c:\\file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "c:\\\\\\\\\\file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
#else
	sFilePathName = "/standard/linux/path/file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "/standard/linux/path////file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "/file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "//////file.txt";
	cout << "SYS PATH\t" << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
#endif // __linux_or_apple__
	sFilePathName = "hdfs:///standard/path/file.txt";
	cout << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "hdfs:///standard/path////file.txt";
	cout << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "hdfs:///file.txt";
	cout << sFilePathName << " => " << GetPathName(sFilePathName) << endl;
	sFilePathName = "gs:///////file.txt";
	cout << sFilePathName << " => " << GetPathName(sFilePathName) << endl;

	// Test de BuildFilePathName
	cout << endl << "Build path" << endl;
	sPathName = "/standard/path/";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "/standard/path";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "/standard/path///";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "/";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "////";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "gs:///standard/path///";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "gs:///";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = "gs://////";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	sPathName = ".";
	cout << sPathName << " => " << BuildFilePathName(sPathName, "file.txt") << endl;
	cout << sPathName << " (.) => " << BuildFilePathName(sPathName, "./file.txt") << endl;
	cout << sPathName << " (..) => " << BuildFilePathName(sPathName, "../file.txt") << endl;

	// Destruction eventuelle du fichier pour repartir du meme etat et
	// assurer la reproductibilite des tests
	RemoveFile(sFilePathName);

	// Construction d'un nom de chemin
	sFileName = "toto.txt";
	sFilePathName = FileService::GetFileSeparator() + sFileName;
	cout << endl << "Construction d'un chemin complet de fichier" << endl;
	sPathName = GetTmpDir();
	cout << GetPortableTmpFilePathName(sPathName) << " + " << GetPortableTmpFilePathName(sFileName) << endl;
	sFilePathName = BuildFilePathName(sPathName, sFileName);
	cout << "-> " << GetPortableTmpFilePathName(sFilePathName) << endl;

	// Modification de parties d'un nom de fichier
	cout << endl << "Modification de parties d'un nom de fichier" << endl;
	cout << "\t"
	     << "Initial file path name: " << GetPortableTmpFilePathName(sFileName) << endl;
	cout << "\t"
	     << "Empty dir: " << GetPortableTmpFilePathName(SetPathName(sFilePathName, "")) << endl;
	cout << "\t"
	     << "New dir: " << GetPortableTmpFilePathName(SetPathName(sFilePathName, "newdir")) << endl;
	cout << "\t"
	     << "Empty file: " << GetPortableTmpFilePathName(SetFileName(sFilePathName, "")) << endl;
	cout << "\t"
	     << "New file: " << GetPortableTmpFilePathName(SetFileName(sFilePathName, "newfile")) << endl;
	cout << "\t"
	     << "Empty prefix: " << GetPortableTmpFilePathName(SetFilePrefix(sFilePathName, "")) << endl;
	cout << "\t"
	     << "New prefix: " << GetPortableTmpFilePathName(SetFilePrefix(sFilePathName, "newprefix")) << endl;
	cout << "\t"
	     << "Empty suffix: " << GetPortableTmpFilePathName(SetFileSuffix(sFilePathName, "")) << endl;
	cout << "\t"
	     << "New suffix: " << GetPortableTmpFilePathName(SetFileSuffix(sFilePathName, "newsuffix")) << endl;

	// Calcul d'un nouveau nom de fichier
	cout << endl << "Calcul d'un nom de fichier inexistant a partir d'un nom de base" << endl;
	sFilePathName2 = CreateNewFile(sFilePathName);
	cout << GetPortableTmpFilePathName(GetFileName(sFilePathName)) << " -> "
	     << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << endl;

	// Ouverture d'un fichier en ecriture
	cout << endl << "Ouverture d'un fichier en ecriture" << endl;
	bOk = FileService::OpenOutputFile(sFilePathName2, fst);
	if (bOk)
	{
		cout << "OK file " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " opened" << endl;
		cout << "\tecriture dans le fichier" << endl;
		for (i = 0; i < 1000; i++)
			fst << i << " ligne test d'ecriture" << endl;
		fst.close();
	}
	else
		cout << "KO file " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " not opened";

	// Test d'existence d'un fichier existant
	cout << endl
	     << "Test d'existence du fichier existant " << GetPortableTmpFilePathName(GetFileName(sFilePathName2))
	     << endl;
	if (FileExists(sFilePathName2))
		cout << "OK " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " exists" << endl;
	else
		cout << "KO " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " not exists" << endl;

	// Test d'existence d'un fichier inexistant
	sFilePathName3 = sFilePathName2 + ".unknown";
	cout << endl
	     << "Test d'existence du fichier inexistant " << GetPortableTmpFilePathName(GetFileName(sFilePathName3))
	     << endl;
	if (FileExists(sFilePathName3))
		cout << "KO " << GetPortableTmpFilePathName(GetFileName(sFilePathName3)) << " exists" << endl;
	else
		cout << "OK " << GetPortableTmpFilePathName(GetFileName(sFilePathName3)) << " not exists" << endl;

	// Recherche d'un fichier
	cout << endl
	     << "Recherche du fichier " << GetPortableTmpFilePathName(sFileName) << " dans le PATH et TMP" << endl;
	sSearchedFile = GetFilePath(sFileName, GetPathDirectoryList() + GetPathSeparator() + GetTmpDir());
	cout << "present : " << GetPortableTmpFilePathName(GetFileName(sSearchedFile)) << endl;

	// Taille d'un fichier
	cout << endl << "Test de la taille d'un fichiers en bytes" << endl;
	cout << "SYS PATH\tFile size\t" << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " -> "
	     << GetFileSize(sFilePathName2) << endl;

	// Ouverture d'un fichier en lecture
	cout << endl << "Test d'ouverture d'un fichier en lecture" << endl;
	bOk = OpenInputFile(sFilePathName2, fst);
	if (bOk)
	{
		cout << "OK file " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " opened" << endl;
		cout << "\tlecture dans le fichier des 5 premieres lignes" << endl;
		char sBuffer[100];
		for (i = 0; i < 4; i++)
		{
			fst.getline(sBuffer, sizeof(sBuffer));
			cout << "\t" << sBuffer << endl;
		}
		fst.close();
	}
	else
		cout << "KO file " << GetPortableTmpFilePathName(GetFileName(sFilePathName2)) << " not opened";
	RemoveFile(sFilePathName2);

	// Test de creation/supression de repertoire
	sNewDirName = "NewDir";
	sFullDirName = BuildFilePathName(GetTmpDir(), sNewDirName);
	bOk = MakeDirectory(sFullDirName);
	cout << "Test de creation de repertoire " << GetPortableTmpFilePathName(sFullDirName) << ": " << bOk << endl;
	bOk = RemoveDirectory(sFullDirName);
	cout << "Test de supression de repertoire " << GetPortableTmpFilePathName(sFullDirName) << ": " << bOk << endl;

	// Test de creation/supression de repertoire avec les repertoire intermediaires
	sNewSuperDirName = "NewSuperDir";
	sFullDirName = BuildFilePathName(GetTmpDir(), sNewSuperDirName + GetFileSeparator() + sNewDirName);
	bOk = MakeDirectories(sFullDirName);
	cout << "Test de creation de repertoires " << GetPortableTmpFilePathName(sFullDirName) << ": " << bOk << endl;
	bOk = RemoveDirectory(sFullDirName);
	bOk = RemoveDirectory(BuildFilePathName(GetTmpDir(), sNewSuperDirName));

	// Test de gestion des fichiers temporaires applicatifs
	cout << "Test de gestion des fichiers temporaires applicatifs" << endl;
	SetApplicationName("ApplicationTmp");
	CreateApplicationTmpDir();
	TouchApplicationTmpDir(10);
	UntouchApplicationTmpDir();
	TouchApplicationTmpDir(10);
	cout << "\tApplication tmp dir: " << GetPortableTmpFilePathName(GetApplicationTmpDir()) << endl;
	sFileName = "App";
	CreateNewDirectory(BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sFileName + "Dir"));
	CreateNewFile(BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sFileName + "File"));
	CreateNewFile(BuildFilePathName(GetApplicationTmpDir(), GetTmpPrefix() + sFileName + "File"));
	CreateUniqueTmpDirectory("TmpDir", NULL);
	CreateTmpDirectory("TmpDir", NULL);
	CreateTmpDirectory("TmpDir", NULL);
	CreateUniqueTmpFile("TmpFile", NULL);
	CreateTmpFile("TmpFile", NULL);
	CreateTmpFile("TmpFile", NULL);
	//
	cout << "Contenu du repertoire temporaire " << GetPortableTmpFilePathName(GetApplicationTmpDir()) << endl;
	GetDirectoryContent(GetApplicationTmpDir(), &svDirectoryNames, &svFileNames);
	for (i = 0; i < svDirectoryNames.GetSize(); i++)
	{
		cout << "SYS PATH\tDir " << svDirectoryNames.GetAt(i) << endl;
		if (i == 10)
		{
			cout << "..." << endl;
			break;
		}
	}
	for (i = 0; i < svFileNames.GetSize(); i++)
	{
		cout << "SYS PATH\tFile " << svFileNames.GetAt(i) << endl;
		if (i == 10)
		{
			cout << "..." << endl;
			break;
		}
	}
	//
	cout << "Destruction recursive du repertoire temporaire " << GetPortableTmpFilePathName(GetApplicationTmpDir());
	SetApplicationTmpDirAutoDeletion(true);
	bOk = DeleteApplicationTmpDir();
	cout << "\t" << bOk << endl;

	// Test des URI
	StringVector svURItests;
	svURItests.Add("hdfs:///tmp/test.txt");
	svURItests.Add("hdfs://datanode/tmp/test.txt");
	svURItests.Add("hdfs://datanode-wrong-uri");
	svURItests.Add("s3:///good-URI/test");
	svURItests.Add("s3://host/good-URI/test");
	svURItests.Add("gcs:///good-URI.txt");
	svURItests.Add("gcs:///");

	// Test oriente linux
	svURItests.Add("file:///home/test.txt");
	svURItests.Add("file://host-name/home/test.txt");
	svURItests.Add("file:///home/test.txt");
	svURItests.Add("file:///./test.txt");
	svURItests.Add("file:///..\\test.txt");

	// Test orientes windows
	svURItests.Add("file:///c:\\home\\test.txt");
	svURItests.Add("file://host-name/c:\\home\\test.txt");

	// Lancement des tests
	for (i = 0; i < svURItests.GetSize(); i++)
	{
		cout << svURItests.GetAt(i) << endl;
		cout << "\t well-formed : " << FileService::IsURIWellFormed(svURItests.GetAt(i)) << endl;
		cout << "\t scheme : " << FileService::GetURIScheme(svURItests.GetAt(i)) << endl;
		cout << "\t file name : " << FileService::GetURIFilePathName(svURItests.GetAt(i)) << endl;
		cout << "\t host name : " << FileService::GetURIHostName(svURItests.GetAt(i)) << endl;
		cout << endl;
	}

	// Test de la reservation de taille d'un fichier
	{
		FILE* fTestReserve;
		longint lReserveSize = lMB;
		char sBuffer[1];
		longint lReadSize;
		longint lNumberA;
		longint lNumberB;

		// Nom du fichier
		sPathName = GetTmpDir();
		sFileName = "TestReserveSize.txt";
		sFilePathName = BuildFilePathName(sPathName, sFileName);
		cout << "Test de reservation de taille pour un fichier en ecriture de taille "
		     << LongintToHumanReadableString(2 * lReserveSize) << ": "
		     << GetPortableTmpFilePathName(sFilePathName) << endl;

		// Ouverture un premiere fois en ecriture, et ecriture de caractere A
		bOk = OpenOutputBinaryFile(sFilePathName, fTestReserve);
		if (bOk)
		{
			ReserveExtraSize(fTestReserve, lReserveSize);
			for (i = 0; i < lReserveSize; i++)
				fwrite("A", sizeof(char), 1, fTestReserve);
			CloseOutputBinaryFile(sFilePathName, fTestReserve);
		}

		// Ouverture un premiere fois en ecriture, et ecriture de caractere B
		bOk = OpenOutputBinaryFileForAppend(sFilePathName, fTestReserve);
		if (bOk)
		{
			ReserveExtraSize(fTestReserve, lReserveSize);
			for (i = 0; i < lReserveSize; i++)
				fwrite("B", sizeof(char), 1, fTestReserve);
			CloseOutputBinaryFile(sFilePathName, fTestReserve);
		}

		// Test des caracteres ecrit
		lNumberA = 0;
		lNumberB = 0;
		bOk = OpenInputBinaryFile(sFilePathName, fTestReserve);
		if (bOk)
		{
			while (not feof(fTestReserve))
			{
				lReadSize = fread(sBuffer, sizeof(char), 1, fTestReserve);
				if (lReadSize == 1)
				{
					if (sBuffer[0] == 'A')
						lNumberA++;
					if (sBuffer[0] == 'B')
						lNumberB++;
				}
				assert(lNumberA >= lNumberB);
			}
			CloseInputBinaryFile(sFilePathName, fTestReserve);
		}
		cout << "\tNumber A\t" << lNumberA << endl;
		cout << "\tNumber B\t" << lNumberB << endl;
		cout << "\tFile check\t" << BooleanToString(lNumberA == lReserveSize and lNumberB == lReserveSize)
		     << endl;

		// Destruction du fichier
		RemoveFile(sFilePathName);
	}
}

char FileService::GetURIFileSeparator()
{
	return '/';
}

boolean FileService::SystemSeekPositionInBinaryFile(FILE* fFile, longint lOffset, int nWhence)
{
	boolean bOk;

	require(nWhence == SEEK_SET or nWhence == SEEK_CUR or nWhence == SEEK_END);

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#ifdef _WIN32
	_fseeki64(fFile, lOffset, nWhence);
#elif defined(__APPLE__)
	fseeko(fFile, lOffset, nWhence);
#else
	fseeko64(fFile, lOffset, nWhence);
#endif
	bOk = (ferror(fFile) == 0);
	if (not bOk)
	{
		ALString sTmp;
		if (nWhence == SEEK_SET)
			Global::AddError("File", "",
					 sTmp + "Unable to call seek from begin of file with offset " +
					     LongintToString(lOffset) + " " + GetLastSystemIOErrorMessage());
		else if (nWhence == SEEK_CUR)
			Global::AddError("File", "",
					 sTmp + "Unable to call seek from current position of file with offset " +
					     LongintToString(lOffset) + " " + GetLastSystemIOErrorMessage());
		else if (nWhence == SEEK_END)
			Global::AddError("File", "",
					 sTmp + "Unable to call seek from end of file with offset " +
					     LongintToString(lOffset) + " " + GetLastSystemIOErrorMessage());
	}
	return bOk;
}

boolean FileService::DeleteApplicationTmpDir()
{
	boolean bOk = true;

	// Destruction uniquement si necessaire
	if (sApplicationTmpDir != "" and DirExists(sApplicationTmpDir))
	{
		// Nettoyage prealable, en gardant le fichier anchor ouvert pour laisser
		// le repertoire actif vis a vis des autres applications le temps de sa destruction
		if (bApplicationTmpDirAutoDeletion)
			DeleteTmpDirectory(sApplicationTmpDir);
	}
	sApplicationTmpDir = "";
	return bOk;
}

char FileService::GetTmpPrefix()
{
	return '~';
}

boolean FileService::DeleteTmpDirectory(const ALString& sTmpPathName)
{
	boolean bOk = true;
	StringVector svDirectoryNames;
	StringVector svFileNames;
	ALString sName;
	int i;

	require(sTmpPathName != "");
	require(GetFileName(sTmpPathName).GetAt(0) == GetTmpPrefix());

	// Uniquement en mode auto-supression
	if (bApplicationTmpDirAutoDeletion)
	{
		// Acces au contenu du repertoire
		GetDirectoryContent(sTmpPathName, &svDirectoryNames, &svFileNames);

		// Destruction de l'eventuel fichier anchor en premier pour indiquer
		// aux autre applications que le directory n'a plus un statut de
		// directory temporaire, et qu'il faut donc l'ignorer
		RemoveFile(BuildFilePathName(sTmpPathName, GetAnchorFileName()));

		// Destruction recursive des sous-repertoires
		for (i = 0; i < svDirectoryNames.GetSize(); i++)
		{
			sName = svDirectoryNames.GetAt(i);
			if (sName.GetAt(0) == GetTmpPrefix())
				bOk = DeleteTmpDirectory(BuildFilePathName(sTmpPathName, sName)) and bOk;
		}

		// Destruction des sous fichiers
		for (i = 0; i < svFileNames.GetSize(); i++)
		{
			sName = svFileNames.GetAt(i);
			if (sName.GetAt(0) == GetTmpPrefix())
				RemoveFile(BuildFilePathName(sTmpPathName, sName));
		}

		// Destruction du repertoire lui-meme
		bOk = RemoveDirectory(sTmpPathName) and bOk;
	}
	return bOk;
}

void FileService::CleanExpiredApplicationTmpDirs(const ALString& sExpiredApplicationTmpDir)
{
	ALString sPathName;
	ALString sDirectoryPrefix;
	ALString sDirectorySuffix;
	ALString sTestedDirectoryName;
	boolean bIsExpiredDirectory;
	ALString sAnchorPathName;
	FILE* fAnchor;
	const int nBufferSize = 100;
	char sBuffer[nBufferSize];
	char* sReturn;
	const ALString sFormat = "GMT expiration date 0000-00-00 00:00:00";
	int i;
	char c;
	longint lExpirationTimestamp;
	longint lCurrentTimeStamp;
	time_t tCurrentTimestamp;
	struct tm* pGMTExpirationDate;
	int nId;
	int nOtherTrial;
	const int nMaxOtherTrials = 100;

	// Decomposition du nom du repertoire
	sPathName = GetPathName(sExpiredApplicationTmpDir);
	sDirectoryPrefix = GetFilePrefix(sExpiredApplicationTmpDir);
	sDirectorySuffix = GetFileSuffix(sExpiredApplicationTmpDir);

	// Parcours avec les variante a un suffixe pres
	nOtherTrial = 0;
	nId = 0;
	sTestedDirectoryName = sExpiredApplicationTmpDir;
	while (nOtherTrial < nMaxOtherTrials)
	{
		nId++;
		nOtherTrial++;

		// Test si le repertoire correspond a un repertoire temporaire applicatif inactif
		// C'est le cas s'il contient un fichier anchor contenant une date d'exprimeation valide
		// et si cette date est expiree
		bIsExpiredDirectory = DirExists(sTestedDirectoryName);
		fAnchor = NULL;
		sAnchorPathName = BuildFilePathName(sTestedDirectoryName, GetAnchorFileName());
		if (bIsExpiredDirectory)
			bIsExpiredDirectory = FileExists(sAnchorPathName);
		if (bIsExpiredDirectory)
		{
			// La fonction p_fopen gere deja les locale correctement
			fAnchor = p_fopen(sAnchorPathName, "r");
			if (fAnchor == NULL)
				bIsExpiredDirectory = false;
			else
			{
				// Lecture du contenu du fichier
				assert(sFormat.GetLength() < nBufferSize);
				sReturn = fgets(sBuffer, nBufferSize,
						fAnchor); // warning : ignoring return value of 'char* fgets
				fclose(fAnchor);

				// Test si lecture correcte
				if (sReturn == NULL)
					bIsExpiredDirectory = false;

				// Test de longueur
				if (bIsExpiredDirectory)
				{
					if ((int)strlen(sBuffer) != sFormat.GetLength())
						bIsExpiredDirectory = false;
				}

				// Test du format
				if (bIsExpiredDirectory)
				{
					for (i = 0; i < sFormat.GetLength(); i++)
					{
						c = sFormat.GetAt(i);

						// Test des libelles
						if (c != '0')
						{
							if (sBuffer[i] != c)
							{
								bIsExpiredDirectory = false;
								break;
							}
						}
						// Test des valeurs numeriques
						else
						{
							if (not isdigit(sBuffer[i]))
							{
								bIsExpiredDirectory = false;
								break;
							}
						}
					}
				}

				// Collecte du timestamp sous forme de longint
				lExpirationTimestamp = 0;
				if (bIsExpiredDirectory)
				{
					for (i = 0; i < sFormat.GetLength(); i++)
					{
						c = sFormat.GetAt(i);

						// Collecte des valeurs numeriques sous forme de puissances de 10
						if (isdigit(sBuffer[i]))
						{
							lExpirationTimestamp *= 10;
							lExpirationTimestamp += sBuffer[i] - '0';
						}
					}
				}

				// Recherche du timestamp courant
				lCurrentTimeStamp = 0;
				if (bIsExpiredDirectory)
				{
					// Recherche de la date courante
					time(&tCurrentTimestamp);

					// Conversion en timestamps GMT
					pGMTExpirationDate = p_gmtime(&tCurrentTimestamp);

					// Ecriture au format longint sous forme de puissances de 10
					lCurrentTimeStamp = pGMTExpirationDate->tm_year + 1900;
					lCurrentTimeStamp *= 100;
					lCurrentTimeStamp += pGMTExpirationDate->tm_mon + 1;
					lCurrentTimeStamp *= 100;
					lCurrentTimeStamp += pGMTExpirationDate->tm_mday;
					lCurrentTimeStamp *= 100;
					lCurrentTimeStamp += pGMTExpirationDate->tm_hour;
					lCurrentTimeStamp *= 100;
					lCurrentTimeStamp += pGMTExpirationDate->tm_min;
					lCurrentTimeStamp *= 100;
					lCurrentTimeStamp += pGMTExpirationDate->tm_sec;
				}

				// Test si date expiree
				if (bIsExpiredDirectory)
				{
					bIsExpiredDirectory = (lCurrentTimeStamp > lExpirationTimestamp);
				}
			}
		}

		// Nettoyage du repertoire s'il est inactif
		if (bIsExpiredDirectory)
		{
			// On nettoie le repertoire temporaire
			// Cette methode detruit le repertoire anchor en premier, ce qui rend
			// le repertoire temporairement inactif vis a vis des autres applications
			DeleteTmpDirectory(sTestedDirectoryName);

			// Remise a zero du compteur d'essai depuis dernier nettoyage
			nOtherTrial = 0;
		}

		// Construction d'un nouveau nom de repertoire
		sTestedDirectoryName = BuildFilePathName(
		    sPathName, BuildFileName(sDirectoryPrefix + "_" + IntToString(nId), sDirectorySuffix));
	}
}

ALString FileService::GetAnchorFileName()
{
	ALString sFileName;
	sFileName += GetTmpPrefix();
	sFileName += GetTmpPrefix();
	sFileName += "anchor";
	sFileName += GetTmpPrefix();
	sFileName += GetTmpPrefix();
	return sFileName;
}

ALString FileService::sUserTmpDir;
ALString FileService::sApplicationName = "Default";
ALString FileService::sApplicationTmpDir;
int FileService::nApplicationTmpDirFreshness = 1;
int FileService::nApplicationTmpDirCreationFreshness = 0;
boolean FileService::bApplicationTmpDirAutoDeletion = true;
boolean FileService::bApplicationTmpDirAutomaticRemove = false;
boolean FileService::bURISmartLabels = true;
ALString FileService::sHDFStmpDir;

////////////////////////////////////////
// Implementation de la classe FileSpec

FileSpec::FileSpec() {}

FileSpec::~FileSpec() {}

void FileSpec::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString& FileSpec::GetLabel() const
{
	return sLabel;
}

void FileSpec::SetFilePathName(const ALString& sValue)
{
	sFilePathName = sValue;
}

const ALString& FileSpec::GetFilePathName() const
{
	return sFilePathName;
}

boolean FileSpec::CheckSimplePath() const
{
	boolean bOk = true;

	require(GetLabel() != "");

	if (GetFilePathName() != "" and FileService::IsPathInFilePath(GetFilePathName()))
	{
		Global::AddError("", "",
				 "The name of the " + GetLabel() + " should not be a path (" + GetFilePathName() + ")");
		bOk = false;
	}
	return bOk;
}

boolean FileSpec::CheckReferenceFileSpec(const FileSpec* refFileSpec) const
{
	boolean bOk = true;
	ALString sPathName;
	ALString sFileName;
	ALString sRefPathName;
	ALString sRefFileName;

	require(refFileSpec != NULL);
	require(GetLabel() != "");
	require(refFileSpec->GetLabel() != "");

	// Test de difference si necessaire
	if (GetFilePathName() != "")
	{
		// Verification de l'egalite des chemins de facon recursive
		sPathName = GetFilePathName();
		sRefPathName = refFileSpec->GetFilePathName();

		if (sPathName.GetLength() == sRefPathName.GetLength() and
		    FileService::GetURIScheme(sPathName) == FileService::GetURIScheme(sRefPathName) and
		    FileService::GetURIHostName(sPathName) == FileService::GetURIHostName(sRefPathName))
		{
			// On enleve la partie URI des chemins (file://hostname/)
			sPathName = FileService::GetURIFilePathName(sPathName);
			sRefPathName = FileService::GetURIFilePathName(sRefPathName);
			bOk = false;
			while (sPathName != "")
			{
				// Extraction des noms de fichier
				sFileName = FileService::GetFileName(sPathName);
				sRefFileName = FileService::GetFileName(sRefPathName);

				// Test d'egalite des noms de fichier, independant de la casse
				// OK si au moins une difference d'un element de chemin
				if (sFileName.CompareNoCase(sRefFileName) != 0)
				{
					bOk = true;
					break;
				}

				// Passage au niveau de repertoire precedent pour remonter au niveau precedent
				sPathName = FileService::GetPathName(sPathName);
				sRefPathName = FileService::GetPathName(sRefPathName);

				// On supprime l'eventuel dernier caractere de fin de chemin
				if (sPathName != "")
				{
					assert(FileService::IsFileSeparator(sPathName[sPathName.GetLength() - 1]));
					sPathName = sPathName.Left(sPathName.GetLength() - 1);
				}
				if (sRefPathName != "")
				{
					assert(
					    FileService::IsFileSeparator(sRefPathName[sRefPathName.GetLength() - 1]));
					sRefPathName = sRefPathName.Left(sRefPathName.GetLength() - 1);
				}
			}
		}

		// Message d'erreur si necessaire
		if (not bOk)
			Global::AddError("", "",
					 "The path of the " + GetLabel() + " should be different from that of the " +
					     refFileSpec->GetLabel() + " (" + refFileSpec->GetFilePathName() + ")");
	}
	return bOk;
}

void FileSpec::Write(ostream& ost) const
{
	ost << GetLabel() << ": " << GetFilePathName() << "\n";
}

const ALString FileSpec::GetClassLabel() const
{
	return "File";
}

const ALString FileSpec::GetObjectLabel() const
{
	return GetLabel() + " " + GetFilePathName();
}

boolean FileSpec::Test()
{
	FileSpec spec1;
	FileSpec spec2;
	boolean bTest;
	boolean bOk = true;

	const ALString sPathName1 = "/user/toto/LearningTest/TestKhiops/Standard/IrisTransfer/results/T_Iris.txt";
	const ALString sPathName2 = "/user/titi/LearningTest/TestKhiops/Standard/IrisTransfer/results/T_Iris.txt";
	const ALString sScheme1 = "foo";
	const ALString sScheme2 = "bar";
	const ALString host1 = "host1";
	const ALString host2 = "host2";

	spec1.SetLabel("std");
	spec2.SetLabel("std");

	// Fichiers std identiques
	spec1.SetFilePathName(sPathName1);
	spec2.SetFilePathName(sPathName1);
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = not bTest and bOk;
	ensure(bOk);

	// Fichiers std differents (mais longueur du path identique)
	spec1.SetFilePathName(sPathName1);
	spec2.SetFilePathName(sPathName2);
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = bTest and bOk;
	ensure(bOk);

	// URI identiques
	spec1.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName1));
	spec2.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName1));
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = not bTest and bOk;
	ensure(bOk);

	// URI schemas differents (mais longueur du path identique)
	spec1.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName1));
	spec2.SetFilePathName(FileService::BuildURI(sScheme2, host1, sPathName1));
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = bTest and bOk;
	ensure(bOk);

	// URI hosts differents (mais longueur du path identique)
	spec1.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName1));
	spec2.SetFilePathName(FileService::BuildURI(sScheme1, host2, sPathName1));
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = bTest and bOk;
	ensure(bOk);

	// URI nom de fichiers differents (mais longueur du path identique)
	spec1.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName1));
	spec2.SetFilePathName(FileService::BuildURI(sScheme1, host1, sPathName2));
	bTest = spec2.CheckReferenceFileSpec(&spec1);
	bOk = bTest and bOk;
	ensure(bOk);

	return bOk;
}
