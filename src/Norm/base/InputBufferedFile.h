// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "BufferedFile.h"
#include "Timer.h"
#include "Vector.h"
#include "PLRemoteFileService.h"
#include "MemoryStatsManager.h"

class InputBufferedFile;

///////////////////////////////////////////////////////////////////////////
// Fichier bufferise en lecture
// TODO dans chaque methode qui fait avancer la position courante, il faut tester
// si on n'a pas une ligne trop longue et l'indiquer a l'utilisateur
// Il ya au moins les methode suivantes :
// 			boolean GetNextField(char* sField, int& nFieldError); // nouvelle valeur de nFieldError quand la
// ligne est trop longue 			boolean SkipField(); 			void GetNextLine(CharVector*
// cvLine); renvoie vide si la ligne est trops longue 			void SkipLine(); revoie false si la ligne est
// trop longue Il faut memoriser la position du debut de la ligne courante et  comparer nCurrentPos avec le debut de la
// ligne pour evaluer si la ligne en cours est trop longue
class InputBufferedFile : public BufferedFile
{
public:
	// Constructeur
	InputBufferedFile();
	~InputBufferedFile();

	// Parametrage du nom du fichier
	// URI: le chemin du fichier peut etre une URI
	void SetFileName(const ALString& sValue) override;

	// Ouverture en mode read
	boolean Open() override;
	boolean IsOpened() const override;

	// Fermeture du fichier
	boolean Close() override;

	// Copie des specifications (hors fichier et buffer)
	void CopyFrom(const InputBufferedFile* bufferedFile);

	// Taille du fichier
	// Methode utilisable uniquement si le fichier est ouvert
	// Pour obtenir la taille d'un fichier non ouvert, passer par FileService ou PLRemoteFileService
	// Renvoie 0 si probleme d'acces au fichier
	longint GetFileSize() const;

	// Taille du buffer
	void SetBufferSize(int nValue) override;

	/////////////////////////////////////////////////////////////////////////////////
	// Remplissage du buffer

	// Remplit le buffer avec des lignes entieres a partir de lBeginPos jusqu'a lBeginPos + nBufferSize
	// Plus precisement : le buffer est rempli a partir du debut de la premiere ligne
	// situe apres lBeginPos, jusqu'a la premiere fin de ligne apres lBeginPos + nBufferSize.
	// La taille du buffer poura etre plus grande que la nBufferSize, au maximum
	// elle sera de nBufferSize +  GetMaxLineLength -1.
	// bSkippedline est mis a true si la derniere ligne du buffer est trop longue : i.e. si
	// le buffer contient un debut de ligne (ou le debut de fichier) et pas de fin de ligne.
	// (Si la premiere est trop longue, le buffer est vide)
	// Renvoie false en cas d'erreur
	// Les cas suivants sont possibles :
	//	- lBeginPos==0 le debut de ligne est 0
	//	- lBeginPos est plus grand que la taille du fichier : le buffer est vide
	//	- le debut de ligne n'est pas trouve : le buffer est vide et GetBufferSkippedLine renvoie false
	//	- la fin de ligne n'est pas trouvee : le buffer est vide et GetBufferSkippedLine renvoie true
	boolean Fill(longint lBeginPos);

	// Taille maximale du buffer dans la methode Fill (2 Go -1 - GetMaxLineLength)
	static int GetMaxBufferSize();

	// Taille minimale du buffer dans la methode Fill (1 bloc : 64 ko)
	static int GetMinBufferSize();

	// Methode de service, ajustant une taille demandee en fonction des contrainte de taille min et max gerees par
	// les buffer Entre GetMinBufferSize et GetMaxBufferSize
	static int FitBufferSize(longint lExpectedSize);

	// Position du debut de la premiere ligne du buffer dans le fichier
	longint GetBufferStartFilePos() const;

	// Retourne true si un ligne a ete skipee
	boolean GetBufferSkippedLine() const;

	// Exemple de lecture d'un fichier
	//
	// int nChunkSize = 8 * lMB;
	// CharVector cvLine;
	//
	// bOk = ibFile.Open();
	// if (bOk)
	// {
	// 		lFilePos = 0;
	// 		while (lFilePos < ibFile.GetFileSize())
	// 		{
	// 			// Remplissage du buffer
	// 			ibFile.SetBufferSize(nChunkSize); // nChunkSize Peut etre modifie pendant la boucle
	// 			bOk = ibFile.Fill(lFilePos);
	// 			if (not bOk)
	// 			{
	// 				ibFile.AddError("Error while reading file");
	// 				break;
	// 			}
	//
	// 			// Comptage des lignes
	// 			nLineNumber += ibFile.GetBufferLineNumber();
	// 			if (ibFile.GetBufferSkippedLine())
	// 			{
	// 				ibFile.AddWarning(sTmp + "line " + IntToString(nLineNumber + 1) + " is
	// skipped"); 				nLineNumber++;
	// 			}
	//
	// 			// Traitement du buffer
	// 			while (not ibFile.IsBufferEnd())
	// 			{
	// 				ibFile.GetNextLine(&cvLine);
	// 				DoSomething(cvLine);
	// 			}
	// 			lFilePos += ibFile.GetBufferSize();
	// 		}
	// 		ibFile.Close();
	// }

	// Recherche la position de prochaine fin de ligne a partir de dBeginPos (hors gestion du buffer)
	// Ne modifie aucun attribut
	virtual longint FindEolPosition(longint lBeginPos, boolean& bEolFound);

	// Indique que l'on a rempli le dernier buffer du fichier
	boolean IsLastBuffer() const;

	// Remplissage efficace du buffer sans tenir compte des debuts et fins de ligne
	// Le remplissage est plus efficace en revanche les methodes de InputBufferedFile sont a utiliser
	// avec precaution (Ne remplit pas le buffer avec des lignes entieres)
	boolean BasicFill(longint lBeginPos);

	/////////////////////////////////////////////////////////////////////////////////
	// Lecture du buffer
	// Un champ d'un fichier est compris entre deux delimiteurs
	//
	// Lorsqu'un champ debute par un double-quote, il doit terminer par un double-quote
	// Il peut alors contenir un delimiteur de champs (mais pas de multi-ligne), ou des
	// double-quotes s'ils sont doubles
	//
	// Une fois extrait, les caracteres d'espaces sont supprimes en debut et fin de champ
	//
	// Ensuite, les tabulations sont remplaces par des espaces
	// Permet d'eviter les tabulations dans les noms de variables et les valeurs categorielles,
	// qui posent probleme pour les saisies a l'interface et les rapports de sortie (tabules)

	// Type d'erreur liees au parsing d'un champs
	// En cas d'erreur, le parsing continue pour rattrapper l'erreur, au mieux en fin de champs, au pire en fin de
	// ligne
	enum
	{
		FieldNoError,           // Pas d'erreur
		FieldTabReplaced,       // Tabulations remplacees par des blancs
		FieldCtrlZReplaced,     // Ctrl-Z (ascii 26) remplaces par des blancs
		FieldMiddleDoubleQuote, // Double-quote non double au milieu d'un champ commencant par un double-quote
		FieldMissingEndDoubleQuote, // Manque un double-quote en fin d'un champ commencant par un double-quote
		FieldTooLong                // Champ trop long (le champ sera tronque)
	};

	// Libelle d'erreur associe a un type d'erreur
	static const ALString GetFieldErrorLabel(int nFieldError);

	// Lecture du prochain champ d'une ligne, qui est rendu nettoye des blancs de debut et de fin
	// La chaine en parametre contient en retour le contenu du champ, termine par le caractere '\0'.
	// Le parametre sField est a declarer en variable locale dans la methode appelante.
	// Ce pointeur est positionne en retour vers un buffer gere par InputBufferFile, ce buffer pouvant
	// etre potentiellement de tres grande taille (cf. nMaxFieldSize). Attention a exploiter
	// ce buffer localement a la methode, si necessaire a en recopier le contenu,
	// mais a ne pas memoriser son pointeur, dont le contenu change apres chaque appel.
	// Le flag nFieldError est positionne en cas d'erreur de parsing du champ
	// Code retour a true si le token est le dernier de la ligne, du buffer ou du fichier
	boolean GetNextField(char*& sField, int& nFieldError);

	// Saut d'un champ
	// Code retour a true si le token est le dernier de la ligne, du buffer ou du fichier
	boolean SkipField();

	// Lecture de la prochaine ligne a partir de la position courante
	void GetNextLine(CharVector* cvLine);

	// Saut d'une ligne
	void SkipLine();

	// Extrait une sous-chaine allant de nBeginPos (inclus) a nEndPos (exclu) dans le buffer
	// Fonctonnalite avancee : ne modifie pas les attributs de la classe
	void ExtractSubBuffer(int nBeginPos, int nEndPos, CharVector* cvSubBuffer) const;

	// On a lu le dernier caractere du buffer
	boolean IsBufferEnd() const;

	// On a lu le dernier caractere du fichier
	// Precisement, on a lu le dernier caractere du derniere buffer
	boolean IsFileEnd() const;

	// Renvoie le nombre de lignes parcourues pendant la lecture du buffer
	int GetCurrentLineNumber() const;

	// Renvoie le nombre de lignes contenues dans le buffer
	int GetBufferLineNumber() const;

	// Taille maximum des champs
	static const unsigned int nMaxFieldSize = 1000000;

	// Taille du buffer de lecture
	static const unsigned int nHugeBufferSize = 1048576; // 1 Mo

	/////////////////////////////////////////////////////////////
	// Information de taille et position, permettant de gerer
	// une progression dans le traitement d'un buffer ou du fichier

	// Position du curseur de lecture dans le buffer
	int GetPositionInBuffer() const;

	// Position du curseur de lecture dans le fichier
	longint GetPositionInFile() const;

	// Verification d'un encodage gerable par la classe
	// Notamment, l'absence d'un BOM en tete de fichier UTF8, UTF16, ou de caracteres null '\0' dans le buffer en
	// cours est verifie Les messages d'erreur sont emis par le errorSender (pas de message si NULL)
	boolean CheckEncoding(const Object* errorSender) const;

	// Taille max des ligne : par defaut 8 MB
	// Si une ligne ne tient pas dans le buffer, la taille du buffer sera automatiquement
	// et temporairement etendue jusqu'a cette taille pour recevoir la ligne en entier.
	// N'est pris en compte que si la taille du buffer est plus petite que la taille max des lignes
	// Les lignes depassant la taille max sont ignoree lors de la lecture (avec warning)
	static int GetMaxLineLength();
	static void SetMaxLineLength(int nValue);

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Extraction des champs de la premiere ligne d'un fichier passe en parametre
	// Les controles de validite avec emission de message d'erreur sont effectues
	// par la classe passee en parametre (pas de message si NULL)
	static boolean GetFirstLineFields(const ALString& sInputFileName, char cInputFieldSeparator,
					  StringVector* svFirstLineFields, const Object* errorSender);

	// Format d'affichage d'une valeur, troncature avec ajout de .. si trop longue
	static ALString GetDisplayValue(const ALString& sValue);

	// Methode de test
	// Compte le nombre de lignes du fichier passe en parametre
	// Fait varier la taille du buffer de 128Mo a 1 Mo
	// Si nFileType=1  le nom du fichier est precede par file://hostname/
	// Si nFileType=2  le nom du fichier est precede par hdfs://hostname/
	static boolean TestCountLines(const ALString& sFileName, int nFileType);

	// Methode de test
	// Generation automatique de plusieurs fichiers ayant des caracteristiques
	// differentes (nombre de lignes, taille...).
	// Puis appel de la methode TestCountLines sur chacun de ces fichiers.
	// si nFileType =0, test les fichiers standards
	// si nFileType=1, test les fichiers distant, dans ce cas, le nom des
	//     fichiers generes est prefixe par file://hostname/
	// si nFileType=2, test des fichiers HDFS, dans ce cas les fichiers doivent etre
	//    deja present sur hdfs
	//
	// Pour tester les fichiers distants on doit avoir les instructions suivantes
	// 			PLTaskDriver::SetFileServerOnSingleHost(true);
	//			PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
	//			PLParallelTask::GetDriver()->StartFileServers();
	//			InputBufferedFile::Test(1);
	//			PLParallelTask::GetDriver()->StopFileServers();
	//
	// Pour tester les fichiers sur HDFS, on doit avoir les instructions ssuivantes
	//			SystemFileDriverCreator::SetDriverHDFS(new HDFSFileDriver);
	//			InputBufferedFile::Test(2);
	//			if (HDFSFileSystem::IsConnected())
	// 				HDFSFileSystem::Disconnect();
	//			delete SystemFileDriverCreator::GetDriverHDFS();
	static boolean Test(int nFileType);

	// Compte le nombre de lignes du fichier passe en parametre
	// La lecture est effectuee par bloc de taille fixe : nChunkSize
	static boolean TestCount(const ALString& sFileName, int nChunkSize);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

protected:
	// Remplissage avec ecrasement de la premiere ligne du fichier
	// Renvoi false si la premiere ligne est trop longue
	boolean FillWithHeaderLine();

	// Variantes d'analyse et de saut des champs, avec gestion d'un double-quote en debut de champ
	// Ces methodes sont appelees si un double-quote a ete detecte en debut de champ
	// Le methode ne gere que les erreurs liees au double-quote.
	// Les champs trop longs sont tronques, mais le message est a emettre par la methode appelante
	// (en testant la valeur de i, position du prochain caractere)
	boolean GetNextDoubleQuoteField(char* sField, int& i, int& nFieldError);
	boolean SkipDoubleQuoteField();

	// Methode pour le test
	// Ecriture d'un fichier, evaluation de son contenu avec TestCountLines, nettoyage du buffer et destruction du
	// fichier
	static boolean TestWriteBuffer(const ALString& sTmpDir, const ALString& sFileName, const ALString& sLabel,
				       FileBuffer* fileBuffer, int nFileType);

	// Methode pour le test: ajout de caracteres dans un buffer de fichier
	static void AddCharsInFileBuffer(FileBuffer* fileBuffer, char c, int nNumber);

	// Prochain caractere
	char GetNextChar();

	// Reinitialisation des donnees de travail
	void Reset();

	// Methode appelee dans la methode Fill lorsque le fichier est local
	boolean FillLocal(longint lBeginPos);

	// Methode technique
	// Recherche du premier debut de ligne situe apres lBeginPos juqu'a lBeginPos + nChunkSize
	// Met a jour les parametres bBolFound et nBeginLinePos
	// Renvoie false en cas d'erreur
	static boolean FindBol(longint lBeginPos, int nChunkSize, longint lFileSize, SystemFile* fileHandle,
			       int& nBeginLinePos, boolean& bBolFound);

	// Taille a allouer au buffer (peut etre differente de nBufferSize)
	int GetAllocatedBufferSize() const override;
	void SetAllocatedBufferSize(int nValue) override;

	///////////////////////////////////////////////////////////////////////////////
	// Declaration des variables d'instance par taille decroissante pour optimiser
	// la taille memoire de l'objet (cf. alignement des variables pour l'OS)

	// Prochaine position dans le fichier
	longint lNextFilePos;

	// Taille du fichier
	longint lFileSize;

	// Position du debut du buffer dans le fichier pendant le remplissage
	longint lBufferBeginPos;

	// Position a partir de laquelle il n'y a pas de BOL jusqu'a lNextFilePos
	// Pour l'optimisation de la methode FillLocal
	longint lLastEndBufferPos;

	// Position du curseur de lecture dans le buffer
	int nPositionInBuffer;

	// Nombre de lignes parcourues dans le buffer
	int nReadLineNumber;

	// Nombre de lignes dans le buffer, calcule apres le remplissage du buffer
	mutable int nBufferLineNumber;

	// Taille a allouer (peut etre different de nBufferSize)
	int nAllocatedBufferSize;

	// Indicateur de saut de ligne
	boolean bIsSkippedLine;

	// Fin de fichier en remplissant le buffer
	boolean bEof;

	// Debut du fichier
	boolean bIsHeadOfFile;

	// Est-ce que le dernier champ lu etait a la fin d'une ligne
	boolean bLastFieldReachEol;

	// Taile max de lignes
	static int nMaxLineLength;

	// Classes friend pour permettre a la librairie Parallel de gerer les fichiers distants
	friend class PLMPIFileServerSlave;       // Serialisation des attributs InputBuffer pour les servers de fichiers
						 // (methode GetBuffer())
	friend class PLBufferedFileDriverRemote; // Lecture directe du buffer
	friend class PLFileConcatenater;         // Ecriture dans HDFS et lecture du buffer
};

///////////////////////
/// Methodes en inline

inline int InputBufferedFile::GetBufferLineNumber() const
{
	require(bIsOpened);

	if (nBufferLineNumber == 0)
	{
		// Calcul du nombre de lignes
		nBufferLineNumber = fbBuffer.ComputeLineNumber(nCurrentBufferSize);

		// Cas particulier du fichier qui ne contient qu'une ligne sans eol
		if (not bIsSkippedLine and nBufferLineNumber == 0 and nCurrentBufferSize > 0)
			nBufferLineNumber = 1;
	}
	return nBufferLineNumber;
}

inline int InputBufferedFile::GetPositionInBuffer() const
{
	return nPositionInBuffer;
}

inline longint InputBufferedFile::GetPositionInFile() const
{
	assert(lBufferBeginPos != -1);
	return lBufferBeginPos + nPositionInBuffer;
}

inline longint InputBufferedFile::GetFileSize() const
{
	require(bIsOpened);
	assert(lFileSize != -1);
	return lFileSize;
}

inline boolean InputBufferedFile::IsFileEnd() const
{
	return GetPositionInFile() >= GetFileSize();
}

inline int InputBufferedFile::GetCurrentLineNumber() const
{
	ensure(0 <= nReadLineNumber and nReadLineNumber <= GetBufferLineNumber());
	return nReadLineNumber;
}

inline char InputBufferedFile::GetNextChar()
{
	char c;
	require(nPositionInBuffer < nCurrentBufferSize);
	c = fbBuffer.GetAt(nPositionInBuffer);
	nPositionInBuffer++;
	return c;
}

inline boolean InputBufferedFile::IsBufferEnd() const
{
	return nPositionInBuffer == nCurrentBufferSize;
}

inline void InputBufferedFile::SkipLine()
{
	while (not IsBufferEnd())
	{
		if (GetNextChar() == '\n')
		{
			nReadLineNumber++;
			return;
		}
	}
}

inline int InputBufferedFile::GetMaxLineLength()
{
	return nMaxLineLength;
}

inline void InputBufferedFile::SetMaxLineLength(int nValue)
{
	require(nValue > 0);
	nMaxLineLength = nValue;
}

inline boolean InputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

inline int InputBufferedFile::GetMaxBufferSize()
{
	return 2 * lGB - 1 - GetMaxLineLength();
}

inline int InputBufferedFile::GetMinBufferSize()
{
	return InternalGetBlockSize();
}

inline int InputBufferedFile::FitBufferSize(longint lExpectedSize)
{
	if (lExpectedSize > GetMaxBufferSize())
		return GetMaxBufferSize();
	if (lExpectedSize < GetMinBufferSize())
		return GetMinBufferSize();
	return int(lExpectedSize);
}

inline longint InputBufferedFile::GetBufferStartFilePos() const
{
	return lBufferBeginPos;
}

inline boolean InputBufferedFile::GetBufferSkippedLine() const
{
	return bIsSkippedLine;
}

inline int InputBufferedFile::GetAllocatedBufferSize() const
{
	return nAllocatedBufferSize;
}

inline void InputBufferedFile::SetAllocatedBufferSize(int nValue)
{
	nAllocatedBufferSize = nValue;
}

inline boolean InputBufferedFile::IsLastBuffer() const
{
	return bEof;
}