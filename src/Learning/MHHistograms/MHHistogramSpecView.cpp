// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MHHistogramSpecView.h"

MHHistogramSpecView::MHHistogramSpecView()
{
	SetIdentifier("MHHistogramSpec");
	SetLabel("Histogram parameters");
	AddStringField("HistogramCriterion", "Histogram criterion", "");
	AddIntField("DeltaCentralBinExponent", "Delta central bin exponent", 0);
	AddBooleanField("TruncationManagementHeuristic", "Truncation management heuristic", false);
	AddBooleanField("SingularityRemovalHeuristic", "Singularity removal heuristic", false);
	AddDoubleField("EpsilonBinWidth", "Epsilon bin width", 0);
	AddBooleanField("OptimalAlgorithm", "Optimal algorithm", false);
	AddStringField("ExportResultHistograms", "Export result histograms", "");
	AddBooleanField("ExportInternalLogFiles", "Export internal log files", false);
	AddIntField("FileFormat", "File format", 0);
	AddStringField("ResultFilesDirectory", "Result files directory", "");
	AddStringField("ResultFilesPrefix", "Result files prefix", "");

	// Parametrage des styles;
	GetFieldAt("HistogramCriterion")->SetStyle("ComboBox");
	GetFieldAt("TruncationManagementHeuristic")->SetStyle("CheckBox");
	GetFieldAt("SingularityRemovalHeuristic")->SetStyle("CheckBox");
	GetFieldAt("OptimalAlgorithm")->SetStyle("CheckBox");
	GetFieldAt("ExportResultHistograms")->SetStyle("ComboBox");
	GetFieldAt("ExportInternalLogFiles")->SetStyle("CheckBox");
	GetFieldAt("ResultFilesDirectory")->SetStyle("DirectoryChooser");

	// ## Custom constructor

	// Contraintes sur les champs
	cast(UIStringElement*, GetFieldAt("HistogramCriterion"))->SetParameters("G-Enum-fp");
	GetFieldAt("HistogramCriterion")->SetVisible(false);
	cast(UIIntElement*, GetFieldAt("DeltaCentralBinExponent"))->SetMinValue(-1);
	cast(UIIntElement*, GetFieldAt("DeltaCentralBinExponent"))->SetMaxValue(1000);
	cast(UIDoubleElement*, GetFieldAt("EpsilonBinWidth"))->SetMinValue(0);
	cast(UIDoubleElement*, GetFieldAt("EpsilonBinWidth"))->SetMaxValue(KWContinuous::GetMaxValue());
	cast(UIDoubleElement*, GetFieldAt("EpsilonBinWidth"))->SetDefaultValue(0);
	cast(UIBooleanElement*, GetFieldAt("TruncationManagementHeuristic"))->SetDefaultValue(true);
	cast(UIBooleanElement*, GetFieldAt("SingularityRemovalHeuristic"))->SetDefaultValue(true);
	cast(UIStringElement*, GetFieldAt("ExportResultHistograms"))->SetParameters("None\nBest\nBestAndRaw\nAll");
	cast(UIIntElement*, GetFieldAt("FileFormat"))->SetMinValue(1);
	cast(UIIntElement*, GetFieldAt("FileFormat"))->SetMaxValue(3);
	cast(UIIntElement*, GetFieldAt("FileFormat"))->SetDefaultValue(1);

	// Info-bulles
	GetFieldAt("HistogramCriterion")
	    ->SetHelpText("Histogram criterion:"
			  "\n. G-Enum-fp: MODL with automatic granularity parameter and floating-point representation"
			  "\n"
			  "\nWith the G-Enum-fp criterion, the optional parameters are:"
			  "\n- delta central bin exponent,"
			  "\n- truncation management heuristic,"
			  "\n- singularity removal heuristic."
			  "\nLast, the following parameters are available for all methods:"
			  "\n- optimal algorithm,"
			  "\n- export result histograms,"
			  "\n- export internal log files.");
	GetFieldAt("DeltaCentralBinExponent")
	    ->SetHelpText(
		"Delta central bin exponent, relative to the initial min central bin exponent"
		"\n. default: -1, means that the central bin is exponent is optimized automatically"
		"\n. above 0: use the minimum possible central bin exponent plus the specified delta parameter"
		"\n (truncated if the result is beyond the maxiumum possible central bin exponent)");
	GetFieldAt("TruncationManagementHeuristic")
	    ->SetHelpText("Indicates whether to apply the heuristic that deals with truncated values");
	GetFieldAt("SingularityRemovalHeuristic")
	    ->SetHelpText(
		"Indicates whether to remove to singular intervals after optimization using a lower granularity,"
		"\n in order to produce a smoother histogram");
	GetFieldAt("EpsilonBinWidth")
	    ->SetHelpText("Epsilon bin width"
			  "\n Accuracy parameter, used if stricly superior to 0, width the Enum or KM criterion"
			  "\n Otherwise, the total number of epsilon bins is used instead");
	GetFieldAt("OptimalAlgorithm")
	    ->SetHelpText("Optimisation algorithm"
			  "\n. false: greedy heuristic (O(n log n))"
			  "\n. true: optimal algorithm (O(n^3))");
	GetFieldAt("ExportResultHistograms")
	    ->SetHelpText("Indicates whether to export result histogram files"
			  "\n . None"
			  "\n . Best: most interpretable histogram"
			  "\n . BestAndRaw: most interpretable histogram plus possibly raw histogram"
			  "\n . All: all histograms");
	GetFieldAt("ExportInternalLogFiles")
	    ->SetHelpText(
		"Indicates whether to export all internal log files"
		"\n The output files named Histogram<.suffix>.log are always produced and saved"
		"\n using the directory and prefix specified in Khiops pane 'Analysis results'"
		"\n "
		"\n The internal log files are exported using the same directory and prefix, only if required."
		"\n The name of the log files are prefixed by 'MODLH.' and are based of the used algorithmic "
		"components."
		"\n See the documentation for the details related to all the available log file"
		"\n "
		"\n 	    Note that the standard Khiops reports are still produced (PreparationReport.xls), but the "
		"\n 	    unsupervised histogram may not be the good one. Indeed, in its current version, Khiops does"
		"\n 	    not handle discretisation with empty intervals, and the empty intervals have been "
		"\n 	    unconditionally merged in the hiops Preparation report to meet this technical constaint."
		"\n 	    The only reference result is the file Histogram.log");

	// Champ masques, car alimentes en interne
	GetFieldAt("FileFormat")->SetVisible(false);
	GetFieldAt("ResultFilesDirectory")->SetVisible(false);
	GetFieldAt("ResultFilesPrefix")->SetVisible(false);

	// ##
}

MHHistogramSpecView::~MHHistogramSpecView()
{
	// ## Custom destructor

	// ##
}

MHHistogramSpec* MHHistogramSpecView::GetMHHistogramSpec()
{
	require(objValue != NULL);
	return cast(MHHistogramSpec*, objValue);
}

void MHHistogramSpecView::EventUpdate(Object* object)
{
	MHHistogramSpec* editedObject;

	require(object != NULL);

	editedObject = cast(MHHistogramSpec*, object);
	editedObject->SetHistogramCriterion(GetStringValueAt("HistogramCriterion"));
	editedObject->SetDeltaCentralBinExponent(GetIntValueAt("DeltaCentralBinExponent"));
	editedObject->SetTruncationManagementHeuristic(GetBooleanValueAt("TruncationManagementHeuristic"));
	editedObject->SetSingularityRemovalHeuristic(GetBooleanValueAt("SingularityRemovalHeuristic"));
	editedObject->SetEpsilonBinWidth(GetDoubleValueAt("EpsilonBinWidth"));
	editedObject->SetOptimalAlgorithm(GetBooleanValueAt("OptimalAlgorithm"));
	editedObject->SetExportResultHistograms(GetStringValueAt("ExportResultHistograms"));
	editedObject->SetExportInternalLogFiles(GetBooleanValueAt("ExportInternalLogFiles"));
	editedObject->SetFileFormat(GetIntValueAt("FileFormat"));
	editedObject->SetResultFilesDirectory(GetStringValueAt("ResultFilesDirectory"));
	editedObject->SetResultFilesPrefix(GetStringValueAt("ResultFilesPrefix"));

	// ## Custom update

	// ##
}

void MHHistogramSpecView::EventRefresh(Object* object)
{
	MHHistogramSpec* editedObject;

	require(object != NULL);

	editedObject = cast(MHHistogramSpec*, object);
	SetStringValueAt("HistogramCriterion", editedObject->GetHistogramCriterion());
	SetIntValueAt("DeltaCentralBinExponent", editedObject->GetDeltaCentralBinExponent());
	SetBooleanValueAt("TruncationManagementHeuristic", editedObject->GetTruncationManagementHeuristic());
	SetBooleanValueAt("SingularityRemovalHeuristic", editedObject->GetSingularityRemovalHeuristic());
	SetDoubleValueAt("EpsilonBinWidth", editedObject->GetEpsilonBinWidth());
	SetBooleanValueAt("OptimalAlgorithm", editedObject->GetOptimalAlgorithm());
	SetStringValueAt("ExportResultHistograms", editedObject->GetExportResultHistograms());
	SetBooleanValueAt("ExportInternalLogFiles", editedObject->GetExportInternalLogFiles());
	SetIntValueAt("FileFormat", editedObject->GetFileFormat());
	SetStringValueAt("ResultFilesDirectory", editedObject->GetResultFilesDirectory());
	SetStringValueAt("ResultFilesPrefix", editedObject->GetResultFilesPrefix());

	// ## Custom refresh

	// ##
}

const ALString MHHistogramSpecView::GetClassLabel() const
{
	return "Histogram parameters";
}

// ## Method implementation

// ##
