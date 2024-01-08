// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DiversTests.h"

void ShowKeyInfo()
{
	time_t lt;

	lt = time(NULL);
	cout << "=========================================================================" << endl;
	cout << ctime(&lt) << endl;
	cout << "OS: " << p_getenv("OS") << endl;
	cout << "PROCESSOR_IDENTIFIER: " << p_getenv("PROCESSOR_IDENTIFIER") << endl;
	cout << "PROCESSOR_REVISION: " << p_getenv("PROCESSOR_REVISION") << endl;
	cout << "MAC Address: " << GetMACAddress() << endl;
}

void TestMemAdvanced()
{
	ObjectArray oaTest;
	int i;
	ALString sTest =
	    "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
	StringObject soTest;
	int nMaxAlloc = 10000;

	soTest.SetString(sTest);

	// Affichage des stats sur la heap
	cout << "MemGetHeapMemory1: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	for (i = 0; i < nMaxAlloc; i++)
		oaTest.Add(soTest.Clone());
	cout << "MemGetHeapMemory2: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	for (i = 0; i < nMaxAlloc / 10; i++)
	{
		delete oaTest.GetAt(i * 10);
		oaTest.SetAt(i * 10, NULL);
	}
	cout << "MemGetHeapMemory3: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	oaTest.DeleteAll();
	cout << "MemGetHeapMemory4: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
}

////////////////////////////////////////////////////////////////////////////////
// Test avec des caracteres non ascii
//   . Ansi: e accent aigu
//   . AnsiAsUtf8: e accent aigu code utf8
//   . Utf8: theta
//
//  Ecriture dans un fichier binaire: ok
//
// Test d'affichage dans la fenetre de log java (ca plante avec cout):
//   . l'affichage dans java est correct: on voit le deux e accent aigu identiquement, et le theta correctement
//
// Test de creation de fichier
//   . avec le code page de la machine (ansi): creation des trois fichiers avec affichage des caractere en utf8
//      . le e accent aigu ansi s'affiche correctement
//      . les caracteres utf8 s'afficage avec deux caracteres ansi
//   . avec le code page utf8 (en commentant le code de p_SetApplicationLocale)
//      . plante avec le nom de fichier ansi
//         . devrait etre re-encode en utf8?
//      . si on supprime le nom ansi du test
//         . cree correctement les deux fichiers utf8, qui s'affichent correctement dans le file explorer
//         . par contre, le GetDirectoryContent renvoie un reencodage ansi des noms de fichier, en echouant
//         potentiellement
//           (Utf8?.txt pour le theta, au lieun des deux octets attendus)
//         . les api microsoft (de type FindNextFileA) sont delicates a utiliser, sans lien avec la gestion du code page
//           cf. https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilea
//           "The fileapi.h header defines FindNextFile as an alias which automatically selects the ANSI or Unicode
//           version
//            of this function based on the definition of the UNICODE preprocessor constant.
//           "The fileapi.h header defines FindNextFile as an alias which automatically selects the ANSI or Unicode
//           version
//            of this function based on the definition of the UNICODE preprocessor constant. Mixing usage of the
//            encoding-neutral alias with code that not encoding-neutral can lead to mismatches that result in
//            compilation or runtime errors."
void StudyCharacterEncodings()
{
	const ALString sRootPath = "C:\\temp";
	OutputBufferedFile logFile;
	OutputBufferedFile outputFile;
	StringVector svDirectoryNames;
	StringVector svFileNames;
	ALString sPathName;
	UICard testCard;
	ALString sAnsi;
	ALString sAnsiAsUtf8;
	ALString sUtf8;
	StringVector svEncodings;
	int i;
	ALString sTmp;

#ifdef _MSC_VER
// C4310: le cast tronque la valeur constante
#pragma warning(disable : 4310) // disable 4310 warning
#endif

	// Initialisation des chaines de caracteres
	sAnsi = sTmp + "Ansi_" + (char)(0xE9) + "_";
	sAnsiAsUtf8 = sTmp + "AnsiAsUtf8" + char(0xC3) + char(0xA9) + "_";
	sUtf8 = sTmp + "Utf8" + char(0xCE) + char(0xB8) + "_";
	svEncodings.Add(sAnsi);
	svEncodings.Add(sAnsiAsUtf8);
	svEncodings.Add(sUtf8);

#ifdef _MSC_VER
// C4310: le cast tronque la valeur constante
#pragma warning(default : 4310) // enable 4310 warning
#endif

	// Ecriture des caracteres dans un fichier de log
	logFile.SetFileName(FileService::BuildFilePathName(sRootPath, "LogEncodings.txt"));
	logFile.Open();
	if (logFile.IsOpened())
	{
		for (i = 0; i < svEncodings.GetSize(); i++)
			logFile.Write("Write chars: " + svEncodings.GetAt(i) + "\n");
	}

	// Affichage dans la fenetre de log java
	// (cela ne marche pas avec un "cout <<" dans la fenetre de console)
	UIObject::SetUIMode(UIObject::Graphic);
	for (i = 0; i < svEncodings.GetSize(); i++)
		Global::AddSimpleMessage(svEncodings.GetAt(i));

	// Affichage dans une boite de dialogue
	testCard.SetIdentifier("Encodings");
	testCard.SetLabel("Encodings");
	testCard.AddStringField("Text", "Chaine de caracteres", "");
	for (i = 0; i < svEncodings.GetSize(); i++)
	{
		testCard.SetStringValueAt("Text", svEncodings.GetAt(i));
		testCard.Open();
		logFile.Write("Show chars: " + svEncodings.GetAt(i) + " -> " + testCard.GetStringValueAt("Text") +
			      "\n");
	}

	// Creation de fichiers dont le nom comporte ces caracteres
	for (i = 0; i < svEncodings.GetSize(); i++)
	{
		// Creation du fichier
		outputFile.SetFileName(FileService::BuildFilePathName(sRootPath, svEncodings.GetAt(i) + ".txt"));
		outputFile.Open();
		if (outputFile.IsOpened())
		{
			outputFile.Write(svEncodings.GetAt(i) + "\n");
			outputFile.Close();
		}

		// Test si le fichier existe
		logFile.Write("Is file  " + outputFile.GetFileName() + ": ");
		logFile.Write(BooleanToString(FileService::FileExists(outputFile.GetFileName())));
		logFile.Write("\n");
	}

	// Parcours des fichiers
	FileService::GetDirectoryContent(sRootPath, &svDirectoryNames, &svFileNames);
	for (i = 0; i < svFileNames.GetSize(); i++)
	{
		if (svFileNames.GetAt(i).Find(".txt") != -1)
			logFile.Write("Found file: " + svFileNames.GetAt(i) + "\n");
	}

	// Fermeture du fichier de log
	if (logFile.IsOpened())
		logFile.Close();
}

///////////////////////////////////////////////////////
// Classe avec ses propres methode d'allocation
class MemoryTest
{
public:
	// Constructeur
	MemoryTest()
	{
		nTest = 0;
		cout << "MemoryTest()\n";
	};
	~MemoryTest()
	{
		cout << "~MemoryTest()\n";
	};

	// Allocateur
	void* operator new(size_t sz)
	{
		cout << "new MemoryTest " << sz << " bytes\n";
		return ::operator new(sz);
	}

	void operator delete(void* p)
	{
		cout << "delete MemoryTest\n";
		::operator delete(p);
	}

protected:
	int nTest;
};

class SubMemoryTest : public MemoryTest
{
public:
	// Constructeur
	SubMemoryTest()
	{
		cout << "SubMemoryTest()\n";
	};
	~SubMemoryTest()
	{
		cout << "~SubMemoryTest()\n";
	};

protected:
	ALString sTest;
};

void StudyMemoryManagement()
{
	MemoryTest memTest1;
	MemoryTest* memTest2;
	SubMemoryTest* subMemTest;

	cout << "MemoryTest: " << sizeof(MemoryTest) << endl;
	memTest2 = new MemoryTest;
	delete memTest2;
	subMemTest = new SubMemoryTest;
	delete subMemTest;
}
