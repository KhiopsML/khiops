// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLTest.h"

int main(int argv, char** argc)
{
#if defined(USE_MPI)
// Pour indiquer les libraries a utiliser par le linker
// Potentiellement inutile apres utilisation de cmake
#pragma comment(lib, "msmpi")
#pragma comment(lib, "PLMPI")

	// Mise en place du fdriver parallel
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Chargement du driver pour l'acces aux fichiers distants (file://)
	if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or PLTaskDriver::GetFileServerOnSingleHost())
		SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

	// Verification des versions de chaque processus
	PLParallelTask::SetVersion("1.0");
	PLMPITaskDriver::CheckVersion();

#endif // defined(USE_MPI)

	// PLParallelTask::SetVerbose(true);
	//  PLParallelTask::SetTracerResources(1);
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
	PLParallelTask::RegisterTask(new PEProgressionTask);

	PLParallelTask::SetParallelSimulated(false);
	PLParallelTask::SetSimulatedSlaveNumber(4);

	// PLTaskDriver::SetFileServerOnSingleHost(true);

	// Nombre de process qui vont etre lances
	// RMResourceConstraints::SetMaxCoreNumber(25);
	// Affectation des handlers pour l'acces au fichiers
	SystemFileDriverCreator::RegisterExternalDrivers();

	if (PLParallelTask::IsMasterProcess())
	{
		PLParallelTask::GetDriver()->MasterInitializeResourceSystem();

		// MemSetAllocIndexExit(1717);

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
		// UIObject::SetUIMode(UIObject::Graphic);
		// UIObject::ParseMainParameters(argv, argc);
		// PEProtocolTestTask::Test();
		// PEProgressionTask::Test();
		// DoubleVector::Test2();

		// InputBufferedFile::WriteEolPos("/home/boeg7312/Desktop/~Census_without_header_chunk2.txt");
		// InputBufferedFile::Test(0);
		// InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Abalone/Abalone.txt", 0,
		// 0); InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Iris/Iris.txt", 0,
		// 0); InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Adult/Adult.txt", 0,
		// 0); InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Nova/Nova.txt", 0,
		// 0); InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Volkert/Volkert.txt",
		// 0, 0);
		// InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Digits/Digits.txt", 0,
		// 0); InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt",
		// 0, 0);

		// InputBufferedFile::Test(0);

		// InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt", true,
		// 7);

		//
		// InputBufferedFile file;
		// OutputBufferedFile oFile;
		// file.SetFileName("/home/boeg7312/Desktop/~Census_without_header_chunk2.txt");
		// file.SetBufferSize(8388608);
		// file.Open();
		// file.Fill(0);
		// file.SetBufferSize(5582986);
		// file.Fill(8388608);
		// file.Close();
		//
		// InputBufferedFile::TestCopy("/tmp/user/1000/8MB_10fields_file.txt", 128 * lKB);
		// InputBufferedFile::TestCopy("/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt", 8 *
		// lMB);
		//  InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 256 * lMB);
		//  InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 128 * lMB);
		//	InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 64 * lMB);
		//	InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 32 * lMB);
		//  InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 16 * lMB);
		//  InputBufferedFile::TestCopy("/tmp/99997lines_3hugeLines_file.txt", 128 * lKB);
		// InputBufferedFile::GetEolPos("/tmp/user/1000/100MB_10fields_file.txt");

		// PLTaskDriver::SetFileServerOnSingleHost(true);
		// PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		// PLParallelTask::GetDriver()->StartFileServers();
		// int nBufferSize = 128 * lMB;
		// while (nBufferSize >= 16 * lKB)
		// {
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt",
		// nBufferSize);
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Census/BigCensus.txt",
		// nBufferSize);
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Adult/Adult.txt",
		// nBufferSize); 	nBufferSize /= 2;
		// }
		// PLParallelTask::GetDriver()->StopFileServers();
		// PLTaskDriver::SetFileServerOnSingleHost(true);
		// PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		// PLParallelTask::GetDriver()->StartFileServers();
		// InputBufferedFile::Test(1);
		// InputBufferedFile::TestCount("/home/boeg7312/Documents/LearningTest/datasets/Census/BigCensus.txt", 8
		// * lMB); PLParallelTask::GetDriver()->StopFileServers();

		// PLTaskDriver::SetFileServerOnSingleHost(true);
		// PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		// PLParallelTask::GetDriver()->StartFileServers();
		// //
		// InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Adult/Adult.txt",
		// 100 * lKB);
		// // InputBufferedFile::Test(1);
		// int nBufferSize = 128 * lMB;
		// while (nBufferSize >= 16 * lKB)
		// {
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt",
		// nBufferSize);
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Census/BigCensus.txt",
		// nBufferSize);
		// 	InputBufferedFile::TestCopy("file://yd-F5DT4D3/home/boeg7312/Documents/LearningTest/datasets/Adult/Adult.txt",
		// nBufferSize); 	nBufferSize /= 2;
		// }
		// PLParallelTask::GetDriver()->StopFileServers();
		//           Parametrage du nom du module applicatif

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
		// InputBufferedFile::Test(0);
		// PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		// PLParallelTask::GetDriver()->StartFileServers();
		// InputBufferedFile::Test(1);
		// PLParallelTask::GetDriver()->StopFileServers();

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
		// searchTask.SeachString("/home/boeg7312/Documents/LearningTest/datasets/Census/S_Census.txt",
		// "1024855", "/tmp/res"); PEFileSearchTask::Test(); KWKeyPositionSampleExtractorTask::Test();
		// PEGrepTask::Test("1024855", "file://yd-CZC4510HN7/home/boeg7312/Documents/BigBigCensus.txt");
		// FileService::SetUserTmpDir("C:\\Temp");
		// Test();
		// PLSerializer::Test();
		// PESerializerTestTask::Test();
		// PESerializerLongTestTask::Test();
		// OutputBufferedFile::TestWriteFile("/home/boeg7312/Documents/LearningTest/datasets/Adult/Adult.txt",
		// "/tmp/Adult.txt", true);
		// OutputBufferedFile::TestWriteFile("/home/boeg7312/Documents/LearningTest/datasets/Census/Census.txt",
		// "/tmp/Census.txt", true);

		// PEPiView::Test(argv, argc);
		// PEIOParallelTestTask::Test();
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
		RMParallelResourceManager::Test();

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
