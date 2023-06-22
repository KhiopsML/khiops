// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLTest.h"

int main(int argv, char** argc)
{

#ifdef USE_MPI
	PLParallelTask::UseMPI("1.0");
#endif // USE_MPI

	PLParallelTask::SetVerbose(true);
	// PLParallelTask::SetTracerResources(1);
	// PLParallelTask::SetTracerMPIActive(true);
	// PLParallelTask::SetTracerProtocolActive(true);

	// Declaration des taches paralleles
	PLParallelTask::RegisterTask(new PEFileSearchTask);
	PLParallelTask::RegisterTask(new PEHelloWorldTask);
	PLParallelTask::RegisterTask(new PEIOParallelTestTask);
	PLParallelTask::RegisterTask(new PEPiTask);
	PLParallelTask::RegisterTask(new PEProtocolTestTask);
	PLParallelTask::RegisterTask(new PESerializerTestTask);
	PLParallelTask::RegisterTask(new PESerializerLongTestTask);
	PLParallelTask::RegisterTask(new PELullabyTask);

	PLParallelTask::SetParallelSimulated(false);
	PLParallelTask::SetSimulatedSlaveNumber(4);

	// longint lMax = 1 * lMB;
	// while (lMax <= 1 * lGB)
	// {
	// 	PLParallelTask::TestComputeStairBufferSize((int)lMB, (int)lMax, 1 * lGB, 8);
	// 	lMax = lMax * 2;
	// }

	// PLTaskDriver::SetFileServerOnSingleHost(true);

	// Nombre de process qui vont etre lances
	// RMResourceConstraints::SetMaxCoreNumber(25);
	// Affectation des handlers pour l'acces au fichiers
	SystemFileDriverCreator::RegisterDrivers();

	if (PLParallelTask::IsMasterProcess())
	{

		// MemSetAllocIndexExit(1382);

		// Utilisation d'un stream dedie au batch
		// ofstream
		// batchCout("C:\\Users\\boeg7312\\Documents\\dvpt\\KhiopsMPI\\ParallelTest\\RMParallelResourceManager\\ref.txt");

		// Sauvegarde du buffer associe a cout
		// streambuf *coutBuf = std::cout.rdbuf();

		// Redirection de cout vers le stream dedie a ubatch
		// cout.rdbuf(batchCout.rdbuf());
		/*cout << "break" << endl;
		char c;
		cin >> c;*/

		// Mise en place des repertoires temporaires
		FileService::SetApplicationName("ParallelTests");

		// Mode maitre
		UIObject::SetUIMode(UIObject::Textual);

		PLTaskDriver::SetFileServerOnSingleHost(true);
		PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		PLParallelTask::GetDriver()->StartFileServers();
		// InputBufferedFile::Test(1);
		InputBufferedFile::TestCount("/home/boeg7312/Documents/LearningTest/datasets/Census/BigCensus.txt",
					     8 * lMB);
		PLParallelTask::GetDriver()->StopFileServers();

		//	InputBufferedFile::Test(0);
		// Parametrage du nom du module applicatif

		// Parametrage de l'arret de l'allocateur

		// MemSetAllocSizeExit(488);
		// MemSetAllocBlockExit((void*)0x00F42044);

		// Analyse de la ligne de commande
		// UIObject::ParseMainParameters(argv, argc);
		// if (UIObject::IsBatchMode())
		// 	UIObject::SetUIMode(UIObject::Textual);
		// else
		// 	UIObject::SetUIMode(UIObject::Graphic);

		// Lancement de l'outil

		// InputBufferedFile2::Test2((const char*)argc[1]);
		//  Lancement de la partie maitre ou esclave

		// PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		// PLParallelTask::GetDriver()->StartFileServers();
		// InputBufferedFile::Test(1);
		// PLParallelTask::GetDriver()->StopFileServers();

		// InputBufferedFile::GetFileDriverCreator()->SetDriverHDFS(new BufferedFileDriverHDFS);
		// InputBufferedFile::Test(2);
		// if (HDFSFileSystem::IsConnected())
		// 		HDFSFileSystem::Disconnect();
		//	delete SystemFileDriverCreator::GetDriverHDFS();

		/*ALString sFileTest = (const char*)argc[1];
		cout << "file " << sFileTest << endl;
		InputBufferedFile::TestCount(sFileTest, 8 * lMB);*/
		// delete InputBufferedFile::GetFileDriverCreator()->GetDriverHDFS();
		// InputBufferedFile::Test2((const char*)argc[1]);
		// PLParallelTask::GetDriver()->StopFileServers();
		// PEGrepTask::Test("IdF", "/home/boeg7312/Documents/AllTarget.txt");
		// PEGrepTask::Test("1024855", "/home/boeg7312/Documents/BigBigCensus.txt");
		// PEFileSearchTask searchTask;
		// searchTask.SeachString("/home/boeg7312/Documents/BigBigCensus.txt", "1024855", "/tmp/res");
		// PEFileSearchTask::Test();
		// KWKeyPositionSampleExtractorTask::Test();
		// PEGrepTask::Test("1024855", "file://yd-CZC4510HN7/home/boeg7312/Documents/BigBigCensus.txt");
		// FileService::SetUserTmpDir("C:\\Temp");
		// Test();
		// PLSerializer::Test();
		// PESerializerTestTask::Test();
		// PESerializerLongTestTask::Test();

		// PEPiView::Test(argv, argc);
		//  PEIOParallelTestTask::Test();
		// PEProtocolTestTask::Test();
		// PEProtocolTestTask::TestFatalError(5);
		// PELullabyTask::Test();
		//  PEFileSearchTask::Test();
		//  PLShared_TaskResourceGrant::Test();
		//  PEHelloWorldTask::Test();
		//  PLSharedVariable::Test();

		// PLShared_ObjectArray::Test();
		// PLShared_ObjectList::Test();
		// PLShared_ObjectDictionary::Test();
		// PLShared_StringVector::Test();
		// PLShared_IntVector::Test();
		// PLShared_DoubleVector::Test();
		// PLShared_CharVector::Test();
		// PLShared_LongintVector::Test();

		// PLKnapsackProblem::Test();
		// PLKnapsackProblemTest::Test();
		// PLShared_TaskResourceGrant::Test();
		// RMParallelResourceManager::Test();

		// PLShared_ResourceRequirement::Test();

		PLParallelTask::GetDriver()->StopSlaves();

		// On restitue cout dans son etat initial
		// cout.rdbuf(coutBuf);
	}
	// Mode esclave
	else
	{
		// Parametrage de l'arret de l'allocateur
		/*	if (GetProcessId() == 1)
				MemSetAllocIndexExit(665);*/

		// Lancement de l'esclave
		PLParallelTask::GetDriver()->StartSlave();
	}
	// Liberation des drivers de fichier
	SystemFileDriverCreator::UnregisterDrivers();
#ifdef __HADOOP__
	// if (SystemFileDriverCreator::GetDriverHDFS() != NULL)
	// 	SystemFileDriverCreator::GetDriverHDFS()->Disconnect();
#endif

	PLParallelTask::DeleteAllTasks();
	return 0;
}

#ifdef KWLearningBatchMode
/********************************************************************
 * Le source suivant permet de compiler des sources developpes avec *
 * l'environnement Norm, d'utiliser le mode UIObject::Textual et    *
 * de ne pas linker avec jvm.lib (a eviter absoluement).            *
 * Moyennant ces conditions, on peut livrer un executable en mode   *
 * textuel ne necessitant pas l'intallation prealable du JRE Java   *
 ********************************************************************/

extern "C"
{
#ifdef _MSC_VER
	int __stdcall _imp__JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(0);
	}
#endif // _MSC_VER

#ifdef __UNIX__
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(0);
	}
#endif // __UNIX__
}
#endif // KWLearningBatchMode