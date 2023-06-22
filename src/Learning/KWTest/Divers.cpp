// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Divers.h"

void LiftCurveStudy()
{
	boolean bAllResults = false;
	const int nSampleNumber = 10000;
	const int nInstanceNumber = 1000;
	const int nFirstTargetModalityNumber = 500;
	int i;
	ObjectArray oaInstances;
	SampleObject* soInstance;
	double dLiftCurveArea;
	double dRandomLiftCurveArea;
	double dOptimaLiftCurveArea;
	int nInstance;
	DoubleVector dvLiftCurveValues;
	double dProb;
	double dSum;
	double dSquareSum;
	double dMean;
	double dStandardDeviation;

	// Initialisation du tableau d'instances
	assert(nFirstTargetModalityNumber <= nInstanceNumber);
	oaInstances.SetSize(nInstanceNumber);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		soInstance = new SampleObject;
		if (nInstance < nFirstTargetModalityNumber)
			soInstance->SetInt(1);
		oaInstances.SetAt(nInstance, soInstance);
	}

	// Calcul des surfaces de lift aleatoire et optimales
	dRandomLiftCurveArea = nFirstTargetModalityNumber * 1.0 * (nInstanceNumber + 1) / 2.0;
	dOptimaLiftCurveArea = nFirstTargetModalityNumber * 1.0 * nInstanceNumber -
			       nFirstTargetModalityNumber * 1.0 * (nFirstTargetModalityNumber - 1) / 2;

	// Collecte des resultats
	SetRandomSeed(1);
	dvLiftCurveValues.SetSize(nSampleNumber);
	for (i = 0; i < nSampleNumber; i++)
	{
		// Perturbation aleatoire de l'ordre des instance
		oaInstances.Shuffle();

		// Calcul de la valeur de lift
		dLiftCurveArea = 0;
		for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
		{
			soInstance = cast(SampleObject*, oaInstances.GetAt(nInstance));

			// Mise de la valeur de lift
			if (soInstance->GetInt() == 1)
				dLiftCurveArea += (nInstanceNumber - nInstance);
		}

		// Memorisation de la valeur
		dvLiftCurveValues.SetAt(i, (dLiftCurveArea - dRandomLiftCurveArea) /
					       (dOptimaLiftCurveArea - dRandomLiftCurveArea));
	}

	// Moyenne et ecart type des resultats
	dSum = 0;
	dSquareSum = 0;
	for (i = 0; i < nSampleNumber; i++)
	{
		dLiftCurveArea = dvLiftCurveValues.GetAt(i);

		// Mise a jour des sommes
		dSum += dLiftCurveArea;
		dSquareSum += dLiftCurveArea * dLiftCurveArea;
	}
	dMean = dSum / (nSampleNumber - 1);
	dStandardDeviation = sqrt(fabs((dSquareSum - dSum * dSum / (nSampleNumber - 1)) / (nSampleNumber - 1)));

	// Affichage des resultats
	if (bAllResults)
	{
		cout << "p\tLift value\tNormal\t" << dRandomLiftCurveArea << "\t" << dOptimaLiftCurveArea << "\t"
		     << dMean << "\t" << dStandardDeviation << "\n";
		for (i = 0; i < nSampleNumber; i++)
		{
			dProb = (i + 0.5) / nSampleNumber;
			cout << dProb << "\t" << dvLiftCurveValues.GetAt(i) << "\t"
			     << KWStat::InvNormal(dProb, dMean, dStandardDeviation) << "\n";
		}
		cout << flush;
	}

	// Simulation des resultats sur les tres petites probabilites
	dProb = 1;
	cout << "p\tNormal\tc(p)\tNormal\n";
	for (i = 0; i < 20; i++)
	{
		dProb /= 10;
		cout << dProb << "\t" << -KWStat::InvNormal(dProb, dMean, dStandardDeviation) << "\t" << 1 - dProb
		     << "\t" << KWStat::InvNormal(1 - dProb, dMean, dStandardDeviation) << "\n";
	}
	cout << flush;

	// Nettoyage memoire
	oaInstances.DeleteAll();
}

////////////////////////////////////////////////////////////////////////////////////

double MODLValue(int nTotalFrequency, int nFirstFrequency)
{
	require(0 <= nFirstFrequency and nFirstFrequency <= nTotalFrequency);

	return KWStat::LnFactorial(nTotalFrequency + 1) - KWStat::LnFactorial(nFirstFrequency) -
	       KWStat::LnFactorial(nTotalFrequency - nFirstFrequency);
}

/////////////////////////////////////////////////////////
// On dispose de trois valeurs consecutives A, B et C
// On envisage les regroupements (A+B), C et A, (B+C)
// Intuitivement, B devrait etre regroupe avec l'intervalle
// de frequence la plus voisine.
// L'experimentation consiste a a verifier cette intuition
void MODLStudy3()
{
	const int nMax = 100;
	const int nMin = 1;
	const int nMin1 = 0;
	int nA;
	int nA1;
	int nB;
	int nB1;
	int nC;
	int nC1;
	double dAB_CValue;
	double dA_BCValue;
	double dAC_BValue;
	double dA_B_CValue;
	double dABCValue;
	double dDiff;
	double dBestDiff;
	double nMatchNumber;
	double nTotalNumber;
	double dMinValue;
	int nMinValueIndex;
	double nMatch1Number;
	double nMatch2Number;
	double nMatch3Number;
	double nMatch4Number;
	double nMatch5Number;

	// Parcours de toutes les frequences possibles d'intervalles
	dBestDiff = 0;
	nMatchNumber = 0;
	nTotalNumber = 0;
	nMatch1Number = 0;
	nMatch2Number = 0;
	nMatch3Number = 0;
	nMatch4Number = 0;
	nMatch5Number = 0;
	/*
	cout << "A\tA1\tB\tB1\tC\tC1\t" <<
	"V(A,B,C)\tV(A+B,C)\tV(A,B+C)\tV(A+C,B)\tV(A+B+C)\t" <<
	"Diff\tVMin\tIndex" << endl;
	*/
	for (nA = nMin; nA <= nMax; nA++)
	{
		for (nB = nMin; nB <= nMax; nB++)
		{
			for (nC = nMin; nC <= nMax; nC++)
			{
				// Parcours des frequence de la premiere modalite
				for (nA1 = nMin1; nA1 <= nA; nA1++)
				{
					for (nB1 = nMin1; nB1 <= nB; nB1++)
					{
						for (nC1 = nMin1; nC1 <= nC; nC1++)
						{
							// Evaluation si la frequence dans B n'est pas comprise
							// entre celles dans A et C, pour une frequence
							// dans A inferieure A celle dans C
							if (nA1 * nB < nB1 * nA and nB1 * nC < nB * nC1)
							{
								nTotalNumber++;

								// Evaluation MODL dans l'hypothese (A+B), C
								dAB_CValue = log(4.0) + MODLValue(nA + nB, nA1 + nB1) +
									     MODLValue(nC, nC1);

								// Evaluation MODL dans l'hypothese A, (B+C)
								dA_BCValue = log(4.0) + MODLValue(nA, nA1) +
									     MODLValue(nB + nC, nB1 + nC1);

								// Evaluation MODL dans l'hypothese (A+C), B
								dAC_BValue = log(4.0) + MODLValue(nA + nC, nA1 + nC1) +
									     MODLValue(nB, nB1);

								// Evaluation MODL dans l'hypothese (A+B+C)
								dABCValue = MODLValue(nA + nB + nC, nA1 + nB1 + nC1);

								// Evaluation MODL dans l'hypothese A, B, C
								dA_B_CValue = log(5.0) + MODLValue(nA, nA1) +
									      MODLValue(nB, nB1) + MODLValue(nC, nC1);

								// Calcul de la valeur minimale et de son index
								nMinValueIndex = 1;
								dMinValue = dA_B_CValue;
								if (dAB_CValue < dMinValue)
								{
									nMinValueIndex = 2;
									dMinValue = dAB_CValue;
								}
								if (dA_BCValue < dMinValue)
								{
									nMinValueIndex = 3;
									dMinValue = dA_BCValue;
								}
								if (dAC_BValue < dMinValue)
								{
									nMinValueIndex = 4;
									dMinValue = dAC_BValue;
								}
								if (dABCValue < dMinValue)
								{
									nMinValueIndex = 5;
									dMinValue = dABCValue;
								}
								if (nMinValueIndex == 1)
									nMatch1Number++;
								else if (nMinValueIndex == 2)
									nMatch2Number++;
								else if (nMinValueIndex == 3)
									nMatch3Number++;
								else if (nMinValueIndex == 4)
									nMatch4Number++;
								else if (nMinValueIndex == 5)
									nMatch5Number++;

								// Evaluation des cas ou le meilleur regroupement est
								// (A+C), B
								dDiff = dAB_CValue - dAC_BValue;
								if (dA_BCValue - dAC_BValue < dDiff)
									dDiff = dA_BCValue - dAC_BValue;
								// if (dDiff > 0)
								// if (1 < nMinValueIndex and
								//	nMinValueIndex < 5)
								{
									nMatchNumber++;
									/*
									dBestDiff = dDiff;
									cout << nA << "\t" << nA1 << "\t" <<
									nB << "\t" << nB1 << "\t" <<
									nC << "\t" << nC1 << "\t" <<
									dA_B_CValue << "\t" <<
									dAB_CValue << "\t" <<
									dA_BCValue << "\t" <<
									dAC_BValue << "\t" <<
									dABCValue << "\t" <<
									dDiff << "\t" <<
									dMinValue << "\t" << nMinValueIndex << endl;
									*/
								}
							}
						}
					}
				}
			}
		}
	}
	cout << endl
	     << "MatchA_B_C\t" << nMatch1Number << endl
	     << "MatchAB_C\t" << nMatch2Number << endl
	     << "MatchA_BC\t" << nMatch3Number << endl
	     << "MatchAC_B\t" << nMatch4Number << endl
	     << "MatchABC\t" << nMatch5Number << endl
	     << "Match\t" << nMatchNumber << endl
	     << "Total\t" << nTotalNumber << endl;
}

/////////////////////////////////////////////////////////
// On dispose de quatre valeurs consecutives A, B, C et D
// On envisage tous les regroupements en trois groupes,
// et tous les regroupements en deux groupes, pour verifier
// que les groupes consitues sont compatibles avec l'ordre
// des valeurs descriptives (par frequence de la premiere
// valeur cible).
void MODLStudy4()
{
	const int nBell4 = 15;
	const int nABC_DValue = 0;
	const int nABD_CValue = 1;
	const int nACD_BValue = 2;
	const int nBCD_AValue = 3;
	const int nAB_CDValue = 4;
	const int nAC_BDValue = 5;
	const int nAD_BCValue = 6;
	const int nAB_C_DValue = 7;
	const int nAC_B_DValue = 8;
	const int nAD_B_CValue = 9;
	const int nBC_A_DValue = 10;
	const int nBD_A_CValue = 11;
	const int nCD_A_BValue = 12;
	const int nABCDValue = 13;
	const int nA_B_C_DValue = 14;
	const int nMax = 50;
	const int nMin = 1;
	const int nMin1 = 0;
	int nA;
	int nA1;
	int nB;
	int nB1;
	int nC;
	int nC1;
	int nD;
	int nD1;
	DoubleVector dvPartitionValues;
	DoubleVector dvMatchNumbers;
	double dTotalNumber;
	double dMinValue;
	int nMinValueIndex;
	int i;
	const double fPointSteps = 1e7;
	double fPointTime;

	// Parcours de toutes les frequences possibles d'intervalles
	dTotalNumber = 0;
	dvPartitionValues.SetSize(nBell4);
	dvMatchNumbers.SetSize(nBell4);
	fPointTime = fPointSteps;
	for (nA = nMin; nA <= nMax; nA++)
	{
		for (nB = nMin; nB <= nMax; nB++)
		{
			for (nC = nMin; nC <= nMax; nC++)
			{
				for (nD = nMin; nD <= nMax; nD++)
				{
					// Parcours des frequence de la premiere modalite
					for (nA1 = nMin1; nA1 <= nA; nA1++)
					{
						for (nB1 = nMin1; nB1 <= nB; nB1++)
						{
							for (nC1 = nMin1; nC1 <= nC; nC1++)
							{
								for (nD1 = nMin1; nD1 <= nD; nD1++)
								{
									// Evaluation si la frequence dans B n'est pas
									// comprise entre celles dans A et C, pour une
									// frequence dans A inferieure A celle dans C
									if (nA1 * nB < nB1 * nA and
									    nB1 * nC < nB * nC1 and nC1 * nD < nC * nD1)
									{
										dTotalNumber++;
										if (dTotalNumber > fPointTime)
										{
											fPointTime += fPointSteps;
											cout << "." << flush;
										}

										// Evaluation MODL pour chaque hypothese
										dvPartitionValues.SetAt(
										    nABC_DValue,
										    log(8.0) +
											MODLValue(nA + nB + nC,
												  nA1 + nB1 + nC1) +
											MODLValue(nD, nD1));
										dvPartitionValues.SetAt(
										    nABD_CValue,
										    log(8.0) +
											MODLValue(nA + nB + nD,
												  nA1 + nB1 + nD1) +
											MODLValue(nC, nC1));
										dvPartitionValues.SetAt(
										    nACD_BValue,
										    log(8.0) +
											MODLValue(nA + nC + nD,
												  nA1 + nC1 + nD1) +
											MODLValue(nB, nB1));
										dvPartitionValues.SetAt(
										    nBCD_AValue,
										    log(8.0) +
											MODLValue(nB + nC + nD,
												  nB1 + nC1 + nD1) +
											MODLValue(nA, nA1));
										dvPartitionValues.SetAt(
										    nAB_CDValue,
										    log(8.0) +
											MODLValue(nA + nB, nA1 + nB1) +
											MODLValue(nC + nD, nC1 + nD1));
										dvPartitionValues.SetAt(
										    nAC_BDValue,
										    log(8.0) +
											MODLValue(nA + nC, nA1 + nC1) +
											MODLValue(nB + nD, nB1 + nD1));
										dvPartitionValues.SetAt(
										    nAD_BCValue,
										    log(8.0) +
											MODLValue(nA + nD, nA1 + nD1) +
											MODLValue(nB + nC, nB1 + nC1));
										//
										dvPartitionValues.SetAt(
										    nAB_C_DValue,
										    log(14.0) +
											MODLValue(nA + nB, nA1 + nB1) +
											MODLValue(nC, nC1) +
											MODLValue(nD, nD1));
										dvPartitionValues.SetAt(
										    nAC_B_DValue,
										    log(14.0) +
											MODLValue(nA + nC, nA1 + nC1) +
											MODLValue(nB, nB1) +
											MODLValue(nD, nD1));
										dvPartitionValues.SetAt(
										    nAD_B_CValue,
										    log(14.0) +
											MODLValue(nA + nD, nA1 + nD1) +
											MODLValue(nB, nB1) +
											MODLValue(nC, nC1));
										dvPartitionValues.SetAt(
										    nBC_A_DValue,
										    log(14.0) +
											MODLValue(nB + nC, nB1 + nC1) +
											MODLValue(nA, nA1) +
											MODLValue(nD, nD1));
										dvPartitionValues.SetAt(
										    nBD_A_CValue,
										    log(14.0) +
											MODLValue(nB + nD, nB1 + nD1) +
											MODLValue(nA, nA1) +
											MODLValue(nC, nC1));
										dvPartitionValues.SetAt(
										    nCD_A_BValue,
										    log(14.0) +
											MODLValue(nC + nD, nC1 + nD1) +
											MODLValue(nA, nA1) +
											MODLValue(nB, nB1));
										//
										dvPartitionValues.SetAt(
										    nABCDValue,
										    MODLValue(nA + nB + nC + nD,
											      nA1 + nB1 + nC1 + nD1));
										//
										dvPartitionValues.SetAt(
										    nA_B_C_DValue,
										    log(15.0) + MODLValue(nA, nA1) +
											MODLValue(nB, nB1) +
											MODLValue(nC, nC1) +
											MODLValue(nD, nD1));

										// Calcul de la valeur minimale et de
										// son index
										nMinValueIndex = -1;
										dMinValue = 1e300;
										for (i = 0; i < nBell4; i++)
										{
											if (dvPartitionValues.GetAt(i) <
											    dMinValue)
											{
												dMinValue =
												    dvPartitionValues
													.GetAt(i);
												nMinValueIndex = i;
											}
										}
										dvMatchNumbers.UpgradeAt(nMinValueIndex,
													 1);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	cout << endl
	     << "Match ABC_D	" << dvMatchNumbers.GetAt(nABC_DValue) << endl
	     << "Match ABD_C	" << dvMatchNumbers.GetAt(nABD_CValue) << endl
	     << "Match ACD_B	" << dvMatchNumbers.GetAt(nACD_BValue) << endl
	     << "Match BCD_A	" << dvMatchNumbers.GetAt(nBCD_AValue) << endl
	     << "Match AB_CD	" << dvMatchNumbers.GetAt(nAB_CDValue) << endl
	     << "Match AC_BD	" << dvMatchNumbers.GetAt(nAC_BDValue) << endl
	     << "Match AD_BC	" << dvMatchNumbers.GetAt(nAD_BCValue) << endl
	     << "Match AB_C_D	" << dvMatchNumbers.GetAt(nAB_C_DValue) << endl
	     << "Match AC_B_D	" << dvMatchNumbers.GetAt(nAC_B_DValue) << endl
	     << "Match AD_B_C	" << dvMatchNumbers.GetAt(nAD_B_CValue) << endl
	     << "Match BC_A_D	" << dvMatchNumbers.GetAt(nBC_A_DValue) << endl
	     << "Match BD_A_C	" << dvMatchNumbers.GetAt(nBD_A_CValue) << endl
	     << "Match CD_A_B	" << dvMatchNumbers.GetAt(nCD_A_BValue) << endl
	     << "Match ABCD	" << dvMatchNumbers.GetAt(nABCDValue) << endl
	     << "Match A_B_C_D	" << dvMatchNumbers.GetAt(nA_B_C_DValue) << endl
	     << "Total\t" << dTotalNumber << endl;
}

//////////////////////////////////////////////////////////////////////////

void MergeFiles(const ALString& sInputFile1, const ALString& sInputFile2, const ALString& sOutputFile,
		const ALString& sSeparatorChar)
{
	boolean bOk;
	fstream fstInputFile1;
	fstream fstInputFile2;
	fstream fstOutputFile;
	int nRecordNumber;
	char c;

	// Ouverture des fichiers
	bOk = FileService::OpenInputFile(sInputFile1, fstInputFile1);
	bOk = FileService::OpenInputFile(sInputFile2, fstInputFile2);
	bOk = FileService::OpenOutputFile(sOutputFile, fstOutputFile);

	// Lecture ecriture des lignes en concatenant les deux lignes des deux fichiers
	nRecordNumber = 0;
	while (not fstInputFile1.eof())
	{
		c = (char)fstInputFile1.get();
		if (c != '\n' and not fstInputFile1.eof())
			fstOutputFile << c;
		else
		{
			fstOutputFile << sSeparatorChar;
			while (not fstInputFile2.eof())
			{
				c = (char)fstInputFile2.get();
				if (not fstInputFile2.eof())
					fstOutputFile << c;
				if (c == '\n' or fstInputFile2.eof())
					break;
			}
			nRecordNumber++;
		}
	}
	cout << sOutputFile << ":\t" << nRecordNumber << " lignes" << endl;

	// Fermeture des fichiers
	fstInputFile1.close();
	fstInputFile2.close();
	fstOutputFile.close();
}

void mainMergeFiles(int argc, char** argv)
{
	ALString sInputFile1;
	ALString sInputFile2;
	ALString sOutputFile;
	ALString sSeparatorChar;

	if (argc != 5)
	{
		cout << "MergeFiles [Input File 1] [Input File 2] [Output File] [Separator char]" << endl;
		cout << "\tSeparator char: TAB, BLANK, any char" << endl;
	}
	else
	{
		sInputFile1 = argv[1];
		sInputFile2 = argv[2];
		sOutputFile = argv[3];
		sSeparatorChar = argv[4];
		if (sSeparatorChar == "TAB")
			sSeparatorChar = "\t";
		else if (sSeparatorChar == "BLANK")
			sSeparatorChar = " ";
		if (sOutputFile == sInputFile1 or sOutputFile == sInputFile2)
			cout << "Output file must be different from input files" << endl;
		else
			MergeFiles(sInputFile1, sInputFile2, sOutputFile, sSeparatorChar);
	}
}

void ReplaceBlanksByTabs(const ALString& sInputFile, const ALString& sOutputFile)
{
	boolean bOk;
	fstream fstInputFile;
	fstream fstOutputFile;
	int nRecordNumber;
	char c;

	// Ouverture des fichiers
	bOk = FileService::OpenInputFile(sInputFile, fstInputFile);
	bOk = FileService::OpenOutputFile(sOutputFile, fstOutputFile);

	// Lecture ecriture des lignes
	nRecordNumber = 0;
	while (not fstInputFile.eof())
	{
		c = (char)fstInputFile.get();
		if (not fstInputFile.eof())
		{
			if (c != ' ')
				fstOutputFile << c;
			else
				fstOutputFile << '\t';
		}
	}

	// Fermeture des fichiers
	fstInputFile.close();
	fstOutputFile.close();
}

void mainReplaceBlanksByTabs(int argc, char** argv)
{
	ALString sInputFile;
	ALString sOutputFile;
	ALString sSeparatorChar;

	if (argc != 3)
	{
		cout << "ReplaceBlanksByTabs [Input File] [Output File]" << endl;
	}
	else
	{
		sInputFile = argv[1];
		sOutputFile = argv[2];
		if (sOutputFile == sInputFile)
			cout << "Output file must be different from input file" << endl;
		else
			ReplaceBlanksByTabs(sInputFile, sOutputFile);
	}
}

void ExpandSparseBinaryFile(const ALString& sInputFile, const ALString& sOutputFile, char cSeparatorChar,
			    int nColumnNumber)
{
	boolean bOk;
	fstream fstInputFile;
	fstream fstOutputFile;
	int nRecordNumber;
	char c;
	IntVector ivColumnIndexes;
	int nColumn;
	ALString sColumnIndex;
	boolean bEmptyLine;
	int nLineOffset;
	int nCharOffset;
	int nValueNumber;

	require(nColumnNumber >= 0);

	// Ouverture des fichiers
	bOk = FileService::OpenInputFile(sInputFile, fstInputFile);
	bOk = FileService::OpenOutputFile(sOutputFile, fstOutputFile);

	// Initialisation du vecteur des index des colonnes non vides
	ivColumnIndexes.SetSize(nColumnNumber);

	// Lecture ecriture des lignes en concatenant les deux lignes des deux fichiers
	nRecordNumber = 0;
	nColumn = 0;
	bEmptyLine = true;
	nLineOffset = 0;
	nCharOffset = 0;
	nValueNumber = 0;
	while (not fstInputFile.eof())
	{
		c = (char)fstInputFile.get();
		nCharOffset++;

		// Analyse de la ligne creuse
		if (c != '\n' and not fstInputFile.eof())
		{
			// Analyse de l'index
			if (c != cSeparatorChar)
				sColumnIndex += c;
			// Memorisation de l'index
			else
			{
				nValueNumber++;
				bEmptyLine = false;
				nColumn = StringToInt(sColumnIndex);
				if (nColumn < 1 or nColumn > nColumnNumber)
					cout << "Pos(" << nLineOffset << ", " << nCharOffset << ")"
					     << " Index (" << sColumnIndex << ") invalide" << endl;
				else
					ivColumnIndexes.SetAt(nColumn - 1, 1);
				sColumnIndex = "";
			}
		}
		// Fin de ligne: transcodification de la ligne creuse dans le fichier plein
		else
		{
			nCharOffset = 0;
			nLineOffset++;
			if (not(bEmptyLine and fstInputFile.eof()))
			{
				for (nColumn = 0; nColumn < nColumnNumber; nColumn++)
				{
					fstOutputFile << ivColumnIndexes.GetAt(nColumn);
					fstOutputFile << cSeparatorChar;
				}
				fstOutputFile << "\n";
				nRecordNumber++;
				ivColumnIndexes.Initialize();
			}
			bEmptyLine = true;
		}
	}
	cout << sOutputFile << "\t" << nRecordNumber << "\tInstances"
	     << "\t" << nLineOffset << "\tLignes"
	     << "\t" << nValueNumber << "\tValeurs" << endl;

	// Fermeture des fichiers
	fstInputFile.close();
	fstOutputFile.close();
}

void mainExpandSparseBinaryFile(int argc, char** argv)
{
	ALString sInputFile;
	ALString sOutputFile;
	ALString sSeparatorChar;
	char cSeparatorChar;
	int nColumnNumber;

	if (argc != 5)
	{
		cout << "ExpandSparseBinaryFile [Input File] [Output File] [Separator char] [Column Number]" << endl;
		cout << "\tSeparator char: TAB, BLANK, any char" << endl;
	}
	else
	{
		sInputFile = argv[1];
		sOutputFile = argv[2];
		sSeparatorChar = argv[3];
		nColumnNumber = StringToInt(argv[4]);
		cSeparatorChar = ' ';
		if (sSeparatorChar == "TAB")
			cSeparatorChar = '\t';
		else if (sSeparatorChar == "BLANK")
			cSeparatorChar = ' ';
		else if (sSeparatorChar == "")
			cSeparatorChar = ' ';
		else
			cSeparatorChar = sSeparatorChar.GetAt(0);
		if (sOutputFile == sInputFile)
			cout << "Output file must be different from input file" << endl;
		else if (nColumnNumber <= 0)
			cout << "Column Number (" << nColumnNumber << ") must be greater than 0" << endl;
		else
			ExpandSparseBinaryFile(sInputFile, sOutputFile, cSeparatorChar, nColumnNumber);
	}
}

//////////////////////////////////////////////////////////////////////////

// Comptage du nombre d'occurrence de chaque mot dans le fichier Reuters
// En sortie, le tableau des nombre d'occurrence par mot est mis a jour
void AnalyseReutersFile(const ALString& sInputFile, IntVector* ivWordGlobalCounts)
{
	boolean bOk;
	const char cSeparatorChar = ';';
	fstream fstInputFile;
	int nWordNumber;
	SymbolVector svWordLabels;
	IntVector ivWordIds;
	IntVector ivWordCounts;
	ALString sClass;
	char c;
	int nFieldCutPos;
	ALString sField;
	ALString sWordId;
	int nWordId;
	ALString sWordLabel;
	ALString sWordCount;
	ALString sMissingWordLabel;
	int nWordCount;
	ALString sTmp;
	int nLineOffset;
	IntVector ivAllWordCounts;

	require(ivWordGlobalCounts != NULL);

	// Ouverture du fichier
	bOk = FileService::OpenInputFile(sInputFile, fstInputFile);

	// Lecture ecriture des lignes
	ivWordGlobalCounts->SetSize(0);
	nWordNumber = 0;
	nLineOffset = 0;
	while (not fstInputFile.eof())
	{
		c = (char)fstInputFile.get();

		// Analyse de d'une ligne
		if (c != '\n' and not fstInputFile.eof())
		{
			// Recherche d'un champ
			if (c != cSeparatorChar)
				sField += c;
			// Analyse d'un champ
			else
			{
				sField.TrimLeft();
				sField.TrimRight();

				// Cas de la premiere ligne: comptage des mots
				if (nLineOffset == 0)
				{
					// Extraction des informations
					nWordId = 0;
					sWordLabel = "";
					nFieldCutPos = sField.Find(' ');
					if (nFieldCutPos == -1)
						Global::AddError("Reuters file analysis",
								 sTmp + "Line " + IntToString(nLineOffset + 1),
								 "Wrong field (" + sField + ")");
					else
					{
						sWordId = sField.Left(nFieldCutPos);
						sWordLabel = sField.Right(sField.GetLength() - nFieldCutPos - 1);
						nWordId = StringToInt(sWordId);
					}

					// Memorisation du nombre max de mots atteints
					if (nWordId + 1 > nWordNumber)
						nWordNumber = nWordId + 1;
				}
				// Cas des lignes d'enregistrement: liste de paires (WordId WordCount), suivi de Class
				else
				{
					// Extraction des informations
					nWordId = 0;
					sWordLabel = "";
					nFieldCutPos = sField.Find(' ');

					// Si une seule valeur: c'est la classe
					if (nFieldCutPos == -1)
					{
						assert(sClass == "");
						sClass = sField;
					}
					// Sinon: paire (WordId, WordCount)
					else
					{
						sWordId = sField.Left(nFieldCutPos);
						sWordCount = sField.Right(sField.GetLength() - nFieldCutPos - 1);
						nWordId = StringToInt(sWordId);
						nWordCount = StringToInt(sWordCount);

						// Incrementation du compteur de mot
						ivWordGlobalCounts->UpgradeAt(nWordId, nWordCount);
					}
				}

				// Reinitialisation du champ
				sField = "";
			}
		}
		// Fin de ligne
		else
		{
			// Ligne d'entete: taillage du vecteur de comptage des mots
			if (nLineOffset == 0)
				ivWordGlobalCounts->SetSize(nWordNumber);

			// Ligne suivante
			sClass = "";
			nLineOffset++;
		}
	}
	cout << sInputFile << "\t" << nLineOffset << "\tLignes" << endl;

	// Fermeture du fichier
	fstInputFile.close();
}

void TranscodeReutersFile(const ALString& sInputFile, const ALString& sOutputFile, int nType, int nMinWordFrequency)
{
	boolean bOk;
	boolean bDisplayLabels = false;
	boolean bDisplayLines = false;
	const char cSeparatorChar = ';';
	fstream fstInputFile;
	fstream fstOutputFile;
	int nTotalWordNumber;
	IntVector ivWordGlobalCounts;
	SymbolVector svWordLabels;
	IntVector ivWordIds;
	IntVector ivWordCounts;
	ALString sClass;
	char c;
	int nFieldCutPos;
	ALString sField;
	ALString sWordId;
	int nWordId;
	ALString sWordLabel;
	ALString sWordCount;
	ALString sMissingWordLabel;
	int nWordCount;
	ALString sTmp;
	int nLineOffset;
	int nValueNumber;
	IntVector ivAllWordCounts;
	int n;
	int i;

	require(0 <= nType and nType <= 2);
	require(nMinWordFrequency >= 0);

	// Comptage du nombre d'occurrence de chaque mot dans le fichier Reuters
	AnalyseReutersFile(sInputFile, &ivWordGlobalCounts);
	nTotalWordNumber = 0;
	for (n = 0; n < ivWordGlobalCounts.GetSize(); n++)
	{
		if (ivWordGlobalCounts.GetAt(n) >= nMinWordFrequency)
			nTotalWordNumber++;
	}
	cout << "Frequent words (threshold=" << nMinWordFrequency << ")\t" << nTotalWordNumber << endl;

	// Ouverture des fichiers
	bOk = FileService::OpenInputFile(sInputFile, fstInputFile);
	bOk = FileService::OpenOutputFile(sOutputFile, fstOutputFile);

	// Lecture ecriture des lignes
	nLineOffset = 0;
	nValueNumber = 0;
	while (not fstInputFile.eof())
	{
		c = (char)fstInputFile.get();

		// Analyse de d'une ligne
		if (c != '\n' and not fstInputFile.eof())
		{
			// Recherche d'un champ
			if (c != cSeparatorChar)
				sField += c;
			// Analyse d'un champ
			else
			{
				sField.TrimLeft();
				sField.TrimRight();

				// Cas de la premiere ligne: liste de paires (WordId WordLabel)
				if (nLineOffset == 0)
				{
					// Extraction des informations
					nWordId = 0;
					sWordLabel = "";
					nFieldCutPos = sField.Find(' ');
					if (nFieldCutPos == -1)
						Global::AddError("Reuters file analysis",
								 sTmp + "Line " + IntToString(nLineOffset + 1),
								 "Wrong field (" + sField + ")");
					else
					{
						sWordId = sField.Left(nFieldCutPos);
						sWordLabel = sField.Right(sField.GetLength() - nFieldCutPos - 1);
						nWordId = StringToInt(sWordId);
					}

					// Memorisation du mot
					assert(nWordId >= nValueNumber);
					assert(svWordLabels.GetSize() == nValueNumber);
					while (nValueNumber < nWordId)
					{
						sMissingWordLabel = sTmp + "Word" + IntToString(nValueNumber);
						svWordLabels.Add(Symbol(sMissingWordLabel));
						nValueNumber++;
					}
					assert(nWordId == nValueNumber);
					svWordLabels.Add(Symbol(sWordLabel));

					// Affichage du mot
					if (bDisplayLabels)
						cout << nWordId << "\t" << sWordLabel << endl;
				}
				// Cas des lignes d'enregistrement: liste de paires (WordId WordCount), suivi de Class
				else
				{
					// Extraction des informations
					nWordId = 0;
					sWordLabel = "";
					nFieldCutPos = sField.Find(' ');

					// Si une seule valeur: c'est la classe
					if (nFieldCutPos == -1)
					{
						assert(sClass == "");
						sClass = sField;

						// Affichage de la classe
						if (bDisplayLines)
							cout << sClass << endl;
					}
					// Sinon: paire (WordId, WordCount)
					else
					{
						sWordId = sField.Left(nFieldCutPos);
						sWordCount = sField.Right(sField.GetLength() - nFieldCutPos - 1);
						nWordId = StringToInt(sWordId);
						nWordCount = StringToInt(sWordCount);

						// Memorisation des informations
						ivWordIds.Add(nWordId);
						ivWordCounts.Add(nWordCount);

						// Affichage des infos
						if (bDisplayLines)
							cout << nWordId << "\t" << svWordLabels.GetAt(nWordId) << "\t"
							     << nWordCount << "\t";
					}
				}

				// Reinitialisation du champs
				nValueNumber++;
				sField = "";
			}
		}
		// Fin de ligne: transcodification de la ligne creuse dans le fichier plein
		else
		{
			// Type 0: standard pour la classification de texte
			if (nType == 0)
			{
				// Ligne d'entete
				if (nLineOffset == 0)
				{
					for (n = 0; n < svWordLabels.GetSize(); n++)
					{
						if (ivWordGlobalCounts.GetAt(n) >= nMinWordFrequency)
							fstOutputFile << svWordLabels.GetAt(n) << "\t";
					}
					fstOutputFile << "Class";
					fstOutputFile << "\n";
				}
				// Ligne d'enregistrement
				else
				{
					// Creation de vecteurs pleins pour les nombres d'occurrence par mots
					ivAllWordCounts.SetSize(0);
					ivAllWordCounts.SetSize(svWordLabels.GetSize());
					for (n = 0; n < ivWordIds.GetSize(); n++)
					{
						assert(0 <= ivWordIds.GetAt(n) and
						       ivWordIds.GetAt(n) < ivAllWordCounts.GetSize());
						ivAllWordCounts.SetAt(ivWordIds.GetAt(n), ivWordCounts.GetAt(n));
					}

					// Ecriture de l'enregistrement
					for (n = 0; n < ivAllWordCounts.GetSize(); n++)
					{
						if (ivWordGlobalCounts.GetAt(n) >= nMinWordFrequency)
							fstOutputFile << ivAllWordCounts.GetAt(n) << "\t";
					}
					fstOutputFile << sClass;
					fstOutputFile << "\n";
				}
			}

			// Type 1: pour le co-clustering, avec IdText, IdWord, WordCount, Class
			if (nType == 1)
			{
				// Ligne d'entete
				if (nLineOffset == 0)
				{
					fstOutputFile << "TextId\tWordId\tWordCount\tClass\n";
				}
				// Ligne d'enregistrement
				else
				{
					// Creation d'autant d'enregistrements qu'il y a de mots dans le texte
					for (n = 0; n < ivWordIds.GetSize(); n++)
					{
						assert(0 <= ivWordIds.GetAt(n) and
						       ivWordIds.GetAt(n) < svWordLabels.GetSize());
						if (ivWordGlobalCounts.GetAt(ivWordIds.GetAt(n)) >= nMinWordFrequency)
						{
							fstOutputFile << "Text" << nLineOffset << "\t"
								      << svWordLabels.GetAt(ivWordIds.GetAt(n)) << "\t"
								      << ivWordCounts.GetAt(n) << "\t" << sClass
								      << "\n";
						}
					}
				}
			}

			// Type 2: pour le co-clustering, avec IdText, IdWord, Class (chaque ligne repetee WordCount
			// fois)
			if (nType == 2)
			{
				// Ligne d'entete
				if (nLineOffset == 0)
				{
					fstOutputFile << "TextId\tWordId\tClass\n";
				}
				// Ligne d'enregistrement
				else
				{
					// Creation d'autant d'enregistrements qu'il y a de mots dans le texte
					for (n = 0; n < ivWordIds.GetSize(); n++)
					{
						assert(0 <= ivWordIds.GetAt(n) and
						       ivWordIds.GetAt(n) < svWordLabels.GetSize());
						if (ivWordGlobalCounts.GetAt(ivWordIds.GetAt(n)) >= nMinWordFrequency)
						{
							for (i = 0; i < ivWordCounts.GetAt(n); i++)
							{
								fstOutputFile << "Text" << nLineOffset << "\t"
									      << svWordLabels.GetAt(ivWordIds.GetAt(n))
									      << "\t" << sClass << "\n";
							}
						}
					}
				}
			}

			// Nettoyage des informations sur la ligne
			nValueNumber = 0;
			sClass = "";
			ivWordIds.SetSize(0);
			ivWordCounts.SetSize(0);
			nLineOffset++;
		}
	}
	cout << sOutputFile << "\t" << nLineOffset << "\tLignes" << endl;

	// Fermeture des fichiers
	fstInputFile.close();
	fstOutputFile.close();
}

void mainTranscodeReutersFile(int argc, char** argv)
{
	ALString sInputFile;
	int nMinWordFrequency;
	ALString sOutputFile;

	if (argc != 3)
	{
		cout << "ExpandSparseBinaryFile [Reuters Input File] [Min Word Frequency]" << endl;
	}
	else
	{
		sInputFile = argv[1];
		nMinWordFrequency = StringToInt(argv[2]);

		// 0: standard pour la classification de texte
		sOutputFile = "Reuters.classification.txt";
		cout << "Transcode to " << sOutputFile << endl;
		TranscodeReutersFile(sInputFile, sOutputFile, 0, nMinWordFrequency);

		// 1: pour le co-clustering, avec IdText, IdWord, WordCount, Class
		sOutputFile = "Reuters.coclustering.txt";
		cout << "Transcode to " << sOutputFile << endl;
		TranscodeReutersFile(sInputFile, sOutputFile, 1, nMinWordFrequency);

		// 2: pour le co-clustering, avec IdText, IdWord, Class (chaque ligne repetee WordCount fois)
		sOutputFile = "Reuters.coclustering.full.txt";
		cout << "Transcode to " << sOutputFile << endl;
		TranscodeReutersFile(sInputFile, sOutputFile, 2, nMinWordFrequency);
	}
}

//////////////////////////////////////////////////////////////////////////

Continuous ComputeProductResult(ContinuousVector* cvObject, int nIndex1, int nIndex2)
{
	Continuous cResult = 1;

	// Calcul du produit
	cResult *= cvObject->GetAt(nIndex1) * cvObject->GetAt(nIndex2);
	return cResult;
}

Continuous ComputeDivideResult(ContinuousVector* cvObject, int nIndex, Continuous cDivisor)
{
	Continuous cResult;

	// Calcul du produit
	cResult = cvObject->GetAt(nIndex) / cDivisor;
	return cResult;
}

Continuous ComputeRoundResult(ContinuousVector* cvObject, int nIndex)
{
	Continuous cResult;

	// Calcul du produit
	cResult = (Continuous)(int)floor(cvObject->GetAt(nIndex) + 0.5);
	return cResult;
}

void PhysicalWrite(fstream* fstDatabase, ContinuousVector* cvObject)
{
	char cFieldSeparator = '\t';
	int i;
	const char* sContinuousValue;
	/*
	double dValue;
	double dMantissa;
	int nExp2;
	int nFactor;
	int nMantissa;
	*/

	// Parcours de tous les attributs Loaded
	for (i = 0; i < cvObject->GetSize(); i++)
	{
		if (i > 0)
			fstDatabase->rdbuf()->sputc(cFieldSeparator);
		//(*fstDatabase) << cFieldSeparator;
		// fstDatabase->rdbuf()->sputn("180", 3);
		//(*fstDatabase) << "180";
		//(*fstDatabase) << KWContinuous::ContinuousToString(cvObject->GetAt(i));
		// sContinuousValue = KWContinuous::ContinuousToString(cvObject->GetAt(i));
		// sContinuousValue = DoubleToString(cvObject->GetAt(i));
		// sContinuousValue = DoubleToString(180);
		/*
		dValue = cvObject->GetAt(i);
		dMantissa = frexp(dValue, &nExp2);
		nFactor = (int)pow(10, nExp2/3);
		dMantissa *= nFactor;
		nMantissa = (int)dMantissa;
		sContinuousValue = IntToString(nMantissa);
		*/
		// sContinuousValue = "180";
		//(*fstDatabase) << sContinuousValue;
		// sContinuousValue = IntToString((int)cvObject->GetAt(i));
		sContinuousValue = KWContinuous::ContinuousToString(cvObject->GetAt(i));
		fstDatabase->rdbuf()->sputn(sContinuousValue, (std::streamsize)strlen(sContinuousValue));
	}
	fstDatabase->rdbuf()->sputc('\n');
	//(*fstDatabase) << '\n';
}

void TestOpenManyFiles()
{
	const int nFileNumber = 1000;
	FILE* fFiles[nFileNumber];
	FILE* fFile;
	ALString sBaseFileName = "c:/temp/Test";
	ALString sFileName;
	boolean bOk;
	int nFile;
	int nCreatedFileNumber;
	ALString sTmp;

	// Ouverture en lecture en mode binaire
	nCreatedFileNumber = 0;
	for (nFile = 0; nFile < nFileNumber; nFile++)
	{
		sFileName = sBaseFileName + IntToString(nFile + 1);
		fFile = p_fopen(sFileName, "wb");
		bOk = fFile != NULL;
		if (not bOk)
		{
			Global::AddError("File", sFileName, "Unable to open file");
			perror("PB!!!");

			// Arret
			break;
		}
		else
		{
			fFiles[nFile] = fFile;
			nCreatedFileNumber++;
		}
	}

	// Fermerture des fichiers
	for (nFile = 0; nFile < nCreatedFileNumber; nFile++)
	{
		fFile = fFiles[nFile];
		fclose(fFile);
	}
	Global::AddSimpleMessage(sTmp + "Created file number: " + IntToString(nCreatedFileNumber));
}

void TestIOPerf()
{
	boolean bTestCPU = true;
	ALString sFileName;
	fstream fst;
	ALString sTmp;
	ALString sLabel;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;
	int i;
	int nObject;
	double dTotal;
	int nObjectNumber;
	int nSourceSize;
	int nTargetSize;
	ContinuousVector cvSourceObjectRef;
	ContinuousVector* cvSourceObject;
	ContinuousVector* cvTargetObject;
	IntVector ivIndex1;
	IntVector ivIndex2;
	const int nBufferSize = 30000;
	char* cStreamBuffer;

	// Initialisations
	nObjectNumber = 10000;
	nSourceSize = 784;
	nTargetSize = 10000;
	cvSourceObjectRef.SetSize(nSourceSize);
	for (i = 0; i < cvSourceObjectRef.GetSize(); i++)
		cvSourceObjectRef.SetAt(i, (Continuous)RandomInt(255));
	ivIndex1.SetSize(nTargetSize);
	ivIndex2.SetSize(nTargetSize);
	for (i = 0; i < ivIndex1.GetSize(); i++)
	{
		ivIndex1.SetAt(i, RandomInt(nSourceSize));
		ivIndex1.SetAt(i, RandomInt(nSourceSize));
	}

	tBegin = clock();

	// Test CPU
	if (bTestCPU)
	{
		Continuous cValue;
		const char* sContinuousValue;

		dTotal = 0;
		for (i = 0; i < 1e7; i++)
		{
			cValue = (Continuous)i;
			sContinuousValue = KWContinuous::ContinuousToString(cValue);
			dTotal += atof(sContinuousValue);
		}
		// dTotal += KWContinuous::StringToContinuous(KWContinuous::ContinuousToString((Continuous)i));
		// dTotal += log(i+1);
	}
	// Test calculs d'agregats
	else
	{
		// Calcul principal
		sFileName = "e:/temp/testCPU.txt";
		cStreamBuffer = SystemObject::NewCharArray(nBufferSize);
		//		fst.rdbuf()->setbuf(cStreamBuffer, nBufferSize);
		FileService::OpenOutputFile(sFileName, fst);
		dTotal = 0;
		cvSourceObject = NULL;
		cvTargetObject = NULL;
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			cvSourceObject = cvSourceObjectRef.Clone();
			cvTargetObject = new ContinuousVector;
			cvTargetObject->SetSize(nTargetSize);

			// Calcul de l'objet cible
			for (i = 0; i < ivIndex1.GetSize(); i++)
			{
				dTotal++;

				cvTargetObject->SetAt(
				    i, ComputeProductResult(cvSourceObject, ivIndex1.GetAt(i), ivIndex2.GetAt(i)));
				cvTargetObject->SetAt(i, ComputeDivideResult(cvTargetObject, i, 256));
				cvTargetObject->SetAt(i, ComputeRoundResult(cvTargetObject, i));

				/*
				cvTargetObject->SetAt(i,
				(Continuous)(int)((cvSourceObject->GetAt(ivIndex1.GetAt(i)) *
				cvSourceObject->GetAt(ivIndex2.GetAt(i)))/256));
				*/

				/*
				cvTargetObject->SetAt(i,
				(Continuous)(int)((i*i)/256));
				*/
			}

			// Sortie
			PhysicalWrite(&fst, cvTargetObject);

			delete cvSourceObject;
			delete cvTargetObject;
		}
		fst.close();
		SystemObject::DeleteCharArray(cStreamBuffer);
	}

	tEnd = clock();

	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;
	sLabel = sTmp + "Calculs(" + DoubleToString(dTotal) + ")";
	cout << sLabel << "\t" << SecondsToString((int)dTotalComputeTime) << endl;
	exit(0);
}

void TestCPUPerf()
{
	Timer timer;
	const int nNumber = 100000000;
	// const char* sFloatNumber="0.123456";
	int n;
	int nValue;
	int nTotal;
	double dValue;
	double dTotal;

	// Addition de int
	timer.Reset();
	timer.Start();
	nValue = 1;
	nTotal = 0;
	for (n = 0; n < nNumber; n++)
	{
		nValue++;
		nTotal += nValue;
	}
	timer.Stop();
	cout << "Int addition (" << nTotal << "): " << SecondsToString(timer.GetElapsedTime()) << endl;

	// Addition de double
	timer.Reset();
	timer.Start();
	dValue = 1;
	dTotal = 0;
	for (n = 0; n < nNumber; n++)
	{
		dValue++;
		dTotal += dValue;
	}
	timer.Stop();
	cout << "Double addition (" << dTotal << "): " << SecondsToString(timer.GetElapsedTime()) << endl;

	// Multiplication de double
	timer.Reset();
	timer.Start();
	dValue = 1;
	dTotal = 0;
	for (n = 0; n < nNumber; n++)
	{
		dValue++;
		dTotal *= dValue;
	}
	timer.Stop();
	cout << "Double multiplication (" << dTotal << "): " << SecondsToString(timer.GetElapsedTime()) << endl;

	// Addition de double
	timer.Reset();
	timer.Start();
	dValue = 1;
	dTotal = 0;
	for (n = 0; n < nNumber; n++)
	{
		dValue++;
		dTotal += log(dValue);
	}
	timer.Stop();
	cout << "Double logs addition (" << dTotal << "): " << SecondsToString(timer.GetElapsedTime()) << endl;

	// Addition de double resultats d'une conversion
	timer.Reset();
	timer.Start();
	dTotal = 0;
	for (n = 0; n < nNumber; n++)
	{
		// dValue = atof(sFloatNumber);
		dValue = atof("0.123456");
		dTotal += dValue;
	}
	timer.Stop();
	cout << "Double conversion (" << dTotal << "): " << SecondsToString(timer.GetElapsedTime()) << endl;

	exit(0);
}

void GenerateScaleFreeGraph(int nNodeNumber, int nDegree, int nEdgeNumber, const ALString& sOutputFile)
{
	IntVector ivNodeDegrees;
	int nNode;
	int nEdge;
	int nSourceNode;
	int nTargetNode;
	IntVector ivSourceNodes;
	IntVector ivTargetNodes;
	const int nInitialNodeNumber = nDegree * 2;
	const int nTotalNodeNumber = nNodeNumber;
	const int nNodeDegree = nDegree;
	int nTotalDegree;
	int nRandomTotal;
	int nTotal;
	IntVector ivCurrentNodeDegrees;
	int nCurrentTotalDegree;
	int nCurrentEdgeNumber;
	int nRandomEdge;
	fstream fstOutputFile;
	boolean bOk;

	// Initialisation du graphe
	ivNodeDegrees.SetSize(nTotalNodeNumber);
	nTotalDegree = 0;
	for (nNode = 0; nNode < nInitialNodeNumber; nNode++)
	{
		// Creation d'un arc entre un noeud et son noeud suivant
		nSourceNode = nNode % nInitialNodeNumber;
		nTargetNode = (nNode + 1) % nInitialNodeNumber;

		// Memorisation de l'arc
		ivSourceNodes.Add(nSourceNode);
		ivTargetNodes.Add(nTargetNode);
		ivNodeDegrees.UpgradeAt(nSourceNode, 1);
		ivNodeDegrees.UpgradeAt(nTargetNode, 1);
		nTotalDegree += 2;

		// Creation d'un arc entre un noeud et son noeud suivant de suivant
		nSourceNode = nNode % nInitialNodeNumber;
		nTargetNode = (nNode + 2) % nInitialNodeNumber;

		// Memorisation de l'arc
		ivSourceNodes.Add(nSourceNode);
		ivTargetNodes.Add(nTargetNode);
		ivNodeDegrees.UpgradeAt(nSourceNode, 1);
		ivNodeDegrees.UpgradeAt(nTargetNode, 1);
		nTotalDegree += 2;
	}

	// Generation aleatoire des arcs pour les autres noeuds, avec attachement preferentiel au noeud de fort degree
	for (nSourceNode = nInitialNodeNumber; nSourceNode < nTotalNodeNumber; nSourceNode++)
	{
		ivCurrentNodeDegrees.CopyFrom(&ivNodeDegrees);
		nCurrentTotalDegree = nTotalDegree;

		// Creation de plusieurs arcs aleatoires
		for (nEdge = 0; nEdge < nNodeDegree; nEdge++)
		{
			// Rercherche d'un index aleatoire entre 0 et le degree total
			nRandomTotal = RandomInt(nCurrentTotalDegree - 1);

			// Recherche du noeud cible (aleatoire) correspondant a cet index, de facon biaisee par le degre
			// du noeud
			nTotal = 0;
			for (nTargetNode = 0; nTargetNode < nTotalNodeNumber; nTargetNode++)
			{
				nTotal += ivCurrentNodeDegrees.GetAt(nTargetNode);
				if (nTotal >= nRandomTotal)
					break;
			}

			// Memorisation de l'arc
			ivSourceNodes.Add(nSourceNode);
			ivTargetNodes.Add(nTargetNode);
			ivNodeDegrees.UpgradeAt(nSourceNode, 1);
			ivNodeDegrees.UpgradeAt(nTargetNode, 1);
			nTotalDegree += 2;
		}
	}

	// Deuxieme passe de generation des arcs une fois que tous les noeuds sont generes,
	// biasee par les degrees des noeuds
	ivCurrentNodeDegrees.CopyFrom(&ivNodeDegrees);
	nCurrentEdgeNumber = ivSourceNodes.GetSize();
	for (nEdge = 0; nEdge < nEdgeNumber; nEdge++)
	{
		// Rercherche d'une arete aleatoire
		nRandomEdge = RandomInt(nCurrentEdgeNumber - 1);
		nSourceNode = ivSourceNodes.GetAt(nRandomEdge);
		nTargetNode = ivTargetNodes.GetAt(nRandomEdge);

		// Memorisation de l'arc
		ivSourceNodes.Add(nSourceNode);
		ivTargetNodes.Add(nTargetNode);
		ivNodeDegrees.UpgradeAt(nSourceNode, 1);
		ivNodeDegrees.UpgradeAt(nTargetNode, 1);
		nTotalDegree += 2;
	}

	// Enregistrement du fichier
	bOk = FileService::OpenOutputFile(sOutputFile, fstOutputFile);
	if (bOk)
	{
		fstOutputFile << "Source\tTarget\n";
		for (nEdge = 0; nEdge < ivSourceNodes.GetSize(); nEdge++)
		{
			fstOutputFile << "N" << ivSourceNodes.GetAt(nEdge) + 1 << "\t"
				      << "N" << ivTargetNodes.GetAt(nEdge) + 1 << "\n";
			fstOutputFile << "N" << ivTargetNodes.GetAt(nEdge) + 1 << "\t"
				      << "N" << ivSourceNodes.GetAt(nEdge) + 1 << "\n";
		}
		fstOutputFile.close();
	}
}

void mainGenerateScaleFreeGraph(int argc, char** argv)
{
	int nNodeNumber;
	int nDegree;
	int nEdgeNumber;
	ALString sOutputFile;

	if (argc != 5)
	{
		cout << "GenerateScaleFreeGraph [Node number] [Degree] [Output File]" << endl;
	}
	else
	{
		nNodeNumber = StringToInt(argv[1]);
		nDegree = StringToInt(argv[2]);
		nEdgeNumber = StringToInt(argv[3]);
		sOutputFile = argv[4];
		GenerateScaleFreeGraph(nNodeNumber, nDegree, nEdgeNumber, sOutputFile);
	}
}

void mainGenerateCurves(int argc, char** argv)
{
	const double dPi = 3.141592654;
	int nParamSinNumber = 4;
	int nParamCosNumber = 4;
	int nParamNormalNumber = 3;
	int nParamClusterNumber = 10;
	int nParamSin;
	int nParamCos;
	int nParamNormal;
	double dParamNormal;
	int nParamCluster;
	const int nPointNumber = 1000000;
	int nPoint;
	double dTime;
	double dNoiseProb;
	int i;

	// Entete
	cout << "CurveId"
	     << "\t"
	     << "Time"
	     << "\t"
	     << "Value"
	     << "\t"
	     << "ParamCurve"
	     << "\t"
	     << "ParamSin"
	     << "\t"
	     << "ParamCos"
	     << "\t"
	     << "ParamNoise"
	     << "\t"
	     << "Alea"
	     << "\n";

	// Generation des points
	for (nPoint = 0; nPoint < nPointNumber; nPoint++)
	{
		// Choix a aleatoire des parametres
		nParamSin = RandomInt(nParamSinNumber - 1);
		nParamCos = RandomInt(nParamCosNumber - 1);
		nParamNormal = RandomInt(nParamNormalNumber - 1);
		i = 0;
		dParamNormal = 0.25;
		while (i < nParamNormal)
		{
			dParamNormal *= 2;
			i++;
		}
		nParamCluster = 1 + RandomInt(nParamClusterNumber - 1);
		dNoiseProb = RandomDouble();

		// Choix aleatoire du temps de mesure
		dTime = RandomDouble();

		// Generation du point de mesure
		cout << "C" << nParamCluster << "(sin(" << nParamSin << ".pi.t)+cos(" << nParamCos << ".pi.t)+Noise("
		     << dParamNormal << ")"
		     << "\t";
		cout << dTime << "\t";
		cout << sin(nParamSin * dPi * dTime) + cos(nParamCos * dPi * dTime) +
			    KWStat::InvNormal(dNoiseProb, 0, dParamNormal)
		     << "\t";
		cout << nParamCluster << "\t";
		cout << nParamSin << "\t";
		cout << nParamCos << "\t";
		cout << dParamNormal << "\t";
		cout << RandomDouble() << "\n";
	}
}

void TranscodeDigitDataset()
{
	int nDisplayDigitNumber = 0;
	KWClass* kwcDigits;
	KWSTDatabaseTextFile databaseDigits;
	KWObject* kwoDigit;
	boolean bOk;
	int i;
	int j;
	Continuous cValue;
	int nIndex;
	fstream fstDigitCurves;

	// Lecture du dictionnaire
	KWClassDomain::GetCurrentDomain()->ReadFile("E:/learning/Digits/digits.kdic");
	KWClassDomain::GetCurrentDomain()->Compile();
	kwcDigits = KWClassDomain::GetCurrentDomain()->LookupClass("Digits");
	assert(kwcDigits->GetLoadedAttributeNumber() == 28 * 28 + 1);

	// Initialisation de la base
	databaseDigits.SetClassName(kwcDigits->GetName());
	databaseDigits.SetDatabaseName("E:/learning/Digits/digits_train.txt");

	// Lecture de la base
	// Ouverture de la base en lecture
	bOk = databaseDigits.OpenForRead();
	if (bOk)
	{
		// Ouverture du fichier resultat
		FileService::OpenOutputFile("E:/learning/Digits/DigitCurves/digit_train_curves.txt", fstDigitCurves);
		fstDigitCurves << "Digit\tIndex\tX\tY\tValue\n";

		// Parcours des digits
		nIndex = 0;
		while (not databaseDigits.IsEnd())
		{
			kwoDigit = databaseDigits.Read();
			if (kwoDigit != NULL)
			{
				nIndex++;

				// Affichage
				if (nIndex <= nDisplayDigitNumber)
				{
					cout << nIndex << ": ";
					cout << kwoDigit->GetSymbolValueAt(
						    kwcDigits->GetLoadedAttributeAt(28 * 28)->GetLoadIndex())
					     << endl;
					for (i = 0; i < 28; i++)
					{
						for (j = 0; j < 28; j++)
						{
							cValue = kwoDigit->GetContinuousValueAt(
							    kwcDigits->GetLoadedAttributeAt(j * 28 + i)
								->GetLoadIndex());
							if (cValue > 0)
								cout << '.';
							else
								cout << ' ';
						}
						cout << endl;
					}
				}

				// Memorisation du resultat
				for (i = 0; i < 28; i++)
				{
					for (j = 0; j < 28; j++)
					{
						cValue = kwoDigit->GetContinuousValueAt(
						    kwcDigits->GetLoadedAttributeAt(j * 28 + i)->GetLoadIndex());
						fstDigitCurves << kwoDigit->GetSymbolValueAt(
						    kwcDigits->GetLoadedAttributeAt(28 * 28)->GetLoadIndex());
						fstDigitCurves << '\t' << nIndex;
						fstDigitCurves << '\t' << j + 1;
						fstDigitCurves << '\t' << 28 - i;
						fstDigitCurves << '\t' << cValue;
						fstDigitCurves << '\n';
					}
				}

				delete kwoDigit;
				// break;
			}
		}
		databaseDigits.Close();
		fstDigitCurves.close();
	}

	// Nettoyage
	KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void BellNumberGenerator(int argc, char** argv)
{
	int nValueNumber;
	int nPartNumber;

	if (argc != 2)
		cout << "Bell number generator <ValueNumber>\n";
	else
	{
		nValueNumber = StringToInt(argv[1]);
		if (nValueNumber < 1)
			nValueNumber = 1;
		cout << "K\tI\tln B(I,K)\n";
		for (nPartNumber = 1; nPartNumber <= nValueNumber; nPartNumber++)
		{
			cout << nValueNumber << "\t" << nPartNumber << "\t" << KWStat::LnBell(nValueNumber, nPartNumber)
			     << endl;
		}
	}
}

void BellNumber(int argc, char** argv)
{
	boolean bOk;
	int nValueNumber;
	int nPartNumber;

	if (argc != 3)
		cout << "Bell number <ValueNumber> <part number>\n";
	else
	{
		bOk = true;
		nValueNumber = StringToInt(argv[1]);
		nPartNumber = StringToInt(argv[2]);
		if (nValueNumber < 1)
		{
			cout << "Value number <" << nValueNumber << "> must be greater than 0" << endl;
			bOk = false;
		}
		if (nPartNumber < 1)
		{
			cout << "Part number <" << nPartNumber << "> must be greater than 0" << endl;
			bOk = false;
		}
		if (nPartNumber > nValueNumber)
		{
			cout << "Part number <" << nPartNumber << "> must be smaller than value number < "
			     << nValueNumber << ">" << endl;
			bOk = false;
		}
		if (bOk)
			cout << KWContinuous::ContinuousToString(KWStat::LnBell(nValueNumber, nPartNumber)) << endl;
	}
}

void AnyCharFileGenerator()
{
	const ALString sFileName = "c:\\temp\\AnyChar.txt";
	fstream fTest;
	int i;
	int nChar;
	char cChar;

	FileService::OpenOutputFile(sFileName, fTest);
	fTest
	    << "Index\tChar\t<Char>"
	       "\tisupper\tislower\tisdigit\tisxdigit\tisalnum\tisspace\tispunct\tisprint\tisgraph\tiscntrl\tisascii\n";
	for (i = 0; i < 20; i++)
	{
		for (nChar = 1; nChar < 256; nChar++)
		{
			cChar = (char)nChar;
			if (cChar == '\n')
				continue;
			//			if (nChar == 26)
			//				continue;
			fTest << nChar << "\t";
			if (cChar == '"')
				fTest << "\"\"\"\""
				      << "\t";
			else
				fTest << "\"" << cChar << "\""
				      << "\t";
			if (cChar == '"')
				fTest << "\"<\"\">\""
				      << "\t";
			else
				fTest << "\"<" << cChar << ">\""
				      << "\t";
			fTest << (isupper(nChar) != 0) << "\t";
			fTest << (islower(nChar) != 0) << "\t";
			fTest << (isdigit(nChar) != 0) << "\t";
			fTest << (isxdigit(nChar) != 0) << "\t";
			fTest << (isalnum(nChar) != 0) << "\t";
			fTest << (isspace(nChar) != 0) << "\t";
			fTest << (ispunct(nChar) != 0) << "\t";
			fTest << (isprint(nChar) != 0) << "\t";
			fTest << (isgraph(nChar) != 0) << "\t";
			fTest << (iscntrl(nChar) != 0) << "\t";
			fTest << (isascii(nChar) != 0) << "\n";
		}
	}
	fTest.close();
}

void MixedAsciiUTF8CharFileGenerator()
{
	const ALString sFileName = "c:\\temp\\MixedAsciiUTF8Chars.txt";
	fstream fTest;
	int i;
	int nChar;
	char cChar;
	IntVector ivUTF8FirstChars;
	StringVector svUTF8Classes;
	int nUTF8Index;

	// Choix des permieres caracteres utf8
	ivUTF8FirstChars.Add(0xc3); // Latin
	ivUTF8FirstChars.Add(0xd0); // Cyrillic
	ivUTF8FirstChars.Add(0xce); // Greek
	ivUTF8FirstChars.Add(0xd8); // Arab
	svUTF8Classes.Add("UTF8 Latin");
	svUTF8Classes.Add("UTF8 Cyrillic");
	svUTF8Classes.Add("UTF8 Greek");
	svUTF8Classes.Add("UTF8 Arab");

	// Ecriture dans le fichier
	FileService::OpenOutputFile(sFileName, fTest);
	fTest << "Char <Char>\tType\n";
	for (i = 0; i < 20; i++)
	{
		// Caracteres ansi et ascii
		for (nChar = 1; nChar < 256; nChar++)
		{
			cChar = (char)nChar;

			// Le caractere
			if (cChar == '\r')
				continue;
			if (cChar == '\n')
				continue;
			if (nChar == 26)
				continue;
			if (cChar == '\t')
				continue;
			if (cChar == '"')
				fTest << "\"" << nChar << " <\"\"> \""
				      << "\t";
			else
				fTest << nChar << " <" << cChar << ">"
				      << "\t";

			// La classe du caractere
			if (nChar < 128)
				fTest << "ASCII\n";
			else
				fTest << "ANSI\n";
		}

		// Caracteres utf8
		for (nUTF8Index = 0; nUTF8Index < ivUTF8FirstChars.GetSize(); nUTF8Index++)
		{
			for (nChar = 0x80; nChar < 0xc0; nChar++)
			{
				cChar = (char)nChar;

				// Le caractere
				fTest << ivUTF8FirstChars.GetAt(nUTF8Index) * 256 + nChar;
				fTest << " <" << (unsigned char)ivUTF8FirstChars.GetAt(nUTF8Index)
				      << (unsigned char)nChar << "> \t";

				// La classe du caractere
				fTest << svUTF8Classes.GetAt(nUTF8Index) << "\n";
			}
		}
	}
	fTest.close();
}

void BuildAllCharValueFile()
{
	const ALString sFileName = "c:\\temp\\AllCharValues.txt";
	fstream fTest;
	int nChar;
	char cChar;
	IntVector ivUTF8FirstChars;
	StringVector svUTF8Classes;
	int nUTF8Index;

	// Choix des permieres caracteres utf8
	ivUTF8FirstChars.Add(0xc3); // Latin
	ivUTF8FirstChars.Add(0xd0); // Cyrillic
	ivUTF8FirstChars.Add(0xce); // Greek
	ivUTF8FirstChars.Add(0xd8); // Arab
	svUTF8Classes.Add("UTF8 Latin");
	svUTF8Classes.Add("UTF8 Cyrillic");
	svUTF8Classes.Add("UTF8 Greek");
	svUTF8Classes.Add("UTF8 Arab");

	// Ecriture dans le fichier
	FileService::OpenOutputFile(sFileName, fTest);

	// Caracteres ansi et ascii
	for (nChar = 1; nChar < 256; nChar++)
	{
		cChar = (char)nChar;

		// Le caractere
		if (cChar == '\r')
			continue;
		if (cChar == '\n')
			continue;
		if (nChar == 26)
			continue;
		if (cChar == '\t')
			continue;
		if (cChar == '"')
			continue;
		if (nChar < 128)
			fTest << "ASCII ";
		else
			fTest << "ANSI ";
		fTest << nChar << " <" << cChar << ">\n";
	}

	// Caracteres utf8
	for (nUTF8Index = 0; nUTF8Index < ivUTF8FirstChars.GetSize(); nUTF8Index++)
	{
		for (nChar = 0x80; nChar < 0xc0; nChar++)
		{
			cChar = (char)nChar;

			// Le caractere
			fTest << svUTF8Classes.GetAt(nUTF8Index) << " ";
			fTest << ivUTF8FirstChars.GetAt(nUTF8Index) * 256 + nChar;
			fTest << " <" << (unsigned char)ivUTF8FirstChars.GetAt(nUTF8Index) << (unsigned char)nChar
			      << ">\n";
		}
	}
	fTest.close();
}

void BuildAllCharValueJsonFile()
{
	const ALString sFileName = "c:\\temp\\AllCharValues.json";
	JSONFile jsonFile;
	ALString sValue;
	int nChar;
	char cChar;
	IntVector ivUTF8FirstChars;
	StringVector svUTF8Classes;
	int nUTF8Index;

	// Choix des permieres caracteres utf8
	ivUTF8FirstChars.Add(0xc3); // Latin
	ivUTF8FirstChars.Add(0xd0); // Cyrillic
	ivUTF8FirstChars.Add(0xce); // Greek
	ivUTF8FirstChars.Add(0xd8); // Arab
	svUTF8Classes.Add("UTF8 Latin");
	svUTF8Classes.Add("UTF8 Cyrillic");
	svUTF8Classes.Add("UTF8 Greek");
	svUTF8Classes.Add("UTF8 Arab");

	// Ecriture dans le fichier
	jsonFile.SetFileName(sFileName);
	jsonFile.OpenForWrite();
	jsonFile.BeginKeyArray("chars");

	// Caracteres ansi et ascii
	for (nChar = 1; nChar < 256; nChar++)
	{
		cChar = (char)nChar;

		// Le caractere
		if (cChar == '\r')
			continue;
		if (cChar == '\n')
			continue;
		if (nChar == 26)
			continue;
		if (cChar == '\t')
			continue;
		if (cChar == '"')
			continue;
		if (nChar < 128)
			sValue = "ASCII ";
		else
			sValue = "ANSI ";
		sValue += IntToString(nChar);
		sValue += " <";
		sValue += cChar;
		sValue += ">";
		jsonFile.WriteString(sValue);
	}

	// Caracteres utf8
	for (nUTF8Index = 0; nUTF8Index < ivUTF8FirstChars.GetSize(); nUTF8Index++)
	{
		for (nChar = 0x80; nChar < 0xc0; nChar++)
		{
			cChar = (char)nChar;

			// Le caractere
			sValue = svUTF8Classes.GetAt(nUTF8Index) + " ";
			sValue += IntToString(ivUTF8FirstChars.GetAt(nUTF8Index) * 256 + nChar);
			sValue += " <";
			sValue += (unsigned char)ivUTF8FirstChars.GetAt(nUTF8Index);
			sValue += (unsigned char)nChar;
			sValue += ">";
			jsonFile.WriteString(sValue);
		}
	}

	// Fermeture
	jsonFile.EndArray();
	jsonFile.Close();
}

void CreateHugeTable()
{
	FILE* fTable;
	const int nRecordsNumber = 1000;
	ALString sRecords;
	int i;
	int nBlock;

	// Creation d'une chaien ayant de nombreux records
	for (i = 0; i < nRecordsNumber; i++)
		sRecords += "0\n1\n";

	// Ecriture dans le fichier
	FileService::OpenOutputBinaryFile("c:\\temp\\test\\HugeTable\\HugeTable.txt", fTable);
	fwrite("Var\n", 1, 4, fTable);
	for (nBlock = 0; nBlock < 15; nBlock++)
	{
		for (i = 0; i < 100000; i++)
			fwrite((char*)sRecords, 1, sRecords.GetLength(), fTable);
		fwrite("a\n", 1, 2, fTable);
	}
	fclose(fTable);
}

void SortStudy1()
{
	int nSize;
	DoubleVector vector;
	Timer timer;
	int i;

	nSize = 1000;
	while (nSize < 200000000)
	{
		cout << "Size\t" << nSize << endl;
		timer.Reset();
		timer.Start();
		vector.SetSize(nSize);
		timer.Stop();
		cout << "\tCreation\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nSize; i++)
			vector.SetAt(i, RandomDouble());
		timer.Stop();
		cout << "\tFill\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.Sort();
		timer.Stop();
		cout << "\tSort\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.SetSize(0);
		timer.Stop();
		cout << "\tReset\t" << timer.GetElapsedTime() << endl;
		nSize *= 2;
	}
}

void SortStudy2()
{
	int nSize;
	ObjectArray vector;
	DoubleObject* dObject;
	Timer timer;
	int i;

	vector.SetCompareFunction(DoubleObjectCompare);
	nSize = 1000;
	while (nSize < 200000000)
	{
		cout << "Size\t" << nSize << endl;
		timer.Reset();
		timer.Start();
		vector.SetSize(nSize);
		timer.Stop();
		cout << "\tCreation\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nSize; i++)
		{
			dObject = new DoubleObject;
			dObject->SetDouble(RandomDouble());
			vector.SetAt(i, dObject);
		}
		timer.Stop();
		cout << "\tFill\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.Sort();
		timer.Stop();
		cout << "\tSort\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.DeleteAll();
		timer.Stop();
		cout << "\tReset\t" << timer.GetElapsedTime() << endl;
		nSize *= 2;
	}
}

void SortStudy3()
{
	int nSize;
	int nValueNumber = INT_MAX;
	SortedList vector(DoubleObjectCompare);
	DoubleObject* dObject;
	Timer timer;
	int i;

	nSize = 1000;
	while (nSize < 200000000)
	{
		cout << "Size\t" << nSize << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nSize; i++)
		{
			dObject = new DoubleObject;
			dObject->SetDouble(RandomInt(nValueNumber - 1));
			vector.Add(dObject);
		}
		timer.Stop();
		cout << "\tCreation, fill and sort\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.DeleteAll();
		timer.Stop();
		cout << "\tReset\t" << timer.GetElapsedTime() << endl;
		nSize *= 2;
	}
}

void SortStudy4()
{
	int nSize;
	int nValueNumber = INT_MAX;
	SortedList vector(KWSortableIndexCompare);
	KWSortableObject* dObject;
	Timer timer;
	int i;

	nSize = 1000;
	while (nSize < 200000000)
	{
		cout << "Size\t" << nSize << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nSize; i++)
		{
			dObject = new KWSortableObject;
			dObject->SetIndex(RandomInt(nValueNumber - 1));
			vector.Add(dObject);
		}
		timer.Stop();
		cout << "\tCreation, fill and sort\t" << timer.GetElapsedTime() << endl;
		timer.Reset();
		timer.Start();
		vector.DeleteAll();
		timer.Stop();
		cout << "\tReset\t" << timer.GetElapsedTime() << endl;
		nSize *= 2;
	}
}

void ComputePrimeNumbers()
{
	int i;
	longint lBase;
	longint lPrime;
	boolean bIsPrime;
	longint lFactor1;
	longint lFactor2;
	longint lFactorBound;

	// Recherche du premier nombre premuer suivant chaque puissance de 2
	i = 0;
	lBase = 1;
	while (lBase < LLONG_MAX / 2)
	{
		i += 1;
		lBase *= 2;

		// Recherche du premier nombre permier suivant la puissance de 2
		lPrime = lBase;
		bIsPrime = false;
		while (not bIsPrime)
		{
			lPrime++;

			// Test si le nombre est premier
			lFactorBound = (longint)ceil(sqrt(lPrime));
			bIsPrime = true;
			for (lFactor1 = 2; lFactor1 <= lFactorBound; lFactor1++)
			{
				lFactor2 = lPrime / lFactor1;
				if (lPrime == lFactor1 * lFactor2)
				{
					bIsPrime = false;
					break;
				}
			}

			// Arret si le nombre est premier
			if (bIsPrime)
				break;
		}

		// Affichage
		cout << lPrime << "," << endl;
	}
}

int ComputeHanoi(int n, char depart, char arrivee, char intermediaire)
{
	if (n == 0)
		return 1;
	else
		return ComputeHanoi(n - 1, depart, intermediaire, arrivee) +
		       ComputeHanoi(n - 1, intermediaire, arrivee, depart);
}

void StudyHanoi()
{
	int nMax = 30;
	int nIterNumber = 10;
	IntVector ivIndexes;
	DoubleVector dvTimes;
	LongintVector lvOperations;
	int i;
	int nIter;
	int nIndex;
	longint lOperations;
	Timer timer;

	// Initialisation des index
	ivIndexes.SetSize(nMax);
	dvTimes.SetSize(nMax);
	lvOperations.SetSize(nMax);
	for (i = 0; i < nMax; i++)
		ivIndexes.SetAt(i, i);

	// Iterations
	for (nIter = 0; nIter < nIterNumber; nIter++)
	{
		// Shuffle des indexes
		ivIndexes.Shuffle();

		// Calcul du probleme des tours de Hanoi
		for (i = 0; i < nMax; i++)
		{
			nIndex = ivIndexes.GetAt(i);
			timer.Reset();
			timer.Start();
			lOperations = ComputeHanoi(nIndex + 1, 'A', 'C', 'B');
			timer.Stop();
			dvTimes.UpgradeAt(nIndex, timer.GetElapsedTime());
			lvOperations.UpgradeAt(nIndex, lOperations);
		}
	}

	// Moyennage des temps
	for (i = 0; i < nMax; i++)
	{
		dvTimes.SetAt(i, dvTimes.GetAt(i) / nIter);
		lvOperations.SetAt(i, lvOperations.GetAt(i) / nIter);
	}

	// Affichage des resultats
	cout << "N\tRatio\tOperations\tTime\n";
	for (i = 1; i < nMax; i++)
		cout << i + 1 << "\t" << dvTimes.GetAt(i) / dvTimes.GetAt(i - 1) << "\t" << lvOperations.GetAt(i)
		     << "\t" << dvTimes.GetAt(i) << endl;
}