// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "FileService.h"
#include "CCHierarchicalDataGrid.h"
#include "JSONFile.h"
#include "KWVersion.h"
#include "KWFrequencyVector.h"
#include "JSONTokenizer.h"
#include "KWLearningErrorManager.h"

/////////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringReport
// Gestion d'un rapport de coclustering
// Lecture/ecriture vers un fichier de rapport
// On se focalise uniquement sur l'entete du rapport ainsi que sur la structure de la grille de coclustering
// en ignorant les aspects hierarchie et informations complementaires
class CCCoclusteringReport : public Object
{
public:
	// Constructeur
	CCCoclusteringReport();
	~CCCoclusteringReport();

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des rapports (uniquement au format json depuis Khiops V11)

	// Suffixe des fichiers de rapports au format json: khcj depuis les rapports au format Khiops V10
	static const ALString GetReportSuffix();

	// Lecture des informations de coclustering a partir d'un fichier de rapport au format json
	// Renvoie true si succes avec initialisation complete de la grille en parametres
	// Emission de messages d'erreur, et reinitialisation de la grille si echec
	boolean ReadReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid);

	// Lecture des informations resume de coclustering a partir de l'entete d'un fichier de rapport au format json
	// Le parsing est tres rapide, mais seules les informations synthetiques sont lues
	// (essentiellement: noms, types et nombres de parties par variables)
	// Les nombre d'instances et de cellules sont renvoye directement depuis la methode,
	// pas depuis la grille partiellement initialisee qui est vide.
	// Renvoie true si succes avec initialisation partielle de la grille en parametres
	// Emission de messages d'erreur, et reinitialisation de la grille si echec
	boolean ReadReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
				 int& nInstanceNumber, int& nCellNumber);

	// Ecriture d'un rapport de coclustering au format JSON
	boolean WriteReport(const ALString& sReportName, const CCHierarchicalDataGrid* coclusteringDataGrid);

	//////////////////////////////////////////////////////////////////////////////////////
	// Methodes standard

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Formats des fichiers
	enum FileFormats
	{
		KHC,
		JSON,
		None
	};

	// Methode de detection du format et de l'encodage d'un fichier, avec emission de message d'erreur si format
	// invalide et de warning si l'encodage pose probleme Le format est renvoye en code retour L'encodage correspond
	// au champ "khiops_encoding" du fichier json (cf. classe JSONFile), ou vide si non trouve ou non valide
	int DetectFileFormatAndEncoding(const ALString& sFileName, ALString& sKhiopsEncoding) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Methodes internes pour la gestion des rapports au format json

	// Lecture des sections d'un rapport de coclustering
	boolean InternalReadReport(CCHierarchicalDataGrid* coclusteringDataGrid, boolean bHeaderOnly);
	boolean ReadSummary(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadDimensionSummaries(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadDimensionPartitions(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadInnerAttributesDimensionSummaries(KWDGAttribute* dgAttribute);
	boolean ReadAttributePartition(KWDGAttribute* attribute, CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadInterval(KWDGAttribute* dgAttribute, KWDGPart* dgPart);
	boolean ReadValueGroup(KWDGAttribute* dgAttribute, KWDGPart* dgPart);
	boolean ReadVarPartAttributeValueGroup(KWDGAttribute* varPartAttribute, KWDGPart* dgPart,
					       const ObjectDictionary* odInnerAttributesAllVarParts,
					       ObjectDictionary* odVarPartAttributeAllVarParts);
	boolean ReadTypicalities(KWDGAttribute* dgAttribute, int nValueNumber, DoubleVector* dvValueTypicalities);
	boolean ReadDimensionHierarchies(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadCells(CCHierarchicalDataGrid* coclusteringDataGrid);

	// Ecriture des sections d'un rapport de coclustering vers un stream en sortie
	void InternalWriteReport(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteSummary(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteDimensionSummaries(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteDimensionSummary(CCHDGAttribute* attribute, JSONFile* fJSON);
	void WriteDimensionPartitions(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteAttributePartition(KWDGAttribute* attribute, JSONFile* fJSON);
	void WriteInnerAttributes(const KWDGInnerAttributes* innerAttributes, JSONFile* fJSON);
	void WriteDimensionHierarchies(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteCells(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);

	// Fichier des gestion du rapport
	ALString sReportFileName;
	FILE* fReport;
	int nLineIndex;
	boolean bEndOfLine;
	static const int nMaxFieldSize = 10000;
	char* sFileBuffer;
	boolean bReadDebug;
	int nHeaderInstanceNumber;
	int nHeaderCellNumber;
	ALString sLocalFileName;

	// Libelle des sections d'un rapport de coclustering
	static const ALString sKeyWordInstances;
	static const ALString sKeyWordCells;
	static const ALString sKeyWordNullCost;
	static const ALString sKeyWordCost;
	static const ALString sKeyWordLevel;
	static const ALString sKeyWordInitialDimensions;
	static const ALString sKeyWordFrequencyAttribute;
	static const ALString sKeyWordDictionary;
	static const ALString sKeyWordDatabase;
	static const ALString sKeyWordSamplePercentage;
	static const ALString sKeyWordSamplingMode;
	static const ALString sKeyWordSelectionVariable;
	static const ALString sKeyWordSelectionValue;
};
