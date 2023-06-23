// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHHistogramSpec.h"
#include "MHHistogramSpecView.h"
#include "MHDiscretizerHistogramMODL.h"
#include "MHBin.h"
#include "Version.h"

////////////////////////////////////////////////////////////////
// Classe GenumCommandLine
// Lancement du calcul d'un histogramme de puis la ligne de commande
class GenumCommandLine : public Object
{
public:
	// Constructeur
	GenumCommandLine();
	~GenumCommandLine();

	// Lancement depuis la ligne de commande
	boolean ComputeHistogram(int argc, char** argv);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Lecture des valeur sous la forme de vecteur
	// Renvoie true si ok
	boolean ReadValues(ContinuousVector*& cvValues);

	// Lecture d'un champ de type valeur
	boolean ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue, boolean& bEndOfLine);

	// Emission d'un warning ou d'un erreur de lecture de fichier pour un numero de ligne donne
	void AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);
	void AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);

	// Initialisation des parametres
	// Initialize l'objet de parametrage histogramSpec
	// Renvoie true si ok, en parametrant le nom des fichiers de valeur en entree et du fichier d'histogramme en
	// sortie
	boolean InitializeParameters(int argc, char** argv);

	// Affichage de l'aide
	void ShowHelp();

	// Initialisation du parametrage
	void InitializeParameterSettings();

	// Nom du fichier de donnees
	ALString sDataFileName;

	// Nom du fichier contenant l'histogramme resultat
	ALString sHistogramFileName;

	// Objet de parametrage des histogramme
	MHHistogramSpec histogramSpec;
};
