// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "FileService.h"
#include "BufferedFileDriver.h"
#include "OutputBufferedFile.h"
#include "InputBufferedFile.h"
#include "SystemFileDriver.h"

//////////////////////////////////////////////////////////////////////////////
// classe PLRemoteFileService
// Methodes utilitaires de gestion des fichiers non standard (HDFS, distants)
// Le createur de driver est celui de la classe InputBufferedFile
// Les types d'URI geres sont les memes que ceux geres InputBufferedFile
class PLRemoteFileService : public Object
{
public:
	// Test d'existence d'un fichier (ou d'un directory)
	static boolean Exist(const ALString& sFileURI);

	// Taille d'un fichiers en bytes
	// Renvoie 0 si probleme d'acces au fichier
	static longint GetFileSize(const ALString& sFileURI);

	// Creation d'un fichier vide, ecrasement eventuellement si fichier existant
	// Pas d'emission de message d'erreur
	static boolean CreateEmptyFile(const ALString& sFileURI);

	// Supprime le fichier
	static boolean RemoveFile(const ALString& sFileURI);

	// Copie le fichier distant/local/hdfs vers local/hdfs
	// TODO a tester en remote
	static boolean CopyFile(const ALString& sSourceURI, const ALString& sDestPath);

	// Creation d'un repertoire
	// Indique en sortie si le repertoire existe, sans message d'erreur
	static boolean MakeDirectory(const ALString& sPathName);

	// Creation d'un repertoire et si necessaire de tous les repertoire intermediaires
	static boolean MakeDirectories(const ALString& sPathName);

	// Recherche de la memoire disque disponible sur un repertoire
	// (se base sur le repertoire courant "." si le repertoire en parametre est "")
	// Renvoie 0 si erreur ou pas de place disponible
	static longint GetDiskFreeSpace(const ALString& sFileURI);

	// Renvoie l'URI telle quelle sur un cluster
	// Renvoie le nom du fichier sur une machine
	// TODO a verifier Cf. SmartLabel de FileService
	static const ALString URItoUserString(const ALString& sFileURI);

	// Creation d'un nom de fichier temporaire en ecriture si necessaire,
	// dans le cas ou le fichier est sur un systeme de fichier non standard (ex/ HDFS)
	// Dans le Clean, le WorkingFileName est remis a vide
	static boolean BuildOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);
	static boolean CleanOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);

	// Creation d'un nom de fichier temporaire en en lecture si necessaire,
	static boolean BuildInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);
	static void CleanInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);

	// Methode de test
	static boolean TestCount(const ALString& sFileURI, int nBufferSize);

protected:
	// Copie le fichier local/hdfs vers local/hdfs
	static boolean CopyFileLocal(const ALString& sSourceURI, const ALString& sDestPath);

	// Numero des noms de fichiers temporaires HDFS
	// TODO BG: mieux commenter
	static int nFileHdfsIndex;

	friend class PLBufferedFileDriverRemote; // Acces a CopyFileLocal
};
