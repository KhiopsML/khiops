// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataTableDriverTextFile.h"

///////////////////////////////////////////////////////////////////
// Classe PLDataTableDriverTextFile
//  Driver associe a la classe PLSTDatabaseTextFile dedie aux applications paralleles.
//  Dans le cadre des applications paralleles, les buffers sont geres en dehors de la classe.
//	Il y a donc des getters pour les buffers d'entree et de sortie.
//  Les methode de KWDataTableDriverTextFile sont reimplementees en enlevant les
//  ouverture et fermeture de buffer.
//  De plus le calcul des index n'est pas realise dans le OpenForRead, il faut l'appeler explicitement.
//  La methode ComputeDataItemLoadIndexes permet de calculer les index (a faire dans le Master)
//  Les methodes GetConstDataItemLoadIndexes et GetDataItemLoadIndexes permettent de transmettre
//  l'index entre le maitre et les esclaves
//  Dans le cas multi-table, les methodes GetConstRootKeyIndexes et GetRootKeyIndexes permettent
//  de transmettre les index des attributs de la cle en cas de classe racine
//  Enfin, chaque process gere une portion de fichier comprise entre une position de debut et de fin,
//  qui peut potentiellement necessiter la lecture de plusieurs buffers pour etre traitee.
class PLDataTableDriverTextFile : public KWDataTableDriverTextFile
{
public:
	// Constructeur
	PLDataTableDriverTextFile();
	~PLDataTableDriverTextFile();

	/////////////////////////////////////////////////////////////////////
	// Parametrage de la portion du fichier a traiter

	// Parametrage de la position de depart pour la lecture (comprise)
	void SetBeginPosition(longint lValue);
	longint GetBeginPosition() const;

	// Parametrage de la position d'arrivee pour la lecture (non comprise)
	void SetEndPosition(longint lValue);
	longint GetEndPosition() const;

	// Parametrage de l'index de l'enregistrement traite (pour une gestion correcte des erreurs)
	void SetRecordIndex(longint lValue);

	//////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de KWDataTableDriver
	KWDataTableDriver* Create() const override;
	boolean OpenForRead(const KWClass* kwcLogicalClass) override;
	boolean OpenForWrite() override;
	boolean IsOpenedForRead() const override;
	boolean IsOpenedForWrite() const override;
	longint GetEstimatedObjectNumber() override;
	boolean BuildDataTableClass(KWClass* kwcDataTableClass) override;
	boolean IsEnd() const override;
	boolean Close() override;
	boolean OpenInputDatabaseFile() override;

	// Lecture du premier buffer a partir de la position de depart
	boolean FillFirstInputBuffer();

	// Reimplementation des methodes virtuelles de KWDataTableDriverTextFile
	// On la passe en public pour pouvoir l'utiliser directement dans les taches paralleles
	boolean UpdateInputBuffer() override;
	double GetReadPercentage() override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	friend class PLSTDatabaseTextFile;
	friend class PLMTDatabaseTextFile;
	friend class PLShared_MTDatabaseTextFile;

	// Parametrage du buffer d'entree
	void SetInputBuffer(InputBufferedFile* buffer);
	InputBufferedFile* GetInputBuffer();

	// Parametrage du buffer d'entree
	void SetOutputBuffer(OutputBufferedFile* buffer);
	OutputBufferedFile* GetOutputBuffer();

	/////////////////////////////////////////////////////////////////////
	// Gestion des index des data items (attributs ou blocs d'attributs)

	// Re-immplementation du calcul des index a partir du dictionaire
	boolean ComputeDataItemLoadIndexes(const KWClass* kwcLogicalClass, const KWClass* kwcHeaderLineClass) override;

	// Acces en lecture et ecriture aux index des data items
	const KWLoadIndexVector* GetConstDataItemLoadIndexes() const;
	KWLoadIndexVector* GetDataItemLoadIndexes();

	// Acces en lecture et ecriture aux index des attributs de la cle en cas de classe racine
	const IntVector* GetConstRootKeyIndexes() const;
	IntVector* GetRootKeyIndexes();

	// Initialisation de la taille de la derniere cle lu
	void InitializeLastReadKeySize(int nValue);

	// Position de debut et de fin dans le fichier
	longint lBeginPosition;
	longint lEndPosition;

	// Indicateur d'ouverture
	boolean bIsOpened;
};

////////////////////////////////////////
// Methode en inline

inline void PLDataTableDriverTextFile::SetBeginPosition(longint lValue)
{
	require(lValue >= 0);
	lBeginPosition = lValue;
}

inline longint PLDataTableDriverTextFile::GetBeginPosition() const
{
	return lBeginPosition;
}

inline void PLDataTableDriverTextFile::SetEndPosition(longint lValue)
{
	require(lValue >= 0);
	lEndPosition = lValue;
}

inline longint PLDataTableDriverTextFile::GetEndPosition() const
{
	return lEndPosition;
}

inline boolean PLDataTableDriverTextFile::IsEnd() const
{
	return (inputBuffer->GetPositionInFile() >= GetEndPosition());
}
