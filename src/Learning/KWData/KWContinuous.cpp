// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWContinuous.h"

// Puissances de 10
static const double dPositivePower10[] = {1.0,
					  10.0,
					  100.0,
					  1000.0,
					  10000.0,
					  100000.0,
					  1000000.0,
					  10000000.0,
					  100000000.0,
					  1000000000.0,
					  10000000000.0,
					  100000000000.0,
					  1000000000000.0,
					  10000000000000.0,
					  100000000000000.0,
					  1000000000000000.0,
					  10000000000000000.0,
					  100000000000000000.0,
					  1000000000000000000.0,
					  10000000000000000000.0,
					  1e20,
					  1e21,
					  1e22,
					  1e23,
					  1e24,
					  1e25,
					  1e26,
					  1e27,
					  1e28,
					  1e29,
					  1e30,
					  1e31,
					  1e32,
					  1e33,
					  1e34,
					  1e35,
					  1e36,
					  1e37,
					  1e38,
					  1e39,
					  1e40,
					  1e41,
					  1e42,
					  1e43,
					  1e44,
					  1e45,
					  1e46,
					  1e47,
					  1e48,
					  1e49,
					  1e50,
					  1e51,
					  1e52,
					  1e53,
					  1e54,
					  1e55,
					  1e56,
					  1e57,
					  1e58,
					  1e59,
					  1e60,
					  1e61,
					  1e62,
					  1e63,
					  1e64,
					  1e65,
					  1e66,
					  1e67,
					  1e68,
					  1e69,
					  1e70,
					  1e71,
					  1e72,
					  1e73,
					  1e74,
					  1e75,
					  1e76,
					  1e77,
					  1e78,
					  1e79,
					  1e80,
					  1e81,
					  1e82,
					  1e83,
					  1e84,
					  1e85,
					  1e86,
					  1e87,
					  1e88,
					  1e89,
					  1e90,
					  1e91,
					  1e92,
					  1e93,
					  1e94,
					  1e95,
					  1e96,
					  1e97,
					  1e98,
					  1e99,
					  1e100,
					  1e101,
					  1e102,
					  1e103,
					  1e104,
					  1e105,
					  1e106,
					  1e107,
					  1e108,
					  1e109,
					  1e110,
					  1e111,
					  1e112,
					  1e113,
					  1e114,
					  1e115,
					  1e116,
					  1e117,
					  1e118,
					  1e119,
					  1e120,
					  1e121,
					  1e122,
					  1e123,
					  1e124,
					  1e125,
					  1e126,
					  1e127,
					  1e128,
					  1e129,
					  1e130,
					  1e131,
					  1e132,
					  1e133,
					  1e134,
					  1e135,
					  1e136,
					  1e137,
					  1e138,
					  1e139,
					  1e140,
					  1e141,
					  1e142,
					  1e143,
					  1e144,
					  1e145,
					  1e146,
					  1e147,
					  1e148,
					  1e149,
					  1e150,
					  1e151,
					  1e152,
					  1e153,
					  1e154,
					  1e155,
					  1e156,
					  1e157,
					  1e158,
					  1e159,
					  1e160,
					  1e161,
					  1e162,
					  1e163,
					  1e164,
					  1e165,
					  1e166,
					  1e167,
					  1e168,
					  1e169,
					  1e170,
					  1e171,
					  1e172,
					  1e173,
					  1e174,
					  1e175,
					  1e176,
					  1e177,
					  1e178,
					  1e179,
					  1e180,
					  1e181,
					  1e182,
					  1e183,
					  1e184,
					  1e185,
					  1e186,
					  1e187,
					  1e188,
					  1e189,
					  1e190,
					  1e191,
					  1e192,
					  1e193,
					  1e194,
					  1e195,
					  1e196,
					  1e197,
					  1e198,
					  1e199,
					  1e200,
					  1e201,
					  1e202,
					  1e203,
					  1e204,
					  1e205,
					  1e206,
					  1e207,
					  1e208,
					  1e209,
					  1e210,
					  1e211,
					  1e212,
					  1e213,
					  1e214,
					  1e215,
					  1e216,
					  1e217,
					  1e218,
					  1e219,
					  1e220,
					  1e221,
					  1e222,
					  1e223,
					  1e224,
					  1e225,
					  1e226,
					  1e227,
					  1e228,
					  1e229,
					  1e230,
					  1e231,
					  1e232,
					  1e233,
					  1e234,
					  1e235,
					  1e236,
					  1e237,
					  1e238,
					  1e239,
					  1e240,
					  1e241,
					  1e242,
					  1e243,
					  1e244,
					  1e245,
					  1e246,
					  1e247,
					  1e248,
					  1e249,
					  1e250,
					  1e251,
					  1e252,
					  1e253,
					  1e254,
					  1e255,
					  1e256,
					  1e257,
					  1e258,
					  1e259,
					  1e260,
					  1e261,
					  1e262,
					  1e263,
					  1e264,
					  1e265,
					  1e266,
					  1e267,
					  1e268,
					  1e269,
					  1e270,
					  1e271,
					  1e272,
					  1e273,
					  1e274,
					  1e275,
					  1e276,
					  1e277,
					  1e278,
					  1e279,
					  1e280,
					  1e281,
					  1e282,
					  1e283,
					  1e284,
					  1e285,
					  1e286,
					  1e287,
					  1e288,
					  1e289,
					  1e290,
					  1e291,
					  1e292,
					  1e293,
					  1e294,
					  1e295,
					  1e296,
					  1e297,
					  1e298,
					  1e299,
					  1e300,
					  1e301,
					  1e302,
					  1e303,
					  1e304,
					  1e305,
					  1e306,
					  1e307,
					  1e308};
static const double dNegativePower10[] = {1,
					  0.1,
					  0.01,
					  0.001,
					  0.0001,
					  0.00001,
					  0.000001,
					  0.0000001,
					  0.00000001,
					  0.000000001,
					  0.0000000001,
					  0.00000000001,
					  0.000000000001,
					  0.0000000000001,
					  0.00000000000001,
					  0.000000000000001,
					  0.0000000000000001,
					  0.00000000000000001,
					  0.000000000000000001,
					  0.0000000000000000001,
					  1e-20,
					  1e-21,
					  1e-22,
					  1e-23,
					  1e-24,
					  1e-25,
					  1e-26,
					  1e-27,
					  1e-28,
					  1e-29,
					  1e-30,
					  1e-31,
					  1e-32,
					  1e-33,
					  1e-34,
					  1e-35,
					  1e-36,
					  1e-37,
					  1e-38,
					  1e-39,
					  1e-40,
					  1e-41,
					  1e-42,
					  1e-43,
					  1e-44,
					  1e-45,
					  1e-46,
					  1e-47,
					  1e-48,
					  1e-49,
					  1e-50,
					  1e-51,
					  1e-52,
					  1e-53,
					  1e-54,
					  1e-55,
					  1e-56,
					  1e-57,
					  1e-58,
					  1e-59,
					  1e-60,
					  1e-61,
					  1e-62,
					  1e-63,
					  1e-64,
					  1e-65,
					  1e-66,
					  1e-67,
					  1e-68,
					  1e-69,
					  1e-70,
					  1e-71,
					  1e-72,
					  1e-73,
					  1e-74,
					  1e-75,
					  1e-76,
					  1e-77,
					  1e-78,
					  1e-79,
					  1e-80,
					  1e-81,
					  1e-82,
					  1e-83,
					  1e-84,
					  1e-85,
					  1e-86,
					  1e-87,
					  1e-88,
					  1e-89,
					  1e-90,
					  1e-91,
					  1e-92,
					  1e-93,
					  1e-94,
					  1e-95,
					  1e-96,
					  1e-97,
					  1e-98,
					  1e-99,
					  1e-100,
					  1e-101,
					  1e-102,
					  1e-103,
					  1e-104,
					  1e-105,
					  1e-106,
					  1e-107,
					  1e-108,
					  1e-109,
					  1e-110,
					  1e-111,
					  1e-112,
					  1e-113,
					  1e-114,
					  1e-115,
					  1e-116,
					  1e-117,
					  1e-118,
					  1e-119,
					  1e-120,
					  1e-121,
					  1e-122,
					  1e-123,
					  1e-124,
					  1e-125,
					  1e-126,
					  1e-127,
					  1e-128,
					  1e-129,
					  1e-130,
					  1e-131,
					  1e-132,
					  1e-133,
					  1e-134,
					  1e-135,
					  1e-136,
					  1e-137,
					  1e-138,
					  1e-139,
					  1e-140,
					  1e-141,
					  1e-142,
					  1e-143,
					  1e-144,
					  1e-145,
					  1e-146,
					  1e-147,
					  1e-148,
					  1e-149,
					  1e-150,
					  1e-151,
					  1e-152,
					  1e-153,
					  1e-154,
					  1e-155,
					  1e-156,
					  1e-157,
					  1e-158,
					  1e-159,
					  1e-160,
					  1e-161,
					  1e-162,
					  1e-163,
					  1e-164,
					  1e-165,
					  1e-166,
					  1e-167,
					  1e-168,
					  1e-169,
					  1e-170,
					  1e-171,
					  1e-172,
					  1e-173,
					  1e-174,
					  1e-175,
					  1e-176,
					  1e-177,
					  1e-178,
					  1e-179,
					  1e-180,
					  1e-181,
					  1e-182,
					  1e-183,
					  1e-184,
					  1e-185,
					  1e-186,
					  1e-187,
					  1e-188,
					  1e-189,
					  1e-190,
					  1e-191,
					  1e-192,
					  1e-193,
					  1e-194,
					  1e-195,
					  1e-196,
					  1e-197,
					  1e-198,
					  1e-199,
					  1e-200,
					  1e-201,
					  1e-202,
					  1e-203,
					  1e-204,
					  1e-205,
					  1e-206,
					  1e-207,
					  1e-208,
					  1e-209,
					  1e-210,
					  1e-211,
					  1e-212,
					  1e-213,
					  1e-214,
					  1e-215,
					  1e-216,
					  1e-217,
					  1e-218,
					  1e-219,
					  1e-220,
					  1e-221,
					  1e-222,
					  1e-223,
					  1e-224,
					  1e-225,
					  1e-226,
					  1e-227,
					  1e-228,
					  1e-229,
					  1e-230,
					  1e-231,
					  1e-232,
					  1e-233,
					  1e-234,
					  1e-235,
					  1e-236,
					  1e-237,
					  1e-238,
					  1e-239,
					  1e-240,
					  1e-241,
					  1e-242,
					  1e-243,
					  1e-244,
					  1e-245,
					  1e-246,
					  1e-247,
					  1e-248,
					  1e-249,
					  1e-250,
					  1e-251,
					  1e-252,
					  1e-253,
					  1e-254,
					  1e-255,
					  1e-256,
					  1e-257,
					  1e-258,
					  1e-259,
					  1e-260,
					  1e-261,
					  1e-262,
					  1e-263,
					  1e-264,
					  1e-265,
					  1e-266,
					  1e-267,
					  1e-268,
					  1e-269,
					  1e-270,
					  1e-271,
					  1e-272,
					  1e-273,
					  1e-274,
					  1e-275,
					  1e-276,
					  1e-277,
					  1e-278,
					  1e-279,
					  1e-280,
					  1e-281,
					  1e-282,
					  1e-283,
					  1e-284,
					  1e-285,
					  1e-286,
					  1e-287,
					  1e-288,
					  1e-289,
					  1e-290,
					  1e-291,
					  1e-292,
					  1e-293,
					  1e-294,
					  1e-295,
					  1e-296,
					  1e-297,
					  1e-298,
					  1e-299,
					  1e-300,
					  1e-301,
					  1e-302,
					  1e-303,
					  1e-304,
					  1e-305,
					  1e-306,
					  1e-307,
					  1e-308};

inline int KWContinuous::ComputeExponent(Continuous cValue)
{
	int nExponent;
	Continuous cPower10;

	require(cValue > 0);

	// Recherche de l'exposant
	nExponent = 0;
	cPower10 = 1;
	if (cValue >= 1)
	{
		// On privilegie les petites puissance de 10
		while (cPower10 <= cValue and nExponent <= 10)
		{
			nExponent++;
			cPower10 = dPositivePower10[nExponent];
		}

		// On passe a une recherche dichotomique si necessaire
		if (cPower10 <= cValue)
		{
			int nLowerIndex;
			int nUpperIndex;

			// Initialisation des index extremites
			nLowerIndex = 10;
			nUpperIndex = 102;

			// Recherche dichotomique de l'intervalle
			nExponent = (nLowerIndex + nUpperIndex + 1) / 2;
			while (nLowerIndex + 1 < nUpperIndex)
			{
				cPower10 = dPositivePower10[nExponent];
				if (cPower10 <= cValue)
					nLowerIndex = nExponent;
				else
					nUpperIndex = nExponent;

				// Modification du prochain intervalle teste
				nExponent = (nLowerIndex + nUpperIndex + 1) / 2;
			}
			assert(nLowerIndex <= nUpperIndex);
			assert(nUpperIndex <= nLowerIndex + 1);
		}
		nExponent--;
	}
	else
	{
		// On privilegie les petites puissances de 10
		while (cPower10 > cValue and nExponent >= -10)
		{
			nExponent--;
			cPower10 = dNegativePower10[-nExponent];
		}

		// On passe a une recherche dichotomique si necessaire
		if (cPower10 > cValue)
		{
			int nLowerIndex;
			int nUpperIndex;

			// Initialisation des index extremites
			nLowerIndex = 10;
			nUpperIndex = 102;

			// Recherche dichotomique de l'intervalle
			nExponent = (nLowerIndex + nUpperIndex + 1) / 2;
			while (nLowerIndex + 1 < nUpperIndex)
			{
				cPower10 = dNegativePower10[nExponent];
				if (cPower10 > cValue)
					nLowerIndex = nExponent;
				else
					nUpperIndex = nExponent;

				// Modification du prochain intervalle teste
				nExponent = (nLowerIndex + nUpperIndex + 1) / 2;
			}
			nExponent = -nExponent;
			assert(nLowerIndex <= nUpperIndex);
			assert(nUpperIndex <= nLowerIndex + 1);
		}
	}
	assert(nExponent >= 100 or nExponent <= -100 or
	       (pow(10.0, nExponent) <= cValue * (1 + 1e-15) and cValue < pow(10.0, nExponent + 1) * (1 + 1e-15)));
	return nExponent;
}

///////////////////////////////////////////////////////////////////////////////////////////////

const char* const KWContinuous::ContinuousToString(Continuous cValue)
{
	char* sBuffer = StandardGetBuffer();

	if (cValue == KWContinuous::GetMissingValue())
	{
		sBuffer[0] = '\0';
	}
	else if (cValue == 0)
	{
		sBuffer[0] = '0';
		sBuffer[1] = '\0';
	}
#ifdef _WIN32
	else if (_isnan(cValue))
#else
	else if (isnan(cValue))
#endif
	{
		sBuffer[0] = 'N';
		sBuffer[1] = 'a';
		sBuffer[2] = 'N';
		sBuffer[3] = '\0';
	}
	else
	{
		const int nStartMantissa = 20;
		int nEndMantissa;
		int nOffset = 0;
		int nExponent;
		double dMantissa;
		longint lLongMantissa;
		int nMantissa;
		int i;
		int nPointOffset;

		// Traitement des nombre negatifs
		if (cValue < 0)
		{
			sBuffer[0] = '-';
			nOffset++;
			cValue = -cValue;
		}
		else
			sBuffer[0] = '\0';

		// Recherche de l'exposant
		nExponent = ComputeExponent(cValue);

		// Cas des valeurs trop grandes
		if (nExponent >= 100)
		{
			if (sBuffer[0] == '-')
			{
				sBuffer[1] = '1';
				sBuffer[2] = 'e';
				sBuffer[3] = '+';
				sBuffer[4] = '1';
				sBuffer[5] = '0';
				sBuffer[6] = '0';
				sBuffer[7] = '\0';
			}
			else
			{
				sBuffer[0] = '1';
				sBuffer[1] = 'e';
				sBuffer[2] = '+';
				sBuffer[3] = '1';
				sBuffer[4] = '0';
				sBuffer[5] = '0';
				sBuffer[6] = '\0';
			}
		}
		// Cas des valeurs trop petites
		else if (nExponent < -100)
		{
			sBuffer[0] = '0';
			sBuffer[1] = '\0';
		}
		// Cas standard
		else
		{
			// Normalisation de la precision de la mantisse
			// On normalise par la puissance de l'exposant avant prendre en compte les decimales
			// selon la precision souhaitee
			if (nExponent >= GetDigitNumber() - 1)
				dMantissa = cValue * dNegativePower10[nExponent - GetDigitNumber() + 1];
			else
				dMantissa = cValue * dPositivePower10[-nExponent + GetDigitNumber() - 1];

			// On doit passer par un longint pour avoir 10 decimales
			lLongMantissa = (longint)(dMantissa + 0.5);

			// Correction de l'exposant si necessaire
			if (lLongMantissa >= 10000000000.0)
			{
				nExponent++;
				lLongMantissa /= 10;
			}

			// Ecriture de la mantisse a l'envers, plus loin dans le buffer
			// On repasse par des int, plus efficace pour les calcul,
			// en traitant d'abord la premiere decimale, puis les suivantes
			i = nStartMantissa;
			sBuffer[i] = '0' + lLongMantissa % 10;
			i++;
			assert(lLongMantissa / 10 <= 1000000000.0);
			nMantissa = (int)(lLongMantissa / 10);
			do
			{
				sBuffer[i] = '0' + nMantissa % 10;
				i++;
				nMantissa /= 10;
			} while (nMantissa > 0);
			nEndMantissa = i - 1;

			// Inversion de l'ecriture de la mantisse pour un petit exposant negatif
			if (-4 <= nExponent and nExponent < 0)
			{
				// Header '0.'
				sBuffer[nOffset] = '0';
				nOffset++;
				sBuffer[nOffset] = '.';
				nOffset++;

				// Ecriture des zero du debut de la mantisse
				for (i = nExponent + 1; i < 0; i++)
				{
					sBuffer[nOffset] = '0';
					nOffset++;
				}

				// Ecriture du reste de la mantisse
				for (i = 0; i < GetDigitNumber(); i++)
				{
					sBuffer[nOffset] = sBuffer[nEndMantissa - i];
					nOffset++;
				}

				// Supression des zeros de fin
				while (sBuffer[nOffset - 1] == '0')
					nOffset--;
				assert(sBuffer[nOffset - 1] != '.');

				// Fin du buffer
				sBuffer[nOffset] = '\0';
			}
			// Inversion de l'ecriture de la mantisse pour un petit exposant positif
			else if (0 <= nExponent and nExponent < GetDigitNumber())
			{
				// Ecriture de la mantisse, avec un point decimal
				nPointOffset = nEndMantissa - nExponent - 1;
				for (i = nEndMantissa; i >= nStartMantissa; i--)
				{
					// Insertion du point
					if (i == nPointOffset)
					{
						sBuffer[nOffset] = '.';
						nOffset++;

						// On passe a -1 pour indiquer qu'un point a etet ecrit
						nPointOffset = -1;
					}

					// Ajout d'une decimale
					sBuffer[nOffset] = sBuffer[i];
					nOffset++;
				}

				// Supression des zeros de fin
				if (nPointOffset == -1)
				{
					while (sBuffer[nOffset - 1] == '0')
						nOffset--;
					if (sBuffer[nOffset - 1] == '.')
						nOffset--;
				}

				// Fin du buffer
				sBuffer[nOffset] = '\0';
			}
			// Inversion de l'ecriture de la mantisse dans les cas avec exposant
			else
			{
				// Premier chifre de la mantisse, suivi d'un '.'
				sBuffer[nOffset] = sBuffer[nEndMantissa];
				nOffset++;
				sBuffer[nOffset] = '.';
				nOffset++;

				// Ecriture du reste de la mantisse
				for (i = nEndMantissa - 1; i >= nStartMantissa; i--)
				{
					sBuffer[nOffset] = sBuffer[i];
					nOffset++;
				}

				// Supression des zeros de fin
				while (sBuffer[nOffset - 1] == '0')
					nOffset--;
				if (sBuffer[nOffset - 1] == '.')
					nOffset--;

				// Ajout de l'exposant
				sBuffer[nOffset] = 'e';
				nOffset++;

				// Ecriture de l'exposant
				if (nExponent < 0)
				{
					sBuffer[nOffset] = '-';
					nOffset++;
					nExponent = -nExponent;
				}
				else
				{
					sBuffer[nOffset] = '+';
					nOffset++;
				}
				assert(nExponent < 200);
				if (nExponent >= 100)
				{
					sBuffer[nOffset] = '1';
					nOffset++;
				}
				sBuffer[nOffset] = '0' + (nExponent % 100) / 10;
				nOffset++;
				sBuffer[nOffset] = '0' + (nExponent % 10);
				nOffset++;

				// Fin du buffer
				sBuffer[nOffset] = '\0';
			}
		}
	}
	return sBuffer;
}

Continuous KWContinuous::DoubleToContinuous(double dValue)
{
	Continuous cValue;
	double dPositiveValue;
	int nExponent;
	int nPower10Index;
	double dMantissa;
	longint lMantissa;

	// Traitement des valeurs manquantes
	if (dValue == -HUGE_VAL or dValue == HUGE_VAL)
	{
		cValue = GetMissingValue();
	}
	// Cas d'une valeur nulle
	else if (dValue == 0)
	{
		cValue = 0;
	}
	// Cas standard
	else
	{
		// Passage par une valeur positive
		if (dValue > 0)
			dPositiveValue = dValue;
		else
			dPositiveValue = -dValue;

		// Recherche de l'exposant
		nExponent = ComputeExponent(dPositiveValue);

		// Cas des valeurs trop grandes
		if (nExponent >= 100)
		{
			if (dValue < 0)
				cValue = GetMinValue();
			else
				cValue = GetMaxValue();
		}
		// Cas des valeurs trop petites
		else if (nExponent < -100)
			cValue = 0;
		// Cas standard
		else
		{
			// Normalisation de la precision de la mantisse
			// On normalise par la puissance de l'exposant avant prendre en compte les decimales
			// selon la precision souhaitee
			nPower10Index = nExponent - GetDigitNumber() + 1;
			if (nPower10Index >= 0)
			{
				dMantissa = dPositiveValue * dNegativePower10[nPower10Index];
				lMantissa = (longint)(dMantissa + 0.5);

				// Gestion de l'effet de bord ou la mantissa a gagne un chiffre supplementaire
				assert(0 <= lMantissa and lMantissa <= 10000000000);
				if (lMantissa == 10000000000)
				{
					lMantissa = 1000000000;
					nPower10Index++;
				}

				// Conversion vers un format standardise
				cValue = lMantissa * dPositivePower10[nPower10Index];
			}
			else
			{
				nPower10Index = -nPower10Index;
				dMantissa = dPositiveValue * dPositivePower10[nPower10Index];
				lMantissa = (longint)(dMantissa + 0.5);

				// Gestion de l'effet de bord ou la mantissa a gagne un chiffre supplementaire
				assert(0 <= lMantissa and lMantissa <= 10000000000);
				if (lMantissa == 10000000000)
				{
					lMantissa = 1000000000;
					nPower10Index--;
				}

				// Conversion vers un format standardise
				cValue = lMantissa * dNegativePower10[nPower10Index];
			}

			// Correction si negatif
			if (dValue < 0)
				cValue = -cValue;
			assert(GetMinValue() <= cValue and cValue <= GetMaxValue());
			assert(fabs(cValue - dValue) < 1e-5 * fabs(dValue));
		}
	}
	return cValue;
}

int KWContinuous::StringToContinuousError(const char* const sValue, Continuous& cValue)
{
	boolean bNegative;
	boolean bNegativeExponent;
	boolean bUsedZeros;
	boolean bMissingExponent;
	boolean bRounded;
	double dMantissa;
	int nExponent;
	int nMantissaExponent;
	int nDigitNumber;
	int nOffset;

	///////////////////////////////////////////////////////////
	// Parsing d'une valeur numerique de la forme
	//   [whitespace] [sign] [digits] [.digits] [ {e | E}[sign]digits]
	//
	// On ne memorise que les chiffres significatifs de la mantisse (dMantissa) pour gagner du temps.
	// Le nombre de chifres de la mantisse est memorise (nDigitNumber).
	// En supposant un chiffre avant la virgule, l'exposant de la mantisse est egalement memorise
	// (nMantissaExponent) en tenant compte de la position initiale de la virgule Les zeros en tete sont ainsi
	// ignores (sauf pour memoriser qu'au moins un chiffre est present (bUsedZeros) L'exposant final est corrige
	// avec le l'exposant de la mantisse, et le calcul du resultat est optimise a l'aide des tableaux de puissance
	// de 10.

	// Cas de la valeur nulle
	if (sValue[0] == '\0')
	{
		cValue = KWContinuous::GetMissingValue();
		assert(cValue < KWContinuous::GetMinValue());
		return KWContinuous::NoError;
	}
	else
	{
		/////////////////////////////////////////////////////////////////
		// Parsing de la chaine de caracteres

		// On saute les blancs initiaux
		nOffset = 0;
		while (isspace(sValue[nOffset]))
			nOffset++;

		// Analyse du signe optionnel
		bNegative = false;
		if (sValue[nOffset] == '+')
			nOffset++;
		else if (sValue[nOffset] == '-')
		{
			bNegative = true;
			nOffset++;
		}

		// On ignore les 0 initiaux
		bUsedZeros = false;
		while (sValue[nOffset] == '0')
		{
			bUsedZeros = true;
			nOffset++;
		}

		// Analyse des chiffres avant la virgule
		nDigitNumber = 0;
		nMantissaExponent = -1;
		dMantissa = 0;
		bRounded = false;
		while (isdigit(sValue[nOffset]))
		{
			// On ne prend en compte que les premier chiffres significatifs
			if (nDigitNumber < KWContinuous::GetDigitNumber())
			{
				dMantissa = dMantissa * 10 + (sValue[nOffset] - '0');
				nDigitNumber++;
				nMantissaExponent++;
			}
			// Arrondi a l'unite superieure selon chiffre suivant
			else if (not bRounded)
			{
				if (sValue[nOffset] - '0' >= 5)
					dMantissa++;
				nMantissaExponent++;
				bRounded = true;
			}
			// On memorise le decalage "virtuel" de la virgule, du aux chiffres ignores
			else
				nMantissaExponent++;
			nOffset++;
		}

		// Analyse de la partie decimale optionnelle
		if (sValue[nOffset] == '.')
		{
			nOffset++;

			// Si aucun chiffre avant la virgule, on saute les 0 initiaux
			if (nDigitNumber == 0)
			{
				while (sValue[nOffset] == '0')
				{
					bUsedZeros = true;
					nMantissaExponent--;
					nOffset++;
				}
			}

			// Prise en compte de la partie decimale
			while (isdigit(sValue[nOffset]))
			{
				// On ne prend en compte que les premier chiffres significatifs
				if (nDigitNumber < KWContinuous::GetDigitNumber())
				{
					dMantissa = dMantissa * 10 + (sValue[nOffset] - '0');
					nDigitNumber++;
				}
				// Arrondi a l'unite superieure selon chiffre suivant
				else if (not bRounded)
				{
					if (sValue[nOffset] - '0' >= 5)
						dMantissa++;
					bRounded = true;
				}
				nOffset++;
			}
		}

		// Analyse de l'exposant optionnel
		nExponent = 0;
		bMissingExponent = false;
		if (sValue[nOffset] == 'e' or sValue[nOffset] == 'E')
		{
			nOffset++;

			// Analyse du signe de l'exposant optionnel
			bNegativeExponent = false;
			if (sValue[nOffset] == '+')
				nOffset++;
			else if (sValue[nOffset] == '-')
			{
				bNegativeExponent = true;
				nOffset++;
			}

			// Analyse de l'exposant
			bMissingExponent = true;
			while (isdigit(sValue[nOffset]))
			{
				// On ne prend pas en compte les exposants trop grand (inutiles)
				// pour eviter les debordements arithmetiques du stockage de l'exposant
				if (nExponent < 1000)
				{
					nExponent = nExponent * 10 + (sValue[nOffset] - '0');
					bMissingExponent = false;
				}
				nOffset++;
			}

			// Prise en compte du signe de l'exposant
			if (bNegativeExponent)
				nExponent = -nExponent;
		}

		// Correction de l'exposant par l'exposant de la mantisse
		nExponent += nMantissaExponent;

		////////////////////////////////////////////////////////////////
		// Analyse des resultats de parsing

		// S'il reste des caracteres n'ayant pu etre converti,
		// c'est qu'il y a un probleme de conversion, et on renvoie la valeur manquante
		if (sValue[nOffset] != '\0')
		{
			cValue = KWContinuous::GetMissingValue();
			if (nOffset == 0)
				return KWContinuous::ErrorBadString;
			else
				return KWContinuous::ErrorBadEndString;
		}
		// Erreur si on a trouve aucun chiffre, ou un exposant manquant
		else if ((nDigitNumber == 0 and not bUsedZeros) or bMissingExponent)
		{
			cValue = KWContinuous::GetMissingValue();
			return KWContinuous::ErrorBadString;
		}
		// Cas de conversion reussie
		else
		{
			// Traitement des valeurs extremes
			if (nExponent >= 100 and (nExponent > 100 or dMantissa > 1))
			{
				if (bNegative)
				{
					cValue = KWContinuous::GetMinValue();
					return KWContinuous::ErrorNegativeOverflow;
				}
				else
				{
					cValue = KWContinuous::GetMaxValue();
					return KWContinuous::ErrorPositiveOverflow;
				}
			}
			else if (dMantissa != 0 and nExponent < -100)
			{
				cValue = 0;
				return KWContinuous::ErrorUnderflow;
			}
			// Cas standard
			else
			{
				// Normalisation de la precision de la mantisse (cf. DoubleToContinuous())
				// On normalise par la puissance de l'exposant avant prendre en compte les decimales
				// selon la precision souhaitee
				if (nDigitNumber >= GetDigitNumber())
					dMantissa = dMantissa * dNegativePower10[nDigitNumber - GetDigitNumber()];
				else
					dMantissa = dMantissa * dPositivePower10[-nDigitNumber + GetDigitNumber()];
				if (nExponent >= GetDigitNumber() - 1)
					cValue = ((longint)(dMantissa + 0.5)) *
						 dPositivePower10[nExponent - GetDigitNumber() + 1];
				else
					cValue = ((longint)(dMantissa + 0.5)) *
						 dNegativePower10[-nExponent + GetDigitNumber() - 1];

				// Correction si negatif
				if (bNegative)
					cValue = -cValue;

				// Retour
				assert(sValue[nOffset] == '\0');
				assert(cValue != -HUGE_VAL);
				assert(cValue != HUGE_VAL);
				assert(KWContinuous::GetMinValue() <= cValue);
				assert(cValue <= KWContinuous::GetMaxValue());
				return KWContinuous::NoError;
			}
		}
	}
}

const ALString KWContinuous::ErrorLabel(int nError)
{
	ALString sErrorLabel;

	require(CheckError(nError));

	if (nError == ErrorUnderflow)
		sErrorLabel = "Underflow";
	else if (nError == ErrorNegativeOverflow)
		sErrorLabel = "Overflow -inf";
	else if (nError == ErrorPositiveOverflow)
		sErrorLabel = "Overflow +inf";
	else if (nError == ErrorBadString)
		sErrorLabel = "Unconverted string";
	else if (nError == ErrorBadEndString)
		sErrorLabel = "Unconverted end of string";
	else
		sErrorLabel = "Conversion OK";
	return sErrorLabel;
}

Continuous KWContinuous::GetHumanReadableLowerMeanValue(Continuous cValue1, Continuous cValue2)
{
	Continuous cMeanValue;
	Continuous cLowerValue;
	Continuous cUpperValue;
	double dLowerMeanValue;
	double dUpperMeanValue;

	// Si l'une des valeurs est manquante, on renvoie la valeur manquante
	if (cValue1 == GetMissingValue() or cValue2 == GetMissingValue())
		return GetMissingValue();
	// Sinon, calcul de moyenne facile a lire, a 10% de la vraie moyenne
	else
	{
		// On determine la valeur inf et sup
		if (cValue1 <= cValue2)
		{
			cLowerValue = cValue1;
			cUpperValue = cValue2;
		}
		else
		{
			cLowerValue = cValue2;
			cUpperValue = cValue1;
		}

		// Encadrement tolere (a 10% pres) pour une approximation lisible
		dLowerMeanValue = (11 * cLowerValue + 9 * cUpperValue) / 20;
		dUpperMeanValue = (9 * cLowerValue + 11 * cUpperValue) / 20;

		// Effet de bord si aux limites de la precision numerique
		if (dLowerMeanValue < cLowerValue)
			dLowerMeanValue = cLowerValue;
		if (dUpperMeanValue > cUpperValue)
			dUpperMeanValue = cUpperValue;

		// Recherche de la valeur la plus simple possible
		cMeanValue = ComputeHumanReadableContinuous(dLowerMeanValue, dUpperMeanValue);

		// Effet de bord quand la moyenne est egale a l'une des valeurs
		if (cMeanValue <= cLowerValue or cMeanValue >= cUpperValue)
			return cLowerValue;
		else
			return cMeanValue;
	}
}

Continuous KWContinuous::GetHumanReadableUpperMeanValue(Continuous cValue1, Continuous cValue2)
{
	Continuous cMeanValue;
	Continuous cLowerValue;
	Continuous cUpperValue;
	double dLowerMeanValue;
	double dUpperMeanValue;

	// Si les deux valeurs sont manquantes, on renvoie la valeur manquante
	if (cValue1 == GetMissingValue() and cValue2 == GetMissingValue())
		return GetMissingValue();
	// Si l'une des valeurs est manquante, on renvoie la valeur minimale
	else if (cValue1 == GetMissingValue() or cValue2 == GetMissingValue())
		return GetMinValue();
	// Sinon, calcul de moyenne facile a lire, a 10% de la vraie moyenne
	else
	{
		// On determine la valeur inf et sup
		if (cValue1 <= cValue2)
		{
			cLowerValue = cValue1;
			cUpperValue = cValue2;
		}
		else
		{
			cLowerValue = cValue2;
			cUpperValue = cValue1;
		}

		// Encadrement tolere (a 10% pres) pour une approximation lisible
		dLowerMeanValue = (11 * cLowerValue + 9 * cUpperValue) / 20;
		dUpperMeanValue = (9 * cLowerValue + 11 * cUpperValue) / 20;

		// Effet de bord si aux limites de la precision numerique
		if (dLowerMeanValue < cLowerValue)
			dLowerMeanValue = cLowerValue;
		if (dUpperMeanValue > cUpperValue)
			dUpperMeanValue = cUpperValue;

		// Recherche de la valeur la plus simple possible
		cMeanValue = ComputeHumanReadableContinuous(dLowerMeanValue, dUpperMeanValue);

		// Effet de bord quand la moyenne est egale a l'une des valeurs
		if (cMeanValue <= cLowerValue or cMeanValue >= cUpperValue)
			return cUpperValue;
		else
			return cMeanValue;
	}
}

Continuous KWContinuous::Acquire(const char* const sLabel, Continuous cDefaultValue)
{
	return (Continuous)AcquireDouble(sLabel, (double)cDefaultValue);
}

void KWContinuous::Test()
{
	ALString sContinuous;

	// Valeurs extremes
	cout << "Special values (inf and nan values differ from Windows to Linux)\n";
	cout << "DigitNumber\t" << GetDigitNumber() << endl;
	cout << "MinValue\t" << GetMinValue() << endl;
	cout << "MaxValue\t" << GetMaxValue() << endl;
	cout << "EpsilonValue\t" << GetEpsilonValue() << endl;
	cout << "MissingValue\t" << GetMissingValue() << endl;
	cout << "ForbiddenValue\t" << GetForbiddenValue() << endl;
	cout << "-HUGE_VAL\t" << -HUGE_VAL << endl;
	cout << "HUGE_VAL\t" << HUGE_VAL << endl;
	cout << "DBL_MIN\t" << DBL_MIN << endl;
	cout << "DBL_MAX\t" << DBL_MAX << endl;
	cout << "NaN\t" << atof("NaN") << " -> " << StringToContinuous("NaN") << endl;
	cout << "1/3\t" << 1.0 / 3 << " -> " << ContinuousToString(1.0 / 3) << endl;

	// Tests de conversion dans les deux sens
	TestConversion();

	// Test intensif de conversion String vers de Continuous
	TestStringToContinuous();

	// Test intensif de conversion de Continuous vers string
	TestContinuousToString();
}

///////////////////////////////////////////////////////////////////////////////////////////////

Continuous KWContinuous::ComputeHumanReadableContinuous(double dLowerValue, double dUpperValue)
{
	double dValue;
	double dMaxDeltaValue;
	Continuous cNewValue;
	Continuous cValue;
	double dPositiveValue;
	int nExponent;
	int nFullExponent;
	double dMantissa;
	longint lMantissa;

	require(dLowerValue <= dUpperValue);

	// Calcul de la valeur moyenne
	dValue = (dLowerValue + dUpperValue) / 2;

	// Calcul de l'ecart max a la moyenne
	dMaxDeltaValue = (dUpperValue - dLowerValue) / 2;

	// Traitement des valeurs manquantes
	if (dValue == -HUGE_VAL or dValue == HUGE_VAL)
	{
		cValue = GetMissingValue();
	}
	// Cas d'une valeur nulle
	else if (dValue == 0)
	{
		cValue = 0;
	}
	// Cas standard
	else
	{
		// Passage par une valeur positive
		if (dValue > 0)
			dPositiveValue = dValue;
		else
			dPositiveValue = -dValue;

		// Recherche de l'exposant
		nExponent = ComputeExponent(dPositiveValue);

		// Cas des valeurs trop grandes
		if (nExponent >= 100)
		{
			if (dValue < 0)
				cValue = GetMinValue();
			else
				cValue = GetMaxValue();
		}
		// Cas des valeurs trop petites
		else if (nExponent < -100)
			cValue = 0;
		// Cas standard
		else
		{
			// Normalisation de la precision de la mantisse
			// On normalise par la puissance de l'exposant avant prendre en compte les decimales
			// selon la precision souhaitee
			nFullExponent = nExponent - GetDigitNumber() + 1;
			if (nFullExponent >= 0)
			{
				dMantissa = dPositiveValue * dNegativePower10[nFullExponent];
				lMantissa = (longint)(dMantissa + 0.5);
				cNewValue = lMantissa * dPositivePower10[nFullExponent];

				// Reduction de la mantisse tant que l'on ne depasse pas l'ecart autorise
				cValue = cNewValue;
				while (fabs(cNewValue - dPositiveValue) < dMaxDeltaValue and nFullExponent < 200)
				{
					cValue = cNewValue;
					lMantissa /= 10;
					nFullExponent += 1;
					assert(0 <= nFullExponent and nFullExponent <= 200);
					cNewValue = lMantissa * dPositivePower10[nFullExponent];
				}
			}
			else
			{
				dMantissa = dPositiveValue * dPositivePower10[-nFullExponent];
				lMantissa = (longint)(dMantissa + 0.5);
				cNewValue = lMantissa * dNegativePower10[-nFullExponent];

				// Reduction de la mantisse tant que l'on ne depasse pas l'ecart autorise
				cValue = cNewValue;
				while (fabs(cNewValue - dPositiveValue) < dMaxDeltaValue and nFullExponent < 200)
				{
					cValue = cNewValue;
					lMantissa /= 10;
					nFullExponent += 1;
					assert(nFullExponent >= -200);
					if (nFullExponent <= 0)
						cNewValue = lMantissa * dNegativePower10[-nFullExponent];
					else
						cNewValue = lMantissa * dPositivePower10[nFullExponent];
				}
			}

			// Correction si negatif
			if (dValue < 0)
				cValue = -cValue;

			// Correction si on est sorti des bornes (possible aux limites)
			assert(fabs(cValue - dValue) <= 11 * dMaxDeltaValue);
			if (cValue < dLowerValue)
				cValue = dLowerValue;
			if (cValue > dUpperValue)
				cValue = dUpperValue;
			assert(GetMinValue() <= cValue and cValue <= GetMaxValue());
			assert(fabs(cValue - dValue) <= 10 * dMaxDeltaValue);
		}
	}

	// On normalise la representation Continuous de la valeur obtenue
	// pour avoir la meme valeur en memoire et suite a ecriture-relecture dans un fichier
	cValue = DoubleToContinuous(cValue);

	return cValue;
}

const char* const KWContinuous::StandardContinuousToString(Continuous cValue)
{
	char* sBuffer = StandardGetBuffer();

	// Precision de 10, inferieure a la precision potentielle de 10, mais conservant
	// un bon comportement de coherence entre valeur interne et representation externe
	// La valeur manquante est convertie en chaine vide
	if (cValue == GetMissingValue())
		sBuffer[0] = '\0';
	else
		sprintf(sBuffer, "%.10g", cValue);
	return sBuffer;
}

inline Continuous KWContinuous::StandardStringToContinuous(const char* const sValue)
{
	Continuous cValue;
	StandardStringToContinuousError(sValue, cValue);
	return cValue;
}

int KWContinuous::StandardStringToContinuousError(const char* const sValue, Continuous& cValue)
{
	char* endptr;
	double dValue;

	require(sValue != NULL);

	// Cas de la valeur manquante
	if (sValue[0] == '\0')
	{
		cValue = GetMissingValue();
		assert(cValue < GetMinValue());
		return NoError;
	}
	// Cas general
	else
	{
		// Conversion
		dValue = strtod(sValue, &endptr);

		// S'il reste des caracteres n'ayant pu etre converti,
		// c'est qu'il y a un probleme de conversion, et on renvoie la valeur manquante
		if (endptr[0] != '\0')
		{
			cValue = GetMissingValue();
			if (strlen(sValue) == strlen(endptr))
				return ErrorBadString;
			else
				return ErrorBadEndString;
		}
		// Cas du NaN (not a number): traite comme missing
#ifdef _WIN32
		else if (_isnan(dValue))
#else
		else if (isnan(dValue))
#endif
		{
			cValue = GetMissingValue();
			assert(cValue < GetMinValue());
			return NoError;
		}
		// Cas de conversion reussie
		else
		{
			// On convertit explicitement en Continuous pour s'assurer de la
			// coherence entre representation interne et representation externe
			cValue = DoubleToContinuous(dValue);

			// Traitement des valeurs extremes
			if (dValue == -HUGE_VAL or dValue < GetMinValue())
			{
				cValue = GetMinValue();
				return ErrorNegativeOverflow;
			}
			else if (dValue == HUGE_VAL or dValue > GetMaxValue())
			{
				cValue = GetMaxValue();
				return ErrorPositiveOverflow;
			}
			else if (dValue != 0 and fabs(dValue) < GetEpsilonValue())
			{
				cValue = 0;
				return ErrorUnderflow;
			}
			// Cas standard
			else
			{
				assert(endptr[0] == '\0');
				assert(cValue != -HUGE_VAL);
				assert(cValue != HUGE_VAL);
				assert(GetMinValue() <= cValue);
				assert(cValue <= GetMaxValue());
				return NoError;
			}
		}
	}
}

void KWContinuous::Conversion(const char* const sValue)
{
	Continuous cValue;
	Continuous cConvertedValue;
	Continuous cMeanValue;
	Continuous cUpperValue;

	cout << sValue << "\t";
	//
	cConvertedValue = StringToContinuous(sValue);
	if (cConvertedValue == KWContinuous::GetMissingValue())
		cout << "Missing\t";
	else
		cout << cConvertedValue << "\t";
	//
	if (StringToContinuousError(sValue, cValue) != NoError)
		cout << "KO";
	cout << "\t";
	//
	cout << ErrorLabel(StringToContinuousError(sValue, cValue)) << "\t";
	//
	cout << IsStringContinuous(sValue) << "\t";
	//
	cValue = StringToContinuous(sValue);
	cout << ContinuousToString(cValue) << "\t";
	cout << flush;
	//
	if (cValue == KWContinuous::GetMissingValue())
		cout << "Missing\t";
	else
		cout << cValue << "\t";
	//
	cout << ContinuousToString(GetLowerMeanValue(cValue, 1)) << "\t";
	cout << Compare(cValue, 1) << "\t";
	cout << (cValue == 1) << "\t";
	cout << Compare(cValue, GetMissingValue()) << "\t";
	cout << (cValue == GetMissingValue()) << "\t";
	//
	cUpperValue = 3.141592653589793;
	//
	cMeanValue = GetLowerMeanValue(cValue, cUpperValue);
	cout << ContinuousToString(cMeanValue) << "\t";
	//
	cMeanValue = GetUpperMeanValue(cValue, cUpperValue);
	cout << ContinuousToString(cMeanValue) << "\t";
	//
	cMeanValue = GetHumanReadableLowerMeanValue(cValue, cUpperValue);
	cout << ContinuousToString(cMeanValue) << "\t";
	//
	cMeanValue = GetHumanReadableUpperMeanValue(cValue, cUpperValue);
	cout << ContinuousToString(cMeanValue) << "\n";
}

void KWContinuous::TestConversion()
{
	// Tests de conversion
	cout << "Test de conversion\n";
	cout << "String\tNumerical\tError\tError label\tValid\tStoCtoS\tatof(str)\tLowerMean(val,1)\t";
	cout << "compare(val,1)\tval==1\tcompare(val,Missing)\tval==Missing\t";
	cout << "LMV((val, Missing)\tUMV((val, Missing)\tHRLMV((val, Missing)\tHRUMV((val, Missing)\n";
	Conversion("0.9");
	Conversion("0.99");
	Conversion("0.999");
	Conversion("0.9999");
	Conversion("0.99999");
	Conversion("0.999999");
	Conversion("0.9999991");
	Conversion("0.9999995");
	Conversion("0.9999999");
	Conversion("0.99999991");
	Conversion("0.99999999");
	Conversion("0.999999999");
	Conversion("0.9999999999");
	Conversion("0.99999999999");
	Conversion("0.999999999999");
	Conversion("0.9999999999999");
	Conversion("0.99999999999999");
	Conversion("0.999999999999999");
	Conversion("0.9999999999999999");
	Conversion("0.99999999999999999");
	Conversion("1.0");
	Conversion("1.0000000000000001");
	Conversion("1.000000000000001");
	Conversion("1.00000000000001");
	Conversion("1.0000000000001");
	Conversion("1.000000000001");
	Conversion("1.00000000001");
	Conversion("1.0000000001");
	Conversion("1.000000001");
	Conversion("1.00000001");
	Conversion("1.0000001");
	Conversion("1.000001");
	Conversion("1.000005");
	Conversion("1.000009");
	Conversion("1.00001");
	Conversion("1.0001");
	Conversion("1.001");
	Conversion("1.01");
	Conversion("1.1");
	Conversion(".9999999999");
	Conversion("9.999999999");
	Conversion("99.99999999");
	Conversion("999.9999999");
	Conversion("9999.999999");
	Conversion("99999.99999");
	Conversion("999999.9999");
	Conversion("9999999.999");
	Conversion("99999999.99");
	Conversion("999999999.9");
	Conversion("9999999999.");
	Conversion("99999999999");
	Conversion("123456");
	Conversion("1234567");
	Conversion("12345678");
	Conversion("3.14");
	Conversion("3.141592653");
	Conversion("3.14159265358979323846");
	Conversion("10");
	Conversion("-3.14");
	Conversion("-3.141592653");
	Conversion("-3.14159265358979323846");
	Conversion("-10");
	Conversion("10.1");
	Conversion("10,1");
	Conversion(" -10.1e2");
	Conversion(" - 10.1 e-2");
	Conversion("10A2");
	Conversion("a10");
	Conversion("1e300");
	Conversion("1e300A");
	Conversion("1e-300");
	Conversion("1e400");
	Conversion("1e400A");
	Conversion("-1e400");
	Conversion("1e-400");
	Conversion("1e30");
	Conversion("-1e30");
	Conversion("1e-30");
	Conversion("-1e-30");
	Conversion("");
	Conversion("-");
	Conversion("-0");
	Conversion("0.");
	Conversion(".0");
	Conversion(" +10");
	Conversion("null");
	Conversion("1.#QNAN");
	Conversion("-1.#QNAN");
	Conversion("-1.#IND");
	Conversion("1.#IND");
	Conversion("-1.#INF");
	Conversion("1.#INF");
	Conversion("1e-37");
	Conversion("1e-38");
	Conversion("1e-39");
	Conversion("1e-40");
	Conversion("1e-41");
	Conversion("1e-42");
	Conversion("1e-43");
	Conversion("1e-44");
	Conversion("1e-45");
	Conversion("1e-46");
	Conversion("1e-47");
	Conversion("1e-48");
	Conversion("1e-49");
	Conversion("1e-50");
	Conversion("1e-97");
	Conversion("1e-98");
	Conversion("1e-99");
	Conversion("1e-100");
	Conversion("1e-101");
	Conversion("1e-102");
	Conversion("1e97");
	Conversion("1e98");
	Conversion("1e99");
	Conversion("1e100");
	Conversion("1e101");
	Conversion("1e102");
	Conversion("1e300");
	Conversion("1e301");
	Conversion("1e302");
	Conversion("1e303");
	Conversion("1e304");
	Conversion("1e305");
	Conversion("1e306");
	Conversion("1e307");
	Conversion("1e308");
	Conversion("1e309");
	Conversion("1e310");
}

void KWContinuous::CompareStringToContinuous(Continuous cValue, boolean bShow)
{
	const double dThreshold = 1.0000002e-9;
	char sValue[100];
	double dRefValue;
	double dNewValue;
	double dDiff;

	// Pas de controle dans les cas extremes
	if (fabs(cValue) >= GetMaxValue() or fabs(cValue) <= GetEpsilonValue())
		return;

	// Conversion
	sprintf(sValue, "%.15g", cValue);
	dRefValue = StandardStringToContinuous(sValue);
	dNewValue = StringToContinuous(sValue);

	// Calcul de la difference relative
	dDiff = (dRefValue - dNewValue) / cValue;

	// Message si difference superieur au seuil
	if (fabs(dDiff) > dThreshold or bShow)
	{
		cout << sValue << "\t";
		cout << MaxPrecisionDoubleToString(dRefValue) << "\t";
		cout << MaxPrecisionDoubleToString(dNewValue) << "\t";
		cout << MaxPrecisionDoubleToString(dDiff) << "\t";
		if (fabs(dDiff) > dThreshold)
			cout << "KO";
		cout << "\n";
	}
}

const char* const KWContinuous::MaxPrecisionDoubleToString(double dValue)
{
	char* sBuffer = StandardGetBuffer();

	sprintf(sBuffer, "%.15g", dValue);

	return sBuffer;
}

void KWContinuous::TestPerformanceStringToContinuous(int nMaxLowerBaseValue, double dMaxUpperBaseValue,
						     int nMaxExponent, boolean bRefConversion, boolean bNewConversion)
{
	char sValue[100];
	int nLowerBaseValue;
	ContinuousVector cvUpperBaseValues;
	int nExponent;
	Continuous cUpperBaseValue;
	int i;
	Continuous cValue;
	Timer timer;
	double dNumber;
	int n;
	int nNumberRepeat;

	require(nMaxLowerBaseValue > 0);
	require(dMaxUpperBaseValue > nMaxLowerBaseValue);
	require(nMaxExponent > 0);

	// Creation des valeurs hautes de base
	// en exploitant des puissances de 10, plus ou moins 1 (pour provoquer des arrondis)
	cvUpperBaseValues.Add(0);
	cUpperBaseValue = 10;
	while (cUpperBaseValue < nMaxLowerBaseValue)
		cUpperBaseValue *= 10;
	while (cUpperBaseValue <= dMaxUpperBaseValue)
	{
		cvUpperBaseValues.Add(cUpperBaseValue - 1);
		cvUpperBaseValues.Add(cUpperBaseValue);
		cvUpperBaseValues.Add(cUpperBaseValue + 1);
		cUpperBaseValue *= 10;
	}

	// Construction d'une grande variete de nombre de la forme
	//   (UpperBaseValue + UpperBaseValue) * 10^Exponent
	timer.Reset();
	timer.Start();
	nNumberRepeat = 10;
	debug(nNumberRepeat = 1);
	for (n = 0; n < nNumberRepeat; n++)
	{
		for (nExponent = 0; nExponent <= nMaxExponent; nExponent++)
		{
			for (i = 0; i < cvUpperBaseValues.GetSize(); i++)
			{
				cUpperBaseValue = cvUpperBaseValues.GetAt(i);
				for (nLowerBaseValue = 0; nLowerBaseValue < nMaxLowerBaseValue; nLowerBaseValue++)
				{
					// Nombre positif avec exposant positif
					cValue = (cUpperBaseValue + nLowerBaseValue) * dPositivePower10[nExponent];
					sprintf(sValue, "%.15g", cValue);
					if (bRefConversion)
						StandardStringToContinuous(sValue);
					if (bNewConversion)
						StringToContinuous(sValue);
					if (bRefConversion and bNewConversion)
						CompareStringToContinuous(cValue, false);

					// Nombre positif avec exposant negatif
					cValue = (cUpperBaseValue + nLowerBaseValue) * dNegativePower10[nExponent];
					sprintf(sValue, "%.15g", cValue);
					if (bRefConversion)
						StandardStringToContinuous(sValue);
					if (bNewConversion)
						StringToContinuous(sValue);
					if (bRefConversion and bNewConversion)
						CompareStringToContinuous(cValue, false);

					// Nombre negatif avec exposant positif
					cValue = -(cUpperBaseValue + nLowerBaseValue) * dPositivePower10[nExponent];
					sprintf(sValue, "%.15g", cValue);
					if (bRefConversion)
						StandardStringToContinuous(sValue);
					if (bNewConversion)
						StringToContinuous(sValue);
					if (bRefConversion and bNewConversion)
						CompareStringToContinuous(cValue, false);

					// Nombre negatif avec exposant negatif
					cValue = -(cUpperBaseValue + nLowerBaseValue) * dNegativePower10[nExponent];
					sprintf(sValue, "%.15g", cValue);
					if (bRefConversion)
						StandardStringToContinuous(sValue);
					if (bNewConversion)
						StringToContinuous(sValue);
					if (bRefConversion and bNewConversion)
						CompareStringToContinuous(cValue, false);
				}
			}
		}
	}
	timer.Stop();
	cout << "TIME\t";
	if (bRefConversion)
		cout << "Std";
	cout << "\t";
	if (bNewConversion)
		cout << "New";
	cout << "\t";
	dNumber = 4.0 * (nMaxExponent + 1) * cvUpperBaseValues.GetSize() * nMaxLowerBaseValue;
	cout << dNumber << "\t";
	cout << timer.GetElapsedTime() << "\t";
	cout << timer.GetElapsedTime() / dNumber << endl;
}

void KWContinuous::TestStringToContinuous()
{
	ContinuousVector cvValues;
	int i;
	Continuous cValue;
	Continuous cBaseValue;
	Timer timer;
	boolean bPerformanceTest;
	int nMaxLowerBaseValue;
	double dMaxUpperBaseValue;
	int nMaxExponent;

	cout << "\nTest String to Continuous" << endl;
	cout << "Value\tOld conv.\tNew conv.\tDiff\tKO" << endl;
	CompareStringToContinuous(123456789012.345, true);

	// Creation de valeurs de base
	cvValues.Add(0);
	cvValues.Add(0.1);
	cvValues.Add(0.12);
	cvValues.Add(0.123);
	cvValues.Add(0.1234);
	cvValues.Add(0.12345);
	cvValues.Add(0.123456);
	cvValues.Add(0.1234567);
	cvValues.Add(0.12345678);
	cvValues.Add(0.123456789);
	cvValues.Add(0.1234567890);
	cvValues.Add(0.12345678901);
	cvValues.Add(0.123456789012);
	cvValues.Add(0.1234567890123);
	cvValues.Add(0.12345678901234);
	cvValues.Add(0.123456789012345);
	cvValues.Add(0.12345678901);
	cvValues.Add(0.12345678902);
	cvValues.Add(0.12345678903);
	cvValues.Add(0.12345678904);
	cvValues.Add(0.12345678905);
	cvValues.Add(0.12345678906);
	cvValues.Add(0.12345678907);
	cvValues.Add(0.12345678908);
	cvValues.Add(0.12345678909);

	// Trois valeurs tres proches
	CompareStringToContinuous(-0.96842050000000002, true);
	CompareStringToContinuous(-0.96842049999999991, true);
	CompareStringToContinuous(-0.9684205, true);

	// Quelques valeurs particulieres
	cBaseValue = 1e-3;
	CompareStringToContinuous(0.1 * cBaseValue, true);
	CompareStringToContinuous(0.11 * cBaseValue, true);
	CompareStringToContinuous(0.01 * cBaseValue, true);
	CompareStringToContinuous(0.011 * cBaseValue, true);
	CompareStringToContinuous(-0.1 * cBaseValue, true);
	CompareStringToContinuous(-0.11 * cBaseValue, true);
	CompareStringToContinuous(-0.01 * cBaseValue, true);
	CompareStringToContinuous(-0.011 * cBaseValue, true);

	// Affichage de base
	for (i = 0; i < cvValues.GetSize(); i++)
	{
		cValue = cvValues.GetAt(i);
		CompareStringToContinuous(cValue, true);
		CompareStringToContinuous(cValue - 1e-15, true);
		CompareStringToContinuous(cValue + 1e-15, true);
		CompareStringToContinuous(cValue - 1e-10, true);
		CompareStringToContinuous(cValue + 1e-10, true);
		cout << "\n";
	}

	// Affichage des valeurs negatives
	for (i = 0; i < cvValues.GetSize(); i++)
	{
		cValue = -cvValues.GetAt(i);
		CompareStringToContinuous(cValue, true);
		CompareStringToContinuous(cValue - 1e-15, true);
		CompareStringToContinuous(cValue + 1e-15, true);
		CompareStringToContinuous(cValue - 1e-10, true);
		CompareStringToContinuous(cValue + 1e-10, true);
		cout << "\n";
	}

	// Affichage des puissances positives
	cValue = 0.123456789012345;
	for (i = 0; i <= 105; i++)
	{
		if (i <= 20 or i >= 95)
			CompareStringToContinuous(cValue, true);
		cValue *= 10;
	}

	// Affichage des puissances negatives
	cValue = 0.123456789012345;
	for (i = 0; i <= 105; i++)
	{
		if (i <= 20 or i >= 95)
			CompareStringToContinuous(cValue, true);
		cValue /= 10;
	}

	////////////////////////////
	// Test de performance

	bPerformanceTest = true;
	if (bPerformanceTest)
	{
		nMaxLowerBaseValue = 100;
		dMaxUpperBaseValue = 1e10;
		nMaxExponent = 20;
		cout << "Performance test" << endl;
		cout << "\t\t\tNumber\tTime\tUnit time" << endl;
		TestPerformanceStringToContinuous(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, false, false);
		TestPerformanceStringToContinuous(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, true, false);
		TestPerformanceStringToContinuous(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, false, true);
		TestPerformanceStringToContinuous(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, true, true);
	}
}

void KWContinuous::CompareContinuousToString(Continuous cValue, boolean bShow)
{
	const double dThreshold = 1.0000002e-9;
	const char* sRefValue;
	const char* sNewValue;
	double dRefValue;
	double dNewValue;
	double dDiff;

	// Pas de controle dans les cas extremes
	if (fabs(cValue) >= KWContinuous::GetMaxValue() or fabs(cValue) <= KWContinuous::GetEpsilonValue())
		return;

	// Conversion
	sRefValue = StandardContinuousToString(cValue);
	sNewValue = ContinuousToString(cValue);

	// Calcul de la differnce relative
	dRefValue = atof(sRefValue);
	dNewValue = atof(sNewValue);
	dDiff = (dRefValue - dNewValue) / cValue;

	// Message si difference superieur au seuil
	if (fabs(dDiff) > dThreshold or bShow)
	{
		cout << MaxPrecisionDoubleToString(cValue) << "\t";
		cout << sRefValue << "\t";
		cout << sNewValue << "\t";
		if (strcmp(sNewValue, sRefValue) != 0)
			cout << "ko";
		cout << "\t";
		if (strcmp(sNewValue, StandardContinuousToString(KWContinuous::DoubleToContinuous(cValue))) != 0)
			cout << "KO";
		cout << "\t";
		dRefValue = atof(sRefValue);
		dNewValue = atof(sNewValue);
		dDiff = (dRefValue - dNewValue) / cValue;
		cout << MaxPrecisionDoubleToString(dDiff) << "\t";
		cout << "\n";
	}
}

void KWContinuous::TestPerformanceContinuousToString(int nMaxLowerBaseValue, double dMaxUpperBaseValue,
						     int nMaxExponent, boolean bRefConversion, boolean bNewConversion)
{
	int nLowerBaseValue;
	ContinuousVector cvUpperBaseValues;
	int nExponent;
	Continuous cUpperBaseValue;
	int i;
	Continuous cValue;
	Timer timer;
	double dNumber;
	int n;
	int nNumberRepeat;

	require(nMaxLowerBaseValue > 0);
	require(dMaxUpperBaseValue > nMaxLowerBaseValue);
	require(nMaxExponent > 0);

	// Creation des valeurs hautes de base
	// en exploitant des puissances de 10, plus ou moins 1 (pour provoquer des arrondis)
	cvUpperBaseValues.Add(0);
	cUpperBaseValue = 10;
	while (cUpperBaseValue < nMaxLowerBaseValue)
		cUpperBaseValue *= 10;
	while (cUpperBaseValue <= dMaxUpperBaseValue)
	{
		cvUpperBaseValues.Add(cUpperBaseValue - 1);
		cvUpperBaseValues.Add(cUpperBaseValue);
		cvUpperBaseValues.Add(cUpperBaseValue + 1);
		cUpperBaseValue *= 10;
	}

	// Construction d'une grande variete de nombre de la forme
	//   (UpperBaseValue + UpperBaseValue) * 10^Exponent
	timer.Reset();
	timer.Start();
	nNumberRepeat = 10;
	debug(nNumberRepeat = 1);
	for (n = 0; n < nNumberRepeat; n++)
	{
		for (nExponent = 0; nExponent <= nMaxExponent; nExponent++)
		{
			for (i = 0; i < cvUpperBaseValues.GetSize(); i++)
			{
				cUpperBaseValue = cvUpperBaseValues.GetAt(i);
				for (nLowerBaseValue = 0; nLowerBaseValue < nMaxLowerBaseValue; nLowerBaseValue++)
				{
					// Nombre positif avec exposant positif
					cValue = (cUpperBaseValue + nLowerBaseValue) * dPositivePower10[nExponent];
					if (bRefConversion)
						StandardContinuousToString(cValue);
					if (bNewConversion)
						ContinuousToString(cValue);
					if (bRefConversion and bNewConversion)
						CompareContinuousToString(cValue, false);

					// Nombre positif avec exposant negatif
					cValue = (cUpperBaseValue + nLowerBaseValue) * dNegativePower10[nExponent];
					if (bRefConversion)
						StandardContinuousToString(cValue);
					if (bNewConversion)
						ContinuousToString(cValue);
					if (bRefConversion and bNewConversion)
						CompareContinuousToString(cValue, false);

					// Nombre negatif avec exposant positif
					cValue = -(cUpperBaseValue + nLowerBaseValue) * dPositivePower10[nExponent];
					if (bRefConversion)
						StandardContinuousToString(cValue);
					if (bNewConversion)
						ContinuousToString(cValue);
					if (bRefConversion and bNewConversion)
						CompareContinuousToString(cValue, false);

					// Nombre negatif avec exposant negatif
					cValue = -(cUpperBaseValue + nLowerBaseValue) * dNegativePower10[nExponent];
					if (bRefConversion)
						StandardContinuousToString(cValue);
					if (bNewConversion)
						ContinuousToString(cValue);
					if (bRefConversion and bNewConversion)
						CompareContinuousToString(cValue, false);
				}
			}
		}
	}
	timer.Stop();
	cout << "TIME\t";
	if (bRefConversion)
		cout << "Std";
	cout << "\t";
	if (bNewConversion)
		cout << "New";
	cout << "\t";
	dNumber = 4.0 * (nMaxExponent + 1) * cvUpperBaseValues.GetSize() * nMaxLowerBaseValue;
	cout << dNumber << "\t";
	cout << timer.GetElapsedTime() << "\t";
	cout << timer.GetElapsedTime() / dNumber << endl;
}

void KWContinuous::TestContinuousToString()
{
	ContinuousVector cvValues;
	int i;
	Continuous cValue;
	Continuous cBaseValue;
	Timer timer;
	boolean bPerformanceTest;
	int nMaxLowerBaseValue;
	double dMaxUpperBaseValue;
	int nMaxExponent;

	cout << "\nTest Continuous to String" << endl;
	cout << "Value\tOld conv.\tNew conv.\tko\tKO\tDiff" << endl;

	CompareContinuousToString(10000000.065, true);

	// Creation de valeurs de base
	cvValues.Add(0);
	cvValues.Add(0.1);
	cvValues.Add(0.12);
	cvValues.Add(0.123);
	cvValues.Add(0.1234);
	cvValues.Add(0.12345);
	cvValues.Add(0.123456);
	cvValues.Add(0.1234567);
	cvValues.Add(0.12345678);
	cvValues.Add(0.123456789);
	cvValues.Add(0.1234567890);
	cvValues.Add(0.12345678901);
	cvValues.Add(0.123456789012);
	cvValues.Add(0.1234567890123);
	cvValues.Add(0.12345678901234);
	cvValues.Add(0.123456789012345);

	// Quelques valeurs particulieres
	cBaseValue = 1e-3;
	CompareContinuousToString(0.1 * cBaseValue, true);
	CompareContinuousToString(0.11 * cBaseValue, true);
	CompareContinuousToString(0.01 * cBaseValue, true);
	CompareContinuousToString(0.011 * cBaseValue, true);
	CompareContinuousToString(-0.1 * cBaseValue, true);
	CompareContinuousToString(-0.11 * cBaseValue, true);
	CompareContinuousToString(-0.01 * cBaseValue, true);
	CompareContinuousToString(-0.011 * cBaseValue, true);

	// Affichage de base
	for (i = 0; i < cvValues.GetSize(); i++)
	{
		cValue = cvValues.GetAt(i);
		CompareContinuousToString(cValue, true);
		CompareContinuousToString(cValue - 1e-15, true);
		CompareContinuousToString(cValue + 1e-15, true);
		CompareContinuousToString(cValue - 1e-10, true);
		CompareContinuousToString(cValue + 1e-10, true);
		cout << "\n";
	}

	// Affichage des valeurs negatives
	for (i = 0; i < cvValues.GetSize(); i++)
	{
		cValue = -cvValues.GetAt(i);
		CompareContinuousToString(cValue, true);
		CompareContinuousToString(cValue - 1e-15, true);
		CompareContinuousToString(cValue + 1e-15, true);
		CompareContinuousToString(cValue - 1e-10, true);
		CompareContinuousToString(cValue + 1e-10, true);
		cout << "\n";
	}

	// Affichage des puissances positives
	cValue = 0.123456789012345;
	for (i = 0; i <= 105; i++)
	{
		if (i <= 20 or i >= 95)
			CompareContinuousToString(cValue, true);
		cValue *= 10;
	}

	// Affichage des puissances negatives
	cValue = 0.123456789012345;
	for (i = 0; i <= 105; i++)
	{
		if (i <= 20 or i >= 95)
			CompareContinuousToString(cValue, true);
		cValue /= 10;
	}

	////////////////////////////
	// Test de performance

	bPerformanceTest = true;
	if (bPerformanceTest)
	{
		nMaxLowerBaseValue = 100;
		dMaxUpperBaseValue = 1e10;
		nMaxExponent = 20;
		cout << "Performance test" << endl;
		cout << "\t\t\tNumber\tTime\tUnit time" << endl;
		TestPerformanceContinuousToString(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, false, false);
		TestPerformanceContinuousToString(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, true, false);
		TestPerformanceContinuousToString(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, false, true);
		TestPerformanceContinuousToString(nMaxLowerBaseValue, dMaxUpperBaseValue, nMaxExponent, true, true);
	}
}

/////////////////////////////////////////////
// Implementation de la classe ContinuousObject

ContinuousObject* ContinuousObject::Clone() const
{
	ContinuousObject* coClone;

	coClone = new ContinuousObject;
	coClone->SetContinuous(GetContinuous());
	return coClone;
}

longint ContinuousObject::GetUsedMemory() const
{
	return sizeof(ContinuousObject);
}

const ALString ContinuousObject::GetClassLabel() const
{
	return "Numerical object";
}

int ContinuousObjectCompare(const void* elem1, const void* elem2)
{
	return KWContinuous::Compare(cast(ContinuousObject*, *(Object**)elem1)->GetContinuous(),
				     cast(ContinuousObject*, *(Object**)elem2)->GetContinuous());
}

/////////////////////////////////////////////
// Implementation de la classe ContinuousVector

int ContinuousVectorCompareValue(const void* elem1, const void* elem2)
{
	Continuous cValue1;
	Continuous cValue2;

	// Acces aux valeurs
	cValue1 = *(Continuous*)elem1;
	cValue2 = *(Continuous*)elem2;

	// Comparaison
	if (cValue1 == cValue2)
		return 0;
	else
		return cValue1 > cValue2 ? 1 : -1;
}

ContinuousVector::~ContinuousVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void ContinuousVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void ContinuousVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void ContinuousVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, ContinuousVectorCompareValue);
}

void ContinuousVector::Shuffle()
{
	int i;
	int iSwap;
	Continuous cSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		cSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, cSwappedValue);
	}
}

void ContinuousVector::CopyFrom(const ContinuousVector* cvSource)
{
	require(cvSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, cvSource->pData.hugeVector,
			    cvSource->nSize, cvSource->nAllocSize);
}

ContinuousVector* ContinuousVector::Clone() const
{
	ContinuousVector* cvClone;

	cvClone = new ContinuousVector;

	// Recopie
	cvClone->CopyFrom(this);
	return cvClone;
}

boolean ContinuousVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void ContinuousVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;
	int i;

	ost << GetClassLabel() + " (size=" << GetSize() << ")\n";
	for (i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t" << GetAt(i) << "\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint ContinuousVector::GetUsedMemory() const
{
	return sizeof(ContinuousVector) + nAllocSize * sizeof(Continuous);
}

const ALString ContinuousVector::GetClassLabel() const
{
	return "Numerical vector";
}

void ContinuousVector::Test()
{
	ContinuousVector* cvTest;
	ContinuousVector* cvClone;
	int nSize;
	int i;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 10000, 1000);
	cvTest = new ContinuousVector;
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
		cvTest->SetAt(i, 1);
	cout << *cvTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	cvClone = cvTest->Clone();
	cout << *cvClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 10000, 2000);
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 10000, 1000);
	cvTest->SetSize(nSize);
	cvTest->Initialize();
	cout << *cvTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		cvTest->Add(0);
	cout << *cvTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
		cvTest->SetAt(i, (Continuous)RandomDouble());
	cout << *cvTest << endl;

	// Tri
	cout << "Tri" << endl;
	cvTest->Sort();
	cout << *cvTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	cvTest->Shuffle();
	cout << *cvTest << endl;

	// Liberations
	delete cvClone;
	delete cvTest;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Continuous

PLShared_Continuous::PLShared_Continuous()
{
	cContinuousValue = 0;
}

PLShared_Continuous::~PLShared_Continuous() {}

void PLShared_Continuous::Write(ostream& ost) const
{
	ost << KWContinuous::ContinuousToString(cContinuousValue);
}

void PLShared_Continuous::Test()
{
	Continuous cValue1;
	PLShared_Continuous shared_symbol;
	Continuous cValue2;
	PLSerializer serializer;

	// Initialisation
	cValue1 = 1;
	shared_symbol = cValue1;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_symbol.Serialize(&serializer);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_symbol.Deserialize(&serializer);
	serializer.Close();
	cValue2 = shared_symbol;

	// Affichage
	cout << "Value 1 " << cValue1 << endl;
	cout << "Value 2 " << cValue2 << endl;
}

void PLShared_Continuous::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutDouble(cContinuousValue);
}

void PLShared_Continuous::DeserializeValue(PLSerializer* serializer)
{
	cContinuousValue = serializer->GetDouble();
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_ContinuousVector

PLShared_ContinuousVector::PLShared_ContinuousVector() {}

PLShared_ContinuousVector::~PLShared_ContinuousVector() {}

void PLShared_ContinuousVector::Test()
{
	ContinuousVector cvVector1;
	ContinuousVector cvVector2;
	PLShared_ContinuousVector shared_symbolVector;
	PLSerializer serializer;
	int i;

	// Initialisation
	for (i = 0; i < 100; i++)
		cvVector1.Add(i);

	// Serialisation
	shared_symbolVector.SetContinuousVector(cvVector1.Clone());
	serializer.OpenForWrite(NULL);
	shared_symbolVector.bIsDeclared = true;
	shared_symbolVector.Serialize(&serializer);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_symbolVector.Deserialize(&serializer);
	serializer.Close();
	cvVector2.CopyFrom(shared_symbolVector.GetContinuousVector());

	// Affichage
	cout << "Vector 1" << endl;
	cout << cvVector1 << endl << endl;
	cout << "Vector 2" << endl;
	cout << cvVector2 << endl;
}

void PLShared_ContinuousVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	ContinuousVector* cv;
	PLShared_Continuous sharedContinuous;
	int i;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	cv = cast(ContinuousVector*, o);

	// Serialisation du nombre de continuous
	serializer->PutInt(cv->GetSize());

	// Serialisation de chaque continuous
	for (i = 0; i < cv->GetSize(); i++)
	{
		sharedContinuous = cv->GetAt(i);
		sharedContinuous.Serialize(serializer);
	}
}

void PLShared_ContinuousVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	ContinuousVector* cv;
	PLShared_Continuous sharedContinuous;
	int i;
	int nSize;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	cv = cast(ContinuousVector*, o);

	// Deserialisation de la taille
	nSize = serializer->GetInt();

	// DeSerialisation de chaque continuous
	for (i = 0; i < nSize; i++)
	{
		sharedContinuous.Deserialize(serializer);
		cv->Add(sharedContinuous);
	}
}
