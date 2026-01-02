// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHBinMerge.h"
#include "FileService.h"
#include "SortedList.h"

////////////////////////////////////////////////////////////////
// Classe MHStreamBining
// Gestion de la binarisation d'un ensemble de valeurs
// avec des bin orientes largeur egale ou virgule flotante
class MHStreamBining : public Object
{
public:
	// Constructeur
	MHStreamBining();
	~MHStreamBining();

	// Parametrage du nombre max de bins
	void SetMaxBinNumber(int nValue);
	int GetMaxBinNumber() const;

	// Parametrage de grille de type virgule flotante pour traiter la fusion des bins adjacents (defaut: false)
	// Sinon, les bins sont orientes largeur egale
	void SetFloatingPointGrid(boolean bValue);
	boolean GetFloatingPointGrid() const;

	// Parametrage du type de bins: sature ou non
	void SetSaturatedBins(boolean bValue);
	boolean GetSaturatedBins() const;

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion en flux de la binarisation d'un ensemble de valeurs

	// Initialisation de la gestion du flux
	virtual void InitializeStream();

	// Indique si le flux est initialise
	virtual boolean IsStreamInitialized() const;

	// Ajout d'une valeur
	virtual void AddStreamValue(Continuous cValue);

	// Finalisation de la gestion du flux
	virtual void FinalizeStream();

	/////////////////////////////////////////////////////////////////////////////////
	// Exploitation d'un resume en cours

	// Valeur inf globale
	virtual Continuous GetStreamLowerValue() const;

	// Valeur sup globale
	virtual Continuous GetStreamUpperValue() const;

	// Effectif  global
	virtual int GetStreamFrequency() const;

	// Export des bins du resume dans un tableau
	// Les bins du resume en cours sont post-traites et exportes dans un tableau
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	virtual void ExportStreamBins(ObjectArray* oaStreamBins) const;

	// Ecriture dans un fichier des bins construits
	// Cette methode est appelable a tout moment, entre l'initialisation et la finalisation
	boolean WriteStreamBinsFile(const ALString& sOutputFileName) const;

	// Ecriture dans un fichier des bins construits
	// Cette methode est appelable a tout momennt, entre l'initialisation et la finalisation
	boolean WriteStreamBinMergesFile(const ALString& sOutputFileName) const;

	// Ecriture dans un fichier des bins construits avec les informations de fusion
	boolean WriteStreamBinsAndMergesFile(const ALString& sOutputFileName) const;

	/////////////////////////////////////////////////////////////////////////////////
	// Gestion de la binarisation d'un ensemble de valeurs pour un fichier en entree

	// Construction d'un fichier de bins a partir d'un fichier de valeurs et d'un nombre maximal de bins
	// Parametres:
	//   sInputDataFileName: fichier de valeurs, avec une valeur par ligne
	//   sOutputBinFileName: fichier de bins (min, max, count) au format tabulaire, avec un bin par ligne
	//   nMaxBinNumber: fichier de valeurs, avec une valeur par ligne
	boolean ComputeBins(const ALString& sInputDataFileName, const ALString& sOutputBinFileName);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class MHStreamBiningMixed;

	// Export des bins du resume dans un tableau, en mode standrad et sature
	void ExportStreamStandardBins(ObjectArray* oaStreamBins) const;
	void ExportStreamSaturatedBins(ObjectArray* oaStreamBins) const;

	// Calcul de la granularite du resume, c'est a dire de la plus petite granularite telle
	// que chaque bin soit inclu dans la grille de granularite correspondante
	// Cela correspond egalement au max de la granularite associee a chaque bin
	int ComputeStreamBinsGranularity() const;

	// Calcul de l'effectif total d'un tableau de bins
	int GetBinArrayTotalFrequency(ObjectArray* oaStreamBins) const;

	// Test de la validite de la gestion d'un valeur dans les listes de bin et de fusions
	boolean CheckStreamValue(Continuous cValue) const;

	// Test de la validite complete des listes de bins et de fusions de bins
	boolean CheckAllBins() const;
	boolean CheckAllBinMerges() const;

	// Lecture d'un champ de type valeur
	boolean ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue, boolean& bEndOfLine);

	// Emission d'un warning ou d'un erreur de lecture de fichier pour un numero de ligne donne
	void AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);
	void AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);

	// Nombre max de bins
	int nMaxBinNumber;

	// Parametrage du type de grille
	boolean bFloatingPointGrid;

	// Parametrage du type de bins
	boolean bSaturatedBins;

	// Bin singleton resumant tout le stream, avec le min, le max et l'effectif total
	MHBinMerge globalStatsBin;

	// Liste triee des bins
	SortedList slBins;

	// Liste triee des fusions de bins
	SortedList slBinMerges;

	// Bin de travail pour la recherche ou l'insertion dans les listes
	// Ce bin doit toujours etre pret a l'emploi pendant la gestion du stream
	MHBinMerge* workingBin;
};
