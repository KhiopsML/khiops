// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "FileService.h"

#include "OutputBufferedFile.h"
#include "InputBufferedFile.h"
#include "SystemFileDriver.h"
#include "HugeBuffer.h"

//////////////////////////////////////////////////////////////////////////////
// classe PLRemoteFileService
// Methodes utilitaires de gestion des fichiers non standard (HDFS, distants)
// Le createur de driver est celui de la classe InputBufferedFile
// Les types d'URI geres sont les memes que ceux geres InputBufferedFile
class PLRemoteFileService : public Object
{
public:
	// Test d'existence d'un fichier
	static boolean FileExists(const ALString& sFileURI);

	// Test d'existence d'un repertoire
	static boolean DirExists(const ALString& sFileURI);

	// Taille d'un fichiers en bytes
	// Renvoie 0 si probleme d'acces au fichier
	static longint GetFileSize(const ALString& sFileURI);

	// Creation d'un fichier vide, ecrasement eventuellement si fichier existant
	// Pas d'emission de message d'erreur
	static boolean CreateEmptyFile(const ALString& sFileURI);

	// Supprime le fichier
	static boolean RemoveFile(const ALString& sFileURI);

	// Copie le fichier distant/local/hdfs vers local/hdfs
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

	// Renvoie la taille de buffer a privilegier lors des lectures/ecriture pour le fichier
	// dont l'URI est passee en parametre.
	// Renvoie FileSystem::nDefaultPreferredBufferSize (8 Mo) si aucune taille n'est a privilegiee ou si le driver
	// necessaire pour acceder au fichier n'est pas enregistre. Quelle que soit la taille specifiee dans la driver,
	// la valeur renvoyee est comprise entre SystemFile::nMinPreferredBufferSize (1 Mo) et
	// SystemFile::nMaxPreferredBufferSize (64 Mo)
	static int GetPreferredBufferSize(const ALString& sURI);

	// Creation d'un nom de fichier temporaire en ecriture si necessaire,
	// dans le cas ou le fichier est sur un systeme de fichier non standard (ex/ HDFS)
	// Dans le Clean, le WorkingFileName est remis a vide
	static boolean BuildOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);
	static boolean CleanOutputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);

	// Creation d'un nom de fichier temporaire en en lecture si necessaire,
	static boolean BuildInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);
	static void CleanInputWorkingFile(const ALString& sPathName, ALString& sWorkingFileName);

	// Renvoie true si l'URI commence par file:// mais qu'on doit la traiter
	// comme un chemin local en extrayant le chemin du fichier de l'URI
	static boolean RemoteIsLocal(const ALString& sURI);

	// Renvoie true si les fichiers REMOTE dont le host est localhost ne sont pas consideres comme "local"
	static void SetRemoteIsNeverLocal(boolean bValue);
	static boolean GetRemoteIsNeverLocal();

	// Renvoie true si les deux fichiers sont strictement identiques
	static boolean FileCompare(const ALString& sFileName1, const ALString& sFileName2);

protected:
	// Copie de fichier en utilisant les drivers de fichiers
	static boolean CopyFileGeneric(const ALString& sSourceURI, const ALString& sDestPath);

	// Index des noms de fichiers temporaires HDFS
	// Utiliser pour evtiter d'avoir 2 fois le meme nom de fichier. C'est problematique car HDFS genere un fichier
	// CRC a partir du non de fichier initial. Ce fichier CRC n'est pas automatiquement efface par HDFS. Par contre
	// on ne peut pas creer un fichier dont le CRC existe deja. i.e. si on cree A.txt, on aura A.txt.crc, si on
	// supprime A.txt, ca ne supprime pas A.txt.crc. Et si on cree a nouveau A.txt, HDFS ne pourra pas cree
	// A.txt.crc car il existe deja et il y aura un bug
	static int nFileHdfsIndex;

	// Lorsque ce boolean est a true, les fichiers REMOTE dont le host est localhost ne sont pas consideres comme
	// "local" Cf. methode IsLocal()
	static boolean bRemoteIsNeverLocal;
};
